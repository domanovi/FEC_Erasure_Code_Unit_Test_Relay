/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Decoder.h
 * Author: silas
 *
 * Created on March 19, 2018, 1:25 PM
 */

#ifndef DECODER_H
#define DECODER_H

#include "Decoder_Basic.h"

class Decoder {
public:
    Decoder(int T_value, int B_value, int N_value, int max_payload_value);

   int decodeStream(unsigned char *codeword, unsigned char *data, bool erasure, int t);

    virtual ~Decoder();

    unsigned char *getG();

private:

    int T, B, N, k, n, max_payload, max_payload_with_header, max_blocklength, number_basic_encoder;
    
    int latest_erasure_seq;
    
    unsigned char *G;
    
    unsigned char **internal_codeword_ptr;
    
    Decoder_Basic **decoder_basic;


};

#endif /* DECODER_H */

