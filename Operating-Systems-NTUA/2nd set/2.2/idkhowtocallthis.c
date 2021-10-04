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


#define SLEEP_TREE_SEC 6
#define SLEEP_LEAF 10

void forks(struct tree_node *);
int main(int argc, char **argv){
        struct tree_node *root;
        int status;
        id_t p; //help files

        if (argc != 2) {
                fprintf(stderr, "Usage: %s <input_tree_file>\n\n", argv[0]);
                exit(1);
        }

        root = get_tree_from_file(argv[1]);
        print_tree(root);
        //Usage
        if(argc != 2){
                fprintf(stderr,"Usage: ./fork-tree [input file]\n");
                exit(1);
        }
        if(open(argv[1], O_RDONLY) < 0){
                perror("Error");
        }
        root = get_tree_from_file(argv[1]);
        //first child
        p = fork();
        if(p < 0){
                perror("Error");
                exit(2);
        }
        if(p == 0){
                forks(root);
                exit(0);
        }
        sleep(SLEEP_TREE_SEC);
        show_pstree(p);
        p = wait(&status);
        explain_wait_status(p, status);
        return 0;
}

void forks(struct tree_node *ptr){
        int i = 0;
        int status;
        pid_t p;
        change_pname(ptr->name);

    //All children.
        while(i < ptr->nr_children){
        printf("Node  %s creates %d children\n", ptr->name, ptr->nr_children - i);
        if(!fork()){
                change_pname((ptr->children + i)->name);
                if((ptr->children + i)->nr_children == 0){
                //leaves
                        printf("Node %s is going to sleep\n", (ptr->children + i)->name);
                        sleep(SLEEP_LEAF);
                        printf("Node %s woke up and exiting now...\n", (ptr->children + i)->name);
                        exit(0);
            }
                else{
                //middle node
                        forks(ptr->children + i);
                }
        }
        i++;
    }

        for (i=0; i<ptr->nr_children; i++){
                p = wait(&status);  //waits for children
                explain_wait_status(p, status); //prints exit status
        }
        exit(0);
}
