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

enum MessageType {
    END = 0x03,
    FYI = 0x01,
    MYM = 0x02,
    TXT = 0x04,
    MOV = 0x05,
    LFT = 0x06
};
int sockfd;
struct sockaddr_in addr;
char movmsg[1024];
void *request(void *args);
void handle_mym(char buf[]){
    printf("Make your move.\n");
    char buffer[MAX_LINE_LEN];
    char buffer1[MAX_LINE_LEN];
    int res;
    printf("Enter row:\n");
    fgets(buffer, MAX_LINE_LEN, stdin);
    printf("Enter column:\n");
    fgets(buffer1, MAX_LINE_LEN, stdin);
    res = atoi(buffer)*3+atoi(buffer1);
    if (atoi(buffer) >= 0 && atoi(buffer) <= 2 && atoi(buffer1) >= 0 && atoi(buffer1) <= 2){
        printf("%d",res);
        sprintf(movmsg, "0x05 %d", res);
    }else{
        printf("ERROR: Enter valid format (numbers between 0 and 2)\n");
    }
};
void handle_txt(char buf[]){
    printf("%s\n",&buf[5]);
};
void handle_end(char buf[]){
    printf("%s\n",&buf[5]);
};
void handle_fyi(char buf[]){
    char newbuf[sizeof(buf) + 1];
    memset(newbuf, ' ', sizeof(newbuf));
    int j;
    printf("Free fields are: \n");
    for (j = 5; j < strlen(buf) -1 ; j++){
        if (buf[j] == '1'){
            newbuf[j] = 'O';
        }else{
            if (buf[j] == '0'){
                newbuf[j] = 'X';
            }else{
                printf("(%d, %d) ",(j-5) / 3,(j - 5) % 3);
                newbuf[j] = ' ';
            }
        }
    }
    printf("\n");
    printf("Here is the board:\n");
    printf("---+---+---\n");
    int i;
    for (i = 0; i < 3; i++)
    {
        printf(" %c | %c | %c \n", newbuf[3 * i + 5], newbuf[3 * i + 6], newbuf[3 * i + 7]);
        printf("---+---+---\n");
    }
};
int get_hello(struct sockaddr_in addr,int sockfd){
    printf("Please type ~Hello~ to start!\n");
    char buffer[MAX_LINE_LEN];
    fgets(buffer, MAX_LINE_LEN, stdin);
    while (strncmp(buffer, "Hello", 5) != 0){
        printf("Please type Hello to connect!\n");
        fflush(stdin);
        fgets(buffer, MAX_LINE_LEN, stdin);
    }
    char msg[MAX_LINE_LEN + 5];
    sprintf(msg, "0x04 %s", buffer);
    int sent = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
    if (sent < strlen(msg))
    {
        printf("ERROR: Line not sent!\n");
        return 1;
    }
    return 0;
};
int main(int argc, char *argv[]){
    //connecting and creating thread
    if (argc != 3)
    {
        printf("ERROR: This function takes exactly 2 arguments (IP_ADDRESS, PORT_NUMBER)\n");
        return 1;
    }
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));

    inet_pton(AF_INET, argv[1], &addr.sin_addr);
    
    int hello = get_hello(addr,sockfd);
    if (hello != 0){
        printf("ERROR: Failed to send hello message to server!");
        return 1;
    }
    pthread_t thread;
    if (pthread_create(&thread, NULL, request, NULL))
    {
        printf("ERROR: Thread not created!\n");
        return 1;
    }
    //conencting and thread for request done
    pthread_join(thread, NULL);
    close(sockfd);
    return 0;
}

void *request(void *args){
    char buffer_rec[MAX_LINE_LEN];
    int cint;
    while (1)
    {
        memset(buffer_rec, 0, MAX_LINE_LEN);

        struct sockaddr addr_rec;
        socklen_t addr_rec_len = sizeof(addr_rec);
        int rec = recvfrom(sockfd, buffer_rec, MAX_LINE_LEN, 0, &addr_rec, &addr_rec_len);
        buffer_rec[rec] = '\0';
        char code[4];
        strncpy(code, buffer_rec, 4);
        cint = atoi(code);
        char *endptr;
        cint = strtol(code, &endptr, 16);
        switch(cint)
        {
        case END:
            handle_end(buffer_rec);
            return 0;
            break;
        case FYI:
            handle_fyi(buffer_rec);
            break;
        case MYM:
            handle_mym(buffer_rec);  
            int snt = sendto(sockfd, movmsg, strlen(movmsg), 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
            if (snt < strlen(movmsg))
            {
                printf("ERROR: Line not sent!\n");
            }
            break;
        case TXT:
            handle_txt(buffer_rec);
            break;
        case LFT://disconnection handled by server 
            break;
        default:
            printf("ERROR: Unknown server signal received, ignoring...");
        }
    }
    free(buffer_rec);
}
