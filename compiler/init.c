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
#include "outproto.h"

/********************************************************** Static Variables */

#ifdef CPU_DEFINED
static BOOL was_bitfield = FALSE;

#endif /* CPU_DEFINED */

static UVAL bit_value = (UVAL) 0;
static int level = 0;


/*********************************************** Static Function Definitions */

static EXPR *constexpr P_ ((TYP *));
static SIZE initarray P_ ((TYP *, BOOL));
static SIZE initbitfield P_ ((TYP *));
static SIZE initbool P_ ((TYP *));
static SIZE initchar P_ ((TYP *));
static SIZE initlong P_ ((TYP *));
static SIZE initpointer P_ ((TYP *));
static SIZE initshort P_ ((TYP *));
static SIZE initstruct P_ ((const TYP *, BOOL));
static SIZE inittype P_ ((TYP *));
static SIZE initunion P_ ((const TYP *, BOOL));
static void check_brace P_ ((BOOL));
static EXPR *initializer P_ ((TYP *, SIZE, STORAGE));

#ifdef CPU_DEFINED
static SIZE alignfield P_ ((SIZE, SIZE));

#endif /* CPU_DEFINED */

#ifdef TOPSPEED
static SIZE initfunc P_ ((BOOL));

#endif /* TOPSPEED */

#ifdef LONGLONG_SUPPORT
static SIZE initlonglong P_ ((TYP *));


#endif	/* LONGLONG_SUPPORT */
    
#ifdef FLOAT_SUPPORT
#ifndef FLOAT_BOOTSTRAP
static SIZE initfloat P_ ((TYP *));
static SIZE initdouble P_ ((TYP *));
static SIZE initlongdouble P_ ((TYP *));

#endif /* FLOAT_BOOTSTRAP */
#endif /* FLOAT_SUPPORT */

/*****************************************************************************/

static void check_brace P1 (BOOL, brace_seen)
{
#ifndef SYNTAX_CORRECT
    if (!brace_seen) {
	if (level == 1) {
	    message (ERR_BRACE);
	} else {
	    message (WARN_BRACE);
	}
    }
#endif /* SYNTAX_CORRECT */
}


SIZE align P2 (const TYP *, tp, SIZE, offset)
{
    SIZE    size = 0L;

#ifdef CPU_DEFINED
    SIZE    al = alignment_of_type (tp);

#else
    SIZE    al = tp->size;

#endif
    if (al) {
	while ((offset + size) % al)
	    size++;
    }
    return size;
}

#ifdef CPU_DEFINED
/*
 * cast an argument back which has been widened on the caller's side.
 * append the resulting assignment expression to init_node
 */
static void castback P3 (SIZE, offset, TYP *, tp1, TYP *, tp2)
{
    EXPR   *ep1, *ep2;

    if (is_same_type (tp1, tp2)) {
	return;
    }
    ep2 = mk_autocon (offset);
    ep1 = copynode (ep2);
    ep2 = mk_ref (ep2, tp1);
    ep2 = mk_node (en_cast, ep2, NIL_EXPR, tp2);
    ep1 = mk_ref (ep1, tp2);
    ep1 = mk_node (en_assign, ep1, ep2, tp2);

    if (init_node == NIL_EXPR) {
	init_node = ep1;
    } else {
	init_node = mk_node (en_comma, init_node, ep1, tp2);
    }
}


/*
 *	 Round 'size' up to a multiple of 'algn'
 */
static SIZE roundup P2 (SIZE, size, SIZE, algn)
{
    if (size % algn) {
	size = ((size / algn) + 1) * algn;
    }
    return size;
}

