/**
 * @defgroup loop Loop
 * @{
 * @brief Event Loop API
 */

#ifndef KS_H
#define KS_H

#include <stdatomic.h>
#include <stddef.h>

#include "ks/ret.h"
#include "ks/tcp.h"

#define KS_STOP_IS_ASYNC_SIGNAL_SAFE (ATOMIC_BOOL_LOCK_FREE == 2)

/**
 * Determines behavior of `ks_run()`.
 *
 * @note        Regardless of the chosen mode, after the loop is stopped,
 *              `ks_run()` always returns with 0.
 */
typedef enum
{
  /**
   * @brief       Does 1 operation unless there are no active handles left,
   *              in which case does 0 if the work queue is empty.
   * @note        In this mode `ks_run()` blocks if necessary.
   *
   * Usage:
   * @code
   * // Keep running while there are active handles
   * while (ks_run(KS_RUN_ONCE_OR_DONE))
   *   ;
   * ks_close();
   * @endcode
   */
  KS_RUN_ONCE_OR_DONE, 

  /**
   * @brief       Does exactly 1 operation.
   * @note        In this mode `ks_run()` blocks if necessary.
   *
   * Usage:
   * @code
   * static void * worker_thread_routine(void * context)
   * {
   *   // Keep running until ks_stop() is called
   *   while (ks_run(KS_RUN_ONCE))
   *     ;
   *   ks_close();
   * }
   * @endcode
   */
  KS_RUN_ONCE,

  /**
   * @brief       Does exactly 1 operation if it is possible without blocking,
  *               otherwise 0.
   * @note        In this mode `ks_run()` never blocks.
   *
   * Usage:
   * @code
   * static void * worker_thread_routine(void * context)
   * {
   *   // Keep running until ks_stop() is called
   *   while (ks_run(KS_RUN_ONCE))
   *     ;
   *   ks_close();
   * }
   * @endcode
   */
  KS_RUN_POLL_ONCE,
} ks_run_mode_t;

typedef void (*ks_work_cb_t)(void * context);

/**
 * @see KS_WORK
 */
typedef struct
{
  ks_work_cb_t   fn;
  void         * context;
} ks_work_t;

/**
 * @brief       Constructs `ks_task_t`.
 *
 * @param[in]   fn_
 * @param[in]   ctx_
 *
 * The main usecase is the following:
 * @code
 * ks_post(KS_WORK(do_this_async, context))
 * @endcode
 */
#define KS_WORK(fn_, ctx_) (ks_work_t) { .fn = fn_, .context = ctx_ }

/**
 * @brief       Runs the Event Loop.
 *
 * @param[in]   mode
 * @return      The number of operations executed.
 *
 * @note        An operation counted is either an async work or
 *              a raw IO operation. In case of running IO operation,
 *              the sum of numbers returned by `ks_run()` might smaller than
 *              IO ops requested by application. For example, `ks_tcp_write()`
 *              does next write IO operation in case of the buffer wasn't fully
 *              sent, so that the whole completion requires several `ks_run()`
 *              calls.
`*
 * @see ks_run_mode_t
 */
int ks_run(ks_run_mode_t mode);

/**
 * @brief       Stops the Event Loop, awaking threads blocked at `ks_run()`.
 *
 *              This operation is async-signal-safe if
 *              `KS_STOP_IS_ASYNC_SIGNAL_SAFE` is 1.
 *
 * @note        This operation is irreversible, subsequent `ks_run()` calls
 *              will return 0 immediately.
 */
void ks_stop(void);

/**
 * @brief       Requests to close the Event Loop.
 *              If all threads whichever ran `ks_run()` call this,
 *              the Event Loop is going to be closed and its resources released.
 *               
 * @note        Can be and should be called only from threads that
 *              called `ks_run()`, as it also releases resources allocated
 *              per working threads.
 */
void ks_close(void);

/**
 * @brief       Posts a work to the work queue,
 *              so it will be executed asynchronously.
 *
 * @param[in]   work_ The work to post.
 *
 * @note        As it is implemented as macro, designated initializers 
 *              are not working with it as comma "," denotes macro parameters
 *              seperatation for the preprocessor.
 *              Use appropriate macros like `KS_WORK`.
 *
 * @see KS_WORK instead.
 */
#define ks_post(work_)                           \
  _Generic((work_),                              \
    ks_work_t    : ks__post_work,                \
    ks_io_work_t : ks__post_io_work) (work_)     \

void ks__post_work(ks_work_t work);
void ks__post_io_work(ks_io_work_t work);

void ks__inform_handle_init(void);
void ks__inform_handle_close(void);

#endif

/** @} */
