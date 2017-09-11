/*
 * C compiler
 * ==========
 *
 * Copyright 1996 Keith Walker & Dave Walker
 * Credits to Matthew Brandt.
 * All commercial rights reserved.
 *
 * This compiler may be redistributed as long there is no
 * commercial interest. The compiler must not be redistributed
 * without its full sources. This notice must stay intact.
 *
 * This file contains the various definitions for the messages used
 * within the compiler.  By including this file into a source file with
 * suitable definitions for the MSG() macro it can be used to define
 * the various structures needed to access the messages.
 */

/*
 *    Parsing Error messages
 */
#ifndef SYNTAX_CORRECT
    MSG (ERR_ADDREGVAR, err_addregvar, "& operator on register variable '%s'")
    MSG (ERR_ARG, err_arg, "declared argument '%s' missing")
    MSG (ERR_ARITHMETIC, err_arithmetic, "arithmetic type expected")
    MSG (ERR_ARRAYRETURN, err_arrayreturn, "function returning array type")
    MSG (ERR_BINARYEXP, err_binaryexp, "no binary exponent")
    MSG (ERR_BITFIELD, err_bitfield, "& operator may not be applied to bitfields")
    MSG (ERR_BRACE, err_brace, "'{' expected on initialiser")
    MSG (ERR_BREAK, err_break, "break not allowed here")
    MSG (ERR_CASE, err_case, "case not allowed here")
    MSG (ERR_CAST, err_cast, "error doing a cast")
    MSG (ERR_CHARCONST, err_charconst, "character constant unterminated or too long")
    MSG (ERR_CONST, err_const, "modified 'const' value")
    MSG (ERR_CONSTEXPR, err_constexpr, "constant expression expected")
    MSG (ERR_CONSTFLOAT, err_constfloat, "floating point constant expected")
    MSG (ERR_CONSTINT, err_constint, "constant integer expression expected")
    MSG (ERR_CONT, err_cont, "continue not allowed here")
    MSG (ERR_COUNTPARAM, err_countparam, "parameter count incorrect for function %s")
    MSG (ERR_DEREF, err_deref, "error dereferencing a pointer")
    MSG (ERR_DUPCASE, err_dupcase, "duplicate case label 'case %ld'")
    MSG (ERR_DUPDEFAULT, err_dupdefault, "duplicate 'default' label")
    MSG (ERR_DUPLABEL, err_duplabel, "duplicate label '%s'")
    MSG (ERR_ENUMWRAP, err_enumwrap, "enumeration constant too large")
    MSG (ERR_EOF, err_eof, "unexpected end of file")
    MSG (ERR_ESCRANGE, err_escrange, "value of escape sequence out of valid range")
    MSG (ERR_EXPREXPECT, err_exprexpect, "expression expected")
    MSG (ERR_FLDTYPE, err_fldtype, "bit field type should be unsigned or int")
    MSG (ERR_FUNC, err_func, "function declarator not allowed here")
    MSG (ERR_FUNCID, err_funcid, "'__func__' can only be used within a function definition")
    MSG (ERR_IDEXPECT, err_idexpect, "identifier expected")
    MSG (ERR_IDLIST, err_idlist, "identifier list not allowed on function declaration")
    MSG (ERR_ILLCAST, err_illcast, "illegal cast from '%s' to '%s'")
    MSG (ERR_ILLCHAR, err_illchar, "illegal character '%c'")
    MSG (ERR_ILLCLASS, err_illclass, "illegal storage class")
    MSG (ERR_ILLINIT, err_illinit, "illegal initialisation")
    MSG (ERR_ILLINLINE, err_illinline, "illegal function specifier")
    MSG (ERR_ILLSIZEOF, err_illsizeof, "illegal 'sizeof' operation")
    MSG (ERR_ILLTYPE, err_illtype, "illegal type combination")
#ifdef TYPEOF
    MSG (ERR_ILLTYPEOF, err_illtypeof, "illegal 'typeof' operation")
