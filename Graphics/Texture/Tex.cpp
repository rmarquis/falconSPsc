/***************************************************************************\
    Tex.cpp
    Scott Randolph
    February 5, 1997

	Provide simple texture loading and managment services to a broad
	range of clients.
\***************************************************************************/

#include "stdafx.h"
#include "Image.h"
#include "Tex.h"
#include "FalcLib\include\playerop.h"

// TODO
// - Mipmaps (warning: this requires the loading code to take care of the attached mipmip surfaces)

// use D3DX_SF_
// DXT1 Opaque / one-bit alpha n/a 
// DXT2 Explicit alpha Yes 
// DXT3 Explicit alpha No 
// DXT4 Interpolated alpha Yes 
// DXT5 Interpolated alpha No 

#ifdef USE_SH_POOLS
MEM_POOL Palette::pool;
#endif

static char			TexturePath[256] = {'\0'};
static DXContext	*rc = NULL; 	

extern bool g_bEnableNonPersistentTextures;
extern bool g_bShowMipUsage;

static HRESULT WINAPI MipLoadCallback(LPDIRECTDRAWSURFACE7 lpDDSurface, LPDDSURFACEDESC2 lpDDSurfaceDesc, LPVOID lpContext);
struct MipLoadContext
{
	int nLevel;
	LPDIRECTDRAWSURFACE7 lpDDSurface;
};
void SetMipLevelColor(MipLoadContext *pCtx);

static char *arrSurfFmt2String[] =
{
	"UNKNOWN",
	"R8G8B8",
	"A8R8G8B8",
	"X8R8G8B8",
	"R5G6B5",
	"R5G5B5",
	"PALETTE4",
	"PALETTE8",
	"A1R5G5B5",
	"X4R4G4B4",
	"A4R4G4B4",
	"L8",
	"A8L8",
	"U8V8",
	"U5V5L6",
	"U8V8L8",
	"UYVY",
	"YUY2",
	"DXT1",
	"DXT3",
	"DXT5",
	"R3G3B2",
	"A8",
	"TEXTUREMAX",

	"Z16S0",
	"Z32S0",
	"Z15S1",
	"Z24S8",
	"S1Z15",
	"S8Z24",

	"<<<Index our of range>>>",
	"<<<Index our of range>>>",
	"<<<Index our of range>>>",
	"<<<Index our of range>>>",
	"<<<Index our of range>>>",
	"<<<Index our of range>>>",
	"<<<Index our of range>>>",
	"<<<Index our of range>>>",
	"<<<Index our of range>>>",
	"<<<Index our of range>>>",
	"<<<Index our of range>>>",
};

static int FindMsb(DWORD val)
{
	for(int i=0;i<32;i++)
		if(val & (1 << (31 - i)))
			break;

	return 31 - i;
}

#ifdef _DEBUG
DWORD Texture::m_dwNumHandles = 0;		// Number of instances
DWORD Texture::m_dwBitmapBytes = 0;		// Bytes allocated for bitmap copies
DWORD Texture::m_dwTotalBytes = 0;			// Total number of bytes allocated (including bitmap copies and object size)
#endif

Texture::Texture()
{
	texHandle = NULL; imageData = NULL; palette = NULL; flags = 0;

	#ifdef _DEBUG
	InterlockedIncrement((long *) &m_dwNumHandles);		// Number of instances
	InterlockedExchangeAdd((long *) &m_dwTotalBytes, sizeof(*this));
	#endif
};

Texture::~Texture()
{
	#ifdef _DEBUG
	InterlockedIncrement((long *) &m_dwNumHandles);		// Number of instances
	InterlockedExchangeAdd((long *) &m_dwTotalBytes, -sizeof(*this));
	#endif

	ShiAssert( (texHandle == NULL) && (imageData == NULL) );
};

/***************************************************************************\
	Store some useful global information.  The path is used for all
	texture loads through this interface and the RC is used for loading.
	This means that at present, only one device at a time can load textures
	through this interface.
\***************************************************************************/

void Texture::SetupForDevice(DXContext *texRC, char *path)
{
	// Store the texture path for future reference
	if(strlen( path )+1 >= sizeof( TexturePath ))
		ShiError( "Texture path name overflow!" );

	strcpy( TexturePath, path );
	if(TexturePath[strlen(TexturePath)-1] != '\\')
		strcat( TexturePath, "\\" );

	rc = texRC;
	Palette::SetupForDevice( texRC );

	TextureHandle::StaticInit(texRC->m_pD3DD);
}


/***************************************************************************\
	This is called when we're done working with a given device (as 
	represented by an RC).
\***************************************************************************/
void Texture::CleanupForDevice( DXContext *texRC)
{
	Palette::CleanupForDevice( texRC );
	rc = NULL;

	TextureHandle::StaticCleanup();
}

// JB 010616
/***************************************************************************\
	This is called to check whether the device is setup.
\***************************************************************************/
bool Texture::IsSetup()
{
	return rc != NULL;
}
// JB 010616

/***************************************************************************\
	Read a data file and store its information.
\***************************************************************************/
BOOL Texture::LoadImage( char *filename, DWORD newFlags, BOOL addDefaultPath )
{
	char fullname[MAX_PATH];
	CImageFileMemory 	texFile;
	int result;

	ShiAssert( filename );
	ShiAssert( imageData == NULL );

	// Add in the users requested flags to any already set
	flags |= newFlags;

	// Add the texture path to the filename provided
	if (addDefaultPath)
	{
		strcpy( fullname, TexturePath );
		strcat( fullname, filename );
	}

	else
	{
		strcpy( fullname, filename );
	}

	// Make sure we recognize this file type
	texFile.imageType = CheckImageType( fullname );
	if (texFile.imageType == IMAGE_TYPE_UNKNOWN)
	{
		ShiWarning( "Unrecognized image type" );
		return FALSE;
	}

	// If the image type has alpha in it, create an alpha per texel texture
	if (texFile.imageType == IMAGE_TYPE_APL)
		flags |= MPR_TI_ALPHA;

	// Open the input file
	result = texFile.glOpenFileMem( fullname );
	if(result != 1)
	{
		ShiWarning( "Failed texture open" );
		return FALSE;
	}

	// Read the image data (note that ReadTextureImage will close texFile for us)
	texFile.glReadFileMem();
	result = ReadTextureImage( &texFile );

	if(result != GOOD_READ)
	{
		ShiWarning( "Failed texture read" );
		return FALSE;
	}

	// Store the image properties in our local storage
	if(texFile.image.width != texFile.image.height)
	{
		ShiWarning( "Texture isn't square" );
		return FALSE;
	}

	dimensions = texFile.image.width;
	ShiAssert( dimensions == 32 || dimensions == 64 || dimensions == 128 || dimensions == 256 );

	if(texFile.image.palette)
		chromaKey = texFile.image.palette[0];
	else
		chromaKey = 0xFFFF0000;		// Default to blue chroma key color

	// We only deal with 8 bit textures
	ShiAssert(flags & MPR_TI_PALETTE);

	imageData = texFile.image.image;

	// Create a palette object if we don't already have one
	ShiAssert( texFile.image.palette );
	if(!palette)
	{
		palette = new Palette;
		palette->Setup32( (DWORD*)texFile.image.palette );
	}

	else
		palette->Reference();

	// Release the image's palette data now that we've got our own copy
	glReleaseMemory( texFile.image.palette );

	#ifdef _DEBUG
	InterlockedExchangeAdd((long *) &m_dwTotalBytes, dimensions * dimensions);
	#endif

	return TRUE;
}


