הקובץ המצורף 20240625_115233.jpg נוסף. 
השיחה נפתחה. הודעה אחת שלא נקראה.

דילוג לתוכן
שימוש ב-דואר Ben-Gurion University of the Negev עם קוראי מסך
1 מתוך 4,222
latest work 3 clients OMG OMG OMG‏
לא בדומיין
דואר נכנס
Tomer E
	
קבצים מצורפים20:37 ‎(לפני 0 דקות)‎
	
אני

 2 קבצים מצורפים  •  נסרקו על ידי Gmail
	

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <poll.h> // Include the poll library for adding timeout functionality

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
    int multicast_port;  // Port for multicast communication
    int num_of_clients;
    int is_started;
    Item items[MAX_ITEMS];
    int num_of_items;
} Sale;

typedef struct {
    int socket;
    struct sockaddr_in address;
    int client_id;
    char user_name[BUFFER_SIZE];
    int selected_sale;
    double money;
    int bid_value;
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
void clear_client_screens(Sale *sale);
void start_sale(Sale *sale);
void handle_bidding(Sale *sale);

// Global parameters
int num_of_sales = 4;
Client *clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;
int serverSocket;
Sale sales[MAX_SALES] = {
    {1, "Summer Sale", "224.2.1.1", 12345, 0, 0, {{0}}, 0},
    {2, "Back to School Sale", "224.2.2.1", 12346, 0, 0, {{0}}, 0},
    {3, "Holiday Sale", "224.2.3.1", 12347, 0, 0, {{0}}, 0},
    {4, "End of Year Clearance", "224.2.4.1", 12348, 0, 0, {{0}}, 0},
    {-1, "Exit", "0.0.0.0", 0, 0, 0, {{0}}, 0}
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

void handle_bidding(Sale *sale) {
    int highest_bid = 0;
    int second_highest_bid = 0;
    int winning_client_id = -1;
    char buffer[BUFFER_SIZE] = {0};
    int bets_received = 0;
    int active_clients = sale->num_of_clients;

    printf("Starting bidding for item: %s with %d active clients\n", sale->items[sale->num_of_items - 1].name, active_clients);

    // Notify clients to place their bids
    snprintf(buffer, BUFFER_SIZE, "Place your bids for item: %.20s (Starting price: %.2f$)\n", 
             sale->items[sale->num_of_items - 1].name, sale->items[sale->num_of_items - 1].start_price);
    send_multicast_message(sale->multicast_ip, sale->multicast_port, buffer);

    // Collect bids from clients
    while (bets_received < active_clients) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] != NULL && clients[i]->selected_sale == sale->id) {
                // Set a receive timeout
                struct timeval tv;
                tv.tv_sec = 10;  // 10 seconds timeout
                tv.tv_usec = 0;
                setsockopt(clients[i]->socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

                int bytes_received = recv(clients[i]->socket, buffer, BUFFER_SIZE, 0);
                if (bytes_received > 0) {
                    buffer[bytes_received] = '\0';
                    int bid = atoi(buffer);
                    clients[i]->bid_value = bid;
                    bets_received++;
                    printf("Received bid of %d$ from client %d (%s)\n", bid, clients[i]->client_id, clients[i]->user_name);

                    // Determine highest and second-highest bids
                    if (bid > highest_bid) {
                        second_highest_bid = highest_bid;
                        highest_bid = bid;
                        winning_client_id = i;
                    } else if (bid > second_highest_bid) {
                        second_highest_bid = bid;
                    }
                } else if (bytes_received == 0) {
                    printf("Client %d disconnected.\n", clients[i]->client_id);
                    close(clients[i]->socket);
                    clients[i] = NULL;
                    active_clients--;
                } else {
                    printf("Client %d timed out or error occurred. Ignoring this client for this round.\n", clients[i]->client_id);
                    active_clients--;
                }
            }
        }
    }

    // Announce the winner (the highest bidder) and charge them the second-highest bid
    if (winning_client_id != -1) {
        snprintf(buffer, BUFFER_SIZE, 
            "The winner is %.20s with a bid of %d$, but they will pay %d$ for the item: %.20s!\n",
            clients[winning_client_id]->user_name, highest_bid, second_highest_bid, sale->items[sale->num_of_items - 1].name);
        
        send_multicast_message(sale->multicast_ip, sale->multicast_port, buffer);

        // Deduct money from the winner based on the second-highest bid
        clients[winning_client_id]->money -= second_highest_bid;

        snprintf(buffer, BUFFER_SIZE, 
            "Congratulations %.20s! You've won the item with a bid of %d$, but you will pay %d$. Your remaining money: %.2f$\n",
            clients[winning_client_id]->user_name, highest_bid, second_highest_bid, clients[winning_client_id]->money);

        sending_data(clients[winning_client_id]->socket, buffer);  // Send personalized message to the winner

        printf("The winner is %s with a bid of %d$, paying %d$ for the item: %s!\n",
               clients[winning_client_id]->user_name, highest_bid, second_highest_bid, sale->items[sale->num_of_items - 1].name);
    } else {
        printf("No winner could be determined for this item.\n");
    }

    sale->num_of_items--;
}


