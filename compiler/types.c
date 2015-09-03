
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

/********************************************************* Macro Definitions */

#define MAXTYPEKEY	((unsigned) 256)	/* hash table size */

/********************************************************** Static Variables */

static TYP *typehashtable[MAXTYPEKEY];	/* hash table */

static RANGE range_schar = {
    -128L, 127L
};
static RANGE range_short = {
    -32768L, 32767L
};
static RANGE range_long = {
    -2147483647L - 1L, 2147483647L
};
static RANGE range_bool = {
    0L, (IVAL) Ox1UL
};
static RANGE range_uchar = {
    0L, (IVAL) OxffUL
};
static RANGE range_ushort = {
    0L, (IVAL) OxffffUL
};
static RANGE range_ulong = {
    0L, (IVAL) OxffffffffUL
};


#ifdef LONGLONG_BOOTSTRAP
static RANGE range_ulonglong = { 0L, (IVAL) ~0UL 
};
static RANGE range_longlong = { -2147483647L - 1L, 2147483647L 
};


#else	/*  */
#ifdef LONGLONG
static RANGE range_ulonglong = { 0L, (IVAL) 0xffffffffffffffffUL 
};
static RANGE range_longlong =
    { -9223372036854775807L - 1L, 9223372036854775807L 
};


#else	/*  */
static RANGE range_ulonglong = { 0L, (IVAL) 0xffffffffUL 
};
static RANGE range_longlong = { -2147483647L - 1L, 2147483647L 
};


#endif	/* LONGLONG */
#endif	/* LONGLONG_BOOTSTRAP */
    
/*****************************************************************************/
static FUNC func_params = {
    fc_normal,
    &init_block,
    0			/*lint !e708*/	/* union initialisation */
};

/*****************************************************************************/

static TYP stp_void = {
    bt_void,
    QUAL_NONE,
    STATE_NONE,
    UNKNOWN_SIZE,
    NIL_TYP,
    NIL_TYP,
    (const CHAR *) "void",
    {
     NIL_RANGE}	    /*lint !e708*/  /* union initialization */
};

static TYP stp_bool = {
    bt_bool,
    QUAL_NONE,
    STATE_NONE,
    1L,
    NIL_TYP,
    NIL_TYP,
    (const CHAR *) "_Bool",
    {
     &range_bool}	    /*lint !e708*/  /* union initialization */
};

static TYP stp_long = {
    bt_long,
    QUAL_NONE,
    STATE_NONE,
    4L,
    NIL_TYP,
    NIL_TYP,
    (const CHAR *) "long",
    {
     &range_long}	    /*lint !e708*/  /* union initialization */
};

static TYP stp_ulong = {
    bt_ulong,
    QUAL_NONE,
    STATE_NONE,
    4L,
    NIL_TYP,
    NIL_TYP,
    (const CHAR *) "unsigned long",
    {
     &range_ulong}	    /*lint !e708*/  /* union initialization */
};

static TYP stp_char = {
    bt_char,
    QUAL_NONE,
    STATE_NONE,
    1L,
    NIL_TYP,
    NIL_TYP,
    (const CHAR *) "char",
    {
     &range_schar}	    /*lint !e708*/  /* union initialization */
};

static TYP stp_uchar = {
    bt_uchar,
    QUAL_NONE,
    STATE_NONE,
    1L,
    NIL_TYP,
    NIL_TYP,
    (const CHAR *) "unsigned char",
    {
     &range_uchar}	    /*lint !e708*/  /* union initialization */
};

static TYP stp_schar = {
    bt_schar,
    QUAL_NONE,
    STATE_NONE,
    1L,
    NIL_TYP,
    NIL_TYP,
    (const CHAR *) "signed char",
    {
     &range_schar}	    /*lint !e708*/  /* union initialization */
};

static TYP stp_short = {
    bt_short,
    QUAL_NONE,
    STATE_NONE,
    2L,
    NIL_TYP,
    NIL_TYP,
    (const CHAR *) "short",
    {
     &range_short}	    /*lint !e708*/  /* union initialization */
};

static TYP stp_ushort = {
    bt_ushort,
    QUAL_NONE,
    STATE_NONE,
    2L,
    NIL_TYP,
    NIL_TYP,
    (const CHAR *) "unsigned short",
    {
     &range_ushort}	    /*lint !e708*/  /* union initialization */
};

static TYP stp_int = {
    bt_int32,
    QUAL_NONE,
    STATE_NONE,
    4L,
    NIL_TYP,
    NIL_TYP,
    (const CHAR *) "int",
    {
     &range_long}	    /*lint !e708*/  /* union initialization */
};

static TYP stp_uint = {
    bt_uint32,
    QUAL_NONE,
    STATE_NONE,
    4L,
    NIL_TYP,
    NIL_TYP,
    (const CHAR *) "unsigned int",
    {
     &range_ulong}	    /*lint !e708*/  /* union initialization */
};

static TYP stp_longlong = {
    bt_longlong,
    QUAL_NONE,
    STATE_NONE,
    8L,
    NIL_TYP,
    NIL_TYP,
    (const CHAR *) "long long",
    {
     &range_longlong}	    /*lint !e708*/  /* union initialization */
};

static TYP stp_ulonglong = {
    bt_ulonglong,
    QUAL_NONE,
    STATE_NONE,
    8L,
    NIL_TYP,
    NIL_TYP,
    (const CHAR *) "unsigned long long",
    {
     &range_ulonglong}	    /*lint !e708*/  /* union initialization */
};

