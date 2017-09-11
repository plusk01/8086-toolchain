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
 *
 * 1995   Ivo Oesch, Started to build a codegenerator for the
 *        signalprocessor TMS320C30 (December)
 */

/*****************************************************************************/

#include "config.h"

#ifdef TMS320C30
#ifdef TARGET_ROSSIN

#define	OUT_MODULE
#include "chdr.h"
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"
#include "genc30.h"
#include "outproto.h"
#include "version.h"
#include <time.h>

/********************************************************** Type Definitions */

enum e_gt
{
    bytegen, wordgen, longgen, longlonggen, floatgen, nogen
};
enum e_sg
{
    noseg, codeseg, dataseg, romseg, constseg
};

/*********************************************** Static Function Definitions */

static void nl P_ ((void));
static void putop P_ ((OPCODE));
static void putconst P_ ((const EXPR *));
static void putamode P_ ((const ADDRESS *));
static void putreg P_ ((REG));
static void put_header P_ ((enum e_gt, SIZE));
static void seg P_ ((enum e_sg, const char *, SIZE));

/*********************************************** Global Function Definitions */

PRIVATE void put_align P_ ((SIZE));
PRIVATE void put_dword P_ ((UVAL));
PRIVATE void put_cseg P_ ((SIZE));
PRIVATE void put_dseg P_ ((SIZE));
PRIVATE void put_kseg P_ ((SIZE));
PRIVATE void put_rseg P_ ((SIZE));
PRIVATE void put_label P_ ((LABEL));
PRIVATE void put_reference P_ ((SYM *));
PRIVATE void put_byte P_ ((UVAL));

/********************************************************** Static Variables */

static enum e_gt gentype = nogen;
static enum e_sg curseg = noseg;
static int outcol = 0;
static SIZE align_type;
static const char *prefix = "L";
static const char *comment = "*";

