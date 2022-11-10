//
// Created by Maxwell Babey on 10/24/22.
//

#include "../../libs/include/error.h"
#include "../../libs/include/manager.h"
#include "../../libs/include/util.h"
#include "../include/server.h"
#include "../include/server-util.h"
#include "../include/setup.h"
#include <arpa/inet.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * The number of connections which may be queued at once.
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
 * Await a SYN message from a connecting client. Respond with a SYN/ACK.
 * </p>
 * @param set - the server settings
 * @param send_packet - the packet struct to send
 * @param recv_packet - the packet struct to receive
 */
void do_accept(struct server_settings *set, struct packet *send_packet);

/**
 * do_messaging.
 * <p>
 * Await a message from the connected client. If no message is received and a timeout occurs,
 * resend the last-sent packet. If a timeout occurs too many times, drop the connection.
 * </p>
 * @param set - the server settings
 * @param send_packet - the packet struct to send
 * @param recv_packet - the packet struct to receive
 */
void do_messaging(struct server_settings *set, struct conn_client *client, struct packet *send_packet,
                  struct packet *recv_packet);

/**
 * process_message
 * <p>
 * Check the flags in the packet struct. Depending on the flags, respond accordingly.
 * </p>
 * @param set - the server settings
 * @param send_packet - the packet struct to send
 * @param recv_packet - the packet struct to receive
 */
void process_message(struct server_settings *set, struct conn_client *client, struct packet *send_packet,
                     struct packet *recv_packet);

/**
 * process_payload
 * <p>
 * Write the information in the payload to the output specified in the server settings.
 * </p>
 * @param set - the server settings
 * @param recv_packet - the packet struct to receive
 */
void process_payload(struct server_settings *set, struct conn_client *client, struct packet *recv_packet);

/**
 * send_resp
 * <p>
 * Send the send packet to the client as a response.
 * </p>
 * @param set - the server settings
 * @param send_packet - the packet struct to send
 */
void send_resp(struct server_settings *set, struct conn_client *client, struct packet *send_packet);

int setup_socket(char *ip, in_port_t port);

void run(int argc, char *argv[], struct server_settings *set)
{
    init_def_state(argc, argv, set);
    
    open_server(set);
    
    if (!errno) await_connect(set);
    
    // Return to main.
}

void open_server(struct server_settings *set)
{
    errno = 0;
    if ((set->server_fd = setup_socket(set->server_ip, set->server_port)) == -1)
    {
        return;
    }
    printf("\nServer running on %s:%d\n\n", set->server_ip, set->server_port);
}

int setup_socket(char *ip, in_port_t port)
{
    struct sockaddr_in addr;
    int                fd;
    
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) // NOLINT(android-cloexec-socket) : SOCK_CLOEXEC dne
    {
        fatal_errno(__FILE__, __func__, __LINE__, errno);
        return -1;
    }
    
    addr.sin_family           = AF_INET;
    addr.sin_port             = htons(port);
    if ((addr.sin_addr.s_addr = inet_addr(ip)) == (in_addr_t) -1)
    {
        fatal_errno(__FILE__, __func__, __LINE__, errno);
        return -1;
    }
    
    if (bind(fd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) == -1)
    {
        fatal_errno(__FILE__, __func__, __LINE__, errno);
        return -1;
    }
    return fd;
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
    struct packet      send_packet;
    struct packet      recv_packet;
    
    FD_ZERO(&set->readfds);
    FD_SET(set->server_fd, &set->readfds);
    set->max_fd = set->server_fd;
    
    /* Select the first MAX_CLIENTS connected clients. */
    curr_cli = set->first_conn_client;
    for (int num_selected_cli = 0; curr_cli != NULL && num_selected_cli < MAX_CLIENTS; ++num_selected_cli)
    {
        FD_SET(curr_cli->c_fd, &set->readfds); /* Add that client's fd to the set. */
        set->max_fd = (curr_cli->c_fd > set->max_fd) ? curr_cli->c_fd : set->max_fd; /* Set new max_fd if necessary. */
        
        curr_cli = curr_cli->next; /* Go to next client in list. */
    }
    
    if (select(set->max_fd, &set->readfds, NULL, NULL, NULL) == -1)
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
    
    if (FD_ISSET(set->server_fd, &set->readfds))
    {
        if (!errno) do_accept(set, &send_packet);
    } else
    {
        curr_cli = set->first_conn_client;
        for (int num_selected_cli = 0; curr_cli != NULL && num_selected_cli < MAX_CLIENTS; ++num_selected_cli)
        {
            if (FD_ISSET(curr_cli->c_fd, &set->readfds))
            {
                if (!errno) do_messaging(set, NULL, &send_packet, &recv_packet);
            }
            curr_cli = curr_cli->next; /* Go to next client in list. */
        }
    }
    
}

