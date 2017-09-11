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

#ifndef _CGLBDEC_H
#define	_CGLBDEC_H

extern FHANDLE input;		/* input source file */
extern FHANDLE output;		/* output assembler file */
extern FHANDLE errfile;		/* error listing file */

#ifdef LIST
extern FHANDLE listfile;	/* source listing file */

#endif /* LIST */
#ifdef DEBUG
extern FHANDLE debugfile;	/* output file for internal debugging */

#endif /* DEBUG */
extern LINE act_line;		/* current line number being processed */
extern const CHAR *act_linetxt;	/* text of the current line */
extern const CHAR *act_file;	/* current source file being processed */
extern const CHAR *in_file;	/* initial source file being processed */
extern LABEL nextlabel;		/* next internally generated label */
extern const char *newline;	/* character(s) to use for a newline */

/*
 * scanner values
 */
extern TOKEN lastst;		/* last symbol token read by the scanner */
extern const CHAR *lastsym;	/* pointer to the last symbol */
extern SIZE lastsymlen;		/* length of the value in laststr */
extern UVAL ival;		/* integral value */

#ifdef FLOAT_SUPPORT
extern RVAL rval;		/* floating point value */

#endif /* FLOAT_SUPPORT */

extern const char *msgtable[];	/* table of messages output */

/*
 * stack frame values
 */
extern SIZE lc_auto;		/* the current size of the stack frame */
extern SIZE lc_auto_max;	/* the maximum size that the stack frame has grown */

extern SIZE bits_in_sizeunit;	/* number of bits which make a generic object */

extern int global_flag;		/* allocate memory for "permanent use */
extern int error_resync;	/* prevent error message cascade */
extern int total_errors;	/* number of errors currently encountered */

/*
 * compiler options
 */
extern BOOL align_option;	/* align structure fields to smallest alignment */

#ifdef ASM
extern BOOL asm_option;		/* asm keyword allowed */

#endif /* ASM */
extern BOOL bitfield_option;	/* reverse allocation order of bitfields */
extern BOOL code_option;	/* generate code */
extern int datamodel_option;	/* method of referencing global data */

#ifdef DEBUGOPT
extern BOOL debug_option;	/* generate debugging information */

#endif /*DEBUGOPT */
#ifdef EXTERNAL
extern BOOL extern_option;	/* generate extern definitions for all globals */

#endif /* EXTERNAL */
#ifdef FLOAT_CHECK
extern BOOL fcheck_option;	/* warn about floating point operations */

#endif /* FLOAT_CHECK */

#ifdef DOINLINE
extern int inline_option;	/* controls whether a function is inlined */
#endif /* DOINLINE */

#ifdef DEBUG
extern int internal_option;	/* controls internal debugging options */

#endif /* DEBUG */
extern BOOL lattice_option;	/* allow Lattice stype stdarg definitions */
extern BOOL IandD_option;	/* separate Instruction and Data segments */
extern int lang_option;		/* C language syntax */

#ifdef LIST
extern BOOL listing_option;	/* list the input source and symbol tables */

#endif /* LIST */
extern BOOL longdouble_option;	/* long doubles are to be the same size as doubles */
extern BOOL obsolete_option;	/* future language directions defined obsolete feature */
extern BOOL opt_option;		/* use the global optimiser */
extern BOOL optimize_option;	/* do "expensive" optimisations */

#ifdef PACKENUM
extern BOOL packenum_option;	/* enumerations are smallest integer type to hold values */

#endif /* PACKENUM */
extern BOOL packbit_option;	/* pack bitfields into specified type */
extern BOOL promoteparam_option;	/* promote parameters */

extern BOOL reg_option;		/* honour the "register" storage */
extern BOOL short_option;	/* integers are to be the same size as shorts */
extern BOOL small_option;	/* small model (8086) */

#ifdef TOPSPEED
extern BOOL topspeed_option;	/* Enable TopSpeed extensions */

#endif /* TOPSPEED */

#ifdef TRANSLATE
extern BOOL trans_option;	/* convert identifier > 8 character in output file */

#endif /* TRANSLATE */
extern BOOL uchar_option;	/* char type to be unsigned */
extern BOOL verbose_option;	/* output extra statistics */
extern int error_option;	/* the current error message level */
extern int warn_option;		/* the current warning message level */
extern BOOL fpu_option;		/* generate code which contains floating point instructions */
extern BOOL fpu_return_option;	/* return FP value in FP register */

