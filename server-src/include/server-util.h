//
// Created by Maxwell Babey on 11/9/22.
//

#ifndef RELIABLE_UDP_SERVER_UTIL_HPP
#define RELIABLE_UDP_SERVER_UTIL_HPP

#include "../include/server-util.h"
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>

/**
 * The default port number for the proxy, client, and server.
 */
#define DEFAULT_PORT (in_port_t) 5000

/**
 * The size of input buffers : 1 KB
 */
#define BUF_LEN 1024

/**
 * The maximum sequence number.
 */
#define MAX_SEQ (uint8_t) 255

/**
 * Bit masks for flags. Bitwise OR these to make combinations of flags (eg: FIN/ACK = FLAG_FIN | FLAG_ACK)
 * When checking if a packet has certain flags, combine flags and check equality
 * (eg: (packet->flags == (FLAG_SYN | FLAG_ACK) is true if packet->flags is a SYN/ACK)
 */
#define FLAG_ACK (uint8_t) 1  // 0000 0001
#define FLAG_PSH (uint8_t) 2  // 0000 0010
#define FLAG_SYN (uint8_t) 4  // 0000 0100
#define FLAG_FIN (uint8_t) 8  // 0000 1000
#define FLAG_TRN (uint8_t) 16 // 0001 0000

/**
 * The number of bytes of a packet before the payload is attached.
 */
#define STD_PKT_BYTES 4

/**
 * The maximum number of clients that can communicate with the server at once.
 */
#define MAX_CLIENTS 2

/**
 * The number of connections which may be queued at once. NOTE: server does not currently time out.
 */
//#define MAX_TIMEOUTS_SERVER 3

/**
 * packet
 * <p>
 * Stores packet information.
 * <ul>
 * <li>flags: the flags set for the packet</li>
 * <li>seq_num: the sequence number of the packet</li>
 * <li>length: the number of bytes in the packet following these two bytes</li>
 * <li>payload: the byte data of the packet</li>
 * </ul>
 * </p>
 */
struct packet
{
    uint8_t  flags;
    uint8_t  seq_num;
    uint16_t length;
    
    uint8_t *payload; // 'payload' is a cooler word than 'data'
};

/**
 * server_settings
 * <p>
 * Stores global settings for the server.
 * <ul>
 * <li>server_ip: the server's ip address</li>
 * <li>server_port: the server's port number</li>
 * <li>server_fd: file descriptor of the socket listening for connections</li>
 * <li>first_conn_client: Head of linked list holding communication information of connected clients</li>
 * <li>timeout: timeval used to determine time server will sv_recvfrom a message before acting</li>
 * <li>mm: a memory manager for the server</li>
 * </ul>
 * </p>
 */
struct server_settings
{
    char      *server_ip;
    in_port_t server_port;
    int       server_fd;
    uint8_t   num_conn_client;
    
    struct conn_client    *first_conn_client;
    struct timeval        *timeout;
    struct memory_manager *mm;
    struct Game           *game;
};

/**
 * conn_client
 * <p>
 * Represents an individual client connected to the server. The server uses this struct to keep track of the connected
 * client's socket file descriptor, address information, and their last sent and received packets.
 * <p>
 */
struct conn_client
{
    int                c_fd;
    struct sockaddr_in *addr;
    struct packet      *s_packet;
    struct packet      *r_packet;
    
    struct conn_client *next;
};


/**
 * check_ip
 * <p>
 * Check the user input IP address to ensure it is within parameters. Namely, that none of its
 * period separated numbers are larger than 255, and that the address is in the form
 * XXX.XXX.XXX.XXX
 * </p>
 * @param ip - char *: the string containing the IP address
 * @param base - int: base in which to interpret the IP address
 */
char *check_ip(char *ip, uint8_t base);

/**
 * set_string
 * <p>
 * Set a string to a value. If the string is NULL, will call malloc. If the string is not NULL, will call
 * realloc.
 * </p>
 * <p>
 * NOTE: Requires a dynamically allocated or NULL first parameter.
 * </p>
 * <p>
 * <h3>
 * WARNING: set_string dynamically allocates memory. Must free the pointer passed as the first parameter
 * </h3>
 * </p>
 * @param str - char**: pointer to the string to be set
 * @param new_str - char*: the new value for the string
 */
