/*
 * C compiler
 * ==========
 *
 * Copyright 1989, 1990, 1991 Christoph van Wuellen.
 * Credits to Matthew Brandt.
 * All commercial rights reserved.
 *
 * This compiler may be redistributed as long there is no
 * commercial interest. The compiler must not be redistributed
 * without its full sources. This notice must stay intact.
 *
 * History:
 *
 * 1989   starting an 68000 C compiler, starting with material
 *        originally by M. Brandt
 * 1990   68000 C compiler further bug fixes
 *        started i386 port (December)
 * 1991   i386 port finished (January)
 *        further corrections in the front end and in the 68000
 *        code generator.
 *        The next port will be a SPARC port
 */

#include "chdr.h"
#include "expr.h"
#include "cglbdec.h"

/*
 * global definitions
 */
FHANDLE input;			/* input source file */
FHANDLE output;			/* output assembler file */
FHANDLE errfile;		/* error listing file */

#ifdef LIST
FHANDLE listfile;		/* source listing file */

#endif /* LIST */
#ifdef DEBUG
FHANDLE debugfile;		/* output file for internal debugging */

#endif /* DEBUG */
LINE    act_line;		/* current line being processed */
const CHAR *act_linetxt;	/* text of the current line */
const CHAR *act_file = (const CHAR *) "<stdin>";	/* current file being processed */
const CHAR *in_file = (const CHAR *) "<stdin>";	/* initial file being processed */
LABEL   nextlabel;		/* next internally generated label */

#ifdef EPOC
const char *newline = "\r\n";	/* new line character sequence */

#else
const char *newline = "\n";	/* new line character sequence */

#endif /* EPOC */

/*
 * scanner values
 */
TOKEN   lastst;			/* last symbol token read by the scanner */
const CHAR *lastsym;		/* pointer to the last symbol */
SIZE    lastsymlen;		/* length of the value in lastsym */
UVAL    ival;			/* integral value */

#ifdef FLOAT_SUPPORT
RVAL    rval;			/* floating point value */

#endif /* FLOAT_SUPPORT */

/*
 * stack frame values
 */
SIZE    lc_auto;		/* the current size of the stack frame */
SIZE    lc_auto_max = 0;	/* the maximum size that the stack frame has grown */

SIZE    bits_in_sizeunit = 8L;	/* number of bits which make a generic object */

int     global_flag;		/* allocate memory for "permanent use */
int     error_resync = 0;	/* prevent error message cascade */
int     total_errors = 0;	/* number of errors currently encountered */

/*
 * These are the default compiler options
 */
#ifdef INTEL
BOOL    align_option = TRUE;	/* align structure fields to smallest alignment */

#else
BOOL    align_option = FALSE;	/* align structure fields to smallest alignment */

#endif /* INTEL */
#ifdef ASM
//BOOL    asm_option = FALSE;	/* asm keyword allowed */
BOOL    asm_option = TRUE;	// -WSF

#endif /* ASM */
BOOL    bitfield_option = FALSE;	/* reverse allocation order of bitfields */
BOOL    code_option = TRUE;	/* generate code */
int     datamodel_option = 0;	/* method of referencing global data */

#ifdef DEBUGOPT
BOOL    debug_option = FALSE;	/* generate debugging information */

#endif /*DEBUGINFO */

#ifdef EXTERNAL
BOOL    extern_option = FALSE;	/* generate extern definitions for all globals */

#endif /* EXTERNAL */
#ifdef FLOAT_CHECK
BOOL    fcheck_option = FALSE;	/* warn about floating point operations */

#endif /* FLOAT_CHECK */

#ifdef DOINLINE
int	inline_option = 100;    /* controls whether a function is inlined */
#endif /* DOINLINE */

#ifdef DEBUG
int     internal_option = 0;	/* controls internal debugging options */

#endif /* DEBUG */
BOOL    lattice_option = FALSE;	/* allow Lattice stype stdarg definitions */
BOOL    IandD_option = FALSE;	/* separate Instruction and Data segments */
int     lang_option = LANG_C90;	/* C language syntax */

