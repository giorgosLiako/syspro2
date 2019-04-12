
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
#include <time.h>
extern int errno;

int receiver_terminate = 0;

void sigint_handler(int sig) /* signal SIGINT*/
{
    receiver_terminate = 1;
}

void sigalrm_handler(int sig) /* signal SIGALRM */
{
    kill(getppid(), SIGUSR2);
    receiver_terminate = 1;
}

int main(int argc, char *argv[])
{
    signal(SIGINT,sigint_handler);
    signal(SIGALRM, sigalrm_handler);

    if (receiver_terminate == 1) /*a signal is received */
        return -1;

    char *buffer = NULL;
    int buffer_size = atoi(argv[5]); /* argv[5] = buffer size */
    
    buffer = (char *)malloc(buffer_size * sizeof(char));
    if (buffer == NULL)
    {
        fprintf(stderr, "Error in malloc at receiver.c .\n");
        return -1;
    }

    char * mirror = NULL;
    /* argv[2] = the client's id , argv[3] = the other client's id */
    mirror = (char*) malloc(  (strlen(argv[4]) + strlen(argv[3])+2) * sizeof(char) );
    if (mirror == NULL)
    {
        fprintf(stderr, "Error in malloc at receiver.c .\n");
        return -1;
    }

    strcpy(mirror,argv[4]); /* argv[4] = mirror directory*/
    strcat(mirror,"/");
    strcat(mirror,argv[3]); 

    DIR *mirror_dir = opendir(mirror); /*open mirror directory*/
    if (mirror_dir != NULL)
    {
        closedir(mirror_dir);
    } 
    else /*if it doesnt exist create it */
    {
        if (mkdir(mirror, 0755) < 0)
        {
            fprintf(stderr, "Error: Failed to create mirror directory %s\n", mirror);
            return -5;
        }
    }

    char *fifo_name;
    /* argv[1] = common directory */
    fifo_name = (char *)malloc((strlen(argv[1]) + strlen(argv[2]) + strlen(argv[3]) + strlen("_to_") + strlen(".fifo") + 2) * sizeof(char));
    if (fifo_name == NULL)
    {
        fprintf(stderr, "Error in malloc at receiver.c .\n");
        free(mirror);
        return -1;
    }
    /* make the path of the fifo */
    strcpy(fifo_name, argv[1]);         /* common*/
    strcat(fifo_name, "/");             /* common/ */
    strcat(fifo_name, argv[3]);         /* common/id */
    strcat(fifo_name, "_to_");          /* common/id_to */
    strcat(fifo_name, argv[2]);         /* common/id_to_otherid */
    strcat(fifo_name, ".fifo");         /* common/id_to_otherid.fifo*/

    if ((mkfifo(fifo_name, 0666) < 0) && (errno != EEXIST)) /*create the fifo file*/
    { 
        /*if an error occured in creation of fifo file and is not that already exist*/
        kill(getppid(),SIGUSR1);
        fprintf(stderr, "Error in creation of fifo at receiver.c \n");
        unlink(fifo_name);
        free(fifo_name);
        return -2;
    }

    FILE *log = fopen(argv[6], "a"); /* argv[6] = log file */
    if (log == NULL)
    {
        fprintf(stderr, "Error can not open log_file for writing in sender.c\n");
        return -5;
    }

    int log_fd = fileno(log); /* take the file descriptor of the log file */
    if (log_fd < 0)
    {
        fprintf(stderr, "Error can not open log_file for writing in sender.c\n");
        return -5;
    }

    int readfd;
    printf("Start of Receiver\n");

    alarm(30); /* alarm for 30 seconds*/

    if ((readfd = open(fifo_name, O_RDONLY)) < 0)
    {
        kill(getppid(),SIGUSR1);
        fprintf(stderr, "Error can not open fifo for reading in receiver.c\n");
        return -5;
    }
    alarm(0); /* cancel the alarm */

    int bytes = 0;
    unsigned short int  name_size = 0; 
    while (receiver_terminate == 0)
    {
        alarm(30); /* alarm for 30 seconds*/
        bytes = read(readfd, &name_size, sizeof(unsigned short int)); /*read 2 bytes that are the name size*/
        alarm(0); /* cancel the alarm */
        
        if (bytes < 0)
        {
            fprintf(stderr, "Error in read at receiver.c \n");
            return -4;
        }

        if (name_size == 0) /*if name size is 0 this is the end */
            break;

        flock(log_fd, LOCK_EX); /*lock the fd*/
        
        fprintf(log,"Read %d bytes(name size: %d)\n",bytes,name_size);
        fflush(log);
        
        flock(log_fd, LOCK_UN); /*unlock the fd*/

        char * file_name = NULL;
        file_name = (char*) malloc( name_size * sizeof(char));
        if(file_name == NULL)
        {
            fprintf(stderr,"Error in malloc at receiver.c\n");
            return -7;
        }

        int bytes2 = 0;
        
        alarm(30); /* alarm for 30 seconds*/
        bytes2 = read(readfd , file_name , name_size); /* read the file_name */
        alarm(0);    /* cancel the alarm */
        
        flock(log_fd, LOCK_EX); /*lock the fd*/
        if (bytes2 < 0)
        {
            flock(log_fd, LOCK_UN);
            fprintf(stderr, "Error in read at receiver.c \n");
            return -4;
        }
        if (bytes2 != name_size) /* you shouldn't read more or less bytes than name_size */
        {   
            fprintf(log,"You should read only %d bytes here , according to the protocol, %d bytes read\n", name_size,bytes2);
        }
        else
            fprintf(log,"Read %d bytes\n",bytes2); /*information to log file */
        
        fflush(log);
        flock(log_fd, LOCK_UN); /*unlock the fd*/

        unsigned int file_size = 0;
        
        alarm(30); /* alarm for 30 seconds*/
        bytes2 = read(readfd, &(file_size), sizeof(unsigned int)); /*read 4 bytes that are the file size */
        alarm(0); /* cancel the alarm */

        flock(log_fd, LOCK_EX); /*lock the fd*/
        if (bytes2 < 0) /*error */
        {
            flock(log_fd, LOCK_UN);
            fprintf(stderr, "Error in read at receiver.c \n");
            return -1;
        }
        if (bytes2 != 4) /* you shouldn't read more or less than 4 bytes */
        {   
            fprintf(log,"You should read only 4 bytes here , according to the protocol, %d bytes read\n", bytes2);
        }
        else
            fprintf(log,"Read %d bytes (file size: %d)\n", bytes2,file_size); /*information to log file */
        
        fflush(log);
        flock(log_fd, LOCK_UN); /* unlock the fd*/

        char *full_path = NULL;
        full_path = (char *)malloc((strlen(file_name) + strlen(mirror) + 2) * sizeof(char));
        if (full_path == NULL)
        {
            fprintf(stderr, "Error in malloc at receiver.c\n");
            return -7;
        }
        strcpy(full_path, mirror); /*make the full path of the file in mirror directory*/
        strcat(full_path, "/");
        strcat(full_path, file_name);

        unsigned short int  is_dir;
        
        alarm(30); /* alarm for 30 seconds*/
        bytes2 = read(readfd, &(is_dir), sizeof(unsigned short int )); /*read 2 bytes that are if the file is directory or not*/
        alarm(0); /* cancel the alarm */

        flock(log_fd, LOCK_EX); /*lock the fd*/
        if (bytes2 < 0) /* error */
        {
            fprintf(stderr, "Error in read at receiver.c \n");
            return -4;
        }
        if (bytes2 != 2) /*you shouldn't read more or less than 2 bytes */
        {   
            fprintf(log,"You should read only 2 bytes here , according to the protocol, %d bytes read\n", bytes2);
        }
        else
            fprintf(log,"Read %d bytes (is dir : %d)\n", bytes2, is_dir); /*information to log file */
        
        fflush(log);
        flock(log_fd, LOCK_UN); /*unlock the fd*/

        if ((file_size == 0) && ( is_dir == 1) ) /*if it is a directory */
        {
            if (mkdir(full_path, 0755) < 0) /*create the directory */
            {
                fprintf(stderr, "Error: Failed to create directory %s\n",full_path );
                return -5;
            }

            flock(log_fd, LOCK_EX); /*lock the fd*/
            fprintf(log, "Read %d bytes (the directory \"%s\" )\n",file_size ,file_name); /*information to log file */
            fflush(log);
            flock(log_fd, LOCK_UN); /*unlock the fd*/

            continue; /*go to the start and read the next protocol */
        }
        
        unsigned int temp_file_size = file_size;
        
        int fd = open(full_path, O_WRONLY | O_CREAT);
        if (fd >= 0)
        {   
            int read_bytes = 0;
            int write_bytes = 0;
            /*read the file in chunks from the pipe */
            while (temp_file_size >= buffer_size) /* read chunks while the temp_file_size is bigger than buffer size*/
            {
                alarm(30); /* alarm for 30 seconds*/
                int r = read(readfd, buffer, buffer_size); /*read from the pipe buffer_size bytes*/
                alarm(0); /* cancel the alarm */

                int w = write(fd, buffer, r); /*write to the file the r bytes that read before*/
                
                temp_file_size = temp_file_size - r; /*reduce the temp_file_size by r bytes */

                read_bytes = read_bytes + r;
                write_bytes = write_bytes + w;
            }
            /*the temp_file_size is smaller than buffer size now */
            while( temp_file_size > 0)
            {
                alarm(30); /* alarm for 30 seconds*/
                int r = read(readfd, buffer, temp_file_size); /* read from the pipe the bytes that left*/
                alarm(0); /* cancel the alarm */

                int w = write(fd, buffer, r); /*write to the filee the r bytes that read before*/

                temp_file_size = temp_file_size - r; /*reduce the temp_file_size by r bytes */

                read_bytes = read_bytes + r;
                write_bytes = write_bytes + w;
            }

            flock(log_fd, LOCK_EX); /*lock the fd*/
            if ((read_bytes == write_bytes) && (read_bytes == file_size)) /*check if the file received correctly*/
                fprintf(log,"Read %d bytes ( received the whole file \"%s\" ) \n",read_bytes, file_name);
            
            else
                fprintf(log,"Something went wrong: Read %d bytes write %d bytes(size:%d)\n",read_bytes, write_bytes,file_size);
            
            fflush(log);
            flock(log_fd, LOCK_UN); /*unlock the fd*/
        } 
        close(fd);
        free(file_name); /*free resources */
        free(full_path);
    }

    flock(log_fd, LOCK_EX); /*lock the fd*/
    fprintf(log,"Read %d bytes (end)\n",bytes); /*read the 2 bytes that read for the end */
    fflush(log);
    flock(log_fd, LOCK_UN); /*unlock the fd*/

    printf("End of Receiver\n");
    
    unlink(fifo_name);
    close(readfd);
    fclose(log);
    
    free(fifo_name);
    free(mirror);
    free(buffer);
    
    return 0;
}