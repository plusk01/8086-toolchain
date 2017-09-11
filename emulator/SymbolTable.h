//
// SymbolTable.h: interface for the CSimControl class.
//
// This class implements the symbol table loading and lookup.
// The symbol is based on the listing file produced by the EE 425
// version of NASM. This code cannot load the symbols from a
// normal NASM listing file.
//
//////////////////////////////////////////////////////////////////////


#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdio.h>


#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif


// Listing file specific definitions:
// These represent the character offset where the item occurs relative to each line (0 is first column)
#define ADDR_COLUMN 9
#define ADDR_LENGTH 8			// Number of hex digits in addresses
#define ASM_COLUMN 48
#define LISTING_EXT "lst"		// File extension for listing file
#define MAX_LINE_SIZE 200


typedef struct TableEntryStruct {
	char *symbol;
	unsigned address;
	struct TableEntryStruct *next;
} TableEntry;


typedef int BOOL;

class CSymbolTable  
{
public:
	CSymbolTable();
	virtual ~CSymbolTable();

	BOOL LoadSymbols(char *fileName, BOOL deriveFilename);
	void FlushTable(void);

	BOOL LookupAddress(unsigned int physicalAddr, const char **name);
	BOOL LookupSymbol(char *name, unsigned int *pPhysicalAddr);

	BOOL TableLoaded(void);

private:
	BOOL GetLine(FILE *pFile, char *buffer, int maxBytes);
	BOOL HasLabel(char *text, int *offset, int *length);
	BOOL HasAddress(char *text, unsigned *pAddress);
	BOOL GetNextAddress(FILE *pFile, unsigned *pAddress);
	BOOL InsertEntry(char *symbol, unsigned address);

	TableEntry *pTableList;
};


#endif // #ifndef SYMBOL_TABLE_H