/***************************************************************************\
	Free the image data (but NOT the texture or palette).
\***************************************************************************/
void Texture::FreeImage()
{
	if(imageData)
	{
		#ifdef _DEBUG
		InterlockedExchangeAdd((long *) &m_dwTotalBytes, -(dimensions * dimensions));
		#endif

		glReleaseMemory( imageData );
		imageData = NULL;
	}

	if(!texHandle)
		FreePalette(); 		// We're totally gone, so get rid of our palette if we had one
}


/***************************************************************************\
	Using image (and optional palette data) already loaded, create an MPR
	texture.
\***************************************************************************/
bool Texture::CreateTexture(char *strName)
{
	ShiAssert( rc != NULL);
	ShiAssert( imageData );
	ShiAssert( texHandle == NULL );
	ShiAssert( dimensions == 32 || dimensions == 64 || dimensions == 128 || dimensions == 256 );

	//ShiAssert(!(flags & MPR_TI_MIPMAP));

	if (
		!F4IsBadReadPtr(palette, sizeof(Palette)) && // JB 010318 CTD
		flags & MPR_TI_PALETTE)
	{
		// Now we're ready to create the palettized MPR texture.
		palette->Activate();
		ShiAssert(palette->palHandle);

		if (!palette->palHandle) // JB 010616
			return false;

		texHandle = new TextureHandle;
		ShiAssert(texHandle);

		palette->palHandle->AttachToTexture(texHandle);
		texHandle->Create(strName, (WORD)flags, 8, static_cast<UInt16>(dimensions), static_cast<UInt16>(dimensions));

		if(imageData)	// OW: Prevent a crash
		{
			if(!texHandle->Load(0, chromaKey, (BYTE*) imageData))
				return false;
		}

		return true;
	}

	else
	{
		// Now we're ready to create the RGB MPR texture.
		texHandle = new TextureHandle;
		ShiAssert(texHandle);

		texHandle->Create(strName, 0, 32, static_cast<UInt16>(dimensions), static_cast<UInt16>(dimensions));
		return texHandle->Load(0, chromaKey, (BYTE*) imageData);
	}
}

/***************************************************************************\
	Release the MPR texture we're holding.
\***************************************************************************/
void Texture::FreeTexture()
{
	if(texHandle)
	{
		delete texHandle;
		texHandle = NULL;
	}

	if(!imageData)
		FreePalette(); 	// We're totally gone, so get rid of our palette if we had one
}

		
/***************************************************************************\
	Release the MPR palette and palette data we're holding.
\***************************************************************************/
BOOL Texture::LoadAndCreate( char *filename, DWORD newFlags )
{ 
	if(LoadImage(filename, newFlags))
	{
		CreateTexture(filename);
		return TRUE;
	}

	return FALSE; 
}


/***************************************************************************\
	Release the MPR palette and palette data we're holding.
\***************************************************************************/
void Texture::FreePalette()
{
	if(palette)
	{
		if(palette->Release() == 0)
			delete palette;

		palette = NULL;
	}
}


/***************************************************************************\
	Reload the MPR texels with the ones we have stored locally.
\***************************************************************************/

bool Texture::UpdateMPR(char *strName)
{
	ShiAssert( rc != NULL);
	ShiAssert( imageData );
	ShiAssert( texHandle );

	if(!texHandle || !imageData)
		return false;

	if(flags & MPR_TI_PALETTE)
		return texHandle->Load(0, chromaKey, (BYTE*) imageData);
	else
		return texHandle->Load(0, chromaKey, (BYTE*) imageData);
}

// OW
void Texture::RestoreAll()
{
	if(texHandle) texHandle->RestoreAll();
}


#ifdef _DEBUG
void Texture::MemoryUsageReport()
{
}
#endif

// TextureHandle
//////////////////////////////////

struct _DDPIXELFORMAT TextureHandle::m_arrPF[TEX_CAT_MAX];
IDirect3DDevice7 *TextureHandle::m_pD3DD = NULL;		// Warning: Not addref'd
struct _D3DDeviceDesc7 *TextureHandle::m_pD3DHWDeviceDesc = NULL;

#ifdef _DEBUG
DWORD TextureHandle::m_dwNumHandles = 0;		// Number of instances
DWORD TextureHandle::m_dwBitmapBytes = 0;		// Bytes allocated for bitmap copies
DWORD TextureHandle::m_dwTotalBytes = 0;			// Total number of bytes allocated (including bitmap copies and object size)
#endif

TextureHandle::TextureHandle()
{
	m_pDDS = NULL;
	m_eSurfFmt = D3DX_SF_UNKNOWN;
	m_nWidth = 0;
	m_nHeight = 0;
	m_dwFlags = NULL;
	m_dwChromaKey = NULL;
	m_pPalAttach = NULL;
	m_pImageData = NULL;
	m_nImageDataStride = -1;

	#ifdef _DEBUG
	InterlockedIncrement((long *) &m_dwNumHandles);		// Number of instances
	InterlockedExchangeAdd((long *) &m_dwTotalBytes, sizeof(*this));
	#endif
}

