#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#define MAX_LINE_LEN 1000
#define MAX_CLIENTS 2

pthread_t threads[MAX_CLIENTS] = {0};


struct client_info{
    char *buffer;
    struct sockaddr *addr_rec;
    socklen_t addr_rec_len;
    int rec;
    int usedflag; // 0 if inactive client and can be replaced, 1 if taken and active
    int player;
};
struct game{
    int free_spots; //number of free spots to keep track of the game state
    int grid[9]; //grid to keep track of where stuff is placed
    int turn; //player to play
};
struct client_info clients[MAX_CLIENTS];

pthread_mutex_t lock;
int sockfd;


void *request(void *args_void){
    printf("In request\n");
    return NULL;
}

int checkWin(int grid[]) {
    int i;
    
    // Check rows
    for (i = 0; i < 9; i += 3) {
        if (grid[i] != 9 && grid[i] == grid[i + 1] && grid[i] == grid[i + 2]) {
            return grid[i];
        }
    }
    
    // Check columns
    for (i = 0; i < 3; i++) {
        if (grid[i] != 9 && grid[i] == grid[i + 3] && grid[i] == grid[i + 6]) {
            return grid[i];
        }
    }
    
    // Check diagonals
    if (grid[0] != 9 && grid[0] == grid[4] && grid[0] == grid[8]) {
        return grid[0];
    }
    
    if (grid[2] != 9 && grid[2] == grid[4] && grid[2] == grid[6]) {
        return grid[2];
    }
    
    return 9; // No winner
}
void handle_txt(struct client_info *args,int sockfd){
    if (strncmp(args->buffer,"0x04 Hello",8) == 0 ){                //See if message is "Hello"
            printf("In connection\n");
            int i;
            for (i = 0; i < MAX_CLIENTS; i++){                         //Check if client is already connected 
                if (clients[i].addr_rec != NULL && memcmp(clients[i].addr_rec, args->addr_rec, sizeof(struct sockaddr)) == 0 && clients[i].usedflag == 1) {
                    printf("Client already connected!\n");
                    free(args->addr_rec);
                    free(args);
                    break;
                }
                if (clients[i].usedflag == 0) { //Connect the client and send welcome message
                    args->usedflag = 1;
                    clients[i] = *args;
                    clients[i].player = i;
                    char welcome_msg[1024];
                    snprintf(welcome_msg, 1024, "0x04 Welcome player %d!\n", i+1);
                    int sent = sendto(sockfd, welcome_msg, strlen(welcome_msg), 0, args->addr_rec, args->addr_rec_len);
                        if (sent < args->rec)
                        {
                            perror("sendto");
                        }
                    //pthread_create(&threads[i], NULL, request, (void *) &clients[i]);
                    break;
                }
            }
            if (i == MAX_CLIENTS){
                printf("Max number of clients has been reached!\n");
                char max_msg[1024];
                    snprintf(max_msg, 1024, "0x04 Sorry, only 2 can play this...\n");
                    int sent = sendto(sockfd, max_msg, strlen(max_msg), 0, args->addr_rec, args->addr_rec_len);
                        if (sent < args->rec)
                        {
                            perror("sendto");
                        }
            }
        }else{
            if(strncmp(args->buffer,"0x04 Disconnect",14) == 0 ){ //handle client disconnection 
                printf("In disconnection\n");
                int i;
                for (i = 0; i < MAX_CLIENTS; i++){   
                    if (clients[i].addr_rec != NULL && memcmp(clients[i].addr_rec, args->addr_rec, sizeof(struct sockaddr)) == 0) {
                        char goodbye_msg[1024];
                        snprintf(goodbye_msg, 1024, "0x04 Come back soon!\n");
                        int sent = sendto(sockfd, goodbye_msg, strlen(goodbye_msg), 0, args->addr_rec, args->addr_rec_len);
                        if (sent < args->rec)
                        {
                            perror("sendto");
                        }
                        clients[i].usedflag = 0;
                        free(args);
                        threads[i]=0;
                        printf("Client disconnected!\n");
                        break;
                    }
                }
            }
        }

}
void handle_end(int sockfd,int grid[]){
    char end[1024];
    snprintf(end, 1024, "0x03 Winner is Player %d!\n", checkWin(grid) + 1);
    char disc[1024];
    snprintf(disc, 1024, "0x04 You have been disconnectedDDDD!\n");
    int win;
        for (win = 0; win < 2; win++){
            int sent = sendto(sockfd, end, strlen(end), 0, clients[win].addr_rec, clients[win].addr_rec_len);
                if (sent < clients[win].rec)
                {
                    perror("sendto");
                    return NULL;
                }
            int sent1 = sendto(sockfd, disc, strlen(disc), 0, clients[win].addr_rec, clients[win].addr_rec_len);
                if (sent1 < clients[win].rec)
                {
                    perror("sendto");
                    return NULL;
                }
        }
}
void handle_draw(int sockfd){
    char draw_msg[1024];
    snprintf(draw_msg, 1024, "0x03 It's a draw!\n");
    char disc[1024];
    snprintf(disc, 1024, "0x04 You have been disconnected!\n");
    int draw;
    for(draw = 0; draw < 2; draw++){
    int sent = sendto(sockfd, draw_msg, strlen(draw_msg), 0, clients[draw].addr_rec, clients[draw].addr_rec_len);
        if (sent < clients[draw].rec)
        {
            perror("sendto");
            return NULL;
        }
        
    int sent1 = sendto(sockfd, disc, strlen(disc), 0, clients[draw].addr_rec, clients[draw].addr_rec_len);
        if (sent1 < clients[draw].rec)
        {
            perror("sendto");
            return NULL;
        }
    }
}
void handle_mym(int game_turn, int sockfd){
    char info_msg[1024];
    snprintf(info_msg, 1024, "0x02\n");
    int sent = sendto(sockfd, info_msg, strlen(info_msg), 0, clients[game_turn].addr_rec, clients[game_turn].addr_rec_len);
    if (sent < 0)
    {
        printf("some rando error\n");
    }
}
void handle_mov(struct client_info *args,int sockfd,struct game *game,int gotans,char* buffer){//add error if wrong player tries to move !!!!!!!!!!!!!
            
            size_t length = strlen(buffer);
            printf("Length of the string: %zu\n", length);
            if(strlen(buffer) < 6 || atoi(buffer + 4) < 0 || atoi(buffer + 4) > 8){
                char put_pos[1024];
                snprintf(put_pos, 1024, "0x04 Enter a valid move command (MOV {number of cell})\n");
                int sent = sendto(sockfd, put_pos, strlen(put_pos), 0, clients[game->turn].addr_rec, clients[game->turn].addr_rec_len);
                if (sent < clients[game->turn].rec)
                {
                    perror("sendto");
                    return 0;
                }
            }else{
                int move = atoi(buffer + 4);
                if (game->grid[move] != 9){
                        char taken[1024];
                        snprintf(taken, 1024, "0x04 This cell is taken, choose another one!\n");
                        int sent = sendto(sockfd, taken, strlen(taken), 0, clients[game->turn].addr_rec, clients[game->turn].addr_rec_len);
                        if (sent < clients[game->turn].rec)
                        {
                            perror("sendto");
                            return 0;
                        }
                }else{
                    game->grid[move] = game->turn;
                    gotans = 1;
                    game->free_spots--;
                    game->turn = !(game->turn);
                }
            }
        
}
void send_board(int board[9],int sockfd,int game_turn){
    char board_state[1024];
    snprintf(board_state, 1024, "0x01 %d%d%d%d%d%d%d%d%d\n", board[0],board[1],board[2],board[3],board[4],board[5],board[6],board[07],board[8]);
    int sent = sendto(sockfd, board_state, strlen(board_state), 0, clients[game_turn].addr_rec, clients[game_turn].addr_rec_len);
    if (sent < clients[game_turn].rec)
    {
        perror("sendto");
    }
}
int main(int argc, char* argv[]) {

    pthread_mutex_init(&lock,NULL);

    if (argc < 2) {
        fprintf(stderr, "The function takes one argument: PORT_NUMBER.\n");
        return 1;
    }

    char *port_str = argv[1];
    int port = strtol(port_str, NULL, 10);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in serverAddr, clientAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) < 0) {
        perror("bind");
        close(sockfd);
        return 1;
    }
    socklen_t addr_size = sizeof(clientAddr);

    struct game *game = malloc(sizeof(struct game));

    int gotans = 0;
    int game_going = 0;
    int not_won = 1;
    game->turn = 0;
    game->free_spots = 9;
    int c1;
    for(c1 = 0; c1 < 9; c1++){
        game->grid[c1] = 9;
    }
    while (1) {
            //initialize game
            //START OF THE GAME
        char info_msg[1024];
        char board_state[1024];
        if (game_going == 1 ){
            if (checkWin(game->grid) != 9){
                send_board(game->grid, sockfd, game->turn);
                send_board(game->grid, sockfd, !(game->turn));
                handle_end(sockfd,game->grid);
                clients[0].usedflag = 0;
                clients[1].usedflag = 0;
                game->turn = 0;
                game->free_spots = 9;
                int c2;
                for(c2 = 0; c2 < 9; c2++){
                    game->grid[c2] = 9;
                }
                
                game_going = 0;
                not_won = 1;


            }
            if(game->free_spots == 0){
                handle_draw(sockfd);
            }
            if (not_won && game->free_spots > 0){
                send_board(game->grid, sockfd, game->turn);
                handle_mym(game->turn, sockfd);
                
            }
        }
        gotans = 0;
        char *buffer = malloc(MAX_LINE_LEN * sizeof(char));
        int recv_bytes = recvfrom(sockfd, buffer, MAX_LINE_LEN, 0, (struct sockaddr *) &clientAddr, &addr_size);
        if (recv_bytes==-1) {
            perror("recvfrom");
            free(buffer);
            return 0;
        }
        buffer[recv_bytes] = '\0'; // Null-terminate the received string
        printf("Received message: %s\n", buffer);
        struct client_info *args = malloc(sizeof(struct client_info));
        args->addr_rec = malloc(sizeof(struct sockaddr));
        memcpy(args->addr_rec, &clientAddr, sizeof(clientAddr));
        args->addr_rec_len = addr_size;
        args->buffer = buffer;
        args->rec = recv_bytes;
        args->player = game->turn;
        printf("%s\n",buffer);
        pthread_mutex_lock(&lock);
        int i; 
        if (strncmp(buffer,"0x04",3) == 0 && !game_going){
            handle_txt(args,sockfd);
        }
        if(strncmp(buffer, "0x05", 3) == 0 && game_going && memcmp(clients[game->turn].addr_rec, args->addr_rec, sizeof(struct sockaddr)) == 0 ){ //add error if wrong player tries to move !!!!!!!!!!!!!
            handle_mov(args,sockfd, game,gotans,buffer);
        }
        if(clients[0].usedflag == 1 && clients[1].usedflag == 1 && game_going == 0){
            game_going = 1;
            gotans = 1;
            int j;
            for (j = 0; j < 2; j++){
                char game_starts[1024];
                snprintf(game_starts, 1024, "0x04 Starting game!\n");
                int sent = sendto(sockfd, game_starts, strlen(game_starts), 0, clients[j].addr_rec, clients[j].addr_rec_len);
                    if (sent < clients[j].rec)
                    {
                        perror("sendto");
                        return 0;
                    }
            }
        }
    }
    printf("Leaving main!\n");
    close(sockfd);
    return 0;
}
