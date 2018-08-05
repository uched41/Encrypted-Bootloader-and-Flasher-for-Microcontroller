#include <stdio.h>
#include "M451Series.h"
#include "aes.h"
#include "commands.h"

/*
	Tests to run :
	1. Quasi pin for input -> tested works ..
*/


#define BRANCH_PIN_PORT PB
#define BRANCH_PIN 10
#define BRANCH_PIN_CMD PB10		// This is used to read this pin, e.g state = PB10

#define PLLCTL_SETTING  CLK_PLLCTL_72MHz_HIRC
#define PLL_CLOCK       71884880

#define	V6M_AIRCR_VECTKEY_DATA		0x05FA0000UL
#define V6M_AIRCR_SYSRESETREQ		0x00000004UL

/* Global Variables */
uint8_t volatile bufhead = 0;								// buffer pointer 
uint8_t volatile bUartDataReady = 0;				// flag that is set when buffer is full
__align(4) uint8_t  uart_rcvbuf[MSG_LENGTH];				// 32 byte buffer to receive data from usart

/* Aes key and structure */
uint8_t keyMCU[] = { 0x24, 0x7e, 0x15, 0x96, 0x21, 0xae, 0x02, 0xa6, 0xcb, 0xf7, 0x15, 0x88, 0x19, 0xc3, 0x4f, 0xdc };
uint8_t ivMCU[] = { 0x89, 0x90, 0xfa, 0x78, 0x12, 0x05, 0x57, 0x90, 0x08, 0xf6, 0x93, 0x16, 0x0c, 0xad, 0xea, 0x0f };

uint8_t nextMsg[MSG_LENGTH];
uint8_t endMsg[MSG_LENGTH];

/* sys_tick interrupt that will serve as system time */
volatile uint32_t msTicks = 0;
void SysTick_Handler(void)
{
	msTicks++;
}

/* Initialize system clock */
static void SYS_Init(void)
{

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Enable HIRC clock (Internal RC 22.1184MHz) */
    CLK->PWRCTL |= (CLK_PWRCTL_HIRCEN_Msk | CLK_PWRCTL_HXTEN_Msk);

    /* Wait for HIRC clock ready */
    while(!(CLK->STATUS & CLK_STATUS_HIRCSTB_Msk));

    /* Select HCLK clock source as HIRC and and HCLK clock divider as 1 */
    //CLK->CLKSEL0 &= ~CLK_CLKSEL0_HCLKSEL_Msk;
    //CLK->CLKSEL0 |= CLK_CLKSEL0_HCLKSEL_HIRC;
    CLK->CLKSEL0 = (CLK->CLKSEL0 & (~CLK_CLKSEL0_HCLKSEL_Msk)) | CLK_CLKSEL0_HCLKSEL_HIRC;
    CLK->CLKDIV0 &= ~CLK_CLKDIV0_HCLKDIV_Msk;
    CLK->CLKDIV0 |= CLK_CLKDIV0_HCLK(1);

    /* Set PLL to Power-down mode */
    CLK->PLLCTL |= CLK_PLLCTL_PD_Msk;

    /* Set core clock as PLL_CLOCK from PLL */
    CLK->PLLCTL = PLLCTL_SETTING;
    while(!(CLK->STATUS & CLK_STATUS_PLLSTB_Msk));
    //CLK->CLKSEL0 &= (~CLK_CLKSEL0_HCLKSEL_Msk);
    //CLK->CLKSEL0 |= CLK_CLKSEL0_HCLKSEL_PLL;
    CLK->CLKSEL0 = (CLK->CLKSEL0 & (~CLK_CLKSEL0_HCLKSEL_Msk)) | CLK_CLKSEL0_HCLKSEL_PLL;

    /* Update System Core Clock */
    PllClock        = PLL_CLOCK;            // PLL
    SystemCoreClock = PLL_CLOCK / 1;        // HCLK
    CyclesPerUs     = PLL_CLOCK / 1000000;  // For CLK_SysTickDelay()

    /* Enable UART module clock */
    CLK->APBCLK0 |= CLK_APBCLK0_UART0CKEN_Msk;

    /* Select UART module clock source as HIRC and UART module clock divider as 1 */
    CLK->CLKSEL1 &= ~CLK_CLKSEL1_UARTSEL_Msk;
    CLK->CLKSEL1 |= CLK_CLKSEL1_UARTSEL_HIRC;
    CLK->CLKDIV0 &= ~CLK_CLKDIV0_UARTDIV_Msk;
    CLK->CLKDIV0 |= CLK_CLKDIV0_UART(1);

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Set PD multi-function pins for UART0 RXD(PD.0) and TXD(PD.1) */
    SYS->GPD_MFPL &= ~(SYS_GPD_MFPL_PD0MFP_Msk | SYS_GPD_MFPL_PD1MFP_Msk);
    SYS->GPD_MFPL |= (SYS_GPD_MFPL_PD0MFP_UART0_RXD | SYS_GPD_MFPL_PD1MFP_UART0_TXD);

}


