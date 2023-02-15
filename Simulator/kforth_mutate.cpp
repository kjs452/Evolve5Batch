/*
 * Copyright (c) 2022 Ken Stauffer
 */


/***********************************************************************
 * MUTATE/MERGE KFORTH PROGRAMS
 *
 *
 */
#include "evolve_simulator.h"
#include "evolve_simulator_private.h"

/*
 * Instruction-level mutations will use XLEN as the maximum
 * instruction segment to manuipulate. (shorter code blocks
 * will use segments lengths smaller than XLEN)
 *
 */
#define XLEN_MAX 100		/* maximum value the kfmo->xlen can be (usually this value ranges from 1 to 10, defaults to 4) */

/*
 * return a random number between a and b. (including a and b)
 * (a and b must be integers)
 *
 * ASSERT( a <= b )
 */
#define CHOOSE(er,a,b)	( (sim_random(er) % ((b)-(a)+1) ) + (a) )

static void choose_instruction(EVOLVE_RANDOM *er, KFORTH_OPERATIONS *kfops, KFORTH_MUTATE_OPTIONS *kfmo, KFORTH_INTEGER *value)
{
	long x;
	int NUMB_NEW = 99;

	ASSERT( kfops != NULL );
	ASSERT( kfmo != NULL );
	ASSERT( er != NULL );
	ASSERT( value != NULL );

	/*
	 * there are no unprotected instructions to choose from, just pick a number
	 */
	if( kfops->nprotected >= kfops->count )
	{
		*value = 0x8000 | CHOOSE(er, -99, 99);
		return;
	}

	x = CHOOSE(er, 0, PROBABILITY_SCALE);
	if( x < PROBABILITY_SCALE/2 ) {
		/*
		 * 1/2 of the time we chose a number
		 */
		*value = 0x8000 | CHOOSE(er, -NUMB_NEW, NUMB_NEW);
	} else {
		/*
		 * 1/2 of the time we chose an instruction
		 */
		*value = CHOOSE(er, kfops->nprotected, kfops->count-1);
	}
}

/*
 * modify an existing instruction.
 *
 *	- modify the instuction (cb, pc)
 *	- (cb, pc) must be a valid instruction
 *	- pick a random number between -NUMB and NUMB. If 0, swap sign, otherwise add it to the
 *	  number.
 *	- if not a number, change instruction to something different (but not to a number)
 *
 * This keeps numbers as numbers and instructions as instructions.
 */
static void modify_single_instruction(EVOLVE_RANDOM *er, KFORTH_MUTATE_OPTIONS *kfmo,
				KFORTH_OPERATIONS *kfops,
				KFORTH_PROGRAM *kfp, int cb, int pc )
{
	int delta;
	int NUMB = 4;		/* new numbers are picked from -NUMB ... +NUMB */
	KFORTH_INTEGER value;

	ASSERT( er != NULL );
	ASSERT( kfops != NULL );
	ASSERT( kfmo != NULL );
	ASSERT( kfp != NULL );
	ASSERT( cb >= 0 && cb < kfp->nblocks );
	ASSERT( pc >= 0 );

	value = kfp->block[cb][pc];

	if( value & 0x8000 ) {
		value = value & 0x7fff;
		if( value & 0x4000 ) {
			value |= 0x8000;		// sign extend
		}

		delta = CHOOSE(er, -NUMB, NUMB);
		if( delta == 0 ) {
			/*
			 * swap sign
		 	 */
			value = -value;

		} else {
			/*
			 * Increment or Decrement by 1 - NUMB
			 */
			value += delta;
		}

		if( value > 16383 ) {
			value = 0;
		} else if( value < -16384 ) {
			value = 0;
		}

		kfp->block[cb][pc] = 0x8000 | value;

	} else {
		if( kfops->nprotected <= kfops->count-1 ) {
			kfp->block[cb][pc] = (unsigned char) CHOOSE(er, kfops->nprotected, kfops->count-1);
		}
	}

}

// Return the length of code block 'cb'
//
//    kfp->block[cb] ----+
//                       |
//                       v
//              +-----+-----+-----+-----+-----+-----+-----+
//              |  6  | if  | 2   |  3  | nop |call | dup |
//              +-----+-----+-----+-----+-----+-----+-----+
//
//
int kforth_program_cblen(KFORTH_PROGRAM *kfp, int cb)
{
	return kfp->block[cb][-1];
}

/*
 * Pick random code block insert it into a random spot.
 */
