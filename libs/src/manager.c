//
// Created by Maxwell Babey on 10/30/22.
//

#include "../include/manager.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * mm_add
 * <p>
 * Add a new memory address node to the memory manager linked list. The memory address will not be added if it is
 * already in the linked list.
 * </p>
 * @param mem - the memory to add.
 * @return - the address of the node added, NULL on failure
 */
void *mm_add(struct memory_manager *mem_manager, void *mem);

/**
 * mm_free
 * <p>
 * Free the parameter memory address and remove it from the memory manager.
 * Set return -1 and errno to [EFAULT] if the memory address cannot be located in the memory manager.
 * </p>
 * @param mem_manager - the memory manager to search
 * @param mem - the memory address to free
 * @return 0 on success, -1 on failure
 */
int mm_free(struct memory_manager *mem_manager, void *mem);

/**
 * mm_free_all
 * <p>
 * Free all memory stored in the memory manager.
 * </p>
 * @return the number of memory items freed on success, -1 on failure
 */
int mm_free_all(struct memory_manager *mem_manager);

/**
 * mm_free_recurse
 * <p>
 * Called by mm_free all. Recurse to the end of the memory linked list, then free and return back up.
 * </p>
 * @param ma - the memory address to free
 * @return the number of memory addresses freed
 */
int mm_free_recurse(struct memory_address *ma);

/**
 * alloc_err
 * <p>
 * Print the file, function, and line where an error of err_code occurred.
 * </p>
 * @param file - file in which error occurred
 * @param func - function in which error occurred
 * @param line - line on which error occurred
 * @param err_code - the error code of the error which occurred
 */
void alloc_err(const char *file, const char *func, size_t line,
               int err_code); // NOLINT(bugprone-easily-swappable-parameters)

struct memory_address
{
    void *addr;
    struct memory_address *next;
};

struct memory_manager *init_memory_manager(void)
{
    struct memory_manager *mm;
    
    if ((mm = (struct memory_manager *) s_malloc(sizeof(struct memory_manager),
            __FILE__, __func__, __LINE__)) == NULL)
    {
        return NULL;
    }
    
    mm->head = NULL;
    
    mm->mm_add      = mm_add;
    mm->mm_free     = mm_free;
    mm->mm_free_all = mm_free_all;
    
    return mm;
}

int free_memory_manager(struct memory_manager *mem_manager)
{
    if (mem_manager == NULL)
    {
        errno = EFAULT;
        return -1;
    }
    
    mem_manager->mm_free_all(mem_manager);
    free(mem_manager);
    
    return 0;
}

void *mm_add(struct memory_manager *mem_manager, void *mem)
{
    struct memory_address *ma     = NULL;
    struct memory_address *ma_cur = NULL;
    
    if ((ma = (struct memory_address *) s_malloc(sizeof(struct memory_address),
            __FILE__, __func__, __LINE__)) == NULL)
    {
        return NULL;
    }
    
    ma->addr = mem;
    ma->next = NULL;
    
    if (mem_manager->head == NULL)
    {
        mem_manager->head = ma;
        return ma;
    }
    
    bool                  exists  = false;
    
    ma_cur = mem_manager->head;
    while (ma_cur->next != NULL)
    {
        if (ma_cur->addr == ma->addr)
        {
            exists = true;
            break;
        }
        ma_cur = ma_cur->next;
    }
    if (!exists)
    {
        ma_cur->next = ma;
    }
    
    return ma;
}

int mm_free(struct memory_manager *mem_manager, void *mem)
{
    struct memory_address *ma      = NULL;
    struct memory_address *ma_prev = NULL;
    
    ma = mem_manager->head;
    while (ma->addr != mem && ma != NULL)
    {
        ma_prev = ma;
        ma      = ma->next;
    }
    
    if (ma == NULL)
    {
        errno = EFAULT;
        return -1;
    }
    
    if (ma == mem_manager->head)
    {
        mem_manager->head = ma->next;
    } else
    {
        ma_prev->next = ma->next;
    }
    
    free(ma->addr);
    free(ma);
    
    return 0;
}

int mm_free_all(struct memory_manager *mem_manager)
{
    int m_freed;
    
    m_freed = mm_free_recurse(mem_manager->head);
    
    return m_freed;
}

int mm_free_recurse(struct memory_address *ma) // NOLINT(misc-no-recursion) : recursion intentional
{
    if (ma == NULL)
    {
        return 0;
    }
    
    int m_freed;
    
    m_freed = 1 + mm_free_recurse(ma->next);
    
    free(ma->addr);
    free(ma);
    
    return m_freed;
}

void *s_malloc(size_t size, const char *file, const char *func, size_t line)
{
    void *mem = NULL;
    if ((mem = malloc(size)) == NULL)
    {
        alloc_err(file, func, line, errno);
    }
    return mem;
}


void *s_calloc(size_t count, size_t size, const char *file, const char *func, size_t line)
{
    void *mem = NULL;
    if ((mem = calloc(count, size)) == NULL)
    {
        alloc_err(file, func, line, errno);
    }
    return mem;
}

void *s_realloc(void *ptr, size_t size, const char *file, const char *func, size_t line)
{
    void *mem = NULL;
    if ((mem = realloc(ptr, size)) == NULL)
    {
        alloc_err(file, func, line, errno);
    }
    return mem;
}

void alloc_err(const char *file, const char *func, const size_t line,
               int err_code) // NOLINT(bugprone-easily-swappable-parameters)
{
    const char *msg;
    
    msg = strerror(err_code); // NOLINT(concurrency-mt-unsafe)
    (void) fprintf(stderr, "Memory allocation error (%s @ %s:%zu %d) - %s\n", file, func, line, err_code,
            msg);
}
