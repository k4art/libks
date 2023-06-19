#ifndef KS_SHARED_T
#define KS_SHARED_T

#include "wq.h"
#include "idles.h"

typedef struct
{
  ks_wq_global_t wq_global;
  ks_idles_t     idles;
} ks_shared_t;

#endif
