//
// Created by Maxwell Babey on 10/27/22.
//

#include "../include/setup.h"
#include "../../libs/include/error.h"
#include "../../libs/include/input-validation.h"
#include "../../libs/include/util.h"
#include <errno.h>
#include <string.h>
#include <unistd.h>

/**
 * Usage message; printed when there is a user error upon running.
 */
#define USAGE "proxy -i <host ip address> -o <server ip address> -p <input port number> -P <output port number> -d <drop chance %> -h <hold chance %>"

/**
 * 100%
 */
#define HUNDRED_PERCENT 100

/**
 * The percent chance a message will be dropped by the proxy server.
 */
#define DEFAULT_DROP_PERCENT 35

/**
 * The percent chance a message will be held for a time by the proxy server.
 */
#define DEFAULT_HOLD_PERCENT 15

/**
 * set_defaults
 * <p>
 * Zero the memory in proxy_settings. Set the default port, the default output, and the default timeout.
 * </p>
 * @param set - proxy_settings *: pointer to the settings for this proxy
 */
void set_defaults(struct proxy_settings *set);

/**
 * read_args
 * <p>
 * Read command line arguments and set values in proxy_settings appropriately
 * </p>
 * @param argc - int: the number of command line arguments
 * @param argv - char **: the command line arguments
 * @param set - proxy_settings *: pointer to the settings for this proxy
 */
void read_args(int argc, char *argv[], struct proxy_settings *set);

int8_t get_percentage(char *num_str, uint8_t base);

void check_settings(struct proxy_settings *set, int8_t hold_chance, int8_t drop_chance);

void init_def_state(int argc, char *argv[], struct proxy_settings *set)
{
    set_defaults(set);
    read_args(argc, argv, set);
}

void set_defaults(struct proxy_settings *set)
{
    memset(set, 0, sizeof(struct proxy_settings));
    set->proxy_port  = DEFAULT_PORT;
    set->output_port = DEFAULT_PORT;
    set->mem_manager = init_mem_manager();
}

void read_args(int argc, char *argv[], struct proxy_settings *set)
{
    const uint8_t base = 10;
    int           c;
    int8_t       hold_chance;
    int8_t       drop_chance;
    
    drop_chance = 0;
    hold_chance = 0;
    while ((c = getopt(argc, argv, ":i:o:p:P:d:h:")) != -1) // NOLINT(concurrency-mt-unsafe) : No threads here
    {
        switch (c)
        {
            case 'i':
            {
                check_ip(optarg, base);
                if (errno == ENOTRECOVERABLE)
                {
                    free_mem_manager(set->mem_manager);
                    exit(EXIT_SUCCESS); // NOLINT(concurrency-mt-unsafe) : no threads here
                }
                set->proxy_ip = optarg;
                break;
            }
            case 'o':
            {
                check_ip(optarg, base);
                if (errno == ENOTRECOVERABLE)
                {
                    free_mem_manager(set->mem_manager);
                    exit(EXIT_SUCCESS); // NOLINT(concurrency-mt-unsafe) : no threads here
                }
                set->output_ip = optarg;
                break;
            }
            case 'p':
            {
                set->proxy_port = parse_port(optarg, base);
                if (errno == ENOTRECOVERABLE)
                {
                    free_mem_manager(set->mem_manager);
                    exit(EXIT_SUCCESS); // NOLINT(concurrency-mt-unsafe) : no threads here
                }
                break;
            }
            case 'P':
            {
                set->output_port = parse_port(optarg, base);
                if (errno == ENOTRECOVERABLE)
                {
                    free_mem_manager(set->mem_manager);
                    exit(EXIT_SUCCESS); // NOLINT(concurrency-mt-unsafe) : no threads here
                }
                break;
            }
            case 'd':
            {
                drop_chance = get_percentage(optarg, base);
                break;
            }
            case 'h':
            {
                hold_chance = get_percentage(optarg, base);
                break;
            }
            default:
            {
                advise_usage(USAGE);
                if (errno == ENOTRECOVERABLE)
                {
                    free_mem_manager(set->mem_manager);
                    exit(EXIT_SUCCESS); // NOLINT(concurrency-mt-unsafe) : no threads here
                }
            }
        }
    }
    
    check_settings(set, hold_chance, drop_chance);
}

void check_settings(struct proxy_settings *set, int8_t hold_chance, int8_t drop_chance)
{
    if (set->proxy_ip == NULL)
    {
        set_self_ip(&set->proxy_ip);
        if (set->proxy_ip == NULL)
        {
            (void) fprintf(stderr,
                           "Could not automatically get host IP address; please enter IP address manually with '-s' flag.\n");
            advise_usage(USAGE);
            if (errno == ENOTRECOVERABLE)
            {
                free_mem_manager(set->mem_manager);
                exit(EXIT_SUCCESS); // NOLINT(concurrency-mt-unsafe) : no threads here
            }
        }
    }
    if (set->output_ip == NULL)
    {
        advise_usage(USAGE);
        if (errno == ENOTRECOVERABLE)
        {
            free_mem_manager(set->mem_manager);
            exit(EXIT_SUCCESS); // NOLINT(concurrency-mt-unsafe) : no threads here
        }
    }
    if (drop_chance == -1)
    {
        drop_chance = DEFAULT_DROP_PERCENT;
    }
    if (hold_chance == -1)
    {
        hold_chance = DEFAULT_HOLD_PERCENT;
    }
    if ((drop_chance + hold_chance) > HUNDRED_PERCENT)
    {
        (void) fprintf(stderr, "Percent chance to drop plus percent chance to hold must not exceed 100%%\n");
        advise_usage(USAGE);
        if (errno == ENOTRECOVERABLE)
        {
            free_mem_manager(set->mem_manager);
            exit(EXIT_SUCCESS); // NOLINT(concurrency-mt-unsafe) : no threads here
        }
    }
    
    set->drop_bound = drop_chance;
    set->hold_bound = hold_chance + drop_chance;
}

int8_t get_percentage(char *num_str, uint8_t base)
{
    long percentage = strtol(num_str, &num_str, base);
    
    if (errno == EINVAL || !(0 <= percentage && percentage <= HUNDRED_PERCENT))
    {
        (void) fprintf(stderr, "Invalid percentage entered; using default value.\n");
        percentage = -1;
    }
    return (int8_t) percentage;
}
