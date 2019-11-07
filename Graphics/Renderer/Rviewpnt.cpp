/***************************************************************************\
    RViewPnt.cpp
    Scott Randolph
    August 20, 1996

    Manages information about a world space location and keeps the weather,
	terrain, and object lists in synch.
\***************************************************************************/
#include "grmath.h"
#include "TimeMgr.h"
#include "TOD.h"
#include "LocalWX.h"
#include "DrawOvc.h"
#include "RViewPnt.h"
#include "mpr_light.h"

//#define _OLD_UPDATE_ // use scotts Update() funtion

/***************************************************************************\
	Setup the view point
\***************************************************************************/
void RViewPoint::Setup( float gndRange, int maxDetail, int minDetail, float wxRange ) 
{
	int i;

	ShiAssert( !IsReady() );
	
	// Intialize our sun and moon textures
	SetupTextures();
	
	// Determine how many object lists we'll need
	nObjectLists = _NUM_OBJECT_LISTS_;		// 0=in terrain, 1=below cloud, 2=in cloud, 3=above clouds, 4= above roof
	
	// Allocate memory for the list of altitude segregated object lists
	objectLists = new ObjectListRecord[nObjectLists];
	ShiAssert( objectLists );

	// Intialize each display list -- update will set the top and base values
	for (i=0; i<nObjectLists; i++) {
		objectLists[i].displayList.Setup();
		objectLists[i].Ztop  = -1e13f;		// -Z is up
	}

	// Initialize the cloud display list
	cloudList.Setup();

	// Setup our base class's terrain information
	float *Ranges = new float[minDetail+1];
	ShiAssert( Ranges );
	for (i=minDetail; i>=0; i--) {
		// Store this detail levels active range
		if (i==minDetail) {
  			// Account for only drawing out to .707 of the lowest detail level
			Ranges[i] = gndRange * 1.414f;
		} else {
			Ranges[i] = gndRange;
		}

		// Make each cover half the distance of the one before
		gndRange /= 2.0f;
	}
	TViewPoint::Setup( maxDetail, minDetail, Ranges );
	delete[] Ranges;
	
	// Construct the weather manager (if a weather range was given)
	SetWeatherRange( wxRange );
}


/***************************************************************************\
	Clean up after ourselves
\***************************************************************************/
void RViewPoint::Cleanup( void )
{
	int i;
	
	ShiAssert( IsReady() );

	// Release our sun and moon textures
	ReleaseTextures();

	// Cleanup all the display lists
	for (i=0; i<nObjectLists; i++) {
		objectLists[i].displayList.Cleanup();
	}

	// Release the list managment memory
	delete[] objectLists;
	objectLists = NULL;

	// Cleanup the weather manager
	if (weather) {
		weather->Cleanup();
		delete weather;
		weather = NULL;
	}

	// Cleanup our bases class's terrain manager
	TViewPoint::Cleanup();
}



/***************************************************************************\
	Change the visible range and detail levels of the terrain
	NOTE:  This shouldn't be called until an update has been done so 
	that the position is valid.
\***************************************************************************/
void RViewPoint::SetGroundRange( float gndRange, int maxDetail, int minDetail )
{
	TViewPoint	tempViewPoint;
	int			i;
	
	ShiAssert( IsReady() );

	// Calculate the ranges we'll need at each LOD
	float *Ranges = new float[minDetail+1];
	ShiAssert( Ranges );
	for (i=minDetail; i>=0; i--) {
		Ranges[i] = gndRange;
		gndRange /= 2.0f;
	}
	delete[] Ranges;

	// Construct a temporary viewpoint with our new parameters and current position
	tempViewPoint.Setup( maxDetail, minDetail, Ranges );
	tempViewPoint.Update( &pos );

	// Reset and update our base class's terrain information
	TViewPoint::Cleanup();
	TViewPoint::Setup( maxDetail, minDetail, Ranges );
	TViewPoint::Update( &pos );

	// Cleanup the temporary viewpoint
	tempViewPoint.Cleanup();
}


