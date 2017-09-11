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
 *
 *  Examines dataflow and checks if data is already in a register
 *  when read-accesses to external ram occurs
 *  needs multiple-passes to work
 */

/*****************************************************************************/

#include "config.h"

#ifdef PEEPFLOW
#ifdef TMS320C30
#include "chdr.h"
#include "expr.h"
#include "cglbdec.h"
#include "proto.h"
#include "genc30.h"
#include "outproto.h"

/********************************************************* Macro Definitions */

#define ENHANCED_FLOW

/*****************************************************************************/

#ifdef DEBUG
/*
 *  To print out the Regusage into assembler-output
 *  Just to check if everything works as I thought
 */

static void print_flowlist P1 (FLOWLISTENTRY *, list)
{
    FLOWLISTENTRY *p;

    for (p = list; p != NULL; p = p->next) {
	putamode_tst (p->ap);
#ifdef NO_MIX_WITH_FLOAT
	if (p->type == OP_FLOAT) {
	    dprintf (DEBUG_FLOW, " [F]; ");
	} else {
	    dprintf (DEBUG_FLOW, " [I]; ");
	}
#else
	dprintf (DEBUG_FLOW, "; ");
#endif
    }
}

/*
 *  To to print out the Regusage into assembler-output
 *  Just to check if everything works as I thought
 */

static void print_regusage P1 (FLOWLISTENTRY **, RegUsage)
{
    REG     reg;

    for (reg = REG_R0; reg <= MAX_REG; reg++) {
	if (RegUsage[reg] != NULL) {
	    putreg_tst (reg);
	    dprintf (DEBUG_FLOW, ": ");
	    print_flowlist (RegUsage[reg]);
	    dprintf (DEBUG_FLOW, "\n");
	}
    }
}
#endif /* DEBUG */

/*
 * Searchs the address 'ap' in the flowlist 'head' and and 
 * returns a pointer to it (or NULL if not found)
 */

static FLOWLISTENTRY *search_flowlist_entry P3 (FLOWLISTENTRY *, head,
						ADDRESS *, ap, ITYPE, type)
{
    FLOWLISTENTRY *p;

#ifdef NO_MIX_WITH_FLOAT
    ITYPE   TypeOfThisList;

    if (head != NULL) {
	TypeOfThisList = head->type;
    }
#endif
    for (p = head; p != NULL; p = p->next) {
#ifdef NO_MIX_WITH_FLOAT
	if (TypeOfThisList != p->type) {
	    FATAL (
		   (__FILE__, "search_flowlist_entry",
		    "mixed int and float in list"));
	}
#endif
	if (is_equal_address (p->ap, ap)) {
#ifdef NO_MIX_WITH_FLOAT
	    if (p->type == type) {
		return p;
	    }
#else
	    return p;
#endif
	}
    }
    return NULL;
}

/*
 * Adds the address 'ap' to the flowlist of register 'reg' in regusage
 * unsuitable 'ap's and double-entries are supressed 
 */

static void add_flow_list P4 (FLOWLISTENTRY **, RegUsage, REG, reg, ADDRESS *,
			      ap, ITYPE, type)
{
    FLOWLISTENTRY *Entry;
    ADDRESS *ap1;
    IVAL    offset;

    if ((ap->mode == am_ainc) || (ap->mode == am_adec)) {
	/* ainc could be treated as am_indx with offset = -increment
	 * in list, sinc addressregister points increment farther than
	 * value was written
	 * except it was stored with itself (arn, *arn++)
	 */
	if ((ap->preg == reg)
	    || (ap->u.offset->nodetype != en_icon)) {
	    return;
	}
	ap1 = copy_addr (ap, am_indx);
	offset =
	    (ap->mode == am_ainc) ? -ap->u.offset->v.i : ap->u.offset->v.i;
	ap1->u.offset = mk_const ((long) offset);
	ap = ap1;
    }
    if ((ap->mode == am_preinc) || (ap->mode == am_predec)) {
	/* preinc could be treated as am_ind in list, sinc
	 * addressregister points still to the same location
	 * except it was stored with itself (arn, *++arn)
	 */
	if (ap->preg == reg) {
	    return;
	}
	ap = copy_addr (ap, am_ind);
    }
    /*
     * check if equivalent entry is already in list
     */
    if (search_flowlist_entry (*RegUsage, ap, type) == NULL) {
	Entry = (FLOWLISTENTRY *) xalloc ((size_t) sizeof (FLOWLISTENTRY));
	Entry->ap = ap;
#ifdef NO_MIX_WITH_FLOAT
	Entry->type = type;
#endif
	Entry->next = *RegUsage;
	*RegUsage = Entry;
    }
}

/*
 * adds the flowlist 'dest' to the flowlist '*regusage', double-
 * entries are supressed
 */

static void add_list_to_flow_list P2 (FLOWLISTENTRY **, RegUsage,
				      FLOWLISTENTRY *, src)
{
    FLOWLISTENTRY *Entry;
    FLOWLISTENTRY *p;

    for (p = src; p != NULL; p = p->next) {

	/*
	 * check if equivalent entry is already in list
	 */
	if (search_flowlist_entry (*RegUsage, p->ap, p->type) == NULL) {
	    Entry =
		(FLOWLISTENTRY *) xalloc ((size_t) sizeof (FLOWLISTENTRY));
	    Entry->ap = p->ap;
#ifdef NO_MIX_WITH_FLOAT
	    Entry->type = p->type;
#endif
	    Entry->next = *RegUsage;
	    *RegUsage = Entry;
	}
    }
}

/*
 * clears the ap-list of the register reg and adds the entry ap
 * if it is suitable (not if it refers to our own register)
 * (ldi *ar2, ar2, ar2 is changed during the operation)
 * autoincrements are also not suitable, they never point
 * to the same value the next time
 */

