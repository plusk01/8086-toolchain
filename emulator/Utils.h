//
// Utils.h: Header for Utils.cpp
//
// This file contains 8086 related and other usefull definitions.
//
//////////////////////////////////////////////////////////////////////

#ifndef UTILS_H
#define UTILS_H

// Some useful macros

#define PHYSICAL_ADDR(SEG,OFF) (((int)(SEG)<<4)+((int)(OFF)))
#define SEGMENT(ADDR)	((ADDR>>4) & 0xF000)
#define OFFSET(ADDR)	((ADDR) & 0xFFFF)


// Get the flags bit value
#define CF_BIT ACCESS_FLAG(F_CF)            // CARRY flag
#define PF_BIT (ACCESS_FLAG(F_PF)>>2)       // PARITY flag
#define AF_BIT (ACCESS_FLAG(F_AF)>>4)       // AUX  flag
#define ZF_BIT (ACCESS_FLAG(F_ZF)>>6)       // ZERO flag
#define SF_BIT (ACCESS_FLAG(F_SF)>>7)       // SIGN flag
#define TF_BIT (ACCESS_FLAG(F_TF)>>8)       // TRAP flag
#define IF_BIT (ACCESS_FLAG(F_IF)>>9)       // INTERRUPT flag
#define DF_BIT (ACCESS_FLAG(F_DF)>>10)      // DIR flag
#define OF_BIT (ACCESS_FLAG(F_OF)>>11)      // OVERFLOW flag

// The following give nearest segment and relative offset
#define SEGMENT_NEAR(ADDR)	((ADDR) >> 4)
#define OFFSET_NEAR(ADDR)	((ADDR) & 0xF)

#ifndef MAX
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))
#endif

#ifndef MIN
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#endif

// Structs

typedef struct {
	int segment;
	int offset;
	int physical;
} Address;

// Register numbers (keep in numerical order for speed)
// The following MUST have the same order as the registerName array in Utils.cpp
#define BAD_REG 0
#define AH_REG 1
#define AL_REG 2
#define BH_REG 3
#define BL_REG 4
#define CH_REG 5
#define CL_REG 6
#define DH_REG 7
#define DL_REG 8
#define AX_REG 9
#define BX_REG 10
#define CX_REG 11
#define DX_REG 12
#define CS_REG 13
#define DS_REG 14
#define ES_REG 15
#define SS_REG 16
#define IP_REG 17
#define SP_REG 18
#define BP_REG 19
#define SI_REG 20
#define DI_REG 21
#define OF_REG 22
#define DF_REG 23
#define IF_REG 24
#define TF_REG 25
#define SF_REG 26
#define ZF_REG 27
#define AF_REG 28
#define PF_REG 29
#define CF_REG 30
#define IMR_REG 31
#define ISR_REG 32
#define IRR_REG 33



// Globals
extern const char *sizeName[];
extern const char *registerName[];

// Headers for useful functions

int GetRegister(char *regName);
bool IsCall(unsigned char *instr);
bool IsVisibleChar(char c);
bool IsDigit(char c);
bool IsLetter(char c);
bool IsNumber(char *string);
bool IsAddress(char *string);
int XtoI(char *string);
int InterpretNumber(char *string);
void InterpretAddress(char *addrString, Address *addr);
bool AddressInRange(int addr);
bool AddressInRange(int addr, int size);
void ToLower(char *string);
void ToUpper(char *string);


#endif // #ifndef UTILS_H


