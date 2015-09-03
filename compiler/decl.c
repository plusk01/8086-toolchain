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
#include "outproto.h"

/********************************************************* Macro Definitions */

/* type specifiers */
#define	T_VOID		((unsigned) 1)
#define	T_CHAR		((unsigned) 2)
#define	T_SHORT		((unsigned) 4)
#define	T_INT		((unsigned) 8)
#define	T_LONG		((unsigned) 16)
#define	T_LONGLONG	((unsigned) 32)
#define	T_FLOAT		((unsigned) 64)
#define	T_DOUBLE	((unsigned) 128)
#define	T_SIGNED	((unsigned) 256)
#define	T_UNSIGNED	((unsigned) 512)
#define	T_BOOL		((unsigned) 1024)
#define	T_COMPLEX	((unsigned) 2048)
#define	T_IMAGINARY	((unsigned) 4096)
#define	T_ALL		(T_VOID|T_CHAR|T_SHORT|T_INT|T_LONG|T_FLOAT|T_DOUBLE|T_SIGNED|T_UNSIGNED|T_BOOL|T_COMPLEX|T_IMAGINARY)

#ifdef SYNTAX_CORRECT
#define	check_stdarg(x)
#define	check_tag(x1,x2)
#define	check_function_declaration(x)
#define	check_function_definition(x)
#define	check_representable(x1,x2)
#define	check_parameters(x1,x2)
#endif /* SYNTAX_CORRECT */

/*********************************************** Static Function Definitions */

static BLOCK *get_parameters P_ ((const SYM *));
static BOOL is_typedef_symbol P_ ((const CHAR *));
static QUALIFIER type_qualifier P_ ((QUALIFIER));
static QUALIFIER type_qualifier_list P_ ((void));
static SIZE declaration P_ ((STORAGE, STORAGE, SIZE));
static SIZE struct_declaration_list P_ ((const TYP *));
static STORAGE storage_class_specifier P_ ((STORAGE, BOOL));
static FSPECIFIER function_specifier P_ ((FSPECIFIER));
static SYM *declarator P_ ((STORAGE, TYP *));
static SYM *declare P_ ((SYM *, STORAGE));
static SYM *direct_declarator P_ ((STORAGE, TYP *));

static TYP *declaration_specifiers P_ ((STORAGE *, STORAGE, FSPECIFIER*));
static TYP *direct_declarator_tail P_ ((SYM *, TYP *));
static TYP *enum_specifier P_ ((STORAGE));
static TYP *enumeration_list P_ ((void));
static TYP *struct_or_union_specifier P_ ((STORAGE));
static void identifier_list P_ ((void));
static void parameter_type_list P_ ((void));

#ifndef SYNTAX_CORRECT
static void check_parameters P_ ((const BLOCK *, const SYM *));
static void check_stdarg P_ ((const SYM *));
static void check_tag P_ ((const SYM *, const TYP *));
#endif /* SYNTAX_CORRECT */

static SYM *func_symbol = NULL;

/*****************************************************************************/

/*
 *   Returns true if the token is one which starts a type
 *   specifier, otherwise it returns false.
 */

static BOOL is_type_specifier P1 (TOKEN, st)
{
    switch (st) {
    case tk_id:
	return is_typedef_symbol (lastsym);
    case kw_void:
    case kw_bool:
    case kw_char:
    case kw_short:
    case kw_unsigned:
    case kw_signed:
    case kw_long:
    case kw_struct:
    case kw_union:
    case kw_enum:
    case kw_float:
    case kw_double:
    case kw_int:
	return TRUE;
    default:
	break;
    }
    return FALSE;
}


/*
 *   Returns true if the token is one which starts a storage
 *   class specifier.
 */

static BOOL is_storage_class P1 (TOKEN, st)
{
    switch (st) {
    case kw_typedef:
    case kw_static:
    case kw_auto:
    case kw_register:
    case kw_extern:
	return TRUE;
    default:
	break;
    }
    return FALSE;
}


/*
 *   Returns true if the token is one which starts a type
 *   qualifier.
 */

static BOOL is_type_qualifier P1 (TOKEN, st)
{
    switch (st) {
    case kw_const:
    case kw_volatile:
#ifdef TOPSPEED
    case kw_cdecl:
#endif /* TOPSPEED */
    case kw_restrict:
	return TRUE;
    default:
	break;
    }
    return FALSE;
}

/*
 *   Return true if the token is one which starts a declaration
 *   specifier, otherwise it returns false.
 */

BOOL is_declaration_specifier P1 (TOKEN, st)
{
    return is_type_specifier (st) ||
	is_storage_class (st) || is_type_qualifier (st);
}


/*
 *   Returns true if the token is one which starts a specifier
 *   qualifier list, otherwise it returns false.
 */

static BOOL is_specifier_qualifier P1 (TOKEN, st)
{
    return is_type_specifier (st) || is_type_qualifier (st);
}


/*
 *   Returns true if the token is one which starts a type
 *   name, otherwise it returns false.
 */

BOOL is_type_name P1 (TOKEN, st)
{
    return is_specifier_qualifier (st);
}

/*
 *   Returns true if the integer constant can be represented by
 *   the specified type, otherwise it returns false.
 */

BOOL is_representable P2 (IVAL, i, const TYP *, tp)
{
    switch (tp->type) {
    case bt_char:
    case bt_schar:
	return (i >= -128L && i < 128L);
    case bt_uchar:
    case bt_charu:
	return (i >= 0L && i < 256L);
    case bt_short:
    case bt_int16:
	return (i >= -32768L && i < 32768L);
    case bt_ushort:
    case bt_uint16:
	return (i >= 0L && i < 65536L);
	
#ifdef LONGLONG_SUPPORT
#ifdef LONGLONG_BOOTSTRAP
    case bt_int32:
    case bt_long:
	return TRUE;
    case bt_uint32:
    case bt_ulong:
	return TRUE;
	
#else	/*  */
    case bt_int32:
    case bt_long:
	return (i >= -(IVAL) 2147483648L && i < (IVAL) 2147483647L);
    case bt_uint32:
    case bt_ulong:
	return (i >= 0L && i < (IVAL) 0x100000000L);
	
#endif	/* LONGLONG_BOOTSTRAP */
#else
    case bt_int32:
    case bt_long:
	return TRUE;
    case bt_uint32:
    case bt_ulong:
	return TRUE;
	
#endif	/* LONGLONG_SUPPORT */
    case bt_longlong:
	return TRUE;
    case bt_ulonglong:
	return TRUE;
    default:
	break;
    }
    return FALSE;
}

/*
 *   Returns true if the current token is a typedef name,
 *   otherwise it returns false.
 */

static BOOL is_typedef_symbol P1 (const CHAR *, str)
{
    SYM    *sp = sym_search (str);

    return ((sp != NIL_SYM) && is_typedef (sp));
}

#ifndef SYNTAX_CORRECT
/*
 *   If the parameter parmN is declared with the register storage
 *   class, with a function or array type, or with a type that is not
 *   compatible with the type that results after application of the
 *   default promotions, the behaviour is undefined.
 */

static void check_stdarg P1 (const SYM *, sp)
{
    TYP    *tp = typeof (sp);

    if (is_register (sp) || is_array_type (tp)) {
	message (WARN_STDARG);
    } else if (is_function_type (tp)) {
	message (WARN_STDARG2);
    } else {
	switch (tp->type) {
	case bt_char:
	case bt_uchar:
	case bt_schar:
	case bt_charu:
	case bt_short:
	case bt_ushort:
	case bt_float:
	    message (WARN_STDARG);
	    break;
	default:
	    break;
	}
    }
}

