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
 * @param send_packet - the packet to construct.
 * @param flags - the flags
 * @param seq_num - the sequence number
 * @param len - the length
 * @param payload - the payload
 */
void create_packet(struct packet *send_packet, uint8_t flags, uint8_t seq_num, uint16_t len, uint8_t *payload);

/**
 * send_msg
 * <p>
 * Serialize the packet to send. Send the serialized packet to the server.
 * </p>
 * @param s_packet - the packet to send
 * @param set - the settings for this client
 */
void send_msg(struct client_settings *set, struct packet *s_packet);

/**
 * await_msg
 * <p>
 * Await a response from the server. If a response is not received within the timeout, retransmit the packet,
 * then wait again. If MAX_TIMEOUT timeouts occur, set running to 0 and return.
 * </p>
 * @param set - the client settings
 * @param serialized_packet - the serialized send packet
 * @param packet_size - the size of the packet
 */
void await_msg(struct client_settings *set, struct packet *s_packet, uint8_t recv_flags);

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
 * @param s_packet - the packet to send
 * @param num_timeouts - the number of timeouts that have occurred
 */
void
handle_recv_timeout(struct client_settings *set, struct packet *s_packet, int num_timeouts);

/**
 * process_response
 * <p>
 * Check the received packet sequence number against the last sent packet sequence number; if they are note the same,
 * retransmit the package and return false. Check if the flags are FIN/ACK; if the flags are FIN/ACK, return false.
 * Otherwise, return true.
 * </p>
 * @param set - the settings for the client
 * @param recv_buffer - the received packet
 * @return false if bad sequence number or FIN/ACK, true otherwise
 */
void process_response(struct client_settings *set, const uint8_t *recv_buffer);

/**
 * retransmit_package
 * <p>
 * Resend a packet and print a relevant message.
 * </p>
 * @param set - the client settings
 * @param s_packet - the packet to send
 */
void retransmit_packet(struct client_settings *set, struct packet *s_packet);

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
    if (!errno) do_messaging(settings);
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

//    if (bind(set->server_fd, (struct sockaddr *) set->client_addr, sizeof(struct sockaddr_in)) == -1)
//    {
//        return;
//    }
    
    do_synchronize(set);
}

void do_synchronize(struct client_settings *set)
{
    struct packet s_packet;
    
    memset(&s_packet, 0, sizeof(struct packet));
    
    printf("\nConnecting to server %s:%u\n\n", set->server_ip, set->server_port);
    
    create_packet(&s_packet, FLAG_SYN, MAX_SEQ, 0, NULL);
    send_msg(set, &s_packet);
    if (!errno) await_msg(set, &s_packet, FLAG_SYN | FLAG_ACK);
    
    create_packet(&s_packet, FLAG_ACK, MAX_SEQ, 0, NULL);
    if (!errno) send_msg(set, &s_packet);
}

void do_messaging(struct client_settings *set)
{
    struct packet s_packet;
    
    uint8_t seq = (uint8_t) 0;
    
    char *msg = NULL;
    
    struct sigaction sa;
    set_signal_handling(&sa);
    
    if (errno) return; /* set_signal_handling may have failed. */
    
    running = 1;
    
    while (running)
    {
        msg = read_msg(set, msg); // TODO(maxwell): this will be 'take input'
        
        if (msg != NULL) // TODO(maxwell): this will be 'is_turn'
        {
            /* Create a packet and increment the sequence number. */
            create_packet(&s_packet, FLAG_PSH, seq++, strlen(msg), (uint8_t *) msg);
            
            printf("Sending message: %s\n\n", msg);
            send_msg(set, &s_packet);
            if (!errno) await_msg(set, &s_packet, FLAG_ACK);
            
            set->mem_manager->mm_free(set->mem_manager, msg);
        }
        
        msg = NULL; // TODO(maxwell): input = NULL
    }
    
    if (!errno) do_fin_seq(set);
}

void do_fin_seq(struct client_settings *set)
{
    struct packet s_packet;
    
    create_packet(&s_packet, FLAG_FIN, MAX_SEQ, 0, NULL);
    send_msg(set, &s_packet);
    if (!errno) await_msg(set, &s_packet, FLAG_FIN | FLAG_ACK);
    if (!errno) await_msg(set, &s_packet, FLAG_FIN);
    
    create_packet(&s_packet, FLAG_FIN | FLAG_ACK, MAX_SEQ, 0, NULL);
    if (!errno) send_msg(set, &s_packet);
    if (!errno) await_msg(set, &s_packet, FLAG_FIN);
}

char *read_msg(struct client_settings *set, char *msg) // TODO(maxwell): deprecated, here for testing purposes
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
    if (errno == ENOMEM) /* If a catastrophic error occurred in set_string. */
    {
        running = 0;
        return NULL;
    }
    set->mem_manager->mm_add(set->mem_manager, msg);
    
    return msg;
}

void send_msg(struct client_settings *set, struct packet *s_packet)
{
    socklen_t sockaddr_in_size;
    uint8_t   *serialized_packet;
    size_t    packet_size;
    
    sockaddr_in_size  = sizeof(struct sockaddr_in);
    serialized_packet = serialize_packet(s_packet);
    if (errno == ENOTRECOVERABLE)
    {
        running = 0;
        return;
    }
    set->mem_manager->mm_add(set->mem_manager, serialized_packet);
    
    packet_size = sizeof(s_packet->flags) + sizeof(s_packet->seq_num) + sizeof(s_packet->length) +
                  s_packet->length;
    
    if (sendto(set->server_fd, serialized_packet, packet_size, 0,
               (struct sockaddr *) set->server_addr, sockaddr_in_size) == -1)
    {
        /* errno will be set. */
        perror("Message transmission to server failed: ");
        return;
    }
    
    printf("Sent packet:\n\tFlags: %s\n\tLength: %zu\n\n", check_flags(s_packet->flags), packet_size);
    
    set->mem_manager->mm_free(set->mem_manager, serialized_packet);
}

