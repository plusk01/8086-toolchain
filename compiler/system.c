/*
 * C compiler
 * ==========
 *
 * Copyright 1995,	K.D.Walker
 *				and D.J.Walker
 * Credits to Matthew Brandt and Christopher van Wuellen.
 * All commercial rights reserved.
 *
 * This compiler may be redistributed as long there is no
 * commercial interest. The compiler must not be redistributed
 * without its full sources. This notice must stay intact.
 *
 * History:
 *
 * Feb 1995 	The operating and environment dependencies isolated
 *				in this module.  This has been done primarily as part
 *				of the compiler to run on the EPOC operating system
 *				used by the Psion 3a pocket computer.
 */

/******************************************************************************
 *
 *	This module handles the interface to the operating system.
 *
 *	There are only a small number of routines in the compiler that
 *	are dependent in any way on the operating environment in which
 *	the compiler is running.  All such dependencies are isolated
 *	in this source file.
 *
 *	The default versions of these routines are suitable for any
 *	Unix compatible system or any system that supports:
 *
 *	a)	The processing of command line arguments within the C startup
 *		module to set up the standard C argc/argv variables for
 *		argument passing.
 *	b)	The setup within the C startup module of the standard C I/O
 *		channels stdin,stdout and stderr.
 *	c)	Support for the following STDIO routines:
 *				vfprintf()
 *				fread()
 *
 *	Where the default routine is not satisfactory, then it will be
 *	necessary to supply alternative routines.  For examples on a
 *	typical instance of this, see the EPOC and QDOS specific parts
 *	later in this file.
 *
 *****************************************************************************/

#include "chdr.h"
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"

#ifndef EPOC

/*
 *	This is a special routine called right at the start of program
 *	initialisation to take any special action required to establish
 *	an initial error channel.	On systems which support the default
 *	'stderr' channel this can be a dummy.
 */
void openerror P0 (void)
{
    return;
}

/*
 *	Open up any files specified on the command line.
 */
void openfiles P2 (int, argc, const char * const*, argv)
{
#ifdef HAS_NLS
    catid = catopen (PROGNAME, 0);
#endif /* HAS_NLS */

#ifdef LIST
    if (argc > 2)
#else
    if (argc > 3)
#endif
    {
	message (MSG_EXTRAPARAM);
	exit (EXIT_FAILURE);
    }
    if (argc > 0) {
	/* used named file instead of stdin */

	if ((input = fopen (*argv, "r")) == NULL) {
	    message (MSG_OPENINPUT, *argv);
	    exit (EXIT_FAILURE);
	}
#ifdef BUFFER_SIZE
	VOIDCAST setvbuf (input, NULL, _IOFBF, BUFFER_SIZE);

#endif /* BUFFSER_SIZE */
	in_file = act_file = (const CHAR *) *argv;
    }
    argv++;
    if (argc > 1) {
	/* used named file instead of stdout */
	if ((output = fopen (*argv, "w")) == NULL) {
	    message (MSG_OPENOUTPUT, *argv);
	    exit (EXIT_FAILURE);
	}
#ifdef BUFFER_SIZE
	VOIDCAST setvbuf (output, NULL, _IOFBF, BUFFER_SIZE);

#endif /* BUFFER_SIZE */
    }
#ifdef LIST
    argv++;
    if (argc > 2) {
	/* used named listing file instead of stderr */
	if ((listfile = fopen (*argv, "w")) == NULL) {
	    message (MSG_OPENLISTING, *argv);
	    exit (EXIT_FAILURE);
	}
    }
#endif /* LIST */
    return;
}


#ifdef HAS_NLS
#include <nl_types.h>
nl_catd catid;

#endif /* HAS_NLS */

/*
 *	message_text Take a message number, and return a pointer
 *			  to the corresponding text string
 */
const char *message_text P1 (MSGNUM, num)
{
#ifdef HAS_NLS
    return (catgets (catid, NL_SETD, num + 1, msgtable[num]));
#else
    return (msgtable[num]);
#endif /* HAS_NLS */
}
#endif /* ! EPOC */