void start_sale(Sale *sale) {
    char buffer[BUFFER_SIZE];

    snprintf(buffer, BUFFER_SIZE, "The sale started!\n");
    send_multicast_message(sale->multicast_ip, sale->multicast_port, buffer);

    snprintf(buffer, BUFFER_SIZE, "The rule is simple: each one of you has %.2f$. There are %d items in this sale. Each item has a starting price.\nSo think well before making offers!\n\nAh! There’s one more important rule.THE SPICE RULE: The second highest price wins!\n\ngood luck!",
             sale->items[0].start_price * 10, sale->num_of_items);
    send_multicast_message(sale->multicast_ip, sale->multicast_port, buffer);

    snprintf(buffer, BUFFER_SIZE, "You have been given %.2f$ for this sale.", sale->items[0].start_price * 10);
    send_multicast_message(sale->multicast_ip, sale->multicast_port, buffer);

    pthread_mutex_lock(&client_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL && clients[i]->selected_sale == sale->id) {
            clients[i]->money = sale->items[0].start_price * 10;
            clients[i]->bid_value = 0;
        }
    }
    pthread_mutex_unlock(&client_mutex);

    sleep(2);
    clear_client_screens(sale);

    while (sale->num_of_items > 0) {
        handle_bidding(sale);

        if (sale->num_of_items > 0) {
            snprintf(buffer, BUFFER_SIZE, "Next item: %s\n", sale->items[sale->num_of_items - 1].name);
            send_multicast_message(sale->multicast_ip, sale->multicast_port, buffer);
            sleep(10);
        }
    }

    snprintf(buffer, BUFFER_SIZE, "The sale has ended! Congratulations to all winners!\n");
    send_multicast_message(sale->multicast_ip, sale->multicast_port, buffer);
}

void clear_client_screens(Sale *sale) {
    char buffer[BUFFER_SIZE] = "\033[2J\033[H";
    send_multicast_message(sale->multicast_ip, sale->multicast_port, buffer);
}

void *handle_client(void *client_ptr) {
    Client *client = (Client *)client_ptr;
    char buffer[BUFFER_SIZE] = {0};
    ssize_t bytes_received, bytes_received_sale;
    int flg = 0;

    while (1) {
        printf("Handling client %d\n", client->client_id);
        bytes_received = recv(client->socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            if (flg == 1) {
                printf("Password use the secret password %d: %s\n", client->client_id, buffer);
                flg = 0;
                int result = strcmp(secret, buffer);
                if (result == 0) {
                    printf("username entered the right pass - %s\n", buffer);
                    sending_data(client->socket, "Authentication successful. Please choose a menu number.\n");
                    sleep(2);
                    while (1) {
                        sendMenu(client->socket);
                        memset(buffer, 0, BUFFER_SIZE);
                        bytes_received_sale = recv(client->socket, buffer, BUFFER_SIZE, 0);
                        int numb_menu = atoi(buffer);
                        if (numb_menu > num_of_sales || numb_menu <= 0) {
                            printf("Problem: menu has only %d options, and user chose number: %d\n", num_of_sales, numb_menu);
                            sending_data(client->socket, "Invalid menu option. Please try again.\n");
                        } else {
                            printf("username chose on menu the number - %s\n", buffer);

                            pthread_mutex_lock(&client_mutex);
                            sales[numb_menu - 1].num_of_clients++;
                            client->selected_sale = numb_menu;
                            pthread_mutex_unlock(&client_mutex);

                            char dat[BUFFER_SIZE];
                            snprintf(dat, BUFFER_SIZE, "Multicast IP: %s\nPort: %d\n", sales[client->selected_sale - 1].multicast_ip, sales[client->selected_sale - 1].multicast_port);
                            sending_data(client->socket, dat);
                            break;
                        }
                    }
                } else {
                    sending_data(client->socket, "Wrong password. I am calling the police! WEEWOOWEEOO\n");
                    close(client->socket);
                    pthread_exit(NULL);
                }
            } else if (flg == 0) {
                printf("Received offer to username from client %d: %s\n", client->client_id, buffer);
                strncpy(client->user_name, buffer, BUFFER_SIZE);
                
                // Remove any trailing newline character from the username
                client->user_name[strcspn(client->user_name, "\n")] = '\0';

                flg = 1;
            }
        } else if (bytes_received == 0) {
            printf("Client %d disconnected\n", client->client_id);

            if (client->selected_sale > 0 && client->selected_sale <= num_of_sales) {
                pthread_mutex_lock(&client_mutex);
                sales[client->selected_sale - 1].num_of_clients--;
                pthread_mutex_unlock(&client_mutex);
            }

            close(client->socket);
            pthread_exit(NULL);
        } else {
            close(client->socket);
            perror("Receive failed");

            if (client->selected_sale > 0 && client->selected_sale <= num_of_sales) {
                pthread_mutex_lock(&client_mutex);
                sales[client->selected_sale - 1].num_of_clients--;
                pthread_mutex_unlock(&client_mutex);
            }

            pthread_exit(NULL);
        }
    }
}


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
    
    // Commenting out or removing the following line to avoid excessive logging
    // printf("Multicast message sent to %s: %s\n", multicast_ip, message);

    close(sockfd);
}

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

void sending_data(int clientSocket, const char *data) {
    int send_me = send(clientSocket, data, strlen(data), 0);
    if (send_me < 0) 
        perror("send failed");
}

void getting_data(int clientSocket, char data[BUFFER_SIZE]) {
    int data_send = recv(clientSocket, data, BUFFER_SIZE, 0);
    if (data_send > 0) 
        printf("this data delivered from the server:\n%s", data);
    else
        printf("Bad getting data\n");
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

server.txt
מציג את client.txt.

