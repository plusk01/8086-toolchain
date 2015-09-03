#ifndef _PROTO_H
#define	_PROTO_H

#ifdef CPU_DEFINED
/* analyze.c */
extern BOOL is_equalnode P_ ((const EXPR *, const EXPR *));
extern CSE *globalopt P_ ((STMT *));
extern EXPR *unsymbolexpr P_ ((EXPR *));
extern USES desire P_ ((const CSE *));
extern void addoptinfo P_ ((SYM *, STORAGE));
extern void deloptinfo P_ ((const EXPR *));
extern void rel_autolst P_ ((void));
extern void rel_reglst P_ ((void));

#endif /* CPU_DEFINED */

/* cmain.c */
extern int main P_ ((int, char **));
extern void options P_ ((const CHAR *, BOOL));
extern void chip_option P_ ((BOOL, const OPTION *, const CHAR *));
extern void enumeration_option P_ ((BOOL, const OPTION *, const CHAR *));
extern void list_option P_ ((BOOL, OPTION *, const CHAR *));
extern void numeric_option P_ ((BOOL, const OPTION *, const CHAR *));
extern void set_option P_ ((BOOL, OPTION *, const CHAR *));
extern void string_option P_ ((BOOL, const OPTION *, const CHAR *));

/* decl.c */
extern BOOL is_type_name P_ ((TOKEN));
extern BOOL is_declaration_specifier P_ ((TOKEN st));
extern SIZE declaration_list P_ ((STORAGE, SIZE));
extern SYM *identifier P_ ((void));
extern BOOL is_representable P_ ((IVAL, const TYP *));
extern TYP *type_name P_ ((void));
extern void translation_unit P_ ((void));

/* expr.c */
extern BOOL is_constant_in_range P_ ((const EXPR *, const TYP *));
extern BOOL is_lvalue P_ ((const EXPR *));
extern EXPR *expression P_ ((void));
extern EXPR *condition_expression P_ ((void));
extern EXPR *copynode P_ ((const EXPR *));
extern EXPR *exprnc P_ ((void));
extern EXPR *implicit_castop P_ ((EXPR *, TYP *));
extern EXPR *integral_expression P_ ((void));
extern EXPR *mk_autocon P_ ((SIZE));
extern EXPR *mk_icon P_ ((IVAL, TYP *));
extern EXPR *mk_lcon P_ ((LABEL));
extern EXPR *mk_litval P_ ((EXPR *, SIZE));
extern EXPR *mk_node P_ ((EXPRTYPE, EXPR *, EXPR *, TYP *));
extern EXPR *mk_ref P_ ((EXPR *, TYP *));
extern EXPR *mk_scon P_ ((const CHAR *, TYP *));
extern EXPR *mk_symnode P_ ((SYM *));
extern EXPR *transform_assign P_ ((EXPR *, const CHAR *, const CHAR *, const CHAR *));
extern EXPR *transform_binary P_ ((EXPR *, const CHAR *));
extern EXPR *walkexpr P_ ((EXPR *, EXPR *(*)(EXPR *)));

#ifndef SYNTAX_CORRECT
extern void check_discard P_ ((const EXPR *));
#endif /* SYNTAX_CORRECT */

#ifdef FLOAT_SUPPORT
extern EXPR *transform_assign2 P_ ((EXPR *, const CHAR *));
extern EXPR *transform_binary_ref P_ ((EXPR *, const CHAR *));
extern EXPR *transform_unary P_ ((EXPR *, const CHAR *));
extern EXPR *transform_unary_ref P_ ((EXPR *, const CHAR *));

#endif /* FLOAT_SUPPORT */

#ifdef ASM
extern EXPR *asm_expression P_ ((void));

#endif /* ASM */
#ifdef TMS320C30
#ifdef FLOAT_SUPPORT
extern EXPR *mk_fcon P_ ((const RVAL *, TYP *));

#endif /* FLOAT_SUPPORT */

#endif /* TMS320C30 */
#ifdef SEQUENCE
extern void sequence_point P_ ((void));

#else
#define	sequence_point()
#endif
#ifdef TRACE
extern EXPR *traceexpr P_ ((void));

#endif /* TRACE */

#ifdef EXTERNAL
/* extern */
extern void funclist P_ ((const SYM *));

#endif /* EXTERNAL */

#ifdef CPU_DEFINED
/* gen*.c */
#ifndef MULTIPLE_PROCESSORS
extern BOOL g_is_ascending_stack P_ ((void));
extern BOOL g_is_bigendian P_ ((void));
extern void g_allocate P_ ((CSE *));
extern void g_auto_align P_ ((void));
extern void g_branch P_ ((LABEL));
extern void g_entry P_ ((SIZE));
extern void g_epilogue P_ ((void));
extern void g_expression P_ ((const EXPR *));
extern void g_flush P_ ((SYM *));
extern void g_jfalse P_ ((const EXPR *, LABEL));
extern void g_jtrue P_ ((const EXPR *, LABEL));
extern void g_label P_ ((LABEL));

