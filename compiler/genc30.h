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

#ifndef _GENC30_H
#define _GENC30_H

#ifdef DEBUG
/*#define REGISTER_FLOW_ANALYZER
 */
#endif
/* enable data-flow-analyzer which differs between float and integer types */
#define NO_MIX_WITH_FLOAT

/*#define SAVE_PEEP_MEMORY */

typedef unsigned int FLAGS;

#define	F_NONE ((FLAGS) 0U)
#define	F_DREG ((FLAGS) 1U)	/* data register direct mode allowed (INT)      */
#define	F_FREG ((FLAGS) 2U)	/* float register direct mode allowed (FLOAT)   */
#define	F_AREG ((FLAGS) 4U)	/* address register direct mode allowed */
#define	F_IREG ((FLAGS) 8U)	/* Indexregister needed */
#define	F_MEM ((FLAGS) 16U)	/* memory alterable modes allowed */
#define	F_IMMED ((FLAGS) 32U)	/* immediate mode allowed */
#define	F_VOL ((FLAGS) 64U)	/* need volitile operand  */
#define	F_NOVALUE ((FLAGS) 128U)	/* dont need result value  */
#define	F_USES ((FLAGS) 256U)	/* need result value more than once */
					/* this forbids autoincrement modes */
#define	F_DIRECT ((FLAGS) 512U)	/* Immediate will later be used as direct */
#define	F_UNSIGNED ((FLAGS) 1024U)	/* Immediate will later be treated as unsigned  */

#define F_XREG          ((FLAGS)(F_DREG | F_AREG | F_IREG))
#define F_ALL           ((FLAGS)(F_DREG | F_FREG | F_AREG | F_IREG | F_MEM | F_IMMED))
#define F_FALL          ((FLAGS)(F_FREG | F_MEM | F_IMMED))
#define F_IALL          ((FLAGS)(F_DREG | F_AREG | F_IREG | F_MEM | F_IMMED))

/* The instructions */

/*
 * The order of the branch instructions must not be changed
 * since array revcond[] in the peephole optimizer relies on
 * them.
 * all arithmethical operations on r0 to r7 affects flags
 * only compare and test set Flags with all registers !!!
 * load of r0-r7 set flags too
 * load of other registers doesn't change flags
 */
