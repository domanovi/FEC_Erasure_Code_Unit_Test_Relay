//
// Created by silas on 21/12/18.
//

#ifndef SIPHON_FEC_MACRO_H
#define SIPHON_FEC_MACRO_H

#define SOURCE_PCM_FILE "conversation_ref.wav" //chose a file larger than (NUMBER_OF_ITERATIONS/
// (1000/INTERARRIVAL_TIME))*MAX_PAYLOAD Bytes
  
//#define INPUTDATAFILE "inputData.wav" //used by VariableRateEncoder
#define INPUTDATAFILE ""
//#define OUTPUTDATAFILE "outputData40_UDP.wav"     //used by VariableRateDecoder, set B_INITIAL = N_INITIAL = 0
//#define OUTPUTDATAFILE "outputData40_31.wav"  //    for fixed coding (e.g., B=3, N=1)
//#define OUTPUTDATAFILE "outputData40.wav"          //B_INITIAL = -1, N_INITIAL = -1, meaning adaptive
//#define OUTPUTDATAFILE "outputData40_MDS.wav"
#define OUTPUTDATAFILE ""

//#define T_INITIAL 10
//#define B_INITIAL -1
//#define N_INITIAL -1
#define T_TOT 20 // T_TOT needs to be >= T_INITAL+T_INTIAL_2
#define T_INITIAL 4
#define B_INITIAL -1
#define N_INITIAL 2

#define T_INITIAL_2 3
#define B_INITIAL_2 2
#define N_INITIAL_2 1

#define ESTIMATION_WINDOW_SIZE 1000    //the size of window used for estimating coding parameters.

//#define NUMBER_OF_ITERATIONS 361000   //total number of iterations. Recommended: 50 times ESTIMATION_WINDOW_SIZE,
#define NUMBER_OF_ITERATIONS 10000
// 360000 packets = 3600 seconds =  60 minutes, 1 minute = 6000 packets

#define ERASURE_TYPE 1
// If ERASURE_TYPE=0, generate no artificial erasures;
// if ERASURE_TYPE=1, iid erasures with Bernoulli(EPSILON) are introduced;
// if ERASURE_TYPE=2, erasures are generated according Gilbert-Elliot channel (ALPHA, BETA, EPSILON);
// if ERASURE_TYPE=3, erasures are generated according to the above Gilbert-Elliot channel with the middle third phase having beta doubled
// if ERASURE_TYPE=4, erasures are generated according to (T,B,N)-periodic erasures
// if Erasure_Type=5, erasures are obtained from the recorded erasures in erasure.bin inside the bin directory
// if ERASURE_TYPE=6, iid erasures with Bernoulli(EPSILON) are introduced on three regions;


#define RELAYING_TYPE 2
// if RELAYING_TYPE=0 P2P
// if RELAYING_TYPE=1 message-wise decode and forward
// if RELAYING_TYPE=2 symbol-wise decode and forward

#define ERASURE_RECORDER 0         //If ERASURE_RECORDER equals 1, erasure will be saved to "erasure.bin" inside the
// bin directory, active only when ERASURE_TYPE not equal to 5


/**************** The following is used for arbitrary erasures ******************/

#define SEED_ARTIFICIAL_ERASURE 1       //the seed for generating artificial erasures

//#define EPSILON 0.001       //probability of random erasures
#define EPSILON 0.1       //probability of random erasures
#define EPSILON_2 0       //probability of random erasures
#define EPSILON_3 0.1       //probability of random erasures


#define ALPHA 0.01            //probability of transitioning from GOOD state to BAD state

#define BETA 0.3         //probability of transitioning from BAD state to GOOD state, expected length of burst
// erasure = 1/BETA

#define ERASURE_T 20

#define ERASURE_B 5

#define ERASURE_N 1




/*************** The following is used for displaying debugging messages *******************/

#define DEBUG_FEC 1 //debugging messages hidden if DEBUG_FEC is 0

#if DEBUG_FEC
#define DEBUG_MSG(str) do {std::cout << str << std::endl; } while( false )
#else
#define DEBUG_MSG(str) do {} while ( false )
#endif

#endif //SIPHON_FEC_MACRO_H
