/**
 * @defgroup loop Loop
 * @{
 * @brief Event Loop API
 */

#ifndef KS_H
#define KS_H

#include <stddef.h>

#include "ks/ret.h"
#include "ks/tcp.h"

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
   * while (ks_run(KS_RUN_ONCE_OR_DONE) == 1)
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
   *   while (ks_run(KS_RUN_ONCE) == 1)
   *     ;
   *   ks_close();
   * }
   * @endcode
   */
  KS_RUN_ONCE,

  /**
   * @brief       Does exactly 1 operation if it is possible without blocking,
  *               otherwise 0.
   * @note        In this mode `ks_run()` nevery blocks.
   *
   * Usage:
   * @code
   * static void * worker_thread_routine(void * context)
   * {
   *   // Keep running until ks_stop() is called
   *   while (ks_run(KS_RUN_ONCE) == 1)
   *     ;
   *   ks_close();
   * }
   * @endcode
   */
  KS_RUN_POLL_ONCE,
} ks_run_mode_t;

typedef void (* ks_work_cb_t)(void * context);

typedef struct
{
  ks_work_cb_t   fn;
  void            * context;
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
 *
 * @see ks_run_mode_t
 */
int ks_run(ks_run_mode_t mode);

/**
 * @brief       Stops the Event Loop, awaking threads blocked at `ks_run()`.
 * @note        This operation is irreversible.
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
 * @note        As it is implemented as macro, designated initializers 
 *              are not working with it as comma "," denotes macro parameters
 *              seperatation for the preprocessor.
 *              Use appropriate macros like @see KS_WORK instead.
 */
#define ks_post(work)                            \
  _Generic((work),                               \
    ks_work_t    : ks__post_work,                \
    ks_io_work_t : ks__post_io_work) (work)      \

// TODO: alternatively there should be one `ks__post()`
//       that takes a function wrapper

void ks__post_work(ks_work_t work);
void ks__post_io_work(ks_io_work_t work);

#endif

/** @} */
