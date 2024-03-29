//
// Created by Maxwell Babey on 10/24/22.
//

#include "../include/Game.h"
#include "../include/manager.h"
#include "../include/server-util.h"
#include "../include/server.h"
#include "../include/setup.h"
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

/**
 * The standard number of bytes in a payload; the size of the game data.
 * The game data is a uint8_t cursor, a char turn indicator, and a 9 B game state array.
 */
#define STD_PAYLOAD_BYTES (sizeof(uint8_t) + sizeof(char) + GAME_STATE_BYTES)

/**
 * While set to > 0, the program will continue running. Will be set to 0 by SIGINT or a catastrophic failure.
 */
static volatile sig_atomic_t running; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables) : var must change

/**
 * open_server
 * <p>
 * Create a socket and bind the IP specified in server_settings to the socket.
 * </p>
 * @param set - the server settings
 */
void open_server(struct server_settings *set);

/**
 * sv_comm_core
 * <p>
 * Create and zero the received packet and sent packet structs. Set the timeout option on the socket.
 * </p>
 * @param set - the server settings
 */
void sv_comm_core(struct server_settings *set);

/**
 * handle_receipt
 * <p>
 * Handle the receipt of a message on any sockets in readfds. If action is on the main socket, then it is a likely
 * a new connection and will be handled accordingly. If action is on any other socket, communicate with the client
 * attached to that socket.
 * </p>
 * @param set - the server settings
 * @param readfds - the set of file descriptors to read form
 */
void handle_receipt(struct server_settings *set, fd_set *readfds);

/**
 * handle_unicast
 * <p>
 * Convert the game state information into a byte array. In reply to a client who has just sent the same packet twice,
 * send the last sent packet with received packet seq num + 1 to the client.
 * </p>
 * @param set - the server settings
 * @param client - the client to which the message will be sent
 */
void handle_unicast(struct server_settings *set, struct conn_client *client);

/**
 * handle_broadcast
 * <p>
 * Convert the game state information into a byte array. For each connected client, send a packet.
 * The packet will have flags PSH or PSH/TRN, depending on the game state. The difference is interpreted
 * by the client to indicate turn status.
 * </p>
 * @param set - the server settings
 */
void handle_broadcast(struct server_settings *set);

/**
 * assemble_game_payload
 * <p>
 * Allocate memory to store the game state information.
 * </p>
 * @param game - the game to store the state of
 * @return pointer to the memory storing the game state
 */
uint8_t *assemble_game_payload(struct Game *game);

/**
 * sv_accept
 * <p>
 * Receive a message. If it is a SYN, connect the new client. Send a SYN/ACK back to the sender on that socket.
 * </p>
 * @param set - the server settings
 */
void sv_accept(struct server_settings *set);

/**
 * sv_recvfrom.
 * <p>
 * Await a message from the connected client. If no go ahead is received from processing the message, resend the last
 * sent message and receive again.
 * </p>
 * @param set - the server settings
 * @param client - the client from which to receive the message
 */
void sv_recvfrom(struct server_settings *set, struct conn_client *client);

/**
 * sv_process
 * <p>
 * Check the flags and sequence number of a message. Respond depending on the result.
 * </p>
 * @param set - the server settings
 * @param client - the client from which the message was received
 * @param packet_buffer - the buffer containing the message
 */
bool sv_process(struct server_settings *set, struct conn_client *client, const uint8_t *packet_buffer);

/**
 * sv_sendto
 * <p>
 * Send a send packet to a client.
 * </p>
 * @param set - the server settings
 * @param client - the client to which a packet will be sent
 */
void sv_sendto(struct server_settings *set, struct conn_client *client);

/**
 * sv_disconnect
 * <p>
 * Disconnect a client from the server by responding to a client FIN message and removing them from the server
 * connected client list.
 * </p>
 * @param set - the server settings
 * @param client - the client to be disconnected
 */
void sv_disconnect(struct server_settings *set, struct conn_client *client);

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
    struct sigaction sa;
    
    init_def_state(argc, argv, set);
    
    set_signal_handling(&sa);
    
    if (!errno)
    { open_server(set); }
    
    if (!errno)
    { sv_comm_core(set); }
}

