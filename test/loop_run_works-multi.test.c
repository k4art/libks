#include <pthread.h>

#include "expect.h"
#include "ks.h"

#define THREADS_NUMBER 100

static int flags[THREADS_NUMBER];

static void raise_flag_cb(void * user_data)
{
  size_t i = (intptr_t) user_data;
  flags[i] = 1;
}

static void * worker_routine(void * context)
{
  ks_run(KS_RUN_ONCE);
  ks_close();

  return NULL;
}

int main(void)
{
  pthread_t thread_ids[THREADS_NUMBER];

  for (size_t i = 0; i < THREADS_NUMBER; i++)
  {
    pthread_create(&thread_ids[i], NULL, worker_routine, (void *) i);
  }

  for (size_t i = 0; i < THREADS_NUMBER; i++)
  {
    ks_post(KS_WORK(raise_flag_cb, (void *) i));
  }
  
  for (size_t i = 0; i < THREADS_NUMBER; i++)
  {
    pthread_join(thread_ids[i], NULL);
  }

  int total = 0;
  for (size_t i = 0; i < THREADS_NUMBER; i++)
  {
    total += flags[i];
  }

  EXPECT(total == THREADS_NUMBER);
}
