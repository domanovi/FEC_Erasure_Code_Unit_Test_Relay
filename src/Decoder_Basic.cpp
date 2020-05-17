/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Decoder_Basic.cpp
 * Author: silas
 * 
 * Created on March 15, 2018, 12:16 PM
 */


#include <cstdlib>
#include <iostream>
#include "Decoder_Block_Code.h"
#include "Decoder_Basic.h"

using std::cout;
using std::endl;

Decoder_Basic::Decoder_Basic(unsigned char *generator, int T_value, int k_value, int n_value) {

    T = T_value;
    k = k_value;
    n = n_value;
    G = generator;

    decoder_blk = (Decoder_Block_Code **) malloc(n * sizeof(Decoder_Block_Code *));

    for (int i = 0; i < n; i++){
        decoder_blk[i] = new Decoder_Block_Code(G, k, n, T);
	}
}


Decoder_Basic::~Decoder_Basic() {

    for (int i = 0; i < n; i++)
        delete decoder_blk[i];

    free(decoder_blk);
}

int Decoder_Basic::decodeStream(unsigned char *codeword, unsigned char *data, bool erasure, int t) {
    //return 0 if the packet with seq # t-T is recovered perfectly; return 1 otherwise

    int i, dataOffset;
    int erased = 0; //for seq #t-T

    dataInternalCounter = t % n;
    dataOffset = dataInternalCounter;

    for (i = 0; i < n; i++) {                                                     //store the codeword with seq. # t

        if (erasure == 1)
            decoder_blk[dataOffset]->decodeSymbol(0x0, erasure, i);
        else
            decoder_blk[dataOffset]->decodeSymbol(codeword[i], erasure, i);

        dataOffset--;
        if (dataOffset == -1)
            dataOffset = n - 1;
    }
    // we now want to output the data with seq. # t-T

    dataOffset = dataInternalCounter - T;
    if (dataOffset < 0)
        dataOffset += n;

    if (data != NULL) {
        for (i = 0;
             i < k; i++) {                                                     //output the symbol with seq. # t-T

            if (decoder_blk[dataOffset]->outputErasure(i) == 1) {
                erased = 1;
                break;
            }
            data[i] = decoder_blk[dataOffset]->outputSymbol(i);

            dataOffset--;
            if (dataOffset == -1)
                dataOffset = n - 1;
        }
    }

    return erased;
}


