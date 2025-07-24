#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>

#include <arpa/inet.h>

#define MAX_BUFFER 16384

int init_connection(char* ip, int port, int* client_fd, struct sockaddr_in* address) {
    *client_fd = socket(AF_INET, SOCK_STREAM, 0);

    address->sin_family = AF_INET;
    address->sin_port = htons(port);

    if (inet_aton(ip, (struct in_addr *) &(address->sin_addr.s_addr)) == 0) {
        printf("Could not find IP.\n");
        exit(EXIT_FAILURE);
    }
    printf("Found IP.\n");

    printf("trying to connect to port %i\n", ntohs(address->sin_port));
    int result = connect(*client_fd, (struct sockaddr *) address, sizeof(*address));
    if (result < 0) {
        printf("Could not connect; Errno: %s (errno %d)\n", strerror(errno), errno);
        exit(EXIT_FAILURE);
    }
    printf("Connected.\n");

    return 0;
}

int main(int argc, char* argv[])
{
    if (argc < 3) {
        printf("Error - Not enough arguments. Include IP and Port.\n");
        return -1;
    }
   
    // You just need to type the IP ending on the terminal (c.d)
    char ip_initials[] = "192.168.";
    char ip_rest[] = "255.255";
    strcpy(ip_rest, argv[1]);
    char ip[32] = {0};
    snprintf(ip, sizeof(ip), "%s%s", ip_initials, ip_rest);
    int port = atoi(argv[2]);
    printf("Trying IP %s:%d\n", ip, port);
    

    // init the tcp
    int client_fd;
    struct sockaddr_in server_address;
    if (init_connection(ip, port, &client_fd, &server_address) < 0) {
        perror("init_connection");
        exit(EXIT_FAILURE);
    }


    ssize_t valread;
    int max_fd;
    fd_set readfds;
    char buffer[MAX_BUFFER] = { 0 };
    char* hello = "Hello from the client!\n";

    // hello protocol
    send(client_fd, hello, strlen(hello), 0);
    printf("SENT: %s\n", hello);
    valread = read(client_fd, buffer, MAX_BUFFER - 1);
    printf("READ: %s\n", buffer);

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
            
            uint32_t output;
            memcpy(&output, buffer, sizeof(uint32_t));
            uint32_t outputFormatted = ntohl(output);
            printf("Received: %x\n", outputFormatted);

            if (strcmp(buffer, "exit") == 0) {
                printf("Peer has exited, closing connection.\n");
                break;
            }

            if (strcmp(buffer, "Left trigger pressed!\n") == 0) {
                // scary ngl
                system("mpg123 -f \"15000\" -q baubau.mp3 &");
                printf("Bau bau!!\n");
            }
        }

        // check for input in the terminal
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            if (fgets(buffer, MAX_BUFFER, stdin) == NULL)  {
                perror("fgets error");
                break;
            }

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
    return 0;
}