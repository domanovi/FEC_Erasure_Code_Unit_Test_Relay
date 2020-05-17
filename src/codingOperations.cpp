/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <typeinfo>
#include <random>
#include <bitset>           // for integer to binary operation
#include <cstring> //memcpy
#include "codingOperations.h"
#include "basicOperations.h"
#include "isa-l.h"

using std::cout;
using std::endl;
using std::cin;
using std::stringstream;
using std::random_device;
using std::mt19937; //Standard mersenne_twister_engine
using std::uniform_int_distribution;
using std::ifstream;
using std::ios;

void save_to_file(unsigned char *data, int payload, ofstream *file) {

  /*  unsigned char payload_first_digit, payload_second_digit;
    payload_second_digit = unsigned(payload % 256);
    payload_first_digit = unsigned((payload - payload % 256) / 256);

    file->write((char *) &payload_first_digit, 1);
    file->write((char *) &payload_second_digit, 1);
    */
    if ((payload > 0) && (data != NULL))
        file->write((char *) data, payload);
    else{
        unsigned char zero[payload];
        for(int i=0; i<payload; i++)
            zero[i]=0x0;
        file->write((char *) zero, payload);
    }

    return;
}

void gen_G_cauchy(unsigned char *G, int T, int B, int N, int k, int n) {

    int i, j;
    unsigned char Gtranspose[n * k];

    if (((T == 10) && (B == 8) && (N == 4)) || ((T == 11) && (B == 5) && (N == 4))) //works for all T<=11
        gf_gen_rs_matrix(Gtranspose, n, k);
    else
        gf_gen_cauchy1_matrix(Gtranspose, n, k);

    gf256_transpose(Gtranspose, G, n, k);

    if (B == 0) // if B equals 0, then rate equals 1
        return;

    if (2 * k >= n) { // high rate regime
        // getting the right structure of zeros for the first B-N columns of G
        for (i = 0; i < B - N; i++) {
            // row i
            for (j = k + N + i; j < n; j++)
                G[i * n + j] = (unsigned char) 0;

            for (j = 0; j < i; j++)
                G[i * n + k + j] = (unsigned char) 0;
        }

        for (i = B - N; i < B; i++) {//row i
            for (j = 0; j < B - N; j++)
                G[i * n + k + j] = (unsigned char) 0;
        }

    } else { //low rate regime, k/n<0.5
        // getting the right structure of zeros for the first B-N columns of G
        for (i = 0; i < B - N; i++) { // row i
            for (j = k + N + i; j < n; j++)
                G[i * n + j] = (unsigned char) 0;

            for (j = 0; j < i; j++)
                G[i * n + B + j] = (unsigned char) 0;
        }

        for (i = B - N; i < k; i++) {
            for (j = 0; j < B - N; j++)
                G[i * n + B + j] = (unsigned char) 0;
        }
    }
    return;
}

void gen_G_random(unsigned char *G, int k, int n) {

    int i, j;
    random_device rd; //Will be used to obtain a seed for the random number engine
    mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()

    uniform_int_distribution<int> distribution(1, 255);
    for (i = 0; i < k; i++) {
        for (j = k; j < n; j++) {
            if (G[i * n + j] != 0x0)
                G[i * n + j] = (unsigned char) distribution(gen);
        }
    }
    return;
}

int init_at_sender(int T, int B, int N, unsigned char *G, int k, int n) {
    gen_G_cauchy(G, T, B, N, k, n);
    return 1;
}

void generateData(unsigned char *data, int payload) {

    int i;
    random_device rd; //Will be used to obtain a seed for the random number engine
    mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    uniform_int_distribution<int> distribution(0, 255);

    for (i = 0; i < payload; i++)
        data[i] = (unsigned char) distribution(gen);

    return;
}

void encodeBlock(unsigned char *data, unsigned char *generator, unsigned char *codeword, int k, int n, int t) {

    int i, j;
    codeword[t] = 0x0;
    for (i = 0; i < k; i++)
        codeword[t] ^= gf256_mul(data[i], generator[i * n + t]);


    if (t == k - 1) {//encode the rest
        for (j = k; j < n; j++) {
            codeword[j] = 0x0;
            for (i = 0; i < k; i++)
                codeword[j] ^= gf256_mul(data[i], generator[i * n + j]);
        }
    }
    return;
}

