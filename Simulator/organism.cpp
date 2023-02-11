/*
 * Copyright (c) 2006 Stauffer Computer Consulting
 */

/***********************************************************************
 * ORGANISM OPERATIONS
 *
 *
 */
#include "evolve_simulator.h"
#include "evolve_simulator_private.h"

/*
 * Color cells:
 *	Set the color field for all cells that
 *	touch 'cell'
 *
 */
#define COLOR_IT(c)	( (c!=NULL) && (!Kforth_Machine_Terminated(&c->kfm) && (c->color==0) ) )

/*
 * Color all non-dead cells reachable from 'cell'
 * New Version: No recursion
 */
static void color_all_cells(UNIVERSE *u, CELL *cell, int *color)
{
	CELL *north, *south, *east, *west;
	CELL *nw, *sw, *ne, *se;

	ASSERT( cell != NULL );
	ASSERT( color != NULL );

	north = Cell_Neighbor(u, cell, 0, -1);
	south = Cell_Neighbor(u, cell, 0, 1);
	east = Cell_Neighbor(u, cell, 1, 0);
	west = Cell_Neighbor(u, cell, -1, 0);

	if( COLOR_IT(north) ) {
		(*color)++;
		Mark_Reachable_Cells_Alive(u, north, *color);
	}

	if( COLOR_IT(south) ) {
		(*color)++;
		Mark_Reachable_Cells_Alive(u, south, *color);
	}

	if( COLOR_IT(east) ) {
		(*color)++;
		Mark_Reachable_Cells_Alive(u, east, *color);
	}

	if( COLOR_IT(west) ) {
		(*color)++;
		Mark_Reachable_Cells_Alive(u, west, *color);
	}

	nw = Cell_Neighbor(u, cell, -1, -1);
	sw = Cell_Neighbor(u, cell, -1, 1);
	ne = Cell_Neighbor(u, cell, 1, -1);
	se = Cell_Neighbor(u, cell, 1, 1);

	if( COLOR_IT(nw) ) {
		(*color)++;
		Mark_Reachable_Cells_Alive(u, nw, *color);
	}

	if( COLOR_IT(sw) ) {
		(*color)++;
		Mark_Reachable_Cells_Alive(u, sw, *color);
	}

	if( COLOR_IT(ne) ) {
		(*color)++;
		Mark_Reachable_Cells_Alive(u, ne, *color);
	}

	if( COLOR_IT(se) ) {
		(*color)++;
		Mark_Reachable_Cells_Alive(u, se, *color);
	}
}

/*
 * Divide the organism into regions, based on
 * how the organism is divided into peices after
 * dead cells are removed.
 *
 * The largest region lives, smaller regions
 * will be converted to dead matter.
 *
 * If two or more regions "tie" for being the
 * largest, only one of them will remain.
 *
 * RETURNS:
 *		0 = no cell deleted was 'u->current_cell'.
 *		1 = u->current_cell was one of the cells deleted.
 *			u->current_cell was adjusted to the next cell.
 */
#define MAX_REGIONS	1000

int Kill_Dead_Cells(UNIVERSE *u, ORGANISM *o)
{
	int max_found;
	long max;
	int ndead, color, keep_color=0, i;
	CELL *c, *prev, *nxt;
	int counts[ MAX_REGIONS ];
	int cc;
	int energy = 0;

	ASSERT( u != NULL );
	ASSERT( o != NULL );

	cc = 0;

	/*
	 * Set the graph coloring field to 0.
	 * (and count the dead)
	 */
	ndead = 0;
	for(c=o->cells; c; c=c->next) {
		c->color = 0;

		if( Kforth_Machine_Terminated(&c->kfm) ) {
			ndead += 1;
		}
	}

	if( ndead == 0 ) {
		return 0;
	}

	/*
	 * Divide organism into contigous regions.
	 * Each region is assigned a unique non-zero 'color' number.
	 */
	color = 0;
	for(c=o->cells; c; c=c->next) {
		if( Kforth_Machine_Terminated(&c->kfm) ) {
			color_all_cells(u, c, &color);
		}
	}

	/*
	 * Count the number of cells in each region.
	 */
	ASSERT( color < MAX_REGIONS );
	for(i=0; i < color; i++)
		counts[i] = 0;

	for(c=o->cells; c; c=c->next) {
		if( Kforth_Machine_Terminated(&c->kfm) )
			continue;

		/*
		 * every non-dead cell should have an non-zero color value
		 */
		ASSERT( c->color != 0 );

		/*
		 * Increment counter for this region
		 */
		counts[ c->color-1 ] += 1;
	}

	/*
	 * Find the largest region, set variable "keep_color" to that color.
	 */
	max = 0;
	max_found = 0;
	for(i=0; i < color; i++) {
		if( counts[i] > max ) {
			max_found = 1;
			keep_color = i+1;
			max = counts[i];
		}
	}

	/*
	 * Kill all cells that are not in 'keep_color'
	 */
	prev = NULL;
	for(c=o->cells; c; c=nxt) {
		nxt = c->next;

		if( max_found && c->color == keep_color ) {
			prev = c;
			continue;
		}

		if( prev == NULL )
			o->cells = nxt;
		else
			prev->next = nxt;
		/*
		 * Free the cell 'c'
		 */
		o->ncells -= 1;

		energy = o->energy / (o->ncells+1);

		if( energy > 0 ) {
			Grid_SetOrganic(u, c->x, c->y, energy);
			o->energy -= energy;
		} else {
			Grid_Clear(u, c->x, c->y);
		}

		if( c->u_next ) {
			c->u_next->u_prev = c->u_prev;
		}
		if( c->u_prev ) {
			c->u_prev->u_next = c->u_next;
		}
		if( u->cells == c ) {
			u->cells = c->u_next;
		}
		
		if( u->current_cell == c ) {
			u->current_cell = c->u_next;
			cc = 1;
		}

		Cell_delete(c);
	}

	return cc;
}

