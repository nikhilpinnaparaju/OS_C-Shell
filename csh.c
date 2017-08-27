/* 
    Welcome to the Custom Shell (CSH) by Nikhil Pinnaparaju and Sailendra DS.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char buffer[1000];
char host[1000];

int main()
{
    getcwd(buffer,1000);
    gethostname(host,1000);

    printf("%s %s\n",buffer,host);

    return 0;
}