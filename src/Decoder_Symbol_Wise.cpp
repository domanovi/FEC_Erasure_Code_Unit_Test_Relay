//
// Created by elad on 2020-05-18.
//

#include <ios>
#include <cstring>
#include <Memory_Allocator.h>
#include <codingOperations.h>
#include "Decoder_Symbol_Wise.h"
#include "FEC_Macro.h"
#include "basicOperations.h"

using std::ios;
namespace siphon {

    Decoder_Symbol_Wise::Decoder_Symbol_Wise(int max_payload_value){
        max_payload=max_payload_value;
        codeword_vector = (unsigned char **) malloc(sizeof(unsigned char *) * (T_TOT + 1));
        codeword_new_vector = (unsigned char **) malloc(sizeof(unsigned char *) * (T_TOT + 1));
        codeword_vector_store_in_burst = (unsigned char **) malloc(sizeof(unsigned char *) * (T_TOT + 1));
        codeword_vector_to_transmit = (unsigned char **) malloc(sizeof(unsigned char *) * (T_TOT + 1));

        temp_erasure_vector = (bool *) malloc(sizeof(bool) * T_TOT);
        for (int i = 0; i < T_TOT + 1; i++) {
            codeword_vector[i] = (unsigned char *) malloc(
                    sizeof(unsigned char) * 10000); //4-byte sequence number + 4-byte
            memset(codeword_vector[i], '\000', 10000);
            codeword_new_vector[i] = (unsigned char *) malloc(
                    sizeof(unsigned char) * 10000); //4-byte sequence number + 4-byte
            memset(codeword_new_vector[i], '\000', 10000);
            codeword_vector_store_in_burst[i] = (unsigned char *) malloc(
                    sizeof(unsigned char) * 10000); //4-byte sequence number + 4-byte
            memset(codeword_vector_store_in_burst[i], '\000', 10000);
            codeword_vector_to_transmit[i]=(unsigned char *) malloc(
                    sizeof(unsigned char) * 10000); //4-byte sequence number + 4-byte
            memset(codeword_vector_to_transmit[i], '\000', 10000);
            temp_erasure_vector[i] = 0;
            n2_vector[i]=0;
            k2_vector[i]=0;
        }

    }

    Decoder_Symbol_Wise::~Decoder_Symbol_Wise() {
        for (int i=0;i<T_TOT;i++) {
            free(codeword_vector[i]);
            free(codeword_new_vector[i]);
            free(codeword_vector_store_in_burst[i]);
            free(codeword_vector_to_transmit[i]);
        }
    }

    void Decoder_Symbol_Wise::copy_elements(Decoder_Symbol_Wise *source,bool encode){
        for (int i = 0; i < T_TOT + 1; i++) {
            for (int j = 0;j<10000;j++) {
                codeword_vector[i][j]=source->codeword_vector[i][j];
                codeword_new_vector[i][j]=source->codeword_new_vector[i][j];
                codeword_vector_store_in_burst[i][j]=source->codeword_vector_store_in_burst[i][j];
                codeword_vector_to_transmit[i][j]=source->codeword_vector_to_transmit[i][j];
            }
            temp_erasure_vector[i]=source->temp_erasure_vector[i];
            burst_codeword_size_vector[i]=source->burst_codeword_size_vector[i];
            codeword_size_vector[i]=source->codeword_size_vector[i];
        }
        decoder_current=new Decoder(source->decoder_current->T,source->decoder_current->B,source->decoder_current->N, source->decoder_current->max_payload);
        if (encode)
            encoder_current=new Encoder(source->encoder_current->T,source->encoder_current->B,source->encoder_current->N, source->encoder_current->max_payload);
    }

    void Decoder_Symbol_Wise::push_current_codeword(unsigned char *message,int n,int n2, int temp_size,int codeword_r_d_size_current){
        for (int i = 0; i < n - 1; i++) {
            memcpy(codeword_vector[i], codeword_vector[i + 1], 10000);
            temp_erasure_vector[i] = temp_erasure_vector[i + 1];//ELAD to check...
        }
        for (int i=0;i<n2-1;i++) {
            memcpy(codeword_new_vector[i], codeword_new_vector[i + 1], 10000);
            memcpy(codeword_vector_to_transmit[i], codeword_vector_to_transmit[i + 1], 10000);
            codeword_size_vector[i]=codeword_size_vector[i+1];
            k2_vector[i]=k2_vector[i+1];
        }
        memcpy(&codeword_vector[n-1][2],message,sizeof(unsigned char)*10000);
        temp_erasure_vector[n - 1] = 0;
    }