static void check_tag P2 (const SYM *, sp, const TYP *, tp)
{
    switch (typeof (sp)->type) {
    case bt_struct:
    case bt_union:
	if (is_same_type (typeof (sp), tp)) {
	    return;
	}
	break;
    case bt_schar:
    case bt_uchar:
    case bt_short:
    case bt_ushort:
    case bt_int16:
    case bt_uint16:
    case bt_int32:
    case bt_uint32:
    case bt_long:
    case bt_ulong:
	if (is_same_type (tp_int, tp)) {
	    return;
	}
	break;
    default:
	break;
    }
    message (ERR_TAG, nameof (sp));
}

static void check_function_declaration P1 (const SYM *, sp)
{
    /*
     *       The storage-specifier, if any, in the declaration specifiers
     *       shall be either extern or static.
     */

    switch (storageof (sp)) {
	TYP    *tp;

    case sc_global:		/* no specifier */
    case sc_external:		/* extern specifier */
    case sc_static:		/* static specifier */
	tp = typeof (sp);
	if (tp != NULL) {
	    tp = returned_type (typeof (sp));
#ifndef SYNTAX_CORRECT
	    if (tp != NULL && !is_void (tp)) {
		check_complete (tp);
	    }
#endif /* SYNTAX_CORRECT */
	}
	break;
    default:
	message (ERR_ILLCLASS);
    }
}

static void check_function_definition P1 (const SYM *, sp)
{
    /*
     *       The return type of a function shall be void or an object type
     *       other than array.
     */

    TYP    *tp = returned_type (typeof (sp));

    switch (tp->type) {
    case bt_func:
	message (ERR_FUNC);
	break;
    case bt_pointer16:
    case bt_pointer32:
	if (is_array_type (tp)) {
	    message (ERR_ARRAYRETURN);
	}
	break;
    case bt_void:
	if (is_qualified_type (tp)) {
	    message (WARN_QUALIFIER);
	}
	break;
    default:
	break;
    }
}


/*
 *   Checks that i has a value representable as a 'tp'
 */
static void check_representable P2 (IVAL, i, const TYP *, tp)
{
    if (!is_representable (i, tp)) {
	message (ERR_REPRESENT, nameoftype (tp));
    }
}

/*
 *   Checks that the parameters in 'block' match those in any
 *   function prototype for 'sp'.
 */
static void check_parameters P2 (const BLOCK *, block, const SYM *, sp)
{
    SYM    *sp1, *sp2 = sym_search (nameof (sp));

    if ((sp2 == NIL_SYM) || !is_func (typeof (sp2))) {
	return;			/* no function definition */
    }
    sp1 = symbolsof (block);
    sp2 = parametersof (typeof (sp2));
    if (sp2 == NIL_SYM) {
	return;			/* no prototype on function definition */
    }
    if (sp1 == NIL_SYM) {
	if (!is_equal_type (tp_void, typeof (sp2))) {
	    message (ERR_PROTO, nameof (sp));
	}
	sp2 = nextsym (sp2);
    }
    while ((sp1 != NIL_SYM) && (sp2 != NIL_SYM)) {
	if (!is_equal_type
	    (promote_type (typeof (sp1)), promote_type (typeof (sp2)))) {
	    message (ERR_PROTO, nameof (sp));
	} else if (!is_equal_type (promote_type (typeof (sp1)), typeof (sp2))) {
	    message (ERR_PROTODEF, nameof (sp));
	}
	sp1 = nextsym (sp1);
	sp2 = nextsym (sp2);
    }
    if (sp1 != sp2) {
	message (ERR_PROTONUM, nameof (sp));
    }
}
#endif /* SYNTAX_CORRECT */

/*
 * Adds the symbol to the symbol table.
 */
static SYM *declare P2 (SYM *, sp, STORAGE, sc)
{
    BOOL    func_body;

    switch (typeof (sp)->type) {
    case bt_func:
	func_body = (lastst == tk_begin || lastst == kw_register ||
		     lastst == kw_auto || is_type_name (lastst));

#ifndef SYNTAX_CORRECT
	switch (fspecifierof (sp)) {
	case fs_inline:
	    if (nameof (sp) == main_name)
	    {
		message (ERR_INLINEMAIN);
	    }
	    break;
	case fs_none:
	    break;
	default:
	    CANNOT_REACH_HERE ();
	    break;
	}
#endif /* SYNTAX_CORRECT */

	switch (storageof (sp)) {
	case sc_external:
	    if (func_body) {
		set_storage (sp, sc_global);
	    }
	    break;
	case sc_global:
	    if (!func_body) {
		set_storage (sp, sc_external);
	    }
	    break;
	case sc_auto:
	    set_storage (sp, sc_external);
	    break;
	case sc_static:
	case sc_typedef:
	    break;
	default:
	    CANNOT_REACH_HERE ();
	    break;
	}
#ifndef SYNTAX_CORRECT
	if (sc == sc_auto) {
	    switch (storageof (sp)) {
	    default:
		message (ERR_ILLCLASS);
		/*lint -fallthrough */
	    case sc_external:
	    case sc_typedef:
		break;
	    }
	}
#endif /* SYNTAX_CORRECT */
	break;
    default:
	break;
    }
    sym_append (&sp);
    return sp;
}


SYM    *identifier P0 (void)
{
    SYM    *sp = sym_search (lastsym);

    if (sp != NIL_SYM) {
#ifndef SYNTAX_CORRECT
	if (is_symbol_outscope (sp)) {
	    message (WARN_OUTSCOPE, nameof (sp));
	}
#endif /* SYNTAX_CORRECT */
	getsym ();
	return sp;
    }

    if (lastsym == func_name && lang_option >= LANG_C99) {
	if (func_symbol) {
	    sp = mk_sym (lastsym, sc_static, copy_type (tp_conststring));
	    getsym ();
	    sym_append (&sp);
	    initfuncid (sp, func_symbol);
	    return sp;
#ifndef SYNTAX_CORRECT
	} else {
	    message (ERR_FUNCID);
#endif /* SYNTAX_CORRECT */
	}
    }
    sp = mk_sym (lastsym, sc_external, tp_long);
    getsym ();
    switch (lastst) {
    case tk_openpa:
	set_type (sp, tp_func);
#ifndef SYNTAX_CORRECT
	if (lang_option >= LANG_C99) {
	    message (ERR_UNDEFINEDFN, nameof (sp));
	} else {
	    message (WARN_IMPLICITFN, nameof (sp));
	}
	break;
#endif /* SYNTAX_CORRECT */
    default:
#ifndef SYNTAX_CORRECT
	message (ERR_UNDEFINED, nameof (sp));
#endif /* SYNTAX_CORRECT */
	break;
    }
    sym_append (&sp);
    return sp;
}


/*ANSI 3.6.2
 *    declaration-list:
 *              declaration
 *              declaration-list declaration
 */
SIZE declaration_list P2 (STORAGE, def_sc, SIZE, offset)
{
    while (is_declaration_specifier (lastst)) {
	if ((def_sc != sc_member) && is_label (lastst)) {
	    return offset;
	}
	offset = declaration (def_sc, def_sc, offset);
    }
    return offset;
}


/*ANSI 3.5.4
 *    identifier-list:
 *              identifier
 *              identifier-list , identifier
 */
