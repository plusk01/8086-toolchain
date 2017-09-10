/* The Netwide Assembler main program module
 *
 * The Netwide Assembler is copyright (C) 1996 Simon Tatham and
 * Julian Hall. All rights reserved. The software is
 * redistributable under the licence given in the file "Licence"
 * distributed in the NASM archive.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "nasm.h"
#include "nasmlib.h"
#include "insns.h"
#include "preproc.h"
#include "parser.h"
#include "eval.h"
#include "assemble.h"
#include "labels.h"
#include "outform.h"
#include "listing.h"

struct forwrefinfo {		       /* info held on forward refs. */
    int lineno;
    int operand;
};

static int get_bits (char *value);
static unsigned long get_cpu (char *cpu_str);
static void parse_cmdline (int, char **);
static void assemble_file (char *);
static int getkw (char *buf, char **value);
static void register_output_formats(void);
static void report_error_gnu (int severity, const char *fmt, ...);
static void report_error_vc (int severity, const char *fmt, ...);
static void report_error_common (int severity, const char *fmt, va_list args);
static int is_suppressed_warning (int severity);
static void usage(void);
static efunc report_error;

static int using_debug_info, opt_verbose_info;
int	   tasm_compatible_mode = FALSE;
int pass0;

static char inname[FILENAME_MAX];
static char outname[FILENAME_MAX];
static char listname[FILENAME_MAX];
static int globallineno;	       /* for forward-reference tracking */
/* static int pass = 0; */
static struct ofmt *ofmt = NULL;

static FILE *error_file;	       /* Where to write error messages */

static FILE *ofile = NULL;
//int optimizing = -1;		/* number of optimization passes to take */
int optimizing = 10;		/* Default to "-O2" option (3 passes), for EE425. -WSF	*/
static int sb, cmd_sb = 16;		       /* by default */
static unsigned long cmd_cpu = IF_PLEVEL;	/* highest level by default */
static unsigned long cpu = IF_PLEVEL;		/* passed to insn_size & assemble.c */
int global_offset_changed;             /* referenced in labels.c */

static loc_t location;
int in_abs_seg;			/* Flag we are in ABSOLUTE seg */
long abs_seg;			/* ABSOLUTE segment basis */
long abs_offset;		/* ABSOLUTE offset */

static struct RAA *offsets;

static struct SAA *forwrefs;	       /* keep track of forward references */
static struct forwrefinfo *forwref;

static Preproc *preproc;
enum op_type {
  op_normal,			/* Preprocess and assemble */
  op_preprocess,		/* Preprocess only */
  op_depend			/* Generate dependencies */
};
static enum op_type operating_mode;

/*
 * Which of the suppressible warnings are suppressed. Entry zero
 * doesn't do anything. Initial defaults are given here.
 */
static char suppressed[1+ERR_WARN_MAX] = {
  0, TRUE, TRUE, TRUE, FALSE, TRUE
};

/*
 * The option names for the suppressible warnings. As before, entry
 * zero does nothing.
 */
static const char *suppressed_names[1+ERR_WARN_MAX] = {
    NULL, "macro-params", "macro-selfref", "orphan-labels", "number-overflow",
    "gnu-elf-extensions"
};

/*
 * The explanations for the suppressible warnings. As before, entry
 * zero does nothing.
 */
static const char *suppressed_what[1+ERR_WARN_MAX] = {
    NULL,
    "macro calls with wrong no. of params",
    "cyclic macro self-references",
    "labels alone on lines without trailing `:'",
    "numeric constants greater than 0xFFFFFFFF",
    "using 8- or 16-bit relocation in ELF, a GNU extension"
};

/*
 * This is a null preprocessor which just copies lines from input
 * to output. It's used when someone explicitly requests that NASM
 * not preprocess their source file.
 */

static void no_pp_reset (char *, int, efunc, evalfunc, ListGen *);
static char *no_pp_getline (void);
static void no_pp_cleanup (int);
static Preproc no_pp = {
    no_pp_reset,
    no_pp_getline,
    no_pp_cleanup
};

/*
 * get/set current offset...
 */
#define GET_CURR_OFFS (in_abs_seg?abs_offset:\
		      raa_read(offsets,location.segment))
#define SET_CURR_OFFS(x) (in_abs_seg?(void)(abs_offset=(x)):\
			 (void)(offsets=raa_write(offsets,location.segment,(x))))

static int want_usage;
static int terminate_after_phase;
int user_nolist = 0;             /* fbk 9/2/00 */

static void nasm_fputs(const char *line, FILE *outfile)
{
    if (outfile) {
	fputs(line, outfile);
	fputc('\n', outfile);
    } else
	puts(line);
}

