/***************************************************************************\
    KneeBoard.cpp
    Scott Randolph
    September 4, 1998

    This class provides the pilots knee board notepad and map.
\***************************************************************************/
#include "stdafx.h"
#include "stdhdr.h"
#include "simveh.h"
#include "campwp.h"
#include "flight.h"
#include "playerop.h"
#include "brief.h"
#include "Graphics\Include\tod.h"
#include "Graphics\Include\TMap.h"
#include "Graphics\Include\filemem.h"
#include "Graphics\Include\image.h"
#include "kneeboard.h"
#include "otwdrive.h"
#include "cpmanager.h"
#include "Falclib\include\dispcfg.h"
#include "flightdata.h"	//MI
#include "aircrft.h"	//MI
#include "phyconst.h"	//MI
#include "simdrive.h"	//MI
#include "navsystem.h"	//MI
#include "f4find.h"

extern bool g_bRealisticAvionics;	//MI
extern bool g_bINS;					//MI


static const char	KNEEBOARD_MAP_NAME[]		= "art\\ckptart\\KneeMap.gif";
static const char	THR_KNEEBOARD_MAP_NAME[]		= "KneeMap.gif";
//static const int	KNEEBOARD_MAP_KM_PER_TEXEL	= 2;		// Property of source art on disk
static const float	KNEEBOARD_SMALLEST_MAP_FRACTION	= 4.0f;	// What is the smallest fraction of the map we'll zoom to
static const float	BORDER_PERCENT	= 0.05f;		// How much map to display outside the bounding box of the waypoints
static const UInt32	WP_COLOR		= 0xFF0000FF;	// The color of the waypoint marks
static const float	WP_SIZE			= 0.03f;		// The radius of the waypoint marker symbol
static const float  ORIDE_WP_SIZE	= 0.08f;		// The size of the override waypoint marker
static const UInt32	AC_COLOR		= 0xFF00FFFF;	// The color of the aircraft location marker
static const float	AC_SIZE			= 0.06f;		// The radius of the aircraft location marker

extern bool g_bVoodoo12Compatible; // JB 010330 Disables the cockpit kneemap to prevent CTDs on the Voodoo 1 and 2.

KneeBoard::KneeBoard()
{
	m_pMapImage = NULL;
	m_imageloaded = false;
	m_imageFile.image.image = NULL;
	m_imageFile.image.palette = NULL;
}

KneeBoard::~KneeBoard()
{
}

void KneeBoard::Setup( DisplayDevice *device, int top, int left, int bottom, int right )
{
	dstRect.top		= top;
	dstRect.left	= left;
	dstRect.bottom	= bottom;
	dstRect.right	= right;

	srcRect.top		= 0;
	srcRect.left	= 0;
	srcRect.bottom	= bottom - top;
	srcRect.right	= right - left;

	mode = BRIEF;

	LoadKneeImage(m_imageFile);
	m_imageloaded = true;

	// Setup our off screen map buffer and renderer
	MPRSurfaceType front = FalconDisplay.theDisplayDevice.IsHardware() ? VideoMem : SystemMem;
	m_pMapImage = new ImageBuffer;
	m_pMapImage->Setup( device, srcRect.right, srcRect.bottom, front, None );
	Render2D::Setup( m_pMapImage );
}


void KneeBoard::Cleanup( void )
{
	if(m_pMapImage)
	{
		m_pMapImage->Cleanup();
		delete m_pMapImage;
	}
	if (m_imageloaded) {
		if (m_imageFile.image.palette)
			glReleaseMemory( (char*)m_imageFile.image.palette );
		if (m_imageFile.image.image)
			glReleaseMemory( (char*)m_imageFile.image.image );

		m_imageloaded = false;
	}

	Render2D::Cleanup();
}


void KneeBoard::DisplayBlit( ImageBuffer *targetBuffer, Render2D *renderer, SimVehicleClass *platform )
{
	if (g_bVoodoo12Compatible) // JB 010330
		return;

	if (mode == MAP) {
		// BLT in the map
		targetBuffer->Compose( m_pMapImage, &srcRect, &dstRect);
	}
}


