//
//  ConnectionManager.cpp
//  simple-UDP
//
//  Created by Salma Emara on 2019-03-08.
//  Copyright © 2019 Salma Emara. All rights reserved.
//

#include "ConnectionManager.h"
#include <string>
#include <cstring>
#include <netinet/in.h>
#include <basicOperations.h>

using std::cerr;
using std::cout;
using std::endl;

ConnectionManager::ConnectionManager(const char *Tx, const char *Rx, int flag) {
    firstTime_send = true;
    firstTime_response = true;

    //Create a socket for sending messages
    socket_created_sending = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_created_sending == -1)
        cerr << "Cannot create a socket" << endl;
    else
        cout << "Created socket for sending, descriptor = " << socket_created_sending << endl;

    //socket created for the response/acknowledgment
    socket_created_response = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_created_response == -1)
        cerr << "Cannot create a socket" << endl;
    else
        cout << "Created socket for response, descriptor = " << socket_created_response << endl;


    //Initialize sender_IP_address:
    memset((char *) &sender_IP_address, 0, sizeof(sender_IP_address));
    sender_IP_address.sin_addr.s_addr = inet_addr(Tx);
    sender_IP_address.sin_family = AF_INET;
//    sender_IP_address.sin_port = htons(1029);

    //Initialize receiver_IP_address:
    memset((char *) &receiver_IP_address, 0, sizeof(receiver_IP_address));
    receiver_IP_address.sin_addr.s_addr = inet_addr(Rx);
    receiver_IP_address.sin_family = AF_INET;
//    receiver_IP_address.sin_port = htons(1028);

    //Initialize sender_IP_address_response, here the sender will need to listen to a response
    memset((char *) &sender_IP_address_response, 0, sizeof(sender_IP_address_response));
    sender_IP_address_response.sin_addr.s_addr = inet_addr(Tx);
    sender_IP_address_response.sin_family = AF_INET;
//    sender_IP_address_response.sin_port = htons(1031);

    //Initialize receiver_IP_address_response, here the receiver will need to send a response
    memset((char *) &receiver_IP_address_response, 0, sizeof(receiver_IP_address_response));
    receiver_IP_address_response.sin_addr.s_addr = inet_addr(Rx);
    receiver_IP_address_response.sin_family = AF_INET;
//    receiver_IP_address_response.sin_port = htons(1030);

    if (flag==0){
        sender_IP_address.sin_port = htons(1029);
        receiver_IP_address.sin_port = htons(1028);
        sender_IP_address_response.sin_port = htons(1031);
        receiver_IP_address_response.sin_port = htons(1030);
    }else{
        sender_IP_address.sin_port = htons(1033);
        receiver_IP_address.sin_port = htons(1032);
        sender_IP_address_response.sin_port = htons(1035);
        receiver_IP_address_response.sin_port = htons(1034);
    }

    size_of_sender_IP_address = sizeof(sender_IP_address);
    size_of_receiver_IP_address = sizeof(receiver_IP_address);
    size_of_receiver_IP_address_response = sizeof(receiver_IP_address_response);
    size_of_sender_IP_address_response = sizeof(sender_IP_address_response);
}

ConnectionManager::~ConnectionManager() {
    close(socket_created_sending);
    close(socket_created_response);
}

void ConnectionManager::UDPsend(int message_size, unsigned char *buffer) {

    if (firstTime_send) {
        try {
            bind(socket_created_sending, (struct sockaddr *) &sender_IP_address, size_of_sender_IP_address);
        } catch (int) {
            cout << "Cannot bind" << endl;
        }
        firstTime_send = false;
    }
    //send the packet
    if (sendto(socket_created_sending, buffer, size_t(message_size), 0, (struct sockaddr *) &receiver_IP_address,
               size_of_receiver_IP_address) < 0) {
        cerr << "Cannot send" << endl;
        cerr << strerror(errno) << endl;
    } else {
        //cout << "Sent " << message_size << " bytes " << endl;
    }
}