#endif /* TYPEOF */
    MSG (ERR_ILLXCHAR, err_illxchar, "illegal unprintable character (value=0x%x)")
    MSG (ERR_IMPLICITADDR, err_implicitaddr, "implicit conversion to pointer on register variable '%s'")
    MSG (ERR_IMPLICITINT, err_implicitint, "implict 'int'")
    MSG (ERR_INCOMPLETE, err_incomplete, "incomplete '%s' declaration")
    MSG (ERR_INLINE, err_inline, "'inline' only allowed on functions")
    MSG (ERR_INLINEMAIN, err_inlinemain, "'inline' not allowed on 'main()'")
    MSG (ERR_INITSIZE, err_initsize, "too many initialisers")
    MSG (ERR_INTEGER, err_integer, "integral type expected")
    MSG (ERR_LINKAGE, err_linkage, "extern definition of '%s' redeclared static")
    MSG (ERR_LVALUE, err_lvalue, "l-value required")
    MSG (ERR_MINUS, err_minus, "cannot subtract a pointer from an integral value")
    MSG (ERR_MISMATCH, err_mismatch, "type mismatch error")
    MSG (ERR_NESTED, err_nested, "cannot nest function definition '%s()'")
    MSG (ERR_NOARRAY, err_noarray, "array type expected")
#ifndef FLOAT_SUPPORT
    MSG (ERR_NOFLOATSUPPORT, err_nofloatsupport, "floating point support not available")
#endif /* FLOAT_SUPPORT */
    MSG (ERR_NOFUNC, err_nofunc, "function type expected")
    MSG (ERR_NOINIT, err_noinit, "initialisation invalid") 
#ifndef LONGLONG_SUPPORT
    MSG (ERR_NOLONGLONGSUPPORT, err_nolonglongsupport, "long long support not available")
#endif /* LONGLONG_SUPPORT */
    MSG (ERR_NOMEMBER, err_nomember, "'%s' is not a struct/union member")
    MSG (ERR_NOPOINTER, err_nopointer, "pointer type expected")
    MSG (ERR_OBJECT, err_object, "an object type expected")
    MSG (ERR_PARMS, err_parms, "error while scanning a parameter list")
    MSG (ERR_PREPROC, err_preproc, "problem with pre-processor output")
    MSG (ERR_PROTO, err_proto, "function '%s()' prototype mismatch")
    MSG (ERR_PROTODEF, err_protodef, "function '%s()' default promotion / prototype mismatch")
    MSG (ERR_PROTONUM, err_protonum, "function '%s()' mismatched number of arguments")
    MSG (ERR_PUNCT, err_punct, "unexpected symbol '%s' found")
    MSG (ERR_QUAL, err_qual, "qualifier mismatch")
    MSG (ERR_QUALIFIER, err_qualifier, "qualifier already specified")
    MSG (ERR_REDECL, err_redecl, "illegal redeclaration of '%s'")
    MSG (ERR_REPRESENT, err_represent, "constant expression exceeds representable range of type '%s'")
    MSG (ERR_RESTRICT, err_restrict, "'restrict' only allowed on pointer types")
    MSG (ERR_SCALAR, err_scalar, "scalar type expected")
    MSG (ERR_SIZE, err_size, "type/operand has unknown size")
    MSG (ERR_STATIC, err_static, "function '%s' declared but never defined")
    MSG (ERR_STRINGCONST, err_stringconst, "string constant unterminated or too long")
    MSG (ERR_TAG, err_tag, "tag usage '%s' mismatch")
    MSG (ERR_TYPE, err_type, "type specifier '%s' already specified")
    MSG (ERR_UNDEFINED, err_undefined, "undefined identifier '%s'")
    MSG (ERR_UNDEFINEDFN, err_undefinedfn, "implicit defintion of function 'int %s()' not allowed")
    MSG (ERR_UNDEFLAB, err_undeflab, "undefined label '%s'")
    MSG (ERR_VALRETURN, err_valreturn, "no value specified in 'return' statement")
    MSG (ERR_VARRAY, err_varray, "variable length array not allowed")
    MSG (ERR_VOIDFUNC, err_voidfunc, "return value specified to void function")
    MSG (ERR_VOIDPARM, err_voidparm, "void parameter is passed to function %s")
    MSG (ERR_VOIDRETURN, err_voidreturn, "return expression of type void")
    MSG (ERR_WIDTH, err_width, "illegal field width")
