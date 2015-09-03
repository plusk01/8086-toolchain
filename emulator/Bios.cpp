//
// Bios.cpp: BIOS emulation routines.
//
// These routines emulate select BIOS and DOS interrupt services.
// There are also routines to handle Simptris events and services.
// This file should be used as a place for code that latches onto
// a desired interrupt via the 8086 emulator code. When this is done, 
// the emulator calls the attached method rather than perfoming the 
// int instruction.
//
//////////////////////////////////////////////////////////////////////

#include "Bios.h"

#include "Utils.h"
#include "x86emu/x86emui.h"


static CSimControl *SimControl;
static CProgramWindow *ProgramWindow;


// The function needs to be called to set the appropriate output window
void SetSimControl(CSimControl *simulator)
{
	SimControl = simulator;
	ProgramWindow = &(simulator->ProgramWindow);
}


// This function sets up the interrupt vector table .
// This allows us to use simulator code to handle interrupts.
void SetUpInterrupts(void)
{
	// External hardware interrupts (comment out these to simulate interrupts in user code)
//	_X86EMU_intrTab[EXT_INT_OFFSET + RESET_INT] = ResetInterrupt;
//	_X86EMU_intrTab[EXT_INT_OFFSET + TICK_INT] = TickInterrupt;
//	_X86EMU_intrTab[EXT_INT_OFFSET + KEYBOARD_INT] = KeyboardInterrupt;

	// Internal interrupts
	_X86EMU_intrTab[DIV_ERROR_INT] = Int00h;

	// Software interrupts
	_X86EMU_intrTab[PC_BIOS_INT] = Int10h;
	_X86EMU_intrTab[SIMPTRIS_INT] = SimptrisServices;
	_X86EMU_intrTab[DOS_IO_INT] = Int21h;
}


//////////////////////////////////////////////////////////////////////

// Sets a 1 byte memory address to the given character.
// When a key is pressed, it's value is stored with this function.
void SetKeyboardBuffer(char c)
{
	wrb((u32)KEY_BUF_ADDR, (u8)c);
}


//////////////////////////////////////////////////////////////////////
// Hardware interrupt handlers. These override user interrupt code
// when enabled in SetUpInterrupts().

void ResetInterrupt(int intNum)
{
	ProgramWindow->PrintText("ResetInterrupt\n");
	NonspecificEOI();
}


void TickInterrupt(int intNum)
{
	static int count = 0;
	count++;
	if(count % 10 == 0) ProgramWindow->PrintText("Tick(%u)\n", count);
	//SimControl->SimIO.PrintMessage("Tick(%u)\n", count);
	NonspecificEOI();
}


void KeyboardInterrupt(int intNum)
{
	ProgramWindow->PrintText("Keypress(%c)\n", (char)rdb(KEY_BUF_ADDR));
	NonspecificEOI();
}


//////////////////////////////////////////////////////////////////////
// Simulator controlled interrupt handlers.

// Handle divide by zero, divide overflow.
void Int00h(int intNum)
{
	SimControl->SystemHalted(EXCEPTION_DIVIDE_BY_ZERO);
}


// Handle BIOS interrupt 10h.
void Int10h(int intNum)
{
	// Nothing handled at this time
}


// Handle select DOS interrupt 21h services.
void Int21h(int intNum)
{
	char *string;
	unsigned int i;
	unsigned int maxi;
	int length;

	switch(M.x86.R_AH) {
	case 0x02:	// Character output. DL is 8-bit character to print
		ProgramWindow->PrintChar(M.x86.R_DL);
		break;

	case 0x09:	// String output. DS:DX points to '$' terminated string
		string = (char *)M.privatevp;
		i = PHYSICAL_ADDR(M.x86.R_DS, M.x86.R_DX);

		for(maxi = i; string[maxi] != '$' && maxi < M.mem_size; maxi++);
		length = maxi - i;

		if(length > 0) ProgramWindow->SendString(string + i, length);
		
		break;
	
	case 0x40:	// Write to device or file. Only supports deivce 0x01 (stdout). BX is handle, CX is num bytes, DS:DX points to data to write.
		if(M.x86.R_BX != 1) {
			ProgramWindow->PrintText("ERROR: Only handle 1 (stdout) supported for INT 21h, service 40h. Handle %Xh was requested.\n", M.x86.R_BX);
			break;
		}

		i = PHYSICAL_ADDR(M.x86.R_DS, M.x86.R_DX);
		length = M.x86.R_CX;
		if(i + length >= M.mem_size) length = M.mem_size - i;
		
		if(length > 0) ProgramWindow->SendString((char *)M.privatevp + i, length);
		
		break;

	case 0x4C:	// Terminate with return code. AL is return code
		SimControl->ProgramTerminated((int)(M.x86.R_AL));
		break;

	default:
		ProgramWindow->PrintText("ERROR: INT 21h, service %xh not supported.\n", (unsigned)M.x86.R_AH);
	}
}