static const char *opl[] = {
    "absf",			/* op_absf */
    "absi",			/* op_absi */
#ifdef USE_ALL_OPCODES
    "addc",			/* op_addc */
#endif
    "addf",			/* op_addf */
    "addi",			/* op_addi */
    "and",			/* op_and */
    "andn",			/* op_andn */
    "ash",			/* op_ash */
    "cmpf",			/* op_cmpf */
    "cmpi",			/* op_cmpi */
    "fix",			/* op_fix */
    "float",			/* op_float */
#ifdef USE_ALL_OPCODES
    "idle",			/* op_idle */
    "lde",			/* op_lde */
#endif
    "ldf",			/* op_ldf */
    "ldf",			/* op_popldf */
#ifdef USE_ALL_OPCODES
    "ldfi",			/* op_ldfi */
#endif
    "ldi",			/* op_ldi */
    "ldi",			/* op_popldi */
#ifdef USE_ALL_OPCODES
    "ldii",			/* op_ldii */
    "ldm",			/* op_ldm */
#endif
    "lsh",			/* op_lsh */
    "mpyf",			/* op_mpyf */
    "mpyi",			/* op_mpyi */
#ifdef USE_ALL_OPCODES
    "negb",			/* op_negb */
#endif
    "negf",			/* op_negf */
    "negi",			/* op_negi */
    "nop",			/* op_nop */
#ifdef USE_ALL_OPCODES
    "norm",			/* op_norm  */
#endif
    "not",			/* op_not */
    "pop",			/* op_pop */
    "popf",			/* op_popf */
    "push",			/* op_push */
    "pushf",			/* op_pushf */
    "push",			/* op_pushnopeep */
    "pushf",			/* op_pushfnopeep */
    "or",			/* op_or */
#ifdef USE_ALL_OPCODES
    "rnd",			/* op_rnd */
    "rol",			/* op_rol */
    "rolc",			/* op_rolc */
    "ror",			/* op_ror */
    "rorc",			/* op_rorc */
#endif
    "rpts",			/* op_rpts */
    "stf",			/* op_stf */
#ifdef USE_ALL_OPCODES
    "stfi",			/* op_stfi */
#endif
    "sti",			/* op_sti */
#ifdef USE_ALL_OPCODES
    "stii",			/* op_stii */
    "sigi",			/* op_sigi */
    "subb",			/* op_subb */
    "subc",			/* op_subc */
#endif
    "subf",			/* op_subf */
    "subi",			/* op_subi */
#ifdef USE_ALL_OPCODES
    "subrb",			/* op_subrb */
#endif
    "subrf",			/* op_subrf */
    "subri",			/* op_subri */
    "tstb",			/* op_tstb  */
    "xor",			/* op_xor   */
#ifdef USE_ALL_OPCODES
    "iack",			/* op_iack  */
    "addc3",			/* op_addc3 */
#endif
    "addf3",			/* op_addf3 */
    "addi3",			/* op_addi3 */
    "and3",			/* op_and3  */
    "andn3",			/* op_andn3 */
    "ash3",			/* op_ash3  */
    "cmpf3",			/* op_cmpf3 */
    "cmpf3",			/* op_cmpf3 */
    "lsh3",			/* op_lsh3  */
    "mpyf3",			/* op_mpyf3 */
    "mpyi3",			/* op_mpyi3 */
    "or3",			/* op_or3   */
#ifdef USE_ALL_OPCODES
    "subb3",			/* op_subb3 */
#endif
    "subf3",			/* op_subf3 */
    "subi3",			/* op_subi3 */
    "tstb3",			/* op_tstb3 */
    "xor3",			/* op_xor3  */
    "ldfu",			/* op_ldfu  */
    "ldflo",			/* op_ldflo */
    "ldfls",			/* op_ldfls */
    "ldfhi",			/* op_ldfhi */
    "ldfhs",			/* op_ldfhs */
    "ldfeq",			/* op_ldfeq */
    "ldfne",			/* op_ldfne */
    "ldflt",			/* op_ldflt */
    "ldfle",			/* op_ldfle */
    "ldfgt",			/* op_ldfgt */
    "ldfge",			/* op_ldfge */
    "ldfz",			/* op_ldfz  */
    "ldfnz",			/* op_ldfnz */
    "ldfp",			/* op_ldfp  */
    "ldfn",			/* op_ldfn  */
    "ldfnn",			/* op_ldfnn */
    "ldiu",			/* op_ldiu  */
    "ldilo",			/* op_ldilo */
    "ldils",			/* op_ldils */
    "ldihi",			/* op_ldihi */
    "ldihs",			/* op_ldihs */
    "ldieq",			/* op_ldieq */
    "ldine",			/* op_ldine */
    "ldilt",			/* op_ldilt */
    "ldile",			/* op_ldile */
    "ldigt",			/* op_ldigt */
    "ldige",			/* op_ldige */
    "ldiz",			/* op_ldiz  */
    "ldinz",			/* op_ldinz */
    "ldip",			/* op_ldip  */
    "ldin",			/* op_ldin  */
    "ldinn",			/* op_ldinn */
    "br",			/* op_br    */
    "brd",			/* op_brd   */
    "call",			/* op_call  */
    "call",			/* op_xcall !!!! */
    "rptb",			/* op_rptb  */
#ifdef USE_ALL_OPCODES
    "swi",			/* op_swi   */
#endif
    "bu",			/* op_bu    */
    "blo",			/* op_blo   */
    "bls",			/* op_bls   */
    "bhi",			/* op_bhi   */
    "bhs",			/* op_bhs   */
    "beq",			/* op_beq   */
    "bne",			/* op_bne   */
    "blt",			/* op_blt   */
    "ble",			/* op_ble   */
    "bgt",			/* op_bgt   */
    "bge",			/* op_bge   */
    "bz",			/* op_bz    */
    "bnz",			/* op_bnz   */
    "bp",			/* op_bp    */
    "bn",			/* op_bn    */
    "bnn",			/* op_bnn   */
    "bud",			/* op_bud   */
    "blod",			/* op_blod  */
    "blsd",			/* op_blsd  */
    "bhid",			/* op_bhid  */
    "bhsd",			/* op_bhsd  */
    "beqd",			/* op_beqd  */
    "bned",			/* op_bned  */
    "bltd",			/* op_bltd  */
    "bled",			/* op_bled  */
    "bgtd",			/* op_bgtd  */
    "bged",			/* op_bged  */
    "bzd",			/* op_bzd   */
    "bnzd",			/* op_bnzd  */
    "bpd",			/* op_bpd   */
    "bnd",			/* op_bnd   */
    "bnnd",			/* op_bnnd  */
#ifdef USE_ALL_OPCODES
    "dbu",			/* op_dbu   */
    "dblo",			/* op_dblo  */
    "dbls",			/* op_dbls  */
    "dbhi",			/* op_dbhi  */
    "dbhs",			/* op_dbhs  */
    "dbeq",			/* op_dbeq  */
    "dbne",			/* op_dbne  */
    "dblt",			/* op_dblt  */
    "dble",			/* op_dble  */
    "dbgt",			/* op_dbgt  */
    "dbge",			/* op_dbge  */
    "dbz",			/* op_dbz   */
    "dbnz",			/* op_dbnz  */
    "dbp",			/* op_dbp   */
    "dbn",			/* op_dbn   */
    "dbnn",			/* op_dbnn  */
    "dbud",			/* op_dbud  */
    "dblod",			/* op_dblod */
    "dblsd",			/* op_dblsd */
    "dbhid",			/* op_dbhid */
    "dbhsd",			/* op_dbhsd */
    "dbeqd",			/* op_dbeqd */
    "dbned",			/* op_dbned */
    "dbltd",			/* op_dbltd */
    "dbled",			/* op_dbled */
    "dbgtd",			/* op_dbgtd */
    "dbged",			/* op_dbged */
    "dbzd",			/* op_dbzd  */
    "dbnzd",			/* op_dbnzd */
    "dbpd",			/* op_dbpd  */
    "dbnd",			/* op_dbnd  */
    "dbnnd",			/* op_dbnnd */
#endif
    "callu",			/* op_callu    */
#ifdef USE_ALL_OPCODES
    "calllo",			/* op_calllo   */
    "callls",			/* op_callls   */
    "callhi",			/* op_callhi   */
    "callhs",			/* op_callhs   */
    "calleq",			/* op_calleq   */
    "callne",			/* op_callne   */
    "calllt",			/* op_calllt   */
    "callle",			/* op_callle   */
    "callgt",			/* op_callgt   */
    "callge",			/* op_callge   */
    "callz",			/* op_callz    */
    "callnz",			/* op_callnz   */
    "callp",			/* op_callp    */
    "calln",			/* op_calln    */
    "callnn",			/* op_callnn   */
#endif
    "trapu",			/* op_trapu    */
#ifdef USE_ALL_OPCODES
    "traplo",			/* op_traplo   */
    "trapls",			/* op_trapls   */
    "traphi",			/* op_traphi   */
    "traphs",			/* op_traphs   */
    "trapeq",			/* op_trapeq   */
    "trapne",			/* op_trapne   */
    "traplt",			/* op_traplt   */
    "traple",			/* op_traple   */
    "trapgt",			/* op_trapgt   */
    "trapge",			/* op_trapge   */
    "trapz",			/* op_trapz    */
    "trapnz",			/* op_trapnz   */
    "trapp",			/* op_trapp    */
    "trapn",			/* op_trapn    */
    "trapnn",			/* op_trapnn   */
#endif
    "retiu",			/* op_retiu    */
#ifdef USE_ALL_OPCODES
    "retilo",			/* op_retilo   */
    "retils",			/* op_retils   */
    "retihi",			/* op_retihi   */
    "retihs",			/* op_retihs   */
    "retieq",			/* op_retieq   */
    "retine",			/* op_retine   */
    "retilt",			/* op_retilt   */
    "retile",			/* op_retile   */
    "retigt",			/* op_retigt   */
    "retige",			/* op_retige   */
    "retiz",			/* op_retiz    */
    "retinz",			/* op_retinz   */
    "retip",			/* op_retip    */
    "retin",			/* op_retin    */
    "retinn",			/* op_retinn   */
#endif
    "retsu",			/* op_retsu    */
#ifdef USE_ALL_OPCODES
    "retslo",			/* op_retslo   */
    "retsls",			/* op_retsls   */
    "retshi",			/* op_retshi   */
    "retshs",			/* op_retshs   */
    "retseq",			/* op_retseq   */
    "retsne",			/* op_retsne   */
    "retslt",			/* op_retslt   */
    "retsle",			/* op_retsle   */
    "retsgt",			/* op_retsgt   */
    "retsge",			/* op_retsge   */
    "retsz",			/* op_retsz    */
    "retsnz",			/* op_retsnz   */
    "retsp",			/* op_retsp    */
    "retsn",			/* op_retsn    */
    "retsnn",			/* op_retsnn   */
    "mpyf3_addf3",		/* op_mpyf3_addf3 */
    "mpyf3_subf3",		/* op_mpyf3_subf3 */
    "mpyi3_addi3",		/* op_mpyi3_addi3 */
    "mpyi3_subi3",		/* op_mpyi3_subi3 */
#endif
    "stf_stf",			/* op_stf_stf */
    "sti_sti",			/* op_sti_sti */
    "ldf_ldf",			/* op_ldf_ldf */
    "ldi_ldi",			/* op_ldi_ldi */
    "absf_stf",			/* op_absf_stf */
    "absi_sti",			/* op_absi_sti */
    "addf3_stf",		/* op_addf3_stf */
    "addi3_sti",		/* op_addi3_sti */
    "and3_sti",			/* op_and3_sti */
    "ash3_sti",			/* op_ash3_sti */
    "fix_sti",			/* op_fix_sti */
    "float_stf",		/* op_float_stf */
    "ldf_stf",			/* op_ldf_stf */
    "ldi_sti",			/* op_ldi_sti */
    "lsh3_sti",			/* op_lsh3_sti */
    "mpyf3_stf",		/* op_mpyf3_stf */
    "mpyi3_sti",		/* op_mpyi3_sti */
    "negf_stf",			/* op_negf_stf */
    "negi_sti",			/* op_negi_sti */
    "not_sti",			/* op_not_sti */
    "or3_sti",			/* op_or3_sti */
    "subf3_stf",		/* op_subf3_stf */
    "subi3_sti",		/* op_subi3_sti */
    "xor3_sti",			/* op_xor3_sti */

    (char *) NULL,		/* op_divs (pseudo) */
    (char *) NULL,		/* op_divu (pseudo) */

#ifdef ASM
    "",				/* op_asm */
#endif				/* ASM */
    "*.line",			/* op_line */
    (char *) NULL,		/* op_label */
    (char *) NULL		/* op_abort */
};

