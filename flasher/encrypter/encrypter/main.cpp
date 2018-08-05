#include <iostream>
#include "aes.hpp"
#include <math.h>
#include <fstream>
#include <bitset>

using namespace std;

/* Aes key and structure */
uint8_t keyFirst[] = {0x09, 0xe0, 0xba, 0x78, 0x12, 0xc5, 0x77, 0x80, 0x08, 0xf6, 0x93, 0xaa, 0xbb, 0xad, 0x34, 0x04};
uint8_t ivFirst[] = {0x10, 0x14, 0x16, 0x64, 0x78, 0x91, 0x79, 0x05, 0x12, 0x13, 0x17, 0x07, 0x56, 0x89, 0x19, 0x08};

struct AES_ctx ctxFirst;

// name of file to search for in program directory
char filename[20];
char* outputFile = "flash.bin";

long fileSize = 0;
uint8_t* binData;

void printBin(uint8_t buf);
void exitProgram();

int main()
{
    // ask for file input
    cout<<endl<<"-- Encrypt Bin file --"<<endl;
    cout<<"Enter name of bin file: ";
    cin>>filename;
    cout<<endl;

    ifstream inFile (filename, ifstream::binary);


    if(!inFile.is_open())
    {
        cout<<"Error "<<filename<<" not found."<<endl;
        exitProgram(); return 0;
    }


    ofstream outFile(outputFile,ios::binary);

    if(!outFile.is_open())
    {
        cout<<"Error, could not create "<<outputFile<<endl;
        exitProgram(); return 0;
    }

    // initialize aes struct for decrypting bin file
    AES_init_ctx_iv(&ctxFirst, keyFirst, ivFirst);

    // print content of file and obtain file size:
    inFile.seekg (0, inFile.end);
    fileSize = inFile.tellg();
    inFile.seekg (0, inFile.beg);

    cout<<filename<<": "<< fileSize<< " bytes."<<endl;
    cout<<"Encrypting ..."<<filename<<" ..."<<endl;

    // read file encrypt and store in new file.
    while(!inFile.eof() && inFile.peek() != EOF)
    {
        uint8_t mcuData[16] = {0};
        inFile.read((char *)mcuData, 16);                   // read data from file
        //for(int h=0; h<16; h++)printBin(mcuData[h]);

        AES_CBC_encrypt_buffer(&ctxFirst, mcuData, 16);     // encrypt data

        for(int h=0; h<16; h++)
        {
            // write to new file
            outFile.put(mcuData[h]);
            //printBin(mcuData[h]);
        }
        //cout<<endl;
    }

    // close file
    inFile.close();
    outFile.close();

    cout<<"Encryption complete."<<endl<<"Created file: "<<outputFile<<endl;
    cout<<" -- Encryption Successful. -- "<<endl;


    exitProgram();
    return 0;
}


void printBin(uint8_t buf)
{
    cout << std::bitset<8>(buf)<<" ";
}

void exitProgram()
{
    cout<<"Press any key to exit."<<endl;
    std::getchar();
}
