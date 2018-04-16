#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>

#include "socket.h"
#include "jobprotocol.h"


#define QUEUE_LENGTH 5

#ifndef JOBS_DIR
    #define JOBS_DIR "jobs/"
#endif

fd_set all_fds, listen_fds;
int max_fd;
struct Clients *client;
int total_clients = 0;
struct jobs *jobs[MAX_JOBS];
char exits[BUFSIZE] = {"[SERVER] Shutting down\n"};
const char s[2] = " ";
//int total_jobs = 0;
//int array[10];


//next job that is open at index
int job_total()
{
    int total = 0;
    int index = 0;
    while(index<MAX_JOBS)
    {
        if(jobs[index]->pid > 1)
        {
            total++;
        }
        index++;
    }
    return total;

}


int job_open()
{
    int index = 0;
    while(index<MAX_JOBS)
    {
        if(jobs[index]->pid == -1)
        {
            return index;
        }
        index++;
    }
    return -1;
}

//return -1 if job->pid does not exit
int find_job(int pid)
{
    int index = 0;
    while(index<MAX_JOBS)
    {
        if(jobs[index]->pid == pid)
        {
            return index;
        }
        index++;
    }
    return -1;
}

int setup_new_client(int fd, struct Clients *client) 
{
    int client_fd = accept_connection(fd);
    if (client_fd < 0) {
        return -1;
    }
    struct Clients (*add) = malloc(sizeof(struct Clients));
    if(total_clients == 0)
    {
        total_clients++;
        client->clientfd = client_fd;
        client->next_client = NULL;
    }
    else 
    {
        total_clients++;
        struct Clients (*current) = client;
        add->clientfd = client_fd;
        add->next_client = NULL;
        while(current->next_client != NULL)
        {
            current = current->next_client;
        }
        current->next_client = add;
    }
    return client_fd;
}

