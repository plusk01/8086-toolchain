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
 *
 * 1995   Ivo Oesch, Started to build a codegenerator for the
 *        signalprocessor TMS320C30 (December)
 */

/*****************************************************************************/

#include "config.h"

#ifdef TMS320C30
#include "chdr.h"
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"
#include "genc30.h"
#include "outproto.h"

/********************************************************* Macro Definitions */

#ifdef DEBUG
/* if defined a consistencycheck of the whole peeplist 
 * is performed everytime the list is modified
 * ATTENTION, uses a lot of runtime if enabled 
 */
/*#define CHECK_LIST_INTEGRITY */
#endif /* DEBUG */

#define PEEP_CALL_ALSO_VIA_TABLE
#define MAX_PEEP_PASSES 100
#define PEEP_REDUNDANT_STI


#define PEEP_AREG       ((unsigned int)0x0001)
#define PEEP_IREG       ((unsigned int)0x0002)
#define PEEP_AINC       ((unsigned int)0x0004)
#define PEEP_IND        ((unsigned int)0x0008)
#define PEEP_INDEX      ((unsigned int)0x0010)
#define PEEP_DIRECT     ((unsigned int)0x0020)
#define PEEP_SP         ((unsigned int)0x0040)
#define PEEP_DST_AREG   ((unsigned int)0x0080)
#define PEEP_DST_IREG   ((unsigned int)0x0100)
#define PEEP_DST_AINC   ((unsigned int)0x0200)
#define PEEP_DST_IND    ((unsigned int)0x0400)
#define PEEP_DST_INDEX  ((unsigned int)0x0800)
#define PEEP_DST_DIRECT ((unsigned int)0x1000)
#define PEEP_DST_SP     ((unsigned int)0x2000)
#define PEEP_TRANSFER   ((unsigned int)0x4000)
#define PEEP_COMPARE    ((unsigned int)0x8000)

/* Group 1 = AR0..AR7, IR0, IR1, BK */
#define GROUP_1_REGISTERS ((REGBITMAP)((0xFFUL<<REG_AR0)|(0x03UL<<REG_IR0)|(1UL<<REG_BK)))
#define GROUP_1_WRITE     ((REGBITMAP)((0xFFUL<<REG_AR0)|(0x03UL<<REG_IR0)|(1UL<<REG_BK)))
#define GROUP_1_READ      ((REGBITMAP)(0xFFUL<<REG_AR0))

#define GROUP_3_REGISTERS ((REGBITMAP)(1UL<<REG_SP))
#define GROUP_3_READ      ((REGBITMAP)(1UL<<REG_SP))
#define GROUP_3_WRITE     ((REGBITMAP)(1UL<<REG_SP))

#define PEEP_STOP       (PEEP_TRANSFER | PEEP_COMPARE)
#define PEEP_USE_GROUP1 (PEEP_AINC | PEEP_IND | PEEP_INDEX | PEEP_DST_AINC | PEEP_DST_IND | PEEP_DST_INDEX)
#define PEEP_DELAY_1CYCLE_GROUP1 (PEEP_AREG)
#define PEEP_DELAY_2CYCLE_GROUP1 (PEEP_DST_AREG | PEEP_DST_IREG)
#define PEEP_DELAY_GROUP1        (PEEP_DST_AREG | PEEP_DST_IREG | PEEP_AREG)
#define PEEP_GROUP1 (PEEP_DELAY_2CYCLE_GROUP1 | PEEP_DELAY_1CYCLE_GROUP1 | PEEP_USE_GROUP1 | PEEP_IREG)

#define is_pipelinesearch_stop(ip)     ((((ip)->flags)&(PEEP_STOP))!=0)
#define is_pipelinegroup1(ip)          ((((ip)->flags)&(PEEP_GROUP1))!=0)
#define is_pipelinegroup1_used(ip)     ((((ip)->flags)&(PEEP_USE_GROUP1))!=0)
#define is_pipelinegroup1_delayed1(ip) ((((ip)->flags)&(PEEP_DELAY_1CYCLE_GROUP1))!=0)
#define is_pipelinegroup1_delayed2(ip) ((((ip)->flags)&(PEEP_DELAY_2CYCLE_GROUP1))!=0)
#define is_pipelinegroup1_delayed(ip)  ((((ip)->flags)&(PEEP_DELAY_GROUP1))!=0)

#define TRANSFER           (OP_BRANCH | OP_JUMP | OP_RETURN | OP_LABEL | OP_CALL | OP_ASM)

#if 1
#define IncPeepStatistic(Item) (PeepStatistic.Item++)
#define AddPeepStatistic(Item,Offset) (PeepStatistic.Item += (Offset))
#else
#define IncPeepStatistic(Item)
#define AddPeepStatistic(Item,Offset)
#endif

#define STANDARD    1
#define COMBINE_FWD 2
#define COMBINE_BWD 4
#define COMMUTATIVE 8
#define REMAPP      16
#define REDUNDANT   32
#define DATAFLOW    64
#define TO_3OPERAND 128
#define ENABLE_ALL  255

#define is_peep_enabled(WHAT) ((SelectedOptimizer&(WHAT))!=0)
#define FIRST_PEEP_STAGE() {CurrentStage = 0; \
			    SelectedOptimizer = OptimizerStages[0];}
#define NEXT_PEEP_STAGE()  (SelectedOptimizer = OptimizerStages[++CurrentStage])

/*
 *      The next couple of #define statements are to make the
 *      code in the branch optimisation clearer.  Tests
 *      have shown that the in-line size cost is about
 *      the same as making them functions so we keep
 *      them in-line for speed.
 */

/* backup over any sequence of labels or linestatements to previous instruction */
#define previous_real_instruction(ip) \
    do { \
        ip = ip->back; \
    } while (ip != NIL_CODE && (ip->opcode == op_label || ip->opcode == op_line))

/* backup over any sequence of lines to previous instruction */
#define previous_ignore_line(ip) \
    do { \
        ip = ip->back; \
    } while (ip != NIL_CODE && ip->opcode == op_line)


#define branch(ip)      (  (ip->opcode >= op_bu && ip->opcode <= op_bnn)\
                         ||(ip->opcode == op_br))

#define anybranch(ip)   (  (ip->opcode >= op_bu && ip->opcode <= op_bnn)\
                         ||(ip->opcode == op_br) \
                         ||(ip->opcode >= op_bud && ip->opcode <= op_bnnd)\
                         ||(ip->opcode == op_brd))

#define is_same_instruction(ip1,ip2) \
    ((ip1 != NIL_CODE) && (ip2 != NIL_CODE) && \
    (ip1->opcode == ip2->opcode) && \
    (ip1->type == ip2->type) && \
    (is_equal_oper (ip1->src1, ip2->src1)) && \
    (is_equal_oper (ip1->src2, ip2->src2)) && \
    (is_equal_oper (ip1->dst,  ip2->dst)) && \
    (is_equal_oper (ip1->src21,ip2->src21)) && \
    (is_equal_oper (ip1->src22,ip2->src22)) && \
    (is_equal_oper (ip1->dst2, ip2->dst2)))
/********************************************************** typedefs */

typedef unsigned char PEEP_STAGES;

/********************************************************** Static Variables */

INSTR_INFO op_flags[] = {
    /* op_absf     */ DEST_OVERWRITE | SET_FLAGS | FLOAT_INSTR,
    /* op_absi     */ DEST_OVERWRITE | SET_FLAGS,
#ifdef USE_ALL_OPCODES
    /* op_addc     */ DEST_MODIFY | SET_FLAGS | COMMUTATIVE_INSTR | HAS_OP3,
#endif				/* USE_ALL_OPCODES */
    /* op_addf     */
    DEST_MODIFY | SET_FLAGS | COMMUTATIVE_INSTR | HAS_OP3 | FLOAT_INSTR,
    /* op_addi     */ DEST_MODIFY | SET_FLAGS | COMMUTATIVE_INSTR | HAS_OP3,
    /* op_and      */ DEST_MODIFY | SET_FLAGS | COMMUTATIVE_INSTR | HAS_OP3,
    /* op_andn     */ DEST_MODIFY | SET_FLAGS | HAS_OP3,
    /* op_ash      */ DEST_MODIFY | SET_FLAGS | HAS_OP3,
    /* op_cmpf     */ SET_FLAGS_FOR_ALL | SET_FLAGS | FLOAT_INSTR,
    /* op_cmpi     */ SET_FLAGS_FOR_ALL | SET_FLAGS,
    /* op_fix      */ DEST_OVERWRITE | SET_FLAGS | FLOAT_INSTR,
    /* op_float    */ DEST_OVERWRITE | SET_FLAGS,
#ifdef USE_ALL_OPCODES
    /* op_idle     */ 0,
    /* op_lde      */ DEST_MODIFY,
#endif				/* USE_ALL_OPCODES */
    /* op_ldf      */ DEST_OVERWRITE | SET_FLAGS | FLOAT_INSTR,
    /* op_popldf   */ DEST_OVERWRITE | SET_FLAGS | FLOAT_INSTR,
#ifdef USE_ALL_OPCODES
    /* op_ldfi     */ DEST_OVERWRITE | SET_FLAGS | FLOAT_INSTR,
#endif				/* USE_ALL_OPCODES */
    /* op_ldi      */ DEST_OVERWRITE | SET_FLAGS,
    /* op_popldi   */ DEST_OVERWRITE | SET_FLAGS,
#ifdef USE_ALL_OPCODES
    /* op_ldii     */ DEST_OVERWRITE | SET_FLAGS,
    /* op_ldm      */ DEST_MODIFY,
#endif				/* USE_ALL_OPCODES */
    /* op_lsh      */ DEST_MODIFY | SET_FLAGS | HAS_OP3,
    /* op_mpyf     */
    DEST_MODIFY | SET_FLAGS | COMMUTATIVE_INSTR | HAS_OP3 | FLOAT_INSTR,
    /* op_mpyi     */ DEST_MODIFY | SET_FLAGS | COMMUTATIVE_INSTR | HAS_OP3,
#ifdef USE_ALL_OPCODES
    /* op_negb     */ DEST_OVERWRITE | SET_FLAGS,
#endif				/* USE_ALL_OPCODES */
    /* op_negf     */ DEST_OVERWRITE | SET_FLAGS | FLOAT_INSTR,
    /* op_negi     */ DEST_OVERWRITE | SET_FLAGS,
    /* op_nop      */ 0,
#ifdef USE_ALL_OPCODES
    /* op_norm     */ DEST_OVERWRITE | SET_FLAGS, |FLOAT_INSTR
#endif				/* USE_ALL_OPCODES */
	/* op_not      */ DEST_OVERWRITE | SET_FLAGS,
    /* op_pop      */ DEST_OVERWRITE | SET_FLAGS | USES_SP,
    /* op_popf     */ DEST_OVERWRITE | SET_FLAGS | USES_SP | FLOAT_INSTR,
    /* op_push     */ USES_SP,
    /* op_pushf    */ USES_SP | FLOAT_INSTR,
    /* op_pushnopeep     */ USES_SP,
    /* op_pushfnopeep    */ USES_SP | FLOAT_INSTR,
    /* op_or       */ DEST_MODIFY | SET_FLAGS | COMMUTATIVE_INSTR | HAS_OP3,
#ifdef USE_ALL_OPCODES
    /* op_rnd      */ DEST_OVERWRITE | SET_FLAGS | FLOAT_INSTR,
    /* op_rol      */ DEST_MODIFY | SET_FLAGS,
    /* op_rolc     */ DEST_MODIFY | SET_FLAGS,
    /* op_ror      */ DEST_MODIFY | SET_FLAGS,
    /* op_rorc     */ DEST_MODIFY | SET_FLAGS,
#endif				/* USE_ALL_OPCODES */
    /* op_rpts     */ OP_BRANCH,
    /* op_stf      */ DEST_OVERWRITE | FLOAT_INSTR,
#ifdef USE_ALL_OPCODES
    /* op_stfi     */ DEST_OVERWRITE | FLOAT_INSTR,
#endif				/* USE_ALL_OPCODES */
    /* op_sti      */ DEST_OVERWRITE,
#ifdef USE_ALL_OPCODES
    /* op_stii     */ DEST_OVERWRITE,
    /* op_sigi     */ 0,
    /* op_subb     */ DEST_MODIFY | SET_FLAGS | HAS_OP3,
    /* op_subc     */ DEST_MODIFY | SET_FLAGS,
#endif				/* USE_ALL_OPCODES */
    /* op_subf     */
    DEST_MODIFY | SET_FLAGS | COMMUTATIVE_INSTR | HAS_OP3 | FLOAT_INSTR,
    /* op_subi     */ DEST_MODIFY | SET_FLAGS | COMMUTATIVE_INSTR | HAS_OP3,
#ifdef USE_ALL_OPCODES
    /* op_subrb    */ DEST_MODIFY | SET_FLAGS,
#endif				/* USE_ALL_OPCODES */
    /* op_subrf    */
    DEST_MODIFY | SET_FLAGS | COMMUTATIVE_INSTR | FLOAT_INSTR,
    /* op_subri    */ DEST_MODIFY | SET_FLAGS | COMMUTATIVE_INSTR,
    /* op_tstb     */ SET_FLAGS_FOR_ALL | SET_FLAGS,
    /* op_xor      */ DEST_MODIFY | SET_FLAGS | COMMUTATIVE_INSTR | HAS_OP3,
#ifdef USE_ALL_OPCODES
    /* op_iack     */ 0,
    /* op_addc3    */ DEST_OVERWRITE | SET_FLAGS,
#endif				/* USE_ALL_OPCODES */
    /* op_addf3    */ DEST_OVERWRITE | SET_FLAGS | FLOAT_INSTR,
    /* op_addi3    */ DEST_OVERWRITE | SET_FLAGS,
    /* op_and3     */ DEST_OVERWRITE | SET_FLAGS,
    /* op_andn3    */ DEST_OVERWRITE | SET_FLAGS,
    /* op_ash3     */ DEST_OVERWRITE | SET_FLAGS,
    /* op_cmpf3    */ SET_FLAGS_FOR_ALL | SET_FLAGS | FLOAT_INSTR,
    /* op_cmpi3    */ SET_FLAGS_FOR_ALL | SET_FLAGS,
    /* op_lsh3     */ DEST_OVERWRITE | SET_FLAGS,
    /* op_mpyf3    */ DEST_OVERWRITE | SET_FLAGS | FLOAT_INSTR,
    /* op_mpyi3    */ DEST_OVERWRITE | SET_FLAGS,
    /* op_or3      */ DEST_OVERWRITE | SET_FLAGS,
#ifdef USE_ALL_OPCODES
    /* op_subb3    */ DEST_OVERWRITE | SET_FLAGS,
#endif				/* USE_ALL_OPCODES */
    /* op_subf3    */ DEST_OVERWRITE | SET_FLAGS | FLOAT_INSTR,
    /* op_subi3    */ DEST_OVERWRITE | SET_FLAGS,
    /* op_tstb3    */ SET_FLAGS_FOR_ALL | SET_FLAGS,
    /* op_xor3     */ DEST_OVERWRITE | SET_FLAGS,
    /* op_ldfu     */ DEST_OVERWRITE | FLOAT_INSTR,
    /* op_ldflo    */ DEST_MODIFY | USE_FLAGS | USES_CARRY_FLAG | FLOAT_INSTR,
    /* op_ldfls    */ DEST_MODIFY | USE_FLAGS | USES_CARRY_FLAG | FLOAT_INSTR,
    /* op_ldfhi    */ DEST_MODIFY | USE_FLAGS | USES_CARRY_FLAG | FLOAT_INSTR,
    /* op_ldfhs    */ DEST_MODIFY | USE_FLAGS | USES_CARRY_FLAG | FLOAT_INSTR,
    /* op_ldfeq    */ DEST_MODIFY | USE_FLAGS | FLOAT_INSTR,
    /* op_ldfne    */ DEST_MODIFY | USE_FLAGS | FLOAT_INSTR,
    /* op_ldflt    */ DEST_MODIFY | USE_FLAGS | FLOAT_INSTR,
    /* op_ldfle    */ DEST_MODIFY | USE_FLAGS | FLOAT_INSTR,
    /* op_ldfgt    */ DEST_MODIFY | USE_FLAGS | FLOAT_INSTR,
    /* op_ldfge    */ DEST_MODIFY | USE_FLAGS | FLOAT_INSTR,
    /* op_ldfz     */ DEST_MODIFY | USE_FLAGS | FLOAT_INSTR,
    /* op_ldfnz    */ DEST_MODIFY | USE_FLAGS | FLOAT_INSTR,
    /* op_ldfp     */ DEST_MODIFY | USE_FLAGS | FLOAT_INSTR,
    /* op_ldfn     */ DEST_MODIFY | USE_FLAGS | FLOAT_INSTR,
    /* op_ldfnn    */ DEST_MODIFY | USE_FLAGS | FLOAT_INSTR,
    /* op_ldiu     */ DEST_OVERWRITE,
    /* op_ldilo    */ DEST_MODIFY | USE_FLAGS | USES_CARRY_FLAG,
    /* op_ldils    */ DEST_MODIFY | USE_FLAGS | USES_CARRY_FLAG,
    /* op_ldihi    */ DEST_MODIFY | USE_FLAGS | USES_CARRY_FLAG,
    /* op_ldihs    */ DEST_MODIFY | USE_FLAGS | USES_CARRY_FLAG,
    /* op_ldieq    */ DEST_MODIFY | USE_FLAGS,
    /* op_ldine    */ DEST_MODIFY | USE_FLAGS,
    /* op_ldilt    */ DEST_MODIFY | USE_FLAGS,
    /* op_ldile    */ DEST_MODIFY | USE_FLAGS,
    /* op_ldigt    */ DEST_MODIFY | USE_FLAGS,
    /* op_ldige    */ DEST_MODIFY | USE_FLAGS,
    /* op_ldiz     */ DEST_MODIFY | USE_FLAGS,
    /* op_ldinz    */ DEST_MODIFY | USE_FLAGS,
    /* op_ldip     */ DEST_MODIFY | USE_FLAGS,
    /* op_ldin     */ DEST_MODIFY | USE_FLAGS,
    /* op_ldinn    */ DEST_MODIFY | USE_FLAGS,
    /* op_br       */ OP_JUMP,
    /* op_brd      */ OP_JUMP,
    /* op_call     */ OP_CALL | USES_SP,
    /* op_xcall    */ OP_CALL | USES_SP,
    /* op_rptb     */ OP_BRANCH,
#ifdef USE_ALL_OPCODES
    /* op_swi      */ OP_CALL | USES_SP,
#endif				/* USE_ALL_OPCODES */
    /* op_bu       */ OP_JUMP,
    /* op_blo      */ OP_BRANCH | USE_FLAGS | USES_CARRY_FLAG,
    /* op_bls      */ OP_BRANCH | USE_FLAGS | USES_CARRY_FLAG,
    /* op_bhi      */ OP_BRANCH | USE_FLAGS | USES_CARRY_FLAG,
    /* op_bhs      */ OP_BRANCH | USE_FLAGS | USES_CARRY_FLAG,
    /* op_beq      */ OP_BRANCH | USE_FLAGS,
    /* op_bne      */ OP_BRANCH | USE_FLAGS,
    /* op_blt      */ OP_BRANCH | USE_FLAGS,
    /* op_ble      */ OP_BRANCH | USE_FLAGS,
    /* op_bgt      */ OP_BRANCH | USE_FLAGS,
    /* op_bge      */ OP_BRANCH | USE_FLAGS,
    /* op_bz       */ OP_BRANCH | USE_FLAGS,
    /* op_bnz      */ OP_BRANCH | USE_FLAGS,
    /* op_bp       */ OP_BRANCH | USE_FLAGS,
    /* op_bn       */ OP_BRANCH | USE_FLAGS,
    /* op_bnn      */ OP_BRANCH | USE_FLAGS,
    /* op_bud      */ OP_JUMP,
    /* op_blod     */ OP_BRANCH | USE_FLAGS | USES_CARRY_FLAG,
    /* op_blsd     */ OP_BRANCH | USE_FLAGS | USES_CARRY_FLAG,
    /* op_bhid     */ OP_BRANCH | USE_FLAGS | USES_CARRY_FLAG,
    /* op_bhsd     */ OP_BRANCH | USE_FLAGS | USES_CARRY_FLAG,
    /* op_beqd     */ OP_BRANCH | USE_FLAGS,
    /* op_bned     */ OP_BRANCH | USE_FLAGS,
    /* op_bltd     */ OP_BRANCH | USE_FLAGS,
    /* op_bled     */ OP_BRANCH | USE_FLAGS,
    /* op_bgtd     */ OP_BRANCH | USE_FLAGS,
    /* op_bged     */ OP_BRANCH | USE_FLAGS,
    /* op_bzd      */ OP_BRANCH | USE_FLAGS,
    /* op_bnzd     */ OP_BRANCH | USE_FLAGS,
    /* op_bpd      */ OP_BRANCH | USE_FLAGS,
    /* op_bnd      */ OP_BRANCH | USE_FLAGS,
    /* op_bnnd     */ OP_BRANCH | USE_FLAGS,
#ifdef USE_ALL_OPCODES
    /* op_dbu      */ OP_BRANCH | DEST_MODIFY,
    /* op_dblo     */ OP_BRANCH | DEST_MODIFY | USE_FLAGS | USES_CARRY_FLAG,
    /* op_dbls     */ OP_BRANCH | DEST_MODIFY | USE_FLAGS | USES_CARRY_FLAG,
    /* op_dbhi     */ OP_BRANCH | DEST_MODIFY | USE_FLAGS | USES_CARRY_FLAG,
    /* op_dbhs     */ OP_BRANCH | DEST_MODIFY | USE_FLAGS | USES_CARRY_FLAG,
    /* op_dbeq     */ OP_BRANCH | DEST_MODIFY | USE_FLAGS,
    /* op_dbne     */ OP_BRANCH | DEST_MODIFY | USE_FLAGS,
    /* op_dblt     */ OP_BRANCH | DEST_MODIFY | USE_FLAGS,
    /* op_dble     */ OP_BRANCH | DEST_MODIFY | USE_FLAGS,
    /* op_dbgt     */ OP_BRANCH | DEST_MODIFY | USE_FLAGS,
    /* op_dbge     */ OP_BRANCH | DEST_MODIFY | USE_FLAGS,
    /* op_dbz      */ OP_BRANCH | DEST_MODIFY | USE_FLAGS,
    /* op_dbnz     */ OP_BRANCH | DEST_MODIFY | USE_FLAGS,
    /* op_dbp      */ OP_BRANCH | DEST_MODIFY | USE_FLAGS,
    /* op_dbn      */ OP_BRANCH | DEST_MODIFY | USE_FLAGS,
    /* op_dbnn     */ OP_BRANCH | DEST_MODIFY | USE_FLAGS,
    /* op_dbud     */ OP_BRANCH | DEST_MODIFY,
    /* op_dblod    */ OP_BRANCH | DEST_MODIFY | USE_FLAGS | USES_CARRY_FLAG,
    /* op_dblsd    */ OP_BRANCH | DEST_MODIFY | USE_FLAGS | USES_CARRY_FLAG,
    /* op_dbhid    */ OP_BRANCH | DEST_MODIFY | USE_FLAGS | USES_CARRY_FLAG,
    /* op_dbhsd    */ OP_BRANCH | DEST_MODIFY | USE_FLAGS | USES_CARRY_FLAG,
    /* op_dbeqd    */ OP_BRANCH | DEST_MODIFY | USE_FLAGS,
    /* op_dbned    */ OP_BRANCH | DEST_MODIFY | USE_FLAGS,
    /* op_dbltd    */ OP_BRANCH | DEST_MODIFY | USE_FLAGS,
    /* op_dbled    */ OP_BRANCH | DEST_MODIFY | USE_FLAGS,
    /* op_dbgtd    */ OP_BRANCH | DEST_MODIFY | USE_FLAGS,
    /* op_dbged    */ OP_BRANCH | DEST_MODIFY | USE_FLAGS,
    /* op_dbzd     */ OP_BRANCH | DEST_MODIFY | USE_FLAGS,
    /* op_dbnzd    */ OP_BRANCH | DEST_MODIFY | USE_FLAGS,
    /* op_dbpd     */ OP_BRANCH | DEST_MODIFY | USE_FLAGS,
    /* op_dbnd     */ OP_BRANCH | DEST_MODIFY | USE_FLAGS,
    /* op_dbnnd    */ OP_BRANCH | DEST_MODIFY | USE_FLAGS,
#endif				/* USE_ALL_OPCODES */
    /* op_callu    */ OP_CALL | USES_SP,
#ifdef USE_ALL_OPCODES
    /* op_calllo   */ OP_CALL | USE_FLAGS | USES_SP | USES_CARRY_FLAG,
    /* op_callls   */ OP_CALL | USE_FLAGS | USES_SP | USES_CARRY_FLAG,
    /* op_callhi   */ OP_CALL | USE_FLAGS | USES_SP | USES_CARRY_FLAG,
    /* op_callhs   */ OP_CALL | USE_FLAGS | USES_SP | USES_CARRY_FLAG,
    /* op_calleq   */ OP_CALL | USE_FLAGS | USES_SP | USES_CARRY_FLAG,
    /* op_callne   */ OP_CALL | USE_FLAGS | USES_SP,
    /* op_calllt   */ OP_CALL | USE_FLAGS | USES_SP,
    /* op_callle   */ OP_CALL | USE_FLAGS | USES_SP,
    /* op_callgt   */ OP_CALL | USE_FLAGS | USES_SP,
    /* op_callge   */ OP_CALL | USE_FLAGS | USES_SP,
    /* op_callz    */ OP_CALL | USE_FLAGS | USES_SP,
    /* op_callnz   */ OP_CALL | USE_FLAGS | USES_SP,
    /* op_callp    */ OP_CALL | USE_FLAGS | USES_SP,
    /* op_calln    */ OP_CALL | USE_FLAGS | USES_SP,
    /* op_callnn   */ OP_CALL | USE_FLAGS | USES_SP,
#endif				/* USE_ALL_OPCODES */
    /* op_trapu    */ OP_CALL | USES_SP,
#ifdef USE_ALL_OPCODES
    /* op_traplo   */ OP_CALL | USE_FLAGS | USES_SP | USES_CARRY_FLAG,
    /* op_trapls   */ OP_CALL | USE_FLAGS | USES_SP | USES_CARRY_FLAG,
    /* op_traphi   */ OP_CALL | USE_FLAGS | USES_SP | USES_CARRY_FLAG,
    /* op_traphs   */ OP_CALL | USE_FLAGS | USES_SP | USES_CARRY_FLAG,
    /* op_trapeq   */ OP_CALL | USE_FLAGS | USES_SP,
    /* op_trapne   */ OP_CALL | USE_FLAGS | USES_SP,
    /* op_traplt   */ OP_CALL | USE_FLAGS | USES_SP,
    /* op_traple   */ OP_CALL | USE_FLAGS | USES_SP,
    /* op_trapgt   */ OP_CALL | USE_FLAGS | USES_SP,
    /* op_trapge   */ OP_CALL | USE_FLAGS | USES_SP,
    /* op_trapz    */ OP_CALL | USE_FLAGS | USES_SP,
    /* op_trapnz   */ OP_CALL | USE_FLAGS | USES_SP,
    /* op_trapp    */ OP_CALL | USE_FLAGS | USES_SP,
    /* op_trapn    */ OP_CALL | USE_FLAGS | USES_SP,
    /* op_trapnn   */ OP_CALL | USE_FLAGS | USES_SP,
#endif				/* USE_ALL_OPCODES */
    /* op_retiu    */ OP_RETURN | USES_SP,
#ifdef USE_ALL_OPCODES
    /* op_retilo   */ OP_RETURN | USE_FLAGS | USES_SP | USES_CARRY_FLAG,
    /* op_retils   */ OP_RETURN | USE_FLAGS | USES_SP | USES_CARRY_FLAG,
    /* op_retihi   */ OP_RETURN | USE_FLAGS | USES_SP | USES_CARRY_FLAG,
    /* op_retihs   */ OP_RETURN | USE_FLAGS | USES_SP | USES_CARRY_FLAG,
    /* op_retieq   */ OP_RETURN | USE_FLAGS | USES_SP,
    /* op_retine   */ OP_RETURN | USE_FLAGS | USES_SP,
    /* op_retilt   */ OP_RETURN | USE_FLAGS | USES_SP,
    /* op_retile   */ OP_RETURN | USE_FLAGS | USES_SP,
    /* op_retigt   */ OP_RETURN | USE_FLAGS | USES_SP,
    /* op_retige   */ OP_RETURN | USE_FLAGS | USES_SP,
    /* op_retiz    */ OP_RETURN | USE_FLAGS | USES_SP,
    /* op_retinz   */ OP_RETURN | USE_FLAGS | USES_SP,
    /* op_retip    */ OP_RETURN | USE_FLAGS | USES_SP,
    /* op_retin    */ OP_RETURN | USE_FLAGS | USES_SP,
    /* op_retinn   */ OP_RETURN | USE_FLAGS | USES_SP,
#endif				/* USE_ALL_OPCODES */
    /* op_retsu    */ OP_RETURN | USES_SP,
#ifdef USE_ALL_OPCODES
    /* op_retslo   */ OP_RETURN | USE_FLAGS | USES_SP | USES_CARRY_FLAG,
    /* op_retsls   */ OP_RETURN | USE_FLAGS | USES_SP | USES_CARRY_FLAG,
    /* op_retshi   */ OP_RETURN | USE_FLAGS | USES_SP | USES_CARRY_FLAG,
    /* op_retshs   */ OP_RETURN | USE_FLAGS | USES_SP | USES_CARRY_FLAG,
    /* op_retseq   */ OP_RETURN | USE_FLAGS | USES_SP,
    /* op_retsne   */ OP_RETURN | USE_FLAGS | USES_SP,
    /* op_retslt   */ OP_RETURN | USE_FLAGS | USES_SP,
    /* op_retsle   */ OP_RETURN | USE_FLAGS | USES_SP,
    /* op_retsgt   */ OP_RETURN | USE_FLAGS | USES_SP,
    /* op_retsge   */ OP_RETURN | USE_FLAGS | USES_SP,
    /* op_retsz    */ OP_RETURN | USE_FLAGS | USES_SP,
    /* op_retsnz   */ OP_RETURN | USE_FLAGS | USES_SP,
    /* op_retsp    */ OP_RETURN | USE_FLAGS | USES_SP,
    /* op_retsn    */ OP_RETURN | USE_FLAGS | USES_SP,
    /* op_retsnn   */ OP_RETURN | USE_FLAGS | USES_SP,
    /* op_mpf3_adf3 */ PAR_DEST_OVERWRITE | FLOAT_INSTR,
    /* op_mpf3_sbf3 */ PAR_DEST_OVERWRITE | FLOAT_INSTR,
    /* op_mpi3_adi3 */ PAR_DEST_OVERWRITE,
    /* op_mpi3_sbi3 */ PAR_DEST_OVERWRITE,
#endif				/* USE_ALL_OPCODES */
    /* op_stf_stf  */ PAR_DEST_OVERWRITE | FLOAT_INSTR,
    /* op_sti_sti  */ PAR_DEST_OVERWRITE,
    /* op_ldf_ldf  */ PAR_DEST_OVERWRITE | FLOAT_INSTR,
    /* op_ldi_ldi  */ PAR_DEST_OVERWRITE,
    /* op_absf_stf */ PAR_DEST_OVERWRITE | SET_FLAGS | FLOAT_INSTR,
    /* op_absi_sti */ PAR_DEST_OVERWRITE | SET_FLAGS,
    /* op_addf3_stf */ PAR_DEST_OVERWRITE | SET_FLAGS | FLOAT_INSTR,
    /* op_addi3_sti */ PAR_DEST_OVERWRITE | SET_FLAGS,
    /* op_and3_sti */ PAR_DEST_OVERWRITE | SET_FLAGS,
    /* op_ash3_sti */ PAR_DEST_OVERWRITE | SET_FLAGS,
    /* op_fix_sti  */ PAR_DEST_OVERWRITE | SET_FLAGS | FLOAT_INSTR,
    /* op_float_stf */ PAR_DEST_OVERWRITE | SET_FLAGS | FLOAT_INSTR,
    /* op_ldf_stf  */ PAR_DEST_OVERWRITE | FLOAT_INSTR,
    /* op_ldi_sti  */ PAR_DEST_OVERWRITE,
    /* op_lsh3_sti */ PAR_DEST_OVERWRITE | SET_FLAGS,
    /* op_mpyf3_stf */ PAR_DEST_OVERWRITE | SET_FLAGS | FLOAT_INSTR,
    /* op_mpyi3_sti */ PAR_DEST_OVERWRITE | SET_FLAGS,
    /* op_negf_stf */ PAR_DEST_OVERWRITE | SET_FLAGS | FLOAT_INSTR,
    /* op_negi_sti */ PAR_DEST_OVERWRITE | SET_FLAGS,
    /* op_not_sti  */ PAR_DEST_OVERWRITE | SET_FLAGS,
    /* op_or3_sti  */ PAR_DEST_OVERWRITE | SET_FLAGS,
    /* op_subf3_stf */ PAR_DEST_OVERWRITE | SET_FLAGS | FLOAT_INSTR,
    /* op_subi3_sti */ PAR_DEST_OVERWRITE | SET_FLAGS,
    /* op_xor3_sti */ PAR_DEST_OVERWRITE | SET_FLAGS,

    /* op_divs     */ 0,
    /* op_divu     */ 0,

#ifdef ASM
    /* op_asm      */ OP_ASM,
#endif				/* ASM */
    /* op_line     */ 0,
    /* op_label    */ OP_LABEL,
    /* op_abort    */ 0,
};

