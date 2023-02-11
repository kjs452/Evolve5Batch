/*
 * Copyright (c) 2022 Stauffer Computer Consulting
 */


/***********************************************************************
 * READ/WRITE operations
 *
 * This module determines what version the file is in, and
 * tries to migrate to the latest.
 *
 */
#include "evolve_simulator.h"
#include "evolve_simulator_private.h"
#include "phascii.h"

/*
 * Top-level call to read ANY simulation version (or ascii/binary)
 */
UNIVERSE *Universe_Read(const char *filename, char *errbuf)
{
	ASSERT( filename != NULL );
	ASSERT( errbuf != NULL );

	if( Phascii_FileIsPhotonAscii(filename) ) {
		return Universe_ReadAscii(filename, errbuf);
	} else {
		// KJS TODO: Decompress file if it is .txt.Z
		//return Universe_ReadBinary(filename, errbuf);
		return NULL;
	}
}

/*
 * Top-level call to write a simulation in the latest version
 * (will write to ascii format is the filename extension is .txt,
 * otherwise will write in binary format)
 */
int Universe_Write(UNIVERSE *u, const char *filename, char *errbuf)
{
	const char *ext;

	ASSERT( filename != NULL );
	ASSERT( errbuf != NULL );

	ext = strrchr(filename, '.');

	if( ext != NULL && stricmp(ext, ".txt") == 0 ) {
		return Universe_WriteAscii(u, filename, errbuf);
	} else {
		// KJS TODO: binary compress the file if it is .txt.Z
		//return Universe_WriteBinary(u, filename, errbuf);
		return NULL;
	}
}
