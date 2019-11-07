/***************************************************************************\
    DrawOVC.cpp
    Scott Randolph
    Jan 28, 1998

    Derived class to handle drawing overcast cloud layers.
\***************************************************************************/
#include "TimeMgr.h"
#include "Matrix.h"
#include "TOD.h"
#include "RenderOW.h"
#include "WXMap.h"
#include "Tex.h"
#include "DrawOVC.h"

#ifdef USE_SH_POOLS
MEM_POOL	DrawableOvercast::pool;
#endif

static const float TEX_UV_LSB	= (1.0f / 1024.0f);

static const float TEX_UV_MIN	= TEX_UV_LSB;
static const float TEX_UV_MAX	= 1.0f - TEX_UV_LSB;


static const int	TEXTURES_IN_SET = 19;
static Texture		textureTopNorm[TEXTURES_IN_SET];
static Texture		textureBottomNorm[TEXTURES_IN_SET];
static Texture		textureTopGreen[TEXTURES_IN_SET];
static Texture		textureBottomGreen[TEXTURES_IN_SET];
static Texture		*textureTop = textureTopNorm;
static Texture		*textureBottom = textureBottomNorm;

static const float	NOMINAL_FUZZ_DEPTH = 600.0f;
static const int	FUZZ_TABLE_SIZE = 16;
static float		fuzzDepthTable[FUZZ_TABLE_SIZE][FUZZ_TABLE_SIZE];

static const Tcolor	cloudTopColor		= { 0.9f, 0.9f, 0.9f };
static const Tcolor	cloudBottomColor	= { 0.4f, 0.4f, 0.4f };

Tcolor	DrawableOvercast::litCloudTopColor;
Tcolor	DrawableOvercast::litCloudBottomColor;



/***************************************************************************\
    Initialize an overcast tile.
\***************************************************************************/
DrawableOvercast::DrawableOvercast( DWORD code, int row, int col )
: DrawableObject( 1.0f )
{
	// Store our cloud tile code
	drawClassID = Overcast;
	tileCode = code;

	// Store the height at our corners
	// TODO:  These should be shared with neighbors...
	SWtop = TheWeather->TopsAt( row,   col   );
	SEtop = TheWeather->TopsAt( row,   col+1 );
	NWtop = TheWeather->TopsAt( row+1, col   );
	NEtop = TheWeather->TopsAt( row+1, col+1 );

	SWbottom = TheWeather->BaseAt( row,   col   );
	SEbottom = TheWeather->BaseAt( row,   col+1 );
	NWbottom = TheWeather->BaseAt( row+1, col   );
	NEbottom = TheWeather->BaseAt( row+1, col+1 );


	// Store our base elevation and top height
	// REMEMBER:  +Z is down!
	base = max(max(SWbottom, SEbottom), max(NWbottom, NEbottom));
	top  = min(min(SWtop,    SEtop),    min(NWtop,    NEtop));
	radius = 0.5f * CLOUD_CELL_SIZE * FEET_PER_KM;	// This is an under estimate, but should be okay
}



/***************************************************************************\
    Release an overcast tile.
\***************************************************************************/
DrawableOvercast::~DrawableOvercast( void )
{
	// Mark this object as invalid
	tileCode = (DWORD)~0;
}



/***************************************************************************\
    Draw the clouds with a normal or green texture.
\***************************************************************************/
void DrawableOvercast::SetGreenMode( BOOL greenMode )
{
	if (greenMode) {
		textureTop		= textureTopGreen;
		textureBottom	= textureBottomGreen;
	} else {
		textureTop		= textureTopNorm;
		textureBottom	= textureBottomNorm;
	}
}



/***************************************************************************\
    Update the position of this overcast tile (called periodically to
	account for wind).
\***************************************************************************/
void DrawableOvercast::UpdateForDrift( int row, int col )
{
	// Store our actual edge locations so we can match our neighbors
	northEdge	= TheWeather->TileToWorld( row+1 ) + TheWeather->xOffset * FEET_PER_KM;
	southEdge	= TheWeather->TileToWorld( row   ) + TheWeather->xOffset * FEET_PER_KM;
	eastEdge	= TheWeather->TileToWorld( col+1 ) + TheWeather->yOffset * FEET_PER_KM;
	westEdge	= TheWeather->TileToWorld( col   ) + TheWeather->yOffset * FEET_PER_KM;

	// Record our center point for list sorting purposes
	position.x = southEdge + (northEdge-southEdge) * 0.5f;
	position.y = westEdge  + (eastEdge -westEdge)  * 0.5f;
	position.z = base;
}



