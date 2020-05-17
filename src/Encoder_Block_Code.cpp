/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Encoder_Block_Code.cpp
 * Author: silas
 * 
 * Created on March 9, 2018, 10:17 AM
 */

#include <cstdlib>
#include <iostream>
#include <cstring>
#include "codingOperations.h"
#include "basicOperations.h"
#include "Encoder_Block_Code.h"

using std::cout;
using std::endl;

Encoder_Block_Code::Encoder_Block_Code(unsigned char *generator, int T_value, int B_value, int N_value, int k_value,
                                       int n_value) {

    T = T_value;
    B = B_value;
    N = N_value;
    k = k_value;
    n = n_value;
    G = generator;       //set matrix G

    int i;
    data = (unsigned char *) malloc(k *
                                    sizeof(unsigned char));     // create data buffer for encoder, which will be modified in a streaming fashion by the encoder
    //for (i = 0; i < k; i++)
        //data[i] = 0x0;
    memset(data, 0, size_t(k));

    codeword = (unsigned char *) malloc(n *
                                        sizeof(unsigned char));   //create codeword buffer which will be modified in a streaming fashion by the encoder
    //for (i = 0; i < n; i++)
        //codeword[i] = 0x0;
    memset(codeword, 0, size_t(n));

}

Encoder_Block_Code::~Encoder_Block_Code() {
    free(data);
    free(codeword);
}

void Encoder_Block_Code::encodeSymbol(unsigned char single_data, int t) {
    //Elad: single_data=data to transmit[i], t=i
    data[t] = single_data;
    encodeBlock(data, G, codeword, k, n, t); //Elad: codeword = the codeword of encoder_blk

    return;
}

void Encoder_Block_Code::outputSymbol(unsigned char *symbol, int t) {
    //Elad: symbol = the output codeword (one large array with packets spread, pointer pointed in jumps in n), t=index in range [0,n-1] (i in Encoder_Basic::encodeStream)
    // If t=k-1, copy all parity from encoder_blk[streamOffset]->codeword
    symbol[t] = codeword[t];

    int j;
    if (t == k - 1) {
        //for (j = k; j < n; j++)
            //symbol[j] = codeword[j];
        memcpy(symbol+k, codeword+k, size_t(n-k));
        //Elad: copy from encoder_blk[streamOffset]->codeword to output codeword
    }

    return;
}

void Encoder_Block_Code::outputGenerator(unsigned char *generator) {
    //for (int i= 0; i < k * n; i++)
        //generator[i] = G[i];
    memcpy(generator, G, size_t(k*n));

    return;
}