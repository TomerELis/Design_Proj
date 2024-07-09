#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <pthread.h>

#define Buffer_size 1024
#define maxLen 256
#define password "Kofiko"
#define MAX_SALES 10

//prototype
int createWelcomeSocket();

int main() {

	createWelcomeSocket();
	listen(lisen_sock, SOMAXCONN)
}

// Function to create and configure the welcome socket
int createWelcomeSocket(){
	
// Create socket
		serverSocket = socket(AF_INET, SOCK_STREAM, 0)
	if (serverSocket < 0) {
		printf("\nSocket creation error\n");
		return -1;
	}
	printf("Socket created successfully.\n");

	int bind_worked = bind(serverSocket,(struct sockaddr *)&serverAddr,server_size)
	if (bind_worked < 0) {
		printf("\nSocket bind error\n");
		close(serverSocket);
		return -1;
	}


}