static void duplicate_code_block(KFORTH_PROGRAM *kfp, KFORTH_MUTATE_OPTIONS *kfmo, EVOLVE_RANDOM *er)
{
	int cb, nblocks, pc, i, len;
	KFORTH_INTEGER *block;

	ASSERT( kfp != NULL );
	ASSERT( er != NULL );

	nblocks = kfp->nblocks;

	if( nblocks == 0 )
		return;

	/*
	 * do not do proceed, if program has max_code_blocks (unprotected) code blocks
	 */
	if( nblocks - kfp->nprotected >= kfmo->max_code_blocks )
		return;

	/*
	 * do not do proceed, if program does not contain any unprotected code blocks to duplicate.
	 */
	if( nblocks <= kfp->nprotected )
		return;

	/*
	 * Pick a code block to duplicate (and remember it)
	 */
	cb	= CHOOSE(er, kfp->nprotected, nblocks-1);
	len	= kforth_program_cblen(kfp, cb);
	block = kfp->block[cb];

	/*
	 * Grow program to fit 1 more code block
	 */
	kfp->block = (KFORTH_INTEGER**) REALLOC(kfp->block, (nblocks+1) * sizeof(KFORTH_INTEGER*));
	kfp->nblocks = nblocks+1;

	/*
	 * Shift all code blocks at 'cb'
	 */
	cb = CHOOSE(er, kfp->nprotected, nblocks);
	if( cb < nblocks ) {
		for(i=nblocks; i>cb; i--) {
			kfp->block[i] = kfp->block[i-1];
		}
	}

	kfp->block[cb] = ((KFORTH_INTEGER*) CALLOC(len+1, sizeof(KFORTH_INTEGER)) + 1);

	for(pc=0; pc<len; pc++) {
		kfp->block[cb][pc] = block[pc];
	}
	kfp->block[cb][-1] = len;
}

/*
 * Pick a random sequence of 1 - XLEN instructions and insert it at a random spot
 */
static void duplicate_instruction(KFORTH_PROGRAM *kfp, KFORTH_MUTATE_OPTIONS *kfmo, EVOLVE_RANDOM *er)
{
	int cb, pc, block_len, len, i;
	KFORTH_INTEGER save[XLEN_MAX];

	ASSERT( kfp != NULL );
	ASSERT( er != NULL );

	/*
 	 * do not proceed, if program has no unproteced code blocks to duplicate instructions from/into
	 */
	if( kfp->nblocks <= kfp->nprotected )
		return;

	cb = CHOOSE(er, kfp->nprotected, kfp->nblocks-1);
	block_len = kforth_program_cblen(kfp, cb);

	if( block_len == 0 )
		return;

	len = CHOOSE(er, 1, ((block_len < kfmo->xlen) ? block_len : kfmo->xlen) );

	/*
	 * pick random instructions to duplicate
	 */
	pc = CHOOSE(er, 0, block_len-len);

	for(i=0; i<len; i++) {
		save[i] = kfp->block[cb][pc+i];
	}

	/*
	 * pick random code block to insert
	 */
	cb = CHOOSE(er, kfp->nprotected, kfp->nblocks-1);
	block_len = kforth_program_cblen(kfp, cb);

	/*
	 * Grow program to fit len more instructions in 'cb'
	 */
	kfp->block[cb] = ((KFORTH_INTEGER*) REALLOC(kfp->block[cb]-1, (block_len+len+1) * sizeof(KFORTH_INTEGER))) + 1;

	/*
	 * make gap at 'pc' for len instructions
	 */
	pc = CHOOSE(er, 0, block_len);
	if( pc < block_len ) {
		for(i=block_len+len-1; i > pc+len-1; i--) {
			kfp->block[cb][i] = kfp->block[cb][i-len];
		}
	}
	kfp->block[cb][-1] = block_len+len;

	/*
	 * Insert copy of instruction
	 */
	for(i=0; i<len; i++){
		kfp->block[cb][pc+i] = save[i];
	}
}

/*
 * Remove an entire code block
 */
static void delete_code_block(KFORTH_PROGRAM *kfp, KFORTH_MUTATE_OPTIONS *kfmo, EVOLVE_RANDOM *er)
{
	int cb, nblocks, i;

	ASSERT( kfp != NULL );
	ASSERT( er != NULL );

	/*
	 * Delete entire code block and move subsequent code blocks up.
	 */
	nblocks = kfp->nblocks;

	/*
	 * do not do proceed, if program has no unprotected code blocks to delete
	 */
	if( nblocks <= kfp->nprotected )
		return;

	/*
	 * do not do this mutation if program only has 1 code block
	 */
	if( nblocks <= 1 )
		return;

	cb = CHOOSE(er, kfp->nprotected, nblocks-1);

	FREE( kfp->block[cb]-1 );

	for(i=cb; i<nblocks-1; i++) {
		kfp->block[i] = kfp->block[i+1];
	}

	kfp->nblocks = nblocks-1;
}

