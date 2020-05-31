/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   Variable_Rate_FEC_Decoder.cpp
 * Author: silas
 *
 * Created on July 12, 2018, 11:12 AM
 */

#include <Decoder_Symbol_Wise.h>
#include <Payload_Simulator.h>
#include "Variable_Rate_FEC_Decoder.h"
#include "codingOperations.h"
#include "basicOperations.h"

using std::ios;

namespace siphon {

    Variable_Rate_FEC_Decoder::Variable_Rate_FEC_Decoder(int max_payload_value, string file_name_input, int
    erasure_recorder_value) {

        erasure_recorder = erasure_recorder_value;

        seq_start = -1;
        seq_start_double_coding = -1;
        seq_end_double_coding = -1;

        receiver_index = 0;

        latest_seq = -1;
        latest_seq_2 = 0; // ELAD to change (handle first packet erased)

        memory_object = new Memory_Allocator(300);

        file_name = file_name_input;
        max_payload = max_payload_value;
        if (file_name != "")
            file_write_decoder.open(file_name, ios::out | ios::binary); //record recovered data

        if (erasure_recorder == 1) {
            file_erasures_recorded = "erasure.bin";
            file_write_erasures.open(file_erasures_recorded, ios::out | ios::binary); //record erasures
        }

        decoder_current = NULL;
        decoder_old = NULL;

        double_coding_flag = 0;

        recovered_message = new FEC_Message();
        recovered_message_vector=(FEC_Message **)malloc(sizeof(FEC_Message*) * T_TOT);
        for (int i=0;i<T_TOT;i++)
            recovered_message_vector[i]=new FEC_Message();
        message_old_encoder = new FEC_Message();

        counter_ = 0;
        loss_counter_ = 0;
        UDP_loss_counter_ = 0;

        final_loss_counter_ = 0;
        final_UDP_loss_counter_ = 0;

        counter_loss_of_char=0;
        final_counter_loss_of_char=0;
        final_counter_loss_of_char_elad=0;

        loss_counter_two_seg_=0;
        final_loss_counter_two_seg_=0;

        display_final_loss_rate_flag = true;

        erasure_length_cap = 1001;

        erasure_counter = (int *) malloc(sizeof(int) * (erasure_length_cap + 1));
        for (int i = 0; i < erasure_length_cap + 1; i++)
            erasure_counter[i] = 0;

        erasure_counter_total = (int *) malloc(sizeof(int) * (erasure_length_cap + 1));
        for (int i = 0; i < erasure_length_cap + 1; i++)
            erasure_counter_total[i] = 0;

        report_window_size = ESTIMATION_WINDOW_SIZE;

        total_session_counter = 0;

        low_fidelity_session_counter_FEC = 0;
        disruption_session_counter_FEC = 0;
        low_fidelity_session_counter_UDP = 0;
        disruption_session_counter_UDP = 0;

        codeword_vector = (unsigned char **) malloc(sizeof(unsigned char *) * (T_TOT+1));
        codeword_new_vector=(unsigned char **) malloc(sizeof(unsigned char *) * (T_TOT+1));
        codeword_vector_store_in_burst=(unsigned char **) malloc(sizeof(unsigned char *) * (T_TOT+1));

//        codeword_vector_new_decoder = (unsigned char **) malloc(sizeof(unsigned char *) * (T_TOT+1));
//        codeword_new_vector_new_decoder = (unsigned char **) malloc(sizeof(unsigned char *) * (T_TOT+1));

        temp_erasure_vector=(bool *) malloc(sizeof(bool)*T_TOT);
        for (int i=0;i<T_TOT+1;i++){
            codeword_vector[i]=(unsigned char *) malloc(
                    sizeof(unsigned char) * (max_payload + 12) * 4 * T_TOT); //4-byte sequence number + 4-byte
//            codeword_vector_new_decoder[i]=(unsigned char *) malloc(
//                    sizeof(unsigned char) * (max_payload + 12) * 4 * T_TOT); //4-byte sequence number + 4-byte
            memset(codeword_vector[i],'\000',(max_payload + 12) * 4 * T_TOT);
            codeword_new_vector[i]=(unsigned char *) malloc(sizeof(unsigned char) * (max_payload + 12) * 4 * T_TOT); //4-byte sequence number + 4-byte
//            codeword_new_vector_new_decoder[i]=(unsigned char *) malloc(sizeof(unsigned char) * (max_payload + 12) * 4 * T_TOT); //4-byte sequence number + 4-byte
            codeword_vector_store_in_burst[i]=(unsigned char *) malloc(sizeof(unsigned char) * (max_payload + 12) * 4 * T_TOT); //4-byte sequence number + 4-byte
            //memset(codeword_new_vector[i],'\000',(max_payload + 12) * 4 * T_INITIAL);
            temp_erasure_vector[i]=0;
        }

        decoder_Symbol_Wise=new Decoder_Symbol_Wise(max_payload_value);
        decoder_Symbol_Backup=new Decoder_Symbol_Wise(max_payload_value);
        for (int i=0;i<50;i++)
            trans_vec[i]=false;

        payload_simulator = new Payload_Simulator(NUMBER_OF_ITERATIONS, max_payload, 0,SOURCE_PCM_FILE);
        raw_data = (unsigned char *) malloc(sizeof(unsigned char) * max_payload);
    }

    Variable_Rate_FEC_Decoder::~Variable_Rate_FEC_Decoder() {

        if ((file_name != "") && file_write_decoder.is_open())
            file_write_decoder.close();

        if (erasure_recorder == 1)
            file_write_erasures.close();

        delete recovered_message;

        if (decoder_current != NULL)
            delete decoder_current;

        if (decoder_old != NULL)
            delete decoder_old;

        delete memory_object;

//        free(erasure_counter);
//        free(erasure_counter_total);
        delete erasure_counter;
        delete erasure_counter_total;

        for (int i=0;i<T_TOT;i++) {
            free(codeword_vector[i]);
            free(codeword_new_vector[i]);
            free(codeword_vector_store_in_burst[i]);
        }
        free(encoder);
        free(raw_data);
        delete payload_simulator;

    }