int main(int argc, char **argv)
{
    pass0 = 1;
    want_usage = terminate_after_phase = FALSE;
    report_error = report_error_gnu;

    nasm_set_malloc_error (report_error);
    offsets = raa_init();
    forwrefs = saa_init ((long)sizeof(struct forwrefinfo));

    preproc = &nasmpp;
    operating_mode = op_normal;

    error_file = stderr;

    seg_init();

    register_output_formats();

    parse_cmdline(argc, argv);

    if (terminate_after_phase)
    {
	if (want_usage)
	    usage();
	return 1;
    }

    if (ofmt->stdmac)
	pp_extra_stdmac (ofmt->stdmac);
    parser_global_info (ofmt, &location);
    eval_global_info (ofmt, lookup_label, &location);

    /* define some macros dependent of command-line */
    {
	char temp [64];
	sprintf (temp, "__OUTPUT_FORMAT__=%s\n", ofmt->shortname);
	pp_pre_define (temp);
    }

    switch ( operating_mode ) {
    case op_depend:
      {
	char *line;
	preproc->reset (inname, 0, report_error, evaluate, &nasmlist);
	if (outname[0] == '\0')
	  ofmt->filename (inname, outname, report_error);
	ofile = NULL;
	fprintf(stdout, "%s: %s", outname, inname);
	while ( (line = preproc->getline()) )
	  nasm_free (line);
	preproc->cleanup(0);
	putc('\n', stdout);
      }
    break;

    case op_preprocess:
    {
      char *line;
      char *file_name = NULL;
      long  prior_linnum=0;
      int   lineinc=0;

      if (*outname) {
	ofile = fopen(outname, "w");
	if (!ofile)
	  report_error (ERR_FATAL | ERR_NOFILE,
			"unable to open output file `%s'", outname);
      } else
	ofile = NULL;

      location.known = FALSE;

/*      pass = 1; */
      preproc->reset (inname, 2, report_error, evaluate, &nasmlist);
      while ( (line = preproc->getline()) ) {
	/*
	 * We generate %line directives if needed for later programs
	 */
	long linnum = prior_linnum += lineinc;
	int  altline = src_get(&linnum, &file_name);
	if (altline) {
	  if (altline==1 && lineinc==1)
	    nasm_fputs("", ofile);
	  else {
	    lineinc = (altline != -1 || lineinc!=1);
	    fprintf(ofile ? ofile : stdout, "%%line %ld+%d %s\n",
		    linnum, lineinc, file_name);
	  }
	  prior_linnum = linnum;
	}
	nasm_fputs(line, ofile);
	nasm_free (line);
      }
      nasm_free(file_name);
      preproc->cleanup(0);
      if (ofile)
	fclose(ofile);
      if (ofile && terminate_after_phase)
	remove(outname);
    }
    break;

    case op_normal:
      {
	/*
	 * We must call ofmt->filename _anyway_, even if the user
	 * has specified their own output file, because some
	 * formats (eg OBJ and COFF) use ofmt->filename to find out
	 * the name of the input file and then put that inside the
	 * file.
	 */
	ofmt->filename (inname, outname, report_error);
	
	ofile = fopen(outname, "wb");
	if (!ofile) {
	  report_error (ERR_FATAL | ERR_NOFILE,
			"unable to open output file `%s'", outname);
	}
	
	/*
	 * We must call init_labels() before ofmt->init() since
	 * some object formats will want to define labels in their
	 * init routines. (eg OS/2 defines the FLAT group)
	 */
	init_labels ();
	
	ofmt->init (ofile, report_error, define_label, evaluate);
	
	assemble_file (inname);
	
        if (!terminate_after_phase) {
	  ofmt->cleanup (using_debug_info);
	  cleanup_labels ();
	    } else {
	  /*
	   * We had an fclose on the output file here, but we
	   * actually do that in all the object file drivers as well,
	   * so we're leaving out the one here.
	   *     fclose (ofile);
	   */
	  remove(outname);
          if (listname[0])
             remove(listname);
	}
      }
    break;
    }

    if (want_usage)
      usage();

    raa_free (offsets);
    saa_free (forwrefs);
    eval_cleanup ();
    nasmlib_cleanup ();

    if (terminate_after_phase)
	return 1;
    else
	return 0;
}


/*
 * Get a parameter for a command line option.
 * First arg must be in the form of e.g. -f...
 */
static char *get_param (char *p, char *q, int *advance)
{
    *advance = 0;
    if (p[2])  		       /* the parameter's in the option */
    {
	p += 2;
	while (isspace(*p))
	    p++;
	return p;
    }
    if (q && q[0])
    {
	*advance = 1;
	return q;
    }
    report_error (ERR_NONFATAL | ERR_NOFILE | ERR_USAGE,
		  "option `-%c' requires an argument",
		  p[1]);
    return NULL;
}

struct textargs
{
	const char *label;
	int value;
};

#define OPT_PREFIX 0
#define OPT_POSTFIX 1
struct textargs textopts[] =
{
	{"prefix",OPT_PREFIX},
	{"postfix",OPT_POSTFIX},
	{NULL,0}
};