static OPCODE convert_op_to_op3[] = {
    /* op_absf     */ (OPCODE) 0,
    /* op_absi     */ (OPCODE) 0,
#ifdef USE_ALL_OPCODES
    /* op_addc     */ op_addc3,
#endif				/* USE_ALL_OPCODES */
    /* op_addf     */ op_addf3,
    /* op_addi     */ op_addi3,
    /* op_and      */ op_and3,
    /* op_andn     */ op_andn3,
    /* op_ash      */ op_ash3,
    /* op_cmpf     */ (OPCODE) 0,
    /* op_cmpi     */ (OPCODE) 0,
    /* op_fix      */ (OPCODE) 0,
    /* op_float    */ (OPCODE) 0,
#ifdef USE_ALL_OPCODES
    /* op_idle     */ (OPCODE) 0,
    /* op_lde      */ (OPCODE) 0,
#endif				/* USE_ALL_OPCODES */
    /* op_ldf      */ (OPCODE) 0,
    /* op_popldf   */ (OPCODE) 0,
#ifdef USE_ALL_OPCODES
    /* op_ldfi     */ (OPCODE) 0,
#endif				/* USE_ALL_OPCODES */
    /* op_ldi      */ (OPCODE) 0,
    /* op_popldi   */ (OPCODE) 0,
#ifdef USE_ALL_OPCODES
    /* op_ldii     */ (OPCODE) 0,
    /* op_ldm      */ (OPCODE) 0,
#endif				/* USE_ALL_OPCODES */
    /* op_lsh      */ op_lsh3,
    /* op_mpyf     */ op_mpyf3,
    /* op_mpyi     */ op_mpyi3,
#ifdef USE_ALL_OPCODES
    /* op_negb     */ (OPCODE) 0,
#endif				/* USE_ALL_OPCODES */
    /* op_negf     */ (OPCODE) 0,
    /* op_negi     */ (OPCODE) 0,
    /* op_nop      */ (OPCODE) 0,
#ifdef USE_ALL_OPCODES
    /* op_norm     */ (OPCODE) 0,
#endif				/* USE_ALL_OPCODES */
    /* op_not      */ (OPCODE) 0,
    /* op_pop      */ (OPCODE) 0,
    /* op_popf     */ (OPCODE) 0,
    /* op_push     */ (OPCODE) 0,
    /* op_pushf    */ (OPCODE) 0,
    /* op_pushnopeep */ (OPCODE) 0,
    /* op_pushfnopeep */ (OPCODE) 0,
    /* op_or       */ op_or3,
#ifdef USE_ALL_OPCODES
    /* op_rnd      */ (OPCODE) 0,
    /* op_rol      */ (OPCODE) 0,
    /* op_rolc     */ (OPCODE) 0,
    /* op_ror      */ (OPCODE) 0,
    /* op_rorc     */ (OPCODE) 0,
#endif				/* USE_ALL_OPCODES */
    /* op_rpts     */ (OPCODE) 0,
    /* op_stf      */ (OPCODE) 0,
#ifdef USE_ALL_OPCODES
    /* op_stfi     */ (OPCODE) 0,
#endif				/* USE_ALL_OPCODES */
    /* op_sti      */ (OPCODE) 0,
#ifdef USE_ALL_OPCODES
    /* op_stii     */ (OPCODE) 0,
    /* op_sigi     */ (OPCODE) 0,
    /* op_subb     */ op_subb3,
    /* op_subc     */ (OPCODE) 0,
#endif				/* USE_ALL_OPCODES */
    /* op_subf     */ op_subf3,
    /* op_subi     */ op_subi3,
#ifdef USE_ALL_OPCODES
    /* op_subrb    */ (OPCODE) 0,
#endif				/* USE_ALL_OPCODES */
    /* op_subrf    */ (OPCODE) 0,
    /* op_subri    */ (OPCODE) 0,
    /* op_tstb     */ (OPCODE) 0,
    /* op_xor      */ op_xor3,
#ifdef USE_ALL_OPCODES
    /* op_iack     */ (OPCODE) 0
#endif				/* USE_ALL_OPCODES */
};


static CODE *peep_head = NIL_CODE;
static CODE *next_ip;
static int changes;

#ifdef VERBOSE
static struct
{
    int     ldi_rx_rx;
    int     ldf_rx_rx;
    int     ldi_rx_ry_ldi_ry_rx;
    int     ldf_rx_ry_ldf_ry_rx;
    int     sti_rx_my_ldi_my_rx;
    int     stf_rx_my_ldf_my_rx;
    int     ldi_mx_ry_sti_ry_mx;
    int     ldf_mx_ry_stf_ry_mx;
    int     redundant_ldi;
    int     redundant_ldf;
#ifdef PEEP_REDUNDANT_STI
    int     redundant_sti;
    int     redundant_stf;
#endif
    int     commutative_i;
    int     commutative_f;
    int     ldi_opi_opi3;
    int     ldf_opf_opf3;
    int     opi_ldi_opi3;
    int     opf_ldf_opf3;
    int     par_ldf_stf;
    int     par_stf_stf;
    int     par_ldf_ldf;
    int     par_ldi_sti;
    int     par_sti_sti;
    int     par_ldi_ldi;
    int     convert_to_op3;
    int     remap_register_fwd;
    int     remap_register_bwd;
    int     superfluous_load;
    int     superfluous_tst;
    int     superfluous_cmpf;
    int     brd_cycles;
    int     brd_memory;
    int     brd_optimized;
    int     br_to_next_line;
    int     bxx_to_next_line;
    int     bxx_to_ldcond;
    int     bxx_over_br;
    int     br_block_move;
    int     br_branch_move;
    int     br_commoned;
    int     dead_code;
    int     label_before_branch;
    int     label_commoned;
    int     label_removed;
} PeepStatistic =
{
#ifdef PEEP_REDUNDANT_STI
    0, 0,
#endif /* PEEP_REDUNDANT_STI */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#ifdef DEBUG
}

, LastPeepStatistic = {
#ifdef PEEP_REDUNDANT_STI
    0, 0,
#endif /* PEEP_REDUNDANT_STI */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#endif /* DEBUG */
};

static clock_t delay_time = 0;
static clock_t tstb_time = 0;
static clock_t pipe_time = 0;
static clock_t paralel_time = 0;
static clock_t remap_time = 0;
static clock_t ldi_time = 0;
static clock_t branch_time = 0;
static clock_t cmpi_time = 0;
static clock_t flow_time = 0;

#endif /* VERBOSE */

static int oldtot = 0;
static int oldasm = 0;
static int oldbranch = 0;
static int oldreg = 0;
static int newtot = 0;
static int newasm = 0;
static int newbranch = 0;
static int newreg = 0;

static PEEP_STAGES SelectedOptimizer = ENABLE_ALL;
static int CurrentStage = 0;

static unsigned peep_level = 0;

/*
 * The following list is to find out, which sequence in optimizer is best
 * (Perhaps there isn't 'the best' but different goods, depending
 *  on the code).
 */
static PEEP_STAGES DefaultOptimizerStages[] = {
    REDUNDANT,
    REMAPP,
    COMMUTATIVE,
    COMBINE_BWD,
    COMBINE_FWD,
    DATAFLOW,
    REDUNDANT,
    REMAPP,
    COMMUTATIVE,
    COMBINE_BWD,
    COMBINE_FWD,
    ENABLE_ALL,
    0
};

static PEEP_STAGES *OptimizerStages = &(DefaultOptimizerStages[0]);

/*
 *  table with the reverse condition for a given condition
 *  opcodetables must be in same order as this table for correctness
 */

static OPCODE revcond[] = {
    /* op_blo */ op_bhs,
    /* op_bls */ op_bhi,
    /* op_bhi */ op_bls,
    /* op_bhs */ op_blo,
    /* op_beq */ op_bne,
    /* op_bne */ op_beq,
    /* op_blt */ op_bge,
    /* op_ble */ op_bgt,
    /* op_bgt */ op_ble,
    /* op_bge */ op_blt,
    /* op_bz  */ op_bnz,
    /* op_bnz */ op_bz,
    /* op_bp  */ op_ble,
    /* op_bn  */ op_bnn,
    /* op_bnn */ op_bn
};

static CODE *code
P_ (
    (OPCODE, ITYPE, ADDRESS *, ADDRESS *, ADDRESS *, ADDRESS *, ADDRESS *,

     ADDRESS *));
static void add_peep P_ ((CODE *));
static void opt3 P_ ((unsigned));
static void peep_delete P_ ((const CODE *));
static void peep_line P_ ((CODE *));
static void peep_label P_ ((CODE *));
static void peep_uctran P_ ((const CODE *));
static BOOL is_label_used P_ ((const ADDRESS *, LABEL));
static BOOL is_restricted_indirect P_ ((const ADDRESS *));
static BOOL is_memoryaccess_independent P_ ((const ADDRESS *, const ADDRESS *));

#ifdef CHECK_LIST_INTEGRITY
static void check_peep P_ ((void));

#endif /* CHECK_LIST_INTEGRITY */

/*****************************************************************************/

/*
 * Returns true if the <ea> is suitable for a 3op instruction.
 */
static BOOL is_3op_possible P1 (const ADDRESS *, ap)
{
    if (ap == NIL_ADDRESS) {
	return (FALSE);
    }
    switch (ap->mode) {
    case am_areg:
    case am_ireg:
    case am_dreg:
    case am_freg:
    case am_sreg:
    case am_ind:
    case am_const_ind:
    case am_indx2:
    case am_indxs:
	return (TRUE);
    case am_indx:
    case am_ainc:
    case am_adec:
    case am_preinc:
    case am_predec:
	if ((ap->u.offset->nodetype == en_icon)
	    && (ap->u.offset->v.i >= -1)
	    && (ap->u.offset->v.i <= 1)) {
	    return (TRUE);
	}
	break;
    case am_immed:
    case am_direct:
    case am_const_direct:
    default:
	break;
    }
    return (FALSE);
}

#ifdef ASM
static ADDRESS *mk_branchcomment P0 (void)
{
    ADDRESS *ap;
    EXPR   *ep;

    ep = mk_node (en_str, NIL_EXPR, NIL_EXPR, tp_void);
    ep->v.str = (const CHAR *) ";Branch occurs here";
    ap = (ADDRESS *) xalloc ((size_t) sizeof (ADDRESS));

    ap->mode = am_str;
    ap->u.offset = ep;
    return ap;
}
#endif /* ASM */

/*
 * Find the end of a block of code.
 */
static CODE *block_end P1 (CODE *, ip)
{
    int     count = 0;

    while (ip != NIL_CODE && ip->opcode != op_br
	   && ip->opcode != op_bu
	   && ip->opcode != op_retiu && ip->opcode != op_retsu) {
	if (count == BRANCH_COUNT)
	    return NIL_CODE;
	if (branch (ip))
	    count++;
	ip = ip->fwd;
    }
    return ip;
}


/*
 * Find the node which contains the label 'lab'.
 */
CODE   *find_label P1 (LABEL, lab)
{
    register CODE *ip;

    for (ip = peep_head; ip != NIL_CODE; ip = ip->fwd) {
	if (ip->opcode == op_label && ip->src1->u.offset->v.l == lab)
	    return ip;
    }
    /* we should have found it */
    return NIL_CODE;
}

/*
 * Returns false if the <ea> is not a label or else isn't equal to label.
 */
static BOOL is_label_used P2 (const ADDRESS *, ap, LABEL, label)
{
    return (ap != NIL_ADDRESS &&
	    ap->mode == am_immed &&
	    ap->u.offset->nodetype == en_labcon &&
	    ap->u.offset->v.l == label);
}

/*
 * Counts the number of times that a label node is referenced.
 */
static int label_references P1 (const CODE *, ip)
{
    CODE   *target;
    struct swtab *sw;
    LABEL   i;
    LABEL   lab = ip->src1->u.offset->v.l;
    int     count = 0;

    for (target = peep_head; target != NIL_CODE; target = target->fwd) {
	if ((target != ip) &&
	    (is_label_used (target->src1, lab) ||
	     is_label_used (target->dst, lab)))
/*
 *  src2, src21, src22, dst2 can never contain a label (in this Positions
 *  only indirects and register are allowed)
 */
	    count++;
    }
    for (sw = swtables; sw != NIL_SWITCH; sw = sw->next) {
	for (i = 0; i < sw->numlabs; i++) {
	    if (sw->labels[i] == lab)
		count++;
	}
    }
    if (ip->back != NIL_CODE && ip->back->opcode == op_label)
	count++;
    if (ip->fwd != NIL_CODE && ip->fwd->opcode == op_label)
	count++;
    return count;
}



/*
 * Compare two address nodes and return true if they are equivalent.
 * a/pre-inc/dec are not treated as same
 */
BOOL is_equal_address P2 (const ADDRESS *, ap1, const ADDRESS *, ap2)
{
    if (ap1 == NIL_ADDRESS || ap2 == NIL_ADDRESS)
	return FALSE;
    if (ap1->mode != ap2->mode)
	return FALSE;
    switch (ap1->mode) {
    case am_areg:
    case am_dreg:
    case am_freg:
    case am_ireg:
    case am_sreg:
    case am_ind:
    case am_const_ind:
	return ap1->preg == ap2->preg;
    case am_indx:
	return ap1->preg == ap2->preg &&
	    is_equalnode (ap1->u.offset, ap2->u.offset);
    case am_indx2:
    case am_indxs:
	return ap1->preg == ap2->preg && ap1->sreg == ap2->sreg;
    case am_immed:
    case am_direct:
    case am_const_direct:
	return is_equalnode (ap1->u.offset, ap2->u.offset);
    default:
	break;
    }
    return FALSE;
}

static BOOL is_equal_oper P2 (const ADDRESS *, ap1, const ADDRESS *, ap2)
{
    if (ap1 == NIL_ADDRESS && ap2 == NIL_ADDRESS)
	return TRUE;
    if (ap1 == NIL_ADDRESS || ap2 == NIL_ADDRESS)
	return FALSE;
    if (ap1->mode != ap2->mode)
	return FALSE;
    switch (ap1->mode) {
    case am_areg:
    case am_dreg:
    case am_freg:
    case am_ireg:
    case am_sreg:
    case am_ind:
    case am_const_ind:
	return ap1->preg == ap2->preg;
    case am_indx:
    case am_ainc:
    case am_adec:
    case am_preinc:
    case am_predec:
	return ap1->preg == ap2->preg &&
	    is_equalnode (ap1->u.offset, ap2->u.offset);
    case am_indx2:
    case am_indxs:
	return ap1->preg == ap2->preg && ap1->sreg == ap2->sreg;
    case am_direct:
    case am_const_direct:
    case am_immed:
	return is_equalnode (ap1->u.offset, ap2->u.offset);
    case am_none:
    case am_line:
	return TRUE;
    default:
	break;
    }
    return FALSE;
}


/*
 * Builds the peepinfo of this instruction.
 */
void build_register_map P2 (const CODE *, ip, PEEPINFO *, map)
{
    ADDRESS *src[4];
    ADDRESS *dst[2];
    ADDRESS *ap;
    int     i;

    if (map == NULL) {
	FATAL ((__FILE__, "build_register_map", "NULL-Pointer"));
    }
    src[0] = ip->src1;
    src[1] = ip->src2;
    src[2] = ip->src21;
    src[3] = ip->src22;
    dst[0] = ip->dst;
    dst[1] = ip->dst2;

    map->write = 0;
    map->modified = 0;
    map->read = 0;
    map->used = 0;
    map->updated = 0;
#ifdef REGISTER_FLOW_ANALYZER
    map->used_later = 0;
    map->fixed = 0;
#endif /* REGISTER_FLOW_ANALYZER */
    for (i = 0; i < 4; i++) {
	if ((ap = src[i]) != NIL_ADDRESS) {
	    switch (ap->mode) {
	    case am_areg:
	    case am_ireg:
	    case am_dreg:
	    case am_freg:
	    case am_sreg:
		map->read |= (1UL << (int) ap->preg);
		break;

	    case am_ainc:
	    case am_adec:
	    case am_preinc:
	    case am_predec:
		map->updated |= (1UL << (int) ap->preg);
		map->read |= (1UL << REG_MEMORY);
		break;

	    case am_indx2:
	    case am_indxs:
		map->used |= (1UL << (int) ap->sreg);
		/*lint -fallthrough */
	    case am_ind:
	    case am_const_ind:
	    case am_indx:
		map->used |= (1UL << (int) ap->preg);
		/*lint -fallthrough */
	    case am_direct:
	    case am_const_direct:
		/*lint -fallthrough */
		map->read |= (1UL << REG_MEMORY);
		break;

	    default:
		break;
	    }
	}
    }
    for (i = 0; i < 2; i++) {
	if ((ap = dst[i]) != NIL_ADDRESS) {
	    switch (ap->mode) {
	    case am_areg:
	    case am_ireg:
	    case am_dreg:
	    case am_freg:
	    case am_sreg:
		map->modified |= (1UL << (int) ap->preg);
#if 0
		/*
		 * stack and normal memory should not interfere each other
		 * i.e. modification of SP should not invalidate still
		 * used memory, else we have some problems anyway
		 */
		if (ap->preg == REG_SP) {
		    /* if we change sp, we dont know what hapens with memory */
		    map->modified |= (1UL << REG_MEMORY);
		}
#endif
		break;

	    case am_ainc:
	    case am_adec:
	    case am_preinc:
	    case am_predec:
		map->updated |= (1UL << (int) ap->preg);
		map->modified |= (1UL << REG_MEMORY);
		break;

	    case am_indx2:
	    case am_indxs:
		map->used |= (1UL << (int) ap->sreg);
		/*lint -fallthrough */
	    case am_ind:
	    case am_const_ind:
	    case am_indx:
		map->used |= (1UL << (int) ap->preg);
		/*lint -fallthrough */
	    case am_direct:
	    case am_const_direct:
		/*lint -fallthrough */
		map->modified |= (1UL << REG_MEMORY);
		break;

	    default:
		break;
	    }
	}
    }
    if (is_dest_overwritten (ip->opcode)) {
	map->write = map->modified;
	map->modified = 0;
    }
    if (is_using_sp (ip->opcode)) {
	map->updated |= (1UL << REG_SP);
	/* if we change sp, we dont know what hapens with memory */
	/*
	 * 29.07.96
	 * call, push, pop's should never change memory we are
	 * using in other addressingmodes
	 * so dont mark memory as modified
	 *
	 * map->modified |= (1UL<<REG_MEMORY);
	 */
    }
#ifdef PEEP_CALL_ALSO_VIA_TABLE
    if ((ip->opcode == op_call)
	|| (ip->opcode == op_callu)
	|| (ip->opcode == op_xcall)
	|| (ip->opcode == op_trapu)) {
	for (i = REG_R0; i <= MAX_DATA; i++) {
	    map->write |= 1UL << i;
	}
	for (i = REG_AR0; i <= MAX_ADDR; i++) {
	    map->write |= 1UL << i;
	}
	if (ip->opcode == op_xcall) {
	    map->read = (1UL << REG_R0) | (1UL << REG_R1);
	}
    }
#endif /* PEEP_CALL_ALSO_VIA_TABLE */
}


/*
 * Attaches a peepoinfo to an instruction.
 */

#ifndef SAVE_PEEP_MEMORY
static void attach_peepinfo_fkt P1 (CODE *, ip)
{
    PEEPINFO *map = (PEEPINFO *) xalloc ((size_t) sizeof (PEEPINFO));

    build_register_map (ip, map);
    ip->info = map;
}

void update_peepinfo_fkt P1 (CODE *, ip)
{
    build_register_map (ip, ip->info);
}

#endif /* SAVE_PEEP_MEMORY */

/*
 * Checks if instruction 'ip1' may be moved before instruction
 * 'ip2', ie If both instructions are independent
 * (both may have the same sources, but the destination or modified
 * address register of the one is not to be used by the other).
 */
