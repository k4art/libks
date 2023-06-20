#ifndef KS_WQ_H
#define KS_WQ_H

#include <pthread.h>
#include <stdalign.h>
#include <stddef.h>
#include <string.h>
#include "macros.h"

#include "idles.h"
#include "ks.h"

#define KS_WQ_CAPACITY 8

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

typedef struct
{
  ks_idles_t      * idles;
  pthread_mutex_t   mutex;
  ks_wq_node_t    * head;
  ks_wq_node_t    * tail;
} ks_wq_global_t;

typedef struct
{
  ks_wq_item_t      cqueue[KS_WQ_CAPACITY];
  size_t            head;
  size_t            tail;
} ks_wq_local_t;

typedef struct
{
  ks_wq_global_t * global;
  ks_wq_local_t    local;
} ks_wq_t;

void ks_wq_global_init(ks_wq_global_t * global, ks_idles_t * idles);
void ks_wq_init(ks_wq_t * wq, ks_wq_global_t * global);

void ks_wq_close(ks_wq_t * wq);
void ks_wq_global_close(ks_wq_global_t * global);

int ks_wq_poll(ks_wq_t * wq);

/**
 * @brief       Adds a work to the work queue.
 *
 * @param[in]   wq_       Work queue.
 * @param[in]   wq_item_  Item to be put in the queue
 */
void ks_wq_push(ks_wq_t * wq, const ks_wq_item_t * wq_item);
void ks_wq_global_push(ks_wq_global_t * wq, const ks_wq_item_t * wq_item);

#endif