enum e_op
{
    op_absf,			/* float: abs(src) -> dst         */
    op_absi,			/* int:   abs(src) -> dst         */
#ifdef USE_ALL_OPCODES
    op_addc,			/* int:   dst + src + cary -> dst */
#endif
    op_addf,			/* float: dst + src -> dst        */
    op_addi,			/* int:   dst + src -> dst        */
    op_and,			/* int:   dst & src -> dst        */
    op_andn,			/* int:   dst & ~src -> dst       */
    op_ash,			/* int:   dst << src -> dst (if src < 0: >> abs(src))  */
    op_cmpf,			/* float: dst - src -> NIL        */
    op_cmpi,			/* int:   dst - src -> NIL        */
    op_fix,			/* int:   int(src) -> dst, src is float */
    op_float,			/* float: float(src) -> dst, src is int */
#ifdef USE_ALL_OPCODES
    op_idle,			/*  idle until interupt (wait for interrupt) */
    op_lde,			/* float: dst.exponentfield -> dst.exponentfield */
#endif
    op_ldf,			/* float: src -> dst              */
    op_popldf,			/* float: src -> dst  pseudo op, like ldf, but peep wont touch it */
#ifdef USE_ALL_OPCODES
    op_ldfi,			/* float: src -> dst, interloked (Shared memoryacccess) */
#endif
    op_ldi,			/* int:   src -> dst              */
    op_popldi,			/* int:   src -> dst  pseudo op, like ldi, but peep wont touch it */
#ifdef USE_ALL_OPCODES
    op_ldii,			/* int:   src -> dst, interloked (Shared memoryacccess) */
    op_ldm,			/* float: src.mantissa -> dst.mantissa  */
#endif
    op_lsh,			/* uint:  dst << src -> dst (if src < 0: >> abs(src))   */
    op_mpyf,			/* float: dst * src -> dst        */
    op_mpyi,			/* int:   dst * src -> dst        */
#ifdef USE_ALL_OPCODES
    op_negb,			/* int:   0 - src - cary -> dst   */
#endif
    op_negf,			/* float: 0.0 - src -> dst        */
    op_negi,			/* int:   0 - src   -> dst        */
    op_nop,			/* no operation, addressarithmetic on src may be performed */
#ifdef USE_ALL_OPCODES
    op_norm,			/* float: normalize(src) -> dst   */
#endif
    op_not,			/* int:   ~src -> dst             */
    op_pop,			/* int:   *sp-- -> dst            */
    op_popf,			/* float: *sp-- -> dst            */
    op_push,			/* int:   src -> *++sp            */
    op_pushf,			/* float: src -> *++sp            */
    op_pushnopeep,		/* int:   src -> *++sp   (pseudoop)     */
    op_pushfnopeep,		/* float: src -> *++sp   (pseudoop)     */
    op_or,			/* int:   dst | src -> dst        */
#ifdef USE_ALL_OPCODES
    op_rnd,			/* float: round(src) -> dst       */
    op_rol,			/* int:   rotateleft_1_bit(dst) -> dst       */
    op_rolc,			/* int:   rotateleft_1_bit_carry(dst) -> dst */
    op_ror,			/* int:   rotateright_1_bit(dst) -> dst      */
    op_rorc,			/* int:   rotateright_1_bit_carry(dst) -> dst */
#endif
    op_rpts,			/* repeat following instruction src times    */
    op_stf,			/* float: src -> dst<memory>      */
#ifdef USE_ALL_OPCODES
    op_stfi,			/* float: src -> dst<memory>, interloked (shared memoryaccess) */
#endif
    op_sti,			/* int:   src -> dst<memory>      */
#ifdef USE_ALL_OPCODES
    op_stii,			/* int:   src -> dst<memory>, interloked (shared memoryaccess) */
    op_sigi,			/* signal interloked, (synchronice multiple processors) */
    op_subb,			/* int:   dst - src - cary -> dst */
    op_subc,			/* int:   conditional subtract and shift, part of divisionsroutine */
#endif
    op_subf,			/* float: dst - src -> dst        */
    op_subi,			/* int:   dst - src -> dst        */
#ifdef USE_ALL_OPCODES
    op_subrb,			/* int:   src - dst - cary -> dst (dst and src exchanged) */
#endif
    op_subrf,			/* float: src - dst -> dst (dst and src exchanged) */
    op_subri,			/* int:   src - dst -> dst (dst and src exchanged) */
    op_tstb,			/* int:   dst & src -> NIL        */
    op_xor,			/* int:   dst ^ src -> dst        */
#ifdef USE_ALL_OPCODES
    op_iack,			/* generate iack pulse (interrupt aknowledge) */
    op_addc3,			/* int:   src1 + src2 + cary -> dst */
#endif
    op_addf3,			/* float: src1 + src2 -> dst        */
    op_addi3,			/* int:   src1 + src2 -> dst        */
    op_and3,			/* int:   src1 & src2 -> dst        */
    op_andn3,			/* int:   src1 & ~src2 -> dst       */
    op_ash3,			/* int:   src2 << src1 -> dst (if src1 < 0: >> abs(src1)) */
    op_cmpf3,			/* float: src1 - src2 -> NIL        */
    op_cmpi3,			/* int:   src1 - src2 -> NIL        */
    op_lsh3,			/* int:   src2 << src1 -> dst (if src1 < 0: >> abs(src1)) */
    op_mpyf3,			/* float: src1 * src2 -> dst        */
    op_mpyi3,			/* int:   src1 * src2 -> dst        */
    op_or3,			/* int:   src1 | src2 -> dst        */
#ifdef USE_ALL_OPCODES
    op_subb3,			/* int:   src1 - src2 -cary -> dst  */
#endif
    op_subf3,			/* float: src1 - src2 -> dst        */
    op_subi3,			/* int:   src1 - src2 -> dst        */
    op_tstb3,			/* int:   src1 & src2 -> NIL        */
    op_xor3,			/* int:   src1 ^ src2 -> dst        */
    op_ldfu,			/* float: src -> dst, flags unaffected */
    op_ldflo,			/* float: if lower:       src -> dst, flags unaffected */
    op_ldfls,			/* float: if lowerorsame: src -> dst, flags unaffected */
    op_ldfhi,			/* float: if higher:      src -> dst, flags unaffected */
    op_ldfhs,			/* float: if higerorsame: src -> dst, flags unaffected */
    op_ldfeq,			/* float: if equal:       src -> dst, flags unaffected */
    op_ldfne,			/* float: if notequal:    src -> dst, flags unaffected */
    op_ldflt,			/* float: if lessthan:    src -> dst, flags unaffected */
    op_ldfle,			/* float: if lessorequal: src -> dst, flags unaffected */
    op_ldfgt,			/* float: if greater:     src -> dst, flags unaffected */
    op_ldfge,			/* float: if greatorequal:src -> dst, flags unaffected */
    op_ldfz,			/* float: if zero:        src -> dst, flags unaffected */
    op_ldfnz,			/* float: if nozero:      src -> dst, flags unaffected */
    op_ldfp,			/* float: if positive:    src -> dst, flags unaffected */
    op_ldfn,			/* float: if negative:    src -> dst, flags unaffected */
    op_ldfnn,			/* float: if notnegative: src -> dst, flags unaffected */
    op_ldiu,			/* int:  src -> dst, flags unaffected */
    op_ldilo,			/* int:   if lower:       src -> dst, flags unaffected */
    op_ldils,			/* int:   if lowerorsame: src -> dst, flags unaffected */
    op_ldihi,			/* int:   if higher:      src -> dst, flags unaffected */
    op_ldihs,			/* int:   if higerorsame: src -> dst, flags unaffected */
    op_ldieq,			/* int:   if equal:       src -> dst, flags unaffected */
    op_ldine,			/* int:   if notequal:    src -> dst, flags unaffected */
    op_ldilt,			/* int:   if lessthan:    src -> dst, flags unaffected */
    op_ldile,			/* int:   if lessorequal: src -> dst, flags unaffected */
    op_ldigt,			/* int:   if greater:     src -> dst, flags unaffected */
    op_ldige,			/* int:   if greatorequal:src -> dst, flags unaffected */
    op_ldiz,			/* int:   if zero:        src -> dst, flags unaffected */
    op_ldinz,			/* int:   if nozero:      src -> dst, flags unaffected */
    op_ldip,			/* int:   if positive:    src -> dst, flags unaffected */
    op_ldin,			/* int:   if negative:    src -> dst, flags unaffected */
    op_ldinn,			/* int:   if notnegative: src -> dst, flags unaffected */
    op_br,			/* branch  (absolute jump, whole addressrange)         */
    op_brd,			/* branch delayed, next 3 instructions will be executed */
    op_call,			/* call subroutine  (absolute jump, whole addressrange) */
    op_xcall,			/* pseudo opcode, marks call of supportrutine, but same */
    /* op_call. is needed for peepholeoptimizer            */
    op_rptb,			/* repeat a block from current pc to label (RC) times  */
#ifdef USE_ALL_OPCODES
    op_swi,			/* Softwareinterupt, only for emulator                 */
#endif
    op_bu,			/* branch unconditionally, if dst = register: absolute */
    /* if dest = label: relative, -\+ 32k addressrange     */
    op_blo,			/* same as bu, but conditionally                       */
    op_bls,			/* same as bu, but conditionally                       */
    op_bhi,			/* same as bu, but conditionally                       */
    op_bhs,			/* same as bu, but conditionally                       */
    op_beq,			/* same as bu, but conditionally                       */
    op_bne,			/* same as bu, but conditionally                       */
    op_blt,			/* same as bu, but conditionally                       */
    op_ble,			/* same as bu, but conditionally                       */
    op_bgt,			/* same as bu, but conditionally                       */
    op_bge,			/* same as bu, but conditionally                       */
    op_bz,			/* same as bu, but conditionally                       */
    op_bnz,			/* same as bu, but conditionally                       */
    op_bp,			/* same as bu, but conditionally                       */
    op_bn,			/* same as bu, but conditionally                       */
    op_bnn,			/* same as bu, but conditionally                       */
    op_bud,			/* same as bu, but delayed (next 3 instr. will be executed */
    op_blod,			/* same as bud, but conditionally                      */
    op_blsd,			/* same as bud, but conditionally                      */
    op_bhid,			/* same as bud, but conditionally                      */
    op_bhsd,			/* same as bud, but conditionally                      */
    op_beqd,			/* same as bud, but conditionally                      */
    op_bned,			/* same as bud, but conditionally                      */
    op_bltd,			/* same as bud, but conditionally                      */
    op_bled,			/* same as bud, but conditionally                      */
    op_bgtd,			/* same as bdu, but conditionally                      */
    op_bged,			/* same as bud, but conditionally                      */
    op_bzd,			/* same as bud, but conditionally                      */
    op_bnzd,			/* same as bud, but conditionally                      */
    op_bpd,			/* same as bud, but conditionally                      */
    op_bnd,			/* same as bud, but conditionally                      */
    op_bnnd,			/* same as bud, but conditionally                      */
#ifdef USE_ALL_OPCODES
    op_dbu,			/* decrement src, branch if not zero reached like bu   */
    op_dblo,			/* like dbu, but for branch condition mudst be true    */
    op_dbls,			/* like dbu, but for branch condition mudst be true    */
    op_dbhi,			/* like dbu, but for branch condition mudst be true    */
    op_dbhs,			/* like dbu, but for branch condition mudst be true    */
    op_dbeq,			/* like dbu, but for branch condition mudst be true    */
    op_dbne,			/* like dbu, but for branch condition mudst be true    */
    op_dblt,			/* like dbu, but for branch condition mudst be true    */
    op_dble,			/* like dbu, but for branch condition mudst be true    */
    op_dbgt,			/* like dbu, but for branch condition mudst be true    */
    op_dbge,			/* like dbu, but for branch condition mudst be true    */
    op_dbz,			/* like dbu, but for branch condition mudst be true    */
    op_dbnz,			/* like dbu, but for branch condition mudst be true    */
    op_dbp,			/* like dbu, but for branch condition mudst be true    */
    op_dbn,			/* like dbu, but for branch condition mudst be true    */
    op_dbnn,			/* like dbu, but for branch condition mudst be true    */
    op_dbud,			/* like dbxx, but delayed, next 3 instr will be executed */
    op_dblod,			/* like dbxx, but delayed, next 3 instr will be executed */
    op_dblsd,			/* like dbxx, but delayed, next 3 instr will be executed */
    op_dbhid,			/* like dbxx, but delayed, next 3 instr will be executed */
    op_dbhsd,			/* like dbxx, but delayed, next 3 instr will be executed */
    op_dbeqd,			/* like dbxx, but delayed, next 3 instr will be executed */
    op_dbned,			/* like dbxx, but delayed, next 3 instr will be executed */
    op_dbltd,			/* like dbxx, but delayed, next 3 instr will be executed */
    op_dbled,			/* like dbxx, but delayed, next 3 instr will be executed */
    op_dbgtd,			/* like dbxx, but delayed, next 3 instr will be executed */
    op_dbged,			/* like dbxx, but delayed, next 3 instr will be executed */
    op_dbzd,			/* like dbxx, but delayed, next 3 instr will be executed */
    op_dbnzd,			/* like dbxx, but delayed, next 3 instr will be executed */
    op_dbpd,			/* like dbxx, but delayed, next 3 instr will be executed */
    op_dbnd,			/* like dbxx, but delayed, next 3 instr will be executed */
    op_dbnnd,			/* like dbxx, but delayed, next 3 instr will be executed */
#endif
    op_callu,			/* call unconditionally, if dst = register: absolute     */
    /* if dest = label: relative, -\+ 32k addressrange      */
#ifdef USE_ALL_OPCODES
    op_calllo,			/* like callu, but contitionally                        */
    op_callls,			/* like callu, but contitionally                        */
    op_callhi,			/* like callu, but contitionally                        */
    op_callhs,			/* like callu, but contitionally                        */
    op_calleq,			/* like callu, but contitionally                        */
    op_callne,			/* like callu, but contitionally                        */
    op_calllt,			/* like callu, but contitionally                        */
    op_callle,			/* like callu, but contitionally                        */
    op_callgt,			/* like callu, but contitionally                        */
    op_callge,			/* like callu, but contitionally                        */
    op_callz,			/* like callu, but contitionally                        */
    op_callnz,			/* like callu, but contitionally                        */
    op_callp,			/* like callu, but contitionally                        */
    op_calln,			/* like callu, but contitionally                        */
    op_callnn,			/* like callu, but contitionally                        */
#endif
    op_trapu,			/* trap unconditional (vectorized call, interupts disabled */
#ifdef USE_ALL_OPCODES
    op_traplo,			/* like trapu, but conditionally                        */
    op_trapls,			/* like trapu, but conditionally                        */
    op_traphi,			/* like trapu, but conditionally                        */
    op_traphs,			/* like trapu, but conditionally                        */
    op_trapeq,			/* like trapu, but conditionally                        */
    op_trapne,			/* like trapu, but conditionally                        */
    op_traplt,			/* like trapu, but conditionally                        */
    op_traple,			/* like trapu, but conditionally                        */
    op_trapgt,			/* like trapu, but conditionally                        */
    op_trapge,			/* like trapu, but conditionally                        */
    op_trapz,			/* like trapu, but conditionally                        */
    op_trapnz,			/* like trapu, but conditionally                        */
    op_trapp,			/* like trapu, but conditionally                        */
    op_trapn,			/* like trapu, but conditionally                        */
    op_trapnn,			/* like trapu, but conditionally                        */
#endif
    op_retiu,			/* return from interrupt                                */
#ifdef USE_ALL_OPCODES
    op_retilo,			/* return from interrupt conditionally                  */
    op_retils,			/* return from interrupt conditionally                  */
    op_retihi,			/* return from interrupt conditionally                  */
    op_retihs,			/* return from interrupt conditionally                  */
    op_retieq,			/* return from interrupt conditionally                  */
    op_retine,			/* return from interrupt conditionally                  */
    op_retilt,			/* return from interrupt conditionally                  */
    op_retile,			/* return from interrupt conditionally                  */
    op_retigt,			/* return from interrupt conditionally                  */
    op_retige,			/* return from interrupt conditionally                  */
    op_retiz,			/* return from interrupt conditionally                  */
    op_retinz,			/* return from interrupt conditionally                  */
    op_retip,			/* return from interrupt conditionally                  */
    op_retin,			/* return from interrupt conditionally                  */
    op_retinn,			/* return from interrupt conditionally                  */
#endif
    op_retsu,			/* return from subroutine                               */
#ifdef USE_ALL_OPCODES
    op_retslo,			/* return from subroutine conditionally                 */
    op_retsls,			/* return from subroutine conditionally                 */
    op_retshi,			/* return from subroutine conditionally                 */
    op_retshs,			/* return from subroutine conditionally                 */
    op_retseq,			/* return from subroutine conditionally                 */
    op_retsne,			/* return from subroutine conditionally                 */
    op_retslt,			/* return from subroutine conditionally                 */
    op_retsle,			/* return from subroutine conditionally                 */
    op_retsgt,			/* return from subroutine conditionally                 */
    op_retsge,			/* return from subroutine conditionally                 */
    op_retsz,			/* return from subroutine conditionally                 */
    op_retsnz,			/* return from subroutine conditionally                 */
    op_retsp,			/* return from subroutine conditionally                 */
    op_retsn,			/* return from subroutine conditionally                 */
    op_retsnn,			/* return from subroutine conditionally                 */
    op_mpyf3_addf3,		/* mpyf3 and addf3 parallel, (simultaneously)     */
    op_mpyf3_subf3,		/* mpyf3 and subf3 parallel, (simultaneously)     */
    op_mpyi3_addi3,		/* mpyi3 and addi3 parallel, (simultaneously)     */
    op_mpyi3_subi3,		/* mpyi3 and subi3 parallel, (simultaneously)     */
#endif
    op_stf_stf,			/* two stf parallel, (simultaneously)             */
    op_sti_sti,			/* two sti parallel, (simultaneously)             */
    op_ldf_ldf,			/* two ldf parallel, (simultaneously)             */
    op_ldi_ldi,			/* two ldi parallel, (simultaneously)             */
    op_absf_stf,		/* absf  and stf parallel, (simultaneously)       */
    op_absi_sti,		/* absi  and sti parallel, (simultaneously)       */
    op_addf3_stf,		/* addf3 and stf parallel, (simultaneously)       */
    op_addi3_sti,		/* addi3 and sti parallel, (simultaneously)       */
    op_and3_sti,		/* and3  and sti parallel, (simultaneously)       */
    op_ash3_sti,		/* ash3  and sti parallel, (simultaneously)       */
    op_fix_sti,			/* fix   and sti parallel, (simultaneously)       */
    op_float_stf,		/* float and stf parallel, (simultaneously)       */
    op_ldf_stf,			/* ldf   and stf parallel, (simultaneously)       */
    op_ldi_sti,			/* ldi   and sti parallel, (simultaneously)       */
    op_lsh3_sti,		/* lsh3  and sti parallel, (simultaneously)       */
    op_mpyf3_stf,		/* mpyf3 and stf parallel, (simultaneously)       */
    op_mpyi3_sti,		/* mpyi3 and sti parallel, (simultaneously)       */
    op_negf_stf,		/* negf  and stf parallel, (simultaneously)       */
    op_negi_sti,		/* negi  and sti parallel, (simultaneously)       */
    op_not_sti,			/* not   and sti parallel, (simultaneously)       */
    op_or3_sti,			/* or3   and sti parallel, (simultaneously)       */
    op_subf3_stf,		/* subf3 and stf parallel, (simultaneously)       */
    op_subi3_sti,		/* subi3 and sti parallel, (simultaneously)       */
    op_xor3_sti,		/* xor3  and sti parallel, (simultaneously)       */

