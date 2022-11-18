//
// Created by Maxwell Babey on 10/24/22.
//

#include "../include/manager.h"
#include "../include/server.h"
#include "../include/server-util.h"
#include "../include/setup.h"
#include "../include/Game.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

/**
 * The number of connections which may be queued at once. NOTE: server does not currently time out.
 */
//#define MAX_TIMEOUTS_SERVER 3

/**
 * While set to > 0, the program will continue running. Will be set to 0 by SIGINT or a catastrophic failure.
 */
static volatile sig_atomic_t running; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables) : var must change

/**
 * open_server
 * <p>
 * Create a socket, bind the IP specified in server_settings to the socket, then listen on the socket.
 * </p>
 * @param set - server_settings *: pointer to the settings for this server
 */
void open_server(struct server_settings *set);

/**
 * await_connect
 * <p>
 * Await and accept client connections. If program is interrupted by the user while waiting, close the server.
 * </p>
 * @param set - server_settings *: pointer to the settings for this server
 */
void await_connect(struct server_settings *set);

/**
 * connect_to
 * <p>
 * Create and zero the received packet and sent packet structs. Set the timeout option on the socket.
 * </p>
 * @param set - the server settings
 */
void connect_to(struct server_settings *set);

/**
 * sv_accept
 * <p>
 * Receive a message. If it is a SYN, connect the new client. Send a SYN/ACK back to the sender on that socket.
 * </p>
 * @param set - the server settings
 * @param s_packet - the packet struct to send
 */
void sv_accept(struct server_settings *set);

/**
 * sv_recvfrom.
 * <p>
 * Await a message from the connected client. If no message is received and a timeout occurs,
 * resend the last-sent packet. If a timeout occurs too many times, drop the connection.
 * </p>
 * @param set - the server settings
 * @param s_packet - the packet struct to send
 * @param r_packet - the packet struct to sv_recvfrom
 */
void sv_recvfrom(struct server_settings *set, struct conn_client *client);

/**
 * decode_message.
 * <p>
 * Load the bytes of the data buffer into the sv_recvfrom packet struct fields.
 * </p>
 * @param set - the server settings
 * @param packet_buffer - the data buffer to load into the received packet
 * @param r_packet - the packet struct to sv_recvfrom
 */
int decode_message(struct server_settings *set, struct packet *r_packet, const uint8_t *packet_buffer);

/**
 * sv_process
 * <p>
 * Check the flags in the packet struct. Depending on the flags, respond accordingly.
 * </p>
 * @param set - the server settings
 * @param s_packet - the packet struct to send
 * @param r_packet - the packet struct to sv_recvfrom
 */
bool sv_process(struct server_settings *set, struct conn_client *client, const uint8_t *packet_buffer);

/**
 * sv_sendto
 * <p>
 * Send the send packet to the client as a response.
 * </p>
 * @param set - the server settings
 * @param s_packet - the packet struct to send
 */
void sv_sendto(struct server_settings *set, struct conn_client *client);

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

void run(int argc, char *argv[], struct server_settings *set)
{
    init_def_state(argc, argv, set);
    
    open_server(set);
    
    if (!errno)
    { await_connect(set); }
    
    /* return to main. */
}

void open_server(struct server_settings *set)
{
    if ((set->server_fd = open_server_socket(set->server_ip, set->server_port)) == -1)
    {
        return;
    }
    printf("\nServer running on %s:%d\n", set->server_ip, set->server_port);
}

void await_connect(struct server_settings *set)
{
    struct sigaction sa;
    
    set_signal_handling(&sa);
    running = 1;
    
    while (running)
    {
        connect_to(set);
    }
}

