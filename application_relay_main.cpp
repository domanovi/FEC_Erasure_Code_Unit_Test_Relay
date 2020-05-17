//
// Created by elad on 2020-05-08.
//

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <iomanip>      // std::setprecision()
#include <random>       //for std::ceil()
#include<unistd.h>

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


int main(int argc, const char *argv[]) {

    int seq_start = 0;

    bool adaptive_mode_MDS = false;   //if true, then the parameter estimator will only output MDS parameters with B=N

    if ((argc != 5) && (argc != 6)) {
//        argv[1]="127.0.0.1";
//        argv[2]="127.0.0.1";
//        argv[3]="127.0.0.1";
        argv[1]="192.168.0.22";
        argv[2]="192.168.0.21";
        argv[3]="192.168.0.22";
        argv[4]="300";
        argv[5]="1000";
        argv[6]="0";
//        cout << "Please enter the arguments in this format: (source IP) (relay IP) (destination IP) (packet size in bytes) (packet interarrival time in ms (smallest is 0.01 ms))  "
//                "(erasure type as described in include/FEC_Macro.h) (MDS_restricted_estimates boolean flag [optional])."
//             << endl;
//        return 1;
    }

    //Source IP address:
    const char *Tx = argv[1];
    cout << "Source IP: " << argv[1] << endl;
    const char *Relay = argv[2];
    cout << "Relay IP: " << argv[2] << endl;
    //Destination IP address:
    const char *Rx = argv[3];
    cout << "Destination IP: " << argv[3] << endl;

    int packet_size = std::stoi(argv[4]);
    int packet_interarrival_time = (int) (std::stof(argv[5])*1000);

    int stream_duration = NUMBER_OF_ITERATIONS;
    int max_payload = packet_size;

    int erasure_type = std::stoi(argv[6]);

    if((argc==8)&&(std::stoi(argv[7])==1))
        adaptive_mode_MDS = true;


    int T;
    int T2;
    if (RELAYING_TYPE==0){
        T=T_INITIAL;
        T2=0;
    }
    else if (RELAYING_TYPE==1){
        // message wise
        T = T_INITIAL;
        T2 = T_INITIAL_2;
    } else if (RELAYING_TYPE==2) {
        T = T_INITIAL;
        T2 = T_INITIAL_2;
    }
    int B2=N_INITIAL_2;
    int N2=N_INITIAL_2;

    Application_Layer_Sender application_layer_relay_sender(Relay, Rx, packet_size, 0, T2, B2, N2 , 1 );

    Application_Layer_Receiver *application_layer_relay_receiver = new Application_Layer_Receiver(Tx, Relay, max_payload,
                                                                                                  erasure_type,
                                                                                                  adaptive_mode_MDS, 0);

    siphon::Erasure_File_Generator erasure_generator;

    switch (erasure_type) {
        case 1:
            erasure_generator.generate_IID(stream_duration + T, EPSILON, "erasure.bin",0);
            break;
        case 2:
            erasure_generator.generate_GE(stream_duration + T, ALPHA, BETA, EPSILON, "erasure.bin");
            break;
        case 3:
            erasure_generator.generate_GE_varying(stream_duration + T, ALPHA, BETA, EPSILON, "erasure.bin");
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

    siphon::Erasure_Simulator erasure_simulator("erasure.bin");

    boost::posix_time::ptime start_time;

    cout << "Iteration = " << stream_duration << endl;

    int seq_number;
    int seq_number2;
    unsigned char received_data[30000];
    unsigned char data_to_transmit_in_relay[30000];
    unsigned char response_from_dest_buffer[6];
    for (int i=0;i<6;i++)
        response_from_dest_buffer[i]='\000';
    int last_seq_received_from_srouce=-1;


    for (int i = 0;; i++) {

//        seq_number = application_layer_receiver->receive_message_and_decode(nullptr, nullptr, nullptr,
//                                                                            &erasure_simulator);
        if (RELAYING_TYPE == 1) {
            for (int j=0;j<6;j++)
                application_layer_relay_receiver->response_from_dest_buffer[j]=response_from_dest_buffer[j];
            seq_number = application_layer_relay_receiver->receive_message_and_decode(nullptr, nullptr,
                                                                                      nullptr,&erasure_simulator);
            if (i>=T) {
                if (application_layer_relay_receiver->fec_decoder->recovered_message_vector[0]->buffer != NULL) {
                    // check there are codewords received in the past
                    for (int j = 0; j < T_INITIAL; j++) {
                        if (application_layer_relay_receiver->fec_decoder->recovered_message_vector[j]->buffer !=
                            NULL) {
                            application_layer_relay_sender.message_wise_encode_at_relay(
                                    application_layer_relay_receiver->fec_decoder->recovered_message_vector[j]->buffer,
                                    application_layer_relay_receiver->fec_decoder->recovered_message_vector[j]->seq_number,
                                    nullptr, nullptr, nullptr, response_from_dest_buffer);
                            // after transmitting zero (in case next packet is lost...)
                            application_layer_relay_receiver->fec_decoder->recovered_message_vector[j]->buffer = NULL;
                            cout << "Response at relay" << endl;
                            printMatrix(response_from_dest_buffer, 1, 6);
//                            application_layer_destination_receiver->receive_message_and_decode(
//                                    nullptr, nullptr,
//                                    nullptr,
//                                    &erasure_simulator);
                        } else {
                            break;
                        }
                    }
                }
                if (seq_number > -1) {
                    if (application_layer_relay_receiver->fec_message->buffer != NULL) {
                        application_layer_relay_sender.message_wise_encode_at_relay(
                                application_layer_relay_receiver->fec_message->buffer, seq_number - T, nullptr, nullptr,
                                nullptr, response_from_dest_buffer);
                        cout << "Response at relay" << endl;
                        printMatrix(response_from_dest_buffer, 1, 6);
//                    application_layer_destination_receiver->receive_message_and_decode(
//                            udp_parameters2, buffer2,
//                            &buffer_size2,
//                            &erasure_simulator2);
                    } else if (application_layer_relay_receiver->fec_decoder->message_old_encoder->buffer != NULL) {
                        application_layer_relay_sender.message_wise_encode_at_relay(
                                application_layer_relay_receiver->fec_decoder->message_old_encoder->buffer,
                                seq_number - T, nullptr, nullptr,
                                nullptr, response_from_dest_buffer);
                        cout << "Response at relay" << endl;
                        printMatrix(response_from_dest_buffer, 1, 6);
//                        application_layer_destination_receiver->receive_message_and_decode(
//                                udp_parameters2, buffer2,
//                                &buffer_size2,
//                                &erasure_simulator2);
                    }
                }
            }
//            seq_number = application_layer_relay_receiver->receive_message_and_decode(nullptr, nullptr,
//                                                                                      nullptr,&erasure_simulator);
//            if (seq_number>-1 && application_layer_relay_receiver->fec_message->buffer!=NULL){
//                if (i>=T) {
//
//                    application_layer_relay_sender.message_wise_encode_at_relay(
//                            application_layer_relay_receiver->fec_message->buffer, seq_number-T,nullptr, nullptr,
//                            nullptr);
//                }
//            }
//            seq_number = application_layer_relay_receiver->receive_message_and_decode(nullptr, nullptr,
//                                                                                      nullptr,
//                                                                                      &erasure_simulator);
//            application_layer_relay_receiver->get_current_packet(received_data, T, i, seq_number,
//                                                                 seq_number -T);
//            std::memcpy(data_to_transmit_in_relay, received_data, sizeof(unsigned char) * 300);
//            application_layer_relay_sender.encode_message_at_relay(data_to_transmit_in_relay, nullptr, nullptr,nullptr);
        }else if (RELAYING_TYPE == 2){
            int n2=T_INITIAL_2+1;
            int k2=T_INITIAL_2-N_INITIAL_2+1;
            seq_number = application_layer_relay_receiver->receive_message_and_symbol_wise_encode(nullptr,nullptr,
                                                                                                  nullptr,
                                                                                                  &erasure_simulator,k2,n2);

            if (seq_number>-1) {
                if (seq_number-last_seq_received_from_srouce>n2) {
                    //Need to send all codeword_vector_store_in_burst
                    for (int seq = 0; seq < n2; seq++) {
                        int index = seq;
                        application_layer_relay_sender.send_sym_wise_message(
                                application_layer_relay_receiver->codeword_vector_store_in_burst[index],
                                application_layer_relay_receiver->temp_size, nullptr, nullptr, nullptr);
//                        application_layer_destination_receiver->receive_message_and_symbol_wise_decode(
//                                udp_parameters2, buffer2,
//                                &buffer_size2,
//                                &erasure_simulator2,
//                                application_layer_sender.variable_rate_FEC_encoder->encoder_current->encoder->getG());
                    }
                    // send the message that was receieved after the burst
                    application_layer_relay_sender.send_sym_wise_message(
                            application_layer_relay_receiver->codeword_new_vector[n2-1],
                            application_layer_relay_receiver->temp_size, nullptr, nullptr, nullptr);
//                    application_layer_destination_receiver->receive_message_and_symbol_wise_decode(
//                            udp_parameters2, buffer2,
//                            &buffer_size2,
//                            &erasure_simulator2,
//                            application_layer_sender.variable_rate_FEC_encoder->encoder_current->encoder->getG());
                }
                else {
                    for (int seq = last_seq_received_from_srouce; seq < seq_number; seq++) {
                        //                    int n=T_INITIAL+1;
                        int index = n2 - (seq_number - seq);
                        application_layer_relay_sender.send_sym_wise_message(
                                application_layer_relay_receiver->codeword_new_vector[index],
                                application_layer_relay_receiver->temp_size, nullptr, nullptr, nullptr);
//                        application_layer_destination_receiver->receive_message_and_symbol_wise_decode(
//                                udp_parameters2, buffer2,
//                                &buffer_size2,
//                                &erasure_simulator2,
//                                application_layer_sender.variable_rate_FEC_encoder->encoder_current->encoder->getG());
                    }
                }


                last_seq_received_from_srouce=seq_number;
            }
//            int seq_number2 = application_layer_relay_receiver->receive_message_and_symbol_wise_encode(nullptr,
//                                                                                                       nullptr,
//                                                                                                       nullptr,
//                                                                                                       &erasure_simulator,
//                                                                                                       application_layer_sender.variable_rate_FEC_encoder->encoder_current->encoder->getG());
//            application_layer_relay_sender.send_sym_wise_message(application_layer_relay_receiver->codeword_new_symbol_wise,
//                                                                 buffer_size, nullptr, nullptr, &nullptr);
        }

        if (i == 0)
            boost::posix_time::ptime start_time = boost::posix_time::second_clock::universal_time();

        if (seq_number >= stream_duration + T +T2 - 1)
            break;
    }

    boost::posix_time::ptime end_time = boost::posix_time::second_clock::universal_time();

    cout << "Time duration = " << end_time - start_time << endl;
    cout << "Last sequence number received = " << seq_number << endl;

    delete application_layer_relay_receiver;

    return 0;
}