SIZE allocate_storage P4 (SYM *, sp, SIZE, offset, STORAGE, def_sc, BOOL,
			  promoted)
{
    TYP    *tp = typeof (sp);
    SIZE    size;

    switch (storageof (sp)) {
    case sc_register:
	if (def_sc == sc_parms) {
	    goto parms;   /*lint !e801*/  /* use of goto is deprecated */
	}
	/*lint -fallthrough */
    case sc_auto:
	size = tp->size;
	switch (tp->type) {
	case bt_struct:
	case bt_union:
	    size = roundup (size, alignment_of_type (tp));
	    break;
	default:
	    break;
	}
	if (g_is_ascending_stack ()) {
	    sp->value.i = (IVAL) (offset + align (tp, offset) + 1L);
	} else {
	    sp->value.i = (IVAL) (-(offset + align (tp, offset) + size));
	}
	offset += align (tp, offset) + size;
	break;
    case sc_static:
	if (sp->value.l == UNDEF_LABEL) {
	    sp->value.l = nextlabel++;
	}
	break;
    case sc_parms:
      parms:
	/*
	 * If we don't have a prototype in scope (ie. promoted is true)
	 * then parameters may have been widened when passed.  By knowing
	 * whether this is a BIGendian or LOWendian machine we can adjust
	 * the stack offset in order to perform the implicit narrowing cast.
	 */
	switch (tp->type) {
	case bt_char:
	case bt_charu:
	case bt_uchar:
	case bt_schar:
	    if (g_is_ascending_stack ()) {
		sp->value.i = (IVAL) (-(offset + tp->size) + 1L);
	    } else {
		if (g_is_bigendian ()) {
		    tp = promoted ? promote_type (tp) : tp_short;
		    sp->value.i = (IVAL) (offset + tp->size - 1L);
		} else {
		    tp = tp_int;
		    sp->value.i = (IVAL) offset;
		}
	    }
	    break;
	case bt_short:
	case bt_ushort:
	case bt_int16:
	case bt_uint16:
	    if (g_is_ascending_stack ()) {
		sp->value.i = (IVAL) (-(offset + tp->size) + 1);
	    } else {
		if (g_is_bigendian ()) {
		    tp = promoted ? promote_type (tp) : tp_short;
		    sp->value.i = (IVAL) (offset + tp->size - 2L);
		} else {
		    tp = tp_int;
		    sp->value.i = (IVAL) offset;
		}
	    }
	    break;
	case bt_float:
	    if (g_is_ascending_stack ()) {
		sp->value.i = (IVAL) (-(offset + tp->size) + 1L);
	    } else {
		if (promoted) {
		    castback (offset, tp_double, tp);
		    tp = tp_double;
		}
		sp->value.i = (IVAL) offset;
	    }
	    break;
	case bt_pointer16:
	case bt_pointer32:
	    tp = tp_pointer;
	    goto lab;		/* common code with function */   /*lint !e801*/  /* use of goto is deprecated */
	case bt_func:
	    tp = tp_func;
	    /*
	     * arrays and functions are never passed. They are really
	     * Pointers
	     */
	  lab:if (is_derived_type (typeof (sp))) {
		global_flag++;
		set_type (sp, copy_type (typeof (sp)));
		global_flag--;
		typeof (sp)->state &= (STATE) ~(unsigned) STATE_DERIVED;
		typeof (sp)->size = tp->size;
	    }
	    /*lint -fallthrough */
	default:
	    if (g_is_ascending_stack ()) {
		sp->value.i = (IVAL) (-(offset + tp->size) + 1L);
	    } else {
		sp->value.i = (IVAL) offset;
	    }
	    break;
	}
	offset += tp->size;
	break;
    default:
	break;
    }
    return offset;
}
#endif /* CPU_DEFINED */

