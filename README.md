# CSE207 Secret Assignment

Welcome to the README file for the CSE207 Secret Assignment by Dimitrije Zdrale.

## Compilation

To compile the program, follow these steps:

1. Navigate to the directory containing `server1.c`, `client1.c`, and `Makefile`.
2. Open a terminal and run the following command: `make`

To reverse the operation and clean the build files, use the following command:

```
make clean
```

## Initialization

To initialize the server, use the following command:

```
./server1 <port number (greater than 1023)>
```

## Connecting Clients

To connect the clients, follow these steps:

1. Open a terminal.
2. Use the following command:

```
./client <ip address> <port number>
```

Note: Replace `<ip address>` with the desired IP address (e.g., 127.0.0.1) and `<port number>` with the same port used for server initialization.

## Gameplay

Once the clients are connected, the game proceeds as follows:

1. You will be greeted with a welcome message. The server automatically receives a "Hello" message for testing purposes.
2. After two clients have connected, the board state will be displayed each turn, along with a prompt to play.
3. To make a move, enter an integer between 0 and 2 (inclusive) in the terminal for the column and row, respectively.
4. The game will continue until there is a winner or the game ends in a draw.
5. Once the game is over, you will be informed of the winner or if the game ended in a draw.
6. If you wish to play again, repeat from Step 3.

Feel free to explore the code and enjoy playing the game, and thanks for reading!

---