void KneeBoard::DisplayDraw( ImageBuffer *targetBuffer, Render2D *renderer, SimVehicleClass *platform )
{
	// Set the viewport to the active region of our display
	renderer->SetViewport(	(float)dstRect.left  /targetBuffer->targetXres()*( 2.0f) - 1.0f, 
							(float)dstRect.top   /targetBuffer->targetYres()*(-2.0f) + 1.0f,
							(float)dstRect.right /targetBuffer->targetXres()*( 2.0f) - 1.0f,
							(float)dstRect.bottom/targetBuffer->targetYres()*(-2.0f) + 1.0f );
	
	if (mode == MAP) {
		// If we're not in Realistic mode, draw the current position marker
		// M.N. Added Full realism mode
		if (PlayerOptions.GetAvionicsType() != ATRealistic && PlayerOptions.GetAvionicsType() != ATRealisticAV ) {
			DrawCurrentPosition( targetBuffer, renderer, platform );
		}
	} else {
		DrawMissionText( renderer, platform );
	}

	// Clear the local coordinates  (I guess we're not expected to do this, so don't bother...)
//	renderer->SetViewport(-1.0f, 1.0f, 1.0f, -1.0f);
}


void KneeBoard::Refresh( SimVehicleClass *platform )
{
	RenderMap( platform );
}


void KneeBoard::DrawMissionText( Render2D *renderer, SimVehicleClass *platform )
{
	float		LINE_HEIGHT = renderer->TextHeight();
	int			lines;
	float		v = 0.80f;
	int oldFont = VirtualDisplay::CurFont();

	if (!SimDriver.RunningDogfight() && !SimDriver.RunningInstantAction())	// M.N. CTD-Fix
	{
	
	renderer->SetColor( 0xFF000000 );		// Black (ink color)
//	lines = (int)((rand()/(float)RAND_MAX * 0xFFFFFF) + 0xff000000);
//	MonoPrint ("Knee color = 0X%X\n", lines);
//	renderer->SetColor (lines);

	VirtualDisplay::SetFont(OTWDriver.pCockpitManager->KneeFont());

	// Display the players call sign and assignment
	char string[1024];
	if (mode == STEERPOINT) { // JPO new kneeboard page
	    v = 0.95 - LINE_HEIGHT;
	    if (GetBriefingData(GBD_PACKAGE_STPTHDR, 0, string, sizeof(string)) != -1) {
		renderer->TextLeft(-0.95f, v, string);
		v -= LINE_HEIGHT;
	    }
	    for (lines = 0; GetBriefingData(GBD_PACKAGE_STPT, lines, string, sizeof(string)) != -1; lines ++) {
		renderer->TextLeft(-0.95f, v, string);
		v -= LINE_HEIGHT;
	    }

		//MI display GPS coords when on ground, for INS alignment stuff
		if(g_bRealisticAvionics && g_bINS)
		{
			v -= 2 * LINE_HEIGHT;
			if(((AircraftClass*)SimDriver.playerEntity) && ((AircraftClass*)SimDriver.playerEntity)->OnGround())
			{
				char latStr[20] = "";
				char longStr[20] = "";
				char tempstr[10] = "";
				float latitude	= (FALCON_ORIGIN_LAT * FT_PER_DEGREE + cockpitFlightData.x) / EARTH_RADIUS_FT;
				float cosLatitude = (float)cos(latitude);
				float longitude	= ((FALCON_ORIGIN_LONG * DTR * EARTH_RADIUS_FT * cosLatitude) + cockpitFlightData.y) / (EARTH_RADIUS_FT * cosLatitude);
				
				latitude	*= RTD;
				longitude	*= RTD;
				
				int longDeg		= FloatToInt32(longitude);
				float longMin	= (float)fabs(longitude - longDeg) * DEG_TO_MIN;

				int latDeg		= FloatToInt32(latitude);
				float latMin	= (float)fabs(latitude - latDeg) * DEG_TO_MIN;

				// format lat/long here
				if(latMin < 10.0F) 
					sprintf(latStr, "LAT  N %3d\x03 0%2.2f\'\n", latDeg, latMin);
				else 
					sprintf(latStr, "LAT  N %3d\x03 %2.2f\'\n", latDeg, latMin);
				if(longMin < 10.0F) 
					sprintf(longStr, "LNG  E %3d\x03 0%2.2f\'\n", longDeg, longMin);
				else 
					sprintf(longStr, "LNG  E %3d\x03 %2.2f\'\n", longDeg, longMin);

				renderer->TextLeft(-0.95F, v, latStr);
				v -= LINE_HEIGHT;
				renderer->TextLeft(-0.95F, v, longStr);
				v -= LINE_HEIGHT;
				sprintf(tempstr, "SALT %dFT", (long)-cockpitFlightData.z);
				renderer->TextLeft(-0.95F, v, tempstr);
			}
		}
	}
	else {
	if (GetBriefingData( GBD_PLAYER_ELEMENT, 0, string, sizeof(string) ) != -1) {
		renderer->TextLeft( -0.9f, v, string );
		v -= LINE_HEIGHT;
	}

	if (GetBriefingData( GBD_PLAYER_TASK,    0, string, sizeof(string) ) != -1) {
		lines = renderer->TextWrap( -0.8f, v, string, LINE_HEIGHT, 1.7f );
		v -= (lines+1)*LINE_HEIGHT;
	}


	// Display the package info (if we are part of a package)
	if (GetBriefingData( GBD_PACKAGE_LABEL, 0, string, sizeof(string) ) != -1) {
		renderer->TextLeft( -0.9f, v, string );
		v -= LINE_HEIGHT;

		// Package mission statement
		if (GetBriefingData( GBD_PACKAGE_MISSION, 0, string, sizeof(string) ) != -1) {
			lines = renderer->TextWrap( -0.8f, v, string, LINE_HEIGHT, 1.7f );
			v -= lines*LINE_HEIGHT;
		}

		// List the flights in the package
		lines = 0;
		while (GetBriefingData( GBD_PACKAGE_ELEMENT_NAME, lines, string, sizeof(string) ) != -1) {

			renderer->TextLeft( -0.8f, v, string );
			if (GetBriefingData( GBD_PACKAGE_ELEMENT_TASK, lines, string, sizeof(string) ) != -1)
				renderer->TextLeft( -0.1f, v, string );
			lines ++;
			v -= LINE_HEIGHT;
		}
	}
	}
	}
	VirtualDisplay::SetFont(oldFont);
}


