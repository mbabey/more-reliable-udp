//
// Created by Maxwell Babey on 10/23/22.
//

#ifndef LIBS_SELF_IP_H
#define LIBS_SELF_IP_H

#include "structs.h"
#include <stdbool.h>

/**
 * The default port number for the proxy, client, and server.
 */
#define DEFAULT_PORT 5000

/**
 * The size of input buffers : 1 KB
 */
#define BUF_LEN 1024

/**
 * The maximum sequence number.
 */
#define MAX_SEQ 255

/**
 * Bit masks for flags. Bitwise OR these to make combinations of flags (eg: FIN/ACK = FLAG_FIN | FLAG_ACK)
 * When checking if a packet has certain flags, combine flags and check equality
 * (eg: (packet->flags == (FLAG_SYN | FLAG_ACK) is true if packet->flags is a SYN/ACK)
 */
#define FLAG_ACK 1 // 0000 0001
#define FLAG_PSH 2 // 0000 0010
#define FLAG_SYN 4 // 0000 0100
#define FLAG_FIN 8 // 0000 1000

/**
 * set_self_ip.
 * <p>
 * Get this host's IP address and attach it to the parameter char pointer pointer.
 * </p>
 * @param ip - char**: pointer to pointer to first char in IP address.
 */
void set_self_ip(char **ip);

/**
 * deserialize_packet
 * <p>
 * Load the bytes of a buffer into the received packet struct fields.
 * </p>
 * @param packet - the packet to store the buffer info
 * @param data_buffer - the buffer to deserialize
 */
void deserialize_packet(struct packet *packet, const uint8_t *data_buffer);

/**
 * serialize_packet
 * <p>
 * Load the  packet struct fields into the bytes of a buffer.
 * </p>
 * @param packet - the packet to serialize
 * @return the buffer storing the packet info
 */
uint8_t *serialize_packet(struct packet *packet);

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
 * WARNING: set_string dynamically allocates memory. Must free the pointer passed as the first parameter!
 * </h3>
 * </p>
 * @param str - char**: pointer to the string to be set
 * @param new_str - char*: the new value for the string
 */
void set_string(char **str, const char *new_str);

int do_open(const char *filename, int oflag);

ssize_t do_read(int fd, void *buf, size_t nbytes);

int do_close(const char *filename, int fd);

ssize_t do_lseek(int fd, off_t offset, int whence);

const char *check_flags(uint8_t flags);

#endif //LIBS_SELF_IP_H
