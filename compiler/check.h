/*
 *             c h e c k . h
 *
 * This file provides the checks that the options selected in config.h are
 * consistent with each other.
 */

#ifndef _CHECK_H
#define	_CHECK_H

#ifdef CPU_DEFINED
#undef CPU_DEFINED
#endif /* CPU_DEFINED */

#ifdef COLDFIRE
#define MC68K
#endif /* COLDFIRE */

#ifdef MC68K
#define MC680X0
#ifdef CPU_DEFINED
#define	MULTIPLE_PROCESSORS
#endif /* CPU_DEFINED */
#define CPU_DEFINED
#endif /* MC68K */
#ifdef MC680X0
#define PROGNAME   "c68"
#define ENVNAME	   "C68"
#endif /* MC680X0 */
#ifdef  INTEL_486
#ifndef INTEL_386
#define INTEL_386
#endif /* INTEL_386 */
#endif /* INTEL_486 */
#ifdef INTEL_386
#define	INTEL
#ifdef CPU_DEFINED
#define	MULTIPLE_PROCESSORS
#endif /* CPU_DEFINED */
#ifndef CPU_DEFINED
#define CPU_DEFINED
#endif /* CPU_DEFINED */
#ifndef PROGNAME
#define PROGNAME   "c386"
#define	ENVNAME	   "C386"
#endif /* PROGNAME */
#endif /* INTEL_386 */
#ifdef INTEL_86
#ifndef INTEL
#define	INTEL
#endif /* INTEL */
#ifdef CPU_DEFINED
#ifndef MULTIPLE_PROCESSORS
#define	MULTIPLE_PROCESSORS
#endif /* MULTIPLE_PROCESSORS */
#else  /* CPU_DEFINED */
#define CPU_DEFINED
#endif /* CPU_DEFINED */
#ifndef PROGNAME
#define PROGNAME   "c86"
#define	ENVNAME	   "C86"
#endif /* PROGNAME */
#endif /* INTEL_86 */
#ifdef ARM
#ifdef CPU_DEFINED
#ifndef MULTIPLE_PROCESSORS
#define	MULTIPLE_PROCESSORS
#endif /* MULTIPLE_PROCESSORS */
#else  /* CPU_DEFINED */
#define CPU_DEFINED
#endif /* CPU_DEFINED */
#ifndef PROGNAME
#define PROGNAME "carm"
#define	ENVNAME	 "CARM"
#endif /* PROGNAME */
#endif /* ARM */
#ifdef TMS320C30
#ifdef CPU_DEFINED
#ifndef MULTIPLE_PROCESSORS
#define	MULTIPLE_PROCESSORS
#endif /* MULTIPLE_PROCESSORS */
#else  /* CPU_DEFINED */
#define CPU_DEFINED
#endif /* CPU_DEFINED */
#ifndef PROGNAME
#define	PROGNAME "cc30"
#define	ENVNAME  "C30"
#endif /* PROGNAME */
#endif /* TMS320C30 */
#ifndef CPU_DEFINED
#define	PROGNAME "cc"
#undef	ICODE
#endif /* CPU_DEFINED */
/*****************************************************************************/
/* LIST must be defined if the ICODE option is defined */
#ifdef ICODE
#undef LIST
#define LIST
#endif /* ICODE */
/*****************************************************************************/
#ifdef FLOAT_MFFP
#define FLOAT_SUPPORT
#ifdef INTEL_386
    error, Motorola Fast Floating Point format not supported on Intel
#endif /* INTEL_386 */
#ifdef FLOAT_IEEE
    error, define only one Floating Point format
#endif /* FLOAT_IEEE */
#endif /* FLOAT_MFFP */
#ifdef FLOAT_IEEE
#define FLOAT_SUPPORT
#endif /* FLOAT_IEEE */
#ifdef FLOAT_SUPPORT
#ifndef FLOAT_KEYWORDS
#define FLOAT_KEYWORDS
#endif /*FLOAT_KEYWORDS */
#endif /* FLOAT_SUPPORT */
#ifdef FLOAT_BOOTSTRAP
#define FLOAT_KEYWORDS
#endif /* FLOAT_BOOTSTRAP */
#ifndef CPU_DEFINED
/* if no CPU has been defined then we are not interested in FP code */
#undef	FLOAT_IEEE
#undef	FLOAT_MFFP
#endif	/* CPU_DEFINED */
    
