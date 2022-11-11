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
#define MAX_TIMEOUTS_SERVER 3

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
 * Receive a message. If it is a SYN, store the sender's information and create a new socket with which to
 * exchange messages with that sender. Send a SYN/ACK back to the sender on that socket.
 * </p>
 * @param set - the server settings
 * @param s_packet - the packet struct to send
 */
void do_accept(struct server_settings *set);

/**
 * do_message.
 * <p>
 * Await a message from the connected client. If no message is received and a timeout occurs,
 * resend the last-sent packet. If a timeout occurs too many times, drop the connection.
 * </p>
 * @param set - the server settings
 * @param s_packet - the packet struct to send
 * @param r_packet - the packet struct to receive
 */
void do_message(struct server_settings *set, struct conn_client *client);

/**
 * decode_message.
 * <p>
 * Load the bytes of the data buffer into the receive packet struct fields.
 * </p>
 * @param set - the server settings
 * @param data_buffer - the data buffer to load into the received packet
 * @param r_packet - the packet struct to receive
 */
void decode_message(struct server_settings *set, struct packet *r_packet, const uint8_t *data_buffer);

/**
 * process_message
 * <p>
 * Check the flags in the packet struct. Depending on the flags, respond accordingly.
 * </p>
 * @param set - the server settings
 * @param s_packet - the packet struct to send
 * @param r_packet - the packet struct to receive
 */
void process_message(struct server_settings *set, struct conn_client *client,
                     struct packet *s_packet, struct packet *r_packet);

/**
 * process_payload
 * <p>
 * Write the information in the payload to the output specified in the server settings.
 * </p>
 * @param set - the server settings
 * @param r_packet - the packet struct to receive
 */
void process_payload(struct server_settings *set, struct packet *r_packet);

/**
 * send_resp
 * <p>
 * Send the send packet to the client as a response.
 * </p>
 * @param set - the server settings
 * @param s_packet - the packet struct to send
 */
void send_resp(struct server_settings *set, struct conn_client *client, struct packet *s_packet);

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
    
    if (!errno) await_connect(set);
    
    /* return to main. */
}

void open_server(struct server_settings *set)
{
    if ((set->server_fd = setup_socket(set->server_ip, set->server_port)) == -1)
    {
        return;
    }
    printf("\nServer running on %s:%d\n\n", set->server_ip, set->server_port);
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
    
    if (FD_ISSET(set->server_fd, &readfds))
    {
        if (!errno) do_accept(set);
    } else
    {
        curr_cli = set->first_conn_client;
        for (int num_selected_cli = 0; curr_cli != NULL && num_selected_cli < MAX_CLIENTS; ++num_selected_cli)
        {
            if (FD_ISSET(curr_cli->c_fd, &readfds))
            {
                if (!errno) do_message(set, curr_cli);
            }
            curr_cli = curr_cli->next; /* Go to next client in list. */
        }
    }
}

void do_accept(struct server_settings *set)
{
    struct sockaddr_in from_addr;
    struct packet      packet;
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
    
    if (*buffer == FLAG_SYN) /* If the message received was a SYN packet. */
    {
        struct conn_client *new_client;
        
        /* Initialize storage for information about the new client. */
        if ((new_client = create_conn_client(set)) == NULL)
        {
            running = 0;
            return; // errno set
        }
        
        *new_client->addr = from_addr; /* Copy the sender's information into the client struct. */
        
        /* Create a new socket with an ephemeral port. */
        if ((new_client->c_fd = setup_socket(set->server_ip, 0)) == -1)
        {
            running = 0;
            return; // errno set
        }
        
        printf("Client connected from: %s:%u\n\n",
               inet_ntoa(new_client->addr->sin_addr), // NOLINT(concurrency-mt-unsafe) : no threads here
               ntohs(new_client->addr->sin_port));
        
        create_pack(&packet, FLAG_SYN | FLAG_ACK, MAX_SEQ, 0, NULL);
        if (!errno) send_resp(set, new_client, &packet);
    }
}

