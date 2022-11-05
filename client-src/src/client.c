#include "../../libs/include/error.h"
#include "../../libs/include/util.h"
#include "../include/client.h"
#include "../include/setup.h"
#include <arpa/inet.h>
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
 * The base timeout duration before retransmission.
 */
#define BASE_TIMEOUT 1

/**
 * While set to > 0, the program will continue running. Will be set to 0 by SIGINT or a catastrophic failure.
 */
static volatile sig_atomic_t running; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables) : var must change

/**
 * open_client
 * <p>
 * Set up the socket for the client. Set up the sockaddr structs for the client and the server. Bind the socket
 * to the client sockaddr.
 * </p>
 * @param set - the set for the client
 */
void open_client(struct client_settings *set);

/**
 * do_synchronize
 * <p>
 * Send a SYN packet to the server. Await a SYN/ACK packet. Send an ACK packet to the server.
 * </p>
 * @param set - the settings for the client
 */
void do_synchronize(struct client_settings *set);

/**
 * do_messaging
 * <p>
 * Main messaging loop. Read a message from the client. Send the message to the server. Await a response from the server.
 * If a SIGINT occurs, the loop will exit.
 * </p>
 * @param set - the settings for the client
 */
void do_messaging(struct client_settings *set);

/**
 * do_fin_seq
 * <p>
 * Send a FIN packet. Wait for a FIN/ACK packet and a FIN packet. Send a FIN/ACK packet. Wait to see if the
 * FIN/ACK was received.
 * </p>
 * @param set - the settings for the client
 */
void do_fin_seq(struct client_settings *set);

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
 * @param set - the settings for this client
 */
void send_msg(struct client_settings *set, struct packet *send_packet);

/**
 * await_response
 * <p>
 * Await a response from the server. If a response is not received within the timeout, retransmit the packet,
 * then wait again. If MAX_TIMEOUT timeouts occur, set running to 0 and return.
 * </p>
 * @param set - the client settings
 * @param serialized_packet - the serialized send packet
 * @param packet_size - the size of the packet
 */
void await_response(struct client_settings *set, struct packet *send_packet, uint8_t flags);

/**
 * read_msg
 * <p>
 * Read a message from the user. A message is either a string from STDIN_FILENO or the content of a file
 * with a file name specified on STDIN_FILENO
 * </p>
 * @param set - the client settings
 * @param msg - the message to be made a payload
 */
char *read_msg(struct client_settings *set, char *msg);

/**
 * hande_recv_timeout
 * <p>
 * Check if the number of timeouts has exceeded MAX_TIMEOUTS and if the packet is a SYN packet. If either of those
 * conditions are true, set running to 0 and return.
 * </p>
 * @param set - the client settings
 * @param serialized_packet - the serialized send packet
 * @param packet_size - the size of the packet
 * @param num_timeouts - the number of timeouts that have occurred
 */
void
handle_recv_timeout(struct client_settings *set, uint8_t *serialized_packet, size_t packet_size, int num_timeouts);

/**
 * process_response
 * <p>
 * Check the received packet sequence number against the last sent packet sequence number; if they are note the same,
 * retransmit the package and return false. Check if the flags are FIN/ACK; if the flags are FIN/ACK, return false.
 * Otherwise, return true.
 * </p>
 * @param set - the settings for the client
 * @param recv_buffer - the received packet
 * @param send_buffer - the last sent packet
 * @param packet_size - the size of the send packet
 * @return false if bad sequence number or FIN/ACK, true otherwise
 */
bool process_response(struct client_settings *set, const uint8_t *recv_buffer, const uint8_t *send_buffer,
                      size_t packet_size);

/**
 * retransmit_package
 * <p>
 * Resend a packet and print a relevant message.
 * </p>
 * @param set - the client settings
 * @param serialized_packet - the packet to send
 * @param packet_size - the size of the packet to send
 */
void retransmit_packet(struct client_settings *set, const uint8_t *serialized_packet, size_t packet_size);

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

void run_client(int argc, char *argv[], struct client_settings *settings)
{
    init_def_state(argc, argv, settings);
    
    open_client(settings);
    if (!errno)
    {
        do_messaging(settings);
    }
}

