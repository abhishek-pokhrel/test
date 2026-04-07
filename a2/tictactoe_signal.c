#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>

volatile sig_atomic_t child_request = 0;   // 1 child = X, other child wants = 0

char board[3][3];
pid_t child_pid = -1;

void init_board(void) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            board[i][j] = ' ';
        }
    }
}

void print_board(void) {
    printf("\n");
    for (int i = 0; i < 3; i++) {
        printf(" %c | %c | %c \n", board[i][0], board[i][1], board[i][2]);
        if (i < 2) {
            printf("-----------\n");
        }
    }
    printf("\n");
    fflush(stdout);
}

int is_full(void) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (board[i][j] == ' ') {
                return 0;
            }
        }
    }
    return 1;
}

char check_winner(void) {
    for (int i = 0; i < 3; i++) {
        if (board[i][0] != ' ' && board[i][0] == board[i][1] && board[i][1] == board[i][2]) {
            return board[i][0];
        }
        if (board[0][i] != ' ' && board[0][i] == board[1][i] && board[1][i] == board[2][i]) {
            return board[0][i];
        }
    }

    if (board[0][0] != ' ' && board[0][0] == board[1][1] && board[1][1] == board[2][2]) {
        return board[0][0];
    }

    if (board[0][2] != ' ' && board[0][2] == board[1][1] && board[1][1] == board[2][0]) {
        return board[0][2];
    }

    return ' ';
}

void place_mark_random(char mark, const char *who) {
    int r = rand() % 9;

    for (int step = 0; step < 9; step++) {
        int idx = (r + step) % 9;
        int i = idx / 3;
        int j = idx % 3;

        if (board[i][j] == ' ') {
            board[i][j] = mark;
            printf("%s placed %c at (%d,%d)\n", who, mark, i, j);
            fflush(stdout);
            return;
        }
    }
}

char mark_from_current_time(time_t now) {
    if (now % 2 == 0) {
        return 'X';
    }
    return 'O';
}

void handle_parent_sigusr1(int sig) {
    child_request = 1;
}

void handle_parent_sigusr2(int sig) {
    child_request = 2;
}

void handle_child_sigusr1(int sig) {
    time_t now = time(NULL);
    char mark = mark_from_current_time(now);

    printf("child received move request at time %ld and chosed %c\n", (long)now, mark);
    fflush(stdout);

    if (mark == 'X') {
        kill(getppid(), SIGUSR1);
    } else {
        kill(getppid(), SIGUSR2);
    }
}

void handle_child_sigusr2(int sig) {
    printf("received game finsihed signal from parent.\n");
    printf("closing\n");
    fflush(stdout);
    kill(getpid(), SIGTERM);
}

int main(void) {
    srand((unsigned int)(time(NULL) ^ getpid()));
    init_board();

    child_pid = fork();

    if (child_pid < 0) {
        perror("fork failed");
        return 1;
    }

    if (child_pid == 0) {
        struct sigaction sa_move;
        struct sigaction sa_gameover;

        memset(&sa_move, 0, sizeof(sa_move));
        memset(&sa_gameover, 0, sizeof(sa_gameover));

        sa_move.sa_handler = handle_child_sigusr1;
        sa_gameover.sa_handler = handle_child_sigusr2;

        sigaction(SIGUSR1, &sa_move, NULL);
        sigaction(SIGUSR2, &sa_gameover, NULL);

        while (1) {
            pause();
        }
    }

    struct sigaction sa_x;
    struct sigaction sa_o;

    memset(&sa_x, 0, sizeof(sa_x));
    memset(&sa_o, 0, sizeof(sa_o));

    sa_x.sa_handler = handle_parent_sigusr1;
    sa_o.sa_handler = handle_parent_sigusr2;

    sigaction(SIGUSR1, &sa_x, NULL);
    sigaction(SIGUSR2, &sa_o, NULL);

    while (1) {
        time_t now = time(NULL);
        char parent_mark = mark_from_current_time(now);

        printf("parent deciding move at time %ld -> %c\n", (long)now, parent_mark);
        place_mark_random(parent_mark, "Parent");
        print_board();

        char winner = check_winner();
        if (winner != ' ') {
            printf(" game winner is %c (after the parent move)\n", winner);
            kill(child_pid, SIGUSR2);
            break;
        }

        if (is_full()) {
            printf("game drawed.\n");
            kill(child_pid, SIGUSR2);
            break;
        }

        sleep(1);  

        printf("parent asks child for next move.\n");
        fflush(stdout);
        kill(child_pid, SIGUSR1);

        child_request = 0;
        while (child_request == 0) {
            pause();
        }

        char child_mark = (child_request == 1) ? 'X' : 'O';
        place_mark_random(child_mark, "Child");
        print_board();

        winner = check_winner();
        if (winner != ' ') {
            printf("game winner is %c (after child move)\n", winner);
            kill(child_pid, SIGUSR2);
            break;
        }

        if (is_full()) {
            printf("game drawed.\n");
            kill(child_pid, SIGUSR2);
            break;
        }

        sleep(1);  // pause before next parent move happenes
    }

    wait(NULL);
    return 0;
}
