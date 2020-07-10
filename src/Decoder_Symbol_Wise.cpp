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
        codeword_vector_to_trasnmit_store = (unsigned char **) malloc(sizeof(unsigned char *) * (T_TOT + 1));

        temp_erasure_vector = (bool *) malloc(sizeof(bool) * T_TOT+1);

        codeword_vector_state_dependent = (unsigned char **) malloc(sizeof(unsigned char *) * (2*T_TOT));
        temp_erasure_vector_state_dependent = (bool *) malloc(sizeof(bool) * 2*T_TOT);
        header = (int **) malloc(sizeof(int *) * (2*T_TOT));

        int maxSizeOfHeader = T_TOT-std::min(N_INITIAL,N_INITIAL_2)+1;
        for (int i = 0; i < T_TOT + 1; i++) {
            codeword_vector[i] = (unsigned char *) malloc(
                    sizeof(unsigned char) * GLOBAL_MAX_SIZE_OF_CODEWORD); //4-byte sequence number + 4-byte
            memset(codeword_vector[i], '\000', GLOBAL_MAX_SIZE_OF_CODEWORD);
            codeword_new_vector[i] = (unsigned char *) malloc(
                    sizeof(unsigned char) * GLOBAL_MAX_SIZE_OF_CODEWORD); //4-byte sequence number + 4-byte
            memset(codeword_new_vector[i], '\000', GLOBAL_MAX_SIZE_OF_CODEWORD);
            codeword_vector_store_in_burst[i] = (unsigned char *) malloc(
                    sizeof(unsigned char) * GLOBAL_MAX_SIZE_OF_CODEWORD); //4-byte sequence number + 4-byte
            memset(codeword_vector_store_in_burst[i], '\000', GLOBAL_MAX_SIZE_OF_CODEWORD);
            codeword_vector_to_trasnmit_store[i]=(unsigned char *) malloc(
                    sizeof(unsigned char) * GLOBAL_MAX_SIZE_OF_CODEWORD); //4-byte sequence number + 4-byte
            memset(codeword_vector_to_trasnmit_store[i], '\000', GLOBAL_MAX_SIZE_OF_CODEWORD);
            codeword_vector_to_transmit[i]=(unsigned char *) malloc(
                    sizeof(unsigned char) * GLOBAL_MAX_SIZE_OF_CODEWORD); //4-byte sequence number + 4-byte
            memset(codeword_vector_to_transmit[i], '\000', GLOBAL_MAX_SIZE_OF_CODEWORD);

            n2_vector[i]=0;
            k2_vector[i]=0;
            temp_erasure_vector[i] = 0;

        }
        for (int i = 0; i < 2* T_TOT ; i++) {
            codeword_vector_state_dependent[i]=(unsigned char *) malloc(
                    sizeof(unsigned char) * GLOBAL_MAX_SIZE_OF_CODEWORD); //4-byte sequence number + 4-byte
            memset(codeword_vector_state_dependent[i], '\000', GLOBAL_MAX_SIZE_OF_CODEWORD);
            temp_erasure_vector_state_dependent[i]=0;
            header[i]=(int *) malloc(sizeof(int) * T_TOT+1);
            for (int jj=0;jj<maxSizeOfHeader;jj++)
                header[i][jj]=jj+1;
        }

    }

    Decoder_Symbol_Wise::~Decoder_Symbol_Wise() {
        for (int i=0;i<T_TOT+1;i++) {
            free(codeword_vector[i]);
            free(codeword_new_vector[i]);
            free(codeword_vector_store_in_burst[i]);
            free(codeword_vector_to_transmit[i]);
            free(codeword_vector_to_trasnmit_store[i]);
        }
        for (int i=0;i<2*T_TOT;i++) {
            free(codeword_vector_state_dependent[i]);
            free(header[i]);
        }
        free(temp_erasure_vector_state_dependent);
        free(temp_erasure_vector);
    }

    void Decoder_Symbol_Wise::copy_elements(Decoder_Symbol_Wise *source,bool encode){
        for (int i = 0; i < T_TOT + 1; i++) {
            for (int j = 0;j<GLOBAL_MAX_SIZE_OF_CODEWORD;j++) {
                codeword_vector[i][j]=source->codeword_vector[i][j];
                codeword_new_vector[i][j]=source->codeword_new_vector[i][j];
                codeword_vector_store_in_burst[i][j]=source->codeword_vector_store_in_burst[i][j];
                codeword_vector_to_transmit[i][j]=source->codeword_vector_to_transmit[i][j];
            }
            temp_erasure_vector[i]=source->temp_erasure_vector[i];
            burst_codeword_size_vector[i]=source->burst_codeword_size_vector[i];
            codeword_size_vector[i]=source->codeword_size_vector[i];
        }
        if (decoder_current!=NULL)
            delete decoder_current;
        decoder_current=new Decoder(source->decoder_current->T,source->decoder_current->B,source->decoder_current->N, source->decoder_current->max_payload);
        if (encode) {
            if (encoder_current!=NULL)
                delete encoder_current;
            encoder_current = new Encoder(source->encoder_current->T, source->encoder_current->B,
                                          source->encoder_current->N, source->encoder_current->max_payload);
        }
    }

    void Decoder_Symbol_Wise::push_current_codeword(unsigned char *message,int n,int n2, int temp_size,int codeword_r_d_size_current){
        for (int i = 0; i < n - 1; i++) {
            memcpy(codeword_vector[i], codeword_vector[i + 1], sizeof(unsigned char)*GLOBAL_MAX_SIZE_OF_CODEWORD);
            temp_erasure_vector[i] = temp_erasure_vector[i + 1];//ELAD to check...            
        }
        for (int i=0;i<n2-1;i++) {
            memcpy(codeword_new_vector[i], codeword_new_vector[i + 1], sizeof(unsigned char)*GLOBAL_MAX_SIZE_OF_CODEWORD);
            memcpy(codeword_vector_to_transmit[i], codeword_vector_to_transmit[i + 1], sizeof(unsigned char)*GLOBAL_MAX_SIZE_OF_CODEWORD);
            codeword_size_vector[i]=codeword_size_vector[i+1];
            k2_vector[i]=k2_vector[i+1];
        }

        for (int i=0;i<2*T_TOT-1;i++) {
            memcpy(codeword_vector_state_dependent[i], codeword_vector_state_dependent[i + 1], sizeof(unsigned char)*GLOBAL_MAX_SIZE_OF_CODEWORD);
            memcpy(header[i],header[i+1],sizeof(int)*T_TOT);
            temp_erasure_vector_state_dependent[i]=temp_erasure_vector_state_dependent[i+1];
        }
        memcpy(&codeword_vector[n-1][2],message,sizeof(unsigned char)*GLOBAL_MAX_SIZE_OF_CODEWORD);
        temp_erasure_vector[n - 1] = 0;
//        memcpy(&codeword_vector_state_dependent[T_TOT][2],message,sizeof(unsigned char)*GLOBAL_MAX_SIZE_OF_CODEWORD);
//        temp_erasure_vector_state_dependent[T_TOT] = 0;
    }

    void Decoder_Symbol_Wise::rotate_pointers_and_insert_zero_word(int n,int n2, int temp_size,int codeword_r_d_size_current,
            bool flag_fot_rotate_burst){
        for (int i = 0; i < n - 1; i++) {
            memcpy(codeword_vector[i], codeword_vector[i + 1], sizeof(unsigned char)*GLOBAL_MAX_SIZE_OF_CODEWORD);
            temp_erasure_vector[i] = temp_erasure_vector[i + 1];//ELAD to check...

//            memcpy(codeword_vector_to_trasnmit_store[i], codeword_vector_to_trasnmit_store[i + 1], sizeof(unsigned char)*GLOBAL_MAX_SIZE_OF_CODEWORD);
//            store_codeword_size_vector[i]=store_codeword_size_vector[i+1];
            if (flag_fot_rotate_burst==true) {
                memcpy(codeword_vector_store_in_burst[i], codeword_vector_store_in_burst[i + 1],
                       sizeof(unsigned char) * GLOBAL_MAX_SIZE_OF_CODEWORD);
                burst_codeword_size_vector[i]=burst_codeword_size_vector[i+1];
            }
        }
        for (int i=0;i<n2-1;i++) {
            memcpy(codeword_new_vector[i], codeword_new_vector[i + 1], sizeof(unsigned char)*GLOBAL_MAX_SIZE_OF_CODEWORD);
            memcpy(codeword_vector_to_transmit[i], codeword_vector_to_transmit[i + 1], sizeof(unsigned char)*GLOBAL_MAX_SIZE_OF_CODEWORD);
//            if (flag_fot_rotate_burst==true) {
//                memcpy(codeword_vector_store_in_burst[i], codeword_vector_store_in_burst[i + 1],
//                       sizeof(unsigned char) * GLOBAL_MAX_SIZE_OF_CODEWORD);
//                burst_codeword_size_vector[i]=burst_codeword_size_vector[i+1];
//            }
            codeword_size_vector[i] = codeword_size_vector[i+1];
            k2_vector[i]=k2_vector[i+1];
        }
        for (int i=0;i<2*T_TOT-1;i++) {
            memcpy(codeword_vector_state_dependent[i], codeword_vector_state_dependent[i + 1], sizeof(unsigned char)*GLOBAL_MAX_SIZE_OF_CODEWORD);
            memcpy(header[i],header[i+1],sizeof(int)*T_TOT);
            temp_erasure_vector_state_dependent[i]=temp_erasure_vector_state_dependent[i+1];
        }
        memset(codeword_vector[n - 1], '\000', sizeof(unsigned char)*GLOBAL_MAX_SIZE_OF_CODEWORD);//ELAD - 300=max_payload
        temp_erasure_vector[n - 1] = 1;
//        memset(codeword_vector_state_dependent[T_TOT], '\000', sizeof(unsigned char)*GLOBAL_MAX_SIZE_OF_CODEWORD);//ELAD - 300=max_payload
//        temp_erasure_vector_state_dependent[T_TOT]=1;
    }

    void Decoder_Symbol_Wise::symbol_wise_encode_state_dependent(int k,int n,int k2,int n2, bool *flag) {
        int erasure_counter = 0;
        for (int i = 0; i < n; i++) {
            if (temp_erasure_vector[i] == 1)
                erasure_counter++;
        }
        int number_of_code_blocks =
                ceil(max_payload / k) + 1;//Since k=k2 no need to worry !
//        int number_of_code_blocks_r_d=ceil(300/k2);

        // Extract the relevant parts from the coded message

        unsigned char temp_codeword[n];
        unsigned char temp_encoded_codeword[n2];
        //memset(codeword_new_vector[T_INITIAL],'\000',(300 + 12) * 4 * T_INITIAL);//ELAD maxpayload
        bool *stam_erasure_vector = (bool *) malloc(sizeof(bool) * T_TOT);
        *flag = false;
        for (int j = 0; j < number_of_code_blocks; j++) {
            int index = -1;
            int tempHeader[T_TOT];
//            for (int symInd = k - N_INITIAL; symInd >= -N_INITIAL_2; symInd--) {
            for (int symInd = k - 1; symInd >= -N_INITIAL_2; symInd--) {
                index++;
                int symbolIndex = -1;
                for (int i = symInd; i < n; i++) {
                    // Need to add decoding
                    //codeword_new_symbol_wise[10+(j)*n+i] = codeword_vector[n -1 - i][10+(j+1)*n-(n-k) - i];
                    symbolIndex++;
                    temp_codeword[symbolIndex] = codeword_vector_state_dependent[i+T_TOT-n+1][2 + j * n +
                                                                    symbolIndex]; // temp_codeword holds the diagonal (a_0,b_1,c_2...)!!!
                }
                if (n - symInd <= k) {// In case the # of sym <=k forward
                    bool notFoundSym = true;
                    int symbolIndex2 = -1;
                    for (int i=0;i<n;i++)
                        tempHeader[i]=0;
                    for (int i = symbolIndex - N_INITIAL; i >= 1; i--) {
                        symbolIndex2++;
                        tempHeader[symbolIndex2] = header[n2 - i - 1][symbolIndex2];
                    }
                    for (int kk = index; kk < n - symInd; kk++) {
                        if (temp_erasure_vector_state_dependent[kk+symInd + T_TOT - n +1] == 0) {
                            // check that this symbol was not sent before
                            bool notFoundFlag = true;
                            for (int jj=0;jj<kk;jj++){
                                if (tempHeader[jj]==kk+1)
                                    notFoundFlag=false;
                            }
                            if (notFoundFlag==true) {
                                codeword_new_symbol_wise[2 + (j) * n2 + index] = temp_codeword[kk];
                                header[n2 - 1][index] = kk + 1;
                                notFoundSym = false;
                                break;
                            }
                        }
                    }
                    if (notFoundSym) {
                        codeword_new_symbol_wise[2 + (j) * n2 + index] = 0;
                        // find non used index
                        int potIndex;
                        for (potIndex=1;potIndex<n;potIndex++) {
                            bool notFoundInd=true;
                            for (int aa = 0; aa <= symbolIndex2; aa++) {
                                if (tempHeader[aa]==potIndex) {
                                    notFoundInd = false;
                                    break;
                                }
                            }
                            if (notFoundInd==true)
                                break;
                        }
                        header[n2-1][index]=potIndex; //Modify the header in case of too many erasures
                    }
                } else {// need to perform decoding and send original symbols
                    for (int aa=0;aa<n2;aa++)
                        stam_erasure_vector[aa] = 0;
                    for (int aa = 0; aa < n - symInd; aa++) {
//                        stam_erasure_vector[aa] = temp_erasure_vector_state_dependent[aa+T_TOT-1-index];
                        stam_erasure_vector[aa] = temp_erasure_vector_state_dependent[aa+symInd +T_TOT-n+1];
                    }
                    for (int aa = n - symInd; aa < n; aa++)
                        stam_erasure_vector[aa] = 1;
                    int erasure_count=0;
                    for (int aa=0;aa<n;aa++) {
                        if (stam_erasure_vector[aa] == 1)
                            erasure_count++;
                    }
                    decodeBlock(&temp_codeword[0], decoder_current->getG(), &temp_codeword[0], stam_erasure_vector, k,
                                n, n - 1, 0);// T=n-1
                    memcpy(temp_encoded_codeword, temp_codeword, n * sizeof(unsigned char));
//                    if (erasure_count<= n-k)
                    encodeBlock(&temp_codeword[0], encoder_current->getG(), &temp_encoded_codeword[0], k2, n2, k2 - 1);

                    // check which symbol was not sent in the past
                    for (int i=0;i<n2;i++)
                        tempHeader[i]=0;
                    int symbolIndex2 = -1;
                    for (int i = symbolIndex - N_INITIAL; i >= 1; i--) {
//                    for (int i = n2 - 1; i >= 0; i--) {
                        symbolIndex2++;
                        tempHeader[symbolIndex2] = header[n2 - i  - 1][symbolIndex2];
                    }
                    //int indexToSend=-1;
                    bool notFoundFlag = true;
                    bool not_assinged_val = true;
//                    for (int i = 0; i <= symbolIndex; i++) {
                    for (int i = 0; i < n2; i++) {
                        notFoundFlag = true;
                        for (int kk = 0; kk < symbolIndex-N_INITIAL; kk++) {
                            if (tempHeader[kk] == i + 1) {
                                notFoundFlag = false;
                                break;
                            }
                        }
                        if (notFoundFlag) {
                            if (erasure_count<= n-k) {
                                codeword_new_symbol_wise[2 + (j) * n2 + index] = temp_encoded_codeword[i];
                                header[n2 - 1][index] = i + 1;
                                not_assinged_val=false;
                                break;
                            }else{// didn't decode, need to forward
                                if (stam_erasure_vector[i]==0){
                                    codeword_new_symbol_wise[2 + (j) * n2 + index] = temp_encoded_codeword[i];
                                    header[n2 - 1][index] = i + 1;
                                    not_assinged_val=false;
                                    break;
                                }
                            }
                        }
                    }
                    if (not_assinged_val==true){//need to handle case with too many erasure (failed to recover...)
                        codeword_new_symbol_wise[2 + (j) * n2 + index] = 0;

                        for (int i=0;i<n2;i++){
                            notFoundFlag = true;
                            for (int kk = 0; kk < index; kk++) {
                                if (tempHeader[kk] == i + 1) {
                                    notFoundFlag = false;
                                    break;
                                }
                            }
                            if (notFoundFlag==true) {
                                header[n2 - 1][index] = i + 1;
                                break;
                            }
                        }
//                        header[n2-1][index]=index+1;
                    }
                }
            }
//            if (j<=4){
//                cout << "[";
//                for (int i = 0; i < n2; i++)
//                    cout << header[n2 - 1][i];
//                cout << "]" << endl;
//                cout << "Elad" << endl;
//            }
        }
        for (int j=0;j<number_of_code_blocks;j++) {
            for (int i = 0; i < n2; i++) {
                codeword_new_vector[n2-1][2+(j)*n2+i]=codeword_new_symbol_wise[2+(j)*n2+i];
            }
        }
        if (DEBUG_FEC) {
            cout << "[";
            for (int i = 0; i < n2; i++)
                cout << header[n2 - 1][i];
            cout << "]" << endl;
            cout << "Elad" << endl;
        }
//        cout << "Elad" << endl;
//        cout << "Elad" << endl;
    }
