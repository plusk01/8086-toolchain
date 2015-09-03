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

#ifndef _GENPPC_H
#define _GENPPC_H

enum e_flags
{
    F_NONE = 0,
    F_DREG = 1,			/* data register direct mode allowed    */
    F_AREG = 2,			/* address register direct mode allowed */
    F_FREG = 4,			/* floating point register mode allowed */
    F_MEM = 8,			/* memory alterable modes allowed       */
    F_IMMED = 16,		/* immediate mode allowed               */
    F_VOL = 32,			/* need volatile operand                */
    F_NOVALUE = 64,		/* dont need result value               */
    F_USES = 128		/* need result value more than once     */
	/* this forbids autoincrement modes     */
};

#define	F_ALL	  ((FLAGS)(F_DREG | F_AREG | F_FREG | F_MEM | F_IMMED))	/* all modes allowed */

/* The instructions */

/*
 * The order of the branch instructions must not be changed
 * since array revcond[] in the peephole optimizer relies on
 * them.
 */
enum e_op
{
    op_move,
    op_moveq,
    op_add,
    op_addi,
    op_addq,
    op_sub,
    op_subi,
    op_subq,
    op_muls,
    op_mulu,
    op_divs,
    op_divu,
    op_and,
    op_andi,
    op_or,
    op_ori,
    op_eor,
    op_asl,
    op_lsr,
    op_jmp,
    op_jsr,
    op_bsr,
    op_movem,
    op_rts,
    op_bra,
    op_beq,
    op_bne,
    op_blt,
    op_ble,
    op_bgt,
    op_bge,
    op_bhi,
    op_bhs,
    op_blo,
    op_bls,
    op_btst,
    op_tst,
    op_ext,
    op_extb,
    op_lea,
    op_swap,
    op_neg,
    op_not,
    op_cmp,
    op_clr,
    op_link,
    op_unlk,
    op_pea,
    op_cmpi,
    op_dbra,
    op_asr,
    op_rol,
    op_ror,
    op_seq,
    op_sne,
    op_slt,
    op_sle,
    op_sgt,
    op_sge,
    op_shi,
    op_shs,
    op_slo,
    op_sls,
    op_st,
    op_nop,
#ifdef FLOAT_IEEE
    op_fabs,
    op_fneg,
    op_fadd,
    op_fsub,
    op_fdiv,
    op_fmul,
    op_fcmp,
    op_ftst,
    op_fmove,
    op_fmovem,
#endif				/* FLOAT_IEEE */
#ifdef ASM
    op_asm,
#endif				/* ASM */
    op_line,
    op_label
};

#define OP_MIN	op_move
#define OP_MAX	op_label

/* addressing modes */
enum e_am
{
    am_dreg,			/* Dn */
    am_areg,			/* An */
    am_ind,			/* (An) */
    am_ainc,			/* (An)+ */
    am_adec,			/* -(An) */
    am_indx,			/* (d16,An) */
    am_indx2,			/* (d8,An,Dn.l) */
    am_indx3,			/* (d8,An,Am.l) */
    am_indx4,			/* (d8,An,Dn.w) */
    am_direct,			/* (xxx) */
    am_immed,			/* #(data) */
    am_smask,			/* Register mask (D0 high, A7 low) */
    am_rmask,			/* Register mask (A7 high, D0 low) */
    am_freg,			/* Floating Pointer Register FPn */
    am_none,
    am_mreg,			/* Dn/Dm */
    am_xreg,			/* Dn/Dm/Dl */
    am_str,			/* string */
    am_line			/* pseudo mode for line numbers */
};

/* register naming, special registers */

#define	GPR0		((REG) 0)	/* GPR0 register */
#define	GPR1		((REG) 1)	/* GPR1 register */
#define	GPR2		((REG) 2)	/* GPR2 register */
#define	GPR3		((REG) 3)	/* GPR3 register */
#define	GPR4		((REG) 4)	/* GPR4 register */
#define	GPR5		((REG) 5)	/* GPR5 register */
#define	GPR6		((REG) 6)	/* GPR6 register */
#define	GPR7		((REG) 7)	/* GPR7 register */
#define	GPR8		((REG) 8)	/* GPR8 register */
#define	GPR9		((REG) 9)	/* GPR9 register */
#define	GPR31		((REG) 31)	/* GPR31 register */

#define	FPR0		((REG) 32)	/* FPR0 register */
#define	FPR31		((REG) 63)	/* FPR31 register */

#define	NUM_REGS	64	/* number of distinct registers */

#define FRAMEPTR  	A6	/* frame pointer register */
#define STACKPTR  	GPR0	/* system stack pointer register */	/*KDW */

