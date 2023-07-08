#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>



#include "mandel-lib.h"

#define MANDEL_MAX_ITERATION 100000

/*
 * POSIX thread functions do not return error numbers in errno,
 * but in the actual return value of the function call instead.
 * This macro helps with error reporting in this case.
 */
#define perror_pthread(ret, msg) \
        do { errno = ret; perror(msg); } while (0)

/***************************
* Compile-time parameters *
***************************/

/*
* Output at the terminal is is x_chars wide by y_chars long
*/
int y_chars = 50;
int x_chars = 90;

/*
* The part of the complex plane to be drawn:
* upper left corner is (xmin, ymax), lower right corner is (xmax, ymin)
*/
double xmin = -1.8, xmax = 1.0;
double ymin = -1.0, ymax = 1.0;

/*
* Every character in the final output is
* xstep x ystep units wide on the complex plane.
*/
double xstep;
double ystep;

/* array of semaphores */
sem_t *mutex;

/*--------------Εverythig for threads----------------*/

/*
 * A (distinct) instance of this structure
 * is passed to each thread
 */
struct process_info_struct {
        pid_t pid; /* process id, as returned by the library */

        int *color_val; /* Pointer to array to manipulate. Each process manipulat                                                                                                             es a line and gives each character a color */

        int prid; /* Application-defined process id */
        int nprocs; /* Number of total processes*/
};

int safe_atoi(char *s, int *val)
{
        long l;
        char *endp;

        l = strtol(s, &endp, 10);
        if (s != endp && *endp == '\0') {
                *val = l;
                return 0;
        } else
                return -1;
}

void *safe_malloc(size_t size)
{
        void *p;

        if ((p = malloc(size)) == NULL) {
                fprintf(stderr, "Out of memory, failed to allocate %zd bytes\n",
                        size);
                exit(1);
        }

        return p;
}

/*------------------------------------------------------------------------------                                                                                                             ---*/

void usage(char *argv0)
{
        fprintf(stderr, "Usage: %s thread_count array_size\n\n"
                "Exactly two argument required:\n"
                "    thread_count: The number of threads to create.\n"
                "    array_size: The size of the array to run with.\n",
                argv0);
        exit(1);
}



/*
* This function computes a line of output
* as an array of x_char color values.
*/
void compute_mandel_line(int line, int color_val[])
{
       /*
        * x and y traverse the complex plane.
        */
       double x, y;

       int n;
       int val;

       /* Find out the y value corresponding to this line */
       y = ymax - ystep * line;

       /* and iterate for all points on this line */
       for (x = xmin, n = 0; n < x_chars; x+= xstep, n++) {

               /* Compute the point's color value */
               val = mandel_iterations_at_point(x, y, MANDEL_MAX_ITERATION);
               if (val > 255)
                       val = 255;

               /* And store it in the color_val[] array */
               val = xterm_color(val);
               color_val[n] = val;
       }
}

/*
* This function outputs an array of x_char color values
* to a 256-color xterm.
*/
void output_mandel_line(int fd, int color_val[])
{
       int i;

       char point ='@';
       char newline='\n';

       for (i = 0; i < x_chars; i++) {
               /* Set the current color, then output the point */
               set_xterm_color(fd, color_val[i]);
               if (write(fd, &point, 1) != 1) {
                       perror("compute_and_output_mandel_line: write point");
                       exit(1);
               }
       }

       /* Now that the line is done, output a newline character */
       if (write(fd, &newline, 1) != 1) {
               perror("compute_and_output_mandel_line: write newline");
               exit(1);
       }
}

void *compute_and_output_mandel_line(void *arg)
{
    int i;
    struct process_info_struct *proc = arg;
    int *color_val = proc->color_val;

    /* thread i manipulates lines i, i + n, i + 2n ....*/
    for (i = proc->prid; i < y_chars; i += proc->nprocs) {
        compute_mandel_line(i, color_val);
        sem_wait(&mutex[i % proc->nprocs]);
        output_mandel_line(1, color_val);
        sem_post(&mutex[(i + 1)% proc->nprocs]);
    }

    return NULL;
}

void *create_shared_memory_area(unsigned int numbytes)
{
        int pages;
        void *addr;

        if (numbytes == 0) {
                fprintf(stderr, "%s: internal error: called for numbytes == 0\n", __func__);
                exit(1);
        }

        /*
         * Determine the number of pages needed, round up the requested number of
         * pages
         */
        pages = (numbytes - 1) / sysconf(_SC_PAGE_SIZE) + 1;

        /* Create a shared, anonymous mapping for this number of pages */
        /* TODO:
                addr = mmap(...)
        */
        addr = mmap(mutex, numbytes, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

        return addr;
}

void destroy_shared_memory_area(void *addr, unsigned int numbytes) {
        int pages;

        if (numbytes == 0) {
                fprintf(stderr, "%s: internal error: called for numbytes == 0\n", __func__);
                exit(1);
        }

        /*
         * Determine the number of pages needed, round up the requested number of
         * pages
         */
        pages = (numbytes - 1) / sysconf(_SC_PAGE_SIZE) + 1;

        if (munmap(addr, pages * sysconf(_SC_PAGE_SIZE)) == -1) {
                perror("destroy_shared_memory_area: munmap failed");
                exit(1);
        }
}




int main(int argc, char *argv[])
{
    int i, nprocs;
    struct process_info_struct *proc;

    /*
     * Parse the command line. User gives number of processes nprocs
     */
    if (argc != 2)
            usage(argv[0]);
    if (safe_atoi(argv[1], &nprocs) < 0 || nprocs <= 0) {
            fprintf(stderr, "`%s' is not valid for `thread_count'\n", argv[1]);
            exit(1);
    }


    xstep = (xmax - xmin) / x_chars;
    ystep = (ymax - ymin) / y_chars;

    /* Create array of nprocs processes. Ecah process has a struct with its info */
    proc = safe_malloc(nprocs * sizeof(*proc));

    create_shared_memory_area(nprocs * 4);

    /* Creat array of semaphore of length prcnt */
    mutex = malloc(nprocs * sizeof(*mutex));

    /*
    * draw the Mandelbrot Set, one line at a time.
    * Output is sent to file descriptor '1', i.e., standard output.
    */
        sem_init(&mutex[0], 0, 1);
        for (i = 1; i < nprocs; i++) {
            sem_init(&mutex[i], 0, 0);
        }


        // create nprocs processes
        for (i = 0; i < nprocs; i++) {
            // crate new process and initialize fields of process struct
            proc[i].prid = i;
            proc[i].nprocs = nprocs;
            proc[i].color_val = safe_malloc(x_chars * sizeof(int));
            proc[i].pid = fork();
            if (pid[i] < 0) {
                perror("C failed...\n");
                exit(1);
            }
            /*if (pid[i] == 0) {
                compute_and_output_mandel_line(&proc[i]);
            }*/

            compute_and_output_mandel_line(&proc[i]);

        }

    reset_xterm_color(1);
    return 0;
}
