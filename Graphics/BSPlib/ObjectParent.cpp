/***************************************************************************\
    ObjectParent.cpp
    Scott Randolph
    February 9, 1998

    Provides structures and definitions for 3D objects.
\***************************************************************************/
#include "stdafx.h"
#include <io.h>
#include <fcntl.h>
#include "StateStack.h"
#include "TexBank.h"
#include "PalBank.h"
#include "ColorBank.h"
#include "ObjectParent.h"
#include "falclib\include\IsBad.h"


ObjectParent		*TheObjectList = NULL;
int					TheObjectListLength = 0;

#ifdef USE_SH_POOLS
MEM_POOL gBSPLibMemPool = NULL;
#endif

ObjectParent::ObjectParent()
{
	refCount					= 0;
	pLODs						= NULL;
	pSlotAndDynamicPositions	= NULL;
}


ObjectParent::~ObjectParent()
{
	ShiAssert( refCount == 0 );

	#ifdef USE_SH_POOLS
	MemFreePtr( pSlotAndDynamicPositions );
	#else
	delete[] pSlotAndDynamicPositions;
	#endif
	pSlotAndDynamicPositions = NULL;

	#ifdef USE_SH_POOLS
	MemFreePtr( pLODs );
	#else
	delete[] pLODs;
	#endif
	pLODs = NULL;
}


void ObjectParent::SetupTable( char *basename )
{
	int		file;
	char	filename[_MAX_PATH];

	#ifdef USE_SH_POOLS
	if ( gBSPLibMemPool == NULL )
	{
		gBSPLibMemPool = MemPoolInit( 0 );
	}
	#endif

	// Open the master object file
	strcpy( filename, basename );
	strcat( filename, ".HDR" );
	file = open( filename, _O_RDONLY | _O_BINARY | _O_SEQUENTIAL );
	if (file < 0) {
		char message[256];
		sprintf(message, "Failed to open object header file %s\n", filename);
		ShiError( message );
	}

	// Read the format version
	VerifyVersion( file );

	// Read the Color Table from the master file
	TheColorBank.ReadPool( file );

	// Read the Palette Table from the master file
	ThePaletteBank.ReadPool( file );

	// Read the Texture Table from the master file
	TheTextureBank.ReadPool( file, basename );

	// Read the object LOD headers from the master file
	ObjectLOD::SetupTable( file, basename );

	// Read the parent object records from the master file
	ReadParentList( file );

	// Close the master file
	close(file);
}


void ObjectParent::CleanupTable( void )
{
	// Make sure all the parent objects are freed
	FlushReferences();

	// Free the LOD table
	ObjectLOD::CleanupTable();

	// Free the texture, palette, and color banks
	TheTextureBank.Cleanup();
	ThePaletteBank.Cleanup();
	TheColorBank.Cleanup();

	// Free our array of parent objects
	#ifdef USE_SH_POOLS
	MemFreePtr( TheObjectList );
	#else
	delete[] TheObjectList;
	#endif
	TheObjectList = NULL;
	TheObjectListLength = 0;

	#ifdef USE_SH_POOLS
	if ( gBSPLibMemPool != NULL )
	{
		MemPoolFree(gBSPLibMemPool);
		gBSPLibMemPool = NULL;
	}
	#endif
}


void ObjectParent::ReadParentList( int file )
{
	ObjectParent	*objParent;
	ObjectParent	*end;
	int				i;
	int				result;


	// Read the length of the parent object array
	result = read( file, &TheObjectListLength, sizeof(TheObjectListLength) );


	// Allocate memory for the parent object array
	#ifdef USE_SH_POOLS
	TheObjectList = (ObjectParent *)MemAllocPtr(gBSPLibMemPool, sizeof(ObjectParent)*TheObjectListLength, 0 );
	#else
	TheObjectList = new ObjectParent[TheObjectListLength];
	#endif

	ShiAssert( TheObjectList );

	// Now read the elements of the parent array
	result = read( file, TheObjectList, sizeof(*TheObjectList)*TheObjectListLength );
	if (result < 0 ) {
		char message[256];
		sprintf( message, "Reading object list:  %s", strerror(errno) );
		ShiError( message );
	}


	// Finally, read the reference arrays for each parent in order
	end = TheObjectList + TheObjectListLength;
	for (objParent=TheObjectList; objParent<end; objParent++) {

		// Skip any unused objects
		if (objParent->nLODs == 0) {
			continue;
		}

		// Allocate and read this parent's slot and dynamic position array
		if (objParent->nSlots) {

			#ifdef USE_SH_POOLS
			objParent->pSlotAndDynamicPositions = (Tpoint *)MemAllocPtr(gBSPLibMemPool, sizeof(Ppoint)*(objParent->nSlots+objParent->nDynamicCoords), 0 );
			#else
			objParent->pSlotAndDynamicPositions = new Ppoint[objParent->nSlots+objParent->nDynamicCoords];
			#endif

			result = read(	file, 
							objParent->pSlotAndDynamicPositions, 
							(objParent->nSlots+objParent->nDynamicCoords)*sizeof(*objParent->pSlotAndDynamicPositions) );
		}
		else 
			objParent->pSlotAndDynamicPositions = NULL; // JPO - jsut in case its wrong in the file

		// Allocate memory for this parent's reference list

		#ifdef USE_SH_POOLS
		objParent->pLODs = (LODrecord *)MemAllocPtr(gBSPLibMemPool, sizeof(LODrecord)*(objParent->nLODs), 0 );
		#else
		objParent->pLODs = new LODrecord[objParent->nLODs];
		#endif

		// Read the reference list
		result = read( file, objParent->pLODs, objParent->nLODs*sizeof(*objParent->pLODs) );
		if (result < 0 ) {
			char message[256];
			sprintf( message, "Reading object reference list:  %s", strerror(errno) );
			ShiError( message );
		}

		// Fixup the LOD references
		ShiAssert( TheObjectLODs );
		for (i=0; i<objParent->nLODs; i++) {
	
			// Replace the offset of the LOD with a pointer into TheObjectLOD array.
			// NOTE:  We're shifting the offset right one bit to clear our special
			// marker.
			objParent->pLODs[i].objLOD = &TheObjectLODs[ ((int)(objParent->pLODs[i].objLOD)>>1) ];
		}
	}
}