void do_message(struct server_settings *set, struct conn_client *client)
{
    struct packet      s_packet;
    struct packet      r_packet;
    uint8_t   buffer[BUF_LEN];
    socklen_t size_addr_in;
    
    memset(&r_packet, 0, sizeof(struct packet));
    memset(buffer, 0, BUF_LEN);
    
    size_addr_in = sizeof(struct sockaddr_in);
    
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
        decode_message(set, &r_packet, buffer);
        if (!errno) process_message(set, client, &s_packet, &r_packet); /* Create/send appropriate resp */
    }
}

void decode_message(struct server_settings *set, struct packet *r_packet, const uint8_t *data_buffer)
{
    deserialize_packet(r_packet, data_buffer);
    if (errno == ENOMEM)
    {
        running = 0;
        return;
    }
    set->mm->mm_add(set->mm, r_packet->payload);
    
    printf("Received packet:\n\tFlags: %s\n\tSequence Number: %d\n\n",
           check_flags(r_packet->flags), r_packet->seq_num);
}

void process_message(struct server_settings *set, struct conn_client *client,
                     struct packet *s_packet, struct packet *r_packet)
{
    if ((r_packet->flags == FLAG_PSH) && (r_packet->seq_num == (uint8_t) (s_packet->seq_num + 1))) // cracked cast
    {
        process_payload(set, r_packet);
        create_pack(s_packet, FLAG_ACK, r_packet->seq_num, 0, NULL);
        send_resp(set, client, s_packet);
    } else if (r_packet->flags == FLAG_FIN)
    {
        create_pack(s_packet, FLAG_FIN | FLAG_ACK, MAX_SEQ, 0, NULL);
        send_resp(set, client, s_packet);
    }
}

void process_payload(struct server_settings *set, struct packet *r_packet)
{
    printf("\n");
    if (write(STDOUT_FILENO, r_packet->payload, r_packet->length) == -1)
    {
        printf("Could not write payload to output.\n");
    }
    printf("\n");
    set->mm->mm_free(set->mm, r_packet->payload);
    r_packet->payload = NULL;
}

void send_resp(struct server_settings *set, struct conn_client *client, struct packet *s_packet)
{
    uint8_t   *data_buffer = NULL;
    socklen_t size_addr_in;
    size_t    packet_size;
    
    if ((data_buffer = serialize_packet(s_packet)) == NULL)
    {
        running = 0;
        return;
    }
    set->mm->mm_add(set->mm, data_buffer);
    
    packet_size = sizeof(s_packet->flags) + sizeof(s_packet->seq_num)
                  + sizeof(s_packet->length) + s_packet->length;
    
    printf("Sending packet:\n\tFlags: %s\n\tSequence Number: %d\n\n",
           check_flags(s_packet->flags), s_packet->seq_num);
    size_addr_in = sizeof(struct sockaddr_in);
    if (sendto(client->c_fd, data_buffer, packet_size, 0, (struct sockaddr *) client->addr, size_addr_in) == -1)
    {
        perror("Message transmission to client failed: ");
        return;
    }
    
    if (s_packet->flags == (FLAG_FIN | FLAG_ACK))
    {
        *data_buffer = FLAG_FIN;
        printf("Sending packet:\n\tFlags: %s\n\tSequence Number: %d\n\n",
               check_flags(*data_buffer), s_packet->seq_num);
        if (sendto(client->c_fd, data_buffer, packet_size, 0, (struct sockaddr *) client->addr, size_addr_in) == -1)
        {
            perror("Message transmission to client failed: ");
            return;
        }
    }
    
    printf("Sending to: %s:%u\n\n",
           inet_ntoa(client->addr->sin_addr),
           ntohs(client->addr->sin_port)); // NOLINT(concurrency-mt-unsafe) : no threads here
    
    set->mm->mm_free(set->mm, data_buffer);
    
    // if from do_accept, next step is do_messaging. if from do_messaging, return to do_message.
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
    free_mem_manager(set->mm);
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
