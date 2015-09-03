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

#ifndef _EXPR_H
#define _EXPR_H

/*
 * expression tree descriptions
 */

enum e_node
{
    en_comma,			/* v.p[0] , v.p[1] */
    en_icon,			/* v.i     integer constant */
    en_fcon,			/* v.f     floating point constant */
    en_labcon,			/* v.l     internal label */
    en_nacon,			/* v.p[0]  global/external variable */
    en_autocon,			/* v.p[0]  auto variable */
    en_global,			/* v.p[0]  global variable (via base register) */
    en_fcall,			/* v.p[0] (v.p[1]) function call */
    en_call,			/* v.p[0] (v.p[1]) function call ... routine removes own parameters */
    en_usercall,		/* v.p[0] (v.p[1]) user specified call */
    en_register,		/* v.r     register variable */
    en_add,			/* v.p[0]  + v.p[1] */
    en_sub,			/* v.p[0]  - v.p[1] */
    en_mul,			/* v.p[0]  * v.p[1] */
    en_mod,			/* v.p[0]  % v.p[1] */
    en_div,			/* v.p[0]  / v.p[1] */
    en_lsh,			/* v.p[0] << v.p[1] */
    en_rsh,			/* v.p[0] >> v.p[1] */
    en_cond,			/* v.p[0]  ? v.p[1]->v.p[0] : v.p[1]->v.[1] */
    en_assign,			/* v.p[0]  = v.p[1] */
    en_asadd,			/* v.p[0] += v.p[1] */
    en_assub,			/* v.p[0] -= v.p[1] */
    en_asmul,			/* v.p[0] *= v.p[1] */
    en_asmul2,			/* v.p[0] *= v.p[1] special for integral *= floating */
    en_asdiv,			/* v.p[0] /= v.p[1] */
    en_asdiv2,			/* v.p[0] /= v.p[1] special for integral *= floating */
    en_asmod,			/* v.p[0] %= v.p[1] */
    en_asrsh,			/* v.p[0] >>= v.p[1] */
    en_asxor,			/* v.p[0] ^= v.p[1] */
    en_aslsh,			/* v.p[0] <<= v.p[1] */
    en_asand,			/* v.p[0] &= v.p[1] */
    en_asor,			/* v.p[0] |= v.p[1] */
    en_uminus,			/* - v.p[0] */
    en_compl,			/* ~ v.p[0] */
    en_test,			/* v.p[0] != 0 */
    en_not,			/* ! v.p[0] */
    en_eq,			/* v.p[0] == v.p[1] */
    en_ne,			/* v.p[0] != v.p[1] */
    en_lt,			/* v.p[0] < v.p[1] */
    en_le,			/* v.p[0] <= v.p[1] */
    en_gt,			/* v.p[0] > v.p[1] */
    en_ge,			/* v.p[0] >= v.p[1] */
    en_and,			/* v.p[0] & v.p[1] */
    en_or,			/* v.p[0] | v.p[1] */
    en_land,			/* v.p[0] && v.p[1] */
    en_lor,			/* v.p[0] || v.p[1] */
    en_xor,			/* v.p[0] ^ v.p[1] */
    en_ainc,			/* v.p[0] ++ */
    en_adec,			/* v.p[0] -- */
    en_ref,			/* v.p[0]   reference to a variable */
    en_cast,			/* v.p[0]   cast operation */
    en_deref,			/* & v.p[0] */
    en_fieldref,		/* v.p[0]   reference to a bitfield */
    en_list,			/* v.p[0] , v.p[1] parameter list */
    en_literal,			/* v.p[0]   compound literal */
    en_litval,			/* v.p[0] , v.p[1] literal value and offset */
    en_sym,			/* v.sp     symbol */
    en_str,			/* v.str */
    en_size			/* evaluated size of a sizeof operation */
};

/*
 * statement node descriptions 
 */

enum e_stmt
{
    st_expr,
    st_while,
    st_for,
    st_do,
    st_if,
    st_switch,
    st_case,
    st_goto,
    st_break,
    st_continue,
    st_label,
    st_return,
    st_compound,
#ifdef ASM
    st_asm,
#endif				/* ASM */
    st_default
};

typedef struct enode EXPR;
typedef struct cse CSE;
typedef enum e_node EXPRTYPE;
typedef enum e_stmt STMTTYPE;
typedef unsigned LINE;
typedef signed char REG;
typedef unsigned short USES;

#define NIL_STMT ((STMT *) 0)
#define NIL_EXPR ((EXPR *) 0)
#define NIL_CSE ((CSE *) 0)

/*
 * expression node
 */

struct enode
{
    EXPRTYPE nodetype;		/* expression operation */
    TYP    *etp;		/* type of the result of the operation */
    union
    {
	LABEL   l;		/* Internal label (static) */
	REG     r;		/* register (en_register) */
	IVAL    i;		/* signed value (constant) */
	UVAL    u;		/* unsigned value (constant) */
#ifdef FLOAT_SUPPORT
	RVAL    f;		/* floating point value */
#endif
	const CHAR *str;	/* string value */
	SYM    *sp;		/* symbol table entry */
	EXPR   *p[2];
	struct
	{
	    EXPR *p;			/* same as p[0] */
	    unsigned char width;	/* bitfield width */
	    unsigned char offset;	/* bitfield offset */
	} bit;
    } v;
};

#define	is_fcon(ep)		((ep)->nodetype == en_fcon)
#define	is_icon(ep)		((ep)->nodetype == en_icon)
#define	is_sym(ep)		((ep)->nodetype == en_sym)

/*
 * statement node
 */