SIZE doinit P2 (SYM *, sp, SIZE, offset)
{
    TYP    *tp = typeof (sp);

#ifdef CPU_DEFINED
    LABEL   label;
    EXPR   *ep1;

#endif /* CPU_DEFINED */
    EXPR   *ep2;
    int     brace_level = 0;

    switch (lastst) {
    case tk_assign:
	/* Initializer */
	getsym ();
	symbol_set (sp);
	switch (storageof (sp)) {
	case sc_auto:
	case sc_register:
	    /* AUTO Initialisation */
	    switch (tp->type) {
	    case bt_pointer16:
	    case bt_pointer32:
		if (!is_array_type (tp)) {
		    goto common;   /*lint !e801*/  /* use of goto is deprecated */
		}
		/* This must be an array.  However it is possible that the
		 * size of the array isn't known until we have parsed the
		 * initializer.  We must therefore delay calculating the
		 * offset of the item on the stack until we know it's actual
		 * size.
		 */
		/*lint -fallthrough */
	    case bt_struct:
	    case bt_union:
#ifndef SYNTAX_CORRECT
		if (lang_option == LANG_KANDR) {
		    message (ERR_ILLINIT);
		}
#endif /* SYNTAX_CORRECT */
		if (lastst == tk_id || lastst == tk_star
		    || lastst == tk_openpa) {
		    goto common;   /*lint !e801*/  /* use of goto is deprecated */
		}
#ifdef CPU_DEFINED
		label = nextlabel++;
		put_kseg (alignment_of_type (tp));
		put_label (label);
#endif /* CPU_DEFINED */
		VOIDCAST inittype (tp);

		if (is_array_type (tp)) {
		    tp = copy_type (tp);
		    set_array_assignment (tp);
		}
#ifdef CPU_DEFINED
		ep2 = mk_lcon (label);
		ep2 = mk_ref (ep2, tp);
		uses_structassign = TRUE;
#endif /* CPU_DEFINED */
		break;
	    default:
	      common:
		while (lastst == tk_begin) {
		    brace_level++;
		    getsym ();
		}
		ep2 = exprnc ();
#ifndef SYNTAX_CORRECT
		if (ep2 == NIL_EXPR) {
		    message (ERR_ILLINIT);
		    break;
		}
#endif /* SYNTAX_CORRECT */
		ep2 = implicit_castop (ep2, tp);
	    }
#ifdef CPU_DEFINED
	    offset = allocate_storage (sp, offset, sc_auto, FALSE);
	    ep1 = mk_symnode (sp);
	    ep1 = mk_ref (ep1, tp);
	    ep1 = mk_node (en_assign, ep1, ep2, tp);

	    if (init_node == NIL_EXPR) {
		init_node = ep1;
	    } else {
		init_node = mk_node (en_comma, init_node, ep1, tp);
	    }
#endif /* CPU_DEFINED */

	    while (brace_level--)
		needpunc (tk_end);
	    break;

	    /* Normal initializers */
	case sc_static:
#ifdef CPU_DEFINED
	    offset = allocate_storage (sp, offset, sc_auto, FALSE);
	    put_dseg (alignment_of_type (tp));
	    put_label (sp->value.l);
#endif /* CPU_DEFINED */
	    VOIDCAST inittype (tp);

	    break;
	case sc_external:
#ifndef SYNTAX_CORRECT
	    if (!is_global_scope (sp)) {
		message (ERR_ILLINIT);
	    }
#endif /* SYNTAX_CORRECT */
	    set_storage (sp, sc_global);
	    /*lint -fallthrough */
	case sc_global:
#ifdef CPU_DEFINED
	    put_dseg (alignment_of_type (tp));
	    put_name (sp);
#endif /* CPU_DEFINED */
	    VOIDCAST inittype (tp);

	    break;
#ifndef SYNTAX_CORRECT
	case sc_parms:
	    message (ERR_ILLINIT);
	    break;
#endif /* SYNTAX_CORRECT */
	default:
	    CANNOT_REACH_HERE ();
	    break;
	}
#ifndef SYNTAX_CORRECT
	if (is_symbol_defined (sp)) {
	    message (ERR_REDECL, nameof (sp));
	}
#endif /* SYNTAX_CORRECT */
	symbol_defined (sp);
	break;
    default:
#ifndef SYNTAX_CORRECT
	switch (storageof (sp)) {
	case sc_auto:
	case sc_global:
	case sc_static:
	    if (is_const_qualified (typeof (sp))) {
		message (WARN_CONSTINIT, nameof (sp));
	    }
	    break;
	default:
	    break;
	}
#endif /* SYNTAX_CORRECT */
#ifdef CPU_DEFINED
	offset = allocate_storage (sp, offset, sc_auto, FALSE);
#endif /* CPU_DEFINED */
	break;
    }
    sequence_point ();
    return offset;
}


