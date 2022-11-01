#include "../../libs/include/error.h"
#include "../../libs/include/input-validation.h"
#include "../../libs/include/manager.h"
#include "../../libs/include/util.h"
#include "../include/setup.h"
#include <assert.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define USAGE "client -i <host IP> -o <server IP> -p <port number> -f (for file input) "

void parse_arguments(int argc, char *argv[], struct client_settings *settings);

void init_def_state(int argc, char *argv[], struct client_settings *settings)
{
    memset(settings, 0, sizeof(struct client_settings));
    
    settings->server_fd   = STDOUT_FILENO;
    settings->server_port = DEFAULT_PORT;
    settings->is_file     = false;
    
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
    
    while ((c = getopt(argc, argv, ":i:o:p:f")) != -1)   // NOLINT(concurrency-mt-unsafe)
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
            case 'f':
            {
                settings->is_file = true;
                break;
            }
            default:
            {
                assert("should not get here");
            }
        }
    }
    if (settings->server_ip == NULL)
    {
        advise_usage(USAGE);
        free_mem_manager(settings->mem_manager);
        exit(EXIT_SUCCESS); // NOLINT(concurrency-mt-unsafe) : no threads here
    }
    if (settings->client_ip == NULL)
    {
        set_self_ip(&settings->client_ip);
        if (settings->client_ip == NULL)
        {
            (void) fprintf(stderr,
                    "Could not automatically get host IP address; please enter IP address manually with '-i' flag.\n");
            advise_usage(USAGE);
            free_mem_manager(settings->mem_manager);
            exit(EXIT_SUCCESS); // NOLINT(concurrency-mt-unsafe) : no threads here
        }
    }
}
