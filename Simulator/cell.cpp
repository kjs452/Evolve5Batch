/*
 * Copyright (c) 2022 Kenneth Stauffer
 */

/***********************************************************************
 * CELL OPERATIONS:
 * In addition to the normal KFORTH language primitives, each cell
 * can execute one of these EVOLVE specific primitive operations:
 *
 *
 *	KFORTH			FUNCTION
 *	NAME			CALLED			DESCRIPTION
 *	--------------	------------------	-----------------------
 * INTERACTION:
 *	OMOVE			Opcode_OMOVE		move organism
 *	CMOVE			Opcode_CMOVE		move cell
 *	CSHIFT			Opcode_CSHIFT		shift cells along line
 *	ROTATE			Opcode_ROTATE		rotate organism
 *	EAT				Opcode_EAT			eat any edible material adjecent to this cell.
 *	SEND-ENERGY		Opcode_SEND_ENERGY		send energy
 *	MAKE-SPORE		Opcode_MAKE_SPORE	release spore onto grid/fertilize existing spore
 *	MAKE-ORGANIC	Opcode_MAKE_ORGANIC	add organic matter onto grid
 *	MAKE-BARRIER	Opcode_MAKE_BARRIER	add (or clear) barrier to grid
 *	GROW			Opcode_GROW			grow organism by 1 cell
 *	GROW.CB			Opcode_GROW_CB		grow organism by 1 cell, using code block
 *	EXUDE			Opcode_EXUDE		place integer value on the grid
 *	SPAWN			Opcode_SPAWN		spawn a new creature
 *
 * VISION:
 *	LOOK			Opcode_LOOK				return 'what' and distance for something seen
 *	NEAREST			Opcode_NEAREST			what's the nearest thing seen?
 *	FARTHEST		Opcode_FARTHEST			what's the farthest thing seen?
 *	SIZE			Opcode_SIZE				return size and distance for something seen
 *	BIGGEST			Opcode_BIGGEST			what's the biggest thing seen?
 *	SMALLEST		Opcode_SMALLEST			what's the smallest thing seen?
 *	TEMPERATURE		Opcode_TEMPERATURE		return energy and distance for something seen
 *	HOTTEST			Opcode_HOTTEST			what's the most energetic cell seen?
 *	COLDEST			Opcode_COLDEST			what's the least energetic cell seen?
 *	SMELL			Opcode_SMELL			query grid integer value placed by EXUDE
 *
 * COMMUNICATE BETWEEN CELLS:
 *	MOOD		Opcode_MOOD			get mood for cell at (xoffset, yoffset)
 *	MOOD!		Opcode_SET_MOOD		set our mood.
 *	BROADCAST	Opcode_BROADCAST	send message to ALL cells
 *	SEND		Opcode_SEND			send message to cell at (xoffset, yoffset)
 *	RECV		Opcode_RECV			get our message
 *	SHOUT		Opcode_SHOUT		send a message to other organisms
 *	LISTEN		Opcode_LISTEN		listen to mood of another other organism
 *	SAY			Opcode_SAY			send a message to other organisms
 *	READ		Opcode_READ			Read a code block from another cell/spore
 *	WRITE		Opcode_WRITE		Write a code block to another cell/spore
 *
 * QUERY STATE OF ORGANISM:
 *	AGE				Opcode_AGE			Age of organism
 *	NUM-CELLS		Opcode_NUM_CELLS	number of cells in organism
 *	HAS-NEIGHBOR	Opcode_HAS_NEIGHBOR	do we have a neighbor at x y?
 *  NEIGHBORS		Opcode_NEIGHBORS	return neighbors as bitmask
 *	KEY-PRESS		Opcode_KEY_PRESS	get keyboard
 *	MOUSE-POS		Opcode_MOUSE_POS	get mouse coordinates
 *	ENERGY			Opcode_ENERGY			Energy left in cell
 *
 * UNIVERSE
 *  POPULATION		Opcode_POPULATION			# of organisms in total
 *  POPULATION.S	Opcode_POPULATION_STRAIN	# of organisms in my strain
 *	GPS				Opcode_GPS					current position on grid
 *	G0				Opcode_G0					get global
 *	G0!				Opcode_SET_G0				set global
 *	S0				Opcode_S0					set strain-wide global
 *	S0!				Opcode_SET_S0				set strain-wide global
 *
 * MISCELLANEOUS:
 *	DIST			Opcode_DIST			compute distance in the square universe max(abs(x),abs(y))
 *  CHOOSE			Opcode_CHOOSE		choose random number between range
 *	RND				Opcode_RND			return random number between min_int...max_int
 *
 * Normalized coordinates are coordinates that translate to
 * the values +1, 0, -1:
 *
 * BEFORE		NORMALIZED COORDINATES
 * (0, 0)		(0, 0)
 * (123, 0)		(+1, 0)
 * (123, -3939)		(+1, -1)
 * (-121, -3939)	(-1, -1)
 * (11, 22)		(+1, +1)
 *
 * Normalize coordinates are used for operations that act
 * on the grid location immediately surrounding the cell.
 *
 * 	(0,0)	<- no offset
 * 	(0,-1)	<- north location
 * 	(1,-1)	<- north east
 * 	(1,0)	<- east
 * 	(1,1)	<- south east
 * 	(0,1)	<- south
 * 	(-1,1)	<- south west
 * 	(-1,0)	<- west
 * 	(-1,-1)	<- north west
 *
 *	+-------+-------+-------+
 *	|       |       |       |
 *	|(-1,-1)| (0,-1)|(+1,-1)|
 *	|       |       |       |
 *	+-------+-------+-------+
 *	|       |       |       |
 *	| (-1,0)| (0,0) | (+1,0)|
 *	|       |       |       |
 *	+-------+-------+-------+
 *	|       |       |       |
 *	|(-1,+1)| (0,+1)|(+1,+1)|
 *	|       |       |       |
 *	+-------+-------+-------+
 *
 * There are 8 normalized locations surrounding every cell
 * on the grid.
 *
 * Operations that use normalized coordinates:
 *	OMOVE
 *	CMOVE
 *	EAT		(looks at all 8 locations)
 *	MAKE-SPORE
 *	GROW
 *	HAS_NEIGHBOR
 *
 * 2022: The new coordinate system is this:
 *
 *	+-------+-------+-------+
 *	|       |       |       |
 *	|(-1,+1)| (0,+1)|(+1,+1)|
 *	|       |       |       |
 *	+-------+-------+-------+
 *	|       |       |       |
 *	| (-1,0)| (0,0) | (+1,0)|
 *	|       |       |       |
 *	+-------+-------+-------+
 *	|       |       |       |
 *	|(-1,-1)| (0,-1)|(+1,-1)|
 *	|       |       |       |
 *	+-------+-------+-------+
 * 
 * Just swapped the Y-axis. Its more mathamatical and it
 * is consistent with how the rendering on mac works.
 *
 */
#include "evolve_simulator.h"
#include "evolve_simulator_private.h"

/*
 * when a value to be returned back to the KFORTH_MACHINE is too big, it
 * exceeds this value. This is max_int.
 */
#define TOO_BIG		32767

/*
 * VISION 'what' values. We use 1 bit position per what value
 * so these things can be OR'd together as a mask.
 */
#define VISION_MASK		0x7fff
#define VISION_TYPE_NONE	0
#define VISION_TYPE_CELL	1
#define VISION_TYPE_SPORE	2
#define VISION_TYPE_ORGANIC	4
#define VISION_TYPE_BARRIER	8
#define VISION_TYPE_SELF	16
#define VISION_STRAIN_0		32
#define VISION_STRAIN_1		64
#define VISION_STRAIN_2		128
#define VISION_STRAIN_3		256
#define VISION_STRAIN_4		512
#define VISION_STRAIN_5		1024
#define VISION_STRAIN_6		2048
#define VISION_STRAIN_7		4096

/*
 * Convert 'v' to:
 *	1	if v is > 0
 *	0	if v is = 0
 *	-1	if v is < 0
 */
#define NORMALIZE_OFFSET(v)	(  (v<0) ? -1 : ((v>0) ? 1 : 0)  )

static int grid_is_blank(UNIVERSE *u, int x, int y)
{
	GRID_TYPE type;
	UNIVERSE_GRID ugrid;

	ASSERT( u != NULL );

	if( x < 0 || x >= u->width )
		return 0;

	if( y < 0 || y >= u->height )
		return 0;

	type = Grid_Get(u, x, y, &ugrid);

	return (type == GT_BLANK);
}

/*
 * Can the organism move into this location?
 * This is routine is used by the OMOVE primitive to examine all destination
 * squares to see if this square is vacant (or contains one of our own cells).
 */
static int grid_can_moveto(UNIVERSE *u, ORGANISM *o, int x, int y)
{
	GRID_TYPE type;
	UNIVERSE_GRID ugrid;

	ASSERT( u != NULL );
	ASSERT( o != NULL );

	if( x < 0 || x >= u->width )
		return 0;

	if( y < 0 || y >= u->height )
		return 0;

	type = Grid_Get(u, x, y, &ugrid);

	if( type == GT_BLANK )
		return 1;

	if( type == GT_CELL && ugrid.u.cell->organism == o )
		return 1;

	return 0;
}

// when grow_mode == 1
//
// Returns if we can grow to the square x,y. Checks if the squar is
// available or part of this organism. If part of this organism, 'c' is
// set to the cell. Else it is set to NULL.
//
static int grid_can_growto(UNIVERSE *u, ORGANISM *o, int x, int y, CELL **c)
{
	GRID_TYPE type;
	UNIVERSE_GRID ugrid;

	ASSERT( u != NULL );
	ASSERT( o != NULL );

	*c = NULL;

	if( x < 0 || x >= u->width )
		return 0;

	if( y < 0 || y >= u->height )
		return 0;

	type = Grid_Get(u, x, y, &ugrid);

	if( type == GT_BLANK )
		return 1;

	if( type == GT_CELL && ugrid.u.cell->organism == o ) {
		*c = ugrid.u.cell;
		return 1;
	}

	return 0;
}

/*
 * Return the cell located at (x,y) if it is part of organism 'o',
 * otherwise return NULL.
 */
static CELL *grid_has_our_cell(UNIVERSE *u, ORGANISM *o, int x, int y)
{
	GRID_TYPE type;
	UNIVERSE_GRID ugrid;

	ASSERT( u != NULL );
	ASSERT( o != NULL );

	if( x < 0 || x >= u->width )
		return NULL;

	if( y < 0 || y >= u->height )
		return NULL;

	type = Grid_Get(u, x, y, &ugrid);

	if( type == GT_CELL ) {
		if( ugrid.u.cell->organism == o )
			return ugrid.u.cell;
	}

	return NULL;
}

/*
 * This routine looks along a simple line in one of the 8 directions.
 * The coordinates (xoffset, yoffset) form a normalized coordinate to
 * look along.
 *
 * Requires (xoffset, yoffset) are one of the 8 valid direction vectors.
 *
 * Always returns a non-zero 'what' and 'dist'
 *
 * 'dist' is distance, always 1 or more.
 * 'what' will atleat BARRIER, even in a completely empty sim.
 * 'size' - number of cells in any organism encountered
 * 'energy' energy of thing reached
 * 'tot_energy' if an organism , then this is the energy of the organism, else tot_energy will be same as energy
 *
 * look_mode:
 *	bit-0 1		SEE_SELF	0=see thru self,			1=can't see thru self
 *	bit-1 2		XXXXXXX		0=NOT USED					1=NOT USED
 *	bit=2 4		SEE_STRAIN	0=don't set strain bits		1=set strain bits
 *	bit=3 8		INVISIBLE	0=i am not invisible		1=i am invisible
 *
 * Look Mode range  0000 - 1111, 0 - 15
 *
 */
typedef struct {
	int what;
	int dist;
	int size;
	int energy;
	int mood;
	int message;
	int strain;
} LOOK_RESULT;

static void look_along_line(UNIVERSE *u, CELL *c, int look_mode, int xoffset, int yoffset, LOOK_RESULT *res)
{
	UNIVERSE_GRID ugrid;
	GRID_TYPE gt;
	int x, y, distance, strain_bit, invisible;
	ORGANISM *looko;

	ASSERT( u != NULL );
	ASSERT( c != NULL );
	ASSERT( xoffset >= -1 && xoffset <= 1 );
	ASSERT( yoffset >= -1 && yoffset <= 1 );
	ASSERT( !(xoffset == 0 && yoffset == 0) );
	ASSERT( res != NULL );

	x = c->x;
	y = c->y;

	x = x + xoffset;
	y = y + yoffset;
	distance = 1;

	while( (x >= 0) && (x < u->width)
				&& (y >= 0) && (y < u->height) ) {

		gt = Grid_Get(u, x, y, &ugrid);

		if( gt != GT_BLANK ) {
			if( gt == GT_CELL ) {
				looko = ugrid.u.cell->organism;
				invisible = (u->strop[ looko->strain ].look_mode & 8);
				if( !invisible ) {
					if( looko != c->organism ) {
						res->what = VISION_TYPE_CELL;
						if( look_mode & 4 ) {
							strain_bit = 1 << (looko->strain + 5);
							res->what |= strain_bit;
						}
						res->dist = distance;
						res->size = ugrid.u.cell->organism->ncells;
						res->energy = ugrid.u.cell->organism->energy;
						res->mood = ugrid.u.cell->mood;
						res->message = ugrid.u.cell->message;
						res->strain = ugrid.u.cell->organism->strain;
						return;
					} else if( look_mode & 1 ) {
						res->what = VISION_TYPE_CELL | VISION_TYPE_SELF;
						if( look_mode & 4 ) {
							strain_bit = 1 << (looko->strain + 5);
							res->what |= strain_bit;
						}
						res->dist = distance;
						res->size = 0;
						res->energy = 0;
						res->mood = ugrid.u.cell->mood;
						res->message = ugrid.u.cell->message;
						res->strain = ugrid.u.cell->organism->strain;
						return;
					}
				}

			} else if( gt == GT_SPORE ) {
				invisible = (u->strop[ ugrid.u.spore->strain ].look_mode & 8);
				if( !invisible ) {
					res->what = VISION_TYPE_SPORE;
					if( look_mode & 4 ) {
						strain_bit = 1 << (ugrid.u.spore->strain + 7);
						res->what |= strain_bit;
					}
					res->dist = distance;
					res->size = 1;
					res->energy = ugrid.u.spore->energy;
					res->mood = 0;
					res->message = 0;
					res->strain = ugrid.u.spore->strain;
					return;
				}

			} else if( gt == GT_ORGANIC ) {
				res->what = VISION_TYPE_ORGANIC;
				res->dist = distance;
				res->size = 1;
				res->energy = ugrid.u.energy;
				res->mood = 0;
				res->message = 0;
				res->strain = 0;
				return;

			} else if( gt == GT_BARRIER ) {
				res->what = VISION_TYPE_BARRIER;
				res->dist = distance;
				res->size = 0;
				res->energy = 0;
				res->mood = 0;
				res->message = 0;
				res->strain = 0;
				return;

			} else {
				ASSERT(0);
			}
		}

		x = x + xoffset;
		y = y + yoffset;
		distance += 1;
	}

	/*
	 * We hit the hard barrier surrounding the universe.
	 */
	res->what = VISION_TYPE_BARRIER;
	res->dist = distance;
	res->size = 0;
	res->energy = 0;
	res->mood = 0;
	res->message = 0;
	res->strain = 0;
}

/*
 * Fixed stack structure for use in mark_reachable_cells()
 * (approx. 1.2 MB of RAM)
 */

#define MRC_STACK_SIZE (EVOLVE_MAX_BOUNDS * 100)

static struct {
	short x;
	short y;
} mrc_stack[ MRC_STACK_SIZE ];

static int mrc_sp;

static void mrc_empty_stack(void)
{
	mrc_sp = 0;
}

static void mrc_push(int x, int y)
{
	ASSERT( mrc_sp < MRC_STACK_SIZE );

	mrc_stack[mrc_sp].x = (short)x;
	mrc_stack[mrc_sp].y = (short)y;

	mrc_sp++;
}

static bool mrc_pop(int *x, int *y)
{
	ASSERT( x != NULL );
	ASSERT( y != NULL );

	if( mrc_sp > 0 ) {
		mrc_sp--;
		*x = mrc_stack[mrc_sp].x;
		*y = mrc_stack[mrc_sp].y;
		return true;
	} else {
		return false;
	}
}

/*
 * Return the cell located at (x,y) if it is part of organism 'o',
 * and color field is 0, otherwise return NULL.
 *
 * 'alive' restricts the cells visited to only those that are alive
 * (not terminated)
 */
static CELL *mrc_get_cell(UNIVERSE *u, ORGANISM *o, int alive, int x, int y)
{
	GRID_TYPE type;
	UNIVERSE_GRID ugrid;

	ASSERT( u != NULL );
	ASSERT( o != NULL );

	if( x < 0 || x >= u->width )
		return NULL;

	if( y < 0 || y >= u->height )
		return NULL;

	type = Grid_Get(u, x, y, &ugrid);

	if( type == GT_CELL ) {
		if( ugrid.u.cell->organism == o ) {
			if( ugrid.u.cell->color == 0 ) {
				if( !alive || !Kforth_Machine_Terminated(&ugrid.u.cell->kfm) ) {
					return ugrid.u.cell;
				}
			}
		}
	}

	return NULL;
}

