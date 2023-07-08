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

static int k = 0; //exit code 
void fork_procs(struct tree_node *root) {

    int status;
    pid_t pid;//[root->nr_children];
    change_pname(root->name);
    printf("%s: Initiating...\n", root->name);

    /*if proccess is a leaf*/
    if (root->nr_children == 0) {
        printf("%s: Sleeping...\n", root->name);
        sleep(SLEEP_PROC_SEC);
        printf("%s: Exiting...\n", root->name);
        exit(k++);
    }

    /*if proccess has children*/
    else {
        int i;
        /*Creating children of root proccess*/
        for (i = 0; i < root->nr_children; i++) {
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
        printf("%s: Waiting for children...\n", root->name);
        int j;
        for (j = 0; j < root->nr_children; j++) {
            pid = wait(&status);
            explain_wait_status(pid, status);      
        }
        printf("%s: Exiting...\n", root->name);
        exit(k++);

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


    /* for ask2-{fork, tree} */
    sleep(SLEEP_TREE_SEC);

    /* Print the process tree root at pid */
    show_pstree(pid);
    /* for ask2-signals */
    /* kill(pid, SIGCONT); */

    /* Wait for the root of the process tree to terminate */
    pid = wait(&status);
    explain_wait_status(pid, status);

    return 0;
}
