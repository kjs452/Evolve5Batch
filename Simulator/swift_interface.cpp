//
//  swift_interface.cpp
//  Evolve5
//
//  Created by Kenneth Stauffer on 10/22/22.
//

#include "evolve_simulator.h"
#include "evolve_simulator_private.h"

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

//
// This weird stuff allows data from the Swift/Data/NSData world to be
// read and written by my old C/C++ code.
//

extern "C" void EvolveFileBridge_rewind();
extern "C" intptr_t EvolveFileBridge_read(char *buf, intptr_t reqlen);
extern "C" intptr_t EvolveFileBridge_write(const char *buf, intptr_t len);

void Universe_Write_Using_CB(UNIVERSE *u)
{
	char errbuf[1000];
	
	Universe_WriteAscii_CB(u, "noname", EvolveFileBridge_write, errbuf);
}

UNIVERSE *Universe_Read_Using_CB()
{
	char errbuf[1000];
	UNIVERSE *u;
	
	u = Universe_ReadAscii_CB("noname", EvolveFileBridge_read, errbuf);
	if( u == NULL ) {
		printf("Universe_Read_Using_CB: %s\n", errbuf);
	}
	return u;
}


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//
// Gross API's For Swift interfacing
//

KFORTH_INTEGER kforth_machine_ith_data_stack(KFORTH_MACHINE *kfm, int i)
{
	return kfm->data_stack[i];
}

KFORTH_INTEGER kforth_machine_ith_register(KFORTH_MACHINE *kfm, int i)
{
	return kfm->R[i];
}

KFORTH_LOC kforth_machine_ith_call_stack(KFORTH_MACHINE *kfm, int i)
{
	return kfm->call_stack[i];
}

CELL_SENSE_DATA_ITEM CellSensoryData_ith_item(CELL_SENSE_DATA *csd, int dir)
{
	return csd->dirs[dir];
}

KFORTH_INTEGER Universe_Get_Strain_Global(UNIVERSE *u, int strain)
{
	return u->S0[strain];
}

KFORTH_MUTATE_OPTIONS* Universe_get_ith_kfmo(UNIVERSE *u, int i)
{
	ASSERT( u != NULL );
	ASSERT( i >= 0 && i < 8 );

	return &u->kfmo[i];
}

KFORTH_OPERATIONS* Universe_get_ith_kfops(UNIVERSE *u, int i)
{
	ASSERT( u != NULL );
	ASSERT( i >= 0 && i < 8 );

	return &u->kfops[i];
}

STRAIN_OPTIONS* Universe_get_ith_strop(UNIVERSE *u, int i)
{
	ASSERT( u != NULL );
	ASSERT( i >= 0 && i < 8 );

	return &u->strop[i];
}

void Universe_set_ith_kfmo(UNIVERSE *u, int i, KFORTH_MUTATE_OPTIONS* kfmo)
{
	ASSERT( u != NULL );
	ASSERT( i >= 0 && i < 8 );
	ASSERT( kfmo != NULL );

	u->kfmo[i] = *kfmo;
}

void Universe_set_ith_kfops(UNIVERSE *u, int i, KFORTH_OPERATIONS* kfops)
{
	ASSERT( u != NULL );
	ASSERT( i >= 0 && i < 8 );
	ASSERT( kfops != NULL );

	u->kfops[i] = *kfops;
}

void Universe_set_ith_strop(UNIVERSE *u, int i, STRAIN_OPTIONS* strop)
{
	ASSERT( u != NULL );
	ASSERT( i >= 0 && i < 8 );
	ASSERT( strop != NULL );

	u->strop[i] = *strop;
}

void StrainOptions_Set_Name(STRAIN_OPTIONS *so, const char *name)
{
	ASSERT( so != NULL );
	ASSERT( name != NULL );

	strlcpy(so->name, name, sizeof(so->name));
}

KFORTH_OPERATIONS* StrainProfile_get_kfops(STRAIN_PROFILE *sp)
{
	ASSERT( sp != NULL );

	return &sp->kfops;
}

EVOLVE_DFLT* EvolvePreferences_get_ith_dflt(EVOLVE_PREFERENCES* ep, int i)
{
	ASSERT( ep != NULL );
	ASSERT( i >= 0 && i <= 7 );

	return &ep->dflt[i];
}

void EvolvePreferences_set_ith_dflt_seed_file(EVOLVE_PREFERENCES* ep, int i, const char *seed_file)
{
	ASSERT( ep != NULL );
	ASSERT( i >= 0 && i <= 7 );

	EVOLVE_DFLT *ed;

	ed = &ep->dflt[i];

	strlcpy(ed->seed_file, seed_file, sizeof(ed->seed_file));
}

void EvolvePreferences_set_terrain_file(EVOLVE_PREFERENCES* ep, const char *terrain_file)
{
	ASSERT( ep != NULL );
	ASSERT( terrain_file != NULL );

	strlcpy(ep->terrain_file, terrain_file, sizeof(ep->terrain_file));
}

void NewUniverse_Set_TerrainFile(NEW_UNIVERSE_OPTIONS *nuo, const char *terrain_file)
{
	ASSERT( nuo != NULL );
	ASSERT( terrain_file != NULL );

	strlcpy(nuo->terrain_file, terrain_file, sizeof(nuo->terrain_file));
}
