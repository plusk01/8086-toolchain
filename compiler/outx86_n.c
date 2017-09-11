/*******************************************************************************

This file was taken from outx86_a.c, which was intended to produce assembly for
the MASM assembler. It has been modified where needed to be generate code for
the NASM assembler instead.  All changes are noted with a comment 
followed by my initials "-WSF" (for Wade S. Fife)

*******************************************************************************/

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
#ifdef TARGET_NASM	// -Was TARGET_MASM

/*
Define EXTENDED_JUMPS to cause the compiler to convert conditional jumps
to a sequence of a conditional jump followed by a regular jmp instruction. 
This allows a MUCH greater jump distance to be used for conditional jumps. 
It is also very inefficient.

NOTE: NASM will also do this same inefficient conversion if you use the -O1 
      option. NASM will do a much better conversion (by calculating if  it's
	  really necessary to add a jmp or not) if you use the -O2 option. The
	  -O2 option causes it to do an additional pass, slowing assembly.

#define EXTENDED_JUMPS
*/

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

#define MAX_LABEL	256		// Added for unique label generation (see variable initialization). -WSF

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
static void putamode P_ ((const ADDRESS *, ILEN, unsigned int, const CODE *)); // Added const CODE * parameter. -WSF
//static void putamode P_ ((const ADDRESS *, ILEN, unsigned int));
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

static char prefix_buffer[MAX_LABEL+1];	// Variable added for unique label generation. -WSF
static char *prefix = "L";				// This is the assembler label prefix (was "I"). -WSF
static const char *comment = ";";		// Character used to begin comments. -WSF

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
	"; >>>>> Line:", 0		/* op_line */	// Changed from '"|.line", 0' - WSF
    }
    , {
	(char *) NULL, 0	/* op_label */
    }
};

/*****************************************************************************/