static OPCODE ParallelOp[][2] = {
#ifdef USE_ALL_OPCODES
    {op_mpyf3, op_addf3},	/* op_mpyf3_addf3 */
    {op_mpyf3, op_subf3},	/* op_mpyf3_subf3 */
    {op_mpyi3, op_addi3},	/* op_mpyi3_addi3 */
    {op_mpyi3, op_subi3},	/* op_mpyi3_subi3 */
#endif
    {op_stf, op_stf},		/* op_stf_stf     */
    {op_sti, op_sti},		/* op_sti_sti     */
    {op_ldf, op_ldf},		/* op_ldf_ldf   */
    {op_ldi, op_ldi},		/* op_ldi_ldi   */
    {op_absf, op_stf},		/* op_absf_stf  */
    {op_absi, op_sti},		/* op_absi_sti  */
    {op_addf3, op_stf},		/* op_addf3_stf */
    {op_addi3, op_sti},		/* op_addi3_sti */
    {op_and3, op_sti},		/* op_and3_sti  */
    {op_ash3, op_sti},		/* op_ash3_sti  */
    {op_fix, op_sti},		/* op_fix_sti   */
    {op_float, op_stf},		/* op_float_stf */
    {op_ldf, op_stf},		/* op_ldf_stf   */
    {op_ldi, op_sti},		/* op_ldi_sti   */
    {op_lsh3, op_sti},		/* op_lsh3_sti  */
    {op_mpyf3, op_stf},		/* op_mpyf3_stf */
    {op_mpyi3, op_sti},		/* op_mpyi3_sti */
    {op_negf, op_stf},		/* op_negf_stf  */
    {op_negi, op_sti},		/* op_negi_sti  */
    {op_not, op_sti},		/* op_not_sti   */
    {op_or3, op_sti},		/* op_or3_sti   */
    {op_subf3, op_stf},		/* op_subf3_stf */
    {op_subi3, op_sti},		/* op_subi3_sti */
    {op_xor3, op_sti},		/* op_xor3_sti  */
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
#ifdef FLOAT_SUPPORT
    case en_fcon:
	if (is_short_float (ep->v.f, bt_float)) {
	    oprintf ("%.9f", (double) ep->v.f);
	} else {
	    FATAL (
		   (__FILE__, "putconst", "illegal constant node (float) %d",
		    ep->nodetype));
	}
	break;
#endif /* FLOAT_SUPPORT */
    case en_labcon:
	oprintf ("%s%u", prefix, (unsigned) ep->v.l);
	break;
    case en_nacon:
	oprintf ("%s", outlate (ep->v.str));
	break;
    case en_add:
	if ((ep->v.p[0]->nodetype == en_icon)
	    || (ep->v.p[0]->nodetype == en_autocon)) {
	    /* avoid a number on first position (ROSSIN_AS30 2.4 hates that) */
	    putconst (ep->v.p[1]);
	    oprintf ("+");
	    putconst (ep->v.p[0]);
	} else {
	    putconst (ep->v.p[0]);
	    oprintf ("+");
	    putconst (ep->v.p[1]);
	}
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
#ifdef ASM
    case en_str:
	oprintf ("%s", ep->v.str);
	break;
#endif /* ASM */
    default:
	FATAL (
	       (__FILE__, "putconst", "illegal constant node %d",
		ep->nodetype));
	break;
    }
}