static BOOL is_ip_swap_possible P2 (const CODE *, ip1, const CODE *, ip2)
{
    REGBITMAP changed1, changed2;
    REGBITMAP needed1, needed2;

#ifndef SAVE_PEEP_MEMORY

    PEEPINFO *map1, *map2;

    map1 = ip1->info;
    map2 = ip2->info;

    /*
     * all registers which are modified in any way in one instruction
     * may not be used in any way by the other instruction
     */
    changed1 = map1->modified | map1->updated | map1->write;
    changed2 = map2->modified | map2->updated | map2->write;
    needed1 = map1->modified | map1->updated | map1->read | map1->used;
    needed2 = map2->modified | map2->updated | map2->read | map2->used;

#else /* SAVE_PEEP_MEMORY */

    PEEPINFO map1, map2;

    build_register_map (ip1, &map1);
    build_register_map (ip2, &map2);

    /*
     * all registers which are modified in any way in one instruction
     * may not be used in any way by the other instruction
     */
    changed1 = map1.modified | map1.updated | map1.write;
    changed2 = map2.modified | map2.updated | map2.write;
    needed1 = map1.modified | map1.updated | map1.read | map1.used;
    needed2 = map2.modified | map2.updated | map2.read | map2.used;
#endif /* SAVE_PEEP_MEMORY */

    /*
     *  we do not want to swap instructions who changes the stack
     *  with instructions who access memory, else we may get a chaos
     *
     *  for example
     *
     *  ldi   *ar2, r0
     *  subi   4, sp
     *
     *  is not save to swapp, ar2 may point to one of the 4 words
     *  behind sp, which are no longer valid after the subi instruction
     *
     */
    if (changed1 & (1UL << REG_SP)) {
	changed1 |= (1UL << REG_MEMORY);
    }
    if (changed2 & (1UL << REG_SP)) {
	changed2 |= (1UL << REG_MEMORY);
    }
    if (((changed1 & needed2) == 0)
	&& ((changed2 & needed1) == 0)) {
	return (TRUE);
    } else {
#ifdef PEEP_REDUNDANT_STI
	if (((changed1 & needed2) | (changed2 & needed1)) ==
	    (1UL << REG_MEMORY)) {
	    if (((changed1 & (1UL << REG_SP)) == 0)
		&& ((changed2 & (1UL << REG_SP)) == 0)
		&& (is_memoryaccess_independent (ip1->dst, ip2->src1))
		&& (is_memoryaccess_independent (ip1->dst, ip2->src2))
		&& (is_memoryaccess_independent (ip1->dst2, ip2->src21))
		&& (is_memoryaccess_independent (ip1->dst2, ip2->src22))
		&& (is_memoryaccess_independent (ip2->dst, ip1->src1))
		&& (is_memoryaccess_independent (ip2->dst, ip1->src2))
		&& (is_memoryaccess_independent (ip2->dst2, ip1->src21))
		&& (is_memoryaccess_independent (ip2->dst2, ip1->src22))) {
		return TRUE;
	    }
	}
#endif
	return (FALSE);
    }
}




/*
 * Sets instuctionsflags for given instruction, simplifies
 * the work for the peephole-optimizer.
 */
static void set_peepflags P1 (CODE *, ip)
{
    ADDRESS *src[4];
    ADDRESS *dst[2];
    ADDRESS *ap;
    int     i;

    src[0] = ip->src1;
    src[1] = ip->src2;
    src[2] = ip->src21;
    src[3] = ip->src22;
    dst[0] = ip->dst;
    dst[1] = ip->dst2;

    ip->flags = 0;
    for (i = 0; i < 4; i++) {
	if ((ap = src[i]) != NIL_ADDRESS) {
	    switch (ap->mode) {
	    case am_areg:
		ip->flags |= PEEP_AREG;
		break;
	    case am_ireg:
		ip->flags |= PEEP_IREG;
		break;
	    case am_ainc:
	    case am_adec:
	    case am_preinc:
	    case am_predec:
		ip->flags |= PEEP_AINC;
		break;
	    case am_ind:
	    case am_const_ind:
	    case am_indx:
		ip->flags |= PEEP_IND;
		break;
	    case am_indx2:
	    case am_indxs:
		ip->flags |= PEEP_INDEX;
		break;
	    case am_direct:
	    case am_const_direct:
		ip->flags |= PEEP_DIRECT;
		break;
	    case am_sreg:
		if (ap->preg == REG_SP) {
		    ip->flags |= PEEP_SP;
		}
		break;
	    default:
		break;
	    }
	}
    }
    for (i = 0; i < 2; i++) {
	if ((ap = dst[i]) != NIL_ADDRESS) {
	    switch (ap->mode) {
	    case am_areg:
		ip->flags |= PEEP_DST_AREG;
		break;
	    case am_ireg:
		ip->flags |= PEEP_DST_IREG;
		break;
	    case am_ainc:
	    case am_adec:
	    case am_preinc:
	    case am_predec:
		ip->flags |= PEEP_DST_AINC;
		break;
	    case am_ind:
	    case am_const_ind:
	    case am_indx:
		ip->flags |= PEEP_DST_IND;
		break;
	    case am_indx2:
	    case am_indxs:
		ip->flags |= PEEP_DST_INDEX;
		break;
	    case am_direct:
	    case am_const_direct:
		ip->flags |= PEEP_DST_DIRECT;
		break;
	    case am_sreg:
		if (ap->preg == REG_SP) {
		    ip->flags |= PEEP_DST_SP;
		}
		break;
	    default:
		break;
	    }
	}
    }
    if (is_controltransfer (ip->opcode)) {
	ip->flags |= PEEP_TRANSFER;
    }
    if (is_set_all_flags (ip->opcode)) {
	ip->flags |= PEEP_COMPARE;
    }
}


/*
 * Global code analysis, search for all sorts of pipeline conflicts
 * and reports them (mainly used for statistical analysis of
 * the optimizerperformance).
 */
static void analyze_code P4 (int *, totsize, int *, branchconflicts,
			     int *, registerconflicts, int *, asmstatements)
{
    CODE   *ip;

    int     tot = 0;
    int     branches = 0;
    int     reg = 0;
    int     asmstat = 0;
    int     group1blocked = 0;
    int     group3blocked = 0;

    REGBITMAP flags;
    PEEPINFO *map;

#ifdef SAVE_PEEP_MEMORY
    PEEPINFO StaticMemory;

#endif /* SAVE_PEEP_MEMORY */

    for (ip = peep_head; ip != NIL_CODE; ip = ip->fwd) {
	switch (ip->opcode) {
	case op_callu:
	case op_call:
	case op_trapu:
	case op_xcall:
	case op_retiu:
	case op_retsu:
	case op_blo:
	case op_bls:
	case op_bhi:
	case op_bhs:
	case op_beq:
	case op_bne:
	case op_blt:
	case op_ble:
	case op_bgt:
	case op_bge:
	case op_bz:
	case op_bnz:
	case op_bp:
	case op_bn:
	case op_bnn:
	case op_bu:
	case op_br:
	    branches += 3;
	    tot++;
	    group1blocked = 0;
	    group3blocked = 0;
	    break;

	case op_abort:
	    FATAL ((__FILE__, "analyze_code", "unexpected abort"));
	    break;

#ifdef ASM
	case op_asm:		/* really it in unknown */
	    asmstat++;
	    break;
#endif /* ASM */


	case op_bud:
	case op_brd:
	default:

	    map = GET_PEEP_INFO (ip, &StaticMemory);

	    flags = map->used | map->updated;
	    if (flags & GROUP_1_REGISTERS) {
		reg += group1blocked;
		group1blocked = 0;
		group3blocked = 0;
	    }
	    if (flags & GROUP_3_REGISTERS) {
		reg += group3blocked;
		group1blocked = 0;
		group3blocked = 0;
	    }
	    flags = map->read;
	    if (flags & GROUP_1_READ) {
		/* Delay is 1, but will allready be decremented in
		 * this loop, so set to 2
		 */
		group1blocked = 2;
	    }
	    if (flags & GROUP_3_READ) {
		/* Delay is 1, but will allready be decremented in
		 * this loop, so set to 2
		 */
		group3blocked = 2;
	    }
	    flags = map->modified | map->write;
	    if (flags & GROUP_1_WRITE) {
		/* Delay is 2, but will allready be decremented in
		 * this loop, so set to 3
		 */
		group1blocked = 3;
	    }
	    if (flags & GROUP_3_WRITE) {
		/* Delay is 2, but will allready be decremented in
		 * this loop, so set to 3
		 */
		group3blocked = 3;
	    }
	    if (group1blocked > 0) {
		group1blocked--;
	    }
	    if (group3blocked > 0) {
		group3blocked--;
	    }
	    tot++;
	    break;

	case op_label:
	case op_line:
	    break;
	}
    }
    *totsize += tot;
    *branchconflicts += branches;
    *registerconflicts += reg;
    *asmstatements += asmstat;
}



/*
 * Returns false if the register reg1 is not used in the <ea> of ap2, otherwise
 * it returns true.  If we aren't sure then return true anyway.
 */
BOOL is_register_used P2 (REG, reg1, const ADDRESS *, ap2)
{
    if (ap2 == NIL_ADDRESS)
	return FALSE;
    switch (ap2->mode) {
    case am_areg:
    case am_dreg:
    case am_freg:
    case am_ireg:
    case am_sreg:
    case am_ainc:
    case am_adec:
    case am_preinc:
    case am_predec:
    case am_ind:
    case am_const_ind:
    case am_indx:
	return ap2->preg == reg1;
    case am_indx2:
    case am_indxs:
	return ap2->sreg == reg1 || ap2->preg == reg1;
    case am_immed:
    case am_direct:
    case am_const_direct:
    case am_none:
    case am_line:
	return FALSE;
    default:
	break;
    }
    return TRUE;
}


/*
 * Checks to see if the register reg is overwritten with a new value
 * before the value is used (or the value is never used!).
 * If the value is not used again, FALSE is returned.
 */
static BOOL is_value_used P2 (REG, reg, const CODE *, ip)
{
    CODE   *ip2, *target;
    BOOL    result = FALSE;
    OPCODE  op;
    SWITCH *sw, *table;
    LABEL   label;
    unsigned i;

    REGBITMAP needed;
    REGBITMAP regbit = (1UL << (int) reg);
    PEEPINFO *map;

#ifdef SAVE_PEEP_MEMORY
    PEEPINFO StaticMemory;

#endif /* SAVE_PEEP_MEMORY */

    if (ip == NIL_CODE)
	return FALSE;
    for (ip2 = ip->fwd; ip2 != NIL_CODE; ip2 = ip2->fwd) {
	switch (ip2->opcode) {
	case op_retsu:
	    if ((reg == RESULT)
		|| (reg == STACKPTR)) {
		/*
		 **  Might be a return value
		 */
		return TRUE;
	    } else {
		return FALSE;
	    }
	case op_retiu:
	case op_abort:
	    return FALSE;
#ifdef ASM
	case op_asm:		/* really it in unknown */
	    return TRUE;
#endif /* ASM */
	case op_blo:
	case op_bls:
	case op_bhi:
	case op_bhs:
	case op_beq:
	case op_bne:
	case op_blt:
	case op_ble:
	case op_bgt:
	case op_bge:
	case op_bz:
	case op_bnz:
	case op_bp:
	case op_bn:
	case op_bnn:
	    op = ip2->opcode;
	    ip2->opcode = op_abort;	/* to prevent looping */
	    /* follow both paths of the branch */
	    result = is_value_used (reg, ip2) ||
		is_value_used (reg, find_label (ip2->src1->u.offset->v.l));
	    ip2->opcode = op;
	    return result;
	case op_bud:
	case op_brd:
	    /*
	     * brd should not be found in this stage of
	     * codeanalysis. return TRUE if still found.
	     */
	    return TRUE;
	case op_bu:
	    /*
	     * bu is used in switchtable, so we cannot say
	     * (we had to check al paths of the switch), assume
	     * value is used, thats save in any case
	     * if enabled in option we analyse switches too
	     */
	    if (!is_peep_phase (peep_level, PEEP_SWITCH)) {
		return TRUE;
	    }
	    /* look for the switchtable belonging to that op_bu */
	    table = NIL_SWITCH;
	    for (sw = swtables; sw != NIL_SWITCH; sw = sw->next) {
		/* genc30 adds the labelname into the offsetfield of src1 */
		if (ip2->src1->u.offset->v.l == sw->tablab) {
		    table = sw;
		    break;
		}
	    }
	    if (table != NIL_SWITCH) {
		op = ip2->opcode;
		ip2->opcode = op_abort;	/* to prevent looping */
		result = is_register_used (reg, ip2->src1);
		/* follow all paths of the switch */
		for (i = 0; i < table->numlabs; i++) {
		    label = table->labels[i];
		    target = find_label (label);
		    if (target == NULL) {
			FATAL (
			       (__FILE__, "is_value_used",
				"switch_target not found"));
		    }
		    result |= is_value_used (reg, target);
		}
		ip2->opcode = op;
		return result;
	    } else {
		FATAL (
		       (__FILE__, "is_value_used",
			"switchtable for bu not found"));
	    }
	    return TRUE;

	case op_br:
	    op = ip2->opcode;
	    ip2->opcode = op_abort;	/* to prevent looping */
	    result = is_register_used (reg, ip2->src1) ||
		is_value_used (reg, find_label (ip2->src1->u.offset->v.l));
	    ip2->opcode = op;
	    return result;
#ifndef PEEP_CALL_ALSO_VIA_TABLE
	case op_xcall:
	    if (is_temporary_register (reg)) {
		if ((reg == REG_R0) || (reg == REG_R1))
		    return TRUE;	/* may be used to pass parameters */
		else
		    return FALSE;
	    }
	    /*lint -fallthrough */
	case op_callu:
	case op_trapu:
	case op_call:
	    if (reg == STACKPTR)
		return TRUE;
	    if (is_temporary_register (reg)) {
		return FALSE;
	    }
	    /*lint -fallthrough */
#endif /* PEEP_CALL_ALSO_VIA_TABLE */
	default:

	    map = GET_PEEP_INFO (ip2, &StaticMemory);

	    needed = map->read | map->modified | map->used | map->updated;

	    /*
	     *  if register is used (read or modified)
	     *  we return TRUE (it is used :-) )
	     */
	    if (needed & regbit) {
		return TRUE;
	    }
	    /*
	     *  if register is written to
	     *  we return FALSE ( Contents are obviously not used)
	     */
	    if (map->write & regbit) {
		return FALSE;
	    }
	    break;
	case op_label:
	case op_line:
	    break;
	}
    }
    return FALSE;
}

/*
 * Checks to see if a remap of registers is possible in a forward direction
 * e.g. if register 'search' may be replaced with 'replace' in the
 * following instruction sequence.
 *
 * This is not possible if:
 *
 *       'search' is STACKPTR.
 *
 *       'search' is RESULT and sequence ends with return.
 *
 *       An asm-statement is in the sequence.
 *
 *       'replace' is used or modified.
 *
 *       'replace' is overwritten and more replacements follow in
 *       this sequence.
 *
 *       'replace' is not an address register and 'search' is used
 *       as an address register or index register in an indirect
 *       address mode.
 *
 * By branches, labels and jumps we return FALSE, because following
 * all paths and do all replacements correctly is somewhat hairy
 * (I just dont know how to do it, so I let it be).
 *
 * The sequence ends if return is found or 'search' is overwritten.
 */
static BOOL is_remap_possible_fwd P3 (REG, search_reg, REG, replace,
				      const CODE *, ip)
{
    CODE   *ip2;
    BOOL    replacement_forbidden = FALSE;
    BOOL    result;
    REGBITMAP changed;
    REGBITMAP needed;
    REGBITMAP replacebit = (1UL << (int) replace);
    REGBITMAP searchbit = (1UL << (int) search_reg);
    PEEPINFO *map;

#ifdef SAVE_PEEP_MEMORY
    PEEPINFO StaticMemory;

#endif /* SAVE_PEEP_MEMORY */

    if (ip == NIL_CODE)
	return FALSE;
    for (ip2 = ip->fwd; ip2 != NIL_CODE; ip2 = ip2->fwd) {
	switch (ip2->opcode) {
	case op_retsu:
	    /* We are at the end of the block, if search_reg is sp
	     * or RESULT we cannot replace registers */
	    if ((search_reg == RESULT)
		|| (search_reg == STACKPTR)) {
		/*
		 **  Might be a return value
		 */
		return FALSE;
	    } else {
		return TRUE;
	    }
	case op_retiu:
	case op_abort:
	    return FALSE;
#ifdef ASM
	case op_asm:		/* really it in unknown */
	    return FALSE;
#endif /* ASM */
	case op_blo:
	case op_bls:
	case op_bhi:
	case op_bhs:
	case op_beq:
	case op_bne:
	case op_blt:
	case op_ble:
	case op_bgt:
	case op_bge:
	case op_bz:
	case op_bnz:
	case op_bp:
	case op_bn:
	case op_bnn:
	    /* may be too complicated to follow, so we let it be for now */
	    /* so we use the simpler variant of just checking if search  */
	    /* and replace are not used anymore on jumpdestination       */
#if 1
	    if (is_peep_phase (peep_level, PEEP_HARD_REMAP)) {
		result =
		    is_value_used (search_reg,
				   find_label (ip2->src1->u.offset->v.l));
		result = result
		    || is_value_used (replace,
				      find_label (ip2->src1->u.offset->v.l));
		if (result == TRUE) {
		    return FALSE;
		}
	    } else {
		return FALSE;
	    }
	    break;
#else
	    return FALSE;
#endif
	case op_br:
#if 1
	    if (is_peep_phase (peep_level, PEEP_HARD_REMAP)) {
		result =
		    is_value_used (search_reg,
				   find_label (ip2->src1->u.offset->v.l));
		result = result
		    || is_value_used (replace,
				      find_label (ip2->src1->u.offset->v.l));
		if (result == TRUE) {
		    return FALSE;
		}
		return TRUE;
	    }
#endif
	    return FALSE;
	case op_bu:
#if 1
	    if (is_peep_phase (peep_level, PEEP_VERY_HARD_REMAP)) {
		/* use ip2->back, then is_value_used will analyze the switch for us */
		result = is_value_used (search_reg, ip2->back);
		result = result || is_value_used (replace, ip2->back);
		if (result == TRUE) {
		    return FALSE;
		}
		return TRUE;
	    }
#endif
	    return FALSE;
	case op_bud:
	case op_brd:
	    /* We wont follow branches, too difficult */
	    return FALSE;
#ifdef  PEEP_CALL_ALSO_VIA_TABLE
	case op_xcall:
	    if ((search_reg == REG_R0) || (search_reg == REG_R1)) {
		/*
		 * Registers are used to pass parameters,
		 * no exchange possible
		 */
		return FALSE;
	    }
	    /*lint -fallthrough */
#else /* PEEP_CALL_ALSO_VIA_TABLE */
	case op_callu:
	case op_trapu:
	case op_call:
	case op_xcall:
	    /* if search_reg or dest is a temporary we cannot do any replacements */
	    if (is_temporary_register (search_reg)
		|| is_temporary_register (replace))
		return FALSE;
	    if (search_reg == STACKPTR)
		return FALSE;
	    /*lint -fallthrough */
#endif /* PEEP_CALL_ALSO_VIA_TABLE */
	default:
	    map = GET_PEEP_INFO (ip2, &StaticMemory);

	    changed = map->write | map->modified | map->updated;
	    needed = map->read | map->modified | map->used | map->updated;

	    /*
	     *  if replaceregister is modified we can do no further replacements
	     */
	    if (changed & (replacebit)) {
		replacement_forbidden = TRUE;
	    }
	    /*
	     *  Check if register is used in any indirect way
	     *  if so, replace must be of same type
	     *  (if search is ar, replace must be ar
	     *   if search is index, replace must be index)
	     */
	    if (((map->used | map->updated) & (searchbit)) != 0) {
		if (!is_same_register_type (search_reg, replace)) {
		    return FALSE;
		}
	    }
	    /*
	     * Check for any pending replacements if replacement is forbidden
	     */
	    if (replacement_forbidden == TRUE) {
		if (needed & (searchbit)) {
		    return FALSE;
		}
	    }
	    /*
	     * if Search is overwritten, we can end remap here
	     */
	    if (map->write & (searchbit)) {
		return TRUE;
	    }
	    /*
	     *  if searchregister is modified, we may only replace
	     *  if replaceregister is not used anymore
	     */
	    if (changed & (searchbit)) {
		if (is_value_used (replace, ip2)) {
		    return FALSE;
		}
	    }
	    break;

	case op_label:
#if 1
	    if (is_peep_phase (peep_level, PEEP_HARD_REMAP)) {
		result = is_value_used (search_reg, ip2);
		result = result || is_value_used (replace, ip2);
		if (result == TRUE) {
		    return FALSE;
		}
		return TRUE;
	    }
#endif
	    return FALSE;
	case op_line:
	    break;
	}
    }
    return TRUE;
}

/*
 * Checks to see if a remap of registers is possible in the backward direction
 * e.g. if register 'search_reg' may be replaced with 'replace' in the
 * following instruction sequence.
 *
 * This is not possible if:
 *
 *       'search_reg' is STACKPTR.
 *
 *       'replace' is temporary and we have a call in the sequence.
 *
 *       'search_reg'  is temporary and we have a call in the sequence.
 *
 *       an asm-statement is in the sequence.
 *
 *       'replace' is used and more replacements follow in
 *       this sequence upwards.
 *
 *       'replace' is not an address register and search_reg is used
 *       as an address register or index register in an indirect
 *       address mode.
 *
 * By branches, labels and jumps we return FALSE, because following
 * all paths and do all replacements correctly is somewhat hairy
 * (I just dont know how to do it, so I let it be).
 *
 * The sequence ends if a block begin is found or 'search_reg' is overwritten.
 */
static BOOL is_remap_possible_bwd P3 (REG, search_reg, REG, replace,
				      const CODE *, ip)
{
    CODE   *ip2;
    BOOL    result;
    REGBITMAP changed;
    REGBITMAP needed;
    REGBITMAP replacebit = (1UL << (int) replace);
    REGBITMAP searchbit = (1UL << (int) search_reg);
    PEEPINFO *map;

#ifdef SAVE_PEEP_MEMORY
    PEEPINFO StaticMemory;

#endif /* SAVE_PEEP_MEMORY */
    if (ip == NIL_CODE)
	return FALSE;
    for (ip2 = ip->back; ip2 != NIL_CODE; ip2 = ip2->back) {
	switch (ip2->opcode) {
	case op_retsu:
	case op_retiu:
	    /* We should have found a label before we reach here, else
	     * this is dead code, so we can swap what we want
	     */
	    return TRUE;
	case op_abort:
	    /* we have closed a loop, so replace must be possible   */
	    return TRUE;
#ifdef ASM
	case op_asm:		/* really it in unknown */
	    return FALSE;
#endif /* ASM */
	case op_blo:
	case op_bls:
	case op_bhi:
	case op_bhs:
	case op_beq:
	case op_bne:
	case op_blt:
	case op_ble:
	case op_bgt:
	case op_bge:
	case op_bz:
	case op_bnz:
	case op_bp:
	case op_bn:
	case op_bnn:
	    /* may be too complicated to follow, so we let it be for now */
	    /* so we use the simpler variant of just checking if search  */
	    /* and replace are not used anymore on jumpdestination       */
#if 1
	    if (is_peep_phase (peep_level, PEEP_HARD_REMAP)) {
		result =
		    is_value_used (search_reg,
				   find_label (ip2->src1->u.offset->v.l));
		result = result
		    || is_value_used (replace,
				      find_label (ip2->src1->u.offset->v.l));
		if (result == TRUE) {
		    return FALSE;
		}
	    } else {
		return FALSE;
	    }
	    break;
#else
	    return FALSE;
#endif
	case op_br:
	case op_bu:
	case op_bud:
	case op_brd:
	    /* We should have found a label before we reach here, else
	     * this is dead code, so we can swap what we want
	     */
	    return TRUE;

	case op_callu:
	case op_call:
	case op_trapu:
	case op_xcall:
	    if (is_temporary_register (replace))
		return FALSE;
	    if ((search_reg == STACKPTR)
		|| (search_reg == RESULT))
		return FALSE;
	    if (is_temporary_register (search_reg))
		return TRUE;
	    /*lint -fallthrough */
	default:

	    map = GET_PEEP_INFO (ip2, &StaticMemory);

	    changed = map->write | map->modified | map->updated;
	    needed = map->read | map->modified | map->used | map->updated;

	    /* if search_reg is overwritten, sequence ends here
	     * replace is possible if it was not forbidden
	     * (If sequence would not end with overwrite,
	     * replacement would be possible independent of forbidden)
	     */
	    if (map->write & (searchbit)) {
		/*
		 * was return !replacement_forbidden;
		 * (see comment somwhat farther down)
		 */
		return TRUE;
	    }
	    /*
	     *  Check if register is used in any indirect way
	     *  if so, replace must be of same type
	     *  (if search is ar, replace must be ar
	     *   if search is index, replace must be index)
	     */
	    if (((map->used | map->updated) & (searchbit)) != 0) {
		if (!is_same_register_type (search_reg, replace)) {
		    return FALSE;
		}
	    }
	    /*
	     *  Check if replaceregister is modified in any way
	     *  if so, replace is not possible
	     *  but there is also a register, whose value is not
	     *  used (-> we started by an ldi search, replace
	     *            and did not found any instruction using
	     *            contents of replace, so this modifications
	     *            of replace are used nowhere)
	     *   theoretically, this should not happen :-)
	     *
	     */
	    if (((changed) & (replacebit)) != 0) {
		return FALSE;
	    }
	    /*
	     *  if contents of replace are needed here, further
	     *  replacement is not possible
	     *
	     *  theoretically I should use here (map->read | map->used)
	     *  instead of needed, but the cases map->modified and map->updated
	     *  which are also contained in needed are already handled in the
	     *  precedent statement, so no problem at all
	     */

	    if (((needed) & (replacebit)) != 0) {

		/*
		 *  Was replacement_forbidden = TRUE;
		 *  but I don't see what what difference that makes
		 *  to directly returning FALSE
		 *  (There was only one possibility to return TRUE,
		 *  when we reached the head of the function without
		 *  any further references of search, which means we
		 *  worked with uninitialized registers, which should
		 *  never happen
		 */

		return FALSE;
	    }
	    break;

	case op_label:
	    /* we wont follow jumps now */
	    if (ip2->back != NIL_CODE)
		return FALSE;
	    else
		return TRUE;	/* Functionshead allways starts with a label */
	case op_line:
	    break;
	}
    }
    return TRUE;
}


static ADDRESS *replace_register P3 (ADDRESS *, ap, REG, search_reg, REG,
				     replace)
{
    ADDRESS *newap;

    if (ap == NIL_ADDRESS)
	return ap;
    switch (ap->mode) {
    case am_areg:
    case am_dreg:
    case am_ireg:
    case am_sreg:
	if (ap->preg == search_reg) {
	    return mk_reg (replace);
	}
	break;
    case am_freg:
	if (ap->preg == search_reg) {
	    return mk_freg (replace);
	}
	break;
    case am_ainc:
    case am_adec:
    case am_preinc:
    case am_predec:
    case am_ind:
    case am_const_ind:
    case am_indx:
	if (ap->preg == search_reg) {
	    if (is_address_register (replace)) {
		newap = copy_addr (ap, ap->mode);
		newap->preg = replace;
		return newap;
	    }
	}
	break;
    case am_indx2:
    case am_indxs:
	if (ap->preg == search_reg) {
	    if (is_address_register (replace)) {
		newap = copy_addr (ap, ap->mode);
		newap->preg = replace;
		return newap;
	    }
	}
	if (ap->sreg == search_reg) {
	    if ((replace == REG_IR0) || (replace == REG_IR1)) {
		newap = copy_addr (ap, ap->mode);
		newap->sreg = replace;
		return newap;
	    }
	}
	break;
    case am_immed:
    case am_direct:
    case am_const_direct:
    case am_none:
    case am_line:
	break;
    default:
	break;
    }
    FATAL ((__FILE__, "replace_register", "inconsistency"));
    return ap;
}


/*
 *  Replaces in the given instruction all references to
 *  register 'search_reg' with references to 'replace'.
 *
 */

