include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "proc-common.h"
#include "tree.h"

#define SLEEP_PROC_SEC 10
#define SLEEP_TREE_SEC 3

void fork_procs(struct tree_node *node, int pfdparent)
{

    change_pname(node->name);
    pid_t pid[node->nr_children];
    int status, i;

    printf("%s: Created\n", node->name);
    if (node->nr_children != 0)
    {
        int pfd[2], data[2], result;
        if (pipe(pfd) < 0)
        {
                perror("pipe");
                exit(1);
        }
        //printf("%s: Waiting for children.\n", node->name);
        for (i = 0; i < node->nr_children; i++)
        {

            /* Fork root of process tree */
            pid[i] = fork();
            if (pid[i] < 0)
            {
                perror("fork");
                exit(1);
            }
            if (pid[i] == 0)
            {

                fork_procs(node->children + i, pfd);
            }

            pid[i] = wait(&status);
            explain_wait_status(pid[i], status);
            read(pfd[0], &data[i], 4);
        }
        result = data[0] + data[1];
        write(pfdparent[1], result, sizofe(result));
        exit(1);
    }
    else
    {
        int x = (int)(node->name);
        write(pfdparent[0], &x, sizeof(x));
        

        printf("%s: Write %d\n", node->name, x);
        exit(1);
    }
}
int main(int argc, char **argv)
{
        int p[2];
    if (pipe(p) < 0)
        exit(1);

    struct tree_node *root;
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <input_tree_file>\n\n", argv[0]);
    exit(1);
    }

    root = get_tree_from_file(argv[1]);

    if (root != NULL)
    {
        pid_t pid;
        int status, result;
        /* Fork root of process tree */
        pid = fork();
        if (pid < 0)
        {
            perror("main: fork");
            exit(1);
        }
        if (pid == 0)
        {
            /* Child */
            fork_procs(root, p);
            exit(1);
        }

           explain_wait_status(pid, status);
        read(p[0], &result, sizeof(result));
        printf("THE ANSWER IS %d\n", result);
    }

    return 0;
}


