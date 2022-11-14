//
// Created by Maxwell Babey on 10/24/22.
//

#include "../../libs/include/error.h"
#include "../../libs/include/manager.h"
#include "../../libs/include/util.h"
#include "../include/server.h"
#include "../include/setup.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

/**
 * The number of connections which may be queued at once. NOTE: server does not currently time out.
 */
//#define MAX_TIMEOUTS_SERVER 3

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
 * do_accept
 * <p>
 * Receive a message. If it is a SYN, connect the new client. Send a SYN/ACK back to the sender on that socket.
 * </p>
 * @param set - the server settings
 * @param s_packet - the packet struct to send
 */
void do_accept(struct server_settings *set);

/**
 * connect_new_client
 * <p>
 * Connect a new client. Store the client's information and create a new socket with which to
 * exchange messages with that client. Increment the number of connected clients.
 * </p>
 * @param set - the client settings
 * @param from_addr - the address from which the message was sent
 * @return - a pointer to the newly connected client.
 */
struct conn_client *connect_new_client(struct server_settings *set, struct sockaddr_in *from_addr);

/**
 * do_await.
 * <p>
 * Await a message from the connected client. If no message is received and a timeout occurs,
 * resend the last-sent packet. If a timeout occurs too many times, drop the connection.
 * </p>
 * @param set - the server settings
 * @param s_packet - the packet struct to send
 * @param r_packet - the packet struct to receive
 */
void do_await(struct server_settings *set, struct conn_client *client);

/**
 * decode_message.
 * <p>
 * Load the bytes of the data buffer into the receive packet struct fields.
 * </p>
 * @param set - the server settings
 * @param data_buffer - the data buffer to load into the received packet
 * @param r_packet - the packet struct to receive
 */
int decode_message(struct server_settings *set, struct packet *r_packet, const uint8_t *data_buffer);

/**
 * process_message
 * <p>
 * Check the flags in the packet struct. Depending on the flags, respond accordingly.
 * </p>
 * @param set - the server settings
 * @param s_packet - the packet struct to send
 * @param r_packet - the packet struct to receive
 */
bool process_message(struct server_settings *set, struct conn_client *client, const uint8_t *data_buffer);

/**
 * print_payload
 * <p>
 * Write the information in the payload to the output specified in the server settings.
 * </p>
 * @param set - the server settings
 * @param r_packet - the packet struct to receive
 */
void print_payload(struct server_settings *set, struct packet *r_packet);

/**
 * send_pack
 * <p>
 * Send the send packet to the client as a response.
 * </p>
 * @param set - the server settings
 * @param s_packet - the packet struct to send
 */
void send_pack(struct server_settings *set, struct conn_client *client);

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
    if ((set->server_fd = setup_socket(set->server_ip, set->server_port)) == -1)
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
        { do_accept(set); }
    } else
    {
        /* Receive messages from each client. */
        curr_cli = set->first_conn_client;
        for (int cli_num = 0; curr_cli != NULL && cli_num < MAX_CLIENTS; ++cli_num)
        {
            if (FD_ISSET(curr_cli->c_fd, &readfds))
            {
                // cli_threads[cli_num] = new pthread
                // create_pthread(cli_threads[cli_num], do_await, cli)
                if (!errno)
                { do_await(set, curr_cli); }
            }
            curr_cli = curr_cli->next; /* Go to next client in list. */
        }
    }
    /* for i < MAX_CLIENTS, pthread_join(cli_threads[i], NULL) */
    
    if (set->num_conn_client == MAX_CLIENTS) /* If MAX_CLIENTS clients are connected, the game is running. */
    {
        // Comm with game to get game state update
        
        curr_cli = set->first_conn_client;
        for (int cli_num = 0; curr_cli != NULL && cli_num < MAX_CLIENTS; ++cli_num)
        {
            /* Decide which client's turn it is. That client will be sent a PSH/TRN */
            uint8_t flags = (cli_num == set->game->turn % MAX_CLIENTS) ? (FLAG_PSH | FLAG_TRN) : FLAG_PSH;
            create_pack(curr_cli->s_packet, flags, (uint8_t) (curr_cli->r_packet->seq_num + 1), 0, NULL);
            send_pack(set, curr_cli);
            do_await(set, curr_cli);
        }
    }
}

void do_accept(struct server_settings *set)
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
        if ((new_client = connect_new_client(set, &from_addr)) == NULL)
        {
            running = 0;
            return; // errno set
        }
        
        printf("\nClient connected from: %s:%u\n",
               inet_ntoa(new_client->addr->sin_addr), // NOLINT(concurrency-mt-unsafe) : no threads here
               ntohs(new_client->addr->sin_port));
        
        create_pack(new_client->s_packet, FLAG_SYN | FLAG_ACK, MAX_SEQ, 0, NULL);
        if (!errno)
        { send_pack(set, new_client); }
    }
}

struct conn_client *connect_new_client(struct server_settings *set, struct sockaddr_in *from_addr)
{
    struct conn_client *new_client;
    
    /* Initialize storage for information about the new client. */
    if ((new_client = create_conn_client(set)) ==
        NULL) // TODO(maxwell): the client is never removed from the linked list.
    {
        return NULL; // errno set
    }
    *new_client->addr = *from_addr; /* Copy the sender's information into the client struct. */
    