/*
 * Delete 1 - XLEN instructions from a random place in a random code block
 */
static void delete_instruction(KFORTH_PROGRAM *kfp, KFORTH_MUTATE_OPTIONS *kfmo, EVOLVE_RANDOM *er)
{
	int cb, pc, block_len, len, i;

	ASSERT( kfp != NULL );
	ASSERT( er != NULL );

	/*
	 * do not proceed, if program has no unprotected code blocks to delete instructions from
	 */
	if( kfp->nblocks <= kfp->nprotected )
		return;

	cb = CHOOSE(er, kfp->nprotected, kfp->nblocks-1);

	block_len = kforth_program_cblen(kfp, cb);

	if( block_len == 0 )
		return;

	len = CHOOSE(er, 1, ((block_len < kfmo->xlen) ? block_len : kfmo->xlen) );

	pc = CHOOSE(er, 0, block_len - len);

	for(i=pc; i < block_len - len; i++) {
		kfp->block[cb][i] = kfp->block[cb][i+len];
	}
	kfp->block[cb][-1] = block_len - len;
}

/*
 * Insert a code block into program. New code block
 * will contain random instructions. Length of new code block
 * will be randomly between 0 and XLEN.
 */
static void insert_code_block(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MUTATE_OPTIONS *kfmo, EVOLVE_RANDOM *er)
{
	int cb, pc, nblocks, i, len;
	KFORTH_INTEGER value;

	ASSERT( kfp != NULL );
	ASSERT( er != NULL );

	/*
	 * Grow program to fit 1 more code block
	 */
	nblocks = kfp->nblocks;

	/*
	 * do not do proceed, if program has max_code_blocks (unprotected) code blocks
	 */
	if( nblocks - kfp->nprotected >= kfmo->max_code_blocks )
		return;

	/*
	 * do not do proceed, if program is smaller than the size of the protected region.
	 * (adding a code block would require modifying protected code blocks)
	 */
	if( nblocks < kfp->nprotected )
		return;

	kfp->block = (KFORTH_INTEGER**)REALLOC(kfp->block, (nblocks+1) * sizeof(KFORTH_INTEGER*));
	kfp->nblocks = nblocks+1;

	if( nblocks == 0 ) {
		cb = 0;
	} else {
		cb = CHOOSE(er, kfp->nprotected, nblocks);
		if( cb < nblocks ) {
			for(i=nblocks; i>cb; i--) {
				kfp->block[i] = kfp->block[i-1];
			}
		}
	}

	/*
	 * create code block at 'cb'
	 * Random length between 0 and XLEN.
	 */
	len = CHOOSE(er, 0, kfmo->xlen);
	kfp->block[cb] = ((KFORTH_INTEGER*) CALLOC(len+1, sizeof(KFORTH_INTEGER))) + 1;

	for(pc=0; pc < len; pc++) {
		choose_instruction(er, kfops, kfmo, &value);
		kfp->block[cb][pc] = value;
	}
	kfp->block[cb][-1] = len;
}

/*
 * Pick random code block and insert some random instructions 1 - XLEN
 * into code block at a random spot.
 */
static void insert_instruction(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MUTATE_OPTIONS *kfmo, EVOLVE_RANDOM *er)
{
	int cb, pc, block_len, len, i;
	KFORTH_INTEGER value[XLEN_MAX];

	ASSERT( kfp != NULL );
	ASSERT( kfops != NULL );
	ASSERT( er != NULL );

	/*
	 * do not do proceed, if program has no unprotected code blocks to delete instructions from
	 */
	if( kfp->nblocks <= kfp->nprotected )
		return;

	cb = CHOOSE(er, kfp->nprotected, kfp->nblocks-1);

	block_len = kforth_program_cblen(kfp, cb);

	/*
	 * Pick 1 to XLEN random instructions.
	 */
	len = CHOOSE(er, 1, kfmo->xlen);
	for(i=0; i<len; i++) {
		choose_instruction(er, kfops, kfmo, &value[i]);
	}

	/*
	 * Grow program to fit len more instructions in 'cb'
	 */
	kfp->block[cb] = ((KFORTH_INTEGER*) REALLOC(kfp->block[cb]-1, (block_len+len+1) * sizeof(KFORTH_INTEGER))) + 1;

	/*
	 * make gap at 'pc' for len instructions
	 */
	pc = CHOOSE(er, 0, block_len);
	if( pc < block_len ) {
		for(i=block_len+len-1; i > pc+len-1; i--) {
			kfp->block[cb][i] = kfp->block[cb][i-len];
		}

	}
	kfp->block[cb][-1] = block_len+len;

	/*
 	 * fill in new instructions
	 */
	for(i=0; i < len; i++) {
		kfp->block[cb][pc+i] = value[i];
	}
}

