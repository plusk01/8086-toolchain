//
// SymbolTable.cpp: implementation of the CSimControl class.
//
// This class implements the symbol table loading and lookup.
// The symbol is based on the listing file produced by the EE 425
// version of NASM. This code cannot load the symbols from a
// normal NASM listing file.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "SymbolTable.h"

#define IS_LETTER(X)		(((X) >= 'a' && (X) <= 'z') || ((X) >= 'A' && (X) <= 'Z'))
#define IS_NUMBER(X)		((X) >= '0' && (X) <= '9')
#define IS_WHITESPACE(X)	((X) == ' ' || (X) == '\t')
#define IS_LABEL_START(X)	(IS_LETTER(X) || (X)=='_' || (X)=='?' || (X)=='$')		// Accounts for restrictions on first letter of a label
#define IS_LABEL_CHAR(X)	(IS_LETTER(X) || IS_NUMBER(X) || (X)=='_' || (X)=='$' || (X)=='#' || (X)=='@' || (X)=='~' || (X)=='.' || (X)=='?')
#define IS_HEX_DIGIT(X)		(IS_NUMBER(X) || ((X)>='a' && (X)<='f') || ((X)>='A' && (X)<='F'))
#define IS_EOL_CHAR(X)		((X)=='\n' || (X) == '\r')

#define MAX_FILE_PATH		300


CSymbolTable::CSymbolTable()
{
	pTableList = NULL;
	FlushTable();
}

CSymbolTable::~CSymbolTable()
{
	FlushTable();
}


// Free any data associated with current symbol table and re-initialize.
void CSymbolTable::FlushTable(void)
{
	TableEntry *pEntry, *pNext;

	pEntry = pTableList;

	while(pEntry) {
		pNext = pEntry->next;
		
		// Free symbol name
		free(pEntry->symbol);

		// Free rest of entry
		free(pEntry);

		pEntry = pNext;
	}

	pTableList = NULL;
}


BOOL CSymbolTable::InsertEntry(char *symbol, unsigned address)
{
	TableEntry *pEntry;
	int length;

	length = strlen(symbol);

	// Insert at beginning of list //
	
	// Allocate entry
	pEntry = (TableEntry *)malloc(sizeof(TableEntry));
	if(pEntry == NULL) return FALSE;
	
	// Allocate symbol name text
	pEntry->symbol = (char *)malloc(length+1);
	if(pEntry->symbol == NULL) {
		free(pEntry);
		return FALSE;
	}
	
	// Set entries
	pEntry->next = pTableList;
	memcpy(pEntry->symbol, symbol, length+1);
	pEntry->address = address;
	
	pTableList = pEntry;

	return TRUE;
}


// This gets a line of text from the source file. Works with DOS, Unix, and Mac files.
// It will NOT return blank lines and does not store any EOL characters in buffer.
// It will not write more than maxBytes (including NULL).
// I used to use fgets() but it doesn't work with the other OSs' end-of-line formats.
// Returns FALSE if an error occured or if reads EOF before a non-EOL character.
BOOL CSymbolTable::GetLine(FILE *pFile, char *buffer, int maxBytes)
{
	char c;

	// Find start of line
	do {
		c = fgetc(pFile);
	} while(IS_EOL_CHAR(c));

	if(c == EOF) return FALSE;

	maxBytes--;

	// Store until end of line
	do {
		if(maxBytes) {
			*buffer = c;
			maxBytes--;
			buffer++;
		}
		c = fgetc(pFile);

		if(c == EOF) {
			if(ferror(pFile)) return FALSE;
			else break;
		}

	} while(!IS_EOL_CHAR(c));

	// Null terminate
	*buffer = '\0';

	return TRUE;
}