int stopoptions = 0;
static int process_arg (char *p, char *q)
{
    char *param;
    int  i, advance = 0;

    if (!p || !p[0])
	return 0;

    if (p[0]=='-' && ! stopoptions)
    {
	switch (p[1]) {
	  case 's':
	      error_file = stdout;
	      break;
	  case 'o':		       /* these parameters take values */
	  case 'O':
	  case 'f':
	  case 'p':
          case 'P':
	  case 'd':
	  case 'D':
	  case 'i':
          case 'I':
	  case 'l':
	  case 'E':
	  case 'F':
	  case 'X':
	    if ( !(param = get_param (p, q, &advance)) )
		break;
	    if (p[1]=='o') {	       /* output file */
		strcpy (outname, param);
	    } else if (p[1]=='f') {    /* output format */
		ofmt = ofmt_find(param);
		if (!ofmt) {
		    report_error (ERR_FATAL | ERR_NOFILE | ERR_USAGE,
				  "unrecognised output format `%s' - "
				  "use -hf for a list",
				  param);
		}
		else
		    ofmt->current_dfmt = ofmt->debug_formats[0];
	    } else if (p[1]=='O') {		    /* Optimization level */
		int opt;
		opt = -99;
		while (*param) {
		    if (isdigit(*param)) {
			opt = atoi(param);
			while(isdigit(*++param)) ;
			if (opt<=0) optimizing = -1;		/* 0.98 behaviour */
			else if (opt==1) optimizing = 0;	/* Two passes, 0.98.09 behavior */
			else if (opt<=3) optimizing = opt*5;	/* Multiple passes */
			else optimizing = opt;			/* Multiple passes */
		    } else {
			if (*param == 'v' || *param == '+') {
			    ++param;
			    opt_verbose_info = TRUE;
			    opt = 0;
			}
		    }
		} /* while (*param) */
		if (opt == -99) report_error(ERR_FATAL,
			"command line optimization level must be 'v', 0..3 or <nn>");
	    } else if (p[1]=='P' || p[1]=='p') {    /* pre-include */
		pp_pre_include (param);
	    } else if (p[1]=='D' || p[1]=='d') {    /* pre-define */
		pp_pre_define (param);
	    } else if (p[1]=='U' || p[1]=='u') {    /* un-define */
		pp_pre_undefine (param);
	    } else if (p[1]=='I' || p[1]=='i') {    /* include search path */
		pp_include_path (param);
	    } else if (p[1]=='l') {    /* listing file */
		strcpy (listname, param);
	    } else if (p[1]=='E') {    /* error messages file */
		error_file = fopen(param, "w");
		if ( !error_file ) {
		  error_file = stderr; /* Revert to default! */
		  report_error (ERR_FATAL | ERR_NOFILE | ERR_USAGE,
				"cannot open file `%s' for error messages",
				param);
		}
	    } else if (p[1] == 'F') {  /* specify debug format */
	        ofmt->current_dfmt = dfmt_find(ofmt, param);
	        if (!ofmt->current_dfmt) {
	            report_error (ERR_FATAL | ERR_NOFILE | ERR_USAGE,
	    	    		  "unrecognized debug format `%s' for"
			    	  " output format `%s'",
				  param, ofmt->shortname);
                }
            } else if (p[1] == 'X') { /* specify error reporting format */
		if (nasm_stricmp("vc", param) == 0)
		    report_error = report_error_vc;
		else if (nasm_stricmp("gnu", param) == 0)
		    report_error = report_error_gnu;
		else
		    report_error (ERR_FATAL | ERR_NOFILE | ERR_USAGE,
				  "unrecognized error reporting format `%s'",
				  param);
	    }
	    break;
	  case 'g':
	    using_debug_info = TRUE;
	    break;
	  case 'h':
	    printf("usage: nasm [-@ response file] [-o outfile] [-f format] "
		   "[-l listfile]\n"
		   "            [options...] [--] filename\n"
		   "    or nasm -r   for version info (obsolete)\n"
		   "    or nasm -v   for version info (preferred)\n\n"
		   "    -t          Assemble in SciTech TASM compatible mode\n"
		   "    -g          Generate debug information in selected format.\n");
	    printf("    -e          preprocess only (writes output to stdout by default)\n"
		   "    -a          don't preprocess (assemble only)\n"
		   "    -M          generate Makefile dependencies on stdout\n\n"
		   "    -E<file>    redirect error messages to file\n"
		   "    -s          redirect error messages to stdout\n\n"
		   "    -F format   select a debugging format\n\n"
		   "    -I<path>    adds a pathname to the include file path\n");
	    printf("    -O<digit>   optimize branch offsets (-O0 disables, default)\n"
		   "    -P<file>    pre-includes a file\n"
		   "    -D<macro>[=<value>] pre-defines a macro\n"
		   "    -U<macro>   undefines a macro\n"
		   "    -X<format>  specifies error reporting format (gnu or vc)\n"
		   "    -w+foo      enables warnings about foo; -w-foo disables them\n"
		   "where foo can be:\n");
	    for (i=1; i<=ERR_WARN_MAX; i++)
		printf("    %-23s %s (default %s)\n",
		       suppressed_names[i], suppressed_what[i],
		       suppressed[i] ? "off" : "on");
	    printf ("\nresponse files should contain command line parameters"
		    ", one per line.\n");
	    if (p[2] == 'f') {
		printf("\nvalid output formats for -f are"
		       " (`*' denotes default):\n");
		ofmt_list(ofmt, stdout);
	    }
	    else {
		printf ("\nFor a list of valid output formats, use -hf.\n");
		printf ("For a list of debug formats, use -f <form> -y.\n");
	    }
	    exit (0);		       /* never need usage message here */
	    break;
          case 'y':
	    printf("\nvalid debug formats for '%s' output format are"
		   " ('*' denotes default):\n",
		ofmt->shortname);
	    dfmt_list(ofmt, stdout);
	    exit(0);
	    break;
	  case 't':
	    tasm_compatible_mode = TRUE;
	    break;
	  case 'r':
	  case 'v':
	  {
	      const char *nasm_version_string =
		  "NASM version " NASM_VER " compiled on " __DATE__
#ifdef DEBUG
		  " with -DDEBUG"
#endif
		  ;
	      puts(nasm_version_string);
	      exit (0);		       /* never need usage message here */
	  }
	  break;
	  case 'e':		       /* preprocess only */
	    operating_mode = op_preprocess;
	    break;
	  case 'a':		       /* assemble only - don't preprocess */
	    preproc = &no_pp;
	    break;
	  case 'w':
	    if (p[2] != '+' && p[2] != '-') {
		report_error (ERR_NONFATAL | ERR_NOFILE | ERR_USAGE,
			      "invalid option to `-w'");
	    } else {
                for (i=1; i<=ERR_WARN_MAX; i++)
		    if (!nasm_stricmp(p+3, suppressed_names[i]))
			break;
		if (i <= ERR_WARN_MAX)
		    suppressed[i] = (p[2] == '-');
		else
		    report_error (ERR_NONFATAL | ERR_NOFILE | ERR_USAGE,
				  "invalid option to `-w'");
	    }
	    break;
          case 'M':
	    operating_mode = op_depend;
	    break;

	  case '-':
	  {
		int s;
		
		if (p[2]==0) {		/* -- => stop processing options */
		    stopoptions = 1;
		    break;
		}
		for(s=0; textopts[s].label; s++)
		{
			if(!nasm_stricmp(p+2, textopts[s].label))
			{
				break;
			}
		}

		switch(s)
		{
		
			case OPT_PREFIX:
			case OPT_POSTFIX:
			{
				if (!q)
				{
					report_error (ERR_NONFATAL | ERR_NOFILE | ERR_USAGE,
			      			"option `--%s' requires an argument",
			      			p+2);
					break;
				}
				else
				{
					advance = 1, param = q;
				}
				
				if(s == OPT_PREFIX)
				{
					strncpy(lprefix,param,PREFIX_MAX-1);
					lprefix[PREFIX_MAX-1]=0;
					break;
				}
				if(s == OPT_POSTFIX)
				{
					strncpy(lpostfix,param,POSTFIX_MAX-1);
					lpostfix[POSTFIX_MAX-1]=0;
					break;
				}
				break;
			}
			default:
			{
				report_error (ERR_NONFATAL | ERR_NOFILE | ERR_USAGE,
			  		"unrecognised option `--%s'",
			  		p+2);
				break;
			}
		}
		break;
	  }

	  default:
	    if (!ofmt->setinfo(GI_SWITCH,&p))
	    	report_error (ERR_NONFATAL | ERR_NOFILE | ERR_USAGE,
			  "unrecognised option `-%c'",
			  p[1]);
	    break;
	}
    }
    else
    {
	if (*inname) {
	    report_error (ERR_NONFATAL | ERR_NOFILE | ERR_USAGE,
			  "more than one input file specified");
	} else
	    strcpy(inname, p);
    }

    return advance;
}

#define ARG_BUF_DELTA 128

