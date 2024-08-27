Memory Mode: Project Overview and Current State

This Memory Mode provides a structured and detailed overview of the project's current state, including all functions, their purposes, the goals of the project, and key improvements. The goal is to ensure a complete understanding and remembrance of the project's code structure.
Project Summary

The project is a network-based auction system where multiple clients can connect to a server to participate in various sales. The server manages these sales by facilitating bid submissions, determining winners, and communicating results back to the clients using multicast messages.

The system allows clients to:

    Connect to the server and authenticate.
    View available sales and choose to participate in one.
    Place bids on items during a sale.
    Receive real-time updates about the auction's progress and results.

The server is responsible for:

    Handling multiple client connections simultaneously.
    Managing the lifecycle of sales and auctions.
    Processing bids dynamically and fairly.
    Broadcasting auction updates and results to all connected clients.

Server Code Overview

The server code is designed to manage multiple clients, facilitate sales and auctions, and handle the bidding process dynamically. The server uses a multicast mechanism to communicate with all clients about the ongoing sale status and results.
Functions in Server Code

    main:
        Initializes the server, sets up the welcome socket, creates the command handler thread, and accepts new client connections.

    createWelcomeSocket:
        Creates a socket to listen for incoming client connections.

    getting_data and sending_data:
        Functions to handle receiving and sending data between the server and clients.

    sendMenu:
        Sends the sales menu to the connected client.

    send_multicast_message:
        Sends multicast messages to clients in a specific sale group.

    clear_client_screens:
        Clears the client screens by sending ANSI escape codes.

    generate_items_for_sale:
        Generates and initializes items available in each sale.

    print_items_table:
        Prints the list of items available in a sale to the server console.

    sort_clients_by_bid and compare_clients:
        Functions to sort clients based on their bids in descending order.

    handle_client (Great Function):
        Manages client connections, including authentication, handling menu selection, and managing the client's participation in a sale.

    command_handler (Great Function):
        Handles server-side commands for controlling sales and displaying items. Commands like /start sale, /resume sale, /show items, and /skip are processed here.

    start_sale:
        Initializes a sale, informs clients, sets initial money for bidding, and begins the bidding process.

    handle_bidding (Great Function):
        Manages the bidding process during a sale, including waiting for bids from all clients, determining the winner, and broadcasting results to all clients.

Client Code Overview

The client code is designed to connect to the server, participate in sales, and place bids. It also receives multicast messages to stay updated on the sale status and results.
Functions in Client Code

    main:
        Initializes the client, establishes a connection to the server, handles user authentication, and manages the menu selection and sale participation.

    getting_data and sending_data:
        Functions to handle receiving and sending data between the client and the server.

    receive_multicast:
        Receives multicast messages from the server, which may include updates about the sale and bid requests.

    handle_bidding:
        Manages the client's bidding process by sending bid values to the server.

What it Means to Be a 'Great Function':

A "Great Function" is one that plays a critical role in the system's core operations. These functions are typically large, handling multiple responsibilities, and are vital to the overall functionality of the system. They are key points for optimization and debugging.

    handle_client: Manages all client interactions, including login, sale selection, and multicast communication setup.
    command_handler: Processes all server-side commands, ensuring the server can control sales dynamically and respond to operator input.
    handle_bidding: Controls the bidding logic during a sale, including bid collection, winner determination, and result communication.

Headers in the Code

The code is organized with headers to separate different functional sections, making it more readable and maintainable. The headers are:

    //========================
    // Connection Management Functions
    //========================

    //========================
    // Data Handling Functions
    //========================

    //========================
    // Sale Management Functions
    //========================

    //========================
    // Bidding Management Functions
    //========================

    //========================
    // Command Management Functions
    //========================

Current Goals and Improvements:

    Allow Command Input During Sale:
        Modify the server to handle command input during an active sale without stopping the command handler thread.

    Improve Client-Side Bidding Function:
        Refactor the handle_bidding function on the client side to ensure smooth bidding during sales.

    Enhance Dynamic Memory Management:
        Optimize dynamic memory usage for better scalability and performance.

    Ensure Robust Error Handling:
        Improve error handling mechanisms to make the system more robust and error-resistant.

Dynamic Memory Usage Details

Dynamic memory allocation is used extensively to manage client connections, handle variable-length strings, and store dynamic arrays (e.g., for clients in a sale). This allows the system to scale efficiently and handle multiple clients and sales dynamically.
Memory Mode Reminder

The most important thing to remember is the 'Memory Mode' - because when you remember what you need to remember, you remember everything.
End of Memory Mode
