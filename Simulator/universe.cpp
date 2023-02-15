/*
 * Copyright (c) 2022 Ken Stauffer
 */

/***********************************************************************
 * UNIVERSE OPERATIONS
 *
 *
 */
#include "evolve_simulator.h"
#include "evolve_simulator_private.h"

#define GET_GRID(u, x, y)	(   &(u)->grid[(y)*(u)->width + (x)]   )

#if 0
/*
 * Debug routine to compute total enerergy in the universe.
 */
int assert_total_energy(UNIVERSE *u)
{
	UNIVERSE_GRID ugrid;
	GRID_TYPE type;
	ORGANISM *o;
	SPORE *spore;
	int x, y;
	int energy;
	CELL *cell;
	int oenergy, cenergy;

	ASSERT( u != NULL );

	cenergy = 0;
	energy = 0;

	for(x=0; x < u->width; x++) {
		for(y=0; y < u->height; y++) {
			type = Universe_Query(u, x, y, &ugrid);

			if( type == GT_ORGANIC ) {
				energy += ugrid.u.energy;
			}

			if( type == GT_SPORE ) {
				spore = ugrid.u.spore;
				energy += spore->energy;
			}
		}
	}

	for(o=u->organisms; o; o=o->next) {
		energy += o->energy;
	}

	return energy;
}
#endif

GRID_TYPE Grid_GetPtr(UNIVERSE *u, int x, int y, UNIVERSE_GRID **ugrid)
{
	UNIVERSE_GRID *grid;

	ASSERT( u != NULL );
	ASSERT( x >= 0 && x < u->width );
	ASSERT( y >= 0 && y < u->height );
	ASSERT( ugrid != NULL );

	grid = GET_GRID(u, x, y);

	*ugrid = grid;

	return (GRID_TYPE) grid->type;
}

GRID_TYPE Grid_Get(UNIVERSE *u, int x, int y, UNIVERSE_GRID *ugrid)
{
	UNIVERSE_GRID *grid;

	ASSERT( u != NULL );
	ASSERT( x >= 0 && x < u->width );
	ASSERT( y >= 0 && y < u->height );
	ASSERT( ugrid != NULL );

	grid = GET_GRID(u, x, y);

	*ugrid = *grid;

	return (GRID_TYPE) grid->type;
}

void Grid_Clear(UNIVERSE *u, int x, int y)
{
	UNIVERSE_GRID *grid;

	ASSERT( u != NULL );
	ASSERT( x >= 0 && x < u->width );
	ASSERT( y >= 0 && y < u->height );

	grid		= GET_GRID(u, x, y);
	grid->type	= GT_BLANK;
	grid->u.energy	= 0;
}

void Grid_SetBarrier(UNIVERSE *u, int x, int y)
{
	UNIVERSE_GRID *grid;

	ASSERT( u != NULL );
	ASSERT( x >= 0 && x < u->width );
	ASSERT( y >= 0 && y < u->height );

	grid		= GET_GRID(u, x, y);
	grid->type	= GT_BARRIER;
	grid->u.energy	= 0;
}

void Grid_SetOdor(UNIVERSE *u, int x, int y, KFORTH_INTEGER odor)
{
	UNIVERSE_GRID *grid;

	ASSERT( u != NULL );
	ASSERT( x >= 0 && x < u->width );
	ASSERT( y >= 0 && y < u->height );

	grid		= GET_GRID(u, x, y);
	grid->odor	= odor;
}

void Grid_SetCell(UNIVERSE *u, CELL *cell)
{
	int x, y;
	UNIVERSE_GRID *grid;

	ASSERT( u != NULL );
	ASSERT( cell != NULL );
	ASSERT( cell->x >= 0 && cell->x < u->width );
	ASSERT( cell->y >= 0 && cell->y < u->height );

	x = cell->x;
	y = cell->y;

	grid		= GET_GRID(u, x, y);
	grid->type	= GT_CELL;
	grid->u.cell	= cell;
}

