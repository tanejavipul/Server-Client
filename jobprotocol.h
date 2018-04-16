#ifndef __JOB_PROTOCOL_H__
#define __JOB_PROTOCOL_H__

#ifndef PORT
  #define PORT 50000
#endif

#ifndef MAX_JOBS
    #define MAX_JOBS 32
#endif

// No paths or lines may be larger than the BUFSIZE below
#define BUFSIZE 256

// TODO: Add any extern variable declarations or struct declarations needed.

#endif

struct Clients
{
  int clientfd;
  struct Clients *next_client;
};

struct jobs
{
  int pid;
  int pfd;
  //int killpfd;
  struct watch_client *client_watch;
  //struct jobs *next_job;

};

struct watch_client
{
  int clientfd;
  struct watch_client *client_watch;
};


int input_checker(char input[BUFSIZE]);
int digit_len(int x);
int job_open();