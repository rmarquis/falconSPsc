/***************************************************************************\
    LocalWX.h
    Scott Randolph
    July 19, 1996

	Manage the "deaggregated" clouds in a given local.
\***************************************************************************/
#ifndef _LOCALWX_H_
#define _LOCALWX_H_

#include "grTypes.h"
#include "WXMap.h"
#include "WXCell.h"


class LocalWeather {
  public:
	LocalWeather( void )	{ cellArray = NULL; };        
	~LocalWeather( void )	{ if (cellArray) Cleanup(); };

	void Setup( float visRange, ObjectDisplayList* objList );
	void Cleanup( void );
	void Update( Tpoint *position, DWORD currentTime );

	float	GetAreaFloor( void )		{ return cloudBase; };
	float	GetAreaCeiling( void )	{ return cloudTops; };

	float	LineOfSight( Tpoint *p1, Tpoint *p2 );

	float	GetMaxRange( void )		{ return range; };
	
	float	GetLocalCloudTops(void);
	float GetRainFactor ();
	float GetVisibility ();
	bool  HasLightning();

  protected:
	void UpdateForDrift( void );
	void SlideLocalCellsVertical( int vx );
	void SlideLocalCellsHorizontal( int vy );
	void RebuildList( void );
	void ComputeAreaFloorAndCeiling( void );

	BOOL horizontalEdgeTest( int row, int col, float x, float y, float z, BOOL downward );
	BOOL verticalEdgeTest( int row, int col, float x, float y, float z, BOOL downward );
	float LineSquareIntersection( int row, int col, float x1, float y1, float z1, float dx, float dy, float dz, BOOL downward );
	WeatherCell	*GetLocalCell( int r, int c )	{	if (abs(r)>cellRange)  return NULL; 
													if (abs(c)>cellRange)  return NULL; 
													return cellArray + (r+cellRange)*rowLen + c+cellRange; };
  protected:
	ObjectDisplayList* objMgr;	// Object list which will sort and draw cloud objects
	 
	float	centerX, centerY;	// What is our current center point	(world space)
	int		cellRow, cellCol;	// What is our current center point	(cell coordinates)

	int		rowShiftHistory;	// Checked against TheWeather to detect map shifts
	int		colShiftHistory;	// Checked against TheWeather to detect map shifts

	float	range;				// How far away can clouds be seen
	int		cellRange;			// How many weather cells into the distance do we need
	int		rowLen;				// How many cells accross (and high) is the cell array

	float	cloudBase;			// Z value at base of local clouds (-Z is up)
	float	cloudTops;			// Z value at tops of local clouds (-Z is up)

	WeatherCell *cellArray;		// ROWxCOL array of local cells
};

#endif // _LOCALWX_H_