/*
 * Swap 2 randomly chosen code blocks
 */
static void transpose_code_block(KFORTH_PROGRAM *kfp, KFORTH_MUTATE_OPTIONS *kfmo, EVOLVE_RANDOM *er)
{
	int cb1, cb2;
	KFORTH_INTEGER *save_block;

	ASSERT( kfp != NULL );
	ASSERT( er != NULL );

	/*
	 * If program doesn't have at least 2 unprotected code blocks to transpose, then do nothing
	 */
	if( kfp->nblocks - kfp->nprotected < 2 )
		return;

	cb1 = CHOOSE(er, kfp->nprotected, kfp->nblocks-1);
	cb2 = CHOOSE(er, kfp->nprotected, kfp->nblocks-1);

	if( cb1 == cb2 )
		return;

	save_block			= kfp->block[cb1];
	kfp->block[cb1]		= kfp->block[cb2];
	kfp->block[cb2]		= save_block;
}

/*
 * Pick 2 random instruction segments (1 to XLEN long) and swap them.
 */
static void transpose_instruction(KFORTH_PROGRAM *kfp, KFORTH_MUTATE_OPTIONS *kfmo, EVOLVE_RANDOM *er)
{
	int cb1, pc1;
	int cb2, pc2;
	int block_len1, block_len2;
	int len, i;

	KFORTH_INTEGER save_value[XLEN_MAX];

	ASSERT( kfp != NULL );
	ASSERT( er != NULL );

	/*
	 * If program doesn't have at least 1 unprotected code block to transpose instructions from, then do nothing
	 */
	if( kfp->nblocks <= kfp->nprotected )
		return;

	cb1 = CHOOSE(er, kfp->nprotected, kfp->nblocks-1);
	block_len1 = kforth_program_cblen(kfp, cb1);

	if( block_len1 == 0 )
		return;

	cb2 = CHOOSE(er, kfp->nprotected, kfp->nblocks-1);
	block_len2 = kforth_program_cblen(kfp, cb2);

	if( block_len2 == 0 )
		return;

	/*
	 * compute, len = MIN(XLEN, block_len1, block_len2)
	 */
	if( block_len1 < block_len2 ) {
		len = CHOOSE(er, 1, (block_len1 < kfmo->xlen) ? block_len1 : kfmo->xlen);
	} else {
		len = CHOOSE(er, 1, (block_len2 < kfmo->xlen) ? block_len2 : kfmo->xlen);
	}

	pc1 = CHOOSE(er, 0, block_len1-len);
	pc2 = CHOOSE(er, 0, block_len2-len);

	for(i=0; i<len; i++) {
		save_value[i] = kfp->block[cb1][pc1+i];
	}

	for(i=0; i<len; i++) {
		kfp->block[cb1][pc1+i] = kfp->block[cb2][pc2+i];
	}

	for(i=0; i<len; i++) {
		kfp->block[cb2][pc2+i] = save_value[i];
	}
}

/*
 * Pick a random code block and modify all numbers in it. instructions unchanged.
 * numbers are increased/decreased by a tiny amount, 
 */
static void modify_code_block(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MUTATE_OPTIONS *kfmo, EVOLVE_RANDOM *er)
{
	int cb, pc, len;

	ASSERT( kfops != NULL );
	ASSERT( kfp != NULL );
	ASSERT( kfmo != NULL );
	ASSERT( er != NULL );

	/*
	 * If program doesn't have at least 1 unprotected code block to modify, then do nothing
	 */
	if( kfp->nblocks - kfp->nprotected < 1 )
		return;

	cb = CHOOSE(er, kfp->nprotected, kfp->nblocks-1);
	len = kforth_program_cblen(kfp, cb);

	if( len == 0 )
		return;

	for(pc=0; pc < len; pc++) {
		if( kfp->block[cb][pc] & 0x8000 ) {
			modify_single_instruction(er, kfmo, kfops, kfp, cb, pc);
		}
	}
}

