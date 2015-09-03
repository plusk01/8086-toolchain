//
// SimControl.cpp: implementation of the CSimControl class.
//
// This class is the heart of the simulator interface. It interprets
// commands and contains the main execution loop for executing
// instructions.
//
//////////////////////////////////////////////////////////////////////

#include "SimControl.h"

#include "Bios.h"
#include "EXEDecode.h"

#include "NasmDisassemble/disasm.h"

#include "Help.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSimControl::CSimControl()
{
	pCoreMemory = NULL;
	execPath = NULL;
	memoryMonitors = NULL;
	breakpoints = NULL;
	registerMonitors = NULL;
}


CSimControl::~CSimControl()
{
	// Clean up
	ClearMemMonitors();
	ClearRegMonitors();
	ClearBreakpoints();
	if(execPath) free(execPath); // if(execPath) delete execPath;
	if(pCoreMemory != NULL) free(pCoreMemory);
}


// Initializes the simulator. Should be called once before starting ExecLoop()
// Returns true if successful.
bool CSimControl::InitSim(char *path)
{
	int status;
	int i;
	int strSize;

	
	// Extract executable path
	if(path && strlen(path) > 0) {
		strSize = strlen(path) + 1;
		// execPath = new char[strlen(argv[0])+1];
		execPath = (char *)malloc(strSize);
		strncpy(execPath, path, strSize);
		for(i = strSize - 1; i >= 0 && execPath[i] != '/' && execPath[i] != '\\'; i--);
		execPath[i+1] = '\0';
	}

	// Initialize output window
	if(!SimIO.InitWindow()) {
		printf("ERROR: Couldn't initialize simulator window.\n");
		return false;
	}

	// Create Output Window
	if((status = ProgramWindow.InitProgramWindow(execPath)) != CRT_OK) {
		SimIO.PrintMessage("ERROR: Could not initialize program output window (error %u).\n", status);
		return false;
	}

	// Allocate real mode memory for simulator
	pCoreMemory = malloc(SIM_MEM_SIZE+64);	// Allocate extra 64 bytes to allow disassembly of opcodes whose args could extend beyond available memory
	if(pCoreMemory == NULL) {
		SimIO.PrintText("ERROR: Could not allocate real mode memory for simulator.\n");
		return false;
	}
	memset(pCoreMemory, MEM_INIT_VALUE, SIM_MEM_SIZE);	// Set memory to illegal opcode

	// Set up simulator core variables to reflect memory allocation
    memset(&M, 0, sizeof(M));	// Clear registers
	M.privatevp = pCoreMemory;
	M.mem_base = (u32)M.privatevp;
	M.mem_size = SIM_MEM_SIZE;
	
	// Init processor registers to standard values
	M.x86.R_SP = STACK_OFFSET;
	M.x86.R_IP = CODE_OFFSET;
	M.x86.R_DS = DATA_SEGMENT;
	M.x86.R_ES = EXTRA_SEGMENT;
	M.x86.R_SS = STACK_SEGMENT;
	M.x86.R_CS = CODE_SEGMENT;
	SET_FLAG(F_IF);

	// Setup interrupt controller
	InitPIC(EXT_INT_OFFSET);

	// Intialize Emulator Execution loop
	InitExec();		// Not really needed, but generates extra code in debug mode.

	// Register simulator (for communication and output)
	SetSimControl(this);

	// Setup interrupt vectors
	SetUpInterrupts();

	// Initialize history
	ClearHistory();


	// Init internal values
	tickInterval = DEFAULT_TICK_INTERVAL;
	tickCount = 0;
	verboseMode = false;

	simptris = false;

	memoryMonitorsActive = false;
	registerMonitorsActive = false;
	breakpointsActive = false;

	return true;
}


// Empties the instruction and interrupt histories which are used by the history command.
void CSimControl::ClearHistory(void)
{
	memset(instHistory, 0xFF, sizeof(*instHistory) * HISTORY_LENGTH);
	memset(intHistory, 0xFF, sizeof(*intHistory) * HISTORY_LENGTH);
	instHistoryOffset = 0;
	intHistoryOffset = 0;
}


// Resets internal variables for a second run: resets PIC and saves SS:SP address to detect final "ret".
// Registers should be initialized before beginning execution.
void CSimControl::ResetProcessor(void)
{
	// Reset the state of the interrupt controller
	ResetPIC();

	// Reset history
	ClearHistory();

	// Reset internal simulator data
	instructionCount = 0;
	tickCount = 0;

	stopFlag = false;
	systemHalted = false;

	stackStart = PHYSICAL_ADDR(M.x86.R_SS, M.x86.R_SP);
}


/* The code for EnqueueInstruction was moved inside the main execution loop for speed.
   The code is used to enque instruction for the history command.

// Add current CS:IP to the instruction history
void CSimControl::EnqueueInstruction(void)
{
	instHistory[instHistoryOffset].segment = M.x86.R_CS;
	instHistory[instHistoryOffset].offset = M.x86.R_IP;
	instHistoryOffset++;
	if(instHistoryOffset >= HISTORY_LENGTH) instHistoryOffset = 0;
}
*/


// Adds current CS:IP to the interrupt history. Called when interrupts occur.
// The queue is used by the history command.
void CSimControl::EnqueueInterrupt(int interruptVector)
{
	intHistory[intHistoryOffset].segment = M.x86.R_CS;
	intHistory[intHistoryOffset].offset = M.x86.R_IP;
	intHistory[intHistoryOffset].vector = interruptVector;
	intHistoryOffset++;

	if(intHistoryOffset >= HISTORY_LENGTH) intHistoryOffset = 0;
}


// Executes repeatCount instructions.
// If repeatCount is 0, will execute indefinately.
void CSimControl::Execute(unsigned int repeatCount)
{
	char c;
	int startICount;

	startICount = instructionCount;

	// if numInstructions is 0, execute the larges possible number
	if(repeatCount == 0) {
		repeatCount = (unsigned int)(-1);
	}
	
	// Initialize register monitors
	InitRegisterMonitorContents();

	while(repeatCount > 0) {
		// Check termination flags
		if(stopFlag) break;

		// Update simulator data
		instructionCount++;
		tickCount++;

		// Print extra infor for verbose mode
		if(verboseMode) {
			SimIO.PrintMessage("  %d>", instructionCount);
			DisassembleNext();
		}

		// Handle simptrs
		if(simptris) {
			if(instructionCount % SIMP_TIME_SCALE == 0) Simptris.handle_simptris();
		}
		
		// Check for input
		if(ProgramWindow.KeyPress()) c = ProgramWindow.GetProgramInput();
		else c = KEY_NONE;

		if(c == CTRL_B) { SimIO.PrintText("< Manual break > - Program not yet terminated.\n"); return; }
		
		// Check for interrupts in order of priority
		if(c == KEY_RESET) FlagInterrupt(RESET_INT);
		else if((tickInterval && tickCount >= tickInterval) || c == KEY_TICK) {
			tickCount = 0;
			FlagInterrupt(TICK_INT);
		}
		else if(c != KEY_NONE) {
			SetKeyboardBuffer(c);
			FlagInterrupt(KEYBOARD_INT);
		}
		
		// Add to instruction history (This code is done here for speed rather than via a method)
		instHistory[instHistoryOffset].segment = M.x86.R_CS;
		instHistory[instHistoryOffset].offset = M.x86.R_IP;
		if(instHistoryOffset >= HISTORY_LENGTH-1) instHistoryOffset = 0;
		else instHistoryOffset++;

		// Execute a single instruction
		X86EMU_exec();

		// Check any breakpoints
		if(breakpointsActive) CheckBreakpoints();

		// Check register monitors
		if(registerMonitorsActive) CheckRegisterMonitors();

		repeatCount--;
	}

	// Check if the simulator didn't finish
	if(repeatCount != 0) {
			SimIO.PrintMessage("Ran for %u instructions (%u total).\n", instructionCount - startICount, instructionCount);
	}

	// Reset flags for next time
	stopFlag = false;
	if(systemHalted) M.x86.intr &= ~(INTR_HALTED);	// Un-halt cpu simulator
}


// Displays a list of the currently enabled breakpoints.
void CSimControl::DisplayBreakpoints(void)
{
	Breakpoint *current;

	const char *symbolName;

	if(breakpoints == NULL) {
		SimIO.PrintText("No current breakpoints set.\n");
	}
	else {
		current = breakpoints;
		SimIO.PrintText("  ID   | Logical Addr | Physical Addr | Symbol\n");
		SimIO.PrintText("  -----|--------------|---------------|------------\n");
		//                 "1024 | 0000:0000    | 00000h"
		while(current != NULL) {
			SimIO.PrintMessage("  %-4d | %04X:%04X    | %05Xh        |", current->ID, current->address.segment, current->address.offset, current->address.physical);
			if(symbolTable.LookupAddress(current->address.physical, &symbolName)) {
				SimIO.PrintMessage(" %s\n", symbolName);
			}
			else SimIO.PrintText("\n");
			current = current->next;
		}
	}
}


// Deletes breakpoint with the passed ID
// Returns ID of the deleted breakpoint, -1 if an error occurs.
int CSimControl::ClearBreakpoint(int ID)
{
	Breakpoint *current, *previous;
	int returnValue;

	returnValue = -1;

	if(breakpoints != NULL) {
		// Find breakpoint in list, then remove
		previous = NULL;
		current = breakpoints;
		
		while(current != NULL && current->ID != ID) {
			previous = current;
			current = current->next;
		}

		if(current != NULL && current->ID == ID) {
			// check if this was the first entry
			if(previous == NULL) breakpoints = current->next;
			else previous->next = current->next;

			free(current);
			returnValue = ID;
		}
	}
	
	// Check if no more breakpoints
	if(breakpoints == NULL) breakpointsActive = false;

	return returnValue;
}


// Deletes all breakpoints.
void CSimControl::ClearBreakpoints(void)
{
	Breakpoint *current, *next;

	current = breakpoints;

	while(current != NULL) {
		next = current->next;
		free(current);
		current = next;
	}

	breakpointsActive = false;

	breakpoints = NULL;
}


// Adds a breakpoint for address of the given type (see header file for different types)
// Returns ID of the new breakpoint if successfull, otherwise returns -1.
int CSimControl::SetBreakpoint(Address *address, int type)
{
	Breakpoint *current;
	int ID;

	ID = 0;

	if(breakpoints == NULL) {
		// This is first to be added
		breakpoints = (Breakpoint *)malloc(sizeof(Breakpoint));
		if(breakpoints == NULL) {
			return -1;
		}
		current = breakpoints;
	}
	else {
		// Find end of list
		current = breakpoints;

		while(current->next != NULL) {
			current = current->next;
		}
		ID = current->ID + 1;
		current->next = (Breakpoint *)malloc(sizeof(Breakpoint));
		if(current->next == NULL) {
			return -1;
		}
		current = current->next;
	}

	current->ID = ID;
	current->address.segment = address->segment;
	current->address.offset = address->offset;
	current->address.physical = address->physical;
	current->type = type;
	current->next = NULL;

	breakpointsActive = true;

	return ID;
}


// Saves copies of all monitired registers.
// This is used during execution to determine if they
// are changed by an instruction.
void CSimControl::InitRegisterMonitorContents(void)
{
	RegisterMonitor *current;

	current = registerMonitors;
	
	while(current != NULL) {
		switch(current->registerNum) {
		case AH_REG: current->lastValue = M.x86.R_AH; break;
		case AL_REG: current->lastValue = M.x86.R_AL; break;
		case BH_REG: current->lastValue = M.x86.R_BH; break;
		case BL_REG: current->lastValue = M.x86.R_BL; break;
		case CH_REG: current->lastValue = M.x86.R_CH; break;
		case CL_REG: current->lastValue = M.x86.R_CL; break;
		case DH_REG: current->lastValue = M.x86.R_DH; break;
		case DL_REG: current->lastValue = M.x86.R_DL; break;
		case AX_REG: current->lastValue = M.x86.R_AX; break;
		case BX_REG: current->lastValue = M.x86.R_BX; break;
		case CX_REG: current->lastValue = M.x86.R_CS; break;
		case DX_REG: current->lastValue = M.x86.R_DX; break;
		case CS_REG: current->lastValue = M.x86.R_CS; break;
		case DS_REG: current->lastValue = M.x86.R_DS; break;
		case ES_REG: current->lastValue = M.x86.R_ES; break;
		case SS_REG: current->lastValue = M.x86.R_SS; break;
		case IP_REG: current->lastValue = M.x86.R_IP; break;
		case SP_REG: current->lastValue = M.x86.R_SP; break;
		case BP_REG: current->lastValue = M.x86.R_BP; break;
		case SI_REG: current->lastValue = M.x86.R_SI; break;
		case DI_REG: current->lastValue = M.x86.R_DI; break;
		case OF_REG: current->lastValue = OF_BIT; break;
		case DF_REG: current->lastValue = DF_BIT; break;
		case IF_REG: current->lastValue = IF_BIT; break;
		case TF_REG: current->lastValue = TF_BIT; break;
		case SF_REG: current->lastValue = SF_BIT; break;
		case ZF_REG: current->lastValue = ZF_BIT; break;
		case AF_REG: current->lastValue = AF_BIT; break;
		case PF_REG: current->lastValue = PF_BIT; break;
		case CF_REG: current->lastValue = CF_BIT; break;

		case IMR_REG: current->lastValue = ReadIMR(); break;
		case ISR_REG: current->lastValue = ReadISR(); break;
		case IRR_REG: current->lastValue = ReadIRR(); break;
		}
		
		current = current->next;
	}
}


// Displays a list of the current register monitors.
void CSimControl::DisplayRegMonitors(void)
{
	RegisterMonitor *current;

	if(registerMonitors == NULL) {
		SimIO.PrintText("No current register monitors.\n");
	}
	else {
		current = registerMonitors;
		SimIO.PrintText("  ID    | Register | Mode\n");
		SimIO.PrintText("  ------|----------|-----------------\n");
		//                 "  1024  | AX       | Become FFFFh"
		while(current != NULL) {
			SimIO.PrintMessage("  %-5d | %-8s | ", current->ID, registerName[current->registerNum]);
			switch(current->mode) {
				case ON_READ:
				case ON_WRITE:
				case ON_ACCESS:	SimIO.PrintText("* INVALID MODE *\n"); break;
				case ON_MODIFY: SimIO.PrintText("Modify\n"); break;
				case ON_BECOME: SimIO.PrintMessage("Become %d (%0Xh)\n", current->value, current->value); break;
			default:		SimIO.PrintMessage("* INVALID MODE *\n");
			}
			current = current->next;
		}
	}
}


// Deletes the register monitor with the given ID.
// Returns ID of deleted monitor if it exists, otherwise returns -1.
int CSimControl::ClearRegMonitor(int ID)
{
	RegisterMonitor *current, *previous;
	int returnValue;
	
	returnValue = -1;
	
	if(registerMonitors != NULL) {
		// Find register monitor in list, then remove
		previous = NULL;
		current = registerMonitors;
		
		while(current != NULL && current->ID != ID) {
			previous = current;
			current = current->next;
		}
		
		if(current == NULL) returnValue = -1;
		else if(current->ID == ID) {
			// Check if this was first entry
			if(previous == NULL) registerMonitors = current->next;
			else previous->next = current->next;

			free(current);
			returnValue = ID;
		}
	}
	
	// Check if no more monitors
	if(registerMonitors == NULL) registerMonitorsActive = false;

	return returnValue;
}