void Grid_SetOrganic(UNIVERSE *u, int x, int y, int energy)
{
	UNIVERSE_GRID *grid;

	ASSERT( u != NULL );
	ASSERT( x >= 0 && x < u->width );
	ASSERT( y >= 0 && y < u->height );
	ASSERT( energy >= 0 );

	grid		= GET_GRID(u, x, y);
	grid->type	= GT_ORGANIC;
	grid->u.energy	= energy;
}

void Grid_SetSpore(UNIVERSE *u, int x, int y, SPORE *spore)
{
	UNIVERSE_GRID *grid;

	ASSERT( u != NULL );
	ASSERT( x >= 0 && x < u->width );
	ASSERT( y >= 0 && y < u->height );
	ASSERT( spore != NULL );

	grid		= GET_GRID(u, x, y);
	grid->type	= GT_SPORE;
	grid->u.spore	= spore;
}

//////////////////////////////////////////////////////////////////////
//
// Simulation Options
//
void SimulationOptions_Init(SIMULATION_OPTIONS *so)
{
	memset(so, 0, sizeof(SIMULATION_OPTIONS));
}

//////////////////////////////////////////////////////////////////////
//
// Strain Options
//
void StrainOptions_Init(STRAIN_OPTIONS *strain)
{
	memset(strain, 0, sizeof(STRAIN_OPTIONS));
}

/***********************************************************************
 * Create a blank universe. A blank universe contains
 * no objects on it grid. the grid is defined as width x height.
 *
 * The random seed is initialized to 'seed'.
 *
 */
UNIVERSE *Universe_Make(uint32_t seed, int width, int height)
{
	UNIVERSE *u;
	KFORTH_MUTATE_OPTIONS kfmo;

	ASSERT( width >= EVOLVE_MIN_BOUNDS && width <= EVOLVE_MAX_BOUNDS );
	ASSERT( height >= EVOLVE_MIN_BOUNDS && height <= EVOLVE_MAX_BOUNDS );

	u = (UNIVERSE *) CALLOC(1, sizeof(UNIVERSE));
	ASSERT( u != NULL );

	kforth_mutate_options_defaults(&kfmo);

	u->seed = seed;
	sim_random_init(seed, &u->er);

	u->next_id = 1;

	u->width = width;
	u->height = height;

	u->grid = (UNIVERSE_GRID*) CALLOC(width * height, sizeof(UNIVERSE_GRID));
	ASSERT( u->grid != NULL );

	SimulationOptions_Init(&u->so);

	for(int i=0; i < 8; i++)
	{
		StrainOptions_Init(&u->strop[i]);
		u->kfops[i] = *EvolveOperations();
		kforth_mutate_options_copy2(&kfmo, &u->kfmo[i]);
	}

	u->step = 0;
	u->age = 0;
	u->current_cell = NULL;
	u->cells = NULL;
	u->G0 = 0;
	u->key = 0;
	u->mouse_x = -1;
	u->mouse_y = -1;

	return u;
}

/*
 * Free all the memory associated with an organism.
 */
static void complete_organism_free(ORGANISM *o)
{
	CELL *c, *nxt;

	if( o->cells ) {
		ASSERT( o->ncells > 0 );

		for(c=o->cells; c; c=nxt) {
			nxt = c->next;
			Cell_delete(c);
		}
	}

	Organism_delete(o);
}

/***********************************************************************
 * Free all memory associated with a universe 'u' object.
 *
 */
void Universe_Delete(UNIVERSE *u)
{
	ORGANISM *curr, *nxt;
	UNIVERSE_GRID *ugp;
	int x, y;

	ASSERT( u != NULL );

	for(curr=u->organisms; curr; curr=nxt) {
		nxt = curr->next;
		complete_organism_free(curr);
	}

	for(x=0; x < u->width; x++) {
		for(y=0; y < u->height; y++) {
			ugp = GET_GRID(u, x, y);
			if( ugp->type == GT_SPORE ) {
				Spore_delete(ugp->u.spore);
			}
		}
	}

	FREE(u->grid);
	FREE(u);
}

