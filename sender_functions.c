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

extern int terminate_sender;

/*function to implement the  communication protocol from sender's side*/
int communication_sender_protocol(char *dir_name, char *subdir, int writefd, char *buffer, int buffer_size, FILE *log,int log_fd)
{
    if (terminate_sender == 1)/*a signal SIGINT is reached so return -1 as error */
        return -1;

    DIR *dir = opendir(dir_name);
    if (dir == NULL)
    {
        fprintf(stderr, "Error in opening input dir. \n");
        return -3;
    }

    struct dirent *dirent_ptr;

    /*read all the files and subdirectories of the input directory which is the dir name*/
    while ((dirent_ptr = readdir(dir)) != NULL)
    {   
        if ( (strcmp(dirent_ptr->d_name, ".") != 0) && ( strcmp(dirent_ptr->d_name, "..") != 0))
        {
            unsigned short int name_size = 0;
            if (subdir == NULL)/*find the name-size*/
                name_size = strlen(dirent_ptr->d_name) + 1 ;
            else
                name_size = strlen(dirent_ptr->d_name) + strlen(subdir) + 2 ;
            
            /*write 2 bytes in fifo which are the size of the name of the file/subdir*/
            int bytes = write(writefd, &name_size, sizeof(unsigned short int ));
            
            flock(log_fd,LOCK_EX); /*lock the file desctiptor of log file*/
            if (bytes < 0) /*error*/
            {
                fprintf(stderr, "Error in write at sender_functions.c \n");
            }
            if (bytes != 2) /*if wrote more bytes than 2*/
            {
                fprintf(log,"You should write only 2 bytes here , according to the ptotocol, %d bytes written\n", bytes);
            }
            else /*write the informations in log file*/
            {
                fprintf(log,"Wrote %d bytes\n",bytes);
            }
            fflush(log);
            flock(log_fd, LOCK_UN); /*unlock the file descriptor of log file*/

            char *file_name = NULL;
            
            if (subdir == NULL)/*if you are not on a sub directory*/
            {
                /*write the name of the file/subdir*/
                bytes = write(writefd, dirent_ptr->d_name, name_size);
            }
            else/*if you are on a sub directory*/
            {
                file_name = (char*) malloc( (strlen(subdir)+ strlen(dirent_ptr->d_name)+ 2 ) * sizeof(char));
                if (file_name == NULL)
                {
                    fprintf(stderr,"Error in malloc at sender_functions.c\n");
                    return -9;
                }
                /*send the path as file_name (path starts after mirror directory)*/
                strcpy(file_name,subdir);
                strcat(file_name,"/");
                strcat(file_name,dirent_ptr->d_name);
    
                bytes = write(writefd, file_name, name_size);
            }

            flock(log_fd, LOCK_EX); /*lock the file desctiptor of log file*/
            if (bytes < 0)
            {
                fprintf(stderr, "Error in write at sender.c \n");
            }
            if (bytes != name_size) /*error if you write in pipe more or less bytes than name_size */
            {
                fprintf(log,"You should write only %d bytes here , according to the ptotocol, %d bytes written\n", name_size, bytes);
            }
            else
            {   
                fprintf(log,"Wrote %d bytes\n", bytes); /*information in log file*/
            }
            fflush(log);
            flock(log_fd, LOCK_UN); /*unlock the file desctiptor of log file*/

            char* full_path_name = (char*) malloc( (strlen(dir_name)+ strlen(dirent_ptr->d_name)+2) *sizeof(char));
            if (full_path_name == NULL)
            {
                fprintf(stderr, "Error in malloc at sender_functions.c\n");
                return -5;
            }
            /*make the path absolute path of the file/subir*/
            strcpy(full_path_name, dir_name);               /* mirror */
            strcat(full_path_name, "/");                    /* mirror/ */
            strcat(full_path_name, dirent_ptr->d_name);     /* mirrror/name */
            
            struct stat st;
            stat(full_path_name, &st);
            unsigned int file_size = (unsigned int)st.st_size; /* take the file size */

            int is_dir = 0;
            if ((st.st_mode & S_IFMT) == __S_IFDIR) /* check if it is a directory*/
            {
                is_dir = 1;
                file_size = 0; /*if it is a directory make the file size = 0*/
            }
            
            /*write 4 bytes that are the file size */
            bytes = write(writefd, &(file_size), sizeof(unsigned int));

            flock(log_fd, LOCK_EX); /*lock the file desctiptor of log file*/
            if (bytes < 0) /*error */
            {
                free(full_path_name);
                closedir(dir);
                flock(log_fd, LOCK_UN);
                fprintf(stderr, "Error in write at sender.c \n");
                return -1;
            }
            if (bytes != 4) /*error if you write in pipe more or less bytes than 4 */
            {
                free(full_path_name);
                closedir(dir);
                fprintf(log,"You should write only 4 bytes here , according to the ptotocol, %d bytes written\n", bytes);
            }
            else
            {   
                fprintf(log,"Wrote %d bytes(filesize: %d)\n", bytes, file_size); /*information in log file */
            }
            
            fflush(log);
            flock(log_fd, LOCK_UN); /*unlock the file desctiptor of log file*/

            unsigned short int dir = 1, no_dir = 0;

            /*write 2 bytes in pipe (1 or 0) that means if it is a directory or not*/
            if (is_dir == 1)
                bytes = write(writefd, &dir, sizeof(unsigned short int));
            else
                bytes = write(writefd, &no_dir, sizeof(unsigned short int));

            flock(log_fd, LOCK_EX); /*lock the file desctiptor of log file*/
            if (bytes < 0) /* error */
            {
                fprintf(stderr, "Error in write at sender.c \n");
                flock(log_fd, LOCK_UN);
                return -1;
            }
            if (bytes != 2) /*error if you write in pipe more or less bytes than 2 */
            {
                free(full_path_name);
                fprintf(log,"You should write only 2 bytes here , according to the ptotocol, %d bytes written\n", bytes);
            }
            else
            {
                fprintf(log,"Wrote %d bytes (is dir : %d)\n", bytes, is_dir);
            }
            fflush(log);
            flock(log_fd, LOCK_UN); /*unlock the file desctiptor of log file*/

            if (is_dir == 1) /* if it is a directory */
            {
                char *new_sub_dirname = NULL;
                if (subdir == NULL) /*if it is in input directory(level = 0) */
                {
                    new_sub_dirname = (char *)malloc((strlen(dirent_ptr->d_name) + 1) * sizeof(char));
                    if (new_sub_dirname == NULL)
                    {
                        free(full_path_name);
                        fprintf(stderr, "Error in malloc at sender_functions.c\n");
                        return -5;
                    }
                    strcpy(new_sub_dirname,dirent_ptr->d_name); /* just take the name */
                }
                else /*if it is not in level = 0 */
                {
                    new_sub_dirname = (char *)malloc((strlen(dirent_ptr->d_name) + strlen(subdir) + 2) * sizeof(char));
                    if (new_sub_dirname == NULL)
                    {
                        fprintf(stderr, "Error in malloc at sender_functions.c\n");
                        return -1;
                    }
                    /*make the path*/
                    strcpy(new_sub_dirname,subdir);             /* /subdir */  
                    strcat(new_sub_dirname,"/");                /* /subdir/ */
                    strcat(new_sub_dirname,dirent_ptr->d_name); /* /subdir/next_subdir */
                }

                flock(log_fd, LOCK_EX); /*lock the file desctiptor of log file*/
                fprintf(log, "Wrote %d bytes (the directory \"%s\" )\n",file_size, new_sub_dirname); /*information in logfile */
                fflush(log);
                flock(log_fd, LOCK_UN); /*un lock the file desctiptor of log file*/

                /*call anadromic function for the subdir */
                /*full_path_name has the absolute paht of the directory */
                /* new_sub_dirname has the sub-path after input/ */
                int res = communication_sender_protocol(full_path_name,new_sub_dirname, writefd , buffer , buffer_size,log,log_fd);
                free(new_sub_dirname);
                if (res < 0 ) /*there was an error*/
                {
                    if (subdir != NULL)
                        free(file_name);
                    free(full_path_name);
                    fprintf(stderr,"Something went wrong\n");
                    if (res == -1)
                        return -1;
                    else
                        return -5;
                }
                continue; /*go to the next file/subdir*/
            }

            int fd = open(full_path_name, O_RDONLY); /* open the file to write its context to the named pipe*/
            if (fd >= 0)
            {   
                unsigned int temp_file_size = file_size;
                int read_bytes = 0;
                int write_bytes = 0;
                /*cut the file in chunks and write them in pipe */
                while (temp_file_size >= buffer_size) /* make chunks while the temp_file_size is bigger than buffer size*/
                {
                    int r = read(fd, buffer, buffer_size); /*read from the file buffer_size bytes*/
                    int w = write(writefd, buffer, r); /*write to the pipe the r bytes that read before*/
                    
                    temp_file_size = temp_file_size - r; /*reduce the temp_file_size by r bytes */

                    read_bytes = read_bytes + r;
                    write_bytes = write_bytes + w;
                }
                /*the temp_file_size is smaller than buffer size now */
                while(temp_file_size > 0)
                {
                    int r = read(fd, buffer, temp_file_size); /* read from the file the bytes that left*/
                    int w = write(writefd, buffer, r);        /*write to the pipe the r bytes that read before*/

                    temp_file_size = temp_file_size - r; /*reduce the temp_file_size by r bytes */

                    read_bytes = read_bytes + r;
                    write_bytes = write_bytes + w;
                }

                flock(log_fd, LOCK_EX); /*lock the file desctiptor of log file*/
                if ((write_bytes == read_bytes) && ( write_bytes == file_size)) /*check if the file transfered correctly*/
                {
                    if (subdir == NULL) /*information to log file */
                        fprintf(log, "Wrote %d bytes ( sent the whole file \"%s\" ) \n", read_bytes, dirent_ptr->d_name);
                    else
                        fprintf(log, "Wrote %d bytes ( sent the whole file \"%s\" ) \n", read_bytes, file_name);
                }
                else
                {
                    fprintf(log,"Something went wrong: Read %d bytes write %d bytes(size: %d)\n", read_bytes, write_bytes,file_size);
                }
                fflush(log);
                flock(log_fd, LOCK_UN); /* unlock the file desctiptor of log file*/
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