/***************************************************************************\
    Draw this overcast cell on the given renderer.

	TODO:  We should really compute all the verts for
	the overcast cells at once since most could 
	be shared.  For now, we do each cell in isolation.
\***************************************************************************/
//void DrawableOvercast::Draw( class RenderOTW *renderer, int LOD )
void DrawableOvercast::Draw( class RenderOTW *renderer, int)
{
	Tpoint p0, p1, p2, p3;
	ThreeDVertex v0, v1, v2, v3;
	float maxFogValue;

	// Generate the posts for the top of the cloud layer
	p0.x = southEdge; p0.y = westEdge; p0.z = SWtop;
	p1.x = northEdge; p1.y = westEdge; p1.z = NWtop;
	p2.x = northEdge; p2.y = eastEdge; p2.z = NEtop;
	p3.x = southEdge; p3.y = eastEdge; p3.z = SEtop;

	// Transform the posts
	renderer->TransformPoint(&p0, &v0);
	renderer->TransformPoint(&p1, &v1);
	renderer->TransformPoint(&p2, &v2);
	renderer->TransformPoint(&p3, &v3);

	// Compute how much fog should be applied at each post
	// TODO:  We need to do this based on cloud max range, not terrain (as this function does)
	v0.a = renderer->GetRangeOnlyFog( v0.csZ );
	v0.a = min(max( v0.a, 0.0f ), 0.98f);

	if(renderer->GetSmoothShadingMode())
	{
		v1.a = renderer->GetRangeOnlyFog( v1.csZ );
		v1.a = min( max( v1.a, 0.0f ), 0.98f );
		v2.a = renderer->GetRangeOnlyFog( v2.csZ );
		v2.a = min( max( v2.a, 0.0f ), 0.98f );
		v3.a = renderer->GetRangeOnlyFog( v3.csZ );
		v3.a = min( max( v3.a, 0.0f ), 0.98f );
		maxFogValue = max( max(v0.a,v1.a), max(v2.a,v3.a) );
	}

	else
	{
		v1.a = v0.a;
		v2.a = v0.a;
		v3.a = v0.a;
		maxFogValue = v0.a;
	}

	// Set the texture coordinates for the posts
	v0.u = TEX_UV_MIN,	v0.v = TEX_UV_MAX,	v0.q = v0.csZ * Q_SCALE;
	v1.u = TEX_UV_MIN,	v1.v = TEX_UV_MIN,	v1.q = v1.csZ * Q_SCALE;
	v2.u = TEX_UV_MAX,	v2.v = TEX_UV_MIN,	v2.q = v2.csZ * Q_SCALE;
	v3.u = TEX_UV_MAX,	v3.v = TEX_UV_MAX,	v3.q = v3.csZ * Q_SCALE;

	// Set the colors for the verts
	v0.r = v1.r = v2.r = v3.r = litCloudTopColor.r;
	v0.g = v1.g = v2.g = v3.g = litCloudTopColor.g;
	v0.b = v1.b = v2.b = v3.b = litCloudTopColor.b;

	// Select the proper texture and rendering state for the top
	if(renderer->GetTerrainTextureLevel() == 0)
		renderer->context.RestoreState( STATE_SOLID );		// Texturing of the terrain is turned off, so don't texture the overcast either
	else
	{
		// Do texturing
		// OW
		// renderer->context.SelectTexture( textureTop[tileCode-1].TexHandle() );

		// Select the appropriate mode
		if(textureTop == textureTopGreen)
		{
			if(tileCode == 0xF)
				renderer->context.RestoreState( STATE_TEXTURE_PERSPECTIVE );
			else
				renderer->context.RestoreState( STATE_TEXTURE_TRANSPARENCY_PERSPECTIVE );
		}

		else
		{
			if(tileCode == 0xF)
			{
				if(maxFogValue > 0.0f)
				{
					if(renderer->GetSmoothShadingMode())
						renderer->context.RestoreState(STATE_FOG_TEXTURE_SMOOTH_PERSPECTIVE);
					else
						renderer->context.RestoreState(STATE_FOG_TEXTURE_PERSPECTIVE);
				}

				else
					renderer->context.RestoreState(STATE_TEXTURE_PERSPECTIVE);
			}

			else
			{
				if(maxFogValue > 0.0f)
				{
					if(renderer->GetAlphaMode())
					{
						if(renderer->GetSmoothShadingMode())
							renderer->context.RestoreState(STATE_APT_FOG_TEXTURE_SMOOTH_PERSPECTIVE);
						else
							renderer->context.RestoreState(STATE_APT_FOG_TEXTURE_PERSPECTIVE);
					}

					else
					{
						if(renderer->GetSmoothShadingMode())
							renderer->context.RestoreState(STATE_FOG_TEXTURE_SMOOTH_TRANSPARENCY_PERSPECTIVE);
						else
							renderer->context.RestoreState(STATE_FOG_TEXTURE_TRANSPARENCY_PERSPECTIVE);
					}
				}

				else
				{
					if(renderer->GetAlphaMode())
						renderer->context.RestoreState(STATE_ALPHA_TEXTURE_PERSPECTIVE);
					else
						renderer->context.RestoreState(STATE_TEXTURE_TRANSPARENCY_PERSPECTIVE);
				}
			}
		}

		// Lets try always doing filtering on the clouds...
//		if(renderer->GetFilteringMode())
//		{
			renderer->context.SetState(MPR_STA_ENABLES, MPR_SE_FILTERING);
			renderer->context.SetState(MPR_STA_TEX_FILTER, MPR_TX_BILINEAR);
			renderer->context.InvalidateState();
//		}

		// OW
		renderer->context.SelectTexture(textureTop[tileCode-1].TexHandle());
	}

	// Submit the triangles making up the top surface
	renderer->DrawTriangle(&v0, &v1, &v2, CULL_ALLOW_CW);
	renderer->DrawTriangle(&v0, &v2, &v3, CULL_ALLOW_CW);

	// Select the proper texture for the bottom
	if(renderer->GetTerrainTextureLevel() != 0)
		renderer->context.SelectTexture(textureBottom[tileCode-1].TexHandle());

	// Generate the posts for the bottom of the cloud layer
	p0.z = SWtop;
	p1.z = NWtop;
	p2.z = NEtop;
	p3.z = SEtop;

	// Transform the posts
	renderer->TransformPoint(&p0, &v0);
	renderer->TransformPoint(&p1, &v1);
	renderer->TransformPoint(&p2, &v2);
	renderer->TransformPoint(&p3, &v3);

	// Assume the fog, texture and rendering states are the same for top and bottom...
	// Set the colors for the verts
	v0.r = v1.r = v2.r = v3.r = litCloudBottomColor.r;
	v0.g = v1.g = v2.g = v3.g = litCloudBottomColor.g;
	v0.b = v1.b = v2.b = v3.b = litCloudBottomColor.b;

	// Submit the triangles making up the bottom surface
	renderer->DrawTriangle(&v0, &v2, &v1, CULL_ALLOW_CW);
	renderer->DrawTriangle(&v0, &v3, &v2, CULL_ALLOW_CW);
}


