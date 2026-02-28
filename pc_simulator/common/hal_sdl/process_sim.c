#include "process_sim.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

/* simple fixed-size array to hold simulated processes */
static psim_entry_t proc_list[PSIM_MAX_PROCS];
static size_t proc_count = 0;

void psim_init(void)
{
    proc_count = 0;
    memset(proc_list, 0, sizeof(proc_list));
}

void psim_list_add(pid_t pid, const char *name, const char *state)
{
    if(proc_count >= PSIM_MAX_PROCS) {
        fprintf(stderr, "psim: process list full, cannot add %s\n", name);
        return;
    }
    proc_list[proc_count].pid = pid;
    strncpy(proc_list[proc_count].name, name, PSIM_NAME_LEN - 1);
    strncpy(proc_list[proc_count].state, state, PSIM_STATE_LEN - 1);
    proc_count++;
}

void psim_fork_process(const char *name)
{
    pid_t pid = fork();
    if(pid < 0) {
        perror("fork");
        return;
    } else if(pid == 0) {
        /* child */
        printf("Child '%s' started, pid=%d\n", name, getpid());
        /* simulate some work */
        sleep(3 + (rand() % 5));
        printf("Child '%s' exiting, pid=%d\n", name, getpid());
        exit(0);
    } else {
        /* parent: record the child in the list */
        psim_list_add(pid, name, "running");
    }
}

void psim_update_states(void)
{
    for(size_t i = 0; i < proc_count; i++) {
        if(strcmp(proc_list[i].state, "running") == 0) {
            int status;
            pid_t ret = waitpid(proc_list[i].pid, &status, WNOHANG);
            if(ret == proc_list[i].pid) {
                /* child terminated */
                strncpy(proc_list[i].state, "terminated", PSIM_STATE_LEN - 1);
            }
        }
    }
}

psim_entry_t *psim_get_list(size_t *count)
{
    if(count) *count = proc_count;
    return proc_list;
}
