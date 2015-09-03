/*
 * C compiler
 * ==========
 *
 * Copyright 1993, David J. Walker.
 * All commercial rights reserved.
 *
 * This source module may be redistributed as long there is no
 * commercial interest. The compiler must not be redistributed
 * without its full sources. This notice must stay intact.
 *
 * History:
 *
 * 1993   First version.  Modelled on the ACK version.
 */

/*****************************************************************************/

#include "config.h"

#ifdef MC680X0
#ifdef TARGET_QMAC

#define OUT_MODULE
#include "chdr.h"
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"
#include "gen68k.h"
#include "outproto.h"
#include "version.h"

/********************************************************** Type Definitions */

enum e_gt
{
    bytegen, wordgen, longgen, longlonggen, stringgen, nogen
};
enum e_sg
{
    noseg, codeseg, dataseg, bssseg, romseg
};

/*********************************************** Static Function Definitions */

static void putop P_ ((OPCODE));
static void putconst P_ ((const EXPR *, ILEN));
static void putlen P_ ((ILEN));
static void putamode P_ ((const ADDRESS *, ILEN));
static void put_mask P_ ((REGMASK));
static void put_rmask P_ ((REGMASK));
static void put_smask P_ ((REGMASK));
static void putreg P_ ((REG));
static void put_header P_ ((enum e_gt, SIZE));
static void seg P_ ((enum e_sg, const char *, SIZE));
static void put_bseg P_ ((SIZE));
static void nl P_ ((void));
static void put_align P_ ((SIZE));

/*********************************************** Global Function Definitions */

PRIVATE void put_name P_ ((SYM *));
PRIVATE void put_dword P_ ((UVAL));
PRIVATE void put_cseg P_ ((SIZE));
PRIVATE void put_dseg P_ ((SIZE));
PRIVATE void put_kseg P_ ((SIZE));
PRIVATE void put_rseg P_ ((SIZE));
PRIVATE void put_label P_ ((LABEL));
PRIVATE void put_reference P_ ((SYM *));
PRIVATE void put_byte P_ ((UVAL));

/********************************************************** Static Variables */

/* variable initialization */

static enum e_gt gentype = nogen;
static enum e_sg curseg = noseg;
static int outcol = 0;
static SIZE align_type = 0L;
static const char *prefix = "I_";
static const char *comment = ";";

