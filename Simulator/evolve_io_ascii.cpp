/*
 * Copyright (c) 2006 Stauffer Computer Consulting
 */

/***********************************************************************
 * READ/WRITE operations (ASCII VERSION)
 *
 * This module reads/writes objects to files.
 * The format of files are "PHOTON ASCII".
 *
 * If you are generating these files the UNIVERSE entity must be first
 * as the reader attaches all other records to the UNIVERSE record.
 * Except for this, order shouldn't matter.
 *
 * Order of this file matters:
 *
 *	1. UNIVERSE object should come first.
 *
 *	2. Next should come these (in any order)
 *		SIMULATION_OPTIONS KFMO, STRAIN_OPTIONS, STRAIN_OPCODES
 *
 *	3. Each ORGANISM (and its cells) should come next.
 *		The order of ORGANISM's and CELL's must match the internal data structure list ordering.
 *
 *	4. And lastly, CELL_LIST should be at the end.
 *
 */
#include "evolve_simulator.h"
#include "evolve_simulator_private.h"
#include "phascii.h"
#include <stdarg.h>

#if 0
#include <time.h> // KJS remove this when debugging is removed
#endif

static const char *prolog1[] = {
	"# PHOTON ASCII",
	"",
};

static const char *prolog2[] = {
	"struct ORGANIC[N] {",
	"	X",
	"	Y",
	"	ENERGY",
	"}",
	"",
	"struct BARRIER[N] {",
	"	X",
	"	Y",
	"}",
	"",
	"struct ER {",
	"	FIDX",
	"	RIDX",
	"	STATE[N] {",
	"		VALUE",
	"	}",
	"}",
	"",
};

static const char *prolog3[] = {
	"struct SIMULATION_OPTIONS {",
	"	MODE",
	"}",
	"",
	"struct STRAIN_OPTIONS[N] {",
	"	ENABLED",
	"	NAME",
	"	LOOK_MODE",
	"	EAT_MODE",
	"	MAKE_SPORE_MODE",
	"	MAKE_SPORE_ENERGY",
	"	CMOVE_MODE",
	"	OMOVE_MODE",
	"	GROW_MODE",
	"	GROW_ENERGY",
	"	GROW_SIZE",
	"	ROTATE_MODE",
	"	CSHIFT_MODE",
	"	MAKE_ORGANIC_MODE",
	"	MAKE_BARRIER_MODE",
	"	EXUDE_MODE",
	"	SHOUT_MODE",
	"	SPAWN_MODE",
	"	LISTEN_MODE",
	"	BROADCAST_MODE",
	"	SAY_MODE",
	"	READ_MODE",
	"	WRITE_MODE",
	"	KEY_PRESS_MODE",
	"	SEND_MODE",
	"	SEND_ENERGY_MODE",
	"}",
	"",
	"struct KFMO[N] {",
	"	MAX_APPLY",
	"	PROB_MUTATE_CODEBLOCK",
	"	PROB_DUPLICATE",
	"	PROB_DELETE",
	"	PROB_INSERT",
	"	PROB_TRANSPOSE",
	"	PROB_MODIFY",
	"	MERGE_MODE",
	"	XLEN",
	"	PROTECTED_CODEBLOCKS",
	"	MAX_CODE_BLOCKS",
	"}",
	"",
	"struct STRAIN_OPCODES[N] {",
	"	NPROTECTED",
	"	TABLE[M] {",
	"		NAME",
	"	}",
	"}",
	"",
};

static const char *prolog4[] = {
	"struct SPORE {",
	"	X",
	"	Y",
	"	ENERGY",
	"	PARENT",
	"	STRAIN",
	"	SFLAGS",
	"	PROGRAM[N] {",
	"		TEXT_LINE",
	"	}",
	"}",
	"",
	"struct CELL {",
	"	ORGANISM_ID",
	"	X",
	"	Y",
	"	MOOD",
	"	MESSAGE",
	"",
	"	MACHINE {",
	"		TERMINATED",
	"		CB",
	"		PC",
	"		R[N] {",
	"			VALUE",
	"		}",
	"",
	"		CALL_STACK[N] {",
	"			CB",
	"			PC",
	"		}",
	"",
	"		DATA_STACK[N] {",
	"			VALUE",
	"		}",
	"	}",
	"}",
	"",
	"struct ORGANISM {",
	"	ORGANISM_ID",
	"	STRAIN",
	"	SIM_COUNT",
	"	OFLAGS",
	"	PARENT1",
	"	PARENT2",
	"	GENERATION",
	"	ENERGY",
	"	AGE",
	"	PROGRAM[N] {",
	"		TEXT_LINE",
	"	}",
	"}",
	"",
	"struct UNIVERSE {",
	"	SEED",
	"	STEP",
	"	AGE",
	"	CURRENT_CELL { X Y }    # -1 -1 means NULL",
	"	NEXT_ID",
	"	NBORN",
	"	NDIE",
	"	WIDTH",
	"	HEIGHT",
	"	G0",
	"	KEY",
	"	MOUSE_X",
	"	MOUSE_Y",
	"	S0[N] { V }"
	"}",
	"",
	"struct CELL_LIST[N] {",
	"	X Y ",
	"}",
	"",
	"struct ODOR_MAP[N] {",
	"	X Y LEN VALUE",
	"}",
	"",
};

static const char *prolog5[] = {
	"struct STRAIN_PROFILES[N] {",
	"	NAME",
	"	SEED_FILE",
	"	ENERGY",
	"	POPULATION",
	"	DESCRIPTION[M] {",
	"		TEXT_LINE",
	"	}",
	"}",
	"",
	"struct EVOLVE_PREFERENCES {",
	"	EVOLVE_BATCH",
	"	EVOLVE_3D",
	"	HELP",
	"	WIDTH",
	"	HEIGHT",
	"	WANT_BARRIER",
	"	TERRAIN",
	"	DFLT[N] {",
	"		STRAIN",
	"		ENERGY",
	"		POPULATION",
	"		SEED_FILE",
	"	}",
	"}",
	"",
};

/* ***********************************************************************
   ***********************************************************************
   ****************************   WRITE ROUTINES   ************************
   ***********************************************************************
   *********************************************************************** */

static int getline(char **inp, char *buf)
{
	char *p, *q;

	ASSERT( inp != NULL );
	ASSERT( buf != NULL );

	p = *inp;
	q = buf;
	while( *p != '\0' && *p != '\n' ) {
		*q++ = *p++;
	}
	*q = '\0';

	if( *p == '\n' )
		p++;

	*inp = p;

	if( *p == '\0' )
		return 0;
	else
		return 1;
}

/*
 * Write the header information for the
 * PHOTON ASCII file.
 */
static void write_prolog(PHASCII_FILE pf)
{
	int i;

	ASSERT( pf != NULL );

	for(i=0; i < sizeof(prolog1)/sizeof(prolog1[0]); i++) {
		Phascii_Printf(pf, "%s\n", prolog1[i]);
	}

	for(i=0; i < sizeof(prolog2)/sizeof(prolog2[0]); i++) {
		Phascii_Printf(pf, "%s\n", prolog2[i]);
	}

	for(i=0; i < sizeof(prolog3)/sizeof(prolog3[0]); i++) {
		Phascii_Printf(pf, "%s\n", prolog3[i]);
	}

	for(i=0; i < sizeof(prolog4)/sizeof(prolog4[0]); i++) {
		Phascii_Printf(pf, "%s\n", prolog4[i]);
	}
}

static void write_spore(UNIVERSE *u, PHASCII_FILE pf, int x, int y, SPORE *spore)
{
	KFORTH_DISASSEMBLY *kfd;
	char buf[5000], *p;
	KFORTH_OPERATIONS *kfops;

	ASSERT( pf != NULL );
	ASSERT( spore != NULL );

	Phascii_Printf(pf,
		"SPORE %d %d %d %lld %d %d\n",
				x, y, spore->energy,
				spore->parent, spore->strain,
				spore->sflags);

	kfops = &u->kfops[spore->strain];

	kfd = kforth_disassembly_make(kfops, &spore->program, 80, 0);

	Phascii_Printf(pf, "  {  # program\n");

	p = kfd->program_text;
	while( getline(&p, buf) ) {
		Phascii_Printf(pf, "\t\"%s\"\n", buf);
	}
	Phascii_Printf(pf, "  }\n");

	kforth_disassembly_delete(kfd);

	Phascii_Printf(pf, "\n");
}

/*
 * Write out each spore as a SPORE photon ascii instance.
 */
static void write_spores(PHASCII_FILE pf, UNIVERSE *u)
{
	int x, y;
	GRID_TYPE type;
	UNIVERSE_GRID ugrid;

	ASSERT( pf != NULL );
	ASSERT( u != NULL );

	for(x=0; x < u->width; x++) {
		for(y=0; y < u->height; y++) {
			type = Grid_Get(u, x, y, &ugrid);

			if( type == GT_SPORE ) {
				write_spore(u, pf, x, y, ugrid.u.spore);
			}
		}
	}
}

/*
 * Write out barrier elements
 */
static void write_barriers(PHASCII_FILE pf, UNIVERSE *u)
{
	int x, y, n;
	GRID_TYPE type;
	UNIVERSE_GRID ugrid;

	ASSERT( pf != NULL );
	ASSERT( u != NULL );

	Phascii_Printf(pf, "\n");

	n = 0;
	Phascii_Printf(pf, "# BARRIER_BEGIN\n");
	Phascii_Printf(pf, "BARRIER {\n");

	for(x=0; x < u->width; x++) {
		for(y=0; y < u->height; y++) {
			type = Grid_Get(u, x, y, &ugrid);
			if( type != GT_BARRIER )
				continue;

			if( n >= 500 ) {
				n = 0;
				Phascii_Printf(pf, "}\n");
				Phascii_Printf(pf, "BARRIER {\n");
			}

			Phascii_Printf(pf, "\t%d\t%d\n", x, y);
			n += 1;
		}
	}

	Phascii_Printf(pf, "}\n");
	Phascii_Printf(pf, "# BARRIER_END\n");
	Phascii_Printf(pf, "\n");

}

static void write_odor_map_item(PHASCII_FILE pf, int *state, int x, int y, int len, KFORTH_INTEGER value)
{
	if( *state == 0 ) {							// state 0
		*state = 2;
		Phascii_Printf(pf, "ODOR_MAP {\n");

	} else if( *state > 1000 ) {				// state 1001
		*state = 1;
		Phascii_Printf(pf, "}\n\n");
		Phascii_Printf(pf, "ODOR_MAP {\n");
	} else {									// states 1..1000
		*state = *state + 1;
	}

	Phascii_Printf(pf, "\t%4d %-4d  %4d  %d\n", x, y, len, (int)value);
}

static void write_odor_map(PHASCII_FILE pf, UNIVERSE *u)
{
	int x, y, nx, len;
	int state;
	KFORTH_INTEGER the_odor;
	UNIVERSE_GRID *ugp;

	Phascii_Printf(pf, "# ODOR BEGIN\n");

	state = 0;
	for(y=0; y < u->height; y++)
	{
		for(x=0; x < u->width; x += len)
		{
			Grid_GetPtr(u, x, y, &ugp);
			the_odor = ugp->odor;

			nx = x + 1;
			while( nx < u->width ) {
				Grid_GetPtr(u, nx, y, &ugp);
				if( ugp->odor != the_odor ) {
					break;
				}
				nx += 1;
			}
			len = nx - x;

			write_odor_map_item(pf, &state, x, y, len, the_odor);
		}
	}

	if( state != 0 ) {
		Phascii_Printf(pf, "}\n\n");
	}

	Phascii_Printf(pf, "# ODOR END\n");
	Phascii_Printf(pf, "\n");
}

/*
 * Write out organic material
 */
static void write_organic(PHASCII_FILE pf, UNIVERSE *u)
{
	int x, y, n;
	GRID_TYPE type;
	UNIVERSE_GRID ugrid;

	ASSERT( pf != NULL );
	ASSERT( u != NULL );

	Phascii_Printf(pf, "\n");

	n = 0;
	Phascii_Printf(pf, "ORGANIC {\n");

	for(x=0; x < u->width; x++) {
		for(y=0; y < u->height; y++) {
			type = Grid_Get(u, x, y, &ugrid);
			if( type != GT_ORGANIC )
				continue;

			if( n >= 500 ) {
				n = 0;
				Phascii_Printf(pf, "}\n");
				Phascii_Printf(pf, "ORGANIC {\n");
			}

			Phascii_Printf(pf, "\t%d\t%d\t%d\n", x, y, ugrid.u.energy);
			n += 1;
		}
	}

	Phascii_Printf(pf, "}\n");
	Phascii_Printf(pf, "\n");
}

void write_evolve_random(PHASCII_FILE pf, EVOLVE_RANDOM *er)
{
	int i, n;
	LONG_LONG state;

	ASSERT( pf != NULL );
	ASSERT( er != NULL );

	Phascii_Printf(pf, "ER %u %u %d\n", er->fidx, er->ridx, EVOLVE_DEG4);

	n = 0;
	for(i=0; i < EVOLVE_DEG4; i++) {
		if( n >= 4 ) {
			n = 0;
			Phascii_Printf(pf, "\n");
		}
	
		state = er->state[i];
		Phascii_Printf(pf, "\t%lld", state);
		n++;
	}
	Phascii_Printf(pf, "\n\n");
}

/*
 * Write simulation options struct
 */
static void write_simulation_options(PHASCII_FILE pf, SIMULATION_OPTIONS *so)
{
	ASSERT( pf != NULL );
	ASSERT( so != NULL );

	Phascii_Printf(pf, "SIMULATION_OPTIONS %d   # mode\n", so->mode );
	Phascii_Printf(pf, "\n");
}

