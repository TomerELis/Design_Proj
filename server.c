#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

#define Buffer_size 1024
#define maxLen 256
#define password "Kofiko"
#define MAX_SALES 10

struct user {
    int username;
    int socket;
    int saleid;  // Assuming titles can be up to 50 characters long
    int switchstate;
};

struct Sale {
    int id;
    char title[50];  // Assuming titles can be up to 50 characters long
    char multicast_ip[50];
    int num_of_clients;
};



//prototype
int createWelcomeSocket(short port, int maxClient);
int exitAll(int maxOpen, int server, int* client, char** user, struct sockaddr_in* addr,char* data);

// Function to send menu to client
void sendMenu(int clientSocket);
int num_of_sales = 4;

int main( int argc, char *argv[] )  {
    // Check if the correct number of arguments (port and max clients) are provided
    if( argc == 3 ) {
        int welcomeSocket,len, maxClient=(int)atoi(argv[2]),i=0,j=0, numOfByte=0,numOfconnect=0,index=0;
        int *clientSocket, fullFlag=0;
        short portNum=(short)atoi(argv[1]);
        char buffer[Buffer_size];
        char **userName,*data=NULL;
        struct sockaddr_in *clientAddr;
        struct sockaddr_in welcomeAddr;
        fd_set current_socket, ready_socket;
        socklen_t welcome_size, client_size;

        // Initialize memory for structs and arrays
        memset(welcomeAddr.sin_zero, 0, sizeof welcomeAddr.sin_zero);

        userName=(char**)malloc(maxClient*sizeof(char *));
        clientAddr=(struct sockaddr_in*)malloc((maxClient * (sizeof(struct sockaddr_in))));
        clientSocket= (int*)malloc(maxClient*(sizeof(int)));
        data=(char*)malloc((maxLen+1)* sizeof(char));

        // Initialize userName array with allocated memory
        for(i=0;i<maxClient;i++){
            userName[i]=(char*) malloc(7* sizeof(char));
            if(userName[i]==NULL){
                for(j=0;j<i;j++){
                    free(userName[j]);
                }
            }
        }
        // Initialize clientSocket array with zeros
        for(i=0;i<maxClient;i++){
            clientSocket[i]=0;
        }

        // Check if memory allocation was successful
        if(clientAddr==NULL||clientSocket==NULL||userName==NULL||data==NULL){
            perror("malloc failed");
            exit(EXIT_FAILURE);
        }

        // Create welcome socket and handle errors
        welcomeSocket= createWelcomeSocket(portNum, maxClient);
        if(welcomeSocket<0){
            exitAll(maxClient,welcomeSocket,clientSocket,userName,clientAddr,data);
        }

        // Initialize file descriptor sets for socket and stdin
        FD_ZERO(&current_socket);
        FD_SET(welcomeSocket,&current_socket);
        FD_SET(fileno(stdin),&current_socket);

        // Main server loop
        while(1) {
            ready_socket= current_socket;
            // Use select to wait for activity on sockets
            if(select(FD_SETSIZE, &ready_socket,NULL,NULL,NULL)<0){
                perror("select failed");
                exitAll(maxClient,welcomeSocket,clientSocket,userName,clientAddr,data);
                exit(EXIT_FAILURE);
            }
            // Check if stdin (console input) is set, indicating server shutdown
            if(FD_ISSET(fileno(stdin),&ready_socket)){
                printf("Bye Bye...\n");
                exitAll(maxClient,welcomeSocket,clientSocket,userName,clientAddr,data);
                return 0;
            }
            // Iterate through all sockets in the set
            for(i=0;i<FD_SETSIZE;i++){
                if(FD_ISSET(i,&ready_socket)){
                    // Handle new connections to the welcome socket
                    if(i == welcomeSocket){
                        // Check if maximum connections reached
                        if((numOfconnect==maxClient)&&!fullFlag){
                            close(welcomeSocket);
                            FD_CLR(welcomeSocket,&current_socket);
                            fullFlag=1;
                        }
                        // Accept new connection if space available
                        if(!fullFlag){
                            index=0;
                            while(index<maxClient&&clientSocket[index]!=0) ++index;
                            client_size= sizeof(clientAddr[index]);
                            memset(&clientAddr[index],0,client_size);
                            if((clientSocket[index]= accept(welcomeSocket,(struct sockaddr*)&clientAddr[index],&client_size))<0){
                                perror("accept failed");
                                exitAll(maxClient,welcomeSocket,clientSocket,userName,clientAddr,data);
                                exit(EXIT_FAILURE);
                            }
                            FD_SET((clientSocket[index]),&current_socket);
                            numOfconnect++;
                        }
                    } else {
                        // Handle messages from connected clients
                        if((numOfByte= recv(i,buffer,Buffer_size,0))<0){
                            perror("reading failed");
                            exitAll(maxClient,welcomeSocket,clientSocket,userName,clientAddr,data);
                            exit(EXIT_FAILURE);
                        } else if(numOfByte==0){
                            // Client disconnected
                            index=0;
                            while (index<numOfconnect&&clientSocket[index]!=i) ++index;
                            printf("%s has left the server...\n",userName[index]);
                            close(clientSocket[index]);
                            clientSocket[index]=0;
                            numOfconnect--;
                            FD_CLR(i,&current_socket);
                            // Reopen welcome socket if necessary
                            if((numOfconnect==(maxClient-1))&&fullFlag){
                                welcomeSocket= createWelcomeSocket(portNum,maxClient);
                                if(welcomeSocket<1){
                                    exitAll(maxClient,welcomeSocket,clientSocket,userName,clientAddr,data);
                                    exit(EXIT_FAILURE);
                                }
                                FD_SET(welcomeSocket,&current_socket);
                                fullFlag=0;
                            }
                        } else {
                            // Handle initial message from client
                            index=0;
                            while (index<numOfconnect&&clientSocket[index]!=i) ++index;
                            sprintf(userName[index],"%6s",buffer);
                            userName[index][6]='\0';
                            len=(16*buffer[7]+buffer[6]);
                            memset(data,0,maxLen);
                            for(j=0;j<len;j++){
                                data[j]=buffer[8+j];
                            }
                            data[len]='\0';
                            printf("%s has connected to the server.\n",userName[index]);
                            memset(buffer,0,Buffer_size);

                            sprintf(buffer,"User: %s has said: %s \n",userName[index],data);
                            printf("%s has connected to the server.\n",buffer);
                            
                            //----------------------------------------------------------------- 
                            //add by bar - check if the password correct (please do use more then 7 usernames)
                            if (strcmp(data, password) == 0) {
   				 // Password matches
    				printf("Client provided correct password: %s\n", password);
    				
    				
        			sendMenu(clientSocket[index]);
		
    				
    	                        // Broadcast message to other clients
    	                        }
    	                    else {
   				 // Password does not match
    				printf("Client provided incorrect password: %s\n", data);			//TODO: add timer and close socket
   				 // Close the connection or handle authentication failure
				}					
    	                       //----------------------------------------------------------------- 
                            for(j=0;j<maxClient;j++){
                                if((clientSocket[j]!=i)&&(clientSocket[j]!=0)){
                                    if(send(clientSocket[j],buffer,Buffer_size,0)<0){
                                        perror("send failed");
                                        exitAll(maxClient,welcomeSocket,clientSocket,userName,clientAddr,data);
                                        exit(EXIT_FAILURE);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

    } else {
        printf("2 argument expected.\n");
        return -1;
    }
}

// Function to clean up and exit server
int exitAll(int maxOpen, int server, int* client, char** user, struct sockaddr_in* addr,char* data){
    int k=0;
    for(k=0;k<maxOpen;k++){
        if(client[k]!=0){
            close(client[k]);
        }
        free(user[k]);
    }
    close(server);
    free(client);
    free(addr);
    free(user);
    free(data);
    return 0;
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
