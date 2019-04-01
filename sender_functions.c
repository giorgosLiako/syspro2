#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

int communication_sender_protocol(char* dir_name , char* subdir ,int writefd, char* buffer ,int buffer_size, FILE* log)
{
    DIR *dir = opendir(dir_name);
    if (dir == NULL)
    {
        fprintf(stderr, "Error in opening input dir. \n");
        return -3;
    }
    printf("START OF LOOP SENDER\n");
    char log_buffer[50];
    struct dirent *dirent_ptr;

    while ((dirent_ptr = readdir(dir)) != NULL)
    {
        if (strcmp(dirent_ptr->d_name, ".") && strcmp(dirent_ptr->d_name, ".."))
        {
            unsigned short name_size = 0;
            if (subdir == NULL)
            {   printf("NAME:   %s\n", dirent_ptr->d_name);
                name_size = strlen(dirent_ptr->d_name) + 1 ;
            }
            else
            {
                printf("NAME:   %s/%s\n",subdir,dirent_ptr->d_name);
                name_size = strlen(dirent_ptr->d_name) + strlen(subdir) + 2 ;
            }
            int bytes = write(writefd, &name_size, sizeof(unsigned short));
            if (bytes < 0)
            {
                fprintf(stderr, "Error in write at sender_functions.c \n");
                return -1;
            }
            if (bytes != 2)
            {
                printf("You should write only 2 bytes here , according to the ptotocol, %d bytes written\n", bytes);
                return -1;
            }
            else
                fprintf(log,"Wrote %d bytes\n",bytes);
            
            
            //write(log_fd,)

            if (subdir == NULL)
                bytes = write(writefd, dirent_ptr->d_name, name_size);
            else
            {
                char* file_name = (char*) malloc( (strlen(subdir)+ strlen(dirent_ptr->d_name)+1) * sizeof(char));
                if (file_name == NULL)
                {
                    fprintf(stderr,"Error in malloc at sender_functions.c\n");
                    return -9;
                }
                strcpy(file_name,subdir);
                strcat(file_name,"/");
                strcat(file_name,dirent_ptr->d_name);
                bytes = write(writefd, file_name, name_size);
                free(file_name);
            }
            
            if (bytes < 0)
            {
                fprintf(stderr, "Error in write at sender.c \n");
                return -1;
            }
            if (bytes != name_size)
            {
                printf("You should write only %d bytes here , according to the ptotocol, %d bytes written\n", name_size, bytes);
                return -1;
            }
            else
            {   fprintf(log,"Wrote %d bytes\n", bytes);
                
            }
            char* full_path_name = (char*) malloc( (strlen(dir_name)+ strlen(dirent_ptr->d_name)+2) *sizeof(char));
            if (full_path_name == NULL)
            {
                fprintf(stderr, "Error in malloc at sender_functions.c\n");
                return -5;
            }
            strcpy(full_path_name, dir_name);
            strcat(full_path_name, "/");
            strcat(full_path_name, dirent_ptr->d_name);
            struct stat st;
            stat(full_path_name, &st);
            int file_size = (int)st.st_size;

            int is_dir = 0;
            if ((st.st_mode & S_IFMT) == __S_IFDIR)
            {
                is_dir = 1;
                file_size = 0;
            }
            bytes = write(writefd, &(file_size), 4);
            if (bytes < 0)
            {
                free(full_path_name);
                closedir(dir);
                fprintf(stderr, "Error in write at sender.c \n");
                return -1;
            }
            if (bytes != 4)
            {
                free(full_path_name);
                closedir(dir);
                printf("You should write only 4 bytes here , according to the ptotocol, %d bytes written\n", bytes);
                return -1;
            }
            else
                fprintf(log,"Wrote %d bytes(filesize: %d)\n", bytes, file_size);

            unsigned short dir = 1, no_dir = 0;

            if (is_dir == 1)
                bytes = write(writefd, &dir, sizeof(unsigned short));
            else
                bytes = write(writefd, &no_dir, sizeof(unsigned short));
            if (bytes < 0)
            {
                fprintf(stderr, "Error in write at sender.c \n");
                return -1;
            }
            if (bytes != 2)
            {
                free(full_path_name);
                printf("You should write only 2 bytes here , according to the ptotocol, %d bytes written\n", bytes);
            }
            else
                fprintf(log,"Wrote %d bytes (is dir : %d)\n", bytes, is_dir);

            if (is_dir == 1)
            {
                char *new_sub_dirname = NULL;
                if (subdir == NULL)
                {
                    new_sub_dirname = (char *)malloc((strlen(dirent_ptr->d_name) + 1) * sizeof(char));
                    if (new_sub_dirname == NULL)
                    {
                        free(full_path_name);
                        fprintf(stderr, "Error in malloc at sender_functions.c\n");
                        return -5;
                    }
                    strcpy(new_sub_dirname,dirent_ptr->d_name);
                }
                else
                {
                    new_sub_dirname = (char *)malloc((strlen(dirent_ptr->d_name) + strlen(subdir) + 1) * sizeof(char));
                    if (new_sub_dirname == NULL)
                    {
                        fprintf(stderr, "Error in malloc at sender_functions.c\n");
                        return -1;
                    }
                    strcpy(new_sub_dirname,subdir);
                    strcat(new_sub_dirname,"/");
                    strcat(new_sub_dirname,dirent_ptr->d_name);
                }
                
                int res = communication_sender_protocol(full_path_name,new_sub_dirname, writefd , buffer , buffer_size,log);
                free(new_sub_dirname);
                if (res < 0 )
                {
                    free(full_path_name);
                    fprintf(stderr,"Something went wrong\n");
                    if (res == -1)
                        return -1;
                    else
                        return -5;
                }
                continue;
            }

            int fd = open(full_path_name, O_RDONLY);
            if (fd >= 0)
            {
                int writen_bytes = 0;
                int read_bytes = 0;
                int k = 0;
                if (file_size <= buffer_size)
                {
                    int s =0;
                    s = read(fd, buffer, file_size);
                    if (s > 0)
                    {
                        read_bytes = read_bytes + s;
                        k = write(writefd, buffer, file_size);
                        writen_bytes = writen_bytes + k;
                    }
                    else 
                    {
                        free(full_path_name);
                        return -1;
                    }
                }
                else
                {
                    int counter = file_size;
                    while (counter > 0)
                    {
                        int s = 0;
                        if (counter >= buffer_size)
                        {
                            s = read(fd, buffer, buffer_size);
                            if (s > 0)
                            {
                                read_bytes = read_bytes + s;
                                
                                k = write(writefd, buffer, buffer_size);
                                writen_bytes = writen_bytes + k;
                            }
                            else
                            {
                                close(fd);
                                free(full_path_name);
                                return -1;
                            }
                            counter = counter - buffer_size;
                        }
                        else
                        {
                            s = read(fd, buffer, counter);
                            if (s > 0)
                            {
                                read_bytes = read_bytes + s;
                                k = write(writefd, buffer, counter);
                                writen_bytes = writen_bytes + k;
                            }
                            else
                            {   close(fd);
                                free(full_path_name);
                                return -1;
                            }
                            counter = 0;
                        }
                    }
                }
                if (writen_bytes == read_bytes)
                    fprintf(log,"Wrote the whole file %d bytes\n", read_bytes);
            }
            close(fd);
            free(full_path_name);
        }
    }

    closedir(dir);
    return 1;
}