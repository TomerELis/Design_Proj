#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <errno.h> // Include this header for error handling

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
    int num_of_clients;
    int is_started;
    Item items[MAX_ITEMS];
    int num_of_items;
    double money_capacity;  // New field to represent money given to each client for this sale
    int multicast_port;     // Field for dynamic multicast port for each sale
} Sale;

typedef struct {
    int socket;
    struct sockaddr_in address;
    int client_id;
    char user_name[BUFFER_SIZE];
    int selected_sale;
    double money;
    int bid_value;
    int sent_bind; // 1 if the client has sent a bid
} Client;

//========================
// Function Declarations
//========================
void sendMenu(int clientSocket);
void sending_data(int clientSocket, const char *data);
void getting_data(int clientSocket, char data[BUFFER_SIZE]);
int createWelcomeSocket(short port, int maxClient);
void send_multicast_message(const char* multicast_ip, int multicast_port, const char* message);
void *handle_client(void *client_ptr);
void *command_handler(void *arg);
void generate_items_for_sale(Sale *sale);
void print_items_table(Item *items, int num_items);
void clear_client_screens(Sale *sale);
void start_sale(Sale *sale);
void handle_bidding(Sale *sale); // Make sure this is declared
int compare_clients(const void *a, const void *b);
void sort_clients_by_bid(Client **client_list, int num_clients);

//========================
// Global Parameters
//========================
int num_of_sales = 4;
Client *clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;
int serverSocket;
int skip_waiting = 0; // Control the waiting period

Sale sales[MAX_SALES] = {
    {1, "Summer Sale", "224.2.1.1", 0, 0, {{0}}, 0, 150.0, 12345},  // Sale 1 with $150 initial money
    {2, "Back to School Sale", "224.2.2.1", 0, 0, {{0}}, 0, 200.0, 12346},  // Sale 2 with $200 initial money
    {3, "Holiday Sale", "224.2.3.1", 0, 0, {{0}}, 0, 300.0, 12347},  // Sale 3 with $300 initial money
    {4, "End of Year Clearance", "224.2.4.1", 0, 0, {{0}}, 0, 250.0, 12348},  // Sale 4 with $250 initial money
    {-1, "Exit", "0.0.0.0", 0, 0, {{0}}, 0, 0.0, 0}  // Exit sale, no money associated
};

//========================
// Main Function
//========================
int main() {
    printf("Server is listening on port %d.\n", PORT);

    serverSocket = createWelcomeSocket(PORT, MAX_CLIENTS);

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
            client->selected_sale = -1;
            client->money = 0.0;
            client->bid_value = 0;

            pthread_mutex_lock(&client_mutex);
            client->client_id = client_count++;
            clients[client->client_id] = client;
            pthread_mutex_unlock(&client_mutex);

            printf("Client %d connected.\n", client->client_id);

            pthread_t client_thread;
            if (pthread_create(&client_thread, NULL, handle_client, (void *)client) != 0) {
                perror("Failed to create client thread");
            }
            pthread_detach(client_thread);
        } else {
            perror("accept failed");
        }
    }

    close(serverSocket);
    return 0;
}

//========================
// Socket Handling Functions
//========================
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

//========================
// Data Handling Functions
//========================
void getting_data(int clientSocket, char data[BUFFER_SIZE]) {
    int data_send = recv(clientSocket, data, BUFFER_SIZE, 0);
    if (data_send > 0) 
        printf("this data delivered from the server:\n%s", data);
    else
        printf("Bad getting data\n");
}

void sending_data(int clientSocket, const char *data) {
    int send_me = send(clientSocket, data, strlen(data), 0);
    if (send_me < 0) 
        perror("send failed");
}

