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

// Prototype declarations
void getting_data(int sock, char buffer4[BUFFER_SIZE]);
void sending_data(int sock, char buffer4[BUFFER_SIZE]);
void *receive_multicast(void *arg);

typedef struct {
    char multicast_ip[50];
    int multicast_port;
} multicast_info;

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    char buffer2[BUFFER_SIZE] = {0};
    char buffer3[BUFFER_SIZE] = {0};    
    char buffer4[BUFFER_SIZE] = {0};
    char num_menu[BUFFER_SIZE];  // Changed to BUFFER_SIZE

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

    // Send username
    char user[50];  // Assuming usernames can be up to 49 characters long
    char pass[50];  // Assuming passwords can be up to 49 characters long

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

    // Wait for the server's response after sending the password
    getting_data(sock, buffer4);

    // Check the server's response
    if (strstr(buffer4, "wrong password") != NULL) {
        printf("Server response: %s\n", buffer4);
        close(sock);
        return 0;  // Exit the program if the password is wrong
    } else if (strstr(buffer4, "Authentication successful") != NULL) {
        printf("Server response: %s\n", buffer4);
    }

    while (1) {            // LOOP TO MENU STAGE
        getting_data(sock, buffer4);
        // Clear buffer to ensure no garbage data is sent
        int buffer_numb = atoi(buffer4);
        memset(buffer4, 0, BUFFER_SIZE);
        if (buffer_numb == 371) {
            printf("the server accept your request to join the sale\n");
            break;
        } else if (buffer_numb == -1) {
            printf("the server accept you request to leave\n");
            break;
        }

        printf("Choose menu number ");
        scanf("%49s", num_menu);  // Changed to BUFFER_SIZE

        // Clear buffer to ensure no garbage data is sent
        memset(buffer4, 0, BUFFER_SIZE);

        // Format data to send to the server
        snprintf(buffer4, BUFFER_SIZE, "%s", num_menu); 
        // Send data to the server
        sending_data(sock, num_menu);

        // Get multicast IP from the server
        getting_data(sock, buffer4);
        printf("Multicast IP received from the server: %s\n", buffer4);

        // Extract multicast IP from the received data
        char multicast_ip[50];
        sscanf(buffer4, "Multicast IP: %49s", multicast_ip);

        // Create a thread to receive multicast messages
        multicast_info m_info;
        strncpy(m_info.multicast_ip, multicast_ip, 50);
        m_info.multicast_port = 12345;

        pthread_t multicast_thread;
        if (pthread_create(&multicast_thread, NULL, receive_multicast, &m_info) != 0) {
            perror("Failed to create multicast receiver thread");
        }
    }

    while (1) {            // LOOP FOR SALE
        // sending_data(sock, num_menu);        
        // getting_data(sock, buffer4);
    }

    // Close the socket
    close(sock);
    printf("Connection closed.\n");

    return 0;
}

void getting_data(int sock, char buffer4[BUFFER_SIZE]) {
    int data_recieved = recv(sock, buffer4, BUFFER_SIZE, 0);    // getting menu numb
    if (data_recieved > 0) 
        printf("this data delivered from the server:\n%s\n", buffer4);
    else
        printf("Bad getting data\n");
}

void sending_data(int sock, char buffer4[BUFFER_SIZE]) {
    int send_me = send(sock, buffer4, strlen(buffer4), 0);
    if (send_me < 0) 
        perror("send failed");
    printf("sss");
}

void *receive_multicast(void *arg) {
    multicast_info *m_info = (multicast_info *)arg;
    int sockfd;
    struct sockaddr_in multicast_addr;
    struct ip_mreq mreq;
    char buffer[BUFFER_SIZE];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        pthread_exit(NULL);
    }

    memset(&multicast_addr, 0, sizeof(multicast_addr));
    multicast_addr.sin_family = AF_INET;
    multicast_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    multicast_addr.sin_port = htons(m_info->multicast_port);

    if (bind(sockfd, (struct sockaddr *)&multicast_addr, sizeof(multicast_addr)) < 0) {
        perror("bind failed");
        close(sockfd);
        pthread_exit(NULL);
    }

    mreq.imr_multiaddr.s_addr = inet_addr(m_info->multicast_ip);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("setsockopt failed");
        close(sockfd);
        pthread_exit(NULL);
    }

    while (1) {
        int nbytes = recv(sockfd, buffer, BUFFER_SIZE, 0);
        if (nbytes < 0) {
            perror("recv failed");
            close(sockfd);
            pthread_exit(NULL);
        }
        buffer[nbytes] = '\0';
        printf("Received multicast message: %s\n", buffer);
    }

    close(sockfd);
    pthread_exit(NULL);
}

