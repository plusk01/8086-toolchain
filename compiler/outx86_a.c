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

#ifdef INTEL
#ifdef TARGET_MASM

#define OUT_MODULE
#include "chdr.h"
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"
#include "genx86.h"
#include "outproto.h"
#include "version.h"

/********************************************************* Macro Definitions */

#define	BI	1U
#define SPN	2U
#define FP	4U

/********************************************************** Type Definitions */

enum e_gt
{
    bytegen, wordgen, longgen, longlonggen, nogen
};
enum e_sg
{
    noseg, codeseg, dataseg
};

/*********************************************** Static Function Definitions */

static unsigned int putop P_ ((OPCODE));
static void putconst P_ ((const EXPR *));
static void putamode P_ ((const ADDRESS *, ILEN, unsigned int));
static void put_header P_ ((enum e_gt, SIZE));
static void seg P_ ((enum e_sg, const char *, SIZE));
static void put_bseg P_ ((SIZE));
static void put_noseg P_ ((void));
static void nl P_ ((void));
static void put_align P_ ((SIZE));

/*********************************************** Global Function Definitions */

PRIVATE void put_label P_ ((LABEL));
PRIVATE void put_byte P_ ((UVAL));
PRIVATE void put_dword P_ ((UVAL));
PRIVATE void put_cseg P_ ((SIZE));
PRIVATE void put_dseg P_ ((SIZE));
PRIVATE void put_kseg P_ ((SIZE));
PRIVATE void put_rseg P_ ((SIZE));
PRIVATE void put_finish P_ ((void));
PRIVATE void put_start P_ ((void));
PRIVATE void put_reference P_ ((SYM *));

/********************************************************** Static Variables */

/* variable initialization */

static enum e_gt gentype = nogen;
static enum e_sg curseg = noseg;
static int outcol = 0;
static SIZE align_type = 0L;
static const char *prefix = "I";
static const char *comment = ";";

