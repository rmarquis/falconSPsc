/***************************************************************************\
    WXCell.cpp
    Scott Randolph
    July 19, 1996

	Base class for a cloud tile.
\***************************************************************************/
#include "WXMap.h"
#include "WXCell.h"

extern float g_fMinCloudWeather;
extern unsigned long    vuxRealTime;

void WeatherCell::Setup( int r, int c, ObjectDisplayList* objList )
{
	DWORD	code;
	float	thickness;

	ShiAssert( objList );
	lightning = false;
	rainFactor = 0;
	visDistance = 1;
	ltimer = 0;

	// Construct the overcast tiling code
	code = 0;
	if ( TheWeather->TypeAt( r,   c   ) >= FIRST_OVC_TYPE)  code  = 1;
	if ( TheWeather->TypeAt( r,   c+1 ) >= FIRST_OVC_TYPE)  code |= 2;
	if ( TheWeather->TypeAt( r+1, c   ) >= FIRST_OVC_TYPE)  code |= 4;
	if ( TheWeather->TypeAt( r+1, c+1 ) >= FIRST_OVC_TYPE)  code |= 8;

	// If we're not overcast, see if we should do a scattered tile
	if (!code) {
		if ((TheWeather->TypeAt( r-1, c   ) >= FIRST_OVC_TYPE)  ||
			(TheWeather->TypeAt( r,   c-1 ) >= FIRST_OVC_TYPE)  ||
			(TheWeather->TypeAt( r+2, c+1 ) >= FIRST_OVC_TYPE)  ||
			(TheWeather->TypeAt( r+1, c+2 ) >= FIRST_OVC_TYPE)) {
			// We want to draw a border scud tile
			code  = 16 + rand()%4;
		}
	}

	// Decide which type of cloud cell to create (if any)
	if (!code) {
		// Clear sky
		thickness = 0.0f;
		drawable = NULL;
	}
	else 
	{
		// Get our thickness
		thickness = TheWeather->ThicknessAt( r, c );

		// Create the drawable overcast object
		drawable = new DrawableOvercast( code, r, c );
		if (!drawable) {
			ShiError( "Failed to create the overcast drawable object" );
		}

		// Add the drawable object to the object manager's list
		objList->InsertObject( drawable );
		rainFactor = TheWeather->RainAt(r, c);
		lightning = TheWeather->LightningAt(r, c);
		visDistance = TheWeather->VisRangeAt(r, c);
		if (lightning) {
		    ltimer = vuxRealTime + (rand()%30)*1000;
		}
	}


	// Store our top and bottom z values
	base	= TheWeather->BaseAt( r, c );
//	top		= base -= thickness;				// -Z is up!
	if (thickness <= 0)
	    top = base;
	else
	    top	= TheWeather->TopsAt(r, c); // JPO - think this is the same as the cloud drawing routine.
	// it seems to give better results anyway.

}


void WeatherCell::Cleanup( ObjectDisplayList* objList )
{
	ShiAssert( objList );

	if (drawable) {
		// Remove the visual object for the object manager list
		objList->RemoveObject( drawable );
		delete drawable;
		drawable = NULL;
	}
}

bool WeatherCell::GetLightning()
{
    if (lightning == true &&
	ltimer < vuxRealTime) {
	ltimer = vuxRealTime + (rand()%30)*1000;
	return true;
    }
    return false;
}