//
// Utils.cpp: Some commonly used routines
//
// These routines are mostly for interpreting or manipulating strings. 
// There are also routines  for identifying 8086 registers. See
// Utils.h for more useful 8086 related definitions.
//
//////////////////////////////////////////////////////////////////////

#include "Utils.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

const char *sizeName[] = { "*ERROR*", "BYTE", "WORD", "triple BYTE", "DWORD" };

// The following MUST have the same order as the defenitions in Utils.h
const char *registerName[] = {
	"BAD_REG_ID",
// 8086 registers:
	"AH",
	"AL",
	"BH",
	"BL",
	"CH",
	"CL",
	"DH",
	"DL",
	"AX",
	"BX",
	"CX",
	"DX",
	"CS",
	"DS",
	"ES",
	"SS",
	"IP",
	"SP",
	"BP",
	"SI",
	"DI",
	"OF",
	"DF",
	"IF",
	"TF",
	"SF",
	"ZF",
	"AF",
	"PF",
	"CF",
// PIC registers:
	"IMR",
	"ISR",
	"IRR",
	""
};

// Convert the register name to a register number.
int GetRegister(char *regName)
{
	char temp[4];
	int i;

	temp[0] = regName[0];
	temp[1] = regName[1];
	temp[2] = regName[2];
	temp[3] = '\0';

	ToUpper(temp);

	for(i = 0; *(registerName[i]) != '\0'; i++) {
		if(!strcmp(registerName[i], temp)) return i;
	}

	return BAD_REG;

	/*char temp[3];

	temp[0] = regName[0];
	temp[1] = regName[1];
	temp[2] = '\0';

	ToUpper(temp);

	if(temp[1] == 'H') {
		switch(temp[0]) {
		case 'A': return AH_REG;
		case 'B': return BH_REG;
		case 'C': return CH_REG;
		case 'D': return CH_REG;
		}
	}
	else if(temp[1] == 'L') {
		switch(temp[0]) {
		case 'A': return AL_REG;
		case 'B': return BL_REG;
		case 'C': return CL_REG;
		case 'D': return CL_REG;
		}
	}
	else if(temp[1] == 'X') {
		switch(temp[0]) {
		case 'A': return AX_REG;
		case 'B': return BX_REG;
		case 'C': return CX_REG;
		case 'D': return CX_REG;
		}
	}
	else if(temp[1] == 'S') {
		switch(temp[0]) {
		case 'C': return CS_REG;
		case 'D': return DS_REG;
		case 'E': return ES_REG;
		case 'S': return SS_REG;
		}
	}
	else if(temp[1] == 'P') {
		switch(temp[0]) {
		case 'I': return IP_REG;
		case 'S': return SP_REG;
		case 'B': return BP_REG;
		}
	}
	else if(temp[1] == 'I') {
		switch(temp[0]) {
		case 'S': return SI_REG;
		case 'D': return DI_REG;
		}
	}
	else if(temp[1] == 'F' || temp[1] == '\0') {
		switch(temp[0]) {
		case 'O': return OF_REG;
		case 'D': return DF_REG;
		case 'I': return IF_REG;
		case 'T': return TF_REG;
		case 'S': return SF_REG;
		case 'Z': return ZF_REG;
		case 'A': return AF_REG;
		case 'P': return PF_REG;
		case 'C': return CF_REG;
		}
	}

	if(!strcmp(regName, "IMR") return IMR_REG;
	if(!strcmp(regName, "ISR") return ISR_REG;
	if(!strcmp(regName, "IRR") return IRR_REG;
	*/
	return BAD_REG;
}

