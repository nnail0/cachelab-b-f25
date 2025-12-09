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
    int i, j, ii, jj;
    int t0, t1, t2, t3, t4, t5, t6, t7;

    /* ---------------- 32 x 32 ---------------- */
    if (M == 32 && N == 32) {
        int block = 8;
        for (ii = 0; ii < N; ii += block) {
            for (jj = 0; jj < M; jj += block) {
                for (i = ii; i < ii + block; ++i) {
                    for (j = jj; j < jj + block; ++j) {
                        if (i != j) {
                            B[j][i] = A[i][j];
                        } else {
                            t0 = A[i][j];
                            B[j][i] = t0;
                        }
                    }
                }
            }
        }
        return;
    }

    /* ---------------- 64 x 64 ---------------- */
    if (M == 64 && N == 64) {
        for (ii = 0; ii < 64; ii += 8) {
            for (jj = 0; jj < 64; jj += 8) {

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

                    B[jj + 0][ii + 4 + i] = t4;
                    B[jj + 1][ii + 4 + i] = t5;
                    B[jj + 2][ii + 4 + i] = t6;
                    B[jj + 3][ii + 4 + i] = t7;
                }

                for (j = 0; j < 4; ++j) {
                    t0 = A[ii + 4][jj + j];
                    t1 = A[ii + 5][jj + j];
                    t2 = A[ii + 6][jj + j];
                    t3 = A[ii + 7][jj + j];

                    t4 = B[jj + j][ii + 4];
                    t5 = B[jj + j][ii + 5];
                    t6 = B[jj + j][ii + 6];
                    t7 = B[jj + j][ii + 7];

                    B[jj + j][ii + 4] = t0;
                    B[jj + j][ii + 5] = t1;
                    B[jj + j][ii + 6] = t2;
                    B[jj + j][ii + 7] = t3;

                    B[jj + 4 + j][ii + 0] = t4;
                    B[jj + 4 + j][ii + 1] = t5;
                    B[jj + 4 + j][ii + 2] = t6;
                    B[jj + 4 + j][ii + 3] = t7;
                }

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

    /* ---------------- 61 x 67 (original working version) ---------------- */
    {
        int block = 16;
        for (ii = 0; ii < N; ii += block) {
            for (jj = 0; jj < M; jj += block) {
                int imax = (ii + block < N) ? ii + block : N;
                int jmax = (jj + block < M) ? jj + block : M;

                for (i = ii; i < imax; i++) {
                    for (j = jj; j < jmax; j++) {
                        B[j][i] = A[i][j];
                    }
                }
            }
        }
        return;
    }
}

/* Baseline reference */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;
    for (i = 0; i < N; i++)
        for (j = 0; j < M; j++)
            B[j][i] = A[i][j];
}

void registerFunctions()
{
    registerTransFunction(transpose_submit, transpose_submit_desc);
    registerTransFunction(trans, trans_desc);
}

int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;
    for (i = 0; i < N; i++)
        for (j = 0; j < M; ++j)
            if (A[i][j] != B[j][i])
                return 0;
    return 1;
}