static void identifier_list P0 (void)
{
    SYM    *sp;

    global_flag--;
    assert (lastst == tk_id);
    for (;;) {
	switch (lastst) {
	case tk_id:
	    sp = mk_sym (lastsym, sc_parms, NIL_TYP);
	    getsym ();
	    sym_append (&sp);
	    break;
	default:
#ifndef SYNTAX_CORRECT
	    message (ERR_PARMS);
#endif /* SYNTAX_CORRECT */
	    global_flag++;
	    return;
	}
	if (lastst != tk_comma) {
	    global_flag++;
	    return;
	}
	getsym ();
    }
}


/*ANSI 3.5.5
 *    type-name:
 *              specifier-qualifier-list abstract-declarator[opt]
 */
TYP    *type_name P0 (void)
{
    TYP    *tp = NIL_TYP;
    SYM    *sp = NIL_SYM;
    STORAGE sc;

    if (is_type_name (lastst)) {
	sc = sc_tag;
	tp = declaration_specifiers (&sc, sc_global, NULL);
	switch (lastst) {
	case tk_star:
	case tk_openbr:
	case tk_openpa:
	    sp = declarator (sc, tp);
	    tp = typeof (sp);
	    break;
	default:
	    break;
	}
    }
    return tp;
}


/*ANSI 3.5.4
 *    parameter-type-list:
 *              parameter-list
 *              parameter-list , ...
 *
 *      parameter-list:
 *              parameter-declaration
 *              parameter-list , parameter-declaration
 *
 *      parameter-declaration:
 *              declaration-specifiers declarator
 *              declaration-specifiers abstract-declarator[opt]
 */
static void parameter_type_list P0 (void)
{
    TYP    *tp;
    SYM    *sp;
    STORAGE sc;

    global_flag++;		/* allocate into global heap */
    while (is_declaration_specifier (lastst)) {
	sc = sc_parms;
	tp = declaration_specifiers (&sc, sc_parms, NULL);
	if (sc == sc_auto) {
	    sc = sc_parms;
	}
	sp = declarator (sc, tp);
	if (is_func (typeof (sp))) {
	    /*
	     * Functions which are passed as a parameter are really
	     * pointers to a function.
	     */
	    set_type (sp, mk_type (tp_pointer, typeof (sp)));
	}
	sym_append (&sp);
	if (lastst != tk_comma || is_void (typeof (sp))) {
	    break;
	}
	getsym ();
	switch (lastst) {
	case tk_ellipsis:
	    getsym ();
	    check_stdarg (sp);
	    tp = mk_type (tp_ellipsis, NIL_TYP);
	    sp = mk_sym ((const CHAR *) "", sc_tag, tp);
	    sym_append (&sp);
	    global_flag--;
	    return;
	case tk_closepa:
	    if (lattice_option) {
		tp = mk_type (tp_ellipsis, NIL_TYP);
		sp = mk_sym ((const CHAR *) "", sc_tag, tp);
		sym_append (&sp);
		global_flag--;
		return;
	    }
	    break;
	default:
	    break;
	}
    }
    global_flag--;
    return;
}

/*ANSI 3.5.2.2
 *    enumeration-list:
 *              enumerator
 *              enumerator-list , enumerator
 *
 *      enumerator:
 *              enumeration-constant
 *              enumeration-constant = constant-expression
 *
 *      enumeration-constant:
 *              identifier
 */
static TYP *enumeration_list P0 (void)
{
    IVAL    evalue, minval, maxval;
    int     count;
    TYP    *tp = mk_type (tp_enum, NIL_TYP);
    TYP    *tp2 = mk_type (tp_int, NIL_TYP);

    set_derived (tp2);
    set_enum (tp2);
    set_enumtype (tp2, tp);
    for (count = 0, minval = maxval = 0L, evalue = -1L; lastst == tk_id;) {
	SYM    *sp = mk_sym (lastsym, sc_const, tp2);

	getsym ();
	if (lastst == tk_assign) {
	    getsym ();
	    evalue = intexpr ();
	} else {
#ifndef SYNTAX_CORRECT
	    switch (tp_int->size) {
	    case 2L:
		if (evalue == 32767L)
		    message (ERR_ENUMWRAP);
		break;
	    case 4L:
		if (evalue == 2147483647L)
		    message (ERR_ENUMWRAP);
		break;
		
#ifdef LONGLONG
#ifndef LONGLONG_BOOTSTRAP
#if 0
	    case 8L:
		if (evalue == 9223372036854775807LL)
		    message (ERR_ENUMWRAP);
		break;
		
#endif	/*  */
#endif	/* LONGLONG_BOOTSTRAP */
#endif	/* LONGLONG */
	    default:
		break;
	    }
#endif /* SYNTAX_CORRECT */
	    evalue++;
	}
	/*
	 *Constraint: The expression that defines the value of an enumeration
	 *            constant shall be an integral constant expression that
	 *            has a value representable as an int.
	 */
	check_representable (evalue, tp_int);
	if (evalue < minval) {
	    minval = evalue;
	}
	if (evalue > maxval) {
	    maxval = evalue;
	}
	sp->value.i = evalue;
	sym_append (&sp);
	count++;
	if (lastst != tk_comma) {
	    break;
	}
	getsym ();
    }
#ifndef SYNTAX_CORRECT
    if (count == 0) {
	message (ERR_INCOMPLETE, "enum");
    }
#endif /* SYNTAX_CORRECT */
    /* now determine the best integer type to hold the enumeration */
#ifdef PACKENUM
    if (packenum_option) {
	if (minval >= -128L && maxval <= 127L) {
	    tp2 = tp_schar;
	} else if (minval >= 0L && maxval <= 256L) {
	    tp2 = tp_uchar;
	} else if (minval >= -32768L && maxval <= 32767L) {
	    tp2 = tp_short;
	} else if (minval >= 0L && maxval <= 65535L) {
	    tp2 = tp_ushort;
	} else {
	    tp2 = tp_long;
	}
    } else
#endif /* PACKENUM */
	tp2 = tp_int;
    tp->type = tp2->type;
    tp->size = tp2->size;
    set_enum (tp);
    set_enumtype (tp, tp);
    return tp;
}



/*ANSI 3.5.2.2
 *    enum-specifier:
 *              "enum" identifier[opt] { enumeration-list }
 *              "enum" identifier
 */
static TYP *enum_specifier P1 (STORAGE, sc)
{
    TYP    *tp;
    SYM    *sp;

    assert (lastst == kw_enum);
    getsym ();
    switch (lastst) {
    case tk_begin:
	getsym ();
	tp = enumeration_list ();
	needpunc (tk_end);
	break;
    case tk_id:
	if ((sp = tag_search (lastsym)) == NIL_SYM) {
	    sp = mk_sym (lastsym, sc_tag, tp_enum);
	    tag_append (&sp);
#ifndef SYNTAX_CORRECT
	    if (sc == sc_parms) {
		message (WARN_DUBIOUS, "enum");
	    }
#endif /* SYNTAX_CORRECT */
	    getsym ();
	    if (lastst == tk_begin) {
		getsym ();
		tp = enumeration_list ();
		set_type (sp, tp);
		set_nameoftype (tp, nameof (sp));
		needpunc (tk_end);
#ifndef SYNTAX_CORRECT
	    } else {
		tp = tp_int;
		message (ERR_INCOMPLETE, "enum");
		break;
#endif /* SYNTAX_CORRECT */
	    }
	} else {
	    getsym ();
	    if (is_local_scope (sp) || lastst != tk_begin) {
		check_tag (sp, tp_int);
		tp = typeof (sp);
#ifndef SYNTAX_CORRECT
		if (lastst == tk_semicolon) {
		    message (ERR_INCOMPLETE, "enum");
		}
#endif /* SYNTAX_CORRECT */
	    } else {
		getsym ();
		tp = enumeration_list ();
		set_type (sp, tp);
		set_nameoftype (tp, nameof (sp));
		needpunc (tk_end);
	    }
	}
	break;
    default:
#ifndef SYNTAX_CORRECT
	tp = tp_int;
	message (ERR_INCOMPLETE, "enum");
#endif /* SYNTAX_CORRECT */
	break;
    }
    return tp;
}