static SIZE inittype P1 (TYP *, tp)
{
    BOOL    brace_seen = FALSE;
    SIZE    nbytes;

    level++;
    if (lastst == tk_begin) {
	brace_seen = TRUE;
	getsym ();
    }
    switch (tp->type) {
    case bt_bool:
	nbytes = initbool (tp);
	break;
    case bt_char:
    case bt_charu:
    case bt_uchar:
    case bt_schar:
	nbytes = initchar (tp);
	break;
    case bt_short:
    case bt_ushort:
    case bt_int16:
    case bt_uint16:
	nbytes = initshort (tp);
	break;
    case bt_pointer16:
    case bt_pointer32:
	if (is_array_type (tp)) {
	    nbytes = initarray (tp, brace_seen);
	} else {
	    nbytes = initpointer (tp);
	}
	break;
    case bt_bitfield:
    case bt_ubitfield:
    case bt_bbitfield:
	nbytes = initbitfield (tp);
	break;
    case bt_int32:
    case bt_uint32:
    case bt_ulong:
    case bt_long:
	nbytes = initlong (tp);
	break;
	
#ifdef LONGLONG_SUPPORT
    case bt_longlong:
    case bt_ulonglong:
	nbytes = initlonglong (tp);
	break;
	
#endif /* LONGLONG_SUPPORT */
    case bt_struct:
	nbytes = initstruct (tp, brace_seen);
	break;
    case bt_union:
	nbytes = initunion (tp, brace_seen);
	break;
#ifdef FLOAT_SUPPORT
#ifndef FLOAT_BOOTSTRAP
    case bt_float:
	nbytes = initfloat (tp);
	break;
    case bt_double:
	nbytes = initdouble (tp);
	break;
    case bt_longdouble:
	nbytes = initlongdouble (tp);
	break;
#endif /* FLOAT_BOOTSTRAP */
#endif /* FLOAT_SUPPORT */
#ifdef TOPSPEED
    case bt_func:
	if (topspeed_option) {
	    nbytes = initfunc (brace_seen);
	    break;
	}
	/*lint -fallthrough */
#endif /* TOPSPEED */
    default:
#ifndef SYNTAX_CORRECT
	message (ERR_NOINIT);
	nbytes = 0L;
#endif /* SYNTAX_CORRECT */
	break;
    }
    if (brace_seen) {
	if (lastst == tk_comma) {
	    getsym ();
	}
	needpunc (tk_end);
    }
    level--;
    return nbytes;
}

#ifdef TOPSPEED
/*
 *	 The TopSpeed C Compiler allows function to be "inlined" as
 *	 a series of initialized bytes.
 */
static SIZE initfunc P1 (BOOL, brace_seen)
{
    SIZE    nbytes;

    check_brace (brace_seen);
    for (nbytes = 0L;;) {
	VOIDCAST arithexpr (tp_uchar);

	nbytes += tp_uchar->size;
	if (lastst != tk_comma) {
	    break;
	}
	getsym ();		/* comma */
	if (lastst == tk_end) {
	    break;
	}
    }
    return nbytes;
}
#endif /* TOPSPEED */

static SIZE initarray P2 (TYP *, tp, BOOL, brace_seen)
{
    SIZE    nbytes;
    TYP    *rtp = referenced_type (tp);

    if (((lastst == tk_sconst) &&
	 (is_char (rtp) || rtp->type == bt_uchar || rtp->type == bt_schar)) ||
	((lastst == tk_wsconst && is_same_type (rtp, tp_wchar)))) {
#ifdef CPU_DEFINED
	SIZE    len;
	const CHAR *p;

#endif /* CPU_DEFINED */

	nbytes = (SIZE) lastsymlen;
#ifdef CPU_DEFINED
	for (len = nbytes, p = lastsym; len--;)
	    put_byte ((UVAL) *p++);
#endif /* CPU_DEFINED */
	if (is_unknown_size (tp)) {
	    tp->size = nbytes + 1L;
	}
	getsym ();		/* skip sconst/wsconst */
    } else {
	check_brace (brace_seen);
	for (nbytes = 0L;;) {
	    nbytes += inittype (rtp);
	    if (lastst != tk_comma || nbytes == tp->size) {
		break;
	    }
	    getsym ();		/* comma */
	    if (lastst == tk_end) {
		break;
	    }
	}
#ifndef SYNTAX_CORRECT
	if (tp->size > nbytes) {
	    message (WARN_INCOMPLETE);
	}
#endif /* SYNTAX_CORRECT */
    }

    if (is_unknown_size (tp)) {
	tp->size = nbytes;
#ifndef SYNTAX_CORRECT
    } else if (tp->size < nbytes) {
	message (ERR_INITSIZE);
#endif /* SYNTAX_CORRECT */
    }
#ifdef CPU_DEFINED
    for (; tp->size > nbytes; nbytes++)
	put_byte ((UVAL) Ox0UL);
#endif /* CPU_DEFINED */
    return nbytes;
}

