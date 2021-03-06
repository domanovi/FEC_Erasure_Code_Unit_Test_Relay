//
// Created by silas on 11/03/19.
//

#include <basicOperations.h>
#include "Application_Layer_Sender.h"
#include "FEC_Macro.h"

Application_Layer_Sender::Application_Layer_Sender(const char *Tx, const char *Rx, int packet_size,
                                                   int packet_interarrival_time, int T_value, int B_value, int N_value,int flag_val) {
    seq_number = 0;

    T = T_value;

    T_ack = T;

    if ((B_value == -1) || (N_value == -1)) {
        B = 0;
        N = 0;
        B_ack = 0;
        N_ack = 0;
        adaptive_coding = true;
    } else {
        B = B_value;
        N = N_value;
        B_ack = B_value;
        N_ack = N_value;
        adaptive_coding = false;
    }

    max_payload = packet_size;

    variable_rate_FEC_encoder = new siphon::Variable_Rate_FEC_Encoder(max_payload, INPUTDATAFILE);
    raw_data = (unsigned char *) malloc(sizeof(unsigned char) * max_payload);
    codeword = (unsigned char *) malloc(
            sizeof(unsigned char) * (max_payload + 12) * 4 * T_TOT); //4-byte sequence number + 4-byte
    // coding parameters

    message_transmitted = (FEC_Message *) malloc(sizeof(FEC_Message));
    payload_simulator = new Payload_Simulator(NUMBER_OF_ITERATIONS, packet_size, packet_interarrival_time,
                                              SOURCE_PCM_FILE);
    flag=flag_val;
    connection_manager = new ConnectionManager(Tx, Rx,flag_val);

    codeword_new_vector=(unsigned char **) malloc(sizeof(unsigned char *) * (T_TOT+1));
    for (int i=0;i<T_TOT+1;i++){
        codeword_new_vector[i]=(unsigned char *) malloc(sizeof(unsigned char) * (max_payload + 12) * 4 * T_TOT); //4-byte sequence number + 4-byte
    }

    debug_rate_second_hop_num_packets=0;
    debug_rate_second_hop=0;

}

Application_Layer_Sender::~Application_Layer_Sender() {
    free(message_transmitted);
    free(raw_data);
    free(codeword);
    delete payload_simulator;
    delete variable_rate_FEC_encoder;
    delete connection_manager;
}

