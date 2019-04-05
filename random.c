#include <stdio.h>
#include <stdlib.h>
#include <time.h>


char set[36] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','0','1','2','3','4','5','6','7','8','9'};


int main(int argc,char* argv[])
{
    srand(time(NULL));
    char dir_name[8];
    for(int i = 0 ; i < 8 ; i++)
    {
        int luck = rand()%36;
        if (luck < 26)
        {
            int low_case = rand()%2;
            if (low_case == 1 )
                dir_name[i] = set[luck];
            else
                dir_name[i] = set[luck] - 32 ;
            
        }
        else
        {
            dir_name[i] = set[luck];
        }
        
    }
    printf("%s",dir_name);
    return 0;
}