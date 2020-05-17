/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   codingOperations.h
 * Author: silas
 *
 * Created on March 5, 2018, 3:23 PM
 */

#ifndef CODINGOPERATIONS_H
#define CODINGOPERATIONS_H

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fstream>

using std::cout;
using std::endl;
using std::string;
using std::ofstream;

int init_at_sender(int T, int B, int N, unsigned char *G, int k, int n);

void encodeBlock(unsigned char *data, unsigned char *generator, unsigned char *codeword, int k, int n, int t);

void decodeBlock(unsigned char *data, unsigned char *generator, unsigned char *codeword, bool *erasure, int k, int n, int T, int t);

void generateData(unsigned char *data, int k);

void gen_G_cauchy(unsigned char *G, int T, int B, int N, int k, int n);

void save_to_file(unsigned char *data, int payload, ofstream* file);

float calculateLoss(unsigned char *data, unsigned char *recovered_data, int max_payload, int *payload, int stream_duration, int T);

float calculateLossMessage(string file_original, string file_recovered);

#endif /* CODINGOPERATIONS_H */