// Deletes all register monitors.
void CSimControl::ClearRegMonitors(void)
{
	RegisterMonitor *current, *next;

	current = registerMonitors;

	while(current != NULL) {
		next = current->next;
		free(current);
		current = next;
	}

	registerMonitorsActive = false;

	registerMonitors = NULL;
}


// Adds a register monitor for the indicated register and using the indicated mode.
// The argument "value" specifies the become mode value on which to stop.
// (see header file for different modes)
// Returns ID of the new monitor if successfull, otherwise returns -1.
int CSimControl::SetRegMonitor(int registerNum, int mode, unsigned short value)
{
	RegisterMonitor *current;
	int ID;

	ID = 0;

	if(registerMonitors == NULL) {
		// This is first to be added
		registerMonitors = (RegisterMonitor *)malloc(sizeof(RegisterMonitor));
		if(registerMonitors == NULL) {
			// SimIO.PrintText("ERROR: Could not allocate memory for new monitor.\n");
			return -1;
		}
		current = registerMonitors;
	}
	else {
		// Find end of list
		current = registerMonitors;

		while(current->next != NULL) current = current->next;
		ID = current->ID + 1;
		current->next = (RegisterMonitor *)malloc(sizeof(RegisterMonitor));
		if(registerMonitors == NULL) {
			// SimIO.PrintText("ERROR: Could not allocate memory for new monitor.\n");
			return -1;
		}
		current = current->next;
	}

	current->ID = ID;
	current->registerNum = registerNum;
	current->mode = mode;
	current->next = NULL;
	current->value = value;

	registerMonitorsActive = true;

	return ID;
}


// Displays a list of the current memory monitors.
void CSimControl::DisplayMemMonitors(void)
{
	MemoryMonitor *current;
	const char *symbolName;

	if(memoryMonitors == NULL) {
		SimIO.PrintText("No current memory monitors.\n");
	}
	else {
		current = memoryMonitors;

		SimIO.PrintText("  ID  | Log. Addr | Phys.  | Symbol           | Size   | Mode\n");
		SimIO.PrintText("  ----|-----------|--------|------------------|--------|---------------------\n");
		//              "  999 | 0000:0000 | 00000h | AReallyLongLabel | 000000 | Become 65536 (FFFFh)"
		//              "  999 | 0000:0000 | 00000h | 12345678901234567| 000000 | Access"
		//               01234567890123456789012345678901234567890123456789012345678901234567890123456789
		while(current != NULL) {
			// Lookup symbol for address
			if(!symbolTable.LookupAddress(current->address.physical, &symbolName)) symbolName = "\0";

			// Display breakpoint information
			SimIO.PrintMessage("  %-3d | %04X:%04X | %05Xh | %-17s| %-6d | ", current->ID, current->address.segment, current->address.offset, current->address.physical, symbolName, current->size);
			switch(current->mode) {
				case ON_READ:	SimIO.PrintText("Read\n"); break;
				case ON_WRITE:	SimIO.PrintText("Write\n"); break;
				case ON_MODIFY: SimIO.PrintText("Modify\n"); break;
				case ON_ACCESS:	SimIO.PrintText("Access\n"); break;
				case ON_BECOME: SimIO.PrintMessage("Become %d (%0Xh)\n", current->value, current->value); break;
			default:		SimIO.PrintText("* INVALID MODE *\n");
			}
			current = current->next;
		}
	}
}


// Deletes the memory monitor with the given ID.
// Returns ID of the deleted monitor if it exists, otherwise returns -1.
int CSimControl::ClearMemMonitor(int ID)
{
	MemoryMonitor *current, *previous;
	int returnValue;

	returnValue = -1;

	if(memoryMonitors != NULL) {
		// Find memory monitor in list, then remove
		previous = NULL;
		current = memoryMonitors;
		
		while(current != NULL && current->ID != ID) {
			previous = current;
			current = current->next;
		}
		
		if(current == NULL) returnValue = -1;
		else if(current->ID == ID) {
			// Check if this was the first entry
			if(previous == NULL) memoryMonitors = current->next;
			else previous->next = current->next;
			
			free(current);
			returnValue = ID;
		}
	}
	
	// Check if no more monitors
	if(memoryMonitors == NULL) memoryMonitorsActive = false;

	return returnValue;
}


// Deletes all current memory monitors.
void CSimControl::ClearMemMonitors(void)
{
	MemoryMonitor *current, *next;

	current = memoryMonitors;

	while(current != NULL) {
		next = current->next;
		free(current);
		current = next;
	}

	memoryMonitorsActive = false;

	memoryMonitors = NULL;
}


// Adds a memory monitor for the given address, of the given size, and in the given mode.
// The argument value specifies the become mode value on which to stop.
// (see header file for different modes)
// Returns monitor ID of the new monitor if successful, otherwise -1
int CSimControl::SetMemMonitor(Address *address, int size, int mode, unsigned int value)
{
	MemoryMonitor *current;
	int ID;

	ID = 0;

	if(memoryMonitors == NULL) {
		// This is first to be added
		memoryMonitors = (MemoryMonitor *)malloc(sizeof(MemoryMonitor));
		if(memoryMonitors == NULL) {
			// SimIO.PrintText("ERROR: Could not allocate memory for new monitor.\n");
			return -1;
		}
		current = memoryMonitors;
	}
	else {
		// Find end of list
		current = memoryMonitors;

		while(current->next != NULL) current = current->next;
		ID = current->ID + 1;
		current->next = (MemoryMonitor *)malloc(sizeof(MemoryMonitor));
		if(memoryMonitors == NULL) {
			// SimIO.PrintText("ERROR: Could not allocate memory for new monitor.\n");
			return -1;
		}
		current = current->next;
	}

	current->ID = ID;
	current->address.segment = address->segment;
	current->address.offset = address->offset;
	current->address.physical = address->physical;
	current->size = size;
	current->mode = mode;
	current->value = value;
	current->next = NULL;

	memoryMonitorsActive = true;

	return ID;
}


// Load an DOS EXE format file for execution.
// This function is not fully tested.
// Returns FILE_LOADED if successful, otherwise FILE_NOT_LOADED.
int CSimControl::LoadFileEXE(char *filename, int segment)
{
	FILE *file;
	EXE exe;
	int exeSize, sizeRead;
	int exe_data_start, extra_data_start;
	int baseAddr;
	int i;
	unsigned short *wordPtr;
	EXE_RELOC *relocTable;
	unsigned char *header;


	baseAddr = segment << 4;

	// Open file
	file = fopen(filename, "rb");
	if(file == NULL) return FILE_NOT_LOADED;

	// Read main file header
	sizeRead = fread((void *)&exe, 1, sizeof(exe), file);
	if(sizeRead != sizeof(exe)) {
		SimIO.PrintText("ERROR: Could not load EXE header.\n");
		return FILE_NOT_LOADED;
	}
	
	// Calculate offset of the beginning of the EXE data
	exe_data_start = exe.header_paragraphs * 16L;

	// Calculate offset of the byte just after the EXE data
	extra_data_start = exe.blocks_in_file * 512L;
	if (exe.bytes_in_last_block) extra_data_start -= (512 - exe.bytes_in_last_block);

	// Check binary executable size
	exeSize = extra_data_start - exe_data_start;
	if(exeSize > (int)M.mem_size-baseAddr) {
		fclose(file);
		SimIO.PrintMessage("ERROR: Binary size (%d bytes) exceeds simulator memory (%d bytes)\nwhen loaded at physical address %04Xh.\n", exeSize, M.mem_size, baseAddr);
		return FILE_NOT_LOADED;
	}

	// Load rest of header data
	header = (unsigned char *)malloc(exe_data_start - sizeof(EXE));
	if(header == NULL) {
		fclose(file);
		SimIO.PrintText("ERROR: Could not allocate memory for EXE header.\n");
		return FILE_NOT_LOADED;
	}
	fread(header, 1, exe_data_start - sizeof(EXE), file);

	// Load executable file data into simulator memory
	sizeRead = fread((void *)((char *)(M.privatevp) + baseAddr), 1, exeSize, file);

	// Was the file fully loaded?
	if(sizeRead != exeSize) return FILE_NOT_LOADED;

	// Adjust segments in EXE according to load address segment
	relocTable = (EXE_RELOC *)(header + exe.reloc_table_offset - sizeof(EXE));
	for(i = 0; i < exe.num_relocs; i++) {
		wordPtr = (unsigned short *)((char *)M.privatevp + PHYSICAL_ADDR(segment + relocTable[i].segment, relocTable[i].offset));
		(*wordPtr) += segment;
	}

	// Setup up processor to run program
	memset(&M.x86, 0, sizeof(M.x86));	// Clear registers
	M.x86.R_SS = segment + exe.ss;
	M.x86.R_CS = segment + exe.cs;
	// Init ES, DS, not necessary for EXEs (?)
	M.x86.R_SP = exe.sp;
	M.x86.R_IP = exe.ip;
	SET_FLAG(F_IF);

	// Clean up
	free(header);
	fclose(file);

	symbolTable.FlushTable();

	return FILE_LOADED;
}


// Loads the indicated file at the indicated physical base address.
// The logical portion of "address" is NOT used, only the physical portion (See defenition of the Address type).
// Returns FILE_LOADED if successful, otherwise FILE_NOT_LOADED.
int CSimControl::LoadFile(char *filename, Address *address)
{
	FILE *file;
	int fileSize;
	int sizeRead;

	// Open filename

	file = fopen(filename, "rb");
	if(file == NULL) return FILE_NOT_LOADED;

	// Get filesize
	fseek(file, 0, SEEK_END);
	fileSize = ftell(file);

	fseek(file, 0, SEEK_SET);

	if(fileSize > (int)M.mem_size - address->physical) {
		fclose(file);
		SimIO.PrintMessage("ERROR: File size (%d bytes) exceeds simulator memory (%d bytes)\nwhen loaded at physical address %04Xh.\n", fileSize, M.mem_size, address->physical);
		return FILE_NOT_LOADED;
	}

	// Load file into simulator memory
	sizeRead = fread((void *)((char *)(M.privatevp) + address->physical), 1, fileSize, file);

	fclose(file);

	// Was the file fully loaded?
	if(sizeRead != fileSize) return FILE_NOT_LOADED;

	// Attempt to load symbol table from .lst file
	if(!symbolTable.LoadSymbols(filename, TRUE)) symbolTable.FlushTable();

	return FILE_LOADED;
}


// If it is a valid numeric address string (of any of the allowed forms)
// this function converts it to the numeric address.
// If it's a label in the symbol table, it load sthe address from there.
// Returns false if the address could not be obtained (i.e., not a valid
// number or the symbol is not in the symbol table).
bool CSimControl::GetAddress(char *string, Address *pAddress)
{
	unsigned physAddress;

	// Is it a numeric address?
	if(IsAddress(string)) {
		InterpretAddress(string, pAddress);
		return true;
	}

	// Attempt table lookup
	if(symbolTable.LookupSymbol(string, &physAddress)) {
		pAddress->physical = physAddress;
		pAddress->offset = OFFSET(physAddress);
		pAddress->segment = SEGMENT(physAddress);
		return true;
	}

	return false;
}



// Sets the indicated register to the indicated value.
// Used by the poke register commands.
// Returns value (adjusted for size of register).
unsigned CSimControl::SetRegister(int registerNum, unsigned value)
{
	switch(registerNum) {
	case AH_REG: value &= 0xFF; M.x86.R_AH = (unsigned char)value; break;
	case AL_REG: value &= 0xFF; M.x86.R_AL = (unsigned char)value; break;
	case BH_REG: value &= 0xFF; M.x86.R_BH = (unsigned char)value; break;
	case BL_REG: value &= 0xFF; M.x86.R_BL = (unsigned char)value; break;
	case CH_REG: value &= 0xFF; M.x86.R_CH = (unsigned char)value; break;
	case CL_REG: value &= 0xFF; M.x86.R_CL = (unsigned char)value; break;
	case DH_REG: value &= 0xFF; M.x86.R_DH = (unsigned char)value; break;
	case DL_REG: value &= 0xFF; M.x86.R_DL = (unsigned char)value; break;
	case AX_REG: M.x86.R_AX = (unsigned short)value; break;
	case BX_REG: M.x86.R_BX = (unsigned short)value; break;
	case CX_REG: M.x86.R_CS = (unsigned short)value; break;
	case DX_REG: M.x86.R_DX = (unsigned short)value; break;
	case CS_REG: M.x86.R_CS = (unsigned short)value; break;
	case DS_REG: M.x86.R_DS = (unsigned short)value; break;
	case ES_REG: M.x86.R_ES = (unsigned short)value; break;
	case SS_REG: M.x86.R_SS = (unsigned short)value; break;
	case IP_REG: M.x86.R_IP = (unsigned short)value; break;
	case SP_REG: M.x86.R_SP = (unsigned short)value; break;
	case BP_REG: M.x86.R_BP = (unsigned short)value; break;
	case SI_REG: M.x86.R_SI = (unsigned short)value; break;
	case DI_REG: M.x86.R_DI = (unsigned short)value; break;
	case OF_REG: value &= 1; (value) ? SET_FLAG(F_OF) : CLEAR_FLAG(F_OF); break;
	case DF_REG: value &= 1; (value) ? SET_FLAG(F_DF) : CLEAR_FLAG(F_DF); break;
	case IF_REG: value &= 1; (value) ? SET_FLAG(F_IF) : CLEAR_FLAG(F_IF); break;
	case TF_REG: value &= 1; (value) ? SET_FLAG(F_TF) : CLEAR_FLAG(F_TF); break;
	case SF_REG: value &= 1; (value) ? SET_FLAG(F_SF) : CLEAR_FLAG(F_SF); break;
	case ZF_REG: value &= 1; (value) ? SET_FLAG(F_ZF) : CLEAR_FLAG(F_ZF); break;
	case AF_REG: value &= 1; (value) ? SET_FLAG(F_AF) : CLEAR_FLAG(F_AF); break;
	case PF_REG: value &= 1; (value) ? SET_FLAG(F_PF) : CLEAR_FLAG(F_PF); break;
	case CF_REG: value &= 1; (value) ? SET_FLAG(F_CF) : CLEAR_FLAG(F_CF); break;
	case IMR_REG: WriteIMR(value);
	case ISR_REG: break;
	case IRR_REG: break;
	}
	return value;
}


// Sets "size" bytes of the simulator memory at "address" to "value".
// Used by the poke memory command.
void CSimControl::SetMemory(int address, unsigned value, int size)
{
	unsigned char *dest;

	dest = (unsigned char *)M.privatevp + address;

	switch(size) {
	case 1:
		dest[0] = value & 0xFF;
		return;
	case 2:
		dest[0] = value & 0xFF;
		dest[1] = (value >> 8) & 0xFF;
		return;
	case 3:
		dest[0] = value & 0xFF;
		dest[1] = (value >> 8) & 0xFF;
		dest[2] = (value >> 16) & 0xFF;
		return;
	case 4:
		dest[0] = value & 0xFF;
		dest[1] = (value >> 8) & 0xFF;
		dest[2] = (value >> 16) & 0xFF;
		dest[3] = (value >> 24) & 0xFF;
		return;
	}
}


