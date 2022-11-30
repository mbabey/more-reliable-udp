//
// Created by Maxwell Babey on 10/24/22.
//

#include "../include/manager.h"
#include "../include/Game.h"
#include "../include/setup.h"
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

/**
 * Usage message; printed when there is a user error upon running.
 */
#define USAGE "server -i <host ip address> -p <port number>"

/**
 * set_server_defaults
 * <p>
 * Zero the memory in server_settings. Set the default port and initialize the memory manager.
 * </p>
 * @param set - server_settings *: pointer to the settings for this server
 */
void set_server_defaults(struct server_settings *set);

/**
 * read_args
 * <p>
 * Read command line arguments and set values in server_settings appropriately
 * </p>
 * @param argc - int: the number of command line arguments
 * @param argv - char **: the command line arguments
 * @param set - server_settings *: pointer to the settings for this server
 */
void read_args(int argc, char *argv[], struct server_settings *set);

void init_def_state(int argc, char *argv[], struct server_settings *set)
{
    set_server_defaults(set);
    if (!errno)
    { read_args(argc, argv, set); }
}

void set_server_defaults(struct server_settings *set)
{
    memset(set, 0, sizeof(struct server_settings));
    set->server_port = DEFAULT_PORT;
    
    if ((set->mm = init_memory_manager()) == NULL)
    {
        return;
    }
    
    if ((set->game = initializeGame()) == NULL)
    {
        return;
    }
    set->mm->mm_add(set->mm, set->game);
}

void read_args(int argc, char *argv[], struct server_settings *set)
{
    const int base = 10;
    int       c;
    
    while ((c = getopt(argc, argv, ":i:p:")) != -1) // NOLINT(concurrency-mt-unsafe) : No threads here
    {
        switch (c)
        {
            case 'i':
            {
                if ((set->server_ip = check_ip(optarg, base)) == NULL)
                {
                    return;
                }
                break;
            }
            case 'p':
            {
                set->server_port = parse_port(optarg, base);
                if (errno == ENOTRECOVERABLE)
                {
                    return;
                }
                
                break;
            }
            default:
            {
                advise_usage(USAGE);
                return;
            }
        }
    }
    if (set->server_ip == NULL)
    {
        set_self_ip(&set->server_ip);
        if (set->server_ip == NULL)
        {
            (void) fprintf(stderr, "Could not automatically get host IP address; "
                                   "please enter IP address manually with '-i' flag.\n");
            advise_usage(USAGE);
            return;
        }
    }
}