//
// Use the coordinates (ex,ey) to place energy, if our organism
// now has no cells.
//
static int Kill_Organism_And_Remove_From_Universe(UNIVERSE *u, ORGANISM *o, int ex, int ey)
{
	int cc;

	ASSERT( u != NULL );
	ASSERT( o != NULL );
	ASSERT( ex >= 0 && ex < u->width );
	ASSERT( ey >= 0 && ey < u->height );

	cc = Kill_Organism(u, o, ex, ey);

	if( o->next ) {
		o->next->prev = o->prev;
	}
	if( o->prev ) {
		o->prev->next = o->next;
	}
	if( u->organisms == o ) {
		u->organisms = o->next;
	}

	if( o == u->selected_organism ) {
		u->selected_organism = NULL;
	}

	/*
	 * Free the ORGANISM node (including the program).
	 * All the cells were free'd inside of Organism_Simulate().
	 */
	ASSERT(o->cells == NULL);

	u->ndie += 1;
	u->norganism -= 1;
	u->strpop[o->strain] -= 1;

	Organism_delete(o);

	return cc;
}

void Universe_Simulate(UNIVERSE *u)
{
	CELL_CLIENT_DATA client_data;
	KFORTH_OPERATIONS *kfops;
	CELL *c;
	ORGANISM *o;
	int cc1, cc2;				// flags to indicate the u->current_cell was moved to the next cell because its current reference was removed
	int ex, ey;

	ASSERT( u != NULL );

	u->step += 1;		// step. always increments. if you wish to run simulations to match a number, this is unique. use this
	
	if( u->norganism == 0 ) {
		u->age += 1;
		return;
	}
	
	cc1 = 0;
	cc2 = 0;
	c = u->current_cell;
	ex = c->x;
	ey = c->y;
	o = c->organism;

	//////////////////////////////////////////////////////////////////////
	//
	// Cell Processing
	//	

	if( ! Kforth_Machine_Terminated(&c->kfm) )
	{
		client_data.universe = u;
		client_data.cell = c;
		kfops = &u->kfops[o->strain];

		kforth_machine_execute(kfops, &o->program, &c->kfm, &client_data);
	}

	//////////////////////////////////////////////////////////////////////
	//
	// Organism Processing
	//	

	o->sim_count--;

	ASSERT( o->sim_count >= 0 ); // cannot be negative
		
	if( o->sim_count == 0 ) {
		o->age += 1;
		cc1 = Kill_Dead_Cells(u, o);

		if( o->ncells == 0 || o->energy == 0 ) {
			cc2 = Kill_Organism_And_Remove_From_Universe(u, o, ex, ey);
		} else {
			o->sim_count = o->ncells;
			ASSERT( o->sim_count > 0 );
		}

	}

	//////////////////////////////////////////////////////////////////////
	//
	// Universe Processing
	//	

	if( cc1 == 0 && cc2 == 0 ) {
		u->current_cell = c->u_next;
	}

	if( u->current_cell == NULL ) {
		u->age += 1;
		u->current_cell = u->cells;
	}
}

/***********************************************************************
 * Calculate infomration about the universe
 *
 */