static int bit_width, bit_offset, bit_next;

/*ANSI 3.5.2.1
 *    struct-declarator-list:
 *              struct-declarator
 *              struct-declarator-list , struct-declarator
 *ANSI 3.5.2.1
 *      struct-declarator:
 *              declarator
 *              declarator[opt] : constant-expression
 */
static SIZE struct_declarator_list P3 (TYP *, tp, SIZE, offset, const TYP *, stp)
{
    SIZE    size, al;
    int     int_bits;
    TYP   *tp_bit = tp_int;

    for (size = 0L;;) {
	SYM    *sp = NIL_SYM;

	switch (lastst) {
	case tk_id:
	case tk_star:
	case tk_openpa:
	    sp = declarator (sc_member, tp);
#ifndef SYNTAX_CORRECT
	    check_complete (typeof (sp));
	    if (is_func (typeof (sp))) {
		message (ERR_FUNC);
	    }
#endif /* SYNTAX_CORRECT */
	    if (lastst != tk_colon) {
		switch (stp->type) {
		case bt_struct:
		    al = align (typeof (sp), offset + size);
		    sp->value.i = (IVAL) (offset + size + al);	/*lint !e776*/	/* possible truncation of addition */
		    field_append (&sp);
		    size += typeof (sp)->size + al;
		    break;
		case bt_union:
		    al = align (typeof (sp), offset);
		    sp->value.i = (IVAL) (offset + al);	/*lint !e776*/	/* possible truncation of addition */
		    field_append (&sp);
		    size = typeof (sp)->size + al;
		    break;
		default:
		    CANNOT_REACH_HERE ();
		}
		bit_next = 0;
		break;
	    }
	    tp = typeof (sp);
	    /*lint -fallthrough */
	case tk_colon:
	    getsym ();
	    /*
	     *Semantic:   A bit-field shall have type int, unsigned int, or
	     *            signed int.
	     */
#ifndef SYNTAX_CORRECT
	    if (!is_same_type (tp, tp_int)
		&& !is_same_type (tp, tp_uint)
		&& !is_same_type (tp, tp_bool)) {
		if (tp->size > tp_int->size) {
		    message (ERR_FLDTYPE);
		} else {
		    message (WARN_FLDTYPE);
		}
	    }
#endif /* SYNTAX_CORRECT */
	    bit_width = (int) intexpr ();
	    bit_offset = bit_next;
	    if (packbit_option)
		tp_bit = tp;

	    /*
	     *Constraint: The expression that specified the width of a bit-field
	     *            shall be an integral constant expression that has
	     *            nonnegative value that shall not exceed the number
	     *            of bits in an ordinary object of compatible type.
	     */
	    int_bits = (int) (tp_bit->size * bits_in_sizeunit);
#ifndef SYNTAX_CORRECT
	    if (bit_width < 0 || bit_width > int_bits) {
		message (ERR_WIDTH);
		bit_width = 1;
	    }
#endif /* SYNTAX_CORRECT */
	    /*ANSI 3.5.2.1
	     *Constraint: If the value is zero, the declaration shall have
	     *            no declarator.
	     *
	     *Semantic:   A bit-field structure member with a width of 0
	     *            indicates that no further bit-field is to be
	     *            packed into the unit in which the previous
	     *            bit-field, if any, was placed.
	     */
	    if (bit_width == 0 || bit_offset + bit_width > int_bits) {
		bit_offset = 0;
	    }
	    bit_next = bit_offset + bit_width;
	    if (sp != NIL_SYM) {
#ifndef SYNTAX_CORRECT
		if (bit_width == 0) {
		    message (ERR_WIDTH);
		}
#endif /* SYNTAX_CORRECT */
		set_type (sp, copy_type (tp));
		typeof (sp)->type =
		    (BTYPE) (is_signed_type (tp) ? bt_bitfield :
			     (is_bool (tp) ? bt_bool : bt_ubitfield));
		typeof (sp)->size = tp_bit->size;
		set_bit_width (typeof (sp), (BITSIZE) bit_width);
		if (bitfield_option) {
		    set_bit_offset (typeof (sp),
				    (BITSIZE) ((int_bits - bit_offset) -
					       bit_width));
		} else {
		    set_bit_offset (typeof (sp), (BITSIZE) bit_offset);
		}
		if (bit_offset > 0) {
		    offset -= tp_bit->size;	/* shares space with previous field */
		}
		al = align (typeof (sp), offset + size);
		sp->value.i = (IVAL) (offset + size + al);	/*lint !e776*/	/* possible truncation of addition */
		field_append (&sp);
		size += typeof (sp)->size + al;
		if (bit_offset > 0) {
		    size -= tp_bit->size;
		}
	    } else {
		if (bit_offset > 0) {
		    offset -= tp_bit->size;	/* shares space with previous field */
		}
		al = align (tp_bit, offset + size);
		size += tp_bit->size + al;
		if (bit_offset > 0) {
		    size -= tp_bit->size;
		}
	    }

	    break;
	case tk_comma:
	    needpunc ((TOKEN) (lastst + 1));	/* force error */
	    break;
	default:
	    break;
	}
	if (lastst != tk_comma) {
	    break;
	}
	getsym ();
    }
    return size;
}

static BOOL is_struct_declarator P1 (TOKEN, st)
{
    switch (st) {
    case tk_colon:
	return TRUE;
    default:
	return is_specifier_qualifier (st);
    }
}

/*ANSI 3.5.2.1
 *    struct-declaration-list:
 *              struct-declaration
 *              struct-declaration-list struct-declaration
 *
 *ANSI .5.2.1
 *      struct-declaration:
 *              specifier-qualifier-list struct-declarator-list ;
 */
static SIZE struct_declaration_list P1 (const TYP *, tp)
{
    SIZE    offset = 0L;
    SIZE    max_size = UNKNOWN_SIZE;
    int     count;

    beginstructblock (members (tp));
    bit_next = 0;
    bit_offset = 0;
    for (count = 0; is_struct_declarator (lastst); count++) {
	TYP    *tp1;
	SIZE    size;
	STORAGE sc = sc_member;

	tp1 = declaration_specifiers (&sc, sc_member, NULL);
	size = struct_declarator_list (tp1, offset, tp);
	if (offset + size > max_size) {
	    max_size = offset + size;
	}
	switch (tp->type) {
	case bt_struct:
	    offset += size;
	    break;
	case bt_union:
	    break;
	default:
	    CANNOT_REACH_HERE ();
	}
	needpunc (tk_semicolon);
    }
#ifndef SYNTAX_CORRECT
    if (count == 0) {
	message (ERR_INCOMPLETE, nameoftype (tp));
    }
#endif /* SYNTAX_CORRECT */
    set_members (tp, endstructblock ());
    return max_size;
}


/*ANSI3.5.2.1
 *    struct-or-union-specifier:
 *              struct-or-union identifier[opt] { struct-declaration-list }
 *              struct-or-union identifier
 *
 *    struct-or-union:
 *              "struct"
 *              "union"
 */
