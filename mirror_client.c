
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>

#include "mirror_functions.h"

//The fixed size of the event buffer:
#define EVENT_SIZE (sizeof(struct inotify_event))

//The size of the read buffer: estimate 1024 events with 16 bytes per name over and above the fixed size above
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))

const char *target_type(struct inotify_event *event);
const char *target_name(struct inotify_event *event);

int terminate1 =  0;
int terminate2 = 0;

void sigint_handler(int sig)
{
    terminate1 = 1;
}

void sigquit_handler(int sig)
{
    terminate2 = 1;
}

int receiver_signal = 0;

void sigusr1_handler(int sig)
{
    receiver_signal = receiver_signal + 1;
}

int main(int argc, char *argv[])
{
    if (argc < 13)
    {
        fprintf(stderr, "Error 01: Expected more argument(s) , %d given.\n", argc);
        return -1;
    }

    signal(SIGINT, sigint_handler);
    signal(SIGQUIT, sigquit_handler);
    signal(SIGUSR1, sigusr1_handler);

    int id;
    int buffer_size;
    char *common;
    char *input;
    char *mirror;
    char *log_file;

    /*take the arguments from command line */
    for (int i = 1; i < argc; i++)
    {
        if (!strcmp("-n", argv[i]))
        {
            for (int j = 0; j < strlen(argv[i + 1]); j++)
            {
                if ((argv[i + 1][j] < '0') || (argv[i + 1][j] > '9'))
                {
                    fprintf(stderr, "ID must have only digits , \"%s\" given.\n", argv[i + 1]);
                    return -2;
                }
            }

            id = atoi(argv[i + 1]);
            i++;
        }
        else if (!strcmp("-c", argv[i]))
        {
            common = (char *)malloc((strlen(argv[i + 1]) + 1) * sizeof(char));
            if (common == NULL)
            {
                fprintf(stderr, "Error in malloc at mirror_client.c\n");
                return -3;
            }

            strcpy(common, argv[i + 1]);

            i++;
        }
        else if (!strcmp("-i", argv[i]))
        {
            input = (char *)malloc((strlen(argv[i + 1]) + 1) * sizeof(char));
            if (input == NULL)
            {
                fprintf(stderr, "Error in malloc at mirror_client.c\n");
                return -3;
            }

            strcpy(input, argv[i + 1]);

            i++;
        }
        else if (!strcmp("-m", argv[i]))
        {
            mirror = (char *)malloc((strlen(argv[i + 1]) + 1) * sizeof(char));
            if (mirror == NULL)
            {
                fprintf(stderr, "Error in malloc at mirror_client.c\n");
                return -3;
            }

            strcpy(mirror, argv[i + 1]);
            i++;
        }
        else if (!strcmp("-b", argv[i]))
        {
            for (int j = 0; j < strlen(argv[i + 1]); j++)
            {
                if ((argv[i + 1][j] < '0') || (argv[i + 1][j] > '9'))
                {
                    fprintf(stderr, "Buffer_size must have only digits , \"%s\" given.\n", argv[i + 1]);
                    return -2;
                }
            }

            buffer_size = atoi(argv[i + 1]);
            i++;
        }
        else if (!strcmp("-l", argv[i]))
        {
            log_file = (char *)malloc((strlen(argv[i + 1]) + 1) * sizeof(char));
            if (log_file == NULL)
            {
                fprintf(stderr, "Error in malloc at mirror_client.c\n");
                return -3;
            }

            strcpy(log_file, argv[i + 1]);

            i++;
        }
    }

    DIR *input_dir = opendir(input);
    if (input_dir == NULL)
    {
        free(mirror);
        free(input);
        free(common);
        free(log_file);

        fprintf(stderr, "Input directory does not exist.\n");
        return -4;
    }
    closedir(input_dir);

    DIR *mirror_dir = opendir(mirror);
    if (mirror_dir != NULL)
    {
        closedir(mirror_dir);

        free(mirror);
        free(input);
        free(common);
        free(log_file);

        fprintf(stderr, "Mirror directory already exists.\n");
        return -4;
    }
    else
    {
        if (mkdir(mirror, 0755) < 0)
        {
            fprintf(stderr, "Error: Failed to create mirror directory %s", mirror);
            return -5;
        }
    }


    DIR *common_dir = opendir(common);
    if (common_dir == NULL)
    {
        if (mkdir(common, 0755) < 0)
        {
            fprintf(stderr, "Error: Failed to create common directory %s", common);
            return -6;
        }
        common_dir = opendir(common);
        if (common_dir == NULL)
        {
            fprintf(stderr,"Error in opening the comon directory\n");
            return -10;
        }
    }

    int valid = produce_id_file_to_common_dir(common, id);
    if (valid < 0)
    {
        free(mirror);
        free(input);
        free(common);
        free(log_file);

        if (valid == -1)
            fprintf(stderr, "ID file already exist in common directory.\n");
        return -7;
    }

    char id_filename[15];
    sprintf(id_filename,"%d.id",id);
    struct dirent *dirent_ptr;
    printf("HERE\n");
    while ((dirent_ptr = readdir(common_dir)) != NULL)
    {
        if ( (strstr(dirent_ptr->d_name, ".id") != NULL) && ( strcmp(dirent_ptr->d_name,id_filename) != 0))  
        {
            char id_str[15];
            char buff_size[15];
            sprintf(id_str, "%d", id);
            sprintf(buff_size, "%d", buffer_size);
            char new_id[15];
            int i;
            for (i = 0; i < strlen(dirent_ptr->d_name); i++)
            {
                if (dirent_ptr->d_name[i] == '.')
                    break;
                new_id[i] = dirent_ptr->d_name[i];
            }
            new_id[i] = '\0';

            pid_t pid1 = fork();
            if (pid1 < 0)
            {
                fprintf(stderr, "Error in fork at mirror_client.c .\n");
                return -11;
            }
            else if (pid1 == 0)
            {
                execl("receiver", "receiver", common, id_str, new_id, mirror, buff_size, (char *)NULL);

                fprintf(stderr, "Error in execl at mirror_client.c .\n");
                return -12;
            }
            else
            {
                pid_t pid2 = fork();
                if (pid2 < 0)
                {
                    fprintf(stderr, "Fail in fork at mirror_client.c .\n");
                    return -11;
                }
                else if (pid2 == 0)
                {
                    execl("sender", "sender", common, id_str, new_id, input, buff_size, (char *)NULL);

                    fprintf(stderr, "Error in execl at mirror_client.c .\n");
                    return -12;
                }
                else
                {
                    int status;

                    if (receiver_signal > 0)
                        printf("SIGNAL CAME\n");

                    while ((waitpid(pid1, &status, WNOHANG)) == 0)
                        ;

                    while ((waitpid(pid2, &status, WNOHANG)) == 0)
                        ;
                }
            }
        }
    }
    closedir(common_dir);
    printf("END\n");

    int fd = inotify_init();
    if (fd < 0)
    {
        fprintf(stderr, "Fail to initialize inotify.\n");
        return -8;
    }

    int wd;
	wd = inotify_add_watch(fd,common, IN_ALL_EVENTS);
	if (wd == -1)
	{	fprintf(stderr,"Failed to add watch %s\n", common);
        return -9;
    }

    int length = 0, read_ptr = 0 , read_offset = 0; //management of variable length events
    char buffer[EVENT_BUF_LEN];        //the buffer to use for reading the events
    struct inotify_event *event;

    int flags = fcntl(fd, F_GETFL, 0);
    if(fcntl(fd, F_SETFL, flags | O_NONBLOCK))
    {
        fprintf(stderr,"Error in fcntl\n");
        return -10;
    }

    char *deleted_mirror = NULL;

    while(( !terminate1 ) && (!terminate2) )
    {
        length = read(fd, buffer + read_offset, sizeof(buffer) - read_offset);
        if (length < 0) 
            continue;

        length += read_offset; // if there was an offset, add it to the number of bytes to process
        read_ptr = 0;

        while(read_ptr + EVENT_SIZE <= length)
        {
            event = (struct inotify_event *)&buffer[read_ptr];

            if (read_ptr + EVENT_SIZE + event->len > length)
                break;

            if (event->mask & IN_CREATE)
            {
                printf("WD:%i in_create %s %s COOKIE=%u\n", event->wd, target_type(event), target_name(event), event->cookie);
                if ( strstr(event->name,".id") != NULL)
                {   
                    char id_str[15] ;
                    char buff_size[15];
                    sprintf(id_str,"%d",id);
                    sprintf(buff_size,"%d",buffer_size);
                    char new_id[15];
                    int i;
                    for(i = 0; i < event->len ; i++)
                    {
                        if (event->name[i] == '.')
                            break;
                        new_id[i] = event->name[i]; 
                    }
                    new_id[i] = '\0';

                    pid_t pid1 = fork();
                    if (pid1 < 0)
                    {
                        fprintf(stderr,"Error in fork at mirror_client.c .\n");
                        return -11;
                    }
                    else if (pid1 == 0) 
                    {   
                        execl("receiver", "receiver", common , id_str ,new_id , mirror , buff_size, (char *)NULL);

                        fprintf(stderr,"Error in execl at mirror_client.c .\n");
                        return -12;
                    }
                    else
                    {
                        pid_t pid2 = fork();
                        if (pid2 < 0)
                        {
                            fprintf(stderr, "Fail in fork at mirror_client.c .\n");
                            return -11;
                        }
                        else if (pid2 == 0)
                        {
                            execl("sender", "sender", common, id_str , new_id , input, buff_size , (char *)NULL);

                            fprintf(stderr, "Error in execl at mirror_client.c .\n");
                            return -12;
                        }
                        else
                        {   int status;

                            if (receiver_signal > 0)
                                printf("SIGNAL CAME\n");

                            while ((waitpid(pid1, &status, WNOHANG)) == 0);

                            while ((waitpid(pid2, &status, WNOHANG)) == 0);
                        }
                    }
                    
                }
            }
            else if (event->mask & IN_DELETE)
            {
                printf("WD:%i in_delete %s %s COOKIE=%u\n", event->wd , target_type(event), target_name(event), event->cookie);

                pid_t pid3 = fork();
                if (pid3 < 0)
                {
                    fprintf(stderr, "Error in fork at mirror_client.c .\n");
                    return -11;
                }
                else if (pid3 == 0)
                {
                    char deleted_id[15];
                    int i;
                    for (i = 0; i < event->len; i++)
                    {
                        if (event->name[i] == '.')
                            break;
                        deleted_id[i] = event->name[i];
                    }
                    deleted_id[i] = '\0';

                    deleted_mirror = (char*)malloc( (strlen(mirror) + strlen(deleted_id) + 2 ) * sizeof(char));
                    if (deleted_mirror == NULL)
                    {
                        fprintf(stderr, "Error in malloc at mirror_client.c\n");
                        return -3;
                    }
                    
                    strcpy(deleted_mirror,mirror);
                    strcat(deleted_mirror,"/");
                    strcat(deleted_mirror,deleted_id);

                    execlp("rm", "rm", "-rf" , deleted_mirror , (char *)NULL);

                    fprintf(stderr, "Error in execl at mirror_client.c .\n");
                    return -12;
                }
                else
                {   int status ;
                    while((waitpid(pid3,&status , WNOHANG)) == 0 );
                }
            }
           
            //advance read_ptr to the beginning of the next event
            read_ptr += EVENT_SIZE + event->len;
        }
        
        if (read_ptr < length)
        {   
            //copy the remaining bytes from the end of the buffer to the beginning of it
            memcpy(buffer, buffer + read_ptr, length - read_ptr);
            //and signal the next read to begin immediatelly after them
            read_offset = length - read_ptr;
        }
        else
            read_offset = 0;
    }

    free(deleted_mirror);

    char id_str[16];
    sprintf(id_str, "%d.id", id);

    char *id_file;

    id_file = (char *)malloc((strlen(common) + strlen(id_str) + 2) * sizeof(char));
    if (id_file == NULL)
    {
        fprintf(stderr, "Error in malloc at mirror_functions.c");
        return -2;
    }

    strcpy(id_file, common);
    strcat(id_file, "/");
    strcat(id_file, id_str);

    if (remove(id_file) == 0)
        printf("Deleted %s successfully\n",id_file);
    else
        printf("Unable to delete the file %s\n",id_file);
    free(id_file);

    pid_t pid4 = fork();
    if (pid4 < 0)
    {
        fprintf(stderr, "Error in fork at mirror_client.c .\n");
        return -11;
    }
    else if (pid4 == 0)
    {
        printf("Now will delete the client's mirror with id :%d \n",id);
        execlp("rm", "rm", "-rf", mirror, (char *)NULL);

        fprintf(stderr, "Error in execl at mirror_client.c .\n");
        return -12;
    }
    else
    {
        int status;
        while ((waitpid(pid4, &status, WNOHANG)) == 0)
            ;
    }

    free(mirror);
    free(input);
    free(common);
    free(log_file);
}

const char *target_type(struct inotify_event *event)
{
    if (event->len == 0)
        return "";
    else
        return (event->mask & IN_ISDIR) ? "directory" : "file";
}

const char *target_name(struct inotify_event *event)
{
    return event->len > 0 ? event->name : NULL;
}
