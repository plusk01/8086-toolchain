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

#ifdef ARM
#ifdef TARGET_OBJ

#define	OUT_MODULE
#include "chdr.h"
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"
#include "genarm.h"
#include "outproto.h"
#include "version.h"

/********************************************************** Type Definitions */

enum e_gt
{
    bytegen, wordgen, longgen, longlonggen, stringgen, nogen
};
enum e_sg
{
    noseg, codeseg, dataseg, bssseg
};

/*********************************************** Static Function Definitions */

static void putop P_ ((OPCODE, CONDITION));
static void putconst P_ ((const EXPR *));
static void putamode P_ ((const ADDRESS *));
static void put_header P_ ((enum e_gt, SIZE));
static void put_align P_ ((SIZE));
static void nl P_ ((void));
static void seg P_ ((enum e_sg, const char *, SIZE));
static void put_bseg P_ ((SIZE));

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

static const char *regname[] = {
    "a1",			/* 0 */
    "a2",			/* 1 */
    "a3",			/* 2 */
    "a4",			/* 3 */
    "v1",			/* 4 */
    "v2",			/* 5 */
    "v3",			/* 6 */
    "v4",			/* 7 */
    "v5",			/* 8 */
    "v6",			/* 9 */
    "sl",			/* 10 */
    "fp",			/* 11 */
    "ip",			/* 12 */
    "sp",			/* 13 */
    "lr",			/* 14 */
    "pc"			/* 15 */
};

/*****************************************************************************/

static void putop P2 (OPCODE, op, CONDITION, cc)
{
    static const char *opl[] = {
	"ADC%s",		/* op_adc  */
	"ADD%s",		/* op_add  */
	"ADC%sS",		/* op_adcs */
	"ADD%sS",		/* op_adds */
	"SBC%s",		/* op_sbc  */
	"SUB%s",		/* op_sub  */
	"SBC%sS",		/* op_sbcs */
	"SUB%sS",		/* op_subs */
	"RSB%s",		/* op_rsb  */
	"RSB%sS",		/* op_rsbs */
	"AND%s",		/* op_and  */
	"AND%sS",		/* op_ands */
	"BIC%s",		/* op_bic  */
	"BIC%sS",		/* op_bics */
	"ORR%s",		/* op_orr  */
	"ORR%sS",		/* op_orrs */
	"EOR%s",		/* op_eor  */
	"EOR%sS",		/* op_eors */
	"MOV%s",		/* op_mov  */
	"MOV%sS",		/* op_movs */
	"MVN%s",		/* op_mvn  */
	"MVN%sS",		/* op_mvns */
	"CMN%sS",		/* op_cmns */
	"CMP%sS",		/* op_cmps */
	"TEQ%sS",		/* op_teqs */
	"TST%sS",		/* op_tsts */
	"MUL%s",		/* op_mul  */
	"MLA%s",		/* op_mla  */
	"B%s",			/* op_b    */
	"BL%s",			/* op_bl   */
	"LDR%s",		/* op_ldr  */
	"STR%s",		/* op_str  */
	"LDM%sEA",		/* op_ldmea */
	"STM%sFD",		/* op_stmfd */
	"SWI%s",		/* op_swi  */
	(char *) NULL,		/* op_label */
#ifdef ASM
	"",			/* op_asm */
#endif				/* ASM */
	(char *) NULL,
    };
    static const char *ccodes[] = {
	"",			/* cc_al */
	"CC",			/* cc_cc */
	"CS",			/* cc_cs */
	"EQ",			/* cc_eq */
	"GE",			/* cc_ge */
	"GT",			/* cc_gt */
	"HI",			/* cc_hi */
	"LE",			/* cc_le */
	"LS",			/* cc_ls */
	"LT",			/* cc_lt */
	"MI",			/* cc_mi */
	"NE",			/* cc_ne */
	"NV",			/* cc_nv */
	"PL",			/* cc_pl */
	"VC",			/* cc_vc */
	"VS"			/* cc_vs */
    };

    if (op >= OP_MIN && op <= OP_MAX && opl[op] != (char *) 0) {
	oprintf ("\t");
	oprintf (opl[op], ccodes[cc]);
    } else {
	FATAL ((__FILE__, "putop", "illegal opcode %d", op));
    }
}

/*
 * put a constant to the output file.
 */