/*
 * Put organic material at (x, y) with 'energy'
 *
 * If organic is already present at x, y, append
 * energy to it.
 *
 */
static void append_organic(UNIVERSE *u, int x, int y, int energy)
{
	GRID_TYPE type;
	UNIVERSE_GRID *ugp;

	type = Grid_GetPtr(u, x, y, &ugp);

	if( type == GT_ORGANIC ) {
		ugp->u.energy += energy;

	} else if( type == GT_BLANK ) {
		if( energy > 0 ) {
			ugp->type = GT_ORGANIC;
			ugp->u.energy = energy;
		}

	} else if( type == GT_CELL ) {
		if( energy > 0 ) {
			ugp->type = GT_ORGANIC;
			ugp->u.energy = energy;
		} else {
			ugp->type = GT_BLANK;
		}

	} else {
		ASSERT(0);
	}
}

/***********************************************************************
 *
 * Kill organism by deleting all cells
 *
 * If the cell has no cells then use append the organisms energy
 * to location (ex, ey).
 *
 */
int Kill_Organism(UNIVERSE *u, ORGANISM *o, int ex, int ey)
{
	int i, cc;
	CELL *c, *nxt;
	int energy, energy_per_cell, energy_residue;

	cc = 0;		// current cell changed flag

	if( o->ncells > 0 ) {
		energy_per_cell = o->energy / o->ncells;
		energy_residue = o->energy % o->ncells;

		i = 0;
		for(c=o->cells; c; c=nxt) {
			nxt = c->next;

			if( i == 0 )
				energy = energy_per_cell + energy_residue;
			else
				energy = energy_per_cell;

			append_organic(u, c->x, c->y, energy);
 			
			if( c->u_next ) {
				c->u_next->u_prev = c->u_prev;
			}
			if( c->u_prev ) {
				c->u_prev->u_next = c->u_next;
			}
			if( u->cells == c ) {
				u->cells = c->u_next;
			}

			if( u->current_cell == c ) {
				u->current_cell = c->u_next;
				cc = 1;
			}

			Cell_delete(c);
			i++;
		}
		o->cells = NULL;
		ASSERT( o->energy == 0 );
	} else {
		append_organic(u, ex, ey, o->energy);
	}

	return cc;
}

/***********************************************************************
 * Create a new organism.
 * We compile 'program_text' and create the first
 * organism with 1 cell at x, y.
 *
 */
ORGANISM *Organism_Make(
			int x, int y,
			int strain, int energy,
			KFORTH_OPERATIONS *kfops,
			int protected_codeblocks,
			const char *program_text,
			char *errbuf )
{
	ORGANISM *o;
	CELL *c;
	KFORTH_PROGRAM *kfp;

	ASSERT( x >= 0 );
	ASSERT( y >= 0 );
	ASSERT( strain >= 0 && strain < EVOLVE_MAX_STRAINS );
	ASSERT( energy > 0 );
	ASSERT( program_text != NULL );
	ASSERT( kfops != NULL );
	ASSERT( errbuf != NULL );

	kfp = kforth_compile(program_text, kfops, errbuf);
	if( kfp == NULL )
		return NULL;

	kfp->nprotected = protected_codeblocks;

	o = (ORGANISM *) CALLOC(1, sizeof(ORGANISM));
	ASSERT( o != NULL );

	c = (CELL *) CALLOC(1, sizeof(CELL));
	ASSERT( c != NULL );

	/*
	 * Initialize organism
	 */
	o->strain = strain;
	o->energy = energy;
	o->ncells = 1;
	o->sim_count = -1;
	o->cells = c;
	o->next = NULL;
	o->prev = NULL;
	o->program = *kfp;
	FREE(kfp);			// we need to free the root pointer, the other stuff has been copied to o->program

	/*
	 * Initialize cell
	 */
	kforth_machine_init(&c->kfm);
	c->x = x;
	c->y = y;
	c->next = NULL;
	c->organism = o;
	c->u_next = NULL;
	c->u_prev = NULL;

	return o;
}

void Organism_delete(ORGANISM *o)
{
	ASSERT( o != NULL );

	kforth_program_deinit(&o->program);

	FREE(o);
}
