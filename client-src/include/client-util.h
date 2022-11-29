//
// Created by Maxwell Babey on 11/17/22.
//

#ifndef RELIABLE_UDP_CLIENT_UTIL_H
#define RELIABLE_UDP_CLIENT_UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/types.h>
#include <netinet/in.h>

/**
 * The default port number for the proxy, client, and server.
 */
#define DEFAULT_PORT (in_port_t) 5000

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
#define HLEN_BYTES 4

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
 * check_ip
 * <p>
 * Check the user input IP address to ensure it is within parameters. Namely, that none of its
 * period separated numbers are larger than 255, and that the address is in the form
 * XXX.XXX.XXX.XXX
 * </p>
 * @param ip - the string containing the IP address
 * @param base - base in which to interpret the IP address
 * @return the valid IP address
 */
char * check_ip(char *ip, uint8_t base);

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
 * @param str - pointer to the string to be set
 * @param new_str - the new value for the string
 */
void set_string(char **str, const char *new_str);

/**
 * parse_port
 * <p>
 * Check the user input port number to ensure it is within parameters. Namely, that it is not
 * larger than 65535.
 * </p>
 * @param buffer - string containing the port number
 * @param base - base in which to interpret the port number
 * @return the port number, an in_port_t
 * @author D'Arcy Smith
 */
in_port_t parse_port(const char *buffer, uint8_t base);

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
void fatal_errno(const char *file, const char *func, size_t line, int err_code);

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


#endif //RELIABLE_UDP_CLIENT_UTIL_H