static const char *opl[] = {
    "MOVE",			/* op_move */
    "MOVEQ",			/* op_moveq */
    "MOVEA",			/* op_movea */
    "ADD",			/* op_add */
    "ADDI",			/* op_addi */
    "ADDQ",			/* op_addq */
    "ADDA",			/* op_adda */
    "ADDX",			/* op_addx */
    "SUB",			/* op_sub */
    "SUBI",			/* op_subi */
    "SUBQ",			/* op_subq */
    "SUBA",			/* op_suba */
    "SUBX",			/* op_subx */
    "MULS",			/* op_muls */
    "MULU",			/* op_mulu */
    "DIVS",			/* op_divs */
    "DIVU",			/* op_divu */
    "AND",			/* op_and */
    "OR",			/* op_or */
    "EOR",			/* op_eor */
    "ASL",			/* op_asl */
    "LSR",			/* op_lsr */
    "JMP",			/* op_jmp */
    "JSR",			/* op_jsr */
    "BSR",			/* op_bsr */
    "MOVEM",			/* op_movem */
    "RTS",			/* op_rts */
    "RTE",			/* op_rte */
    "BRA",			/* op_bra */
    "BEQ",			/* op_beq */
    "BNE",			/* op_bne */
    "BLT",			/* op_blt */
    "BLE",			/* op_ble */
    "BGT",			/* op_bgt */
    "BGE",			/* op_bge */
    "BHI",			/* op_bhi */
    "BCC",			/* op_bhs */
    "BCS",			/* op_blo */
    "BLS",			/* op_bls */
    "TST",			/* op_tst */
    "EXT",			/* op_ext */
    "EXTB",			/* op_extb */
    "LEA",			/* op_lea */
    "SWAP",			/* op_swap */
    "NEG",			/* op_neg */
    "NEGX",			/* op_negx */ 
    "NOT",			/* op_not */
    "CMP",			/* op_cmp */
    "CMPA",			/* op_cmpa */
    "CLR",			/* op_clr */
    "LINK",			/* op_link */
    "UNLK",			/* op_unlk */
    "PEA",			/* op_pea */
    "CMPI",			/* op_cmpi */
    "DBRA",			/* op_dbra */
    "ASR",			/* op_asr */
    "ROL",			/* op_rol */
    "ROR",			/* op_ror */
    "ROXL",			/* op_roxl */
    "ROXR",			/* op_roxr */
    "SEQ",			/* op_seq */
    "SNE",			/* op_sne */
    "SLT",			/* op_slt */
    "SLE",			/* op_sle */
    "SGT",			/* op_sgt */
    "SGE",			/* op_sge */
    "SHI",			/* op_shi */
    "SCC",			/* op_shs */
    "SCS",			/* op_slo */
    "SLS",			/* op_sls */
    "NOP",			/* op_nop */
    "TRAP",			/* op_trap */
#ifdef FLOAT_IEEE
    "FADD",			/* op_fadd */
    "FSUB",			/* op_fsub */
    "FDIV",			/* op_fdiv */
    "FMUL",			/* op_fmul */
    "FCMP",			/* op_fcmp */
    "FMOVE",			/* op_fmove */
    "FMOVEM",			/* op_fmovem */
#endif				/* FLOAT_IEEE */
#ifdef ASM
    "",				/* op_asm */
#endif				/* ASM */
    "COMMENT",			/* op_line */
    (char *) NULL,		/* op_label */
};

/*****************************************************************************/

static void putop P1 (OPCODE, op)
{
    if (op >= OP_MIN && op <= OP_MAX && opl[op] != (char *) 0) {
	oprintf ("\t%s", opl[op]);
    } else {
	FATAL ((__FILE__, "putop", "illegal opcode %d", op));
    }
}

/*
 * put a constant to the output file.
 */
static void putconst P2 (const EXPR *, ep, ILEN, len)
{
    switch (ep->nodetype) {
    case en_autocon:
    case en_icon:
	if (len == IL8) {
#ifdef LONGLONG
	    oprintf ("%ld", (long) ((UVAL) ep->v.i >> 32));
#else
	    if (is_signed_type (ep->etp) && ep->v.i < 0) {
		oprintf ("-1");
	    } else {
		oprintf ("0");
	    }
	    
#endif	/* LONGLONG */
	} else {
	    oprintf ("%ld", (long) ep->v.i);
	}
	break;
#ifndef FLOAT_BOOTSTRAP
#ifdef FLOAT_MFFP
    case en_fcon:
	oprintf ("$%lX", genffp (ep->v.f));
	break;
#endif /* FLOAT_MFFP */
#endif /* FLOAT_BOOTSTRAP */
    case en_labcon:
	oprintf ("%s%u", prefix, (unsigned) ep->v.l);
	break;
    case en_nacon:
	if (len == IL8) {
	    oprintf ("0");
	} else {
	    oprintf ("%s", outlate (ep->v.str));
	}
	break;
    case en_sym:
	oprintf ("%s", outlate (nameof (ep->v.sp)));
	break;
    case en_add:
	putconst (ep->v.p[0], len);
	oprintf ("+");
	putconst (ep->v.p[1], len);
	break;
    case en_sub:
	putconst (ep->v.p[0], len);
	oprintf ("-");
	putconst (ep->v.p[1], len);
	break;
    case en_uminus:
	oprintf ("-");
	/*lint -fallthrough */
    case en_cast:
	putconst (ep->v.p[0], len);
	break;
    case en_str:
	oprintf ("%s", ep->v.str);
	break;
    default:
	FATAL (
	       (__FILE__, "putconst", "illegal constant node %d",
		ep->nodetype));
	break;
    }
}