void Application_Layer_Sender::generate_message_and_encode(unsigned char *udp_parameters, unsigned char
*udp_codeword, int *udp_codeword_size) {

    // This
    // function is
    // called recursively at the sender
    payload_simulator->generate_payload(raw_data);                                  //fill raw data

    int response_size;
    unsigned char response_buffer[12];

    int T2_ack,B2_ack,N2_ack;

    if (udp_parameters == nullptr)
        update_parameter(&response_size, response_buffer);
    else{
        for (int i=0;i<12;i++)
            response_buffer[i]=udp_parameters[i];
        if((adaptive_coding == 1)&& ((int) udp_parameters[0]!=0)) {
            T = (int) udp_parameters[0];
            B = (int) udp_parameters[1];
            N = (int) udp_parameters[2];
            T_ack = (int) udp_parameters[3];
            B_ack = (int) udp_parameters[4];
            N_ack = (int) udp_parameters[5];
            T2 = (int) udp_parameters[6];
            B2 = (int) udp_parameters[7];
            N2 = (int) udp_parameters[8];
            T2_ack = (int) udp_parameters[9];
            B2_ack = (int) udp_parameters[10];
            N2_ack = (int) udp_parameters[11];
        }
    }
//    cout << "Response at source" << endl;
    //printMatrix(response_buffer, 1, 12);
    if (DEBUG_COMM==1) {
        cout << "Response at source" << endl;
        printMatrix(response_buffer, 1, 12);
    }
    if (RELAYING_TYPE==1 && seq_number>0 && adaptive_coding==1) {
        variable_rate_FEC_encoder->T2_ack = T2_ack;
        variable_rate_FEC_encoder->N2_ack = N2_ack;
        variable_rate_FEC_encoder->B2_ack = B2_ack;
    }

    if ((RELAYING_TYPE==2 || RELAYING_TYPE==3) && seq_number>0 && adaptive_coding==1){
//        if (N>=T_TOT) {
//            cout << "N1=T_TOT" << endl;
//            N=N_ack;
//        }
//        else if (N+N2<=T_TOT) {
//        if (DOUBLE_ERAUSRE_NUM>1) {
//            N = std::min(T_TOT, (int)floor(DOUBLE_ERAUSRE_NUM * N));
//            N2 = std::min(T_TOT, (int) floor(DOUBLE_ERAUSRE_NUM * N2));
//        }
        N = std::min(T_TOT, (int)floor(DOUBLE_ERAUSRE_NUM * N));
        N2 = std::min(T_TOT, (int) floor(DOUBLE_ERAUSRE_NUM * N2));
        if (N+N2<=T_TOT) {
            T = T_TOT - N2;
            T2 = T_TOT - N;
            if (T>=1) // T1 cannot be lower than 1
            {
                if (MIN_T2>0) {
                    if (T2 >= MIN_T2) {
                        variable_rate_FEC_encoder->N2 = N2;
                        variable_rate_FEC_encoder->B2 = N2;
                    } else {
                        N = N_ack;
                        T = T_ack;
                    }
                }else{ //T1>=1
                    if (MIN_N2>0 && N2<MIN_N2){
                        N2=MIN_N2;
                        T=T_TOT-N2;
                        N=std::min(N,T);
                        T2=T_TOT-N;
                        variable_rate_FEC_encoder->N2 = N2;
                        variable_rate_FEC_encoder->B2 = N2;
                    }else {
                        variable_rate_FEC_encoder->N2 = N2;
                        variable_rate_FEC_encoder->B2 = N2;
                    }
                }

            }else{//recommended T1=0
                T=1;
                N2=T_TOT-T;
                N=std::min(N,T);
                T2=T_TOT-N;
                variable_rate_FEC_encoder->N2 = N2;
                variable_rate_FEC_encoder->B2 = N2;
//                N=N_ack;
//                T=T_ack;
            }
//            if (T2>=4) {
//                variable_rate_FEC_encoder->N2 = N2;
//                variable_rate_FEC_encoder->B2 = N2;
//            }else{
//                N=N_ack;
//                T=T_ack;
//            }
//            variable_rate_FEC_encoder->N2 = N2;
//            variable_rate_FEC_encoder->B2 = N2;

//        T2=T_TOT-N;
        }else {//N+N2>T_TOT
            // split proportionally
            if (SPLIT_PROP == 1) {
                int N_temp = (int) floor((N * T_TOT) / (N + N2));
                int N2_temp = (int) floor((N2 * T_TOT) / (N + N2));
                cout << "N1=" << N << "+N2=" << N2 << ">T_TOT=" << T_TOT << " N1new=" << N_temp << ", N2new=" << N2_temp
                     << endl;
                N = N_temp;
                N2 = N2_temp;
                T = T_TOT - N2;
                T2 = T_TOT - N;
                variable_rate_FEC_encoder->N2 = N2;
                variable_rate_FEC_encoder->B2 = N2;
            }else{
//           // give priority to the 2nd hop
//            N=T_TOT-N2;
//            T = T_TOT - N2;
//            variable_rate_FEC_encoder->N2 = N2;
//            variable_rate_FEC_encoder->B2 = N2;
                // Stay with current values
                cout<<"N1="<<N << "+N2=" << N2 <<">T_TOT="<<T_TOT<<endl;
                N=N_ack;
                T=T_ack;
            }
        }

        variable_rate_FEC_encoder->T2_ack = T2_ack;
        variable_rate_FEC_encoder->B2_ack = B2_ack;
        variable_rate_FEC_encoder->N2_ack = N2_ack;
    }

    if (RELAYING_TYPE>0)// Currently supporting only arbitrary erasures
        message_transmitted->set_parameters(seq_number, T, N, N, max_payload, raw_data);
    else
        message_transmitted->set_parameters(seq_number, T, B, N, max_payload, raw_data);
//    if (RELAYING_TYPE==2 && adaptive_coding==1) {
//        variable_rate_FEC_encoder->T2_ack = T2_ack;
//        variable_rate_FEC_encoder->B2_ack = B2_ack;
//        variable_rate_FEC_encoder->N2_ack = N2_ack;
//    }

//    if ((N!=N_ack || N2!=N2_ack) && (N_ack+N2_ack<=T_TOT)){
//        T=T_TOT-N2_ack;
//        T_ack=T;
//        T2=T_TOT-N_ack;
//    }


    variable_rate_FEC_encoder->encode(message_transmitted, T_ack, B_ack, N_ack,flag);

//   for (int i = 0; i < message_transmitted->size; i++)
//  codeword[8 + i] = (message_transmitted->buffer)[i];

    if (RELAYING_TYPE==2 || RELAYING_TYPE==3){
        memcpy(codeword + 16, message_transmitted->buffer, message_transmitted->size);

        codeword[15] = (unsigned char) (variable_rate_FEC_encoder->N2_old);
        codeword[14] = (unsigned char) (variable_rate_FEC_encoder->T2_old);

        codeword[13] = (unsigned char) (variable_rate_FEC_encoder->N_old);
        codeword[12] = (unsigned char) (variable_rate_FEC_encoder->T_old);

        codeword[11] = (unsigned char) (message_transmitted->counter_for_start_and_end);
        codeword[10] = (unsigned char) (variable_rate_FEC_encoder->N2);
        codeword[9] = (unsigned char) (variable_rate_FEC_encoder->B2);
        codeword[8] = (unsigned char) (variable_rate_FEC_encoder->T2);

        codeword[7] = (unsigned char) (message_transmitted->counter_for_start_and_end);
        codeword[6] = (unsigned char) (message_transmitted->N);
        codeword[5] = (unsigned char) (message_transmitted->B);
        codeword[4] = (unsigned char) (message_transmitted->T);

        codeword[3] = (unsigned char) (seq_number % 256);
        codeword[2] = (unsigned char) ((seq_number / 256) % 256);
        codeword[1] = (unsigned char) ((seq_number / 256 / 256) % 256);
        codeword[0] = (unsigned char) ((seq_number / 256 / 256 / 256) % 256);
        if (DEBUG_COMM==1) {
            cout << "Sending from source to relay (in codeword)" << endl;
            printMatrix(codeword, 1, 12);
        }

// send(8 + message_transmitted->size, codeword, udp_codeword, udp_codeword_size);
        if (udp_parameters == nullptr)
            connection_manager->UDPsend(12 + message_transmitted->size, codeword);
        else {
            *udp_codeword_size = 12 + message_transmitted->size;
            memcpy(udp_codeword, codeword, size_t(*udp_codeword_size));
        }
    }else {

        memcpy(codeword + 8, message_transmitted->buffer, message_transmitted->size);

        codeword[7] = (unsigned char) (message_transmitted->counter_for_start_and_end);
        codeword[6] = (unsigned char) (message_transmitted->N);
        codeword[5] = (unsigned char) (message_transmitted->B);
        codeword[4] = (unsigned char) (message_transmitted->T);

        codeword[3] = (unsigned char) (seq_number % 256);
        codeword[2] = (unsigned char) ((seq_number / 256) % 256);
        codeword[1] = (unsigned char) ((seq_number / 256 / 256) % 256);
        codeword[0] = (unsigned char) ((seq_number / 256 / 256 / 256) % 256);

// send(8 + message_transmitted->size, codeword, udp_codeword, udp_codeword_size);
        if (udp_parameters == nullptr)
            connection_manager->UDPsend(8 + message_transmitted->size, codeword);
        else {
            *udp_codeword_size = 8 + message_transmitted->size;//ELAD to change 300
            memcpy(udp_codeword, codeword, size_t(*udp_codeword_size));
        }
    }

    seq_number++;

}

