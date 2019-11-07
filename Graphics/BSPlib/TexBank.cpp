/***************************************************************************\
    TexBank.cpp
    Scott Randolph
    March 4, 1998

    Provides the bank of textures used by all the BSP objects.
\***************************************************************************/
#include "stdafx.h"
#include <io.h>
#include <fcntl.h>
#include "Utils\lzss.h"
#include "Loader.h"
#include "grinline.h"
#include "StateStack.h"
#include "ObjectLOD.h"
#include "PalBank.h"
#include "TexBank.h"


// Static data members (used to avoid requiring "this" to be passed to every access function)
int					TextureBankClass::nTextures			= 0;
TexBankEntry*		TextureBankClass::TexturePool		= NULL;
BYTE*				TextureBankClass::CompressedBuffer	= NULL;	
//int					TextureBankClass::TextureFile		= -1;
int					TextureBankClass::deferredLoadState	= 0;
FileMemMap  TextureBankClass::TexFileMap;
#ifdef _DEBUG
int		    TextureBankClass::textureCount= 0;
#endif

#ifdef USE_SH_POOLS
extern MEM_POOL gBSPLibMemPool;
#endif

extern bool g_bUseMipMaps, g_bUseMappedFiles;

// Management functions
void TextureBankClass::Setup( int nEntries )
{
	// Create our array of texture headers
	nTextures	= nEntries;
	if (nEntries) {
		#ifdef USE_SH_POOLS
		TexturePool = (TexBankEntry *)MemAllocPtr(gBSPLibMemPool, sizeof(TexBankEntry)*nEntries, 0 );
		#else
		TexturePool	= new TexBankEntry[nEntries];
		#endif
	} else {
		TexturePool = NULL;
	}
}


void TextureBankClass::Cleanup( void )
{
	// Clean up our array of texture headers
	#ifdef USE_SH_POOLS
	MemFreePtr( TexturePool );
	#else
	delete[] TexturePool;
	#endif
	TexturePool = NULL;
	nTextures = 0;

	// Clean up the decompression buffer
	#ifdef USE_SH_POOLS
	MemFreePtr( CompressedBuffer );
	#else
	delete[] CompressedBuffer;
	#endif
	CompressedBuffer = NULL;

	// Close our texture resource file
	if (TexFileMap.IsReady()) {
		CloseTextureFile();
	}
}


void TextureBankClass::ReadPool( int file, char *basename )
{
	int		result;
	char	drive[_MAX_DRIVE];
	char	dir[_MAX_DIR];
	char	path[_MAX_PATH];
	int		maxCompressedSize;

	// Read the number of textures in the pool
	result = read( file, &nTextures, sizeof(nTextures) );

	// Read the size of the biggest compressed texture in the pool
	result = read( file, &maxCompressedSize, sizeof(maxCompressedSize) );

	// Allocate memory for later decompression
	#ifdef USE_SH_POOLS
	CompressedBuffer = (BYTE *)MemAllocPtr(gBSPLibMemPool, sizeof(BYTE)*maxCompressedSize, 0 );
	#else
	CompressedBuffer = new BYTE[ maxCompressedSize ];
	#endif
	ShiAssert( CompressedBuffer );
	if(CompressedBuffer) ZeroMemory(CompressedBuffer, maxCompressedSize);

	// Setup the pool
	_splitpath( basename, drive, dir, NULL, NULL );
	strcpy( path, drive );
	strcat( path, dir );
	Setup( nTextures );

	// Quite now if there are not textures in the pool
	if (nTextures == 0) {
		return;
	}

	#if 1
	// Read our texture pool
	result = read( file, TexturePool, sizeof(*TexturePool)*nTextures );
	if (result < 0 ) {
		char message[256];
		sprintf( message, "Reading object texture bank:  %s", strerror(errno) );
		ShiError( message );
	}
	#else
	// OW bla
	TexBankEntry_Old *pTmpEntries;
	#ifdef USE_SH_POOLS
	pTmpEntries = (TexBankEntry_Old *)MemAllocPtr(gBSPLibMemPool, sizeof(TexBankEntry_Old)*nTextures, 0 );
	#else
	pTmpEntries	= new TexBankEntry_Old[nEntries];
	#endif

	// Read our texture pool
	result = read(file, pTmpEntries, sizeof(*pTmpEntries)*nTextures);

	if(result < 0 )
	{
		#ifdef USE_SH_POOLS
		MemFreePtr(pTmpEntries);
		#else
		delete[] pTmpEntries;
		#endif

		char message[256];
		sprintf( message, "Reading object texture bank:  %s", strerror(errno) );
		ShiError( message );
	}

	// convert
	for(int i=0;i<nTextures;i++)
		TexturePool[i] = pTmpEntries[i];

	#ifdef USE_SH_POOLS
	MemFreePtr(pTmpEntries);
	#else
	delete[] pTmpEntries;
	#endif
	#endif

	if(g_bUseMipMaps)
	{
		// Enable mip mapping for all textures used by BSPs
		for(int i=0;i<nTextures;i++)
			TexturePool[i].tex.flags |= MPR_TI_MIPMAP;
	}

	// Open our texture resource file
	OpenTextureFile( basename );
}


