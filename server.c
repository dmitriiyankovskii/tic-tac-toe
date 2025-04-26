#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>

#define PORT 1234
#define BOARD_SIZE 3

typedef struct {
    int board[BOARD_SIZE][BOARD_SIZE];
    int current_player;
    int winner;
} GameState;

void print_board(int board[BOARD_SIZE][BOARD_SIZE]) {
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
    printf("\n");
}

int check_winner(int board[BOARD_SIZE][BOARD_SIZE]) {
    // Проверка строк и столбцов
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (board[i][0] != 0 && board[i][0] == board[i][1] && board[i][0] == board[i][2])
            return board[i][0];
        if (board[0][i] != 0 && board[0][i] == board[1][i] && board[0][i] == board[2][i])
            return board[0][i];
    }

    // Проверка диагоналей
    if (board[0][0] != 0 && board[0][0] == board[1][1] && board[0][0] == board[2][2])
        return board[0][0];
    if (board[0][2] != 0 && board[0][2] == board[1][1] && board[0][2] == board[2][0])
        return board[0][2];

    // Проверка на ничью
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            if (board[i][j] == 0)
                return 0; // Игра продолжается

    return -1; // Ничья
}

int main() {
    int server_fd, client_socket1, client_socket2;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    GameState game;
    memset(&game, 0, sizeof(game));
    game.current_player = 1;

    printf("Initializing server...\n");

    // Создание сокета
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Настройка сокета
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Привязка сокета к порту
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Ожидание подключений
    if (listen(server_fd, 2) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for players...\n");

    // Подключение первого игрока
    if ((client_socket1 = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    printf("Player 1 (X) connected!\n");
    int player1_id = 1;
    send(client_socket1, &player1_id, sizeof(player1_id), 0); // Отправляем номер игрока

    // Подключение второго игрока (O)
    if ((client_socket2 = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    printf("Player 2 (O) connected!\n");
    int player2_id = 2;
    send(client_socket2, &player2_id, sizeof(player2_id), 0);

    printf("Game started!\n");

    // Отправляем начальное состояние обоим игрокам
    send(client_socket1, &game, sizeof(game), 0);
    send(client_socket2, &game, sizeof(game), 0);

    // Главный игровой цикл
    while (1) {
        int move[2];
        int current_socket = (game.current_player == 1) ? client_socket1 : client_socket2;

        // Получаем ход от текущего игрока
        int bytes_received = recv(current_socket, &move, sizeof(move), 0);
        if (bytes_received <= 0) {
            printf("Player %d disconnected!\n", game.current_player);
            break;
        }

        // Проверяем валидность хода
        if (move[0] < 0 || move[0] >= BOARD_SIZE || move[1] < 0 || move[1] >= BOARD_SIZE || game.board[move[0]][move[1]] != 0) {
            printf("Invalid move from player %d!\n", game.current_player);
            continue;
        }

        // Обновляем доску
        game.board[move[0]][move[1]] = game.current_player;
        game.winner = check_winner(game.board);

        // Проверяем конец игры
        if (game.winner != 0) {
            printf("Game over! Winner: %d\n", game.winner);
            send(client_socket1, &game, sizeof(game), 0);
            send(client_socket2, &game, sizeof(game), 0);
            break;
        }

        // Меняем игрока
        game.current_player = (game.current_player == 1) ? 2 : 1;

        // Отправляем обновлённое состояние
        send(client_socket1, &game, sizeof(game), 0);
        send(client_socket2, &game, sizeof(game), 0);

        print_board(game.board);
    }

    // Определение результата
    if (game.winner == -1) {
        printf("Game ended in a draw!\n");
    } else {
        printf("Player %d wins!\n", game.winner);
    }

    close(client_socket1);
    close(client_socket2);
    close(server_fd);
    return 0;
}