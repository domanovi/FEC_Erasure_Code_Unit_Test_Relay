//
// Created by silas on 11/03/19.
//

#include <basicOperations.h>
#include <Payload_Simulator.h>
#include "Application_Layer_Receiver.h"
#include "FEC_Macro.h"

Application_Layer_Receiver::Application_Layer_Receiver(const char *Tx, const char *Rx, int max_payload, int
erasure_type_value, bool adaptive_mode_MDS_value, int flag) {

    erasure_type = erasure_type_value;

    temp_seq=-1;
    last_recieved_seq=-1;

    estimator = new siphon::Parameter_Estimator(T_TOT, adaptive_mode_MDS_value);
    background_estimator = new siphon::Parameter_Estimator(T_TOT, adaptive_mode_MDS_value);
    flag_for_estimation_cycle=1;
    if (erasure_type != 5) {
        fec_decoder = new siphon::Variable_Rate_FEC_Decoder(max_payload, OUTPUTDATAFILE, ERASURE_RECORDER);
//        fec_decoderTemp = new siphon::Variable_Rate_FEC_Decoder(max_payload, OUTPUTDATAFILE, ERASURE_RECORDER);
    }
    else
        fec_decoder = new siphon::Variable_Rate_FEC_Decoder(max_payload, OUTPUTDATAFILE, 0);   //do not record if the
    // existing erasure file is used (when erasure_type equals 5)

    fec_message = new FEC_Message();
    fec_messageTemp = new FEC_Message();
    codeword = (unsigned char *) malloc(
            sizeof(unsigned char) * (max_payload + 12) * 4 * T_TOT); //4-byte sequence number + 4-byte

    connection_manager = new ConnectionManager(Tx, Rx,flag);

    receiver_index=0;
    latest_seq=0;

}

Application_Layer_Receiver::~Application_Layer_Receiver() {
    free(codeword);
    delete fec_message;
    delete fec_messageTemp;
    delete estimator;
    delete background_estimator;
    delete fec_decoder;
    delete connection_manager;
}

void Application_Layer_Receiver::set_receiver_index(int index){
    receiver_index=index;
    fec_decoder->receiver_index=index;
}

