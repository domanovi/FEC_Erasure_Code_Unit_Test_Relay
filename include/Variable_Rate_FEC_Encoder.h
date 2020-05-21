/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Variable_Rate_FEC_Encoder.h
 * Author: silas
 *
 * Created on July 12, 2018, 11:12 AM
 */

#ifndef VARIABLE_RATE_FEC_ENCODER_H
#define VARIABLE_RATE_FEC_ENCODER_H

#include "FEC_Encoder.h"
#include "Memory_Allocator.h"
#include "FEC_Message.h"
#include "Decoder_Symbol_Wise.h"
#include <fstream>
#include <string>

namespace siphon {

    class Variable_Rate_FEC_Encoder {
    public:

        Variable_Rate_FEC_Encoder(int max_payload_value, string file_name_input);

        virtual ~Variable_Rate_FEC_Encoder();

        void encode(FEC_Message *message, int T_ack, int B_ack, int N_ack, int flag);

        void onReceivedMessage(FEC_Message *message,int flag);

        FEC_Encoder *encoder_current, *encoder_old;

        int T2_ack, B2_ack, N2_ack;

        int T, B, N, max_payload;                 //transition counter varies from 0 to T; if it is < T, that means the old FEC_Encoder still has a codeword to transmit (concatenated to the new codeword); if it equals T, that means the old FEC_Encoder has nothing to send
        int T2, B2, N2;
    private:

        int B_old, N_old;

        int number_of_encoded_total,counter_encoded, final_number_of_encoded_total, final_counter_encoded;

        float sum_coding_rate, final_sum_coding_rate;

        float sum_coding_rate_min_2_seg, final_sum_coding_rate_min_2_seg;

        float MDS_percent, adaptive_percent, no_coding_percent;

        //FEC_Encoder *encoder_current, *encoder_old;

        Memory_Allocator *memory_object;

        string file_name;

        ofstream file_write_encoder;

        int counter_transition;

        bool transition_flag;          //it equals 1 if counter_transition is between 0 and T; otherwise it equals 0

        bool double_coding_flag;     //it equals 1 if counter_transition is between 0 and T-1; otherwise it equals 0};

        bool display_final_coding_rate_flag;

        int report_window_size;

    };


}
#endif /* VARIABLE_RATE_FEC_ENCODER_H */

