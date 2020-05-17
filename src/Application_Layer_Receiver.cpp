//
// Created by silas on 11/03/19.
//

#include <basicOperations.h>
#include "Application_Layer_Receiver.h"
#include "FEC_Macro.h"

Application_Layer_Receiver::Application_Layer_Receiver(const char *Tx, const char *Rx, int max_payload, int
erasure_type_value, bool adaptive_mode_MDS_value, int flag) {

    erasure_type = erasure_type_value;

    estimator = new siphon::Parameter_Estimator(T_TOT, adaptive_mode_MDS_value);
    background_estimator = new siphon::Parameter_Estimator(T_TOT, adaptive_mode_MDS_value);
    flag_for_estimation_cycle=1;
    if (erasure_type != 5) {
        fec_decoder = new siphon::Variable_Rate_FEC_Decoder(max_payload, OUTPUTDATAFILE, ERASURE_RECORDER);
        fec_decoderTemp = new siphon::Variable_Rate_FEC_Decoder(max_payload, OUTPUTDATAFILE, ERASURE_RECORDER);
    }
    else
        fec_decoder = new siphon::Variable_Rate_FEC_Decoder(max_payload, OUTPUTDATAFILE, 0);   //do not record if the
    // existing erasure file is used (when erasure_type equals 5)

    fec_message = new FEC_Message();
    fec_messageTemp = new FEC_Message();
    codeword = (unsigned char *) malloc(
            sizeof(unsigned char) * (max_payload + 12) * 4 * T_TOT); //4-byte sequence number + 4-byte
    // coding parameters
    codeword_vector = (unsigned char **) malloc(sizeof(unsigned char *) * (T_TOT+1));
    codeword_new_vector=(unsigned char **) malloc(sizeof(unsigned char *) * (T_TOT+1));
    codeword_vector_store_in_burst=(unsigned char **) malloc(sizeof(unsigned char *) * (T_TOT+1));

    temp_erasure_vector=(bool *) malloc(sizeof(bool)*T_TOT);
    for (int i=0;i<T_TOT+1;i++){
        codeword_vector[i]=(unsigned char *) malloc(
                sizeof(unsigned char) * (max_payload + 12) * 4 * T_TOT); //4-byte sequence number + 4-byte
        memset(codeword_vector[i],'\000',(max_payload + 12) * 4 * T_TOT);
        codeword_new_vector[i]=(unsigned char *) malloc(sizeof(unsigned char) * (max_payload + 12) * 4 * T_TOT); //4-byte sequence number + 4-byte
        codeword_vector_store_in_burst[i]=(unsigned char *) malloc(sizeof(unsigned char) * (max_payload + 12) * 4 * T_TOT); //4-byte sequence number + 4-byte
        //memset(codeword_new_vector[i],'\000',(max_payload + 12) * 4 * T_INITIAL);
        temp_erasure_vector[i]=0;
    }

    //codeword_new_vector[T_INITIAL]=(unsigned char *) malloc(sizeof(unsigned char) * (max_payload + 12) * 4 * T_INITIAL); //4-byte sequence number + 4-byte
    memset(codeword_new_symbol_wise,'\000',(max_payload + 12) * 4 * T_TOT);

    connection_manager = new ConnectionManager(Tx, Rx,flag);

    receiver_index=0;
    latest_seq=0;


}

Application_Layer_Receiver::~Application_Layer_Receiver() {
    free(codeword);
    for (int i=0;i<T_TOT;i++) {
        free(codeword_vector[i]);
        free(codeword_new_vector[i]);
    }
    delete fec_message;
    delete fec_messageTemp;
    delete estimator;
    delete background_estimator;
    delete fec_decoder;
    delete fec_decoderTemp;
    delete connection_manager;
}

void Application_Layer_Receiver::set_receiver_index(int index){
    receiver_index=index;
    fec_decoder->receiver_index=index;
}

