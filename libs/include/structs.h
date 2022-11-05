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

#endif //RELIABLE_UDP_STRUCTS_H
