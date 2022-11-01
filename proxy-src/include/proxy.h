//
// Created by Maxwell Babey on 10/27/22.
//

#ifndef RELIABLE_UDP_PROXY_H
#define RELIABLE_UDP_PROXY_H

#include "../../libs/include/structs.h"

/**
 * run.
 * <p>
 * Set up the proxy based on command line arguments, then link it.
 * </p>
 * @param argc - the number of command line arguments
 * @param argv - the command line arguments
 * @param set - the proxy settings
 */
void run(int argc, char *argv[], struct proxy_settings *set);

#endif //RELIABLE_UDP_PROXY_H