#ifdef FLOAT_MFFP
static TYP stp_float = {
    bt_float,
    QUAL_NONE,
    STATE_NONE,
    4L,
    NIL_TYP,
    NIL_TYP,
    (const CHAR *) "float",
    {
     NIL_RANGE}	    /*lint !e708*/  /* union initialization */
};

static TYP stp_double = {
    bt_double,
    QUAL_NONE,
    STATE_NONE,
    4L,
    NIL_TYP,
    NIL_TYP,
    (const CHAR *) "double",
    {
     NIL_RANGE}	    /*lint !e708*/  /* union initialization */
};

static TYP stp_longdouble = {
    bt_longdouble,
    QUAL_NONE,
    STATE_NONE,
    4L,
    NIL_TYP,
    NIL_TYP,
    (const CHAR *) "long double",
    {
     NIL_RANGE}	    /*lint !e708*/  /* union initialization */
};

#else
static TYP stp_float = {
    bt_float,
    QUAL_NONE,
    STATE_NONE,
    4L,
    NIL_TYP,
    NIL_TYP,
    (const CHAR *) "float",
    {
     NIL_RANGE}	    /*lint !e708*/  /* union initialization */
};

static TYP stp_double = {
    bt_double,
    QUAL_NONE,
    STATE_NONE,
    8L,
    NIL_TYP,
    NIL_TYP,
    (const CHAR *) "double",
    {
     NIL_RANGE}	    /*lint !e708*/  /* union initialization */
};

static TYP stp_longdouble = {
    bt_longdouble,
    QUAL_NONE,
    STATE_NONE,
    12L,
    NIL_TYP,
    NIL_TYP,
    (const CHAR *) "long double",
    {
     NIL_RANGE}	    /*lint !e708*/  /* union initialization */
};

#endif /* FLOAT_MFFP */
static TYP stp_string = {
    bt_pointer32,
    QUAL_NONE,
    STATE_DERIVED,
    4L,
    &stp_char,
    NIL_TYP,
    (const CHAR *) "pointer to char",
    {
     NIL_RANGE}	    /*lint !e708*/  /* union initialization */
};

static TYP stp_conststring = {
    bt_pointer32,
    QUAL_CONST,
    STATE_DERIVED,
    4L,
    &stp_char,
    NIL_TYP,
    (const CHAR *) "const pointer to char",
    {
     NIL_RANGE}	    /*lint !e708*/  /* union initialization */
};

static TYP stp_func = {
    bt_func,
    QUAL_NONE,
    STATE_DERIVED,
    4L,
    &stp_int,
    NIL_TYP,
    (const CHAR *) "function",
    {
     (RANGE *) &func_params}	    /*lint !e708*/  /* union initialization */	/*lint !e826*/
};

static TYP stp_struct = {
    bt_struct,
    QUAL_NONE,
    STATE_NONE,
    UNKNOWN_SIZE,
    NIL_TYP,
    NIL_TYP,
    (const CHAR *) "struct",
    {
     NIL_RANGE}	    /*lint !e708*/  /* union initialization */
};

static TYP stp_union = {
    bt_union,
    QUAL_NONE,
    STATE_NONE,
    UNKNOWN_SIZE,
    NIL_TYP,
    NIL_TYP,
    (const CHAR *) "union",
    {
     NIL_RANGE}	    /*lint !e708*/  /* union initialization */
};

static TYP stp_pointer = {
    bt_pointer32,
    QUAL_NONE,
    STATE_NONE,
    4L,
    &stp_void,
    NIL_TYP,
    (const CHAR *) "pointer",
    {
     NIL_RANGE}	    /*lint !e708*/  /* union initialization */
};

static TYP stp_array = {
    bt_pointer32,
    QUAL_NONE,
    STATE_DERIVED,
    UNKNOWN_SIZE,
    NIL_TYP,
    NIL_TYP,
    (const CHAR *) "array",
    {
     NIL_RANGE}	    /*lint !e708*/  /* union initialization */
};

static TYP stp_enum = {
    bt_int32,
    QUAL_NONE,
    STATE_NONE,
    UNKNOWN_SIZE,
    NIL_TYP,
    NIL_TYP,
    (const CHAR *) "int",
    {
     NIL_RANGE}	    /*lint !e708*/  /* union initialization */
};

static TYP stp_ellipsis = {
    bt_ellipsis,
    QUAL_NONE,
    STATE_NONE,
    UNKNOWN_SIZE,
    NIL_TYP,
    NIL_TYP,
    (const CHAR *) "...",
    {
     NIL_RANGE}	    /*lint !e708*/  /* union initialization */
};

static TYP stp_floatcomplex = {
    bt_floatcomplex,
    QUAL_NONE,
    STATE_NONE,
    8L,
    NIL_TYP,
    NIL_TYP,
    (const CHAR *) "float _Complex",
    {
     NIL_RANGE}	    /*lint !e708*/  /* union initialization */
};

static TYP stp_floatimaginary = {
    bt_floatimaginary,
    QUAL_NONE,
    STATE_NONE,
    4L,
    NIL_TYP,
    NIL_TYP,
    (const CHAR *) "float _Imaginary",
    {
     NIL_RANGE}	    /*lint !e708*/  /* union initialization */
};