    void Decoder_Symbol_Wise::rotate_pointers_and_insert_zero_word(int n,int n2, int temp_size,int codeword_r_d_size_current,bool flag_fot_rotate_burst){
        for (int i = 0; i < n - 1; i++) {
            memcpy(codeword_vector[i], codeword_vector[i + 1], sizeof(unsigned char)*10000);
            temp_erasure_vector[i] = temp_erasure_vector[i + 1];//ELAD to check...
        }
        for (int i=0;i<n2-1;i++) {
            memcpy(codeword_new_vector[i], codeword_new_vector[i + 1], sizeof(unsigned char)*10000);
            memcpy(codeword_vector_to_transmit[i], codeword_vector_to_transmit[i + 1], 10000);
            if (flag_fot_rotate_burst==true) {
                memcpy(codeword_vector_store_in_burst[i], codeword_vector_store_in_burst[i + 1],
                       sizeof(unsigned char) * 10000);
                burst_codeword_size_vector[i]=burst_codeword_size_vector[i+1];
            }
            codeword_size_vector[i] = codeword_size_vector[i+1];
            k2_vector[i]=k2_vector[i+1];
        }
        memset(codeword_vector[n - 1], '\000', sizeof(unsigned char)*10000);//ELAD - 300=max_payload
        temp_erasure_vector[n - 1] = 1;
    }


    void Decoder_Symbol_Wise::symbol_wise_encode_1(int k,int n,int k2,int n2, bool *flag){
        int erasure_counter=0;
        for (int i=0;i<n;i++){
            if (temp_erasure_vector[i]==1)
                erasure_counter++;
        }
        int number_of_code_blocks=ceil(304/k);//TODO replace 300 wih max_payload !!! Since k=k2 no need to worry !
//        int number_of_code_blocks_r_d=ceil(300/k2);//TODO replace 300 wih max_payload !!!

        // Extract the relevant parts from the coded message

        unsigned char temp_codeword[n];
        unsigned char temp_encoded_codeword[n2];
        //memset(codeword_new_vector[T_INITIAL],'\000',(300 + 12) * 4 * T_INITIAL);//ELAD maxpayload
        bool *stam_erasure_vector=(bool *) malloc(sizeof(bool)*T_TOT);
        *flag=false;
        for (int j=0;j<number_of_code_blocks;j++) {
            for (int i = 0; i < n; i++) {
                // Need to add decoding
                //codeword_new_symbol_wise[10+(j)*n+i] = codeword_vector[n -1 - i][10+(j+1)*n-(n-k) - i];
                temp_codeword[i]=codeword_vector[i][2+j*n+i]; // temp_codeword holds the diagonal (a_0,b_1,c_2...)!!!
            }
            // Decoding
            if (erasure_counter>0 && erasure_counter<(n-k+1)){
                for (int aa=0;aa<n-1;aa++)
                    stam_erasure_vector[aa]=temp_erasure_vector[aa];
                decodeBlock(&temp_codeword[0], decoder_current->getG(), &temp_codeword[0], stam_erasure_vector,  k,  n,n-1,0);// T=n-1
            }else if (erasure_counter>=(n-k+1)){
                *flag=true;
            }
            for (int i=0;i<k;i++)
                codeword_new_symbol_wise[2+(j)*n+i]=temp_codeword[k-1-i];
        }
//        if (flag==true){
//            loss_counter_++;
//            final_loss_counter_++;
//        }
        free(stam_erasure_vector);

        // Encoding
        int k_min=(int) std::min(k,k2);
        int delta_k;
        if (k2>k)
            delta_k=k2-k;
        else
            delta_k=0;
        for (int j=0;j<number_of_code_blocks;j++) {
            for (int i = 0; i < k_min; i++) {
                codeword_new_vector[n2-1][2+(j)*n2+i+delta_k]=codeword_new_symbol_wise[2+(j)*n+i];
            }
        }
        //memcpy(codeword_new_vector[n2-1],codeword_new_symbol_wise,temp_size);//ELAD - to change


        for (int j=0;j<number_of_code_blocks;j++) {
            for (int delta=0;delta<n2-k2;delta++) {//this is to fill all parity symbols in codeword_new_vector[n2 - 1]
                for (int i = 0+delta; i < k_min+delta; i++) {
                    temp_codeword[i-delta] = codeword_new_vector[i][2 + j * n2 +i-delta]; // temp_codeword holds the diagonal (c_0,b_0,a_0...)!!!
                }
                if (k2>=k)
                    memcpy(&temp_encoded_codeword[k2-k], temp_codeword, k * sizeof(unsigned char));
                else
                    memcpy(temp_encoded_codeword, temp_codeword, k * sizeof(unsigned char));
                encodeBlock(&temp_codeword[0], encoder_current->getG(), &temp_encoded_codeword[0], k2, n2, k2 - 1);

                codeword_new_vector[n2 - 1][2 + j * n2 + n2 - 1-delta] = temp_encoded_codeword[n2 - 1-delta];
            }
//            for (int i = k2; i < n2; i++) {
//                codeword_new_vector[i][2+j*n2+i]=temp_encoded_codeword[i];
//                //codeword_new_symbol_wise[10+j*n+i]=temp_encoded_codeword[i];
//            }
        }
    }

