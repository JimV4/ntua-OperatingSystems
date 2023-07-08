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

void fork_procs(struct tree_node *root) {

    int status;
    pid_t pid;//[root->nr_children];
    double value[root->nr_children];
    int pfd[2];
    change_pname(root->name);
    printf("%s: Initiating...\n", root->name);

    /*if proccess is a leaf*/
    if (root->nr_children == 0) {
        /*printf("%s: Sleeping...\n", root->name);
        sleep(SLEEP_PROC_SEC);
        printf("%s: Exiting...\n", root->name);
        exit(k++);*/
        value[0] = atoi(root->name);
        if (write(pfd[1], &value[0], sizeof(value[0])) != sizeof(value[0])) {
            perror("write to pipe");
            exit(1);
        }
        exit(0);
    }

    /*if proccess has children*/
    else {
        int i;
        /*Creating children of root proccess*/
        for (i = 0; i < root->nr_children; i++) {
            if (pipe(pfd) < 0) {
                perror("pipe");
                exit(1);
            }
            pid = fork();

            if (pid < 0) {
                perror("fork failed...\n");
                exit(1);
            }

            /*Child i doing...*/
            if (pid == 0) {
                
                fork_procs(root->children + i);
            }
            
        }


        /*root doing after children are created*/
        //printf("%s: Waiting for children...\n", root->name);
        int j;
        for (j = 0; j < root->nr_children; j++) {
            pid = wait(&status);
            explain_wait_status(pid, status);      
        }
        printf("%s: Exiting...\n", root->name);
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
    double value;
    int pfd[2];

    /*create a pipe to receive data from child process*/
    if (pipe(pfd) < 0) {
        perror("pipe");
        exit(1);
    }

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

    /* for ask2-{fork, tree} */
    sleep(SLEEP_TREE_SEC);

    /* Print the process tree root at pid */
    show_pstree(pid);

    /* Wait for the root of the process tree to terminate */
    pid = wait(&status);
    explain_wait_status(pid, status);

    /*main process reads from pipe with child process and
    /*returns the result*/
    if (read(pfd[0], &value, sizeof(value)) != sizeof(value)) {
        perror("read from pipe");
        exit(1);
    }
    else {
        printf("The result is %f", value);
    }

    return 0;
}
