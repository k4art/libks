#include <signal.h>
#include <stdlib.h>

#include "ks.h"

typedef struct
{
  ks_tcp_conn_t tcp;
} app_conn_t;

static char                m_blackhole[1028];
static app_conn_t       * m_conns[2048];     // maps: FD -> user_conn_t *
static ks_tcp_acceptor_t * m_acceptor;

static void spaghettify_cb(int res, void * user_data);
static void past_event_horizon_cb(int accept_res, void * user_data);

static void accept_conn(ks_tcp_acceptor_t * acceptor, ks_io_cb_t cb)
{
  app_conn_t * conn = malloc(sizeof(app_conn_t));
  
  ks_tcp_accept(acceptor, &conn->tcp, cb, conn);
}

static void spaghettify(app_conn_t * conn)
{
  ks_tcp_read_some(&conn->tcp, m_blackhole, sizeof(m_blackhole), spaghettify_cb, conn);
}

static void spaghettify_cb(int res, void * user_data)
{
  app_conn_t * conn = user_data;

  if (res <= 0)
  {
    spaghettify(conn);
  }
  else
  {
    ks_tcp_close(&conn->tcp);
    m_conns[conn->tcp.socket.fd] = NULL;
    free(conn);
  }
}

static void past_event_horizon_cb(int accept_res, void * user_data)
{
  app_conn_t * conn = user_data;
  
  if (accept_res == 0)
  {
    m_conns[conn->tcp.socket.fd] = conn;
    spaghettify(conn);
  }
  else
  {
    free(conn);
  }

  accept_conn(m_acceptor, past_event_horizon_cb);
}

static void ctrl_c_handler(int signum)
{
  ks_stop();
  signal(SIGINT, SIG_DFL); // next ^C terminates the process
}

static void cleanup_conns(void)
{
  for (size_t i = 0; i < sizeof(m_conns) / sizeof(m_conns[0]); i++)
  {
    free(m_conns[i]);
    m_conns[i] = NULL;
  }
}

int main(void)
{
  m_acceptor = &(ks_tcp_acceptor_t)
  {
    .addr.ip   = KS_IPv4(127, 0, 0, 1),
    .addr.port = 8080,
  };

  ks_tcp_init(m_acceptor);
  accept_conn(m_acceptor, past_event_horizon_cb);

  signal(SIGINT, ctrl_c_handler);   // ctrl_c_handler calls ks_stop(),
                                    // this will stop the loop
  while (ks_run(KS_RUN_ONCE))
    ;

  cleanup_conns();                  // after that release all resources
  ks_tcp_close(m_acceptor);
  ks_close();
}
