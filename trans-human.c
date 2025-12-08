/*********
 * Roxanne Lutz, Nathan Nail
 * CS 341L Fall 2025
 * 
 * trans-human.c: human attempts, along with
 * final successes, to minimize cache misses for 
 * specified sizes. 
 * 
 * all code generated without use of genAI.
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
void trans(int M, int N, int A[N][M], int B[M][N]);
void best_32_32(int M, int N, int A[N][M], int B[M][N]);
void best_64_64(int M, int N, int A[N][M], int B[M][N]);
void best_61_67(int M, int N, int A[N][M], int B[M][N]);
/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
  if (M == 32 && N == 32) { // optimized 32x32 output size
    best_32_32(M, N, A, B);
  } else if (M == 64 && N == 64) { // optimized 64x64 output size
    best_64_64(M, N, A, B);
  } else if (M == 61 && N == 67) { // optimized 61x67 output size
    best_61_67(M, N, A, B);
  } else { // given anything else, no optimization built.
    trans(M, N, A, B);
  } // end if
} // end submission function

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
  int i, j;
  for (i = 0; i < N; i++) { 
    for (j = 0; j < M; j++) { 
      B[j][i] = A[i][j];
    }
  }
}

/**
 * ====================================================
 * FUNCTIONS WRITTEN BY HUMAN OPTIMIZED FOR EACH SIZE
 * 
 * note: please read documentation in report for more
 * thorough discussion on the approach. header comments
 * will give a brief summation only.
 * ====================================================
 */


/**
 * for the 32x32, we perform a very simple 8x8 blocking
 * technique to capitalize on the 8 ints stored in a line 
 * once accessed.
 * further, we address the diagonal block conflict zone by 
 * avoiding a thrashing of A vs B accesses by avoiding all
 * ii == jj points until we're ready to relinquish the set 
 * to B's control.
 */
char best_32_32_func[] = "Best For 32x32";
void best_32_32(int M, int N, int A[N][M], int B[M][N]) {
    int blocksize = 8;
    int i, j, ii, jj, temp;
    
    for (i = 0; i < N; i += blocksize) { // row block increaser
        for (j = 0; j < M; j += blocksize) { // col block increaser

            if (i == j) { // if we're in a "diagonal block"
                for (ii = i; ii < i + blocksize; ii++) {
                    temp = A[ii][ii]; // give A control over the row
                    for (jj = j; jj < j + blocksize; jj++) {
                        if (jj != ii) { // skip over all cols of B that will thrash A's row control
                            B[jj][ii] = A[ii][jj];
                        } // end if
                    } // end loop
                    B[ii][ii] = temp; // give B control over this set by ending with a diagonal set
                } // end loop
            } else { // otherwise, there are no overlapping set conflicts with A and B,
              // proceed normally and fill the B block as transpose of A block.
                for (ii = i; ii < i + blocksize; ii++) {
                    for (jj = j; jj < j + blocksize; jj++) {
                        B[jj][ii] = A[ii][jj];
                    } // end loop 
                } // end loop
            } // end if
        } // end loop
    } // end loop
} // end 32x32 optimization

/**
 * for the 64x64, we need to employ a couple tactics.
 * it has the unique problem of each 8x8 block having the same
 * sets in the first 4 rows as the second 4 rows. 
 * this causes huge conflict along the diagonals as well as the general case.
 * 
 * so, we are going to handle the diagonals first in a strange way:
 * + for the first half of the columns:
 *  + load up the values of A diagonal block into the top right 
 *    rows of B (rows 0 -> 4, cols 48 -> 64). access A rowwise in this process,
 *    meaning fill top half of A block into cols 56 -> 63 and bottom half into 48 -> 55.
 *  + THEN, fill B also row wise from those corresponding sections in B's corner.
 * + for the second half of columns, we do essentially the same thing but
 *   use the bottom left corner of B.
 * 
 * then, the rest of the matrix (avoiding all now finished diagonal blocks) 
 * will be filled by accessing A blocks in a sideways U shape which minimizes the misses
 * in the general-non-diagonal case well between A and B.
 */