/***************************************************************************\
	Change the visible range of the weather
\***************************************************************************/
void RViewPoint::SetWeatherRange( float wxRange )
{
	// Delete the old weather manager if any
	delete weather;
	
	if (wxRange > 1.0f) {
		weather	= new LocalWeather;
		ShiAssert( weather );
		weather->Setup( wxRange, &cloudList );
	} else {
		weather = NULL;
		cloudBase	= -(SKY_ROOF_HEIGHT-0.1f);
		cloudTops	= -SKY_ROOF_HEIGHT;
		roofHeight	= -SKY_ROOF_HEIGHT;
	}
}

#ifdef _OLD_UPDATE_
/***************************************************************************\
	Update the state of the RViewPoint (time and location)
\***************************************************************************/
void RViewPoint::Update( Tpoint *pos )
{
	int				i;
	float			previousTop;
	DrawableObject	*lowList;
	DrawableObject	*highList;
	DrawableObject	*p;
	
	ShiAssert( IsReady() );

	// Update the terrain center of attention
	TViewPoint::Update( pos );
	TViewPoint::GetAreaFloorAndCeiling( &terrainFloor, &terrainCeiling );

	// Update the weather
	if (weather) {
		weather->Update( pos, TheTimeManager.GetClockTime() );
		cloudBase = weather->GetAreaFloor();
		cloudTops = weather->GetAreaCeiling();
	}
	roofHeight = -SKY_ROOF_HEIGHT;

	// For now we fix this here.  We might get smarter later...
	// (This comes into play when there is no data available for a weather tile)
	// Don't forget that -Z is up!
//	if (cloudTops <= roofHeight)  cloudTops = roofHeight + 0.1f;

	// Update the ceiling values of the object display lists
	objectLists[0].Ztop = terrainCeiling;
	objectLists[1].Ztop = cloudBase;
	objectLists[2].Ztop = cloudTops;
	objectLists[3].Ztop = roofHeight;
	// objectLists[4].Ztop is always "infinity"
	ShiAssert( terrainCeiling > cloudBase );	// -Z is down
	ShiAssert( cloudBase >= cloudTops );		// -Z is down


	// Update the membership of the altitude segregated object lists
	previousTop = 1e12f;		// -Z is up
	for (i=0; i<nObjectLists; i++) {
		lowList = NULL;
		highList = NULL;
		objectLists[i].displayList.UpdateMetrics( pos, 
												  previousTop, objectLists[i].Ztop, 
												  &lowList, &highList );
		previousTop = objectLists[i].Ztop;

		// Put all the objects which were too low for the list into the next lower one
		while (lowList) {
			p = lowList;
			lowList = lowList->next;
			objectLists[i-1].displayList.InsertObject( p );
		}

		// Put all the objects which were too high for the list into the next higher one
		while (highList) {
			p = highList;
			highList = highList->next;
			objectLists[i+1].displayList.InsertObject( p );
		}
	}

	// Resort each object list
	for (i=0; i<nObjectLists; i++) {
		objectLists[i].displayList.SortForViewpoint();
	}


	// Resort the cloud object list
	cloudList.UpdateMetrics( pos, 1e12f, -1e12f, &lowList, &highList );
	ShiAssert( (!lowList) && (!highList) );
	cloudList.SortForViewpoint();


	// Decide if we're affected by the nearest cloud
	// TODO:  Would we win elsewhere if we set a BOOL flag?
	DrawableOvercast *nearestCloud = (DrawableOvercast*)cloudList.GetNearest();
	if (nearestCloud) {
		ShiAssert( nearestCloud->GetClass() == DrawableObject::Overcast );
		cloudOpacity = nearestCloud->GetLocalOpacity( pos);
		nearestCloud->GetLocalColor( pos, &cloudColor );
	} else {
		cloudOpacity	= 0.0f;
	}
}

