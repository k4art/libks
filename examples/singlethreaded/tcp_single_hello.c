#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "ks.h"

#define HELLO_MESSAGE "hello"

static ks_tcp_acceptor_t m_acceptor;

static void done_io_cb(int result, void * user_data)
{
  ks_tcp_conn_t * tcp = user_data;

  if (result != strlen(HELLO_MESSAGE))
  {
    ks_error("Error: message sent was impaired (%d out of %zu)",
             result,
             strlen(HELLO_MESSAGE));

    exit(1);
  }

  ks_tcp_close(tcp);
  ks_tcp_close(&m_acceptor);
}

static void say_hello_io_cb(int result, void * user_data)
{
  ks_tcp_conn_t * tcp = user_data;

  if (result < 0)
  {
    ks_error("Error: async accept failed with %s", strerror(errno));
    exit(1);
  }

  ks_tcp_write(tcp, HELLO_MESSAGE, strlen(HELLO_MESSAGE), done_io_cb, tcp);
}

int main(void)
{
  ks_tcp_conn_t single_conn;

  m_acceptor = (ks_tcp_acceptor_t)
  {
    .addr.ip   = KS_IPv4(127, 0, 0, 1),
    .addr.port = 8080,
  };

  KS_RET_CHECKED(ks_tcp_init(&m_acceptor));
  KS_RET_CHECKED(ks_tcp_accept(&m_acceptor,
                               &single_conn,
                               say_hello_io_cb,
                               &single_conn));

  while (ks_run(KS_RUN_ONCE_OR_DONE))
    ;

  ks_close();
}
