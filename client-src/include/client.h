//
// Created by chris on 2022-10-29.
//

#ifndef RELIABLE_UDP_CLIENT_H
#define RELIABLE_UDP_CLIENT_H

#include "../../libs/include/manager.h"
#include <sys/types.h>

/**
 * client_settings
 * <p>
 * Stores global settings for the client.
 * <ul>
 * <li>server_ip: the server's ip address</li>
 * <li>server_port: the server's port number</li>
 * <li>server_fd: file descriptor of the socket connected to the server</li>
 * <li>is_file: if the client wishes to read a file to standard input</li>
 * </ul>
 * </p>
 */
struct client_settings
{
    char      *client_ip;
    char      *server_ip;
    int       server_fd;
    in_port_t server_port;
    
    struct sockaddr_in *client_addr;
    struct sockaddr_in *server_addr;
    
    struct timeval        *timeout;
    struct memory_manager *mem_manager;
};

/**
 * run.
 * <p>
 * Set up the client based on command line arguments, then connect to the server.
 * </p>
 * @param argc - the number of command line arguments
 * @param argv - the command line arguments
 * @param settings - the client settings
 */
void run_client(int argc, char *argv[], struct client_settings *settings);

/**
 * close_client
 * <p>
 * Close the client socket if open, and free all memory.
 * </p>
 * @param set - the client settings
 */
void close_client(struct client_settings *set);

#endif //RELIABLE_UDP_CLIENT_H
