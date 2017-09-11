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
 *		  originally by M. Brandt
 * 1990   68000 C compiler further bug fixes
 *		  started i386 port (December)
 * 1991   i386 port finished (January)
 *		  further corrections in the front end and in the 68000
 *		  code generator.
 *		  The next port will be a SPARC port
 */

#ifndef _GENARM_H
#define _GENARM_H


typedef unsigned int FLAGS;

#define F_NONE	((FLAGS) 0U)
#define F_REG	((FLAGS) 1U)	/* a register  */
#define F_MEM	((FLAGS) 2U)	/* memory alterable modes allowed               */
#define F_IMMED ((FLAGS) 4U)	/* immediate mode allowed                               */
#define F_VOL	((FLAGS) 8U)	/* need volatile operand                                */
#define F_NOVALUE ((FLAGS) 16U)	/* dont need result value                               */
#define F_USES	((FLAGS) 32U)	/* need result value more than once     */

#define F_ALL	((FLAGS)(F_REG | F_IMMED | F_MEM))	/* any mode */


enum e_op
{
    op_adc,			/* Add with carry */
    op_add,			/* Add without carry */
    op_adcs,			/* Add with carry set condition codes */
    op_adds,			/* Add without carry, set condition codes */
    op_sbc,			/* Subtract with carry */
    op_sub,			/* subtract without carry */
    op_sbcs,			/* Subtract with carry, set condition codes */
    op_subs,			/* Subtract without carry, set condition codes */
    op_rsb,			/* Reverse subtract with carry */
    op_rsbs,			/* Reverse subtract with caryy, set condition codes */
    op_and,			/* Bitwise AND */
    op_ands,			/* Bitwise AND, set condition codes */
    op_bic,			/* Bitwise AND NOT */
    op_bics,			/* Bitwise AND NOT, set condition codes */
    op_orr,			/* Bitwise OR */
    op_orrs,			/* Bitwise OR, set condition codes */
    op_eor,			/* Bitwise EOR */
    op_eors,			/* Bitwise EOR, set condition codes */
    op_mov,			/* Move */
    op_movs,			/* Move, set condition codes */
    op_mvn,			/* Move NOT */
    op_mvns,			/* Move NOT, set condition codes */
    op_cmns,			/* Compare NOT */
    op_cmps,			/* compare */
    op_teqs,			/* Test equal */
    op_tsts,			/* Test */
    op_mul,			/* Multiply */
    op_mla,			/* Multiply -accumulate */
    op_b,			/* Branch */
    op_bl,			/* Branch and link */
    op_ldr,			/* Load register */
    op_str,			/* Store register */
    op_ldmea,			/* Load multiple registers (empty stack, descending) */
    op_stmfd,			/* Store multiple registers (full stack, descending) */
    op_swi,			/* SoftWare Interupt */
#ifdef ASM
    op_asm,			/* ASM string */
#endif				/* ASm */
    op_line,			/* Pseudo Op for line numbers */
    op_label			/* Pseudo Op for labels */
};

#define OP_MIN	op_adc
#define OP_MAX	op_label

enum e_cc
{
    cc_al,			/* Always */
    cc_cc,			/* Carry clear */
    cc_cs,			/* Carry set */
    cc_eq,			/* Equal */
    cc_ge,			/* Greater than or equal */
    cc_gt,			/* Greater than */
    cc_hi,			/* Higher */
    cc_le,			/* Less than or equal */
    cc_ls,			/* Lower or same */
    cc_lt,			/* Less than */
    cc_mi,			/* Negative */
    cc_ne,			/* Not equal */
    cc_nv,			/* Never */
    cc_pl,			/* Positive */
    cc_vc,			/* Overflow clear */
    cc_vs			/* Overflow set */
};

enum e_am
{
    am_none,
    am_direct,			/* Label */
    am_reg,			/* Rn */
    am_immed,			/* #value */
    am_lsl,			/* Rn,LSL #value */
    am_lsr,			/* Rn,LSR #value */
    am_asr,			/* Rn,ASR #value */
    am_ror,			/* Rn,ROR #value */
    am_lslr,			/* Rn,LSL Rm */
    am_lsrr,			/* Rn,LSR Rm */
    am_asrr,			/* Rn,ASR Rm */
    am_rorr,			/* Rn,ROR Rm */
    am_pre,			/* [Rn,#0] */
    am_prelsl,			/* [Rn,Rm,LSL #value] */
    am_prelsr,			/* [Rn,Rm,LSR #value] */
    am_preasr,			/* [Rn,Rm,ASR #value] */
    am_preror,			/* [Rn,Rm,ROR #value] */
    am_post,			/* [Rn],#0 */
    am_postlsl,			/* [Rn],Rm,LSL #value */
    am_postlsr,			/* [Rn],Rm,LSR #value */
    am_postasr,			/* [Rn],Rm,ASR #value */
    am_postror,			/* [Rn],Rm,ROR #value */
    am_mask,			/* {Rn-Rm} */
    am_str,			/* string */
    am_line
};

