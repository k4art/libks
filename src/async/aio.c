#include "aio.h"
#include "ks/alloc.h"
#include "ks/ret.h"
#include <liburing.h>

#define IO_URING_ENTRIES 1024

void ks_aio_init(ks_aio_t * aio, int eventfd)
{
  assert(aio);
  assert(eventfd > 0);

  io_uring_queue_init(IO_URING_ENTRIES, &aio->io_uring, 0);
  io_uring_register_eventfd(&aio->io_uring, eventfd);
}

void ks_aio_close(ks_aio_t * aio)
{
  assert(aio);
  io_uring_queue_exit(&aio->io_uring);
}

static void * ks__aio_entry(const ks_aio_polling_base_t * polling,
                            size_t                        polling_size)
{
  const size_t entry_size = KS_AIO_POLLING_DERIVED_MAX_SIZE;

  assert(polling);
  assert(polling_size <= entry_size);

  void * entry = ks_malloc(entry_size);
  memcpy(entry, polling, polling_size);
  return entry;
}

static ks_ret_t ks__aio_push_entry(ks_aio_t          * aio,
                                   const ks_aio_op_t * op,
                                   void              * entry)
{
  assert(aio);
  assert(op);
  assert(entry);
  
  struct io_uring_sqe * sqe = NULL;

  sqe = io_uring_get_sqe(&aio->io_uring);

  if (!sqe)
    return KS_ENOMEM;

  io_uring_prep_rw(op->opcode,
                   sqe,
                   op->issue_data.fd,
                   op->issue_data.buffer,
                   op->issue_data.bytes,
                   0);

  io_uring_sqe_set_data(sqe, entry);

  return io_uring_submit(&aio->io_uring);
}

ks_ret_t ks_aio_request(ks_aio_t                    * aio,
                        const ks_aio_op_t           * op,
                        const ks_aio_polling_base_t * polling,
                        size_t                        polling_size)
{
  assert(aio);
  assert(op);
  assert(polling);
  
  ks_ret_t   ret   = KS_ESUCCESS;
  void     * entry = NULL;

  entry = ks__aio_entry(polling, polling_size);
  ret   = ks__aio_push_entry(aio, op, entry);

  if (ks_ret_is_err(ret))
    ks_free(entry);

  return ret;
}

int ks_aio_poll(ks_aio_t * aio)
{
  assert(aio);

  struct io_uring_cqe * cqe = NULL;
  
  if (!io_uring_peek_cqe(&aio->io_uring, &cqe) && cqe)
  {
    ks_aio_polling_base_t * entry = NULL;
    ks_aio_op_t             op;
    
    entry = (ks_aio_polling_base_t *) cqe->user_data;

    switch (entry->poll(entry, cqe->res, &op))
    {
      case KS_AIO_POLL_STOP:
        ks_free(entry);
        break;

      case KS_AIO_POLL_CONTINUE:
        ks__aio_push_entry(aio, &op, entry);
        break;
    }

    io_uring_cqe_seen(&aio->io_uring, cqe);
    return 1;
  }

  return 0;
}
