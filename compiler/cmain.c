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
 *		  originally by M. Brandt
 * 1990   68000 C compiler further bug fixes
 *		  started i386 port (December)
 * 1991   i386 port finished (January)
 *		  further corrections in the front end and in the 68000
 *		  code generator.
 *		  The next port will be a SPARC port
 */

/*****************************************************************************/

#include "chdr.h"
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"
#include "version.h"

#ifdef SIGNAL
#include <signal.h>
#ifndef _NSIG
#ifdef NSIG
#define _NSIG	NSIG
#endif /* NSIG */
#endif /* _NSIG */
#endif /* SIGNAL */

#ifdef __MWERKS__
#include <console.h>
#include <sioux.h>
#endif /* __MWERKS__ */

/*********************************************** Static Function Definitions */

#ifdef _NSIG
static void exception P_ ((int));

#endif /* _NSIG */
static void help_option P_ ((BOOL, OPTION *, const CHAR *));
static void uniq_option P_ ((BOOL, OPTION *, const CHAR *));

/********************************************************** Static Variables */

#ifdef DEBUG
static OPTSET debugopts[] = {
    {(const CHAR *) "global", MEMBER (DEBUG_GLOBAL)},
    {(const CHAR *) "peep", MEMBER (DEBUG_PEEP)},
    {(const CHAR *) "expr", MEMBER (DEBUG_EXPR)},
    {(const CHAR *) "code", MEMBER (DEBUG_CODE)},
    {(const CHAR *) "register", MEMBER (DEBUG_REGISTER)},
    {(const CHAR *) "symbol", MEMBER (DEBUG_SYMBOL)},
    {(const CHAR *) "flow", MEMBER (DEBUG_FLOW)},
    {(const CHAR *) "longlong", MEMBER (DEBUG_LONGLONG)}, 
	{(const CHAR *) "all", (SETVAL) ~ 0U},
    {(const CHAR *) NULL, (SETVAL) 0}
};

#endif /* DEBUG */

static OPTENUM intopts[] = {
    {(const CHAR *) "16", 1},
    {(const CHAR *) "32", 0},
    {(const CHAR *) NULL, 0}
};

static OPTENUM langopts[] = {
    {(const CHAR *) "kandr", LANG_KANDR},
    {(const CHAR *) "c90", LANG_C90},
    {(const CHAR *) "c99", LANG_C99},
    {(const CHAR *) NULL, 0}
};

static OPTENUM yesnoopts[] = {
    {(const CHAR *) "yes", 1},
    {(const CHAR *) "no", 0},
    {(const CHAR *) NULL, 0}
};