void Universe_Information(UNIVERSE *u, UNIVERSE_INFORMATION *uinfo)
{
	UNIVERSE_GRID ugrid;
	GRID_TYPE type;
	CELL *cell;
	ORGANISM *o;
	SPORE *spore;
	KFORTH_PROGRAM *kfp;
	int x, y;

	ASSERT( u != NULL );
	ASSERT( uinfo != NULL );

	memset(uinfo, 0, sizeof(UNIVERSE_INFORMATION));

	for(x=0; x < u->width; x++) {
		for(y=0; y < u->height; y++) {
			type = Universe_Query(u, x, y, &ugrid);

			if( type == GT_ORGANIC ) {
				uinfo->energy += ugrid.u.energy;
				uinfo->num_organic++;
				uinfo->organic_energy += ugrid.u.energy;
			}

			if( type == GT_SPORE ) {
				spore = ugrid.u.spore;
				kfp = &spore->program;
				uinfo->num_instructions += kforth_program_length(kfp);
				uinfo->energy += spore->energy;
				uinfo->num_spores++;
				uinfo->spore_memory += sizeof(SPORE);
				uinfo->program_memory += kforth_program_size(kfp);
				uinfo->spore_energy += spore->energy;
			}

			if( type == GT_CELL ) {
				cell = ugrid.u.cell;
				uinfo->call_stack_nodes += cell->kfm.csp;
				uinfo->data_stack_nodes += cell->kfm.dsp;
				uinfo->num_cells++;
			}

			uinfo->grid_memory += sizeof(UNIVERSE_GRID);
		}
	}

	for(o=u->organisms; o; o=o->next) {
		kfp = &o->program;
		uinfo->num_instructions += kforth_program_length(kfp);

		uinfo->energy += o->energy;

		uinfo->organism_memory += sizeof(ORGANISM);
		uinfo->organism_memory += o->ncells * (sizeof(CELL) + sizeof(KFORTH_MACHINE));
		uinfo->program_memory += kforth_program_size(kfp);

		if( o->parent1 != o->parent2 )
			uinfo->num_sexual += 1;

		ASSERT( o->strain >= 0 && o->strain < EVOLVE_MAX_STRAINS );

		uinfo->strain_population[o->strain] += 1;

		if( o->oflags & ORGANISM_FLAG_RADIOACTIVE ) {
			uinfo->radioactive_population[o->strain] += 1;
		}
	}

	uinfo->cstack_memory = uinfo->call_stack_nodes * sizeof(KFORTH_LOC);
	uinfo->dstack_memory = uinfo->data_stack_nodes * sizeof(KFORTH_INTEGER);
}

/***********************************************************************
 * Create a barrier (if grid location is blank).
 *
 */
void Universe_SetBarrier(UNIVERSE *u, int x, int y)
{
	UNIVERSE_GRID *g;

	ASSERT( u != NULL );
	ASSERT( x >= 0 && x < u->width );
	ASSERT( y >= 0 && y < u->height );

	g = GET_GRID(u, x, y);

	if( g->type == GT_BLANK ) {
		g->type = GT_BARRIER;
	}
}

/***********************************************************************
 * Clear a barrier (if grid location is a barrier).
 *
 */
void Universe_ClearBarrier(UNIVERSE *u, int x, int y)
{
	UNIVERSE_GRID *g;

	ASSERT( u != NULL );
	ASSERT( x >= 0 && x < u->width );
	ASSERT( y >= 0 && y < u->height );

	g = GET_GRID(u, x, y);

	if( g->type == GT_BARRIER ) {
		g->type = GT_BLANK;
	}

}

/***********************************************************************
 * Query an x,y position in the universe.
 *
 * Returns the grid type, and any associated data is returned in 'ugrid'
 *
 */
GRID_TYPE Universe_Query(UNIVERSE *u, int x, int y, UNIVERSE_GRID *ugrid)
{
	UNIVERSE_GRID *g;

	ASSERT( u != NULL );
	ASSERT( x >= 0 && x < u->width );
	ASSERT( y >= 0 && y < u->height );
	ASSERT( ugrid != NULL );

	g = GET_GRID(u, x, y);

	*ugrid = *g;

	return (GRID_TYPE) g->type;
}

