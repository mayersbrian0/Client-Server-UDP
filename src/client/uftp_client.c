#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#define USAGE_ERROR(S) {printf("Usage: %s [IP ADDR] [PORT #]\n", S); exit(0);} //usage error message
#define RET_BUFFER_SIZE 1024 //handle all non-file transfer responses
#define MAX_COMMAND_LENGTH 60 //max command that we can send
#define MAX_FILENAME_LENGTH 50
#define MAX_FILE_LENGTH 6000000

void error(char *msg) {
    perror(msg);
    exit(0);
}

//prompt for user
void prompt() {
    printf("\nCommand Options: \n------------ \n get [file_name] \n put [file_name] \n delete [file_name]\n ls\n exit\n------------\n>> ");
}

//store users input
typedef struct {
    char* command;
    int size;
} INPUT_BUFFER;

INPUT_BUFFER* create_buffer() {
    INPUT_BUFFER* new_buffer = malloc(sizeof(INPUT_BUFFER));
    new_buffer->size = MAX_COMMAND_LENGTH;
    new_buffer->command = malloc(new_buffer->size);
}

void free_buffer(INPUT_BUFFER* input_buffer) {
    free(input_buffer->command);
    free(input_buffer);
}

//get and format the input from the user
void get_input(INPUT_BUFFER* input_buffer) {
    int size = MAX_COMMAND_LENGTH;

    ssize_t len = getline(&(input_buffer->command), &(input_buffer->size), stdin);
    if (len == -1) { fprintf(stderr, "Error Reading line\n"); exit(0); }
}


int main(int argc, char** argv) {
    int socket_fd, port_num, n;
    int server_len;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname, *ptr;
    char input[1024], filename[MAX_FILENAME_LENGTH];
    unsigned char file_buffer[MAX_FILE_LENGTH + 1]; //buffer to handle sending files

    if (argc != 3) USAGE_ERROR(argv[0]);
    

    port_num = strtol(argv[2], &ptr, 10);
    if ( *ptr != '\0') { printf("Please enter a valid port number]n"); exit(0); }
    
    //create the socket
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd == -1) perror("Error opening socket");
    
    server = gethostbyname(argv[1]); //get the ip addr
    if (server == NULL) { printf("Invalid address: %s\n", argv[1]); exit(0);}
    
    //build ip addr
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr_list[0], (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(port_num);
    server_len = sizeof(serveraddr);

    INPUT_BUFFER* input_buffer = create_buffer();
    while (1) {
        prompt();
        get_input(input_buffer); 

        //send message
        n = sendto(socket_fd, input_buffer->command, input_buffer->size, 0, &serveraddr, server_len);
        if (n < 0) error("Error in sendto()");

        /*
            Handle File Transfer differently than ls,exit,delete (different buffer size, different protocal)
        */
        if (strncmp("get", input_buffer->command, 3) == 0) {
            if (sscanf(input_buffer->command,"get %s", filename) == 1) {
                memset(file_buffer, 0, MAX_FILE_LENGTH);
                n = recvfrom(socket_fd, file_buffer, MAX_FILE_LENGTH, 0, &serveraddr, &server_len);
                if (n < 0) error("Error in recvfrom()");
                printf("%d\n", n);
                if (strncmp("Filename not found", file_buffer, 18) == 0) {
                    printf("Invalid file name\n");
                }

                else {
                    FILE* fp = fopen(filename, "wb+"); //copy contents of buffer to new file
                    fwrite(file_buffer, sizeof(char), n, fp); //copy contents into the file
                    printf("File recieved\n");
                    fclose(fp);
                }
            }

            else {
                printf("Invalid command\n");
            }
        }

        else if (strncmp("put", input_buffer->command,3) == 0) {
            if (sscanf(input_buffer->command, "put %s", filename)) {
                FILE* fp = fopen(filename, "rb");
                size_t bytes_read;
                if (fp == NULL) {
                    printf("File not found: %s\n", filename);
                }
                
                else {
                    memset(file_buffer, 0, MAX_FILE_LENGTH);
                    bytes_read = fread(file_buffer, 1, MAX_FILE_LENGTH, fp);
                    fclose(fp);
                    n = sendto(socket_fd, file_buffer, bytes_read, 0, &serveraddr, server_len);
                    if (n < 0) error("Error in sendto()");
                    printf("Sent file\n");
                }
            }

            else {
                printf("Invalid command\n");
            }
        }

        //break from loop if request client to exit
        else if (strncmp("exit", input_buffer->command, 4) == 0) {
            printf("Server exit request sent\nClient now exiting\n");
            break;
        }

        //handle ls/delete
        else { 
            memset(input, 0, RET_BUFFER_SIZE);
            n = recvfrom(socket_fd, input, RET_BUFFER_SIZE, 0, &serveraddr, &server_len);
            if (n < 0) error("Error in recvfrom()");
            printf("%s\n", input);
        }
    }
    free_buffer(input_buffer);

    return 0;
}