int Application_Layer_Receiver::receive_message_and_symbol_wise_encode(unsigned char *udp_parameters,unsigned char *udp_parameters2, unsigned char *udp_codeword,
                                                                       int *udp_codeword_size, siphon::Erasure_Simulator *erasure_simulator,
                                                                       int k2, int n2,int *codeword_size_final,int *k2_out,int *n2_out,
                                                                       bool is_erasure) {
    //int temp_size;

    //receive(&temp_size, codeword, udp_codeword, udp_codeword_size);
    if (udp_parameters == nullptr)
        connection_manager->UDPreceive(&temp_size, codeword);
    else{
        temp_size = *udp_codeword_size;
        memcpy(codeword, udp_codeword, size_t(temp_size));
    }
    if (temp_seq==-1){
        T_s_r=int(codeword[4]);
        N_s_r=int(codeword[6]);
    }
    temp_seq = int(codeword[3]) + 256 * int(codeword[2]) + 256 * 256 * int(codeword[1]) + 256 * 256 * 256 * int
            (codeword[0]);

    if (is_erasure){
        temp_seq = last_recieved_seq + 1;
        fec_message->set_parameters(temp_seq, 0, 0, 0, 0, nullptr);
        int temp_latest_seq=fec_decoder->latest_seq;
//        fec_decoder->decode_erased_packet_for_constnat_trans(fec_message);
        fec_decoder->receive_message_and_symbol_wise_encode_erased_packet_for_constnat_trans(fec_message,0,0,0,0,0,codeword_size_final);
        fec_decoder->latest_seq=temp_latest_seq+1;
        last_recieved_seq = temp_seq;
        return temp_seq;
    }



    if (erasure_type != 0) {
        if ((temp_seq < NUMBER_OF_ITERATIONS + T_TOT) &&
            ((erasure_simulator->is_erasure(temp_seq) == true))) {//artificial erasure
            return -1;
        }
    }

    // Perform estimation
    fec_message->set_parameters(temp_seq, int(codeword[4]), int(codeword[5]), int(codeword[6]),
                                temp_size - 16, codeword + 16);
    fec_message->counter_for_start_and_end = int(codeword[7]);

    estimator->estimate(fec_message);
    background_estimator->estimate(fec_message);

    if ((temp_seq + 1) >flag_for_estimation_cycle*ESTIMATION_WINDOW_SIZE/ESTIMATION_WINDOW_SIZE_REDUCTION_FACTOR){
        cout << "Old (T,B,N) at relay receiver=" << "(" << estimator->T << "," << estimator->B_current << "," << estimator->N_current << ")" << endl;
        free(estimator);
        estimator=background_estimator;
        for (int i = 0; i < 12; i++)
            estimator->erasure[i] = background_estimator->erasure[i];
        cout << "New (T,B,N) at relay receiver=" << "(" << estimator->T << "," << estimator->B_current << "," << estimator->N_current << ")" << endl;
        background_estimator = new siphon::Parameter_Estimator(T_TOT, false);
        flag_for_estimation_cycle=flag_for_estimation_cycle+1;
    }

    int T_value=int(codeword[4]);
    int B_value=int(codeword[5]);
    int N_value=int(codeword[6]);

//    int T_value_old = int(codeword[12]);
//    int N_value_old = int(codeword[13]);

    int T_value_2=int(codeword[8]);
    int B_value_2=int(codeword[9]);
    int N_value_2=int(codeword[10]);

//    int T_value_2_old = int(codeword[14]);
//    int N_value_2_old = int(codeword[15]);



    int k=T_value-N_value+1;
    int n=T_value+1;
//    int k_old=T_value_old-N_value_old+1;
//    int n_old=T_value_old+1;
//    int k2_2=T_value_2-N_value_2+1;
//    int n2_2=T_value_2+1;
//    int k2_2_old=T_value_2_old-N_value_2_old+1;
//    int n2_2_old=T_value_2_old+1;
//    int k2_new;
//    int n2_new;

    if (T_s_r!=T_value || N_s_r!=N_value){//any change in T1,N1->change T2,N2
        T_s_r=T_value;
        N_s_r=N_value;
        k2_new = T_value_2 - N_value_2 + 1;
        n2_new = T_value_2 + 1;
    }else{
        k2_new=k2;
        n2_new=n2;
    }


//    if (T_value_2==0){
//        k2_new=k2;
//        n2_new=n2;
//    }else {
//        k2_new = T_value_2 - N_value_2 + 1;
//        n2_new = T_value_2 + 1;
//    }

    *k2_out=k2_new;
    *n2_out=n2_new;

    fec_decoder->receive_message_and_symbol_wise_encode(fec_message,n,k,n2_new,k2_new,temp_size,codeword_size_final);
//    if (RELAYING_TYPE==2)
//        fec_decoder->receive_message_and_symbol_wise_encode(fec_message,n,k,n2_new,k2_new,temp_size,codeword_size_final);
//    else if (RELAYING_TYPE==3)
//        fec_decoder->receive_message_and_state_dependent_symbol_wise_encode(fec_message,n,k,n2_new,k2_new,temp_size,codeword_size_final);


//    if (temp_seq>10){
//        estimator->B_current=0;
//        estimator->N_current=0;
//    }

    unsigned char temp_parameters2[12];
    //recommended T, B and N
    temp_parameters2[0] = (unsigned char) (estimator->T);
    temp_parameters2[1] = (unsigned char) (estimator->B_current);
    temp_parameters2[2] = (unsigned char) (estimator->N_current);
    //acknowledgment of T, B and N
    temp_parameters2[3] = (unsigned char) (fec_message->T);
    temp_parameters2[4] = (unsigned char) (fec_message->B);
    temp_parameters2[5] = (unsigned char) (fec_message->N);
    for (int i=6;i<12;i++)
        temp_parameters2[i]=udp_parameters2[i-6];
    if (udp_parameters == nullptr) {
        connection_manager->UDPsendResponse(temp_parameters2, 12);
        if (DEBUG_COMM==1) {
            cout << "Sending 12 bytes from relay to source" << endl;
            printMatrix(temp_parameters2, 1, 12);
        }
    }else{
        // Need to add...
        for (int i=0;i<12;i++)
            udp_parameters[i]=temp_parameters2[i];
        if (DEBUG_COMM==1) {
            cout << "Sending 12 bytes from relay to source" << endl;
            printMatrix(udp_parameters, 1, 12);
        }
    }
    last_recieved_seq=temp_seq;
    return temp_seq;
}