    op_divs,			/* pseudo opcodes */
    op_divu,			/* pseudo opcodes */

    op_line,
#ifdef ASM
    op_asm,
#endif				/* ASM */
    op_label,
    op_abort			/* pseudo opcode for peep as marker in list */
};

#define OP_MIN   op_absf
#define OP_MAX   op_abort
#ifdef USE_ALL_OPCODES
#define OP_PARALLEL_FIRST op_mpyf3_addf3
#else
#define OP_PARALLEL_FIRST op_stf_stf
#endif
#define OP_PARALLEL_LAST  op_xor3_sti

/* addressing modes */
enum e_am
{
    am_dreg,			/* Rn        */
    am_areg,			/* ARn       */
    am_ireg,			/* IRn       */
    am_freg,			/* Rn float  */
    am_sreg,			/* specialregs DP, BK, SP, RS, RE, RC  */
    am_ind,			/* *ARn      */
    am_const_ind,		/* *ARn,  compilerconstant, can never be accessed 
				 *        by pointer or be written to by user   */
    am_ainc,			/* *ARn++    */
    am_adec,			/* *ARn--    */
    am_preinc,			/* *++ARn    */
    am_predec,			/* *--ARn    */
    am_indx,			/* *+ARn(d8) */
    am_indx2,			/* *+ARn(IRn) */
    am_indxs,			/* *-ARn(IRn) */
    am_direct,			/* @xxxx     */
    am_const_direct,		/* @xxxx, compilerconstant, can never be accessed 
				 *        by pointer or be written to by user   */
    am_immed,			/* #data     */
    am_none,
    am_str,			/* string */
    am_line			/* pseudo mode for line numbers */
};

