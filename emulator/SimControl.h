// SimControl.h: interface for the CSimControl class.
//
// This class is the heart of the simulator interface. It interprets
// commands and conains the main execution loop for executing
// instructions.
//
// This header file contains the many defenitions needed by the
// simulator interface. The simulator can be reconfigured by changing
// these defenitions.
//
//////////////////////////////////////////////////////////////////////


#ifndef SIMCONTROL_H
#define SIMCONTROL_H

#include "ProgramWindow.h"
#include "SimWindow.h"
#include "Simptris.h"

#include "SymbolTable.h"
#include "Utils.h"

#include "x86emu/x86emui.h"


// Simulator configuration //////////////////////////////////////////

//#define SIM_MEM_SIZE			0x40000		// Should be <= 2^20  and must be a multiple of 0x10 (to avoid crashes in memory dumps!!)
#define SIM_MEM_SIZE			0x100000
#define MEM_INIT_VALUE			0xF1		// Value used to initialize memory (value is an illegal opcode).
#define MAX_COMMAND_PARAMETERS	3			// Maximum number of arguments simulator command can have
#define COMMAND_BUFFER_SIZE		70			// Size of string to receive commands
#define DEFAULT_MEM_DUMP_SIZE	1			// Default num bytes to dump with mem dump command. (Will always dump a multiple of 0x10)
#define DEFAULT_DISASSEMBE_SIZE	20			// Default number of instructions so disassemble with disassemble command
#define DEFAULT_STACK_DUMP_SIZE 10			// Default number of bytes to dump from top of stack
#define DEFAULT_HISTORY_SIZE	10			// Default number of history entries to display with history command

#define DATA_SEGMENT	0x00	// Same as CS
#define EXTRA_SEGMENT	0x00	// Same as CS
#define STACK_SEGMENT	0x00	// Same as CS
#define CODE_SEGMENT	0x00	// 0x30 would offset 0x100 will avoid the 1024 byte vector table
#define CODE_OFFSET		0x100	// Use 100 for DOS COM compatibility
#define STACK_OFFSET	0xFFFE	// ((STACK_SEGMENT << 4) + STACK_OFFSET) must be less than SIM_MEM_SIZE

#define DEFAULT_EXE_LOAD_SEGMENT	0x16	// Base segment at which to load EXE files. Physical addr 100h
#define DEFAULT_LOAD_SEGMENT		0x0		// Use first segment
#define DEFAULT_LOAD_OFFSET			0x0		// Default offset for DOS COM files is 100h

#define DEFAULT_TICK_INTERVAL	10000

#define HISTORY_LENGTH	5000	// Length of interrupt and instruction history to keep track of


// SimContorl method values ///////////////////////////////////

#define DECIMAL		1
#define UNSIGNED	2
#define HEX			3

#define FILE_LOADED		1
#define FILE_NOT_LOADED 2


// Simulator Command IDs /////////////////////////////////////////////

enum CommandID {
	CMD_INVALID,

	CMD_HELP,
	CMD_EXPLAIN_REGISTERS,

	CMD_LOAD,
	CMD_QUIT,
	CMD_CLEAR,

	CMD_GIVE_ADDRESS,

	CMD_EXECUTE,
	CMD_EXECUTE_TO,
	CMD_STEP,
	CMD_STEP_OVER,
	CMD_VERBOSE_TOGGLE,

	CMD_DISPLAY_HISTORY,
	CMD_DISPLAY_REGISTERS,
	CMD_DISASSEMBLE,

	CMD_DUMP_MEM,
	CMD_DUMP_MEM_BYTE,
	CMD_DUMP_MEM_WORD,
	CMD_DUMP_MEM_DWORD,
	CMD_DUMP_STACK,

	CMD_POKE_REGISTER,
	CMD_POKE_MEM_BYTE,
	CMD_POKE_MEM_WORD,
	CMD_POKE_MEM_DWORD,

	CMD_SET_TICK,
	CMD_ASSERT_INTERRUPT,

	CMD_DISPLAY_BREAKPOINTS,
	CMD_CLEAR_BREAKPOINT,
	CMD_SET_BREAKPOINT,

	CMD_DISPLAY_REG_MONITORS,
	CMD_CLEAR_REG_MONITOR,
	CMD_SET_REG_MONITOR,

	CMD_CLEAR_ALL,

	CMD_DISPLAY_MEM_MONITORS,
	CMD_CLEAR_MEM_MONITOR,
	CMD_SET_MEM_MONITOR,