/*
 * Pick a random starting instruction and modify 1 - XLEN sequential instructions.
 */
static void modify_instruction(KFORTH_OPERATIONS *kfops, KFORTH_PROGRAM *kfp, KFORTH_MUTATE_OPTIONS *kfmo, EVOLVE_RANDOM *er)
{
	int cb, pc, block_len, len, i;

	ASSERT( kfops != NULL );
	ASSERT( kfp != NULL );
	ASSERT( kfmo != NULL );
	ASSERT( er != NULL );

	/*
	 * If program doesn't have at least 1 unprotected code block to modify, then do nothing
	 */
	if( kfp->nblocks - kfp->nprotected < 1 )
		return;

	cb = CHOOSE(er, kfp->nprotected, kfp->nblocks-1);

	block_len = kforth_program_cblen(kfp, cb);

	if( block_len == 0 )
		return;

	len = CHOOSE(er, 1, ((block_len < kfmo->xlen) ? block_len : kfmo->xlen) );
	pc = CHOOSE(er, 0, block_len - len);

	for(i=0; i<len; i++) {
		modify_single_instruction(er, kfmo, kfops, kfp, cb, pc+i);
	}
}


/***********************************************************************
 * Using the mutation options 'kfmo' and
 * the random number generator 'er' make a tiny
 * random change to kfp.
 *
 * The first step is to determine if we will act at the instruction-level or
 * the codeblock-level. Once this determiniation is made, we check to
 * see if any of the 5 kinds of mutations will be applied (it may turn
 * out they we perfom NO mutations at all).
 *
 */
void kforth_mutate(KFORTH_OPERATIONS *kfops,
			KFORTH_MUTATE_OPTIONS *kfmo,
			EVOLVE_RANDOM *er,
			KFORTH_PROGRAM *kfp)
{
	long x;
	int mutate_code_block;
	int napply, i;

	ASSERT( kfops != NULL );
	ASSERT( kfp != NULL );
	ASSERT( kfmo != NULL );
	ASSERT( er != NULL );
	ASSERT( kfp->nblocks > 0 );

	/*
	 * Program is smaller than the protected region by 1 code block,
	 * no mutations will change this, so we can exit now.
 	 */
	if( kfp->nblocks < kfp->nprotected )
		return;

	if( kfmo->max_apply == 1 ) {
		napply = 1;

	} else if( kfmo->max_apply == 0 ) {
		return;

	} else {
		napply = CHOOSE(er, 1, kfmo->max_apply);
	}

	for(i=0; i < napply; i++) {
		/*
		 * Mutate at codeblock-level or instruction-level?
		 */
		x = CHOOSE(er, 0, PROBABILITY_SCALE);
		if( x < kfmo->prob_mutate_codeblock )
			mutate_code_block = 1;
		else
			mutate_code_block = 0;

		/*
		 * Do Duplication?
		 */
		x = CHOOSE(er, 0, PROBABILITY_SCALE);
		if( x < kfmo->prob_duplicate ) {
			if( mutate_code_block )
				duplicate_code_block(kfp, kfmo, er);
			else
				duplicate_instruction(kfp, kfmo, er);
		}

		/*
		 * Do Deletion?
		 */
		x = CHOOSE(er, 0, PROBABILITY_SCALE);
		if( x < kfmo->prob_delete ) {
			if( mutate_code_block )
				delete_code_block(kfp, kfmo, er);
			else
				delete_instruction(kfp, kfmo, er);
		}

		/*
		 * Do Insertion?
		 */
		x = CHOOSE(er, 0, PROBABILITY_SCALE);
		if( x < kfmo->prob_insert ) {
			if( mutate_code_block )
				insert_code_block(kfops, kfp, kfmo, er);
			else
				insert_instruction(kfops, kfp, kfmo, er);
		}

		/*
		 * Do Transposition?
		 */
		x = CHOOSE(er, 0, PROBABILITY_SCALE);
		if( x < kfmo->prob_transpose ) {
			if( mutate_code_block )
				transpose_code_block(kfp, kfmo, er);
			else
				transpose_instruction(kfp, kfmo, er);
		}

		/*
		 * Do Modification?
		 */
		x = CHOOSE(er, 0, PROBABILITY_SCALE);
		if( x < kfmo->prob_modify ) {
			if( mutate_code_block )
				modify_code_block(kfops, kfp, kfmo, er);
			else
				modify_instruction(kfops, kfp, kfmo, er);
		}
	}
}

