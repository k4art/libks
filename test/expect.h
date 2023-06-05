#include <assert.h>

#ifndef NDEBUG
#define EXPECT(x) assert(x)
#else
#define EXPECT(x) do { if (!(x)) exit(EXIT_FAILURE) } while (0)
#endif 