#ifdef DEBUGOPT
extern void g_line P_ ((LINE, const CHAR *));

#endif /*DEBUGOPT */
extern void g_preload P_ ((CSE *));
extern void g_return P_ ((const EXPR *, TYP *));
extern void g_stack P_ ((SIZE));
extern void g_switch_compare P_ ((const EXPR *, STMT *));
extern void g_switch_table P_ ((const EXPR *, const SWITCH *, UVAL, UVAL));
extern EXPR *g_order P_ ((EXPR *));
extern EXPR *g_transform P_ ((EXPR *));
extern void g_initialize P_ ((void));
extern void g_terminate P_ ((void));
extern REGUSAGE *g_regusage P_ ((TYP *));

#endif /* MULTIPLE_PROCESSORS */

/* genffp.c */
#ifdef MC680X0
#ifdef FLOAT_MFFP
extern unsigned long genffp P_ ((const RVAL *));

#endif /* FLOAT_MFFP */
#endif /* MC680X0 */

#ifdef ICODE
/* genicode.c */
extern void genicse P_ ((CSE *));
extern void genicode P_ ((const STMT *, int));

#endif /* ICODE */

/* genieee.c */
#ifdef FLOAT_IEEE
extern void ieee_single P_ ((const RVAL *, unsigned long *));
extern void ieee_double P_ ((const RVAL *, unsigned long *, BOOL));
extern void ieee_longdouble P_ ((const RVAL *, unsigned long *, BOOL));

#endif /* FLOAT_IEEE */

/* geninit.c */
#ifdef CPU_DEFINED
extern void g_init P_ ((const EXPR *));
#endif /* CPU_DEFINED */

/* genstmt.c */
extern void genfunc P_ ((SYM *, const STMT *, CSE *, LINE, const CHAR *));

/* genutil.c */
#ifdef FLOAT_SUPPORT
#ifndef FLOAT_BOOTSTRAP
extern LABEL mk_flabel P_ ((const RVAL *, const TYP *));

#endif /* FLOAT_BOOTSTRAP */
#endif /* FLOAT_SUPPORT */
extern EXPR *mk_add P_ ((EXPR *, EXPR *));
extern EXPR *mk_const P_ ((IVAL));
extern EXPR *mk_global P_ ((const CHAR *, const EXPR *));
extern void sync_stack P_ ((void));

#endif /* CPU_DEFINED */
extern UVAL bitmask P_ ((BITSIZE));
extern BOOL tst_const P_ ((const EXPR *));
extern void swap_nodes P_ ((EXPR *));

#ifdef DEBUGOPT
extern const CHAR *mk_string P_ ((const CHAR *));

#endif /* DEBUGOPT */


/* getsym.c */
extern BOOL is_label P_ ((TOKEN));
extern void endsym P_ ((void));
extern void getsym P_ ((void));
extern void initsym P_ ((void));
extern void needpunc P_ ((TOKEN));

/* init.c */
extern SIZE align P_ ((const TYP *, SIZE));
extern SIZE doinit P_ ((SYM *, SIZE));
extern void initfuncid P_ ((const SYM *, const SYM *));
extern EXPR *initializer_list P_ ((TYP *, SIZE, STORAGE));

#ifdef CPU_DEFINED
extern SIZE allocate_storage P_ ((SYM *, SIZE, STORAGE, BOOL));

#endif /* CPU_DEFINED */

/* inline.c */
#ifdef DOINLINE
extern int inline_statement_cost P_ ((STMT *));
#endif /* DOINLINE */

/* intexpr.c */
#ifdef FLOAT_SUPPORT
extern void floatexpr P_ ((TYP *, RVAL *));

#endif /* FLOAT_SUPPORT */
extern IVAL arithexpr P_ ((TYP *));
extern IVAL intexpr P_ ((void));

#ifdef LIST
/* list.c */
extern void summary P_ ((const BLOCK *, LEVEL));

#endif

/* memmgt.c */
extern VOIDSTAR xalloc P_ ((size_t));
extern VOIDSTAR galloc P_ ((size_t));
extern void rel_local P_ ((void));
extern void rel_global P_ ((void));

/* msgout.c */
extern void eprintf P_ ((const char *, ...));

#ifdef ICODE
extern void iprintf P_ ((const char *, ...));

#endif
#ifdef LIST
extern void lprintf P_ ((const char *, ...));

#endif /* LIST */
extern void message P_ ((int, ...));
/*lint -function(exit,fatal)*/
extern void fatal P_ ((const char *, const char *, const char *, ...));


#ifdef LONGLONG_SUPPORT
extern char *longlongStr P_ ((UVAL));


#endif	/* LONGLONG_SUPPORT */
#ifdef CPU_DEFINED
#ifdef DEBUG
extern void dprintf P_ ((int, const char *, ...));

#endif /* DEBUG */
extern void oprintf P_ ((const char *, ...));

#endif /* CPU_DEFINED */