/*
 * append the length field to an instruction.
 */
static void putlen P1 (ILEN, l)
{
    switch (l) {
    case IL0:
	break;
    case IL1:
	oprintf (".B");
	break;
    case IL2:
	oprintf (".W");
	break;
    case IL4:
	oprintf (".L");
	break;
    case (ILEN) (IL4 + 1):
	oprintf (".S");
	break;
    case (ILEN) (IL8 + 1):
	oprintf (".D");
	break;
    case (ILEN) (IL12 + 1):
	oprintf (".X");
	break;
    default:
	FATAL ((__FILE__, "putlen", "illegal length field %d", (int) l));
	break;
    }
}

/*
 * output a general addressing mode.
 */
static void putamode P2 (const ADDRESS *, ap, ILEN, len)
{
    IVAL    i_val;

    switch (ap->mode) {
    case am_immed:
	oprintf ("#");
	/*
	 * Suppress overflow in immediate arguments -
	 * which may occur due to optimization of constants
	 */
	if (is_icon (ap->u.offset)) {
	    i_val = ap->u.offset->v.i;
	    switch (len) {
	    case IL1:
		i_val &= (IVAL) 0x000000ffL;
		break;
	    case IL2:
		i_val &= (IVAL) 0x0000ffffL;
		break;
	    default:
		break;
	    }
	    oprintf ("%ld", (long) i_val);
	    break;
	}
	/*lint -fallthrough */
    case am_direct:
	putconst (ap->u.offset, len);
	break;
    case am_areg:
	oprintf ("A%d", (int) ap->preg - (int) A0);
	break;
    case am_dreg:
	oprintf ("D%d", (int) ap->preg);
	break;
    case am_ind:
	oprintf ("(A%d)", (int) ap->preg - (int) A0);
	break;
    case am_ainc:
	oprintf ("(A%d)+", (int) ap->preg - (int) A0);
	break;
    case am_adec:
	oprintf ("-(A%d)", (int) ap->preg - (int) A0);
	break;
    case am_indx:
	/* allow 32-bit offsets */
	putconst (ap->u.offset, IL4);
	oprintf ("(A%d)", (int) ap->preg - (int) A0);
	break;
    case am_indx2:
	/* allow 32-bit offsets */
	putconst (ap->u.offset, IL4);
	oprintf ("(A%d,D%d.%c)", (int) ap->preg - (int) A0, (int) ap->sreg,
		 'L');
	break;
    case am_indx3:
	/* allow 32-bit offsets */
	putconst (ap->u.offset, IL4);
	oprintf ("(A%d,A%d.L)", (int) ap->preg - (int) A0,
		 (int) ap->sreg - (int) A0);
	break;
    case am_indx4:
	/* allow 32-bit offsets */
	putconst (ap->u.offset, IL4);
	oprintf ("(A%d,D%d.%c)", (int) ap->preg - (int) A0, (int) ap->sreg,
		 'W');
	break;
    case am_indxpc:
	putconst (ap->u.offset, IL4);
	oprintf ("(PC)");
	break;
    case am_indx2pc:
	putconst (ap->u.offset, IL4);
	oprintf ("(a%d,PC)", (int) ap->preg - (int) A0);
	break;
    case am_rmask:
	put_rmask (ap->u.mask);
	break;
    case am_smask:
	put_smask (ap->u.mask);
	break;
    case am_freg:
	oprintf ("FP%d", (int) ap->preg - (int) FP0);
	break;
    case am_line:
    case am_str:
	putconst (ap->u.offset, IL4);
	break;
    default:
	FATAL ((__FILE__, "putamode", "illegal address mode %d", ap->mode));
	break;
    }
}

/*
 * output a generic instruction.
 */