#define MAX_POS_SHORT_FLOAT    2.5594E2
#define MIN_POS_SHORT_FLOAT    7.8125E-3
#define MAX_NEG_SHORT_FLOAT   -7.8163E-3
#define MIN_NEG_SHORT_FLOAT   -2.5600E2

#define MAX_SINGLE_FLOAT   3.4028234E38
#define MIN_SINGLE_FLOAT  -3.4028236E38


/* register naming, special registers */

#define REG_R0  ((REG) 0)
#define REG_R1  ((REG) 1)
#define REG_R2  ((REG) 2)
#define REG_R3  ((REG) 3)
#define REG_R4  ((REG) 4)
#define REG_R5  ((REG) 5)
#define REG_R6  ((REG) 6)
#define REG_R7  ((REG) 7)

#define REG_AR0 ((REG) 8)
#define REG_AR1 ((REG) 9)
#define REG_AR2 ((REG) 10)
#define REG_AR3 ((REG) 11)
#define REG_AR4 ((REG) 12)
#define REG_AR5 ((REG) 13)
#define REG_AR6 ((REG) 14)
#define REG_AR7 ((REG) 15)

#define REG_DP  ((REG) 16)
#define REG_IR0 ((REG) 17)
#define REG_IR1 ((REG) 18)
#define REG_BK  ((REG) 19)
#define REG_SP  ((REG) 20)

