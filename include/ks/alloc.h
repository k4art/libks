#ifndef KS_ALLOC_H
#define KS_ALLOC_H

#include <stddef.h>

typedef void * (* ks_malloc_t)(size_t size);
typedef void (* ks_free_t)(void * ptr);

typedef struct
{
  /**
   * @brief       Custom memory allocator, defaults to standard malloc().
   * @note        Aborts execution if it returned `NULL`.
   */
  ks_malloc_t malloc;

  /**
   * @brief       Free function for the custom memory allocator,
   *              defaults to standard free().
   * @note        Must handle `NULL`.
   */
  ks_free_t   free;
} ks_alloc_table_t;

void ks_alloc_table_set(const ks_alloc_table_t * table);

void * ks_malloc(size_t size);
void ks_free(void * ptr);

#endif