/*
 * put a constant to the output file.
 */
static void putconstneg P1 (const EXPR *, ep)
{

    switch (ep->nodetype) {
    case en_autocon:
    case en_icon:
	oprintf ("%ld", -ep->v.i);
	break;
#ifdef FLOAT_SUPPORT
    case en_fcon:
	if (is_short_float (ep->v.f, bt_float)) {
	    oprintf ("$%f", -(double) ep->v.f);
	} else {
	    FATAL (
		   (__FILE__, "putconstneg",
		    "illegal constant node (float) %d", ep->nodetype));
	}
	break;
#endif /* FLOAT_SUPPORT */
    case en_labcon:
	oprintf ("-%s%u", prefix, (unsigned) ep->v.l);
	break;
    case en_nacon:
	oprintf ("-%s", outlate (ep->v.str));
	break;
    case en_add:
	putconstneg (ep->v.p[0]);
	oprintf ("-");
	putconst (ep->v.p[1]);
	break;
    case en_sub:
	putconst (ep->v.p[1]);
	oprintf ("-");
	putconst (ep->v.p[0]);
	break;
    case en_uminus:
	/*oprintf("+"); */
	/*lint -fallthrough */
    case en_cast:
	putconst (ep->v.p[0]);
	break;
    default:
	FATAL (
	       (__FILE__, "putconstneg", "illegal constant node %d",
		ep->nodetype));
	break;
    }
}