#ifdef ICODE
extern BOOL icode_option;	/* generate an icode file */

#endif /* ICODE */
#ifdef PROBES
extern BOOL probe_option;	/* generate stack probes on function entry */

#endif /* PROBE */
#ifdef FORMAT_CHECK
extern BOOL format_option;	/* check fprintf and fscanf format strings */

#endif /* FORMAT_CHECK */
#ifdef STACK_CHECK
extern BOOL stackcheck_option;	/* check stack with run-time routine */

#endif /* STACK_CHECK */
extern BOOL trad_option;	/* accept only K&R plus a few extras */

#ifdef TRACE
extern BOOL trace_option;	/* generate trace code */

#endif /* TRACE */
extern int max_error_count;	/* maximum number of errors before stopping */

#ifdef ASMCALL
extern CHAR *asmcall_option;	/* function call via user specified assembler */
#endif /* ASMCALL */

extern TYP *ret_type;		/* function return type */
extern EXPR *init_node;		/* initialisations in compound statement */

#ifdef VERBOSE
/*
 * statistics collected during the verbose mode
 */
extern clock_t decl_time;	/* time spent parsing the declarations */
extern clock_t parse_time;	/* time spend parsing the statements/expressions */
extern clock_t opt_time;	/* time spend in the optimisers */
extern clock_t gen_time;	/* time spend in the code generator */

#endif /* VERBOSE */

extern BLOCK init_block;	/* empty block table */
extern BOOL errorloop;		/* prevents recursion during error recovery */

/*
 * the basic types
 */
extern TYP *tp_void;		/* void type */
extern TYP *tp_char;		/* char type */
extern TYP *tp_uchar;		/* unsigned char type */
extern TYP *tp_schar;		/* signed char type */
extern TYP *tp_int;		/* int type */
extern TYP *tp_uint;		/* unsigned int type */
extern TYP *tp_short;		/* short type */
extern TYP *tp_ushort;		/* unsigned short type */
extern TYP *tp_long;		/* long type */
extern TYP *tp_ulong;		/* unsigned long type */
extern TYP *tp_pointer;		/* pointer type */
extern TYP *tp_array;		/* array type */
extern TYP *tp_float;		/* float type */
extern TYP *tp_double;		/* double type */
extern TYP *tp_longdouble;	/* long double type */
extern TYP *tp_enum;		/* enum type */
extern TYP *tp_struct;		/* struct type */
extern TYP *tp_union;		/* union type */
extern TYP *tp_ellipsis;	/* ellipsis type */
extern TYP *tp_func;		/* function type */
extern TYP *tp_string;		/* pointer to char type */
extern TYP *tp_conststring;	/* const pointer to char type */
extern TYP *tp_wstring;		/* pointer to wchar_t type */
extern TYP *tp_bool;		/* _Bool type */
extern TYP *tp_longlong;	/* long long type */
extern TYP *tp_ulonglong;	/* unsigned long long type */
extern TYP *tp_floatcomplex;	/* float _Complex type */
extern TYP *tp_floatimaginary;	/* float _Imaginary type */
extern TYP *tp_doublecomplex;	/* double _Complex type */
extern TYP *tp_doubleimaginary;	/* double _Imaginary type */
extern TYP *tp_longdoublecomplex;	/* long double _Complex type */
extern TYP *tp_longdoubleimaginary;	/* long double _Imaginary type */

/*
 * defined types
 */
extern TYP *tp_wchar;		/* wchar_t type */
extern TYP *tp_size;		/* size_t type */
extern TYP *tp_ptrdiff;		/* ptrdiff_t type */

/*
 * names used in the parser and code generators
 */
extern const CHAR *alloca_name;	/* pointer to the name alloca */
extern const CHAR *printf_name;	/* pointer to the name printf */
extern const CHAR *fprintf_name;	/* pointer to the name fprintf */
extern const CHAR *sprintf_name;	/* pointer to the name sprintf */
extern const CHAR *scanf_name;	/* pointer to the name scanf */
extern const CHAR *fscanf_name;	/* pointer to the name fscanf */
extern const CHAR *sscanf_name;	/* pointer to the name sscanf */
extern const CHAR *func_name;	/* pointer to the name __func__ */
extern const CHAR *main_name;	/* pointer to the name main */