static TYP stp_doublecomplex = {
    bt_doublecomplex,
    QUAL_NONE,
    STATE_NONE,
    16L,
    NIL_TYP,
    NIL_TYP,
    (const CHAR *) "double _Complex",
    {
     NIL_RANGE}	    /*lint !e708*/  /* union initialization */
};

static TYP stp_doubleimaginary = {
    bt_doubleimaginary,
    QUAL_NONE,
    STATE_NONE,
    8L,
    NIL_TYP,
    NIL_TYP,
    (const CHAR *) "double _Imaginary",
    {
     NIL_RANGE}	    /*lint !e708*/  /* union initialization */
};

static TYP stp_longdoublecomplex = {
    bt_longdoublecomplex,
    QUAL_NONE,
    STATE_NONE,
    24L,
    NIL_TYP,
    NIL_TYP,
    (const CHAR *) "long double _Complex",
    {
     NIL_RANGE}	    /*lint !e708*/  /* union initialization */
};

static TYP stp_longdoubleimaginary = {
    bt_longdoubleimaginary,
    QUAL_NONE,
    STATE_NONE,
    12L,
    NIL_TYP,
    NIL_TYP,
    (const CHAR *) "long double _Imaginary",
    {
     NIL_RANGE}	    /*lint !e708*/  /* union initialization */
};

#ifndef CPU_DEFINED
/*
 *   The following tables specify the alignment requirements of the
 *   basic types depending on the processor type.
 */
static SIZE alignments[] = {
    1L,				/* bt_void      */
    1L,				/* bt_bool      */
    1L,				/* bt_char      */
    1L,				/* bt_charu     */
    1L,				/* bt_uchar     */
    1L,				/* bt_schar     */
    2L,				/* bt_short     */
    2L,				/* bt_ushort    */
    2L,				/* bt_int16     */
    2L,				/* bt_uint16    */
    2L,				/* bt_int32     */
    2L,				/* bt_uint32    */
    2L,				/* bt_long      */
    2L,				/* bt_ulong     */
    2L,				/* bt_longlong  */
    2L,				/* bt_ulonglong */
    2L,				/* bt_float     */
    2L,				/* bt_double    */
    2L,				/* bt_longdouble */
    2L,				/* bt_floatcomplex */
    2L,				/* bt_doublecomplex */
    2L,				/* bt_longdoublecomplex */
    2L,				/* bt_floatimaginary */
    2L,				/* bt_doubleimaginary */
    2L,				/* bt_longdoubleimaginary */
    2L,				/* bt_pointer16 */
    2L,				/* bt_pointer32 */
    2L,				/* bt_struct    */
    2L,				/* bt_union     */
    2L,				/* bt_func      */
    2L,				/* bt_bitfield  */
    2L,				/* bt_ubitfield */
    1L,				/* bt_bbitfield */
    2L				/* bt_ellipsis - used for alignment suitable for all types */
};

static SIZE *g_alignments = &alignments[0];

#endif /* CPU_DEFINED */

/*
 *  Creste a function node
 */
static FUNC *mk_func P2(FUNCCALL, kind, BLOCK *, params)
{
    FUNC *fp = (FUNC *) galloc (sizeof (FUNC));

    if (fp)
    {
	fp->kind = kind;
	fp->lst = (params ? params : mk_block ());
    }
    return fp;
}

/*****************************************************************************/

/*
 *   returns true it tp is an integral type,
 *   otherwise return false.
 */

BOOL is_integral_type P1 (const TYP *, tp)
{
    switch (tp->type) {
    case bt_bool:
    case bt_char:
    case bt_charu:
    case bt_uchar:
    case bt_schar:
    case bt_short:
    case bt_ushort:
    case bt_int16:
    case bt_uint16:
    case bt_int32:
    case bt_uint32:
    case bt_long:
    case bt_longlong:
    case bt_ulong:
    case bt_ulonglong:
	return TRUE;
    default:
	break;
    }
    return FALSE;
}


#ifdef FLOAT_SUPPORT

/*
 *   returns true if the type is a floating type,
 *   otherwise it returns false.
 */

BOOL is_floating_type P1 (const TYP *, tp)
{
    return is_real_floating_type (tp) || is_complex_type (tp);
}


/*
 *   returns true if the type is a real floating type,
 *   otherwise it returns false.
 */

BOOL is_real_floating_type P1 (const TYP *, tp)
{
    switch (tp->type) {
    case bt_float:
    case bt_double:
    case bt_longdouble:
	return TRUE;
    default:
	break;
    }
    return FALSE;
}


/*
 *   returns true if the type is a complex type
 *   otherwise it returns false
 */
BOOL is_complex_type P1 (const TYP *, tp)
{
    switch (tp->type) {
    case bt_floatcomplex:
    case bt_doublecomplex:
    case bt_longdoublecomplex:
	return TRUE;
    default:
	break;
    }
    return FALSE;
}
#endif	/* FLOAT_SUPPORT */

/*
 *   returns true if the type is an arithmetic type,
 *   otherwise it returns false.
 */
BOOL is_arithmetic_type P1 (const TYP *, tp)
{
    return (is_integral_type (tp)
#ifdef FLOAT_SUPPORT
	    || is_floating_type (tp)
#endif /* FLOAT_SUPPORT */
	);
}


/*
 *   returns true if the type is a function, otherwise returns false.
 */

BOOL is_function_type P1 (const TYP *, tp)
{
    if (tp == NIL_TYP) {
	return FALSE;
    }
    /* functions are implicitly converted to pointers to functions */
    return (is_func (tp)) ||
	(is_pointer_type (tp) && is_func (referenced_type (tp)));
}


