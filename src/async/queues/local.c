#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "async/queues/batch.h"
#include "async/queues/global.h"
#include "async/queues/local.h"

#define INDEX_MASK (KS_Q_LOCAL_CAPACITY - 1)

static_assert(KS_Q_LOCAL_CAPACITY % 2 == 0,
              "Local queue relies on valid value of INDEX_MASK. "
              "Adjust KS_Q_LOCAL_CAPACITY to one less a power of 2");

void ks_q_local_init(ks_q_local_t * q_local)
{
  assert(q_local);

  memset(q_local, 0, sizeof(ks_q_local_t));
}

void ks_q_local_close(ks_q_local_t * q_local)
{
  assert(q_local);

  // no resources to release
}

bool ks_q_local_is_empty(ks_q_local_t * q_local)
{
  assert(q_local);

  return q_local->tail == q_local->head;
}

static bool ks_q_local_is_full(ks_q_local_t * q_local)
{
  assert(q_local);

  return q_local->tail - q_local->head == KS_Q_LOCAL_CAPACITY;
}

static void ks_q_local_push(ks_q_local_t    * q_local,
                            const ks_task_t * task)
{
  assert(q_local);
  assert(task);

  assert(!ks_q_local_is_full(q_local));

  size_t tail = q_local->tail & INDEX_MASK;
  memcpy(&q_local->cqueue[tail], task, sizeof(ks_task_t));

  q_local->tail++;
}

static void ks_q_local_share(ks_q_local_t  * q_local,
                             ks_q_global_t * q_global)
{
  size_t head = q_local->head & INDEX_MASK;
  size_t tail = (q_local->tail - 1) & INDEX_MASK; // TODO: refactor
  size_t midl = ((q_local->tail + q_local->head) / 2) & INDEX_MASK;

  ks_task_t * first = &q_local->cqueue[midl];  /* first to share */
  ks_task_t * last  = &q_local->cqueue[tail];  /* last to share */

  ks_tasks_batch_t * batch = NULL;
  ks_tasks_batch_create(&batch, (q_local->tail - q_local->head) / 2);

  size_t n = 0;

  if (first < last)
  {
    n += ks_tasks_batch_fill(batch, first, last);

    assert(ks_tasks_batch_is_full(batch));
  }
  else
  {
    ks_task_t * q_end = &q_local->cqueue[KS_Q_LOCAL_CAPACITY - 1];
    ks_task_t * q_beg = q_local->cqueue;

    n += ks_tasks_batch_fill(batch, first, q_end);
    n += ks_tasks_batch_fill(batch, q_beg, last);

    assert(ks_tasks_batch_is_full(batch));
  }

  ks_q_global_push(q_global, batch);

  q_local->tail -= n;
}

int ks_q_local_push_or_share(ks_q_local_t    * q_local,
                             const ks_task_t * task,
                             ks_q_global_t   * q_global)
{
  assert(q_local);
  assert(task);
  assert(q_global);

  if (ks_q_local_is_full(q_local))
  {
    ks_q_local_share(q_local, q_global);

    // Even though the function name might imply either push or share,
    // in the case of full local queue it is still necessary
    // to push the new task.
    //
    // Alternative to this is sharing after making local queue full.
    ks_q_local_push(q_local, task);

    return 1;
  }
  else
  {
    ks_q_local_push(q_local, task);
    return 0;
  }
}

bool ks_q_local_pop(ks_q_local_t * q_local, ks_task_t * task)
{
  if (ks_q_local_is_empty(q_local)) return false;

  size_t head = q_local->head & INDEX_MASK;
  memcpy(task, &q_local->cqueue[head], sizeof(ks_task_t));
  q_local->head++;

  return true;
}

void ks_q_local_get_shared(ks_q_local_t * q_local, ks_q_global_t * q_global)
{
  ks_tasks_batch_t * batch = NULL;

  if (ks_q_global_pop(q_global, &batch))
  {
    // TODO: Implement in general case, currently works
    //       only if local queue is empty.

    assert(ks_q_local_is_empty(q_local));
    assert(KS_Q_LOCAL_CAPACITY >= batch->length);

    q_local->head = 0;
    q_local->tail = batch->length;

    memcpy(q_local->cqueue, batch->tasks, batch->length * sizeof(ks_task_t));

    ks_tasks_batch_destroy(batch);
  }
}
