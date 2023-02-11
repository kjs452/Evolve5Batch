/*
 * Copyright (c) 2022 Ken Stauffer
 */

/***********************************************************************
 * EXECUTE KFORTH PROGRAMS
 *
 *
 */
#include "evolve_simulator.h"
#include "evolve_simulator_private.h"
#include <ctype.h>

// is the machine terminated (halted)?
int kforth_machine_terminated(KFORTH_MACHINE *kfm)
{
	return kfm->loc.cb == -1;
}

// mark machine as terminated (halted)
void kforth_machine_terminate(KFORTH_MACHINE *kfm)
{
	ASSERT( ! Kforth_Machine_Terminated(kfm) );

	kfm->loc.cb = -1;
}

void kforth_data_stack_push(KFORTH_MACHINE *kfm, KFORTH_INTEGER value)
{
	ASSERT( kfm != NULL );
	ASSERT( kfm->dsp < KF_MAX_DATA );

	kfm->data_stack[ kfm->dsp ] = value;
	kfm->dsp++;
}

KFORTH_INTEGER kforth_data_stack_pop(KFORTH_MACHINE *kfm)
{
	ASSERT( kfm != NULL );
	ASSERT( kfm->dsp > 0 );

	kfm->dsp--;
	return kfm->data_stack[ kfm->dsp ];
}

void kforth_call_stack_push(KFORTH_MACHINE *kfm, int cb, int pc)
{
	KFORTH_LOC loc;

	ASSERT( kfm != NULL );
	ASSERT( kfm->csp < KF_MAX_CALL );

	loc.cb = cb;
	loc.pc = pc;

	kfm->call_stack[ kfm->csp ] = loc;
	kfm->csp++;
}

/***********************************************************************
 * Initialize a kforth_machine for blank state
 *
 */
void kforth_machine_init(KFORTH_MACHINE *kfm)
{
	ASSERT( kfm != NULL );
	
	memset(kfm, 0, sizeof(KFORTH_MACHINE));
}

/***********************************************************************
 * Create a kforth machine object.
 * Stacks are empty, and execution is setup to
 * begin at code block 0 (cb=0), instruction 0 (pc=0).
 *
 */
KFORTH_MACHINE *kforth_machine_make()
{
	KFORTH_MACHINE *kfm;

	kfm = (KFORTH_MACHINE*) CALLOC(1, sizeof(KFORTH_MACHINE));
	ASSERT( kfm != NULL );

	return kfm;
}

/*
 * De-allocate subobjects inside of 'kfm'
 */
void kforth_machine_deinit(KFORTH_MACHINE *kfm)
{
	// nothing to deinit
}

/***********************************************************************
 * Delete a KFORTH machine. Delete any stack space occupied
 * by the machine.
 *
 */
void kforth_machine_delete(KFORTH_MACHINE *kfm)
{
	ASSERT( kfm != NULL );

	kforth_machine_deinit(kfm);

	FREE( kfm );
}

/***********************************************************************
 * Make a copy of 'kfm' in 'kfm2'
 * kfm2 is assumed to be empty.
 *
 */
void kforth_machine_copy2(KFORTH_MACHINE *kfm, KFORTH_MACHINE *kfm2)
{
	ASSERT( kfm != NULL );
	ASSERT( kfm2 != NULL );

	*kfm2		= *kfm;
}

/***********************************************************************
 * Return a complete copy of 'kfm' (including stacks).
 *
 * NOTE: The returned copy refers to the same 'client_data' and 'program' as 'kfm'.
 *
 */
KFORTH_MACHINE *kforth_machine_copy(KFORTH_MACHINE *kfm)
{
	KFORTH_MACHINE *kfm2;

	ASSERT( kfm != NULL );

	kfm2 = (KFORTH_MACHINE *) CALLOC(1, sizeof(KFORTH_MACHINE));
	ASSERT( kfm2 != NULL );

	kforth_machine_copy2(kfm, kfm2);

	return kfm2;
}

/***********************************************************************
 * Execute the KFORTH machine 1 execution step.
 * If program finished, set kfm->loc.cb to -1 which indicated the machine
 * is in the terminated (halted) state.
 *
 * This routine requires that the program has
 * not previously terminated.
 *
 */
void kforth_machine_execute(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *program, KFORTH_MACHINE *kfm, void *client_data)
{
	int cb, pc;
	int nblocks, block_len;
	int opcode;
	KFORTH_INTEGER value;
	KFORTH_FUNCTION func;
	KFORTH_OPERATION *kfop;
	int diff;

	ASSERT( kfops != NULL );
	ASSERT( program != NULL );
	ASSERT( kfm != NULL );
	ASSERT( ! Kforth_Machine_Terminated(kfm) );

	cb = kfm->loc.cb;
	pc = kfm->loc.pc;

	nblocks = program->nblocks;
	ASSERT( cb < nblocks );

	block_len = program->block[cb][-1];

	if( pc >= block_len ) {
		/*
		 * return from code block
		 */
		if( kfm->csp > 0 ) {
			kfm->csp--;
			kfm->loc = kfm->call_stack[ kfm->csp ];
			kfm->loc.pc += 1;
		} else {
			// ASSERT( kfm->loc.cb == 0 );		// the assumption that all code starts at (0,0) isn't true for GROW.CB/SPAWN
			kfm->loc.cb = -1;					// mark machine as terminated
		}
		return;
	}

	opcode = program->block[cb][pc];					// FETCH opcode
	if( opcode & 0x8000 ) {								// DECODED as a number
		if( kfm->dsp < KF_MAX_DATA ) {
			value = opcode & 0x7fff;
			if( value & 0x4000 )
				value |= 0x8000; // sign extention
			kfm->data_stack[kfm->dsp++] = value;		// PUSH number
		}
	} else {											// DECODED as a instruction
		ASSERT( opcode >= 0 && opcode < KFORTH_OPS_LEN );

		kfop = &kfops->table[opcode];
		func = kfop->func;
		diff = kfop->out - kfop->in;
		if( (kfm->dsp >= kfop->in) && (kfm->dsp + diff <= KF_MAX_DATA) )
		{
			(*func)(kfops, program, kfm, client_data);			// EXECUTE instruction
		}
	}

	kfm->loc.pc += 1;
}

/***********************************************************************
 * Reset the kforth machine so that is can restart
 * execution of the program.
 *
 */
void kforth_machine_reset(KFORTH_MACHINE *kfm)
{
	int i;

	ASSERT( kfm != NULL );

	kfm->loc.cb = 0;
	kfm->loc.pc = 0;
	kfm->dsp = 0;
	kfm->csp = 0;

	for(i=0; i<10; i++)
		kfm->R[i] = 0;
}


/* ----------------------------------------------------------------------
 * KFORTH BUILT-IN OPCODES
 * Each function below may be called during execution of a KFORTH program.
 * The KFORTH_OPERATIONS table is indexed by the opcode, and
 * one of these functions is called.
 *
 * These operations form the CORE set of KFORTH operators that exist
 * in any KFORTH implementation. Additional operators
 * may be added by other implementations.
 *
 */

/*
 * Remove a value from the data stack. If
 * Stack is empty do nothing.
 * ( a -- )
 */
static void kfop_pop(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	kfm->dsp--;
}

/*
 * Call operation: Call code block
 * appearing on top of data stack. If code block number
 * is invalid, do nothing. If call stack full, do nothing.
 *
 * ( a -- )
 *
 */
static void kfop_call(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER cb;
	KFORTH_LOC loc;

	cb = Kforth_Data_Stack_Pop(kfm);
	if( cb < 0 || cb >= kfp->nblocks )
		return;

	if( kfm->csp >= KF_MAX_CALL )
		return;

	if( kfm->loc.cb >= kfp->nprotected ) {
		if( cb < kfp->nprotected ) {
			return;
		}
	}

	loc.cb = kfm->loc.cb;
	loc.pc = kfm->loc.pc;
	Kforth_Call_Stack_Push(kfm, loc);

	kfm->loc.pc = -1;		// 'pc' will get incremented to 0, inside the main execution loop
	kfm->loc.cb = cb;
}

/*
 * Call code block 'cb' if value is non-zero. Else
 * do nothing. If call stack is full this is a NOP.
 *
 * (value cb --)
 */
