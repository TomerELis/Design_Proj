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
#define password "Kofiko"
#define MAX_SALES 10
#define PORT 8083
#define IP "127.0.0.1"








//prototype


//********************************
//loop fucntion to be completed
//******************************



void getting_data(int sock, char buffer4[BUFFER_SIZE]);


int main() {

	
	int sock = 0;
	struct sockaddr_in serv_addr;
	char buffer[BUFFER_SIZE] = {0};
	char buffer2[BUFFER_SIZE] = {0};
	char buffer3[BUFFER_SIZE] = {0};	
	char buffer4[BUFFER_SIZE] = {0};

	// Create socket
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
           printf("\nSocket creation error\n");
	   return -1;
	    }
	printf("Socket created successfully.\n");


	// Set address family to IPv4
	serv_addr.sin_family = AF_INET;

	// Set port number and convert to network byte order
	serv_addr.sin_port = htons(PORT);
	
	// Convert IPv4 and IPv6 addresses from text to binary form
    	if (inet_pton(AF_INET, IP, &serv_addr.sin_addr) <= 0) {
        	printf("\nInvalid address/ Address not supported\n");
        	return -1;
    	}
    	
    	printf("Address converted successfully.\n");
    	
    	
	    // Connect to server
	    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		printf("\nConnection Failed\n");
		return -1;
	    }
	    printf("Connected to server successfully.\n");
	    
	    
	    
	    //*****************************************the func below not sure needed
	    
	    // Send username
	    char user[50];  // Assuming usernames can be up to 49 characters long
	    char pass[50];  // Assuming passwords can be up to 49 characters long
	    

            getting_data(sock, buffer4);
	    printf("Enter username: ");
	    scanf("%49s", user);  // Read up to 49 characters for username (prevent buffer overflow)
	    
	    // Clear buffer to ensure no garbage data is sent
	    memset(buffer, 0, BUFFER_SIZE);

	    // Format data to send to the server
	    snprintf(buffer, BUFFER_SIZE, "%s\n", user);
	    

	    // Send data to the server
	    ssize_t bytes_sent = send(sock, buffer, strlen(buffer), 0);
	    if (bytes_sent < 0) {
		perror("send failed");
		close(sock);
		return -1;
	    }
	    
	    // Read initial message from server
	    //read(sock, buffer, BUFFER_SIZE);
	    //printf("%s", buffer);
	    
	    printf("Enter password: ");
	    scanf("%49s", pass);  // Read up to 49 characters for password (prevent buffer overflow)

	    // Clear buffer to ensure no garbage data is sent
	    memset(buffer, 0, BUFFER_SIZE);

	     // Format data to send to the server
	    snprintf(buffer2, BUFFER_SIZE, "%s", pass);
	    
	    // Send data to the server
	    ssize_t pass_sent = send(sock, buffer2, strlen(buffer2), 0);
	    if (pass_sent < 0) {
		perror("send failed");
		close(sock);
		return -1;
	    }

	    printf("Connecting to the server... Data sent: user %s, pass %s\n", user, pass);
	    
		



    // Main loop to check for user input or other operations
    printf("Loading\n");
    int i=0;
    char num_menu[50];
    while (1) {
            	getting_data(sock, buffer4);
		getting_data(sock, buffer4);
        	

		printf("Choose menu number ");
	    	scanf("%49s",num_menu ); 

 		// Clear buffer to ensure no garbage data is sent
	    	memset(buffer, 0, BUFFER_SIZE);

	     // Format data to send to the server
	    snprintf(buffer3, BUFFER_SIZE, "%s", num_menu); 
		 // Send data to the server
	    ssize_t bytes_sent = send(sock, num_menu, strlen(buffer3), 0);
	    if (bytes_sent < 0) {
		perror("send failed");
		close(sock);
		return -1;
	    }
        

    }


    		// Close the socket
    		close(sock);
    		printf("Connection closed.\n");

		return 0;
}




void getting_data(int sock, char buffer4[BUFFER_SIZE]){
    int data_recieved = recv(sock, buffer4, BUFFER_SIZE,0);	//getting menu
    if (data_recieved > 0) 
   	 printf("this data delivered from the server:\n%s", buffer4);
    else{
	printf("Bad getting data\n");
	close(sock);
	}
}