/*
 * Apply the mutation algorthm to 'block'
 *
 * 'block' is a pointer to a pointer to MALLOC'd memory.
 * 'block' may be changed depending on the mutations applied.
 */
void kforth_mutate_cb(KFORTH_OPERATIONS *kfops,
			KFORTH_MUTATE_OPTIONS *kfmo,
			EVOLVE_RANDOM *er,
			KFORTH_INTEGER **block )
{
	KFORTH_PROGRAM kfp;
	KFORTH_INTEGER *blocks[1];
	KFORTH_MUTATE_OPTIONS new_kfmo;

	ASSERT( kfops != NULL );
	ASSERT( kfmo != NULL );
	ASSERT( er != NULL );
	ASSERT( block != NULL );
	ASSERT( *block != NULL );

	kfp.nblocks = 1;
	kfp.nprotected = 0;
	kfp.block = blocks;
	blocks[0] = *block;

	new_kfmo = *kfmo;

	// don't mutate at code block level
	new_kfmo.prob_mutate_codeblock = 0;		

	// don't use the max_apply full amount, it may be too much for a single code block.
	new_kfmo.max_apply = (new_kfmo.max_apply) ? 1 : 0;	

	kforth_mutate(kfops, &new_kfmo, er, &kfp);

	*block = blocks[0];		// 'block' may have been reallocated
}

/*
**********************************************************************
**********************************************************************
**********************************************************************
**********************************************************************
**********************************************************************
**********************************************************************
*/

/***********************************************************************
 *
 * Merge two kforth program.
 *
 * Merge kfp1 and kfp2 and put the result into kfp
 *
 * Algorithm:
 * - Pick a 16-bit 'mask' parameter (based on mode/random)
 * - Iterate over each program
 * - A 0 bit in the mask pattern takes from kfp1
 * - A 1 bit in the mask pattern takes from kfp2
 * - If the 16-bits are exhausted, the mask is repeated.
 *
 * Example,
 *
 * If mask is 0000 0000 1011 0001
 *
 *	PROGRAM 1	PROGRAM 2	RESULT
 *	---------	---------	----------
 *	ABBA		BLAG		BLAG
 *	JUNK		CRAP		JUNK
 *	FOOBAR		GREPLINUX	FOOBAR
 *	STUFF		XXYYZZ		STUFF
 *	SUPERDUPER	FOOFOOFOO	SUPERDUPER
 *	FOO12h		FOO666		FOO12h
 *	HHIIGG					HHIIGG
 *	HHUUPP					HHUUPP
 *	JJJJJJ					JJJJJJ
 *
 * Create a new program from 'kfp1' and 'kfp2'. 
 *
 * If programs have different number of code blocks, then
 * extra code blocks from the longer program are appended
 * to the new program.
 *
 */
void kforth_merge2(EVOLVE_RANDOM *er, KFORTH_MUTATE_OPTIONS *kfmo, KFORTH_PROGRAM *kfp1, KFORTH_PROGRAM *kfp2, KFORTH_PROGRAM *kfp)
{
	int mask;
	KFORTH_PROGRAM *p;
	int cb, pc, len;
	int bit, curmask;

	ASSERT( er != NULL );
	ASSERT( kfmo != NULL );
	ASSERT( kfp1 != NULL );
	ASSERT( kfp2 != NULL );
	ASSERT( kfp != NULL );

	if( kfmo->merge_mode == 0 ) {
		// Merge using a random 16-bit bit string.
		mask = CHOOSE(er, 0x0000, 0xFFFF);

	} else if( kfmo->merge_mode == 1 ) {
		// Merge using parent1 (kfp1) first, then alternate derministically
		mask = 0xAAAA;			// 1010101010101010

	} else if( kfmo->merge_mode == 2 ) {
		// Merge using parent2 (kfp2) first, then alternate deterministically
		mask = 0x5555;			// 0101010101010101
	}

	if( kfp1->nblocks > kfp2->nblocks ) {
		kfp->nblocks = kfp1->nblocks;
	} else {
		kfp->nblocks = kfp2->nblocks;
	}

	kfp->nprotected = (kfp1->nprotected > kfp2->nprotected) ? kfp1->nprotected : kfp2->nprotected ;

	kfp->block = (KFORTH_INTEGER**) CALLOC(kfp->nblocks, sizeof(KFORTH_INTEGER**));

	bit = 0;
	curmask = mask;
	for(cb=0; cb < kfp->nblocks; cb++) {
		if( (curmask & 0x0001) == 0 ) {
			if( cb < kfp1->nblocks )
				p = kfp1;
			else
				p = kfp2;
		} else {
			if( cb < kfp2->nblocks )
				p = kfp2;
			else
				p = kfp1;
		}

		len = kforth_program_cblen(p, cb);

		kfp->block[cb] = ((KFORTH_INTEGER*) CALLOC(len+1, sizeof(KFORTH_INTEGER))) + 1;

		for(pc=0; pc < len; pc++) {
			kfp->block[cb][pc] = p->block[cb][pc];
		}
		kfp->block[cb][-1] = len;

		curmask = curmask >> 1;
		bit += 1;
		if( bit >= 16 ) {
			curmask = mask;
		}
	}
}