/*
 * put a constant to the output file.
 */
static int testconst P1 (const EXPR *, ep)
{

    switch (ep->nodetype) {
    case en_autocon:
    case en_icon:
	return (ep->v.i >= 0 ? 1 : -1);
#ifdef FLOAT_SUPPORT
    case en_fcon:
	return (ep->v.f >= 0.0 ? 2 : -2);
#endif /* FLOAT_SUPPORT */
    case en_labcon:
	return 3;
    case en_nacon:
	return 3;
    case en_add:
	return 4;
    case en_sub:
	return 5;
    case en_cast:
	return testconst (ep->v.p[0]);
    case en_uminus:
	return -testconst (ep->v.p[0]);
    default:
	return 99;
    }
}

#ifdef DEBUG
void putamode_tst P1 (const ADDRESS *, ap)
{
    FHANDLE save = output;

    output = debugfile;
    putamode (ap);
    output = save;
}

void putreg_tst P1 (REG, reg)
{
    FHANDLE save = output;

    output = debugfile;
    putreg (reg);
    output = save;
}

#endif /* DEBUG */

/*
 * output a general addressing mode.
 */
static void putamode P1 (const ADDRESS *, ap)
{
    switch (ap->mode) {
    case am_const_direct:
#ifdef DEBUG
	if (is_debugging (DEBUG_CODE) || is_debugging (DEBUG_FLOW)) {
	    oprintf ("(const)");
	}
#endif /* DEBUG */
	/*lint -fallthrough */
    case am_direct:
	oprintf ("@");
	/*lint -fallthrough */
    case am_immed:
	putconst (ap->u.offset);
	break;
    case am_areg:
    case am_dreg:
    case am_ireg:
    case am_sreg:
    case am_freg:
	putreg (ap->preg);
	break;
    case am_const_ind:
#ifdef DEBUG
	if (is_debugging (DEBUG_CODE) || is_debugging (DEBUG_FLOW)) {
	    oprintf ("(const)");
	}
#endif /* DEBUG */
	/*lint -fallthrough */
    case am_ind:
	oprintf ("*ar%d", (int) ap->preg - REG_AR0);
	break;
    case am_ainc:
	oprintf ("*ar%d++(", (int) ap->preg - REG_AR0);
	putconst (ap->u.offset);
	oprintf (")");
	break;
    case am_adec:
	oprintf ("*ar%d--(", (int) ap->preg - REG_AR0);
	putconst (ap->u.offset);
	oprintf (")");
	break;
    case am_preinc:
	oprintf ("*++ar%d(", (int) ap->preg - REG_AR0);
	putconst (ap->u.offset);
	oprintf (")");
	break;
    case am_predec:
	oprintf ("*--ar%d(", (int) ap->preg - REG_AR0);
	putconst (ap->u.offset);
	oprintf (")");
	break;
    case am_indx:
	if (testconst (ap->u.offset) >= 0) {
	    oprintf ("*+ar%d(", (int) ap->preg - REG_AR0);
	    putconst (ap->u.offset);
	} else {
	    oprintf ("*-ar%d(", (int) ap->preg - REG_AR0);
	    putconstneg (ap->u.offset);
	}
	oprintf (")");
	break;
    case am_indx2:
	oprintf ("*+ar%d(ir%d)", (int) ap->preg - REG_AR0,
		 (int) ap->sreg - REG_IR0);
	break;
    case am_indxs:
	oprintf ("*-ar%d(ir%d)", (int) ap->preg - REG_AR0,
		 (int) ap->sreg - REG_IR0);
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
 * Following function is only used for debugging-purpose
 */
#if defined(FAST_PEEP) && !defined(SAVE_PEEP_MEMORY)
static void put_peepinfo (PEEPINFO *info)
{
    oprintf ("\t");
    if (info != NULL) {
	oprintf ("r:%08X ", info->read);
	oprintf ("w:%08X ", info->write);
	oprintf ("m:%08X ", info->modified);
	oprintf ("us:%08X ", info->used);
	oprintf ("up:%08X", info->updated);
    } else {
	oprintf ("<NoPeepinfo>");
    }
}

/* #define PUT_PEEPINFO(info) put_peepinfo(info)
 */
#define PUT_PEEPINFO(info)
#else
#define PUT_PEEPINFO(info)
#endif


/*
 * output a generic instruction.
 */
PRIVATE void put_code P1 (const CODE *, ip)
{
    if (ip->opcode < OP_PARALLEL_FIRST) {
	putop (ip->opcode);
	if (ip->src1 != NIL_ADDRESS) {
	    oprintf ("\t");
	    putamode (ip->src1);
	    if (ip->src2 != NIL_ADDRESS) {
		oprintf (",");
		putamode (ip->src2);
	    }
	    if (ip->dst != NIL_ADDRESS) {
		oprintf (",");
		putamode (ip->dst);
	    }
	} else if (ip->dst != NIL_ADDRESS) {
	    oprintf ("\t");
	    putamode (ip->dst);
	}
	PUT_PEEPINFO (ip->info);
	oprintf ("%s", newline);
    } else if (ip->opcode <= op_xor3_sti) {
	putop (ParallelOp[ip->opcode - OP_PARALLEL_FIRST][0]);	/*lint !e656*/	/*arithmetic operation uses compatible enum's */
	if (ip->src1 != NIL_ADDRESS) {
	    oprintf ("\t");
	    putamode (ip->src1);
	    if (ip->src2 != NIL_ADDRESS) {
		oprintf (",");
		putamode (ip->src2);
	    }
	    if (ip->dst != NIL_ADDRESS) {
		oprintf (",");
		putamode (ip->dst);
	    }
	}
	oprintf ("%s|| ", newline);
	putop (ParallelOp[ip->opcode - OP_PARALLEL_FIRST][1]);	/*lint !e656*/	/*arithmetic operation uses compatible enum's */
	if (ip->src21 != NIL_ADDRESS) {
	    oprintf ("\t");
	    putamode (ip->src21);
	    if (ip->src22 != NIL_ADDRESS) {
		oprintf (",");
		putamode (ip->src22);
	    }
	    if (ip->dst2 != NIL_ADDRESS) {
		oprintf (",");
		putamode (ip->dst2);
	    }
	}
	PUT_PEEPINFO (ip->info);
	oprintf ("%s", newline);
#ifdef ASM
    } else if (ip->opcode == op_asm) {
	/*oprintf ("\t"); we cannot output labels if tab is there */
	putamode (ip->src1);
	PUT_PEEPINFO (ip->info);
	oprintf ("\n");
#endif
    } else if (ip->opcode == op_line) {
	oprintf ("\n*>>>> line ");
	putamode (ip->src1);
	oprintf (": ");
	putamode (ip->src2);
	PUT_PEEPINFO (ip->info);
	oprintf ("%s%s", newline, newline);
    } else {
	FATAL ((__FILE__, "put_code", "illegal op %d", ip->opcode));
    }
}

/*
 * generate a register name
 */
static void putreg P1 (REG, r)
{
    static const char *regname[] = {
	"r0",			/* REG_R0 */
	"r1",			/* REG_R1 */
	"r2",			/* REG_R2 */
	"r3",			/* REG_R3 */
	"r4",			/* REG_R4 */
	"r5",			/* REG_R5 */
	"r6",			/* REG_R6 */
	"r7",			/* REG_R7 */
	"ar0",			/* REG_AR0 */
	"ar1",			/* REG_AR1 */
	"ar2",			/* REG_AR2 */
	"ar3",			/* REG_AR3 */
	"ar4",			/* REG_AR4 */
	"ar5",			/* REG_AR5 */
	"ar6",			/* REG_AR6 */
	"ar7",			/* REG_AR7 */
	"dp",			/* REG_DP */
	"ir0",			/* REG_IR0 */
	"ir1",			/* REG_IR1 */
	"bk",			/* REG_BK */
	"sp",			/* REG_SP */
	"st",			/* REG_ST */
	"ie",			/* REG_IE */
	"if",			/* REG_IF */
	"iof",			/* REG_IOF */
	"rs",			/* REG_RS */
	"re",			/* REG_RE */
	"rc"			/* REG_RC */
    };

    if (r >= REG_R0 && r <= REG_RC) {
	oprintf ("%s", regname[r]);
    } else {
	oprintf ("R%d??", (int) r);
    }
}

/*
 * generate a named label.
 */
PRIVATE void put_name P1 (SYM *, sp)
{
    put_reference (sp);
    oprintf ("%s:%s", outlate (sp->name), newline);
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
	".word \t",		/* bytegen */
	".word \t",		/* wordgen */
	".word \t",		/* longgen */
	".word \t",		/* longlonggen */
	".float\t",		/* floatgen */
    };

    if (gentype != gtype || outcol >= MAX_WIDTH) {
	put_align (al);
	gentype = gtype;
	outcol = 15;
	oprintf ("\t%s", directive[gtype]);
    } else
	oprintf (",");
}

PRIVATE void put_byte P1 (UVAL, val)
{
    put_header (bytegen, alignment_of_type (tp_char));
    oprintf ("0%lxh", val & OxffUL);
    outcol += 5;
}

PRIVATE void put_word P1 (UVAL, val)
{
    put_header (wordgen, alignment_of_type (tp_short));
    oprintf ("0%lxh", val & OxffffUL);
    outcol += 7;
}

#ifdef FLOAT_SUPPORT
PRIVATE void put_float P1 (const RVAL *, vp)
{
    put_header (floatgen, alignment_of_type (tp_float));
    oprintf ("%f", *vp);
    outcol += 80;
}

PRIVATE void put_double P1 (const RVAL *, vp)
{
    put_header (floatgen, alignment_of_type (tp_double));
    oprintf ("%f", *vp);
    outcol += 80;
}

PRIVATE void put_longdouble P1 (const RVAL *, vp)
{
    put_header (floatgen, alignment_of_type (tp_longdouble));
    oprintf ("%f", *vp);
    outcol += 80;
}

#endif /* FLOAT_SUPPORT */

PRIVATE void put_dword P1 (UVAL, val)
{
    put_header (longgen, alignment_of_type (tp_long));
    oprintf ("0%lxh", val);
    outcol += 11;
}

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
    put_header (longgen, alignment_of_type (tp_pointer));
    putconst (ep);
    outcol += 10;
}

