/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Decoder_Block_Code.cpp
 * Author: silas
 * 
 * Created on March 9, 2018, 1:29 PM
 */


#include <cstdlib>
#include <iostream>
#include <cstring>
#include "codingOperations.h"
#include "basicOperations.h"
#include "Decoder_Block_Code.h"

using std::cout;
using std::endl;

Decoder_Block_Code::Decoder_Block_Code(unsigned char *generator, int k_value, int n_value, int T_value) {

    k = k_value;
    n = n_value;
    T = T_value;

    int i;
    G = (unsigned char *) malloc(k * n * sizeof(unsigned char)); //Used for storing the generator
    //for (i = 0; i < k * n; i++)
    //G[i] = generator[i];
    memcpy(G, generator, size_t(k * n));

    data = (unsigned char *) malloc(k * sizeof(unsigned char)); // create buffer for recovered data
    //for (i = 0; i < k; i++)
    //data[i] = 0x0;
    memset(data, 0, size_t(k));

    codeword = (unsigned char *) malloc(
            n * sizeof(unsigned char)); //create codeword buffer which will be modified by the decoder
    //for (i = 0; i < n; i++)
        //codeword[i] = 0x0;
    memset(codeword, 0, size_t(n));

    erasure = (bool *) malloc(n * sizeof(bool)); //store the erasure pattern for future use
    for (i = 0; i < n; i++)
        erasure[i] = false;
}

Decoder_Block_Code::~Decoder_Block_Code() {

    free(G);
    free(data);
    free(codeword);
    free(erasure);
}

void Decoder_Block_Code::decodeSymbol(unsigned char symbol, bool erasure_value, int t) { //t between 0 and n-1

    erasure[t] = erasure_value;
    if (erasure_value == 0)
        codeword[t] = symbol;

    if (t < T) // no need to decode anything
        return;

    decodeBlock(data, G, codeword, erasure, k, n, T, t - T); //need to decode the symbol transmitted by t-T

    if (t == n - 1) {
        for (int j = t - T + 1; j < k; j++)
            decodeBlock(data, G, codeword, erasure, k, n, T, j); //decode the remaining non-urgent symbols
    }

    return;
}

unsigned char Decoder_Block_Code::outputSymbol(int t) { //t denotes sequence number

    return data[t];
}

bool Decoder_Block_Code::outputErasure(int t) { //t denotes sequence number

    return erasure[t];
}