/*
 *   returns true if the type is a scalar type,
 *   otherwise returns false.
 */
BOOL is_scalar_type P1 (const TYP *, tp)
{
    return (is_arithmetic_type (tp) ||
	    is_pointer_type (tp) || is_bitfield_type (tp));
}


/*
 *   returns true is the type is an object type,
 *   otherwise returns false.
 */

BOOL is_object_type P1 (const TYP *, tp)
{
    switch (tp->type) {
    case bt_func:
	return FALSE;
    default:
	break;
    }
    return TRUE;
}


/*
 *   returns true if the type is a struct or union,
 *   otherwise returns false.
 */

BOOL is_structure_type P1 (const TYP *, tp)
{
    switch (tp->type) {
    case bt_struct:
    case bt_union:
	return TRUE;
    default:
	break;
    }
    return FALSE;
}


/*
 *  returns true is the type is an anonymous struct or union,
 *  otherwise returns false.
 */

BOOL is_anonymous_structure P1 (const TYP *, tp)
{
    if (is_structure_type (tp)) {
	const CHAR *name = nameoftype (tp);

	return ((name == stp_struct.sname) || (name == stp_union.sname));
    }
    return FALSE;
}

/*
 *   returns true if the type is a pointer type,
 *   otherwise it returns false.
 */

BOOL is_pointer_type P1 (const TYP *, tp)
{
    switch (tp->type) {
    case bt_pointer32:
    case bt_pointer16:
	return TRUE;
    default:
	break;
    }
    return FALSE;
}


/*
 *   returns type if the type is an array,
 *   otherwise returns false.
 */

BOOL is_array_type P1 (const TYP *, tp)
{
    return (is_pointer_type (tp) && is_derived_type (tp));
}


/*
 *   return true if the type is an array with
 *   a variable length, otherwise returns false
 */
BOOL is_variable_array_type P1 (const TYP *, tp)
{
    return is_array_type (tp) && 0;
}

/*
 *   returns true if tp is a signed type,
 *   otherwise it returns false.
 */

BOOL is_signed_type P1 (const TYP *, tp)
{
    switch (tp->type) {
    case bt_char:
    case bt_schar:
    case bt_short:
    case bt_int16:
    case bt_int32:
    case bt_long:
    case bt_longlong:
    case bt_bitfield:
	return TRUE;
    default:
	break;
    }
    return FALSE;
}

/*
 *   returns true if tp is an unsigned type,
 *   otherwise it returns false.
 */

BOOL is_unsigned_type P1 (const TYP *, tp)
{
    switch (tp->type) {
    case bt_bool:
    case bt_ulonglong:
    case bt_ulong:
    case bt_uint32:
    case bt_uint16:
    case bt_ushort:
    case bt_uchar:
    case bt_charu:
    case bt_pointer16:
    case bt_pointer32:
    case bt_ubitfield:
    case bt_bbitfield:
	return TRUE;
    default:
	break;
    }
    return FALSE;
}

static BOOL is_compatible_proto P2 (const BLOCK *, block1, const BLOCK *, block2)
{
    SYM    *sp1 = symbolsof (block1);
    SYM    *sp2 = symbolsof (block2);

    if (sp1 == sp2) {
	return TRUE;
    }
    if ((sp1 == NIL_SYM) || (sp2 == NIL_SYM)) {
	return TRUE;
    }
    while ((sp1 != NIL_SYM) && (sp2 != NIL_SYM)) {
	if (!is_compatible_type (typeof (sp1), typeof (sp2))) {
	    return FALSE;
	}
	sp1 = nextsym (sp1);
	sp2 = nextsym (sp2);
    }
    return (sp1 == sp2);
}

static BOOL is_equal_proto P2 (const SYM *, sp1, const SYM *, sp2)
{
    if (sp1 == sp2) {
	return TRUE;
    }
    if ((sp1 == NIL_SYM) || (sp2 == NIL_SYM)) {
	return TRUE;
    }
    while ((sp1 != NIL_SYM) && (sp2 != NIL_SYM)) {
	if (!is_equal_type (typeof (sp1), typeof (sp2))) {
	    return FALSE;
	}
	sp1 = nextsym (sp1);
	sp2 = nextsym (sp2);
    }

    return (sp1 == sp2);
}

BOOL is_equal_type P2 (const TYP *, tp1, const TYP *, tp2)
{
    if (tp1 == tp2) {
	return TRUE;
    }
    if (tp1 == NIL_TYP || tp2 == NIL_TYP) {
	return FALSE;
    }
    if (!is_same_type (tp1, tp2)) {
	return FALSE;
    }
    if (tp1->qual != tp2->qual) {
	return FALSE;
    }
    switch (tp1->type) {
    case bt_pointer16:
    case bt_pointer32:
	return is_equal_type (referenced_type (tp1), referenced_type (tp2));
    case bt_func:
	return is_equal_type (returned_type (tp1), returned_type (tp2)) &&
	    is_equal_proto (parametersof (tp1), parametersof (tp2));
    case bt_struct:
    case bt_union:
	return (members (tp1) == members (tp2));
    default:
	break;
    }
    return TRUE;
}

