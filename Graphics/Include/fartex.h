/***************************************************************************\
    FarTex.h
    Scott Randolph
    May 22, 1997

	Maintain the list of distant terrain textures.
\***************************************************************************/
#ifndef _FARRTEX_H_
#define _FARRTEX_H_

#include "TerrTex.h"
#include "Falclib\Include\FileMemMap.h"

struct IDirectDrawPalette;

// The one and only Distant Texture Database object
extern class FarTexDB TheFarTextures;

#define IMAGE_SIZE 32
class FarTexFile : public FileMemMap {
public:
    BYTE *GetFarTex(DWORD offset) { 
	return (BYTE*)GetData(offset*IMAGE_SIZE*IMAGE_SIZE, sizeof(BYTE) * IMAGE_SIZE*IMAGE_SIZE);
    };
};

typedef struct FarTexEntry {
	BYTE		*bits;		// 8 bit pixel data (NULL if not loaded)
	UInt		handle;		// Rasterization engine texture handle (NULL if not available)
	int			refCount;	// Reference count
							// Add Terrain type here?  Have to get it in MapDice...
} FarTexEntry;


class FarTexDB {
  public:
    FarTexDB()		{ texArray = NULL; };
    ~FarTexDB()		{ ShiAssert( !IsReady() ); };

	BOOL		Setup( DXContext *hrc, const char* texturePath );
	void		Cleanup(void);

	BOOL		IsReady(void)	{ return (texArray != NULL); };


	// Functions to load and use textures at model load time and render time
	void		Request( TextureID texID );
	void		Release( TextureID texID );
	void		Select( ContextMPR *localContext, TextureID texID );

	void RestoreAll();	// OW

  protected:
	FarTexFile	fartexFile;

  	int			texCount;
	FarTexEntry	*texArray;		// Array of texture records

	DWORD		palette[256];	// Original, unlit palette data
	PaletteHandle *palHandle;		// Rasterization engine palette handle (NULL if not available)

	DXContext *private_rc;

	CRITICAL_SECTION	cs_textureList;

  protected:
	// These functions acutally load and release the texture bitmap memory
	void	Load( DWORD offset );
	void	Activate( DWORD offset );
	void	Deactivate( DWORD offset );
	void	Free( DWORD offset );

	// Handle time of day and lighting notifications
	static void TimeUpdateCallback( void *self );
	void SetLightLevel( void );

#ifdef _DEBUG
	// These are for debuggin and instrumentation only -- they should go in final versions
  public:
	int			LoadedTextureCount;
	int			ActiveTextureCount;
#endif

	friend void THP_NECB(TextureHandle *pNewHandle, LPVOID Ctx);
};

#endif // _FARRTEX_H_