static void replace_all_registers P3 (CODE *, ip, REG, search_reg, REG,
				      replace)
{
    if (is_register_used (search_reg, ip->src1)) {
	ip->src1 = replace_register (ip->src1, search_reg, replace);
    }
    if (is_register_used (search_reg, ip->src2)) {
	ip->src2 = replace_register (ip->src2, search_reg, replace);
    }
    if (is_register_used (search_reg, ip->src21)) {
	ip->src21 = replace_register (ip->src21, search_reg, replace);
    }
    if (is_register_used (search_reg, ip->src22)) {
	ip->src22 = replace_register (ip->src22, search_reg, replace);
    }
    if (is_register_used (search_reg, ip->dst)) {
	ip->dst = replace_register (ip->dst, search_reg, replace);
    }
    if (is_register_used (search_reg, ip->dst2)) {
	ip->dst2 = replace_register (ip->dst2, search_reg, replace);
    }
    /*
     * Not forgetting to update infofield in instruction !!!
     */
    Update_Peep_Info (ip);
}

/*
 *  Replaces in the given instruction all source references to
 *  register 'search_reg' with references to 'replace'.
 *
 */
static void replace_source_registers P3 (CODE *, ip, REG, search_reg, REG,
					 replace)
{
    if (is_register_used (search_reg, ip->src1)) {
	ip->src1 = replace_register (ip->src1, search_reg, replace);
    }
    if (is_register_used (search_reg, ip->src2)) {
	ip->src2 = replace_register (ip->src2, search_reg, replace);
    }
    if (is_register_used (search_reg, ip->src21)) {
	ip->src21 = replace_register (ip->src21, search_reg, replace);
    }
    if (is_register_used (search_reg, ip->src22)) {
	ip->src22 = replace_register (ip->src22, search_reg, replace);
    }
    /*
     * Not forgetting to update infofield in instruction !!!
     */
    Update_Peep_Info (ip);
}

/*
 *  Replaces in the given instruction all destination references to
 *  register 'search_reg' with references to 'replace'.
 *
 */
static void replace_destination_registers P3 (CODE *, ip, REG, search_reg,
					      REG, replace)
{
    if (is_register_used (search_reg, ip->dst)) {
	ip->dst = replace_register (ip->dst, search_reg, replace);
    }
    if (is_register_used (search_reg, ip->dst2)) {
	ip->dst2 = replace_register (ip->dst2, search_reg, replace);
    }
    /*
     * Not forgetting to update infofield in instruction !!!
     */
    Update_Peep_Info (ip);
}


/*
 * Remaps registers, eg replaces register 'search_reg' with 'replace' in the
 * following instruction sequence until 'search_reg' is overwritten.
 *
 * This routine is only to be called if replace is really possible
 * (Check with 'is_remap_possible_fwd') else FATAL will be called.
 *
 * Remap is not possible if:
 *
 *       'search_reg' is STACKPTR.
 *
 *       'search_reg' is RESULT and sequence ends with return.
 *
 *       An asm-statement is in the sequence.
 *
 *       'replace' is used or modified.
 *
 *       'replace' is overwritten and more replacements follow in
 *       this sequence.
 *
 *       'replace' is not an address register and 'search_reg' is used
 *       as an address register or index register in an indirect
 *       address mode.
 *
 *
 * By branches, labels and jumps we return FALSE, because following
 * all paths and do all replacements correctly is somewhat hairy
 * (I just dont know how to do it, so I let it be)
 *
 * The sequence ends if return is found or 'search_reg' is overwritten.
 */

static void do_remap_fwd P3 (REG, search_reg, REG, replace, const CODE *, ip)
{
    CODE   *ip2;

    /* Inhibits further replacements */
    BOOL    replacement_forbidden = FALSE;
    REGBITMAP changed;
    REGBITMAP needed;
    REGBITMAP replacebit = (1UL << (int) replace);
    REGBITMAP searchbit = (1UL << (int) search_reg);
    PEEPINFO *map;

#ifdef SAVE_PEEP_MEMORY
    PEEPINFO StaticMemory;

#endif /* SAVE_PEEP_MEMORY */
    if (ip == NIL_CODE) {
	FATAL ((__FILE__, "do_remap_fwd", "inconsistency 0"));
	return;
    }
    for (ip2 = ip->fwd; ip2 != NIL_CODE; ip2 = ip2->fwd) {
	switch (ip2->opcode) {
	case op_retsu:
	case op_retiu:
	    /* We have reached end of block, we are finished */
	    return;
	case op_abort:
	    /* should never happen */
	    FATAL ((__FILE__, "do_remap_fwd", "inconsistency 0"));
	    return;
#ifdef ASM
	case op_asm:		/* really it in unknown */
	    FATAL ((__FILE__, "do_remap_fwd", "inconsistency 1"));
	    return;
#endif /* ASM */
	case op_blo:
	case op_bls:
	case op_bhi:
	case op_bhs:
	case op_beq:
	case op_bne:
	case op_blt:
	case op_ble:
	case op_bgt:
	case op_bge:
	case op_bz:
	case op_bnz:
	case op_bp:
	case op_bn:
	case op_bnn:
#if 0
	    /* may be too complicated to follow, so we let it be for now */
	    op = ip2->opcode;
	    ip2->opcode = op_abort;	/* to prevent looping */
	    result =
		is_remap_possible_fwd (search_reg, remap,
				       find_jump_with_label (ip2->src1->
							     u.offset->v.l));
	    ip2->opcode = op;
	    return result;
#endif
	    if (!is_peep_phase (peep_level, PEEP_HARD_REMAP)) {
		FATAL ((__FILE__, "do_remap_fwd", "inconsistency 2"));
		return;
	    }
	    break;
	case op_br:
	    if (!is_peep_phase (peep_level, PEEP_HARD_REMAP)) {
		FATAL ((__FILE__, "do_remap_fwd", "inconsistency 2"));
	    }
	    return;
	case op_bu:
	    if (!is_peep_phase (peep_level, PEEP_VERY_HARD_REMAP)) {
		FATAL ((__FILE__, "do_remap_fwd", "inconsistency 2b"));
	    }
	    return;
	case op_bud:
	case op_brd:
	    FATAL ((__FILE__, "do_remap_fwd", "inconsistency 2a"));
	    return;

#ifdef  PEEP_CALL_ALSO_VIA_TABLE
	case op_xcall:
	    if ((search_reg == REG_R0) || (search_reg == REG_R1)) {
		/*
		 * Registers are used to pass parameters,
		 * no exchange possible
		 */
		FATAL ((__FILE__, "do_remap_fwd", "inconsistency 5a"));
		return;
	    }
	    /*lint -fallthrough */
#else /* PEEP_CALL_ALSO_VIA_TABLE */
	case op_callu:
	case op_call:
	case op_trapu:
	case op_xcall:
	    if (is_temporary_register (search_reg)
		|| is_temporary_register (replace)) {
		FATAL ((__FILE__, "do_remap_fwd", "inconsistency 6"));
		return;
	    }
	    /*lint -fallthrough */
#endif /* PEEP_CALL_ALSO_VIA_TABLE */
	default:
	    map = GET_PEEP_INFO (ip2, &StaticMemory);

	    changed = map->write | map->modified | map->updated;
	    needed = map->read | map->modified | map->used | map->updated;

	    /*
	     *  if replaceregister if modified we can do no further replacements
	     */
	    if (changed & (replacebit)) {
		replacement_forbidden = TRUE;
	    }
	    /*
	     *  Check if register is used in any indirect way
	     *  if so, replace must be of same type
	     *  (if search is ar, replace must be ar
	     *   if search is index, replace must be index)
	     */
	    if (((map->used | map->updated) & (searchbit)) != 0) {
		if (!is_same_register_type (search_reg, replace)) {
		    FATAL (
			   (__FILE__, "do_remap_fwd",
			    "inconsistency FastPeep 9"));
		}
	    }
	    /*
	     * Check for any pending replacements if replacement is forbidden
	     */
	    if (replacement_forbidden == TRUE) {
		if (needed & (searchbit)) {
		    FATAL (
			   (__FILE__, "do_remap_fwd",
			    "inconsistency FastPeep 10"));
		}
	    }
	    /*
	     * if Search is overwritten, we can end remap here
	     */
	    if (map->write & (searchbit)) {
		/*
		 * check if sources must be replaced
		 */
		if (needed & (searchbit)) {
		    replace_source_registers (ip2, search_reg, replace);
		}
		return;
	    }
	    /*
	     *  if searchregister is modified, we may only replace
	     *  if replaceregister is not used anymore
	     *  for now we simply do not replace
	     */
	    /*
	     * we do not check again if register is not used anymore
	     * makes no sense to distrust my own software completely...
	     * if (changed & (searchbit)) {
	     * FATAL ((__FILE__,"do_remap_fwd","inconsistency FastPeep 11"));
	     * }
	     */


	    /*
	     *  do all necessary exchanges of registers
	     */

	    if ((changed | needed) & (searchbit)) {
		replace_all_registers (ip2, search_reg, replace);
	    }
	    break;
	case op_label:
	    /* we wont follow jumps now */
	    if (!is_peep_phase (peep_level, PEEP_HARD_REMAP)) {
		FATAL ((__FILE__, "do_remap_fwd", "inconsistency 17"));
		return;
	    }
	    break;
	case op_line:
	    break;
	}
    }
    return;
}

/*
 * Remaps registers, eg replaces register 'search_reg' with 'replace' in the
 * following instruction sequence until 'search_reg' is overwritten.
 *
 * This routine is only to be called if replace is really possible
 * (Check with 'is_remap_possible_bwd') else FATAL will be called.
 *
 * This is not possible if:
 *
 *       'search_reg' is STACKPTR.
 *
 *       'replace' is temporary and we have a call in the sequence.
 *
 *       'search_reg'  is temporary and we have a call in the sequence.
 *
 *       An asm-statement is in the sequence.
 *
 *       'replace' is used and more replacements follow in
 *       this sequence upwards.
 *
 *       'replace' is not an address register and search_reg is used
 *       as an address register or index register in an indirect
 *       address mode.
 *
 * By branches, labels and jumps we return FALSE, because following
 * all paths and do all replacements correctly is somewhat hairy
 * (I just dont know how to do it, so I let it be).
 *
 * The sequence ends if blockbeginn is found or search_reg is overwritten.
 */
static void do_remap_bwd P3 (REG, search_reg, REG, replace, const CODE *, ip)
{
    CODE   *ip2;
    REGBITMAP changed;
    REGBITMAP needed;
    REGBITMAP replacebit = (1UL << (int) replace);
    REGBITMAP searchbit = (1UL << (int) search_reg);
    PEEPINFO *map;

#ifdef SAVE_PEEP_MEMORY
    PEEPINFO StaticMemory;

#endif /* SAVE_PEEP_MEMORY */
    if (ip == NIL_CODE) {
	FATAL ((__FILE__, "do_remap_bwd", "inconsistency 0"));
	return;
    }
    for (ip2 = ip->back; ip2 != NIL_CODE; ip2 = ip2->back) {
	switch (ip2->opcode) {
	case op_retsu:
	case op_retiu:
	    /* We are at blockbegin, so remapping is finished */
	    return;
	case op_abort:
	    /* We have already been here, so remapping is finished for this path */
	    return;
#ifdef ASM
	case op_asm:		/* really it in unknown */
	    FATAL ((__FILE__, "do_remap_bwd", "inconsistency 1"));
	    return;
#endif /* ASM */
	case op_blo:
	case op_bls:
	case op_bhi:
	case op_bhs:
	case op_beq:
	case op_bne:
	case op_blt:
	case op_ble:
	case op_bgt:
	case op_bge:
	case op_bz:
	case op_bnz:
	case op_bp:
	case op_bn:
	case op_bnn:
#if 0
	    /* may be too complicated to follow, so we let it be for now */
	    op = ip2->opcode;
	    ip2->opcode = op_abort;	/* to prevent looping */
	    result =
		is_remap_possible_bwd (search_reg, remap,
				       find_jump_with_label (ip2->src1->
							     u.offset->v.l));
	    ip2->opcode = op;
	    return result;
#endif
	    if (!is_peep_phase (peep_level, PEEP_HARD_REMAP)) {
		FATAL ((__FILE__, "do_remap_bwd", "inconsistency 2"));
		return;
	    }
	    break;
	case op_bu:
	case op_br:
	case op_bud:
	case op_brd:
	    /* We should have found a label bevore we reach here, else
	     * this is dead code, so we can swap what we want
	     */
	    return;

	case op_callu:
	case op_call:
	case op_trapu:
	case op_xcall:
	    if (is_temporary_register (replace))
		FATAL ((__FILE__, "do_remap_bwd", "inconsistency 6"));
	    if ((search_reg == STACKPTR)
		|| (search_reg == RESULT))
		FATAL ((__FILE__, "do_remap_bwd", "inconsistency 6a"));
	    if (is_temporary_register (search_reg))
		return;
	    /*lint -fallthrough */
	default:
	    map = GET_PEEP_INFO (ip2, &StaticMemory);

	    changed = map->write | map->modified | map->updated;
	    needed = map->read | map->modified | map->used | map->updated;

	    /* if search_reg is overwritten, sequence ends here
	     * replace is possible if it was not forbidden
	     * (If sequence would not end with overwrite,
	     * replacement would be possible indpendent of forbidden)
	     */
	    if (map->write & (searchbit)) {
		replace_destination_registers (ip2, search_reg, replace);
		return;
	    }
	    /*
	     *  Check if register is used in any indirect way
	     *  if so, replace must be of same type
	     *  (if search is ar, replace must be ar
	     *   if search is index, replace must be index)
	     */
	    if (((map->used | map->updated) & (searchbit)) != 0) {
		if (!is_same_register_type (search_reg, replace)) {
		    FATAL (
			   (__FILE__, "do_remap_bwd",
			    "inconsistency FastPeep 7"));
		}
	    }
	    /*
	     *  Check if replaceregister is modified in any Way
	     *  if so, replace is not possible
	     *   but there is also a register, whose value is not
	     *   used (-> we startet by an ldi search, replace
	     *            and did not found any instruction using
	     *            contents of replace, so this modifications
	     *            of replace are used nowhere)
	     *   theoretically, this should not happen :-)
	     *
	     */
	    if (((changed) & (replacebit)) != 0) {
		FATAL (
		       (__FILE__, "do_remap_bwd",
			"inconsistency FastPeep 8"));
	    }
	    /*
	     *  if contents of replace are needed here, further
	     *  replacement is not possible
	     *
	     *  theoretically I should use here (map->read | map->used)
	     *  instead of needed, but the cases map->modified and map->updated
	     *  which are also contained in needed are already handled in the
	     *  precedent statement, so no problem at all
	     */

	    if (((needed) & (replacebit)) != 0) {
		FATAL (
		       (__FILE__, "do_remap_bwd",
			"inconsistency FastPeep 8"));
	    }
	    /*
	     *  Do all the necessary registerreplacement
	     */
	    if (((needed | changed) & (searchbit)) != 0) {
		replace_all_registers (ip2, search_reg, replace);
	    }
	    break;
	case op_label:
	    /* we wont follow jumps now */
	    if (ip2->back != NIL_CODE)
		FATAL ((__FILE__, "do_remap_bwd", "inconsistency 17"));
	    return;
	case op_line:
	case op_pushnopeep:
	case op_pushfnopeep:
	    break;
	}
    }
    return;
}


/*
 * Checks if address mode is possible for parallel.
 */
static BOOL is_restricted_indirect P1 (const ADDRESS *, ap)
{
    if (ap == NIL_ADDRESS)
	return FALSE;
    switch (ap->mode) {
    case am_ind:
    case am_const_ind:
    case am_indx2:
    case am_indxs:
	return TRUE;
    case am_ainc:
    case am_adec:
    case am_preinc:
    case am_predec:
    case am_indx:
	return ap->u.offset->nodetype == en_icon &&
	    ap->u.offset->v.i >= -1 && ap->u.offset->v.i <= 1;
    default:
	break;
    }
    return FALSE;
}

/*
 * Checks if address mode is possible for parallel.
 */
static BOOL is_parallel_possible P2 (const ADDRESS *, ap1, const ADDRESS *, ap2)
{
    if ((ap1 == NIL_ADDRESS) || (ap2 == NIL_ADDRESS))
	return FALSE;
    if (!is_restricted_indirect (ap1) || !is_restricted_indirect (ap2))
	return FALSE;
    if (((ap1->mode == am_ainc) || (ap1->mode == am_adec)
	 || (ap1->mode == am_preinc) || (ap1->mode == am_predec))) {
/*        &&
 *    ((ap2->mode == am_ainc) || (ap2->mode == am_adec)
 *       || (ap2->mode == am_preinc) || (ap2->mode == am_predec))) {
 *
 *       if the first instruction is modifiing the addressregister,
 *       the second cannot use the same register for any kind
 *       of indirect addressing, because in parallelmode it 
 *       wont get the modified register, but the register before
 *       the autinc/dec
 */

	/*
	 * if both modify the same addressregister, they cannot
	 * be used in parallel
	 */
	return ap1->preg != ap2->preg;
    }
    return TRUE;
}


static CODE *code P8 (OPCODE, op, ITYPE, type, ADDRESS *, ap1, ADDRESS *, ap2,
		      ADDRESS *, ap3, ADDRESS *, ap21, ADDRESS *, ap22,
		      ADDRESS *, ap23)
{
    CODE   *newcode;
    newcode = (CODE *) xalloc ((int) sizeof (CODE));

    newcode->opcode = op;
    newcode->type = type;
    newcode->src1 = ap1;
    newcode->src2 = ap2;
    newcode->dst = ap3;
    newcode->src21 = ap21;
    newcode->src22 = ap22;
    newcode->dst2 = ap23;
#ifndef SAVE_PEEP_MEMORY
    newcode->info = NULL;
#endif /* SAVE_PEEP_MEMORY */
#ifdef DEBUG
    if (is_float_instruction (op)) {
	if (type != OP_FLOAT) {
	    FATAL ((__FILE__, "code", "problem with op_float"));
	}
    } else {
	if (type == OP_FLOAT) {
	    FATAL ((__FILE__, "code", "problem with op_int"));
	}
    }
#endif
    return newcode;
}

/*
 * Generate a code sequence into the peep list.
 */
void g_code P4 (OPCODE, op, ITYPE, type, ADDRESS *, ap1, ADDRESS *, ap2)
{
    add_peep (code
	      (op, type, ap1, NIL_ADDRESS, ap2, NIL_ADDRESS, NIL_ADDRESS,
	       NIL_ADDRESS));
}

void g_code3 P5 (OPCODE, op, ITYPE, type, ADDRESS *, ap1, ADDRESS *, ap2,
		 ADDRESS *, ap3)
{
    add_peep (code
	      (op, type, ap1, ap2, ap3, NIL_ADDRESS, NIL_ADDRESS,
	       NIL_ADDRESS));
}

/*
 * Generate a code sequence into the peep list.
 */
void g_code_parallel P8 (OPCODE, op, ITYPE, type, ADDRESS *, ap1, ADDRESS *,
			 ap2, ADDRESS *, ap3, ADDRESS *, ap21, ADDRESS *,
			 ap22, ADDRESS *, ap23)
{
    add_peep (code (op, type, ap1, ap2, ap3, ap21, ap22, ap23));
}

/*
 * Add the instruction pointed to by 'newcode' to the peep list.
 */
static void add_peep P1 (CODE *, newcode)
{
    static CODE *peep_tail;

    if (peep_head == NIL_CODE) {
	peep_head = peep_tail = newcode;
	newcode->fwd = NIL_CODE;
	newcode->back = NIL_CODE;
    } else {
	newcode->fwd = NIL_CODE;
	newcode->back = peep_tail;
	peep_tail->fwd = newcode;
	peep_tail = newcode;
    }
}

/*
 * Output all code and labels in the peep list.
 */
void flush_peep P1 (unsigned, level)
{
    register CODE *ip;
    SWITCH *sw;
    EXPR   *ep2;
    LABEL   i;

    opt3 (level);		/* do the peephole optimizations */
    for (ip = peep_head; ip != NIL_CODE; ip = ip->fwd) {
	if (ip->opcode == op_label)
	    put_label (ip->src1->u.offset->v.l);
	else
	    put_code (ip);
#ifdef DEBUG
	if (is_float_instruction (ip->opcode)) {
	    if (ip->type != OP_FLOAT) {
		FATAL ((__FILE__, "flush_peep", "problem with op_float"));
	    }
	} else {
	    if (ip->type == OP_FLOAT) {
		FATAL ((__FILE__, "flush_peep", "problem with op_int"));
	    }
	}
#endif
    }
    peep_head = NIL_CODE;
    for (sw = swtables; sw; sw = sw->next) {
	put_rseg (alignment_of_type (tp_pointer));
	put_label (sw->tablab);
	ep2 = mk_lcon ((LABEL) 0);
	/* generate the switch jump table as a series of 4-byte addresses */
	for (i = 0; i < sw->numlabs; i++) {
	    ep2->v.l = sw->labels[i];
	    put_pointer (ep2);
	}
    }
    swtables = NIL_SWITCH;
}


void init_peep P0 (void)
{
    const CHAR *str;
    int     count = 1;
    PEEP_STAGES *ptr;

#ifdef CHECK_LIST_INTEGRITY
    printf ("\n WARNING, CHECK_LIST_INTEGRY was define in peepc30.c \n");
    printf ("this enables some internal consistencychecks and uses a lot \n");
    printf
	("of runtime, disable this switch and compile the compiler again \n");
    printf
	("if You want a faster compilerversion (needed only for debugging)\n");
#endif /* CHECK_LIST_INTEGRITY */
    if (*opt_peep_sequence == 0) {
	return;
    }
    for (str = opt_peep_sequence; *str != 0; str++) {
	if (*str == '/') {
	    count++;
	}
    }
    OptimizerStages =
	(PEEP_STAGES *) galloc ((count + 2) * sizeof (*OptimizerStages));
    ptr = OptimizerStages;
    *ptr = 0;
    for (str = opt_peep_sequence; *str != 0; str++) {
	switch (*str) {
	case '0':
	    *ptr |= STANDARD;
	    break;
	case '1':
	    *ptr |= COMBINE_FWD;
	    break;
	case '2':
	    *ptr |= COMBINE_BWD;
	    break;
	case '3':
	    *ptr |= COMMUTATIVE;
	    break;
	case '4':
	    *ptr |= REMAPP;
	    break;
	case '5':
	    *ptr |= REDUNDANT;
	    break;
	case '6':
	    *ptr |= DATAFLOW;
	    break;
	case '7':
	    *ptr |= TO_3OPERAND;
	    break;
	case '/':
	    ptr++;
	    *ptr = 0;
	    if (--count <= 0) {
		FATAL ((__FILE__, "init_peep", "inconsistency"));
	    }
	    break;
	default:
	    break;
	}
    }
    *++ptr = 0;

    /* Do some consistency-check in opcodetable    */
    /* checks only size, but proved to be usefull */
    assert (((sizeof (op_flags) / sizeof (op_flags[0])) - 1) ==
	    (OP_MAX - OP_MIN));
    assert (convert_op_to_op3[op_xor] == op_xor3);
}

/*
 * Delete an instruction referenced by ip.
 */
static void peep_delete P1 (const CODE *, ip)
{
    if (ip == NIL_CODE) {
	FATAL ((__FILE__, "peep_delete", ""));
    }
    if (ip->back == NIL_CODE) {
	peep_head = ip->fwd;
	if (ip->fwd)
	    ip->fwd->back = NIL_CODE;
	next_ip = ip->fwd;
    } else {
	if ((ip->back->fwd = ip->fwd) != NIL_CODE) {
	    ip->fwd->back = ip->back;
	}
	next_ip = ip->back;
    }
    changes++;
}

/*
 * Set all peep-flags for the whole list.
 */
static void set_all_peepflags P0 (void)
{
    register CODE *ip;

    for (ip = peep_head; ip != NIL_CODE; ip = ip->fwd) {
	set_peepflags (ip);
    }
}


#ifndef SAVE_PEEP_MEMORY
/*
 * Set all peep-infos for the whole list.
 */
static void set_all_peepinfo P0 (void)
{
    register CODE *ip;

    for (ip = peep_head; ip != NIL_CODE; ip = ip->fwd) {
	attach_peepinfo (ip);
    }
}
#endif /* SAVE_PEEP_MEMORY */

#ifdef CHECK_LIST_INTEGRITY

/*
 * Check Peeplist for consistency (For debugging only).
 */
static void check_peep P0 (void)
{
    register CODE *ip;
    int     Count = 0;

    for (ip = peep_head; ip != NIL_CODE; ip = ip->fwd) {
#ifdef DEBUG
	if (is_float_instruction (ip->opcode)) {
	    if (ip->type != OP_FLOAT) {
		FATAL ((__FILE__, "flush_peep", "problem with op_float"));
	    }
	} else {
	    if (ip->type == OP_FLOAT) {
		FATAL ((__FILE__, "flush_peep", "problem with op_int"));
	    }
	}
#endif
	if (ip->fwd != NIL_CODE) {
	    if (ip->fwd->back != ip) {
		FATAL ((__FILE__, "check_peep", "list is inconsistent"));
		DPRINTF (
			 (DEBUG_PEEP, "Peeplist is inconsistent (%d)!!!\n",
			  Count++));

		/* 'repair' it to supress further errors */
		ip->fwd->back = ip;
	    }
	}
    }
}
#endif /* CHECK_LIST_INTEGRITY */


/****************************************************************
 *
 * Registerusage-analyzer
 *
 ****************************************************************/

