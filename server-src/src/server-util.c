//
// Created by Maxwell Babey on 11/9/22.
//

#include "../include/manager.h"
#include "../include/server-util.h"
#include "../include/setup.h"
#include <arpa/inet.h>
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *check_ip(char *ip, uint8_t base)
{
    const char *msg     = NULL;
    char       *ip_cpy  = NULL;
    char       *end     = NULL;
    char       *tok     = NULL;
    char       delim[2] = ".";
    int        tok_count;
    long       num;
    
    set_string(&ip_cpy, ip);
    if (errno == ENOMEM)
    {
        fatal_errno(__FILE__, __func__, __LINE__, errno);
        free(ip_cpy);
        return NULL;
    }
    
    /* Tokenize the IP address by byte. */
    tok = strtok(ip_cpy, delim); // NOLINT(concurrency-mt-unsafe) : No threads here
    
    tok_count = 0;
    while (tok != NULL)
    {
        ++tok_count;
        
        num = strtol(tok, &end, base);
        
        if (end == tok)
        {
            msg = "IP address must be a decimal number";
        } else if (*end != '\0')
        {
            msg = "IP address input must not have extra characters appended";
        } else if ((num < 0 || num > UINT8_MAX) ||
                   ((LONG_MIN == num || LONG_MAX == num) && ERANGE == errno))
        {
            msg = "IP address unit must be between 0 and 255";
        }
        
        if (msg)
        {
            advise_usage(msg);
            free(ip_cpy);
            return NULL;
        }
        
        tok = strtok(NULL, delim); // NOLINT(concurrency-mt-unsafe) : No threads here
    }
    if (tok_count != 4)
    {
        msg = "IP address must be in form XXX.XXX.XXX.XXX";
        advise_usage(msg);
        free(ip_cpy);
        return NULL;
    }
    free(ip_cpy);
    
    return ip;
}