#ifdef LIST
BOOL    listing_option = FALSE;	/* list the input source and symbol tables */

#endif /* LIST */
BOOL    longdouble_option = FALSE;	/* long doubles are to be the same size as doubles */
BOOL    obsolete_option = FALSE;	/* future language directions defined obsolete feature */
BOOL    opt_option = TRUE;	/* use the global optimiser */
BOOL    optimize_option = FALSE;	/* do "expensive" optimisations */

#ifdef PACKENUM
BOOL    packenum_option = FALSE;	/* enumerations are smallest integer type to hold values */

#endif /* PACKENUM */
BOOL   packbit_option = FALSE;	/* pack bitfields into specified type */
BOOL    promoteparam_option = FALSE;	/* promote parameters */

BOOL    reg_option = TRUE;	/* honour the "register" storage */

#ifdef EPOC
BOOL    short_option = TRUE;	/* integers are to be the same size as shorts */

// Follwoing added. -WSF
#elif defined INTEL_86_DEFAULT
BOOL	short_option = TRUE;
// End added

#else
BOOL    short_option = FALSE;	/* integers are to be the same size as shorts */

#endif /* EPOC */
#ifdef EPOC
BOOL    small_option = TRUE;	/* small model (8086) */

// Follwoing added. -WSF
#elif defined INTEL_86_DEFAULT
BOOL	small_option = TRUE;
// End added

#else
BOOL    small_option = FALSE;	/* small model (8086) */

#endif /* EPOC */
#ifdef TOPSPEED
BOOL    topspeed_option = FALSE;	/* Enable TopSpeed extensions */

#endif /* TOPSPEED */
BOOL    trad_option = FALSE;	/* accept only K&R plus a few extras */

#ifdef TRANSLATE
BOOL    trans_option = FALSE;	/* convert identifier > 8 character in output file */

#endif /* TRANSLATE */
BOOL    uchar_option = FALSE;	/* char type to be unsigned */
BOOL    verbose_option = FALSE;	/* output extra statistics */
int     error_option = ERRORLEVEL_DEFAULT;	/* the current error message level */
int     warn_option = WARNLEVEL_DEFAULT;	/* the current warning message level */
BOOL    fpu_option = FALSE;	/* generate code which contains floating point instructions */
BOOL    fpu_return_option = FALSE;	/* return FP value in FP register */

#ifdef ICODE
BOOL    icode_option = FALSE;	/* generate an icode file */

#endif /* ICODE */
#ifdef PROBES
BOOL    probe_option = FALSE;	/* generate stack probes on function entry */

#endif /* PROBE */
#ifdef FORMAT_CHECK
BOOL    format_option = TRUE;	/* check fprintf and fscanf format strings */

#endif /* FORMAT_CHECK */
#ifdef STACK_CHECK
BOOL    stackcheck_option = FALSE;	/* check stack with run-time routine */

#endif /* STACK_CHECK */
#ifdef TRACE
BOOL    trace_option = FALSE;	/* generate trace code */

#endif /* TRACE */
int     max_error_count = MAX_ERROR_COUNT;	/* maximum number of errors before stopping */

#ifdef ASMCALL
CHAR *asmcall_option = NULL;	/* function call via user specified assembler */
#endif /* ASMCALL */

TYP    *ret_type;		/* function return type */
EXPR   *init_node;		/* initialisations in compound statement */

#ifndef EPOC
const char *msgtable[] = {
#undef MSG
#define	MSG(x,y,z)	z,
#include "message.h"		/*lint !e537*/	/* repeated include file */
    (const char *) NULL
};

#endif /*EPOC */

#ifdef VERBOSE
/*
 * statistics collected during the verbose mode
 */
clock_t decl_time = (clock_t) 0;	/* time spent parsing the declarations */
clock_t parse_time = (clock_t) 0;	/* time spend parsing the statements/expressions */
clock_t opt_time = (clock_t) 0;	/* time spend in the optimisers */
clock_t gen_time = (clock_t) 0;	/* time spend in the code generator */

