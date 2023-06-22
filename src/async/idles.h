#ifndef KS_IDLES_H
#define KS_IDLES_H

#include <pthread.h>
#include <stdbool.h>
#include <sys/eventfd.h>

/* TODO: implement unbounded via dynamic vector */
#define KS_IDLES_MAX_CAPACITY 64

typedef struct
{
  pthread_mutex_t mutex;
  size_t          sleepers_number;
  int             sleeper_evtfd[KS_IDLES_MAX_CAPACITY];
  bool            is_stopped;
} ks_idles_t;

void ks_idles_init(ks_idles_t * idles);
void ks_idles_close(ks_idles_t * idles);

void ks_idles_wakeup_all_and_stop(ks_idles_t * idles);
void ks_idles_wakeup(ks_idles_t * idles);
void ks_idles_sleep(ks_idles_t * idles, int evtfd);

#endif