/**
 * The MapReduce coordinator.
 */

#include "coordinator.h"

#ifndef SIG_PF
#define SIG_PF void (*)(int)
#endif

/* Global coordinator state. */
coordinator* state;

extern void coordinator_1(struct svc_req*, struct SVCXPRT*);

/* Set up and run RPC server. */
int main(int argc, char** argv) {
  register SVCXPRT* transp;

  pmap_unset(COORDINATOR, COORDINATOR_V1);

  transp = svcudp_create(RPC_ANYSOCK);
  if (transp == NULL) {
    fprintf(stderr, "%s", "cannot create udp service.");
    exit(1);
  }
  if (!svc_register(transp, COORDINATOR, COORDINATOR_V1, coordinator_1, IPPROTO_UDP)) {
    fprintf(stderr, "%s", "unable to register (COORDINATOR, COORDINATOR_V1, udp).");
    exit(1);
  }

  transp = svctcp_create(RPC_ANYSOCK, 0, 0);
  if (transp == NULL) {
    fprintf(stderr, "%s", "cannot create tcp service.");
    exit(1);
  }
  if (!svc_register(transp, COORDINATOR, COORDINATOR_V1, coordinator_1, IPPROTO_TCP)) {
    fprintf(stderr, "%s", "unable to register (COORDINATOR, COORDINATOR_V1, tcp).");
    exit(1);
  }

  coordinator_init(&state);

  svc_run();
  fprintf(stderr, "%s", "svc_run returned");
  exit(1);
  /* NOTREACHED */
}

/* EXAMPLE RPC implementation. */
int* example_1_svc(int* argp, struct svc_req* rqstp) {
  static int result;

  result = *argp + 1;

  return &result;
}

/* SUBMIT_JOB RPC implementation. */
int* submit_job_1_svc(submit_job_request* argp, struct svc_req* rqstp) {
  static int result;

  printf("Received submit job request\n");

  /* TODO */
  job *job_ptr = malloc(sizeof(job));
  result = init_job(job_ptr, argp);
  if (result == -1) return &result;

  g_queue_push_tail(state->map_queue, job_ptr);
  g_hash_table_insert(state->jobs, GINT_TO_POINTER(job_ptr->job_id), job_ptr);

  /* Do not modify the following code. */
  /* BEGIN */
  struct stat st;
  if (stat(argp->output_dir, &st) == -1) {
    mkdirp(argp->output_dir);
  }

  return &result;
  /* END */
}

/* POLL_JOB RPC implementation. */
poll_job_reply* poll_job_1_svc(int* argp, struct svc_req* rqstp) {
  static poll_job_reply result;

  printf("Received poll job request\n");

  /* TODO */
  int job_id = *argp;
  job *lookup_job = g_hash_table_lookup(state->jobs, GINT_TO_POINTER(job_id));
  if (lookup_job == NULL) {
    result.done = false;
    result.failed = false;
    result.invalid_job_id = true;
    return &result;
  }
  result.invalid_job_id = false;
  if (lookup_job->job_status == DONE) {
    result.done = true;
  } else {
    result.done = false;
  }
  if (lookup_job->job_status == FAILED) {
    result.done = true;
    result.failed = true;
  } else {
    result.failed = false;
  }

  return &result;
}

/* GET_TASK RPC implementation. */
get_task_reply* get_task_1_svc(void* argp, struct svc_req* rqstp) {
  static get_task_reply result;

  printf("Received get task request\n");
  result.file = "";
  result.output_dir = "";
  result.app = "";
  result.wait = true;
  result.args.args_len = 0;

  /* TODO */
  job *job_ptr;
  for (GList *elem = state->reduce_queue->head; elem; elem = elem->next) {
    job_ptr = elem->data;
    g_assert_nonnull(job_ptr);
    for (int i = 0; i < job_ptr->n_reduce; i++) {
      if (job_ptr->reduces_given[i] == false || (time(NULL) - job_ptr->reduces_start[i]) > TASK_TIMEOUT_SECS) {
        // give reduce task i
        printf("Assigned job %d reduce task %d\n", job_ptr->job_id, i);
        job_ptr->reduces_given[i] = true;
        job_ptr->reduces_start[i] = time(NULL);
        result.job_id = job_ptr->job_id;
        result.task = i;
        result.output_dir = job_ptr->output_dir;
        result.app = job_ptr->app;
        result.n_reduce = job_ptr->n_reduce;
        result.n_map = job_ptr->files.files_len;
        result.reduce = true;
        result.wait = false;
        result.args.args_len = job_ptr->args.args_len;
        result.args.args_val = job_ptr->args.args_val;
        return &result;
      }
    }
  }

  for (GList *elem = state->map_queue->head; elem; elem = elem->next) {
    job_ptr = elem->data;
    g_assert_nonnull(job_ptr);
    for (int i = 0; i < job_ptr->files.files_len; i++) {
      if (job_ptr->maps_given[i] == false || (time(NULL) - job_ptr->maps_start[i]) > TASK_TIMEOUT_SECS) {
        // give map task i
        printf("Assigned job %d map task %d\n", job_ptr->job_id, i);
        job_ptr->maps_given[i] = true;
        job_ptr->maps_start[i] = time(NULL);
        result.job_id = job_ptr->job_id;
        result.task = i;
        result.file = job_ptr->files.files_val[i];
        result.output_dir = job_ptr->output_dir;
        result.app = job_ptr->app;
        result.n_reduce = job_ptr->n_reduce;
        result.n_map = job_ptr->files.files_len;
        result.reduce = false;
        result.wait = false;
        result.args.args_len = job_ptr->args.args_len;
        result.args.args_val = job_ptr->args.args_val;
        return &result;
      }
    }
  }

  return &result;
}

/* FINISH_TASK RPC implementation. */
void* finish_task_1_svc(finish_task_request* argp, struct svc_req* rqstp) {
  static char* result;

  printf("Received finish task request\n");

  /* TODO */
  job *job_ptr = g_hash_table_lookup(state->jobs, GINT_TO_POINTER(argp->job_id));
  if (argp->success == false) {
    job_ptr->job_status = FAILED;
  } else if (argp->reduce == true) {
    job_ptr->reduces_done[argp->task] = true;
    bool all_done = true;
    for (int i = 0; i < job_ptr->n_reduce; i++) {
      if (job_ptr->reduces_done[i] == false) {
        all_done = false;
        break;
      }
    }
    if (all_done) {
      printf("All job %d reduce tasks done\n", job_ptr->job_id);
      g_assert(g_queue_remove(state->reduce_queue, job_ptr));
      job_ptr->job_status = DONE;
    }
  } else if (argp->reduce == false) {
    job_ptr->maps_done[argp->task] = true;
    bool all_done = true;
    for (int i = 0; i < job_ptr->files.files_len; i++) {
      if (job_ptr->maps_done[i] == false) {
        all_done = false;
        break;
      }
    }
    if (all_done) {
      printf("All job %d map tasks done\n", job_ptr->job_id);
      g_assert(g_queue_remove(state->map_queue, job_ptr));
      job_ptr->job_status = REDUCE;
      g_queue_push_tail(state->reduce_queue, job_ptr);
    }
  }

  return (void*)&result;
}

/* Initialize coordinator state. */
void coordinator_init(coordinator** coord_ptr) {
  *coord_ptr = malloc(sizeof(coordinator));

  coordinator* coord = *coord_ptr;

  /* TODO */
  coord->map_queue = g_queue_new();
  coord->reduce_queue = g_queue_new();
  coord->jobs = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, NULL);
}