/*
 *    Error/Warning messages are ordered in order of increasing severity
 *      If they are to be treated as errors or merely warnings is controlled
 *      by the current setting of the error_option.   For warnings, whether
 *      they are to be output at all is controlled by warn_option.
 */
/*
 *    Level 1
 *      This is the most severe level of warnings.   These should
 *      not be suppressed unless one is certain that is OK.
 */
    MSG (WARN_COUNTPARAM, warn_countparam, "parameter count incorrect for function %s")
    MSG (WARN_EXTERN, warn_extern, "extern definition of '%s' redeclared static")
    MSG (WARN_FLDTYPE, warn_fldtype, "bit field type should be unsigned or int")
    MSG (WARN_TYPEDEF, warn_typedef, "typedef has no name")
/*
 *    Level 2
 *      Default warning level.  These should normally be fixed.
 *      Typically this just requires a cast or something similar.
 */
    MSG (WARN_FORMATEND, warn_formatend, "format string for '%s()' incorrect")
    MSG (WARN_NOHEX, warn_nohex, "\\x not followed by any hex characters")
    MSG (WARN_PARAMSIZE, warn_paramsize, "size of parameter %d changed by prototype on function %s")
    MSG (WARN_REPRESENT, warn_represent, "case label value %ld exceeds representable range of switch control expression type '%s'")
    MSG (WARN_SIZEOF0, warn_sizeof0, "'sizeof' value is zero")
    MSG (WARN_SIZEOFBIG, warn_sizeofbig, "'sizeof' value %ld is greater than '65535'")
    MSG (WARN_TYPECAST, warn_typecast, "conversion between incompatible types '%s' and '%s'")
/*
 *    Level 3
 *      Warnings at this level are often encountered when porting
 *      code. They are quite likely to indicate an error.
 */
    MSG (WARN_DUBIOUS, warn_dubious, "dubious %s declaration; use tag only")
    MSG (WARN_ESCAPECH, warn_escapech, "escape ignored in sequence '\\%c'")
    MSG (WARN_IMPLICITFN, warn_implicitfn, "implicitly declared function: 'int %s()'")
    MSG (WARN_LOCAL, warn_local, "returning address of a local variable")
    MSG (WARN_OUTSCOPE, warn_outscope, "using out of scope declaration for '%s'")
    MSG (WARN_PTRCAST, warn_ptrcast, "conversion between incompatible pointer types")
    MSG (WARN_QUALIFIER, warn_qualifier, "qualifier inconsistent with type 'void'") 
    MSG (WARN_RANGEI, warn_rangei, "constant %ld not within range of type '%s'")
    MSG (WARN_RANGEU, warn_rangeu, "constant 0x%lx not within range of type '%s'")
    MSG (WARN_SHORTPTR, warn_shortptr, "dangerous truncation of pointer to '%s'")
    MSG (WARN_STATIC, warn_static, "function '%s' declared but never defined")
    MSG (WARN_STDARG, warn_stdarg, "parameter before ', ...' causes undefined behaviour")
    MSG (WARN_REDECL, warn_redecl, "redeclaration of '%s'")
    MSG (WARN_VALRETURN, warn_valreturn, "no value specified in 'return' statement")
    MSG (WARN_UNSIGNED, warn_unsigned, "'%s' is always positive")
    MSG (WARN_ZERO, warn_zero, "division by zero")