    void Variable_Rate_FEC_Decoder::receive_message_and_symbol_wise_encode(FEC_Message *message,int n,int k,int n2,
                                                                           int k2,int temp_size,int *codeword_size_final) {
        if (seq_start == -1) { //initialize the decoder upon receiving the first packet
            decoder_Symbol_Wise->decoder_current=new Decoder(n-1, n-k, n-k, max_payload);
            decoder_Symbol_Wise->encoder_current=new Encoder(n2-1, n2-k2, n2-k2, max_payload);
//            initialize_decoder(message);
            //encoder = new Encoder(n2 - 1, n2 - k2, n2 - k2, max_payload);
            T = message->T;
            B = message->B;
            N = message->N;
            seq_start=0;
            latest_seq = seq_start;
            k2_old=k2;
            n2_old=n2;
            k2_last_used=k2;
            n2_last_used=n2;
        }
        int received_seq = message->seq_number;

        if (received_seq < latest_seq) { //if an out-of-order sequence is received, just discard it
            //cout << "\033[1;31mOut-of-sequence packet: #" << received_seq << "\033[0m" << endl;
            return;
        }

        if ((T != message->T) || (B != message->B) || (N != message->N)) { //record when the double_coding begins
            seq_start_double_coding = received_seq - message->counter_for_start_and_end;
            cout<<"Start double coding at the relay"<<endl;
        }

        // first decode the old message using the old decoder

        unsigned char *codeword_received_with_header = message->buffer;
//        unsigned char *codeword_received = codeword_received_with_header + 2;
        unsigned char *codeword_received = codeword_received_with_header+2;
        int size_received = int(codeword_received_with_header[0]) * 256 + int(codeword_received_with_header[1]);
        unsigned char *codeword_received_transition = codeword_received + size_received;
//        int size_received_transition = message->size - 2 - size_received;
        int size_received_transition = message->size - size_received;
        char codeword_final[30000];
        bool flag=false;

//        decoder_Symbol_Wise->receive_message_and_symbol_wise_encode(message->buffer,received_seq,latest_seq,n,k,k2,
//                n2,&UDP_loss_counter_,&final_UDP_loss_counter_,
//                &loss_counter_,&final_loss_counter_,temp_size,
//                decoder_current->decoder->getG(),encoder->getG());

        int codeword_r_d_size_old= (ceil(float(max_payload + 2) / k2_old)+1) * n2_old;
        int codeword_r_d_size_current= (ceil(float(max_payload + 2) / k2)+1) * n2;

        flag_for_burst=0;
        if (received_seq>latest_seq+n2_old-1) {
            flag_for_burst=n2_old;
            //TODO handle a burst...
            // in case of a burst longer than n fill codeword_vector with zeros and store codeword_new_vector in codeword_vector_store_in_burst
            for (int seq = latest_seq; seq < latest_seq + n2_old ; seq++) {
                UDP_loss_counter_++;
                final_UDP_loss_counter_++;
                DEBUG_MSG("\033[1;34m" << "Burst: packet dropped in (s,r) #" << seq << ": " << "\033[0m");
                if ((seq > seq_end_double_coding) && (double_coding_flag == 1)) {
                    double_coding_flag = 0;
//                    decoder_Symbol_Wise = decoder_Symbol_Wise_new;
                    decoder_Symbol_Backup->copy_elements(decoder_Symbol_Wise,true);
                    decoder_Symbol_Backup->n=n2_old;
                    decoder_Symbol_Wise->copy_elements(decoder_Symbol_Wise_new,true);
                    n2_old=n2;
                    k2_old=k2;
                    cout<<"Stopped double coding at the relay"<<endl;
                }
                trans_vec[seq-latest_seq]=double_coding_flag;
                if (seq == seq_start_double_coding) {
                    seq_end_double_coding = seq_start_double_coding + T_TOT+1 - 1;//TODO need to make sure it use the right T...
                    if (decoder_Symbol_Wise_new != NULL)
                        delete decoder_Symbol_Wise_new;
                    T=message->T;
                    B=message->B;
                    N=message->N;

                    decoder_Symbol_Wise_new=new Decoder_Symbol_Wise(max_payload);
                    decoder_Symbol_Wise_new->decoder_current=new Decoder(T,B,N, max_payload);
                    decoder_Symbol_Wise_new->encoder_current=new Encoder(n2-1,n2-k2,n2-k2, max_payload);
                    double_coding_flag = 1;
                }
                if (double_coding_flag == 0) {
                    decoder_Symbol_Wise->rotate_pointers_and_insert_zero_word(n,n2,size_received,codeword_r_d_size_current,true);
//                    DEBUG_MSG("\033[1;34m" << "Packet dropped in (s,r) #" << seq << ": " << "\033[0m");
                    decoder_Symbol_Wise->symbol_wise_encode_1(k,n,k2,n2,&flag);
                    if (flag==true){
                        loss_counter_++;
                        final_loss_counter_++;
                    }
                    *codeword_size_final = codeword_r_d_size_current+2;  //2 extra bytes at the very beginning to indicate codeword_r_d_size_current
                    int codeword_size_final_temp = codeword_r_d_size_current+2;
                    codeword_final[1] = (unsigned char) (codeword_r_d_size_current);
                    codeword_final[0] = (unsigned char) ((codeword_r_d_size_current) / 256);
                    memcpy(codeword_final + 2, decoder_Symbol_Wise->codeword_new_vector[n2-1], size_t(codeword_r_d_size_current));
                    memcpy(decoder_Symbol_Wise->codeword_vector_to_transmit[n2-1],codeword_final,size_t(codeword_size_final_temp));
                    decoder_Symbol_Wise->k2_vector[n2-1]=k2;
                    decoder_Symbol_Wise->n2_vector[n2-1]=n2;
                    display_udp_statistics(seq);
                    memcpy(decoder_Symbol_Wise->codeword_vector_store_in_burst[n2 - 1],
                           decoder_Symbol_Wise->codeword_vector_to_transmit[n2 - 1], size_t(codeword_size_final_temp));
                    decoder_Symbol_Wise->codeword_size_vector[n2-1]=codeword_size_final_temp;
                } else{
                    //TODO check codeword size in case of transition !!!
                    decoder_Symbol_Wise->rotate_pointers_and_insert_zero_word(n_old,n2_old,size_received_transition,codeword_r_d_size_old,true);
                    decoder_Symbol_Wise_new->rotate_pointers_and_insert_zero_word(n,n2,size_received,codeword_r_d_size_current,true);
                    decoder_Symbol_Wise->symbol_wise_encode_1(k_old,n_old,k2_old,n2_old,&flag);
                    if (flag==true){
                        loss_counter_++;
                        final_loss_counter_++;
                    }
                    decoder_Symbol_Wise_new->symbol_wise_encode_1(k,n,k2,n2,&flag);
//                    DEBUG_MSG("\033[1;34m" << "Packet dropped in (s,r) #" << seq << ": " << "\033[0m");
                    int codeword_size_final_temp = codeword_r_d_size_current + codeword_r_d_size_old + 2;  //2 extra bytes at the very beginning to indicate codeword_r_d_size_current
                    *codeword_size_final = codeword_size_final_temp;

                    int temp_remainder;

                    memset(codeword_final, 0, size_t(codeword_size_final_temp));

                    temp_remainder = codeword_r_d_size_current % 256;
                    codeword_final[1] = (unsigned char) (temp_remainder);
                    codeword_final[0] = (unsigned char) ((codeword_r_d_size_current - temp_remainder) / 256);

                    memcpy(codeword_final + 2, decoder_Symbol_Wise_new->codeword_new_vector[n2-1], size_t(codeword_r_d_size_current));
                    memcpy(codeword_final + codeword_r_d_size_current + 2, decoder_Symbol_Wise->codeword_new_vector[n2_old - 1], size_t(codeword_r_d_size_old));
                    memcpy(decoder_Symbol_Wise->codeword_vector_to_transmit[n2_old-1],codeword_final,size_t(codeword_size_final_temp));
                    decoder_Symbol_Wise->k2_vector[n2_old-1]=k2_old;
                    decoder_Symbol_Wise->n2_vector[n2_old-1]=n2_old;
                    memcpy(decoder_Symbol_Wise->codeword_vector_store_in_burst[n2_old - 1], decoder_Symbol_Wise->codeword_vector_to_transmit[n2 - 1], size_t(codeword_size_final_temp));
                    decoder_Symbol_Wise->codeword_size_vector[n2_old-1]=codeword_size_final_temp;
                    display_udp_statistics(seq);
                }

//                display_udp_statistics(seq);
            }
//            for (int i=0;i<n2_old-1;i++)// zero the output (after storing)
//                memset(decoder_Symbol_Wise->codeword_new_vector[i], '\000', 10000);//ELAD - 300=max_payloa
            // go over the rest of the missing UDP packets
            for (int seq = latest_seq + n2_old; seq < received_seq ; seq++) {
                UDP_loss_counter_++;
                final_UDP_loss_counter_++;
                if ((seq > seq_end_double_coding) && (double_coding_flag == 1)) {
                    double_coding_flag = 0;
//                    decoder_Symbol_Wise = decoder_Symbol_Wise_new;
                    decoder_Symbol_Backup->copy_elements(decoder_Symbol_Wise,true);
                    decoder_Symbol_Backup->n=n2_old;
                    decoder_Symbol_Wise->copy_elements(decoder_Symbol_Wise_new,true);
                    n2_old=n2;
                    k2_old=k2;
                    cout<<"Stopped double coding at the relay"<<endl;
                }
                trans_vec[seq-latest_seq]=double_coding_flag;
                DEBUG_MSG("\033[1;34m" << "Burst: packet dropped in (s,r) #" << seq << ": " << "\033[0m");
                if (double_coding_flag==0){
                    decoder_Symbol_Wise->rotate_pointers_and_insert_zero_word(n,n2,size_received,codeword_r_d_size_current,false);
                    decoder_Symbol_Wise->symbol_wise_encode_1(k,n,k2,n2,&flag);
                }else{
                    decoder_Symbol_Wise->rotate_pointers_and_insert_zero_word(n_old,n2_old,size_received_transition,codeword_r_d_size_old,false);
                    decoder_Symbol_Wise_new->rotate_pointers_and_insert_zero_word(n,n2,size_received,codeword_r_d_size_current,false);
                    decoder_Symbol_Wise->symbol_wise_encode_1(k_old,n_old,k2_old,n2_old,&flag);
                    decoder_Symbol_Wise_new->symbol_wise_encode_1(k,n,k2,n2,&flag);
                }
                display_udp_statistics(seq);
            }
        }else { // NOT IN BURST !!!
            for (int seq = latest_seq; seq < received_seq; seq++) {// need to handle bursts longer than n
                UDP_loss_counter_++;
                final_UDP_loss_counter_++;
                if ((seq > seq_end_double_coding) && (double_coding_flag == 1)) {
                    double_coding_flag = 0;
//                    decoder_Symbol_Wise = decoder_Symbol_Wise_new;
                    decoder_Symbol_Backup->copy_elements(decoder_Symbol_Wise,true);
                    decoder_Symbol_Backup->n=n2_old;
                    decoder_Symbol_Wise->copy_elements(decoder_Symbol_Wise_new,true);
                    n2_old=n2_last_used;
                    k2_old=k2_last_used;
                    cout<<"Stopped double coding at the relay"<<endl;
                }
                trans_vec[seq-latest_seq]=double_coding_flag;

                if (seq == seq_start_double_coding) {
                    seq_end_double_coding = seq_start_double_coding + T_TOT + 1 - 1;//TODO need to make sure it use the right T...
                    if (decoder_Symbol_Wise_new != NULL)
                        delete decoder_Symbol_Wise_new;
                    k_old=T-N+1;
                    n_old=T+1;
                    T=message->T;
                    B=message->B;
                    N=message->N;

                    decoder_Symbol_Wise_new=new Decoder_Symbol_Wise(max_payload);
                    decoder_Symbol_Wise_new->decoder_current=new Decoder(T,B,N, max_payload);
                    decoder_Symbol_Wise_new->encoder_current=new Encoder(n2-1,n2-k2,n2-k2, max_payload);
                    double_coding_flag = 1;
                }

                if (double_coding_flag == 0) {
                    decoder_Symbol_Wise->rotate_pointers_and_insert_zero_word(n,n2,size_received,codeword_r_d_size_current,true);
                    DEBUG_MSG("\033[1;34m" << "Packet dropped in (s,r) #" << seq << ": " << "\033[0m");
                    decoder_Symbol_Wise->symbol_wise_encode_1(k,n,k2,n2,&flag);
                    if (flag==true){
                        loss_counter_++;
                        final_loss_counter_++;
                    }
                    *codeword_size_final = codeword_r_d_size_current+2;  //2 extra bytes at the very beginning to indicate codeword_r_d_size_current
                    int codeword_size_final_temp = codeword_r_d_size_current+2;
                    codeword_final[1] = (unsigned char) (codeword_r_d_size_current);
                    codeword_final[0] = (unsigned char) ((codeword_r_d_size_current) / 256);
                    memcpy(codeword_final + 2, decoder_Symbol_Wise->codeword_new_vector[n2-1], size_t(codeword_r_d_size_current));
                    memcpy(decoder_Symbol_Wise->codeword_vector_to_transmit[n2-1],codeword_final,size_t(codeword_size_final_temp));
                    decoder_Symbol_Wise->k2_vector[n2-1]=k2;
                    decoder_Symbol_Wise->n2_vector[n2-1]=n2;
                    decoder_Symbol_Wise->codeword_size_vector[n2-1]=codeword_size_final_temp;
                    display_udp_statistics(seq);
                } else{
                    //TODO check codeword size in case of transition !!!
                    decoder_Symbol_Wise->rotate_pointers_and_insert_zero_word(n_old,n2_old,size_received_transition,codeword_r_d_size_old,true);
                    decoder_Symbol_Wise_new->rotate_pointers_and_insert_zero_word(n,n2,size_received,codeword_r_d_size_current,true);
                    decoder_Symbol_Wise->symbol_wise_encode_1(k_old,n_old,k2_old,n2_old,&flag);
                    if (flag==true){
                        loss_counter_++;
                        final_loss_counter_++;
                    }
                    decoder_Symbol_Wise_new->symbol_wise_encode_1(k,n,k2,n2,&flag);
                    DEBUG_MSG("\033[1;34m" << "Packet dropped in (s,r) #" << seq << ": " << "\033[0m");
                    int codeword_size_final_temp = codeword_r_d_size_current + codeword_r_d_size_old + 2;  //2 extra bytes at the very beginning to indicate codeword_r_d_size_current
                    *codeword_size_final = codeword_size_final_temp;

                    int temp_remainder;

                    memset(codeword_final, 0, size_t(codeword_size_final_temp));

                    temp_remainder = codeword_r_d_size_current % 256;
                    codeword_final[1] = (unsigned char) (temp_remainder);
                    codeword_final[0] = (unsigned char) ((codeword_r_d_size_current - temp_remainder) / 256);

                    memcpy(codeword_final + 2, decoder_Symbol_Wise_new->codeword_new_vector[n2-1], size_t(codeword_r_d_size_current));
                    memcpy(codeword_final + codeword_r_d_size_current + 2, decoder_Symbol_Wise->codeword_new_vector[n2_old - 1], size_t(codeword_r_d_size_old));
                    memcpy(decoder_Symbol_Wise->codeword_vector_to_transmit[n2_old-1],codeword_final,size_t(codeword_size_final_temp));
                    decoder_Symbol_Wise->k2_vector[n2_old-1]=k2_old;
                    decoder_Symbol_Wise->n2_vector[n2_old-1]=n2_old;
                    decoder_Symbol_Wise->codeword_size_vector[n2_old-1]=codeword_size_final_temp;
//                    // prepare decoder_Symbol_Wise_new->codeword_new_vector[n2-1] for sending if needed!!!
//                    memcpy(codeword_final + 2, decoder_Symbol_Wise_new->codeword_new_vector[n2-1], size_t(codeword_r_d_size_current));
//                    memcpy(decoder_Symbol_Wise_new->codeword_new_vector[n2-1],codeword_final,size_t(codeword_r_d_size_current+2));



                    display_udp_statistics(seq);
                }
            }
        }

//        if ((T != message->T) || (B != message->B) || (N != message->N)) { //record when the double_coding begins
//            seq_start_double_coding = received_seq - message->counter_for_start_and_end;
//            cout<<"Start double coding at the relay"<<endl;
//        }

        if ((received_seq > seq_end_double_coding) && (double_coding_flag == 1)) {
            double_coding_flag = 0;
//            decoder_Symbol_Wise = decoder_Symbol_Wise_new;
            decoder_Symbol_Backup->copy_elements(decoder_Symbol_Wise,true);
            decoder_Symbol_Backup->n=n2_old;
            decoder_Symbol_Wise->copy_elements(decoder_Symbol_Wise_new,true);
            n2_old=n2;
            k2_old=k2;
            cout<<"Stopped double coding at the relay"<<endl;
        }
        trans_vec[received_seq-latest_seq]=double_coding_flag;

        if (received_seq == seq_start_double_coding) {
            seq_end_double_coding = seq_start_double_coding + T_TOT + 1 - 1;//TODO need to make sure it use the right T...
            if (decoder_Symbol_Wise_new != NULL)
                delete decoder_Symbol_Wise_new;
            k_old=T-N+1;
            n_old=T+1;
            T=message->T;
            B=message->B;
            N=message->N;

            decoder_Symbol_Wise_new=new Decoder_Symbol_Wise(max_payload);
            decoder_Symbol_Wise_new->decoder_current=new Decoder(T,B,N, max_payload);
            decoder_Symbol_Wise_new->encoder_current=new Encoder(n2-1,n2-k2,n2-k2, max_payload);
            double_coding_flag = 1;
        }
        if (double_coding_flag==0) {
            decoder_Symbol_Wise->push_current_codeword(codeword_received, n, n2, size_received,codeword_r_d_size_current);
            decoder_Symbol_Wise->symbol_wise_encode_1(k, n, k2,n2,&flag);
            if (flag==true){
                loss_counter_++;
                final_loss_counter_++;
            }
            *codeword_size_final = codeword_r_d_size_current+2;  //2 extra bytes at the very beginning to indicate codeword_r_d_size_current

            int codeword_size_final_temp = codeword_r_d_size_current+2;
            codeword_final[1] = (unsigned char) (codeword_r_d_size_current);
            codeword_final[0] = (unsigned char) ((codeword_r_d_size_current) / 256);
            memcpy(codeword_final + 2, decoder_Symbol_Wise->codeword_new_vector[n2-1], size_t(codeword_r_d_size_current));
            memcpy(decoder_Symbol_Wise->codeword_vector_to_transmit[n2-1],codeword_final,size_t(codeword_size_final_temp));
            decoder_Symbol_Wise->codeword_size_vector[n2-1]=codeword_size_final_temp;
        }else{
            decoder_Symbol_Wise->push_current_codeword(codeword_received_transition, n_old, n2_old, size_received_transition,codeword_r_d_size_old);
            decoder_Symbol_Wise->symbol_wise_encode_1(k_old, n_old, k2_old,n2_old,&flag);
            if (flag==true){
                loss_counter_++;
                final_loss_counter_++;
            }
            decoder_Symbol_Wise_new->push_current_codeword(codeword_received, n, n2, size_received,codeword_r_d_size_current);//TODO to fix !!!!!
            decoder_Symbol_Wise_new->symbol_wise_encode_1(k, n, k2,n2,&flag);
            int codeword_size_final_temp = codeword_r_d_size_current + codeword_r_d_size_old + 2;  //2 extra bytes at the very beginning to indicate codeword_r_d_size_current
            *codeword_size_final = codeword_size_final_temp;


//            int codeword_r_d_size_old=ceil(float(max_payload + 2) / k2_old) * n2_old;
//            int codeword_r_d_size_current=ceil(float(max_payload + 2) / k2) * k2;

//            int codeword_size_final = codeword_r_d_size_current + codeword_r_d_size_old +
//                                      2;  //2 extra bytes at the very beginning to indicate codeword_r_d_size_current
            int temp_remainder;

            memset(codeword_final, 0, size_t(codeword_size_final_temp));

            temp_remainder = codeword_r_d_size_current % 256;
            codeword_final[1] = (unsigned char) (temp_remainder);
            codeword_final[0] = (unsigned char) ((codeword_r_d_size_current - temp_remainder) / 256);

            memcpy(codeword_final + 2, decoder_Symbol_Wise_new->codeword_new_vector[n2-1], size_t(codeword_r_d_size_current));
            memcpy(codeword_final + codeword_r_d_size_current + 2, decoder_Symbol_Wise->codeword_new_vector[n2_old - 1], size_t(codeword_r_d_size_old));
            memcpy(decoder_Symbol_Wise->codeword_vector_to_transmit[n2_old-1],codeword_final,size_t(codeword_size_final_temp));
//            // add also 2 chars to decoder_Symbol_Wise_new->codeword_new_vector[n2-1]
//            memcpy(decoder_Symbol_Wise_new->codeword_vector_to_transmit[n2-1]+2,codeword_final,size_t(codeword_r_d_size_current+2));

            decoder_Symbol_Wise->codeword_size_vector[n2_old-1]=codeword_size_final_temp;
//            // prepare decoder_Symbol_Wise_new->codeword_new_vector[n2-1] for sending if needed!!!
//            memcpy(codeword_final + 2, decoder_Symbol_Wise_new->codeword_new_vector[n2-1], size_t(codeword_r_d_size_current));
//            memcpy(decoder_Symbol_Wise_new->codeword_new_vector[n2-1],codeword_final,size_t(codeword_r_d_size_current+2));
        }

        //memcpy(codeword_new_symbol_wise,codeword_new_vector[n2-1],temp_size);//ELAD - to change
        display_udp_statistics(received_seq);
        display_fec_statistics(received_seq);

        counter_++; //Elad - need to check

        latest_seq = received_seq + 1;
        k2_last_used=k2;
        n2_last_used=n2;
        //return received_seq;//ELAD to change
    }