// Displays the contents of the CPU and PIC registers in the indicated format.
void CSimControl::DisplayRegisters(int format)
{
	if(format == DECIMAL) {
		SimIO.PrintMessage("  Instruction Count: %-11u PIC Regs: ( IMR:%+04d ISR:%+04d IRR:%+04d )\n", instructionCount, (int)(short)ReadIMR(), (int)(short)ReadISR(), (int)(short)ReadIRR());
		SimIO.PrintMessage("  AX:%+06d { AH:%+04d AL:%+04d }  BX:%+06d { BH:%+04d BL:%+04d }\n", (int)(short)M.x86.R_AX, (int)(char)M.x86.R_AH, (int)(char)M.x86.R_AL, (int)(short)M.x86.R_BX, (int)(char)M.x86.R_BH, (int)(char)M.x86.R_BL);
		SimIO.PrintMessage("  CX:%+06d { CH:%+04d CL:%+04d }  DX:%+06d { DH:%+04d DL:%+04d }\n", (int)(short)M.x86.R_CX, (int)(char)M.x86.R_CH, (int)(char)M.x86.R_CL, (int)(short)M.x86.R_DX, (int)(char)M.x86.R_DH, (int)(char)M.x86.R_DL);
		SimIO.PrintMessage("  SI:%+06d DI:%+06d IP:%+06d  SP:%+06d BP:%+06d\n", (int)(short)M.x86.R_SI, (int)(short)M.x86.R_DI, (int)(short)M.x86.R_IP, (int)(short)M.x86.R_SP, (int)(short)M.x86.R_BP);
		SimIO.PrintMessage("  DS:%+06d ES:%+06d CS:%+06d  SS:%+06d\n", (int)(short)M.x86.R_DS, (int)(short)M.x86.R_ES, (int)(short)M.x86.R_CS, (int)(short)M.x86.R_SS);
		SimIO.PrintMessage("  { OF:%d DF:%d IF:%d TF:%d SF:%d ZF:%d AF:%d PF:%d CF:%d }\n", (int)GET_FLAG(F_OF), (int)GET_FLAG(F_DF), (int)GET_FLAG(F_IF), (int)GET_FLAG(F_TF), (int)GET_FLAG(F_SF), (int)GET_FLAG(F_ZF), (int)GET_FLAG(F_AF), (int)GET_FLAG(F_PF), (int)GET_FLAG(F_CF));
	}
	else if(format == UNSIGNED) {
		SimIO.PrintMessage("  Instruction Count: %-8u PIC Regs: ( IMR:%03u ISR:%03u IRR:%03u )\n", instructionCount, (unsigned)ReadIMR(), (unsigned)ReadISR(), (unsigned)ReadIRR());
		SimIO.PrintMessage("  AX:%05u { AH:%03u AL:%03u }  BX:%05u { BH:%03u BL:%03u }\n", (unsigned)M.x86.R_AX, (unsigned)M.x86.R_AH, (unsigned)M.x86.R_AL, (unsigned)M.x86.R_BX, (unsigned)M.x86.R_BH, (unsigned)M.x86.R_BL);
		SimIO.PrintMessage("  CX:%05u { CH:%03u CL:%03u }  DX:%05u { DH:%03u DL:%03u }\n", (unsigned)M.x86.R_CX, (unsigned)M.x86.R_CH, (unsigned)M.x86.R_CL, (unsigned)M.x86.R_DX, (unsigned)M.x86.R_DH, (unsigned)M.x86.R_DL);
		SimIO.PrintMessage("  SI:%05u DI:%05u IP:%05u  SP:%05u BP:%05u\n", (unsigned)M.x86.R_SI, (unsigned)M.x86.R_DI, (unsigned)M.x86.R_IP, (unsigned)M.x86.R_SP, (unsigned)M.x86.R_BP);
		SimIO.PrintMessage("  DS:%05u ES:%05u CS:%05u  SS:%05u\n", (unsigned)M.x86.R_DS, (unsigned)M.x86.R_ES, (unsigned)M.x86.R_CS, (unsigned)M.x86.R_SS);
		SimIO.PrintMessage("  { OF:%d DF:%d IF:%d TF:%d SF:%d ZF:%d AF:%d PF:%d CF:%d }\n", (int)GET_FLAG(F_OF), (int)GET_FLAG(F_DF), (int)GET_FLAG(F_IF), (int)GET_FLAG(F_TF), (int)GET_FLAG(F_SF), (int)GET_FLAG(F_ZF), (int)GET_FLAG(F_AF), (int)GET_FLAG(F_PF), (int)GET_FLAG(F_CF));
	}
	else {
		SimIO.PrintMessage("  Instruction Count: %-17u PIC Regs: ( IMR:%02X ISR:%02X IRR:%02X )\n", instructionCount, (unsigned)ReadIMR(), (unsigned)ReadISR(), (unsigned)ReadIRR());
		SimIO.PrintMessage("  AX:%04X BX:%04X CX:%04X DX:%04X SI:%04X DI:%04X BP:%04X SP:%04X IP:%04X\n", (unsigned)M.x86.R_AX, (unsigned)M.x86.R_BX, (unsigned)M.x86.R_CX, (unsigned)M.x86.R_DX, (unsigned)M.x86.R_SI, (unsigned)M.x86.R_DI, (unsigned)M.x86.R_BP, (unsigned)M.x86.R_SP, (unsigned)M.x86.R_IP);
		SimIO.PrintMessage("  { O:%d D:%d I:%d T:%d S:%d Z:%d A:%d P:%d C:%d } DS:%04X ES:%04X SS:%04X CS:%04X\n", (int)GET_FLAG(F_OF), (int)GET_FLAG(F_DF), (int)GET_FLAG(F_IF), (int)GET_FLAG(F_TF), (int)GET_FLAG(F_SF), (int)GET_FLAG(F_ZF), (int)GET_FLAG(F_AF), (int)GET_FLAG(F_PF), (int)GET_FLAG(F_CF), (unsigned)M.x86.R_DS, (unsigned)M.x86.R_ES, (unsigned)M.x86.R_SS, (unsigned)M.x86.R_CS);
	}
}


// Dumps memory in rows of 16 bytes. Uses a logical address to calculate a starting address
// that is a multiple of 16. Prints a minimum of numBytes from the start address.
void CSimControl::DumpMemory(void *memory, int segment, int offset, int numBytes)
{
	unsigned char *ptr;
	int i, byteCount;

	numBytes += offset % 16;
	offset -= offset % 16;

	ptr = (unsigned char *)memory + PHYSICAL_ADDR(segment, offset);

	// Print header
	SimIO.PrintText(" Seg:Off  |  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F |    Text dump    \n");
	SimIO.PrintText("----------|-------------------------------------------------|-----------------\n");
	//                 "0000:0000 | CD CD CD CD CD CD CD CD CD CD CD CD CD CD CD CD | ----------------"
	// Dump memory 1 line at a time (16 bytes per line)
	for(byteCount = 0; byteCount < numBytes; byteCount += 16) {
		// Print segment:offset
		SimIO.PrintMessage("%04X:%04X | ", segment, offset);

		// Print hex values of 16 bytes
		for(i = 0; i < 16; i++) {
			SimIO.PrintMessage("%02X ", (int)ptr[i]);
		}

		SimIO.PrintText("| ");
		
		// Print ascii values
		for(i = 0; i < 16; i++) {
			SimIO.DumpChar(ptr[i]);
		}

		SimIO.PrintText("\n");

		ptr += 16;
		offset += 16;

		// Adjust segment:offset if necessary
		if(offset > 0xFFFF) {
			offset = 0;
			segment += 0x1000;
		}
	}
}

// Dumps numBytes of memory as bytes, words, or dwords depending on size in hex, unsigned decimal, and signed decimal.
void CSimControl::DumpMemoryValues(void *memory, int segment, int offset, int numBytes, int size)
{
	int byteCount;
	unsigned int value;
	unsigned int address;

	address = PHYSICAL_ADDR(segment, offset);

	for(byteCount = 0; byteCount < numBytes; byteCount += size) {
		// Print segment:offset
		SimIO.PrintMessage("%04X:%04X  ", segment, offset);

		// Retrieve value
		value = GetMemoryValue(address, size);

		// Print value
		switch(size) {
		case 1:
			SimIO.PrintMessage("%02Xh  %3u  %+4d '", value, value, (int)(char)value);
			SimIO.DumpChar(value);
			SimIO.PrintText("'\n");
			break;
		case 2:
			SimIO.PrintMessage("%04Xh  %5u  %+6d\n", value, value, (int)(short)value);
			break;
		/*case 3:
			SimIO.PrintMessage("%06Xh  %8u  %+8d\n", value, value, value);
			break;*/
		case 4:
			SimIO.PrintMessage("%08Xh  %10u  %+11d\n", value, value, value);
			break;
		}

		address += size;
		offset += size;	

		// Adjust segment:offset if necessary
		if(offset > 0xFFFF) {
			offset %= 0x10000;
			segment += 0x1000;
		}
	}
}


// Disassembles and displays the instruction passed.
// Prints segment:offset, dump of instruction, and assembly string.
void CSimControl::PrintInstruction(int segment, int offset, unsigned char *instData, int instSize, char *instruction, char sep) 
{
	int bytesDumped;

	// Print offset
	SimIO.PrintMessage("  %04X:%04X %c ", segment, offset, sep);

	// Print instruction dump in hex (maximum of 8 bytes)
	for(bytesDumped = 0; bytesDumped < instSize && bytesDumped < 8; bytesDumped++) {
		SimIO.PrintMessage("%02X", instData[bytesDumped]);
	}

	// Print instruction
	SimIO.PrintMessage("%*s%c %s", (8-bytesDumped)*2, "", sep, instruction);

	// Check that entire instruction was dumped (since limited to MAX_DUMP bytes on main line)
	if(bytesDumped < instSize) {
		// Print rest of dump	
		while(bytesDumped < instSize) {
			if(bytesDumped % 8 == 0) SimIO.PrintText("\n          ...");
			SimIO.PrintMessage("%02X", instData[bytesDumped]);
			bytesDumped++;
		}
	}
	SimIO.PrintText("\n");
}


// Uses NASM disassembler code to disassembles "numInst" instructions starting at segment:offset.
void CSimControl::Disassemble(int segment, int offset, int numInst)
{
	unsigned char *instPtr;
    char outbuf[512];
    int instSize;
	int physical;

	physical = PHYSICAL_ADDR(segment, offset);

	instPtr = (unsigned char *)M.privatevp + physical;

	if(!AddressInRange(physical)) return;		// Verify proper bounds

	// This code assumes a suffiction buffer beyond available memory for the largest possibl instruction.
	while(numInst > 0) {
		instSize = disasm (instPtr, outbuf, 16, offset, 0, 0L);
		if(!AddressInRange(PHYSICAL_ADDR(segment, offset), instSize)) {
			SimIO.PrintText("Next instrcution extends beyond available memory.\n");
			break;
		}
		
		if(instSize < 1) {
			strcpy(outbuf, "** INVALID INSTRUCTION **");
			instSize = 1;
		}
		PrintInstruction(segment, offset, instPtr, instSize, outbuf, '|');
		instPtr += instSize;	// Adjust pointer to instruction
		offset += instSize;		// Keep track of offset for correct jump addresses
		numInst--;				// Keep track of number of instructions

		// Adjust segment:offset if necessary
		if(offset > 0xFFFF) {
			offset = 0;
			segment += 0x1000;
		}
	}
}


// Disassembles the next instruction (at CS:IP)
void CSimControl::DisassembleNext(void)
{
	unsigned char *instPtr;
    char instString[256];
    int instSize;
	int segment, offset, physical;

	segment = M.x86.R_CS;
	offset = M.x86.R_IP;

	physical = PHYSICAL_ADDR(segment, offset);

	if(!AddressInRange(physical)) {
		SimIO.PrintText("  ADDRESS OUT OF RANGE\n");
		return;		// Verify proper bounds
	}

	instPtr = (unsigned char *)M.privatevp + physical;

	instSize = disasm(instPtr, instString, 16, offset, 0, 0);
	PrintInstruction(segment, offset, instPtr, instSize, instString, ' ');
}