#else

float defcloudop = 0.0;
/***************************************************************************\
	Update the state of the RViewPoint (time and location)
\***************************************************************************/
void RViewPoint::Update( Tpoint *pos )
{
	int				i;
	DrawableObject	*first, *p;
	TransportStr	transList; // used to move DrawableObjects from one list to another
	float			previousTop;
	
	ShiAssert( IsReady() );

	// Update the terrain center of attention
	TViewPoint::Update( pos );
	TViewPoint::GetAreaFloorAndCeiling( &terrainFloor, &terrainCeiling );

	// Update the weather
	if (weather) {
		weather->Update( pos, TheTimeManager.GetClockTime() );
		cloudBase = weather->GetAreaFloor();
		cloudTops = weather->GetAreaCeiling();
	}
	roofHeight = -SKY_ROOF_HEIGHT;

	// For now we fix this here.  We might get smarter later...
	// (This comes into play when there is no data available for a weather tile)
	// Don't forget that -Z is up!
//	if (cloudTops <= roofHeight)  cloudTops = roofHeight + 0.1f;

	// Update the ceiling values of the object display lists
	objectLists[0].Ztop = terrainCeiling;
	objectLists[1].Ztop = cloudBase;
	objectLists[2].Ztop = cloudTops;
	objectLists[3].Ztop = roofHeight;
	// objectLists[4].Ztop is always "infinity"
	//  JPO - clouds can be below terrain level now
//	ShiAssert( terrainCeiling > cloudBase );	// -Z is down - 
	ShiAssert( cloudBase >= cloudTops );		// -Z is down

	previousTop = 1e12f;		// -Z is up
	for(i=0;i<nObjectLists;i++)
	{
		transList.list[i]=NULL;
		transList.top[i]=objectLists[i].Ztop;
		transList.bottom[i]=previousTop;
		previousTop=transList.top[i];
	}
	transList.top[nObjectLists-1]=-1e12f;

	// Update the membership of the altitude segregated object lists
	previousTop = 1e12f;		// -Z is up
	for (i=0; i<nObjectLists; i++)
		objectLists[i].displayList.UpdateMetrics( i, pos, &transList);


	for(i=0;i<nObjectLists;i++)
	{
		// Put any objects moved into their new list
		first=transList.list[i];
		while(first)
		{
			p=first;
			first=first->next;
			objectLists[i].displayList.InsertObject(p);
		}
	}

	// Resort each object list
	for (i=0; i<nObjectLists; i++) {
		objectLists[i].displayList.SortForViewpoint();
	}


	// Resort the cloud object list
	cloudList.UpdateMetrics( pos );
//	ShiAssert( (!lowList) && (!highList) );
	cloudList.SortForViewpoint();


	// Decide if we're affected by the nearest cloud
	// TODO:  Would we win elsewhere if we set a BOOL flag?
	DrawableOvercast *nearestCloud = (DrawableOvercast*)cloudList.GetNearest();
	if (nearestCloud) {
		ShiAssert( nearestCloud->GetClass() == DrawableObject::Overcast );
		cloudOpacity = nearestCloud->GetLocalOpacity( pos);
		nearestCloud->GetLocalColor( pos, &cloudColor );
	} else {
		cloudOpacity	= defcloudop;
	}
	if (cloudOpacity == 0) // Hack - set to haze color
	    TheTimeOfDay.GetVisColor(&cloudColor);
}

#endif

/***************************************************************************\
	Remove an instance of an object from the active display lists
\***************************************************************************/
void RViewPoint::InsertObject( DrawableObject* object )
{
	int i;

	ShiAssert( object );

	if (!object) // JB 010710 CTD?
		return;

	// Decide into which list to put the object
	for (i=0; i<nObjectLists; i++) {
		if ( object->position.z >= objectLists[i].Ztop ) {		// -Z is up
			objectLists[i].displayList.InsertObject( object );
			return;
		}
	}

	// We could only get here if the object was higher than the highest level
	ShiWarning( "Object with way to much altitude!" );
}


