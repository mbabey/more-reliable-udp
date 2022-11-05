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

/**
 * The maximum number of timeouts to occur before the connection is deemed lost.
 */
#define MAX_TIMEOUT 3

/**
 * deprecated, soon to be removed.
 */
#define OFFSET 0

/**
 * The base timeout duration before retransmission.
 */
#define BASE_TIMEOUT 10

/**
 * While set to > 0, the program will continue running. Will be set to 0 by SIGINT or a catastrophic failure.
 */
static volatile sig_atomic_t running; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables) : var must change

/**
 * set_signal_handling
 * <p>
 * Setup a handler for the SIGINT signal.
 * </p>
 * @author D'Arcy Smith
 * @param sa - the sigaction for setup
 */
static void set_signal_handling(struct sigaction *sa);

/**
 * signal_handler
 * <p>
 * Callback function for the signal handler. Will set running to 0 upon signal.
 * </p>
 * @param sig - the signal
 * @author D'Arcy Smith
 */
static void signal_handler(int sig);

/**
 * open_client
 * <p>
 * Set up the socket for the client. Set up the sockaddr structs for the client and the server. Bind the socket
 * to the client sockaddr.
 * </p>
 * @param settings - the settings for the client
 */
void open_client(struct client_settings *settings);

/**
 * handshake
 * <p>
 * Send a SYN packet to the server. Await a SYN/ACK packet. Send an ACK packet to the server.
 * </p>
 * @param settings - the settings for the client
 */
void handshake(struct client_settings *settings);

/**
 * do_messaging
 * <p>
 * Main messaging loop. Read a message from the client. Send the message to the server. Await a response from the server.
 * If a SIGINT occurs, the loop will exit.
 * </p>
 * @param settings - the settings for the client
 */
void do_messaging(struct client_settings *settings);

/**
 * create_packet
 * <p>
 * Create a packet with flag = flag, sequence number = seq_num, length = len, and payload = payload.
 * </p>
 * @param packet - the packet to construct.
 * @param flag - the flags
 * @param seq_num - the sequence number
 * @param len - the length
 * @param payload - the payload
 */
void create_packet(struct packet *packet, uint8_t flag, uint8_t seq_num, uint16_t len, uint8_t *payload);

/**
 * send_msg
 * <p>
 * Serialize the packet to send. Send the serialized packet to the server.
 * </p>
 * @param send_packet - the packet to send
 * @param settings - the settings for this client
 */
void send_msg(struct packet *send_packet, struct client_settings *settings);

/**
 * await_response
 * <p>
 * Await a response from the server. If a response is not received within the timeout, retransmit the packet,
 * then wait again. If MAX_TIMEOUT timeouts occur, set running to 0 and return.
 * </p>
 * @param settings - the client settings
 * @param serialized_packet - the serialized send packet
 * @param packet_size - the size of the packet
 */
void await_response(struct client_settings *settings, uint8_t *serialized_packet, size_t packet_size);

/**
 * read_msg
 * <p>
 * Read a message from the user. A message is either a string from STDIN_FILENO or the content of a file
 * with a file name specified on STDIN_FILENO
 * </p>
 * @param settings - the client settings
 * @param msg - the message to be made a payload
 */
void read_msg(struct client_settings *settings, char **msg);

/**
 * hande_recv_timeout
 * <p>
 * Check if the number of timeouts has exceeded MAX_TIMEOUTS and if the packet is a SYN packet. If either of those
 * conditions are true, set running to 0 and return.
 * </p>
 * @param settings - the client settings
 * @param serialized_packet - the serialized send packet
 * @param packet_size - the size of the packet
 * @param num_timeouts - the number of timeouts that have occurred
 */
void
handle_recv_timeout(struct client_settings *settings, uint8_t *serialized_packet, size_t packet_size, int num_timeouts);

/**
 * process_response
 * <p>
 * Check the received packet sequence number against the last sent packet sequence number; if they are note the same,
 * retransmit the package and return false. Check if the flags are FIN/ACK; if the flags are FIN/ACK, return false.
 * Otherwise, return true.
 * </p>
 * @param settings - the settings for the client
 * @param recv_buffer - the received packet
 * @param send_buffer - the last sent packet
 * @param packet_size - the size of the send packet
 * @return false if bad sequence number or FIN/ACK, true otherwise
 */
bool process_response(struct client_settings *settings, const uint8_t *recv_buffer, const uint8_t *send_buffer,
                      size_t packet_size);

/**
 * retransmit_package
 * <p>
 * Resend a packet and print a relevant message.
 * </p>
 * @param settings - the client settings
 * @param serialized_packet - the packet to send
 * @param packet_size - the size of the packet to send
 */
