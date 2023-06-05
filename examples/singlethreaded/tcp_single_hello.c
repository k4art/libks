#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "ks.h"

#define HELLO_MESSAGE "Hello"

static void close_tcp_io_cb(int result, void * user_data)
{
  ks_tcp_conn_t * tcp = user_data;

  if (result != sizeof(HELLO_MESSAGE))
  {
    fprintf(stderr, "Error: message sent was impaired\n");
    exit(1);
  }

  ks_tcp_close(tcp);
}

static void say_hello_io_cb(int result, void * user_data)
{
  ks_tcp_conn_t * tcp = user_data;

  if (result < 0)
  {
    fprintf(stderr, "Error: async accept failed with %s\n", strerror(errno));
    exit(1);
  }

  ks_tcp_write(tcp, HELLO_MESSAGE, sizeof(HELLO_MESSAGE), close_tcp_io_cb, tcp);
}

int main(void)
{
  ks_tcp_conn_t     single_conn;
  ks_tcp_acceptor_t acceptor =
  {
    .addr.ip   = KS_IPv4(127, 0, 0, 1),
    .addr.port = 8080,
  };

  ks_tcp_init(&acceptor);
  ks_tcp_accept(&acceptor, &single_conn, say_hello_io_cb, &single_conn);

  while (ks_run(KS_RUN_ONCE_OR_DONE) == 1)
    ;

  ks_tcp_close(&acceptor);
  ks_close();
}