/***************************************************************************\
	Remove an instance of an object from the active display lists
\***************************************************************************/
void RViewPoint::RemoveObject( DrawableObject* object )
{
	ShiAssert( object );
	ShiAssert( object->parentList );
	
	// Take the given object out of its parent list
	object->parentList->RemoveObject( object );
}


/***************************************************************************\
	Reset all the object lists to start with their most distance objects.
\***************************************************************************/
void RViewPoint::ResetObjectTraversal( void )
{
	ShiAssert( IsReady() );
	
	int i;

	for (i=0; i<nObjectLists; i++) {
		objectLists[i].displayList.ResetTraversal();
	}
	cloudList.ResetTraversal();
}


/***************************************************************************\
	Return the index of the object list which contains object at the given
	z value.
\***************************************************************************/
int RViewPoint::GetContainingList( float zValue )
{
	ShiAssert( IsReady() );
	
	int i;

	for (i=0; i<nObjectLists; i++) {
		if ( zValue > objectLists[i].Ztop ) {
			// Remember, positive Z is downward
			return i;
		}
	}

	ShiWarning( "Z way too big" ); // Can only get here if we got a really huge z value
	return -1;
}


/***************************************************************************\
	Return TRUE if a line of sight exists between the two points with 
	respect to both clouds and terrain
\***************************************************************************/
float RViewPoint::CompositLineOfSight( Tpoint *p1, Tpoint *p2 )
{
	if (LineOfSight( p1, p2 )) {
		if (weather) {
			return weather->LineOfSight( p1 ,p2 );
		} else {
			return 1.0f;
		}
	} else {
		return 0.0f;
	}
}

/***************************************************************************\
	Return TRUE if a line of sight exists between the two points with 
	respect to clouds 
\***************************************************************************/
int RViewPoint::CloudLineOfSight( Tpoint *p1, Tpoint *p2 )
{
	if (weather) {
		return FloatToInt32(weather->LineOfSight( p1 ,p2 ));
	} else {
		return 1;
	}
}


/***************************************************************************\
	Update the moon texture for the time of day and altitude.
\***************************************************************************/
void RViewPoint::UpdateMoon()
{
	if (!TheTimeOfDay.ThereIsAMoon()) {
		lastDay = 1;
		return;
	}
	if (!lastDay) return;
	lastDay = 0;			// do it only once when the moon appear

	TheTimeOfDay.CalculateMoonPhase();

	// Edit the image data for the texture to darken a portion of the moon
	TheTimeOfDay.CreateMoonPhase ((unsigned char *)OriginalMoonTexture.imageData, (unsigned char *)MoonTexture.imageData);

	// Create the green moon texture based on the color version
	BYTE *texel		= (BYTE*)MoonTexture.imageData;
	BYTE *dest		= (BYTE*)GreenMoonTexture.imageData;
	BYTE *stopTexel	= (BYTE*)MoonTexture.imageData + MoonTexture.dimensions * MoonTexture.dimensions;
	while (texel < stopTexel) {
		if (*texel != 0) {					// Don't touch the chromakeyed texels!
			*dest++ = (BYTE)((*texel++) | 128);		// Use the "green" set of palette entries
		} 
		else {
			texel++;
			*dest++ = 0;
		}
	}
	MoonTexture.UpdateMPR();
	GreenMoonTexture.UpdateMPR();
}

