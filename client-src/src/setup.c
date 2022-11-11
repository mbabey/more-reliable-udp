#include "../../libs/include/error.h"
#include "../../libs/include/input-validation.h"
#include "../../libs/include/util.h"
#include "../include/setup.h"
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

/**
 * Usage message; printed when there is a user error upon running.
 */
#define USAGE "client -i <host IP> -o <server IP> -p <port number>"

/**
 * set_client_defaults
 * <p>
 * Zero the memory in client_settings. Set the default port, initialize the memory manager, and setup the timeout
 * struct.
 * </p>
 * @param set - client_settings *: pointer to the settings for this client
 */
void set_client_defaults(struct client_settings *set);

/**
 * parse_arguments
 * <p>
 * Parse the command line arguments and set values in the set appropriately.
 * Clean up and terminate the program if a user error occurs.
 * </p>
 * @param argc - the number of command line arguments
 * @param argv - the command line arguments
 * @param set - the set for the Client
 */
void parse_arguments(int argc, char *argv[], struct client_settings *set);

void init_def_state(int argc, char *argv[], struct client_settings *set)
{
    set_client_defaults(set);
    
    parse_arguments(argc, argv, set);
}

void set_client_defaults(struct client_settings *set)
{
    memset(set, 0, sizeof(struct client_settings));
    set->server_port = DEFAULT_PORT;
    
    if ((set->mm = init_memory_manager()) == NULL)
    {
        exit(EXIT_FAILURE);
    }
    
    if ((set->timeout = (struct timeval *) s_calloc(1, sizeof(struct timeval),
            __FILE__, __func__, __LINE__)) ==  NULL)
    {
        free_memory_manager(set->mm);
        exit(EXIT_FAILURE); // NOLINT(concurrency-mt-unsafe) : no threads here
    }
    
    set->mm->mm_add(set->mm, set->timeout);
}

void parse_arguments(int argc, char *argv[], struct client_settings *set)
{
    const uint8_t base = 10;
    int           c;
    
    while ((c = getopt(argc, argv, ":o:p:")) != -1)   // NOLINT(concurrency-mt-unsafe)
    {
        switch (c)
        {
            case 'o':
            {
                if ((set->server_ip = check_ip(optarg, base)) == NULL)
                {
                    free_memory_manager(set->mm);
                    exit(EXIT_SUCCESS); // NOLINT(concurrency-mt-unsafe) : no threads here
                }
                break;
            }
            case 'p':
            {
                set->server_port = parse_port(optarg, base);
                if (errno == ENOTRECOVERABLE)
                {
                    free_memory_manager(set->mm);
                    exit(EXIT_SUCCESS); // NOLINT(concurrency-mt-unsafe) : no threads here
                }
                
                break;
            }
            default:
            {
                advise_usage(USAGE);
                free_memory_manager(set->mm);
                exit(EXIT_SUCCESS); // NOLINT(concurrency-mt-unsafe) : no threads here
            }
        }
    }
    if (set->server_ip == NULL) /* If the server IP has not been input, exit. */
    {
        advise_usage(USAGE);
        free_memory_manager(set->mm);
        exit(EXIT_SUCCESS); // NOLINT(concurrency-mt-unsafe) : no threads here
    }
}