// Parses the command and calls appropriate routine for the command.
// Returns an op code for the command (CMD_INVALID if command is bad).
int CSimControl::ExecCommand(char *cmdString, SimCommand *cmd)
{
	int maxLen;
	int i;

	maxLen = strlen(cmdString);

	// Find start of command
	for(i = 0; i < maxLen && !IsVisibleChar(cmdString[i]); i++);
	cmdString = &cmdString[i];

	cmd->cmdString = cmdString;
	cmd->numParams = 0;
	
	// Null terminate parts of command
	for(i = 0; i < maxLen; i++) {
		if(!IsVisibleChar(cmdString[i])) {
			cmdString[i] = '\0';

			// Save location of parameter
			if(IsVisibleChar(cmdString[i+1])) {
				// If a quote delimited, take whole quotation as parameter
				if(cmdString[i+1] == '\"') {
					i += 2;
					cmd->param[cmd->numParams] = &cmdString[i];
					while(cmdString[i] != '\"' && i < maxLen) i++;	// Find next '"'

					if(i != maxLen) cmdString[i] = '\0';
				}
				else cmd->param[cmd->numParams] = &cmdString[i+1];
					
				cmd->numParams++;
			}
		}
	}

	ToLower(cmdString);		// Reduce command to lower case for case insensitivity

	// TemplaTe:   else if(!strcmp(cmdString, "")) cmd->op = CMD_;

	if(!strcmp(cmdString, "h"))				{ cmd->op = CMD_HELP;					CmdHelp(cmd); }
	else if(!strcmp(cmdString, "?"))		{ cmd->op = CMD_HELP;					CmdHelp(cmd); }
	else if(!strcmp(cmdString, "help"))		{ cmd->op = CMD_HELP;					CmdHelp(cmd); }

	else if(!strcmp(cmdString, "regs")
		|| !strcmp(cmdString, "reg")
		|| !strcmp(cmdString, "register")
		|| !strcmp(cmdString, "registers"))	{ cmd->op = CMD_EXPLAIN_REGISTERS;		CmdExplainRegisters(cmd); }

	
	// Not finished command (need to add some interrupts before most EXEs will run)
	else if(!strcmp(cmdString, "lexe")) LoadFileEXE(cmd->param[0], DEFAULT_EXE_LOAD_SEGMENT);
	//

	else if(!strcmp(cmdString, "l"))		{ cmd->op = CMD_LOAD;					CmdLoad(cmd); }
	else if(!strcmp(cmdString, "q")
		|| !strcmp(cmdString, "exit"))		{ cmd->op = CMD_QUIT;					CmdQuit(cmd); }
	else if(!strcmp(cmdString, "w"))		{ cmd->op = CMD_CLEAR;					CmdClear(cmd); }
	else if(!strcmp(cmdString, "e"))		{ cmd->op = CMD_EXECUTE;				CmdExecute(cmd); }
	else if(!strcmp(cmdString, "eto"))		{ cmd->op = CMD_EXECUTE_TO;				CmdExecuteTo(cmd); }
	else if(!strcmp(cmdString, "s"))		{ cmd->op = CMD_STEP;					CmdStep(cmd); }
	else if(!strcmp(cmdString, "o"))		{ cmd->op = CMD_STEP_OVER;				CmdStepOver(cmd); }
	else if(!strcmp(cmdString, "v"))		{ cmd->op = CMD_VERBOSE_TOGGLE;			CmdVerboseToggle(cmd); }
	
	else if(!strcmp(cmdString, "hist")
		|| !strcmp(cmdString, "history"))	{ cmd->op = CMD_DISPLAY_HISTORY;		CmdDisplayHistory(cmd); }
	
	else if(!strcmp(cmdString, "r")
		|| !strcmp(cmdString, "rh")
		|| !strcmp(cmdString, "rd")
		|| !strcmp(cmdString, "ru"))		{ cmd->op = CMD_DISPLAY_REGISTERS;		CmdDisplayRegisters(cmd); }

	else if(!strcmp(cmdString, "addr"))		{ cmd->op = CMD_GIVE_ADDRESS;			CmdGiveAddress(cmd); }

	else if(!strcmp(cmdString, "d"))		{ cmd->op = CMD_DISASSEMBLE;			CmdDisassemble(cmd); }
	else if(!strcmp(cmdString, "ds"))		{ cmd->op = CMD_DUMP_STACK;				CmdDumpStack(cmd); }
	else if(!strcmp(cmdString, "dump")
		|| !strcmp(cmdString, "dmp"))		{ cmd->op = CMD_DUMP_MEM;				CmdDumpMem(cmd); }
	else if(!strcmp(cmdString, "dumpb")
		|| !strcmp(cmdString, "dmpb"))		{ cmd->op = CMD_DUMP_MEM_BYTE;			CmdDumpMem(cmd); }
	else if(!strcmp(cmdString, "dumpw")
		|| !strcmp(cmdString, "dmpw"))		{ cmd->op = CMD_DUMP_MEM_WORD;			CmdDumpMem(cmd); }
	else if(!strcmp(cmdString, "dumpdw")
		|| !strcmp(cmdString, "dumpd")
		|| !strcmp(cmdString, "dmpd"))		{ cmd->op = CMD_DUMP_MEM_DWORD;			CmdDumpMem(cmd); }

	else if(!strcmp(cmdString, "pr"))		{ cmd->op = CMD_POKE_REGISTER;			CmdPokeReg(cmd); }
	else if(!strcmp(cmdString, "p")
		|| !strcmp(cmdString, "pb"))		{ cmd->op = CMD_POKE_MEM_BYTE;			CmdPokeMem(cmd); }
	else if(!strcmp(cmdString, "pw"))		{ cmd->op = CMD_POKE_MEM_WORD;			CmdPokeMem(cmd); }
	else if(!strcmp(cmdString, "pd")
		|| !strcmp(cmdString, "pdw"))		{ cmd->op = CMD_POKE_MEM_DWORD;			CmdPokeMem(cmd); }
	
	else if(!strcmp(cmdString, "t"))		{ cmd->op = CMD_SET_TICK;				CmdSetTick(cmd); }
	else if(!strcmp(cmdString, "a"))		{ cmd->op = CMD_ASSERT_INTERRUPT;		CmdAssertInterrupt(cmd); }
	
	else if(!strcmp(cmdString, "b")
		&& cmd->numParams == 0)				{ cmd->op = CMD_DISPLAY_BREAKPOINTS;	CmdDisplayBreakpoints(cmd); }
	else if(!strcmp(cmdString, "b"))		{ cmd->op = CMD_SET_BREAKPOINT;			CmdSetBreakpoint(cmd); }
	else if(!strcmp(cmdString, "c"))		{ cmd->op = CMD_CLEAR_BREAKPOINT;		CmdClearBreakpoint(cmd); }

	else if(!strcmp(cmdString, "clear"))	{ cmd->op = CMD_CLEAR_ALL;				CmdClearAll(cmd); }

	else if(!strcmp(cmdString, "f")
		&& cmd->numParams == 0)				{ cmd->op = CMD_DISPLAY_REG_MONITORS;	CmdDisplayRegMonitors(cmd); }
	else if(!strcmp(cmdString, "f"))		{ cmd->op = CMD_SET_REG_MONITOR;		CmdSetRegMonitor(cmd); }
	else if(!strcmp(cmdString, "fr"))		{ cmd->op = CMD_SET_REG_MONITOR;		CmdSetRegMonitor(cmd); }
	else if(!strcmp(cmdString, "fw"))		{ cmd->op = CMD_SET_REG_MONITOR;		CmdSetRegMonitor(cmd); }
	else if(!strcmp(cmdString, "fm"))		{ cmd->op = CMD_SET_REG_MONITOR;		CmdSetRegMonitor(cmd); }
	else if(!strcmp(cmdString, "fa"))		{ cmd->op = CMD_SET_REG_MONITOR;		CmdSetRegMonitor(cmd); }
	else if(!strcmp(cmdString, "fb"))		{ cmd->op = CMD_SET_REG_MONITOR;		CmdSetRegMonitor(cmd); }
	else if(!strcmp(cmdString, "g"))		{ cmd->op = CMD_CLEAR_REG_MONITOR;		CmdClearRegMonitor(cmd); }

	else if(!strcmp(cmdString, "n"))		{ cmd->op = CMD_CLEAR_MEM_MONITOR;		CmdClearMemMonitor(cmd); }
	else if(!strcmp(cmdString, "m")
		&& cmd->numParams == 0)				{ cmd->op = CMD_DISPLAY_MEM_MONITORS;	CmdDisplayMemMonitors(cmd); }
	else if(!strcmp(cmdString, "m"))		{ cmd->op = CMD_SET_MEM_MONITOR;		CmdSetMemMonitor(cmd); }
	else if(!strcmp(cmdString, "mr"))		{ cmd->op = CMD_SET_MEM_MONITOR;		CmdSetMemMonitor(cmd); }
	else if(!strcmp(cmdString, "mw"))		{ cmd->op = CMD_SET_MEM_MONITOR;		CmdSetMemMonitor(cmd); }
	else if(!strcmp(cmdString, "mm"))		{ cmd->op = CMD_SET_MEM_MONITOR;		CmdSetMemMonitor(cmd); }
	else if(!strcmp(cmdString, "ma"))		{ cmd->op = CMD_SET_MEM_MONITOR;		CmdSetMemMonitor(cmd); }
	else if(!strcmp(cmdString, "mb"))		{ cmd->op = CMD_SET_MEM_MONITOR;		CmdSetMemMonitor(cmd); }

	else if(!strcmp(cmdString, "echo"))		{ if(cmd->numParams >= 1) ProgramWindow.PrintText(cmd->param[0]); }
	else if(!strcmp(cmdString, "simptris"))	{ cmd->op = CMD_SIMPTRIS;				CmdInitSimptris(cmd); }
	else if(!strcmp(cmdString, "test")) {
		// Do any testing here
		//SimIO.PrintText("Don't ever type that again!\n");
		
		// printf("Flags are: %X\n", M.x86.R_FLG);

		/*
		// Try this on an actual spice machine to test PROGRAMCRT output speed
		int i;
		for(i = 0; i < 1000; i++) ProgramWindow.SendString("This is a really cool string. ", 30);
		*/
		
		/*
		SimIO.PrintText("Simtris test. Press a key single step through test (e to exit)...\n");
		if(!simptris) CmdInitSimptris(NULL);
		Simptris.init_simptris();
		char c;
		int count = 0;
		int i;
		while(true) {
			while((c = ProgramWindow.GetProgramInput()) == KEY_NONE);
			if(c == 'e') break;
			for(i = 0; i < 50; i++) Simptris.handle_simptris();
			if(Simptris.GameOver()) break;
			SimIO.PrintMessage("Step %d\n", count++);
		}
		if( Simptris.GameOver()) SimIO.PrintText("Game over!\n");
		*/		
	}
	else {
		SimIO.PrintMessage("Invalid command: \"%s\"\n", cmd->cmdString);
		cmd->op = CMD_INVALID;
	}

	return cmd->op;
}



//////////////////////////////////////////////////////////////////////
// COMMAND HANDLERS //////////////////////////////////////////////////


void CSimControl::CmdInvalid(SimCommand *cmd)	{ SimIO.PrintMessage("Invalid command: \"%s\"\n", cmd->cmdString); }


// Handle help command (h,l)
void CSimControl::CmdHelp(SimCommand *cmd)
{
	SimIO.PrintText(HelpMessage);
}


// Handle register help command (reg)
void CSimControl::CmdExplainRegisters(SimCommand *cmd)
{
	SimIO.PrintText(RegisterDescription);
}

// Handle load command (l)
void CSimControl::CmdLoad(SimCommand *cmd)
{
	// Syntax: CMD_LOAD [FILE] [ADDR] [ADDR]
	// CMD_LOAD							(Load the file that was last loaded using the same arguments as before)
	// CMD_LOAD [FILE]					(Load FILE using default parameters)
	// CMD_LOAD [FILE] [ADDR]			(Load FILE at ADDR, using all other default parameters)
	// CMD_LOAD [FILE] [ADDR] [ADDR]	(Load FILE at ADDR, initlialze all segment registers to SEGMENT(ADDR), and IP to OFFSET(ADDR))
	// CMD_LOAD [ADDR]					(Load the previously loaded file at ADDR)
	// CMD_LOAD [ADDR] [ADDR]			(Load the previously loaded file at ADDR, initlialze all segment registers to SEGMENT(ADDR), and IP to OFFSET(ADDR))
	static char lastFileName[COMMAND_BUFFER_SIZE] = "";
	static Address address;
	static Address CS_IP;
	Address temp_address;
	Address temp_CS_IP;
	char *fileName;

	fileName = lastFileName;
	temp_CS_IP.offset = CODE_OFFSET;
	temp_CS_IP.segment = CODE_SEGMENT;
	temp_CS_IP.physical = PHYSICAL_ADDR(CODE_SEGMENT, CODE_OFFSET);

	temp_address = address;

	// Check if command indicated filename
	if(cmd->numParams > 0 && !IsAddress(cmd->param[0])) {
		fileName = cmd->param[0];
		temp_address.segment = DEFAULT_LOAD_SEGMENT;
		temp_address.offset = DEFAULT_LOAD_OFFSET;
		temp_address.physical = PHYSICAL_ADDR(temp_address.segment, temp_address.offset);
		
		if(cmd->numParams >= 2) {
			if(!IsAddress(cmd->param[1])) {
				SimIO.PrintMessage("Invalid load address: \"%s\"\nFile not loaded.", cmd->param[1]);
				return;
			}
			InterpretAddress(cmd->param[1], &temp_address);
		}
		// Check to see if segment and offset of start given
		if(cmd->numParams >= 3) {
			if(!IsAddress(cmd->param[2])) {
				SimIO.PrintMessage("Invalid start address: \"%s\"\nFile not loaded.", cmd->param[2]);
				return;
			}
			InterpretAddress(cmd->param[2], &temp_CS_IP);
		}

	}
	// If not, verify that a file was loaded previously
	else if(lastFileName[0] == '\0') {
		if(cmd->numParams >=2) SimIO.PrintText("Filename required as first parameter.\n");
		else SimIO.PrintText("Filename required.\n");
		return;
	}
	// New address, but same filename
	else if(cmd->numParams >= 1) {
		if(!IsAddress(cmd->param[0])) {
			SimIO.PrintMessage("Invalid load address: \"%s\"\nFile not loaded.", cmd->param[0]);
			return;
		}
		InterpretAddress(cmd->param[0], &temp_address);

		// Check to see if segment and offset of start given
		if(cmd->numParams >= 2) {
			if(!IsAddress(cmd->param[1])) {
				SimIO.PrintMessage("Invalid start address: \"%s\"\nFile not loaded.", cmd->param[1]);
				return;
			}
			InterpretAddress(cmd->param[1], &temp_CS_IP);
		}
	}
	
	// Load file
	if(LoadFile(fileName, &temp_address) == FILE_LOADED) {
		// Setup up processor to run program
		memset(&M.x86, 0, sizeof(M.x86));	// Clear registers
		M.x86.R_DS = M.x86.R_ES = M.x86.R_SS = M.x86.R_CS = temp_CS_IP.segment;
		M.x86.R_IP = temp_CS_IP.offset;
		M.x86.R_SP = STACK_OFFSET;		// Set to default
		SET_FLAG(F_IF);

		ResetProcessor();

		if(simptris) {
			Simptris.window->ClearSimpWindow();
			Simptris.EndGame();
		}

		strcpy(lastFileName, fileName);
	
		SimIO.PrintMessage("File \"%s\" loaded at address %04X:%04X (CS:IP -> %04X:%04X).\n", fileName, temp_address.segment, temp_address.offset, temp_CS_IP.segment, temp_CS_IP.offset);	

		CS_IP = temp_CS_IP;
		address = temp_address;
	}
	else SimIO.PrintMessage("Could not load file: %s\n", fileName); 
}



// Handle quit command (q)
void CSimControl::CmdQuit(SimCommand *cmd)	{ /*exit(0);*/ }


// Handle window clear/wipe command (w)
void CSimControl::CmdClear(SimCommand *cmd)	{ ProgramWindow.ClearWindow(); }


// Handle Execute command (e)
void CSimControl::CmdExecute(SimCommand *cmd)
{
	// Syntax: CMD_EXECUTE [numInstructions]
	// CMD_EXECUTE						(Execute until program terminates)
	// CMD_EXECUTE numInstructions		(Execute numInstructions instructions)
	if(cmd->numParams == 0) {
		Execute(0);	// Execute until end of program
	}
	else {
		int numInstructions;
		numInstructions = atoi(cmd->param[0]);
		if(IsNumber(cmd->param[0]) && numInstructions > 0) {
			Execute(numInstructions);
		}
		else SimIO.PrintMessage("Invalid number of instructions: \"%s\"\n", cmd->param[0]);
	}
}



// Handle Execute To command (eto)
void CSimControl::CmdExecuteTo(SimCommand *cmd)
{
	// Syntax: CMD_EXECUTE address
	
	Address address;
	int breakpointID;

	// Check for argument
	if(cmd->numParams != 1) {
		SimIO.PrintText("Invalid number of arguments.\n");
		return;
	}

	// Get address target
	if(!GetAddress(cmd->param[0], &address)) {
		SimIO.PrintMessage("Invalid address or symbol: \"%s\"\n", cmd->param[0]);
		return;
	}

	// Check address validity
	if(!AddressInRange(address.physical)) {		// Verify proper bounds
		SimIO.PrintText("Target address is out of bounds.\n");
		return;
	}

	// Set breakpoint for the target address
	breakpointID = SetBreakpoint(&address, BREAK_STEP_OVER);

	if(breakpointID != -1) {
		// Execute then remove the temprorary breakpoint
		Execute(0);
		ClearBreakpoint(breakpointID);
	}
	else SimIO.PrintText("ERROR: Could not add temporary breakpoint.\n");
}



// Handle step command (s)
void CSimControl::CmdStep(SimCommand *cmd)
{
	bool lastVerbose;
	lastVerbose = verboseMode;

	verboseMode = true;	// Temporarily put into verbose mode.
	Execute(1);
	verboseMode = lastVerbose;
}


// Handle step over command (o)
void CSimControl::CmdStepOver(SimCommand *cmd)
{
	unsigned char *instPtr;
    char instString[256];
    int instSize;
	int breakpointID;
	Address address;

	address.segment = M.x86.R_CS;
	address.offset = M.x86.R_IP;
	address.physical = PHYSICAL_ADDR(address.segment, address.offset);

	instPtr = (unsigned char *)M.privatevp + address.physical;

	if(IsCall(instPtr)) {
		// Display instruction
		SimIO.PrintMessage("  %d>", instructionCount+1);
		DisassembleNext();	// Show instruction

		// Get size of instruction
		instSize = disasm(instPtr, instString, 16, address.offset, 0, 0);
		
		// Set breakpoint for return instruction address
		address.physical += instSize;
		address.segment = SEGMENT(address.physical);
		address.offset = OFFSET(address.physical);	
		if(!AddressInRange(address.physical)) {		// Verify proper bounds
			SimIO.PrintText("WARNING: Return address is out of bounds. Use step command.\n");
			return;
		}
		breakpointID = SetBreakpoint(&address, BREAK_STEP_OVER);
		
		if(breakpointID != -1) {
			// Execute then remove the temprorary breakpoint
			Execute(0);
			ClearBreakpoint(breakpointID);
		}
		else SimIO.PrintText("ERROR: Could not add temporary breakpoint. Use step command.\n");
	}
	else CmdStep(cmd);
}

