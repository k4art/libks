#ifndef KS_Q_GLOBAL_H
#define KS_Q_GLOBAL_H

#include <pthread.h>
#include <stddef.h>

#include "async/queues/batch.h"
#include "async/task.h"

typedef struct
{
  pthread_mutex_t    mutex;
  ks_tasks_batch_t * head;
  ks_tasks_batch_t * tail;
} ks_q_global_t;

void ks_q_global_init(ks_q_global_t * q_global);
void ks_q_global_close(ks_q_global_t * q_global);

void ks_q_global_push_single(ks_q_global_t * q_global, const ks_task_t * task);
void ks_q_global_push(ks_q_global_t * q_global, ks_tasks_batch_t * batch);
bool ks_q_global_pop(ks_q_global_t * q_global, ks_tasks_batch_t ** batch);

#endif
