#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

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

sem_t *mutex;

/*--------------Εverythig for threads----------------*/

/*
 * A (distinct) instance of this structure
 * is passed to each thread
 */
struct thread_info_struct {
        pthread_t tid; /* POSIX thread id, as returned by the library */

        int *color_val; /* Pointer to array to manipulate. Each thread manipulat                                                                                                             es a line and gives each character a color */

        int thrid; /* Application-defined thread id */
        int thrcnt; /* Number of total threads*/
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
    struct thread_info_struct *thr = arg;
    int *color_val = thr->color_val;

    /* thread i manipulates lines i, i + n, i + 2n ....*/
    for (i = thr->thrid; i < y_chars; i += thr->thrcnt) {
        compute_mandel_line(i, color_val);
        sem_wait(&mutex[i % thr->thrcnt]);
        output_mandel_line(1, color_val);
        sem_post(&mutex[(i + 1)% thr->thrcnt]);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    int i, thrcnt, ret;
    struct thread_info_struct *thr;

    /*
     * Parse the command line. User gives number of threads
     */
    if (argc != 2)
            usage(argv[0]);
    if (safe_atoi(argv[1], &thrcnt) < 0 || thrcnt <= 0) {
            fprintf(stderr, "`%s' is not valid for `thread_count'\n", argv[1]);
            exit(1);
    }


    xstep = (xmax - xmin) / x_chars;
    ystep = (ymax - ymin) / y_chars;

    /* Create array of thrcnt threads */
    thr = safe_malloc(thrcnt * sizeof(*thr));

    /* Creat array of mutexes of length trhcnt */
    mutex = malloc(thrcnt * sizeof(*mutex));

    /*
    * draw the Mandelbrot Set, one line at a time.
    * Output is sent to file descriptor '1', i.e., standard output.
    */
        sem_init(&mutex[0], 0, 1);
        for (i = 1; i < thrcnt; i++) {
                sem_init(&mutex[i], 0, 0);
        }

        for (i = 0; i < thrcnt; i++) {
            /* Initialize per-thread structure */
            thr[i].thrid = i;
            thr[i].thrcnt = thrcnt;
            thr[i].color_val = safe_malloc(x_chars * sizeof(int));
            /* Spawn new thread */
            ret = pthread_create(&thr[i].tid, NULL, compute_and_output_mandel_line, &thr[i]);                                                                                                             ne, &thr[i]);
                if (ret) {
                    perror_pthread(ret, "pthread_create");
                    exit(1);
                }
        }

    /*
     * Wait for all threads to terminate
     */
    for (i = 0; i < thrcnt; i++) {
            ret = pthread_join(thr[i].tid, NULL);
            if (ret) {
                    perror_pthread(ret, "pthread_join");
                    exit(1);
            }
    }

    reset_xterm_color(1);
    return 0;
}