/***************************************************************************\
    Return the alpha value from the texture for this cell at the
	provided coordinates in world space.
\***************************************************************************/
float DrawableOvercast::GetTextureAlpha( float x, float y )
{
	int		imgIndex;
	BYTE	palIndex;

	// Special case for the opaque tile to save time
	if (tileCode == 0xF) {
		return 1.0f;
	}

	// Skip out if we not within this cell
	if ((x > northEdge) || (x < southEdge) || 
		(y > eastEdge)  || (y < westEdge)) {
		return 0.0f;
	}

	ShiAssert( tileCode );								// Don't come here with an empty cell...
	ShiAssert( textureTopNorm[tileCode-1].imageData );	// When wouldn't we have a texture???
	ShiAssert( textureTopNorm[tileCode-1].palette );	// When wouldn't we have a palette???

	// Convert from world to texel space
	x -= northEdge;
	x *= textureTopNorm[tileCode-1].dimensions / (southEdge-northEdge);
	y -= westEdge;
	y *= textureTopNorm[tileCode-1].dimensions / (eastEdge-westEdge);

	imgIndex = FloatToInt32(x) * textureTopNorm[tileCode-1].dimensions + FloatToInt32(y);

	palIndex = ((BYTE*)textureTopNorm[tileCode-1].imageData)[ imgIndex ];

	// To be truely correct, we should bilinear filter, but that would require looking into
	// neighboring cells, and probably isn't worth it...
	if (textureTopNorm[tileCode-1].palette)
		return (textureTopNorm[tileCode-1].palette->paletteData[ palIndex ] >> 24) / 255.0f;
	
	return 0.0;
}