#define R0	 ((REG) 0)	/* R0 register */
#define R1	 ((REG) 1)	/* R1 register */
#define R2	 ((REG) 2)	/* R2 register */
#define R3	 ((REG) 3)	/* R3 register */
#define R4	 ((REG) 4)	/* R4 register */
#define R5	 ((REG) 5)	/* R5 register */
#define R6	 ((REG) 6)	/* R6 register */
#define R7	 ((REG) 7)	/* R7 register */
#define R8	 ((REG) 8)	/* R8 register */
#define R9	 ((REG) 9)	/* R9 register */
#define R10  ((REG) 10)		/* R10 register */
#define R11  ((REG) 11)		/* R11 register */
#define R12  ((REG) 12)		/* R12 register */
#define R13  ((REG) 13)		/* R13 register */
#define R14  ((REG) 14)		/* R14 register */
#define R15  ((REG) 15)		/* R15 register */

#define NUM_REGS	16	/* number of distinct registers */

#define MAX_REG 	R3	/* scratch registers */
#define LIMITPTR	R10	/* stack limit */
#define FRAMEPTR	R11	/* stack frame register */
#define WORKPTR 	R12	/* temporary work register */
#define STACKPTR	R13	/* stack pointer */
#define LINKPTR 	R14	/* link address register */
#define PCPTR		R15	/* Program counter register */

typedef enum e_cc CONDITION;	/* Condition codes */

/* support routines */
#define SUP_STACK	(CHAR *)"x$stack_overflow"

#ifndef _CODE_DEFINED
#define _CODE_DEFINED
typedef struct ocode CODE;

#endif

/* instruction lengths */

enum ilength
{
    IL0 = 0,
    IL1 = 1,
    IL2 = 2,
    IL4 = 4
};

#define	REGBIT(x)	(REGMASK)(1U << (int)x)

typedef enum e_am AMODE;	/* Addressing mode */
typedef struct amode ADDRESS;
typedef enum e_op OPCODE;
typedef enum ilength ILEN;
typedef unsigned int REGMASK;
typedef signed char DEEP;
typedef struct reglist REGLIST;

#define NIL_ADDRESS ((ADDRESS *) 0)
#define NIL_CODE	((CODE *) 0)


/* addressing mode structure */

struct amode
{
    AMODE   mode;		/* addressing mode */
    REG     preg, sreg;		/* register(s) used in addressing mode */
    DEEP    deep;
    EXPR   *offset;		/* expression used in addression mode */
};

/* output code structure */

struct ocode
{
    OPCODE  opcode;		/* opcode for this instruction */
    CONDITION cc;		/* condition codes */
    ADDRESS *oper1;		/* first operand */
    ADDRESS *oper2;		/* second operand */
    ADDRESS *oper3;		/* third operand */
    CODE   *fwd;		/* next instruction */
    CODE   *back;		/* previous instruction */
};

struct reglist
{
    int     number;		/* number of registers in the list */
    REG    *reg;		/* register list */
};

/*
 *	 The usage of registers is controlled by the information held
 *	 within the following structure.
 */
struct reg_use
{
    REGLIST *parameter;		/* Registers used to pass parameters */
    REGLIST *save;		/* Registers saved by the function */
    REGLIST *result;		/* Registers used to return results */
};

/* peephole optimisations */
#define PEEP_NONE	0
#define PEEP_FLOW	1
#define PEEP_ALL	255

#ifdef MULTIPLE_PROCESSORS
/*
 * remap function names - it is necessary to change the names of functions
 * if there are multiple code generators build into the compiler in order
 * to prevent name clashes.
 *
 * The following defines do the necessary renaming
 */
#define checkstack		checkstackARM
#define is_equal_address	is_equal_addressARM
#define flush_peep		flush_peepARM
#define freeop			freeopARM
#define g_code			g_codeARM
#define data_register		data_registerARM
#define initstack		initstackARM
#define mk_reg			mk_regARM
#define temp_inv		temp_invARM
#define validate		validateARM
#define max_reg 		max_regARM
#define reg_usage		reg_usageARM
#endif /* MULTIPLE_PROCESSORS */

extern REG max_reg;		/* maximum temporary register */
extern REGUSAGE *reg_usage;	/* register usage */

/* genarm.c */
ADDRESS *mk_reg P_ ((REG));

/* peeparm.c */
void g_code P_ ((OPCODE, CONDITION, ADDRESS *, ADDRESS *, ADDRESS *));
void flush_peep P_ ((unsigned));

/* regarm.c */
ADDRESS *data_register P_ ((void));
void initstack P_ ((void));
void checkstack P_ ((void));
void validate P_ ((const ADDRESS *));
void freeop P_ ((const ADDRESS *));
void temp_inv P_ ((void));

#endif /* _GENARM_H */
