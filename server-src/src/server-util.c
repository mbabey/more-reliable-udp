//
// Created by Maxwell Babey on 11/9/22.
//

#include "../../libs/include/error.h"
#include "../../libs/include/manager.h"
#include "../include/server-util.h"
#include "../include/setup.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

int open_server_socket(char *ip, in_port_t port)
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

int set_readfds(struct server_settings *set, fd_set *readfds)
{
    struct conn_client *curr_cli;
    int                max_fd;
    
    FD_ZERO(readfds); /* Clean the readfds. */
    FD_SET(set->server_fd, readfds);
    max_fd = set->server_fd;
    
    /* Select the first MAX_CLIENTS connected clients. */
    curr_cli = set->first_conn_client;
    for (int num_selected_cli = 0; curr_cli != NULL && num_selected_cli < MAX_CLIENTS; ++num_selected_cli)
    {
        FD_SET(curr_cli->c_fd, readfds); /* Add that client's fd to the set. */
        
        max_fd = (curr_cli->c_fd > max_fd) ? curr_cli->c_fd : max_fd; /* Set new max_fd if necessary. */
        
        curr_cli = curr_cli->next; /* Go to next client in list. */
    }
    
    return max_fd;
}

struct conn_client *connect_client(struct server_settings *set, struct sockaddr_in *from_addr)
{
    struct conn_client *new_client;
    
    /* Initialize storage for information about the new client. */
    if ((new_client = create_conn_client(set)) ==
        NULL) // TODO(maxwell): the client is never removed from the linked list.
    {
        return NULL; // errno set
    }
    *new_client->addr = *from_addr; /* Copy the sender's information into the client struct. */
    
    /* Create a new socket with an ephemeral port. */
    if ((new_client->c_fd = open_server_socket(set->server_ip, 0)) == -1)
    {
        return NULL; // errno set
    }
    
    ++set->num_conn_client; /* Increment the number of connected clients. */
    
    return new_client;
}

struct conn_client *create_conn_client(struct server_settings *set)
{
    struct conn_client *new_client;
    if ((new_client = s_calloc(1, sizeof(struct conn_client), __FILE__, __func__, __LINE__)) == NULL)
    {
        return NULL;
    }
    if ((new_client->addr     = s_calloc(1, sizeof(struct sockaddr_in), __FILE__, __func__, __LINE__)) == NULL)
    {
        free(new_client);
        return NULL;
    }
    if ((new_client->s_packet = s_calloc(1, sizeof(struct packet), __FILE__, __func__, __LINE__)) == NULL)
    {
        free(new_client->addr);
        free(new_client);
        return NULL;
    }
    if ((new_client->r_packet = s_calloc(1, sizeof(struct packet), __FILE__, __func__, __LINE__)) == NULL)
    {
        free(new_client->addr);
        free(new_client->s_packet);
        free(new_client);
        return NULL;
    }
    
    set->mm->mm_add(set->mm, new_client->r_packet); /* Add the members of new client first that they are freed first. */
    set->mm->mm_add(set->mm, new_client->s_packet);
    set->mm->mm_add(set->mm, new_client->addr);
    set->mm->mm_add(set->mm, new_client);
    
    if (set->first_conn_client == NULL) /* Add the new client to the back of the connected client list. */
    {
        set->first_conn_client = new_client;
    } else
    {
        struct conn_client *curr_node;
        
        curr_node = set->first_conn_client;
        while (curr_node->next != NULL)
        {
            curr_node = curr_node->next;
        }
        curr_node->next = new_client;
    }
    
    return new_client;
}

uint8_t modify_timeout(uint8_t timeout_count)
{
    switch (timeout_count)
    {
        case 0:
        {
            return SERVER_TIMEOUT_SHORT;
        }
        case 1:
        {
            return SERVER_TIMEOUT_MED;
        }
        case 2:
        {
            return SERVER_TIMEOUT_LONG;
        }
        default:
        {
            return SERVER_TIMEOUT_SHORT;
        }
    }
}
