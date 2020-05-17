/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Variable_Rate_FEC_Decoder.h
 * Author: silas
 *
 * Created on July 12, 2018, 11:12 AM
 */

#ifndef VARIABLE_RATE_FEC_DECODER_H
#define VARIABLE_RATE_FEC_DECODER_H

#include<string>
#include <fstream>
#include"boost/date_time/posix_time/posix_time.hpp"

#include "FEC_Decoder.h"
#include "Memory_Allocator.h"

namespace siphon {

    class VariableRateDecoder;

    class Variable_Rate_FEC_Decoder {

    public:
        Variable_Rate_FEC_Decoder(int max_payload_value, string file_name_input, int erasure_recorder_value);

        void decode(FEC_Message *message);

        virtual ~Variable_Rate_FEC_Decoder();

        void onDecodedMessage(FEC_Message *message);

        void initialize_decoder(FEC_Message *message);

        void decode_for_erased_codeword(FEC_Message *recovered_message, int seq, FEC_Decoder *decoder);

        void decode_for_current_codeword(FEC_Message *message, unsigned char *codeword_received, int size_received,
                                         int received_seq, FEC_Decoder *decoder);

        void update_decoder(FEC_Message *message);

        void display_udp_statistics(int seq_value);

        void display_fec_statistics(int seq_value);

        int receiver_index;

        FEC_Message *recovered_message;
        FEC_Message ** recovered_message_vector;
        FEC_Message *message_old_encoder;

        FEC_Decoder *decoder_current;
        Memory_Allocator *memory_object;
        FEC_Decoder *decoder_old;
        int *erasure_counter;
        int *erasure_counter_total;

        unsigned char **codeword_new_vector;
        unsigned char **codeword_vector_store_in_burst;
        unsigned char codeword_new_symbol_wise[30000];

        void receive_message_and_symbol_wise_encode(FEC_Message *message, int n, int k, int n2, int k2, int temp_size);

        void receive_message_and_symbol_wise_decode(FEC_Message *message, int n, int k, int temp_size);

    private:

        int T{}, B{}, N{}, max_payload;

        int latest_seq;
        int latest_seq_2;

        int seq_start, seq_start_double_coding, seq_end_double_coding;

        string file_name, file_erasures_recorded;

        ofstream file_write_decoder, file_write_erasures;

        bool double_coding_flag;

        bool display_final_loss_rate_flag;

        int erasure_recorder;

        int total_session_counter;

        int low_fidelity_session_counter_UDP;

        int disruption_session_counter_UDP;

        int low_fidelity_session_counter_FEC;

        int disruption_session_counter_FEC;

        unsigned char *codeword;
        unsigned char **codeword_vector;

        bool *temp_erasure_vector;

        /**
  * @brief The counter which keeps track of the received number of KBs.
  */
        size_t counter_, loss_counter_, UDP_loss_counter_, final_loss_counter_, final_UDP_loss_counter_;
        size_t loss_counter_two_seg_, final_loss_counter_two_seg_;
        /**
         * @brief Time of last report of data receiving rate.
         */
        boost::posix_time::ptime last_report_time_, start_time_;

        int report_window_size;

        int erasure_length_cap;

        void symbol_wise_encode(int k, int n, unsigned char *generator, int temp_size, int k2, int n2);

        void symbol_wise_decode(int k, int n, unsigned char *generator, int temp_seq);

    };

}
#endif /* VARIABLE_RATE_FEC_DECODER_H */


