#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

int produce_id_file_to_common_dir(char* path , int id )
{
    char id_str[16];
    sprintf(id_str,"%d.id",id);

    char * id_file ;

    id_file = (char*)malloc( ( strlen(path) + strlen(id_str) + 2 ) * sizeof(char));
    if (id_file == NULL )
    {
        fprintf(stderr,"Error in malloc at mirror_functions.c");
        return -2;
    }

    strcpy(id_file,path);
    strcat(id_file,"/");
    strcat(id_file,id_str);

    int fd ;
    if ( (fd = open(id_file, O_WRONLY, 0777)) == -1)
    {
        if ( (fd = open(id_file, O_WRONLY | O_CREAT , 0777)) == -1)
        {
            fprintf(stderr,"Error in file id creation.\n");
            return -3;
        }

        pid_t pid = getpid();

        char buf[16];
        sprintf(buf , "%d",pid);

        write(fd,buf, strlen(buf) );
    }
    else
    {   
        free(id_file);
        return -1;
    }
    
    close(fd);
    free(id_file);
    return 0 ;
}