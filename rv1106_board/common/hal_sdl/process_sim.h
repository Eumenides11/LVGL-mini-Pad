#ifndef PROCESS_SIM_H
#define PROCESS_SIM_H

#include <sys/types.h>

/* Maximum number of simulated processes */
#define PSIM_MAX_PROCS 16

/* human readable state string length */
#define PSIM_STATE_LEN 16
#define PSIM_NAME_LEN 32

typedef struct {
    pid_t pid;
    char name[PSIM_NAME_LEN];
    char state[PSIM_STATE_LEN];
} psim_entry_t;

/* initialize the process simulator data structures */
void psim_init(void);

/* create a new child process with the given name */
void psim_fork_process(const char *name);

/* refresh the status of existing processes */
void psim_update_states(void);

/* return pointer to array and count (for UI) */
psim_entry_t *psim_get_list(size_t *count);

#endif /* PROCESS_SIM_H */
