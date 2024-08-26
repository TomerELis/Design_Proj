#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#define BUFFER_SIZE 1024
#define PORT 8083
#define MAX_CLIENTS 10
#define secret "Kofiko"
#define MAX_SALES 10
#define MAX_ITEMS 5  // Maximum number of items per sale

typedef struct {
    char name[50];
    int id;
    char type[50];
    double start_price;
} Item;

typedef struct {
    int id;
    char title[50];
    char multicast_ip[50];
    char data[50];
    int num_of_clients;
    int is_started;
    time_t star_time;
    Item items[MAX_ITEMS];
    int num_of_items;
    int multicast_port;
    double money_capacity;  // New field for money capacity per sale
} Sale;

typedef struct {
    int socket;
    struct sockaddr_in address;
    int client_id;
    char user_name[BUFFER_SIZE];
    char comments[BUFFER_SIZE];
    int selected_sale;
    double money;  // Amount of money the client has for the current sale
} Client;

// Function declarations
void sendMenu(int clientSocket);
void sending_data(int clientSocket, const char *data);
void getting_data(int clientSocket, char data[BUFFER_SIZE]);
int createWelcomeSocket(short port, int maxClient);
void send_multicast_message(const char* multicast_ip, int multicast_port, const char* message);
void *handle_client(void *client_ptr);
void *command_handler(void *arg);
void generate_items_for_sale(Sale *sale);
void print_items_table(Item *items, int num_items);
int check_sale(Sale my_sale);
void start_sale(Sale *sale);

int num_of_sales = 4;
Client *clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;
time_t current_time, start_time, real_time;
int remaining_time;
int serverSocket;
Sale sales[MAX_SALES] = {
    {1, "Summer Sale", "224.2.1.1", "Summer sale data", 0, 0, 220, {{0}}, 0, 12345, 150.0},
    {2, "Back to School Sale", "224.2.2.1", "Back to school data", 0, 0, 60, {{0}}, 0, 12346, 200.0},
    {3, "Holiday Sale", "224.2.3.1", "Holiday sale data", 0, 0, 215, {{0}}, 0, 12347, 300.0},
    {4, "End of Year Clearance", "224.2.4.1", "End of year data", 0, 0, 120, {{0}}, 0, 12348, 250.0},
    {-1, "Exit", "0.0.0.0", "Exit data", 0, 0, 0, {{0}}, 0, 0, 0.0}
};

// Function definitions
void generate_items_for_sale(Sale *sale) {
    if (sale->id == 1) {
        strcpy(sale->items[0].name, "Beach Towel");
        strcpy(sale->items[1].name, "Sunscreen");
        strcpy(sale->items[2].name, "Sunglasses");
        strcpy(sale->items[3].name, "Flip Flops");
        strcpy(sale->items[4].name, "Swimwear");
    } else if (sale->id == 2) {
        strcpy(sale->items[0].name, "Backpack");
        strcpy(sale->items[1].name, "Notebooks");
        strcpy(sale->items[2].name, "Pens");
        strcpy(sale->items[3].name, "Lunchbox");
        strcpy(sale->items[4].name, "Water Bottle");
    } else if (sale->id == 3) {
        strcpy(sale->items[0].name, "Christmas Tree");
        strcpy(sale->items[1].name, "Holiday Lights");
        strcpy(sale->items[2].name, "Gift Wrap");
        strcpy(sale->items[3].name, "Ornaments");
        strcpy(sale->items[4].name, "Stocking");
    } else if (sale->id == 4) {
        strcpy(sale->items[0].name, "Winter Coat");
        strcpy(sale->items[1].name, "Scarf");
        strcpy(sale->items[2].name, "Gloves");
        strcpy(sale->items[3].name, "Boots");
        strcpy(sale->items[4].name, "Sweater");
    }

    for (int i = 0; i < MAX_ITEMS; i++) {
        sale->items[i].id = i + 1;
        snprintf(sale->items[i].type, sizeof(sale->items[i].type), "Type %d", i % 3 + 1);
        sale->items[i].start_price = (i + 1) * 10.0;
    }
    sale->num_of_items = MAX_ITEMS;
}

void print_items_table(Item *items, int num_items) {
    printf("+----+-----------------+-------+-------------+\n");
    printf("| ID | Name            | Type  | Start Price |\n");
    printf("+----+-----------------+-------+-------------+\n");
    for (int i = 0; i < num_items; i++) {
        printf("| %-2d | %-15s | %-5s | $%-10.2f |\n", 
               items[i].id, items[i].name, items[i].type, items[i].start_price);
    }
    printf("+----+-----------------+-------+-------------+\n");
}