void do_accept(struct server_settings *set, struct packet *send_packet)
{
    printf("Awaiting connections.\n\n");
    
    socklen_t sockaddr_in_size;
    uint8_t   buffer[BUF_LEN];
    
    struct conn_client *new_client = alloc_conn_client(
            set); /* Allocate memory to store information about the new client. */
    
    send_packet->seq_num = MAX_SEQ;
    sockaddr_in_size = sizeof(struct sockaddr_in);
    do /* This loop will hang until a SYN packet is received. It is essentially accept. */
    {
        if ((recvfrom(set->server_fd, buffer, BUF_LEN, 0, /* Get client sockaddr_in here. */
                      (struct sockaddr *) new_client->addr, &sockaddr_in_size)) == -1)
        {
            switch (errno)
            {
                case EINTR:
                {
                    // running set to 0 with signal handler.
                    return;
                }
                case EWOULDBLOCK: // TODO: unset the timeout when a client disconnects.
                {
                    break;
                }
                default:
                {
                    fatal_errno(__FILE__, __func__, __LINE__, errno);
                    running = 0;
                    return;
                }
            }
        }
    } while (*buffer != FLAG_SYN);
    
    printf("Client connected from: %s\n\n",
           inet_ntoa(new_client->addr->sin_addr)); // NOLINT(concurrency-mt-unsafe) : no threads here
    
    create_pack(send_packet, FLAG_SYN | FLAG_ACK, MAX_SEQ, 0, NULL);
    send_resp(set, NULL, send_packet);
}

void do_messaging(struct server_settings *set, struct conn_client *client,
                  struct packet *send_packet, struct packet *recv_packet)
{
    uint8_t   buffer[BUF_LEN];
    socklen_t sockaddr_in_size;
    uint8_t   timeout_count;
    