static TYP *struct_or_union_specifier P1 (STORAGE, sc)
{
    TYP    *tp = NIL_TYP;
    SYM    *sp;
    SIZE    size;

    switch (lastst) {
    case kw_struct:
	tp = tp_struct;
	break;
    case kw_union:
	tp = tp_union;
	break;
    default:
	CANNOT_REACH_HERE ();
	break;
    }
    getsym ();

    switch (lastst) {
    case tk_id:
	if ((sp = tag_search (lastsym)) == NIL_SYM) {
	    tp = mk_type (tp, NIL_TYP);
	    sp = mk_sym (lastsym, sc_tag, tp);
	    getsym ();
	    set_nameoftype (tp, nameof (sp));
	    tag_append (&sp);
#ifndef SYNTAX_CORRECT
	    if (sc == sc_parms) {
		message (WARN_DUBIOUS, nameoftype (tp));
	    }
#endif /* SYNTAX_CORRECT */
	    beginstructblock (members (tp));
	    set_members (tp, endstructblock ());
	} else {
	    getsym ();
	    if (is_local_scope (sp)) {
		check_tag (sp, tp);
	    } else {
		switch (lastst) {
		case tk_semicolon:
		    /* ANSI 3.5.2.3
		     *        A declaration of the form
		     *          struct-or-union identifier ;
		     *  specifies a structure or union and declares a tag
		     *  both visible only within the scope in which the
		     *  declaration occurs.  It declares a new type distinct
		     *  from any type with the same tag in an enclosing scope
		     *  (if any).
		     */
		    tp = mk_type (tp, NIL_TYP);
		    sp = mk_sym (nameof (sp), sc_tag, tp);
		    set_nameoftype (tp, nameof (sp));
		    tag_append (&sp);
		    beginstructblock (NIL_BLOCK);
		    set_members (tp, endstructblock ());
		    return tp;
		case tk_begin:
		    /*
		     * Defining a new structure/union which will hide the one
		     * at an outer scope.
		     */
		    tp = mk_type (tp, NIL_TYP);
		    sp = mk_sym (nameof (sp), sc_tag, tp);
		    set_nameoftype (tp, nameof (sp));
		    tag_append (&sp);
#ifndef SYNTAX_CORRECT
		    if (sc == sc_parms) {
			message (WARN_DUBIOUS, nameoftype (tp));
		    }
#endif /* SYNTAX_CORRECT */
		    beginstructblock (NIL_BLOCK);
		    set_members (tp, endstructblock ());
		    break;
		default:
		    break;
		}
	    }
	}
	tp = typeof (sp);
	size = tp->size;
	if (lastst == tk_begin) {
#ifndef SYNTAX_CORRECT
	    if (is_symbol_defined (sp)) {
		message (ERR_REDECL, nameof (sp));
	    }
#endif /* SYNTAX_CORRECT */
	    getsym ();
	    size = struct_declaration_list (tp);
	    needpunc (tk_end);
	    symbol_defined (sp);
	}
	break;
    case tk_begin:
#ifndef SYNTAX_CORRECT
	if (sc == sc_parms) {
	    message (WARN_DUBIOUS, nameoftype (tp));
	}
#endif /* SYNTAX_CORRECT */
	getsym ();
	tp = mk_type (tp, NIL_TYP);
	size = struct_declaration_list (tp);
	needpunc (tk_end);
	break;
    default:
#ifndef SYNTAX_CORRECT
	message (ERR_INCOMPLETE, nameoftype (tp));
	tp = mk_type (tp, NIL_TYP);
#endif /* SYNTAX_CORRECT */
	return tp;
    }
    if (size != UNKNOWN_SIZE) {
	size += align (tp, size);
    }
    tp->size = size;
    return tp;
}


/*ANSI 3.5.1
 *    storage-class-specifier:
 *              "typedef"
 *              "extern"
 *              "static"
 *              "auto"
 *              "register"
 *
 * Only one storage-class is allowed on a declaration.
 */

static STORAGE storage_class_specifier P2 (STORAGE, sc, BOOL, sc_set)
{
    /*
     *Constraint: At most one storage-class specifier may be given in the
     *            declaration specifiers in a declaration.
     */
#ifndef SYNTAX_CORRECT
    if (sc_set) {
	message (ERR_ILLCLASS);
    }
#endif /* SYNTAX_CORRECT */
    switch (lastst) {
    case kw_typedef:
	sc = sc_typedef;
	break;
    case kw_extern:
	sc = sc_external;
	break;
    case kw_static:
	sc = sc_static;
	break;
    case kw_auto:
	sc = sc_auto;
	break;
    case kw_register:
	sc = sc_register;
	break;
    default:
	CANNOT_REACH_HERE ();
	break;
    }
    getsym ();
    return sc;
}

static FSPECIFIER function_specifier P1 (FSPECIFIER, fs)
{
    switch (lastst) {
    case kw_inline:
	fs = fs_inline;
	break;
    default:
	CANNOT_REACH_HERE ();
	break;
    }
    getsym ();
    return fs;
}

/*ANSI 3.5.3
 *    type_qualifier:
 *              "const"
 *              "volatile"
 */

static QUALIFIER type_qualifier P1 (QUALIFIER, qual)
{
    QUALIFIER q = QUAL_NONE;;

    switch (lastst) {
    case kw_const:
	q = QUAL_CONST;
	break;
    case kw_volatile:
	q = QUAL_VOLATILE;
	break;
#ifdef TOPSPEED
    case kw_cdecl:
	q = QUAL_NONE;
	break;
#endif /* TOPSPEED */
    case kw_restrict:
	q = QUAL_RESTRICT;
	break;
    default:
	CANNOT_REACH_HERE ();
	break;
    }
    /*
     *Constraint: The same type qualifier shall not appear more than once in
     *            the same specifier list or qualifier list.
     */
#ifndef SYNTAX_CORRECT
    if (qual & q) {
	message (ERR_QUALIFIER);
    }
#endif /* SYNTAX_CORRECT */
    getsym ();
    return (QUALIFIER) (qual | q);
}


/*ANSI 3.5
 *    declaration-specifiers:
 *              storage-class-specifier declaration-specifiers[opt]
 *              type-specifier declaration-specifiers[opt]
 *              type-qualifier declaration-specifiers[opt]
 *ANSI 3.5.2.1
 *      specifier-qualifier-list:
 *              type-specifier specifier-qualifier-list[opt]
 *              type-qualifier specifier-qualifier-list[opt]
 *
 * These definitions can be combined because the "storage" for the
 * specifier-qualifier-list is "sc_member" which ensures that no
 * storage-class-specifier can be selected for this definition.
 *
 *ANSI 3.5.2
 *      type-specifier:
 *              "void"
 *              "char"
 *              "short"
 *              "int"
 *              "long"
 *              "float"
 *              "double"
 *              "signed"
 *              "unsigned"
 *              struct-or-union-specifier
 *              enum-specifier
 *              typedef-name
 *
 *    class-specifier-qualifier-list:
 *              type-specifier class-specifier-qualifier-list[opt]
 *              type-qualifier class-specifier-qualifier-list[opt]
 *              visibility class-specifier-qualifier-list[opt]
 *
 *    visibility:
 *              "public"
 *              "private"
 */