// Handle Simptris software interrupt commands.
void SimptrisServices(int intNum)
{
	if(!SimControl->SimptrisMode()) {
		SimControl->SimIO.PrintText("ERROR: Simptris mode not enabled.\n");
		return;
	}

	int number;
	int dir;

	switch(M.x86.R_AH) {
	case SLIDE_PIECE:
		// AL>		Direction
		// DX>		Piece ID
		dir = M.x86.R_AL;
		number = (unsigned)M.x86.R_DX;
		SimControl->Simptris.SlidePieceCmd(number, dir);
		
		// printf("Sliding piece %d in direction %d\n", number, dir);
		
		break;
	case ROTATE_PIECE:
		// AL>		Direction
		// DX>		Piece ID
		dir = M.x86.R_AL;
		number = (unsigned)M.x86.R_DX;
		SimControl->Simptris.RotatePieceCmd(number, dir);

		// printf("Rotating piece %d in direction %d\n", number, dir);

		break;
	case SEED_NUMBER:
		// CX,DX>	Seed number
		number = ((unsigned)M.x86.R_CX) << 16;
		number |= (unsigned)M.x86.R_DX;
		SimControl->Simptris.seed(number);
		break;
	case START_SIMPTRIS:
		SimControl->Simptris.init_simptris();
		break;
	default:
		SimControl->SimIO.PrintMessage("WARNING: Bad Simptris service number in call to int %Xh.\n", SIMPTRIS_INT);
	}
}



//////////////////////////////////////////////////////////////////////////////
// This function is used by Simptris to signal the various events that occur.

void signal_interrupt(int intID, int a, int b, int c, int d)
{
	switch (intID)
	{
	case TET_GAME_OVER:
		// Signal that the game has ended
		FlagInterrupt(SIMP_GAME_OVER_INT);
		break;
	case TET_TOUCHDOWN:
		// Signal that a touchdown has occured
		// a is piece id
		// b through d are the 96 bits for the playing screen bitmap
		wrw(TET_TOUCHDOWN_ADDR, a);

		// Store bits, 1 col per word; MSB is bottom of coloumn
		wrw(TET_BITMAP_ADDR+0x0, b >> 16);	// Col 1 (leftmost)
		wrw(TET_BITMAP_ADDR+0x2, b);		// Col 2
		wrw(TET_BITMAP_ADDR+0x4, c >> 16);	// Col 3
		wrw(TET_BITMAP_ADDR+0x6, c);		// Col 4
		wrw(TET_BITMAP_ADDR+0x8, d >> 16);	// Col 5
		wrw(TET_BITMAP_ADDR+0xA, d);		// Col 6 (rightmost)
		FlagInterrupt(SIMP_TOUCHDOWN_INT);
		break;
	case TET_PIECE_A:
		// Signal that a new corner peice has entered scene (type A)
		// a is piece id
		// b is piece orientation
		// c is entry column
		wrw(TET_PIECE_TYPE_ADDR, TET_TYPE_A);
		wrw(TET_PIECE_ID_ADDR, a);
		wrw(TET_ORIENT_ADDR, b);
		wrw(TET_COLUMN_ADDR, c);
		FlagInterrupt(SIMP_NEW_PIECE_INT);

		//printf("\nNew piece of type A created. ID: %d    Orientation: %d    Column: %d\n\n", a, b, c);

		break;
	case TET_PIECE_B:
		// Signal that a new straight peice has entered scene (type B)
		// a is piece id
		// b is piece orientation
		// c is entry column
		wrw(TET_PIECE_TYPE_ADDR, TET_TYPE_B);
		wrw(TET_PIECE_ID_ADDR, a);
		wrw(TET_ORIENT_ADDR, b);
		wrw(TET_COLUMN_ADDR, c);
		FlagInterrupt(SIMP_NEW_PIECE_INT);

		//printf("\nNew piece of type B created. ID: %d    Orientation: %d    Column: %d\n\n", a, b, c);

		break;
	case TET_RECEIVED:
		// Signal that last command was received and Tetris is ready for a new command
		FlagInterrupt(SIMP_RECEIVED_INT);
		
		// printf("Command received\n");
		
		break;
	case TET_LINE_CLEAR:
		// Signal that a line has been cleared
		FlagInterrupt(SIMP_LINE_CLEAR_INT);
		break;
	default:
		return;
	}
}

