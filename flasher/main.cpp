#include <iostream>
#include "aes.hpp"
#include <math.h>
#include <fstream>
#include <bitset>

using namespace std;

/* Aes key and structure */
uint8_t keyFirst[] = {0x09, 0xe0, 0xba, 0x78, 0x12, 0xc5, 0x77, 0x80, 0x08, 0xf6, 0x93, 0xaa, 0xbb, 0xad, 0x34, 0x04};
uint8_t ivFirst[] = {0x10, 0x14, 0x16, 0x64, 0x78, 0x91, 0x79, 0x05, 0x12, 0x13, 0x17, 0x07, 0x56, 0x89, 0x19, 0x08};
uint8_t keyMCU[] = {0x24, 0x7e, 0x15, 0x96, 0x21, 0xae, 0x02, 0xa6, 0xcb, 0xf7, 0x15, 0x88, 0x19, 0xc3, 0x4f, 0xdc};
uint8_t ivMCU[]  = {0x89, 0x90, 0xfa, 0x78, 0x12, 0x05, 0x57, 0x90, 0x08, 0xf6, 0x93, 0x16, 0x0c, 0xad, 0xea, 0x0f};

struct AES_ctx ctxFirst;
struct AES_ctx ctxMCU;

// name of file to search for in program directory
char* filename = "flash3.bin";
long fileSize = 0;
uint8_t* binData;

void printBin(uint8_t buf);

int main()
{
    // ask for file input
    cout<<endl<<"-- Custom Flashing Tool --"<<endl;
    cout<<"Searching for flash.bin..."<<endl;

    ifstream myFile (filename, ifstream::binary);
    if(!myFile.is_open())
    {
        cout<<"** Error flash.bin not found."<<endl;
        return 0;
    }

    // initialize aes struct for decrypting bin file
    AES_init_ctx_iv(&ctxFirst, keyFirst, ivFirst);
    AES_init_ctx_iv(&ctxMCU, keyMCU, ivMCU);

    // obtain file size:
    myFile.seekg (0, myFile.end);
    fileSize = myFile.tellg();
    myFile.seekg (0, myFile.beg);

    cout<<"File found."<<endl<<"Flashing...."<<endl;

    // 1. Read file
    // 2. decrypt
    // 3. encrypt with new key
    // 4. send to microcontroller
    while(!myFile.eof() && myFile.peek() != EOF)
    {
        uint8_t mcuData[16] = {0};
        myFile.read((char *)mcuData, 16);                   // read data from file

        AES_CBC_decrypt_buffer(&ctxFirst, mcuData, 16);     // decrypt data

        AES_CBC_encrypt_buffer(&ctxMCU, mcuData, 16);       // encrypt data to send to microcontroller


        for(int h=0; h<16; h++)
        {
            cout << std::bitset<8>(mcuData[h])<<" ";
        }
        cout<<endl;
        //AES_CBC_encrypt_buffer(&ctxMCU, mcuData, 16);              // encrypt data to send to microcontroller
    }

    // close file
    myFile.close();

    return 0;
}



void printBin(uint8_t buf)
{
    cout << std::bitset<8>(buf)<<" ";
}