void start_sale(Sale *sale) {
    char message[BUFFER_SIZE];

    snprintf(message, BUFFER_SIZE, "The sale started!");
    send_multicast_message(sale->multicast_ip, sale->multicast_port, message);

    snprintf(message, BUFFER_SIZE, "The rule is simple: each one of you has %.2f$.\nThere are %d items in this sale. Each item has a starting price.\nSo think well before making offers!\n\nAh! There’s one more important rule: THE SPICE RULE: The second highest price wins!\n\nGood luck!", sale->money_capacity, sale->num_of_items);
    send_multicast_message(sale->multicast_ip, sale->multicast_port, message);

    // Assign money to each client in this sale (no need to send multicast message repeatedly)
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL && clients[i]->selected_sale == sale->id) {
            clients[i]->money = sale->money_capacity;  // Set client's money to the sale's capacity
        }
    }
    
    // Send money capacity message once
    snprintf(message, BUFFER_SIZE, "You have been given %.2f$ for this sale.\n", sale->money_capacity);
    send_multicast_message(sale->multicast_ip, sale->multicast_port, message);

    // Simulate a 30-second countdown
    sleep(30);

    // Clear the terminal screen for all clients in this multicast group
    snprintf(message, BUFFER_SIZE, "\033[2J\033[H");  // ANSI escape code to clear the screen
    send_multicast_message(sale->multicast_ip, sale->multicast_port, message);

    // Send the first item to the clients
    snprintf(message, BUFFER_SIZE, "Item 1: %s (Starting price: $%.2f)", sale->items[0].name, sale->items[0].start_price);
    send_multicast_message(sale->multicast_ip, sale->multicast_port, message);
}

void *handle_client(void *client_ptr) {
    Client *client = (Client *)client_ptr;
    char buffer[BUFFER_SIZE] = {0};
    ssize_t bytes_received, bytes_received_sale;
    int flg = 0;  // Flag to track whether we're expecting a password or username

    while (1) {
        printf("Handling client %d\n", client->client_id);
        bytes_received = recv(client->socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';  // Null-terminate the received data
            if (flg == 1) {  // If flag is 1, we are expecting a password
                printf("Password received from client %d: %s\n", client->client_id, buffer);
                flg = 0;  // Reset flag
                int result = strcmp(secret, buffer);
                if (result == 0) { // Password is correct
                    printf("Client %d entered the correct password.\n", client->client_id);
                    sending_data(client->socket, "Authentication successful. Please choose a menu number.\n");
                    sleep(2);
                    while (1) {  // Now we expect the client to choose a sale number
                        sendMenu(client->socket);
                        memset(buffer, 0, BUFFER_SIZE);
                        bytes_received_sale = recv(client->socket, buffer, BUFFER_SIZE, 0);
                        int numb_menu = atoi(buffer);
                        if (numb_menu > num_of_sales || numb_menu <= 0) {
                            printf("Invalid menu option from client %d: chose %d (only %d options available).\n", client->client_id, numb_menu, num_of_sales);
                            sending_data(client->socket, "Invalid menu option. Please try again.\n");
                        } else {
                            printf("Client %d chose menu option %s\n", client->client_id, buffer);

                            // Track the number of clients in this sale
                            pthread_mutex_lock(&client_mutex);
                            sales[numb_menu - 1].num_of_clients++;
                            client->selected_sale = numb_menu;  // Track the sale selected by the client
                            pthread_mutex_unlock(&client_mutex);

                            // Send the correct multicast IP to the client
                            char dat[BUFFER_SIZE];
                            snprintf(dat, BUFFER_SIZE, "Multicast IP: %s\nPort: %d\n", sales[client->selected_sale - 1].multicast_ip, sales[client->selected_sale - 1].multicast_port);
                            sending_data(client->socket, dat);
                            break;  // Exit after sending the multicast IP to the client
                        }
                    }
                } else { // Password is incorrect
                    printf("Client %d entered an incorrect password.\n", client->client_id);
                    sending_data(client->socket, "Wrong password. I am calling the police! WEEWOOWEEOO\n");
                    close(client->socket);
                    
                    // Clean up client state
                    pthread_mutex_lock(&client_mutex);
                    if (client->selected_sale > 0 && client->selected_sale <= num_of_sales) {
                        sales[client->selected_sale - 1].num_of_clients--;
                    }
                    clients[client->client_id] = NULL;
                    pthread_mutex_unlock(&client_mutex);
                    
                    free(client);  // Free client memory
                    pthread_exit(NULL);
                }
            } else if (flg == 0) {  // If flag is 0, we are expecting a username
                printf("Username received from client %d: %s\n", client->client_id, buffer);
                strncpy(client->user_name, buffer, BUFFER_SIZE);
                flg = 1;  // Set flag to 1 to expect a password next
            }
        } else if (bytes_received == 0) {
            printf("Client %d disconnected.\n", client->client_id);
            
            // Clean up client state
            pthread_mutex_lock(&client_mutex);
            if (client->selected_sale > 0 && client->selected_sale <= num_of_sales) {
                sales[client->selected_sale - 1].num_of_clients--;
            }
            clients[client->client_id] = NULL;
            pthread_mutex_unlock(&client_mutex);

            close(client->socket);
            free(client);  // Free client memory
            pthread_exit(NULL);
        } else {
            perror("Receive failed");
            
            // Clean up client state
            pthread_mutex_lock(&client_mutex);
            if (client->selected_sale > 0 && client->selected_sale <= num_of_sales) {
                sales[client->selected_sale - 1].num_of_clients--;
            }
            clients[client->client_id] = NULL;
            pthread_mutex_unlock(&client_mutex);

            close(client->socket);
            free(client);  // Free client memory
            pthread_exit(NULL);
        }
    }
}


