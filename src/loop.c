#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <stdatomic.h>
#include <sys/eventfd.h>

#include "idles.h"
#include "ks/log/thlog.h"
#include "ks/alloc.h"
#include "worker.h"
#include "shared.h"

#include "ks.h"
#include "../test/expect.h"

typedef struct
{
  atomic_uint    workers_running;
  atomic_uint    handles_count;
  atomic_bool    is_stopped;
  pthread_once_t init_once;
  ks_shared_t    shared;
} ks__loop_t;

static ks__loop_t m_loop =
{
  .init_once = PTHREAD_ONCE_INIT,
};

static void ks__init(void)
{
  ks_idles_init(&m_loop.shared.idles);
  ks_wq_global_init(&m_loop.shared.wq_global, &m_loop.shared.idles);

  // Other struct fields default to 0 as needed.
}

static void ks__close(void)
{
  ks_wq_global_close(&m_loop.shared.wq_global);
  ks_idles_close(&m_loop.shared.idles);
}

void ks_stop(void)
{
  atomic_store(&m_loop.is_stopped, true);
  ks_idles_wakeup_all(&m_loop.shared.idles);
}

void ks_close(void)
{
  assert(ks_worker_is_init());

  ks_worker_close();
  unsigned count = atomic_fetch_sub_explicit(&m_loop.workers_running,
                                             1,
                                             memory_order_release);

  if (count == 1)
    ks__close();
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
    ks_worker_init(&m_loop.shared);
    // atomic_fetch_add_explicit(&m_loop.workers_running, 1, memory_order_relaxed);
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
  ks_worker_wait();
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

  // Reached here if the loop is stopped.
  return 0;
}

static void ks__work_wq_cb_wrapper(ks_wq_item_t * wq_item)
{
  ks_work_cb_t user_cb = wq_item->user_cb;

  user_cb(wq_item->user_data);
}

void ks__post_wq_item(const ks_wq_item_t * wq_item)
{
  ks__ensure_init();

  if (ks_worker_is_init())
  {
    ks_worker_post(wq_item);
  }
  else
  {
    ks_wq_global_push(&m_loop.shared.wq_global, wq_item);
    ks_idles_wakeup(&m_loop.shared.idles);
  }
}

void ks__post_work(ks_work_t work)
{
  ks__post_wq_item(&(ks_wq_item_t) 
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
  ks__post_wq_item(&(ks_wq_item_t)
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
  unsigned handles = atomic_fetch_sub_explicit(&m_loop.handles_count,
                                               1,
                                               memory_order_acq_rel);

  if (handles == 1)
  {
    // Notify workers blocked in ks_run(KS_RUN_ONCE_OR_DONE)
    // because of presence of active handlers.

    ks_idles_wakeup(&m_loop.shared.idles);
  }
}

