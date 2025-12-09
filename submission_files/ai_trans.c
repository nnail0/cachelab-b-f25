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

/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* forward declarations for the separate-case helpers */
void trans_32(int M, int N, int A[N][M], int B[M][N]);
void trans_64(int M, int N, int A[N][M], int B[M][N]);
void trans_61x67(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - Dispatcher (keeps description string for driver).
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    /* dispatch to specialized functions so each function is small and stable */
    if (M == 32 && N == 32) {
        trans_32(M, N, A, B);
        return;
    }
    if (M == 64 && N == 64) {
        trans_64(M, N, A, B);
        return;
    }
    /* default / given case in the assignment */
    if (M == 61 && N == 67) {
        trans_61x67(M, N, A, B);
        return;
    }

    /* fallback (generic) */
    trans_61x67(M, N, A, B);
}

/* ------------------- 32x32 implementation -------------------
   Typical 8x8 blocking; keep diagonals out-of-place until end of block
   to avoid repeatedly evicting the diagonal element from cache.
   Local var count kept small.
*/
void trans_32(int M, int N, int A[N][M], int B[M][N])
{
    int ii, jj, i, j;
    int block = 8;
    int tmp;      /* store diagonal element temporarily */
    int diag = -1;

    for (ii = 0; ii < N; ii += block) {
        for (jj = 0; jj < M; jj += block) {
            for (i = ii; i < ii + block; ++i) {
                /* careful: when block along diagonal, we save diagonal element
                   and write it after the inner loop so we don't thrash the cache. */
                for (j = jj; j < jj + block; ++j) {
                    if (i == j) {
                        tmp = A[i][j];
                        diag = i;   /* remember diagonal index */
                    } else {
                        B[j][i] = A[i][j];
                    }
                }
                if (diag == i) {
                    B[i][i] = tmp;
                    diag = -1;
                }
            }
        }
    }
}

/* ------------------- 64x64 implementation -------------------
   Carefully tuned variant for 8x8 sub-blocks that avoids conflict misses
   on a 1KB direct-mapped, 32-byte blocks cache (classic solution).
   Keep variable count within limits.
*/
void trans_64(int M, int N, int A[N][M], int B[M][N])
{
    int ii, jj, i, j;
    int t0, t1, t2, t3, t4, t5, t6, t7;

    /* iterate over 8x8 blocks */
    for (ii = 0; ii < 64; ii += 8) {
        for (jj = 0; jj < 64; jj += 8) {

            /* Stage 1: read the top-left 4x8 strip and place the left half
               into B's top-left and the right half temporarily into B's top-right */
            for (i = 0; i < 4; ++i) {
                t0 = A[ii + i][jj + 0];
                t1 = A[ii + i][jj + 1];
                t2 = A[ii + i][jj + 2];
                t3 = A[ii + i][jj + 3];
                t4 = A[ii + i][jj + 4];
                t5 = A[ii + i][jj + 5];
                t6 = A[ii + i][jj + 6];
                t7 = A[ii + i][jj + 7];

                B[jj + 0][ii + i] = t0;
                B[jj + 1][ii + i] = t1;
                B[jj + 2][ii + i] = t2;
                B[jj + 3][ii + i] = t3;

                /* place the right half of the row into B's block rows 0..3 but columns ii+4..ii+7
                   (these are temporary positions that we will move into final place below) */
                B[jj + 0][ii + 4 + i] = t4;
                B[jj + 1][ii + 4 + i] = t5;
                B[jj + 2][ii + 4 + i] = t6;
                B[jj + 3][ii + 4 + i] = t7;
            }

            /* Stage 2: move the temporarily stored pieces and finish transpose for the block */
            for (j = 0; j < 4; ++j) {
                t0 = A[ii + 4][jj + j];
                t1 = A[ii + 5][jj + j];
                t2 = A[ii + 6][jj + j];
                t3 = A[ii + 7][jj + j];

                /* read the temp that we stored earlier */
                t4 = B[jj + j][ii + 4];
                t5 = B[jj + j][ii + 5];
                t6 = B[jj + j][ii + 6];
                t7 = B[jj + j][ii + 7];

                /* place left-bottom */
                B[jj + j][ii + 4] = t0;
                B[jj + j][ii + 5] = t1;
                B[jj + j][ii + 6] = t2;
                B[jj + j][ii + 7] = t3;

                /* move the temporary upper-right pieces into the lower-left area */
                B[jj + 4 + j][ii + 0] = t4;
                B[jj + 4 + j][ii + 1] = t5;
                B[jj + 4 + j][ii + 2] = t6;
                B[jj + 4 + j][ii + 3] = t7;
            }

            /* Stage 3: finish the bottom-right 4x4 corner */
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
}

/* ------------------- 61 x 67 implementation -------------------
   Use 16x16 blocking and bounds checks.
*/
void trans_61x67(int M, int N, int A[N][M], int B[M][N])
{
    int ii, jj, i, j;
    int block = 16;
    int imax, jmax;

    for (ii = 0; ii < N; ii += block) {
        for (jj = 0; jj < M; jj += block) {
            imax = (ii + block < N) ? ii + block : N;
            jmax = (jj + block < M) ? jj + block : M;
            for (i = ii; i < imax; ++i) {
                for (j = jj; j < jmax; ++j) {
                    B[j][i] = A[i][j];
                }
            }
        }
    }
}

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
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

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.
 */
void registerFunctions()
{
    /* Register your solution function (the driver looks for this desc) */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions (optional helpers/baseline) */
    registerTransFunction(trans, trans_desc); 
}

/* 
 * is_transpose - This helper checks if B is the transpose of A.
 */
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