int Application_Layer_Receiver::receive_message_and_symbol_wise_decode(unsigned char *udp_parameters, unsigned char *udp_codeword,
                                                                       int *udp_codeword_size, siphon::Erasure_Simulator
                                                                       *erasure_simulator,int num_of_packets_to_ignore_est) {
    int temp_size;

//    int k=T_INITIAL-N_INITIAL+1; //Elad - to change
//    int n=T_INITIAL+1; //Elad - to change

    //receive(&temp_size, codeword, udp_codeword, udp_codeword_size);
    if (udp_parameters == nullptr)
        connection_manager->UDPreceive(&temp_size, codeword);
    else{
        temp_size = *udp_codeword_size;
        memcpy(codeword, udp_codeword, size_t(temp_size));
    }
    //push new codeword into array if there was no erasure
    int temp_seq = int(codeword[3]) + 256 * int(codeword[2]) + 256 * 256 * int(codeword[1]) + 256 * 256 * 256 * int
            (codeword[0]);

    if (erasure_type != 0) {
        if ((temp_seq < NUMBER_OF_ITERATIONS + T_TOT) &&
            ((erasure_simulator->is_erasure(temp_seq) == true))) {//artificial erasure
            return -1;
        }
    }



    // Perform estimation
    fec_message->set_parameters(temp_seq, int(codeword[4]), int(codeword[5]), int(codeword[6]),
                                temp_size - 8, codeword + 8);
    fec_message->counter_for_start_and_end = int(codeword[7]);

    //disable estimation at the receiver.
//    if (num_of_packets_to_ignore_est>0){
//        for (int kk=0;kk<num_of_packets_to_ignore_est;kk++) {
//            estimator->previous_win_end=temp_seq-num_of_packets_to_ignore_est+kk;
//            background_estimator->previous_win_end=temp_seq-num_of_packets_to_ignore_est+kk;
//        }
//    }

    estimator->estimate(fec_message);
    background_estimator->estimate(fec_message);


    if ((temp_seq + 1) >flag_for_estimation_cycle*ESTIMATION_WINDOW_SIZE/ESTIMATION_WINDOW_SIZE_REDUCTION_FACTOR){
        cout << "Old (T,B,N) at destination=" << "(" << estimator->T << "," << estimator->B_current << "," << estimator->N_current << ")" << endl;
        free(estimator);
        estimator=background_estimator;
        for (int i = 0; i < 12; i++)
            estimator->erasure[i] = background_estimator->erasure[i];
        cout << "New (T,B,N) at destination=" << "(" << estimator->T << "," << estimator->B_current << "," << estimator->N_current << ")" << endl;
        background_estimator = new siphon::Parameter_Estimator(T_TOT, false);
        flag_for_estimation_cycle=flag_for_estimation_cycle+1;
    }

    int T_value=int(codeword[4]);
    int B_value=int(codeword[5]);
    int N_value=int(codeword[6]);

//    int T_value2=int(codeword[8]);
//    int B_value2=int(codeword[9]);
//    int N_value2=int(codeword[10]);

    int k=T_value-N_value+1;
    int n=T_value+1;

    fec_decoder->receive_message_and_symbol_wise_decode(fec_message,n,k,temp_size,erasure_simulator);

//    if (RELAYING_TYPE==2)
//        fec_decoder->receive_message_and_symbol_wise_decode(fec_message,n,k,temp_size,erasure_simulator);
//    else if (RELAYING_TYPE==3)
//        fec_decoder->receive_message_and_state_dependent_symbol_wise_decode(fec_message,n,k,temp_size,erasure_simulator);

//    if (temp_seq>10){
//        estimator->B_current=0;
//        estimator->N_current=0;
//    }

    unsigned char temp_parameters[6];
    //recommended T, B and N
    temp_parameters[0] = (unsigned char) (estimator->T);
    temp_parameters[1] = (unsigned char) (estimator->B_current);
    temp_parameters[2] = (unsigned char) (estimator->N_current);
    //acknowledgment of T, B and N
    temp_parameters[3] = (unsigned char) (fec_message->T);
    temp_parameters[4] = (unsigned char) (fec_message->B);
    temp_parameters[5] = (unsigned char) (fec_message->N);

    if (udp_parameters == nullptr) {
        connection_manager->UDPsendResponse(temp_parameters, 6);
        if (DEBUG_COMM==1) {
            cout << "Sending 6 bytes from destination" << endl;
            printMatrix(temp_parameters, 1, 6);
        }
    }else {
        //recommended T, B and N
        udp_parameters[0] = (unsigned char) (estimator->T);
        udp_parameters[1] = (unsigned char) (estimator->B_current);
        udp_parameters[2] = (unsigned char) (estimator->N_current);
        //acknowledgment of T, B and N
        udp_parameters[3] = (unsigned char) (fec_message->T);
        udp_parameters[4] = (unsigned char) (fec_message->B);
        udp_parameters[5] = (unsigned char) (fec_message->N);
        if (DEBUG_COMM==1) {
            cout << "Sending 6 bytes from destination" << endl;
            printMatrix(udp_parameters, 1, 6);
        }
    }

    return 0;


}