TYP    *qualify_type P2 (TYP *, tp, QUALIFIER, qualifier)
{
    if (tp != NIL_TYP)
    {
	if (qualifier != tp->qual) {
	    TYP    *tp1 = tp;
	    QUALIFIER q = tp1->qual;

	    tp1->qual = qualifier;
	    tp = mk_type (tp1, tp1->btp);
	    tp1->qual = q;
	}
    }
    return tp;
}

/*
 * ANSI 3.1.2.6
 *   A composite type can be constructed from two types that are compatible;
 *   it is a type that is compatible with both of the two types and
 *   satisfies the following conditions:
 *
 *       If one type is an array of know size, the composite type is an
 *       array of that size.
 *
 *       If only one type is a function type with a parameter list (a
 *       function prototype), the composite type is a function prototype
 *       with the parameter list.
 *
 *       If both types are function types with parameter type lists, the
 *       type of each parameter in the composite parameter type list
 *       is the composite type of the corresponding parameters.
 *
 *   These rules apply recursively to the types from which the two
 *   types are derived.
 */
TYP    *composite_type P2 (TYP *, tp1, TYP *, tp2)
{
    SYM    *sp1, *sp2;

    if (!is_equal_type (tp1, tp2)) {
	return tp1;		/* error path */
    }
    if (tp1 == tp2) {
	return tp1;
    }
    switch (tp1->type) {
    case bt_pointer16:
    case bt_pointer32:
	/*
	 *   Combine the pointed at types.
	 */
	set_referenced_type (tp1,
			     composite_type (referenced_type (tp1),
					     referenced_type (tp2)));

	if (is_array_type (tp1)) {
	    if (is_unknown_size (tp1)) {
		/*
		 *   tp1 is an array with unknown size ... return
		 *   tp2 as it might have a known size.
		 */
		return tp2;
	    }
	    if (is_unknown_size (tp2)) {
		/*
		 *   tp2 is an array with unknown size ... return
		 *   tp1 as it has a known size.
		 */
		return tp1;
	    }
	}
	break;
    case bt_func:
	/*
	 *   Combine the function return types.
	 */
	set_returned_type (tp1,
			   composite_type (returned_type (tp1),
					   returned_type (tp2)));
	set_returned_type (tp2, returned_type (tp1));

	sp1 = parametersof (tp1);
	if (sp1 == NIL_SYM) {
	    /*
	     *       tp1 has no parameters so return tp2 ... it might have
	     */
	    return tp2;
	}
	sp2 = parametersof (tp2);
	if (sp2 == NIL_SYM) {
	    /*
	     *       tp2 has no parameters so return tp1 ... it does
	     */
	    return tp1;
	}
	/*
	 *   Now combine the type information for the function parameters
	 */
	while (sp1 != NIL_SYM && sp2 != NIL_SYM) {
	    set_type (sp1, composite_type (typeof (sp1), typeof (sp2)));
	    set_type (sp2, typeof (sp1));
	    sp1 = nextsym (sp1);
	    sp2 = nextsym (sp2);
	}
	break;
    default:
	break;
    }
    return tp1;
}

/*
 * This is used to tell valid if two types are compatible with each other.
 *
 * ANSI 3.1.2.6
 *      Two types have compatible type if their types are the same.
 *      Two structure, union, or enumeration types declared in separate
 *      translation units are compatible if they have the same number of
 *      members, the same member names, and compatible member types; for
 *      two structures, the members shall be in the same order; for two
 *      structures or unions, te bit-fields shall have the same widths;
 *      for two enumerations, the members shall have the same values.
 *
 *      All declarations that refer to the same object or function shall have
 *      compatible type; otherwise the behaviour is undefined.
 *
 * ANSI 3.5.3
 *      for two qualified types to be compatible, both shall have the
 *      identically qualified version of a compatible type;  the order of the
 *      qualifiers within a list of specifiers or qualifiers does not
 *      affect the specified type.
 *
 * ANSI 3.5.4.1
 *      for two pointers to be compatible, both shall be identically qualified
 *      and both shall be pointers to compatible types.
 *
 * ANSI 3.5.4.2
 *      For two arrays to be compatible, both shall have compatible element
 *      types, and if both size specifiers are present they shall have the
 *      same value.
 *
 * ANSI 3.5.4.3
 *      for two function type to be compatible, both shall specify compatible
 *      return types.  Moreover, the parameter type lists, if both are
 *      present, shall agree in the number of parameters and in use of the
 *      ellipsis terminator; corresponding parameters shall have compatible
 *      types.  If one type has a parameter list and the other type is
 *      specified by a function declarator that is not part of a function
 *      definition and that contains an empty identifier list, the
 *      parameter list shall not have an ellipsis terminator and the type
 *      of each parameter shall be compatible with the type that results
 *      from the application of the default argument promotions.  If one type
 *      has a parameter type list and the other type is specified by a function
 *      definition that contains a (possibly empty) identifier list, both
 *      shall agree in the numbe of parameters, and the type of each
 *      prototype argument shall be compatible with the type that results
 *      from the application of the default argument promotions to the type
 *      of the corresponding identifier.
 */
BOOL is_compatible_type P2 (const TYP *, tp1, const TYP *, tp2)
{
    if (tp1 == tp2) {
	return TRUE;
    }
    if (tp1 == NIL_TYP || tp2 == NIL_TYP) {
	return FALSE;
    }
    if (!is_same_type (tp1, tp2)) {
	return FALSE;
    }
    switch (tp1->type) {
    case bt_pointer16:
    case bt_pointer32:
	if (is_void (referenced_type (tp1))
	    || is_void (referenced_type (tp2))) {
	    return TRUE;
	}
	return (is_compatible_type
		(referenced_type (tp1), referenced_type (tp2)));
    case bt_func:
	return is_compatible_type (returned_type (tp1), returned_type (tp2))
	    && is_compatible_proto (parameters (tp1), parameters (tp2));
    case bt_struct:
    case bt_union:
	return (members (tp1) == members (tp2));
    default:
	break;
    }
    return TRUE;
}


