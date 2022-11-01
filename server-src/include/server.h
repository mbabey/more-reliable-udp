//
// Created by Maxwell Babey on 10/24/22.
//

#ifndef RELIABLE_UDP_SERVER_H
#define RELIABLE_UDP_SERVER_H

#include "../../libs/include/structs.h"

/**
 * run.
 * <p>
 * Set up the server based on command line arguments, then open it.
 * </p>
 * @param argc - the number of command line arguments
 * @param argv - the command line arguments
 * @param set - the server settings
 */
void run(int argc, char *argv[], struct server_settings *set);


#endif //RELIABLE_UDP_SERVER_H