/***********************************************************************
 * These routines keep track of a single organism, and allow
 * the caller to remove it, make a copy, or insert an organism
 * into another universe.
 *
 * If the selected organism is removed from universe, the selection
 * is also removed.
 * 
 *	Universe_SelectOrganism(UNIVERSE *, ORGANISM *);
 *		Remembers an organism. If the organism is not
 *		part of the universe, then this routine ASSERT's
 *
 *	Universe_GetSelection(UNIVERSE *);
 *		Returns the currently selected organism. NULL  is returned
 *		if no selection exists.
 *
 *	Universe_CopyOrganism(UNIVERSE *);
 *		Makes a complete copy of the organism and returns it.
 *		(The copy is detatched from the universe).
 *
 *	Universe_CutOrganism(UNIVERSE *);
 *		Removes the currently selected organism from the universe, and returns
 *		a pointer to the organism.
 *
 *	Universe_PasteOrganism(UNIVERSE *, ORGANISM *);
 *		Inserts the organism into the universe. If the organism is already
 *		in the universe, then nothing is done.
 *	
 *	Universe_FreeOrganism(ORGANISM *);
 *		Free's all the memory associated with the organism.
 *
 */

/***********************************************************************
 * 'o' must be a valid organism inside of 'u'.
 * Flags this organism as selected. It will remain the
 * selected organism until:
 *
 *	1. Selection is cleared
 *	2. Another organism is selected
 *	3. Organism dies during simulation
 *
 */
void Universe_SelectOrganism(UNIVERSE *u, ORGANISM *o)
{
	ORGANISM *curr;

	ASSERT( u != NULL );
	ASSERT( o != NULL );

	/*
	 * Make sure the organism is in our universe.
	 */
	for(curr=u->organisms; curr; curr=curr->next) {
		if( o == curr ) {
			u->selected_organism = o;
			return;
		}
	}
	ASSERT(0);
}

/***********************************************************************
 * Clears the selection (if any)
 *
 */
void Universe_ClearSelectedOrganism(UNIVERSE *u)
{
	ASSERT( u != NULL );

	u->selected_organism = NULL;
}

/***********************************************************************
 * Returns the selected organism (if any).
 *
 */
ORGANISM *Universe_GetSelection(UNIVERSE *u)
{
	ASSERT( u != NULL );

	return u->selected_organism;
}

/***********************************************************************
 * Make a copy of 'osrc'
 */
ORGANISM *Universe_DuplicateOrganism(ORGANISM *osrc)
{
	ORGANISM *odst;
	CELL *cprev, *csrc, *cdst;

	ASSERT( osrc != NULL );

	odst = (ORGANISM *) CALLOC(1, sizeof(ORGANISM) );
	ASSERT( odst != NULL );

	*odst = *osrc;

	odst->next = NULL;
	odst->prev = NULL;

	/*
	 * Copy the program;
	 */
	kforth_program_init(&odst->program);
	kforth_copy2(&osrc->program, &odst->program);

	/*
	 * Copy the cells.
	 */
	cprev = NULL;
	for(csrc=osrc->cells; csrc; csrc=csrc->next) {
		cdst = (CELL *) CALLOC(1, sizeof(CELL) );
		ASSERT(cdst);

		*cdst = *csrc;

		kforth_machine_copy2(&csrc->kfm, &cdst->kfm);
		cdst->next = NULL;
		cdst->u_next = NULL;
		cdst->u_prev = NULL;
		cdst->organism = odst;

		if( cprev == NULL ) {
			odst->cells = cdst;
		} else {
			cprev->next = cdst;
		}

		cprev = cdst;
	}

	return odst;
}

/***********************************************************************
 * Copy Selected Organism and returns it.
 * The returned organism is not attached to any UNIVERSE.
 *
 */
ORGANISM *Universe_CopyOrganism(UNIVERSE *u)
{
	ORGANISM *odst, *osrc;

	ASSERT( u != NULL );
	ASSERT( u->selected_organism != NULL );

	osrc = u->selected_organism;

	odst = Universe_DuplicateOrganism(osrc);

	return odst;
}

/***********************************************************************
 * Removes selected organism from universe and returns the
 * selection.
 *
 */
