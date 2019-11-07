/***************************************************************************\
    KneeBoard.h
    Scott Randolph
    September 4, 1998

    This class provides the pilots knee board notepad and map.
\***************************************************************************/
#ifndef _KNEEBOARD_H_
#define _KNEEBOARD_H_

#include "Graphics\Include\Render2d.h"
#include "Graphics\Include\image.h"

class SimVehicleClass;
class WayPointClass;

class CImageFileMemory;
class KneeBoard : public Render2D {
  public:
	KneeBoard();
	virtual ~KneeBoard();

	virtual void Setup( class DisplayDevice *device, int top, int left, int bottom, int right );
	virtual void Cleanup( void );

	enum Page {BRIEF, MAP, STEERPOINT};

	void SetPage( Page m )	{ mode = m; };
	Page GetPage( void )	{ return mode; };

	void Refresh( SimVehicleClass *platform );
	void DisplayBlit( ImageBuffer *imageBuffer, Render2D *renderer, class SimVehicleClass *platform );
	void DisplayDraw( ImageBuffer *imageBuffer, Render2D *renderer, class SimVehicleClass *platform );

  private:
	  void LoadKneeImage(CImageFileMemory &imagefile);
	// Which page we're displaying
	Page		mode;

	// Map image surface and Blt rectangles
	ImageBuffer	*m_pMapImage;
	RECT		srcRect;
	RECT		dstRect;

	// Real world map dimensions
	float		wsVcenter;
	float		wsHcenter;
	float		wsHsize;
	float		wsVsize;
	int			pixelMag;
	bool m_imageloaded;
	CImageFileMemory 	m_imageFile;
	float m_pixel2nmX, m_pixel2nmY;

	// Internal worker functions for the text page
	void DrawMissionText( Render2D *renderer, SimVehicleClass *platform );

	// Internal worker funtions for the map page
	void RenderMap( SimVehicleClass *platform );
	void UpdateMapDimensions( SimVehicleClass *platform );
	void DrawMap( void );
	void DrawWaypoints( SimVehicleClass *platform );
	void DrawCurrentPosition( ImageBuffer *targetBuffer, Render2D *renderer, SimVehicleClass *platform );
	void MapWaypointToDisplay( WayPointClass *curWaypoint, float *x, float *y );
};

#endif // _KNEEBOARD_H_