TextureHandle::~TextureHandle()
{
	#ifdef _DEBUG
	InterlockedDecrement((long *) &m_dwNumHandles);		// Number of instances
	InterlockedExchangeAdd((long *) &m_dwTotalBytes, -sizeof(*this));
	InterlockedExchangeAdd((long *) &m_dwTotalBytes, -m_strName.size());

	if(m_pDDS)
	{
		DDSURFACEDESC2 ddsd;
		ZeroMemory(&ddsd, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		HRESULT hr = m_pDDS->GetSurfaceDesc(&ddsd);
		ShiAssert(SUCCEEDED(hr));

		if(ddsd.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
			InterlockedExchangeAdd((long *) &m_dwTotalBytes, -(ddsd.lPitch * ddsd.dwHeight));
	}

	if(m_pImageData && m_bImageDataOwned)
	{
		DWORD dwSize = m_nImageDataStride * m_nHeight;
		InterlockedExchangeAdd((long *) &m_dwTotalBytes, -dwSize);
		InterlockedExchangeAdd((long *) &m_dwBitmapBytes, -dwSize);		
	}
	#endif

	if(m_pDDS && !F4IsBadReadPtr(m_pDDS, sizeof(IDirectDrawSurface7))) // JB 010318 CTD
		m_pDDS->Release();
	if(m_pPalAttach) m_pPalAttach->DetachFromTexture(this);
	if(m_pImageData && m_bImageDataOwned) delete[] m_pImageData;
}

bool TextureHandle::Create(char *strName, UInt16 info, UInt16 bits, UInt16 width, UInt16 height, DWORD dwFlags)
{
	m_dwFlags = info;

	#ifdef _DEBUG
	if(strName)
	{
		m_strName = strName;
		InterlockedExchangeAdd((long *) &m_dwTotalBytes, m_strName.size());
	}
	#endif

	m_nWidth = width;	// remember original dimensions
	m_nHeight = height;	// remember original dimensions

	try
	{
		DDSURFACEDESC2 ddsd;
		ZeroMemory(&ddsd, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT;
		ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE; 
		ddsd.dwWidth  = m_nWidth;
		ddsd.dwHeight = m_nHeight; 

		if(info & MPR_TI_MIPMAP)
			ddsd.ddsCaps.dwCaps |= DDSCAPS_MIPMAP | DDSCAPS_COMPLEX; 

		if (F4IsBadReadPtr(m_pD3DHWDeviceDesc, sizeof(_D3DDeviceDesc7))) // JB 010326 CTD
		{
			ReportTextureLoadError("Bad Read Pointer");
			return false;
		}

		if((info & MPR_TI_MIPMAP) || (m_pD3DHWDeviceDesc->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2))
		{
			// force power of 2
			int nMsb;

			nMsb = FindMsb(ddsd.dwWidth);
			if(ddsd.dwWidth & ~(1 << nMsb)) ddsd.dwWidth = 1 << (nMsb + 1);

			nMsb = FindMsb(ddsd.dwHeight);
			if(ddsd.dwHeight & ~(1 << nMsb)) ddsd.dwHeight = 1 << (nMsb + 1);
		}

		if(ddsd.dwWidth != ddsd.dwHeight && (m_pD3DHWDeviceDesc->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_SQUAREONLY))
		{
			// force square
			ddsd.dwWidth = ddsd.dwHeight = max(ddsd.dwWidth, ddsd.dwHeight);
		}

		if(dwFlags & FLAG_RENDERTARGET)
		{
			dwFlags |= FLAG_NOTMANAGED | FLAG_MATCHPRIMARY;		// cant render to managed surfaces

			if(rc->m_eDeviceCategory >= DXContext::D3DDeviceCategory_Hardware)	// hw devices cannot render to system memory surfaces
				dwFlags |= FLAG_INLOCALVIDMEM;

			ddsd.ddsCaps.dwCaps |= DDSCAPS_3DDEVICE;
		}

	    // Turn on texture management for HW devices
		if(rc->m_eDeviceCategory >= DXContext::D3DDeviceCategory_Hardware)
		{
			if(!(dwFlags & FLAG_NOTMANAGED))
			{
				ddsd.ddsCaps.dwCaps2 |= DDSCAPS2_TEXTUREMANAGE;

				if(g_bEnableNonPersistentTextures)
					ddsd.ddsCaps.dwCaps2 |= DDSCAPS2_DONOTPERSIST;	// do not create system memory copies
			}

			else if(dwFlags & FLAG_INLOCALVIDMEM)	// Note: mutually exclusive with texture management
				ddsd.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;	// | DDSCAPS_LOCALVIDMEM;
		}

		else
			ddsd.ddsCaps.dwCaps  |= DDSCAPS_SYSTEMMEMORY;

		if(dwFlags & FLAG_HINT_STATIC)
			ddsd.ddsCaps.dwCaps2 |= DDSCAPS2_HINTSTATIC;
		if(dwFlags & FLAG_HINT_DYNAMIC)
			ddsd.ddsCaps.dwCaps2 |= DDSCAPS2_HINTDYNAMIC;

		if(m_dwFlags & MPR_TI_PALETTE)
		{
			ShiAssert(m_pPalAttach);

			if(m_dwFlags & MPR_TI_ALPHA)
			{
				// D3DX doesnt recognize alpha palettes yet.
				// Convert to RGBA8888

				if(m_dwFlags & MPR_TI_CHROMAKEY)
				{
					ddsd.ddpfPixelFormat = m_arrPF[TEX_CAT_CHROMA_ALPHA];
 				}

				else
				{
					ddsd.ddpfPixelFormat = m_arrPF[TEX_CAT_ALPHA];
				}
			}

			else if(m_dwFlags & MPR_TI_CHROMAKEY)
			{
				// Real color keying is not widely supported (even on the latest hardware at the time writing this)
				// Convert to RGB555 + 1 Bit alpha
				// OW FIXME: use D3DX_SF_A1R5G5B5 or D3DX_SF_Z15S1 (with stencil buffer) ??

				ddsd.ddpfPixelFormat = m_arrPF[TEX_CAT_CHROMA];
			}

			else
			{
				// OW FIXME Error checking
				ddsd.ddpfPixelFormat = m_arrPF[TEX_CAT_DEFAULT];
			}
		}

		else
		{
			// These are not supported yet!!
			ShiAssert(!(m_dwFlags & MPR_TI_CHROMAKEY));
			ShiAssert(!(m_dwFlags & MPR_TI_ALPHA));

			if(dwFlags & FLAG_MATCHPRIMARY)
			{
				IDirectDrawSurface7Ptr pDDS;
				CheckHR(m_pD3DD->GetRenderTarget(&pDDS));

				IDirectDraw7Ptr pDD;
				CheckHR(pDDS->GetDDInterface((void **) &pDD));

				DDSURFACEDESC2 ddsdMode;
				ZeroMemory(&ddsdMode, sizeof(ddsdMode));
				ddsdMode.dwSize = sizeof(ddsdMode);

				CheckHR(pDD->GetDisplayMode(&ddsdMode));
				ddsd.ddpfPixelFormat = ddsdMode.ddpfPixelFormat;
			}

			else ddsd.ddpfPixelFormat = m_arrPF[TEX_CAT_DEFAULT];
		}

		HRESULT hr = rc->m_pDD->CreateSurface(&ddsd, &m_pDDS, NULL);

		if(FAILED(hr))
		{
			if(hr == DDERR_OUTOFVIDEOMEMORY)
			{
				MonoPrint("TextureHandle::Create - EVICTING MANAGED TEXTURES !!\n");

				// if we are out of video memory, evict all managed textures and retry
				CheckHR(rc->m_pD3D->EvictManagedTextures());
				CheckHR(rc->m_pDD->CreateSurface(&ddsd, &m_pDDS, NULL));
			}

			else throw _com_error(hr);
		}

		m_eSurfFmt = D3DXMakeSurfaceFormat(&ddsd.ddpfPixelFormat);

		m_nActualWidth = ddsd.dwWidth;
		m_nActualHeight = ddsd.dwHeight;

		// Attach DirectDraw palette if real palettized texture format created
		switch(m_eSurfFmt)
		{
			case D3DX_SF_PALETTE8:
			{
				if (m_pDDS && m_pPalAttach)
					m_pDDS->SetPalette(m_pPalAttach->m_pIDDP);
				break;
			}
		}

		#ifdef _DEBUG
		if(m_pDDS)
		{
			DDSURFACEDESC2 ddsd;
			ZeroMemory(&ddsd, sizeof(ddsd));
			ddsd.dwSize = sizeof(ddsd);
			HRESULT hr = m_pDDS->GetSurfaceDesc(&ddsd);
			ShiAssert(SUCCEEDED(hr));

#ifdef DEBUG_TEXTURE
			MonoPrint("Texture: %s [%s] created in %s memory\n",
				strName, arrSurfFmt2String[m_eSurfFmt], 
				ddsd.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY  ? "SYSTEM" :
				(ddsd.ddsCaps.dwCaps & DDSCAPS_LOCALVIDMEM ? "VIDEO" : "AGP"));
#endif
			if(m_dwFlags & (MPR_TI_CHROMAKEY | MPR_TI_ALPHA))
			{
				ShiAssert(ddsd.ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS);	// must have alpha channel!!

				if(ddsd.ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS)
				{
					DWORD dwMask = ddsd.ddpfPixelFormat.dwRGBAlphaBitMask;
					WORD wBits = 0;
					while(dwMask) {	dwMask = dwMask & ( dwMask - 1 );  wBits++; }
				}
			}

			if(ddsd.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
				InterlockedExchangeAdd((long *) &m_dwTotalBytes, ddsd.lPitch * ddsd.dwHeight);
		}
		#endif

		return true;
	}

	catch(_com_error e)
	{
		ReportTextureLoadError(e.Error());
		return false;
	}
}

bool TextureHandle::Load(UInt16 mip, UInt chroma,UInt8 *TexBuffer, bool bDoNotLoadBits, bool bDoNotCopyBits, int nImageDataStride)
{
	ShiAssert(TexBuffer);	// huh
#ifdef DEBUG
	if(m_dwFlags & MPR_TI_PALETTE)
		ShiAssert(m_pPalAttach);		// must be attached before calling this
#endif

	// Convert chroma key
	m_dwChromaKey = RGBA_MAKE(RGBA_GETBLUE(chroma), RGBA_GETGREEN(chroma), RGBA_GETRED(chroma), RGBA_GETALPHA(chroma));	// remember chroma key

	switch(m_eSurfFmt)
	{
		case D3DX_SF_A8R8G8B8:
		case D3DX_SF_X8R8G8B8:
		case D3DX_SF_PALETTE8:
		{
			// intentionally left blank
			break;
		}

		case D3DX_SF_A1R5G5B5:
		{
			PALETTEENTRY *pal = (PALETTEENTRY *) &m_dwChromaKey;
			m_dwChromaKey = (pal[0].peRed >> 3) | ((pal[0].peGreen >> 3) << 5) | ((pal[0].peBlue >> 3) << 10) | ((pal[0].peFlags >> 7) << 15);
			break;
		}

		case D3DX_SF_A4R4G4B4:
		{
			PALETTEENTRY *pal = (PALETTEENTRY *) &m_dwChromaKey;
			m_dwChromaKey = (pal[0].peRed >> 4) | ((pal[0].peGreen >> 4) << 4) | ((pal[0].peBlue >> 4) << 8) | ((pal[0].peFlags >> 4) << 12);
			break;
		}

		case D3DX_SF_R5G6B5:
		{
			PALETTEENTRY *pal = (PALETTEENTRY *) &m_dwChromaKey;
			m_dwChromaKey = (pal[0].peRed >> 3) | ((pal[0].peGreen >> 2) << 5) | ((pal[0].peBlue >> 3) << 11);
			break;
		}

		case D3DX_SF_R5G5B5:
		{
			PALETTEENTRY *pal = (PALETTEENTRY *) &m_dwChromaKey;
			m_dwChromaKey = (pal[0].peRed >> 3) | ((pal[0].peGreen >> 3) << 5) | ((pal[0].peBlue >> 3) << 10);
			break;
		}

		default: ShiAssert(false);	// huh
	}

	m_nImageDataStride = nImageDataStride != -1 ? nImageDataStride : m_nWidth;

	if((m_dwFlags & MPR_TI_PALETTE) && m_eSurfFmt != D3DX_SF_PALETTE8)
	{
		DWORD dwSize = m_nImageDataStride * m_nHeight;

		// free previously allocated surface memory copy
		if(m_pImageData && m_bImageDataOwned)
		{
			#ifdef _DEBUG
			InterlockedExchangeAdd((long *) &m_dwTotalBytes, -dwSize);
			InterlockedExchangeAdd((long *) &m_dwBitmapBytes, -dwSize);
			#endif

			delete[] m_pImageData;
		}

		if(!bDoNotCopyBits)
		{
			m_pImageData = new BYTE[dwSize];

			if(!m_pImageData)
			{
				ReportTextureLoadError(E_OUTOFMEMORY, true);
				return false;
			}

			memcpy(m_pImageData, TexBuffer, dwSize);
			m_bImageDataOwned = true;

			#ifdef _DEBUG
			InterlockedExchangeAdd((long *) &m_dwTotalBytes, dwSize);
			InterlockedExchangeAdd((long *) &m_dwBitmapBytes, dwSize);
			#endif
		}

		else
		{
			m_pImageData = TexBuffer;	// owner promises not to delete it while we are using it
			m_bImageDataOwned = false;
		}

		// Load it
		if(!bDoNotLoadBits)
		{
			if(!Reload())
			{
				// if loading failed free memory
				if(m_pImageData && m_bImageDataOwned) delete[] m_pImageData;
				m_pImageData = NULL;
				m_bImageDataOwned = false;

				return false;
			}
		}

		return true;
	}

	else
	{
		// These are not supported yet!!
		ShiAssert(!(m_dwFlags & MPR_TI_CHROMAKEY));
		ShiAssert(!(m_dwFlags & MPR_TI_ALPHA));

		// free previously allocated surface memory copy
		if(m_pImageData && m_bImageDataOwned)
		{
			#ifdef _DEBUG
			InterlockedExchangeAdd((long *) &m_dwTotalBytes, -(m_nImageDataStride * m_nHeight));
			#endif

			delete[] m_pImageData;
		}

		m_pImageData = TexBuffer;	// temporary value

		// Load it
		bool bResult = Reload();
		m_pImageData = NULL;		// not copied make invalid

		return bResult;
	}
}

bool TextureHandle::Reload()
{
	if(!(m_dwFlags & MPR_TI_PALETTE))
	{
		ShiAssert(false);	// whats the point of reloading a surface if you dont have the src data anymore????
		return true;
	}

	if(!m_pImageData)
		return false;

	ShiAssert(m_pImageData);	// huh
	DDSURFACEDESC2 ddsd;
	ZeroMemory(&ddsd, sizeof(ddsd));

	try
	{
		// Lock the directdraw surface
		ddsd.dwSize = sizeof(ddsd);

		if (F4IsBadReadPtr(m_pDDS, sizeof(IDirectDrawSurface7))) // JB 010305 CTD
			return false; // JB 010305 CTD

		HRESULT hr = m_pDDS->Lock(NULL, &ddsd, DDLOCK_WAIT | DDLOCK_WRITEONLY |
			DDLOCK_SURFACEMEMORYPTR , NULL);

		if(FAILED(hr))
		{
			if(hr == DDERR_SURFACELOST)
			{
				// if the surface is lost, restore it and retry
				CheckHR(m_pDDS->Restore());

				CheckHR(m_pDDS->Lock(NULL, &ddsd, DDLOCK_WAIT | DDLOCK_WRITEONLY |
					DDLOCK_SURFACEMEMORYPTR , NULL));
			}

			else throw _com_error(hr);
		}

		// Can be larger but not smaller
		ShiAssert(m_nWidth <= ddsd.dwWidth && m_nHeight <= ddsd.dwHeight);

		// reloading is a different story - because this is called VERY frequently we have to be very fast with whatever we are doing here
		switch(m_eSurfFmt)
		{
			case D3DX_SF_PALETTE8:
			{
				BYTE *pSrc = m_pImageData;
				BYTE *pDst = (BYTE *) ddsd.lpSurface;
				DWORD dwPitch = ddsd.lPitch;

				if(dwPitch == m_nWidth && m_nImageDataStride == m_nWidth)
					memcpy(pDst, pSrc, m_nWidth * m_nHeight);	// If source and destination pitch match, use single loop

				else
				{
					for(int y=0;y<m_nHeight;y++)
					{
						memcpy(pDst, pSrc, m_nWidth);

						pSrc += m_nImageDataStride;
						pDst += dwPitch;
					}
				}

				break;
			}

			case D3DX_SF_A8R8G8B8:
			case D3DX_SF_X8R8G8B8:
			{
				DWORD dwTmp;

				// Convert palette to 16bit 
				DWORD palette[256];
				DWORD *pal = &m_pPalAttach->m_pPalData[0];

				for(int i=0;i<m_pPalAttach->m_nNumEntries;i++)
				{
					dwTmp = pal[i];

					if(dwTmp != m_dwChromaKey)
						palette[i] = dwTmp;
					else
						palette[i] = dwTmp & 0xffffff;	// zero alpha but preserve RGB for pre-alpha test filtering (0 == full transparent, 0xff == full opaque)
				}

				BYTE *pSrc = m_pImageData;
				DWORD *pDst = (DWORD *) ddsd.lpSurface;
				DWORD dwPitch = ddsd.lPitch >> 2;

				if(dwPitch == m_nWidth && m_nImageDataStride == m_nWidth)
				{
					DWORD dwSize = m_nWidth * m_nHeight;

					for(int i=0;i<dwSize;i++)
						pDst[i] = palette[pSrc[i]];
				}

				else
				{
					for(int y=0;y<m_nHeight;y++)
					{
						for(int x=0;x<m_nWidth;x++)
							pDst[x] = palette[pSrc[x]];

						pSrc += m_nImageDataStride;
						pDst += dwPitch;
					}
				}

				break;
			}

			case D3DX_SF_A1R5G5B5:
			{
				WORD dwTmp;

				// Convert palette to 16bit 
				WORD palette[256];
				PALETTEENTRY *pal = (PALETTEENTRY *) &m_pPalAttach->m_pPalData[0];

				for(int i=0;i<m_pPalAttach->m_nNumEntries;i++)
				{
					dwTmp = (pal[i].peRed >> 3) | ((pal[i].peGreen >> 3) << 5) | ((pal[i].peBlue >> 3) << 10) | ((pal[i].peFlags >> 7) << 15);

					if(dwTmp != (WORD) m_dwChromaKey)
						palette[i] = dwTmp;
					else
						palette[i] = dwTmp & 0x7fff;	// zero alpha but preserve RGB for pre-alpha test filtering (0 == full transparent, 0xff == full opaque)
				}

				BYTE *pSrc = m_pImageData;
				WORD *pDst = (WORD *) ddsd.lpSurface;

				if (F4IsBadReadPtr(pSrc, sizeof(BYTE)) || F4IsBadReadPtr(pDst, sizeof(WORD))) // JB 010404 CTD
					break;

				DWORD dwPitch = ddsd.lPitch >> 1;

				if(dwPitch == m_nWidth && m_nImageDataStride == m_nWidth)
				{
					// If source and destination pitch match, use single loop
					DWORD dwSize = m_nWidth * m_nHeight;

					for(int i=0;i<dwSize;i++)
						pDst[i] = palette[pSrc[i]];
				}

				else
				{
					for(int y=0;y<m_nHeight;y++)
					{
						for(int x=0;x<m_nWidth;x++)
							pDst[x] = palette[pSrc[x]];

						pSrc += m_nImageDataStride;
						pDst += dwPitch;
					}
				}

				break;
			}

			case D3DX_SF_A4R4G4B4:
			{
				WORD dwTmp;

				// Convert palette to 16bit 
				WORD palette[256];
				PALETTEENTRY *pal = (PALETTEENTRY *) &m_pPalAttach->m_pPalData[0];

				for(int i=0;i<m_pPalAttach->m_nNumEntries;i++)
				{
					dwTmp = (pal[i].peRed >> 4) | ((pal[i].peGreen >> 4) << 4) | ((pal[i].peBlue >> 4) << 8) | ((pal[i].peFlags >> 4) << 12);

					if(dwTmp != (WORD) m_dwChromaKey)
						palette[i] = dwTmp;
					else
						palette[i] = dwTmp & 0xfff;	// zero alpha but preserve RGB for pre-alpha test filtering (0 == full transparent, 0xff == full opaque)
				}

				BYTE *pSrc = m_pImageData;
				WORD *pDst = (WORD *) ddsd.lpSurface;
				DWORD dwPitch = ddsd.lPitch >> 1;

				if(dwPitch == m_nWidth && m_nImageDataStride == m_nWidth)
				{
					// If source and destination pitch match, use single loop
					DWORD dwSize = m_nWidth * m_nHeight;

					for(int i=0;i<dwSize;i++)
						pDst[i] = palette[pSrc[i]];
				}

				else
				{
					for(int y=0;y<m_nHeight;y++)
					{
						for(int x=0;x<m_nWidth;x++)
							pDst[x] = palette[pSrc[x]];

						pSrc += m_nImageDataStride;
						pDst += dwPitch;
					}
				}

				break;
			}

			case D3DX_SF_R5G6B5:
			{
				ShiAssert(!(m_dwFlags & MPR_TI_CHROMAKEY));

				WORD dwTmp;

				// Convert palette to 16bit 
				WORD palette[256];
				PALETTEENTRY *pal = (PALETTEENTRY *) &m_pPalAttach->m_pPalData[0];

				for(int i=0;i<m_pPalAttach->m_nNumEntries;i++)
				{
					dwTmp = (pal[i].peRed >> 3) | ((pal[i].peGreen >> 2) << 5) | ((pal[i].peBlue >> 3) << 11);
					palette[i] = (WORD) dwTmp;
				}

				BYTE *pSrc = m_pImageData;
				WORD *pDst = (WORD *) ddsd.lpSurface;
				DWORD dwPitch = ddsd.lPitch >> 1;

				if(dwPitch == m_nWidth && m_nImageDataStride == m_nWidth)
				{
					// If source and destination pitch match, use single loop
					DWORD dwSize = m_nWidth * m_nHeight;

					for(int i=0;i<dwSize;i++)
						pDst[i] = palette[pSrc[i]];
				}

				else
				{
					for(int y=0;y<m_nHeight;y++)
					{
						for(int x=0;x<m_nWidth;x++)
							pDst[x] = palette[pSrc[x]];

						pSrc += m_nImageDataStride;
						pDst += dwPitch;
					}
				}

				break;
			}

			case D3DX_SF_R5G5B5:
			{
				ShiAssert(!(m_dwFlags & MPR_TI_CHROMAKEY));

				WORD dwTmp;

				// Convert palette to 16bit 
				WORD palette[256];
				PALETTEENTRY *pal = (PALETTEENTRY *) &m_pPalAttach->m_pPalData[0];

				for(int i=0;i<m_pPalAttach->m_nNumEntries;i++)
				{
					dwTmp = (pal[i].peRed >> 3) | ((pal[i].peGreen >> 3) << 5) | ((pal[i].peBlue >> 3) << 10);
					palette[i] = (WORD) dwTmp;
				}

				BYTE *pSrc = m_pImageData;
				WORD *pDst = (WORD *) ddsd.lpSurface;
				DWORD dwPitch = ddsd.lPitch >> 1;

				if(dwPitch == m_nWidth && m_nImageDataStride == m_nWidth)
				{
					// If source and destination pitch match, use single loop
					DWORD dwSize = m_nWidth * m_nHeight;

					for(int i=0;i<dwSize;i++)
						pDst[i] = palette[pSrc[i]];
				}

				else
				{
					for(int y=0;y<m_nHeight;y++)
					{
						for(int x=0;x<m_nWidth;x++)
							pDst[x] = palette[pSrc[x]];

						pSrc += m_nImageDataStride;
						pDst += dwPitch;
					}
				}

				break;
			}

			default: ShiAssert(false);	// huh
		}

		CheckHR(m_pDDS->Unlock(NULL));

		if(m_dwFlags & MPR_TI_MIPMAP)
		{
			MipLoadContext ctx = { 0, m_pDDS };

			if(g_bShowMipUsage)
				SetMipLevelColor(&ctx);

			CheckHR(m_pDDS->EnumAttachedSurfaces(&ctx, MipLoadCallback));
		}

		return true;
	}

	catch(_com_error e)
	{
		// unlock if still locked
		if(ddsd.lpSurface) m_pDDS->Unlock(NULL);

		ReportTextureLoadError(e.Error(), true);
		return false;
	}
}

void TextureHandle::RestoreAll()
{
	if(m_pDDS && m_pDDS->IsLost() == DDERR_SURFACELOST)
	{
		HRESULT hr = m_pDDS->Restore();

		if(SUCCEEDED(hr))
		{
			#ifdef _DEBUG
			MonoPrint("TextureHandle::RestoreAll - %s restored successfully\n", m_strName.c_str());
			#endif

			Reload();
		}

		#ifdef _DEBUG
		else MonoPrint("TextureHandle::RestoreAll - FAILED to restore %s \n", m_strName.c_str());
		#endif
	}
}

void TextureHandle::Clear()
{
	DDSURFACEDESC2 ddsd;
	ZeroMemory(&ddsd, sizeof(ddsd));

	try
	{
		// Lock the directdraw surface
		ddsd.dwSize = sizeof(ddsd);

		CheckHR(m_pDDS->Lock(NULL, &ddsd, DDLOCK_WAIT | DDLOCK_WRITEONLY |
			DDLOCK_SURFACEMEMORYPTR , NULL));

		// Can be larger but not smaller
		ShiAssert(m_nWidth <= ddsd.dwWidth && m_nHeight <= ddsd.dwHeight);

		if(ddsd.lPitch == ddsd.dwWidth)
		{
			// If source and destination pitch match, use single loop
			DWORD dwSize = ddsd.dwWidth * ddsd.dwHeight * (ddsd.ddpfPixelFormat.dwRGBBitCount >> 3);
			memset(ddsd.lpSurface, 0, dwSize);
		}

		else
		{
			BYTE *pDst = (BYTE *) ddsd.lpSurface;
			DWORD dwSize = ddsd.dwWidth * (ddsd.ddpfPixelFormat.dwRGBBitCount >> 3);

			for(int y=0;y<ddsd.dwHeight;y++)
			{
				memset(pDst, 0, dwSize);
				pDst += ddsd.lPitch;
			}
		}

		CheckHR(m_pDDS->Unlock(NULL));

		if(m_dwFlags & MPR_TI_MIPMAP)
		{
			MipLoadContext ctx = { 0, m_pDDS };

			if(g_bShowMipUsage)
				SetMipLevelColor(&ctx);

			CheckHR(m_pDDS->EnumAttachedSurfaces(&ctx, MipLoadCallback));
		}
	}

	catch(_com_error e)
	{
		// unlock if still locked
		if(ddsd.lpSurface) m_pDDS->Unlock(NULL);
	}
}

bool TextureHandle::SetPriority(DWORD dwPrio)
{
	if(m_pDDS) return SUCCEEDED(m_pDDS->SetPriority(dwPrio));
	return false;
}

void TextureHandle::PreLoad()
{
	if(m_pDDS && m_pD3DD)
		m_pD3DD->PreLoad(m_pDDS);
}

void TextureHandle::ReportTextureLoadError(HRESULT hr, bool bDuringLoad)
{
	#ifdef _DEBUG
	MonoPrint("Texture: Failed to %s texture %s (Code: %X)\n", bDuringLoad ? "load" : "create", m_strName.c_str(), hr);
	#endif
}

void TextureHandle::ReportTextureLoadError(char *strReason)
{
	#ifdef _DEBUG
	MonoPrint("Texture: %s failed to load (Reason: %X)\n", m_strName.c_str(), strReason);
	#endif
}

void TextureHandle::PaletteAttach(PaletteHandle *p)
{
	m_pPalAttach = p;
}

void TextureHandle::PaletteDetach(PaletteHandle *p)
{
	ShiAssert(p == m_pPalAttach);
	m_pPalAttach = NULL;
}

void TextureHandle::StaticInit(IDirect3DDevice7 *pD3DD)
{
	// Warning: Not addref'd
	ShiAssert(pD3DD);
	m_pD3DD = pD3DD;
	if(!m_pD3DD)
		return;

	m_pD3DHWDeviceDesc = new D3DDEVICEDESC7;
	if(!m_pD3DHWDeviceDesc)
		return;

	HRESULT hr = m_pD3DD->GetCaps(m_pD3DHWDeviceDesc);
	ShiAssert(SUCCEEDED(hr));

	// LOW
	///////////////////
	TEXTURESEARCHINFO tsi_low[4] =
	{
		{ 8, 0, TRUE, FALSE, &m_arrPF[0] },
		{ 16, 1, FALSE, FALSE, &m_arrPF[1] },	
		{ 16, 4, FALSE, FALSE, &m_arrPF[2] },
		{ 16, 4, FALSE, FALSE, &m_arrPF[3] },
	};

	TEXTURESEARCHINFO tsi_alt_low[4] =
	{
		{ 16, 0, FALSE, FALSE, &m_arrPF[0] },	
		{ 16, 1, FALSE, FALSE, &m_arrPF[1] },	
		{ 16, 1, FALSE, FALSE, &m_arrPF[2] },
		{ 16, 1, FALSE, FALSE, &m_arrPF[3] },
	};

	TEXTURESEARCHINFO tsi_alt2_low[4] =
	{
		{ 16, 0, FALSE, FALSE, &m_arrPF[0] },	
		{ 16, 1, FALSE, FALSE, &m_arrPF[1] },
		{ 16, 1, FALSE, FALSE, &m_arrPF[2] },
		{ 16, 1, FALSE, FALSE, &m_arrPF[3] },
	};

	// MED
	///////////////////
	TEXTURESEARCHINFO tsi_med[4] =
	{
		{ 8, 0, TRUE, FALSE, &m_arrPF[0] },
		{ 16, 1, FALSE, FALSE, &m_arrPF[1] },	
		{ 16, 4, FALSE, FALSE, &m_arrPF[2] },
		{ 16, 4, FALSE, FALSE, &m_arrPF[3] },
	};

	TEXTURESEARCHINFO tsi_alt_med[4] =
	{
		{ 32, 0, FALSE, FALSE, &m_arrPF[0] },	
		{ 16, 1, FALSE, FALSE, &m_arrPF[1] },	
		{ 16, 4, FALSE, FALSE, &m_arrPF[2] },
		{ 16, 4, FALSE, FALSE, &m_arrPF[3] },
	};

	TEXTURESEARCHINFO tsi_alt2_med[4] =
	{
		{ 16, 0, FALSE, FALSE, &m_arrPF[0] },	
		{ 16, 1, FALSE, FALSE, &m_arrPF[1] },
		{ 16, 1, FALSE, FALSE, &m_arrPF[2] },
		{ 16, 1, FALSE, FALSE, &m_arrPF[3] },
	};

	// HIGH
	///////////////////
	TEXTURESEARCHINFO tsi_high[4] =
	{
		{ 8, 0, TRUE, FALSE, &m_arrPF[0] },
		{ 32, 8, FALSE, FALSE, &m_arrPF[1] },	
		{ 32, 8, FALSE, FALSE, &m_arrPF[2] },
		{ 32, 8, FALSE, FALSE, &m_arrPF[3] },
	};

	TEXTURESEARCHINFO tsi_alt_high[4] =
	{
		{ 32, 0, FALSE, FALSE, &m_arrPF[0] },	
		{ 16, 1, FALSE, FALSE, &m_arrPF[1] },	
		{ 16, 4, FALSE, FALSE, &m_arrPF[2] },
		{ 16, 4, FALSE, FALSE, &m_arrPF[3] },
	};

	TEXTURESEARCHINFO tsi_alt2_high[4] =
	{
		{ 16, 0, FALSE, FALSE, &m_arrPF[0] },	
		{ 16, 1, FALSE, FALSE, &m_arrPF[1] },
		{ 16, 1, FALSE, FALSE, &m_arrPF[2] },
		{ 16, 1, FALSE, FALSE, &m_arrPF[3] },
	};

	TEXTURESEARCHINFO *ptsi;
	TEXTURESEARCHINFO *ptsi_alt;
	TEXTURESEARCHINFO *ptsi_alt2;

	switch(PlayerOptions.m_eTexMode - 70158)	// OW FIXME: hack, use constant
	{
		case PlayerOptionsClass::TEX_MODE_MED:
		{
			ptsi = tsi_med;
			ptsi_alt = tsi_alt_med;
			ptsi_alt2 = tsi_alt2_med;
			break;
		}

		case PlayerOptionsClass::TEX_MODE_HIGH:
		{
			ptsi = tsi_high;
			ptsi_alt = tsi_alt_high;
			ptsi_alt2 = tsi_alt2_high;
			break;
		}

		case PlayerOptionsClass::TEX_MODE_LOW:
		default:
		{
			ptsi = tsi_low;
			ptsi_alt = tsi_alt_low;
			ptsi_alt2 = tsi_alt2_low;
			break;
		}
	}

	for(int i=0;i<TEX_CAT_MAX;i++)
	{
		m_pD3DD->EnumTextureFormats(TextureSearchCallback, &ptsi[i]);

		if(!ptsi[i].bFoundGoodFormat)
		{
			m_pD3DD->EnumTextureFormats(TextureSearchCallback, &ptsi_alt[i]);

			if(!ptsi_alt[i].bFoundGoodFormat)
			{
				m_pD3DD->EnumTextureFormats(TextureSearchCallback, &ptsi_alt2[i]);

				if(!ptsi_alt2[i].bFoundGoodFormat)
				{
					ShiAssert(false);
				}
			}
		}
	}
}

void TextureHandle::StaticCleanup()
{
	if(m_pD3DHWDeviceDesc)
	{
		delete m_pD3DHWDeviceDesc;
		m_pD3DHWDeviceDesc = NULL;
	}

	if(m_pD3DD)
	{
		// m_pD3DD->Release();
		m_pD3DD = NULL;
	}
}

HRESULT CALLBACK TextureHandle::TextureSearchCallback(DDPIXELFORMAT* pddpf, VOID* param)
{
    if(NULL==pddpf || NULL==param)
        return DDENUMRET_OK;

    TEXTURESEARCHINFO* ptsi = (TEXTURESEARCHINFO*)param;

    // Skip any funky modes
    if(pddpf->dwFlags & (DDPF_LUMINANCE|DDPF_BUMPLUMINANCE|DDPF_BUMPDUDV))
        return DDENUMRET_OK;

    // Check for palettized formats
    if(ptsi->bUsePalette)
    {
        if(!(pddpf->dwFlags & DDPF_PALETTEINDEXED8))
            return DDENUMRET_OK;

        // Accept the first 8-bit palettized format we get
        memcpy( ptsi->pddpf, pddpf, sizeof(DDPIXELFORMAT));
        ptsi->bFoundGoodFormat = TRUE;
        return DDENUMRET_CANCEL;
    }

    // Else, skip any paletized formats (all modes under 16bpp)
    if(pddpf->dwRGBBitCount < 16)
        return DDENUMRET_OK;

    // Skip any FourCC formats
    if(pddpf->dwFourCC != 0)
        return DDENUMRET_OK;

	// Calc alpha depth
	DWORD dwMask = pddpf->dwRGBAlphaBitMask;
	WORD wAlphaBits = 0;
	while(dwMask) {	dwMask = dwMask & ( dwMask - 1 ); wAlphaBits++; }

    // Make sure current alpha format agrees with requested format type
    if((ptsi->dwDesiredAlphaBPP) && !(pddpf->dwFlags&DDPF_ALPHAPIXELS) || wAlphaBits < ptsi->dwDesiredAlphaBPP)
        return DDENUMRET_OK;
    if((!ptsi->dwDesiredAlphaBPP) && (pddpf->dwFlags&DDPF_ALPHAPIXELS))
        return DDENUMRET_OK;

    // Check if we found a good match
    if(pddpf->dwRGBBitCount == ptsi->dwDesiredBPP)
    {
        memcpy(ptsi->pddpf, pddpf, sizeof(DDPIXELFORMAT));
        ptsi->bFoundGoodFormat = TRUE;
        return DDENUMRET_CANCEL;
    }

    return DDENUMRET_OK;
}

#ifdef _DEBUG
void TextureHandle::MemoryUsageReport()
{
}

#endif

DWORD RGB32ToSurfaceColor(DWORD col, LPDIRECTDRAWSURFACE7 lpDDSurface, DDSURFACEDESC2 *pddsd = NULL)
{
	// NOTE: This function is sloooooow

	// Is it a palette surface?
	IDirectDrawPalettePtr pPal;
	if(SUCCEEDED(lpDDSurface->GetPalette(&pPal)))
	{
		// Palettes are 32 Bit
		return col;
	}

	DDSURFACEDESC2 ddsd;

	if(pddsd == NULL)
	{
		ZeroMemory(&ddsd, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		HRESULT hr = lpDDSurface->GetSurfaceDesc(&ddsd);
		ShiAssert(SUCCEEDED(hr));

		pddsd = &ddsd;
	}

	// Compute the right shifts required to get from 24 bit RGB to this pixel format
	UInt32	mask;

	int redShift;
	int greenShift;
	int blueShift;

	// RED
	mask = pddsd->ddpfPixelFormat.dwRBitMask;
	redShift = 8;
	ShiAssert( mask );

	while( !(mask & 1) )
	{
		mask >>= 1;
		redShift--;
	}

	while( mask & 1 )
	{
		mask >>= 1;
		redShift--;
	}

	// GREEN
	mask = pddsd->ddpfPixelFormat.dwGBitMask;
	greenShift = 16;
	ShiAssert( mask );

	while( !(mask & 1) )
	{
		mask >>= 1;
		greenShift--;
	}

	while( mask & 1 )
	{
		mask >>= 1;
		greenShift--;
	}

	// BLUE
	mask = pddsd->ddpfPixelFormat.dwBBitMask;
	ShiAssert( mask );
	blueShift = 24;

	while( !(mask & 1) )
	{
		mask >>= 1;
		blueShift--;
	}

	while( mask & 1 )
	{
		mask >>= 1;
		blueShift--;
	}

	// Convert the key color from 32 bit RGB to the current pixel format
	DWORD dwResult;	

	// RED
	if (redShift >= 0)
		dwResult = (col >>  redShift) & pddsd->ddpfPixelFormat.dwRBitMask;
	else
		dwResult = (col << -redShift) & pddsd->ddpfPixelFormat.dwRBitMask;

	// GREEN
	if (greenShift >= 0)
		dwResult |= (col >>  greenShift) & pddsd->ddpfPixelFormat.dwGBitMask;
	else
		dwResult |= (col << -greenShift) & pddsd->ddpfPixelFormat.dwGBitMask;

	// BLUE
	if(blueShift >= 0)
		dwResult |= (col >>  blueShift) & pddsd->ddpfPixelFormat.dwBBitMask;
	else
		dwResult |= (col << -blueShift) & pddsd->ddpfPixelFormat.dwBBitMask;

	return dwResult;
}

static void SetMipLevelColor(MipLoadContext *pCtx)
{
	static DWORD arrMipColors[] = { 0xffffff, 0xff0000, 0xff00, 0xff, 0xffff00, 0xffff, 0xff00ff, 0x808080, 0x800000, 0x8000, 0x80, 0x808000, 0x8080, 0x800080,  };
	_ASSERTE(pCtx->nLevel < sizeof(arrMipColors) / sizeof(arrMipColors[0]));

	DDSURFACEDESC2 ddsd;
	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	HRESULT hr = pCtx->lpDDSurface->GetSurfaceDesc(&ddsd);
	ShiAssert(SUCCEEDED(hr));

	if(FAILED(hr))
		return;

	// Fill surface with a unique color representing the mipmap level
	DDBLTFX bfx;
	ZeroMemory(&bfx, sizeof(bfx));
	bfx.dwSize = sizeof(bfx);
	bfx.dwFillColor = RGB32ToSurfaceColor(arrMipColors[pCtx->nLevel], pCtx->lpDDSurface, &ddsd);

	hr = pCtx->lpDDSurface->Blt(NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &bfx);
}

static HRESULT WINAPI MipLoadCallback(LPDIRECTDRAWSURFACE7 lpDDSurface, LPDDSURFACEDESC2 lpDDSurfaceDesc, LPVOID lpContext)
{
	MipLoadContext *pCtx = (MipLoadContext *) lpContext;
	HRESULT hr;

	if(lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_MIPMAP)
	{
		if(g_bShowMipUsage)
			SetMipLevelColor(pCtx);
		else
		{
			// Perform a 2:1 stretch blit from the parent surface
			hr = lpDDSurface->Blt(NULL, pCtx->lpDDSurface, NULL, DDBLT_WAIT, NULL);
	
			if(FAILED(hr))
			{
				ShiAssert(false);		// OW: Message me if you see this happening
				return DDENUMRET_CANCEL;
			}
		}

		// Process next lower mipmap level recursively
		MipLoadContext ctx = { pCtx->nLevel + 1, lpDDSurface };
		hr = lpDDSurface->EnumAttachedSurfaces(&ctx, MipLoadCallback);

		if(FAILED(hr))
		{
			ShiAssert(false);		// OW: Message me if you see this happening
			return DDENUMRET_CANCEL;
		}
	}

	return DDENUMRET_OK;
}
