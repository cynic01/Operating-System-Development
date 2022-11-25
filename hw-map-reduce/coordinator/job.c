/**
 * Logic for job and task management.
 *
 * You are not required to modify this file.
 */

#include "job.h"

uint32_t next_id = 0;

int init_job(job *job_ptr, submit_job_request* request) {
    job_ptr->id = next_id;
    next_id++;
    job_ptr->job_status = WAITING;

    job_ptr->job_app = get_app(request->app);
    if (job_ptr->job_app.name == NULL) return -1;
    // job_ptr->job_app = malloc(sizeof(app));
    // if (job_ptr->job_app == NULL) return false;
    // job_ptr->job_app->name = malloc(strlen(app_ptr->name) + 1);
    // strcpy(job_ptr->job_app->name, app_ptr->name);
    // job_ptr->job_app->map = app_ptr->map;
    // job_ptr->job_app->reduce = app_ptr->reduce;
    // job_ptr->job_app->process_output = app_ptr->process_output;

    job_ptr->files.files_len = request->files.files_len;
    job_ptr->files.files_val = malloc(request->files.files_len * sizeof(path));
    if (job_ptr->files.files_val == NULL) return -1;
    for (int i = 0; i < request->files.files_len; i++) {
        job_ptr->files.files_val[i] = strdup(request->files.files_val[i]);
        if (job_ptr->files.files_val[i] == NULL) return -1;
    }
    job_ptr->files_mapped = 0;

    job_ptr->output_dir = strdup(request->output_dir);
    if (job_ptr->output_dir == NULL) return -1;

    job_ptr->n_reduce = request->n_reduce;
    job_ptr->reduce_tasks_given = 0;

    job_ptr->args.args_len = request->args.args_len;
    job_ptr->args.args_val = strndup(request->args.args_val, request->args.args_len);
    if (job_ptr->args.args_val == NULL) return -1;

    return job_ptr->id;
}