/**
 * httpd.c
 */

#include <stdio.h>       /* For perror()        */
#include <stdlib.h>      /* For exit()          */
#include <unistd.h>      /* For fork()          */
#include <string.h>      /* For memset()        */
//#include <sys/types.h>   /* For ???             */
#include <sys/socket.h>  /* For accept()
                                socket()
                                socketaddr_in 
                                socklen_t       */
#include <netdb.h>       /* For struct addrinfo */
#include <signal.h>      /* For signal() */
#include <sys/mman.h>    /* For mmap()   */

/**
 * NOTE: MAP_ANONYMOUS is not defined on some other
 *       UNIX systems, use MAP_ANON instead.
 */
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

/* Disambiguate static keyword */
#define global static
#define internal static
#define persist static

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
        int payload_size;
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
        printf("Fetch the data using `payload` variable.");
    }
  
    ROUTE_END()
}

internal void respond(int n, int clients[], int clientfd,
                      struct client_request *request, struct header_t *reqhdr, char *memory)
{
        int rcvd, fd, bytes_read;
        char *ptr;

        rcvd = recv(clients[n], memory, Megabytes(20), 0);

        if(rcvd<0)        /* receive error */
                fprintf(stderr,("recv() error\n"));
        else if(rcvd==0)  /* receive socket closed */
                fprintf(stderr,"Client disconnected upexpectedly.\n");
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
                while(h < reqhdr+16) {
                        char *k,*v,*t;
                        k = strtok(NULL, "\r\n: \t"); if (!k) break;
                        v = strtok(NULL, "\r\n");     while(*v && *v==' ') v++;
                        h->name  = k;
                        h->value = v;
                        h++;
                        fprintf(stderr, "[H] %s: %s\n", k, v);
                        t = v + 1 + strlen(v);
                        if (t[1] == '\r' && t[2] == '\n') break;
                }
                t++; // now the *t shall be the beginning of user payload
                t2 = request_header("Content-Length", h); // and the related header if there is  
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

int main(int argc, char* argv[])
{
        struct sockaddr_in clientaddr;
        socklen_t addrlen;

        char port[5] = "8080\0";

        /* Init client array */
        int clients[CONNMAX];
        for(int i = 0; i < CONNMAX; i++)
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
        int listenfd;
        for(p = res; p!=NULL; p=p->ai_next)
        {
                int option = 1;
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

        void *memory = mmap(baseaddr,Megabytes(20),
                            PROT_READ|PROT_WRITE,MAP_ANONYMOUS|MAP_PRIVATE,-1,0);
        struct client_request request = {0};
        struct header_t reqhdr[17] = {{"\0", "\0"}};
        int clientfd = 0;
        int slot = 0;
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
                        if (fork()==0 )  /* TODO: fork() is not portable */
                        {
                                respond(slot, clients, clientfd, &request, reqhdr, memory);
                                exit(0);
                        }
                }

                while (clients[slot]!=-1) slot = (slot+1)%CONNMAX;

        }
}
