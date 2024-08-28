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

#define header_size 72


//========================
// Data Structures
//========================
typedef struct {
    char multicast_ip[50];
    int multicast_port;
    int server_sock;
    int client_id;  // New field to store client ID
} multicast_info;

//========================
// Function Declarations
//========================
void getting_data(int sock, char *data, int *type, int *length, int *client_id, int *sale_id, int *item_id);
void sending_data(int sock, const char *data, int type, int client_id, int sale_id, int item_id);
void *receive_multicast(void *arg);
void handle_bidding(int sock, int client_id);

//========================
// Main Function
//========================
int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char *buffer = malloc(BUFFER_SIZE);
    char *num_menu = malloc(BUFFER_SIZE);
    multicast_info m_info;
    pthread_t multicast_thread;

    int version, type, length, reserved, client_id, sale_id, item_id;

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
    char *user = malloc(50);
    char *pass = malloc(50);

    printf("Enter username: ");
    scanf("%49s", user);

    // Clear buffer to ensure no garbage data is sent
    memset(buffer, 0, BUFFER_SIZE);

    // Format data to send to the server
    snprintf(buffer, BUFFER_SIZE, "%s\n", user);

    // Send data to the server
    sending_data(sock, buffer, 3, 0, 0,0);

    printf("Enter password: ");
    scanf("%49s", pass);

    // Clear buffer to ensure no garbage data is sent
    memset(buffer, 0, BUFFER_SIZE);

    // Format data to send to the server
    snprintf(buffer, BUFFER_SIZE, "%s", pass);

    // Send data to the server
    sending_data(sock, buffer, 3, 0, 0,0);

    printf("Connecting to the server... Data sent: user %s, pass %s\n", user, pass);

    // Wait for server's authentication response
    getting_data(sock, buffer, &type, &length, &client_id, &sale_id, &item_id);
    printf("Server response: %s\n", buffer);

    if (strstr(buffer, "Wrong password") != NULL) {
        close(sock);
        printf("Wrong password. Connection closed.\n");
        return 0;
    }

    // Main loop to handle menu selection
    while (1) {
        getting_data(sock, buffer, &type, &length, &client_id, &sale_id, &item_id);
        printf("%s", buffer);

        printf("Choose a sale number: ");
        scanf("%49s", num_menu);
        
        // Clear buffer to ensure no garbage data is sent
    memset(buffer, 0, BUFFER_SIZE);

    // Format data to send to the server
    snprintf(buffer, BUFFER_SIZE, "%s", num_menu);

	sending_data(sock, buffer, 4, 0, 0,0);//sending numer from the menu
	
        // Receive multicast IP from the server
        getting_data(sock, buffer, &type, &length, &client_id, &sale_id, &item_id);
        printf("Multicast IP received from the server: %s\n", buffer);

        // Extract multicast IP from the received data
        sscanf(buffer, "Multicast IP: %49s\nPort: %d\n", m_info.multicast_ip, &m_info.multicast_port);
        m_info.server_sock = sock;
        m_info.client_id = client_id;

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

    free(buffer);
    free(num_menu);
    free(user);
    free(pass);

    return 0;
}

//========================
// Data Handling Functions
//========================
void getting_data(int sock, char *data, int *type, int *length, int *client_id, int *sale_id, int *item_id) {
    char buffer[BUFFER_SIZE];
    int data_received = recv(sock, buffer, BUFFER_SIZE, 0);
    if (data_received > 0) {
        buffer[data_received] = '\0';

        // Parse SBP Header with the correct format for each field
        sscanf(buffer, "%4d%16d%16d%8d%16d", type, length, client_id, sale_id, item_id);


        // Extract data without header
        strcpy(data, buffer + header_size);  // Extract the actual data part after the header
        printf("%s",data);

    } else {
        printf("Failed to receive data.\n");
    }
}