// Returns true if the text at &text[ASM_COLUMN] contains a valid assembly label.
// If it does, *offset will be set to the index of the first character of the label
// and *length is set to the lengh of the label (not including the delimiting ':' or whitespace).
// If it doesn't have a label, returns false and leaves *offset and *length untouched.
// Also leaves *offset and *length untouched if they are null pointers, respectively.
// NOTE: NASM allows labels without a colon, but to be recognized by this function
// the colon MUST be present. Local labels (beginning with a "." are not supported.
BOOL CSymbolTable::HasLabel(char *text, int *offset, int *length)
{
	int i;
	int startOffset, endOffset;

	// Check for valid start (probably not really necessary)
	if(!IS_WHITESPACE(*text) && !IS_LABEL_START(*text)) return FALSE;


	// Find first character of label
	for(i = ASM_COLUMN; !IS_LABEL_START(text[i]); i++) {
		// Are we still in whitespace?
		if(text[i] == '\0' || !IS_WHITESPACE(text[i])) return FALSE;
	}

	startOffset = i;

	// Find end of label
	for(i += 1; IS_LABEL_CHAR(text[i]); i++);

	// Is this a valid end of label?
	if(text[i] == '\0' || (text[i] != ':' && !IS_WHITESPACE(text[i]))) return FALSE;

	endOffset = i-1;

	// Seek beyond whitespace, if any
	for(; IS_WHITESPACE(text[i]); i++);

	// Make sure there's a colon (confirms this is a label and not an instruction)
	if(text[i] != ':') return FALSE;

	// Set label info
	if(offset) *offset = startOffset;
	if(length) *length = endOffset - startOffset + 1;

	return TRUE;
}


// Returns true if the text at &text[ADDR_COLUMN] is a valid ADDR_LENGTH digit hex number,
// dilimited by whitespace at the end. If it finds a valid address, it stores its value in *pAddress
// if it's a non-null pointer.
BOOL CSymbolTable::HasAddress(char *text, unsigned *pAddress)
{
	int i;

	// See if number is valid
	for(i = ADDR_COLUMN; i < ADDR_COLUMN+ADDR_LENGTH; i++) {
		if(!IS_HEX_DIGIT(text[i])) return FALSE;
	}

	// Make sure it's whitespace delimited at the end
	if(!IS_WHITESPACE(text[ADDR_COLUMN+ADDR_LENGTH])) return FALSE;

	if(pAddress) sscanf(&text[ADDR_COLUMN], "%x", pAddress);

	return TRUE;
}



// Finds the next address in the file, starting at the file pointer pFile.
BOOL CSymbolTable::GetNextAddress(FILE *pFile, unsigned *pAddress)
{
	char lineData[MAX_LINE_SIZE+1];
	fpos_t startPos;

	if(fgetpos(pFile, &startPos)) return FALSE;

	while(TRUE) {
		//if(fgets(lineData, MAX_LINE_SIZE, pFile) == NULL) {
		if(!GetLine(pFile, lineData, MAX_LINE_SIZE)) {
			// Check for EOF. If EOF, verify that last line has listing contents
			if(feof(pFile) && strlen(lineData) <= ASM_COLUMN) {
				fsetpos(pFile, &startPos);
				return FALSE;
			}

			// Check for error
			else if(ferror(pFile)) {
				fsetpos(pFile, &startPos);
				return FALSE;
			}
		}

		if(HasAddress(lineData, pAddress)) break;
	}

	// Restore file position
	fsetpos(pFile, &startPos);
	
	return TRUE;
}



