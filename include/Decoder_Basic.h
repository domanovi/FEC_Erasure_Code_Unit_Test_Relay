/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Decoder_Basic.h
 * Author: silas
 *
 * Created on March 15, 2018, 12:16 PM
 */

#ifndef DECODER_BASIC_H
#define DECODER_BASIC_H

#include "Decoder_Block_Code.h"

class Decoder_Basic {

public:
    Decoder_Basic(unsigned char *generator, int T_value, int k_value, int n_value);
    
    int decodeStream(unsigned char *codeword, unsigned char *data, bool erasure, int t);

    virtual ~Decoder_Basic();

private:
    
    unsigned char *G;

    int T, k, n;

    int dataInternalCounter;               //increase by 1 per each codeword output

    Decoder_Block_Code **decoder_blk;

};

#endif /* DECODER_BASIC_H */

