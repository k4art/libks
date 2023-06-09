#include "expect.h"
#include "ks.h"

static void do_increment(void * context)
{
  size_t * count = context;

  (*count)++;
}

int main(void)
{
  const size_t half_count = 100;

  size_t extra_work_count = half_count;
  size_t counted = 0;

  for (size_t i = 0; i < half_count; i++)
  {
    ks_post(KS_WORK(do_increment, &counted));
  }

  while (ks_run(KS_RUN_ONCE_OR_DONE))
  {
    if (extra_work_count > 0)
    {
      extra_work_count--;
      ks_post(KS_WORK(do_increment, &counted));
    }
  }

  EXPECT(counted == half_count * 2);
}

