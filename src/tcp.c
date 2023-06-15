#include <errno.h>
#include <unistd.h>

#include "worker.h"
#include "ks.h"
#include "ks/alloc.h"
#include "ks/tcp.h"

#define BACKLOG 512

static ks_ret_t ks__tcp_bind_socket(int sockfd, const ks_ipv4_addr_t * addr)
{
  assert(sockfd > 0);
  assert(addr);
  
  struct sockaddr_in sockaddr;

  ks_ipv4_to_os_sockaddr(addr, &sockaddr);

  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0)
    return -errno;

  if (bind(sockfd, (struct sockaddr *) &sockaddr, sizeof(sockaddr)) < 0)
    return -errno;

  if (listen(sockfd, BACKLOG) < 0)
    return -errno;

  return KS_ESUCCESS;
}

ks_ret_t ks__tcp_init(ks__tcp_t * tcp, int reuse_sockfd, const ks_ipv4_addr_t * binding_addr)
{
  int sockfd;

  if (!tcp)
    return KS_EINVAL;

  if (binding_addr && !ks_ipv4_addr_is_valid(binding_addr))
    return KS_EINVAL;

  if (reuse_sockfd < 0)
    return KS_EINVAL;

  if (reuse_sockfd == KS__TCP_REUSE_SOCKFD)
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
  else
    sockfd = reuse_sockfd;

  if (sockfd < 0)
    return -errno;

  if (binding_addr)
    ks__tcp_bind_socket(sockfd, binding_addr);

  tcp->fd = sockfd;
  
  ks__inform_handle_init();
  return KS_ESUCCESS;
}

ks_ret_t ks__tcp_close(ks__tcp_t * tcp)
{
  printf("close fd: %d\n", tcp->fd);
  if (!tcp)
    return KS_EINVAL;

  ks_ret_asserting(close(tcp->fd) == 0);

  ks__inform_handle_close();
  return KS_ESUCCESS;
}

/***************************************************/
/******************** TCP Accept *******************/
/***************************************************/

typedef struct 
{
  ks_aio_polling_base_t   aio;
  ks_tcp_conn_t         * p_conn;
  ks_io_cb_t              cb;
  void                  * user_data;
} ks__tcp_accept_polling_t;

KS_AIO_STATIC_ASSERT_EXTENDED_POLLING_TYPE(ks__tcp_accept_polling_t);

static ks_aio_poll_res_t ks__tcp_accept_poll(ks_aio_polling_base_t * self,
                                             int                     res,
                                             ks_aio_op_t           * p_op)
{
  assert(self);
  assert(p_op);
  
  ks__tcp_accept_polling_t * polling = (void *) self;

  if (res > 0)
    ks_tcp_init_reuse_socket(polling->p_conn, res);

  ks_post(KS_IO_WORK(polling->cb, res, polling->user_data));

  return KS_AIO_POLL_STOP;
}

ks_ret_t ks_tcp_accept(ks_tcp_acceptor_t * acceptor,
                       ks_tcp_conn_t     * p_conn,
                       ks_io_cb_t          cb,
                       void              * user_data)
{
  if (!acceptor) return KS_EINVAL;
  if (!p_conn)   return KS_EINVAL;

  ks__tcp_accept_polling_t polling =
  {
    .aio.poll  = ks__tcp_accept_poll,
    .p_conn    = p_conn,
    .cb        = cb,
    .user_data = user_data,
  };

  return ks_worker_aio_request(&KS_AIO_OP_ACCEPT(acceptor->socket.fd),
                               (ks_aio_polling_base_t *) &polling,
                               sizeof(polling));
}

/***************************************************/
/************* TCP Read/Write (Common) *************/
/***************************************************/

typedef struct
{
  ks_aio_polling_base_t   aio;
  ks_tcp_conn_t         * conn;
  void                  * buffer;
  size_t                  nbytes;
  size_t                  bytes_left;
  ks_io_cb_t              cb;
  void                  * user_data;
  ks_aio_op_code_t        opcode;
} ks__tcp_rw_polling_t;

KS_AIO_STATIC_ASSERT_EXTENDED_POLLING_TYPE(ks__tcp_rw_polling_t);

static ks_aio_poll_res_t ks__tcp_rw_poll(ks_aio_polling_base_t * self,
                                           int                   res,
                                           ks_aio_op_t         * p_op)
{
  assert(self);
  assert(p_op);
  
  ks__tcp_rw_polling_t * polling = (void *) self;

  if (res < 0)
  {
    ks_post(KS_IO_WORK(polling->cb, res, polling->user_data));
    return KS_AIO_POLL_STOP;
  }
  else if (polling->bytes_left == res)
  {
    ks_post(KS_IO_WORK(polling->cb, polling->nbytes, polling->user_data));
    return KS_AIO_POLL_STOP;
  }
  else
  {
    polling->buffer = (char *)polling->buffer + res;
    polling->bytes_left -= res;

    *p_op = (ks_aio_op_t)
    {
      .opcode            = polling->opcode,
      .issue_data.fd     = polling->conn->socket.fd,
      .issue_data.buffer = polling->buffer,
      .issue_data.bytes  = polling->bytes_left,
    };

    return KS_AIO_POLL_CONTINUE;
  }
}