// Handle verbose toggle command (v)
void CSimControl::CmdVerboseToggle(SimCommand *cmd)
{
	if(verboseMode) {
		verboseMode = false;
		SimIO.PrintText("Verbose mode now OFF\n");
	}
	else {
		verboseMode = true;
		SimIO.PrintText("Verbose mode now ON\n");
	}
}

// Handle history command (hist,history)
void CSimControl::CmdDisplayHistory(SimCommand *cmd)
{
	// Syntax: hist [numEntries]
	int iInst, iInt, i;
	int numEntries;
	const char *symbolName;

	// Get number of entries to display
	if(cmd->numParams >= 1) {
		if(IsNumber(cmd->param[0])) numEntries = InterpretNumber(cmd->param[0]);
		else {
			SimIO.PrintMessage("Invalid number: \"%s\"\n", cmd->param[0]);
			return;
		}
	}
	else numEntries = DEFAULT_HISTORY_SIZE;

	if(numEntries > HISTORY_LENGTH) numEntries = HISTORY_LENGTH;

	iInst = (instHistoryOffset  + (HISTORY_LENGTH - numEntries)) % HISTORY_LENGTH;
	iInt = (intHistoryOffset + (HISTORY_LENGTH - numEntries)) % HISTORY_LENGTH;

	SimIO.PrintMessage("  CS:IP for previous %d instructions executed and previous %d interrupted.\n  Most recent entries are at the bottom:\n", numEntries, numEntries);
	SimIO.PrintText("  Executed                   |  Interrupted\n");
	SimIO.PrintText("  --------------------------------------------------\n");

	for(i = 0; i < numEntries; i++) {
		if(instHistory[iInst].segment == 0xFFFF && instHistory[iInst].offset == 0xFFFF) SimIO.PrintText("      -                     ");
		else {
			if(!symbolTable.LookupAddress(PHYSICAL_ADDR(instHistory[iInst].segment, instHistory[iInst].offset), &symbolName)) symbolName = (const char *)"\0";
			SimIO.PrintMessage("  %04X:%04X %-16s", instHistory[iInst].segment, instHistory[iInst].offset, symbolName);
		}

		SimIO.PrintText(" |  ");
		
		if(intHistory[iInt].segment == 0xFFFF && intHistory[iInt].offset == 0xFFFF) SimIO.PrintText("    -    \n");
		else {
			if(!symbolTable.LookupAddress(PHYSICAL_ADDR(intHistory[iInt].segment, intHistory[iInt].offset), &symbolName)) symbolName = (const char *)"\0";
			SimIO.PrintMessage("%04X:%04X (IRQ %Xh) %s\n", intHistory[iInt].segment, intHistory[iInt].offset, intHistory[iInt].vector, symbolName);
		}

		iInst++;
		iInt++;
		if(iInst == HISTORY_LENGTH) iInst = 0;
		if(iInt == HISTORY_LENGTH) iInt = 0;
	}
	if(numEntries == HISTORY_LENGTH) SimIO.PrintMessage("  The max of %d entries displayed\n", HISTORY_LENGTH);
}

// Handle register display command (r,rh,rd,ru)
void CSimControl::CmdDisplayRegisters(SimCommand *cmd)
{
	// Synatx: r[d,u]
	// 1) r		(hex)
	// 2) rh	(hex)
	// 3) rd	(signed decimal)
	// 4) ru	(unsigned decimal)
	// 5) r h	(hex)
	// 6) r d	(signed decimal)
	// 7) r u	(unsigned decimal)
	int format;

	if(cmd->numParams == 0) {
		if(cmd->cmdString[1] == '\0' || cmd->cmdString[1] == 'h') format = HEX;
		else if(cmd->cmdString[1] == 'd') format = DECIMAL;
		else if(cmd->cmdString[1] == 'u') format = UNSIGNED;
		else {
			SimIO.PrintMessage("Invalid variation of display registers command: \"%s\"\n", cmd->cmdString);
			return;
		}
	}
	else {
		if(cmd->param[0][0] == 'd') format = DECIMAL;
		else if(cmd->param[0][0] == 'u') format = UNSIGNED;
		else {
			SimIO.PrintMessage("Invalid paramater: \"%s\"\n", cmd->param[0]);
			return;
		}
	}

	DisplayRegisters(format);
	SimIO.PrintText("  Next instr ->");
	DisassembleNext();
}



// Handle the give address command (addr)
void CSimControl::CmdGiveAddress(SimCommand *cmd)
{
	// Syntax: CMD_GIVE_ADDRESS SYMBOL
	Address address;

	// Verify a simbol name was given
	if(cmd->numParams != 1) {
		SimIO.PrintText("You must specify a symbol name.\n");
		return;
	}

	// Symbol table loaded?
	if(!symbolTable.TableLoaded()) {
		SimIO.PrintText("Symbol table not loaded.\n");
		return;
	}

	// Lookup symbol and get address
	if(symbolTable.LookupSymbol(cmd->param[0], (unsigned *)&(address.physical))) {
		address.offset = OFFSET(address.physical);
		address.segment = SEGMENT(address.physical);
		SimIO.PrintMessage("%04X:%04X (%05Xh)\n", address.segment, address.offset, address.physical);
	}
	else SimIO.PrintMessage("Symbol not found: \"%s\"\n", cmd->param[0]);
}



// Handle dissasemble command (d)
void CSimControl::CmdDisassemble(SimCommand *cmd)
{
	// Syntax: CMD_DISASSEMBLE [address] [numInstr]
	//	1) CMD_DISASSEMBLE					(assume numInstr = DEFAULT_DISASSEMBLE_SIZE, address = CS:IP)
	//	2) CMD_DISASSEMBLE address			(assume numInstr = DEFAULT_DISASSEMBLE_SIZE)
	//  3) CMD_DISASSEMBLE address numInstr
	Address address;
	int numInstr;

	// Get address
	if(cmd->numParams >= 1) {
		if(!GetAddress(cmd->param[0], &address)) {
			SimIO.PrintMessage("Invalid address or symbol: \"%s\"\n", cmd->param[0]);
			return;
		}
	}
	else {
		address.segment = M.x86.R_CS;
		address.offset = M.x86.R_IP;
		address.physical = PHYSICAL_ADDR(address.segment, address.offset);
	}

	// Get numButes
	if(cmd->numParams >= 2) {
		if(IsNumber(cmd->param[1])) numInstr = InterpretNumber(cmd->param[1]);
		else {
			SimIO.PrintMessage("Invalid number of instructions: \"%s\"\n", cmd->param[1]);
			return;
		}
	}
	else numInstr = DEFAULT_DISASSEMBE_SIZE;

	// Check address
	if(!AddressInRange(address.physical)) {
		SimIO.PrintText("Invalid address: Extends beyond available memory range.\n");	
		return;
	}

	SimIO.PrintText("   Seg:Off  | Hex Dump        | Assembly\n  ----------|-----------------|---------------------------\n");
	Disassemble(address.segment, address.offset, numInstr);
}



void CSimControl::CmdDumpStack(SimCommand *cmd)
{
	// Syntax: CMD_DUMP_STACK [numWords]
	//	1) CMD_DUMP_MEM (DEFAULT_STACK_DUMP_SIZE bytes)
	//	2) CMD_DUMP_MEM numBytes

	Address addr;
	int numWords, numBytes;
	int count;
	unsigned value;

	// Interpret given numWords string
	if(cmd->numParams >= 1) {
		if(IsAddress(cmd->param[0])) numWords = InterpretNumber(cmd->param[0]);
		else {
			SimIO.PrintMessage("Invalid number: \"%s\"\n", cmd->param[0]);
			return;
		}

		if(numWords <= 0) {
			SimIO.PrintMessage("Invalid number of bytes: \"%s\"\n", cmd->param[0]);
			return;
		}
	}
	else numWords = DEFAULT_STACK_DUMP_SIZE;

	numBytes = 2*numWords;

	// Resize numBytes if would cause segment wrap-around
	numBytes = MIN(0xFFFF - M.x86.R_SP + 1, numBytes);

	// Initialize address components
	addr.segment = M.x86.R_SS;
	addr.offset = M.x86.R_SP + numBytes - 2;
	addr.physical = PHYSICAL_ADDR(addr.segment, addr.offset);

	SimIO.PrintText("   Seg:Off  | Value\n  ----------|-------");

	for(count = 0; count < numBytes; count+= 2) {
		value = GetMemoryValue(addr.physical, 2);

		SimIO.PrintMessage("\n  %04X:%04X | %04Xh <-[BP%+d]", addr.segment, addr.offset, value, addr.offset-M.x86.R_BP);

		addr.offset -= 2;
		addr.physical -= 2;
	}

	SimIO.PrintText(" <-[SS:SP]\n");
}


// Handle memory dump command (dump)
void CSimControl::CmdDumpMem(SimCommand *cmd)
{
	// Syntax: CMD_DUMP_MEM address [numItems]
	//	1) CMD_DUMP_MEM address				(Assume 1 byte, word, or dwords)
	//	2) CMD_DUMP_MEM address numBytes
	Address address;
	int numItems, numBytes;
	
	// Interpret given address string
	if(cmd->numParams >= 1) {
		if(!GetAddress(cmd->param[0], &address)) {
			SimIO.PrintMessage("Invalid address or symbol: \"%s\"\n", cmd->param[0]);
			return;
		}
	}
	
	// Two general 	cases: number of bytes specified, number not specified.
	if(cmd->numParams == 1) numItems = DEFAULT_MEM_DUMP_SIZE;
	else if(cmd->numParams == 2) {
		bool valid;
		valid = IsNumber(cmd->param[1]);
		if(valid) numItems = InterpretNumber(cmd->param[1]);
		if(!valid || numItems <= 0) {
			SimIO.PrintMessage("Invalid number of bytes: \"%s\"\n", cmd->param[1]);
			return;
		}
	}
	else {
		SimIO.PrintText("Invalid number of paramaters.\n");
		return;
	}

	// Convert numItems from words or dwords to bytes.
	if(cmd->op == CMD_DUMP_MEM_WORD) numBytes = numItems * 2;
	else if(cmd->op == CMD_DUMP_MEM_DWORD) numBytes = numItems * 4;
	else numBytes = numItems;
	
	// Dump memory
	if(!AddressInRange(address.physical, numBytes)) SimIO.PrintText("Invalid address of number of bytes: Extends beyond available memory range.\n");	
	else {
		switch(cmd->op) {
		case CMD_DUMP_MEM:
			DumpMemory(M.privatevp, address.segment, address.offset, numBytes);
			break;
		case CMD_DUMP_MEM_BYTE:
			DumpMemoryValues(M.privatevp, address.segment, address.offset, numBytes, 1);
			break;
		case CMD_DUMP_MEM_WORD:
			DumpMemoryValues(M.privatevp, address.segment, address.offset, numBytes, 2);
			break;
		case CMD_DUMP_MEM_DWORD:
			DumpMemoryValues(M.privatevp, address.segment, address.offset, numBytes, 4);
			break;
		}
	}
}



// Handle register poke command (pr)
void CSimControl::CmdPokeReg(SimCommand *cmd)
{
	// Syntax: CMD_POKE_REG regName value
	int	registerNum;
	unsigned value;
	
	// Check for correct number of parameters
	if(cmd->numParams != 2) {
		SimIO.PrintText("Invalid number of parameters.\n");
		return;
	}
	
	// Get register
	registerNum = GetRegister(cmd->param[0]);
	if(registerNum == BAD_REG) {
		SimIO.PrintMessage("Invalid register: \"%s\"\n", cmd->param[0]);
		return;
	}
	else if(registerNum == ISR_REG || registerNum == IRR_REG) {
		SimIO.PrintMessage("ERROR: Modification of the ISR and IRR registers is not permitted.\n", registerName[registerNum]);
	}

	// Get value
	if(!IsNumber(cmd->param[1])) {
		SimIO.PrintMessage("Invalid value: \"%s\"\n", cmd->param[1]);
		return;
	}
	value = InterpretNumber(cmd->param[1]);

	value = SetRegister(registerNum, value);

	SimIO.PrintMessage("Register %s set to %Xh.\n", registerName[registerNum], value);
}

// Handle memory poke command (pm)
void CSimControl::CmdPokeMem(SimCommand *cmd)
{
	// Syntax: CMD_POKE_MEM_<SIZE> address value
	Address addr;
	unsigned int val;
	if(cmd->numParams == 2) {
		if(GetAddress(cmd->param[0], &addr)) {
			if(IsNumber(cmd->param[1])) {
				
				val = (unsigned int)InterpretNumber(cmd->param[1]);
				
				// Separate 	cases, store as little-endian
				if(cmd->op == CMD_POKE_MEM_BYTE && AddressInRange(addr.physical)) {
					SetMemory(addr.physical, val, 1);
					SimIO.PrintMessage("Byte at address %04X:%04X set to %02Xh (%dd).\n", addr.segment, addr.offset, val, val);
				}
				else if(cmd->op == CMD_POKE_MEM_WORD && AddressInRange(addr.physical, 2)) {
					SetMemory(addr.physical, val, 2);
					SimIO.PrintMessage("16-bit word at address %04X:%04X set to %02Xh (%dd).\n", addr.segment, addr.offset, val, val);
				}
				else if(cmd->op == CMD_POKE_MEM_DWORD && AddressInRange(addr.physical, 4)) {
					SetMemory(addr.physical, val, 4);
					SimIO.PrintMessage("32-bit dword at adress %04X:%04X set to %02Xh (%dd).\n", addr.segment, addr.offset, val, val);
				}
				else SimIO.PrintText("Invalid address or size: Extends beyond available memory range.\n");
			}
			else SimIO.PrintMessage("Invalid parameter: \"%s\"\n", cmd->param[1]);
		}
		else SimIO.PrintMessage("Invalid address: \"%s\"\n", cmd->param[0]);
	}
	else SimIO.PrintText("Invalid number of parameters.\n");
}


// Handle tick set command (t)
void CSimControl::CmdSetTick(SimCommand *cmd)
{
	// Syntax: CMD_SET_TICK tickInterval
	// CMD_SET_TICK		(Display current interval)
	// CMD_SET_TICK 0	(Disable ticks)
	// CMD_SET_TICK num (Set tick interval to num)
	if(cmd->numParams == 0) {
		if(tickInterval == 0) SimIO.PrintText("Tick disabled\n");
		else SimIO.PrintMessage("Tick interval: %d\n", tickInterval);
	}
	else if(cmd->numParams == 1) {
		if(IsNumber(cmd->param[0])) {
			tickInterval = InterpretNumber(cmd->param[0]);
			if(tickInterval == 0) SimIO.PrintText("Tick disabled.\n");
			else SimIO.PrintMessage("Tick inverval set to %d.\n", tickInterval);
		}
		else SimIO.PrintMessage("Invalid number: \"%s\"\n", cmd->param[0]);
	}
	else SimIO.PrintText("Invalid number of parameters.\n");
}