char best_64_64_func[] = "Best For 64x64";
void best_64_64(int M, int N, int A[N][M], int B[M][N]) {

 int blocksize = 4; 
 int double_blocksize = 8;
 int i, j, ii, jj; 
 int diag_row_placeholder, diag_col_placeholder; 

  // we're gonna fill a little differently. instead we are going
  // to in place put the top row from A into top right row of B
  // and bottom row into one to the left of the other in B is well.
  // THEN, we will fill B diag rowwise in the needed transpose order.
  // the following does the first half of the matrix diagonals.
  for (i = 0, j = 0; i < (N / 2); i += 8, j += 8) {
    // fill top row from A into top right rows of B
    for (ii = i, diag_row_placeholder = 0; ii < i + blocksize; ii++, diag_row_placeholder++){
      for (jj = j, diag_col_placeholder = 56; jj < j + double_blocksize; jj++, diag_col_placeholder++) {
        B[diag_row_placeholder][diag_col_placeholder] = A[ii][jj]; // LAST 8 col of B, 56 -> 63
      } // end loop
    } // end loop

    // fill next row OVER to left with bottom half from A
    for (ii = i + blocksize, diag_row_placeholder = 0; ii < i + double_blocksize; ii++, diag_row_placeholder++){
      for (jj = j, diag_col_placeholder = 48; jj < j + double_blocksize; jj++, diag_col_placeholder++) {
        B[diag_row_placeholder][diag_col_placeholder] = A[ii][jj]; // NEXT 8 col of B, 48 -> 55
      } // end loop
    } // end loop

    // THEN, we fill B accordingly and row wise by 4x4 blocks from B rows in a sideways u shape
    for (ii = i, diag_row_placeholder = 0; ii < i + blocksize; ii++, diag_row_placeholder++) { // fill the upper left square
      for (jj = j, diag_col_placeholder = 56; jj < j + blocksize; jj++, diag_col_placeholder++) {
          B[jj][ii] = B[diag_row_placeholder][diag_col_placeholder];
      } // end loop
    } // end loop

    for (ii = i + blocksize, diag_row_placeholder = 0; ii < i + double_blocksize; ii++, diag_row_placeholder++) { // fill the upper right square
      for (jj = j, diag_col_placeholder = 48; jj < j + blocksize; jj++, diag_col_placeholder++) {
          B[jj][ii] = B[diag_row_placeholder][diag_col_placeholder];
      }  // end loop
    }  // end loop


    for (ii = i, diag_row_placeholder = 0; ii < i + blocksize; ii++, diag_row_placeholder++) { // fill the bottom left square
      for (jj = j + blocksize, diag_col_placeholder = 60; jj < j + double_blocksize; jj++, diag_col_placeholder++) {
          B[jj][ii] = B[diag_row_placeholder][diag_col_placeholder];
      } // end loop
    }  // end loop

    for (ii = i + blocksize, diag_row_placeholder = 0; ii < i + double_blocksize; ii++, diag_row_placeholder++) { // fill the bottom right square
      for (jj = j + blocksize, diag_col_placeholder = 52; jj < j + double_blocksize; jj++, diag_col_placeholder++) {
          B[jj][ii] = B[diag_row_placeholder][diag_col_placeholder];
      } // end loop
    } // end loop
   
  } // end loop

  // THEN, we do the other half diagonals. which means we use the bottom left rows of B 
  // instead and fill the same way.
  for (i = (N / 2), j = (M / 2); i < N; i += double_blocksize, j += double_blocksize) {

    // fill top row from A into BOTTOM LEFT rows
    for (ii = i, diag_row_placeholder = 60; ii < i + blocksize; ii++, diag_row_placeholder++){
      for (jj = j, diag_col_placeholder = 0; jj < j + double_blocksize; jj++, diag_col_placeholder++) {
        B[diag_row_placeholder][diag_col_placeholder] = A[ii][jj]; // FIRST 8 col of B, 0 -> 7 in LAST ROW
      } // end loop
    } // end loop

    // fill next row OVER to RIGHT with bottom half from A
    for (ii = i + blocksize, diag_row_placeholder = 60; ii < i + double_blocksize; ii++, diag_row_placeholder++){
      for (jj = j, diag_col_placeholder = 8; jj < j + double_blocksize; jj++, diag_col_placeholder++) {
        B[diag_row_placeholder][diag_col_placeholder] = A[ii][jj]; // NEXT 8 col of B, 8 -> 15 in LAST ROW
      } // end loop
    } // end loop

    // THEN, we fill B accordingly and row wise by 4x4 blocks from B rows
    for (ii = i, diag_row_placeholder = 60; ii < i + blocksize; ii++, diag_row_placeholder++) { // fill the upper left square
      for (jj = j, diag_col_placeholder = 0; jj < j + blocksize; jj++, diag_col_placeholder++) {
          B[jj][ii] = B[diag_row_placeholder][diag_col_placeholder];
      } // end loop
    } // end loop

    for (ii = i + blocksize, diag_row_placeholder = 60; ii < i + double_blocksize; ii++, diag_row_placeholder++) { // fill the upper right square
      for (jj = j, diag_col_placeholder = 8; jj < j + blocksize; jj++, diag_col_placeholder++) {
          B[jj][ii] = B[diag_row_placeholder][diag_col_placeholder];
      } // end loop
    } // end loop


    for (ii = i, diag_row_placeholder = 60; ii < i + blocksize; ii++, diag_row_placeholder++) { // fill the bottom left square
      for (jj = j + blocksize, diag_col_placeholder = 4; jj < j + double_blocksize; jj++, diag_col_placeholder++) {
          B[jj][ii] = B[diag_row_placeholder][diag_col_placeholder];
      } // end loop
    } // end loop


    for (ii = i + blocksize, diag_row_placeholder = 60; ii < i + double_blocksize; ii++, diag_row_placeholder++) { // fill the bottom right square
      for (jj = j + blocksize, diag_col_placeholder = 12; jj < j + double_blocksize; jj++, diag_col_placeholder++) {
          B[jj][ii] = B[diag_row_placeholder][diag_col_placeholder];
      } // end loop
    } // end loop
   
  } // end loop

  // now, onto the remaining, non-diagonal blocks.
  // access A in a sideways U pattern to fill with minimal misses generally.
  for (i = 0; i < N; i += double_blocksize) { 
    for (j = 0; j < M; j += double_blocksize) {
      if (i != j) { // skip over diagonals.

        for (ii = i; ii < i + blocksize; ii++) { // upper left square
          for (jj = j; jj < j + blocksize; jj++) {
              B[jj][ii] = A[ii][jj];
          } // end loop
        } // end loop

         for (ii = i; ii < i + blocksize; ii++) { // upper right square
          for (jj = j + blocksize; jj < j + double_blocksize; jj++) {
              B[jj][ii] = A[ii][jj];
          } // end loop
        } // end loop

       
        for (ii = i + blocksize; ii < i + double_blocksize; ii++) { // bottom right square
          for (jj = j + blocksize; jj < j + double_blocksize; jj++) {
              B[jj][ii] = A[ii][jj];
          } // end loop
        } // end loop
       
        for (ii = i + blocksize; ii < i + double_blocksize; ii++) { // bottom left square
          for (jj = j; jj < j + blocksize; jj++) {
              B[jj][ii] = A[ii][jj];
          } // end loop
        } // end loop

      } // end if
    } // end loop
  } // end loop

} // end 64x64 optimization