static OPTION opts[] = {
    /*
     *               Options which affect the front-end
     */
    {
     (const CHAR *) "h", help_option,       /* Changed from "?" to "h" for Unix compatibility - WSF */
     {NULL},		    /*lint !e708*/  /* union initialization */
     {NULL}		    /*lint !e708*/  /* union initialization */
     },
    {
     (const CHAR *) "v", uniq_option,
     {&verbose_option},	    /*lint !e708*/  /* union initialization */
     {NULL}		    /*lint !e708*/  /* union initialization */
     },
#ifdef DEBUG
    {
     (const CHAR *) "debug=", set_option,
     {&internal_option},    /*lint !e708*/  /* union initialization */
     {(void *)&debugopts[0]}/*lint !e708*/  /* union initialization */
     },
#endif /* DEBUG */
    {
     (const CHAR *) "error=", numeric_option,
     {&error_option},	    /*lint !e708*/  /* union initialization */
     {NULL}		    /*lint !e708*/  /* union initialization */
     },
    {
     (const CHAR *) "warn=", numeric_option,
     {&warn_option},	    /*lint !e708*/  /* union initialization */
     {NULL}		    /*lint !e708*/  /* union initialization */
     },
    {
     (const CHAR *) "maxerr=", numeric_option,
     {&max_error_count},    /*lint !e708*/  /* union initialization */
     {NULL}		    /*lint !e708*/  /* union initialization */
     },
    {
     (const CHAR *) "align=", enumeration_option,
     {&align_option},	    /*lint !e708*/  /* union initialization */
     {&yesnoopts[0]}	    /*lint !e708*/  /* union initialization */
     },
#ifdef ASM
    {
     (const CHAR *) "asm=", enumeration_option,
     {&asm_option},	    /*lint !e708*/  /* union initialization */
     {&yesnoopts[0]}	    /*lint !e708*/  /* union initialization */
     },
#endif /* ASM */
#ifdef EXTERNAL
    {
     (const CHAR *) "extern=", enumeration_option,
     {&extern_option},	    /*lint !e708*/  /* union initialization */
     {&yesnoopts[0]}	    /*lint !e708*/  /* union initialization */
     },
#endif /* EXTERNAL */
#ifdef FLOAT_CHECK
    {
     (const CHAR *) "fcheck=", enumeration_option,
     {&fcheck_option},	    /*lint !e708*/  /* union initialization */
     {&yesnoopts[0]}	    /*lint !e708*/  /* union initialization */
     },
#endif /* FLOAT_CHECK */
#ifdef FORMAT_CHECK
    {
     (const CHAR *) "format=", enumeration_option,
     {&format_option},	    /*lint !e708*/  /* union initialization */
     {&yesnoopts[0]}	    /*lint !e708*/  /* union initialization */
     },
#endif /* FORMAT_CHECK */
#ifdef ICODE
    {
     (const CHAR *) "icode", uniq_option,
     {&icode_option},	    /*lint !e708*/  /* union initialization */
     {NULL}		    /*lint !e708*/  /* union initialization */
     },
#endif /* ICODE */
#ifdef DOINLINE
    {
     (const CHAR *) "inline=", numeric_option,
     {&inline_option},	    /*lint !e708*/  /* union initialization */
     {NULL}		    /*lint !e708*/  /* union initialization */
     },
#endif /* DOINLINE */
    {
     (const CHAR *) "int=", enumeration_option,
     {&short_option},	    /*lint !e708*/  /* union initialization */
     {&intopts[0]}	    /*lint !e708*/  /* union initialization */
     },
    {
     (const CHAR *) "lang=", enumeration_option,
     {&lang_option},	    /*lint !e708*/  /* union initialization */
     {&langopts[0]}	    /*lint !e708*/  /* union initialization */
     },
    {
     (const CHAR *) "lattice=", enumeration_option,
     {&lattice_option},	    /*lint !e708*/  /* union initialization */
     {&yesnoopts[0]}	    /*lint !e708*/  /* union initialization */
     },
#ifdef LIST
    {
     (const CHAR *) "list=", enumeration_option,
     {&listing_option},	    /*lint !e708*/  /* union initialization */
     {&yesnoopts[0]}	    /*lint !e708*/  /* union initialization */
     },
#endif /* LIST */
    {
     (const CHAR *) "obsolete=", enumeration_option,
     {&obsolete_option},    /*lint !e708*/  /* union initialization */
     {&yesnoopts[0]}	    /*lint !e708*/  /* union initialization */
     },
#ifdef PACKENUM
    {
     (const CHAR *) "packenum=", enumeration_option,
     {&packenum_option},    /*lint !e708*/  /* union initialization */
     {&yesnoopts[0]}	    /*lint !e708*/  /* union initialization */
     },
#endif	/* PACKENUM */
    {
     (const CHAR *) "packbit=", enumeration_option, 
     {&packbit_option},     /*lint !e708*/  /* union initialization */
     {&yesnoopts[0]}	    /*lint !e708*/  /* union initialization */
     },
    {
     (const CHAR *) "promoteparam=", enumeration_option,
     {&promoteparam_option},/*lint !e708*/  /* union initialization */
     {&yesnoopts[0]}	    /*lint !e708*/  /* union initialization */
     },
    {
     (const CHAR *) "revbit=", enumeration_option,
     {&bitfield_option},    /*lint !e708*/  /* union initialization */
     {&yesnoopts[0]}	    /*lint !e708*/  /* union initialization */
     },
#ifdef TOPSPEED
    {
     (const CHAR *) "topspeed=", enumeration_option,
     {&topspeed_option},    /*lint !e708*/  /* union initialization */
     {&yesnoopts[0]}	    /*lint !e708*/  /* union initialization */
     },
#endif /* TOPSPEED */
    {
     (const CHAR *) "trad=", enumeration_option,
     {&trad_option},	    /*lint !e708*/  /* union initialization */
     {&yesnoopts[0]}	    /*lint !e708*/  /* union initialization */
     },
    {
     (const CHAR *) "uchar=", enumeration_option,
     {&uchar_option},	    /*lint !e708*/  /* union initialization */
     {&yesnoopts[0]}	    /*lint !e708*/  /* union initialization */
     },

    /*
     *               Options which affect the code generation
     */
#ifdef DEBUGOPT
    {
     (const CHAR *) "g", uniq_option,
     {&debug_option},	    /*lint !e708*/  /* union initialization */
     {NULL}		    /*lint !e708*/  /* union initialization */
     },
#endif /*DEBUGOPT */
    {
     (const CHAR *) "O", uniq_option,
     {&optimize_option},    /*lint !e708*/  /* union initialization */
     {NULL}		    /*lint !e708*/  /* union initialization */
     },
    {
     (const CHAR *) "code=", enumeration_option,
     {&code_option},	    /*lint !e708*/  /* union initialization */
     {&yesnoopts[0]}	    /*lint !e708*/  /* union initialization */
     },
    {
     (const CHAR *) "longdouble=", enumeration_option,
     {&longdouble_option},	    /*lint !e708*/  /* union initialization */
     {&yesnoopts[0]}	    /*lint !e708*/  /* union initialization */
     },
    {
     (const CHAR *) "opt=", enumeration_option,
     {&opt_option},	    /*lint !e708*/  /* union initialization */
     {&yesnoopts[0]}	    /*lint !e708*/  /* union initialization */
     },
    {
     (const CHAR *) "pointer=", enumeration_option,
     {&small_option},	    /*lint !e708*/  /* union initialization */
     {&intopts[0]}	    /*lint !e708*/  /* union initialization */
     },
    {
     (const CHAR *) "separate=", enumeration_option,
     {&IandD_option},	    /*lint !e708*/  /* union initialization */
     {&yesnoopts[0]}	    /*lint !e708*/  /* union initialization */
     },
#ifdef TRACE
    {
     (const CHAR *) "trace=", enumeration_option,
     {&trace_option},	    /*lint !e708*/  /* union initialization */
     {&yesnoopts[0]}	    /*lint !e708*/  /* union initialization */
     },
#endif /* TRACE */
    {
     NULL, NULL,
     {NULL},		    /*lint !e708*/  /* union initialization */
     {NULL}		    /*lint !e708*/  /* union initialization */
     }
};