	CMD_SIMPTRIS,
	CMD_SIMPTRIS_KILL
};



// Exception Codes ///////////////////////////////////////////////////

#define EXCEPTION_BYTE_READ_OUT_OF_RANGE	0
#define EXCEPTION_WORD_READ_OUT_OF_RANGE	1
#define EXCEPTION_DWORD_READ_OUT_OF_RANGE	2
#define EXCEPTION_BYTE_WRITE_OUT_OF_RANGE	3
#define EXCEPTION_WORD_WRITE_OUT_OF_RANGE	4
#define EXCEPTION_DWORD_WRITE_OUT_OF_RANGE	5
#define EXCEPTION_DIVIDE_BY_ZERO			6
#define EXCEPTION_DIVIDE_OVERFLOW			7
#define EXCEPTION_BAD_OPCODE				8
#define EXCEPTION_BAD_EXT_OPCODE			9



// Monitor Definitions ///////////////////////////////////////////////

// Break modes for registers and memory regions
#define ON_NONE		-1
#define ON_READ		0
#define ON_WRITE	1
#define ON_MODIFY	2
#define ON_ACCESS	3
#define ON_BECOME	4
#define DEFAULT_MODE	ON_MODIFY

#define DEFAULT_MONITOR_SIZE	2		// Make it word size

// Memory monitor data struct
typedef struct MemoryMonitorStruct {
	int			ID;				// Uniqely identifies this monitor
	Address		address;		// Address of first byte to monitor
	int			size;			// Number of bytes to monitor
	int			mode;			// Condition under wich to stop. (ON_READ, ON_WRITE, etc.)
	unsigned	value;			// If mode is ON_BECOME, this is the value we're watching for.
	MemoryMonitorStruct *next;	// For linked list
} MemoryMonitor;

// Register monitor data struct
typedef struct RegisterMonitorStruct {
	int			ID;				// Uniqely identifies this monitor
	int				registerNum;		// Number of the register to monitor (see Utils.h for numbers)
	unsigned		lastValue;			// Value the register had last time it was checked
	int				mode;				// Mode
	unsigned		value;				// If mode is ON_BECOME, this is the value we're watching for.
	RegisterMonitorStruct *next;		// For linked list
} RegisterMonitor;



// Breakpoint Definitions ///////////////////////////////////////////

typedef struct BreakpointStruct {
	int			ID;				// Uniqely identifies this breakpoint
	Address		address;		// Address at which to break
	int			type;			// Identifies purpose of breakpoint
	BreakpointStruct *next;		// For linked list
} Breakpoint;

#define BREAK_USER			0
#define BREAK_STEP_OVER		1


// History Lists /////////////////////////////////////////////////////

typedef struct {
	unsigned short segment;
	unsigned short offset;
} InstHistEntry;

typedef struct {
	unsigned short segment;
	unsigned short offset;
	int vector;
} IntHistEntry;


// Command Structure /////////////////////////////////////////////////

typedef struct {
	char		*cmdString;							// Point to command string.
	int			numParams;							// Number of parameters given
	CommandID	op;									// Command/operation to be performed
	char		*param[MAX_COMMAND_PARAMETERS];		// Pointer to argument strings for command within cmdString
} SimCommand;





// Class Defenition /////////////////////////////////////////////////

class CSimControl  
{
public:
	CSimControl();
	virtual ~CSimControl();
	
	// Interface //

	bool InitSim(char *path);
	void ExecLoop(void);

	void EnqueueInterrupt(int interruptVector);
	void ProgramTerminated(unsigned short exitCode);
	void SystemHalted(int errorCode);
	void AddressWrite(int address, int size, unsigned int newValue);
	void AddressRead(int readAddress, int readSize);
	void CheckBreakpoints(void);
	void CheckRegisterMonitors(void);

	bool SimptrisMode(void) { return simptris; }
	bool MemoryMonitors(void) { return memoryMonitorsActive; }
	bool Verbose(void) { return verboseMode; }

	void CheckReturn(void);

	// Program Window Interfaces //

	CSimWindow SimIO;
	CProgramWindow ProgramWindow;
	CSimptris Simptris;

private:
	void ResetProcessor(void);
	void ClearHistory(void);
	// void EnqueueInstruction(void);

	void Execute(unsigned int repeatCount);
	int LoadFileEXE(char *filename, int segment);
	int LoadFile(char *filename, Address *address);

	bool GetAddress(char *string, Address *address);

