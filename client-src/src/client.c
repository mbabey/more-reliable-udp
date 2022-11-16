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
 * The number of bytes needed to be sent to the server to update the server-side game state.
 */
#define GAME_DATA_BYTES 2

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
 * cl_connect
 * <p>
 * Send a SYN packet to the server. Await a SYN/ACK packet. Synchronize the communication port number with the server.
 * Send an ACK packet to the server on that port.
 * </p>
 * @param set - the settings for the client
 */
void cl_connect(struct client_settings *set);

/**
 * sv_recvfrom
 * <p>
 * Main messaging loop. Read a message from the client. Send the message to the server. Await a response from the server.
 * If a SIGINT occurs, the loop will exit.
 * </p>
 * @param set - the settings for the client
 */
void cl_messaging(struct client_settings *set);

/**
 * cl_disconnect
 * <p>
 * Send a FIN packet. Wait for a FIN/ACK packet and a FIN packet. Send a FIN/ACK packet. Wait to see if the
 * FIN/ACK was received.
 * </p>
 * @param set - the settings for the client
 */
void cl_disconnect(struct client_settings *set);

/**
 * cl_sendto
 * <p>
 * Serialize the packet to send. Send the serialized packet to the server.
 * </p>
 * @param set - the settings for this client
 * @param s_packet - the packet to send
 */
void cl_sendto(struct client_settings *set);

/**
 * cl_recvfrom
 * <p>
 * Await a response from the server. If a response is not received within the timeout, retransmit the packet,
 * then wait again. If MAX_TIMEOUT timeouts occur, set running to 0 and return. If the message received from the
 * server does not match the expected flags and sequence number, retransmit the last sent packet.
 * </p>
 * @param set - the client settings
 * @param s_packet - the packet to retransmit, if necessary
 * @param flag_set - the expected flags to be received
 */
void cl_recvfrom(struct client_settings *set, uint8_t *flag_set, uint8_t num_flags, uint8_t seq_num);

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
int handle_timeout(struct client_settings *set, int num_timeouts);

/**
 * cl_process
 * <p>
 * Handle logic when receiving a PSH packet or a PSH/TRN packet.
 * </p>
 * @param set - the settings for the client
 * @param packet_buffer - the received packet
 */
void cl_process(struct client_settings *set, const uint8_t *packet_buffer);

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

void run_client(int argc, char *argv[], struct client_settings *set)
{
    init_def_state(argc, argv, set);
    
    open_client_socket(set);
    if (!errno)
    { cl_messaging(set); }
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
    
    cl_connect(set);
}

void cl_connect(struct client_settings *set)
{
    memset(set->s_packet, 0, sizeof(struct packet));
    
    printf("\nConnecting to server %s:%u\n", set->server_ip, set->server_port);
    
    create_packet(set->s_packet, FLAG_SYN, MAX_SEQ, 0, NULL);
    cl_sendto(set);
    if (!errno)
    {
        uint8_t flag_set[] = {FLAG_SYN | FLAG_ACK};
        cl_recvfrom(set, flag_set, sizeof(flag_set), set->s_packet->seq_num);
    }
    
    create_packet(set->s_packet, FLAG_ACK, MAX_SEQ, 0, NULL);
    if (!errno)
    { cl_sendto(set); }
}

void cl_messaging(struct client_settings *set)
{
    struct sigaction sa;
    uint8_t          input_buffer[GAME_DATA_BYTES];
    
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
            cl_recvfrom(set, flag_set, sizeof(flag_set), (uint8_t) (set->s_packet->seq_num + 1));
        }
        
        create_packet(set->s_packet, FLAG_ACK, set->r_packet->seq_num, 0, NULL);
        cl_sendto(set);
        
        if (set->turn)
        {
            uint8_t cursor; /* The position of the cursor. */
            bool btn; /* Whether the button has been pressed. */
            // input buffer: 1 B cursor, 1 B btn press
            cursor = useController(set->game->cursor, &btn); // update the buffer, updating the button press
            
            input_buffer[0] = cursor;
            input_buffer[1] = btn;
            
            /* Send input to server. */
            create_packet(set->s_packet, FLAG_PSH, (uint8_t) (set->r_packet->seq_num + 1),
                          GAME_DATA_BYTES, input_buffer);
            cl_sendto(set);
            
            if (!errno)
            {
                uint8_t flag_set[] = {FLAG_ACK};
                cl_recvfrom(set, flag_set, sizeof(flag_set), set->s_packet->seq_num);
            }
        }
    }
    
    if (!errno)
    { cl_disconnect(set); }
}