/*****************************************************************************/ 
    
#ifdef LONGLONG
#define LONGLONG_SUPPORT
#else	/*  */
#ifdef LONGLONG_BOOTSTRAP
#define LONGLONG_SUPPORT
#endif	/* LONGLONG_BOOTSTRAP */
#endif	/* LONGLONG */
/*****************************************************************************/
#ifdef MC68K_DEFAULT
#define PROCESSOR_DEFINED
#ifndef MC680X0
    error, A 68000 processor has not been configured to be build
#endif /* MC680X0 */
#endif /* MC68K_DEFAULT */
#ifdef INTEL_86_DEFAULT
#define PROCESSOR_DEFINED
#ifndef INTEL_86
    error, An Intel 8086 processor has not been configured to be build
#endif /* INTEL_86 */
#endif /* INTEL_86_DEFAULT */
#ifdef INTEL_386_DEFAULT
#define PROCESSOR_DEFINED
#ifndef INTEL_386
    error, An Intel 80386 processor has not been configured to be build
#endif /* INTEL_386 */
#endif /* INTEL_386_DEFAULT */
#ifdef ARM_DEFAULT
#define PROCESSOR_DEFINED
#ifndef ARM
    error, An ARM processor has not been configured to be build
#endif /* ARM */
#endif /* ARM_DEFAULT */
#ifdef TMS320C30_DEFAULT
#define PROCESSOR_DEFINED
#ifndef TMS320C30
    error, A TI TMS320C30 processor has not been configured to be build
#endif /* TMS320C30 */
#endif /* TMS320C30_DEFAULT */
#ifndef PROCESSOR_DEFINED
#ifdef MC680X0
#define	MC68K_DEFAULT
#else
#ifdef INTEL_86
#define	INTEL_86_DEFAULT
#else
#ifdef INTEL_386
#define	INTEL_386_DEFAULT
#else
#ifdef ARM
#define	ARM_DEFAULT
#ifdef TMS320C30
#define	TMS320C30
#endif /* TMS320C30_DEFAULT */
#endif /* ARM_DEFAULT */
#endif /* INTEL_386_DEFAULT */
#endif /* INTEL_86_DEFAULT */
#endif /* MC68K_DEFAULT */
#endif /* PROCESSOR_DEFINED */
/*****************************************************************************/
#ifdef	TARGET_ACK_DEFAULT
#define	TARGET_DEFINED
#ifndef MC680X0
    error, Requires a 68000 code generator to be selected
#endif /* MC680X0 */
#endif /* TARGET_ACK_DEFAUL T */
#ifdef	TARGET_CPM_DEFAULT
#define	TARGET_DEFINED
#ifndef MC680X0
    error, Requires a 68000 code generator to be selected
#endif /* MC680X0 */
#endif /* TARGET_CPM_DEFAULT */
#ifdef	TARGET_QMAC_DEFAULT
#define	TARGET_DEFINED
#ifndef MC680X0
    error, Requires a 68000 code generator to be selected
#endif /* MC680X0 */
#endif /* TARGET_QMAC_DEFAULT */
#ifdef	TARGET_GAS_DEFAULT
#define	TARGET_DEFINED
#ifndef MC680X0
#ifndef INTEL
    error, Requires a 68000 or Intel 80 X86 code generator to be selected
#endif /* INTEL */
#endif /* MC680X0 */
#endif /* TARGET_GAS_DEFAULT */
#ifdef	TARGET_SUN_DEFAULT
#define	TARGET_DEFINED
#ifndef INTEL
    error, Requires an Intel 80 X86 code generator to be selected
#endif /* INTEL */
#endif /* TARGET_SUN_DEFAULT */
#ifdef	TARGET_BAS_DEFAULT
#define	TARGET_DEFINED
#ifndef INTEL
    error, Requires an Intel 80 X86 code generator to be selected
#endif /* INTEL */
#endif /* TARGET_BAS_DEFAULT */
#ifdef	TARGET_SYSV_DEFAULT
#define	TARGET_DEFINED
#ifndef INTEL
    error, Requires an Intel 80 X86 code generator to be selected