int Application_Layer_Receiver::receive_message_and_symbol_wise_encode(unsigned char *udp_parameters, unsigned char *udp_codeword,
                                                                       int *udp_codeword_size, siphon::Erasure_Simulator *erasure_simulator, int k2, int n2) {
    //int temp_size;

    //receive(&temp_size, codeword, udp_codeword, udp_codeword_size);
    if (udp_parameters == nullptr)
        connection_manager->UDPreceive(&temp_size, codeword);
    else{
        temp_size = *udp_codeword_size;
        memcpy(codeword, udp_codeword, size_t(temp_size));
    }
    int temp_seq = int(codeword[3]) + 256 * int(codeword[2]) + 256 * 256 * int(codeword[1]) + 256 * 256 * 256 * int
            (codeword[0]);

    // Perform estimation
    fec_message->set_parameters(temp_seq, int(codeword[4]), int(codeword[5]), int(codeword[6]),
                                temp_size - 8, codeword + 8);
    fec_message->counter_for_start_and_end = int(codeword[7]);

    estimator->estimate(fec_message);

    int T_value=int(codeword[4]);
    int B_value=int(codeword[5]);
    int N_value=int(codeword[6]);

    int k=T_value-N_value+1;
    int n=T_value+1;

//    int k=T_INITIAL-N_INITIAL+1; //Elad - to change
//    int n=T_INITIAL+1; //Elad - to change


//    if (temp_seq==0){//ELAD - need to handle the cases where packet 0 not arriving !!!
//        fec_decoder->initialize_decoder(fec_message);
//    }
//    unsigned char* generator= fec_decoder->decoder_current->decoder->getG();
//
//    if (temp_seq < latest_seq) { //if an out-of-order sequence is received, just discard it
//        //cout << "\033[1;31mOut-of-sequence packet: #" << received_seq << "\033[0m" << endl;
//        return -1; // ELAD to chage to perform estimation send back !!!
//    }
//
    if (erasure_type != 0) {
        if ((temp_seq < NUMBER_OF_ITERATIONS + T_TOT) &&
            ((erasure_simulator->is_erasure(temp_seq) == true))) {//artificial erasure
            return -1;
        }
    }

    fec_decoder->receive_message_and_symbol_wise_encode(fec_message,n,k,n2,k2,temp_size);

//    // rotate pointers
//    int lim_for_code;
//    if (temp_seq>latest_seq+n-1) {
//        // in case of a burst longer than n fill codeword_vector with zeros and store codeword_new_vector in codeword_vector_store_in_burst
//        for (int seq = latest_seq; seq < latest_seq + n ; seq++) {
//            for (int i = 0; i < n - 1; i++) {
//                memcpy(codeword_vector[i], codeword_vector[i + 1], temp_size);
//                memcpy(codeword_vector_store_in_burst[i], codeword_vector_store_in_burst[i + 1], temp_size);
//                temp_erasure_vector[i] = temp_erasure_vector[i + 1];//ELAD to check...
//            }
//            for (int i=0;i<n2-1;i++)
//                memcpy(codeword_new_vector[i], codeword_new_vector[i + 1], temp_size);
//            // zero the input
//            memset(codeword_vector[n - 1], '\000', (300 + 12) * 4 * T_TOT);//ELAD - 300=max_payload
//            temp_erasure_vector[n - 1] = 1;
//            DEBUG_MSG("\033[1;34m" << "Burst: packet dropped in (s,r) #" << seq << ": " << "\033[0m");
//            symbol_wise_encode(k, n, generator, temp_size,k2,n2); // decode past codewords
//            memcpy(codeword_vector_store_in_burst[n - 1], codeword_new_vector[n - 1], temp_size);
//
//        }
//        for (int i=0;i<n-1;i++)// zero the output (after storing)
//            memset(codeword_new_vector[i], '\000', (300 + 12) * 4 * T_TOT);//ELAD - 300=max_payload
//    }else {
//        for (int seq = latest_seq; seq < temp_seq; seq++) {// need to handle bursts longer than n
//            for (int i = 0; i < n - 1; i++) {
//                memcpy(codeword_vector[i], codeword_vector[i + 1], temp_size);
//                temp_erasure_vector[i] = temp_erasure_vector[i + 1];//ELAD to check...
//            }
//            for (int i=0;i<n2-1;i++)
//                memcpy(codeword_new_vector[i], codeword_new_vector[i + 1], temp_size);
//            memset(codeword_vector[n - 1], '\000', (300 + 12) * 4 * T_TOT);//ELAD - 300=max_payload
//            temp_erasure_vector[n - 1] = 1;
//            DEBUG_MSG("\033[1;34m" << "Packet dropped in (s,r) #" << seq << ": " << "\033[0m");
//            symbol_wise_encode(k, n, generator, temp_size,k2,n2); // decode past codewords
//        }
//    }
//
//    // push current codeword
//    for (int i = 0; i < n - 1; i++) {
//        memcpy(codeword_vector[i], codeword_vector[i + 1], temp_size);
//        temp_erasure_vector[i] = temp_erasure_vector[i + 1];//ELAD to check...
//    }
//    for (int i=0;i<n2-1;i++)
//        memcpy(codeword_new_vector[i], codeword_new_vector[i + 1], temp_size);
//    memcpy(codeword_vector[n-1],codeword,temp_size);
//    temp_erasure_vector[n-1]=0;
//
//    symbol_wise_encode(k,n,generator,temp_size,k2,n2);
//    memcpy(codeword_new_symbol_wise,codeword_new_vector[n-1],temp_size);//ELAD - to change
//
//    latest_seq = temp_seq + 1;
    return temp_seq;//ELAD to change
}

