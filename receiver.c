
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <time.h>
extern int errno;

void sigalrm_handler(int sig)
{
    printf("The 30 seconds have passed and nothing was in the pipe to read\n");
    kill(getppid(), SIGUSR2);
    printf("%ld\n",time(NULL));
}

int receiver_terminate = 0;

void sigint_handler(int sig)
{
    receiver_terminate = 1;
}

int main(int argc, char *argv[])
{
    signal(SIGINT,sigint_handler);
    signal(SIGALRM, sigalrm_handler);

    if (receiver_terminate == 1)
        return -1;
    char *buffer = NULL;
    int buffer_size = atoi(argv[5]);
    buffer = (char *)malloc(buffer_size * sizeof(char));
    if (buffer == NULL)
    {
        fprintf(stderr, "Error in malloc at receiver.c .\n");
        return -1;
    }

    char * mirror = NULL;
    
    mirror = (char*) malloc(  (strlen(argv[4]) + strlen(argv[3])+2) * sizeof(char) );
    if (mirror == NULL)
    {
        fprintf(stderr, "Error in malloc at receiver.c .\n");
        return -1;
    }

    strcpy(mirror,argv[4]);
    strcat(mirror,"/");
    strcat(mirror,argv[3]);

    DIR *mirror_dir = opendir(mirror);
    if (mirror_dir != NULL)
    {
        closedir(mirror_dir);
    }
    else
    {
        if (mkdir(mirror, 0755) < 0)
        {
            fprintf(stderr, "Error: Failed to create mirror directory %s\n", mirror);
            return -5;
        }
    }

    char *fifo_name;

    fifo_name = (char *)malloc((strlen(argv[1]) + strlen(argv[2]) + strlen(argv[3]) + strlen("_to_") + strlen(".fifo") + 2) * sizeof(char));
    if (fifo_name == NULL)
    {
        fprintf(stderr, "Error in malloc at receiver.c .\n");
        free(mirror);
        return -1;
    }

    strcpy(fifo_name, argv[1]);
    strcat(fifo_name, "/");
    strcat(fifo_name, argv[3]);
    strcat(fifo_name, "_to_");
    strcat(fifo_name, argv[2]);
    strcat(fifo_name, ".fifo");
    printf("%s\n", fifo_name);
    if ((mkfifo(fifo_name, 0666) < 0) && (errno != EEXIST) )
    {   
        kill(getppid(),SIGUSR1);
        fprintf(stderr, "Error in creation of fifo at receiver.c \n");
        unlink(fifo_name);
        free(fifo_name);
        return -2;
    }

    FILE *log = fopen(argv[6], "a");
    if (log == NULL)
    {
        fprintf(stderr, "Error can not open log_file for writing in sender.c\n");
        return -5;
    }

    int readfd;
    printf("START OF RECEIVER\n");

    printf("%ld\n",time(NULL));
    alarm(30); 
    //fifo_name[5]= 'a';
    if ((readfd = open(fifo_name, O_RDONLY)) < 0)
    {
        kill(getppid(),SIGUSR1);
        fprintf(stderr, "Error can not open fifo for reading in receiver.c\n");
        return -5;
    }
    alarm(0);
    int bytes = 0;
    unsigned short int  name_size = 0; 
    printf("START OF LOOP RECEIVER\n");
    while ( (  (bytes = read(readfd , &name_size , sizeof(unsigned short int))) > 0) && (name_size != 0) &&(receiver_terminate == 0))
    {   
        fprintf(log,"Read %d bytes(name size: %d)\n",bytes,name_size);
        char * file_name = NULL;
        file_name = (char*) malloc( name_size * sizeof(char));
        if(file_name == NULL)
        {
            fprintf(stderr,"Error in malloc at receiver.c\n");
            return -7;
        }
        
        int bytes2 = 0;
        bytes2 = read(readfd , file_name , name_size);
        if (bytes2 < 0)
        {
            fprintf(stderr, "Error in read at receiver.c \n");
            kill(getppid(), SIGUSR1);
            return -4;
        }
        if (bytes2 != name_size)
        {   fprintf(log,"You should read only %d bytes here , according to the protocol, %d bytes read\n", name_size,bytes2);
        }
        else
            fprintf(log,"Read %d bytes\n",bytes2);
        
        printf("FILENAME: %s\n",file_name);
        unsigned int file_size = 0;
        bytes2 = read(readfd, &(file_size), sizeof(unsigned int));
        if (bytes2 < 0)
        {
            fprintf(stderr, "Error in read at receiver.c \n");
            kill(getppid(), SIGUSR1);
            return -1;
        }
        if (bytes2 != 4)
        {   fprintf(log,"You should read only 4 bytes here , according to the protocol, %d bytes read\n", bytes2);
        }
        else
            fprintf(log,"Read %d bytes (file size: %d)\n", bytes2,file_size); 

        char *full_path = NULL;
        full_path = (char *)malloc((strlen(file_name) + strlen(mirror) + 2) * sizeof(char));
        if (full_path == NULL)
        {
            fprintf(stderr, "Error in malloc at receiver.c\n");
            return -7;
        }
        strcpy(full_path, mirror);
        strcat(full_path, "/");
        strcat(full_path, file_name);

        unsigned short int  is_dir;
        bytes2 = read(readfd, &(is_dir), sizeof(unsigned short int ));
        if (bytes2 < 0)
        {
            fprintf(stderr, "Error in read at receiver.c \n");
            kill(getppid(), SIGUSR1);
            return -4;
        }
        if (bytes2 != 2)
        {   fprintf(log,"You should read only 2 bytes here , according to the protocol, %d bytes read\n", bytes2);
        }
        else
            fprintf(log,"Read %d bytes (is dir : %d)\n", bytes2, is_dir);

        if ((file_size == 0) && ( is_dir == 1) )
        {
            if (mkdir(full_path, 0755) < 0)
            {
                fprintf(stderr, "Error: Failed to create directory %s\n",full_path );
                return -5;
            }
            fprintf(log, "Read the directory \"%s\" %d bytes\n", file_name, file_size);
            continue;
        }

        int fd = open(full_path, O_WRONLY | O_CREAT);
        if (fd >= 0)
        {   int read_bytes = 0;
            int write_bytes = 0;
            while (file_size >= buffer_size)
            {
                int r = read(readfd, buffer, buffer_size);
                int w = write(fd, buffer, r);
                file_size = file_size - r;

                read_bytes = read_bytes + r;
                write_bytes = write_bytes + w;
            }

            if ( file_size != 0)
            {   
                int r = read(readfd, buffer, file_size);
                int w = write(fd, buffer, r);

                read_bytes = read_bytes + r;
                write_bytes = write_bytes + w;
            }

            if (read_bytes == write_bytes)
                fprintf(log, "Read the whole file \"%s\" %d bytes\n", file_name, read_bytes);
            else
                fprintf(log, "Read %d bytes write %d bytes\n", read_bytes, write_bytes);
        } 
        close(fd);
        free(file_name);
        free(full_path);
    }

    fprintf(log,"END:Read %d bytes\n",bytes);
    printf("END OF RECEIVER\n");
    unlink(fifo_name);
    close(readfd);
    fclose(log);
    free(fifo_name);
    free(mirror);
    free(buffer);
    return 0;
}