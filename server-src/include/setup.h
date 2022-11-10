//
// Created by Maxwell Babey on 10/24/22.
//

#ifndef RELIABLE_UDP_SETUP_H
#define RELIABLE_UDP_SETUP_H

#include "server-util.h"

/**
 * Server timeout intervals in seconds.
 */
#define SERVER_TIMEOUT_SHORT 10
#define SERVER_TIMEOUT_MED 30
#define SERVER_TIMEOUT_LONG 60

void init_def_state(int argc, char *argv[], struct server_settings *set);

#endif //RELIABLE_UDP_SETUP_H
