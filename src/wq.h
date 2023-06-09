#ifndef KS_WQ_H
#define KS_WQ_H

#include <pthread.h>
#include <stdalign.h>
#include <stddef.h>
#include <string.h>
#include "macros.h"

#include "ks.h"

typedef struct ks_wq_node_s ks_wq_node_t;
typedef struct ks_wq_item_s ks_wq_item_t;

typedef void (* ks_wq_cb_wrapper_t)(ks_wq_item_t * wq_item);

struct ks_wq_item_s
{
  ks_wq_cb_wrapper_t   cb_wrapper;
  void              (* user_cb)();
  void               * user_data;
  int                  io_res;
};

struct ks_wq_node_s
{
  ks_wq_node_t * next;
  ks_wq_item_t   entry;
};

typedef struct
{
  int               evt_fd;
  ks_wq_node_t    * head;
  ks_wq_node_t    * tail;
  pthread_mutex_t   mutex;
} ks_wq_t;

void ks_wq_init(ks_wq_t * wq, int evt_fd);
void ks_wq_close(ks_wq_t * wq);

int ks_wq_poll(ks_wq_t * wq);

/**
 * @brief       Adds a work to the work queue.
 *
 * @param[in]   wq_       Work queue.
 * @param[in]   wq_item_  Item to be put in the queue
 */
void ks_wq_push(ks_wq_t * wq, const ks_wq_item_t * wq_item);

#endif
