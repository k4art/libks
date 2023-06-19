#include <string.h>
#include <assert.h>

#include "idles.h"
#include "macros.h"

#include "ks/log/thlog.h"

void ks_idles_init(ks_idles_t * idles)
{
  assert(idles);

  memset(idles, 0, sizeof(*idles));

  pthread_mutex_init(&idles->mutex, NULL);
}

void ks_idles_close(ks_idles_t * idles)
{
  if (!idles) return;

  pthread_mutex_destroy(&idles->mutex);
}

static bool ks_idles_push(ks_idles_t * idles, int evtfd)
{
  assert(idles);
  assert(evtfd > 0);

  bool success = false;

  KS_CRITICAL_SECTION(&idles->mutex)
  {
    if (!idles->is_stopped)
    {
      idles->sleeper_evtfd[idles->sleepers_number] = evtfd;
      idles->sleepers_number++;
      success = true;
    }
  }

  return success;
}

static int ks_idles_pop(ks_idles_t * idles)
{
  assert(idles);

  int evtfd = 0;

  KS_CRITICAL_SECTION(&idles->mutex)
  {
    if (idles->sleepers_number > 0)
    {
      idles->sleepers_number--;
      evtfd = idles->sleeper_evtfd[idles->sleepers_number];
    }
  }

  return evtfd;
}

static void ks_idles_remove(ks_idles_t * idles, int evtfd)
{
  assert(idles);
  assert(evtfd > 0);

  KS_CRITICAL_SECTION(&idles->mutex)
  {
    size_t i = idles->sleepers_number;

    while (i --> 0)
    {
      if (evtfd == idles->sleeper_evtfd[i])
        break;
    }

    if (i < idles->sleepers_number)
    {
      int    * from          = idles->sleeper_evtfd + i + 1;
      size_t   total_to_move = idles->sleepers_number - i - 1;

      memmove(idles->sleeper_evtfd + i, from, total_to_move * sizeof(int));

      idles->sleepers_number--;
    }
  }
}

void ks_idles_wakeup(ks_idles_t * idles)
{
  assert(idles);

  thlog("wakeup");
  int evtfd = ks_idles_pop(idles);

  if (evtfd > 0)
  {
    eventfd_write(evtfd, 1);
  }
}

void ks_idles_wakeup_all(ks_idles_t * idles)
{
  assert(idles);

  KS_CRITICAL_SECTION(&idles->mutex)
  {
    for (size_t i = 0; i < idles->sleepers_number; i++)
    {
      eventfd_write(idles->sleeper_evtfd[i], 1);
    }

    idles->sleepers_number = 0;
    idles->is_stopped = true;
  }
}

void ks_idles_sleep(ks_idles_t * idles, int evtfd)
{
  assert(idles);
  assert(evtfd > 0);

  if (ks_idles_push(idles, evtfd))
  {
  	eventfd_read(evtfd, &(eventfd_t) {0});
    ks_idles_remove(idles, evtfd);  // if wakes up not by `ks_idles_sleep()`
  }
  else
  {
    // idles is stopped, so nobody can sleep
    // assert(idles->is_stopped == true);
  }
}
