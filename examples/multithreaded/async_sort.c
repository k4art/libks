#include <stdio.h>
#include <stdatomic.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "ks.h"

typedef struct
{
  size_t size;
  long   data[];
} longs_t;

static void * worker_routine(void * context)
{
  while (ks_run(KS_RUN_ONCE_OR_DONE) == 1)
    ;

  ks_close();

  return NULL;
}

typedef struct
{
  ks_work_t      then_work;
  atomic_uint  * async_left;
  long         * beginning;
  long         * ending;
} sorting_ctx_t;

static void async_quicksort_recurse(void * context)
{
  /* SAFETY:
   * `ctx->async_left` indicates the number of contexts allocated on heap
   * (besides the number of not finished async calls of this function).
   * So each malloc() increments it, while free() decrements it */
  sorting_ctx_t * ctx = context;

  /* The local variables hold the context for left recursive calls.  */
  long * beg_left = ctx->beginning;
  long * end_left = ctx->ending;

  while (end_left - beg_left > 1)
  {
    long   pivot = *beg_left;
    long * sep   = beg_left + 1;

    for (long * ptr = beg_left + 1; ptr != end_left; ptr++)
    {
      long temp = *ptr;

      if (temp < pivot)
      {
        *ptr = *sep;
        *sep = temp;

        sep++;
      }
    }

    sep--;

    *beg_left = *sep;
    *sep      = pivot;

    // Allocate context for the right recursive call in heap
    sorting_ctx_t * ctx_right = malloc(sizeof(sorting_ctx_t));
    memcpy(ctx_right, ctx, sizeof(*ctx));
    
    ctx_right->beginning = sep + 1;
    ctx_right->ending    = end_left;

    end_left = sep + 1;

    // SAFETY:
    // During execution of this call, it is guaranteed
    // that any other async calls cannot be the last.
    // These increments will be published to other workers once
    // this call exits (atomically) assuming it is not the last one.
    atomic_fetch_add_explicit(ctx->async_left, 1, memory_order_relaxed);
    ks_post(KS_WORK(async_quicksort_recurse, ctx_right));

    // Continue sorting left part on the same stack frame
    // avoiding extra recursive calls and heap allocations.
  }

  if (atomic_fetch_sub_explicit(ctx->async_left, 1, memory_order_acq_rel) == 1)
  {
    // Under this condition, this is the end of the last async recursive call.
    ks_post(ctx->then_work);
    free(ctx->async_left);
  }

  free(ctx);
}

static void async_sort(longs_t * array, ks_work_cb_t cb, void * user_data)
{
  sorting_ctx_t * ctx      = malloc(sizeof(sorting_ctx_t));
  atomic_uint   * work_arc = malloc(sizeof(atomic_uint));

  *work_arc = 1;  // no synchronization needed

  ctx->then_work  = KS_WORK(cb, user_data);
  ctx->async_left = work_arc;
  ctx->beginning  = array->data;
  ctx->ending     = array->data + array->size;
  
  ks_post(KS_WORK(async_quicksort_recurse, ctx));
}

static void input_array(longs_t ** p_array)
{
  longs_t * temp;
  ssize_t     no_elems_in;

  fflush(stdout);
  KS_RET_CHECKED(scanf("%zd", &no_elems_in));

  if (no_elems_in <= 0)
  {
    printf("No. Bad input. "
           "Consider entering positive number of elements next time.\n");
    exit(0);
  }

  temp = malloc(sizeof(longs_t) + no_elems_in * sizeof(long));
  temp->size = no_elems_in;

  for (size_t i = 0; i < no_elems_in; i++)
  {
    KS_RET_CHECKED(scanf("%ld", &temp->data[i]));
  }

  *p_array = temp;
}

static void output_and_free_array_cb(void * user_data)
{
  longs_t * array = user_data;

  for (size_t i = 0; i < array->size; i++)
  {
    if (i != 0) putchar(' ');

    printf("%ld", array->data[i]);
  }

  printf("\n");
  free(array);
}

int main(int argc, char ** argv)
{
  if (argc != 2)
  {
    printf("Usage: %s <threads no.>\n", argv[0]);
    return 0;
  }

  size_t      threads_no = atoi(argv[1]);
  pthread_t * threads    = malloc(threads_no * sizeof(pthread_t));

  longs_t * array;
  
  input_array(&array);
  async_sort(array, output_and_free_array_cb, array);

  for (size_t i = 0; i < threads_no; i++)
  {
    pthread_create(&threads[i], NULL, worker_routine, NULL);
  }
 
  for (size_t i = 0; i < threads_no; i++)
  {
    pthread_join(threads[i], NULL);
  }

  free(threads);
}
