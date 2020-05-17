//
// Created by silas on 30/01/19.
//

#include "Erasure_Simulator.h"
#include <sys/stat.h>
#include<cstdlib>

using std::ios;

namespace siphon {

    Erasure_Simulator::Erasure_Simulator(string filename_value) {

        string filename = filename_value;

        ifstream file_read;
        file_read.open(filename, ios::in | ios::binary);        //open file

        file_read.seekg(0, ios::end);                           //find file size
        long fileSize = file_read.tellg();
        number_of_erasure = fileSize;                                       //record erasure size = file size
        erasure_seq = (unsigned char *) malloc(number_of_erasure * sizeof(unsigned char));

        file_read.seekg(0, ios::beg);
        for (int i = 0; i < number_of_erasure; i++)
            file_read.read((char *) (erasure_seq + i), 1);

        file_read.close();

    }

    Erasure_Simulator::Erasure_Simulator() {

        number_of_erasure = 10000;                                                  //record erasures
        erasure_seq = (unsigned char *) malloc(number_of_erasure * sizeof(char));

        for (int seq = 0; seq < number_of_erasure; seq++)
            if (((seq >= 5) &&
                 (seq <= 8)) || ((seq >= 16) && (seq <= 19)) || ((seq >= 27) && (seq <= 30)) ||
                ((seq >= 38) && (seq <= 41))) {
                erasure_seq[seq] = 0x1;
            } else
                erasure_seq[seq] = 0x0;

    }

    Erasure_Simulator::~Erasure_Simulator() {
        free(erasure_seq);
    }

    bool Erasure_Simulator::is_erasure(int seq) {
        if ((erasure_seq[seq] == 0x1) && (seq < number_of_erasure))
            return true;
        else
            return false;
    }

}