/* Interrupt handler of Uart*/
void UART0_IRQHandler(void)
{
    /*----- Determine interrupt source -----*/
    uint32_t u32IntSrc = UART0->INTSTS;

    if(u32IntSrc & 0x11) { //RDA FIFO interrupt & RDA timeout interrupt
        while(((UART0->FIFOSTS & UART_FIFOSTS_RXEMPTY_Msk) == 0) && (bufhead < MSG_LENGTH))	//RX fifo not empty
            uart_rcvbuf[bufhead++] = UART0->DAT;
    }

    if(bufhead == MSG_LENGTH) {
        bUartDataReady = TRUE;
        bufhead = 0;
    } else if(u32IntSrc & 0x10) {
        bufhead = 0;
    }
}


static void UART0_Init()
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init UART                                                                                               */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Reset UART module */
    SYS->IPRST1 |=  SYS_IPRST1_UART0RST_Msk;
    SYS->IPRST1 &= ~SYS_IPRST1_UART0RST_Msk;

    /* Configure UART0 and set UART0 baud rate */
    UART0->BAUD = UART_BAUD_MODE2 | UART_BAUD_MODE2_DIVIDER(__HIRC, 115200);
    UART0->LINE = UART_WORD_LEN_8 | UART_PARITY_NONE | UART_STOP_BIT_1;

    /* Set RTS Trigger Level as 14 bytes */
    UART0->FIFO = (UART0->FIFO & (~UART_FIFO_RTSTRGLV_Msk)) | UART_FIFO_RTSTRGLV_14BYTES;

    /* Set RX Trigger Level as 14 bytes */
    UART0->FIFO = (UART0->FIFO & (~UART_FIFO_RFITL_Msk)) | UART_FIFO_RFITL_14BYTES;

    /* Enable RDA\RTO Interrupt */
    UART0->INTEN |= (UART_INTEN_RDAIEN_Msk | UART_INTEN_RXTOIEN_Msk);

    /* Set Timeout time 0x3E bit-time and time-out counter enable */
    UART0->TOUT = (UART0->TOUT & ~UART_TOUT_TOIC_Msk) | (0x3E);
    UART0->INTEN |= UART_INTEN_TOCNTEN_Msk;

    /* Enable UART0 interrupt */
    NVIC_EnableIRQ(UART0_IRQn);
}





/* Function that checks the gpio pin and reset vector to determine whether to branch
	 If a low value is returned, we branch to bootloader */
static uint8_t toApplication(void )
{
	// write quasi pin as 1 to enable us read it see: 
	//https://forum.allaboutcircuits.com/threads/on-the-8051-what-is-a-quasi-bi-directional-port.102601/
	BRANCH_PIN_CMD = 1;
	
	// wait 4 clock cycles 
	// see page 359 http://www.nuvoton.com/resource-files/TRM_M451_Series_EN_Rev2.08.pdf 
	__nop(); __nop(); __nop(); __nop();		
	
	// return the value of the pin 
	return BRANCH_PIN_CMD;
}



