/***************************************************************************\
    FarTex.cpp
    Scott Randolph
    May 22, 1997

	Maintain the list of distant terrain textures.
\***************************************************************************/
#include "stdafx.h"
#include <stdio.h>
#include "TimeMgr.h"
#include "TOD.h"
#include "Image.h"
#include "FarTex.h"
#include "Falclib\Include\openfile.h"
#include "Falclib\Include\IsBad.h"

#ifdef USE_SH_POOLS
MEM_POOL gFartexMemPool;
#endif

#define _USE_TEX_POOL

#ifdef _USE_TEX_POOL
#include "TexPool.h"
#endif

extern bool g_bEnableStaticTerrainTextures;
extern bool g_bUseMipMaps;
extern bool g_bUseMappedFiles;

// OW avoid having the TextureHandle maintaining a copy of the texture bitmaps
#define _DONOT_COPY_BITS

// The one and only Distant Texture Database object
FarTexDB	TheFarTextures;

static const DWORD	INVALID_TEXID	= 0xFFFFFFFF;
//static const int	IMAGE_SIZE		= 32;

#ifdef _USE_TEX_POOL
typedef Containers::Pool<TextureHandle> TextureHandlePool;

static void THP_NECB(TextureHandle *pNewHandle, LPVOID Ctx)
{
	FarTexDB *pThis = (FarTexDB *) Ctx;

	pThis->palHandle->AttachToTexture(pNewHandle);

	DWORD dwFlags = NULL;
	WORD info = MPR_TI_PALETTE;

	if(g_bEnableStaticTerrainTextures)
		dwFlags |= TextureHandle::FLAG_HINT_STATIC;

	if(g_bUseMipMaps)
		info |= MPR_TI_MIPMAP;

	bool bResult = pNewHandle->Create("FarTexDB", info, 8, IMAGE_SIZE, IMAGE_SIZE, dwFlags);
	ShiAssert(bResult);
}

TextureHandlePool *m_pTHP = NULL;
#endif

//#pragma optimize( "", off )

