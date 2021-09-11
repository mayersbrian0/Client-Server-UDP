#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#define USAGE_ERROR(S) {printf("Usage: %s [IP ADDR] [PORT #]\n", S); exit(0);} //usage error message
#define BUFFER_SIZE 1024
#define MAX_COMMAND_LENGTH 35

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
    char input[1024];

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

    INPUT_BUFFER* input_buffer = create_buffer();
    while (1) {
        prompt();
        get_input(input_buffer); 

        //send message
        server_len = sizeof(serveraddr);
        n = sendto(socket_fd, input_buffer->command, input_buffer->size, 0, &serveraddr, server_len);
        if (n < 0) error("Error in sendto()");

        memset(input, 0, 1024);
        n = recvfrom(socket_fd, input, 1024, 0, &serveraddr, &server_len);
        if (n < 0) error("Error in recvfrom()");
        printf("%s\n", input);
    }
    free_buffer(input_buffer);

    return 0;
}