/***************************************************************************\
    Setup the celestial textures (sun and moon)
\***************************************************************************/
void RViewPoint::SetupTextures()
{
	DWORD	*p, *stop;


	// Note that we haven't adapted for a particular day yet
	lastDay = -1;

	// Build the normal sun texture
	SunTexture.LoadAndCreate( "sun5.apl", MPR_TI_CHROMAKEY | MPR_TI_PALETTE );
	SunTexture.FreeImage();


	// Now load the image to construct the green sun texture
	// (Could do without this, but this is easy and done only once...)
	if (!GreenSunTexture.LoadImage( "sun5.apl", MPR_TI_CHROMAKEY | MPR_TI_PALETTE )) {
		ShiError( "Failed to load sun texture(2)" );
	}

	// Convert palette data to Green
	p		= GreenSunTexture.palette->paletteData;
	stop	= p+256;
	while( p < stop ) {
		*p &= 0xFF00FF00;
		p++;
	}
	GreenSunTexture.palette->UpdateMPR();

	// Convert chroma color to Green
	GreenSunTexture.chromaKey &= 0xFF00FF00;

	// Send the texture to MPR
	GreenSunTexture.CreateTexture();
	GreenSunTexture.FreeImage();

	// Now setup the moon textures.  (We'll tweak them periodicaly in BuildMoon)
	OriginalMoonTexture.LoadAndCreate( "moon.gif", MPR_TI_CHROMAKEY | MPR_TI_PALETTE | MPR_TI_ALPHA);
	MoonTexture.LoadAndCreate( "moon.gif", MPR_TI_CHROMAKEY | MPR_TI_PALETTE | MPR_TI_ALPHA);
	GreenMoonTexture.palette = MoonTexture.palette;
	GreenMoonTexture.LoadAndCreate( "moon.gif", MPR_TI_CHROMAKEY | MPR_TI_PALETTE | MPR_TI_ALPHA);

// build white moon with alpha
	MoonTexture.palette->paletteData[0] = 0;
	unsigned int color;
	DWORD *src;
	DWORD *dst = &MoonTexture.palette->paletteData[1];
	DWORD *end = &MoonTexture.palette->paletteData[48];
	while (dst < end) {
		int a = *dst & 0xff;
		*dst++ = (a << 24) + 0xffffff;
	}
#ifdef USE_TRANSPARENT_MOON
	src = &MoonTexture.palette->paletteData[1];
	dst = &MoonTexture.palette->paletteData[1+128];
	end = &MoonTexture.palette->paletteData[128+48];
#else
	src = &MoonTexture.palette->paletteData[1];
	dst = &MoonTexture.palette->paletteData[1+48];
	end = &MoonTexture.palette->paletteData[48+48];
	while (dst < end) {
		color = *src++;
		color = ((color >> 3) & 0xff000000) + 0xffffff;
		*dst++ = color;
	}
	src = &MoonTexture.palette->paletteData[1];
	dst = &MoonTexture.palette->paletteData[1+128];
	end = &MoonTexture.palette->paletteData[128+48+48];
#endif
// Setup the green portion of the moon palette
	while (dst < end) {
		color = *src++;
		*dst++ = (color & 0xff000000) + 0x00ff00;
	}

	// Update the moon palette
	MoonTexture.palette->UpdateMPR();



	// Request time updates
	TheTimeManager.RegisterTimeUpdateCB( TimeUpdateCallback, this );
}


/***************************************************************************\
    Release the celestial textures (sun and moon)
\***************************************************************************/
void RViewPoint::ReleaseTextures(void)
{
	// Stop receiving time updates
	TheTimeManager.ReleaseTimeUpdateCB( TimeUpdateCallback, this );

	// Free our texture resources
	SunTexture.FreeAll();
	GreenSunTexture.FreeAll();
	OriginalMoonTexture.FreeAll();
	MoonTexture.FreeAll();
	GreenMoonTexture.FreeAll();
}


/***************************************************************************\
	Update the moon and sun properties based on the time of day
\***************************************************************************/
void RViewPoint::TimeUpdateCallback( void *self ) {
	((RViewPoint*)self)->UpdateMoon();
}