static TYP *declaration_specifiers P3 (STORAGE *, storage, STORAGE, def_sc, FSPECIFIER *, fspecifier)
{
    QUALIFIER qualifier = QUAL_NONE;
    unsigned ts, type = (unsigned) 0;
    int     count;
    TYP    *tp = NIL_TYP;
    SYM    *sp;
    BOOL    done, sc_set = FALSE;

    for (count = 0, done = FALSE; !done; count++) {
	switch (lastst) {
	    /* storage class specifiers */
	case kw_extern:
	case kw_static:
	case kw_auto:
	case kw_typedef:
#ifndef SYNTAX_CORRECT
	    if (def_sc == sc_parms) {
		message (ERR_ILLCLASS);
	    }
	    /*lint -fallthrough */
#endif /* SYNTAX_CORRECT */
	case kw_register:
#ifndef SYNTAX_CORRECT
	    if ((count != 0) && obsolete_option) {
		message (WARN_STORAGE);
	    }
#endif /* SYNTAX_CORRECT */
	    *storage = storage_class_specifier (*storage, sc_set);
	    sc_set = TRUE;
	    continue;

	case kw_inline:
	    if (fspecifier) {
		*fspecifier = function_specifier (*fspecifier);
#ifndef SYNTAX_CORRECT
	    } else {
		message (ERR_ILLINLINE);
		getsym ();
#endif /* SYNTAX_CORRECT */
	    }
	    continue;

	    /* type specifiers */
	case tk_id:
	    if (type) {
		done = TRUE;
		continue;
	    }
	    sp = sym_search (lastsym);
	    if (sp == NIL_SYM || !is_typedef (sp)) {
		done = TRUE;
		continue;
	    }
	    tp = typeof (sp);
#ifndef SYNTAX_CORRECT
	    if (tp->qual & qualifier) {
		message (ERR_QUALIFIER);
	    }
#endif /* SYNTAX_CORRECT */
	    qualifier |= tp->qual;

	    /*
	     *       Typdef'd arrays of incomplete types must be copied as
	     *       their use must not complete the typedef type.
	     */
	    if (is_array_type (tp) && is_incomplete_type (tp)) {
		tp = copy_type (tp);
	    }
	    getsym ();
	    type = T_ALL;
	    continue;
	case kw_void:
	    ts = T_VOID;
	    break;
	case kw_char:
	    ts = T_CHAR;
	    break;
	case kw_short:
	    ts = T_SHORT;
	    break;
	case kw_int:
	    ts = T_INT;
	    break;
	case kw_long:
	    if (lang_option >= LANG_C99) {
		ts = (type & T_LONG) ? T_LONGLONG : T_LONG;
	    } else {
		ts = T_LONG;
	    }
	    break;
	case kw_float:
	    ts = T_FLOAT;
	    break;
	case kw_double:
	    ts = T_DOUBLE;
	    break;
	case kw_signed:
	    ts = T_SIGNED;
	    break;
	case kw_unsigned:
	    ts = T_UNSIGNED;
	    break;
	case kw_bool:
	    ts = T_BOOL;
	    break;
	case kw_complex:
	    ts = T_COMPLEX;
	    break;
	case kw_imaginary:
	    ts = T_IMAGINARY;
	    break;
	case kw_struct:
	case kw_union:
#ifndef SYNTAX_CORRECT
	    if (type) {
		message (ERR_ILLTYPE);
	    }
#endif /* SYNTAX_CORRECT */
	    tp = struct_or_union_specifier (def_sc);
	    type = T_ALL;
	    continue;
	case kw_enum:
#ifndef SYNTAX_CORRECT
	    if (type) {
		message (ERR_ILLTYPE);
	    }
#endif /* SYNTAX_CORRECT */
	    tp = enum_specifier (def_sc);
	    type = T_ALL;
	    continue;

	case kw_restrict:
#ifndef SYNTAX_CORRECT
	    message (ERR_RESTRICT);
	    /*lint -fallthrough */
#endif /* SYNTAX_CORRECT */

	    /* type qualifiers */
	case kw_const:
	case kw_volatile:
#ifdef TOPSPEED
	case kw_cdecl:
#endif /* TOPSPEED */
	    qualifier = type_qualifier (qualifier);
	    continue;
	default:
	    done = TRUE;
	    continue;
	}
	if (type == T_ALL) {
	    break;
	}
#ifndef SYNTAX_CORRECT
	if (ts & type) {
	    message (ERR_TYPE, lastsym);
	}
#endif /* SYNTAX_CORRECT */
	type |= ts;
	getsym ();
    }
    switch (type) {
    case T_VOID:
	tp = tp_void;
	break;
    case 0:
	if (lang_option >= LANG_C99) {
#ifndef SYNTAX_CORRECT
	    message (ERR_IMPLICITINT);
#endif /* SYNTAX_CORRECT */
	}
	/*lint -fallthrough */
    case T_INT:
    case T_SIGNED:
    case T_INT | T_SIGNED:
	tp = tp_int;
	break;
    case T_UNSIGNED:
    case T_INT | T_UNSIGNED:
	tp = tp_uint;
	break;
    case T_CHAR:
	tp = tp_char;
	break;
    case T_CHAR | T_UNSIGNED:
	tp = tp_uchar;
	break;
    case T_CHAR | T_SIGNED:
	tp = tp_schar;
	break;
    case T_SHORT:
    case T_SHORT | T_SIGNED:
    case T_SHORT | T_INT:
    case T_SHORT | T_INT | T_SIGNED:
	tp = tp_short;
	break;
    case T_SHORT | T_UNSIGNED:
    case T_SHORT | T_INT | T_UNSIGNED:
	tp = tp_ushort;
	break;
    case T_LONG:
    case T_LONG | T_INT:
    case T_LONG | T_SIGNED:
    case T_LONG | T_INT | T_SIGNED:
	tp = tp_long;
	break;
    case T_LONG | T_UNSIGNED:
    case T_LONG | T_INT | T_UNSIGNED:
	tp = tp_ulong;
	break;
    case T_LONGLONG | T_LONG:
    case T_LONGLONG | T_LONG | T_INT:
    case T_LONGLONG | T_LONG | T_SIGNED:
    case T_LONGLONG | T_LONG | T_INT | T_SIGNED:
	tp = tp_longlong;
	break;
    case T_LONGLONG | T_LONG | T_UNSIGNED:
    case T_LONGLONG | T_LONG | T_INT | T_UNSIGNED:
	tp = tp_ulonglong;
	break;
    case T_FLOAT:
	tp = tp_float;
	break;
    case T_DOUBLE:
	tp = tp_double;
	break;
    case T_DOUBLE | T_LONG:
	tp = tp_longdouble;
	break;
    case T_BOOL:
	tp = tp_bool;
	break;
    case T_FLOAT | T_COMPLEX:
	tp = tp_floatcomplex;
	break;
    case T_FLOAT | T_IMAGINARY:
	tp = tp_floatimaginary;
	break;
    case T_DOUBLE | T_COMPLEX:
	tp = tp_doublecomplex;
	break;
    case T_DOUBLE | T_IMAGINARY:
	tp = tp_doubleimaginary;
	break;
    case T_LONG | T_DOUBLE | T_COMPLEX:
	tp = tp_longdoublecomplex;
	break;
    case T_LONG | T_DOUBLE | T_IMAGINARY:
	tp = tp_longdoubleimaginary;
	break;
    case T_ALL:
	break;
    default:
#ifndef SYNTAX_CORRECT
	message (ERR_ILLTYPE);
	tp = tp_int;
#endif /* SYNTAX_CORRECT */
	break;
    }
    return qualify_type (tp, qualifier);
}


