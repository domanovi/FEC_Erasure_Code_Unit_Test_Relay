/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Decoder_Block_Code.h
 * Author: silas
 *
 * Created on March 9, 2018, 1:29 PM
 */

#ifndef DECODER_BLOCK_CODE_H
#define DECODER_BLOCK_CODE_H

class Decoder_Block_Code {
    
public:
    Decoder_Block_Code(unsigned char *generator, int k_value, int n_value, int T_value);
    
    virtual ~Decoder_Block_Code();
    
    void decodeSymbol(unsigned char symbol, bool erasure_value, int t);                    //receive symbol at time t and decodes the symbol transmitted at t-T
    
    unsigned char outputSymbol(int t); //t denotes sequence number from 0 to n-1
    
    bool outputErasure(int t);

private:
     
    int k, n, T;
    
    unsigned char *G;
    
    unsigned char *data, *codeword;
    
    bool *erasure;

};

#endif /* DECODER_BLOCK_CODE_H */

