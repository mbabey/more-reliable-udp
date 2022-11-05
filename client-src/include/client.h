//
// Created by chris on 2022-10-29.
//

#ifndef RELIABLE_UDP_CLIENT_H
#define RELIABLE_UDP_CLIENT_H

#include "../../libs/include/structs.h"

/**
 * run.
 * <p>
 * Set up the client based on command line arguments, then connect to the server.
 * </p>
 * @param argc - the number of command line arguments
 * @param argv - the command line arguments
 * @param settings - the client settings
 */
void run_client(int argc, char *argv[], struct client_settings *settings);

/**
 * close_client
 * <p>
 * Close the client socket if open, and free all memory.
 * </p>
 * @param set - the client settings
 */
void close_client(struct client_settings *set);

#endif //RELIABLE_UDP_CLIENT_H
