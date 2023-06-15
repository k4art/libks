#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <stdatomic.h>
#include <sys/eventfd.h>

#include "ks/alloc.h"
#include "ks/log/thlog.h"
#include "wq.h"
#include "worker.h"

#include "ks.h"

typedef struct
{
  atomic_uint    handles_count;
  atomic_bool    is_stopped;
  int            eventfd;
  ks_wq_t        wq;
  pthread_once_t init_once;
} ks__loop_t;

static ks__loop_t m_loop =
{
  .init_once = PTHREAD_ONCE_INIT,
};

static void ks__init(void)
{
  m_loop.eventfd = eventfd(0, 0);
  ks_wq_init(&m_loop.wq, m_loop.eventfd);

  // Other struct fields defaults to 0 as needed.
}

static void ks__close(void)
{
  ks_wq_close(&m_loop.wq);
  close(m_loop.eventfd);
}

void ks_stop(void)
{
  atomic_store(&m_loop.is_stopped, true);
  eventfd_write(m_loop.eventfd, 1);
}

// not thread-safe
void ks_close(void)
{
  assert(ks_worker_is_init());

  // TODO: close the event loop after the last worker is closed
}

static void ks__ensure_init(void)
{
  pthread_once(&m_loop.init_once, ks__init);
}

static void ks__ensure_worker_init(void)
{
  if (!ks_worker_is_init())
  {
    ks__ensure_init();
    ks_worker_init(&m_loop.wq, m_loop.eventfd);
  }
}

static bool ks__is_alive(void)
{
  return !atomic_load_explicit(&m_loop.is_stopped, memory_order_acquire);
}

static bool ks__has_active_handles(void)
{
  unsigned handles = atomic_load_explicit(&m_loop.handles_count,
                                          memory_order_acquire);

  return handles > 0;
}

static void ks__wait_any_event(void)
{
  eventfd_t evt_value;
  thlog("wait start");
  eventfd_read(m_loop.eventfd, &evt_value);
  thlog("wait end");

  (void) evt_value;
}

int ks_run(ks_run_mode_t mode)
{
  ks__ensure_worker_init();

  while (ks__is_alive())
  {
    if (ks_worker_poll())
      return 1;

    if (mode == KS_RUN_POLL_ONCE)
      return 0;

    if (mode == KS_RUN_ONCE_OR_DONE && !ks__has_active_handles())
      return 0;

    ks__wait_any_event();
  }

  eventfd_write(m_loop.eventfd, 1);  // ks_close() wakes up only one worker,
                                     // to notify others eventfd_write() is repeated

  // Reached here if the loop is stopped.
  return 0;
}

static void ks__work_wq_cb_wrapper(ks_wq_item_t * wq_item)
{
  ks_work_cb_t user_cb = wq_item->user_cb;

  user_cb(wq_item->user_data);
}

void ks__post_work(ks_work_t work)
{
  ks__ensure_init();

  ks_wq_push(&m_loop.wq, &(ks_wq_item_t)
  {
    .cb_wrapper = ks__work_wq_cb_wrapper,
    .user_cb    = work.fn,
    .user_data  = work.context,
  });
}

static void ks__io_work_wq_cb_wrapper(ks_wq_item_t * wq_item)
{
  ks_io_cb_t user_cb = wq_item->user_cb;

  user_cb(wq_item->io_res, wq_item->user_data);
}

void ks__post_io_work(ks_io_work_t io_work)
{
  ks__ensure_init();

  ks_wq_push((&m_loop.wq), &(ks_wq_item_t)
  {
    .cb_wrapper = ks__io_work_wq_cb_wrapper,
    .io_res     = io_work.res,
    .user_cb    = io_work.cb,
    .user_data  = io_work.user_data,
  });
}

void ks__inform_handle_init(void)
{
  ks__ensure_worker_init();
  atomic_fetch_add_explicit(&m_loop.handles_count, 1, memory_order_relaxed);
}

void ks__inform_handle_close(void)
{
  ks__ensure_worker_init();
  atomic_fetch_sub_explicit(&m_loop.handles_count, 1, memory_order_release);

  eventfd_write(m_loop.eventfd, 1);
}

