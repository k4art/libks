#ifndef KS_TASK_H
#define KS_TASK_H

typedef struct ks_task_s ks_task_t;
typedef void (* ks_task_cb_wrapper_t)(ks_task_t * task);

struct ks_task_s
{
  ks_task_cb_wrapper_t    cb_wrapper;
  void                 (* user_cb)();
  void                  * user_data;
  int                     io_res;
};

#endif