void
decodeBlock(unsigned char *data, unsigned char *generator, unsigned char *codeword, bool *erasure_dec, int k, int n,
            int T,
            int t) { //modify the noisy codeword with actual data; modify the erasure pattern when a data is decoded

    if (t < k) {
        if (erasure_dec[t] == 0)
            data[t] = codeword[t];
        //else
          //  data[t] = 0x0;
    }

    int i, j, counter;
    int windowSize = t + T + 1; // set window size

    if (windowSize > n) // the maximum window size is n because the block length is n
        windowSize = n;

    unsigned char decMatrix[k * windowSize];

    // fill in the entries of the decoding matrix
    counter=0;
    for (j = 0; j < windowSize; j++) { //column j
        if (erasure_dec[j] == 1) {
            counter++;
            for (i = 0; i < k; i++)
                decMatrix[i * windowSize + j] = 0x0;
        } else {
            for (i = 0; i < k; i++)
                decMatrix[i * windowSize + j] = generator[i * n + j];
        }
    }
    if(counter==windowSize)                 // if the window is filled with erasures, no need to perform reduced row echelon form
        return;

    unsigned char rref[k * windowSize];
    unsigned char decCodeword[windowSize];
    unsigned char decData[windowSize]; // a decoder version of the decoded data

    // fill in the entries of the decoding codeword

    //for (j = 0; j < windowSize; j++) //column j
       // decCodeword[j] = codeword[j];
    memcpy(decCodeword, codeword, size_t(windowSize));

    unsigned char action[windowSize * windowSize];
    gf256_rref_matrix(decMatrix, rref, action, k,
                      windowSize); // perform reduced row echelon form and store the action matrix

   // used for debugging only; unsigned char decProduct[k * windowSize];  gf256_matrix_mul(decMatrix, action, decProduct, k, windowSize, windowSize);

   gf256_matrix_mul(decCodeword, action, decData, 1, windowSize, windowSize);

    // check whether a symbol can be decoded by looking at the rref matrix

    for (i = 0; i < k; i++) {

        if (erasure_dec[i] == 0)
            continue;

        for (j = i; j < k; j++) {
            if (rref[i * windowSize + j] == 0x1)
                break;
        }

        if (j == k) //cannot find a nonzero element corresponding to data i
            continue;

        //  cout<< unsigned(rref[i*windowSize+j]);

        for (counter = i + 1;
             counter < k; counter++) //the case where j is the column index where element (i,j) equals 1)
            if (rref[counter * windowSize + j] != 0x0)
                break;

        if (counter == k) { //i can be decoded if the column is a unit vector

            erasure_dec[i] = false;
            data[i] = decData[j];
            codeword[i] = data[i]; //modify the noisy codeword with actual data
        }
    }
    return;
}

float
calculateLoss(unsigned char *data, unsigned char *recovered_data, int max_payload, int *payload, int stream_duration,
              int T) {

    int i, t;
    float loss_probability = 0;

    for (t = 0; t < stream_duration; t++) {
        for (i = 0; i < payload[t]; i++)
            if (data[t * max_payload + i] != recovered_data[t * max_payload + i]) {
                //       cout<<"data =" << unsigned(data[t*max_payload+i])<< " recovered = "<< unsigned(recovered_data[t*max_payload+i])<<endl;
                loss_probability++;
                break;
            }
    }

    return loss_probability / (stream_duration - T);

}

float calculateLossMessage(string file_original, string file_recovered) {

    float loss = 0;
    int total_decoded = 0;

    ifstream file_read_encoder;
    file_read_encoder.open(file_original, ios::in | ios::binary);
    ifstream file_read_decoder;
    file_read_decoder.open(file_recovered, ios::in | ios::binary);

    //file_read.seekg(0, ios::end);                           //find file size
    //fileSize = file_read.tellg();
    //file_read.seekg(0, ios::beg);                           //set file pointer to the beginning
    //current_file_position = 0;
    //read_counter = 1;

    file_read_encoder.seekg(0, ios::end);           //find file size
    long fileSize_input = file_read_encoder.tellg();
    file_read_encoder.seekg(0, ios::beg);                           //set file pointer to the beginning

    file_read_decoder.seekg(0, ios::end);
    long fileSize_output = file_read_decoder.tellg();
    file_read_decoder.seekg(0,ios::beg);

    if (fileSize_input!=fileSize_output) {
        cout << "THe two files have different sizes!" << endl;
        return 1;
    }

    unsigned char buffer_input[fileSize_input];
    unsigned char buffer_output[fileSize_output];
   file_read_encoder.read((char *) buffer_input, fileSize_input);
    file_read_decoder.read((char *) buffer_output, fileSize_output);

    for(int i=0; i<fileSize_input;i ++){
        if(buffer_input[i]!=buffer_output[i])
            loss++;
    }

    file_read_encoder.close();
    file_read_decoder.close();

    return loss / fileSize_input;
}

