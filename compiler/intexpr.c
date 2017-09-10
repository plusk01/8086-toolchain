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
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"

/*****************************************************************************/

#ifdef FLOAT_SUPPORT
#ifndef FLOAT_BOOTSTRAP
/*
 *  floating point expression
 */
void floatexpr P2 (TYP *, tp, RVAL *, fp)
{
    EXPR   *ep = exprnc ();

    if (ep != NIL_EXPR) {
	ep = implicit_castop (ep, tp);
	ep = constantopt (ep);

	if (is_fcon (ep)) {
	    FASSIGN (*fp, ep->v.f);
	    return;
	}
    }
#ifndef SYNTAX_CORRECT
    message (ERR_CONSTFLOAT);
    FASSIGN (*fp, F_one);
    return;
#endif /* SYNTAX_CORRECT */
}
#endif /* FLOAT_BOOTSTRAP */
#endif /* FLOAT_SUPPORT */

/*
 *  integer arithmetic expression
 */
IVAL arithexpr P1 (TYP *, tp)
{
    EXPR   *ep = exprnc ();

    if (ep != NIL_EXPR) {
	ep = implicit_castop (ep, tp);
	ep = constantopt (ep);
	if (is_icon (ep)) {
	    return ep->v.i;
	}
    }
#ifndef SYNTAX_CORRECT
    message (ERR_CONSTINT);
    getsym ();
#endif /* SYNTAX_CORRECT */
    return 0L;
}

IVAL intexpr P0 (void)
{
    EXPR   *ep = exprnc ();

    if (ep != NIL_EXPR) {
	ep = constantopt (ep);
	if (is_icon (ep)) {
	    return ep->v.i;
	}
    }
#ifndef SYNTAX_CORRECT
    message (ERR_CONSTINT);
    /*
     * any return value is wrong, but 1 is
     * less likely than 0 to cause spurious
     * errors later in the compilation.
     */
#endif /* SYNTAX_CORRECT */
    return 1L;
}
