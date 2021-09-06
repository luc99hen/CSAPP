#include <stdio.h>
#include "csapp.h"
#include "thread.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

void    sigpipe_handler(int sig);
void    clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
int     parseURI(const char*, char*, char*, char*, char*);
void    split_header(char* buf, char* name, char* content);
int     is_valid_header(char* buf);
void*   thread(void* vargp);
void    doit(int);

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3";
sbuf_t sbuf;

int main(int argc, char** argv)
{
    int listenfd, connfd;
    char clientname[MAXLINE], clientport[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    // check cmd line args
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    // handler broken pipe properly
    Signal(SIGPIPE, sigpipe_handler);
    listenfd = Open_listenfd(argv[1]);

    // init thread pool
    sbuf_init(&sbuf, SBUFSIZE);
    for (int i = 0; i < NTHREADS; i++) {
        Pthread_create(&tid, NULL, thread, NULL);
        thread_fds[i].tid = tid;
    }
        
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);

        // log
        Getnameinfo((SA*)&clientaddr, clientlen, clientname, MAXLINE, clientport, MAXLINE, 0);
        printf("Proxy accepted conn from (%s %s)", clientname, clientport); 

        sbuf_insert(&sbuf, connfd);
    }
}

void* thread(void* vargp)
{
    pthread_t tid = Pthread_self();
    Pthread_detach(tid);
    while (1) {
        printf("thread id: %ld\n", tid);
        int client_fd = sbuf_remove(&sbuf);
        doit(client_fd);
        free_fd();
    }
}

void sigpipe_handler(int sig) 
{
    fprintf(stderr, "%s", "\nWrite to closed conn socket.\n");
}

/*
 * doit - handle one HTTP request/response transaction
 */
void doit(int client_fd)
{
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];

    rio_t rio_c, rio_s;
    Rio_readinitb(&rio_c, client_fd);
    set_fd(client_fd, 1);

    // check request format
    if (Rio_readlineb(&rio_c, buf, MAXLINE) == 0) {
        clienterror(client_fd, method, "400","Bad Request", "Request format error: first line is empty");
        return;
    }
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);
    if (strcasecmp(method, "GET")) {                 
        clienterror(client_fd, method, "501", "Not Implemented", "Tiny does not implement this method");
        return;
    }

    // parse request headers & open conn to host
    char host[MAXLINE], path[MAXLINE], hostname[MAXLINE], port[MAXLINE];
    if (parseURI(uri, host, path, hostname, port) == 0) {
        clienterror(client_fd, method, "400", "Bad Request", "Request format error: first line is empty");
        return;
    }
    int server_fd = Open_clientfd(hostname, port);  
    Rio_readinitb(&rio_s, server_fd);
    set_fd(server_fd, 0);
    
    // write new header to socket
    sprintf(buf, "%s %s %s\r\n", method, path, "HTTP/1.0");
    printf("######## New Request ######## \n%s", buf);
    Rio_writen(server_fd, buf, strlen(buf));
    while (1) {
        Rio_readlineb(&rio_c, buf, MAXLINE);      
        if (strcmp(buf, "\r\n") == 0)
            break;  
        if (!is_valid_header(buf))
            continue;
        Rio_writen(server_fd, buf, strlen(buf));
        printf("%s", buf);
    }
    sprintf(buf, "Host: %s\r\nUser-Agent: %s\r\nProxy-Connection: close\r\nConnection: close\r\n\r\n", 
        host, user_agent_hdr);
    printf("%s", buf);
    Rio_writen(server_fd, buf, strlen(buf));

    // r&w server response
    char headername[MAXLINE], headercontent[MAXLINE];
    int contentlength = -1;  //  assume contentlength < MAX_INT
    Rio_readlineb(&rio_s, buf, MAXLINE); // read first line
    Rio_writen(client_fd, buf, strlen(buf));
    printf("%s", buf);
    // r&w res header
    while (1) {
        Rio_readlineb(&rio_s, buf, MAXLINE);        
        printf("%s", buf);
        Rio_writen(client_fd, buf, strlen(buf));
        if (strcmp(buf, "\r\n") == 0)
            break;
        split_header(buf, headername, headercontent);
        if (strcmp(headername, "Content-length"))
            sscanf(headercontent, "%d\r\n", &contentlength);
    }
    // r&w res content
    if (contentlength >= 0) {
        Rio_readn(server_fd, buf, contentlength);
        printf("%s", buf);
        Rio_writen(client_fd, buf, contentlength);
    } else {
        int n;
        while((n = Rio_readlineb(&rio_s, buf, MAXLINE)) > 0) {
            printf("%s", buf);
            Rio_writen(client_fd, buf, n);
        }
    }

}

void split_header(char* buf, char* name, char* content) {
    char* headerSplit = strstr(buf, ":");
    if (!headerSplit)
        return;
    *headerSplit = 0;
    strcpy(name, buf);
    strcpy(content,headerSplit+1);
}

int is_valid_header(char* buf) {
    char* headerSplit = strstr(buf, ":");
    *headerSplit = 0;
    const char * invalidHeaderName[4] = {"Host", "Connection", "User-Agent", "Proxy-Connection"};

    for (int i=0; i<4; i++) {
        if (strcmp(invalidHeaderName[i], buf) == 0)
            return 0;
    }

    *headerSplit = ':';
    return 1;
}


int parseURI(const char* uri, char* host, char* path, char* hostname, char* port) {
    // split protocol :// host 
    char* host_start = strstr(uri, "://");
    if (!host_start)
        return 0;
    
    // split host/path
    host_start += 3;
    char* path_start = strstr(host_start, "/");
    if (!path_start)
        strcpy(path, "/");
    else
        strcpy(path, path_start);
    *path_start = 0;
    strcpy(host, host_start);

    // split hostname:port
    char* port_start = strstr(host_start, ":");
    if (!port_start)
        strcpy(port, "80");
    else
        strcpy(port, port_start+1);
    *port_start = 0;
    strcpy(hostname, host_start);
    return 1;
}

/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE];

    /* Print the HTTP response headers */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Tiny Error</title>");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<body bgcolor=""ffffff"">\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Tiny Web server</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}
/* $end clienterror */