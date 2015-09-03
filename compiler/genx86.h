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

#ifndef _GENX86_H
#define _GENX86_H

typedef unsigned int FLAGS;

#define	F_NONE	((FLAGS) 0U)
#define	F_DREG	((FLAGS) 1U)	/* a data register, like %eax, %edx etc. */
#define	F_AREG	((FLAGS) 2U)	/* an address  register, like %esi, %edi */
#define	F_MEM	((FLAGS) 4U)	/* direct, indirect, indexed */
#define	F_IMMED	((FLAGS) 8U)	/* immediate */
#define	F_FREG	((FLAGS) 16U)	/* top of floating-point stack */
#define	F_NOVALUE ((FLAGS) 32U)	/* dont need result value */
#define	F_VOL	((FLAGS) 64U)	/* need value in scratch register */
#define	F_NOEDI	((FLAGS) 128U)	/* do not use %edi and %esi */
#define	F_NOECX	((FLAGS) 256U)	/* do not use %exc */
#define	F_EAXEDX ((FLAGS) 512U)	/* result needed in %eax */
#define	F_ECX	((FLAGS) 1024U)	/* use %ecx if a register is needed */

#define F_ALL	((FLAGS)(F_DREG | F_AREG | F_MEM | F_IMMED | F_FREG))	/* any mode */

/*
 *   The 80X86 OP codes.  This table does not contain all the OP codes
 *   supported by the 80X86 but only those which the compiler
 *   generates.
 */
enum e_op
{
    op_movsbl,
    op_movzbl,
    op_movswl,
    op_movzwl,
    op_movsbw,
    op_movzbw,
    op_cdq,
    op_cwd,
    op_cbw,
    op_mov,
    op_xchg,
    op_lea,
    op_not,
    op_neg,
    op_add,
    op_sub,
    op_adc,
    op_sbb,
    op_imul,
    op_idiv,
    op_div,
    op_and,
    op_or,
    op_xor,
    op_inc,
    op_dec,
    op_cmp,
    op_push,
    op_pop,
    op_jmp,
    op_loop,
    op_call,
    op_leave,
    op_enter,
    op_ret,
    op_test,
    op_bra,
    op_je,
    op_jne,
    op_jl,
    op_jle,
    op_jg,
    op_jge,
    op_ja,
    op_jae,
    op_jb,
    op_jbe,
    op_rep,
    op_smov,
    op_shl,
    op_shr,
    op_asl,
    op_asr,
    op_rol,
    op_ror,
    op_sahf,
    op_sete,
    op_setne,
    op_setb,
    op_setbe,
    op_seta,
    op_setae,
    op_setl,
    op_setle,
    op_setg,
    op_setge,
    op_nop,
#ifdef FLOAT_IEEE
    op_fadd,
    op_faddp,
    op_fsub,
    op_fsubp,
    op_fdiv,
    op_fdivp,
    op_fmul,
    op_fmulp,
    op_fsubr,
    op_fsubrp,
    op_fdivr,
    op_fdivrp,
    op_fld,
    op_fldz,
    op_fst,
    op_fstp,
    op_fpop,
    op_fild,
    op_fildl,
    op_fistp,
    op_fistpl,
    op_ftst,
    op_fchs,
    op_fcomp,
    op_fcompp,
    op_fnstsw,
    op_fwait,
#endif				/* FLOAT_IEEE */
#ifdef ASM
    op_asm,			/* pseudo OP code ... holds ASM string */
#endif				/* ASM */
    op_line,			/* pseudo OP code ... line number info */
    op_label			/* pseudo OP code ... label */
};

#define OP_MIN	op_movsbl
#define OP_MAX	op_label

/* addressing modes */
enum e_am
{
    am_dreg,			/* Rn */
    am_areg,			/* Rn */
    am_ind,			/* (Rn) */
    am_indx,			/* (disp,Rn) */
    am_indx2,			/* (disp,Rn,Rm) */
    am_direct,			/* (xxx) */
    am_immed,			/* #(data) */
    am_freg,			/* ST(n) */
    am_mreg,			/* Rn/Rm */
    am_str,			/* string */
    am_line			/* pseudo mode for line numbers */
};

#define	is_register_mode(mode)	((mode) == am_dreg || (mode) == am_areg || (mode) == am_mreg)

/* register naming, special registers */