/* optimize.c */
extern EXPR *opt0 P_ ((EXPR *));
extern EXPR *constantopt P_ ((EXPR *));
extern int pwrof2 P_ ((IVAL));

#ifdef CPU_DEFINED
/* outgen.c */
extern const CHAR *outlate P_ ((const CHAR *));

#endif /* CPU_DEFINED */

/* stmt.c */
extern void funcbody P_ ((SYM *, const BLOCK *));
STMT *walkstmt P_ ((STMT *, STMT *(*) (STMT *), EXPR *(*) (EXPR *)));

/* symbol.c */
extern BLOCK *endparamblock P_ ((void));
extern BLOCK *endstructblock P_ ((void));
extern BLOCK *mk_block P_ ((void));
extern LABEL lab_define P_ ((const CHAR *));
extern LABEL lab_search P_ ((const CHAR *));
extern SYM *internal_symbol P_ ((const CHAR *, TYP *));
extern SYM *mk_sym P_ ((const CHAR *, STORAGE, TYP *));
extern SYM *search P_ ((const CHAR *, const TABLE *));
extern SYM *sym_search P_ ((const CHAR *));
extern SYM *tag_search P_ ((const CHAR *));
extern BOOL is_local_scope P_ ((const SYM *));
extern void beginblock P_ ((void));
extern void beginfuncblock P_ ((const BLOCK *));
extern void beginparamblock P_ ((void));
extern void beginstructblock P_ ((BLOCK *));
extern void check_labels P_ ((void));
extern void endblock P_ ((void));
extern void endfuncblock P_ ((void));
extern void field_append P_ ((SYM **));
extern void sym_append P_ ((SYM **));
extern void tag_append P_ ((SYM **));

/* system.c */
extern const char *message_text P_ ((MSGNUM));
extern void openerror P_ ((void));
extern void openfiles P_ ((int, const char * const*));

#ifdef EPOC
extern VOID CommandLineParameters P_ ((int *, char ***));
extern size_t fread P_ ((char *, size_t, size_t, FHANDLE));
extern int vfprintf P_ ((FHANDLE, char *, va_list));
extern int atoi P_ ((const char *));

#endif /* EPOC */

/* types.c */
extern BOOL is_arithmetic_type P_ ((const TYP *));
extern BOOL is_array_type P_ ((const TYP *));
extern BOOL is_compatible_type P_ ((const TYP *, const TYP *));
extern BOOL is_constant_in_type_range P_ ((IVAL, const TYP *, const TYP *));

#ifdef FLOAT_SUPPORT
extern BOOL is_real_floating_type P_ ((const TYP *));
extern BOOL is_floating_type P_ ((const TYP *));
extern BOOL is_complex_type P_ ((const TYP *));


#endif /* FLOAT_SUPPORT */
extern BOOL is_function_type P_ ((const TYP *));
extern BOOL is_incomplete_type P_ ((TYP *));
extern BOOL is_integral_type P_ ((const TYP *));
extern BOOL is_object_type P_ ((const TYP *));
extern BOOL is_pointer_type P_ ((const TYP *));
extern BOOL is_equal_type P_ ((const TYP *, const TYP *));
extern BOOL is_scalar_type P_ ((const TYP *));
extern BOOL is_signed_type P_ ((const TYP *));
extern BOOL is_structure_type P_ ((const TYP *));
extern BOOL is_anonymous_structure P_ ((const TYP *));
extern BOOL is_subtype P_ ((const TYP *, const TYP *));
extern BOOL is_unsigned_type P_ ((const TYP *));
extern BOOL is_variable_array_type P_ ((const TYP *));
extern SIZE alignment_of_type P_ ((const TYP *));
extern TYP *composite_type P_ ((TYP *, TYP *));
extern TYP *copy_type P_ ((const TYP *));
extern TYP *mk_type P_ ((const TYP *, TYP *));
extern TYP *promote_type P_ ((TYP *));
extern TYP *qualify_type P_ ((TYP *, QUALIFIER));
extern TYP *unary_conversion P_ ((TYP *));

#ifdef SYNTAX_CORRECT
#define	check_complete(tp)
#define	check_qualifiers(tp,tp2)
#else
extern void check_complete P_ ((TYP *));
extern void check_qualifiers P_ ((const TYP *, const TYP *));
#endif /* SYNTAX_CORRECT */
extern void initialize_types P_ ((void));
extern void size_type P_ ((TYP *));

#ifdef EPOC
/*
 *   The following defines map ANSI routines onto their PLIB equivalents.
 *   This allows the compiler to be built without linking in the C library.
 */
#define	strlen(s)		p_slen((s))
#define	exit(r)			p_exit(r)
#define	free(s)			p_free(s)
#define	malloc(s)		p_alloc(s)
#define	realloc(p,s)		p_realloc((p),(s))
#define isdigit(c)		p_isdigit(c)
#define	memmove(s,d,l)		p_bcpy((s),(d),(l))
#define	memcmp(s,d,l)		p_bcmp((s),(l),(d),(l))
#endif /* EPOC */

#endif /* _PROTO_H */