void sendMenu(int clientSocket) {
    char Mbuffer[BUFFER_SIZE];
    memset(Mbuffer, 0, sizeof(Mbuffer));

    int offset = 0;
    for (int i = 0; i < num_of_sales + 1; ++i) {
        offset += sprintf(Mbuffer + offset, "%d. %s\n", sales[i].id, sales[i].title);
    }

    int succeed = send(clientSocket, Mbuffer, strlen(Mbuffer), 0);
    if (succeed < 0) {
        perror("send failed");
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
    } else {
        printf("Multicast message sent to %s: %s\n", multicast_ip, message);
    }

    close(sockfd);
}

void clear_client_screens(Sale *sale) {
    char buffer[BUFFER_SIZE] = "\033[2J\033[H"; // ANSI escape code to clear the screen
    send_multicast_message(sale->multicast_ip, sale->multicast_port, buffer);
}

//========================
// Utility Functions
//========================
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

void sort_clients_by_bid(Client **client_list, int num_clients) {
    for (int i = 0; i < num_clients - 1; i++) {
        for (int j = 0; j < num_clients - i - 1; j++) {
            if (client_list[j]->bid_value < client_list[j + 1]->bid_value) {
                Client *temp = client_list[j];
                client_list[j] = client_list[j + 1];
                client_list[j + 1] = temp;
            }
        }
    }
}

int compare_clients(const void *a, const void *b) {
    Client *client_a = *(Client **)a;
    Client *client_b = *(Client **)b;
    return client_b->bid_value - client_a->bid_value;  // Sort in descending order
}

//========================
// Client Handling Functions
//========================
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

