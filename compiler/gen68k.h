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

#ifndef _GEN68K_H
#define _GEN68K_H

enum
{
    target_68000,
    target_68010,
    target_coldfire,
    target_68020,
    target_68030,
    target_68040
};

#ifdef COLDFIRE
#define	is_coldfire()	(target_option == target_coldfire)
#else
/*lint -emacro( (506), is_coldfire )*/
/*lint -emacro( (774), is_coldfire )*/
#define	is_coldfire()	0
#endif

enum
{
    model_absolute,
    model_small,
    model_large
};

typedef unsigned int FLAGS;

#define	F_NONE	((FLAGS) 0U)
#define	F_DREG	((FLAGS) 1U)	/* data register direct mode allowed    */
#define	F_AREG	((FLAGS) 2U)	/* address register direct mode allowed */
#define	F_FREG	((FLAGS) 4U)	/* floating point register mode allowed */
#define	F_MEM	((FLAGS) 8U)	/* memory alterable modes allowed       */
#define	F_IMMED	((FLAGS) 16U)	/* immediate mode allowed               */
#define	F_VOL	((FLAGS) 32U)	/* need volatile operand                */
#define	F_NOVALUE ((FLAGS) 64U)	/* dont need result value               */
#define	F_USES	((FLAGS) 128U)	/* need result value more than once     */
#define	F_COLD	((FLAGS) 256U)	/* coldfire requires long               */
				/* this forbids autoincrement modes     */


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
    op_movea,
    op_add,
    op_addi,
    op_addq,
    op_adda,
    op_addx,
    op_sub,
    op_subi,
    op_subq,
    op_suba,
    op_subx,
    op_muls,
    op_mulu,
    op_divs,
    op_divu,
    op_and,
    op_or,
    op_eor,
    op_asl,
    op_lsr,
    op_jmp,
    op_jsr,
    op_bsr,
    op_movem,
    op_rts,
    op_rte,
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
    op_tst,
    op_ext,
    op_extb,
    op_lea,
    op_swap,
    op_neg, op_negx,
    op_not,
    op_cmp,
    op_cmpa,
    op_clr,
    op_link,
    op_unlk,
    op_pea,
    op_cmpi,
    op_dbra,
    op_asr,
    op_rol,
    op_ror,
    op_roxl,
    op_roxr,
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
    op_nop,
    op_trap,
#ifdef FLOAT_IEEE
    op_fadd,
    op_fsub,
    op_fdiv,
    op_fmul,
    op_fcmp,
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
    am_indxpc,			/* (d16,PC) */
    am_indx2pc,			/* (d8,An,PC) */
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

#define	D0		((REG) 0)	/* D0 register */
#define	D1		((REG) 1)	/* D1 register */
#define	D2		((REG) 2)	/* D2 register */
#define	D3		((REG) 3)	/* D3 register */
#define	D4		((REG) 4)	/* D4 register */
#define	D5		((REG) 5)	/* D5 register */
#define	D6		((REG) 6)	/* D6 register */
#define	D7		((REG) 7)	/* D7 register */

#define	A0		((REG) 8)	/* A0 register */
#define	A1		((REG) 9)	/* A1 register */
#define	A2		((REG) 10)	/* A2 register */
#define	A3		((REG) 11)	/* A3 register */
#define	A4		((REG) 12)	/* A4 register */
#define	A5		((REG) 13)	/* A5 register */
#define	A6		((REG) 14)	/* A6 register */
#define	A7		((REG) 15)	/* A7 register */

#define FP0		((REG) 16)	/* FP0 register */
#define FP1		((REG) 17)	/* FP1 register */
#define FP2		((REG) 18)	/* FP2 register */
#define FP3		((REG) 19)	/* FP3 register */
#define FP4		((REG) 20)	/* FP4 register */
#define FP5		((REG) 21)	/* FP5 register */
#define FP6		((REG) 22)	/* FP6 register */
#define FP7		((REG) 23)	/* FP7 register */

#define REG_MEMORY	((REG) 24)	/* special flag for peephole optimizer  */

#define	NUM_REGS	24	/* number of distinct registers */