static void kfop_if(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value, cb;
	KFORTH_LOC loc;

	cb = Kforth_Data_Stack_Pop(kfm);
	value = Kforth_Data_Stack_Pop(kfm);

	if( cb < 0 || cb >= kfp->nblocks )
		return;

	if( value == 0 )
		return;

	if( kfm->csp >= KF_MAX_CALL )
		return;

	if( kfm->loc.cb >= kfp->nprotected ) {
		if( cb < kfp->nprotected ) {
			return;
		}
	}

	loc.cb = kfm->loc.cb;
	loc.pc = kfm->loc.pc;
	Kforth_Call_Stack_Push(kfm, loc);
				
	kfm->loc.pc = -1;		// 'pc' will get incremented to 0, inside the main execution loop
	kfm->loc.cb = cb;
}

/*
 * Call code block 'cb1' if value is non-zero. Else
 * call code block 'cb2'. If code block 1 or 2 is invalid,
 * assume it calls an empty NO-OP block. If call stack is full this is a NOP.
 *
 * (value cb1 cb2 --)
 */
static void kfop_ifelse(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value, cb1, cb2;
	KFORTH_LOC loc;

	cb2 = Kforth_Data_Stack_Pop(kfm);
	cb1 = Kforth_Data_Stack_Pop(kfm);
	value = Kforth_Data_Stack_Pop(kfm);

	if( kfm->csp >= KF_MAX_CALL )
		return;

	if( value != 0 ) {
		if( cb1 >= 0 && cb1 < kfp->nblocks ) {
			if( kfm->loc.cb >= kfp->nprotected ) {
				if( cb1 < kfp->nprotected ) {
					return;
				}
			}

			loc.cb = kfm->loc.cb;
			loc.pc = kfm->loc.pc;
			Kforth_Call_Stack_Push(kfm, loc);
			kfm->loc.pc = -1;			// 'pc' will get incremented to 0, inside the main execution loop
			kfm->loc.cb = cb1;
		}
	} else {
		if( cb2 >= 0 && cb2 < kfp->nblocks ) {
			if( kfm->loc.cb >= kfp->nprotected ) {
				if( cb2 < kfp->nprotected ) {
					return;
				}
			}

			loc.cb = kfm->loc.cb;
			loc.pc = kfm->loc.pc;
			Kforth_Call_Stack_Push(kfm, loc);
			kfm->loc.pc = -1;				// 'pc' will get incremented to 0, inside the main execution loop
			kfm->loc.cb = cb2;
		}
	}
}

/*
 * Loop to start of code block if n is non-zero.
 * ( n -- )
 */
static void kfop_loop(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	value = Kforth_Data_Stack_Pop(kfm);
	if( value != 0 ) {
		kfm->loc.pc = -1;				// 'pc' will get incremented to 0, inside the main execution loop
	}
}

/*
 * Exit code block (if n non-zero)
 * ( n -- )
 */
static void kfop_exit(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	value = Kforth_Data_Stack_Pop(kfm);
	if( value != 0 ) {
		kfm->loc.pc = kfp->block[kfm->loc.cb][-1]-1;			// pc is incremented when we return to execute()
	}
}

/*
 * ( a -- a a )
 */