/*
 *   perform the integral promotions and return the type
 */

TYP    *promote_type P1 (TYP *, tp)
{
    switch (tp->type) {
    case bt_bool:
    case bt_charu:
    case bt_uchar:
    case bt_ushort:
	if (is_same_size (tp, tp_int)) {
	    return tp_uint;
	}
	/*lint -fallthrough */
    case bt_char:
    case bt_schar:
    case bt_short:
	return tp_int;
    case bt_float:
	return tp_double;
    case bt_pointer16:
    case bt_pointer32:
	return tp;
    case bt_func:
	/* always passed by reference, never by value */
	return tp_func;
    default:
	break;
    }
    return tp;
}

/*
 *   perform the unary conversions and return the type
 */

TYP    *unary_conversion P1 (TYP *, tp)
{
    switch (tp->type) {
    case bt_bool:
    case bt_charu:
    case bt_uchar:
    case bt_ushort:
	if (is_same_size (tp, tp_int)) {
	    return tp_uint;
	}
	/*lint -fallthrough */
    case bt_char:
    case bt_schar:
    case bt_short:
	return tp_int;
    case bt_pointer16:
    case bt_pointer32:
	/* always passed by reference, never by value */
	return tp_pointer;
    case bt_func:
	/* always passed by reference, never by value */
	return tp_func;
    default:
	break;
    }
    return tp;
}

/*
 *   returns type if the size of the type is not known.
 *   There is a complication that "copied" types (i.e. ones
 *   with qualifiers) might have been copied before the size
 *   was known so we need to calculate it now!.
 */

BOOL is_incomplete_type P1 (TYP *, tp)
{
    if (is_unknown_size (tp)) {
	if (is_structure_type (tp) && (membersof (tp) != NIL_SYM)) {
	    /* the tag must be in the tag table */
	    SYM    *sp = tag_search (nameoftype (tp));

	    tp->size = typeof (sp)->size;
	    return is_incomplete_type (tp);
	}
	return TRUE;
    }
    return FALSE;
}


#ifndef SYNTAX_CORRECT
/*
 * ensure that the size of ep is known.
 */

void check_complete P1 (TYP *, tp)
{
    if (is_incomplete_type (tp)) {
	message (ERR_SIZE);
    }
}


/*
 *   Check that tp1 has all the qualifiers of tp2
 */
void check_qualifiers P2 (const TYP *, tp1, const TYP *, tp2)
{
    if (!is_pointer_type (tp1) || !is_pointer_type (tp2)) {
	return;
    }
    if ((tp1->btp->qual & tp2->btp->qual) != tp2->btp->qual) {
	message (ERR_QUAL);
    }
}
#endif /* SYNTAX_CORRECT */


TYP    *copy_type P1 (const TYP *, tp)
{
    TYP    *ret_tp;

    if (tp == NIL_TYP) {
	return NIL_TYP;
    }
    ret_tp = (TYP *) galloc (sizeof (TYP));

    *ret_tp = *tp;
    set_nexttyp (ret_tp, NIL_TYP);

    switch (tp->type) {
    case bt_pointer16:
    case bt_pointer32:
	set_referenced_type (ret_tp, copy_type (referenced_type (tp)));
	break;
    case bt_func:
	set_returned_type (ret_tp, copy_type (returned_type (tp)));
	set_function (ret_tp, mk_func (funckind (tp), NULL));
	break;
    default:
	break;
    }
    return ret_tp;
}

static unsigned typehash P1 (const TYP *, tp)
{
    return ((unsigned) tp >> (unsigned) 2 & (MAXTYPEKEY - (unsigned) 1));
}

/*
 *   Checks to see if the value 'i'  of type 'tp1' is within the
 *   range supported by the type 'tp2'.  This routine is only valid
 *   to be called for scalar types as these are the only ones which
 *   have a "range".
 */
BOOL is_constant_in_type_range P3 (IVAL, i, const TYP *, tp1, const TYP *,
				   tp2)
{
    RANGE  *rp2;
    RANGE   range;

    if (!is_scalar_type (tp2)) {
	return FALSE;
    }
#ifdef FLOAT_SUPPORT
    if (is_real_floating_type (tp2))
	return TRUE;
#endif /* FLOAT_SUPPORT */

    if (is_pointer_type (tp2))
	return TRUE;

    if (is_bitfield_type (tp2)) {
	if (is_signed_type (tp2)) {
	    range.low = -(IVAL) (1UL << ((int) bitfield_width (tp2))) - 1L;
	    range.high =
		(IVAL) (1UL << ((int) bitfield_width (tp2) - 1)) - 1L;
	} else {
	    range.low = 0L;
	    range.high = (IVAL) (1UL << (int) bitfield_width (tp2)) - 1L;
	}
	rp2 = &range;
    } else {
	rp2 = rangeof (tp2);
    }
    if (is_signed_type (tp1) && is_signed_type (tp2)) {
	return ((rp2->low <= i) && (i <= rp2->high));
    }
    if (is_unsigned_type (tp1) && is_unsigned_type (tp2)) {
	return (((UVAL) rp2->low <= (UVAL) i)
		&& ((UVAL) i <= (UVAL) rp2->high));
    }
    if (is_signed_type (tp1) && is_unsigned_type (tp2)) {
	return (((UVAL) rp2->low <= (UVAL) i)
		&& ((UVAL) i <= (UVAL) rp2->high));
    }
    /* is_unsigned_type (tp1) && is_signed_type (tp2) */
    return ((UVAL)i <= (UVAL)rp2->high);
}