static void new_flow_list P4 (FLOWLISTENTRY **, RegUsage, REG, reg, ADDRESS *,
			      ap, ITYPE, type)
{
    FLOWLISTENTRY *Entry;
    ADDRESS *ap1;
    IVAL    offset;

    switch (ap->mode) {
    case am_indx2:
    case am_indxs:
	if (ap->sreg == reg) {
	    *RegUsage = NULL;
	    return;
	}
	/*FALLTHRU */
    case am_ind:
    case am_const_ind:
    case am_indx:
	if (ap->preg == reg) {
	    *RegUsage = NULL;
	    return;
	}
	break;
    case am_preinc:
    case am_predec:
	/*
	 * preinc could be treated as am_ind in list, sinc
	 * addressregister points still to the same location
	 * except it was loaded with itself (*++arn, arn)
	 */
	if (ap->preg == reg) {
	    *RegUsage = NULL;
	    return;
	}
	ap = copy_addr (ap, am_ind);
	break;
    case am_ainc:
    case am_adec:
	/*
	 * ainc could be treated as am_indx with offset = -increment
	 * in list, sinc addressregister points increment farther than
	 * value was written
	 * except it was loaded with itself (*arn++, arn)
	 */
	if ((ap->preg == reg)
	    || (ap->u.offset->nodetype != en_icon)) {
	    *RegUsage = NULL;
	    return;
	}
	ap1 = copy_addr (ap, am_indx);
	offset =
	    (ap->mode == am_ainc) ? -ap->u.offset->v.i : ap->u.offset->v.i;
	ap1->u.offset = mk_const (offset);
	ap = ap1;
	break;

    default:
	break;
    }

    Entry = (FLOWLISTENTRY *) xalloc ((size_t) sizeof (FLOWLISTENTRY));
    Entry->ap = ap;
#ifdef NO_MIX_WITH_FLOAT
    Entry->type = type;
#endif
    Entry->next = NULL;
    *RegUsage = Entry;
}

/* 
 * Generates a duplicate of the delivered flowlist
 * and returns a pointer to it
 */

static FLOWLISTENTRY *copy_flow_list P1 (FLOWLISTENTRY *, List)
{
    FLOWLISTENTRY *NewList = NULL;
    FLOWLISTENTRY *Entry = NULL;
    FLOWLISTENTRY *p;

    for (p = List; p != NULL; p = p->next) {
	Entry = (FLOWLISTENTRY *) xalloc ((size_t) sizeof (FLOWLISTENTRY));
	Entry->ap = p->ap;
#ifdef NO_MIX_WITH_FLOAT
	Entry->type = p->type;
#endif
	Entry->next = NewList;
	NewList = Entry;
    }
    return NewList;
}

/*
 * Only entries contained in both lists are delivered back in dest
 * (the biggest common set is returned in dest)
 *
 * Returns the amount of changes which have been done.
 * Is needed for the multipass, we are looping through the code
 * until no more changes are done
 */


static int merge_flow_list P2 (FLOWLISTENTRY **, dest, FLOWLISTENTRY *, src)
{
    FLOWLISTENTRY *Last = NULL;
    FLOWLISTENTRY *p;
    int     changes = 0;

    for (p = *dest; p != NULL; p = p->next) {
	if (search_flowlist_entry (src, p->ap, p->type) == NULL) {
	    changes++;
	    if (Last == NULL) {
		*dest = p->next;
	    } else {
		Last->next = p->next;
	    }
	}
	Last = p;
    }
    return changes;
}

/*
 * removes all entries from dest-flowlist which references
 * register reg in any way
 * REG_MEMORY is a pseudo-reg and means any memory-access
 */

static void remove_from_flowlist P2 (FLOWLISTENTRY **, dest, REG, reg)
{
    FLOWLISTENTRY *Last = NULL;
    FLOWLISTENTRY *p;

    if (reg == REG_MEMORY) {
	for (p = *dest; p != NULL; p = p->next) {
	    if ((p->ap->mode == am_indx)
		|| (p->ap->mode == am_ind)
		/* || (p->ap->mode == am_const_ind) */
		|| (p->ap->mode == am_direct)
		/* || (p->ap->mode == am_const_direct) */
		|| (p->ap->mode == am_ainc)
		|| (p->ap->mode == am_adec)
		|| (p->ap->mode == am_preinc)
		|| (p->ap->mode == am_predec)
		|| (p->ap->mode == am_indx2)
		|| (p->ap->mode == am_indxs)) {
		if (Last == NULL) {
		    *dest = p->next;
		} else {
		    Last->next = p->next;
		}
	    }
	    Last = p;
	}
    } else {
	for (p = *dest; p != NULL; p = p->next) {
	    if (is_register_used (reg, p->ap)) {
		if (Last == NULL) {
		    *dest = p->next;
		} else {
		    Last->next = p->next;
		}
	    }
	    Last = p;
	}
    }
}

/*
 * clears the whole regusage-list
 */

static void clear_regusage P1 (FLOWLISTENTRY **, RegUsage)
{
    REG     reg;

    for (reg = REG_R0; reg <= MAX_REG; reg++) {
	RegUsage[reg] = NULL;
    }
}

/*
 * Searchs if any register already contains the value corresponding
 * to ap. if a register is found, its number is returned, otherewise
 * -1 is returnned
 */

static REG search_in_regusage P3 (FLOWLISTENTRY **, RegUsage, ADDRESS *, ap,
				  ITYPE, type)
{
    REG     reg;

    for (reg = REG_R0; reg <= MAX_REG; reg++) {
	if (search_flowlist_entry (RegUsage[reg], ap, type) != NULL) {
	    return reg;
	}
    }
    return NO_REG;
}

/*
 * Produces a copy of an registerusage-list and returns a pointer to it
 */

