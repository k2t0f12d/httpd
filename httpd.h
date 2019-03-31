/**
 * httpd.h
 */

#include <stdio.h>       /* For perror()        */
#include <stdlib.h>      /* For exit()          */
#include <unistd.h>      /* For fork()          */
#include <string.h>      /* For memset()        */

#include <fcntl.h>        /* what these      */
#include <dlfcn.h>        /* For dlopen()    */
#ifdef __APPLE__
  #include <libproc.h>    /* for proc_pidpath() */
#else
  #include <sys/sendfile.h> /* For sendfile() */
#endif

#include <sys/types.h>   /* For ???             */
#include <sys/socket.h>  /* For accept()
                                socket()
                                socketaddr_in 
                                socklen_t       */
#include <errno.h>       /* For errno() */
#include <netdb.h>       /* For struct addrinfo */
#include <signal.h>      /* For signal() */

#include "kat_memory.h"  /* For KAT_VMEM() */

/* NOTE: There is no build-in boolean in C */
#define TRUE  1
#define FALSE 0

/* NOTE: Disambiguate static keyword */
#define global static
#define internal static
#define persist static

typedef int8_t   int8;
typedef int16_t  int16;
typedef int32_t  int32;
typedef int64_t  int64;
typedef int32    bool32;
typedef int64    bool64;

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float    real32;
typedef double   real64;

typedef int8     i8;
typedef int16    i16;
typedef int32    i32;
typedef int64    i64;
typedef bool32   b32;
typedef bool64   b64;

typedef uint8    u8;
typedef uint16   u16;
typedef uint32   u32;
typedef uint64   u64;

typedef real32   r32;
typedef real64   r64;


#define pi32     3.14159265359f

#define MAX_PATH 4096

/* FEDEC Denominations */
#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

/* Maximum number of clients */
#define CONNMAX 1000

/* some interesting macro for `route()` */
#define ROUTE_START()       if (0) {
#define ROUTE(METHOD,URI)   } else if (strcmp(URI,request->uri)==0&&strcmp(METHOD,request->method)==0) {
#define ROUTE_GET(URI)      ROUTE("GET", URI) 
#define ROUTE_POST(URI)     ROUTE("POST", URI) 
#define ROUTE_END()         } else printf(\
                                "HTTP/1.1 500 Not Handled\r\n\r\n" \
                                "The server has no handler to the request.\r\n" \
                            );

global void error(char *);

struct client_request
{
        char *method;     /* HTTP method, Eg. GET or POST */
        char *uri;        /* Endpoint, Eg. '/index.html', things before '?' */
        char *qs;         /* "a=1&b=2" theings after '?' */
        char *prot;       /* "HTTP/1.1" */
        char *payload;    /* for POST */
        i32 payload_size;
};

struct header_t
{
        char *name;
        char *value;
};

/* get request header */
internal char *request_header(const char* name, struct header_t *reqhdr)
{
    struct header_t *h = reqhdr;
    while(h->name) {
        if (strcmp(h->name, name) == 0) return h->value;
        h++;
    }
    return NULL;
}

void route(struct client_request *request, struct header_t *reqhdr)
{
    ROUTE_START()

    ROUTE_GET("/")
    {
        printf("HTTP/1.1 200 OK\r\n\r\n");
        printf("Hello! You are using %s", request_header("User-Agent", reqhdr));
    }

    ROUTE_POST("/")
    {
        printf("HTTP/1.1 200 OK\r\n\r\n");
        printf("Wow, seems that you POSTed %d bytes. \r\n", request->payload_size);
        printf("%s", request->payload);
    }
  
    ROUTE_END()
}

internal void respond(int n, int clients[], int clientfd,
                      struct client_request *request, struct header_t *reqhdr, char *memory)
{
        i32 rcvd, fd, bytes_read;
        char *ptr;

        rcvd = recv(clients[n], memory, Megabytes(20), 0);
        //fprintf(stderr, "READ FROM SOCKET >>>>>>\n%s\n", memory);
        fflush(stderr);

