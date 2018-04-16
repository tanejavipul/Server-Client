#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "socket.h"
#include "jobprotocol.h"


int main(int argc, char **argv) {
    // This line causes stdout and stderr not to be buffered.
    // Don't change this! Necessary for autotesting.
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    if (argc != 2) {
        fprintf(stderr, "Usage: jobclient hostname\n");
        exit(1);
    }

    char buf[BUFSIZE + 1];
    char input[BUFSIZE + 1];
    int soc = connect_to_server(PORT, argv[1]);


    /* TODO: Accept commands from the user, verify correctness 
     * of commands, send to server. Monitor for input from the 
     * server and echo to STDOUT.
     */

    int max_fd = soc;
    fd_set all_fds, listen_fds;
    FD_ZERO(&all_fds);
    FD_SET(soc, &all_fds);
    FD_SET(STDIN_FILENO, &all_fds);
    while (1) {
        listen_fds = all_fds;
        int nready = select(max_fd + 1, &listen_fds, NULL, NULL, NULL);
        if (nready == -1) {
            perror("server: select");
            exit(1);
        }


        if(FD_ISSET(STDIN_FILENO, &listen_fds))
        {
            int num_read = read(STDIN_FILENO, buf, BUFSIZE);
            if (num_read == 0) {
                //printf("read exit\n");
                close(soc);
                return 0;
            }
            strcpy(input, buf);
            input[num_read-1] = '\0';        // Just because I'm paranoid
            int x = input_checker(input);
            //printf("output index: %d\n",x);
            if(x == 0) {
                printf("Command not found\n");
            }
            else if(x == 5)
            {
                printf("[CLIENT] Connection closed\n");
                close(soc);
                return 0;
            }
            else
            {
                int num_written = write(soc, buf, num_read);
                if (num_written != num_read) {
                    close(soc);
                    exit(1);
                }
            }
        }

        if(FD_ISSET(soc, &listen_fds))
        {
            int num_read = read(soc, buf, BUFSIZE);
            if (num_read == 0) {
                close(soc);
                exit(1);
                break;
            }
            buf[num_read] = '\0';
            printf("%s", buf);
        }
    }
    close(soc);
    return 0;
}
