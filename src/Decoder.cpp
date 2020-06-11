/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Decoder.cpp
 * Author: silas
 * 
 * Created on March 19, 2018, 1:25 PM
 */

#include <cstdlib>
#include <iostream>
#include <random>
#include <cstring>
#include "codingOperations.h"
#include "Decoder.h"

using std::cout;
using std::endl;

Decoder::Decoder(int T_value, int B_value, int N_value, int max_payload_value) {

    T = T_value;
    B = B_value;
    N = N_value;
    k = T - N + 1;
    n = k + B;

    latest_erasure_seq = -1;

    G = (unsigned char *) malloc(k * n * sizeof(unsigned char));

    init_at_sender(T, B, N, G, k, n); // generate G

    max_payload = max_payload_value;
    max_payload_with_header = max_payload_value + 2;
    max_blocklength = ceil((float) (max_payload_with_header) / k) * n;

    cout << "(T,B,N)=" << "(" << T << "," << B << "," << N << ")" << endl;   // display the selected parameters
    cout << "k=" << k << " and n=" << n << endl;                         // display the optimal k=T-N+1 and n=k+B

    int i;
    number_basic_encoder = max_blocklength / n;
    decoder_basic = (Decoder_Basic **) malloc(number_basic_encoder * sizeof(Decoder_Basic *));
    for (i = 0; i < number_basic_encoder; i++)
        decoder_basic[i] = new Decoder_Basic(G, T, k, n);

    internal_codeword_ptr = (unsigned char **) malloc(n * sizeof(unsigned char *));

    for (i = 0; i < n; i++)
        internal_codeword_ptr[i] = NULL;
}

Decoder::~Decoder() {

    for (int i = 0; i < number_basic_encoder; i++)
        delete decoder_basic[i];

    free(decoder_basic);
    free(G);
    free(internal_codeword_ptr);

}

unsigned char * Decoder::getG(){
    return G;
}

int Decoder::decodeStream(unsigned char *codeword, unsigned char *data_with_header, bool erasure,
                          int t) { //output data for packet t-T, input codeword t if erasure=0; the function return payload. If payload=0, the packet cannot be recovered

    int i, j, payload = 0;

    if (erasure == 0) {
        internal_codeword_ptr[t % n] = codeword; //store codeword

        if (t - latest_erasure_seq > T) // if the last erasure moves out of the sliding window
            latest_erasure_seq = -1;

        if (latest_erasure_seq == -1) { // in this case, copy the data from the stored codeword and then return function

            int tempIndex = t % n - T; //index for codeword t-T
            if (tempIndex < 0)
                tempIndex += n;

            if (internal_codeword_ptr[tempIndex] != NULL) {

                if (k > 1)
                    payload =
                            (int(internal_codeword_ptr[tempIndex][0])) * 256 + int(internal_codeword_ptr[tempIndex][1]);
                else
                    payload =
                            (int(internal_codeword_ptr[tempIndex][0])) * 256 + int(internal_codeword_ptr[tempIndex][n]);

                int number_of_blocks_needed = ceil(float(payload + 2) / k);
                for (j = 0; j < number_of_blocks_needed; j++) {
                    for (i = 0; i < k; i++) {
                        data_with_header[j * k + i] = internal_codeword_ptr[tempIndex][j * n + i];
                    }
                }
            } else
                payload = 0;

            return payload;
        }
    } else {
        // if erasure==1
        if (latest_erasure_seq == -1) {

            int temp_counter = t % n;            // = t%n - n
            for (i = 0; i < n - T; i++, temp_counter++) {
                if (temp_counter >= n)
                    temp_counter -= n;
                for (j = 0; j < number_basic_encoder; j++)
                    decoder_basic[j]->decodeStream(NULL, NULL, 1, temp_counter);
            }

            temp_counter = t % n - T;
            if (temp_counter < 0)
                temp_counter += n;
            for (i = 0; i < T; i++, temp_counter++) {
                if (temp_counter >= n)
                    temp_counter -= n;

                if (internal_codeword_ptr[temp_counter]) { //just store the codeword in the decoder for future decoding; outputting the message at time temp_counter-T is not needed
                    for (j = 0; j < number_basic_encoder; j++)
                        decoder_basic[j]->decodeStream(internal_codeword_ptr[temp_counter] + j * n, NULL, 0,
                                                       temp_counter);
                }
            }
        }
        latest_erasure_seq = t; // record the erasure position before which there is no erasure inside the sliding window
    }

    //the following code will run whenever latest_erasure_seq not equal to -1
    int erased = decoder_basic[0]->decodeStream(codeword, data_with_header, erasure, t);
    if (erased == 0) {

        if (k == 1)
            decoder_basic[1]->decodeStream(codeword + n, data_with_header + k, erasure, t);

        payload = (int(data_with_header[0])) * 256 + int(data_with_header[1]); //payload for packet t-T
        ////////////////////////////////////////////////////
        // TODO - currently the destination may try to recover messages that were erased at the relay. This makes sure there will be no drift in massage size !!!
        if (payload>max_payload)
            payload=max_payload;
        /////////////////////////////////////////////////
        int payload_with_header = payload + 2;
        int lastIndexWithData = ceil(float(payload_with_header) / k) - 1;

        if ((k > 1) && (lastIndexWithData > 0))
            decoder_basic[1]->decodeStream(codeword + n, data_with_header + k, erasure, t);

        for (j = 2; j < lastIndexWithData + 1; j++)
            decoder_basic[j]->decodeStream(codeword + j * n, data_with_header + j * k, erasure, t);

        // for the remaining subblocks

        for (j = lastIndexWithData + 1; j < number_basic_encoder; j++)
            decoder_basic[j]->decodeStream(codeword + j * n, NULL, erasure, t);
    } else {
        payload = 0;

        for (j = 1; j < number_basic_encoder; j++)
            decoder_basic[j]->decodeStream(codeword + j * n, NULL, erasure, t);
    }

    if (erased == 1)
        return 0;
    else
        return payload;
}
