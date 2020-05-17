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

    if ((argc != 5)&&(argc !=7)) {
        argv[1]="127.0.0.1";
        argv[2]="127.0.0.1";
        argv[3]="300";
        argv[4]="1000";
//        cout << "Please enter the arguments in this format: (source IP) (destination IP) (packet size in bytes) "
//             << "(packet interarrival time in ms (smallest is 0.01 ms)) (B (optional)) (N (optional))"
//             << endl;
//        return 1;
    }

//    int B=-1;
//    int N=-1;

    int T=T_INITIAL;
    int B=N_INITIAL;
    int N=N_INITIAL;

    if(argc == 7){
        B=std::stoi(argv[5]);
        N=std::stoi(argv[6]);
        if(B>T_INITIAL)
            B=T_INITIAL;
    }

    int packet_size = std::stoi(argv[3]);
    int packet_interarrival_time = (int) (std::stof(argv[4])*1000);

    cout<< "Packet interarrival time = "<< packet_interarrival_time << " microseconds"<<endl;

    int seq_start = 0;

    //int T = T_INITIAL;

   // int stream_duration = NUMBER_OF_ITERATIONS;

    //Source IP address:
    const char *Tx = argv[1];
    cout << "Source IP: " << argv[1] << endl;
    //Destination IP address:
    const char *Rx = argv[2];
    cout << "Destination IP: " << argv[2] << endl;

    Application_Layer_Sender application_layer_sender(Tx, Rx, packet_size, packet_interarrival_time, T, B, N,0);

   // boost::posix_time::ptime start_time = boost::posix_time::microsec_clock::universal_time();

    for (int seq_number = seq_start; ;seq_number++) {
        application_layer_sender.generate_message_and_encode(nullptr, nullptr, nullptr); //udp_codeword is sent;
    }

   // boost::posix_time::ptime end_time = boost::posix_time::microsec_clock::universal_time();

   // cout << "Time duration = " << end_time - start_time << endl;
   // cout << "Iteration = " << stream_duration << endl;

    return 0;
}