	unsigned SetRegister(int registerNum, unsigned value);
	void SetMemory(int address, unsigned value, int size);
	
	void DisplayBreakpoints(void);
	int  ClearBreakpoint(int ID);
	void ClearBreakpoints(void);
	int  SetBreakpoint(Address *address, int type);
	
	void InitRegisterMonitorContents(void);
	void DisplayRegMonitors(void);
	int  ClearRegMonitor(int ID);
	void ClearRegMonitors(void);
	int  SetRegMonitor(int registerNum, int mode, unsigned short value);

	void DisplayMemMonitors(void);
	int  ClearMemMonitor(int ID);
	void ClearMemMonitors(void);
	int  SetMemMonitor(Address *address, int size, int mode, unsigned int value);

	void DisplayRegisters(int format);

	void DumpMemory(void *memory, int segment, int offset, int numBytes);
	void DumpMemoryValues(void *memory, int segment, int offset, int numBytes, int size);

	void PrintInstruction(int segment, int offset, unsigned char *instData, int instSize, char *instruction, char sep);
	void Disassemble(int segment, int offset, int numInst);
	void DisassembleNext(void);

	int ExecCommand(char *cmdString, SimCommand *cmd);


	// Internal Methods for memory monitors

	bool MemoryWillChange(int writeAddress, int writeSize, unsigned int writeValue, int monitorAddress, int monitorSize);
	bool MemoryWillBecome(int writeAddress, int writeSize, unsigned int writeValue, int monitorAddress, int monitorSize, unsigned int monitorValue);

	
	// Command interpretation methods //

	void CmdInvalid(SimCommand *cmd);
	void CmdHelp(SimCommand *cmd);
	void CmdExplainRegisters(SimCommand *cmd);
	void CmdLoad(SimCommand *cmd);
	void CmdQuit(SimCommand *cmd);
	void CmdClear(SimCommand *cmd);
	void CmdExecute(SimCommand *cmd);
	void CmdExecuteTo(SimCommand *cmd);
	void CmdStep(SimCommand *cmd);
	void CmdStepOver(SimCommand *cmd);
	void CmdVerboseToggle(SimCommand *cmd);
	void CmdDisplayHistory(SimCommand *cmd);
	void CmdDisplayRegisters(SimCommand *cmd);
	void CmdGiveAddress(SimCommand *cmd);
	void CmdDisassemble(SimCommand *cmd);
	void CmdDumpMem(SimCommand *cmd);
	void CmdDumpStack(SimCommand *cmd);
	void CmdPokeReg(SimCommand *cmd);
	void CmdPokeMem(SimCommand *cmd);
	void CmdSetTick(SimCommand *cmd);
	void CmdAssertInterrupt(SimCommand *cmd);
	void CmdDisplayBreakpoints(SimCommand *cmd);
	void CmdClearBreakpoint(SimCommand *cmd);
	void CmdSetBreakpoint(SimCommand *cmd);
	void CmdDisplayRegMonitors(SimCommand *cmd);
	void CmdClearRegMonitor(SimCommand *cmd);
	void CmdSetRegMonitor(SimCommand *cmd);
	void CmdDisplayMemMonitors(SimCommand *cmd);
	void CmdClearMemMonitor(SimCommand *cmd);
	void CmdSetMemMonitor(SimCommand *cmd);
	void CmdClearAll(SimCommand *cmd);
	void CmdInitSimptris(SimCommand *cmd);
	

	// Commonly used methods //

	bool AddressInRange(int addr);
	bool AddressInRange(int addr, int size);
	unsigned int GetMemoryValue(int address, int size);


	// History Queues //

	InstHistEntry instHistory[HISTORY_LENGTH];
	IntHistEntry intHistory[HISTORY_LENGTH];
	int	instHistoryOffset;
	int intHistoryOffset;
	

	// Variable delcarations //

	int	stackStart;

	MemoryMonitor	*memoryMonitors;
	Breakpoint		*breakpoints;
	RegisterMonitor	*registerMonitors;

	unsigned int instructionCount;
	unsigned int tickInterval;
	unsigned int tickCount;
	bool verboseMode;
	
	char *execPath;

	void *pCoreMemory;


	// Flags //
	
	bool memoryMonitorsActive;
	bool registerMonitorsActive;
	bool breakpointsActive;
	bool simptris;
	bool systemHalted;
	bool stopFlag;

	

	// Class/Struct Modules //

	CSymbolTable symbolTable;
	SimCommand cmdCurrent;

};

#endif