    void Variable_Rate_FEC_Decoder::receive_message_and_symbol_wise_decode(FEC_Message *message,int n,int k,int temp_size) {
//        if (seq_start == -1) //initialize the decoder upon receiving the first packet
//            initialize_decoder(message);
        if (seq_start == -1) { //initialize the decoder upon receiving the first packet
            decoder_Symbol_Wise->decoder_current=new Decoder(n-1, n-k, n-k, max_payload);
//            decoder_Symbol_Wise->encoder_current=new Encoder(n2-1, n2-k2, n2-k2, max_payload);
//            initialize_decoder(message);
            //encoder = new Encoder(n2 - 1, n2 - k2, n2 - k2, max_payload);
            T = message->T;
            B = message->B;
            N = message->N;
            seq_start=0;
            latest_seq = seq_start;
            k_old=k;
            n_old=n;
            k_last_used=k;
            n_last_used=n;
        }
        int received_seq = message->seq_number;

        if (received_seq < latest_seq) { //if an out-of-order sequence is received, just discard it
            //cout << "\033[1;31mOut-of-sequence packet: #" << received_seq << "\033[0m" << endl;
            return;
        }
        int transition_flag=0;
        if ((T != message->T) || (B != message->B) || (N != message->N)){ //record when the double_coding begins
            seq_start_double_coding = received_seq - message->counter_for_start_and_end;
            transition_flag=1;
            cout<<"Start double coding at the destination"<<endl;
        }

        unsigned char *codeword_received_with_header = message->buffer;
//        unsigned char *codeword_received = codeword_received_with_header + 2;
        unsigned char *codeword_received = codeword_received_with_header+2;
        int size_received = int(codeword_received_with_header[0]) * 256 + int(codeword_received_with_header[1]);
        unsigned char *codeword_received_transition = codeword_received + size_received;
//        int size_received_transition = message->size - 2 - size_received;
        int size_received_transition = message->size - size_received-2;

        unsigned char buffer[30000];
        unsigned char temp_buffer[30000];
        bool flag;

        // rotate pointers
        for (int seq = latest_seq; seq < received_seq; seq++) {// need to handle bursts longer than n
            UDP_loss_counter_++;
            final_UDP_loss_counter_++;
            DEBUG_MSG("\033[1;34m" << "Packet dropped in (r,d) #" << seq << ": " << "\033[0m");
            if ((seq > seq_end_double_coding) && (double_coding_flag == 1)) {
                double_coding_flag = 0;
//                decoder_Symbol_Wise = decoder_Symbol_Wise_new;
                decoder_Symbol_Wise->copy_elements(decoder_Symbol_Wise_new,false);
                cout<<"Stopped double coding at the destination"<<endl;
                n_last_used=n;
                k_last_used=k;
//            n2_old=n2;
//            k2_old=k2;
            }
            if (seq == seq_start_double_coding) {
                seq_end_double_coding = seq_start_double_coding + T_TOT + 1 - 1;//TODO need to make sure it use the right T...
                if (decoder_Symbol_Wise_new != NULL)
                    delete decoder_Symbol_Wise_new;
                k_old=T-N+1;
                n_old=T+1;
                T=message->T;
                B=message->B;
                N=message->N;

                decoder_Symbol_Wise_new=new Decoder_Symbol_Wise(max_payload);
                decoder_Symbol_Wise_new->decoder_current=new Decoder(T,B,N, max_payload);
                double_coding_flag = 1;
            }

            if (double_coding_flag==0){
                // the k,n inserted from above are the new ones in case packet dropped before beginning of transition...
                decoder_Symbol_Wise->rotate_pointers_and_insert_zero_word(n_last_used,0,size_received,0,false);
                flag=false;
                decoder_Symbol_Wise->symbol_wise_decode_1(buffer,&flag,k_last_used,n_last_used);
                if (flag) {
                    loss_counter_++;
                    final_loss_counter_++;
                }
                decoder_Symbol_Wise->extract_data(buffer,k,n,seq,temp_buffer);
                if (DEBUG_CHAR==1) {
                    calc_missed_chars(seq,temp_buffer);
                }
            }else{
                decoder_Symbol_Wise->rotate_pointers_and_insert_zero_word(n_old,0,size_received_transition,0,false);
                flag=false;
                decoder_Symbol_Wise->symbol_wise_decode_1(buffer,&flag,k_old,n_old);
                if (flag) {
                    loss_counter_++;
                    final_loss_counter_++;
                }
                decoder_Symbol_Wise->extract_data(buffer,k_old,n_old,seq,temp_buffer);
                if (DEBUG_CHAR==1) {
                    calc_missed_chars(seq,temp_buffer);
                }
                counter_++;
                decoder_Symbol_Wise_new->rotate_pointers_and_insert_zero_word(n,0,size_received,0,false);
                decoder_Symbol_Wise_new->symbol_wise_decode_1(buffer,&flag,k,n);
            }
//            for (int i = 0; i < n - 1; i++) {
//                memcpy(codeword_vector[i], codeword_vector[i + 1], temp_size);
//                temp_erasure_vector[i] = temp_erasure_vector[i + 1];//ELAD to check...
//            }
//            memset(codeword_vector[n - 1], '\000', (300 + 12) * 4 * T_TOT);//ELAD - 300=max_payload
//            temp_erasure_vector[n - 1] = 1;
//            DEBUG_MSG("\033[1;34m" << "Packet dropped in (r,d) #" << seq << ": " << "\033[0m");
//            symbol_wise_decode(k,n,decoder_Symbol_Wise->decoder_current->getG(),seq);// decode past messages (taking erasure in (r,d) into account)
//
//            DEBUG_MSG("\033[1;34m" << "Packet dropped in (r,d) #" << seq << ": " << "\033[0m");
//            decoder_Symbol_Wise->extract_data(buffer,k,n,received_seq);
            display_udp_statistics(seq);
        }


        if ((received_seq > seq_end_double_coding) && (double_coding_flag == 1)) {
            double_coding_flag = 0;
//            decoder_Symbol_Wise = decoder_Symbol_Wise_new;
            decoder_Symbol_Wise->copy_elements(decoder_Symbol_Wise_new,false);
            cout<<"Stopped double coding at the destination"<<endl;
            n_last_used=n;
            k_last_used=k;
//            n2_old=n2;
//            k2_old=k2;
        }

        if (received_seq == seq_start_double_coding || (received_seq>=seq_start_double_coding && transition_flag==1)) {
            seq_end_double_coding = seq_start_double_coding + T_TOT + 1 - 1;//TODO need to make sure it use the right T...
            if (decoder_Symbol_Wise_new != NULL)
                delete decoder_Symbol_Wise_new;
            k_old=T-N+1;
            n_old=T+1;
            T=message->T;
            B=message->B;
            N=message->N;

            decoder_Symbol_Wise_new=new Decoder_Symbol_Wise(max_payload);
            decoder_Symbol_Wise_new->decoder_current=new Decoder(T,B,N, max_payload);
            double_coding_flag = 1;

        }

//        // push current codeword
//        for (int i = 0; i < n - 1; i++) {
//            memcpy(codeword_vector[i], codeword_vector[i + 1], temp_size);
//            temp_erasure_vector[i] = temp_erasure_vector[i + 1];//ELAD to check...
//        }
//        memcpy(codeword_vector[n-1],message->buffer,temp_size);
//        temp_erasure_vector[n-1]=0;
//        unsigned char buffer[30000];
//        if (received_seq == seq_start_double_coding) {
//            seq_end_double_coding = seq_start_double_coding + T - 1;//TODO need to make sure it use the right T...
//            if (decoder_Symbol_Wise_new != NULL)
//                delete decoder_Symbol_Wise_new;
//            k_old=T-N+1;
//            n_old=T+1;
//            T=message->T;
//            B=message->B;
//            N=message->N;
//
//            decoder_Symbol_Wise_new=new Decoder_Symbol_Wise(max_payload);
//            decoder_Symbol_Wise_new->decoder_current=new Decoder(T,B,N, max_payload);
//            double_coding_flag = 1;
//        }
        if (double_coding_flag==0) {
            decoder_Symbol_Wise->push_current_codeword(codeword_received,n,0,size_received,0);
            flag=false;
            decoder_Symbol_Wise->symbol_wise_decode_1(buffer,&flag,k,n);
            if (flag) {
                loss_counter_++;
                final_loss_counter_++;
            }
            decoder_Symbol_Wise->extract_data(buffer,k,n,received_seq,temp_buffer);
            if (DEBUG_CHAR==1) {
                calc_missed_chars(received_seq,temp_buffer);
            }

//            // temp extraction of data
//            unsigned char temp_buffer[30000];
//            int ind=0;
//            for (int j=0;j<100;j++) {
//                for (int i = 0; i < k; i++) {
//                    temp_buffer[ind++]=buffer[(j) * n + n-k + i];
//                }
//            }
//            // Need to add decoding...
//            DEBUG_MSG("\033[1;34m" << "Message recovered at destination from symbol-wise DF #" << received_seq << ": " << "\033[0m");
//            printMatrix(&temp_buffer[2], 1, 300);
            counter_++;
            //            symbol_wise_decode(k, n, decoder_current->decoder->getG(), received_seq);
        }else{
            decoder_Symbol_Wise->push_current_codeword(codeword_received_transition,n_old,0,size_received_transition,0);
            bool flag=false;
            decoder_Symbol_Wise->symbol_wise_decode_1(buffer,&flag,k_old,n_old);

            if (flag) {
                loss_counter_++;
                final_loss_counter_++;
            }
            decoder_Symbol_Wise->extract_data(buffer,k_old,n_old,received_seq,temp_buffer);
            if (DEBUG_CHAR==1) {
                calc_missed_chars(received_seq,temp_buffer);
            }
//            // temp extraction of data
//            unsigned char temp_buffer[30000];
//            int ind=0;
//            for (int j=0;j<100;j++) {
//                for (int i = 0; i < k; i++) {
//                    temp_buffer[ind++]=buffer[(j) * n_old + n-k + i];
//                }
//            }
//            // Need to add decoding...
//            DEBUG_MSG("\033[1;34m" << "Message recovered at destination from symbol-wise DF #" << received_seq << ": " << "\033[0m");
//            printMatrix(&temp_buffer[2], 1, 300);
            counter_++;
            decoder_Symbol_Wise_new->push_current_codeword(codeword_received,n,0,size_received,0);
            decoder_Symbol_Wise_new->symbol_wise_decode_1(buffer,&flag,k,n);
        }

        display_udp_statistics(received_seq);
        display_fec_statistics(received_seq);

        latest_seq = received_seq + 1;
    }