/**
 * for the 61x61, we can actually use a pretty simple
 * blocking tactic to get it < 2000 misses.
 * the sets are repetitive in the same way the 64x64 is, but
 * they do not sit on top of each other, and instead drift
 * diagonally.further, this means the conflict areas
 * are not consistently on the diagonals. 
 * so, we can use a simple 8x8 blocking technique, and
 * by moving the B blocks rowwise and A blocks columnwise, 
 * we are able to minimize misses and get under the lower bound.
 */
char best_61_67_func[] = "Best For 61x67";
void best_61_67(int M, int N, int A[N][M], int B[M][N]) {

  int blocksize = 8; // simple 8x8 blocking
  int i, j, ii, jj;

  for (j = 0; j < M; j += blocksize) { // swapped outer loops to move B blocks rowwise
    for (i = 0; i < N; i += blocksize) { // A blocks col-wise
    
      for (ii = i; ii < i + blocksize; ii++) {
        for (jj = j; jj < j + blocksize; jj++) {
          if (ii < N && jj < M) { // if we reach past row/col, don't access
            B[jj][ii] = A[ii][jj];
          } // end if
        } // end loop
      } // end loop

    } // end loop
  } // end loop

} // end 61x67 optimization

/**
 * ====================================================
 * FAILED ATTEMPTS -- INCLUDED FOR REPORT
 * 
 * these are merely included to provide evidence and 
 * numbers for failed attempts to assist in explaining 
 * the process to get the successful attempt.
 * ====================================================
 */

 /**
  * ===============
  * 32x32 ATTEMPTS
  * ===============
  */

