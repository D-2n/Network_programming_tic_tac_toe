#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_LINE_LEN 1000

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

#define  END 0x03
#define  FYI 0x01
#define  MYM 0x02
#define  TXT 0x04
#define  MOV 0x05
#define  LFT 0x06

int sockfd;
struct sockaddr_in addr;
char movmsg[3];

void *request(void *args);
void handle_mym();
void handle_txt(char buf[], int len);
void handle_end(char buf[]);
void handle_fyi(char *buf, int len);

int get_hello(struct sockaddr_in addr,int sockfd);
int main(int argc, char *argv[]);

/**
 * Prompts the user to make a move by entering a column and row.
 * Sends the move message to the server.
 *
 * @param sockfd The socket file descriptor.
 */
void handle_mym()
{
    printf("Make your move.\n");
    int column, row;

    do
    {
        printf("Enter column (0-2):\n");
        scanf("%d", &column);
    }
    while (column < 0 || column > 2);

    do
    {
        printf("Enter row (0-2):\n");
        scanf("%d", &row);
    }
    while (row < 0 || row > 2);

    movmsg[0] = MOV;
    movmsg[1] = column;
    movmsg[2] = row;

    sendto(sockfd, movmsg, sizeof(movmsg), 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
}
/**
 * Handles the TXT command received from the server.
 *
 * @param buf The received buffer.
 * @param len The length of the buffer.
 */
void handle_txt(char buf[], int len)
{
    buf[len] = '\0';
    printf("%s\n", buf+1);
}
/**
 * Handles the END command received from the server.
 * Prints the game result based on the winner information.
 *
 * @param buf The received buffer.
 */
void handle_end(char buf[])
{
    printf("Game ended.\n");

    unsigned char winner = (unsigned char)buf[1];
    switch(winner)
    {
    case 0:
        printf("It's a tie!\n");
        break;
    case 1:
        printf("Player 1 won!\n");
        break;
    case 2:
        printf("Player 2 won!\n");
        break;
    default:
        printf("Unexpected winner information received: %d\n", winner);
        break;
    }

    exit(0);
}

/**
 * Handles the FYI command received from the server.
 * Prints the current game board and the moves made by the players.
 *
 * @param buf The received buffer.
 * @param len The length of the buffer.
 */
void handle_fyi(char *buf, int len)
{
    char board[3][3] = { {' ', ' ', ' '}, {' ', ' ', ' '}, {' ', ' ', ' '} };

    unsigned char num_moves = (unsigned char)buf[1];

    printf("Fields taken: %d\n", num_moves);
    int i;

    if (num_moves > 0){
        for (i = 0; i < num_moves; i++)
            {
                unsigned char player = (unsigned char)buf[2 + 3*i];
                unsigned char column = (unsigned char)buf[3 + 3*i];
                unsigned char row = (unsigned char)buf[4 + 3*i];

                board[row][column] = (player == 1) ? 'X' : 'O';
                printf("Player %d put on (%d,%d)\n", player, column, row);
            }
    }

    printf("Here is the board:\n");
    printf("---+---+---\n");
    for (i = 0; i < 3; i++)
    {
        printf(" %c | %c | %c \n", board[i][0], board[i][1], board[i][2]);
        printf("---+---+---\n");
    }
}
/**
 * Function executed by the request thread.
 * Continuously receives messages from the server and handles them accordingly.
 *
 * @param args The thread arguments.
 */
void *request(void *args)
{
    char buffer_rec[MAX_LINE_LEN];
    int len;
    socklen_t addrlen;

    while(1)
    {
        len = recvfrom(sockfd, buffer_rec, MAX_LINE_LEN, 0, (struct sockaddr *)&addr, &addrlen);

        if(len < 0)
        {
            perror("Error on receiving: ");
            exit(EXIT_FAILURE);
        }

        switch(buffer_rec[0])
        {
        case FYI:
            handle_fyi(buffer_rec, len);
            break;
        case TXT:
            handle_txt(buffer_rec, len);
            break;
        case MYM:
            handle_mym();
            break;
        case END:
            handle_end(buffer_rec);
            break;
        default:
            fprintf(stderr, "Unknown message type: %d\n", buffer_rec[0]);
            break;
        }
    }

    pthread_exit(NULL);
}
/**
 * Sends a hello message to the server.
 * I've decided to do it automatically to speed up the testing process, writing Hello every time for hundreds of executions gets boring... :0
 *
 * @param addr The server's address.
 * @param sockfd The socket file descriptor.
 * @return 0 if successful, -1 otherwise.
 */
int get_hello(struct sockaddr_in addr,int sockfd)
{
    char buffer_hello[2] = {TXT, 0};

    if(sendto(sockfd, buffer_hello, sizeof(buffer_hello), 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0)
    {
        perror("Error on sending hello: ");
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    pthread_t thread;
    int ret;

    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Error on creating socket: ");
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &(addr.sin_addr));

    if(get_hello(addr,sockfd) < 0)
    {
        return -1;
    }

    ret = pthread_create(&thread, NULL, request, NULL);

    if(ret != 0)
    {
        printf("Error: pthread_create() failed\n");
        return -1;
    }

    pthread_detach(thread);

    while(1) sleep(1);

    return 0;
}