#define	DATAPTR		A5	/* global data pointer register */
#define FRAMEPTR  	A6	/* frame pointer register */
#define STACKPTR  	A7	/* system stack pointer register */

#ifdef FLOAT_IEEE
#define	MAX_REG		FP7
#else
#define	MAX_REG		A7
#endif /* FLOAT_IEEE */

#define	MAX_REG_STACK	((DEEP)30)
#define	UNUSED		((DEEP)-1)
#define	EMPTY		((DEEP)0)

/*
 *   Defines what the register can be used for
 */
#define	D_REG		((REGTYPE)1)	/* data register */
#define	A_REG		((REGTYPE)2)	/* address register */
#define	F_REG		((REGTYPE)4)	/* float register */
#define	T_REG		((REGTYPE)8)	/* temporary register */
#define	is_data_register(r)		((regtype[(int)r] & D_REG) != (REGTYPE)0)
#define	is_address_register(r)		((regtype[(int)r] & A_REG) != (REGTYPE)0)
#define	is_float_register(r)		((regtype[(int)r] & F_REG) != (REGTYPE)0)
#define	is_temporary_register(r)	((regtype[(int)r] & T_REG) != (REGTYPE)0)
#define	is_temporary_data_register(r)	((regtype[(int)r] & (D_REG|T_REG)) == (D_REG|T_REG))
#define	is_temporary_address_register(r)	((regtype[(int)r] & (A_REG|T_REG)) == (A_REG|T_REG))
#define	is_temporary_float_register(r)	((regtype[(int)r] & (F_REG|T_REG)) == (F_REG|T_REG))
#define	set_temporary_register(r)	(regtype[(int)r] |= T_REG)
typedef unsigned char REGTYPE;