#ifdef FLOAT_IEEE
#define	MAX_REG		FPR31
#else
#define	MAX_REG		GPR31
#endif /* FLOAT_IEEE */

#define	MAX_REG_STACK	((DEEP)30)
#define	UNUSED		((DEEP)-1)
#define	EMPTY		((DEEP)0)

/*
 *    Defines what the register can be used for
 */
#define	D_REG		((REGTYPE)1)	/* data register */
#define	A_REG		((REGTYPE)2)	/* address register */
#define	F_REG		((REGTYPE)4)	/* float register */
#define	T_REG		((REGTYPE)8)	/* temporary register */
#define	is_data_register(r)		((regtype[(int)r] & D_REG) != 0)
#define	is_address_register(r)		((regtype[(int)r] & A_REG) != 0)
#define	is_float_register(r)		((regtype[(int)r] & F_REG) != 0)
#define	is_temporary_register(r)	((regtype[(int)r] & T_REG) != 0)
#define	is_temporary_data_register(r)	((regtype[(int)r] & (D_REG|T_REG)) == (D_REG|T_REG))
#define	is_temporary_address_register(r)	((regtype[(int)r] & (A_REG|T_REG)) == (A_REG|T_REG))
#define	is_temporary_float_register(r)	((regtype[(int)r] & (F_REG|T_REG)) == (F_REG|T_REG))
typedef unsigned char REGTYPE;