/*
 *    Level 4
 *      Warnings at this level are often encountered when porting
 *      code, but are quite likely not be an error.
 */
    MSG (WARN_ADDFUNC, warn_addfunc, "& operator on function ignored")
    MSG (WARN_ARRAY, warn_array, "array type used in '%s' statement")
    MSG (WARN_CASTCONST, warn_castconst, "implicit cast of pointer loses const/volatile qualifier")
    MSG (WARN_CONDVOID, warn_condvoid, "%d expression to '?:' operator cast to 'void'")
    MSG (WARN_EMPTY, warn_empty, "empty statement")
    MSG (WARN_EMPTYIF, warn_emptyif, "'if' statement has no effect")
    MSG (WARN_GLOBAL, warn_global, "function '%s' redeclared, assumed static")
    MSG (WARN_HIDE, warn_hide, "definition of '%s' hides an earlier definition")
    MSG (WARN_IMPLICIT, warn_implicit, "argument '%s' implicitly declared 'int'")
    MSG (WARN_NOTREACHED, warn_notreached, "statement not reached")
    MSG (WARN_POINTER, warn_pointer, "pointer difference between different pointer types")
    MSG (WARN_PROTOTYPE, warn_prototype, "K&R style function %s")
    MSG (WARN_PROMOTE, warn_promote, "parameter %d to function %s() promoted to '%s'")
    MSG (WARN_SHIFT, warn_shift, "shift by %ld outside range of '%s'")
    MSG (WARN_STORAGE, warn_storage, "storage specifier not at start of definition")
/*
 *    Level 5
 *      These are warnings for things that are common C practise, but
 *      that might possibly indicate an error if you are having trouble
 *      tracking down a particular problem
 */
    MSG (WARN_ACCESS, warn_access, "'%s' modified and accessed between sequence points")
    MSG (WARN_ASSIGN, warn_assign, "assignment in conditional context")
    MSG (WARN_CONST, warn_const, "constant expression used in '%s' statement")
    MSG (WARN_CONSTINIT, warn_constinit, "'%s' has 'const' qualifier but is not initialised")
    MSG (WARN_DISCARD, warn_discard, "result of expression has been discarded")
    MSG (WARN_ELSE, warn_else, "dangling 'else' statement")
    MSG (WARN_FORMAT, warn_format, "format mismatch with parameter %d on function '%s()'")
    MSG (WARN_IGNORE, warn_ignore, "ignored return value from function %s")
    MSG (WARN_IMPLICITRET, warn_implicitret, "no value specified in implicit 'return' statement")
    MSG (WARN_LABNOTUSED, warn_labnotused, "label '%s' declared but not used")
    MSG (WARN_MINUS, warn_minus, "unary '-' applied to unsigned expression")
    MSG (WARN_MODIFIED, warn_modified, "'%s' modified more than once between sequence points")
    MSG (WARN_NOPROTO, warn_noproto, "no prototype defined on called function %s")
    MSG (WARN_NOTSET, warn_notset, "variable '%s' may be used before set")
    MSG (WARN_NOTUSED, warn_notused, "variable/function '%s' not used")
/*
 *    Level 6 - Only output if checking for portability
 *      Warnings at this level are normally too pedantic to be useful
 */
    MSG (WARN_CHAR, warn_char, "use of 'char' is possibly non-portable")
    MSG (WARN_CHARINDEX, warn_charindex, "use of 'char' as array index is possibly non-portable")
    MSG (WARN_CONSTANT, warn_constant, "constant promoted to '%s'")
    MSG (WARN_ENUM, warn_enum, "implicit cast of '%s' to enumeration value")
#ifdef FLOAT_CHECK
    MSG (WARN_FLOAT, warn_float, "expression involving floating point")
#endif /* FLOAT_CHECK */
    MSG (WARN_INCOMPLETE, warn_incomplete, "initialisation incomplete - remaining fields zeroed")
    MSG (WARN_NARROWER, warn_narrower, "a cast from '%s' to '%s' loses accuracy")
    MSG (WARN_STDARG2, warn_stdarg2, "parameter before ', ...' causes undefined behaviour")
    MSG (WARN_UNDEFINED, warn_undefined, "signed operand to '%s' implementation defined")
    MSG (WARN_WIDER, warn_wider, "possible unnecessary cast from '%s' to '%s'")
/*
 *    Level 7 - Only output if you are wishing to "tidy" up your code
 *      Warnings at this level are normally too pedantic to be useful
 */
    MSG (WARN_BRACE, warn_brace, "partially elided braces on initializer")
    MSG (WARN_DEFAULT, warn_default, "'switch' has no 'default' statement")