#endif /* VERBOSE */

BLOCK   init_block = {
    {
     NIL_SYM, NIL_SYM},
    {
     NIL_SYM, NIL_SYM},
    NIL_BLOCK,
    0L
};

					/* empty block table */
BOOL    errorloop = FALSE;	/* prevents recursion during error recovery */

/*
 * the basic types
 */
TYP    *tp_void;		/* void type */
TYP    *tp_long;		/* long type */
TYP    *tp_ulong;		/* unsigned long type */
TYP    *tp_char;		/* char type */
TYP    *tp_uchar;		/* unsigned char type */
TYP    *tp_schar;		/* signed char type */
TYP    *tp_short;		/* short type */
TYP    *tp_ushort;		/* unsigned short type */
TYP    *tp_int;			/* int type */
TYP    *tp_uint;		/* unsigned int type */
TYP    *tp_float;		/* float type */
TYP    *tp_double;		/* double type */
TYP    *tp_longdouble;		/* long double type */
TYP    *tp_string;		/* pointer to char type */
TYP    *tp_conststring;		/* const pointer to char type */
TYP    *tp_wstring;		/* pointer to wchar_t type */
TYP    *tp_func;		/* function type */
TYP    *tp_pointer;		/* pointer type */
TYP    *tp_array;		/* array type */
TYP    *tp_enum;		/* enumeration type */
TYP    *tp_struct;		/* struct type */
TYP    *tp_union;		/* union type */
TYP    *tp_ellipsis;		/* ellipsis type */
TYP    *tp_bool;		/* _Bool type */
TYP    *tp_longlong;		/* long long type */
TYP    *tp_ulonglong;		/* unsigned long long type */
TYP    *tp_floatcomplex;	/* float _Complex */
TYP    *tp_floatimaginary;	/* float _Imaginary */
TYP    *tp_doublecomplex;	/* double _Complex */
TYP    *tp_doubleimaginary;	/* double _Imaginary */
TYP    *tp_longdoublecomplex;	/* long double _Complex */
TYP    *tp_longdoubleimaginary;	/* long double _Imaginary */

/*
 * defined types
 */
TYP    *tp_wchar;		/* wchar_t type */
TYP    *tp_size;		/* size_t type */
TYP    *tp_ptrdiff;		/* ptrdiff_t type */

/*
 * names used in the parser and code generators
 */
const CHAR *alloca_name;	/* pointer to the name alloca */
const CHAR *printf_name;	/* pointer to the name printf */
const CHAR *fprintf_name;	/* pointer to the name fprintf */
const CHAR *sprintf_name;	/* pointer to the name sprintf */
const CHAR *scanf_name;		/* pointer to the name scanf */
const CHAR *fscanf_name;	/* pointer to the name fscanf */
const CHAR *sscanf_name;	/* pointer to the name sscanf */
const CHAR *func_name;		/* pointer to the name __func__ */
const CHAR *main_name;		/* pointer to the name main */

STRING *strtab;			/* table of strings to be output to the assembler file */
BOOL    uses_structassign;	/* function uses a structure assignment */
BOOL    is_leaf_function;	/* function doesn't call any other function */
BOOL    is_parameter = FALSE;	/* controls whether stack optimisation is allowed */

#ifdef MC680X0
BOOL    volatile_found = FALSE;	/* the volatile keyword has been found */

#endif /* MC680X0 */

#ifdef FLOAT_SUPPORT
/*
 * floating point constants used within the compiler
 */
RVAL    F_zero;			/* contains the value  0.0 */
RVAL    F_one;			/* contains the value  1.0 */
RVAL    F_two;			/* contains the value  2.0 */
RVAL    F_ten;			/* contains the value 10.0 */
RVAL    F_sixteen;		/* contains the value 16.0 */
RVAL    F_half;			/* contains the value  0.5 */