void retransmit_packet(struct client_settings *settings, const uint8_t *serialized_packet, size_t packet_size);

/**
 * read_file
 * <p>
 * Read the content of a file into the buffer msg.
 * </p>
 * @param msg - the buffer into which the file should be read.
 * @param input - the input file name
 */
void read_file(char **msg, const char *input);

/**
 * close_client
 * <p>
 * Close the client socket if open, and free all memory.
 * </p>
 * @param settings - the client settings
 * @param exit_code
 */
_Noreturn void close_client(struct client_settings *settings, int exit_code);

void run_client(int argc, char *argv[], struct client_settings *settings)
{
    init_def_state(argc, argv, settings);
    
    open_client(settings);
    handshake(settings);
    do_messaging(settings);
    
    close_client(settings, EXIT_SUCCESS); // TODO: return to main
}

void open_client(struct client_settings *settings)
{
    settings->client_addr = (struct sockaddr_in *) s_calloc(1, sizeof(struct sockaddr_in), __FILE__, __func__,
                                                            __LINE__);
    if (errno == ENOTRECOVERABLE)
    {
        free_mem_manager(settings->mem_manager);
        exit(EXIT_SUCCESS); // NOLINT(concurrency-mt-unsafe) : no threads here // TODO: return to main
    }
    settings->mem_manager->mm_add(settings->mem_manager, settings->client_addr);
    
    settings->client_addr->sin_family           = AF_INET;
    settings->client_addr->sin_port             = 0; // Ephemeral port.
    if ((settings->client_addr->sin_addr.s_addr = inet_addr(settings->client_ip)) == (in_addr_t) -1)
    {
        fatal_errno(__FILE__, __func__, __LINE__, errno);
        close_client(settings, EXIT_FAILURE); // TODO: return to main
    }
    
    settings->server_addr.sin_family           = AF_INET;
    settings->server_addr.sin_port             = htons(settings->server_port);
    if ((settings->server_addr.sin_addr.s_addr = inet_addr(settings->server_ip)) == (in_addr_t) -1)
    {
        fatal_errno(__FILE__, __func__, __LINE__, errno);
        close_client(settings, EXIT_FAILURE); // TODO: return to main
    }
    
    if ((settings->server_fd = socket(AF_INET, SOCK_DGRAM, 0)) ==
        -1) // NOLINT(android-cloexec-socket) : SOCK_CLOEXEC dne
    {
        fatal_errno(__FILE__, __func__, __LINE__, errno);
        close_client(settings, EXIT_FAILURE); // TODO: return to main
    }
    
    if (bind(settings->server_fd, (struct sockaddr *) settings->client_addr, sizeof(struct sockaddr_in)) == -1)
    {
        fatal_errno(__FILE__, __func__, __LINE__, errno);
        close_client(settings, EXIT_FAILURE); // TODO: return to main
    }
    
    printf("\nConnecting to server %s:%d\n\n", settings->server_ip, settings->server_port);
}

void handshake(struct client_settings *settings)
{
    struct packet send_packet;
    
    memset(&send_packet, 0, sizeof(struct packet));
    
    create_packet(&send_packet, FLAG_SYN, MAX_SEQ, 0, NULL);
    send_msg(&send_packet, settings);
    create_packet(&send_packet, FLAG_ACK, MAX_SEQ, 0, NULL);
    send_msg(&send_packet, settings);
}

void do_messaging(struct client_settings *settings)
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
    create_packet(&send_packet, FLAG_FIN | FLAG_ACK, MAX_SEQ, 0,
                  NULL); // NOLINT(hicpp-signed-bitwise) : highest order bit unused
    send_msg(&send_packet, settings);
}

void read_msg(struct client_settings *settings, char **msg)
{
    char input[BUF_LEN];
    
    printf("%s", (settings->is_file) ?
                 "Please enter the name of the file you wish to send: \n" :
                 "Please enter the string you wish to send: \n");
    
    if (fgets(input, BUF_LEN, stdin) == NULL)
    {
        if (errno == EINTR || feof(stdin))
        {
            return;
        }
    }
    
    input[strcspn(input, "\n")] = '\0';
    
    if (settings->is_file)
    {
        printf("Opening file\n");
        read_file(msg, input); // TODO: fix file input
    } else
    {
        set_string(msg, input);
    }
    
    if (errno == ENOTRECOVERABLE)
    {
        close_client(settings, EXIT_FAILURE); // TODO: return to main
    }
    settings->mem_manager->mm_add(settings->mem_manager, *msg);
    printf("Sending message: %s\n\n", *msg);
}

