/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Encoder.cpp
 * Author: silas
 * 
 * Created on March 19, 2018, 1:25 PM
 */

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <random>       //for std::ceil()
#include <cstring>    //for memcpy
#include "codingOperations.h"       // coding operations including block encoding and decoding
#include "Encoder_Basic.h"
#include "Encoder.h"

using std::cout;
using std::endl;

Encoder::Encoder(int T_value, int B_value, int N_value, int max_payload_value) {

    T = T_value;
    B = B_value;
    N = N_value;
    k = T - N + 1;
    n = k + B;
    max_payload = max_payload_value;
    max_payload_with_header = max_payload + 2;   // a 2-byte header to storing the length of payload of each packet

    cout << "(T,B,N)=" << "(" << T << "," << B << "," << N << ")" << endl;   // display the selected parameters
    cout << "k=" << k << " and n=" << n << endl;                         // display the optimal k=T-N+1 and n=k+B

    max_blocklength = ceil((float) (max_payload_with_header) / k) * n;

    G = (unsigned char *) malloc(k * n * sizeof(unsigned char));
    optimality = init_at_sender(T, B, N, G, k,
                                n);              // if optimality=1, G is optimal; an optimal G is randomly generated in 20 trials;

    number_basic_encoder = max_blocklength / n;
    encoder_basic = (Encoder_Basic **) malloc(number_basic_encoder * sizeof(Encoder_Basic *));

    for (int i = 0; i < number_basic_encoder; i++)
        encoder_basic[i] = new Encoder_Basic(G, T, B, N, k, n);
}

Encoder::~Encoder() {

    for (int i = 0; i < number_basic_encoder; i++)
        delete encoder_basic[i];

    free(encoder_basic);
    free(G);
}

unsigned char * Encoder::getG(){
    return G;
}

void Encoder::encodeStream(unsigned char *data, unsigned char *codeword, int payload, int t) {
    //Elad:
    //data=data
    //codeword=codeword_internal
    //payload=payload
    //t=seq

    int i;
    int payload_with_header = payload + 2;
    int lastIndexWithData = ceil(float(payload_with_header) / k) - 1;
    unsigned char data_with_header[max_payload_with_header];

    //for (i = 0; i < payload; i++)
        //data_with_header[i + 2] = data[i];

    memcpy(data_with_header+2, data, size_t(payload));

    data_with_header[1] = unsigned(payload % 256);
    data_with_header[0] = unsigned((payload - payload % 256) / 256);

    for (i = 0; i < lastIndexWithData + 1; i++)
        encoder_basic[i]->encodeStream(data_with_header + i * k, codeword + i * n, t);

    unsigned char temp_data[k];

    //for (i = 0; i < k; i++)
       // temp_data[i] = 0x0;
    memset(temp_data, 0, size_t(k));

    for (i = lastIndexWithData + 1; i < number_basic_encoder; i++)
        encoder_basic[i]->encodeStream(temp_data, codeword + i * n, t);

    return;                                       // return the number of non-zero bytes
}

