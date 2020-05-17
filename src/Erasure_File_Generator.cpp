//
// Created by silas on 01/02/19.
//


#include "Erasure_File_Generator.h"
#include "FEC_Macro.h"

#include <random>
#include<fstream>
#include<cstdlib>
#include<iostream>

using std::random_device;
using std::mt19937; //Standard mersenne_twister_engine
using std::ios;
using std::ofstream;

namespace siphon {

    Erasure_File_Generator::Erasure_File_Generator() { good_state = true; }

    Erasure_File_Generator::~Erasure_File_Generator() {}

    void Erasure_File_Generator::generate_IID(int number_of_erasure, float erasure_prob, string filename, int seed) {

        ofstream file_write;
        file_write.open(filename, ios::out | ios::binary);

        ofstream myfile;
        myfile.open (filename.substr (0,filename.length()-4)+".txt");
        mt19937 gen;
        if (seed==0)
            gen.seed(SEED_ARTIFICIAL_ERASURE); //Standard mersenne_twister_engine seeded with rd()
        else
            gen.seed(seed); //Standard mersenne_twister_engine seeded with rd()

        std::uniform_real_distribution<> dist(0, 1);

        unsigned char zero = 0x0;
        unsigned char one = 0x1;

        for (int i = 0; i < number_of_erasure; i++) {
            if (dist(gen) < erasure_prob) {
                file_write.write((char *) &one, 1);
                myfile << "1\n";
            }
            else {
                file_write.write((char *) &zero, 1);
                myfile << "0\n";
            }
        }

        myfile.close();
        file_write.close();

        return;

    }

    void Erasure_File_Generator::generate_three_sections_IID(int number_of_erasure1, float erasure_prob1,int number_of_erasure2,
                                                             float erasure_prob2,int number_of_erasure3, float erasure_prob3, string filename) {

        ofstream file_write;
        file_write.open(filename, ios::out | ios::binary);

        ofstream myfile;
        myfile.open (filename.substr (0,filename.length()-4)+".txt");


        mt19937 gen(SEED_ARTIFICIAL_ERASURE); //Standard mersenne_twister_engine seeded with rd()
        std::uniform_real_distribution<> dist(0, 1);

        unsigned char zero = 0x0;
        unsigned char one = 0x1;

        for (int i = 0; i < number_of_erasure1; i++) {
            if (dist(gen) < erasure_prob1) {
                file_write.write((char *) &one, 1);
                myfile << "1\n";
            }
            else {
                file_write.write((char *) &zero, 1);
                myfile << "0\n";
            }
        }

        for (int i = number_of_erasure1; i < number_of_erasure1+number_of_erasure2; i++) {
            if (dist(gen) < erasure_prob2) {
                file_write.write((char *) &one, 1);
                myfile << "1\n";
            }
            else {
                file_write.write((char *) &zero, 1);
                myfile << "0\n";
            }
        }

        for (int i = number_of_erasure1+number_of_erasure2; i < number_of_erasure1+number_of_erasure2+number_of_erasure3; i++) {
            if (dist(gen) < erasure_prob3) {
                file_write.write((char *) &one, 1);
                myfile << "1\n";
            }
            else {
                file_write.write((char *) &zero, 1);
                myfile << "0\n";
            }
        }
        myfile.close();
        file_write.close();

        return;

    }

    void Erasure_File_Generator::generate_GE(int number_of_erasure, float alpha, float beta, float erasure_prob,
                                             string filename) {

        ofstream file_write;
        file_write.open(filename, ios::out | ios::binary);

        mt19937 gen(SEED_ARTIFICIAL_ERASURE); //Standard mersenne_twister_engine seeded with rd()
        std::uniform_real_distribution<> dist(0, 1);

        unsigned char zero = 0x0;
        unsigned char one = 0x1;

        for (int i = 0; i < number_of_erasure; i++) {

            if (good_state) {
                if (dist(gen) < erasure_prob)
                    file_write.write((char *) &one, 1);
                else
                    file_write.write((char *) &zero, 1);
            } else
                file_write.write((char *) &one, 1);


            //calculate the next state
            if (good_state) {
                if (dist(gen) < alpha)
                    good_state = false;
            } else {
                if (dist(gen) < beta)
                    good_state = true;
            }

        }

        file_write.close();

        return;

    }

    void Erasure_File_Generator::generate_GE_varying(int number_of_erasure, float alpha, float beta, float erasure_prob,
                                                     string filename) {

        ofstream file_write;
        file_write.open(filename, ios::out | ios::binary);

        mt19937 gen(SEED_ARTIFICIAL_ERASURE); //Standard mersenne_twister_engine seeded with rd()
        std::uniform_real_distribution<> dist(0, 1);

        unsigned char zero = 0x0;
        unsigned char one = 0x1;

        for (int i = 0; i < number_of_erasure; i++) {

            if (good_state) {
                if (dist(gen) < erasure_prob)
                    file_write.write((char *) &one, 1);
                else
                    file_write.write((char *) &zero, 1);
            } else
                file_write.write((char *) &one, 1);


            //calculate the next state
            if (good_state) {
                if (dist(gen) < alpha)
                    good_state = false;
            } else {
                bool middle = (i >= number_of_erasure / 3) && (i <= number_of_erasure * 2 / 3);

                double random_temp = dist(gen);
                if (!middle) {
                    //not in the middle
                    if (random_temp < beta)
                        good_state = true;              //with probability = beta, return to GOOD state
                } else                                   //n the middle
                    good_state = true;
            }

        }

        file_write.close();

        return;

    }

    void Erasure_File_Generator::generate_periodic(int number_of_erasure, int T, int B, int N, string filename) {

        ofstream file_write;
        file_write.open(filename, ios::out | ios::binary);

        unsigned char zero = 0x0;
        unsigned char one = 0x1;
        int index;

        for (int i = 0; i < number_of_erasure; i++) {

            index = i % (T - N + 1 + B);

            if (index <= B - 1)
                file_write.write((char *) &one, 1);
            else
                file_write.write((char *) &zero, 1);
        }

        file_write.close();

        return;

    }
}