#define EAX	((REG) 0)
#define EDX	((REG) 1)
#define ECX	((REG) 2)
#define EBX	((REG) 3)
#define ESI	((REG) 4)
#define EDI	((REG) 5)
#define ESP	((REG) 6)
#define EBP	((REG) 7)	/* frame pointer */
/* attention: same order as above */
#define	AX	((REG) 8)
#define DX	((REG) 9)
#define BX	((REG) 10)
#define CX	((REG) 11)
#define SI	((REG) 12)
#define DI	((REG) 13)
#define	SP	((REG) 14)
#define	BP	((REG) 15)
/* attention: same order as above */
#define AL	((REG) 16)
#define DL	((REG) 17)
#define BL	((REG) 18)
#define CL	((REG) 19)
/* attention: same order as above */
#define	AH	((REG) 20)
#define	DH	((REG) 21)
#define	BH	((REG) 22)
#define	CH	((REG) 23)

#define	ST0	((REG) 24)
#define	ST1	((REG) 25)
#define	ST2	((REG) 26)
#define	ST3	((REG) 27)
#define	ST4	((REG) 28)
#define	ST5	((REG) 29)
#define	ST6	((REG) 30)
#define	ST7	((REG) 31)

#define	NUM_REGS	32	/* number of registers */

/*
 * The code generator does not distinguish between %eax, %ax, %al
 * because some assemblers want it strict, the real register names
 * are determined when the assembly instruction is PRINTED, e.g.
 * code generator produces movb junk,%eax,
 * assembly code printer prints movb junk,%al
 * The conversion is done by the following macros
 */
#define REG16(X) (((REG)(((int)X)-((int)EAX))+((int)AX)))
#define REG8(X)  (((REG)(((int)X)-((int)EAX))+((int)AL)))

#define REG8HI(X)  (((REG)(((int)X)+((int)AH))-((int)AL)))

#define NUMREG		((int)(REG8(EBP))+4)

#define STACKPTR	ESP	/* system stack pointer */
#define FRAMEPTR	EBP	/* frame pointer */
#define MAX_REG 	EBX	/* scratch registers: %eax..%edx */

#define	UNUSED		((DEEP)-1)	/* register stack entry unused */
#define	EMPTY		((DEEP)0)	/* register slot index */

/*
 *   Defines what the register can be used for
 */
#define	D_REG		((REGTYPE)1)	/* data register */
#define	A_REG		((REGTYPE)2)	/* address register */
#define	F_REG		((REGTYPE)4)	/* float register */
#define	T_REG		((REGTYPE)8)	/* temporary register */
#define	M_REG		((REGTYPE)16)	/* multiple data register */
#define	X_REG		((REGTYPE)32)	/* AX/DX register */
#define	Y_REG		((REGTYPE)64)	/* not AX/DX register */
#define	C_REG		((REGTYPE)128)	/* CX data register */
#define Z_REG		((REGTYPE)256)	/* AX data register */
#define N_REG		((REGTYPE)512)	/* no CX data register */
#define	is_data_register(r)		((regtypes[(int)r] & D_REG) != 0)
#define	is_address_register(r)		((regtypes[(int)r] & A_REG) != 0)
#define	is_float_register(r)		((regtypes[(int)r] & F_REG) != 0)
#define	is_data_or_address_register(r)	((regtypes[(int)r] & (D_REG | A_REG)) != 0)
#define	is_temporary_register(r)	((regtypes[(int)r] & T_REG) != 0)
#define	is_temporary_data_register(r)	((regtypes[(int)r] & (D_REG|T_REG)) == (D_REG|T_REG))
#define	is_temporary_address_register(r)	((regtypes[(int)r] & (A_REG|T_REG)) == (A_REG|T_REG))
typedef unsigned short REGTYPE;

#ifndef _CODE_DEFINED
#define _CODE_DEFINED
typedef struct ocode CODE;

#endif

/* instruction lengths */

enum ilength
{
    IL0 = 0,			/* no operands */
    IL1 = 1,			/* byte operands */
    IL2 = 2,			/* word operands */
    IL4 = 4,			/* double word operands / float operands */
    IL8 = 8,			/* long real */
    IL10 = 10			/* long double real */
};

#define	REGBIT(x)	(REGMASK)(1U << (int)x)

typedef enum e_am AMODE;	/* Addressing mode */
typedef struct amode ADDRESS;
typedef enum e_op OPCODE;
typedef enum ilength ILEN;
typedef unsigned int REGMASK;
typedef signed char DEEP;
typedef struct reglist REGLIST;

#define NIL_ADDRESS	((ADDRESS *) 0)
#define	NIL_CODE	((CODE *) 0)

/* stack optimisations */
#define	OPT_SAFE	0
#define	OPT_MINIMUM	1
#define	OPT_AVERAGE	2
#define	OPT_MAXIMUM	3