void connect_to(struct server_settings *set)
{
    struct conn_client *curr_cli;
    fd_set             readfds;
    int                max_fd;
    // pthread_t cli_threads[MAX_CLIENTS]
    
    max_fd = set_readfds(set, &readfds);
    
    // set->timeout->tv_sec = modify_timeout(timeout_count); // Here in case a timeout will be set in select.
    
    if (select(max_fd + 1, &readfds, NULL, NULL, NULL) == -1)
    {
        switch (errno)
        {
            case EINTR:
            {
                // running set to 0 with signal handler.
                return;
            }
            default:
            {
                fatal_errno(__FILE__, __func__, __LINE__, errno);
                running = 0;
                return;
            }
        }
    }
    
    /* If there is action on the main socket, it is a new connection. */
    if (FD_ISSET(set->server_fd, &readfds))
    {
        if (!errno)
        { sv_accept(set); }
    } else
    {
        /* Receive messages from each client. */
        curr_cli = set->first_conn_client;
        for (int cli_num = 0; curr_cli != NULL && cli_num < MAX_CLIENTS; ++cli_num)
        {
            if (FD_ISSET(curr_cli->c_fd, &readfds))
            {
                // cli_threads[cli_num] = new pthread
                // create_pthread(cli_threads[cli_num], sv_recvfrom, cli)
                if (!errno)
                { sv_recvfrom(set, curr_cli); }
            }
            curr_cli = curr_cli->next; /* Go to next client in list. */
        }
    }
    /* for i < MAX_CLIENTS, pthread_join(cli_threads[i], NULL) */
    
    if (set->num_conn_client == MAX_CLIENTS) /* If MAX_CLIENTS clients are connected, the game is running. */
    {
        // TODO: Comm with game to get game state update
        
        curr_cli = set->first_conn_client;
        for (int cli_num = 0; curr_cli != NULL && cli_num < MAX_CLIENTS; ++cli_num)
        {
            /* Decide which client's turn it is. That client will be sent a PSH/TRN */
            uint8_t flags = (cli_num == set->game->turn % MAX_CLIENTS) ? (FLAG_PSH | FLAG_TRN) : FLAG_PSH;
            create_packet(curr_cli->s_packet, flags, (uint8_t) (curr_cli->r_packet->seq_num + 1), 0, NULL);
            sv_sendto(set, curr_cli);
            sv_recvfrom(set, curr_cli);
        }
    }
}

void sv_accept(struct server_settings *set)
{
    struct sockaddr_in from_addr;
    socklen_t          size_addr_in;
    uint8_t            buffer[BUF_LEN];
    
    size_addr_in = sizeof(struct sockaddr_in);
    
    /* Get client sockaddr_in here. */
    if ((recvfrom(set->server_fd, buffer, BUF_LEN, 0, (struct sockaddr *) &from_addr, &size_addr_in)) == -1)
    {
        switch (errno)
        {
            case EINTR:
            {
                /* running set to 0 with signal handler. */
                return;
            }
            default:
            {
                fatal_errno(__FILE__, __func__, __LINE__, errno);
                running = 0;
                return;
            }
        }
    }
    
    if (set->num_conn_client < MAX_CLIENTS && *buffer == FLAG_SYN) /* If the message received was a SYN packet. */
    {
        struct conn_client *new_client;
        if ((new_client = connect_client(set, &from_addr)) == NULL)
        {
            running = 0;
            return; // errno set
        }
    
        printf("\nClient connected from: %s:%u\n",
               inet_ntoa(new_client->addr->sin_addr), // NOLINT(concurrency-mt-unsafe) : no threads here
               ntohs(new_client->addr->sin_port));
    
        create_packet(new_client->s_packet, FLAG_SYN | FLAG_ACK, MAX_SEQ, 0, NULL);
        if (!errno)
        { sv_sendto(set, new_client); }
    }
}

