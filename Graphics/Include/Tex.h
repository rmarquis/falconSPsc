/***************************************************************************\
    Tex.h
    Scott Randolph
    February 5, 1997

	Provide simple texture loading and managment services to a broad
	range of clients.
\***************************************************************************/
#ifndef _TEX_H_
#define _TEX_H_

#include "grtypes.h"
#include "Context.h"
#include "Palette.h"

struct IDirectDrawSurface7;
struct IDirect3DDevice7;
enum _D3DX_SURFACEFORMAT;
class PaletteHandle;

class Texture
{
	public:
	Texture();
	~Texture();

	public:
	int dimensions;
	void *imageData;
	Palette *palette;
	DWORD flags;
	DWORD chromaKey;

	#ifdef _DEBUG
	static DWORD m_dwNumHandles;		// Number of instances
	static DWORD m_dwBitmapBytes;		// Bytes allocated for bitmap copies
	static DWORD m_dwTotalBytes;			// Total number of bytes allocated (including bitmap copies and object size)
	#endif

	protected:
	// OW
	TextureHandle *texHandle;

	public:
	static void SetupForDevice( DXContext *texRC, char *path);
	static void CleanupForDevice(DXContext *texRC);
	static bool IsSetup(); // JB 010616
	BOOL LoadImage(char *filename, DWORD newFlags = 0, BOOL addDefaultPath = TRUE);
	void FreeImage();
	bool CreateTexture(char *strName = NULL);
	void FreeTexture();
	BOOL LoadAndCreate(char *filename, DWORD newFlags = 0);
	bool UpdateMPR(char *strName = NULL);
	void FreePalette();
	void FreeAll() { FreeTexture(); FreeImage(); };
	DWORD TexHandle() { return (DWORD) texHandle; };
	void RestoreAll();	// OW

	#ifdef _DEBUG
	public:
	static void MemoryUsageReport();
	#endif
};

#endif // _TEX_H_