static struct oplst
{
    const char *s;
    unsigned int sa;		/* special addressing modes:
				 *    BI      immediate is bracketed,
				 *    SPN     size prefix needed.
				 *    FP      387 op
				 */
} opl[] =
{
    {
	"movsx", BI | SPN	/* op_movsbl */
    }
    , {
	"movzx", BI | SPN	/* op_movzbl */
    }
    , {
	"movsx", BI | SPN	/* op_movswl */
    }
    , {
	"movzx", BI | SPN	/* op_movzwl */
    }
    , {
	"movsx", BI | SPN	/* op_movsbw */
    }
    , {
	"movzx", BI | SPN	/* op_movzbw */
    }
    , {
	"cdq", BI | SPN		/* op_cdq */
    }
    , {
	"cwd", BI | SPN		/* op_cwd */
    }
    , {
	"cbw", BI | SPN		/* op_cbw */
    }
    , {
	"mov", BI | SPN		/* op_mov */
    }
    , {
	"xchg", BI | SPN	/* op_xchg */
    }
    , {
	"lea", BI | SPN		/* op_lea */
    }
    , {
	"not", BI | SPN		/* op_not */
    }
    , {
	"neg", BI | SPN		/* op_neg */
    }
    , {
	"add", BI | SPN		/* op_add */
    }
    , {
	"sub", BI | SPN		/* op_sub */
    }
    , {
	"adc", BI | SPN		/* op_adc */
    }
    , {
	"sbb", BI | SPN		/* op_sbb */
    }
    , {
	"imul", BI | SPN	/* op_imul */
    }
    , {
	"idiv", BI | SPN	/* op_idiv */
    }
    , {
	"div", BI | SPN		/* op_div */
    }
    , {
	"and", BI | SPN		/* op_and */
    }
    , {
	"or", BI | SPN		/* op_or */
    }
    , {
	"xor", BI | SPN		/* op_xor */
    }
    , {
	"inc", BI | SPN		/* op_inc */
    }
    , {
	"dec", BI | SPN		/* op_dec */
    }
    , {
	"cmp", BI | SPN		/* op_cmp */
    }
    , {
	"push", BI | SPN	/* op_push */
    }
    , {
	"pop", BI | SPN		/* op_pop */
    }
    , {
	"jmp", SPN		/* op_jmp */
    }
    , {
	"loop", SPN		/* op_loop */
    }
    , {
	"call", SPN		/* op_call */
    }
    , {
	"leave", SPN		/* op_leave */
    }
    , {
	"enter", SPN		/* op_enter */
    }
    , {
	"ret", SPN		/* op_ret */
    }
    , {
	"test", BI | SPN	/* op_test */
    }
    , {
	"jmp", SPN		/* op_bra */
    }
    , {
	"je", SPN		/* op_je */
    }
    , {
	"jne", SPN		/* op_jne */
    }
    , {
	"jl", SPN		/* op_jl */
    }
    , {
	"jle", SPN		/* op_jle */
    }
    , {
	"jg", SPN		/* op_jg */
    }
    , {
	"jge", SPN		/* op_jge */
    }
    , {
	"ja", SPN		/* op_ja */
    }
    , {
	"jae", SPN		/* op_jae */
    }
    , {
	"jb", SPN		/* op_jb */
    }
    , {
	"jbe", SPN		/* op_jbe */
    }
    , {
	"rep", SPN		/* op_rep */
    }
    , {
	"movs", BI | SPN	/* op_smov */
    }
    , {
	"shl", BI | SPN		/* op_shl */
    }
    , {
	"shr", BI | SPN		/* op_shr */
    }
    , {
	"sal", BI | SPN		/* op_asl */
    }
    , {
	"sar", BI | SPN		/* op_asr */
    }
    , {
	"rol", BI | SPN		/* op_rol */
    }
    , {
	"ror", BI | SPN		/* op_ror */
    }
    , {
	"sahf", BI | SPN	/* op_sahf */
    }
    , {
	"sete", SPN		/* op_sete */
    }
    , {
	"setne", SPN		/* op_setne */
    }
    , {
	"setb", SPN		/* op_setb */
    }
    , {
	"setbe", SPN		/* op_setbe */
    }
    , {
	"seta", SPN		/* op_seta */
    }
    , {
	"setae", SPN		/* op_setae */
    }
    , {
	"setl", SPN		/* op_setl */
    }
    , {
	"setle", SPN		/* op_setle */
    }
    , {
	"setg", SPN		/* op_setg */
    }
    , {
	"setge", SPN		/* op_setge */
    }
    , {
	"nop", 0		/* op_nop */
    }
    ,
#ifdef FLOAT_IEEE
    {
	"fadd", BI | SPN | FP	/* op_fadd */
    }
    , {
	"faddp", BI | SPN | FP	/* op_faddp */
    }
    , {
	"fsub", BI | SPN | FP	/* op_fsub */
    }
    , {
	"fsubp", BI | SPN | FP	/* op_fsubp */
    }
    , {
	"fdiv", BI | SPN | FP	/* op_fdiv */
    }
    , {
	"fdivp", BI | SPN | FP	/* op_fdivp */
    }
    , {
	"fmul", BI | SPN | FP	/* op_fmul */
    }
    , {
	"fmulp", BI | SPN | FP	/* op_fmulp */
    }
    , {
	"fsubr", BI | SPN | FP	/* op_fsubr */
    }
    , {
	"fsubrp", BI | SPN | FP	/* op_fsubrp */
    }
    , {
	"fdivr", BI | SPN | FP	/* op_fdivr */
    }
    , {
	"fdivrp", BI | SPN | FP	/* op_fdivrp */
    }
    , {
	"fld", BI | SPN | FP	/* op_fld */
    }
    , {
	"fldz", BI | SPN | FP	/* op_fldz */
    }
    , {
	"fst", BI | SPN | FP	/* op_fst */
    }
    , {
	"fstp", BI | SPN | FP	/* op_fstp */
    }
    , {
	"fstp st(0)", BI | SPN | FP	/* op_fpop */
    }
    , {
	"fild", BI | SPN | FP	/* op_fild */
    }
    , {
	"fildl", BI | SPN | FP	/* op_fildl */
    }
    , {
	"fistp", BI | SPN | FP	/* op_fistp */
    }
    , {
	"fistpl", BI | SPN | FP	/* op_fistpl */
    }
    , {
	"ftst", BI | SPN | FP	/* op_ftst */
    }
    , {
	"fchs", BI | SPN | FP	/* op_fchs */
    }
    , {
	"fcomp", BI | SPN | FP	/* op_fcomp */
    }
    , {
	"fcompp", BI | SPN | FP	/* op_fcompp */
    }
    , {
	"fnstsw", BI | SPN | FP	/* op_fnstsw */
    }
    , {
	"fwait", BI | SPN | FP	/* op_fwait */
    }
    ,
#endif /* FLOAT_IEEE */
#ifdef ASM
    {
	"", 0			/* op_asm */
    }
    ,
#endif /* ASM */
    {
	"|.line", 0		/* op_line */
    }
    , {
	(char *) NULL, 0	/* op_label */
    }
};

