/* 
    Welcome to the Custom Shell (CSH) by Nikhil Pinnaparaju and Sailendra DS.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

char workingDir[1000];
char host[1000];
char logged_in[1000];

int main()
{
    char *found, *start;

    start = workingDir;

    getcwd(workingDir,1000);
    gethostname(host,1000);
    
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

    printf("%s@%s>%s\n",logged_in,host,workingDir);

    return 0;
}
