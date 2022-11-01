
#include "../../libs/include/structs.h"
#include "../include/server.h"
#include <stdlib.h>

int main(int argc, char *argv[])
{
    struct server_settings set;
    run(argc, argv, &set);
    
    return EXIT_SUCCESS;
}
