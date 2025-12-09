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

/* ai_trans.c — corrected and tuned version */

#include <stdio.h>
#include "cachelab.h"

char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]) {
    int i, j, ii, jj;
    int t0, t1, t2, t3, t4, t5, t6, t7;

    /* ===================== 32 x 32 ===================== */
    if (M == 32 && N == 32) {
        for (ii = 0; ii < 32; ii += 8) {
            for (jj = 0; jj < 32; jj += 8) {
                for (i = ii; i < ii + 8; i++) {
                    for (j = jj; j < jj + 8; j++) {
                        if (i != j)
                            B[j][i] = A[i][j];
                    }
                    /* handle diagonal after finishing the row */
                    if (ii == jj) {
                        t0 = A[i][i];
                        B[i][i] = t0;
                    }
                }
            }
        }
        return;
    }

    /* ===================== 64 x 64 ===================== */
    if (M == 64 && N == 64) {
        /* Standard 8x8 with 4x4 micro-block staging */
        for (ii = 0; ii < 64; ii += 8) {
            for (jj = 0; jj < 64; jj += 8) {

                /* top-left 4x8 */
                for (i = 0; i < 4; i++) {
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

                    B[jj + 0][ii + i + 4] = t4;
                    B[jj + 1][ii + i + 4] = t5;
                    B[jj + 2][ii + i + 4] = t6;
                    B[jj + 3][ii + i + 4] = t7;
                }

                /* bottom-left 4x4 + swaps */
                for (j = 0; j < 4; j++) {
                    t0 = A[ii + 4][jj + j];
                    t1 = A[ii + 5][jj + j];
                    t2 = A[ii + 6][jj + j];
                    t3 = A[ii + 7][jj + j];

                    t4 = B[jj + j][ii + 4];
                    t5 = B[jj + j][ii + 5];
                    t6 = B[jj + j][ii + 6];
                    t7 = B[jj + j][ii + 7];

                    /* place bottom-left values */
                    B[jj + j][ii + 4] = t0;
                    B[jj + j][ii + 5] = t1;
                    B[jj + j][ii + 6] = t2;
                    B[jj + j][ii + 7] = t3;

                    /* move previously staged top-right block */
                    B[jj + 4 + j][ii + 0] = t4;
                    B[jj + 4 + j][ii + 1] = t5;
                    B[jj + 4 + j][ii + 2] = t6;
                    B[jj + 4 + j][ii + 3] = t7;
                }

                /* bottom-right remaining 4x4 */
                for (i = 4; i < 8; i++) {
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

    /* ===================== 61 x 67 (KEEP AS IS) ===================== */
    {
        int block = 16;
        for (ii = 0; ii < N; ii += block) {
            for (jj = 0; jj < M; jj += block) {
                int imax = ii + block;
                int jmax = jj + block;
                if (imax > N) imax = N;
                if (jmax > M) jmax = M;

                for (i = ii; i < imax; i++) {
                    for (j = jj; j < jmax; j++) {
                        if (i != j) B[j][i] = A[i][j];
                    }
                    if (jj <= ii && ii < jj + block && ii < imax && ii < jmax) {
                        B[i][i] = A[i][i];
                    }
                }
            }
        }
    }
}

