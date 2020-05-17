/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Payload_Simulator.cpp
 * Author: silas
 * 
 * Created on July 26, 2018, 12:39 PM 
 * 
 */

#include "Payload_Simulator.h"
#include "FEC_Macro.h"
#include<cstdlib>
#include<iostream>
#include <chrono>
#include<thread>

Payload_Simulator::Payload_Simulator(int total_iteration_value, int max_payload_value, int interarrival_time_value,
                                     string file_name_source_value) {

    total_iteration = total_iteration_value;
    max_payload = max_payload_value;
    interarrival_time = interarrival_time_value;
    file_name_source = file_name_source_value;

    if (file_name_source != "")
        file_read.open(file_name_source, ios::out | ios::binary); //record original data

    file_read.seekg(0, ios::end);                           //find file size
    fileSize = file_read.tellg();
    file_read.seekg(0, ios::beg);                           //set file pointer to the beginning
    current_file_position = 0;
    read_counter = 1;
    start_time = boost::posix_time::microsec_clock::universal_time();

}

Payload_Simulator::~Payload_Simulator() {
    if (file_name_source != "")
        file_read.close();
}

int Payload_Simulator::generate_payload(unsigned char *payload_buffer) {

    for (int i = 0; i < 11; i++) {
        std::this_thread::sleep_for(std::chrono::microseconds(interarrival_time / 10));
        boost::posix_time::time_duration diff = boost::posix_time::microsec_clock::universal_time() - start_time;
        if (diff.total_microseconds() >= interarrival_time * read_counter) {
            read_counter++;
            break;
        }
    }

    if (read_counter == 10000) {
        start_time = boost::posix_time::microsec_clock::universal_time();
        read_counter = 1;
    }


    if (current_file_position + max_payload < fileSize) {

        file_read.seekg(current_file_position, ios::beg);                           //set file pointer
        file_read.read((char *) payload_buffer, max_payload);
        current_file_position += max_payload;
    } else {

        file_read.read((char *) payload_buffer, fileSize - current_file_position);
        cout << "File size = " << fileSize << endl;
        cout << "End of source file. Reading from the beginning again..." << endl;
        file_read.seekg(0, ios::beg);                           //set file pointer
        if (max_payload - fileSize + current_file_position > 0)
            file_read.read((char *) payload_buffer + fileSize - current_file_position,
                           max_payload - fileSize + current_file_position);

        current_file_position = max_payload - fileSize + current_file_position;
    }

    return 1;
}
