//
// PIC.cpp: Programmable Interrupt Controller emulator
//
// The SciTech simulator does not support the functionality of the
// 8259A interrupt controller. This module contains the code to
// emulate certain aspects of this support chip in fully nested mode.
//
// This model mimics the behavior of the 8259A at the hardware level
// so understanding its operation may help you understand this code.
//
// This module is intnded to be associated with the SciTech code and
// not so much with the simulator classes.
//
//////////////////////////////////////////////////////////////////////

#include "PIC.h"
#include "x86emu/x86emui.h"


#define CLEAR_BIT(VAR, B_NUM)	((VAR) &= ~(1 << (B_NUM)))
#define SET_BIT(VAR, B_NUM)		((VAR) |= (1 << (B_NUM)))


// Global Variables //////////////////////////////////////////////////

bool INTR;		// Equivalent to INT pin on 8259A. When set INTR pin is being asserted on processor.


// Static Globals ////////////////////////////////////////////////////

static unsigned IRR;		// Interrupt Requrest Register
static unsigned ISR;		// In-Service Regsiter
static unsigned IMR;		// Interrupt Mask Register (A 0 bit means enabled, a 1 bit means disabled)
static int offset;			// Offset to combine with IRQ numbers
							//   to obtain the ISR vector number.
static int PICstate = PIC_NO_CMD;	// Current internal PIC state, needed for IRR and ISR reads




//////////////////////////////////////////////////////////////////////
// Local Functions ///////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


// Returns the bit number (where 0 is least significant bit 
// and has the highest priority) for the highest set bit in reg.
// Returns 1 more than the lowest priority if no bits are set. (for local use only)
static int GetPriority(unsigned int reg)
{
	int IRQ;

	if(reg == 0) return NUM_INTERRUPT_PINS;	// Return (lowest priority + 1)

	for(IRQ = 0; !(reg & 0x1); IRQ++) reg = reg >> 1;
	
	return IRQ;
}


// Returns the highest priority ready interrupt. If none are 
// available (i.e., a higher priority interrupt is in service or
// the requested interrupts are masked by the IMR) then 
// NUM_INTERRUPT_PINS is returned. (for local use only)
static inline int PriorityDecode(void)
{
	// The logic here assumes fully nested mode
	int IRQ;

	IRQ = GetPriority(IRR & ~IMR);

	if(IRQ < GetPriority(ISR)) return IRQ;
	else return NUM_INTERRUPT_PINS;
}


// Sets flags to signal that an interrupt is waiting to be serviced.
// Equivalent to the INTR pin on the processor going high. (for local use only)
static void RaiseINTR(void)
{
	INTR = true;
	M.x86.intr |= INTR_ASYNCH;	// Not sure if this is what INTR_ASYNCH was intended for, but this works.
}


//////////////////////////////////////////////////////////////////////
// Global Functions (PIC interface) //////////////////////////////////
//////////////////////////////////////////////////////////////////////


// When an interrupt is generated the upper 5 bits of the global 'offset'
// are concatenated with the interrupt number. That is, if an interrupt 
// occurs with IRQ number IRnum, then InterruptAcknowledge() will return
// the value: ((offset & ~0x7) + IRnum).
// The ISRoffset must be at least 8 and < 248 on any 80x86 system. Otherwise
// an offset of 8 is assumed.
void InitPIC(int ISRoffset)
{
	// Set up the offset
	if(ISRoffset >= 0 && ISRoffset < 248) offset = (ISRoffset & ~0x7);
	else offset = 8;

	// Set up internal registers
	ResetPIC();
}


// Put the PIC back into its initial state
void ResetPIC(void)
{
	IRR = ISR = IMR = 0;
	INTR = false;
}


// Signals an interrupt request.
// This is equivalent to asserting IRQ pin number 'IRnum' on the PIC.
void FlagInterrupt(int IRQ)
{
	// Check for valid IRQ
	if(IRQ < 0 || IRQ > NUM_INTERRUPT_PINS) return;

	// Signal interrupt request
	SET_BIT(IRR, IRQ);

	// Set flags if this interrupt should be serviced
	if(PriorityDecode() != NUM_INTERRUPT_PINS) {
		RaiseINTR();
	}
}


// Acknowledge interrupt and get the interrupt vector number.
// Similar to what happens when the 80x86 sets its INTA pin low.
// Returns -1 if there are no interrupts to service.
// This should ONLY be called by simulator when INTR is high.
int InterruptAcknowledge(void)
{
	int IRQ;

	// Get highest priority ready interrupt
	IRQ = PriorityDecode();

	// Reset flags
	INTR = false;

	// Check for validity
	if(IRQ == NUM_INTERRUPT_PINS) return -1;

	// Move interrupt from IRR to ISR
	CLEAR_BIT(IRR, IRQ);
	SET_BIT(ISR, IRQ);

	return offset + IRQ;
}



// Does what the 8259A does when it receives a non-specific EOI command
void NonspecificEOI(void)
{
	int priority;

	// Clear the highest priority bit in ISR (In fully nested mode, this is always currently running interrupt)
	priority = GetPriority(ISR);
	if(priority < NUM_INTERRUPT_PINS) CLEAR_BIT(ISR, GetPriority(ISR));
	
	// Signal next highest interrupt if one previously signaled
	if(PriorityDecode() != NUM_INTERRUPT_PINS) {
		RaiseINTR();
	}
}



// Sets the IMR (Interrupt Mask Register)
// A 1 bit DISABLES the corresponding interrupt. A 0 bit
// ENABLES the corresponding interrupt.
void WriteIMR(unsigned int mask)
{
	// Set mask register
	IMR = mask;

	// Set flags if interrupt should now be serviced because of change
	if(PriorityDecode() != NUM_INTERRUPT_PINS) {
		RaiseINTR();
	}
}


// Reads the IMR (Interrupt Mask Register)
// A 1 bit means the corresponding IRQ is DISABLED. A 0 bit
// means it is ENABLED.
unsigned int ReadIMR(void)
{
	return IMR;
}


// Reads the ISR (In-Service Register).
// A 1 bit means that the corresponding IRQ is currently being serviced.
unsigned int ReadISR(void)
{
	return ISR;
}


// Reads the IRR (Interrupt Request Register).
// A 1 bit means that the corresponding IRQ pin is currently asserted and
// is waiting to be serviced.
unsigned int ReadIRR(void)
{
	return IRR;
}


// Handle a write to a PIC port
void PICWriteByte(unsigned short port, unsigned char value)
{
	if(port & 1) {	// Treat like port 0x21
		// Must be IMR write
		WriteIMR(value);
	}
	else {			// Treat like port 0x20
		// Check for Nonspecific EOI
		if((value & 0xF8) == 0x20) NonspecificEOI();

		// Check for IRR read command
		else if(value == 0x0A) PICstate = PIC_IRR_READ_CMD;

		// Check for ISR read command
		else if(value == 0x0B) PICstate = PIC_ISR_READ_CMD;
	}
}

// Handle a read from a PIC port
unsigned char PICReadByte(unsigned short port)
{
	if(port & 1) {	// Treat like port 0x21
		return ReadIMR();
	}
	else {			// Treat like port 0x20
		if(PICstate == PIC_IRR_READ_CMD) {
			PICstate = PIC_NO_CMD;
			return ReadIRR();
		}
		else if(PICstate == PIC_ISR_READ_CMD) {
			PICstate = PIC_NO_CMD;
			return ReadISR();
		}
	}
	return 0;
}

