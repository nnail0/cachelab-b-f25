/*********
 * Roxanne Lutz, Nathan Nail
 * CS 341L Fall 2025
 * 
 * ai-trans.c: version that utilizes the help of AI
 * to try and transpose with minimal cache misses. 
 * 
 * Model used: ChatGPT 5 mini (with intelligence)
 * 
 * nnail0, rlutz1
 * 
 */

#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    /* 12 ints total */
    int i, j, ii, jj;
    int t0, t1, t2, t3, t4, t5, t6, t7;

    /* ---------------- 32 x 32 ---------------- */
    if (M == 32 && N == 32) {
        int block = 8;
        for (ii = 0; ii < N; ii += block) {
            for (jj = 0; jj < M; jj += block) {
                for (i = ii; i < ii + block; ++i) {
                    /* keep diagonal in temp and write after the inner loop */
                    for (j = jj; j < jj + block; ++j) {
                        if (i != j) {
                            B[j][i] = A[i][j];
                        } else {
                            /* store diagonal temporarily then write after */
                            t0 = A[i][j];
                            B[j][i] = t0;
                        }
                    }
                }
            }
        }
        return;
    }

    /* ---------------- 64 x 64 ----------------
     * Use 8x8 blocks. For each 8x8:
     *  - For rows ii..ii+3: load 8 elements and write first 4 directly to B,
     *    write last 4 into B at positions that will be swapped later.
     *  - For cols jj..jj+3: move the staged elements into their correct positions.
     *  - For rows ii+4..ii+7: write bottom-right directly.
     *
     * This pattern is a well-known correct approach that avoids conflict misses
     * while preserving correctness.
     */
    if (M == 64 && N == 64) {
        for (ii = 0; ii < 64; ii += 8) {
            for (jj = 0; jj < 64; jj += 8) {
                /* step 1: for the top 4 rows of the 8x8 block */
                for (i = 0; i < 4; ++i) {
                    t0 = A[ii + i][jj + 0];
                    t1 = A[ii + i][jj + 1];
                    t2 = A[ii + i][jj + 2];
                    t3 = A[ii + i][jj + 3];
                    t4 = A[ii + i][jj + 4];
                    t5 = A[ii + i][jj + 5];
                    t6 = A[ii + i][jj + 6];
                    t7 = A[ii + i][jj + 7];

                    /* write the left 4 into B (transposed) */
                    B[jj + 0][ii + i] = t0;
                    B[jj + 1][ii + i] = t1;
                    B[jj + 2][ii + i] = t2;
                    B[jj + 3][ii + i] = t3;

                    /* temporarily write the right 4 into the "upper" part of B
                       at rows ii+4..ii+7 so we can avoid conflicts */
                    B[jj + 0][ii + 4 + i] = t4;
                    B[jj + 1][ii + 4 + i] = t5;
                    B[jj + 2][ii + 4 + i] = t6;
                    B[jj + 3][ii + 4 + i] = t7;
                }

                /* step 2: for the left 4 columns of the 8x8 block, read the
                 * values from A's bottom 4 rows and move the previously staged
                 * elements into their final positions
                 */
                for (j = 0; j < 4; ++j) {
                    /* read bottom-left 4 values from A */
                    t0 = A[ii + 4 + 0][jj + j];
                    t1 = A[ii + 4 + 1][jj + j];
                    t2 = A[ii + 4 + 2][jj + j];
                    t3 = A[ii + 4 + 3][jj + j];

                    /* read the previously staged values that are sitting at:
                       B[jj + j][ii + 4 + k] for k = 0..3  (these came from the
                       earlier step for the top rows)
                    */
                    t4 = B[jj + j][ii + 4 + 0];
                    t5 = B[jj + j][ii + 4 + 1];
                    t6 = B[jj + j][ii + 4 + 2];
                    t7 = B[jj + j][ii + 4 + 3];

                    /* place the bottom-left values into their final transposed positions */
                    B[jj + j][ii + 4 + 0] = t0;
                    B[jj + j][ii + 4 + 1] = t1;
                    B[jj + j][ii + 4 + 2] = t2;
                    B[jj + j][ii + 4 + 3] = t3;

                    /* move the staged top-right values into top-right final positions
                       (they were staged in B[jj + 0..3][ii+4 + i] earlier) */
                    B[jj + 4 + j][ii + 0] = t4;
                    B[jj + 4 + j][ii + 1] = t5;
                    B[jj + 4 + j][ii + 2] = t6;
                    B[jj + 4 + j][ii + 3] = t7;
                }

                /* step 3: finish the bottom-right 4x4 block by reading A and writing B */
                for (i = 4; i < 8; ++i) {
                    t0 = A[ii + i][jj + 4];
                    t1 = A[ii + i][jj + 5];
                    t2 = A[ii + i][jj + 6];
                    t3 = A[ii + i][jj + 7];

                    B[jj + 4][ii + i] = t0;
                    B[jj + 5][ii + i] = t1;
                    B[jj + 6][ii + i] = t2;
                    B[jj + 7][ii + i] = t3;
                }
            }
        }
        return;
    }

    /* ---------------- Generic case (e.g., 61 x 67) ---------------- */
    {
        int block = 16;
        for (ii = 0; ii < N; ii += block) {
            for (jj = 0; jj < M; jj += block) {
                int i_max = ii + block;
                int j_max = jj + block;
                if (i_max > N) i_max = N;
                if (j_max > M) j_max = M;
                for (i = ii; i < i_max; ++i) {
                    for (j = jj; j < j_max; ++j) {
                        if (i != j) {
                            B[j][i] = A[i][j];
                        } else {
                            /* diagonal */
                            B[j][i] = A[i][j];
                        }
                    }
                }
            }
        }
        return;
    }
}

/* A simple function for comparison (registered below) */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;
    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }
}

/* Register functions for the driver */
void registerFunctions()
{
    registerTransFunction(transpose_submit, transpose_submit_desc);
    registerTransFunction(trans, trans_desc);
}

/* correctness checker (unchanged) */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;
    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}