extern struct slit *strtab;	/* table of strings to be output to the assembler file */
extern BOOL uses_structassign;	/* function uses a structure assignment */
extern BOOL is_leaf_function;	/* function doesn't call any other function */
extern BOOL is_parameter;	/* controls whether stack optimisation is allowed */

#ifdef MC680X0
extern BOOL volatile_found;	/* the volatile keyword has been found */

#endif /* MC680X0 */

#ifdef FLOAT_SUPPORT
/*
 * floating point constants used within the compiler
 */
extern RVAL F_zero;		/* contains the value  0.0 */
extern RVAL F_one;		/* contains the value  1.0 */
extern RVAL F_two;		/* contains the value  2.0 */
extern RVAL F_ten;		/* contains the value 10.0 */
extern RVAL F_sixteen;		/* contains the value 16.0 */
extern RVAL F_half;		/* contains the value  0.5 */

#endif /* FLOAT_SUPPORT */

#ifdef CPU_DEFINED
/*
 * code generator values
 */

#ifdef MC680X0
extern OPTIONS opts68k;

#endif /* MC680X0 */

#ifdef INTEL_386
extern OPTIONS opts386;

#endif /* INTEL_386 */

#ifdef INTEL_86
extern OPTIONS opts86;

#endif /* INTEL_86 */

#ifdef ARM
extern OPTIONS optsarm;

#endif /* ARM */

#ifdef TMS320C30
extern OPTIONS optsc30;

#endif /* TMS320C30 */

#ifdef MULTIPLE_PROCESSORS
extern struct genfuncs *GFuncs;

#ifdef MC680X0
extern struct genfuncs mc68k_funcs;

#endif /* MC680X0 */
#ifdef INTEL_386
extern struct genfuncs mc386_funcs;

#endif /* INTEL_386 */
#ifdef INTEL_86
extern struct genfuncs mc86_funcs;

#endif /* INTEL_86 */
#ifdef ARM
extern struct genfuncs mcarm_funcs;

#endif /* ARM */
#ifdef TMS320C30
extern struct genfuncs mcc30_funcs;

#endif /* TMS320C30 */
#else
extern SIZE *g_alignments;	/* code generator alignment table */

#endif /* MULTIPLE_PROCESSORS */

#ifdef MULTIPLE_ASSEMBLERS
/*
 *  Pointers to target code generator and assembler
 */
extern struct funcs *Funcs;

#ifdef MC680X0
#ifdef TARGET_ACK
extern struct funcs ack68k_funcs;

#endif /* TARGET_ACK */
#ifdef TARGET_GAS
extern struct funcs gas68k_funcs;

#endif /* TARGET_GAS */
#ifdef TARGET_CPM
extern struct funcs cpm68k_funcs;

#endif /* TARGET_CPM */
#ifdef TARGET_QMAC
extern struct funcs qmac68k_funcs;

#endif /* TARGET_QMAC */
#endif /* MC680X0 */

#ifdef INTEL
#ifdef TARGET_NASM
extern struct funcs nasmx86_func;

#endif /* TARGET_NASM */
#ifdef TARGET_MASM
extern struct funcs masmx86_func;

#endif /* TARGET_MASM */
#ifdef TARGET_BAS
extern struct funcs basx86_func;

#endif /* TARGET_BAS */
#ifdef TARGET_GAS
extern struct funcs gasx86_func;

#endif /* TARGET_GAS */
#ifdef TARGET_SYSV
extern struct funcs sysvx86_func;

#endif /* TARGET_SYSV */
#endif /* INTEL */

#ifdef ARM
#ifdef TARGET_ARM
extern struct funcs armarm_funcs;

#endif /* TARGET_OBJ */
#ifdef TARGET_OBJ
extern struct funcs armobj_funcs;

#endif /* TARGET_OBJ */
#endif /* ARM */

#ifdef TMS320C30
#ifdef TARGET_ROSSIN
extern struct funcs rosc30_funcs;

#endif /* TARGET_ROSSIN */
#endif /* TMS320C30 */

#endif /* MULTIPLE_ASSEMBLERS */

extern const CHAR *external_prefix;	/* prefix for external/global symbols */
extern SWITCH *swtables;	/* switch jump tables to be output to the assembler file */
extern SIZE stack_offset;	/* the number of bytes to remove from the stack */
extern SIZE max_scratch;	/* the maxmimum number of bytes allocated to temporary variables */
extern SIZE act_scratch;	/* the current number of bytes allocated to temporary variables */

#endif /* CPU_DEFINED */

#endif /* _CGLBDEC_H */
