#pragma once
#ifndef COMMS_H
#define COMMS_H

#include "stdafx.h"

#ifdef _UNICODE
#define tstring std::wstring
#else
#define tstring std::string
#endif 

/*
	length of each message in bytes.
	Note: This should be a multiple of 16, the aes library requires this
	*/
#define MSG_LENGTH 16

extern HANDLE hComm;
extern int selectedPort;
extern uint32_t checkSum;


// functions

uint8_t initCommunication(LPCWSTR cport);										// initialize COM port

void genConnectBuf(uint8_t* buf, int len);										// generate buffer to be sent for initial connection

void commWrite(LPCVOID buf, DWORD length, LPDWORD noWritten);					// write to COM port

int commRead(uint8_t* buf, int length);											// read from COM port

bool isNextReply(uint8_t *buf, int len);										// check is data if correct reply from Mcu

void getEndData(uint8_t* buffer, int len);										// generate buffer to signify end of connection

void getCheckSum(uint8_t* buffer, int len);										// get checksum and add to global variable

uint8_t isLastReply(uint8_t* buffer, int len);									// check is this is the last reply expected from Mcu

void closeComm();																// close COM ports 

void DetectComPorts(std::vector< tstring >& ports, size_t upperLimit );			// detect availale COM ports 

#endif 