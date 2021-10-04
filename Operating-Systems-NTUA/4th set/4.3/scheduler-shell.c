#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <assert.h>

#include <sys/wait.h>
#include <sys/types.h>

#include "proc-common.h"
#include "request.h"

/* Compile-time parameters. */
#define SCHED_TQ_SEC 2                /* time quantum */
#define TASK_NAME_SZ 60               /* maximum size for a task's name */
#define SHELL_EXECUTABLE_NAME "shell" /* executable for shell */

/* Linked List Node */
typedef struct node {
        pid_t process_pid;
        int id;
        char* exec_name;
        char* prio;
        struct node* next;
} proc_node;

proc_node *curr_proc;
proc_node *next_proc;
proc_node *head = NULL;
proc_node *tail = NULL;

int whichproc; /*Auto-Increment counter for ids of processes */

/*Safe malloc implementation */
void *safe_malloc(size_t size)
{
        void *pid;
        if ((pid = malloc(size)) == NULL) {
                fprintf(stderr, "Out of memory, failed to allocate %zd bytes\n",size);
                exit(1);
        }
        return pid;
}

/* Print a list of all tasks currently being scheduled.  */
static void sched_print_tasks(void)
{
        proc_node *temp;
        temp = head;
        while(temp != tail) {
                if (temp == curr_proc)
                        printf("CURRENT ");
                printf("Process with ID: %d, PID: %d and NAME: %s is currently being scheduled\n", temp->id, temp->process_pid,  temp->exec_name);
                temp = temp->next;
        }
        printf("Process with ID: %d, PID: %d and NAME: %s is currently being scheduled\n", temp->id, temp->process_pid,  temp->exec_name);

}

/* Send SIGKILL depending on its id*/
static int sched_kill_task_by_id(int id)
{
        proc_node *temp;
        temp = head;
        while (temp->id != id) temp = temp->next;

        /*Continue from the right process after freeing temp */
        if (curr_proc->next == temp) {next_proc = temp->next;}
        else {  next_proc = curr_proc->next;}

        /*Free temp by sigchld handler */
        curr_proc = temp;
        if (kill(temp->process_pid, SIGKILL) < 0) {
                perror("kill:SIGKILL");
                exit(1);
        }

        printf("Process with ID: %d, PID: %d and NAME: %s killed\n", id, temp->process_pid, temp->exec_name);
        return 0;
}


/* Create a new task.  */
static void sched_create_task(char *executable)
{
        whichproc++;
        /* Create new  process linked list node */
        proc_node *new_proc;
        new_proc = safe_malloc(sizeof(proc_node));
        new_proc->exec_name = executable;
        new_proc->id = whichproc;
        new_proc->prio = "l";
        /* New process at the end of the list*/
        tail->next = new_proc;
        tail = new_proc;
        new_proc->next = head;
        new_proc->process_pid = fork();
        if (new_proc->process_pid < 0) {
                perror("fork");
                exit(1);
        }
        else if (new_proc->process_pid  == 0) {
                char *executable = new_proc->exec_name;
                char *newargv[] = { strdup(executable), NULL, NULL, NULL };
                char *newenviron[] = { NULL };
                if (raise(SIGSTOP) < 0) {
                        perror("raise:SIGSTOP");
                        exit(1);
                }
                execve(executable, newargv, newenviron);
                /* execve() only returns on error */
                perror("execve");
                exit(1);
        }
}

