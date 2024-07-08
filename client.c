#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

#define Buffer_size 1024
//help functions
void clearInputBuffer();

int main( int argc, char *argv[] )  {
    // Check if the program is executed with exactly 5 arguments
    if( argc == 5) {
        // Declare variables for managing file descriptors and data
        fd_set fdset, rdset;
        FD_ZERO(&fdset); // Initialize fdset to zero
        FD_ZERO(&rdset); // Initialize rdset to zero
        char ch;
        int c;
        // Duplicate and store command line arguments as strings
        char* username = strdup(argv[3]);
        char* message = strdup(argv[4]);
        int msg_len = strlen(message); // Calculate length of the message

        // Declare variables related to socket communication
        int clientSocket, byteSend, numOfRecive = 0;
        char buffer[Buffer_size]; // Buffer to hold data to send/receive
        struct sockaddr_in serverAddr; // Structure to hold server address
        socklen_t addr_size;

        // Initialize buffer to zero
        memset(buffer, 0, sizeof(buffer));

        // Create a socket for communication (IPv4, TCP)
        clientSocket = socket(PF_INET, SOCK_STREAM, 0);
        if(clientSocket < 0){
            perror("socket failed");
            free(username);
            free(message);
            exit(EXIT_FAILURE);
        }

        // Set up server address structure
        serverAddr.sin_family = AF_INET; // IPv4
        serverAddr.sin_port = htons((short)atoi(argv[2])); // Port number
        serverAddr.sin_addr.s_addr = inet_addr(argv[1]); // IP address
        memset(serverAddr.sin_zero, 0, sizeof(serverAddr.sin_zero)); // Fill with zeros
        addr_size = sizeof(serverAddr); // Size of server address structure

        // Connect to the server
        if(connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size) < 0){
            perror("connect failed");
            close(clientSocket);
            free(username);
            free(message);
            exit(EXIT_FAILURE);
        }

        // Connection successful
        printf("Connection succeeded!\n");

        // Prepare and send a message to the server
        sprintf(buffer, "%6s%d%d%s\n", username, 0, 0, message); // Format the message
        buffer[6] = (char)(msg_len % 16); // Store message length (low byte)
        buffer[7] = (char)(msg_len / 16); // Store message length (high byte)

        if((byteSend = send(clientSocket, buffer, Buffer_size, 0)) < 0){
            perror("send failed");
            close(clientSocket);
            free(username);
            free(message);
            exit(EXIT_FAILURE);
        }

        // Monitor the socket for incoming data and user input
        FD_SET(clientSocket, &fdset); // Add clientSocket to fdset
        FD_SET(fileno(stdin), &fdset); // Add stdin (user input) to fdset





        while(1) {
            rdset = fdset; // Copy fdset to rdset for select()

            // Check for activity on file descriptors
            if(select(FD_SETSIZE, &rdset, NULL, NULL, NULL) < 0){
                perror("select failed");
                close(clientSocket);
                free(username);
                free(message);
                exit(EXIT_FAILURE);
            }

            // Check if there is user input from stdin
            if(FD_ISSET(fileno(stdin), &rdset)){
                //printf("Client decided to not  quit\n");
                //close(clientSocket);
                //free(username);
                //free(message);
                //return 0;
                setbuf(stdin, NULL);  // Flush stdin buffer
                
                         // Read input from stdin and handle it
            char input[100];  // Adjust size according to your needs
            if (fgets(input, sizeof(input), stdin)) {
                // Process input here
                int num;
                if (sscanf(input, "%d", &num) == 1) {
                    printf("You entered: %d\n", num);
                    // Send the integer string to the server
		    int succeed = send(clientSocket, input, strlen(input), 0);
		    if (succeed < 0) {
			perror("send failed");
			exit(EXIT_FAILURE);
		    }
                } else {
                    printf("Invalid input\n");
                }
            }
                
                //clearInputBuffer();
                continue;
                
            } else { // Check if socket is ready to receive data
                memset(buffer, 0, Buffer_size); // Clear buffer

                // Receive data from the server
                
                
                if((numOfRecive = recv(clientSocket, buffer, Buffer_size, 0)) > 0) {
                    buffer[numOfRecive] = '\0'; // Null-terminate received data
                    printf("%s\n", buffer); // Print received message
                }

                // Handle server closing the connection
                if(numOfRecive == 0) {
                    printf("Server closed the connection\n");
                    free(username);
                    free(message);
                    return 0;
                }

                // Handle receive errors
                if(numOfRecive < 0) {
                    perror("reading failed");
                    close(clientSocket);
                    free(username);
                    free(message);
                    exit(EXIT_FAILURE);
                }
            }
        }
    } else {
        printf("4 arguments expected.\n"); // Print error message if arguments are incorrect
        return -1;
    }
}

void clearInputBuffer() {
int c;
    while ((c = getchar()) != '\n' && c != EOF);

}
