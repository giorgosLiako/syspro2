
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

#include "sender_functions.h"

int terminate_sender = 0;

void sigint_handler(int sig)
{
    terminate_sender = 1;
}

extern int errno;

int main(int argc , char* argv[])
{
    signal(SIGINT, sigint_handler);

    if (terminate_sender == 1)
        return -1;
    char* fifo_name;

    char* buffer = NULL;
    int buffer_size = atoi(argv[5]);
    buffer = (char*) malloc(buffer_size * sizeof(char));
    if (buffer == NULL)
    {
        fprintf(stderr, "Error in malloc at sender.c .\n");
        return -1;
    }

    fifo_name = (char*) malloc( (strlen(argv[1]) + strlen(argv[2])+ strlen(argv[3]) + strlen("_to_") + strlen(".fifo") + 2 ) * sizeof(char) );
    if (fifo_name == NULL)
    {
        fprintf(stderr,"Error in malloc at sender.c .\n");
        return -1;
    }
    strcpy(fifo_name, argv[1]);
    strcat(fifo_name, "/");
    strcat(fifo_name, argv[2]);
    strcat(fifo_name, "_to_");
    strcat(fifo_name, argv[3]);
    strcat(fifo_name, ".fifo");
    printf("%s\n", fifo_name);
    if ( (mkfifo(fifo_name, 0666) < 0) && (errno != EEXIST))
    {
        kill(getppid(),SIGUSR1);
        fprintf(stderr, "Error in creation of fifo at sender.c \n");
        unlink(fifo_name);
        free(fifo_name);
        return -2;
    }

    int writefd;
    printf("START OF SENDER\n");
    //fifo_name[5] = 'a';
    if ((writefd = open(fifo_name, O_WRONLY)) < 0)
    {
        kill(getppid(), SIGUSR1);
        fprintf(stderr,"Error can not open fifo for writing in sender.c\n");
        return -5;
    }

    FILE* log = fopen(argv[6] ,"a");
    if (log == NULL )
    {
        fprintf(stderr, "Error can not open log_file for writing in sender.c\n");
        return -5;
    }

    int res = communication_sender_protocol(argv[4] ,NULL, writefd ,buffer , buffer_size, log);
    if (res < 0)
    {
        fprintf(stderr,"Something went wrong\n");
        if (res == -1)
        {
            unlink(fifo_name);
            close(writefd);
            fclose(log);
            free(fifo_name);
            free(buffer);
        }
        return -1;
    }
    unsigned short int end = 0;
    int bytes = write(writefd, &(end), sizeof(unsigned short int));
    if (bytes < 0)
    {
        fprintf(stderr, "Error in write at sender.c \n");
        return -4;
    }
    if (bytes != 2)
        printf("You should write only 2 bytes here , according to the ptotocol, %d bytes written\n", bytes);
    else
        fprintf(log,"END:Wrote %d bytes\n",bytes);
    printf("END OF SENDER\n");

    unlink(fifo_name);
    close(writefd);
    fclose(log);
    free(fifo_name);
    free(buffer);

    return 0 ;
}