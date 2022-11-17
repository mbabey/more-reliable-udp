//
// Created by chris on 2022-10-29.
//

#ifndef RELIABLE_UDP_CLIENT_H
#define RELIABLE_UDP_CLIENT_H

#include "manager.h"
#include <stdbool.h>
#include <sys/types.h>

/**
 * client_settings
 * <p>
 * Stores global settings for the client.
 * <ul>
 * <li>server_ip: the server's ip address</li>
 * <li>server_port: the server's port number</li>
 * <li>server_fd: file descriptor of the socket connected to the server</li>
 * <li>turn: whether it is this client's turn</li>
 * <li>server_addr: the address of the server connection</li>
 * <li>timeout: timeval used to determine time client will sv_recvfrom a message before acting</li>
 * <li>mm: a memory manager for the client</li>
 * <li>s_packet: the last-sent packet for this client</li>
 * <li>r_packet: the last-received packet for this client</li>
 * </ul>
 * </p>
 */
struct client_settings
{
    char      *server_ip;
    in_port_t server_port;
    int       server_fd;
    bool turn;
    
    struct sockaddr_in    *server_addr;
    struct timeval        *timeout;
    struct memory_manager *mm;
    
    struct packet *s_packet;
    struct packet *r_packet;
};

/**
 * run.
 * <p>
 * Set up the client based on command line arguments, then connect to the server.
 * </p>
 * @param argc - the number of command line arguments
 * @param argv - the command line arguments
 * @param set - the client set
 */
void run_client(int argc, char *argv[], struct client_settings *set);

/**
 * close_client
 * <p>
 * Close the client socket if open, and free all memory.
 * </p>
 * @param set - the client settings
 */
void close_client(struct client_settings *set);

#endif //RELIABLE_UDP_CLIENT_H
