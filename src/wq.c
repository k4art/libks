#include <sys/eventfd.h>

#include "ks/alloc.h"
#include "ks/log/thlog.h"
#include "wq.h"
#include "macros.h"

#include "../test/expect.h"

#define INDEX_MASK (KS_WQ_CAPACITY - 1)

struct ks_wq_node_s
{
  ks_wq_node_t * next;
  size_t         items_number;
  ks_wq_item_t   items[];
};

void ks_wq_global_init(ks_wq_global_t * global, ks_idles_t * idles)
{
  assert(global);
  assert(idles);

  global->head = NULL;
  global->tail = NULL;
  global->idles = idles;

  pthread_mutex_init(&global->mutex, NULL);
}

void ks_wq_global_close(ks_wq_global_t * global)
{
  if (!global) return;
  
  pthread_mutex_destroy(&global->mutex);
}

void ks_wq_global_push_batch(ks_wq_global_t     * global,
                             size_t               items_number,
                             const ks_wq_item_t * wq_items)
{
  assert(global);
  assert(items_number > 0);
  assert(wq_items);

  size_t items_size = items_number * sizeof(ks_wq_item_t);

  ks_wq_node_t * node = ks_malloc(sizeof(ks_wq_node_t) + items_size);

  node->next = NULL;
  node->items_number = items_number;

  memcpy(node->items, wq_items, items_size);

  EXPECT_EQ(pthread_mutex_lock(&global->mutex), 0);
  {
    if (!global->head)
    {
      global->head = node;
    }
    else
    {
      assert(global->tail);
      global->tail->next = node;
    }

    global->tail = node;
  }
  EXPECT_EQ(pthread_mutex_unlock(&global->mutex), 0);
}

void ks_wq_global_push(ks_wq_global_t * global, const ks_wq_item_t * wq_item)
{
  ks_wq_global_push_batch(global, 1, wq_item);
}

size_t ks_wq_global_pop_batch(ks_wq_global_t * global, ks_wq_item_t * dst, size_t capacity)
{
  ks_wq_node_t * node = NULL;

  EXPECT_EQ(pthread_mutex_lock(&global->mutex), 0);
  {
    if (global->head)
    {
      node = global->head;
      global->head = global->head->next;

      if (!global->head)
        global->tail = NULL;
    }
  }
  EXPECT_EQ(pthread_mutex_unlock(&global->mutex), 0);

  if (!node) return 0;

  size_t items_number = node->items_number;
  assert(items_number <= capacity);

  memcpy(dst, node->items, items_number * sizeof(ks_wq_item_t));
  ks_free(node);

  return items_number;
}

static void ks_wq_local_init(ks_wq_local_t * local)
{
  local->head = 0;
  local->tail = 0;
}

void ks_wq_init(ks_wq_t * wq, ks_wq_global_t * global)
{
  assert(wq);
  assert(global);
  
  wq->global = global;

  ks_wq_local_init(&wq->local);
}

void ks_wq_close(ks_wq_t * wq)
{
  // local queue does not require clean-up,
  // while global queue is borrowed
}

bool ks__wq_local_is_empty(ks_wq_local_t * local)
{
  assert(local);
  
  return local->head == local->tail;
}

bool ks__wq_local_pop(ks_wq_local_t * local, ks_wq_item_t * p_item)
{
  assert(local);
  
  if (!ks__wq_local_is_empty(local))
  {
    size_t head = local->head & INDEX_MASK;
    *p_item = local->cqueue[head];

    local->head++;
    return true;
  }

  return false;
}

static bool ks__wq_try_poll_local(ks_wq_t * wq)
{
  ks_wq_item_t work;

  if (ks__wq_local_pop(&wq->local, &work))
  {
    if (work.cb_wrapper) work.cb_wrapper(&work);
    return true;
  }

  return false;
}

int ks_wq_poll(ks_wq_t * wq)
{
  assert(wq);

  size_t items;

  if (ks__wq_try_poll_local(wq))
    return 1;

  items = ks_wq_global_pop_batch(wq->global,
                                 wq->local.cqueue,
                                 KS_WQ_CAPACITY);

  wq->local.head = 0;
  wq->local.tail = items;

  return ks__wq_try_poll_local(wq);
}

static void ks__wq_local_push(ks_wq_local_t * local, const ks_wq_item_t * wq_item)
{
  assert(local);
  assert(wq_item);
  assert(local->tail - local->head < KS_WQ_CAPACITY);

  size_t tail = local->tail & INDEX_MASK;

  memcpy(local->cqueue + tail, wq_item, sizeof(*wq_item));
  local->tail++;
}

void ks_wq_push(ks_wq_t * wq, const ks_wq_item_t * wq_item)
{
  assert(wq);
  assert(wq_item);

  if (wq->local.tail - wq->local.head == KS_WQ_CAPACITY)
  {
    ks_wq_global_push(wq->global, wq_item);
  }
  else
  {
    ks__wq_local_push(&wq->local, wq_item);
  }

  thlog("wq queue: %zu", wq->local.tail - wq->local.head);

  if (wq->local.tail - wq->local.head == KS_WQ_CAPACITY)
  {
    // TODO: Currently it moves arbitrary number of works from
    //       the beginning of the local.cqueue all elements.
    //       But this should move half of the queue.

    size_t tail = wq->local.tail & INDEX_MASK;
    size_t head = wq->local.head & INDEX_MASK;

    ks_wq_item_t * batch_start = wq->local.cqueue;
    size_t         batch_size  = tail;

    if (tail > 0)
    {
      ks_wq_global_push_batch(wq->global, batch_size, batch_start);
      ks_idles_wakeup(wq->global->idles);

      wq->local.tail -= tail;
    }

  }
}
