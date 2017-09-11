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

#include "chdr.h"
#ifdef CPU_DEFINED
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"

/********************************************************** Static Variables */

static LABEL breaklab;
static LABEL contlab;
static LABEL retlab;

/*********************************************** Static Function Definitions */

static void genwhile P_ ((const STMT *));
static void genfor P_ ((const STMT *));
static void genif P_ ((const STMT *));
static void gendo P_ ((const STMT *));
static void genswitch P_ ((const STMT *));
static void gencase P_ ((const STMT *));
static void genxswitch P_ ((const STMT *));
static void genreturn P_ ((const STMT *));
static void genstmt P_ ((const STMT *));

/*****************************************************************************/

static BOOL is_nolazy P2 (const STMT *, stmt, BOOL, all_jumps)
{
    for (; stmt; stmt = stmt->next) {
	switch (stmt->stype) {
	case st_for:
	case st_while:
	case st_do:
	    if (is_nolazy (stmt->s1, FALSE)) {
		return TRUE;
	    }
	    break;
	case st_goto:
	    return TRUE;
	case st_continue:
	case st_break:
#ifdef ASM
	case st_asm:		/* assume the worst! */
#endif /* ASM */
	    return all_jumps;
	case st_if:
	    if (is_nolazy (stmt->v1.s, all_jumps)) {
		return TRUE;
	    }
	    /*lint -fallthrough */
	case st_compound:
	case st_switch:
	    if (is_nolazy (stmt->s1, all_jumps)) {
		return TRUE;
	    }
	    break;
	case st_case:
	case st_default:
	case st_label:
	    if (is_nolazy (stmt->v1.s, all_jumps)) {
		return TRUE;
	    }
	    break;
	case st_expr:
	case st_return:
	    break;
	default:
	    CANNOT_REACH_HERE ();
	    break;
	}
    }
    return FALSE;
}

/*
 *   save the current offset of delayed stack optimisations and reset the
 *   pointer to 0.
 */
static SIZE lazystack P1 (BOOL, nolazy)
{
    SIZE    offset;

    if (nolazy) {
	return 0L;
    }
    offset = stack_offset;
    stack_offset = 0L;
    return offset;
}

/*
 *   restore the delayed stack operations back to a previously saved value
 */
static void reset_lazystack P2 (BOOL, nolazy, SIZE, offset)
{
    if (!nolazy) {
	g_stack (stack_offset);
	stack_offset = offset;
    }
}


/*
 * generate code to evaluate a while statement.
 */
static void genwhile P1 (const STMT *, stmt)
{
    LABEL   lab1 = contlab;	/* save old continue label */
    LABEL   lab2 = breaklab;	/* save old break label */
    LABEL   looplab = nextlabel++;	/* loop pabel */
    BOOL    nolazy = is_nolazy (stmt, TRUE);
    SIZE    offset = lazystack (nolazy);

    contlab = nextlabel++;	/* new continue label */
    breaklab = nextlabel++;	/* new break label */
    g_branch (contlab);
    g_label (looplab);
    genstmt (stmt->s1);
    g_label (contlab);
    g_jtrue (stmt->exp, looplab);
    g_label (breaklab);
    breaklab = lab2;		/* restore old break label */
    contlab = lab1;		/* restore old continue label */
    reset_lazystack (nolazy, offset);
}

/*
 * generate code to evaluate a for loop
 */
static void genfor P1 (const STMT *, stmt)
{
    LABEL   old_break = breaklab;
    LABEL   old_cont = contlab;
    LABEL   loop_label = nextlabel++;
    LABEL   entry_label = nextlabel++;
    BOOL    nolazy = is_nolazy (stmt, TRUE);
    SIZE    offset = lazystack (nolazy);

    breaklab = nextlabel++;
    contlab = nextlabel++;
    g_expression (stmt->exp);
    g_branch (entry_label);
    g_label (loop_label);
    genstmt (stmt->s1);
    g_label (contlab);
    g_expression (stmt->v2.e);
    g_label (entry_label);
    if (stmt->v1.e == NIL_EXPR) {
	g_branch (loop_label);
    } else {
	g_jtrue (stmt->v1.e, loop_label);
    }
    g_label (breaklab);
    breaklab = old_break;
    contlab = old_cont;
    reset_lazystack (nolazy, offset);
}

/*
 * generate code to evaluate an if statement.
 */
static void genif P1 (const STMT *, stmt)
{
    LABEL   lab1 = nextlabel++;
    BOOL    nolazy = is_nolazy (stmt, TRUE);
    SIZE    offset = lazystack (nolazy);

    g_jfalse (stmt->exp, lab1);
    genstmt (stmt->s1);
    if (stmt->v1.s != NIL_STMT) {	/* else part exists */
	LABEL   lab2 = nextlabel++;	/* exit label */

	g_branch (lab2);
	g_label (lab1);
	genstmt (stmt->v1.s);
	g_label (lab2);
    } else {			/* no else code */
	g_label (lab1);
    }
    reset_lazystack (nolazy, offset);
}

