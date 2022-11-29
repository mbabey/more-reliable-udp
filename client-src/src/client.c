#include "../include/Controller.h"
#include "../include/Game.h"
#include "../include/client-util.h"
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
 * The maximum number of timeouts to occur before the timeout duration is reduced.
 */
#define MAX_NUM_TIMEOUTS 3

/**
 * Timeout duration below which a connection is deemed failed.
 */
#define MIN_TIMEOUT 1 /* seconds */

/**
 * The base timeout duration before retransmission.
 */
#define BASE_TIMEOUT 8 /* seconds */

/**
 * The number of bytes needed to be sent to the server to update the server-side game state.
 */
#define GAME_SEND_BYTES 2

/**
 * While set to > 0, the program will continue running. Will be set to 0 by SIGINT or a catastrophic failure.
 */
static volatile sig_atomic_t running; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables) : var must change

/**
 * open_client_socket
 * <p>
 * Set up the socket for the client. Setup the server sockaddr_in struct.
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
 * take_turn
 * <p>
 * Take input from the controller and send it to the server.
 * </p>
 * @param set - the settings for the client
 */
void take_turn(struct client_settings *set);

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
 * then wait again. If MAX_NUM_TIMEOUTS timeouts occur, set running to 0 and return. If the message received from the
 * server does not match the expected flags and sequence number, retransmit the last sent packet.
 * </p>
 * @param set - the client settings
 * @param flag_set - the expected flags to be received
 * @param num_flags - the number of different expected flags
 * @param seq_num - the expected sequence number to be received
 */
void cl_recvfrom(struct client_settings *set, const uint8_t *flag_set, uint8_t num_flags, uint8_t seq_num);

/**
 * cl_recvfrom_err
 * <p>
 * Handle an error occurring while receiving a message. Returns 0 if the error is a timeout and the timeout limit has
 * not been reached, or returns -1 if another error type occurs.
 * </p>
 * @param set - the client settings
 * @param num_to - the number of timeouts that have occurred
 * @return 0 if a timeout error occurs and timeout limit has not been reached, -1 otherwise
 */
int cl_recvfrom_err(struct client_settings *set, int *num_to);

/**
 * hande_recv_timeout
 * <p>
 * If the number of timeouts occurring at the current timeout interval is equal to or exceeds MAX_NUM_TIMEOUTS,
 * reduce the current timeout interval by one half. If the timeout interval is then less than MIN_TIMEOUT, print a
 * relevant message, set running to 0, and return -1. Otherwise, return 0.
 * </p>
 * @param set - the client settings
 * @param num_to - the number of timeouts that have occurred at the current interval
 * @return -1 if the timeout interval has been reduced below the limit, 0 if it has not
 */
int handle_timeout(struct client_settings *set, int *num_to);

/**
 * cl_process
 * <p>
 * React to the received packet appropriately based on its flags and sequence number.
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

void run(int argc, char *argv[], struct client_settings *set)
{
    struct sigaction sa;
    
    init_def_state(argc, argv, set);
    
    set_signal_handling(&sa);
    
    if (!errno)
    { open_client_socket(set); }
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
    create_packet(set->s_packet, FLAG_SYN, MAX_SEQ, 0, NULL);
    cl_sendto(set);
    if (!errno)
    {
        uint8_t flag_set[] = {FLAG_SYN | FLAG_ACK};
        cl_recvfrom(set, flag_set, sizeof(flag_set), set->s_packet->seq_num);
    }
    
    if (!errno)
    {
        printf("\nConnected to server %s:%u\n",
               inet_ntoa(set->server_addr->sin_addr), // NOLINT(concurrency-mt-unsafe) : no threads here
               ntohs(set->server_addr->sin_port));
        
        create_packet(set->s_packet, FLAG_ACK, MAX_SEQ, 0, NULL);
        cl_sendto(set);
    }
}

void cl_messaging(struct client_settings *set) //
{
    running = 1;
    while (running)
    {
        set->turn = false; /* Clean the turn indicator. */
        
        /* Update game board, set turn. */
        uint8_t flag_set[] = {FLAG_PSH, FLAG_PSH | FLAG_TRN};
        cl_recvfrom(set, flag_set, sizeof(flag_set), (uint8_t) (set->s_packet->seq_num + 1));
        
        if (!errno)
        {
            create_packet(set->s_packet, FLAG_ACK, set->r_packet->seq_num, 0, NULL);
            cl_sendto(set);
        }
        
        if (set->turn)
        {
            take_turn(set);
        }
    }
    
    if (!errno || errno == EINTR)
    { cl_disconnect(set); }
}

void take_turn(struct client_settings *set)
{
    volatile uint8_t cursor; /* The position of the cursor. */
    volatile bool    btn = false; /* Whether the button has been pressed. */
    uint8_t          input_buffer[GAME_SEND_BYTES];
    // input buffer: 1 B cursor, 1 B btn press
    cursor = useController(set->game->cursor, &btn); // update the buffer, updating the button press

    input_buffer[0] = cursor;
    input_buffer[1] = (uint8_t) btn;
    
    /* Send input to server. */
    create_packet(set->s_packet, FLAG_PSH, (uint8_t) (set->r_packet->seq_num + 1),
                  GAME_SEND_BYTES, input_buffer);
    if (!errno)
    { cl_sendto(set); }
    
    if (!errno)
    {
        uint8_t flag_set[] = {FLAG_ACK};
        cl_recvfrom(set, flag_set, sizeof(flag_set), set->s_packet->seq_num);
    }
}