/*****************************************************************************/

static unsigned int putop P1 (OPCODE, op)
{
    if (op >= OP_MIN && op <= OP_MAX && opl[op].s != (char *) 0) {
	oprintf ("\t%s", opl[op].s);
	return opl[op].sa;
    }
    FATAL ((__FILE__, "putop", "illegal opcode %d", op));
    return 0U;	/*lint !e527*/	/* unreachable */
}

/*
 * put a constant to the output file.
 */
static void putconst P1 (const EXPR *, ep)
{

    if (ep == NIL_EXPR) {
	FATAL ((__FILE__, "putconst", "ep == 0"));
    }
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
	oprintf ("(");
	putconst (ep->v.p[0]);
	oprintf ("+");
	putconst (ep->v.p[1]);
	oprintf (")");
	break;
    case en_sub:
	oprintf ("(");
	putconst (ep->v.p[0]);
	oprintf ("-");
	putconst (ep->v.p[1]);
	oprintf (")");
	break;
    case en_uminus:
	oprintf ("-");
	putconst (ep->v.p[0]);
	break;
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
    case IL0:
	break;
    case IL1:
	oprintf ("byte ptr ");
	break;
    case IL2:
	oprintf ("word ptr ");
	break;
    case IL4:
	oprintf ("dword ptr ");
	break;
    case IL4 + 1:	/*lint !e408*/	/* type mismatch with switch expression */
	oprintf ("real4 ptr ");
	break;
    case IL8 + 1:	/*lint !e408*/	/* type mismatch with switch expression */
	oprintf ("real8 ptr ");
	break;
    case IL10 + 1:	/*lint !e408*/	/* type mismatch with switch expression */
	break;
    default:
	CANNOT_REACH_HERE ();
	break;
    }
}

/*
 * output a general addressing mode.
 */