/* Function that jumps to Application program */
static void startApplication(void)
{
	outpw(&SYS->RSTSTS, 3);//clear bit
	
	// set next booting source as APROM
	outpw(&FMC->ISPCTL, inpw(&FMC->ISPCTL) & 0xFFFFFFFC);		
	
	// request reset 
	outpw(&SCB->AIRCR, (V6M_AIRCR_VECTKEY_DATA | V6M_AIRCR_SYSRESETREQ));

	/* Trap the CPU to trigger reset  */
	while(1);
}




/*Function to reply the PC */
static void replyPC(uint8_t* msg)
{
	// wait until the outgoing buffer is empty
	int i = 0;
	for(i=0; i<MSG_LENGTH; i++)
	{
		//while((UART0->FIFOSTS & UART_FIFOSTS_TXEMPTY_Msk) == 0)	//RX fifo not empty
		UART0->DAT = msg[i]; CLK_SysTickDelay(10);
	}
	/*** Update needed add some kind of check mechanisms to verify integrity */
	// maybe send inverse of send data or send data twice 
}







/* save space by generating buffers dynamically */
void generateBuffers(void)
{
	int i;
	for(i=0; i<MSG_LENGTH; i++)
	{
		nextMsg[i] = 0x44;
		endMsg[i] = 0x55;
	}
}

/* Main Program entry point */
int main(void)
{
	struct AES_ctx ctxMCU;
	generateBuffers();
	
	/* Unlock protected registers and setup system clock*/
	SYS_UnlockReg();
	WDT->CTL &= ~WDT_CTL_WDTEN_Msk;		// *** questionable, watchdog timer is being disabled review later.******
	SYS_Init();
	
	// Enable Isp commands 
	FMC->ISPCTL |= FMC_ISPCTL_ISPEN_Msk;	// (1ul << 0)
	
	/* Configure SysTick to generate an interrupt every millisecond */
	if(SysTick_Config(SystemCoreClock / 1000))
	{
		// returns 1, there is an error. so we configure another time source
		
		/******** UPDATE *********/
	}		
  
	
	/* Configure PC.9 as Output mode */
   //GPIO_SetMode(BRANCH_PIN_PORT, BRANCH_PIN, GPIO_MODE_QUASI);
	BRANCH_PIN_PORT->MODE |= (0b0011 << (BRANCH_PIN*2));
	
	// check Branch pin Status and branch accordingly
	if(toApplication())
	{
		// set next boot source to APROM and reset
		startApplication();
	}
	
	
	
	//--------  Continue with Bootloader and wait for data -----//
	
	/* Init UART0 for Communication */
  UART0_Init();
	
	g_apromSize = GetApromSize();
  GetDataFlashInfo(&g_dataFlashAddr , &g_dataFlashSize);
	
	/* Initialize AES */
	AES_init_ctx_iv(&ctxMCU, keyMCU, ivMCU);
	
	/* wait for the first command which contains the first 2 bytes of address */
	while(1)
	{
		// If MSG_LENGTH buffer is full 
		if(bUartDataReady)
		{
			// first we decrypt the data
			AES_CBC_decrypt_buffer(&ctxMCU, uart_rcvbuf, MSG_LENGTH);		
			
			bUartDataReady = FALSE;
			
			// exit first loop if we have received the first data 
			if(isStartData(uart_rcvbuf)) break;
		}
	}
	
	// ask for next data from PC 
	replyPC(nextMsg);
	
	// now we can start receiving datablocks after we have received the first block
	dTypes ans;
	
	while(1)
	{
		// If 32byte buffer is full 
		if(bUartDataReady)
		{
			// first we decrypt the data
			AES_CBC_decrypt_buffer(&ctxMCU, uart_rcvbuf, MSG_LENGTH);		
			
			// parse the received data, exit if we have received the last data 
			ans = parseData(uart_rcvbuf);
			
			if(ans == dataEnd)break;
			
			replyPC(nextMsg);
			
			//else if(ans 
			bUartDataReady = FALSE;
		}
	}
	
	checkSum = checkSum%255;
	endMsg[0] = (uint8_t)(checkSum );
	
	msTicks = 0;
	
	// send data a couple of times because pc is missing first one
	replyPC(endMsg); CLK_SysTickDelay(400000);	replyPC(endMsg);	
	
	// set next boot source to APROM and reset
	startApplication();
	while(1);
}