#define REG_ST  ((REG) 21)
#define REG_IE  ((REG) 22)
#define REG_IF  ((REG) 23)
#define REG_IOF ((REG) 24)
#define REG_RS  ((REG) 25)
#define REG_RE  ((REG) 26)
#define REG_RC  ((REG) 27)

#define REG_MEMORY ((REG) 31)	/* special flag for peephole optimizer  */

#define	NUM_REGS	28	/* number of distinct registers */
#define MAX_REG   REG_RC
#define FLOAT_REG 0X40		/* Flag for Floatregisteruse of R0-R7  */

#define RESULT    REG_R0	/* register returning function results */
#define FRAMEPTR  REG_AR7	/* frame pointer register */
#define STACKPTR  REG_SP	/* system stack pointer register */

#define MAX_ADDR  REG_AR1	/* max. scratch address register (AR1) */
#define MAX_DATA  REG_R2	/* max. scratch data    register (R2) */

#define ALL_TEMPORARYS ( (1UL<<REG_R0)|(1UL<<REG_R1)|(1UL<<REG_R2)\
                        |(1UL<<REG_AR0)|(1UL<<REG_AR1) \
                        |(1UL<<REG_IR0)|(1UL<<REG_IR1))

#define MASK_REG_DATA    (0xFFUL << REG_R0)
#define MASK_REG_ADDRESS (0xFFUL << REG_AR0)
#define MASK_REG_ALL     (0x0FFFFFFFUL)