//void Application_Layer_Receiver::symbol_wise_encode(int k,int n,unsigned char *generator, int temp_size,int k2, int n2){
//    int erasure_counter=0;
//    for (int i=0;i<n;i++){
//        if (temp_erasure_vector[i]==1)
//            erasure_counter++;
//    }
//    // Extract the relevant parts from the coded message
//
//    unsigned char temp_codeword[n];
//    unsigned char temp_encoded_codeword[n2];
//    //memset(codeword_new_vector[T_INITIAL],'\000',(300 + 12) * 4 * T_INITIAL);//ELAD maxpayload
//    bool *stam_erasure_vector=(bool *) malloc(sizeof(bool)*T_TOT);
//    for (int j=0;j<100;j++) {
//        for (int i = 0; i < n; i++) {
//            // Need to add decoding
//            //codeword_new_symbol_wise[10+(j)*n+i] = codeword_vector[n -1 - i][10+(j+1)*n-(n-k) - i];
//            temp_codeword[i]=codeword_vector[i][10+j*n+i]; // temp_codeword holds the diagonal (a_0,b_1,c_2...)!!!
//        }
//        // Decoding
//        if (erasure_counter>0 && erasure_counter<(n-k+1)){
//            for (int aa=0;aa<n-1;aa++)
//                stam_erasure_vector[aa]=temp_erasure_vector[aa];
//            decodeBlock(&temp_codeword[0], generator, &temp_codeword[0], stam_erasure_vector,  k,  n,n-1,0);// T=n-1
//        }
//        for (int i=0;i<k;i++)
//            codeword_new_symbol_wise[10+(j)*n+i]=temp_codeword[k-1-i];
//    }
//    free(stam_erasure_vector);
//    // Encoding
//    for (int j=0;j<100;j++) {
//        for (int i = 0; i < k; i++) {
//            codeword_new_vector[n2-1][10+(j)*n2+i]=codeword_new_symbol_wise[10+(j)*n+i];
//        }
//    }
//    //memcpy(codeword_new_vector[n2-1],codeword_new_symbol_wise,temp_size);//ELAD - to change
//    for (int j=0;j<100;j++) {
//        for (int i = 0; i < k; i++) {
//            temp_codeword[i]=codeword_new_vector[i][10+j*n2+i]; // temp_codeword holds the diagonal (c_0,b_0,a_0...)!!!
//        }
//        memcpy(temp_encoded_codeword,temp_codeword,k*sizeof(unsigned char));
//        encodeBlock(&temp_codeword[0], generator, &temp_encoded_codeword[0], k2, n2, k2-1);
//        for (int i = k2; i < n2; i++) {
//            codeword_new_vector[i][10+j*n2+i]=temp_encoded_codeword[i];
//            //codeword_new_symbol_wise[10+j*n+i]=temp_encoded_codeword[i];
//        }
//    }
//}

