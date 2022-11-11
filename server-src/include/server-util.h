//
// Created by Maxwell Babey on 11/9/22.
//

#ifndef RELIABLE_UDP_SERVER_UTIL_HPP
#define RELIABLE_UDP_SERVER_UTIL_HPP

#include "../../libs/include/util.h"
#include <signal.h>
#include <stdbool.h>
#include <sys/types.h>

/**
 * The maximum number of clients that can communicate with the server at once.
 */
#define MAX_CLIENTS 2

/**
 * While set to > 0, the program will continue running. Will be set to 0 by SIGINT or a catastrophic failure.
 */
static volatile sig_atomic_t running; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables) : var must change

/**
 * server_settings
 * <p>
 * Stores global settings for the server.
 * <ul>
 * <li>server_ip: the server's ip address</li>
 * <li>server_port: the server's port number</li>
 * <li>server_fd: file descriptor of the socket listening for connections</li>
 * <li>max_fd: the maximum value of the file descriptor</li>
 * <li>output_fd: file descriptor for the output</li>
 * </ul>
 * </p>
 */
struct server_settings
{
    char      *server_ip;
    in_port_t server_port;
    int       server_fd;
    
    struct timeval        *timeout;
    struct conn_client    *first_conn_client; /* Linked list holding fds, IPs, and port nums for connected clients. */
    struct memory_manager *mm;
};

struct conn_client
{
    int                c_fd;
    struct sockaddr_in *addr;
    
    struct conn_client *next;
};

/**
 * create_conn_client
 * <p>
 * Allocate memory for and return a pointer to a new connected client node. Add the new client to the
 * server settings linked list of connected clients and add the memory to the memory manager.
 * </p>
 * @return a pointer to the newly allocated connected client struct.
 */
struct conn_client *create_conn_client(struct server_settings *set);

/**
 * setup_socket
 * <p>
 * Create and bind a new socket to the given IP address and port number. Passing 0 as the port will bind
 * the socket to an ephemeral port (random port number).
 * </p>
 * @param ip - the IP address to which the new socket shall be bound
 * @param port - the port number to which the new socket shall be bound
 * @return the file descriptor to the new socket
 */
int setup_socket(char *ip, in_port_t port);

/**
 * set_readfds
 * <p>
 * Set the readfds set for use in the select function: clear the set, then add the server's file descriptor followed by
 * up to MAX_CLIENTS client file descriptors. Keep track of the file descriptor with the greatest value.
 * </p>
 * @param set - the server settings
 * @param readfds - the file descriptor set to be monitored for read activity
 * @return the file descriptor with the greatest value
 */
int set_readfds(struct server_settings *set, fd_set *readfds);

/**
 * modify_timeout
 * <p>
 * Change the timeout duration based on the number of timeouts that have occurred.
 * </p>
 * @return
 */
uint8_t modify_timeout(uint8_t timeout_count);

/**
 * create_pack
 * <p>
 * Zero a struct packet. Set the flags, sequence number, length, and payload.
 * </p>
 * @param packet - the packet struct to initialize
 * @param flags - the flags to set in the send packet
 * @param seq_num - the sequence number to set in the send packet
 * @param len - the length of the payload
 * @param payload - the payload
 */
void create_pack(struct packet *packet, uint8_t flags, uint8_t seq_num, uint16_t len, uint8_t *payload);

#endif //RELIABLE_UDP_SERVER_UTIL_HPP
