#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_NEIGHBORS 4

// A structure for BFS queue entries.
typedef struct {
    int x;
    int y;
    int color;
} Triple;

// Shuffle an integer array in place.
void shuffle_int_array(int *arr, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

// Generate an n x n board with exactly n connected color regions.
// Each region is grown from a distinct seed cell (color 1..n).
// If more than max_singular regions are single-cell, returns NULL.
int *generate_board_with_n_colors(int n, int max_singular) {
    int total = n * n;
    int *board = calloc(total, sizeof(int));  // board[i*n+j]
    if (!board) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }
    
    // Create an array of all cell indices and shuffle.
    int *indices = malloc(total * sizeof(int));
    if (!indices) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < total; i++) {
        indices[i] = i;
    }
    shuffle_int_array(indices, total);
    
    // Use first n indices as seed positions.
    Triple *queue = malloc(total * sizeof(Triple));
    if (!queue) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    int q_start = 0, q_end = 0;
    
    for (int c = 1; c <= n; c++) {
        int idx = indices[c - 1];
        int x = idx / n;
        int y = idx % n;
        board[x * n + y] = c;
        queue[q_end].x = x;
        queue[q_end].y = y;
        queue[q_end].color = c;
        q_end++;
    }
    free(indices);
    
    // Define neighbor offsets (4 directions)
    int dx[MAX_NEIGHBORS] = {-1, 1,  0, 0};
    int dy[MAX_NEIGHBORS] = { 0, 0, -1, 1};
    
    // Multi-source BFS expansion.
    while (q_start < q_end) {
        // For variety, shuffle the current queue slice.
        int len = q_end - q_start;
        for (int i = q_start; i < q_end; i++) {
            int j = q_start + rand() % len;
            Triple temp = queue[i];
            queue[i] = queue[j];
            queue[j] = temp;
        }
        
        int currentCount = q_end - q_start;
        for (int i = 0; i < currentCount; i++) {
            Triple cur = queue[q_start++];
            for (int d = 0; d < MAX_NEIGHBORS; d++) {
                int nx = cur.x + dx[d];
                int ny = cur.y + dy[d];
                if (nx >= 0 && nx < n && ny >= 0 && ny < n) {
                    if (board[nx * n + ny] == 0) {
                        board[nx * n + ny] = cur.color;
                        queue[q_end].x = nx;
                        queue[q_end].y = ny;
                        queue[q_end].color = cur.color;
                        q_end++;
                    }
                }
            }
        }
    }
    free(queue);
    
    // Check for forbidden single-cell regions.
    int *regionCount = calloc(n+1, sizeof(int));
    if (!regionCount) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < total; i++) {
        int col = board[i];
        if (col >= 1 && col <= n)
            regionCount[col]++;
    }
    int singletons = 0;
    for (int c = 1; c <= n; c++) {
        if (regionCount[c] == 1)
            singletons++;
    }
    free(regionCount);
    if (singletons > max_singular) {
        free(board);
        return NULL;
    }
    return board;
}

// Backtracking solver globals are passed via parameters.
void backtrack(int row, int n, int *board, int *current_sol, int *sol_count, int *best_sol,
               char *used_cols, char *used_diag1, char *used_diag2, char *used_color) {
    if (row == n) {
        (*sol_count)++;
        if (*sol_count == 1) {
            for (int i = 0; i < n; i++) {
                best_sol[i] = current_sol[i];
            }
        }
        return;
    }
    for (int col = 0; col < n; col++) {
        int color = board[row * n + col];
        int d1 = row - col + (n - 1);  // index offset for diag1
        int d2 = row + col;            // index for diag2
        if (used_cols[col] || used_diag1[d1] || used_diag2[d2] || used_color[color])
            continue;
        // Place queen.
        current_sol[row] = col;
        used_cols[col] = 1;
        used_diag1[d1] = 1;
        used_diag2[d2] = 1;
        used_color[color] = 1;
        backtrack(row + 1, n, board, current_sol, sol_count, best_sol,
                  used_cols, used_diag1, used_diag2, used_color);
        if (*sol_count > 1) return; // early exit if more than one solution
        // Remove queen.
        used_cols[col] = 0;
        used_diag1[d1] = 0;
        used_diag2[d2] = 0;
        used_color[color] = 0;
    }
}

// Returns 1 if exactly one solution is found; solution is stored in best_sol (array of n ints).
// Otherwise returns 0.
int solve_unique(int *board, int n, int *best_sol) {
    int sol_count = 0;
    int *current_sol = malloc(n * sizeof(int));
    if (!current_sol) { perror("malloc"); exit(EXIT_FAILURE); }
    
    // Allocate helper arrays.
    char *used_cols = calloc(n, sizeof(char));
    char *used_diag1 = calloc(2 * n - 1, sizeof(char));
    char *used_diag2 = calloc(2 * n - 1, sizeof(char));
    // Colors are in range [1, n]. Use an array of size n+1.
    char *used_color = calloc(n+1, sizeof(char));
    if (!used_cols || !used_diag1 || !used_diag2 || !used_color) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }
    
    backtrack(0, n, board, current_sol, &sol_count, best_sol,
              used_cols, used_diag1, used_diag2, used_color);
    
    free(current_sol);
    free(used_cols);
    free(used_diag1);
    free(used_diag2);
    free(used_color);
    
    return (sol_count == 1);
}

// Checks if board has a unique queen solution.
// If yes, returns 1 and fills best_sol; otherwise returns 0.
int isitKosher(int *board, int n, int *best_sol) {
    return solve_unique(board, n, best_sol);
}

// Repeatedly generate boards until one is "kosher" (i.e. has a unique queen solution).
// Returns a valid board (dynamically allocated) and fills best_sol with the queen solution.
int *generate_valid_board(int n, int max_singular, int *best_sol) {
    int *board = NULL;
    while (1) {
        board = generate_board_with_n_colors(n, max_singular);
        if (board == NULL)
            continue;
        if (isitKosher(board, n, best_sol))
            return board;
        free(board);
    }
}

void print_board(int *board, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            printf("%2d ", board[i * n + j]);
        }
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s n [max_singular]\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    int n = atoi(argv[1]);
    if (n <= 0) {
        fprintf(stderr, "n must be a positive integer.\n");
        return EXIT_FAILURE;
    }
    
    int max_singular = 1;
    if (argc >= 3) {
        max_singular = atoi(argv[2]);
    }
    
    srand(time(NULL)); // initialize random seed
    
    int *best_sol = malloc(n * sizeof(int));
    if (!best_sol) { perror("malloc"); exit(EXIT_FAILURE); }
    
    clock_t start = clock();
    int *board = generate_valid_board(n, max_singular, best_sol);
    clock_t end = clock();
    
    printf("Board:\n");
    print_board(board, n);
    printf("\nUnique queen solution (row, col):\n");
    for (int i = 0; i < n; i++) {
        printf("(%d, %d) ", i, best_sol[i]);
    }
    printf("\n");
    double time_taken = (double)(end - start) / CLOCKS_PER_SEC;
    printf("\nTime taken to generate board: %.4f seconds\n", time_taken);
    
    free(board);
    free(best_sol);
    return 0;
}

