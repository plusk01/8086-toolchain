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

/*****************************************************************************/

#include "config.h"

#ifdef MC680X0
#ifdef TARGET_CPM

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
    bytegen, wordgen, longgen, longlonggen, nogen
};
enum e_sg
{
    noseg, codeseg, dataseg, bssseg
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
static const char *prefix = "L";
static const char *comment = "*";

static const char *opl[] = {
    "move",			/* op_move */
    "moveq",			/* op_moveq */
    "move",			/* op_movea */
    "add",			/* op_add */
    "add",			/* op_addi */
    "addq",			/* op_addq */
    "add",			/* op_adda */
    "addx",			/* op_addx */
    "sub",			/* op_sub */
    "sub",			/* op_subi */
    "subq",			/* op_subq */
    "sub",			/* op_suba */
    "subx",			/* op_subx */
    "muls",			/* op_muls */
    "mulu",			/* op_mulu */
    "divs",			/* op_divs */
    "divu",			/* op_divu */
    "and",			/* op_and */
    "or",			/* op_or */
    "eor",			/* op_eor */
    "asl",			/* op_asl */
    "lsr",			/* op_lsr */
    "jmp",			/* op_jmp */
    "jsr",			/* op_jsr */
    "bsr",			/* op_bsr */
    "movem",			/* op_movem */
    "rts",			/* op_rts */
    "rte",			/* op_rte */
    "bra",			/* op_bra */
    "beq",			/* op_beq */
    "bne",			/* op_bne */
    "blt",			/* op_blt */
    "ble",			/* op_ble */
    "bgt",			/* op_bgt */
    "bge",			/* op_bge */
    "bhi",			/* op_bhi */
    "bcc",			/* op_bhs */
    "bcs",			/* op_blo */
    "bls",			/* op_bls */
    "tst",			/* op_tst */
    "ext",			/* op_ext */
    "extb",			/* op_extb */
    "lea",			/* op_lea */
    "swap",			/* op_swap */
    "neg",			/* op_neg */
    "negx", /* op_negx */ 
	"not",			/* op_not */
    "cmp",			/* op_cmp */
    "cmp",			/* op_cmpa */
    "clr",			/* op_clr */
    "link",			/* op_link */
    "unlk",			/* op_unlk */
    "pea",			/* op_pea */
    "cmp",			/* op_cmpi */
    "dbra",			/* op_dbra */
    "asr",			/* op_asr */
    "rol",			/* op_rol */
    "ror",			/* op_ror */
    "roxl",			/* op_roxl */
    "roxr",			/* op_roxr */
    "seq",			/* op_seq */
    "sne",			/* op_sne */
    "slt",			/* op_slt */
    "sle",			/* op_sle */
    "sgt",			/* op_sgt */
    "sge",			/* op_sge */
    "shi",			/* op_shi */
    "scc",			/* op_shs */
    "scs",			/* op_slo */
    "sls",			/* op_sls */
    "nop",			/* op_nop */
    "trap",			/* op_trap */
#ifdef FLOAT_IEEE
    "fadd",			/* op_fadd */
    "fsub",			/* op_fsub */
    "fdiv",			/* op_fdiv */
    "fmul",			/* op_fmul */
    "fcmp",			/* op_fcmp */
    "fmove",			/* op_fmove */
    "fmovem",			/* op_fmovem */
#endif				/* FLOAT_IEEE */
#ifdef ASM
    "",				/* op_asm */
#endif				/* ASM */
    "*.line",			/* op_line */
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
	    oprintf ("%ld,", (long) ((UVAL) ep->v.i >> 32));
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
	oprintf ("$%lx", genffp (ep->v.f));
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
	oprintf (".b");
	break;
    case IL2:
	oprintf (".w");
	break;
    case IL4:
	oprintf (".l");
	break;
    case (ILEN) (IL4 + 1):
	oprintf (".s");
	break;
    case (ILEN) (IL8 + 1):
	oprintf (".d");
	break;
    case (ILEN) (IL12 + 1):
	oprintf (".x");
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
    switch (ap->mode) {
    case am_immed:
	oprintf ("#");
	/*lint -fallthrough */
    case am_direct:
	putconst (ap->u.offset, len);
	break;
    case am_areg:
	oprintf ("a%d", (int) ap->preg - (int) A0);
	break;
    case am_dreg:
	oprintf ("d%d", (int) ap->preg);
	break;
    case am_ind:
	oprintf ("(a%d)", (int) ap->preg - (int) A0);
	break;
    case am_ainc:
	oprintf ("(a%d)+", (int) ap->preg - (int) A0);
	break;
    case am_adec:
	oprintf ("-(a%d)", (int) ap->preg - (int) A0);
	break;
    case am_indx:
	putconst (ap->u.offset, len);
	oprintf ("(a%d)", (int) ap->preg - (int) A0);
	break;
    case am_indx2:
	putconst (ap->u.offset, len);
	oprintf ("(a%d,d%d.%c)", (int) ap->preg - (int) A0, (int) ap->sreg,
		 'l');
	break;
    case am_indx3:
	putconst (ap->u.offset, len);
	oprintf ("(a%d,a%d.l)", (int) ap->preg - (int) A0,
		 (int) ap->sreg - (int) A0);
	break;
    case am_indx4:
	putconst (ap->u.offset, len);
	oprintf ("(a%d,d%d.%c)", (int) ap->preg - (int) A0, (int) ap->sreg,
		 'w');
	break;
    case am_indxpc:
	putconst (ap->u.offset, len);
	oprintf ("(pc)");
	break;
    case am_indx2pc:
	putconst (ap->u.offset, len);
	oprintf ("(a%d,pc)", (int) ap->preg - (int) A0);
	break;
    case am_rmask:
	put_rmask (ap->u.mask);
	break;
    case am_smask:
	put_smask (ap->u.mask);
	break;
    case am_freg:
	oprintf ("fp%d", (int) ap->preg - (int) FP0);
	break;
    case am_line:
    case am_str:
	putconst (ap->u.offset, len);
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
	oprintf ("d%d", (int) r);
	break;
    case A0:
    case A1:
    case A2:
    case A3:
    case A4:
    case A5:
    case A6:
    case A7:
	oprintf ("a%d", (int) r - A0);
	break;
    case FP0:
    case FP1:
    case FP2:
    case FP3:
    case FP4:
    case FP5:
    case FP6:
    case FP7:
	oprintf ("fp%d", (int) r - FP0);
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
    oprintf ("%s%u:%s", prefix, (unsigned int) lab, newline);
}

static void put_header P2 (enum e_gt, gtype, SIZE, al)
{
    static const char *directive[] = {
	"dc.b\t",		/* bytegen */
	"dc.w\t",		/* wordgen */
	"dc.l\t",		/* longgen */
	"dc.l\t",		/* longlonggen */
    };

    if (gentype != gtype || outcol >= MAX_WIDTH) {
	put_align (al);
	gentype = gtype;
	outcol = 15;
	oprintf ("\t%s", directive[gtype]);
    } else {
	oprintf (",");
    }
}

PRIVATE void put_byte P1 (UVAL, val)
{
    put_header (bytegen, alignment_of_type (tp_char));
    oprintf ("0x%lx", (unsigned long) (val & OxffUL));
    outcol += 4;
}

PRIVATE void put_word P1 (UVAL, val)
{
    put_header (wordgen, alignment_of_type (tp_short));
    oprintf ("0x%lx", (unsigned long) (val & OxffffUL));
    outcol += 6;
}

PRIVATE void put_dword P1 (UVAL, val)
{
    put_header (longgen, alignment_of_type (tp_long));
    oprintf ("$%lx", (unsigned long) val);
    outcol += 10;
}

#ifndef FLOAT_BOOTSTRAP
#ifdef FLOAT_IEEE
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
PRIVATE void put_float P1 (const RVAL *, vp)
{
    put_dword (genffp (vp));
}

PRIVATE void put_double P1 (const RVAL *, vp)
{
    put_dword (genffp (vp));
}

PRIVATE void put_longdouble P1 (const RVAL *, val)
{
    put_dword (genffp (val));
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
    oprintf ("\tds.b\t%ld%s", typeof (sp)->size, newline);
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

    if (trad_option) {
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

/* put the definition of a global name in the output file */
PRIVATE void put_reference P1 (SYM *, sp)
{
    if (!is_symbol_output (sp)) {
	switch (storageof (sp)) {
	case sc_global:
	    nl ();
	    oprintf ("\t.globl %s%s", outlate (nameof (sp)), newline);
	    break;
	case sc_external:
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
	case 1L:
	    break;
	case 2L:
	case 4L:
	    oprintf ("\t.even%s", newline);
	    break;
	default:
	    CANNOT_REACH_HERE ();
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
	oprintf ("%s", newline);
	gentype = nogen;
	outcol = 0;
    }
}

static void seg P3 (enum e_sg, segtype, const char *, segname, SIZE, al)
{
    nl ();
    if (curseg != segtype) {
	oprintf ("\t.sect\t%s%s", segname, newline);
	curseg = segtype;
	align_type = 0L;
    }
    put_align (al);
}

PRIVATE void put_cseg P1 (SIZE, al)
{
    seg (codeseg, ".text", al);
}

PRIVATE void put_dseg P1 (SIZE, al)
{
    seg (dataseg, ".data", al);
}

static void put_bseg P1 (SIZE, al)
{
    seg (bssseg, ".bss", al);
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
}

#ifdef MULTIPLE_ASSEMBLERS
struct funcs cpm68k_funcs = {
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
#endif /* TARGET_CPM */
#endif /* MC680X0 */
