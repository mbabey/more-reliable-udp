//
// Created by Maxwell Babey on 10/30/22.
//

#ifndef MEMORY_MANAGER_MANAGER_H
#define MEMORY_MANAGER_MANAGER_H

#include <stddef.h>

/**
 * memory_manager
 * <p>
 * Manages memory for an application by using a linked list and providing methods to add and free memory addresses
 * stored therein.
 * </p>
 */
struct memory_manager
{
    struct memory_address *head;
    
    void *(*mm_add)(struct memory_manager *, void *);
    
    int (*mm_free)(struct memory_manager *, void *);
    
    int (*mm_free_all)(struct memory_manager *);
};

/**
 * init_memory_manager
 * <p>
 * Constructor. Initializes a memory manager. Allocates memory, sets the linked list head to null, and initializes
 * function pointers.
 * </p>
 * @return a pointer to the newly initialized memory manager, NULL if allocation fails.
 */
struct memory_manager *init_memory_manager(void);

/**
 * free_memory_manager
 * <p>
 * Free all memory stored in the memory manager, then free the memory manager.
 * </p>
 * @param manager - the memory manager to free.
 * @return the number of memory addresses freed, or -1 if the memory manager passed is NULL
 */
int free_memory_manager(struct memory_manager *manager);

/**
 * s_malloc
 * <p>
 * A malloc that reports memory allocation errors.
 * </p>
 * @param size - the size of memory to allocate
 * @param file - the file in which s_malloc is invoked
 * @param func - the function in which s_malloc is invoked
 * @param line - the line on which s_malloc is invoked
 * @return a pointer to the newly allocated memory, or NULL on allocation failure.
 */
void *s_malloc(size_t size, const char *file, const char *func, size_t line);

/**
 * s_calloc
 * <p>
 * A calloc that reports memory allocation errors.
 * </p>
 * @param count - the number memory units to allocate
 * @param size - the size of memory units to allocate
 * @param file - the file in which s_calloc is invoked
 * @param func - the function in which s_calloc is invoked
 * @param line - the line on which s_calloc is invoked
 * @return a pointer to the newly allocated memory, or NULL on allocation failure.
 */
void *s_calloc(size_t count, size_t size, const char *file, const char *func, size_t line);

/**
 * s_realloc
 * <p>
 * A realloc that reports memory allocation errors.
 * </p>
 * @param ptr - a pointer to the memory to reallocate
 * @param size - the new size of memory to reallocate
 * @param file - the file in which s_realloc is invoked
 * @param func - the function in which s_realloc is invoked
 * @param line - the line on which s_realloc is invoked
 * @return a pointer to the newly allocated memory, or NULL on allocation failure.
 */
void *s_realloc(void *ptr, size_t size, const char *file, const char *func, size_t line);

#endif //MEMORY_MANAGER_MANAGER_H