PRIVATE void put_storage P1 (const SYM *, sp)
{
    nl ();
    oprintf ("\t.bss   ");
    /*bseg(); */

    if (sp->storage_class == sc_static) {
	oprintf ("%s%u", prefix, (unsigned int) sp->value.l);
    } else {
	oprintf ("%s", outlate (sp->name));
    }
    oprintf (", %ld%s", sp->tp->size, newline);
}


/*
 * dump the string literal pool.
 * if we are producing single copies of strings (which should therefore
 * be read only we put them in the text segement - else in the data segment.
 */
PRIVATE void put_literals P0 (void)
{
    const CHAR *cp;
    size_t  len;

    if (lang_option == LANG_KANDR) {
	put_dseg (alignment_of_type (tp_char));
    } else {
	put_rseg (alignment_of_type (tp_char));
    }
    for (; strtab != (struct slit *) NULL; strtab = strtab->next) {
	nl ();
	put_label (strtab->label);
	cp = strtab->str;
	for (len = strtab->len; len--;)
	    put_byte ((UVAL) *cp++);
	put_byte ((UVAL) 0);
	nl ();
    }
    nl ();
}

/* put the definition of an external name in the ouput file */
PRIVATE void put_reference P1 (SYM *, sp)
{
    if (!is_symbol_output (sp)) {
	switch (sp->storage_class) {
	case sc_global:
	    nl ();
	    oprintf ("\t.global %s%s", outlate (sp->name), newline);
	    break;
	case sc_external:
	    nl ();
	    oprintf ("\t.global %s%s", outlate (sp->name), newline);
	    break;
	default:
	    break;
	}
	symbol_output (sp);
    }
}

