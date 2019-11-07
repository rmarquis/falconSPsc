///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include "stdhdr.h"
#include "dispcfg.h"
#include "Graphics\Include\Setup.h"
#include "Graphics\Include\RenderOW.h"
#include "otwdrive.h"
#include "Graphics\Include\image.h"
#include "Graphics\Include\device.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Properties of the animation image palette layout
static const int	NUM_SPLASH_FRAMES	= 5;
static const int	PAL_FRAME_LENGTH	= 32;

// location and dimensions of the radar thing (which we want to cover up)
static const int	COVER_TOP	= 246;
static const int	COVER_LEFT	= 159;

// Pointers to the global resources used while the loading screen is up
BYTE			*originalImage		= NULL;
unsigned long	*originalPalette	= NULL;
ImageBuffer		*coverImage			= NULL;

// Some data about the source image
static int	originalWidth	= 0;
static int	originalHeight	= 0;
static int	coverWidth		= 0;
static int	coverHeight		= 0;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/*
** Called to start showing splash screen
** This loads the loading screen data and show the initial frame
*/

void OTWDriverClass::SetupSplashScreen(void)
{
	int					result;
	CImageFileMemory 	texFile;
	BYTE				*imagePtr;
	char				*filename;
	unsigned long		*palPtr;
	void				*buffer;
	int					r, c;
	char				string[MAX_PATH];


	//********************************
	// Load the primary source image
	//********************************
	// Make sure we recognize this file type

	if (OTWImage->targetXres () >= 1024)
	{
		filename = "art\\splash\\load10.gif";
	}
	else if (OTWImage->targetXres () >= 800)
	{
		filename = "art\\splash\\load8.gif";
	}
	else
	{
		filename = "art\\splash\\load6.gif";
	}

	texFile.imageType = CheckImageType( filename );
	if ( texFile.imageType < 0 ) {
		ShiWarning( "We failed to read a splash screen image" );
		return;
	}

	// Open the input file
	result = texFile.glOpenFileMem( filename );
	if ( result != 1 ) {
		ShiWarning( "We failed to read a splash screen image" );
		return;
	}

	// Read the image data (note that ReadTextureImage will close texFile for us)
	texFile.glReadFileMem();
	result = ReadTextureImage( &texFile );
	if (result != GOOD_READ) {
		ShiWarning( "We failed to read a splash screen image" );
		return;
	}

	// Store the image data
	originalWidth	= texFile.image.width;
	originalHeight	= texFile.image.height;
	originalImage	= texFile.image.image;
	originalPalette	= texFile.image.palette;
	ShiAssert( originalImage );
	ShiAssert( originalPalette );

	//*****************************
	// Load the cover source image
	//*****************************
	// Make sure we recognize this file type
	sprintf (string, "art\\splash\\load4.gif");
	texFile.imageType = CheckImageType( string );
	if ( texFile.imageType < 0 ) {
		ShiWarning( "We failed to read a splash screen image" );
		return;
	}

	// Open the input file
	result = texFile.glOpenFileMem( "art\\splash\\load4.gif" );
	if ( result != 1 ) {
		ShiWarning( "We failed to read a splash screen image" );
		return;
	}

	// Read the image data (note that ReadTextureImage will close texFile for us)
	texFile.glReadFileMem();
	result = ReadTextureImage( &texFile );
	if (result != GOOD_READ) {
		ShiWarning( "We failed to read a splash screen image" );
		return;
	}

	// Store the image data
	coverWidth	= texFile.image.width;
	coverHeight	= texFile.image.height;
	imagePtr	= texFile.image.image;
	palPtr		= texFile.image.palette;
	ShiAssert( imagePtr );
	ShiAssert( palPtr );

	//*********************************************
	// Convert the cover image into an ImageBuffer
	//*********************************************
	// Setup the target image buffer
	coverImage = new ImageBuffer();
	coverImage->Setup( OTWImage->GetDisplayDevice(), coverWidth, coverHeight, SystemMem, None );

	// Lock the surface and do the pixel lookups
	buffer = coverImage->Lock();
	ShiAssert( buffer );

	// OW
	switch(coverImage->PixelSize())
	{
		case 2:
		{
			WORD *pixel;

			for (r=0; r<coverHeight; r++)
			{
				pixel = (WORD*)coverImage->Pixel( buffer, r, 0 );
				for (c=0; c<coverWidth; c++) *pixel++ = coverImage->Pixel32toPixel16( palPtr[*imagePtr++]);
			}

			break;
		}

		case 4:
		{
			DWORD *pixel;

			for (r=0; r<coverHeight; r++)
			{
				pixel = (DWORD*)coverImage->Pixel( buffer, r, 0 );
				for (c=0; c<coverWidth; c++) *pixel++ = coverImage->Pixel32toPixel32( palPtr[*imagePtr++]);
			}

			break;
		}

		default: ShiAssert(false);
	}

	coverImage->Unlock();

	// Set the chromakey color for the cover image
	coverImage->SetChromaKey( palPtr[0] );

	// Free the uncoverted cover data
	glReleaseMemory( (char*)texFile.image.image );
	glReleaseMemory( (char*)texFile.image.palette );
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/*
** Called to finish showing splash screen
** This frees the loading screen data
*/

void OTWDriverClass::CleanupSplashScreen(void)
{
	// Release the original image data
	glReleaseMemory( (char*)originalImage );
	glReleaseMemory( (char*)originalPalette );
	originalImage = NULL;
	originalPalette = NULL;

	// Release the converted cover image
	coverImage->Cleanup();
	delete coverImage;
	coverImage = NULL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/*
** This function is called periodically during startup to update the
** animation on the load screen
*/

void OTWDriverClass::SplashScreenUpdate( int frame )
{
	int					top, left;
	BYTE				*imagePtr;
	void				*buffer;
	int					x, y;
	unsigned long		tweakedPalette[256];
	unsigned long		*srcPal, *dstPal;
	unsigned long		*startLit, *stopLit, *startInvar, *stop;
	RECT				srcRect, dstRect;

	// Validate our parameter
	if( frame >= NUM_SPLASH_FRAMES ) {
		ShiWarning( "Bad frame requested for the splash screen" );
		return;
	}

	if (!originalImage)
	{
		return;
	}

	// Get the upper left corner of the target area
	top		= (OTWImage->targetYres() - originalHeight) / 2;
	left	= (OTWImage->targetXres() - originalWidth) / 2;
	if ((top < 0) || (left < 0)) {
		ShiWarning( "Source art for splash screen too big!" );
		return;
	}

	// Clear the back buffer to black to erase anything that might already be there
	renderer->SetViewport( -1.0f, 1.0f, 1.0f, -1.0f );
	renderer->StartFrame();
	renderer->SetBackground( 0xFF000000 );
	renderer->ClearFrame();
	renderer->FinishFrame();

	// Darken all but the specified frame in the palette
	srcPal		= originalPalette;
	dstPal		= tweakedPalette;
	startLit	= originalPalette + frame * PAL_FRAME_LENGTH;
	stopLit		= startLit + PAL_FRAME_LENGTH;
	startInvar	= originalPalette + NUM_SPLASH_FRAMES * PAL_FRAME_LENGTH;
	stop		= originalPalette + 256;

	ShiAssert( srcPal <= startLit );
	ShiAssert( startLit <= stopLit );
	ShiAssert( stopLit <= startInvar );
	ShiAssert( startInvar <= stop );

	// Divide the dimmed color intensities by 4 (knock them down 2 bits in each channel)
	while (srcPal < startLit)
	{
		*dstPal++ = (*srcPal++ & 0x00FCFCFC) >> 1;
	}

	// Copy the "lit" color intensities
	while (srcPal < stopLit)
	{
		*dstPal++ = *srcPal++;
	}

	// Divide the dimmed color intensities by 4 (knock them down 2 bits in each channel)
	while (srcPal < startInvar)
	{
		*dstPal++ = (*srcPal++ & 0x00FCFCFC) >> 1;
	}

	// Copy the invariant high portion of the palette
	while (srcPal < stop)
	{
		*dstPal++ = *srcPal++;
	}

	// Lock the surface and do the pixel lookups
	buffer = OTWImage->Lock();
	ShiAssert( buffer );

	imagePtr = originalImage;

	// OW
	switch(coverImage->PixelSize())
	{
		case 2:
		{
			WORD *pixel;

			for (y = 0; y < originalHeight; y ++)
			{
				pixel = (WORD*)OTWImage->Pixel (buffer, y + top, left);

				//for (x = 0; x < originalWidth; x ++)
				for (x = 0; x < originalWidth && imagePtr && *imagePtr >= 0 && *imagePtr < 256; x ++) // JB 010326 CTD
				{
					if (OTWImage)
						*pixel = OTWImage->Pixel32toPixel16(tweakedPalette[*imagePtr]);

					pixel ++;
					imagePtr ++;
				}
			}

			break;
		}

		case 4:
		{
			DWORD *pixel;

			for (y = 0; y < originalHeight; y ++)
			{
				pixel = (DWORD*)OTWImage->Pixel (buffer, y + top, left);

				for (x = 0; x < originalWidth && imagePtr && *imagePtr >= 0 && *imagePtr < 256; x ++)
				{
					if (OTWImage)
						*pixel = OTWImage->Pixel32toPixel32(tweakedPalette[*imagePtr]);

					pixel ++;
					imagePtr ++;
				}
			}

			break;
		}

		default: ShiAssert(false);
	}

	OTWImage->Unlock();

	// Cover up the old radar wheel
	srcRect.top		= 0;
	srcRect.left	= 0;
	srcRect.bottom	= coverHeight;
	srcRect.right	= coverWidth;
	dstRect.top		= srcRect.top  + top + COVER_TOP;
	dstRect.left	= srcRect.left + left + COVER_LEFT;
	dstRect.bottom	= dstRect.top  + coverHeight;
	dstRect.right	= dstRect.left + coverWidth;
//	OTWImage->ComposeTransparent (coverImage, &srcRect, &dstRect);

	// flip the surface
	OTWImage->SwapBuffers(NULL);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void OTWDriverClass::ShowSimpleWaitScreen( char *name )
{
	int		top, left;
	int		width, height;
	char	filename[MAX_PATH];


	if (OTWImage->targetXres () >= 1024)
	{
		sprintf (filename, "art\\splash\\%s10.gif", name);
		width = 1024;
		height = 768;
	}
	else if (OTWImage->targetXres () >= 800)
	{
		sprintf (filename, "art\\splash\\%s8.gif", name);
		width = 800;
		height = 600;
	}
	else
	{
		sprintf (filename, "art\\splash\\%s6.gif", name);
		width = 640;
		height = 480;
	}

	// Get the upper left corner of the target area
	top		= (OTWImage->targetYres() - height) / 2;
	left	= (OTWImage->targetXres() - width) / 2;
	if ((top < 0) || (left < 0)) {
		ShiWarning( "Source art for splash screen too big!" );
		return;
	}

	// Clear the back buffer to black to erase anything that might already be there
	renderer->SetViewport( -1.0f, 1.0f, 1.0f, -1.0f );
	renderer->StartFrame();
	renderer->SetBackground( 0xFF000000 );
	renderer->ClearFrame();
	renderer->Render2DBitmap( 0, 0, left, top, width, height, filename );
	renderer->FinishFrame();
	OTWImage->SwapBuffers(NULL);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
