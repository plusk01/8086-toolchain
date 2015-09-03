// Help Messages ////////////////////////////////////////////////////

const char HelpMessage[] =
//	"||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| <- max length
	"Paramater Formats:\n"
	" Addresses:   Addresses can be of the following forms:\n"
	"                1) SEG:OFF          (Hex segment and offset)\n"
	"                2) ABh, 0xAB        (Hex)\n"
	"                3) 171              (Decimal)\n"
	"                4) Label            (A symbolic label from the listing file)\n"
	" Numbers:     Numbers can be hex (e.g. ABh, 0xAB) or decimal (e.g. 171)\n"
	"\t\n"
	"Command Descriptions:\n"
	" Ctrl+B        Press Ctrl+B to force a manual break in program execution.\n"
	" a N           Assert IRQ number N.\n"
	" addr SYMBOL   Give the address associated with the label SYMBOL.\n"
	" b             Display current breakpoints and information.\n"
	" b ADDR        Set breakpoint for instruction located at ADDR.\n"
	" c ID          Clear breakpoint with given ID, use ID 'all' to clear all.\n"
	" clear         Clear all breakpoints, memory watches, and register watches.\n"
	" d [ADDR] [N]  Disassemble N instructions at ADDR. If no ADDR, use CS:IP.\n"
	" ds [N]        Display at least N bytes of current stack as word values.\n"
	" dump ADDR [N] Hex & ASCII dump of (at least) N bytes at ADDR.\n"
	" dumpX ADDR [N]Show value at ADDR as hex, unsigned decimal, and signed\n"
	"                 decimal. X can be b (byte), w (word), d (double word).\n"
	" e [N]         Execute from current IP to end of program, or execute the\n"
	" eto ADDR      Execute until instruction at ADDR reached.\n"
	"                 number of instructions specified by N.\n"
	" f             Display current register monitors and their information.\n"
	" fm REG        Set monitor on register REG to halt when REG is modified.\n"
	" fb REG VAL    Set monitor on register REG to halt when REG becomes VAL.\n"
	" g ID          Clear reg monitor with given ID, use ID 'all' to clear all.\n"
	" h,?           Print this list of commands.\n"
	" hist [NUM]    Print addresses of recently executed/interrupted instructions.\n"
//	" i             *List interrupts available during execution.\n"
	" l [FILE] [AD] Load FILE at address AD. Loads previous file if none given.\n"
//	" llist [FILE]  Load symbols from listing file.\n"
	" m             Display current memory monitors and information.\n"
	" mX ADDR [LEN] Set memory monitor to stop execution when event type X occurs\n"
	"                 at memory location ADDR. X is first letter of event type\n"
	"                 (modify, read, write, access, or become), and LEN is number\n"
	"                 of bytes to monitor at ADDR (default 2). For 'become' mode a\n"
	"                 VALUE is needed; syntax is:  mb ADDR VALUE [LEN]   (LEN <= 4)\n"
	" n ID          Clear memory monitor with given ID, use ID 'all' to clear all.\n" 
	" o             Step over next instruction.\n"
	" pX ADDR VAL   Poke VAL into memory at ADDR. X can be b (byte), w (word),\n"
	"                 d (double word).\n"
/*	" pb ADDR VAL   Poke VAL into byte located at ADDR.\n"
	" pw ADDR VAL   Poke VAL into word located at ADDR.\n"
	" pd ADDR VAL   Poke VAL into double word located at ADDR.\n"*/
	" pr REG VAL    Poke VAL into register REG. REG is text name of the register.\n"
	" q             Quit simulator.\n"
	" r[d,u]        Print hex register values. Add d for decimal, u for unsigned.\n"
//	" reset         Reset the simulator processor to initial state.\n"
	" regs          Display register names and descriptions.\n"
	" s             Single step through next instruction.\n"
	" simptris      Toggle simptris mode.\n"
	" t [N]         Set timer tick interval to N instructions. Enter \"t 0\" to\n"
	"                 turn ticks off. Enter \"t\" to display current interval.\n"
	" v             Toggle verbose mode.\n"			// What should verbose do?
	" w             Wipe (clear) program output screen.\n"	// Wipe output screen?
	" ?,h           Print this list of commands \n";

