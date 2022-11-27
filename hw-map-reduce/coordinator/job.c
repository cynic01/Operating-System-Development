/**
 * Logic for job and task management.
 *
 * You are not required to modify this file.
 */

#include "job.h"

uint32_t next_id = 0;

int init_job(job *job_ptr, submit_job_request* request) {
    job_ptr->job_id = next_id;
    next_id++;
    job_ptr->job_status = MAP;

    job_ptr->app = strdup(request->app);
    if (job_ptr->app == NULL) return -1;

    job_ptr->files.files_len = request->files.files_len;
    job_ptr->files.files_val = malloc(request->files.files_len * sizeof(path));
    if (job_ptr->files.files_val == NULL) return -1;
    for (int i = 0; i < request->files.files_len; i++) {
        job_ptr->files.files_val[i] = strdup(request->files.files_val[i]);
        if (job_ptr->files.files_val[i] == NULL) return -1;
    }
    job_ptr->maps_given = malloc(request->files.files_len * sizeof(bool));
    memset(job_ptr->maps_given, false, request->files.files_len);
    job_ptr->maps_done = malloc(request->files.files_len * sizeof(bool));
    memset(job_ptr->maps_done, false, request->files.files_len);

    job_ptr->output_dir = strdup(request->output_dir);
    if (job_ptr->output_dir == NULL) return -1;

    job_ptr->n_reduce = request->n_reduce;
    job_ptr->reduces_given = malloc(request->n_reduce * sizeof(bool));
    memset(job_ptr->reduces_given, false, request->n_reduce);
    job_ptr->reduces_done = malloc(request->n_reduce * sizeof(bool));
    memset(job_ptr->reduces_done, false, request->n_reduce);

    job_ptr->args.args_len = request->args.args_len;
    job_ptr->args.args_val = strndup(request->args.args_val, request->args.args_len);
    if (job_ptr->args.args_val == NULL) return -1;

    return job_ptr->job_id;
}

void free_job_contents(job *job_ptr) {
    free(job_ptr->args.args_val);
    free(job_ptr->reduces_done);
    free(job_ptr->reduces_given);
    free(job_ptr->output_dir);
    free(job_ptr->maps_done);
    free(job_ptr->maps_given);
    for (int i = 0; i < job_ptr->files.files_len; i++) {
        free(job_ptr->files.files_val[i]);
    }
    free(job_ptr->files.files_val);
}