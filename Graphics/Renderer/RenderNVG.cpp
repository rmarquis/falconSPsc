/***************************************************************************\
    RenderNVG.cpp
    Scott Randolph
    August 12, 1996

    This sub class draws an out the window view in simulated IR (green on black)
\***************************************************************************/
#include "Tmap.h"
#include "Tpost.h"
#include "TOD.h"
#include "Tex.h"
#include "Draw2D.h"
#include "DrawOVC.h"
#include "ColorBank.h"
#include "RenderNVG.h"


/***************************************************************************\
	Setup the rendering context for this view
\***************************************************************************/
void RenderNVG::Setup( ImageBuffer *imageBuffer, RViewPoint *vp )
{
	// Skip our parent class and go to our grandparent
	RenderOTW::Setup( imageBuffer, vp );
}



/***************************************************************************\
	Do start of frame housekeeping
\***************************************************************************/
void RenderNVG::StartFrame( void ) {
	RenderOTW::StartFrame();
	Drawable2D::SetGreenMode( TRUE );
	DrawableOvercast::SetGreenMode( TRUE );

	TheColorBank.SetColorMode( ColorBankClass::UnlitGreenMode );
}



/***************************************************************************\
	Convert color requests to green to avoid special cases in the calling
	code when the target is "Green"
\***************************************************************************/
void RenderNVG::SetColor( DWORD packedRGBA )
{
#if 1
	// Go ahead and use the true color
	RenderOTW::SetColor( packedRGBA );
#else
	// This would be the same as the RenderTV -- everything would go green
	packedRGBA |= (packedRGBA<<8) & 0xFF;	// Pull in red
	packedRGBA |= (packedRGBA>>8) & 0xFF;	// Pull in blue
	RenderOTW::SetColor( packedRGBA & 0xFF00FF00 );
#endif
}


/***************************************************************************\
	We actually want our Grandparents draw sun routine...
\***************************************************************************/
void RenderNVG::DrawSun( void )
{
	RenderOTW::DrawSun();
}



/***************************************************************************\
    Compute the color and texture blend value for a single terrain vertex.
\***************************************************************************/
void RenderNVG::ComputeVertexColor( TerrainVertex *vert, Tpost *post, float distance )
{
	float	alpha;
	float	fog;
	float	inv_fog;
	float	intensity;

	// We're drawing a green on black display, so no red or blue components
	vert->r		= 0.0f;
	vert->b		= 0.0f;
	intensity	= TheMap.GreenTable[post->colorIndex].g * NVG_LIGHT_LEVEL;

	// Get the color information from the post and optionally fog it
	if ( distance > haze_start+haze_depth ) {

		vert->g = haze_ground_color.g;
		vert->a = FOG_MIN;

		vert->RenderingStateHandle = state_far;

	} else if (distance < PERSPECTIVE_RANGE) {

		vert->g = intensity;
		vert->a = FOG_MAX;

		vert->RenderingStateHandle = state_fore;
		
	} else if (!hazed && distance < haze_start) {

		vert->g = intensity;
		vert->a = FOG_MAX;

		vert->RenderingStateHandle = state_near;
		
	} else {

		fog = GetValleyFog( distance, post->z );
		if (fog > FOG_MAX)		fog = FOG_MAX;
		if (fog < FOG_MIN)		fog = FOG_MIN;
		inv_fog = 1.0f - fog;

		vert->g = (fog*haze_ground_color.g + inv_fog*intensity);

		if (distance > blend_start+blend_depth) {
			vert->a = FOG_MIN;
			vert->RenderingStateHandle = state_far;
		} else {
			alpha = (distance - blend_start) / blend_depth;
			if (alpha > FOG_MAX)	alpha = FOG_MAX;
			if (alpha < fog)		alpha = fog;

			vert->a = 1.0f - alpha;
			vert->RenderingStateHandle = state_mid;
		}
	}
}



/***************************************************************************\
    Establish the lighting parameters for this renderer as the time of
	day changes.
\***************************************************************************/
void RenderNVG::SetTimeOfDayColor( void )
{
	// Set 3D object lighting environment
	lightAmbient = NVG_LIGHT_LEVEL;
	lightDiffuse = 0.0f;
	TheTimeOfDay.GetLightDirection( &lightVector );

	// Get the new colors for this time of day
	sky_color.r			= 0.0f;
	sky_color.g			= NVG_SKY_LEVEL;
	sky_color.b			= 0.0f;
	haze_sky_color.r	= 0.0f;
	haze_sky_color.g	= NVG_SKY_LEVEL;
	haze_sky_color.b	= 0.0f;
	earth_end_color.r	= 0.0f;
	earth_end_color.g	= NVG_SKY_LEVEL;
	earth_end_color.b	= 0.0f;
	haze_ground_color.r	= 0.0f;
	haze_ground_color.g	= NVG_SKY_LEVEL;
	haze_ground_color.b	= 0.0f;

	// Set the fog color for the terrain
	DWORD ground_haze = (FloatToInt32(haze_ground_color.g * 255.9f) <<  8 ) + + 0xff000000;
	context.SetState( MPR_STA_FOG_COLOR, ground_haze );

	// TODO:  Set the fog color for the objects
//	TheStateStack.SetDepthCueColor( 0.0f, haze_ground_color.g, 0.0f );

	// Adjust the color of the roof textures if they're loaded
	if (texRoofTop.TexHandle()) {
		Tcolor	light = { 0.0f, NVG_LIGHT_LEVEL, 0.0f };;
		texRoofTop.palette->LightTexturePalette( &light );
		texRoofBottom.palette->LightTexturePalette( &light );
	}
}



/***************************************************************************\
    Adjust the target color as necessary for display.  This is used for
	sky colors.
\***************************************************************************/
void RenderNVG::ProcessColor( Tcolor *color )
{
	color->r  = 0.0f;
	color->g *= NVG_LIGHT_LEVEL;
	color->b  = 0.0f;
}