static SIZE initunion P2 (const TYP *, tp, BOOL, brace_seen)
{
    SYM    *sp = membersof (tp);
    SIZE    nbytes;

/*
 * Initialize the first branch
 */
    if (sp == NIL_SYM) {
	return 0L;
    }
    check_brace (brace_seen);
    nbytes = inittype (typeof (sp));
#ifdef CPU_DEFINED
    for (; nbytes < tp->size; nbytes++)
	put_byte ((UVAL) Ox0UL);
#endif /* CPU_DEFINED */
    return nbytes;
}

#ifdef CPU_DEFINED
static SIZE alignfield P2 (SIZE, nbytes, SIZE, offset)
{
    if (was_bitfield && nbytes < offset) {
	if (tp_int->size == 2L) {
	    put_word (bit_value);
	} else {
	    put_dword (bit_value);
	}
	bit_value = (UVAL) 0;
	was_bitfield = FALSE;
	nbytes += tp_int->size;
    }
    for (; nbytes < offset; nbytes++)
	put_byte ((UVAL) Ox0UL);
    return nbytes;
}
#endif /* CPU_DEFINED */

static SIZE initstruct P2 (const TYP *, tp, BOOL, brace_seen)
{
    SYM    *sp;

#ifdef CPU_DEFINED
    SIZE    nbytes = 0L;

#endif /* CPU_DEFINED */
    check_brace (brace_seen);
    for (sp = membersof (tp); sp != NIL_SYM;) {
	if (is_const (sp)) {
	    sp = nextsym (sp);
	    continue;
	}
#ifdef CPU_DEFINED
	nbytes = alignfield (nbytes, (SIZE) sp->value.i);
	nbytes += inittype (typeof (sp));
#endif /* CPU_DEFINED */
	sp = nextsym (sp);
	if (lastst != tk_comma || sp == NIL_SYM) {
	    break;
	}
	getsym ();		/* comma */
	if (lastst == tk_end) {
	    break;
	}
    }
#ifndef SYNTAX_CORRECT
    if (sp != NIL_SYM) {
	message (WARN_INCOMPLETE);
    }
#endif /* SYNTAX_CORRECT */
#ifdef CPU_DEFINED
    nbytes = alignfield (nbytes, tp->size);
#endif /* CPU_DEFINED */
    return tp->size;
}

static SIZE initbitfield P1 (TYP *, tp)
{
    EXPR   *ep;
    UVAL    val;

#ifdef CPU_DEFINED
    was_bitfield = TRUE;
#endif /* CPU_DEFINED */
    if ((ep = constexpr (tp)) == NIL_EXPR) {
	return 0L;
    }
    val = ep->v.u & bitmask (bitfield_width (tp));
    bit_value |= (val << (int) bitfield_offset (tp));
    return 0L;
}

static SIZE initbool P1 (TYP *, tp)
{
    EXPR   *ep;

    if ((ep = constexpr (tp)) == NIL_EXPR) {
	return 0L;
    }
#ifdef CPU_DEFINED
    put_char (unsymbolexpr (ep));
#endif /* CPU_DEFINED */
    return tp->size;
}

static SIZE initchar P1 (TYP *, tp)
{
    EXPR   *ep;

    if ((ep = constexpr (tp)) == NIL_EXPR) {
	return 0L;
    }
#ifdef CPU_DEFINED
    put_char (unsymbolexpr (ep));
#endif /* CPU_DEFINED */
    return tp->size;
}

static SIZE initshort P1 (TYP *, tp)
{
    EXPR   *ep;

    if ((ep = constexpr (tp)) == NIL_EXPR) {
	return 0L;
    }
#ifdef CPU_DEFINED
    put_short (unsymbolexpr (ep));
#endif /* CPU_DEFINED */
    return tp->size;
}

static SIZE initlong P1 (TYP *, tp)
{
    EXPR   *ep;

    if ((ep = constexpr (tp)) == NIL_EXPR) {
	return 0L;
    }
#ifdef CPU_DEFINED
    put_long (unsymbolexpr (ep));
#endif /* CPU_DEFINED */
    return tp->size;
}