    void Variable_Rate_FEC_Decoder::decode(FEC_Message *message) {

        if (seq_start == -1) //initialize the decoder upon receiving the first packet
            initialize_decoder(message);

        int received_seq = message->seq_number;
        int received_seq_2;

        if (received_seq < latest_seq) { //if an out-of-order sequence is received, just discard it
            //cout << "\033[1;31mOut-of-sequence packet: #" << received_seq << "\033[0m" << endl;
            return;
        }

        if ((T != message->T) || (B != message->B) || (N != message->N)) //record when the double_coding begins
            seq_start_double_coding = received_seq - message->counter_for_start_and_end;

        // first decode the old message using the old decoder

        unsigned char *codeword_received_with_header = message->buffer;
        unsigned char *codeword_received = codeword_received_with_header + 2;
        int size_received = int(codeword_received_with_header[0]) * 256 + int(codeword_received_with_header[1]);
        unsigned char *codeword_received_transition = codeword_received + size_received;
        int size_received_transition = message->size - 2 - size_received;

        if (receiver_index==0) {
            if (received_seq - latest_seq > 0) {
                if (received_seq - latest_seq <= erasure_length_cap) {
                    erasure_counter[received_seq - latest_seq]++;
                    erasure_counter_total[received_seq - latest_seq]++;
                } else {
                    erasure_counter[0]++;
                    erasure_counter_total[0]++;
                }
            }
        }else{
            received_seq_2 = message->seq_number2;
            if (received_seq_2 - latest_seq_2 > 0) {
                if (received_seq_2 - latest_seq_2 <= erasure_length_cap) {
                    erasure_counter[received_seq_2 - latest_seq_2]++;
                    erasure_counter_total[received_seq_2 - latest_seq_2]++;
                } else {
                    erasure_counter[0]++;
                    erasure_counter_total[0]++;
                }
            }
        }

        unsigned char zero = 0x0;
        unsigned char one = 0x1;

        for (int i=0;i<T_TOT;i++) {
            recovered_message_vector[i]->buffer = NULL;
        }
        recovered_message->buffer = NULL;
        message_old_encoder->buffer = NULL;

        int index=-1;
        for (int seq = latest_seq; seq < received_seq; seq++) {
            UDP_loss_counter_++;
            final_UDP_loss_counter_++;
            if (RELAYING_TYPE>0 && receiver_index==0){
                DEBUG_MSG("\033[1;31mMissing packet at relay: #" << seq << "\033[0m" << endl);
            }else{
                DEBUG_MSG("\033[1;31mMissing packet at dest: #" << seq << "\033[0m" << endl);
            }
//            if (receiver_index==1) {
//                loss_counter_two_seg_++;
//                final_loss_counter_two_seg_++;
//            }
            if (erasure_recorder == 1)
                file_write_erasures.write((char *) &one, 1);

            display_udp_statistics(seq);

            if ((seq > seq_end_double_coding) && (double_coding_flag == 1))
                double_coding_flag = 0;

            if (seq == seq_start_double_coding) {
                seq_end_double_coding = seq_start_double_coding + T - 1;
                update_decoder(message);
                double_coding_flag = 1;
            }

            if (double_coding_flag == 0) {

                decode_for_erased_codeword(recovered_message, seq,
                                           decoder_current); //message is for output by calling onDecodedMessage()

                if (seq - T >= seq_start)
                    onDecodedMessage(recovered_message);
                if (recovered_message->buffer!=NULL){
                    index++;
                    recovered_message_vector[index]->buffer=recovered_message->buffer;
                    recovered_message_vector[index]->seq_number=recovered_message->seq_number;
                }

            } else {
                int tempIndex=index+1;
                if (decoder_old != NULL) {
                    decode_for_erased_codeword(recovered_message, seq, decoder_old);

                    if (seq - T >= seq_start) {
                        onDecodedMessage(recovered_message);
                        if (recovered_message->buffer!=NULL){
                            recovered_message_vector[tempIndex]->buffer=recovered_message->buffer;
                            recovered_message_vector[tempIndex]->seq_number=recovered_message->seq_number;
                            index=tempIndex;
                        }
                    }
                }
                // Elad - decoding with new decoder
                decode_for_erased_codeword(recovered_message, seq, decoder_current);
                if (recovered_message->buffer!=NULL){ // ELAD - very delicate... if both decoders managed to recover???
                    recovered_message_vector[tempIndex]->buffer=recovered_message->buffer;
                    recovered_message_vector[tempIndex]->seq_number=recovered_message->seq_number;
                    index=tempIndex;
                }
            }

        }

        if (erasure_recorder == 1)
            file_write_erasures.write((char *) &zero, 1);

        if ((received_seq > seq_end_double_coding) && (double_coding_flag == 1))
            double_coding_flag = 0;

        if (received_seq == seq_start_double_coding) {
            seq_end_double_coding = seq_start_double_coding + T - 1;
            update_decoder(message);
            double_coding_flag = 1;
        }

        display_udp_statistics(received_seq);

        if (double_coding_flag == 0) {

            decode_for_current_codeword(message, codeword_received, size_received, received_seq,
                                        decoder_current); //message is for output by calling onDecodedMessage()

            if (received_seq - T >= seq_start)
                onDecodedMessage(message);

        } else {
            if (decoder_old != NULL) {

                decode_for_current_codeword(message, codeword_received_transition,
                                            size_received_transition, received_seq, decoder_old);

                if (received_seq - T >= seq_start) {
                    onDecodedMessage(message);
                    if (message->buffer != NULL)
                        message_old_encoder->buffer = message->buffer;
                }
            }

            decode_for_current_codeword(message, codeword_received, size_received, received_seq,
                                        decoder_current);
            if (message->buffer != NULL)
                message_old_encoder->buffer = message->buffer;
        }

        latest_seq = received_seq + 1;    //the next expected sequence number = latest_seq
        if (receiver_index==1)
            latest_seq_2 = received_seq_2 + 1;
        return;
    }