int Application_Layer_Receiver::receive_message_and_decode(unsigned char *udp_parameters,unsigned char *udp_parameters2, unsigned char *udp_codeword,
                                                           int *udp_codeword_size, siphon::Erasure_Simulator
                                                           *erasure_simulator,bool is_erasure) {

//receive udp_codeword using a blocking function and obtain udp_codeword and udp_codeword_size
    int temp_size;

    //receive(&temp_size, codeword, udp_codeword, udp_codeword_size);
    if (udp_parameters == nullptr)
        connection_manager->UDPreceive(&temp_size, codeword);
    else{
        temp_size = *udp_codeword_size;
        memcpy(codeword, udp_codeword, size_t(temp_size));
    }

    int temp_seq = int(codeword[3]) + 256 * int(codeword[2]) + 256 * 256 * int(codeword[1]) + 256 * 256 * 256 * int
            (codeword[0]);
    int second_seg_seq;

    if (is_erasure){
        temp_seq = last_recieved_seq + 1;
        fec_message->set_parameters(temp_seq, 0, 0, 0, 0, nullptr);
        int temp_latest_seq=fec_decoder->latest_seq;
        fec_decoder->decode_erased_packet_for_constnat_trans(fec_message);
        fec_decoder->latest_seq=temp_latest_seq;
        last_recieved_seq = temp_seq;
        return temp_seq;
    }


    if (erasure_type != 0) {
        int temp_seq_test=temp_seq;
        if (RELAYING_TYPE==1 && receiver_index==1)
            temp_seq_test=temp_seq_test+T_INITIAL;
        if ((temp_seq_test < NUMBER_OF_ITERATIONS + T_INITIAL) &&
            ((erasure_simulator->is_erasure(temp_seq_test) == true))) {//artificial erasure2
            return -1;
        }
    }

    if (RELAYING_TYPE==0 || receiver_index==0) {

        fec_message->set_parameters(temp_seq, int(codeword[4]), int(codeword[5]), int(codeword[6]),
                                    temp_size - 8, codeword + 8);
        fec_message->counter_for_start_and_end = int(codeword[7]);

        estimator->estimate(fec_message);
        background_estimator->estimate(fec_message);

    }else if (RELAYING_TYPE==1){
        second_seg_seq= int(codeword[7]) + 256 * int(codeword[6]) + 256 * 256 * int(codeword[5]) + 256 * 256 * 256 * int
                (codeword[4]);
        fec_message->set_parameters(second_seg_seq, int(codeword[8]), int(codeword[9]), int(codeword[10]),
                                    temp_size - 12, codeword + 12);
        fec_message->seq_number2=second_seg_seq;
        fec_message->counter_for_start_and_end = int(codeword[11]);

        estimator->estimate(fec_message);
        background_estimator->estimate(fec_message);

        fec_message->set_parameters(temp_seq, int(codeword[8]), int(codeword[9]), int(codeword[10]),
                                    temp_size - 12, codeword + 12);
    }

    if ((temp_seq + 1) >flag_for_estimation_cycle*ESTIMATION_WINDOW_SIZE/ESTIMATION_WINDOW_SIZE_REDUCTION_FACTOR){
        if (RELAYING_TYPE==0 || receiver_index==0)
            cout << "Old (T,B,N) at relay receiver=" << "(" << estimator->T << "," << estimator->B_current << "," << estimator->N_current << ")" << endl;
        else
            cout << "Old (T,B,N) at destination=" << "(" << estimator->T << "," << estimator->B_current << "," << estimator->N_current << ")" << endl;
        free(estimator);
        estimator=background_estimator;
        for (int i = 0; i < 12; i++)
            estimator->erasure[i] = background_estimator->erasure[i];
        if (RELAYING_TYPE==0 || receiver_index==0)
            cout << "New (T,B,N) at relay receiver=" << "(" << estimator->T << "," << estimator->B_current << "," << estimator->N_current << ")" << endl;
        else
            cout << "New (T,B,N) at destination=" << "(" << estimator->T << "," << estimator->B_current << "," << estimator->N_current << ")" << endl;
        background_estimator = new siphon::Parameter_Estimator(T_TOT, false);
        flag_for_estimation_cycle=flag_for_estimation_cycle+1;
    }


    int received_seq = fec_message->seq_number;

    fec_decoder->decode(fec_message);
    //std::memcpy(fec_decoderTemp->recovered_message, fec_decoder->recovered_message, sizeof(FEC_Message));
    unsigned char temp_parameters[6];
    //recommended T, B and N
    temp_parameters[0] = (unsigned char) (estimator->T);
    temp_parameters[1] = (unsigned char) (estimator->B_current);
    temp_parameters[2] = (unsigned char) (estimator->N_current);
    //acknowledgment of T, B and N
    temp_parameters[3] = (unsigned char) (fec_message->T);
    temp_parameters[4] = (unsigned char) (fec_message->B);
    temp_parameters[5] = (unsigned char) (fec_message->N);
    if (RELAYING_TYPE>0 && receiver_index==0){
        unsigned char temp_parameters2[12];
        //recommended T, B and N
        temp_parameters2[0] = (unsigned char) (estimator->T);
        temp_parameters2[1] = (unsigned char) (estimator->B_current);
        temp_parameters2[2] = (unsigned char) (estimator->N_current);
        //acknowledgment of T, B and N
        temp_parameters2[3] = (unsigned char) (fec_message->T);
        temp_parameters2[4] = (unsigned char) (fec_message->B);
        temp_parameters2[5] = (unsigned char) (fec_message->N);
        for (int i=6;i<12;i++)
            temp_parameters2[i]=udp_parameters2[i-6];
        if (udp_parameters == nullptr) {
            connection_manager->UDPsendResponse(temp_parameters2, 12);
            if (DEBUG_COMM==1) {
                cout << "Sending 12 bytes from relay" << endl;
                printMatrix(temp_parameters2, 1, 12);
            }
        }else{
            // Need to add...
            for (int i=0;i<12;i++)
                udp_parameters[i]=temp_parameters2[i];
            if (DEBUG_COMM==1) {
                cout << "Sending 12 bytes from relay" << endl;
                printMatrix(udp_parameters, 1, 12);
            }
        }

    } else {
        if (udp_parameters == nullptr) {
            connection_manager->UDPsendResponse(temp_parameters, 6);
            if (DEBUG_COMM==1) {
                cout << "Sending 6 bytes from destination" << endl;
                printMatrix(temp_parameters, 1, 6);
            }
        }else {
            //recommended T, B and N
            udp_parameters[0] = (unsigned char) (estimator->T);
            udp_parameters[1] = (unsigned char) (estimator->B_current);
            udp_parameters[2] = (unsigned char) (estimator->N_current);
            //acknowledgment of T, B and N
            udp_parameters[3] = (unsigned char) (fec_message->T);
            udp_parameters[4] = (unsigned char) (fec_message->B);
            udp_parameters[5] = (unsigned char) (fec_message->N);
            if (DEBUG_COMM==1) {
                cout << "Sending 6 bytes from destination" << endl;
                printMatrix(udp_parameters, 1, 6);
            }
        }
    }
    last_recieved_seq=received_seq;
    return received_seq;
}

