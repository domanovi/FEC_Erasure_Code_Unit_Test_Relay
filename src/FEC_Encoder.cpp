/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   FEC_Encoder.cpp
 * Author: silas
 * 
 * Created on April 10, 2018, 1:11 PM
 */

#include "codingOperations.h"
#include "Memory_Allocator.h"
#include <random>
#include <cstdlib>
#include "basicOperations.h"
#include "FEC_Encoder.h"
#include <cstring>

FEC_Encoder::FEC_Encoder(int max_payload_value, int T_value, int B_value, int N_value, Memory_Allocator *memory) {

    counter = 0;
    max_payload = max_payload_value;
    T = T_value;
    B = B_value;
    N = N_value;
    k = T - N + 1;
    n = k + B;
    max_blocklength = ceil(float(max_payload + 2) / k) * n;

    encoder = new Encoder(T, B, N, max_payload);

    memory_object = memory;
}

FEC_Encoder::~FEC_Encoder() {
    delete encoder;
}

unsigned char *FEC_Encoder:: onTransmit(unsigned char *data, int payload, int seq, int *codeword_size_value) {
    //Elad:
    //data=message->buffer
    //payload=message->size
    //seq=seq message->seq_number,
    //*codeword_size_value = &codeword_size_current
    unsigned char *codeword_internal = memory_object->allocate_memory(max_blocklength);
    //codeword_internal points to a buffer that is used by the encoder object for encoding with memory

    encoder->encodeStream(data, codeword_internal, payload, seq);

    int codeword_size;

    for (codeword_size = max_blocklength - 1;; codeword_size--)
        if (codeword_internal[codeword_size] != 0x0) {
            codeword_size++;
            break;
        }
    *codeword_size_value = codeword_size;

    unsigned char *codeword = memory_object->allocate_memory(codeword_size);         //memory for transmission
    //for (i = 0; i < codeword_size; i++)
        //codeword[i] = codeword_internal[i];
    memcpy(codeword, codeword_internal, size_t(codeword_size));

    return codeword;
}