PRIVATE void put_code P1 (const CODE *, ip)
{
    putop (ip->opcode);
    putlen (ip->length);
    if (ip->oper1 != NIL_ADDRESS) {
	oprintf ("\t");
	putamode (ip->oper1, ip->length);
	if (ip->oper2 != NIL_ADDRESS) {
	    if (ip->opcode == op_line) {
		oprintf ("%s%s>>>>\t", newline, comment);
	    } else {
		oprintf (",");
	    }
	    putamode (ip->oper2, ip->length);
	}
    }
    oprintf ("%s", newline);
}

/*
 * generate a register mask.
 */
static void put_mask P1 (REGMASK, mask)
{
    REG     reg;
    BOOL    pending = FALSE;

    for (reg = D0; reg <= FP7; reg++) {
	if (mask & (REGMASK) 1) {
	    if (pending) {
		oprintf ("/");
	    }
	    putreg (reg);
	    pending = TRUE;
	}
	mask >>= 1;
    }
}

/*
 * generate a register mask for save.
 */
static void put_smask P1 (REGMASK, mask)
{
    put_mask (mask);
}

/*
 * generate a register mask for restore.
 */
static void put_rmask P1 (REGMASK, mask)
{
    put_mask (mask);
}

/*
 * generate a register name from a tempref number.
 */
static void putreg P1 (REG, r)
{
    switch (r) {
    case D0:
    case D1:
    case D2:
    case D3:
    case D4:
    case D5:
    case D6:
    case D7:
	oprintf ("D%d", (int) r);
	break;
    case A0:
    case A1:
    case A2:
    case A3:
    case A4:
    case A5:
    case A6:
    case A7:
	oprintf ("A%d", (int) r - (int) A0);
	break;
    case FP0:
    case FP1:
    case FP2:
    case FP3:
    case FP4:
    case FP5:
    case FP6:
    case FP7:
	oprintf ("FP%d", (int) r - (int) FP0);
	break;
    default:
	CANNOT_REACH_HERE ();
    }
}

/*
 * generate a named label.
 */
PRIVATE void put_name P1 (SYM *, sp)
{
    put_reference (sp);
    oprintf ("%s:%s", outlate (nameof (sp)), newline);
}

/*
 * output a compiler generated label.
 */
PRIVATE void put_label P1 (LABEL, lab)
{
    oprintf ("%s%u:%s", prefix, (unsigned) lab, newline);
}

static void put_header P2 (enum e_gt, gtype, SIZE, al)
{
    static const char *directive[] = {
	"DC.B\t",		/* bytegen */
	"DC.W\t",		/* wordgen */
	"DC.L\t",		/* longgen */
	"DC.L\t",		/* longlonggen */
	"DC.B\t\'",		/* stringgen */
    };

    if (gentype != gtype || outcol >= MAX_WIDTH) {
	put_align (al);
	gentype = gtype;
	outcol = 15;
	oprintf ("\t%s", directive[gtype]);
    } else if (gentype != stringgen) {
	oprintf (",");
    }
}

PRIVATE void put_byte P1 (UVAL, val)
{
    if ((val >= (UVAL) 32) &&
	(val <= (UVAL) 126) &&
	(val != '\\') &&
	(val != (UVAL) '\'') &&
	(val != '[') &&
	(val != ']')) {
	put_header (stringgen, alignment_of_type (tp_char));
	oprintf ("%c", (int) val);
	outcol++;
    } else {
	put_header (bytegen, alignment_of_type (tp_char));
	oprintf ("$%lX", (unsigned long) (val & OxffUL));
	outcol += 4;
    }
}

PRIVATE void put_word P1 (UVAL, val)
{
    put_header (wordgen, alignment_of_type (tp_short));
    oprintf ("$%lX", (unsigned long) (val & OxffffUL));
    outcol += 6;
}