void KneeBoard::RenderMap( SimVehicleClass *platform )
{
	if (g_bVoodoo12Compatible) // JB 010330
		return;

	ShiAssert( platform );

	// Recompute what portion of the world map we need to draw
	UpdateMapDimensions( platform );

	// Copy the map image into the target buffer
	DrawMap();

#if 1
	// OW FIXME: the following StartFrame() call will result in a call to IDirect3DDevice7::SetRenderTarget. We can't do this on the Voodoo 1 & 2 ;(
	DeviceManager::DDDriverInfo *pDI = FalconDisplay.devmgr.GetDriver(DisplayOptions.DispVideoDriver);
	if(!pDI->SupportsSRT())
		return;
#endif

	// Draw in the waypoints
	StartFrame();
	DrawWaypoints( platform );
	FinishFrame();
}


void KneeBoard::UpdateMapDimensions( SimVehicleClass *platform )
{
	WayPointClass*	wp;
	float			x, y, z;
	float			left, right, top, bottom;

	ShiAssert( platform );
	ShiAssert(m_imageloaded);
	if (m_imageloaded == false) return;

	m_pixel2nmY = (TheMap.NorthEdge() - TheMap.SouthEdge()) * FT_TO_KM;
	m_pixel2nmX = (TheMap.EastEdge() - TheMap.WestEdge()) * FT_TO_KM;
	
	m_pixel2nmY /= m_imageFile.image.height;
	m_pixel2nmX /= m_imageFile.image.width;
	// Start with our current location
	top   = bottom  = platform->XPos();
	right = left    = platform->YPos();

	// Walk the waypoints and get min/max info
	for (wp=platform->waypoint; wp; wp=wp->GetNextWP()) {
		wp->GetLocation (&x, &y, &z);

		right	= max( right,	y );
		left	= min( left,	y );
		top		= max( top,		x );
		bottom	= min( bottom,	x );
	}

	// Add the position of the override waypoint (if any)
	ShiAssert( platform->GetCampaignObject() );
	ShiAssert( platform->GetCampaignObject()->IsFlight() );
	wp = ((FlightClass*)platform->GetCampaignObject())->GetOverrideWP();
	if (wp) {
		wp->GetLocation (&x, &y, &z);

		right	= max( right,	y );
		left	= min( left,	y );
		top		= max( top,		x );
		bottom	= min( bottom,	x );
	}

	// Now get the center of the map we want to display
	wsHcenter = (right+left) * 0.5f;
	wsVcenter = (top+bottom) * 0.5f;

	// Now figure out the minimum width and height we want (as a distance for center for now)
	wsHsize = (right - left) * 0.5f * (1.0f + BORDER_PERCENT);
	wsVsize = (top - bottom) * 0.5f * (1.0f + BORDER_PERCENT);
	if (wsHsize >= TheMap.EastEdge() - TheMap.WestEdge()) {
		wsHsize = TheMap.EastEdge() - TheMap.WestEdge() - 1.0f;		// -1 is for rounding safety...
	}
	if (wsVsize >= TheMap.NorthEdge() - TheMap.SouthEdge()) {
		wsVsize = TheMap.NorthEdge() - TheMap.SouthEdge() - 1.0f;	// -1 is for rounding safety...
	}

	// See how many source pixels we're talking about and round down to an even divisor of the dest pixels
	float hSourcePixels = wsHsize*2.0f*FT_TO_KM / m_pixel2nmX;
	float vSourcePixels = wsVsize*2.0f*FT_TO_KM / m_pixel2nmY;

	float hPixelMag = srcRect.right  / hSourcePixels;
	float vPixelMag = srcRect.bottom / vSourcePixels;

	// Cap the pixel magnification at a reasonable level
	float mapPixels		= (TheMap.NorthEdge()-TheMap.SouthEdge())*FT_TO_KM/m_pixel2nmY;
	float drawPixels	= (float)srcRect.bottom;
	float maxMag		= drawPixels / mapPixels * KNEEBOARD_SMALLEST_MAP_FRACTION;
	pixelMag = FloatToInt32( (float)floor( min( min( hPixelMag, vPixelMag), maxMag) ) );

	// Detect the case where the whole desired image won't fit on screen
	if (pixelMag < 1) {
		pixelMag = 1;

		// Recenter on the current position to ensure its visible
		wsHcenter = platform->YPos();
		wsVcenter = platform->XPos();
	}

	// Now readjust our world space dimensions to reflect what we'll actually draw
	wsHsize = 0.5f * srcRect.right  / (float)pixelMag * m_pixel2nmX * KM_TO_FT;
	wsVsize = 0.5f * srcRect.bottom / (float)pixelMag * m_pixel2nmY * KM_TO_FT;

	// Finally shift the center point as necessary to ensure we won't try to draw off the edge
	if (wsHcenter-wsHsize <= TheMap.WestEdge()) {
		wsHcenter = TheMap.WestEdge() + wsHsize + 0.5f;		// +1/2 is for rounding safety...
	}
	if (wsHcenter+wsHsize >= TheMap.EastEdge()) {
		wsHcenter = TheMap.EastEdge() - wsHsize - 0.5f;		// -1/2 is for rounding safety...
	}
	if (wsVcenter-wsVsize <= TheMap.SouthEdge()) {
		wsVcenter = TheMap.SouthEdge() + wsVsize + 0.5f;	// +1/2 is for rounding safety...
	}
	if (wsVcenter+wsVsize >= TheMap.NorthEdge()) {
		wsVcenter = TheMap.NorthEdge() - wsVsize - 0.5f;	// -1/2 is for rounding safety...
	}
}