typedef struct
{
  ks_aio_polling_base_t   aio;
  ks_io_cb_t              cb;
  void                  * user_data;
} ks__tcp_rw_some_polling_t;

KS_AIO_STATIC_ASSERT_EXTENDED_POLLING_TYPE(ks__tcp_accept_polling_t);

static ks_aio_poll_res_t ks__tcp_rw_some_poll(ks_aio_polling_base_t * self,
                                              int                     res,
                                              ks_aio_op_t           * p_op)
{
  ks__tcp_rw_some_polling_t * polling = (void *) self;

  ks_post(KS_IO_WORK(polling->cb, res, polling->user_data));
  return KS_AIO_POLL_STOP;
}

/***************************************************/
/******************** TCP Read *********************/
/***************************************************/

ks_ret_t ks_tcp_read(ks_tcp_conn_t * conn,
                     void          * buffer,
                     size_t          nbytes,
                     ks_io_cb_t      cb,
                     void          * user_data)
{
  if (!conn)   return KS_EINVAL;
  if (!buffer) return KS_EINVAL;
  if (!nbytes) return KS_EINVAL;

  ks__tcp_rw_polling_t polling =
  {
    .aio.poll   = ks__tcp_rw_poll,
    .conn       = conn,
    .buffer     = buffer,
    .nbytes     = nbytes,
    .bytes_left = nbytes,
    .cb         = cb,
    .user_data  = user_data,
    .opcode     = KS_AIO_OP_CODE_READ,
  };

  return ks_worker_aio_request(&KS_AIO_OP_READ(conn->socket.fd, buffer, nbytes),
                               (ks_aio_polling_base_t *) &polling,
                               sizeof(polling));
}

ks_ret_t ks_tcp_read_some(ks_tcp_conn_t * conn,
                          void          * buffer,
                          size_t          nbytes,
                          ks_io_cb_t      cb,
                          void          * user_data)
{
  if (!conn)   return KS_EINVAL;
  if (!buffer) return KS_EINVAL;
  if (!nbytes) return KS_EINVAL;

  ks__tcp_rw_some_polling_t polling =
  {
    .aio.poll  = ks__tcp_rw_some_poll,
    .cb        = cb,
    .user_data = user_data,
  };

  return ks_worker_aio_request(&KS_AIO_OP_READ(conn->socket.fd, buffer, nbytes),
                               (ks_aio_polling_base_t *) &polling,
                               sizeof(polling));
}

/***************************************************/
/******************* TCP Write *********************/
/***************************************************/

ks_ret_t ks_tcp_write(ks_tcp_conn_t * conn,
                      void          * buffer,
                      size_t          nbytes,
                      ks_io_cb_t      cb,
                      void          * user_data)
{
  if (!conn)   return KS_EINVAL;
  if (!buffer) return KS_EINVAL;
  if (!nbytes) return KS_EINVAL;

  ks__tcp_rw_polling_t polling =
  {
    .aio.poll   = ks__tcp_rw_poll,
    .conn       = conn,
    .buffer     = buffer,
    .nbytes     = nbytes,
    .bytes_left = nbytes,
    .cb         = cb,
    .user_data  = user_data,
    .opcode     = KS_AIO_OP_CODE_WRITE,
  };

  return ks_worker_aio_request(&KS_AIO_OP_WRITE(conn->socket.fd, buffer, nbytes),
                               (ks_aio_polling_base_t *) &polling,
                               sizeof(polling));
}

ks_ret_t ks_tcp_write_some(ks_tcp_conn_t * conn,
                           void          * buffer,
                           size_t          nbytes,
                           ks_io_cb_t      cb,
                           void          * user_data)
{
  if (!conn)   return KS_EINVAL;
  if (!buffer) return KS_EINVAL;
  if (!nbytes) return KS_EINVAL;

  ks__tcp_rw_some_polling_t polling =
  {
    .aio.poll  = ks__tcp_rw_some_poll,
    .cb        = cb,
    .user_data = user_data,
  };

  return ks_worker_aio_request(&KS_AIO_OP_WRITE(conn->socket.fd, buffer, nbytes),
                               (ks_aio_polling_base_t *) &polling,
                               sizeof(polling));
}