/* Change priority to high */
static void sched_high_task (int id) {
        proc_node *temp;
        proc_node *temp_prev;
        temp = head;
        temp_prev = tail;
        while (temp->id != id) {
                temp_prev = temp;
                temp = temp->next;
                if(temp == head) {
                        printf("ID: %d is invalid! No process matches with it!\n", id);
                        return;
                }
        }
        if (!strcmp(temp->prio, "h")) { /*if temp->prio != h it already is high*/
                printf("Process with ID: %d already has HIGH priority!\n", temp->id);
                return;
        }

        /*Change priority */
        temp->prio = "h";
        /* Pop temp */
        temp_prev->next = temp->next;
        /*Edge cases */
        if (temp == tail) tail = temp_prev;
        if (temp == head) /*Only low priority processes exist */
                head = temp->next;
        /* Move list to the end of high priority processes */
        proc_node *temph;
        temph = head;
        temp_prev = tail;
        while (!strcmp(temph->prio, "h")){
                temp_prev = temph;
                temph = temph->next;
                if (temph == head) {break;}/*Already at the end of the linked list full of high priorities */
        }
        /*temph=1st low prio elementt */
        temp->next = temph;
        temp_prev->next = temp;
        if (temph == head) head = temp;
        printf("Process with ID: %d has HIGH priority from now on\n", temp->id);

}

/* Change priority to low */
static void sched_low_task (int id) {
        proc_node *temp;
        proc_node *temp_prev;
        temp = head;
        temp_prev = tail;
        while (temp->id != id) {
                temp_prev = temp;
                temp = temp->next;
                if(temp == head) {
                        printf("ID: %d is invalid! No process matches with it!\n", id);
                        return;
                }
        }
        if (!strcmp(temp->prio, "l")) {
                printf("Process with ID: %d already has LOW priority!\n", temp->id);
                return;
        }

        /*Change priority*/
        temp->prio = "l";
        /* Pop temp*/
        temp_prev->next = temp->next;
        /*Edge cases */
        if (temp == tail) /*Only high priority processes existed */
                tail = temp_prev;
        if (temp == head) head = temp->next;
        /* Move list node to the end of low priority processes (tail)*/
        tail->next = temp;
        temp->next = head;
        tail = temp;
        printf("Process with id: %d has LOW priority from now on\n", temp->id);
}

/* Process requests by shell*/
static int process_request(struct request_struct *rq)
{
        switch (rq->request_no) {
                case REQ_PRINT_TASKS:
                        sched_print_tasks();
                        return 0;

                case REQ_KILL_TASK:
                        return sched_kill_task_by_id(rq->task_arg);

                case REQ_EXEC_TASK:
                        sched_create_task(rq->exec_task_arg);
                        return 0;

                case REQ_HIGH_TASK:
                        sched_high_task(rq->task_arg);
                        return 0;
                case REQ_LOW_TASK:
                        sched_low_task(rq->task_arg);
                        return 0;

                default:
                        return -ENOSYS;
        }
}

/*
 * SIGALRM handler
*/
static void sigalrm_handler(int signum)
{
        /*tq ended -> SIGSTOP to current process*/
        if (kill(curr_proc->process_pid, SIGSTOP) < 0) {
                perror("kill:SIGSTOP");
                exit(1);
        }
}

/*
 * SIGCHLD handler
*/
static void sigchld_handler(int signum)
{
        int p, status;

        for(;;) {
                p = waitpid(-1, &status, WUNTRACED | WNOHANG);
                if (p < 0) {
                        perror("waitpid");
                        exit(1);
                }
                if (p==0)  /*Child(ren) still in the same state*/
                        break;
                explain_wait_status(p, status);

                if (WIFEXITED(status) || WIFSIGNALED(status)) {
                        /* A child has died */
                        /*Pop process from list and proceed to next one*/
                        if (head == tail) {
                                free(curr_proc);
                                exit(1);
                        }
                        if (curr_proc == head) {
                                head = head->next;
                                tail->next = curr_proc->next;
                                free(curr_proc);

                        }
                        else if (curr_proc == tail) {
                                proc_node *temp = head;
                                while (temp->next != curr_proc) {
                                        temp = temp->next;
                                }
                                tail = temp;
                                temp->next = curr_proc->next;
                                free(curr_proc);
                        }
                        else {
                                proc_node *temp = head;
                                while (temp->next != curr_proc) {
                                        temp = temp->next;
                                }
                                temp->next = curr_proc->next;
                                free(curr_proc);
                        }
                        if (kill(next_proc->process_pid, SIGCONT) < 0) {
                                perror("kill:SIGCONT");
                        }
                        curr_proc = next_proc;
                        next_proc = curr_proc->next;
                        alarm(SCHED_TQ_SEC);
                }
                if (WIFSTOPPED(status)) {
                        /* SIGSTOP/SIGTSTP, etc */
                        /*Checking if shell is the only open process */
                        if (!strcmp(next_proc->exec_name ,"shell") && head == tail) {
                                if (kill(next_proc->process_pid, SIGKILL)) {
                                        perror("kill:SIGKILL");
                                }
                        }
                        if (!strcmp(next_proc->prio, "l") && !strcmp(head->prio, "h"))
                                next_proc = head;
                        if (kill(next_proc->process_pid, SIGCONT) < 0) {
                                perror("kill:SIGCONT");
                        }
                        curr_proc = next_proc;
                        next_proc = curr_proc->next;
                        alarm(SCHED_TQ_SEC);
                }
        }
}