/* support routines */
#define	SUP_DATA	(const CHAR *)".Xdata"
#define	SUP_ASLDIV	(const CHAR *)".Xasldiv"
#define	SUP_ASLLDIV	(const CHAR *)".Xaslldiv"
#define	SUP_ASLLLSH	(const CHAR *)".Xaslllsh"
#define	SUP_ASLMUL	(const CHAR *)".Xaslmul"
#define	SUP_ASLLMUL	(const CHAR *)".Xasllmul"
#define	SUP_ASLREM	(const CHAR *)".Xaslrem"
#define	SUP_ASLLREM	(const CHAR *)".Xasllrem"
#define	SUP_ASLLRSH	(const CHAR *)".Xasllrsh"
#define	SUP_ASOPL	(const CHAR *)".Ybfasop"
#define	SUP_ASULDIV	(const CHAR *)".Xasuldiv"
#define	SUP_ASULLDIV	(const CHAR *)".Xasulldiv"
#define	SUP_ASULLLSH	(const CHAR *)".Xasulllsh"
#define	SUP_ASULMUL	(const CHAR *)".Xasulmul"
#define	SUP_ASULLMUL	(const CHAR *)".Xasullmul"
#define	SUP_ASULREM	(const CHAR *)".Xasulrem"
#define	SUP_ASULLREM	(const CHAR *)".Xasullrem"
#define	SUP_ASULLRSH	(const CHAR *)".Xasullrsh"
#define	SUP_LDIV	(const CHAR *)".Xldiv"
#define	SUP_LLDIV	(const CHAR *)".Xlldiv"
#define	SUP_LLLSH	(const CHAR *)".Xlllsh"
#define	SUP_LMUL	(const CHAR *)".Xlmul"
#define	SUP_LLMUL	(const CHAR *)".Xllmul"
#define	SUP_LREM	(const CHAR *)".Xlrem"
#define	SUP_LLREM	(const CHAR *)".Xllrem"
#define	SUP_LLRSH	(const CHAR *)".Xllrsh"
#define	SUP_ULDIV	(const CHAR *)".Xuldiv"
#define	SUP_ULLDIV	(const CHAR *)".Xulldiv"
#define	SUP_ULLLSH	(const CHAR *)".Xulllsh"
#define	SUP_ULMUL	(const CHAR *)".Xulmul"
#define	SUP_ULLMUL	(const CHAR *)".Xullmul"
#define	SUP_ULREM	(const CHAR *)".Xulrem"
#define	SUP_ULLREM	(const CHAR *)".Xullrem"
#define	SUP_ULLRSH	(const CHAR *)".Xullrsh"
#ifdef FLOAT_IEEE
#define	SUP_ASDFADD	(const CHAR *)".Yasdfadd"
#define	SUP_ASDFDIV	(const CHAR *)".Yasdfdiv"
#define	SUP_ASDIVDC	(const CHAR *)".Yasdivdc"
#define	SUP_ASDIVDF	(const CHAR *)".Yasdivdf"
#define	SUP_ASDIVLC	(const CHAR *)".Yasdivlc"
#define	SUP_ASDIVSC	(const CHAR *)".Yasdivsc"
#define	SUP_ASDFMUL	(const CHAR *)".Yasdfmul"
#define	SUP_ASMULDC	(const CHAR *)".Yasmuldc"
#define	SUP_ASMULDF	(const CHAR *)".Yasmuldf"
#define	SUP_ASMULLC	(const CHAR *)".Yasmullc"
#define	SUP_ASMULSC	(const CHAR *)".Yasmulsc"
#define	SUP_ASMULSF	(const CHAR *)".Yasmulsf"
#define	SUP_ASDFSUB	(const CHAR *)".Yasdfsub"
#define	SUP_ASLFADD	(const CHAR *)".Yaslfadd"
#define	SUP_ASLFDIV	(const CHAR *)".Yaslfdiv"
#define	SUP_ASDIVLF	(const CHAR *)".Yasdivlf"
#define	SUP_ASLFMUL	(const CHAR *)".Yaslfmul"
#define	SUP_ASMULLF	(const CHAR *)".Yaslfmullf"
#define	SUP_ASLFSUB	(const CHAR *)".Yaslfsub"
#define	SUP_ASDCADD	(const CHAR *)".Yasdcadd"
#define	SUP_ASDCDIV	(const CHAR *)".Yasdcdiv"
#define	SUP_ASDCMUL	(const CHAR *)".Yasdcmul"
#define	SUP_ASDCSUB	(const CHAR *)".Yasdcsub"
#define	SUP_ASLCADD	(const CHAR *)".Yaslcadd"
#define	SUP_ASLCDIV	(const CHAR *)".Yaslcdiv"
#define	SUP_ASLCMUL	(const CHAR *)".Yaslcmul"
#define	SUP_ASLCSUB	(const CHAR *)".Yaslcsub"
#define	SUP_ASSCADD	(const CHAR *)".Yasscadd"
#define	SUP_ASSCDIV	(const CHAR *)".Yasscdiv"
#define	SUP_ASSCMUL	(const CHAR *)".Yasscmul"
#define	SUP_ASSCSUB	(const CHAR *)".Yasscsub"
#define	SUP_ASSFSUB	(const CHAR *)".Yassfsub"
#define	SUP_ASSFADD	(const CHAR *)".Yassfadd"
#define	SUP_ASSFDIV	(const CHAR *)".Yassfdiv"
#define	SUP_ASDIVSF	(const CHAR *)".Yasdivsf"
#define	SUP_ASSFMUL	(const CHAR *)".Yassfmul"
#define	SUP_DCADD	(const CHAR *)".Ydcadd"
#define	SUP_DCDIV	(const CHAR *)".Ydcdiv"
#define	SUP_DCINC	(const CHAR *)".Ydcinc"
#define	SUP_DCLTODC	(const CHAR *)".Ydcltodc"
#define	SUP_DCLLTODC	(const CHAR *)".Ydclltodc"
#define	SUP_DCMUL	(const CHAR *)".Ydcmul"
#define	SUP_DCNEG	(const CHAR *)".Ydcneg"
#define	SUP_DCSUB	(const CHAR *)".Ydcsub"
#define	SUP_DCTODF	(const CHAR *)".Ydctodf"
#define	SUP_DCTOL	(const CHAR *)".Ydctol"
#define	SUP_DCTOLC	(const CHAR *)".Ydctolc"
#define	SUP_DCTOLF	(const CHAR *)".Ydctolf"
#define	SUP_DCTOLL	(const CHAR *)".Ydctoll"
#define	SUP_DCTOSC	(const CHAR *)".Ydctosc"
#define	SUP_DCTOSF	(const CHAR *)".Ydctosf"
#define	SUP_DCTOUL	(const CHAR *)".Ydctoul"
#define	SUP_DCTOULL	(const CHAR *)".Ydctoull"
#define	SUP_DCUTODC	(const CHAR *)".Ydcutodc"
#define	SUP_DCULLTODC	(const CHAR *)".Ydculltodc"
#define	SUP_DFADD	(const CHAR *)".Ydfadd"
#define	SUP_DFCMP	(const CHAR *)".Xdfcmp"
#define	SUP_DFDEC	(const CHAR *)".Ydfdec"
#define	SUP_DFDIV	(const CHAR *)".Ydfdiv"
#define	SUP_DFINC	(const CHAR *)".Ydfinc"
#define	SUP_DFLTODF	(const CHAR *)".Ydfltodf"
#define	SUP_DFLLTODF	(const CHAR *)".Ydflltodf"
#define	SUP_DFMUL	(const CHAR *)".Ydfmul"
#define	SUP_DFNEG	(const CHAR *)".Ydfneg"
#define	SUP_DFSUB	(const CHAR *)".Ydfsub"
#define	SUP_DFTOL	(const CHAR *)".Ydftol"
#define	SUP_DFTOLC	(const CHAR *)".Ydftolc"
#define	SUP_DFTOLF	(const CHAR *)".Ydftolf"
#define	SUP_DFTOLL	(const CHAR *)".Ydftoll"
#define	SUP_DFTODC	(const CHAR *)".Ydftodc"
#define	SUP_DFTOSC	(const CHAR *)".Ydftosc"
#define	SUP_DFTOSF	(const CHAR *)".Ydftosf"
#define	SUP_DFTOUL	(const CHAR *)".Ydftoul"
#define	SUP_DFTOULL	(const CHAR *)".Ydftoull"
#define	SUP_DFUTODF	(const CHAR *)".Ydfutodf"
#define	SUP_DFULLTODF	(const CHAR *)".Ydfulltodf"
#define	SUP_LCADD	(const CHAR *)".Ylcadd"
#define	SUP_LCDIV	(const CHAR *)".Ylcdiv"
#define	SUP_LCINC	(const CHAR *)".Ylcinc"
#define	SUP_LCLTOLC	(const CHAR *)".Ylcltolc"
#define	SUP_LCLLTOLC	(const CHAR *)".Ylclltolc"
#define	SUP_LCMUL	(const CHAR *)".Ylcmul"
#define	SUP_LCNEG	(const CHAR *)".Ylcneg"
#define	SUP_LCSUB	(const CHAR *)".Ylcsub"
#define	SUP_LCTODC	(const CHAR *)".Ylctodc"
#define	SUP_LCTODF	(const CHAR *)".Ylctodf"
#define	SUP_LCTOL	(const CHAR *)".Ylctol"
#define	SUP_LCTOLF	(const CHAR *)".Ylctolf"
#define	SUP_LCTOLL	(const CHAR *)".Ylctoll"
#define	SUP_LCTOSC	(const CHAR *)".Ylctosc"
#define	SUP_LCTOSF	(const CHAR *)".Ylctosf"
#define	SUP_LCTOUL	(const CHAR *)".Ylctoul"
#define	SUP_LCTOULL	(const CHAR *)".Ylctoull"
#define	SUP_LCUTOLC	(const CHAR *)".Ylcutolc"
#define	SUP_LCULLTOLC	(const CHAR *)".Ylculltolc"
#define	SUP_LFADD	(const CHAR *)".Ylfadd"
#define	SUP_LFCMP	(const CHAR *)".Xlfcmp"
#define	SUP_LFDEC	(const CHAR *)".Ylfdec"
#define	SUP_LFDIV	(const CHAR *)".Ylfdiv"
#define	SUP_LFINC	(const CHAR *)".Ylfinc"
#define	SUP_LFLLTOLF	(const CHAR *)".Ylflltolf"
#define	SUP_LFMUL	(const CHAR *)".Ylfmul"
#define	SUP_LFNEG	(const CHAR *)".Ylfneg"
#define	SUP_LFSUB	(const CHAR *)".Ylfsub"
#define	SUP_LFLTOLF	(const CHAR *)".Ylfltolf"
#define	SUP_LFTODC	(const CHAR *)".Ylftodc"
#define	SUP_LFTODF	(const CHAR *)".Ylftodf"
#define	SUP_LFTOL	(const CHAR *)".Ylftol"
#define	SUP_LFTOLL	(const CHAR *)".Ylftoll"
#define	SUP_LFTOUL	(const CHAR *)".Ylftoul"
#define	SUP_LFTOULL	(const CHAR *)".Ylftoull"
#define	SUP_LFTOSC	(const CHAR *)".Ylftosc"
#define	SUP_LFTOSF	(const CHAR *)".Ylftosf"
#define	SUP_LFULLTOLF	(const CHAR *)".Ylfulltolf"
#define	SUP_LFUTOLF	(const CHAR *)".Ylfutolf"
#define	SUP_SCADD	(const CHAR *)".Yscadd"
#define	SUP_SCDIV	(const CHAR *)".Yscdiv"
#define	SUP_SCINC	(const CHAR *)".Yscinc"
#define	SUP_SCLTOSC	(const CHAR *)".Yscltosc"
#define	SUP_SCLLTOSC	(const CHAR *)".Ysclltosc"
#define	SUP_SCMUL	(const CHAR *)".Yscmul"
#define	SUP_SCNEG	(const CHAR *)".Yscneg"
#define	SUP_SCSUB	(const CHAR *)".Yscsub"
#define	SUP_SCTODC	(const CHAR *)".Ysctodc"
#define	SUP_SCTODF	(const CHAR *)".Ysctodf"
#define	SUP_SCTOL	(const CHAR *)".Ysctol"
#define	SUP_SCTOLC	(const CHAR *)".Ysctolc"
#define	SUP_SCTOLF	(const CHAR *)".Ysctolf"
#define	SUP_SCTOLL	(const CHAR *)".Ysctoll"
#define	SUP_SCTOSF	(const CHAR *)".Ysctosf"
#define	SUP_SCTOUL	(const CHAR *)".Ysctoul"
#define	SUP_SCTOULL	(const CHAR *)".Ysctoull"
#define	SUP_SCUTOSC	(const CHAR *)".Yscutosc"
#define	SUP_SCULLTOSC	(const CHAR *)".Ysculltosc"
#define	SUP_SFADD	(const CHAR *)".Ysfadd"
#define	SUP_SFCMP	(const CHAR *)".Xsfcmp"
#define	SUP_SFDEC	(const CHAR *)".Ysfdec"
#define	SUP_SFDIV	(const CHAR *)".Ysfdiv"
#define	SUP_SFINC	(const CHAR *)".Ysfinc"
#define	SUP_SFLLTOSF	(const CHAR *)".Ysflltosf"
#define	SUP_SFLTOSF	(const CHAR *)".Ysfltosf"
#define	SUP_SFMUL	(const CHAR *)".Ysfmul"
#define	SUP_SFNEG	(const CHAR *)".Ysfneg"
#define	SUP_SFSUB	(const CHAR *)".Ysfsub"
#define	SUP_SFTODC	(const CHAR *)".Ysftodc"
#define	SUP_SFTODF	(const CHAR *)".Ysftodf"
#define	SUP_SFTOL	(const CHAR *)".Ysftol"
#define	SUP_SFTOLL	(const CHAR *)".Ysftoll"
#define	SUP_SFTOLC	(const CHAR *)".Ysftolc"
#define	SUP_SFTOLF	(const CHAR *)".Ysftodf"
#define	SUP_SFTOSC	(const CHAR *)".Ysftosc"
#define	SUP_SFTOUL	(const CHAR *)".Ysftoul"
#define	SUP_SFTOULL	(const CHAR *)".Ysftoull"
#define	SUP_SFUTOSF	(const CHAR *)".Ysfutosf"
#define	SUP_SFULLTOSF	(const CHAR *)".Ysfulltosf"
#endif /* FLOAT_IEEE */
#ifdef FLOAT_MFFP
#define	SUP_ASFPADD	(const CHAR *)".Xasfpadd"
#define	SUP_ASFPDEC	(const CHAR *)".Xasfpdec"
#define	SUP_ASFPDIV	(const CHAR *)".Xasfpdiv"
#define	SUP_ASFPINC	(const CHAR *)".Xasfpinc"
#define	SUP_ASFPMULT	(const CHAR *)".Xasfpmult"
#define	SUP_ASFPREM	(const CHAR *)".Xasfprem"
#define	SUP_ASFPSUB	(const CHAR *)".Xasfpsub"
#define	SUP_FPADD	(const CHAR *)".Xfpadd"
#define	SUP_FPADD	(const CHAR *)".Xfpadd"
#define	SUP_FPCMP	(const CHAR *)".Xfpcmp"
#define	SUP_FPDEC	(const CHAR *)".Xfpdec"
#define	SUP_FPDIV	(const CHAR *)".Xfpdiv"
#define	SUP_FPFTOL	(const CHAR *)".Xfpftol"
#define	SUP_FPFTOU	(const CHAR *)".Xfpftou"
#define	SUP_FPINC	(const CHAR *)".Xfpinc"
#define	SUP_FPLTOF	(const CHAR *)".Xfpltof"
#define	SUP_FPMULT	(const CHAR *)".Xfpmult"
#define	SUP_FPNEG	(const CHAR *)".Xfpneg"
#define	SUP_FPREM	(const CHAR *)".Xfprem"
#define	SUP_FPSUB	(const CHAR *)".Xfpsub"
#define	SUP_FPSUB	(const CHAR *)".Xfpsub"
#define	SUP_FPTST	(const CHAR *)".Xfptst"
#define	SUP_FPUTOF	(const CHAR *)".Xfputof"
#endif /* FLOAT_IEEE */
#ifdef STACK_CHECK
#define	SUP_STACKCHECK	(const CHAR *)".stackcheck"
#endif /* STACK_CHECK */
#ifdef COLDFIRE
#define	SUP_ASCDIV	(const CHAR *)".Xascdiv"
#define	SUP_ASUCDIV	(const CHAR *)".Xasucdiv"
#define	SUP_ASSDIV	(const CHAR *)".Xassdiv"
#define	SUP_ASUSDIV	(const CHAR *)".Xasusdiv"
#define	SUP_SDIV	(const CHAR *)".Xsdiv"
#define	SUP_USDIV	(const CHAR *)".Xusdiv"
#endif /* COLDFIRE */

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

