#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "tree.h"
#include "proc-common.h"

void fork_procs(struct tree_node *root)
{
        /*
         * Start
         */
        int status;
        int i;
        pid_t *pid;
        printf("PID = %ld, name %s, starting...\n",
                        (long)getpid(), root->name);
        change_pname(root->name);
        int nr_child=root->nr_children;
        if (nr_child==0){
        /* ... */

        /*
         * Suspend Self
         */
                raise(SIGSTOP);
                printf("PID = %ld, name = %s is awake\n",
                        (long)getpid(), root->name);
                exit(0);
        }
        else{
                pid=calloc(nr_child, sizeof(pid_t)); //just like malloc, except for nr_child units
                for (i=0;i<root->nr_children;i++){
                        pid_t next;
                        next=fork();
                        if (next<0) {perror("Error");}
                        if (next==0){
                                fork_procs(root->children+i);
                                exit(0);
                        }
                        else {pid[i]=next;} //PID stored in pid
                        wait_for_ready_children(1);
                }
                raise(SIGSTOP);
                printf("PID = %ld, name = %s is awake\n",
                        (long)getpid(), root->name);
                for (i=0;i<nr_child;i++){
                        kill(pid[i],SIGCONT); //send signal
                        wait(&status);
                        explain_wait_status(pid[i], status);
                }
                wait(&status);
                explain_wait_status(getpid(), status);


        /* ... */

        /*
         * Exit
         */
                exit(0);
        }
}

/*
 * The initial process forks the root of the process tree,
 * waits for the process tree to be completely created,
 * then takes a photo of it using show_pstree().
 *
 * How to wait for the process tree to be ready?
 * In ask2-{fork, tree}:
 *      wait for a few seconds, hope for the best.
 * In ask2-signals:
 *      use wait_for_ready_children() to wait until
 *      the first process raises SIGSTOP.
 */

int main(int argc, char *argv[])
{
        pid_t pid;
        int status;
        struct tree_node *root;

        if (argc < 2){
                fprintf(stderr, "Usage: %s <tree_file>\n", argv[0]);
                exit(1);
        }

        /* Read tree into memory */
        root = get_tree_from_file(argv[1]);

        /* Fork root of process tree */
        pid = fork();
        if (pid < 0) {
                perror("main: fork");
                exit(1);
        }
        if (pid == 0) {
                /* Child */
                fork_procs(root);
                exit(1);
        }

        /*
         * Father
         */
        /* for ask2-signals */
        wait_for_ready_children(1);

        /* for ask2-{fork, tree} */
        /* sleep(SLEEP_TREE_SEC); */

        /* Print the process tree root at pid */
        show_pstree(pid);

        /* for ask2-signals */
        kill(pid, SIGCONT);

        /* Wait for the root of the process tree to terminate */
        wait(&status);
        explain_wait_status(pid, status);

        return 0;
}