ORGANISM *Universe_CutOrganism(UNIVERSE *u)
{
	ORGANISM *o;
	CELL *cell, *nxt;

	ASSERT( u != NULL );
	ASSERT( u->selected_organism != NULL );

	/*
	 * Detach the delected organism
	 */
	o = u->selected_organism;

	if( u->organisms == o ) {
		u->organisms = o->next;
	}

	if( o->prev != NULL ) {
		o->prev->next = o->next;
	}

	if( o->next != NULL ) {
		o->next->prev = o->prev;
	}

	o->next = NULL;
	o->prev = NULL;
	u->selected_organism = NULL;
	u->norganism -= 1;
	u->strpop[o->strain] -= 1;

	/*
	 * Iterate over cells in organism and delete detach them from UNIVERSE and GRID
	 */
	for(cell=o->cells; cell; cell=nxt) {
		nxt = cell->next;
		if( cell == u->current_cell )
		{
			u->current_cell = cell->u_next;
			if( u->current_cell == NULL ) {
				u->current_cell = u->cells;
			}
		}

		if( u->cells == cell ) {
			u->cells = cell->u_next;
		}

		if( cell->u_prev != NULL ) {
			cell->u_prev->u_next = cell->u_next;
		}

		if( cell->u_next != NULL ) {
			cell->u_next->u_prev = cell->u_prev;
		}

		Grid_Clear(u, cell->x, cell->y);
		cell->u_next = NULL;
		cell->u_prev = NULL;
	}
	
	return o;
}

/***********************************************************************
 * Insert organism into universe.
 *
 * The organism 'o' will be set as the selected organism.
 *
 */
void Universe_PasteOrganism(UNIVERSE *u, ORGANISM *o)
{
	CELL *cell;
	int x, y;
	int origin_x, origin_y;
	int tmp_x, tmp_y;
	int good_cells;
	GRID_TYPE type;
	UNIVERSE_GRID ugrid;
	int start_paste_x, start_paste_y, paste_dir;

	ASSERT( u != NULL );
	ASSERT( o != NULL );
	ASSERT( o->next == NULL );

	/*
	 * Add the organism.
	 */
	o->id		= u->next_id++;
	
	o->next		= u->organisms;
	o->prev		= NULL;
	if( u->organisms != NULL ) {
		u->organisms->prev = o;
	}
	u->organisms	= o;
	u->norganism	+= 1;
	u->strpop[o->strain] += 1;

	u->selected_organism = o;

	o->sim_count = o->ncells;
	
	for(cell=o->cells; cell; cell=cell->next) {
		if( u->current_cell == NULL ) {
			u->current_cell = cell;
		}
		cell->u_next = u->cells;
		cell->u_prev = NULL;
		if( u->cells != NULL ) {
			u->cells->u_prev = cell;
		}
		u->cells = cell;
	}

#if 0
	good_cells = 0;
	for(cell=o->cells; cell; cell=cell->next) {
		if( cell->x < 0 || cell->x > u->width )
			continue;

		if( cell->y < 0 || cell->y > u->height )
			continue;

		type = Grid_Get(u, cell->x, cell->y, &ugrid);
		if( type == GT_BLANK )
			good_cells += 1;
	}

	/*
	 * All cells are located on blank squares
	 */
	if( good_cells == o->ncells ) {
		for(cell=o->cells; cell; cell=cell->next) {
			Grid_SetCell(u, cell);
		}
		return;
	}
#endif

	/*
	 * Normalize the coordinates in the organism
	 * First cell is (0,0) and all other
	 * cells have their (x,y) coordinates
	 * adjusted relative to that.
	 */
	origin_x = o->cells->x;
	origin_y = o->cells->y;

	for(cell=o->cells; cell; cell=cell->next) {
		cell->x = cell->x - origin_x;
		cell->y = cell->y - origin_y;
	}

	/*
	 * determine where to begin looking for a good spot to paste organism.
	 */	
	if( (origin_x < u->width) && (origin_y < u->height) ) {
		start_paste_x = origin_x;
		start_paste_y = origin_y;
	} else {
		start_paste_x = u->width/2;
		start_paste_y = u->height/2;
	}

	/*
	 * Scan the universe for a spot to insert
	 * the organism.
	 */
	for(paste_dir=0; paste_dir <= 7; paste_dir++) {
		x = start_paste_x;
		y = start_paste_y;

		while( x < u->width && y < u->height ) {

			good_cells = 0;
			for(cell=o->cells; cell; cell=cell->next) {
				tmp_x = x + cell->x;
				tmp_y = y + cell->y;

				if( tmp_x < 0 || tmp_x >= u->width )
					break;

				if( tmp_y < 0 || tmp_y >= u->height )
					break;

				type = Grid_Get(u, tmp_x, tmp_y, &ugrid);

				if( type != GT_BLANK )
					break;

				good_cells += 1;
			}

			if( good_cells == o->ncells ) {
				for(cell=o->cells; cell; cell=cell->next) {
					cell->x = cell->x + x;
					cell->y = cell->y + y;
					Grid_SetCell(u, cell);
				}
				return;
			}

			switch( paste_dir ) {
			case 0:    x += 5; y += 5;   break;
			case 1:    x -= 5; y -= 5;   break;
			case 2:    x += 5; y -= 5;   break;
			case 3:    x -= 5; y += 5;   break;
			case 4:    x += 0; y += 5;   break;
			case 5:    x += 0; y -= 5;   break;
			case 6:    x += 5; y += 0;   break;
			case 7:    x -= 5; y += 0;   break;
			default:
				ASSERT(0);
			}
		}
	}

	/*
	 * If we get here, then we failed to find
	 * a vacant spot to paste. In this case we silently fail.
	 *
	 * KJS TODO: Clean up. remove  organism, etc..
	 */
}

