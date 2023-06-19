#include <threads.h>
#include <unistd.h>

#include "idles.h"
#include "ks/log/thlog.h"
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
  ks_wq_t              wq;
} ks__worker_ctx_t;

static thread_local ks__worker_ctx_t m_ctx_tss;

void ks_worker_init(ks_shared_t * shared)
{
  assert(shared);

  int evtfd = eventfd(0, 0);
  KS_RET_CHECKED(evtfd);

  m_ctx_tss.shared = shared;
  m_ctx_tss.eventfd = evtfd;

  ks_wq_init(&m_ctx_tss.wq, &shared->wq_global);
  ks_aio_init(&m_ctx_tss.aio, evtfd);
}

bool ks_worker_is_init(void)
{
  return m_ctx_tss.shared != NULL;
}

void ks_worker_close(void)
{
  ks_aio_close(&m_ctx_tss.aio);
  ks_wq_close(&m_ctx_tss.wq);
  close(m_ctx_tss.eventfd);
}

int ks_worker_poll(void)
{
  ks__worker_ctx_t * ctx = &m_ctx_tss;

  switch (ctx->last_queue_polled)
  {
    case KS__IOWQ:
      ctx->last_queue_polled = KS__WQ;

      if (ks_wq_poll(&ctx->wq)) return 1;
      if (ks_aio_poll(&ctx->aio)) return 1;
      return 0;

    case KS__WQ: 
      ctx->last_queue_polled = KS__IOWQ;

      if (ks_aio_poll(&ctx->aio)) return 1;
      if (ks_wq_poll(&ctx->wq)) return 1;
      return 0;
  }
}

ks_ret_t ks_worker_aio_request(const ks_aio_op_t           * op,
                               const ks_aio_polling_base_t * polling,
                               size_t                        polling_size)
{
  assert(op);
  assert(polling);
  
  return ks_aio_request(&m_ctx_tss.aio, op, polling, polling_size);
}

void ks_worker_wait(void)
{
  ks_idles_sleep(&m_ctx_tss.shared->idles, m_ctx_tss.eventfd);
}

void ks_worker_post(const ks_wq_item_t * wq_item)
{
  ks_wq_push(&m_ctx_tss.wq, wq_item);
}