/* align the following data */
PRIVATE void put_align P1 (SIZE, al)
{
    nl ();
    if (al > align_type) {
	switch (al) {
	case 1:
	    break;
	case 2:
	case 4:
	    oprintf ("\t.even%s", newline);
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
	oprintf ("%s", newline);
	gentype = nogen;
	outcol = 0;
    }
}

static void seg P3 (enum e_sg, segtype, const char *, segname, SIZE, al)
{
    nl ();
    if (curseg != segtype) {
	oprintf ("\t%s%s", segname, newline);
	curseg = segtype;
	align_type = 0;
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

PRIVATE void put_kseg P1 (SIZE, al)
{
    seg (constseg, ".sect \".const\"", al);
}

PRIVATE void put_rseg P1 (SIZE, al)
{
    seg (romseg, ".sect \".rom\"", al);
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
    seg (codeseg, ".text", 0L);
    seg (romseg, ".sect \".rom\"", 0L);
    seg (dataseg, ".data", 0L);
    /*seg(bssseg, ".bss", 0L); */
    /* Do some consistency-check in opcodetable    */
    /* checks only size, but proved to be usefull */
    assert (((sizeof (opl) / sizeof (opl[0])) - 1) == (OP_MAX - OP_MIN));	/*lint !e656*/	/*arithmetic operation uses compatible enum's */
}

#ifdef MULTIPLE_ASSEMBLERS
struct funcs rosc30_funcs = {
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
#endif /* TARGET_ROSSIN */
#endif /* TMS320C30 */