// Uses the specified listing file (output by EE 425 NASM) to generate a symbole table.
// If deriveFilename is TRUE then this function will take fileName, remove the extension
// (if one is present) and add the ".lst" extension in its place. If deriveFilename
// is false then the function uses fileName exactly as it is.
// Returns TRUE if symbol table created. FALSE otherwise.
BOOL CSymbolTable::LoadSymbols(char *fileName, BOOL deriveFilename)
{
	FILE *pFile;
	char newFilename[MAX_FILE_PATH+sizeof(LISTING_EXT)];
	int i, strLength;
	int offset, length;

	unsigned address;

	char lineData[MAX_LINE_SIZE+1];
	char label[MAX_LINE_SIZE+1];


	// Flush any old data
	FlushTable();


	// New copy filename
	strncpy(newFilename, fileName, MAX_FILE_PATH);
	newFilename[MAX_FILE_PATH-1] = '\0';


	// Generate listing file name from fileName, if requested.
	if(deriveFilename) {
		strLength = strlen(fileName);

		// Find possible start of extension
		for(i = strLength-1; i >= 0 && newFilename[i] != '.' && newFilename[i] != '\\' && newFilename[i] != '/'; i--);
	    if(i == -1 || newFilename[i]=='\\' || newFilename[i]=='/') i = strLength;	// No extension
		else i = i + 1;		// Typical extension (e.g., fileName was "~/project/myFile.bin" or "~/project/.bin" or ".bin")

		// Copy new file extension
		strcpy(&newFilename[i], LISTING_EXT);
	}

	// Open file
	pFile = fopen(newFilename, "rb");	// Use binary so there's no end-of-line assumptions
	if(pFile == NULL) return FALSE;

	// Parse table
	while(TRUE) {
		//if(fgets(lineData, MAX_LINE_SIZE, pFile) == NULL) {
		if(!GetLine(pFile, lineData, MAX_LINE_SIZE)) {
			// Check for EOF. If EOF, verify that last line has listing contents
			if(feof(pFile) <= ASM_COLUMN) break;

			// Check for error
			else if(ferror(pFile)) break;
		}

		// Check if this line has a symbol
		if(strlen(lineData) > ASM_COLUMN && HasLabel(lineData, &offset, &length)) {
			// Extract label
			strncpy(label, &lineData[offset], length);
			label[length] = '\0';

			// This line might not have an address for it
			if(!HasAddress(lineData, &address)) {
				if(!GetNextAddress(pFile, &address)) break;
			}

			// Add to symbol table
			if(!InsertEntry(label, address)) {
				printf("Not enough memory to load symbol table.\n");
				break;
			}
		}
	}

	fclose(pFile);

	if(pTableList) return TRUE;
	else return FALSE;
}



/* The following version works. It makes a copy instead of returning pointer.
// Looks up the given physical address (physicalAddr) is in 
// the symbol table. If found, the associated symbol name 
// is saved in *name and the function returns TRUE.  
// Otherwise, returns FALSE. The argument maxSize is the 
// maximum number of bytes that can be stored in name, 
// including NULL. If no symobl table has been loaded, the
// function returns FALSE.
BOOL CSymbolTable::LookupAddress(unsigned int physicalAddr, char *name, int maxSize)
{
	TableEntry *pEntry;

	pEntry = pTableList;

	while(pEntry) {
		if(pEntry->address == physicalAddr) {
			strncpy(name, pEntry->symbol, maxSize);
			name[maxSize-1] = '\0';
			return TRUE;
		}
		pEntry = pEntry->next;
	}

	return FALSE;
}
*/



// Looks up the given physical address (physicalAddr) in 
// the symbol table. If found, a pointer to a string with
// the symbol name is saved in *name and the function 
// returns TRUE. Otherwise, it returns FALSE. 
// If no symobl table has been loaded, the function returns FALSE.
// NOTE 1: If there is more than one SYMBOL associated with
//         an address, the first matching symbol is
//         returned.
// NOTE 2: The symbol table is searched from high addresses
//         to low, so the first matching symbol will the 
//         one that appears last in the listing file.
// NOTE 3: Remember that for argument "name", the data 
//         pointed to (**name) is constant, not the pointers
//         themselves (*name and name)
// WARNING: The data pointed to by the pointer placed in *name MUST NOT BE
//          MODIFIED!
BOOL CSymbolTable::LookupAddress(unsigned int physicalAddr, const char **name)
{
	TableEntry *pEntry;

	pEntry = pTableList;

	while(pEntry) {
		if(pEntry->address == physicalAddr) {
			*name = (const char *)(pEntry->symbol);
			return TRUE;
		}
		pEntry = pEntry->next;
	}

	return FALSE;
}



// Looks up the given symbol in the symbol table.
// Returns false if the symbol is not found. Otherwise,
// it stores in *pPhysicalAddr the address of the symbol as indicated in
// the listing file.
// If no symobl table has been loaded, returns FALSE.
BOOL CSymbolTable::LookupSymbol(char *name, unsigned int *pPhysicalAddr)
{
	TableEntry *pEntry;

	pEntry = pTableList;

	while(pEntry) {
		if(!strcmp(name, pEntry->symbol)) {
			*pPhysicalAddr = pEntry->address;
			return TRUE;
		}
		pEntry = pEntry->next;
	}

	return FALSE;
}


// Checks if a symbol table has been loaded. Returns TRUE
// if so, FALSE otherwise.
BOOL CSymbolTable::TableLoaded(void)
{
	if(pTableList) return TRUE;
	return FALSE;
}

