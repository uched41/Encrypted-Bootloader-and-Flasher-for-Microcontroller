// flasher.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

/* comment next line, if you dont want to encrypt communication */
#define ENCRYPT 1

/* Set PRINT_DEBUG to 1 to print more debug messages */
#define PRINT_DEBUG 0



using namespace std;

#ifdef ENCRYPT

	/* Aes key and structure */
	// first aes key and IV are for decrypting bin file, This must match with what the bin file was encrypted with
	uint8_t keyFirst[] = { 0x69, 0xe0, 0xad, 0x98, 0x12, 0xc5, 0x37, 0x80, 0x08, 0xf6, 0x93, 0xba, 0xbb, 0xad, 0x22, 0x11 };
	uint8_t ivFirst[] = { 0x89, 0x34, 0x56, 0x94, 0x08, 0x91, 0xa9, 0xb5, 0x1c, 0x14, 0x47, 0x07, 0x45, 0x89, 0x29, 0x08 };

	// this second key and IV must match with what is in the bootloader
	uint8_t keyMCU[] = { 0x24, 0x7e, 0x05, 0x96, 0x21, 0xae, 0x02, 0xa6, 0xcb, 0xf7, 0x15, 0x33, 0x19, 0xc3, 0x05, 0xde };
	uint8_t ivMCU[] = { 0x29, 0x90, 0x22, 0x78, 0x12, 0x05, 0x57, 0x09, 0x08, 0xf6, 0x45, 0x01, 0x0c, 0xad, 0xea, 0x22 };

	struct AES_ctx ctxFirst;
	struct AES_ctx ctxMCU;

#endif


/*
	Hard coded name of file to search for, edit this to match filename to be provided to customer
	Or you can add functionality to ask user for filename
*/
char* filename = (char*)"flash.bin";		

long fileSize = 0;
uint8_t* binData;
int selectedPort;
uint32_t checkSum = 0;
void printBin(uint8_t buf);

void exitProgram()
{
	cout << "Press any key to exit" << endl;
	std::getchar();
}