    void Variable_Rate_FEC_Decoder::onDecodedMessage(FEC_Message *message) {

        counter_++;

        if (message->buffer != NULL) {
            if (receiver_index==0)
                if (RELAYING_TYPE == 0){
                    DEBUG_MSG("\033[1;34m" << "Recovered message at destination #" << message->seq_number << ": " << "\033[0m");
                }else {
                    DEBUG_MSG("\033[1;35m" << "Recovered message at relay #" << message->seq_number << ": " << "\033[0m");
                }
            else {
                DEBUG_MSG("\033[1;34m" << "Recovered message at destination #" << message->seq_number << ": "
                                       << "\033[0m");
            }
            printMatrix(message->buffer, 1, message->size);
            if ((file_name != "") && (counter_ <= NUMBER_OF_ITERATIONS))
                save_to_file(message->buffer, message->size, &file_write_decoder);
//            if (receiver_index>0 && message->buffer[0]=='\000' && message->buffer[1]=='\000' && message->buffer[22]=='\000' && message->buffer[3]=='\000' && message->buffer[14]=='\000' && message->buffer[100]=='\000'){
//                loss_counter_two_seg_++;
//                final_loss_counter_two_seg_++;
//            }
        } else {
            if (receiver_index == 0) {
                if (RELAYING_TYPE == 0){
                    DEBUG_MSG("\033[1;31merased:" << " #" << message->seq_number << "\033[0m" << endl);
                }else{
                    DEBUG_MSG("\033[1;31merased at (s,r):" << " #" << message->seq_number << "\033[0m" << endl);
                }
            } else{
                DEBUG_MSG("\033[1;34merased at (r,d):" << " #" << message->seq_number << "\033[0m" << endl);
            }
            if ((file_name != "") && (counter_ <= NUMBER_OF_ITERATIONS))
                save_to_file(NULL, max_payload, &file_write_decoder);
            loss_counter_++;
            final_loss_counter_++;
//            if (receiver_index>0){
//                loss_counter_two_seg_++;
//                final_loss_counter_two_seg_++;
//            }

        }


        display_fec_statistics(message->seq_number);

        return;
    }