//========================
// Command Handling Function
//========================
void *command_handler(void *arg) {
    char command_buffer[BUFFER_SIZE];
    while (1) {
        printf("Enter command: ");
        if (fgets(command_buffer, BUFFER_SIZE, stdin) != NULL) {
            command_buffer[strcspn(command_buffer, "\n")] = '\0';

            if (command_buffer[0] == '/') {
                char *command = strtok(command_buffer, " ");
                char *sub_command = strtok(NULL, " ");
                char *sale_number_str = strtok(NULL, " ");
                int sale_number = 0;

                if (sale_number_str) {
                    sale_number = atoi(sale_number_str);
                }

                switch (command[1]) {
                    case 's':
                        if (strcmp(command, "/start") == 0 && sub_command && strcmp(sub_command, "sale") == 0) {
                            if (sale_number > 0 && sale_number <= num_of_sales) {
                                pthread_mutex_lock(&client_mutex);
                                int num_clients = sales[sale_number - 1].num_of_clients;
                                pthread_mutex_unlock(&client_mutex);

                                if (num_clients >= 2) {
                                    pthread_mutex_lock(&client_mutex);
                                    if (sales[sale_number - 1].is_started == 0) {
                                        sales[sale_number - 1].is_started = 1;
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
                        } else if (strcmp(command, "/skip") == 0) {
                            // Implement skip functionality
                            skip_waiting = 1; // Set flag to skip waiting period
                            printf("Skipping the waiting period...\n");
                        } else {
                            printf("Unknown command: %s\n", command_buffer);
                        }
                        break;

                    case 'h':
                        if (strcmp(command, "/help") == 0) {
                            printf("Available commands:\n");
                            printf("/start sale <sale_number> - Start a sale if there are enough clients.\n");
                            printf("/resume sale <sale_number> - Resume a previously started sale.\n");
                            printf("/show items <sale_number> - Show all items in a specific sale.\n");
                            printf("/secret_command - Execute a secret command.\n");
                            printf("/skip - Skip the current waiting period.\n");
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

//========================
// Sale Management Functions
//========================
void start_sale(Sale *sale) {
    char buffer[BUFFER_SIZE];

    // Inform clients that the sale has started
    snprintf(buffer, BUFFER_SIZE, "The sale %s has started!\n", sale->title);
    send_multicast_message(sale->multicast_ip, sale->multicast_port, buffer);

    // Inform clients of the rules and the initial money
    snprintf(buffer, BUFFER_SIZE, "The rule is simple: each one of you has %.2f$. There are %d items in this sale. Each item has a starting price.\nSo think well before making offers!\n\nAh! There’s one more important rule: THE SPICE RULE: The second highest price wins!\n\ngood luck!", sale->money_capacity, sale->num_of_items);
    send_multicast_message(sale->multicast_ip, sale->multicast_port, buffer);

    snprintf(buffer, BUFFER_SIZE, "You have been given %.2f$ for this sale.\n", sale->money_capacity);
    send_multicast_message(sale->multicast_ip, sale->multicast_port, buffer);

    // Set initial money for each client in this sale
    pthread_mutex_lock(&client_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL && clients[i]->selected_sale == sale->id) {
            clients[i]->money = sale->money_capacity;
            clients[i]->sent_bind = 0;  // Reset sent_bind flag
        }
    }
    pthread_mutex_unlock(&client_mutex);

    // Wait for 20 seconds or until /skip command is issued
    int wait_time = 20;
    while (wait_time > 0 && !skip_waiting) {
        sleep(1);
        wait_time--;
    }

    // Reset the skip_waiting flag
    skip_waiting = 0;

    // Clear client screens and start the bidding process
    clear_client_screens(sale);

    // Handle the bidding process
    handle_bidding(sale);
}

//========================
// Bidding Management Function
//========================
void handle_bidding(Sale *sale) {
    int num_clients = sale->num_of_clients;
    Client **bidding_clients = malloc(sizeof(Client *) * num_clients);
    int client_count = 0;
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL && clients[i]->selected_sale == sale->id) {
            bidding_clients[client_count++] = clients[i];
        }
    }

    time_t start_time = time(NULL);
    while (time(NULL) - start_time < 20) {
        int all_bids_received = 1;
        for (int i = 0; i < client_count; i++) {
            if (bidding_clients[i]->sent_bind == 0) {
                all_bids_received = 0;
                break;
            }
        }
        if (all_bids_received) break;
        usleep(100000); // Sleep for 100 ms to avoid busy-waiting
    }

    // Sort the clients based on their bid value in descending order
    qsort(bidding_clients, client_count, sizeof(Client *), compare_clients);

    // Determine the highest and second highest bids
    int highest_bid = (client_count > 0) ? bidding_clients[0]->bid_value : 0;
    int second_highest_bid = (client_count > 1) ? bidding_clients[1]->bid_value : 0;

    // Find the winning client
    Client *winning_client = (client_count > 0) ? bidding_clients[0] : NULL;

    if (winning_client != NULL) {
        // Calculate the required size for the formatted string
        int size = snprintf(NULL, 0, "The winner is %s with a bid of %d$ for the item: %s!\n",
                            winning_client->user_name, highest_bid, sale->items[sale->num_of_items - 1].name);
        size += 1; // For the null terminator

        // Dynamically allocate memory for the buffer
        char *buffer = malloc(size);

        // Use snprintf to format the string into the dynamically allocated buffer
        snprintf(buffer, size, "The winner is %s with a bid of %d$ for the item: %s!\n",
                 winning_client->user_name, highest_bid, sale->items[sale->num_of_items - 1].name);

        send_multicast_message(sale->multicast_ip, sale->multicast_port, buffer);
        
        // Update winning client's money
        winning_client->money -= second_highest_bid;

        // Send the results to the winning client
        size = snprintf(NULL, 0, "Congratulations %s! You've won the item for %d$. Your remaining money: %.2f$\n",
                        winning_client->user_name, second_highest_bid, winning_client->money);
        size += 1;

        char *result_message = malloc(size);
        snprintf(result_message, size, "Congratulations %s! You've won the item for %d$. Your remaining money: %.2f$\n",
                 winning_client->user_name, second_highest_bid, winning_client->money);

        sending_data(winning_client->socket, result_message);

        // Free the dynamically allocated memory
        free(buffer);
        free(result_message);
    }

    free(bidding_clients);

    sale->num_of_items--;
}