/***************************************************************************\
    This function is called get the properties of the cloud at the specified
	location.  The location of interest may be outside the boundry of the
	cloud.  REMEMBER... -Z is up!
\***************************************************************************/
float DrawableOvercast::GetLocalOpacity( Tpoint *point )
{
	float EffectiveFuzzDepth = GetLocalFuzzDepth( point );
//	float thickness = (base-top) + EffectiveFuzzDepth + EffectiveFuzzDepth;

	// Problem:  We're assuming flat tiles.  Will stair-step at junctions.
	// TODO:  Compute exact distance from the drawn triangles...

	// Compute opacity based on our depth within the cloud
	float mid = (base+top)/2.0f;
	float d = (float)fabs(point->z - mid);
	float o = ((mid-top+EffectiveFuzzDepth) - d)/EffectiveFuzzDepth;
	if (o < 0.0f)	o = 0.0f;
	if (o > 1.0f)	o = 1.0f;

	// If we have a texture, we need to account for transparency in the texture
	ShiAssert( textureTopNorm[tileCode-1].imageData );	// When wouldn't we have a texture???
//	if (textureTopNorm[tileCode-1].imageData)
		o *= GetTextureAlpha( point->x, point->y );

	return o;
}



/***************************************************************************\
    This function is called get the properties of the cloud at the specified
	location.  The location of interest may be outside the boundry of the
	cloud.  REMEMBER... -Z is up!
\***************************************************************************/
void DrawableOvercast::GetLocalColor( Tpoint *point, Tcolor *color )
{
	float	h, invh;
	float	EffectiveFuzzDepth = GetLocalFuzzDepth( point );
	float	thickness = (base-top) + EffectiveFuzzDepth + EffectiveFuzzDepth;

	// Problem:  We're assuming flat tiles.  Will stair-step at junctions.
	// TODO:  Compute exact distance from the drawn triangles...

	// Compute color
	h			=  (point->z - (top-EffectiveFuzzDepth)) / thickness;
	invh		= 1.0f - h;
	color->r	= invh * litCloudTopColor.r + h * litCloudBottomColor.r;
	color->g	= invh * litCloudTopColor.g + h * litCloudBottomColor.g;
	color->b	= invh * litCloudTopColor.b + h * litCloudBottomColor.b;
}



/***************************************************************************\
    This function is called from the miscellanious texture loader function.
	It must be hardwired into that function.
\***************************************************************************/
//void DrawableOvercast::SetupTexturesOnDevice( DWORD rc )
void DrawableOvercast::SetupTexturesOnDevice(DXContext *rc)
{
	char			filename[_MAX_PATH];
	int				i;

	// Load our normal textures
	textureTopNorm[0].LoadAndCreate( "FogTileT01.APL", MPR_TI_CHROMAKEY | MPR_TI_PALETTE );
	textureBottomNorm[0].palette = textureTopNorm[0].palette;
	textureBottomNorm[0].LoadAndCreate( "FogTileB01.APL", MPR_TI_CHROMAKEY | MPR_TI_PALETTE );

	for (i=1; i<TEXTURES_IN_SET; i++) {
		sprintf( filename, "FogTileT%02d.APL", i+1 );
		textureTopNorm[i].palette = textureTopNorm[0].palette;
		textureTopNorm[i].LoadAndCreate( filename, MPR_TI_CHROMAKEY | MPR_TI_PALETTE );

		sprintf( filename, "FogTileB%02d.APL", i+1 );
		textureBottomNorm[i].palette = textureBottomNorm[0].palette;
		textureBottomNorm[i].LoadAndCreate( filename, MPR_TI_CHROMAKEY | MPR_TI_PALETTE );
	}

	// Load our green textures
	textureTopGreen[0].LoadAndCreate( "FogTileGT01.GIF", MPR_TI_CHROMAKEY | MPR_TI_PALETTE );
	textureBottomGreen[0].palette = textureTopGreen[0].palette;
	textureBottomGreen[0].LoadAndCreate( "FogTileGB01.GIF", MPR_TI_CHROMAKEY | MPR_TI_PALETTE );

	for (i=1; i<TEXTURES_IN_SET; i++) {
		sprintf( filename, "FogTileGT%02d.GIF", i+1 );
		textureTopGreen[i].palette = textureTopGreen[0].palette;
		textureTopGreen[i].LoadAndCreate( filename, MPR_TI_CHROMAKEY | MPR_TI_PALETTE );

		sprintf( filename, "FogTileGB%02d.GIF", i+1 );
		textureBottomGreen[i].palette = textureBottomGreen[0].palette;
		textureBottomGreen[i].LoadAndCreate( filename, MPR_TI_CHROMAKEY | MPR_TI_PALETTE );
	}

	// Build our fuzz depth table
	BuildFuzzDepthTable();

	// Initialize the lighting conditions and register for future time of day updates
	TimeUpdateCallback( NULL );
	TheTimeManager.RegisterTimeUpdateCB( TimeUpdateCallback, NULL );
}



