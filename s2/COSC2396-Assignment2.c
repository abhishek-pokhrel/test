#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <string.h>

/* Board */
#define EMPTY ' '
#define ROWS  3
#define COLS  3

char board[ROWS][COLS];        

/* Global state */
pid_t child_pid  = -1;         
pid_t parent_pid = -1;         

volatile sig_atomic_t child_move_mark = 'X';   /* mark child wants to place */
volatile sig_atomic_t got_signal      = 0;     /* signal received flag */
volatile sig_atomic_t game_over_flag  = 0;     /* set when parent sends SIGUSR2 to child */

/* Utilities */

/* Print the board */
void print_board(void) {
    printf("\n");
    printf("  0   1   2\n");
    for (int i = 0; i < ROWS; i++) {
        printf("%d ", i);
        for (int j = 0; j < COLS; j++) {
            printf(" %c ", board[i][j]);
            if (j < COLS - 1) printf("|");
        }
        printf("\n");
        if (i < ROWS - 1) printf("  ---|---|---\n");
    }
    printf("\n");
}

/* Initialise every cell to EMPTY */
void init_board(void) {
    for (int i = 0; i < ROWS; i++)
        for (int j = 0; j < COLS; j++)
            board[i][j] = EMPTY;
}

/* Return 'X', 'O' if there is a winner, 'D' for draw, or 0 for game ongoing */
char check_winner(void) {
    /* Rows and columns */
    for (int i = 0; i < 3; i++) {
        if (board[i][0] != EMPTY &&
            board[i][0] == board[i][1] &&
            board[i][1] == board[i][2])
            return board[i][0];

        if (board[0][i] != EMPTY &&
            board[0][i] == board[1][i] &&
            board[1][i] == board[2][i])
            return board[0][i];
    }
    /* Diagonals */
    if (board[0][0] != EMPTY &&
        board[0][0] == board[1][1] &&
        board[1][1] == board[2][2])
        return board[0][0];

    if (board[0][2] != EMPTY &&
        board[0][2] == board[1][1] &&
        board[1][1] == board[2][0])
        return board[0][2];

    /* Check draw */
    int filled = 0;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            if (board[i][j] != EMPTY) filled++;
    if (filled == 9) return 'D';

    return 0;   /* game still going */
}

/*
 * Find a free cell starting from random index r.
 * Returns 1 on success (fills *row, *col), 0 if board is full.
 */
int find_free_cell(int r, int *row, int *col) {
    for (int k = 0; k < 9; k++) {
        int idx = (r + k) % 9;
        int i = idx / 3, j = idx % 3;
        if (board[i][j] == EMPTY) {
            *row = i; *col = j;
            return 1;
        }
    }
    return 0;   /* board full */
}

/* Child signal handlers */

/*
 * Child receives SIGUSR1 from parent -> "your turn".
 * Child receives SIGUSR2 from parent -> "game over".
 */
void child_sigusr1_handler(int sig) {
    (void)sig;
    got_signal = 1;          
}

void child_sigusr2_handler(int sig) {
    (void)sig;
    game_over_flag = 1;      
    got_signal     = 1;
}

/* Parent signal handlers */

/*
 * Parent receives SIGUSR1 -> child wants to place X.
 * Parent receives SIGUSR2 -> child wants to place O.
 */
void parent_sigusr1_handler(int sig) {
    (void)sig;
    child_move_mark = 'X';
    got_signal      = 1;
}

void parent_sigusr2_handler(int sig) {
    (void)sig;
    child_move_mark = 'O';
    got_signal      = 1;
}

