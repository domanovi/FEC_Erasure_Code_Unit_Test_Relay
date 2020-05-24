/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Encoder.h
 * Author: silas
 *
 * Created on March 19, 2018, 1:25 PM
 */

#ifndef ENCODER_H
#define ENCODER_H
#include "Encoder_Basic.h"

class Encoder {
public:
    Encoder(int T_value, int B_value, int N_value, int max_payload_value);

    void encodeStream(unsigned char *data, unsigned char *codeword, int payload, int t);

    virtual ~Encoder();

    unsigned char *getG();

    int T;
    int B;
    int N;
    int max_payload;
private:

    int k, n, number_basic_encoder, max_blocklength, optimality;

    int max_payload_with_header;

    unsigned char *G;

    Encoder_Basic **encoder_basic;


};

#endif /* ENCODER_H */