#ifdef EPOC
/********************************************************************* EPOC */
/*
 *	The EPOC operating system is used on the Psion range of pocket
 *	computers.	 These systems are based on the Intel X86 range of
 *	processors.  All programs have to conform to the small memory
 *	model (64k code + 64k data), although there are facilities for
 *	using inter-process communication and Dynamic Link Libraries to
 *	circumvent some of these restrictions.
 *
 *	The Psion SDK does provide a library (CLIB) that provides full
 *	support for all ANSI features, including those that would be
 *	required to use the DEFAULT section above.	However, if this is
 *	done, the compiler is too large to fit within the constraints of
 *	the small memory model.
 *
 *	An alternative library (PLIB) is available that is instead based
 *	around calling functions that are held in the Psion ROM.  This
 *	provides significantly smaller executables than are generated
 *	when CLIB is used.	However the functionality match to the ANSI
 *	routines is not exact, so that the emulation routines defined in
 *	this section are used instead.	Doing this, it is possible (just)
 *	to get the compiler within the small memory model.
 *
 *	NOTE.  You also need the CPOCLIB.LIB file as this contains some
 *		   generic support routines used by all components within
 *		   the EPOC emulation of the compilation system.
 *****************************************************************************/

#include <cpoclib.h>
#include <rscfile.xg>
#include "cpoclib.h"

GLREF_D VOID *DatCommandPtr;
LOCAL_C VOID *rcb;
LOCAL_D char buf[256];		/* scratch buffer used for resources loaded */

/*
 *	 Open resource file at add-file slot 2 inside own .img file.
 *		Set up the value of 'rcb'.	 This means that we avoid the
 *		overhead of having all messages embedded in the code which
 *		is important on the Psion because of small memory model.
 *
 *		Note that if we later want to pick up language dependent
 *		resource files, we can simply do it by changing this code
 *		to pick up the correct resource file.
 */
LOCAL_C VOID InitRsc P0 (VOID)
{
    HANDLE  OlibCat;

    p_findlib ("OLIB.DYL", &OlibCat);
    if ((rcb = p_newlibh (OlibCat, C_RSCFILE)) != NULL) {
	INT     ret;

	if ((ret = p_entersend3 (rcb, O_RS_INIT, DatCommandPtr)) == 0) {
	    return;
	}
    }
    p_exit (150);
}

/*
 *	 Read the relevant message from the resource file
 *	 Returns pointer to buffer containing the message text.
 *	 Note that the static buffer buf[] is used in every case.
 */
const char *message_text P1 (MSGNUM, num)
{
    if (p_entersend4 (rcb, O_RS_READ_BUF, num, buf) < 0) {
	p_exit (151);
    }
    return (char *) buf;
}

#ifdef HAS_STDARG
#include <stdarg.h>
#else
#include <varargs.h>
#endif /* HAS_STDARG */

/*
 *	EPOC based systems have a ROM based routine that is equivalent
 *	to a C style sprintf routine.	We take advantage of this to
 *	avoid the need to pull in a STDIO package.
 *
 *	The following important assumptions apply:
 *
 *	a)	The return value from 'vfprintf' is only relevant so far
 *		as it is checked as being non-zero
 *
 *	b)	The output must not exceed the size of the buffer allocated below
 *		N.B.  Should this be a constant set in chdr.h?
 */
int vfprintf P3 (FHANDLE, fd, char *, fmt, va_list, args)
{
    INT     len;
    TEXT    obuf[512];		/* scratch buffer used for messages */

    len = p_atob (&obuf[0], fmt, args);
#if 1
    if (fd == errfile && WriteToParentSetup () != NULL) {
	obuf[len] = '\0';
	WriteToParent (obuf, len + 1);
	return (len);
    }
#endif
    return p_write (fd, &obuf[0], len);
}

