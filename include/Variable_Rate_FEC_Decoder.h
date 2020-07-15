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
#include "Encoder.h"
#include "Decoder_Symbol_Wise.h"
#include "Payload_Simulator.h"
#include "Erasure_Simulator.h"

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
        FEC_Message ** burst_erased_message_vector;
        FEC_Message *message_old_encoder;

        FEC_Decoder *decoder_current;
        Memory_Allocator *memory_object;
        FEC_Decoder *decoder_old;
        int *erasure_counter;
        int *erasure_counter_total;
        int flag_for_burst,flag_for_burst_index;

        unsigned char **codeword_new_vector;
//        unsigned char **codeword_new_vector_new_decoder;
        unsigned char **codeword_vector_store_in_burst;
        unsigned char codeword_new_symbol_wise[30000];
        unsigned char **codeword_vector_to_transmit_stored;
        int codeword_vector_to_transmit_stored_index;
        int codeword_vector_to_transmit_stored_word_size[MAX_BURST_SIZE_MWDF];
        int codeword_vector_to_transmit_stored_seq[MAX_BURST_SIZE_MWDF];
        int codeword_vector_to_transmit_stored_counter_for_start_and_end[MAX_BURST_SIZE_MWDF];

        unsigned char **message_vector_to_transmit_stored;
        int message_vector_to_transmit_stored_index;
        int message_vector_to_transmit_stored_seq[MAX_BURST_SIZE_MWDF];

        Payload_Simulator *payload_simulator;
        unsigned char *raw_data;

        void receive_message_and_symbol_wise_encode(FEC_Message *message, int n, int k, int n2, int k2, int temp_size,int *codeword_size_final);

        void receive_message_and_symbol_wise_decode(FEC_Message *message, int n, int k, int temp_size,siphon::Erasure_Simulator *erasure_simulator);

        void calc_missed_chars(int received_seq, unsigned char *temp_buffer);

        void receive_message_and_state_dependent_symbol_wise_encode(FEC_Message *message, int n,
                                                                                               int k, int n2_new, int k2_new,
                                                                                               int temp_size, int *codeword_size_final);

        void receive_message_and_state_dependent_symbol_wise_decode(FEC_Message *message,int n,int k,int temp_size,siphon::Erasure_Simulator
        *erasure_simulator);

        Decoder_Symbol_Wise *decoder_Symbol_Wise;
        Decoder_Symbol_Wise *decoder_Symbol_Wise_new;
        Decoder_Symbol_Wise *decoder_Symbol_Backup;
        bool trans_vec[MAX_BURST_SIZE_MWDF];


        int n2_old;
        size_t final_counter_loss_of_char,final_counter_loss_of_char_elad;
        size_t counter_loss_of_full_packet,final_counter_loss_of_full_packet;
        size_t final_counter_loss_of_packets_swdf;

        float debug_rate_second_hop;
        float debug_rate_second_hop_curr;
        int debug_rate_second_hop_num_packets;

        void decode_erased_packet(FEC_Message *message);

    private:

        int T{}, B{}, N{}, max_payload;
        int T2, B2, N2;
        int n_old,k_old;
        int k2_old;
        int n_last_used,k_last_used;
        int n2_last_used,k2_last_used;

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
//        unsigned char **codeword_vector_new_decoder;

        Encoder *encoder;

        bool *temp_erasure_vector;

        /**
  * @brief The counter which keeps track of the received number of KBs.
  */
        size_t counter_, loss_counter_, UDP_loss_counter_, final_loss_counter_, final_UDP_loss_counter_;
        size_t loss_counter_two_seg_, final_loss_counter_two_seg_;
        size_t counter_loss_of_char;
        /**
         * @brief Time of last report of data receiving rate.
         */
        boost::posix_time::ptime last_report_time_, start_time_;

        int report_window_size;

        int erasure_length_cap;

        void symbol_wise_encode(int k, int n, unsigned char *generator_s_r, unsigned char *generator_r_d, int temp_size, int k2, int n2);

        void symbol_wise_decode(int k, int n, unsigned char *generator, int temp_seq);

        void symbol_wise_decode(int k, int n, unsigned char *generator, int temp_seq, size_t *loss_counter_,
                                size_t *final_loss_counter_);

    };

}
#endif /* VARIABLE_RATE_FEC_DECODER_H */