PRIVATE void put_dword P1 (UVAL, val)
{
    put_header (longgen, alignment_of_type (tp_long));
    oprintf ("$%lX", (unsigned long) val);
    outcol += 10;
}

#ifndef FLOAT_BOOTSTRAP
#ifdef FLOAT_IEEE
/*
 * Generate IEEE single and double numbers
 */
PRIVATE void put_float P1 (const RVAL *, vp)
{
    unsigned long ul;

    ieee_single (vp, &ul);
    put_dword ((UVAL) ul);
}

PRIVATE void put_double P1 (const RVAL *, vp)
{
    unsigned long ul[2];

    ieee_double (vp, ul, TRUE);
    put_dword ((UVAL) ul[0]);
    put_dword ((UVAL) ul[1]);
}

PRIVATE void put_longdouble P1 (const RVAL *, vp)
{
    unsigned long ul[3];

    ieee_longdouble (vp, ul, TRUE);
    put_dword ((UVAL) ul[0]);
    put_dword ((UVAL) ul[1]);
    put_dword ((UVAL) ul[2]);
}

#endif /* FLOAT_IEEE */
#ifdef FLOAT_MFFP
/*
 * Generate MOTOROLA FFP numbers
 */
PRIVATE void put_float P1 (const RVAL *, vp)
{
    put_dword (genffp (vp));
}

PRIVATE void put_double P1 (const RVAL *, vp)
{
    put_dword (genffp (vp));
}

PRIVATE void put_longdouble P1 (const RVAL *, vp)
{
    put_dword (genffp (vp));
}

#endif /* FLOAT_MFFP */
#endif /* FLOAT_BOOTSTRAP */

#ifndef RELOC_BUG
PRIVATE void put_char P1 (const EXPR *, ep)
{
    put_header (bytegen, alignment_of_type (tp_char));
    putconst (ep, IL1);
    outcol += 10;
}
PRIVATE void put_short P1 (const EXPR *, ep)
{
    put_header (wordgen, alignment_of_type (tp_short));
    putconst (ep, IL2);
    outcol += 10;
}

#endif /* RELOC_BUG */

PRIVATE void put_long P1 (const EXPR *, ep)
{
    put_header (longgen, alignment_of_type (tp_long));
    putconst (ep, IL4);
    outcol += 10;
}

PRIVATE void put_longlong P1 (const EXPR *, ep)
{
    put_header (longlonggen, alignment_of_type (tp_longlong));
    putconst (ep, IL8);
    oprintf (", ");
    putconst (ep, IL4);
    outcol += 10;
}

PRIVATE void put_pointer P1 (const EXPR *, ep)
{
    put_header (longgen, alignment_of_type (tp_pointer));
    putconst (ep, IL4);
    outcol += 10;
}

PRIVATE void put_storage P1 (SYM *, sp)
{
    SIZE    al = alignment_of_type (typeof (sp));

    put_bseg (al);
    if (is_static (sp)) {
	put_label (sp->value.l);
    } else {
	put_name (sp);
    }
    oprintf ("\tDS.B\t%ld%s", typeof (sp)->size, newline);
}


/*
 * dump the string literal pool.
 * if we are producing single copies of strings (which should therefore
 * be read only we put them in the text segment - else in the data segment.
 */
PRIVATE void put_literals P0 (void)
{
    const CHAR *cp;
    size_t  len;

    if (lang_option == LANG_KANDR) {
	put_dseg (alignment_of_type (tp_char));
    } else {
	put_kseg (alignment_of_type (tp_char));
    }
    for (; strtab != NIL_STRING; strtab = strtab->next) {
	nl ();
	put_label (strtab->label);
	cp = strtab->str;
	for (len = strtab->len; len--;)
	    put_byte ((UVAL) *cp++);
	put_byte ((UVAL) 0);
    }
    nl ();
}

