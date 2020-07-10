//
// Created by silas on 30/01/19.
//

#ifndef SIPHON_ERASURE_SIMULATOR_H
#define SIPHON_ERASURE_SIMULATOR_H

#include<string>
#include <fstream>

using std::string;
using std::ifstream;

namespace siphon {

    class Erasure_Simulator {

    public:
        unsigned char *erasure_seq;
        bool is_erasure(int seq);

        Erasure_Simulator();

        Erasure_Simulator(string filename_value);

        ~Erasure_Simulator();

        //unsigned char *erasure_seq;
        long number_of_erasure;
    };

}
#endif //SIPHON_ERASURE_SIMULATOR_H