void ObjectParent::VerifyVersion( int file )
{
	int				result;
	UInt32			fileVersion;
	char			message[80];

	// Read the magic number at the head of the file
	result = read( file, &fileVersion, sizeof(fileVersion) );

	// If the version doesn't match our expectations, report an error
	if (fileVersion != FORMAT_VERSION) {
		Beep( 2000, 500 );
		Beep( 2000, 500 );
		sprintf( message, "Got object format version 0x%08X, want 0x%08X", fileVersion, FORMAT_VERSION );
		ShiError( message );
	}
}


void ObjectParent::FlushReferences(void)
{
	// Make sure all the parent objects are freed
	for (int i=0; i<TheObjectListLength; i++) {
#if 1
		ShiAssert( TheObjectList[i].refCount == 0 );
#else
		while (TheObjectList[i].refCount) {
			TheObjectList[i].Release();
		}
#endif
	}
}


void ObjectParent::Reference(void)
{
	if (refCount==0) {
		// Reference each of our child LODs (loaded or not)
		LODrecord	*record = pLODs+nLODs-1;

		ShiAssert( FALSE == F4IsBadReadPtr(pLODs, sizeof *pLODs) );
		while (record >= pLODs) {
			ShiAssert(FALSE == F4IsBadReadPtr(record, sizeof *record)); // JPO CTD check
			//if (record && !F4IsBadReadPtr(record, sizeof(LODrecord)) && record->objLOD && !F4IsBadCodePtr((FARPROC) record->objLOD)) // JB 010221 CTD
			if (record && !F4IsBadReadPtr(record, sizeof(LODrecord)) && record->objLOD && !F4IsBadReadPtr(record->objLOD, sizeof(ObjectLOD))) // JB 010318 CTD
				record->objLOD->Reference();
			record--;
		}
	}

	refCount++;
}


void ObjectParent::ReferenceWithFetch(void)
{
	if (refCount==0) {
		// Reference each of our child LODs (loaded or not)
		LODrecord	*record = pLODs+nLODs-1;

		while (record >= pLODs) {
			ShiAssert(FALSE == F4IsBadReadPtr(record, sizeof *record)); // JPO CTD check
			record->objLOD->Reference();
			record->objLOD->Fetch();
			record--;
		}
	}

	refCount++;
}


void ObjectParent::Release(void)
{
	ShiAssert (refCount > 0);

	LODrecord	*record		= pLODs+nLODs-1;

	// Now reduce our reference count
	refCount--;
	if (refCount==0) {
		// Dereference each of our child LODs
		while (record >= pLODs) {
			record->objLOD->Release();
			record--;
		}
	}
}


ObjectLOD* ObjectParent::ChooseLOD(float range, int *used, float *max_range)
{
	LODrecord	*record		= NULL; // pLODs+nLODs-1;
	ObjectLOD	*objLODptr	= NULL;
	int
		LOD = nLODs - 1;

// 2002-03-29 MN possible CTD fix
	ShiAssert( FALSE == F4IsBadReadPtr(pLODs, sizeof *pLODs) );
	if (!pLODs)		
		return NULL;
	
	record = pLODs+nLODs-1;

	// Check each LOD from lowest detail to highest appropriate
	//while (record >= pLODs) {
	while (
		record && //!F4IsBadReadPtr(record, sizeof(LODrecord)) && // JB 010318 CTD (too high CPU)
		record >= pLODs) { 
	    ShiAssert( FALSE == F4IsBadReadPtr(record, sizeof *record)); // JPO CTD check

		// Quit if we've reached a record which has too much detail
		if (range > record->maxRange) {
			break;
		}

		// Save this candidate for drawing if its available
		if (record->objLOD->Fetch()) {
			objLODptr = record->objLOD;
			*max_range = record->maxRange;
			*used = LOD;
		}

		LOD --;
		record--;
		ShiAssert(record == NULL || FALSE == F4IsBadReadPtr(record, sizeof *record));
	}

	// Return our final drawing candidate (if any)
	return objLODptr;
}
