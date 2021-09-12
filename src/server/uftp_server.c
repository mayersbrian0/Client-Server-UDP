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
#define MAX_FILE_LENGTH 5120 //max is 5KB

//send the contents of given file to the client
void send_file(int sockfd, struct sockaddr_in clientaddr, int clientlen, char* filename, char* file_buffer) {
    int n;
    size_t bytes_read;

    printf("File transfer request: %s\n", filename);
    FILE* fp = fopen(filename, "rb"); 
    if (fp == NULL) {
        printf("File not found: %s\n", filename);
        memset(file_buffer, 0, strlen(file_buffer));
        strcpy(file_buffer, "Filename not found");
        n = sendto(sockfd, file_buffer, strlen(file_buffer), 0, (struct sockaddr *) &clientaddr, clientlen);
        if (n < 0) error("(get) ERROR in function sendto");
    }

    else {
        bytes_read = fread(file_buffer, sizeof(char), MAX_FILE_LENGTH, fp);
        file_buffer[bytes_read] = '\0';
        n = sendto(sockfd, file_buffer, strlen(file_buffer), 0, (struct sockaddr *) &clientaddr, clientlen);
        if (n < 0) error("(not found) ERROR in function sendto");
        fclose(fp);
        printf("File sent: %s\n", filename);
    }
}

void receive_file(int sockfd, struct sockaddr_in clientaddr, int clientlen, char* filename, char* file_buffer) {
    int n;
    FILE* fp = fopen(filename, "wb+"); //create a new file
    n = recvfrom(sockfd, file_buffer, MAX_FILE_LENGTH, 0, (struct sockaddr *)&clientaddr, &clientlen);
    if (n < 0) error("ERROR in recvfrom");
    fwrite(file_buffer, sizeof(char), strlen(file_buffer), fp); //copy contents into the file
    printf("File %s added\n", filename);
    fclose(fp);
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

    n = sendto(sockfd, ls_buffer, strlen(ls_buffer), 0, (struct sockaddr *)&clientaddr, clientlen);
    if (n < 0) error("(ls) ERROR in function sendto");
    
    return;
}

//function to handle deleteing a file
void delete_file(int sockfd, struct sockaddr_in clientaddr, int clientlen, char* filename, char* ret) {
    int n;
    printf("Filename recieved: %s\n", filename);
    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("File not found: %s\n", filename);
        memset(ret, 0, strlen(ret));
        strcpy(ret, "Filename not found: ");
        strcat(ret, filename);
        n = sendto(sockfd, ret, strlen(ret), 0, (struct sockaddr *) &clientaddr, clientlen);
        if (n < 0) error("(delete) ERROR in function sendto");
    }

    else {
        printf("File: %s deleted\n", filename);
        memset(ret, 0, strlen(ret));
        strcpy(ret, "File deleted");
        fclose(fp);
        remove(filename); //delete the file if it is found
        n = sendto(sockfd, ret, strlen(ret), 0, &clientaddr, clientlen);
        if (n < 0) error("(delete) ERROR in function sendto");
    }
}

//send message on invalid command
void invalid_command(int sockfd, struct sockaddr_in clientaddr, int clientlen, char* ret) {
    int n;
    memset(ret, 0, strlen(ret));
    strcpy(ret, "Command not found");
    n = sendto(sockfd, ret, strlen(ret), 0, (struct sockaddr *) &clientaddr, clientlen);
    if (n < 0) error("(not found) ERROR in function sendto");
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
    char ret[65], filename[MAX_FILENAME_LENGTH], file_buffer[MAX_FILE_LENGTH + 1]; //buffers to be used

    if (argc != 2) {
        fprintf(stderr, "Usage: %s [PORT #]\n", argv[0]);
        exit(1);
    }
    
    portno = strtol(argv[1], &ptr, 10);
    if ( *ptr != '\0') { printf("Please enter a valid port number]n"); exit(0); }

    //create parent socket
     //create parent socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) error("ERROR opening socket");

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
    printf("Server listening on port: %s\n", argv[1]);
    
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

        //handle possible commands
        if (strncmp(buf, "get", 3) == 0) {
            if (sscanf(buf, "get %s", filename) == 1) {
                send_file(sockfd, clientaddr, clientlen, filename, file_buffer);
            }
        }

        else if (strncmp(buf, "put", 3) == 0) {
            if (sscanf(buf, "put %s", filename) == 1) {
                receive_file(sockfd, clientaddr, clientlen, filename, file_buffer);
            }
        }

        else if (strncmp(buf, "delete", 6) == 0) {
            if (sscanf(buf, "delete %s", filename) == 1) {
                delete_file(sockfd, clientaddr, clientlen, filename, ret);
            }
        }

        else if (strncmp(buf, "ls", 2) == 0) {
            printf("ls command requested\n");
            ls(sockfd, clientaddr, clientlen);
        }

        else if (strncmp(buf, "exit", 4) == 0) {
            printf("Server Shutting Down\n");
            break;
        }

        else {
            invalid_command(sockfd, clientaddr, clientlen, ret);
        }
    }

    return 0;
}