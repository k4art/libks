#include "expect.h"
#include "ks.h"

static void do_increment(void * context)
{
  size_t * count = context;

  count++;
}

int main(void)
{
  size_t should_count = 100;
  size_t counted = 0;

  for (size_t i = 0; i < should_count; i++)
  {
    ks_post(KS_WORK(do_increment, &counted));
  }

  while (ks_run(KS_RUN_ONCE_OR_DONE))
    ;

  EXPECT(counted == should_count);
}