static void write_strain_options_item(PHASCII_FILE pf, int i, STRAIN_OPTIONS *strop)
{
	Phascii_Printf(pf, "\t# Strain %d\n", i);
	Phascii_Printf(pf, "\t%d        # enabled\n", strop->enabled);
	Phascii_Printf(pf, "\t\"%s\"        # strain name these settings were based on\n", strop->name);
	Phascii_Printf(pf, "\t%d        # LOOK mode\n", strop->look_mode);
	Phascii_Printf(pf, "\t%d        # EAT mode\n", strop->eat_mode);
	Phascii_Printf(pf, "\t%d        # MAKE-SPORE mode\n", strop->make_spore_mode);
	Phascii_Printf(pf, "\t%d        # MAKE-SPORE energy\n", strop->make_spore_energy);
	Phascii_Printf(pf, "\t%d        # CMOVE mode\n", strop->cmove_mode);
	Phascii_Printf(pf, "\t%d        # OMOVE mode\n", strop->omove_mode);
	Phascii_Printf(pf, "\t%d        # GROW mode\n", strop->grow_mode);
	Phascii_Printf(pf, "\t%d        # GROW energy\n", strop->grow_energy);
	Phascii_Printf(pf, "\t%d        # GROW size\n", strop->grow_size);
	Phascii_Printf(pf, "\t%d        # ROTATE mode\n", strop->rotate_mode);
	Phascii_Printf(pf, "\t%d        # CSHIFT mode\n", strop->cshift_mode);
	Phascii_Printf(pf, "\t%d        # MAKE-ORGANIC mode\n", strop->make_organic_mode);
	Phascii_Printf(pf, "\t%d        # MAKE-BARRIER mode\n", strop->make_barrier_mode);
	Phascii_Printf(pf, "\t%d        # EXUDE mode\n", strop->exude_mode);
	Phascii_Printf(pf, "\t%d        # SHOUT mode\n", strop->shout_mode);
	Phascii_Printf(pf, "\t%d        # SPAWN mode\n", strop->spawn_mode);
	Phascii_Printf(pf, "\t%d        # LISTEN mode\n", strop->listen_mode);
	Phascii_Printf(pf, "\t%d        # BROADCAST mode\n", strop->broadcast_mode);
	Phascii_Printf(pf, "\t%d        # SAY mode\n", strop->say_mode);
	Phascii_Printf(pf, "\t%d        # READ mode\n", strop->read_mode);
	Phascii_Printf(pf, "\t%d        # WRITE mode\n", strop->write_mode);
	Phascii_Printf(pf, "\t%d        # KEY-PRESS mode\n", strop->key_press_mode);
	Phascii_Printf(pf, "\t%d        # SEND mode\n", strop->send_mode);
	Phascii_Printf(pf, "\t%d        # SEND-ENERGY mode\n", strop->send_energy_mode);
	Phascii_Printf(pf, "\n");
}

/*
 * 'strain' points to an array of [8] items. One for each strain
 */
static void write_strain_options(PHASCII_FILE pf, STRAIN_OPTIONS *strop)
{
	int i;

	ASSERT( pf != NULL );
	ASSERT( strop != NULL );

	Phascii_Printf(pf, "\n");
	Phascii_Printf(pf, "STRAIN_OPTIONS {\n");

	for(i=0; i < 8; i++) {
		write_strain_options_item(pf, i, &strop[i]);
	}

	Phascii_Printf(pf, "}\n\n");
}

static void write_kfmo_item(PHASCII_FILE pf, int i, KFORTH_MUTATE_OPTIONS *kfmo)
{
	Phascii_Printf(pf, "\t# Strain %d\n", i);
	Phascii_Printf(pf, "\t%d		# max apply\n", kfmo->max_apply );
	Phascii_Printf(pf, "\t%d		# prob. mutate codeblock\n", kfmo->prob_mutate_codeblock );
	Phascii_Printf(pf, "\t%d		# prob_duplicate\n", kfmo->prob_duplicate) ;
	Phascii_Printf(pf, "\t%d		# prob_delete\n", kfmo->prob_delete );
	Phascii_Printf(pf, "\t%d		# prob_insert\n", kfmo->prob_insert );
	Phascii_Printf(pf, "\t%d		# prob_transpose\n", kfmo->prob_transpose );
	Phascii_Printf(pf, "\t%d		# prob_modify\n", kfmo->prob_modify );
	Phascii_Printf(pf, "\t%d		# merge_mode\n", kfmo->merge_mode );
	Phascii_Printf(pf, "\t%d		# xlen \n", kfmo->xlen );
	Phascii_Printf(pf, "\t%d		# protected_codeblocks \n", kfmo->protected_codeblocks );
	Phascii_Printf(pf, "\t%d		# max_code_blocks \n", kfmo->max_code_blocks );

	Phascii_Printf(pf, "\n");
}

/*
 * kfmo points to an array of [8] items. One for each strain
 */
static void write_kfmo(PHASCII_FILE pf, KFORTH_MUTATE_OPTIONS *kfmo)
{
	int i;

	ASSERT( pf != NULL );
	ASSERT( kfmo != NULL );

	Phascii_Printf(pf, "\n");
	Phascii_Printf(pf, "KFMO {\n");

	for(i=0; i < 8; i++) {
		write_kfmo_item(pf, i, &kfmo[i]);
	}

	Phascii_Printf(pf, "}\n\n");
}

static void write_strain_opcodes_item(PHASCII_FILE pf, int n, KFORTH_OPERATIONS *kfops)
{
	int m;
	char buf[1000];

	ASSERT( pf != NULL );
	ASSERT( kfops != NULL );

	Phascii_Printf(pf, "\n");
	Phascii_Printf(pf, "\t%d  # number of protected instructions at start of table\n", kfops->nprotected);
	Phascii_Printf(pf, "\t# instruction table for strain %d\n", n);
	Phascii_Printf(pf, "\t{\n");

	for(m=0; m < kfops->count; m++) {
		snprintf(buf, sizeof(buf), "\"%s\"", kfops->table[m].name);
		Phascii_Printf(pf, "\t\t%-20s\t\t\t# opcode %d\n", buf, m);
					// KJS TODO: Sanitize the string if needed any bad chars in the string? like slash
	}
	Phascii_Printf(pf, "\t}\n");
}

/*
 * 'kfops' points to an array of [8] items. One for each strain
 */
static void write_strain_opcodes(PHASCII_FILE pf, KFORTH_OPERATIONS *kfops)
{
	int n;

	ASSERT( pf != NULL );
	ASSERT( kfops != NULL );

	Phascii_Printf(pf, "\n");
	Phascii_Printf(pf, "STRAIN_OPCODES {\n");

	for(n=0; n < 8; n++) {
		write_strain_opcodes_item(pf, n, &kfops[n]);
	}

	Phascii_Printf(pf, "}\n\n");
}

static void write_cell(PHASCII_FILE pf, LONG_LONG organism_id, CELL *c)
{
	KFORTH_MACHINE *kfm;
	KFORTH_LOC *loc;
	int i;

	ASSERT( pf != NULL );
	ASSERT( c != NULL );

	Phascii_Printf(pf,
		"CELL %lld   %d %d\n",
			organism_id,
			c->x,
			c->y );

	Phascii_Printf(pf,
		"\t%d %d\n",
			c->mood, c->message);

	kfm = &c->kfm;
	Phascii_Printf(pf, "\t%d %d %d\n",
			kforth_machine_terminated(kfm),
			kfm->loc.cb,
			kfm->loc.pc);

	Phascii_Printf(pf,
			"\t{ %d %d %d %d %d\n",
			kfm->R[0], kfm->R[1], kfm->R[2], kfm->R[3], kfm->R[4]);

	Phascii_Printf(pf,
			"\t  %d %d %d %d %d }\n",
			kfm->R[5], kfm->R[6], kfm->R[7], kfm->R[8], kfm->R[9]);

	/*
	 * call stack
	 */
	Phascii_Printf(pf, "\t{\n");
	for(i=0; i < kfm->csp; i++) {
		loc = &kfm->call_stack[i];
		Phascii_Printf(pf, "\t\t%d %d\n", loc->cb, loc->pc);
	}
	Phascii_Printf(pf, "\t}\n");

	/*
	 * data stack
	 */
	Phascii_Printf(pf, "\t{\n");
	for(i=0; i < kfm->dsp; i++) {
		Phascii_Printf(pf, "\t\t%d\n", kfm->data_stack[i]);
	}
	Phascii_Printf(pf, "\t}\n\n");

}

static void write_organism(PHASCII_FILE pf, UNIVERSE *u, ORGANISM *o)
{
	KFORTH_DISASSEMBLY *kfd;
	char buf[5000], *p;
	CELL *c;
	KFORTH_OPERATIONS *kfops;

	ASSERT( pf != NULL );
	ASSERT( o != NULL );

	Phascii_Printf(pf,
		"ORGANISM %lld  %d %d %d   %lld %lld  %d %d %d\n",
			o->id,
			o->strain,
			o->sim_count,
			o->oflags,
			o->parent1,
			o->parent2,
			o->generation,
			o->energy,
			o->age);

	kfops = &u->kfops[ o->strain ];

	kfd = kforth_disassembly_make(kfops, &o->program, 80, 0);

	Phascii_Printf(pf, "  {  # program\n");

	p = kfd->program_text;
	while( getline(&p, buf) ) {
		Phascii_Printf(pf, "\t\"%s\"\n", buf);
	}
	Phascii_Printf(pf, "  }\n");

	kforth_disassembly_delete(kfd);

	Phascii_Printf(pf, "\n");

	for(c=o->cells; c; c=c->next) {
		write_cell(pf, o->id, c);
	}
}

static void write_organisms(PHASCII_FILE pf, UNIVERSE *u)
{
	ORGANISM *o;

	ASSERT( pf != NULL );
	ASSERT( u != NULL );

	for(o=u->organisms; o; o=o->next) {
		write_organism(pf, u, o);
	}
}

static void write_cell_list(PHASCII_FILE pf, UNIVERSE *u)
{
	CELL *c;

	ASSERT( pf != NULL );
	ASSERT( u != NULL );

	Phascii_Printf(pf, "\n");
	Phascii_Printf(pf, "CELL_LIST {\n");

	for(c=u->cells; c != NULL; c=c->u_next) {
		Phascii_Printf(pf, "\t%d %d\n", c->x, c->y);
	}
	Phascii_Printf(pf, "}\n\n");
}

static void write_universe(PHASCII_FILE pf, UNIVERSE *u)
{
	int x, y, i;

	ASSERT( pf != NULL );
	ASSERT( u != NULL );

	if( u->current_cell != NULL ) {
		x = u->current_cell->x;
		y = u->current_cell->y;
	} else {
		x = -1;
		y = -1;
	}

	Phascii_Printf(pf, "UNIVERSE %u             # seed\n", u->seed );
	Phascii_Printf(pf, "         %lld           # step\n", u->step );
	Phascii_Printf(pf, "         %lld           # age\n", u->age );
	Phascii_Printf(pf, "         %d %d          # current cell location (x,y)\n", x, y);
	Phascii_Printf(pf, "         %lld           # next id\n", u->next_id);
	Phascii_Printf(pf, "         %lld %lld      # number births, deaths\n", u->nborn, u->ndie);
	Phascii_Printf(pf, "         %d %d          # dimensions: width x height\n", u->width, u->height);
	Phascii_Printf(pf, "         %d             # global register G0\n", u->G0);

	Phascii_Printf(pf, "         %d             # key\n", u->key);
	Phascii_Printf(pf, "         %d             # mouse_x\n", u->mouse_x);
	Phascii_Printf(pf, "         %d             # mouse_y\n", u->mouse_y);

	Phascii_Printf(pf, "       { ");
	for(i=0; i < 8; i++) {
		Phascii_Printf(pf, "%d ", u->S0[i]);
	}
	Phascii_Printf(pf, "}  # S0's for each strain\n");

	Phascii_Printf(pf, "\n");
}

#define ERROR_STR_SIZE 1000
/*
 * This is a replacement for sprintf() which uses a limit of 1000
 */
static void errfmt(char *errbuf, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);

	vsnprintf(errbuf, ERROR_STR_SIZE, fmt, args);
}

/***********************************************************************
 * Write the entire state of the universe to 'filename'
 *
 */
static int Do_Write_Ascii(UNIVERSE *u, const char *filename, PhasciiWriteCB wcb, char *errbuf)
{
	PHASCII_FILE pf;

	ASSERT( u != NULL );
	ASSERT( filename != NULL );
	ASSERT( errbuf != NULL );

	if( wcb == NULL ) {
		pf = Phascii_Open(filename, "w");
	} else {
		pf = Phascii_Open_WriteCB(filename, wcb);
	}

	if( pf == NULL )
	{
		if( wcb == NULL ) {
			errfmt(errbuf, "%s: Phascii_Open_WriteCB failed", filename);
		} else {
			errfmt(errbuf, "%s: %s", filename, strerror(errno));
		}
		return 0;
	}

	write_prolog(pf);
	write_universe(pf, u);
	write_evolve_random(pf, &u->er);

	write_simulation_options(pf, &u->so);
	write_strain_options(pf, u->strop);
	write_kfmo(pf, u->kfmo);
	write_strain_opcodes(pf, u->kfops);

	write_barriers(pf, u);
	write_odor_map(pf, u);
	write_organic(pf, u);
	write_spores(pf, u);
	write_organisms(pf, u);
	write_cell_list(pf, u);

	Phascii_Close(pf);
	
	return 1;
}

/* ***********************************************************************
   ***********************************************************************
   ****************************   READ ROUTINES   ************************
   ***********************************************************************
   *********************************************************************** */