#if 0
//
// KJS remove this because diagonal mode is gone
//
/***********************************************************************
 * Visit each cell reachable in the N, S, E, W direction from 'cell'
 * and set color field to 'color'.
 *
 * Assumes that the color field has already been cleared prior
 * to calling this routine.
 *
 * On return, every cell reachable from 'cell' in a N, S, E, W direction will
 * have its color field set to 'color'.
 *
 * 'diagonal' mode increases the search to include the diagonal directions too.
 *
 * Returns the number of cells that were reached (including 'cell')
 *
 * if 'alive' is true, then visit only cells which are alive (not terminated)
 *
 */
static int mark_reachable_cells_straight(UNIVERSE *u, CELL *cell, int color, int alive)
{
	ORGANISM *o;
	CELL *c;
	int x, y, y1;
	int cnt;
	bool span_left, span_right;

	ASSERT( cell != NULL );

	o = cell->organism;

	x = cell->x;
	y = cell->y;

	cnt = 0;

	mrc_empty_stack();
    
	mrc_push(x, y);
    
	while( mrc_pop(&x, &y) ) {
		y1 = y;

		while( mrc_get_cell(u, o, alive, x, y1) ) {
			y1--;
		}

		y1++;

		span_left = false;
		span_right = false;

		while( (c = mrc_get_cell(u, o, alive, x, y1)) ) {

			ASSERT( c != NULL && c->organism == o );

			c->color = color;
			cnt += 1;

			if( !span_left && mrc_get_cell(u, o, alive, x-1, y1) ) {
				mrc_push(x-1, y1);
				span_left = true;

			} else if( span_left && !mrc_get_cell(u, o, alive, x-1, y1) ) {
				span_left = false;
			}

			if( !span_right && mrc_get_cell(u, o, alive, x+1, y1) ) {
				mrc_push(x+1, y1);
				span_right = true;

			} else if( span_right && !mrc_get_cell(u, o, alive, x+1, y1) ) {
				span_right = false;
			}
			y1++;
		}
	}

	return cnt;
}
#endif

/***********************************************************************
 * This version makes sure the organism is connected in any of 
 * the 8 directions (diagonal and straight)
 *
 * Visit each cell reachable in all the directions from 'cell'
 * and set color field to 'color'.
 *
 * Assumes that the color field has already been cleared prior
 * to calling this routine.
 *
 * On return, every cell reachable from 'cell' in any direction
 * will have its color field set to 'color'.
 *
 * Returns the number of cells that were reached (including 'cell')
 *
 * if 'alive' is true, then only visit alive cells (not terminated)
 *
 */
static int mark_reachable_cells_diagonal(UNIVERSE *u, CELL *cell, int color, int alive)
{
	ORGANISM *o;
	int x, y;
	int cnt;
	CELL *n, *s, *e, *w;
	CELL *ne, *se, *nw, *sw;

	ASSERT( cell != NULL );

	o = cell->organism;
	x = cell->x;
	y = cell->y;

	mrc_empty_stack();
	mrc_push(x, y);

	cnt = 1;
	cell->color = color;
	while( mrc_pop(&x, &y) ) {

		n  = mrc_get_cell(u, o, alive, x+0, y-1);
		s  = mrc_get_cell(u, o, alive, x+0, y+1);
		e  = mrc_get_cell(u, o, alive, x+1, y+0);
		w  = mrc_get_cell(u, o, alive, x-1, y+0);
		ne = mrc_get_cell(u, o, alive, x+1, y-1);
		se = mrc_get_cell(u, o, alive, x+1, y+1);
		nw = mrc_get_cell(u, o, alive, x-1, y-1);
		sw = mrc_get_cell(u, o, alive, x-1, y+1);

		if( n )  {  n->color = color; cnt++; mrc_push( n->x,  n->y); }
		if( s )  {  s->color = color; cnt++; mrc_push( s->x,  s->y); }
		if( e )  {  e->color = color; cnt++; mrc_push( e->x,  e->y); }
		if( w )  {  w->color = color; cnt++; mrc_push( w->x,  w->y); }
		if( ne ) { ne->color = color; cnt++; mrc_push(ne->x, ne->y); }
		if( se ) { se->color = color; cnt++; mrc_push(se->x, se->y); }
		if( nw ) { nw->color = color; cnt++; mrc_push(nw->x, nw->y); }
		if( sw ) { sw->color = color; cnt++; mrc_push(sw->x, sw->y); }
	}

	return cnt;
}

int Mark_Reachable_Cells(UNIVERSE *u, CELL *cell, int color)
{
	return mark_reachable_cells_diagonal(u, cell, color, 0);
}

int Mark_Reachable_Cells_Alive(UNIVERSE *u, CELL *cell, int color)
{
	return mark_reachable_cells_diagonal(u, cell, color, 1);
}

/*
 * Create a spore in the grid at (x,y).
 */
static void create_spore(UNIVERSE *u, ORGANISM *o, int x, int y, int energy)
{
	SPORE *spore;

	ASSERT( u != NULL );
	ASSERT( o != NULL );

	spore = Spore_make(&o->program, energy, o->id, o->strain);
	
	if( o->oflags & ORGANISM_FLAG_RADIOACTIVE ) {
		spore->sflags |= SPORE_FLAG_RADIOACTIVE;
	}

	Grid_SetSpore(u, x, y, spore);
}

/*
 * Interrupt the cell 'cell', if intflags are set.
 *
 * Returns:
 *	0 - success - target cell was interrupted
 *  1 - intflags  are 0
 *	2 - cell is already running in trap handler
 *	3 - cell is terminated
 *	4 - cell doesn't have that code block number
 *	5 - cell doesn't have any call stack elements left
 *	6 - cell doesn't have at least 2 data stack element avail
 *
 */
static int interrupt(CELL *cell, int intflags)
{
	KFORTH_LOC loc;

	ASSERT( cell != NULL );
	ASSERT( intflags >= 0 && intflags <= 7 );

	if( intflags == 0 )
	{
		return 1;
	}

	if( cell->kfm.loc.cb == intflags ) {
		return 2;
	}

	if( Kforth_Machine_Terminated(&cell->kfm) )
	{
		return 3;
	}

	if( intflags > cell->organism->program.nblocks )
	{
		return 4;
	}

	if( cell->kfm.csp >= KF_MAX_CALL ) {
		return 5;
	}

	if( cell->kfm.dsp >= KF_MAX_DATA-1 ) {
		return 6;
	}

	// save current loc (cb, pc) on call stack
	loc.cb = cell->kfm.loc.cb;
	loc.pc = cell->kfm.loc.pc - 1;			// the return from code block logic increments the pc.
	Kforth_Call_Stack_Push(&cell->kfm, loc);

	// set location to (intflags, 0)
	cell->kfm.loc.cb = intflags;
	cell->kfm.loc.pc = 0;

	return 0;
}

/*
 * Organism 'o' is wanting to eat anything
 * in grid location (x,y)
 * 'self_eat' means the organism can eat itself, teriminating the cell
 *
 *	eat_mode is a bit mask:
 *		bit-0 1		SELF_EAT		0=can't eat self			1=can eat self
 *		bit-1 2		STRAIN_EAT		0=can eat other strains		1=cannot eat other strains
 *		bit-2 4		S.UNEATABLE		0=i can eat my own strain	1=i cannot eat my own strain
 *		bit-3 8		UNEATABLE		0=i can be eaten			1=i am un-eatable
 *		bit-4 16    REMAINDER		0=i get to eat any energy remeainder 1=i don't get the energy remainder
 *		bit-5 32					0=off						1=use make-spore amount
 *		bit-6 64					0=off						1=use GROW amount
 *		bit-7 128					0=off						1=use cell energy / 2 + remainder
 *		bit-8 256					0=off						1=take as much energy as my cell has
 *		bit-9 512					0=terminate cell			1=don't terminate cell
 *		bit-10 1024					0=interrupt off				1=configure interrupt handler to be trap bit-0
 *		bit-11 2048					0=interrupt off				1=configure interrupt handler to be trap bit-1
 *		bit-12 4096					0=interrupt off				1=configure interrupt handler to be trap bit-2
 *
 * Eat_Mode Range:  0 - 8191
 *
 */
static int eat(UNIVERSE *u, ORGANISM *o, CELL *cell, int eat_mode, int x, int y)
{
	GRID_TYPE type;
	UNIVERSE_GRID ugrid;
	SPORE *spore;
	CELL *eatc;
	ORGANISM *eato;
	int energy;
	int intflags;

	ASSERT( u != NULL );
	ASSERT( o != NULL );

	if( x < 0 || x >= u->width )
		return 0;

	if( y < 0 || y >= u->height )
		return 0;

	type = Grid_Get(u, x, y, &ugrid);

	if( type == GT_ORGANIC ) {
		energy = ugrid.u.energy;

		o->energy += energy;

		Grid_Clear(u, x, y);
		return energy;

	} else if( type == GT_SPORE ) {
		spore = ugrid.u.spore;

		if( u->strop[spore->strain].eat_mode & 8 ) {
			// this spore is from a strain which is marked 'un-eatable'
			return 0;
		}

		if( spore->strain == o->strain ) {
			if( eat_mode & 4 ) {
				// i cannot eat my own strain
				return 0;
			}
		} else {
			if( eat_mode & 2 ) {
				// cannot eat other strains
				return 0;
			}
		}

		energy = spore->energy;

		o->energy += energy;

		Grid_Clear(u, x, y);
		Spore_delete(spore);
		return energy;

	} else if( type == GT_CELL  ) {
		/*
		 * Energy transfer between living organisms
		 *
		 * 1. Make sure cell isn't one of ours
		 * 2. Make sure cell isn't already dead
		 * 3. Set cell kfm to terminated (this kills the cell)
		 * 4. convert cell stack nodes to energy and add to us.
		 * 5. add 1/n amount of energy from eaten organism.
		 *
		 * Self_Eat means we eat ourselves causing harm (killing the cell)
		 * No energy is transferred.
		 *
		 */
		eatc = ugrid.u.cell;
		eato = eatc->organism;

		if( u->strop[eato->strain].eat_mode & 8 ) {
			// the cell to be eaten is from a strain which is marked 'un-eatable'
			return 0;
		}

		if( eato->strain == o->strain ) {
			if( eat_mode & 4 ) {
				// i cannot eat my own strain
				return 0;
			}
		} else {
			if( eat_mode & 2 ) {
				// i cannot eat other strains
				return 0;
			}
		}

		if( eato == o ) {
			if( (eat_mode & 1) == 0 )
			{
				// i cannot eat myself
				return 0;
			}
		}

		if( Kforth_Machine_Terminated(&eatc->kfm) ) {
			// the cell is already dead, cannot get energy from it
			return 0;
		}

		ASSERT( eato->ncells > 0 );

		if( eat_mode & 16 ) {
			energy = (eato->energy / eato->ncells);
		} else if( eat_mode & 32 ) {
			energy = u->strop[o->strain].make_spore_mode;
			energy = ( energy > eato->energy ) ? eato->energy : energy;
		} else if( eat_mode & 64 ) {
			energy = u->strop[o->strain].grow_mode;
			energy = ( energy > eato->energy ) ? eato->energy : energy;
		} else if( eat_mode & 128 ) {
			energy = (eato->energy / eato->ncells);
			energy = energy / 2 + energy % 2;
		} else if( eat_mode & 256 ) {
			energy = (eato->energy / eato->ncells);
			energy = energy / 3 + energy % 3;
		} else {
			energy = (eato->energy / eato->ncells) + (eato->energy % eato->ncells);
		}

		ASSERT(energy <= eato->energy);
		eato->energy -= energy;
		o->energy += energy;

		if( eat_mode & 512 ) {
			if( eato->energy / eato->ncells == 0 ) {
				Kforth_Machine_Terminate(&eatc->kfm);
			} else {
				// interrupt
				intflags = (u->strop[ eato->strain ].eat_mode >> 10) & 7;
				interrupt(eatc, intflags);
			}
		} else {
			Kforth_Machine_Terminate(&eatc->kfm);
		}

		return energy;

	} else {
		return 0;
	}
}

/*
 * Shift cells along a vector
 * Does the mechanics for the CSHIFT instruction, but is also used by GROW/CMOVE.
 *
 *	return a non-zero number, the number of cells shifted if the operation was successful
 *	return 0 if the operation could not be performed
 *
 * if 'grow' is true (the Gap Will Be Filled) so check the connectivity differenty
 */
static int cshift(UNIVERSE *u, ORGANISM *o, int grow, CELL *cell, int xoffset, int yoffset)
{
	CELL *c, *prev, *t;
	int x, y;
	int cnt;
	CELL *cprev;				// variable used to stop backward walk.
	int cprev_x, cprev_y;		// variables used to stop backward walk
	int cpast_x, cpast_y;		// variables used to stop forward walk
	int count;					// number of cells to be shifted

	ASSERT( u != NULL );
	ASSERT( o != NULL );
	ASSERT( cell != NULL );
	ASSERT( ! (yoffset == 0 && xoffset == 0) );

	cprev_x = cell->x - xoffset;
	cprev_y = cell->y - yoffset;
	cprev = grid_has_our_cell(u, o, cprev_x, cprev_y);

	/*
	 * Pass 1: WALK FORWARD. Walk line forward from self to see if we can shift
	 */
	count = 0;
	prev = NULL;
	c = cell;
	x = c->x;
	y = c->y;
	while( c ) {
		prev = c;
		x += xoffset;
		y += yoffset;
		c = grid_has_our_cell(u, o, x, y);
		count += 1;
	}

	if( ! grid_is_blank(u, x, y) ) {
		return 0;
	}

	cpast_x = x + xoffset;	// remember ending square to stop forward pass 4
	cpast_y = y + yoffset;

	/*
	 * Pass 2: WALK BACKWARDS. Move cells
	 */
	c = prev;
	x -= xoffset;
	y -= yoffset;
	while( !(x == cprev_x && y == cprev_y) ) {
		Grid_Clear(u, c->x, c->y);
		c->x += xoffset;
		c->y += yoffset;
		Grid_SetCell(u, c);

		prev = c;
		x -= xoffset;
		y -= yoffset;
		c = grid_has_our_cell(u, o, x, y);
	}

	/*
	 * Pass 3: check connectivity
	 */
	for(t=o->cells; t; t=t->next) {
		t->color = 0;
	}

	cnt = Mark_Reachable_Cells(u, prev, 1);

	if( grow && c )
	{
		// Interesting fact c == the cell running GROW
		cnt += Mark_Reachable_Cells(u, c, 1);
	}

	if( cnt == o->ncells )
	{
		return count;
	}

	/*
	 * Pass 4: WALK FORWARDS. undo shift
	 */
	c = cell;
	x = c->x;
	y = c->y;
	while( !(x == cpast_x && y == cpast_y) ) {
		Grid_Clear(u, c->x, c->y);
		c->x -= xoffset;
		c->y -= yoffset;
		Grid_SetCell(u, c);

		x += xoffset;
		y += yoffset;
		c = grid_has_our_cell(u, o, x, y);
	}

	return 0;
}

/***********************************************************************
 * KFORTH NAME:		OMOVE
 * STACK BEHAVIOR:	(x y -- r)
 *
 * Move organism in the direction specified by (x, y).
 *
 * If the move is successful r=number of cells moved, else r=0.
 *
 * If data stack doesn't have 2 elements, then do nothing.
 *
 * RULES:
 *	1. Every desintation location must be blank. or
 *	2. destination location is one of our own cells
 *
 */
