#include "../../libs/include/structs.h"
#include "../include/client.h"
#include <stdlib.h>

int main(int argc, char *argv[])
{
    struct client_settings settings;
    run_client(argc, argv, &settings);

    return EXIT_SUCCESS;
}
