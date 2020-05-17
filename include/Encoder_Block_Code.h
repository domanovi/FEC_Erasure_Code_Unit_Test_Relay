/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Encoder_Block_Code.h
 * Author: silas
 *
 * Created on March 9, 2018, 10:17 AM
 */

#ifndef ENCODER_BLOCK_CODE_H
#define ENCODER_BLOCK_CODE_H

class Encoder_Block_Code{
public:

    Encoder_Block_Code(unsigned char *generator, int T_value, int B_value, int N_value, int k_value, int n_value);

    virtual ~Encoder_Block_Code();
    
    void encodeSymbol(unsigned char single_data, int t);            //encode symbol at time t
    
    void outputSymbol(unsigned char *symbol, int t);                   //output symbol at time t
   
    void outputGenerator(unsigned char *generator);                 //output G
    
private:
    
    int T, B, N, k, n;
    
    unsigned char *G;
    
    unsigned char *data, *codeword;

};

#endif /* ENCODER_BLOCK_CODE_H */

