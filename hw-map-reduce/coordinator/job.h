/**
 * Logic for job and task management.
 *
 * You are not required to modify this file.
 */

#ifndef JOB_H__
#define JOB_H__

/* You may add definitions here */
#include <stdint.h>
#include "../rpc/rpc.h"
#include "../app/app.h"

enum status {WAITING, MAP, REDUCE, PROCESS_OUTPUT, DONE, FAILED};

typedef struct {
    uint32_t id;
    enum status job_status;
    app job_app;
    struct {
		u_int files_len;
		path *files_val;
	} files;
    uint32_t files_mapped;
	path output_dir;
    int n_reduce;
    int reduce_tasks_given;
    struct {
		u_int args_len;
		char *args_val;
	} args;
} job;

int init_job(job *job_ptr, submit_job_request* request);

#endif