struct snode
{
    STMTTYPE stype;
    STMT   *next;		/* next statement */
    EXPR   *exp;		/* condition or expression */
    STMT   *s1;			/* internal statement */
    union
    {
	EXPR   *e;		/* condition or expression */
	STMT   *s;		/* internal statement (else) */
	IVAL    i;		/* case value */
	LABEL   l;		/* label */
    } v1   , v2;
#ifdef DEBUGOPT
    LINE    line;		/* source line number */
    const CHAR *linetxt;	/* source text of line */
#endif				/*DEBUGOPT */
};


/*
 * common sub expression - used in the global optimiser analyze.c
 */
struct cse
{
    CSE    *next;
    EXPR   *exp;		/* optimizable expression */
    USES    uses;		/* number of uses */
    USES    duses;		/* number of dereferenced uses */
    BOOL    voidf;		/* cannot optimize flag */
    REG     reg;		/* allocated register */
};

#define NO_REG	((REG) -1)	/* stored in "REG reg;" in struct cse */

#ifdef TRACE
#define	SUP_TRACE	(const CHAR *)".stmttrace"
#endif /* TRACE */

#ifdef MULTIPLE_PROCESSORS
/*
 * More than one processor type has been defined ... all function calls
 * specific to a given processor type must be vectored.
 */
struct genfuncs
{
    void    (*G_expression) P_ ((const EXPR *));
    void    (*G_jtrue) P_ ((const EXPR *, LABEL));
    void    (*G_jfalse) P_ ((const EXPR *, LABEL));
    void    (*G_stack) P_ ((SIZE));
    void    (*G_switch_table)
            P_ ((const EXPR *, SWITCH *, UVAL, UVAL));
    void    (*G_switch_compare) P_ ((const EXPR *, STMT *));
    void    (*G_entry) P_ ((SIZE));
    void    (*G_return) P_ ((const EXPR *, TYP *));
    void    (*G_epilogue) P_ ((void));
    void    (*G_label) P_ ((LABEL));
    void    (*G_branch) P_ ((LABEL));
#ifdef DEBUGOPT
    void    (*G_line) P_ ((LINE, const CHAR *));
#endif				/*DEBUGOPT */
    void    (*G_allocate) P_ ((CSE *));
    void    (*G_preload) P_ ((CSE *));
    void    (*G_flush) P_ ((SYM *));
    void    (*G_auto_align) P_ ((void));
    BOOL    (*G_is_bigendian) P_ ((void));
    BOOL    (*G_is_ascending_stack) P_ ((void));
    EXPR   *(*G_order) P_ ((EXPR *));
    EXPR   *(*G_transform) P_ ((EXPR *));
    void    (*G_initialize) P_ ((void));
    void    (*G_terminate) P_ ((void));
    REGUSAGE *(*G_regusage) P_ ((TYP *));
    SIZE   *G_alignments;
};

#define	g_expression(node)		(GFuncs->G_expression)(node)
#define	g_jtrue(node,label)		(GFuncs->G_jtrue)(node,label)
#define	g_jfalse(node,label)		(GFuncs->G_jfalse)(node,label)
#define g_stack(bytes)			(GFuncs->G_stack)(bytes)
#define	g_switch_table(ep,sw,min,max)	(GFuncs->G_switch_table)(ep,sw,min,max)
#define	g_switch_compare(ep,stmt)	(GFuncs->G_switch_compare)(ep,stmt)
#define	g_entry(size)			(GFuncs->G_entry)(size)
#define	g_return(ep,tp)			(GFuncs->G_return)(ep,tp)
#define	g_epilogue()			(GFuncs->G_epilogue)()
#define	g_label(l)			(GFuncs->G_label)(l)
#define	g_branch(l)			(GFuncs->G_branch)(l)
#ifdef DEBUGOPT
#define	g_line(l, s)			(GFuncs->G_line)(l,s)
#endif /*DEBUGOPT */
#define	g_allocate(csp)			(GFuncs->G_allocate)(csp)
#define	g_preload(csp)			(GFuncs->G_preload)(csp)
#define	g_flush(na)			(GFuncs->G_flush)(na)
#define	g_auto_align()			(GFuncs->G_auto_align)()
#define	g_is_bigendian()		(GFuncs->G_is_bigendian)()
#define	g_is_ascending_stack()		(GFuncs->G_is_ascending_stack)()
#define	g_transform(e)			(GFuncs->G_transform)(e)
#define	g_order(e)			(GFuncs->G_order)(e)
#define	g_initialize()			(GFuncs->G_initialize)()
#define	g_terminate()			(GFuncs->G_terminate)()
#define	g_regusage(tp)			(GFuncs->G_regusage)(tp)
#define	g_alignments			(GFuncs->G_alignments)

#ifdef GEN_MODULE
#define PRIVATE	static
#undef	g_allocate
#undef	g_auto_align
#undef	g_branch
#undef	g_entry
#undef	g_epilogue
#undef	g_expression
#undef	g_flush
#undef	g_initialize
#undef	g_is_ascending_stack
#undef	g_is_bigendian
#undef	g_jfalse
#undef	g_jtrue
#undef	g_label
#undef	g_line
#undef	g_order
#undef	g_preload
#undef	g_return
#undef	g_stack
#undef	g_switch_compare
#undef	g_switch_table
#undef	g_terminate
#undef	g_regusage
#undef	g_transform
#endif /* GEN_MODULE */
#else /* MULTIPLE_PROCESSORS */
#ifdef GEN_MODULE
#define	PRIVATE
#endif /* GEN_MODULE */
#endif /* MULTIPLE_PROCESSORS */


#endif /* _EXPR_H */
