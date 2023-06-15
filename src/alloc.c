#include <stdio.h>
#include <stdlib.h>

#include "ks/alloc.h"
#include "ks/ret.h"

static ks_alloc_table_t m_table =
{
  .malloc = malloc,
  .free   = free,
};

void ks_alloc_table_set(const ks_alloc_table_t * table)
{
  m_table.malloc = table->malloc;
  m_table.free = table->free;
}

void * ks_malloc(size_t size)
{
  void * ptr = m_table.malloc(size);

  if (!ptr)
  {
    ks_error("bad malloc");
    abort();
  }

  return ptr;
}

void ks_free(void * ptr)
{
  m_table.free(ptr);
}

