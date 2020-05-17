//
//  ConnectionManager.hpp
//  simple-UDP
//
//  Created by Salma Emara on 2019-03-08.
//  Copyright © 2019 Salma Emara. All rights reserved.
//

#ifndef ConnectionManager_h
#define ConnectionManager_h

#include <ctime>
#include <iostream>
#include <errno.h>
#include <sys/socket.h>  //available in all linux systems
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>

class ConnectionManager {
public:
    //This is the constructor. It initializes the IP addresses and port numbers for the sending of messages and receival of responses 
    ConnectionManager(const char *, const char *,int flag);
    ~ConnectionManager();

    //this function will be called by encoder. The encoder should send the size of the message and a pointer to the message to be sent
    void UDPsend(int, unsigned char *);

    //this function should be called by the receiver. It will call a function to decode the received message
    void UDPreceive(int *, unsigned char *);

    //Send respose will be called whenever a packet has been received. The notification message is to be passed.
    void UDPsendResponse(unsigned char *, int);

    //Receive response is non-blocking. It has a timeout of 1 microsecond. It is check periodically for acknowledgments whenever something is being sent
    void UDPreceiveResponse(int *feedback_size, unsigned char *feedback_message);

//Bindings are there to bind the socket to the port number
    void UDPsendResponseBind();

    void UDPreceiveResponseBind();

private:
    const double MILLION = 1000000;

    //IP addresses and port numbers of senders and receivers
    struct sockaddr_in sender_IP_address;
    struct sockaddr_in receiver_IP_address;

    //IP addresses and port numbers of senders receiving response and receivers sending acknowledgments
    struct sockaddr_in sender_IP_address_response;
    struct sockaddr_in receiver_IP_address_response;

    int socket_created_sending;
    int socket_created_response;

    //Size of what is received by response function
    long received_response;

    socklen_t size_of_sender_IP_address;
    socklen_t size_of_receiver_IP_address;
    socklen_t size_of_sender_IP_address_response;
    socklen_t size_of_receiver_IP_address_response;

    //Will be used to bind sockets when UDP send or receive is called
    bool firstTime_send, firstTime_response;
};


#endif /* ConnectionManager_h */
