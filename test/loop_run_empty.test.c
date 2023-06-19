#include <signal.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "expect.h"
#include "ks.h"

#define WAIT_SIGNAL(signum_)                                       \
  do {                                                             \
    sigset_t signal_set;                                           \
    int      signum;                                               \
    sigemptyset(&signal_set);                                      \
    sigaddset(&signal_set, signum_);                               \
    KS_RET_CHECKED(pthread_sigmask(SIG_BLOCK, &signal_set, NULL)); \
    sigwait(&signal_set, &signum);                                 \
    assert(signum == signum_);                                     \
  } while (0)

static void * thread_routine(void * context)
{
  EXPECT(ks_run(KS_RUN_POLL_ONCE) == 0);
  EXPECT(ks_run(KS_RUN_ONCE_OR_DONE) == 0);

  EXPECT(ks_run(KS_RUN_ONCE) == 0);

  ks_close();
  return NULL;
}

int main(void)
{
  pthread_t thrd;

  pthread_create(&thrd, NULL, thread_routine, NULL);
  alarm(1);
  WAIT_SIGNAL(SIGALRM);
  ks_stop();
  pthread_join(thrd, NULL);
}
