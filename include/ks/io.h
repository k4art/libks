/**
 * @defgroup io IO
 * @{
 * @brief IO
 */

#ifndef KS_IO_H
#define KS_IO_H

typedef void (* ks_io_cb_t)(int result, void * user_data);

/**
 * @brief       IO work that is supposed to be associated with
 *              an IO operation and be called once the operation is compeleted.
 */
typedef struct
{
  ks_io_cb_t   cb;
  int          res;
  void       * user_data;
} ks_io_work_t;

/**
 * @brief       Builds `ks_io_work_t`.
 *
 * @param[in]   fn_
 * @param[in]   res_
 * @param[in]   user_data_
 */
#define KS_IO_WORK(fn_, res_, user_data_)                                      \
  (ks_io_work_t) { .fn = fn_, .res = res_, .user_data = user_data_ }

#endif

/** @} */
