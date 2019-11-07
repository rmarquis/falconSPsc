/***************************************************************************\
    RenderIR.cpp
    Scott Randolph
    August 5, 1996

    This sub class draws an out the window view in simulated IR (green on black)
\***************************************************************************/
#include "Tmap.h"
#include "Tpost.h"
#include "Draw2D.h"
#include "DrawOVC.h"
#include "RenderIR.h"


/***************************************************************************\
	Setup the rendering context for this view
\***************************************************************************/
void RenderIR::Setup( ImageBuffer *imageBuffer, RViewPoint *vp )
{
	// Call our base classes setup routine
	RenderTV::Setup( imageBuffer, vp );
	
	// Use a black sky
	sky_color.r			= sky_color.g			= sky_color.b			= 0.0f;
	haze_sky_color.r	= haze_sky_color.g		= haze_sky_color.b		= 0.0f;
	haze_ground_color.r	= haze_ground_color.g	= haze_ground_color.b	= 0.0f;
	earth_end_color.r	= earth_end_color.g		= earth_end_color.b		= 0.0f;

	// Use only bright ambient lighting on the objects
	lightAmbient = 1.0f;
	lightDiffuse = 0.0f;

	// TODO:  Set the fog color for the objects
//	TheStateStack.SetDepthCueColor( haze_ground_color.r, haze_ground_color.g, haze_ground_color.b );
}



/***************************************************************************\
	Do start of frame housekeeping
\***************************************************************************/
void RenderIR::StartFrame( void ) {
	RenderOTW::StartFrame();
	Drawable2D::SetGreenMode( TRUE );
	DrawableOvercast::SetGreenMode( TRUE );

	TheColorBank.SetColorMode( ColorBankClass::UnlitGreenMode );
}



/***************************************************************************\
    Compute the color and texture blend value for a single terrain vertex.
\***************************************************************************/
void RenderIR::ComputeVertexColor( TerrainVertex *vert, Tpost *post, float distance )
{
	float	alpha;

	// We're drawing a green on black display, so no red or blue components
	vert->r = 0.0f;
	vert->b = 0.0f;

	// Get the color information from the post and optionally fog it
	if ( hazed && (distance > haze_start)) {
		alpha = (distance - haze_start) / (far_clip - haze_start);
		if (alpha > 1.0f)  alpha = 1.0f;
		vert->g = (1.0f - alpha) * TheMap.ColorTable[post->colorIndex].r;
	} else {
		vert->g = TheMap.ColorTable[post->colorIndex].r;
	}

	// Set the rendering state for this vertex
	vert->RenderingStateHandle = state_far;
}
