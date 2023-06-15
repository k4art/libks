#include <assert.h>

#include "ks/ret.h"

#define EXPECT(x)                                     \
  do {                                                \
    typeof((x)) r = (x);                              \
    if (!r)                                           \
    {                                                 \
      ks_error("EXPECT(%s) failed with %d\n", #x, r); \
      exit(EXIT_FAILURE);                             \
    }                                                 \
  } while (0)
