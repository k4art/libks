#include <signal.h>
#include <sys/eventfd.h>
#include <stdatomic.h>

#include "ks.h"

typedef struct
{
  atomic_uint           handles_count;
  int                   eventfd;
  volatile sig_atomic_t is_stopped;
  // task_queue
  // ...
} ks__loop_t;

ks__loop_t g_loop;

// this should be async-signal-safe
void ks_stop(void)
{
  g_loop.is_stopped = 1;
  eventfd_write(g_loop.eventfd, 0xDEAD);
}