#define MAX_REG_STACK   ((DEEP)30)
#define UNUSED          ((DEEP)-1)
#define EMPTY           ((DEEP)0)

/*
   **   Defines what the register can be used for
 */
#define	D_REG		((REGTYPE)1)	/* data register      */
#define	A_REG		((REGTYPE)2)	/* address register   */
#define	F_REG		((REGTYPE)4)	/* float register     */
#define	I_REG		((REGTYPE)8)	/* index register     */
#define	S_REG		((REGTYPE)16)	/* specialregister    */
#define	T_REG		((REGTYPE)32)	/* temporary register */
#define DFAIS_REG       ((REGTYPE)(A_REG | D_REG | F_REG | I_REG | S_REG))
#define	is_data_register(r)		 ((regtype[(int)r] & D_REG) != 0)
#define	is_address_register(r)		 ((regtype[(int)r] & A_REG) != 0)
#define	is_index_register(r)		 ((regtype[(int)r] & I_REG) != 0)
#define	is_float_register(r)		 ((regtype[(int)r] & F_REG) != 0)
#define	is_temporary_register(r)	 ((regtype[(int)r] & T_REG) != 0)
#define	is_temporary_data_register(r)	 ((regtype[(int)r] & (D_REG|T_REG)) == (D_REG|T_REG))
#define	is_temporary_address_register(r) ((regtype[(int)r] & (A_REG|T_REG)) == (A_REG|T_REG))
#define	is_temporary_index_register(r)	 ((regtype[(int)r] & (I_REG|T_REG)) == (I_REG|T_REG))
#define	is_temporary_float_register(r)	 ((regtype[(int)r] & (F_REG|T_REG)) == (F_REG|T_REG))

#define	is_address_or_index_register(r)	 ((regtype[(int)r] & (A_REG | I_REG)) != 0)
#define	is_same_register_type(r1,r2)	 ((regtype[(int)r1] & regtype[(int)r2] & (DFAIS_REG)) != 0)
typedef unsigned char REGTYPE;


/* support routines */
#define SUP_FPDIV       (const CHAR *)".Xfpdiv"
#define SUP_FPREM       (const CHAR *)".Xfprem"
#define SUP_LDIV        (const CHAR *)".Xldiv"
#define SUP_LREM        (const CHAR *)".Xlrem"
#define SUP_ULDIV       (const CHAR *)".Xuldiv"
#define SUP_ULREM       (const CHAR *)".Xulrem"

#ifdef STACK_CHECK
#define SUP_STACKCHECK  (const CHAR *)".stackcheck"
#endif /* STACK_CHECK */

#ifndef _CODE_DEFINED
#define _CODE_DEFINED
typedef struct ocode CODE;

#endif

/* instruction lengths */

enum itype
{
    OP_NOTYPE = 0,
    OP_INT = 1,
    OP_FLOAT = 2
};

typedef enum e_am AMODE;	/* Addressing mode */
typedef struct amode ADDRESS;
typedef enum e_op OPCODE;
typedef enum itype ITYPE;
typedef unsigned long REGMASK;
typedef signed char DEEP;
typedef struct reglist REGLIST;
typedef unsigned int PEEPFLAGS;
typedef unsigned long REGBITMAP;

typedef struct regmap PEEPINFO;
typedef struct flowlist FLOWLISTENTRY;

struct regmap
{
    REGBITMAP write;		/* destination of an operation e.g. ldi ...,X  */
    REGBITMAP modified;		/* source and destination o. a. o.  addi ...,X */
    REGBITMAP read;		/* source of an operation :         ldi X,...  */
    REGBITMAP used;		/* needed for addressgeneration:    *arn,      */
    REGBITMAP updated;		/* updated during addressgeneration *arn++     */
#ifdef REGISTER_FLOW_ANALYZER
    REGBITMAP used_later;	/* For regusage flow analyzer, contains registers */
    /* wich will be used later in codesequence       */
    REGBITMAP fixed;		/* Marks registers which cannot be reassigned    */
#endif
};

struct flowlist
{
    ADDRESS *ap;
#ifdef NO_MIX_WITH_FLOAT
    ITYPE   type;
#endif
    FLOWLISTENTRY *next;
};

#define NIL_ADDRESS     ((ADDRESS *) 0)
#define NIL_CODE        ((CODE *) 0)


/* addressing mode structure */

struct amode
{
    AMODE   mode;		/* addressing mode */
    REG     preg, sreg;		/* register(s) used in addressing mode */
    DEEP    deep;
    union
    {
	EXPR   *offset;		/* expression used in addressing mode */
	DEEP    sdeep;		/* deep for secondary register */
    } u;
};

/* output code structure */

