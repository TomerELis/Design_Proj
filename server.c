#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#define BUFFER_SIZE 1024
#define PORT 8083
#define MAX_CLIENTS 10
#define secret "Kofiko"
#define MAX_SALES 10
#define password_DURATION 30  // 1 minute since we open the server

typedef struct {
    int socket;
    struct sockaddr_in address;
    int client_id;
    char user_name[BUFFER_SIZE];
    char comments[BUFFER_SIZE];
} Client;

struct Sale {
    int id;
    char title[50];  // Assuming titles can be up to 50 characters long
    char multicast_ip[50];
    int num_of_clients;
};

//help function
void sendMenu(int clientSocket);

//global parameters
int num_of_sales = 4;
Client *clients[MAX_CLIENTS];
int client_count = 0;
int flg=0;
time_t current_time,start_time;
int remaining_time;	


int accept_bets(int server_fd) {
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int new_socket;
    char buffer[BUFFER_SIZE] = {0};
    ssize_t bytes_received,bytes_received_sale;
    Client *client = (Client *)malloc(sizeof(Client));
    
    new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        //insert thread
        
        if (new_socket >= 0) {
       
            client->socket = new_socket;
            client->address = address;
            client->client_id = client_count++;
            memset(client->comments, 0, BUFFER_SIZE);

            clients[client->client_id] = client;
            printf("Client %d connected.\n", client->client_id);
        }
	
    while (1) {

        

        // Receive data from client if available
        printf("!!!!!!!!!\n");
        bytes_received = recv(new_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';  // Null-terminate the received data
            if(flg==1)
            {
             printf("Password use the secret password %d: %s\n", client_count - 1, buffer);
             flg=0;
            
             int result = strcmp(secret, buffer);




		    if (result == 0) {		//usernasme enter the menu mode
	   		printf("username enter the right pass - %s\n", buffer);
	   		
	   		
	   		//****************
	   		// Send data to the client
	   		sleep(2);
	   		sendMenu(client->socket);
			bytes_received_sale = recv(new_socket, buffer, BUFFER_SIZE, 0);
	   		/*char menu[40]= "POPO_SHMOPO_IN_THE_HOUSE";
	    		ssize_t menu_send = send(client->socket, menu, strlen(menu), 0);
	    		if (menu_send < 0) {
			perror("send failed");
			close(client->socket);
				return -1;
	    		}*/
	   		
		    
		    }
		    	else{	//username needed to be socket closed (maybe after 3 wrong)
		    	printf("str1 and str2 are NOT equal - WEEEWOOOOWEEEEWOOOO\n");
		   	 }
	    }
            else if(flg==0)
            {printf("Received offer to username from client %d: %s\n", client_count - 1, buffer);
                strncpy(client->user_name, buffer, BUFFER_SIZE);
            	flg=1;
            	//printf("POPO!: %s",client->user_name);
            	}
            	
            
            strncpy(clients[client_count - 1]->comments, buffer, BUFFER_SIZE);
               
            
            // Send data to the client his username is 
            
	
        } else if (bytes_received == 0) {
            printf("Client %d disconnected\n", client_count - 1);
            close(new_socket);
        } else {
 	    close(new_socket);
            perror("Receive failed");
		start_time = time(NULL);
	
		while(1){
		current_time = time(NULL);
 		remaining_time = (int)difftime(start_time + password_DURATION, current_time);
	 	printf("\rabord in:%d",remaining_time);
		if (remaining_time <=0){
			break;
		}
	}
           return -1;
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
    while(1){
    accept_bets(server_fd);
	}

    // Close the server socket (unreachable in this loop)
    close(server_fd);

    return 0;
}
// Function to send menu to client
void sendMenu(int clientSocket) {

    struct Sale sales[MAX_SALES] = {
        {1, "Summer Sale","0.0.0.0",0},
        {2, "Back to School Sale","0.0.0.0",0},
        {3, "Holiday Sale","0.0.0.0",0},
        {4, "End of Year Clearance","0.0.0.0",0},
        {-1, "Exit","0.0.0.0",0}
    };

    char Mbuffer[1024];  // Buffer to hold serialized menu data
    memset(Mbuffer, 0, sizeof(Mbuffer));  // Clear buffer

    // Serialize menu data into buffer
    int offset = 0;
    for (int i = 0; i < num_of_sales+1; ++i) {
        offset += sprintf(Mbuffer + offset, "%d. %s\n", sales[i].id, sales[i].title);
    }

    // Send serialized menu to client
    int succeed = send(clientSocket, Mbuffer, strlen(Mbuffer), 0);
    if (succeed < 0) {
        perror("send failed");
        // Handle send failure as per your server application logic
    }
}
