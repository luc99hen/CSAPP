#include "csapp.h"
#include "thread.h"
proxy_fd thread_fds[NTHREADS];

void sbuf_init(sbuf_t* sp, int n)
{
    sp->buf = Calloc(n, sizeof(int));
    sp->n = n;
    sp->front = sp->rear = 0;
    Sem_init(&sp->mutex, 0, 1);
    Sem_init(&sp->slots, 0, n);
    Sem_init(&sp->items, 0, 0);
}

void sbuf_deinit(sbuf_t* sp)
{
    Free(sp->buf);
}

void sbuf_insert(sbuf_t* sp, int item)
{
    P(&sp->slots);
    P(&sp->mutex);
    sp->buf[(++(sp->rear)) % (sp->n)] = item;
    V(&sp->mutex);
    V(&sp->items);
}

int sbuf_remove(sbuf_t* sp)
{
    int item;
    P(&sp->items);
    P(&sp->mutex);
    item = sp->buf[(++(sp->front)) % (sp->n)];
    V(&sp->mutex);
    V(&sp->slots);  
    return item;
}

void set_fd(int fd, int is_client)
{
    pthread_t tid = Pthread_self();
    int i;
    for (i=0; i<NTHREADS; i++) {
        if (tid == thread_fds[i].tid) {
            if (is_client)
                thread_fds[i].client_fd = fd;
            else
                thread_fds[i].server_fd = fd;
        }            
    }

    if (i > NTHREADS)
        unix_error("unknown thread id");
}

void free_fd()
{
    pthread_t tid = Pthread_self();
    int i;
    for (i=0; i<NTHREADS; i++) {
        if (tid == thread_fds[i].tid) {
            if (thread_fds[i].client_fd > 0) {
                Close(thread_fds[i].client_fd);
                thread_fds[i].client_fd = 0;
            }
            if (thread_fds[i].server_fd > 0) {
                Close(thread_fds[i].server_fd);
                thread_fds[i].server_fd = 0;
            }
        }            
    }

    if (i > NTHREADS)
        unix_error("unknown thread id");
}


void* thread(void *);
void unix_thread_error(char* msg) // like Unix-style error but only terminate cur thread
{
    pthread_t tid = Pthread_self();
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    free_fd();
    for (int i=0; i < NTHREADS; i++) {
        if (tid == thread_fds[i].tid) {
            Pthread_create(&tid, NULL, thread, NULL);
            thread_fds[i].tid = tid;
            printf("thread %ld created to replace %ld\n", tid, Pthread_self());
        }
    }
    pthread_exit(0);
}