// The following function was modified from commented version below to allow
// use of EXTENDED_JUMPS. Extendend jumps were no longer needed when NASM
// added the ability to do it automatically using the -O2 optimization. -WSF
static unsigned int putop P1 (OPCODE, op)
{
	if (op >= OP_MIN && op <= OP_MAX && opl[op].s != (char *) 0) {
#ifdef EXTENDED_JUMPS
		// Replace the conditional jump with logically opposite conditional jump.
		// Below are all the jcc instructions generated by the compiler. -WSF
		switch(op) {
		case op_je:
			oprintf("\tjne\t($+5)\t; jne over jmp%s", newline);
			oprintf("\tjmp NEAR");	// Do real jump
			break;
		case op_jne:
			oprintf("\tje\t($+5)\t; je over jmp%s", newline);
			oprintf("\tjmp NEAR");	// Do real jump
			break;
		case op_jl:
			oprintf("\tjnl\t($+5)\t; jnl over jmp%s", newline);
			oprintf("\tjmp NEAR");	// Do real jump
			break;
		case op_jle:
			oprintf("\tjnle\t($+5)\t; jnle over jmp%s", newline);
			oprintf("\tjmp NEAR");	// Do real jump
			break;
		case op_jg:
			oprintf("\tjng\t($+5)\t; jng over jmp%s", newline);
			oprintf("\tjmp NEAR");	// Do real jump
			break;
		case op_jge:
			oprintf("\tjnge\t($+5)\t; jnge over jmp%s", newline);
			oprintf("\tjmp NEAR");	// Do real jump
			break;
		case op_ja:
			oprintf("\tjna\t($+5)\t; jna over jmp%s", newline);
			oprintf("\tjmp NEAR");	// Do real jump
			break;
		case op_jae:
			oprintf("\tjnae\t($+5)\t; jnae over jmp%s", newline);
			oprintf("\tjmp NEAR");	// Do real jump
			break;
		case op_jb:
			oprintf("\tjnb\t($+5)\t; jnb over jmp%s", newline);
			oprintf("\tjmp NEAR");	// Do real jump
			break;
		case op_jbe:
			oprintf("\tjnbe\t($+5)\t; jnbe over jmp%s", newline);
			oprintf("\tjmp NEAR");	// Do real jump
			break;
		default:
			oprintf ("\t%s", opl[op].s);	// Resort to normal case if not a cond jump
		}
#else
		oprintf ("\t%s", opl[op].s);
#endif	// #ifdef EXTENDED_JUMPS
		return opl[op].sa;
	}
	FATAL ((__FILE__, "putop", "illegal opcode %d", op));
	return 0U;	/*lint !e527*/	/* unreachable */
}
/*
static unsigned int putop P1 (OPCODE, op)
{
    if (op >= OP_MIN && op <= OP_MAX && opl[op].s != (char *) 0) {
	oprintf ("\t%s", opl[op].s);
	return opl[op].sa;
    }
    FATAL ((__FILE__, "putop", "illegal opcode %d", op));
    return 0U;	/ *lint !e527* /	/ * unreachable * /
}
*/

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
	//oprintf ("%ld", ((int)((ep->v.i) << 16))>>16);   // Possible fix to above - WSF
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
	// In this code, all used to include "ptr" identifier (e.g., "byte ptr"). "ptr" has been 
    switch (l) {
    case IL0:
	break;
    case IL1:
	oprintf ("byte ");
	break;
    case IL2:
	oprintf ("word ");
	break;
    case IL4:
	oprintf ("dword ");
	break;
    case IL4 + 1:	/*lint !e408*/	/* type mismatch with switch expression */
	oprintf ("dword ");	// Was "real4 ptr " -WSF
	break;
    case IL8 + 1:	/*lint !e408*/	/* type mismatch with switch expression */
	oprintf ("qword ");	// Was "real8 ptr" -WSF
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
// The following function had the "const CODE* ip" parameter added to handle special NASM cases. -WSF
// For this reason P4 was originally P3 also. -WSF
static void putamode P4 (const ADDRESS *, ap, ILEN, len, unsigned int, sa, const CODE *, ip)
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
	"st0",			/* ST(0) */	// Changed from "st". -WSF
	"st1",			/* ST(1) */	// Changed from "st(1)". -WSF
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
		// oprintf ("offset ");	// This isn't needed by NASM. -WSF
		break;
	    default:
		break;
	    }
	}
	putconst (ap->u.offset);
	break;
    case am_direct:
	if(ip->opcode != op_lea) putlen (len);	// Used to be just putlen(len). NASM doesn't like a size specifier on lea. -WSF
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
	// The following case was modifed from below for proper indexing in NASM (e.g. [bp-2] instead of -2[bp].) -WSF
	case am_indx:
	if(ip->opcode != op_lea) putlen (len);
	oprintf ("[%s", regname[small_option ? REG16 (ap->preg) : ap->preg]);
	if (ap->u.offset != NIL_EXPR) {
		if(ap->u.offset->v.i >= 0) oprintf("+");
		putconst (ap->u.offset);
	}
	oprintf ("]");
	break;
	/*
    case am_indx:
	putlen (len);
	if (ap->u.offset != NIL_EXPR) {
	    putconst (ap->u.offset);
	}
	*/
	/*lint -fallthrough */
    case am_ind:
	if(ip->opcode != op_lea) putlen (len);	// This is needed by NASM. -WSF
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

	// The following code replaces the commented version below. -WSF
	
	/*	This version prints line number and comments on one line
	if(ip->oper1 != NIL_ADDRESS) {
		if(ip->oper2 != NIL_ADDRESS) {
			if(ip->opcode != op_line) {
				oprintf ("\t");
				putamode(ip->oper2, len2, sa, ip);
				oprintf(", ");
				putamode(ip->oper1, len1, sa, ip);
			}
			else {
				char *string;
				
				// Skip over leading spaces
				string = ip->oper2->u.offset->v.str; 
				while(*string != 0 && (*string == ' ' || *string == '\t')) string++;
				// Print the comment
				oprintf(" %ld> %s", (long)ip->oper1->u.offset->v.i, string); // Add space after to prevent NASM EOL character problems
			}
		}
		else {
			oprintf("\t");
			putamode(ip->oper1, len1, sa, ip);
		}
	}*/

	// This version prints line number and comments on 2 lines
	if (ip->oper1 != NIL_ADDRESS) {
		if(ip->opcode != op_asm) oprintf ("\t");
		if (ip->oper2 != NIL_ADDRESS && ip->opcode != op_line) {
			putamode (ip->oper2, len2, sa, ip);
			oprintf (", ");
		}
		putamode (ip->oper1, len1, sa, ip);
		if (ip->oper2 != NIL_ADDRESS && ip->opcode == op_line) {
			char *string;
			string = ip->oper2->u.offset->v.str; 
			while(*string != 0 && (*string == ' ' || *string == '\t')) string++;

			oprintf("%s\t%s >>>>> %s ", newline, comment, string);
		}
    }

	// End replacement section. -WSF

	/* Original code:
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
	*/

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
	"DB\t",			/* bytegen */
	"DW\t",			/* wordgen */
	"DD\t",			/* longgen */
	"DD\t",			/* longlonggen */
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
// The following function has been rewritten for NASM. See original below. -WSF
PRIVATE void put_storage P1 (SYM *, sp)
{
	// This function inserts assembler pseudo ops for allocating storage space. -WSF
	SIZE    size = typeof (sp)->size;
	
	put_bseg (alignment_of_type (typeof (sp)));
	if (is_static (sp)) {
		oprintf ("%s%u:%s\tTIMES\t%ld db 0%s", prefix, (unsigned) sp->value.l, newline, size, newline);
	}
	else {
		oprintf ("%s:%s\tTIMES\t%ld db 0%s", outlate (nameof (sp)), newline, size, newline);
	}
}
/*
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
*/


