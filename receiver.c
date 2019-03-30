
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

extern int errno;

void sigalrm_handler(int sig)
{
    printf("The 30 seconds have passed and nothing wa in the pipe to read\n");
    kill(getppid(), SIGUSR1);
}

int main(int argc, char *argv[])
{
    signal(SIGALRM, sigalrm_handler);
    char *buffer = NULL;
    int b_size = atoi(argv[5]);
    buffer = (char *)malloc(b_size * sizeof(char));
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
            fprintf(stderr, "Error: Failed to create mirror directory %s", mirror);
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
        
            fprintf(stderr, "Error in creation of fifo at receiver.c \n");
            free(fifo_name);
            return -2;
    }
    int readfd;
    printf("START OF RECEIVER\n");

    //siginterrupt(SIGALRM,1);
    //alarm(5); 
    if ((readfd = open(fifo_name, O_RDONLY)) < 0)
    {
        fprintf(stderr, "Error can not open fifo for reading in receiver.c\n");
        return -5;
    }
    
    int bytes;
    unsigned short name_size ; 
    while ( (bytes = read(readfd , &name_size , sizeof(unsigned short))) > 0)
    {   printf("Read 2 bytes\n");
        char * file_name = NULL;
        file_name = (char*) malloc( bytes * sizeof(char));
        if(file_name == NULL)
        {
            fprintf(stderr,"Error in malloc at receiver.c\n");
            return -7;
        }
        int bytes2;
        bytes2 = read(readfd , file_name , name_size);
        if (bytes2 < 0)
        {
            fprintf(stderr, "Error in read at receiver.c \n");
            return -4;
        }
        if (bytes2 != name_size)
            printf("You should read only %d bytes here , according to the protocol, %d bytes read\n", name_size,bytes2);
        else
            printf("Read %d bytes\n",bytes2);

        int size;
        bytes2 = read(readfd , &(size) , 4 );
        if (bytes2 < 0)
        {
            fprintf(stderr, "Error in read at receiver.c \n");
            return -4;
        }
        if (bytes2 != 4)
            printf("You should read only 4 bytes here , according to the protocol, %d bytes read\n", bytes2);
        else
            printf("Read %d bytes (file size: %d)\n", bytes2,size);

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
        //printf("%s\n",full_path);
        int fd = open(full_path, O_WRONLY | O_CREAT);
        if (fd > 0)
        {   int s ;
            s = read(readfd,buffer,size)  ; 
            if (s>0)
                write(fd,buffer, size);
            printf("Read the whole file %d bytes\n",s);
        }
        close(fd);
        free(full_path);
        free(file_name);
    }


    printf("END OF RECEIVER\n");
    close(readfd);
    free(fifo_name);
    free(mirror);
    free(buffer);
    return 0;
}