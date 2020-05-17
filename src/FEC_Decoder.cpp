/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   FEC_Decoder.cpp
 * Author: silas
 * 
 * Created on April 10, 2018, 1:11 PM
 */

#include<random>
#include <cstdlib>
#include <iostream>
#include "codingOperations.h"
#include "basicOperations.h"
#include "Memory_Allocator.h"
#include "FEC_Decoder.h"
#include <cstring>

using std::cout;
using std::endl;

FEC_Decoder::FEC_Decoder(int max_payload_value, int T_value, int B_value, int N_value, Memory_Allocator *memory) {

    max_payload = max_payload_value;
    T = T_value;
    B = B_value;
    N = N_value;
    k = T - N + 1;
    n = k + B;
    max_blocklength = ceil(float(max_payload + 2) / k) * n;

    decoder = new Decoder(T, B, N, max_payload);

    memory_object = memory;

    data_with_header = (unsigned char *) malloc(sizeof(unsigned char) * (max_blocklength * k / n));
}

FEC_Decoder::~FEC_Decoder() {
    free(data_with_header);
    delete decoder;
}

unsigned char *
FEC_Decoder::onReceive(unsigned char *codeword_input, int codeword_size, int seq, int *payload, bool erasure) {

    int data_size = 0;

    if (erasure == 0) {
        //Record the codeword
        unsigned char *codeword = memory_object->allocate_memory(max_blocklength);

        // for (i = 0; i < codeword_size; i++)
        // codeword[i] = codeword_input[i];
        memcpy(codeword, codeword_input, size_t(codeword_size));

        //for (i = codeword_size; i < max_blocklength; i++)
        //codeword[i] = 0x0;
        memset(codeword + codeword_size, 0, size_t(max_blocklength - codeword_size));

        data_size = decoder->decodeStream(codeword, data_with_header, 0, seq);
    } else
        data_size = decoder->decodeStream(NULL, data_with_header, 1, seq);

    *payload = data_size;

    return data_with_header + 2;      // return the actual data excluding the 2-byte header
}







