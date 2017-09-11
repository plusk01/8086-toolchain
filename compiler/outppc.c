/*
 * C compiler
 * ==========
 *
 * This compiler may be redistributed as long there is no
 * commercial interest. The compiler must not be redistributed
 * without its full sources. This notice must stay intact.
 */

/*****************************************************************************/

#include "config.h"

#ifdef POWERPC
#ifdef TARGET_ACK

#define	OUT_MODULE
#include "chdr.h"
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"
#include "genppc.h"
#include "outproto.h"
#include "version.h"
#include <time.h>

/********************************************************** Type Definitions */

enum e_gt
{
    bytegen, wordgen, longgen, longlonggen, stringgen, floatgen, nogen
};
enum e_sg
{
    noseg, codeseg, dataseg, bssseg, romseg
};

/*********************************************** Static Function Definitions */

static void nl P_ ((void));
static void putop P_ ((OPCODE));
static void putconst P_ ((const EXPR *));
static void putlen P_ ((ILEN));
static void putamode P_ ((const ADDRESS *, ILEN));
static void putreg P_ ((REG));
static void put_header P_ ((enum e_gt, SIZE));
static void seg P_ ((enum e_sg, const char *, SIZE));
static void put_bseg P_ ((SIZE));
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
static const char *comment = "!";

static const char *opl[] = {
#ifdef ASM
    "",				/* op_asm */
#endif				/* ASM */
    ".line",			/* op_line */
    0,				/* op_label */
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
static void putconst P1 (const EXPR *, ep)
{
    switch (ep->nodetype) {
    case en_autocon:
    case en_icon:
	oprintf ("%ld", ep->v.i);
	break;
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
	/*FALLTHRU */
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

/*
 * append the length field to an instruction.
 */
static void putlen P1 (ILEN, l)
{
    switch (l) {
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
    case am_line:
    case am_str:
	putconst (ap->u.offset);
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
 * generate a register name from a tempref number.
 */
static void putreg P1 (REG, r)
{
    switch (r) {
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
	".data1\t",		/* bytegen */
	".data2\t",		/* wordgen */
	".data4\t",		/* longgen */
	".data4\t",		/* longlonggen */
	".ascii\t\"",		/* stringgen */
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
    if (val >= (UVAL) 32 && val <= (UVAL) 126 && val != (UVAL) '\\'
	&& val != (UVAL) '"') {
	put_header (stringgen, alignment_of_type (tp_char));
	oprintf ("%c", (int) val);
	outcol++;
    } else {
	put_header (bytegen, alignment_of_type (tp_char));
	oprintf ("0x%lx", val & OxffUL);
	outcol += 4;
    }
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

#ifndef FLOAT_BOOTSTRAP

#ifdef FLOAT_IEEE
/*
 * Generate IEEE single and double numbers
 */

PRIVATE void put_float P1 (const RVAL *, vp)
{
    unsigned long ul;

    ieee_single (vp, &ul);
    put_dword (ul);
}

PRIVATE void put_double P1 (const RVAL *, vp)
{
    unsigned long ul[2];

    ieee_double (vp, ul, TRUE);
    put_dword (ul[0]);
    put_dword (ul[1]);
}

PRIVATE void put_longdouble P1 (const RVAL *, vp)
{
    unsigned long ul[3];

    ieee_longdouble (vp, ul, TRUE);
    put_dword (ul[0]);
    put_dword (ul[1]);
    put_dword (ul[2]);
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

#endif /* FLOAT_BOOTSTRAP */

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
    SIZE    al = alignment_of_type (typeof (sp));

    put_bseg (al);
    if (is_static (sp)) {
	put_label (sp->value.l);
    } else {
	put_name (sp);
    }
    oprintf ("\t.space\t%ld%s", typeof (sp)->size, newline);
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
    for (; strtab != 0; strtab = strtab->next) {
	nl ();
	put_label (strtab->label);
	cp = strtab->str;
	for (len = strtab->len; len--;)
	    put_byte ((UVAL) *cp++);
	put_byte (Ox0UL);

    }
    nl ();
}

PRIVATE void put_reference P1 (SYM *, sp)
{
    if (!is_symbol_output (sp)) {
	switch (storageof (sp)) {
	case sc_global:
	    nl ();
	    oprintf ("\t.extern %s%s", outlate (nameof (sp)), newline);
	    break;
	case sc_external:
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
	if (al > 1l) {
	    oprintf ("\t.align\t%ld%s", al, newline);
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
	if (gentype == stringgen) {
	    oprintf ("\"");
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
    /* introduce the sections */
    seg (codeseg, ".text", (SIZE) 0);
    seg (romseg, ".rom", (SIZE) 0);

    seg (dataseg, ".data", (SIZE) 0);
    seg (bssseg, ".bss", (SIZE) 0);
}

#ifdef MULTIPLE_ASSEMBLERS
struct funcs ackpcc_funcs = {
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
#ifdef FLOAT_SUPPORT
    put_float,
    put_double,
    put_longdouble,
#endif				/* FLOAT_SUPPORT */
};

#endif /* MULTIPLE_ASSEMBLERS */
#endif /* TARGET_ACK */
#endif /* POWERPC */
