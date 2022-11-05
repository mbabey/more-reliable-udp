
#include "../include/server.h"
#include <stdlib.h>

int main(int argc, char *argv[])
{
    struct server_settings set;
    run(argc, argv, &set);
    
    close_server(&set);
    
    return EXIT_SUCCESS;
}

/* TODO:
 * create a server object
 *      a server should allow its owner to start it
 *          starting a server will make it await connections
 * a server has one server_settings
 * a server can connect to muiltiple clients
 *      each client connection can be a thread, initiated after the a client sends a SYN
 *      each client thread will close when that client disconnects
 *      another client should
 * a server can host a game
 *
 *
 * currently:
 * a server can connect to one client
 */