#ifdef REGISTER_FLOW_ANALYZER
static void attach_registerusage P0 (void)
{
    int     LabelChanges, i;
    LABEL   label, switchlabel;
    register CODE *ip;
    register CODE *target;
    CODE   *tail;
    REGBITMAP RegUsage = 0;
    REGBITMAP RegFixed = 0;
    REGBITMAP UsedAnywhere = 0;
    struct swtab *sw;
    BOOL    found;

    ip = peep_head;
    if (ip == NIL_CODE) {
	return;
    }
    while (ip->fwd != NIL_CODE) {
	ip = ip->fwd;
    }
    tail = ip;
    do {
	LabelChanges = 0;
	RegUsage = 0;
	for (ip = tail; ip != NIL_CODE; ip = ip->back) {
	    switch (ip->opcode) {

	    case op_blo:
	    case op_bls:
	    case op_bhi:
	    case op_bhs:
	    case op_beq:
	    case op_bne:
	    case op_blt:
	    case op_ble:
	    case op_bgt:
	    case op_bge:
	    case op_bz:
	    case op_bnz:
	    case op_bp:
	    case op_bn:
	    case op_bnn:
		if ((ip->info->used_later & RegUsage) != RegUsage) {
		    ip->info->used_later |= RegUsage;
		LabelChanges++}
		RegUsage = ip->info->used_later;
		RegFixed = RegUsage;
		ip->info->fixed = RegFixed;
		break;

	    case op_label:
		ip->info->used_later = RegUsage;
		ip->info->fixed = RegFixed;
		RegFixed = RegUsage;
		label = ip->src1->u.offset->v.l;
		for (target = peep_head; target != NIL_CODE;
		     target = target->fwd) {
		    if (is_label_used (target->src1, label)) {
			if ((target->info->used_later & RegUsage) != RegUsage) {
			    target->info->used_later |= RegUsage;
			LabelChanges++}
		    }
		}
		for (sw = swtables; sw != NIL_SWITCH; sw = sw->next) {
		    switchlabel = 0;
		    for (i = 0; i < sw->numlabs; i++) {
			if (sw->labels[i] == label)
			    switchlabel = sw->tablab;
		    }
		    if (switchlabel != 0) {
			found = FALSE;
			for (target = peep_head; target != NIL_CODE;
			     target = target->fwd) {
			    if ((target->opcode == op_bu)
				&& (target->src1->u.offset->v.l ==
				    switchlabel)) {
				if ((target->info->used_later & RegUsage) !=
				    RegUsage) {
				    target->info->used_later |= RegUsage;
				LabelChanges++}
				found = TRUE;
			    }
			}
			if (found == FALSE) {
			    FATAL (
				   (__FILE__, "attach_registerusage",
				    "bu for switch not found"));
			}
		    }
		}
		break;

#ifdef ASM
	    case op_asm:
		ip->info->used_later = RegUsage;
		RegUsage = ALL_TEMPORARYS;
		ip->info->fixed = RegFixed;
		RegFixed = 0;
		break;
#endif /* ASM */

	    case op_br:
	    case op_bu:
#error must do something with registerdestination of indirect jumps
		RegUsage = ip->info->used_later;
		RegFixed = RegUsage;
		ip->info->fixed = RegFixed;
		break;

	    case op_retiu:
		ip->info->used_later = RegUsage;
		ip->info->fixed = RegFixed;
		RegUsage = (1L << REG_SP);
		RegFixed = RegUsage;
		break;

	    case op_retsu:
		ip->info->used_later = RegUsage;
		ip->info->fixed = RegFixed;
		RegUsage = (1L << REG_R0) | (1L << REG_SP);
		RegFixed = RegUsage;
		break;

	    case op_call:
	    case op_trapu:
	    case op_callu:
		ip->info->used_later = RegUsage;
		ip->info->fixed = RegFixed;
		RegUsage &= ~ALL_TEMPORARYS;
		RegUsage |= (1L << REG_SP);
		RegFixed &= ~ALL_TEMPORARYS;
		RegFixed |= (1L << REG_SP);
		break;

	    case op_xcall:
		ip->info->used_later = RegUsage;
		ip->info->fixed = RegFixed;
		RegUsage &= ~ALL_TEMPORARYS;
		RegUsage |= (1L << REG_R0) | (1L << REG_R1) | (1L << REG_SP);
		RegFixed &= ~ALL_TEMPORARYS;
		RegFixed |= (1L << REG_R0) | (1L << REG_R1) | (1L << REG_SP);
		break;

	    default:
		ip->info->used_later = RegUsage;
		ip->info->fixed = RegFixed;
		RegFixed &= ~ip->info->write;
		RegUsage &= ~ip->info->write;
		RegUsage |= ip->info->read;
		RegUsage |= ip->info->used;
		RegUsage |= ip->info->modified;
		RegUsage |= ip->info->updated;
		break;

	    }
	}
    } while (LabelChanges != 0);
}

/*
 * Prints out the register usage, for debugging-purpose only.
 */

static void print_registerusage P0 (void)
{
    REG     reg;
    register CODE *ip;
    REGBITMAP UsedAnywhere = 0;

    if (is_debugging (DEBUG_PEEP)) {
	for (ip = peep_head; ip != NIL_CODE; ip = ip->fwd) {
	    if (ip->opcode == op_label)
		put_label (ip->src1->u.offset->v.l);
	    else
		put_code (ip);
	    for (reg = REG_R0; reg <= MAX_REG; reg++) {
		if ((ip->info->used_later & (1L << reg)) != 0)
		    if ((ip->info->fixed & (1L << reg)) != 0)
			oprintf ("F");
		    else
			oprintf ("|");
		else if ((ip->info->fixed & (1L << reg)) != 0)
		    oprintf ("=");
		else
		    oprintf ("-");
	    }
	    UsedAnywhere |= ip->info->used_later;
	}
	oprintf ("\n Totalusage:\n");
	for (reg = REG_R0; reg <= MAX_REG; reg++) {
	    if ((UsedAnywhere & (1L << reg)) != 0)
		oprintf ("|");
	    else
		oprintf ("-");
	}
    }
}

#if 0
static void reassign_registers P0 (void)
{
    REG     reg;
    REG     last_address = REG_AR0;
    REG     last_index = REG_IR0;
    REG     last_data = REG_R0;
    register CODE *ip;
    register CODE *target;
    REGBITMAP RegUsage = 0;
    REGBITMAP RegFixed = 0;
    BOOL    found;

    for (ip = peep_head; ip != NIL_CODE; ip = ip->fwd) {
	if ((ip->info->write != 0)
	    && ((ip->infp->fixed & ip->info->write) == 0)
	    && ((ip->info->write & ALL_TEMPORARYS) == 0)) {
	    switch (ip->opcode) {
	    default:		/* Do some very clever things with the registers */
		break;
	    }
	}
    }
}
#endif

static void peep_registerusage P0 (void)
{
    attach_registerusage ();
    print_registerusage ();
}

#endif /* REGISTER_FLOW_ANALYZER */

/****************************************************************
 *
 * End of new Registerusage-analyzer            
 *
 ****************************************************************/


/*
 * Delete branches to the following statement
 */
static void peep_bxx P1 (const CODE *, ip)
{
    CODE   *p = ip->fwd;
    LABEL   label;
    OPCODE  new_base_opcode;
    int     count;

    if (ip->src1->mode != am_immed)
	return;
#if 1
    label = ip->src1->u.offset->v.l;
    /* delete branches to the following statement */
    while (p != NIL_CODE && ((p->opcode == op_label)
			     || (p->opcode == op_line))) {
	if ((p->opcode == op_label) && (p->src1->u.offset->v.l == label)) {
	    peep_delete (ip);
	    IncPeepStatistic (bxx_to_next_line);
	    DPRINTF ((DEBUG_PEEP, " Peep eliminated bxx to next line\n"));
	    return;
	}
	p = p->fwd;
    }
#endif
#if 1
    /*
     * optimizes conditional branch over a bra.
     */
    for (p = ip->fwd; (p != NIL_CODE) && (p->opcode == op_line); p = p->fwd) {
    }
    if (p == NIL_CODE)
	return;
    if (p->opcode == op_br) {
	/* peep_uctran increases the 'hit' probability */
	peep_uctran (p);
	if (p->fwd == NIL_CODE)
	    return;
	if (p->fwd->opcode == op_label &&
	    ip->src1->u.offset->v.l == p->fwd->src1->u.offset->v.l) {
	    p->opcode = revcond[(int) (ip->opcode - op_bu) - 1];
	    peep_delete (ip);
	    IncPeepStatistic (bxx_over_br);
	    DPRINTF ((DEBUG_PEEP, " Peep removed bxx over bra\n"));
	}
    }
#endif
#if 1
    if (opt_branches < OPT_LEVEL1) {
	return;
    }
    /* check if no more than 3 ldi/ldf follows immediately
     * behind this branch
     * if the destination is behind this ldi/ldf we may
     * replace the branch with conditional loads
     * is not applicable with autoinc/dec
     *
     *      tstb r1,   r1     becomes   tstb   r1,   r1
     *      bne  Lxxx
     *      ldi  *ar2, r1               ldieq  *ar2, r1
     *      ldi  3,    ar0              ldieq  3,    ar0
     *      ldf  2.0,  r2               ldfeq  2.0,  r2
     * Lxxx addi r1,   ar0         Lxxx addi   r1,   ar0
     *
     * Note:
     * with some more work, further optimization is possible:
     * (but is currently not implemented)
     *
     *      tstb r1,   r1  could become tstb   r1,   r1
     *      bne  Lxxx
     *      ldi  *ar2, r1               ldieq  *ar2, r1
     *      br   Lxxy
     * Lxxx ldi  3,    ar0              ldine  3,    ar0
     *      ldf  2.0,  r2               ldfne  2.0,  r2
     * Lxxy addi r1,   ar0         Lxxy addi   r1,   ar0
     *
     *
     *  Lxxx is used nowhere else
     */
    p = ip->fwd;
    count = 3;
    while ((p != NIL_CODE) && (count > 0) && (p->opcode != op_label)) {
	if (p->opcode == op_line) {
	    p = p->fwd;
	} else if ((p->opcode == op_ldi) || (p->opcode == op_ldf)) {
	    switch (p->src1->mode) {
	    case am_ainc:
	    case am_adec:
	    case am_predec:
	    case am_preinc:
		/* ainc/dec cannot be optimized in this way */
		count = 0;
		break;
	    default:
		p = p->fwd;
		count--;
		break;
	    }
	} else {
	    count = 0;
	}
    }
    if (count > 0) {

	/* check if the neded label follows */
	while (p != NIL_CODE && ((p->opcode == op_label)
				 || (p->opcode == op_line))) {
	    if ((p->opcode == op_label) && (p->src1->u.offset->v.l == label)) {

		/* we found the right label, so we can do the replacements */
		p = ip->fwd;
		while ((p != NIL_CODE) && (p->opcode != op_label)) {
		    if (p->opcode == op_line) {
			p = p->fwd;
		    } else if ((p->opcode == op_ldi) || (p->opcode == op_ldf)) {
			new_base_opcode =
			    (p->opcode == op_ldi) ? op_ldiu : op_ldfu;
			p->opcode =
			    (OPCODE) ((revcond[(ip->opcode - op_bu) - 1] - op_bu) + new_base_opcode);
			/*
			 * Not forgetting to update infofield in instruction !!!
			 */
			Update_Peep_Info (p);

			p = p->fwd;
		    } else {
			FATAL ((__FILE__, "peep_bxx", "inconsistency 0"));
		    }
		}
		peep_delete (ip);
		IncPeepStatistic (bxx_to_ldcond);
		DPRINTF ((DEBUG_PEEP, " Peep converted bxx to ldcond\n"));
		return;
	    }
	    p = p->fwd;
	}
    }
#endif
}

/* ensure we have a label to branch to (create if needed) */
static void insert_label P2 (const CODE *, ip, CODE *, target)
{
    if (target->opcode == op_label) {
	ip->src1->u.offset->v.l = target->src1->u.offset->v.l;
    } else if (target->back->opcode == op_label) {
	ip->src1->u.offset->v.l = target->back->src1->u.offset->v.l;
    } else {
	CODE   *p;

	p =
	    code (op_label, OP_NOTYPE, mk_label (nextlabel), NIL_ADDRESS,
		  NIL_ADDRESS, NIL_ADDRESS, NIL_ADDRESS, NIL_ADDRESS);
	p->fwd = target;
	p->back = target->back;
	target->back->fwd = p;
	target->back = p;
	ip->src1->u.offset->v.l = nextlabel++;
	/*
	 * Not forgetting to update infofield in instruction !!!
	 */
	attach_peepinfo (p);
    }
}


/* do optimisations on br-instructions */
static void peep_br P1 (CODE *, ip)
{
    CODE   *p = ip->fwd;
    LABEL   label;
    CODE   *target;
    int     count;

    if (ip->src1->mode != am_immed)
	return;

    label = ip->src1->u.offset->v.l;
    /* delete branches to the following statement */
    while (p != NIL_CODE && ((p->opcode == op_label)
			     || (p->opcode == op_line))) {
	if ((p->opcode == op_label) && (p->src1->u.offset->v.l == label)) {
	    peep_delete (ip);
	    IncPeepStatistic (br_to_next_line);
	    DPRINTF ((DEBUG_PEEP, " Peep eliminated br to next line\n"));
	    return;
	}
	p = p->fwd;
    }

    if (opt_branches < OPT_LEVEL1) {
	peep_uctran (ip);
	return;
    }
    target = find_label (label);
    /* we should have found it */
    if (target == NIL_CODE)
	FATAL ((__FILE__, "peep_bra", "target == 0"));

    /* Space optimisation:
     * if the code before the target of the branch is itself a branch
     * then we can move the destination block of code to eliminate the branch
     */
    p = target->back;
    while ((p != NIL_CODE) && (p->opcode == op_line)) {
	p = p->back;
    };
    if (p != NIL_CODE && ((p->opcode == op_br)
			  || (p->opcode == op_bu)
			  || (p->opcode == op_retiu)
			  || (p->opcode == op_retsu))) {
	p = block_end (target);
	if (p != NIL_CODE && p != ip) {
	    if (ip->fwd)
		ip->fwd->back = p;
	    if (p->fwd)
		p->fwd->back = target->back;
	    target->back->fwd = p->fwd;
	    p->fwd = ip->fwd;
	    target->back = ip;
	    ip->fwd = target;
	    IncPeepStatistic (br_block_move);
	    DPRINTF ((DEBUG_PEEP, " Peep eliminated br trough blockmove\n"));
	    peep_delete (ip);
	    return;
	}
    }
    /*if (!optimize_option) {
     *  peep_uctran (ip);
     *  return;
     *}
     */

    if (opt_branches < OPT_LEVEL2) {
	peep_uctran (ip);
	return;
    }
#if 1
    /* Space optimisation:
     * if the code before the branch instruction is the same as the
     * instruction before the label the branch can be moved back an
     * instruction.
     */

    p = ip;
    previous_ignore_line (p);
    previous_real_instruction (target);
    if (p == target)
	return;
    /* now skip back over identical instruction sequences */
    count = 0;
    while (is_same_instruction (p, target)) {
	peep_delete (p);
	previous_ignore_line (p);
	previous_real_instruction (target);
	count++;
    }
    if (count > 0) {
	AddPeepStatistic (br_branch_move, count);
	DPRINTF ((DEBUG_PEEP, " Peep moved branch back\n"));
    }
    insert_label (ip, target->fwd);
    label = ip->src1->u.offset->v.l;

    if (opt_branches < OPT_LEVEL3) {
	peep_uctran (ip);
	return;
    }
    /*  Space optimisation:
     *  Optimise for the situation where two branches to the same
     *  target label have identical instruction sequences
     *  leading up to the branch.   We can instead eliminate one
     *  of these instruction sequences by branching to the other one.
     */
    for (target = peep_head; target != NIL_CODE; target = target->fwd) {
	if ((target != ip) &&
	    (target->opcode == op_br) &&
	    (target->src1->u.offset->v.l == label)) {

	    CODE   *t = target;

	    p = ip;
	    previous_ignore_line (p);
	    previous_real_instruction (t);
	    count = 0;
	    while (is_same_instruction (p, t)) {
		peep_delete (p);
		previous_ignore_line (p);
		previous_real_instruction (t);
		count++;
	    }
	    if (count != 0) {
		AddPeepStatistic (br_commoned, count);
		DPRINTF (
			 (DEBUG_PEEP,
			  " Peep commoned up code before branch\n"));
		insert_label (ip, t->fwd);
		break;
	    }
	}
    }
#endif
    peep_uctran (ip);
}



static void peep_brdelayed P1 (CODE *, ip)
{
    CODE   *p, *next;
    CODE   *newcode;
    CODE   *target;
    CODE   *insert;
    LABEL   label;
    int     count, i;
    BOOL    notdead;
    int     memorywaste = 0;
    int     cyclewaste = 0;

    /* Speed optimisation:
     * if the code before the branch contains no labels and no branches or
     * calls we use delayed branches (the next three instructions after
     * the branch are executed before the branch really occurs, therfore
     * the next three instructiuns cannot be any kind of branches, labels,
     * asm, calls and jumps
     * if the branch cannot be moved for three locations, we must fill
     * in nops
     * this replacement should only be done in the last pass of the optimizer
     * e.g when all unneeded labels are eliminated and all codemoves are done
     */
    for (p = ip->back, count = 0; (p != NIL_CODE) && (count < 3); p = p->back) {
	if ((p->opcode == op_label)
	    || (p->opcode == op_call)
	    || (p->opcode == op_xcall)
	    || (p->opcode == op_callu)
	    || (p->opcode == op_trapu)
#ifdef ASM
	    || (p->opcode == op_asm)
#endif /* ASM */
	    || (anybranch (p))
	    || (p->opcode == op_retiu)
	    || (p->opcode == op_retsu)) {
	    break;
	} else {
	    if (p->opcode != op_line) {
		count++;
	    }
	}
    }
    insert = p;
    label = ip->src1->u.offset->v.l;
    target = find_label (label);
    if (target == NIL_CODE) {
	FATAL ((__FILE__, "peep_brdelayed", "target == 0"));
    }
    /*
     * check how many instructions we may take from
     * target
     */
    i = count;
    for (p = target->fwd; (p != NIL_CODE) && (i < 3); p = p->fwd) {
	if ((p->opcode == op_call)
	    || (p->opcode == op_xcall)
	    || (p->opcode == op_callu)
	    || (p->opcode == op_trapu)
#ifdef ASM
	    || (p->opcode == op_asm)
#endif /* ASM */
	    || (anybranch (p))
	    || (p->opcode == op_retiu)
	    || (p->opcode == op_retsu)) {
	    break;
	} else if (p->opcode != op_label) {
	    if (p->opcode != op_line) {
		i++;
	    }
	}
    }
    /* do replace only if at least (opt_delayed_branches) number
     * of valid instructions will follow in the next 3 instructions
     * after the branch
     * (or not more as (3-opt_delayed_branches) nops must be included
     * this allows some balancing between speed and codesize
     */
    if (i >= opt_delayed_branches) {
#ifdef ASM
	/*  AddPeepStatistic(brd_memory,3-count);
	 * AddPeepStatistic(brd_cycles,count);
	 */
	newcode =
	    code (op_asm, OP_INT, mk_branchcomment (), NIL_ADDRESS,
		  NIL_ADDRESS, NIL_ADDRESS, NIL_ADDRESS, NIL_ADDRESS);
	newcode->back = ip;
	newcode->fwd = ip->fwd;
	if (ip->fwd != NIL_CODE) {
	    ip->fwd->back = newcode;
	}
	ip->fwd = newcode;
	/*
	 * Not forgetting to update infofield in instruction !!!
	 */
	attach_peepinfo (newcode);
#endif /* ASM */
	/*
	 * Try to move some instructions from the branch-target
	 * behind the branch-instruction
	 * and move the destlabel to the next instruction
	 *
	 */
#if 1
	if (count < 3) {
	    notdead = FALSE;
	    /*     label = ip->src1->u.offset->v.l;
	     *    target = find_label(label);
	     *    if (target == NIL_CODE) {
	     *      FATAL ((__FILE__,"peep_brdelayed","target == 0"));
	     *    }
	     */
	    if (label_references (target) > 1) {
		notdead = TRUE;
	    } else {
		peep_delete (target);
	    }
	    newcode = target;
	    previous_ignore_line (newcode);
	    if (!((newcode == NIL_CODE) || (newcode->opcode == op_br)
		  || (newcode->opcode == op_bu)
		  || (newcode->opcode == op_retiu)
		  || (newcode->opcode == op_retsu))) {
		notdead = TRUE;
	    }
	    p = target->fwd;
	    while ((p != NIL_CODE) && (count < 3)) {
		next = p->fwd;
		if ((p->opcode == op_call)
		    || (p->opcode == op_xcall)
		    || (p->opcode == op_callu)
		    || (p->opcode == op_trapu)
#ifdef ASM
		    || (p->opcode == op_asm)
#endif /* ASM */
		    || (anybranch (p))
		    || (p->opcode == op_retiu)
		    || (p->opcode == op_retsu)) {
		    next = p;	/* just to be sure that p points
				 * to this instruction at end of for
				 */
		    break;
		} else if (p->opcode == op_label) {
		    notdead = TRUE;
		} else {
		    if (p->opcode != op_line) {
			if (notdead == FALSE) {
			    peep_delete (p);
			    newcode = p;
			} else {
			    newcode =
				code (p->opcode, p->type, p->src1, p->src2,
				      p->dst, p->src21, p->src22, p->dst2);
			    memorywaste++;
			}
			/* insert before ip (branch) */
			newcode->fwd = ip;
			newcode->back = ip->back;
			ip->back->fwd = newcode;
			ip->back = newcode;
			/*
			 * Not forgetting to update infofield in instruction !!!
			 */
			attach_peepinfo (newcode);
			count++;
		    }
		}
		p = next;
	    }
	    insert_label (ip, p);
	}
#endif
	while (count < 3) {
	    newcode =
		code (op_nop, OP_INT, NIL_ADDRESS, NIL_ADDRESS, NIL_ADDRESS,
		      NIL_ADDRESS, NIL_ADDRESS, NIL_ADDRESS);
	    newcode->back = ip;
	    newcode->fwd = ip->fwd;
	    ip->fwd->back = newcode;
	    ip->fwd = newcode;
	    /*
	     * Not forgetting to update infofield in instruction !!!
	     */
	    attach_peepinfo (newcode);
	    count++;
	    memorywaste++;
	    cyclewaste++;
	}

	AddPeepStatistic (brd_memory, memorywaste);
	AddPeepStatistic (brd_cycles, 3 - cyclewaste);
	IncPeepStatistic (brd_optimized);
	DPRINTF ((DEBUG_PEEP, " Peep added delayed branch\n"));
	peep_delete (ip);
	if (insert == NIL_CODE) {
	    peep_head->back = ip;
	    ip->fwd = peep_head;
	    ip->back = NIL_CODE;
	    peep_head = ip;
	} else {
	    insert->fwd->back = ip;
	    ip->fwd = insert->fwd;
	    ip->back = insert;
	    insert->fwd = ip;
	}
	ip->opcode = op_brd;
    }
    return;
}

/*
 * Peephole optimization for stf instructions.
 */
static void peep_parallel_stf P1 (CODE *, ip)
{
    CODE   *ip2;

    /* replace following sequence with their parallel aequivalent:
     *          ldf    *ARx, Rm
     *          stf    Rn, *ARy   Rm != Rn
     *                            ARx != ARy if both are modified (++/--)
     *  or
     *          stf    Rn, *ARy
     *          stf    Rm, *ARx   ARx != ARy if both are modified (++/--)
     *
     *   *ARxy may be:
     *
     *      *ARxy *+ARxy(1)   *-ARxy(1)   *++ARxy(1)   *--ARxy(1)
     *            *+ARxy(IRn) *-ARxy(IRn) *++ARxy(IRn) *--ARxy(IRn)
     */

    /*
     * go back to previous valid instruction
     */
    ip2 = ip;
    do {
	ip2 = ip2->back;
    } while ((ip2 != NIL_CODE) && (ip2->opcode == op_line));

    if ((ip2 != NIL_CODE) && (ip2->opcode == op_ldf) &&
	is_parallel_possible (ip2->src1, ip->dst) &&
	(ip->src1->mode == am_freg) &&
	(ip2->dst->mode == am_freg) && (ip->src1->preg != ip2->dst->preg)) {
	ip2->opcode = op_ldf_stf;
	ip2->src21 = ip->src1;
	ip2->dst2 = ip->dst;
	/*
	 * Not forgetting to update infofield in instruction !!!
	 */
	Update_Peep_Info (ip2);

	peep_delete (ip);
	IncPeepStatistic (par_ldf_stf);
	DPRINTF (
		 (DEBUG_PEEP,
		  " Peep parallelized ldf *arx,ry stf rx,*ary\n"));
	return;
    }
    if ((ip2 != NIL_CODE) && (ip2->opcode == op_stf) &&
	is_parallel_possible (ip2->dst, ip->dst) &&
	(ip->src1->mode == am_freg) && (ip2->src1->mode == am_freg)) {
	ip2->opcode = op_stf_stf;
	ip2->src21 = ip->src1;
	ip2->dst2 = ip->dst;
	/*
	 * Not forgetting to update infofield in instruction !!!
	 */
	Update_Peep_Info (ip2);

	peep_delete (ip);
	IncPeepStatistic (par_stf_stf);
	DPRINTF (
		 (DEBUG_PEEP,
		  " Peep parallelized stf ry,*arx stf rx,*ary\n"));
	return;
    }
}

/*
 * Peephole optimization for ldf instructions.
 */
static void peep_parallel_ldf P1 (CODE *, ip)
{
    CODE   *ip2;

    /* replace following sequence with their parallel aequivalent:
     *          stf    Rn, *ARy
     *          ldf    *ARx, Rm   ARx != ARy if both are modified (++/--)
     *          Above optimization can only be used if we are
     *          shure that ARx and ARy don't point to the same
     *          address, and since this if difficult to tell we don't
     *          otpimize this case
     *          (The only case we can be shure is, when ar and ary are the
     *           same register, but have different offsets)
     *  or
     *          ldf    Rn, *ARy
     *          ldf    Rm, *ARx   ARx != ARy if both are modified (++/--)
     *
     *   *ARxy may be:
     *
     *      *ARxy *+ARxy(1)   *-ARxy(1)   *++ARxy(1)   *--ARxy(1)
     *            *+ARxy(IRn) *-ARxy(IRn) *++ARxy(IRn) *--ARxy(IRn)
     */

    /*
     * go back to previous valid instruction
     */
    ip2 = ip;
    do {
	ip2 = ip2->back;
    } while ((ip2 != NIL_CODE) && (ip2->opcode == op_line));

    if (is_peep_phase (peep_level, PEEP_PARALLEL_ALL)) {
	if ((ip2 != NIL_CODE) && (ip2->opcode == op_stf) &&
	    is_parallel_possible (ip->src1, ip2->dst) &&
	    (ip->dst->mode == am_freg) && (ip2->src1->mode == am_freg) &&
#ifdef PEEP_REDUNDANT_STI
	    (is_memoryaccess_independent (ip->src1, ip2->dst))) {
#else
	    (ip->src1->preg == ip2->dst->preg)) {
#endif
	    ip2->opcode = op_ldf_stf;
	    ip2->src21 = ip2->src1;
	    ip2->dst2 = ip2->dst;
	    ip2->src1 = ip->src1;
	    ip2->dst = ip->dst;
	    /*
	     * Not forgetting to update infofield in instruction !!!
	     */
	    Update_Peep_Info (ip2);

	    peep_delete (ip);
	    IncPeepStatistic (par_ldf_stf);
	    DPRINTF (
		     (DEBUG_PEEP,
		      " Peep parallelized ldf *arx,ry stf rx,*ary\n"));
	    return;
	}
    }
    if ((ip2 != NIL_CODE) && (ip2->opcode == op_ldf) &&
	is_parallel_possible (ip2->src1, ip->src1) &&
	(ip->dst->mode == am_freg) &&
	(ip2->dst->mode == am_freg) && (ip2->src1->preg != ip->dst->preg)) {
	ip2->opcode = op_ldf_ldf;
	ip2->src21 = ip->src1;
	ip2->dst2 = ip->dst;
	/*
	 * Not forgetting to update infofield in instruction !!!
	 */
	Update_Peep_Info (ip2);

	peep_delete (ip);
	IncPeepStatistic (par_ldf_ldf);
	DPRINTF (
		 (DEBUG_PEEP,
		  " Peep parallelized ldf *arx,ry ldf *ary, rx\n"));
	return;
    }
}

/*
 * Peephole optimization for sti instructions.
 */
