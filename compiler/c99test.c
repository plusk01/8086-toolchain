/*----------------------------------------------------------------------
 *
 *		c99test.c
 *
 *	This program is designed to help test new features introduced
 *	into the compiler as a result of the ISO 1999 C standard
 *
 *	In particular it is intended to test the new data types with all
 *	the standard operators.
 *
 *	Its expected use is with a version of the c68 compiler that has
 *	been compiled with LONGLONG_BOOTSTRAP defined (but not LONGLONG)
 *	so that all operations that are likely to be used internally by
 *	the compiler can be tested prior to trying to complete the
 *	bootstrap process
 *
 *	LAST AMENDED
 *	~~~~~~~~~~~~
 *	14 Oct 2000 DJW   - First version completed for tests for booting
 *						C68 with the 'long long' data type.
 *
 *---------------------------------------------------------------------*/

/*
 *	Define the following if floating point not supported in compiler
 */
#define NOFLOAT

/*
 *	Define the following to specify whether variables are byte reversed
 */
#ifdef WIN32
#define BYTE_REVERSED	TRUE
#else
#define BYTE_REVERSED	FALSE
#endif

/*
 *	Defines for using the 64 bit data type supported by Visual C++
 *	Useful for testing this program prior to C68 workign correctly
 */
#ifdef WIN32
#define LONGLONG __int64
#else
#define LONGLONG long long
#endif

#define TRUE	1
#define FALSE	0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char	_prog_name[] = "C99Test";

/*
 *	List of operator types that are tested for.
 */
enum op_types {
	op_add=1,
	op_subtract,
	op_multiply,
	op_divide,
	op_remainder,
	/* logical */
	op_less,
	op_greater,
	op_equality,
	op_inequality,
	op_logicaland,
	op_logicalor,
	op_hook,

	/* unary */
	op_minus,
	op_not,
	op_postincrement,
	op_preincrement,
	op_postdecrement,
	op_predecrement,
	/* bitwise */
	op_and,
	op_or,
	op_xor,
	op_negate,
	op_lsh,
	op_rsh,
	
	op_cast,
	/* END entry */
	op_maximum
};

static int need_getchar;	   /* Used to control whether getchar() needed */
static int print_count;
#define PRINT_COUNT_MAX 10	   /* Number of prints between getchar() */


union {
	long	l[2];
	unsigned long ul[2];
	LONGLONG	ll;
	unsigned LONGLONG	ull;
	} ux,  uy, uz, uxu, uyu, uzu;

long x, y, z;

/*
 *	Print out results of signed long long calculations
 */
void	result_ll (char * prompt,  long result, int line)
{
	printf ("%4.d: %-8s %-30s $%08.8lx %08.8lx = %ld",
			line,
			result == uz.l[BYTE_REVERSED==FALSE] ? "OK" : "FAILED",
			prompt,
			uz.l[BYTE_REVERSED == TRUE],
			uz.l[BYTE_REVERSED == FALSE],
			uz.l[BYTE_REVERSED == FALSE]);

	if (result != uz.l[BYTE_REVERSED==FALSE])
	{
		printf (" (%ld)", result);
	}
	printf ("\n");
	print_count++;

	if (ux.l[BYTE_REVERSED == FALSE] != x)
	{
		printf ("%4.d: FAILED - x value corrupted (now %ld)\n",
				line,
				ux.l[BYTE_REVERSED == FALSE]);
		print_count++;
	}
	if (uy.l[BYTE_REVERSED == FALSE] != y)
	{
		printf ("%4.d: FAILED - y value corrupted (now %ld)\n",
				line,
				uy.l[BYTE_REVERSED == FALSE]);
		print_count++;
	}
	if (need_getchar && print_count >= PRINT_COUNT_MAX)
	{
		print_count = 0;
		fgetc(stderr);
	}
	return;
}

