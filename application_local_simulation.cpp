/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.cpp
 * Author: silas
 *
 * Created on February 21, 2018, 10:13 AM
 */

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <iomanip>      // std::setprecision()
#include <random>       //for std::ceil()
#include<unistd.h>
#include<chrono>

#include"boost/date_time/posix_time/posix_time.hpp"
#include "isa-l.h"            //Intel library
#include "basicOperations.h"        // basic GF(256) operations including reduced row echelon form and matrix printing
#include "codingOperations.h"       // coding operations including block encoding and decoding
#include "Variable_Rate_FEC_Encoder.h"
#include "Variable_Rate_FEC_Decoder.h"
#include "FEC_Message.h"
#include "Parameter_Estimator.h"
#include "Payload_Simulator.h"
#include "Erasure_File_Generator.h"
#include "Erasure_Simulator.h"
#include "FEC_Macro.h"
#include "Application_Layer_Sender.h"
#include "Application_Layer_Receiver.h"
#include "basicOperations.h"

using std::cout;
using std::cerr;
using std::endl;
using namespace std::chrono;

//int RELAYING_TYPE;

int main(int argc, const char *argv[]) {
    remove("packet_loss_MWDF.txt" );
    remove("packet_loss_SWDF.txt" );
    remove("packet_loss_SD_SWDF.txt" );
//    for (RELAYING_TYPE=1;RELAYING_TYPE<=2;RELAYING_TYPE++) {
        int B = N_INITIAL;                       //if B=-1 and N=-1, then it is adaptive; otherwise, it is non-adaptive
        int N = N_INITIAL;

        int B2 = N_INITIAL_2;                       //if B=-1 and N=-1, then it is adaptive; otherwise, it is non-adaptive
        int N2 = N_INITIAL_2;

        bool adaptive_mode_MDS = false;

        int erasure_type = ERASURE_TYPE;
        int relaying_type = RELAYING_TYPE;
        //***********************************************   fix the parameters below **********/

        const char *Tx = "127.0.0.1";
        const char *Rx = "127.0.0.1";

        const char *Tx2 = "127.0.0.1";
        const char *Rx2 = "127.0.0.1";

        int packet_size = 300;

        int seq_start = 0;

        int T, T2, k;
        unsigned char **received_data_vector;

        if (relaying_type == 0) {
            T = T_INITIAL;
            T2 = 0;
        } else if (relaying_type == 1) {
            // message wise
            T = T_INITIAL;
            T2 = T_INITIAL_2;
        } else if (relaying_type == 2 || relaying_type == 3) {
            if (N_INITIAL == -1)
                T = T_TOT;
            else
                T = T_TOT - N_INITIAL_2;
            if (N_INITIAL_2 == -1)
                T2 = T_TOT;
            else
                T2 = T_TOT - N_INITIAL;
//        T=T_INITIAL;
//        T2=T_INITIAL_2;
        }

        int stream_duration = NUMBER_OF_ITERATIONS;
        int max_payload = packet_size;

        Application_Layer_Sender application_layer_sender(Tx, Rx, packet_size, 0, T, B, N, 0);
        Application_Layer_Sender application_layer_relay_sender(Tx2, Rx2, packet_size, 0, T2, B2, N2, 1);

        application_layer_sender.variable_rate_FEC_encoder->T2 = T2;
        if (N2 == -1) {
            application_layer_sender.variable_rate_FEC_encoder->N2 = 0;
            application_layer_sender.variable_rate_FEC_encoder->B2 = 0;
        }
        Application_Layer_Receiver *application_layer_relay_receiver = new Application_Layer_Receiver(Tx, Rx,
                                                                                                      max_payload,
                                                                                                      erasure_type,
                                                                                                      adaptive_mode_MDS,
                                                                                                      0);
        Application_Layer_Receiver *application_layer_destination_receiver = new Application_Layer_Receiver(Tx2, Rx2,
                                                                                                            max_payload,
                                                                                                            erasure_type,
                                                                                                            adaptive_mode_MDS,
                                                                                                            1);
        application_layer_destination_receiver->set_receiver_index(1);

        siphon::Erasure_File_Generator erasure_generator;
        siphon::Erasure_File_Generator erasure_generator2;

        switch (erasure_type) {
            case 1:
                erasure_generator.generate_IID(stream_duration + T + T2, EPSILON, "erasure.bin", 0);
                erasure_generator2.generate_IID(stream_duration + T + T2, EPSILON, "erasure2.bin", 0);
                break;
            case 2:
                erasure_generator.generate_GE(stream_duration + T, ALPHA, BETA, EPSILON, "erasure.bin");
                break;
            case 3:
                erasure_generator.generate_GE_varying(stream_duration + T + T2, ALPHA, BETA, EPSILON, "erasure.bin");
                break;
            case 4:
                erasure_generator.generate_periodic(stream_duration + T, ERASURE_T, ERASURE_B, ERASURE_N,
                                                    "erasure.bin");
                break;
            case 5:
                break;                                            // In case 5, erasure.bin is assumed to be already present
            case 6:
                erasure_generator.generate_three_sections_IID(3000, EPSILON, 3000, EPSILON_2,
                                                              stream_duration - 3000 - 3000 + T + T2, EPSILON_3,
                                                              "erasure.bin");
                break;
            default:
                erasure_generator.generate_periodic(stream_duration + T, T, 0, 0, "erasure.bin");
        }
        erasure_generator.generate_three_sections_IID(30000, 0.1, 40000, 0, stream_duration - 30000 - 40000 + T + T2,
                                                      0.1, "erasure.bin");
//    erasure_generator.generate_three_sections_IID(4000,0,2000,0.33,stream_duration-4000-2000 + T+T2,0,"erasure.bin");
        erasure_generator2.generate_IID(stream_duration + T + T2, 0.1, "erasure2.bin", 2);


        siphon::Erasure_Simulator erasure_simulator("../Experimental_Logs/erasure50.bin");
        siphon::Erasure_Simulator erasure_simulator2("../Experimental_Logs/erasure20.bin");
//    siphon::Erasure_Simulator erasure_simulator("erasure.bin");
//    siphon::Erasure_Simulator erasure_simulator2("erasure2.bin");

        auto start_time = high_resolution_clock::now();;


//    erasure_simulator.erasure_seq[4]='\001';
//    erasure_simulator.erasure_seq[5]='\001';
    for (int i=360000;i<erasure_simulator2.number_of_erasure;i++) {
        erasure_simulator2.erasure_seq[i] = '\000';
    }
//
    for (int i=360000;i<erasure_simulator.number_of_erasure;i++) {
        erasure_simulator.erasure_seq[i] = '\000';
    }

//    for (int i=0;i<361000;i++) {
//        erasure_simulator2.erasure_seq[i] = '\000';
//    }
////
//    for (int i=0;i<361000;i++) {
//        erasure_simulator.erasure_seq[i] = '\000';
//    }
//    erasure_simulator.erasure_seq[1] = '\001';
//    erasure_simulator.erasure_seq[2] = '\001';
//    erasure_simulator.erasure_seq[3] = '\001';
//    erasure_simulator.erasure_seq[4] = '\001';

//////    erasure_simulator.erasure_seq[2] = '\001';
//    erasure_simulator2.erasure_seq[4] = '\001';
//    erasure_simulator2.erasure_seq[5] = '\001';
//    erasure_simulator.erasure_seq[6] = '\001';
//    erasure_simulator.erasure_seq[2] = '\001';
//    erasure_simulator.erasure_seq[3] = '\001';
//    erasure_simulator.erasure_seq[4]='\001';
//    erasure_simulator.erasure_seq[5]='\001';
//    erasure_simulator.erasure_seq[6]='\001';
//    erasure_simulator.erasure_seq[13]='\001';
//    erasure_simulator.erasure_seq[14]='\001';
//    erasure_simulator.erasure_seq[15]='\001';
//
//    erasure_simulator2.erasure_seq[8]='\001';
//    erasure_simulator2.erasure_seq[11]='\001';
//    erasure_simulator2.erasure_seq[12]='\001';
//    erasure_simulator.erasure_seq[5]='\001';
//    erasure_simulator.erasure_seq[6]='\001';
//    erasure_simulator.erasure_seq[7]='\001';
//    erasure_simulator2.erasure_seq[1]='\001';
//    erasure_simulator2.erasure_seq[3]='\001';
//    erasure_simulator2.erasure_seq[5]='\001';
//    erasure_simulator.erasure_seq[7]='\001';
//    erasure_simulator2.erasure_seq[8]='\001';
//    erasure_simulator.erasure_seq[9]='\001';
//    erasure_simulator.erasure_seq[10]='\001';
//    erasure_simulator2.erasure_seq[10]='\001';

        cout << "Iteration = " << stream_duration << endl;

        int seq_number;
        int seq_number2;

        unsigned char *udp_parameters = nullptr;
        unsigned char *udp_parameters2 = nullptr;

        udp_parameters = (unsigned char *) malloc(sizeof(unsigned char) * 12);
        udp_parameters2 = (unsigned char *) malloc(sizeof(unsigned char) * 6);

        for (int j = 0; j < 12; j++)
            udp_parameters[j] = '\000';
        for (int j = 0; j < 6; j++)
            udp_parameters2[j] = '\000';

        int buffer_size;
        int buffer_size2;
        unsigned char buffer[30000];
        unsigned char buffer2[30000];
        unsigned char received_data[30000];
        unsigned char data_to_transmit_in_relay[30000];
//    unsigned char *zero_data=(unsigned char *)malloc(sizeof(unsigned char *) * 300); // Elad to change
//    memset(zero_data,'\000',300);
        unsigned char response_from_dest_buffer[6];
        for (int i = 0; i < 6; i++)
            response_from_dest_buffer[i] = 0;
        int last_seq_received_from_srouce = -1;
//    int n2=T2+1;
//    int k2=T2-N_INITIAL_2+1;
        int n2_new;
        int k2_new;
        if (N_INITIAL_2 == -1) {
            n2_new = T2 + 1;
            k2_new = n2_new;
        } else {
            n2_new = T2 + 1;
            k2_new = T2 - N_INITIAL_2 + 1;
        }
        int input;
        int packet_counter = 0;
        float min_rate_debug_debug = 0;
        int min_rate_debug_packet_count = 0;
        for (int i = 0;; i++) {
            packet_counter++;
//        application_layer_sender.generate_message_and_encode(udp_parameters, buffer, &buffer_size); //udp_codeword is

            // sent;
            if (relaying_type == 0) {// P2P (no relay)
                application_layer_sender.generate_message_and_encode(udp_parameters, buffer,
                                                                     &buffer_size); //udp_codeword is

                // P2P
                application_layer_sender.generate_message_and_encode(udp_parameters, buffer,
                                                                     &buffer_size); //udp_codeword is

                seq_number = application_layer_relay_receiver->receive_message_and_decode(udp_parameters, nullptr,buffer,
                                                                                          &buffer_size,
                                                                                          &erasure_simulator);
            } else if (relaying_type == 1) {//Message-wise decode and forward
                // 1. Currently there is no signalling from the relay to the destination about erased packets at the relay.
                // This means that the destination may try to recover erased packets (resulting with false recovery)
                // 2. Currently each hop performs adaptation separately and independently. Currently there is no shift in delay between hops.
                // 3. Max burst size is MAX_BURST_SIZE_MWDF
                // 4. Need to handle restart of reading from file in calc_missed_chars
                application_layer_sender.generate_message_and_encode(udp_parameters, buffer,
                                                                     &buffer_size); //udp_codeword is

                //if (i >= T) {

                // message wise DF
//                for (int j = 0; j < 6; j++)
//                    application_layer_relay_receiver->response_from_dest_buffer[j] = udp_parameters2[j];

                seq_number = application_layer_relay_receiver->receive_message_and_decode(udp_parameters,udp_parameters2, buffer,
                                                                                          &buffer_size,
                                                                                          &erasure_simulator);
                if (i >= T) {
                    if (seq_number > -1) {
                        int numOfMessagesStored = application_layer_relay_receiver->fec_decoder->message_vector_to_transmit_stored_index;
                        for (int kk = 0; kk <= numOfMessagesStored; kk++) {
//                    printMatrix(application_layer_relay_receiver->fec_decoder->message_vector_to_transmit_stored[kk],1,300);
//                    cout << application_layer_relay_receiver->fec_decoder->message_vector_to_transmit_stored_seq[kk] << endl;
                            application_layer_relay_sender.message_wise_encode_at_relay(
                                    application_layer_relay_receiver->fec_decoder->message_vector_to_transmit_stored[kk],
                                    application_layer_relay_receiver->fec_decoder->message_vector_to_transmit_stored_seq[kk],
                                    udp_parameters2, buffer2,
                                    &buffer_size2);
                            application_layer_destination_receiver->receive_message_and_decode(
                                    udp_parameters2, nullptr,buffer2,
                                    &buffer_size2,
                                    &erasure_simulator2);
                        }
                    }
                }
//            if (i>=T) {
//                if (application_layer_relay_receiver->fec_decoder->recovered_message_vector[0]->buffer != NULL) {//recover past messages
//                    // check if there are codewords recovered in the past
//                    for (int j = 0; j < T_INITIAL; j++) {
//                        if (application_layer_relay_receiver->fec_decoder->recovered_message_vector[j]->buffer !=
//                            NULL) {
//                            printMatrix(application_layer_relay_receiver->fec_decoder->recovered_message_vector[j]->buffer,1,300);
//                            cout<<application_layer_relay_receiver->fec_decoder->recovered_message_vector[j]->seq_number<<endl;
//                            application_layer_relay_sender.message_wise_encode_at_relay(
//                                    application_layer_relay_receiver->fec_decoder->recovered_message_vector[j]->buffer,
//                                    application_layer_relay_receiver->fec_decoder->recovered_message_vector[j]->seq_number,
//                                    udp_parameters2, buffer2,
//                                    &buffer_size2, response_from_dest_buffer);
//                            // after transmitting zero (in case next packet is lost...)
//                            application_layer_relay_receiver->fec_decoder->recovered_message_vector[j]->buffer=NULL;
//                            application_layer_destination_receiver->receive_message_and_decode(
//                                    udp_parameters2, buffer2,
//                                    &buffer_size2,
//                                    &erasure_simulator2);
//                        } else {
//                            break;
//                        }
//                    }
//                    for (int j = 0; j < MAX_BURST_SIZE_MWDF; j++) {
//                        if (application_layer_relay_receiver->fec_decoder->burst_erased_message_vector[j]->buffer !=
//                            NULL) {
//                            printMatrix(application_layer_relay_receiver->fec_decoder->burst_erased_message_vector[j]->buffer,1,300);
//                            cout<<application_layer_relay_receiver->fec_decoder->burst_erased_message_vector[j]->seq_number<<endl;
//                            application_layer_relay_sender.message_wise_encode_at_relay(
//                                    application_layer_relay_receiver->fec_decoder->burst_erased_message_vector[j]->buffer,
//                                    application_layer_relay_receiver->fec_decoder->burst_erased_message_vector[j]->seq_number,
//                                    udp_parameters2, buffer2,
//                                    &buffer_size2, response_from_dest_buffer);
//                            // after transmitting zero (in case next packet is lost...)
//                            application_layer_relay_receiver->fec_decoder->burst_erased_message_vector[j]->buffer=NULL;
//                            application_layer_destination_receiver->receive_message_and_decode(
//                                    udp_parameters2, buffer2,
//                                    &buffer_size2,
//                                    &erasure_simulator2);
//                        } else {
//                            break;
//                        }
//                    }
//                }
//                if (seq_number > -1) {
//                    if (application_layer_relay_receiver->fec_message->buffer != NULL) {
//                        printMatrix(application_layer_relay_receiver->fec_message->buffer,1,300);
//                        cout<<seq_number - T<<endl;
//                        application_layer_relay_sender.message_wise_encode_at_relay(
//                                application_layer_relay_receiver->fec_message->buffer, seq_number - T, udp_parameters2,
//                                buffer2,
//                                &buffer_size2, response_from_dest_buffer);
////                        cout<<seq_number - T<<endl;
////                        printMatrix(application_layer_relay_receiver->fec_message->buffer,1,300);
////                        cout<<application_layer_relay_receiver->fec_decoder->message_vector_to_transmit_stored_seq[0]<<endl;
////                        printMatrix(application_layer_relay_receiver->fec_decoder->message_vector_to_transmit_stored[0],1,300);
//                        application_layer_destination_receiver->receive_message_and_decode(
//                                udp_parameters2, buffer2,
//                                &buffer_size2,
//                                &erasure_simulator2);
//                    } else if (application_layer_relay_receiver->fec_decoder->message_old_encoder->buffer != NULL) {// if double coding extract from old encoder
//                        printMatrix(application_layer_relay_receiver->fec_decoder->message_old_encoder->buffer,1,300);
//                        cout<<seq_number - T<<endl;
//                        application_layer_relay_sender.message_wise_encode_at_relay(
//                                application_layer_relay_receiver->fec_decoder->message_old_encoder->buffer,
//                                seq_number - T, udp_parameters2, buffer2,
//                                &buffer_size2, response_from_dest_buffer);
//                        application_layer_destination_receiver->receive_message_and_decode(
//                                udp_parameters2, buffer2,
//                                &buffer_size2,
//                                &erasure_simulator2);
//                    }
//                }
//            }
                min_rate_debug_packet_count += 1;
                min_rate_debug_debug += std::min(
                        application_layer_sender.variable_rate_FEC_encoder->debug_rate_first_hop_curr,
                        application_layer_relay_sender.debug_rate_second_hop_curr);


            } else if (relaying_type == 2) { // Symbol-wise decode and forward
                application_layer_sender.generate_message_and_encode(udp_parameters, buffer,
                                                                     &buffer_size); //udp_codeword is


//            if (k2>n2)
//                k2=n2;
                int codeword_size_final = 0;

//                for (int j = 0; j < 6; j++)
//                    application_layer_relay_receiver->response_from_dest_buffer[j] = udp_parameters2[j];

                seq_number = application_layer_relay_receiver->receive_message_and_symbol_wise_encode(udp_parameters,
                                                                                                      udp_parameters2,
                                                                                                      buffer,
                                                                                                      &buffer_size,
                                                                                                      &erasure_simulator,
                                                                                                      k2_new,
                                                                                                      n2_new,
                                                                                                      &codeword_size_final,
                                                                                                      &k2_new, &n2_new);
                if (seq_number > -1) {
                    int numOfStoredCodeWords = application_layer_relay_receiver->fec_decoder->codeword_vector_to_transmit_stored_index;
                    for (int kk = 0; kk <= numOfStoredCodeWords; kk++) {
                        int size_of_codeword = application_layer_relay_receiver->fec_decoder->codeword_vector_to_transmit_stored_word_size[kk];
                        int seq = application_layer_relay_receiver->fec_decoder->codeword_vector_to_transmit_stored_seq[kk];
                        int counter_for_start_and_end =
                                application_layer_relay_receiver->fec_decoder->codeword_vector_to_transmit_stored_counter_for_start_and_end[kk] -
                                numOfStoredCodeWords + kk;
                        if (counter_for_start_and_end < 0)
                            counter_for_start_and_end = 255 + counter_for_start_and_end;
//                    printMatrix(application_layer_relay_receiver->fec_decoder->codeword_vector_to_transmit_stored[kk],1,size_of_codeword);
//                    cout<<counter_for_start_and_end<<endl;
                        application_layer_relay_sender.send_sym_wise_message(
                                application_layer_relay_receiver->fec_decoder->codeword_vector_to_transmit_stored[kk],
                                size_of_codeword, udp_parameters2, buffer2, &buffer_size2, seq,
                                k2_new, n2_new,
                                counter_for_start_and_end);
                        application_layer_destination_receiver->receive_message_and_symbol_wise_decode(
                                udp_parameters2, buffer2,
                                &buffer_size2,
                                &erasure_simulator2, 0);
                    }
                }
                min_rate_debug_packet_count += 1;
                min_rate_debug_debug += std::min(
                        application_layer_sender.variable_rate_FEC_encoder->debug_rate_first_hop_curr,
                        application_layer_destination_receiver->fec_decoder->debug_rate_second_hop_curr);
            } else if (relaying_type == 3) { // Symbol-wise decode and forward
                application_layer_sender.generate_message_and_encode(udp_parameters, buffer,
                                                                     &buffer_size); //udp_codeword is


//            if (k2>n2)
//                k2=n2;
                int codeword_size_final = 0;

//                for (int j = 0; j < 6; j++)
//                    application_layer_relay_receiver->response_from_dest_buffer[j] = udp_parameters2[j];

                seq_number = application_layer_relay_receiver->receive_message_and_symbol_wise_encode(udp_parameters,udp_parameters2,
                                                                                                      buffer,
                                                                                                      &buffer_size,
                                                                                                      &erasure_simulator,
                                                                                                      k2_new,
                                                                                                      n2_new,
                                                                                                      &codeword_size_final,
                                                                                                      &k2_new, &n2_new);
                if (seq_number > -1) {
                    int numOfStoredCodeWords = application_layer_relay_receiver->fec_decoder->codeword_vector_to_transmit_stored_index;
                    for (int kk = 0; kk <= numOfStoredCodeWords; kk++) {
                        int size_of_codeword = application_layer_relay_receiver->fec_decoder->codeword_vector_to_transmit_stored_word_size[kk];
                        int seq = application_layer_relay_receiver->fec_decoder->codeword_vector_to_transmit_stored_seq[kk];
                        int counter_for_start_and_end =
                                application_layer_relay_receiver->fec_decoder->codeword_vector_to_transmit_stored_counter_for_start_and_end[kk] -
                                numOfStoredCodeWords + kk;
                        if (counter_for_start_and_end < 0)
                            counter_for_start_and_end = 255 + counter_for_start_and_end;
//                    printMatrix(application_layer_relay_receiver->fec_decoder->codeword_vector_to_transmit_stored[kk],1,size_of_codeword);
//                    cout<<counter_for_start_and_end<<endl;
                        application_layer_relay_sender.send_sym_wise_message(
                                application_layer_relay_receiver->fec_decoder->codeword_vector_to_transmit_stored[kk],
                                size_of_codeword, udp_parameters2, buffer2, &buffer_size2, seq, k2_new, n2_new,
                                counter_for_start_and_end);
                        application_layer_destination_receiver->receive_message_and_symbol_wise_decode(
                                udp_parameters2,buffer2,
                                &buffer_size2,
                                &erasure_simulator2, 0);
                    }
                }
                min_rate_debug_packet_count += 1;
                min_rate_debug_debug += std::min(
                        application_layer_sender.variable_rate_FEC_encoder->debug_rate_first_hop_curr,
                        application_layer_destination_receiver->fec_decoder->debug_rate_second_hop_curr);
            }


//            bool flag_for_using_backup=false;
//            int length_of_burst=0;
//            if (seq_number>-1) {
////                if (seq_number-last_seq_received_from_srouce>n2_new) {
//                length_of_burst=application_layer_relay_receiver->fec_decoder->flag_for_burst;
//                if (length_of_burst>0){
//                    //Need to send all codeword_vector_store_in_burst
//                    int double_coding_sum=0;
//                    if (seq_number-last_seq_received_from_srouce>1){// packets erased in (s,r)
//                        // check if double-coding ended
//                        for (int kk=0;kk<seq_number-last_seq_received_from_srouce;kk++){
//                            if (application_layer_relay_receiver->fec_decoder->trans_vec[kk]==true)
//                                double_coding_sum++;
//                        }
//                        if (application_layer_relay_receiver->fec_decoder->trans_vec[0]==true && double_coding_sum>0 && double_coding_sum<seq_number-last_seq_received_from_srouce) {
//                            flag_for_using_backup = true;
//                            cout << "ELAD" << endl;
//                        }
//                    }
//                    int count=-1;
//
//                    for (int seq = 0; seq < length_of_burst; seq++) {
//                        count++;
//                        if (flag_for_using_backup==true && count<double_coding_sum) {
////                            int burst_index_back;
////                            burst_index_back=application_layer_relay_receiver->fec_decoder->decoder_Symbol_Backup->n-double_coding_sum_to_use+count;
//                            int n2_old_old=application_layer_relay_receiver->fec_decoder->decoder_Symbol_Backup->n;
//                            int double_coding_sum_to_use=std::min(double_coding_sum,n2_old_old);
//                            int burst_index=seq+n2_old_old-double_coding_sum_to_use;
//
//                            int size_of_codeword=application_layer_relay_receiver->fec_decoder->decoder_Symbol_Backup->burst_codeword_size_vector[burst_index];
//                            if (size_of_codeword>10000 || size_of_codeword<100) {
//                                cout << "Problem with codeword size line 368"<<endl;
//                                size_of_codeword=0;
//                                cin >> input;
//                            }
//                            printMatrix(application_layer_relay_receiver->fec_decoder->decoder_Symbol_Backup->codeword_vector_store_in_burst[burst_index],1,size_of_codeword);
//                            cout << application_layer_relay_receiver->fec_message->counter_for_start_and_end << endl;
//                            application_layer_relay_sender.send_sym_wise_message(
//                                    application_layer_relay_receiver->fec_decoder->decoder_Symbol_Backup->codeword_vector_store_in_burst[burst_index],
//                                    size_of_codeword, udp_parameters2, buffer2, &buffer_size2, 0,
//                                    response_from_dest_buffer, k2_new, n2_new,
//                                    application_layer_relay_receiver->fec_message->counter_for_start_and_end);
////                            int size_of_codeword=application_layer_relay_receiver->fec_decoder->decoder_Symbol_Backup->codeword_size_vector[n2_old_old-double_coding_sum_to_use+count];
////                            application_layer_relay_sender.send_sym_wise_message(
////                                    application_layer_relay_receiver->fec_decoder->decoder_Symbol_Backup->codeword_vector_store_in_burst[n2_old_old-double_coding_sum_to_use+count],
////                                    size_of_codeword, udp_parameters2, buffer2, &buffer_size2, 0,
////                                    response_from_dest_buffer, k2_new, n2_new,
////                                    application_layer_relay_receiver->fec_message->counter_for_start_and_end);
//                        }else {
//                            int burst_index;
//                            if (flag_for_using_backup==true)
////                                burst_index=seq-double_coding_sum+application_layer_relay_receiver->fec_decoder->flag_for_burst_index-length_of_burst;
////                                    burst_index=seq-double_coding_sum+application_layer_relay_receiver->fec_decoder->flag_for_burst_index-1;
//                                 burst_index=seq-length_of_burst+application_layer_relay_receiver->fec_decoder->flag_for_burst_index;
//                            else
//                                burst_index=seq+application_layer_relay_receiver->fec_decoder->flag_for_burst_index-length_of_burst;
//
//                            int size_of_codeword=application_layer_relay_receiver->fec_decoder->decoder_Symbol_Wise->burst_codeword_size_vector[burst_index];
//                            if (size_of_codeword>10000 || size_of_codeword<100) {
//                                cout << "Problem with codeword size line 393"<<endl;
//                                cin >> input;
//                            }
//                            int delta_for_counter_for_start_and_end=seq_number-last_seq_received_from_srouce-seq-1;
//                            int counter_for_start_and_end=application_layer_relay_receiver->fec_message->counter_for_start_and_end-delta_for_counter_for_start_and_end;
//                            if (counter_for_start_and_end<0)
//                                counter_for_start_and_end=255+counter_for_start_and_end;
//                            printMatrix(application_layer_relay_receiver->fec_decoder->decoder_Symbol_Wise->codeword_vector_store_in_burst[burst_index],1,size_of_codeword);
//                            cout << counter_for_start_and_end << endl;
//                            application_layer_relay_sender.send_sym_wise_message(
//                                    application_layer_relay_receiver->fec_decoder->decoder_Symbol_Wise->codeword_vector_store_in_burst[burst_index],
//                                    size_of_codeword, udp_parameters2, buffer2, &buffer_size2, 0,
//                                    response_from_dest_buffer,
//                                    k2_new, n2_new,
//                                    counter_for_start_and_end);
//                        }
//                        application_layer_destination_receiver->receive_message_and_symbol_wise_decode(
//                                udp_parameters2, buffer2,
//                                &buffer_size2,
//                                &erasure_simulator2,0);
//                    }
//                    // send the message that was received after the burst
//                    int number_of_missing_packets_after_burst=seq_number-last_seq_received_from_srouce-length_of_burst-1; // correcting seq_number in the sent packet
//                    int index_to_use;
//                    if (application_layer_relay_receiver->fec_decoder->trans_vec[seq_number-last_seq_received_from_srouce-1]==true)
//                        index_to_use=application_layer_relay_receiver->fec_decoder->n2_old;
//                    else
//                        index_to_use=n2_new;
//                    printMatrix(application_layer_relay_receiver->fec_decoder->decoder_Symbol_Wise->codeword_vector_to_transmit[index_to_use-1],1,codeword_size_final);
//                    cout<<application_layer_relay_receiver->fec_message->counter_for_start_and_end<<endl;
//                    application_layer_relay_sender.send_sym_wise_message(
//                            application_layer_relay_receiver->fec_decoder->decoder_Symbol_Wise->codeword_vector_to_transmit[index_to_use-1],
//                            codeword_size_final, udp_parameters2, buffer2, &buffer_size2,
//                            number_of_missing_packets_after_burst,
//                            response_from_dest_buffer,k2_new,n2_new,application_layer_relay_receiver->fec_message->counter_for_start_and_end);
//                    application_layer_destination_receiver->receive_message_and_symbol_wise_decode(
//                            udp_parameters2, buffer2,
//                            &buffer_size2,
//                            &erasure_simulator2,seq_number-last_seq_received_from_srouce-length_of_burst-1);
//                } // not in burst
//                else {
//                    int double_coding_sum=0;
//                    if (seq_number-last_seq_received_from_srouce>1){// packets erased in (s,r)
//                        // check if double-coding ended
//                        for (int k=0;k<seq_number-last_seq_received_from_srouce;k++){
//                            if (application_layer_relay_receiver->fec_decoder->trans_vec[k]==true)
//                                double_coding_sum++;
//                        }
//                        if (application_layer_relay_receiver->fec_decoder->trans_vec[0]==true && double_coding_sum>0 && double_coding_sum<seq_number-last_seq_received_from_srouce) {
//                            flag_for_using_backup = true;
//                            cout << "ELAD" << endl;
//                        }
//                    }
//                    int count=-1;
////                    int count=seq_number-last_seq_received_from_srouce-1;
//                    for (int seq = last_seq_received_from_srouce; seq < seq_number; seq++) {
//                        //                    int n=T_INITIAL+1;
////                        count--;
//                        count++;
//                        int index = application_layer_relay_receiver->fec_decoder->n2_old - (seq_number - seq);
//                        if (flag_for_using_backup==true && count<double_coding_sum) {
//                            int n2_old_old=application_layer_relay_receiver->fec_decoder->decoder_Symbol_Backup->n2;
////                            int index2=double_coding_sum-count;
//                            int size_of_codeword=application_layer_relay_receiver->fec_decoder->decoder_Symbol_Backup->codeword_size_vector[n2_old_old-double_coding_sum+count];
//                            if (size_of_codeword>10000 || size_of_codeword<100) {
//                                cout << "Problem with codeword size line 454"<<endl;
//                                cin >> input;
//                            }
//                            printMatrix(application_layer_relay_receiver->fec_decoder->decoder_Symbol_Backup->codeword_vector_to_transmit[n2_old_old-double_coding_sum+count],1,size_of_codeword);
//                            cout<<application_layer_relay_receiver->fec_message->counter_for_start_and_end<<endl;
//                            application_layer_relay_sender.send_sym_wise_message(
//                                    application_layer_relay_receiver->fec_decoder->decoder_Symbol_Backup->codeword_vector_to_transmit[n2_old_old-double_coding_sum+count],
//                                    size_of_codeword, udp_parameters2, buffer2, &buffer_size2, 0,
//                                    response_from_dest_buffer, k2_new, n2_new,
//                                    application_layer_relay_receiver->fec_message->counter_for_start_and_end);
//                        }else {
//                            int counter_for_start_and_end=application_layer_relay_receiver->fec_message->counter_for_start_and_end-(seq_number-seq-1);
//                            if (counter_for_start_and_end<0)
//                                counter_for_start_and_end=255+counter_for_start_and_end;
//                            int size_of_codeword=application_layer_relay_receiver->fec_decoder->decoder_Symbol_Wise->codeword_size_vector[index];
//                            if (size_of_codeword>10000 || size_of_codeword<100) {
//                                cout << "Problem with codeword size line 468"<<endl;
//                                cin >> input;
//                            }
//                            printMatrix(application_layer_relay_receiver->fec_decoder->decoder_Symbol_Wise->codeword_vector_to_transmit[index],1,size_of_codeword);
//                            cout<<counter_for_start_and_end<<endl;
//                            application_layer_relay_sender.send_sym_wise_message(
//                                    application_layer_relay_receiver->fec_decoder->decoder_Symbol_Wise->codeword_vector_to_transmit[index],
//                                    size_of_codeword, udp_parameters2, buffer2, &buffer_size2, 0,
//                                    response_from_dest_buffer, k2_new, n2_new,
//                                    counter_for_start_and_end);
//                        }
//                        application_layer_destination_receiver->receive_message_and_symbol_wise_decode(
//                                udp_parameters2, buffer2,
//                                &buffer_size2,
//                                &erasure_simulator2,0);
//                    }
//                }
//
//
//                last_seq_received_from_srouce=seq_number;
//            }
//            min_rate_debug_packet_count+=1;
//            min_rate_debug_debug+=std::min(application_layer_sender.variable_rate_FEC_encoder->debug_rate_first_hop_curr,
//                    application_layer_destination_receiver->fec_decoder->debug_rate_second_hop_curr);
//        }

            if (i == 0)
                start_time = high_resolution_clock::now();

            if (seq_number >= stream_duration + T + T2 - 1) //Elad added T2
                break;
        }

        auto stop = high_resolution_clock::now();

        auto duration = duration_cast<minutes>(stop - start_time);

        cout << "Time duration = " << duration.count() << " minutes" << endl;
        cout << "Last sequence number received = " << seq_number << endl;

        if (RELAYING_TYPE > 0) {
            time_t now = time(0);
            tm *ltm = localtime(&now);
            std::string timeStamp = std::to_string(1900 + ltm->tm_year) + "_" + std::to_string(1 + ltm->tm_mon) + "_" +
                                    std::to_string(ltm->tm_mday) + "_" + std::to_string(ltm->tm_hour) + "_" +
                                    std::to_string(1 + ltm->tm_min);
            ofstream myfile;
            myfile.open("results_summary.csv", std::ios_base::app);
            myfile << timeStamp << ",";
            // show type of run
            if (RELAYING_TYPE == 3) {
                if (N_INITIAL == -1 && N_INITIAL_2 == -1) {
                    DEBUG_MSG("\033[1;34m" << "adaptive SD-SWDF, T_TOT=" << T_TOT << " estimation window=" <<
                                           ESTIMATION_WINDOW_SIZE / ESTIMATION_WINDOW_SIZE_REDUCTION_FACTOR
                                           << "\033[0m");
                    myfile << "adaptive SD-SWDF, T_TOT=" << T_TOT << " estimation window=" <<
                           ESTIMATION_WINDOW_SIZE / ESTIMATION_WINDOW_SIZE_REDUCTION_FACTOR
                           << ",";
                } else {
                    DEBUG_MSG(
                            "\033[1;34m" << "Fixed-rate SD-SWDF, hop1 (T1=" << T_TOT - N_INITIAL_2 << ", N1="
                                         << N_INITIAL
                                         << "), hop2 (T2=" << T_TOT - N_INITIAL << ", N2=" << N_INITIAL_2 << ")"
                                         << "\033[0m");
                    myfile << "Fixed-rate SD-SWDF, hop1 (T1=" << T_TOT - N_INITIAL_2 << "_N1=" << N_INITIAL
                           << ") hop2 (T2=" << T_TOT - N_INITIAL << "_N2=" << N_INITIAL_2 << ")" << ",";

                }
            } else if (RELAYING_TYPE == 2) {
                if (N_INITIAL == -1 && N_INITIAL_2 == -1) {
                    DEBUG_MSG("\033[1;34m" << "adaptive SWDF, T_TOT=" << T_TOT << " estimation window=" <<
                                           ESTIMATION_WINDOW_SIZE / ESTIMATION_WINDOW_SIZE_REDUCTION_FACTOR
                                           << "\033[0m");
                    myfile << "adaptive SWDF, T_TOT=" << T_TOT << " estimation window=" <<
                           ESTIMATION_WINDOW_SIZE / ESTIMATION_WINDOW_SIZE_REDUCTION_FACTOR
                           << ",";
                } else {
                    DEBUG_MSG(
                            "\033[1;34m" << "Fixed-rate SWDF, hop1 (T1=" << T_TOT - N_INITIAL_2 << ", N1=" << N_INITIAL
                                         << "), hop2 (T2=" << T_TOT - N_INITIAL << ", N2=" << N_INITIAL_2 << ")"
                                         << "\033[0m");
                    myfile << "Fixed-rate SWDF, hop1 (T1=" << T_TOT - N_INITIAL_2 << "_N1=" << N_INITIAL
                           << ") hop2 (T2=" << T_TOT - N_INITIAL << "_N2=" << N_INITIAL_2 << ")" << ",";

                }
            } else {
                if (N_INITIAL == -1 && N_INITIAL_2 == -1) {
                    DEBUG_MSG("\033[1;34m" << "adaptive MWDF, T1=" << T_INITIAL << ", T2=" << T_INITIAL_2 << "\033[0m");
                    myfile << "adaptive MWDF, T1=" << T_INITIAL << " T2=" << T_INITIAL_2 << ",";
                } else {
                    DEBUG_MSG(
                            "\033[1;34m" << "Fixed-rate MWDF, hop1 (T1=" << T_INITIAL << ", N1=" << N_INITIAL
                                         << "), hop2 (T2=" << T_INITIAL_2 << ", N2=" << N_INITIAL_2 << ")"
                                         << "\033[0m");
                    myfile << "Fixed-rate MWDF, hop1 (T1=" << T_INITIAL << "_N1=" << N_INITIAL
                           << ") hop2 (T2=" << T_INITIAL_2 << "_N2=" << N_INITIAL_2 << ")"
                           << ",";
                }
            }
            // show total char loss
            int num_of_packets_lost =
                    application_layer_destination_receiver->fec_decoder->final_counter_loss_of_full_packet +
                    application_layer_destination_receiver->fec_decoder->final_counter_loss_of_packets_swdf;
            if (DEBUG_CHAR == 1) {
                float final_char_loss_in_per;
                if (RELAYING_TYPE == 2 || RELAYING_TYPE == 3)
                    final_char_loss_in_per =
                            (float) (application_layer_destination_receiver->fec_decoder->final_counter_loss_of_char +
                                     application_layer_destination_receiver->fec_decoder->final_counter_loss_of_full_packet *
                                     300) / (300 * (packet_counter - T_TOT)) * 100;
                else
                    final_char_loss_in_per =
                            (float) (num_of_packets_lost *
                                     300) / (300 * (packet_counter - T_INITIAL - T_INITIAL_2)) * 100;

//        if (RELAYING_TYPE==2)
//            num_of_packets_lost=application_layer_destination_receiver->fec_decoder->final_counter_loss_of_full_packet+
//                    application_layer_destination_receiver->fec_decoder->final_counter_loss_of_packets_swdf;
//        else
//            num_of_packets_lost=application_layer_destination_receiver->fec_decoder->final_counter_loss_of_full_packet;

                DEBUG_MSG("\033[1;34m" << "Total Char loss " << final_char_loss_in_per << "% " << "\033[0m");
                myfile << "Total Char loss " << final_char_loss_in_per << "% " << ",";
            }
//        num_of_packets_lost=application_layer_destination_receiver->fec_decoder->final_counter_loss_of_full_packet+
//                            application_layer_destination_receiver->fec_decoder->final_counter_loss_of_packets_swdf;
            DEBUG_MSG("\033[1;34m" << "Total number of erased packets " << num_of_packets_lost << "\033[0m");
            myfile << "Total number of erased packets " << num_of_packets_lost << ",";
            DEBUG_MSG("\033[1;34m" << "Total occurrences of bug words  "
                                   << application_layer_destination_receiver->fec_decoder->final_counter_loss_of_char_elad
                                   << "\033[0m");
            if (RELAYING_TYPE > 0) {
                float avg_rate_first_hop =
                        (float) (application_layer_sender.variable_rate_FEC_encoder->debug_rate_first_hop) /
                        (application_layer_sender.variable_rate_FEC_encoder->debug_rate_first_hop_num_packets);
                float avg_rate_second_hop;
                if (RELAYING_TYPE == 1)
                    avg_rate_second_hop = (float) (application_layer_relay_sender.debug_rate_second_hop) /
                                          (application_layer_relay_sender.debug_rate_second_hop_num_packets);
                else
                    avg_rate_second_hop =
                            (float) (application_layer_destination_receiver->fec_decoder->debug_rate_second_hop) /
                            (application_layer_destination_receiver->fec_decoder->debug_rate_second_hop_num_packets);
                DEBUG_MSG("\033[1;34m" << "Average rate in first hop " << avg_rate_first_hop << "\033[0m");
                myfile << "Average rate in first hop " << avg_rate_first_hop << ",";
                DEBUG_MSG("\033[1;34m" << "Average rate in second hop " << avg_rate_second_hop << "\033[0m");
                myfile << "Average rate in second hop " << avg_rate_second_hop << ",";
                DEBUG_MSG("\033[1;34m" << "Average min rate " << min_rate_debug_debug / min_rate_debug_packet_count
                                       << "\033[0m");
                myfile << "Average min rate " << min_rate_debug_debug / min_rate_debug_packet_count << ",";

            }
            myfile << "\n";
            myfile.close();


        }


//    cout << "Loss probability = " << calculateLossMessage(INPUTDATAFILE, OUTPUTDATAFILE);

        free(udp_parameters);
        delete application_layer_relay_receiver;
        delete application_layer_destination_receiver;
//    }
    return 0;
}