static void peep_parallel_sti P1 (CODE *, ip)
{
    CODE   *ip2;

    /* replace following sequence with their parallel aequivalent:
     *          ldi    *ARx, Rm
     *          sti    Rn, *ARy   Rm != Rn
     *                            ARx != ARy if both are modified (++/--)
     *  or
     *          sti    Rn, *ARy
     *          sti    Rm, *ARx   ARx != ARy if both are modified (++/--)
     *
     *   *ARxy may be:
     *
     *      *ARxy *+ARxy(1)   *-ARxy(1)   *++ARxy(1)   *--ARxy(1)
     *            *+ARxy(IRn) *-ARxy(IRn) *++ARxy(IRn) *--ARxy(IRn)
     */

    /*
     * go back to previous valid instruction
     */
    ip2 = ip;
    do {
	ip2 = ip2->back;
    } while ((ip2 != NIL_CODE) && (ip2->opcode == op_line));

    if ((ip2 != NIL_CODE) && (ip2->opcode == op_ldi) &&
	is_parallel_possible (ip2->src1, ip->dst) &&
	(ip->src1->mode == am_dreg) &&
	(ip2->dst->mode == am_dreg) && (ip->src1->preg != ip2->dst->preg)) {
	ip2->opcode = op_ldi_sti;
	ip2->src21 = ip->src1;
	ip2->dst2 = ip->dst;
	/*
	 * Not forgetting to update infofield in instruction !!!
	 */
	Update_Peep_Info (ip2);

	peep_delete (ip);
	IncPeepStatistic (par_ldi_sti);
	DPRINTF (
		 (DEBUG_PEEP,
		  " Peep parallelized ldi *arx,ry sti rx,*ary\n"));
	return;
    }
    if ((ip2 != NIL_CODE) && (ip2->opcode == op_sti) &&
	is_parallel_possible (ip2->dst, ip->dst) &&
	(ip->src1->mode == am_dreg) && (ip2->src1->mode == am_dreg)) {
	ip2->opcode = op_sti_sti;
	ip2->src21 = ip->src1;
	ip2->dst2 = ip->dst;
	/*
	 * Not forgetting to update infofield in instruction !!!
	 */
	Update_Peep_Info (ip2);

	peep_delete (ip);
	IncPeepStatistic (par_sti_sti);
	DPRINTF (
		 (DEBUG_PEEP,
		  " Peep parallelized sti ry,*arx sti rx,*ary\n"));
	return;
    }
}

/*
 * Peephole optimization for ldi instructions.
 */
static void peep_parallel_ldi P1 (CODE *, ip)
{
    CODE   *ip2;

    /* replace following sequence with their parallel aequivalent:
     *          sti    Rn, *ARy
     *          ldi    *ARx, Rm   ARx != ARy if both are modified (++/--)
     *          Above optimization can only be used if we are
     *          shure that ARx and ARy don't point to the same
     *          address, and since this if difficult to tell we don't
     *          otpimize this case
     *          (The only case we can be shure is, when ar and ary are the
     *           same register, but have different offsets)
     *  or
     *          ldi    Rn, *ARy
     *          ldi    Rm, *ARx   ARx != ARy if both are modified (++/--)
     *
     *   *ARxy may be:
     *
     *      *ARxy *+ARxy(1)   *-ARxy(1)   *++ARxy(1)   *--ARxy(1)
     *            *+ARxy(IRn) *-ARxy(IRn) *++ARxy(IRn) *--ARxy(IRn)
     */

    /*
     * go back to previous valid instruction
     */
    ip2 = ip;
    do {
	ip2 = ip2->back;
    } while ((ip2 != NIL_CODE) && (ip2->opcode == op_line));


    if (is_peep_phase (peep_level, PEEP_PARALLEL_ALL)) {
	if ((ip2 != NIL_CODE) && (ip2->opcode == op_sti) &&
	    is_parallel_possible (ip->src1, ip2->dst) &&
	    (ip->dst->mode == am_dreg) && (ip2->src1->mode == am_dreg) &&
#ifdef PEEP_REDUNDANT_STI
	    (is_memoryaccess_independent (ip->src1, ip2->dst))) {
#else
	    (ip->src1->preg == ip2->dst->preg)) {
#endif
	    ip2->opcode = op_ldi_sti;
	    ip2->src21 = ip2->src1;
	    ip2->dst2 = ip2->dst;
	    ip2->src1 = ip->src1;
	    ip2->dst = ip->dst;
	    /*
	     * Not forgetting to update infofield in instruction !!!
	     */
	    Update_Peep_Info (ip2);

	    peep_delete (ip);
	    IncPeepStatistic (par_ldi_sti);
	    DPRINTF (
		     (DEBUG_PEEP,
		      " Peep parallelized ldi *arx,ry sti rx,*ary\n"));
	    return;
	}
    }
    if ((ip2 != NIL_CODE) && (ip2->opcode == op_ldi) &&
	is_parallel_possible (ip2->src1, ip->src1) &&
	(ip->dst->mode == am_dreg) &&
	(ip2->dst->mode == am_dreg) && (ip2->src1->preg != ip->dst->preg)) {
	ip2->opcode = op_ldi_ldi;
	ip2->src21 = ip->src1;
	ip2->dst2 = ip->dst;
	/*
	 * Not forgetting to update infofield in instruction !!!
	 */
	Update_Peep_Info (ip2);

	peep_delete (ip);
	IncPeepStatistic (par_ldi_ldi);
	DPRINTF (
		 (DEBUG_PEEP,
		  " Peep parallelized ldi *arx,ry ldi *ary, rx\n"));
	return;
    }
}

/*
 * Peephole optimization for remapping of registers.
 */
static void peep_remap P1 (const CODE *, ip)
{
    /*
     * assumed we are on a ldi/ldf instruction
     *
     * if a sequence like this is found beforehead:
     *
     *  ldi   X,rx (dest overwritten) Overwritten marks end of sequence
     *  addi  Y,rx (dest modified)
     *  ldi  rx,Q  (used)
     *  mpyi  Z,rx (dest modified)
     *  ldi  rx,ry         <-ip <<<<<<<<<<<
     *  .
     *
     * or if a sequence like this is found after ldi/ldf:
     *
     *  ldi  ry,rx         <-ip <<<<<<<<<<<
     *  addi Y, rx (dest modified)
     *  mpyi Z, rx (dest modified)
     *  ldi  rx,Q  (used)
     *  ldi  G, rx (dest overwritten) Overwritten marks end of sequence
     *  .
     *
     *  and in the following statements ry is not used
     *  we delete the ldi instruction and replace rx with
     *  ry in this sequence
     */
    if (is_am_register (ip->src1)
	&& is_am_register (ip->dst)) {
	if (!is_value_used (ip->src1->preg, ip)) {
	    /* following statements do not use value, so we may try to
	     * remap the whole thing
	     */
	    if (is_remap_possible_bwd (ip->src1->preg, ip->dst->preg, ip)) {
		do_remap_bwd (ip->src1->preg, ip->dst->preg, ip);
		peep_delete (ip);
		IncPeepStatistic (remap_register_bwd);
		DPRINTF ((DEBUG_PEEP, " Peep remapped register backwards\n"));
		return;
	    }
	}
	/*
	 * backward remapping was not possible, so try forward remapping
	 */
	if (is_remap_possible_fwd (ip->dst->preg, ip->src1->preg, ip)) {
	    do_remap_fwd (ip->dst->preg, ip->src1->preg, ip);
	    peep_delete (ip);
	    IncPeepStatistic (remap_register_fwd);
	    DPRINTF ((DEBUG_PEEP, " Peep remapped register forward\n"));
	    return;
	}
    }
    /*
     * if destination of ldi is never used again and
     * source does not modify any ar (am_ainc/am_adec)
     * we can delete this instruction
     *
     */
    if ((ip->src1->mode != am_ainc)
	&& (ip->src1->mode != am_adec)
	&& (ip->src1->mode != am_preinc)
	&& (ip->src1->mode != am_predec)) {
	if (!is_value_used (ip->dst->preg, ip)) {
	    peep_delete (ip);
	    IncPeepStatistic (superfluous_load);
	    DPRINTF (
		     (DEBUG_PEEP,
		      " Peep removed load with never used destination\n"));
	    return;
	}
    }
}

#ifdef PEEP_REDUNDANT_STI
/*
 *  returns true if we are sure that ap1 and ap2 do not
 * refer to the same memory-location
 * of that we can be sure when
 * ap1 = am_direct and ap2 = am_direct and the address is not the same
 * ap1 = am_direct and ap2 refers to stackframe (am_ind, indx, indx2 and preg = frameptr)
 * ap2 = am_direct and ap1 refers to stackframe (am_ind, indx, indx2 and preg = frameptr)
 * ap1 and ap2 both refer to stackframe, but do not have the same offset
 * ap1 and ap2 both are indexed and based on same addressreg but have
 *             different offsets
 *
 */

static BOOL is_memoryaccess_independent P2 (const ADDRESS *, ap1, const ADDRESS *, ap2)
{
    if ((ap1 == NIL_ADDRESS) || (ap2 == NIL_ADDRESS)) {
	return TRUE;
    }
    /* same address cannot be independent */
    if (is_equal_address (ap1, ap2)) {
	return FALSE;
    }
    /* const adresses must be independent, since they era never written to */
    if ((ap1->mode == am_const_direct) || (ap2->mode == am_const_direct)
	|| (ap1->mode == am_const_ind) || (ap2->mode == am_const_ind)) {
	return TRUE;
    }
    switch (ap2->mode) {
    case am_indx2:
    case am_indxs:
    case am_ind:
    case am_const_ind:
    case am_indx:
    case am_direct:
    case am_const_direct:
    case am_ainc:
    case am_adec:
    case am_preinc:
    case am_predec:
	break;

    default:
	/* registeracces, we are independent for sure */
	return TRUE;
    }

    switch (ap1->mode) {
    case am_indx2:
    case am_indxs:
	return ((ap2->mode == am_direct) && (ap1->preg == frameptr));
    case am_ind:
    case am_const_ind:
    case am_indx:
	if (ap1->preg != frameptr) {
	    if (((ap2->mode == am_indx) || (ap2->mode == am_ind))
		&& (ap1->preg == ap2->preg)) {
		/* same adressregister, but not same offset, must be independent */
		return TRUE;
	    } else {
		return FALSE;
	    }
	}
	break;

    case am_direct:
    case am_const_direct:
	break;

    case am_ainc:
    case am_adec:
    case am_preinc:
    case am_predec:
	return FALSE;

    default:
	/* registeracces, we are independent for sure */
	return TRUE;
    }

    /* ap1 is now either direct or stackframe */
    switch (ap2->mode) {
    case am_indx2:
    case am_indxs:
	/* stackframe and direct are independent */
	return ((ap1->mode == am_direct) && (ap2->preg == frameptr));
    case am_ind:
    case am_const_ind:
    case am_indx:
	if (ap2->preg != frameptr) {
	    /* we cannot tell where that normal addressregisters point */
	    return FALSE;
	}
	/* both accesses on stackframe, but with different offsets, therefore independent */
	return TRUE;

    case am_direct:
    case am_const_direct:
	/* direct is independent from direct or stackframe */
	return TRUE;

    case am_ainc:
    case am_adec:
    case am_preinc:
    case am_predec:
	return FALSE;

    default:
	/* registeracces, we are independent for sure */
	return TRUE;
    }
    return TRUE;
}
#endif

/*
 * Determine whether a move is redundent ... this is done by looking back
 * along the code list (following all branches to labels) to determine
 * whether the destination already contains the necessary result.
 *
 * ip must be a ldf or ldi
 */
static BOOL was_load_redundant P2 (CODE *, ip, CODE *, ip2)
{
    LABEL   label;
    struct swtab *sw;

#ifdef SAVE_PEEP_MEMORY
    PEEPINFO StaticMemory;

#endif /* SAVE_PEEP_MEMORY */
    PEEPINFO *map = GET_PEEP_INFO (ip, &StaticMemory);
    REGBITMAP Changed;
    REGBITMAP DoNotTouch =

	map->read | map->used | map->updated | map->write | map->modified;

    /* 
     * check for sequence ldi *arn,arn, ldi *+arn(offset),arn 
     * those cannot be left out
     * since arn points not to the same location later
     */

    if (is_address_register (ip->dst->preg)
	&& ((ip->src1->mode == am_ind)
	    || (ip->src1->mode == am_const_ind)
	    || (ip->src1->mode == am_indx)
	    || (ip->src1->mode == am_indx2)
	    || (ip->src1->mode == am_indxs))
	&& (ip->src1->preg == ip->dst->preg)) {
	return FALSE;
    }
    if (is_index_register (ip->dst->preg)
	&& ((ip->src1->mode == am_indx2)
	    || (ip->src1->mode == am_indxs))
	&& (ip->src1->sreg == ip->dst->preg)) {
	return FALSE;
    }
    for (ip2 = ip2->back; ip2 != NIL_CODE; ip2 = ip2->back) {
	switch (ip2->opcode) {
	case op_label:
	    label = ip2->src1->u.offset->v.l;

	    /* first check code before the label */
	    if (!was_load_redundant (ip, ip2)) {
		return FALSE;
	    }
	    /* ... and then check all branches to this label */
	    for (ip2 = peep_head; ip2 != NIL_CODE; ip2 = ip2->fwd) {
		switch (ip2->opcode) {
		case op_br:
		case op_bu:
		case op_blo:
		case op_bls:
		case op_bhi:
		case op_bhs:
		case op_beq:
		case op_bne:
		case op_blt:
		case op_ble:
		case op_bgt:
		case op_bge:
		case op_bz:
		case op_bnz:
		case op_bp:
		case op_bn:
		case op_bnn:
		    if (is_label_used (ip2->src1, label)) {
			OPCODE  op = ip2->opcode;

			ip2->opcode = op_abort;
			if (!was_load_redundant (ip, ip2)) {
			    ip2->opcode = op;
			    return FALSE;
			}
			ip2->opcode = op;
		    }
		    break;
		default:
		    break;
		}
	    }

	    /* but if it is via a jump table we must check all table */
	    for (sw = swtables; sw != NIL_SWITCH; sw = sw->next) {
		LABEL   lab;

		for (lab = 0; lab < sw->numlabs; lab++) {
		    if (sw->labels[lab] == label) {
			if (!is_peep_phase (peep_level, PEEP_SWITCH)) {
			    /* do not check table, assume load is not redundant */
			    return FALSE;
			} else {
			    /* will be overwritten if we find bu for this switch */
			    OPCODE  op = op_abort;

			    for (ip2 = peep_head; ip2 != NIL_CODE;
				 ip2 = ip2->fwd) {
				if (ip2->opcode == op_bu) {
				    /* genc30 puts labelnumber into offset of src1 */
				    if (ip2->src1->u.offset->v.l ==
					sw->tablab) {
					op = ip2->opcode;

					ip2->opcode = op_abort;
					if (!was_load_redundant (ip, ip2)) {
					    ip2->opcode = op;
					    return FALSE;
					}
					ip2->opcode = op;
				    }
				}
			    }
			    if (op == op_abort) {
				FATAL (
				       (__FILE__, "was_load_redundant",
					"bu for switchtable not found"));
			    }
			}
		    }
		}
	    }
	    return TRUE;
	case op_retsu:
	case op_retiu:
	case op_bu:
	case op_br:
	    /* should have at least hit a label before here! */
	case op_abort:
	    return TRUE;
	case op_callu:
	case op_trapu:
	case op_call:
	case op_xcall:
	    return FALSE;
#ifdef ASM
	case op_asm:
	    return FALSE;
#endif /* ASM */
	case op_line:
	    break;
	default:
	    /* Check for same instruction, not ainc or adec */
	    if ((ip2->opcode == ip->opcode)
		&& is_equal_address (ip->src1, ip2->src1)
		&& is_equal_address (ip->dst, ip2->dst)) {
		return TRUE;
	    }
	    /* Check for ldi/ldf a,b preceeded with ldi/ldf b,a  */
	    /* not ainc or adec                                  */
	    if ((ip2->opcode == ip->opcode)
		&& is_equal_address (ip->src1, ip2->dst)
		&& is_equal_address (ip->dst, ip2->src1)) {
		return TRUE;
	    }
	    /* Check for ldi/ldf a,b preceeded with sti/stf b,a  */
	    /* not ainc or adec                                  */
	    if ((((ip2->opcode == op_stf) && (ip->opcode == op_ldf))
		 || ((ip2->opcode == op_sti) && (ip->opcode == op_ldi)))
		&& is_equal_address (ip->src1, ip2->dst)
		&& is_equal_address (ip->dst, ip2->src1)) {
		return TRUE;
	    }
	    map = GET_PEEP_INFO (ip2, &StaticMemory);

	    Changed = map->updated | map->write | map->modified;
	    if ((Changed & DoNotTouch) != 0) {
#ifdef PEEP_REDUNDANT_STI
		if ((Changed & DoNotTouch) == (1UL << REG_MEMORY)) {
		    if ((!is_memoryaccess_independent (ip->src1, ip2->dst))
			||
			(!is_memoryaccess_independent (ip->src1, ip2->dst2)))
		    {
			return FALSE;
		    }
		} else {
		    return FALSE;
		}
#else
		return FALSE;
#endif
	    }
	    break;
	}
    }
    return FALSE;
}


/*
 * Determine whether a sti is redundent ... this is done by looking forward
 * along the code list (following all branches to labels) to determine
 * whether the destination is used already contains the necessary result.
 *
 * ip must be a stf or sti
 */
#ifdef PEEP_REDUNDANT_STI
static BOOL is_store_redundant P2 (CODE *, ip, CODE *, ip2)
{
    OPCODE  op;
    BOOL    result;
    LABEL   label;
    CODE   *target;
    SWITCH *sw, *table;
    unsigned i;

#ifdef SAVE_PEEP_MEMORY
    PEEPINFO StaticMemory;

#endif /* SAVE_PEEP_MEMORY */
    PEEPINFO *map = GET_PEEP_INFO (ip, &StaticMemory);
    REGBITMAP Changed;
    REGBITMAP DoNotTouch = 0;
    BOOL    is_frame_space = FALSE;

    switch (ip->dst->mode) {
    case am_indx2:
    case am_indxs:
	DoNotTouch = (1UL << ip->dst->sreg);
	/*lint -fallthrough */
    case am_ind:
    case am_const_ind:
    case am_indx:
	DoNotTouch |= (1UL << ip->dst->preg);
	is_frame_space = (ip->dst->preg == frameptr);
	break;

    case am_direct:
    case am_const_direct:
	break;

    case am_ainc:
    case am_adec:
    case am_preinc:
    case am_predec:
	return FALSE;

    default:
	FATAL (
	       (__FILE__, "is_store_redundent",
		"illegal addressmode for sti/stf"));
	break;
    }

    if (ip == NIL_CODE)
	return FALSE;

    for (ip2 = ip2->fwd; ip2 != NIL_CODE; ip2 = ip2->fwd) {
	switch (ip2->opcode) {
	case op_retsu:
	case op_retiu:
	    if (is_frame_space) {
		/*
		 **  Will not be used further
		 */
		return TRUE;
	    } else {
		/* maybe not on frame and will therefore be used further */
		return FALSE;
	    }
	case op_abort:
	    /* We have closed a loop, will not be used further */
	    return TRUE;
#ifdef ASM
	case op_asm:		/* really it in unknown */
	    return FALSE;
#endif /* ASM */
	case op_blo:
	case op_bls:
	case op_bhi:
	case op_bhs:
	case op_beq:
	case op_bne:
	case op_blt:
	case op_ble:
	case op_bgt:
	case op_bge:
	case op_bz:
	case op_bnz:
	case op_bp:
	case op_bn:
	case op_bnn:
	    op = ip2->opcode;
	    ip2->opcode = op_abort;	/* to prevent looping */
	    /* follow both paths of the branch */
	    result = is_store_redundant (ip, ip2) &&
		is_store_redundant (ip,
				    find_label (ip2->src1->u.offset->v.l));
	    ip2->opcode = op;
	    return result;
	case op_bud:
	case op_brd:
	    /*
	     * brd should not be found in this stage of
	     * codeanalysis. return FALSE if still found.
	     */
	    return FALSE;
	case op_bu:
	    /*
	     * bu is used in switchtable, so we cannot say
	     * (we had to check al paths of the switch), assume
	     * value is used, thats save in any case
	     * if enabled in option we analyse switches too
	     */
	    if (!is_peep_phase (peep_level, PEEP_SWITCH)) {
		return FALSE;
	    }
	    /* look for the switchtable belonging to that op_bu */
	    table = NIL_SWITCH;
	    for (sw = swtables; sw != NIL_SWITCH; sw = sw->next) {
		/* genc30 adds the labelname into the offsetfield of src1 */
		if (ip2->src1->u.offset->v.l == sw->tablab) {
		    table = sw;
		    break;
		}
	    }
	    if (table != NIL_SWITCH) {
		op = ip2->opcode;
		ip2->opcode = op_abort;	/* to prevent looping */
		result = TRUE;
		/* follow all paths of the switch */
		for (i = 0; i < table->numlabs; i++) {
		    label = table->labels[i];
		    target = find_label (label);
		    if (target == NULL) {
			FATAL (
			       (__FILE__, "is_store_redundant",
				"switch_target not found"));
		    }
		    result = result && is_store_redundant (ip, target);
		}
		ip2->opcode = op;
		return result;
	    } else {
		FATAL (
		       (__FILE__, "is_store_redundant",
			"switchtable for bu not found"));
	    }
	    return FALSE;


	case op_br:
	    op = ip2->opcode;
	    ip2->opcode = op_abort;	/* to prevent looping */
	    result =
		is_store_redundant (ip,
				    find_label (ip2->src1->u.offset->v.l));
	    ip2->opcode = op;
	    return result;

	case op_xcall:
	case op_callu:
	case op_trapu:
	case op_call:
	    /* We do not know what functions does with memory */
	    return FALSE;

	default:

	    map = GET_PEEP_INFO (ip2, &StaticMemory);

	    Changed = map->write | map->modified | map->updated;

	    /*
	     * If any of the registers belonging to this addressmode is
	     * changed we must do the store (it is not redundant)
	     */

	    if ((Changed & DoNotTouch) != 0) {
		return FALSE;
	    }
	    /*
	     * If we read from memory we may just read the
	     * stored value, so return TRUE (it is used)
	     */

	    if ((map->read & (1UL << REG_MEMORY)) != 0) {
		/*
		 *  except it was written in the stackframe
		 *  and is now read from a different address
		 *  from the stackframe, or it was written
		 *  direct and is now read from a different
		 *  direct address, or if it was written direct
		 *  and is now read from stackframe or vice versa
		 *  those are the only cases we can be sure that
		 *  there is no aliasing
		 */
		if ((!is_memoryaccess_independent (ip->dst, ip2->src1))
		    || (!is_memoryaccess_independent (ip->dst, ip2->src2))
		    || (!is_memoryaccess_independent (ip->dst, ip2->src21))
		    || (!is_memoryaccess_independent (ip->dst, ip2->src22))) {
		    return FALSE;
		}
	    }
	    /*
	     * If we write to memory we may just overwrite the
	     * stored value, so return TRUE (store is redundant)
	     */

	    if ((map->write & (1UL << REG_MEMORY)) != 0) {
		if ((is_equal_address (ip->dst, ip2->dst))
		    || (is_equal_address (ip->dst, ip2->dst2))) {
		    return TRUE;
		}
	    }
	    break;

	case op_label:
	case op_line:
	    break;
	}
    }
    return TRUE;
}
#endif

/*
 * Peephole optimization for ldi instructions.
 */
static void peep_ldi P1 (CODE *, ip)
{
    CODE   *ip2;

    /*
     * remove ldi rx,rx (they may set the flags but flags are ignored after ldi
     *
     */
    if (is_equal_address (ip->src1, ip->dst)) {
	peep_delete (ip);
	IncPeepStatistic (ldi_rx_rx);
	DPRINTF ((DEBUG_PEEP, " Peep eliminated ldi rx,rx\n"));
	return;
    }
    /*
     * go back to previous valid instruction
     */
    ip2 = ip;
    do {
	ip2 = ip2->back;
    } while ((ip2 != NIL_CODE) && (ip2->opcode == op_line));

    /* eliminate redundant move with registers on sequence:
     *          ldi    Xn, Xm
     *          ldi    Xm, Xn
     */
    if ((ip2 != NIL_CODE) && (ip2->opcode == op_ldi) &&
	is_equal_address (ip->src1, ip2->dst) &&
	is_equal_address (ip->dst, ip2->src1)) {
	peep_delete (ip);
	IncPeepStatistic (ldi_rx_ry_ldi_ry_rx);
	DPRINTF ((DEBUG_PEEP, " Peep eliminated ldi rx,ry ldi ry,rx\n"));
	return;
    }
    /* eliminate redundant move with registers on sequence:
     *          sti    Xn, Xm
     *          ldi    Xm, Xn
     */
    if ((ip2 != NIL_CODE) && (ip2->opcode == op_sti) &&
	is_equal_address (ip->src1, ip2->dst) &&
	is_equal_address (ip->dst, ip2->src1)) {
	peep_delete (ip);
	IncPeepStatistic (sti_rx_my_ldi_my_rx);
	DPRINTF ((DEBUG_PEEP, " Peep eliminated sti rx,ry ldi ry,rx\n"));
	return;
    }
    /* remove transfer via memory on sequence:
     * change
     *          sti    Rx, Xm
     *          ldi    Xm, Ry
     * to
     *          sti    Rx, Xm
     *          ldi    Rx, Ry
     */
    if ((ip2 != NIL_CODE) && (ip2->opcode == op_sti) &&
	is_equal_address (ip->src1, ip2->dst)) {
	ip->src1 = ip2->src1;
	/*
	 * Not forgetting to update infofield in instruction !!!
	 */
	Update_Peep_Info (ip);
	DPRINTF ((DEBUG_PEEP, " Peep changed sti rx,my ldi my,rz\n"));
	return;
    }
    if (is_peep_phase (peep_level, PEEP_REDUNDANT) &&
	is_peep_enabled (REDUNDANT) && was_load_redundant (ip, ip)) {

	peep_delete (ip);
	IncPeepStatistic (redundant_ldi);
	DPRINTF ((DEBUG_PEEP, " Peep eliminated redundant ldi \n"));
	return;
    }
    /* eliminate redundant move with registers on sequence:
     *          or     Xn, Xm  <- Commutative instruction
     *          ldi    Xm, Xn
     *          Xm is not used further
     * replace with
     *          or     Xm,Xn
     *
     */
    if ((ip2 != NIL_CODE) && (is_commutative_opcode (ip2->opcode)) &&
	is_peep_enabled (COMMUTATIVE) &&
	is_equal_address (ip->src1, ip2->dst) &&
	is_equal_address (ip->dst, ip2->src1) &&
	!is_value_used (ip->src1->preg, ip)) {
	ADDRESS *ap = ip2->dst;

	ip2->dst = ip2->src1;
	ip2->src1 = ap;
	if (ip2->opcode == op_subi) {
	    ip2->opcode = op_subri;
	    /* DPRINTF ((DEBUG_PEEP, "++++++ subri ++++++++++++\n")); */
	} else if (ip2->opcode == op_subri) {
	    /* DPRINTF ((DEBUG_PEEP, "++++++ subi ++++++++++++\n"));  */
	    ip2->opcode = op_subi;
	}
	/*
	 * Not forgetting to update infofield in instruction !!!
	 */
	Update_Peep_Info (ip2);

	peep_delete (ip);
	IncPeepStatistic (commutative_i);
	DPRINTF (
		 (DEBUG_PEEP,
		  " Peep eliminated commutative rx,ry ldi ry,rx\n"));
	return;
    }
#if 1
    /* combine move with preceding instructipon on sequence:
     *          addi   Xn, Xm   binary instruction
     *          ldi    Xm, Xy
     *          Xm is not used further
     * replace with
     *          addi3  Xm,Xn,Xy
     *
     */
    if ((ip2 != NIL_CODE) && (is_op_op3_availlable (ip2->opcode)) &&
	is_peep_enabled (COMBINE_BWD) &&
	is_equal_address (ip->src1, ip2->dst) &&
	is_3op_possible (ip2->src1) && !is_value_used (ip->src1->preg, ip)) {

	ip2->opcode = convert_op_to_op3[ip2->opcode];
	ip2->src2 = ip2->dst;
	ip2->dst = ip->dst;
	/*
	 * Not forgetting to update infofield in instruction !!!
	 */
	Update_Peep_Info (ip2);
	peep_delete (ip);
	IncPeepStatistic (opi_ldi_opi3);
	DPRINTF (
		 (DEBUG_PEEP,
		  " Peep combined opi z,ry ldi x,ry to opi3 z,x,ry\n"));
	return;
    }
#endif
    /* Combine load followeded by binary instruction to
     * 3-op instruction:
     *
     *          ldi    Xn, Xm
     *          addi   Xr, Xm
     *           Xn, Xr may not by Immediate, Direct
     *                  or indirect with offset > +/-1
     * replace with
     *          addi3  Xm,Xr,Xm
     *
     */

    /*
     * go back to previous valid instruction
     */
    ip2 = ip;
    do {
	ip2 = ip2->fwd;
    } while ((ip2 != NIL_CODE) && (ip2->opcode == op_line));
    if ((ip2 != NIL_CODE) && (is_op_op3_availlable (ip2->opcode)) &&
	is_peep_enabled (COMBINE_FWD) && is_equal_address (ip->dst, ip2->dst)) {

	if (is_3op_possible (ip->src1)
	    && is_3op_possible (ip2->src1)) {

	    ip2->opcode = convert_op_to_op3[ip2->opcode];
	    ip2->src2 = ip->src1;
	    /*
	     * Not forgetting to update infofield in instruction !!!
	     */
	    Update_Peep_Info (ip2);
	    peep_delete (ip);
	    IncPeepStatistic (ldi_opi_opi3);
	    DPRINTF (
		     (DEBUG_PEEP,
		      " Peep combined ldi x,ry opi z,ry to opi3 z,x,ry\n"));
	    return;
	}
    }
}

