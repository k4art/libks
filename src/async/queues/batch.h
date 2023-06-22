#ifndef KS_TASKS_BATCH_H
#define KS_TASKS_BATCH_H

#include <stddef.h>
#include <stdbool.h>

#include "async/task.h"

typedef struct ks_tasks_batch_s ks_tasks_batch_t;

struct ks_tasks_batch_s
{
  ks_tasks_batch_t * next;
  size_t             capacity;
  size_t             length;
  ks_task_t          tasks[];
};

void ks_tasks_batch_create(ks_tasks_batch_t ** batch, size_t capacity);
void ks_tasks_batch_destroy(ks_tasks_batch_t * batch);

size_t ks_tasks_batch_fill(ks_tasks_batch_t * batch,
                           const ks_task_t  * from,
                           const ks_task_t  * last);

bool ks_tasks_batch_is_full(ks_tasks_batch_t * batch);

#endif
