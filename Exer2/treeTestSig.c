#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "proc-common.h"
#include "tree.h"

#define SLEEP_PROC_SEC  10
#define SLEEP_TREE_SEC  3

int count(struct tree_node *root);

void fork_procs(struct tree_node *root) {

    int status;
    pid_t pid[root->nr_children];
    change_pname(root->name);
    printf("PID = %ld, name %s, starting...\n", (long)getpid(), root->name);


    /*if proccess is a leaf*/
    if (root->nr_children == 0) {
        raise(SIGSTOP);
        printf("PID = %ld, name = %s is awake\n", (long)getpid(), root->name);
        exit(0);
    }

    /*if proccess has children*/
    else {
        int i;
        /*Creating children of root proccess*/
        for (i = 0; i < root->nr_children; i++) {
            pid[i] = fork();

            if (pid[i] < 0) {
                perror("fork failed...\n");
                exit(1);
            }

            /*Child i doing...*/
            if (pid[i] == 0) {
                fork_procs(root->children + i);
            }
            
        }

        /*root doing after children are created*/
        /*stop root proccess*/
        wait_for_ready_children(root->nr_children);
        raise(SIGSTOP);
        
        printf("PID = %ld, name = %s is awake\n", (long)getpid(), root->name);

        for (i = 0; i < root->nr_children; i++) {
            kill(pid[i], SIGCONT);
            pid[i] = wait(&status);
            explain_wait_status(pid[i], status);      
        }
        exit(0);

    }
}

int main(int argc, char **argv) {

    struct tree_node *root;

    if (argc != 2) {        
            fprintf(stderr, "Usage: %s <input_tree_file>\n\n", argv[0]);
            exit(1);
    }

    root = get_tree_from_file(argv[1]);
    pid_t pid;
    int status;

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

    /*make sure that every child is stopped*/
    wait_for_ready_children(1);

    /* Print the process tree root at pid */
    show_pstree(pid);
    /* for ask2-signals */
    /*root process continue*/
    kill(pid, SIGCONT); 

    /* Wait for the root of the process tree to terminate */
    pid = wait(&status);
    explain_wait_status(pid, status);

    return 0;
}
