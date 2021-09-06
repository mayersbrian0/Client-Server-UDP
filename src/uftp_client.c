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
    char* buffer;
    long buffer_size; //number allocated
    ssize_t buffer_length; //number of actual characters
} INPUT_BUFFER;


//allocate space for user input
INPUT_BUFFER* create_buffer() {
    INPUT_BUFFER* new_buff = malloc(sizeof(INPUT_BUFFER));
    new_buff->buffer = NULL;
    new_buff->buffer_size = BUFFER_SIZE;
    new_buff->buffer_length =0;
}

//clean up when finished
void free_buffer(INPUT_BUFFER* buff) {
    free(buff->buffer);
    free(buff);
}

//get and format the input from the user
void get_input(INPUT_BUFFER* input_buffer) {
    ssize_t bytes_read = getline(&(input_buffer->buffer), &(input_buffer->buffer_size), stdin); //get user input geline, NOTE: getline calls realloc
    if (bytes_read <= 0) { printf("Error reading input"); exit(0); }

    input_buffer->buffer_length = bytes_read -1;
    input_buffer->buffer[bytes_read -1] = '\0';
}


int main(int argc, char** argv) {
    int socket_fd, port_num, n;
    int server_len;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname, *ptr;

    if (argc != 3) USAGE_ERROR(argv[0]);
    

    port_num = strtol(argv[2], &ptr, 10);
    if ( *ptr != '\0') { printf("Please enter a valid port number]n"); exit(0); }
    
    //create the socket
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd == -1) perror("Error opening socket");
    
    server = gethostbyname(argv[1]); //get the ip addr
    if (server == NULL) { printf("Invalid address: %s\n", argv[1]); exit(0);}
    
    //build ip addr
    bzero((char *)&serveraddr, sizeof(serveraddr)); 
    serveraddr.sin_family = AF_INET; 
    bcopy((char *)server->h_addr_list[0], (char *)&serveraddr.sin_addr.s_addr, server->h_length); 
    serveraddr.sin_port = htons(port_num);

    INPUT_BUFFER* input_buffer = create_buffer();
    while (1) {
        prompt();
        get_input(input_buffer); 

        //send message
        server_len = sizeof(serveraddr);
        n = sendto(socket_fd, input_buffer->buffer, strlen(input_buffer->buffer), 0, &serveraddr, server_len);
        if (n < 0) error("Error in sendto()");

        char input[1024];
        memset(input, 0, 1024);
        n = recvfrom(socket_fd, input, 1024, 0, &serveraddr, &server_len);
        if (n < 0) error("Error in recvfrom()");
        printf("%s\n", input);

    }
    free_buffer(input_buffer);

    return 0;
}