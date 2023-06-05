/**
 * @defgroup tcp TCP
 * @{
 * @brief Asynchronous TCP API
 */

#ifndef KS_TCP_H
#define KS_TCP_H

#include "ks/ret.h"
#include "ks/io.h"
#include "ks/ipv4.h"

#include <stddef.h>

typedef struct
{
  int fd;
} ks__tcp_t;

/**
 * @brief       TCP connection.
 */
typedef struct
{
  ks__tcp_t socket;
} ks_tcp_conn_t;

/**
 * @brief       TCP Acceptor.
 * @note        Only IPv4 is supported.
 */
typedef struct
{
  ks__tcp_t socket;
  ks_ipv4_addr_t addr;
} ks_tcp_acceptor_t;

ks_ret_t ks__tcp_init(ks__tcp_t * tcp, const ks_ipv4_addr_t * binding_addr);
ks_ret_t ks__tcp_close(ks__tcp_t * tcp);


#define ks_tcp_init(tcp)                                            \
  _Generic((tcp),                                                   \
   ks_tcp_conn_t     *: ks__tcp_init(&(tcp)->socket, NULL),         \
   ks_tcp_acceptor_t *: ks__tcp_init(&(tcp)->socket, &(tcp)->addr)) \

#define ks_tcp_close(tcp)                              \
  _Generic((tcp),                                      \
   ks_tcp_conn_t     *: ks__tcp_close(&(tcp)->socket), \
   ks_tcp_acceptor_t *: ks__tcp_close(&(tcp)->socket)) \

/**
 * @brief       Asynchronous TCP accept.
 * @note        At most one pending accept per acceptor must be active at any time.
 *
 * @param[in]   acceptor
 * @param[out]  p_socket
 * @param[in]   cb
 * @param[in]   user_data
 *
 * @retval      KS_EINVAL    The acceptor is not initialized.
 */
ks_ret_t ks_tcp_accept(ks_tcp_acceptor_t * acceptor,
                       ks_tcp_conn_t     * p_socket,
                       ks_io_cb_t      cb,
                       void              * user_data);

/**
 * @brief       Asynchronous TCP read of exact size.
 * @note        At most one pending read per TCP connection must be active at any time.
 *
 * @param[in]   conn
 * @param[out]  buffer
 * @param[in]   nbytes
 * @param[in]   cb
 * @param[in]   user_data
 *
 * @retval      KS_EINVAL  The connection is not initialized.
 */
ks_ret_t ks_tcp_read(ks_tcp_conn_t  * socket,
                     void           * buffer,
                     size_t           nbytes,
                     ks_io_cb_t       cb,
                     void           * user_data);

/**
 * @brief       Asynchronous TCP read.
 * @note        At most one pending read per TCP connection must be active at any time.
 *
 * @param[in]   conn
 * @param[out]  buffer
 * @param[in]   nbytes
 * @param[in]   cb
 * @param[in]   user_data
 *
 * @retval      KS_EINVAL  The connection is not initialized.
 * 
 * @section     ex Example
 * @snippet     examples/tcp_single_echo.c ks_tcp_read_some Example
 */
ks_ret_t ks_tcp_read_some(ks_tcp_conn_t  * conn,
                          void           * buffer,
                          size_t           nbytes,
                          ks_io_cb_t       cb,
                          void           * user_data);

/**
 * @brief       Asynchronous TCP write of exact size.
 * @note        At most one pending write per TCP connection must be active at any time.
 *
 * @param[in]   conn
 * @param[out]  buffer
 * @param[in]   nbytes
 * @param[in]   cb
 * @param[in]   user_data
 *
 * @retval      KS_EINVAL  The connection is not initialized.
 */
ks_ret_t ks_tcp_write(ks_tcp_conn_t  * conn,
                      void           * buffer,
                      size_t           nbytes,
                      ks_io_cb_t       cb,
                      void           * user_data);

/**
 * @brief       Asynchronous TCP write.
 * @note        At most one pending write per TCP connection must be active at any time.
 *
 * @param[in]   conn
 * @param[out]  buffer
 * @param[in]   nbytes
 * @param[in]   cb
 * @param[in]   user_data
 *
 * @retval      KS_EINVAL  The connection is not initialized.
 */
ks_ret_t ks_tcp_write_some(ks_tcp_conn_t  * conn,
                           void           * buffer,
                           size_t           nbytes,
                           ks_io_cb_t       cb,
                           void           * user_data);
#endif 

/** @} */