static void process_respfile (FILE *rfile)
{
    char *buffer, *p, *q, *prevarg;
    int bufsize, prevargsize;

    bufsize = prevargsize = ARG_BUF_DELTA;
    buffer = nasm_malloc(ARG_BUF_DELTA);
    prevarg = nasm_malloc(ARG_BUF_DELTA);
    prevarg[0] = '\0';

    while (1) {   /* Loop to handle all lines in file */

	p = buffer;
	while (1) {  /* Loop to handle long lines */
	    q = fgets(p, bufsize-(p-buffer), rfile);
	    if (!q)
		break;
	    p += strlen(p);
	    if (p > buffer && p[-1] == '\n')
		break;
	    if (p-buffer > bufsize-10) {
		int offset;
		offset = p - buffer;
		bufsize += ARG_BUF_DELTA;
		buffer = nasm_realloc(buffer, bufsize);
		p = buffer + offset;
	    }
	}

	if (!q && p == buffer) {
	    if (prevarg[0])
		process_arg (prevarg, NULL);
	    nasm_free (buffer);
	    nasm_free (prevarg);
	    return;
	}

	/*
	 * Play safe: remove CRs, LFs and any spurious ^Zs, if any of
	 * them are present at the end of the line.
	 */
	*(p = &buffer[strcspn(buffer, "\r\n\032")]) = '\0';

	while (p > buffer && isspace(p[-1]))
	    *--p = '\0';

	p = buffer;
	while (isspace(*p))
	    p++;

	if (process_arg (prevarg, p))
	    *p = '\0';

	if (strlen(p) > prevargsize-10) {
	    prevargsize += ARG_BUF_DELTA;
	    prevarg = nasm_realloc(prevarg, prevargsize);
	}
	strcpy (prevarg, p);
    }
}

/* Function to process args from a string of args, rather than the
 * argv array. Used by the environment variable and response file
 * processing.
 */
static void process_args (char *args) {
    char *p, *q, *arg, *prevarg;
    char separator = ' ';

    p = args;
    if (*p && *p != '-')
	separator = *p++;
    arg = NULL;
    while (*p) {
	q = p;
	while (*p && *p != separator) p++;
	while (*p == separator) *p++ = '\0';
	prevarg = arg;
	arg = q;
	if (process_arg (prevarg, arg))
	    arg = NULL;
    }
    if (arg)
	process_arg (arg, NULL);
}

static void parse_cmdline(int argc, char **argv)
{
    FILE *rfile;
    char *envreal, *envcopy=NULL, *p, *arg;

    *inname = *outname = *listname = '\0';

    /*
     * First, process the NASMENV environment variable.
     */
    envreal = getenv("NASMENV");
    arg = NULL;
    if (envreal) {
	envcopy = nasm_strdup(envreal);
	process_args(envcopy);
	nasm_free (envcopy);
    }

    /*
     * Now process the actual command line.
     */
    while (--argc)
    {
	int i;
	argv++;
	if (argv[0][0] == '@') {
	    /* We have a response file, so process this as a set of
	     * arguments like the environment variable. This allows us
	     * to have multiple arguments on a single line, which is
	     * different to the -@resp file processing below for regular
	     * NASM.
	     */
	    char *str = malloc(2048);
	    FILE *f = fopen(&argv[0][1],"r");
	    if (!str) {
		printf("out of memory");
		exit(-1);
		}
	    if (f) {
		while (fgets(str,2048,f)) {
		    process_args(str);
		}
		fclose(f);
	    }
	    free(str);
	    argc--;
	    argv++;
	}
	if (!stopoptions && argv[0][0] == '-' && argv[0][1] == '@') {
	    if ((p = get_param (argv[0], argc > 1 ? argv[1] : NULL, &i))) {
		if ((rfile = fopen(p, "r"))) {
		    process_respfile (rfile);
		    fclose(rfile);
		} else
		    report_error (ERR_NONFATAL | ERR_NOFILE | ERR_USAGE,
			    "unable to open response file `%s'", p);
	    }
	} else
	    i = process_arg (argv[0], argc > 1 ? argv[1] : NULL);
	argv += i, argc -= i;
    }

    if (!*inname)
	report_error (ERR_NONFATAL | ERR_NOFILE | ERR_USAGE,
		      "no input file specified");
}


