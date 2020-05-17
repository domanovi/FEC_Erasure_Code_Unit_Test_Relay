/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Encoder_Basic.h
 * Author: silas
 *
 * Created on March 9, 2018, 9:12 AM
 */

#ifndef ENCODER_BASIC_H
#define ENCODER_BASIC_H
#include "Encoder_Block_Code.h"             //include the encoder class for block code 

class Encoder_Basic {
    
public:
    Encoder_Basic(unsigned char *generator, int T_value, int B_value, int N_value, int k_value, int n_value);                                  //constructor
    
    virtual ~Encoder_Basic();                         //destructor
    
    void encodeStream(unsigned char* data, unsigned char* codeword, int t);
    
private:
    
    int T, B, N, k, n;

    unsigned char *G;
    
    Encoder_Block_Code **encoder_blk;
};

#endif /* ENCODER_BASIC_H */

