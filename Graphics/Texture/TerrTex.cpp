/***************************************************************************\
    TerrTex.cpp
    Scott Randolph
    October 2, 1995

	Hold information on all the terrain texture tiles.
\***************************************************************************/

#include "stdafx.h"
#include <stdio.h>
#include "TimeMgr.h"
#include "TOD.h"
#include "Image.h"
#include "TerrTex.h"
#include "Falclib\include\openfile.h"
#include "Falclib\Include\IsBad.h"

extern bool g_bEnableStaticTerrainTextures;
extern bool g_bUseMipMaps;

#ifdef USE_SH_POOLS
MEM_POOL gTexDBMemPool = NULL;
#endif

// OW avoid having the TextureHandle maintaining a copy of the texture bitmaps
#define _DONOT_COPY_BITS

// The one and only Texture Database object
TextureDB		TheTerrTextures;

/***************************************************************************\
	Setup the texture database
\***************************************************************************/
BOOL TextureDB::Setup( DXContext *hrc, const char* path )
{
	char	filename[MAX_PATH];
	HANDLE	listFile;	
	BOOL	result;
	DWORD	bytesRead;
	int		dataSize;
	int		i, j, k;

  
	ShiAssert( hrc );
	ShiAssert( path );

	#ifdef USE_SH_POOLS
	if ( gTexDBMemPool == NULL )
	{
		gTexDBMemPool = MemPoolInit( 0 );
	}
	#endif


	// Store the texture path for future reference
	if ( strlen( path )+1 >= sizeof( texturePath ) ) {
		ShiError( "Texture path name overflow!" );
	}
	strcpy( texturePath, path );
	if (texturePath[strlen(texturePath)-1] != '\\') {
		strcat( texturePath, "\\" );
	}


	// Store the rendering context to be used just for managing our textures
	private_rc = hrc;
	ShiAssert( private_rc != NULL);

	// Initialize data members to default values
	overrideHandle = NULL;
	numSets = 0;
#ifdef _DEBUG
	LoadedSetCount = 0;
	LoadedTextureCount = 0;
	ActiveTextureCount = 0;
#endif

	// Initialize the lighting conditions and register for future time of day updates
	TimeUpdateCallback( this );
	TheTimeManager.RegisterTimeUpdateCB( TimeUpdateCallback, this );

	// Create the synchronization objects we'll need
	InitializeCriticalSection( &cs_textureList );

	// Open the texture database description file
	strcpy( filename, texturePath );
	strcat( filename, "Texture.BIN" );
	listFile = CreateFile_Open( filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if (listFile == INVALID_HANDLE_VALUE) {
		//char	string[80];
		//char	message[120];
		//PutErrorString( string );
		//sprintf( message, "%s:  Couldn't open texture list - disk error?", string );
		//ShiError( message );
		// We need to exit the game if they select cancel/abort from the file open dialog
	}

	// Read the number of texture sets and tiles in the database.
	result = ReadFile( listFile, &numSets, sizeof(numSets), &bytesRead, NULL );
	if (result) {
		result = ReadFile( listFile, &totalTiles, sizeof(totalTiles), &bytesRead, NULL );
	}
	if (!result) {
		char	string[80];
		char	message[120];
		PutErrorString( string );
		sprintf( message, "%s:  Couldn'd read texture list.", string );
		MessageBox( NULL, message, "Proceeding with Empty List", MB_OK );
		numSets = 0;
	}


	// Allocate memory for the texture set records.
	#ifdef USE_SH_POOLS
	TextureSets = (SetEntry *)MemAllocPtr( gTexDBMemPool, sizeof(SetEntry ) * numSets, 0 ) ;
	#else
	TextureSets = new SetEntry[ numSets ];
	#endif
	if (!TextureSets) {
		ShiError( "Failed to allocate memory for the texture database." );
	}


	// Read the descriptions for the sets.
	for (i=0; i<numSets; i++) {

		// Read the description
		result = ReadFile( listFile, &TextureSets[i].numTiles, sizeof(TextureSets[i].numTiles), &bytesRead, NULL );
		if (result) {
			result = ReadFile( listFile, &TextureSets[i].terrainType, sizeof(TextureSets[i].terrainType), &bytesRead, NULL );
		}
		if (!result) {
			char	string[80];
			char	message[120];
			PutErrorString( string );
			sprintf( message, "%s:  Couldn't read set description - disk error?", string );
			ShiError( message );
		}

		// Mark the set as unused
		TextureSets[i].refCount = 0;
		TextureSets[i].palette = NULL;
		TextureSets[i].palHandle = NULL;


		// Allocate memory for the tile headers in this set
		if ( TextureSets[i].numTiles > 0 ) {
			#ifdef USE_SH_POOLS
			TextureSets[i].tiles = (TileEntry *)MemAllocPtr( gTexDBMemPool, sizeof(TileEntry ) * TextureSets[i].numTiles, 0 ) ;
			#else
			TextureSets[i].tiles = new TileEntry[ TextureSets[i].numTiles ];
			#endif
			ShiAssert( TextureSets[i].tiles );
		} else {
			TextureSets[i].tiles = NULL;
		}


		// Read the descriptions for the tiles within the sets.
		for (j=0; j<TextureSets[i].numTiles; j++) {

			// Read the tile name and area and path counts
			result = ReadFile( listFile, &TextureSets[i].tiles[j].filename, sizeof(TextureSets[i].tiles[j].filename), &bytesRead, NULL );
			if (result) {
				result = ReadFile( listFile, &TextureSets[i].tiles[j].nAreas, sizeof(TextureSets[i].tiles[j].nAreas), &bytesRead, NULL );
			}
			if (result) {
				result = ReadFile( listFile, &TextureSets[i].tiles[j].nPaths, sizeof(TextureSets[i].tiles[j].nPaths), &bytesRead, NULL );
			}
			if (!result) {
				char	string[80];
				char	message[120];
				PutErrorString( string );
				sprintf( message, "%s:  Couldn't read tile header - disk error?", string );
				ShiError( message );
			}

			// Start with this tile unused
			for (k=0; k<TEX_LEVELS; k++) {
				TextureSets[i].tiles[j].refCount[k]	= 0;
				TextureSets[i].tiles[j].bits[k]		= NULL;
				TextureSets[i].tiles[j].handle[k]	= NULL;
			}


			// Now read all the areas
			if ( TextureSets[i].tiles[j].nAreas == 0 ) {
				TextureSets[i].tiles[j].Areas = NULL;
			} else {
				dataSize = TextureSets[i].tiles[j].nAreas * sizeof(TexArea);
				#ifdef USE_SH_POOLS
				TextureSets[i].tiles[j].Areas = (TexArea*)MemAllocPtr( gTexDBMemPool, sizeof(char ) * dataSize, 0 ) ;
				#else
				TextureSets[i].tiles[j].Areas = (TexArea*)new char[ dataSize ];
				#endif
				ShiAssert( TextureSets[i].tiles[j].Areas );
				result = ReadFile( listFile, TextureSets[i].tiles[j].Areas, dataSize, &bytesRead, NULL );
				if (!result) {
					char	string[80];
					char	message[120];
					PutErrorString( string );
					sprintf( message, "%s:  Couldn't read tile areas - disk error?", string );
					ShiError( message );
				}
			}

			// Now read all the paths
			if ( TextureSets[i].tiles[j].nPaths == 0 ) {
				TextureSets[i].tiles[j].Paths = NULL;
			} else {
				dataSize = TextureSets[i].tiles[j].nPaths * sizeof(TexPath);
				#ifdef USE_SH_POOLS
				TextureSets[i].tiles[j].Paths = (TexPath*)MemAllocPtr( gTexDBMemPool, sizeof(char) * dataSize, 0 ) ;
				#else
				TextureSets[i].tiles[j].Paths = (TexPath*)new char[ dataSize ];
				#endif
				ShiAssert( TextureSets[i].tiles[j].Paths );
				result = ReadFile( listFile, TextureSets[i].tiles[j].Paths, dataSize, &bytesRead, NULL );
				if (!result) {
					char	string[80];
					char	message[120];
					PutErrorString( string );
					sprintf( message, "%s:  Couldn't read tile paths - disk error?", string );
					ShiError( message );
				}
			}

		}
	}

	// Now close the texture list file
	CloseHandle( listFile );

	return TRUE;
}


void TextureDB::Cleanup(void)
{
	int i, j;
	int res;

	ShiAssert( IsReady() );

	ShiAssert(TextureSets);
	if (!TextureSets)
		return;

	// Stop receiving time updates
	TheTimeManager.ReleaseTimeUpdateCB( TimeUpdateCallback, this );


	// Free the entire texture list (palettes will be freed as textures go away)
	for (i=0; i<numSets; i++) {

		for (j=0; j<TextureSets[i].numTiles; j++) {

			// Free the area descriptions
			#ifdef USE_SH_POOLS
			if( TextureSets[i].tiles[j].Areas )
				MemFreePtr( TextureSets[i].tiles[j].Areas );
			TextureSets[i].tiles[j].Areas = NULL;
			if( TextureSets[i].tiles[j].Paths )
				MemFreePtr( TextureSets[i].tiles[j].Paths );
			TextureSets[i].tiles[j].Paths = NULL;
			#else
			delete[] TextureSets[i].tiles[j].Areas;
			TextureSets[i].tiles[j].Areas = NULL;
			delete[] TextureSets[i].tiles[j].Paths;
			TextureSets[i].tiles[j].Paths = NULL;
			#endif

			// Free the texture data
			for (res=TEX_LEVELS-1; res>=0; res--) {
				if ( TextureSets[i].tiles[j].handle[res] ) {
					Deactivate( &TextureSets[i], &TextureSets[i].tiles[j], res );
				}
				if ( TextureSets[i].tiles[j].bits[res] ) {
					Free( &TextureSets[i], &TextureSets[i].tiles[j], res );
				}
			}
		}


		#ifdef USE_SH_POOLS
		if( TextureSets[i].tiles )
			MemFreePtr( TextureSets[i].tiles );
		#else
		delete[] TextureSets[i].tiles;
		#endif
		TextureSets[i].tiles = NULL;
	}

	#ifdef USE_SH_POOLS
	if( TextureSets )
		MemFreePtr( TextureSets );
	#else
	delete[] TextureSets;
	#endif
	TextureSets = NULL;


	// We no longer need our texture managment RC
	private_rc = NULL;


	ShiAssert( ActiveTextureCount == 0 );
	ShiAssert( LoadedTextureCount == 0 );
	ShiAssert( LoadedSetCount == 0 );

	#ifdef USE_SH_POOLS
	if ( gTexDBMemPool != NULL )
	{
		MemPoolFree( gTexDBMemPool );
		gTexDBMemPool = NULL;
	}
	#endif

	// Release the sychronization objects we've been using
	DeleteCriticalSection( &cs_textureList );
}


// Set the light level applied to the terrain textures.
void TextureDB::TimeUpdateCallback( void *self ) {
	((TextureDB*)self)->SetLightLevel();
}
void TextureDB::SetLightLevel( void )
{
	// Store the new light level

	// Decide what color to use for lighting
	if (TheTimeOfDay.GetNVGmode()) {
		lightLevel		= NVG_LIGHT_LEVEL;
		lightColor.r	= 0.0f;
		lightColor.g	= NVG_LIGHT_LEVEL;
		lightColor.b	= 0.0f;
	} else {
		lightLevel		= TheTimeOfDay.GetLightLevel();
		TheTimeOfDay.GetTextureLightingColor( &lightColor );
	}
	

	// Update all the currently loaded textures
	// TODO: Do this gradually in the background?
	for (int i=0; i<numSets; i++) {

		if ( TextureSets[i].palHandle ) {
			StoreMPRPalette( &TextureSets[i] );
		}
	}
}

			
// Lite and store a texture palette in MPR.
void TextureDB::StoreMPRPalette( SetEntry *pSet )
{
	DWORD	palette[256];		// Scratch palette for doing lighting calculations
	BYTE	*to, *from, *stop;	// Pointers used to walk through the palette

	ShiAssert( pSet->palette );
	ShiAssert( pSet->palHandle );


	// Apply the current lighting
	from	= (BYTE*)pSet->palette;

	if (F4IsBadReadPtr(from, sizeof(BYTE))) // JB 010408 CTD
		return;

	to		= (BYTE*)palette;
	stop	= to + 256*4;
	while (	to < stop ) {
		*to = static_cast<BYTE>(FloatToInt32(*from * lightColor.r));	to++, from++;	// Red
		*to = static_cast<BYTE>(FloatToInt32(*from * lightColor.g));	to++, from++;	// Green
		*to = static_cast<BYTE>(FloatToInt32(*from * lightColor.b));	to++, from++;	// Blue
																		to++, from++;	// Alpha
	}

	// Turn on the lights if it is dark enough
	// TODO: Blend these in gradually
	if (lightLevel < 0.5f) {

		to		= (BYTE*)&(palette[252]);

		if (TheTimeOfDay.GetNVGmode()) {
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
		} else  {
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

// OW
#if 0
	// Update MPR's palette
	MPRLoadTexturePalette(  private_rc, 
							pSet->palHandle,
							MPR_TI_PALETTE,	// Palette info
							32,				// Bits per entry
							0,				// Start index
							256,			// Number of entries
							(BYTE*)&palette );
#else
	((PaletteHandle *) pSet->palHandle)->Load(MPR_TI_PALETTE,	// Palette info
							32,				// Bits per entry
							0,				// Start index
							256,			// Number of entries
							(BYTE*)&palette );
#endif
}


// This function is called by anyone wishing the use of a particular texture.
// If the texture is not already loaded, this will do the I/O required.
void TextureDB::Request( TextureID texID ) {
	int		set		= ExtractSet( texID );
	int		tile	= ExtractTile( texID );
	int		res		= ExtractRes( texID );
	int		r;
	BOOL	needToLoad[TEX_LEVELS];

	ShiAssert( IsReady() );
	ShiAssert( set >= 0 );
	ShiAssert( set < numSets );
	ShiAssert( tile < TextureSets[set].numTiles );
	ShiAssert( res < TEX_LEVELS );

	EnterCriticalSection( &cs_textureList );

	// Check this bitmap and all lower res bitmaps
	for (r=res; r>=0; r--) {
		// Decide if anyone needs to be loaded and increment our reference counts.
		needToLoad[r] = (TextureSets[set].tiles[tile].refCount[r] == 0);
		TextureSets[set].tiles[tile].refCount[r]++;
	}

	LeaveCriticalSection( &cs_textureList );


	// Get the data for this bitmap and any lower res bitmaps required
	for (r=res; r>=0; r--) {
		if (needToLoad[r]) {
			ShiAssert( TextureSets[set].tiles[tile].bits[r] == NULL );
			ShiAssert( TextureSets[set].tiles[tile].handle[r] == NULL );

			if (overrideHandle == NULL) {
				Load( &TextureSets[set], &TextureSets[set].tiles[tile], r );
			} else {
				// When using an override texture, we just assume its handle. 
				TextureSets[set].tiles[tile].handle[res]	= overrideHandle;
				TextureSets[set].tiles[tile].bits[res]		= NULL;
			}
		}
	}
}


// This function must eventually be called by anyone who calls the Request function above.
// This call will decrease the reference count of the texture and if it reaches zero, remove
// it from memory.
void TextureDB::Release( TextureID texID ) {

	int set		= ExtractSet( texID );
	int tile	= ExtractTile( texID );
	int res		= ExtractRes( texID );

	ShiAssert( IsReady() );
	ShiAssert( set >= 0 );
	ShiAssert( set < numSets );
	ShiAssert( tile < TextureSets[set].numTiles );
	ShiAssert( res < TEX_LEVELS );

	EnterCriticalSection( &cs_textureList );

	// Release our hold on this texture (and its lower res versions)
	while (res >= 0) {

		TextureSets[set].tiles[tile].refCount[res]--;

		if (TextureSets[set].tiles[tile].refCount[res] == 0) {
			if (overrideHandle == NULL) {
				Deactivate( &TextureSets[set], &TextureSets[set].tiles[tile], res );
				Free( &TextureSets[set], &TextureSets[set].tiles[tile], res );
			} else {
				// When using an override texture, we don't do any clean up, we just drop 
				// our handle to the override texture.
				TextureSets[set].tiles[tile].handle[res]	= NULL;
				TextureSets[set].tiles[tile].bits[res]		= NULL;
			}
		}

		res--;
	}

	LeaveCriticalSection( &cs_textureList );
}


// Return a pointer to the Nth path of type TYPE, where N is from the offset parameter.
// (TYPE of 0 means any type, and offset is then from the start of the list) 
TexPath* TextureDB::GetPath( TextureID texID, int type, int offset )
{
	TexPath *a;
	TexPath *stop;

	int set		= ExtractSet( texID );
	int tile	= ExtractTile( texID );

	ShiAssert( set >= 0 );
	ShiAssert( set < numSets );

	a = TextureSets[set].tiles[tile].Paths;
	stop = a + TextureSets[set].tiles[tile].nPaths;

	if (type) {
		// Find the first entry of the required type
		while ((a < stop) && (a->type != type)) {
			a++;
		}
	}

	// Step to the requested offset
	a += offset;

	// We didn't find enough (or any) matching types
	if ((a >= stop) || ((type) && (a->type != type))) {
		return NULL;		
	}

	return a;	// We found a match
}


// Return a pointer to the Nth area of type TYPE, where N is from the offset parameter.
// (TYPE of 0 means any type, and offset is then from the start of the list) 
TexArea* TextureDB::GetArea( TextureID texID, int type, int offset )
{
	TexArea *a;
	TexArea *stop;

	int set		= ExtractSet( texID );
	int tile	= ExtractTile( texID );

	ShiAssert( set >= 0 );
	ShiAssert( set < numSets );

	a = TextureSets[set].tiles[tile].Areas;
	stop = a + TextureSets[set].tiles[tile].nAreas;

	if (type) {
		// Find the first entry of the required type
		while ((a < stop) && (a->type != type)) {
			a++;
		}
	}

	// Step to the requested offset
	a += offset;

	// We didn't find enough (or any) matching types
	if ((a >= stop) || ((type) && (a->type != type))) {
		return NULL;		
	}

	return a;	// We found a match
}


BYTE TextureDB::GetTerrainType( TextureID texID )
{
	int set		= ExtractSet( texID );

	ShiAssert( set >= 0 );
	ShiAssert( set < numSets );

	return TextureSets[set].terrainType;
}

	
// This function reads texel data from disk.  It reads only the resolution
// level that is requested.
void TextureDB::Load( SetEntry* pSet, TileEntry* pTile, int res ) {
	char				filename[MAX_PATH];
	int					result;
	CImageFileMemory 	texFile;

	ShiAssert( IsReady() );
	ShiAssert( pSet );
	ShiAssert( pTile );
	ShiAssert( res < TEX_LEVELS );
	ShiAssert( !pTile->handle[res] );


	// Construct the full texture file name including path
	strcpy( filename, texturePath );
	strcat( filename, pTile->filename );
	if (res == 1) {
		filename[strlen( texturePath )] = 'M';
	} else if (res == 0) {
		filename[strlen( texturePath )] = 'L';
	}

	// Make sure we recognize this file type
	texFile.imageType = CheckImageType( filename );
	ShiAssert( texFile.imageType != IMAGE_TYPE_UNKNOWN );

	// Open the input file
	result = texFile.glOpenFileMem( filename );
	if ( result != 1 ) {
		char	message[256];
		sprintf( message, "Failed to open %s", filename );
		ShiError( message );
	}

	// Read the image data (note that ReadTextureImage will close texFile for us)
	texFile.glReadFileMem();
	result = ReadTextureImage( &texFile );
	if (result != GOOD_READ) {
		ShiError( "Failed to read terrain texture.  CD Error?" );
	}


	// Store pointer to the image data
	pTile->bits[res] = (BYTE*)texFile.image.image;
	ShiAssert( pTile->bits[res] );

	// Store the width and height of the texture for use when loading the texture
	ShiAssert( texFile.image.width == texFile.image.height );	// Only allow square textures
	ShiAssert( texFile.image.width == 32 || texFile.image.width == 64 || texFile.image.width == 128 || texFile.image.width == 256);
	pTile->width[res] = texFile.image.width;
	pTile->height[res] = texFile.image.height;


	// Make sure this texture is palettized
	ShiAssert( texFile.image.palette );

	// If we already have this palette, we don't need it again
	if ( pSet->palette ) {

		glReleaseMemory( texFile.image.palette );

	} else {

		// This must be the first reference to this tile since we don't have a palette
		ShiAssert( pSet->refCount == 0 );
		ShiAssert( pSet->palHandle == 0 );

		// Save the palette from this image for future use
		pSet->palette = (DWORD*)texFile.image.palette;

#ifdef _DEBUG
		// Temporary.  Keep count of how many palettes are loaded at once
		LoadedSetCount++;
#endif
	}

	ShiAssert( pSet->palette );

	// Note that this tile is referencing it's parent set
	pSet->refCount++;

#ifdef _DEBUG
	// Temporary.  Keep count of how many textures are loaded at once
	LoadedTextureCount++;
#endif
}


// This function sends texture data to MPR
void TextureDB::Activate( SetEntry* pSet, TileEntry* pTile, int res )
{
	ShiAssert( IsReady() );
	ShiAssert( private_rc );
	ShiAssert( pSet );
	ShiAssert( pTile );
	ShiAssert( res < TEX_LEVELS );
	ShiAssert( !pTile->handle[res] );
	ShiAssert( pTile->bits[res] );
	ShiAssert( pTile->width[res] == pTile->height[res] );
	ShiAssert( pTile->width[res] == 32 || pTile->width[res] == 64 || pTile->width[res] == 128 || pTile->width[res] == 256 );

	// Pass the palette to MPR if it isn't already there
	if (pSet->palHandle == 0)
	{
		pSet->palHandle = (UInt) new PaletteHandle( private_rc->m_pDD, 32, 256 );
		ShiAssert( pSet->palHandle );
		StoreMPRPalette( pSet );
	}

	pTile->handle[res] = (UInt) new TextureHandle;
	ShiAssert( pTile->handle[res] );

	// Attach the palette
	((PaletteHandle *) pSet->palHandle)->AttachToTexture((TextureHandle *) pTile->handle[res]);

	DWORD dwFlags = NULL;
	WORD info = MPR_TI_PALETTE;

	if(g_bEnableStaticTerrainTextures)
		dwFlags |= TextureHandle::FLAG_HINT_STATIC;

	if(g_bUseMipMaps)
		info |= MPR_TI_MIPMAP;

	((TextureHandle *) pTile->handle[res])->Create("TextureDB", info, 8, 
		static_cast<UInt16>(pTile->width[res]), static_cast<UInt16>(pTile->height[res]), dwFlags);

#ifndef _DONOT_COPY_BITS
	((TextureHandle *) pTile->handle[res])->Load(0, 0, (BYTE*)pTile->bits[res]);

	// Now that we don't need the local copy of the image, drop it
	glReleaseMemory( (char*)pTile->bits[res] );
	pTile->bits[res] = NULL;

	#ifdef _DEBUG
		// Temporary counter used to track texture usage
		LoadedTextureCount--;
	#endif
#else
	((TextureHandle *) pTile->handle[res])->Load(0, 0, (BYTE*)pTile->bits[res], false, true);
#endif

#ifdef _DEBUG
	// Temporary.  Keep count of how many textures are active at once
	ActiveTextureCount++;
#endif
}


// This function will release the MPR handle for the specified texture.
void TextureDB::Deactivate( SetEntry* pSet, TileEntry* pTile, int res )
{
	ShiAssert( IsReady() );
	ShiAssert( pSet );
	ShiAssert( pTile );

	// make sure we're not freeing a tile that is in use
	ShiAssert( pTile->refCount[res] == 0 );

	// Quit now if we've got nothing to do
	if(pTile->handle[res])
	{
		delete (TextureHandle *) pTile->handle[res];
		pTile->handle[res] = NULL;

	#ifdef _DEBUG
		// Temporary.  Keep count of how many textures are active at once
		ActiveTextureCount--;
	#endif
	}

#ifdef _DONOT_COPY_BITS
	if((char*)pTile->bits[res])
	{
		glReleaseMemory( (char*)pTile->bits[res] );
		pTile->bits[res] = NULL;

	#ifdef _DEBUG
		// Temporary.  Keep count of how many textures are active at once
		LoadedTextureCount--;
	#endif
	}
#endif
}


// This function will release the memory image of a texture.
// It is called when the reference count of a texture reaches zero.
// Since all lower res textures are referenced anytime a higher res
// texture is used, it is ensured that res 0 will be the last texture
// freed.  When res 0 become unreferenced, the set reference count is
// reduced by one.  When the set reference count reaches zero, the
// set palette will also be freed.
void TextureDB::Free( SetEntry* pSet, TileEntry* pTile, int res )
{
	ShiAssert( IsReady() );
	ShiAssert( pSet );
	ShiAssert( pTile );

	// make sure we're not freeing a tile that is in use
	ShiAssert( pTile->refCount[res] == 0 );
	ShiAssert( pTile->handle[res] == NULL );

   // KLUDGE to prevent release runtime crash
   if (!pTile || !pSet)
      return;

	// Rlease the image memory if it isn't already gone
#ifndef _DONOT_COPY_BITS
	if((char*)pTile->bits[res])
	{
		glReleaseMemory( (char*)pTile->bits[res] );
		pTile->bits[res] = NULL;

#ifdef _DEBUG
		// Temporary counter used to track texture usage
		LoadedTextureCount--;
#endif
	}
#endif

	pSet->refCount--;

	// Free the set palette if no tiles are in use
	if(pSet->refCount == 0)
	{
		if(pSet->palHandle)
		{
			delete (PaletteHandle *) pSet->palHandle;
			pSet->palHandle = NULL;
		}

		glReleaseMemory( pSet->palette );
		pSet->palette = NULL;

#ifdef _DEBUG
		LoadedSetCount--;
#endif
	}
}


// Select a "Load"ed texture into an RC for immediate use by the rasterizer
void TextureDB::Select( ContextMPR *localContext, TextureID texID )
{
	ShiAssert( IsReady() );
	ShiAssert( localContext );

	int set		= ExtractSet( texID );
	int tile	= ExtractTile( texID );
	int res		= ExtractRes( texID );

	ShiAssert( set >= 0 );
	ShiAssert( set < numSets );
	ShiAssert( tile >= 0 );
	ShiAssert( tile < TextureSets[set].numTiles );

	if (!(set >= 0 && set < numSets && tile >= 0 && tile < TextureSets[set].numTiles)) // JB 010318 CTD
		return; // JB 010318 CTD

	// Make sure the texture we're trying to use is local to MPR
	if (TextureSets[set].tiles[tile].handle[res] == NULL) {
		ShiAssert( TextureSets[set].tiles[tile].bits[res] );
		Activate( &TextureSets[set], &TextureSets[set].tiles[tile], res );
	}
	ShiAssert( TextureSets[set].tiles[tile].handle[res] );

	// Make the required texture the current texture in the rendering context
// OW
//	localContext->SelectTexture( TextureSets[set].tiles[tile].handle[res] );

#ifdef LITE_TEXTURE
	localContext->SetState( MPR_STA_ENABLES,			MPR_SE_MODULATION );
	localContext->SetState( MPR_STA_TEX_FUNCTION,		MPR_TF_MULTIPLY );
	localContext->SetState( MPR_STA_ENABLES,			MPR_SE_SHADING );
#endif

	// OW
	localContext->SelectTexture( TextureSets[set].tiles[tile].handle[res] );
}


// OW
void TextureDB::RestoreAll()
{
    ShiAssert(IsReady());
    if (!IsReady()) return; // JPO
    EnterCriticalSection( &cs_textureList );
    
    // Restore the entire texture list
    for(int i=0; i<numSets; i++)
    {
	for(int j=0; j<TextureSets[i].numTiles; j++)
	{
	    // Free the texture data
	    for(int res=TEX_LEVELS-1; res>=0; res--)
	    {
		if(TextureSets[i].tiles[j].handle[res])
		    ((TextureHandle *) TextureSets[i].tiles[j].handle[res])->RestoreAll();
	    }
	}
    }
    
    LeaveCriticalSection( &cs_textureList );
}
