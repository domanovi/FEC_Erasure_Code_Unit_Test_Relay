/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   FEC_Decoder.h
 * Author: silas
 *
 * Created on April 10, 2018, 1:11 PM
 */

#ifndef FEC_DECODER_H
#define FEC_DECODER_H

#include<random>
#include <fstream>
#include <string>
#include"Decoder.h"
#include"FEC_Message.h"
#include "Memory_Allocator.h"

using std::string;
using std::ofstream;

class FEC_Decoder {
public:
    
    FEC_Decoder(int max_payload_value, int T_value, int B_value, int N_value, Memory_Allocator *memory);

    virtual ~FEC_Decoder();
    
    void decode(FEC_Message *message);
    
   unsigned char* onReceive(unsigned char *codeword_received, int codeword_size, int seq, int *payload, bool erasure);

    Decoder *decoder;
private:

    int k, n, T, B, N, max_payload, max_blocklength;

    Memory_Allocator *memory_object;

    unsigned char *data_with_header;
};

#endif /* FEC_DECODER_H */

