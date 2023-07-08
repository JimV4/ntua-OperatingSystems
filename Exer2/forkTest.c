#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "proc-common.h"

#define SLEEP_PROC_SEC  10
#define SLEEP_TREE_SEC  3

/*
 * Create this process tree:
 * A-+-B---D
 *   `-C
 */
void fork_procs(void)
{
        /*
         * initial process is A.
         */
        int status;
        change_pname("A");
        printf("A: Initialising...\n");
        /*Create child B of A*/
        pid_t pid1 = fork();
        if (pid1 < 0) 
        {
                perror("B fork failed...\n");
                exit(16);
        }
        /*Proccess B doing...*/
        if (pid1 == 0) {
                change_pname("B");
                printf ("B: Initiating...\n");

                /*Create child D of B*/
                pid1 = fork();
                if (pid1 < 0) {
                        perror("D fork failed...\n");
                        exit(13);
                }
                /*Process D doing...*/
                if (pid1 == 0) {
                        change_pname("D");
                        printf("D: Initialising...\n");
                        printf("D: Sleeping...\n");
                        sleep(SLEEP_PROC_SEC);
			printf("D: Exiting...\n");
                        exit(13);
                }

                /*Process B doing after D is created...*/
                printf("Waiting for D to terminate...\n");
               // sleep(SLEEP_TREE_SEC);
                pid1 = wait(&status);
                explain_wait_status(pid1, status);
                printf("B: Exiting...\n");
                exit(19);
        }


        /*Create child C of A*/
        pid_t pid2 = fork();
        if (pid2 < 0) 
        {
                perror("C fork failed...\n");
                exit(17);
        }
        /*Proccess C doing...*/
        if (pid2 == 0) {
                change_pname("C");

		printf ("C: Initiating...\n");
                printf ("C: Sleeping...\n");
                sleep(SLEEP_PROC_SEC);
                printf("C: EXiting...\n");
                exit(17);
        }


        /*Process A doing*/
        printf("A: Waiting for children..\n");
        //sleep(SLEEP_PROC_SEC);
        pid1 = wait(&status);
        explain_wait_status(pid1, status);
        pid2 = wait(&status);
        explain_wait_status(pid2, status);
        printf("A: Exiting...\n");
        exit(16);
}

/*
 * The initial process forks the root of the process tree,
 * waits for the process tree to be completely created,
 * then takes a photo of it using show_pstree().
 *
 In ask2-{fork, tree}:
 *      wait for a few seconds, hope for the best.

 * How to wait for the process tree to be ready?
* In ask2-signals:
 *      use wait_for_ready_children() to wait until
 *      the first process raises SIGSTOP.
 */
int main(void)
{
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
                fork_procs();
                exit(1);
        }

        /*
         * Father
         */
        /* for ask2-signals */
        /* wait_for_ready_children(1); */

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


