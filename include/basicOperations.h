#ifndef BASICOPERATIONS_H
#define BASICOPERATIONS_H

#include "FEC_Macro.h"   //include macros used by FEC

unsigned char gf256_add(unsigned char a, unsigned char b);

unsigned char gf256_mul(unsigned char a, unsigned char b);

unsigned char gf256_inv(unsigned char a);

int gf256_invert_matrix(unsigned char *in, unsigned char *out, const int n);

/* reduced row echelon form; *in times *action = *out where *out is in reduced row echelon form   */
void gf256_rref_matrix(unsigned char *in, unsigned char *out, unsigned char *action, int k, int n);

void gf256_transpose(unsigned char *in, unsigned char *out, int k, int n); // input a k by n matrix and output its transpose

/*    m1: # of rows for inMatrix1; m2: # of columns for inMatrix1; m3: $ of columns for inMatrix2  */
void gf256_matrix_mul(unsigned char *inMatrix1, unsigned char *inMatrix2, unsigned char *outMatrix, int m1, int m2, int m3);

void printMatrix(unsigned char *matrix, int row, int column);

void printMatrix(bool *matrix, int row, int column);

#endif // BASICOPERATIONS_H_INCLUDED
