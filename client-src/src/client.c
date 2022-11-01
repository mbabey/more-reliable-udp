#include "../../libs/include/error.h"
#include "../../libs/include/manager.h"
#include "../../libs/include/util.h"
#include "../include/client.h"
#include "../include/setup.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define MAX_TIMEOUT 3
#define OFFSET 0
#define BASE_TIMEOUT 10

static volatile sig_atomic_t running; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables) : var must change

static void set_signal_handling(struct sigaction *sa);

static void signal_handler(int sig);

void connect_client(struct client_settings *settings);

void init_connect(struct client_settings *settings);

void messaging_service(struct client_settings *settings);

void create_packet(struct packet *packet, uint8_t flag, uint8_t seq_num, uint16_t len, uint8_t *payload);

void send_msg(struct packet *send_packet, struct client_settings *settings);

void await_response(struct client_settings *settings, uint8_t *serialized_packet, size_t packet_size);

void read_msg(struct client_settings *settings, char **msg);

void
handle_recv_timeout(struct client_settings *settings, uint8_t *serialized_packet, size_t packet_size, int num_timeouts);

bool process_response(struct client_settings *settings, const uint8_t *recv_buffer, const uint8_t *send_buffer,
                      size_t packet_size);

void retransmit_packet(struct client_settings *settings, const uint8_t *serialized_packet, size_t packet_size);

void read_file(char **msg, const char *input);

_Noreturn void close_client(struct client_settings *settings, int exit_code);

void run_client(int argc, char *argv[], struct client_settings *settings)
{
    init_def_state(argc, argv, settings);
    
    connect_client(settings);
    init_connect(settings);
    messaging_service(settings);
    
    close_client(settings, EXIT_SUCCESS);
}

