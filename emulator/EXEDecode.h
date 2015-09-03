//
// EXEDecode.h: EXE file format description
//
// These structure defenitions allow easy access to the parts of the
// DOS executable (.EXE) file format. This file is used by the 
// CSimControl::LoadFileEXE() method to load EXE files.
//
//////////////////////////////////////////////////////////////////////


/* Format of EXE header
00-01  0x5a, 0x4d. This is the "magic number" of an EXE file. The first byte of the file is 0x4d and the second is 0x5a. 
02-03  The number of bytes in the last block of the program that are actually used. If this value is zero, that means the entire last block is used (i.e. the effective value is 512). 
04-05  Number of blocks in the file that are part of the EXE file. If [02-03] is non-zero, only that much of the last block is used. 
06-07  Number of relocation entries stored after the header. May be zero. 
08-09  Number of paragraphs in the header. The program's data begins just after the header, and this field can be used to calculate the appropriate file offset. The header includes the relocation entries. Note that some OSs and/or programs may fail if the header is not a multiple of 512 bytes. 
0A-0B  Number of paragraphs of additional memory that the program will need. This is the equivalent of the BSS size in a Unix program. The program can't be loaded if there isn't at least this much memory available to it. 
0C-0D  Maximum number of paragraphs of additional memory. Normally, the OS reserves all the remaining conventional memory for your program, but you can limit it with this field. 
0E-0F  Relative value of the stack segment. This value is added to the segment the program was loaded at, and the result is used to initialize the SS register. 
10-11  Initial value of the SP register. 
12-13  Word checksum. If set properly, the 16-bit sum of all words in the file should be zero. Usually, this isn't filled in. 
14-15  Initial value of the IP register. 
16-17  Initial value of the CS register, relative to the segment the program was loaded at. 
18-19  Offset of the first relocation item in the file. 
1A-1B  Overlay number. Normally zero, meaning that it's the main program. 
*/

/*

// The offset of the beginning of the EXE data
exe_data_start = exe.header_paragraphs * 16L;

// The offset of the byte just after the EXE data
extra_data_start = exe.blocks_in_file * 512L;
if (exe.bytes_in_last_block) extra_data_start -= (512 - exe.bytes_in_last_block);

*/


typedef struct EXE {
  unsigned short signature; /* == 0x5a4D */
  unsigned short bytes_in_last_block;
  unsigned short blocks_in_file;
  unsigned short num_relocs;
  unsigned short header_paragraphs;
  unsigned short min_extra_paragraphs;
  unsigned short max_extra_paragraphs;
  unsigned short ss;
  unsigned short sp;
  unsigned short checksum;
  unsigned short ip;
  unsigned short cs;
  unsigned short reloc_table_offset;
  unsigned short overlay_number;
} EXE;

typedef struct EXE_RELOC {
  unsigned short offset;
  unsigned short segment;
} EXE_RELOC;





