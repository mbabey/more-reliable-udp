#include "../../libs/include/error.h"
#include "../../libs/include/input-validation.h"
#include "../../libs/include/manager.h"
#include "../../libs/include/util.h"
#include "../include/client.h"
#include "../include/setup.h"
#include <assert.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define USAGE "client -i <host IP> -o <server IP> -p <port number>"

/**
 * parse_arguments
 * <p>
 * Parse the command line arguments and set values in the settings appropriately.
 * Clean up and terminate the program if a user error occurs.
 * </p>
 * @param argc - the number of command line arguments
 * @param argv - the command line arguments
 * @param settings - the settings for the Client
 */
void parse_arguments(int argc, char *argv[], struct client_settings *settings);

void init_def_state(int argc, char *argv[], struct client_settings *settings)
{
    memset(settings, 0, sizeof(struct client_settings));
    
    settings->server_fd   = STDOUT_FILENO;
    settings->server_port = DEFAULT_PORT;
    
    settings->timeout = (struct timeval *) s_calloc(1, sizeof(struct timeval), __FILE__, __func__, __LINE__);
    if (errno == ENOTRECOVERABLE)
    {
        exit(EXIT_FAILURE); // NOLINT(concurrency-mt-unsafe) : no threads here
    }
    
    settings->mem_manager = init_mem_manager();
    settings->mem_manager->mm_add(settings->mem_manager, settings->timeout);
    
    parse_arguments(argc, argv, settings);
}

void parse_arguments(int argc, char *argv[], struct client_settings *settings)
{
    const uint8_t base = 10;
    int           c;
    
    while ((c = getopt(argc, argv, ":i:o:p:")) != -1)   // NOLINT(concurrency-mt-unsafe)
    {
        switch (c)
        {
            case 'i':
            {
                check_ip(optarg, base);
                if (errno == ENOTRECOVERABLE)
                {
                    free_mem_manager(settings->mem_manager);
                    exit(EXIT_SUCCESS); // NOLINT(concurrency-mt-unsafe) : no threads here
                }
                
                settings->client_ip = optarg;
                break;
            }
            case 'o':
            {
                check_ip(optarg, base);
                if (errno == ENOTRECOVERABLE)
                {
                    free_mem_manager(settings->mem_manager);
                    exit(EXIT_SUCCESS); // NOLINT(concurrency-mt-unsafe) : no threads here
                }
                
                settings->server_ip = optarg;
                break;
            }
            case 'p':
            {
                settings->server_port = parse_port(optarg, base);
                if (errno == ENOTRECOVERABLE)
                {
                    free_mem_manager(settings->mem_manager);
                    exit(EXIT_SUCCESS); // NOLINT(concurrency-mt-unsafe) : no threads here
                }
                
                break;
            }
            default:
            {
                assert("should not get here");
            }
        }
    }
    if (settings->server_ip == NULL) /* If the server IP has not been input, exit. */
    {
        advise_usage(USAGE);
        free_mem_manager(settings->mem_manager);
        exit(EXIT_SUCCESS); // NOLINT(concurrency-mt-unsafe) : no threads here
    }
    
    if (settings->client_ip == NULL) /* If the client IP has not been input, try the self_ip function. */
    {
        set_self_ip(&settings->client_ip);
        if (settings->client_ip == NULL) /* If the client IP has still not been set, exit. */
        {
            (void) fprintf(stderr,
                    "Could not automatically get host IP address; please enter IP address manually with '-i' flag.\n");
            advise_usage(USAGE);
            free_mem_manager(settings->mem_manager);
            exit(EXIT_SUCCESS); // NOLINT(concurrency-mt-unsafe) : no threads here
        }
    }
}
