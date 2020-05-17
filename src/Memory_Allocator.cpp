/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Memory_Allocator.cpp
 * Author: silas
 * 
 * Created on July 10, 2018, 10:06 AM
 */
#include <cstdlib>
#include <iostream>
#include "Memory_Allocator.h"

using std::cout;
using std::endl;

Memory_Allocator::Memory_Allocator(int number_of_buffers_value) {

    number_of_buffers = number_of_buffers_value;

    buffer = (unsigned char **) malloc(number_of_buffers * sizeof(unsigned char *)); // 100 pointers to arrays

    for (int i = 0; i < number_of_buffers; i++)
        buffer[i] = (unsigned char *) malloc(33000*sizeof(unsigned char) );

    unallocated_buffer_index = 0;
}

Memory_Allocator::~Memory_Allocator() {

    for (int i = 0; i < number_of_buffers; i++) {
        if (buffer[i] != NULL)
            free(buffer[i]);
    }
    free(buffer);
}

unsigned char *Memory_Allocator::allocate_memory(int size) {

    unsigned char *temp = NULL;

    if(size>33000)
        cout<<"Cannot allocate memory of size "<< size<<endl;

    if (size > 0) {

        temp = buffer[unallocated_buffer_index];

        unallocated_buffer_index = (unallocated_buffer_index + 1) % number_of_buffers;
    }

    return temp;
}