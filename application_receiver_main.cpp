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

    if ((argc != 5) && (argc != 6)) {
//        argv[1]="127.0.0.1";
//        argv[2]="127.0.0.1";
        argv[1]="192.168.0.22";
        argv[2]="192.168.0.21";
        argv[3]="300";
        argv[4]="0";
//        cout << "Please enter the arguments in this format: (source IP) (destination IP) (packet size in bytes) "
//                "(erasure type as described in include/FEC_Macro.h) (MDS_restricted_estimates boolean flag [optional])."
//             << endl;
//        return 1;
    }

    //Source IP address:
    const char *Tx = argv[1];
    cout << "Source IP: " << argv[1] << endl;
    //Destination IP address:
    const char *Rx = argv[2];
    cout << "Destination IP: " << argv[2] << endl;

    int stream_duration = NUMBER_OF_ITERATIONS;
    int max_payload = std::stoi(argv[3]);

    int erasure_type = std::stoi(argv[4]);

    if((argc==6)&&(std::stoi(argv[5])==1))
        adaptive_mode_MDS = true;

    Application_Layer_Receiver *application_layer_receiver;
    if (RELAYING_TYPE==0) {
        application_layer_receiver = new Application_Layer_Receiver(Tx, Rx, max_payload,
                                                                    erasure_type,
                                                                    adaptive_mode_MDS, 0);
    }else{
        application_layer_receiver = new Application_Layer_Receiver(Tx, Rx, max_payload,
                                                                    erasure_type,
                                                                    adaptive_mode_MDS, 1);
        application_layer_receiver->set_receiver_index(1);
    }

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

    if (RELAYING_TYPE == 0)
        cout << "P2P Receiver" << endl;
    else if (RELAYING_TYPE == 1)
        cout << "Message-wise DF Receiver" << endl;
    else
        cout << "Symbol-wise DF Receiver" << endl;

    int seq_number;

    for (int i = 0;; i++) {
        if (RELAYING_TYPE == 0) {
            seq_number = application_layer_receiver->receive_message_and_decode(nullptr, nullptr, nullptr,
                                                                                &erasure_simulator);
        }else if (RELAYING_TYPE == 1) {
            seq_number = application_layer_receiver->receive_message_and_decode(nullptr, nullptr, nullptr,
                                                                                &erasure_simulator);

        }else if (RELAYING_TYPE == 2) {
            int n=T_INITIAL_2+1;
            int k=T_INITIAL_2-N_INITIAL_2+1;
            seq_number = application_layer_receiver->receive_message_and_symbol_wise_decode(
                    nullptr, nullptr,
                    nullptr,
                    &erasure_simulator);
        }

        if (i == 0)
            boost::posix_time::ptime start_time = boost::posix_time::second_clock::universal_time();

        if (seq_number >= stream_duration + T - 1)
            break;
    }

    boost::posix_time::ptime end_time = boost::posix_time::second_clock::universal_time();

    cout << "Time duration = " << end_time - start_time << endl;
    cout << "Last sequence number received = " << seq_number << endl;

    delete application_layer_receiver;

    return 0;
}

