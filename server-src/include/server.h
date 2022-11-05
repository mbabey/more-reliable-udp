//
// Created by Maxwell Babey on 10/24/22.
//

#ifndef RELIABLE_UDP_SERVER_H
#define RELIABLE_UDP_SERVER_H

#include <sys/types.h>
#include <stdbool.h>

/**
 * server_settings
 * <p>
 * Stores global settings for the server.
 * <ul>
 * <li>server_ip: the server's ip address</li>
 * <li>server_port: the server's port number</li>
 * <li>server_fd: file descriptor of the socket listening for connections</li>
 * <li>accept_fd: file descriptor of the socket created when a connection is made</li>
 * <li>output_fd: file descriptor for the output</li>
 * </ul>
 * </p>
 */
struct server_settings
{
    char      *server_ip;
    in_port_t server_port;
    int       server_fd;
    int       output_fd;
    bool      connected;
    
    struct timeval        *timeout;
    struct sockaddr_in    *client_addr;
    struct memory_manager *mm;
};

/**
 * run.
 * <p>
 * Set up the server based on command line arguments, then open it.
 * </p>
 * @param argc - the number of command line arguments
 * @param argv - the command line arguments
 * @param set - the server settings
 */
void run(int argc, char *argv[], struct server_settings *set);

/**
 * close_server
 * <p>
 * Free allocated memory. Close the server socket.
 * </p>
 * @param set - the server settings
 * @param send_packet - the send packet
 * @param recv_packet - the receive packet
 */
void close_server(struct server_settings *set);

#endif //RELIABLE_UDP_SERVER_H
