#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <threads.h>
#include <stdarg.h>

#include "ks/log/thlog.h"

static thread_local FILE * m_logfile;
static thread_local int    m_tid;

/** unused */
void thlog__exit(void)
{
  fclose(m_logfile);
}

bool thlog__is_init(void)
{
  return !!m_logfile;
}

int thlog__tid(void)
{
  if (!m_tid) m_tid = gettid();

  return m_tid;
}

void thlog__init(void)
{
  if (!m_logfile)
  {
    char filename[64];
    int tid = thlog__tid();
    sprintf(filename, "thlog.%d", tid);

    m_logfile = fopen(filename, "aw");
  }
}

void thlog__log(const char * format, ...)
{
  va_list args;
  va_start(args, format);
  {
    vfprintf(m_logfile, format, args);
  }
  va_end(args);
}