/*
 * generate code for a do - while loop.
 */
static void gendo P1 (const STMT *, stmt)
{
    LABEL   oldcont = contlab;
    LABEL   oldbreak = breaklab;
    LABEL   dolab = nextlabel++;
    BOOL    nolazy = is_nolazy (stmt, TRUE);
    SIZE    offset = lazystack (nolazy);

    contlab = nextlabel++;
    breaklab = nextlabel++;
    g_label (dolab);
    genstmt (stmt->s1);		/* generate body */
    g_label (contlab);
    g_jtrue (stmt->exp, dolab);
    g_label (breaklab);
    breaklab = oldbreak;
    contlab = oldcont;
    reset_lazystack (nolazy, offset);
}

/*
 * The full range of case labels has no defined minimum or maximum, because
 * you can look at them as signed or as unsigned integers.  But they can be
 * compared, if close in value, using a special "<=":                   (kjb)
 */
#ifdef LONGLONG
#ifdef LONGLONG_BOOTSTRAP
#define caselab_leq(lab1, lab2)	\
	(((UVAL) (lab2) - (UVAL) (lab1)) < (UVAL)Ox80000000UL)
#else
#define caselab_leq(lab1, lab2)	\
	(((UVAL) (lab2) - (UVAL) (lab1)) < (UVAL)0x8000000000000000UL)
#endif /* LONGLONG_BOOTSTRAP */
#else
#define caselab_leq(lab1, lab2)	\
	(((UVAL) (lab2) - (UVAL) (lab1)) < (UVAL)Ox80000000UL)
#endif /* LONGLONG */

/*
 * generate a switch statement.
 */
static void genswitch P1 (const STMT *, stmt)
{
    LABEL   deflab;
    UVAL    min_caselabel, max_caselabel, nxt_caselabel;
    UVAL    i;
    UVAL    number_of_cases;
    int     number_of_ranges;
    LABEL  *labels;
    STMT   *s, *head;
    STMT   *defcase = NIL_STMT;
    EXPR   *stmtexp = stmt->exp;

    head = stmt->s1;
    /*
     * analyze the switch statement
     */
    s = head;
    if (s != NIL_STMT && s->stype == st_default) {
	defcase = s;
	s = s->s1;
    }
    /* s points to the first case label */
    number_of_cases = (UVAL) 1;
    number_of_ranges = 1;
    if (s != NIL_STMT) {
	/*
	 * look for a suitable minimum case label
	 */
	min_caselabel = (UVAL) s->v2.i;
	for (s = s->s1; s != NIL_STMT; s = s->s1) {
	    if (s->stype == st_default) {
		defcase = s;
	    } else {
		if (!caselab_leq (min_caselabel, s->v2.i)) {
		    min_caselabel = (UVAL) s->v2.i;
		}
		number_of_cases++;
		if (s->s1
		    && (s->s1 != s->v1.s || s->s1->v2.i != s->v2.i + 1L)) {
		    number_of_ranges++;
		}
	    }
	}
	/*
	 * find the maximum using min_caselabel as "zero"
	 */
	max_caselabel = min_caselabel;
	for (s = head; s != NIL_STMT; s = s->s1) {
	    if (s->stype != st_default) {
		if (((UVAL) s->v2.i - min_caselabel) >
		    (max_caselabel - min_caselabel)) {
		    max_caselabel = (UVAL) s->v2.i;
		}
	    }
	}
    }
    if (defcase == NIL_STMT) {
	deflab = breaklab;
    } else {
	deflab = nextlabel++;
	defcase->v2.l = deflab;
    }

    if ((number_of_cases > (UVAL) 7) &&
	(number_of_ranges > 4) &&
	((max_caselabel - min_caselabel) / number_of_cases <= (UVAL) 5)) {
	/*
	 * search the cases: look for min_caselabel. nxt_caselabel is set to
	 * the label looked for in the next pass
	 */
	SWITCH *sw = (SWITCH *) xalloc (sizeof (SWITCH));

	sw->tablab = nextlabel++;
	sw->beglab = IandD_option ? nextlabel++ : sw->tablab;
	sw->deflab = deflab;
	sw->numlabs = (LABEL) ((max_caselabel - min_caselabel) + (UVAL) 1);
	sw->labels = labels =

	    (LABEL *) xalloc (sizeof (LABEL) * (size_t) sw->numlabs);

	sw->next = swtables;
	swtables = sw;

	g_switch_table (stmtexp, sw, min_caselabel, max_caselabel);

	while (caselab_leq (min_caselabel, max_caselabel)) {
	    nxt_caselabel = max_caselabel + (UVAL) 1;
	    for (s = head; s != NIL_STMT; s = s->s1) {
		if (s->stype != st_default) {
		    if ((UVAL) s->v2.i == min_caselabel) {
			s->v2.l = *labels++ = nextlabel++;
			/* remove statement from the list */
			s->stype = st_default;
		    } else if (!caselab_leq (nxt_caselabel, s->v2.i)) {
			nxt_caselabel = (UVAL) s->v2.i;
		    }
		}
	    }
	    /* fill the holes */
	    for (i = min_caselabel + (UVAL) 1;
		 !caselab_leq (nxt_caselabel, i); i++) {
		*labels++ = deflab;
	    }
	    min_caselabel = nxt_caselabel;
	}
    } else {
	/*
	 * generate sequential compare instructions this is not very
	 * intelligent, but quicker than a lookup in a (value,label) table
	 * and has no startup time, this makes it fast if there are few
	 * labels.
	 */
	g_switch_compare (stmtexp, head);
	g_branch (deflab);
    }
}