int Application_Layer_Receiver::receive_message_and_symbol_wise_decode(unsigned char *udp_parameters, unsigned char *udp_codeword,
                                                                       int *udp_codeword_size, siphon::Erasure_Simulator
                                                                       *erasure_simulator) {
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

    // Perform estimation
    fec_message->set_parameters(temp_seq, int(codeword[4]), int(codeword[5]), int(codeword[6]),
                                temp_size - 8, codeword + 8);
    fec_message->counter_for_start_and_end = int(codeword[7]);

    estimator->estimate(fec_message);

    int T_value=int(codeword[4]);
    int B_value=int(codeword[5]);
    int N_value=int(codeword[6]);

    int k=T_value-N_value+1;
    int n=T_value+1;

//    if (temp_seq==0){//ELAD - need to handle the cases where packet 0 not arriving !!!
//        fec_decoder->initialize_decoder(fec_message);
//    }
//    unsigned char* generator= fec_decoder->decoder_current->decoder->getG();
//
//    if (temp_seq < latest_seq) { //if an out-of-order sequence is received, just discard it
//        //cout << "\033[1;31mOut-of-sequence packet: #" << received_seq << "\033[0m" << endl;
//        return -1; // ELAD to chage to perform estimation send back !!!
//    }

    if (erasure_type != 0) {
        if ((temp_seq < NUMBER_OF_ITERATIONS + T_TOT) &&
            ((erasure_simulator->is_erasure(temp_seq) == true))) {//artificial erasure
            return -1;
        }
    }

    fec_decoder->receive_message_and_symbol_wise_decode(fec_message,n,k,temp_size);

//    // rotate pointers
//    for (int seq = latest_seq; seq < temp_seq; seq++) {// need to handle bursts longer than n
//        for (int i = 0; i < n - 1; i++) {
//            memcpy(codeword_vector[i], codeword_vector[i + 1], temp_size);
//            temp_erasure_vector[i] = temp_erasure_vector[i + 1];//ELAD to check...
//        }
//        memset(codeword_vector[n - 1], '\000', (300 + 12) * 4 * T_TOT);//ELAD - 300=max_payload
//        temp_erasure_vector[n - 1] = 1;
//        DEBUG_MSG("\033[1;34m" << "Packet dropped in (r,d)) #" << seq << ": " << "\033[0m");
//        symbol_wise_decode(k,n,generator,seq);// decode past messages (taking erasure in (r,d) into account)
//    }
//    // push current codeword
//    for (int i = 0; i < n - 1; i++) {
//        memcpy(codeword_vector[i], codeword_vector[i + 1], temp_size);
//        temp_erasure_vector[i] = temp_erasure_vector[i + 1];//ELAD to check...
//    }
//    memcpy(codeword_vector[n-1],codeword,temp_size);
//    temp_erasure_vector[n-1]=0;
//
//    symbol_wise_decode(k,n,generator,temp_seq);
//    latest_seq = temp_seq + 1;
    return 0;//ELAD to change


}

//void Application_Layer_Receiver::symbol_wise_decode(int k,int n,unsigned char *generator,int temp_seq){
//    int erasure_counter=0;
//    for (int i=0;i<n;i++){
//        if (temp_erasure_vector[i]==1)
//            erasure_counter++;
//    }
//    // Extract the relevant parts from the coded message
//    // If no erasures:
//
//    unsigned char buffer[30000];
//    unsigned char temp_codeword[n];
//    bool *stam_erasure_vector=(bool *) malloc(sizeof(bool)*T_INITIAL);
//    for (int j=0;j<100;j++) {
//        for (int i = 0; i < n; i++) {
//            temp_codeword[n-1-i] = codeword_vector[n -1 - i][10+(j+1)*n-1 - i];// code word is [c_0,b_0,a_0,...]
//            //buffer[(j)*n+i] = codeword_vector[n -1 - i][10+(j+1)*n-(n-k) - i];
//        }
//        if (erasure_counter>0 && erasure_counter<(n-k+1)) {
//            for (int aa = 0; aa < n-1; aa++)
//                stam_erasure_vector[aa] = temp_erasure_vector[aa];
//            decodeBlock(&temp_codeword[0], generator, &temp_codeword[0], stam_erasure_vector, k, n, n-1, 0);// T=n-1
//        }
//        for (int i=0;i<n;i++){
//            buffer[(j)*n+i]=temp_codeword[n-1-i]; //code word in [X,a_0,b_0,c_0]
//        }
//    }
//    // decoding
//
//    // temp extraction of data
//    unsigned char temp_buffer[30000];
//    int ind=0;
//    for (int j=0;j<100;j++) {
//        for (int i = 0; i < k; i++) {
//            temp_buffer[ind++]=buffer[(j) * n + n-k + i];
//        }
//    }
//    // Need to add decoding...
//    DEBUG_MSG("\033[1;34m" << "Message recovered at destination from symbol-wise DF #" << temp_seq << ": " << "\033[0m");
//    printMatrix(&temp_buffer[2], 1, 300);
//}

