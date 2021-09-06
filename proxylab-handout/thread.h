#include <pthread.h>
#include <semaphore.h>

typedef struct 
{
    int *buf;       // buffer array
    int n;          // Max number of slots
    int front;      // buf[(front+1) % n] is first item
    int rear;       // buf[rear%n] is last item
    sem_t mutex;    // protect accesses to buf
    sem_t slots;    // counts available slots
    sem_t items;    // counts available items
} sbuf_t;

#define NTHREADS 4
#define SBUFSIZE 16

typedef struct
{
    pthread_t tid;
    int client_fd;
    int server_fd;
} proxy_fd;

extern proxy_fd thread_fds[NTHREADS];

void sbuf_init(sbuf_t* , int);
void sbuf_deinit(sbuf_t*);
void sbuf_insert(sbuf_t*, int);
int  sbuf_remove(sbuf_t*);
void set_fd(int, int);
void free_fd();
void unix_thread_error(char *msg);
