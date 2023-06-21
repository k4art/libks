#include <threads.h>
#include <unistd.h>

#include "async/idles.h"
#include "async/queues/local.h"
#include "async/queues/global.h"

#include "ks.h"

#include "worker.h"

#include "../test/expect.h"

typedef enum
{
  KS__IOWQ,
  KS__WQ,
} ks__queue_symbol_t;

typedef struct
{
  ks_shared_t        * shared;
  ks__queue_symbol_t   last_queue_polled;
  int                  eventfd;
  size_t               active_handles;
  ks_aio_t             aio;
  ks_q_local_t         q_local;
} ks__worker_ctx_t;

static thread_local ks__worker_ctx_t m_ctx_tss;

void ks_worker_init(ks_shared_t * shared)
{
  assert(shared);

  int evtfd = eventfd(0, 0);
  KS_RET_CHECKED(evtfd);

  m_ctx_tss.shared = shared;
  m_ctx_tss.eventfd = evtfd;

  ks_aio_init(&m_ctx_tss.aio, evtfd);
  ks_q_local_init(&m_ctx_tss.q_local);
}

bool ks_worker_is_init(void)
{
  return m_ctx_tss.shared != NULL;
}

void ks_worker_close(void)
{
  assert(ks_worker_is_init());
  
  ks_aio_close(&m_ctx_tss.aio);
  ks_q_local_close(&m_ctx_tss.q_local);
  close(m_ctx_tss.eventfd);
}

static int ks__worker_wq_poll(void)
{
  ks_q_local_t  * q_local  = &m_ctx_tss.q_local;
  ks_q_global_t * q_global = &m_ctx_tss.shared->q_global;

  ks_task_t task;

  if (ks_q_local_is_empty(q_local))
  {
    ks_q_local_get_shared(q_local, q_global);
  }

  if (ks_q_local_pop(q_local, &task))
  {
    if (task.cb_wrapper) task.cb_wrapper(&task);

    return 1;
  }

  return 0;
}

int ks_worker_poll(void)
{
  assert(ks_worker_is_init());

  ks__worker_ctx_t * ctx = &m_ctx_tss;

  switch (ctx->last_queue_polled)
  {
    case KS__IOWQ:
      ctx->last_queue_polled = KS__WQ;

      if (ks__worker_wq_poll()) return 1;
      if (ks_aio_poll(&ctx->aio)) return 1;
      return 0;

    case KS__WQ: 
      ctx->last_queue_polled = KS__IOWQ;

      if (ks_aio_poll(&ctx->aio)) return 1;
      if (ks__worker_wq_poll()) return 1;
      return 0;
  }
}

ks_ret_t ks_worker_aio_request(const ks_aio_op_t           * op,
                               const ks_aio_polling_base_t * polling,
                               size_t                        polling_size)
{
  assert(ks_worker_is_init());
  assert(op);
  assert(polling);
  assert(polling_size > 0);
  
  return ks_aio_request(&m_ctx_tss.aio, op, polling, polling_size);
}

void ks_worker_wait(void)
{
  assert(ks_worker_is_init());

  ks_idles_sleep(&m_ctx_tss.shared->idles, m_ctx_tss.eventfd);
}

void ks_worker_post(const ks_task_t * task)
{
  assert(ks_worker_is_init());
  assert(task);

  ks_q_local_t  * q_local  = &m_ctx_tss.q_local;
  ks_q_global_t * q_global = &m_ctx_tss.shared->q_global;
  
  if (ks_q_local_push_or_share(q_local, task, q_global))
  {
    ks_idles_t * idles = &m_ctx_tss.shared->idles;

    ks_idles_wakeup(idles);
  }
}