/*
 *   Returns TRUE is tp2 is a sub-type of tp1, i.e tp1 can contain
 *   all values of tp2
 */
BOOL is_subtype P2 (const TYP *, tp1, const TYP *, tp2)
{
    if (is_integral_type (tp1) && is_integral_type (tp2)) {
	RANGE  *rp = rangeof (tp2);

	return (is_constant_in_type_range (rp->low, tp2, tp1) &&
		is_constant_in_type_range (rp->high, tp2, tp1));
    }
    return FALSE;
}

#if 0
/*
 *   Creates a range node for a type.
 */
RANGE  *mk_range P2 (IVAL, low, IVAL, high)
{
    RANGE  *rp;
    rp = (RANGE *) galloc (sizeof (RANGE));

    rp->low = low;
    rp->high = high;
    return rp;
}
#endif

TYP    *mk_type P2 (const TYP *, tp1, TYP *, btp)
{
    TYP    *tp;
    unsigned keyno;

    if (tp1 == NIL_TYP)
	return NIL_TYP;

    keyno = typehash (tp1);

    /*
     *       Look to see if this type has already been defined
     */
    switch (tp1->type) {
    case bt_bool:
    case bt_void:
    case bt_char:
    case bt_schar:
    case bt_uchar:
    case bt_charu:
    case bt_short:
    case bt_int16:
    case bt_ushort:
    case bt_uint16:
    case bt_pointer16:
    case bt_long:
    case bt_int32:
    case bt_longlong:
    case bt_ulong:
    case bt_uint32:
    case bt_ulonglong:
    case bt_pointer32:
	for (tp = typehashtable[keyno]; tp; tp = nexttyp (tp)) {
	    if ((btp != NIL_TYP) && is_func (btp)) {
		continue;
	    }
	    if (is_same_type (tp, tp1) &&
		(tp->qual == tp1->qual) &&
		is_same_size (tp, tp1) &&
		(tp->btp == btp) &&
		(is_enum (tp) == is_enum (tp1)) &&
		(is_derived_type (tp) == is_derived_type (tp1))) {
		return tp;
	    }
	}
	break;
    default:
	break;
    }

    tp = (TYP *) galloc (sizeof (TYP));
    if (tp == NULL) {
	return NULL;
    }

    *tp = stp_void;
    tp->size = tp1->size;
    tp->type = tp1->type;
    tp->qual = tp1->qual;
    set_nameoftype (tp, nameoftype (tp1));
    tp->btp = btp;
    if (is_derived_type (tp1)) {
	set_derived (tp);
    }
    if (is_enum (tp1)) {
	set_enum (tp);
    }
    switch (tp->type) {
    case bt_func:
	set_function (tp, mk_func (fc_normal, parameters (tp1)));
	break;
    case bt_struct:
    case bt_union:
	tp->t.structure = (STRUCT *) galloc (sizeof (STRUCT));
	if (tp->t.structure != NULL)
	{
	    if ((tp1 != NIL_TYP) && (tp1->t.structure != NULL) && members (tp1)) {
		set_members (tp, members (tp1));
	    } else {
		set_members (tp, mk_block ());
	    }
	}
	break;
    case bt_bool:
	set_range (tp, &range_bool);
	break;
    case bt_char:
    case bt_schar:
	set_range (tp, &range_schar);
	break;
    case bt_uchar:
    case bt_charu:
	set_range (tp, &range_uchar);
	break;
    case bt_short:
    case bt_int16:
	set_range (tp, &range_short);
	break;
    case bt_ushort:
    case bt_uint16:
    case bt_pointer16:
	set_range (tp, &range_ushort);
	break;
    case bt_long:
    case bt_int32:
	set_range (tp, &range_long);
	break;
    case bt_ulong:
    case bt_uint32:
    case bt_pointer32:
	set_range (tp, &range_ulong);
	break;
    case bt_longlong:
	set_range (tp, &range_longlong);
	break;
    case bt_ulonglong:
	set_range (tp, &range_ulonglong);
	break;
    default:
	break;
    }

    set_nexttyp (tp, typehashtable[keyno]);
    typehashtable[keyno] = tp;
    return tp;
}

void size_type P1 (TYP *, tp)
{
    TYP    *rtp;

    if (tp == NIL_TYP) {
	return;
    }
    if (is_array_type (tp)) {
	rtp = referenced_type (tp);
	size_type (rtp);
	if (rtp != NIL_TYP &&
	    (array_index (tp) != UNKNOWN_SIZE) && (!is_unknown_size (rtp))) {
	    tp->size = rtp->size * array_index (tp);
	}
    }
}

/*
 *   Calculate the alignment requirements of the specified type
 */
