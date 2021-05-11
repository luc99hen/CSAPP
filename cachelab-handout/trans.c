/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes. (2^10 (total) = 2^5 (block) * 2^5 (group))
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

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
    int i, j;
    int b_i, b_j;
    int x1, x2, x3, x4, x5, x6, x7, x8;

    if(M == 32){
        // split A/B into k x k, k is determined by N
        // int k = x1 * x1 * 4 / 1024; 
        
        for(i = 0 ; i < 4 ; i++){ 
            for(j = 0; j < 4; j++){
                // reverse block[i][j] and block[j][i]
                // block[i][j] (as b): b[b_i][b_j] = B[i*8+b_i][j*b_w+b_j]
                for(b_i = 0; b_i < 8; b_i++){
                    for(b_j = 0; b_j < 8; b_j++){
                        if(i != j)
                            B[i*8+b_i][j*8+b_j] = A[j*8+b_j][i*8+b_i];
                        else{  // if this block is on the diagnol
                            // put block[k-1-i][k-1-j] of A on block[i][j] of B
                            B[i*8+b_i][j*8+b_j] = A[(4-1-j)*8+b_j][(4-1-i)*8+b_i];
                        }  
                    }
                }
            }
        }

        // reverse block on the diagnol
        for(i = 0; i < 4 / 2; i++){
            j = 4-1-i;
            for(b_i = 0; b_i < 8; b_i++){
                for(b_j = 0; b_j < 8; b_j++){
                    x1 = B[i*8+b_i][i*8+b_j];
                    B[i*8+b_i][i*8+b_j] = B[j*8+b_i][j*8+b_j];
                    B[j*8+b_i][j*8+b_j] = x1;
                }
            }
        }
    } else if(M == 64) {
        for(i = 0 ; i < N ; i+=8){ 
            for(j = 0; j < M; j+=8){
                for(b_i = i; b_i < i+4; b_i++){
                    x1 = A[b_i][j];
                    x2 = A[b_i][j+1];
                    x3 = A[b_i][j+2];
                    x4 = A[b_i][j+3];
                    x5 = A[b_i][j+4];
                    x6 = A[b_i][j+5];
                    x7 = A[b_i][j+6];
                    x8 = A[b_i][j+7];

                    B[j][b_i] = x1;
                    B[j][b_i+4] = x5;
                    B[j+1][b_i] = x2;
                    B[j+1][b_i+4] = x6;
                    B[j+2][b_i] = x3;
                    B[j+2][b_i+4] = x7;
                    B[j+3][b_i] = x4;           
                    B[j+3][b_i+4] = x8;
                }


                for(b_i = 0; b_i < 4; b_i++){

                    x1 = B[j+b_i][i+4];
                    x2 = B[j+b_i][i+5];
                    x3 = B[j+b_i][i+6];
                    x4 = B[j+b_i][i+7];
                    x5 = A[i+4][j+b_i];
                    x6 = A[i+5][j+b_i];
                    x7 = A[i+6][j+b_i];
                    x8 = A[i+7][j+b_i];

                    B[j+b_i][i+4] = x5;
                    B[j+b_i][i+5] = x6;
                    B[j+b_i][i+6] = x7;
                    B[j+b_i][i+7] = x8;
            
                    B[j+4+b_i][i] = x1;
                    B[j+4+b_i][i+1] = x2;
                    B[j+4+b_i][i+2] = x3;
                    B[j+4+b_i][i+3] = x4;
                }

                for(b_i = 0; b_i < 4; b_i++){
                    x1 = A[i+4+b_i][j+4];
                    x2 = A[i+4+b_i][j+5];
                    x3 = A[i+4+b_i][j+6];
                    x4 = A[i+4+b_i][j+7];

                    B[j+4][i+4+b_i] = x1;
                    B[j+5][i+4+b_i] = x2;
                    B[j+6][i+4+b_i] = x3;
                    B[j+7][i+4+b_i] = x4;
                }
                
            }
        }
    } else {
        // basic 16x16 block
        for(i = 0 ; i < N ; i+=16){ 
            for(j = 0; j < M; j+=16){
                // reverse block[i][j] and block[j][i]
                for(b_i = i; b_i < i+16 && b_i < N; b_i++){
                    for(b_j = j; b_j < j+16 && b_j < M; b_j++){
                        B[b_j][b_i] = A[b_i][b_j];
                    }
                }
            }
        }
    }

}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 