void	result_ll_val (char * op_prompt, long val1, long val2, long result, int line)
{
	char	prompt[100];

	if (0 == val2)
		sprintf (prompt, "%ldLL %s",
				 val1,
				 op_prompt);
	else
		sprintf (prompt, "%ldLL %s %ldLL",
				 val1,
				 op_prompt,
				 val2);

	result_ll (prompt,result, line);
}

/*
 *	Print out results of unsigned long long calculations
 */
void	result_ull (char * prompt,	unsigned long result, int line)
{
	printf ("%4.d: %-8s %-30s $%08.8lx %08.8lx = %lu",
			line,
			result == uz.ul[BYTE_REVERSED==FALSE] ? "OK" : "FAILED",
			prompt,
			uz.ul[BYTE_REVERSED==TRUE],
			uz.ul[BYTE_REVERSED==FALSE],
			uz.ul[BYTE_REVERSED==FALSE]);

	if (result != uz.ul[BYTE_REVERSED==FALSE])
	{
		printf (" (%lu)", result);
	}
	printf ("\n");
	print_count++;

	if (ux.l[BYTE_REVERSED == FALSE] != x)
	{
		printf ("%4.d: FAILED - x value corrupted (now %lu)\n",
				line,
				ux.ul[BYTE_REVERSED == FALSE]);
	}
	if (uy.l[BYTE_REVERSED == FALSE] != y)
	{
		printf ("%4.d: FAILED - y value corrupted (now %lu)\n",
				line,
				uy.ul[BYTE_REVERSED == FALSE]);
	}
	if (need_getchar && print_count >= PRINT_COUNT_MAX)
	{
		print_count = 0;
		fgetc(stderr);
	}
	return;
}

void	result_ull_val (char * op_prompt, unsigned long val1, unsigned long val2, unsigned long result, int line)
{
	char	prompt[100];

	if (0== val2)
		sprintf (prompt, "%luULL %s",
				 val1,
				 op_prompt);
	else
		sprintf (prompt, "%luULL %s %luULL",
				 val1,
				 op_prompt,
				 val2);

	result_ull (prompt,result, line);
}

/*
 *	Do the specified operation using signed operands
 */