    /* Create a new socket with an ephemeral port. */
    if ((new_client->c_fd = setup_socket(set->server_ip, 0)) == -1)
    {
        return NULL; // errno set
    }
    
    ++set->num_conn_client; /* Increment the number of connected clients. */
    
    return new_client;
}

void do_await(struct server_settings *set, struct conn_client *client)
{
    uint8_t   buffer[BUF_LEN];
    socklen_t size_addr_in;
    bool      resend;
    
    memset(client->r_packet, 0, sizeof(struct packet));
    memset(buffer, 0, BUF_LEN);
    
    size_addr_in = sizeof(struct sockaddr_in);
    resend = false;
    do
    {
        if (recvfrom(client->c_fd, buffer, BUF_LEN, 0, (struct sockaddr *) client->addr, &size_addr_in) == -1)
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
            if ((resend = process_message(set, client, buffer))) /* Create/send appropriate resp */
            {
                send_pack(set, client); /* Case: bad ACK seq num, retransmit. */
            }
        }
    } while (resend);
}

bool process_message(struct server_settings *set, struct conn_client *client, const uint8_t *data_buffer)
{
    printf("\nReceived packet:\n\tFlags: %s\n\tSequence Number: %d\n",
           check_flags(*data_buffer), *(data_buffer + 1));
    
    /* If wrong ACK, can save some time by not decoding. */
    if ((*data_buffer == FLAG_ACK) &&
        (*(data_buffer + 1) != client->s_packet->seq_num))
    {
        return true; /* Bad seq num: must resend previously sent packet. */
    }
    
    if (decode_message(set, client->r_packet, data_buffer) == -1)
    {
        running = 0;
        return false;
    }
    
    if ((client->r_packet->flags & FLAG_PSH) &&
        (client->r_packet->seq_num == (uint8_t) (client->s_packet->seq_num + 1))) // cracked cast
    {
        create_pack(client->s_packet, FLAG_ACK, client->r_packet->seq_num, 0, NULL);
        send_pack(set, client);
    
        if (client->r_packet->flags & FLAG_TRN && !set->game->validate(set->game))
        {
            return false;
        }
        
        // put the payload into Game struct
        set->game->updateBoard(set->game);
        
    } else if (client->r_packet->flags == FLAG_FIN)
    {
        create_pack(client->s_packet, FLAG_FIN | FLAG_ACK, MAX_SEQ, 0, NULL);
        send_pack(set, client);
    }
    
    return false;
}

void print_payload(struct server_settings *set, struct packet *r_packet)
{
//    printf("\n");
//    if (write(STDOUT_FILENO, r_packet->payload, r_packet->length) == -1)
//    {
//        printf("Could not write payload to output.");
//    }
//    printf("\n");

    // set the cursor in game
    set->game->cursor = r_packet->payload;
    
    set->game->displayBoardWithCursor(set->game);
    set->mm->mm_free(set->mm, r_packet->payload);
    r_packet->payload = NULL;
}

int decode_message(struct server_settings *set, struct packet *r_packet, const uint8_t *data_buffer)
{
    deserialize_packet(r_packet, data_buffer);
    if (errno == ENOMEM)
    {
        return -1;
    }
    set->mm->mm_add(set->mm, r_packet->payload);
    
    return 0;
}

void send_pack(struct server_settings *set, struct conn_client *client)
{
    uint8_t   *data_buffer = NULL;
    socklen_t size_addr_in;
    size_t    packet_size;
    
    if ((data_buffer = serialize_packet(client->s_packet)) == NULL)
    {
        running = 0;
        return;
    }
    set->mm->mm_add(set->mm, data_buffer);
    
    packet_size = sizeof(client->s_packet->flags) + sizeof(client->s_packet->seq_num)
                  + sizeof(client->s_packet->length) + client->s_packet->length;
    
    printf("\nSending packet:\n\tFlags: %s\n\tSequence Number: %d\n",
           check_flags(client->s_packet->flags), client->s_packet->seq_num);
    size_addr_in = sizeof(struct sockaddr_in);
    if (sendto(client->c_fd, data_buffer, packet_size, 0, (struct sockaddr *) client->addr, size_addr_in) == -1)
    {
        perror("\nMessage transmission to client failed: \n");
        return;
    }
    
    if (client->s_packet->flags == (FLAG_FIN | FLAG_ACK))
    {
        *data_buffer = FLAG_FIN;
        printf("\nSending packet:\n\tFlags: %s\n\tSequence Number: %d\n",
               check_flags(*data_buffer), client->s_packet->seq_num);
        if (sendto(client->c_fd, data_buffer, packet_size, 0, (struct sockaddr *) client->addr, size_addr_in) == -1)
        {
            perror("\nMessage transmission to client failed: \n");
            return;
        }
    }
    
    printf("\nSending to: %s:%u\n",
           inet_ntoa(client->addr->sin_addr), // NOLINT(concurrency-mt-unsafe) : no threads here
           ntohs(client->addr->sin_port));
    
    set->mm->mm_free(set->mm, data_buffer);
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