static int read_universe(PHASCII_INSTANCE pi, UNIVERSE **pu, char *errmsg, int *cc_x, int *cc_y)
{
	UNIVERSE *u;
	int n, x, y, g0, s0, len, i;

	ASSERT( pi != NULL );
	ASSERT( pu != NULL );
	ASSERT( errmsg != NULL );

	if( *pu != NULL ) {
		errfmt(errmsg, "multiple UNIVERSE instances not allowed");
		return 0;		
	}

	u = (UNIVERSE *) CALLOC(1, sizeof(UNIVERSE));
	ASSERT( u != NULL );

	n = Phascii_Get(pi, "UNIVERSE.SEED", "%u", &u->seed);
	if( n != 1 ) {
		errfmt(errmsg, "missing UNIVERSE.SEED");
		FREE(u);
		return 0;
	}

	n = Phascii_Get(pi, "UNIVERSE.STEP", "%ll", &u->step);
	if( n != 1 ) {
		errfmt(errmsg, "missing UNIVERSE.STEP");
		FREE(u);
		return 0;
	}

	n = Phascii_Get(pi, "UNIVERSE.AGE", "%ll", &u->age);
	if( n != 1 ) {
		errfmt(errmsg, "missing UNIVERSE.AGE");
		FREE(u);
		return 0;
	}

	n = Phascii_Get(pi, "UNIVERSE.CURRENT_CELL.{X,Y}", "%d %d", cc_x, cc_y);
	if( n != 2 ) {
		errfmt(errmsg, "missing UNIVERSE.CURRENT_CELL.{X,Y}");
		FREE(u);
		return 0;
	}

	n = Phascii_Get(pi, "UNIVERSE.NEXT_ID", "%ll", &u->next_id);
	if( n != 1 ) {
		errfmt(errmsg, "missing UNIVERSE.NEXT_ID");
		FREE(u);
		return 0;
	}

	n = Phascii_Get(pi, "UNIVERSE.NBORN", "%ll", &u->nborn);
	if( n != 1 ) {
		errfmt(errmsg, "missing UNIVERSE.NBORN");
		FREE(u);
		return 0;
	}

	n = Phascii_Get(pi, "UNIVERSE.NDIE", "%ll", &u->ndie);
	if( n != 1 ) {
		errfmt(errmsg, "missing UNIVERSE.NDIE");
		FREE(u);
		return 0;
	}

	n = Phascii_Get(pi, "UNIVERSE.WIDTH", "%d", &u->width);
	if( n != 1 ) {
		errfmt(errmsg, "missing UNIVERSE.WIDTH");
		FREE(u);
		return 0;
	}

	n = Phascii_Get(pi, "UNIVERSE.HEIGHT", "%d", &u->height);
	if( n != 1 ) {
		errfmt(errmsg, "missing UNIVERSE.HEIGHT");
		FREE(u);
		return 0;
	}

	if( u->width < 0 || u->width > EVOLVE_MAX_BOUNDS ) {
		errfmt(errmsg, "UNIVERSE.WIDTH out of bounds");
		FREE(u);
		return 0;
	}

	if( u->height < 0 || u->height > EVOLVE_MAX_BOUNDS ) {
		errfmt(errmsg, "UNIVERSE.HEIGHT out of bounds");
		FREE(u);
		return 0;
	}

	n = Phascii_Get(pi, "UNIVERSE.G0", "%d", &g0);
	if( n != 1 ) {
		errfmt(errmsg, "missing UNIVERSE.G0");
		FREE(u);
		return 0;
	}

	n = Phascii_Get(pi, "UNIVERSE.KEY", "%d", &u->key);
	if( n != 1 ) {
		errfmt(errmsg, "missing UNIVERSE.KEY");
		FREE(u);
		return 0;
	}

	n = Phascii_Get(pi, "UNIVERSE.MOUSE_X", "%d", &u->mouse_x);
	if( n != 1 ) {
		errfmt(errmsg, "missing UNIVERSE.MOUSE_X");
		FREE(u);
		return 0;
	}

	n = Phascii_Get(pi, "UNIVERSE.MOUSE_Y", "%d", &u->mouse_y);
	if( n != 1 ) {
		errfmt(errmsg, "missing UNIVERSE.MOUSE_Y");
		FREE(u);
		return 0;
	}

	u->G0 = g0;

	n = Phascii_Get(pi, "UNIVERSE.S0.N", "%d", &len);
	if( n != 1 ) {
		errfmt(errmsg, "missing UNIVERSE.S0.N");
		FREE(u);
		return 0;
	}

	if( len != 8 ) {
		errfmt(errmsg, "UNIVERSE.S0.N must be 8, got %d", len);
		FREE(u);
		return 0;
	}

	for(i=0; i < 8; i++) {
		n = Phascii_Get(pi, "UNIVERSE.S0[%0].V", i, "%d", &s0);
		if( n != 1 ) {
			errfmt(errmsg, "missing UNIVERSE.S0[%d]", i);
			FREE(u);
			return 0;
		}
		u->S0[i] = s0;
	}

	u->grid = (UNIVERSE_GRID*) CALLOC( u->width * u->height, sizeof(UNIVERSE_GRID) );
	ASSERT( u->grid != NULL );

	for(x=0; x < u->width; x++) {
		for(y=0; y < u->height; y++) {
			Grid_Clear(u, x, y);
		}
	}

	for(int i=0; i < 8; i++)
	{
		u->kfops[i] = *EvolveOperations();
	}

	*pu = u;
	return 1;
}


/*
 * Read the ER instance and build an EVOLVE_RANDOM object and
 * attach it to UNIVERSE 'u'.
 */
static int read_er(PHASCII_INSTANCE pi, UNIVERSE *u, char *errmsg, int *got_it)
{
	int n, i, num, fidx, ridx;
	LONG_LONG state;
	EVOLVE_RANDOM *er;

	ASSERT( pi != NULL );
	ASSERT( errmsg != NULL );

	if( u == NULL ) {
		errfmt(errmsg, "a UNIVERSE instance must appear before ER instance");
		return 0;
	}

	if( *got_it ) {
		errfmt(errmsg, "multiple ER instances not allowed");
		return 0;
	}
	
	*got_it += 1;

	n = Phascii_Get(pi, "ER.FIDX", "%d", &fidx);
	if( n != 1 ) {
		errfmt(errmsg, "missing ER.FIDX");
		return 0;
	}

	n = Phascii_Get(pi, "ER.RIDX", "%d", &ridx);
	if( n != 1 ) {
		errfmt(errmsg, "missing ER.RIDX");
		return 0;
	}

	n = Phascii_Get(pi, "ER.STATE.N", "%d", &num);
	if( n != 1 ) {
		errfmt(errmsg, "missing ER.STATE.N");
		return 0;
	}

	if( num != EVOLVE_DEG4 ) {
		errfmt(errmsg, "ER.STATE.N = %d should be %d", num, EVOLVE_DEG4);
		return 0;
	}

	er = &u->er;

	er->fidx = fidx;
	er->ridx = ridx;

	for(i=0; i < num; i++) {
		n = Phascii_Get(pi, "ER.STATE[%0].VALUE", i, "%ll", &state);

		if( n != 1 ) {
			errfmt(errmsg, "ER.STATE[%d].VALUE missing", i);
			return 0;
		}

		er->state[i] = (uint32_t)state;
	}

	return 1;
}

static int read_simulation_options_item(PHASCII_INSTANCE pi, SIMULATION_OPTIONS *so, char *errmsg)
{
	int n;

	n = Phascii_Get(pi, "SIMULATION_OPTIONS.MODE", "%d", &so->mode);
	if( n != 1 ) {
		errfmt(errmsg, "missing SIMULATION_OPTIONS.MODE");
		return 0;
	}
	return 1;
}

static int read_simulation_options(PHASCII_INSTANCE pi, UNIVERSE *u, char *errmsg, int *got_it)
{
	int success;

	ASSERT( pi != NULL );
	ASSERT( errmsg != NULL );

	if( u == NULL ) {
		errfmt(errmsg, "a UNIVERSE instance must appear before SIMULATION_OPTIONS instance");
		return 0;
	}

	if( *got_it ) {
		errfmt(errmsg, "multiple SIMULATION_OPTIONS instances not allowed");
		return 0;
	}

	*got_it += 1;

	SimulationOptions_Init(&u->so);

	success = read_simulation_options_item(pi, &u->so, errmsg);

	return success;
}

static int read_strain_options_item(PHASCII_INSTANCE pi, int i, STRAIN_OPTIONS *strop, char *errmsg)
{
	int n;

	ASSERT( pi != NULL );
	ASSERT( strop != NULL );
	ASSERT( errmsg != NULL );

	n = Phascii_Get(pi, "STRAIN_OPTIONS[%0].ENABLED", i, "%d", &strop->enabled);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_OPTIONS[%d].ENABLED", i);
		return 0;
	}

	n = Phascii_Get(pi, "STRAIN_OPTIONS[%0].NAME", i, "%*s", sizeof(strop->name), strop->name);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_OPTIONS[%d].NAME", i);
		return 0;
	}

	n = Phascii_Get(pi, "STRAIN_OPTIONS[%0].LOOK_MODE", i, "%d", &strop->look_mode);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_OPTIONS[%d].LOOK_MODE", i);
		return 0;
	}

	n = Phascii_Get(pi, "STRAIN_OPTIONS[%0].EAT_MODE", i, "%d", &strop->eat_mode);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_OPTIONS[%d].EAT_MODE", i);
		return 0;
	}

	n = Phascii_Get(pi, "STRAIN_OPTIONS[%0].MAKE_SPORE_MODE", i, "%d", &strop->make_spore_mode);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_OPTIONS[%d].MAKE_SPORE_MODE", i);
		return 0;
	}

	n = Phascii_Get(pi, "STRAIN_OPTIONS[%0].MAKE_SPORE_ENERGY", i, "%d", &strop->make_spore_energy);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_OPTIONS[%d].MAKE_SPORE_ENERGY", i);
		return 0;
	}

	n = Phascii_Get(pi, "STRAIN_OPTIONS[%0].CMOVE_MODE", i, "%d", &strop->cmove_mode);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_OPTIONS[%d].CMOVE_MODE", i);
		return 0;
	}

	n = Phascii_Get(pi, "STRAIN_OPTIONS[%0].OMOVE_MODE", i, "%d", &strop->omove_mode);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_OPTIONS[%d].OMOVE_MODE", i);
		return 0;
	}

	n = Phascii_Get(pi, "STRAIN_OPTIONS[%0].GROW_MODE", i, "%d", &strop->grow_mode);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_OPTIONS[%d].GROW_MODE", i);
		return 0;
	}

	n = Phascii_Get(pi, "STRAIN_OPTIONS[%0].GROW_ENERGY", i, "%d", &strop->grow_energy);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_OPTIONS[%d].GROW_ENERGY", i);
		return 0;
	}

	n = Phascii_Get(pi, "STRAIN_OPTIONS[%0].GROW_SIZE", i, "%d", &strop->grow_size);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_OPTIONS[%d].GROW_SIZE", i);
		return 0;
	}

	n = Phascii_Get(pi, "STRAIN_OPTIONS[%0].ROTATE_MODE", i, "%d", &strop->rotate_mode);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_OPTIONS[%d].ROTATE_MODE", i);
		return 0;
	}

	n = Phascii_Get(pi, "STRAIN_OPTIONS[%0].CSHIFT_MODE", i, "%d", &strop->cshift_mode);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_OPTIONS[%d].CSHIFT_MODE", i);
		return 0;
	}

	n = Phascii_Get(pi, "STRAIN_OPTIONS[%0].MAKE_ORGANIC_MODE", i, "%d", &strop->make_organic_mode);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_OPTIONS[%d].MAKE_ORGANIC_MODE", i);
		return 0;
	}

	n = Phascii_Get(pi, "STRAIN_OPTIONS[%0].MAKE_BARRIER_MODE", i, "%d", &strop->make_barrier_mode);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_OPTIONS[%d].MAKE_BARRIER_MODE", i);
		return 0;
	}

	n = Phascii_Get(pi, "STRAIN_OPTIONS[%0].EXUDE_MODE", i, "%d", &strop->exude_mode);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_OPTIONS[%d].EXUDE_MODE", i);
		return 0;
	}

	n = Phascii_Get(pi, "STRAIN_OPTIONS[%0].SHOUT_MODE", i, "%d", &strop->shout_mode);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_OPTIONS[%d].SHOUT_MODE", i);
		return 0;
	}

	n = Phascii_Get(pi, "STRAIN_OPTIONS[%0].SPAWN_MODE", i, "%d", &strop->spawn_mode);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_OPTIONS[%d].SPAWN_MODE", i);
		return 0;
	}

	n = Phascii_Get(pi, "STRAIN_OPTIONS[%0].LISTEN_MODE", i, "%d", &strop->listen_mode);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_OPTIONS[%d].LISTEN_MODE", i);
		return 0;
	}

	n = Phascii_Get(pi, "STRAIN_OPTIONS[%0].BROADCAST_MODE", i, "%d", &strop->broadcast_mode);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_OPTIONS[%d].BROADCAST_MODE", i);
		return 0;
	}

	n = Phascii_Get(pi, "STRAIN_OPTIONS[%0].SAY_MODE", i, "%d", &strop->say_mode);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_OPTIONS[%d].SAY_MODE", i);
		return 0;
	}

	n = Phascii_Get(pi, "STRAIN_OPTIONS[%0].READ_MODE", i, "%d", &strop->read_mode);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_OPTIONS[%d].READ_MODE", i);
		return 0;
	}

	n = Phascii_Get(pi, "STRAIN_OPTIONS[%0].WRITE_MODE", i, "%d", &strop->write_mode);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_OPTIONS[%d].WRITE_MODE", i);
		return 0;
	}

	n = Phascii_Get(pi, "STRAIN_OPTIONS[%0].WRITE_MODE", i, "%d", &strop->write_mode);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_OPTIONS[%d].WRITE_MODE", i);
		return 0;
	}

	n = Phascii_Get(pi, "STRAIN_OPTIONS[%0].KEY_PRESS_MODE", i, "%d", &strop->key_press_mode);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_OPTIONS[%d].KEY_PRESS_MODE", i);
		return 0;
	}

	n = Phascii_Get(pi, "STRAIN_OPTIONS[%0].SEND_MODE", i, "%d", &strop->send_mode);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_OPTIONS[%d].SEND_MODE", i);
		return 0;
	}

	n = Phascii_Get(pi, "STRAIN_OPTIONS[%0].SEND_ENERGY_MODE", i, "%d", &strop->send_energy_mode);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_OPTIONS[%d].SEND_ENERGY_MODE", i);
		return 0;
	}

	return 1;
}

