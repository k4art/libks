#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

static ks_tcp_acceptor_t m_acceptor;

#define COMMAND_FIX_SIZE 5
#define COMMAND_HELLO    "hello"
#define COMMAND_SLEEP    "sleep"

#define COMMAND_SLEEP_DURATION_SEC 5
#define COMMAND_HELLO_RESPONSE_MSG "hello"

typedef struct
{
  ks_tcp_conn_t tcp;
  char          rx_buffer[COMMAND_FIX_SIZE];
} user_conn_t;

static void on_read_cb(int read_res, void * user_data);

static void execute_command_cb(void * user_data)
{
  user_conn_t * conn = user_data;

  ks_tcp_read(&conn->tcp, conn->rx_buffer, COMMAND_FIX_SIZE, on_read_cb, conn);
}

static void execute_command_io_cb(int res, void * user_data)
{
  execute_command_cb(user_data);
}

static void do_sleep_command(void * user_data)
{
  sleep(COMMAND_SLEEP_DURATION_SEC);

  ks_post(KS_WORK(execute_command_cb, user_data));
}

static void do_hello_command(void * user_data)
{
  user_conn_t * conn = user_data;

  ks_tcp_write(&conn->tcp,
               COMMAND_HELLO_RESPONSE_MSG,
               sizeof(COMMAND_HELLO_RESPONSE_MSG),
               execute_command_io_cb, conn);
}

static void async_execute_command(char * command, user_conn_t * conn)
{
  ks_work_cb_t fn;

  if      (!strcmp(command, COMMAND_SLEEP)) fn = &do_sleep_command;
  else if (!strcmp(command, COMMAND_HELLO)) fn = &do_hello_command;

  if (fn != NULL)
  {
    ks_post(KS_WORK(fn, conn));
  }
  else
  {
    ks_post(KS_WORK(execute_command_cb, conn));
  }
}

static void on_read_cb(int read_res, void * user_data)
{
  user_conn_t * conn = user_data;

  if (read_res <= 0)
  {
    ks_tcp_close(&conn->tcp);
    free(conn);
  }
  else
  {
    async_execute_command(conn->rx_buffer, conn);
  }
}

static void handle_connection(user_conn_t * conn)
{
  ks_tcp_read(&conn->tcp, conn->rx_buffer, COMMAND_FIX_SIZE, on_read_cb, conn);
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

  ks_io_cb_t conn_handler = user_data;

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
    
  ks_post(KS_WORK(launch_server, on_accept_cb));

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

  free(thread_ids);
}