/**
 * basic 8x8 blocking of the matrix.
 */
char attempt_1_32_32_func[] = "32x32, Attempt 1 - 8x8 Block, No Diag";
void attempt_1_32_32(int M, int N, int A[N][M], int B[M][N]) {
  int blocksize = 8; // attempt a blocksize of 8
  int i, j, ii, jj;


   for (i = 0; i < N; i += blocksize) { // row block increaser
    for (j = 0; j < M; j += blocksize) { // col block increaser
       
        for (ii = i; ii < i + blocksize; ii++) { // fill the mini blocks
            for (jj = j; jj < j + blocksize; jj++) {
                B[jj][ii] = A[ii][jj];
            } // end loop
        } // end loop
    } // end loop
  } // end loop
} // end attempt


 /**
  * ===============
  * 64x64 ATTEMPTS
  * ===============
  */

/**
 * basic 64x64 8x8 blocking to see how it behaves
 */
char attempt_1_64_64_func[] = "64x64, Attempt 1 - 8x8 Block, No Diag";
void attempt_1_64_64(int M, int N, int A[N][M], int B[M][N]) {
  int blocksize = 8; // attempt a blocksize of 8
  int i, j, ii, jj;

   for (i = 0; i < N; i += blocksize) { // row block increaser
    for (j = 0; j < M; j += blocksize) { // col block increaser
       
        for (ii = i; ii < i + blocksize; ii++) { // fill the mini blocks
            for (jj = j; jj < j + blocksize; jj++) {
                B[jj][ii] = A[ii][jj];
            } // end loop
        } // end loop
    } // end loop
  } // end loop
} // end attempt 

/**
 * basic 64x64 4x4 blocking to see how it behaves
 */
char attempt_2_64_64_func[] = "64x64, Attempt 2 - 4x4 Block, No Diag";
void attempt_2_64_64(int M, int N, int A[N][M], int B[M][N]) {
  int blocksize = 4; // attempt a blocksize of 8
  int i, j, ii, jj;

   for (i = 0; i < N; i += blocksize) { // row block increaser
    for (j = 0; j < M; j += blocksize) { // col block increaser
       
        for (ii = i; ii < i + blocksize; ii++) { // fill the mini blocks
            for (jj = j; jj < j + blocksize; jj++) {
                B[jj][ii] = A[ii][jj];
            } // end loop
        } // end loop
    } // end loop
  } // end loop
} // end attempt 


/**
 * basic 64x64 4x4 blocking with a sideways U
 * general case access pattern
 */