static FLOWLISTENTRY **copy_regusage P1 (FLOWLISTENTRY **, RegUsage)
{
    FLOWLISTENTRY **NewRegUsage;
    FLOWLISTENTRY **p;
    REG     reg;

    NewRegUsage = (FLOWLISTENTRY **) xalloc ((size_t)
					     sizeof (FLOWLISTENTRY *
						     [MAX_REG + 1]));
    for (reg = REG_R0, p = NewRegUsage; reg <= MAX_REG; reg++, p++) {
	*p = copy_flow_list (RegUsage[reg]);
    }
    return NewRegUsage;
}

/*
 * Merges the biggest common registerusage-set from dst and src 
 * into src
 *
 * Returns the amount of changes which have been done.
 * Is needed for the multipass, we are looping through the code
 * until no more changes are done
 */

static int merge_regusage P2 (FLOWLISTENTRY **, dest, FLOWLISTENTRY **, src)
{
    REG     reg;
    int     changes = 0;

    for (reg = REG_R0; reg <= MAX_REG; reg++) {
	changes += merge_flow_list (&(dest[reg]), src[reg]);
    }
    return changes;
}

/*
 * Removes all references to and from register reg
 */

static void remove_from_regusage P2 (FLOWLISTENTRY **, RegUsage, REG, reg)
{
    REG     r;

    if (reg != REG_MEMORY) {
	RegUsage[reg] = NULL;
    }
    for (r = REG_R0; r <= MAX_REG; r++) {
	remove_from_flowlist (&(RegUsage[r]), reg);
    }
}

/*
 * Removes all references to and from all temporary registers
 */

static void clear_tempreg_usage P1 (FLOWLISTENTRY **, RegUsage)
{
    REG     reg;

    for (reg = REG_R0; reg <= MAX_DATA; reg++) {
	RegUsage[reg] = NULL;
	remove_from_regusage (RegUsage, reg);
    }
    for (reg = REG_AR0; reg <= MAX_ADDR; reg++) {
	RegUsage[reg] = NULL;
	remove_from_regusage (RegUsage, reg);
    }
}

/*
 * Attachs the Regusagelist to the Label which is destination of
 * ip (ip must be jump or branch)
 * The usagelist of the label is corrected to the biggest common set
 * of regusage and the old List of the label.
 * 
 * Returns the amount of changes which have been done.
 * Is needed for the multipass, we are looping through the code
 * until no more changes are done
 */

static int attach_regusage_to_label P3 (CODE *, ip, FLOWLISTENTRY **,
					RegUsage, int, level)
{
    LABEL   label;
    CODE   *target;
    unsigned i;
    int     count;
    SWITCH *sw;
    SWITCH *table;


    if (ip->opcode != op_bu) {
	if (ip->src1->mode != am_immed) {
	    return 0;
	}
	label = ip->src1->u.offset->v.l;
	target = find_label (label);
	if (target == NULL) {
	    FATAL (
		   (__FILE__, "attach_regusage_to_label",
		    "target not found"));
	}
	if (target->src21 == NULL) {
	    target->src21 = (ADDRESS *) copy_regusage (RegUsage);
	    return 1;
	} else {
	    return merge_regusage ((FLOWLISTENTRY **) target->src21,
				   RegUsage);
	}
    } else {
	if (!is_peep_phase (level, PEEP_SWITCH)) {
	    return 0;
	}
	/* look for the switchtable belonging to that op_bu */
	table = NIL_SWITCH;
	for (sw = swtables; sw != NIL_SWITCH; sw = sw->next) {
	    /* genc30 adds the labelname into the offsetfield of src1 */
	    if (ip->src1->u.offset->v.l == sw->tablab) {
		table = sw;
		break;
	    }
	}
	if (table != NIL_SWITCH) {
	    count = 0;
	    /* attach flowinfo to all labels */
	    for (i = 0; i < table->numlabs; i++) {
		label = table->labels[i];
		target = find_label (label);
		if (target == NULL) {
		    FATAL (
			   (__FILE__, "attach_regusage_to_label",
			    "switch_target not found"));
		}
		if (target->src21 == NULL) {
		    target->src21 = (ADDRESS *) copy_regusage (RegUsage);
		    count++;
		} else {
		    count +=
			merge_regusage ((FLOWLISTENTRY **) target->src21,
					RegUsage);
		}
	    }
	    return count;
	} else {
	    FATAL (
		   (__FILE__, "attach_regusage_to_label",
		    "switchtable for bu not found"));
	    return 1;
	}

    }

}

/* 
 * Merges the Regusagelist from the Label ip (ip must be a label)
 * and the Regusage-List (result is the biggest common set of both
 * lists.)
 * Both Lists are overwritten with the newly generated common List 
 * If the label is Referenced in an switch-table, an empty-list
 * is provided, since we dont know anything about registerusage
 * after a switch. 
 *
 * Returns the amount of changes which have been done.
 * Is needed for the multipass, we are looping through the code
 * until no more changes have been done
 */

static int merge_regusage_from_label P3 (CODE *, ip, FLOWLISTENTRY **,
					 RegUsage, int, level)
{
    LABEL   label;
    SWITCH *sw;
    int     found;

    label = ip->src1->u.offset->v.l;

    found = 0;
    if (!is_peep_phase (level, PEEP_SWITCH)) {
	for (sw = swtables; sw != (SWITCH *) NULL; sw = sw->next) {
	    LABEL   i;

	    for (i = 0; i < sw->numlabs; i++) {
		if (sw->labels[i] == label) {
		    found++;
		}
	    }
	}
    }
    if (found != 0) {
	clear_regusage (RegUsage);
	if (ip->src21 == NULL) {
	    ip->src21 = (ADDRESS *) copy_regusage (RegUsage);
	    return 1;
	} else {
	    clear_regusage (RegUsage);
	    return merge_regusage ((FLOWLISTENTRY **) ip->src21, RegUsage);
	}
    } else {
	if (ip->src21 == NULL) {
	    ip->src21 = (ADDRESS *) copy_regusage (RegUsage);
	    return 1;
	} else {
	    found = merge_regusage ((FLOWLISTENTRY **) ip->src21, RegUsage);
	    VOIDCAST merge_regusage (RegUsage, (FLOWLISTENTRY **) ip->src21);

	    return found;
	}
    }
}

