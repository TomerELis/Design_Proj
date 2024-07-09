#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define PORT 8083
#define MAX_CLIENTS 10

typedef struct {
    int socket;
    struct sockaddr_in address;
    int client_id;
    char comments[BUFFER_SIZE];
} Client;

Client *clients[MAX_CLIENTS];
int client_count = 0;

void accept_bets(int server_fd) {
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int new_socket;
    char buffer[BUFFER_SIZE] = {0};
    ssize_t bytes_received;

    while (1) {
    printf("!!!!fdsfdfssdf");
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        //insert thread
        
        if (new_socket >= 0) {
            Client *client = (Client *)malloc(sizeof(Client));
            client->socket = new_socket;
            client->address = address;
            client->client_id = client_count++;
            memset(client->comments, 0, BUFFER_SIZE);

            clients[client->client_id] = client;
            printf("Client %d connected.\n", client->client_id);
        }

        // Receive data from client if available
        printf("!!!!!!!!!\n");
        bytes_received = recv(new_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';  // Null-terminate the received data
            printf("Received offer to username from client %d: %s\n", client_count - 1, buffer);
            printf("befot\n");
            strncpy(clients[client_count - 1]->comments, buffer, BUFFER_SIZE);
            printf("after\n");    
            
            // Send data to the client his username is 
            
	
        } else if (bytes_received == 0) {
            printf("Client %d disconnected\n", client_count - 1);
            close(new_socket);
        } else {
            perror("Receive failed");
        }
    }
}

int main() {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }
    printf("Socket created successfully.\n");

    // Set socket option to allow address reuse
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Setsockopt error");
        close(server_fd);
        return -1;
    }
    printf("Socket options set successfully.\n");

    // Set address family (IPv4), IP to listen on all interfaces, and port number
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the specified address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return -1;
    }
    printf("Socket bound successfully.\n");

    // Set the socket to listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(server_fd);
        return -1;
    }
    printf("Server is listening on port %d.\n", PORT);

    // Accept bets from clients
    accept_bets(server_fd);

    // Close the server socket (unreachable in this loop)
    close(server_fd);

    return 0;
}