static void Opcode_OMOVE(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	ORGANISM *o;
	CELL *cell, *ccurr;
	UNIVERSE *u;
	KFORTH_INTEGER value;
	int x, y, xoffset, yoffset;
	int failed;
	CELL_CLIENT_DATA *cd;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	o = cell->organism;
	u = cd->universe;

	value = Kforth_Data_Stack_Pop(kfm);
	yoffset = NORMALIZE_OFFSET(value);

	value = Kforth_Data_Stack_Pop(kfm);
	xoffset = NORMALIZE_OFFSET(value);

	/*
	 * OMOVE of (0,0) always fails, helps look logic
	 * to evolve.
	 */
	if( xoffset == 0 && yoffset == 0 ) {
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	failed = 0;
	for(ccurr=o->cells; ccurr; ccurr=ccurr->next) {
		x = ccurr->x + xoffset;
		y = ccurr->y + yoffset;

		if( ! grid_can_moveto(u, o, x, y) ) {
			failed = 1;
			break;
		}
	}

	if( failed ) {
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	for(ccurr=o->cells; ccurr; ccurr=ccurr->next) {
		Grid_Clear(u, ccurr->x, ccurr->y);
	}

	for(ccurr=o->cells; ccurr; ccurr=ccurr->next) {
		x = ccurr->x + xoffset;
		y = ccurr->y + yoffset;

		ccurr->x = x;
		ccurr->y = y;

		Grid_SetCell(u, ccurr);
	}

	value = (o->ncells < TOO_BIG) ? o->ncells : TOO_BIG;

	Kforth_Data_Stack_Push(kfm, value);
}

/***********************************************************************
 * KFORTH NAME:		CMOVE
 * STACK BEHAVIOR:	(x y -- r)
 *
 * Move cell relative to organism. (x, y) specifies
 * a direction to move relative to current location.
 *
 * If successful r=1, else r=0.
 *
 * RULES:
 *
 * CMOVE_MODE == 0 RULES:
 *	1. The destination location must be vacant
 *	2. After the move, the all cells must be connected in the 8 directions.
 *
 * CMOVE_MODE == 1 RULES:
 *	NOT DEFINED. I planned on something, but no more. See CSHIFT instruction.
 *
 */
static void Opcode_CMOVE(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	CELL *cell, *c;
	ORGANISM *o;
	UNIVERSE *u;
	int x, y, xoffset, yoffset;
	int save_x, save_y;
	KFORTH_INTEGER value;
	int cnt;
	CELL_CLIENT_DATA *cd;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	o = cell->organism;
	u = cd->universe;

	value = Kforth_Data_Stack_Pop(kfm);
	yoffset = NORMALIZE_OFFSET(value);

	value = Kforth_Data_Stack_Pop(kfm);
	xoffset = NORMALIZE_OFFSET(value);

	x = cell->x + xoffset;
	y = cell->y + yoffset;

	if( ! grid_is_blank(u, x, y) ) {
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	/*
	 * Move cell to new location
	 */
	save_x = cell->x;
	save_y = cell->y;

	Grid_Clear(u, cell->x, cell->y);

	cell->x = x;
	cell->y = y;
	Grid_SetCell(u, cell);

	/*
	 * Make sure all cells in organism have at least 1 neighbor in the
	 * allowable connected directions.
	 */
	for(c=o->cells; c; c=c->next) {
		c->color = 0;
	}

	cnt = Mark_Reachable_Cells(u, o->cells, 1);

	if( cnt != o->ncells ) {
		/*
		 * connectivity requirement is not true, so undo the move,
		 * and push 0 on data stack.
		 */
		Grid_Clear(u, cell->x, cell->y);
		cell->x = save_x;
		cell->y = save_y;
		Grid_SetCell(u, cell);
		Kforth_Data_Stack_Push(kfm, 0);
	} else {
		/* success! */
		Kforth_Data_Stack_Push(kfm, 1);
	}
}

/***********************************************************************
 * KFORTH NAME:		CSHIFT
 * STACK BEHAVIOR:	(x y -- r)
 *
 * Shift cells along the line defined by this cell and the (x,y) direction vector.
 *
 * If successful r=number of cells shifted, else r=0.
 *
 * CSHIFT_MODE == 0 RULES:
 *	1. test each cell along line to see if room exists.
 *	2. Shift the cells
 *	3. Check the connectivity
 *	4. If connectivity is violated, undo the shift.
 *
 */
static void Opcode_CSHIFT(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	CELL *cell;
	ORGANISM *o;
	UNIVERSE *u;
	int xoffset, yoffset;
	KFORTH_INTEGER value;
	int num_cells;
	CELL_CLIENT_DATA *cd;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	o = cell->organism;
	u = cd->universe;

	value = Kforth_Data_Stack_Pop(kfm);
	yoffset = NORMALIZE_OFFSET(value);

	value = Kforth_Data_Stack_Pop(kfm);
	xoffset = NORMALIZE_OFFSET(value);

	if( yoffset == 0 && xoffset == 0 ) {
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	num_cells = cshift(u, o, 0, cell, xoffset, yoffset);

	value = (num_cells < TOO_BIG) ? num_cells : TOO_BIG;
	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * Compute new (x, y) coordinate for the input 'x' and 'y'
 * 
 * The rotation amount is given by 'n' (which is a number from -3 to 3).
 *
 * The origin (x, y) is given by 'origin_x' and 'origin_y'.
 *
 */
static void rotate(int n, int origin_x, int origin_y, int x, int y, int *newx, int *newy)
{
	int xoffset, yoffset;
	int new_xoffset = 0, new_yoffset = 0;

	ASSERT( newx != NULL );
	ASSERT( newy != NULL );

	xoffset = x - origin_x;
	yoffset = y - origin_y;

	switch(n) {
	case 0:
		new_xoffset = xoffset;
		new_yoffset = yoffset;
		break;

	case  1:
	case -3:
		new_xoffset = yoffset *  1;
		new_yoffset = xoffset * -1;
		break;

	case  2:
	case -2:
		new_xoffset = xoffset *  1;
		new_yoffset = yoffset *  1;
		break;

	case  3:
	case -1:
		new_xoffset = yoffset * -1;
		new_yoffset = xoffset *  1;
		break;

	default:
		ASSERT(0);
		break;
	}

	*newx = origin_x + new_xoffset;
	*newy = origin_y + new_yoffset;
}

#define ABS(x)		((x<0) ? -x : x)
#define MAX(x,y)	((x>y) ?  x : y)

static void rotateCCW(int origin_x, int origin_y, int px, int py, int *newx, int *newy)
{
	int x, y, shell;

	x = px - origin_x;
	y = py - origin_y;

	shell = MAX( ABS(x), ABS(y) );

	if( ABS(x) == shell ) {
		if( x < 0 ) {
			y = y - shell;
			if( y < -shell ) {
				x = x + (-shell - y);
				y = -shell;
			}
		} else {
			y = y + shell;
			if( y > shell ) {
				x = x - (y - shell);
				y = shell;
			}
		}
	} else {
		if( y < 0 ) {
			x = x + shell;
			if( x > shell ) {
				y = y + (x - shell);
				x = shell;
			}
		} else {
			x = x - shell;
			if( x < -shell ) {
				y = y - (-shell - x);
				x = -shell;
			}
		}
	}

	*newx = origin_x + x;
	*newy = origin_y + y;
}

static void rotateCW(int origin_x, int origin_y, int px, int py, int *newx, int *newy)
{
	int x, y, shell;

	x = px - origin_x;
	y = py - origin_y;

	shell = MAX( ABS(x), ABS(y) );

	if( ABS(x) == shell ) {
		if( x < 0 ) {
			y = y + shell;
			if( y > shell ) {
				x = x + (y - shell);
				y = shell;
			}
		} else {
			y = y - shell;
			if( y < -shell ) {
				x = x - (-shell - y);
				y = -shell;
			}
		}
	} else {
		if( y < 0 ) {
			x = x - shell;
			if( x < -shell ) {
				y = y + (-shell - x);
				x = -shell;
			}
		} else {
			x = x + shell;
			if( x > shell ) {
				y = y - (x - shell);
				x = shell;
			}
		}
	}

	*newx = origin_x + x;
	*newy = origin_y + y;
}

/*
 * ROTATE coordinate by +/- 45 degrees. If dir is -1, -45 degree rotation
 * is performed. Else 45 degree.
 *
 * The origin (x, y) is given by 'origin_x' and 'origin_y'.
 *
 * Compute new (x, y) coordinate for the input 'x' and 'y'
 *
 */
static void rotate45(int dir, int origin_x, int origin_y, int x, int y, int *newx, int *newy)
{
	ASSERT( newx != NULL );
	ASSERT( newy != NULL );
	ASSERT( dir == -1 || dir == 1 );

	if( dir == -1 )
		rotateCCW(origin_x, origin_y, x, y, newx, newy);
	else
		rotateCW(origin_x, origin_y, x, y, newx, newy);
}

static void bounding_box(ORGANISM *o, int *left, int *right, int *top, int *bottom)
{
	CELL *c;

	ASSERT( o );
	ASSERT( o->cells );

	for(c=o->cells; c; c=c->next)
	{
		if( c->x < *left   || c == o->cells)	*left = c->x;
		if( c->x > *right  || c == o->cells)	*right = c->x;
		if( c->y < *top    || c == o->cells)	*top = c->y;
		if( c->y > *bottom || c == o->cells)	*bottom = c->y;
	}
}

/***********************************************************************
 * KFORTH NAME:		ROTATE
 * STACK BEHAVIOR:	( n -- r )
 *
 * Rotate organism. return number of cells rotated
 * n > 0		roate clockwise
 * n < 0		rotate counter-clockwise
 * n == 0		no rotation
 *
 *
 * ROTATE_MODE == 0 RULES:
 *		1. Any cell can rotate as origin
 *
 * ROTATE_MODE == 1 RULES:
 *		1. center point of organism is used to form the origin
 *
 * ROATE_MODE bit-2: restrict to 90 degree rotate
 *
 */
static void Opcode_ROTATE(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	CELL *cell, *c;
	ORGANISM *o;
	UNIVERSE *u;
	KFORTH_INTEGER value;
	int newx, newy;
	int failed, n, cnt;
	CELL_CLIENT_DATA *cd;
	int left, right, top, bottom;
	int rotate_mode;
	int rx, ry;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	o = cell->organism;
	u = cd->universe;

	rotate_mode = u->strop[o->strain].rotate_mode;

	value = Kforth_Data_Stack_Pop(kfm);
	n = NORMALIZE_OFFSET(value);

	if( n == 0 ) {
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	if( (rotate_mode & 1) != 0 )
	{
		bounding_box(o, &left, &right, &top, &bottom);

		rx = (right + left)/2;
		ry = (bottom + top)/2;
	} else {
		rx = cell->x;
		ry = cell->y;
	}

	/*
	 * translate all cells to new location, and
	 * make sure they are vacant.
	 */
	failed = 0;
	for(c=o->cells; c; c=c->next) {
		if( (rotate_mode & 2) == 0 )
			rotate45(n, rx, ry, c->x, c->y, &newx, &newy);
		else
			rotate(n, rx, ry, c->x, c->y, &newx, &newy);

		if( ! grid_can_moveto(u, o, newx, newy)  ) {
			failed = 1;
			break;
		}
	}

	if( failed ) {
		Kforth_Data_Stack_Push(kfm, 0);
		return;		
	}

	/*
 	 * Clear all cells at current location
	 */
	for(c=o->cells; c; c=c->next) {
		Grid_Clear(u, c->x, c->y);
	}

	/*
	 * The rotate operation has space to move the cells, so
	 * repeat loop above, and move each cell.
	 */
	for(c=o->cells; c; c=c->next) {
		if( (rotate_mode & 2) == 0 )
			rotate45(n, rx, ry, c->x, c->y, &newx, &newy);
		else
			rotate(n, rx, ry, c->x, c->y, &newx, &newy);

		c->x = newx;
		c->y = newy;

		Grid_SetCell(u, c);
	}

	// scan for connectivity
	for(c=o->cells; c; c=c->next) {
		c->color = 0;
	}

	cnt = Mark_Reachable_Cells(u, cell, 1);
	ASSERT( cnt != 0 );

	if( cnt == o->ncells ) {
		value = ( cnt < TOO_BIG ) ? cnt : TOO_BIG;
		Kforth_Data_Stack_Push(kfm, value);
		return;
	}

	// undo
	// looks like the organism isn't connected properly. undo the rotate

	/*
 	 * Clear all cells at current location
	 */
	for(c=o->cells; c; c=c->next) {
		Grid_Clear(u, c->x, c->y);
	}

	for(c=o->cells; c; c=c->next) {
		if( (rotate_mode & 2) == 0 )
			rotate45(-n, rx, ry, c->x, c->y, &newx, &newy);
		else
			rotate(-n, rx, ry, c->x, c->y, &newx, &newy);

		c->x = newx;
		c->y = newy;

		Grid_SetCell(u, c);
	}

	Kforth_Data_Stack_Push(kfm, 0);
}

/***********************************************************************
 * KFORTH NAME:		EAT
 * STACK BEHAVIOR:	( x y -- e )
 *
 * Attempt to eat something at the normalized (x,y) offset from this cell.
 * This will eat:
 *	* Spores
 *	* Organic material
 *	* Other Organisms
 *
 * 'e' will equal the amount of energy we acquired.
 *
 * If nothing was eaten, e=0.
 *
 */
static void Opcode_EAT(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	CELL *cell;
	ORGANISM *o;
	UNIVERSE *u;
	int x, y, xoffset, yoffset, energy;
	KFORTH_INTEGER value;
	CELL_CLIENT_DATA *cd;
	int eat_mode;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	o = cell->organism;
	u = cd->universe;

	value = Kforth_Data_Stack_Pop(kfm);
	yoffset = NORMALIZE_OFFSET(value);

	value = Kforth_Data_Stack_Pop(kfm);
	xoffset = NORMALIZE_OFFSET(value);

	x = cell->x;
	y = cell->y;

	eat_mode = u->strop[o->strain].eat_mode;

	energy = eat(u, o, cell, eat_mode, x+xoffset, y+yoffset);

	Kforth_Data_Stack_Push(kfm, energy);
}

/***********************************************************************
 * (NEW VERSION 4.7 -- 2nd spore creation can have 0 energy)
 *
 * KFORTH NAME:		MAKE-SPORE
 * STACK BEHAVIOR:	(x y e -- s)
 *
 * Create spore xoffset, yoffset relative to
 * this cell. The spore will be created if enough
 * energy exists.
 *
 * If a spore is already there, then we fertilize the
 * spore an create a new organism.
 *
 * Rules:
 *	1. (x,y) offset location must be vacant or another spore.
 *	2. Our energy must be equal to or more than 'e'
 *	3. New spore will possses 'e' energy
 *	4. We will lose 'e' energy.
 *	5. Must be 3 elements on the data stack
 *	6. If (x,y) contains a spore, we fertilize it and create
 *	   a new organism.
 *
 *	7. s=0 if we failed to make a spore. s=1 is we created a spore. s=-1 if
 *	   we fertilized an existing spore.
 *	8. e must be 1 or more.
 *	9. If a spore exists, it must be a spore matching our strain. If not then,
 *		s=0.
 *
 * This version allows the 2nd spore (the one that fertilizes the 1st spore) to
 * have 0 energy. This will encourage sexual reproduction.
 *
 */
static void Opcode_MAKE_SPORE(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	ORGANISM *o;
	CELL *cell;
	UNIVERSE *u;
	GRID_TYPE type;
	UNIVERSE_GRID ugrid;
	KFORTH_INTEGER value;
	int x, y, xoffset, yoffset, energy;
	CELL_CLIENT_DATA *cd;
	int make_spore_mode;
	int make_spore_energy;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	o = cell->organism;
	u = cd->universe;

	make_spore_mode = u->strop[o->strain].make_spore_mode;
	make_spore_energy = u->strop[o->strain].make_spore_energy;

	value = Kforth_Data_Stack_Pop(kfm);
	energy = (int) value;

	value = Kforth_Data_Stack_Pop(kfm);
	yoffset = NORMALIZE_OFFSET(value);

	value = Kforth_Data_Stack_Pop(kfm);
	xoffset = NORMALIZE_OFFSET(value);

	if( energy < 0 || energy > o->energy ) {
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	x = cell->x + xoffset;
	y = cell->y + yoffset;

	if( x < 0 || x >= u->width ) {
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	if( y < 0 || y >= u->height ) {
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	type = Grid_Get(u, x, y, &ugrid);

	if( type == GT_BLANK ) {
		if( make_spore_mode & 8 ) {
			Kforth_Data_Stack_Push(kfm, 0);
			return;
		}

		/*
		 * 1st spore, energy must be non-zero.
		 */
		if( make_spore_energy != 0 ) {
			// minimum energy to reproduce
			if( energy < make_spore_energy ) {
				Kforth_Data_Stack_Push(kfm, 0);
				return;
			}
		}

		if( energy == 0 ) {
			Kforth_Data_Stack_Push(kfm, 0);
			return;
		}
		o->energy -= energy;
		create_spore(u, o, x, y, energy);
		Kforth_Data_Stack_Push(kfm, 1);

	} else if( type == GT_SPORE ) {
		if( ugrid.u.spore->strain != o->strain ) {
			Kforth_Data_Stack_Push(kfm, 0);
			return;
		}

		if( make_spore_mode & 2 ) {
			if( ugrid.u.spore->parent == o->id ) {
				Kforth_Data_Stack_Push(kfm, 0);
				return;
			}
		}

		if( make_spore_mode & 4 ) {
			if( ugrid.u.spore->parent != o->id ) {
				Kforth_Data_Stack_Push(kfm, 0);
				return;
			}
		}

		if( make_spore_mode & 16 ) {
			Kforth_Data_Stack_Push(kfm, 0);
			return;
		}

		/*
		 * 2nd spore, energy can be zero.
		 */
		if( make_spore_mode & 32 ) {
			if( make_spore_energy != 0 ) {
				// minimum energy to reproduce
				if( energy < make_spore_energy ) {
					Kforth_Data_Stack_Push(kfm, 0);
					return;
				}
			}
		}

		o->energy -= energy;
		Spore_fertilize(u, o, ugrid.u.spore, x, y, energy);
		Kforth_Data_Stack_Push(kfm, -1);

	} else {
		Kforth_Data_Stack_Push(kfm, 0);
	}
}

/***********************************************************************
 * KFORTH NAME:		MAKE-ORGANIC
 * STACK BEHAVIOR:	(x y e -- s)
 *
 * Create organic matter at xoffset, yoffset (normalized) relative to
 * this cell.
 *
 * If a organic is already there, then we add the energy to it.
 *
 */
static void Opcode_MAKE_ORGANIC(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	ORGANISM *o;
	CELL *cell;
	UNIVERSE *u;
	GRID_TYPE type;
	UNIVERSE_GRID *ugrid;
	KFORTH_INTEGER value;
	int x, y, xoffset, yoffset, energy;
	CELL_CLIENT_DATA *cd;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	o = cell->organism;
	u = cd->universe;

	value = Kforth_Data_Stack_Pop(kfm);
	energy = (int) value;

	value = Kforth_Data_Stack_Pop(kfm);
	yoffset = NORMALIZE_OFFSET(value);

	value = Kforth_Data_Stack_Pop(kfm);
	xoffset = NORMALIZE_OFFSET(value);

	if( energy <= 0 ) {
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}
	
	if( energy > o->energy ) {
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	x = cell->x + xoffset;
	y = cell->y + yoffset;

	if( x < 0 || x >= u->width ) {
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	if( y < 0 || y >= u->height ) {
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	type = Grid_GetPtr(u, x, y, &ugrid);

	if( type == GT_BLANK ) {
		ugrid->type = GT_ORGANIC;
		ugrid->u.energy = energy;
		o->energy -= energy;
		Kforth_Data_Stack_Push(kfm, energy);

	} else if( type == GT_ORGANIC ) {
		ugrid->u.energy += energy;
		o->energy -= energy;
		Kforth_Data_Stack_Push(kfm, energy);

	} else {
		Kforth_Data_Stack_Push(kfm, 0);
	}
}

/***********************************************************************
 * KFORTH NAME:		MAKE-BARRIER
 * STACK BEHAVIOR:	(x y -- s)
 *
 * make barrier at normalized (x, y). If barrier alreadfy exists
 * then remove it.
 *
 * Return s=0 when it failed,
 *		s=1 if successful.
 *
 * make_barrier_mode
 *		bit 0	1	0=allowed to create barrier 1=cannot create barrier
 *		bit 1	2	0=allowed to clear barrier 1=cannot clear barrier
 *		bit 2	4
 *
 */
static void Opcode_MAKE_BARRIER(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	ORGANISM *o;
	CELL *cell;
	UNIVERSE *u;
	GRID_TYPE type;
	UNIVERSE_GRID *ugrid;
	KFORTH_INTEGER value;
	int x, y, xoffset, yoffset;
	CELL_CLIENT_DATA *cd;
	int make_barrier_mode;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	o = cell->organism;
	u = cd->universe;

	make_barrier_mode = u->strop[o->strain].make_barrier_mode;

	value = Kforth_Data_Stack_Pop(kfm);
	yoffset = NORMALIZE_OFFSET(value);

	value = Kforth_Data_Stack_Pop(kfm);
	xoffset = NORMALIZE_OFFSET(value);

	x = cell->x + xoffset;
	y = cell->y + yoffset;

	if( x < 0 || x >= u->width ) {
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	if( y < 0 || y >= u->height ) {
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	type = Grid_GetPtr(u, x, y, &ugrid);

	if( type == GT_BLANK ) {
		if( make_barrier_mode & 1 ) {
			// not allowed to make barrier
			Kforth_Data_Stack_Push(kfm, 0);
		} else {
			ugrid->type = GT_BARRIER;
			u->barrier_flag = 1;
			Kforth_Data_Stack_Push(kfm, 1);
		}
	} else if( type == GT_BARRIER ) {
		if( make_barrier_mode & 2 ) {
			// not allowed to clear barrier
			Kforth_Data_Stack_Push(kfm, 0);
		} else {
			ugrid->type = GT_BLANK;
			u->barrier_flag = 1;
			Kforth_Data_Stack_Push(kfm, 1);
		}
	} else {
		Kforth_Data_Stack_Push(kfm, 0);
	}
}

//
// do the grow logic
//
// use_cb == 0:
//		Use this interface:		(x y cb -- rc)	// new energy model
//	else
//		Use this interface:	(x y -- rc)			// new energy model
//
static void grow(KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data, int use_cb)
{
	ORGANISM *o;
	CELL *cell, *ncell;
	UNIVERSE *u;
	KFORTH_INTEGER value;
	int x, y, xoffset, yoffset;
	int grow_mode, grow_size, grow_energy;
	CELL_CLIENT_DATA *cd;
	int energy, cb;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	o = cell->organism;
	u = cd->universe;
	
	grow_mode = u->strop[o->strain].grow_mode;
	grow_size = u->strop[o->strain].grow_size;
	grow_energy = u->strop[o->strain].grow_energy;

	if( use_cb ) {
		value = Kforth_Data_Stack_Pop(kfm);
		cb = value;
	}

	value = Kforth_Data_Stack_Pop(kfm);
	yoffset = NORMALIZE_OFFSET(value);

	value = Kforth_Data_Stack_Pop(kfm);
	xoffset = NORMALIZE_OFFSET(value);

	x = cell->x + xoffset;
	y = cell->y + yoffset;

	if( ! grid_is_blank(u, x, y) ) {
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	energy = (o->ncells+1) * grow_energy;
	if( energy > o->energy )
	{
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	if( use_cb ) {
		if( cb < 0 || cb >= o->program.nblocks ) {
			Kforth_Data_Stack_Push(kfm, 0);
			return;
		}

		// cannot invoke protected cb from unprotected cb
		if( kfm->loc.cb >= kfp->nprotected ) {
			if( cb < kfp->nprotected ) {
				Kforth_Data_Stack_Push(kfm, 0);
				return;
			}
		}
	}

	// maximum number of cells reached
	if( grow_size > 0 && o->ncells >= grow_size )
	{
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	/*
	 * Create new cell:
	 */
	ncell = (CELL *) CALLOC(1, sizeof(CELL));
	ASSERT( ncell != NULL );

	*ncell = *cell;

	kforth_machine_init(&ncell->kfm);
	kforth_machine_copy2(&cell->kfm, &ncell->kfm);

	ncell->kfm.loc.pc += 1;
	ncell->x = x;
	ncell->y = y;
	ncell->next = o->cells;
	o->cells = ncell;

	o->ncells	+= 1;

	ncell->u_next = u->cells;
	ncell->u_prev = NULL;
	if( u->cells ) {
		u->cells->u_prev = ncell;
	}
	u->cells = ncell;

	Grid_SetCell(u, ncell);

	/*
	 * Push 1 on Parent cell's data stack
	 */
	Kforth_Data_Stack_Push(kfm, 1);

	if( use_cb ) {
		ASSERT( cb >= 0 && cb < kfp->nblocks );
		ncell->kfm.loc.cb = cb;
		ncell->kfm.loc.pc = 0;
	} else {
		/*
		 * Push -1 on New cell's data stack
		 */
		Kforth_Data_Stack_Push(&ncell->kfm, -1);
	}
}

/***********************************************************************
 * KFORTH NAME:		GROW
 * STACK BEHAVIOR:	(x y -- s)
 *
 * Organism will grow by 1 cell. The new cell will be
 * placed at the normalized relative offset (x,y) from
 * the cell executing this instruction.
 *
 * Rules grow_mode == 0:
 *	1. the normalized location (x,y) must be vacant.
 *	2. The new cell inherits everything. The only difference is
 *	   its 's' on the stack will be -1.
 *	3. s=0 if grow failed. s=1 for the parent. S=-1 for the new cell.
 *	4. Organism must have enough energy to create new data/call stacks
 *	5. Must be connected to organism in N, S, E, W direction.
 *
 * 's' is set as follows:
 *	0	- failure to grow, parent cell (no new cell).
 *	1	- success, parent cell
 *	-1	- success, new cell
 *
 * grow_mode == 1 rules:
 *	1. Growing into an occupied location CSHIFT's the cells out of the way.
 *
 * The new cell is added to the FRONT of the linked list of cells
 * attached to the organism. (this is important for the Universe_Simulate() functions
 * to work)
 *
 */
static void Opcode_GROW(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	CELL_CLIENT_DATA *cd;
	UNIVERSE *u;

	cd = (CELL_CLIENT_DATA*)client_data;
	u = cd->universe;

	grow(kfp, kfm, client_data, 0);
}

/***********************************************************************
 * KFORTH NAME:		GROW.CB
 * STACK BEHAVIOR:	(x y cb -- s)
 *
 * Organism will grow by 1 cell. The new cell will be
 * placed at the normalized relative offset (x,y) from
 * the cell executing this instruction.
 *
 * Rules grow_mode == 0:
 *	1. the normalized location (x,y) must be vacant.
 *	2. The new cell inherits everything. The only difference is
 *	   its 's' on the stack will be -1.
 *	3. s=0 if grow failed. s=1 for the parent. S=-1 for the new cell.
 *	4. Organism must have enough energy to create new data/call stacks
 *	5. Must be connected to organism in N, S, E, W direction.
 *
 * 's' is set as follows:
 *	0	- failure to grow, parent cell (no new cell).
 *	1	- success, parent cell
 *
 * grow_mode == 1 rules:
 *	1. Growing into an occupied location CSHIFT's the cells out of the way.
 *
 * The new cell is added to the FRONT of the linked list of cells
 * attached to the organism. (this is important for the Universe_Simulate() functions
 * to work)
 *
 */
static void Opcode_GROW_CB(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	CELL_CLIENT_DATA *cd;
	UNIVERSE *u;

	cd = (CELL_CLIENT_DATA*)client_data;
	u = cd->universe;

	grow(kfp, kfm, client_data, 1);
}

/***********************************************************************
 * KFORTH NAME:		EXUDE
 * STACK BEHAVIOR:	(value x y -- )
 *
 * Deposit a number 'value' onto the grid at normalized offset (x,y). Can be 0.
 * Use SMELL to retrieve this value.
 *
 */
static void Opcode_EXUDE(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	CELL *cell;
	ORGANISM *o, *oo;
	UNIVERSE *u;
	KFORTH_INTEGER value;
	int xoffset, yoffset, x, y;
	CELL_CLIENT_DATA *cd;
	int exude_mode;
	UNIVERSE_GRID *ugrid;
	GRID_TYPE gt;
	int ostrain;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	o = cell->organism;
	u = cd->universe;
	exude_mode = u->strop[ o->strain ].exude_mode;

	value = Kforth_Data_Stack_Pop(kfm);
	yoffset = NORMALIZE_OFFSET(value);

	value = Kforth_Data_Stack_Pop(kfm);
	xoffset = NORMALIZE_OFFSET(value);

	value = Kforth_Data_Stack_Pop(kfm);

	x = cell->x + xoffset;
	y = cell->y + yoffset;

	if( x < 0 || x >= u->width || y < 0 || y >= u->height ) {
		return;
	}

	gt = Grid_GetPtr(u, x, y, &ugrid);

	if( exude_mode & 1 ) {
		if( gt != GT_BLANK )
			return;
	}

	if( exude_mode & 2 ) {
		if( gt == GT_BARRIER )
			return;
	}

	if( exude_mode & 4 ) {
		if( gt == GT_CELL ) {
			oo = ugrid->u.cell->organism;
			if( o != oo ) {
				return;
			}
		}
	}

	if( exude_mode & 8 ) {
		if( gt == GT_CELL ) {
			ostrain = ugrid->u.cell->organism->strain;
			if( o->strain != ostrain ) {
				return;
			}
		} else if( gt == GT_SPORE ) {
			ostrain = ugrid->u.spore->strain;
			if( o->strain != ostrain )
				return;
		}
	}

	if( exude_mode & 16 ) {
		if( gt == GT_SPORE ) {
			return;
		}
	}

	if( exude_mode & 32 ) {
		if( gt == GT_ORGANIC ) {
			return;
		}
	}

	Grid_SetOdor(u, x, y, value);
}

static int spawn(UNIVERSE *u, int spawn_mode, int x, int y, int energy, int strain, int cb, CELL *cell)
{
	ORGANISM *no;
	CELL *nc;
	KFORTH_PROGRAM np;
	KFORTH_OPERATIONS *kfops1, *kfops2;
	KFORTH_MUTATE_OPTIONS *kfmo;
	ORGANISM *o;
	int success, num_dstack;

	ASSERT( u != NULL );
	ASSERT( cell != NULL );
	ASSERT( strain >= 0 && strain < 7 );
	ASSERT( x >= 0 );
	ASSERT( y >= 0 );
	ASSERT( energy >= 1 );
	ASSERT( cb >= 0 && cb < cell->organism->program.nblocks );

	o = cell->organism;

	kforth_program_init(&np);
	kfops2 = &u->kfops[o->strain];
	kfops1 = &u->kfops[strain];

	kforth_copy2(&o->program, &np);

	if( o->strain != strain ) {
		success = kforth_remap_instructions(kfops1, kfops2, &np);
		if( ! success ) {
			kforth_program_deinit(&np);
			return 0;
		}

		if( (spawn_mode & 2) == 0 ) {
			// correct protected code blocks when strain changes.
			np.nprotected = u->kfmo[strain].protected_codeblocks;
		} else {
			// don't change protected code blocks when strain changes.
		}
	}

	if( (spawn_mode & 8) == 0 ) {
		// mutate program when mode bit-8 is OFF.
		kfmo = &u->kfmo[o->strain];
		kforth_mutate(kfops1, kfmo, &u->er, &np);
	}

	no = (ORGANISM *) CALLOC(1, sizeof(ORGANISM));
	ASSERT( no != NULL );

	nc = (CELL *) CALLOC(1, sizeof(CELL));
	ASSERT( nc != NULL );

	kforth_machine_init(&nc->kfm);
	if( spawn_mode & 1 ) {
		// inherit cell registers
		memcpy(&nc->kfm.R,  cell->kfm.R, sizeof(cell->kfm.R));
	}

	num_dstack = ((spawn_mode >> 4) & 7);
	num_dstack = (cell->kfm.dsp < num_dstack) ? cell->kfm.dsp : num_dstack;

	nc->kfm.dsp = num_dstack;
	memcpy(nc->kfm.data_stack, cell->kfm.data_stack + (cell->kfm.dsp - num_dstack), num_dstack * sizeof(KFORTH_INTEGER) );

	nc->kfm.loc.cb = cb;
	nc->kfm.loc.pc = 0;

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

	no->strain	= strain;
	no->id		= u->next_id++;
	no->parent1	= o->id;
	no->parent2	= o->id;
	no->generation	= o->generation + 1;
	no->energy	= energy;
	o->energy	-= energy;
	no->age		= 0;
	no->program	= np;

	no->ncells	= 1;
	no->sim_count = 1;
	no->cells	= nc;

	no->next	= NULL;
	no->prev	= NULL;

	/*
	 * If organism is radioactive, then new organism is too
	 */
	if( o->oflags & ORGANISM_FLAG_RADIOACTIVE ) {
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

	return 1;
}

/***********************************************************************
 * KFORTH NAME:		SPAWN
 * STACK BEHAVIOR:	(x y e strain cb -- r)
 *
 * Spawn a new organism with strain 'strain'. Energy will be 'e'.
 * Location will be the normalized (x,y) offset. 'cb' is the code block
 * which the organism will start running.
 *
 * Returns 1 on success, else 0.
 *
 * spawn_mode:
 *		BIT	MASK	Meaning when bit is 0				Meaning when bit is 1
 *		0	1	do not inherit cell register values		inherit register values
 *		1	2	NOT DEFINED								NOT DEFINED
 *		2	4	cannot spawn to a different strain		can spawn to a different strain
 *		3	8	mutate the program using my settings	do not mutate the program
 *		4	16	do not inherit data stack				inherit N data stack items from top and bit-0 of N is 1
 *		5	32	do not inherit data stack				inherit N data stack items from top and bit-1 of N is 1
 *		6	64	do not inherit data stack				inherit N data stack items from top and bit-2 of N is 1
 *
 */
static void Opcode_SPAWN(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	CELL *cell;
	ORGANISM *o;
	UNIVERSE *u;
	KFORTH_INTEGER value;
	int xoffset, yoffset, x, y, success;
	int energy, cb, strain, spawn_mode;
	CELL_CLIENT_DATA *cd;
	GRID_TYPE type;
	UNIVERSE_GRID ugrid;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	o = cell->organism;
	u = cd->universe;

	spawn_mode = u->strop[ o->strain ].spawn_mode;

	value = Kforth_Data_Stack_Pop(kfm);
	cb = value;

	value = Kforth_Data_Stack_Pop(kfm);
	strain = value;

	value = Kforth_Data_Stack_Pop(kfm);
	energy = value;

	value = Kforth_Data_Stack_Pop(kfm);
	yoffset = NORMALIZE_OFFSET(value);

	value = Kforth_Data_Stack_Pop(kfm);
	xoffset = NORMALIZE_OFFSET(value);

	x = cell->x + xoffset;
	y = cell->y + yoffset;

	// position is invalid
	if( x < 0 || x >= u->width || y < 0 || y >= u->height ) {
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	type = Grid_Get(u, x, y, &ugrid);
	if( type != GT_BLANK ) {
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	// energy is negative
	if( energy <= 0 ) {
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	// not enough energy
	if( energy >= o->energy ) {
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	// strain is bogus
	if( strain < 0 || strain > 7 || u->strop[strain].enabled == 0 ) {
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	// can't change strain in spawn operation
	if( (spawn_mode & 4) == 0 && strain != o->strain) {
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	// cb is bogus
	if( cb < 0 || cb >= cell->organism->program.nblocks ) {
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	// cb is protected, and we are running in unprotected mode
	if( kfm->loc.cb >= kfp->nprotected ) {
		if( (spawn_mode & 2) == 0 ) {
			if( cb < kfp->nprotected ) {
				Kforth_Data_Stack_Push(kfm, 0);
				return;
			}
		} else {
			if( cb < u->kfmo[strain].protected_codeblocks ) {
				Kforth_Data_Stack_Push(kfm, 0);
				return;
			}
		}
	}

	success = spawn(u, spawn_mode, x, y, energy, strain, cb, cell);
	Kforth_Data_Stack_Push(kfm, success);
}

/***********************************************************************
 * KFORTH NAME:		LOOK
 * STACK BEHAVIOR:	(x y -- what dist)
 *
 * Look along (x, y) and return a 'what' value and a 'dist' value.
 *
 */
static void Opcode_LOOK(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	CELL *cell;
	ORGANISM *o;
	UNIVERSE *u;
	KFORTH_INTEGER value;
	int xoffset, yoffset, look_mode;
	LOOK_RESULT res;
	CELL_CLIENT_DATA *cd;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	o = cell->organism;
	u = cd->universe;

	value = Kforth_Data_Stack_Pop(kfm);
	yoffset = NORMALIZE_OFFSET(value);

	value = Kforth_Data_Stack_Pop(kfm);
	xoffset = NORMALIZE_OFFSET(value);

	if( yoffset == 0 && xoffset == 0 ) {
		Kforth_Data_Stack_Push(kfm, 0);
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	look_mode = u->strop[o->strain].look_mode;

	look_along_line(u, cell, look_mode, xoffset, yoffset, &res);

	Kforth_Data_Stack_Push(kfm, res.what);
	Kforth_Data_Stack_Push(kfm, res.dist);
}

/*
 * Pick random number between 'a' and 'b' (includes 'a' and 'b')
 */
#define CHOOSE(er,a,b)	( (sim_random(er) % ((b)-(a)+1) ) + (a) )

/***********************************************************************
 *
 * This routine implements the logic for many instructions.
 *
 *	NEAREST, FARTHEST,BIGGEST, SMALLEST, HOTTEST, COLDEST
 *
 */
enum {
	ATTR_NEAREST,
	ATTR_FARTHEST,
	ATTR_BIGGEST,
	ATTR_SMALLEST,
	ATTR_HOTTEST,		// most energy for cell
	ATTR_COLDEST,		// least energy for cell
};

static void generic_vision_search(KFORTH_MACHINE *kfm, int attribute, void *client_data)
{
	static const int xoffset[8] = {  0,  1,  1,  1,  0, -1, -1, -1 };
	static const int yoffset[8] = { -1, -1,  0,  1,  1,  1,  0, -1 };

	CELL *cell;
	ORGANISM *o;
	UNIVERSE *u;
	KFORTH_INTEGER value;
	CELL_CLIENT_DATA *cd;
	LOOK_RESULT res;
	int i, mask;
	int look_mode;
	int found, c = 0;

	int dir, best_dir = 0;
	int best_dist = 0;
	int best_energy = 0;
	int best_size = 0;
	//int best_tot_energy = 0;

	ASSERT( kfm != NULL );

	value = Kforth_Data_Stack_Pop(kfm);
	mask = (int) (value & VISION_MASK);

	if( mask == 0 ) {
		Kforth_Data_Stack_Push(kfm, 0);
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	o = cell->organism;
	u = cd->universe;
	look_mode = u->strop[o->strain].look_mode;

	switch( attribute ) {
	case ATTR_NEAREST:
		best_dist = EVOLVE_MAX_BOUNDS + 1000;
		break;
	case ATTR_FARTHEST:
		best_dist = -1;
		break;
	case ATTR_BIGGEST:
		best_size = -1;
		break;
	case ATTR_SMALLEST:
		best_size = 1000000;		// 1 million
		break;
	case ATTR_HOTTEST:
		best_energy = -1;
		break;
	case ATTR_COLDEST:
		best_energy = 1000000;		// 1 million
		break;
	}

	/*
	 * Pick a random starting direction, then
	 * scan clock-wise.
	 */
	dir = CHOOSE(&u->er, 0, 7);

	found = 0;
	for(i=0; i<8; i++) {
		look_along_line(u, cell, look_mode, xoffset[dir], yoffset[dir], &res);

		ASSERT( res.dist != 0 );
		ASSERT( res.what != 0 );

		if( (res.what & mask) ) {
			found = 1;
			switch( attribute ) {
			case ATTR_NEAREST:
				c = res.dist < best_dist;
				break;
			case ATTR_FARTHEST:
				c = res.dist > best_dist;
				break;
			case ATTR_BIGGEST:
				c = res.size > best_size;
				break;
			case ATTR_SMALLEST:
				c = res.size < best_size;
				break;
			case ATTR_HOTTEST:
				c = res.energy > best_energy;
				break;
			case ATTR_COLDEST:
				c = res.energy < best_energy;
				break;
			}

			if( c ) {
				best_dist = res.dist;
				best_dir = dir;
				best_energy = res.energy;
				best_size = res.size;
			}
		}
		dir += 1;
		if( dir > 7 )
			dir = 0;
	}

	if( found ) {
		Kforth_Data_Stack_Push(kfm, xoffset[best_dir]*best_dist );
		Kforth_Data_Stack_Push(kfm, yoffset[best_dir]*best_dist );
	} else {
		Kforth_Data_Stack_Push(kfm, 0);
		Kforth_Data_Stack_Push(kfm, 0);
	}
}

/***********************************************************************
 * KFORTH NAME:		NEAREST
 * STACK BEHAVIOR:	(mask -- x y)
 *
 * 	- Look infinitely in all 8 directions.
 *	- Return (x, y) direction that contains the smallest distance, and
 *	  matches 'mask'.
 *
 */
static void Opcode_NEAREST(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	generic_vision_search(kfm, ATTR_NEAREST, client_data);
}

/***********************************************************************
 * KFORTH NAME:		FARTHEST
 * STACK BEHAVIOR:	(mask -- x y)
 *
 * Look in all 8 directions and
 * find the (x, y) vision coordinate contains has
 * the farthest object matching the 'mask'.
 *
 */
static void Opcode_FARTHEST(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	generic_vision_search(kfm, ATTR_FARTHEST, client_data);
}

/***********************************************************************
 * KFORTH NAME:		SIZE
 * STACK BEHAVIOR:	(x y -- size dist)
 *
 * Look along x,y. repost size and and distance to what was found.
 * Return 0 0 if nothing 
 */
static void Opcode_SIZE(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	CELL *cell;
	ORGANISM *o;
	UNIVERSE *u;
	KFORTH_INTEGER value;
	int xoffset, yoffset;
	int look_mode;
	CELL_CLIENT_DATA *cd;
	LOOK_RESULT res;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	o = cell->organism;
	u = cd->universe;
	look_mode = u->strop[o->strain].look_mode;

	value = Kforth_Data_Stack_Pop(kfm);
	yoffset = NORMALIZE_OFFSET(value);

	value = Kforth_Data_Stack_Pop(kfm);
	xoffset = NORMALIZE_OFFSET(value);

	if( yoffset == 0 && xoffset == 0 ) {
		Kforth_Data_Stack_Push(kfm, 0);
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	look_along_line(u, cell, look_mode, xoffset, yoffset, &res);

	value = (res.size < TOO_BIG) ? res.size : TOO_BIG;

	Kforth_Data_Stack_Push(kfm, value);
	Kforth_Data_Stack_Push(kfm, res.dist);
}

/***********************************************************************
 * KFORTH NAME:		BIGGEST
 * STACK BEHAVIOR:	(mask -- x y)
 *
 * Look along in all 8 directions. Report the x,y vector to
 * the biggest thing. 'mask' filters on what to consider.
 *
 * Return 0,0 if nothing 
 */
static void Opcode_BIGGEST(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	generic_vision_search(kfm, ATTR_BIGGEST, client_data);
}

/***********************************************************************
 * KFORTH NAME:		SMALLEST
 * STACK BEHAVIOR:	(mask -- x y)
 *
 * Look along in all 8 directions. Report the x,y vector to
 * the smallest thing. 'mask' filters on what to consider
 */
static void Opcode_SMALLEST(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	generic_vision_search(kfm, ATTR_SMALLEST, client_data);
}

/***********************************************************************
 * KFORTH NAME:		TEMPERATURE
 * STACK BEHAVIOR:	(x y -- energy dist)
 *
 * Look along x,y. repost energy level and and distance to what was found.
 * Return 0 0 if nothing 
 */
static void Opcode_TEMPERATURE(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	CELL *cell;
	ORGANISM *o;
	UNIVERSE *u;
	KFORTH_INTEGER value;
	int xoffset, yoffset;
	int look_mode;
	CELL_CLIENT_DATA *cd;
	LOOK_RESULT res;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	o = cell->organism;
	u = cd->universe;

	value = Kforth_Data_Stack_Pop(kfm);
	yoffset = NORMALIZE_OFFSET(value);

	value = Kforth_Data_Stack_Pop(kfm);
	xoffset = NORMALIZE_OFFSET(value);

	if( yoffset == 0 && xoffset == 0 ) {
		Kforth_Data_Stack_Push(kfm, 0);
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	look_mode = u->strop[o->strain].look_mode;

	look_along_line(u, cell, look_mode, xoffset, yoffset, &res);

	value = (res.energy < TOO_BIG) ? res.energy : TOO_BIG;

	Kforth_Data_Stack_Push(kfm, value);
	Kforth_Data_Stack_Push(kfm, res.dist);
}

/***********************************************************************
 * KFORTH NAME:		HOTTEST
 * STACK BEHAVIOR:	(mask -- x y)
 *
 * Look in all 8 directions. report x y vector to most energetic thing.
 * 'mask' filters on what to consider.
 * Return 0,0 if nothing
 */
static void Opcode_HOTTEST(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	generic_vision_search(kfm, ATTR_HOTTEST, client_data);
}

/***********************************************************************
 * KFORTH NAME:		COLDEST
 * STACK BEHAVIOR:	(mask -- x y)
 *
 * Look in all 8 directions. report x y vector to least energetic thing.
 * 'mask' filters on what to consider.
 * Return 0,0 if nothing 
 */
static void Opcode_COLDEST(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	generic_vision_search(kfm, ATTR_COLDEST, client_data);
}

/***********************************************************************
 * KFORTH NAME:		SMELL
 * STACK BEHAVIOR:	(x y -- value)
 *
 * Query the grid at the normalized coordinates (x,y) relative to this cell.
 * Return the grid's 'odor' value.
 */
static void Opcode_SMELL(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	CELL *cell;
	ORGANISM *o;
	UNIVERSE *u;
	UNIVERSE_GRID *ugp;
	KFORTH_INTEGER value;
	int xoffset, yoffset, x, y;
	CELL_CLIENT_DATA *cd;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	o = cell->organism;
	u = cd->universe;

	value = Kforth_Data_Stack_Pop(kfm);
	yoffset = NORMALIZE_OFFSET(value);

	value = Kforth_Data_Stack_Pop(kfm);
	xoffset = NORMALIZE_OFFSET(value);

	x = cell->x + xoffset;
	y = cell->y + yoffset;

	if( x < 0 || x >= u->width || y < 0 || y >= u->height ) {
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	Grid_GetPtr(u, x, y, &ugp);

	Kforth_Data_Stack_Push(kfm, ugp->odor);
}

/***********************************************************************
 * KFORTH NAME:		MOOD
 * STACK BEHAVIOR:	(x y -- mood)
 *
 * Get mood of the cell that is located
 * at xoffset, yoffset relative to this cell
 *
 * mood will be set to the mood of the cell located at (x,y). If there
 * is no such cell, then mood=0.
 *
 */
static void Opcode_MOOD(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	CELL *cell, *c;
	ORGANISM *o;
	UNIVERSE *u;
	int xoffset, yoffset;
	KFORTH_INTEGER value;
	CELL_CLIENT_DATA *cd;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	o = cell->organism;
	u = cd->universe;

	value = Kforth_Data_Stack_Pop(kfm);
	yoffset = (int)value;

	value = Kforth_Data_Stack_Pop(kfm);
	xoffset = (int)value;

	c = Cell_Neighbor(u, cell, xoffset, yoffset);

	if( c != NULL ) {
		value = c->mood;
	} else {
		value = 0;
	}

	Kforth_Data_Stack_Push(kfm, value);
}

/***********************************************************************
 * KFORTH NAME:		MOOD!
 * STACK BEHAVIOR:	(m -- )
 *
 * Set our mood to 'm'.
 *
 */
static void Opcode_SET_MOOD(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	CELL *cell;
	ORGANISM *o;
	UNIVERSE *u;
	KFORTH_INTEGER value;
	CELL_CLIENT_DATA *cd;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	o = cell->organism;
	u = cd->universe;

	value = Kforth_Data_Stack_Pop(kfm);

	cell->mood = value;
}

/***********************************************************************
 * KFORTH NAME:		BROADCAST
 * STACK BEHAVIOR:	(m -- )
 *
 * Send message 'm' to every cell in the organism (including ourselves).
 *
 */
static void Opcode_BROADCAST(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	CELL *cell, *ccurr;
	ORGANISM *o;
	UNIVERSE *u;
	KFORTH_INTEGER value;
	CELL_CLIENT_DATA *cd;
	int broadcast_mode;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	o = cell->organism;
	u = cd->universe;
	broadcast_mode = u->strop[o->strain].broadcast_mode;

	value = Kforth_Data_Stack_Pop(kfm);

	for(ccurr=o->cells; ccurr; ccurr=ccurr->next) {
		ccurr->message = value;
		if( ccurr != cell ) {
			interrupt(ccurr, broadcast_mode);
		}
	}
}

/***********************************************************************
 * KFORTH NAME:		SEND
 * STACK BEHAVIOR:	(m x y -- )
 *
 * 3 values are on the stack. X offset and Y offset
 * identify a cell relative to this one. Send the
 * message to that organism.
 *
 */
static void Opcode_SEND(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	CELL *cell, *c;
	UNIVERSE *u;
	int xoffset, yoffset;
	KFORTH_INTEGER value, message;
	CELL_CLIENT_DATA *cd;
	int o_send_mode;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	u = cd->universe;

	value = Kforth_Data_Stack_Pop(kfm);
	yoffset = (int)value;

	value = Kforth_Data_Stack_Pop(kfm);
	xoffset = (int)value;

	message = Kforth_Data_Stack_Pop(kfm);

	c = Cell_Neighbor(u, cell, xoffset, yoffset);
	if( c != NULL ) {
		c->message = message;

		o_send_mode = u->strop[c->organism->strain].send_mode;
		interrupt(c, o_send_mode);
	}
}

/***********************************************************************
 * KFORTH NAME:		RECV
 * STACK BEHAVIOR:	( -- m)
 *
 * Push contents of recieve buffer onto stack
 */
static void Opcode_RECV(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	CELL *cell;
	CELL_CLIENT_DATA *cd;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;

	Kforth_Data_Stack_Push(kfm, cell->message);
}

static int shout(UNIVERSE *u, CELL *cell, int shout_mode, KFORTH_INTEGER message, int xoffset, int yoffset)
{
	int x, y;
	GRID_TYPE gt;
	UNIVERSE_GRID *ugrid;
	CELL *c;
	int other_shout_mode, intflags;

	ASSERT( xoffset >= -1 && xoffset <= 1 );
	ASSERT( yoffset >= -1 && yoffset <= 1 );

	x = cell->x;
	y = cell->y;
	gt = GT_BLANK; // trick loop to enter first time
	while( gt == GT_BLANK ) {
		x = x + xoffset;
		y = y + yoffset;

		if( x < 0 || x >= u->width
			|| y < 0 || y >= u->height ) {
			gt = GT_BARRIER;
		} else {
			gt = Grid_GetPtr(u, x, y, &ugrid);
		}

		if( (shout_mode & 1) == 0 ) { // see thru self
			if( gt == GT_CELL
				&& ugrid->u.cell->organism == cell->organism )
			{
				gt = GT_BLANK; // trick loop to see thru self
			}
		}
	}

	if( gt != GT_CELL ) {
		return 0;
	}

	c = ugrid->u.cell;

	// cannot shout at self
	if( c->organism == cell->organism ) {
		return 0;
	}

	if( (shout_mode & 4)						// cannot shout at strain
			&& c->organism->strain == cell->organism->strain ) {
		return 0;
	}

	if( c->organism->strain != cell->organism->strain ) {
		other_shout_mode = u->strop[ c->organism->strain ].shout_mode;
		if( other_shout_mode & 8 ) {					// other strains cannot shout at this organism
			return 0;
		}
	}

	c->message = message;

	intflags = (shout_mode >> 4) & 7;
	interrupt(c, intflags);

	return 1;
}

/***********************************************************************
 * KFORTH NAME:		SHOUT
 * STACK BEHAVIOR:	(m -- r)
 *
 * try to broadcast the message 'm' to surrounding cells
 * They don't have to be part of my organism.
 *
 *	shout_mode	bit-0	1		0=i Shout thru self					1=i cannot shout thru self
 *				bit-1	2		0=i can shout at self				1=i cannot shout at self
 *				bit-2	4		0=i can shout at my strain			1=i cannot shout at my strain
 *				bit-3	8		0=i can shout at other strains		1=i cannot shout at other strains
 *				bit-4	16		0=other strains can shout at me		1=other strains cannot shout at me
 *
 */
static void Opcode_SHOUT(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	CELL *cell;
	ORGANISM *o;
	UNIVERSE *u;
	KFORTH_INTEGER value, message;
	CELL_CLIENT_DATA *cd;
	int shout_mode;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	o = cell->organism;
	u = cd->universe;

	shout_mode = u->strop[o->strain].shout_mode;

	message = Kforth_Data_Stack_Pop(kfm);

	value = 0;
	value += shout(u, cell, shout_mode, message, 0, -1); // n
	value += shout(u, cell, shout_mode, message, 1, -1); // ne
	value += shout(u, cell, shout_mode, message, 1, 0); // e
	value += shout(u, cell, shout_mode, message, 1, 1); // se
	value += shout(u, cell, shout_mode, message, 0, 1); // s
	value += shout(u, cell, shout_mode, message, -1, 1); // sw
	value += shout(u, cell, shout_mode, message, -1, 0); // w 
	value += shout(u, cell, shout_mode, message, -1, -1); // nw

	Kforth_Data_Stack_Push(kfm, value);
}

/***********************************************************************
 * KFORTH NAME:		LISTEN
 * STACK BEHAVIOR:	(x y -- mood dist)
 *
 * Listen for a MOOD register value from a cell along (x,y) vector.
 * Return (0,0) if nothing to listen to found.
 *
 * Listen_mode is supposed to be same as look_mode, infact they must be same:
 *
 *	bit-0 1		SEE_SELF	0=see thru self,			1=can't see thru self
 *
 */
static void Opcode_LISTEN(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	CELL *cell;
	ORGANISM *o;
	UNIVERSE *u;
	KFORTH_INTEGER value;
	int xoffset, yoffset;
	int listen_mode, look_mode;
	CELL_CLIENT_DATA *cd;
	LOOK_RESULT res;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	o = cell->organism;
	u = cd->universe;
	listen_mode = u->strop[o->strain].listen_mode;

	value = Kforth_Data_Stack_Pop(kfm);
	yoffset = NORMALIZE_OFFSET(value);

	value = Kforth_Data_Stack_Pop(kfm);
	xoffset = NORMALIZE_OFFSET(value);

	if( yoffset == 0 && xoffset == 0 ) {
		Kforth_Data_Stack_Push(kfm, 0);
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	if( listen_mode & 1 ) {
		look_mode = 1;
	} else {
		look_mode = 0;
	}

	look_along_line(u, cell, look_mode, xoffset, yoffset, &res);

	value = res.mood;

	if( res.what & VISION_TYPE_CELL ) {
		Kforth_Data_Stack_Push(kfm, value);
		Kforth_Data_Stack_Push(kfm, res.dist);
	} else {
		Kforth_Data_Stack_Push(kfm, 0);
		Kforth_Data_Stack_Push(kfm, 0);
	}
}

/***********************************************************************
 * KFORTH NAME:		SAY
 * STACK BEHAVIOR:	(m x y -- dist)
 *
 * Send the message 'm' outward in normalized direction (x,y). If
 * it reaches a cell, place 'm' into its message register.
 *
 * Returns: non-zero on success, zero on fail. returns distance to thing SAY'd to.
 *
 * Rules:
 *	(0,0)		does nothing
 *
 * BIT	MASK	Meaning when bit is 0	Meaning when bit is 1
 * 0	1	i can speak thru myself		i cannot speak thru myself
 * 1	2	i can speak at cell that belong to my organism			i cannot speak at cell that belong to my organism
 * 2	4	i can speak at cells that belong to my strain			i cannot speak at cells that belong to my strain
 * 3	8	i can speak at cell the belong to OTHER strains			i cannot speak at cell the belong to OTHER strains
 * 4	16	other strains can speak at me							other strains cannot speak at me
 * 5	32	do not interrupt me, when somebody speaks a message to me		interrupt me and bit 0 of the trap number shall be 1
 * 6	64	do not interrupt me, when somebody speaks a message to me		interrupt me and bit 1 of the trap number shall be 1
 * 7	128	do not interrupt me, when somebody speaks a message to me		interrupt me and bit 2 of the trap number shall be 1
 */
static void Opcode_SAY(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	CELL *cell;
	ORGANISM *o;
	UNIVERSE *u;
	KFORTH_INTEGER value;
	int xoffset, yoffset;
	int say_mode, look_mode;
	CELL_CLIENT_DATA *cd;
	LOOK_RESULT res;
	GRID_TYPE gt;
	UNIVERSE_GRID *ugrid;
	ORGANISM *oo;
	CELL *ocell;
	int x, y, o_say_mode, intflags;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	o = cell->organism;
	u = cd->universe;
	say_mode = u->strop[o->strain].say_mode;

	value = Kforth_Data_Stack_Pop(kfm);
	yoffset = NORMALIZE_OFFSET(value);

	value = Kforth_Data_Stack_Pop(kfm);
	xoffset = NORMALIZE_OFFSET(value);

	value = Kforth_Data_Stack_Pop(kfm);

	if( yoffset == 0 && xoffset == 0 ) {
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	if( say_mode & 1 ) {
		look_mode = 1;
	} else {
		look_mode = 0;
	}

	look_along_line(u, cell, look_mode, xoffset, yoffset, &res);

	if( (res.what & VISION_TYPE_CELL) == 0 ) {
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	x = cell->x + xoffset * res.dist;
	y = cell->y + yoffset * res.dist;

	gt = Grid_GetPtr(u, x, y, &ugrid);

	ASSERT( gt == GT_CELL );

	ocell = ugrid->u.cell;
	oo = ocell->organism;

	if( say_mode & 2 ) {
		if( oo == o ) {
			Kforth_Data_Stack_Push(kfm, 0);
			return;
		}
	}

	if( say_mode & 4 ) {
		if( oo->strain == o->strain ) {
			Kforth_Data_Stack_Push(kfm, 0);
			return;
		}
	}

	if( say_mode & 8 ) {
		if( oo->strain != o->strain ) {
			Kforth_Data_Stack_Push(kfm, 0);
			return;
		}
	}

	o_say_mode = u->strop[ oo->strain ].say_mode;

	if( o_say_mode & 16 ) {
		if( oo->strain != o->strain ) {
			Kforth_Data_Stack_Push(kfm, 0);
			return;
		}
	}

	ocell->message = value;

	intflags = (o_say_mode >> 5) & 7;
	interrupt(ocell, intflags);

	Kforth_Data_Stack_Push(kfm, res.dist);
}

/***********************************************************************
 * KFORTH NAME:		READ
 * STACK BEHAVIOR:	(x y cb cbme -- rc)
 *
 * Read the code block 'cb' from the cell or spore located at the normalized (x,y) offset.
 * Place the contents of this code block into my program store at code block 'cbme'.
 *
 * modes:
 *
 */
static void Opcode_READ(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	CELL_CLIENT_DATA *cd;
	UNIVERSE *u;
	ORGANISM *org;
	CELL *cell, *ocell;
	SPORE *ospore;
	GRID_TYPE gt;
	UNIVERSE_GRID *ugrid;
	KFORTH_INTEGER value;
	KFORTH_INTEGER *src_block;
	KFORTH_INTEGER *new_block;
	KFORTH_INTEGER opcode;
	KFORTH_MUTATE_OPTIONS *kfmo;
	KFORTH_PROGRAM *okfp;
	KFORTH_OPERATIONS *okfops;
	int xoffset, yoffset, x, y, pc, ostrain;
	int success, read_mode, o_read_mode;
	int cb, cbme, num_cnt, len;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	org = cell->organism;
	u = cd->universe;
	read_mode = u->strop[ org->strain ].read_mode;

	value = Kforth_Data_Stack_Pop(kfm);
	cbme = value;

	value = Kforth_Data_Stack_Pop(kfm);
	cb = value;

	value = Kforth_Data_Stack_Pop(kfm);
	yoffset = NORMALIZE_OFFSET(value);

	value = Kforth_Data_Stack_Pop(kfm);
	xoffset = NORMALIZE_OFFSET(value);

	if( cb < 0 ) {
		// invalid code block cb
		Kforth_Data_Stack_Push(kfm, -1);
		return;
	}

	if( cbme < 0 || cbme >= kfp->nblocks ) {
		// invalid code block 'cbme'
		Kforth_Data_Stack_Push(kfm, -2);
		return;
	}

	x = cell->x + xoffset;
	y = cell->y + yoffset;

	if( x < 0 || x >= u->width ) {
		Kforth_Data_Stack_Push(kfm, -3);
		return;
	}

	if( y < 0 || y >= u->height ) {
		Kforth_Data_Stack_Push(kfm, -3);
		return;
	}

	gt = Grid_GetPtr(u, x, y, &ugrid);

	if( gt == GT_SPORE ) {
		ospore = ugrid->u.spore;
		ostrain = ospore->strain;
		okfp = &ospore->program;

	} else if( gt == GT_CELL ) {
		ocell = ugrid->u.cell;
		ostrain = ocell->organism->strain;
		okfp = &ocell->organism->program;

	} else {
		// nothing to READ
		Kforth_Data_Stack_Push(kfm, -3);
		return;
	}

	if( cb >= okfp->nblocks ) {
		// invalid code block 'cb'
		Kforth_Data_Stack_Push(kfm, -1);
		return;
	}

	if( kfm->loc.cb >= kfp->nprotected ) {
		if( cb < okfp->nprotected ) {
			// unprotected code cannot READ from a protected code block.
			Kforth_Data_Stack_Push(kfm, -4);
			return;
		}

		if( cbme < kfp->nprotected ) {
			// unprotected code cannot READ from something and then put it into a protected code block.
			Kforth_Data_Stack_Push(kfm, -5);
			return;
		}
	}

	if( read_mode & 2 ) {
		if( ostrain == org->strain ) {
			// can't read from my strain
			Kforth_Data_Stack_Push(kfm, -6);
			return;
		}
	}

	if( (read_mode & 4) == 0 ) {
		if( ostrain != org->strain ) {
			// can't read from other strains
			Kforth_Data_Stack_Push(kfm, -6);
			return;
		}
	}

	if( read_mode & 8 ) {
		if( gt == GT_SPORE ) {
			// can't read from spore
			Kforth_Data_Stack_Push(kfm, -7);
			return;
		}
	}

	if( read_mode & 16 ) {
		if( gt == GT_CELL ) {
			// can't read from cells
			Kforth_Data_Stack_Push(kfm, -8);
			return;
		}
	}

	if( (read_mode & 1) == 0 ) {
		if( gt == GT_CELL ) {
			if( ugrid->u.cell->organism == cell->organism ) {
				// can't read from self
				Kforth_Data_Stack_Push(kfm, -9);
				return;
			}
		}
	}

	o_read_mode = u->strop[ ostrain ].read_mode;

	if( o_read_mode & 32 ) {
		// this strain cannot be read
		Kforth_Data_Stack_Push(kfm, -10);
		return;
	}

	//
	// All permission checks have occured.
	// We can proceed with the 'READ' operation.
	//
	// 1. send_mode & 64 will be checked for mutations
	// 2. instruction remap happens which can trigger a failure
	// 3. If Protected instructions are found, then the READ operation fails.
	//

	src_block = okfp->block[cb];
	len = src_block[-1];

	new_block = (KFORTH_INTEGER*) CALLOC(len+1, sizeof(KFORTH_INTEGER)) + 1;
	ASSERT( new_block != NULL );

	new_block[-1] = len;

	num_cnt = 0;
	for(pc=0; pc < len; pc++) {
		new_block[pc] = src_block[pc];
		if( src_block[pc] & 0x8000 ) {
			num_cnt += 1;
		}
	}

	if( num_cnt != len ) {
		if( ostrain != org->strain ) {
			okfops = &u->kfops[ ostrain ];
			success = kforth_remap_instructions_cb(kfops, okfops, new_block);
			if( ! success ) {
				// remap instructions failed
				FREE( new_block-1 );

				Kforth_Data_Stack_Push(kfm, -11);
				return;
			}
		}

		if( kfm->loc.cb >= kfp->nprotected ) {
			for(pc=0; pc < len; pc++) {
				opcode = new_block[pc];
				if( (opcode & 0x8000) == 0) {
					if( opcode < kfops->nprotected ) {
						//
						// unprotected code cannot read a code block that
						// contains any protected instructions.
						//
						FREE( new_block-1 );

						Kforth_Data_Stack_Push(kfm, -12);
						return;
					}
				}
			}
		}
	}

	if( (read_mode & 64) == 0 ) {
		kfmo = &u->kfmo[org->strain];
		kforth_mutate_cb(kfops, kfmo, &u->er, &new_block);
	}

	FREE( kfp->block[cbme]-1 );
	kfp->block[cbme] = new_block;

	org->oflags |= ORGANISM_FLAG_READWRITE;

	Kforth_Data_Stack_Push(kfm, len);
}

/***********************************************************************
 * KFORTH NAME:		WRITE
 * STACK BEHAVIOR:	(x y cb cbme -- rc)
 *
 * Write the code block 'cbme' from my program store to the cell or spore located at the normalized (x,y) offset.
 * Place the contents 'cbme' code block into code block 'cb' of the destination cell or spore.
 *
 * modes:
 *
 */
static void Opcode_WRITE(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	CELL_CLIENT_DATA *cd;
	UNIVERSE *u;
	ORGANISM *org;
	CELL *cell, *ocell;
	SPORE *ospore;
	GRID_TYPE gt;
	UNIVERSE_GRID *ugrid;
	KFORTH_INTEGER value;
	KFORTH_INTEGER *src_block;
	KFORTH_INTEGER *new_block;
	KFORTH_INTEGER opcode;
	KFORTH_MUTATE_OPTIONS *kfmo;
	KFORTH_PROGRAM *okfp;
	KFORTH_OPERATIONS *okfops;
	int xoffset, yoffset, x, y, pc, ostrain;
	int success, write_mode, o_write_mode;
	int cb, cbme, intflags, len, num_cnt;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	org = cell->organism;
	u = cd->universe;
	write_mode = u->strop[ org->strain ].write_mode;

	value = Kforth_Data_Stack_Pop(kfm);
	cbme = value;

	value = Kforth_Data_Stack_Pop(kfm);
	cb = value;

	value = Kforth_Data_Stack_Pop(kfm);
	yoffset = NORMALIZE_OFFSET(value);

	value = Kforth_Data_Stack_Pop(kfm);
	xoffset = NORMALIZE_OFFSET(value);

	if( cb < 0 ) {
		// invalid code block cb
		Kforth_Data_Stack_Push(kfm, -1);
		return;
	}

	if( cbme < 0 || cbme >= kfp->nblocks ) {
		// invalid code block 'cbme'
		Kforth_Data_Stack_Push(kfm, -2);
		return;
	}

	x = cell->x + xoffset;
	y = cell->y + yoffset;

	if( x < 0 || x >= u->width ) {
		Kforth_Data_Stack_Push(kfm, -3);
		return;
	}

	if( y < 0 || y >= u->height ) {
		Kforth_Data_Stack_Push(kfm, -3);
		return;
	}

	gt = Grid_GetPtr(u, x, y, &ugrid);

	if( gt == GT_SPORE ) {
		ospore = ugrid->u.spore;
		ostrain = ospore->strain;
		okfp = &ospore->program;
		okfops = &u->kfops[ ostrain ];

	} else if( gt == GT_CELL ) {
		ocell = ugrid->u.cell;
		ostrain = ocell->organism->strain;
		okfp = &ocell->organism->program;
		okfops = &u->kfops[ ostrain ];

	} else {
		// nothing to WRITE
		Kforth_Data_Stack_Push(kfm, -3);
		return;
	}

	if( cb >= okfp->nblocks ) {
		// invalid code block 'cb'
		Kforth_Data_Stack_Push(kfm, -1);
		return;
	}

	if( kfm->loc.cb >= kfp->nprotected ) {
		if( cb < okfp->nprotected ) {
			// unprotected code cannot write to the other protected code block.
			Kforth_Data_Stack_Push(kfm, -4);
			return;
		}

		if( cbme < kfp->nprotected ) {
			// unprotected code cannot WRITE a protected code block.
			Kforth_Data_Stack_Push(kfm, -5);
			return;
		}
	}

	if( write_mode & 2 ) {
		if( ostrain == org->strain ) {
			// can't write to my strain
			Kforth_Data_Stack_Push(kfm, -6);
			return;
		}
	}

	if( (write_mode & 4) == 0 ) {
		if( ostrain != org->strain ) {
			// can't write to other strains
			Kforth_Data_Stack_Push(kfm, -6);
			return;
		}
	}

	if( write_mode & 8 ) {
		if( gt == GT_SPORE ) {
			// can't write to spore
			Kforth_Data_Stack_Push(kfm, -7);
			return;
		}
	}

	if( (write_mode & 16) == 0 ) {
		if( gt == GT_CELL ) {
			// can't write to cells
			Kforth_Data_Stack_Push(kfm, -8);
			return;
		}
	}

	if( (write_mode & 1) == 0 ) {
		if( gt == GT_CELL ) {
			if( ugrid->u.cell->organism == cell->organism ) {
				// can't write to self
				Kforth_Data_Stack_Push(kfm, -9);
				return;
			}
		}
	}

	o_write_mode = u->strop[ ostrain ].write_mode;

	if( o_write_mode & 32 ) {
		// this strain cannot be written
		Kforth_Data_Stack_Push(kfm, -10);
		return;
	}

	//
	// All permission checks have occured.
	// We can proceed with the 'WRITE' operation.
	//
	// 1. send_mode & 64 will be checked for mutations
	// 2. instruction remap happens which can trigger a failure
	// 3. If Protected instructions are found, then the WRITE operation fails.
	//

	src_block = kfp->block[cbme];
	len = src_block[-1];

	new_block = (KFORTH_INTEGER*) CALLOC(len+1, sizeof(KFORTH_INTEGER)) + 1;
	ASSERT( new_block != NULL );

	new_block[-1] = len;

	num_cnt = 0;
	for(pc=0; pc < len; pc++) {
		new_block[pc] = src_block[pc];
		if( src_block[pc] & 0x8000 ) {
			num_cnt += 1;
		}
	}

	if( num_cnt != len ) {
		if( ostrain != org->strain ) {
			success = kforth_remap_instructions_cb(okfops, kfops, new_block);
			if( ! success ) {
				// remap instructions failed
				FREE( new_block-1 );

				Kforth_Data_Stack_Push(kfm, -11);
				return;
			}
		}

		if( kfm->loc.cb >= kfp->nprotected ) {
			for(pc=0; pc < len; pc++) {
				opcode = new_block[pc];
				if( (opcode & 0x8000) == 0) {
					if( opcode < okfops->nprotected ) {
						//
						// unprotected code cannot write a code block that
						// contains any protected instructions.
						//
						FREE( new_block-1 );

						Kforth_Data_Stack_Push(kfm, -12);
						return;
					}
				}
			}
		}
	}

	if( (write_mode & 64) == 0 ) {
		kfmo = &u->kfmo[org->strain];
		kforth_mutate_cb(okfops, kfmo, &u->er, &new_block);
	}

	FREE( okfp->block[cb]-1 );
	okfp->block[cb] = new_block;

	if( gt == GT_CELL ) {
		intflags = (o_write_mode >> 7) & 7;
		interrupt(ocell, intflags);
		ocell->organism->oflags |= ORGANISM_FLAG_READWRITE;
	}

	Kforth_Data_Stack_Push(kfm, len);
}

//
// take 'energy' from what is at (x,y) and give it to 'o'
//
static int take_energy(UNIVERSE *u, ORGANISM *o, int send_energy_mode, int x, int y, int energy)
{
	GRID_TYPE gt;
	UNIVERSE_GRID *ugrid;
	CELL *ocell;
	SPORE *ospore;
	int ostrain, oenergy, intflags;
	int o_send_energy_mode;

	ASSERT( u != NULL );
	ASSERT( o != NULL );
	ASSERT( energy > 0 );

	gt = Grid_GetPtr(u, x, y, &ugrid);

	if( gt == GT_SPORE ) {
		ospore = ugrid->u.spore;
		ostrain = ospore->strain;
		oenergy = ospore->energy;
	} else if( gt == GT_CELL ) {
		ocell = ugrid->u.cell;
		ostrain = ocell->organism->strain;
		oenergy = ocell->organism->energy;

		if( ocell->organism == o ) {
			// cannot take energy from self
			return 0;
		}
	} else {
		// no cell or spore to take energy from
		return 0;
	}

	if( (send_energy_mode & 2) && o->strain != ostrain ) {
		return 0;
	}

	if( (send_energy_mode & 8) && gt == GT_SPORE ) {
		return 0;
	}

	if( oenergy < energy ) {
		// not enough energy to be taken
		return 0;
	}

	if( gt == GT_SPORE ) {
		ospore->energy -= energy;
		o->energy += energy;

		if( ospore->energy == 0 ) {
			Grid_Clear(u, x, y);
			Spore_delete(ospore);
		}
	} else {
		ocell->organism->energy -= energy;
		o->energy += energy;

		o_send_energy_mode = u->strop[ ostrain ].send_energy_mode;

		intflags = (o_send_energy_mode >> 7) & 7;

		// interrupts
		interrupt(ocell, intflags);
	}

	return energy;
}

//
// give 'energy' from 'o' to what is at (x,y)
//
static int give_energy(UNIVERSE *u, ORGANISM *o, int send_energy_mode, int x, int y, int energy)
{
	GRID_TYPE gt;
	UNIVERSE_GRID *ugrid;
	CELL *ocell;
	SPORE *ospore;
	int ostrain, oenergy, intflags;
	int o_send_energy_mode;

	ASSERT( u != NULL );
	ASSERT( o != NULL );
	ASSERT( energy <= o->energy );

	gt = Grid_GetPtr(u, x, y, &ugrid);

	if( gt == GT_SPORE ) {
		ospore = ugrid->u.spore;
		ostrain = ospore->strain;
		oenergy = ospore->energy;
	} else if( gt == GT_CELL ) {
		ocell = ugrid->u.cell;
		ostrain = ocell->organism->strain;
		oenergy = ocell->organism->energy;

		if( ocell->organism == o ) {
			// cannot send energy to self
			return 0;
		}
	} else {
		// no cell or spore to give energy to
		return 0;
	}

	if( (send_energy_mode & 1) && o->strain != ostrain ) {
		return 0;
	}

	if( (send_energy_mode & 4) && gt == GT_SPORE ) {
		return 0;
	}

	if( gt == GT_SPORE ) {
		o->energy -= energy;
		ospore->energy += energy;

	} else {
		o->energy -= energy;
		ocell->organism->energy += energy;

		o_send_energy_mode = u->strop[ ostrain ].send_energy_mode;

		intflags = (o_send_energy_mode >> 4) & 7;

		// interrupts
		interrupt(ocell, intflags);
	}

	return energy;
}

/***********************************************************************
 * KFORTH NAME:		SEND-ENERGY
 * STACK BEHAVIOR:	(e x y -- rc)
 *
 * 3 values are on the stack. X offset and Y offset
 * identify a cell relative to this one. Send the
 * energy to that cell
 *
 *	BIT	MASK	Meaning when bit is 0	Meaning when bit is 1
 *	0	1	i can give energy to other strains					i cannot give energy to other strains
 *	1	2	i can take energy from other strains				i cannot take energy from other strains
 *	2	4	i can give energy to spores							i cannot give energy to spores
 *	3	8	i can take energy from spores						i cannot take energy from spores
 *	4	16	do not interrupt me, when somebody gives energy to me			interrupt me and bit 0 of the trap number shall be 1
 *	5	32	do not interrupt me, when somebody gives energy to me			interrupt me and bit 1 of the trap number shall be 1
 *	6	64	do not interrupt me, when somebody gives energy to me			interrupt me and bit 2 of the trap number shall be 1
 *	7	128	do not interrupt me, when somebody takes energy from me			interrupt me and bit 0 of the trap number shall be 1
 *	8	256	do not interrupt me, when somebody takes energy from me			interrupt me and bit 1 of the trap number shall be 1
 *	9	512	do not interrupt me, when somebody take energy from me			interrupt me and bit 2 of the trap number shall be 1
 *	10	1024	can only send energy to adjacent neighbors			can transmit energy across distances along normalized vector
 *
 */
static void Opcode_SEND_ENERGY(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	CELL *cell;
	ORGANISM *org;
	UNIVERSE *u;
	int xoffset, yoffset, x, y;
	KFORTH_INTEGER value;
	int energy, send_energy_mode, rc;
	CELL_CLIENT_DATA *cd;
	int look_mode;
	LOOK_RESULT res;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	org = cell->organism;
	u = cd->universe;
	send_energy_mode = u->strop[ org->strain ].send_energy_mode;

	value = Kforth_Data_Stack_Pop(kfm);
	yoffset = NORMALIZE_OFFSET(value);

	value = Kforth_Data_Stack_Pop(kfm);
	xoffset = NORMALIZE_OFFSET(value);

	value = Kforth_Data_Stack_Pop(kfm);
	energy = value;

	if( energy == 0 ) {
		// no energy to give/take
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	if( energy > org->energy )
	{
		// not enough energy to give
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	if( send_energy_mode & 1024 ) {
		// remote energy transfer
		if( energy < 0 ) {
			// cannot remotely take energy (can only give energy remotely)
			Kforth_Data_Stack_Push(kfm, 0);
			return;
		}

		look_mode = 1;
		look_along_line(u, cell, look_mode, xoffset, yoffset, &res);

		if( res.dist == 0 ) {
			Kforth_Data_Stack_Push(kfm, 0);
			return;
		}

		if( (res.what & (VISION_TYPE_CELL | VISION_TYPE_SPORE)) == 0 ) {
			Kforth_Data_Stack_Push(kfm, 0);
			return;
		}

		x = cell->x + xoffset * res.dist;
		y = cell->y + yoffset * res.dist;

	} else {
		// adjacent energy transfer
		x = cell->x + xoffset;
		y = cell->y + yoffset;

		if( x < 0 || x >= u->width ) {
			Kforth_Data_Stack_Push(kfm, 0);
			return;
		}

		if( y < 0 || y >= u->height ) {
			Kforth_Data_Stack_Push(kfm, 0);
			return;
		}
	}

	if( energy > 0 ) {
		rc = give_energy(u, org, send_energy_mode, x, y, energy);
	} else {
		rc = take_energy(u, org, send_energy_mode, x, y, -energy);
	}

	Kforth_Data_Stack_Push(kfm, rc);
}

/***********************************************************************
 * KFORTH NAME:		ENERGY
 * STACK BEHAVIOR:	( -- n)
 *
 * Push cell energy amount onto stack
 */
static void Opcode_ENERGY(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	CELL *cell;
	UNIVERSE *u;
	CELL_CLIENT_DATA *cd;
	KFORTH_INTEGER value;
	int energy;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	u = cd->universe;

	energy = cell->organism->energy;

	value = (energy < TOO_BIG) ? energy : TOO_BIG;

	Kforth_Data_Stack_Push(kfm, value);
}

/***********************************************************************
 * KFORTH NAME:		AGE
 * STACK BEHAVIOR:	( -- n)
 *
 * Push organism age onto stack
 */
static void Opcode_AGE(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	CELL *cell;
	CELL_CLIENT_DATA *cd;
	KFORTH_INTEGER value;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;

	value = ( cell->organism->age < TOO_BIG ) ? cell->organism->age : TOO_BIG;

	Kforth_Data_Stack_Push(kfm, value);
}

/***********************************************************************
 * KFORTH NAME:		NUM-CELLS
 * STACK BEHAVIOR:	( -- n)
 *
 * Get number of cells in organism onto stack.
 *
 * Push organism 'ncells' onto stack.
 */
static void Opcode_NUM_CELLS(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	CELL *cell;
	CELL_CLIENT_DATA *cd;
	KFORTH_INTEGER value;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;

	value = ( cell->organism->ncells < TOO_BIG ) ? cell->organism->ncells : TOO_BIG;

	Kforth_Data_Stack_Push(kfm, value);
}

/***********************************************************************
 * KFORTH NAME:		HAS-NEIGHBOR
 * STACK BEHAVIOR:	(x y -- s)
 *
 * Do we have a neighbor cell at x, y offsets?
 *
 * Offsets are NOT normalized.
 *
 */
static void Opcode_HAS_NEIGHBOR(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	CELL *cell, *c;
	UNIVERSE *u;
	int xoffset, yoffset;
	KFORTH_INTEGER value;
	CELL_CLIENT_DATA *cd;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	u = cd->universe;

	value = Kforth_Data_Stack_Pop(kfm);
	yoffset = (int)value;

	value = Kforth_Data_Stack_Pop(kfm);
	xoffset = (int)value;

	c = Cell_Neighbor(u, cell, xoffset, yoffset);
	if( c != NULL ) {
		Kforth_Data_Stack_Push(kfm, 1);
	} else {
		Kforth_Data_Stack_Push(kfm, 0);
	}
}

/***********************************************************************
 * KFORTH NAME:		DIST
 * STACK BEHAVIOR:	(x y -- dist)
 *
 * Calculate distance of the x,y coordinate. just applies a formula:
 *	max(abs(x),abs(y))
 *
 */
static void Opcode_DIST(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER x, y, dist;

	y = Kforth_Data_Stack_Pop(kfm);
	x = Kforth_Data_Stack_Pop(kfm);

	dist = MAX(ABS(x), ABS(y));

	Kforth_Data_Stack_Push(kfm, dist);
}

/***********************************************************************
 * KFORTH NAME:		CHOOSE
 * STACK BEHAVIOR:	(low high -- rnd)
 *
 * Return a random number between low..high. If range is empty nop.
 *
 */
static void Opcode_CHOOSE(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	UNIVERSE *u;
	int low, high;
	KFORTH_INTEGER value;
	CELL_CLIENT_DATA *cd;

	cd = (CELL_CLIENT_DATA*)client_data;
	u = cd->universe;

	value = Kforth_Data_Stack_Top(kfm);
	high = (int)value;

	value = Kforth_Data_Stack_2nd(kfm);
	low = (int)value;

	if( high < low ) {
		// invalid range, treat as NO OP.
		return;
	}

	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);

	value = (KFORTH_INTEGER) CHOOSE(&u->er, low, high);

	Kforth_Data_Stack_Push(kfm, value);
}

/***********************************************************************
 * KFORTH NAME:		RND
 * STACK BEHAVIOR:	( -- rnd)
 *
 * Return a random number between min_int .. max_int
 *
 */
static void Opcode_RND(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	UNIVERSE *u;
	int low, high;
	KFORTH_INTEGER value;
	CELL_CLIENT_DATA *cd;

	cd = (CELL_CLIENT_DATA*)client_data;
	u = cd->universe;

	low = -32768;
	high = 32767;

	value = (KFORTH_INTEGER) CHOOSE(&u->er, low, high);

	Kforth_Data_Stack_Push(kfm, value);
}

/***********************************************************************
 * KFORTH NAME:		POPULATION
 * STACK BEHAVIOR:	( -- pop)
 *
 * Returns the current population (# of organisms)
 */
static void Opcode_POPULATION(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	UNIVERSE *u;
	KFORTH_INTEGER value;
	CELL_CLIENT_DATA *cd;

	cd = (CELL_CLIENT_DATA*)client_data;
	u = cd->universe;

	value = (u->norganism < TOO_BIG) ? u->norganism : TOO_BIG;

	Kforth_Data_Stack_Push(kfm, value);
}

/***********************************************************************
 * KFORTH NAME:		POPULATION.S
 * STACK BEHAVIOR:	( -- pop)
 *
 * Returns the current population of my strain (# of organisms which belong to the same strain as the
 * organism running this instruction).
 */
static void Opcode_POPULATION_STRAIN(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	UNIVERSE *u;
	KFORTH_INTEGER value;
	CELL_CLIENT_DATA *cd;
	CELL *cell;
	int strain;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	u = cd->universe;
	strain = cell->organism->strain;

	value = (u->strpop[strain] < TOO_BIG) ? u->strpop[strain] : TOO_BIG;

	Kforth_Data_Stack_Push(kfm, value);
}

/***********************************************************************
 * KFORTH NAME:		GPS
 * STACK BEHAVIOR:	( -- x y)
 *
 * Returns the current position.
 */
static void Opcode_GPS(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;
	CELL_CLIENT_DATA *cd;
	CELL *cell;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;

	value = (cell->x < TOO_BIG) ? cell->x : TOO_BIG;
	Kforth_Data_Stack_Push(kfm, value);

	value = (cell->y < TOO_BIG) ? cell->y : TOO_BIG;
	Kforth_Data_Stack_Push(kfm, value);
}

/***********************************************************************
 * KFORTH NAME:		NEIGHBORS
 * STACK BEHAVIOR:	( -- mask)
 *
 * Returns what cells have a neigbor
 *		bit 0 is north
 *		rest of bits assigned clock wise.
 *		bit 7 is north west
 */
static void Opcode_NEIGHBORS(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	CELL_CLIENT_DATA *cd;
	CELL *cell, *c;
	ORGANISM *o;
	int mask;
	UNIVERSE *u;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	o = cell->organism;
	u = cd->universe;

	//                    +----+----+----+----+----+----+----+----+
	// Set bits in order: | nw | w  | sw | s  | se | e  | ne | n  |
	//                    +----+----+----+----+----+----+----+----+

	mask = 0;

	c = grid_has_our_cell(u, o, cell->x + -1, cell->y + 1); // nw
	if( c != NULL ) {
		mask |= 1;
	}
	mask <<= 1;

	c = grid_has_our_cell(u, o, cell->x + -1, cell->y + 0); // w
	if( c != NULL ) {
		mask |= 1;
	}
	mask <<= 1;

	c = grid_has_our_cell(u, o, cell->x + -1, cell->y + -1); // sw
	if( c != NULL ) {
		mask |= 1;
	}
	mask <<= 1;

	c = grid_has_our_cell(u, o, cell->x + 0, cell->y + -1); // s
	if( c != NULL ) {
		mask |= 1;
	}
	mask <<= 1;

	c = grid_has_our_cell(u, o, cell->x + 1, cell->y + -1); // se
	if( c != NULL ) {
		mask |= 1;
	}
	mask <<= 1;

	c = grid_has_our_cell(u, o, cell->x + 1, cell->y + 0); // e
	if( c != NULL ) {
		mask |= 1;
	}
	mask <<= 1;

	c = grid_has_our_cell(u, o, cell->x + 1, cell->y + 1); // ne
	if( c != NULL ) {
		mask |= 1;
	}
	mask <<= 1;

	c = grid_has_our_cell(u, o, cell->x + 0, cell->y + 1); // n
	if( c != NULL ) {
		mask |= 1;
	}

	Kforth_Data_Stack_Push(kfm, mask);
}

/***********************************************************************
 * KFORTH NAME:		G0
 * STACK BEHAVIOR:	( -- value)
 *
 * Returns value of universe wide global variable.
 */
static void Opcode_G0(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;
	CELL_CLIENT_DATA *cd;
	CELL *cell;
	UNIVERSE *u;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	u = cd->universe;

	value = u->G0;
	Kforth_Data_Stack_Push(kfm, value);
}

/***********************************************************************
 * KFORTH NAME:		G0!
 * STACK BEHAVIOR:	(value -- )
 *
 * set value of universe-wide global variable.
 */
static void Opcode_SET_G0(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;
	CELL_CLIENT_DATA *cd;
	CELL *cell;
	UNIVERSE *u;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	u = cd->universe;

	value = Kforth_Data_Stack_Pop(kfm);
	u->G0 = value;
}

/***********************************************************************
 * KFORTH NAME:		S0
 * STACK BEHAVIOR:	( -- value)
 *
 * Returns value of strain wide global variable.
 */
static void Opcode_S0(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;
	CELL_CLIENT_DATA *cd;
	CELL *cell;
	UNIVERSE *u;
	int strain;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	u = cd->universe;
	strain = cell->organism->strain;

	value = u->S0[strain];
	Kforth_Data_Stack_Push(kfm, value);
}

/***********************************************************************
 * KFORTH NAME:		S0!
 * STACK BEHAVIOR:	(value -- )
 *
 * set value of strain-wide global variable.
 */
static void Opcode_SET_S0(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;
	CELL_CLIENT_DATA *cd;
	CELL *cell;
	UNIVERSE *u;
	int strain;

	cd = (CELL_CLIENT_DATA*)client_data;
	cell = cd->cell;
	u = cd->universe;
	strain = cell->organism->strain;

	value = Kforth_Data_Stack_Pop(kfm);
	u->S0[strain] = value;
}

/***********************************************************************
 * KFORTH NAME:		KEY-PRESS
 * STACK BEHAVIOR:	( -- key)
 *
 * get keyboard character or 0.
 */
static void Opcode_KEY_PRESS(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;
	CELL_CLIENT_DATA *cd;
	UNIVERSE *u;

	cd = (CELL_CLIENT_DATA*)client_data;
	u = cd->universe;

	value = u->key;
	Kforth_Data_Stack_Push(kfm, value);
}

/***********************************************************************
 *
 * UNIVERSE API's for Key Press setting and Mouse Position setting
 *
 */

/*
 * Visit all cells in the UNIVERSE. If it belongs to
 * a strain which needs to be interrupted, then
 * interrupt the cell.
 *
 * flag = 0 		use key_press change interrupt handler
 * flag = 1 		use mouse_pos changed interrupt handler
 */
static void interrupt_all(UNIVERSE *u, int flag)
{
	CELL *cell;
	int key_press_mode, strain, intflags, needed;
	int i;

	/*
 	 * Quick check to see if interrupts are enabled
	 * for keypress events.
	 */
	needed = 0;
	for(i=0; i < 8; i++) {
		if( u->strop[i].enabled ) {
			key_press_mode = u->strop[i].key_press_mode;
			if( flag == 0 )
				intflags = (key_press_mode >> 0) & 7;
			else
				intflags = (key_press_mode >> 3) & 7;

			if( intflags != 0 )
				needed += 1;
		}
	}

	if( ! needed )
		return;

	for(cell=u->cells; cell; cell=cell->u_next) {
		strain = cell->organism->strain;
		key_press_mode = u->strop[ strain ].key_press_mode;
		if( flag == 0 ) {
			intflags = (key_press_mode >> 0) & 7;
		} else {
			intflags = (key_press_mode >> 3) & 7;
		}
		interrupt(cell, intflags);
	}
}

/*
 * If key changed, then interrupt any cells which are configured to be interrupted
 */
void Universe_SetKey(UNIVERSE *u, int key)
{
	int changed;

	ASSERT( u != NULL );
	ASSERT( key >= ' ' && key <= '~' );

	changed = 0;
	if( u->key != key ) {
		changed = 1;
	}

	u->key = key;

	if( changed ) {
		interrupt_all(u, 0);
	}
}

/*
 * Clear the key. if the key changed, then trigger interrupt.
 */
void Universe_ClearKey(UNIVERSE *u)
{
	int changed;

	ASSERT( u != NULL );

	changed = 0;
	if( u->key != 0 ) {
		changed = 1;
	}

	u->key = 0;

	if( changed ) {
		interrupt_all(u, 0);
	}
}

void Universe_SetMouse(UNIVERSE *u, int x, int y)
{
	int changed;

	ASSERT( u != NULL );
	ASSERT( x >= 0 && x < u->width );
	ASSERT( y >= 0 && y < u->height );

	changed = 0;
	if( u->mouse_x != x || u->mouse_y != y ) {
		changed = 1;
	}

	u->mouse_x = x;
	u->mouse_y = y;

	if( changed ) {
		interrupt_all(u, 1);
	}
}

void Universe_ClearMouse(UNIVERSE *u)
{
	int changed;

	ASSERT( u != NULL );

	changed = 0;
	if( u->mouse_x != -1 || u->mouse_y != -1 ) {
		changed = 1;
	}

	u->mouse_x = -1;
	u->mouse_y = -1;

	if( changed ) {
		interrupt_all(u, 1);
	}
}

/***********************************************************************
 * KFORTH NAME:		MOUSE-POS
 * STACK BEHAVIOR:	( -- x y)
 *
 * get keyboard character or 0.
 */
static void Opcode_MOUSE_POS(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;
	CELL_CLIENT_DATA *cd;
	UNIVERSE *u;

	cd = (CELL_CLIENT_DATA*)client_data;
	u = cd->universe;

	value = u->mouse_x;
	Kforth_Data_Stack_Push(kfm, value);

	value = u->mouse_y;
	Kforth_Data_Stack_Push(kfm, value);
}

/* ********************************************************************** */

/*
 * Get a neighboring cell for 'cell'. Return
 * NULL if cell has no neighbor.
 *
 */
CELL *Cell_Neighbor(UNIVERSE *u, CELL *cell, int xoffset, int yoffset)
{
	UNIVERSE_GRID grid;
	int x, y;
	GRID_TYPE type;

	ASSERT( cell != NULL );

	x = cell->x + xoffset;
	y = cell->y + yoffset;

	if( x < 0 || x >= u->width )
		return NULL;

	if( y < 0 || y >= u->height )
		return NULL;

	type = Grid_Get(u, x, y, &grid);
	if( type == GT_CELL ) {
		if( grid.u.cell->organism == cell->organism ) {
			return grid.u.cell;
		}
	}

	return NULL;
}

void Cell_delete(CELL *c)
{
	ASSERT( c != NULL );

	kforth_machine_deinit(&c->kfm);

	FREE(c);
}

static int odor(UNIVERSE *u, int x, int y)
{
	UNIVERSE_GRID ugrid;

	if( x < 0 || x >= u->width )
		return 0;

	if( y < 0 || y >= u->height )
		return 0;

	Grid_Get(u, x, y, &ugrid);
	return ugrid.odor;
}

/***********************************************************************
 *
 * The GUI wil want to perform a virtual "look" operation to display
 * the current visual/sound/odor information for a cell.
 *
 * This looks in all 8 direction surrounding cell 'c' and
 * stored the 'what' and 'dist' values in 'cvd'.
 *
 * Also collects the mood/message registers for any cell seen
 * Also collects the odor values surrounding the cell
 *
 * The returns coordinates are 0 - north, ...., 7 - north west
 *
 * Flipped the y-axis for return to caller on Mac. This gives
 * the correct orgientation.
 *
 */
void Universe_CellSensoryData(UNIVERSE *u, CELL *cell, CELL_SENSE_DATA *csd)
{
	static const int xoffset[8] = {  0,  1,  1,  1,  0, -1, -1, -1 };
	static const int yoffset[8] = {  1,  1,  0, -1, -1, -1,  0,  1 };

	ORGANISM *o;
	int look_mode;
	LOOK_RESULT res;
	int i, x, y;

	ASSERT( cell != NULL );
	ASSERT( csd != NULL );

	o = cell->organism;
	look_mode = u->strop[o->strain].look_mode;

	for(i=0; i < 8; i++) {
		x = cell->x + xoffset[i];
		y = cell->y + yoffset[i];

		look_along_line(u, cell,  look_mode,  xoffset[i], yoffset[i], &res);

		csd->dirs[i].what = res.what;
		csd->dirs[i].dist = res.dist;
		csd->dirs[i].size = res.size;
		csd->dirs[i].energy = res.energy;
		csd->dirs[i].mood = res.mood;
		csd->dirs[i].message = res.message;
		csd->dirs[i].strain = res.strain;

		x = cell->x + xoffset[i];
		y = cell->y + yoffset[i];
		csd->dirs[i].odor = odor(u, x, y);
	}

	/*
	 * Odor at (0,0)
	 */
	csd->odor = odor(u, cell->x, cell->y);
}

/***********************************************************************
 * Returns a static copy of the MASTER LIST of all CELL instructions.
 *
 * Strains can selectively remove instructions from their kforth program,
 * but they choose from this list.
 *
 * This table contains the ORGANISM/CELL operations as well as the core KFORTH operations.
 *
 * When a new instruction needs to be added, add it to this list.
 *
 * This is a "once" function meaning it computes the 
 * KFORTH_OPERATIONS table once, then returns it
 * on subsequent calls to this function.
 *
 */
KFORTH_OPERATIONS *EvolveOperations(void)
{
	static int first_time = 1;
	static KFORTH_OPERATIONS kfops;

	int i;

	if( first_time ) {
		first_time = 0;
		kforth_ops_init(&kfops);
		kforth_ops_add(&kfops,	"CMOVE",			2, 1,	Opcode_CMOVE);
		kforth_ops_add(&kfops,	"OMOVE",			2, 1,	Opcode_OMOVE);
		kforth_ops_add(&kfops,	"ROTATE",			1, 1,	Opcode_ROTATE);
		kforth_ops_add(&kfops,	"EAT",				2, 1,	Opcode_EAT);
		kforth_ops_add(&kfops,	"MAKE-SPORE",		3, 1,	Opcode_MAKE_SPORE);
		kforth_ops_add(&kfops,	"MAKE-ORGANIC",		3, 1,	Opcode_MAKE_ORGANIC);
		kforth_ops_add(&kfops,	"MAKE-BARRIER",		2, 1,	Opcode_MAKE_BARRIER);
		kforth_ops_add(&kfops,	"GROW",				2, 1,	Opcode_GROW);
		kforth_ops_add(&kfops,	"GROW.CB",			3, 1,	Opcode_GROW_CB);
		kforth_ops_add(&kfops,	"CSHIFT",			2, 1,	Opcode_CSHIFT);
		kforth_ops_add(&kfops,	"EXUDE",			3, 0,	Opcode_EXUDE);
		kforth_ops_add(&kfops,	"LOOK",				2, 2,	Opcode_LOOK);
		kforth_ops_add(&kfops,	"NEAREST",			1, 2,	Opcode_NEAREST);
		kforth_ops_add(&kfops,	"FARTHEST",			1, 2,	Opcode_FARTHEST);
		kforth_ops_add(&kfops,	"SIZE",				2, 2,	Opcode_SIZE);
		kforth_ops_add(&kfops,	"BIGGEST",			1, 2,	Opcode_BIGGEST);
		kforth_ops_add(&kfops,	"SMALLEST",			1, 2,	Opcode_SMALLEST);
		kforth_ops_add(&kfops,	"TEMPERATURE",		2, 2,	Opcode_TEMPERATURE);
		kforth_ops_add(&kfops,	"HOTTEST",			1, 2,	Opcode_HOTTEST);
		kforth_ops_add(&kfops,	"COLDEST",			1, 2,	Opcode_COLDEST);
		kforth_ops_add(&kfops,	"SMELL",			2, 1,	Opcode_SMELL);
		kforth_ops_add(&kfops,	"MOOD",				2, 1,	Opcode_MOOD);
		kforth_ops_add(&kfops,	"MOOD!",			1, 0,	Opcode_SET_MOOD);
		kforth_ops_add(&kfops,	"BROADCAST",		1, 0,	Opcode_BROADCAST);
		kforth_ops_add(&kfops,	"SEND",				3, 0,	Opcode_SEND);
		kforth_ops_add(&kfops,	"RECV",				0, 1,	Opcode_RECV);
		kforth_ops_add(&kfops,	"ENERGY",			0, 1,	Opcode_ENERGY);
		kforth_ops_add(&kfops,	"AGE",				0, 1,	Opcode_AGE);
		kforth_ops_add(&kfops,	"NUM-CELLS",		0, 1,	Opcode_NUM_CELLS);
		kforth_ops_add(&kfops,	"HAS-NEIGHBOR",		2, 1,	Opcode_HAS_NEIGHBOR);
		kforth_ops_add(&kfops,	"DIST",				2, 1,	Opcode_DIST);
		kforth_ops_add(&kfops,	"CHOOSE",			2, 1,	Opcode_CHOOSE);
		kforth_ops_add(&kfops,	"RND",				0, 1,	Opcode_RND);
		kforth_ops_add(&kfops,	"SEND-ENERGY",		3, 1,	Opcode_SEND_ENERGY);
		kforth_ops_add(&kfops,	"POPULATION",		0, 1,	Opcode_POPULATION);
		kforth_ops_add(&kfops,	"POPULATION.S",		0, 1,	Opcode_POPULATION_STRAIN);
		kforth_ops_add(&kfops,	"GPS",				0, 2,	Opcode_GPS);
		kforth_ops_add(&kfops,	"NEIGHBORS",		0, 1,	Opcode_NEIGHBORS);
		kforth_ops_add(&kfops,	"SHOUT",			1, 1,	Opcode_SHOUT);
		kforth_ops_add(&kfops,	"LISTEN",			2, 2,	Opcode_LISTEN);
		kforth_ops_add(&kfops,	"SAY",				3, 1,	Opcode_SAY);
		kforth_ops_add(&kfops,	"READ",				4, 1,	Opcode_READ);
		kforth_ops_add(&kfops,	"WRITE",			4, 1,	Opcode_WRITE);
		kforth_ops_add(&kfops,	"KEY-PRESS",		0, 1,	Opcode_KEY_PRESS);
		kforth_ops_add(&kfops,	"MOUSE-POS",		0, 2,	Opcode_MOUSE_POS);
		kforth_ops_add(&kfops,	"SPAWN",			5, 1,	Opcode_SPAWN);
		kforth_ops_add(&kfops,	"S0",				0, 1,	Opcode_S0);
		kforth_ops_add(&kfops,	"S0!",				1, 0,	Opcode_SET_S0);
		kforth_ops_add(&kfops,	"G0",				0, 1,	Opcode_G0);
		kforth_ops_add(&kfops,	"G0!",				1, 0,	Opcode_SET_G0);

		// give each instruction entry a unique 'key' number which 
		// is important for remap_instructions() to work.
		// - these numbers must be same in all instruction tables.
		// - these numbers must always be increasing in all instruction tables.
		for(i=0; i < kfops.count; i++) {
			kfops.table[i].key = 1000 + i;
		}
	}

	return &kfops;
}