static void kfop_dup(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	value = Kforth_Data_Stack_Top(kfm);
	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * ( a b -- b a )
 */
static void kfop_swap(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER tmp;

	tmp = kfm->data_stack[kfm->dsp-1];
	kfm->data_stack[kfm->dsp-1] = kfm->data_stack[kfm->dsp-2];
	kfm->data_stack[kfm->dsp-2] = tmp;
}

/*
 * ( a b -- a b a )
 */
static void kfop_over(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	value = kfm->data_stack[ kfm->dsp-2 ];
	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * ( a b c -- b c a )
 */
static void kfop_rot(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER a, b, c;

	a = kfm->data_stack[ kfm->dsp-3 ];
	b = kfm->data_stack[ kfm->dsp-2 ];
	c = kfm->data_stack[ kfm->dsp-1 ];

	kfm->data_stack[ kfm->dsp-3 ] = b;
	kfm->data_stack[ kfm->dsp-2 ] = c;
	kfm->data_stack[ kfm->dsp-1 ] = a;
}

/*
 * ( a b c -- c a b )
 */
static void kfop_reverse_rot(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER a, b, c;

	a = kfm->data_stack[ kfm->dsp-3 ];
	b = kfm->data_stack[ kfm->dsp-2 ];
	c = kfm->data_stack[ kfm->dsp-1 ];

	kfm->data_stack[ kfm->dsp-3 ] = c;
	kfm->data_stack[ kfm->dsp-2 ] = a;
	kfm->data_stack[ kfm->dsp-1 ] = b;
}

/*
 * ( n -- n n | 0 ) duplicate only if non-zero
 */
static void kfop_dup_if(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	value = Kforth_Data_Stack_Top(kfm);
	if( value ) {
		if( kfm->dsp < KF_MAX_DATA ) {
			Kforth_Data_Stack_Push(kfm, value);
		}
	}
}

/*
 * Swap 2 pairs
 *
 * ( a  b  c  d --  c  d  a  b )
 *   |  |  |  |     |  |  |  |
 *   |  |  |  |     |  |  |  |
 *   n1 n2 n3 n4    n1 n2 n3 n4
 */
static void kfop_2swap(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER *n1, *n2, *n3, *n4;
	KFORTH_INTEGER a, b, c, d;

	n4 = &kfm->data_stack[ kfm->dsp-1 ];
	n3 = &kfm->data_stack[ kfm->dsp-2 ];
	n2 = &kfm->data_stack[ kfm->dsp-3 ];
	n1 = &kfm->data_stack[ kfm->dsp-4 ];

	a = *n1;
	b = *n2;
	c = *n3;
	d = *n4;

	*n1 = c;
	*n2 = d;
	*n3 = a;
	*n4 = b;
}

/*
 * ( a b c d -- a b c d a b)
 */
static void kfop_2over(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER a, b;

	a = kfm->data_stack[ kfm->dsp-4 ];
	b = kfm->data_stack[ kfm->dsp-3 ];
	Kforth_Data_Stack_Push(kfm, a);
	Kforth_Data_Stack_Push(kfm, b);
}

/*
 * ( a b -- a b a b )
 */
static void kfop_2dup(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER a, b;

	a = kfm->data_stack[ kfm->dsp-2 ];
	b = kfm->data_stack[ kfm->dsp-1 ];
	Kforth_Data_Stack_Push(kfm, a);
	Kforth_Data_Stack_Push(kfm, b);
}

/*
 * ( a b -- )
 */
static void kfop_2pop(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
}

/*
 * ( a b -- b )
 */
static void kfop_nip(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	value = Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * ( a b -- b a b)
 */
static void kfop_tuck(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	kfop_swap(kfops, kfp, kfm, client_data);
	kfop_over(kfops, kfp, kfm, client_data);
}

/*
 * ( n -- n+1 )
 */
static void kfop_increment(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER n, value;

	n = Kforth_Data_Stack_Pop(kfm);
	value = n+1;
	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * ( n -- n-1 )
 */
static void kfop_decrement(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER n, value;

	n = Kforth_Data_Stack_Pop(kfm);
	value = n-1;
	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * ( n -- n+2 )
 */
static void kfop_increment2(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER n, value;

	n = Kforth_Data_Stack_Pop(kfm);
	value = n+2;
	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * ( n -- n-2 )
 */
static void kfop_decrement2(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER n, value;

	n = Kforth_Data_Stack_Pop(kfm);
	value = n-2;
	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * ( n -- n/2 )
 */
static void kfop_half(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER n, value;

	n = Kforth_Data_Stack_Pop(kfm);
	value = n/2;
	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * ( n -- n*2 )
 */
static void kfop_double(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER n, value;

	n = Kforth_Data_Stack_Pop(kfm);
	value = n*2;
	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * ( n -- ABS(n) )
 */
static void kfop_abs(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER n, value;

	n = Kforth_Data_Stack_Pop(kfm);
	value = (n < 0) ? -n : n;
	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * If n is negative, do nothing.
 * ( n -- SQRT(n) )
 */
static void kfop_sqrt(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER n, value;
	double d;

	n = Kforth_Data_Stack_Top(kfm);
	if( n >= 0 )
	{
		Kforth_Data_Stack_Pop(kfm);
		d = (double)n;
		d = sqrt(d);
		value = (KFORTH_INTEGER)d;
		Kforth_Data_Stack_Push(kfm, value);
	}
}

/*
 * ( a b -- a+b )
 */
static void kfop_plus(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER a, b, value;

	b = Kforth_Data_Stack_Pop(kfm);
	a = Kforth_Data_Stack_Pop(kfm);
	value = (a+b);
	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * ( a b -- a-b )
 */
static void kfop_minus(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER a, b, value;

	b = Kforth_Data_Stack_Pop(kfm);
	a = Kforth_Data_Stack_Pop(kfm);
	value = (a-b);
	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * ( a b -- a*b )
 */
static void kfop_multiply(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER a, b, value;

	b = Kforth_Data_Stack_Pop(kfm);
	a = Kforth_Data_Stack_Pop(kfm);
	value = (a*b);
	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * if b==0, do nothing.
 * ( a b -- a/b )
 */
static void kfop_divide(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER a, b;

	b = Kforth_Data_Stack_Top(kfm);
	if( b != 0 )
	{
		Kforth_Data_Stack_Pop(kfm);
		a = Kforth_Data_Stack_Pop(kfm);
		Kforth_Data_Stack_Push(kfm, a/b);
	}
}

/*
 * if b==0, do nothing.
 * ( a b -- a%b )
 */
static void kfop_modulos(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER a, b;

	b = Kforth_Data_Stack_Top(kfm);
	if( b != 0 )
	{
		Kforth_Data_Stack_Pop(kfm);
		a = Kforth_Data_Stack_Pop(kfm);
		Kforth_Data_Stack_Push(kfm, a%b);
	}
}

/*
 * if b==0, do nothing.
 * ( a b -- a%b a/b )
 */
static void kfop_divmod(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER a, b;

	b = Kforth_Data_Stack_Top(kfm);
	if( b != 0 )
	{
		Kforth_Data_Stack_Pop(kfm);
		a = Kforth_Data_Stack_Pop(kfm);

		Kforth_Data_Stack_Push(kfm, a%b);
		Kforth_Data_Stack_Push(kfm, a/b);
	}
}

/*
 * ( a -- -a )
 */
static void kfop_negate(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER a, value;

	a = Kforth_Data_Stack_Pop(kfm);
	value = -a;
	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * ( a b -- -a -b )
 */
static void kfop_2negate(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER a, b;

	b = Kforth_Data_Stack_Pop(kfm);
	a = Kforth_Data_Stack_Pop(kfm);
	Kforth_Data_Stack_Push(kfm, -a);
	Kforth_Data_Stack_Push(kfm, -b);
}

/*
 * ( a b -- a<<b )
 */
static void kfop_lshift(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER a, b, value;

	b = Kforth_Data_Stack_Pop(kfm);
	a = Kforth_Data_Stack_Pop(kfm);

	if( b >= 0 ) {
		value = a << b;
	} else {
		value = a >> -b;
	}

	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * ( a b -- a>>b )
 */
static void kfop_rshift(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER a, b, value;

	b = Kforth_Data_Stack_Pop(kfm);
	a = Kforth_Data_Stack_Pop(kfm);

	if( b >= 0 ) {
		value = a >> b;
	} else {
		value = a << -b;
	}

	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * ( a b -- a==b )
 */
static void kfop_eq(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER a, b, value;

	b = Kforth_Data_Stack_Pop(kfm);
	a = Kforth_Data_Stack_Pop(kfm);
	value = (a==b);
	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * ( a b -- a!=b )
 */
static void kfop_ne(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER a, b, value;

	b = Kforth_Data_Stack_Pop(kfm);
	a = Kforth_Data_Stack_Pop(kfm);
	value = (a!=b);
	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * ( a b -- a<b )
 */
static void kfop_lt(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER a, b, value;

	b = Kforth_Data_Stack_Pop(kfm);
	a = Kforth_Data_Stack_Pop(kfm);
	value = (a<b);
	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * ( a b -- a>b )
 */
static void kfop_gt(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER a, b, value;

	b = Kforth_Data_Stack_Pop(kfm);
	a = Kforth_Data_Stack_Pop(kfm);
	value = (a>b);
	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * ( a b -- a<=b )
 */
static void kfop_le(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER a, b, value;

	b = Kforth_Data_Stack_Pop(kfm);
	a = Kforth_Data_Stack_Pop(kfm);
	value = (a<=b);
	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * ( a b -- a>=b )
 */
static void kfop_ge(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER a, b, value;

	b = Kforth_Data_Stack_Pop(kfm);
	a = Kforth_Data_Stack_Pop(kfm);
	value = (a>=b);
	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * ( a -- a==0 )
 */
static void kfop_equal_zero(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER a, value;

	a = Kforth_Data_Stack_Pop(kfm);
	value = (a == 0);
	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * ( a b -- a|b )
 */
static void kfop_or(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER a, b, value;

	b = Kforth_Data_Stack_Pop(kfm);
	a = Kforth_Data_Stack_Pop(kfm);
	value = (a|b);
	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * ( a b -- a&b )
 */
static void kfop_and(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER a, b, value;

	b = Kforth_Data_Stack_Pop(kfm);
	a = Kforth_Data_Stack_Pop(kfm);
	value = (a&b);
	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * ( a -- !a )
 */
static void kfop_not(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER a, value;

	a = Kforth_Data_Stack_Pop(kfm);
	value = !a;
	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * ( a -- ~a )
 */
static void kfop_invert(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER a, value;

	a = Kforth_Data_Stack_Pop(kfm);
	value = ~a;
	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * ( a b -- a^b )
 */
static void kfop_xor(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER a, b, value;

	b = Kforth_Data_Stack_Pop(kfm);
	a = Kforth_Data_Stack_Pop(kfm);
	value = (a^b);
	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * ( a b -- MIN(a,b) )
 */
static void kfop_min(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER a, b, value;

	b = Kforth_Data_Stack_Pop(kfm);
	a = Kforth_Data_Stack_Pop(kfm);
	value = (a<b) ? a : b;
	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * ( a b -- MAX(a,b) )
 */
static void kfop_max(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER a, b, value;

	b = Kforth_Data_Stack_Pop(kfm);
	a = Kforth_Data_Stack_Pop(kfm);
	value = (a>b) ? a : b;
	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * ( -- CB )   ; current code block number
 */
static void kfop_cb(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Push(kfm, kfm->loc.cb);
}

/*
 * (n -- len )   ; length of code block
 */
static void kfop_cblen(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER n, value;
	int len;

	n = Kforth_Data_Stack_Pop(kfm);
	if( n < 0 || n >= kfp->nblocks )
	{
		value = -1;
		Kforth_Data_Stack_Push(kfm, value);
		return;
	}

	if( kfm->loc.cb >= kfp->nprotected ) {
		if( n < kfp->nprotected ) {
			value = -1;
			Kforth_Data_Stack_Push(kfm, value);
			return;
		}
	}

	len = kforth_program_cblen(kfp, n);

	value = len;

	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * ( -- CSLEN )   ; call stack length
 */
static void kfop_cslen(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	value = kfm->csp;
	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * ( -- DSLEN )   ; data stack length
 */
static void kfop_dslen(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	value = kfm->dsp;
	Kforth_Data_Stack_Push(kfm, value);
}

static void kfop_r0(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Push(kfm, kfm->R[0]);
}

static void kfop_r1(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Push(kfm, kfm->R[1]);
}

static void kfop_r2(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Push(kfm, kfm->R[2]);
}

static void kfop_r3(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Push(kfm, kfm->R[3]);
}

static void kfop_r4(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Push(kfm, kfm->R[4]);
}

static void kfop_r5(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Push(kfm, kfm->R[5]);
}

static void kfop_r6(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Push(kfm, kfm->R[6]);
}

static void kfop_r7(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Push(kfm, kfm->R[7]);
}

static void kfop_r8(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Push(kfm, kfm->R[8]);
}

static void kfop_r9(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Push(kfm, kfm->R[9]);
}

static void kfop_set_r0(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	value = Kforth_Data_Stack_Pop(kfm);
	kfm->R[0] = value;
}

static void kfop_set_r1(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	value = Kforth_Data_Stack_Pop(kfm);
	kfm->R[1] = value;
}

static void kfop_set_r2(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	value = Kforth_Data_Stack_Pop(kfm);
	kfm->R[2] = value;
}

static void kfop_set_r3(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	value = Kforth_Data_Stack_Pop(kfm);
	kfm->R[3] = value;
}

static void kfop_set_r4(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	value = Kforth_Data_Stack_Pop(kfm);
	kfm->R[4] = value;
}

static void kfop_set_r5(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	value = Kforth_Data_Stack_Pop(kfm);
	kfm->R[5] = value;
}

static void kfop_set_r6(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	value = Kforth_Data_Stack_Pop(kfm);
	kfm->R[6] = value;
}

static void kfop_set_r7(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	value = Kforth_Data_Stack_Pop(kfm);
	kfm->R[7] = value;
}

static void kfop_set_r8(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	value = Kforth_Data_Stack_Pop(kfm);
	kfm->R[8] = value;
}

static void kfop_set_r9(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	value = Kforth_Data_Stack_Pop(kfm);
	kfm->R[9] = value;
}

/* R0 ---------------------------------------------------------------------- */
static void kfop_r0_inc(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	value = kfm->R[0];
	kfm->R[0] += 1;
	Kforth_Data_Stack_Push(kfm, value);
}

static void kfop_r0_dec(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	kfm->R[0] -= 1;
	value = kfm->R[0];
	Kforth_Data_Stack_Push(kfm, value);
}

/* R1 ---------------------------------------------------------------------- */
static void kfop_r1_inc(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	value = kfm->R[1];
	kfm->R[1] += 1;
	Kforth_Data_Stack_Push(kfm, value);
}

static void kfop_r1_dec(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	kfm->R[1] -= 1;
	value = kfm->R[1];
	Kforth_Data_Stack_Push(kfm, value);
}

/* R2 ---------------------------------------------------------------------- */
static void kfop_r2_inc(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	value = kfm->R[2];
	kfm->R[2] += 1;
	Kforth_Data_Stack_Push(kfm, value);
}

static void kfop_r2_dec(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	kfm->R[2] -= 1;
	value = kfm->R[2];
	Kforth_Data_Stack_Push(kfm, value);
}

/* R3 ---------------------------------------------------------------------- */
static void kfop_r3_inc(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	value = kfm->R[3];
	kfm->R[3] += 1;
	Kforth_Data_Stack_Push(kfm, value);
}

static void kfop_r3_dec(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	kfm->R[3] -= 1;
	value = kfm->R[3];
	Kforth_Data_Stack_Push(kfm, value);
}

/* R4 ---------------------------------------------------------------------- */
static void kfop_r4_inc(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	value = kfm->R[4];
	kfm->R[4] += 1;
	Kforth_Data_Stack_Push(kfm, value);
}

static void kfop_r4_dec(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	kfm->R[4] -= 1;
	value = kfm->R[4];
	Kforth_Data_Stack_Push(kfm, value);
}

/* R5 ---------------------------------------------------------------------- */
static void kfop_r5_inc(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	value = kfm->R[5];
	kfm->R[5] += 1;
	Kforth_Data_Stack_Push(kfm, value);
}

static void kfop_r5_dec(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	kfm->R[5] -= 1;
	value = kfm->R[5];
	Kforth_Data_Stack_Push(kfm, value);
}

/* R6 ---------------------------------------------------------------------- */
static void kfop_r6_inc(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	value = kfm->R[6];
	kfm->R[6] += 1;
	Kforth_Data_Stack_Push(kfm, value);
}

static void kfop_r6_dec(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	kfm->R[6] -= 1;
	value = kfm->R[6];
	Kforth_Data_Stack_Push(kfm, value);
}

/* R7 ---------------------------------------------------------------------- */
static void kfop_r7_inc(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	value = kfm->R[7];
	kfm->R[7] += 1;
	Kforth_Data_Stack_Push(kfm, value);
}

static void kfop_r7_dec(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	kfm->R[7] -= 1;
	value = kfm->R[7];
	Kforth_Data_Stack_Push(kfm, value);
}

/* R8 ---------------------------------------------------------------------- */
static void kfop_r8_inc(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	value = kfm->R[8];
	kfm->R[8] += 1;
	Kforth_Data_Stack_Push(kfm, value);
}

static void kfop_r8_dec(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	kfm->R[8] -= 1;
	value = kfm->R[8];
	Kforth_Data_Stack_Push(kfm, value);
}

/* R9 ---------------------------------------------------------------------- */
static void kfop_r9_inc(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	value = kfm->R[9];
	kfm->R[9] += 1;
	Kforth_Data_Stack_Push(kfm, value);
}

static void kfop_r9_dec(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER value;

	kfm->R[9] -= 1;
	value = kfm->R[9];
	Kforth_Data_Stack_Push(kfm, value);
}

/* peek/poke  ---------------------------------------------------------------------- */
static void kfop_peek(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER n, value;
	int addr;

	n = Kforth_Data_Stack_Pop(kfm);

	if( n < 0 ) {
		addr = kfm->dsp + n;
	} else {
		addr = n;
	}

	if( addr < 0 || addr >= kfm->dsp )
	{
		value = -1;
		Kforth_Data_Stack_Push(kfm, value);
		return;
	}

	value = kfm->data_stack[addr];
	Kforth_Data_Stack_Push(kfm, value);
}

static void kfop_poke(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER n, value;
	int addr;

	n = Kforth_Data_Stack_Pop(kfm);
	value = Kforth_Data_Stack_Pop(kfm);

	if( n < 0 ) {
		addr = kfm->dsp + n;
	} else {
		addr = n;
	}

	if( addr < 0 || addr >= kfm->dsp )
	{
		return;
	}

	kfm->data_stack[addr] = value;
}

/* ---------------------------------------------------------------------- */

/*
 * Fetch a number from the kforth program location.
 *
 * Usage:		(cb pc -- value)
 *
 * Returns:
 *		-1		code block 'cb' was invalid
 *		-2		program counter 'pc' was invalid
 *		-3		location was an opcode, expecting number
 *		value	the value retrieved from that location
 *
 * Value will always be a 15-bit signed number
 *
 */
static void kfop_number(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER pc, cb;
	KFORTH_INTEGER value;
	int len, opcode;

	pc = Kforth_Data_Stack_Pop(kfm);
	cb = Kforth_Data_Stack_Pop(kfm);

	if( cb < 0 || cb >= kfp->nblocks ) {
		Kforth_Data_Stack_Push(kfm, -1);
		return;
	}

	if( kfm->loc.cb >= kfp->nprotected ) {
		if( cb < kfp->nprotected ) {
			Kforth_Data_Stack_Push(kfm, -1);
			return;
		}
	}

	len = kforth_program_cblen(kfp, cb);

	if( pc < 0 || pc >= len ) {
		Kforth_Data_Stack_Push(kfm, -2);
		return;
	}

	opcode = kfp->block[cb][pc];

	if( opcode & 0x8000 ) {
		value = opcode & 0x7fff;
		if( value & 0x4000 )
			value |= 0x8000; // sign extension
	} else {
		// instruction
		value = -3;
	}

	Kforth_Data_Stack_Push(kfm, value);
}

/*
 * Set a kforth program location to be the number 'value'
 *
 * Usage:		(value cb pc -- )
 *
 * Value will always be a 15-bit signed number
 *
 * Rules
 *	* Value must be a 15-bit signed number
 *
 */
static void kfop_set_number(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER pc, cb;
	KFORTH_INTEGER value;
	int len, number;

	pc = Kforth_Data_Stack_Pop(kfm);
	cb = Kforth_Data_Stack_Pop(kfm);
	value = Kforth_Data_Stack_Pop(kfm);

	// reduce 'value' to a 15-bit value
	number = 0x7fff & value;

	if( cb < 0 || cb >= kfp->nblocks ) {
		return;
	}

	if( kfm->loc.cb >= kfp->nprotected ) {
		if( cb < kfp->nprotected ) {
			return;
		}
	}

	len = kforth_program_cblen(kfp, cb);

	if( pc < 0 || pc >= len ) {
		return;
	}

	kfp->block[cb][pc] = (0x8000 | value);
}

static void kfop_test_set_number(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER pc, cb;
	KFORTH_INTEGER number;
	KFORTH_INTEGER value, curr_value;
	int len, opcode;

	pc = Kforth_Data_Stack_Pop(kfm);
	cb = Kforth_Data_Stack_Pop(kfm);
	value = Kforth_Data_Stack_Pop(kfm);

	// reduce 'value' to a 15-bit value
	number = 0x7fff & value;

	if( cb < 0 || cb >= kfp->nblocks ) {
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	if( kfm->loc.cb >= kfp->nprotected ) {
		if( cb < kfp->nprotected ) {
			Kforth_Data_Stack_Push(kfm, 0);
			return;
		}
	}

	len = kforth_program_cblen(kfp, cb);

	if( pc < 0 || pc >= len ) {
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	opcode = kfp->block[cb][pc];

	if( (opcode & 0x8000) == 0 ) {
		// instruction
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	curr_value = opcode & 0x7fff;
	if( curr_value & 0x4000 )
		curr_value |= 0x8000; // sign extension

	if( curr_value != 0 ) {
		Kforth_Data_Stack_Push(kfm, 0);
		return;
	}

	kfp->block[cb][pc] = (0x8000 | number);

	value = number;
	if( value & 0x4000 )
		value |= 0x8000; // sign extention

	Kforth_Data_Stack_Push(kfm, value);
}

static void kfop_opcode(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER pc, cb;
	KFORTH_INTEGER value;
	int len;

	pc = Kforth_Data_Stack_Pop(kfm);
	cb = Kforth_Data_Stack_Pop(kfm);

	if( cb < 0 || cb >= kfp->nblocks ) {
		Kforth_Data_Stack_Push(kfm, -1);
		return;
	}

	if( kfm->loc.cb >= kfp->nprotected ) {
		if( cb < kfp->nprotected ) {
			Kforth_Data_Stack_Push(kfm, -1);
			return;
		}
	}

	len = kforth_program_cblen(kfp, cb);

	if( pc < 0 || pc >= len ) {
		Kforth_Data_Stack_Push(kfm, -2);
		return;
	}

	value = kfp->block[cb][pc];

	if( value & 0x8000 ) {
		value = -3;
	} else {
		// instruction
		if( value < kfops->nprotected ) {
			if( kfm->loc.cb >= kfp->nprotected ) {
				// unprotected code is trying to read a protected instruction.
				value = -4;
			}
		}
	}

	Kforth_Data_Stack_Push(kfm, value);
}

static void kfop_set_opcode(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER pc, cb;
	KFORTH_INTEGER opcode;
	int len;

	pc = Kforth_Data_Stack_Pop(kfm);
	cb = Kforth_Data_Stack_Pop(kfm);
	opcode = Kforth_Data_Stack_Pop(kfm);

	if( opcode < 0 || opcode >= kfops->count ) {
		return;
	}

	if( kfm->loc.cb >= kfp->nprotected ) {
		if( cb < kfp->nprotected ) {
			return;
		}

		if( opcode < kfops->nprotected ) {
			return;
		}
	}

	if( cb < 0 || cb >= kfp->nblocks ) {
		return;
	}

	len = kforth_program_cblen(kfp, cb);

	if( pc < 0 || pc >= len ) {
		return;
	}

	kfp->block[cb][pc] = opcode;
}

// ( -- opcode )
static void kfop_lit_opcode(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER pc, cb;
	KFORTH_INTEGER value;
	int len;

	pc = kfm->loc.pc+1;		/* the next instruction */
	cb = kfm->loc.cb;

	len = kforth_program_cblen(kfp, cb);

	if( pc >= len ) {
		Kforth_Data_Stack_Push(kfm, -2);
		return;
	}

	kfm->loc.pc += 1;		// skip next instruciton no matter what.

	value = kfp->block[cb][pc];

	if( value & 0x8000 ) {
		value = -3;
	} else {
		// instruction
		if( value < kfops->nprotected ) {
			if( kfm->loc.cb >= kfp->nprotected ) {
				// unprotected code is trying to read a protected instruction.
				value = -4;
			}
		}
	}

	Kforth_Data_Stack_Push(kfm, value);
}

static void do_trap(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, KFORTH_INTEGER cb)
{
	KFORTH_LOC loc;

	ASSERT( kfops != NULL );
	ASSERT( kfp != NULL );
	ASSERT( kfm != NULL );
	ASSERT( cb >= 1 && cb <= 9 );

	if( kfm->csp >= KF_MAX_CALL )
		return;

	if( cb >= kfp->nblocks ) {
		return;
	}

	loc.cb = kfm->loc.cb;
	loc.pc = kfm->loc.pc;
	Kforth_Call_Stack_Push(kfm, loc);

	kfm->loc.pc = -1;		// 'pc' will get incremented to 0, inside the main execution loop
	kfm->loc.cb = cb;
}

static void kfop_trap1(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	do_trap(kfops, kfp, kfm, 1);
}

static void kfop_trap2(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	do_trap(kfops, kfp, kfm, 2);
}

static void kfop_trap3(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	do_trap(kfops, kfp, kfm, 3);
}

static void kfop_trap4(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	do_trap(kfops, kfp, kfm, 4);
}

static void kfop_trap5(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	do_trap(kfops, kfp, kfm, 5);
}

static void kfop_trap6(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	do_trap(kfops, kfp, kfm, 6);
}

static void kfop_trap7(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	do_trap(kfops, kfp, kfm, 7);
}

static void kfop_trap8(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	do_trap(kfops, kfp, kfm, 8);
}

static void kfop_trap9(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	do_trap(kfops, kfp, kfm, 9);
}

/* ---------------------------------------------------------------------- */

/*
 * ( n -- SIGN(n) )
 */
static void kfop_sign(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER n;

	n = Kforth_Data_Stack_Pop(kfm);

	if( n > 0 )
		n = 1;
	else if( n < 0 )
		n = -1;
	else
		n = 0;

	Kforth_Data_Stack_Push(kfm, n);
}

/*
 * ( a b -- n )
 */
static void kfop_pack(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER a, b, n;

	b = Kforth_Data_Stack_Pop(kfm) & 0xff;
	a = Kforth_Data_Stack_Pop(kfm) & 0xff;

	n = (a << 8) | (b);

	Kforth_Data_Stack_Push(kfm, n);
}

/*
 * ( n -- a b )
 */
static void kfop_unpack(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	KFORTH_INTEGER a, b, n;

	n = Kforth_Data_Stack_Pop(kfm);

	a = (n >> 8);
	b = (n & 0xff);

	Kforth_Data_Stack_Push(kfm, a);
	Kforth_Data_Stack_Push(kfm, b);
}

/*
 * ( -- 32767 )
 */
static void kfop_max_int(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Push(kfm, 32767);
}

/*
 * ( -- -32768 )
 */
static void kfop_min_int(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Data_Stack_Push(kfm, -32768);
}

/*
 * Halt 'kfm'
 */
static void kfop_halt(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
	Kforth_Machine_Terminate(kfm);
}

/*
 * NOP - No Operation
 */
static void kfop_nop(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MACHINE *kfm, void *client_data)
{
}

/***********************************************************************
 * Create a KFORTH_OPERATIONS table, and
 * add all the KFORTH_OPERATION
 * initialize with the KFORTH primitives.
 *
 * This table can be extended with additional primitives depending
 * on the application of KFORTH to a particular problem domain.
 *
 * These primitives always exist in the KFORTH language.
 *
 * KFORTH LANGUAGE PRIMITIVES:
 *
 *
 * Name			Function		Description
 * ---------	---------------	------------------------------------------
 *
 * pop			kfop_pop		( n -- )
 *
 * call			kfop_call		( code-block -- )		call code block
 * if			kfop_if			( expr code-block -- )
 * ifelse		kfop_ifelse		( expr true-block false-block -- )
 * ?loop		kfop_loop		( n -- )	goto start of code block
 * ?exit		kfop_exit		( n -- )	exit code block if n true
 *
 * dup			kfop_dup		( a -- a a )
 * swap			kfop_swap		( a b -- b a )
 * over			kfop_over		( a b  -- a b a )
 * rot			kfop_rot		( a b c -- b c a )
 * -rot			kfop_reverse_rot ( a b c -- c a b )
 * ?dup			kfop_dup_if		( n -- n n | 0 ) duplicate only if non-zero
 * 2swap		kfop_2swap		( a b c d --  c d a b )
 * 2over		kfop_2over		( a b c d -- a b c d a b)
 * 2dup			kfop_2dup		( a b -- a b a b )
 * 2pop			kfop_2pop		( a b  -- )
 * nip			kfop_nip		( a b -- b )
 * tuck			kfop_tuck		( a b -- b a b)
 * 1+			kfop_increment	( n -- n+1 )
 * 1-			kfop_decrement	( n -- n-1 )
 * 2+			kfop_increment2	( n -- n+2 )
 * 2-			kfop_decrement2	( n -- n-2 )
 * 2/			kfop_half		( n -- n/2 )
 * 2*			kfop_double		( n -- n*2 )
 * abs			kfop_abs		( n -- abs(n) )
 * sqrt			kfop_sqrt		( n -- sqrt(n) )
 * +			kfop_plus		( a b -- a+b )
 * -			kfop_minus		( a b -- a-b )
 * *			kfop_multiply	( a b -- a*b )
 * /			kfop_divide		( a b -- a/b )
 * mod			kfop_modulos	( a b -- a%b )
 * /mod			kfop_divmod		( a b -- a%b a/b )
 * negate		kfop_negate		( n -- -n )
 * 2negate		kfop_2negate	( a b -- -a -b )
 * =			kfop_eq			( a b -- EQ(a,b) )
 * <>			kfop_ne			( a b -- NE(a,b) )
 * <			kfop_lt			( a b -- LT(a,b) )
 * >			kfop_gt			( a b -- GT(a,b) )
 * <=			kfop_le			( a b -- LE(a,b) )
 * >=			kfop_ge			( a b -- GE(a,b) )
 * 0=			kfop_equal_zero	( n -- EQ(n,0) )
 * or			kfop_or			( a b -- a|b )	logical (and bitwise) or
 * and			kfop_and		( a b -- a&b )	logical (and bitwise) and
 * not			kfop_not		( n -- !n )	logical not
 * invert		kfop_invert		( n -- ~n )	bitwise not
 * xor			kfop_xor		( a b -- a^b )	bitwise XOR
 * min			kfop_min		( a b -- min(a,b) )
 * max			kfop_max		( a b  -- max(a,b) )
 *
 * CB			kfop_cb			( -- CB )	get code block number
 * R0			kfop_r0			(   -- R0 )	get register r0
 * R1			kfop_r1			(   -- R1 )	get register r1
 * R2			kfop_r2			(   -- R2 )	get register r2
 * R3			kfop_r3			(   -- R3 )	get register r3
 * R4			kfop_r4			(   -- R4 )	get register r4
 * R5			kfop_r5			(   -- R5 )	get register r5
 * R6			kfop_r6			(   -- R6 )	get register r6
 * R7			kfop_r7			(   -- R7 )	get register r7
 * R8			kfop_r8			(   -- R8 )	get register r8
 * R9			kfop_r9			(   -- R9 )	get register r9
 *
 * R0!			kfop_set_r0		( val -- )	set regiser 0 to val
 * R1!			kfop_set_r1		( val -- )	set regiser 1 to val
 * R2!			kfop_set_r2		( val -- )	set regiser 2 to val
 * R3!			kfop_set_r3		( val -- )	set regiser 3 to val
 * R4!			kfop_set_r4		( val -- )	set regiser 4 to val
 * R5!			kfop_set_r5		( val -- )	set regiser 5 to val
 * R6!			kfop_set_r6		( val -- )	set regiser 6 to val
 * R7!			kfop_set_r7		( val -- )	set regiser 7 to val
 * R8!			kfop_set_r8		( val -- )	set regiser 8 to val
 * R9!			kfop_set_r9		( val -- )	set regiser 9 to val
 *
 * R0++			kfop_r0_inc		( -- val)	post increment register 0
 * R1++			kfop_r1_inc		( -- val)	post increment register 1
 * R2++			kfop_r2_inc		( -- val)	post increment register 2
 * R3++			kfop_r3_inc		( -- val)	post increment register 3
 * R4++			kfop_r4_inc		( -- val)	post increment register 4
 * R5++			kfop_r5_inc		( -- val)	post increment register 5
 * R6++			kfop_r6_inc		( -- val)	post increment register 6
 * R7++			kfop_r7_inc		( -- val)	post increment register 7
 * R8++			kfop_r8_inc		( -- val)	post increment register 8
 * R9++			kfop_r9_inc		( -- val)	post increment register 9
 *
 * --R0			kfop_r0_dec		( -- val)	pre decrement register 0
 * --R1			kfop_r1_dec		( -- val)	pre decrement register 1
 * --R2			kfop_r2_dec		( -- val)	pre decrement register 2
 * --R3			kfop_r3_dec		( -- val)	pre decrement register 3
 * --R4			kfop_r4_dec		( -- val)	pre decrement register 4
 * --R5			kfop_r5_dec		( -- val)	pre decrement register 5
 * --R6			kfop_r6_dec		( -- val)	pre decrement register 6
 * --R7			kfop_r7_dec		( -- val)	pre decrement register 7
 * --R8			kfop_r8_dec		( -- val)	pre decrement register 8
 * --R9			kfop_r9_dec		( -- val)	pre decrement register 9
 *
 * PEEK			kfop_peek		(n -- val)	get an arbitrary data stack element
 * POKE			kfop_poke		(val n -- )	set an arbitrary data stack element
 *
 * NUMBER		kfop_number				(cb pc -- val)		get kforth program number literal
 * NUMBER!		kfop_set_number			(val cb pc -- )		set a kforth program slots to a number literal
 * ?NUMBER!		kfop_test_set_number	(val cb pc -- val|0)	test and set a kforth program slots (if zero, else do nothing).
 *
 * OPCODE		kfop_opcode				(cb pc -- val)		get kforth program opcode
 * OPCODE!		kfop_set_opcode			(val cb pc -- )		set a kforth program slots to be opcode
 * OPCODE'		kfop_lit_opcode			( -- val)			get kforth program literal from next program location
 *
 * TRAP1		kfop_trap1		( -- )		call to code block 1, set Trap corresponding trap bit
 * TRAP2		kfop_trap2		( -- )		call to code block 1, set Trap corresponding trap bit
 * TRAP3		kfop_trap3		( -- )		call to code block 1, set Trap corresponding trap bit
 * TRAP4		kfop_trap4		( -- )		call to code block 1, set Trap corresponding trap bit
 * TRAP5		kfop_trap5		( -- )		call to code block 1, set Trap corresponding trap bit
 * TRAP6		kfop_trap6		( -- )		call to code block 1, set Trap corresponding trap bit
 * TRAP7		kfop_trap7		( -- )		call to code block 1, set Trap corresponding trap bit
 * TRAP8		kfop_trap8		( -- )		call to code block 1, set Trap corresponding trap bit
 * TRAP9		kfop_trap9		( -- )		call to code block 1, set Trap corresponding trap bit
 *
 * sign			kfop_sign		( n -- SIGN(n) ) sign of 'n' (returns: -1, 0, 1)
 * pack2		kfop_pack		( a b -- n )	combine 2 8-bit ints into a single 'n'
 * unpack2		kfop_unpack		( n -- a b )	extracts 2 8-bit values from a single 'n'
 * MAX_INT		kfop_max_int 	( -- 32767)	returns max signed int 
 * MIN_INT		kfop_min_int 	( -- -32768)	returns min signed int 
 * HALT			kfop_halt 		( -- )			halts the kforth machine
 * NOP			kfop_nop 		( -- )			does nothing
 */
void kforth_ops_init(KFORTH_OPERATIONS *kfops)
{
	kfops->count = 0;
	kfops->nprotected = 0;

	/*
	 * the first entry (opcde=0) is special.
	 * it is added here, without using kforth_ops_add()
	 * as "" is not a valid operator name.
 	 *
 	 * Instruction 0 is created by the compiler
	 * when it compiles the end of a code block.
	 *
	 */
	kforth_ops_add(kfops, "call",		1,	0,		kfop_call);
	kforth_ops_add(kfops, "if",			2,	0,		kfop_if);
	kforth_ops_add(kfops, "ifelse",		3,	0,		kfop_ifelse);
	kforth_ops_add(kfops, "?loop",		1,	0,		kfop_loop);
	kforth_ops_add(kfops, "?exit",		1,	0,		kfop_exit);

	kforth_ops_add(kfops, "pop",		1,	0,		kfop_pop);
	kforth_ops_add(kfops, "dup",		1,	2,		kfop_dup);
	kforth_ops_add(kfops, "swap",		2,	2,		kfop_swap);
	kforth_ops_add(kfops, "over",		2,	3,		kfop_over);
	kforth_ops_add(kfops, "rot",		3,	3,		kfop_rot);

	kforth_ops_add(kfops, "?dup",		1,	0,		kfop_dup_if);
	kforth_ops_add(kfops, "-rot",		3,	3,		kfop_reverse_rot);
	kforth_ops_add(kfops, "2swap",		4,	4,		kfop_2swap);
	kforth_ops_add(kfops, "2over",		4,	6,		kfop_2over);
	kforth_ops_add(kfops, "2dup",		2,	4,		kfop_2dup);
	kforth_ops_add(kfops, "2pop",		2,	0,		kfop_2pop);
	kforth_ops_add(kfops, "nip",		2,	1,		kfop_nip);
	kforth_ops_add(kfops, "tuck",		2,	3,		kfop_tuck);
	kforth_ops_add(kfops, "1+",			1,	1,		kfop_increment);
	kforth_ops_add(kfops, "1-",			1,	1,		kfop_decrement);
	kforth_ops_add(kfops, "2+",			1,	1,		kfop_increment2);
	kforth_ops_add(kfops, "2-",			1,	1,		kfop_decrement2);
	kforth_ops_add(kfops, "2/",			1,	1,		kfop_half);
	kforth_ops_add(kfops, "2*",			1,	1,		kfop_double);
	kforth_ops_add(kfops, "abs",		1,	1,		kfop_abs);
	kforth_ops_add(kfops, "sqrt",		1,	1,		kfop_sqrt);
	kforth_ops_add(kfops, "+",			2,	1,		kfop_plus);
	kforth_ops_add(kfops, "-",			2,	1,		kfop_minus);
	kforth_ops_add(kfops, "*",			2,	1,		kfop_multiply);
	kforth_ops_add(kfops, "/",			2,	1,		kfop_divide);
	kforth_ops_add(kfops, "mod",		2,	1,		kfop_modulos);
	kforth_ops_add(kfops, "/mod",		2,	2,		kfop_divmod);
	kforth_ops_add(kfops, "negate",		1,	1,		kfop_negate);
	kforth_ops_add(kfops, "2negate",	2,	2,		kfop_2negate);
	kforth_ops_add(kfops, "<<",			2,	1,		kfop_lshift);
	kforth_ops_add(kfops, ">>",			2,	1,		kfop_rshift);
	kforth_ops_add(kfops, "=",			2,	1,		kfop_eq);
	kforth_ops_add(kfops, "<>",			2,	1,		kfop_ne);
	kforth_ops_add(kfops, "<",			2,	1,		kfop_lt);
	kforth_ops_add(kfops, ">",			2,	1,		kfop_gt);
	kforth_ops_add(kfops, "<=",			2,	1,		kfop_le);
	kforth_ops_add(kfops, ">=",			2,	1,		kfop_ge);
	kforth_ops_add(kfops, "0=",			1,	1,		kfop_equal_zero);
	kforth_ops_add(kfops, "or",			2,	1,		kfop_or);
	kforth_ops_add(kfops, "and",		2,	1,		kfop_and);
	kforth_ops_add(kfops, "not",		1,	1,		kfop_not);
	kforth_ops_add(kfops, "invert",		1,	1,		kfop_invert);
	kforth_ops_add(kfops, "xor",		2,	1,		kfop_xor);
	kforth_ops_add(kfops, "min",		2,	1,		kfop_min);
	kforth_ops_add(kfops, "max",		2,	1,		kfop_max);

	kforth_ops_add(kfops, "CB",			0,	1,		kfop_cb);
	kforth_ops_add(kfops, "CBLEN",		1,	1,		kfop_cblen);
	kforth_ops_add(kfops, "CSLEN",		0,	1,		kfop_cslen);
	kforth_ops_add(kfops, "DSLEN",		0,	1,		kfop_dslen);
	kforth_ops_add(kfops, "R0",			0,	1,		kfop_r0);
	kforth_ops_add(kfops, "R1",			0,	1,		kfop_r1);
	kforth_ops_add(kfops, "R2",			0,	1,		kfop_r2);
	kforth_ops_add(kfops, "R3",			0,	1,		kfop_r3);
	kforth_ops_add(kfops, "R4",			0,	1,		kfop_r4);
	kforth_ops_add(kfops, "R5",			0,	1,		kfop_r5);
	kforth_ops_add(kfops, "R6",			0,	1,		kfop_r6);
	kforth_ops_add(kfops, "R7",			0,	1,		kfop_r7);
	kforth_ops_add(kfops, "R8",			0,	1,		kfop_r8);
	kforth_ops_add(kfops, "R9",			0,	1,		kfop_r9);

	kforth_ops_add(kfops, "R0!",		1,	0,		kfop_set_r0);
	kforth_ops_add(kfops, "R1!",		1,	0,		kfop_set_r1);
	kforth_ops_add(kfops, "R2!",		1,	0,		kfop_set_r2);
	kforth_ops_add(kfops, "R3!",		1,	0,		kfop_set_r3);
	kforth_ops_add(kfops, "R4!",		1,	0,		kfop_set_r4);
	kforth_ops_add(kfops, "R5!",		1,	0,		kfop_set_r5);
	kforth_ops_add(kfops, "R6!",		1,	0,		kfop_set_r6);
	kforth_ops_add(kfops, "R7!",		1,	0,		kfop_set_r7);
	kforth_ops_add(kfops, "R8!",		1,	0,		kfop_set_r8);
	kforth_ops_add(kfops, "R9!",		1,	0,		kfop_set_r9);

	kforth_ops_add(kfops, "R0++",		0,	1,		kfop_r0_inc);
	kforth_ops_add(kfops, "R1++",		0,	1,		kfop_r1_inc);
	kforth_ops_add(kfops, "R2++",		0,	1,		kfop_r2_inc);
	kforth_ops_add(kfops, "R3++",		0,	1,		kfop_r3_inc);
	kforth_ops_add(kfops, "R4++",		0,	1,		kfop_r4_inc);
	kforth_ops_add(kfops, "R5++",		0,	1,		kfop_r5_inc);
	kforth_ops_add(kfops, "R6++",		0,	1,		kfop_r6_inc);
	kforth_ops_add(kfops, "R7++",		0,	1,		kfop_r7_inc);
	kforth_ops_add(kfops, "R8++",		0,	1,		kfop_r8_inc);
	kforth_ops_add(kfops, "R9++",		0,	1,		kfop_r9_inc);

	kforth_ops_add(kfops, "--R0",		0,	1,		kfop_r0_dec);
	kforth_ops_add(kfops, "--R1",		0,	1,		kfop_r1_dec);
	kforth_ops_add(kfops, "--R2",		0,	1,		kfop_r2_dec);
	kforth_ops_add(kfops, "--R3",		0,	1,		kfop_r3_dec);
	kforth_ops_add(kfops, "--R4",		0,	1,		kfop_r4_dec);
	kforth_ops_add(kfops, "--R5",		0,	1,		kfop_r5_dec);
	kforth_ops_add(kfops, "--R6",		0,	1,		kfop_r6_dec);
	kforth_ops_add(kfops, "--R7",		0,	1,		kfop_r7_dec);
	kforth_ops_add(kfops, "--R8",		0,	1,		kfop_r8_dec);
	kforth_ops_add(kfops, "--R9",		0,	1,		kfop_r9_dec);

	kforth_ops_add(kfops, "PEEK",		1,	1,		kfop_peek);
	kforth_ops_add(kfops, "POKE",		2,	0,		kfop_poke);

	kforth_ops_add(kfops, "NUMBER",		2,	1,		kfop_number);
	kforth_ops_add(kfops, "NUMBER!",	3,	0,		kfop_set_number);
	kforth_ops_add(kfops, "?NUMBER!",	3,	1,		kfop_test_set_number);
	kforth_ops_add(kfops, "OPCODE",		2,	1,		kfop_opcode);
	kforth_ops_add(kfops, "OPCODE!",	3,	0,		kfop_set_opcode);
	kforth_ops_add(kfops, "OPCODE'",	0,	1,		kfop_lit_opcode);
	kforth_ops_add(kfops, "TRAP1",		0,	2,		kfop_trap1);
	kforth_ops_add(kfops, "TRAP2",		0,	2,		kfop_trap2);
	kforth_ops_add(kfops, "TRAP3",		0,	2,		kfop_trap3);
	kforth_ops_add(kfops, "TRAP4",		0,	2,		kfop_trap4);
	kforth_ops_add(kfops, "TRAP5",		0,	2,		kfop_trap5);
	kforth_ops_add(kfops, "TRAP6",		0,	2,		kfop_trap6);
	kforth_ops_add(kfops, "TRAP7",		0,	2,		kfop_trap7);
	kforth_ops_add(kfops, "TRAP8",		0,	2,		kfop_trap8);
	kforth_ops_add(kfops, "TRAP9",		0,	2,		kfop_trap9);

	kforth_ops_add(kfops, "sign",		1,	1,		kfop_sign);
	kforth_ops_add(kfops, "pack2",		2,	1,		kfop_pack);
	kforth_ops_add(kfops, "unpack2",	1,	2,		kfop_unpack);

	kforth_ops_add(kfops, "MAX_INT",	0,	1,		kfop_max_int);
	kforth_ops_add(kfops, "MIN_INT",	0,	1,		kfop_min_int);
	kforth_ops_add(kfops, "HALT",		0,	0,		kfop_halt);
	kforth_ops_add(kfops, "nop",		0,	0,		kfop_nop);
}

KFORTH_OPERATIONS *kforth_ops_make(void)
{
	KFORTH_OPERATIONS *kfops;

	kfops = (KFORTH_OPERATIONS*) CALLOC(1, sizeof(KFORTH_OPERATIONS));

	ASSERT( kfops != NULL );

	kforth_ops_init(kfops);

	return kfops;
}

/***********************************************************************
 * Free a KFORTH_OPERATIONS table.
 *
 */
void kforth_ops_delete(KFORTH_OPERATIONS *kfops)
{
	ASSERT( kfops != NULL );

	FREE(kfops);
}

/***********************************************************************
 * A valid kforth operator name:
 *	1. Does not include ';' (clashes with comments)
 *	2. Does not include ':'	(clashes with labels)
 *	3. Does not include '{'	or '}' (clashes with braces)
 *	4. Does not include white space
 *	5. Does not include control characters
 *	6. Cannot clash with a number '999' '123', '0', '-12'
 *	7. Does not have a length of 0.
 *
 */
static int kforth_valid_operator_name(const char *name)
{
	const char *p;
	int digits, len, leading_minus;

	ASSERT( name != NULL );

	if( name[0] == '-' )
		leading_minus = 1;
	else
		leading_minus = 0;

	digits = 0;
	len = 0;
	for(p=name; *p; p++) {
		if( *p == ':' )
			return 0;

		if( *p == ';' )
			return 0;

		if( *p == '{' )
			return 0;

		if( *p == '}' )
			return 0;

		if( isspace(*p) )
			return 0;

		if( iscntrl(*p) )
			return 0;

		if( isdigit(*p) )
			digits++;

		len++;
	}

	if( len == 0 )
		return 0;

	if( (digits > 0) && (leading_minus + digits) == len )
		return 0;

	return 1;

}

static void kforth_ops_add_impl(KFORTH_OPERATIONS *kfops, const char *name, int key, int in, int out, KFORTH_FUNCTION func)
{
	int i;

	ASSERT( kfops != NULL );
	ASSERT( name != NULL );
	ASSERT( func != NULL );
	ASSERT( kfops->count+1 < KFORTH_OPS_LEN );

	for(i=0; i < kfops->count; i++) {
		ASSERT( kfops->table[i].name != NULL );
		ASSERT( stricmp(name, kfops->table[i].name) != 0 );
	}

	kfops->count += 1;
	kfops->table[i].name	= name;
	kfops->table[i].func	= func;
	kfops->table[i].key		= key;
	kfops->table[i].in		= in;
	kfops->table[i].out		= out;
}

/***********************************************************************
 * This routine requires that the 'kfops' table not be full,
 * and that 'name' has not already been used.
 * And that 'name' is a valid KFORTH operator name.
 *
 */
void kforth_ops_add(KFORTH_OPERATIONS *kfops, const char *name, int in, int out, KFORTH_FUNCTION func)
{
	ASSERT( name != NULL );
	ASSERT( kforth_valid_operator_name(name) );
	
	kforth_ops_add_impl(kfops, name, 0, in, out, func);
}

void kforth_ops_add2(KFORTH_OPERATIONS *kfops, KFORTH_OPERATION *kfop)
{
	kforth_ops_add_impl(kfops, kfop->name, kfop->key, kfop->in, kfop->out, kfop->func);
}

/*
 * Return the maximum defined opcode for 'kfops'
 *
 * Valid opcodes will be: 0 .. max_opcode
 *
 * max_opcode will be less than or equal to KFORTH_OPS_LEN
 *
 */
int kforth_ops_max_opcode(KFORTH_OPERATIONS *kfops)
{
	ASSERT( kfops != NULL );

	return kfops->count - 1;
}

int kforth_ops_find(KFORTH_OPERATIONS *kfops, const char *name)
{
	int i;
	
	for(i=0; i < kfops->count; i++) {
		if( stricmp(name, kfops->table[i].name) == 0 ) {
			return i;
		}
	}
	return -1;
}

void kforth_ops_del(KFORTH_OPERATIONS *kfops, const char *name)
{
	int i, del_idx;

	ASSERT( kfops != NULL );
	ASSERT( name != NULL );

	del_idx = kforth_ops_find(kfops, name);

	ASSERT( del_idx != -1 );

	if( del_idx < kfops->nprotected ) {
		kfops->nprotected -= 1;
	}

	kfops->count -= 1;
	for(i=del_idx; i < kfops->count; i++) {
		kfops->table[i] = kfops->table[i+1];
	}
}

KFORTH_OPERATION* kforth_ops_get(KFORTH_OPERATIONS *kfops, int idx)
{
	ASSERT( kfops != NULL );
	ASSERT( idx >= 0 && idx < KFORTH_OPS_LEN );
	
	return &kfops->table[idx];
}

/***********************************************************************
 * Designates the instruction 'name' as "protected" by moving
 * to the protected instruction region.
 *
 * - Find 'name' and put in tmp
 * - remove 'name' from kfops
 * - grow kfops for 1 more protected instruction
 * - store 'tmp' into new protected slot. (Sorted by key in the protected region)
 * - increment kfops->nprotected
 *
 * Precondition: 'name' must exist in table, and NOT be protected.
 *
 */
void kforth_ops_set_protected(KFORTH_OPERATIONS *kfops, const char *name)
{
	int i;
	int found_idx, insert_idx;
	KFORTH_OPERATION tmp;

	ASSERT( kfops != NULL );
	ASSERT( name != NULL );

	found_idx = kforth_ops_find(kfops, name);

	ASSERT( found_idx >= kfops->nprotected );

	tmp = kfops->table[found_idx];

	kforth_ops_del(kfops, name);

	insert_idx = kfops->nprotected;
	for(i=0; i < kfops->nprotected; i++) {
		if( tmp.key < kfops->table[i].key )
		{
			insert_idx = i;
			break;
		}
	}

	for(i=kfops->count; i > insert_idx; i--) {
		kfops->table[i] = kfops->table[i-1];
	}

	kfops->table[insert_idx] = tmp;
	kfops->nprotected += 1;
	kfops->count += 1;
}

/***********************************************************************
 * It marks the instruction 'name' as unprotected (assuming it was already protected)
 *
 * Moves the instruction from its slot in the protected region and puts
 * it in the unprotected region (sorted by key).
 *
 * Number of protected instructions is decreased by 1
 *
 */
void kforth_ops_set_unprotected(KFORTH_OPERATIONS *kfops, const char *name)
{
	int i;
	int found_idx, insert_idx;
	KFORTH_OPERATION tmp;

	ASSERT( kfops != NULL );
	ASSERT( name != NULL );

	found_idx = kforth_ops_find(kfops, name);

	ASSERT( found_idx != -1 && found_idx < kfops->nprotected );

	tmp = kfops->table[found_idx];

	kforth_ops_del(kfops, name);

	insert_idx = kfops->count;
	for(i=kfops->nprotected; i < kfops->count; i++) {
		if( tmp.key < kfops->table[i].key )
		{
			insert_idx = i;
			break;
		}
	}

	for(i=kfops->count; i > insert_idx; i--) {
		kfops->table[i] = kfops->table[i-1];
	}

	kfops->table[insert_idx] = tmp;
//	kfops->nprotected -= 1;
	kfops->count += 1;
}
