//
// Created by silas on 11/03/19.
//

#ifndef FEC_ERASURE_CODE_APPLICATION_LAYER_RECEIVER_H
#define FEC_ERASURE_CODE_APPLICATION_LAYER_RECEIVER_H
#include "Variable_Rate_FEC_Decoder.h"
#include "Parameter_Estimator.h"
#include "ConnectionManager.h"
#include "Erasure_Simulator.h"
#include "codingOperations.h"

class Application_Layer_Receiver {

private:
    siphon::Parameter_Estimator *estimator;
    siphon::Parameter_Estimator *background_estimator;
    int flag_for_estimation_cycle;

//    siphon::Variable_Rate_FEC_Decoder *fec_decoderTemp;

    unsigned char *codeword;
//    unsigned char **codeword_vector;
//
//    bool *temp_erasure_vector;

    ConnectionManager *connection_manager;

    int erasure_type;

    int receiver_index;

public:
    Application_Layer_Receiver(const char *Tx, const char *Rx, int max_payload, int erasure_type_value, bool
    adaptive_mode_MDS_value, int flag);
    ~Application_Layer_Receiver();

    int receive_message_and_decode(unsigned char *udp_parameters, unsigned char *udp_codeword, int
  *udp_codeword_size, siphon::Erasure_Simulator *erasure_simulator);          //to be called recursively at the decoder

    int receive_message_and_symbol_wise_encode(unsigned char *udp_parameters, unsigned char *udp_codeword,
                                               int *udp_codeword_size, siphon::Erasure_Simulator
                                                                       *erasure_simulator,int k2, int n2,
                                                                       int *codeword_size_final,int *k2_out,int *n2_out);
    int receive_message_and_symbol_wise_decode(unsigned char *udp_parameters, unsigned char *udp_codeword,
                                           int *udp_codeword_size, siphon::Erasure_Simulator
                                           *erasure_simulator);

    void update_parameter(unsigned char *udp_parameters);    //use parameter estimator to udpate the coding parameters

    void receive(int *message_size, unsigned char *message_buffer, unsigned char *udp_codeword, int
    *udp_codeword_size); // call udp_receive() and obtain the udp codeword

    void update_parameter(unsigned char *coding_parameters, unsigned char *udp_parameters);

    FEC_Message *fec_message;
    FEC_Message *fec_messageTemp;

    siphon::Variable_Rate_FEC_Decoder *fec_decoder;

    void get_current_packet(unsigned char *received_data, int T, int i,int seq_number, int curr_seq_num);

    void set_receiver_index(int index);

//    unsigned char codeword_new_symbol_wise[30000];
    char response_from_dest_buffer[6];
    int latest_seq;



//    void symbol_wise_encode(int k, int n, unsigned char *generator, int temp_size,int k2, int n2);
//
//    void symbol_wise_decode(int k, int n, unsigned char *generator, int temp_size);
//    unsigned char **codeword_new_vector;
//    unsigned char **codeword_vector_store_in_burst;

    int temp_size;
};


#endif //FEC_ERASURE_CODE_APPLICATION_LAYER_RECEIVER_H