void	op_test_ll (int op_type, int x_val, int y_val)
{
	x = x_val;	y = y_val;
	ux.ll = x_val;	uy.ll = y_val;
	switch (op_type)
	{
		case op_add:
				z = x + y;
				uz.ll = ux.ll + uy.ll;
				result_ll_val("+",x_val,y_val,z,__LINE__ -1);
				z = x; z += y;
				uz.ll = ux.ll ;uz.ll += uy.ll;
				result_ll_val("+=",x_val,y_val,z,__LINE__ -1);
				break;
		case op_subtract:
				z = x - y;
				uz.ll = ux.ll - uy.ll;
				result_ll_val("-",x_val,y_val,z,__LINE__ -1);
				z = x; z -= y;
				uz.ll = ux.ll ;uz.ll -= uy.ll;
				result_ll_val("-=",x_val,y_val,z,__LINE__ -1);
				break;
		case op_multiply:
				z = x * y;
				uz.ll = ux.ll * uy.ll;
				result_ll_val("*",x_val,y_val,z,__LINE__ -1);
				z = x; z *= y;
				uz.ll = ux.ll ;uz.ll *= uy.ll;
				result_ll_val("*=",x_val,y_val,z,__LINE__ -1);
				break;
		case op_divide:
				z = x / y;
				uz.ll = ux.ll / uy.ll;
				result_ll_val("/",x_val,y_val,z,__LINE__ -1);
				z = x; z /= y;
				uz.ll = ux.ll ;uz.ll /= uy.ll;
				result_ll_val("/=",x_val,y_val,z,__LINE__ -1);
				break;
		case op_remainder:
				z = x % y;
				uz.ll = ux.ll % uy.ll;
				result_ll_val("%",x_val,y_val,z,__LINE__ -1);
				z = x; z %= y;
				uz.ll = ux.ll ;uz.ll %= uy.ll;
				result_ll_val("%=",x_val,y_val,z,__LINE__ -1);
				break;
	/* logical */
		case op_less:
				z = x < y;
				uz.ll = ux.ll < uy.ll;
				result_ll_val("<",x_val,y_val,z,__LINE__ -1);
				z = x <= y;
				uz.ll = ux.ll <= uy.ll;
				result_ll_val("<=",x_val,y_val,z,__LINE__ -1);
				break;
		case op_greater:
				z = x > y;
				uz.ll = ux.ll > uy.ll;
				result_ll_val(">",x_val,y_val,z,__LINE__ -1);
				z = x >= y;
				uz.ll = ux.ll >= uy.ll;
				result_ll_val(">=",x_val,y_val,z,__LINE__ -1);
				break;
		case op_equality:
				z = x == y;
				uz.ll = ux.ll == uy.ll;
				result_ll_val("==",x_val,y_val,z,__LINE__ -1);
				break;
		case op_inequality:
				z = x != y;
				uz.ll = ux.ll != uy.ll;
				result_ll_val("!=",x_val,y_val,z,__LINE__ -1);
				break;
		case op_logicaland:
				z = x && y;
				uz.ll = ux.ll && uy.ll;
				result_ll_val("&&",x_val,y_val,z,__LINE__ -1);
				break;
		case op_logicalor:
				z = x || y;
				uz.ll = ux.ll || uy.ll;
				result_ll_val("||",x_val,y_val,z,__LINE__ -1);
				break;
#if 0
		case op_hook:
				z = (x == y ? 0 : 1);
				uz.ll = (ux.ll == uy.ll ? 0 : 1);
				result_ll_val("?:",x_val,y_val,z,__LINE__ -1);
				break;
#endif
	/* unary */
		case op_minus:
				z = -x;
				uz.ll = -ux.ll;
				result_ll_val("- (unary)",x_val, 0L, z,__LINE__ -1);
				break;
		case op_not:
				z = !x;
				uz.ll = !ux.ll;
				result_ll_val("! (unary)",x_val, 0L, z,__LINE__ -1);
				break;
		case op_postincrement:
				z = x++;
				uz.ll = ux.ll++;
				result_ll_val("++(post)",x_val, 0L, z,__LINE__ -1);
				z = x; z++;
				uz.ll = ux.ll;	uz.ll++;
				result_ll_val("++(post - unary)",x_val, 0L, z,__LINE__ -1);
				break;
		case op_preincrement:
				z = ++x;
				uz.ll = ++ux.ll;
				result_ll_val("++(pre)",x_val, 0L, z,__LINE__ -1);
				z = x; ++z;
				uz.ll = ux.ll;	++uz.ll;
				result_ll_val("++(pre - unary)",x_val, 0L, z,__LINE__ -1);
				break;
		case op_postdecrement:
				z = x--;
				uz.ll = ux.ll--;
				result_ll_val("--(post)",x_val, 0L, z,__LINE__ -1);
				z = x; z--;
				uz.ll = ux.ll;	uz.ll--;
				result_ll_val("--(post - unary)",x_val, 0L, z,__LINE__ -1);
				break;
		case op_predecrement:
				z = --x;
				uz.ll = --ux.ll;
				result_ll_val("--(pre)",x_val, 0L, z,__LINE__ -1);
				z = x; --z;
				uz.ll = ux.ll;	--uz.ll;
				result_ll_val("--(pre - unary)",x_val, 0L, z,__LINE__ -1);
				break;
	/* bitwise */
		case op_and:
				z = x & y;
				uz.ll = ux.ll & uy.ll;
				result_ll_val("&",x_val, y_val, z,__LINE__ -1);
				z = x; z &= y;
				uz.ll = ux.ll;	uz.ll &= uy.ll;
				result_ll_val("&=",x_val, y_val, z,__LINE__ -1);
				break;
		case op_or:
				z = x | y;
				uz.ll = (ux.ll | uy.ll);
				result_ll_val("|",x_val, y_val, z,__LINE__ -1);
				z = x; z |= y;
				uz.ll = ux.ll;	uz.ll |= uy.ll;
				result_ll_val("|=",x_val, y_val, z,__LINE__ -1);
				break;
		case op_xor:
				z = x ^ y;
				uz.ll = (ux.ll ^ uy.ll);
				result_ll_val("^",x_val, y_val, z,__LINE__ -1);
				z = x; z ^= y;
				uz.ll = ux.ll;	uz.ll ^= uy.ll;
				result_ll_val("^=",x_val, y_val, z,__LINE__ -1);
				break;
		case op_negate:
				z = ~x;
				uz.ll = ~ux.ll;
				result_ll_val("~ (bitwise)",x_val, 0L, z,__LINE__ -1);
				break;
		case op_lsh:
				z = x << y;
				uz.ll = (ux.ll << uy.ll);
				result_ll_val("<<",x_val, y_val, z,__LINE__ -1);
				z = x; z <<= y;
				uz.ll = ux.ll; uz.ll <<= uy.ll;
				result_ll_val("<<=",x_val, y_val, z,__LINE__ -1);
				break;
		case op_rsh:
				z = x >> y;
				uz.ll = (ux.ll >> uy.ll);
				result_ll_val(">>",x_val, y_val, z,__LINE__ -1);
				z = x; z >>= y;
				uz.ll = ux.ll; uz.ll >>= uy.ll;
				result_ll_val(">>=",x_val, y_val, z,__LINE__ -1);
				break;
		/* Casting */
		case op_cast:
			{
#ifndef NOFLOAT
				float f;
				double	d;
#endif
				char	c;
				unsigned char uc;
				short s;
				unsigned short us;
				int 	i;
				unsigned int ui;
				long	l;
				unsigned long ul;

#ifndef NOFLOAT
				f = (float)x;
				uz.ll = (LONGLONG)f;
				result_ll_val("cast from float",x_val, 0L, (long)f,__LINE__ -1);
				d = x;
				uz.ll = (LONGLONG)d;
				result_ll_val("cast from double",x_val, 0L, (long)d,__LINE__ -1);
#endif
				c = (char)x;
				uz.ll = c;
				result_ll_val("cast from char",x_val, 0L, c,__LINE__ -1);
				uc = (unsigned char)x;
				uz.ll = uc;
				result_ll_val("cast from unsigned char",x_val, 0L, (long)uc,__LINE__ -1);
				s = (short)x;
				uz.ll = s;
				result_ll_val("cast from short",x_val, 0L, (long)s,__LINE__ -1);
				us = (unsigned short)x;
				uz.ll = us;
				result_ll_val("cast from unsigned short",x_val, 0L, (long)us,__LINE__ -1);
				i = x;
				uz.ll = i;
				result_ll_val("cast from int",x_val, 0L, (long)i,__LINE__ -1);
				ui = x;
				uz.ll = ui;
				result_ll_val("cast from unsigned int",x_val, 0L, (long)ui,__LINE__ -1);
				l = x;
				uz.ll = l;
				result_ll_val("cast from long",x_val, 0L, (long)l,__LINE__ -1);
				ul = x;
				uz.ll = ul;
				result_ll_val("cast from unsigned long",x_val, 0L, (long)ul,__LINE__ -1);
				break;
			}

	}
}