void open_server(struct server_settings *set)
{
    struct sockaddr_in addr;
    
    if ((set->server_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) // NOLINT(android-cloexec-socket) : SOCK_CLOEXEC dne
    {
        fatal_errno(__FILE__, __func__, __LINE__, errno);
        return;
    }
    
    addr.sin_family           = AF_INET;
    addr.sin_port             = htons(set->server_port);
    if ((addr.sin_addr.s_addr = inet_addr(set->server_ip)) == (in_addr_t) -1)
    {
        fatal_errno(__FILE__, __func__, __LINE__, errno);
        return;
    }
    
    if (bind(set->server_fd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) == -1)
    {
        fatal_errno(__FILE__, __func__, __LINE__, errno);
        return;
    }
    
    printf("\nServer running on %s:%d\n", set->server_ip, set->server_port);
}

void sv_comm_core(struct server_settings *set)
{
    fd_set readfds;
    int    max_fd;
    
    running = 1;
    while (running)
    {
        max_fd = set_readfds(set, &readfds);
        
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
        
        set->do_broadcast = false; /* May be set true if received message is a PSH or ACK255. */
        set->do_unicast = false; /* May be set true if received message is a duplicate. */
        
        handle_receipt(set, &readfds); /* Handle a received message on any of the active sockets. */
        
        if (!errno &&
            set->num_conn_client == MAX_CLIENTS &&         /* If MAX_CLIENTS connected, */
            set->do_broadcast)                             /* If not retransmission, */
        {                                                  /* Broadcast game state to all connected clients. */
            handle_broadcast(set);
        }
        
        if (set->num_conn_client < MAX_CLIENTS) /* If fewer than MAX_CLIENTS, reset game state. */
        {
            set->game->updateGameState(set->game, NULL, NULL, NULL);
        }
    }
}

void handle_receipt(struct server_settings *set, fd_set *readfds)
{
    struct conn_client *curr_cli;
    
    /* If there is action on the main socket, it is a new connection. */
    if (FD_ISSET(set->server_fd, readfds))
    {
        if (!errno)
        { sv_accept(set); }
    } else
    {
        curr_cli = set->first_conn_client;
        for (int cli_num = 0;
             curr_cli != NULL && cli_num < MAX_CLIENTS; ++cli_num)  /* Receive messages from each client. */
        {
            if (FD_ISSET(curr_cli->c_fd, readfds))
            {
                if (!errno)
                { sv_recvfrom(set, curr_cli); }
            }
            if (!errno && !set->do_broadcast && set->do_unicast)
            {
                handle_unicast(set, curr_cli);
                set->do_unicast = false;
            }
            if (curr_cli->r_packet->flags == FLAG_FIN)
            {
                struct conn_client *disconn_client;
                
                disconn_client = curr_cli;
                curr_cli       = curr_cli->next; /* Go to next client in list. */
                
                sv_disconnect(set, disconn_client);
            } else
            {
                curr_cli = curr_cli->next; /* Go to next client in list. */
            }
        }
    }
}

void handle_unicast(struct server_settings *set, struct conn_client *client)
{
    uint8_t *payload;
    
    if ((payload = assemble_game_payload(set->game)) == NULL)
    {
        running = 0;
        return;
    }
    set->mm->mm_add(set->mm, payload);
    
    if (!errno)
    {
        create_packet(client->s_packet,
                      client->s_packet->flags,
                      (uint8_t) (client->r_packet->seq_num + 1),
                      STD_PAYLOAD_BYTES, payload);
    }
    if (!errno)
    { sv_sendto(set, client); }
    if (!errno)
    { sv_recvfrom(set, client); }
    
    set->mm->mm_free(set->mm, payload);
}

void handle_broadcast(struct server_settings *set)
{
    struct conn_client *curr_cli;
    uint8_t            *payload;
    
    if ((payload = assemble_game_payload(set->game)) == NULL)
    {
        running = 0;
        return;
    }
    set->mm->mm_add(set->mm, payload);
    
    curr_cli = set->first_conn_client;
    for (int cli_num = 0; curr_cli != NULL && cli_num < MAX_CLIENTS; ++cli_num)
    {
        /* Decide which client's turn it is. That client will be sent a PSH/TRN */
        if (!errno)
        {
            uint8_t flags = (cli_num == set->game->turn % MAX_CLIENTS) ? (FLAG_PSH | FLAG_TRN) : FLAG_PSH;
            create_packet(curr_cli->s_packet, flags, (uint8_t) (curr_cli->r_packet->seq_num + 1),
                          STD_PAYLOAD_BYTES, payload);
        }
        if (!errno)
        { sv_sendto(set, curr_cli); }
        if (!errno)
        { sv_recvfrom(set, curr_cli); }
        
        curr_cli = curr_cli->next;
    }
    
    set->mm->mm_free(set->mm, payload);
}

uint8_t *assemble_game_payload(struct Game *game)
{
    uint8_t *payload;
    
    if ((payload = (uint8_t *) s_calloc(STD_PAYLOAD_BYTES, sizeof(uint8_t),
                                        __FILE__, __func__, __LINE__)) == NULL)
    {
        return NULL;
    }
    
    *payload       = game->cursor;
    *(payload + 1) = game->turn;
    memcpy(payload + 2, game->trackGame, sizeof(game->trackGame));
    
    return payload;
}

void sv_accept(struct server_settings *set)
{
    struct sockaddr_in from_addr;
    socklen_t          size_addr_in;
    uint8_t            buffer[HLEN_BYTES];
    
    size_addr_in = sizeof(struct sockaddr_in);
    
    /* Get client sockaddr_in here. */
    if ((recvfrom(set->server_fd, buffer, sizeof(buffer), 0, (struct sockaddr *) &from_addr, &size_addr_in)) == -1)
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
        if (!errno)
        { sv_recvfrom(set, new_client); }
    } else if (set->num_conn_client == MAX_CLIENTS && *buffer == FLAG_SYN)
    {
        printf("\n--- Client connection denied: lobby full ---\n");
    }
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
    packet_size  = HLEN_BYTES + client->s_packet->length;
    
    printf("\nSending packet:\n\tIP: %s\n\tPort: %u\n\tFlags: %s\n\tSequence Number: %d\n",
           inet_ntoa(client->addr->sin_addr), // NOLINT(concurrency-mt-unsafe) : no threads here
           ntohs(client->addr->sin_port),
           check_flags(client->s_packet->flags),
           client->s_packet->seq_num);
    
    if (sendto(client->c_fd, packet_buffer, packet_size, 0, (struct sockaddr *) client->addr, size_addr_in) == -1)
    {
        perror("\nMessage transmission to client failed: \n");
        return;
    }
    
    set->mm->mm_free(set->mm, packet_buffer);
}

