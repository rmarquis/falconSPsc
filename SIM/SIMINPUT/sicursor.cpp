#include <stdio.h>

#include "stdhdr.h"
#include "Graphics\Include\device.h"
#include "Graphics\Include\Filemem.h"
#include "Graphics\Include\Image.h"
#include "Graphics\Include\render2d.h"
#include "f4thread.h"
#include "f4find.h"
#include "otwdrive.h"
#include "sinput.h"
#include "dispcfg.h"


// ALL RESMGR CODE ADDITIONS START HERE
#define _SI_USE_RES_MGR_ 1

#ifndef _SI_USE_RES_MGR_ // DON'T USE RESMGR

	#define SI_HANDLE FILE
	#define SI_OPEN   fopen
	#define SI_READ   fread
	#define SI_CLOSE  fclose
	#define SI_SEEK   fseek
	#define SI_TELL   ftell

#else // USE RESMGR

	#include "cmpclass.h"
	extern "C"
	{
		#include "codelib\resources\reslib\src\resmgr.h"
	}

	#define SI_HANDLE FILE
	#define SI_OPEN   RES_FOPEN
	#define SI_READ   RES_FREAD
	#define SI_CLOSE  RES_FCLOSE
	#define SI_SEEK   RES_FSEEK
	#define SI_TELL   RES_FTELL

#endif

#define NUM_FIELDS 5

SimCursor*			gpSimCursors;
int					gTotalCursors;


BOOL CreateSimCursors(){
	SI_HANDLE*	pCursorFile;
	char			pFileName[MAX_PATH] = "";
	char			pFilePath[MAX_PATH] = "";
	BOOL			Result = TRUE;
	int				i;
	
	sprintf(pFilePath, "%s%s%s", FalconDataDirectory, SIM_CURSOR_DIR, SIM_CURSOR_FILE);
	pCursorFile = SI_OPEN(pFilePath, "r");

	if(pCursorFile == NULL){
		return (FALSE);
	}

	if(fscanf(pCursorFile, "%d", &gTotalCursors) != 1){
		return(FALSE);
	}

	gpSimCursors = new SimCursor[gTotalCursors];

	for(i = 0; i < gTotalCursors; i++){
      // Note, the %hu is to read in unsigned shorts instead of unsigned ints
		if(fscanf(pCursorFile, "\t%hu %hu %hu %hu %s\n", &gpSimCursors[i].Width, &gpSimCursors[i].Height,
										&gpSimCursors[i].xHotspot, &gpSimCursors[i].yHotspot, pFileName) != NUM_FIELDS){
			return(FALSE);
		}
		
		gpSimCursors[i].CursorBuffer = new ImageBuffer;

		sprintf(pFilePath, "%s%s%s", FalconDataDirectory, SIM_CURSOR_DIR, pFileName);

		gpSimCursors[i].CursorBuffer->Setup(&FalconDisplay.theDisplayDevice, gpSimCursors[i].Width, gpSimCursors[i].Height, SystemMem, None);
		gpSimCursors[i].CursorBuffer->SetChromaKey(0xFFFF0000);

// OW FIXME: fix it :)
#if 1	// This avoids using MPR since the bitmap function is broken at the moment...  SCR 5/14/98
		int					r, c;
		BYTE				*p;
		void				*imgPtr;

		int					result;
		CImageFileMemory 	texFile;

		// Make sure we recognize this file type
		texFile.imageType = CheckImageType( pFilePath );
		ShiAssert( texFile.imageType != IMAGE_TYPE_UNKNOWN );

		// Open the input file
		result = texFile.glOpenFileMem( pFilePath );
		ShiAssert( result == 1 );

		// Read the image data (note that ReadTextureImage will close texFile for us)
		texFile.glReadFileMem();
		result = ReadTextureImage( &texFile );
		if (result != GOOD_READ) {
			ShiError( "Failed to read bitmap.  CD Error?" );
		}

		ShiAssert( texFile.image.palette );
		ShiAssert( texFile.image.width == gpSimCursors[i].Width );
		ShiAssert( texFile.image.height == gpSimCursors[i].Height );

		// Convert from palettized to screen format
		p = texFile.image.image;
		imgPtr = gpSimCursors[i].CursorBuffer->Lock();

		switch(gpSimCursors[i].CursorBuffer->PixelSize())
		{
			case 2:
			{
				for (r=0; r<gpSimCursors[i].Height; r++) {
					for (c=0; c<gpSimCursors[i].Width; c++) {
						*(WORD*)gpSimCursors[i].CursorBuffer->Pixel( imgPtr, r, c ) = gpSimCursors[i].CursorBuffer->Pixel32toPixel16( texFile.image.palette[*p++] );
					}
				}

				break;
			}

			case 4:
			{
				for (r=0; r<gpSimCursors[i].Height; r++) {
					for (c=0; c<gpSimCursors[i].Width; c++) {
						*(DWORD*)gpSimCursors[i].CursorBuffer->Pixel( imgPtr, r, c ) = gpSimCursors[i].CursorBuffer->Pixel32toPixel32( texFile.image.palette[*p++] );
					}
				}

				break;
			}

			default: ShiAssert(false);
		}

		gpSimCursors[i].CursorBuffer->Unlock();

		// Release the raw image data
		glReleaseMemory( (char*)texFile.image.palette );
		glReleaseMemory( (char*)texFile.image.image );
#else
		Render2D		CursorRenderer;
		CursorRenderer.Setup(gpSimCursors[i].CursorBuffer);
		CursorRenderer.StartFrame();
		CursorRenderer.ClearFrame();
		CursorRenderer.Render2DBitmap(0, 0, 0, 0, gpSimCursors[i].Width, gpSimCursors[i].Height, pFilePath);
		CursorRenderer.FinishFrame();
		CursorRenderer.SetColor (0xff00ff00);
		CursorRenderer.Cleanup();
#endif
	}


	SI_CLOSE(pCursorFile);

	return(Result);
}