/*
 * Peephole optimization for ldf instructions.
 */
static void peep_ldf P1 (CODE *, ip)
{
    CODE   *ip2;

    /*
     * remove ldf rx,rx (they may set the flags but flags are ignored after ldi
     *
     */
    if (is_equal_address (ip->src1, ip->dst)) {
	peep_delete (ip);
	IncPeepStatistic (ldf_rx_rx);
	DPRINTF ((DEBUG_PEEP, " Peep eliminated ldf rx,rx\n"));
	return;
    }
    /*
     * go back to previous valid instruction
     */
    ip2 = ip;
    do {
	ip2 = ip2->back;
    } while ((ip2 != NIL_CODE) && (ip2->opcode == op_line));

    /* eliminate redundant move with registers on sequence:
     *          ldf    Xn, Xm
     *          ldf    Xm, Xn
     */
    if ((ip2 != NIL_CODE) && (ip2->opcode == op_ldf) &&
	is_equal_address (ip->src1, ip2->dst) &&
	is_equal_address (ip->dst, ip2->src1)) {
	peep_delete (ip);
	IncPeepStatistic (ldf_rx_ry_ldf_ry_rx);
	DPRINTF ((DEBUG_PEEP, " Peep eliminated ldf rx,ry ldf ry,rx\n"));
	return;
    }
    /* eliminate redundant move with registers on sequence:
     *          stf    Xn, Xm
     *          ldf    Xm, Xn
     */
    if ((ip2 != NIL_CODE) && (ip2->opcode == op_stf) &&
	is_equal_address (ip->src1, ip2->dst) &&
	is_equal_address (ip->dst, ip2->src1)) {
	peep_delete (ip);
	IncPeepStatistic (stf_rx_my_ldf_my_rx);
	DPRINTF ((DEBUG_PEEP, " Peep eliminated stf rx,ry ldf ry,rx\n"));
	return;
    }
    /* remove transfer via memory on sequence:
     * change
     *          stf    Rx, Xm
     *          ldf    Xm, Ry
     * to
     *          stf    Rx, Xm
     *          ldf    Rx, Ry
     */
    if ((ip2 != NIL_CODE) && (ip2->opcode == op_stf) &&
	is_equal_address (ip->src1, ip2->dst)) {
	ip->src1 = ip2->src1;
	/*
	 * Not forgetting to update infofield in instruction !!!
	 */
	Update_Peep_Info (ip);
	DPRINTF ((DEBUG_PEEP, " Peep changed stf rx,my ldf my,rz\n"));
	return;
    }
    if (is_peep_phase (peep_level, PEEP_REDUNDANT) &&
	is_peep_enabled (REDUNDANT) && was_load_redundant (ip, ip)) {
	peep_delete (ip);
	IncPeepStatistic (redundant_ldf);
	DPRINTF ((DEBUG_PEEP, " Peep eliminated redundant ldf \n"));
	return;
    }
    /* eliminate redundant move with registers on sequence:
     *          addf   Xn, Xm  <- Commutative instruction
     *          ldf    Xm, Xn
     *          Xm is not used further
     * replace with
     *          addf     Xm,Xn
     *
     */
    if ((ip2 != NIL_CODE) && (is_commutative_opcode (ip2->opcode)) &&
	is_peep_enabled (COMMUTATIVE) &&
	is_equal_address (ip->src1, ip2->dst) &&
	is_equal_address (ip->dst, ip2->src1) &&
	!is_value_used (ip->src1->preg, ip)) {
	ADDRESS *ap = ip2->dst;

	ip2->dst = ip2->src1;
	ip2->src1 = ap;
	if (ip2->opcode == op_subf) {
	    ip2->opcode = op_subrf;
	    /* DPRINTF ((DEBUG_PEEP, "++++++ subrf ++++++++++++\n")); */
	} else if (ip2->opcode == op_subrf) {
	    /* DPRINTF ((DEBUG_PEEP, "++++++ subf ++++++++++++\n"));  */
	    ip2->opcode = op_subf;
	}
	/*
	 * Not forgetting to update infofield in instruction !!!
	 */
	Update_Peep_Info (ip2);
	peep_delete (ip);
	IncPeepStatistic (commutative_f);
	DPRINTF (
		 (DEBUG_PEEP,
		  " Peep eliminated commutative rx,ry ldf ry,rx\n"));
	return;
    }
#if 1
    /* combine move with preceding instructipon on sequence:
     *          addf   Xn, Xm   binary instruction
     *          ldf    Xm, Xy
     *          Xm is not used further
     * replace with
     *          addf3  Xm,Xn,Xy
     *
     */
    if ((ip2 != NIL_CODE) && (is_op_op3_availlable (ip2->opcode)) &&
	is_peep_enabled (COMBINE_BWD) &&
	is_equal_address (ip->src1, ip2->dst) &&
	is_3op_possible (ip2->src1) && !is_value_used (ip->src1->preg, ip)) {

	ip2->opcode = convert_op_to_op3[ip2->opcode];
	ip2->src2 = ip2->dst;
	ip2->dst = ip->dst;
	/*
	 * Not forgetting to update infofield in instruction !!!
	 */
	Update_Peep_Info (ip2);
	peep_delete (ip);
	IncPeepStatistic (opf_ldf_opf3);
	DPRINTF (
		 (DEBUG_PEEP,
		  " Peep combined opf z,ry ldf x,ry to opf3 z,x,ry\n"));
	return;
    }
#endif
    /* Combine load followeded by binary instruction to
     * 3-op instruction:
     *
     *          ldf    Xn, Xm
     *          addf   Xr, Xm
     *           Xn, Xr may not by Immediate, Direct
     *                  or indirect with offset > +/-1
     * replace with
     *          addf3  Xm,Xr,Xm
     *
     */

    /*
     * go back to previous valid instruction
     */
    ip2 = ip;
    do {
	ip2 = ip2->fwd;
    } while ((ip2 != NIL_CODE) && (ip2->opcode == op_line));
    if ((ip2 != NIL_CODE) && (is_op_op3_availlable (ip2->opcode)) &&
	is_peep_enabled (COMBINE_FWD) && is_equal_address (ip->dst, ip2->dst)) {

	if (is_3op_possible (ip->src1)
	    && is_3op_possible (ip2->src1)) {

	    ip2->opcode = convert_op_to_op3[ip2->opcode];
	    ip2->src2 = ip->src1;
	    /*
	     * Not forgetting to update infofield in instruction !!!
	     */
	    Update_Peep_Info (ip2);
	    peep_delete (ip);
	    IncPeepStatistic (ldf_opf_opf3);
	    DPRINTF (
		     (DEBUG_PEEP,
		      " Peep combined ldf x,ry opf z,ry to opf3 z,x,ry\n"));
	    return;
	}
    }
}

/*
 * Peephole optimization for sti instructions.
 */
static void peep_sti P1 (CODE *, ip)
{
    CODE   *ip2;

    /* eliminate redundant move with registers on sequence:
     *          ldi    Xn, Xm
     *          sti    Xm, Xn
     */
    /*
     * go back to previous valid instruction
     */
    ip2 = ip;
    do {
	ip2 = ip2->back;
    } while ((ip2 != NIL_CODE) && (ip2->opcode == op_line));


    if ((ip2 != NIL_CODE) && (ip2->opcode == op_ldi) &&
	is_equal_address (ip->src1, ip2->dst) &&
	is_equal_address (ip->dst, ip2->src1)) {
	peep_delete (ip);
	IncPeepStatistic (ldi_mx_ry_sti_ry_mx);
	DPRINTF ((DEBUG_PEEP, " Peep eliminated ldi rx,ry sti ry,rx\n"));
	return;
    }
#ifdef PEEP_REDUNDANT_STI
    /*
     * remove sti if destination is not used in future 
     */
    if (is_peep_phase (peep_level, PEEP_STORE)
	&& is_peep_enabled (REDUNDANT)
	&& (is_store_redundant (ip, ip))) {
	peep_delete (ip);
	IncPeepStatistic (redundant_sti);
	DPRINTF ((DEBUG_PEEP, " Peep eliminated redundant sti\n"));
	return;
    }
#endif
}


/*
 * Peephole optimization for stf instructions.
 */
static void peep_stf P1 (CODE *, ip)
{
    CODE   *ip2;

    /* eliminate redundant move with registers on sequence:
     *          ldf    Xn, Xm
     *          stf    Xm, Xn
     */
    /*
     * go back to previous valid instruction
     */
    ip2 = ip;
    do {
	ip2 = ip2->back;
    } while ((ip2 != NIL_CODE) && (ip2->opcode == op_line));

    if ((ip2 != NIL_CODE) && (ip2->opcode == op_ldf) &&
	is_equal_address (ip->src1, ip2->dst) &&
	is_equal_address (ip->dst, ip2->src1)) {
	peep_delete (ip);
	IncPeepStatistic (ldf_mx_ry_stf_ry_mx);
	DPRINTF ((DEBUG_PEEP, " Peep eliminated ldf rx,ry stf ry,rx\n"));
	return;
    }
#ifdef PEEP_REDUNDANT_STI
    /*
     * remove stf if destination is not used in future 
     */
    if (is_peep_phase (peep_level, PEEP_STORE)
	&& is_peep_enabled (REDUNDANT)
	&& (is_store_redundant (ip, ip))) {
	peep_delete (ip);
	IncPeepStatistic (redundant_stf);
	DPRINTF ((DEBUG_PEEP, " Peep eliminated redundant stf\n"));
	return;
    }
#endif
}


/*
 * Peephole optimization for unconditional transfers.  Deletes instructions
 * which have no path.  Applies to bra, jmp, and rts instructions.
 */
static void peep_uctran P1 (const CODE *, ip)
{
    while (ip->fwd != NIL_CODE && ip->fwd->opcode != op_label) {
	if ((ip->fwd->opcode != op_line)
#ifdef ASM
	    && (ip->fwd->opcode != op_asm)
#endif /* ASM */
	    ) {
	    /* we want to preserve asm and linestatements */
	    peep_delete (ip->fwd);
	    IncPeepStatistic (dead_code);
	    DPRINTF ((DEBUG_PEEP, " Peep deleted dead code\n"));
	} else {
	    ip = ip->fwd;
	}
    }
}


/*
 * Replaces an instruction with their 3-op equivalent, if
 * possible, may lead to better code.
 * Other optimizations may become possible, since long data paths
 * may become interruptable.
 */
static void convert_to_op3 P1 (CODE *, ip)
{
    if ((ip != NIL_CODE) && (is_op_op3_availlable (ip->opcode))
	&& (is_am_register (ip->src1))) {
	/* 
	 * we only want to modify pure register operations
	 * only those offer best optimizations possibilities
	 *
	 * e.g. add r0, r3  ---> add r0, r3, r3
	 */
	ip->opcode = convert_op_to_op3[ip->opcode];
	ip->src2 = ip->dst;
	/*
	 * Not forgetting to update infofield in instruction !!!
	 */
	Update_Peep_Info (ip);
	IncPeepStatistic (convert_to_op3);
	DPRINTF ((DEBUG_PEEP, " Peep converted to op3 \n"));
	/* 
	 * we did not delete anything, so we must increment 
	 * changes manually
	 */
	changes++;
    }
}


/*
 * If a label is followed by a branch to another label, the
 * branch statement can be deleted when the label is moved.
 */
static void peep_label P1 (CODE *, ip)
{
    CODE   *prev, *next, *target;
    struct swtab *sw;
    LABEL   i, lab, label;

    next = ip->fwd;
    while ((next != NIL_CODE) && (next->opcode == op_line)) {
	next = next->fwd;
    };
    if (next == NIL_CODE)
	return;

    /*if (!optimize_option)
     *    return;
     */

    lab = ip->src1->u.offset->v.l;
    switch (next->opcode) {
    case op_label:
	/* if a label is followed by a label then common them up */
	label = next->src1->u.offset->v.l;
	for (target = peep_head; target != NIL_CODE; target = target->fwd) {
	    if (is_label_used (target->src1, label))
		target->src1->u.offset->v.l = lab;
	    /*
	     * since labels of branches and jumps are always the first
	     * operand of those instructions we need only look at src1
	     * (I hope I'm correct)
	     *
	     *   if (is_label_used ( target->src2, label))
	     *       target->src2->u.offset->v.l = lab;
	     *   if (is_label_used ( target->dst, label))
	     *       target->dst->u.offset->v.l = lab;
	     */
	}
	for (sw = swtables; sw != NIL_SWITCH; sw = sw->next) {
	    for (i = 0; i < sw->numlabs; i++) {
		if (sw->labels[i] == label)
		    sw->labels[i] = lab;
	    }
	}
	peep_delete (next);
	IncPeepStatistic (label_commoned);
	DPRINTF ((DEBUG_PEEP, " Peep commoned up 2 labels\n"));
	break;

    case op_bu:
    case op_br:
	prev = ip->back;
	/*
	 * To make this fast, assume that the label number is really
	 * next->src1->u.offset->v.l
	 */
	label = next->src1->u.offset->v.l;
	if (label == lab)
	    return;
	target = find_label (label);
	if (target == NIL_CODE) {
	    message (MSG_PEEPLABEL);
	    return;
	}
	/* move label */
	if (target->fwd == ip)
	    return;
	peep_delete (ip);
	ip->fwd = target->fwd;
	ip->back = target;
	target->fwd = ip;
	if (ip->fwd != NIL_CODE)
	    ip->fwd->back = ip;
	IncPeepStatistic (label_before_branch);
	DPRINTF (
		 (DEBUG_PEEP,
		  " Moved label before branch to Dest of branch\n"));
	/* possibly remove branches */
	/* in fact, prev is always != 0 if peep_delete has succeeded */
	if (prev != NIL_CODE) {
	    if (prev->opcode == op_br || prev->opcode == op_bu
		|| prev->opcode == op_retsu || prev->opcode == op_retiu)
		peep_uctran (prev);
	}
	break;
    default:
	/* check that there are still references to this label */
	if (label_references (ip) == 0) {
	    IncPeepStatistic (label_removed);
	    DPRINTF ((DEBUG_PEEP, " Peep eliminated unreferenced label\n"));
	    peep_delete (ip);
	}
	break;
    }
}


/* delete multiple debugging line statements */
static void peep_line P1 (CODE *, ip)
{
    CODE   *ip2;

    if (ip->fwd == NIL_CODE)
	return;
    switch (ip->fwd->opcode) {
    case op_line:
	if (ip->src1->u.offset->v.i == ip->fwd->src1->u.offset->v.i)
	    peep_delete (ip);
	break;
    case op_label:
	/* move the line number to after the label */
	ip2 = ip->fwd;
	if (ip->back)
	    ip->back->fwd = ip2;
	else
	    peep_head = ip2;
	if (ip2->fwd)
	    ip2->fwd->back = ip;
	ip2->back = ip->back;
	ip->fwd = ip2->fwd;
	ip2->fwd = ip;
	ip->back = ip2;
	break;
    default:
	break;
    }
}

/*
 * Peephole optimization for compare instructions.
 * changes compare #0 to tst
 */
static void peep_cmpi P1 (CODE *, ip)
{
    CODE   *p;
    EXPR   *ep;

    if (ip->src1->mode != am_immed)
	return;
    ep = ip->src1->u.offset;
    if (ep->nodetype == en_icon && ep->v.i == 0L) {
	/*
	 *   cmpi 0,rn =>    tstb3 rn,rn
	 *   but only if following cond. instr is not
	 *   using the carry-flag in any way.
	 *
	 *   Carry is only used for unsigned compares,
	 *   but an unsigned compare against 0
	 *   can only be same, not same, or higher.
	 *
	 *   Lower is always false,
	 *   lower-same is identical to equal,
	 *   higher is identical to not-equal and
	 *   higher same is always true.
	 *
	 *   Those cases should be simplified best in the global optimizer
	 *   so we simply do no changes here if such a case happens    
	 */
	p = ip->fwd;
	while ((p != NIL_CODE)
	       && (!is_using_flags (p->opcode))) {
	    p = p->fwd;
	}
	if ((p == NIL_CODE) || (is_using_carry (p->opcode))) {
	    eprintf ("DEBUG: peep_cmpi: unsigned compare against 0\n");
	    return;
	}
	ip->src1 = ip->src2;
	ip->dst = NIL_ADDRESS;
	ip->opcode = op_tstb3;
	next_ip = ip;
	/*
	 * Not forgetting to update infofield in instruction !!!
	 */
	Update_Peep_Info (ip);
	return;
    }
}


/*
 * Deletes a tst instruction if the flags are already set.
 */
static void peep_tst P1 (const CODE *, ip)
{
    CODE   *ip2;

    if (ip->src1->mode != am_dreg) {
	/*
	 *  we assume that op_tst is allways used in the form
	 *  tstb rn, rn
	 *  that also both operands are the same registers
	 *  
	 *  only operations with dst = R0-R7 can set flags
	 *  (except cmp and tst, of course)
	 */
	return;
    }
    /*
     * go back to find an instruction which sets the flags
     */
    for (ip2 = ip->back; ip2 != NIL_CODE; ip2 = ip2->back) {
	switch (ip2->opcode) {
	case op_label:
#ifdef ASM
	case op_asm:
#endif /* ASM */
	case op_call:
	case op_trapu:
	case op_callu:
	case op_xcall:
	    /* br and bu cannot happen, we should find a label before */
	    return;

	case op_line:
	    break;

	default:
	    if (is_set_flags (ip2)) {
		if ((ip2->dst != NIL_ADDRESS)	/* May happen with tstb3, cmp3 */
		    &&(ip2->dst->mode == am_dreg)
		    && (ip2->dst->preg == ip->src1->preg)) {
		    /*
		     *  Flags are set as we need it, so delete tst
		     */
		    peep_delete (ip);
		    IncPeepStatistic (superfluous_tst);
		    DPRINTF ((DEBUG_PEEP, " Peep deleted tstb/tstb3 \n"));
		    return;
		}
		return;
	    }
	}
    }
}

#ifdef FLOAT_SUPPORT
/*
 * Deletes a cmpf instruction if the flags are already set.
 * Applies only for cmpf 0.0,Rn
 */
static void peep_cmpf P1 (const CODE *, ip)
{
    CODE   *ip2;

    if ((ip->src1->mode == am_immed)
	&& (ip->src1->u.offset->nodetype == en_fcon)
	&& (ip->src1->u.offset->v.f == 0.0)) {
	/*
	 * go back to find an instruction which sets the flags
	 */
	for (ip2 = ip->back; ip2 != NIL_CODE; ip2 = ip2->back) {
	    switch (ip2->opcode) {
	    case op_label:
#ifdef ASM
	    case op_asm:
#endif /* ASM */
	    case op_call:
	    case op_trapu:
	    case op_callu:
	    case op_xcall:
		/* br and bu cannot happen, we should find a label before */
		return;

	    case op_line:
		break;

	    default:
		if (is_set_flags (ip2)) {
		    if ((ip2->dst != NIL_ADDRESS)	/* May happen with tstb3, cmp3 */
			&&(ip2->dst->mode == am_freg)
			&& (ip2->dst->preg == ip->src2->preg)) {
			/*
			 *  Flags are set as we need it, so delete tst
			 */
			peep_delete (ip);
			IncPeepStatistic (superfluous_cmpf);
			DPRINTF (
				 (DEBUG_PEEP,
				  " Peep deleted cmpf 0.0,Rn \n"));
			return;
		    }
		    return;
		}
	    }
	}
    }
}
#endif /* FLOAT_SUPPORT */


/* tries to reduce pipeline conflicts */
static void peep_pipeline P1 (CODE *, ip)
{
    CODE   *ip2, *ip3, *ip4, *insert;
    int     delay, maxdelay;

    /* Eliminate pipeline conflicts:
     *   due to read or writes from an address register
     *   followed by using an address registers for address generation
     *
     *          ldi    Rz, Rg
     *          add    Rn, Ry
     *          ldi    Xn, Am
     *          ldi   *Ax, Xn <-- Pipeline conflict, 2 cycles delay
     *
     * can be solved:
     *
     *          ldi    Xn, Am <-- Moved 2 Instr. forward, see (*)
     *          ldi    Rz, Rg <-- Ar cannot be used for address generation
     *          add    Rn, Ry <-- Ar cannot be used for address generation
     *          ldi   *Ax, Xn <-- No Pipeline conflict
     *
     * (*) can only be done if instruction sequence moved is not
     *     dependant on the 2 instructions in the middle.
     *
     *          ldi    Xn, Am
     *          ldi   *Ax, Xn <-- Pipeline conflict, 2 cycles delay
     *          ldi    Rz, Rg
     *          add    Rn, Ry
     *
     * can be solved:
     *
     *          ldi    Xn, Am
     *          ldi    Rz, Rg <-- Ar cannot be used for address generation
     *          add    Rn, Ry <-- Ar cannot be used for address generation
     *          ldi   *Ax, Xn <-- Moved 2 Instr. forward, see (*)
     *                            No pipeline conflict now
     *
     * (*) can only be done if instruction sequence moved is not
     *     dependant on the 2 instructions in the middle.
     *
     */
    ip2 = ip;			/* go back to previous valid instruction */
    do {
	ip2 = ip2->back;
    } while ((ip2 != NIL_CODE) && (ip2->opcode == op_line));

    if ((ip2 != NIL_CODE) && (!is_pipelinesearch_stop (ip2))) {
	if (is_pipelinegroup1_used (ip)) {
	    delay = 0;
	    if (is_pipelinegroup1_delayed2 (ip2)) {
		delay = 2;
	    } else if (is_pipelinegroup1_delayed1 (ip2)) {
		delay = 1;
	    } else {
		do {
		    ip2 = ip2->back;
		} while ((ip2 != NIL_CODE) && (ip2->opcode == op_line));
		if ((ip2 != NIL_CODE) && (!is_pipelinesearch_stop (ip2))
		    && (is_pipelinegroup1_delayed2 (ip2))) {
		    delay = 1;
		}
	    }
	    ip3 = ip2;
	    insert = NIL_CODE;
	    maxdelay = delay;
	    while (delay > 0) {
		do {
		    ip3 = ip3->back;
		} while ((ip3 != NIL_CODE) && (ip3->opcode == op_line));
		if ((ip3 != NIL_CODE) && (!is_pipelinesearch_stop (ip3))) {
		    if (is_pipelinegroup1 (ip3)) {
			/* no further optimization possible */
			break;
		    } else {
			if (is_ip_swap_possible (ip2, ip3)) {
			    /*
			     * check if we dont move into an other
			     * pipelineconflict (then there is no use in moving
			     * anything, becaouse there is nothing to win
			     */
			    ip4 = ip3;
			    do {
				ip4 = ip4->back;
			    } while ((ip4 != NIL_CODE)
				     && (ip4->opcode == op_line));
			    if ((ip4 != NIL_CODE)
				&& (!is_pipelinesearch_stop (ip4))) {
				if (is_pipelinegroup1_delayed (ip4)) {
				    /* further optimization does not make sense */
				    break;
				} else {
				    do {
					ip4 = ip4->back;
				    } while ((ip4 != NIL_CODE)
					     && (ip4->opcode == op_line));
				    if ((ip4 != NIL_CODE)
					&& (!is_pipelinesearch_stop (ip4))
					&& (is_pipelinegroup1_delayed2 (ip4))) {
					/* further optimization does not make sense */
					break;
				    }
				}
			    }
			    delay--;
			    insert = ip3;
			} else {
			    /* no further optimisation in this direction possible */
			    break;
			}
		    }
		} else {
		    break;
		}
	    }
	    if (insert != NIL_CODE) {
		/* Found position to insert ip2 */
		/* we can use delete to move ip2 out of linked list */
		peep_delete (ip2);
		/* now we must insert ip bevore insert */
		ip2->back = insert->back;
		ip2->fwd = insert;
		if (insert->back != NIL_CODE) {
		    insert->back->fwd = ip2;
		} else {
		    peep_head = ip2;
		}
		insert->back = ip2;
		DPRINTF (
			 (DEBUG_PEEP, "peep pipeline won %d of %d cycle \n",
			  maxdelay - delay, maxdelay));
	    }
	    ip3 = ip;
	    insert = NIL_CODE;
	    while (delay > 0) {
		do {
		    ip3 = ip3->fwd;
		} while ((ip3 != NIL_CODE) && (ip3->opcode == op_line));
		if ((ip3 != NIL_CODE) && (!is_pipelinesearch_stop (ip3))) {
		    if (is_pipelinegroup1_delayed (ip3)) {
			/* no further optimization possible */
			break;
		    } else {
			if (is_ip_swap_possible (ip, ip3)) {
			    /*
			     * dont check for uselessnes in swapping in this
			     * direction, allows for multipass optimization
			     * do only check for count of won cycles
			     */
			    ip4 = ip3;
			    do {
				ip4 = ip4->fwd;
			    } while ((ip4 != NIL_CODE)
				     && (ip4->opcode == op_line));
			    if ((ip4 != NIL_CODE)
				&& (!is_pipelinesearch_stop (ip4))) {
				if (is_pipelinegroup1_used (ip4)
				    && is_pipelinegroup1_delayed (ip)) {
				    /* we won no cycles but do change anyway */
				    delay++;
				} else {
				    do {
					ip4 = ip4->fwd;
				    } while ((ip4 != NIL_CODE)
					     && (ip4->opcode == op_line));
				    if ((ip4 != NIL_CODE)
					&& (!is_pipelinesearch_stop (ip4))
					&& (is_pipelinegroup1_used (ip4))
					&& is_pipelinegroup1_delayed2 (ip)) {
					/* we won no cycles but do change anyway */
					delay++;
				    }
				}
			    }
			    delay--;
			    insert = ip3;
			} else {
			    /* no further optimisation in this direction possible */
			    break;
			}
		    }
		} else {
		    break;
		}
	    }
	    if (insert != NIL_CODE) {
		/* Found position to insert ip (behind insert) */
		/* we can use delete to move insert out of linked list */
		peep_delete (ip);
		/* now we must insert ip behind insert */
		ip->back = insert;
		ip->fwd = insert->fwd;
		if (insert->fwd != NIL_CODE) {
		    insert->fwd->back = ip;
		}
		insert->fwd = ip;
		DPRINTF (
			 (DEBUG_PEEP,
			  "peep pipelineback won %d of %d cycle \n",
			  maxdelay - delay, maxdelay));
	    }
	}
    }
    return;
}