    void Variable_Rate_FEC_Decoder::initialize_decoder(FEC_Message *message) {

        //seq_start = message->seq_number - message->counter_for_start_and_end;   <--- this line can be used if the
        // sequence number is not fixed to be zero at start
        seq_start = 0;
        latest_seq = seq_start;
        T = message->T;
        B = message->B;
        N = message->N;

        decoder_current = new FEC_Decoder(max_payload, T, B, N, memory_object);

        last_report_time_ = boost::posix_time::microsec_clock::universal_time();
        start_time_ = last_report_time_;

        return;
    }

    void
    Variable_Rate_FEC_Decoder::decode_for_erased_codeword(FEC_Message *message, int seq,
                                                          FEC_Decoder *decoder) {

        int payload=0;
        unsigned char *data = NULL;

        if (decoder != NULL)
            data = decoder->onReceive(NULL, 0, seq, &payload,
                                      1); //return the payload with seq_number seq-T,  1 indicates erasure
        else
            payload = 0;

        if (payload > 0) {

            unsigned char *buffer_data = memory_object->allocate_memory(payload);

            //for (int j = 0; j < payload; j++)
            //buffer_data[j] = data[j];
            memcpy(buffer_data, data, size_t(payload));

            message->buffer = buffer_data;
            message->size = payload;
            message->seq_number = seq - T;

        } else {
            message->buffer = NULL;
            message->size = 0;
            message->seq_number = seq - T;
        }

        return;
    }

