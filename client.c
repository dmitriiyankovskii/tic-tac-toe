#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 1234
#define BOARD_SIZE 3

typedef struct {
    int board[BOARD_SIZE][BOARD_SIZE];
    int current_player;
    int winner;
} GameState;

void print_board(int board[BOARD_SIZE][BOARD_SIZE], int player_id) {
    printf("\n");
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            char c;
            if (board[i][j] == 0) c = '-';
            else if (board[i][j] == 1) c = 'X';
            else c = 'O';
            printf("%c ", c);
        }
        printf("\n");
    }
    printf("You are %c\n", (player_id == 1) ? 'X' : 'O');
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <server_ip>\n", argv[0]);
        return 1;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0) {
        printf("Invalid address\n");
        return 1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection failed\n");
        return 1;
    }

    // Получаем номер игрока (1 или 2)
    int player_id;
    recv(sock, &player_id, sizeof(player_id), 0);
    printf("You are Player %d (%c)\n", player_id, (player_id == 1) ? 'X' : 'O');

    GameState game;
    while (1) {
        // Получаем текущее состояние игры
        recv(sock, &game, sizeof(game), 0);

        // Проверяем конец игры
        if (game.winner != 0) {
            if (game.winner == -1) {
                printf("Game ended in a draw!\n");
            } else if (game.winner == player_id) {
                printf("You win!\n");
            } else {
                printf("Player %d wins!\n", game.winner);
            }
            break;
        }

        print_board(game.board, player_id);

        // Если сейчас наш ход
        if (game.current_player == player_id) {
            printf("Your turn. Enter row and column (0-2): ");
            int x, y;
            scanf("%d %d", &x, &y);

            // Отправляем ход на сервер
            int move[2] = {x, y};
            send(sock, move, sizeof(move), 0);
        } else {
            printf("Waiting for opponent's move...\n");
        }
    }

    close(sock);
    return 0;
}