void sv_recvfrom(struct server_settings *set, struct conn_client *client)
{
    uint8_t   packet_buffer[HLEN_BYTES + GAME_RECV_BYTES];
    socklen_t size_addr_in;
    bool      go_ahead;
    
    size_addr_in = sizeof(struct sockaddr_in);
    go_ahead     = false;
    do
    {
        memset(packet_buffer, 0, sizeof(packet_buffer));
        if (recvfrom(client->c_fd, packet_buffer, sizeof(packet_buffer), 0,
                     (struct sockaddr *) client->addr, &size_addr_in) == -1)
        {
            switch (errno)
            {
                case EINTR: /* User presses ctrl+C */
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
    printf("\nReceived packet:\n\tIP: %s\n\tPort: %u\n\tFlags: %s\n\tSequence Number: %d\n",
           inet_ntoa(client->addr->sin_addr), // NOLINT(concurrency-mt-unsafe) : no threads here
           ntohs(client->addr->sin_port),
           check_flags(*packet_buffer),
           *(packet_buffer + 1));
    
    if ((*packet_buffer == client->r_packet->flags) &&
        (*(packet_buffer + 1) == client->r_packet->seq_num))
    {
        set->do_unicast = true;
        return true; /* Retransmission received: go ahead. */
    }
    if ((*packet_buffer == FLAG_ACK) &&
        (*(packet_buffer + 1) != client->s_packet->seq_num))
    {
        return false; /* Bad seq num: do not go ahead. */
    }
    if (*packet_buffer == (FLAG_FIN | FLAG_ACK))
    {
        remove_client(set, client);
        return true; /* Client disconnected: go ahead. */
    }
    
    deserialize_packet(client->r_packet, packet_buffer); /* Deserialize the packet to store its contents. */
    if (errno == ENOMEM)
    {
        running = 0;
        return true; /* Deserializing failure: go ahead. */
    }
    set->mm->mm_add(set->mm, client->r_packet->payload);
    
    if ((*packet_buffer & FLAG_PSH) &&
        (*(packet_buffer + 1) == (uint8_t) (client->s_packet->seq_num + 1)))
    {
        create_packet(client->s_packet, FLAG_ACK, client->r_packet->seq_num, 0, NULL);
        sv_sendto(set, client);
        
        /* Update the game state. */
        set->game->cursor = *client->r_packet->payload;
        if (*(client->r_packet->payload + 1))
        {
            set->game->updateBoard(set->game);
        }
        
        set->do_broadcast = true; /* Do a broadcast because the game state was just updated. */
    }
    if ((*packet_buffer == FLAG_ACK) && (*(packet_buffer + 1) == MAX_SEQ))
    {
        set->do_broadcast = true; /* Do a broadcast because the game has just started. */
    }
    
    set->mm->mm_free(set->mm, client->r_packet->payload);
    client->r_packet->payload = NULL;
    
    return true; /* Good message received: go ahead. */
}

void sv_disconnect(struct server_settings *set, struct conn_client *client)
{
    create_packet(client->s_packet, FLAG_FIN | FLAG_ACK, MAX_SEQ, 0, NULL);
    sv_sendto(set, client);
    if (!errno)
    {
        create_packet(client->s_packet, FLAG_FIN, MAX_SEQ, 0, NULL);
        sv_sendto(set, client);
    }
    if (!errno)
    { sv_recvfrom(set, client); } /* Wait for FIN/ACK */
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