#endif /* FLOAT_SUPPORT */

#ifdef CPU_DEFINED
/*
 * code generator values
 */

#ifdef MULTIPLE_PROCESSORS
#ifdef MC68K_DEFAULT
struct genfuncs *GFuncs = &mc68k_funcs;

#else
#ifdef INTEL_386_DEFAULT
struct genfuncs *GFuncs = &mc386_funcs;

#else
#ifdef INTEL_86_DEFAULT
struct genfuncs *GFuncs = &mc86_funcs;

#endif /* INTEL_86_DEFAULT */
#ifdef TMS320C30_DEFAULT
struct genfuncs *GFuncs = &mcc30_funcs;

#endif /* TMS320C30_DEFAULT */
#endif /* INTEL_386_DEFAULT */
#endif /* MC68K_DEFAULT */
#endif /* MULTIPLE_PROCESSORS */

#ifdef MULTIPLE_ASSEMBLERS
/*
 *  Pointers to target code generator and assembler
 */
#ifdef MC680X0
#ifdef TARGET_ACK_DEFAULT
struct funcs *Funcs = &ack68k_funcs;

#else
#ifdef TARGET_GAS_DEFAULT
struct funcs *Funcs = &gas68k_funcs;

#else
#ifdef TARGET_CPM_DEFAULT
struct funcs *Funcs = &cpm68k_funcs;

#else
#ifdef TARGET_QMAC_DEFAULT
struct funcs *Funcs = &qmac68k_funcs;

#endif /* TARGET_QMAC_DEFAULT */
#endif /* TARGET_CPM_DEFAULT */
#endif /* TARGET_GAS_DEFAULT */
#endif /* TARGET_ACK_DEFAULT */
#else
#ifdef INTEL
#ifdef TARGET_NASM_DEFAULT
struct funcs *Funcs = &nasmx86_func;

#else
#ifdef TARGET_MASM_DEFAULT
struct funcs *Funcs = &masmx86_func;

#else
#ifdef TARGET_BAS_DEFAULT
struct funcs *Funcs = &basx86_func;

#else
#ifdef TARGET_GAS_DEFAULT
struct funcs *Funcs = &gasx86_func;

#else
#ifdef TARGET_SVR4_DEFAULT
struct funcs *Funcs = &sysvx86_func;

#endif /* TARGET_SVR4_DEFAULT */
#endif /* TARGET_GAS_DEFAULT */
#endif /* TARGET_BAS_DEFAULT */
#endif /* TARGET_MASM_DEFAULT */
#endif /* TARGET_NASM_DEFAULT */

#else
#ifdef ARM
#ifdef TARGET_ARM_DEFAULT
struct funcs *Funcs = &armar_func;

#else
#ifdef TARGET_OBJ_DEFAULT
struct funcs *Funcs = &armobj_func;
#endif /* TARGET_OBJ_DEFAULT */
#endif /* TARGET_ARM_DEFAULT */

#else
#ifdef TMS320C30
#ifdef TARGET_ROSSIN_DEFAULT
struct funcs *Funcs = &rosc30_funcs;

#endif /* TARGET_ROSSIN_DEFAULT */
#endif /* TMS320C30 */
#endif /* ARM */
#endif /* INTEL */
#endif /* MC680X0 */

#endif /* MULTIPLE_ASSEMBLERS */

SWITCH *swtables;		/* switch jump tables to be output to the assembler file */
const CHAR *external_prefix = (const CHAR *) ""; // -WSF, was: "_";	/* prefix for external/global symbols */
SIZE    stack_offset = 0L;	/* the number of bytes to remove from the stack */
SIZE    max_scratch;		/* the maximum number of bytes allocated to temporary variables */
SIZE    act_scratch;		/* the current number of bytes allocated to temporary variables */

#ifdef MULTIPLE_DEBUG
struct dbgfuncs *DbgFuncs = &dwarf2_funcs;

#endif /* MULTIPLE_DEBUG */

#endif /* CPU_DEFINED */