void await_msg(struct client_settings *set, struct packet *s_packet, uint8_t recv_flags)
{
    socklen_t sockaddr_in_size;
    uint8_t   recv_buffer[BUF_LEN];
    bool      correct_packet;
    int       num_timeouts;
    
    set->timeout->tv_sec = BASE_TIMEOUT;
    
    sockaddr_in_size = sizeof(struct sockaddr_in);
    correct_packet   = false; /* We will assume we have the incorrect packet until we don't */
    num_timeouts     = 0;
    do
    {
        printf("Awaiting response with flags: %s\n\n", check_flags(recv_flags));
        
        /* Update our socket's timeout. */
        if (setsockopt(set->server_fd,
                       SOL_SOCKET, SO_RCVTIMEO, (const char *) set->timeout, sizeof(struct timeval)) == -1)
        {
            fatal_errno(__FILE__, __func__, __LINE__, errno);
            running = 0;
            return;
        }
        
        memset(recv_buffer, 0, BUF_LEN); /* Clear our reception buffer. */
        
        if (recvfrom(set->server_fd, recv_buffer, BUF_LEN, 0,
                     (struct sockaddr *) set->client_addr, &sockaddr_in_size) == -1)
        {
            switch (errno)
            {
                case EINTR: /* If the user presses ctrl+C */
                {
                    return;
                }
                case EWOULDBLOCK: /* If the socket times out */
                {
                    ++num_timeouts;
                    handle_recv_timeout(set, s_packet, num_timeouts);
                    if (num_timeouts >= MAX_TIMEOUT)
                    {
                        return;
                    }
                    break;
                }
                default: /* Any other error is not predicted */
                {
                    fatal_errno(__FILE__, __func__, __LINE__, errno);
                    running = 0;
                    return;
                }
            }
        } else
        {
            num_timeouts = 0; /* Reset num_timeouts: a packet was received. */
            
            /* Check the seq num and flags in the recv_buffer. If either is invalid, we have the wrong packet. */
            correct_packet = *(recv_buffer + 1) == s_packet->flags && *recv_buffer == recv_flags;
            
            if (!correct_packet)
            {
                retransmit_packet(set, s_packet); /* Can error, but it's okay. */
            }
        }
    } while (!correct_packet); /* While the packet we received is not correct */
    
    process_response(set, recv_buffer); /* Once we have the correct packet, we will process it */
    
}

void handle_recv_timeout(struct client_settings *set, struct packet *s_packet, int num_timeouts) // TODO(maxwell): implement changing timeout
{
    printf("Timeout occurred, timeouts remaining: %d\n\n", (MAX_TIMEOUT - num_timeouts));
    
    // Connection to server failure.
    if (num_timeouts >= MAX_TIMEOUT && s_packet->flags == FLAG_SYN)
    {
        printf("Too many unsuccessful attempts to connect to server, terminating\n");
        running = 0;
        return;
    }
    
    // Waiting to see if final FIN/ACK sent successfully.
    if (num_timeouts >= MAX_TIMEOUT && s_packet->flags == (FLAG_FIN | FLAG_ACK))
    {
        printf("Assuming server terminated, disconnecting.\n\n");
        running = 0;
        return;
    }
    
    retransmit_packet(set, s_packet);
}

void retransmit_packet(struct client_settings *set, struct packet *s_packet)
{
    printf("Retransmitting packet\n\n");
    
    socklen_t sockaddr_in_size;
    uint8_t   *serialized_packet;
    size_t    packet_size;
    
    serialized_packet = serialize_packet(s_packet);
    if (errno == ENOMEM)
    {
        running = 0;
        return;
    }
    set->mem_manager->mm_add(set->mem_manager, serialized_packet);
    
    sockaddr_in_size = sizeof(struct sockaddr_in);
    packet_size      = sizeof(s_packet->flags) + sizeof(s_packet->seq_num)
                       + sizeof(s_packet->length) + s_packet->length;
    
    if (sendto(set->server_fd, serialized_packet, packet_size, 0, (struct sockaddr *) set->server_addr,
               sockaddr_in_size) == -1)
    {
        // Just tell em and go back to timeout.
        perror("Message retransmission to server failed: ");
        return;
    }
    
    printf("Sent packet:\n\tFlags: %s\n\tLength: %zu\n\n", check_flags(*serialized_packet), packet_size);
    
    set->mem_manager->mm_free(set->mem_manager, serialized_packet);
}

void
process_response(struct client_settings *set, const uint8_t *recv_buffer) // TODO(maxwell): Will eventually set errno
{
    printf("Received response:\n\tFlags: %s\n", check_flags(*recv_buffer));
    
    if (*recv_buffer == FLAG_PSH)
    {
        // means game board update, so update UI
    }
    
    if (*recv_buffer == (FLAG_PSH | FLAG_TRN))
    {
        // means update turn status; true --> false, false --> true
    }
}

void create_packet(struct packet *send_packet, uint8_t flags, uint8_t seq_num, uint16_t len, uint8_t *payload)
{
    memset(send_packet, 0, sizeof(struct packet));
    
    send_packet->flags   = flags;
    send_packet->seq_num = seq_num;
    send_packet->length  = len;
    send_packet->payload = payload;
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
