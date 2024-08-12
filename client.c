#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define BUFFER_SIZE 1024
#define PORT 8083
#define IP "127.0.0.1"
#define MAX_SALES 10

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
    char num_menu[BUFFER_SIZE];  // Changed to BUFFER_SIZE
    multicast_info m_info;
    pthread_t multicast_thread;

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
    snprintf(buffer, BUFFER_SIZE, "%s", pass);

    // Send data to the server
    ssize_t pass_sent = send(sock, buffer, strlen(buffer), 0);
    if (pass_sent < 0) {
        perror("send failed");
        close(sock);
        return -1;
    }

    printf("Connecting to the server... Data sent: user %s, pass %s\n", user, pass);

    // Wait for server's authentication response
    getting_data(sock, buffer);
    printf("Server response: %s\n", buffer);

    // Main loop to handle menu selection
    while (1) {
        getting_data(sock, buffer);
        printf("%s", buffer);

        printf("Choose a sale number: ");
        scanf("%49s", num_menu);  // Changed to BUFFER_SIZE

        // Send the chosen menu number to the server
        sending_data(sock, num_menu);

        // Receive multicast IP and port from the server
        getting_data(sock, buffer);
        printf("Multicast IP and port received from the server: %s\n", buffer);

        // Extract multicast IP and port from the received data
        sscanf(buffer, "Multicast IP: %49s\nPort: %d", m_info.multicast_ip, &m_info.multicast_port);

        // Create a thread to receive multicast messages
        if (pthread_create(&multicast_thread, NULL, receive_multicast, &m_info) != 0) {
            perror("Failed to create multicast receiver thread");
        }
        pthread_detach(multicast_thread);  // Automatically reclaim resources when the thread terminates
        break;  // Exit after setting up the multicast listener
    }

    // The main loop keeps the connection open
    while (1) {
        // Implement additional logic here, such as receiving specific commands or sending more data
        // For now, just keep the connection open
        sleep(1);
    }

    // The connection will only close when the user decides to exit the loop
    close(sock);
    printf("Connection closed.\n");

    return 0;
}

void getting_data(int sock, char buffer[BUFFER_SIZE]) {
    int data_received = recv(sock, buffer, BUFFER_SIZE, 0);
    if (data_received > 0) {
        buffer[data_received] = '\0';
    } else {
        printf("Failed to receive data.\n");
    }
}

void sending_data(int sock, char buffer[BUFFER_SIZE]) {
    if (send(sock, buffer, strlen(buffer), 0) < 0) {
        perror("Failed to send data");
    }
}

void *receive_multicast(void *arg) {
    multicast_info *m_info = (multicast_info *)arg;
    int sockfd;
    struct sockaddr_in local_addr;
    struct ip_mreq mreq;
    char buffer[BUFFER_SIZE];

    // Create a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        pthread_exit(NULL);
    }

    // Allow multiple sockets to use the same port number
    unsigned int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        perror("Reusing ADDR failed");
        close(sockfd);
        pthread_exit(NULL);
    }

    // Set up the local address structure
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(m_info->multicast_port);  // Use the dynamic port

    // Bind the socket to the local address and port
    if (bind(sockfd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        perror("bind failed");
        close(sockfd);
        pthread_exit(NULL);
    }

    // Join the multicast group
    mreq.imr_multiaddr.s_addr = inet_addr(m_info->multicast_ip);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("setsockopt failed");
        close(sockfd);
        pthread_exit(NULL);
    }

    // Receive messages from the multicast group
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

