/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Memory_Allocator.h
 * Author: silas
 *
 * Created on July 10, 2018, 10:06 AM
 */

#ifndef MEMORY_ALLOCATOR_H
#define MEMORY_ALLOCATOR_H

#include<cstdlib>

class Memory_Allocator {
public:
    Memory_Allocator(int number_of_buffers);
  
    virtual ~Memory_Allocator();
    
    unsigned char* allocate_memory(int size);
    
private:
    
    unsigned char **buffer;
    
    int unallocated_buffer_index;
    
    int number_of_buffers;

};

#endif /* MEMORY_ALLOCATOR_H */

