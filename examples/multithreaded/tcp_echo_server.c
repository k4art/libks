#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "ks/alloc.h"
#include "ks/log/thlog.h"
#include "ks.h"

#define RX_BUFFER_SIZE 512
#define WORKERS_NUMBER 2

typedef struct
{
  ks_tcp_conn_t tcp;
  char          rx_buffer[RX_BUFFER_SIZE];
} app_conn_t;

static inline app_conn_t * app_conn_create(ks_tcp_conn_t * tcp_temp)
{
  app_conn_t * conn = ks_malloc(sizeof(app_conn_t));
  memcpy(&conn->tcp, tcp_temp, sizeof(*tcp_temp));
  return conn;
}

static inline void app_conn_destroy(app_conn_t * conn)
{
  ks_tcp_close(&conn->tcp);
  free(conn);
}

static ks_tcp_acceptor_t m_acceptor;

static void * worker_routine(void * context);
static void app_echo_repeat(int res, void * user_data);

static void app_echo_listen(int res, void * user_data)
{
  printf("listen: res = %d\n", res);
  app_conn_t * conn = user_data;

  if (ks_ret_is_err(res))
  {
    printf("conn destroy by listen\n");
    app_conn_destroy(conn);
    return;
  }

  ks_tcp_read_some(&conn->tcp, conn->rx_buffer, RX_BUFFER_SIZE, app_echo_repeat, conn);
}

static void app_echo_repeat(int res, void * user_data)
{
  printf("repeat: res = %d\n", res);
  app_conn_t * conn = user_data;

  if (ks_ret_is_err(res) || res == 0)
  {
    printf("conn destroy by repeat: %d\n", res);
    app_conn_destroy(conn);
    return;
  }

  ks_tcp_write(&conn->tcp, conn->rx_buffer, res, app_echo_listen, conn);
}

static void app_echo_accept(int res, void * user_data)
{
  printf("accept: res = %d\n", res);
  ks_tcp_conn_t * tcp_temp = user_data;

  if (!ks_ret_is_err(res))
  {
    app_conn_t * conn = app_conn_create(tcp_temp);
    app_echo_listen(0 /* initiate */, conn);
  }

  // STYLE ISSUE:
  // two places call `ks_tcp_accept()`
  ks_tcp_accept(&m_acceptor, tcp_temp, app_echo_accept, tcp_temp);
}

static void launch_server(void * user_data)
{
  printf("launch\n");
  static ks_tcp_conn_t tcp_temp;
  ks_io_cb_t on_conn = user_data;

  ks_tcp_accept(&m_acceptor, &tcp_temp, on_conn, &tcp_temp);
}

static void runtime(void)
{
  pthread_t threads[WORKERS_NUMBER];

  for (size_t i = 0; i < WORKERS_NUMBER; i++)
  {
    pthread_create(&threads[i], NULL, worker_routine, NULL);
  }

  for (size_t i = 0; i < WORKERS_NUMBER; i++)
  {
    pthread_join(threads[i], NULL);
  }
}

int main(void)
{
  m_acceptor = (ks_tcp_acceptor_t)
  {
    .addr.ip   = KS_IPv4(0, 0, 0, 0),
    .addr.port = 8080,
  };

  ks_tcp_init(&m_acceptor);
  ks_post(KS_WORK(launch_server, app_echo_accept));

  runtime();
}

static void * worker_routine(void * context)
{
  while (ks_run(KS_RUN_ONCE))
    ;

  ks_close();
  return NULL;
}
