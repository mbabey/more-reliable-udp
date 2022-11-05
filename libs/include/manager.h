//
// Created by Maxwell Babey on 10/30/22.
//

#ifndef MEMORY_MANAGER_MANAGER_H
#define MEMORY_MANAGER_MANAGER_H

#include <stddef.h>

struct memory_manager
{
    struct memory_address *head;
    
    void *(*mm_add)(struct memory_manager *, void *);
    
    int (*mm_free)(struct memory_manager *, void *);
    
    int (*mm_free_all)(struct memory_manager *);
};

struct memory_manager *init_mem_manager(void);

int free_mem_manager(struct memory_manager *mem_manager);

void *s_malloc(size_t size, const char *file, const char *func, size_t line);

void *s_calloc(size_t count, size_t size, const char *file, const char *func, size_t line);

void *s_realloc(void *ptr, size_t size, const char *file, const char *func, size_t line);

#endif //MEMORY_MANAGER_MANAGER_H
