#ifndef KS_SHARED_T
#define KS_SHARED_T

#include "async/queues/global.h"
#include "async/idles.h"

typedef struct
{
  ks_idles_t    idles;
  ks_q_global_t q_global;
} ks_shared_t;

#endif