static OPTIONS optsstd = {
    (const CHAR *) "Common ",
    opts
};

static OPTIONS *optsarray[] = {
    &optsstd,
#ifdef MC680X0
    &opts68k,
#endif				/* MC680X0 */
#ifdef INTEL_386
    &opts386,
#endif				/* INTEL_386 */
#ifdef INTEL_86
    &opts86,
#endif				/* INTEL_86 */
#ifdef TMS320C30
    &optsc30,
#endif				/* TMS320C30 */
#ifdef ARM
    &optsarm,
#endif				/* ARM */
    (OPTIONS *) NULL
};

/*****************************************************************************/

/*ARGSUSED2 */
static void uniq_option P3 (BOOL, set, OPTION *, optptr, const CHAR *, arg)
{
    arg = arg;			/* keep compiler quiet */
    if (set) {
	*(optptr->u1.ip) = 1;
    } else {
	if (*(optptr->u1.ip)) {
	    eprintf ("-%s", optptr->text);
	} else {
	    eprintf ("\t");
	}
	eprintf ("\t\t-%s%s", optptr->text, newline);
    }
}

void numeric_option P3 (BOOL, set, const OPTION *, optptr, const CHAR *, arg)
{
    if (set) {
	switch (*arg) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	    *(optptr->u1.ip) = atoi ((char *) arg);
	    break;
	default:
	    break;
	}
    } else {
	eprintf ("-%s%d\t\t-%sn%s",
		 optptr->text, *(optptr->u1.ip), optptr->text, newline);
    }
}

void string_option P3 (BOOL, set, const OPTION *, optptr, const CHAR *, arg)
{
    if (set) {
	*(optptr->u1.sp) = (const char *) arg;
    } else {
	eprintf ("-%s%s\t-%sstr%s",
		 optptr->text, *(optptr->u1.sp), optptr->text, newline);
    }
}

