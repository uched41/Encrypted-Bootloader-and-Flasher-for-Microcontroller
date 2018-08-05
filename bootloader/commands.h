#ifndef _COMMANDS_H
#define _COMMANDS_H

#include <stdio.h>
#include "M451Series.h"

#define MSG_LENGTH 16 

// commands to send to PC
#define NEXT    (uint8_t)2
#define RESEND  (uint8_t)3
#define END 		(uint8_t)4

typedef enum dtypes
{
	dataNext = 	0x01,
	dataError   =   0x03,
	dataEnd     = 	0x20				// sent when last data has been received 
}dTypes;

extern uint16_t addrHead;		// first 2 bytes of address 


void EraseAP(unsigned int addr_start, unsigned int addr_end);

// checks if the calculated checksum is equal to the sent checksum
uint8_t isCheckSum(uint8_t *buf, int len, uint8_t checksum);

/* Function that checks if the data received is the start data */
uint8_t isStartData(uint8_t* _data);

/* Function to parse received data and act according ly */
dTypes parseData(uint8_t* _data);

extern uint32_t checkSum;

// target device functions
extern uint32_t g_apromSize, g_dataFlashAddr, g_dataFlashSize;
extern void GetDataFlashInfo(uint32_t *addr, uint32_t *size);
extern uint32_t GetApromSize(void);

#endif

