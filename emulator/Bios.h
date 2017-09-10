//
// Bios.h: header file for Bios.cpp
//
// These defenitions identify interrupt vector number and important
// memory addresses. It also has prototypes for Bios.cpp routines.
//
//////////////////////////////////////////////////////////////////////

/*********************************************************************
IMPORTANT NOTE: The defined values in this file are highly dependent
on the vaues in the vector table and clib loaded with the user 
program. Changes in one necessitates a change in the other.
*********************************************************************/

#ifndef BIOS_H
#define BIOS_H

#include "SimControl.h"


////////////////////////////////
// Hardware Interrupt Numbers //

// Internal Interrupts: (8086 limited to vectors 0-15, Am186ES is more versitile)
#define DIV_ERROR_INT	0x00

// WARNING! Any changes here should be reflected in the user code vector table! //
// External Interrupts (IRQs)
#define EXT_INT_OFFSET	0x08	// Offset at which the hardware interrupt vectors start
#define RESET_INT		0x00
#define TICK_INT		0x01
#define KEYBOARD_INT	0x02

// Simptris Interrupts
#define SIMP_GAME_OVER_INT	0x03
#define SIMP_NEW_PIECE_INT	0x04
#define SIMP_RECEIVED_INT	0x05
#define SIMP_TOUCHDOWN_INT	0x06
#define SIMP_LINE_CLEAR_INT	0x07

///////////////////////////////////////
// Software Interrupt Vector Numbers //

#define PC_BIOS_INT		0x10
#define SIMPTRIS_INT	0x1B
#define DOS_IO_INT		0x21


/////////////////////////////
// Important I/O addresses //

// Keyboard buffer
#define KEY_BUF_ADDR		0xC0

// Simptris communcations memory; Change ONLY TET_COM_ADDR to relocate
#define TET_COM_ADDR		0xC2
#define TET_PIECE_TYPE_ADDR	TET_COM_ADDR+0x0
#define TET_PIECE_ID_ADDR	TET_COM_ADDR+0x2
#define TET_ORIENT_ADDR		TET_COM_ADDR+0x4
#define TET_COLUMN_ADDR		TET_COM_ADDR+0x6
#define TET_TOUCHDOWN_ADDR	TET_COM_ADDR+0x8
#define TET_BITMAP_ADDR		TET_COM_ADDR+0xA 


//////////////////////////////
// Simptris Service Numbers //

#define SLIDE_PIECE		0x00
#define ROTATE_PIECE	0x01
#define SEED_NUMBER		0x02
#define START_SIMPTRIS	0x03


/////////////////////////
// Bios.cpp Prototypes //

void SetSimControl(CSimControl *simulator);
void SetUpInterrupts(void);

void SetKeyboardBuffer(char c);

void ResetInterrupt(int intNum);
void TickInterrupt(int intNum);
void KeyboardInterrupt(int intNum);

void Int00h(int intNum);
void Int10h(int intNum);
void Int21h(int intNum);

void SimptrisServices(int intNum);
void signal_interrupt(int intID, int a, int b, int c, int d);


#endif