static void assemble_file (char *fname)
{
    char   * value, * p, * q, * special, * line, debugid[80];
    insn   output_ins;
    int    i, rn_error, validid;
    long   seg, offs;
    struct tokenval tokval;
    expr   * e;
    int pass, pass_max;
    int pass_cnt = 0;		/* count actual passes */

    if (cmd_sb == 32 && cmd_cpu < IF_386)
      report_error(ERR_FATAL, "command line: "
                    "32-bit segment size requires a higher cpu");

   pass_max = (optimizing>0 ? optimizing : 0) + 2;    /* passes 1, optimizing, then 2 */
   pass0 = !(optimizing>0);		/* start at 1 if not optimizing */
   for (pass = 1; pass <= pass_max  &&  pass0 <= 2; pass++) {
      int pass1, pass2;
      ldfunc def_label;

      pass1 = pass < pass_max ? 1 : 2;  /* seq is 1, 1, 1,..., 1, 2 */
      pass2 = pass > 1 ? 2 : 1;         /* seq is 1, 2, 2,..., 2, 2 */
  /*      pass0                            seq is 0, 0, 0,..., 1, 2 */

      def_label = pass > 1 ? redefine_label : define_label;


      sb = cmd_sb;        /* set 'bits' to command line default */
      cpu = cmd_cpu;
      if (pass0 == 2) {
         if (*listname)
            nasmlist.init(listname, report_error);
      }
      in_abs_seg = FALSE;
      global_offset_changed = FALSE;      /* set by redefine_label */
      location.segment = ofmt->section(NULL, pass2, &sb);
      if (pass > 1) {
         saa_rewind (forwrefs);
         forwref = saa_rstruct (forwrefs);
         raa_free (offsets);
         offsets = raa_init();
      }
      preproc->reset(fname, pass1, report_error, evaluate, &nasmlist);
      globallineno = 0;
      if (pass == 1) location.known = TRUE;
      location.offset = offs = GET_CURR_OFFS;

      while ( (line = preproc->getline()) )
      {
         globallineno++;

         /* here we parse our directives; this is not handled by the 'real'
            * parser. */
         if ( (i = getkw (line, &value)) )
         {
               switch (i) {
               case 1:               /* [SEGMENT n] */
                  seg = ofmt->section (value, pass2, &sb);
                  if (seg == NO_SEG) {
                     report_error (pass1==1 ? ERR_NONFATAL : ERR_PANIC,
                                    "segment name `%s' not recognised",
                                    value);
                  } else {
                     in_abs_seg = FALSE;
                     location.segment = seg;
                  }
                  break;
               case 2:               /* [EXTERN label:special] */
                  if (*value == '$') value++;   /* skip initial $ if present */
                  if (pass0 == 2) {
                        q = value;
                        while (*q && *q != ':')
                           q++;
                        if (*q == ':') {
                           *q++ = '\0';
                           ofmt->symdef(value, 0L, 0L, 3, q);
                        }
                  } else if (pass == 1) {   /* pass == 1 */
                        q = value;
                        validid = TRUE;
                        if (!isidstart(*q))
                           validid = FALSE;
                        while (*q && *q != ':') {
                           if (!isidchar(*q))
                                 validid = FALSE;
                           q++;
                        }
                        if (!validid) {
                           report_error (ERR_NONFATAL,
                                          "identifier expected after EXTERN");
                           break;
                        }
                        if (*q == ':') {
                           *q++ = '\0';
                           special = q;
                        } else
                           special = NULL;
                        if (!is_extern(value)) {   /* allow re-EXTERN to be ignored */
                           int temp = pass0;
                           pass0 = 1;	/* fake pass 1 in labels.c */
                           declare_as_global (value, special, report_error);
                           define_label (value, seg_alloc(), 0L, NULL, FALSE, TRUE,
                                          ofmt, report_error);
                           pass0 = temp;
                        }
                  } /* else  pass0 == 1 */
                  break;
               case 3:               /* [BITS bits] */
                  sb = get_bits(value);
                  break;
               case 4:               /* [GLOBAL symbol:special] */
                  if (*value == '$') value++;   /* skip initial $ if present */
                  if (pass0 == 2) { /* pass 2 */
                        q = value;
                        while (*q && *q != ':')
                           q++;
                        if (*q == ':') {
                           *q++ = '\0';
                           ofmt->symdef(value, 0L, 0L, 3, q);
                        }
                  } else if (pass2 == 1) { /* pass == 1 */
                        q = value;
                        validid = TRUE;
                        if (!isidstart(*q))
                           validid = FALSE;
                        while (*q && *q != ':') {
                           if (!isidchar(*q))
                                 validid = FALSE;
                           q++;
                        }
                        if (!validid) {
                           report_error (ERR_NONFATAL,
                                          "identifier expected after GLOBAL");
                           break;
                        }
                        if (*q == ':') {
                           *q++ = '\0';
                           special = q;
                        } else
                           special = NULL;
                        declare_as_global (value, special, report_error);
                  } /* pass == 1 */
                  break;
               case 5:               /* [COMMON symbol size:special] */
                  if (*value == '$') value++;   /* skip initial $ if present */
                  if (pass0 == 1) {
                        p = value;
                        validid = TRUE;
                        if (!isidstart(*p))
                           validid = FALSE;
                        while (*p && !isspace(*p)) {
                           if (!isidchar(*p))
                                 validid = FALSE;
                           p++;
                        }
                        if (!validid) {
                           report_error (ERR_NONFATAL,
                                          "identifier expected after COMMON");
                           break;
                        }
                        if (*p) {
                           long size;

                           while (*p && isspace(*p))
                                 *p++ = '\0';
                           q = p;
                           while (*q && *q != ':')
                                 q++;
                           if (*q == ':') {
                                 *q++ = '\0';
                                 special = q;
                           } else
                                 special = NULL;
                           size = readnum (p, &rn_error);
                           if (rn_error)
                                 report_error (ERR_NONFATAL, "invalid size specified"
                                             " in COMMON declaration");
                           else
                                 define_common (value, seg_alloc(), size,
                                                special, ofmt, report_error);
                        } else
                           report_error (ERR_NONFATAL, "no size specified in"
                                          " COMMON declaration");
                  } else if (pass0 == 2) { /* pass == 2 */
                        q = value;
                        while (*q && *q != ':') {
                           if (isspace(*q))
                                 *q = '\0';
                           q++;
                        }
                        if (*q == ':') {
                           *q++ = '\0';
                           ofmt->symdef(value, 0L, 0L, 3, q);
                        }
                  }
                  break;
               case 6:                       /* [ABSOLUTE address] */
                  stdscan_reset();
                  stdscan_bufptr = value;
                  tokval.t_type = TOKEN_INVALID;
                  e = evaluate(stdscan, NULL, &tokval, NULL, pass2, report_error,
                              NULL);
                  if (e) {
                     if (!is_reloc(e))
                           report_error (pass0==1 ? ERR_NONFATAL : ERR_PANIC,
                                 "cannot use non-relocatable expression as "
                                 "ABSOLUTE address");
                     else {
			 abs_seg = reloc_seg(e);
			 abs_offset = reloc_value(e);
                     }
                  } else
                     if (pass==1) abs_offset = 0x100;/* don't go near zero in case of / */
                     else report_error (ERR_PANIC, "invalid ABSOLUTE address "
                                    "in pass two");
                  in_abs_seg = TRUE;
                  location.segment = NO_SEG;
                  break;
               case 7:    /* DEBUG       */
                  p = value;
                  q = debugid;
                  validid = TRUE;
                  if (!isidstart(*p))
                     validid = FALSE;
                  while (*p && !isspace(*p)) {
                     if (!isidchar(*p))
                           validid = FALSE;
                     *q++ = *p++;
                  }
                  *q++ = 0;
                  if (!validid) {
                     report_error (pass==1 ? ERR_NONFATAL : ERR_PANIC,
                                    "identifier expected after DEBUG");
                     break;
                  }
                  while (*p && isspace(*p)) p++;
                  if (pass==pass_max) ofmt->current_dfmt->debug_directive (debugid, p);
                  break;
               case 8:			/* [WARNING {+|-}warn-name] */
                  if (pass1 == 1) {
		     while (*value && isspace(*value))
		        value++;

                     if (*value == '+' || *value == '-') {
		        validid = (*value == '-') ? TRUE : FALSE;
		        value++;
		     } else
		        validid = FALSE;

		     for (i=1; i<=ERR_WARN_MAX; i++)
		        if (!nasm_stricmp(value, suppressed_names[i]))
			    break;
		     if (i <= ERR_WARN_MAX)
		        suppressed[i] = validid;
		     else
		        report_error (ERR_NONFATAL, "invalid warning id in WARNING directive");
	          }
		  break;
               case 9:  /* cpu */
                  cpu = get_cpu (value);
                  break;
               case 10:        /* fbk 9/2/00 */       /* [LIST {+|-}] */
                   while (*value && isspace(*value))
                       value++;

                   if (*value == '+') {
                       user_nolist = 0;
		     }
                     else {
                          if (*value == '-') {
                          user_nolist = 1;
                          }
                          else {
                              report_error (ERR_NONFATAL, "invalid parameter to \"list\" directive");
                          }
                      }
                  break;
               default:
                  if (!ofmt->directive (line+1, value, pass2))
                     report_error (pass1==1 ? ERR_NONFATAL : ERR_PANIC,
                              "unrecognised directive [%s]",
                              line+1);
               }
         }
         else         /* it isn't a directive */
         {
               parse_line (pass1, line, &output_ins,
                           report_error, evaluate,
                           def_label);

               if (!(optimizing>0) && pass == 2) {
                  if (forwref != NULL && globallineno == forwref->lineno) {
                     output_ins.forw_ref = TRUE;
                     do {
                        output_ins.oprs[forwref->operand].opflags |= OPFLAG_FORWARD;
                        forwref = saa_rstruct (forwrefs);
                     } while (forwref != NULL && forwref->lineno == globallineno);
                  } else
                     output_ins.forw_ref = FALSE;
               }


               if (!(optimizing>0) && output_ins.forw_ref)
               {
                  if (pass == 1) {
                        for(i = 0; i < output_ins.operands; i++)
                        {
                           if (output_ins.oprs[i].opflags & OPFLAG_FORWARD)
                           {
                                    struct forwrefinfo *fwinf =
                                       (struct forwrefinfo *)saa_wstruct(forwrefs);
                                 fwinf->lineno = globallineno;
                                 fwinf->operand = i;
                           }
                        }
                  } else { /* pass == 2 */
                        /*
                        * Hack to prevent phase error in the code
                        *   rol ax,x
                        *   x equ 1
                        *
                        * If the second operand is a forward reference,
                        * the UNITY property of the number 1 in that
                        * operand is cancelled. Otherwise the above
                        * sequence will cause a phase error.
                        *
                        * This hack means that the above code will
                        * generate 286+ code.
                        *
                        * The forward reference will mean that the
                        * operand will not have the UNITY property on
                        * the first pass, so the pass behaviours will
                        * be consistent.
                        */

                        if (output_ins.operands >= 2 &&
                        (output_ins.oprs[1].opflags & OPFLAG_FORWARD))
                        {
                           output_ins.oprs[1].type &= ~(ONENESS|BYTENESS);
                        }

                  } /* pass == 2 */

               } /*  forw_ref */


               if (output_ins.opcode == I_EQU) {
                     if (pass1 == 1)
                     {
                        /*
                        * Special `..' EQUs get processed in pass two,
                        * except `..@' macro-processor EQUs which are done
                        * in the normal place.
                        */
                        if (!output_ins.label)
                           report_error (ERR_NONFATAL,
                                          "EQU not preceded by label");

                        else if (output_ins.label[0] != '.' ||
                                 output_ins.label[1] != '.' ||
                                 output_ins.label[2] == '@')
                        {
                           if (output_ins.operands == 1 &&
                                 (output_ins.oprs[0].type & IMMEDIATE) &&
                                 output_ins.oprs[0].wrt == NO_SEG)
                           {
                              int isext = output_ins.oprs[0].opflags & OPFLAG_EXTERN;
                              def_label (output_ins.label,
					 output_ins.oprs[0].segment,
					 output_ins.oprs[0].offset,
					 NULL, FALSE, isext, ofmt, report_error);
                           }
                           else if (output_ins.operands == 2 &&
                                       (output_ins.oprs[0].type & IMMEDIATE) &&
                                       (output_ins.oprs[0].type & COLON) &&
                                       output_ins.oprs[0].segment == NO_SEG &&
                                       output_ins.oprs[0].wrt == NO_SEG &&
                                       (output_ins.oprs[1].type & IMMEDIATE) &&
                                       output_ins.oprs[1].segment == NO_SEG &&
                                       output_ins.oprs[1].wrt == NO_SEG)
                           {
                                 def_label (output_ins.label,
                                             output_ins.oprs[0].offset | SEG_ABS,
                                             output_ins.oprs[1].offset,
                                             NULL, FALSE, FALSE, ofmt, report_error);
                           }
                           else
                                 report_error(ERR_NONFATAL, "bad syntax for EQU");
                        }
                     } else {  /* pass == 2 */
                        /*
                        * Special `..' EQUs get processed here, except
                        * `..@' macro processor EQUs which are done above.
                        */
                        if (output_ins.label[0] == '.' &&
                           output_ins.label[1] == '.' &&
                           output_ins.label[2] != '@')
                        {
                           if (output_ins.operands == 1 &&
                                 (output_ins.oprs[0].type & IMMEDIATE)) {
                                 define_label (output_ins.label,
                                             output_ins.oprs[0].segment,
                                             output_ins.oprs[0].offset,
                                             NULL, FALSE, FALSE, ofmt, report_error);
                           }
                           else if (output_ins.operands == 2 &&
                                       (output_ins.oprs[0].type & IMMEDIATE) &&
                                       (output_ins.oprs[0].type & COLON) &&
                                       output_ins.oprs[0].segment == NO_SEG &&
                                       (output_ins.oprs[1].type & IMMEDIATE) &&
                                       output_ins.oprs[1].segment == NO_SEG)
                           {
                                 define_label (output_ins.label,
                                             output_ins.oprs[0].offset | SEG_ABS,
                                             output_ins.oprs[1].offset,
                                             NULL, FALSE, FALSE, ofmt, report_error);
                           }
                           else
                                 report_error(ERR_NONFATAL, "bad syntax for EQU");
                        }
                     }  /* pass == 2 */
               } else { /* instruction isn't an EQU */

                     if (pass1 == 1) {

                        long l = insn_size (location.segment, offs, sb, cpu,
                                          &output_ins, report_error);

                        /* if (using_debug_info)  && output_ins.opcode != -1)*/
                        if (using_debug_info)  /* fbk 03/25/01 */

                        {
                           /* this is done here so we can do debug type info */
                           long typeinfo = TYS_ELEMENTS(output_ins.operands);
                           switch (output_ins.opcode) {
                                    case I_RESB:
                                        typeinfo = TYS_ELEMENTS(output_ins.oprs[0].offset) | TY_BYTE;
                                        break;
                                    case I_RESW:
                                        typeinfo = TYS_ELEMENTS(output_ins.oprs[0].offset) | TY_WORD;
                                        break;
                                    case I_RESD:
                                        typeinfo = TYS_ELEMENTS(output_ins.oprs[0].offset) | TY_DWORD;
                                        break;
                                    case I_RESQ:
                                        typeinfo = TYS_ELEMENTS(output_ins.oprs[0].offset) | TY_QWORD;
                                        break;
                                    case I_REST:
                                        typeinfo = TYS_ELEMENTS(output_ins.oprs[0].offset) | TY_TBYTE;
                                        break;
                                    case I_DB:
                                        typeinfo |= TY_BYTE;
                                        break;
                                    case I_DW:
                                        typeinfo |= TY_WORD;
                                        break;
                                    case I_DD:
                                       if (output_ins.eops_float)
                                                typeinfo |= TY_FLOAT;
                                       else
                                                typeinfo |= TY_DWORD;
                                       break;
                                    case I_DQ:
                                       typeinfo |= TY_QWORD;
                                       break;
                                    case I_DT:
                                       typeinfo |= TY_TBYTE;
                                       break;
                                    default:
                                       typeinfo = TY_LABEL;

                           }

                           ofmt->current_dfmt->debug_typevalue(typeinfo);

                        }
                        if (l != -1) {
                           offs += l;
                           SET_CURR_OFFS (offs);
                        }
                        /*
                        * else l == -1 => invalid instruction, which will be
                        * flagged as an error on pass 2
                        */

                     } else { /* pass == 2 */
                        offs += assemble (location.segment, offs, sb, cpu,
                                       &output_ins, ofmt, report_error, &nasmlist);
                        SET_CURR_OFFS (offs);

                     }
               } /* not an EQU */
               cleanup_insn (&output_ins);
         }
         nasm_free (line);
         location.offset = offs = GET_CURR_OFFS;
      } /* end while (line = preproc->getline... */

      if (pass1==2 && global_offset_changed)
         report_error(ERR_NONFATAL, "phase error detected at end of assembly.");

      if (pass1 == 1) preproc->cleanup(1);

      if (pass1==1 && terminate_after_phase) {
         fclose(ofile);
         remove(outname);
         if (want_usage)
               usage();
         exit (1);
      }
      pass_cnt++;
      if (pass>1 && !global_offset_changed) {
   	 pass0++;
   	 if (pass0==2) pass = pass_max - 1;
      }	else if (!(optimizing>0)) pass0++;

   } /* for (pass=1; pass<=2; pass++) */

   preproc->cleanup(0);
   nasmlist.cleanup();
#if 1
   if (optimizing>0 && opt_verbose_info)	/*  -On and -Ov switches */
      fprintf(stdout,
                "info:: assembly required 1+%d+1 passes\n", pass_cnt-2);
#endif
} /* exit from assemble_file (...) */


