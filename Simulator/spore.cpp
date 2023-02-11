/*
 * Copyright (c) 2006 Stauffer Computer Consulting
 */

/***********************************************************************
 * SPORE:
 *
 */
#include "evolve_simulator.h"
#include "evolve_simulator_private.h"

SPORE *Spore_make(KFORTH_PROGRAM *program, int energy, LONG_LONG parent, int strain)
{
	SPORE *spore;

	ASSERT( program != NULL );
	ASSERT( energy > 0 );

	spore = (SPORE *) CALLOC(1, sizeof(SPORE));
	ASSERT( spore != NULL );

	kforth_copy2(program, &spore->program);

	spore->energy = energy;
	spore->parent = parent;
	spore->strain = strain;

	return spore;
}

void Spore_delete(SPORE *spore)
{
	ASSERT( spore != NULL );

	kforth_program_deinit(&spore->program);

	FREE(spore);
}

/*
 * Fertilize the spore:
 */
void Spore_fertilize(UNIVERSE *u, ORGANISM *o, SPORE *spore,
					int x, int y, int energy)
{
	ORGANISM *no;
	CELL *nc;
	KFORTH_PROGRAM np;
	KFORTH_OPERATIONS *kfops;
	KFORTH_MUTATE_OPTIONS *kfmo;

	ASSERT( u != NULL );
	ASSERT( o != NULL );
	ASSERT( spore != NULL );
	ASSERT( o->strain == spore->strain );
	ASSERT( energy >= 0 );
	
	kforth_program_init(&np);
	kfops = &u->kfops[o->strain];
	kfmo = &u->kfmo[o->strain];
	kforth_merge2(&u->er, kfmo, &o->program, &spore->program, &np);
	kforth_mutate(kfops, kfmo, &u->er, &np);

	no = (ORGANISM *) CALLOC(1, sizeof(ORGANISM));
	ASSERT( no != NULL );

	nc = (CELL *) CALLOC(1, sizeof(CELL));
	ASSERT( nc != NULL );

	kforth_machine_init(&nc->kfm);
	nc->x		= x;
	nc->y		= y;
	nc->organism	= no;
	nc->next	= NULL;

	nc->u_next = u->cells;
	nc->u_prev = NULL;
	if( u->cells ) {
		u->cells->u_prev = nc;
	}
	u->cells = nc;

	no->strain	= spore->strain;
	no->id		= u->next_id++;
	no->parent1	= spore->parent;
	no->parent2	= o->id;
	no->generation	= o->generation + 1;
	no->energy	= spore->energy + energy;
	no->age		= 0;
	no->program	= np;

	no->ncells	= 1;
	no->sim_count = 1;
	no->cells	= nc;

	no->next	= NULL;
	no->prev	= NULL;

	/*
	 * If organism or spore is radioactive, then new organism is too
	 */
	if( (spore->sflags & SPORE_FLAG_RADIOACTIVE)
				|| (o->oflags & ORGANISM_FLAG_RADIOACTIVE) ) {
		no->oflags |= ORGANISM_FLAG_RADIOACTIVE;
	}

	Grid_SetCell(u, nc);

	no->next = u->organisms;
	no->prev = NULL;
	if( u->organisms != NULL ) {
		u->organisms->prev = no;
	}
	u->organisms = no;
	u->nborn++;
	u->norganism += 1;
	u->strpop[no->strain] += 1;

	Spore_delete(spore);
}
