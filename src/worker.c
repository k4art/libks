#include <threads.h>

#include "wq.h"
#include "ks.h"

#include "worker.h"

typedef enum
{
  KS__IOWQ,
  KS__WQ,
} ks__queue_symbol_t;

typedef struct
{
  ks_wq_t            * wq;
  ks__queue_symbol_t   last_queue_polled;
  int                  eventfd;
  size_t               active_handles;
  ks_aio_t             aio;
} ks__worker_ctx_t;

static thread_local ks__worker_ctx_t m_ctx_tss;

void ks_worker_init(ks_wq_t * wq, int eventfd)
{
  assert(wq);
  assert(eventfd > 0);

  m_ctx_tss.wq      = wq;
  m_ctx_tss.eventfd = eventfd;

  ks_aio_init(&m_ctx_tss.aio, eventfd);
}

bool ks_worker_is_init(void)
{
  return m_ctx_tss.wq != NULL;
}

void ks_worker_close(void)
{
  ks_aio_close(&m_ctx_tss.aio);
}

int ks_worker_poll(void)
{
  ks__worker_ctx_t * ctx = &m_ctx_tss;

  switch (ctx->last_queue_polled)
  {
    case KS__IOWQ:
      if (ks_wq_poll(ctx->wq))    return 1;
      if (ks_aio_poll(&ctx->aio)) return 1;
      return 0;

    case KS__WQ: 
      if (ks_aio_poll(&ctx->aio)) return 1;
      if (ks_wq_poll(ctx->wq))    return 1;
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