// Handle assert interrupt command (a)
void CSimControl::CmdAssertInterrupt(SimCommand *cmd)
{
	// Syntax: CMD_ASSERT_INTERRUPT vectorNumber
	int IRQ;

	if(cmd->numParams == 1) {
		if(IsNumber(cmd->param[0])) {

			IRQ = InterpretNumber(cmd->param[0]);
			
			if(IRQ >= 0 && IRQ <= NUM_INTERRUPT_PINS) {
				FlagInterrupt(IRQ);
				SimIO.PrintMessage("IRQ %d signalled.\n", IRQ);
			}
			else SimIO.PrintMessage("Invalid IRQ number: \"%s\". Must be from 0 to %d.\n", cmd->param[0], NUM_INTERRUPT_PINS);
		} 
		else SimIO.PrintMessage("Invalid number: \"%s\"\n", cmd->param[0]);
	}
	else SimIO.PrintText("Invalid number of parameters.\n");
}

// Handle display breakpoint command (b)
void CSimControl::CmdDisplayBreakpoints(SimCommand *cmd)
{
	if(cmd->numParams != 0) SimIO.PrintText("Warning: Paramaters ignored.\n");
	DisplayBreakpoints();
}

// Handle clear breakpoint command (c)
void CSimControl::CmdClearBreakpoint(SimCommand *cmd)
{
	int num;
	if(cmd->numParams == 0) {
		SimIO.PrintText("Missing parameter: Enter a breakpoint ID to clear, or 'all' to clear all.\n");
	}
	else if(cmd->numParams == 1){
		if(IsNumber(cmd->param[0])) {

			num = InterpretNumber(cmd->param[0]);
			
			if(ClearBreakpoint(num) == num) SimIO.PrintMessage("Breakpoint ID %d removed.\n", num);
			else SimIO.PrintMessage("ID \"%d\" is not a valid breakpoint ID.\n", num);
		}
		else if(!strcmp(cmd->param[0], "all")) {
			ClearBreakpoints();
			SimIO.PrintText("Breakpoints cleared.\n");
		}
		else SimIO.PrintMessage("Invalid number: \"%s\"\n", cmd->param[0]);
	}
	else SimIO.PrintText("Invalid number of parameters.\n");

}



// Handle set breakpoint command (b)
void CSimControl::CmdSetBreakpoint(SimCommand *cmd)
{
	// Syntax: CMD_SET_BREAKPOINT ADDR

	Address	address;
	int ID;
	
	// Check for correct number of parameters
	if(cmd->numParams != 1) {
		SimIO.PrintText("Invalid number of parameters.\n");
		return;
	}
 
	// Get address parameter
	if(!GetAddress(cmd->param[0], &address)) {
		SimIO.PrintMessage("Invalid address or symbol: \"%s\"\n", cmd->param[0]);
		return;
	}

	// Execute command //

	ID = SetBreakpoint(&address, BREAK_USER);
	if(ID == -1) SimIO.PrintText("ERROR: Could not add breakpoint.\n");
	else {
		SimIO.PrintMessage("Breakpoint ID %d added.\nAddress: %04X:%04X (%Xh)", ID, address.segment, address.offset, address.physical);
		if(!IsAddress(cmd->param[0])) SimIO.PrintMessage(" \"%s\"\n", cmd->param[0]);
		else SimIO.PrintText("\n");
	}
}


// Handle display register monitor commands (f)
void CSimControl::CmdDisplayRegMonitors(SimCommand *cmd)
{
	if(cmd->numParams != 0) SimIO.PrintText("Warning: Paramaters ignored.\n");
	DisplayRegMonitors();
}

// Handle clear register monitor command (g)
void CSimControl::CmdClearRegMonitor(SimCommand *cmd)
{
	int num;
	if(cmd->numParams == 0) {
		SimIO.PrintText("Missing parameters: Enter register monitor ID to clear, or 'all' to clear all.\n");
	}
	else if(cmd->numParams == 1){
		if(IsNumber(cmd->param[0])) {

			num = InterpretNumber(cmd->param[0]);
			
			if(ClearRegMonitor(num) == num) SimIO.PrintMessage("Register monitor ID %d removed.\n", num);
			else SimIO.PrintMessage("ID \"%d\" is not a valid register monitor ID.\n", num);
		}
		else if(!strcmp(cmd->param[0], "all")) {
			ClearRegMonitors();
			SimIO.PrintText("Register monitors cleared.\n");
		}
		else SimIO.PrintMessage("Invalid number: \"%s\"\n", cmd->param[0]);
	}
	else SimIO.PrintText("Invalid number of parameters.\n");
}

// Handle set register monitor command (f)
void CSimControl::CmdSetRegMonitor(SimCommand *cmd)
{
	// Syntax: CMD_SET_REG_MONITOR REG
	// 1) f				(display register monitors only)
	// *2) fr REG		(stop when read from)
	// *3) fw REG		(stop when written to)
	// 4) fm REG		(stop when modified [i.e. change to a DIFFERENT value])
	// *5) fa REG		(stop when accessed)
	// 6) fb REG VALUE 	(stop when it becomes 'VALUE')
	// * => The modes with a * are not permited
	int	registerNum, ID;
	unsigned value;
	
	// Check for correct number of parameters
	if(cmd->numParams < 1 || cmd->numParams > 2) {
		SimIO.PrintText("Invalid number of parameters.\n");
		return;
	}

	// Check if no specific mode was given
	if(cmd->cmdString[1] == '\0') cmd->cmdString[1] = 'm';	// Use modify mode by default

	// Check for valid REG parameter
	registerNum = GetRegister(cmd->param[0]);
	if(registerNum == BAD_REG) {
		SimIO.PrintMessage("Invalid register: \"%s\"\n", cmd->param[0]);
		return;
	}

	// Execute commands //

	// Check for form *2) mr ADDR [SIZE]		(stop when read from)
	if(cmd->cmdString[1] == 'r') {
		SimIO.PrintText("Invalid mode: 'read' mode is not permitted with registers.\n");
	}

	// Check for form *3) mw ADDR [SIZE]		(stop when written to)
	else if(cmd->cmdString[1] == 'w') {
		SimIO.PrintText("Invalid mode: 'write' mode is not permitted with registers.\n");
	}

	// Check for form 4) mm ADDR [SIZE]		(stop on modify [i.e. change to a DIFFERENT value])
	else if(cmd->cmdString[1] == 'm') {
		if((ID = SetRegMonitor(registerNum, ON_MODIFY, NULL)) == -1) SimIO.PrintText("ERROR: Could not add register monitor.\n");
		else SimIO.PrintMessage("Register monitor ID %d added.\nRegister: %s    Mode: Modify\n", ID, registerName[registerNum]);
	}

	// Check for form *5) ma ADDR [SIZE]		(stop when accessed)
	else if(cmd->cmdString[1] == 'a') {
		SimIO.PrintText("Invalid mode: 'access' mode is not permitted with registers.\n");
	}

	// Check for form 6) mb ADDR VALUE [SIZE] 	(stop when it becomes 'VALUE')
	else if(cmd->cmdString[1] == 'b') {
		// Make sure we got the  value paramater
		if(cmd->numParams < 2) {
			SimIO.PrintText("Missing parameter: 'become' mode requires a VALUE paramater.\n");
			return;
		}

		// Get second paramater (VALUE) if it exists (should already have VALUE, 1st paramater)
		if(!IsNumber(cmd->param[1])) {
			SimIO.PrintMessage("Invalid value: \"%s\"\n", cmd->param[1]);
			return;
		}
		value = InterpretNumber(cmd->param[1]);

		if((ID = SetRegMonitor(registerNum, ON_BECOME, value)) == -1) SimIO.PrintText("ERROR: Could not add register monitor.\n");
		else SimIO.PrintMessage("Register monitor ID %d added.\nRegister: %s    Mode: Become    Value: %d (%Xh)\n", ID, registerName[registerNum], value, value);
	}

	// No more possibilities (This actually should never occur)
	else SimIO.PrintMessage("Invalid variation of register monitor command: \"%s\"\n", cmd->cmdString);
}

// Handle display memory monitors command (m)
void CSimControl::CmdDisplayMemMonitors(SimCommand *cmd)
{
	if(cmd->numParams != 0) SimIO.PrintText("Warning: Paramaters ignored.\n");
	DisplayMemMonitors();
}

// Handle clear memory monitors command (n)
void CSimControl::CmdClearMemMonitor(SimCommand *cmd)
{
	int num;
	if(cmd->numParams == 0) {
		SimIO.PrintText("Missing parameter: Enter memory monitor ID to clear, or 'all' to clear all.\n");
	}
	else if(cmd->numParams == 1){
		if(IsNumber(cmd->param[0])) {

			num = InterpretNumber(cmd->param[0]);
			
			if(ClearMemMonitor(num) == num) SimIO.PrintMessage("Memory monitor ID %d removed.\n", num);
			else SimIO.PrintMessage("ID \"%d\" is not a valid memory monitor ID.\n", num);
		}
		else if(!strcmp(cmd->param[0], "all")) {
			ClearMemMonitors();
			SimIO.PrintText("Memory monitors cleared.\n");
		}
		else SimIO.PrintMessage("Invalid number: \"%s\"\n", cmd->param[0]);
	}
	else SimIO.PrintText("Invalid number of parameters.\n");
}

// Handle set memory monitor command (m)
void CSimControl::CmdSetMemMonitor(SimCommand *cmd)
{
	// Syntax: CMD_SET_MEM_MONITORX [ADDR] (VALUE) [SIZE]
	// Syntax variations: (All assume size of DEFAULT_MONITOR_SIZE bytes if size not given)
	// 1) m						(display memory monitors only)
	// 2) mr ADDR [SIZE]		(stop when read from)
	// 3) mw ADDR [SIZE]		(stop when written to)
	// 4) mm ADDR [SIZE]		(stop when modified [i.e. change to a DIFFERENT value])
	// 5) ma ADDR [SIZE]		(stop when accessed)
	// 6) mb ADDR VALUE [SIZE] 	(stop when it becomes 'VALUE')

	Address	address;
	int		p1, p2;
	int ID;
	
	// Check for correct number of parameters
	if(cmd->numParams < 1 || cmd->numParams > 3) {
		SimIO.PrintText("Invalid number of parameters.\n");
		return;
	}

	// Check if no specific mode was given
	if(cmd->cmdString[1] == '\0') cmd->cmdString[1] = 'm';	// Use modify mode by default

	// Get address parameter
	if(!GetAddress(cmd->param[0], &address)) {
		SimIO.PrintMessage("Invalid address or symbol: \"%s\"\n", cmd->param[0]);
		return;
	}

	// Get second parameter (if there is one)
	if(cmd->numParams >= 2) {
		if(!IsNumber(cmd->param[1])) {
			SimIO.PrintMessage("Invalid parameter: \"%s\"\n", cmd->param[1]);
			return;
		}
		p1 = InterpretNumber(cmd->param[1]);
	}
	else p1 = DEFAULT_MONITOR_SIZE;


	// Execute commands //

	// Check for form 2) mr ADDR [SIZE]		(stop when read from)
	if(cmd->cmdString[1] == 'r') {
		if((ID = SetMemMonitor(&address, p1, ON_READ, NULL)) == -1) SimIO.PrintText("ERROR: Could not add memory monitor.\n");
		else SimIO.PrintMessage("Memory monitor ID %d added.\nAddress: %04X:%04X (%Xh)   Mode: Read   Size: %d\n", ID, address.segment, address.offset, address.physical, p1);
	}

	// Check for form 3) mw ADDR [SIZE]		(stop when written to)
	else if(cmd->cmdString[1] == 'w') {
		if((ID = SetMemMonitor(&address, p1, ON_WRITE, NULL)) == -1) SimIO.PrintText("ERROR: Could not add memory monitor.\n");
		else SimIO.PrintMessage("Memory monitor ID %d added.\nAddress: %04X:%04X (%Xh)   Mode: Write   Size: %d\n", ID, address.segment, address.offset, address.physical, p1);
	}

	// Check for form 4) mm ADDR [SIZE]		(stop on modify [i.e. change to a DIFFERENT value])
	else if(cmd->cmdString[1] == 'm') {
		if((ID = SetMemMonitor(&address, p1, ON_MODIFY, NULL)) == -1) SimIO.PrintText("ERROR: Could not add memory monitor.\n");
		else SimIO.PrintMessage("Memory monitor ID %d added.\nAddress: %04X:%04X (%Xh)   Mode: Modify   Size: %d\n", ID, address.segment, address.offset, address.physical, p1);
	}

	// Check for form 5) ma ADDR [SIZE]		(stop when accessed)
	else if(cmd->cmdString[1] == 'a') {
		if((ID = SetMemMonitor(&address, p1, ON_ACCESS, NULL)) == -1) SimIO.PrintText("ERROR: Could not add memory monitor.\n");
		else SimIO.PrintMessage("Memory monitor ID %d added.\nAddress: %04X:%04X (%Xh)   Mode: Access   Size: %d\n", ID, address.segment, address.offset, address.physical, p1);
	}

	// Check for form 6) mb ADDR VALUE [SIZE] 	(stop when it becomes 'VALUE')
	else if(cmd->cmdString[1] == 'b') {
		// Make sure we got the  value paramater
		if(cmd->numParams < 2) {
			SimIO.PrintText("Missing parameter: 'become' mode requires a VALUE paramater.\n");
			return;
		}

		// Get third paramater (SIZE) if it exists (should already have VALUE, 1st paramater)
		if(cmd->numParams >= 3) {
			if(!IsNumber(cmd->param[2])) {
				SimIO.PrintMessage("Invalid parameter: \"%s\"\n", cmd->param[1]);
				return;
			}
			p2 = InterpretNumber(cmd->param[2]);
			
			// Check SIZE
			if(p2 < 1 || p2 > 4) {
				SimIO.PrintText("Invalid parameter: For 'become' mode, size must be from 1 to 4 bytes.\n");
				return;
			}
		}
		else p2 = DEFAULT_MONITOR_SIZE;

		// Note that p1 has a different role (the become value) in this case

		if((ID = SetMemMonitor(&address, p2, ON_BECOME, p1)) == -1) SimIO.PrintText("ERROR: Could not add memory monitor.\n");
		else SimIO.PrintMessage("Memory monitor ID %d added.\nAddress: %04X:%04X (%Xh)   Mode: Become   Value: %d (%Xh)    Size: %d\n", ID, address.segment, address.offset, address.physical, p1, p1, p2);
	}

	// No more possibilities (This actually should never occur)
	else SimIO.PrintMessage("Invalid variation of memory monitor command: \"%s\"\n", cmd->cmdString);
}



// Handle clear all command (clear)
void CSimControl::CmdClearAll(SimCommand *cmd)
{
	ClearMemMonitors();
	ClearRegMonitors();
	ClearBreakpoints();
	SimIO.PrintText("All breakpoints, memory monitors, and register monitors cleared.\n");
}



// Handle init simptris command (simptris)
void CSimControl::CmdInitSimptris(SimCommand *cmd)
{
	int status;

	if(simptris) {
		ProgramWindow.KillSimptris();
		simptris = false;
		
		SimIO.PrintText("Simptris mode disabled.\n");
	}
	else {
		status = ProgramWindow.InitSimptris();
		if(status != CRT_OK) {
			SimIO.PrintMessage("ERROR: Could not create Simptris window. (Error %d)\n", status);
			return;
		}
		
		Simptris.SetWindow(&ProgramWindow);
		simptris = true;
		
		SimIO.PrintText("Simptris mode enabled.\n");
	}
}



//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


// This determines if a return instruction passes control back to the OS
// by checking the current SS:SP.
// This function should be called when return instructions are executed.
void CSimControl::CheckReturn(void)
{
	if(PHYSICAL_ADDR(M.x86.R_SS, M.x86.R_SP) == stackStart) {
		ProgramTerminated(M.x86.R_AX);
	}
}


