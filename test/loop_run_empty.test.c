#include <signal.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "expect.h"
#include "ks.h"

static void stop_loop_handler(int signum)
{
  ks_stop();
}

int main(void)
{
  EXPECT(ks_run(KS_RUN_POLL_ONCE) == 0);
  EXPECT(ks_run(KS_RUN_ONCE_OR_DONE) == 0);

  signal(SIGALRM, stop_loop_handler);
  alarm(1);

  EXPECT(ks_run(KS_RUN_ONCE) == 0);

  ks_close();
}

