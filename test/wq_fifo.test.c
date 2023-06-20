#include "ks.h"
#include "expect.h"

#define MAX 100

static void f(void * user_data)
{
  static size_t expected = 0;

  size_t got = (intptr_t) user_data;

  EXPECT_EQ(got, expected);
  expected++;
}

int main(void)
{
  for (size_t i = 0; i < MAX; i++)
    ks_post(KS_WORK(f, (void *) i));

  while (ks_run(KS_RUN_ONCE_OR_DONE))
    ;
}