// Returns true if the indicated instruction is a call or int instruction.
// (Used by the step over command)
// NOTE: This function currently does NOT detect calls with segment overrides!
bool IsCall(unsigned char *instr)
{
	unsigned char opcode;
	unsigned char ModRM;

	opcode = instr[0];

	/*
	NOTE: This section of the function would look for and skip over valid
	  instruction prefixes. It turned out this isn't really needed because the 
	  simulator decodes each prefix as if it had been an instruction by itself.
  
	// Check for instruction prefixes
	while(
		//	Lock and repeat prfixes are not not valid with call instruction
		//opcode == 0xF0 ||	// LOCK		- Lock and Repeat -
		//opcode == 0xF2 ||	// REPNE/REPNZ
		//opcode == 0xF3 ||	// REP/REPE/REPZ
		//

		opcode == 0x2E ||	// CS		- Segment Overrides -
		opcode == 0x36 ||	// SS
		opcode == 0x3E ||	// DS
		opcode == 0x26 ||	// ES
		opcode == 0x64 ||	// FS (386)
		opcode == 0x65 ||	// GS (386)

		opcode == 0x66 ||	//			- Operand Size -
		
		opcode == 0x67   	//			- Address Size - 
		) {
		instr++;
		opcode = *instr;
	}
	*/

	ModRM = instr[1];

	// Determine if it's a call or int instruction
	if(	opcode == 0xE8 ||				// CALL rel		- E8 cw
		(opcode == 0xFF && (
			(ModRM & 0x38) == 0x10 ||	// CALL reg/mem	- FF /2
			(ModRM & 0x38) == 0x18)		// CALL m16:16	- FF /3
		) ||								
		opcode == 0x9A ||				// CALL far ptr	- 9A cd
		opcode == 0xCC ||				// INT3			- CC
		opcode == 0xCD ||				// INT #		- CD ib
		opcode == 0xCE					// INIO			- CE
	) return true;

	return false;
}


// Returns true if c is a visible ascii character (not whitespace or control character)
bool IsVisibleChar(char c)
{
	if(c > ' ' && c < 127) return true;
	return false;
}

// Returns true if argument c is a numeric digit, false otherwise.
bool IsDigit(char c)
{
	if(c >= '0' && c <= '9') return true;
	return false;
}

// Returns true if argument c is a numeric digit or a letter a-f (or A-F). False otherwise.
bool IsHexDigit(char c)
{
	if(IsDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) return true;
	return false;
}

// Returns true if argument c is a-z or A-Z.
bool IsLetter(char c)
{
	if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) return true;
	return false;
}

// Returns true if the string represents a number:
// Valid styles:
// 1) 0xAB, 0XAB	(hex)
// 2) ABh, ABH		(hex)
// 3) 171			(decimal assumed)
bool IsNumber(char *string)
{
	int len;
	int i;
	
	// Get string length
	for(len = 0; string[len] != '\0'; len++);

	if(string[1] == 'x' || string[1] == 'X') {
		// Case 1) First digits "0x" or "0X", all others should be hex digits
		if(string[0] != '0') return false;
		for(i = 2; i < len; i++) if(!IsHexDigit(string[i])) return false;
		return true;
	}
	else if(string[len-1] == 'h' || string[len-1] == 'H') {
		// Case 2) All but last byte (the h or H) should be hex digits
		len--;
		for(i = 0; i < len; i++) if(!IsHexDigit(string[i])) return false;
		return true;
	}
	else {
		// Case 3) Must be all numbers
		for(i = 0; i < len; i++) if(!IsDigit(string[i])) return false;
		return true;
	}
	return false;
}

// Returns true if the string is of the correct form for an address.
// Valid styles:
// Valid styles:
//  1) seg:off		(hex only for segment and offset)
//  2) 0xAB, 0XAB	(hex)
//  3) ABh, ABH		(hex)
//  3) 171			(decimal assumed)
bool IsAddress(char *string)
{
	int colonIndex;
	int len;
	int i;

	for(len = 0, colonIndex = 0; string[len] != '\0'; len++) {
		if(string[len] == ':') colonIndex = len;
	}

	if(colonIndex) {
		// Case 1) Has colon in middle or end. All digits before and after must be hex digits
		for(i = 0; i < colonIndex; i++) if(!IsHexDigit(string[i])) return false;
		for(i++; i < len; i++) if(!IsHexDigit(string[i])) return false;
		return true;
	}
	
	// Must be a physical address in the form of a number (hex or dec)
	return IsNumber(string);
}

