#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "ks.h"

typedef struct 
{
  ks_tcp_conn_t tcp;
  size_t        rx_buffer_length;
  char          rx_buffer[2048];
} user_conn_t;

static ks_tcp_acceptor_t m_acceptor;

static void on_write_io_cb(int write_res, void * user_data)
{
  user_conn_t * conn = user_data;

  if (write_res < 0)
  {
    ks_error("Error: connection failed with %s on write", strerror(errno));
    exit(1);
  }

  if (write_res != conn->rx_buffer_length)
  {
    ks_error("Error: message sent was impaired");
    exit(1);
  }

  ks_tcp_close(&conn->tcp);
  free(conn);

  ks_tcp_close(&m_acceptor);
}

static void on_read_io_cb(int read_res, void * user_data)
{
  user_conn_t * conn = user_data;

  if (read_res < 0)
  {
    ks_error("Error: connection failed with %s on read", strerror(errno));
    exit(1);
  }
  else if (read_res == 0)
  {
    ks_error("Error: unexpected disconnect");
    exit(1);
  }

  conn->rx_buffer_length = read_res;
  ks_tcp_write(&conn->tcp, conn->rx_buffer, read_res, on_write_io_cb, conn);
}

static void on_connection_io_cb(int result, void * user_data)
{
  if (result < 0)
  {
    ks_error("Error: async accept failed with %s", strerror(errno));
    exit(1);
  }

  ks_tcp_conn_t * tcp_temp = user_data;
  user_conn_t   * conn     = malloc(sizeof(user_conn_t));

  memcpy(&conn->tcp, tcp_temp, sizeof(*tcp_temp));

  ks_tcp_read_some(tcp_temp,
                   conn->rx_buffer,
                   sizeof(conn->rx_buffer) - 1,
                   on_read_io_cb,
                   conn);
}

int main(void)
{
  ks_tcp_conn_t     tcp_temp;
  m_acceptor = (ks_tcp_acceptor_t)
  {
    .addr.ip   = KS_IPv4(127, 0, 0, 1),
    .addr.port = 8080,
  };

  ks_tcp_init(&m_acceptor);
  ks_tcp_accept(&m_acceptor, &tcp_temp, on_connection_io_cb, &tcp_temp);

  while (ks_run(KS_RUN_ONCE_OR_DONE))
    ;

  ks_close();
}