struct ocode
{
    OPCODE  opcode;		/* opcode for this instruction       */
    ITYPE   type;		/* float or integerinstruction       */
    ADDRESS *src1;		/* first operand                     */
    ADDRESS *src2;		/* second operand                    */
    ADDRESS *dst;		/* third operand                     */
    /* operands for parallel instructions */
    ADDRESS *src21;		/* first  operand of 2nd instruction */
    ADDRESS *src22;		/* second operand of 2nd instruction */
    ADDRESS *dst2;		/* third  operand of 2nd instruction */
#ifndef SAVE_MEMORY
    PEEPINFO *info;		/* information for peephole optimizer */
#endif
    PEEPFLAGS flags;		/* flags for peephole optimizer      */
    CODE   *fwd;		/* next instruction                  */
    CODE   *back;		/* previous instruction              */
};

struct reglist
{
    int     number;		/* number of registers in the list */
    REG    *reg;		/* register list */
};

/*
 *      The usage of registers is controlled by the information held
 *      within the following structure.
 */
struct reg_use
{
    REGLIST *parameter;		/* Registers used to pass parameters */
    REGLIST *save;		/* Registers saved by the function */
    REGLIST *result;		/* Registers used to return results */
};

#define BRANCH_COUNT    2	/* abandon branch optimistaion if exceeded */


/*
 *    defines for the peephole opimiser
 */
#ifndef DEBUG
typedef unsigned int INSTR_INFO;

#else
typedef unsigned long INSTR_INFO;

#endif

#define DEST_MODIFY        ((INSTR_INFO)0x0001)
#define DEST_OVERWRITE     ((INSTR_INFO)0x0002)
#define SET_FLAGS          ((INSTR_INFO)0x0004)
#define USE_FLAGS          ((INSTR_INFO)0x0008)
#define SET_FLAGS_FOR_ALL  ((INSTR_INFO)0x0010)
#define COMMUTATIVE_INSTR  ((INSTR_INFO)0x0020)
#define PARALLEL_INSTR     ((INSTR_INFO)0x0040)
#define HAS_OP3            ((INSTR_INFO)0x0080)
#define OP_BRANCH          ((INSTR_INFO)0x0100)
#define OP_JUMP            ((INSTR_INFO)0x0200)
#define OP_RETURN          ((INSTR_INFO)0x0400)
#define OP_LABEL           ((INSTR_INFO)0x0800)
#define OP_CALL            ((INSTR_INFO)0x1000)
#define OP_ASM             ((INSTR_INFO)0x2000)
#define USES_SP            ((INSTR_INFO)0x4000)
#define USES_CARRY_FLAG    ((INSTR_INFO)0x8000)

#ifdef DEBUG
#define FLOAT_INSTR       ((INSTR_INFO)0x10000)
#else
#define FLOAT_INSTR       ((INSTR_INFO)0)
#endif

#define DEST_ALTERED       (DEST_MODIFY | DEST_OVERWRITE)
#define PAR_DEST_OVERWRITE (PARALLEL_INSTR | DEST_OVERWRITE)

#define is_set_flags(ip)              ((op_flags[(ip)->opcode]&SET_FLAGS)!=0)
#define is_set_all_flags(opcode)      ((op_flags[opcode]&SET_FLAGS_FOR_ALL)!=0)
#define is_dest_overwritten(opcode)   ((op_flags[opcode]&DEST_OVERWRITE)!=0)
#define is_parallel_opcode(opcode)    ((op_flags[opcode]&PARALLEL_INSTR)!=0)
#define is_commutative_opcode(opcode) ((op_flags[opcode]&COMMUTATIVE_INSTR)!=0)
#define is_op_op3_availlable(opcode)  ((op_flags[opcode]&HAS_OP3)!=0)
#define is_controltransfer(opcode)    ((op_flags[opcode]&TRANSFER)!=0)
#define is_pipelinegroup2_used(ip)    ((op_flags[(ip)->opcode]&USES_SP)!=0)
#define is_using_sp(opcode)           ((op_flags[opcode]&USES_SP)!=0)
#define is_using_carry(opcode)        ((op_flags[opcode]&USES_CARRY_FLAG)!=0)
#define is_using_flags(opcode)        ((op_flags[opcode]&USE_FLAGS)!=0)

#ifdef DEBUG
#define is_float_instruction(opcode)  ((op_flags[opcode]&FLOAT_INSTR)!=0)
#endif

#ifdef SAVE_PEEP_MEMORY
#define GET_PEEP_INFO(ip, PeepMap)	build_register_map(ip,PeepMap)
#define Update_Peep_Info(ip)
#define attach_peepinfo(ip)
#else /* SAVE_PEEP_MEMORY */
#define GET_PEEP_INFO(ip, Dummy)	(ip)->info
#define Update_Peep_Info(ip)		update_peepinfo_fkt(ip)
#define attach_peepinfo(ip)		attach_peepinfo_fkt(ip)
#endif /* SAVE_PEEP_MEMORY */

#define is_am_register(ap) (  (ap->mode == am_areg)||(ap->mode == am_ireg)\
                            ||(ap->mode == am_dreg)||(ap->mode == am_freg)\
                            ||(ap->mode == am_sreg))

#ifdef MULTIPLE_PROCESSORS
/*
 * remap function names - it is necessary to change the names of functions
 * if there are multiple code generators build into the compiler in order
 * to prevent name clashes.
 *
 * The following defines do the necessary renaming
 */