void TextureBankClass::OpenTextureFile( char *basename )
{
	char	filename[_MAX_PATH];

	ShiAssert( !TexFileMap.IsReady() );

	// Open the texture image file
	strcpy( filename, basename );
	strcat( filename, ".TEX" );
#if 0
	TextureFile = open( filename, _O_RDONLY | _O_BINARY );
	if (TextureFile < 0) {
		char message[256];
		sprintf(message, "Failed to open object texture file %s\n", filename);
		ShiError( message );
	}
#endif
	if (!TexFileMap.Open(filename, FALSE, !g_bUseMappedFiles)) {
		char message[256];
		sprintf(message, "Failed to open object texture file %s\n", filename);
		ShiError( message );
	}
}


void TextureBankClass::CloseTextureFile( void )
{
	TexFileMap.Close();
//	close( TextureFile );
//	TextureFile = -1;
}


void TextureBankClass::Reference( int id )
{
	int		isLoaded;


	ShiAssert( IsValidIndex(id) );


	// Get our reference to this texture recorded to ensure it doesn't disappear out from under us
    EnterCriticalSection( &ObjectLOD::cs_ObjectLOD );

	isLoaded = TexturePool[id].refCount;
	ShiAssert( isLoaded >= 0 );
	TexturePool[id].refCount++;


	// If we already have the data, just verify that fact.  Otherwise, load it.
	if (isLoaded) {

		ShiAssert( TexturePool[id].tex.imageData || TexturePool[id].tex.TexHandle() || deferredLoadState );

	LeaveCriticalSection( &ObjectLOD::cs_ObjectLOD );

	} else {

		ShiAssert( TexFileMap.IsReady() );
		ShiAssert( CompressedBuffer );
		ShiAssert( TexturePool[id].tex.imageData == NULL );
		ShiAssert( TexturePool[id].tex.TexHandle() == NULL );
	
		// Get the palette pointer
		TexturePool[id].tex.palette = &ThePaletteBank.PalettePool[ TexturePool[id].palID ];
		ShiAssert( TexturePool[id].tex.palette );
		TexturePool[id].tex.palette->Reference();

    LeaveCriticalSection( &ObjectLOD::cs_ObjectLOD );

		if (!deferredLoadState) {
			ReadImageData( id );
		}
	}
}


// The call to this function are enclosed in the critical section cs_ObjectLOD by ObjectLOD::Unload()
void TextureBankClass::Release( int id )
{
	ShiAssert( IsValidIndex(id) );

	ShiAssert( TexturePool[id].refCount > 0 );
	if (TexturePool[id].refCount > 0)
	    TexturePool[id].refCount--;

	if (TexturePool[id].refCount == 0) {
		TexturePool[id].tex.FreeAll();
#ifdef _DEBUG
		textureCount --;
#endif
	}
}


