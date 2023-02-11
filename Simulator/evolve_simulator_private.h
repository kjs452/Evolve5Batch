#ifndef _EVOLVE_SIMULATOR_PRIVATE_H
#define _EVOLVE_SIMULATOR_PRIVATE_H

#define KFORTH_COMPILE_FAST

/*
 * Copyright (c) 2022 Stauffer Computer Consulting
 */

/***********************************************************************
 * evolve_simulator:
 *
 * Header file for the evolve simulator implementation.
 *
 * (Private stuff not needed by clients of the evolve_simulator library)
 *
 */

typedef struct {
	CELL		*cell;
	UNIVERSE	*universe;
} CELL_CLIENT_DATA;

//
// These macros access the KFORTH_MACHINE directly instead of calling a function
//
#define Kforth_Data_Stack_Top(kfm)				( (kfm)->data_stack[ (kfm)->dsp-1 ] 			)
#define Kforth_Data_Stack_2nd(kfm)				( (kfm)->data_stack[ (kfm)->dsp-2 ] 			)
#define Kforth_Data_Stack_Pop(kfm)				( (kfm)->data_stack[ --(kfm)->dsp ] 			)
#define Kforth_Machine_Terminated(kfm)			( (kfm)->loc.cb == -1 							)
#define Kforth_Data_Stack_Push(kfm, value)		do { (kfm)->data_stack[(kfm)->dsp++] = value;	} while(0) // force stmt usage
#define Kforth_Call_Stack_Push(kfm, loc)		do { (kfm)->call_stack[(kfm)->csp++] = loc;		} while(0) // force stmt usage
#define Kforth_Machine_Terminate(kfm)			do { (kfm)->loc.cb = -1;						} while(0) // force stmt usage

/***********************************************************************
 * PROTOTYPES (private to this library)
 */

/*
 * cell.cpp
 */
extern int	Mark_Reachable_Cells(UNIVERSE *u, CELL *cell, int color);
extern int	Mark_Reachable_Cells_Alive(UNIVERSE *u, CELL *cell, int color);

/*
 * organism.cpp
 */
int Kill_Dead_Cells(UNIVERSE *u, ORGANISM *o);
int Kill_Organism(UNIVERSE *u, ORGANISM *o, int ex, int ey);

/*
 * evolve_io_ascii.cpp
 */
extern UNIVERSE			*Universe_ReadAscii(const char *filename, char *errbuf);
extern int				Universe_WriteAscii(UNIVERSE *u, const char *filename, char *errbuf);

//////////////////////////////////////////////////////////////////////
///
/// PORTING BEGIN
///
//////////////////////////////////////////////////////////////////////

#define __macosx__
//#define __linux__
//#define __windows__

#ifdef __windows__
//
// WINDOWS
//
#endif // __windows__

#ifdef __linux__
//
// LUNIX PORTING STUFF
//
#define stricmp(x,y)            strcasecmp(x,y)
#endif // __linux__

#ifdef __macosx__
//
// MACOS PORTING STUFF
//
#define stricmp(x,y)            strcasecmp(x,y)

#if DEBUG
#define EVOLVE_DEBUG
#endif

#endif // __macosx__

#ifdef EVOLVE_DEBUG
#define ASSERT(x)			assert(x)
#else
#define ASSERT(x)
#endif

#define MALLOC(x)			malloc(x)
#define CALLOC(n,s)			calloc(n,s)
#define REALLOC(p,s)		realloc(p,s)
#define FREE(x)				free(x)
#define STRDUP(x)			strdup(x)

//////////////////////////////////////////////////////////////////////
///
/// PORTING END
///
//////////////////////////////////////////////////////////////////////

//
// Common includes
//
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#endif
