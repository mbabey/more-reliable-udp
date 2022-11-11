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
//#include <sys/time.h>
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
 * open_client_socket
 * <p>
 * Set up the socket for the client. Set up the sockaddr struct for the server.
 * </p>
 * @param set - the settings for the client
 */
void open_client_socket(struct client_settings *set);

/**
 * do_connect
 * <p>
 * Send a SYN packet to the server. Await a SYN/ACK packet. Synchronize the communication port number with the server.
 * Send an ACK packet to the server on that port.
 * </p>
 * @param set - the settings for the client
 */
void do_connect(struct client_settings *set);

/**
 * do_message
 * <p>
 * Main messaging loop. Read a message from the client. Send the message to the server. Await a response from the server.
 * If a SIGINT occurs, the loop will exit.
 * </p>
 * @param set - the settings for the client
 */
void do_messaging(struct client_settings *set);

/**
 * get_input
 * <p>
 * Call the controller pointed to by the client settings struct to turn the light on and take user input.
 * </p>
 * @param set - the client settings
 * @return - an integer representing the player's choice of grid location upon which to play
 */
uint8_t get_input(struct client_settings *set);

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
 * @param flags - the flags
 * @param seq_num - the sequence number
 * @param len - the length
 * @param payload - the payload
 */
void create_packet(struct packet *packet, uint8_t flags, uint8_t seq_num, uint16_t len, uint8_t *payload);

/**
 * send_msg
 * <p>
 * Serialize the packet to send. Send the serialized packet to the server.
 * </p>
 * @param set - the settings for this client
 * @param s_packet - the packet to send
 */
void send_msg(struct client_settings *set);

/**
 * await_msg
 * <p>
 * Await a response from the server. If a response is not received within the timeout, retransmit the packet,
 * then wait again. If MAX_TIMEOUT timeouts occur, set running to 0 and return. If the message received from the
 * server does not match the expected flags and sequence number, retransmit the last sent packet.
 * </p>
 * @param set - the client settings
 * @param s_packet - the packet to retransmit, if necessary
 * @param flag_set - the expected flags to be received
 */
void await_msg(struct client_settings *set, uint8_t *flag_set, size_t num_flags, uint8_t seq_num);

/**
 * hande_recv_timeout
 * <p>
 * Check if the number of timeouts has exceeded MAX_TIMEOUTS. If that is the case, return -1. If the sent packet
 * was a FIN/ACK or a SYN, set running to 0 and print a relevant message. Otherwise, retransmit the sent packet
 * and return 0.
 * </p>
 * @param set - the client settings
 * @param s_packet - the packet to send
 * @param num_timeouts - the number of timeouts that have occurred
 * @return -1 if the timeout count has been exceeded, 0 if it has not
 */
int
handle_timeout(struct client_settings *set, int num_timeouts);

/**
 * process_response
 * <p>
 * Handle logic when receiving a PSH packet or a PSH/TRN packet.
 * </p>
 * @param set - the settings for the client
 * @param recv_buffer - the received packet
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
void retransmit_packet(struct client_settings *set);

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
    
    open_client_socket(settings);
    if (!errno)
    { do_messaging(settings); }
}

void open_client_socket(struct client_settings *set)
{
    if ((set->server_addr = (struct sockaddr_in *) s_calloc(1, sizeof(struct sockaddr_in),
                                                            __FILE__, __func__, __LINE__)) == NULL)
    {
        return;
    }
    set->mm->mm_add(set->mm, set->server_addr);
    
    
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
    
    do_connect(set);
}

void do_connect(struct client_settings *set)
{
    memset(set->s_packet, 0, sizeof(struct packet));
    
    printf("\nConnecting to server %s:%u\n", set->server_ip, set->server_port);
    
    create_packet(set->s_packet, FLAG_SYN, MAX_SEQ, 0, NULL);
    send_msg(set);
    if (!errno)
    {
        uint8_t flag_set[] = {FLAG_SYN | FLAG_ACK};
        await_msg(set, flag_set, sizeof(flag_set), set->s_packet->seq_num);
    }
    
    create_packet(set->s_packet, FLAG_ACK, MAX_SEQ, 0, NULL);
    if (!errno)
    { send_msg(set); }
}

void do_messaging(struct client_settings *set)
{
    struct sigaction sa;
    uint8_t          input;
    
    set_signal_handling(&sa);
    
    if (errno)
    { return; /* set_signal_handling may have failed. */ }
    
    running = 1;
    while (running)
    {
        set->turn = false; /* Clean the turn indicator. */
    
        /* Update game board, set turn. */
        { /* Scoped to allow variable name consistency. */
            uint8_t flag_set[] = {FLAG_PSH, FLAG_PSH | FLAG_TRN};
            await_msg(set, flag_set, sizeof(flag_set), (uint8_t) (set->s_packet->seq_num + 1));
        }
        
        create_packet(set->s_packet, FLAG_ACK, set->r_packet->seq_num, 0, NULL);
        send_msg(set);
        
        if (set->turn)
        {
            input = get_input(set); // TODO(m / p): get_input will turn the light on. After input taken, turn light off
            
            /* Send input to server. */
            create_packet(set->s_packet, FLAG_PSH, (uint8_t) (set->r_packet->seq_num + 1),
                          sizeof(input), &input);
            send_msg(set);
            
            if (!errno)
            {
                uint8_t flag_set[] = {FLAG_ACK};
                await_msg(set, flag_set, sizeof(flag_set), set->s_packet->seq_num);
            }
        }
    }
    
    if (!errno)
    { do_fin_seq(set); }
}