void connect_client(struct client_settings *settings)
{
    settings->addr = (struct sockaddr_in *) s_calloc(1, sizeof(struct sockaddr_in), __FILE__, __func__, __LINE__);
    if (errno == ENOTRECOVERABLE)
    {
        free_mem_manager(settings->mem_manager);
        exit(EXIT_SUCCESS); // NOLINT(concurrency-mt-unsafe) : no threads here
    }
    settings->mem_manager->mm_add(settings->mem_manager, settings->addr);
    
    if ((settings->server_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) // NOLINT(android-cloexec-socket) : SOCK_CLOEXEC dne
    {
        fatal_errno(__FILE__, __func__, __LINE__, errno);
        close_client(settings, EXIT_FAILURE);
    }
    
    settings->addr->sin_family      = AF_INET;
    settings->addr->sin_port        = 0;
    if ((settings->addr->sin_addr.s_addr = inet_addr(settings->client_ip)) == (in_addr_t) -1)
    {
        fatal_errno(__FILE__, __func__, __LINE__, errno);
        close_client(settings, EXIT_FAILURE);
    }
    
    settings->to_addr.sin_family      = AF_INET;
    settings->to_addr.sin_port        = htons(settings->server_port);
    if ((settings->to_addr.sin_addr.s_addr = inet_addr(settings->server_ip)) == (in_addr_t) -1)
    {
        fatal_errno(__FILE__, __func__, __LINE__, errno);
        close_client(settings, EXIT_FAILURE);
    }
    
    if (bind(settings->server_fd, (struct sockaddr *) settings->addr, sizeof(struct sockaddr_in)) == -1)
    {
        fatal_errno(__FILE__, __func__, __LINE__, errno);
        close_client(settings, EXIT_FAILURE);
    }
    
    printf("\nCONNECTING TO SERVER %s:%d\n\n", settings->server_ip, settings->server_port);
}

void init_connect(struct client_settings *settings)
{
    struct packet send_packet;
    
    memset(&send_packet, 0, sizeof(struct packet));
    
    create_packet(&send_packet, FLAG_SYN, MAX_SEQ, 0, NULL);
    send_msg(&send_packet, settings);
    create_packet(&send_packet, FLAG_ACK, MAX_SEQ, 0, NULL);
    send_msg(&send_packet, settings);
}

void messaging_service(struct client_settings *settings)
{
    struct packet send_packet;
    
    uint8_t seq = (uint8_t) 0;
    
    char *msg = NULL;
    
    struct sigaction sa;
    set_signal_handling(&sa);
    running = 1;
    
    while (running)
    {
        read_msg(settings, &msg);
        
        if (msg != NULL)
        {
            create_packet(&send_packet, FLAG_PSH, seq, strlen(msg), (uint8_t *) msg);
            seq++;
            send_msg(&send_packet, settings);
            settings->mem_manager->mm_free(settings->mem_manager, msg);
        }
        
        msg = NULL;
    }
    
    create_packet(&send_packet, FLAG_FIN, MAX_SEQ, 0, NULL);
    send_msg(&send_packet, settings);
    create_packet(&send_packet, FLAG_FIN | FLAG_ACK, MAX_SEQ, 0, NULL); // NOLINT(hicpp-signed-bitwise) : highest order bit unused
    send_msg(&send_packet, settings);
}

void read_msg(struct client_settings *settings, char **msg)
{
    printf("----- MESSAGING SERVICE -----\n\n");
    
    char input[BUF_LEN];

    printf("%s", (settings->is_file) ?
                 "Please enter the name of the file you wish to send: " :
                 "Please enter the string you wish to send: ");
    
    if (fgets(input, BUF_LEN, stdin) == NULL)
    {
        if (errno == EINTR || feof(stdin))
        {
            return;
        }
    }
    
    input[strcspn(input, "\n")] = '\0';
    
    printf("\n");
    
    if (settings->is_file)
    {
        printf("-- OPENING FILE --\n");
        read_file(msg, input);
    } else
    {
        set_string(msg, input);
    }
    
    if (errno == ENOTRECOVERABLE)
    {
        close_client(settings, EXIT_FAILURE);
    }
    settings->mem_manager->mm_add(settings->mem_manager, *msg);
    printf("Sending message: %s\n\n", *msg);
}

void read_file(char **msg, const char *input)
{
    int     fd;
    ssize_t len;
    
    if ((fd = do_open(input, O_RDONLY)) == -1)
    {
        *msg = NULL;
        return;
    }
    
    if ((len = do_lseek(fd, OFFSET, SEEK_END) + 1) == 0)
    {
        *msg = NULL;
        return;
    }
    if (do_lseek(fd, OFFSET, SEEK_SET) == -1)
    {
        *msg = NULL;
        return;
    }
    
    *msg = (char *) s_calloc(len, sizeof(char), __FILE__, __func__, __LINE__);
    if (errno == ENOTRECOVERABLE)
    {
        return;
    }
    
    if (do_read(fd, *msg, len - 1) == -1)
    {
        *msg = NULL;
        return;
    }
    printf("File content: %s\n\n", *msg);
    do_close(input, fd);
    if (errno == EIO)
    {
        fatal_errno(__FILE__, __func__, __LINE__, errno);
    }
}

void send_msg(struct packet *send_packet, struct client_settings *settings)
{
    socklen_t sockaddr_in_size;
    uint8_t   *serialized_packet;
    size_t    packet_size;
    
    sockaddr_in_size  = sizeof(struct sockaddr_in);
    serialized_packet = serialize_packet(send_packet);
    if (errno == ENOTRECOVERABLE)
    {
        close_client(settings, EXIT_FAILURE);
    }
    settings->mem_manager->mm_add(settings->mem_manager, serialized_packet);
    
    packet_size       = sizeof(send_packet->flags) + sizeof(send_packet->seq_num) + sizeof(send_packet->length) +
                        send_packet->length;
    
    if (sendto(settings->server_fd, serialized_packet, packet_size, 0,
               (struct sockaddr *) &settings->to_addr, sockaddr_in_size) == -1)
    {
        if (send_packet->flags == FLAG_SYN) // If we are sending the SYN and this returns -1, you'll need to restart.
        {
            fatal_errno(__FILE__, __func__, __LINE__, errno);
            close_client(settings, EXIT_FAILURE);
        } else // If we are sending PSH, ACK, FIN, FIN/ACK and this returns -1, go ahead and try again.
        {
            return;
        }
    }
    
    printf("SENT PACKET\nFlags: %s\nLength: %zu\n\n", check_flags(send_packet->flags), packet_size);
    
    if (send_packet->flags == (FLAG_FIN | FLAG_ACK)) // NOLINT(hicpp-signed-bitwise) : highest order bit unused
    {
        printf("Waiting to receive FIN from server; feel free to terminate.\n\n");
    }
    
    if (send_packet->flags != FLAG_ACK)
    {
        await_response(settings, serialized_packet, packet_size);
    }
    
    settings->mem_manager->mm_free(settings->mem_manager, serialized_packet);
}

void await_response(struct client_settings *settings, uint8_t *serialized_packet, size_t packet_size)
{
    socklen_t sockaddr_in_size;
    uint8_t   recv_buffer[BUF_LEN];
    bool      received;
    int       num_timeouts;
    
    settings->timeout->tv_sec = BASE_TIMEOUT;
    if (setsockopt(settings->server_fd, SOL_SOCKET, SO_RCVTIMEO, (const char *) settings->timeout,
                   sizeof(struct timeval)) == -1)
    {
        fatal_errno(__FILE__, __func__, __LINE__, errno);
        close_client(settings, EXIT_FAILURE);
    }
    
    sockaddr_in_size = sizeof(struct sockaddr_in);
    received         = false;
    num_timeouts     = 0;
    do
    {
        memset(recv_buffer, 0, BUF_LEN);
        printf("----- AWAITING RESPONSE -----\n\n");
        
        if (recvfrom(settings->server_fd, recv_buffer, BUF_LEN, 0,
                     (struct sockaddr *) settings->addr, &sockaddr_in_size) == -1)
        {
            switch (errno)
            {
                case EINTR:
                {
                    return;
                }
                case EWOULDBLOCK:
                {
                    ++num_timeouts;
                    handle_recv_timeout(settings, serialized_packet, packet_size, num_timeouts);
                    printf("Timeout occurred, timeouts remaining: %d\n\n", (MAX_TIMEOUT - num_timeouts));
                    
                    if (num_timeouts >= MAX_TIMEOUT && *serialized_packet == (FLAG_FIN | FLAG_ACK)) // NOLINT(hicpp-signed-bitwise) : highest order bit unused
                    {
                        printf("Assuming server terminated, disconnecting.\n\n");
                    }
                    
                    break;
                }
                default:
                {
                    fatal_errno(__FILE__, __func__, __LINE__, errno);
                    close_client(settings, EXIT_FAILURE);
                }
            }
        } else
        {
            received = process_response(settings, recv_buffer, serialized_packet, packet_size);
        }
    } while (!received && num_timeouts < MAX_TIMEOUT);
    
    serialized_packet = NULL;
}

bool process_response(struct client_settings *settings, const uint8_t *recv_buffer, const uint8_t *send_buffer,
                      size_t packet_size)
{
    bool received = true;
    
    printf("Received response from server\n");
    printf("Received packet flags: %s\n\n", check_flags(*recv_buffer));
    
    // FIN/ACK received from server, FIN should be incoming. Go back to await the FIN.
    if (*recv_buffer == (FLAG_FIN | FLAG_ACK)) // NOLINT(hicpp-signed-bitwise) : highest order bit unused
    {
        received = false;
    }
    
    // Check the sequence number on the packet received. If it is invalid, retransmit the packet.
    if (*(recv_buffer + 1) != *(send_buffer + 1))
    {
        received = false;
        retransmit_packet(settings, send_buffer, packet_size);
    }
    
    return received;
}

void handle_recv_timeout(struct client_settings *settings,
                         uint8_t *serialized_packet,
                         size_t packet_size,
                         int num_timeouts)
{
    // Connection to server failure.
    if (num_timeouts >= MAX_TIMEOUT && *serialized_packet == FLAG_SYN)
    {
        printf("Too many unsuccessful attempts to connect to server, terminating\n");
        close_client(settings, EXIT_SUCCESS);
    }
    
    retransmit_packet(settings, serialized_packet, packet_size);
}

void retransmit_packet(struct client_settings *settings, const uint8_t *serialized_packet, size_t packet_size)
{
    socklen_t sockaddr_in_size;
    
    printf("Retransmitting packet\n\n");
    
    sockaddr_in_size = sizeof(struct sockaddr_in);
    
    if (sendto(settings->server_fd, serialized_packet, packet_size, 0, (struct sockaddr *) &settings->to_addr,
               sockaddr_in_size) == -1)
    {
        // Just tell em and go back to timeout.
        perror("Message retransmission to server failed: ");
    }
    
    printf("SENT PACKET\nLength: %zu\n\n", packet_size);
}

void create_packet(struct packet *packet, uint8_t flag, uint8_t seq_num, uint16_t len, uint8_t *payload)
{
    memset(packet, 0, sizeof(struct packet));
    
    packet->flags   = flag;
    packet->seq_num = seq_num;
    packet->length  = len;
    packet->payload = payload;
}

_Noreturn void close_client(struct client_settings *settings, int exit_code)
{
    if (settings->server_fd != 0)
    {
        close(settings->server_fd);
    }
    free_mem_manager(settings->mem_manager);
    exit(exit_code); // NOLINT(concurrency-mt-unsafe) : no threads here
}

static void set_signal_handling(struct sigaction *sa)
{
    sigemptyset(&sa->sa_mask);
    sa->sa_flags   = 0;
    sa->sa_handler = signal_handler;
    if ((sigaction(SIGINT, sa, NULL)) == -1)
    {
        fatal_errno(__FILE__, __func__, __LINE__, errno);
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static void signal_handler(int sig)
{
    running = 0;
}

#pragma GCC diagnostic pop