int read_client(int soc_index)
{
    char client_input[BUFSIZE+1];
    char input_copy[BUFSIZE+1];
    int len = read(soc_index, client_input, BUFSIZE);
        if(len == 0)
    {
        return 0;
    } 
    client_input[len] = '\0'; 
    printf("[CLIENT %d] %s",soc_index,client_input);
    client_input[len-1] = '\0';
    strcpy(input_copy,client_input);
    int index = input_checker(input_copy);
    strcpy(input_copy,client_input);
    if(index == 1) //JOBS
    {
        if(job_total() == 0)
        {
            printf("[SERVER] No currently running jobs\n");
            write(soc_index,"[SERVER] No currently running jobs\n",BUFSIZE);
        }
        else 
        {
            printf("[SERVER]");
            write(soc_index,"[SERVER]",BUFSIZE);
            for(int x = 0;x<MAX_JOBS;x++)
            {
                if(jobs[x]->pid > -1)
                {
                    char job_pid[BUFSIZE];
                    sprintf(job_pid, " %d", jobs[x]->pid);
                    printf("%s",job_pid);
                    write(soc_index,job_pid,BUFSIZE);
                }
            }
            printf("\n");
            write(soc_index,"\n",BUFSIZE);
        }
        return 99;
    }
    else if(index == 2) //RUN
    {
        int open_job_index = job_open();
        if(open_job_index == -1)
        {
            write(soc_index,"[SERVER] MAXJOBS exceeded\n",BUFSIZE);
            printf("[SERVER] MAXJOBS exceeded\n");
            return 1;
        }
        int pfd[2];
        const char s[2] = " ";
        char *token;
        char *token2;
        input_copy[len-1] = '\0';
        token = strtok(input_copy, s);
        token = strtok(NULL, s);
        char exe_file[BUFSIZE];
        sprintf(exe_file, "./%s%s", JOBS_DIR,token); // ./jobs/___
        token2 = strtok(NULL, s);
        int argc = -1;
        char **argv  = NULL;
        if(token2 == NULL)
        {
            argc = 0;
        }
        else 
        {
            argc = 1;
            argv = malloc(sizeof(char *)*BUFSIZE);
            for(int w = 0; w<BUFSIZE; w++)
            {
                argv[w] = malloc(sizeof(char)*BUFSIZE);
            }
            strcpy(input_copy,client_input);
            input_copy[len-1] = '\0';
            token = strtok(input_copy,s);
            token = strtok(NULL,s);
            int x = 0;
            while(token !=NULL)
            {
                strncpy(argv[x],token,BUFSIZE-1);
                x++;
                token = strtok(NULL,s);
            }
        }

        pipe(pfd);
        int ret = fork();
        if(ret == 0)
        {
            dup2(pfd[1],STDOUT_FILENO);
            if(argc == 0)
            {
                char exe_file[BUFSIZE];
                close(pfd[0]);
                sprintf(exe_file, "./%s%s", JOBS_DIR,token);
                execl(exe_file,token,NULL);
                close(pfd[1]);
                exit(0);
            }
            else 
            {
                execv(exe_file,argv);
            }
        }
        // FREE TEST
        if(argc != 0)
        {
            for(int o = 0;o<BUFSIZE;o++)
            {
                free(argv[o]);
            }
        free(argv);
        }




        close(pfd[1]);
        FD_SET(pfd[0], &all_fds);
        jobs[open_job_index]->pid = ret;
        jobs[open_job_index]->pfd = pfd[0];
        struct watch_client (*watch_add) = malloc(sizeof(struct watch_client));
        watch_add->clientfd = soc_index;
        watch_add->client_watch = NULL;
        jobs[open_job_index]->client_watch = watch_add;
        printf("[SERVER] Job %d created\n", ret);
        char job_create[BUFSIZE];
        sprintf(job_create, "[SERVER] Job %d created\n", ret);
        write(soc_index,job_create,BUFSIZE);
        if (pfd[0] > max_fd) 
        {
            max_fd = pfd[0];
        }
        return 99;
    }
    else if(index == 3) //KILL
    {
        char *token;
        input_copy[len-1] = '\0';
        token = strtok(input_copy, s);
        token = strtok(NULL, s);
        int pid = strtol(token, NULL, 10);
        int index_pid = find_job(pid);
        //printf("index: %d\n",index_pid);
        if(index_pid == -1)
        {
            printf("[SERVER] Job %d not found\n",pid);
            char job_not_found[BUFSIZE];
            sprintf(job_not_found, "[SERVER] Job %d not found\n", pid);
            write(soc_index,job_not_found,BUFSIZE);
        }
        else 
        {
            printf("[Job %d] Exited due to signal\n",pid);
            char job_kill[BUFSIZE];
            sprintf(job_kill, "[Job %d] Exited due to signal\n", pid);
            write(soc_index,job_kill,BUFSIZE);
            struct watch_client (*find) = jobs[index_pid]->client_watch;
            while(find != NULL)
            {
                write(find->clientfd,job_kill,BUFSIZE);
                struct watch_client (*backup) = find;
                find = find->client_watch;
                free(backup);
            }
            jobs[index_pid]->client_watch = NULL;
            kill(pid,SIGKILL);
            close(jobs[index_pid]->pfd);
            FD_CLR(jobs[index_pid]->pfd, &all_fds);
            jobs[index_pid]->pid = -1;
            //free watch
        }
        return 99;
        
    }
    else if(index == 4)
    {

        char *token;
        input_copy[len-1] = '\0';
        token = strtok(input_copy, s);
        token = strtok(NULL, s);
        int pid = strtol(token, NULL, 10);
        int index_job = find_job(pid);
        if(index_job == -1)
        {
            printf("[SERVER] Job %d not found\n",pid);
            char watch_exists[BUFSIZE];
            sprintf(watch_exists, "[SERVER] Job %d not found\n", pid);
            write(soc_index,watch_exists,BUFSIZE);
        }
        else
        {
            struct watch_client (*find) = jobs[index_job]->client_watch;
            if(find != NULL && find->clientfd == soc_index)
            {
                char watch_no[BUFSIZE];
                sprintf(watch_no, "[SERVER] No longer watching job %d\n", pid);
                write(soc_index,watch_no,BUFSIZE);
                find->clientfd = -1;
                return 99;
            }
            while(find->client_watch != NULL)
            {
                if(find->clientfd == soc_index)
                {
                    char watch_no[BUFSIZE];
                    sprintf(watch_no, "[SERVER] No longer watching job %d\n", pid);
                    write(soc_index,watch_no,BUFSIZE);
                    find->clientfd = -1;
                    return 99;
                }
                find = find->client_watch;
            }
            if(find != NULL && find->clientfd == soc_index)
            {
                char watch_no[BUFSIZE];
                sprintf(watch_no, "[SERVER] No longer watching job %d\n", pid);
                write(soc_index,watch_no,BUFSIZE);
                find->clientfd = -1;
                return 99;
            }
            char watch_yes[BUFSIZE];
            sprintf(watch_yes, "[SERVER] Watching job %d\n", pid);
            write(soc_index,watch_yes,BUFSIZE);
            struct watch_client (*watch_add) = malloc(sizeof(struct watch_client));
            watch_add->clientfd = soc_index;
            watch_add->client_watch = NULL;
            find->client_watch = watch_add;
        }
        return 99;
    }
    printf("[SERVER] Invalid command: %s\n",client_input);
    return -1;
}


void handler(int code) {
    printf("%s",exits);

    while(client != NULL)
    {
        //printf("fd: %d\n",client->clientfd);
        write(client->clientfd,exits,BUFSIZE);
        if(client->clientfd != -1)
        {
            close(client->clientfd);
        }
        struct Clients (*temp) = client;
        client = client->next_client;
        free(temp);
    }

    for(int x = 0; x<MAX_JOBS;x++)
    {
        struct watch_client (*find) = jobs[x]->client_watch;
        if(jobs[x]->pid > -1)
        {
            kill(jobs[x]->pid,SIGKILL);
            
        }
        if(jobs[x]->pfd > -1)
        {
            close(jobs[x]->pfd);
        }
        while(find != NULL)
        {
            struct watch_client (*backup) = find;
            find = find->client_watch;
            free(backup);
        }
        free(jobs[x]);
    }
    exit(0);
}


