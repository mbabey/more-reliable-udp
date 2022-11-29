//
// Created by Maxwell Babey on 11/17/22.
//

#include "../include/client-util.h"
#include "../include/manager.h"
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        return 0;
    }
    
    port = (in_port_t) sl;
    
    return port;
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
    
    packet_size = HLEN_BYTES + packet->length;
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
