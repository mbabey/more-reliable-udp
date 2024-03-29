#include "../include/Controller.h"
#include "../include/Game.h"
#include "../include/client-util.h"
#include "../include/setup.h"
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

/**
 * Usage message; printed when there is a user error upon running.
 */
#define USAGE "client -o <server IP> -p <port number>"

/**
 * set_client_defaults
 * <p>
 * Zero the memory in client_settings. Initialize the controller. Allocate memory for and initialize state parameters.
 * </p>
 * @param set - the client settings
 */
void set_client_defaults(struct client_settings *set);

/**
 * allocate_defaults
 * <p>
 * Allocate memory for timeval and packet structs in client settings. Initialize memory manager. Add timeval and
 * packet structs to memory manager. Return -1 if any allocation is unsuccessful. Return 0 otherwise.
 * </p>
 * @param set - the client settings
 * @return -1 if an allocation fails, 0 otherwise
 */
int allocate_defaults(struct client_settings *set);

/**
 * parse_arguments
 * <p>
 * Parse the command line arguments and set state parameters appropriately.
 * </p>
 * @param argc - the number of command line arguments
 * @param argv - the command line arguments
 * @param set - the client settings
 */
void parse_arguments(int argc, char *argv[], struct client_settings *set);

void init_def_state(int argc, char *argv[], struct client_settings *set)
{
    set_client_defaults(set);
    
    if (!errno)
    { parse_arguments(argc, argv, set); }
}

void set_client_defaults(struct client_settings *set)
{
    memset(set, 0, sizeof(struct client_settings));
    set->server_port = DEFAULT_PORT;
    set->turn        = false;
    
    if ((controllerSetup()) == -1)
    {
        return;
    }
    
    errno = 0; /* errno set in controllerSetup, but it does not matter; clean it. */
    
    if (allocate_defaults(set) == -1)
    {
        return;
    }
}

int allocate_defaults(struct client_settings *set)
{
    if ((set->mm       = init_memory_manager()) == NULL)
    {
        return -1;
    }
    if ((set->timeout  = (struct timeval *) s_calloc(1, sizeof(struct timeval),
                                                     __FILE__, __func__, __LINE__)) == NULL)
    {
        free_memory_manager(set->mm);
        return -1;
    }
    if ((set->s_packet = (struct packet *) s_calloc(1, sizeof(struct packet),
                                                    __FILE__, __func__, __LINE__)) == NULL)
    {
        free(set->timeout);
        free_memory_manager(set->mm);
        return -1;
    }
    if ((set->r_packet = (struct packet *) s_calloc(1, sizeof(struct packet),
                                                    __FILE__, __func__, __LINE__)) == NULL)
    {
        free(set->s_packet);
        free(set->timeout);
        free_memory_manager(set->mm);
        return -1;
    }
    if ((set->game = initializeGame()) == NULL)
    {
        free(set->r_packet);
        free(set->s_packet);
        free(set->timeout);
        free_memory_manager(set->mm);
        return -1;
    }
    
    set->mm->mm_add(set->mm, set->game);
    set->mm->mm_add(set->mm, set->timeout);
    set->mm->mm_add(set->mm, set->s_packet);
    set->mm->mm_add(set->mm, set->r_packet);
    
    return 0;
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
    if (set->server_ip == NULL) /* If the server IP has not been input, exit. */
    {
        advise_usage(USAGE);
        return;
    }
}
