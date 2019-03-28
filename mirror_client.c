
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc , char* argv[])
{
    if (argc < 13)
    {
        perror("Error 01: Expected more argument(s).\n");
        exit(-1);
    }

    int id ;
    int buffer_size;
    char* common_dir;
    char* input_dir;
    char* mirror_dir;
    char* log_file;

    /*take the arguments from command line */
    for (int i = 1 ; i < argc ; i++)
    {
        if ( !strcmp("-n", argv[i]) )
        {
            for (int j = 0 ; j < strlen(argv[i+1]) ; j++ )
            {   if ( ( argv[i+1][j] < '0') || ( argv[i+1][j] > '9') )
                {
                    printf("ID must have only digits , \"%s\" given.\n" , argv[i+1]);
                    return -2;
                }
            }

            id = atoi(argv[i+1]);
            i++;
        }
        else if ( !strcmp("-c", argv[i]) )
        {
            common_dir = (char*) malloc( strlen( (argv[i+1])+1) * sizeof(char));
            if (common_dir == NULL)
            {
                printf("Error in malloc at mirror_client.c\n");
                return -3;
            }

            strcpy(common_dir, argv[i+1]);
            i++;
        }
        else if ( !strcmp("-i", argv[i]) )
        {
            input_dir = (char *)malloc(strlen((argv[i + 1]) + 1) * sizeof(char));
            if (input_dir == NULL)
            {
                printf("Error in malloc at mirror_client.c\n");
                return -3;
            }

            strcpy(input_dir, argv[i+1]);

            i++;
        }
        else if ( !strcmp("-m", argv[i]) )
        {
            mirror_dir = (char *)malloc(strlen((argv[i + 1]) + 1) * sizeof(char));
            if (mirror_dir == NULL)
            {
                printf("Error in malloc at mirror_client.c\n");
                return -3;
            }

            strcpy(mirror_dir, argv[i+1]);
            i++;
        }
        else if ( !strcmp("-b", argv[i]) )
        {
            for (int j = 0; j < strlen(argv[i + 1]); j++)
            {
                if ((argv[i + 1][j] < '0') || (argv[i + 1][j] > '9'))
                {
                    printf("Buffer_size must have only digits , \"%s\" given.\n", argv[i + 1]);
                    return -2;
                }
            }

            buffer_size = atoi(argv[i + 1]);
            i++;
        }
        else if ( !strcmp("-l", argv[i]) )
        {
            log_file = (char *)malloc(strlen((argv[i + 1]) + 1) * sizeof(char));
            if (log_file == NULL)
            {
                perror("Error in malloc at mirror_client.c\n");
                exit(-3);
            }

            strcpy(log_file, argv[i + 1]);

            i++;
        }
    }






    free(mirror_dir);
    free(input_dir);
    free(common_dir);
    free(log_file);
}