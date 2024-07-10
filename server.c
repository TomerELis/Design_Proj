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
#define password_DURATION 5  // 1 minute since we open the server

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
    char multicast_ip[50], data[50];
    int num_of_clients;
    time_t star_time;
};

//help function
void sendMenu(int clientSocket);
void sending_data(int clientSocket, char data[50]);
void getting_data(int clientSocket, char data[50]);
int createWelcomeSocket(short port, int maxClient);


//global parameters
int num_of_sales = 4;
Client *clients[MAX_CLIENTS];
int client_count = 0;
int flg=0;
time_t current_time,start_time,real_time;
int remaining_time;
int serverSocket;

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

        //char dat[50] = "abcdefg";			//******HOW TO SEND DATA TO CLIENT
	//sending_data(client->socket, dat);
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
			while(1)			//while user choose right menu numb
			{
		   		sendMenu(client->socket);
				memset(buffer, 0, BUFFER_SIZE);
				bytes_received_sale = recv(new_socket, buffer, BUFFER_SIZE, 0);
				int numb_menu = atoi(buffer);
				if (numb_menu > num_of_sales)
				{
					printf("problem cause in menu we have only %d options and user 					choose number : %d \n", num_of_sales, numb_menu);
				}
				else
				{
				printf("username choose on menu the numb - %s\n", buffer);
				char dat[50] = "371";
				sending_data(client->socket, dat);				
				break;
				}
			}

	   		/*char menu[40]= "POPO_SHMOPO_IN_THE_HOUSE";
	    		ssize_t menu_send = send(client->socket, menu, strlen(menu), 0);
	    		if (menu_send < 0) {
			perror("send failed");
			close(client->socket);
				return -1;
	    		}*/
	   		
		    
		    }
		    	else{	//username needed to be socket closed (maybe after 3 wrong)
			sending_data(client->socket,"worng password i call to the police WEEWOOWEEOO\n");
			close(server_fd);
			return -1;
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
			//break;
			return -1;
		}
	}
           return -1;
		}
	}
}

    



int main() {
	real_time = time(NULL);
    printf("Server is listening on port %d.\n", PORT);


    // Accept bets from clients
    while(1){
    serverSocket = createWelcomeSocket(PORT, MAX_CLIENTS);
    accept_bets(serverSocket);
	}

    // Close the server socket (unreachable in this loop)
    close(serverSocket);


    return 0;
}
// Function to send menu to client

void sending_data(int clientSocket, char data[50]) {

	int send_me = send(clientSocket, data, strlen(data), 0);
	//send_me = recv(clientSocket, data, BUFFER_SIZE, 0);

	if (send_me < 0) 
	  perror("send failed");
	printf("sss");
   };


void getting_data(int clientSocket, char data[50]){
    int data_send = recv(clientSocket, data, BUFFER_SIZE,0);	//getting menu
    if (data_send > 0) 
   	 printf("this data delivered from the server:\n%s", data);
    else
	printf("Bad getting data\n");


}



void sendMenu(int clientSocket) {

    struct Sale sales[MAX_SALES] = {
        {1, "Summer Sale","224.2.1.1",0,220},
        {2, "Back to School Sale","224.2.2.1",0,60},
        {3, "Holiday Sale","224.2.3.1",0,215},
        {4, "End of Year Clearance","224.2.4.1",0,120},
        {-1, "Exit","0.0.0.0",0,0}
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

// Function to mange sale
int check_sale(struct Sale my_sale) {
		current_time = time(NULL);
 		remaining_time = (int)difftime(real_time + my_sale.star_time, current_time);
		if(remaining_time >0){
	 		printf("\rsale start in:%d",remaining_time);
		}
		return 1;
}

// Function to create and configure the welcome socket
int createWelcomeSocket(short port, int maxClient){
    int serverSocket, opt=1;
    struct sockaddr_in serverAddr;
    socklen_t server_size;

    // Create TCP socket
    serverSocket= socket(PF_INET,SOCK_STREAM,0);
    if(serverSocket<0){
        perror("socket failed");
        return -1;
    }
    // Set socket options to reuse address and port
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,&opt, sizeof(opt))){
        perror("socket option failed");
        close(serverSocket);
        return -1;
    }
    // Configure server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    server_size= sizeof(serverAddr);

    // Bind server socket to address and port
    if((bind(serverSocket,(struct sockaddr *)&serverAddr,server_size))<0) {
        perror("binding failed");
        close(serverSocket);
        return -1;
    }

    // Start listening for client connections
    printf("Server is listen to port %d and wait for new client...\n", port);
    if((listen(serverSocket,maxClient))<0){
        perror("listen failed");
        close(serverSocket);
        return -1;
    }
    return serverSocket;
}

//creating multicast group
void ceating_multicast(char multicast_ip[16]){
	
	struct sockaddr_in addr;
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	addr.sin_family= AF_INET; 
	addr.sin_addr.s_addr= inet_addr(multicast_ip);
	addr.sin_port= htons(6000); 
}

//sending message to the  multicast group
void sending_multicast(char mess[1024],int sock,struct sockaddr_in addr){	
	int addrlen = sizeof(addr);
	sendto(sock, mess, strlen(mess), 0, (struct sockaddr*) &addr, sizeof(addr));
}