/*
 * Peephole optimizer. This routine calls the instruction specific
 * optimization routines above for each instruction in the peep list.
 */
static void opt3 P1 (unsigned, level)
{
    CODE   *ip;
    int     pipecount;

    int     PassCount = 0;

#ifdef VERBOSE
#ifdef DEBUG
    int     Size;
    int     Size1;
    int     Size2;
    int     TotalMemWin;
    int     TotalCycleWin;

#endif /* DEBUG */
    int     TotChanges = 0;

    STARTTIME (LastTime);

#endif /* VERBOSE */
    if (level == PEEP_NONE) {
	return;
    }
    peep_level = level;
    FIRST_PEEP_STAGE ();
#ifndef SAVE_PEEP_MEMORY
    set_all_peepinfo ();
#endif /* SAVE_PEEP_MEMORY */
#ifdef VERBOSE
    analyze_code (&oldtot, &oldbranch, &oldreg, &oldasm);
#endif /* VERBOSE */
    /* 
     * In a first pass we replace all cmpi 0,rn with tstb rn
     */
    for (next_ip = peep_head; (ip = next_ip) != NIL_CODE; next_ip = ip->fwd) {
	if (ip->opcode == op_cmpi) {
	    peep_cmpi (ip);
	}
    }
#ifdef VERBOSE
    DOTIME (cmpi_time, LastTime);
    if (verbose_option) {
	DPRINTF ((DEBUG_PEEP, "\nChanges: "));
    }
#endif /* VERBOSE */

    /* 
     * In a second pass we do the biggest part of all optimizations
     * We loop as long as there are any changes in the code
     * (A change is in most cases a delete of an instruction)
     */

    do {
	changes = 0;
	for (next_ip = peep_head; (ip = next_ip) != NIL_CODE;
	     next_ip = ip->fwd) {
	    switch (ip->opcode) {
	    case op_br:
		peep_br (ip);
		break;
	    case op_blo:
	    case op_bls:
	    case op_bhi:
	    case op_bhs:
	    case op_beq:
	    case op_bne:
	    case op_blt:
	    case op_ble:
	    case op_bgt:
	    case op_bge:
	    case op_bz:
	    case op_bnz:
	    case op_bp:
	    case op_bn:
	    case op_bnn:
		peep_bxx (ip);
		break;
	    case op_bu:
	    case op_retsu:
	    case op_retiu:
		peep_uctran (ip);
		break;
	    case op_label:
		peep_label (ip);
		break;
	    case op_line:
		peep_line (ip);
		break;
	    default:
		break;
	    }
#ifdef CHECK_LIST_INTEGRITY
	    check_peep ();
#endif /* CHECK_LIST_INTEGRITY */
	}
	DOTIME (branch_time, LastTime);
	if (is_peep_enabled
	    (STANDARD | COMBINE_FWD | COMBINE_BWD | COMMUTATIVE | REDUNDANT)) {
	    for (next_ip = peep_head; (ip = next_ip) != NIL_CODE;
		 next_ip = ip->fwd) {
		switch (ip->opcode) {
		case op_ldi:
		    peep_ldi (ip);
		    break;
		case op_ldf:
		    peep_ldf (ip);
		    break;
		case op_sti:
		    peep_sti (ip);
		    break;
		case op_stf:
		    peep_stf (ip);
		    break;
		case op_line:
		    peep_line (ip);
		    break;
		default:
		    if (is_peep_phase (level, PEEP_3OPERAND) &&
			is_peep_enabled (TO_3OPERAND)) {
			convert_to_op3 (ip);
		    }
		    break;
		}
#ifdef CHECK_LIST_INTEGRITY
		check_peep ();
#endif /* CHECK_LIST_INTEGRITY */
	    }
	}
	DOTIME (ldi_time, LastTime);
	if (is_peep_phase (level, PEEP_REMAP) && is_peep_enabled (REMAPP)) {
	    next_ip = peep_head;
	    while (next_ip != NIL_CODE) {
		ip = next_ip;
		switch (ip->opcode) {
		case op_ldi:
		case op_ldf:
		    peep_remap (ip);
		    break;
		default:
		    break;
		}
		next_ip = ip->fwd;
#ifdef CHECK_LIST_INTEGRITY
		check_peep ();
#endif /* CHECK_LIST_INTEGRITY */
	    }
	}
	DOTIME (remap_time, LastTime);

#ifdef PEEPFLOW
	if (is_peep_phase (level, PEEP_FLOW) && is_peep_enabled (DATAFLOW)) {
	    changes += flow_dataflow (peep_head, level);
	}
#endif /* PEEPFLOW */

#ifdef VERBOSE
	TotChanges += changes;
	if (verbose_option) {
	    DPRINTF ((DEBUG_PEEP, "%4d ", changes));
	}
#endif /* VERBOSE */
	DOTIME (flow_time, LastTime);
	if (changes == 0) {
	    changes = NEXT_PEEP_STAGE ();
	}
	PassCount++;
    } while (changes && (PassCount < MAX_PEEP_PASSES));
#ifdef VERBOSE
    if (verbose_option) {
	DPRINTF ((DEBUG_PEEP, "\n"));
    }
#endif /* VERBOSE */
#ifdef REGISTER_FLOW_ANALYZER
    peep_registerusage ();
#endif /* REGISTER_FLOW_ANALYZER */
    /*
     * In a fourth last pass we try to use parallel instructions
     * whenever it is possible,this must be done after all the
     * other optimizations, since it may block most of them
     */
    if (is_peep_phase (level, PEEP_PARALLEL)) {
	next_ip = peep_head;
	while (next_ip != NIL_CODE) {
	    ip = next_ip;
	    switch (ip->opcode) {
	    case op_sti:
		peep_parallel_sti (ip);
		break;
	    case op_stf:
		peep_parallel_stf (ip);
		break;
	    case op_ldi:
		peep_parallel_ldi (ip);
		break;
	    case op_ldf:
		peep_parallel_ldf (ip);
		break;
	    default:
		break;
	    }
	    next_ip = ip->fwd;
#ifdef CHECK_LIST_INTEGRITY
	    check_peep ();
#endif /* CHECK_LIST_INTEGRITY */
	}
    }
    DOTIME (paralel_time, LastTime);

    /*
     * In a third last pass we try to reduce pipeline-conflicts
     * with rearranging of instructions
     * These cannot be done earlyer, because other optimisations
     * may waste again cycles we won here 
     */
    if (is_peep_phase (level, PEEP_PIPELINE)) {
	set_all_peepflags ();
	pipecount = 0;
	do {
	    changes = 0;
	    next_ip = peep_head;
	    while (next_ip != NIL_CODE) {
		ip = next_ip;
		peep_pipeline (ip);
		next_ip = ip->fwd;
#ifdef CHECK_LIST_INTEGRITY
		check_peep ();
#endif /* CHECK_LIST_INTEGRITY */
	    }
	    DPRINTF (
		     (DEBUG_PEEP, "Pipelineoptimizer, pass %d \n",
		      ++pipecount));
	} while (changes && (pipecount < 15));
    }
    DOTIME (pipe_time, LastTime);

    /*
     * In a second last pass we remove all tstb and cmpf 0.0,rn,
     * whose flags are already set.
     * This must be done in the second last pass, since the
     * flag-setting instructions connot be swapped or deleted
     * anymore, otherwise the flags may become invalid.
     * Only pure movement of code is still allowed, which is exactly
     * what opt_delayed_branches does
     */
    /* I think we will do that in all cases, there cannot be much
     * trouble inside this optimization (I'm an optimist, I know..)
     */
    next_ip = peep_head;
    while (next_ip != NIL_CODE) {
	ip = next_ip;
	next_ip = ip->fwd;
	switch (ip->opcode) {
	case op_tstb:
	case op_tstb3:
	    peep_tst (ip);
	    break;
#ifdef FLOAT_SUPPORT
	case op_cmpf:
	    peep_cmpf (ip);
	    break;
#endif /* FLOAT_SUPPORT */
	default:
	    break;
	}
#ifdef CHECK_LIST_INTEGRITY
	check_peep ();
#endif /* CHECK_LIST_INTEGRITY */
    }
    DOTIME (tstb_time, LastTime);
    /*
     * In a last pass we try to use delayed branches whenever it is possible.
     *
     * This must be done in the last pass, since the three statements following
     * a delayed branch will be executed till the real branch is done
     * and those instructions must not be one of label, branches, calls,
     * returns and repeats.
     *
     * So no instruction following a delayed branch can be deleted
     * or moved.
     */
    if (opt_delayed_branches > 0) {
	next_ip = peep_head;
	while (next_ip != NIL_CODE) {
	    ip = next_ip;
	    next_ip = ip->fwd;
	    switch (ip->opcode) {
	    case op_br:
		peep_brdelayed (ip);
		break;
	    default:
		break;
	    }
#ifdef CHECK_LIST_INTEGRITY
	    check_peep ();
#endif /* CHECK_LIST_INTEGRITY */
	}
    }
    DOTIME (delay_time, LastTime);
#ifdef VERBOSE
    TotChanges += changes;
    analyze_code (&newtot, &newbranch, &newreg, &newasm);
    if (verbose_option) {
	DPRINTF ((DEBUG_PEEP, "%4d passes ", PassCount));

	message (MSG_PEEPCHANGES, TotChanges);
#ifdef DEBUG
	Size = PeepStatistic.ldi_rx_rx - LastPeepStatistic.ldi_rx_rx;
	TotalMemWin = Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d ldi rx,rx\n", Size));

	Size = PeepStatistic.ldf_rx_rx - LastPeepStatistic.ldf_rx_rx;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d ldf rx,rx\n", Size));

	Size =
	    PeepStatistic.ldi_rx_ry_ldi_ry_rx -
	    LastPeepStatistic.ldi_rx_ry_ldi_ry_rx;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d ldi rx,ry ldi ry,rx\n", Size));

	Size =
	    PeepStatistic.ldf_rx_ry_ldf_ry_rx -
	    LastPeepStatistic.ldf_rx_ry_ldf_ry_rx;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d ldf rx,ry ldf ry,rx\n", Size));

	Size =
	    PeepStatistic.sti_rx_my_ldi_my_rx -
	    LastPeepStatistic.sti_rx_my_ldi_my_rx;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d sti rx,my ldi my,rx\n", Size));

	Size =
	    PeepStatistic.stf_rx_my_ldf_my_rx -
	    LastPeepStatistic.stf_rx_my_ldf_my_rx;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d stf rx,my ldf my,rx\n", Size));

	Size =
	    PeepStatistic.ldi_mx_ry_sti_ry_mx -
	    LastPeepStatistic.ldi_mx_ry_sti_ry_mx;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d ldi mx,ry sti ry,mx\n", Size));

	Size =
	    PeepStatistic.ldf_mx_ry_stf_ry_mx -
	    LastPeepStatistic.ldf_mx_ry_stf_ry_mx;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d ldf mx,ry stf ry,mx\n", Size));

	Size = PeepStatistic.commutative_i - LastPeepStatistic.commutative_i;
	TotalMemWin += Size;
	DPRINTF (
		 (DEBUG_PEEP, "Removed %d commutative rx,ry ldi ry,rx\n",
		  Size));

	Size = PeepStatistic.commutative_f - LastPeepStatistic.commutative_f;
	TotalMemWin += Size;
	DPRINTF (
		 (DEBUG_PEEP, "Removed %d commutative rx,ry ldf ry,rx\n",
		  Size));

	Size = PeepStatistic.redundant_ldi - LastPeepStatistic.redundant_ldi;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d redundant ldi\n", Size));

	Size = PeepStatistic.redundant_ldf - LastPeepStatistic.redundant_ldf;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d redundant ldf\n", Size));

#ifdef PEEP_REDUNDANT_STI
	Size = PeepStatistic.redundant_sti - LastPeepStatistic.redundant_sti;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d redundant sti\n", Size));

	Size = PeepStatistic.redundant_stf - LastPeepStatistic.redundant_stf;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d redundant stf\n", Size));
#endif
	Size = PeepStatistic.ldi_opi_opi3 - LastPeepStatistic.ldi_opi_opi3;
	TotalMemWin += Size;
	DPRINTF (
		 (DEBUG_PEEP,
		  "Combined %d ldi X,rn opi Y,rn to opi3 Y,X,rn\n", Size));

	Size = PeepStatistic.ldf_opf_opf3 - LastPeepStatistic.ldf_opf_opf3;
	TotalMemWin += Size;
	DPRINTF (
		 (DEBUG_PEEP,
		  "Combined %d ldf X,rn opf Y,rn to opf3 Y,X,rn\n", Size));

	Size = PeepStatistic.opi_ldi_opi3 - LastPeepStatistic.opi_ldi_opi3;
	TotalMemWin += Size;
	DPRINTF (
		 (DEBUG_PEEP,
		  "Combined %d opi Y,rn ldi X,rn to opi3 Y,X,rn\n", Size));

	Size = PeepStatistic.opf_ldf_opf3 - LastPeepStatistic.opf_ldf_opf3;
	TotalMemWin += Size;
	DPRINTF (
		 (DEBUG_PEEP,
		  "Combined %d opf Y,rn ldf X,rn to opf3 Y,X,rn\n", Size));

	Size = PeepStatistic.par_ldf_stf - LastPeepStatistic.par_ldf_stf;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Replaced %d par_ldf_stf\n", Size));

	Size = PeepStatistic.par_stf_stf - LastPeepStatistic.par_stf_stf;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Replaced %d par_stf_stf\n", Size));

	Size = PeepStatistic.par_ldf_ldf - LastPeepStatistic.par_ldf_ldf;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Replaced %d par_ldf_ldf\n", Size));

	Size = PeepStatistic.par_ldi_sti - LastPeepStatistic.par_ldi_sti;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Replaced %d par_ldi_sti\n", Size));

	Size = PeepStatistic.par_sti_sti - LastPeepStatistic.par_sti_sti;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Replaced %d par_sti_sti\n", Size));

	Size = PeepStatistic.par_ldi_ldi - LastPeepStatistic.par_ldi_ldi;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Replaced %d par_ldi_ldi\n", Size));

	Size =
	    PeepStatistic.remap_register_fwd -
	    LastPeepStatistic.remap_register_fwd;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Remapped %d times register fwd\n", Size));

	Size =
	    PeepStatistic.remap_register_bwd -
	    LastPeepStatistic.remap_register_bwd;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Remapped %d times register bwd\n", Size));

	Size =
	    PeepStatistic.superfluous_tst - LastPeepStatistic.superfluous_tst;
	Size2 =
	    PeepStatistic.superfluous_cmpf -
	    LastPeepStatistic.superfluous_cmpf;
	TotalMemWin += Size + Size2;
	DPRINTF (
		 (DEBUG_PEEP,
		  "Rmoved superfluous tst/tst3 (%d)/cmpf 0.0,Rn(%d)\n", Size,
		  Size2));

	Size =
	    PeepStatistic.superfluous_load -
	    LastPeepStatistic.superfluous_load;
	TotalMemWin += Size;
	DPRINTF (
		 (DEBUG_PEEP,
		  "Rmoved %d times ldi/ldf with never used dest\n", Size));

	Size = PeepStatistic.brd_cycles - LastPeepStatistic.brd_cycles;
	TotalCycleWin = Size;
	Size2 = PeepStatistic.brd_memory - LastPeepStatistic.brd_memory;
	TotalMemWin -= Size2;
	Size1 = PeepStatistic.brd_optimized - LastPeepStatistic.brd_optimized;
	DPRINTF (
		 (DEBUG_PEEP, "Won %d cycles using %d brd, lost %d memory\n",
		  Size, Size1, Size2));

	Size =
	    PeepStatistic.br_to_next_line - LastPeepStatistic.br_to_next_line;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d Branches to next line\n", Size));

	Size =
	    PeepStatistic.bxx_to_next_line -
	    LastPeepStatistic.bxx_to_next_line;
	TotalMemWin += Size;
	DPRINTF (
		 (DEBUG_PEEP, "Removed %d Cond. Branches to next line\n",
		  Size));

	Size = PeepStatistic.bxx_to_ldcond - LastPeepStatistic.bxx_to_ldcond;
	TotalMemWin += Size;
	DPRINTF (
		 (DEBUG_PEEP, "Converted %d Cond. Branches to cond. loads\n",
		  Size));

	Size = PeepStatistic.bxx_over_br - LastPeepStatistic.bxx_over_br;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d bxx over br\n", Size));

	Size =
	    PeepStatistic.br_branch_move - LastPeepStatistic.br_branch_move;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d instr. moving branches\n", Size));

	Size = PeepStatistic.br_commoned - LastPeepStatistic.br_commoned;
	TotalMemWin += Size;
	DPRINTF (
		 (DEBUG_PEEP, "Removed %d instr. commonig up blocks\n",
		  Size));

	Size = PeepStatistic.br_block_move - LastPeepStatistic.br_block_move;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d branches moving blocks\n", Size));

	Size = PeepStatistic.dead_code - LastPeepStatistic.dead_code;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d dead codelines\n", Size));

	Size =
	    PeepStatistic.label_before_branch -
	    LastPeepStatistic.label_before_branch;
	DPRINTF (
		 (DEBUG_PEEP,
		  "Moved %d labels before a branch to dest of branch\n",
		  Size));

	Size =
	    PeepStatistic.label_commoned - LastPeepStatistic.label_commoned;
	DPRINTF ((DEBUG_PEEP, "Commoned up %d labels\n", Size));

	Size = PeepStatistic.label_removed - LastPeepStatistic.label_removed;
	DPRINTF ((DEBUG_PEEP, "Removed %d unused labels\n", Size));

	DPRINTF ((DEBUG_PEEP, "Won total %d words of memory\n", TotalMemWin));
	DPRINTF (
		 (DEBUG_PEEP, "Won total %d cycles with brd (speed) \n",
		  TotalCycleWin));
	DPRINTF ((DEBUG_PEEP, "\n"));

	LastPeepStatistic = PeepStatistic;
#endif /* DEBUG */
    }
#endif /* VERBOSE */
}

#ifdef VERBOSE
/*
 * Gives out some statistics about what the peephole optimizer has done.
 * Called from g_terminate (only for debugging).
 */
void c30_peep_report P0 (void)
{
    int     Size, Size1, Size2;
    int     TotalMemWin;
    int     TotalCycleWin;

    if (verbose_option) {
	Size = PeepStatistic.ldi_rx_rx;
	TotalMemWin = Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d ldi rx,rx\n", Size));

	Size = PeepStatistic.ldf_rx_rx;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d ldf rx,rx\n", Size));

	Size = PeepStatistic.ldi_rx_ry_ldi_ry_rx;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d ldi rx,ry ldi ry,rx\n", Size));

	Size = PeepStatistic.ldf_rx_ry_ldf_ry_rx;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d ldf rx,ry ldf ry,rx\n", Size));

	Size = PeepStatistic.sti_rx_my_ldi_my_rx;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d sti rx,my ldi my,rx\n", Size));

	Size = PeepStatistic.stf_rx_my_ldf_my_rx;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d stf rx,my ldf my,rx\n", Size));

	Size = PeepStatistic.ldi_mx_ry_sti_ry_mx;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d ldi mx,ry sti ry,mx\n", Size));

	Size = PeepStatistic.ldf_mx_ry_stf_ry_mx;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d ldf mx,ry stf ry,mx\n", Size));

	Size = PeepStatistic.commutative_i;
	TotalMemWin += Size;
	DPRINTF (
		 (DEBUG_PEEP, "Removed %d commutative rx,ry ldi ry,rx\n",
		  Size));

	Size = PeepStatistic.commutative_f;
	TotalMemWin += Size;
	DPRINTF (
		 (DEBUG_PEEP, "Removed %d commutative rx,ry ldf ry,rx\n",
		  Size));

	Size = PeepStatistic.ldi_opi_opi3;
	TotalMemWin += Size;
	DPRINTF (
		 (DEBUG_PEEP,
		  "Combined %d ldi X,rn opi Y,rn to opi3 Y,X,rn\n", Size));

	Size = PeepStatistic.ldf_opf_opf3;
	TotalMemWin += Size;
	DPRINTF (
		 (DEBUG_PEEP,
		  "Combined %d ldf X,rn opf Y,rn to opf3 Y,X,rn\n", Size));

	Size = PeepStatistic.opi_ldi_opi3;
	TotalMemWin += Size;
	DPRINTF (
		 (DEBUG_PEEP,
		  "Combined %d opi Y,rn ldi X,rn to opi3 Y,X,rn\n", Size));

	Size = PeepStatistic.opf_ldf_opf3;
	TotalMemWin += Size;
	DPRINTF (
		 (DEBUG_PEEP,
		  "Combined %d opf Y,rn ldf X,rn to opf3 Y,X,rn\n", Size));

	Size = PeepStatistic.redundant_ldi;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d redundant ldi\n", Size));

	Size = PeepStatistic.redundant_ldf;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d redundant ldf\n", Size));
#ifdef PEEP_REDUNDANT_STI
	Size = PeepStatistic.redundant_sti;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d redundant sti\n", Size));

	Size = PeepStatistic.redundant_stf;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d redundant stf\n", Size));
#endif
	Size = PeepStatistic.par_ldf_stf;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Replaced %d par_ldf_stf\n", Size));

	Size = PeepStatistic.par_stf_stf;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Replaced %d par_stf_stf\n", Size));

	Size = PeepStatistic.par_ldf_ldf;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Replaced %d par_ldf_ldf\n", Size));

	Size = PeepStatistic.par_ldi_sti;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Replaced %d par_ldi_sti\n", Size));

	Size = PeepStatistic.par_sti_sti;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Replaced %d par_sti_sti\n", Size));

	Size = PeepStatistic.par_ldi_ldi;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Replaced %d par_ldi_ldi\n", Size));

	Size = PeepStatistic.remap_register_fwd;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Remapped %d times register fwd\n", Size));

	Size = PeepStatistic.remap_register_bwd;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Remapped %d times register bwd\n", Size));

	Size = PeepStatistic.superfluous_load;
	TotalMemWin += Size;
	DPRINTF (
		 (DEBUG_PEEP,
		  "Removed %d times ldi/ldf with never used dest\n", Size));

	Size = PeepStatistic.superfluous_tst;
	Size2 = PeepStatistic.superfluous_cmpf;
	TotalMemWin += Size + Size2;
	DPRINTF (
		 (DEBUG_PEEP,
		  "Removed superfluous tst/tst3 (%d)/cmpf 0.0,Rn(%d)\n", Size,
		  Size2));

	Size = PeepStatistic.brd_cycles;
	TotalCycleWin = Size;
	Size2 = PeepStatistic.brd_memory;
	TotalMemWin -= Size2;
	Size1 = PeepStatistic.brd_optimized;
	DPRINTF (
		 (DEBUG_PEEP, "Won %d cycles using %d brd, lost %d memory\n",
		  Size, Size1, Size2));

	Size = PeepStatistic.br_to_next_line;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d Branches to next line\n", Size));

	Size = PeepStatistic.bxx_to_next_line;
	TotalMemWin += Size;
	DPRINTF (
		 (DEBUG_PEEP, "Removed %d Cond. Branches to next line\n",
		  Size));

	Size = PeepStatistic.bxx_to_ldcond;
	TotalMemWin += Size;
	DPRINTF (
		 (DEBUG_PEEP, "Converted %d Cond. Branches to cond. loads\n",
		  Size));

	Size = PeepStatistic.bxx_over_br;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d bxx over br\n", Size));

	Size = PeepStatistic.br_block_move;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d branches moving blocks\n", Size));

	Size = PeepStatistic.br_branch_move;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d instr. moving branches\n", Size));

	Size = PeepStatistic.br_commoned;
	TotalMemWin += Size;
	DPRINTF (
		 (DEBUG_PEEP, "Removed %d instr. commoning up blocks\n",
		  Size));

	Size = PeepStatistic.dead_code;
	TotalMemWin += Size;
	DPRINTF ((DEBUG_PEEP, "Removed %d dead codelines\n", Size));

	Size = PeepStatistic.label_before_branch;
	DPRINTF (
		 (DEBUG_PEEP,
		  "Moved %d labels before a branch to dest of branch\n",
		  Size));

	Size = PeepStatistic.label_commoned;
	DPRINTF ((DEBUG_PEEP, "Commoned up %d labels\n", Size));

	Size = PeepStatistic.label_removed;
	DPRINTF ((DEBUG_PEEP, "Removed %d unused labels\n", Size));

	DPRINTF ((DEBUG_PEEP, "Won total %d words of memory\n", TotalMemWin));
	DPRINTF (
		 (DEBUG_PEEP, "Won total %d cycles with brd (speed) \n",
		  TotalCycleWin));
	DPRINTF ((DEBUG_PEEP, "\n"));
	DPRINTF (
		 (DEBUG_PEEP,
		  "Codewords      reduced from %5d to %5d instructions (%3d%%)\n",
		  oldtot, newtot,
		  ((oldtot - newtot) * 100) / ((oldtot != 0) ? oldtot : 1)));
	DPRINTF (
		 (DEBUG_PEEP,
		  "Asmstatements  reduced from %5d to %5d instructions \n",
		  oldasm, newasm));
	DPRINTF (
		 (DEBUG_PEEP,
		  "Pipelinedelays reduced from %5d to %5d Cycles  \n",
		  oldreg + oldbranch, newreg + newbranch));
	DPRINTF (
		 (DEBUG_PEEP,
		  "     Registers reduced from %5d to %5d Cycles  \n", oldreg,
		  newreg));
	DPRINTF (
		 (DEBUG_PEEP,
		  "     Branches  reduced from %5d to %5d Cycles  \n",
		  oldbranch, newbranch));
	DPRINTF ((DEBUG_PEEP, "Times:\n"));
	DPRINTF (
		 (DEBUG_PEEP,
		  "delay  tstb   pipe   paral  remap  ldi    branch cmpi   flow\n"));
	DPRINTF (
		 (DEBUG_PEEP,
		  "%6ld %6ld %6ld %6ld %6ld %6ld %6ld %6ld %6ld\n\n",
		  delay_time, tstb_time, pipe_time, paralel_time, remap_time,
		  ldi_time, branch_time, cmpi_time, flow_time));

    }
}
#endif /* VERBOSE */

#endif /* TMS320C30 */
