//
// Created by Maxwell Babey on 10/23/22.
//

#include "../include/error.h"
#include "../include/input-validation.h"
#include "../include/util.h"
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

char *check_ip(char *ip, uint8_t base)
{
    const char *msg     = NULL;
    char       *ip_cpy  = NULL;
    char       *end     = NULL;
    char       *tok     = NULL;
    char       delim[2] = ".";
    int        tok_count;
    long       num;
    
    set_string(&ip_cpy, ip);
    if (errno == ENOMEM)
    {
        fatal_errno(__FILE__, __func__, __LINE__, errno);
        return NULL;
    }
    
    /* Tokenize the IP address by byte. */
    tok = strtok(ip_cpy, delim); // NOLINT(concurrency-mt-unsafe) : No threads here
    
    tok_count = 0;
    while (tok != NULL)
    {
        ++tok_count;
        
        num = strtol(tok, &end, base);
        
        if (end == tok)
        {
            msg = "IP address must be a decimal number";
        } else if (*end != '\0')
        {
            msg = "IP address input must not have extra characters appended";
        } else if ((num < 0 || num > UINT8_MAX) ||
                   ((LONG_MIN == num || LONG_MAX == num) && ERANGE == errno))
        {
            msg = "IP address unit must be between 0 and 255";
        }
        
        if (msg)
        {
            advise_usage(msg);
            free(ip_cpy);
            return NULL;
        }
        
        tok = strtok(NULL, delim); // NOLINT(concurrency-mt-unsafe) : No threads here
    }
    if (tok_count != 4)
    {
        msg = "IP address must be in form XXX.XXX.XXX.XXX";
        advise_usage(msg);
        free(ip_cpy);
        return NULL;
    }
    free(ip_cpy);
    
    return ip;
}

in_port_t parse_port(const char *buffer, uint8_t base)
{
    const char *msg = NULL;
    char       *end;
    long       sl;
    in_port_t  port;
    
    sl = strtol(buffer, &end, base);
    
    if (end == buffer)
    {
        msg = "Port number must be a decimal number";
    } else if (*end != '\0')
    {
        msg = "Port number input must not have extra characters appended";
    } else if ((sl < 0 || sl > UINT16_MAX) ||
               ((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno))
    {
        msg = "Port number must be between 0 and 65535";
    }
    
    if (msg)
    {
        advise_usage(msg);
    }
    
    port = (in_port_t) sl;
    
    return port;
}
