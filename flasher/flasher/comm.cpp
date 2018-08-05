#include "stdafx.h"

HANDLE hComm;

// Function to detect available com ports 
void DetectComPorts(std::vector< tstring >& ports, size_t upperLimit = 128)
 {
	 for (size_t i = 1; i <= upperLimit; i++)
	 {
		 TCHAR strPort[32] = { 0 };
		 _stprintf(strPort, _T("COM%d"), i);

		 DWORD dwSize = 0;
		 LPCOMMCONFIG lpCC = (LPCOMMCONFIG) new BYTE[1];
		 BOOL ret = GetDefaultCommConfig(strPort, lpCC, &dwSize);
		 delete[] lpCC;

		 lpCC = (LPCOMMCONFIG) new BYTE[dwSize];
		 ret = GetDefaultCommConfig(strPort, lpCC, &dwSize);
		 delete[] lpCC;

		 if (ret) ports.push_back(strPort);
	 }
 }



// Initialize serial communication
uint8_t  initCommunication(LPCWSTR cport)
{
	// initializing serial communication 
	hComm = CreateFile(cport,						    // port name
		GENERIC_READ | GENERIC_WRITE,					// Read/Write
		0,											    // No Sharing
		NULL,											// No Security
		OPEN_EXISTING,								    // Open existing port only
		0,											    // Non Overlapped I/O
		NULL);										    // Null for Comm Devices

	// return result if port was opened successfully 
	if (hComm == INVALID_HANDLE_VALUE)
		return 0;
		
	// configure serial comm if opened successfully
	DCB dcbSerialParams = { 0 }; // Initializing DCB structure
	GetCommState(hComm, &dcbSerialParams);
	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
	dcbSerialParams.BaudRate = CBR_115200;  // Setting BaudRate = 9600
	dcbSerialParams.ByteSize = 8;         // Setting ByteSize = 8
	dcbSerialParams.StopBits = ONESTOPBIT;// Setting StopBits = 1
	dcbSerialParams.Parity = NOPARITY;  // Setting Parity = None
	dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;

	BOOL status = SetCommState(hComm, &dcbSerialParams);

	COMMTIMEOUTS timeouts = { 0 };

	timeouts.ReadIntervalTimeout = 200;
	timeouts.ReadTotalTimeoutConstant = 200;
	timeouts.ReadTotalTimeoutMultiplier = 200;
	timeouts.WriteTotalTimeoutConstant = 50;
	timeouts.WriteTotalTimeoutMultiplier = 10;

	SetCommTimeouts(hComm, &timeouts);

	if (!status) return 0;

	status = SetCommMask(hComm, EV_RXCHAR);

	return(1);
}


// write to Serial port 
void commWrite(LPCVOID buf, DWORD length, LPDWORD noWritten)
{
	BOOL Status = WriteFile(hComm,        // Handle to the Serial port
				buf,					 // Data to be written to the port
				length,					 //No of bytes to write
				noWritten,				 //Bytes written
				NULL);

	if (!Status)std::cout << "Write error" << std::endl;
}



// close serial port 
void closeComm()
{
	CloseHandle(hComm);				//Closing the Serial Port
}



// generate custom byte sequence to serve as connection byte, 
// Note: You can edit this to match any sequence of your choice, as far as it
//		 corresponds with what is in the bootloader.
void genConnectBuf(uint8_t* buf, int len)
{
	buf[0] = rand() % 235;		// makes sure the members of array are always 8bit, even after adding 16

	for (int i = 1; i < len; i++)
	{
		buf[i] = buf[i - 1] + 1;
	}
}



// read message from Serial communication
int commRead(uint8_t* buf, int length)
{
	DWORD bytesRead;
	long count = 0;
	LPDWORD dwEventMask = 0;
	LPDWORD Err = 0;
	COMSTAT CST;
	
	//DWORD dwEventMask;
	BOOL Status = WaitCommEvent(hComm, dwEventMask, NULL);

	BOOL ans = ReadFile(hComm,         //Handle of the Serial port
				 buf,			      //Temporary character
				 length,		      //Size of TempChar
				 &bytesRead,		  //Number of bytes read
				 NULL);

	return bytesRead;
}



// check custom reply which we expect from microcontroller, 
// This can also be anything as far as it corresponds with what the bootloader has
// been configured to do.
bool isNextReply(uint8_t *buf, int len)
{
	for (uint8_t i = 0; i < len; i++)
	{
		if (buf[i] != 0x44)
		{
			std::cout << "wrong message" << std::endl;
			return 0;
		}
	}

	return 1;
}



// generate custom byte array to signify end of connection
// This can also be anything as far as it corresponds with what the bootloader has
// been configured to do.
void getEndData(uint8_t* buffer, int len)
{
	
	buffer[0] = rand() % 100 + 20;

	// decreasing order of elements in array 
	for (int i = 1; i < len; i++)
	{
		buffer[i] = buffer[i - 1] - 1;
	}
}



// custom checksum :)
void getCheckSum(uint8_t* buffer, int len)
{
	for (int i = 0; i < len; i++)
	{
		checkSum = (checkSum + (int)buffer[i]) % 0xffff;
	}
}


// check if this is the end data from microcontroller
// This can also be anything as far as it corresponds with what the bootloader has
// been configured to do.
uint8_t isLastReply(uint8_t* buffer, int len)
{
	// check the last 14 bytes 
	for (int i = 2; i < len; i++)
	{
		if (buffer[i] != 0x55) return 0;
	}
	
	return 1;
}