/*
 *	Do the specified operations using unsigned operands
 */

void	op_test_ull (int op_type, unsigned int x_val, unsigned int y_val)
{
	x = x_val;	y = y_val;
	ux.ull = x_val; uy.ull = y_val;
	switch (op_type)
	{
		case op_add:
				z = x + y;
				uz.ull = ux.ull + uy.ull;
				result_ull_val("+",x_val,y_val,z,__LINE__ -1);
				z = x; z += y;
				uz.ull = ux.ull ;uz.ull += uy.ull;
				result_ull_val("+=",x_val,y_val,z,__LINE__ -1);
				break;
		case op_subtract:
				z = x - y;
				uz.ull = ux.ull - uy.ull;
				result_ull_val("-",x_val,y_val,z,__LINE__ -1);
				z = x; z -= y;
				uz.ull = ux.ull ;uz.ull -= uy.ull;
				result_ull_val("-=",x_val,y_val,z,__LINE__ -1);
				break;
		case op_multiply:
				z = x * y;
				uz.ull = ux.ull * uy.ull;
				result_ull_val("*",x_val,y_val,z,__LINE__ -1);
				z = x; z *= y;
				uz.ull = ux.ull ;uz.ull *= uy.ull;
				result_ull_val("*=",x_val,y_val,z,__LINE__ -1);
				break;
		case op_divide:
				z = x / y;
				uz.ll = ux.ll / uy.ll;
				result_ll_val("/",x_val,y_val,z,__LINE__ -1);
				z = x; z /= y;
				uz.ll = ux.ll ;uz.ll /= uy.ll;
				result_ll_val("/=",x_val,y_val,z,__LINE__ -1);
				break;
		case op_remainder:
				z = x % y;
				uz.ull = ux.ull % uy.ull;
				result_ull_val("%",x_val,y_val,z,__LINE__ -1);
				z = x; z %= y;
				uz.ull = ux.ull ;uz.ull %= uy.ull;
				result_ull_val("%=",x_val,y_val,z,__LINE__ -1);
				break;
	/* logical */
		case op_less:
				z = x < y;
				uz.ull = ux.ull < uy.ull;
				result_ull_val("<",x_val,y_val,z,__LINE__ -1);
				z = x <= y;
				uz.ull = ux.ull <= uy.ull;
				result_ull_val("<=",x_val,y_val,z,__LINE__ -1);
				break;
		case op_greater:
				z = x > y;
				uz.ull = ux.ull > uy.ull;
				result_ull_val(">",x_val,y_val,z,__LINE__ -1);
				z = x >= y;
				uz.ull = ux.ull >= uy.ull;
				result_ull_val(">=",x_val,y_val,z,__LINE__ -1);
				break;
		case op_equality:
				z = x == y;
				uz.ull = ux.ull == uy.ull;
				result_ull_val("==",x_val,y_val,z,__LINE__ -1);
				break;
		case op_inequality:
				z = x != y;
				uz.ull = ux.ull != uy.ull;
				result_ull_val("!=",x_val,y_val,z,__LINE__ -1);
				break;
		case op_logicaland:
				z = x && y;
				uz.ull = ux.ull && uy.ull;
				result_ull_val("&&",x_val,y_val,z,__LINE__ -1);
				break;
		case op_logicalor:
				z = x || y;
				uz.ull = ux.ull || uy.ull;
				result_ull_val("||",x_val,y_val,z,__LINE__ -1);
				break;
#if 0
		case op_hook:
				z = (x == y ? 0 : 1);
				uz.ull = (ux.ull == uy.ull ? 0 : 1);
				result_ull_val("?:",x_val,y_val,z,__LINE__ -1);
				break;
#endif
	/* unary */
#if 0
		case op_minus:
				z = -x;
				uz.ull = -ux.ull;
				result_ull_val("-",x_val, 0L, z,__LINE__ -1);
				break;
#endif
		case op_not:
				z = !x;
				uz.ull = !ux.ull;
				result_ull_val("!",x_val, 0L, z,__LINE__ -1);
				break;
		case op_postincrement:
				z = x++;
				uz.ull = ux.ull++;
				result_ull_val("++(post)",x_val, 0L, z,__LINE__ -1);
				z = x; z++;
				uz.ull = ux.ull;  uz.ull++;
				result_ull_val("++(post - unary)",x_val, 0L, z,__LINE__ -1);
				break;
		case op_preincrement:
				z = ++x;
				uz.ull = ++ux.ull;
				result_ull_val("++(pre)",x_val, 0L, z,__LINE__ -1);
				z = x; ++z;
				uz.ull = ux.ull;  ++uz.ull;
				result_ull_val("++(pre - unary)",x_val, 0L, z,__LINE__ -1);
				break;
		case op_postdecrement:
				z = x--;
				uz.ull = ux.ull--;
				result_ull_val("--(post)",x_val, 0L, z,__LINE__ -1);
				z = x; z--;
				uz.ull = ux.ull;  uz.ull--;
				result_ull_val("--(post - unary)",x_val, 0L, z,__LINE__ -1);
				break;
		case op_predecrement:
				z = --x;
				uz.ull = --ux.ull;
				result_ull_val("--(pre)",x_val, 0L, z,__LINE__ -1);
				z = x; --z;
				uz.ull = ux.ull;  --uz.ull;
				result_ull_val("--(pre - unary)",x_val, 0L, z,__LINE__ -1);
				break;
	/* bitwise */
		case op_and:
				z = x & y;
				uz.ull = ux.ull & uy.ull;
				result_ull_val("&",x_val, y_val, z,__LINE__ -1);
				z = x; z &= y;
				uz.ull = ux.ull;  uz.ull &= uy.ull;
				result_ull_val("&=",x_val, y_val, z,__LINE__ -1);
				break;
		case op_or:
				z = x | y;
				uz.ull = (ux.ull | uy.ull);
				result_ull_val("|",x_val, y_val, z,__LINE__ -1);
				z = x; z |= y;
				uz.ull = ux.ull;  uz.ull |= uy.ull;
				result_ull_val("|=",x_val, y_val, z,__LINE__ -1);
				break;
		case op_xor:
				z = x ^ y;
				uz.ull = (ux.ull ^ uy.ull);
				result_ull_val("^",x_val, y_val, z,__LINE__ -1);
				z = x; z ^= y;
				uz.ull = ux.ull;  uz.ull ^= uy.ull;
				result_ull_val("^=",x_val, y_val, z,__LINE__ -1);
				break;
		case op_negate:
				z = ~x;
				uz.ull = ~ux.ull;
				result_ull_val("~",x_val, 0L, z,__LINE__ -1);
				break;
		case op_lsh:
				z = x << y;
				uz.ull = (ux.ull << uy.ull);
				result_ull_val("<<",x_val, y_val, z,__LINE__ -1);
				z = x; z <<= y;
				uz.ull = ux.ull; uz.ull <<= uy.ull;
				result_ull_val("<<=",x_val, y_val, z,__LINE__ -1);
				break;
		case op_rsh:
				z = x >> y;
				uz.ull = (ux.ull >> uy.ull);
				result_ull_val(">>",x_val, y_val, z,__LINE__ -1);
				z = x; z >>= y;
				uz.ull = ux.ull; uz.ull >>= uy.ull;
				result_ull_val(">>=",x_val, y_val, z,__LINE__ -1);
				break;
		/* Casting */
		case op_cast:
			{
#ifndef NOFLOAT
				float f;
				double	d;
#endif
				char	c;
				unsigned char uc;
				short s;
				unsigned short us;
				int 	i;
				unsigned int ui;
				long	l;
				unsigned long ul;

#ifndef NOFLOAT
				f = (float)x;
				ux.ull = (LONGLONG)f;
				result_ull_val("cast from float",x_val, 0L, (unsigned long)f,__LINE__ -1);
				d = x;
				ux.ull = (LONGLONG)d;
				result_ull_val("cast from double",x_val, 0L, (unsigned long)d,__LINE__ -1);
#endif
				c = (char)x;
				uz.ull = c;
				result_ull_val("cast from char",x_val, 0L, (unsigned long)c,__LINE__ -1);
				uc = (unsigned char)x;
				uz.ull = uc;
				result_ull_val("cast from unsigned char",x_val, 0L, (unsigned long)uc,__LINE__ -1);
				s = (short)x;
				uz.ull = s;
				result_ull_val("cast from short",x_val, 0L, (unsigned long)s,__LINE__ -1);
				us = (unsigned short)x;
				uz.ull = us;
				result_ull_val("cast from unsigned short",x_val, 0L, (unsigned long)us,__LINE__ -1);
				i = x;
				uz.ull = i;
				result_ull_val("cast from int",x_val, 0L, (unsigned long)i,__LINE__ -1);
				ui = x;
				uz.ull = ui;
				result_ull_val("cast from unsigned int",x_val, 0L, (unsigned long)ui,__LINE__ -1);
				l = x;
				ux.ull = l;
				result_ull_val("cast from long",x_val, 0L, (unsigned long)l,__LINE__ -1);
				ul = x;
				uz.ull = ul;
				result_ull_val("cast from unsigned long",x_val, 0L, (unsigned long)ul,__LINE__ -1);
				break;
			}

	}
}