/*ANSI 3.5.4
 *    direct-declarator:
 *              identifier
 *              ( declarator )
 *              direct-declarator [ type-qualifier-list assignment-expression[opt] ]
 *              direct-declarator [ static type-qualifier-list[opt] assignment-expression ]
 *              direct-declarator [ type-qualifier-list static assignment-expression ]
 *              direct-declarator [ type-qualifier-list[opt] * ]
 *              direct-declarator ( parameter-type-list )
 *              direct-declarator ( identifier-list[opt] )
 *
 *      direct-abstract-declarator:
 *              ( abstract-declarator )
 *              direct-abstract-declarator[opt] [ constant-expression[opt] ]
 *              direct-abstract-declarator[opt] ( parameter-type-list[opt] )
 */
static TYP *direct_declarator_tail P2 (SYM *, sp, TYP *, tp)
{
    SIZE    size, ind;

    switch (lastst) {
    case tk_openbr:
	getsym ();
	if (lastst != tk_closebr) {
	    ind = (SIZE) intexpr ();
#ifndef SYNTAX_CORRECT
	    /*
	     *Constraint: The expression delimited by [ and ] (which
	     *        specifies the size of an array) shall be an
	     *        integral constant expression that has a value
	     *        greater than zero.
	     */
	    if (ind <= 0L) {
		message (ERR_SIZE);
	    }
#endif /* SYNTAX_CORRECT */
	} else {
	    ind = UNKNOWN_SIZE;
	}
	needpunc (tk_closebr);
	tp = direct_declarator_tail (sp, tp);
	size = tp ? tp->size : (SIZE) 1;
	tp = mk_type (tp_array, tp);
	set_array_index (tp, ind);
	set_derived (tp);
	if (size == UNKNOWN_SIZE || ind == UNKNOWN_SIZE) {
	    tp = copy_type (tp);
	    tp->size = UNKNOWN_SIZE;
	} else {
	    tp->size = size * ind;
	}
	return tp;

    case tk_openpa:
	getsym ();
	tp = mk_type (tp_func, tp);
	set_derived (tp);

	switch (lastst) {
	case tk_id:
	    if (!is_typedef_symbol (lastsym)) {
#ifdef FACIST
#ifndef SYNTAX_CORRECT
		message (WARN_OLDDEF);
#endif /* SYNTAX_CORRECT */
#endif /* FACIST */
		beginparamblock ();
		identifier_list ();
		set_parameters (tp, endparamblock ());
#ifndef SYNTAX_CORRECT
		if (obsolete_option) {
		    message (WARN_PROTOTYPE, nameof (sp));
		}
#endif /* SYNTAX_CORRECT */
		break;
	    }
	    /*lint -fallthrough */
	default:
	    if (lang_option >= LANG_C90) {
		beginparamblock ();
		parameter_type_list ();
		set_parameters (tp, endparamblock ());
		set_ansi (tp);
	    }
	    break;
	case tk_closepa:
	    set_parameters (tp, &init_block);
#ifndef SYNTAX_CORRECT
#ifdef FACIST
	    message (WARN_OLDDEF);
#endif /* FACIST */
	    if (obsolete_option) {
		message (WARN_PROTOTYPE, nameof (sp));
	    }
#endif /* SYNTAX_CORRECT */
	    break;
	}
	needpunc (tk_closepa);
#ifdef ASMCALL
	if (asmcall_option != NULL) {
	    funckind (tp) = fc_user;
	    funccall (tp) = asmcall_option;
	    asmcall_option = NULL;
	}
#endif /* ASMCALL */
	return direct_declarator_tail (sp, tp);
    default:
	return tp;
    }
}

static SYM *direct_declarator P2 (STORAGE, sc, TYP *, tp)
{
    SYM    *sp;

    switch (lastst) {
    case tk_id:
	sp = mk_sym (lastsym, sc, tp);
	getsym ();
	break;
    case tk_openpa:
	getsym ();
	if (lastst == tk_closepa) {
	    /*
	     * empty parentheses in a type-name are interpreted as "function
	     * with no parameter specification", rather than redundant
	     * parentheses around the omitted identifier.
	     */
	    getsym ();
	    tp = mk_type (tp_func, tp);
	    sp = mk_sym ((const CHAR *) "", sc, tp);
	} else {
	    TYP    *tp1;

	    sp = declarator (sc, NIL_TYP);
	    needpunc (tk_closepa);
	    tp = direct_declarator_tail (sp, tp);
	    tp1 = typeof (sp);
	    if (tp1 != NIL_TYP) {
		for (; tp1->btp; tp1 = tp1->btp)
		    /* nothing */ ;
		tp1->btp = tp;
	    } else {
		set_type (sp, tp);
	    }
	    size_type (typeof (sp));
	}
	return sp;
    default:
	/* abstract */
	sp = mk_sym ((const CHAR *) "", sc, tp);
	break;
    }
    set_type (sp, direct_declarator_tail (sp, typeof (sp)));
    return sp;
}


/*ANSI 3.5.4
 *    type-qualifier-list:
 *              type-qualifier
 *              type_qualifier-list type-qualifier
 */
static QUALIFIER type_qualifier_list P0 (void)
{
    QUALIFIER qual = QUAL_NONE;

    while (is_type_qualifier (lastst)) {
	qual = type_qualifier (qual);
    }
    return qual;
}


/*ANSI 3.5.4
 *    declarator:
 *              pointer[opt] direct-declarator
 *
 *      abstract-declarator:
 *              pointer
 *              pointer[opt] direct-abstract-declarator
 *
 *      pointer:
 *              * type-qualifier-list[opt]
 *              * type-qualifier-list[opt] pointer
 */
static SYM *declarator P2 (STORAGE, sc, TYP *, tp)
{
    SYM    *sp;

    switch (lastst) {
    case tk_star:
	getsym ();
	tp = mk_type (tp_pointer, tp);
	if (tp->btp == NIL_TYP) {
	    /*
	     *       The type of pointer isn't known .... it will only be
	     *       known later.    We must ensure that the same pointer is
	     *       therefore not used more than once.
	     */
	    tp = copy_type (tp);
	}
	tp = qualify_type (tp, type_qualifier_list ());
	sp = declarator (sc, tp);
	break;
    case tk_id:
    case tk_openpa:
    case tk_openbr:
	sp = direct_declarator (sc, tp);
	break;
    default:
	sp = mk_sym ((const CHAR *) "", sc, tp);
    }
    return sp;
}