static int getkw (char *buf, char **value)
{
    char *p, *q;

    /*  allow leading spaces or tabs */
    while (*buf==' ' || *buf=='\t')
       buf++;

    if (*buf!='[')
    	return 0;

    p = buf;

    while (*p && *p != ']') p++;

    if (!*p)
	return 0;

    q = p++;

    while (*p && *p != ';') {
	if (!isspace(*p))
	    return 0;
	p++;
    }
    q[1] = '\0';

    p = buf+1;
    while (*buf && *buf!=' ' && *buf!=']' && *buf!='\t')
    	buf++;
    if (*buf==']') {
	*buf = '\0';
	*value = buf;
    } else {
	*buf++ = '\0';
        while (isspace(*buf)) buf++;   /* beppu - skip leading whitespace */
	*value = buf;
	while (*buf!=']') buf++;
	*buf++ = '\0';
    }
#if 0
    for (q=p; *q; q++)
	*q = tolower(*q);
#endif	
    if (!nasm_stricmp(p, "segment") || !nasm_stricmp(p, "section"))
    	return 1;
    if (!nasm_stricmp(p, "extern"))
    	return 2;
    if (!nasm_stricmp(p, "bits"))
    	return 3;
    if (!nasm_stricmp(p, "global"))
    	return 4;
    if (!nasm_stricmp(p, "common"))
    	return 5;
    if (!nasm_stricmp(p, "absolute"))
    	return 6;
    if (!nasm_stricmp(p, "debug"))
	return 7;
    if (!nasm_stricmp(p, "warning"))
    	return 8;
    if (!nasm_stricmp(p, "cpu"))
	return 9;
    if (!nasm_stricmp(p, "list"))    /* fbk 9/2/00 */
        return 10;
    return -1;
}

