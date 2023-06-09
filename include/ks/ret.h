#ifndef KS_RET_H
#define KS_RET_H

#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define KS__ERR_MAP(XX)                                         \
  XX(ESUCCESS,      (0),              "success")                \
  XX(EINVAL,        (-EINVAL),        "invalid argument")       \
  XX(EADDRINUSE,    (-EADDRINUSE),    "address already in use") \
  XX(EADDRNOTAVAIL, (-EADDRNOTAVAIL), "address not available")  \
  XX(ENOMEM,        (-ENOMEM),        "ran out of space")  \

#define ks__error(format, ...) fprintf(stderr, format "\n", ## __VA_ARGS__)

/**
 * @brief       Aborts if the expression evaluates to an error.
 *
 * @param[in]   expr
 */
#ifndef NDEBUG
#define ks_ret_asserting(expr_) assert(expr_)
#define KS_RET_CHECKED(expr)                           \
  do {                                                 \
    ks_ret_t ret = (expr);                             \
    if (ret < 0)                                       \
    {                                                  \
      ks__error("KS_RET_CHECK() failure at %s:%s:%d\n" \
                "`%s` returned error: %s",             \
                __FILE__,                              \
                __FUNCTION__,                          \
                __LINE__,                              \
                #expr,                                 \
                ks_ret_name(ret));                     \
      abort();                                         \
    }                                                  \
  } while (0)
#else
#define ks_ret_asserting(expr_) (void) (expr_)

#define KS_RET_CHECKED(expr)                       \
  do {                                             \
    ks_ret_t ret = (expr);                         \
    if (ret < 0)                                   \
    {                                              \
      ks__error("KS_RET_CHECK() failed error: %s", \
                ks_ret_name(ret));                 \
      abort();                                     \
    }                                              \
  } while (0)
#endif

/**
 * @brief       Enumerates error codes, all of which are negatives of
 *              corresponding errno values.
 */
typedef enum
{
  #define XX(name, code, message) KS_ ## name = code,
  KS__ERR_MAP(XX)
  #undef  XX
} ks_ret_t;

#define ks_ret_is_err(ret_) ((ret_) < 0)

/**
 * @brief       Returns an immutable static-lifetime reference to
 *              the string representation of an error code.
 *
 * @param[in]   ret
 *
 * @return      immutable static-lifetime reference to the name
 */
const char * ks_ret_name(ks_ret_t ret);

/**
 * @brief       Fills the buffer with the string representation of
 *              the given error code.
 *
 * @param[out]  p_buffer
 * @param[in]   buflen
 *
 * @return      the value of the parameter `p_buffer`
 */
char * ks_ret_name_r(ks_ret_t ret, char * p_buffer, size_t buflen);

#endif