/* stack optimisations */
#define	OPT_SAFE	0
#define	OPT_MINIMUM	1
#define	OPT_AVERAGE	2
#define	OPT_MAXIMUM	3

#define	REGBIT(x)	(REGMASK)(1U << (int)x)

typedef enum e_am AMODE;	/* Addressing mode */
typedef struct amode ADDRESS;
typedef enum e_op OPCODE;
typedef enum ilength ILEN;
typedef unsigned long REGMASK;
typedef signed char DEEP;
typedef struct reglist REGLIST;

#ifdef PEEPFLOW
typedef unsigned long REGBITMAP;
typedef struct regmap REGMAP;
typedef struct regentry REGENTRY;

struct regmap
{
    REGBITMAP write;		/* destination of an operation */
    REGBITMAP modified;		/* source and destination */
    REGBITMAP read;		/* source of an operation */
    REGBITMAP used;		/* needed for address generation */
    REGBITMAP updated;		/* updated during address generation */
};

/*
 * Used to maintain a list of values & addresses which are held in
 * a register.
 */
struct regentry
{
    ADDRESS *ap;
    REGENTRY *next;
};

#define	NIL_REGENTRY	((REGENTRY *)0)
#endif /* PEEPFLOW */

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
#ifdef PEEPFLOW
    REGMAP *regmap;		/* information for peephole optimizer */
