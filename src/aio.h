#include <liburing.h>
#include <string.h>

#include "ks/io.h"
#include "ks/ret.h"
#include "macros.h"

// ISSUE: io_uring supports multishot accept (since 5.19) and multishot recv
//        (since 6.0), which potentially work better for ks_tcp_read() and
//        (in potential future) multishot version of ks_tcp_accept().
//
//        See https://github.com/axboe/liburing/wiki/io_uring-and-networking-in-2023
//
//        Currently ks_tcp_read() that receives exact number of bytes works
//        via registering subsequenct SQE in case of a partial read
//        in the immediate callback.

typedef struct
{
  struct io_uring io_uring;
} ks_aio_t;

typedef enum
{
  KS_AIO_OP_CODE_READ   = IORING_OP_RECV,
  KS_AIO_OP_CODE_WRITE  = IORING_OP_SEND,
  KS_AIO_OP_CODE_ACCEPT = IORING_OP_ACCEPT,
} ks_aio_op_code_t;

typedef struct
{
  int      fd;
  void   * buffer;
  size_t   bytes;
} ks_aio_issue_t;

typedef struct
{
  ks_aio_op_code_t opcode;
  ks_aio_issue_t   issue_data;
} ks_aio_op_t;

#define KS_AIO_OP_WRITE(fd_, buffer_, bytes_) \
  (ks_aio_op_t)                               \
  {                                           \
    .opcode = KS_AIO_OP_CODE_WRITE,           \
    .issue_data = { fd_,                      \
                    buffer_,                  \
                    bytes_ }                  \
  }

#define KS_AIO_OP_READ(fd_, buffer_, bytes_) \
  (ks_aio_op_t)                              \
  {                                          \
    .opcode = KS_AIO_OP_CODE_READ,           \
    .issue_data = { fd_,                     \
                    buffer_,                 \
                    bytes_ }                 \
  }

#define KS_AIO_OP_ACCEPT(fd_)        \
  (ks_aio_op_t)                      \
  {                                  \
    .opcode = KS_AIO_OP_CODE_ACCEPT, \
    .issue_data = { fd_ }            \
  }

typedef enum
{
  KS_AIO_POLL_STOP,
  KS_AIO_POLL_CONTINUE,
} ks_aio_poll_res_t;

#define KS_AIO_POLLING_DERIVED_MAX_SIZE 64
#define KS_AIO_STATIC_ASSERT_EXTENDED_POLLING_TYPE(type) \
  KS_STATIC_ASSERT_TYPE_OR_EXTENDED(*(type *)0, ks_aio_polling_base_t, aio);

typedef struct ks_aio_polling_base_s ks_aio_polling_base_t;

typedef ks_aio_poll_res_t (* ks_aio_poll_cb_t)(ks_aio_polling_base_t * self,
                                               int                     res,
                                               ks_aio_op_t           * p_op);

struct ks_aio_polling_base_s
{
  ks_aio_poll_cb_t poll;
};

ks_ret_t ks_aio_request(ks_aio_t                    * aio,
                        const ks_aio_op_t           * op,
                        const ks_aio_polling_base_t * polling,
                        size_t                        polling_size);

void ks_aio_init(ks_aio_t * aio, int eventfd);
void ks_aio_close(ks_aio_t * aio);

int ks_aio_poll(ks_aio_t * aio);