static int read_strain_options(PHASCII_INSTANCE pi, UNIVERSE *u, char *errmsg, int *got_it)
{
	int n, i, num, success;
	STRAIN_OPTIONS *strop;

	ASSERT( pi != NULL );
	ASSERT( errmsg != NULL );

	if( u == NULL ) {
		errfmt(errmsg, "a UNIVERSE instance must appear before STRAIN_OPTIONS instance");
		return 0;
	}

	if( *got_it ) {
		errfmt(errmsg, "multiple STRAIN_OPTIONS instances not allowed");
		return 0;
	}

	*got_it += 1;

	n = Phascii_Get(pi, "STRAIN_OPTIONS.N", "%d", &num);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_OPTIONS.N");
		return 0;
	}

	if( num != 8 )
	{
		errfmt(errmsg, "array length must be 8, not STRAIN_OPTIONS.N=%d", num);
		return 0;
	}

	for(i=0; i<8; i++)
	{
		strop = &u->strop[i];

		StrainOptions_Init(strop);

		success = read_strain_options_item(pi, i, strop, errmsg);
		if( !success ) {
			return 0;
		}
	}

	return 1;
}

static int read_strain_opcodes_item(PHASCII_INSTANCE pi, KFORTH_OPERATIONS *master_kfops, int i, KFORTH_OPERATIONS *kfops, char *errmsg)
{
	char buf[5000];
	int n, m, num, j;
	KFORTH_OPERATION *found;

	n = Phascii_Get(pi, "STRAIN_OPCODES[%0].NPROTECTED", i, "%d", &num);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_OPCODES[%d].NPROTECTED", i);
		return 0;
	}

	kfops->nprotected = num;

	n = Phascii_Get(pi, "STRAIN_OPCODES[%0].TABLE.M", i, "%d", &num);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_OPCODES[%d].TABLE.M", i);
		return 0;
	}

	if( num > KFORTH_OPS_LEN ) {
		errfmt(errmsg, "STRAIN_OPCODES[%d].TABLE.M == %d, exceeds limit of %d", i, num, KFORTH_OPS_LEN);
		return 0;
	}

	kfops->count = 0;
	for(m=0; m < num; m++) {
		n = Phascii_Get(pi, "STRAIN_OPCODES[%0].TABLE[%1].NAME", i, m, "%s", buf);
		if( n != 1 ) {
			errfmt(errmsg, "missing STRAIN_OPCODES[%d].TABLE[%d].NAME", i, m);
			return 0;
		}

		found = NULL;
		for(j=0; j < master_kfops->count; j++) {
			if( stricmp(buf, master_kfops->table[j].name) == 0 ) {
				found = &master_kfops->table[j];
			}
		}

		if( found == NULL )
		{
			errfmt(errmsg, "no such opcode STRAIN_OPCODES[%d].TABLE[%d].NAME = '%s'", i, m, buf);
			return 0;
		}

		kforth_ops_add2(kfops, found);
	}

	return 1;
}

static int read_strain_opcodes(PHASCII_INSTANCE pi, UNIVERSE *u, char *errmsg, int *got_it)
{
	int n, i, num, success;
	KFORTH_OPERATIONS *kfops;
	KFORTH_OPERATIONS *master_kfops;

	ASSERT( pi != NULL );
	ASSERT( errmsg != NULL );

	if( u == NULL ) {
		errfmt(errmsg, "a UNIVERSE instance must appear before STRAIN_OPCODES instance");
		return 0;
	}

	if( *got_it ) {
		errfmt(errmsg, "multiple STRAIN_OPCODES instances not allowed");
		return 0;
	}

	*got_it += 1;

	n = Phascii_Get(pi, "STRAIN_OPCODES.N", "%d", &num);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_OPCODES.N");
		return 0;
	}

	if( num != 8 )
	{
		errfmt(errmsg, "array length must be 8, not SIMULATION_OPCODES.N=%d", num);
		return 0;
	}

	master_kfops = EvolveOperations();

	for(i=0; i<8; i++)
	{
		kfops = &u->kfops[i];

		success = read_strain_opcodes_item(pi, master_kfops, i, kfops, errmsg);
		if( !success ) {
			return 0;
		}
	}

	return 1;
}

static int read_cell_list(PHASCII_INSTANCE pi, UNIVERSE *u, char *errmsg, int *got_it)
{
	int num, n, i;
	int x, y;
	CELL *prev, *c;
	UNIVERSE_GRID ugrid;
	GRID_TYPE type;
	ORGANISM *o;

	ASSERT( pi != NULL );
	ASSERT( errmsg != NULL );
	ASSERT( got_it != NULL );

	if( u == NULL ) {
		errfmt(errmsg, "a UNIVERSE instance must appear before CELL_LIST instance");
		return 0;
	}

	if( *got_it ) {
		errfmt(errmsg, "multiple CELL_LIST instances not allowed");
		return 0;
	}
	
	*got_it += 1;

	n = Phascii_Get(pi, "CELL_LIST.N", "%d", &num);
	if( n != 1 ) {
		errfmt(errmsg, "missing CELL_LIST.N");
		return 0;
	}

	ASSERT( u->cells == NULL );

	prev = NULL;

	for(i=0; i<num; i++)
	{
		n = Phascii_Get(pi, "CELL_LIST[%0].{X,Y}", i, "%d %d", &x, &y);
		if( n != 2 )
		{
			errfmt(errmsg, "CELL_LIST[%d].{X,Y} missing", i);
			return 0;
		}

		type = Grid_Get(u, x, y, &ugrid);
		if( type != GT_CELL )
		{
			errfmt(errmsg, "CELL_LIST[%d] -> (%d,%d). not found", i, x, y);
			return 0;
		}

		c = ugrid.u.cell;

		if( prev == NULL )
		{
			u->cells = c;
			c->u_prev = NULL;
			c->u_next = NULL;
		} else {
			prev->u_next = c;
			c->u_prev = prev;
			c->u_next = NULL;
		}
		prev = c;
	}

	// Every cell should be in the list.
	if( u->norganism > 1 ) {
		for(o=u->organisms; o != NULL; o=o->next) {
			for(c=o->cells; c != NULL; c=c->next) {
				if( c->u_next == NULL && c->u_prev == NULL ) {
					errfmt(errmsg, "cell at (%d, %d) was not found in u->cells list", c->x, c->y);
					return 0;
				}
			}
		}
	}

	return 1;
}

static int read_kfmo_item(PHASCII_INSTANCE pi, int i, KFORTH_MUTATE_OPTIONS *kfmo, char *errmsg)
{
	int n;

	n = Phascii_Get(pi, "KFMO[%0].MAX_APPLY", i, "%d", &kfmo->max_apply);
	if( n != 1 ) {
		errfmt(errmsg, "missing KFMO[%d].MAX_APPLY", i);
		return 0;
	}

	n = Phascii_Get(pi, "KFMO[%0].PROB_MUTATE_CODEBLOCK", i, "%d", &kfmo->prob_mutate_codeblock);
	if( n != 1 ) {
		errfmt(errmsg, "missing KFMO[%d].PROB_MUTATE_CODEBLOCK", i);
		return 0;
	}

	n = Phascii_Get(pi, "KFMO[%0].PROB_DUPLICATE", i, "%d", &kfmo->prob_duplicate);
	if( n != 1 ) {
		errfmt(errmsg, "missing KFMO[%d].PROB_DUPLICATE", i);
		return 0;
	}

	n = Phascii_Get(pi, "KFMO[%0].PROB_DELETE", i, "%d", &kfmo->prob_delete);
	if( n != 1 ) {
		errfmt(errmsg, "missing KFMO[%d].PROB_DELETE", i);
		return 0;
	}

	n = Phascii_Get(pi, "KFMO[%0].PROB_INSERT", i, "%d", &kfmo->prob_insert);
	if( n != 1 ) {
		errfmt(errmsg, "missing KFMO[%d].PROB_INSERT", i);
		return 0;
	}

	n = Phascii_Get(pi, "KFMO[%0].PROB_TRANSPOSE", i, "%d", &kfmo->prob_transpose);
	if( n != 1 ) {
		errfmt(errmsg, "missing KFMO[%d].PROB_TRANSPOSE", i);
		return 0;
	}

	n = Phascii_Get(pi, "KFMO[%0].PROB_MODIFY", i, "%d", &kfmo->prob_modify);
	if( n != 1 ) {
		errfmt(errmsg, "missing KFMO[%d].PROB_MODIFY", i);
		return 0;
	}

	n = Phascii_Get(pi, "KFMO[%0].MERGE_MODE", i, "%d", &kfmo->merge_mode);
	if( n != 1 ) {
		errfmt(errmsg, "missing KFMO[%d].MERGE_MODE", i);
		return 0;
	}

	n = Phascii_Get(pi, "KFMO[%0].XLEN", i, "%d", &kfmo->xlen);
	if( n != 1 ) {
		errfmt(errmsg, "missing KFMO[%d].XLEN", i);
		return 0;
	}

	n = Phascii_Get(pi, "KFMO[%0].PROTECTED_CODEBLOCKS", i, "%d", &kfmo->protected_codeblocks);
	if( n != 1 ) {
		errfmt(errmsg, "missing KFMO[%d].PROTECTED_CODEBLOCKS", i);
		return 0;
	}

	n = Phascii_Get(pi, "KFMO[%0].MAX_CODE_BLOCKS", i, "%d", &kfmo->max_code_blocks);
	if( n != 1 ) {
		errfmt(errmsg, "missing KFMO[%d].MAX_CODE_BLOCKS", i);
		return 0;
	}

	return 1;
}

static int read_kfmo(PHASCII_INSTANCE pi, UNIVERSE *u, char *errmsg, int *got_it)
{
	int n, i, num, success;

	KFORTH_MUTATE_OPTIONS *kfmo;

	ASSERT( pi != NULL );
	ASSERT( errmsg != NULL );

	if( u == NULL ) {
		errfmt(errmsg, "a UNIVERSE instance must appear before KFMO instance");
		return 0;
	}

	if( *got_it ) {
		errfmt(errmsg, "multiple KFMO instances not allowed");
		return 0;
	}
	
	*got_it += 1;

	n = Phascii_Get(pi, "KFMO.N", "%d", &num);
	if( n != 1 ) {
		errfmt(errmsg, "missing KFMO.N");
		return 0;
	}

	if( num != 8 )
	{
		errfmt(errmsg, "array length must be 8, not KFMO.N=%d", num);
		return 0;
	}

	for(i=0; i < 8; i++)
	{
		kfmo = &u->kfmo[i];
		kforth_mutate_options_defaults(kfmo);

		success = read_kfmo_item(pi, i, kfmo, errmsg);
		if( !success )
			return 0;
	}

	return 1;
}

static int read_organic(PHASCII_INSTANCE pi, UNIVERSE *u, char *errmsg)
{
	int n, i, num;
	int x, y, energy;

	ASSERT( pi != NULL );
	ASSERT( errmsg != NULL );

	if( u == NULL ) {
		errfmt(errmsg, "a UNIVERSE instance must appear before ORGANIC instance");
		return 0;
	}

	n = Phascii_Get(pi, "ORGANIC.N", "%d", &num);
	if( n != 1 ) {
		errfmt(errmsg, "missing ORGANIC.N");
		return 0;
	}

	for(i=0; i < num; i++) {
		n = Phascii_Get(pi, "ORGANIC[%0].X", i, "%d", &x);
		if( n != 1 ) {
			errfmt(errmsg, "ORGANIC[].X missing");
			return 0;
		}

		n = Phascii_Get(pi, "ORGANIC[%0].Y", i, "%d", &y);
		if( n != 1 ) {
			errfmt(errmsg, "ORGANIC[].Y missing");
			return 0;
		}

		n = Phascii_Get(pi, "ORGANIC[%0].ENERGY", i, "%d", &energy);
		if( n != 1 ) {
			errfmt(errmsg, "ORGANIC[].ENERGY missing");
			return 0;
		}

		if( x < 0 || x >= u->width ) {
			errfmt(errmsg, "ORGANIC[%d].X = %d, out of bounds", i, x);
			return 0;
		}

		if( y < 0 || y >= u->height ) {
			errfmt(errmsg, "ORGANIC[%d].Y = %d, out of bounds", i, y);
			return 0;
		}

		if( energy < 0 ) {
			errfmt(errmsg, "ORGANIC[%d].ENERGY = %d, negative", i, energy);
			return 0;
		}

		Grid_SetOrganic(u, x, y, energy);
	}

	return 1;
}