void CleanupSimCursors(){
	int	i;
	
	for(i = 0; i < gTotalCursors; i++){
		if(gpSimCursors[i].CursorBuffer){
			gpSimCursors[i].CursorBuffer->Cleanup();
			delete(gpSimCursors[i].CursorBuffer);
		}
		gpSimCursors[i].CursorBuffer = NULL;
	}

	delete [] gpSimCursors;
	gTotalCursors = 0;
}


void ClipAndDrawCursor(int displayWidth, int displayHeight) {

	RECT CursorSrc;
	RECT CursorDest;

	if(gSelectedCursor < 0 || gSelectedCursor >= gTotalCursors) {
		return;
	}

	CursorSrc.top		= 0;
	CursorSrc.left		= 0;
	CursorSrc.bottom	= gpSimCursors[gSelectedCursor].Height;
	CursorSrc.right	= gpSimCursors[gSelectedCursor].Width;

	CursorDest.top		=	gyPos - gpSimCursors[gSelectedCursor].yHotspot; 
	CursorDest.left	=	gxPos - gpSimCursors[gSelectedCursor].xHotspot;
	CursorDest.bottom	=	CursorDest.top	+ gpSimCursors[gSelectedCursor].Height;
	CursorDest.right	=	CursorDest.left + gpSimCursors[gSelectedCursor].Width;

	gyLast = gyPos;
	gxLast = gxPos;

	if(CursorDest.top < 0) 
	{					// Clipping necessary for the swap compose
		CursorSrc.top -= CursorDest.top;
		CursorDest.top = 0;
	}

	if(CursorDest.left < 0) 
	{
		CursorSrc.left -= CursorDest.left;
		CursorDest.left = 0;
	}

	if(CursorDest.right > displayWidth - 1) 
	{
		CursorSrc.right -= (CursorDest.right - displayWidth + 1);
		CursorDest.right = displayWidth - 1;
	}
	
	if(CursorDest.bottom > displayHeight - 1) 
	{
		CursorSrc.bottom -= (CursorDest.bottom - displayHeight + 1);
		CursorDest.bottom = displayHeight - 1;
	}

	OTWDriver.OTWImage->ComposeTransparent(gpSimCursors[gSelectedCursor].CursorBuffer, &CursorSrc, &CursorDest);
}