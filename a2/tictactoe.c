#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <time.h>
#include <string.h>



typedef struct {
    char board[3][3];
    int winner_player;      /* 0 = none/draw, 1 = parent, 2 = child */
    char winner_mark;       /* 'X', 'O', or ' ' */
    int draw;               /* 1 if draw, else 0 */
} SharedState;

static SharedState *shared = NULL;
static pid_t child_pid = -1;

/* Parent-side flag:
   1 = child chose X (received SIGUSR1)
   2 = child chose O (received SIGUSR2)
*/
static volatile sig_atomic_t child_request = 0;

void init_board(void) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            shared->board[i][j] = ' ';
        }
    }
    shared->winner_player = 0;
    shared->winner_mark = ' ';
    shared->draw = 0;
}

void print_board(void) {
    printf("\n");
    for (int i = 0; i < 3; i++) {
        printf(" %c | %c | %c \n",
               shared->board[i][0], shared->board[i][1], shared->board[i][2]);
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
            if (shared->board[i][j] == ' ') {
                return 0;
            }
        }
    }
    return 1;
}

char check_winner_mark(void) {
    for (int i = 0; i < 3; i++) {
        if (shared->board[i][0] != ' ' &&
            shared->board[i][0] == shared->board[i][1] &&
            shared->board[i][1] == shared->board[i][2]) {
            return shared->board[i][0];
        }

        if (shared->board[0][i] != ' ' &&
            shared->board[0][i] == shared->board[1][i] &&
            shared->board[1][i] == shared->board[2][i]) {
            return shared->board[0][i];
        }
    }

    if (shared->board[0][0] != ' ' &&
        shared->board[0][0] == shared->board[1][1] &&
        shared->board[1][1] == shared->board[2][2]) {
        return shared->board[0][0];
    }

    if (shared->board[0][2] != ' ' &&
        shared->board[0][2] == shared->board[1][1] &&
        shared->board[1][1] == shared->board[2][0]) {
        return shared->board[0][2];
    }

    return ' ';
}

char mark_from_current_time(time_t now) {
    return (now % 2 == 0) ? 'X' : 'O';
}

void place_mark_random(char mark, const char *player_name) {
    int r = rand() % 9;
    int i, j;

    for (int step = 0; step < 9; step++) {
        int idx = (r + step) % 9;
        i = idx / 3;
        j = idx % 3;

        if (shared->board[i][j] == ' ') {
            shared->board[i][j] = mark;
            printf("%s placed %c at board[%d][%d]\n", player_name, mark, i, j);
            fflush(stdout);
            return;
        }
    }
}

/* paren signal handler*/

void parent_receive_x(int sig) {
    (void)sig;
    child_request = 1;
}

void parent_receive_o(int sig) {
    (void)sig;
    child_request = 2;
}

/* child signal handler */

void child_decide_move(int sig) {
    (void)sig;

    time_t now = time(NULL);
    char mark = mark_from_current_time(now);

    printf("Child received SIGUSR1 from parent.\n");
    printf("Child checked current time: %ld seconds\n", (long)now);
    printf("Child decided next mark should be %c\n", mark);
    fflush(stdout);

    if (mark == 'X') {
        kill(getppid(), SIGUSR1);
    } else {
        kill(getppid(), SIGUSR2);
    }
}

void child_game_over(int sig) {
    (void)sig;

    printf("Child received SIGUSR2 from parent. Game over.\n");

    if (shared->draw) {
        printf("Child result: draw.\n");
    } else if (shared->winner_player == 2) {
        printf("Child result: I won the game with %c.\n", shared->winner_mark);
    } else if (shared->winner_player == 1) {
        printf("Child result: I lost the game. Parent won with %c.\n", shared->winner_mark);
    } else {
        printf("Child result: Final result could not be determined.\n");
    }

    printf("Child closes by sending SIGTERM to itself.\n");
    fflush(stdout);

    kill(getpid(), SIGTERM);
}


int main(void) {
    shared = mmap(NULL, sizeof(SharedState),
                  PROT_READ | PROT_WRITE,
                  MAP_SHARED | MAP_ANONYMOUS,
                  -1, 0);

    if (shared == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }

    init_board();
    srand((unsigned int)(time(NULL) ^ getpid()));

    child_pid = fork();

    if (child_pid < 0) {
        perror("fork failed");
        munmap(shared, sizeof(SharedState));
        return 1;
    }

    if (child_pid == 0) {
        struct sigaction sa_move, sa_over;

        memset(&sa_move, 0, sizeof(sa_move));
        memset(&sa_over, 0, sizeof(sa_over));

        sa_move.sa_handler = child_decide_move;
        sa_over.sa_handler = child_game_over;

        sigaction(SIGUSR1, &sa_move, NULL);
        sigaction(SIGUSR2, &sa_over, NULL);

        while (1) {
            pause();
        }
    }

    /* parent process */
    struct sigaction sa_x, sa_o;

    memset(&sa_x, 0, sizeof(sa_x));
    memset(&sa_o, 0, sizeof(sa_o));

    sa_x.sa_handler = parent_receive_x;
    sa_o.sa_handler = parent_receive_o;

    sigaction(SIGUSR1, &sa_x, NULL);
    sigaction(SIGUSR2, &sa_o, NULL);

    printf("Parent created child process with PID %d\n", child_pid);
    printf("Initial empty board:\n");
    print_board();

    while (1) {
        /* parent move */
        time_t now = time(NULL);
        char parent_mark = mark_from_current_time(now);

        printf("Parent checked current time: %ld seconds\n", (long)now);
        printf("Parent decided to place %c\n", parent_mark);
        place_mark_random(parent_mark, "Parent");
        print_board();

        char winner = check_winner_mark();
        if (winner != ' ') {
            shared->winner_player = 1;
            shared->winner_mark = winner;
            shared->draw = 0;

            printf("Game result: Parent wins with %c\n", winner);
            fflush(stdout);
            kill(child_pid, SIGUSR2);
            break;
        }

        if (is_full()) {
            shared->winner_player = 0;
            shared->winner_mark = ' ';
            shared->draw = 1;

            printf("Game result: Draw\n");
            fflush(stdout);
            kill(child_pid, SIGUSR2);
            break;
        }

        sleep(1);

        /* asking child to decide next move */
        printf("Parent sends SIGUSR1 to child requesting child's move.\n");
        fflush(stdout);
        kill(child_pid, SIGUSR1);

        child_request = 0;
        while (child_request == 0) {
            pause();
        }

        char child_mark = (child_request == 1) ? 'X' : 'O';
        printf("Parent received child's decision: %c\n", child_mark);
        place_mark_random(child_mark, "Child");
        print_board();

        winner = check_winner_mark();
        if (winner != ' ') {
            shared->winner_player = 2;
            shared->winner_mark = winner;
            shared->draw = 0;

            printf("Game result: Child wins with %c\n", winner);
            fflush(stdout);
            kill(child_pid, SIGUSR2);
            break;
        }

        if (is_full()) {
            shared->winner_player = 0;
            shared->winner_mark = ' ';
            shared->draw = 1;

            printf("Game result: Draw\n");
            fflush(stdout);
            kill(child_pid, SIGUSR2);
            break;
        }

        sleep(1);
    }

    wait(NULL);
    munmap(shared, sizeof(SharedState));
    return 0;
}