/* Disable delivery of SIGALRM and SIGCHLD. */
static void signals_disable(void)
{
        sigset_t sigset;

        sigemptyset(&sigset);
        sigaddset(&sigset, SIGALRM);
        sigaddset(&sigset, SIGCHLD);
        if (sigprocmask(SIG_BLOCK, &sigset, NULL) < 0) {
                perror("signals_disable: sigprocmask");
                exit(1);
        }
}

/* Enable delivery of SIGALRM and SIGCHLD.  */
static void signals_enable(void)
{
        sigset_t sigset;

        sigemptyset(&sigset);
        sigaddset(&sigset, SIGALRM);
        sigaddset(&sigset, SIGCHLD);
        if (sigprocmask(SIG_UNBLOCK, &sigset, NULL) < 0) {
                perror("signals_enable: sigprocmask");
                exit(1);
        }
}


/* Install two signal handlers.
 *  * One for SIGCHLD, one for SIGALRM.
 *   * Make sure both signals are masked when one of them is running.
 **/
static void install_signal_handlers(void)
{
        sigset_t sigset;
        struct sigaction sa;

        sa.sa_handler = sigchld_handler;
        sa.sa_flags = SA_RESTART;
        sigemptyset(&sigset);
        sigaddset(&sigset, SIGCHLD);
        sigaddset(&sigset, SIGALRM);
        sa.sa_mask = sigset;
        if (sigaction(SIGCHLD, &sa, NULL) < 0) {
                perror("sigaction: sigchld");
                exit(1);
        }

        sa.sa_handler = sigalrm_handler;
        if (sigaction(SIGALRM, &sa, NULL) < 0) {
                perror("sigaction: sigalrm");
                exit(1);
        }

        /*
         ** Ignore SIGPIPE, so that write()s to pipes
         ** with no reader do not result in us being killed,
         ** and write() returns EPIPE instead.
         **/
        if (signal(SIGPIPE, SIG_IGN) < 0) {
                perror("signal: sigpipe");
                exit(1);
        }
}

static void do_shell(char *executable, int wfd, int rfd)
{
        char arg1[10], arg2[10];
        char *newargv[] = { executable, NULL, NULL, NULL };
        char *newenviron[] = { NULL };

        sprintf(arg1, "%05d", wfd);
        sprintf(arg2, "%05d", rfd);
        newargv[1] = arg1;
        newargv[2] = arg2;

        raise(SIGSTOP);
        execve(executable, newargv, newenviron);

        /* execve() only returns on error */
        perror("scheduler: child: execve");
        exit(1);
}

/* Create a new shell task.
 **
 ** The shell gets special treatment:
 ** two pipes are created for communication and passed
 ** as command-line arguments to the executable.
 **/
