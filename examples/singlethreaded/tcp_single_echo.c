#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "ks.h"

typedef struct 
{
  ks_tcp_conn_t tcp;
  char          rx_buffer[2048];
} user_conn_t;

static void on_write_io_cb(int write_res, void * user_data)
{
  user_conn_t * conn = user_data;

  if (write_res < 0)
  {
    fprintf(stderr, "Error: connection failed with %s on write\n", strerror(errno));
    exit(1);
  }

  if (write_res != strlen(conn->rx_buffer))
  {
    fprintf(stderr, "Error: message sent was impaired\n");
    exit(1);
  }

  ks_tcp_close(&conn->tcp);
  free(conn);
}

static void on_read_io_cb(int read_res, void * user_data)
{
  user_conn_t * conn = user_data;

  if (read_res < 0)
  {
    fprintf(stderr, "Error: connection failed with %s on read\n", strerror(errno));
    exit(1);
  }
  else if (read_res == 0)
  {
    fprintf(stderr, "Error: unexpected disconnect\n");
    exit(1);
  }

  ks_tcp_write(&conn->tcp, conn->rx_buffer, read_res, on_write_io_cb, conn);
}

static void on_connection_io_cb(int result, void * user_data)
{
  if (result < 0)
  {
    fprintf(stderr, "Error: async accept failed with %s\n", strerror(errno));
    exit(1);
  }

  ks_tcp_conn_t * tcp_temp = user_data;
  user_conn_t   * conn     = malloc(sizeof(user_conn_t));

  memcpy(&conn->tcp, tcp_temp, sizeof(*tcp_temp));

  ks_tcp_read_some(tcp_temp,
                   conn->rx_buffer,
                   sizeof(conn->rx_buffer),
                   on_read_io_cb,
                   conn);
}

int main(void)
{
  ks_tcp_conn_t     tcp_temp;
  ks_tcp_acceptor_t acceptor =
  {
    .addr.ip   = KS_IPv4(127, 0, 0, 1),
    .addr.port = 8080,
  };

  ks_tcp_init(&acceptor);
  ks_tcp_accept(&acceptor, &tcp_temp, on_connection_io_cb, &tcp_temp);

  // ks_tcp_accept(&acceptor, ...) is not called any more,
  // so that once a single connection is handled,
  // there will be no active handlds and
  // ks_run(..) will return 0 stopping the while loop
  while (ks_run(KS_RUN_ONCE_OR_DONE) == 1)
    ;

  ks_tcp_close(&acceptor);
  ks_close();
}