// JPO - load the actual image, try the campaign dir first.
void KneeBoard::LoadKneeImage(CImageFileMemory &imageFile)
{
    char pathname[MAX_PATH];
    int result;

    sprintf (pathname, "%s\\%s", FalconCampaignSaveDirectory, THR_KNEEBOARD_MAP_NAME);
    // Make sure we recognize this file type
    imageFile.imageType = CheckImageType( pathname );
    ShiAssert( imageFile.imageType != IMAGE_TYPE_UNKNOWN );
    
    // Open the input file
    result = imageFile.glOpenFileMem( pathname );
    if (result != 1) {
	imageFile.imageType = CheckImageType( KNEEBOARD_MAP_NAME );
	ShiAssert( imageFile.imageType != IMAGE_TYPE_UNKNOWN );
	
	// Open the input file
	result = imageFile.glOpenFileMem( KNEEBOARD_MAP_NAME );
    }
    ShiAssert( result == 1 );
    
    // Read the image data (note that ReadTextureImage will close texFile for us)
    imageFile.glReadFileMem();
    result = ReadTextureImage( &imageFile );
    if (result != GOOD_READ) {
	ShiError( "Failed to read kneeboard image." );
    }
}


void KneeBoard::DrawMap( void )
{
	ShiAssert(m_imageloaded);
	if (m_imageloaded == false) return;
	switch(m_pMapImage->PixelSize())
	{
		case 2:
		{
//			CImageFileMemory 	imageFile;
			int					w, h;
			UInt16				palette[256];
			UInt32				*p;
			UInt8				*src;
			UInt16				*dst;
			UInt16				*ptr;
			int					srcRowOffset;
			int					srcColOffset;
			int					dstRow, dstCol;
			DWORD				lighting;
			DWORD				inColor;
			DWORD				mask;
			DWORD				outColor;


#if 0 // JPO moved out to per class
			/* 
			 * Read the map source file 
			 */
			LoadKneeImage(imageFile);
#endif

			// Copy the width and height for convienience
			w = m_imageFile.image.width;
			h = m_imageFile.image.height;


			/* 
			 * Convert the palette from 8 bit to screen format and light it
			 */
			
			// Decide what lighting to apply
			if (TheTimeOfDay.GetNVGmode()) {
				// Convert from floating point to a 16.16 fixed point representation
				lighting = FloatToInt32(NVG_LIGHT_LEVEL * 65536.0f);

				// Use only green
				mask = 0xFF00FF00;
			} else { 
				// Convert from floating point to a 16.16 fixed point representation
				lighting = FloatToInt32(TheTimeOfDay.GetLightLevel() * 65536.0f);

				// Use full color
				mask = 0xFFFFFFFF;
			}

			// Light the palette entries and convert to 16 bit colors
			p = m_imageFile.image.palette;
			for (dst = palette; dst < &palette[256]; dst++) {
				inColor = *p++;

				outColor  = ((((inColor)     & 0xFF) * lighting) >> 16);
				outColor |= ((((inColor>>8)  & 0xFF) * lighting) >> 8) & 0x0000FF00;
				outColor |= ((((inColor>>16) & 0xFF) * lighting))      & 0x00FF0000;

				*dst = m_pMapImage->Pixel32toPixel16( outColor & mask );
			}
//			glReleaseMemory( (char*)imageFile.image.palette );


			/* 
			 * Copy the appropriate portion of the map into the kneeboard image 
			 */

			// Decide where to start in the source image
			float rowrange = TheMap.NorthEdge() - (wsVcenter + wsVsize);
			rowrange *= FT_TO_KM;
			srcRowOffset	= FloatToInt32((TheMap.NorthEdge() - (wsVcenter + wsVsize)) * FT_TO_KM / m_pixel2nmY);
			srcColOffset	= FloatToInt32((wsHcenter - wsHsize)                        * FT_TO_KM / m_pixel2nmX);
			ShiAssert(srcRowOffset >= 0 && srcRowOffset < h);
			ShiAssert(srcColOffset >= 0 && srcColOffset < w);
			// Lock the target surface
			ptr = (UInt16*)m_pMapImage->Lock();

			// Copy the rectangular sub-region
			for (dstRow = 0; dstRow < srcRect.bottom; dstRow++) {
				int srcRow = srcRowOffset + dstRow/pixelMag;

				dst = (UInt16*)m_pMapImage->Pixel( ptr, dstRow, 0 );
				ShiAssert(srcRow*w + srcColOffset < w * h);
				src = m_imageFile.image.image + srcRow*w + srcColOffset;

				for (dstCol = 0; dstCol < srcRect.right; dstCol++) {
					*dst++ = palette[*src];
					if (dstCol%pixelMag == pixelMag-1) {
						src++;
					}
				}
			}

			// Release the source image and unlock the target surface
//			glReleaseMemory( (char*)imageFile.image.image );
			m_pMapImage->Unlock();

			break;
		}

		case 4:
		{
	//		CImageFileMemory 	imageFile;
			int					w, h;
			DWORD				palette[256];
			UInt32				*p;
			UInt8				*src;
			DWORD				*dst;
			DWORD				*ptr;
			int					srcRowOffset;
			int					srcColOffset;
			int					dstRow, dstCol;
			DWORD				lighting;
			DWORD				inColor;
			DWORD				mask;
			DWORD				outColor;


			/* 
			 * Read the map source file 
			 */

			// Make sure we recognize this file type
//			LoadKneeImage(imageFile);

			// Copy the width and height for convienience
			w = m_imageFile.image.width;
			h = m_imageFile.image.height;


			/* 
			 * Convert the palette from 8 bit to screen format and light it
			 */
			
			// Decide what lighting to apply
			if (TheTimeOfDay.GetNVGmode()) {
				// Convert from floating point to a 16.16 fixed point representation
				lighting = FloatToInt32(NVG_LIGHT_LEVEL * 65536.0f);

				// Use only green
				mask = 0xFF00FF00;
			} else { 
				// Convert from floating point to a 16.16 fixed point representation
				lighting = FloatToInt32(TheTimeOfDay.GetLightLevel() * 65536.0f);

				// Use full color
				mask = 0xFFFFFFFF;
			}

			// Light the palette entries and convert to 16 bit colors
			p = m_imageFile.image.palette;
			for (dst = palette; dst < &palette[256]; dst++) {
				inColor = *p++;

				outColor  = ((((inColor)     & 0xFF) * lighting) >> 16);
				outColor |= ((((inColor>>8)  & 0xFF) * lighting) >> 8) & 0x0000FF00;
				outColor |= ((((inColor>>16) & 0xFF) * lighting))      & 0x00FF0000;

				*dst = m_pMapImage->Pixel32toPixel32( outColor & mask );
			}
//			glReleaseMemory( (char*)imageFile.image.palette );


			/* 
			 * Copy the appropriate portion of the map into the kneeboard image 
			 */

			// Decide where to start in the source image
			srcRowOffset	= FloatToInt32((TheMap.NorthEdge() - (wsVcenter + wsVsize)) * FT_TO_KM / m_pixel2nmX);
			srcColOffset	= FloatToInt32((wsHcenter - wsHsize)                        * FT_TO_KM / m_pixel2nmY);

			// Lock the target surface
			ptr = (DWORD*)m_pMapImage->Lock();

			// Copy the rectangular sub-region
			for (dstRow = 0; dstRow < srcRect.bottom; dstRow++) {
				int srcRow = srcRowOffset + dstRow/pixelMag;

				dst = (DWORD*)m_pMapImage->Pixel( ptr, dstRow, 0 );
				src = m_imageFile.image.image + srcRow*w + srcColOffset;

				for (dstCol = 0; dstCol < srcRect.right; dstCol++) {
					*dst++ = palette[*src];
					if (dstCol%pixelMag == pixelMag-1) {
						src++;
					}
				}
			}

			// Release the source image and unlock the target surface
//			glReleaseMemory( (char*)imageFile.image.image );
			m_pMapImage->Unlock();

			break;
		}
	}
}