//                // Decoding
//                if (erasure_counter > 0 && erasure_counter < (n - k + 1)) {
//                    for (int aa = 0; aa < n - 1; aa++)
//                        stam_erasure_vector[aa] = temp_erasure_vector[aa];
//                    decodeBlock(&temp_codeword[0], decoder_current->getG(), &temp_codeword[0], stam_erasure_vector, k,
//                                n, n - 1, 0);// T=n-1
//                } else if (erasure_counter >= (n - k + 1)) {
//                    *flag = true;
//                }
//                for (int i = 0; i < k; i++)
//                    codeword_new_symbol_wise[2 + (j) * n + i] = temp_codeword[k - 1 - i];
//            }

//        if (flag==true){
//            loss_counter_++;
//            final_loss_counter_++;
//        }
//        free(stam_erasure_vector);
//
//        // Encoding
//        int k_min=(int) std::min(k,k2);
//        int delta_k;
//        if (k2>k)
//            delta_k=k2-k;
//        else
//            delta_k=0;
//        for (int j=0;j<number_of_code_blocks;j++) {
//            for (int i = 0; i < k_min; i++) {
//                codeword_new_vector[n2-1][2+(j)*n2+i+delta_k]=codeword_new_symbol_wise[2+(j)*n+i];
//            }
//        }
//        //memcpy(codeword_new_vector[n2-1],codeword_new_symbol_wise,temp_size);//ELAD - to change
//
//
//        for (int j=0;j<number_of_code_blocks;j++) {
//            for (int delta=0;delta<n2-k2;delta++) {//this is to fill all parity symbols in codeword_new_vector[n2 - 1]
//                for (int i = 0+delta; i < k_min+delta; i++) {
//                    temp_codeword[i-delta] = codeword_new_vector[i][2 + j * n2 +i-delta]; // temp_codeword holds the diagonal (c_0,b_0,a_0...)!!!
//                }
//                if (k2>=k)
//                    memcpy(&temp_encoded_codeword[k2-k], temp_codeword, k * sizeof(unsigned char));
//                else
//                    memcpy(temp_encoded_codeword, temp_codeword, k * sizeof(unsigned char));
//                encodeBlock(&temp_codeword[0], encoder_current->getG(), &temp_encoded_codeword[0], k2, n2, k2 - 1);
//
//                codeword_new_vector[n2 - 1][2 + j * n2 + n2 - 1-delta] = temp_encoded_codeword[n2 - 1-delta];
//            }
////            for (int i = k2; i < n2; i++) {
////                codeword_new_vector[i][2+j*n2+i]=temp_encoded_codeword[i];
////                //codeword_new_symbol_wise[10+j*n+i]=temp_encoded_codeword[i];
////            }
//        }
//    }

    void Decoder_Symbol_Wise::symbol_wise_decode_state_dependent(unsigned char *buffer,bool *flag,int k,int n){

        // Extract the relevant parts from the coded message
        // If no erasures:

//        unsigned char buffer[30000];
        unsigned char temp_codeword[n];
        int number_of_code_blocks=ceil(max_payload/k)+1;
        bool *stam_erasure_vector=(bool *) malloc(sizeof(bool)*T_INITIAL);
        int tempHeader[T_TOT+1];
        *flag=false;
        for (int j=0;j<number_of_code_blocks;j++) {
            for (int k_shift=0;k_shift<k;k_shift++) {
                for (int i = 0; i < n; i++) {
//                    temp_codeword[n - 1 - i] = codeword_vector_state_dependent[n - 1 - i][4 + (j + 1) * n - 1 -
//                                                                          i];// code word is [c_0,b_0,a_0,...]
                    temp_codeword[n - 1 - i] = codeword_vector_state_dependent[2*T_TOT-k_shift - i-1][4 + (j + 1) * n - 1 -
                                                                                          i];// code word is [c_0,b_0,a_0,...]
                    //buffer[(j)*n+i] = codeword_vector[n -1 - i][10+(j+1)*n-(n-k) - i];
                    tempHeader[n-1-i] = header[2*T_TOT-k_shift - i-1][n-1-i];
                }

                unsigned char temp_temp_codeword[n];
                for (int i = 0; i < n; i++)
                    temp_temp_codeword[i]='\000';
                for (int i = 0; i < n; i++) {
                    if (tempHeader[i]!=0)
                        temp_temp_codeword[tempHeader[i] - 1] = temp_codeword[i];
                }
                for (int i = 0; i < n; i++)
                    temp_codeword[i] = temp_temp_codeword[i];

                int erasure_counter=0;
                for (int i=0;i<n;i++){
                    if (tempHeader[i]==0)
                        erasure_counter++;
                }
                if (erasure_counter > 0 && erasure_counter < (n - k + 1)) {
                    for (int aa = 0; aa < n ; aa++)
                        stam_erasure_vector[aa] = 1;
                    for (int aa = 0; aa < n ; aa++) {
                        if (tempHeader[aa]!=0)
                            stam_erasure_vector[tempHeader[aa]-1] = 0;
                    }
                    decodeBlock(&temp_codeword[0], decoder_current->getG(), &temp_codeword[0], stam_erasure_vector, k,
                                n, n - 1, 0);// T=n-1
                } else if (erasure_counter >= (n - k + 1)) {
                    *flag = true;
                }
                buffer[(j) * n + n-k+k_shift] = temp_codeword[k_shift];
//                for (int i = 0; i < n; i++) {
//                    //buffer[(j)*n+i]=temp_codeword[n-1-i]; //code word in [X,a_0,b_0,c_0]
//                    buffer[(j) * n + i] = temp_codeword[i];
//                }
            }
        }
    }

    void Decoder_Symbol_Wise::symbol_wise_encode_1(int k,int n,int k2,int n2, bool *flag){
        int erasure_counter=0;
        for (int i=0;i<n;i++){
            if (temp_erasure_vector[i]==1)
                erasure_counter++;
        }
        int number_of_code_blocks=ceil(max_payload/k)+1;//TODO replace 300 wih max_payload !!! Since k=k2 no need to worry !
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
                for (int aa=0;aa<n-1;aa++) //TODO: to check "n-1" or "n"
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
        int number_of_code_blocks=ceil(max_payload/k)+1;//TODO replace 300 wih max_payload !!!
        bool *stam_erasure_vector=(bool *) malloc(sizeof(bool)*T_INITIAL);
        *flag=false;
        for (int j=0;j<number_of_code_blocks;j++) {
            for (int i = 0; i < n; i++) {
                temp_codeword[n-1-i] = codeword_vector[n -1 - i][4+(j+1)*n-1 - i];// code word is [c_0,b_0,a_0,...]
                //buffer[(j)*n+i] = codeword_vector[n -1 - i][10+(j+1)*n-(n-k) - i];
            }
            if (erasure_counter>0 && erasure_counter<(n-k+1)) {
                for (int aa = 0; aa < n-1; aa++) //TODO: to check "n-1" or "n"
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
        int number_of_code_blocks=ceil(max_payload/k)+1;
        int ind=0;
        for (int j=0;j<number_of_code_blocks;j++) {
            for (int i = 0; i < k; i++) {
                temp_buffer[ind++]=buffer[0+(j) * n + n-k + i];
            }
        }
// Need to add decoding...
        DEBUG_MSG("\033[1;34m" << "Message recovered at destination (T=" << n-1 << ", N=" << n-k <<") R=" << k <<"/" << n
                               << " from symbol-wise DF received seq #" << received_seq << " original seq #" << received_seq-T_TOT << ": " << "\033[0m");
        printMatrix(&temp_buffer[2], 1, max_payload);}


}