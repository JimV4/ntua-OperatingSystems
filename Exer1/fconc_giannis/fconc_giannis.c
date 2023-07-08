#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

void doWrite(int fd, char *buff)
{
    size_t Len, idx;
    ssize_t wcnt;
    idx = 0;
    Len = strlen(buff);

    // writes buff into the output file

    do
    {
        wcnt = write(fd, buff + idx, Len - idx);
        idx += wcnt;
    } while (idx < Len);
}

void write_file(int fd, const char *infile)
{
    int fdread;
    fdread = open(infile, O_RDONLY);

    // perform read(...)
    char buff[1024];
    ssize_t rcnt;
    for (;;)
    {
        rcnt = read(fdread, buff, sizeof(buff) - 1);
        if (rcnt == 0) /* end-of-file */
            break;
        buff[rcnt] = '\0';
    }
    doWrite(fd, buff);

    close(fdread);
}

int main(int argc, char **argv)
{
    // check if not enough or too many filew are given

    if (argc != 4 && argc != 3) {
        printf("Usage: ./fconc infile1 infile2 [outfile (default:fconc.out)]\n");
        exit(1);
    }

    // checks if the files exists

    
    for (int i = 1; i < 3; i++)
    {
        int fdread;
        fdread = open(argv[i], O_RDONLY);
        if (fdread == -1)
        {
            perror(argv[i]);
            exit(1);
        }

        close(fdread);
    }

    int fd, oflags, mode;
    oflags = O_CREAT | O_WRONLY | O_TRUNC;
    mode = S_IRUSR | S_IWUSR;
    fd = open(argv[3], oflags, mode);
    // creats output file if none exists

    if (fd == -1)
    {
        FILE *f;
        f = fopen("./fconc.out", "w");
        fclose(f);
        fd = open("./fconc.out", oflags, mode);
    }

    // perform write(...)
    write_file(fd, argv[1]);
    write_file(fd, argv[2]);
    close(fd);
    return 0;
}



