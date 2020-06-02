/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Variable_Rate_FEC_Encoder.cpp
 * Author: silas
 * 
 * Created on July 12, 2018, 11:12 AM
 */

#include "Variable_Rate_FEC_Encoder.h"
#include "codingOperations.h"
#include "basicOperations.h"
#include <string.h>

using std::cout;
using std::endl;
using std::ios;

namespace siphon {

    Variable_Rate_FEC_Encoder::Variable_Rate_FEC_Encoder(int max_payload_value, string file_name_input) {

        max_payload = max_payload_value;
        memory_object = new Memory_Allocator(300);
        file_name = file_name_input;
        if (file_name != "")
            file_write_encoder.open(file_name, ios::out | ios::binary); //record original data

        encoder_current = NULL;
        encoder_old = NULL;

        counter_transition = 0;
        transition_flag = 1;
        double_coding_flag = 1;

        number_of_encoded_total = 0;                          //used for calculating coding rate
        final_number_of_encoded_total = 0;
        sum_coding_rate = 0;
        final_sum_coding_rate = 0;
        sum_coding_rate_seg2_symb_wise=0;
        final_sum_coding_rate_seg2_symb_wise=0;
        sum_coding_rate_min_2_seg=0;
        final_sum_coding_rate_min_2_seg = 0;
        MDS_percent = 0;
        adaptive_percent = 0;
        no_coding_percent = 0;
        counter_encoded = 0;
        final_counter_encoded = 0;
        display_final_coding_rate_flag = true;


        report_window_size = ESTIMATION_WINDOW_SIZE;
    }

    Variable_Rate_FEC_Encoder::~Variable_Rate_FEC_Encoder() {
        if ((file_name != "") && file_write_encoder.is_open())
            file_write_encoder.close();

        if (encoder_current != NULL)
            delete encoder_current;

        if (encoder_old != NULL)
            delete encoder_old;

        delete memory_object;
    }