void *command_handler(void *arg) {
    char command_buffer[BUFFER_SIZE];
    while (1) {
        printf("Enter command: ");
        if (fgets(command_buffer, BUFFER_SIZE, stdin) != NULL) {
            command_buffer[strcspn(command_buffer, "\n")] = '\0'; // Remove newline character

            // Check if the command starts with "/"
            if (command_buffer[0] == '/') {
                // Split the command into parts for easier handling
                char *command = strtok(command_buffer, " ");
                char *sub_command = strtok(NULL, " ");
                char *sale_number_str = strtok(NULL, " ");
                int sale_number = 0;

                if (sale_number_str) {
                    sale_number = atoi(sale_number_str);  // Convert sale number from string to integer
                }

                // Switch-case based on the command
                switch (command[1]) { // Check the first character after '/'
                    case 's': // Handle /start sale, /resume sale, /secret_command, /show items
                        if (strcmp(command, "/start") == 0 && sub_command && strcmp(sub_command, "sale") == 0) {
                            if (sale_number > 0 && sale_number <= num_of_sales) {
                                pthread_mutex_lock(&client_mutex);
                                int num_clients = sales[sale_number - 1].num_of_clients;
                                pthread_mutex_unlock(&client_mutex);

                                if (num_clients >= 2) {
                                    pthread_mutex_lock(&client_mutex);
                                    if (sales[sale_number - 1].is_started == 0) {
                                        sales[sale_number - 1].is_started = 1;  // Mark the sale as started
                                        pthread_mutex_unlock(&client_mutex);

                                        printf("Starting sale %d: %s\n", sale_number, sales[sale_number - 1].title);
                                        start_sale(&sales[sale_number - 1]);
                                    } else {
                                        pthread_mutex_unlock(&client_mutex);
                                        printf("Sale %d: %s has already started.\n", sale_number, sales[sale_number - 1].title);
                                    }
                                } else {
                                    printf("Cannot start sale %d: %s. Not enough clients (need at least 2, currently %d).\n", sale_number, sales[sale_number - 1].title, num_clients);
                                }
                            } else {
                                printf("Invalid sale number: %s\n", sale_number_str);
                            }
                        } else if (strcmp(command, "/resume") == 0 && sub_command && strcmp(sub_command, "sale") == 0) {
                            if (sale_number > 0 && sale_number <= num_of_sales) {
                                pthread_mutex_lock(&client_mutex);
                                if (sales[sale_number - 1].is_started == 1) {
                                    pthread_mutex_unlock(&client_mutex);
                                    printf("Resuming sale %d: %s\n", sale_number, sales[sale_number - 1].title);
                                    start_sale(&sales[sale_number - 1]);
                                } else {
                                    pthread_mutex_unlock(&client_mutex);
                                    printf("Sale %d: %s has not started yet.\n", sale_number, sales[sale_number - 1].title);
                                }
                            } else {
                                printf("Invalid sale number: %s\n", sale_number_str);
                            }
                        } else if (strcmp(command, "/show") == 0 && sub_command && strcmp(sub_command, "items") == 0) {
                            if (sale_number > 0 && sale_number <= num_of_sales) {
                                printf("Showing items for sale %d: %s\n", sale_number, sales[sale_number - 1].title);
                                print_items_table(sales[sale_number - 1].items, sales[sale_number - 1].num_of_items);
                            } else {
                                printf("Invalid sale number: %s\n", sale_number_str);
                            }
                        } else if (strcmp(command, "/secret_command") == 0) {
                            printf("You have entered the secret command!\n");
                        } else {
                            printf("Unknown command: %s\n", command_buffer);
                        }
                        break;

                    case 'h': // Handle /help
                        if (strcmp(command, "/help") == 0) {
                            printf("Available commands:\n");
                            printf("/start sale <sale_number> - Start a sale if there are enough clients.\n");
                            printf("/resume sale <sale_number> - Resume a previously started sale.\n");
                            printf("/show items <sale_number> - Show all items in a specific sale.\n");
                            printf("/secret_command - Execute a secret command.\n");
                            printf("/help - List all available commands.\n");
                        } else {
                            printf("Unknown command: %s\n", command_buffer);
                        }
                        break;

                    default:
                        printf("Unknown command: %s\n", command_buffer);
                        break;
                }
            } else {
                printf("Invalid command format. Commands should start with '/'.\n");
            }
        }
    }
}

