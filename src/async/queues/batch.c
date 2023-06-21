#include <assert.h>
#include <string.h>

#include "async/queues/batch.h"

#include "ks/alloc.h"
#include "ks/ret.h"

void ks_tasks_batch_create(ks_tasks_batch_t ** p_batch, size_t capacity)
{
  assert(p_batch);
  assert(capacity > 0);

  size_t tasks_space = capacity * sizeof(ks_task_t);
  size_t batch_size  = sizeof(ks_tasks_batch_t) + tasks_space;

  ks_tasks_batch_t * batch = ks_malloc(batch_size);

  batch->capacity = capacity;
  batch->length = 0;
  batch->next = NULL;

  *p_batch = batch;
}

void ks_tasks_batch_destroy(ks_tasks_batch_t * batch)
{
  if (!batch) return;

  ks_free(batch);
}

size_t ks_tasks_batch_fill(ks_tasks_batch_t * batch,
                           const ks_task_t  * from,
                           const ks_task_t  * last)
{
  assert(batch);
  assert(from);
  assert(last);
  assert(from <= last);

  ks_task_t * insert_point = &batch->tasks[batch->length];
  size_t n = last - from + 1;

  if (n > batch->capacity - batch->length)
  {
    n = batch->capacity - batch->length;
  }
  
  memcpy(insert_point, from, n * sizeof(ks_task_t));
  batch->length += n;
  return n;
}

bool ks_tasks_batch_is_full(ks_tasks_batch_t * batch)
{
  assert(batch);

  return batch->length == batch->capacity;
}