PRIVATE void put_reference P1 (SYM *, sp)
{
    if (!is_symbol_output (sp)) {
	switch (storageof (sp)) {
	case sc_global:
	    nl ();
	    oprintf ("\tXDEF %s%s", outlate (nameof (sp)), newline);
	    break;
	case sc_external:
	    nl ();
	    oprintf ("\tXREF %s%s", outlate (nameof (sp)), newline);
	    break;
	default:
	    break;
	}
	symbol_output (sp);
    }
}

/* align the following data */
static void put_align P1 (SIZE, al)
{
    nl ();
    if (al > align_type) {
	switch (al) {
	case 2L:
	    oprintf ("\tDS.W\t0%s", newline);
	    break;
	case 4L:
	    oprintf ("\tDS.L\t0%s", newline);
	    break;
	default:
	    break;
	}
    }
    align_type = al;
}

/*
 * output any function epilogue code
 */
PRIVATE void put_epilogue P2 (SYM *, sp, LABEL, label)
{
    sp = sp;			/* keep the compiler quiet */
    label = label;		/* keep the compiler quiet */
}

PRIVATE void nl P0 (void)
{
    if (outcol > 0) {
	if (gentype == stringgen) {
	    oprintf ("\'");
	}
	oprintf ("%s", newline);
	gentype = nogen;
	outcol = 0;
    }
}

static void seg P3 (enum e_sg, segtype, const char *, segname, SIZE, al)
{
    nl ();
    if (curseg != segtype) {
	oprintf ("\tSECTION\t%s%s", segname, newline);
	curseg = segtype;
	align_type = 0L;
    }
    put_align (2L);	/* force to even boundary */
    put_align (al);
}

PRIVATE void put_cseg P1 (SIZE, al)
{
    seg (codeseg, "TEXT", al);
}

PRIVATE void put_dseg P1 (SIZE, al)
{
    seg (dataseg, "DATA", al);
}

static void put_bseg P1 (SIZE, al)
{
    seg (bssseg, "BSS", al);
}

PRIVATE void put_kseg P1 (SIZE, al)
{
    if (IandD_option) {
	put_dseg (al);
    } else {
	put_cseg (al);
    }
}

PRIVATE void put_rseg P1 (SIZE, al)
{
    put_cseg (al);
}

PRIVATE void put_finish P0 (void)
{
}

PRIVATE void put_start P0 (void)
{
    oprintf ("%s Generated by %s %s %s (%s) from \"%s\"%s",
	     comment, PROGNAME, VERSION, LAST_CHANGE_DATE, __DATE__, in_file,
	     newline);
#ifdef VERBOSE
    {
	time_t  time_of_day;
	VOIDCAST time (&time_of_day);

	oprintf ("%s Compilation date/time: %s%s",
		 comment, ctime (&time_of_day), newline);
    }
#endif /* VERBOSE */
    /* introduce the sections */
    seg (codeseg, "TEXT", (SIZE) 0);
    seg (romseg, "ROM", (SIZE) 0);
    seg (dataseg, "DATA", (SIZE) 0);
    seg (bssseg, "BSS", (SIZE) 0);
}

#ifdef MULTIPLE_ASSEMBLERS
struct funcs qmac68k_funcs = {
    put_code,
    put_name,
    put_label,
    put_byte,
    put_word,
    put_dword,
#ifndef RELOC_BUG
    put_char,
    put_short,
#endif				/* RELOC_BUG */
    put_long,
    put_longlong,
    put_pointer,
    put_storage,
    put_literals,
    put_finish,
    put_start,
    put_reference,
    put_epilogue,
    put_cseg,
    put_dseg,
    put_kseg,
    put_rseg, 
#ifndef FLOAT_BOOTSTRAP
#ifdef FLOAT_SUPPORT
	put_float,
    put_double,
    put_longdouble,
#endif	/* FLOAT_SUPPORT */
#endif	/* FLOAT_BOOTSTRAP */
    NULL
};

#endif /* MULTIPLE_ASSEMBLERS */
#endif /* TARGET_QMAC */
#endif /* MC680X0 */