    void Variable_Rate_FEC_Decoder::decode_for_current_codeword(FEC_Message *message,
                                                                unsigned char *codeword_received,
                                                                int size_received, int received_seq,
                                                                FEC_Decoder *decoder) {

        unsigned char *data = NULL;
        int payload = 0;
        if (decoder != NULL)
            data = decoder->onReceive(codeword_received, size_received, received_seq, &payload, 0);
        else
            payload = 0;

        if (payload > 0) {

            unsigned char *buffer_data = memory_object->allocate_memory(payload);

            //for (int j = 0; j < payload; j++)
            //buffer_data[j] = data[j];
            memcpy(buffer_data, data, size_t(payload));

            message->buffer = buffer_data;
            message->size = payload;
            message->seq_number = received_seq - T;
        } else { //payload = 0 and seq-T >= seq_start
            message->buffer = NULL;
            message->size = 0;
            message->seq_number = received_seq - T;
        }

        return;
    }

    void Variable_Rate_FEC_Decoder::update_decoder(FEC_Message *message) {

        if (decoder_old != NULL)
            delete decoder_old;

        decoder_old = decoder_current;

        T = message->T;
        B = message->B;
        N = message->N;

        decoder_current = new FEC_Decoder(max_payload, T, B, N, memory_object);

//        for (int i=0;i<T_TOT+1;i++) {
//            memset(codeword_vector_new_decoder[i], '\000', (max_payload + 12) * 4 * T_TOT);
//        }
        return;
    }

