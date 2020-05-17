#include <cstdlib>
#include <iostream>
#include <sstream>
#include <typeinfo>
#include <random>
#include <bitset>
#include <cstring>
#include "basicOperations.h"
#include "isa-l.h"

using std::cout;
using std::endl;

unsigned char gf256_add(unsigned char a, unsigned char b) {
    return a ^ b;
}

unsigned char gf256_mul(unsigned char a, unsigned char b) {
    return gf_mul(a, b);
}

unsigned char gf256_inv(unsigned char a) {
    return gf_inv(a);
}

void gf256_transpose(unsigned char *in, unsigned char *out, int k, int n) {
    for (int i = 0; i < k; i++) {
        for (int j = 0; j < n; j++) {
            out[j * k + i] = in[i * n + j];
        }
    }
    return;
}

int gf256_invert_matrix(unsigned char *in, unsigned char *out, const int n) {
    unsigned char temp[n * n];
    for (int i = 0; i < n * n; i++) {
        temp[i] = in[i];
    }
    return gf_invert_matrix(temp, out, n);
}

void gf256_rref_matrix(unsigned char *in, unsigned char *out, unsigned char *action, int m,
                       int n) {  // in is an m by n matrix
    int i, j, k;
    int offset = 0;               //offset for pivot element
    unsigned char temp;

    //for (i = 0; i < n * n; i++)     // initialize action matrix
    //action[i] = 0x0;
    memset(action, 0, size_t(n * n));


    for (i = 0; i < n; i++)
        action[i * n + i] = 0x1;

    //for (i = 0; i < m * n; i++)        //initialize out matrix
    //out[i] = in[i];
    memcpy(out, in, size_t(m * n));

    // Inverse
    for (i = 0; i < n; i++) {                //locating the i^th column
        if (i + offset >= m)
            break;

        //         Check for 0 in pivot element
        if (out[(i + offset) * n + i] == 0x0) {
            //     Find a column with non-zero in current row and swap
            for (j = i + 1; j < n; j++)
                if (out[(i + offset) * n + j] != 0x0)
                    break;
            if (j == n) {    // cannot find a pivot
                offset++;
                i--;
                continue;
            }

            for (k = 0; k < m; k++) {       // Swap columns i,j
                temp = out[k * n + i];
                out[k * n + i] = out[k * n + j];
                out[k * n + j] = temp;
            }

            for (k = 0; k < n; k++) {
                temp = action[k * n + i];
                action[k * n + i] = action[k * n + j];
                action[k * n + j] = temp;
            }
        }

        if (out[(i + offset) * n + i] == 0x0)
            return;

        temp = gf_inv(out[(i + offset) * n + i]);       // 1/pivot

        for (k = 0; k < m; k++) {                          // Scale column i by 1/pivot
            out[k * n + i] = gf_mul(out[k * n + i], temp);
        }

        for (k = 0; k < n; k++) {
            action[k * n + i] = gf_mul(action[k * n + i], temp);
        }

        for (j = 0; j < n; j++) {              //subtracting columns
            if (j == i)
                continue;

            temp = out[(i + offset) * n + j];
            if (temp == 0x0)
                continue;

            for (k = 0; k < m; k++) {
                out[k * n + j] ^= gf_mul(temp, out[k * n + i]);
            }

            for (k = 0; k < n; k++) {
                action[k * n + j] ^= gf_mul(temp, action[k * n + i]);
            }
        }
    }
    return;
}

void
gf256_matrix_mul(unsigned char *inMatrix1, unsigned char *inMatrix2, unsigned char *outMatrix, int m1, int m2, int m3) {

    unsigned char result;

    for (int outRowVar = 0; outRowVar < m1; outRowVar++) {
        for (int outColVar = 0; outColVar < m3; outColVar++) {
            result = 0x0;
            for (int innerProdCount = 0; innerProdCount < m2; innerProdCount++) {
                result ^= gf256_mul(inMatrix1[outRowVar * m2 + innerProdCount],
                                    inMatrix2[innerProdCount * m3 + outColVar]);
            }
            outMatrix[outRowVar * m3 + outColVar] = result;
        }
    }
    return;
}

void printMatrix(unsigned char *matrix, int row, int column) {

    if (DEBUG_FEC == 1) {                         //defined in basicOperations.h
        int i, j, k, digit, digitCount;
        cout << "[";
        for (i = 0; i < row; i++) {
            if (i > 0)
                cout << " ";
            for (j = 0; j < column; j++) {
                digit = (int) unsigned(matrix[i * column + j]);
                //count the number of digits
                if (digit >= 100)
                    digitCount = 3;
                else if (digit >= 10)
                    digitCount = 2;
                else
                    digitCount = 1;
                for (k = 0; k < 3 - digitCount; k++)
                    cout << " ";
                cout << digit << " ";
            }
            if (i < row - 1)
                cout << endl;
            else
                cout << "]" << endl << endl;
        }

    }
    return;
}


void printMatrix(bool *matrix, int row, int column) {
    int i, j, k, digit, digitCount;
    cout << "[";
    for (i = 0; i < row; i++) {
        if (i > 0)
            cout << " ";
        for (j = 0; j < column; j++) {
            digit = (int) matrix[i * column + j];
            //count the number of digits
            if (digit >= 100)
                digitCount = 3;
            else if (digit >= 10)
                digitCount = 2;
            else
                digitCount = 1;

            for (k = 0; k < 3 - digitCount; k++)
                cout << " ";

            cout << digit << " ";
        }

        if (i < row - 1)
            cout << endl;
        else
            cout << "]" << endl << endl;
    }
    return;
}