void read_file(char **msg, const char *input)
{
    int     fd;
    ssize_t len;
    
    if ((fd = do_open(input, O_RDONLY)) == -1) // TODO: FIX THIS SHIT UGH
    {
        *msg = NULL;
        return;
    }
    
    if ((len = do_lseek(fd, OFFSET, SEEK_END) + 1) == 0) // TODO: get the file size in a better way
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
        close_client(settings, EXIT_FAILURE); // TODO: return to main
    }
    settings->mem_manager->mm_add(settings->mem_manager, serialized_packet);
    
    packet_size = sizeof(send_packet->flags) + sizeof(send_packet->seq_num) + sizeof(send_packet->length) +
                  send_packet->length;
    
    if (sendto(settings->server_fd, serialized_packet, packet_size, 0,
               (struct sockaddr *) &settings->server_addr, sockaddr_in_size) == -1)
    {
        if (send_packet->flags == FLAG_SYN) // If we are sending the SYN and this returns -1, you'll need to restart.
        {
            fatal_errno(__FILE__, __func__, __LINE__, errno); // TODO: this should just send SYN again
            close_client(settings, EXIT_FAILURE); // TODO: return to main
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
        await_response(settings, serialized_packet, packet_size); // TODO: decouple this
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
        close_client(settings, EXIT_FAILURE); // TODO: return to main
    }
    
    sockaddr_in_size = sizeof(struct sockaddr_in);
    received         = false;
    num_timeouts     = 0;
    do
    {
        memset(recv_buffer, 0, BUF_LEN);
        printf("----- AWAITING RESPONSE -----\n\n");
        
        if (recvfrom(settings->server_fd, recv_buffer, BUF_LEN, 0,
                     (struct sockaddr *) settings->client_addr, &sockaddr_in_size) == -1)
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
                    
                    if (num_timeouts >= MAX_TIMEOUT && *serialized_packet == (FLAG_FIN |
                                                                              FLAG_ACK)) // NOLINT(hicpp-signed-bitwise) : highest order bit unused
                    {
                        printf("Assuming server terminated, disconnecting.\n\n");
                    }
                    
                    break;
                }
                default:
                {
                    fatal_errno(__FILE__, __func__, __LINE__, errno);
                    close_client(settings, EXIT_FAILURE); // TODO: return to main
                }
            }
        } else
        {
            received = process_response(settings, recv_buffer, serialized_packet, packet_size);
        }
    } while (!received && num_timeouts < MAX_TIMEOUT);
    
    serialized_packet = NULL;
}

bool process_response(struct client_settings *settings,
                      const uint8_t *recv_buffer,
                      const uint8_t *send_buffer,
                      size_t packet_size)
{
    printf("Received response:\n\tFlags: %s\n", check_flags(*recv_buffer));
    
    // Check the sequence number on the packet received. If it is invalid, retransmit the packet.
    if (*(recv_buffer + 1) != *(send_buffer + 1))
    {
        retransmit_packet(settings, send_buffer, packet_size);
        return false;
    }
    
    // FIN/ACK received from server, FIN should be incoming. Go back to await the FIN.
    if (*recv_buffer == (FLAG_FIN | FLAG_ACK)) // NOLINT(hicpp-signed-bitwise) : highest order bit unused
    {
        return false;
    }
    
    return true;
}

void handle_recv_timeout(struct client_settings *settings,
                         uint8_t *serialized_packet,
                         size_t packet_size,
                         int num_timeouts) // TODO: implement changing timeout
{
    // Connection to server failure.
    if (num_timeouts >= MAX_TIMEOUT && *serialized_packet == FLAG_SYN)
    {
        printf("Too many unsuccessful attempts to connect to server, terminating\n");
        close_client(settings, EXIT_SUCCESS); // TODO: return to main
    }
    
    retransmit_packet(settings, serialized_packet, packet_size);
}

void retransmit_packet(struct client_settings *settings, const uint8_t *serialized_packet, size_t packet_size)
{
    socklen_t sockaddr_in_size;
    
    printf("Retransmitting packet\n\n");
    
    sockaddr_in_size = sizeof(struct sockaddr_in);
    
    if (sendto(settings->server_fd, serialized_packet, packet_size, 0, (struct sockaddr *) &settings->server_addr,
               sockaddr_in_size) == -1)
    {
        // Just tell em and go back to timeout.
        perror("Message retransmission to server failed: ");
    }
    
    printf("Sent packet:\n\tFlags: %s\n\tLength: %zu\n\n", check_flags(*serialized_packet), packet_size);
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
        fatal_errno(__FILE__, __func__, __LINE__, errno); // TODO: return to main
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static void signal_handler(int sig)
{
    running = 0;
}

#pragma GCC diagnostic pop
