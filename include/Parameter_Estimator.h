/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Parameter_Estimator.h
 * Author: silas
 *
 * Created on July 16, 2018, 10:40 AM
 */

#ifndef PARAMETER_ESTIMATOR_H
#define PARAMETER_ESTIMATOR_H

#include "FEC_Message.h"

namespace siphon{

    class Parameter_Estimator {
    public:
        Parameter_Estimator(int T_value, bool adaptive_mode_MDS_value);

        Parameter_Estimator(int T_value, int B_value, int N_value);

        virtual ~Parameter_Estimator();

        void estimate(FEC_Message *message);

        void reset();

        void make_MDS_estimates();

    public:

        bool adaptive_mode_MDS;

        int seq_start;

        int T, B, N, N_max;

        int B_current, N_current;

        bool *erasure;

        int previous_win_end, current_win_end;

    };
}
#endif /* PARAMETER_ESTIMATOR_H */

