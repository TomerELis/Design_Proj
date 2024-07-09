#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>



#include <pthread.h>

#define BUFFER_SIZE 1024
#define maxLen 256
#define secret_password "Kofiko"
#define MAX_SALES 10
#define PORT 8083
#define IP 127.0.0.1
#define MAX_CLIENTS 10


//****************************
typedef struct {
    /*
    Structure to store client information
    */
    int socket;
    struct sockaddr_in address;
    int client_id;
    int username;
    int password;
    int bet_team;
    int bet_amount;
    char comments[BUFFER_SIZE];
} Client;
//****************************


//prototype
int createWelcomeSocket();
void accept_bets(int server_fd);

//global shiri
Client *clients[MAX_CLIENTS];
int client_count = 0;
//GameState game_state;
time_t start_time;
int game_started = 0;  // Flag to indicate if the game has started

int main() {

	//int server_fd;
    	struct sockaddr_in address;
    	int opt = 1;
    	
    	

	
	// Create socket
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		printf("\nSocket creation error\n");
		return -1;
	}
	printf("Socket created successfully.\n");


	



	
	//***********************************************************
	// Set socket option to allow address reuse
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
	    perror("setsockopt");   // Print error if setting socket option fails
	    close(server_fd);       // Close the socket
	    exit(EXIT_FAILURE);     // Exit with failure status
	}
	printf("Socket options set successfully.\n");

	// Set address family (IPv4), IP to listen on all interfaces, and port number
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	// Bind the socket to the specified address
	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
	    perror("bind failed");  // Print error if binding fails
	    close(server_fd);       // Close the socket
	    exit(EXIT_FAILURE);     // Exit with failure status
	}
	printf("Socket bound successfully.\n");

	// Set the socket to listen for incoming connections
	if (listen(server_fd, 3) < 0) {
	    perror("listen");       // Print error if listen fails
	    close(server_fd);       // Close the socket
	    exit(EXIT_FAILURE);     // Exit with failure status
	}
	printf("Server is listening on port %d.\n", PORT);

    //*******************************the above func not sure if needed
	

	accept_bets(server_fd);
		
		return 0;
}




/*
Accept bets from new clients until the game starts
*/
void accept_bets(int server_fd) {
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int new_socket;
    int client_count = 0; 

    char buffer[BUFFER_SIZE] = {0};
    ssize_t bytes_received;
    //start_time = time(NULL);

    while (1) {

    
    //****************************************************************************************
        //pthread_mutex_lock(&lock_time);
        //time_t current_time = time(NULL);
        //double remaining_time = difftime(start_time + GAME_DURATION, current_time);
        //pthread_mutex_unlock(&lock_time);

        //if (remaining_time <= 0 && !game_started) {
        //   start_game();
         //  break;
        //}
    //****************************************************************************************
	//check if there new client
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket >= 0) {
            Client *client = (Client *)malloc(sizeof(Client));
            client->socket = new_socket;
            client->address = address;
            client->client_id = client_count++;
            memset(client->comments, 0, BUFFER_SIZE);

            clients[client_count - 1] = client;
	}
	
	//check if client send data
	bytes_received = recv(server_fd, buffer, BUFFER_SIZE, 0);
	if(bytes_received>=0){	
	// Receive offer from client
	
         printf("Received offer from client : unknon - %s\n", buffer);
         // Store offer in client struct or process it as needed
         //strncpy(client->comments, buffer, BUFFER_SIZE);
            //pthread_t client_thread;
            //pthread_create(&client_thread, NULL, handle_new_client, (void *)client);
        }
    }
}