    void Decoder_Symbol_Wise::symbol_wise_decode_1(unsigned char *buffer,bool *flag,int k,int n){
        int erasure_counter=0;
        for (int i=0;i<n;i++){
            if (temp_erasure_vector[i]==1)
                erasure_counter++;
        }
        // Extract the relevant parts from the coded message
        // If no erasures:

//        unsigned char buffer[30000];
        unsigned char temp_codeword[n];
        int number_of_code_blocks=ceil(304/k);//TODO replace 300 wih max_payload !!!
        bool *stam_erasure_vector=(bool *) malloc(sizeof(bool)*T_INITIAL);
        *flag=false;
        for (int j=0;j<number_of_code_blocks;j++) {
            for (int i = 0; i < n; i++) {
                temp_codeword[n-1-i] = codeword_vector[n -1 - i][4+(j+1)*n-1 - i];// code word is [c_0,b_0,a_0,...]
                //buffer[(j)*n+i] = codeword_vector[n -1 - i][10+(j+1)*n-(n-k) - i];
            }
            if (erasure_counter>0 && erasure_counter<(n-k+1)) {
                for (int aa = 0; aa < n-1; aa++)
                    stam_erasure_vector[aa] = temp_erasure_vector[aa];
                decodeBlock(&temp_codeword[0], decoder_current->getG(), &temp_codeword[0], stam_erasure_vector, k, n, n-1, 0);// T=n-1
            }else if (erasure_counter>=(n-k+1)){
                *flag=true;
            }
            for (int i=0;i<n;i++){
                buffer[(j)*n+i]=temp_codeword[n-1-i]; //code word in [X,a_0,b_0,c_0]
            }
        }
    }

    void Decoder_Symbol_Wise::extract_data(unsigned char *buffer,int k,int n,int received_seq,unsigned char *temp_buffer){// temp extraction of data
//        unsigned char temp_buffer[30000];
        int number_of_code_blocks=ceil(304/k);//TODO replace 300 wih max_payload !!!
        int ind=0;
        for (int j=0;j<number_of_code_blocks;j++) {
            for (int i = 0; i < k; i++) {
                temp_buffer[ind++]=buffer[0+(j) * n + n-k + i];
            }
        }
// Need to add decoding...
        DEBUG_MSG("\033[1;34m" << "Message recovered at destination (T=" << n-1 << ", N=" << n-k <<") R=" << k <<"/" << n
        << " from symbol-wise DF received seq #" << received_seq << " original seq #" << received_seq-T_TOT << ": " << "\033[0m");
        printMatrix(&temp_buffer[2], 1, 300);}


}