//
// Created by chris on 2022-10-29.
//

#ifndef RELIABLE_UDP_SETUP_H
#define RELIABLE_UDP_SETUP_H

#include "../include/client.h"

/**
 * init_def_state
 * <p>
 * Initialize the state of the client by assigning default values to state parameters then reading command line
 * arguments to update the state parameters accordingly
 * </p>
 * @param argc - the number of command line arguments
 * @param argv - the command line arguments
 * @param set - the client settings
 */
void init_def_state(int argc, char *argv[], struct client_settings *set);

#endif //RELIABLE_UDP_SETUP_H