/*
 * dump the string literal pool.
 */
// The following function has been rewritten for NASM. See original below. -WSF
PRIVATE void put_literals P0 (void)
{
	const CHAR *cp;
	size_t  len;
	int instr;
	
	if (lang_option == LANG_KANDR) {
		put_dseg (alignment_of_type (tp_char));
	}
	else {
		put_kseg (alignment_of_type (tp_char));
	}
	for (; strtab != NIL_STRING; strtab = strtab->next) {
		nl ();
		put_label (strtab->label);
		cp = strtab->str;
		
		// The rest of this function has been heavily revised to allow nice strings in NASM. -WSF
		put_header (bytegen, alignment_of_type (tp_char));
		instr = 0;
		for (len = strtab->len; len--;) {
			// Is it a visible character?
			if(*cp >= 32 && *cp <= 126) {
				// Is it a quote?
				if(*cp == '"') {
					if(instr) {
						// End string
						oprintf("\",");
						outcol += 2;
						instr = 0;
					}
					oprintf("'\"',");
					outcol += 4;
				}
				else {
					if(!instr) {
						// Start new string
						oprintf("\"");
						outcol ++;
						instr = 1;
					}
					oprintf("%c", (char)(*cp));
					outcol += 1;
				}
			}
			// Special charcter, insert hex number
			else {
				if(instr) {
					// End string
					oprintf("\",");
					outcol += 2;
					instr = 0;
				}
				oprintf ("0x%lX,", (UVAL)(*cp) & OxffUL);
				outcol += 5;
			}
			cp++;
		}
		if(instr) {
			oprintf("\",");
			instr = 0;
			outcol += 2;
		}
		oprintf("0");	// Null terminate
	}
	nl ();
}
/*
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
*/

/*
 * write out the type of the symbol
 */
