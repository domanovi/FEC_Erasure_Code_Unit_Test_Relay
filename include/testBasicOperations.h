/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   testBasicOperations.h
 * Author: silas
 *
 * Created on March 5, 2018, 1:10 PM
 */

#ifndef TESTBASICOPERATIONS_H
#define TESTBASICOPERATIONS_H

#ifdef __cplusplus
extern "C" {
#endif






#include <cstdlib>
#include <iostream>
#include <sstream>
//#include <typeinfo>

using namespace std;

void basicOperationsTest()
{
unsigned char x1, x2;
    unsigned char a, b, c;

    int i;
    
    int dummy;

    int mode =0; // 0 is addition, 1 is multiplication, 2 is division

    string response;

    const int n=3;
    
    int sizeX=n;
    int sizeY=n;
    
    unsigned char *ary = new unsigned char[sizeX*sizeY];
    
    unsigned char *out = new unsigned char[sizeX*sizeY];
    
    unsigned char *result = new unsigned char[sizeX*sizeY];

     
     for (i=0; i<n*n; i++)
     {
         ary[i]=i+1;
     }
   
    
    gf256_invert_matrix(ary, out, n);
     
    
    gf256_matrix_mul(ary, out, result, n, n, n);
    
    printMatrix(ary, n, n);
    
    printMatrix(out, n, n);
    
    printMatrix(result, n, n);

  while(1)  {


      cout<<"Choose a binary operation: 1. addition; 2. multiplication"<<endl;

      getline(cin, response);

       stringstream x0Integer(response);

        x0Integer>>dummy;

      mode=dummy;

      cout<<"You have input ";

      switch (mode){

      case 1: cout<< "1"<<endl; break;

      case 2: cout<< "2"<<endl; break;


      }

    cout << "Enter two numbers between 0 and 255 separated by a space" << endl;


        getline(cin, response);

        cout << "You have input " << response<< endl;

        stringstream x1Integer(response);


        x1Integer>>dummy;                     //convert the string into an integer between 0 and 255

        x1= (unsigned char) dummy;

        x1Integer>>dummy;                   //remove the space character

        x1Integer>>dummy;

        x2=(unsigned char) dummy;


         switch (mode){

      case 1: a=gf256_add(x1, x2); break;

      case 2: a=gf256_mul(x1, x2); break;


      }


     cout <<"The answer is " << unsigned(a)<< endl;

      

  }
    
    delete [] ary;
    delete [] out;
    delete [] result;
    
 return;   
}


void basicOperationsTest_rref()
{
    int i,j;
    
    int k=3;
    
    int n=15;
    
    
    unsigned char *in = new unsigned char[k*n];
    
    unsigned char *out = new unsigned char[k*n];
    
     unsigned char *result = new unsigned char[k*n];
    
    unsigned char *action = new unsigned char[n*n];

     
     for (i=0; i<k*n; i++)
     {
         in[i]=(unsigned char) i;
         
         
     }
    
    for (j=0; j<k; j++)
             in[j*n]=0x0;
    
     for (j=0; j<k; j++)
             in[j*n+2]=0x0;
   
    
    gf256_rref_matrix(in, out, action, k, n);
     
    cout<< "Input matrix:" << endl;
    printMatrix(in, k, n);
    
    cout<< "Output matrix:" << endl;
    printMatrix(out, k, n);
    
    cout<< "Action matrix:" << endl;
    printMatrix(action, n, n);

  
    gf256_matrix_mul(in, action, result, k, n, n);
     cout<< "Resultant matrix:" << endl;
    printMatrix(result, k, n);
    
    delete [] in;
    delete [] out;
    delete [] action;
    delete [] result;
    
 return;   
}



void testForOptimality(){
        
        unsigned char *G;
        
        
        
        int T, B, N, k, n;
        
        for (T=1; T<13; T++){
            
            cout<<"T="<<T<<endl;
            
            for (B=1; B<=T; B++){
                
                for (N=1; N<=B; N++){
                   
                    
                    k=T-N+1; 
                    
                    n=k+B;
                    
                    G=(unsigned char *) malloc(k*n*sizeof(unsigned char));
        
                    if(init_at_sender(T, B, N, G, k, n)==0) 
                        cout<< "(T,B,N)=("<<T<<","<<B<<","<<N<<") is not optimal"<<endl;
                
                    free(G);
                }
        
            }
        
        }
}
    




#ifdef __cplusplus
}
#endif

#endif /* TESTBASICOPERATIONS_H */
