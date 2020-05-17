/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Encoder_Basic.cpp
 * Author: silas
 * 
 * Created on March 9, 2018, 9:12 AM
 */

#include <cstdlib>
#include <iostream>
#include "codingOperations.h"
#include "basicOperations.h"
#include "Encoder_Basic.h"

using std::cout;
using std::endl;

Encoder_Basic::Encoder_Basic(unsigned char *generator, int T_value, int B_value, int N_value, int k_value,
                             int n_value) {

    T = T_value;
    B = B_value;
    N = N_value;
    k = k_value;
    n = n_value;
    G = generator;       //copy G from the sender

    encoder_blk = (Encoder_Block_Code **) malloc(n * sizeof(Encoder_Block_Code *));

    for (int i = 0; i < n; i++)
        encoder_blk[i] = new Encoder_Block_Code(G, T, B, N, k, n);
}

Encoder_Basic::~Encoder_Basic() {

    for (int i = 0; i < n; i++)
        delete encoder_blk[i];

    free(encoder_blk);
}


void Encoder_Basic::encodeStream(unsigned char *data, unsigned char *codeword, int t) {

    int i, streamOffset;
    streamOffset = t % n;  // dataInternalCounter;                       //current stream is dataInternalCounter

    for (i = 0; i < k; i++) {                                        //store each encoded symbol to a block coding object
        encoder_blk[streamOffset]->encodeSymbol(data[i], i);
        // Elad - symbols [0,k-2] are just copied to the right place in the diagonal, when i==k-1 -> put all parities
        streamOffset--;
        if (streamOffset < 0)
            streamOffset = n - 1;
    }

    // the following is to output a codeword symbol
    streamOffset = t % n;                       //current stream is dataInternalCounter

    for (i = 0; i < n; i++) {                                        //copy the values in the block coding objects back to the output codeword
        encoder_blk[streamOffset]->outputSymbol(codeword, i);
        // Elad - rearrange symbols stored on the diagonals (each encoder_blk) to one array with packets spread
        streamOffset--;
        if (streamOffset < 0)
            streamOffset = n - 1;
    }

    return;
}