int Application_Layer_Receiver::receive_message_and_decode(unsigned char *udp_parameters, unsigned char *udp_codeword,
                                                           int *udp_codeword_size, siphon::Erasure_Simulator
                                                           *erasure_simulator) {

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

    if (erasure_type != 0) {
        if ((temp_seq < NUMBER_OF_ITERATIONS + T_INITIAL) &&
            ((erasure_simulator->is_erasure(temp_seq) == true))) {//artificial erasure

            return -1;
        }
    }

    if (RELAYING_TYPE==0 || receiver_index==0) {

        fec_message->set_parameters(temp_seq, int(codeword[4]), int(codeword[5]), int(codeword[6]),
                                    temp_size - 8, codeword + 8);
        fec_message->counter_for_start_and_end = int(codeword[7]);

        estimator->estimate(fec_message);
        background_estimator->estimate(fec_message);

    }else{
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

    if ((temp_seq + 1) >flag_for_estimation_cycle*ESTIMATION_WINDOW_SIZE/10){
        if (RELAYING_TYPE==0 || receiver_index==0)
            cout << "Old (T,B,N) at relay receiver=" << "(" << estimator->T << "," << estimator->B_current << "," << estimator->N_current << ")" << endl;
        else
            cout << "Old (T,B,N) at destination=" << "(" << estimator->T << "," << estimator->B_current << "," << estimator->N_current << ")" << endl;
        free(estimator);
        estimator=background_estimator;
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
            temp_parameters2[i]=response_from_dest_buffer[i-6];
        if (udp_parameters == nullptr) {
            connection_manager->UDPsendResponse(temp_parameters2, 12);
            cout << "Sending 12 bytes from relay" << endl;
            printMatrix(temp_parameters2,1,12);
        }else{
            // Need to add...
            for (int i=0;i<12;i++)
                udp_parameters[i]=temp_parameters2[i];
            cout << "Sending 12 bytes from relay" << endl;
            printMatrix(udp_parameters,1,12);
        }

    } else {
        if (udp_parameters == nullptr) {
            connection_manager->UDPsendResponse(temp_parameters, 6);
            cout << "Sending 6 bytes from destination" << endl;
            printMatrix(temp_parameters, 1, 6);
        }else {
            //recommended T, B and N
            udp_parameters[0] = (unsigned char) (estimator->T);
            udp_parameters[1] = (unsigned char) (estimator->B_current);
            udp_parameters[2] = (unsigned char) (estimator->N_current);
            //acknowledgment of T, B and N
            udp_parameters[3] = (unsigned char) (fec_message->T);
            udp_parameters[4] = (unsigned char) (fec_message->B);
            udp_parameters[5] = (unsigned char) (fec_message->N);
            cout << "Sending 6 bytes from destination" << endl;
            printMatrix(udp_parameters,1,6);
        }
    }

    return received_seq;
}

void Application_Layer_Receiver::get_current_packet(unsigned char *received_data,int T,int i,int seq_number,int curr_seq_num){

    if (i>=T && fec_message->buffer==NULL){
        // Elad - this is a case the relay fails to decode the packet hence it sends all-zeros packet
        std::cout << "Elad: Erased packet"<< std::endl;
        memset(received_data,'\000',300);
//        for (int t=0;t<300;t++) {
//            received_data[t] = '\000';
//        }
        DEBUG_MSG("\033[1;34m" << "Generated dummy message at relay #" << curr_seq_num << ": " << "\033[0m");
        printMatrix(received_data, 1, 300);
    }
    else if  (i>=T && seq_number==0){
        std::memcpy(received_data,fec_messageTemp->buffer,fec_messageTemp->size*sizeof(unsigned char));
        DEBUG_MSG("\033[1;34m" << "Recovered message at relay #" << fec_messageTemp->seq_number << ": " << "\033[0m");
        printMatrix(received_data, 1, fec_messageTemp->size);
    }
    else if (i>=T){
        std::memcpy(received_data,fec_message->buffer,fec_message->size*sizeof(unsigned char));
        DEBUG_MSG("\033[1;34m" << "Recovered message at relay #" << fec_message->seq_number << ": " << "\033[0m");
        printMatrix(received_data, 1, fec_message->size);
    }
}