#ifdef LONGLONG_SUPPORT
static SIZE initlonglong P1 (TYP *, tp)
{
    EXPR   *ep;

    if ((ep = constexpr (tp)) == NIL_EXPR) {
	return 0L;
    }
#ifdef CPU_DEFINED
    put_longlong (unsymbolexpr (ep));
#endif /* CPU_DEFINED */
    return tp->size;
}


#endif /* LONGLONG_SUPPORT */
#ifdef FLOAT_SUPPORT
#ifndef FLOAT_BOOTSTRAP
static SIZE initfloat P1 (TYP *, tp)
{
    RVAL    val;

    floatexpr (tp, &val);
#ifdef CPU_DEFINED
    put_float (&val);
#endif /* CPU_DEFINED */
    return tp->size;
}

static SIZE initdouble P1 (TYP *, tp)
{
    RVAL    val;

    floatexpr (tp, &val);
#ifdef CPU_DEFINED
    put_double (&val);
#endif /* CPU_DEFINE */
    return tp->size;
}

static SIZE initlongdouble P1 (TYP *, tp)
{
    RVAL    val;

    floatexpr (tp, &val);
#ifdef CPU_DEFINED
    put_longdouble (&val);
#endif /* CPU_DEFINED */
    return tp->size;
}
#endif /* FLOAT_BOOTSTRAP */
#endif /* FLOAT_SUPPORT */

static SIZE initpointer P1 (TYP *, tp)
{
    EXPR   *ep;

    if ((ep = constexpr (tp)) == NIL_EXPR) {
	return 0L;
    }
#ifdef CPU_DEFINED
    put_pointer (unsymbolexpr (ep));
#endif /* CPU_DEFINED */
    return tp->size;
}

static EXPR *constexpr P1 (TYP *, tp)
{
    EXPR   *ep = exprnc ();

#ifndef SYNTAX_CORRECT
    if (ep == NIL_EXPR) {
	message (ERR_CONSTEXPR);
	getsym ();
	return NIL_EXPR;
    }
#endif /* SYNTAX_CORRECT */
    ep = implicit_castop (ep, tp);
    ep = constantopt (ep);
    /* ep may still contain casts between 32-bit integers and pointers */
    while ((ep->nodetype == en_cast)
	   && (ep->etp->type == bt_long ||
	       ep->etp->type == bt_ulong ||
	       ep->etp->type == bt_int32 ||
	       ep->etp->type == bt_uint32 || ep->etp->type == bt_pointer32)
	   && (ep->v.p[0]->etp->type == bt_long ||
	       ep->v.p[0]->etp->type == bt_ulong ||
	       ep->v.p[0]->etp->type == bt_int32 ||
	       ep->v.p[0]->etp->type == bt_uint32 ||
	       ep->v.p[0]->etp->type == bt_pointer32)) {
	ep->v.p[0]->etp = ep->etp;
	ep = ep->v.p[0];
    }

#ifndef SYNTAX_CORRECT
    if (!tst_const (ep)) {
	message (ERR_CONSTEXPR);
	getsym ();
	return NIL_EXPR;
    }
#endif /* SYNTAX_CORRECT */
    return ep;
}

void initfuncid P2 (const SYM *, sp, const SYM *, funcsp)
{
    SIZE    nbytes = (SIZE) strlen ((const char *) nameof (funcsp)) + 1;
    TYP    *tp = typeof (sp);

#ifdef CPU_DEFINED
    const CHAR *p;
    SIZE    len;

    put_cseg (alignment_of_type (tp));
    put_label (sp->value.l);
    for (len = nbytes, p = nameof (funcsp); len--;)
	put_byte ((UVAL) *p++);
#endif /* CPU_DEFINED */
    tp->size = nbytes;
}

/*
 *	initializer:
 *	assignment_expression
 *	{ initializer-list }
 *	{ initializer-list , }
 */