/* support routines */
#define	SUP_ASLDIV	(CHAR *)".Xasldiv"
#define	SUP_ASLMUL	(CHAR *)".Xaslmul"
#define	SUP_ASLREM	(CHAR *)".Xaslrem"
#define	SUP_ASOPDF	(CHAR *)".Yasopdf"
#define	SUP_ASOPL	(CHAR *)".Ybfasop"
#define	SUP_ASOPLF	(CHAR *)".Yasoplf"
#define	SUP_ASOPSF	(CHAR *)".Yasopsf"
#define	SUP_ASULDIV	(CHAR *)".Xasuldiv"
#define	SUP_ASULMUL	(CHAR *)".Xasulmul"
#define	SUP_ASULREM	(CHAR *)".Xasulrem"
#define	SUP_LDIV	(CHAR *)".Xldiv"
#define	SUP_LMUL	(CHAR *)".Xlmul"
#define	SUP_LREM	(CHAR *)".Xlrem"
#define	SUP_ULDIV	(CHAR *)".Xuldiv"
#define	SUP_ULMUL	(CHAR *)".Xulmul"
#define	SUP_ULREM	(CHAR *)".Xulrem"
#ifdef FLOAT_IEEE
#define	SUP_ASDFADD	(CHAR *)".Yasdfadd"
#define	SUP_ASDFDIV	(CHAR *)".Yasdfdiv"
#define	SUP_ASDIVDF	(CHAR *)".Yasdivdf"
#define	SUP_ASDFMUL	(CHAR *)".Yasdfmul"
#define	SUP_ASMULDF	(CHAR *)".Yasmuldf"
#define	SUP_ASDFSUB	(CHAR *)".Yasdfsub"
#define	SUP_ASLFADD	(CHAR *)".Yaslfadd"
#define	SUP_ASLFDIV	(CHAR *)".Yaslfdiv"
#define	SUP_ASDIVLF	(CHAR *)".Yasdivlf"
#define	SUP_ASLFMUL	(CHAR *)".Yaslfmul"
#define	SUP_ASMULLF	(CHAR *)".Yaslfmullf"
#define	SUP_ASLFSUB	(CHAR *)".Yaslfsub"
#define	SUP_ASSFSUB	(CHAR *)".Yassfsub"
#define	SUP_ASSFADD	(CHAR *)".Yassfadd"
#define	SUP_ASSFDIV	(CHAR *)".Yassfdiv"
#define	SUP_ASDIVSF	(CHAR *)".Yasdivsf"
#define	SUP_ASSFMUL	(CHAR *)".Yassfmul"
#define	SUP_ASMULSF	(CHAR *)".Yasmulsf"
#define	SUP_DFADD	(CHAR *)".Ydfadd"
#define	SUP_DFCMP	(CHAR *)".Ydfcmp"
#define	SUP_DFDEC	(CHAR *)".Ydfdec"
#define	SUP_DFDIV	(CHAR *)".Ydfdiv"
#define	SUP_DFINC	(CHAR *)".Ydfinc"
#define	SUP_DFLTODF	(CHAR *)".Ydfltodf"
#define	SUP_DFMUL	(CHAR *)".Ydfmul"
#define	SUP_DFNEG	(CHAR *)".Ydfneg"
#define	SUP_DFSUB	(CHAR *)".Ydfsub"
#define	SUP_DFTOL	(CHAR *)".Ydftol"
#define	SUP_DFTOLF	(CHAR *)".Ydftolf"
#define	SUP_DFTOSF	(CHAR *)".Ydftosf"
#define	SUP_DFTOUL	(CHAR *)".Ydftoul"
#define	SUP_DFTST	(CHAR *)".Ydftst"
#define	SUP_DFUTODF	(CHAR *)".Ydfutodf"
#define	SUP_LFADD	(CHAR *)".Ylfadd"
#define	SUP_LFCMP	(CHAR *)".Ylfcmp"
#define	SUP_LFDEC	(CHAR *)".Ylfdec"
#define	SUP_LFDIV	(CHAR *)".Ylfdiv"
#define	SUP_LFINC	(CHAR *)".Ylfinc"
#define	SUP_LFMUL	(CHAR *)".Ylfmul"
#define	SUP_LFNEG	(CHAR *)".Ylfneg"
#define	SUP_LFSUB	(CHAR *)".Ylfsub"
#define	SUP_LFLTOLF	(CHAR *)".Ylfltodf"
#define	SUP_LFTODF	(CHAR *)".Ylftodf"
#define	SUP_LFTOL	(CHAR *)".Ylftol"
#define	SUP_LFTOUL	(CHAR *)".Ylftoul"
#define	SUP_LFTOSF	(CHAR *)".Ylftosf"
#define	SUP_LFTST	(CHAR *)".Ylftst"
#define	SUP_LFUTOLF	(CHAR *)".Ylfutolf"
#define	SUP_SFADD	(CHAR *)".Ysfadd"
#define	SUP_SFCMP	(CHAR *)".Ysfcmp"
#define	SUP_SFDEC	(CHAR *)".Ysfdec"
#define	SUP_SFDIV	(CHAR *)".Ysfdiv"
#define	SUP_SFINC	(CHAR *)".Ysfinc"
#define	SUP_SFLTOSF	(CHAR *)".Ysfltosf"
#define	SUP_SFMUL	(CHAR *)".Ysfmul"
#define	SUP_SFNEG	(CHAR *)".Ysfneg"
#define	SUP_SFSUB	(CHAR *)".Ysfsub"
#define	SUP_SFTODF	(CHAR *)".Ysftodf"
#define	SUP_SFTOL	(CHAR *)".Ysftol"
#define	SUP_SFTOLF	(CHAR *)".Ysftodf"
#define	SUP_SFTOUL	(CHAR *)".Ysftoul"
#define	SUP_SFTST	(CHAR *)".Ysftst"
#define	SUP_SFUTOSF	(CHAR *)".Ysfutosf"
#endif /* FLOAT_IEEE */
#ifdef FLOAT_MFFP
#define	SUP_ASFPADD	(CHAR *)".Xasfpadd"
#define	SUP_ASFPDEC	(CHAR *)".Xasfpdec"
#define	SUP_ASFPDIV	(CHAR *)".Xasfpdiv"
#define	SUP_ASFPINC	(CHAR *)".Xasfpinc"
#define	SUP_ASFPMULT	(CHAR *)".Xasfpmult"
#define	SUP_ASFPREM	(CHAR *)".Xasfprem"
#define	SUP_ASFPSUB	(CHAR *)".Xasfpsub"
#define	SUP_FPADD	(CHAR *)".Xfpadd"
#define	SUP_FPADD	(CHAR *)".Xfpadd"
#define	SUP_FPCMP	(CHAR *)".Xfpcmp"
#define	SUP_FPDEC	(CHAR *)".Xfpdec"
#define	SUP_FPDIV	(CHAR *)".Xfpdiv"
#define	SUP_FPFTOL	(CHAR *)".Xfpftol"
#define	SUP_FPFTOU	(CHAR *)".Xfpftou"
#define	SUP_FPINC	(CHAR *)".Xfpinc"
#define	SUP_FPLTOF	(CHAR *)".Xfpltof"
#define	SUP_FPMULT	(CHAR *)".Xfpmult"
#define	SUP_FPNEG	(CHAR *)".Xfpneg"
#define	SUP_FPREM	(CHAR *)".Xfprem"
#define	SUP_FPSUB	(CHAR *)".Xfpsub"
#define	SUP_FPSUB	(CHAR *)".Xfpsub"
#define	SUP_FPTST	(CHAR *)".Xfptst"
#define	SUP_FPUTOF	(CHAR *)".Xfputof"
#endif /* FLOAT_IEEE */
#ifdef STACK_CHECK
#define	SUP_STACKCHECK	(CHAR *)".stackcheck"
#endif /* STACK_CHECK */

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
    IL4 = 4,
    IL8 = 8,
    IL12 = 12
};

