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
#include <sys/types.h>
#include <sys/wait.h>

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
#define y_chars 50
//int y_chars = 50;
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

/* shared buffer between processes */
int *buffer[y_chars];

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


void store_lines_in_buffer(void *arg)
{
    int i, j;
    struct process_info_struct *proc = arg;
    int *color_val = proc->color_val;

    for (i = proc->prid; i < y_chars; i += proc->nprocs) {
        compute_mandel_line(i, color_val);
        for (j = 0; j < x_chars; j++) {
            buffer[i][j] = color_val[j];
        }
    }

}

void *compute_and_output_mandel_line(int fd, int line)
{

   output_mandel_line(fd, buffer[line]);

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
        addr = mmap(NULL, pages, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

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
    int i, nprocs, status;
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

    /* initialize shared buffer where the mandlebrot will be stored */
    for (i = 0; i < y_chars; i++) {
        buffer[i] = create_shared_memory_area(sizeof(int) * x_chars);
    }

        // create nprocs processes
        for (i = 0; i < nprocs; i++) {
            // crate new process and initialize fields of process struct
            proc[i].prid = i;
            proc[i].nprocs = nprocs;
            proc[i].color_val = safe_malloc(x_chars * sizeof(int));
            proc[i].pid = fork();

            if (proc[i].pid < 0) {
                perror("Fork failed...:(");
                exit(1);
            }
            // child
            if(proc[i].pid == 0) {
                /* each child process stores the lines that
                belong to it to the shared buffer */
                store_lines_in_buffer(&proc[i]);
                exit(1);
            }

        }
        // parent waits for each childe proccess to finish
        for (i = 0; i < nprocs; i++) {
            proc[i].pid = wait(&status);
        }


    int line;

    /*
     * draw the Mandelbrot Set, one line at a time.
     * Output is sent to file descriptor '1', i.e., standard output.
     */
    // parent prints the mandlebrot set
    for (line = 0; line < y_chars; line++) {
        compute_and_output_mandel_line(1, line);
    }


    // destroy shared buffer
    for (i = 0; i < y_chars; i++) {
        destroy_shared_memory_area(buffer[i], sizeof(int) * x_chars);
    }

    reset_xterm_color(1);
    return 0;
}