void Application_Layer_Sender::send_sym_wise_message(unsigned char *encoded_symwise_word, int encoded_symwise_word_size,
                                                     unsigned char *udp_parameters, unsigned char *udp_codeword, int *udp_codeword_size,int missing_packets,
                                                     int k2,int n2,int counter_for_start_and_end) {

    int response_size;
    unsigned char response_buffer[6];

//    seq_number=seq_number+missing_packets;
    seq_number=missing_packets;


    if (udp_parameters == nullptr)
        update_parameter(&response_size, response_buffer);
    else{
        for (int i=0;i<6;i++)
            response_buffer[i]=udp_parameters[i];
        if((adaptive_coding == 1)&& ((int) udp_parameters[0]!=0)) {
            T = (int) udp_parameters[0];
            B = (int) udp_parameters[1];
            N = (int) udp_parameters[2];
            T_ack = (int) udp_parameters[3];
            B_ack = (int) udp_parameters[4];
            N_ack = (int) udp_parameters[5];
        }
    }

//    message_transmitted->set_parameters(seq_number, T, B, N, max_payload, raw_data);
//    variable_rate_FEC_encoder->encode(message_transmitted, T_ack, B_ack, N_ack);

    //   for (int i = 0; i < message_transmitted->size; i++)
    //  codeword[8 + i] = (message_transmitted->buffer)[i];


    memcpy(codeword + 8, encoded_symwise_word, encoded_symwise_word_size);
    //memcpy(codeword , encoded_symwise_word, encoded_symwise_word_size);

//    int counter_for_start_and_end=0;

    codeword[7] = (unsigned char) (counter_for_start_and_end);
    codeword[6] = (unsigned char) (n2-k2);//N
    codeword[5] = (unsigned char) (n2-k2);//B
    codeword[4] = (unsigned char) (n2-1);//T

    codeword[3] = (unsigned char) (seq_number % 256);
    codeword[2] = (unsigned char) ((seq_number / 256) % 256);
    codeword[1] = (unsigned char) ((seq_number / 256 / 256) % 256);
    codeword[0] = (unsigned char) ((seq_number / 256 / 256 / 256) % 256);
    if (DEBUG_COMM==1) {
        cout << "Sending from relay to destination (in codeword)" << endl;
        printMatrix(codeword, 1, 8);
    }

    // send(8 + message_transmitted->size, codeword, udp_codeword, udp_codeword_size);
    if (udp_parameters == nullptr)
        connection_manager->UDPsend(encoded_symwise_word_size+8, codeword);
    else{
        *udp_codeword_size = encoded_symwise_word_size+8;
        memcpy(udp_codeword,codeword, size_t(*udp_codeword_size));
    }

    seq_number++;

}