uint8_t get_input(struct client_settings *set)
{
    // TODO(m / p): this will call the controller, which will return the input. This function should probably belong to the controller.
    return 0;
}

void do_fin_seq(struct client_settings *set)
{
    create_packet(set->s_packet, FLAG_FIN, MAX_SEQ, 0, NULL);
    send_msg(set);
    if (!errno)
    {
        uint8_t flag_set[] = {FLAG_FIN | FLAG_ACK};
        await_msg(set, flag_set, sizeof(flag_set), set->s_packet->seq_num);
    }
    if (!errno)
    {
        uint8_t flag_set[] = {FLAG_FIN};
        await_msg(set, flag_set, sizeof(flag_set), set->s_packet->seq_num);
    }
    
    create_packet(set->s_packet, FLAG_FIN | FLAG_ACK, MAX_SEQ, 0, NULL);
    if (!errno)
    { send_msg(set); }
    if (!errno)
    {
        uint8_t flag_set[] = {FLAG_FIN};
        await_msg(set, flag_set, sizeof(flag_set), set->s_packet->seq_num);
    }
}

void send_msg(struct client_settings *set)
{
    socklen_t sockaddr_in_size;
    uint8_t   *serialized_packet;
    size_t    packet_size;
    
    sockaddr_in_size  = sizeof(struct sockaddr_in);
    serialized_packet = serialize_packet(set->s_packet); /* Serialize the packet to send. */
    if (errno == ENOTRECOVERABLE)
    {
        running = 0;
        return;
    }
    set->mm->mm_add(set->mm, serialized_packet);
    packet_size = sizeof(set->s_packet->flags) + sizeof(set->s_packet->seq_num)
                  + sizeof(set->s_packet->length) + set->s_packet->length;
    
    if (sendto(set->server_fd, serialized_packet, packet_size, 0,
               (struct sockaddr *) set->server_addr, sockaddr_in_size) == -1)
    {
        /* errno will be set. */
        perror("Message transmission to server failed: ");
        return;
    }
    
    printf("Sent packet:\n\tFlags: %s\n\tSequence number: %d\n\n", check_flags(set->s_packet->flags),
           set->s_packet->seq_num);
    
    set->mm->mm_free(set->mm, serialized_packet);
}

void await_msg(struct client_settings *set, uint8_t *flag_set, size_t num_flags, uint8_t seq_num)
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
        for (size_t i = 0; i < num_flags; ++i)
        {
            printf("\nAwaiting response with flags: %s", check_flags(flag_set[i]));
        }
        printf("\n");
        
        /* Update socket's timeout. */
