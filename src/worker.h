#ifndef KS_WORKER_H
#define KS_WORKER_H

#include "aio.h"
#include "shared.h"

#include "ks.h"

void ks_worker_init(ks_shared_t * shared);
bool ks_worker_is_init(void);
void ks_worker_close(void);

ks_ret_t ks_worker_aio_request(const ks_aio_op_t           * op,
                               const ks_aio_polling_base_t * polling,
                               size_t                        polling_size);

void ks_worker_incrment_handles_count(void);
void ks_worker_decrement_handles_count(void);

void ks_worker_wait(void);
void ks_worker_post(const ks_wq_item_t * wq_item);

int ks_worker_poll(void);

#endif
