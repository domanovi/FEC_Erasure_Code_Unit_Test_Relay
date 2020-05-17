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


int main(int argc, const char *argv[]) {

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

    int T,T2,k;
    unsigned char **received_data_vector;

    if (relaying_type==0){
        T=T_INITIAL;
        T2=0;
    }
    else if (relaying_type==1){
        // message wise
        T = T_INITIAL;
        T2 = T_INITIAL_2;
    } else if (relaying_type==2) {
        T = T_INITIAL;
        T2 = T_INITIAL_2;
    }

    int stream_duration = NUMBER_OF_ITERATIONS;
    int max_payload = packet_size;

    Application_Layer_Sender application_layer_sender(Tx, Rx, packet_size, 0, T,B, N, 0);
    Application_Layer_Sender application_layer_relay_sender(Tx2, Rx2, packet_size, 0, T2, B2, N2 , 1 );

    Application_Layer_Receiver *application_layer_relay_receiver = new Application_Layer_Receiver(Tx, Rx, max_payload,
                                                                                                  erasure_type,
                                                                                                  adaptive_mode_MDS, 0);
    Application_Layer_Receiver *application_layer_destination_receiver = new Application_Layer_Receiver(Tx2, Rx2, max_payload,
                                                                                                        erasure_type,
                                                                                                        adaptive_mode_MDS, 1);
    application_layer_destination_receiver->set_receiver_index(1);

    siphon::Erasure_File_Generator erasure_generator;
    siphon::Erasure_File_Generator erasure_generator2;

    switch (erasure_type) {
        case 1:
            erasure_generator.generate_IID(stream_duration + T+T2, EPSILON, "erasure.bin",0);
            erasure_generator2.generate_IID(stream_duration + T+T2, EPSILON, "erasure2.bin",0);
            break;
        case 2:
            erasure_generator.generate_GE(stream_duration + T, ALPHA, BETA, EPSILON, "erasure.bin");
            break;
        case 3:
            erasure_generator.generate_GE_varying(stream_duration + T+T2, ALPHA, BETA, EPSILON, "erasure.bin");
            break;
        case 4:
            erasure_generator.generate_periodic(stream_duration + T, ERASURE_T, ERASURE_B, ERASURE_N, "erasure.bin");
            break;
        case 5:
            break;                                            // In case 5, erasure.bin is assumed to be already present
        case 6:
            erasure_generator.generate_three_sections_IID(3000,EPSILON,3000,EPSILON_2,stream_duration-3000-3000 + T+T2,EPSILON_3,"erasure.bin");
            break;
        default:
            erasure_generator.generate_periodic(stream_duration + T, T, 0, 0, "erasure.bin");
    }
    erasure_generator.generate_three_sections_IID(3000,0.33,4000,0,stream_duration-3000-4000 + T+T2,0.33,"erasure.bin");
    erasure_generator2.generate_IID(stream_duration + T+T2, 0.1, "erasure2.bin",2);


    siphon::Erasure_Simulator erasure_simulator("erasure.bin");
    siphon::Erasure_Simulator erasure_simulator2("erasure2.bin");

    auto start_time = high_resolution_clock::now();;


//    erasure_simulator.erasure_seq[4]='\001';
//    erasure_simulator.erasure_seq[5]='\001';

//    for (int i=0;i<1000;i++) {
//        erasure_simulator.erasure_seq[i] = '\000';
//    }
//
//    for (int i=0;i<1000;i++) {
//        erasure_simulator2.erasure_seq[i] = '\000';
//    }
//    erasure_simulator.erasure_seq[4]='\001';
//    erasure_simulator.erasure_seq[6]='\001';
//    erasure_simulator.erasure_seq[7]='\001';

//    erasure_simulator.erasure_seq[2]='\001';
//    erasure_simulator.erasure_seq[3]='\001';
//    erasure_simulator.erasure_seq[4]='\001';
    erasure_simulator2.erasure_seq[5]='\001';
    erasure_simulator2.erasure_seq[6]='\001';
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
    unsigned char *zero_data=(unsigned char *)malloc(sizeof(unsigned char *) * 300); // Elad to change
    memset(zero_data,'\000',300);
    unsigned char response_from_dest_buffer[6];
    for (int i=0;i<6;i++)
        response_from_dest_buffer[i]=0;
    int last_seq_received_from_srouce=-1;

    for (int i = 0;; i++) {
        application_layer_sender.generate_message_and_encode(udp_parameters, buffer, &buffer_size); //udp_codeword is

        // sent;
        if (relaying_type == 0) {
            // P2P
            application_layer_sender.generate_message_and_encode(udp_parameters, buffer, &buffer_size); //udp_codeword is

            seq_number = application_layer_relay_receiver->receive_message_and_decode(udp_parameters, buffer, &buffer_size,
                                                                                      &erasure_simulator);
        }
        else if (relaying_type == 1) {
            //if (i >= T) {

            // message wise DF
            for (int j=0;j<6;j++)
                application_layer_relay_receiver->response_from_dest_buffer[j]=response_from_dest_buffer[j];

            seq_number = application_layer_relay_receiver->receive_message_and_decode(udp_parameters, buffer,
                                                                                      &buffer_size,&erasure_simulator);
            if (i>=T) {
                if (application_layer_relay_receiver->fec_decoder->recovered_message_vector[0]->buffer != NULL) {
                    // check if there are codewords received in the past
                    for (int j = 0; j < T_INITIAL; j++) {
                        if (application_layer_relay_receiver->fec_decoder->recovered_message_vector[j]->buffer !=
                            NULL) {
                            application_layer_relay_sender.message_wise_encode_at_relay(
                                    application_layer_relay_receiver->fec_decoder->recovered_message_vector[j]->buffer,
                                    application_layer_relay_receiver->fec_decoder->recovered_message_vector[j]->seq_number,
                                    udp_parameters2, buffer2,
                                    &buffer_size2, response_from_dest_buffer);
                            // after transmitting zero (in case next packet is lost...)
                            application_layer_relay_receiver->fec_decoder->recovered_message_vector[j]->buffer=NULL;
                            application_layer_destination_receiver->receive_message_and_decode(
                                    udp_parameters2, buffer2,
                                    &buffer_size2,
                                    &erasure_simulator2);
                        } else {
                            break;
                        }
                    }
                }
                if (seq_number > -1) {
                    if (application_layer_relay_receiver->fec_message->buffer != NULL) {
                        application_layer_relay_sender.message_wise_encode_at_relay(
                                application_layer_relay_receiver->fec_message->buffer, seq_number - T, udp_parameters2,
                                buffer2,
                                &buffer_size2, response_from_dest_buffer);
                        application_layer_destination_receiver->receive_message_and_decode(
                                udp_parameters2, buffer2,
                                &buffer_size2,
                                &erasure_simulator2);
                    } else if (application_layer_relay_receiver->fec_decoder->message_old_encoder->buffer != NULL) {
                        application_layer_relay_sender.message_wise_encode_at_relay(
                                application_layer_relay_receiver->fec_decoder->message_old_encoder->buffer,
                                seq_number - T, udp_parameters2, buffer2,
                                &buffer_size2, response_from_dest_buffer);
                        application_layer_destination_receiver->receive_message_and_decode(
                                udp_parameters2, buffer2,
                                &buffer_size2,
                                &erasure_simulator2);
                    }
                }
            }

//                application_layer_relay_receiver->get_current_packet(received_data, T, i, seq_number,
//                                                               application_layer_sender.message_transmitted->seq_number -
//                                                               T);
//                std::memcpy(data_to_transmit_in_relay, received_data, sizeof(unsigned char) * 300);
//                application_layer_relay_sender.message_wise_encode_at_relay(data_to_transmit_in_relay, udp_parameters2, buffer2,
//                                                                       &buffer_size2);
//                seq_number2 = application_layer_destination_receiver->receive_message_and_decode(udp_parameters2, buffer2,
//                                                                                                 &buffer_size2,
//                                                                                                 &erasure_simulator2);
                //}
            } else if (relaying_type == 2) {
                int n2=T_INITIAL_2+1;
                int k2=T_INITIAL_2-N_INITIAL_2+1;
                int new_temp_size=n2*100+8;

                seq_number = application_layer_relay_receiver->receive_message_and_symbol_wise_encode(udp_parameters,buffer,
                                                                                                      &buffer_size,
                                                                                                      &erasure_simulator,k2,n2);

                if (seq_number>-1) {
                    if (seq_number-last_seq_received_from_srouce>n2) {
                        //Need to send all codeword_vector_store_in_burst
                        for (int seq = 0; seq < n2; seq++) {
                            int index = seq;
                            application_layer_relay_sender.send_sym_wise_message(
                                    application_layer_relay_receiver->fec_decoder->codeword_vector_store_in_burst[index],
                                    new_temp_size, udp_parameters2, buffer2, &buffer_size2);
                            application_layer_destination_receiver->receive_message_and_symbol_wise_decode(
                                    udp_parameters2, buffer2,
                                    &buffer_size2,
                                    &erasure_simulator2);
                        }
                        // send the message that was received after the burst
                        application_layer_relay_sender.send_sym_wise_message(
                                application_layer_relay_receiver->fec_decoder->codeword_new_vector[n2-1],
                                new_temp_size, udp_parameters2, buffer2, &buffer_size2);
                        application_layer_destination_receiver->receive_message_and_symbol_wise_decode(
                                udp_parameters2, buffer2,
                                &buffer_size2,
                                &erasure_simulator2);
                    }
                    else {
                        for (int seq = last_seq_received_from_srouce; seq < seq_number; seq++) {
                            //                    int n=T_INITIAL+1;
                            int index = n2 - (seq_number - seq);
                            application_layer_relay_sender.send_sym_wise_message(
                                    application_layer_relay_receiver->fec_decoder->codeword_new_vector[index],
                                    new_temp_size, udp_parameters2, buffer2, &buffer_size2);
                            application_layer_destination_receiver->receive_message_and_symbol_wise_decode(
                                    udp_parameters2, buffer2,
                                    &buffer_size2,
                                    &erasure_simulator2);
                        }
                    }


                    last_seq_received_from_srouce=seq_number;
                }


            }

            if (i == 0)
                start_time = high_resolution_clock::now();

            if (seq_number >= stream_duration + T +T2+1000- 1) //Elad added T2
                break;
        }

        auto stop = high_resolution_clock::now();

        auto duration = duration_cast<minutes>(stop - start_time);

        cout << "Time duration = " << duration.count() << " minutes" << endl;
        cout << "Last sequence number received = " << seq_number << endl;

//    cout << "Loss probability = " << calculateLossMessage(INPUTDATAFILE, OUTPUTDATAFILE);

        free(udp_parameters);
        delete application_layer_relay_receiver;
        delete application_layer_destination_receiver;

        return 0;
    }
