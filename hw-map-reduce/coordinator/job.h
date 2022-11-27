/**
 * Logic for job and task management.
 *
 * You are not required to modify this file.
 */

#ifndef JOB_H__
#define JOB_H__

/* You may add definitions here */
#include <stdint.h>
#include <stdbool.h>
#include "../rpc/rpc.h"
#include "../app/app.h"

enum status {MAP, REDUCE, DONE, FAILED};

typedef struct {
  int job_id;
  enum status job_status;
  char *app;
  struct {
		u_int files_len;
		path *files_val;
	} files;
  bool *maps_given;
  bool *maps_done;
	path output_dir;
  int n_reduce;
  bool *reduces_given;
  bool *reduces_done;
  struct {
		u_int args_len;
		char *args_val;
	} args;
} job;

int init_job(job *job_ptr, submit_job_request* request);
void free_job_contents(job *job_ptr);

#endif
