#include <stdio.h>
#include <string.h>



char board[3][3];

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
        if (board[i][0] != ' ' &&
            board[i][0] == board[i][1] &&
            board[i][1] == board[i][2]) {
            return board[i][0];
        }

        if (board[0][i] != ' ' &&
            board[0][i] == board[1][i] &&
            board[1][i] == board[2][i]) {
            return board[0][i];
        }
    }

    if (board[0][0] != ' ' &&
        board[0][0] == board[1][1] &&
        board[1][1] == board[2][2]) {
        return board[0][0];
    }

    if (board[0][2] != ' ' &&
        board[0][2] == board[1][1] &&
        board[1][1] == board[2][0]) {
        return board[0][2];
    }

    return ' ';
}


int place_mark_sequential(int r, char mark) {
    for (int step = 0; step < 9; step++) {
        int idx = (r + step) % 9;
        int i = idx / 3;
        int j = idx % 3;

        if (board[i][j] == ' ') {
            board[i][j] = mark;
            return idx;
        }
    }
    return -1;
}

void print_result(const char *test_name, int passed) {
    if (passed) {
        printf("[PASS] %s\n", test_name);
    } else {
        printf("[FAIL] %s\n", test_name);
    }
}

int test_empty_board(void) {
    init_board();
    return check_winner() == ' ' && is_full() == 0;
}

int test_row_winner(void) {
    init_board();
    board[1][0] = 'X';
    board[1][1] = 'X';
    board[1][2] = 'X';
    return check_winner() == 'X';
}

int test_column_winner(void) {
    init_board();
    board[0][2] = 'O';
    board[1][2] = 'O';
    board[2][2] = 'O';
    return check_winner() == 'O';
}

int test_diagonal_winner(void) {
    init_board();
    board[0][0] = 'X';
    board[1][1] = 'X';
    board[2][2] = 'X';
    return check_winner() == 'X';
}

int test_draw_detection(void) {
    init_board();

    board[0][0] = 'X'; board[0][1] = 'O'; board[0][2] = 'X';
    board[1][0] = 'X'; board[1][1] = 'O'; board[1][2] = 'O';
    board[2][0] = 'O'; board[2][1] = 'X'; board[2][2] = 'X';

    return check_winner() == ' ' && is_full() == 1;
}

int test_sequential_placement(void) {
    init_board();

    board[0][0] = 'X';
    board[0][1] = 'O';
    board[0][2] = 'X';

    int idx = place_mark_sequential(0, 'O');

    return idx == 3 && board[1][0] == 'O';
}

int test_full_board_reject(void) {
    init_board();

    char fill[3][3] = {
        {'X', 'O', 'X'},
        {'O', 'X', 'O'},
        {'O', 'X', 'O'}
    };

    memcpy(board, fill, sizeof(board));

    return place_mark_sequential(4, 'X') == -1;
}

int main(void) {
    int passed = 0;
    int total = 7;

    printf("Running Tic-Tac-Toe test cases...\n");

    if (test_empty_board()) passed++;
    print_result("TC1 - Empty board has no winner and is not full", test_empty_board());

    if (test_row_winner()) passed++;
    print_result("TC2 - Row winner detection works", test_row_winner());

    if (test_column_winner()) passed++;
    print_result("TC3 - Column winner detection works", test_column_winner());

    if (test_diagonal_winner()) passed++;
    print_result("TC4 - Diagonal winner detection works", test_diagonal_winner());

    if (test_draw_detection()) passed++;
    print_result("TC5 - Draw detection works", test_draw_detection());

    if (test_sequential_placement()) passed++;
    print_result("TC6 - Sequential empty-cell placement works", test_sequential_placement());

    if (test_full_board_reject()) passed++;
    print_result("TC7 - Full board rejects further placement", test_full_board_reject());

    printf("\nSummary: %d/%d test cases passed.\n", passed, total);

    if (passed == total) {
        printf("All core logic test cases passed successfully.\n");
    } else {
        printf("Some test cases failed. Please review the functions.\n");
    }

    return 0;
}