void KneeBoard::DrawWaypoints( SimVehicleClass *platform )
{
	WayPointClass*	wp=NULL;
	BOOL			isFirst=TRUE;
	float			x1=0.0F, y1=0.0F, x2=0.0F, y2=0.0F;


	SetColor( WP_COLOR );

	// Draw in the waypoints
	for (wp=platform->waypoint; wp; wp=wp->GetNextWP()) {

		// Get the display coordinates of this waypoint
		MapWaypointToDisplay(wp, &x1, &y1);
		
		// Draw the waypoint marker and the connecting line if this isn't the first one
		Circle( x1, y1, WP_SIZE );
		if (!isFirst) {
			Line( x1, y1, x2, y2 );
		}

		// Step to the next waypoint
		x2	= x1;
		y2	= y1;
		isFirst = FALSE;
	}

	// Draw the override waypoint marker (if any)
	ShiAssert( platform->GetCampaignObject() );
	ShiAssert( platform->GetCampaignObject()->IsFlight() );
	wp = ((FlightClass*)platform->GetCampaignObject())->GetOverrideWP();
	if (wp) {

		// Get the display coordinates of this waypoint
		MapWaypointToDisplay(wp, &x1, &y1);
		x2 = x1+ORIDE_WP_SIZE;
		y2 = y1+ORIDE_WP_SIZE;
		x1 = x1-ORIDE_WP_SIZE;
		y1 = y1-ORIDE_WP_SIZE;
		
		// Draw the waypoint marker
		Line( x1, y1, x2, y1 );
		Line( x2, y1, x2, y2 );
		Line( x2, y2, x1, y2 );
		Line( x1, y2, x1, y1 );
	}
}


