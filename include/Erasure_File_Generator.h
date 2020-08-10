//
// Created by silas on 01/02/19.
//

#ifndef SIPHON_ERASURE_FILE_GENERATOR_H
#define SIPHON_ERASURE_FILE_GENERATOR_H

#include<cstdlib>
#include<string>

using std::string;

namespace siphon {

    class Erasure_File_Generator {

    private:
        bool good_state;

    public:
        Erasure_File_Generator();

        ~Erasure_File_Generator();

        void generate_IID(int number_of_erasure, float erasure_prob, string filename,int seed);

        void generate_GE(int number_of_erasure, float alpha, float beta, float erasure_prob, string filename,int seed);

        void generate_GE_varying(int number_of_erasure, float alpha, float beta, float erasure_prob, string filename,int seed);

        void generate_periodic(int number_of_erasure, int T, int B, int N, string filename);

        void
        generate_three_sections_IID(int number_of_erasure1, float erasure_prob1, int number_of_erasure2,
                                    float erasure_prob2,
                                    int number_of_erasure3, float erasure_prob3, string filename,int seed);
    };

}
#endif //SIPHON_ERASURE_FILE_GENERATOR_H