/**
 * gnu style error reporting
 * This function prints an error message to error_file in the
 * style used by GNU. An example would be:
 * file.asm:50: error: blah blah blah
 * where file.asm is the name of the file, 50 is the line number on 
 * which the error occurs (or is detected) and "error:" is one of
 * the possible optional diagnostics -- it can be "error" or "warning"
 * or something else.  Finally the line terminates with the actual 
 * error message.
 * 
 * @param severity the severity of the warning or error 
 * @param fmt the printf style format string
 */
static void report_error_gnu (int severity, const char *fmt, ...)
{
    va_list ap;

    if (is_suppressed_warning(severity))
	return;

    if (severity & ERR_NOFILE)
	fputs ("nasm: ", error_file);
    else {
	char * currentfile = NULL;
	long lineno = 0;
	src_get (&lineno, &currentfile);
	fprintf (error_file, "%s:%ld: ", currentfile, lineno);
	nasm_free (currentfile);
    }
    va_start (ap, fmt);
    report_error_common (severity, fmt, ap);
    va_end (ap);
}

/**
 * MS style error reporting
 * This function prints an error message to error_file in the
 * style used by Visual C and some other Microsoft tools. An example 
 * would be:
 * file.asm(50) : error: blah blah blah
 * where file.asm is the name of the file, 50 is the line number on 
 * which the error occurs (or is detected) and "error:" is one of
 * the possible optional diagnostics -- it can be "error" or "warning"
 * or something else.  Finally the line terminates with the actual 
 * error message.
 * 
 * @param severity the severity of the warning or error 
 * @param fmt the printf style format string
 */
static void report_error_vc (int severity, const char *fmt, ...)
{
    va_list ap;

    if (is_suppressed_warning (severity))
	return;

    if (severity & ERR_NOFILE)
	fputs ("nasm: ", error_file);
    else {
	char * currentfile = NULL;
	long lineno = 0;
	src_get (&lineno, &currentfile);
	fprintf (error_file, "%s(%ld) : ", currentfile, lineno);
	nasm_free (currentfile);
    }
    va_start (ap, fmt);
    report_error_common (severity, fmt, ap);
    va_end (ap);
}

/**
 * check for supressed warning
 * checks for suppressed warning or pass one only warning and we're 
 * not in pass 1
 *
 * @param severity the severity of the warning or error
 * @return true if we should abort error/warning printing
 */
static int is_suppressed_warning (int severity)
{
    /*
     * See if it's a suppressed warning.
     */
    return ((severity & ERR_MASK) == ERR_WARNING &&
	(severity & ERR_WARN_MASK) != 0 &&
	suppressed[ (severity & ERR_WARN_MASK) >> ERR_WARN_SHR ]) ||
    /*
     * See if it's a pass-one only warning and we're not in pass one.
     */
	((severity & ERR_PASS1) && pass0 == 2);
}