void ConnectionManager::UDPreceive(int *received_length, unsigned char *message_buffer) {

    if (firstTime_send) {
        //Bind receiver socket
        try {
            bind(socket_created_sending, (struct sockaddr *) &receiver_IP_address, size_of_receiver_IP_address);
        } catch (int) {
            cout << "Cannot bind" << endl;
        }
        firstTime_send = false;
    }

    //now receive
//    cout << "waiting on port " << receiver_IP_address.sin_port << endl;

    //Buffer is for the data the data at the recevier
    unsigned char receiver_buffer[30000];

    long r = recvfrom(socket_created_sending, receiver_buffer, 30000, 0, (struct sockaddr *) &sender_IP_address,
                      &size_of_sender_IP_address);
//    cout<< "I may have received something with size "<<r << " bytes" <<endl;

    *received_length = (int) r;

    if (*received_length < 0) {
        cerr << "Failed to receive" << endl;
        cout << strerror(errno) << endl;
    } else {
        //Print size of data received
        //      cout << "Received " << *received_length << " bytes " << endl;

        //Changing the content of packet received
        memcpy(message_buffer, receiver_buffer, size_t(*received_length));

    }
}

void ConnectionManager::UDPsendResponse(unsigned char *notification, int size_of_notification) {

    //The response message should be set by decoder

    if (firstTime_response) {
        UDPsendResponseBind();
        firstTime_response = false;
    }
    //printMatrix(notification,1,6);
    if (sendto(socket_created_response, notification, size_t(size_of_notification), 0,
               (struct sockaddr *) &sender_IP_address_response, size_of_sender_IP_address_response) < 0) {
        cerr << "Cannot send response" << endl;
        cerr << strerror(errno) << endl;
    } else {
        //cout << "Acknowledgement sent!" << endl;
    }
}

void ConnectionManager::UDPreceiveResponse(int *feedback_size, unsigned char *feedback_message) {
    if (firstTime_response) {
        UDPreceiveResponseBind();
        firstTime_response = false;
    }

//    unsigned char response[6];
    unsigned char response[12];

    //This function is called by sender to receive response from the receiver. It is non blocking!
//    received_response = recvfrom(socket_created_response, response, 6, 0,
//                                 (struct sockaddr *) &receiver_IP_address_response,
//                                 &size_of_receiver_IP_address_response);
    received_response = recvfrom(socket_created_response, response, 12, 0,
                                 (struct sockaddr *) &receiver_IP_address_response,
                                 &size_of_receiver_IP_address_response);

    if (received_response < 0) {
        // cerr << "Failed to receive response or No response" << endl;
        *feedback_size = 0;
    } else {
        //cout << "Received " << received_response << " bytes" << endl;
        //TO DO: You may want to do something with the notification message here!
        *feedback_size = int(received_response);
        memcpy(feedback_message, response, size_t(received_response));
    }
}

void ConnectionManager::UDPreceiveResponseBind() {
    //Making the response receival non-blocking, i.e. if the reponse is not received in 2 microseconds then go to the next instruction
    struct timeval read_timeout;
    memset(&read_timeout, 0, sizeof(struct timeval));

    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = 1;
    setsockopt(socket_created_response, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout));

    //Bind sender socket
    try {
        bind(socket_created_response, (struct sockaddr *) &sender_IP_address_response,
             size_of_sender_IP_address_response);
    } catch (int) {
        cout << "Cannot bind" << endl;
    }
}

void ConnectionManager::UDPsendResponseBind() {
    //Bind receiver socket
    try {
        bind(socket_created_response, (struct sockaddr *) &receiver_IP_address_response,
             size_of_receiver_IP_address_response);
    } catch (int) {
        cout << "Cannot bind" << endl;
        cout << strerror(errno) << endl;
    }
}