static int read_barrier(PHASCII_INSTANCE pi, UNIVERSE *u, char *errmsg)
{
	int n, i, num;
	int x, y;

	ASSERT( pi != NULL );
	ASSERT( errmsg != NULL );

	if( u == NULL ) {
		errfmt(errmsg, "a UNIVERSE instance must appear before BARRIER instance");
		return 0;
	}

	n = Phascii_Get(pi, "BARRIER.N", "%d", &num);
	if( n != 1 ) {
		errfmt(errmsg, "missing BARRIER.N");
		return 0;
	}

	for(i=0; i < num; i++) {
		n = Phascii_Get(pi, "BARRIER[%0].X", i, "%d", &x);
		if( n != 1 ) {
			errfmt(errmsg, "BARRIER[].X missing");
			return 0;
		}

		n = Phascii_Get(pi, "BARRIER[%0].Y", i, "%d", &y);
		if( n != 1 ) {
			errfmt(errmsg, "BARRIER[].Y missing");
			return 0;
		}

		if( x < 0 || x >= u->width ) {
			errfmt(errmsg, "BARRIER[%d].X = %d, out of bounds", i, x);
			return 0;
		}

		if( y < 0 || y >= u->height ) {
			errfmt(errmsg, "BARRIER[%d].Y = %d, out of bounds", i, y);
			return 0;
		}

		Grid_SetBarrier(u, x, y);
	}

	return 1;
}

static int read_odor_map(PHASCII_INSTANCE pi, UNIVERSE *u, char *errmsg)
{
	int n, i, num, j;
	int x, y, len;
	KFORTH_INTEGER the_value;

	ASSERT( pi != NULL );
	ASSERT( errmsg != NULL );

	if( u == NULL ) {
		errfmt(errmsg, "a UNIVERSE instance must appear before ODOR_MAP instance");
		return 0;
	}

	n = Phascii_Get(pi, "ODOR_MAP.N", "%d", &num);
	if( n != 1 ) {
		errfmt(errmsg, "missing ODOR_MAP.N");
		return 0;
	}

	for(i=0; i < num; i++) {
		n = Phascii_Get(pi, "ODOR_MAP[%0].X", i, "%d", &x);
		if( n != 1 ) {
			errfmt(errmsg, "ODOR_MAP[].X missing");
			return 0;
		}

		n = Phascii_Get(pi, "ODOR_MAP[%0].Y", i, "%d", &y);
		if( n != 1 ) {
			errfmt(errmsg, "ODOR_MAP[].Y missing");
			return 0;
		}

		n = Phascii_Get(pi, "ODOR_MAP[%0].LEN", i, "%d", &len);
		if( n != 1 ) {
			errfmt(errmsg, "ODOR_MAP[].LEN missing");
			return 0;
		}

		n = Phascii_Get(pi, "ODOR_MAP[%0].VALUE", i, "%hd", &the_value);
		if( n != 1 ) {
			errfmt(errmsg, "ODOR_MAP[].VALUE missing");
			return 0;
		}

		if( x < 0 || x >= u->width ) {
			errfmt(errmsg, "ODOR_MAP[%d].X = %d, out of bounds", i, x);
			return 0;
		}

		if( y < 0 || y >= u->height ) {
			errfmt(errmsg, "ODOR_MAP[%d].Y = %d, out of bounds", i, y);
			return 0;
		}

		if( len <= 0 || len > u->width ) {
			errfmt(errmsg, "ODOR_MAP[%d].LEN = %d, out of bounds", i, len);
			return 0;
		}

		for(j=0; j < len; j++)
		{
			Grid_SetOdor(u, x+j, y, the_value);
		}
	}

	return 1;
}


static int read_spore(PHASCII_INSTANCE pi, UNIVERSE *u, KFORTH_SYMTAB **strain_kfst, char *errmsg)
{
	char buf[5000];
	int n, num, i, len;
	int x, y, energy, strain, sflags;
	LONG_LONG parent;
	SPORE *spore;
	KFORTH_PROGRAM *kfp;
	char *program_text;
	char *p, *q;

	ASSERT( pi != NULL );
	ASSERT( errmsg != NULL );
	ASSERT( strain_kfst != NULL );

	if( u == NULL ) {
		errfmt(errmsg, "a UNIVERSE instance must appear before SPORE instance");
		return 0;
	}

	n = Phascii_Get(pi, "SPORE.X", "%d", &x);
	if( n != 1 ) {
		errfmt(errmsg, "missing SPORE.X");
		return 0;
	}

	if( x < 0 || x >= u->width ) {
		errfmt(errmsg, "SPORE.X = %d, out of bounds", x);
		return 0;
	}

	n = Phascii_Get(pi, "SPORE.Y", "%d", &y);
	if( n != 1 ) {
		errfmt(errmsg, "missing SPORE.Y");
		return 0;
	}

	if( y < 0 || y >= u->height ) {
		errfmt(errmsg, "SPORE.Y = %d, out of bounds", y);
		return 0;
	}

	n = Phascii_Get(pi, "SPORE.ENERGY", "%d", &energy);
	if( n != 1 ) {
		errfmt(errmsg, "missing SPORE.ENERGY");
		return 0;
	}

	n = Phascii_Get(pi, "SPORE.PARENT", "%ll", &parent);
	if( n != 1 ) {
		errfmt(errmsg, "missing SPORE.PARENT");
		return 0;
	}

	strain = 0;
	Phascii_Get(pi, "SPORE.STRAIN", "%d", &strain);
	if( n == 1 && (strain < 0 || strain >= 8) )
	{
		errfmt(errmsg, "SPORE.STRAIN out of range 0...7");
		return 0;
	}

	sflags = 0;
	Phascii_Get(pi, "SPORE.SFLAGS", "%d", &sflags);

	n = Phascii_Get(pi, "SPORE.PROGRAM.N", "%d", &num);
	if( n != 1 ) {
		errfmt(errmsg, "missing SPORE.PROGRAM.N");
		return 0;
	}

#ifndef KFORTH_COMPILE_FAST
	/*
	 * Compute length
	 */
	len = 0;
	for(i=0; i < num; i++) {
		n = Phascii_Get(pi, "SPORE.PROGRAM[%0].TEXT_LINE", i, "%s", buf);
		if( n != 1 ) {
			errfmt(errmsg, "missing SPORE.PROGRAM[].TEXT_LINE");
			return 0;
		}

		len += (int) strlen(buf) + 1;
	}
	len += 1;

	program_text = (char*) CALLOC(len, sizeof(char));
	ASSERT( program_text != NULL );

	program_text[0] = '\0';
	q = program_text;
	for(i=0; i < num; i++) {
		n = Phascii_Get(pi, "SPORE.PROGRAM[%0].TEXT_LINE", i, "%s", buf);
		ASSERT( n == 1 );

#ifndef KFORTH_COMPILE_FAST
		strcat(program_text, buf);
		strcat(program_text, "\n");
#else
		p = buf;
		while( *p != '\0' ) {
			*q++ = *p++;
		}
		*q++ = '\n';
		*q = '\0';
#endif
	}
#else
	/* single pass implementation */

	int program_text_len = 1000;
	int idx;

	program_text = (char*) CALLOC(1000, sizeof(char));
	ASSERT( program_text != NULL );
	q = program_text;
	for(i=0; i < num; i++) {
		n = Phascii_Get(pi, "SPORE.PROGRAM[%0].TEXT_LINE", i, "%s", buf);
		if( n != 1 ) {
			errfmt(errmsg, "missing SPORE.PROGRAM[].TEXT_LINE");
			return 0;
		}

		len = strlen(buf);
		idx = q - program_text;

		if( program_text_len - idx < len + 1 + 1 )
		{
			program_text_len += (len + 1 + 1);
			program_text_len *= 2;
			program_text = (char*) REALLOC(program_text, program_text_len);
			q = &program_text[idx];
		}

		p = buf;
		while( *p != '\0' ) {
			*q++ = *p++;
		}
		*q++ = '\n';
		*q = '\0';
	}
#endif

	KFORTH_OPERATIONS *kfops = &u->kfops[strain];
	KFORTH_SYMTAB *kfst = strain_kfst[strain];
	if( kfst == NULL ) {
		errfmt(errmsg, "missing strain_kfst[%d] == NULL", strain);
		return 0;
	}

	kfp = kforth_compile_kfst(program_text, kfst, kfops, errmsg);
	if( kfp == NULL ) {
		FREE(program_text);
		return 0;
	}
	FREE(program_text);

	spore = (SPORE *) CALLOC(1, sizeof(SPORE));
	ASSERT( spore != NULL );

	spore->energy = energy;
	spore->parent = parent;
	spore->strain = strain;
	spore->sflags = sflags;
	spore->program = *kfp;
	FREE(kfp);

	Grid_SetSpore(u, x, y, spore);

	return 1;
}

static int read_organism(PHASCII_INSTANCE pi, UNIVERSE *u, KFORTH_SYMTAB **strain_kfst, char *errmsg)
{
	char buf[5000];
	int n, num, i, len;
	LONG_LONG organism_id, parent1, parent2;
	int generation, energy, age, strain, oflags, sim_count;
	ORGANISM *o, *curr;
	KFORTH_PROGRAM *kfp;
	char *program_text;
	char *p, *q;

	ASSERT( pi != NULL );
	ASSERT( errmsg != NULL );
	ASSERT( strain_kfst != NULL );

	if( u == NULL ) {
		errfmt(errmsg, "a UNIVERSE instance must appear before ORGANISM instance");
		return 0;
	}

	n = Phascii_Get(pi, "ORGANISM.ORGANISM_ID", "%ll", &organism_id);
	if( n != 1 ) {
		errfmt(errmsg, "missing ORGANISM.ORGANISM_ID");
		return 0;
	}

	strain = 0;
	n = Phascii_Get(pi, "ORGANISM.STRAIN", "%d", &strain);
	if( n == 0 || (strain < 0 || strain >= 8) )
	{
		errfmt(errmsg, "missing SPORE.STRAIN or out of range 0...7");
		return 0;
	}

	n = Phascii_Get(pi, "ORGANISM.SIM_COUNT", "%d", &sim_count);
	if( n != 1 )
	{
		errfmt(errmsg, "missing SPORE.SIM_COUNT");
		return 0;
	}

	oflags = 0;
	n = Phascii_Get(pi, "ORGANISM.OFLAGS", "%d", &oflags);
	if( n != 1 ) {
		errfmt(errmsg, "missing ORGANISM.OFLAGS");
		return 0;
	}

	n = Phascii_Get(pi, "ORGANISM.PARENT1", "%ll", &parent1);
	if( n != 1 ) {
		errfmt(errmsg, "missing ORGANISM.PARENT1");
		return 0;
	}

	n = Phascii_Get(pi, "ORGANISM.PARENT2", "%ll", &parent2);
	if( n != 1 ) {
		errfmt(errmsg, "missing ORGANISM.PARENT2");
		return 0;
	}

	n = Phascii_Get(pi, "ORGANISM.GENERATION", "%d", &generation);
	if( n != 1 ) {
		errfmt(errmsg, "missing ORGANISM.GENERATION");
		return 0;
	}

	n = Phascii_Get(pi, "ORGANISM.ENERGY", "%d", &energy);
	if( n != 1 ) {
		errfmt(errmsg, "missing ORGANISM.ENERGY");
		return 0;
	}

	n = Phascii_Get(pi, "ORGANISM.AGE", "%d", &age);
	if( n != 1 ) {
		errfmt(errmsg, "missing ORGANISM.AGE");
		return 0;
	}

	n = Phascii_Get(pi, "ORGANISM.PROGRAM.N", "%d", &num);
	if( n != 1 ) {
		errfmt(errmsg, "missing ORGANISM.PROGRAM.N");
		return 0;
	}

#ifndef KFORTH_COMPILE_FAST
	/*
	 * Compute length
	 */
	len = 0;
	for(i=0; i < num; i++) {
		n = Phascii_Get(pi, "ORGANISM.PROGRAM[%0].TEXT_LINE", i, "%s", buf);
		if( n != 1 ) {
			errfmt(errmsg, "missing ORGANISM.PROGRAM[].TEXT_LINE");
			return 0;
		}

		len += (int) strlen(buf) + 1;
	}
	len += 1;

	program_text = (char*) CALLOC(len, sizeof(char));
	ASSERT( program_text != NULL );

	program_text[0] = '\0';
	q = program_text;
	for(i=0; i < num; i++) {
		n = Phascii_Get(pi, "ORGANISM.PROGRAM[%0].TEXT_LINE", i, "%s", buf);
		ASSERT( n == 1 );

#ifndef KFORTH_COMPILE_FAST
		strcat(program_text, buf);
		strcat(program_text, "\n");
#else
		p = buf;
		while( *p != '\0' ) {
			*q++ = *p++;
		}
		*q++ = '\n';
		*q = '\0';
#endif
	}
#else
	/* single pass implementation */

	int program_text_len = 1000;
	int idx;

	program_text = (char*) CALLOC(1000, sizeof(char));
	ASSERT( program_text != NULL );
	q = program_text;
	for(i=0; i < num; i++) {
		n = Phascii_Get(pi, "ORGANISM.PROGRAM[%0].TEXT_LINE", i, "%s", buf);
		if( n != 1 ) {
			errfmt(errmsg, "missing ORGANISM.PROGRAM[].TEXT_LINE");
			return 0;
		}

		len = strlen(buf);
		idx = q - program_text;

		if( program_text_len - idx < len + 1 + 1 )
		{
			program_text_len += (len + 1 + 1);
			program_text_len *= 2;
			program_text = (char*) REALLOC(program_text, program_text_len);
			q = &program_text[idx];
		}

		p = buf;
		while( *p != '\0' ) {
			*q++ = *p++;
		}
		*q++ = '\n';
		*q = '\0';
	}

#endif

	KFORTH_OPERATIONS *kfops = &u->kfops[strain];
	KFORTH_SYMTAB *kfst = strain_kfst[strain];
	if( kfst == NULL ) {
		errfmt(errmsg, "missing strain_kfst[%d] == NULL", strain);
		return 0;
	}

	kfp = kforth_compile_kfst(program_text, kfst, kfops, errmsg);
	if( kfp == NULL ) {
		FREE(program_text);
		return 0;
	}
	FREE(program_text);

	kfp->nprotected = u->kfmo[strain].protected_codeblocks;

	o = (ORGANISM *) CALLOC(1, sizeof(ORGANISM));
	ASSERT( o != NULL );

	o->id			= organism_id;
	o->strain		= strain;
	o->sim_count	= sim_count;
	o->oflags		= oflags;
	o->parent1		= parent1;
	o->parent2		= parent2;
	o->generation	= generation;
	o->energy		= energy;
	o->age			= age;
	o->program		= *kfp;
	FREE(kfp);

	/*
	 * Attach organism to universe
	 * (preserve order in 'u->organisms' list)
	 */
	u->norganism	+= 1;

	if( u->organisms == NULL ) {
		u->organisms = o;
		o->prev = NULL;
		o->next = NULL;

	} else {
		for(curr=u->organisms; curr->next; curr=curr->next) {
		}

		curr->next = o;
		o->prev = curr;
		o->next = NULL;
	}

	return 1;
}