SIZE alignment_of_type P1 (const TYP *, tp)
{
    SYM    *sp;
    SIZE    algn, al;

    switch (tp->type) {
    case bt_struct:
    case bt_union:
	if (align_option) {
	    /* align structures to the alignment of the worst aligned member */
	    algn = g_alignments[bt_char];;
	    for (sp = membersof (tp); sp != NIL_SYM; sp = nextsym (sp)) {
		al = alignment_of_type (typeof (sp));
		if (al > algn) {
		    algn = al;
		}
	    }
	    return algn;
	}
	/*lint -fallthrough */
    case bt_void:
    case bt_bool:
    case bt_char:
    case bt_charu:
    case bt_uchar:
    case bt_schar:
    case bt_short:
    case bt_ushort:
    case bt_int16:
    case bt_uint16:
    case bt_int32:
    case bt_uint32:
    case bt_long:
    case bt_longlong:
    case bt_ulong:
    case bt_ulonglong:
    case bt_float:
    case bt_double:
    case bt_longdouble:
    case bt_floatcomplex:
    case bt_floatimaginary:
    case bt_doublecomplex:
    case bt_doubleimaginary:
    case bt_longdoublecomplex:
    case bt_longdoubleimaginary:
    case bt_func:
	return g_alignments[tp->type];
    case bt_pointer16:
    case bt_pointer32:
	return is_array_type (tp) ? alignment_of_type (referenced_type (tp)) :
	    g_alignments[tp->type];
    case bt_bitfield:
    case bt_ubitfield:
	return alignment_of_type (tp_int);
    case bt_bbitfield:
	return alignment_of_type (tp_bool);
    default:
	CANNOT_REACH_HERE ();
	return 0L;	/*lint !e527*/	    /* unreachable */
    }
}

void initialize_types P0 (void)
{
#ifdef LONGLONG
#ifdef LONGLONG_BOOTSTRAP
    {
	UVAL   uval;

	range_ulong.low = 0;
	uval = (UVAL) 0xFFFFFFFFUL;
	range_ulong.high = (IVAL) uval;
	range_ulonglong.low = 0;
	uval = 0;
	range_ulonglong.high = (IVAL) ~uval;
	uval = 1;
	range_longlong.low = (IVAL) (uval << 63);
	uval = 0;
	range_longlong.high = (IVAL) ((~uval) >> 1);
    };
    
#endif	/* LONGLONG_BOOTSTRAP */
#endif	/* LONGLONG */
    if (short_option) {
	/*
	 * set 'int' and 'enum' type
	 */
	stp_int.type = bt_int16;
	stp_uint.type = bt_uint16;
	stp_int.size = 2L;
	stp_uint.size = 2L;
    }
#ifdef INTEL_86
    if (small_option) {
	stp_pointer.type = bt_pointer16;
	stp_string.type = bt_pointer16;
	stp_array.type = bt_pointer16;
	stp_pointer.size = 2L;
	stp_string.size = 2L;
	stp_func.size = 2L;
    }
#endif /* INTEL_86 */
    if (uchar_option) {
	stp_char.type = bt_charu;
    }
    tp_void = mk_type (&stp_void, NIL_TYP);
    tp_long = mk_type (&stp_long, NIL_TYP);
    tp_ulong = mk_type (&stp_ulong, NIL_TYP);
    tp_char = mk_type (&stp_char, NIL_TYP);
    tp_uchar = mk_type (&stp_uchar, NIL_TYP);
    tp_schar = mk_type (&stp_schar, NIL_TYP);
    tp_short = mk_type (&stp_short, NIL_TYP);
    tp_ushort = mk_type (&stp_ushort, NIL_TYP);
    tp_int = mk_type (&stp_int, NIL_TYP);
    tp_uint = mk_type (&stp_uint, NIL_TYP);
    tp_float = mk_type (&stp_float, NIL_TYP);
    tp_double = mk_type (&stp_double, NIL_TYP);
    if (longdouble_option) {
	tp_longdouble = mk_type (&stp_longdouble, NIL_TYP);
    } else {
	tp_longdouble = tp_double;
    }
    tp_size = mk_type (&STP_SIZE, NIL_TYP);
    tp_ptrdiff = mk_type (&STP_PTRDIFF, NIL_TYP);
    tp_wchar = mk_type (&STP_WIDE, NIL_TYP);
    tp_string = mk_type (&stp_string, tp_char);
    tp_conststring = mk_type (&stp_conststring, tp_char);
    tp_wstring = mk_type (&stp_pointer, tp_wchar);
    tp_func = mk_type (&stp_func, tp_int);
    tp_pointer = mk_type (&stp_pointer, tp_void);
    tp_array = mk_type (&stp_array, NIL_TYP);
    tp_enum = mk_type (&stp_enum, NIL_TYP);
    tp_struct = &stp_struct;
    tp_union = &stp_union;
    tp_ellipsis = mk_type (&stp_ellipsis, NIL_TYP);

    tp_bool = mk_type (&stp_bool, NULL);
    tp_longlong = mk_type (&stp_longlong, NULL);
    tp_ulonglong = mk_type (&stp_ulonglong, NULL);
    tp_floatcomplex = mk_type (&stp_floatcomplex, NULL);
    tp_floatimaginary = mk_type (&stp_floatimaginary, NULL);
    tp_doublecomplex = mk_type (&stp_doublecomplex, NULL);
    tp_doubleimaginary = mk_type (&stp_doubleimaginary, NULL);
    tp_longdoublecomplex = mk_type (&stp_longdoublecomplex, NULL);
    tp_longdoubleimaginary = mk_type (&stp_longdoubleimaginary, NULL);
}
