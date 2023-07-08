#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/*writes the content of buff from stream f to the file with file descriptor fd*/
void doWrite(int fd, char *buff)
{
	size_t len, idx;
    ssize_t wcnt;
    idx = 0;
    len = strlen(buff);

    // writes buff into the output file
	
    do
    {
        wcnt = write(fd, buff + idx, len - idx);
		//error
		if (wcnt == -1)
		{ 
			error("write");
		}
        idx += wcnt;
    } while (idx < len);
}

/*writes the content of the file named infile to the file with file descriptor fd*/
void write_file(int fd, const char *infile) 
{
    int fdread = open(infile, O_RDONLY);
	char buff[1024];
	ssize_t rcnt;
	for (;;)
	{
		rcnt = read(fdread, buff, sizeof(buff) - 1);
		/* end‐of‐file */
		if (rcnt == 0) 
			break;
		
		/* error */
		if (rcnt == -1)
		{ 
			perror("read");
			break;
		}
		buff[rcnt] = '\0';
	}
	
	doWrite(fd, buff);

	close(fdread);
}

int main (int argc, char **argv) 
{
	/*check if not enough or too many files given e.g ./fconc A or ./fconc A B C D */
	if (argc != 3 && argc != 4)
	{
		printf("Usage: ./fconc2 infile1 infile2 [outfile (default:fconc2.out)]\n");
		return 0;
	}

	int i;
	for (i = 1; i < 3; i++)
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

	/*if output file given, open output file for writing*/
	if (argv[3] != '\0') 
	{
		fd = open(argv[3], oflags, mode);
	}
	/*if no ouptut file given,
	  create default output file fconc.out*/
	else
	{
		fd = open("fconc.out", oflags, mode);
	}
	
	/*perform wtite(...)  */
	write_file(fd, argv[1]);
	write_file(fd, argv[2]);
	close(fd);
	return 0;
}
	