static void puttype P1 (const TYP *, tp)
{
    if (tp == NIL_TYP) {
	/* runtime support routine */
	oprintf ("far");	// I don't think this works in NASM. -WSF
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
	oprintf ("dword");	// Changed from "real4". -WSF
	break;
    case bt_double:
    case bt_longdouble:
	oprintf ("qword");	// Changed from "real8". -WSF
	break;
    case bt_func:
	oprintf ("far");	// I don't think this works in NASM. -WSF
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
// The following code is commented because public and extern declarations aren't needed unless
// we use a linker. -WSF
PRIVATE void put_reference P1 (SYM *, sp)
{
	/*
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
	*/
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
	    oprintf ("\tALIGN\t%d%s", (int) al, newline);
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
	//oprintf ("\tassume ds:flat%s", newline);	// Not supported by NASM. -WSF
	//oprintf ("\t%s%s", segname, newline);		// Not supported by NASM. -WSF
	curseg = segtype;
	align_type = 0L;
    }
    put_align (al);	// This is needed. Removal causes strange results. -WSF
}

PRIVATE void put_cseg P1 (SIZE, al)
{
    seg (codeseg, ".TEXT", al);	// Was ".code". -WSF
}

PRIVATE void put_dseg P1 (SIZE, al)
{
    seg (dataseg, ".DATA", al); // Was ".data"
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
	/*	The following isn't supported by NASM. -WSF
    nl ();
    if (curseg != noseg) {
	curseg = noseg;
	align_type = 0L;
	oprintf ("\tassume\tds:nothing%s", newline);
    }
	*/
}

PRIVATE void put_finish P0 (void)
{
    //oprintf ("\tend%s", newline);	// The "end" indicator is not needed by NASM. -WSF
}

// The following function was replaced. See original below. -WSF
PRIVATE void put_start P0 (void)
{
	int end, start, i;

    oprintf ("%s Generated by %s (BYU-NASM) %s from %s%s", comment, PROGNAME, VERSION, in_file, newline);

	// Add header info
	oprintf("\tCPU\t8086%s", newline);
	oprintf("\tALIGN\t2%s", newline);
	//oprintf("\tORG\t100h%s%s", newline, newline);

	// Add instruction to jump to start of program
	//oprintf("\tsection .text%s", newline);
	oprintf("\tjmp\t%smain\t; Jump to program start%s", external_prefix, newline);
	
	// Append file name to labels for unique names //
	// This will give labels the form: L_file_name_#
	
	strcpy(prefix_buffer, prefix);
	strcat(prefix_buffer, "_");
	
	// Find start of filename (eliminate any extra path)
	for(start = strlen(in_file)-1; start >= 0; start--) {
		if(in_file[start] == '\\' || in_file[start] == '/') break;
	}	
	prefix = &in_file[start+1];

	// Find index of '.' in file name, if it has one
	for(end = 0; (prefix[end] != '.') && (prefix[end+1] != '\0') && (end < MAX_LABEL - (strlen(prefix_buffer)+2)); end++);

	strncat(prefix_buffer, prefix, end);
	strcat(prefix_buffer, "_");
	prefix = prefix_buffer;

	// Eliminate any spaces (for windows)
	for(i = strlen(prefix_buffer); i >= 0; i--) {
		if(prefix_buffer[i] == ' ') prefix_buffer[i] = '_';
	}
	
	//
}
/*
PRIVATE void put_start P0 (void)
{
    oprintf ("%s Generated by %s (masm) %s from %s%s",
	     comment, PROGNAME, VERSION, in_file, newline);
    oprintf ("\t.386%s", newline);	// * directive to support 386 instructions * /
    oprintf ("\t.387%s", newline);	// * directive to support 487 instructions * /
    oprintf ("\t.model	flat, C%s", newline);
    oprintf ("\t.data%s", newline);
    oprintf ("\t.code%s", newline);
    oprintf ("\toption dotname%s", newline);
    oprintf ("\tassume ds:flat%s", newline);
    oprintf ("\tassume ss:flat%s", newline);
}
*/

#ifdef MULTIPLE_ASSEMBLERS
struct funcs nasmx86_func = {
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
#endif /* TARGET_NASM */
#endif /* INTEL */