/***************************************************************************\
    This function is called from the miscellanious texture cleanup function.
	It must be hardwired into that function.
\***************************************************************************/
//void DrawableOvercast::ReleaseTexturesOnDevice( DWORD rc )
void DrawableOvercast::ReleaseTexturesOnDevice(DXContext *rc)
{
	// Stop receiving time updates
	TheTimeManager.ReleaseTimeUpdateCB( TimeUpdateCallback, NULL );

	// Release our textures
	for (int i=0; i<TEXTURES_IN_SET; i++) {
		textureTopNorm[i].FreeAll();
		textureBottomNorm[i].FreeAll();
		textureTopGreen[i].FreeAll();
		textureBottomGreen[i].FreeAll();
	}
}



/***************************************************************************\
    This function is called get the properties of the cloud at the specified
	location.  The location of interest may be outside the boundry of the
	cloud.  REMEMBER... -Z is up!
\***************************************************************************/
float DrawableOvercast::GetLocalFuzzDepth( Tpoint *point )
{
	int	r, c;

	r = FloatToInt32( point->x/100.0f ) % (FUZZ_TABLE_SIZE-1);
	c = FloatToInt32( point->y/100.0f ) % (FUZZ_TABLE_SIZE-1);

	return fuzzDepthTable[r][c];
}



/***************************************************************************\
    This function is called from the miscellanious texture cleanup function.
	It must be hardwired into that function.
\***************************************************************************/
void DrawableOvercast::BuildFuzzDepthTable( void )
{
	int r, c;

	// First fill in random values in every other cell
	for (r=0; r<FUZZ_TABLE_SIZE; r+=2) {
		for (c=0; c<FUZZ_TABLE_SIZE; c+=2) {
			fuzzDepthTable[r][c] = NOMINAL_FUZZ_DEPTH * (((float)rand() / RAND_MAX) + 0.5f);
		}
	}

	// Blend along the columns (wrapping at the edge)
	for (r=1; r<FUZZ_TABLE_SIZE; r+=2) {
		for (c=0; c<FUZZ_TABLE_SIZE; c+=2) {

			fuzzDepthTable[r][c]     =  0.2f * NOMINAL_FUZZ_DEPTH * (((float)rand() / RAND_MAX) + 0.5f);

			fuzzDepthTable[r][c]     += 0.4f * fuzzDepthTable[r-1][c];
			if (r+1 < FUZZ_TABLE_SIZE) {
				fuzzDepthTable[r][c] += 0.4f * fuzzDepthTable[r+1][c];
			} else {
				fuzzDepthTable[r][c] += 0.4f * fuzzDepthTable[0][c];
			}
		}
	}

	// Blend along the rows (wrapping at the edge)
	for (r=0; r<FUZZ_TABLE_SIZE; r++) {
		for (c=1; c<FUZZ_TABLE_SIZE; c+=2) {

			fuzzDepthTable[r][c]     =  0.2f * NOMINAL_FUZZ_DEPTH * (((float)rand() / RAND_MAX) + 0.5f);

			fuzzDepthTable[r][c]     += 0.4f * fuzzDepthTable[r][c-1];
			if (c+1 < FUZZ_TABLE_SIZE) {
				fuzzDepthTable[r][c] += 0.4f * fuzzDepthTable[r][c+1];
			} else {
				fuzzDepthTable[r][c] += 0.4f * fuzzDepthTable[r][0];
			}
		}
	}
}



/***************************************************************************\
    Update the light level on the clouds.
	NOTE:  Since the textures are static, this function can also
		   be static, so the self parameter is ignored.
\***************************************************************************/
//void DrawableOvercast::TimeUpdateCallback( void *unused )
void DrawableOvercast::TimeUpdateCallback( void *)
{
	Tcolor	light;

	// Get the light level from the time of day manager
	TheTimeOfDay.GetTextureLightingColor( &light );


	// Update the color color for time of day
	litCloudTopColor.r = cloudTopColor.r * light.r;
	litCloudTopColor.g = cloudTopColor.g * light.g;
	litCloudTopColor.b = cloudTopColor.b * light.b;

	litCloudBottomColor.r = cloudBottomColor.r * light.r;
	litCloudBottomColor.g = cloudBottomColor.g * light.g;
	litCloudBottomColor.b = cloudBottomColor.b * light.b;

	
	// Update the cloud texture palettes for time of day
	textureTopNorm[0].palette->LightTexturePalette( &light );
	textureTopGreen[0].palette->LightTexturePalette( &light );
}
