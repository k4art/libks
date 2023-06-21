#include <assert.h>

#include "async/queues/global.h"
#include "async/queues/batch.h"
#include "etc/macros.h"

void ks_q_global_init(ks_q_global_t * q_global)
{
  assert(q_global);

  q_global->head = NULL;
  q_global->tail = NULL;

  pthread_mutex_init(&q_global->mutex, NULL);
}

void ks_q_global_close(ks_q_global_t * q_global)
{
  assert(q_global);

  pthread_mutex_destroy(&q_global->mutex);
}

static bool ks__q_global_is_empty(ks_q_global_t * q_global)
{
  assert(q_global);

  return q_global->head == NULL;
}

void ks_q_global_push_single(ks_q_global_t * q_global, const ks_task_t * task)
{
  assert(q_global);
  assert(task);

  // This logic mostly exists because of `ks_post()` calls
  // from non-worker threads.

  ks_tasks_batch_t * batch = NULL;
  ks_tasks_batch_create(&batch, 1);

  // Fill the batch with only the given task
  ks_tasks_batch_fill(batch, /* from: */ task, /* to: */ task);

  ks_q_global_push(q_global, batch);
}

void ks_q_global_push(ks_q_global_t * q_global, ks_tasks_batch_t * batch)
{
  assert(q_global);
  assert(batch);

  KS_CRITICAL_SECTION(&q_global->mutex)
  {
    if (ks__q_global_is_empty(q_global))
    {
      q_global->head = batch;
      q_global->tail = batch;
    }
    else
    {
      ks_tasks_batch_t * prev = q_global->tail;

      prev->next = batch;
      q_global->tail = batch;
    }
  }
}

bool ks_q_global_pop(ks_q_global_t * q_global, ks_tasks_batch_t ** batch)
{
  assert(q_global);
  assert(batch);

  bool success = false;
  
  KS_CRITICAL_SECTION(&q_global->mutex)
  {
    if (!ks__q_global_is_empty(q_global))
    {
      *batch = q_global->head;
      q_global->head = q_global->head->next;

      success = true;
    }
  }

  return success;
}