void open_client(struct client_settings *set)
{
    if ((set->client_addr = (struct sockaddr_in *) s_calloc(1, sizeof(struct sockaddr_in),
                                                            __FILE__, __func__, __LINE__)) == NULL)
    {
        return;
    }
    if ((set->server_addr = (struct sockaddr_in *) s_calloc(1, sizeof(struct sockaddr_in),
                                                            __FILE__, __func__, __LINE__)) == NULL)
    {
        return;
    }
    set->mem_manager->mm_add(set->mem_manager, set->client_addr);
    set->mem_manager->mm_add(set->mem_manager, set->server_addr);
    
    set->client_addr->sin_family           = AF_INET;
    set->client_addr->sin_port             = 0; // Ephemeral port.
    if ((set->client_addr->sin_addr.s_addr = inet_addr(set->client_ip)) == (in_addr_t) -1)
    {
        return;
    }
    
    set->server_addr->sin_family           = AF_INET;
    set->server_addr->sin_port             = htons(set->server_port);
    if ((set->server_addr->sin_addr.s_addr = inet_addr(set->server_ip)) == (in_addr_t) -1)
    {
        return;
    }
    
    if ((set->server_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) // NOLINT(android-cloexec-socket) : SOCK_CLOEXEC dne
    {
        return;
    }
    
    if (bind(set->server_fd, (struct sockaddr *) set->client_addr, sizeof(struct sockaddr_in)) == -1)
    {
        return;
    }
    
    do_synchronize(set);
}

void do_synchronize(struct client_settings *set)
{
    struct packet send_packet;
    
    memset(&send_packet, 0, sizeof(struct packet));
    
    printf("\nConnecting to server %s:%u\n\n", set->server_ip, set->server_port);
    
    create_packet(&send_packet, FLAG_SYN, MAX_SEQ, 0, NULL);
    send_msg(set, &send_packet);
    await_response(set, &send_packet, 0);
    if (!errno)
    {
        create_packet(&send_packet, FLAG_ACK, MAX_SEQ, 0, NULL);
        send_msg(set, &send_packet);
        await_response(set, &send_packet, 0);
    }
}

void do_messaging(struct client_settings *set)
{
    struct packet send_packet;
    
    uint8_t seq = (uint8_t) 0;
    
    char *msg = NULL;
    
    struct sigaction sa;
    set_signal_handling(&sa);
    if (errno)
    {
        return;
    }
    running = 1;
    
    while (running)
    {
        msg = read_msg(set, msg); // TODO: this will be 'take input'
        
        if (msg != NULL) // TODO: this will be 'is_turn'
        {
            /* Create a packet and increment the sequence number. */
            create_packet(&send_packet, FLAG_PSH, seq++, strlen(msg), (uint8_t *) msg);
            
            printf("Sending message: %s\n\n", msg);
            send_msg(set, &send_packet);
            await_response(set, &send_packet, 0);
            
            set->mem_manager->mm_free(set->mem_manager, msg);
        }
        
        msg = NULL; // TODO: input = NULL
    }
    
    if (!errno)
    {
        do_fin_seq(set);
    }
}

void do_fin_seq(struct client_settings *set)
{
    struct packet send_packet;
    
    create_packet(&send_packet, FLAG_FIN, MAX_SEQ, 0, NULL);
    send_msg(set, &send_packet);
    await_response(set, &send_packet, 0);
    if (!errno)
    {
        create_packet(&send_packet, FLAG_FIN | FLAG_ACK, MAX_SEQ, 0,
                      NULL); // NOLINT(hicpp-signed-bitwise) : highest order bit unused
        send_msg(set, &send_packet);
        await_response(set, &send_packet, 0);
    }
}

char *read_msg(struct client_settings *set, char *msg) // TODO: deprecated, here for testing purposes
{
    char input[BUF_LEN];
    
    printf("Please enter the string you wish to send: \n");
    
    errno = 0;
    if (fgets(input, BUF_LEN, stdin) == NULL)
    {
        if (errno == EINTR) /* If the user presses ctrl+C during input. */
        {
            errno = 0;
            return NULL;
        }
        if (feof(stdin)) /* If the user presses ctrl+D during input. */
        {
            errno = 0;
            running = 0;
            return NULL;
        }
    }
    
    input[strcspn(input, "\n")] = '\0';
    
    set_string(&msg, input);
    if (errno == ENOMEM) /* If a catastrophic error occurred in or set_string. */
    {
        running = 0;
        return NULL;
    }
    set->mem_manager->mm_add(set->mem_manager, msg);
    
    return msg;
}

void send_msg(struct client_settings *set, struct packet *send_packet)
{
    socklen_t sockaddr_in_size;
    uint8_t   *serialized_packet;
    size_t    packet_size;
    
    sockaddr_in_size  = sizeof(struct sockaddr_in);
    serialized_packet = serialize_packet(send_packet);
    if (errno == ENOTRECOVERABLE)
    {
        running = 0;
        return;
    }
    set->mem_manager->mm_add(set->mem_manager, serialized_packet);
    
    packet_size = sizeof(send_packet->flags) + sizeof(send_packet->seq_num) + sizeof(send_packet->length) +
                  send_packet->length;
    
    if (sendto(set->server_fd, serialized_packet, packet_size, 0,
               (struct sockaddr *) set->server_addr, sockaddr_in_size) == -1)
    {
        /* errno will be set. */
        perror("Message transmission to server failed: ");
        return;
    }
    
    printf("Sent packet:\n\tFlags: %s\n\tLength: %zu\n\n", check_flags(send_packet->flags), packet_size);
    
    if (send_packet->flags == (FLAG_FIN | FLAG_ACK)) // NOLINT(hicpp-signed-bitwise) : highest order bit unused
    {
        printf("Waiting to receive FIN from server; feel free to terminate.\n\n");
    }
    
    set->mem_manager->mm_free(set->mem_manager, serialized_packet);
}

void await_response(struct client_settings *set, struct packet *send_packet, uint8_t flags)
{
    socklen_t sockaddr_in_size;
    uint8_t   *serialized_packet;
    uint8_t   recv_buffer[BUF_LEN];
    size_t    packet_size;
    bool      received;
    int       num_timeouts;
    
    set->timeout->tv_sec = BASE_TIMEOUT;
    if (setsockopt(set->server_fd, SOL_SOCKET, SO_RCVTIMEO, (const char *) set->timeout,
                   sizeof(struct timeval)) == -1)
    {
        fatal_errno(__FILE__, __func__, __LINE__, errno);
        running = 0;
        return;
    }
    
    serialized_packet = serialize_packet(send_packet);
    if (errno == ENOTRECOVERABLE)
    {
        running = 0;
        return;
    }
    
    packet_size = sizeof(send_packet->flags) + sizeof(send_packet->seq_num) + sizeof(send_packet->length) +
                  send_packet->length;
    
    sockaddr_in_size = sizeof(struct sockaddr_in);
    received         = false;
    num_timeouts     = 0;
    do
    {
        memset(recv_buffer, 0, BUF_LEN);
        printf("Awaiting response with flags: %s\n\n", check_flags(flags));
        
        if (recvfrom(set->server_fd, recv_buffer, BUF_LEN, 0,
                     (struct sockaddr *) set->client_addr, &sockaddr_in_size) == -1)
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
                    printf("Timeout occurred, timeouts remaining: %d\n\n", (MAX_TIMEOUT - num_timeouts));
                    handle_recv_timeout(set, serialized_packet, packet_size, num_timeouts);
                    
                    if (num_timeouts >= MAX_TIMEOUT &&
                        *serialized_packet ==
                        (FLAG_FIN | FLAG_ACK)) // NOLINT(hicpp-signed-bitwise) : highest order unused
                    {
                        printf("Assuming server terminated, disconnecting.\n\n");
                    }
                    
                    break;
                }
                default:
                {
                    fatal_errno(__FILE__, __func__, __LINE__, errno);
                    running = 0;
                    return;
                }
            }
        } else
        {
            received = process_response(set, recv_buffer, serialized_packet, packet_size);
        }
    } while (!received && num_timeouts < MAX_TIMEOUT);
    
    serialized_packet = NULL;
}

