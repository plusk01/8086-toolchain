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

/******************************************************************************
 *
 *      This module handles the output of all messages.
 *
 *      This has been done to improve the size of
 *      generated code by exploiting the vfprint()
 *      family of routines, and also to make it
 *      easier to locate and change message text.
 *
 *      Merely by amending the messages in this module
 *      you can convert C68 to use a different language.
 *
 *      Author:   D. J. Walker          April 1992.
 *
 *
 *****************************************************************************/

#include "chdr.h"
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"

#ifdef HAS_STDARG
#include <stdarg.h>
#define VA_START(a,s)	va_start(a,s)
#ifdef __STDC__
#define P1V(t1, a1)			(t1 a1, ...)
#define P2V(t1, a1, t2, a2)		(t1 a1, t2 a2, ...)
#define P3V(t1, a1, t2, a2, t3, a3)	(t1 a1, t2 a2, t3 a3, ...)
#else /* __STDC__ */
#define P1V(t1, a1)			(a1) t1 a1;
#define P2V(t1, a1, t2, a2)		(a1, a2, a3) t1 a1; t2 a2;
#define P3V(t1, a1, t2, a2, t3, a3)	(a1, a2, a3) t1 a1; t2 a2; t3 a3;
#endif /* __STDC__ */
#else /* HAS_STDARG */
#include <varargs.h>
#define VA_START(a,s)	va_start(a)
#define P1V(t1, a1)			(a1, va_alist) t1 a1; va_dcl
#define P2V(t1, a1, t2, a2)		(a1, a2, va_alist) t1 a1; t2 a2; va_dcl
#define P3V(t1, a1, t2, a2, t3, a3)	(a1, a2, a3, va_alist) t1 a1; t2 a2; t3 a3; va_dcl
#endif /* HAS_STDARG */

/********************************************************* Macro Definitions */

#define	WARN_LEVEL_MAX	((int)(sizeof(warnlevel)/sizeof(int))-1)

/********************************************************** Static Variables */

#ifndef SYNTAX_CORRECT
/*
 *   The following table is used to determine the breaks between the
 *   various levels of error/warning messages, and for warnings, whether
 *   the warning level allows messages to be output
 */
static MSGNUM warnlevel[] = {
    WARN_LEVEL1,
    WARN_LEVEL2,
    WARN_LEVEL3,
    WARN_LEVEL4,
    WARN_LEVEL5,
    WARN_LEVEL6,
    WARN_LEVEL7,
#ifdef FACIST
    WARN_LEVEL8,
#endif				/* FACIST */
    MSG_BASE
};
#endif /* SYNTAX_CORRECT */

/*****************************************************************************/

/*
 *   This routine handle all output and checks the return value
 *   from the output routine to ensure that it was successful.
 */

static void vprint P3 (FHANDLE, file, const char *, fmt, va_list, ap)
{
    if (vfprintf (file, fmt, ap) < 0) {
	if (file != errfile) {
	    message (MSG_WRITEFAIL);
	}
	exit (EXIT_SUCCESS);
    }
}

/*
 *  eprintf  functional equivalent to printf that writes
 *           to stderr instead of stdout.
 */
/*PRINTFLIKE1 */
void eprintf P1V (const char *, formatstr)
{
    va_list ap;

    VA_START (ap, formatstr);
    vprint (errfile, formatstr, ap);
    va_end (ap);
}

#ifdef ICODE
/*
 *  iprintf  functional equivalent to printf that writes
 *           to icode instead of stdout.
 */
/*PRINTFLIKE1 */
void iprintf P1V (const char *, formatstr)
{
    va_list ap;

    VA_START (ap, formatstr);
    vprint (listfile, formatstr, ap);
    va_end (ap);
}

#endif /* ICODE */

#ifdef LIST
/*
 *  lprintf  functional equivalent to printf that writes
 *           to list instead of stdout.
 */
/*PRINTFLIKE1 */
void lprintf P1V (const char *, formatstr)
{
    va_list ap;

    VA_START (ap, formatstr);
    vprint (listfile, formatstr, ap);
    va_end (ap);
}

#endif /* LIST */

#if defined(DEBUG) && !defined(MACOSX)
/*
 *  dprintf  functional equivalent to printf that writes
 *           to debug instead of stdout.  The flag argument
 *           determines whether to output the message.
 */
/*PRINTFLIKE2 */
void dprintf P2V (int, flag, const char *, formatstr)
{
    if (is_debugging (flag)) {
	va_list ap;

	VA_START (ap, formatstr);
	vprint (debugfile, formatstr, ap);
	va_end (ap);
    }
}
#endif /* DEBUG */