#endif				/* PEEPFLOW */
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
    REGLIST *save;		/* Registers saved by the function */
    REGLIST *result;		/* Registers used to return results */
};

#define BRANCH_COUNT	2	/* abandon branch optimisation if exceeded */

/* peephole optimisations */
#define	PEEP_INSTRUCTION	0
#define	PEEP_JUMPS		1
#define	PEEP_FLOW		2
#define	is_peep_phase(l,x)	(((unsigned) l) & MEMBER(x))

#ifdef MULTIPLE_PROCESSORS
/*
 * remap function names - it is necessary to change the names of functions
 * if there are multiple code generators build into the compiler in order
 * to prevent name clashes.
 *
 * The following defines do the necessary renaming
 */
#define	data_register		data_register68k
#define	address_register	address_register68k
#define	float_register		float_register68k
#define	mdata_register		mdata_register68k
#define	xdata_register		xdata_register68k
#define	build_regmap		build_regmap68k
#define	checkstack		checkstack68k
#define	copy_addr		copy_addr68k
#define	count_registers		count_registers68k
#define	find_label		find_label68k
#define	find_register		find_register68k
#define	flow_dataflow		flow_dataflow68k
#define	flush_peep		flush_peep68k
#define	freeop			freeop68k
#define	g_code			g_code68k
#define	g_fcode			g_fcode68k
#define	g_move			g_move68k
#define	get_register		get_register68k
#define	initstack		initstack68k
#define	is_equal_address	is_equal_address68k
#define	is_free_addr		is_free_addr68k
#define	is_free_data		is_free_data68k
#define	is_register_used	is_register_used68k
#define	mk_label		mk_label68k
#define	mk_reg			mk_reg68k
#define	mk_mreg			mk_mreg68k
#define	mk_xreg			mk_xreg68k
#define	temp_reg		temp_reg68k
#define	temp_inv		temp_inv68k
#define	validate		validate68k
#define	reg_usage		reg_usage68k
#define	regtype			regtype68k
#define	reglist_to_mask		reglist_to_mask68k
#endif /* MULTIPLE_PROCESSORS */