void sending_data(int sock, const char *data, int type, int client_id, int sale_id, int item_id) {
    // Convert the input data to ASCII hexadecimal representation
    int original_length = strlen(data);
    char *ascii_data = malloc(original_length * 2 + 1); // Each character becomes 2 hex digits, +1 for null terminator
    if (!ascii_data) {
        perror("Failed to allocate memory for ASCII data");
        return;
    }

    // Convert each character to its ASCII hex representation
    for (int i = 0; i < original_length; i++) {
        snprintf(&ascii_data[i * 2], 3, "%02X", (unsigned char)data[i]);
    }

    // Calculate the length of the ASCII data
    int length = strlen(ascii_data);

    // Allocate memory for the SBP header (11 bytes for 88 bits)
    unsigned char header[11] = {0}; // Initialize all bytes to 0

    // Set the Type (4 bits) in the first nibble of the first byte (bits 0-3)
    header[0] = (type & 0x0F);  // Mask the type to 4 bits (0x0F)

    // Set the Length (16 bits) in bytes 1-2 (bits 8-23)
    uint16_t length_net = htons(length); // Convert to network byte order
    memcpy(&header[1], &length_net, 2); // Copy length to the correct position in the header

    // Set the Client ID (16 bits) in bytes 3-4 (bits 24-39)
    uint16_t client_id_net = htons(client_id); // Convert to network byte order
    memcpy(&header[3], &client_id_net, 2); // Copy client ID to the correct position in the header

    // Set the Sale ID (8 bits) in byte 5 (bits 40-47)
    header[5] = (unsigned char)(sale_id & 0xFF); // Mask the sale ID to 8 bits

    // Set the Item ID (16 bits) in bytes 6-7 (bits 48-63)
    uint16_t item_id_net = htons(item_id); // Convert to network byte order
    memcpy(&header[6], &item_id_net, 2); // Copy item ID to the correct position in the header

    // Combine header and ASCII data into a single message buffer
    size_t total_size = sizeof(header) + length; // Total size of header + ASCII data
    char *sbp_message = malloc(total_size);
    if (!sbp_message) {
        perror("Failed to allocate memory for SBP message");
        free(ascii_data);  // Free the allocated memory for ASCII data
        return;
    }

    // Copy header and ASCII data into the combined message buffer
    memcpy(sbp_message, header, sizeof(header));
    memcpy(sbp_message + sizeof(header), ascii_data, length);

    // Print the constructed header and message for debugging purposes
    printf("Header: ");
    for (int i = 0; i < sizeof(header); i++) {
        printf("%02X ", header[i]); // Print each byte as a hexadecimal value
    }
    printf("\nASCII Data: %s\nSBP Message: ", ascii_data);
    for (int i = 0; i < total_size; i++) {
        printf("%02X ", (unsigned char)sbp_message[i]); // Print each byte of the entire message
    }
    printf("\n");

    // Send the SBP message to the server
    if (send(sock, sbp_message, total_size, 0) < 0) {
        perror("Failed to send data");
    }

    // Free the allocated memory
    free(sbp_message);
    free(ascii_data);
}
//========================
// Multicast Handling Functions
//========================
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
        if (strstr(buffer, "BID_REQUEST") != NULL) {
            handle_bidding(m_info->server_sock, m_info->client_id);  // Handle the bidding process
        }

        // If the server announces a winner
        if (strstr(buffer, "WINNER") != NULL) {
            printf("%s\n", buffer);
        }
    }

    close(sockfd);
    pthread_exit(NULL);
}

//========================
// Bidding Function
//========================
void handle_bidding(int sock, int client_id) {
    char *buffer = malloc(BUFFER_SIZE);
    char *bid = malloc(BUFFER_SIZE);

    printf("Enter your bid: ");
    scanf("%s", bid);

    int version = 1;    // SBP version
    int type = 2;       // Message type (e.g., 2 for bidding)
    int length = strlen(bid); // Length of the bid data
    int reserved = 0;   // Reserved field, set to 0
    int sale_id = 1;    // Assuming sale ID is 1 for simplicity
    int item_id = 1;    // Assuming item ID is 1 for simplicity

    // Format the SBP-compliant message header
    snprintf(buffer, BUFFER_SIZE, "%d%d%d%d%d%d%d%s", 
             version, type, length, reserved, client_id, sale_id, item_id, bid);

    // Send the SBP-compliant message
    sending_data(sock, buffer, type, client_id, sale_id, item_id);

    printf("Bid sent: %s\n", buffer);

    free(buffer);
    free(bid);
}

