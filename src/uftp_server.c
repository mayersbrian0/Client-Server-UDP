#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024
#define MAX_FILENAME_LENGTH 50


char* call_ls() {
    
}

int main(int argc, char **argv)
{
    int sockfd;
    int portno;
    int clientlen;                 /* byte size of client's address */
    struct sockaddr_in serveraddr; /* server's addr */
    struct sockaddr_in clientaddr; /* client addr */
    struct hostent *hostp;         /* client host info */
    char buf[BUFFER_SIZE];         /* message buf */
    char *hostaddrp;               /* dotted decimal host addr string */
    int optval;                    /* flag value for setsockopt */
    int n;                         /* message byte size */

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    portno = atoi(argv[1]);

    //create parent socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
    optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
               (const void *)&optval, sizeof(int));

    //build ip addr
    bzero((char *)&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)portno);

    //bind: associate the parent socket with a port
    if (bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) error("ERROR on binding");

    clientlen = sizeof(clientaddr);
    while (1) {
        //recvfrom: receive a UDP datagram from a client
        bzero(buf, BUFFER_SIZE);
        n = recvfrom(sockfd, buf, BUFFER_SIZE, 0, (struct sockaddr *)&clientaddr, &clientlen);
        if (n < 0) error("ERROR in recvfrom");

        //gethostbyaddr: determine who sent the datagram
        hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);
        if (hostp == NULL)
            error("ERROR on gethostbyaddr");
        hostaddrp = inet_ntoa(clientaddr.sin_addr);
        if (hostaddrp == NULL) error("ERROR on inet_ntoa\n");
        printf("server received datagram from %s (%s)\n", hostp->h_name, hostaddrp);
        printf("server received %d/%d bytes: %s\n", strlen(buf), n, buf);

        n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&clientaddr, clientlen);
        if (n < 0) error("ERROR in sendto");

        //handle possible commands
        char filename[MAX_FILENAME_LENGTH];
        if (strncmp(buf, "get", 3) == 0) {
            if (sscanf(buf, "get %s", filename) != 1) return;
            printf("%s\n", filename);
        }

        else if (strncmp(buf, "put", 3) == 0) {
            if (sscanf(buf, "put %s", filename) != 1) return;
            printf("%s\n", filename);
        }

        else if (strncmp(buf, "delete", 6) == 0) {
            if (sscanf(buf, "delete %s", filename) != 1) return;
            printf("%s\n", filename);
        }

        else if (strncmp(buf, "ls", 2) == 0) {
            call_ls();
        }

        else if (strncmp(buf, "exit", 4) == 0) {
        }

        else {
            
        }
    }
}