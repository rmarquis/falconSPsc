/***************************************************************************\
    WXMap.cpp
    Scott Randolph
    July 18, 1996

	Manage the weather database (cloud and precipitation locations).
\***************************************************************************/
#include <stdio.h>
#include "grTypes.h"
#include "TMap.h"
#include "WXMap.h"
#include "Falclib\Include\openfile.h"
#include "Tod.h"

// The one and only weather map pointer (must be constructed by the app elsewhere!)
WeatherMap	*TheWeather;

float	SKY_ROOF_HEIGHT			= 30000.0f;
float	SKY_ROOF_RANGE			= 200000.0f;
float	SKY_MAX_HEIGHT			= 70000.0f;

extern float g_fMinCloudWeather;
extern float g_fCloudThicknessFactor;

WeatherMap::WeatherMap()
{
	map			= NULL;
	w			= h = 0;
	cellSize	= 0.0F;
	mapType		= 0;
}


WeatherMap::~WeatherMap()
{
	if (map) Cleanup(); 
}


//
//	Initialize data
//
void WeatherMap::Setup( void )
{
	// Make sure someone has created the shared Weather Map object before we get here
	ShiAssert( this );  // "TheWeather" must be constructed by the application before we get here.
	
	// Make sure we're clean before doing this
	Cleanup();

	// Record the size of a weather cell
	cellSize = CLOUD_CELL_SIZE * FEET_PER_KM;

	// Store the weather map's width and height
	w = FloatToInt32( (float)floor(((TheMap.EastEdge()  - TheMap.WestEdge())  / cellSize) + 0.5f) );
	h = FloatToInt32( (float)floor(((TheMap.NorthEdge() - TheMap.SouthEdge()) / cellSize) + 0.5f) );

	// Start with no cloud offset (might want to save/restore this)
	xOffset = 0.0f;
	yOffset = 0.0f;

	// Start with no recorded shift history
	rowShiftHistory = 0;
	colShiftHistory = 0;

	// Allocate memory for the weather map.
	map = new CellState[ w*h ];
	if (!map) {
		ShiError( "Failed to allocate memory for the weather map." );
	}

	// Intialize the map to "clear"
	for (DWORD k = 0; k < w*h; k++) {
		map[k].Type = 0;
		map[k].BaseAltitude = 0xFF;
	}

	mapType = 0;		// Reset the map type, since we cleared the map.
}

int WeatherMap::Load( char *filename, int type )
{
	HANDLE		mapFile;
	int			result;
	unsigned	nw,nh;
	DWORD		bytesRead;
	float		ncs;

	if (mapType && type == mapType) {
		return 1;							// Map already loaded
	}

	ncs = CLOUD_CELL_SIZE * FEET_PER_KM;
	nw = FloatToInt32( (float)floor(((TheMap.EastEdge()  - TheMap.WestEdge())  / cellSize) + 0.5f) );
	nh = FloatToInt32( (float)floor(((TheMap.NorthEdge() - TheMap.SouthEdge()) / cellSize) + 0.5f) );
	if (nw != w || nh != h || ncs != cellSize) {
		Setup();			// Redo the setup stuff if stuff has changed
	}

	// Start with no cloud offset (might want to save/restore this)
	xOffset = 0.0f;
	yOffset = 0.0f;

	// Open the specified file for reading
	mapFile = CreateFile_Open( filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if (mapFile == INVALID_HANDLE_VALUE) {
		//char	string[80];
		//char	message[120];
		//PutErrorString( string );
		//sprintf( message, "%s:  Couldn't open weather map.", string );
		//MessageBox( NULL, message, "Proceeding without intial weather", MB_OK );
		//result = FALSE;
		// We can tolerate no weather
	} else {
		// Read the weather map file.
		result = ReadFile( mapFile, map, w*h*sizeof(*map), &bytesRead, NULL );
		if ((!result) || (bytesRead != w*h*sizeof(*map))) {
			char	string[80];
			char	message[120];
			PutErrorString( string );
			sprintf( message, "%s:  Couldn'd read weather map.", string );
			MessageBox( NULL, message, "Proceeding without intial weather", MB_OK );
			result = FALSE;
		}

		// Close the weather map file
		CloseHandle( mapFile );
	}

	mapType = type;

	return 1;
}

//
//	Clean up after ourselves and shut down
//
void WeatherMap::Cleanup( void )
{
	// Release our weather map memory
	delete[] map;
	map = NULL;
}

float WeatherMap::RainAt(UINT r, UINT c)
{
    float thickness = ThicknessAt(r, c);
    float rainFactor = 0;
    if (fabs(thickness) > g_fMinCloudWeather) { // if the clouds are thick, then its raining.
	rainFactor = ((float)fabs(thickness) - g_fMinCloudWeather)/g_fCloudThicknessFactor;
	if (rainFactor > 1) rainFactor = 1;
    }
    return rainFactor;
}

bool WeatherMap::LightningAt(UINT r, UINT c)
{
    float thickness = ThicknessAt(r, c);
    float rainFactor = 0;
    if (fabs(thickness) > g_fMinCloudWeather) { // if the clouds are very thick, then its thunderstorms.
	rainFactor = ((float)fabs(thickness) - g_fMinCloudWeather)/g_fCloudThicknessFactor;
	if (rainFactor > 1) return true;
    }
    return false;
}

float WeatherMap::VisRangeAt(UINT r, UINT c)
{
    float vr  = RainAt(r, c);
    vr = 1 - vr;
    return max(vr, TheTimeOfDay.GetMinVisibility());
}
