//
// PIC.cpp: Programmable interrupt controller header
//
// These defenitions inidicate the ports and values for defferent
// PIC commands.
//
//////////////////////////////////////////////////////////////////////

#ifndef PIC_H
#define PIC_H

//////////////////////////////////////////////////////////////////////
// The following defenition may be changed to have any number of 
// interrupts from 0 to 8*sizeof(unsigned int). However, it should be 
// 8 for the 8259A.
//////////////////////////////////////////////////////////////////////
#define NUM_INTERRUPT_PINS  8
//////////////////////////////////////////////////////////////////////
// The following defentions may be changed. These determine the ports
// used by the PIC
//////////////////////////////////////////////////////////////////////
#define IS_PIC_BYTE_PORT(PORT) (bool)(PORT == 0x20 || PORT == 0x21)
//////////////////////////////////////////////////////////////////////




// PIC State Machine States
#define PIC_NO_CMD			0
#define PIC_IRR_READ_CMD	1
#define PIC_ISR_READ_CMD	2

//////////////////////////////////////////////////////////////////////
// Flags /////////////////////////////////////////////////////////////

extern bool INTR;	// This is set when the processor needs to 
					// service an interrupt. Done in simulator by 
					// calling InterruptAcknowledge().

//////////////////////////////////////////////////////////////////////
// Function prototypes ///////////////////////////////////////////////

void InitPIC(int ISRoffset);
void ResetPIC(void);
void FlagInterrupt(int IRQ);
int InterruptAcknowledge(void);
void NonspecificEOI(void);
void WriteIMR(unsigned int mask);
unsigned int ReadIMR(void);
unsigned int ReadISR(void);
unsigned int ReadIRR(void);
void PICWriteByte(unsigned short port, unsigned char value);
unsigned char PICReadByte(unsigned short port);



#endif // #ifndef PIC_H