/*
 *	The routine build into the EPOC ROM is very similar to the
 *	ANSI C fread() routine except that it returns a -ve value on
 *	error rather than 0, and we need to correct for this
 */
size_t fread P4 (char *, buffer, size_t, size, size_t, num, FHANDLE, fd)
{
    INT     reply;

    if ((reply = p_read (fd, buffer, (unsigned) size * num)) < 0) {
	return (0);
    } else {
	return (reply);
    }
}

/*
 *	On EPOC based systems there is no stderr file opened by default
 *	when using PLIB.  We will therefore try and open such a channel
 *	on the current default drive.
 *
 *	We will try and append to the file if it already exists.  This
 *	allows for batch compiles.	It is up to the CPOC front-end to
 *	delete the error file as required.
 */
void openerror P0 (void)
{
#if 0
    TEXT    errname[P_FNAMESIZE];

#endif

    InitRsc ();
#if 0
    if (p_getenv ("CPOC", errname) < 0) {
	p_scpy (errname, "CPOC.LOG");
    }
    /* If file exists, then append with write access */
    if ((p_open (&errfile, errname, P_FSTREAM | P_FUPDATE | P_FAPPEND) < 0)
	/*      If file does not exist, then create new one */
	&& (p_open (&errfile, errname, P_FSTREAM | P_FUPDATE | P_FCREATE) <
	    0)) {
	/* we cannot output a failure message with no channel ! */
	exit (EXIT_FAILURE);
    }
#endif
}

/*
 *	On EPOC systems, we will always be passed an input file as
 *	this is guaranteed by the CPOC front-end.  Whether we will
 *	be passed a target assembler file will depend on whether
 *	we are generating code or not.
 */
void openfiles P2 (int, argc, char **, argv)
{
#if 0
    /*
     * Do we need to check the number of parameters ?
     * Will this not be controlled by the CPOC frint-end?
     */
    if (argc > 3) {
	message (MSG_EXTRAPARAM);
	exit (EXIT_FAILURE);
    }
#endif
    if (p_open (&input, argv[0], P_FSTREAM_TEXT) < 0) {
	message (MSG_OPENINPUT, argv[0]);
	exit (EXIT_FAILURE);
    }
    in_file = act_file = argv[0];

    if (p_open (&output, argv[1], P_FSTREAM_TEXT | P_FUPDATE | P_FREPLACE) <
	0) {
	message (MSG_OPENOUTPUT, argv[1]);
	exit (EXIT_FAILURE);
    }
}

#endif /* EPOC */

#ifdef QDOS
/****************************************************************** QDOS/SMS */
/*
 *		The QDOS operating system originated with the Sinclair QL.
 *		It has been ported to also run on the Atari ST (with hardware
 *		assist) and the Amiga.
 *
 *		There are also a number of clones of QDOS available:
 *			  SMS2	   Which runs on Atari ST
 *			  SMSQ	   Which runs on QXL (a 68040 board for a PC)
 *			  SMSQ/E   Which runs on QL's fitted with Gold Card or
 *					   Super Goldcard accelerators; Atari ST and TT;
 *					   and PCs fitted with the QXL
 *
 *		The C68 compiler forms part of a complete Public Domain C
 *		development system for all these environments.
 *
 *****************************************************************************/

#include <qdos.h>
#include <fcntl.h>
#include <sys/signal.h>
#include "version.h"

/*
 *		Tailor the start-up environment
 */
void    (*_consetup) (chanid_t, struct WINDOWDEF *) = consetup_title;
int     (*_conwrite) (struct UFB *, void *, unsigned) = NULL;
int     (*_Open) (const char *, int, ...) = qopen;

char    _prog_name[] = PROGNAME;
char    _version[] = VERSION;
char    _copyright[] = LAST_CHANGE_DATE;
char   *__SigNoMsg = NULL;
long    _stack = 20L * 1024L;
long    _stackmargin = 1024L;

#endif /* QDOS */
