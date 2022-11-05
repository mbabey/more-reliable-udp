
#include "../include/proxy.h"
#include <stdlib.h>

int main(int argc, char *argv[])
{
    struct proxy_settings set;
    run(argc, argv, &set);
    
    exit(EXIT_SUCCESS); // NOLINT(concurrency-mt-unsafe) : no threads here
}