static EXPR   *initializer P3 (TYP *, tp, SIZE, offset, STORAGE, sc)
{
    EXPR   *ep = NIL_EXPR;
    int     brace_level = 0;

    /*
     *      the type of the entity to be initialized shall be an
     *      array of unknown size or an object type that is not
     *      a variable length array type.
     */
    switch (lastst) {
    case tk_begin:
	getsym ();
	ep = initializer_list (tp, offset, sc);
	if (lastst == tk_comma) {
	    getsym ();
	}
	needpunc (tk_end);
	break;
    default:
	while (lastst == tk_begin) {
	    brace_level++;
	    getsym ();
	}
	switch (tp->type) {
	case bt_pointer16:
	case bt_pointer32:
	    if (is_array_type (tp)) {
		TYP    *rtp = referenced_type (tp);

		if (lastst == tk_sconst &&
		    (is_char (rtp) || rtp->type == bt_uchar
		     || rtp->type == bt_schar)) {
		    /*
		     *      An aray of character type may be initialized
		     *      by a character string literal, optionally
		     *      enclosed in braces.  Successive characters of
		     *      the character string literal (including the
		     *      terminating null character if there is room or
		     *      if the array is of unknown size) initializes the
		     *      elements of the array.
		     */
		    if (is_unknown_size (tp)) {
			tp->size = lastsymlen + 1L;
#ifndef SYNTAX_CORRECT
		    } else if (lastsymlen > tp->size) {
			message (ERR_INITSIZE);
#endif /* SYNTAX_CORRECT */
		    }
		    ep = mk_scon (lastsym, tp);
		    getsym ();
		} else if (lastst == tk_wsconst
			   && is_same_type (rtp, tp_wchar)) {
		    /*      An array with element type compatible with
		     *      wchar_t may be initialized by a wide string
		     *      literal,optionally enclosed in braces.
		     *      Succesive wide charaters of the wide string
		     *      literal (including the terminating null wide
		     *      character if there is room or if the array
		     *      is of unknon size) initialized the elemens of
		     *      the array.
		     */
		    if (is_unknown_size (tp)) {
			tp->size = lastsymlen + 1L;
#ifndef SYNTAX_CORRECT
		    } else if (lastsymlen > tp->size) {
			message (ERR_INITSIZE);
#endif /* SYNTAX_CORRECT */
		    }
		    ep = mk_scon (lastsym, tp);
		    getsym ();
		} else {
		}
		break;
	    }
	    /*lint -fallthrough */
	case bt_bool:
	case bt_char:
	case bt_schar:
	case bt_charu:
	case bt_uchar:
	case bt_short:
	case bt_int16:
	case bt_ushort:
	case bt_uint16:
	case bt_int32:
	case bt_uint32:
	case bt_long:
	case bt_ulong:
	case bt_longlong:
	case bt_ulonglong:
	case bt_float:
	case bt_double:
	case bt_longdouble:
	    /*
	     *      The initialize for a scalar shall be a single
	     *      expression, optionally enclosed in braces.      The
	     *      initial value of the object is that of the
	     *      expression (after conversion);  the same type
	     *      constraints and conversions as for simple
	     *      assignment apply, taking the type of the scalar
	     *      to be the unqualified version of its declared
	     *      type
	     */
	    ep = exprnc ();
	    break;
	case bt_struct:
	case bt_union:
	    /*
	     *      The initializer for a structure or union object
	     *      that has automatic storage duration shall be
	     *      shall be either an initializer list, or a
	     *      single expression that has compatible structure
	     *      or union type.  In the latter case, the initial
	     *      value of the object, including unnamed members,
	     *      is that of the expression.
	     */
	    switch (lastst) {
	    case tk_id:
	    case tk_star:
	    case tk_openpa:
		break;
	    default:
		ep = exprnc ();
		break;
	    }
	    break;
	default:
	    CANNOT_REACH_HERE ();
	}
	while (brace_level--) {
	    needpunc (tk_end);
	}

	/*
	 *      All the expressions in an initializer for an object
	 *      that has static storage duration shall be constant
	 *      expressions or string literals
	 */
	switch (sc) {
	case sc_static:
#ifndef SYNTAX_CORRECT
	    if (!tst_const (ep)) {
		message (ERR_CONSTEXPR);
	    }
#endif /* SYNTAX_CORRECT */
	    break;
	default:
	    break;
	}
	ep = implicit_castop (ep, tp);
	ep = mk_litval (ep, offset);
	break;
    }
    return ep;
}