static BLOCK *get_parameters P1 (const SYM *, sp)
{
    BLOCK  *block, *block2;
    SYM    *sp1, *sp2;

    block = parameters (typeof (sp));
    if (is_ansi (typeof (sp))) {
#ifndef SYNTAX_CORRECT
	if (lastst == tk_begin) {
	    /*
	     * Parameters to function definitions must have a name
	     */
	    for (sp2 = symbolsof (block); sp2 != NIL_SYM; sp2 = nextsym (sp2)) {
		if (is_void (typeof (sp2)) || is_ellipsis (typeof (sp2))) {
		    break;
		}
		if (nameof (sp2) == NIL_CHAR || nameof (sp2)[0] == (CHAR) 0) {
		    message (ERR_PARMS);
		    break;
		}
		symbol_set (sp2);
	    }
	}
#endif /* SYNTAX_CORRECT */
    } else {
	beginparamblock ();
	VOIDCAST declaration_list (sc_parms, 0L);

	block2 = endparamblock ();
	set_parameters (typeof (sp), &init_block);

#ifndef SYNTAX_CORRECT
	/*
	 * an id list on a function declaration is illegal
	 */
	if ((lastst != tk_begin) && (symbolsof (block) != NIL_SYM)) {
	    message (ERR_IDLIST);
	}
#endif /* SYNTAX_CORRECT */
	/*
	 * now match up the identifier list with the declarations
	 */
	for (sp1 = symbolsof (block); sp1 != NIL_SYM; sp1 = nextsym (sp1)) {
	    sp2 = search (nameof (sp1), &block2->symbols);
	    if (sp2 == NIL_SYM) {
#ifndef SYNTAX_CORRECT
		message (WARN_IMPLICIT, nameof (sp1));
#endif /* SYNTAX_CORRECT */
		set_type (sp1, tp_int);
	    } else {
		set_storage (sp1, storageof (sp2));
		set_type (sp1, typeof (sp2));
		sp1->value.i = sp2->value.i;
		symbol_set (sp1);
	    }
	}
#ifndef SYNTAX_CORRECT
	/*
	 * now check for declared items which aren't in the
	 * identifier list for the function.
	 */
	for (sp2 = symbolsof (block2); sp2 != NIL_SYM; sp2 = nextsym (sp2)) {
	    if (is_const (sp2)) {
		continue;
	    }
	    if (search (nameof (sp2), &block->symbols) == NIL_SYM) {
		message (ERR_ARG, nameof (sp2));
	    }
	    symbol_set (sp2);
	}
	/*
	 * check the function parameter declarations with any function
	 * prototype which may be in scope.
	 */
	check_parameters (block, sp);
#endif /* SYNTAX_CORRECT */
    }
    return block;
}

/*ANSI 3.7
 *    external-declaration:
 *              function-declaration
 *              declaration
 *
 *ANSI 3.5
 *      declaration:
 *              declaration-specifiers init-declaration-list[opt] ;
 *
 *      init-declaration-list:
 *              init-declarator
 *              init-declarator-list , init-declarator
 *
 *      init-declarator:
 *              declarator
 *              declarator = initializer
 *
 *      function-definition:
 *              declaration-specifiers[opt] declarator declaration-list[opt]
 *                                                      compound-statement
 */
static SIZE declaration P3 (STORAGE, sc, STORAGE, def_sc, SIZE, offset)
{
    TYP    *tp;
    SYM    *sp;
    FSPECIFIER fs = fs_none;

    tp = declaration_specifiers (&sc, def_sc, &fs);
#ifndef SYNTAX_CORRECT
    switch (def_sc) {
    case sc_global:
	if (sc == sc_register || sc == sc_auto) {
	    message (ERR_ILLCLASS);
	}
	break;
    case sc_parms:
	if (sc != sc_register && sc != sc_parms) {
	    message (ERR_ILLCLASS);
	}
	break;
    default:
	break;
    }
#endif /* SYNTAX_CORRECT */
    for (sp = NIL_SYM;;) {
	switch (lastst) {
	case tk_id:
	case tk_star:
	case tk_openpa:
	    if (sc != sc_global && tp == NIL_TYP) {
		/*ANSI 3.5
		 * If an identifier for an object is declared with no linkage,
		 * the type for the object shall be complete by the end of its
		 * declarator, or by the end of its identifiers if it has an
		 * initializer.
		 *ANSI 3.1.2.2
		 * The following identifiers have no linkage: an identifier
		 * declared to be anything other than an object or a function;
		 * an identifier declared to be a function parameter; a block
		 * scope identifier for an object declared without the
		 * storage-class specifier extern.
		 */
		return offset;
	    }
	    sp = declarator (sc, tp);
	    switch (typeof (sp)->type) {
		BLOCK  *block;

	    case bt_func:
		set_fspecifier (sp, fs);
		check_function_definition (sp);
		if (lastst != tk_semicolon) {
		    block = get_parameters (sp);
		} else {
		    block = NIL_BLOCK;
		}
		if (def_sc == sc_parms) {
		    /*
		     * Functions which are passed as a parameter are really
		     * pointers to a function.
		     */
		    set_type (sp, mk_type (tp_pointer, typeof (sp)));
		}
		if (lastst == tk_begin) {
		    /* function declaration */
		    sp = declare (sp, def_sc);
#ifndef SYNTAX_CORRECT
		    check_function_declaration (sp);
		    if (def_sc != sc_global) {
			message (ERR_NESTED, nameof (sp));
			break;
		    }
		    if (is_symbol_defined (sp)) {
			message (ERR_REDECL, nameof (sp));
		    }
		    symbol_defined (sp);
#endif /* SYNTAX_CORRECT */
#ifdef EXTERNAL
		    funclist (sp);
#endif /* EXTERNAL */
		    func_symbol = sp;
		    funcbody (sp, block);
		    func_symbol = NULL;
		    return offset;
		}
		/*lint -fallthrough */
	    default:
#ifndef SYNTAX_CORRECT
		if (fs == fs_inline) {
		    message (ERR_INLINE);
		}
#endif /* SYNTAX_CORRECT */
		sp = declare (sp, def_sc);
		offset = doinit (sp, offset);
		if (def_sc == sc_auto) {
		    switch (storageof (sp)) {
		    default:
#ifndef SYNTAX_CORRECT
			check_complete (typeof (sp));
#endif /* SYNTAX_CORRECT */
#ifdef CPU_DEFINED
			addoptinfo (sp, def_sc);
#endif /* CPU_DEFINED */
			/*lint -fallthrough */
		    case sc_external:
		    case sc_typedef:
			break;
		    }
		}
	    }
	    break;
	case tk_eof:
	    return offset;
	case tk_comma:
	    needpunc ((TOKEN) (lastst + 1));	/* force error */
	    break;
	default:
	    break;
	}
	if (lastst != tk_comma) {
	    break;
	}
	getsym ();
    }

#ifndef SYNTAX_CORRECT
    if (sp == NIL_SYM) {
	switch (sc) {
	case sc_typedef:
	    if (is_anonymous_structure (tp)) {
		/* no declarator defined */
		message (ERR_IDEXPECT);
	    } else {
		/* struct declator defined so is legal */
		message (WARN_TYPEDEF);
	    }
	    /*lint -fallthrough*/
	case sc_external:
	case sc_global:
	case sc_static:
	    if (is_structure_type (tp)) {
	    } else if (is_enum (tp)) {
	    } else {
		message (ERR_IDEXPECT);
	    }
	    break;
	default:
	    break;
	}
    }
#endif /* SYNTAX_CORRECT */

    if (lastst == tk_semicolon) {
	needpunc (tk_semicolon);
    } else {
	needpunc (tk_semicolon);
	getsym ();
    }
    return offset;
}


/*
 * main compiler routine. this routine parses all of the declarations using
 * declare which will call funcbody as functions are encountered.
 *
 *ANSI 3.7
 *      translation-unit:
 *              external-declaration
 *              translation-unit external-declaration
 */
void translation_unit P0 (void)
{
    SIZE    offset = 0L;

#ifdef VERBOSE
    clock_t ltime = clock ();

#endif

    global_flag = 1;
    strtab = NIL_STRING;
    beginblock ();
    nextlabel = (LABEL) 1;
#ifdef CPU_DEFINED
    if (code_option) {
	put_start ();
    }
#endif /* CPU_DEFINED */
    while (lastst != tk_eof) {
#ifndef SYNTAX_CORRECT
	if (lastst == tk_semicolon) {
	    message (WARN_EMPTY);
	}
#endif /* SYNTAX_CORRECT */
	offset += declaration (sc_global, sc_global, offset);
    }
    endblock ();
#ifdef CPU_DEFINED
    if (code_option) {
	put_literals ();
	put_finish ();
    }
#endif /* CPU_DEFINED */

#ifdef VERBOSE
    decl_time += clock () - ltime;
#endif
}
