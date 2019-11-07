/***************************************************************************\
    RenderTV.cpp
    Scott Randolph
    August 12, 1996

    This sub class draws an out the window view in simulated IR (green on black)
\***************************************************************************/
#include "TOD.h"
#include "Tmap.h"
#include "Tpost.h"
#include "Tex.h"
#include "Draw2D.h"
#include "DrawOVC.h"
#include "ColorBank.h"
#include "RViewPnt.h"
#include "RenderTV.h"


/***************************************************************************\
	Setup the rendering context for this view
\***************************************************************************/
void RenderTV::Setup( ImageBuffer *imageBuffer, RViewPoint *vp )
{
	// Call our base classes setup routine
	RenderOTW::Setup( imageBuffer, vp );
	
	// Set the default sky and haze properties
	SetSmoothShadingMode( TRUE );
	// JPO - this is weird. Doesn't do what it says on the tin, so I'm removing it
//	SetTerrainTextureLevel( -1 );	// Start with terrain textures OFF
	SetObjectTextureState( FALSE );	// Start with object textures OFF
	SetHazeMode( TRUE );			// Start with hazing turned ON

	// Update our colors to account for our changes to the default settings
	TimeUpdateCallback( this );
}



/***************************************************************************\
	Do start of frame housekeeping
\***************************************************************************/
void RenderTV::StartFrame( void ) {
	RenderOTW::StartFrame();
	Drawable2D::SetGreenMode( TRUE );
	DrawableOvercast::SetGreenMode( TRUE );

	TheColorBank.SetColorMode( ColorBankClass::GreenMode );
}



/***************************************************************************\
	Do end of frame housekeeping
\***************************************************************************/
void RenderTV::FinishFrame( void ) {
	RenderOTW::FinishFrame();
	Drawable2D::SetGreenMode( FALSE );
	DrawableOvercast::SetGreenMode( FALSE );
}



/***************************************************************************\
	Convert color requests to green to avoid special cases in the calling
	code when the target is "Green"
\***************************************************************************/
void RenderTV::SetColor( DWORD packedRGBA )
{
	packedRGBA |= (packedRGBA<<8) & 0xFF;	// Pull in red
	packedRGBA |= (packedRGBA>>8) & 0xFF;	// Pull in blue
	RenderOTW::SetColor( packedRGBA & 0xFF00FF00 );
}



/***************************************************************************\
    Compute the color and texture blend value for a single terrain vertex.
\***************************************************************************/
void RenderTV::ComputeVertexColor( TerrainVertex *vert, Tpost *post, float distance )
{
	float	alpha;
	float	intensity;

	// We're drawing a green on black display, so no red or blue components
	vert->r = 0.0f;
	vert->b = 0.0f;
	intensity =	TheMap.DarkGreenTable[post->colorIndex].g;

	// Get the color information from the post and optionally fog it
	if ( hazed && (distance > haze_start)) {
		alpha = (distance - haze_start) / (far_clip - haze_start);
		if (alpha > 1.0f)  alpha = 1.0f;
		vert->g = alpha*haze_ground_color.g + (1.0f - alpha)*intensity;
	} else {
		vert->g = intensity;
	}

	// Set the rendering state for this vertex
	vert->RenderingStateHandle = state_far;
}


/***************************************************************************\
    Adjust the target color as necessary for display.  This is used for
	sky colors.
\***************************************************************************/
void RenderTV::ProcessColor( Tcolor *color )
{
	color->r = 0.0f;
	color->b = 0.0f;
}


/***************************************************************************\
	Draw a green sun
\***************************************************************************/
void RenderTV::DrawSun( void )
{
	Tpoint	center;

	ShiAssert( TheTimeOfDay.ThereIsASun() );

	// Get the center point of the body on a unit sphere in world space
	TheTimeOfDay.CalculateSunMoonPos( &center, FALSE );

	// Draw the sun
	context.RestoreState( STATE_TEXTURE_TRANSPARENCY );
	context.SelectTexture( viewpoint->GreenSunTexture.TexHandle() );
	DrawCelestialBody( &center, SUN_DIST );
}


/***************************************************************************\
	Draw a green moon
\***************************************************************************/
void RenderTV::DrawMoon( void )
{
	Tpoint	center;

	ShiAssert( TheTimeOfDay.ThereIsAMoon() );

	// Get the center point of the body on a unit sphere in world space
	TheTimeOfDay.CalculateSunMoonPos( &center, TRUE );

	// Draw the object
	context.RestoreState( STATE_TEXTURE_TRANSPARENCY );
	context.SelectTexture( viewpoint->GreenMoonTexture.TexHandle() );
	DrawCelestialBody( &center, MOON_DIST );
}