KFORTH_PROGRAM *kforth_merge(EVOLVE_RANDOM *er, KFORTH_MUTATE_OPTIONS *kfmo, KFORTH_PROGRAM *kfp1, KFORTH_PROGRAM *kfp2)
{
	KFORTH_PROGRAM *kfp;

	ASSERT( er != NULL );
	ASSERT( kfmo != NULL );
	ASSERT( kfp1 != NULL );
	ASSERT( kfp2 != NULL );

	kfp = (KFORTH_PROGRAM*) CALLOC(1, sizeof(KFORTH_PROGRAM));
	ASSERT( kfp != NULL );

	kforth_merge2(er, kfmo, kfp1, kfp2, kfp);

	return kfp;
}

// Copy code block cb1 to cb2, including 
static void kforth_program_cbcpy(KFORTH_INTEGER *cb1, KFORTH_INTEGER *cb2)
{
	int pc, len;

	len = cb1[-1];

	for(pc=0; pc < len; pc++)			// 	CHANGED
	{
		cb2[pc] = cb1[pc];
	}
	//cb2[pc] = 0; // ENDCB instruction		// DELETED
	cb2[-1] = cb1[-1];						// ADDED
}

void kforth_copy2(KFORTH_PROGRAM *kfp, KFORTH_PROGRAM *kfp2)
{
	int cb, len;

	ASSERT( kfp != NULL );
	ASSERT( kfp2 != NULL );

	kfp2->nblocks = kfp->nblocks;
	kfp2->nprotected = kfp->nprotected;
	kfp2->block = (KFORTH_INTEGER**) CALLOC(kfp->nblocks, sizeof(KFORTH_INTEGER**));

	for(cb=0; cb < kfp->nblocks; cb++)
	{
		len = kforth_program_cblen(kfp, cb);
		kfp2->block[cb] = ((KFORTH_INTEGER*) CALLOC(len+1, sizeof(KFORTH_INTEGER))) + 1;		// CHANGED
		kforth_program_cbcpy(kfp->block[cb], kfp2->block[cb]);

	}
}

/*
 * Make a copy of of 'kfp'
 */
KFORTH_PROGRAM *kforth_copy(KFORTH_PROGRAM *kfp)
{
	KFORTH_PROGRAM *new_kfp;

	ASSERT( kfp != NULL );

	new_kfp = (KFORTH_PROGRAM*) CALLOC(1, sizeof(KFORTH_PROGRAM));
	ASSERT( new_kfp != NULL );

	kforth_copy2(kfp, new_kfp);

	return new_kfp;
}

/*
 * Create a KFORTH_MUTATE_OPTIONS structure
 *
 * Inputs are:
 *	max_code_blocks:
 *		- mutated programs will never get mutated to bigger than this size.
 *		  (if they are already bigger than this, then they can still be mutated, just
 *		  never grow bigger. In other words no truncation on existing programs
 *		  is perfomed).
 *
 *	max_apply:
 *		- maximum number of times to apply mutations to a program. Pick
 *		a random count from 1.. max_apply, and apply the mutation algorithm
 *		that many times. If 0, disable mutations.
 *
 *	prob_mutate_codeblock
 *		- probability a code block will be mutated versus a single instruction
 *
 *	prob_duplicate
 *		- probability the duplicate mutation will take place
 *
 *	prob_delete
 *		- probability the delete mutation will take place
 *
 *	prob_insert
 *		- probability the insert mutation will take place
 *
 *	prob_transpose
 *		- probability the transpose mutation will take place
 *
 *	prob_modify
 *		- probability the modify mutation will take place
 *
 * 	max_opcode		- when mutations are synthesizing a new instruction, use this maximum.
 *
 * frozen_cb_start
 * frozen_cb_end	- within this range do not modify code blocks. Keep them unchanging and also
 *						don't peek within this range to find instructions mutate elsewhere with.
 *
 * All values are floating point (doubles) representing
 * probabilities (between 0.0 and 1.0)
 *
 * We support 5 types of mutations. Each type can be applied
 * at the code block level, or instruction level. That is
 * what the probability 'prob_mutate_codeblock' determines.
 *
 *
 */
