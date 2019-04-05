#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

extern int terminate_sender;


int communication_sender_protocol(char *dir_name, char *subdir, int writefd, char *buffer, int buffer_size, FILE *log)
{
    if (terminate_sender == 1)
        return -1;

    DIR *dir = opendir(dir_name);
    if (dir == NULL)
    {
        fprintf(stderr, "Error in opening input dir. \n");
        return -3;
    }
    printf("START OF LOOP SENDER\n");

    struct dirent *dirent_ptr;

    while ((dirent_ptr = readdir(dir)) != NULL)
    {   
        if ( (strcmp(dirent_ptr->d_name, ".") != 0) && ( strcmp(dirent_ptr->d_name, "..") != 0))
        {
            unsigned short int name_size = 0;
            if (subdir == NULL)
            {   printf("NAME:   %s\n", dirent_ptr->d_name);
                name_size = strlen(dirent_ptr->d_name) + 1 ;
            }
            else
            {
                printf("NAME:   %s/%s\n",subdir,dirent_ptr->d_name);
                name_size = strlen(dirent_ptr->d_name) + strlen(subdir) + 2 ;
            }
            int bytes = write(writefd, &name_size, sizeof(unsigned short int ));
            if (bytes < 0)
            {
                fprintf(stderr, "Error in write at sender_functions.c \n");
            }
            if (bytes != 2)
            {
                fprintf(log,"You should write only 2 bytes here , according to the ptotocol, %d bytes written\n", bytes);
            }
            else
                fprintf(log,"Wrote %d bytes\n",bytes);

            char *file_name = NULL;
            
            if (subdir == NULL)
                bytes = write(writefd, dirent_ptr->d_name, name_size);
            else
            {
                file_name = (char*) malloc( (strlen(subdir)+ strlen(dirent_ptr->d_name)+ 2 ) * sizeof(char));
                if (file_name == NULL)
                {
                    fprintf(stderr,"Error in malloc at sender_functions.c\n");
                    return -9;
                }
                strcpy(file_name,subdir);
                strcat(file_name,"/");
                strcat(file_name,dirent_ptr->d_name);
                bytes = write(writefd, file_name, name_size);
                //free(file_name);
            }
            
            if (bytes < 0)
            {
                fprintf(stderr, "Error in write at sender.c \n");
            }
            if (bytes != name_size)
            {
                fprintf(log,"You should write only %d bytes here , according to the ptotocol, %d bytes written\n", name_size, bytes);
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
            unsigned int file_size = (unsigned int)st.st_size;

            int is_dir = 0;
            if ((st.st_mode & S_IFMT) == __S_IFDIR)
            {
                is_dir = 1;
                file_size = 0;
            }
            bytes = write(writefd, &(file_size), sizeof(unsigned int));
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
                fprintf(log,"You should write only 4 bytes here , according to the ptotocol, %d bytes written\n", bytes);
            }
            else
                fprintf(log,"Wrote %d bytes(filesize: %d)\n", bytes, file_size);

            unsigned short int dir = 1, no_dir = 0;

            if (is_dir == 1)
                bytes = write(writefd, &dir, sizeof(unsigned short int));
            else
                bytes = write(writefd, &no_dir, sizeof(unsigned short int));
            if (bytes < 0)
            {
                fprintf(stderr, "Error in write at sender.c \n");
                return -1;
            }
            if (bytes != 2)
            {
                free(full_path_name);
                fprintf(log,"You should write only 2 bytes here , according to the ptotocol, %d bytes written\n", bytes);
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
                    new_sub_dirname = (char *)malloc((strlen(dirent_ptr->d_name) + strlen(subdir) + 2) * sizeof(char));
                    if (new_sub_dirname == NULL)
                    {
                        fprintf(stderr, "Error in malloc at sender_functions.c\n");
                        return -1;
                    }
                    strcpy(new_sub_dirname,subdir);
                    strcat(new_sub_dirname,"/");
                    strcat(new_sub_dirname,dirent_ptr->d_name);
                }
                fprintf(log, "Wrote the directory \"%s\" %d bytes\n", new_sub_dirname, file_size);
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
                int read_bytes = 0;
                int write_bytes = 0;
                while (file_size >= buffer_size)
                {
                    int r = read(fd, buffer, buffer_size);
                    int w = write(writefd, buffer, r);
                    file_size = file_size - r;

                    read_bytes = read_bytes + r;
                    write_bytes = write_bytes + w;
                }

                if (file_size != 0)
                {
                    int r = read(fd, buffer, file_size);
                    int w = write(writefd, buffer, r);

                    read_bytes = read_bytes + r;
                    write_bytes = write_bytes + w;
                }
                if (write_bytes == read_bytes)
                {
                    if (subdir == NULL)
                        fprintf(log, "Wrote %d bytes (the whole file \"%s\") \n", read_bytes, dirent_ptr->d_name);
                    else
                        fprintf(log, "Wrote %d bytes (the whole file \"%s\") \n", read_bytes, file_name);
                }
                else
                {
                    fprintf(log, "Something went wrong: Read %d bytes write %d bytes\n", read_bytes, write_bytes);
                }
            }
            if (subdir != NULL)
                free(file_name);

            close(fd);
            free(full_path_name);
        }
    }

    closedir(dir);
    return 1;
}