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

//======================
// Function Declarations
//======================
void getting_data(int sock, char buffer[BUFFER_SIZE]);
void sending_data(int sock, const char *buffer);
void *receive_multicast(void *arg);
void handle_bidding(int sock);

//======================
// Struct Definitions
//======================
typedef struct {
    char multicast_ip[50];
    int multicast_port;
    int server_sock;
} multicast_info;

//==================
// Main Function
//==================
int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    char num_menu[BUFFER_SIZE];
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
    char user[50];
    char pass[50];

    printf("Enter username: ");
    scanf("%49s", user);

    // Clear buffer and send username
    memset(buffer, 0, BUFFER_SIZE);
    snprintf(buffer, BUFFER_SIZE, "%s\n", user);
    sending_data(sock, buffer);

    // Send password
    printf("Enter password: ");
    scanf("%49s", pass);

    memset(buffer, 0, BUFFER_SIZE);
    snprintf(buffer, BUFFER_SIZE, "%s", pass);
    sending_data(sock, buffer);

    printf("Connecting to the server... Data sent: user %s, pass %s\n", user, pass);

    // Wait for server's authentication response
    getting_data(sock, buffer);
    printf("Server response: %s\n", buffer);

    if (strstr(buffer, "Wrong password") != NULL) {
        close(sock);
        printf("Wrong password. Connection closed.\n");
        return 0;
    }

    // Main loop to handle menu selection
    while (1) {
        getting_data(sock, buffer);
        printf("%s", buffer);

        printf("Choose a sale number: ");
        scanf("%49s", num_menu);

        // Send the chosen menu number to the server
        sending_data(sock, num_menu);

        // Receive multicast IP from the server
        getting_data(sock, buffer);
        printf("Multicast IP received from the server: %s\n", buffer);

        // Extract multicast IP and port from the received data
        sscanf(buffer, "Multicast IP: %49s\nPort: %d\n", m_info.multicast_ip, &m_info.multicast_port);
        m_info.server_sock = sock;

        // Create a thread to receive multicast messages
        if (pthread_create(&multicast_thread, NULL, receive_multicast, &m_info) != 0) {
            perror("Failed to create multicast receiver thread");
        }
        pthread_detach(multicast_thread);  // Automatically reclaim resources when the thread terminates
        break;  // Exit after setting up the multicast listener
    }

    // Wait for the sale to start and bid
    while (1) {
        sleep(1);
    }

    // The connection will only close when the user decides to exit the loop
    close(sock);
    printf("Connection closed.\n");

    return 0;
}

//========================
// Data Handling Functions
//========================
void getting_data(int sock, char buffer[BUFFER_SIZE]) {
    int data_received = recv(sock, buffer, BUFFER_SIZE, 0);
    if (data_received > 0) {
        buffer[data_received] = '\0';
    } else {
        printf("Failed to receive data.\n");
    }
}

void sending_data(int sock, const char *buffer) {
    if (send(sock, buffer, strlen(buffer), 0) < 0) {
        perror("Failed to send data");
    }
}

//===============================
// Multicast and Bidding Functions
//===============================
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
    local_addr.sin_port = htons(m_info->multicast_port);

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
        printf("%s\n", buffer);

        // If the server announces a bid request
        if (strstr(buffer, "Place your bids") != NULL) {
            handle_bidding(m_info->server_sock);  // Handle the bidding process
        }

        // If the server announces a winner
        if (strstr(buffer, "The winner is") != NULL) {
            printf("%s\n", buffer);
        }
    }

    close(sockfd);
    pthread_exit(NULL);
}

void handle_bidding(int sock) {
    char bid[BUFFER_SIZE];

    printf("Enter your bid: ");
    scanf("%s", bid);

    // Send the bid to the server
    sending_data(sock, bid);

    printf("Bid sent: %s\n", bid);
}