//        if (setsockopt(set->server_fd,
//                       SOL_SOCKET, SO_RCVTIMEO, (const char *) set->timeout, sizeof(struct timeval)) == -1)
//        {
//            fatal_errno(__FILE__, __func__, __LINE__, errno);
//            running = 0;
//            return;
//        }
        
        memset(recv_buffer, 0, BUF_LEN); /* Clear our reception buffer. */
        
        /* set->server_addr will be overwritten when a message is received on the socket set->server_fd */
        if (recvfrom(set->server_fd, recv_buffer, BUF_LEN, 0,
                     (struct sockaddr *) set->server_addr, &sockaddr_in_size) == -1)
        {
            switch (errno)
            {
                case EINTR: /* If the user presses ctrl+C */
                {
                    return;
                }
                case EWOULDBLOCK: /* If the socket times out */
                {
                    if (handle_timeout(set, ++num_timeouts) == -1)
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
            
            for (size_t i = 0; !correct_packet && i < num_flags; ++i) /* For each flag in the set, */
            {
                /* Check against the seq num and flags in the recv_buffer. If either is invalid, we have the wrong packet. */
                correct_packet = *recv_buffer == flag_set[i] && *(recv_buffer + 1) == seq_num;
            }
            
            if (!correct_packet)
            {
                retransmit_packet(set);
            }
        }
    } while (!correct_packet); /* While the packet we received is not correct */
    
    process_response(set, recv_buffer); /* Once we have the correct packet, we will process it */
}

int handle_timeout(struct client_settings *set, int num_timeouts) // TODO(maxwell): implement changing timeout
{
    printf("\nTimeout occurred, timeouts remaining: %d\n", (MAX_TIMEOUT - num_timeouts));
    
    // Connection to server failure.
    if (num_timeouts >= MAX_TIMEOUT && set->s_packet->flags == FLAG_SYN)
    {
        printf("\nToo many unsuccessful attempts to connect to server, terminating\n");
        running = 0;
        return -1;
    }
    
    // Timeout after sending FIN/ACK.
    if (num_timeouts >= MAX_TIMEOUT && set->s_packet->flags == (FLAG_FIN | FLAG_ACK))
    {
        printf("\nAssuming server terminated, disconnecting.\n");
        running = 0;
        return -1;
    }
    
    // Timeout count exceeded in any other circumstance.
    if (num_timeouts >= MAX_TIMEOUT)
    {
        return -1;
    }
    
    // Timeout count not exceeded, retransmit.
    retransmit_packet(set);
    return 0;
}

void retransmit_packet(struct client_settings *set)
{
    printf("\nRetransmitting packet\n");
    
    socklen_t sockaddr_in_size;
    uint8_t   *serialized_packet;
    size_t    packet_size;
    
    serialized_packet = serialize_packet(set->s_packet);
    if (errno == ENOMEM)
    {
        running = 0;
        return;
    }
    set->mm->mm_add(set->mm, serialized_packet);
    
    sockaddr_in_size = sizeof(struct sockaddr_in);
    packet_size      = sizeof(set->s_packet->flags) + sizeof(set->s_packet->seq_num)
                       + sizeof(set->s_packet->length) + set->s_packet->length;
    
    if (sendto(set->server_fd, serialized_packet, packet_size, 0, (struct sockaddr *) set->server_addr,
               sockaddr_in_size) == -1)
    {
        // Just tell em and go back to timeout.
        perror("\nMessage retransmission to server failed: \n");
        return;
    }
    
    printf("\nSent packet:\n\tFlags: %s\n\tSequence number: %d\n", check_flags(*serialized_packet),
           set->s_packet->seq_num);
    
    set->mm->mm_free(set->mm, serialized_packet);
}

void process_response(struct client_settings *set, const uint8_t *recv_buffer)
{
    printf("\nReceived response:\n\tFlags: %s\n\tSequence number: %d\n", check_flags(*recv_buffer), *(recv_buffer + 1));
    
    deserialize_packet(set->r_packet, recv_buffer);
    if (errno == ENOMEM)
    {
        running = 0;
        return;
    }
    set->mm->mm_add(set->mm, set->r_packet->payload);
    
    if (set->r_packet->flags & FLAG_TRN) /* Indicates that it is this client's turn. */
    {
        set->turn = true;
        printf("\nYour turn!\n");
    }
    
    if (set->r_packet->flags & FLAG_PSH) /* Indicates that the packet contains data which must be displayed. */
    {
        printf("\nUI updated.\n"); // TODO(maxwell): show board here
    }
    
    set->mm->mm_free(set->mm, set->r_packet->payload);
}

void create_packet(struct packet *packet, uint8_t flags, uint8_t seq_num, uint16_t len, uint8_t *payload)
{
    memset(packet, 0, sizeof(struct packet));
    
    packet->flags   = flags;
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
    free_memory_manager(set->mm);
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