typedef enum e_am AMODE;	/* Addressing mode */
typedef struct amode ADDRESS;
typedef enum e_flags FLAGS;
typedef enum e_op OPCODE;
typedef enum ilength ILEN;
typedef unsigned long REGMASK;
typedef signed char DEEP;
typedef struct reglist REGLIST;

#define NIL_ADDRESS	((ADDRESS *) 0)
#define	NIL_CODE	((CODE *) 0)


/* addressing mode structure */

struct amode
{
    AMODE   mode;		/* addressing mode */
    REG     preg, sreg;		/* register(s) used in addressing mode */
    DEEP    deep;
    union
    {
	EXPR   *offset;		/* expression used in addressing mode */
	REG     xreg;		/* 3rd register */
	REGMASK mask;		/* register mask */
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
 *    The usage of registers is controlled by the information held
 *      within the following structure.
 */
struct reg_use
{
    REGLIST *parameter;		/* Registers used to pass parameters */
    REGLIST *save;		/* Registers saved by the function */
    REGLIST *result;		/* Registers used to return results */
};

#define BRANCH_COUNT	2	/* abandon branch optimisation if exceeded */

/* peehole optimisations */
#define	PEEP_NONE	0
#define	PEEP_FLOW	1
#define	PEEP_DEBUG	2
#define	PEEP_ALL	255

#ifdef MULTIPLE_PROCESSORS
/*
 * remap function names - it is necessary to change the names of functions
 * if there are multiple code generators build into the compiler in order
 * to prevent name clashes.
 *
 * The following defines do the necessary renaming
 */
#define	data_register		data_registerPPC
#define	address_register	address_registerPPC
#define	float_register		float_registerPPC
#define	mdata_register		mdata_registerPPC
#define	xdata_register		xdata_registerPPC
#define	checkstack		checkstackPPC
#define	copy_addr		copy_addrPPC
#define	is_equal_address	is_equal_addressPPC
#define	is_free_data		is_free_dataPPC
#define	is_free_addr		is_free_addrPPC
#define	flush_peep		flush_peepPPC
#define	freeop			freeopPPC
#define	g_code			g_codePPC
#define	g_fcode			g_fcodePPC
#define	get_register		get_registerPPC
#define	initstack		initstackPPC
#define	mk_label		mk_labelPPC
#define	mk_reg			mk_regPPC
#define	mk_mreg			mk_mregPPC
#define	mk_xreg			mk_xregPPC
#define	temp_inv		temp_invPPC
#define	temp_reg		temp_regPPC
#define	validate		validatePPC
#define	reg_usage		reg_usagePPC
#define	regtype			regtypePPC
#define	reglist_to_mask		reglist_to_maskPPC
#endif /* MULTIPLE_PROCESSORS */

extern REGUSAGE *reg_usage;	/* register usage */
extern REGTYPE regtype[];	/* type of register */

/* genppc */
ADDRESS *mk_reg P_ ((REG));
ADDRESS *mk_mreg P_ ((REG, REG));
ADDRESS *mk_xreg P_ ((REG, REG, REG));
ADDRESS *mk_label P_ ((LABEL));
ADDRESS *copy_addr P_ ((ADDRESS *, AMODE));
BOOL is_short P_ ((const EXPR *));

/* peepppc */
void g_code P_ ((OPCODE, ILEN, ADDRESS *, ADDRESS *));
void g_fcode P_ ((OPCODE, ILEN, ADDRESS *, ADDRESS *));
void flush_peep P_ ((unsigned));
BOOL is_equal_address P_ ((ADDRESS *, ADDRESS *));

/* regppc */
ADDRESS *address_register P_ ((void));
ADDRESS *data_register P_ ((void));
ADDRESS *float_register P_ ((void));
ADDRESS *mdata_register P_ ((void));
ADDRESS *xdata_register P_ ((void));
ADDRESS *temp_reg P_ ((FLAGS));
BOOL is_free_addr P_ ((void));
BOOL is_free_data P_ ((void));
REGMASK reglist_to_mask P_ ((const REGLIST *));
void checkstack P_ ((void));
void freeop P_ ((const ADDRESS *));
void initstack P_ ((void));
void temp_inv P_ ((void));
void validate P_ ((const ADDRESS *));

#endif /* _GENPPC */
