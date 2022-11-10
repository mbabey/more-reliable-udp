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
    struct conn_client    *first_conn_client;
    struct memory_manager *mm;
};

struct conn_client
{
    int c_fd;
    struct sockaddr_in *addr;
    
    struct conn_client *next;
};

/**
 * alloc_conn_client
 * <p>
 * Allocate memory for and return a pointer to a new connected client node. Add the new client to the
 * server settings linked list of connected clients and add the memory to the memory manager.
 * </p>
 * @return a pointer to the newly allocated connected client struct.
 */
struct conn_client *alloc_conn_client(struct server_settings *set);

/**
 * modify_timeout
 * <p>
 * Change the timeout duration based on the number of timeouts that have occured.
 * </p>
 * @return
 */
uint8_t modify_timeout(uint8_t timeout_count);

/**
 * decode_message.
 * <p>
 * Load the bytes of the data buffer into the receive packet struct fields.
 * </p>
 * @param set - the server settings
 * @param send_packet - the packet struct to send
 * @param recv_packet - the packet struct to receive
 */
void decode_message(struct server_settings *set, struct packet *recv_packet, const uint8_t *data_buffer);

/**
 * create_pack
 * <p>
 * Zero the send packet. Set the flags and the sequence number.
 * </p>
 * @param flags - the flags to set in the send packet
 * @param seq_num - the sequence number to set in the send packet
 * @param send_packet - the packet struct to send
 */
void create_pack(struct packet *send_packet, uint8_t flags, uint8_t seq_num, uint16_t len, uint8_t *payload);

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

#endif //RELIABLE_UDP_SERVER_UTIL_HPP