    timeout_count    = 0;
    sockaddr_in_size = sizeof(struct sockaddr_in);
    do
    {
        printf("Awaiting message.\n\n");
        
        set->timeout->tv_sec = modify_timeout(timeout_count);
        errno = 0;
        if (setsockopt(client->c_fd, SOL_SOCKET, SO_RCVTIMEO,
                       (const char *) set->timeout, sizeof(struct timeval)) == -1)
        {
            fatal_errno(__FILE__, __func__, __LINE__, errno);
            running = 0;
            return;
        }
        
        memset(recv_packet, 0, sizeof(struct packet));
        memset(buffer, 0, BUF_LEN);
        
        if (recvfrom(client->c_fd, buffer, BUF_LEN, 0,
                     (struct sockaddr *) client->addr, &sockaddr_in_size) == -1)
        {
            switch (errno)
            {
                case EINTR:
                {
                    // running set to 0 with signal handler.
                    return;
                }
                case EWOULDBLOCK:
                {
                    printf("Timeout occurred. Timeouts remaining: %d\n\n", (MAX_TIMEOUTS_SERVER - (timeout_count + 1)));
                    ++timeout_count;
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
            timeout_count = 0;
            decode_message(set, recv_packet, buffer);
            if (!errno) process_message(set, client, send_packet, recv_packet);
        }
    } while (timeout_count < MAX_TIMEOUTS_SERVER && *buffer != (FLAG_FIN | FLAG_ACK));
    // TODO(maxwell): make the loop not go forever
}

void decode_message(struct server_settings *set, struct packet *recv_packet, const uint8_t *data_buffer)
{
    deserialize_packet(recv_packet, data_buffer);
    if (errno == ENOTRECOVERABLE)
    {
        running = 0;
        return;
    }
    set->mm->mm_add(set->mm, recv_packet->payload);
    
    printf("Received packet:\n\tFlags: %s\n\tSequence Number: %d\n\n",
           check_flags(recv_packet->flags), recv_packet->seq_num);
}

void process_message(struct server_settings *set, struct conn_client *client,
                     struct packet *send_packet, struct packet *recv_packet)
{
    if (recv_packet->flags == FLAG_ACK && !set->connected)
    {
        set->connected = true; // TODO: this is busted for multiple clients (maybe?). Each thread will need it's own connected (maybe?)
    }
    // FIN/ACK || end of 3-way do_synchronize || additional connect attempt
    if ((send_packet->flags == (FLAG_FIN | FLAG_ACK) ||
         recv_packet->flags == FLAG_ACK ||
         recv_packet->flags == FLAG_SYN) && set->connected)
    {
        return;
    }
    if ((recv_packet->flags == FLAG_PSH) &&
        (recv_packet->seq_num == (uint8_t) (send_packet->seq_num + 1))) // Why I have to cast this, C??
    {
        process_payload(set, NULL, recv_packet);
        create_pack(send_packet, FLAG_ACK, recv_packet->seq_num, 0, NULL);
    }
    if (recv_packet->flags == FLAG_FIN)
    {
        create_pack(send_packet, FLAG_FIN | FLAG_ACK, MAX_SEQ, 0, NULL);
    }
    
    send_resp(set, client, send_packet);
}

void process_payload(struct server_settings *set, struct conn_client *client, struct packet *recv_packet)
{
    printf("\n");
    if (write(client->c_fd, recv_packet->payload, recv_packet->length) == -1)
    {
        printf("Could not write payload to output.\n");
    }
    printf("\n");
    set->mm->mm_free(set->mm, recv_packet->payload);
    recv_packet->payload = NULL;
}

void send_resp(struct server_settings *set, struct conn_client *client, struct packet *send_packet)
{
    uint8_t   *data_buffer = NULL;
    socklen_t sockaddr_in_size;
    size_t    packet_size;
    
    if ((data_buffer = serialize_packet(send_packet)) == NULL)
    {
        running = 0;
        return;
    }
    set->mm->mm_add(set->mm, data_buffer);
    
    packet_size = sizeof(send_packet->flags) + sizeof(send_packet->seq_num)
                  + sizeof(send_packet->length) + send_packet->length;
    
    printf("Sending packet:\n\tFlags: %s\n\tSequence Number: %d\n\n",
           check_flags(send_packet->flags), send_packet->seq_num);
    
    sockaddr_in_size = sizeof(struct sockaddr_in);
    if (sendto(set->server_fd, data_buffer, packet_size, 0, (struct sockaddr *) client->addr, sockaddr_in_size) ==
        -1)
    {
        perror("Message transmission to client failed: ");
        return;
    }
    
    printf("Sending to: %s\n\n",
           inet_ntoa(client->addr->sin_addr)); // NOLINT(concurrency-mt-unsafe) : no threads here
    
    set->mm->mm_free(set->mm, data_buffer);
    
    // if from do_accept, next step is do_messaging. if from do_messaging, return to do_messaging.
}

void close_server(struct server_settings *set)
{
    printf("\nClosing server.\n");
    
    if (set->server_fd != 0)
    {
        close(set->server_fd);
    }
    free_mem_manager(set->mm);
}
