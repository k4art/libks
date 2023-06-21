#include <pthread.h>
#include <string.h>
#include <signal.h>

#include "ks.h"
#include "ks/alloc.h"
#include "ks/log/thlog.h"

#define RX_BUFFER_SIZE 1024
#define WORKERS_NUMBER 1

#define HTTP_HELLO_RESPONSE \
  "HTTP/1.1 200 OK\r\nContent-Length: 11\r\nConnection: close\r\n\r\nHello World"

#define HTTP_HELLO_SIZE (sizeof(HTTP_HELLO_RESPONSE) - 1)

#define WAIT_SIGNAL(signum_)                                       \
  do {                                                             \
    sigset_t signal_set;                                           \
    int      signum;                                               \
    sigemptyset(&signal_set);                                      \
    sigaddset(&signal_set, signum_);                               \
    KS_RET_CHECKED(pthread_sigmask(SIG_BLOCK, &signal_set, NULL)); \
    sigwait(&signal_set, &signum);                                 \
    assert(signum == signum_);                                     \
  } while (0)


static ks_tcp_acceptor_t m_acceptor;

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

static void * worker_routine(void * context);

static void app_http_close(int res, void * user_data)
{
	app_conn_t * conn = user_data;
  thlog("close %d", conn->tcp.socket.fd);

	app_conn_destroy(conn);
}

static void app_http_send_hello(int res, void * user_data)
{
	app_conn_t * conn = user_data;

  thlog("write %d", conn->tcp.socket.fd);

  if (ks_ret_is_err(res))
  {
    app_conn_destroy(conn);
    return;
  }

  ks_tcp_write(&conn->tcp,
               HTTP_HELLO_RESPONSE,
               HTTP_HELLO_SIZE,
               app_http_close,
               conn);
}

static void app_http_read_request(void * user_data)
{
  app_conn_t * conn = user_data;

  thlog("read %d", conn->tcp.socket.fd);

  ks_tcp_read_some(&conn->tcp,
                   conn->rx_buffer,
                   RX_BUFFER_SIZE,
                   app_http_send_hello,
                   user_data);
}

static void app_accept(int res, void * user_data)
{
  thlog("accept received %d", res);

  ks_tcp_conn_t * tcp_temp = user_data;

  ks_tcp_accept(&m_acceptor, tcp_temp, app_accept, tcp_temp);

  if (!ks_ret_is_err(res))
  {
    app_conn_t * conn = app_conn_create(tcp_temp);
    app_http_read_request(conn);
  }
}

static void launch_server(void * user_data)
{
  static ks_tcp_conn_t tcp_temp;
  ks_io_cb_t on_conn = user_data;

  ks_tcp_init(&m_acceptor);
  ks_tcp_accept(&m_acceptor, &tcp_temp, on_conn, &tcp_temp);
}

static void runtime(void)
{
  pthread_t threads[WORKERS_NUMBER];

  for (size_t i = 0; i < WORKERS_NUMBER; i++)
  {
    pthread_create(&threads[i], NULL, worker_routine, NULL);
  }

  WAIT_SIGNAL(SIGINT);
  ks_stop();

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

  ks_post(KS_WORK(launch_server, app_accept));

  runtime();
}

static void * worker_routine(void * context)
{
  while (ks_run(KS_RUN_ONCE))
    ;

  ks_close();
  return NULL;
}