#endif /* INTEL */
#endif /* TARGET_SYSV_DEFAULT */
#ifdef	TARGET_MASM_DEFAULT
#define	TARGET_DEFINED
#ifndef INTEL
    error, Requires an Intel 80 X86 code generator to be selected
#endif /* INTEL */
#endif /* TARGET_MASM_DEFAULT */
#ifdef	TARGET_NASM_DEFAULT
#define	TARGET_DEFINED
#ifndef INTEL
    error, Requires an Intel 80 X86 code generator to be selected
#endif /* INTEL */
#endif /* TARGET_NASM_DEFAULT */
#ifdef	TARGET_OBJ_DEFAULT
#define	TARGET_DEFINED
#ifndef ARM
    error, Requires an ARM code generator to be selected
#endif /* ARM */
#endif /* TARGET_OBJ_DEFAULT */
#ifdef	TARGET_ROSSIN_DEFAULT
#define	TARGET_DEFINED
#ifndef TMS320C30
    error, Requires a TI TMS320C30 code generator to be selected
#endif /* TMS320C30 */
#endif /* TARGET_ROSSIN_DEFAULT */
#ifndef TARGET_DEFINED
#ifdef TARGET_ACK
#define	TARGET_ACK_DEFAULT
#else
#ifdef TARGET_CPM
#define	TARGET_CPM_DEFAULT
#else
#ifdef TARGET_QMAC
#define	TARGET_QMAC_DEFAULT
#else
#ifdef TARGET_GAS
#define	TARGET_GAS_DEFAULT
#else
#ifdef TARGET_BAS
#define	TARGET_BAS_DEFAULT
#else
#ifdef TARGET_SYSV
#define	TARGET_SYSV_DEFAULT
#else
#ifdef TARGET_MASM
#define	TARGET_MASM_DEFAULT
#else
#ifdef TARGET_MASM
#define	TARGET_NASM_DEFAULT
#else
#ifdef TARGET_OBJ
#define	TARGET_OBJ_DEFAULT
#else
#ifdef TARGET_ROSSIN
#define	TARGET_ROSSIN_DEFAULT
#endif /* TARGET_ROSSIN */
#endif /* TARGET_OBJ */
#endif /* TARGET_NASM */
#endif /* TARGET_MASM */
#endif /* TARGET_SYSV */
#endif /* TARGET_BAS */
#endif /* TARGET_GAS */
#endif /* TARGET_QMAC */
#endif /* TARGET_CPM */
#endif /* TARGET_ACK */
#endif /* TARGET_DEFINED */
/*****************************************************************************/
/* check to see whether we have multiple assemblers */
#ifdef MULTIPLE_PROCESSORS
#define	MULTIPLE_ASSEMBLERS
#endif /* MULTIPLE_PROCESSORS */
#ifdef MC680X0
#if ((defined(TARGET_ACK) + defined(TARGET_GAS) + defined(TARGET_CPM) + defined(TARGET_QMAC)) > 1)
#ifndef MULTIPLE_ASSEMBLERS
#define MULTIPLE_ASSEMBLERS
#endif /* MULTIPLE_ASSEMBLERS */
#endif
#endif /* MC680X0 */
#ifdef INTEL
#if ((defined(TARGET_BAS) + defined(TARGET_GAS) + defined(TARGET_SUN) + defined(TARGET_SYSV) + defined(TARGET_MASM) + defined(TARGET_NASM)) >1)
#ifndef MULTIPLE_ASSEMBLERS
#define MULTIPLE_ASSEMBLERS
#endif /* MULTIPLE_ASSEMBLERS */
#endif
#endif /* INTEL_386 */
#ifdef ARM
#if ((defined(TARGET_OBJ)) > 1)
#define MULTIPLE_ASSEMBLERS
#endif
#endif /* ARM */
#ifdef TMS320C30
#if ((defined(TARGET_ROSSIN)) > 1)
#define MULTIPLE_ASSEMBLERS
#endif
#endif /* TMS320C30 */
/*****************************************************************************/
/*
 *   Check to see if a code generator that has at least
 *   one appropriate assembler for that processor type has been selected
 */