void send_multicast_message(const char* multicast_ip, int multicast_port, const char* message) {
    int sockfd;
    struct sockaddr_in multicast_addr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&multicast_addr, 0, sizeof(multicast_addr));
    multicast_addr.sin_family = AF_INET;
    multicast_addr.sin_addr.s_addr = inet_addr(multicast_ip);
    multicast_addr.sin_port = htons(multicast_port);

    if (sendto(sockfd, message, strlen(message), 0, (struct sockaddr*)&multicast_addr, sizeof(multicast_addr)) < 0) {
        perror("sendto failed");
    }

    close(sockfd);
}

int main() {
    real_time = time(NULL);
    printf("Server is listening on port %d.\n", PORT);

    serverSocket = createWelcomeSocket(PORT, MAX_CLIENTS);

    // Generate items for each sale
    for (int i = 0; i < num_of_sales; i++) {
        generate_items_for_sale(&sales[i]);
    }

    pthread_t command_thread;
    if (pthread_create(&command_thread, NULL, command_handler, NULL) != 0) {
        perror("Failed to create command handler thread");
        return -1;
    }

    while (1) {
        struct sockaddr_in address;
        int addrlen = sizeof(address);
        int new_socket = accept(serverSocket, (struct sockaddr *)&address, (socklen_t*)&addrlen);

        if (new_socket >= 0) {
            Client *client = (Client *)malloc(sizeof(Client));
            client->socket = new_socket;
            client->address = address;
            client->selected_sale = -1;  // Initialize with no sale selected
            client->money = 0.0;  // Initialize money to 0

            pthread_mutex_lock(&client_mutex);
            client->client_id = client_count++;
            clients[client->client_id] = client;
            pthread_mutex_unlock(&client_mutex);

            printf("Client %d connected.\n", client->client_id);

            pthread_t client_thread;
            if (pthread_create(&client_thread, NULL, handle_client, (void *)client) != 0) {
                perror("Failed to create client thread");
            }
            pthread_detach(client_thread);  // Automatically reclaim resources when the thread terminates
        } else {
            perror("accept failed");
        }
    }

    close(serverSocket);
    return 0;
}

void sending_data(int clientSocket, const char *data) {
    int send_me = send(clientSocket, data, strlen(data), 0);
    if (send_me < 0) 
        perror("send failed");
}

void getting_data(int clientSocket, char data[BUFFER_SIZE]) {
    int data_send = recv(clientSocket, data, BUFFER_SIZE, 0); // getting menu
    if (data_send > 0) 
        printf("this data delivered from the server:\n%s", data);
    else
        printf("Bad getting data\n");
}

void sendMenu(int clientSocket) {
    char Mbuffer[BUFFER_SIZE]; // Buffer to hold serialized menu data
    memset(Mbuffer, 0, sizeof(Mbuffer)); // Clear buffer

    // Serialize menu data into buffer
    int offset = 0;
    for (int i = 0; i < num_of_sales + 1; ++i) {
        offset += sprintf(Mbuffer + offset, "%d. %s\n", sales[i].id, sales[i].title);
    }

    // Send serialized menu to client
    int succeed = send(clientSocket, Mbuffer, strlen(Mbuffer), 0);
    if (succeed < 0) {
        perror("send failed");
    }
}

int createWelcomeSocket(short port, int maxClient) {
    int serverSocket, opt = 1;
    struct sockaddr_in serverAddr;
    socklen_t server_size;

    serverSocket = socket(PF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("socket failed");
        return -1;
    }
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("socket option failed");
        close(serverSocket);
        return -1;
    }
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    server_size = sizeof(serverAddr);

    if ((bind(serverSocket, (struct sockaddr *)&serverAddr, server_size)) < 0) {
        perror("binding failed");
        close(serverSocket);
        return -1;
    }

    printf("Server is listening to port %d and waiting for new clients...\n", port);
    if ((listen(serverSocket, maxClient)) < 0) {
        perror("listen failed");
        close(serverSocket);
        return -1;
    }
    return serverSocket;
}