bool process_response(struct client_settings *set,
                      const uint8_t *recv_buffer,
                      const uint8_t *send_buffer,
                      size_t packet_size)
{
    printf("Received response:\n\tFlags: %s\n", check_flags(*recv_buffer));
    
    // Check the sequence number on the packet received. If it is invalid, retransmit the packet.
    if (*(recv_buffer + 1) != *(send_buffer + 1))
    {
        retransmit_packet(set, send_buffer, packet_size);
        return false;
    }
    
    // FIN/ACK received from server, FIN should be incoming. Go back to await the FIN.
    if (*recv_buffer == (FLAG_FIN | FLAG_ACK)) // NOLINT(hicpp-signed-bitwise) : highest order bit unused
    {
        return false;
    }
    
    return true;
}

void handle_recv_timeout(struct client_settings *set,
                         uint8_t *serialized_packet,
                         size_t packet_size,
                         int num_timeouts) // TODO: implement changing timeout
{
    // Connection to server failure.
    if (num_timeouts >= MAX_TIMEOUT && *serialized_packet == FLAG_SYN)
    {
        printf("Too many unsuccessful attempts to connect to server, terminating\n");
        running = 0;
        return;
    }
    
    retransmit_packet(set, serialized_packet, packet_size);
}

void retransmit_packet(struct client_settings *set, const uint8_t *serialized_packet, size_t packet_size)
{
    socklen_t sockaddr_in_size;
    
    printf("Retransmitting packet\n\n");
    
    sockaddr_in_size = sizeof(struct sockaddr_in);
    
    if (sendto(set->server_fd, serialized_packet, packet_size, 0, (struct sockaddr *) set->server_addr,
               sockaddr_in_size) == -1)
    {
        // Just tell em and go back to timeout.
        perror("Message retransmission to server failed: ");
        return;
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

void close_client(struct client_settings *set)
{
    if (set->server_fd != 0)
    {
        close(set->server_fd);
    }
    free_mem_manager(set->mem_manager);
}

static void set_signal_handling(struct sigaction *sa)
{
    sigemptyset(&sa->sa_mask);
    sa->sa_flags   = 0;
    sa->sa_handler = signal_handler;
    errno = 0;
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