extern REGUSAGE *reg_usage;	/* register usage */
extern REGTYPE regtype[];	/* type of register */
extern int target_option;	/* Motorola code generation target */

#ifdef PEEPFLOW
/* flow68k */
int flow_dataflow P_ ((CODE *));
#endif /* PEEPFLOW */

/* gen68k */
ADDRESS *mk_reg P_ ((REG));
ADDRESS *mk_mreg P_ ((REG, REG));
ADDRESS *mk_xreg P_ ((REG, REG, REG));
ADDRESS *mk_label P_ ((LABEL));
ADDRESS *copy_addr P_ ((const ADDRESS *, AMODE));
BOOL is_short P_ ((const EXPR *));
void g_move P_ ((ILEN, ADDRESS *, ADDRESS *));

/* peep68k */
void g_code P_ ((OPCODE, ILEN, ADDRESS *, ADDRESS *));
void flush_peep P_ ((unsigned));
BOOL is_equal_address P_ ((const ADDRESS *, const ADDRESS *));
BOOL is_register_used P_ ((REG, const ADDRESS *));
CODE   *find_label P_ ((LABEL));

#ifdef PEEPFLOW
REGMAP *build_regmap P_ ((CODE *));
#endif /* PEEPFLOW */

#ifdef FLOAT_IEEE
void g_fcode P_ ((OPCODE, ILEN, ADDRESS *, ADDRESS *));

#endif /* FLOAT_IEEE */

/* reg68k */
ADDRESS *address_register P_ ((void));
ADDRESS *data_register P_ ((void));

#ifdef FLOAT_IEEE
ADDRESS *float_register P_ ((void));

#endif /* FLOAT_IEEE */
ADDRESS *mdata_register P_ ((void));
ADDRESS *xdata_register P_ ((void));
ADDRESS *temp_reg P_ ((FLAGS));
BOOL is_free_addr P_ ((void));
BOOL is_free_data P_ ((void));
REG find_register P_ ((REGTYPE, REGMASK));
REGMASK reglist_to_mask P_ ((const REGLIST *));
int count_registers P_ ((REGMASK));
void checkstack P_ ((void));
void freeop P_ ((const ADDRESS *));
void initstack P_ ((void));
void temp_inv P_ ((REGUSAGE *));
void validate P_ ((const ADDRESS *));

#endif /* _GEN68K_H */
