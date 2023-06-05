#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ks.h"

#define CHECKED(expr)                                              \
  do {                                                             \
   int ret = (expr);                                               \
   if (ret != 0)                                                   \
   {                                                               \
     fprintf(stderr,                                               \
            "CHECKED(%s) failed with %s\n", #expr, strerror(ret)); \
     abort();                                                      \
   }                                                               \
 } while (0)                                                       \

#define WAIT_SIGNAL(signum_)                                       \
  do {                                                             \
    sigset_t    signal_set;                                        \
    int         signum;                                            \
    sigemptyset(&signal_set);                                      \
    sigaddset(&signal_set, signum_);                               \
    CHECKED(pthread_sigmask(SIG_BLOCK, &signal_set, NULL));        \
    sigwait(&signal_set, &signum);                                 \
    assert(signum == signum_);                                     \
  } while (0)                                                      \

static atomic_uint       m_online_count;
static char              m_blackhole[2048];
static ks_tcp_acceptor_t m_acceptor;

typedef struct
{
  ks_tcp_conn_t tcp;
} user_conn_t;

static void on_read_cb(int read_res, void * user_data)
{
  user_conn_t * conn = user_data;

  if (read_res <= 0)
  {
    atomic_fetch_sub(&m_online_count, 1);
    ks_tcp_close(&conn->tcp);
    free(conn);
  }
  else
  {
    ks_tcp_read_some(&conn->tcp, m_blackhole, sizeof(m_blackhole), on_read_cb,
                     conn);
  }
}

static void handle_connection(user_conn_t * conn)
{
  atomic_fetch_add(&m_online_count, 1);

  ks_tcp_read_some(&conn->tcp, m_blackhole, sizeof(m_blackhole), on_read_cb,
                   conn);
}

static void on_accept_cb(int result, void * user_data)
{
  ks_tcp_conn_t * tcp_temp = user_data;
  user_conn_t   * conn     = NULL;

  if (result < 0)
  {
    fprintf(stderr, "Error: async accept failed with %s\n", strerror(errno));
  }
  else
  {
    conn = malloc(sizeof(user_conn_t));
    memcpy(&conn->tcp, tcp_temp, sizeof(*tcp_temp));
  }

  ks_tcp_accept(&m_acceptor, tcp_temp, on_accept_cb, tcp_temp);
  if (conn) handle_connection(conn);
}

static void launch_server(void * user_data)
{
  static ks_tcp_conn_t tcp_temp;
  ks_tcp_cb_t conn_handler = user_data;

  m_acceptor = (ks_tcp_acceptor_t)
  {
    .addr.ip   = KS_IPv4(127, 0, 0, 1),
    .addr.port = 8080,
  };

  ks_tcp_accept(&m_acceptor, &tcp_temp, conn_handler, &tcp_temp);
}

static void * worker_routine(void * context)
{
  while (ks_run(KS_RUN_ONCE) == 1)
    ;

  ks_close();
  return NULL;
}

int main(int argc, char ** argv)
{
  if (argc != 2)
  {
    printf("Usage: %s <thread no.>\n", argv[0]);
    return 0;
  }

  size_t      threads_no = atoi(argv[1]);
  pthread_t * thread_ids = malloc(threads_no * sizeof(pthread_t));
    
  ks_post(KS_TASK(launch_server, on_accept_cb));

  for (size_t i = 0; i < threads_no; i++)
  {
    pthread_create(&thread_ids[i], NULL, worker_routine, NULL);
  }

  WAIT_SIGNAL(SIGINT);
  ks_stop();
    
  for (size_t i = 0; i < threads_no; i++)
  {
    pthread_join(thread_ids[i], NULL);
  }
}