static void sched_create_shell(char *executable, int *request_fd, int *return_fd)
{
        pid_t p;
        int pfds_rq[2], pfds_ret[2];

        if (pipe(pfds_rq) < 0 || pipe(pfds_ret) < 0) {
                perror("pipe");
                exit(1);
        }

        p = fork();
        if (p < 0) {
                perror("scheduler: fork");
                exit(1);
        }

        if (p == 0) {
                /* Child */
                close(pfds_rq[0]);
                close(pfds_ret[1]);
                do_shell(executable, pfds_rq[1], pfds_ret[0]);
                assert(0);
        }
        /* Parent */
        close(pfds_rq[1]);
        close(pfds_ret[0]);
        *request_fd = pfds_rq[0];
        *return_fd = pfds_ret[1];

        /* Insert shell in processes linked list */
        proc_node *new_proc;
        new_proc = safe_malloc(sizeof(proc_node));
        new_proc->exec_name = "shell";
        new_proc->id = 1;
        new_proc->prio = "l";

        head = new_proc;
        tail = new_proc;
        new_proc->next = new_proc;
        new_proc->process_pid = p;
}

static void shell_request_loop(int request_fd, int return_fd)
{
        int ret;
        struct request_struct rq;

        /*
         *       * Keep receiving requests from the shell.
         *               */
        for (;;) {
                if (read(request_fd, &rq, sizeof(rq)) != sizeof(rq)) {
                        perror("scheduler: read from shell");
                        fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
                        break;
                }

                signals_disable();
                ret = process_request(&rq);
                signals_enable();

                if (write(return_fd, &ret, sizeof(ret)) != sizeof(ret)) {
                        perror("scheduler: write to shell");
                        fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
                        break;
                }
        }
}

int main(int argc, char *argv[])
{
        /* Two file descriptors for communication with the shell */
        static int request_fd, return_fd;

        /* Create the shell. */
        sched_create_shell(SHELL_EXECUTABLE_NAME, &request_fd, &return_fd);

        /*
         * For each of argv[1] to argv[argc - 1],
         * create a new child process, add it to the process list.
         */

        whichproc = 1; /* number of proccesses goes here, 1 because shell is the first process*/

        int i;
        for (i=1; i < argc; i++) {
                /* Adding process to the list */
                whichproc++;
                proc_node *new_proc;
                new_proc = safe_malloc(sizeof(proc_node));
                new_proc->exec_name = argv[i];
                new_proc->id = whichproc;
                new_proc->prio = "l";
                /*Find tail of list and point it to new proc*/
                proc_node *temp = tail;
                tail = new_proc;
                temp->next = new_proc;

                if (i == argc-1) {
                        /*Reached tail*/
                        new_proc->next = head;
                }
                else {new_proc->next = NULL;}
                new_proc->process_pid = fork();
                if (new_proc->process_pid < 0) {
                        perror("fork");
                        exit(1);
                }
                else if (new_proc->process_pid  == 0) {
                        char *executable = new_proc->exec_name;
                        char *newargv[] = { executable, NULL, NULL, NULL };
                        char *newenviron[] = { NULL };
                        if (raise(SIGSTOP) < 0) {
                                perror("raise:SIGSTOP");
                                exit(1);
                        }
                        execve(executable, newargv, newenviron);

                        /* execve() only returns on error */
                        perror("execve");
                        exit(1);
                }
        }

        /* Wait for all children to raise SIGSTOP before exec()ing. */
        wait_for_ready_children(whichproc);

        /* Install SIGALRM and SIGCHLD handlers. */
        install_signal_handlers();

        if (whichproc == 0) {
                fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
                exit(1);
        }

        /* Start of scheduling - SIGCONT goes to 1st process*/
        if (kill(head->process_pid, SIGCONT) < 0){
                perror("kill:SIGCONT");
                exit(1);
        }
        curr_proc = head;
        if (whichproc == 1) {curr_proc->next = curr_proc;}
        next_proc = curr_proc->next;
        alarm(SCHED_TQ_SEC);

        shell_request_loop(request_fd, return_fd);

        /* Now that the shell is gone, just loop forever
         ** until we exit from inside a signal handler.
         **/
        while (pause())
                ;

        /* Unreachable */
        fprintf(stderr, "Internal error: Reached unreachable point\n");
        return 1;
}