void cl_disconnect(struct client_settings *set)
{
    create_packet(set->s_packet, FLAG_FIN, MAX_SEQ, 0, NULL);
    cl_sendto(set);
    if (!errno)
    {
        uint8_t flag_set[] = {FLAG_FIN | FLAG_ACK};
        cl_recvfrom(set, flag_set, sizeof(flag_set), set->s_packet->seq_num);
    }
    if (!errno)
    {
        uint8_t flag_set[] = {FLAG_FIN};
        cl_recvfrom(set, flag_set, sizeof(flag_set), set->s_packet->seq_num);
    }
    
    create_packet(set->s_packet, FLAG_FIN | FLAG_ACK, MAX_SEQ, 0, NULL);
    if (!errno)
    { cl_sendto(set); }
    if (!errno)
    {
        uint8_t flag_set[] = {FLAG_FIN};
        cl_recvfrom(set, flag_set, sizeof(flag_set), set->s_packet->seq_num);
    }
}

void cl_sendto(struct client_settings *set)
{
    socklen_t size_addr_in;
    uint8_t   *packet_buffer;
    size_t    packet_size;
    
    packet_buffer = serialize_packet(set->s_packet); /* Serialize the packet to send. */
    if (errno == ENOTRECOVERABLE)
    {
        running = 0;
        return;
    }
    set->mm->mm_add(set->mm, packet_buffer);
    
    size_addr_in = sizeof(struct sockaddr_in);
    packet_size  = PKT_STD_BYTES + set->s_packet->length;
    
    if (sendto(set->server_fd, packet_buffer, packet_size, 0, (struct sockaddr *) set->server_addr, size_addr_in) == -1)
    {
        /* errno will be set. */
        perror("Message transmission to server failed: ");
        return;
    }
    
    printf("\nSent packet:\n\tFlags: %s\n\tSequence number: %d\n\n",
           check_flags(set->s_packet->flags), set->s_packet->seq_num);
    
    set->mm->mm_free(set->mm, packet_buffer);
}

void cl_recvfrom(struct client_settings *set, uint8_t *flag_set, uint8_t num_flags, uint8_t seq_num)
{
    socklen_t size_addr_in;
    uint8_t   packet_buffer[BUF_LEN];
    bool      go_ahead;
    int       num_timeouts;
    
    memset(set->r_packet, 0, sizeof(struct packet));
    set->timeout->tv_sec = BASE_TIMEOUT;
    
    size_addr_in = sizeof(struct sockaddr_in);
    go_ahead     = false;
    num_timeouts = 0;
    do
    {
        printf("\n");
        for (uint8_t i = 0; i < num_flags; ++i)
        {
            printf("Awaiting response with flags: %s\n", check_flags(flag_set[i]));
        }
        
        /* Update socket's timeout. */
//        if (setsockopt(set->server_fd,
//                       SOL_SOCKET, SO_RCVTIMEO, (const char *) set->timeout, sizeof(struct timeval)) == -1)
//        {
//            fatal_errno(__FILE__, __func__, __LINE__, errno);
//            running = 0;
//            return;
//        }
        
        /* set->server_addr will be overwritten when a message is received on the socket set->server_fd */
        memset(packet_buffer, 0, BUF_LEN);
        if (recvfrom(set->server_fd, packet_buffer, BUF_LEN, 0, (struct sockaddr *) set->server_addr, &size_addr_in) == -1)
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
    
            /* Check the seq num and all flags in the set of accepted flags against
             * the seq num and flags in the packet_buffer.
             * If either is invalid at any point, we have the wrong packet. */
            for (uint8_t i = 0; i < num_flags; ++i)
            {
                if (!(go_ahead = *packet_buffer == flag_set[i] && *(packet_buffer + 1) == seq_num))
                {
                    cl_sendto(set);
                    break;
                }
            }
        }
    } while (!go_ahead);
    
    cl_process(set, packet_buffer); /* Once we have the correct packet, we will process it */
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
    cl_sendto(set);
    return 0;
}

void cl_process(struct client_settings *set, const uint8_t *packet_buffer)
{
    printf("\nReceived response:\n\tFlags: %s\n\tSequence number: %d\n",
           check_flags(*packet_buffer), *(packet_buffer + 1));
    
    deserialize_packet(set->r_packet, packet_buffer);
    if (errno == ENOMEM)
    {
        running = 0;
        return;
    }
    set->mm->mm_add(set->mm, set->r_packet->payload);
    
    if (set->r_packet->flags & FLAG_TRN) /* Indicates that it is this client's turn. */
    {
        set->turn = true;
    }
    
    if (set->r_packet->flags & FLAG_PSH) /* Indicates that the packet contains data which must be displayed. */
    {
        set->game->updateGameState(set->r_packet->payload, set->r_packet->payload + 1, set->r_packet->payload + 2);
        set->game->displayBoardWithCursor(set->game);
    }
    
    set->mm->mm_free(set->mm, set->r_packet->payload);
}

void close_client(struct client_settings *set)
{
    if (set->server_fd != 0)
    {
        close(set->server_fd);
    }
    free_memory_manager(set->mm);
    printf("Closing client.\n");
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
