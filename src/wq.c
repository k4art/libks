#include <sys/eventfd.h>

#include "ks/alloc.h"
#include "ks/log/thlog.h"
#include "wq.h"
#include "macros.h"

void ks_wq_init(ks_wq_t * wq, int evt_fd)
{
  assert(wq);
  assert(evt_fd);
  
  wq->head = NULL;
  wq->tail = NULL;
  wq->evt_fd = evt_fd;

  pthread_mutex_init(&wq->mutex, NULL);
}

static void ks__wq_destroy_nodes(ks_wq_t * wq)
{
  assert(wq);
  
  ks_wq_node_t * node = wq->head;
  ks_wq_node_t * temp;

  while (node)
  {
    temp = node;
    node = node->next;

    free(temp);
  }

  wq->head = wq->tail = NULL;  // avoid keeping danging pointers
}

void ks_wq_close(ks_wq_t * wq)
{
  assert(wq);
  
  ks__wq_destroy_nodes(wq);
  pthread_mutex_destroy(&wq->mutex);

  // wq->evt_fd was borrowed
}

bool ks__wq_is_empty(ks_wq_t * wq)
{
  assert(wq);
  
  return wq->head == NULL;
}

ks_wq_node_t * ks__wq_pop(ks_wq_t * wq)
{
  assert(wq);
  
  ks_wq_node_t * wq_node = NULL;

  KS_CRITICAL_SECTION(&wq->mutex)
  {
    if (!ks__wq_is_empty(wq))
    {
      wq_node = wq->head;
      wq->head = wq->head->next;

      if (wq->head == NULL)
        wq->tail = NULL;  // avoid keeping danging pointer
    }
  }

  return wq_node;
}

int ks_wq_poll(ks_wq_t * wq)
{
  assert(wq);

  ks_wq_node_t * wq_node = ks__wq_pop(wq);

  if (wq_node)
  {
    ks_wq_item_t * work = &wq_node->entry;

    work->cb_wrapper(work);
    ks_free(wq_node);  // wq_node must be freed after the work is executed,
                       // because the execution might use data
                       // allocated together with the node
  }

  return !!wq_node;
}

ks_wq_node_t * ks__wq_entry(ks_wq_node_t       * work_node,
                            const ks_wq_item_t * wq_item)
{
  ks_wq_node_t * node = ks_malloc(sizeof(ks_wq_node_t));

  memcpy(&node->entry, wq_item, sizeof(*wq_item));
  node->next = NULL;

  return node;
}

void ks_wq_push(ks_wq_t * wq, const ks_wq_item_t * wq_item)
{
  assert(wq);
  assert(wq_item);

  ks_wq_node_t * work_node = ks__wq_entry(work_node, wq_item);

  KS_CRITICAL_SECTION(&wq->mutex)
  {
    if (wq->head == NULL)
    {
      assert(wq->tail == NULL);

      wq->head = wq->tail = work_node;
    }
    else
    {
      work_node->next = wq->head;
      wq->head = work_node;
    }
  }

  thlog("notify");
  KS_RET_CHECKED(eventfd_write(wq->evt_fd, 1));
}