/*
 * generate the label and statement for the case statement.  Also generates
 * the label and statement for label statements.
 */
static void gencase P1 (const STMT *, stmt)
{
    g_label (stmt->v2.l);
    genstmt (stmt->v1.s);
}

/*
 * analyze and generate best switch statement.
 */
static void genxswitch P1 (const STMT *, stmt)
{
    LABEL   oldbreak = breaklab;
    SIZE    offset;		/* delayed stack offset */
    BOOL    nolazy;

    nolazy = is_nolazy (stmt, TRUE);
    offset = lazystack (nolazy);
    breaklab = nextlabel++;
    genswitch (stmt);
    genstmt (stmt->v1.s);
    g_label (breaklab);
    breaklab = oldbreak;
    reset_lazystack (nolazy, offset);
}

/*
 * generate a return statement.
 */
static void genreturn P1 (const STMT *, stmt)
{
    if (stmt->exp != NIL_EXPR) {
	g_return (stmt->exp, ret_type);
    }
    if (retlab == UNDEF_LABEL) {
	retlab = nextlabel++;
    }
    g_branch (retlab);
}

/*
 * genstmt will generate a statement and follow the next pointer until the
 * block is generated.
 */
static void genstmt P1 (const STMT *, stmt)
{
    for (; stmt != NIL_STMT; stmt = stmt->next) {
#ifdef DEBUGOPT
	if (debug_option) {
	    g_line (stmt->line, stmt->linetxt);
	}
#endif /*DEBUGOPT */
	switch (stmt->stype) {
	case st_goto:
	    g_branch (stmt->v2.l);
	    break;
#ifdef ASM
	case st_asm:
#endif /* ASM */
	case st_expr:
	    g_expression (stmt->exp);
	    break;
	case st_return:
	    genreturn (stmt);
	    break;
	case st_if:
	    genif (stmt);
	    break;
	case st_while:
	    genwhile (stmt);
	    break;
	case st_do:
	    gendo (stmt);
	    break;
	case st_for:
	    genfor (stmt);
	    break;
	case st_continue:
	    g_branch (contlab);
	    break;
	case st_break:
	    g_branch (breaklab);
	    break;
	case st_switch:
	    genxswitch (stmt);
	    break;
	case st_compound:
	    genstmt (stmt->s1);
	    break;
	case st_label:
	case st_case:
	case st_default:
	    gencase (stmt);
	    break;
	default:
	    FATAL (
		   (__FILE__, "genstmt", "unknown statement %d",
		    stmt->stype));
	    break;
	}
    }
}

/*
 * generate a function body.
 */
void genfunc P5 (SYM *, sp, const STMT *, stmt, CSE *, cse, LINE, line,
		 const CHAR *, linetxt)
{
    LABEL   startlab = nextlabel++;
    LABEL   mainlab = nextlabel++;

    retlab = contlab = breaklab = UNDEF_LABEL;
    max_scratch = 0L;
    stack_offset = 0L;
    if (lc_auto > lc_auto_max) {
	lc_auto_max = lc_auto;
    }
    g_auto_align ();

#ifdef DEBUGOPT
    if (debug_option) {
	g_line (line, linetxt);
    }
#endif /*DEBUGOPT */
    g_branch (startlab);

    g_label (mainlab);
    g_preload (cse);		/* load any necessary expression into regs */
    genstmt (stmt);
    if (retlab != UNDEF_LABEL) {
	g_label (retlab);
    }
    g_epilogue ();

    g_label (startlab);
    lc_auto_max += max_scratch;
    g_entry (lc_auto_max);
    g_branch (mainlab);
    g_flush (sp);
}
#endif /* CPU_DEFINED */