/*
 * Read a cell and attach to organism.
 *
 */
static int read_cell(PHASCII_INSTANCE pi, UNIVERSE *u, char *errmsg)
{
	int i, n, num;
	int cb, pc;
	LONG_LONG organism_id;
	KFORTH_INTEGER value;
	ORGANISM *o, *ocurr;
	CELL *c, *ccurr;
	KFORTH_MACHINE kfm;
	int terminated;

	ASSERT( pi != NULL );
	ASSERT( errmsg != NULL );

	if( u == NULL ) {
		errfmt(errmsg, "a UNIVERSE instance must appear before CELL instance");
		return 0;
	}

	n = Phascii_Get(pi, "CELL.ORGANISM_ID", "%ll", &organism_id);
	if( n != 1 ) {
		errfmt(errmsg, "missing CELL.ORGANISM_ID");
		return 0;
	}

	c = (CELL*) CALLOC(1, sizeof(CELL));
	ASSERT( c != NULL );

	n = Phascii_Get(pi, "CELL.X", "%d", &c->x);
	if( n != 1 ) {
		FREE(c);
		errfmt(errmsg, "missing CELL.X");
		return 0;
	}

	if( c->x < 0 || c->x >= u->width ) {
		FREE(c);
		errfmt(errmsg, "CELL.X = %d, out of bounds", c->x);
		return 0;
	}

	n = Phascii_Get(pi, "CELL.Y", "%d", &c->y);
	if( n != 1 ) {
		FREE(c);
		errfmt(errmsg, "missing CELL.Y");
		return 0;
	}

	if( c->y < 0 || c->y >= u->height ) {
		FREE(c);
		errfmt(errmsg, "CELL.Y = %d, out of bounds", c->y);
		return 0;
	}
	
	n = Phascii_Get(pi, "CELL.MOOD", "%hd", &c->mood);
	if( n != 1 ) {
		FREE(c);
		errfmt(errmsg, "missing CELL.MOOD");
		return 0;
	}

	n = Phascii_Get(pi, "CELL.MESSAGE", "%hd", &c->message);
	if( n != 1 ) {
		FREE(c);
		errfmt(errmsg, "missing CELL.MOOD");
		return 0;
	}

	kforth_machine_init(&kfm);

	n = Phascii_Get(pi, "CELL.MACHINE.TERMINATED", "%d", &terminated);
	if( n != 1 ) {
		kforth_machine_deinit(&kfm);
		FREE(c);
		errfmt(errmsg, "missing CELL.MACHINE.TERMINATED");
		return 0;
	}

	if( terminated ) {
		kforth_machine_terminate(&kfm);
	}

	n = Phascii_Get(pi, "CELL.MACHINE.CB", "%hd", &kfm.loc.cb);
	if( n != 1 ) {
		kforth_machine_deinit(&kfm);
		FREE(c);
		errfmt(errmsg, "missing CELL.MACHINE.CB");
		return 0;
	}

	n = Phascii_Get(pi, "CELL.MACHINE.PC", "%hd", &kfm.loc.pc);
	if( n != 1 ) {
		kforth_machine_deinit(&kfm);
		FREE(c);
		errfmt(errmsg, "missing CELL.MACHINE.PC");
		return 0;
	}

	/*
	 * Read REGISTER array
	 */
	for(i=0; i<10; i++) {
		n = Phascii_Get(pi, "CELL.MACHINE.R[%0].VALUE", i, "%hd", &kfm.R[i]);
		if( n != 1 ) {
			kforth_machine_deinit(&kfm);
			FREE(c);
			errfmt(errmsg, "missing CELL.MACHINE.R[%d].VALUE", i);
			return 0;
		}
	}

	/*
	 * Read CALL_STACK
	 */
	n = Phascii_Get(pi, "CELL.MACHINE.CALL_STACK.N", "%d", &num);
	if( n != 1 ) {
		kforth_machine_deinit(&kfm);
		FREE(c);
		errfmt(errmsg, "missing CELL.MACHINE.CALL_STACK.N");
		return 0;
	}

	for(i=0; i < num; i++) {
		n = Phascii_Get(pi, "CELL.MACHINE.CALL_STACK[%0].CB", i, "%d", &cb);
		if( n != 1 ) {
			kforth_machine_deinit(&kfm);
			FREE(c);
			errfmt(errmsg, "missing CELL.MACHINE.CALL_STACK[%d].CB", i);
			return 0;
		}

		n = Phascii_Get(pi, "CELL.MACHINE.CALL_STACK[%0].PC", i, "%d", &pc);
		if( n != 1 ) {
			kforth_machine_deinit(&kfm);
			FREE(c);
			errfmt(errmsg, "missing CELL.MACHINE.CALL_STACK[%d].PC", i);
			return 0;
		}

		kforth_call_stack_push(&kfm, cb, pc);
	}


	/*
	 * Read DATA_STACK
	 */
	n = Phascii_Get(pi, "CELL.MACHINE.DATA_STACK.N", "%d", &num);
	if( n != 1 ) {
		kforth_machine_deinit(&kfm);
		FREE(c);
		errfmt(errmsg, "missing CELL.MACHINE.DATA_STACK.N");
		return 0;
	}

	for(i=0; i < num; i++) {
		n = Phascii_Get(pi, "CELL.MACHINE.DATA_STACK[%0].VALUE", i, "%hd", &value);
		if( n != 1 ) {
			kforth_machine_deinit(&kfm);
			FREE(c);
			errfmt(errmsg, "missing CELL.MACHINE.DATA_STACK[%d].VALUE", i);
			return 0;
		}
		kforth_data_stack_push(&kfm, value);
	}

	/*
	 * Find organism
	 */
	o = NULL;
	for(ocurr=u->organisms; ocurr; ocurr=ocurr->next) {
		if( ocurr->id == organism_id ) {
			o = ocurr;
			break;
		}
	}

	if( o == NULL ) {
		errfmt(errmsg, "ORGANISM %lld not found", organism_id);
		kforth_machine_deinit(&kfm);
		FREE(c);
		return 0;
	}

	c->kfm			= kfm;
	c->organism		= o;

	/*
	 * Attach cell to organism
	 */
	o->ncells += 1;
	if( o->cells == NULL ) {
		o->cells = c;
	} else {
		for(ccurr=o->cells; ccurr->next; ccurr=ccurr->next)
			;
		ccurr->next = c;
	}

	Grid_SetCell(u, c);

	return 1;
}

/***********************************************************************
 * Symbol Tables
 *
 * For each strain build a KFORTH_SYMTAB for that strains
 * kforth instruction symbols.
 *
 * 	KFORTH_SYMTAB*	strain_kfst[8];		<--- kfst
 * 	build a struct of up to 8 of these things:
 */
static void make_fast_symtabs(UNIVERSE *u, KFORTH_SYMTAB** strain_kfst)
{
	int i;

	ASSERT( u != NULL );
	ASSERT( strain_kfst != NULL );

	for(i=0; i < 8; i++) {
		strain_kfst[i] = NULL;

		if( u->strop[i].enabled ) {
			strain_kfst[i] = kforth_symtab_make( &u->kfops[i] );
		}
	}
}

static void delete_fast_symtabs(KFORTH_SYMTAB** strain_kfst)
{
	int i;

	ASSERT( strain_kfst != NULL );

	for(i=0; i < 8; i++) {
		if( strain_kfst[i] != NULL ) {
			kforth_symtab_delete(strain_kfst[i]);
		}
	}
}

/***********************************************************************
 * Read a universe from 'filename'.
 *
 * We're reading photon ascii. So the basic algorithm is to read
 * 'Photon Ascii Instances' and then switch on the type and read the fields.
 * Ignore unknown instances.
 *
 * RETURNS:
 *	On Success, a universe object is returned.
 *	On Failure, NULL is returned and errbuf contains a error message.
 *
 */
static UNIVERSE *Do_Read_Ascii(const char *filename, PhasciiReadCB rcb, char *errbuf)
{
	PHASCII_FILE phf;
	PHASCII_INSTANCE pi;
	char errmsg[1000];
	int success;
	UNIVERSE *u;
	int cc_x, cc_y;
	int got_er;
	int got_kfmo;
	int got_strain_opcodes;
	int got_strain_options;
	int got_sim_options;
	int got_cell_list;
	KFORTH_SYMTAB* strain_kfst[8];		// fast hash table of instruction opcodes for each strain 
	int i;

#if 0
	// debug time the read operation
	time_t x;
	time(&x);
	printf("Do_Read_Ascii BEGIN %s\n", ctime(&x));
#endif

	ASSERT( filename != NULL );
	ASSERT( errbuf != NULL );

	for(i=0; i < 8; i++) {
		strain_kfst[i] = NULL;
	}

	if( rcb != NULL )
	{
		phf = Phascii_Open_ReadCB(filename, rcb);
		if( phf == NULL ) {
			strcpy(errbuf, Phascii_GetError());
			return NULL;
		}
	} else {
		phf = Phascii_Open(filename, "r");
		if( phf == NULL ) {
			strcpy(errbuf, Phascii_GetError());
			return NULL;
		}
	}

	got_er = 0;
	got_kfmo = 0;
	got_strain_opcodes = 0;
	got_strain_options = 0;
	got_sim_options = 0;
	got_cell_list = 0;
	cc_x = -1;
	cc_y = -1;
	u = NULL;

	while( (pi = Phascii_GetInstance(phf)) ) {

		if( Phascii_IsInstance(pi, "ORGANIC") ) {
			success = read_organic(pi, u, errmsg);

		} else if( Phascii_IsInstance(pi, "BARRIER") ) {
			success = read_barrier(pi, u, errmsg);

		} else if( Phascii_IsInstance(pi, "ODOR_MAP") ) {
			success = read_odor_map(pi, u, errmsg);

		} else if( Phascii_IsInstance(pi, "ER") ) {
			success = read_er(pi, u, errmsg, &got_er);

		} else if( Phascii_IsInstance(pi, "KFMO") ) {
			success = read_kfmo(pi, u, errmsg, &got_kfmo);

		} else if( Phascii_IsInstance(pi, "SPORE") ) {
			success = read_spore(pi, u, strain_kfst, errmsg);

		} else if( Phascii_IsInstance(pi, "CELL") ) {
			success = read_cell(pi, u, errmsg);

		} else if( Phascii_IsInstance(pi, "ORGANISM") ) {
			success = read_organism(pi, u, strain_kfst, errmsg);

		} else if( Phascii_IsInstance(pi, "UNIVERSE") ) {
			success = read_universe(pi, &u, errmsg, &cc_x, &cc_y);

		} else if( Phascii_IsInstance(pi, "SIMULATION_OPTIONS") ) {
			success = read_simulation_options(pi, u, errmsg, &got_sim_options);

		} else if( Phascii_IsInstance(pi, "STRAIN_OPTIONS") ) {
			success = read_strain_options(pi, u, errmsg, &got_strain_options);

		} else if( Phascii_IsInstance(pi, "STRAIN_OPCODES") ) {
			success = read_strain_opcodes(pi, u, errmsg, &got_strain_opcodes);
			if( success ) {
				make_fast_symtabs(u, strain_kfst);
			}

		} else if( Phascii_IsInstance(pi, "CELL_LIST") ) {
			success = read_cell_list(pi, u, errmsg, &got_cell_list);

		} else {
			success = 1;
		}

		Phascii_FreeInstance(pi);

		if( ! success ) {
			errfmt(errbuf, "%s", errmsg);
			Phascii_Close(phf);
			return NULL;
		}
	}

	delete_fast_symtabs(strain_kfst);

	if( ! Phascii_Eof(phf) ) {
		errfmt(errbuf, "%s\n", Phascii_Error(phf));
		Phascii_Close(phf);
		return NULL;
	}

	Phascii_Close(phf);

	if( u == NULL ) {
		errfmt(errbuf, "%s: No UNIVERSE instance", filename);
		return NULL;
	}

	/*
	 * Attach current_cell, based on its coordinates. This happens at the end
	 * after all cells have been populated
	 */
	if( cc_x == -1 && cc_y == -1 ) {
		errfmt(errbuf, "%s: current_cell not set (%d, %d). could not be found", filename, cc_x, cc_y);
		Universe_Delete(u);
		return NULL;
	} else {
		GRID_TYPE type;
		UNIVERSE_GRID ugrid;

		type = Grid_Get(u, cc_x, cc_y, &ugrid);
		if( type != GT_CELL ) {
			errfmt(errbuf, "%s: current_cell at (%d, %d) -> %d, could not be found", filename, cc_x, cc_y, type);
			Universe_Delete(u);
			return NULL;
		}

		u->current_cell = ugrid.u.cell;
	}

	/*
	 * Rebuild u->strpop[]
	 */
	ORGANISM *o;
	for(o=u->organisms; o; o=o->next) {
		u->strpop[ o->strain ] += 1;
	}

#if 0
	// KJS debug timer
	time(&x);
	printf("Do_Read_Ascii END %s\n", ctime(&x));
#endif

	return u;
}