/* 
 * Copies the Regusagelist from the Label ip (ip must be a label)
 * to the Regusage-List (old contents are overwritten)
 * If the label is Refrenced in an switch-table, an empty-list
 * is provided, since we dont know anything about registerusage
 * after an switch. 
 */

static void copy_regusage_from_label P3 (CODE *, ip, FLOWLISTENTRY **,
					 RegUsage, int, level)
{
    LABEL   label;
    SWITCH *sw;
    int     found;
    FLOWLISTENTRY **p;
    FLOWLISTENTRY **src;

    label = ip->src1->u.offset->v.l;

    found = 0;
    if (!is_peep_phase (level, PEEP_SWITCH)) {
	for (sw = swtables; sw != (SWITCH *) NULL; sw = sw->next) {
	    LABEL   i;

	    for (i = 0; i < sw->numlabs; i++) {
		if (sw->labels[i] == label) {
		    found++;
		}
	    }
	}
    }
    if (found != 0) {
	clear_regusage (RegUsage);
	if (ip->src21 == NULL) {
	    ip->src21 = (ADDRESS *) copy_regusage (RegUsage);
	    return;
	} else {
	    clear_regusage ((FLOWLISTENTRY **) ip->src21);
	    return;
	}
    } else {
	if (ip->src21 == NULL) {
	    clear_regusage (RegUsage);
	    return;
	} else {
	    REG     r;

	    src = (FLOWLISTENTRY **) ip->src21;
	    for (r = REG_R0, p = RegUsage; r <= MAX_REG; r++, p++) {
		*p = copy_flow_list (src[r]);
	    }
	}
    }
}


/*
 * Invalidates (throws out) all references in registers and
 * to registers, which are changed by the instruction ip, 
 * including memory references
 */

static void validate_regusage P2 (CODE *, ip, FLOWLISTENTRY **, RegUsage)
{
#ifdef SAVE_PEEP_MEMORY
    PEEPINFO StaticMemory;

#endif /* SAVE_PEEP_MEMORY */
    PEEPINFO *map;
    REGBITMAP changed;
    REGBITMAP current = 1;
    REG     r;

    map = GET_PEEP_INFO (ip, StaticMemory);
    changed = map->write | map->modified | map->updated;
    for (r = REG_R0; r <= MAX_REG; r++) {
	if ((changed & current) != 0) {
	    remove_from_regusage (RegUsage, r);
	}
	current <<= 1;
    }
    if ((changed & (1UL << REG_MEMORY)) != 0) {
	remove_from_regusage (RegUsage, REG_MEMORY);
    }
}

/*
 * Invalidates (throws out) all references in registers and
 * to registers, which are updated during addressgeneration of 
 * the instruction ip
 */
#ifdef ENHANCED_FLOW

static void validate_addressregusage P2 (CODE *, ip, FLOWLISTENTRY **,
					 RegUsage)
{
#ifdef SAVE_PEEP_MEMORY
    PEEPINFO StaticMemory;

#endif /* SAVE_PEEP_MEMORY */
    PEEPINFO *map;
    REGBITMAP changed;
    REGBITMAP current = 1;
    REG     r;

    map = GET_PEEP_INFO (ip, StaticMemory);
    changed = map->updated;
    for (r = REG_R0; r <= MAX_REG; r++) {
	if ((changed & current) != 0) {
	    remove_from_regusage (RegUsage, r);
	}
	current <<= 1;
    }
}
#endif

/* Is called if an amode could be exchanged with register reg 
 * that is if both contain the same value
 * 
 * We do the replace only if we think it useful
 * We replace following cases
 * Old               New
 * Temporary D-Reg   D-Reg
 * Temporary A-Reg   Not A-Reg  Reduces Pipelincconflicts
 * Memory            Any Reg    Avoids Memory-access
 *                              frees busbandwith
 * 
 */

static BOOL is_exchangable P2 (ADDRESS **, apptr, REG, reg)
{
    ADDRESS *ap = *apptr;

    switch (ap->mode) {
    case am_dreg:
    case am_freg:
	if ((ap->preg <= MAX_DATA) && (is_data_register (reg))
	    /* 
	     * to avoid endless exchanges do replaces only higher 
	     * registernumbers with lower registernumbers
	     */
	    && (reg < ap->preg)) {
	    /*
	     * replace only if temporary
	     */

	    *apptr = mk_reg (reg);
	    (*apptr)->mode = ap->mode;
	    DPRINTF ((DEBUG_FLOW, " and replaced\n"));
	    return TRUE;
	}
	break;
    case am_ireg:
    case am_sreg:
	break;
    case am_areg:
	if (((!is_address_register (reg)) || (ap->preg <= MAX_ADDR))
	    /* 
	     * to avoid endless exchanges do replaces only higher 
	     * registernumbers with lower registernumbers
	     */
	    && ((reg < ap->preg) || (is_index_register (reg)))) {
	    *apptr = mk_reg (reg);
	    DPRINTF ((DEBUG_FLOW, " and replaced\n"));
	    return TRUE;
	}
	break;
    default:
	*apptr = mk_reg (reg);
	DPRINTF ((DEBUG_FLOW, " and replaced\n"));
	return TRUE;
    }
    DPRINTF ((DEBUG_FLOW, " but not changed\n"));
    return FALSE;
}

/*
 *  Examines dataflow and checks if data is already in a register
 *  when read-accesses to external ram occurs
 *  needs multiple-passes to work
 */