/* Install a sigaction handler */
void install_handler(int sig, void (*handler)(int)) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(sig, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

/* Spin-wait until a signal sets got_signal */
void wait_for_signal(void) {
    got_signal = 0;
    while (!got_signal)
        pause();   /* sleep until any signal arrives */
}

 /* PARENT PROCESS */

void run_parent(void) {
    srand((unsigned)getpid());

    /* Install handlers */
    install_handler(SIGUSR1, parent_sigusr1_handler);
    install_handler(SIGUSR2, parent_sigusr2_handler);

    int move_count = 0;   /* track total moves on the board */

    /* P0: Initialise board */
    init_board();
    printf("[Parent PID=%d] Game started!\n", getpid());
    print_board();

    /* Main game loop */
    while (1) {

        /* Parent chooses a cell and a mark */
        int r   = rand() % 9;
        int row, col;
        if (!find_free_cell(r, &row, &col)) {
            /* Board full should have been caught by check_winner, but safety net */
            break;
        }

        /* Mark depends on current time parity */
        char my_mark = (time(NULL) % 2 == 0) ? 'X' : 'O';
        board[row][col] = my_mark;
        move_count++;

        printf("[Parent] Places '%c' at (%d,%d)\n", my_mark, row, col);
        print_board();

        /* Check winner after parent's move */
        char result = check_winner();
        if (result) {
            if (result == 'D')
                printf("*** DRAW! No winner. ***\n");
            else
                printf("*** PARENT WINS with '%c'! ***\n", result);

            /* Notify child: game over */
            kill(child_pid, SIGUSR2);
            /* Wait for child to exit gracefully */
            wait(NULL);
            return;
        }

        /* Tell child it's their turn */
        kill(child_pid, SIGUSR1);

        /* Wait for child's signal (SIGUSR1=X, SIGUSR2=O) */
        wait_for_signal();

        /* Now place child's mark on the board */
        r = rand() % 9;
        if (!find_free_cell(r, &row, &col)) break;

        board[row][col] = child_move_mark;
        move_count++;

        printf("[Parent] Places child's '%c' at (%d,%d)\n", child_move_mark, row, col);
        print_board();

        /* Check winner after child's move */
        result = check_winner();
        if (result) {
            if (result == 'D')
                printf("*** DRAW! No winner. ***\n");
            else
                printf("*** CHILD WINS with '%c'! ***\n", result);

            kill(child_pid, SIGUSR2);
            wait(NULL);
            return;
        }

        /* Loop back for parent's next move */
    }
}

 /* CHILD PROCESS */

void run_child(void) {
    /* Install handlers */
    install_handler(SIGUSR1, child_sigusr1_handler);
    install_handler(SIGUSR2, child_sigusr2_handler);

    printf("[Child  PID=%d] Ready.\n", getpid());

    while (1) {

        /*  Wait for parent to say "your turn" (SIGUSR1) */
        wait_for_signal();

        /* If game_over_flag, handle result and exit */
        if (game_over_flag) {
            char result = check_winner();
            if (result == 'D')
                printf("[Child] It's a draw – I'll take it!\n");
            else
                printf("[Child] I lost this round. GG!\n");

            /* BONUS: Decide whether to request replay */
            if (time(NULL) % 2 == 0) {
                printf("[Child] Requesting a REPLAY (even seconds – sending SIGUSR1 for X)!\n");
                kill(parent_pid, SIGUSR1);
            } else {
                printf("[Child] Requesting a REPLAY (odd seconds – sending SIGUSR2 for O)!\n");
                kill(parent_pid, SIGUSR2);
            }

            /* Terminate self with SIGTERM */
            kill(getpid(), SIGTERM);
            return;   /* should not reach here */
        }

        /* Decide mark from current time */
        char my_mark = (time(NULL) % 2 == 0) ? 'X' : 'O';
        printf("[Child] Decides to place '%c'\n", my_mark);

        if (my_mark == 'X')
            kill(parent_pid, SIGUSR1);
        else
            kill(parent_pid, SIGUSR2);
    }
}

int main(void) {
    parent_pid = getpid();

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        /* Child process */
        run_child();
        exit(EXIT_SUCCESS);
    } else {
        /* Parent process */
        child_pid = pid;
        usleep(50000);   /* 50 ms */

        run_parent();
        exit(EXIT_SUCCESS);
    }
}
