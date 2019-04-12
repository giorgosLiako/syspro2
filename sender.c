
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/errno.h>

#include "sender_functions.h"

int terminate_sender = 0;

void sigint_handler(int sig) /*signal SIGINT */
{
    terminate_sender = 1;
}

extern int errno;

int main(int argc , char* argv[])
{
    signal(SIGINT, sigint_handler); 

    if (terminate_sender == 1) /*a signal SIGINT is reached from father*/
        return -1;
   
    char* fifo_name;

    char* buffer = NULL;
    int buffer_size = atoi(argv[5]); /* argv[5] = buffer size */
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
    /*make the path of the fifo file*/
    strcpy(fifo_name, argv[1]); /* argv[1] = common directory */
    strcat(fifo_name, "/");
    strcat(fifo_name, argv[2]); /* argv[2] = id of client */
    strcat(fifo_name, "_to_");
    strcat(fifo_name, argv[3]); /* argv[3] = id of the other client*/
    strcat(fifo_name, ".fifo");

    if ( (mkfifo(fifo_name, 0666) < 0) && (errno != EEXIST)) /*create the fifo file*/
    {
        /*if an error occured in creation of fifo file and is not that already exist*/
        kill(getppid(),SIGUSR1);  /*send a signal SIGUSR1 to father*/
        fprintf(stderr, "Error in creation of fifo at sender.c \n");
        unlink(fifo_name);
        free(fifo_name);/*free and return*/
        return -2;
    }

    int writefd;
    printf("Start of Sender \n");

    if ((writefd = open(fifo_name, O_WRONLY)) < 0) /*open the named pipe*/
    {   /*if an error occured send a signal SIGUSR1 to father and return */
        kill(getppid(), SIGUSR1);
        fprintf(stderr,"Error can not open fifo for writing in sender.c\n");
        return -5;
    }
    /*argv[6] = log file */
    FILE* log = fopen(argv[6] ,"a"); /*open log file*/
    if (log == NULL )
    {
        fprintf(stderr, "Error can not open log_file for writing in sender.c\n");
        return -5;
    }

    int log_fd = fileno(log); /*take the file descriptor of log file*/
    if ( log_fd < 0 )
    {
        fprintf(stderr, "Error can not have file descriptor of log_file in sender.c\n");
        return -5;
    }

    /*call the function that is responsible to implement the communication protocol from sender's side*/
    /* argv[4] = input directory */
    int res = communication_sender_protocol(argv[4] ,NULL, writefd ,buffer , buffer_size, log, log_fd);
    if (res < 0)
    {   /*there was an error */
        fprintf(stderr,"Something went wrong\n");
        if (res == -1) /*free the allocatedresources and return*/
        {
            unlink(fifo_name);
            close(writefd);
            fclose(log);
            close(log_fd);
            free(fifo_name);
            free(buffer);
        }
        return -1;
    }
    unsigned short int end = 0;
    /*write the 2 bytes that means the end*/
    int bytes = write(writefd, &(end), sizeof(unsigned short int));
    
    flock(log_fd,LOCK_EX); /*lock the fd*/
    
    if (bytes < 0) /*error*/
    {   flock(log_fd,LOCK_UN); 
        fprintf(stderr, "Error in write at sender.c \n");
        return -4;
    }
    if (bytes != 2) /*error if you write in pipe more or less bytes than 2 */
    {   
        fprintf(log,"You should write only 2 bytes here , according to the ptotocol, %d bytes written\n", bytes);
    }
    else
        fprintf(log,"Wrote %d bytes (end)\n",bytes); /*information to log_file*/
    
    fflush(log); /*transfer the data directly to file */
    flock(log_fd, LOCK_UN); /*unlock the fd*/
    printf("End of Sender\n");

    unlink(fifo_name); /*free the allocated resources and return*/
    close(writefd);
    fclose(log);
    close(log_fd);
    free(fifo_name);
    free(buffer);

    return 0 ;
}