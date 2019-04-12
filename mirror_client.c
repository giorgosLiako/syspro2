
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

/*The fixed size of the event buffer:*/
#define EVENT_SIZE (sizeof(struct inotify_event))

/*The size of the read buffer: estimate 1024 events with 16 bytes per name over and above the fixed size above*/
#define EVENT_BUF_LEN (32 * (EVENT_SIZE + 16))

int terminate1 =  0;
int terminate2 = 0; /*global variables that are connected with the signals*/

void sigint_handler(int sig) /* signal SIGINT*/
{
    terminate1 = 1;
}

void sigquit_handler(int sig) /*signal SIGQUIT*/
{
    terminate2 = 1;
}

int user2_signals = 0;

void sigusr2_handler(int sig) /*signal SIGUSR2 */
{
    user2_signals = 1; /*the 30 seconds have passed from receiver*/
}

int user1_signals = 0;

void sigusr1_handler(int signo, siginfo_t *si,void* data) /*signal SIGUSR1*/
{
    user1_signals = 1;
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
    signal(SIGUSR2, sigusr2_handler);
                                        /*signals handlers*/
    static struct sigaction act;
    act.sa_sigaction = sigusr1_handler;
    act.sa_flags = SA_SIGINFO;
    sigfillset(&(act.sa_mask));
    sigaction(SIGUSR1, &act, NULL);

    int id;
    int buffer_size;
    char *common = NULL;
    char *input = NULL;
    char *mirror = NULL;
    char *log_file = NULL;

    /*take the arguments from command line */
    for (int i = 1; i < argc; i++)
    {
        if (!strcmp("-n", argv[i])) /*id*/
        {
            for (int j = 0; j < strlen(argv[i + 1]); j++) 
            {
                if ((argv[i + 1][j] < '0') || (argv[i + 1][j] > '9'))/*check if id has only digits*/
                {
                    fprintf(stderr, "ID must have only digits , \"%s\" given.\n", argv[i + 1]);
                    return -2;
                }
            }

            id = atoi(argv[i + 1]);
            i++;
        }
        else if (!strcmp("-c", argv[i])) /*common directory*/
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
        else if (!strcmp("-i", argv[i])) /*input directory*/
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
        else if (!strcmp("-m", argv[i])) /*mirror directory*/
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
        else if (!strcmp("-b", argv[i])) /*buffer size*/
        {
            for (int j = 0; j < strlen(argv[i + 1]); j++)
            {
                if ((argv[i + 1][j] < '0') || (argv[i + 1][j] > '9')) /*check buffer size to have only digits*/
                {
                    fprintf(stderr, "Buffer_size must have only digits , \"%s\" given.\n", argv[i + 1]);
                    return -2;
                }
            }

            buffer_size = atoi(argv[i + 1]);
            i++;
        }
        else if (!strcmp("-l", argv[i])) /*name of log life*/
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
    if (input_dir == NULL) /*check if input dir already exists*/
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
    if (mirror_dir != NULL) /*check if mirror directory already exists*/
    {
        closedir(mirror_dir);

        free(mirror);
        free(input);
        free(common);
        free(log_file);

        fprintf(stderr, "Mirror directory already exists.\n");
        return -4;
    }
    else /*if mirror doesnt exist create it */
    {
        if (mkdir(mirror, 0755) < 0)
        {
            fprintf(stderr, "Error: Failed to create mirror directory %s", mirror);
            return -5;
        }
    }


    DIR *common_dir = opendir(common);
    if (common_dir == NULL) /*check if common dir is already exist*/
    {
        if (mkdir(common, 0755) < 0) /*if it doesnt exist create it and open it*/
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

    /*function to create the id file in the common directory*/
    int valid = produce_id_file_to_common_dir(common, id);
    if (valid < 0)
    {   /*already exists the id file so exit the system */
        free(mirror);
        free(input);
        free(common);
        free(log_file);

        if (valid == -1)
            fprintf(stderr, "ID file already exist in common directory.\n");
        return -7;
    }
    
    FILE *log = fopen(log_file, "a");/*open the log_file*/
    if (log == NULL)
    {
        fprintf(stderr, "Error can not open log_file for writing in sender.c\n");
        return -5;
    }
    fprintf(log,"Log file of client with id %d\n",id);/*write the first line of the log file*/
    fclose(log);

    char id_filename[15];
    sprintf(id_filename,"%d.id",id);
    struct dirent *dirent_ptr;
    int retry = 0;
    
    /*synchronize the new client with the clients that are already connected in the system*/
    while ((dirent_ptr = readdir(common_dir)) != NULL) /*search the common directory for id files*/
    {
        if ( (strstr(dirent_ptr->d_name, ".id") != NULL) && ( strcmp(dirent_ptr->d_name,id_filename) != 0))  
        {   /*find an id file that is different from this client's id file*/

            char id_str[15];
            char buff_size[15];
            sprintf(id_str, "%d", id);
            sprintf(buff_size, "%d", buffer_size);
            char new_id[15];
            int i;
start_again:
            for (i = 0; i < strlen(dirent_ptr->d_name); i++)/*take the id from the file name*/
            {
                if (dirent_ptr->d_name[i] == '.')
                    break;
                new_id[i] = dirent_ptr->d_name[i];
            }
            new_id[i] = '\0';/*the id of the other client*/
  
            pid_t pid1 = fork();
            if (pid1 < 0)
            {
                fprintf(stderr, "Error in fork at mirror_client.c .\n");
                return -11;
            }
            else if (pid1 == 0)
            {
                /*execute the receiver*/
                /*receiver's arguments: common directory , id of this client , id of the already connected client , mirror directory, 
                  buffer size , log file */
                execl("receiver", "receiver", common, id_str, new_id, mirror, buff_size,log_file, (char *)NULL);

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
                    /*execute the sender */
                    /*sender's arguments: common directory , id of this client , id of the already connected client , input directory,
                      buffer size , log file */
                    execl("sender", "sender", common, id_str, new_id, input, buff_size ,log_file, (char *)NULL);

                    fprintf(stderr, "Error in execl at mirror_client.c .\n");
                    return -12;
                }
                else
                {
                    int status1 = 0 , status2 = 0;

                    /*wait receiver to end , unless a signal SIGUSR1 or SIGUSR2 has came to this process*/
                    while (((waitpid(pid1, &status1, WNOHANG)) == 0) && (user2_signals == 0) &&(user1_signals == 0))
                        ;

                    if (user2_signals == 1) /*this signal means that receiver was waiting for 30 seconds for read and pipe was empty*/
                    {
                        printf("30 seconds have passed and nothing was in the pipe to read for receiver,exit the system\n");
                        closedir(common_dir);
                        kill(pid2,SIGINT); /*send a signal to sender to terminate*/
                        goto end;
                    }

                    if (user1_signals == 1)/*this signal means that receiver or sender had an error with creating or opening the pipe*/
                    {
                        user1_signals = 0;
                        retry = retry + 1; 
                        kill(pid2, SIGINT); /*send a signal to terminate sender*/
                        kill(pid1 ,SIGINT); /*send a signal to terminate receiver if hs not terminated yet*/
                        if (retry < 3) /*try 3 times and if the error exists exit the system*/
                        {
                            printf("Error in the transfer files for %d time(s),try again\n", retry);
                            goto start_again; /*try again*/
                        }
                        else /*3 times tried and 3 times exists the error so exit the system*/
                        {
                            printf("Error in the transfer files for %d times,exit the system\n", retry);
                            closedir(common_dir); 
                            goto end; /*go to the end */
                        }
                    }

                    /*wait sender to end , unless a signal SIGUSR1 or SIGUSR2 has came to this process*/
                    while (((waitpid(pid2, &status2, WNOHANG)) == 0) && (user1_signals == 0))
                        ;

                    if (user1_signals == 1) /*this signal means that receiver or sender had an error with creating or opening the pipe*/
                    {   
                        user1_signals = 0;
                        retry = retry + 1;
                        if (retry < 3) /*try 3 times and if the error exists exit the system*/
                        {
                            printf("Error in the transfer files for %d time(s),try again\n",retry);
                            goto start_again;
                        }
                        else
                        {
                            printf("Error in the transfer files for %d times,exit the system\n", retry);
                            closedir(common_dir);
                            goto end;
                        }
                        
                    }
                }   
            }
            printf("Successful transfer files between clients %d and %s\n", id, new_id);
        }
    }
    closedir(common_dir);

    int fd = inotify_init(); /*initialize inotify*/
    if (fd < 0)
    {
        fprintf(stderr, "Fail to initialize inotify.\n");
        return -8;
    }

    int wd;
	wd = inotify_add_watch(fd,common, IN_CREATE | IN_DELETE); /*watch common directory*/
	if (wd == -1)
	{	fprintf(stderr,"Failed to add watch %s\n", common);
        return -9;
    }

    int length = 0, read_ptr = 0 , read_offset = 0; //management of variable length events
    char buffer[EVENT_BUF_LEN];        //the buffer to use for reading the events
    struct inotify_event *event;

    int flags = fcntl(fd, F_GETFL, 0);
    if(fcntl(fd, F_SETFL, flags | O_NONBLOCK))/*make read in line 389 non block*/
    {
        fprintf(stderr,"Error in fcntl\n");
        return -10;
    }

    char deleted_mirror[150];

    while(( !terminate1 ) && (!terminate2) )
    {
        length = read(fd, buffer + read_offset, sizeof(buffer) - read_offset);/*read events*/
        if (length < 0) 
            continue;

        length += read_offset; // if there was an offset, add it to the number of bytes to process
        read_ptr = 0;

        while(read_ptr + EVENT_SIZE <= length) /*do it until you reach the number of bytes that read in line 389*/
        {
            event = (struct inotify_event *)&buffer[read_ptr]; /*cast*/

            if (read_ptr + EVENT_SIZE + event->len > length) 
                break;

            if ( (event->mask & IN_CREATE) && (strstr(event->name, ".id") != NULL) ) 
            {   /*a new id file is created in common directory */
                
                char id_str[15];
                char buff_size[15];
                sprintf(id_str, "%d", id);
                sprintf(buff_size, "%d", buffer_size);
                char new_id[15];
                int i;
start_again2:
                for (i = 0; i < event->len; i++)
                {
                    if (event->name[i] == '.')
                        break;
                    new_id[i] = event->name[i];
                }
                new_id[i] = '\0'; /*the id of the other client*/

                pid_t pid1 = fork();
                if (pid1 < 0)
                {
                    fprintf(stderr, "Error in fork at mirror_client.c .\n");
                    return -11;
                }
                else if (pid1 == 0)
                {
                    /*execute the receiver*/
                    /*receiver's arguments: common directory , id of this client , id of the already connected client , mirror directory, 
                     buffer size , log file */
                    execl("receiver", "receiver", common, id_str, new_id, mirror, buff_size, log_file, (char *)NULL);
                            
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
                        /*execute the sender */
                        /*sender's arguments: common directory , id of this client , id of the already connected client , input directory,
                        buffer size , log file */
                        execl("sender", "sender", common, id_str, new_id, input, buff_size, log_file, (char *)NULL);

                        fprintf(stderr, "Error in execl at mirror_client.c .\n");
                        return -12;
                    }
                    else
                    {
                        int status1 = 0, status2 = 0;
                        /*wait receiver to end , unless a signal SIGUSR1 or SIGUSR2 has came to this process*/
                        while (((waitpid(pid1, &status1, WNOHANG)) == 0) && (user2_signals == 0) && (user1_signals == 0))
                            ;

                        if (user2_signals == 1) /*this signal means that receiver was waiting for 30 seconds for read and pipe was empty*/
                        {
                            printf("30 seconds have passed and nothing was in the pipe to read for receiver,exit the system\n");
                            kill(pid2, SIGINT); /*send a signal to sender to terminate*/
                            goto end;
                        }

                        if (user1_signals == 1) /*this signal means that receiver or sender had an error with creating or opening the pipe*//*this signal means that receiver or sender had an error with creating or opening the pipe*/
                        {
                            user1_signals = 0;
                            retry = retry + 1;
                            kill(pid2, SIGINT); /*send a signal to terminate sender*/
                            kill(pid1, SIGINT); /*send a signal to terminate receiver if hs not terminated yet*/
                            if (retry < 3)
                            {
                                printf("Error in the transfer files for %d time(s),try again\n", retry);
                                goto start_again2;
                            }
                            else
                            {
                                printf("Error in the transfer files for %d times,exit the system\n", retry);
                                goto end;
                            }
                        }

                        /*wait sender to end , unless a signal SIGUSR1 or SIGUSR2 has came to this process*/
                        while (((waitpid(pid2, &status2, WNOHANG)) == 0) && (user1_signals == 0))
                        ;

                        if (user1_signals == 1)/*this signal means that receiver or sender had an error with creating or opening the pipe*/
                        {
                            user1_signals = 0;

                            retry = retry + 1;
                            if (retry < 3)
                            {
                                printf("Error in the transfer files for %d time(s),try again\n", retry);
                                goto start_again2;
                            }
                            else
                            {
                                printf("Error in the transfer files for %d times,exit the system\n", retry);
                                goto end;
                            }
                        }
                    }
                    printf("Successful transfer files between clients %d and %s\n", id, new_id);
                }
                
            }
            else if ( (event->mask & IN_DELETE) && (strstr(event->name, ".id") != NULL) )
            {
                /*an id file is deleted  in common directory */

                char deleted_id[15];
                int i;
                for (i = 0; i < event->len; i++)
                {
                    if (event->name[i] == '.')
                        break;
                    deleted_id[i] = event->name[i];
                }
                deleted_id[i] = '\0'; /*the id of the deleted client*/

                pid_t pid3 = fork();
                if (pid3 < 0)
                {
                    fprintf(stderr, "Error in fork at mirror_client.c .\n");
                    return -11;
                }
                else if (pid3 == 0)
                {
                    strcpy(deleted_mirror, mirror);
                    strcat(deleted_mirror, "/");
                    strcat(deleted_mirror, deleted_id);/*make the path of the deleted mirror*/

                    /*execute rm -rf deleted mirror */
                    execlp("rm", "rm", "-rf", deleted_mirror, (char *)NULL);

                    fprintf(stderr, "Error in execl at mirror_client.c .\n");
                    return -12;
                }
                else
                {
                    int status;
                    /*wait until rm end*/
                    while ((waitpid(pid3, &status, WNOHANG)) == 0)
                    ;
                    printf("Client %s deleted\n", deleted_id);
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


    char id_str[16];
end:
    sprintf(id_str, "%d.id", id);

    char *id_file = NULL;

    id_file = (char *)malloc((strlen(common) + strlen(id_str) + 2) * sizeof(char));
    if (id_file == NULL)
    {
        fprintf(stderr, "Error in malloc at mirror_functions.c");
        return -2;
    }

    strcpy(id_file, common);
    strcat(id_file, "/");
    strcat(id_file, id_str);/*make the path of the id file in common directory*/

    if (remove(id_file) == 0) /*delete the id file of this client*/
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
        /*execute rm -rf to redelete the mirror directory of this client*/
        printf("Now will delete the client's mirror with id :%d \n",id);
        execlp("rm", "rm", "-rf", mirror, (char *)NULL);

        fprintf(stderr, "Error in execl at mirror_client.c .\n");
        return -12;
    }
    else
    {
        int status;
        /*wait until rm end*/
        while ((waitpid(pid4, &status, WNOHANG)) == 0)
            ;
    }
    log = fopen(log_file, "a");
    if (log == NULL)
    {
        fprintf(stderr, "Error can not open log_file for writing in sender.c\n");
        return -5;
    }
    fprintf(log,"Client leaved the system\n");
    fclose(log);
    /*exit*/
    printf("Successful exit from the system for client %d\n",id);
    
    /*free the allocated resources*/
    free(mirror);
    free(input);
    free(common);
    free(log_file);
}

