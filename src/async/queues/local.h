#ifndef KS_Q_LOCAL_H
#define KS_Q_LOCAL_H

#include <stddef.h>
#include <stdbool.h>

#include "async/queues/global.h"
#include "async/task.h"

#define KS_Q_LOCAL_CAPACITY 4

typedef struct
{
  size_t    head;
  size_t    tail;
  ks_task_t cqueue[KS_Q_LOCAL_CAPACITY];
} ks_q_local_t;

void ks_q_local_init(ks_q_local_t * q_local);
void ks_q_local_close(ks_q_local_t * q_local);

int ks_q_local_push_or_share(ks_q_local_t    * q_local,
                             const ks_task_t * task,
                             ks_q_global_t   * q_global);

bool ks_q_local_is_empty(ks_q_local_t * q_local);
bool ks_q_local_pop(ks_q_local_t * q_local, ks_task_t * task);
void ks_q_local_get_shared(ks_q_local_t * q_local, ks_q_global_t * q_global);

#endif