#ifdef CPU_DEFINED
/*
 *  oprintf  functional equivalent to printf that writes
 *           to output instead of stdout.
 */
/*PRINTFLIKE1 */
void oprintf P1V (const char *, formatstr)
{
    va_list ap;

    VA_START (ap, formatstr);
    vprint (output, formatstr, ap);
    va_end (ap);
}

#endif /* CPU_DEFINED */

/*
 *  message  Generalised message output routine.
 *
 *           It accepts a message number plus optional extra parameters
 *           in printf() style.
 *
 *           The range of the message number passed is used to classify
 *           the message type and take appropriate action.
 *
 * The parameter to message() cannot be an enumeration because the 2nd
 * parameter to va_start() cannot be a parameter which could be changed
 * by default promotions.
 */
/*VARARGS1 */
void message P1V (int, mnum)
{
    va_list ap;
    MSGNUM  msgnum = (MSGNUM) mnum;

    VA_START (ap, mnum);
#ifndef SYNTAX_CORRECT
    if (msgnum < MSG_BASE) {
	MSGNUM  errtype;

	/* In case of warning messages, check against warning level */
	if (msgnum >= warnlevel[warn_option]) {
	    /* warning level too low - ignore message */
	    va_end (ap);
	    return;
	}
	if (error_resync) {
	    /* still resyncing after an error */
	    va_end (ap);
	    return;
	}
	if (msgnum < warnlevel[error_option]) {
	    /* error being generated so restart resyncing */
	    error_resync = ERROR_RESYNC;
	}
	eprintf (message_text (MSG_LINE), act_file, act_line);
	errtype =
	    (MSGNUM) (msgnum <
		      warnlevel[error_option] ? MSG_ERROR : MSG_WARNING);
	eprintf (message_text (errtype));
	if (msgnum >= WARN_LEVEL1) {
	    int     level;

	    for (level = 1;
		 (level <= WARN_LEVEL_MAX) && (msgnum >= warnlevel[level]);
		 level++) continue;
	    eprintf (" [%d]", level);
	}
	eprintf (": ");
    }
#endif /* SYNTAX_CORRECT */

    /* finally we output text + newline */
    vprint (errfile, message_text (msgnum), ap);
    eprintf ("%s", newline);

#ifndef SYNTAX_CORRECT
    if (msgnum < warnlevel[error_option]) {
	/*
	 * Do not proceed if more than 'max_error_count' errors were detected.
	 * 'max_error_count' should be high since each error might be reported
	 * more than once
	 */
	if (++total_errors >= max_error_count) {
	    message (MSG_MAXERROR);
	    exit (EXIT_FAILURE);
	}
	code_option = FALSE;	/* stop generating code - syntax check only */
    }
#endif /* SYNTAX_CORRECT */
    va_end (ap);
}


/*
 *   fatal() is called when a serious error has occurred within the
 *   compiler.  Some diagnostic information is also output which may
 *   aid in determining what went wrong.
 */
/*PRINTFLIKE3 */
void fatal P3V (const char *, fname, const char *, routine, const char *, msg)
{
    va_list ap;
    static BOOL beenhere = FALSE;

    VA_START (ap, msg);
    if (!beenhere) {
	beenhere = TRUE;
	message (MSG_FATAL, fname, routine);
	vprint (errfile, msg, ap);
	message (MSG_MISSING);
#ifdef CPU_DEFINED
	g_flush (NIL_SYM);
#endif /* CPU_DEFINED */
    }
    va_end (ap);
#ifdef WIN32
#ifdef _DEBUG
    VOIDCAST getchar ();
#endif /* _DEBUG */
#endif /* WIN32 */
    exit (EXIT_FAILURE);
}


#ifdef LONGLONG_SUPPORT
/*
 *  Dump a long long value
 */ 
char   *longlongStr P1 (UVAL, value) 
{
    static char buffer[22];
    union
    {
	UVAL   ll;
	unsigned long l[2];
    } v;

    v.ll = value;
    
#ifdef WIN32
	VOIDCAST sprintf (buffer, "[0x%.8lx, 0x%.8lx]", v.l[1], v.l[0]);
    
#else	/*  */
	VOIDCAST sprintf (buffer, "[0x%.8lx, 0x%.8lx]", v.l[0], v.l[1]);
    
#endif	/*  */
	return buffer;
}


#endif	/* LONGLONG_SUPPORT */