char attempt_3_64_64_func[] = "64x64, Attempt 3 - 4x4 Block, No Diag, U Access";
void attempt_3_64_64(int M, int N, int A[N][M], int B[M][N]) {
  int blocksize = 4; // attempt a blocksize of 8
  int double_blocksize = 8;
  int i, j, ii, jj;
  // access A in a sideways U pattern to fill with minimal misses generally.
  for (i = 0; i < N; i += double_blocksize) { 
    for (j = 0; j < M; j += double_blocksize) {

      for (ii = i; ii < i + blocksize; ii++) { // upper left square
        for (jj = j; jj < j + blocksize; jj++) {
            B[jj][ii] = A[ii][jj];
        } // end loop
      } // end loop

        for (ii = i; ii < i + blocksize; ii++) { // upper right square
        for (jj = j + blocksize; jj < j + double_blocksize; jj++) {
            B[jj][ii] = A[ii][jj];
        } // end loop
      } // end loop

      
      for (ii = i + blocksize; ii < i + double_blocksize; ii++) { // bottom right square
        for (jj = j + blocksize; jj < j + double_blocksize; jj++) {
            B[jj][ii] = A[ii][jj];
        } // end loop
      } // end loop
      
      for (ii = i + blocksize; ii < i + double_blocksize; ii++) { // bottom left square
        for (jj = j; jj < j + blocksize; jj++) {
            B[jj][ii] = A[ii][jj];
        } // end loop
      } // end loop

    } // end loop
  } // end loop
 
} // end attempt 

/**
 * attempt made to test out the idea of placing 
 * values from A into B and then using B as the fill
 */
char attempt_4_64_64_func[] = "64x64, Attempt 4 - 4x4 Block, First Attempt Diag Handling, U Access";
void attempt_4_64_64(int M, int N, int A[N][M], int B[M][N]) {
  int blocksize = 4; 
  int double_blocksize = 8;
  int i, j, ii, jj;

  // attempt to avoid set clash by storing A vals next to destination diagonal in B
  for (i = 0; i < N - double_blocksize; i += double_blocksize) {
    for (ii = i; ii < i + double_blocksize; ii++){
      for (jj = i; jj < i + double_blocksize; jj++) {
        B[ii][jj + double_blocksize] = A[ii][jj];
      } // end loop
    } // end loop
  } // end loop

  // have to put the final diagonals to the left of the last one
  for (ii = N - double_blocksize; ii < N; ii++) {
    for (jj = N - double_blocksize; jj < N; jj++) {
      B[ii][jj - double_blocksize] = A[ii][jj];
    } // end loop
  } // end loop

  // now, let's fill the diagonals FROM B
  // we're gonna fill as a sideways U shape starting in top left
  for (i = 0, j = 0; i < N; i += double_blocksize, j += double_blocksize) {

    if (i < M - double_blocksize) { // NOT the last row  
      for (int ii = i; ii < i + blocksize; ii++) { // fill the upper left square
        for (int jj = j; jj < j + blocksize; jj++) {
            B[jj][ii] = B[ii][jj + double_blocksize];
        } // end loop
      } // end loop


      for (int ii = i; ii < i + blocksize; ii++) { // fill the upper right square
        for (int jj = j + blocksize; jj < j + double_blocksize; jj++) {
            B[jj][ii] = B[ii][jj + double_blocksize];
        } // end loop
      } // end loop


      for (int ii = i + blocksize; ii < i + double_blocksize; ii++) { // fill the bottom right square
        for (int jj = j + blocksize; jj < j + double_blocksize; jj++) {
            B[jj][ii] = B[ii][jj + double_blocksize];
        } // end loop
      } // end loop


      for (int ii = i + blocksize; ii < i + double_blocksize; ii++) { // fill the bottom left square
        for (int jj = j; jj < j + blocksize; jj++) {
            B[jj][ii] = B[ii][jj + double_blocksize];
        } // end loop
      } // end loop
    } else { // special case for last diag -- look left

      for (int ii = i; ii < i + blocksize; ii++) { // fill the upper left square
        for (int jj = j; jj < j + blocksize; jj++) {
            B[jj][ii] = B[ii][jj - double_blocksize];
        } // end loop
      } // end loop


      for (int ii = i; ii < i + blocksize; ii++) { // fill the upper right square
        for (int jj = j + blocksize; jj < j + double_blocksize; jj++) {
            B[jj][ii] = B[ii][jj - double_blocksize];
        } // end loop
      } // end loop


      for (int ii = i + blocksize; ii < i + double_blocksize; ii++) { // fill the bottom right square
        for (int jj = j + blocksize; jj < j + double_blocksize; jj++) {
            B[jj][ii] = B[ii][jj - double_blocksize];
        } // end loop
      } // end loop


      for (int ii = i + blocksize; ii < i + double_blocksize; ii++) { // fill the bottom left square
        for (int jj = j; jj < j + blocksize; jj++) {
            B[jj][ii] = B[ii][jj - double_blocksize];
        } // end loop
      } // end loop

    } // end if
  } // end loop


  // access A in a sideways U pattern to fill with minimal misses generally.
  for (i = 0; i < N; i += double_blocksize) { 
    for (j = 0; j < M; j += double_blocksize) {

      if (i != j) { // avoid diagonals
        for (ii = i; ii < i + blocksize; ii++) { // upper left square
          for (jj = j; jj < j + blocksize; jj++) {
              B[jj][ii] = A[ii][jj];
          } // end loop
        } // end loop

        for (ii = i; ii < i + blocksize; ii++) { // upper right square
          for (jj = j + blocksize; jj < j + double_blocksize; jj++) {
              B[jj][ii] = A[ii][jj];
          } // end loop
        } // end loop

        
        for (ii = i + blocksize; ii < i + double_blocksize; ii++) { // bottom right square
          for (jj = j + blocksize; jj < j + double_blocksize; jj++) {
              B[jj][ii] = A[ii][jj];
          } // end loop
        } // end loop
        
        for (ii = i + blocksize; ii < i + double_blocksize; ii++) { // bottom left square
          for (jj = j; jj < j + blocksize; jj++) {
              B[jj][ii] = A[ii][jj];
          } // end loop
        } // end loop
      } // end if
      
    } // end loop
  } // end loop
 
} // end attempt 

 /**
  * ===============
  * 61x67 ATTEMPTS
  * ===============
  */