    void Variable_Rate_FEC_Decoder::display_udp_statistics(int seq) {
        if (((seq + 1) % report_window_size == 0) && display_final_loss_rate_flag) { //calculate UDP loss rate

            total_session_counter++;

            double udp_loss_rate = (double) UDP_loss_counter_ / report_window_size;
            if (RELAYING_TYPE==0)
                cout << "Instantaneous UDP loss rate = " << udp_loss_rate << endl;
            else {
                if (receiver_index == 0)
                    cout << "Instantaneous UDP loss rate (s,r) = " << udp_loss_rate << endl;
                else
                    cout << "Instantaneous UDP loss rate (r,d) = " << udp_loss_rate << endl;
            }

            if (udp_loss_rate > 0.1)
                low_fidelity_session_counter_UDP++;
            if (udp_loss_rate > 0.2)
                disruption_session_counter_UDP++;

            cout << "Burst length: Counts" << endl;
            for (int i = 1; i < erasure_length_cap + 1; i++) {
                if (erasure_counter[i] != 0)
                    cout << i << ": " << erasure_counter[i] << endl;
            }
            if (erasure_counter[0] > 0)
                cout << " > " << erasure_length_cap << ": " << erasure_counter[0] << endl;

//initialize counter
            UDP_loss_counter_ = 0;
            for (int i = 0; i < erasure_length_cap + 1; i++)
                erasure_counter[i] = 0;
        }

        if ((seq + 1 == NUMBER_OF_ITERATIONS) && display_final_loss_rate_flag) {
            cout << endl << "Total number of sessions = " << total_session_counter << endl;

            cout << "Final UDP loss rate = " << (double) final_UDP_loss_counter_ / (seq - seq_start + 1)
                 << endl;

            cout << "Final UDP session low-fidelity probability = "
                 << (double) low_fidelity_session_counter_UDP / total_session_counter << endl;
            cout << "Final UDP session disruption probability = "
                 << (double) disruption_session_counter_UDP / total_session_counter << endl << endl;


            int total_number_of_bursts = 0;
            cout << "Total burst length: Counts" << endl;
            for (int i = 1; i < erasure_length_cap + 1; i++) {
                if (erasure_counter_total[i] != 0) {
                    cout << i << ": " << erasure_counter_total[i] << endl;
                    total_number_of_bursts += erasure_counter_total[i];
                }
            }
            if (erasure_counter[0] > 0)
                cout << " > " << erasure_length_cap << ": " << erasure_counter_total[0] << endl;

            cout << "Total burst length: Percentage" << endl;

            for (int i = 1; i < erasure_length_cap + 1; i++) {
                if (erasure_counter_total[i] != 0)
                    cout << i << ": " << float(erasure_counter_total[i])/total_number_of_bursts << endl;
            }
            if (erasure_counter[0] > 0)
                cout << " > " << erasure_length_cap << ": " << float(erasure_counter_total[0])/total_number_of_bursts  << endl <<endl;
        }
    }

    void Variable_Rate_FEC_Decoder::display_fec_statistics(int seq) {
        if (((seq + 1) % report_window_size == 0) && display_final_loss_rate_flag) {

            boost::posix_time::time_duration t =
                    boost::posix_time::microsec_clock::universal_time() - last_report_time_;

            last_report_time_ = boost::posix_time::microsec_clock::universal_time();

            double loss_rate = (double) loss_counter_ / counter_;
            if (RELAYING_TYPE==0)
                cout << "FEC loss rate over " << report_window_size << " packets ending at seq " << seq
                     << " = " << loss_rate << endl;
            else {
                if (receiver_index == 0)
                    cout << "FEC loss rate over (s,r) " << report_window_size << " packets ending at seq " << seq
                         << " = " << loss_rate << endl;
                else{
                    if (RELAYING_TYPE==1)
                        cout << "FEC loss rate over both links " << report_window_size << " packets ending at seq " << seq
                             << " = " << loss_rate << endl;
                    else
                        cout << "FEC loss rate over (r,d) " << report_window_size << " packets ending at seq " << seq
                             << " = " << loss_rate << endl;
                }
            }

            if (loss_rate > 0.1)
                low_fidelity_session_counter_FEC++;
            if (loss_rate > 0.2)
                disruption_session_counter_FEC++;

            cout << "Instantaneous FEC packet rate = "
                 << (double) counter_ / t.total_milliseconds() * 1000
                 << " packets per second" << endl;
            cout << "Instantaneous FEC decoding rate = "
                 << (double) counter_ * max_payload / 1000 / t.total_milliseconds() * 1000
                 << " kB/s" << endl << endl;

            //initialize counter
            loss_counter_ = 0;
            counter_ = 0;
        }

        if ((seq + 1 == NUMBER_OF_ITERATIONS) && display_final_loss_rate_flag) {

            if (receiver_index==0) {
                cout << "Final FEC loss rate = " << (double) final_loss_counter_ / (seq - seq_start + 1) <<
                     endl;
            }
            else {
                cout << "Final FEC loss rate in 2nd segment (entire link) = " << (double) final_loss_counter_ / (seq - seq_start + 1)
                     <<endl;
//                cout << "Final FEC loss entire link = " << (double) final_loss_counter_two_seg_ / (seq - seq_start + 1)
//                     <<endl;
            }
            cout << "Final FEC session low-fidelity probability = "
                 << (double) low_fidelity_session_counter_FEC / total_session_counter << endl;
            cout << "Final FEC session disruption probability = "
                 << (double) disruption_session_counter_FEC / total_session_counter << endl;

            display_final_loss_rate_flag = false;
        }
    }
    void Variable_Rate_FEC_Decoder::calc_missed_chars(int received_seq, unsigned char *temp_buffer) {
        while ((payload_simulator->current_file_position) / 300 < (received_seq + 1) - T_TOT)
            payload_simulator->generate_payload(raw_data);
        int count_char_errors = 0;
        int sum_of_false_chars = 0;
        int sum_of_all_chars = 0;
        int sum_of_all_chars_geq_200_leq_250= 0;
        if (received_seq >= T_TOT) {
            for (int kk = 0; kk < 300; kk++) {
                if (kk<250)
                    sum_of_all_chars=sum_of_all_chars+(int) temp_buffer[kk + 2];
                if (kk>200 && kk<250)
                    sum_of_all_chars_geq_200_leq_250=sum_of_all_chars_geq_200_leq_250+(int) temp_buffer[kk + 2];
                if (temp_buffer[kk + 2] != raw_data[kk]) {
                    if (kk < 250)
                        sum_of_false_chars = sum_of_false_chars + (int) temp_buffer[kk + 2];
                    count_char_errors++;
                    counter_loss_of_char++;
                    final_counter_loss_of_char++;
                }
            }
//                DEBUG_MSG("\033[1;34m" << "% of bad chars " << (float)count_char_errors/300*100 << "% " << "\033[0m");
//                DEBUG_MSG("\033[1;34m" << "Original Message #" << received_seq-T_TOT << ": " << "\033[0m");
            printMatrix(raw_data, 1, 300);
            if (sum_of_false_chars > 0 && count_char_errors > 200) {
                cout << "Elad";
                final_counter_loss_of_char_elad++;
            }
            if (sum_of_all_chars>0 && sum_of_all_chars_geq_200_leq_250==0 && count_char_errors > 100) {
                cout << "Dror";
            }
        }
    }
}