int main(void) {
    // This line causes stdout and stderr not to be buffered.
    // Don't change this! Necessary for autotesting.
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    struct sigaction exithand;
    exithand.sa_handler = handler;
    exithand.sa_flags = 0;
    sigemptyset(&exithand.sa_mask);
    sigaction(SIGINT, &exithand, NULL);


    struct sockaddr_in *self = init_server_addr(PORT);
    int sock_fd = setup_server_socket(self, QUEUE_LENGTH);
    

    for(int k = 0; k<MAX_JOBS; k++)
    {
        jobs[k] = malloc(sizeof(struct jobs));
        jobs[k]->pid = -1;
        jobs[k]->pfd = -1;
        jobs[k]->client_watch = NULL;
    }

    /* TODO: Initialize job and client tracking structures, start accepting
     * connections. Listen for messages from both clients and jobs. Execute
     * client commands if properly formatted. Forward messages from jobs
     * to appropriate clients. Tear down cleanly.
     */

    /* Here is a snippet of code to create the name of an executable to execute:
     * char exe_file[BUFSIZE];
     * sprintf(exe_file, "%s/%s", JOBS_DIR, <job_name>);
     */
    client = malloc(sizeof(struct Clients));
    max_fd = sock_fd;
    FD_ZERO(&all_fds);
    FD_SET(sock_fd, &all_fds);
    while (1) 
    {
        struct Clients (*reset) = client;
        listen_fds = all_fds;
        int nready = select(max_fd + 1, &listen_fds, NULL, NULL, NULL);
        if (nready == -1) {
            perror("server: select");
            exit(1);
        }

        // Is it the original socket? Create a new connection ...
        if (FD_ISSET(sock_fd, &listen_fds)) {
            int client_fd = setup_new_client(sock_fd, client);
            if (client_fd < 0) {
                continue;
            }
            if (client_fd > max_fd) {
                max_fd = client_fd;
            }
            FD_SET(client_fd, &all_fds);
            printf("Accepted connection\n");
        }
        for (int index = 0; index < total_clients; index++) {
            if (reset->clientfd != -1 && FD_ISSET(reset->clientfd, &listen_fds)) {
                int client_closed = read_client(reset->clientfd);
                if (client_closed == 0) {
                    FD_CLR(reset->clientfd, &all_fds);
                    close(reset->clientfd);
                    printf("[CLIENT %d] Connection closed\n", reset->clientfd);
                    reset->clientfd = -1;
                }
            }
            reset = reset->next_client;
        }

        for (int index_jobs = 0; index_jobs < MAX_JOBS; index_jobs++) {
            if (jobs[index_jobs]->pid != -1 && FD_ISSET(jobs[index_jobs]->pfd, &listen_fds)) {
                char test[BUFSIZE+1];
                int read_len = read(jobs[index_jobs]->pfd,test,BUFSIZE);
                //printf("Len of jobs: %d\n",read_len);
                test[read_len] = '\0'; // just to be sure
                if(read_len == 0)
                {
                    char output_status[BUFSIZE];
                    sprintf(output_status, "[JOB %d] Exited with status %d.\n", jobs[index_jobs]->pid, WIFEXITED(jobs[index_jobs]->pid));
                    printf("[JOB %d] Exited with status %d.\n",jobs[index_jobs]->pid, WIFEXITED(jobs[index_jobs]->pid));
                    struct watch_client (*find) = jobs[index_jobs]->client_watch;
                    while(find != NULL)
                    {
                        write(find->clientfd,output_status,BUFSIZE);
                        find = find->client_watch;
                    }
                    jobs[index_jobs]->pid = -1;
                    close(jobs[index_jobs]->pfd);
                    FD_CLR(jobs[index_jobs]->pfd, &all_fds);
                }
                else 
                {
                    //printf("[JOB %d]: %s",jobs[index_jobs]->pfd,test) strtok
                    char *token;
                    const char n[2] = "\n";
                    token = strtok(test, n);
                    char output_job[BUFSIZE];
                   /* if(token[strlen(token)-1]=='\n')
                    {
                        token[strlen(token)-1] = '\0';
                    }*/
                    while(token != NULL)
                    {
                        sprintf(output_job,"[JOB %d] %s\n",jobs[index_jobs]->pid,token);
                        struct watch_client (*find) = jobs[index_jobs]->client_watch;
                        printf("%s",output_job);
                        while(find != NULL)
                        {
                            write(find->clientfd,output_job,BUFSIZE);
                            find = find->client_watch;
                        }
                        token = strtok(NULL,n);
                    }

                }
            }
        }
    }
    free(self);
    close(sock_fd);
    return 0;
}