void sv_recvfrom(struct server_settings *set, struct conn_client *client)
{
    uint8_t   packet_buffer[BUF_LEN];
    socklen_t size_addr_in;
    bool      go_ahead;
    
    memset(client->r_packet, 0, sizeof(struct packet));
    
    size_addr_in = sizeof(struct sockaddr_in);
    go_ahead     = false;
    do
    {
        memset(packet_buffer, 0, BUF_LEN);
        if (recvfrom(client->c_fd, packet_buffer, BUF_LEN, 0, (struct sockaddr *) client->addr, &size_addr_in) == -1)
        {
            switch (errno)
            {
                case EINTR:
                {
                    // running set to 0 with signal handler.
                    return;
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
            /* If bad message received, do not go ahead. If good message received, do go ahead. */
            if (!(go_ahead = sv_process(set, client, packet_buffer)))
            {
                sv_sendto(set, client); /* Case: bad ACK seq num, retransmit. */
            }
        }
    } while (!go_ahead);
}

bool sv_process(struct server_settings *set, struct conn_client *client, const uint8_t *packet_buffer)
{
    printf("\nReceived packet:\n\tFlags: %s\n\tSequence Number: %d\n",
           check_flags(*packet_buffer), *(packet_buffer + 1));
    
    /* If wrong ACK, can save some time by not decoding. */
    if ((*packet_buffer == FLAG_ACK) &&
        (*(packet_buffer + 1) != client->s_packet->seq_num))
    {
        return false; /* Bad seq num: do not go ahead. */
    }
    
    if (decode_message(set, client->r_packet, packet_buffer) == -1)
    {
        running = 0;
        return true; /* Decoding failure: go ahead. */
    }
    
    if ((client->r_packet->flags & FLAG_PSH) &&
        (client->r_packet->seq_num == (uint8_t) (client->s_packet->seq_num + 1))) // cracked cast
    {
        create_packet(client->s_packet, FLAG_ACK, client->r_packet->seq_num, 0, NULL);
        sv_sendto(set, client);
        
        // TODO: put the payload into Game struct
        set->game->cursor = *client->r_packet->payload;
        
        set->game->updateBoard(set->game);
        
        set->mm->mm_free(set->mm, client->r_packet->payload);
        client->r_packet->payload = NULL;
        
    } else if (client->r_packet->flags == FLAG_FIN)
    {
        create_packet(client->s_packet, FLAG_FIN | FLAG_ACK, MAX_SEQ, 0, NULL);
        sv_sendto(set, client);
        if (!errno)
        {
            create_packet(client->s_packet, FLAG_FIN, MAX_SEQ, 0, NULL);
            sv_sendto(set, client);
        }
    }
    
    return true; /* Good message received: go ahead. */
}

int decode_message(struct server_settings *set, struct packet *r_packet, const uint8_t *packet_buffer)
{
    deserialize_packet(r_packet, packet_buffer);
    if (errno == ENOMEM)
    {
        return -1;
    }
    set->mm->mm_add(set->mm, r_packet->payload);
    
    return 0;
}

void sv_sendto(struct server_settings *set, struct conn_client *client)
{
    uint8_t   *packet_buffer = NULL;
    socklen_t size_addr_in;
    size_t    packet_size;
    
    if ((packet_buffer = serialize_packet(client->s_packet)) == NULL)
    {
        running = 0;
        return;
    }
    set->mm->mm_add(set->mm, packet_buffer);
    
    size_addr_in = sizeof(struct sockaddr_in);
    packet_size  = PKT_STD_BYTES + client->s_packet->length;
    
    printf("\nSending packet:\n\tFlags: %s\n\tSequence Number: %d\n",
           check_flags(client->s_packet->flags), client->s_packet->seq_num);
    
    if (sendto(client->c_fd, packet_buffer, packet_size, 0, (struct sockaddr *) client->addr, size_addr_in) == -1)
    {
        perror("\nMessage transmission to client failed: \n");
        return;
    }
    
    printf("\nSending to: %s:%u\n",
           inet_ntoa(client->addr->sin_addr), // NOLINT(concurrency-mt-unsafe) : no threads here
           ntohs(client->addr->sin_port));
    
    set->mm->mm_free(set->mm, packet_buffer);
}

void close_server(struct server_settings *set)
{
    printf("\nClosing server.\n");
    
    if (set->server_fd != 0)
    {
        close(set->server_fd);
    }
    if (set->first_conn_client != NULL)
    {
        for (struct conn_client *curr_cli = set->first_conn_client; curr_cli != NULL; curr_cli = curr_cli->next)
        {
            if (curr_cli->c_fd != 0)
            {
                close(curr_cli->c_fd);
            }
        }
    }
    free_memory_manager(set->mm);
}

static void set_signal_handling(struct sigaction *sa)
{
    sigemptyset(&sa->sa_mask);
    sa->sa_flags   = 0;
    sa->sa_handler = signal_handler;
    if (sigaction(SIGINT, sa, 0) == -1)
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
