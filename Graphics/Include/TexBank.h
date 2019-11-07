/***************************************************************************\
    TexBank.h
    Scott Randolph
    March 4, 1998

    Provides the bank of textures used by all the BSP objects.
\***************************************************************************/
#ifndef _TEXBANK_H_
#define _TEXBANK_H_

#include "Tex.h"
#include "PolyLib.h"
#include "Falclib\Include\FileMemMap.h"

// The one and only texture bank.  This would need to be replaced
// by pointers to instances of TextureBankClass passed to each call
// if more than one color store were to be simultaniously maintained.
extern class TextureBankClass		TheTextureBank;

#if 0
// OW thanks to the "flexible" serialization concept ;(
typedef struct TexBankEntry_Old {
	long	 	fileOffset;		// How far into the .TEX file does the compressed data start?
	long	 	fileSize;		// How big is the compressed data on disk?
	Texture_Old		tex;			// The container class which manages the texture data
	int			palID;			// The offset into ThePaletteBank to use with this texture
	int			refCount;		// How many objects want this texture right now
} TexBankEntry_Old;

typedef struct TexBankEntry {
	long	 	fileOffset;		// How far into the .TEX file does the compressed data start?
	long	 	fileSize;		// How big is the compressed data on disk?
	Texture		tex;			// The container class which manages the texture data
	int			palID;			// The offset into ThePaletteBank to use with this texture
	int			refCount;		// How many objects want this texture right now

	// OW
	TexBankEntry& operator=(TexBankEntry_Old& ref)
	{
		fileOffset = ref.fileOffset;
		fileSize = ref.fileSize;
		tex = ref.tex;
		palID = ref.palID;
		refCount = ref.refCount;

		return *this;
	}	
} TexBankEntry;
#else

typedef struct TexBankEntry {
	long	 	fileOffset;		// How far into the .TEX file does the compressed data start?
	long	 	fileSize;		// How big is the compressed data on disk?
	Texture		tex;			// The container class which manages the texture data
	int			palID;			// The offset into ThePaletteBank to use with this texture
	int			refCount;		// How many objects want this texture right now
} TexBankEntry;
#endif

class TextureBankClass {
  public:
	TextureBankClass()	{ nTextures = 0; TexturePool = NULL; };
	~TextureBankClass()	{};

	// Management functions
	static void Setup( int nEntries );
	static void Cleanup( void );
	static void ReadPool( int file, char *basename );
	static void FlushHandles( void );

	static void Reference( int id );
	static void Release( int id );
	static void Select( int id );

	static void RestoreAll();	// OW

	// Deferred texture load support (to improve startup load times)
	static void SetDeferredLoad( BOOL state );
	
	// Debug parameter validation
	static BOOL IsValidIndex( int id );

  public:
	static TexBankEntry	*TexturePool;
	static int			nTextures;
#ifdef _DEBUG
	static int textureCount; // JPO some stats
#endif

  protected:
	//static int			TextureFile;
	static FileMemMap  TexFileMap; // JPO - MMFILE
	static BYTE			*CompressedBuffer;
	static int			deferredLoadState;

  protected:
	static void OpenTextureFile( char *basename );
	static void	ReadImageData( int id );
	static void CloseTextureFile( void );

	static void LoaderCallBack( struct LoaderQ* request );
};
#endif // _TEXBANK_H_