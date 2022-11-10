//
// Created by Maxwell Babey on 11/9/22.
//

#include "../../libs/include/manager.h"
#include "../include/server-util.h"
#include "../include/setup.h"
#include "../../libs/include/error.h"
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

struct conn_client *create_conn_client(struct server_settings *set)
{
    struct conn_client *new_client;
    if ((new_client = s_calloc(1, sizeof(struct conn_client), __FILE__, __func__, __LINE__)) == NULL)
    {
        running = 0;
        return NULL;
    }
    
    if ((new_client->addr = s_calloc(1, sizeof(struct sockaddr_in), __FILE__, __func__, __LINE__)) == NULL)
    {
        free(new_client);
        running = 0;
        return NULL;
    }
    
    set->mm->mm_add(set->mm, new_client->addr); /* Add the new client to the memory manager. */
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

void create_pack(struct packet *packet, uint8_t flags, uint8_t seq_num, uint16_t len, uint8_t *payload)
{
    memset(packet, 0, sizeof(struct packet));
    
    packet->flags   = flags;
    packet->seq_num = seq_num;
    packet->length = len;
    packet->payload = payload;
}

void set_signal_handling(struct sigaction *sa)
{
    sigemptyset(&sa->sa_mask);
    sa->sa_flags   = 0;
    sa->sa_handler = signal_handler;
    if (sigaction(SIGINT, sa, 0) == -1)
    {
        fatal_errno(__FILE__, __func__, __LINE__, errno);
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

void signal_handler(int sig)
{
    running = 0;
}

#pragma GCC diagnostic pop