/**
 * common error reporting
 * This is the common back end of the error reporting schemes currently
 * implemented.  It prints the nature of the warning and then the 
 * specific error message to error_file and may or may not return.  It
 * doesn't return if the error severity is a "panic" or "debug" type.
 * 
 * @param severity the severity of the warning or error 
 * @param fmt the printf style format string
 */
static void report_error_common (int severity, const char *fmt, va_list args)
{
    switch (severity & ERR_MASK) {
      case ERR_WARNING:
	fputs ("warning: ", error_file); break;
      case ERR_NONFATAL:
	fputs ("error: ", error_file); break;
      case ERR_FATAL:
	fputs ("fatal: ", error_file); break;
      case ERR_PANIC:
	fputs ("panic: ", error_file); break;
      case ERR_DEBUG:
        fputs("debug: ", error_file); break;
    }

    vfprintf (error_file, fmt, args);
    fputc ('\n', error_file);

    if (severity & ERR_USAGE)
	want_usage = TRUE;

    switch (severity & ERR_MASK) {
      case ERR_WARNING: case ERR_DEBUG:
	/* no further action, by definition */
	break;
      case ERR_NONFATAL:
	/* hack enables listing(!) on errors */
        terminate_after_phase = TRUE;
	break;
      case ERR_FATAL:
	if (ofile) {
	    fclose(ofile);
	    remove(outname);
	}
	if (want_usage)
	    usage();
	exit(1);		       /* instantly die */
	break;			       /* placate silly compilers */
      case ERR_PANIC:
	fflush(NULL);
/*	abort();	*/	       /* halt, catch fire, and dump core */
	exit(3);
	break;
    }
}

static void usage(void)
{
    fputs("type `nasm -h' for help\n", error_file);
}

static void register_output_formats(void)
{
    ofmt = ofmt_register (report_error);
}

#define BUF_DELTA 512

static FILE *no_pp_fp;
static efunc no_pp_err;
static ListGen *no_pp_list;
static long no_pp_lineinc;

static void no_pp_reset (char *file, int pass, efunc error, evalfunc eval,
			 ListGen *listgen)
{
    src_set_fname(nasm_strdup(file));
    src_set_linnum(0);
    no_pp_lineinc = 1;
    no_pp_err = error;
    no_pp_fp = fopen(file, "r");
    if (!no_pp_fp)
	no_pp_err (ERR_FATAL | ERR_NOFILE,
		   "unable to open input file `%s'", file);
    no_pp_list = listgen;
    (void) pass;		       /* placate compilers */
    (void) eval;		       /* placate compilers */
}

static char *no_pp_getline (void)
{
    char *buffer, *p, *q;
    int bufsize;

    bufsize = BUF_DELTA;
    buffer = nasm_malloc(BUF_DELTA);
    src_set_linnum(src_get_linnum() + no_pp_lineinc);

    while (1) {   /* Loop to handle %line */

	p = buffer;
	while (1) {  /* Loop to handle long lines */
	    q = fgets(p, bufsize-(p-buffer), no_pp_fp);
	    if (!q)
		break;
	    p += strlen(p);
	    if (p > buffer && p[-1] == '\n')
		break;
	    if (p-buffer > bufsize-10) {
		int offset;
		offset = p - buffer;
		bufsize += BUF_DELTA;
		buffer = nasm_realloc(buffer, bufsize);
		p = buffer + offset;
	    }
	}

	if (!q && p == buffer) {
	    nasm_free (buffer);
	    return NULL;
	}

	/*
	 * Play safe: remove CRs, LFs and any spurious ^Zs, if any of
	 * them are present at the end of the line.
	 */
	buffer[strcspn(buffer, "\r\n\032")] = '\0';

	if (!strncmp(buffer, "%line", 5)) {
	    long ln;
	    int  li;
	    char *nm = nasm_malloc(strlen(buffer));
	    if (sscanf(buffer+5, "%ld+%d %s", &ln, &li, nm) == 3) {
		nasm_free( src_set_fname(nm) );
		src_set_linnum(ln);
		no_pp_lineinc = li;
		continue;
	    }
	    nasm_free(nm);
	}
	break;
    }

    no_pp_list->line (LIST_READ, buffer);

    return buffer;
}

static void no_pp_cleanup (int pass)
{
    fclose(no_pp_fp);
}

static unsigned long get_cpu (char *value)
{

    if (!strcmp(value, "8086")) return IF_8086;
    if (!strcmp(value, "186")) return IF_186;
    if (!strcmp(value, "286")) return IF_286;
    if (!strcmp(value, "386")) return IF_386;
    if (!strcmp(value, "486")) return IF_486;
    if (!strcmp(value, "586")    ||
    	!nasm_stricmp(value, "pentium") ) 	return IF_PENT;
    if (!strcmp(value, "686")  ||
    	!nasm_stricmp(value, "ppro") ||
    	!nasm_stricmp(value, "pentiumpro") ||
    	!nasm_stricmp(value, "p2")    ) 	return IF_P6;
    if (!nasm_stricmp(value, "p3")    ||
    	!nasm_stricmp(value, "katmai") ) 	return IF_KATMAI;
    if (!nasm_stricmp(value, "p4")    ||	/* is this right? -- jrc */
    	!nasm_stricmp(value, "willamette") ) 	return IF_WILLAMETTE;
    if (!nasm_stricmp(value, "ia64") ||
	!nasm_stricmp(value, "ia-64") ||
	!nasm_stricmp(value, "itanium") ||
	!nasm_stricmp(value, "itanic") ||
	!nasm_stricmp(value, "merced") )	return IF_IA64;

    report_error (pass0<2 ? ERR_NONFATAL : ERR_FATAL, "unknown 'cpu' type");

    return IF_PLEVEL;	/* the maximum level */
}


static int get_bits (char *value)
{
    int i;

    if ((i = atoi(value)) == 16)  return i;   /* set for a 16-bit segment */
    else if (i == 32) {
	if (cpu < IF_386) {
	    report_error(ERR_NONFATAL,
	    	"cannot specify 32-bit segment on processor below a 386");
	    i = 16;
	}
    } else {
	report_error(pass0<2 ? ERR_NONFATAL : ERR_FATAL,
	   "`%s' is not a valid segment size; must be 16 or 32",
	   value);
	i = 16;
    }
    return i;
}

/* end of nasm.c */
