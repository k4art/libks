#ifndef THLOG_H
#define THLOG_H

#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>

bool thlog__is_init(void);
void thlog__init(void);
void thlog__log(const char * format, ...);
int  thlog__tid(void);

#ifdef KS_THLOG_ENABLE

#define thlog(format, ...)                                                     \
  do {                                                                         \
    if (!thlog__is_init())                                                     \
      thlog__init();                                                           \
                                                                               \
    struct timespec t;                                                         \
    clock_gettime(CLOCK_MONOTONIC, &t);                                        \
    thlog__log("[%d] %lu %lu: " format "\n", thlog__tid(), t.tv_sec,           \
               t.tv_nsec, ##__VA_ARGS__);                                      \
  } while (0)

#else

#define thlog(format, ...)

#endif

#endif