void TextureBankClass::ReadImageData( int id )
{
	int		retval;
	int		size;
	BYTE *cdata;
	
	ShiAssert( TexturePool[id].refCount );

	if (g_bUseMappedFiles) {
	    cdata = TexFileMap.GetData(TexturePool[id].fileOffset, TexturePool[id].fileSize);
	    ShiAssert(cdata);
	}
	else {
	    if (!TexFileMap.ReadDataAt(TexturePool[id].fileOffset, CompressedBuffer, TexturePool[id].fileSize)) {
		char	message[120];
		sprintf( message, "%s:  Bad object texture seek (%0d)", strerror( errno ), TexturePool[id].fileOffset );
		ShiError( message );
	    }
	    cdata = CompressedBuffer;
	}

	
	// Allocate memory for the new texture
	size = TexturePool[id].tex.dimensions;
	size = size*size;
	TexturePool[id].tex.imageData = glAllocateMemory( size, FALSE );
	ShiAssert( TexturePool[id].tex.imageData );
	
	// Uncompress the data into the texture structure
	retval = LZSS_Expand( cdata, (BYTE*)TexturePool[id].tex.imageData, size );
	ShiAssert( retval == TexturePool[id].fileSize );
#ifdef _DEBUG
	textureCount ++;
#endif
}


void TextureBankClass::SetDeferredLoad( BOOL state )
{
	LoaderQ		*request;

	// Allocate space for the async request
	request = new LoaderQ;
	if (!request) {
		ShiError( "Failed to allocate memory for a object texture load state change request" );
	}

	// Build the data transfer request to get the required object data
	request->filename	= NULL;
	request->fileoffset	= 0;
	request->callback	= LoaderCallBack;
	request->parameter	= (void*)state;

	// Submit the request to the asynchronous loader
	TheLoader.EnqueueRequest( request );
}


void TextureBankClass::LoaderCallBack( LoaderQ* request )
{
	BOOL state = (int)request->parameter;

    EnterCriticalSection( &ObjectLOD::cs_ObjectLOD );

	// If we're turning deferred loads off, go back and do all the loads we held up
	if (deferredLoadState && !state)
	{
		// Check each texture
		for (int id=0; id<nTextures; id++) {

			// See if it is in use
			if (TexturePool[id].refCount) {

				// This one is in use.  Is it already loaded?
				if (!TexturePool[id].tex.imageData && !TexturePool[id].tex.TexHandle()) {

					// Nope, go get it.
					ReadImageData( id );
				}
			}
		}
	}

	// Now store the new state
	deferredLoadState = state;

    LeaveCriticalSection( &ObjectLOD::cs_ObjectLOD );

	// Free the request queue entry
	delete request;
}


void TextureBankClass::FlushHandles( void )
{
	int id;

	for (id=0; id<nTextures; id++) {

		ShiAssert( TexturePool[id].refCount == 0 );

#if 1  // If a quick hack is required to clean up, this would be it...
		while (TexturePool[id].refCount > 0) { // JPO -ve is very bad
			Release( id );
		}
#endif
	}
}


void TextureBankClass::Select( int id )
{
	ShiAssert( IsValidIndex(id) );
	if (IsValidIndex(id))
	{
		ShiAssert( TexturePool[id].refCount > 0 );

		// If we haven't sent the texture to MPR yet, do it
		if (TexturePool[id].tex.TexHandle() == NULL) {
			TexturePool[id].tex.CreateTexture();
			TexturePool[id].tex.FreeImage();
		}

		// Select this texture into the MPR context
		TheStateStack.context->SelectTexture( TexturePool[id].tex.TexHandle());
	}
}


BOOL TextureBankClass::IsValidIndex( int id ) {
	return ((id >= 0) && (id < nTextures));
}


// OW
void TextureBankClass::RestoreAll()
{
}
