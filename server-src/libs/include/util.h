//
// Created by Maxwell Babey on 10/23/22.
//

#ifndef LIBS_SELF_IP_H
#define LIBS_SELF_IP_H

#include <stdbool.h>
#include <sys/types.h>

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
#define PKT_STD_BYTES 4

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

#endif //LIBS_SELF_IP_H
