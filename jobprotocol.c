#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "socket.h"
#include "jobprotocol.h"
// TODO: Use this file for helper functions (especially those you want available
// to both executables.

//each have there own return, if something fails then return 0;
int input_checker(char input[BUFSIZE])
{
    //checking for 
    const char s[2] = " ";
    char *token;
    token = strtok(input, s);
    //printf("token:%s\n",token);
    if(strcmp(input,"jobs") == 0)
    {
        return 1;
    }
    else if(token != NULL && strcmp(token,"run") == 0)
    {
        token = strtok(NULL, s);
        if(token != NULL && strlen(token) > 0)
        {
            return 2;
        }
    }
    else if(token != NULL && strcmp(token,"kill") == 0)
    {
        token = strtok(NULL, s);
        int ret = strtol(token, NULL, 10);
        if(token != NULL && digit_len(ret) == strlen(token))
        {
            return 3;
        }
    }
    else if(token != NULL && strcmp(token,"watch") == 0)
    {
        token = strtok(NULL, s);
        int ret = strtol(token, NULL, 10);
        if(token != NULL && digit_len(ret) == strlen(token))
        {
            return 4;
        }
    }
    else if(strcmp(input,"exit") == 0)
    {
        return 5;
    }
    return 0;
}


int digit_len(int x)
{
    int count = 0;
    while(x!=0)
    {
        x/=10;
        count++;
    }
    return count;
}
