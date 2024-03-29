//
// Created by Maxwell Babey on 10/24/22.
//

#ifndef RELIABLE_UDP_SERVER_H
#define RELIABLE_UDP_SERVER_H

#include "../include/server-util.h"
#include <stdbool.h>
#include <sys/types.h>

/**
 * run.
 * <p>
 * Set up the server based on command line arguments, then open for client connections.
 * </p>
 * @param argc - the number of command line arguments
 * @param argv - the command line arguments
 * @param set - the server settings
 */
void run(int argc, char *argv[], struct server_settings *set);

/**
 * close_server
 * <p>
 * Free allocated memory. Close open sockets.
 * </p>
 * @param set - the server settings
 */
void close_server(struct server_settings *set);

#endif //RELIABLE_UDP_SERVER_H