void enumeration_option P3 (BOOL, set, const OPTION *, optptr, const CHAR *, arg)
{
    OPTENUM *optenums;
    const CHAR *seperator;
    int     value;
    size_t  len;

    if (set) {
	len = strlen ((char *) arg);
	for (optenums = optptr->u2.ep; optenums->text; optenums++) {
	    if (memcmp (optenums->text, arg, len) == 0) {
		*(optptr->u1.ip) = optenums->val;
		return;
	    }
	}
#if 0
	message (MSG_ENUMOPT, arg, optptr->text);
#endif
    } else {
	seperator = (const CHAR *) "";
	eprintf ("-%s", optptr->text);
	for (value = 0, optenums = optptr->u2.ep; optenums->text;
	     value++, optenums++) {
	    if (optenums->val == *(optptr->u1.ip)) {
		eprintf ("%s", (optptr->u2.ep)[value].text);
	    }
	}
	eprintf ("\t\t-%s", optptr->text);
	for (optenums = optptr->u2.ep; optenums->text; optenums++) {
	    eprintf ("%s%s", seperator, optenums->text);
	    seperator = (const CHAR *) "|";
	}
	eprintf ("%s", newline);
    }
}

/*ARGSUSED2 */
void set_option P3 (BOOL, set, OPTION *, optptr, const CHAR *, arg)
{
    OPTSET *optset;
    const CHAR *seperator;
    SETVAL  value;
    size_t  len;

    if (set) {
	value = (SETVAL) 0;
	while (*arg) {
	    for (optset = optptr->u2.sp; optset->text; optset++) {
		len = strlen ((const char *) optset->text);
		if (memcmp (optset->text, arg, len) == 0) {
		    value |= optset->val;
		    if (arg[len] == ',') {
			arg++;
		    }
		    arg += len;
		    break;
		}
	    }
	    if (optset->text == NULL) {
		break;
	    }
	}
	*(optptr->u1.ulp) = value;
    } else {
	eprintf ("-%s", optptr->text);
	seperator = (const CHAR *) "";
	for (value = (unsigned long) 0, optset = optptr->u2.sp; optset->text;
	     value++, optset++) {
	    if ((pwrof2 ((IVAL) (optset->val & *(optptr->u1.ulp))) != -1)
		&& (pwrof2 ((IVAL) optset->val) != -1)) {
		eprintf ("%s%s", seperator, (optptr->u2.sp)[value].text);
		seperator = (const CHAR *) ",";
	    }
	}
	seperator = (const CHAR *) "";
	eprintf ("\t\t-%s{", optptr->text);
	for (optset = optptr->u2.sp; optset->text; optset++) {
	    eprintf ("%s%s", seperator, optset->text);
	    seperator = (const CHAR *) "|";
	}
	eprintf ("}%s", newline);
    }
}

/*ARGSUSED2 */
void list_option P3 (BOOL, set, OPTION *, optptr, const CHAR *, arg)
{
    (void) arg;
    if (!set) {
	eprintf ("-%s", optptr->text);
	eprintf ("%s", newline);
    }
}

void chip_option P3 (BOOL, set, const OPTION *, optptr, const CHAR *, arg)
{
    arg = arg;			/* keep compiler quiet */
    if (set) {
#ifdef MULTIPLE_ASSEMBLERS
	Funcs = (struct funcs *) optptr->u1.value;
#endif /* MULTIPLE_ASSEMBLERS */
#ifdef MULTIPLE_PROCESSORS
	GFuncs = (struct genfuncs *) optptr->u2.value2;
#endif /* MULTIPLE_PROCESSORS */
    } else {
	eprintf ("\t\t\t-%s%s", optptr->text, newline);
    }
}

/*ARGSUSED2 */
static void help_option P3 (BOOL, set, OPTION *, optptr, const CHAR *, arg)
{
    OPTIONS **optpptr;

    arg = arg;			/* keep compiler quiet */
    if (set) {
	message (MSG_USAGE, PROGNAME);
	for (optpptr = optsarray; *optpptr; optpptr++) {
	    message (MSG_OPTPHASE, (*optpptr)->text);
	    for (optptr = (*optpptr)->opts; optptr->text; optptr++) {
		optptr->cmd (FALSE, optptr, (const CHAR *) NULL);
	    }
	}
	eprintf ("%s", newline);
	exit (EXIT_FAILURE);
    }
}

/*
 *	Process a parameter option
 *
 *	For convenience with front-end CC programs, any of the
 *	parameters can optionally be preceded by 'Q'.
 */
