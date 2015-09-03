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
 *  generate a MOTOROLA FastFloatingPoint numbers
 *
 *****************************************************************************/

#include "chdr.h"

#ifdef MC680X0
#ifdef FLOAT_MFFP
#ifndef FLOAT_BOOTSTRAP

#include "expr.h"
#include "cglbdec.h"
#include "proto.h"

/*****************************************************************************/

unsigned long genffp P1 (const RVAL *, dp)
{
    unsigned long mantissa;
    int     sign = 0, exponent = 64, i;
    RVAL    d;

    FASSIGN (d, dp);
    if (FLT (d, F_zero)) {
	sign = 128;
	FNEG (d);
    }
    while (FLT (d, F_half)) {
	FADD (d, d);
	--exponent;
	if (exponent == 0) {
	    return sign;	/* zero fp number */
	}
    }

    while (FGT (d, F_one)) {
	FMUL (d, F_half);
	++exponent;
	if (exponent >= 127) {
	    return 127 + sign;	/* +/- infinity */
	}
    }

    /*
     *       0.5 <=d <1.0 now: construct the mantissa
     */

    mantissa = 0;
    for (i = 0; i < 24; i++) {
	/* 24 mantissa bits */
	FADD (d, d);
	mantissa = mantissa + mantissa;
	if (FGT (d, F_one)) {
	    ++mantissa;
	    FSUB (d, F_one);
	}
    }

    /*
     *       round up, if the next bit would be 1
     */
    if (FGE (d, F_half)) {
	++mantissa;
    }
    /*
     *       check on mantissa overflow
     */
    if (mantissa > Ox00ffffffUL) {
	++exponent;
	/* exponent overflow? */
	if (exponent >= 127) {
	    return (127 + sign);
	}
	mantissa >>= 1;
    }
    /*
     *       put the parts together and return the value
     */

    return (mantissa << 8) + sign + exponent;
}
#endif /* FLOAT_BOOTSTRAP */
#endif /* FLOAT_MFFP */
#endif /* MC680X0 */