/***************************************************************************\
	Setup the texture database
\***************************************************************************/
BOOL FarTexDB::Setup( DXContext *hrc, const char* path )
{
	char	filename[MAX_PATH];
	HANDLE	listFile;	
	BOOL	result;
	DWORD	bytesRead;
	int		i;
	DWORD	tilesAtLOD;

  
	ShiAssert( hrc );
	ShiAssert( path );

	#ifdef USE_SH_POOLS
	gFartexMemPool = MemPoolInitFS( sizeof(BYTE)*IMAGE_SIZE*IMAGE_SIZE, 24, MEM_POOL_SERIALIZE );
	#endif

	// Store the rendering context to be used just for managing our textures
	private_rc = hrc;
	ShiAssert( private_rc );

	// Initialize data members to default values
	texCount = 0;
#ifdef _DEBUG
	LoadedTextureCount = 0;
	ActiveTextureCount = 0;
#endif

	// Create the synchronization objects we'll need
	InitializeCriticalSection( &cs_textureList );

	// Open the texture database description file
	sprintf( filename, "%s%s", path, "FarTiles.PAL" );
	listFile = CreateFile_Open( filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if (listFile == INVALID_HANDLE_VALUE) {
		//char	string[80];
		//char	message[120];
		//PutErrorString( string );
		//sprintf( message, "%s:  Couldn't open far texture list %s", string, filename );
		//ShiError( message );
		// We need to exit the game if they select cancel/abort from the file open dialog
	}

	// Read the palette data shared by all the distant textures
	result = ReadFile( listFile, &palette, sizeof(palette), &bytesRead, NULL );
	if (!result) {
		char	string[80];
		char	message[120];
		PutErrorString( string );
		sprintf( message, "%s:  Couldn'd read far texture palette.", string );
		ShiError( message );
	}

	// Read the number of textures in the database.
	texCount = 0;
	tilesAtLOD = 0;
	do {
		texCount += tilesAtLOD;
		result = ReadFile( listFile, &tilesAtLOD, sizeof(tilesAtLOD), &bytesRead, NULL );
	} while (bytesRead == sizeof(tilesAtLOD));


	// Now close the far texture palette file
	CloseHandle( listFile );


	// Create the MPR palette we'll use
// OW FIXME
	palHandle = new PaletteHandle(private_rc->m_pDD, 32, 256);
	ShiAssert( palHandle );

#ifdef _USE_TEX_POOL
	m_pTHP = new TextureHandlePool(16, THP_NECB, this);
	ShiAssert(m_pTHP);
#endif

	// Allocate memory for the texture records.
	//ShiAssert( texCount < 0xFFFF ); // now 32 bits
	texArray = new FarTexEntry[ texCount ];
	if (!texArray)
		ShiError( "Failed to allocate memory for the distant texture array." );

	// Set up the texture array
	for (i=0; i<texCount; i++)
	{
		// For now just initialize the headers.  Could read in tile specific info here
		texArray[i].bits		= NULL;
		texArray[i].handle		= NULL;
		texArray[i].refCount	= 0;
	}

	// Open and hang onto the distant texture composite image file
	sprintf( filename, "%s%s", path, "FarTiles.RAW" );
	fartexFile.Open(filename, FALSE, !g_bUseMappedFiles);
	// Initialize the lighting conditions and register for future time of day updates
	TimeUpdateCallback( this );
	TheTimeManager.RegisterTimeUpdateCB( TimeUpdateCallback, this );


	return TRUE;
}


void FarTexDB::Cleanup(void)
{
	ShiAssert( IsReady() );

	// Stop receiving time updates
	TheTimeManager.ReleaseTimeUpdateCB( TimeUpdateCallback, this);

	// Free the entire texture list
	for(int i=0; i<texCount; i++) 
	{
		Deactivate(static_cast<UInt32>(i));
		Free(static_cast<UInt32>(i));
	}

	delete[] texArray;
	texArray = NULL;

	// Free the MPR palette
	if(palHandle)
	{
		delete palHandle;
		palHandle = NULL;
	}

	// We no longer need our texture managment RC
	private_rc = NULL;

	// Close the distant texture image file
	fartexFile.Close();

	ShiAssert( ActiveTextureCount == 0 );
	ShiAssert( LoadedTextureCount == 0 );

	// Release the sychronization objects we've been using
	DeleteCriticalSection( &cs_textureList );

#ifdef _USE_TEX_POOL
	if(m_pTHP)
	{
		delete m_pTHP;
		m_pTHP = NULL;
	}
#endif

	#ifdef USE_SH_POOLS
	MemPoolFree( gFartexMemPool );
	#endif
}


// Set the light level applied to the terrain textures.
void FarTexDB::TimeUpdateCallback( void *self )
{
	((FarTexDB*)self)->SetLightLevel();
}

void FarTexDB::SetLightLevel( void )
{
	Tcolor	lightColor;			// Current light color
	DWORD	scratchPal[256];	// Scratch palette for doing lighting calculations
	BYTE	*to, *from, *stop;	// Pointers used to walk through the palette

	ShiAssert( palette );
	ShiAssert( palHandle );


	// Decide what color to use for lighting
	if(TheTimeOfDay.GetNVGmode())
	{
		lightColor.r = 0.0f;
		lightColor.g = NVG_LIGHT_LEVEL;
		lightColor.b = 0.0f;
	}

	else
		TheTimeOfDay.GetTextureLightingColor( &lightColor );

	// Apply the current lighting
	from	= (BYTE*)palette;
	to		= (BYTE*)scratchPal;
	stop	= to + 256*4;

	while (	to < stop ) 
	{
		*to = static_cast<BYTE>(FloatToInt32(*from * lightColor.r));to++, from++;	// Red
		*to = static_cast<BYTE>(FloatToInt32(*from * lightColor.g));to++, from++;	// Green
		*to = static_cast<BYTE>(FloatToInt32(*from * lightColor.b));to++, from++;	// Blue
		to++, from++;	// Alpha
	}

	// Turn on the lights if it is dark enough
	if (TheTimeOfDay.GetLightLevel() < 0.5f)
	{
		to = (BYTE*)&(scratchPal[252]);

		if(TheTimeOfDay.GetNVGmode())
		{
			*to = 0;		to++;	// Red
			*to = 255;		to++;	// Green
			*to = 0;		to++;	// Blue
			*to = 255;		to++;	// Alpha

			*to = 0;		to++;	// Red
			*to = 255;		to++;	// Green
			*to = 0;		to++;	// Blue
			*to = 255;		to++;	// Alpha

			*to = 0;		to++;	// Red
			*to = 255;		to++;	// Green
			*to = 0;		to++;	// Blue
			*to = 255;		to++;	// Alpha

			*to = 0;		to++;	// Red
			*to = 255;		to++;	// Green
			*to = 0;		to++;	// Blue
			*to = 255;		to++;	// Alpha
		}

		else
		{
			*to = 115;		to++;	// Red
			*to = 171;		to++;	// Green
			*to = 155;		to++;	// Blue
			*to = 255;		to++;	// Alpha

			*to = 183;		to++;	// Red
			*to = 127;		to++;	// Green
			*to = 83;		to++;	// Blue
			*to = 255;		to++;	// Alpha

			*to = 171;		to++;	// Red
			*to = 179;		to++;	// Green
			*to = 139;		to++;	// Blue
			*to = 255;		to++;	// Alpha

			*to = 171;		to++;	// Red
			*to = 171;		to++;	// Green
			*to = 171;		to++;	// Blue
			*to = 255;		to++;	// Alpha
		}
	}

	// Update MPR's palette
	palHandle->Load(MPR_TI_PALETTE,	// Palette info
							32,				// Bits per entry
							0,				// Start index
							256,			// Number of entries
							(BYTE*)&scratchPal);
}


// This function is called by anyone wishing the use of a particular texture.
// If the texture is not already loaded, this will do the I/O required.

void FarTexDB::Request(TextureID texID)
{
	BOOL needToLoad;

	ShiAssert( IsReady() );

	if (texID == INVALID_TEXID)
		return;


	ShiAssert( texID >= 0 );
	ShiAssert( texID < texCount );

	EnterCriticalSection(&cs_textureList);

	ShiAssert(texArray);
	if (!texArray)	// 2002-04-13 MN CTD fix
		return;

	// If this is the first reference, we need to load the data
	needToLoad = (texArray[texID].refCount == 0);

	// Increment our reference count.
	texArray[texID].refCount++;

	LeaveCriticalSection( &cs_textureList );

	if(needToLoad)
	{
		ShiAssert( texArray[texID].bits == NULL );
		ShiAssert( texArray[texID].handle == NULL );

		Load(texID);
		ShiAssert( texArray[texID].bits);
	}
}


// This function must eventually be called by anyone who calls the Request function above.
// This call will decrease the reference count of the texture and if it reaches zero, remove
// it from memory.
void FarTexDB::Release( TextureID texID )
{
	ShiAssert( IsReady() );

	if(texID == INVALID_TEXID)
		return;

	ShiAssert( texID >= 0 );
	ShiAssert( texID < texCount );

	EnterCriticalSection( &cs_textureList );

	// Release our hold on this texture
	texArray[texID].refCount--;

	if (texArray[texID].refCount == 0)
	{
		Deactivate( texID );
		Free( texID );
	}

	LeaveCriticalSection( &cs_textureList );
}

	
// This function reads texel data from disk.  It reads only the resolution
// level that is requested.

void FarTexDB::Load( DWORD offset )
{

	ShiAssert( IsReady() );
	ShiAssert( offset >= 0 );
	ShiAssert( offset < texCount );
	ShiAssert( texArray[offset].bits == NULL );
	
	if (g_bUseMappedFiles) {
	    texArray[offset].bits = fartexFile.GetFarTex(offset);
	    ShiAssert( texArray[offset].bits );
	}
	else {
	    
	    // Allocate space for the bitmap
#ifdef USE_SH_POOLS
	    texArray[offset].bits = (BYTE *)MemAllocFS( gFartexMemPool );
#else
	    texArray[offset].bits = new BYTE[ IMAGE_SIZE*IMAGE_SIZE ];
#endif
	    ShiAssert( texArray[offset].bits );
	    
	    // Read the image data
	    if( !fartexFile.ReadDataAt(offset*IMAGE_SIZE*IMAGE_SIZE, texArray[offset].bits, IMAGE_SIZE*IMAGE_SIZE) )
	    {
		char	string[80];
		char	message[120];
		PutErrorString( string );
		sprintf( message, "%s:  Couldn'd read far texture image %0d.", string, offset );
		ShiError( message );
	    }
	}
#ifdef _DEBUG
	// Temporary.  Keep count of how many textures are loaded at once
	LoadedTextureCount++;
#endif
}


// This function sends texture data to MPR
void FarTexDB::Activate( DWORD offset )
{
	ShiAssert( IsReady() );
	ShiAssert( offset >= 0 );
	ShiAssert( offset < texCount );
	ShiAssert( texArray[offset].bits != NULL );
	ShiAssert( texArray[offset].handle == NULL );

#ifdef _USE_TEX_POOL
	texArray[offset].handle = (UInt) m_pTHP->GetElement();
#else
	texArray[offset].handle = (UInt) new TextureHandle;
	ShiAssert( texArray[offset].handle );
	palHandle->AttachToTexture((TextureHandle *) texArray[offset].handle);

	DWORD dwFlags = NULL;
	WORD info = MPR_TI_PALETTE;

	if(g_bEnableStaticTerrainTextures)
		dwFlags |= TextureHandle::FLAG_HINT_STATIC;

	if(g_bUseMipMaps)
		info |= MPR_TI_MIPMAP;

	((TextureHandle *) texArray[offset].handle)->Create("FarTexDB", info, 8, IMAGE_SIZE, IMAGE_SIZE, dwFlags);
#endif

#ifdef _DONOT_COPY_BITS
	((TextureHandle *) texArray[offset].handle)->Load(0, 0, texArray[offset].bits, false, true);
#else
	((TextureHandle *) texArray[offset].handle)->Load(0, 0, texArray[offset].bits);
#endif

	// Now that we don't need the local copy of the image, drop it
	Free( offset );

#ifdef _DEBUG
	// Temporary.  Keep count of how many textures are active at once
	ActiveTextureCount++;
#endif
}


// This function will release the MPR handle for the specified texture.  When
// the last low resolution reference to a tile is released, its memory image
// will also be released and the set reference count reduced.
void FarTexDB::Deactivate( DWORD offset )
{
	ShiAssert( IsReady() );
	ShiAssert( offset >= 0 );
	ShiAssert( offset < texCount );
	ShiAssert( texArray[offset].refCount == 0 );

	if(texArray == 0)
		return;

	// Quit now if we've got nothing to do
	if(texArray[offset].handle)
	{
		#ifdef _USE_TEX_POOL
			TextureHandle *pTex = (TextureHandle *) texArray[offset].handle;
			if (!F4IsBadReadPtr(pTex, sizeof(TextureHandle))) // JB 010326 CTD
			{
				pTex->m_pImageData = NULL;	// we need to set this to zero because the texture is still attached to the palette  but the pointer will soon become invalid
				m_pTHP->FreeElement(pTex);
			}
		#else
			// Release the texture from the MPR context
			delete (TextureHandle *) texArray[offset].handle;
		#endif
			texArray[offset].handle = NULL;

		#ifdef _DEBUG
			// Temporary.  Keep count of how many textures are active at once
			ActiveTextureCount--;
		#endif
	}

#ifdef _DONOT_COPY_BITS
	if(texArray[offset].bits)
	{
	    if (!g_bUseMappedFiles) {
		#ifdef USE_SH_POOLS
		MemFreeFS( texArray[offset].bits );
		#else
		delete[] texArray[offset].bits;
		#endif
	    }
	    texArray[offset].bits = NULL;

		#ifdef _DEBUG
		// Temporary counter used to track texture usage
		LoadedTextureCount--;
		#endif
	}
#endif
}


// This function will release the memory image of a texture.
// It is called when the reference count of a texture reaches zero OR
// when the texture is moved into MPR and a local copy is no longer required.
void FarTexDB::Free( DWORD offset ) {
	ShiAssert( IsReady() );
	ShiAssert( offset >= 0 );
	ShiAssert( offset < texCount );

	// Quit now if we've got nothing to do
	if(texArray[offset].bits == NULL )
		return;

	// Rlease the image memory
#ifndef _DONOT_COPY_BITS
	if (!g_bUseMappedFiles) {
	#ifdef USE_SH_POOLS
	MemFreeFS( texArray[offset].bits );
	#else
	delete[] texArray[offset].bits;
	#endif
	}
	texArray[offset].bits = NULL;

	#ifdef _DEBUG
	// Temporary counter used to track texture usage
	LoadedTextureCount--;
	#endif
#endif
}


// Select a "Load"ed texture into an RC for immediate use by the rasterizer
void FarTexDB::Select( ContextMPR *localContext, TextureID texID )
{
	ShiAssert( IsReady() );
	ShiAssert( localContext );

	if (texID == INVALID_TEXID) {
		return;
	}

	ShiAssert( texID >= 0 );
	ShiAssert( texID < texCount );

	// Make sure the texture we're trying to use is local to MPR
	if (texArray[texID].handle == NULL)
	{
		ShiAssert( texArray[texID].bits );

		Activate( texID );
	}

	ShiAssert( texArray[texID].handle );

	// Make the required texture the current texture in the rendering context
	localContext->SelectTexture(texArray[texID].handle);
}

// OW
void FarTexDB::RestoreAll()
{
	EnterCriticalSection( &cs_textureList );

	// Free the entire texture list
	for(int i=0; i<texCount; i++) 
	{
		if(texArray[i].handle)
			((TextureHandle *) texArray[i].handle)->RestoreAll();
	}

	LeaveCriticalSection( &cs_textureList );
}

//#pragma optimize( "", on )