// Stops execution and displays program termination message.
void CSimControl::ProgramTerminated(unsigned short exitCode)
{
	stopFlag = true;

	//SimIO.PrintMessage("Program terminated with exit code %Xh.\n", (int)exitCode);
	SimIO.PrintMessage("PROGRAM FINISHED with exit code %Xh.\n", (int)exitCode);
}


// Called to halt execution when an exception/error occurs.
// Things that would normally crash a program are handled here.
void CSimControl::SystemHalted(int errorCode)
{
	systemHalted = true;
	stopFlag = true;
	
	switch(errorCode) {
	case EXCEPTION_BYTE_READ_OUT_OF_RANGE:
		SimIO.PrintText("SIMULATOR STOP: Read address for BYTE (8-bit) value is out of bounds.\n");
		break;
	case EXCEPTION_WORD_READ_OUT_OF_RANGE:
		SimIO.PrintText("SIMULATOR STOP: Read address for WORD (16-bit) value is out of bounds.\n");
		break;
	case EXCEPTION_DWORD_READ_OUT_OF_RANGE:
		SimIO.PrintText("SIMULATOR STOP: Read address for DWORD (32-bit) value is out of bounds.\n");
		break;
	case EXCEPTION_BYTE_WRITE_OUT_OF_RANGE:
		SimIO.PrintText("SIMULATOR STOP: Write address for BYTE (8-bit) value is out of bounds.\n");
		break;
	case EXCEPTION_WORD_WRITE_OUT_OF_RANGE:
		SimIO.PrintText("SIMULATOR STOP: Write address for WORD (16-bit) value is out of bounds.\n");
		break;
	case EXCEPTION_DWORD_WRITE_OUT_OF_RANGE:
		SimIO.PrintText("SIMULATOR STOP: Write address for DWORD (32-bit) value is out of bounds.\n");
		break;
	case EXCEPTION_DIVIDE_BY_ZERO:	// These two have the same value (same interrupt occurs)
	case EXCEPTION_DIVIDE_OVERFLOW:
		SimIO.PrintText("SIMULATOR STOP: Divide by zero or divide overflow error.\n");
		break;
	case EXCEPTION_BAD_OPCODE:
		SimIO.PrintText("SIMULATOR STOP: Illegal opcode.\n");
		break;
	case EXCEPTION_BAD_EXT_OPCODE:
		SimIO.PrintText("SIMULATOR STOP: Illegal extended opcode.\n");
		break;
	default:
		SimIO.PrintText("SIMULATOR STOP: Unknown exception.\n");
	}
}


// Does calculations to determine if writeSize bytes of writeValue written at writeAddress 
// will change any of the monitorSize bytes at monitorAddress.
// Used to detect when memory monitor conditions occur.
// Returns true if the monitored address will be changed by the write, otherwise false.
inline bool CSimControl::MemoryWillChange(int writeAddress, int writeSize, unsigned int writeValue, int monitorAddress, int monitorSize)
{
	int endAddress;

	endAddress = monitorAddress + monitorSize - 1;

	switch(writeSize)
	{
		case 1:
		if(
			((((unsigned char *)M.privatevp)[writeAddress] != (unsigned char)(writeValue)) && (writeAddress >= monitorAddress && writeAddress <= endAddress))
		) return true;
		return false;
		case 2:
		if(
			((((unsigned char *)M.privatevp)[writeAddress] != (unsigned char)(writeValue)) && (writeAddress >= monitorAddress && writeAddress <= endAddress)) ||
			((((unsigned char *)M.privatevp)[writeAddress+1] != (unsigned char)(writeValue >> 8 )) && (writeAddress+1 >= monitorAddress && writeAddress+1 <= endAddress))
		) return true;
		return false;
		case 3:
		if(
			((((unsigned char *)M.privatevp)[writeAddress] != (unsigned char)(writeValue)) && (writeAddress >= monitorAddress && writeAddress <= endAddress)) ||
			((((unsigned char *)M.privatevp)[writeAddress+1] != (unsigned char)(writeValue >> 8 )) && (writeAddress+1 >= monitorAddress && writeAddress+1 <= endAddress)) ||
			((((unsigned char *)M.privatevp)[writeAddress+2] != (unsigned char)(writeValue >> 16)) && (writeAddress+2 >= monitorAddress && writeAddress+2 <= endAddress))
		) return true;
		return false;
		case 4:
		if(
			((((unsigned char *)M.privatevp)[writeAddress] != (unsigned char)(writeValue)) && (writeAddress >= monitorAddress && writeAddress <= endAddress)) ||
			((((unsigned char *)M.privatevp)[writeAddress+1] != (unsigned char)(writeValue >> 8 )) && (writeAddress+1 >= monitorAddress && writeAddress+1 <= endAddress)) ||
			((((unsigned char *)M.privatevp)[writeAddress+2] != (unsigned char)(writeValue >> 16)) && (writeAddress+2 >= monitorAddress && writeAddress+2 <= endAddress)) ||
			((((unsigned char *)M.privatevp)[writeAddress+3] != (unsigned char)(writeValue >> 24)) && (writeAddress+3 >= monitorAddress && writeAddress+3 <= endAddress))
		) return true;
		return false;
	}
	return false;
}


// Determines if writing writeSize bytes of writeValue at writeAddress will cause 
// monitorSize bytes of monitorAddres to become monitorValue.
// Used to detect when memory monitor conditions occur.
// Returns true if the it will become the monitor become value, otherwise false.
bool CSimControl::MemoryWillBecome(int writeAddress, int writeSize, unsigned int writeValue, int monitorAddress, int monitorSize, unsigned int monitorValue)
{
	// Possibilities for memory overlap:
	// Memory:  0123  6789
	// Monitor:    3456
	// Buffer:  0123456789
	// Data:       3456
	static unsigned char buffer[10];		// Temp memory copy around monitorAddress (from monitorAddress-3 to monitorAddress+6)
	static unsigned char *data = buffer+3;	// Pointer in buffer to region corresponding to monitorAddress
	int i;

	i = writeAddress - monitorAddress;
	if(i > 3 || i < -3) {	// Condition should never be true, since the write region should overlap with the monitored region
		// For debuggin/testing
		SimIO.PrintMessage("ERROR: A MET CONDITION SHOULDN'T HAVE OCCURED.\nA 'become' mode memory monitor for physical address %Xh is not functioning and should be deleted.\n", monitorAddress);
		return false;	
	}
	
	switch(monitorSize) {
		case 1:	
		// Copy memory monitor region
		data[0] = ((unsigned char *)M.privatevp)[monitorAddress];
		
		// Make changes to buffer
		data[i]		= (unsigned char)(writeValue);
		data[i+1]	= (unsigned char)(writeValue >> 8);
		data[i+2]	= (unsigned char)(writeValue >> 16);
		data[i+3]	= (unsigned char)(writeValue >> 24);

		// Check if data matches monitorValue
		if(
			data[0] == (unsigned char)monitorValue
		)  return true;
		return false;
		
		case 2:
		// Copy memory monitor region
		data[0] = ((unsigned char *)M.privatevp)[monitorAddress];
		data[1] = ((unsigned char *)M.privatevp)[monitorAddress+1];
		
		// Make changes to buffer
		data[i]		= (unsigned char)(writeValue);
		data[i+1]	= (unsigned char)(writeValue >> 8);
		data[i+2]	= (unsigned char)(writeValue >> 16);
		data[i+3]	= (unsigned char)(writeValue >> 24);

		// Check if data matches monitorValue
		if(
			data[0] == (unsigned char)monitorValue &&
			data[1] == (unsigned char)(monitorValue >> 8)

		)  return true;
		return false;
		
		case 3:
		// Copy memory monitor region
		data[0] = ((unsigned char *)M.privatevp)[monitorAddress];
		data[1] = ((unsigned char *)M.privatevp)[monitorAddress+1];
		data[2] = ((unsigned char *)M.privatevp)[monitorAddress+2];
		
		// Make changes to buffer
		data[i]		= (unsigned char)(writeValue);
		data[i+1]	= (unsigned char)(writeValue >> 8);
		data[i+2]	= (unsigned char)(writeValue >> 16);
		data[i+3]	= (unsigned char)(writeValue >> 24);
		
		// Check if data matches monitorValue
		if(
			data[0] == (unsigned char)monitorValue &&
			data[1] == (unsigned char)(monitorValue >> 8) &&
			data[2] == (unsigned char)(monitorValue >> 16)
		)  return true;
		return false;

		case 4:
		// Copy memory monitor region
		data[0] = ((unsigned char *)M.privatevp)[monitorAddress];
		data[1] = ((unsigned char *)M.privatevp)[monitorAddress+1];
		data[2] = ((unsigned char *)M.privatevp)[monitorAddress+2];
		data[3] = ((unsigned char *)M.privatevp)[monitorAddress+3];
		
		// Make changes to buffer
		data[i]		= (unsigned char)(writeValue);
		data[i+1]	= (unsigned char)(writeValue >> 8);
		data[i+2]	= (unsigned char)(writeValue >> 16);
		data[i+3]	= (unsigned char)(writeValue >> 24);
		
		// Check if data matches monitorValue
		if(
			data[0] == (unsigned char)monitorValue &&
			data[1] == (unsigned char)(monitorValue >> 8) &&
			data[2] == (unsigned char)(monitorValue >> 16) &&
			data[3] == (unsigned char)(monitorValue >> 24)
		)  return true;
		return false;

	}
	return false;
}


// Returns the "size"-byte value stored in memory at "address".
// Converts from little endian format for big endian machines.
// A faster implemenation could be written to run on little endian machines.
unsigned int CSimControl::GetMemoryValue(int address, int size)
{
	unsigned int value;
	unsigned char *memory;

	memory = (unsigned char *)M.privatevp;
	value = 0;

	switch(size) {
		case 4: value |= (unsigned int)memory[address+3] << 24;
		case 3: value |= (unsigned int)memory[address+2] << 16;
		case 2: value |= (unsigned int)memory[address+1] << 8;
		case 1: value |= (unsigned int)memory[address];
		return value;
	default: 
		SimIO.PrintText("ERROR: Invalid value handed to CSimControl::GetValue(int, size)\n");
		return 1;
	}
	
	return value;
}


// Checks memory monitors to determine if write will cause a break. If so, it sets global flags to stop execution.
// This function must be called before changes to memory are made.
void CSimControl::AddressWrite(int writeAddress, int writeSize, unsigned int writeValue)
{
	MemoryMonitor *current;
	const char *symbolName;
	const char *d1, *d2;
	const char empty[] = "\0";
	const char left[] = " (";
	const char right[] = ")";

	// if(!memoryMonitorsActive) return;

	current = memoryMonitors;
	
	while(current != NULL) {
		// Check if this write is in bounds of memory monitor
		if((current->address.physical + current->size) > writeAddress && current->address.physical < (writeAddress + writeSize)) {
			switch(current->mode) {
			case ON_WRITE:
				if(!symbolTable.LookupAddress(writeAddress, &symbolName)) { symbolName = empty; d1 = d2 = empty; }
				else { d1 = left; d2 = right; }
				SimIO.PrintMessage(
					"< Memory monitor break > - Write to monitor ID %d%s%s%s.\n"
					"  Wrote %s %Xh to physical address %Xh. Was %s %Xh.\n", 
					current->ID, d1, symbolName, d2,
					sizeName[writeSize], writeValue, writeAddress, sizeName[writeSize], GetMemoryValue(writeAddress, writeSize)
					);
				stopFlag = true;
				break;
			case ON_MODIFY:
				if(MemoryWillChange(writeAddress, writeSize, writeValue, current->address.physical, current->size)) {
					if(!symbolTable.LookupAddress(writeAddress, &symbolName)) { symbolName = empty; d1 = d2 = empty; }
					else { d1 = left; d2 = right; }
					SimIO.PrintMessage(
						"< Memory monitor break > - Modification of monitor ID %d%s%s%s.\n"
						"  Wrote %s %Xh to physical address %Xh. Was %s %Xh.\n", 
						current->ID, d1, symbolName, d2,
						sizeName[writeSize], writeValue, writeAddress, sizeName[writeSize], GetMemoryValue(writeAddress, writeSize)
						);
					stopFlag = true;
				}
				break;
			case ON_ACCESS:
				if(!symbolTable.LookupAddress(writeAddress, &symbolName)) { symbolName = empty; d1 = d2 = empty; }
				else { d1 = left; d2 = right; }
				SimIO.PrintMessage(
					"< Memory monitor break > - Access to monitor ID %d%s%s%s.\n"
					"  Wrote %s %Xh to physical address %Xh. Was %s %Xh.\n", 
					current->ID, d1, symbolName, d2,
					sizeName[writeSize], writeValue, writeAddress, sizeName[writeSize], GetMemoryValue(writeAddress, writeSize)
					);
				stopFlag = true;
				break;
			case ON_BECOME:
				if(MemoryWillBecome(writeAddress, writeSize, writeValue, current->address.physical, current->size, current->value)) {
					if(!symbolTable.LookupAddress(writeAddress, &symbolName)) { symbolName = empty; d1 = d2 = empty; }
					else { d1 = left; d2 = right; }
					SimIO.PrintMessage(
						"< Memory monitor break > - Condition met for monitor ID %d%s%s%s.\n"
						"  Wrote %s %Xh to physical address %Xh. Was %s %Xh.\n"
						"  %s at %04X:%04X (%Xh) now equals monitor value of %Xh.\n", 
						current->ID, d1, symbolName, d2,
						sizeName[writeSize], writeValue, writeAddress, sizeName[writeSize], GetMemoryValue(writeAddress, writeSize), 
						sizeName[current->size], current->address.segment, current->address.offset, current->address.physical, current->value
						);
					stopFlag = true;
				}
				break;
				}
		}
		current = current->next;
	}
}


// Checks memory monitors to determine if read will cause a break. If so, it sets global flags to stop execution.
void CSimControl::AddressRead(int readAddress, int readSize)
{
	MemoryMonitor *current;
	const char *symbolName;
	const char *d1, *d2;
	const char empty[] = "\0";
	const char left[] = " (";
	const char right[] = ")";

	current = memoryMonitors;
	
	while(current != NULL) {
		// Check if this write is in bounds of memory monitor
		if((current->address.physical + current->size) > readAddress && current->address.physical < (readAddress + readSize)) {
			if(current->mode == ON_READ) {
				if(!symbolTable.LookupAddress(readAddress, &symbolName)) { symbolName = empty; d1 = d2 = empty; }
				else { d1 = left; d2 = right; }
				SimIO.PrintMessage(				
					"< Memory monitor break > - Read from memory monitor ID %d%s%s%s.\n"
					"  Read %s %Xh from physical address %Xh.\n",
					current->ID, d1, symbolName, d2,
					sizeName[readSize], GetMemoryValue(readAddress, readSize), readAddress
				);
				stopFlag = true;
			}
			else if(current->mode == ON_ACCESS) {
				if(!symbolTable.LookupAddress(readAddress, &symbolName)) { symbolName = empty; d1 = d2 = empty; }
				else { d1 = left; d2 = right; }
				SimIO.PrintMessage(				
					"< Memory monitor break > - Access to memory monitor ID %d%s%s%s.\n"
					"  Read %s %Xh from physical address %Xh.\n", 
					current->ID, d1, symbolName, d2,
					sizeName[readSize], GetMemoryValue(readAddress, readSize), readAddress
				);				
				stopFlag = true;
			}
		}
		current = current->next;
	}
}


