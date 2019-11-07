/***************************************************************************\
    TerrTex.h
    Scott Randolph
    October 2, 1995

	Hold information on all the terrain texture tiles.
\***************************************************************************/
#ifndef _TERRTEX_H_
#define _TERRTEX_H_

#include "grtypes.h"
#include "Context.h"
#include "mpr_light.h"

typedef DWORD TextureID; // JPO - increased to 32 bits... scary


#define TEX_LEVELS	3		// 128x128, 64x64, 32x32


// Terrain and feature types defined in the the visual basic tile tool
const int COVERAGE_NODATA     = 0;
const int COVERAGE_WATER		= 1;
const int COVERAGE_RIVER		= 2;
const int COVERAGE_SWAMP		= 3;
const int COVERAGE_PLAINS		= 4;
const int COVERAGE_BRUSH		= 5;
const int COVERAGE_THINFOREST	= 6;
const int COVERAGE_THICKFOREST	= 7;
const int COVERAGE_ROCKY		= 8;
const int COVERAGE_URBAN		= 9;
const int COVERAGE_ROAD			= 10;
const int COVERAGE_RAIL			= 11;
const int COVERAGE_BRIDGE		= 12;
const int COVERAGE_RUNWAY		= 13;
const int COVERAGE_STATION		= 14;
const int COVERAGE_OBJECT = 15; // JB carrier

// The one and only Texture Database object
extern class TextureDB TheTerrTextures;


typedef struct TexArea {
	int		type;
	float	radius;
	float	x;
	float	y;
} TexArea;


typedef struct TexPath {
	int		type;
	float	width;
	float	x1, y1;
	float	x2, y2;
} TexPath;


typedef struct TileEntry {
	char 		filename[20];	// Source filename of the bitmap (extension, but no path)

	int			nAreas;
	TexArea		*Areas;					// List of special areas (NULL if none)
	int			nPaths;
	TexPath		*Paths;					// List of paths (NULL if none)

	BYTE		*bits[TEX_LEVELS];		// 8 bit pixel data (NULL if not loaded)
	int			width[TEX_LEVELS];		// texture width in pixels
	int			height[TEX_LEVELS];		// texture height in pixels
	UInt		handle[TEX_LEVELS];		// Rasterization engine texture handle (NULL if not available)
	int			refCount[TEX_LEVELS];	// Reference count
} TileEntry;


typedef struct SetEntry {
	int			refCount;		// Reference count

	DWORD		*palette;		// 32 bit palette entries (NULL if not loaded)
	UInt		palHandle;		// Rasterization engine palette handle (NULL if not available)

	BYTE		terrainType;	// Terrain coverage type represented by this texture set
	int			numTiles;		// How many tiles in this set?
	TileEntry	*tiles;			// Array of tiles in this set
} SetEntry;


class TextureDB {
  public:
    TextureDB()		{ TextureSets = NULL; };
    ~TextureDB()	{ ShiAssert( !IsReady() ); };

	BOOL		Setup( DXContext *hrc, const char* texturePath );
	void		Cleanup(void);

	BOOL		IsReady(void)	{ return (TextureSets != NULL); };


	// Function to force a single texture to override all others (for ACMI wireframe)
	void		SetOverrideTexture( UInt texHandle )	{ overrideHandle = texHandle; };


	// Functions to load and use textures at model load time and render time
	void		Request( TextureID texID );
	void		Release( TextureID texID );
	void		Select( ContextMPR *localContext, TextureID texID );

	void RestoreAll();	// OW

	// Functions to interact with the texture database entries
	const char *GetTexturePath(void)	{ return texturePath; };
	TexPath*	GetPath( TextureID id, int type, int offset );
	TexArea*	GetArea( TextureID id, int type, int offset );
	BYTE		GetTerrainType( TextureID id );

  protected:
	char		texturePath[MAX_PATH];

  	int			totalTiles;
  	int			numSets;
	SetEntry	*TextureSets;	// Array of texture set records

	UInt		overrideHandle;	// If nonNull, use this handle for ALL texture selects

	float		lightLevel;		// Current light level (0.0 to 1.0)
	Tcolor		lightColor;		// Current light color

//	MPRHandle_t			private_rc;
	DXContext *private_rc;

	CRITICAL_SECTION	cs_textureList;

  protected:
	// These functions acutally load and release the texture bitmap memory
	void	Load( SetEntry* pSet, TileEntry* pTile, int res );
	void	Activate( SetEntry* pSet, TileEntry* pTile, int res );
	void	Deactivate( SetEntry* pSet, TileEntry* pTile, int res );
	void	Free( SetEntry* pSet, TileEntry* pTile, int res );

	// Extract set, tile, and resolution from a texID
	int		ExtractSet( TextureID texID )		{ return (texID >> 4) & 0xFF;	};
	int		ExtractTile( TextureID texID )		{ return texID & 0xF;			};
	int		ExtractRes( TextureID texID )		{ return (texID >> 12) & 0xF;	};

	// Handle time of day and lighting notifications
	static void TimeUpdateCallback( void *self );
	void SetLightLevel( void );

	// This function handles lighting and storing the MPR version of a specific palette
	void	StoreMPRPalette( SetEntry *pSet );


#ifdef _DEBUG
	// These are for debugging and instrumentation only -- they should go in final versions
  public:
	int			LoadedSetCount;
	int			LoadedTextureCount;
	int			ActiveTextureCount;
#endif
};

#endif // _TERRTEX_H_