    void Variable_Rate_FEC_Encoder::encode(FEC_Message *message, int T_ack, int B_ack, int N_ack, int flag) {

        if (encoder_current == NULL) {

            T = message->T;
            B = message->B;
            N = message->N;

            encoder_current = new FEC_Encoder(max_payload, T, B, N, memory_object);

            transition_flag = 1;
            double_coding_flag = 0;

        } else {

//            if (((message->T != T) || (message->B != B) || (message->N != N)) &&
//                (transition_flag == 0) && (T_ack == T) && (B_ack == B) &&
//                (N_ack == N) && (T2_ack == T2) && (B2_ack == B2) && (N2_ack == N2)) { //currently not in transition and the current coding parameters have been acknowledged
            if (((message->T != T) || (message->B != B) || (message->N != N)) &&
                (transition_flag == 0) && (T_ack == T) && (B_ack == B)){
                cout<<"Start double coding at the source"<<endl;

                T_old = T;
                B_old = B;
                N_old = N;

                T = message->T;
                B = message->B;
                N = message->N;

                T2_old=T2;
                B2_old=B2;
                N2_old=N2;

                T2 = T_TOT-N;
//                B2 = B2;
//                N2 = N2; // N2 is modified at Applicaiton_Layer_Sender

                transition_flag = 1; //set transition flag
                double_coding_flag = 1;  //set double coding flag

                counter_transition = 0;

                if (encoder_old != NULL)
                    delete encoder_old;
                encoder_old = encoder_current;
                encoder_current = new FEC_Encoder(max_payload, T, B, N, memory_object);

            } else {             //keep the old T, B, N

                message->T = T;
                message->B = B;
                message->N = N;
            }
        }

        onReceivedMessage(message,flag);

        int codeword_size_old = 0;
        int codeword_size_current = 0;

        unsigned char *codeword_old = NULL;
        unsigned char *codeword_current = NULL;

        codeword_current = encoder_current->onTransmit(message->buffer, message->size, message->seq_number,
                                                       &codeword_size_current);

        message->counter_for_start_and_end = counter_transition;

        if (RELAYING_TYPE==2){
            if (counter_transition <= T_TOT+1) {

                if (counter_transition == T_TOT+1) {
                    double_coding_flag = 0;   //no need to use the old encoder to protect the data transmitted at T time slots ago
                    cout<<"Stop double coding at the source"<<endl;
                }

                counter_transition++;

                if ((encoder_old != NULL) && (double_coding_flag == 1))
                    codeword_old = encoder_old->onTransmit(message->buffer, message->size, message->seq_number,
                                                           &codeword_size_old);

//            } else
//                transition_flag = 0;
        } else if (counter_transition>2*T_TOT) //Can assist in debugging !!!
            transition_flag = 0;
        else
            counter_transition++;
        }else {
            if (counter_transition <= T) {

                if (counter_transition == T) {
                    double_coding_flag = 0;   //no need to use the old encoder to protect the data transmitted at T time slots ago
                    cout<<"Stop double coding at the source"<<endl;
                }

                counter_transition++;

                if ((encoder_old != NULL) && (double_coding_flag == 1))
                    codeword_old = encoder_old->onTransmit(message->buffer, message->size, message->seq_number,
                                                           &codeword_size_old);

            } else
                transition_flag = 0;
        }

        // update the transmitted codeword

        int codeword_size_final = codeword_size_current + codeword_size_old +
                                  2;  //2 extra bytes at the very beginning to indicate codeword_size_current
        unsigned char *codeword_final = memory_object->allocate_memory(codeword_size_final);

        int temp_remainder;
        // for (i = 0; i < codeword_size_final; i++)
        // codeword_final[i] = 0x0;
        memset(codeword_final, 0, size_t(codeword_size_final));

        temp_remainder = codeword_size_current % 256;
        codeword_final[1] = (unsigned char) (temp_remainder);
        codeword_final[0] = (unsigned char) ((codeword_size_current - temp_remainder) / 256);

        // for (i = 0; i < codeword_size_current; i++)
        //  codeword_final[i + 2] = codeword_current[i];
        memcpy(codeword_final + 2, codeword_current, size_t(codeword_size_current));


        //for (i = 0; i < codeword_size_old; i++)
        //codeword_final[codeword_size_current + i + 2] = codeword_old[i];
        memcpy(codeword_final + codeword_size_current + 2, codeword_old, size_t(codeword_size_old));

        message->size = codeword_size_final;
        message->buffer = codeword_final;

        number_of_encoded_total++;
        final_number_of_encoded_total++;
        counter_encoded++;
        if (double_coding_flag == false) {
            sum_coding_rate += (float) (T - N + 1) / (T - N + 1 + B);
            final_sum_coding_rate += (float) (T - N + 1) / (T - N + 1 + B);
        } else {
            sum_coding_rate += (float) (T - N + 1) / ((T - N + 1 + B) + (T - N_old + 1) + (T - N_old + 1 + B));
            final_sum_coding_rate +=
                    (float) (T - N + 1) / ((T - N + 1 + B) + (T - N_old + 1) + (T - N_old + 1 + B));
        }
        if (RELAYING_TYPE==2){
            if (double_coding_flag == false) {
                sum_coding_rate_seg2_symb_wise+= (float) (T2 - N2 + 1) / (T2 - N2 + 1 + B2);
                final_sum_coding_rate_seg2_symb_wise+= (float) (T2 - N2 + 1) / (T2 - N2 + 1 + B2);
                sum_coding_rate_min_2_seg+=std::min((float) (T - N + 1) / (T - N + 1 + B),(float) (T2 - N2 + 1) / (T2 - N2 + 1 + B2));
                final_sum_coding_rate_min_2_seg+=std::min((float) (T - N + 1) / (T - N + 1 + B),(float) (T2 - N2 + 1) / (T2 - N2 + 1 + B2));
            } else {//TODO add T2_old into the rate calculations
                sum_coding_rate_seg2_symb_wise+= (float) (T2 - N2 + 1) / ((T2 - N2 + 1 + B2) + (T2 - N2_old + 1) + (T2 - N2_old + 1 + B));
                final_sum_coding_rate_seg2_symb_wise+= (float) (T2 - N2 + 1) / ((T2 - N2 + 1 + B2) + (T2 - N2_old + 1) + (T2 - N2_old + 1 + B));
                sum_coding_rate_min_2_seg+=std::min((float) (T - N + 1) / ((T - N + 1 + B) + (T - N_old + 1) + (T - N_old + 1 + B)),
                                                    (float) (T2 - N2 + 1) / ((T2 - N2 + 1 + B2) + (T2 - N2_old + 1) + (T2 - N2_old + 1 + B)));
                final_sum_coding_rate_min_2_seg+=std::min((float) (T - N + 1) / ((T - N + 1 + B) + (T - N_old + 1) + (T - N_old + 1 + B)),
                                                          (float) (T2 - N2 + 1) / ((T2 - N2 + 1 + B2) + (T2 - N2_old + 1) + (T2 - N2_old + 1 + B)));
            }
        }

        if (RELAYING_TYPE==1 && T2_ack!=0 && flag==0){
            if (double_coding_flag == false) {
                sum_coding_rate_min_2_seg += std::min((float) (T - N + 1) / (T - N + 1 + B),(float) (T2_ack - N2_ack + 1) / (T2_ack - N2_ack + 1 + B2_ack));
                final_sum_coding_rate_min_2_seg += std::min((float) (T - N + 1) / (T - N + 1 + B),(float) (T2_ack - N2_ack + 1) / (T2_ack - N2_ack + 1 + B2_ack));
            } else {
                sum_coding_rate_min_2_seg += std::min((float) (T - N + 1) / ((T - N + 1 + B) + (T - N_old + 1) + (T - N_old + 1 + B)),(float) (T2_ack - N2_ack + 1) / (T2_ack - N2_ack + 1 + B2_ack));
                final_sum_coding_rate_min_2_seg +=
                        std::min((float) (T - N + 1) / ((T - N + 1 + B) + (T - N_old + 1) + (T - N_old + 1 + B)),(float) (T2_ack - N2_ack + 1) / (T2_ack - N2_ack + 1 + B2_ack));
            }
        }
        if(B==0)
            no_coding_percent += 1;
        else
        if (B==N)
            MDS_percent += 1;
        else
            adaptive_percent += 1;

        if (number_of_encoded_total == NUMBER_OF_ITERATIONS) {
            if (RELAYING_TYPE==1){
                if (flag==0) {
                    cout << "Final coding rate in (s,r)= " << final_sum_coding_rate / final_number_of_encoded_total
                         << endl;
                    cout << "Final coding rate (min over two)= "
                         << final_sum_coding_rate_min_2_seg / final_number_of_encoded_total << endl;
                }
                else
                    cout << "Final coding rate in (r,d)= " << final_sum_coding_rate / final_number_of_encoded_total << endl;
            }else if (RELAYING_TYPE==0) {
                cout << "Final coding rate = " << final_sum_coding_rate / final_number_of_encoded_total << endl;
            }else if (RELAYING_TYPE==2){
                cout << "Final coding rate in (s,r)= " << final_sum_coding_rate / final_number_of_encoded_total
                     << endl;
                cout << "Final coding rate in (r,d)= " << final_sum_coding_rate_seg2_symb_wise / final_number_of_encoded_total
                     << endl;
                cout << "Final coding rate (min over two)= " << final_sum_coding_rate_min_2_seg / final_number_of_encoded_total
                     << endl;
            }

            cout << "No coding fraction = " << no_coding_percent / final_number_of_encoded_total << endl;
            cout << "MDS fraction = " << MDS_percent/ final_number_of_encoded_total << endl;
            cout << "Non-MDS fraction = " << adaptive_percent/ final_number_of_encoded_total << endl << endl;

            display_final_coding_rate_flag = false;
            float final_coding_rate = final_sum_coding_rate / final_number_of_encoded_total;

            if (int(final_coding_rate) != 1) {

                int B_max = 0;
                for (B_max = 0; B_max < T; B_max++) {
                    if (T * 1000000 >= int(final_coding_rate * 1000000 * (T + B_max)))
                        continue;
                    else
                        break;
                }

                cout << "The longest burst a fixed-rate code with highest rate lower than "
                     << final_coding_rate << " can correct = " << B_max << endl;


                for (int j = B_max; j >= 1; j--) {

                    int temp_i = 1;
                    for (temp_i = 1; temp_i <= j; temp_i++)
                        if ((T - temp_i + 1) * 1000000 >= int(final_coding_rate * 1000000 * (T - temp_i + 1 + j)))
                            continue;
                        else {

                            cout << "The largest number of arbitrary erasures a fixed-rate code with B = " << j
                                 << " and highest rate lower than"
                                 << final_coding_rate << " can correct = " << temp_i << endl << endl;

                            cout << "Try fixed (B,N) = "
                                 << "(" << j << "," << temp_i << ")" << " with coding rate "
                                 << (float) (T - temp_i + 1) / (T - temp_i + 1
                                                                + j) << endl << endl;

                            break;
                        }
                }
            }
        }

        if (display_final_coding_rate_flag && (counter_encoded == report_window_size)) {
            //reset counter; to display coding rate for every report_window_size of packets
            if (RELAYING_TYPE==1){
                if (flag==0){
                    cout << "Coding rate in (s,r) over " << report_window_size << " packets ending at seq " << message->seq_number
                         << " = " << sum_coding_rate / counter_encoded << endl;
                    cout << "Min coding rate over the two segments " << report_window_size << " packets ending at seq " << message->seq_number
                         << " = " << sum_coding_rate_min_2_seg / counter_encoded << endl;
                }else
                {
                    cout << "Coding rate in (r,d) over " << report_window_size << " packets ending at seq " << message->seq_number
                         << " = " << sum_coding_rate / counter_encoded << endl;
                }
            }else if (RELAYING_TYPE==0){
                cout << "Coding rate over " << report_window_size << " packets ending at seq " << message->seq_number
                     << " = " << sum_coding_rate / counter_encoded << endl;
            }else if (RELAYING_TYPE==2){
                cout << "Coding rate in (s,r) over " << report_window_size << " packets ending at seq " << message->seq_number
                     << " = " << sum_coding_rate / counter_encoded << endl;
                cout << "Coding rate in (r,d) over " << report_window_size << " packets ending at seq " << message->seq_number
                     << " = " << sum_coding_rate_seg2_symb_wise / counter_encoded << endl;
                cout << "Coding rate (min over the two hops) " << report_window_size << " packets ending at seq " << message->seq_number
                     << " = " << sum_coding_rate_min_2_seg / counter_encoded << endl;
            }
            sum_coding_rate = 0;
            sum_coding_rate_min_2_seg=0;
            sum_coding_rate_seg2_symb_wise=0;
            counter_encoded = 0;
        }

        return;
    }

    void Variable_Rate_FEC_Encoder::onReceivedMessage(FEC_Message *message, int flag) {

        if ((file_name != "") && (number_of_encoded_total <= NUMBER_OF_ITERATIONS))
            save_to_file(message->buffer, message->size, &file_write_encoder);

        if (flag==0) {
            if (RELAYING_TYPE>0){
                DEBUG_MSG("\033[1;32m" << "Source message # " << message->seq_number << " (T=" << message->T << ", N=" << message->N << ") R="
                                       << (message->T-message->N+1) << "/" << (message->T+1) << " : " << "\033[0m");
            }
            else
                DEBUG_MSG("\033[1;32m" << "Source message #" << message->seq_number << ": " << "\033[0m");

            if (message->size > 0)
                printMatrix(message->buffer, 1, message->size);
            else
                DEBUG_MSG("null" << endl);
        }
        return;

    }

}