//void Application_Layer_Receiver::get_current_packet(unsigned char *received_data,int T,int i,int seq_number,int curr_seq_num){
//
//    if (i>=T && fec_message->buffer==NULL){
//        // Elad - this is a case the relay fails to decode the packet hence it sends all-zeros packet
//        std::cout << "Elad: Erased packet"<< std::endl;
//        memset(received_data,'\000',300);
////        for (int t=0;t<300;t++) {
////            received_data[t] = '\000';
////        }
//        DEBUG_MSG("\033[1;34m" << "Generated dummy message at relay #" << curr_seq_num << ": " << "\033[0m");
//        printMatrix(received_data, 1, 300);
//    }
//    else if  (i>=T && seq_number==0){
//        std::memcpy(received_data,fec_messageTemp->buffer,fec_messageTemp->size*sizeof(unsigned char));
//        DEBUG_MSG("\033[1;34m" << "Recovered message at relay #" << fec_messageTemp->seq_number << ": " << "\033[0m");
//        printMatrix(received_data, 1, fec_messageTemp->size);
//    }
//    else if (i>=T){
//        std::memcpy(received_data,fec_message->buffer,fec_message->size*sizeof(unsigned char));
//        DEBUG_MSG("\033[1;34m" << "Recovered message at relay #" << fec_message->seq_number << ": " << "\033[0m");
//        printMatrix(received_data, 1, fec_message->size);
//    }
//}