#ifdef MC680X0
#if ((defined(TARGET_ACK) + defined(TARGET_GAS) + defined(TARGET_CPM) + defined(TARGET_QMAC)) == 0)
    error, You must select an assembler for the
    68000 processor
#endif
#endif /* MC680X0 */
#ifdef INTEL
#if ((defined(TARGET_BAS) + defined(TARGET_GAS) + defined(TARGET_SUN) + defined(TARGET_SYSV) + defined(TARGET_MASM) + defined(TARGET_NASM)) == 0)
	error, You must select an assembler for the
	Intel 8086 / 80386 processor
#endif
#endif /* INTEL_386 */
#ifdef ARM
#if ((defined(TARGET_OBJ)) == 0)
	    error, You must select an assembler for the
	    ARM processor
#endif
#endif /* ARM */
#ifdef TMS320C30
#if ((defined(TARGET_ROSSIN)) == 0)
		error, You must select an assembler for the
		TI TMS320C30 processor
#endif
#endif /* TMS320C30 */
/*****************************************************************************/
/*
 * Macros to allow function definitions to be either ANSI or K&R style
 */
#ifdef __STDC__
#define P0(t1)			(t1)
#define P1(t1, a1)			(t1 a1)
#define P2(t1, a1, t2, a2)		(t1 a1, t2 a2)
#define P3(t1, a1, t2, a2, t3, a3)	(t1 a1, t2 a2, t3 a3)
#define P4(t1, a1, t2, a2, t3, a3, t4, a4)	(t1 a1, t2 a2, t3 a3, t4 a4)
#define P5(t1, a1, t2, a2, t3, a3, t4, a4, t5, a5)	(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5)
#define P8(t1, a1, t2, a2, t3, a3, t4, a4, t5, a5, t6, a6, t7, a7, t8, a8)	(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6, t7 a7, t8 a8)
#else
#define P0(t1)			()
#define P1(t1, a1)			(a1) t1 a1;
#define P2(t1, a1, t2, a2)		(a1, a2) t1 a1; t2 a2;
#define P3(t1, a1, t2, a2, t3, a3)	(a1, a2, a3) t1 a1; t2 a2; t3 a3;
#define P4(t1, a1, t2, a2, t3, a3, t4, a4)	(a1, a2, a3, a4) t1 a1; t2 a2; t3 a3; t4 a4;
#define P5(t1, a1, t2, a2, t3, a3, t4, a4, t5, a5)	(a1, a2, a3, a4, a5) t1 a1; t2 a2; t3 a3; t4 a4; t5 a5;
#define P8(t1, a1, t2, a2, t3, a3, t4, a4, t5, a5, t6, a6, t7, a7, t8, a8)	(a1, a2, a3, a4, a5, a6, a7, a8) t1 a1; t2 a2; t3 a3; t4 a4; t5 a5; t6 a6; t7 a7; t8 a8;
#endif /* __STDC__ */
/*****************************************************************************/
#ifndef AL_HOST
/*
 *   Determine the worst case alignment requirements.
 */
		    union alignment_union
		{
		    char    c;
		    short   s;
		    int     i;
		    long    l;
		    void   *vp;
		    float   f;
		    double  d;
		    void    (*fn) (int);
		};

#define	AL_HOST		((SIZE)sizeof(union alignment_union))
#endif /* AL_HOST */

/*****************************************************************************/
/*
 *   The following item ensures that every file contains at least
 *   one definition.   Some compilers insist (rather than just warn)
 *   that a file contains a declaration.
 */

#ifdef NO_EMPTY_FILES
static int dummy;

#endif /* NO_EMPTY_FILES */

/*****************************************************************************/
/*
 *   If space is very tight then the following definition allows the
 *   internal consistency checks to be omitted ... note however this is
 *   generally a bad idea even in production systems.
 */

#ifdef EPOC
#define	FATAL(x)
#else
#define	FATAL(x)	fatal x
#endif /* EPOC */

/*****************************************************************************/
/*
 *   Debugging information
 */
#ifdef DWARF2
#define	DEBUGINFO
#endif /* DWARF2 */

#endif /* _CHECK_H */
