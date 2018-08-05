#include "commands.h"
#include "fmcUser.h"

uint16_t addrHead = 0;
uint32_t apromCounter = FMC_APROM_BASE;		// counter for flash address 
uint32_t g_apromSize, g_dataFlashAddr, g_dataFlashSize;
uint32_t checkSum = 0;

static void Checksum(uint8_t *buf, int len)
{
	  int i;
    for( i = 0 ; i < len; i++) 
		{
       checkSum = (checkSum+(int)buf[i])%0xffff;
    }
}


//bAprom == TRUE erase all aprom besides data flash
void EraseAP(unsigned int addr_start, unsigned int addr_end)
{
    unsigned int eraseLoop = addr_start;

    for(; eraseLoop < addr_end; eraseLoop += FMC_FLASH_PAGE_SIZE ) {
        FMC_Erase_User(eraseLoop);
    }
    return;
}



// checks if this is the first data we should be receiving 
uint8_t isStartData(uint8_t* _data)
{
	/* We will implement a simple check to check our start data */
	uint8_t i;
	for(i=0; i<MSG_LENGTH-1; i++)
	{
		//each next element will be 1 greater than the previous
		if(_data[i] != (_data[i+1]-1)) return 0;
	}
	
	// erase all flash
	EraseAP(FMC_APROM_BASE, g_dataFlashAddr); // erase APROM
	
	return 1;
}


// checks if this is the end data 
static uint8_t isEndData(uint8_t* _data)
{
	/* We will implement a simple check to check our start data */
	uint8_t i;
	for(i=0; i<MSG_LENGTH-1; i++)
	{
		// decreasing other of numbers 
		if(_data[i] != (_data[i+1]+1 )) return 0;
	}
	
	return 1;
}


/* Function to parse received data and act accordingly */
dTypes parseData(uint8_t* _data)
{
	// check if we have be received data that signifies dataend 
	if(isEndData(_data)) return dataEnd;
	
	Checksum(_data, MSG_LENGTH);
	
	// write data to flash 
	WriteData(apromCounter, apromCounter+MSG_LENGTH, (uint32_t*)_data);
	
	apromCounter+=MSG_LENGTH;	// increment apromcounter 
	
	return dataNext;
}