in_port_t parse_port(const char *buffer, uint8_t base)
{
    const char *msg = NULL;
    char       *end;
    long       sl;
    in_port_t  port;
    
    sl = strtol(buffer, &end, base);
    
    if (end == buffer)
    {
        msg = "Port number must be a decimal number";
    } else if (*end != '\0')
    {
        msg = "Port number input must not have extra characters appended";
    } else if ((sl < 0 || sl > UINT16_MAX) ||
               ((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno))
    {
        msg = "Port number must be between 0 and 65535";
    }
    
    if (msg)
    {
        advise_usage(msg);
    }
    
    port = (in_port_t) sl;
    
    return port;
}

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


void set_self_ip(char **ip)
{
    struct addrinfo hints;
    struct addrinfo *results;
    struct addrinfo *res_next;
    char            *res_ip;
    char            *hostname;
    const size_t    hostname_buf = 128;
    
    hostname = (char *) s_malloc(hostname_buf, __FILE__, __func__, __LINE__);
    
    gethostname(hostname, hostname_buf);
    
    memset(&hints, 0,
           sizeof(struct addrinfo)); // NOLINT(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling) : Not a POSIX function
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;
    
    if (getaddrinfo(hostname, NULL, &hints, &results) != 0)
    {
        // Result: user will be informed they must enter their IP address manually.
        free(hostname);
        return;
    }
    
    for (res_next = results; res_next != NULL; res_next = res_next->ai_next)
    {
        res_ip = inet_ntoa( // NOLINT : No threads here, upcast intentional
                ((struct sockaddr_in *) res_next->ai_addr)->sin_addr);
        
        if (strcmp(res_ip, "127.0.0.1") != 0) // If the address is localhost, go to the next address.
        {
            *ip = res_ip;
        }
    }
    
    free(hostname);
    freeaddrinfo(results);
}

void deserialize_packet(struct packet *packet, const uint8_t *buffer)
{
    size_t bytes_copied;
    
    bytes_copied = 0;
    memcpy(&packet->flags, buffer + bytes_copied, sizeof(packet->flags));
    bytes_copied += sizeof(packet->flags);
    
    memcpy(&packet->seq_num, buffer + bytes_copied, sizeof(packet->seq_num));
    bytes_copied += sizeof(packet->seq_num);
    
    memcpy(&packet->length, buffer + bytes_copied, sizeof(packet->length));
    packet->length = ntohs(packet->length);
    bytes_copied += sizeof(packet->length);
    
    if (packet->length > 0)
    {
        if ((packet->payload = (uint8_t *) s_malloc(packet->length + 1,
                                                    __FILE__, __func__, __LINE__)) == NULL)
        {
            return;
        }
        memcpy(packet->payload, buffer + bytes_copied, packet->length);
        *(packet->payload + packet->length) = '\0';
    }
}

uint8_t *serialize_packet(struct packet *packet)
{
    uint8_t  *buffer;
    size_t   packet_size;
    size_t   bytes_copied;
    uint16_t n_packet_length;
    
    packet_size = PKT_STD_BYTES + packet->length;
    if ((buffer = (uint8_t *) s_malloc(packet_size, __FILE__, __func__, __LINE__)) == NULL)
    {
        return NULL;
    }
    
    bytes_copied = 0;
    memcpy(buffer + bytes_copied, &packet->flags, sizeof(packet->flags));
    bytes_copied += sizeof(packet->flags);
    
    memcpy(buffer + bytes_copied, &packet->seq_num, sizeof(packet->seq_num));
    bytes_copied += sizeof(packet->seq_num);
    
    n_packet_length = htons(packet->length);
    memcpy(buffer + bytes_copied, &n_packet_length, sizeof(n_packet_length));
    bytes_copied += sizeof(n_packet_length);
    
    if (packet->length > 0)
    {
        memcpy(buffer + bytes_copied, packet->payload, packet->length);
    }
    
    return buffer;
}

void set_string(char **str, const char *new_str)
{
    size_t buf = strlen(new_str) + 1;
    
    
    if (!*str) // Double negative: true if the string is NULL
    {
        if ((*str = (char *) malloc(buf)) == NULL)
        {
            return;
        }
    } else
    {
        if ((*str = (char *) realloc(*str, buf)) == NULL)
        {
            return;
        }
    }
    
    strcpy(*str, new_str);
}

void create_packet(struct packet *packet, uint8_t flags, uint8_t seq_num, uint16_t len, uint8_t *payload)
{
    memset(packet, 0, sizeof(struct packet));
    
    packet->flags   = flags;
    packet->seq_num = seq_num;
    packet->length  = len;
    packet->payload = payload;
}

const char *check_flags(uint8_t flags)
{
    switch (flags)
    {
        case FLAG_ACK:
        {
            return "ACK";
        }
        case FLAG_PSH:
        {
            return "PSH";
        }
        case FLAG_SYN:
        {
            return "SYN";
        }
        case FLAG_FIN:
        {
            return "FIN";
        }
        case (FLAG_PSH | FLAG_TRN):
        {
            return "PSH/TRN";
        }
        case (FLAG_SYN | FLAG_ACK):
        {
            return "SYN/ACK";
        }
        case (FLAG_FIN | FLAG_ACK):
        {
            return "FIN/ACK";
        }
        default:
        {
            return "INVALID";
        }
    }
}

void fatal_errno(const char *file, const char *func, const size_t line, int err_code) // NOLINT(bugprone-easily-swappable-parameters)
{
    const char *msg;
    
    msg = strerror(err_code);                                                                  // NOLINT(concurrency-mt-unsafe)
    fprintf(stderr, "Error (%s @ %s:%zu %d) - %s\n", file, func, line, err_code, msg);  // NOLINT(cert-err33-c)
    errno = ENOTRECOVERABLE;                                                                           // NOLINT(concurrency-mt-unsafe)
}

void advise_usage(const char *usage_message)
{
    fprintf(stderr, "Usage: %s\n", usage_message); // NOLINT(cert-err33-c) : val not needed
    errno = ENOTRECOVERABLE;
}
