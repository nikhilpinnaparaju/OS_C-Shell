/* 
    Welcome to the Custom Shell (CSH) by Nikhil Pinnaparaju and Sailendra DS.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

char retrUser[1024];
char host[1024];
char logged_in[1024];

int main()
{
    char *found, *start;

    start = retrUser;

    getcwd(retrUser,1024);
    gethostname(host,1024);
    
    char pwd[1024] = "~";

    // printf("%s\n",start);
    int count = 0;

    while ( (found = strsep(&start,"/")) != NULL)
    {
        // printf("%d:%s\n",count,found);

        if (count == 2)
        {
            strcpy(logged_in,found);
        }

        count++;
    }

    while (1)
    {
        printf("%s@%s:%s>",logged_in,host,pwd);
        
        char input[10240],*tokens[1000] = {NULL};
        
        scanf("%s",input);

        // parseInput(input,tokens," ");

    }

    return 0;
}