/***********************************************************************
 * Free an organism that was copied, or cut out of a universe.
 *
 */
void Universe_FreeOrganism(ORGANISM *o)
{
	ASSERT( o != NULL );
	ASSERT( o->next == NULL );

	complete_organism_free(o);
}

//////////////////////////////////////////////////////////////////////
/*
 * This contains an organism plus the related strain properties and settings
 */
COPIED_ORGANISM *Universe_CopyOrganismCo(UNIVERSE *u)
{
	COPIED_ORGANISM *co;
	ORGANISM *o;

	ASSERT( u != NULL );
	ASSERT( u->selected_organism != NULL );

	co = (COPIED_ORGANISM*) CALLOC(1, sizeof(COPIED_ORGANISM));
	ASSERT( co != NULL );

	o = Universe_CopyOrganism(u);

	co->o = o;
	co->kfops = u->kfops[o->strain];
	co->strop = u->strop[o->strain];
	co->kfmo = u->kfmo[o->strain];

	return co;
}

COPIED_ORGANISM *Universe_CutOrganismCo(UNIVERSE *u)
{
	COPIED_ORGANISM *co;
	ORGANISM *o;

	ASSERT( u != NULL );
	ASSERT( u->selected_organism != NULL );

	co = (COPIED_ORGANISM*) CALLOC(1, sizeof(COPIED_ORGANISM));

	o = Universe_CutOrganism(u);

	co->o = o;
	co->kfops = u->kfops[o->strain];
	co->strop = u->strop[o->strain];
	co->kfmo = u->kfmo[o->strain];

	return co;
}

//
// Paste has rules:
//		When pasting into a universe, just override the strain with 
//	the strain propertie of 'co'. All existing spores and organisms
// will have their kforth programs remapped to confirm to the new strain
// protections.
//
void Universe_PasteOrganismCo(UNIVERSE *u, COPIED_ORGANISM *co)
{
	ORGANISM *o, *no;
	int strain;

	ASSERT( u != NULL );
	ASSERT( co != NULL );

	// KJS TODO - Assert co->program->nprotected ==  co->kfmo->protected_code_blocks
	ASSERT( co->o->program.nprotected == co->kfmo.protected_codeblocks );

	o = co->o;
	strain = o->strain;

	//
	// For all spores:
	//	- re_map instructions to look like 'co->kfops'
	//	- set spore->program->nprotected to be co->kfmo->protected_codeblocks
	//
	// For all organisms
	//	- re_map instructions to look like 'co->kfops'
	//	- set o->program->nprotected to be co->kfmo->protected_codeblocks
	//
	//	In the event of an error, we're kinda hosed, cause we have already done
	// the work.
	//
	u->strop[strain] = co->strop;
	u->kfmo[strain] = co->kfmo;
	Universe_update_protections(u, strain, &co->kfops, co->kfmo.protected_codeblocks);

	no = Universe_DuplicateOrganism(o);

	Universe_PasteOrganism(u, no);
}

