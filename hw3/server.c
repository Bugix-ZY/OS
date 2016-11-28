#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>
#include <ctype.h>

#include "status.h"

#define PORT "12345" // port
#define BACKLOG 10 // #pending connections queue
#define STACK_SIZE (256*256)
#define DB_SIZE 100
#define true 1
#define false 0

char *domain_db[DB_SIZE];
char *addr_db[DB_SIZE];
int count = 0; //count the number of entries
int readcount = 0;
sem_t mutex;
sem_t wrt;

void sigchld_handler(int s)
{
  while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr，IPv4 / IPv6：
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

static inline int send_response(int sfd, size_t size, const char *response)
{
  if (write(sfd, &size, sizeof(size_t)) == -1)
    return -1;
  if (write(sfd, response, size) == -1)
    return -1;

  return 0;
}

static inline int receive_request(int sfd, size_t *size, char *request)
{
  ssize_t ret;
  ret = read(sfd, size, sizeof(size_t));
  if (ret <= 0)
    return -1;

  ret = read(sfd, request, *size);
  if (ret <= 0)
    return -1;

  return 0;
}

struct thread_para {
  int t_sockfd;
  int t_new_fd;
};

int domainLegal(char *domain){
  int status, i;
  char *delim = ".";
  char *p;
  char *ad[10];
  ad[0] = strtok(domain, delim);
  //printf("0 %s\n",ad[0]);
  for(i = 1; i < 10 && (ad[i] = strtok(NULL, delim)); i++){
    //printf("%d %s\n",i, ad[i]);
  }
  if(i <= 1){
    return false;
  }

  return true;
}

int addrLegal(char *addr, int address[]){
  int status, i;
  char *delim = ".";
  char *p;
  char *ad[4];


  for(i = 0; addr[i] != '\0'; i++){
    if (!isdigit(addr[i]) && addr[i] != '.'){
      return false;
    }
  }

  ad[0] = strtok(addr, delim);
  //printf("0 %s\n",ad[0]);
  for(i = 1; i < 4 && (ad[i] = strtok(NULL, delim)); i++){
    //printf("%d %s\n",i, ad[i]);
  }
  if(i != 4){
    return false;
  }

  for(i = 0; i < 4; i++){
    char *tmp;
    int c = atoi(ad[i]);
    address[i] = c;
    if (c >=256 || c < 0){
      return false;
    }
  }
  //printf("store = %s\n", store);
  return true;
}

int work_thread(void *arg){

  struct thread_para *t = arg;

  int sockfd = t->t_sockfd;
  int new_fd = t->t_new_fd;
  close(sockfd); // child does not need listener

  /*get method, domain and addr*/
  size_t size_request, size_response;
  char request[128];
  char response[128];
  receive_request(new_fd, &size_request, request);
  request[size_request] = '\0';
  printf("request = %s\n", request);

  char *delim = " ";
  char *method;
  char *domain;
  char *addr;
  char *p;
  const char *set = "SET";
  const char *get = "GET";
  const char *info = "INFO";

  method = strtok(request, delim);
  for(int i = 1; i < 3 && (p = strtok(NULL, delim)); i++){
    if (i == 1){
      domain = p;
    } else if (i == 2){
      addr = p;
    }
  }
  printf("method: %s\n", method);
  printf("domain: %s\n", domain);
  printf("address: %s\n", addr);



/*1. whether methods are allowed*/
  if(strcmp(method,set)!= 0 && strcmp(method,get) != 0 && strcmp(method,info) != 0 ){
    printf("%d \"%s\"\n", status_code[METHOD_NOT_ALLOWED], status_str[METHOD_NOT_ALLOWED]);
    size_response = snprintf(response, sizeof(response),"%d \"%s\"", status_code[METHOD_NOT_ALLOWED], status_str[METHOD_NOT_ALLOWED]);
    send_response(new_fd,size_response, response);
    //printf("not allowed response = %s\n", response);
  }
        
  if(strcmp(method,set) == 0){
    char sdomain[100];
    char saddr[20];
    memset(sdomain, '\0', 100);
    strcpy(sdomain, domain);
    char saddress[20] = "";
    int address[4];

    int c = 0;
    int i = 0;
    while(sdomain[c] != '\0'){
      sdomain[c] = tolower(sdomain[c]);
      c += 1;
    }
    
    //printf("domain --> %s\n", sdomain);

    if(domain != NULL && addr != NULL && domainLegal(domain) && addrLegal(addr,address)){
      for(int k = 0; k < 4; k++){
        memset(saddr, '\0', 20);
        if(k != 3)
          sprintf(saddr,"%d.",address[k]);
        else
          sprintf(saddr,"%d",address[k]);
        strcat(saddress, saddr);
      }

      for( i = 0; i < count; i++){
          if(strncmp(domain_db[i], sdomain, sizeof(sdomain)) == 0)
            break;
      }
      if(i >= count){
        sem_wait(&wrt);
        domain_db[count] =  sdomain;
        addr_db[count] = saddress;
        count += 1;
        sem_post(&wrt);
      } 
      else{
        sem_wait(&wrt);
        addr_db[i] = saddress;
        sem_post(&wrt);
      }

      //send response to the client
      size_response = snprintf(response, sizeof(response),"%d \"%s\" ", status_code[OK], status_str[OK]);
      send_response(new_fd,size_response, response);
      }
    
    
    else {
      printf("%d \"%s\"\n", status_code[BAD_REQUEST], status_str[BAD_REQUEST]);
      size_response = snprintf(response, sizeof(response),"%d \"%s\" ", status_code[BAD_REQUEST], status_str[BAD_REQUEST]);
      send_response(new_fd,size_response, response);        
    } 
  }


  else if (strcmp(method, get) == 0) {
      char sdomain[100];
      memset(sdomain, '\0', 100);
      strcpy(sdomain, domain);

      int c = 0;
      while(sdomain[c] != '\0'){
        sdomain[c] = tolower(sdomain[c]);
        c += 1;
      }
      //printf("domain --> %s\n", sdomain);

      if(addr != NULL || domain == NULL){
        printf("%d \"%s\"\n", status_code[BAD_REQUEST], status_str[BAD_REQUEST]);
        size_response = snprintf(response, sizeof(response),"%d \"%s\" ", status_code[BAD_REQUEST], status_str[BAD_REQUEST]);
        send_response(new_fd,size_response, response);
      }
      else {
        sem_wait(&mutex);
        readcount += 1;
        if(readcount == 1)
          sem_wait(&wrt);
        sem_post(&mutex);
        int i;
        for( i = 0; i < count; i++){
          if(strncmp(domain_db[i], sdomain, sizeof(sdomain)) == 0)
            break;
        }
        if(i < count){ //found
          size_response = snprintf(response, sizeof(response),"%d \"%s\" %s", status_code[OK], status_str[OK],addr_db[i]);
          send_response(new_fd,size_response, response);
        } 

        else { //not found
          size_response = snprintf(response, sizeof(response),"%d \"%s\" ", status_code[NOT_FOUND], status_str[NOT_FOUND]);
          send_response(new_fd,size_response, response);
        }

        sem_wait(&mutex);
        readcount -= 1;
        if(readcount == 0)
          sem_post(&wrt);
        sem_post(&mutex);

      }

  } 
  else if (strcmp(method, info) == 0){
    if(domain != NULL){
      printf("%d \"%s\"\n", status_code[BAD_REQUEST], status_str[BAD_REQUEST]);
      size_response = snprintf(response, sizeof(response),"%d \"%s\" ", status_code[BAD_REQUEST], status_str[BAD_REQUEST]);
      send_response(new_fd,size_response, response);
    }
    else {
      sem_wait(&mutex);
      readcount += 1;
      if(readcount == 1)
        sem_wait(&wrt);
      sem_post(&mutex);
      size_response = snprintf(response, sizeof(response),"%d \"%s\" %d", status_code[OK], status_str[OK], count);
      send_response(new_fd,size_response, response);

      sem_wait(&mutex);
      readcount -= 1;
      if(readcount == 0)
        sem_post(&wrt);
      sem_post(&mutex);
    }
  }      
  close(new_fd);
  sleep(1);
  exit(1);
      
}


int main(void)
{
  int sockfd, new_fd; //  sock_fd  listen
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr;
  socklen_t sin_size;
  struct sigaction sa;
  int yes=1;
  char s[INET6_ADDRSTRLEN];
  int rv;

  //open semaphore
  // mutex = sem_open("mutex", O_CREAT, 0777, 1);
  // wrt = sem_open("wrt", O_CREAT, 0777, 1);
  sem_init(&mutex,1,1);
  sem_init(&wrt,1,1);


  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; 

  if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  // bind first result
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
      p->ai_protocol)) == -1) {
      perror("server: socket");
      continue;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
        sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("server: bind");
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "server: failed to bind\n");
    return 2;
  }

  freeaddrinfo(servinfo); 

  if (listen(sockfd, BACKLOG) == -1) {
    perror("listen");
    exit(1);
  }

  sa.sa_handler = sigchld_handler; // deal with dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }

  printf("server: waiting for connections...\n");

  while(1) {  
  
    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

    //printf("listen pid = %d\n", (int)getpid());
    if (new_fd == -1) {
      perror("accept");
      continue;
    }


    inet_ntop(their_addr.ss_family,
    get_in_addr((struct sockaddr *)&their_addr),
      s, sizeof s);
    printf("server: got connection from %s\n", s);

    struct thread_para *tinfo;
    char *stack;
    char *stackTop;
    stack = malloc(STACK_SIZE);
    stackTop = stack + STACK_SIZE;
    tinfo = calloc(1,sizeof(int)*2);
    if( tinfo == NULL)
      printf("calloc error\n");

    tinfo->t_sockfd = sockfd;
    tinfo->t_new_fd = new_fd;


    //printf("before clone count = %d\n", count);
    clone(work_thread, stackTop, CLONE_VM  | CLONE_SYSVSEM , tinfo);
    //pthread_t *workthread;
    //workthread = (pthread_t*) malloc(sizeof(*workthread));
    //start the thread
    //pthread_create(workthread,NULL,(void*)work_thread, tinfo);

    close(new_fd); 
    sleep(1);
     
    
  }
  close(new_fd);
  return 0;
}