KFORTH_MUTATE_OPTIONS *kforth_mutate_options_make(
			int max_code_blocks,
			int max_apply,
			double prob_mutate_codeblock,
			double prob_duplicate,
			double prob_delete,
			double prob_insert,
			double prob_transpose,
			double prob_modify,
			int merge_mode,
			int xlen,
			int protected_codeblocks )
{
	KFORTH_MUTATE_OPTIONS *kfmo;

	ASSERT( max_code_blocks > 0 );
	ASSERT( max_apply >= 0 && max_apply <= MUTATE_MAX_APPLY_LIMIT );
	ASSERT( prob_mutate_codeblock >= 0.0 && prob_mutate_codeblock <= 1.0 );
	ASSERT( prob_duplicate >= 0.0 && prob_duplicate <= 1.0 );
	ASSERT( prob_delete >= 0.0 && prob_delete <= 1.0 );
	ASSERT( prob_insert >= 0.0 && prob_insert <= 1.0 );
	ASSERT( prob_transpose >= 0.0 && prob_transpose <= 1.0 );
	ASSERT( prob_modify >= 0.0 && prob_modify <= 1.0 );
	ASSERT( merge_mode >= 0 && merge_mode <= 1 );
	ASSERT( xlen >= 1 && xlen <= 50 );
	ASSERT( protected_codeblocks >= 0 );

	kfmo = (KFORTH_MUTATE_OPTIONS *) CALLOC(1, sizeof(KFORTH_MUTATE_OPTIONS));
	ASSERT( kfmo != NULL );

	kfmo->max_code_blocks		= max_code_blocks;
	kfmo->max_apply				= max_apply;
	kfmo->prob_mutate_codeblock	= (int) (prob_mutate_codeblock * PROBABILITY_SCALE);
	kfmo->prob_duplicate		= (int) (prob_duplicate * PROBABILITY_SCALE);
	kfmo->prob_delete			= (int) (prob_delete * PROBABILITY_SCALE);
	kfmo->prob_insert			= (int) (prob_insert * PROBABILITY_SCALE);
	kfmo->prob_transpose		= (int) (prob_transpose * PROBABILITY_SCALE);
	kfmo->prob_modify			= (int) (prob_modify * PROBABILITY_SCALE);
	kfmo->merge_mode			= merge_mode;
	kfmo->xlen					= xlen;
	kfmo->protected_codeblocks	= protected_codeblocks;

	return kfmo;
}

void kforth_mutate_options_copy2(KFORTH_MUTATE_OPTIONS *kfmo, KFORTH_MUTATE_OPTIONS *kfmo2)
{
	*kfmo2 = *kfmo;
}

/*
 * Return a CALLOC'd copy of kfmo.
 */
KFORTH_MUTATE_OPTIONS *kforth_mutate_options_copy(KFORTH_MUTATE_OPTIONS *kfmo)
{
	KFORTH_MUTATE_OPTIONS *new_kfmo;

	ASSERT( kfmo != NULL );

	new_kfmo = (KFORTH_MUTATE_OPTIONS *) CALLOC(1, sizeof(KFORTH_MUTATE_OPTIONS));
	ASSERT( new_kfmo != NULL );

	kforth_mutate_options_copy2(kfmo, new_kfmo);

	return new_kfmo;
}

/*
 * These are the default kforth mutation rates
 *
 */
void kforth_mutate_options_defaults(KFORTH_MUTATE_OPTIONS *kfmo)
{
	KFORTH_MUTATE_OPTIONS *tmp;

	ASSERT( kfmo != NULL );

	tmp = kforth_mutate_options_make(100, 10, 0.25, 0.02, 0.04, 0.02, 0.02, 0.02,
						0, 4, 0);

	*kfmo = *tmp;

	kforth_mutate_options_delete(tmp);
}

void kforth_mutate_options_delete(KFORTH_MUTATE_OPTIONS *kfmo)
{
	ASSERT( kfmo != NULL );

	FREE( kfmo );
}