/* OLD, from original SIMP simulator
	    printw("  Commands:\n");
	    printw("  a num        ; asserts interrupt at priority level num\n");
	    printw("  b            ; list all breakpoints\n");
	    printw("  b addr       ; set breakpoint for instruction at address\n");
	    printw("  c [num]      ; clear breakpoint, or all breakpoints (no [num])\n");
	    printw("  d x [len]    ; bin2a-like dump of memory from x (addr or label)\n");
	    printw("               ; for len locations. len = 20 default \n");
	    printw("  e [steps]    ; execute from current pc to end of program or\n");
	    printw("               ; specified number of instructions\n");
	    printw("  f            ; list all registers monitored\n");
	    printw("  f num        ; set register monitor for register num (use 0 for IMR)\n");
	    printw("  g [num]      ; clear reg. monitor, or all reg. monitors (no [num])\n");
	    printw("  h            ; print this list of commands \n");
	    printw("  i            ; list interrupts available during execution \n");
	    printw("  l [file]     ; load binary file, default is last file loaded\n");
	    printw("  m addr [len] ; set memory monitor on write in range\n");
	    printw("               ; from addr to addr+len.  default len = 3 bytes\n");
	    printw("  m            ; list all active monitor ranges\n");
	    printw("  o [num]      ; turn monitor range off, or all if no [num]\n");
	    printw("  p addr val   ; pokes val (32 bits) into word at address\n");
	    printw("  q            ; quit simulator \n");
	    printw("  r [h]        ; print state and register values [h: in hex]\n");
	    printw("  s            ; single step through next instruction \n");
	    printw("  t [num]      ; set timer tick interval to num instructions \n");
	    printw("               ; 't 0' turns off, 't' displays interval\n");
	    printw("  v            ; toggle verbose mode \n");
	    printw("  w            ; clear (wipe) output screen\n");
	    printw("  ?            ; print this list of commands \n");	  
*/

const char RegisterDescription[] = 
	"PIC Register Information:\n"
	"  IMR  Interrupt Mask Register    : A 1 bit disables the corresponding IRQ\n"
	"  ISR  In-Service Register        : Indicates which interrupts are in service\n"
	"  IRR  Interrupt Request Register : Indicates a device is waiting for service\n"
	"\t\n"
	"8086 Register Information:\n"
	"  General Purpose Registers (scratch registers)\n"
	"    AX (AH,AL)  Accumulator : Main arithmetic register\n"
	"    BX (BH,BL)  Base        : Generally used as a memory base or offset\n"
	"    CX (CH,CL)  Counter     : Generally used as a counter for loops\n"
	"    DX (DH,DL)  Data        : General 16-bit storage, division remainder\n"
	"\t\n"
	"  Offset Registers\n"
	"    IP  Instruction pointer : Current instruction offset\n"
	"    SP  Stack pointer       : Current stack offset\n"
	"    BP  Base pointer        : Base for referencing values stored on stack\n"
	"    SI  Source index        : General addressing, source offset in string ops\n"
	"    DI  Destination index   : General addressing, destination in string ops\n"
	"\t\n"
	"  Segment Registers\n"
	"    CS  Code segment   : Segment to which IP refers\n"
	"    SS  Stack segment  : Segment to which SP refers\n"
	"    DS  Data segment   : General addressing, usually for program's data area\n"
	"    ES  Extra segment  : General addressing, destination segment in string ops\n"
	"\t\n"
	"  Flags Register (Respectively bits 11,10,9,8,7,6,4,2,0)\n"
	"    OF  Overflow flag  : Indicates a signed arithmetic overflow occured\n"
	"    DF  Direction flag : Controls incr. direction in string ops (0=inc, 1=dec)\n"
	"    IF  Interrupt flag : Controls whether interrupts are enabled\n"
	"    TF  Trap flag      : Controls debug interrupt generation after instruction\n"
	"    SF  Sign flag      : Indicates a negative result or comparison\n"
	"    ZF  Zero flag      : Indicates a zero result or an equal comparison\n"
	"    AF  Auxiliary flag : Indicates adjustment is needed after BCD arithmetic\n"
	"    PF  Parity flag    : Indicates an even number of 1 bits\n"
	"    CF  Carry flag     : Indicates an arithmetic carry occured\n";