/**
 * basic 4x4 blocking of the matrix.
 */
char attempt_1_61_67_func[] = "61x67, Attempt 1 - 4x4 Block, No Diag";
void attempt_1_61_67(int M, int N, int A[N][M], int B[M][N]) {
  int blocksize = 4; // attempt a blocksize of 4
  int i, j, ii, jj;

  for (i = 0; i < N; i += blocksize) {
    for (j = 0; j < N; j += blocksize) {
     
      for (ii = i; ii < i + blocksize; ii++) {
        for (jj = j; jj < j + blocksize; jj++) {
          if (ii < N && jj < M) {
            B[jj][ii] = A[ii][jj];
          } // end if
        } // end loop
      } // end loop

    } // end loop
  } // end loop

} // end attempt

/**
 * basic 8x8 blocking of the matrix.
 */
char attempt_2_61_67_func[] = "61x67, Attempt 2 - 8x8 Block, No Diag";
void attempt_2_61_67(int M, int N, int A[N][M], int B[M][N]) {
  int blocksize = 8; // attempt a blocksize of 4
  int i, j, ii, jj;

  for (i = 0; i < N; i += blocksize) {
    for (j = 0; j < N; j += blocksize) {
     
      for (ii = i; ii < i + blocksize; ii++) {
        for (jj = j; jj < j + blocksize; jj++) {
          if (ii < N && jj < M) {
            B[jj][ii] = A[ii][jj];
          } // end if
        } // end loop
      } // end loop

    } // end loop
  } // end loop

} // end attempt


/**
 * ====================================================
 * GIVEN FUNCTIONS TO RUN TEST CASES
 * ====================================================
 */

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); // normal transpose
    registerTransFunction(best_32_32, best_32_32_func); // best for 32x32
    registerTransFunction(best_64_64, best_64_64_func); // best for 64x64
    registerTransFunction(best_61_67, best_61_67_func); // best for 61x67

    // failed attempts, comment to disable these when running
    registerTransFunction(attempt_1_32_32, attempt_1_32_32_func);

    registerTransFunction(attempt_1_64_64, attempt_1_64_64_func);
    registerTransFunction(attempt_2_64_64, attempt_2_64_64_func);
    registerTransFunction(attempt_3_64_64, attempt_3_64_64_func);
    registerTransFunction(attempt_4_64_64, attempt_4_64_64_func);

    registerTransFunction(attempt_1_61_67, attempt_1_61_67_func);
    registerTransFunction(attempt_2_61_67, attempt_2_61_67_func);
    
}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
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