// Checks breakpoints to see if current CS and IP are at a breakpoint. 
// If so, global flags are set to stop executions.
void CSimControl::CheckBreakpoints(void)
{
	Breakpoint *current;

	current = breakpoints;
	
	while(current != NULL) {
		if(PHYSICAL_ADDR((int)M.x86.R_CS, (int)M.x86.R_IP) == current->address.physical) { 
			stopFlag = true;
			if(current->type == BREAK_USER) {
				SimIO.PrintMessage(				
					"< Breakpoint > - Reached breakpoint ID %d at address %04X:%04X (%Xh).\n",
					current->ID, current->address.segment, current->address.offset, current->address.physical
				);
			}
			// If this is a type BREAK_STEP_OVER breakpoint then nothing needs to be done
		}
		current = current->next;
	}
}


// Check register monitors to see if monitored registers have changed since the last call
// to this function or to InitRegisterMonitorContents(). If so, global variables are set
// to stop execution.
void CSimControl::CheckRegisterMonitors(void)
{
	RegisterMonitor *current;
	unsigned newValue, lastValue;
	bool breakFlag = false;

	current = registerMonitors;
	
	while(current != NULL) {
		switch(current->registerNum) {

		// Begin 8086 registers:

		case AH_REG:
			if((current->mode == ON_MODIFY && current->lastValue != M.x86.R_AH) ||  
				(current->mode == ON_BECOME && current->value == M.x86.R_AH && current->value != current->lastValue)) {
				newValue = M.x86.R_AH;
				lastValue = current->lastValue;
				breakFlag = true;
			}
			current->lastValue = M.x86.R_AH; 
			break;
		case AL_REG: 
			if((current->mode == ON_MODIFY && current->lastValue != M.x86.R_AL) ||  
				(current->mode == ON_BECOME && current->value == M.x86.R_AL && current->value != current->lastValue)) {
				newValue = M.x86.R_AL;
				lastValue = current->lastValue;
				breakFlag = true;
			}
			current->lastValue = M.x86.R_AL; 
			break;
		case BH_REG: 
			if((current->mode == ON_MODIFY && current->lastValue != M.x86.R_BH) ||  
				(current->mode == ON_BECOME && current->value == M.x86.R_BH && current->value != current->lastValue)) {
				newValue = M.x86.R_BH;
				lastValue = current->lastValue;
				breakFlag = true;
			}
			current->lastValue = M.x86.R_BH; 
			break;
		case BL_REG: 
			if((current->mode == ON_MODIFY && current->lastValue != M.x86.R_BL) ||  
				(current->mode == ON_BECOME && current->value == M.x86.R_BL && current->value != current->lastValue)) {
				newValue = M.x86.R_BL;
				lastValue = current->lastValue;
				breakFlag = true;
			}
			current->lastValue = M.x86.R_BL; 
			break;
		case CH_REG: 
			if((current->mode == ON_MODIFY && current->lastValue != M.x86.R_CH) ||  
				(current->mode == ON_BECOME && current->value == M.x86.R_CH && current->value != current->lastValue)) {
				newValue = M.x86.R_CH;
				lastValue = current->lastValue;
				breakFlag = true;
			}
			current->lastValue = M.x86.R_CH; 
			break;
		case CL_REG: 
			if((current->mode == ON_MODIFY && current->lastValue != M.x86.R_CL) ||  
				(current->mode == ON_BECOME && current->value == M.x86.R_CL)) {
				newValue = M.x86.R_CL;
				lastValue = current->lastValue;
				breakFlag = true;
			}
			current->lastValue = M.x86.R_CL; 
			break;
		case DH_REG: 
			if((current->mode == ON_MODIFY && current->lastValue != M.x86.R_DH) ||  
				(current->mode == ON_BECOME && current->value == M.x86.R_DH && current->value != current->lastValue)) {
				newValue = M.x86.R_DH;
				lastValue = current->lastValue;
				breakFlag = true;
			}
			current->lastValue = M.x86.R_DH; 
			break;
		case DL_REG: 
			if((current->mode == ON_MODIFY && current->lastValue != M.x86.R_DL) ||  
				(current->mode == ON_BECOME && current->value == M.x86.R_DL && current->value != current->lastValue)) {
				newValue = M.x86.R_DL;
				lastValue = current->lastValue;
				breakFlag = true;
			}
			current->lastValue = M.x86.R_DL; 
			break;
		case AX_REG: 
			if((current->mode == ON_MODIFY && current->lastValue != M.x86.R_AX) ||  
				(current->mode == ON_BECOME && current->value == M.x86.R_AX && current->value != current->lastValue)) {
				newValue = M.x86.R_AX;
				lastValue = current->lastValue;
				breakFlag = true;
			}
			current->lastValue = M.x86.R_AX; 
			break;
		case BX_REG: 
			if((current->mode == ON_MODIFY && current->lastValue != M.x86.R_BX) ||  
				(current->mode == ON_BECOME && current->value == M.x86.R_BX && current->value != current->lastValue)) {
				newValue = M.x86.R_BX;
				lastValue = current->lastValue;
				breakFlag = true;
			}
			current->lastValue = M.x86.R_BX; 
			break;
		case CX_REG: 
			if((current->mode == ON_MODIFY && current->lastValue != M.x86.R_CX) ||  
				(current->mode == ON_BECOME && current->value == M.x86.R_CX && current->value != current->lastValue)) {
				newValue = M.x86.R_CX;
				lastValue = current->lastValue;
				breakFlag = true;
			}
			current->lastValue = M.x86.R_CX; 
			break;
		case DX_REG: 
			if((current->mode == ON_MODIFY && current->lastValue != M.x86.R_DX) ||  
				(current->mode == ON_BECOME && current->value == M.x86.R_DX && current->value != current->lastValue)) {
				newValue = M.x86.R_DX;
				lastValue = current->lastValue;
				breakFlag = true;
			}
			current->lastValue = M.x86.R_DX; 
			break;
		case CS_REG: 
			if((current->mode == ON_MODIFY && current->lastValue != M.x86.R_CS) ||  
				(current->mode == ON_BECOME && current->value == M.x86.R_CS && current->value != current->lastValue)) {
				newValue = M.x86.R_CS;
				lastValue = current->lastValue;
				breakFlag = true;
			}
			current->lastValue = M.x86.R_CS; 
			break;
		case DS_REG: 
			if((current->mode == ON_MODIFY && current->lastValue != M.x86.R_DS) ||  
				(current->mode == ON_BECOME && current->value == M.x86.R_DS && current->value != current->lastValue)) {
				newValue = M.x86.R_DS;
				lastValue = current->lastValue;
				breakFlag = true;
			}
			current->lastValue = M.x86.R_DS; 
			break;
		case ES_REG: 
			if((current->mode == ON_MODIFY && current->lastValue != M.x86.R_ES) ||  
				(current->mode == ON_BECOME && current->value == M.x86.R_ES && current->value != current->lastValue)) {
				newValue = M.x86.R_ES;
				lastValue = current->lastValue;
				breakFlag = true;
			}
			current->lastValue = M.x86.R_ES; 
			break;
		case SS_REG: 
			if((current->mode == ON_MODIFY && current->lastValue != M.x86.R_SS) ||  
				(current->mode == ON_BECOME && current->value == M.x86.R_SS && current->value != current->lastValue)) {
				newValue = M.x86.R_SS;
				lastValue = current->lastValue;
				breakFlag = true;
			}
			current->lastValue = M.x86.R_SS; 
			break;
		case IP_REG: 
			if((current->mode == ON_MODIFY && current->lastValue != M.x86.R_IP) ||  
				(current->mode == ON_BECOME && current->value == M.x86.R_IP && current->value != current->lastValue)) {
				newValue = M.x86.R_IP;
				lastValue = current->lastValue;
				breakFlag = true;
			}
			current->lastValue = M.x86.R_IP; 
			break;
		case SP_REG: 
			if((current->mode == ON_MODIFY && current->lastValue != M.x86.R_SP) ||  
				(current->mode == ON_BECOME && current->value == M.x86.R_SP && current->value != current->lastValue)) {
				newValue = M.x86.R_SP;
				lastValue = current->lastValue;
				breakFlag = true;
			}
			current->lastValue = M.x86.R_SP; 
			break;
		case BP_REG: 
			if((current->mode == ON_MODIFY && current->lastValue != M.x86.R_BP) ||  
				(current->mode == ON_BECOME && current->value == M.x86.R_BP && current->value != current->lastValue)) {
				newValue = M.x86.R_BP;
				lastValue = current->lastValue;
				breakFlag = true;
			}
			current->lastValue = M.x86.R_BP; 
			break;
		case SI_REG: 
			if((current->mode == ON_MODIFY && current->lastValue != M.x86.R_SI) ||  
				(current->mode == ON_BECOME && current->value == M.x86.R_SI && current->value != current->lastValue)) {
				newValue = M.x86.R_SI;
				lastValue = current->lastValue;
				breakFlag = true;
			}
			current->lastValue = M.x86.R_SI; 
			break;
		case DI_REG: 
			if((current->mode == ON_MODIFY && current->lastValue != M.x86.R_DI) ||  
				(current->mode == ON_BECOME && current->value == M.x86.R_DI && current->value != current->lastValue)) {
				newValue = M.x86.R_DI;
				lastValue = current->lastValue;
				breakFlag = true;
			}
			current->lastValue = M.x86.R_DI; 
			break;
		case OF_REG: 
			if((current->mode == ON_MODIFY && current->lastValue != OF_BIT) ||  
				(current->mode == ON_BECOME && current->value == OF_BIT && current->value != current->lastValue)) {
				newValue = OF_BIT;
				lastValue = current->lastValue;
				breakFlag = true;
			}
			current->lastValue = OF_BIT; 
			break;
		case DF_REG: 
			if((current->mode == ON_MODIFY && current->lastValue != DF_BIT) ||  
				(current->mode == ON_BECOME && current->value == DF_BIT && current->value != current->lastValue)) {
				newValue = DF_BIT;
				lastValue = current->lastValue;
				breakFlag = true;
			}
			current->lastValue = DF_BIT; 
			break;
		case IF_REG: 
			if((current->mode == ON_MODIFY && current->lastValue != IF_BIT) ||  
				(current->mode == ON_BECOME && current->value == IF_BIT && current->value != current->lastValue)) {
				newValue = IF_BIT;
				lastValue = current->lastValue;
				breakFlag = true;
			}
			current->lastValue = IF_BIT; 
			break;
		case TF_REG: 
			if((current->mode == ON_MODIFY && current->lastValue != TF_BIT) ||  
				(current->mode == ON_BECOME && current->value == TF_BIT && current->value != current->lastValue)) {
				newValue = TF_BIT;
				lastValue = current->lastValue;
				breakFlag = true;
			}
			current->lastValue = TF_BIT; 
			break;
		case SF_REG: 
			if((current->mode == ON_MODIFY && current->lastValue != SF_BIT) ||  
				(current->mode == ON_BECOME && current->value == SF_BIT && current->value != current->lastValue)) {
				newValue = SF_BIT;
				lastValue = current->lastValue;
				breakFlag = true;
			}
			current->lastValue = SF_BIT; 
			break;
		case ZF_REG: 
			if((current->mode == ON_MODIFY && current->lastValue != ZF_BIT) ||  
				(current->mode == ON_BECOME && current->value == ZF_BIT && current->value != current->lastValue)) {
				newValue = ZF_BIT;
				lastValue = current->lastValue;
				breakFlag = true;
			}
			current->lastValue = ZF_BIT; 
			break;
		case AF_REG: 
			if((current->mode == ON_MODIFY && current->lastValue != AF_BIT) ||  
				(current->mode == ON_BECOME && current->value == AF_BIT && current->value != current->lastValue)) {
				newValue = AF_BIT;
				lastValue = current->lastValue;
				breakFlag = true;
			}
			current->lastValue = AF_BIT; 
			break;
		case PF_REG: 
			if((current->mode == ON_MODIFY && current->lastValue != PF_BIT) ||  
				(current->mode == ON_BECOME && current->value == PF_BIT && current->value != current->lastValue)) {
				newValue = PF_BIT;
				lastValue = current->lastValue;
				breakFlag = true;
			}
			current->lastValue = PF_BIT; 
			break;
		case CF_REG: 
			if((current->mode == ON_MODIFY && current->lastValue != CF_BIT) ||  
				(current->mode == ON_BECOME && current->value == CF_BIT && current->value != current->lastValue)) {
				newValue = CF_BIT;
				lastValue = current->lastValue;
				breakFlag = true;
			}
			current->lastValue = CF_BIT; 
			break;

		// Begin PIC registers:
			
		case IMR_REG: 
			if((current->mode == ON_MODIFY && current->lastValue != ReadIMR()) ||  
				(current->mode == ON_BECOME && current->value == ReadIMR() && current->value != current->lastValue)) {
				newValue = ReadIMR();
				lastValue = current->lastValue;
				breakFlag = true;
			}
			current->lastValue = ReadIMR(); 
			break;
		case ISR_REG: 
			if((current->mode == ON_MODIFY && current->lastValue != ReadISR()) ||  
				(current->mode == ON_BECOME && current->value == ReadISR() && current->value != current->lastValue)) {
				newValue = ReadISR();
				lastValue = current->lastValue;
				breakFlag = true;
			}
			current->lastValue = ReadISR(); 
			break;
		case IRR_REG: 
			if((current->mode == ON_MODIFY && current->lastValue != ReadIRR()) ||  
				(current->mode == ON_BECOME && current->value == ReadIRR() && current->value != current->lastValue)) {
				newValue = ReadIRR();
				lastValue = current->lastValue;
				breakFlag = true;
			}
			current->lastValue = ReadIRR(); 
			break;

		}

		if(breakFlag) {
			stopFlag = true;
			if(current->mode == ON_MODIFY) {
				SimIO.PrintMessage(
					"< Register monitor break > -  Modification in register monitor ID %d.\n"
					"  Register %s was changed from %Xh to %Xh.\n",
					current->ID, registerName[current->registerNum], lastValue, newValue
				);
			}
			else {
				SimIO.PrintMessage(
					"< Register monitor break > -  Condition met for register monitor ID %d.\n"
					"  Register %s was changed from %Xh to %Xh.\n",
					current->ID, registerName[current->registerNum], lastValue, newValue
				);
			}
		}

		current = current->next;
	}
}


// This is the Simulators execution loop. Call this function to start the similator
// after initialization.
void CSimControl::ExecLoop(void)
{
	char cmdString[COMMAND_BUFFER_SIZE];
	int status;
	int length;

	SimIO.PrintText("\nWelcome to the BYU ECEn 425 8086 simulator.\nFor a list of commands, enter h or ?.\n\n");

	do {
		// Display Prompt
		SimIO.PrintText("Emu86>");

		// Get next command form user interface
		length = SimIO.GetCommandString(cmdString, COMMAND_BUFFER_SIZE);

		// Interpret command
		if(length > 0) status = ExecCommand(cmdString, &cmdCurrent);
	} while (status != CMD_QUIT);		
}


// Check if the passed physical address is within the CPUs available memory.
// Returns true if address is in valid range. Used for bounds checking to 
// prevent the simulator from crashing.
bool CSimControl::AddressInRange(int addr)
{
	if(addr < 0 || addr >= (long int)M.mem_size) return false;
	return true;
}


// Checks if a value of size bytes at address is within the CPUs available memory.
// Returns true if size bytes at address is in valid range. Used for bounds checking to 
// prevent the simulator from crashing.
bool CSimControl::AddressInRange(int addr, int size)
{
	if(addr < 0 || addr+size >= (long int)M.mem_size) return false;
	return true;
}

