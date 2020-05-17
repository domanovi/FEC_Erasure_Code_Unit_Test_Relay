/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   FEC_Message.h
 * Author: silas
 *
 * Created on May 24, 2018, 10:54 AM
 */

#ifndef PACKET_H
#define PACKET_H

class FEC_Message {
public:
    FEC_Message();

    void set_parameters(int seq_number_value, int T_value, int B_value, int N_value, int size_value, unsigned char *buffer_ptr);

    virtual ~FEC_Message();

    int seq_number, T, B, N, counter_for_start_and_end, size;//0<=N<=B<=T<=11

    int seq_number2;

    unsigned char *buffer;

};

#endif /* PACKET_H */

