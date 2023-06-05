#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "ks.h"

#define RX_BUFFER_SIZE 1024

typedef struct
{
  ks_tcp_conn_t tcp;
  size_t        rx_offset;
  char          rx_buffer[RX_BUFFER_SIZE];
} user_conn_t;

static ks_tcp_acceptor_t * m_acceptor;
static user_conn_t       * m_conns[2048]; // maps: FD -> user_conn_t *

static void on_read_cb(int result, void * user_data);

static void convert_pings_to_pongs(char   * buffer,
                                   size_t   buffer_len,
                                   size_t * p_skipped_last)
{
  char * ping    = buffer;
  char * buf_end = buffer + buffer_len;

  while ((ping = strstr(ping, "PING")) != NULL)
  {
    ping[1] = 'O'; // PING -> PONG
  }

  if      (!strcmp(buf_end - 1, "P"))   *p_skipped_last = 1;
  else if (!strcmp(buf_end - 2, "PI"))  *p_skipped_last = 2;
  else if (!strcmp(buf_end - 3, "PIN")) *p_skipped_last = 3;
}

static void on_write_cb(int result, void * user_data)
{
  user_conn_t * conn = user_data;
  
  if (result <= 0)
  {
    ks_tcp_close(&conn->tcp);
    m_conns[conn->tcp.socket.fd] = NULL;
    free(conn);
  }
  else
  {
    size_t tx_length = result;
    size_t skipped   = strlen(conn->rx_buffer + tx_length);

    assert(skipped < sizeof("PING"));

    if (skipped > 0)
    {
      char temp[4];
      memcpy(temp, conn->rx_buffer + tx_length, skipped);
      memcpy(conn->rx_buffer, temp, skipped);
    }

    conn->rx_offset = skipped;
    
    ks_tcp_read_some(&conn->tcp,
                     conn->rx_buffer + skipped,
                     RX_BUFFER_SIZE,
                     on_read_cb,
                     conn);
  }
}

static void on_read_cb(int result, void * user_data)
{
  user_conn_t * conn = user_data;
  
  if (result <= 0)
  {
    ks_tcp_close(&conn->tcp);
    m_conns[conn->tcp.socket.fd] = NULL;
    free(conn);
  }
  else
  {
    size_t skipped, rx_length = result + conn->rx_offset;

    convert_pings_to_pongs(conn->rx_buffer, rx_length, &skipped);

    ks_tcp_write(&conn->tcp,
                 conn->rx_buffer,
                 rx_length - skipped,
                 on_write_cb,
                 conn);
  }
}

static void async_accept(ks_tcp_acceptor_t * acceptor, ks_io_cb_t cb)
{
  static ks_tcp_conn_t tcp_temp;

  ks_tcp_accept(acceptor, &tcp_temp, cb, &tcp_temp);
}

static void on_accept_cb(int result, void * user_data)
{
  if (result < 0)
  {
    fprintf(stderr, "Error: async accept failed with %s\n", strerror(errno));
  }
  else
  {
    ks_tcp_conn_t * tcp_temp = user_data;
    user_conn_t   * conn     = malloc(sizeof(user_conn_t));

    conn->rx_offset = 0;
    memcpy(&conn->tcp, tcp_temp, sizeof(*tcp_temp));
    
    m_conns[conn->tcp.socket.fd] = conn;
    ks_tcp_read_some(&conn->tcp,
                     conn->rx_buffer,
                     RX_BUFFER_SIZE,
                     on_read_cb,
                     conn);
  }

  async_accept(m_acceptor, on_accept_cb);
}

static void cleanup_conns(void)
{
  for (size_t i = 0; i < sizeof(m_conns) / sizeof(m_conns[0]); i++)
  {
    free(m_conns[i]);
    m_conns[i] = NULL;
  }
}

static void ctrl_c_handler(int signum)
{
  ks_stop();
  signal(SIGINT, SIG_DFL);
}

int main(void)
{
  m_acceptor = &(ks_tcp_acceptor_t)
  {
    .addr.ip   = KS_IPv4(127, 0, 0, 1),
    .addr.port = 8080,
  };

  ks_tcp_init(m_acceptor);
  async_accept(m_acceptor, on_accept_cb);

  signal(SIGINT, ctrl_c_handler);

  while (ks_run(KS_RUN_ONCE) == 1)
    ;

  cleanup_conns();
  ks_tcp_close(m_acceptor);
  ks_close();
}
