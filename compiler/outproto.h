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
#ifdef CPU_DEFINED

#ifndef _OUT_H
#define _OUT_H

#define	MAX_WIDTH	60	/* line length of assembler line */

#ifndef _CODE_DEFINED
#define _CODE_DEFINED
typedef struct ocode CODE;

#endif

#ifdef MULTIPLE_ASSEMBLERS
/*
 * Support multiple assembler output types
 */

struct funcs
{
    void    (*Put_code) P_ ((const CODE *));
    void    (*Put_name) P_ ((SYM *));
    void    (*Put_label) P_ ((LABEL));
    void    (*Put_byte) P_ ((UVAL));
    void    (*Put_word) P_ ((UVAL));
    void    (*Put_dword) P_ ((UVAL));
#ifndef RELOC_BUG
    void    (*Put_char) P_ ((const EXPR *));
    void    (*Put_short) P_ ((const EXPR *));
#endif				/* RELOC_BUG */
    void    (*Put_long) P_ ((const EXPR *));
    void    (*Put_longlong) P_ ((const EXPR *));
    void    (*Put_pointer) P_ ((const EXPR *));
    void    (*Put_storage) P_ ((SYM *));
    void    (*Put_literals) P_ ((void));
    void    (*Put_finish) P_ ((void));
    void    (*Put_start) P_ ((void));
    void    (*Put_reference) P_ ((SYM *));
    void    (*Put_epilogue) P_ ((SYM *, LABEL));
    void    (*Put_cseg) P_ ((SIZE));
    void    (*Put_dseg) P_ ((SIZE));
    void    (*Put_kseg) P_ ((SIZE));
    void    (*Put_rseg) P_ ((SIZE));
           
#ifndef FLOAT_BOOTSTRAP
#ifdef FLOAT_SUPPORT
    void    (*Put_float) P_ ((const RVAL *));
    void    (*Put_double) P_ ((const RVAL *));
    void    (*Put_longdouble) P_ ((const RVAL *));
           
#endif	/* FLOAT_SUPPORT */
#endif				/* FLOAT_BOOTSTRAP */
    void    (*Last) P_ ((void));
       };

#define	put_code(ip)		(Funcs->Put_code)(ip)
#define	put_name(s)		(Funcs->Put_name)(s)
#define	put_label(lab)		(Funcs->Put_label)(lab)
#define	put_byte(val)		(Funcs->Put_byte)(val)
#define	put_word(val)		(Funcs->Put_word)(val)
#define	put_dword(val)		(Funcs->Put_dword)(val)
#define	put_pointer(node)	(Funcs->Put_pointer)(node)
#define	put_long(node)		(Funcs->Put_long)(node)
#define	put_longlong(node)	(Funcs->Put_longlong)(node)
#define	put_char(node)		(Funcs->Put_char)(node)
#define	put_short(node)		(Funcs->Put_short)(node)
#define	put_storage(sp)		(Funcs->Put_storage)(sp)
#define	put_literals()		(Funcs->Put_literals)()
#define	put_finish()		(Funcs->Put_finish)()
#define	put_start()		(Funcs->Put_start)()
#define	put_reference(s)	(Funcs->Put_reference)(s)
#define	put_epilogue(s,l)	(Funcs->Put_epilogue)(s,l)
#define	put_cseg(align)		(Funcs->Put_cseg)(align)
#define	put_dseg(align)		(Funcs->Put_dseg)(align)
#define	put_kseg(align)		(Funcs->Put_kseg)(align)
#define	put_rseg(align)		(Funcs->Put_rseg)(align)
#ifdef FLOAT_SUPPORT
#define	put_float(val)		(Funcs->Put_float)(val)
#define put_double(val)		(Funcs->Put_double)(val)
#define put_longdouble(val)	(Funcs->Put_longdouble)(val)
#endif /* FLOAT_SUPPORT */

#ifdef OUT_MODULE
#define PRIVATE	static
#undef	put_byte
#undef	put_char
#undef	put_code
#undef	put_cseg
#undef	put_double
#undef	put_dseg
#undef	put_dword
#undef	put_epilogue
#undef	put_finish
#undef	put_float
#undef	put_kseg
#undef	put_label
#undef	put_literals
#undef	put_long
#undef	put_longdouble
#undef	put_name
#undef	put_pointer
#undef	put_longlong
#undef	put_reference
#undef	put_rseg
#undef	put_short
#undef	put_start
#undef	put_storage
#undef	put_word
#endif /* OUT_MODULE */

#else /* MULTIPLE_ASSEMBLER */

extern void put_code P_ ((const CODE *));
extern void put_name P_ ((SYM *));
extern void put_label P_ ((LABEL));
extern void put_byte P_ ((UVAL));
extern void put_word P_ ((UVAL));
extern void put_dword P_ ((UVAL));

#ifdef FLOAT_SUPPORT
extern void put_float P_ ((const RVAL *));
extern void put_double P_ ((const RVAL *));
extern void put_longdouble P_ ((const RVAL *));

#endif /* FLOAT_SUPPORT */
extern void put_pointer P_ ((const EXPR *));
extern void put_storage P_ ((SYM *));
extern void put_literals P_ ((void));
extern void put_finish P_ ((void));
extern void put_start P_ ((void));
extern void put_reference P_ ((SYM *));
extern void put_epilogue P_ ((SYM *, LABEL));
extern void put_cseg P_ ((SIZE));
extern void put_dseg P_ ((SIZE));
extern void put_kseg P_ ((SIZE));
extern void put_rseg P_ ((SIZE));

#ifndef RELOC_BUG
extern void put_char P_ ((const EXPR *));
extern void put_short P_ ((const EXPR *));

#endif /* RELOC_BUG */
extern void put_long P_ ((const EXPR *));
extern void put_longlong P_ ((const EXPR *));

#ifdef OUT_MODULE
#define	PRIVATE
#endif /* OUT_MODULE */

#endif /* MULTIPLE_ASSEMBLERS */

#ifndef CPU_DEFINED
#undef put_byte
#undef put_char
#undef put_short
#undef put_pointer
#undef put_float
#undef put_double
#undef put_longdouble

#define put_byte(t)
#define put_char(t)
#define put_short(t)
#define put_pointer(t)
#define put_float(t)
#define put_double(t)
#define put_longdouble(t)
#endif /* CPU_DEFINED */

#endif /* _OUT_MODULE */

/****************************************************************************/

#ifdef DEBUGINFO
#ifdef MULTIPLE_DEBUG
struct dbgfuncs
{
    void    (*Put_void) P_ ((void));
};

#define	put_void()		(DbgFuncs->Put_void)(ip)

#else /* MULTIPLE_DEBUG */

extern void put_void P_ ((void));

#endif /* MULTIPLE_DEBUG */

#endif /* DEBUGINFO */

#endif /* CPU_DEFINED */