int main()
{
	cout << endl << "-- Custom Flashing Tool --" << endl;
	cout << "Searching for flash.bin ..." << endl;
	

	// attempt to open file 
	ifstream myFile(filename, ifstream::binary);
	if (!myFile.is_open())
	{
		cout << "** Error flash.bin not found." << endl;
		exitProgram();
		return 0;
	}
	cout << "Flash.bin found." << endl;


	// ask for target port 
	cout << "Searching for available ports ..." << endl;
	std::vector< tstring > ports;

	// detect available ports
	DetectComPorts(ports, 128);				

	// print available ports 
	cout << "Available Com Ports: " << ports.size() << endl;
	for (std::vector< tstring >::const_iterator it = ports.begin(); it != ports.end(); ++it)
	{
		wcout <<" "<<it-ports.begin() << " - "<< *it << endl;
	}
	cout<< endl<< "Enter index of target port: ";

	// ask for user to select port 
	cin >> selectedPort;


	// open selected port
	cout << endl << "Opening port ";
	wcout << ports[selectedPort] << endl; 
	if (!initCommunication(ports[selectedPort].c_str()))			// if opening port failed 
	{
		cout << "Error Opening Selected Serial Port." << endl;
		exitProgram();
		return 0;
	}
	else cout << "Port Initialization success ..." << endl;


	// initialize aes struct for decrypting bin file
	AES_init_ctx_iv(&ctxFirst, keyFirst, ivFirst);			// first aes structure is used to decrypt the encrypted bin data
	AES_init_ctx_iv(&ctxMCU, keyMCU, ivMCU);				// second aes structure is to encrypt again before sending to microcontroller


	// obtain file size:
	myFile.seekg(0, myFile.end);
	fileSize = myFile.tellg();
	myFile.seekg(0, myFile.beg);


	// first of all we send the connect string and wait for the device to respond
	LPDWORD bytesWritten = NULL;
	LPDWORD numRead = NULL;
	uint8_t conData[MSG_LENGTH] = { 0 };
	uint8_t readBuf[MSG_LENGTH] = { 0 };
	cout << "Attempting to connect ..." << endl;
	while (1)
	{
		genConnectBuf(conData, MSG_LENGTH);										// generate connection string to be sent to mcu
		AES_CBC_encrypt_buffer(&ctxMCU, conData, MSG_LENGTH);					// encrypt connection string to send to microcontroller
		commWrite((void*)conData, (DWORD)MSG_LENGTH, bytesWritten);				// write data to uart

		// Wait for reply from microcontroller 
		if (commRead(readBuf, MSG_LENGTH) > 0)		
		{
			// print reply
			if (PRINT_DEBUG)
			{
				for (int h = 0; h<MSG_LENGTH; h++) cout << std::bitset<8>(readBuf[h]) << " ";
				cout << endl;
			}

			// break out of loop if this is the reply we are expecting 
			if (isNextReply(readBuf, MSG_LENGTH)) break;
		}
		Sleep(200);		// delay
	}
	cout << "Flashing ..." << endl;

	/*
	 After we have confirmed sync with microcontroller, we start sending binary data in the following steps:
	 (1) Read file 
	 (2) decrypt 
	 (3) encrypt with new key 
	 (4) send to microcontroller
	 (5) wait for microcontroller to reply that it has received this data
	 */

	// loop through whole data 
	while (!myFile.eof() && myFile.peek() != EOF)
	{
		// read data from file
		uint8_t mcuData[MSG_LENGTH] = { 0 };
		myFile.read((char *)mcuData, MSG_LENGTH);									 

		// decrypt bin data which we read from bin file
		AES_CBC_decrypt_buffer(&ctxFirst, mcuData, MSG_LENGTH);						 

		if (PRINT_DEBUG)
		{
			for (int h = 0; h<MSG_LENGTH; h++) cout << std::bitset<8>(mcuData[h]) << " ";
			cout << endl;
		}

		// calculate custom checksum which will be checked after last byte is sent
		getCheckSum(mcuData, MSG_LENGTH);											 

		// encrypt data with new key before sending to microcontroller 
		AES_CBC_encrypt_buffer(&ctxMCU, mcuData, MSG_LENGTH);					     

		// send data through uart 
		commWrite((void*)mcuData, (DWORD)MSG_LENGTH, bytesWritten);				     

		// wait for microcontroller to confirm reception of the message sent
		// Note: we can add functionality to get checksum for each message sent
		while(1)
		{
			uint8_t readBuf2[MSG_LENGTH] = { 0 };								

			// get data from com port 
			if (commRead(readBuf2, MSG_LENGTH) > 0)
			{
				// check if this is the expected data
				if (isNextReply(readBuf2, MSG_LENGTH)) break;
			}		
		}
		
	}

	// generate message to signify end of transmission
	getEndData(conData, MSG_LENGTH);
	AES_CBC_encrypt_buffer(&ctxMCU, conData, MSG_LENGTH);					 // encrypt data to send to microcontroller
	commWrite((void*)conData, (DWORD)MSG_LENGTH, bytesWritten);				// write data to uart


	// wait for reply, which is contains the checksum of the whole data sent
	while(1)
	{
		if (commRead(readBuf, MSG_LENGTH))
		{
			if (isLastReply(readBuf, MSG_LENGTH)) break;
			if (PRINT_DEBUG)
			{
				for (int h = 0; h<MSG_LENGTH; h++) cout << std::bitset<8>(readBuf[h]) << " ";
				cout << endl;
			}
		}
	}


	// check if checksum is correct 
	checkSum = checkSum%255;
	uint8_t tempCheckSum = readBuf[0];
	cout << checkSum << " " << tempCheckSum << endl;
	if (tempCheckSum == checkSum)
	{
		cout << "Flashing Complete." << endl;
	}
	else cout << "Error occured" << endl;

	// close file
	myFile.close();
	closeComm();

	exitProgram();
	return 0;
}





void printBin(uint8_t buf)
{
	cout << std::bitset<8>(buf) << " ";
}