#define address_register        address_registerC30
#define	build_register_map	build_register_mapC30
#define checkstack              checkstackC30
#define copy_addr               copy_addrC30
#define data_register           data_registerC30
#define	find_label		find_labelC30
#define float_register          float_registerC30
#define flush_peep              flush_peepC30
#define freeop                  freeopC30
#define g_code                  g_codeC30
#define g_code_parallel         g_code_parallelC30
#define get_register            get_registerC30
#define index_register          index_registerC30
#define initstack               initstackC30
#define is_free_data            is_free_dataC30
#define is_free_addr            is_free_addrC30
#define is_free_ireg            is_free_iregC30
#define	is_register_used	is_register_usedC30
#define mk_mreg                 mk_mregC30
#define mk_reg                  mk_regC30
#define	op_flags		op_flagsC30
#define putamode_tst		putamode_tstC30
#define putreg_tst		putreg_tstC30
#define reg_usage               reg_usageC30
#define regtype                 regtypeC30
#define temp_inv                temp_invC30
#define temporary_register      temporary_registerC30
#define	update_peepinfo_fkt	update_peepinfo_fktC30
#define validate                validateC30

#endif /* MULTIPLE_PROCESSORS */

extern REGUSAGE *reg_usage;	/* register usage */

/* Constants for Optionsenumeration */
#define OPT_YES     1
#define OPT_NO      0
#define OPT_NONE    0
#define OPT_LEVEL1  1
#define OPT_LEVEL2  2
#define OPT_LEVEL3  3
#define OPT_LEVEL4  4
#define OPT_LEVEL5  5
#define OPT_DO_ALL  99
#define OPT_DEBUG   100

/* stack optimisations */
#define	OPT_SAFE	0
#define	OPT_MINIMUM	1
#define	OPT_AVERAGE	2
#define	OPT_MAXIMUM	3

/* peehole optimisations */
#define	PEEP_NONE	0
#define	PEEP_FLOW	0
#define	PEEP_PIPELINE	1
#define	PEEP_3OPERAND	2
#define	PEEP_PARALLEL	3
#define	PEEP_PARALLEL_ALL 4
#define	PEEP_REMAP	5
#define	PEEP_MINIMAL    6	/* dummy to enable minimalst optimisations */
#define	PEEP_REDUNDANT  7
#define	PEEP_STORE      8
#define	PEEP_SWITCH     9
#define	PEEP_HARD_REMAP 10
#define	PEEP_VERY_HARD_REMAP 11
#define	PEEP_ALL	((unsigned int)(0xFFFF))
#define	PEEP_STANDARD  (MEMBER (PEEP_FLOW)     | \
                        MEMBER (PEEP_PARALLEL) | \
                        MEMBER (PEEP_REMAP)    | \
                        MEMBER (PEEP_REDUNDANT))
#define	is_peep_phase(l,x)	(((unsigned) l) & MEMBER(x))
#define	is_switch_peep_enabled() (opt_peep_test > 9)

extern REGTYPE regtype[];	/* type of register */
extern INSTR_INFO op_flags[];
extern int opt_delayed_branches;
extern const CHAR *opt_peep_sequence;
extern int opt_branches;
extern int opt_peep_test;


/* flowc30 */
int flow_dataflow P_ ((CODE *, int));

/* genc30 */
extern REG frameptr;

ADDRESS *mk_reg P_ ((REG));
ADDRESS *mk_freg P_ ((REG));
ADDRESS *copy_addr P_ ((ADDRESS *, AMODE));
ADDRESS *mk_label P_ ((LABEL));

#ifdef FLOAT_SUPPORT
BOOL is_short_float P_ ((const RVAL, BTYPE));

#endif /* FLOAT_SUPPORT */

/* outc30 */
#ifdef DEBUG
void putamode_tst P_ ((const ADDRESS *));
void putreg_tst P_ ((REG));

#endif /* DEBUG */

/* peepc30 */
BOOL is_equal_address P_ ((const ADDRESS *, const ADDRESS *));
BOOL is_register_used P_ ((REG, const ADDRESS *));
CODE   *find_label P_ ((LABEL));
void build_register_map P_ ((const CODE *, PEEPINFO *));
void g_code P_ ((OPCODE, ITYPE, ADDRESS *, ADDRESS *));
void g_code3 P_ ((OPCODE, ITYPE, ADDRESS *, ADDRESS *, ADDRESS *));
void    g_code_parallel
P_ (
    (OPCODE, ITYPE, ADDRESS *, ADDRESS *, ADDRESS *, ADDRESS *, ADDRESS *,

     ADDRESS *));
void flush_peep P_ ((unsigned));
void init_peep P_ ((void));
void update_peepinfo_fkt P_ ((CODE *));

#ifdef VERBOSE
void c30_peep_report P0 (void);

#endif

/* regc30 */
ADDRESS *address_register P_ ((void));
ADDRESS *data_register P_ ((FLAGS));
ADDRESS *index_register P_ ((void));
ADDRESS *temporary_register P_ ((FLAGS));
BOOL is_free_addr P_ ((void));
BOOL is_free_data P_ ((void));
BOOL is_free_ireg P_ ((void));
void checkstack P_ ((void));
void freeop P_ ((const ADDRESS *));
void initstack P_ ((void));
void temp_inv P_ ((void));
void validate P_ ((const ADDRESS *));

#endif /* _GENC30_H */