int flow_dataflow P2 (CODE *, peep_head, int, level)
{
    FLOWLISTENTRY *RegUsage[MAX_REG + 1];
    int     LabelChanges;
    BOOL    headless;		/* is true when currently no entrypoint for code was found 

				 * (after a jump before the first label is found 
				 */
    REG     reg;
    register CODE *ip;
    int     replacecount = 0;
    static int totreplace = 0;

#ifdef DEBUG
    int     Round = 1;

#endif /* DEBUG */

    do {
	DPRINTF ((DEBUG_FLOW, "Dataflow, round %d...", Round++));
	LabelChanges = 0;
	/* currently no entrypoint found */
	headless = FALSE;
	/* clear all lists to start analysis */
	clear_regusage (&(RegUsage[0]));
	for (ip = peep_head; ip != NIL_CODE; ip = ip->fwd) {
	    /*
	     * invalidate all entries with references to registers
	     * modified in this instruction
	     */
	    validate_regusage (ip, &(RegUsage[0]));

	    switch (ip->opcode) {
	    case op_ldi_ldi:
	    case op_ldf_ldf:
		/* destination now references to source */
		new_flow_list (&(RegUsage[ip->dst2->preg]), ip->dst2->preg,
			       ip->src21, ip->type);
		if (is_am_register (ip->src21)) {
		    /* dest references now to the same list of values as source */
		    add_list_to_flow_list (&(RegUsage[ip->dst2->preg]),
					   RegUsage[ip->src21->preg]);
		    /* source refers now also to destination */
		    add_flow_list (&(RegUsage[ip->src21->preg]),
				   ip->src21->preg, ip->dst2, ip->type);
		}
		/*FALLTHRU */
	    case op_ldi:
	    case op_ldf:
	    case op_ldiu:
	    case op_ldfu:
		/* destination now references to source */
		new_flow_list (&(RegUsage[ip->dst->preg]), ip->dst->preg,
			       ip->src1, ip->type);
		if (is_am_register (ip->src1)) {
		    /* dest references now to the same list of values as source */
		    add_list_to_flow_list (&(RegUsage[ip->dst->preg]),
					   RegUsage[ip->src1->preg]);
		    /* source refers now also to destination */
		    add_flow_list (&(RegUsage[ip->src1->preg]),
				   ip->src1->preg, ip->dst, ip->type);
		}
		break;

	    case op_ldi_sti:
	    case op_ldf_stf:
		/* destination now references to source */
		new_flow_list (&(RegUsage[ip->dst->preg]), ip->dst->preg,
			       ip->src1, ip->type);
		if (is_am_register (ip->src1)) {
		    /* dest references now to the same list of values as source */
		    add_list_to_flow_list (&(RegUsage[ip->dst->preg]),
					   RegUsage[ip->src1->preg]);
		    /* source refers now also to destination */
		    add_flow_list (&(RegUsage[ip->src1->preg]),
				   ip->src1->preg, ip->dst, ip->type);
		}
		/* source of sti refers now also to destination */
		add_flow_list (&(RegUsage[ip->src21->preg]), ip->src21->preg,
			       ip->dst2, ip->type);
		break;

	    case op_sti_sti:
	    case op_stf_stf:
		/* source refers now also to destination */
		add_flow_list (&(RegUsage[ip->src2->preg]), ip->src2->preg,
			       ip->dst2, ip->type);
		/*FALLTHRU */
	    case op_sti:
	    case op_stf:
		/* source refers now also to destination */
		add_flow_list (&(RegUsage[ip->src1->preg]), ip->src1->preg,
			       ip->dst, ip->type);
		break;

	    case op_absf_stf:
	    case op_absi_sti:
	    case op_addf3_stf:
	    case op_addi3_sti:
	    case op_and3_sti:
	    case op_ash3_sti:
	    case op_fix_sti:
	    case op_float_stf:
	    case op_lsh3_sti:
	    case op_mpyf3_stf:
	    case op_mpyi3_sti:
	    case op_negf_stf:
	    case op_negi_sti:
	    case op_not_sti:
	    case op_or3_sti:
	    case op_subf3_stf:
	    case op_subi3_sti:
	    case op_xor3_sti:
		/* source of sti refers now also to destination */
		add_flow_list (&(RegUsage[ip->src2->preg]), ip->src2->preg,
			       ip->dst2, ip->type);
		if ((ip->dst != NULL) && (is_am_register (ip->dst))) {
		    /* destination of operation does not reference to anywhere now */
		    RegUsage[ip->dst->preg] = NULL;
		}
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
		/* attach aktual registerusageinfo to label of jumpdestination */
		LabelChanges +=
		    attach_regusage_to_label (ip, &(RegUsage[0]), level);
		break;

	    case op_label:
		if (headless == TRUE) {
		    /* first entrypoint of this piece of code, so use whole information from label */
		    copy_regusage_from_label (ip, &(RegUsage[0]), level);
		    headless = FALSE;
		} else {
		    /* not first entrypoint, so merge information from label */
		    LabelChanges +=
			merge_regusage_from_label (ip, &(RegUsage[0]), level);
		}
		break;

	    case op_bu:
	    case op_br:
		/* attach aktual registerusageinfo to label of jumpdestination */
		LabelChanges +=
		    attach_regusage_to_label (ip, &(RegUsage[0]), level);
		/* FALLTHRU */

	    case op_retsu:
	    case op_retiu:
		/* no information abaout registers availlable now */
		clear_regusage (&(RegUsage[0]));
		/* since no entrypoint found now (unconditional controltransfer) */
		headless = TRUE;
		break;

	    case op_asm:
		/* clear whole list, asm can do anything to registers and memory */
		clear_regusage (&(RegUsage[0]));
		break;

	    case op_rpts:
	    case op_rptb:
		/* remove all references to and from blockrepeatregisters */
		RegUsage[REG_RC] = NULL;
		RegUsage[REG_RS] = NULL;
		RegUsage[REG_RE] = NULL;
		remove_from_regusage (RegUsage, REG_RC);
		remove_from_regusage (RegUsage, REG_RS);
		remove_from_regusage (RegUsage, REG_RE);
		break;

	    case op_call:
	    case op_trapu:
	    case op_xcall:
	    case op_callu:
		/* remove all references to and from temporary registers */
		clear_tempreg_usage (&(RegUsage[0]));
		/*
		 * we do not know what called routine does
		 * with memory
		 */
		remove_from_regusage (RegUsage, REG_MEMORY);
		/* FALLTHRU */

	    case op_push:
	    case op_pop:
	    case op_pushf:
	    case op_popf:
	    case op_pushnopeep:
	    case op_pushfnopeep:
		/* references from SP are no longer valid */
		RegUsage[REG_SP] = NULL;
		/* FALLTHRU */

	    default:
		if ((ip->dst != NULL) && (is_am_register (ip->dst))) {
		    /* destination of operation does not reference to anywhere now */
		    RegUsage[ip->dst->preg] = NULL;
		}
		if ((ip->dst2 != NULL) && (is_am_register (ip->dst2))) {
		    /* destination of operation does not reference to anywhere now */
		    RegUsage[ip->dst2->preg] = NULL;
		}
		break;

	    }
	}
	DPRINTF ((DEBUG_FLOW, " %d changes \n", LabelChanges));
    } while (LabelChanges != 0);

    DPRINTF ((DEBUG_FLOW, "%s\n", "Dataflow, REPLACEMENT ROUND"));
    LabelChanges = 0;
    /* currently no entrypoint found */
    headless = FALSE;
    /* clear all lists to start analysis */
    clear_regusage (&(RegUsage[0]));
    for (ip = peep_head; ip != NIL_CODE; ip = ip->fwd) {
	/*
	 * invalidate all entries with references to registers
	 * modified in this instruction
	 */
#ifdef ENHANCED_FLOW
	validate_addressregusage (ip, &(RegUsage[0]));
#else
	validate_regusage (ip, &(RegUsage[0]));
#endif
	switch (ip->opcode) {
	case op_ldi_ldi:
	case op_ldf_ldf:
	    if (
		(reg =
		 search_in_regusage (&(RegUsage[0]), ip->src1,
				     ip->type)) >= 0) {
		DPRINTF (
			 (DEBUG_FLOW, "FoundReplacement for src1 %d",
			  (int) reg));
		if (is_exchangable (&(ip->src1), reg)) {
		    replacecount++;
		    totreplace++;
		    Update_Peep_Info (ip);
		}
	    }
	    if (
		(reg =
		 search_in_regusage (&(RegUsage[0]), ip->src21,
				     ip->type)) >= 0) {
		DPRINTF (
			 (DEBUG_FLOW, "FoundReplacement for src21 %d",
			  (int) reg));
		if (is_exchangable (&(ip->src21), reg)) {
		    replacecount++;
		    totreplace++;
		    Update_Peep_Info (ip);
		}
	    }
#ifdef ENHANCED_FLOW
	    validate_regusage (ip, &(RegUsage[0]));
#endif
	    /* destination now references to source */
	    new_flow_list (&(RegUsage[ip->dst->preg]), ip->dst->preg,
			   ip->src1, ip->type);
	    if (is_am_register (ip->src1)) {
		/* dest references now to the same list of values as source */
		add_list_to_flow_list (&(RegUsage[ip->dst->preg]),
				       RegUsage[ip->src1->preg]);
		/* source refers now also to destination */
		add_flow_list (&(RegUsage[ip->src1->preg]), ip->src1->preg,
			       ip->dst, ip->type);
	    }
	    /* destination now references to source */
	    new_flow_list (&(RegUsage[ip->dst2->preg]), ip->dst2->preg,
			   ip->src21, ip->type);
	    if (is_am_register (ip->src21)) {
		/* dest references now to the same list of values as source */
		add_list_to_flow_list (&(RegUsage[ip->dst2->preg]),
				       RegUsage[ip->src21->preg]);
		/* source refers now also to destination */
		add_flow_list (&(RegUsage[ip->src21->preg]), ip->src21->preg,
			       ip->dst2, ip->type);
	    }
	    break;

	case op_ldi:
	case op_ldf:
	case op_ldiu:
	case op_ldfu:
	    if (
		(reg =
		 search_in_regusage (&(RegUsage[0]), ip->src1,
				     ip->type)) >= 0) {
		DPRINTF (
			 (DEBUG_FLOW, "FoundReplacement for src1 %d",
			  (int) reg));
		if (is_exchangable (&(ip->src1), reg)) {
		    replacecount++;
		    totreplace++;
		    Update_Peep_Info (ip);
		}
	    }
#ifdef ENHANCED_FLOW
	    validate_regusage (ip, &(RegUsage[0]));
#endif
	    /* destination now references to source */
	    new_flow_list (&(RegUsage[ip->dst->preg]), ip->dst->preg,
			   ip->src1, ip->type);
	    if (is_am_register (ip->src1)) {
		/* dest references now to the same list of values as source */
		add_list_to_flow_list (&(RegUsage[ip->dst->preg]),
				       RegUsage[ip->src1->preg]);
		/* source refers now also to destination */
		add_flow_list (&(RegUsage[ip->src1->preg]), ip->src1->preg,
			       ip->dst, ip->type);
	    }
	    break;

	case op_ldi_sti:
	case op_ldf_stf:
	    if (
		(reg =
		 search_in_regusage (&(RegUsage[0]), ip->src1,
				     ip->type)) >= 0) {
		DPRINTF (
			 (DEBUG_FLOW, "FoundReplacement for src1 %d",
			  (int) reg));
		if (is_exchangable (&(ip->src1), reg)) {
		    replacecount++;
		    totreplace++;
		    Update_Peep_Info (ip);
		}
	    }
	    if (
		(reg =
		 search_in_regusage (&(RegUsage[0]), ip->src21,
				     ip->type)) >= 0) {
		DPRINTF (
			 (DEBUG_FLOW, "FoundReplacement for src21 %d",
			  (int) reg));
		if (is_exchangable (&(ip->src21), reg)) {
		    replacecount++;
		    totreplace++;
		    Update_Peep_Info (ip);
		}
	    }
#ifdef ENHANCED_FLOW
	    validate_regusage (ip, &(RegUsage[0]));
#endif
	    /* destination now references to source */
	    new_flow_list (&(RegUsage[ip->dst->preg]), ip->dst->preg,
			   ip->src1, ip->type);
	    if (is_am_register (ip->src1)) {
		/* dest references now to the same list of values as source */
		add_list_to_flow_list (&(RegUsage[ip->dst->preg]),
				       RegUsage[ip->src1->preg]);
		/* source refers now also to destination */
		add_flow_list (&(RegUsage[ip->src1->preg]), ip->src1->preg,
			       ip->dst, ip->type);
	    }
	    /* source of sti refers now also to destination */
	    add_flow_list (&(RegUsage[ip->src21->preg]), ip->src21->preg,
			   ip->dst2, ip->type);
	    break;

	case op_sti_sti:
	case op_stf_stf:
#ifdef ENHANCED_FLOW
	    validate_regusage (ip, &(RegUsage[0]));
#endif
	    /* source refers now also to destination */
	    add_flow_list (&(RegUsage[ip->src2->preg]), ip->src2->preg,
			   ip->dst2, ip->type);
	    /* source refers now also to destination */
	    add_flow_list (&(RegUsage[ip->src1->preg]), ip->src1->preg,
			   ip->dst, ip->type);
	    break;

	case op_sti:
	case op_stf:
#ifdef ENHANCED_FLOW
	    validate_regusage (ip, &(RegUsage[0]));
#endif
	    /* source refers now also to destination */
	    add_flow_list (&(RegUsage[ip->src1->preg]), ip->src1->preg,
			   ip->dst, ip->type);
	    break;


	case op_absf_stf:
	case op_absi_sti:
	case op_addf3_stf:
	case op_addi3_sti:
	case op_and3_sti:
	case op_ash3_sti:
	case op_fix_sti:
	case op_float_stf:
	case op_lsh3_sti:
	case op_mpyf3_stf:
	case op_mpyi3_sti:
	case op_negf_stf:
	case op_negi_sti:
	case op_not_sti:
	case op_or3_sti:
	case op_subf3_stf:
	case op_subi3_sti:
	case op_xor3_sti:
#if 0
/* 
 * sorry, currently no replace possible, since
 * src1 must be r0-r7
 * src2 must be restricted indirect
 * src21 must be r0-r7
 */
	    if (ip->src1 != NULL) {
		if (
		    (reg =
		     search_in_regusage (&(RegUsage[0]), ip->src1,
					 ip->type)) >= 0) {
		    DPRINTF (
			     (DEBUG_FLOW, "FoundReplacement for src1 %d",
			      reg));
		    if (is_exchangable (&(ip->src1), reg)) {
			replacecount++;
			totreplace++;
			Update_Peep_Info (ip);
		    }
		}
	    }
	    if (ip->src2 != NULL) {
		if (
		    (reg =
		     search_in_regusage (&(RegUsage[0]), ip->src2,
					 ip->type)) >= 0) {
		    DPRINTF (
			     (DEBUG_FLOW, "FoundReplacement for src2 %d",
			      reg));
		    if (is_exchangable (&(ip->src2), reg)) {
			replacecount++;
			totreplace++;
			Update_Peep_Info (ip);
		    }
		}
	    }
#endif
#ifdef ENHANCED_FLOW
	    validate_regusage (ip, &(RegUsage[0]));
#endif
	    /* source of sti refers now also to destination */
	    add_flow_list (&(RegUsage[ip->src21->preg]), ip->src21->preg,
			   ip->dst2, ip->type);
	    if ((ip->dst != NULL)
		&& (is_am_register (ip->dst))) {
		/* destination of operation does not reference to anywhere now */
		RegUsage[ip->dst->preg] = NULL;
	    }
	    break;
	case op_label:
	    if (headless == TRUE) {
		/* first entrypoint of this piece of code, so use whole information from label */
		copy_regusage_from_label (ip, &(RegUsage[0]), level);
		headless = FALSE;
	    } else {
		/* not first entrypoint, so merge information from label */
		LabelChanges +=
		    merge_regusage_from_label (ip, &(RegUsage[0]), level);
	    }
	    /*
	     *  Remove regusage-entry from label
	     */
	    ip->src21 = NULL;
	    break;

	case op_bu:
	    if ((ip->src1 != NULL) && (ip->src1->mode != am_immed)) {
		if (
		    (reg =
		     search_in_regusage (&(RegUsage[0]), ip->src1,
					 ip->type)) >= 0) {
		    DPRINTF (
			     (DEBUG_FLOW, "FoundReplacement for src1 %d",
			      (int) reg));
		    if (is_exchangable (&(ip->src1), reg)) {
			replacecount++;
			totreplace++;
			Update_Peep_Info (ip);
		    }
		}
	    }
	    /* FALLTHRU */
	case op_br:
	    /* FALLTHRU */
	case op_retsu:
	case op_retiu:
#ifdef ENHANCED_FLOW
	    validate_regusage (ip, &(RegUsage[0]));
#endif
	    /* no information abaout registers availlable now */
	    clear_regusage (&(RegUsage[0]));
	    /* since no entrypoint found now (unconditional controltransfer) */
	    headless = TRUE;
	    break;

	case op_asm:
	    /* clear whole list, asm can do anything to registers and memory */
	    clear_regusage (&(RegUsage[0]));
	    break;

	case op_rpts:
	    if (ip->src1 != NULL) {
		if (
		    (reg =
		     search_in_regusage (&(RegUsage[0]), ip->src1,
					 ip->type)) >= 0) {
		    DPRINTF (
			     (DEBUG_FLOW, "FoundReplacement for src1 %d",
			      (int) reg));
		    if (is_exchangable (&(ip->src1), reg)) {
			replacecount++;
			totreplace++;
			Update_Peep_Info (ip);
		    }
		}
	    }
	    /* FALLTHRU */

	case op_rptb:
#ifdef ENHANCED_FLOW
	    validate_regusage (ip, &(RegUsage[0]));
#endif
	    /* remove all references to and from blockrepeatregisters */
	    RegUsage[REG_RC] = NULL;
	    RegUsage[REG_RS] = NULL;
	    RegUsage[REG_RE] = NULL;
	    remove_from_regusage (RegUsage, REG_RC);
	    remove_from_regusage (RegUsage, REG_RS);
	    remove_from_regusage (RegUsage, REG_RE);
	    break;

	case op_callu:
	    if ((ip->src1 != NULL) && (ip->src1->mode != am_immed)) {
		if (
		    (reg =
		     search_in_regusage (&(RegUsage[0]), ip->src1,
					 ip->type)) >= 0) {
		    DPRINTF (
			     (DEBUG_FLOW, "FoundReplacement for src1 %d",
			      (int) reg));
		    if (is_exchangable (&(ip->src1), reg)) {
			replacecount++;
			totreplace++;
			Update_Peep_Info (ip);
		    }
		}
	    }
	    /* FALLTHRU */
	case op_call:
	case op_trapu:
	case op_xcall:
#ifdef ENHANCED_FLOW
	    validate_regusage (ip, &(RegUsage[0]));
#endif
	    /* remove all references to and from temporary registers */
	    clear_tempreg_usage (&(RegUsage[0]));
	    /*
	     * we do not know what called routine does
	     * with memory
	     */
	    remove_from_regusage (RegUsage, REG_MEMORY);
	    /* FALLTHRU */

	case op_push:
	case op_pop:
	case op_pushf:
	case op_popf:
	case op_pushnopeep:
	case op_pushfnopeep:
	    /* references from SP are no longer valid */
	    RegUsage[REG_SP] = NULL;
	    /* FALLTHRU */

	default:
	    if (ip->src1 != NULL) {
		if (
		    (reg =
		     search_in_regusage (&(RegUsage[0]), ip->src1,
					 ip->type)) >= 0) {
		    DPRINTF (
			     (DEBUG_FLOW, "FoundReplacement for src1 %d",
			      (int) reg));
		    if (is_exchangable (&(ip->src1), reg)) {
			replacecount++;
			totreplace++;
			Update_Peep_Info (ip);
		    }
		}
	    }
	    if (ip->src2 != NULL) {
		if (
		    (reg =
		     search_in_regusage (&(RegUsage[0]), ip->src2,
					 ip->type)) >= 0) {
		    DPRINTF (
			     (DEBUG_FLOW, "FoundReplacement for src2 %d",
			      (int) reg));
		    if (is_exchangable (&(ip->src2), reg)) {
			replacecount++;
			totreplace++;
			Update_Peep_Info (ip);
		    }
		}
	    }
	    if (ip->src21 != NULL) {
		if (
		    (reg =
		     search_in_regusage (&(RegUsage[0]), ip->src21,
					 ip->type)) >= 0) {
		    DPRINTF (
			     (DEBUG_FLOW, "FoundReplacement for src21 %d",
			      (int) reg));
		    if (is_exchangable (&(ip->src21), reg)) {
			replacecount++;
			totreplace++;
			Update_Peep_Info (ip);
		    }
		}
	    }
	    if (ip->src22 != NULL) {
		if (
		    (reg =
		     search_in_regusage (&(RegUsage[0]), ip->src22,
					 ip->type)) >= 0) {
		    DPRINTF (
			     (DEBUG_FLOW, "FoundReplacement for src22 %d",
			      (int) reg));
		    if (is_exchangable (&(ip->src22), reg)) {
			replacecount++;
			totreplace++;
			Update_Peep_Info (ip);
		    }
		}
	    }
#ifdef USE_ALL_OPCODES
	    /* FALLTHRU */
	    /* do no replacements with parallel mul/add */
	case op_mpf3_adf3:
	case op_mpf3_sbf3:
	case op_mpi3_adi3:
	case op_mpi3_sbi3:
#endif /* USE_ALL_OPCODES */
#ifdef ENHANCED_FLOW
	    validate_regusage (ip, &(RegUsage[0]));
#endif
	    if ((ip->dst != NULL) && (is_am_register (ip->dst))) {
		/* destination of operation does not reference to anywhere now */
		RegUsage[ip->dst->preg] = NULL;
	    }
	    if ((ip->dst2 != NULL) && (is_am_register (ip->dst2))) {
		/* destination of operation does not reference to anywhere now */
		RegUsage[ip->dst2->preg] = NULL;
	    }
	    break;

	}
#ifdef DEBUG
	if (is_debugging (DEBUG_FLOW)) {
	    FHANDLE save = output;

	    output = debugfile;
	    if (ip->opcode == op_label) {
		put_label (ip->src1->u.offset->v.l);
	    } else {
		put_code (ip);
	    }
	    output = save;
	    print_regusage (&(RegUsage[0]));
	}
#endif /* DEBUG */
    }
    DPRINTF ((DEBUG_FLOW, "Replaced %d ", replacecount));
    DPRINTF ((DEBUG_FLOW, "Total %d \n", totreplace));
    return replacecount;
}
#endif /* TMS320C30 */
#endif /* PEEPFLOW */