char transpose_by_block_desc[] = "Transpose by block";
void transpose_by_block(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;
    int k = 4;

    // !! N == M only

    // split A/B into k x k
    int b_i, b_j;
    for(i = 0 ; i < N ; i+=k){ 
        for(j = 0; j < M; j+=k){
            // reverse block[i][j] and block[j][i]
            for(b_i = i; b_i < i+k && b_i < N; b_i++){
                for(b_j = j; b_j < j+k && b_j < M; b_j++){
                    B[b_j][b_i] = A[b_i][b_j];
                }
            }
        }
    }
}




char use_para_desc[] = "Use variable as x1 cache";
void use_para(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;
    int b_i;
    int x1, x2, x3, x4, x5, x6, x7, x8;

    for(i = 0 ; i < N ; i+=8){ 
        for(j = 0; j < M; j+=8){
            for(b_i = i; b_i < i+4; b_i++){
                x1 = A[b_i][j];
                x2 = A[b_i][j+1];
                x3 = A[b_i][j+2];
                x4 = A[b_i][j+3];
                x5 = A[b_i][j+4];
                x6 = A[b_i][j+5];
                x7 = A[b_i][j+6];
                x8 = A[b_i][j+7];

                B[j][b_i] = x1;
                B[j+1][b_i] = x2;
                B[j+2][b_i] = x3;
                B[j+3][b_i] = x4;

                B[j][b_i+4] = x5;
                B[j+1][b_i+4] = x6;
                B[j+2][b_i+4] = x7;
                B[j+3][b_i+4] = x8;
            }

            for(b_i = i+4; b_i < i+8; b_i++){

                x1 = A[b_i][j];
                x2 = A[b_i][j+1];
                x3 = A[b_i][j+2];
                x4 = A[b_i][j+3];
                x5 = A[b_i][j+4];
                x6 = A[b_i][j+5];
                x7 = A[b_i][j+6];
                x8 = A[b_i][j+7];

                B[j+4][b_i] = x5;
                B[j+5][b_i] = x6;
                B[j+6][b_i] = x7;
                B[j+7][b_i] = x8;

                B[j+4][b_i-4] = x1;
                B[j+5][b_i-4] = x2;
                B[j+6][b_i-4] = x3;
                B[j+7][b_i-4] = x4;
            }

            for(b_i = 0; b_i < 4; b_i++){
                // x1 = B[j+b_i][i+4+b_j];
                // B[j+b_i][i+4+b_j] = B[j+4+b_i][i+b_j];
                // B[j+4+b_i][i+b_j] = x1;

                x1 = B[j+b_i][i+4];
                x2 = B[j+b_i][i+5];
                x3 = B[j+b_i][i+6];
                x4 = B[j+b_i][i+7];

                x5 = B[j+4+b_i][i];
                x6 = B[j+4+b_i][i+1];
                x7 = B[j+4+b_i][i+2];
                x8 = B[j+4+b_i][i+3];
                B[j+4+b_i][i] = x1;
                B[j+4+b_i][i+1] = x2;
                B[j+4+b_i][i+2] = x3;
                B[j+4+b_i][i+3] = x4;

                B[j+b_i][i+4] = x5;
                B[j+b_i][i+5] = x6;
                B[j+b_i][i+6] = x7;
                B[j+b_i][i+7] = x8;
            }
            
        }
    }

}