static void putconst P1 (const EXPR *, ep)
{


    switch (ep->nodetype) {
    case en_autocon:
    case en_icon:
	oprintf ("%ld", ep->v.i);
	break;
#ifdef FLOAT_MFFP
    case en_fcon:
	oprintf ("0x%lx", genffp (ep->v.f));
	break;
#endif /* FLOAT_MFFP */
    case en_labcon:
	oprintf ("%s%u", prefix, (unsigned) ep->v.l);
	break;
    case en_nacon:
	oprintf ("%s", outlate (ep->v.str));
	break;
    case en_sym:
	oprintf ("%s", outlate (nameof (ep->v.sp)));
	break;
    case en_add:
	putconst (ep->v.p[0]);
	oprintf ("+");
	putconst (ep->v.p[1]);
	break;
    case en_sub:
	putconst (ep->v.p[0]);
	oprintf ("-");
	putconst (ep->v.p[1]);
	break;
    case en_uminus:
	oprintf ("-");
	/*lint -fallthrough */
    case en_cast:
	putconst (ep->v.p[0]);
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

static void putreg P1 (REG, reg)
{
    oprintf ("%s", regname[reg]);
}

static void put_mask P1 (REGMASK, mask)
{
    REG     reg;
    BOOL    pending = FALSE;

    oprintf ("{");
    if (mask & (REGMASK) 1) {
	putreg (R0);
	pending = TRUE;
    }
    for (reg = R1; reg <= R15; reg++) {
	if ((mask = mask >> 1) & (REGMASK) 1) {
	    if (pending) {
		oprintf (",");
	    }
	    putreg (reg);
	    pending = TRUE;
	}
    }
    oprintf ("}");
}

/*
 * output a general addressing mode.
 */
static void putamode P1 (const ADDRESS *, ap)
{
    IVAL    i_val;

    switch (ap->mode) {
    case am_immed:
	oprintf ("#");
	/*
	 * Suppress overflow in immediate arguments -
	 * which may occur due to optimization of constants
	 */
	if (is_icon (ap->offset)) {
	    i_val = ap->offset->v.i;
	    oprintf ("%ld", i_val);
	    break;
	}
	putconst (ap->offset);
	break;
    case am_direct:
	putconst (ap->offset);
	break;
    case am_reg:
	oprintf ("%s", regname[ap->preg]);
	break;
    case am_lsl:
	oprintf ("%s, LSL #", regname[ap->preg]);
	putconst (ap->offset);
	break;
    case am_lsr:
	oprintf ("%s, LSR #", regname[ap->preg]);
	putconst (ap->offset);
	break;
    case am_asr:
	oprintf ("%s, ASR #", regname[ap->preg]);
	putconst (ap->offset);
	break;
    case am_ror:
	oprintf ("%s, ROR #", regname[ap->preg]);
	putconst (ap->offset);
	break;
    case am_lslr:
	oprintf ("%s, LSL %s", regname[ap->preg], regname[ap->sreg]);
	break;
    case am_lsrr:
	oprintf ("%s, LSR %s", regname[ap->preg], regname[ap->sreg]);
	break;
    case am_asrr:
	oprintf ("%s, ASR %s", regname[ap->preg], regname[ap->sreg]);
	break;
    case am_rorr:
	oprintf ("%s, ROR %s", regname[ap->preg], regname[ap->sreg]);
	break;
    case am_pre:
	oprintf ("[%s,#", regname[ap->preg]);
	putconst (ap->offset);
	oprintf ("]");
	break;
    case am_prelsl:
	oprintf ("[%s, %s, LSL #", regname[ap->preg], regname[ap->sreg]);
	putconst (ap->offset);
	oprintf ("]");
	break;
    case am_prelsr:
	oprintf ("[%s, %s, LSR #", regname[ap->preg], regname[ap->sreg]);
	putconst (ap->offset);
	oprintf ("]");
	break;
    case am_preasr:
	oprintf ("[%s, %s, ASR #", regname[ap->preg], regname[ap->sreg]);
	putconst (ap->offset);
	oprintf ("]");
	break;
    case am_preror:
	oprintf ("[%s, %s, ROR #", regname[ap->preg], regname[ap->sreg]);
	putconst (ap->offset);
	oprintf ("]");
	break;
    case am_post:
	oprintf ("[%s], #", regname[ap->preg]);
	putconst (ap->offset);
	break;
    case am_postlsl:
	oprintf ("[%s], %s, LSL #", regname[ap->preg], regname[ap->sreg]);
	putconst (ap->offset);
	break;
    case am_postlsr:
	oprintf ("[%s], %s, LSR #", regname[ap->preg], regname[ap->sreg]);
	putconst (ap->offset);
	break;
    case am_postasr:
	oprintf ("[%s], %s, ASR #", regname[ap->preg], regname[ap->sreg]);
	putconst (ap->offset);
	break;
    case am_postror:
	oprintf ("[%s], %s, ROR #", regname[ap->preg], regname[ap->sreg]);
	putconst (ap->offset);
	break;
    case am_mask:
	put_mask ((REGMASK) ap->offset->v.i);
	break;
    case am_line:
    case am_str:
	putconst (ap->offset);
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
    putop (ip->opcode, ip->cc);
    if (ip->oper1 != NIL_ADDRESS) {
	oprintf ("\t");
	putamode (ip->oper1);
	if (ip->oper2 != NIL_ADDRESS) {
	    oprintf (", ");
	    putamode (ip->oper2);
	    if (ip->oper3 != NIL_ADDRESS) {
		oprintf (", ");
		putamode (ip->oper3);
	    }
	}
    }
    oprintf ("%s", newline);
}

/*
 * generate a named label.
 */
PRIVATE void put_name P1 (SYM *, sp)
{
    put_reference (sp);
    oprintf ("%s%s", outlate (nameof (sp)), newline);
}

/*
 * output a compiler generated label.
 */
PRIVATE void put_label P1 (LABEL, lab)
{
    oprintf ("%s%u%s", prefix, (unsigned int) lab, newline);
}

static void put_header P2 (enum e_gt, gtype, SIZE, al)
{
    static const char *directive[] = {
	"DCB\t",		/* bytegen */
	"DCW2\t",		/* wordgen */
	"DCD\t",		/* longgen */
	"DCD\t",		/* longlonggen */
	"DCB\t\"",		/* stringgen */
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
    put_header (bytegen, alignment_of_type (tp_char));
    oprintf ("0x%lx", val & OxffUL);
    outcol += 4;
}


PRIVATE void put_word P1 (UVAL, val)
{
    put_header (wordgen, alignment_of_type (tp_short));
    oprintf ("0x%lx", val & OxffffUL);
    outcol += 6;
}

PRIVATE void put_dword P1 (UVAL, val)
{
    put_header (longgen, alignment_of_type (tp_long));
    oprintf ("0x%lx", val);
    outcol += 10;
}

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

PRIVATE void put_char P1 (const EXPR *, ep)
{
    put_header (bytegen, alignment_of_type (tp_char));
    putconst (ep);
    outcol += 10;
}

PRIVATE void put_short P1 (const EXPR *, ep)
{
    put_header (wordgen, alignment_of_type (tp_short));
    putconst (ep);
    outcol += 10;
}

PRIVATE void put_long P1 (const EXPR *, ep)
{
    put_header (longgen, alignment_of_type (tp_long));
    putconst (ep);
    outcol += 10;
}

PRIVATE void put_longlong P1 (const EXPR *, ep)
{
    put_header (longlonggen, alignment_of_type (tp_longlong));
    putconst (ep);
    outcol += 10;
}

PRIVATE void put_pointer P1 (const EXPR *, ep)
{
    put_header (longgen, alignment_of_type (tp_pointer));
    putconst (ep);
    outcol += 10;
}

PRIVATE void put_storage P1 (SYM *, sp)
{
    put_bseg (alignment_of_type (typeof (sp)));
    if (is_static (sp)) {
	put_label (sp->value.l);
    } else {
	put_name (sp);
    }
    oprintf ("\t%%\t%ld%s", typeof (sp)->size, newline);
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
	    oprintf ("\tEXPORT\t|%s|%s", nameof (sp), newline);
	    break;
	case sc_external:
	    oprintf ("\tIMPORT\t|%s|%s", nameof (sp), newline);
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
	case 1:
	    break;
	case 2:
	    oprintf ("\tALIGN\t1%s", newline);
	    break;
	case 4:
	    oprintf ("\tALIGN\t2%s", newline);
	    break;
	default:
	    FATAL ((__FILE__, "put_align", "align %ld", al));
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

static void nl P0 (void)
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
	oprintf ("%s, %ld%s", segname, al, newline);
	curseg = segtype;
	align_type = al;
    } else {
	put_align (al);
    }
}

PRIVATE void put_cseg P1 (SIZE, al)
{
    seg (codeseg, "\tAREA |C$$code|, CODE, READONLY", al);
}

PRIVATE void put_dseg P1 (SIZE, al)
{
    seg (dataseg, "\tAREA |C$$data|, DATA, NOINIT", al);
}

static void put_bseg P1 (SIZE, al)
{
    seg (bssseg, "\tAREA |C$$bss|, DATA", al);
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
    nl ();
    oprintf ("\tEND%s", newline);
}

PRIVATE void put_start P0 (void)
{
    REG     reg;

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
    for (reg = R0; reg <= R15; reg++)
	oprintf ("%s RN %d%s", regname[reg], (int) reg, newline);
}

#ifdef MULTIPLE_ASSEMBLERS
struct funcs armobj_funcs = {
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
#endif /* TARGET_OBJ */
#endif /* ARM */
