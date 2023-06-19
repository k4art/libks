#include <assert.h>

#include "ks/ret.h"

#define EXPECT_EQ(a, b)                                       \
  do {                                                        \
    int r = a == b;                                           \
    if (!r)                                                   \
    {                                                         \
      ks_error("EXPECT_EQ(%s == %s): failed as %ld != %ld",   \
               #a,                                            \
               #b,                                            \
               (long)a,                                       \
               (long)b);                                      \
      exit(EXIT_FAILURE);                                     \
    }                                                         \
  } while (0)

#define EXPECT(x)                                     \
  do {                                                \
    typeof((x)) r = (x);                              \
    if (!r)                                           \
    {                                                 \
      ks_error("EXPECT(%s) failed with %d", #x, r);   \
      exit(EXIT_FAILURE);                             \
    }                                                 \
  } while (0)
