#include "../../libs/include/structs.h"
#include "../include/client.h"
#include <stdlib.h>

/**
 * main
 * <p>
 * Drives the program.
 * </p>
 * @param argc - the number of command line arguments
 * @param argv - the command line arguments
 * @return - 0, success
 */
int main(int argc, char *argv[])
{
    struct client_settings settings;
    run_client(argc, argv, &settings);
    
    close_client(&settings);
    
    return EXIT_SUCCESS;
}