void cl_sendto(struct client_settings *set)
{
    socklen_t size_addr_in;
    uint8_t   *buffer;
    size_t    packet_size;
    
    buffer = serialize_packet(set->s_packet); /* Serialize the packet to send. */
    if (errno == ENOTRECOVERABLE)
    {
        running = 0;
        return;
    }
    set->mm->mm_add(set->mm, buffer);
    
    size_addr_in = sizeof(struct sockaddr_in);
    packet_size  = HLEN_BYTES + set->s_packet->length;
    
    if (sendto(set->server_fd, buffer, packet_size, 0, (struct sockaddr *) set->server_addr, size_addr_in) == -1)
    {
        /* errno will be set. */
        perror("Message transmission to server failed: ");
        return;
    }
    
//    printf("\nSending packet:\n\tIP: %s\n\tPort: %u\n\tFlags: %s\n\tSequence Number: %d\n",
//           inet_ntoa(set->server_addr->sin_addr), // NOLINT(concurrency-mt-unsafe) : no threads here
//           ntohs(set->server_addr->sin_port),
//           check_flags(set->s_packet->flags),
//           set->s_packet->seq_num);
    
    set->mm->mm_free(set->mm, buffer);
}

void cl_recvfrom(struct client_settings *set, const uint8_t *flag_set, uint8_t num_flags, uint8_t seq_num)
{
    socklen_t size_addr_in;
    uint8_t   buffer[HLEN_BYTES + GAME_SEND_BYTES + GAME_STATE_BYTES];
    bool      go_ahead;
    int num_to;
    
    set->timeout->tv_sec = BASE_TIMEOUT;
    
    size_addr_in = sizeof(struct sockaddr_in);
    go_ahead     = false;
    num_to = 0;
    do
    {
//        printf("\nAwaiting packet with flags: ");
//        for (uint8_t i = 0; i < num_flags; ++i)
//        {
//            printf("%s, ", check_flags(flag_set[i]));
//        }
//        printf("\n");
        
        

        memset(buffer, 0, sizeof(buffer));
        if (recvfrom(set->server_fd, buffer, sizeof(buffer), 0,
                     (struct sockaddr *) set->server_addr, &size_addr_in) == -1)
        {
            if (cl_recvfrom_err(set, &num_to) == -1)
            {
                return;
            }
            cl_sendto(set); /* Timeout limit not exceeded, retransmit. */
        } else
        {
            /* Packet received: reset the timeout. */
            num_to = 0;
            set->timeout->tv_usec = BASE_TIMEOUT;
    
            /* Check the seq num and all flags in the set of accepted flags against
             * the seq num and flags in the buffer. If one is valid, we have right packet.
             * Otherwise, resend the last sent packet. */
            for (uint8_t i = 0; i < num_flags; ++i)
            {
                if ((go_ahead = *buffer == flag_set[i] && *(buffer + 1) == seq_num))
                {
                    break;
                }
            }
            
            if (!go_ahead)
            {
                cl_sendto(set);
            }
        }
    } while (!go_ahead);
    
    cl_process(set, buffer); /* Once we have the correct packet, we will process it */
}

int cl_recvfrom_err(struct client_settings *set, int *num_to)
{
    int ret_val;
    
    switch (errno)
    {
        case EINTR: /* If the user presses ctrl+C */
        {
            ret_val = -1;
            break;
        }
        case EWOULDBLOCK: /* If the socket times out */
        {
            errno = 0;
            ret_val = handle_timeout(set, num_to);
            break;
        }
        default: /* Any other error is not predicted */
        {
            fatal_errno(__FILE__, __func__, __LINE__, errno);
            running = 0;
            ret_val = -1;
            break;
        }
    }
    
    return ret_val;
}

int handle_timeout(struct client_settings *set, int *num_to)
{
    if (++(*num_to) >= MAX_NUM_TIMEOUTS) /* Every MAX_NUM_TIMEOUTS timeouts, reduce the timeout time. */
    {
        *num_to = 0;
        set->timeout->tv_sec /= 2; /* Reduce the timeout duration by a factor of one half. */
    }
    
    if (set->timeout->tv_sec < MIN_TIMEOUT)
    {
        if (set->s_packet->flags == FLAG_SYN) /* Connection to server failed. */
        {
            printf("\nServer connection request timed out.\n");
        } else if (set->s_packet->flags == (FLAG_FIN | FLAG_ACK)) /* Waiting to see if server missed FIN/ACK. */
        {
            printf("\nAssuming server disconnected.\n");
        } else
        {
            printf("\nConnection to server interrupted.\n");
        }
        running = 0;
        return -1;
    }

//    printf("\nTimeout occurred. Next timeout in %ld seconds.\n", set->timeout->tv_sec);
    return 0;
}

void cl_process(struct client_settings *set, const uint8_t *packet_buffer)
{
//    printf("\nReceived packet:\n\tIP: %s\n\tPort: %u\n\tFlags: %s\n\tSequence Number: %d\n",
//           inet_ntoa(set->server_addr->sin_addr), // NOLINT(concurrency-mt-unsafe) : no threads here
//           ntohs(set->server_addr->sin_port),
//           check_flags(*packet_buffer),
//           *(packet_buffer + 1));
    
    if (*packet_buffer == FLAG_ACK || *packet_buffer & FLAG_FIN)
    {
        return;
    }
    
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
        set->game->updateGameState(set->game, set->r_packet->payload, (char *) set->r_packet->payload + 1,
                                   set->r_packet->payload + 2);
        set->game->displayBoardWithCursor(set->game);
        if (set->game->isGameOver(set->game))
        {
            running = 0;
            set->turn = false;
        }
    }
    
    set->mm->mm_free(set->mm, set->r_packet->payload);
}

void cl_disconnect(struct client_settings *set)
{
    errno = 0;
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
