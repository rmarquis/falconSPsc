/***************************************************************************\
    WXMap.h
    Scott Randolph
    July 18, 1996

	Manage the weather database (cloud and precipitation locations).
\***************************************************************************/
#ifndef _WXMAP_H_
#define _WXMAP_H_

#include <math.h>
#include "GRTypes.h"


// The global weather database used by everyone
extern class WeatherMap *TheWeather;

// Some handy constants which bound the weather environment
extern float				SKY_ROOF_HEIGHT;
extern float				SKY_ROOF_RANGE;
extern float				SKY_MAX_HEIGHT;


// Conversion factor from BYTE base elevation to world space elevation (Z down)
static const float WxAltScale		= -50.0f;
static const float WxThickScale		= -250.0f;

#define LAST_CLEAR_TYPE		2
#define FIRST_OVC_TYPE		4
#define MAX_CLOUD_TYPE		80		// Maximum value of cloud type
#define	CLOUD_CELL_SIZE		8.0f	// How big is a cloud tile (in KM)


typedef struct CellState {
	BYTE	BaseAltitude;	// hundreds of feet MSL
	BYTE	Type;			// cloud tile number (or thickness for OVC)
} CellState;


class WeatherMap {
  public:
	WeatherMap( void );
	~WeatherMap( void );

  public:
	void Setup( void );
	void Cleanup( void );
	int Load(char *filename, int type);

	DWORD			TypeAt( UINT r, UINT c ){ if ((r>=h) || (c >=w)) return 0; 
											  else return map[r*w+c].Type; };
	float			BaseAt( UINT r, UINT c ){ if ((r>=h) || (c >=w)) return 1.0f-SKY_ROOF_HEIGHT;
											  return map[r*w+c].BaseAltitude * WxAltScale + WxAltShift; };
	float			ThicknessAt( UINT r, UINT c ){ if ((r>=h) || (c >=w)) return 1.0f;
											  return (map[r*w+c].Type-FIRST_OVC_TYPE) * WxThickScale; };
	float			TopsAt( UINT r, UINT c ){ return BaseAt(r,c) + ThicknessAt(r,c); };

	float	CellSize( void )				{ return cellSize; };
	int		WorldToTile( float distance )	{ return FloatToInt32((float)floor(distance/cellSize)); };
	float	TileToWorld( int rowORcol )		{ return (rowORcol * cellSize); };
	virtual float TemperatureAt(const Tpoint *pos)	{ return 20.0f;};
	float RainAt(UINT r, UINT c);
	float VisRangeAt(UINT r, UINT c);
	bool  LightningAt(UINT r, UINT c);


  protected:
	static float WxAltShift;	// Added to cloud heights to keep them above the terrain
	CellState	*map;
	UINT		w;
	UINT		h;

	float		cellSize;

	int			mapType;		// User defined

  public:
	int			rowShiftHistory;
	int			colShiftHistory;

	float		xOffset;
	float		yOffset;
};

#endif // _WXMAP_H_