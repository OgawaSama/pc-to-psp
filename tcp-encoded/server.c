#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>

#include <arpa/inet.h>

#define MAX_BUFFER 1024

int init_connection(int* server_fd, int* client_fd, struct sockaddr_in* address) {
    if ((*server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    } 
    printf("socket created\n");

    address->sin_family = AF_INET;
    address->sin_port = htons(8080);
    address->sin_addr.s_addr = INADDR_ANY;

    int opt = 1;
    if (setsockopt(*server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    } 
    printf("socket set\n");

    if (bind(*server_fd, (struct sockaddr*) address, sizeof(*address)) < 0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    } 
    printf("bind successful\n");

    // allows up to 4 clients
    if (listen(*server_fd, 4) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    } 
    printf("listening on port %i\n", ntohs(address->sin_port));

    socklen_t addrlen = sizeof(*address);
    if ((*client_fd = accept(*server_fd, (struct sockaddr*) address, &addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    } 
    printf("Accepted someone\n");

    return 0;
}

int main(int argc, char* argv[]) {

    int server_fd, client_fd;
    struct sockaddr_in address;

    if (init_connection(&server_fd, &client_fd, &address) < 0) {
        perror("init_connection");
        exit(EXIT_FAILURE);
    }
    
    ssize_t valread;
    int max_fd;
    fd_set readfds;
    char buffer[MAX_BUFFER] = { 0 };
    char* hello = "Hello from the server!";

    // send and receives a "hello"
    valread = read(client_fd, buffer, MAX_BUFFER - 1);
    printf("READ: %s\b\n", buffer);
    send(client_fd, hello, strlen(hello), 0);
    printf("SENT: %s\b\n", hello);

    max_fd = (client_fd > STDIN_FILENO) ? client_fd : STDIN_FILENO;
    printf("Chat started. Type 'exit' to quit.\n");
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(client_fd, &readfds);

        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            perror("select error");
            break;
        }

        if (FD_ISSET(client_fd, &readfds)) {
            valread = read(client_fd, buffer, MAX_BUFFER - 1);
            if (valread <= 0) {
                printf("Connection closed by peer\n");
                break;
            }

            buffer[valread] = '\0';
            printf("Received: %s", buffer);

            if (strcmp(buffer, "exit") == 0) {
                printf("Peer has exited, closing connection.\n");
                break;
            }
        }

        // check for input in the terminal
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            if (fgets(buffer, MAX_BUFFER, stdin) == NULL)  {
                perror("fgets error");
                break;
            }

            // if "exit" is typed, exit program.
            if (strcmp(buffer, "exit\n") == 0) {
                send(client_fd, buffer, strlen(buffer), 0);
                printf("Exiting...\n");
                break;
            }

            send(client_fd, buffer, strlen(buffer), 0);
            printf("Sent: %s", buffer);
        }
    }


    close(client_fd);
    close(server_fd);


    return 0;
}