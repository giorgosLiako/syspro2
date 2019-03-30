
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

int main(int argc , char* argv[])
{
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
        fprintf(stderr, "Error in creation of fifo at sender.c \n");
        free(fifo_name);
        return -2;
    }

    int writefd;
    printf("START OF SENDER\n");
    if ((writefd = open(fifo_name, O_WRONLY)) < 0)
    {
        fprintf(stderr,"Error can not open fifo for writing in sender.c\n");
        return -5;
    }

    DIR * input_dir = opendir(argv[4]);
    if (input_dir == NULL)
    {
        fprintf(stderr,"Error in opening input dir. \n");
        return -3;
    }
    printf("START OF LOOP SENDER\n");
    struct dirent *dirent_ptr;

    while ((dirent_ptr = readdir(input_dir)) != NULL)
    {  
        if (strcmp(dirent_ptr->d_name, ".") && strcmp(dirent_ptr->d_name, ".."))
        {
            printf("LOOP\n");
            printf("NAME:   %s\n",dirent_ptr->d_name);
            unsigned short name_size = strlen(dirent_ptr->d_name) +1;
            int bytes = write(writefd,&name_size, sizeof(unsigned short));
            if (bytes < 0)
            {
                fprintf(stderr,"Error in write at sender.c \n");
                return -4;
            }
            if (bytes != 2)
                printf("You should write only 2 bytes here , according to the ptotocol, %d bytes written\n",bytes);
            else
                printf("Wrote 2 bytes\n");
            
            bytes = write(writefd, dirent_ptr->d_name , name_size);
            if (bytes < 0)
            {
                fprintf(stderr, "Error in write at sender.c \n");
                return -4;
            }
            if (bytes != name_size)
                printf("You should write only %d bytes here , according to the ptotocol, %d bytes written\n",name_size, bytes);
            else 
                printf("Wrote %d bytes\n",name_size);

            char b[500];
            strcpy(b,argv[4]);
            strcat(b,"/");
            strcat(b,dirent_ptr->d_name);
            struct stat st;
            stat(b, &st);
            int file_size = (int)st.st_size; 

            int is_dir = 0;
            if ( (st.st_mode & S_IFMT) == __S_IFDIR)
            {   is_dir = 1;
                file_size = 0;
            }
            bytes = write(writefd, &(file_size), 4);
            if (bytes < 0)
            {
                fprintf(stderr, "Error in write at sender.c \n");
                return -4;
            }
            if (bytes != 4)
                printf("You should write only 4 bytes here , according to the ptotocol, %d bytes written\n",bytes);
            else
                printf("Wrote %d bytes(filesize: %d)\n", bytes,file_size);

            unsigned short dir = 1 , no_dir = 0;
            
            if (is_dir == 1)
                bytes = write(writefd, &dir, sizeof(unsigned short));
            else
                bytes = write(writefd, &no_dir , sizeof(unsigned short));    
            if (bytes < 0)
            {
                fprintf(stderr, "Error in write at sender.c \n");
                return -4;
            }
            if (bytes != 2)
                printf("You should write only 2 bytes here , according to the ptotocol, %d bytes written\n", bytes);
            else
                printf("Wrote %d bytes (is dir : %d)\n", bytes, is_dir);

            if (is_dir == 1)
                continue;

            int fd = open(b, O_RDONLY);
            if (fd >= 0)
            {   int writen_bytes = 0;
                if (file_size <= buffer_size)
                {
                    int s;
                    s = read(fd, buffer, file_size);
                    if (s > 0)
                    {   writen_bytes = writen_bytes + s; 
                        write(writefd, buffer, file_size);
                    }
                }
                else
                {
                    int counter = file_size;
                    while (counter > 0)
                    {   
                        int s;
                        if (counter > buffer_size)
                        {   
                            s = read(fd,buffer,buffer_size);
                            if (s > 0)
                            {   writen_bytes = writen_bytes + s; 
                                write(writefd , buffer , buffer_size);
                            }
                            counter = counter - buffer_size;
                        }
                        else
                        {
                            s = read(fd,buffer,counter);
                            if (s > 0)
                            {   writen_bytes = writen_bytes + s; 
                                write(writefd , buffer , counter);
                            }
                            counter = 0 ;
                        }
                        
                    }
                }
                
                printf("Wrote the whole file %d bytes\n",writen_bytes);
                
            }
            close(fd);
        }
    }
    unsigned short end = 0;
    int bytes = write(writefd, &(end), sizeof(unsigned short));
    if (bytes < 0)
    {
        fprintf(stderr, "Error in write at sender.c \n");
        return -4;
    }
    if (bytes != 2)
        printf("You should write only 2 bytes here , according to the ptotocol, %d bytes written\n", bytes);
    else
        printf("Wrote 2 bytes\n");
    printf("END OF SENDER\n");

    unlink(fifo_name);
    close(writefd);
    closedir(input_dir);
    free(fifo_name);
    free(buffer);
    return 0 ;
}