void Universe_FreeOrganismCo(COPIED_ORGANISM *co)
{
	ASSERT( co != NULL );

	Universe_FreeOrganism(co->o);
	FREE(co);
}

void Universe_ClearTracers(UNIVERSE *u)
{
	int x, y;
	UNIVERSE_GRID ugrid;
	GRID_TYPE type;

	ASSERT( u != NULL );

	for(x=0; x < u->width; x++) {
		for(y=0; y < u->height; y++) {
			type = Universe_Query(u, x, y, &ugrid);
			if( type == GT_BLANK )
				continue;

			if( type == GT_SPORE ) {
				Universe_ClearSporeTracer(ugrid.u.spore);

			} else if( type == GT_CELL ) {
				Universe_ClearOrganismTracer(ugrid.u.cell->organism);
			}
		}
	}
}

void Universe_SetSporeTracer(SPORE *spore)
{
	ASSERT( spore != NULL );

	spore->sflags |= SPORE_FLAG_RADIOACTIVE;
}

void Universe_SetOrganismTracer(ORGANISM *organism)
{
	ASSERT( organism != NULL );

	organism->oflags |= ORGANISM_FLAG_RADIOACTIVE;
}

void Universe_ClearSporeTracer(SPORE *spore)
{
	ASSERT( spore != NULL );

	spore->sflags &= ~SPORE_FLAG_RADIOACTIVE;
}

void Universe_ClearOrganismTracer(ORGANISM *organism)
{
	ASSERT( organism != NULL );

	organism->oflags &= ~ORGANISM_FLAG_RADIOACTIVE;
}

/***********************************************************************
 * Make 'kfops' the new instruction table for strain 'strain'.
 *
 * For all organisms and spores in the universe, with strain
 * equal to 'strain', update their genetic program so that
 * opcodes are re-adjusted to confirm to 'kfops'.
 *
 * Requires that kfops is compatible with strain's existing kforth ops table.
 *
 * Compatible defined as: 'kfops' must be a >= super set of exiting table.
 *
 */
void Universe_update_protections(UNIVERSE *u, int strain, KFORTH_OPERATIONS* kfops, int protected_code_blocks)
{
	ASSERT( u != NULL );
	ASSERT( strain >= 0 && strain < 8 );
	ASSERT( kfops != NULL );

	ORGANISM *o;
	SPORE *s;
	int x, y;
	int success;
	GRID_TYPE t;
	UNIVERSE_GRID ugrid;

	/*
	 * do organisms
 	 */
	for(o=u->organisms; o; o=o->next)
	{
		if( o->strain == strain ) {
			o->program.nprotected = protected_code_blocks;
			success = kforth_remap_instructions(kfops, &u->kfops[strain], &o->program);
			ASSERT( success );
		}
	}

	/*
	 * do spores
 	 */
	for(x=0; x < u->width; x++) {
		for(y=0; y < u->height; y++) {
			t = Grid_Get(u, x, y, &ugrid);
			if( t == GT_SPORE ) {
				s = ugrid.u.spore;
				if( s->strain == strain ) {
					s->program.nprotected = protected_code_blocks;
					success = kforth_remap_instructions(kfops, &u->kfops[strain], &s->program);
					ASSERT( success );
				}
			}
		}
	}
	u->kfops[strain] = *kfops;
}