#ifdef FACIST
    MSG (WARN_KEYWORD, warn_keyword, "C++ keyword '%s' used")
#endif /* FACIST */
    MSG (WARN_NOT, warn_not, "'!' operator used with a constant expression")
    MSG (WARN_NULLCAST, warn_nullcast, "implicit cast of 0 to pointer type")
    MSG (WARN_OLDDEF, warn_olddef, "function not using ANSI style parameters")
    MSG (WARN_VOID, warn_void, "unnecessary cast to 'void'")
#ifdef FACIST
/*
 *    Level 8 - Only output if really strict checking is to be done.
 *      Warnings at this level are really "facist".
 */
    MSG (WARN_DUPDECL, warn_dupdecl, "'%s' has already been declared")
    MSG (WARN_NOIMPLICIT, warn_noimplicit, "implicit cast from '%s' to '%s'")
    MSG (WARN_PREVDECL, warn_prevdecl, "'%s' has not been previously declared")
#endif /* FACIST */
#endif /* SYNTAX_CORRECT */
/*
 *    Messages below this point are output without file/line number details
 */
/*
 *    Miscellaneous messages
 */
    MSG (MSG_ENUMOPT, msg_enumopt, "unknown enumerated value '%s' to option '-%s'")
    MSG (MSG_ERROR, msg_error, "error")
    MSG (MSG_ERRORCNT, msg_errorcnt, "\n -- %d errors found")
    MSG (MSG_EXTRAPARAM, msg_extraparam, "too many parameters supplied")
    MSG (MSG_FATAL, msg_fatal, "FATAL error encountered in file '%s' routine '%s'\nMessage: ")
    MSG (MSG_LINE, msg_line, "\"%s\", line %u: ")
    MSG (MSG_LOCALMEM, msg_localmem, "not enough local memory (%d Kbytes used) [global %d Kbytes]")
    MSG (MSG_MAXERROR, msg_maxerror, "Program terminated due to maximum error count")
    MSG (MSG_MAXMEMORY, msg_maxmemory, "Maximum memory request was %d Kbytes")
    MSG (MSG_MISSING, msg_missing, "\n(this may occur due to a feature left out of the compiler, or just an\ninternal compiler error)")
    MSG (MSG_NOMEMORY, msg_nomemory, "not enough global memory (%d Kbytes used) [local %d Kbytes]")
    MSG (MSG_OPENINPUT, msg_openinput, "Cannot open file '%s' for input")
#ifdef LIST
    MSG (MSG_OPENLISTING, msg_openlisting, "Cannot open file '%s' for listing")
#endif /*LIST */
    MSG (MSG_OPENOUTPUT, msg_openoutput, "Cannot open file '%s' for output")
    MSG (MSG_OPTPHASE, msg_optphase, "\n%sOption settings:")
    MSG (MSG_PEEPLABEL, msg_peeplabel, "INCONSISTENCY: PEEP LABEL (FATAL?)")
#ifdef VERBOSE
    MSG (MSG_PEEPCHANGES, msg_peepchanges, "\t%d changes made")
    MSG (MSG_RELEASEGLB, msg_releaseglb, "releasing %d Kbytes global tables")
    MSG (MSG_RELEASELOC, msg_releaseloc, "releasing %d Kbytes local tables")
#endif /* VERBOSE */
#ifdef SIGNAL
    MSG (MSG_SIGNAL, msg_signal, "TERMINATING: - signal %d received\n")
#endif /* SIGNAL */
#ifdef VERBOSE
    MSG (MSG_TIMES, msg_times, "Times: %ld + %ld + %ld + %ld")
#endif /* VERBOSE */
    MSG (MSG_UNKNOWNOPT, msg_unknownopt, "option '%s' not recognised")
    MSG (MSG_USAGE, msg_usage, "\nUsage: %s [options] [input_file [output_file [listing_file]]]")
    MSG (MSG_WARNING, msg_warning, "warning")
    MSG (MSG_WRITEFAIL, msg_writefail, "write failure")
