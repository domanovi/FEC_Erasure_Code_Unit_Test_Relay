/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   FEC_Message.cpp
 * Author: silas
 * 
 * Created on May 24, 2018, 10:54 AM
 */

#include "FEC_Message.h"
#include <cstdlib>
#include <iostream>

using std::cout;
using std::endl;

FEC_Message::FEC_Message() {

    size = 0;
    counter_for_start_and_end = 0;
    buffer = NULL;
}

void FEC_Message::set_parameters(int seq_number_value, int T_value, int B_value, int N_value, int size_value, unsigned char *buffer_ptr) {

    seq_number = seq_number_value;
    size = size_value;
    T = T_value;
    B = B_value;
    N = N_value;
    buffer = buffer_ptr;

    return;
}

FEC_Message::~FEC_Message() {
}