UNIVERSE *Universe_ReadAscii(const char *filename, char *errbuf)
{
	return Do_Read_Ascii(filename, NULL, errbuf);
}

//
// provide a reader callback 'rcb' which gets data from the input (Swift NSData thingy)
//
UNIVERSE *Universe_ReadAscii_CB(const char *name, intptr_t (*rcb)(char *buf, intptr_t reqlen), char *errbuf)
{
	return Do_Read_Ascii(name, (PhasciiReadCB)rcb, errbuf);
}

int Universe_WriteAscii(UNIVERSE *u, const char *filename, char *errbuf)
{
	return Do_Write_Ascii(u, filename, NULL, errbuf);
}

intptr_t Universe_WriteAscii_CB( UNIVERSE *u,
						const char *name,
						intptr_t (*wcb)(const char *buf, intptr_t len),
						char *errbuf)
{
	return Do_Write_Ascii(u, name, (PhasciiWriteCB)wcb, errbuf);
}

static int read_evolve_preferences(PHASCII_INSTANCE pi, EVOLVE_PREFERENCES *ep, char *errmsg, int *got_ep)
{
	int n, num, i;
	EVOLVE_DFLT *ed;

	if( *got_ep ) {
		errfmt(errmsg, "multiple EVOLVE_PREFERENCES instances not allowed");
		return 0;
	}
	
	*got_ep += 1;

	n = Phascii_Get(pi, "EVOLVE_PREFERENCES.EVOLVE_BATCH", "%1000s", ep->evolve_batch_path);
	if( n != 1 ) {
		errfmt(errmsg, "missing EVOLVE_PREFERENCES.EVOLVE_BATCH");
		return 0;
	}

	n = Phascii_Get(pi, "EVOLVE_PREFERENCES.EVOLVE_3D", "%1000s", ep->evolve_3d_path);
	if( n != 1 ) {
		errfmt(errmsg, "missing EVOLVE_PREFERENCES.EVOLVE_3D");
		return 0;
	}

	n = Phascii_Get(pi, "EVOLVE_PREFERENCES.HELP", "%1000s", ep->help_path);
	if( n != 1 ) {
		errfmt(errmsg, "missing EVOLVE_PREFERENCES.HELP");
		return 0;
	}

	n = Phascii_Get(pi, "EVOLVE_PREFERENCES.WIDTH", "%d", &ep->width);
	if( n != 1 ) {
		errfmt(errmsg, "missing EVOLVE_PREFERENCES.WIDTH");
		return 0;
	}

	n = Phascii_Get(pi, "EVOLVE_PREFERENCES.HEIGHT", "%d", &ep->height);
	if( n != 1 ) {
		errfmt(errmsg, "missing EVOLVE_PREFERENCES.HEIGHT");
		return 0;
	}

	n = Phascii_Get(pi, "EVOLVE_PREFERENCES.WANT_BARRIER", "%d", &ep->want_barrier);
	if( n != 1 ) {
		errfmt(errmsg, "missing EVOLVE_PREFERENCES.WANT_BARRIER");
		return 0;
	}

	n = Phascii_Get(pi, "EVOLVE_PREFERENCES.TERRAIN", "%1000s", ep->terrain_file);
	if( n != 1 ) {
		errfmt(errmsg, "missing EVOLVE_PREFERENCES.TERRAIN");
		return 0;
	}

	n = Phascii_Get(pi, "EVOLVE_PREFERENCES.DFLT.N", "%d", &num);
	if( n != 1 ) {
		errfmt(errmsg, "missing EVOLVE_PREFERENCES.DFLT.N");
		return 0;
	}

	if( num != 8 ) {
		errfmt(errmsg, "EVOLVE_PREFERENCES.DFLT.N must be 8, but got %d", num);
		return 0;
	}

 	for(i=0; i < 8; i++) {
		ed = &ep->dflt[i];

		n = Phascii_Get(pi, "EVOLVE_PREFERENCES.DFLT[%0].STRAIN", i, "%d", &ed->profile_idx);
		if( n != 1 ) {
			errfmt(errmsg, "missing EVOLVE_PREFERENCES.DFLT[%d].STRAIN", i);
			return 0;
		}

		n = Phascii_Get(pi, "EVOLVE_PREFERENCES.DFLT[%0].ENERGY", i, "%d", &ed->energy);
		if( n != 1 ) {
			errfmt(errmsg, "missing EVOLVE_PREFERENCES.DFLT[%d].ENERGY", i);
			return 0;
		}

		n = Phascii_Get(pi, "EVOLVE_PREFERENCES.DFLT[%0].POPULATION", i, "%d", &ed->population);
		if( n != 1 ) {
			errfmt(errmsg, "missing EVOLVE_PREFERENCES.DFLT[%d].POPLATION", i);
			return 0;
		}

		n = Phascii_Get(pi, "EVOLVE_PREFERENCES.DFLT[%0].SEED_FILE", i, "%1000s", ed->seed_file);
		if( n != 1 ) {
			errfmt(errmsg, "missing EVOLVE_PREFERENCES.DFLT[%d].SEED_FILE", i);
			return 0;
		}
	}

	return 1;
}

static int read_prefs_simulation_options(PHASCII_INSTANCE pi, EVOLVE_PREFERENCES *ep, char *errmsg, int *got_so)
{
	if( *got_so ) {
		errfmt(errmsg, "multiple SIMULATION_OPTIONS instances not allowed");
		return 0;
	}
	
	*got_so += 1;

	return read_simulation_options_item(pi, &ep->so, errmsg);
}

static int read_strain_profile_item(PHASCII_INSTANCE pi, int i, STRAIN_PROFILE *profile, char *errmsg)
{
	int n, num, len, j;
	char buf[1000];

	n = Phascii_Get(pi, "STRAIN_PROFILES[%0].NAME", i, "%100s", profile->name);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_PROFILES[%d].NAME", i);
		return 0;
	}

	n = Phascii_Get(pi, "STRAIN_PROFILES[%0].SEED_FILE", i, "%1000s", profile->seed_file);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_PROFILES[%d].SEED_FILE", i);
		return 0;
	}

	n = Phascii_Get(pi, "STRAIN_PROFILES[%0].ENERGY", i, "%d", &profile->energy);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_PROFILES[%d].ENERGY", i);
		return 0;
	}

	n = Phascii_Get(pi, "STRAIN_PROFILES[%0].POPULATION", i, "%d", &profile->population);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_PROFILES[%d].POPULATION", i);
		return 0;
	}

	n = Phascii_Get(pi, "STRAIN_PROFILES[%0].DESCRIPTION.M", i, "%d", &num);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_PROFILES[%d].DESCIPTION.", i);
		return 0;
	}

	profile->description[0] = '\0';

	for(j=0; j < num; j++)
	{
		n = Phascii_Get(pi, "STRAIN_PROFILES[%0].DESCRIPTION[%1].TEXT_LINE", i, j, "%900s", buf);
		if( n != 1 ) {
			errfmt(errmsg, "missing STRAIN_PROFILES.DESCRIPTION[].TEXT_LINE");
			return 0;
		}
		strcat(buf, "\n");

		len = (int)strlen(profile->description) + (int)strlen(buf) + 1;

		if( len < sizeof(profile->description) ) {
			strcat(profile->description, buf);
		}
	}

	return 1;
}

static int read_prefs_strain_profiles(PHASCII_INSTANCE pi, EVOLVE_PREFERENCES *ep, char *errmsg, int *got_sp)
{
	int i, n, num;
	int success;
	STRAIN_PROFILE *profile;

	ASSERT( pi != NULL );
	ASSERT( ep != NULL );
	ASSERT( errmsg != NULL );
	ASSERT( got_sp != NULL );

	if( *got_sp ) {
		errfmt(errmsg, "multiple STRAIN_PROFILES instances not allowed");
		return 0;
	}
	
	*got_sp += 1;

	n = Phascii_Get(pi, "STRAIN_PROFILES.N", "%d", &num);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_PROFILES.N");
		return 0;
	}

	if( num > 100 )
	{
		errfmt(errmsg, "STRAIN_PROFILES.N=%d too large. %d > 100", num, num);
		return 0;
	}

	FREE(ep->strain_profiles);
	ep->strain_profiles = (STRAIN_PROFILE*) CALLOC(num, sizeof(STRAIN_PROFILE));
	ASSERT( ep->strain_profiles != NULL );

	for(i=0; i < num; i++)
	{
		profile = &ep->strain_profiles[i];
		StrainProfile_Init( profile );

		success = read_strain_profile_item(pi, i, profile, errmsg);
		if( !success )
			return 0;

		ep->nprofiles += 1;
	}

	return 1;
}

static int read_prefs_strain_options(PHASCII_INSTANCE pi, EVOLVE_PREFERENCES *ep, char *errmsg, int *got_sto)
{
	int i, n, num;
	int success;
	STRAIN_OPTIONS *strop;

	ASSERT( pi != NULL );
	ASSERT( ep != NULL );
	ASSERT( errmsg != NULL );
	ASSERT( got_sto != NULL );

	if( *got_sto ) {
		errfmt(errmsg, "multiple STRAIN_OPTIONS instances not allowed");
		return 0;
	}
	
	*got_sto += 1;

	n = Phascii_Get(pi, "STRAIN_OPTIONS.N", "%d", &num);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_OPTIONS.N");
		return 0;
	}

	if( num != ep->nprofiles )
	{
		errfmt(errmsg, "STRAIN_OPTIONS.N array length must be %d (STRAIN_PROFILE.N), not %d", ep->nprofiles, num);
		return 0;
	}

	for(i=0; i < ep->nprofiles; i++)
	{
		strop = &ep->strain_profiles[i].strop;

		success = read_strain_options_item(pi, i, strop, errmsg);
		if( ! success ) {
			return 0;
		}
	}

	return 1;
}

static int read_prefs_kfmo(PHASCII_INSTANCE pi, EVOLVE_PREFERENCES *ep, char *errmsg, int *got_kfmo)
{
	int i, n, num, success;
	KFORTH_MUTATE_OPTIONS* kfmo;

	ASSERT( pi != NULL );
	ASSERT( ep != NULL );
	ASSERT( errmsg != NULL );
	ASSERT( got_kfmo != NULL );

	if( *got_kfmo ) {
		errfmt(errmsg, "multiple KFMO instances not allowed");
		return 0;
	}
	
	*got_kfmo += 1;

	n = Phascii_Get(pi, "KFMO.N", "%d", &num);
	if( n != 1 ) {
		errfmt(errmsg, "missing KFMO.N");
		return 0;
	}

	if( num != ep->nprofiles )
	{
		errfmt(errmsg, "KFMO.N array length must be %d (STRAIN_PROFILE.N), not %d", ep->nprofiles, num);
		return 0;
	}

	for(i=0; i < ep->nprofiles; i++)
	{
		kfmo = &ep->strain_profiles[i].kfmo;

		success = read_kfmo_item(pi, i, kfmo, errmsg);
		if( ! success ) {
			return 0;
		}
	}

	return 1;
}

static int read_prefs_strain_opcodes(PHASCII_INSTANCE pi, EVOLVE_PREFERENCES *ep, char *errmsg, int *got_strop)
{
	int i, n, num, success;
	KFORTH_OPERATIONS* kfops;
	KFORTH_OPERATIONS *master_kfops;

	ASSERT( pi != NULL );
	ASSERT( ep != NULL );
	ASSERT( errmsg != NULL );
	ASSERT( got_strop != NULL );

	if( *got_strop ) {
		errfmt(errmsg, "multiple STRAIN_OPCODES instances not allowed");
		return 0;
	}
	
	*got_strop += 1;

	n = Phascii_Get(pi, "STRAIN_OPCODES.N", "%d", &num);
	if( n != 1 ) {
		errfmt(errmsg, "missing STRAIN_OPCODES.N");
		return 0;
	}

	if( num != ep->nprofiles )
	{
		errfmt(errmsg, "STRAIN_OPCODES.N array length must be %d (STRAIN_PROFILE.N), not %d", ep->nprofiles, num);
		return 0;
	}

	master_kfops = EvolveOperations();

	for(i=0; i < ep->nprofiles; i++)
	{
		kfops = &ep->strain_profiles[i].kfops;

		success = read_strain_opcodes_item(pi, master_kfops, i, kfops, errmsg);
		if( ! success ) {
			return 0;
		}
	}

	return 1;
}

/***********************************************************************
 *
 *
 */