/*
 *	initializer-list:
 *	designation[opt] initializer
 *	initializer-list , designation[opt] initializer
 *
 *	designation:
 *	designator-list =
 *
 *	designator-list:
 *	designator
 *	desingator-list designator
 *
 *	designator:
 *	[ constant-expression ]
 *	. identifier
 */
EXPR   *initializer_list P3 (TYP *, tp, SIZE, offset, STORAGE, sc)
{
    EXPR   *ephead, *eptail;
    EXPR   *ep1;

    /*
     *      Each brace-enclosed initializer list has an
     *      associated current object.      When no
     *      designations are present, subobjects of the
     *      current object are initialized in order
     *      according to the type of the current object:
     *      array elements in increasing subscript order,
     *      structure members in declaration order, and
     *      the first named member of a union.      In contrast,
     *      a designation causes the following initializer
     *      to begin initialization of the subobject
     *      described by the designator.  Initialization
     *      then continues forward in order, beginning
     *      with the next subobject after that described by
     *      the designator.
     */

    /*
     *      Each designator list begins its description with
     *      with the current object associated with the
     *      closest surrounding brace pair.  Each item in
     *      the designator list (in order) specifies a
     *      particular member of its current object and
     *      changes the current object for the next
     *      designator (if any) to be that member.  The
     *      current object that results at the end of the
     *      designator list is the subobject to be initialized
     *      by the following initializer.
     */

    /*
     *      The initialization shall occur in initializer 
     *      list order,  Each initializer provided for a
     *      particular subobject overriding any previously
     *      listed initializer for the same subobject; all
     *      subobjects that are not initialized explicitly
     *      shall be initialized the same as objects that
     *      have static storage duration.
     */

    ephead = eptail = NIL_EXPR;
    for (;;) {
	TYP    *tp1 = tp;
	SIZE    off = offset;

	switch (lastst) {
	case tk_openbr:
	case tk_dot:
	    for (;;) {
		SIZE    ind;
		SYM    *sp;

		switch (lastst) {
		case tk_openbr:
		    /*
		     *      If a designator has the form
		     *
		     *              [ constant-expression ]
		     *

		     *      then the current object shall have array type and
		     *      the expression shall be an integer constant
		     *      expression.   If the array is of unknown size,
		     *      any nonnegative value is valid.
		     */
		    getsym ();
#ifndef SYNTAX_CORRECT
		    if (!is_array_type (tp1)) {
			message (ERR_NOARRAY);
		    }
#endif /* SYNTAX_CORRECT */
		    ind = (SIZE) intexpr ();
		    tp1 = referenced_type (tp1);
		    needpunc (tk_closebr);
		    off += tp1->size * ind;
		    break;
		case tk_dot:
		    /*
		     *      If a designator has the form
		     *
		     *              . identifier
		     *
		     *      then the current object shall have structure or
		     *      union type and the identifier shall be the name
		     *      of a member of that type.
		     */
		    getsym ();
		    if (lastst == tk_id) {
			if (is_structure_type (tp1) &&
			    (members (tp1) != NIL_BLOCK) &&
			    ((sp
			      =
			      search (lastsym,
				      &(members (tp1)->symbols))) !=
			     NIL_SYM)) {
			    tp1 = typeof (sp);
			    getsym ();
			    off += (SIZE) sp->value.u;
#ifndef SYNTAX_CORRECT
			} else {
			    message (ERR_NOMEMBER, lastsym);
#endif /* SYNTAX_CORRECT */
			}
#ifndef SYNTAX_CORRECT
		    } else {
			message (ERR_IDEXPECT);
#endif /* SYNTAX_CORRECT */
		    }
		    break;
		case tk_assign:
		    getsym ();
		    goto common;   /*lint !e801*/  /* use of goto is deprecated */
		default:
		    needpunc (tk_assign);
		    return NIL_EXPR;
		}
	    }
	    break;
	default:
	  common:
	    ep1 = initializer (tp1, off, sc);
	    break;
	}
	if (eptail) {
	    eptail->v.p[1] = mk_node (en_list, ep1, NIL_EXPR, tp_void);
	    eptail = eptail->v.p[1];
	} else {
	    ephead = eptail = mk_node (en_list, ep1, NIL_EXPR, tp_void);
	}
	if (lastst != tk_comma)
	    return ephead;
	getsym ();
    }
}