        if(rcvd<0)        /* receive error */
                fprintf(stderr,("recv() error\n"));
        else if(rcvd==0)  /* receive socket closed */
                fprintf(stderr,"Client disconnected unexpectedly.\n");
        else              /* message received */
        {
                memory[rcvd] = '\0';

                request->method = strtok(memory,  " \t\r\n");
                request->uri    = strtok(NULL, " \t");
                request->prot   = strtok(NULL, " \t\r\n"); 

                fprintf(stderr, "\x1b[32m + [%s] %s\x1b[0m\n", request->method, request->uri);

                /* TODO: Ensure this assignment works as a control sentry */
                if((request->qs = strchr(request->uri, '?')))
                {
                        *request->qs++ = '\0'; //split URI
                } else {
                        request->qs = request->uri - 1; //use an empty string
                }

                struct header_t *h = reqhdr;
                char *t = 0;
                char *t2 = 0;
                /* TODO: Check to see if strtok_r should be used here for thread saftey */
                i32 count = 0;
                while(h < reqhdr+16) {
                        char *k,*v,*t;
                        k = strtok(NULL, "\r\n: \t"); if (!k) break;
                        v = strtok(NULL, "\r\n"); while(*v && *v==' ') v++;
                        h->name  = k;
                        h->value = v;
                        h++;
                        fprintf(stderr, "[H] %s: %s\n", k, v);
                        t = v + 1 + strlen(v);
                        if (t[1] == '\r' && t[2] == '\n') break;
                }
                t=t+2; // now the *t shall be the beginning of user payload
                t2 = request_header("Content-Length", reqhdr); // and the related header if there is  
                request->payload = t;
                request->payload_size = t2 ? atol(t2) : (rcvd-(t-memory));

                // bind clientfd to stdout, making it easier to write
                clientfd = clients[n];
                dup2(clientfd, STDOUT_FILENO);
                close(clientfd);

                // call router
                route(request, reqhdr);

                // tidy up
                fflush(stdout);
                shutdown(STDOUT_FILENO, SHUT_WR);
                close(STDOUT_FILENO);
        }

        //Closing SOCKET
        shutdown(clientfd, SHUT_RDWR);         //All further send and recieve operations are DISABLED...
        close(clientfd);
        clients[n]=-1;
}

internal i32 serve()
{
        struct sockaddr_in clientaddr;
        socklen_t addrlen;

        char port[] = "8080\0";

        /* Init client array */
        i32 clients[CONNMAX];
        for(i32 i = 0; i < CONNMAX; i++)
        {
                clients[i] = -1;
        }

        /* getaddrinfo() for host */
        struct addrinfo hints, *res, *p;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        if(getaddrinfo( NULL, port, &hints, &res) != 0)
        {
                perror("getaddrinfo() error");
                exit(1);
        }

        /* socket and bind */
        i32 listenfd;
        for(p = res; p!=NULL; p=p->ai_next)
        {
                i32 option = 1;
                listenfd = socket(p->ai_family, p->ai_socktype, 0);
                setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
                if(listenfd == -1) continue;
                if(bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) break;
        }
        if(p==NULL)
        {
                perror ("socket() or bind()");
                exit(1);
        }

        freeaddrinfo(res);

        /* listen for incoming connections */
        if(listen (listenfd, 1000000) != 0)
        {
                perror("listen() error");
                exit(1);
        }

        /* Ignore SIGCHLD to avoid zombie threads */
        signal(SIGCHLD,SIG_IGN);

#ifdef HTTPD_INTERNAL
        void *baseaddr = (void *)Terabytes(2);
#else
        void *baseaddr = (void *)(0);
#endif

        void *memory = KAT_VMEM(baseaddr,Megabytes(20));
        struct client_request request = {0};
        struct header_t reqhdr[17] = {{"\0", "\0"}};
        i32 clientfd = 0;
        i32 slot = 0;
        while (1)
        {
                addrlen = sizeof(clientaddr);
                clients[slot] = accept(listenfd, (struct sockaddr *) &clientaddr, &addrlen);

                if (clients[slot]<0)
                {
                        perror("accept() error");
                }
                else
                {
                        if (fork()==0)  /* TODO: fork() is not portable */
                        {
                                respond(slot, clients, clientfd, &request, reqhdr, memory);
                                exit(0);
                        }
                }

                while (clients[slot]!=-1) slot = (slot+1)%CONNMAX;

        }
}
