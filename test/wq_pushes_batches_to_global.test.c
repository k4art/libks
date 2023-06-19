#include <pthread.h>
#include <unistd.h>

#include "expect.h"
#include "ks.h"

#define RUNTIME_WORKS_PRODUCED_ITER  500
#define RUNTIME_WORKS_PRODUCED_RATE   10

static size_t total_works_done = 0;

static void dummy_work(void * user_data) {}

static void increment(void * user_data)
{
  size_t * counter = user_data;

  (*counter)++;
}

static void * worker_only_consumes(void * context)
{
  while (ks_run(KS_RUN_ONCE_OR_DONE))
    ;


  ks_close();
  return NULL;
}

static void * worker_produces_more(void * context)
{
  size_t i = 0;

  while (ks_run(KS_RUN_ONCE_OR_DONE))
  {
    if (i < RUNTIME_WORKS_PRODUCED_ITER)
    {
      for (size_t j = 0; j < RUNTIME_WORKS_PRODUCED_RATE; j++)
      {
        ks_post(KS_WORK(increment, &total_works_done));
      }

      i++;
    }
  }

  ks_close();
  return NULL;
}

int main(void)
{
  pthread_t threads[2];

  ks_post(KS_WORK(dummy_work, NULL));

  pthread_create(&threads[1], NULL, worker_produces_more, NULL);

  // allows the first worker to produce some work for the second worker
  sleep(1);

  pthread_create(&threads[0], NULL, worker_only_consumes, NULL);

  pthread_join(threads[0], NULL);
  pthread_join(threads[1], NULL);

  EXPECT_EQ(total_works_done, RUNTIME_WORKS_PRODUCED_ITER * RUNTIME_WORKS_PRODUCED_RATE);
}