void Application_Layer_Sender::message_wise_encode_at_relay(unsigned char *received_data, int orig_seq_num, unsigned char *udp_parameters, unsigned char
*udp_codeword, int *udp_codeword_size) {

    // This
    // function is
    // called recursively at the sender
    //payload_simulator->generate_payload(raw_data);                                  //fill raw data

    int response_size;
    unsigned char response_buffer[6];
    if (udp_parameters == nullptr)
        update_parameter(&response_size, response_buffer);
    else{
        for (int i=0;i<6;i++)
            response_buffer[i]=udp_parameters[i];
        if((adaptive_coding == 1)&& ((int) udp_parameters[0]!=0)) {
            T = (int) udp_parameters[0];
            B = (int) udp_parameters[1];
            N = (int) udp_parameters[2];
            T_ack = (int) udp_parameters[3];
            B_ack = (int) udp_parameters[4];
            N_ack = (int) udp_parameters[5];
        }
    }
    if (DEBUG_COMM==1) {
        cout << "Response at relay transmitter" << endl;
        printMatrix(response_buffer, 1, 6);
    }

    message_transmitted->set_parameters(orig_seq_num, T, B, N, max_payload, received_data);

    if (message_transmitted->size > 0) {
        DEBUG_MSG("\033[1;36m" << "Transmitted message at relay # "  << message_transmitted->seq_number << " (T=" << message_transmitted->T << ", N="
                               << message_transmitted->N <<") R=" << message_transmitted->T-message_transmitted->N+1 << "/" << message_transmitted->T+1 << " : " << "\033[0m");
        printMatrix(message_transmitted->buffer, 1, message_transmitted->size);
    }
    else
        DEBUG_MSG("null" << endl);

    variable_rate_FEC_encoder->encode(message_transmitted, T_ack, B_ack, N_ack,flag);
//    DEBUG_MSG("\033[1;36m" << "Relay->destination message #" << message_transmitted->seq_number << ": (T=" << T_ack << ", N=" <<
//                           N_ack << ") R=" << T_ack-N_ack+1 << "/" << T_ack+1 << "\033[0m");
//    DEBUG_MSG("\033[1;36m" << "Relay->destination message #" << message_transmitted->seq_number << ": (T=" << T_ack << ", N=" <<
//                           N_ack << ") R=" << message_transmitted->T-message_transmitted->N+1 << "/" << message_transmitted->T+1 << "\033[0m");
    debug_rate_second_hop_num_packets+=1;
    debug_rate_second_hop_curr=(float)(message_transmitted->T-message_transmitted->N+1)/(message_transmitted->T+1);
    debug_rate_second_hop+=(float)(message_transmitted->T-message_transmitted->N+1)/(message_transmitted->T+1);
    //   for (int i = 0; i < message_transmitted->size; i++)
    //  codeword[8 + i] = (message_transmitted->buffer)[i];

    memcpy(codeword + 12, message_transmitted->buffer, message_transmitted->size);

    codeword[11] = (unsigned char) (message_transmitted->counter_for_start_and_end);
    codeword[10] = (unsigned char) (message_transmitted->N);
    codeword[9] = (unsigned char) (message_transmitted->B);
    codeword[8] = (unsigned char) (message_transmitted->T);

    if (seq_number>orig_seq_num)
        cout << "missing " << endl;

    // For error analysis purposes on the 2nd link need to add this counter
    codeword[7] = (unsigned char) (seq_number % 256);
    codeword[6] = (unsigned char) ((seq_number / 256) % 256);
    codeword[5] = (unsigned char) ((seq_number / 256 / 256) % 256);
    codeword[4] = (unsigned char) ((seq_number / 256 / 256 / 256) % 256);

    codeword[3] = (unsigned char) (orig_seq_num % 256);
    codeword[2] = (unsigned char) ((orig_seq_num / 256) % 256);
    codeword[1] = (unsigned char) ((orig_seq_num / 256 / 256) % 256);
    codeword[0] = (unsigned char) ((orig_seq_num / 256 / 256 / 256) % 256);

    // send(8 + message_transmitted->size, codeword, udp_codeword, udp_codeword_size);
    if (udp_parameters == nullptr)
        connection_manager->UDPsend(12 + message_transmitted->size, codeword);
    else{
        *udp_codeword_size = 12 + message_transmitted->size;
        memcpy(udp_codeword,codeword, size_t(*udp_codeword_size));
    }

    seq_number++;

}

void Application_Layer_Sender::update_parameter(int *response_size, unsigned char *response_buffer) {

    connection_manager->UDPreceiveResponse(response_size, response_buffer);

    if ((*response_size > 0) && (adaptive_coding == 1)) {
        T = (int) response_buffer[0];
        B = (int) response_buffer[1];
        N = (int) response_buffer[2];
        T_ack = (int) response_buffer[3];
        B_ack = (int) response_buffer[4];
        N_ack = (int) response_buffer[5];

    }
}