void options P2 (const CHAR *, arg, BOOL, is_pragma)
{
    OPTIONS **optpptr;
    OPTION *optptr;
    const CHAR *s = arg;
    BOOL    done = FALSE;

    s++;			/* forget '-' */
    if (*s == (CHAR) 'Q') {
	s++;			/* forget 'Q' */
    }
    /*
     * Search the table to try and find a match
     */
    for (optpptr = optsarray; *optpptr; optpptr++) {
	for (optptr = (*optpptr)->opts; optptr->text; optptr++) {
	    size_t  len = strlen ((const char *) optptr->text);

	    if ((memcmp (optptr->text, s, len) == 0) &&
		((s[len - 1] == '=') || (s[len] == 0))) {
		optptr->cmd (TRUE, optptr, &s[(int) len]);
		done = TRUE;
		break;
	    }
	}
    }
    if (!done && !is_pragma) {
	message (MSG_UNKNOWNOPT, arg);
    }
}

/*
 *	 Parse the command line
 */
static void commandline P2 (int, argc, char **, argv)
{
    int     i;
    char  **pptr;

#ifdef ENVVAR
    char   *ptr, *p;

#endif

#ifdef EPOC
    CommandLineParameters (&argc, &argv);
#else
    /* set up default files */
    input = stdin;
    output = stdout;
    errfile = stderr;
#endif /* EPOC */

#ifdef ENVVAR
    /* Accept parameters from Environment */
    if ((ptr = getenv (ENVNAME)) != NULL) {
	for (p = ptr; (ptr = strtok (p, " \t")) != NULL; p = NULL) {
	    options ((const CHAR *) ptr, FALSE);
	}
    }
#endif /* ENVAR */

    argc--;
    argv++;
    /* Accept parameters from the command line */
    for (i = 0, pptr = argv; i < argc;) {
	if (**pptr == (char) '-') {
	    char  **pp;
	    int     count;

	    options ((const CHAR *) *pptr, FALSE);
	    argc--;
	    for (pp = pptr, count = argc - i; count > 0; count--, pp++) {
		pp[0] = pp[1];
	    }
	} else {
	    i++;
	    pptr++;
	}
    }
    openfiles (argc, argv);
#ifdef LIST
    listfile = errfile;
#endif /* LIST */
#ifdef DEBUG
    debugfile = errfile;
#endif /* DEBUG */
    if (trad_option) {
	lang_option = LANG_KANDR;
    }
}

#ifdef _NSIG
static void exception P1 (int, sig)
{
    message (MSG_SIGNAL, sig);
    exit (EXIT_FAILURE);
}

#endif /* _NSIG */

int main P2 (int, argc, char **, argv)
{
#ifdef _NSIG
    int     i;

    for (i = 1; i < _NSIG; ++i)
	if (signal (i, SIG_IGN) != SIG_IGN) {
	    VOIDCAST signal (i, exception);
	}
#endif /* _NSIG */

    openerror ();
#ifdef __MWERKS__
    argc = ccommand (&argv);
#endif /* __MWERKS__ */
    commandline (argc, argv);

    if (verbose_option) {
	eprintf ("%s v%s (%s)%s", PROGNAME, VERSION, LAST_CHANGE_DATE,
		 newline);
    }
    /*
     * set and check some global options
     */
    if (lang_option == LANG_KANDR) {
	obsolete_option = 0;
    }
#ifdef FLOAT_SUPPORT
#ifndef FLOAT_BOOTSTRAP
    {
	/* set up floating point constants used within the compiler */
	int     j;

	j = 0;
	ITOF (F_zero, j);
	j = 1;
	ITOF (F_one, j);
	j = 2;
	ITOF (F_two, j);
	j = 10;
	ITOF (F_ten, j);
	j = 16;
	ITOF (F_sixteen, j);
	FASSIGN (F_half, F_one);
	FDIV (F_half, F_two);
    }
#endif /* FLOAT_BOOTSTRAP */
#endif /* FLOAT_SUPPORT */

    initialize_types ();
    initsym ();
    getsym ();
#ifdef CPU_DEFINED
    g_initialize ();
#endif /* CPU_DEFINED */
    translation_unit ();
#ifdef CPU_DEFINED
    g_terminate ();
#endif /* CPU_DEFINED */
    endsym ();
    rel_global ();
#ifdef VERBOSE
    if (verbose_option) {
	message (MSG_TIMES, decl_time, parse_time, opt_time, gen_time);
	message (MSG_ERRORCNT, total_errors);
    }
#endif
        return (total_errors > 0 ? EXIT_FAILURE : EXIT_SUCCESS);
}