/* addressing mode structure */

struct amode
{
    AMODE   mode;		/* addressing mode */
    REG     preg;		/* primary register used in address */
    REG     sreg;		/* secondary register used in address */
    DEEP    deep;		/* position is "pushed" stack */
    union
    {
	EXPR   *offset;		/* expression used in address mode */
    } u;
};

/* output code structure */

struct ocode
{
    OPCODE  opcode;		/* opcode for this instruction */
    ILEN    length;		/* operand length of the instruction */
    ADDRESS *oper1;		/* first operand */
    ADDRESS *oper2;		/* second operand */
    CODE   *fwd;		/* next instruction */
    CODE   *back;		/* previous instruction */
};

struct reglist
{
    int     number;		/* number of registers in the list */
    REG    *reg;		/* register list */
};

/*
 *   The usage of registers is controlled by the information held
 *   within the following structure.
 */
struct reg_use
{
    REGLIST *parameter;		/* Registers used to pass parameters */
    REGLIST *save;		/* Registers saved by the function */
    REGLIST *result;		/* Registers used to return results */
};

#define BRANCH_COUNT	2	/* abandon branch optimisation if exceeded */

#define	PEEP_NONE		0
#define	PEEP_INSTRUCTION	0
#define	PEEP_JUMPS		1
#define	PEEP_FLOW		2
#define	PEEP_ALL		255
#define	is_peep_phase(l,x)	(((unsigned)l) & MEMBER(x))

#ifdef MULTIPLE_PROCESSORS
/*
 *   remap function names - it is necessary to change the names of
 *   functions if there are multiple code generators build into the
 *   compiler in order to prevent name clashes.
 *
 *   The following defines do the necessary renaming
 */
#define	checkstack		checkstackX86
#define	is_equal_address	is_equal_addressX86
#define	flush_peep		flush_peepX86
#define	freeop			freeopX86
#define	g_code			g_codeX86
#define	g_fcode			g_fcodeX86
#define	data_register		data_registerX86
#define	data_register_no_cx	data_register_no_cxX86
#define	address_register	address_registerX86
#define	axdx_register		axdx_registerX86
#define	ax_register		ax_registerX86
#define	cx_register		cx_registerX86
#define	float_register		float_registerX86
#define	get_mregister		get_mregisterX86
#define	initstack		initstackX86
#define	is_register_used	is_register_usedX86
#define	mk_immed		mk_immedX86
#define	mk_label		mk_labelX86
#define	mk_reg			mk_regX86
#define	mk_mreg			mk_mregX86
#define	temp_inv		temp_invX86
#define	uses_temp		uses_tempX86
#define	validate		validateX86
#define	max_reg			max_regX86
#define	regtypes		regtypesX86
#define	reg_usage		reg_usageX86
#define	reglist_to_mask		reglist_to_maskX86
#endif /* MULTIPLE_PROCESSORS */

extern REGUSAGE *reg_usage;	/* register usage */
extern REGTYPE *regtypes;	/* register types */

/* gen?86.c */
ADDRESS *mk_reg P_ ((REG));
ADDRESS *mk_mreg P_ ((REG, REG));
ADDRESS *mk_label P_ ((LABEL));
ADDRESS *mk_immed P_ ((IVAL));

/* peepx86.c */
void g_code P_ ((OPCODE, ILEN, ADDRESS *, ADDRESS *));
void flush_peep P_ ((unsigned));
BOOL is_equal_address P_ ((ADDRESS *, ADDRESS *));

#ifdef FLOAT_SUPPORT
void g_fcode P_ ((OPCODE, ILEN, ADDRESS *, ADDRESS *));

#endif /* FLOAT_SUPPORT */

/* regx86.c */
ADDRESS *address_register P_ ((void));
ADDRESS *axdx_register P_ ((void));
ADDRESS *ax_register P_ ((void));
ADDRESS *cx_register P_ ((void));
ADDRESS *data_register P_ ((void));
ADDRESS *data_register_no_cx P_ ((void));
ADDRESS *mdata_register P_ ((void));
BOOL is_register_used P_ ((REG));
BOOL uses_temp P_ ((const ADDRESS *));
REGMASK reglist_to_mask P_ ((const REGLIST *));
void initstack P_ ((void));
void checkstack P_ ((void));
void validate P_ ((const ADDRESS *));
void freeop P_ ((const ADDRESS *));
void temp_inv P_ ((void));

#ifdef FLOAT_SUPPORT
ADDRESS *float_register P_ ((void));

#endif /* FLOAT_SUPPORT */

#endif /* _GENX86_H */
