/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   FEC_Encoder.h
 * Author: silas
 *
 * Created on April 10, 2018, 1:11 PM
 */

#ifndef FEC_ENCODER_H
#define FEC_ENCODER_H

#include <fstream>
#include <string>
#include "Encoder.h"
#include "FEC_Message.h"
#include "Memory_Allocator.h"

using std::string;
using std::ofstream;

class FEC_Encoder {

public:

    FEC_Encoder(int max_payload_value, int T_value, int B_value, int N_value, Memory_Allocator *memory);

    virtual ~FEC_Encoder();

    unsigned char *onTransmit(unsigned char *data, int payload, int seq, int *codeword_size);

    Encoder *encoder;
private:

    //Encoder *encoder;

    Memory_Allocator *memory_object;

    int T, B, N, k, n, max_payload, max_blocklength, counter;

    string file_name;

    ofstream file_write_encoder;
};


#endif /* FEC_ENCODER_H */