void set_string(char **str, const char *new_str);

/**
 * parse_port
 * <p>
 * Check the user input port number to ensure it is within parameters. Namely, that it is not
 * larger than 65535.
 * </p>
 * @param buffer - char *: string containing the port number
 * @param base - int: base in which to interpret the port number
 * @return the port number, an in_port_t
 * @author D'Arcy Smith
 */
in_port_t parse_port(const char *buffer, uint8_t base);

/**
 * set_self_ip.
 * <p>
 * Get this host's IP address and attach it to the parameter char pointer pointer.
 * </p>
 * @param ip - char**: pointer to pointer to first char in IP address.
 */
void set_self_ip(char **ip);

/**
 * open_server_socket
 * <p>
 * Create and bind a new socket to the given IP address and port number. Passing 0 as the port will bind
 * the socket to an ephemeral port (random port number).
 * </p>
 * @param ip - the IP address to which the new socket shall be bound
 * @param port - the port number to which the new socket shall be bound
 * @return the file descriptor to the new socket
 */
int open_server_socket(char *ip, in_port_t port);

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
 * connect_client
 * <p>
 * Connect a new client. Store the client's information and create a new socket with which to
 * exchange messages with that client. Increment the number of connected clients.
 * </p>
 * @param set - the client settings
 * @param from_addr - the address from which the message was sent
 * @return - a pointer to the newly connected client.
 */
struct conn_client *connect_client(struct server_settings *set, struct sockaddr_in *from_addr);

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
 * disconnect_client
 * <p>
 * Remove a client from the connected client list. Free the client's memory.
 * </p>
 * @param set - the server settings
 * @param client - the disconnecting client
 */
void disconnect_client(struct server_settings *set, struct conn_client *client);

/**
 * delete_conn_client
 * <p>
 * Free the memory associated with a client. Decrement the number of connected clients counter.
 * </p>
 * @param set - the server settings
 * @param client - the client to free
 */
void delete_conn_client(struct server_settings *set, struct conn_client *client);

/**
 * deserialize_packet
 * <p>
 * Load the bytes of a buffer into the received packet struct fields.
 * </p>
 * @param packet - the packet to store the buffer info
 * @param buffer - the buffer to deserialize
 */
void deserialize_packet(struct packet *packet, const uint8_t *buffer);

/**
 * serialize_packet
 * <p>
 * Load the packet struct fields into the bytes of a buffer.
 * </p>
 * @param packet - the packet to serialize
 * @return the buffer storing the packet info
 */
uint8_t *serialize_packet(struct packet *packet);

/**
 * create_packet
 * <p>
 * Zero a struct packet. Set the flags, sequence number, length, and payload.
 * </p>
 * @param packet - the packet struct to initialize
 * @param flags - the flags to set in the send packet
 * @param seq_num - the sequence number to set in the send packet
 * @param len - the length of the payload
 * @param payload - the payload
 */
void create_packet(struct packet *packet, uint8_t flags, uint8_t seq_num, uint16_t len, uint8_t *payload);

/**
 * check_flags
 * <p>
 * Check the input flags; return a string representation.
 * </p>
 * @param flags - the flags
 * @return a string representation of the flags
 */
const char *check_flags(uint8_t flags);

/**
 * modify_timeout
 * <p>
 * Change the timeout duration based on the number of timeouts that have occurred.
 * </p>
 * @return
 */
uint8_t modify_timeout(uint8_t timeout_count);

/**
 * fatal_errno.
 * <p>
 * For calling when a significant unrecoverable error has occurred. Prints an error message.
 * Following a call to this function, program memory should be cleaned up and the program should terminate.
 * Sets errno to ENOTRECOVERABLE.
 * </p>
 * @param file - file in which error occurred
 * @param func - function in which error occurred
 * @param line - line on which error occurred
 * @param err_code - the error code of the error which occurred
 */
void fatal_errno(const char *file, const char *func, const size_t line, int err_code);

/**
 * advise_usage.
 * <p>
 * For calling when a user enters the program command incorrectly. Prints
 * a message that advises the user on how to enter the program command
 * properly. Sets errno to ENOTRECOVERABLE.
 * </p>
 * @param usage_message - the advice message
 */
void advise_usage(const char *usage_message);


#endif //RELIABLE_UDP_SERVER_UTIL_HPP
