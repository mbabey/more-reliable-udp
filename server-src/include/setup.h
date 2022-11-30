//
// Created by Maxwell Babey on 10/24/22.
//

#ifndef RELIABLE_UDP_SETUP_H
#define RELIABLE_UDP_SETUP_H

#include "server-util.h"

/**
 * init_def_state
 * <p>
 * Initialize the default values in the server settings. Parse command line arguments.
 * </p>
 * @param argc - the number of command line arguments
 * @param argv - the command line arguments
 * @param set - the sever settings
 */
void init_def_state(int argc, char *argv[], struct server_settings *set);

#endif //RELIABLE_UDP_SETUP_H
