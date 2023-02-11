/*
 * cell_noops.cpp
 *
 * This file automatically generated from cell_noops.tmpl and cell_noops.tt
 * (processed by render_code.py)
 *
 * This file contains dummy cell functions for use by the Kforth Interpreter.
 *
 */
#include "evolve_simulator.h"
#include "evolve_simulator_private.h"

//
// 'CMOVE', type: INTERACT, usage: ( x y -- r )
//
static void dummy_CMOVE(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'OMOVE', type: INTERACT, usage: ( x y -- n )
//
static void dummy_OMOVE(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'ROTATE', type: INTERACT, usage: ( n -- r )
//
static void dummy_ROTATE(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'EAT', type: INTERACT, usage: ( x y -- n )
//
static void dummy_EAT(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'SEND-ENERGY', type: INTERACT, usage: (e x y -- rc)
//
static void dummy_SEND_ENERGY(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'MAKE-SPORE', type: INTERACT, usage: ( x y e -- r )
//
static void dummy_MAKE_SPORE(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'GROW', type: INTERACT, usage: ( x y -- r )
//
static void dummy_GROW(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'LOOK', type: VISION, usage: ( x y -- what dist )
//
static void dummy_LOOK(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Push(kfm, 0);
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'NEAREST', type: VISION, usage: ( mask -- x y )
//
static void dummy_NEAREST(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Push(kfm, 0);
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'FARTHEST', type: VISION, usage: ( mask -- x y )
//
static void dummy_FARTHEST(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Push(kfm, 0);
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'MOOD', type: COMMS, usage: ( x y -- m )
//
static void dummy_MOOD(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'MOOD!', type: COMMS, usage: ( m -- )
//
static void dummy_SET_MOOD(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
}

//
// 'BROADCAST', type: COMMS, usage: ( m -- )
//
static void dummy_BROADCAST(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
}

//
// 'SEND', type: COMMS, usage: ( m x y -- )
//
static void dummy_SEND(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
}

//
// 'RECV', type: COMMS, usage: ( -- m )
//
static void dummy_RECV(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'ENERGY', type: QUERY, usage: ( -- e )
//
static void dummy_ENERGY(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'AGE', type: QUERY, usage: ( -- a )
//
static void dummy_AGE(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'NUM-CELLS', type: QUERY, usage: ( -- n )
//
static void dummy_NUM_CELLS(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'HAS-NEIGHBOR', type: QUERY, usage: ( x y -- r )
//
static void dummy_HAS_NEIGHBOR(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'MAKE-ORGANIC', type: INTERACT, usage: (x y e -- s)
//
static void dummy_MAKE_ORGANIC(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'GROW.CB', type: INTERACT, usage: (x y cb -- r)
//
static void dummy_GROW_CB(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'CSHIFT', type: INTERACT, usage: (x y -- r)
//
static void dummy_CSHIFT(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'SPAWN', type: INTERACT, usage: (x y e strain cb -- r)
//
static void dummy_SPAWN(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'MAKE-BARRIER', type: INTERACT, usage: (x y -- r)
//
static void dummy_MAKE_BARRIER(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'SIZE', type: VISION, usage: (x y -- size dist)
//
static void dummy_SIZE(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Push(kfm, 0);
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'BIGGEST', type: VISION, usage: (mask -- x y)
//
static void dummy_BIGGEST(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Push(kfm, 0);
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'SMALLEST', type: VISION, usage: (mask -- x y)
//
static void dummy_SMALLEST(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Push(kfm, 0);
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'TEMPERATURE', type: VISION, usage: (x y -- energy dist)
//
static void dummy_TEMPERATURE(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Push(kfm, 0);
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'HOTTEST', type: VISION, usage: (mask -- x y)
//
static void dummy_HOTTEST(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Push(kfm, 0);
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'COLDEST', type: VISION, usage: (mask -- x y)
//
static void dummy_COLDEST(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Push(kfm, 0);
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'SHOUT', type: COMMS, usage: (m -- r)
//
static void dummy_SHOUT(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'SAY', type: COMMS, usage: (m x y -- dist)
//
static void dummy_SAY(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'LISTEN', type: COMMS, usage: (x y -- mood dist)
//
static void dummy_LISTEN(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Push(kfm, 0);
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'READ', type: COMMS, usage: (x y cb cbme -- rc)
//
static void dummy_READ(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'WRITE', type: COMMS, usage: (x y cb cbme -- rc)
//
static void dummy_WRITE(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'EXUDE', type: INTERACT, usage: (value x y -- )
//
static void dummy_EXUDE(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
}

//
// 'SMELL', type: INTERACT, usage: (x y -- value)
//
static void dummy_SMELL(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'G0', type: UNIVERSE, usage: ( -- value)
//
static void dummy_G0(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'G0!', type: UNIVERSE, usage: (value -- )
//
static void dummy_SET_G0(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
}

//
// 'S0', type: UNIVERSE, usage: ( -- value)
//
static void dummy_S0(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'S0!', type: UNIVERSE, usage: (value -- )
//
static void dummy_SET_S0(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
}

//
// 'NEIGHBORS', type: QUERY, usage: ( -- mask)
//
static void dummy_NEIGHBOR(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'POPULATION', type: UNIVERSE, usage: ( -- pop)
//
static void dummy_POPULATION(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'POPULATION.S', type: UNIVERSE, usage: ( -- pop)
//
static void dummy_POPULATION_STRAIN(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'GPS', type: QUERY, usage: ( -- x y)
//
static void dummy_GPS(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Push(kfm, 0);
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'KEY-PRESS', type: UNIVERSE, usage: ( -- key)
//
static void dummy_KEY_PRESS(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'MOUSE-POS', type: UNIVERSE, usage: ( -- x y)
//
static void dummy_MOUSE_POS(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Push(kfm, 0);
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'DIST', type: MISC, usage: (x y -- dist)
//
static void dummy_DIST(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'CHOOSE', type: MISC, usage: (min max -- rnd)
//
static void dummy_CHOOSE(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// 'RND', type: MISC, usage: ( -- rnd)
//
static void dummy_RND(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Push(kfm, 0);
}

//
// Returns a list of CORE instructions (which are real and operate on the kforth machine)
// And the complete set of CELL instructions, except they are NO-OP's. They will
// handle args, but will do nothing
//
KFORTH_OPERATIONS *DummyEvolveOperations(void)
{
	static int first_time = 1;
	static KFORTH_OPERATIONS kfops;

	int i;

	if( first_time ) {
		first_time = 0;
		kforth_ops_init(&kfops);
		kforth_ops_add(&kfops,	"CMOVE", 2, 1, dummy_CMOVE);
		kforth_ops_add(&kfops,	"OMOVE", 2, 1, dummy_OMOVE);
		kforth_ops_add(&kfops,	"ROTATE", 1, 1, dummy_ROTATE);
		kforth_ops_add(&kfops,	"EAT", 2, 1, dummy_EAT);
		kforth_ops_add(&kfops,	"SEND-ENERGY", 3, 1, dummy_SEND_ENERGY);
		kforth_ops_add(&kfops,	"MAKE-SPORE", 3, 1, dummy_MAKE_SPORE);
		kforth_ops_add(&kfops,	"GROW", 2, 1, dummy_GROW);
		kforth_ops_add(&kfops,	"LOOK", 2, 2, dummy_LOOK);
		kforth_ops_add(&kfops,	"NEAREST", 1, 2, dummy_NEAREST);
		kforth_ops_add(&kfops,	"FARTHEST", 1, 2, dummy_FARTHEST);
		kforth_ops_add(&kfops,	"MOOD", 2, 1, dummy_MOOD);
		kforth_ops_add(&kfops,	"MOOD!", 1, 0, dummy_SET_MOOD);
		kforth_ops_add(&kfops,	"BROADCAST", 1, 0, dummy_BROADCAST);
		kforth_ops_add(&kfops,	"SEND", 3, 0, dummy_SEND);
		kforth_ops_add(&kfops,	"RECV", 0, 1, dummy_RECV);
		kforth_ops_add(&kfops,	"ENERGY", 0, 1, dummy_ENERGY);
		kforth_ops_add(&kfops,	"AGE", 0, 1, dummy_AGE);
		kforth_ops_add(&kfops,	"NUM-CELLS", 0, 1, dummy_NUM_CELLS);
		kforth_ops_add(&kfops,	"HAS-NEIGHBOR", 2, 1, dummy_HAS_NEIGHBOR);
		kforth_ops_add(&kfops,	"MAKE-ORGANIC", 3, 1, dummy_MAKE_ORGANIC);
		kforth_ops_add(&kfops,	"GROW.CB", 3, 1, dummy_GROW_CB);
		kforth_ops_add(&kfops,	"CSHIFT", 2, 1, dummy_CSHIFT);
		kforth_ops_add(&kfops,	"SPAWN", 5, 1, dummy_SPAWN);
		kforth_ops_add(&kfops,	"MAKE-BARRIER", 2, 1, dummy_MAKE_BARRIER);
		kforth_ops_add(&kfops,	"SIZE", 2, 2, dummy_SIZE);
		kforth_ops_add(&kfops,	"BIGGEST", 1, 2, dummy_BIGGEST);
		kforth_ops_add(&kfops,	"SMALLEST", 1, 2, dummy_SMALLEST);
		kforth_ops_add(&kfops,	"TEMPERATURE", 2, 2, dummy_TEMPERATURE);
		kforth_ops_add(&kfops,	"HOTTEST", 1, 2, dummy_HOTTEST);
		kforth_ops_add(&kfops,	"COLDEST", 1, 2, dummy_COLDEST);
		kforth_ops_add(&kfops,	"SHOUT", 1, 1, dummy_SHOUT);
		kforth_ops_add(&kfops,	"SAY", 3, 1, dummy_SAY);
		kforth_ops_add(&kfops,	"LISTEN", 2, 2, dummy_LISTEN);
		kforth_ops_add(&kfops,	"READ", 4, 1, dummy_READ);
		kforth_ops_add(&kfops,	"WRITE", 4, 1, dummy_WRITE);
		kforth_ops_add(&kfops,	"EXUDE", 3, 0, dummy_EXUDE);
		kforth_ops_add(&kfops,	"SMELL", 2, 1, dummy_SMELL);
		kforth_ops_add(&kfops,	"G0", 0, 1, dummy_G0);
		kforth_ops_add(&kfops,	"G0!", 1, 0, dummy_SET_G0);
		kforth_ops_add(&kfops,	"S0", 0, 1, dummy_S0);
		kforth_ops_add(&kfops,	"S0!", 1, 0, dummy_SET_S0);
		kforth_ops_add(&kfops,	"NEIGHBORS", 0, 1, dummy_NEIGHBOR);
		kforth_ops_add(&kfops,	"POPULATION", 0, 1, dummy_POPULATION);
		kforth_ops_add(&kfops,	"POPULATION.S", 0, 1, dummy_POPULATION_STRAIN);
		kforth_ops_add(&kfops,	"GPS", 0, 2, dummy_GPS);
		kforth_ops_add(&kfops,	"KEY-PRESS", 0, 1, dummy_KEY_PRESS);
		kforth_ops_add(&kfops,	"MOUSE-POS", 0, 2, dummy_MOUSE_POS);
		kforth_ops_add(&kfops,	"DIST", 2, 1, dummy_DIST);
		kforth_ops_add(&kfops,	"CHOOSE", 2, 1, dummy_CHOOSE);
		kforth_ops_add(&kfops,	"RND", 0, 1, dummy_RND);

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