static int Do_Read_Preferences( EVOLVE_PREFERENCES *ep,
					const char *filename,
					PhasciiReadCB rcb,
					char *errbuf )
{
	PHASCII_FILE phf;
	PHASCII_INSTANCE pi;
	char errmsg[1000];
	int success;
	int got_ep;
	int got_so;
	int got_sp;
	int got_sto;
	int got_kfmo;
	int got_strop;
	int i;
	EVOLVE_DFLT *ed;

	ASSERT( ep != NULL );
	ASSERT( filename != NULL );
	ASSERT( errbuf != NULL );

	if( rcb != NULL )
	{
		phf = Phascii_Open_ReadCB(filename, rcb);
		if( phf == NULL ) {
			strcpy(errbuf, Phascii_GetError());
			return 0;
		}
	} else {
		phf = Phascii_Open(filename, "r");
		if( phf == NULL ) {
			strcpy(errbuf, Phascii_GetError());
			return 0;
		}
	}

	got_ep = 0;
	got_so = 0;
	got_sp = 0;
	got_sto = 0;
	got_kfmo = 0;
	got_strop = 0;

	while( (pi = Phascii_GetInstance(phf)) ) {

		if( Phascii_IsInstance(pi, "EVOLVE_PREFERENCES") ) {
			success = read_evolve_preferences(pi, ep, errmsg, &got_ep);

		} else if( Phascii_IsInstance(pi, "SIMULATION_OPTIONS") ) {
			success = read_prefs_simulation_options(pi, ep, errmsg, &got_so);

		} else if( Phascii_IsInstance(pi, "STRAIN_PROFILES") ) {
			success = read_prefs_strain_profiles(pi, ep, errmsg, &got_sp);

		} else if( Phascii_IsInstance(pi, "STRAIN_OPTIONS") ) {
			success = read_prefs_strain_options(pi, ep, errmsg, &got_sto);

		} else if( Phascii_IsInstance(pi, "KFMO") ) {
			success = read_prefs_kfmo(pi, ep, errmsg, &got_kfmo);

		} else if( Phascii_IsInstance(pi, "STRAIN_OPCODES") ) {
			success = read_prefs_strain_opcodes(pi, ep, errmsg, &got_strop);

		} else {
			success = 1;
		}

		Phascii_FreeInstance(pi);

		if( ! success ) {
			errfmt(errbuf, "%s", errmsg);
			Phascii_Close(phf);
			return NULL;
		}
	}

	if( ! Phascii_Eof(phf) ) {
		errfmt(errbuf, "%s\n", Phascii_Error(phf));
		Phascii_Close(phf);
		return 0;
	}

	Phascii_Close(phf);

	//
	// Error Checking
	//
	for(i=0; i < 8; i++) {
		ed = &ep->dflt[i];

		if( ed->profile_idx < -1 || ed->profile_idx >= ep->nprofiles )
		{
			errfmt(errbuf, "EVOLVE_PREFERENCES.DFLT[%d].PROFILE out of range", i);
			return 0;
		}
	}

	return 1;
}

int EvolvePreferences_Read(EVOLVE_PREFERENCES *ep, const char *filename, char *errbuf)
{
	return Do_Read_Preferences(ep, filename, NULL, errbuf);
}

int EvolvePreferences_Read_CB(EVOLVE_PREFERENCES *ep, const char *name, intptr_t (*rcb)(char *buf, intptr_t reqlen), char *errbuf)
{
	return Do_Read_Preferences(ep, name, (PhasciiReadCB)rcb, errbuf);
}

void write_evolve_preferences_prolog(PHASCII_FILE pf)
{
	int i;

	ASSERT( pf != NULL );

	for(i=0; i < sizeof(prolog1)/sizeof(prolog1[0]); i++) {
		Phascii_Printf(pf, "%s\n", prolog1[i]);
	}

	for(i=0; i < sizeof(prolog3)/sizeof(prolog3[0]); i++) {
		Phascii_Printf(pf, "%s\n", prolog3[i]);
	}

	for(i=0; i < sizeof(prolog5)/sizeof(prolog5[0]); i++) {
		Phascii_Printf(pf, "%s\n", prolog5[i]);
	}
}

void write_evolve_preferences(PHASCII_FILE pf, EVOLVE_PREFERENCES *ep)
{
	int i;

	// KJS TODO: Clean/Escape strings so they won't break phascii.

	Phascii_Printf(pf, "EVOLVE_PREFERENCES\n");
	Phascii_Printf(pf, "\t\"%s\"				# Evolve Batch Path\n", ep->evolve_batch_path);
	Phascii_Printf(pf, "\t\"%s\"				# Evolve 3d Path\n", ep->evolve_3d_path);
	Phascii_Printf(pf, "\t\"%s\"				# Help Path\n", ep->help_path);
	Phascii_Printf(pf, "\t%d					# Default Width\n", ep->width);
	Phascii_Printf(pf, "\t%d					# Default Height\n", ep->height);
	Phascii_Printf(pf, "\t%d					# Default Want Barrier\n", ep->want_barrier);
	Phascii_Printf(pf, "\t%\"%s\"				# Default Terrain File\n", ep->terrain_file);

	Phascii_Printf(pf, "\n");
	Phascii_Printf(pf, "\t8   # number of strains to follow:\n\n");

	for(i=0; i < 8; i++) {

		EVOLVE_DFLT *ed;
		ed = &ep->dflt[i];

		Phascii_Printf(pf, "\t# strain %d\n", i);
		Phascii_Printf(pf, "\t%d				# Default Strain Profile (-1 means this DFLT not set)\n", ed->profile_idx);
		Phascii_Printf(pf, "\t%d				# Default Energy\n", ed->energy);
		Phascii_Printf(pf, "\t%d				# Default Population\n", ed->population);
		Phascii_Printf(pf, "\t\"%s\"			# Default Seed File\n", ed->seed_file);
		Phascii_Printf(pf, "\n");
	}
	Phascii_Printf(pf, "\n");
}

void write_strain_profile_item(PHASCII_FILE pf, int i, STRAIN_PROFILE *sp)
{
	char line[1000];
	char *p;
	int j;

	Phascii_Printf(pf, "\t# Strain Profile (Strain %d)\n", i);
	Phascii_Printf(pf, "\t\"%s\"\n", sp->name);
	Phascii_Printf(pf, "\t\"%s\"\n" , sp->seed_file);

	Phascii_Printf(pf, "\t%d      # default energy\n", sp->energy);
	Phascii_Printf(pf, "\t%d      # default population\n", sp->population);
	Phascii_Printf(pf, "\t{\n");

	p = sp->description;
	while( *p != '\0' )
	{
		j = 0;
		while( *p != '\n' && *p != '\0' ) {
			if( j < 1000 ) {
				line[j++] = *p++;
			}
		}
		line[j] = '\0';

		Phascii_Printf(pf, "\t\t\"%s\"\n", line);
		if( *p == '\n' )
			p++;
	}

	Phascii_Printf(pf, "\t}\n");
	Phascii_Printf(pf, "\n");
}

void write_strain_profiles(PHASCII_FILE pf, int nprofiles, STRAIN_PROFILE* strain_profiles)
{
	int i;

	ASSERT( pf != NULL );
	ASSERT( strain_profiles != NULL );

	Phascii_Printf(pf, "STRAIN_PROFILES {\n");
	for(i=0; i < nprofiles; i++)
	{
		write_strain_profile_item(pf, i, &strain_profiles[i]);
	}
	Phascii_Printf(pf, "}\n\n");

	Phascii_Printf(pf, "STRAIN_OPTIONS {\n");
	for(i=0; i < nprofiles; i++)
	{
		write_strain_options_item(pf, i, &strain_profiles[i].strop);
	}
	Phascii_Printf(pf, "}\n\n");

	Phascii_Printf(pf, "KFMO {\n");
	for(i=0; i < nprofiles; i++)
	{
		write_kfmo_item(pf, i, &strain_profiles[i].kfmo);
	}
	Phascii_Printf(pf, "}\n\n");

	Phascii_Printf(pf, "STRAIN_OPCODES {\n");
	for(i=0; i < nprofiles; i++)
	{
		write_strain_opcodes_item(pf, i, &strain_profiles[i].kfops);
	}
	Phascii_Printf(pf, "}\n\n");
}

/***********************************************************************
 *
 *
 */
static int Do_Write_Preferences( EVOLVE_PREFERENCES *ep,
						const char *filename,
						PhasciiWriteCB wcb,
						char *errbuf )
{
	PHASCII_FILE pf;

	ASSERT( ep != NULL );
	ASSERT( filename != NULL );
	ASSERT( errbuf != NULL );

	if( wcb == NULL ) {
		pf = Phascii_Open(filename, "w");
	} else {
		pf = Phascii_Open_WriteCB(filename, wcb);
	}

	if( pf == NULL )
	{
		if( wcb == NULL ) {
			errfmt(errbuf, "%s: Phascii_Open_WriteCB failed", filename);
		} else {
			errfmt(errbuf, "%s: %s", filename, strerror(errno));
		}
		return 0;
	}

	write_evolve_preferences_prolog(pf);
	write_evolve_preferences(pf, ep);
	write_simulation_options(pf, &ep->so);
	write_strain_profiles(pf, ep->nprofiles, ep->strain_profiles);

	Phascii_Close(pf);
	
	return 1;
}

int EvolvePreferences_Write(EVOLVE_PREFERENCES *ep, const char *filename, char *errbuf)
{
	return Do_Write_Preferences(ep, filename, NULL, errbuf);
}

int EvolvePreferences_Write_CB( EVOLVE_PREFERENCES *ep,
						const char *name,
						intptr_t (*wcb)(const char *buf, intptr_t len),
						char *errbuf )
{
	return Do_Write_Preferences(ep, name, (PhasciiWriteCB)wcb, errbuf);
}

static void transform(UNIVERSE *u, int x, int y, UNIVERSE *u2, int *x2, int *y2)
{
	ASSERT( u != NULL );
	ASSERT( u2 != NULL );
	ASSERT( x2 != NULL );
	ASSERT( y2 != NULL );

	double w, h;
	double w2, h2;
	double fx, fy;
	double fx2, fy2;

	w = (double)u->width;
	h = (double)u->height;

	fx = (double)x;
	fy = (double)y;

	w2 = (double)u2->width;
	h2 = (double)u2->height;

	fx2 = fx/w * w2;
	fy2 = fy/h * h2;

	*x2 = fx2;
	if( *x2 >= u2->width )
		*x2 = u2->width-1;

	*y2 = fy2;
	if( *y2 >= u2->height )
		*y2 = u2->height-1;
}

/*
 * Add terrain to a universe.
 *
 * The data comes from a file, which is a subset of the normal Evolve Simulation
 * photon ascii file.
 * 
 * It is expected to contains a UNIVERSE record with legit width x height values.
 * It is expected to contains some BARRIER instances.
 *
 * The barrier blocks are scaled into 'u' based on dimension data.
 * won't overlay barrier blocks around the strains.
 *
 * Returns: 0 - on error, and errbuf contains the error message
 *		1 on success
 *
 */
int Terrain_Read(UNIVERSE *u, const char *filename, char *errbuf)
{
	PHASCII_FILE phf;
	PHASCII_INSTANCE pi;
	char errmsg[1000];
	UNIVERSE* univ;
	int cc_x, cc_y;
	struct { int top, bottom, left, right; } srect[8];
	int s, x, y, x2, y2, found, success, got_u;
	GRID_TYPE gt;
	UNIVERSE_GRID ugrid;

	ASSERT( u != NULL );
	ASSERT( filename != NULL );
	ASSERT( errbuf != NULL );

	phf = Phascii_Open(filename, "r");
	if( phf == NULL ) {
		strcpy(errbuf, Phascii_GetError());
		return 0;
	}

	univ = NULL;

	got_u = 0;
	while( (pi = Phascii_GetInstance(phf)) ) {
		if( Phascii_IsInstance(pi, "UNIVERSE") ) {
			success = read_universe(pi, &univ, errmsg, &cc_x, &cc_y);
			got_u = 1;
		} else if( Phascii_IsInstance(pi, "BARRIER") ) {
			success = read_barrier(pi, univ, errmsg);
		} else if( Phascii_IsInstance(pi, "SPORE") ) {
			break;
		} else {
			success = 1;
		}

		Phascii_FreeInstance(pi);

		if( ! success ) {
			errfmt(errbuf, "%s", errmsg);
			Phascii_Close(phf);
			return 0;
		}
	}

	Phascii_Close(phf);

	if( ! got_u )
	{
		errfmt(errbuf, "%s: missing UNIVERSE instance in terrain file\n", filename);
		return 0;
	}

	/*
	 * Find bounding box for each strain
	 */
	for(s=0; s < 8; s++) {
		srect[s].top = 0;
		srect[s].bottom = u->height;
		srect[s].left = u->width;
		srect[s].right = 0;
	}

	for(x=0; x < u->width; x++) {
		for(y=0; y < u->height; y++) {
			gt = Grid_Get(u, x, y, &ugrid);
			if( gt == GT_CELL ) {
				s = ugrid.u.cell->organism->strain;
				if( x > srect[s].right )	srect[s].right = x;
				if( x < srect[s].left )		srect[s].left = x;
				if( y < srect[s].bottom )	srect[s].bottom = y;
				if( y > srect[s].top )		srect[s].top = y;
			}
		}
	}

	for(x=0; x < u->width; x++) {
		for(y=0; y < u->height; y++) {
			gt = Grid_Get(u, x, y, &ugrid);
			if( gt != GT_BLANK )
				continue;

			transform(u, x, y, univ, &x2, &y2);
//			x2 = x; if( x2 >= univ->width ) x2 = univ->width-1;
//			y2 = y; if( y2 >= univ->height ) y2 = univ->height-1;

			gt = Grid_Get(univ, x2, y2, &ugrid);
			if( gt != GT_BARRIER ) {
				continue;
			}

			found = 0;
			for(s=0; s < 8; s++) {
				if( x >= srect[s].left && x <= srect[s].right
						&& y >= srect[s].bottom && y <= srect[s].top ) {
					found = 1;
					break;
				}
			}
			if( found )
				continue;

			Grid_SetBarrier(u, x, y);
		}
	}

	Universe_Delete(univ);

	return 1;
}
