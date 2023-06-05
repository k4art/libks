#include <stdio.h>
#include <stdatomic.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "ks.h"

static void * worker_routine(void * context)
{
  while (ks_run(KS_RUN_ONCE_OR_DONE) == 1)
    ;

  ks_close();

  return NULL;
}

typedef struct
{
  atomic_uint  * async_left;
  ks_task_t      then_task;
  double       * beginning;
  double       * ending;
} sorting_ctx_t;

static void async_sort_recurse(void * context)
{
  sorting_ctx_t * ctx = context;

  double * beginning_left = ctx->beginning;
  double * ending_left    = ctx->ending;

  if (ctx->beginning == ctx->ending)
  {
    if (atomic_load_explicit(&ctx->async_left, memory_order_acquire) == 0)
    {
      ks_post(ctx->then_task);
      free(ctx);
    }
    else
    {
      atomic_fetch_sub_explicit(&ctx->async_left, 1, memory_order_release);
    }

    return;
  }

  while (beginning_left != ending_left)
  {
    double   pivot = beginning_left[0];
    double * left  = beginning_left;

    for (double * ptr = beginning_left + 1; ptr != ending_left; ptr++)
    {
      double temp = *ptr;

      if (temp < pivot)
      {
        *ptr    = *left;
        *left++ = temp;
      }
    }

    sorting_ctx_t * ctx_right = malloc(sizeof(sorting_ctx_t));
    memcpy(ctx_right, ctx, sizeof(*ctx));
    
    ctx_right->beginning = left + 1;
    ctx_right->ending    = ending_left;

    ending_left = left;

    atomic_fetch_add_explicit(&ctx->async_left, 1, memory_order_relaxed);
    async_sort_recurse(ctx);
  }
}


static void async_sort(double * array, size_t len)
{
  sorting_ctx_t * ctx = malloc(sizeof(sorting_ctx_t));

  ctx->async_left = 0;
  ctx->beginning  = array;
  ctx->ending     = array + len;
  
  async_sort_recurse(ctx);
}

static void input_array(double ** p_array, size_t * p_len)
{
  double * array = NULL;
  ssize_t  no_elems_in;

  printf("INPUT\tno. elements: ");
  fflush(stdout);
  scanf("%zd", &no_elems_in);

  if (no_elems_in <= 0)
  {
    printf("No. Bad input. Consider entering positive number of elements next time.\n");
    exit(0);
  }

  printf("INPUT\twaiting %ld elements...\n", no_elems_in);
  array = malloc(no_elems_in * sizeof(double));
  for (size_t i = 0; i < no_elems_in; i++)
  {
    scanf("%lf", &array[i]);
  }

  *p_array = array;
  *p_len   = no_elems_in;
}

static void output_and_free_array(double * array, size_t len)
{
  printf("OUTPUT\t: ");
  for (size_t i = 0; i < len; i++)
  {
    printf("%f ", array[i]);
  }

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

  for (size_t i = 0; i < threads_no; i++)
  {
    pthread_create(&threads[i], NULL, worker_routine, NULL);
  }

  double * array = NULL;
  size_t   array_len = 0;
  
  input_array(&array, &array_len);
  async_sort(array, array_len);
  output_and_free_array(array, array_len);
 
  for (size_t i = 0; i < threads_no; i++)
  {
    pthread_join(threads[i], NULL);
  }

  free(threads);
}

