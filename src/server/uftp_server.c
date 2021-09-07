#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>

#define BUFFER_SIZE 1024
#define MAX_FILENAME_LENGTH 50

#define FILE_NOT_FOUND(S) { fprintf(stdout, "File %s not found\n", S); }

//send the contents of given file to the client
void send_file(FILE* fp, int sockfd, struct sockaddr_in clientaddr) {

}


//list the files in current directory and sends to requestor
void ls(int sockfd, struct sockaddr_in clientaddr, int clientlen) {
    char ls_buffer[1024];
    memset(ls_buffer, 0, 1024);
    int n;
    size_t bytes_read;
    struct dirent *de;
    DIR* dr = opendir(".");
    if (dr == NULL) { fprintf(stdout, "Could not open current diretory\n"); return;}
    
    FILE* tmp_fp = fopen("ls_log", "w+");
    
    while ((de = readdir(dr)) != NULL) {
        fprintf(tmp_fp, "%s\n", de->d_name); //write value to a file
    }
    free(dr); 
    
    fseek(tmp_fp, 0, 0); //go back to the start
    bytes_read = fread(&ls_buffer, 1, 1024, tmp_fp); //read at most 1024 bytes
    fclose(tmp_fp);
    remove("ls_log");

    printf("%s\n", ls_buffer);

    n = sendto(sockfd, ls_buffer, strlen(ls_buffer), 0, (struct sockaddr *)&clientaddr, clientlen);
    printf("%d\n", n);
    if (n < 0) error("(ls) ERROR in function sendto");
    
    return;
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
    char *hostaddrp, *ptr;               /* dotted decimal host addr string */
    int optval;                    /* flag value for setsockopt */
    int n;                         /* message byte size */

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    
    portno = strtol(argv[1], &ptr, 10);
    if ( *ptr != '\0') { printf("Please enter a valid port number]n"); exit(0); }

    //create parent socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    //don't have to wait for socket to get cleaned up
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
            if (sscanf(buf, "get %s", filename) == 1) {
                printf("Filename recieved: %s\n", filename);
                FILE* fp = fopen(filename, "r");
                if (fopen == NULL) FILE_NOT_FOUND(filename);
                send_file(fp,sockfd,clientaddr);
            }
        }

        else if (strncmp(buf, "put", 3) == 0) {
            if (sscanf(buf, "put %s", filename) != 1) {

            }
        }

        else if (strncmp(buf, "delete", 6) == 0) {
            if (sscanf(buf, "delete %s", filename) == 1) {
                printf("Filename recieved: %s\n", filename);
                FILE* fp = fopen(filename, "r");
                if (fopen == NULL) FILE_NOT_FOUND(filename);
                fclose(fp);
                remove(filename); //delete the file if it is found
            }
        }

        else if (strncmp(buf, "ls", 2) == 0) {
            ls(sockfd, clientaddr, clientlen);
        }

        else if (strncmp(buf, "exit", 4) == 0) {
        }

        else {
            char * er = "Not Found\n";
            n = sendto(sockfd, er, strlen(er), 0, (struct sockaddr *) &clientaddr, clientlen);
            if (n < 0) error("(ls) ERROR in function sendto");
        }
    }
}