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

#ifndef _GEN386_H
#define _GEN386_H

#include "genx86.h"

/* support routines */
#define	SUP_ASMUL	(const CHAR *)".asmul"
#define	SUP_ASDIV	(const CHAR *)".asdiv"
#define	SUP_FPADD	(const CHAR *)".fpadd"
#define	SUP_FPCMP	(const CHAR *)".fpcmp"
#define	SUP_FPDEC	(const CHAR *)".fpdec"
#define	SUP_FPDIV	(const CHAR *)".fpdiv"
#define	SUP_FPFADD	(const CHAR *)".fpfadd"
#define	SUP_FPFDEC	(const CHAR *)".fpfdec"
#define	SUP_FPFDIV	(const CHAR *)".fpfdiv"
#define	SUP_FPFINC	(const CHAR *)".fpfinc"
#define	SUP_FPFMULT	(const CHAR *)".fpfmult"
#define	SUP_FPFNEG	(const CHAR *)".fpfneg"
#define	SUP_FPFSUB	(const CHAR *)".fpfsub"
#define	SUP_FPFTST	(const CHAR *)".fpftst"
#define	SUP_FPGETSTK386	(const CHAR *)".fpgetstk386"
#define	SUP_FPGETSTK387	(const CHAR *)".fpgetstk387"
#define	SUP_FPINC	(const CHAR *)".fpinc"
#define	SUP_FPLDI	(const CHAR *)".fpldi"
#define	SUP_FPLDL	(const CHAR *)".fpldl"
#define	SUP_FPLDS	(const CHAR *)".fplds"
#define	SUP_FPLDU	(const CHAR *)".fpdlu"
#define	SUP_FPLADD	(const CHAR *)".fpladd"
#define	SUP_FPLDEC	(const CHAR *)".fpldec"
#define	SUP_FPLDIV	(const CHAR *)".fpldiv"
#define	SUP_FPLINC	(const CHAR *)".fplinc"
#define	SUP_FPLMULT	(const CHAR *)".fplmult"
#define	SUP_FPLNEG	(const CHAR *)".fplneg"
#define	SUP_FPLSUB	(const CHAR *)".fplsub"
#define	SUP_FPLTST	(const CHAR *)".fpltst"
#define	SUP_FPMULT	(const CHAR *)".fpmult"
#define	SUP_FPNEG	(const CHAR *)".fpneg"
#define	SUP_FPPOP	(const CHAR *)".fppop"
#define	SUP_FPPUTSTK386	(const CHAR *)".fpputstk386"
#define	SUP_FPPUTSTK387	(const CHAR *)".fpputstk387"
#define	SUP_FPSTI	(const CHAR *)".fpsti"
#define	SUP_FPSTL	(const CHAR *)".fpstl"
#define	SUP_FPSTPL	(const CHAR *)".fpstpl"
#define	SUP_FPSTPS	(const CHAR *)".fpstps"
#define	SUP_FPSTS	(const CHAR *)".fpsts"
#define	SUP_FPSTU	(const CHAR *)".fpstu"
#define	SUP_FPSUB	(const CHAR *)".fpsub"
#define	SUP_FPTST	(const CHAR *)".fptst"
#define	SUP__FLDI	(const CHAR *)"._fldi"
#define	SUP__FLDU	(const CHAR *)"._fldu"
#define	SUP__FSTI	(const CHAR *)"._fsti"
#define	SUP__FSTU	(const CHAR *)"._fstu"
#define	SUP_STACKCHECK	(const CHAR *)".stackcheck"

#define	SUP_SFTOL	(const CHAR *)".sftol"
#define	SUP_DFTOL	(const CHAR *)".dftol"
#define	SUP_LFTOL	(const CHAR *)".lftol"
#define	SUP_SFTOUL	(const CHAR *)".sftoul"
#define	SUP_DFTOUL	(const CHAR *)".dftoul"
#define	SUP_LFTOUL	(const CHAR *)".lftoul"
#define	SUP_LTOSF	(const CHAR *)".ltosf"
#define	SUP_ULTOSF	(const CHAR *)".ultosf"
#define	SUP_DFTOSF	(const CHAR *)".dftosf"
#define	SUP_LFTOSF	(const CHAR *)".uftosf"
#define	SUP_LTODF	(const CHAR *)".ltodf"
#define	SUP_ULTODF	(const CHAR *)".ultodf"
#define	SUP_SFTODF	(const CHAR *)".sftodf"
#define	SUP_LFTODF	(const CHAR *)".lftodf"
#define	SUP_LTOLF	(const CHAR *)".ltolf"
#define	SUP_ULTOLF	(const CHAR *)".ultolf"
#define	SUP_SFTOLF	(const CHAR *)".sftolf"
#define	SUP_DFTOLF	(const CHAR *)".dftolf"
#endif /* _GEN386_H */
