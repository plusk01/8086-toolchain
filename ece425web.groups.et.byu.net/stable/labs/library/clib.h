// Header file for clib.s output routines

// Text Output:
void print(char *string, int length); // Print length bytes of string
void printNewLine(void);              // Print carriage return and line feed
void printChar(char c);               // Print character c
void printString(char *string);       // Print string

// Decimal Output:
void printInt(int val);
void printLong(long val);
void printUInt(unsigned val);
void printULong(unsigned long val);

// Hexadecimal Output:
void printByte(char val);
void printWord(int val);
void printDWord(long val);

// Program Control:
void exit(unsigned char code);        // Terminate with exit code

// PIC Functions:
void signalEOI(void);                 // Send non-specific EOI to PIC

