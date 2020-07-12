/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Parameter_Estimator.cpp
 * Author: silas
 * 
 * Created on July 16, 2018, 10:40 AM
 */

#include <cstdlib>
#include <iostream>
#include "Parameter_Estimator.h"
#include "FEC_Macro.h"

using std::cout;
using std::endl;

namespace siphon {

    Parameter_Estimator::Parameter_Estimator(int T_value, bool adaptive_mode_MDS_value) {

        adaptive_mode_MDS = adaptive_mode_MDS_value;

        T = T_value;
        B = 0;
        N = 0;
        N_max = 0;
        B_current = 0;
        N_current = 0;

        erasure = (bool *) malloc(12 * sizeof(bool)); // the window size equals T+1 where T+1<=12

        for (int i = 0; i < 12; i++)
            erasure[i] = 0;

        previous_win_end = -2;

    }

    Parameter_Estimator::Parameter_Estimator(int T_value, int B_value, int N_value) {

        T = T_value;
        B = B_value;
        N = N_value;

        erasure = (bool *) malloc(12 * sizeof(bool)); // the window size equals T+1 where T+1<=12
        for (int i = 0; i < 12; i++)
            erasure[i] = 0;

        previous_win_end = -2;
    }


    void Parameter_Estimator::estimate(FEC_Message *message) {

        if (T == 0)
            return;

        if (previous_win_end == -2) {
            T = message->T;
            // previous_win_end = current_win_end - message->counter_for_start_and_end - 1;
//            previous_win_end = -1;
            previous_win_end=message->seq_number-1; // Elad - to reset the estimator. Need to handle with not receiving packet 0 !!!
            //  seq_start = current_win_end - message->counter_for_start_and_end;
            seq_start = 0;
        }

        if (RELAYING_TYPE==2 || RELAYING_TYPE==3) {
//            T = message->T;
            T=T_TOT;
        }

        current_win_end = message->seq_number;

        int difference = current_win_end - previous_win_end;

        if (difference < 1)
            return;                //ignore out-of-order packets

        int i;

        for (int seq = previous_win_end + 1; seq <= current_win_end; seq++) {// if difference > 1, meaning at

//            if ((seq + 1) % ESTIMATION_WINDOW_SIZE == 0)
//                reset();

            for (i = T; i >= 1; i--)
                erasure[i] = erasure[i - 1]; //

            if (seq < current_win_end)
                erasure[0] = 1;                                 //indicates an erasure
            else
                erasure[0] = 0;                                 //indicates no erasure if offset = difference

            int sum = 0;
            for (i = 0; i <= T; i++)
                if (erasure[i] == 1)
                    sum++;

            if ((sum == T + 1) || (sum == 0))
                continue;                       // if the whole window is erased, just keep B and N unchanged and then move on to the next sliding window

            //the code below runs only if sum is between 1 and T

            if (B == 0)
                B = 1;
            if (N == 0)
                N = 1;

            if (sum > N_max) // record the largest ever number of arbitrary erasures < T + 1
                N_max = sum;

            int first_nonzero, last_nonzero; //used for calculating the span = the last nonzero - first nonzero + 1

            for (i = 0; i <= T; i++)
                if (erasure[i] != 0)
                    break;
            first_nonzero = i;

            for (i = T; i >= 0; i--)
                if (erasure[i] != 0)
                    break;
            last_nonzero = i;

            int span = 0;
            span = last_nonzero - first_nonzero + 1;

            if (span == T + 1) { //if span = T+1, update N and force B to be N, or update both N and B to be N_max
                if (sum > N) {
                    N = sum;
                    B = N;
                }
            } else { //if span < T+1, update either B or N

                int max_B_and_sum;
                if (sum > B)
                    max_B_and_sum = sum;
                else
                    max_B_and_sum = B;

                int max_B_and_span;
                if (span > B)
                    max_B_and_span = span;
                else
                    max_B_and_span = B;

                if ((T - N + 1) * (T - sum + 1 + max_B_and_sum) >= (T - sum + 1) * (T - N + 1 + max_B_and_span)) {
                    if (span > B) {  //update B and keep N unchanged if the resultant rate (T-N+1)/(T-N+1+B) is higher
                        B = span;
                        N = span; // ELAD since we're after a code in which B=N
                    }
                } else {
                    if (sum > N) {// update N, and force B to be at least N if (T-N+1)/(T-N+1+B) is higher
                        N = sum;
                        B = sum; // ELAD since we're after a code in which B=N
                    }
                    if (N > B)
                        B = N;
                }
            }

            // last step: compare the updated (B,N) with (N_max, N_max)
            if ((T - N_max + 1) * (T - N + 1 + B) > (T - N + 1) * (T + 1)) {
                B = N_max;
                N = N_max;
            }
        }

        previous_win_end = current_win_end;

        //will update B_current and N_current if the most recent update corrects more erasures

        if ((T - N_current + 1) * (T - N + 1 + B) >= (T - N + 1) * (T - N_current + 1 + B_current)) {
            B_current = B;
            N_current = N;
        }

        if (adaptive_mode_MDS == true)
            make_MDS_estimates();

        return;
    }


    Parameter_Estimator::~Parameter_Estimator() {
        free(erasure);
    }

    void Parameter_Estimator::reset() {

        //record most recent estimate
        B_current = B;
        N_current = N;
        if (adaptive_mode_MDS == true)
            make_MDS_estimates();

        //parameters reset
        B = 0;
        N = 0;
        N_max = 0;

        for (int i = 0; i < 12; i++)
            erasure[i] = 0;

        // cout << "Most recent estimate for (T, B, N) = (" << T << "," << B_current << "," << N_current << ")" <<endl;
        return;
    }

    void Parameter_Estimator::make_MDS_estimates() {
        if (B_current > N_current) {
            // float rate = float(T - N_current + 1) / (T - N_current + 1 + B_current);
            // if  float(T-N_current)/(T+1)>rate, then increment N_current
            while ((T - N_current) * (T - N_current + 1 + B_current) > (T + 1) * (T - N_current + 1)) {
                N_current++;
            }
            B_current = N_current;
        }
        return;
    }
}
