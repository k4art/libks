#include <assert.h>

#ifndef NDEBUG
#define EXPECT(x) assert(x)
#else
#define EXPECT(x)                                                              \
  do {                                                                         \
    typeof((x)) r = (x);                                                       \
    if (!r) {                                                                  \
      fprintf(stderr, "EXPECT(%s) failed with %d\n", #x, r);                   \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)
#endif 