// Convert positive hex string to integer. Return -1 if not a valid hex number. Only first 8 chars will be considered.
int XtoI(char *string)
{
	int i, pwr, length;
	unsigned int val;
	char c;

	// Determin string length
	for(length = 0; string[length] != '\0' && length < 8; length++);
	
	// Determin value
	val = 0;
	pwr = 1;
	for(i = length-1; i >= 0; i--) {
		c = string[i];
		if(IsDigit(c)) {
			val += (unsigned int)(c - '0') * pwr;
		}
		else if(c >= 'a' && c <= 'f') {
			val += (unsigned int)(c - ('a' - 10)) * pwr;
		}
		else if(c >= 'A' && c <= 'F') {
			val += (unsigned int)(c - ('A' - 10)) * pwr;
		}
		else return -1;

		pwr *= 0x10;
	}
	return val;
}


// Converts a string to integer.
// The string number can be of the following forms
// 1) 0xAB, 0XAB	(hex)
// 2) ABh, ABH		(hex)
// 3) 171			(decimal assumed)
int InterpretNumber(char *string)
{
	int num, len;

	// Determin string length
	for(len = 0; string[len] != '\0'; len++);

	// Is it hex?
	if(string[1] == 'x' || string[1] == 'X' || string[len-1] == 'h' || string[len-1] == 'H') {
		sscanf(string, "%x", &num);
	}
	else sscanf(string, "%d", &num);

	return num;
}


// Takes a string representing an Address
// It can be of the following forms:
//  1) seg:off		(hex only for segment and offset)
//  2) 0xAB, 0XAB	(hex)
//  3) ABh, ABH		(hex)
//  3) 171			(decimal assumed)
void InterpretAddress(char *addrString, Address *addr)
{
	int i, len;
	bool isColon;
	char *segmentStr, *offsetStr;
	
	isColon = false;

	// Determin string length
	for(len = 0; addrString[len] != '\0'; len++);

	// Check for HEX segment offset form 0000:0000
	for(i = 0; i < len; i++) {
		if(addrString[i] == ':') {
			isColon = true;
			addrString[i] = '\0';		// Null terminate segment
			segmentStr = &addrString[0];
			offsetStr = &addrString[i+1];
		}
	}
	
	if(isColon) {
		// Of form seg:off. Get the two and calculate physical address.
		//addr->segment = XtoI(segmentStr);
		//addr->offset = XtoI(offsetStr);
		sscanf(segmentStr, "%x", &(addr->segment));
		sscanf(offsetStr, "%x", &(addr->offset));
		addr->physical = PHYSICAL_ADDR(addr->segment, addr->offset);
	}
	else {
		// Single number. Get it and calculate segment and offset.
		addr->physical = InterpretNumber(addrString);
		addr->segment = SEGMENT(addr->physical);
		addr->offset = OFFSET(addr->physical);
	}
}


// Converts a null terminated string to all lowercase.
void ToLower(char *string)
{
	int i;
	char c;

	i = 0;
	c = string[0];
	while(c != '\0') {
		if(c >= 'A' && c <= 'Z') {
			c += ('a' - 'A');
			string[i] = c;
		}
		i++;
		c = string[i];
	}
}

// Converts a null terminated string to all uppercase.
void ToUpper(char *string)
{
	int i;
	char c;

	i = 0;
	c = string[0];
	while(c != '\0') {
		if(c >= 'a' && c <= 'z') {
			c += ('A' - 'a');
			string[i] = c;
		}
		i++;
		c = string[i];
	}
}


