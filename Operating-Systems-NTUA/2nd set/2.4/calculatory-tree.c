#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <assert.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

#include "proc-common.h"
#include "tree.h"

#define SLEEP_TREE_SEC 0.1
#define SLEEP_LEAF 1
void forks(struct tree_node *, int);
int main(int argc, char **argv){
        struct tree_node *root;
        int status, fd[2];
        int result;
        pid_t pid;

        if(argc != 2){
                fprintf(stderr,"Usage: ./calculatory-tree [input file]\n");
                exit(1);
        }

        if(pipe(fd) < 0){
                perror("Initial pipe creation");
                exit(1);
        }

        if(open(argv[1], O_RDONLY) < 0){perror("tree-file error");}
        root = get_tree_from_file(argv[1]);
        pid = fork();
        if(pid < 0){
                perror("Initial fork error");
                exit(2);
        }
        if(pid == 0){
                close(fd[0]);           //stop reading
                forks(root, fd[1]);     //pass fd for writing
                exit(0);
        }
        //parent waits 0.1 sec and prints tree
        sleep(SLEEP_TREE_SEC);
        show_pstree(pid);
        //parent waits for result and reads it
        close(fd[1]); //stop writing
        if(read(fd[0], &result, sizeof(int)) != sizeof(int)) {perror("read pipe");}
        pid = wait(&status);
        explain_wait_status(pid, status);
        printf("Result is: %d\n", result);
        return 0;
}

void forks(struct tree_node *ptr, int pd){
        int i=0;
        int status, result, temp;
        int *number;
        pid_t pid;

        change_pname(ptr->name);
        int fd[2*ptr->nr_children];
        while(i<ptr->nr_children){ //one pipe for each child
                if(pipe(fd + 2*i) < 0){
                        perror("pipe creation");
                        exit(1);
                }

                if(!fork()){
                        change_pname((ptr->children + i)->name);
                        if((ptr->children + i)->nr_children == 0){ //leaf
                                sleep(SLEEP_LEAF);     //wait till right tree is printed
                                int num = atoi((ptr->children + i)->name); //string to int
                                close(fd[2*i]);     //close read port
                                if(write(fd[2*i + 1], &num, sizeof(int)) != sizeof(int)){perror("write pipe");}
                                exit(0);
                        }
                        else{forks(ptr->children + i, fd[2*i + 1]);} //write
                }
                i++;
        }

        number = (int *) malloc (ptr->nr_children * sizeof(int));//mid node
        for(i = 0; i < ptr->nr_children; i++){
                if(read(fd[2*i], &temp, sizeof(temp)) != sizeof(temp)){perror("read pipe");}
                *(number + i)= temp;
        } //pipes for calculations

        switch(strcmp(ptr->name,"+")) {
                case 0:
                        result = *(number + 1) + *number;
                        break;
                default:
                        result = *(number + 1) * *number;
        }

        //pass the result
        if(write(pd, &result, sizeof(int)) != sizeof(int)){perror("write pipe");}

        //children exit&print
        for (i=0; i<ptr->nr_children; i++){
                pid = wait(&status);
                explain_wait_status(pid, status);
        }
        exit(0);
}