static void putamode P3 (const ADDRESS *, ap, ILEN, len, unsigned int, sa)
{
    static const char *regname[(int) NUMREG + 1] = {
	"eax",			/* EAX */
	"edx",			/* EDX */
	"ecx",			/* ECX */
	"ebx",			/* EBX */
	"esi",			/* ESI */
	"edi",			/* EDI */
	"esp",			/* ESP */
	"ebp",			/* EBP */
	"ax",			/* AX */
	"dx",			/* DX */
	"cx",			/* CX */
	"bx",			/* BX */
	"si",			/* SI */
	"di",			/* DI */
	"sp",			/* SP */
	"bp",			/* BP */
	"al",			/* AL */
	"dl",			/* DL */
	"cl",			/* CL */
	"bl",			/* BL */
	"ah",			/* AH */
	"dh",			/* DH */
	"ch",			/* CH */
	"bh",			/* BH */
	"st",			/* ST(0) */
	"st(1)",		/* ST(1) */
	"INVALID_REGISTER",
	"INVALID_REGISTER",
    };

    REG     reg;

#ifdef FLOAT_IEEE
    if (sa & FP) {
	/* assume that st(n) never appears explicitly as an operand here */
	putlen (len);
    }
#endif
    switch (ap->mode) {
    case am_immed:
	if (sa & BI) {
	    switch (ap->u.offset->nodetype) {
	    case en_labcon:
	    case en_nacon:
		oprintf ("offset ");
		break;
	    default:
		break;
	    }
	}
	putconst (ap->u.offset);
	break;
    case am_direct:
	putlen (len);
	if (sa & BI) {
	    oprintf ("[");
	    putconst (ap->u.offset);
	    oprintf ("]");
	} else {
	    putconst (ap->u.offset);
	}
	break;
    case am_dreg:
    case am_areg:
	reg = ap->preg;
	switch (len) {
	case IL1:
	    reg = REG8 (reg);
	    break;
	case IL2:
	    reg = REG16 (reg);
	    break;
	default:
	    break;
	}
	oprintf ("%s", regname[reg]);
	break;
    case am_indx:
	putlen (len);
	if (ap->u.offset != NIL_EXPR) {
	    putconst (ap->u.offset);
	}
	/*lint -fallthrough */
    case am_ind:
	oprintf ("[%s]", regname[small_option ? REG16 (ap->preg) : ap->preg]);
	break;
    case am_indx2:
	putlen (len);
	if (ap->u.offset != NIL_EXPR) {
	    putconst (ap->u.offset);
	}
	oprintf ("[%s+%s]",
		 regname[small_option ? REG16 (ap->preg) : ap->preg],
		 regname[small_option ? REG16 (ap->sreg) : ap->sreg]);
	break;
    case am_freg:
	oprintf ("%s", regname[ap->preg]);
	break;
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
    ILEN    len1 = ip->length, len2 = ip->length;
    unsigned int sa;

    sa = putop (ip->opcode);
    /*
     * this is expensive, but some assemblers require it
     * this is to be moved to the peephole optimizer
     */
    switch (ip->opcode) {
    case op_shl:
    case op_shr:
    case op_asl:
    case op_asr:
	len1 = IL1;
	break;
    case op_movsbw:
    case op_movzbw:
	len2 = IL2;
	/*lint -fallthrough */
    case op_movsbl:
    case op_movzbl:
	len1 = IL1;
	break;
    case op_movswl:
    case op_movzwl:
	len1 = IL2;
	break;
    case op_smov:
	switch (ip->length) {
	case IL1:
	    oprintf ("b");
	    break;
	case IL2:
	    oprintf ("w");
	    break;
	case IL4:
	    oprintf ("d");
	    break;
	default:
	    FATAL (
		   (__FILE__, "putcode", "illegal length field %d",
		    (int) ip->length));
	    break;
	}
	break;
    default:
	break;
    }

    /*
     * Masm uses the INTEL syntax:
     * The destination comes first
     * The source comes second
     */
    if (ip->oper1 != NIL_ADDRESS) {
	oprintf ("\t");
	if (ip->oper2 != NIL_ADDRESS && ip->opcode != op_line) {
	    putamode (ip->oper2, len2, sa);
	    oprintf (", ");
	}
	putamode (ip->oper1, len1, sa);
	if (ip->oper2 != NIL_ADDRESS && ip->opcode == op_line) {
	    oprintf ("%s%s>>>>\t", newline, comment);
	    putamode (ip->oper2, len2, sa);
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
	"db\t",			/* bytegen */
	"dw\t",			/* wordgen */
	"dd\t",			/* longgen */
	"dd\t",			/* longlonggen */
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

    ieee_double (vp, ul, FALSE);
    put_dword ((UVAL) ul[0]);
    put_dword ((UVAL) ul[1]);
}

PRIVATE void put_longdouble P1 (const RVAL *, vp)
{
    unsigned long ul[3];

    ieee_longdouble (vp, ul, FALSE);
    put_dword ((UVAL) ul[0]);
    put_dword ((UVAL) ul[1]);
    put_dword ((UVAL) ul[2]);
}

#endif /* FLOAT_IEEE */
#endif /* FLOAT_BOOTSTRAP */

#ifndef RELOC_BUG
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

#endif /* RELOC_BUG */

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
    put_header ((tp_pointer->size == 2L ? wordgen : longgen),
		alignment_of_type (tp_pointer));
    putconst (ep);
    outcol += 10;
}

/*ARGSUSED */
PRIVATE void put_storage P1 (SYM *, sp)
{
    SIZE    size = typeof (sp)->size;

    put_bseg (alignment_of_type (typeof (sp)));
    if (is_static (sp)) {
	oprintf ("%s%u\tbyte\t%ld dup (0)%s",
		 prefix, (unsigned) sp->value.l, size, newline);
    } else {
	oprintf ("%s\tbyte\t%ld dup (0)%s", outlate (nameof (sp)), size,
		 newline);
    }
}


/*
 * dump the string literal pool.
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
	put_byte ((UVAL) Ox0UL);
    }
    nl ();
}

/*
 * write out the type of the symbol
 */
static void puttype P1 (const TYP *, tp)
{
    if (tp == NIL_TYP) {
	/* runtime support routine */
	oprintf ("far");
	return;
    }
    switch (tp->type) {
    case bt_char:
    case bt_schar:
    case bt_uchar:
    case bt_charu:
	oprintf ("byte");
	break;
    case bt_short:
    case bt_ushort:
    case bt_int16:
    case bt_uint16:
    case bt_pointer16:
	oprintf ("word");
	break;
    case bt_int32:
    case bt_uint32:
    case bt_long:
    case bt_ulong:
    case bt_pointer32:
	oprintf ("dword");
	break;
    case bt_float:
	oprintf ("real4");
	break;
    case bt_double:
    case bt_longdouble:
	oprintf ("real8");
	break;
    case bt_func:
	oprintf ("far");
	break;
    default:
	oprintf ("byte");
	break;
    }
}

/* put the definition of an external name in the ouput file */
/* assembler can find out about externals itself. This also has the
 * advantage that I don't have to worry if the symbol is in text or
 * data segment. Therefore this function is a noop
 */
PRIVATE void put_reference P1 (SYM *, sp)
{
    if (!is_symbol_output (sp)) {
	switch (storageof (sp)) {
	case sc_global:
	    put_noseg ();
	    oprintf ("\tpublic\t%s%s", outlate (nameof (sp)), newline);
	    break;
	case sc_external:
	    put_noseg ();
	    oprintf ("\textrn\t%s:", outlate (nameof (sp)));
	    puttype (typeof (sp));
	    oprintf ("%s", newline);
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
	case 0L:
	    break;
	case 2L:
	case 4L:
	    oprintf ("\talign\t%d%s", (int) al, newline);
	    break;
	default:
	    FATAL ((__FILE__, "put_align", "align == %ld", al));
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
	oprintf ("\tassume ds:flat%s", newline);
	oprintf ("\t%s%s", segname, newline);
	curseg = segtype;
	align_type = 0L;
    }
    put_align (al);
}

PRIVATE void put_cseg P1 (SIZE, al)
{
    seg (codeseg, ".code", al);
}

PRIVATE void put_dseg P1 (SIZE, al)
{
    seg (dataseg, ".data", al);
}

static void put_bseg P1 (SIZE, al)
{
    put_dseg (al);
}

PRIVATE void put_kseg P1 (SIZE, al)
{
    put_dseg (al);
}

PRIVATE void put_rseg P1 (SIZE, al)
{
    put_cseg (al);
}

static void put_noseg P0 (void)
{
    nl ();
    if (curseg != noseg) {
	curseg = noseg;
	align_type = 0L;
	oprintf ("\tassume\tds:nothing%s", newline);
    }
}

PRIVATE void put_finish P0 (void)
{
    oprintf ("\tend%s", newline);
}

PRIVATE void put_start P0 (void)
{
    oprintf ("%s Generated by %s (masm) %s from %s%s",
	     comment, PROGNAME, VERSION, in_file, newline);
    oprintf ("\t.386%s", newline);	/* directive to support 386 instructions */
    oprintf ("\t.387%s", newline);	/* directive to support 487 instructions */
    oprintf ("\t.model	flat, C%s", newline);
    oprintf ("\t.data%s", newline);
    oprintf ("\t.code%s", newline);
    oprintf ("\toption dotname%s", newline);
    oprintf ("\tassume ds:flat%s", newline);
    oprintf ("\tassume ss:flat%s", newline);
}


#ifdef MULTIPLE_ASSEMBLERS
struct funcs masmx86_func = {
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
#endif /* TARGET_MAS */
#endif /* INTEL */
