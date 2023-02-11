/*
 * Copyright (c) 2022 Ken Stauffer
 */


/***********************************************************************
 * Utility functions for the kforth interpreter client
 *
 * Provide working RND, CHOOSE, DIST functions instead of dummy versions.
 *
 */
#include "evolve_simulator.h"
#include "evolve_simulator_private.h"

/*
 * Pick random number between 'a' and 'b' (includes 'a' and 'b')
 */
#define CHOOSE(er,a,b)	( (sim_random(er) % ((b)-(a)+1) ) + (a) )
#define ABS(x)		((x<0) ? -x : x)
#define MAX(x,y)	((x>y) ?  x : y)


/***********************************************************************
 * KFORTH NAME:		DIST
 * STACK BEHAVIOR:	(x y -- dist)
 *
 *
 * Calculate distance of the x,y coordinate. just applies a formula:
 *	max(abs(x),abs(y))
 *
 */
static void KforthInterpreter_DIST(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
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
static void KforthInterpreter_CHOOSE(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	EVOLVE_RANDOM *er;
	int low, high;
	KFORTH_INTEGER value;
	KFORTH_INTERPRETER_CLIENT_DATA *cd;

	cd = (KFORTH_INTERPRETER_CLIENT_DATA*)client_data;
	er = &cd->er;

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

	value = (KFORTH_INTEGER) CHOOSE(er, low, high);

	Kforth_Data_Stack_Push(kfm, value);
}

/***********************************************************************
 * KFORTH NAME:		RND
 * STACK BEHAVIOR:	( -- rnd)
 *
 * Return a random number between min_int .. max_int
 *
 */
static void KforthInterpreter_RND(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	EVOLVE_RANDOM *er;
	int low, high;
	KFORTH_INTEGER value;
	KFORTH_INTERPRETER_CLIENT_DATA *cd;

	cd = (KFORTH_INTERPRETER_CLIENT_DATA*)client_data;
	er = &cd->er;

	low = -32768;
	high = 32767;

	value = (KFORTH_INTEGER) CHOOSE(er, low, high);

	Kforth_Data_Stack_Push(kfm, value);
}


/*
 * Replace instructions that can be used inside of
 * the kforth interpreter without requiring
 * a UNIVERSE or CELL object.
 */
void KforthInterpreter_Replace_Instructions(KFORTH_OPERATIONS *kfops)
{
	int i;

	i = kforth_ops_find(kfops, "RND");
	ASSERT( i >= 0 );
	kfops->table[i].func = KforthInterpreter_RND;

	i = kforth_ops_find(kfops, "DIST");
	ASSERT( i >= 0 );
	kfops->table[i].func = KforthInterpreter_DIST;

	i = kforth_ops_find(kfops, "CHOOSE");
	ASSERT( i >= 0 );
	kfops->table[i].func = KforthInterpreter_CHOOSE;
}