char ref1_desc[] = "ref: https://www.cnblogs.com/liqiuhao/p/8026100.html?utm_source=debugrun&utm_medium=referral";
void ref1(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;
    int b_i;
    int x1, x2, x3, x4, x5, x6, x7, x8;

    for(i = 0 ; i < N ; i+=8){ 
        for(j = 0; j < M; j+=8){
            for(b_i = i; b_i < i+4; b_i++){
                x1 = A[b_i][j];
                x2 = A[b_i][j+1];
                x3 = A[b_i][j+2];
                x4 = A[b_i][j+3];
                x5 = A[b_i][j+4];
                x6 = A[b_i][j+5];
                x7 = A[b_i][j+6];
                x8 = A[b_i][j+7];

                B[j][b_i] = x1;
                B[j+1][b_i] = x2;
                B[j+2][b_i] = x3;
                B[j+3][b_i] = x4;

                // reverse the order 
                B[j][b_i+4] = x8;
                B[j+1][b_i+4] = x7;
                B[j+2][b_i+4] = x6;
                B[j+3][b_i+4] = x5;
            }


            for(b_i = 0; b_i < 4; b_i++){

                x1 = A[i+4][j+3-b_i];
                x2 = A[i+5][j+3-b_i];
                x3 = A[i+6][j+3-b_i];
                x4 = A[i+7][j+3-b_i];

                x5 = A[i+4][j+4+b_i];
                x6 = A[i+5][j+4+b_i];
                x7 = A[i+6][j+4+b_i];
                x8 = A[i+7][j+4+b_i];

                B[j+4+b_i][i] = B[j+3-b_i][i+4];
                B[j+4+b_i][i+1] = B[j+3-b_i][i+5];
                B[j+4+b_i][i+2] = B[j+3-b_i][i+6];
                B[j+4+b_i][i+3] = B[j+3-b_i][i+7];
                B[j+4+b_i][i+4] = x5;
                B[j+4+b_i][i+5] = x6;
                B[j+4+b_i][i+6] = x7;
                B[j+4+b_i][i+7] = x8;

                B[j+3-b_i][i+4] = x1;
                B[j+3-b_i][i+5] = x2;
                B[j+3-b_i][i+6] = x3;
                B[j+3-b_i][i+7] = x4;
            
            }
            
        }
    }

}


char ref2_desc[] = "ref2: https://zhuanlan.zhihu.com/p/28585726";
void ref2(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;
    int b_i;
    int x1, x2, x3, x4, x5, x6, x7, x8;

    for(i = 0 ; i < N ; i+=8){ 
        for(j = 0; j < M; j+=8){
            for(b_i = i; b_i < i+4; b_i++){
                x1 = A[b_i][j];
                x2 = A[b_i][j+1];
                x3 = A[b_i][j+2];
                x4 = A[b_i][j+3];
                x5 = A[b_i][j+4];
                x6 = A[b_i][j+5];
                x7 = A[b_i][j+6];
                x8 = A[b_i][j+7];

                B[j][b_i] = x1;
                B[j][b_i+4] = x5;
                B[j+1][b_i] = x2;
                B[j+1][b_i+4] = x6;
                B[j+2][b_i] = x3;
                B[j+2][b_i+4] = x7;
                B[j+3][b_i] = x4;           
                B[j+3][b_i+4] = x8;
            }


            for(b_i = 0; b_i < 4; b_i++){

                x1 = B[j+b_i][i+4];
                x2 = B[j+b_i][i+5];
                x3 = B[j+b_i][i+6];
                x4 = B[j+b_i][i+7];
                x5 = A[i+4][j+b_i];
                x6 = A[i+5][j+b_i];
                x7 = A[i+6][j+b_i];
                x8 = A[i+7][j+b_i];

                B[j+b_i][i+4] = x5;
                B[j+b_i][i+5] = x6;
                B[j+b_i][i+6] = x7;
                B[j+b_i][i+7] = x8;
        
                B[j+4+b_i][i] = x1;
                B[j+4+b_i][i+1] = x2;
                B[j+4+b_i][i+2] = x3;
                B[j+4+b_i][i+3] = x4;
            }

            for(b_i = 0; b_i < 4; b_i++){
                x1 = A[i+4+b_i][j+4];
                x2 = A[i+4+b_i][j+5];
                x3 = A[i+4+b_i][j+6];
                x4 = A[i+4+b_i][j+7];

                B[j+4][i+4+b_i] = x1;
                B[j+5][i+4+b_i] = x2;
                B[j+6][i+4+b_i] = x3;
                B[j+7][i+4+b_i] = x4;
            }
            
        }
    }

}


/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "baseline transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, x1;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            x1 = A[i][j];
            B[j][i] = x1;
        }
    }    

}

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
    // registerTransFunction(use_para, use_para_desc); 
    // registerTransFunction(trans, trans_desc); 
    // registerTransFunction(transpose_by_block, transpose_by_block_desc); 
    registerTransFunction(ref1, ref1_desc); 
    registerTransFunction(ref2, ref2_desc); 



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

