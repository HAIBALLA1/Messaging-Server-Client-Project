# Messaging Server-Client Project

## Overview

This project is a server-client messaging application written in C. The application allows clients to connect to a server, exchange messages, and save messages in a MySQL database for persistence. The communication between clients and the server is implemented using TCP sockets, and multi-threading is used to handle multiple client connections simultaneously.

The server allows clients to either log in or sign up, and then exchange messages between themselves or broadcast messages to all connected clients. The MySQL database is used to manage user accounts and store messages securely.

## Project Structure

The project consists of several source files, each of which is responsible for a particular part of the server-client system. Below is an overview of the key files:

### 1. **main.c (Server Main)**

- **Purpose**: Implements the main logic of the server, including socket creation, binding, listening, and handling incoming client connections.
- **Features**:
  - The server listens on a specified port for incoming client connections.
  - Handles the setup for MySQL connection, which is used to store messages and user information.
  - Accepts new connections and spawns threads for each client to manage communications.

### 2. **client.c (Client Main)**

- **Purpose**: Implements the main logic of the client application, including connecting to the server and sending/receiving messages.
- **Features**:
  - The client connects to the server on a given IP address and port.
  - The client supports a login/signup feature to authenticate or register with the server.
  - Uses threads to manage user input for sending messages and for receiving server responses.

### 3. **socket.c**

- **Purpose**: Contains utility functions for creating and managing sockets.
- **Key Functions**:
  - **creer_TCPI_IPv4_Socket()**: Creates a TCP/IPv4 socket.
  - **creer_IPv4_adresse()**: Creates an IPv4 address structure for the given IP and port.
  - **Accepter_nouv_connection()**: Accepts a new incoming client connection.
  - **Commence_Accepter_Connections()**: Handles the loop for accepting multiple client connections.

### 4. **mysql_utils.c**

- **Purpose**: Contains functions for interacting with the MySQL database.
- **Key Functions**:
  - **initialize_mysql()**: Initializes a connection to the MySQL database.
  - **verifier_et_reconnecter_mysql()**: Verifies the MySQL connection is alive, and reconnects if necessary.
  - **sauvegarde_message()**: Saves messages to the database.
  - **verifier_utilisateur()**: Verifies user login credentials.
  - **ajouter_utilisateur()**: Adds a new user to the database.

### 5. **client_thread_functions.c**

- **Purpose**: Manages threads for the server and client communication.
- **Key Functions**:
  - **traiter_nouveau_client_avec_diff_threads()**: Handles creating a new thread for each client connected to the server.
  - **Recevoir_Affiecher_Messages_des_clients()**: Handles the incoming messages from clients and broadcasts them if necessary.
  - **Evoyer_Groupe()**: Sends messages to all connected clients.

### 6. **client_io.c**

- **Purpose**: Manages client input/output.
- **Key Functions**:
  - **lire_messages_de_la_console_envoiyer()**: Reads messages from the console and sends them to the server.
  - **Recevoir_Reponses_serv()**: Receives responses from the server and prints them on the client console.

## Key Features

- **User Management**: Clients can either register (sign-up) or log in to the server, using stored credentials in the MySQL database.
- **Multi-threaded Server**: The server can handle multiple clients simultaneously using pthreads.
- **Message Persistence**: Messages exchanged between clients are stored in a MySQL database, enabling future access.
- **Broadcast and Direct Messaging**: Clients can send messages either to all connected clients or to a specific client by specifying the recipient's username.
- **Reconnection to MySQL**: The server maintains the MySQL connection and reconnects if the connection is lost.

## How to Build and Run

### Requirements

- **C Compiler**: GCC or any modern C compiler.
- **MySQL Server**: Ensure MySQL server is running and accessible.
- **Libraries**: `libmysqlclient` for MySQL integration.

### Building

To compile all files, use the following command:

```sh
gcc main.c socket.c mysql_utils.c client_thread_functions.c client_io.c -o server -lpthread -lmysqlclient
```

For the client:

```sh
gcc client.c socket.c client_io.c -o client -lpthread
```

### Running

First, start the server:

```sh
./server
```

Then start one or more clients:

```sh
./client
```

Clients will be prompted to either log in or sign up and then can begin messaging.

## Usage

- **Sign Up**: Clients can register new accounts by entering a username and password.
- **Login**: Clients can log in with an existing username and password.
- **Messaging**: After authentication, clients can send messages either to a specific user or to all users.

## Database Setup

Ensure the MySQL database is set up with the following structure:

```sql
CREATE DATABASE MESSAGERIEPROJECT;
USE MESSAGERIEPROJECT;

CREATE TABLE utilisateur (
    username VARCHAR(100) PRIMARY KEY,
    password VARCHAR(100) NOT NULL
);

CREATE TABLE messages (
    id INT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(100),
    message TEXT,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

## Dependencies

- **libmysqlclient**: To interact with MySQL.
- **pthread**: For multi-threading to handle multiple clients.

## Contributing

Feel free to contribute by submitting pull requests for new features, bug fixes, or improvements to the documentation.

## License

This project is licensed under the MIT License.

