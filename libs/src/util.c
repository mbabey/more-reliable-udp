//
// Created by Maxwell Babey on 10/23/22.
//

#include "../../libs/include/manager.h"
#include "../include/error.h"
#include "../include/util.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
        res_ip = inet_ntoa(
                ((struct sockaddr_in *) res_next->ai_addr)->sin_addr); // NOLINT : No threads here, upcast intentional
        
        if (strcmp(res_ip, "127.0.0.1") != 0) // If the address is localhost, go to the next address.
        {
            *ip = res_ip;
        }
    }
    
    free(hostname);
    freeaddrinfo(results);
}

void deserialize_packet(struct packet *packet, const uint8_t *data_buffer)
{
    size_t bytes_copied;
    
    bytes_copied = 0;
    memcpy(&packet->flags, data_buffer + bytes_copied, sizeof(packet->flags));
    bytes_copied += sizeof(packet->flags);
    
    memcpy(&packet->seq_num, data_buffer + bytes_copied, sizeof(packet->seq_num));
    bytes_copied += sizeof(packet->seq_num);
    
    memcpy(&packet->length, data_buffer + bytes_copied, sizeof(packet->length));
    packet->length = ntohs(packet->length);
    bytes_copied += sizeof(packet->length);
    
    if (packet->length > 0)
    {
        if ((packet->payload = (uint8_t *) s_malloc(packet->length + 1,
                                                    __FILE__, __func__, __LINE__)) == NULL)
        {
            return;
        }
        memcpy(packet->payload, data_buffer + bytes_copied, packet->length);
        *(packet->payload + packet->length) = '\0';
    }
}

uint8_t *serialize_packet(struct packet *packet)
{
    uint8_t  *data_buffer;
    size_t   packet_size;
    size_t   bytes_copied;
    uint16_t n_packet_length;
    
    packet_size = sizeof(packet->flags) + sizeof(packet->seq_num) + sizeof(packet->length) + packet->length;
    if ((data_buffer = (uint8_t *) s_malloc(packet_size, __FILE__, __func__, __LINE__)) == NULL)
    {
        return NULL;
    }
    
    bytes_copied = 0;
    memcpy(data_buffer + bytes_copied, &packet->flags, sizeof(packet->flags));
    bytes_copied += sizeof(packet->flags);
    
    memcpy(data_buffer + bytes_copied, &packet->seq_num, sizeof(packet->seq_num));
    bytes_copied += sizeof(packet->seq_num);
    
    n_packet_length = htons(packet->length);
    memcpy(data_buffer + bytes_copied, &n_packet_length, sizeof(n_packet_length));
    bytes_copied += sizeof(n_packet_length);
    
    if (packet->length > 0)
    {
        memcpy(data_buffer + bytes_copied, packet->payload, packet->length);
    }
    
    return data_buffer;
}

void set_string(char **str, const char *new_str)
{
    size_t buf = strlen(new_str) + 1;
    
    if (!*str) // Double negative: true if the string is NULL
    {
        *str = (char *) s_malloc(buf, __FILE__, __func__, __LINE__);
    } else
    {
        *str = (char *) s_realloc(*str, buf, __FILE__, __func__, __LINE__);
    }
    
    strcpy(*str, new_str);
}

int do_open(const char *filename, int oflag)
{
    int fd;
    
    if ((fd = open(filename, oflag)) == -1)
    {
        const char *msg;
        
        msg = strerror(errno); // NOLINT(concurrency-mt-unsafe) : no threads here
        fprintf(stderr, "Error opening %s: %s\n\n", filename, msg); // NOLINT (cert-err33-c)
    }
    
    return fd;
}

ssize_t do_read(int fd, void *buf, size_t nbytes)
{
    ssize_t result;
    
    if ((result = read(fd, buf, nbytes)) == -1)
    {
        const char *msg;
        
        msg = strerror(errno); // NOLINT(concurrency-mt-unsafe) : no threads here
        fprintf(stderr, "Error reading %d: %s\n", fd, msg); // NOLINT (cert-err33-c)
    }
    
    return result;
}

void do_close(const char *filename, int fd)
{
    errno = 0;
    if (close(fd) == -1)
    {
        const char *msg;
        
        msg = strerror(errno); // NOLINT(concurrency-mt-unsafe) : no threads here
        fprintf(stderr, "Error closing %s: %s\n", filename, msg); // NOLINT (cert-err33-c)
    }
}

ssize_t do_lseek(int fd, off_t offset, int whence)
{
    ssize_t result;
    
    if ((result = lseek(fd, offset, whence)) == -1)
    {
        const char *msg;
        
        msg = strerror(errno); // NOLINT(concurrency-mt-unsafe) : no threads here
        fprintf(stderr, "Error seeking %d: %s\n", fd, msg); // NOLINT (cert-err33-c)
    }
    
    return result;
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
        case (FLAG_SYN | FLAG_ACK): // NOLINT(hicpp-signed-bitwise) : highest order bit unused
        {
            return "SYN/ACK";
        }
        case (FLAG_FIN | FLAG_ACK): // NOLINT(hicpp-signed-bitwise) : highest order bit unused
        {
            return "FIN/ACK";
        }
        default:
        {
            return "INVALID";
        }
    }
}
