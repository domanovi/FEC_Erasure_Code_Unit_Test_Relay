/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Payload_Simulator.h
 * Author: silas
 *
 * Created on July 26, 2018, 12:39 PM
 */

#ifndef PAYLOAD_SIMULATOR_H
#define PAYLOAD_SIMULATOR_H

#include <string>
#include <unistd.h>
#include <fstream>
#include "boost/date_time/posix_time/posix_time.hpp"

using namespace std;

class Payload_Simulator {
public:
    Payload_Simulator(int total_iteration_value, int max_payload_value, int interarrival_time_value, string
    file_name_source_value);
  
    virtual ~Payload_Simulator();
    
    int generate_payload(unsigned char * payload_buffer) ;

    long current_file_position;
private:
    
    int max_payload, interarrival_time;

    int total_iteration, read_counter;

    string file_name_source;

    ifstream file_read;

    long fileSize;

    boost::posix_time::ptime start_time;

};

#endif /* PAYLOAD_SIMULATOR_H */