#if 0
LONGLONG	func_ll (LONGLONG param1, long param2, int line)
{
	printf ("%4.d: %-8s Function call with long long parameter\n", line, param1 == param2 ? "OK" : "FAILED");
	return param1;
}


unsigned LONGLONG	func_ull (unsigned LONGLONG param1, unsigned long param2, int line)
{
	printf ("%4.d: %-8s Function call with unsigned long long parameter\n", line, param1 == param2 ? "OK" : "FAILED");
	return param1;
}
#endif

void main (int argc, char * argv[])
{
	int i;
	LONGLONG	ll;
	unsigned LONGLONG	ull;
	char	filename[200];


	if (argc > 1)
	{
		if (NULL == freopen (argv[1], "w", stdout))
		{
			fprintf (stderr, "%s: Failed to open file '%s' for output\n",
					 _prog_name, argv[1]);
			exit (-1);
		}
		else
		{
			need_getchar = FALSE;
		}
	}
	else
	{
		printf ("C99TEST\n");
		printf ("~~~~~~~\n");

		printf ("Enter filename for results\n(or ENTER for stout)\n:  ");
		gets(filename);
		if (strlen(filename))
		{
			if (NULL == freopen (filename, "w", stdout))
			{
				fprintf (stderr, "%s: Failed to open file '%s' for output\n",
						 _prog_name, filename);
				exit (-1);
			}
			else
			{
				need_getchar = FALSE;
			}
		}
		else
		{
			need_getchar = TRUE;
		}

	}

	if (FALSE == need_getchar)
	{
		printf ("C99TEST\n");
		printf ("~~~~~~~\n");
	}

	printf ("Operator Tests:\n\n");

	printf ("Line  Status   Test Performed                  High     Low      = Result (expected)\n");

	for (i = op_add ; i < op_maximum ; i++ )
	{
			op_test_ll (i, 20,3);
			op_test_ll (i, 20,-3);
			op_test_ll (i, -20,3);
			op_test_ll (i, -20,-3);
			op_test_ull (i, 20,3);
	}

	printf ("\nFunction Call Tests:\n\n");


#if 0
	ll = func_ll((LONGLONG)27,27,__LINE__);
	printf ("%4.d: %-8s Function call returning long long\n", __LINE__ -1, ll == 27 ? "OK" : "FAILED");

	ull = func_ull((unsigned LONGLONG)27,27,__LINE__);
	printf ("%4.d: %-8s Function call returning unsigned long long\n", __LINE__ -1, ull == 27 ? "OK" : "FAILED");
#endif

	printf ("\nTESTS COMPLETED\n");
	if (need_getchar)
	{
		fgetc(stderr);
	}
	else
	{
		fprintf (stderr,"\nTESTS COMPLETED\n");
	}
	exit(0);
}
