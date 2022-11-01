//
// Created by Maxwell Babey on 10/23/22.
//

#ifndef LIBS_INPUT_VALIDATION_H
#define LIBS_INPUT_VALIDATION_H

#include <sys/types.h>
#include <netinet/in.h>

/**
 * check_ip
 * <p>
 * Check the user input IP address to ensure it is within parameters. Namely, that none of its
 * period separated numbers are larger than 255, and that the address is in the form
 * XXX.XXX.XXX.XXX
 * </p>
 * @param ip - char *: the string containing the IP address
 * @param base - int: base in which to interpret the IP address
 */
void check_ip(char *ip, uint8_t base);

/**
 * parse_port
 * <p>
 * Check the user input port number to ensure it is within parameters. Namely, that it is not
 * larger than 65535.
 * </p>
 * @param buffer - char *: string containing the port number
 * @param base - int: base in which to interpret the port number
 * @return the port number, an in_port_t
 * @author D'Arcy Smith
 */
in_port_t parse_port(const char *buffer, uint8_t base);

#endif //LIBS_INPUT_VALIDATION_H
