//
// Created by Maxwell Babey on 10/24/22.
//

/**
 * structs.h
 * <p>
 * Contains the data structures for reliable-udp.
 * </p>
 */
#ifndef RELIABLE_UDP_STRUCTS_H
#define RELIABLE_UDP_STRUCTS_H

#include "../../libs/include/manager.h"
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
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
    struct memory_manager *mem_manager;
};

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
    bool is_file;
    
    struct sockaddr_in *addr;
    struct sockaddr_in to_addr;
    
    struct timeval        *timeout;
    struct memory_manager *mem_manager;
};

/**
 * server_settings
 * <p>
 * Stores global settings for the server.
 * <ul>
 * <li>input_ip: the input connection ip address</li>
 * <li>output_ip: the output connection ip address</li>
 * <li>input_port: the input connection port number</li>
 * <li>output_port: the output connection port number</li>
 * <li>server_fd: file descriptor of the socket listening for connections</li>
 * <li>accept_fd: file descriptor of the socket created when a connection is made</li>
 * <li>output_fd: file descriptor for the output</li>
 * </ul>
 * </p>
 */
struct proxy_settings
{
    char      *proxy_ip;
    char      *output_ip;
    in_port_t proxy_port;
    in_port_t output_port;
    int       proxy_fd;
    uint8_t   drop_bound;
    uint8_t   hold_bound;
    
    struct sockaddr_in    *from_addr;
    struct sockaddr_in    *output_addr;
    struct sockaddr_in    *input_addr;
    struct memory_manager *mem_manager;
};

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

#endif //RELIABLE_UDP_STRUCTS_H