void KneeBoard::DrawCurrentPosition( ImageBuffer *targetBuffer, Render2D *renderer, SimVehicleClass *platform )
{
	if (g_bVoodoo12Compatible) // JB 010330
		return;

	const float	aspect = (float)srcRect.right/(float)srcRect.bottom;
	float		h, v;
	mlTrig		trig;
	static const struct {
		float	x,	y;
	} pos[] = {
		 0.0f,   1.0f,			// nose
		 0.0f,  -1.0f,			// tail
		-1.0f,  -0.4f,			// left wing tip
		 1.0f,  -0.4f,			// right wing tip
		 0.0f,   0.3f,			// leading edge at fuselage
		-0.5f,  -1.0f,			// left stab
		-0.5f,  -1.0f,			// right stab
	};
	static const int	numPoints = sizeof(pos)/sizeof(pos[0]);
	float	x[numPoints];
	float	y[numPoints];

	// Convert our position into display space within the destination rect
	v = (platform->XPos() - wsVcenter) / wsVsize;
	h = (platform->YPos() - wsHcenter) / wsHsize;
	if (fabs(v) > 0.95f) {
		if (v > 0.95f) {
			v =  0.95f;
		} else {
			v = -0.95f;
		}

		if (vuxRealTime & 0x200) {
			// Don't draw to implement a flashing icon when the real postion is off screen.
			return;
		}
	}
	if (fabs(h) > 0.95f) {
		if (h > 0.95f) {
			h =  0.95f;
		} else {
			h = -0.95f;
		}

		if (vuxRealTime & 0x200) {
			// Don't draw to implement a flashing icon when the real postion is off screen.
			return;
		}
	}

	// Setup local coordinates for the symbol drawing
	renderer->CenterOriginInViewport();
	renderer->AdjustOriginInViewport( h, v );

	// Transform all our verts
	mlSinCos( &trig, platform->Yaw() );
	trig.sin *= AC_SIZE;
	trig.cos *= AC_SIZE;
	for (int i=numPoints-1; i>=0; i--) {
		x[i] = ( pos[i].x * trig.cos + pos[i].y * trig.sin);
		y[i] = (-pos[i].x * trig.sin + pos[i].y * trig.cos) * aspect;
	}

	// Draw the aircraft symbol
	renderer->SetColor( AC_COLOR );
	renderer->Line( x[0], y[0], x[1], y[1] );				// Body
	renderer->Line( x[5], y[5], x[6], y[6] );				// Tail
	renderer->Tri(  x[2], y[2], x[3], y[3], x[4], y[4] );	// Wing

	// Draw a ring around the aircraft symbol to highlight it
	renderer->SetColor( 0xFF00FF00 );
	renderer->Circle( 0.0f, 0.0f, 1.5f*AC_SIZE );

	// Clear the local coordinates  (I guess we're not expected to do this, so don't bother...)
//	renderer->CenterOriginInViewport();
//	renderer->SetViewport(-1.0f, 1.0f, 1.0f, -1.0f);
}


void KneeBoard::MapWaypointToDisplay( WayPointClass *pwaypoint, float *h, float *v )
{
	float wpX;
	float wpY;
	float wpZ;

	pwaypoint->GetLocation (&wpX, &wpY, &wpZ);

	// Return values are in screen space, while the waypoint location is in falcon (X North, Y East)
	*v = (wpX - wsVcenter) / wsVsize;
	*h = (wpY - wsHcenter) / wsHsize;
}

