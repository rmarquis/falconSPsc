/***************************************************************************\
    DrawSgmt.cpp
    Scott Randolph
    May 3, 1996

    Derived class to handle drawing segmented trails (like contrails).
\***************************************************************************/
#include "TimeMgr.h"
#include "TOD.h"
#include "RenderOW.h"
#include "RViewPnt.h"
#include "Tex.h"
#include "DrawSgmt.h"

#ifdef USE_SH_POOLS
MEM_POOL	DrawableTrail::pool;
MEM_POOL	TrailElement::pool;
#endif

// this define handles LODing each seg with
// more or less triangles.  It basically corresponds to a pixel
// dimension
#define SEG_LOD_VAL		20.0f
#define HIGH_LOD_VAL	(SEG_LOD_VAL * 2.0f )

#define NUM_HIGH_LOD_VERTS	18
#define HALF_HIGH_LOD_VERTS	(NUM_HIGH_LOD_VERTS/2)

Tpoint gCircleVerts[NUM_HIGH_LOD_VERTS];
ThreeDVertex gV0[NUM_HIGH_LOD_VERTS];
ThreeDVertex gV1[NUM_HIGH_LOD_VERTS];
ThreeDVertex gVHead1[5];
extern Texture	gAplTextures[];
extern Texture	gAplTexturesGreen[];
extern char FalconObjectDataDir[];
BOOL gHeadOn = FALSE;

// bleh
extern int sGreenMode;
BOOL gTextured;

typedef struct TrailTypeEntry {
	BOOL	selfIlum;
	float 	initLight;
	float	lightFade;
	float	tileAmt;
	float	startRadius;
	float	maxRadius;
	float	expandRate;
	float	disipation;
	float	r, g, b, a;
	float	rLite, gLite, bLite;
	Texture	*tex;
	int		headOnTex;
} TrailTypeEntry;


// !!! Have to manually add new textures here
static Texture MissleTrailTexture;
static Texture FireTrailTexture;
static Texture SmokeTrailTexture;
static Texture GunTrailTexture;


// !!! Have to manually add new trail types here
static TrailTypeEntry types[] = {
//   silum	ilight	lfade	    tile    radius   rmax	 expand	  decay/s   red    green  blue   alpha	 NA	   NA	 NA	   texture
	{FALSE,	1.0f,	0.0040f,	3.0f,  10.0f, 20.0f,  0.018f,  0.0003f,  1.00f, 1.00f, 1.00f, 1.00f, 0.0f, 0.0f, 0.0f, &MissleTrailTexture, 7 }, // 0 Contrail
	{FALSE,	1.0f,	0.0040f,	3.0f,   2.0f, 20.0f,  0.000f,  0.0016f,  0.90f, 0.90f, 0.90f, 0.50f, 0.0f, 0.0f, 0.0f, &MissleTrailTexture, 7 }, // 1 Vortex

#if 0
	{FALSE,	1.0f,	0.0040f,	1.0f,   1.0f,  6.0f,  0.032f,  0.00008f, 0.90f, 0.90f, 0.90f, 0.95f, 0.0f, 0.0f, 0.0f, &MissleTrailTexture, 7 }, // 2 Missle Trail AIM120
#else
	// OW - almost twice as thick, and twice the decay time
	{FALSE,	1.0f,	0.0040f,	1.0f,   1.5f,  9.0f,  0.032f,  0.00004f, 0.90f, 0.90f, 0.90f, 0.95f, 0.0f, 0.0f, 0.0f, &MissleTrailTexture, 7 }, // 2 Missle Trail AIM120
#endif

	{TRUE,	1.0f,	0.0000f,	1.0f,   0.5f,  0.5f,  0.000f,  0.0004f,  0.75f, 0.30f, 0.30f, 0.65f, 0.0f, 0.0f, 0.0f, NULL, 0 },   // 3 tracers
	{TRUE,	1.0f,	0.0000f,	3.0f,   0.5f,  0.5f,  0.000f,  0.016f,   1.00f, 0.80f, 0.40f, 0.50f, 0.0f, 0.0f, 0.0f, NULL, 0 },				  // 4 Tracer
	{FALSE,	1.0f,	0.0040f,	1.0f,   1.0f, 34.0f,  0.018f,  0.00016f, 0.10f, 0.10f, 0.10f, 0.60f, 0.0f, 0.0f, 0.0f, &SmokeTrailTexture, 10 },  // 5 Smoke
	{TRUE,	1.0f,	0.0000f,	1.0f,   4.0f, 44.0f,  0.002f,  0.0007f,  0.75f, 0.30f, 0.30f, 0.80f, 0.0f, 0.0f, 0.0f, &FireTrailTexture,0  },   // 6 Fire
	{FALSE,	1.0f,	0.0040f,	3.0f,   5.0f, 35.0f,  0.002f,  0.00016f, 0.20f, 0.20f, 0.20f, 0.80f, 0.0f, 0.0f, 0.0f, &SmokeTrailTexture, 10 }, // 7 Exploding piece trail
	{TRUE,	1.0f,	0.0000f,	3.0f,   4.0f,  4.0f,  0.000f,  0.0004f,  0.75f, 0.30f, 0.30f, 0.80f, 0.0f, 0.0f, 0.0f, &FireTrailTexture, 0 },   // 8 Thinner Fire
	{TRUE,	1.0f,	0.0000f,	3.0f,  50.0f, 50.0f,  0.000f,  0.00008f, 0.90f, 0.90f, 0.90f, 1.00f, 0.0f, 0.0f, 0.0f, NULL }, // 9 Missle Trail distant
	{FALSE,	1.0f,	0.0040f,	1.0f,  6.0f,  34.0f,  0.0025f, 0.00008f, 0.40f, 0.40f, 0.30f, 1.00f, 0.0f, 0.0f, 0.0f, NULL }, // 10 Dust
	{FALSE,	1.0f,	0.0040f,	2.0f,  1.0f,  24.0f,  0.022f,  0.00173f, 0.60f, 0.60f, 0.60f, 0.90f, 0.0f, 0.0f, 0.0f, &GunTrailTexture, 10 }, // 11 Gun fire
	{TRUE,	1.0f,	0.0000f,	1.0f,   .1f,    .1f,  0.000f,  0.00000f, 1.00f, 0.00f, 0.00f, 1.00f, 0.0f, 0.0f, 0.0f, NULL, 0 }, // 12 left ACMI wing trail
	{TRUE,	1.0f,	0.0000f,	1.0f,   .1f,    .1f,  0.000f,  0.00000f, 0.00f, 1.00f, 0.00f, 1.00f, 0.0f, 0.0f, 0.0f, NULL, 0 }, // 13 right ACMI wing trail
	{FALSE,	1.0f,	0.0040f,	3.0f,   1.0f,  1.0f,  0.000f,  0.00168f, 0.90f, 0.90f, 0.90f, 0.60f, 0.0f, 0.0f, 0.0f, &MissleTrailTexture, 7 }, // 14 Rocket Trail
	{FALSE,	1.0f,	0.0040f,	1.0f,   1.0f, 20.0f,  0.038f,  0.00004f, 0.50f, 0.50f, 0.50f, 0.70f, 0.0f, 0.0f, 0.0f, &SmokeTrailTexture, 10 },  // 15 Missile Smoke
	{FALSE,	1.0f,	0.0040f,	1.0f,   1.0f,  2.0f,  0.032f,  0.00008f, 0.90f, 0.90f, 0.90f, 0.70f, 0.0f, 0.0f, 0.0f, &MissleTrailTexture, 7 }, // 16 Generic Missle Trail
	{FALSE,	1.0f,	0.0040f,	1.0f,   1.0f, 34.0f,  0.018f,  0.00012f, 0.10f, 0.10f, 0.10f, 1.00f, 0.0f, 0.0f, 0.0f, &SmokeTrailTexture, 10 },  // 17 Darker Smoke
	{TRUE,	1.0f,	0.0000f,	1.0f,   3.0f, 26.0f,  0.010f,  0.00040f,  0.75f, 0.30f, 0.30f, 1.00f, 0.0f, 0.0f, 0.0f, &FireTrailTexture, 0 },   // 18 Fire
	{FALSE,	1.0f,	0.0040f,	1.0f,   0.5f,  2.0f,  0.032f,  0.00004f, 0.90f, 0.90f, 0.90f, 0.95f, 0.0f, 0.0f, 0.0f, &MissleTrailTexture, 7 }, // 19 wing tip vortex
};
static const int nTypes = sizeof( types ) / sizeof( types[0] );
static const char TRAILFILE[] = "trail.dat";
extern FILE* OpenCampFile (char *filename, char *ext, char *mode);

void LoadTrails ()
{
	int ind;
	float 	initLight;
	float	lightFade;
	float	tileAmt;
	float	startRadius;
	float	maxRadius;
	float	expandRate;
	float	disipation;
	float	r, g, b, a;

	TrailTypeEntry *tp;
    char path[_MAX_PATH];
    FILE *fp;

    fp = OpenCampFile("trail", "dat", "rt");
    if (fp == NULL) {
	sprintf (path, "%s\\%s", FalconObjectDataDir, TRAILFILE);
	fp = fopen(path, "r");
	if (fp == NULL) return;
    }
    while (fgets (path, sizeof path, fp)) {
	if (path[0] == '#' || path[0] == ';' ||path[0] == '\n')
	    continue;
	if (sscanf (path, "%d %g %g %g %g %g %g %g %g %g %g %g", 
	    &ind,
	    &initLight, &lightFade, &tileAmt, &startRadius,
	    &maxRadius, &expandRate, &disipation, &r, &g, &b, &a) != 12)
	    continue;
	if (ind < 0 || ind >= nTypes)
	    continue;
	tp = &types[ind];
	tp->initLight = initLight;
	tp->lightFade = lightFade;
	tp->tileAmt = tileAmt;
	tp->startRadius = startRadius;
	tp->maxRadius = maxRadius;
	tp->expandRate = expandRate;
	tp->disipation = disipation;
	tp->r = r;
	tp->g = g;
	tp->b = b;
	tp->a = a;
    }
    fclose (fp);
}


// !! Add any type ids which are NOT self-illuminating here
static TrailTypeEntry* liteTypes[] = {
	&types[0], 
	&types[1],
	&types[2],
	&types[5],
	&types[7],
	&types[10],
	&types[11],
	&types[14],
	&types[15],
	&types[16],
	&types[17],
	&types[19],
};
static const int nLiteTypes = sizeof( liteTypes ) / sizeof( liteTypes[0] );


// !! Add any textures which are NOT self-illuminating here
static const Texture* liteTextures[] = {
	&MissleTrailTexture,
	&SmokeTrailTexture,
	&GunTrailTexture
};
static const int nLiteTextures = sizeof( liteTextures ) / sizeof( liteTextures[0] );

static	Tcolor	gLight;

// for when fakerand just won't do
#define	NRANDPOS ((float)( (float)rand()/(float)RAND_MAX ))
#define NRAND	 ( 1.0f - 2.0F * NRANDPOS )



/***************************************************************************\
    Initialize a segmented trial object.
\***************************************************************************/
DrawableTrail::DrawableTrail( int trailType, float scale )
: DrawableObject( scale )
{
	ShiAssert( trailType >= 0 );

	// Store the trail type
	type = trailType;

	// Store this trail's radius
	radius = types[trailType].startRadius;

	// Start with no trail segments
	head = NULL;

	// used by ACMI
	keepStaleSegs = FALSE;

   // Set to position 0.0, 0.0, 0.0;
   position.x = 0.0F;
   position.y = 0.0F;
   position.z = 0.0F;
}



/***************************************************************************\
    Remove an instance of a segmented trail object.
\***************************************************************************/
DrawableTrail::~DrawableTrail( void )
{
	// Delete this object's trail
	delete head;	// (recursively deletes entire trail in TrailElement destructor)
	head = NULL;
}



/***************************************************************************\
    Add a point to the list which define this segmented trail.
\***************************************************************************/
void DrawableTrail::AddPointAtHead( Tpoint *p, DWORD now )
{
	// ignore time passed in (for now)
	now = TheTimeManager.GetClockTime();

	TrailElement *t = new TrailElement( p, now );
	ShiAssert( t );

	// Store the age of the old head before we add the new one
	if (head) {
		head->time = now - head->time;
	}
	
	// Update the trail head pointer
	t->next = head;
	head = t;

	// Update the location of this object
	position = *p;
}


/***************************************************************************\
    Rewinds a trail backwards based on time value.  Needed for ACMI.
\***************************************************************************/
int DrawableTrail::RewindTrail( DWORD now )
{
	TrailElement *t;
	DWORD htime;
	int numremoved = 0;

	if ( !head )
		return numremoved;

	// get last absolute time
	htime = head->time;

	// nothing to rewind
	if ( head->time <= now )
		return numremoved;

	// get next seg and delete head
	t = head;
	head = head->next;
	t->next = NULL;
	delete t;
	numremoved++;

	// loop thru remainded checking times
	while ( head )
	{
		htime -= head->time;
		if ( htime > now )
		{
			t = head;
			head = head->next;
			t->next = NULL;
			delete t;
			numremoved++;
			continue;
		}
		head->time = htime;
		position = head->point;
		break;
	}

	return numremoved;
	
}



/***************************************************************************\
    Cut the trail off after the specified number of points.
\***************************************************************************/
void DrawableTrail::TrimTrail( int len )
{
	int				i = 0;
	TrailElement*	t = head;

	if (len == 0) {
		delete head;
		head = NULL;
	} else {
		while (t) {
			i++;
			if (i == len) {
				delete t->next;	// Recursivly deletes the rest of the trail
				t->next = NULL;
				break;
			}
			t = t->next;
		}
	}
}



/***************************************************************************\
    Draw this segmented trail on the given renderer.
\***************************************************************************/
//void DrawableTrail::Draw( class RenderOTW *renderer, int LOD )
void DrawableTrail::Draw( class RenderOTW *renderer, int)
{
	ThreeDVertex		v0, v1, v2, v3;
	TrailElement		*current;
	float				alpha;
	float				tile;
	int					i = 0;
	int					dT;
	ThreeDVertex		vS, vE;
	Tpoint				cpos, cend;
	int					lineColor = 0;
	float 				width1, width2;
	float				lightIntensity;
	float				dx, dy;

	gTextured = FALSE;

	ShiAssert( type >= 0 );
	ShiAssert( type < sizeof(types) / sizeof(TrailTypeEntry) );

	// If we don't have a least two points, we have nothing to do
	if ((!head) || (!head->next)) {
		return;
	}


	// Set up our drawing mode
	if (renderer->GetObjectTextureState() && types[type].tex && !sGreenMode )
	{
		gTextured = TRUE;

		// Do texturing
		if ( renderer->GetAlphaMode() )
		{
//			renderer->context.RestoreState( STATE_ALPHA_TEXTURE_GOURAUD_TRANSPARENCY_PERSPECTIVE );
			renderer->context.RestoreState( STATE_ALPHA_TEXTURE_GOURAUD_PERSPECTIVE );
		}
		else
		{
			renderer->context.RestoreState( STATE_TEXTURE_GOURAUD_TRANSPARENCY_PERSPECTIVE );
		}

		if (renderer->GetFilteringMode())
		{
			renderer->context.SetState( MPR_STA_ENABLES, MPR_SE_FILTERING );
			renderer->context.SetState( MPR_STA_TEX_FILTER, MPR_TX_BILINEAR );
			renderer->context.InvalidateState();
		}

		if (renderer->GetDitheringMode())
		{
			renderer->context.SetState( MPR_STA_ENABLES, MPR_SE_DITHERING );
			renderer->context.InvalidateState();
		}

		renderer->context.SelectTexture( types[type].tex->TexHandle() );
	}
	else
	{
		if (renderer->GetSmoothShadingMode())
		{
			renderer->context.RestoreState( STATE_ALPHA_GOURAUD );
		}
		else
		{
			renderer->context.RestoreState( STATE_SOLID );
		}
	}


	// Start at the head of the trail
	current	= head;
	dT = (TheTimeManager.GetClockTime() - head->time);
	alpha	= types[type].a 
			  - types[type].disipation * dT;
	if (alpha < 0.001f) {
		if ( keepStaleSegs == FALSE )
		{
			delete head;
			head = NULL;
		}
		return;
	}

	radius	= types[type].startRadius;
	lightIntensity	= types[type].initLight - types[type].lightFade * dT;
	if ( lightIntensity > 1.0f )
		lightIntensity = 1.0f;

	// Compute the screen space location of the starting corners
	// (Note:  since we're passing in the start and end points backward, we swap left
	//  and right in the output)
	cpos.x = current->next->point.x - renderer->X();
	cpos.y = current->next->point.y - renderer->Y();
	cpos.z = current->next->point.z - renderer->Z();
	cend.x = current->point.x - renderer->X();
	cend.y = current->point.y - renderer->Y();
	cend.z = current->point.z - renderer->Z();
	ConstructSegmentEnd( renderer, &cpos, &cend, &v2, &v3 );
	dx = (float)fabs( v3.x - v2.x );
	dy = (float)fabs( v3.y - v2.y );
	if ( dx > dy )
		width2 = 2.0f * dx + dy;
	else
		width2 = 2.0f * dy + dx;

	// transform the actual point of the segment
	renderer->TransformCameraCentricPoint( &cend,  &vE  );

	// Construct MPR vertices as if we had already drawn to them to prime the pump...
	// vE.r = v3.r = v2.r = types[type].rLite;
	// vE.g = v3.g = v2.g = types[type].gLite;
	// vE.b = v3.b = v2.b = types[type].bLite;
	if ( gTextured )
	{
		vE.a = alpha;
		v3.a = v2.a = 0.0f;
		if ( types[type].selfIlum )
		{
			vE.r = 1.0f;
			vE.g = 1.0f;
			vE.b = 1.0f;
			v3.r = v2.r = 0.1f * NRANDPOS * 0.8f;
			v3.g = v2.g = 0.1f * NRANDPOS * 0.8f;
			v3.b = v2.b = 0.1f * NRANDPOS * 0.8f;
		}
		else
		{
			vE.r = max( gLight.r, lightIntensity );
			vE.g = max( gLight.g, lightIntensity );
			vE.b = max( gLight.b, lightIntensity );
			v3.r = v2.r = vE.r;
			v3.g = v2.g = vE.g;
			v3.b = v2.b = vE.b;
		}
	}
	else if ( sGreenMode )
	{
		vE.r = 0.0f;
		vE.g = (types[type].gLite + types[type].rLite + types[type].bLite) * 0.33f;
		vE.b = 0.0f;
		vE.a = alpha;
		v3.r = v2.r = 0.0f;
		v3.g = v2.g = vE.g;
		v3.b = v2.b = 0.0f;
		v3.a = v2.a = 0.0f;
	}
	else
	{
		vE.r = types[type].rLite;
		vE.g = types[type].gLite;
		vE.b = types[type].bLite;
		vE.a = alpha;
		v3.r = v2.r = types[type].rLite;
		v3.g = v2.g = types[type].gLite;
		v3.b = v2.b = types[type].bLite;
		v3.a = v2.a = 0.0f;
	}

	tile = types[type].tileAmt;

	if ( width2 < SEG_LOD_VAL )
		v2.a = v3.a = vE.a;

	if ( width2 >= HIGH_LOD_VAL )
	{
		float aStep = 0.7f/(float)(NUM_HIGH_LOD_VERTS>>2);
		float a = 0.0f;
		for ( i = 0; i < NUM_HIGH_LOD_VERTS; i++ )
		{

			gV1[i].r = vE.r;
			gV1[i].g = vE.g;
			gV1[i].b = vE.b;
			gV1[i].a = vE.a * a * a;
			a += aStep;
			if ( a > 0.7f || a < 0.0f )
			{
				aStep = -aStep;
				a += aStep;
			}

			if ( gV1[i].clipFlag & CLIP_NEAR )
			{
				gV1[i].a = 0.0f;
			}

		}
		gV1[0].a = 0.0f;
		gV1[HALF_HIGH_LOD_VERTS].a = 0.0f;
	}


	// Continue as long as we have another segment to which to connect
	while (current->next) {

		// Move last times end points into the starting point position
		v1 = v3;
		v0 = v2;
		vS = vE;
		width1 = width2;
		if ( width1 >= HIGH_LOD_VAL )
		{
			for ( i = 0; i < NUM_HIGH_LOD_VERTS; i++ )
			{
				gV0[i] = gV1[i];
			}
		}

		// Update the alpha value for the end point now under consideration
		if (type >= 0 && type < sizeof(types) / sizeof(TrailTypeEntry) && !F4IsBadReadPtr(current->next, sizeof(TrailElement))) // JB 010220 CTD
			// Somehow the next line can CTD.  Wacky!  Let's do more checks and see if the CTD moves.
			alpha  -= types[type].disipation * current->next->time;

		radius	+= types[type].expandRate * current->next->time;
		radius = min( radius, types[type].maxRadius );

		if ( gTextured && !types[type].selfIlum )
		{
			if ( !types[type].selfIlum )
			{
				lightIntensity -= types[type].lightFade * current->next->time;
				vE.r = max( gLight.r, lightIntensity );
				vE.g = max( gLight.g, lightIntensity );
				vE.b = max( gLight.b, lightIntensity );
				v3.r = v2.r = vE.r;
				v3.g = v2.g = vE.g;
				v3.b = v2.b = vE.b;
			}
			else
			{
				v3.r = v2.r = 0.1f * NRANDPOS * 0.8f;
				v3.g = v2.g = 0.1f * NRANDPOS * 0.8f;
				v3.b = v2.b = 0.1f * NRANDPOS * 0.8f;
			}
		}

		// Compute the screen space location of the corners of the end of this segment
		cpos.x = current->next->point.x - renderer->X();
		cpos.y = current->next->point.y - renderer->Y();
		cpos.z = current->next->point.z - renderer->Z();
		cend.x = current->point.x - renderer->X();
		cend.y = current->point.y - renderer->Y();
		cend.z = current->point.z - renderer->Z();
		ConstructSegmentEnd( renderer, &cend, &cpos, &v3, &v2 );


		dx = (float)fabs( v3.x - v2.x );
		dy = (float)fabs( v3.y - v2.y );
		if ( dx > dy )
			width2 = 2.0f * dx + dy;
		else
			width2 = 2.0f * dy + dx;

		if ( width2 >= HIGH_LOD_VAL )
		{
			float aStep = 0.7f/(float)(NUM_HIGH_LOD_VERTS>>2);
			float a = 0.0f;
			for ( i = 0; i < NUM_HIGH_LOD_VERTS; i++ )
			{
				gV1[i].r = vE.r;
				gV1[i].g = vE.g;
				gV1[i].b = vE.b;
				gV1[i].a = vE.a * a * a;
				a += aStep;
				if ( a > 0.7f || a < 0.0f )
				{
					aStep = -aStep;
					a += aStep;
				}
				if ( gV1[i].clipFlag & CLIP_NEAR )
				{
					gV1[i].a = 0.0f;
				}
	
			}
			gV1[0].a = 0.0f;
			gV1[HALF_HIGH_LOD_VERTS].a = 0.0f;
		}

		if ( width2 >= SEG_LOD_VAL )
		{
			// transform the actual point of the segment
			renderer->TransformCameraCentricPoint( &cpos,  &vE  );
		}

		// just draw a line?
		if ( gTextured == FALSE && width1 < 0.9f && width2 < 0.9f )
		{
			// TODO: lineColor should be precalc'd class member(?)
			if ( lineColor == 0 )
			{
				if ( !sGreenMode )
				{
					lineColor =	((unsigned int)(alpha*255.0f) << 24) +	// alpha
						((unsigned int)(types[type].bLite*255.0f) << 16) +	// blue
						((unsigned int)(types[type].gLite*255.0f) << 8)  +	// green
						((unsigned int)(types[type].rLite*255.0f));		 	// red
				}
				else
				{
					vE.g = (types[type].gLite + types[type].rLite + types[type].bLite) * 0.33f;
					lineColor =	((unsigned int)(alpha*255.0f) << 24) +	// alpha
						((unsigned int)(vE.g*255.0f) << 8); // green
				}
			}

			renderer->SetColor( lineColor );
			renderer->Render3DLine( &current->point, &current->next->point );
			if (alpha < 0.001f) {
				ShiAssert( current->next );
				if ( keepStaleSegs == FALSE )
				{
					if (current->next && current->next->next && !F4IsBadReadPtr(current->next, sizeof(TrailElement)) && !F4IsBadReadPtr(current->next->next, sizeof(TrailElement))) // JB 010220 CTD
						delete current->next->next;	// Recursivly deletes the rest of the trail
					current->next->next = NULL;	// Terminate the trail at the current point
				}
				alpha = 0.001f;				// Clamp alpha to not less than 0
			}
			current = current->next;
			continue;
		}


		// Stop drawing and trim the trail if alpha gets to be less than zero
		if (alpha < 0.001f) {
			ShiAssert( current->next );
			if ( keepStaleSegs == FALSE )
			{
				if (!F4IsBadWritePtr(current->next, sizeof(TrailElement))) // JB 010222 CTD
					delete current->next->next;	// Recursivly deletes the rest of the trail
				current->next->next = NULL;	// Terminate the trail at the current point
			}
			alpha = 0.001f;				// Clamp alpha to not less than 0
		}

		// Update the MPR verticies for the two new end points
		// v3.a = alpha;
		// v2.a = alpha;

		// last segement has 0 alpha at end
		if ( current->next->next == NULL )
			vE.a = 0.0f;
		// outright hack for missile trails
		else if ( type == 16 )
		{
			// 2nd point
			if ( i & 1 )
				vE.a = alpha * 0.2f;
			else
				vE.a = alpha;
			i++;
		}
		else
			vE.a = alpha;

		// Advance to the next segment for next time arround
		current = current->next;

		// Set the segment texture coordinates
		v0.u = 0.0f;	v0.v = 1.0f;	v0.q = v0.csZ * 0.001f;
		v1.u = 0.0f;	v1.v = 0.0f;	v1.q = v1.csZ * 0.001f;
		v2.u = tile;	v2.v = 1.0f;	v2.q = v2.csZ * 0.001f;
		v3.u = tile;	v3.v = 0.0f;	v3.q = v3.csZ * 0.001f;
		vS.u = 0.0f;	vS.v = 0.5f;	vS.q = vS.csZ * 0.001f;
		vE.u = tile;	vE.v = 0.5f;	vE.q = vE.csZ * 0.001f;

		if ( width2 >= HIGH_LOD_VAL )
		{
			float aStep = 0.7f/(float)(NUM_HIGH_LOD_VERTS>>2);
			float a = 0.0f;
			for ( i = 0; i < NUM_HIGH_LOD_VERTS; i++ )
			{
				gV1[i].r = vE.r;
				gV1[i].g = vE.g;
				gV1[i].b = vE.b;
				gV1[i].a = vE.a * a * a;
				a += aStep;
				if ( a > 0.7f || a < 0.0f )
				{
					aStep = -aStep;
					a += aStep;
				}
				if ( gV1[i].clipFlag & CLIP_NEAR )
				{
					gV1[i].a = 0.0f;
				}
	
			}
			gV1[0].a = 0.0f;
			gV1[HALF_HIGH_LOD_VERTS].a = 0.0f;
		}


// HACK HACK HACK!  Take this out when Marc fixes MPR for patch 1.
#if 0
		{
			DWORD color;

			color = 0xFF000000 |
				((FloatToInt32(vS.r * 255.9f) & 0xFF))			|
				((FloatToInt32(vS.g * 255.9f) & 0xFF) << 8)		|
				((FloatToInt32(vS.b * 255.9f) & 0xFF) << 16);

			renderer->context.SelectForegroundColor( ~color );
			renderer->context.SelectForegroundColor( color );
		}
#endif
// END HACK

		if ( gHeadOn == TRUE )
		{
			gVHead1[4].a = vE.a;

			if ( gVHead1[4].a < 0.0f )
				gVHead1[4].a = 0.0f;

			if ( gVHead1[4].a > 1.0f )
				gVHead1[4].a = 1.0f;

			gVHead1[0].q = gVHead1[0].csZ * 0.001f;
			gVHead1[0].r = vE.r;
			gVHead1[0].g = vE.g;
			gVHead1[0].b = vE.b;
			gVHead1[1].q = gVHead1[1].csZ * 0.001f;
			gVHead1[1].r = vE.r;
			gVHead1[1].g = vE.g;
			gVHead1[1].b = vE.b;
			gVHead1[2].q = gVHead1[2].csZ * 0.001f;
			gVHead1[2].r = vE.r;
			gVHead1[2].g = vE.g;
			gVHead1[2].b = vE.b;
			gVHead1[3].q = gVHead1[3].csZ * 0.001f;
			gVHead1[3].r = vE.r;
			gVHead1[3].g = vE.g;
			gVHead1[3].b = vE.b;
			gVHead1[4].q = gVHead1[4].csZ * 0.001f;
			gVHead1[4].r = vE.r;
			gVHead1[4].g = vE.g;
			gVHead1[4].b = vE.b;

			renderer->context.SelectTexture( gAplTextures[types[type].headOnTex].TexHandle() );

			renderer->DrawTriangle( &gVHead1[4], &gVHead1[0], &gVHead1[1], CULL_ALLOW_ALL );
			renderer->DrawTriangle( &gVHead1[4], &gVHead1[1], &gVHead1[2], CULL_ALLOW_ALL );
			renderer->DrawTriangle( &gVHead1[4], &gVHead1[2], &gVHead1[3], CULL_ALLOW_ALL );
			renderer->DrawTriangle( &gVHead1[4], &gVHead1[3], &gVHead1[0], CULL_ALLOW_ALL );

			renderer->context.SelectTexture( types[type].tex->TexHandle() );
		}
		else if ( width2 >= HIGH_LOD_VAL && gTextured && !sGreenMode )
		{
			if (  width1 >= HIGH_LOD_VAL )
			{
				float vStep = 1.0f/(float)HALF_HIGH_LOD_VERTS;
				float v = 0.0;

				for ( i = 0; i < NUM_HIGH_LOD_VERTS; i++ )
				{
					if ( i == HALF_HIGH_LOD_VERTS )
					{
						v = 1.0f;
						vStep = -vStep;
					}
					gV0[i].u = 0.0f;	gV0[i].v = v;	gV0[i].q = gV0[i].csZ * 0.001f;
					gV1[i].u = tile;	gV1[i].v = v;	gV1[i].q = gV1[i].csZ * 0.001f;
					v += vStep;

				}
				for ( i = 0; i < NUM_HIGH_LOD_VERTS-1; i++ )
				{
					renderer->DrawTriangle( &gV0[i], &gV1[i], &gV1[i+1], CULL_ALLOW_ALL );
					renderer->DrawTriangle( &gV0[i], &gV0[i+1], &gV1[i+1], CULL_ALLOW_ALL );
				}
				renderer->DrawTriangle( &gV0[i], &gV1[i], &gV1[0], CULL_ALLOW_ALL );
				renderer->DrawTriangle( &gV0[i], &gV0[0], &gV1[0], CULL_ALLOW_ALL );
			}
			else
			{
				float vStep = 1.0f/(float)HALF_HIGH_LOD_VERTS;
				float v = 0.0;

				for ( i = 0; i < NUM_HIGH_LOD_VERTS; i++ )
				{
					if ( i == HALF_HIGH_LOD_VERTS )
					{
						v = 1.0f;
						vStep = -vStep;
					}
					gV1[i].u = tile;	gV1[i].v = v;	gV1[i].q = gV1[i].csZ * 0.001f;
					v += vStep;

				}

				renderer->DrawTriangle( &gV1[0], &vS, &v1, CULL_ALLOW_ALL );
				for ( i = 0; i < NUM_HIGH_LOD_VERTS; i++ )
				{
					renderer->DrawTriangle( &gV1[i], &vS, &gV1[(i+1) % NUM_HIGH_LOD_VERTS], CULL_ALLOW_ALL );
				}
				renderer->DrawTriangle( &gV1[HALF_HIGH_LOD_VERTS], &vS, &v0, CULL_ALLOW_ALL );

			}

		}
		else if ( width1 >= HIGH_LOD_VAL && gTextured && !sGreenMode )
		{
			float vStep = 1.0f/(float)HALF_HIGH_LOD_VERTS;
			float v = 0.0;

			for ( i = 0; i < NUM_HIGH_LOD_VERTS; i++ )
			{
				if ( i == HALF_HIGH_LOD_VERTS )
				{
					v = 1.0f;
					vStep = -vStep;
				}
				gV1[i].u = 0.0f;	gV1[i].v = v;	gV1[i].q = gV1[i].csZ * 0.001f;
				v += vStep;

			}

			gV0[0].q = gV0[0].csZ * 0.001f;

			renderer->DrawTriangle( &gV0[0], &vE, &v3, CULL_ALLOW_ALL );

			for ( i = 0; i < NUM_HIGH_LOD_VERTS; i++ )
			{
				gV0[i].q = gV0[i].csZ * 0.001f;
				gV0[(i+1)%NUM_HIGH_LOD_VERTS].q = gV0[(i+1)%NUM_HIGH_LOD_VERTS].csZ * 0.001f;
				vE.q = vE.csZ * 0.001f;

				renderer->DrawTriangle( &gV0[i], &vE, &gV0[(i+1) % NUM_HIGH_LOD_VERTS], CULL_ALLOW_ALL );
			}
			renderer->DrawTriangle( &gV0[HALF_HIGH_LOD_VERTS], &vE, &v2, CULL_ALLOW_ALL );
		}
		else if ( width2 < SEG_LOD_VAL )
		{
			v2.a = v3.a = vE.a;
			if ( width1 < SEG_LOD_VAL )
			{
				renderer->DrawTriangle( &v0, &v1, &v3, CULL_ALLOW_ALL );
				renderer->DrawTriangle( &v0, &v3, &v2, CULL_ALLOW_ALL );
			}
			else
			{
				renderer->DrawTriangle( &vS, &v1, &v3, CULL_ALLOW_ALL );
				renderer->DrawTriangle( &vS, &v3, &v2, CULL_ALLOW_ALL );
				renderer->DrawTriangle( &vS, &v0, &v2, CULL_ALLOW_ALL );
			}
		}
		else
		{
			v3.a = v2.a =  0.0f;
			if ( width1 < SEG_LOD_VAL )
			{
				renderer->DrawTriangle( &v1, &v3, &vE, CULL_ALLOW_ALL );
				renderer->DrawTriangle( &v1, &v0, &vE, CULL_ALLOW_ALL );
				renderer->DrawTriangle( &v0, &v2, &vE, CULL_ALLOW_ALL );
			}
			else
			{
				renderer->DrawTriangle( &vS, &v1, &v3, CULL_ALLOW_ALL );
				renderer->DrawTriangle( &vS, &v3, &vE, CULL_ALLOW_ALL );
				renderer->DrawTriangle( &vS, &v0, &v2, CULL_ALLOW_ALL );
				renderer->DrawTriangle( &vS, &v2, &vE, CULL_ALLOW_ALL );
			}
		}
	}
}



/***************************************************************************\
    Help function to compute the transformed locations of the corners of
	a segment end given the world space location of the end point
\***************************************************************************/
void DrawableTrail::ConstructSegmentEnd( RenderOTW *renderer, Tpoint *start, Tpoint *end, ThreeDVertex *xformLeft, ThreeDVertex *xformRight )
{
	Tpoint	left, right;
	Tpoint	UP, AT, LEFT, LOOK;
	float	dx, dy;
	float	widthX, widthY, widthZ;
	float	mag, normalizer;
	int i;

	gHeadOn = FALSE;
	
	// Get the vector from the eye to the trail segment in world space
	// DOV.x = end->x - renderer->X();
	// DOV.y = end->y - renderer->Y();
	// DOV.z = end->z - renderer->Z();

	// Compute the direction of this segment in world space
	AT.x = end->x - start->x;
	AT.y = end->y - start->y;
	AT.z = end->z - start->z;

	if ( AT.x == 0.0f && AT.y == 0.0f && AT.z == 0.0f )
	{
	 	renderer->GetAt( &AT );
	}
	
	// Compute the cross product of the two vectors
	// widthX = DOV.y * dz - DOV.z * dy;
	// widthY = DOV.z * dx - DOV.x * dz;
	// widthZ = DOV.x * dy - DOV.y * dx;
	widthX = end->y * AT.z - end->z * AT.y;
	widthY = end->z * AT.x - end->x * AT.z;
	widthZ = end->x * AT.y - end->y * AT.x;

	// Compute the magnitude of the cross product result
	mag = (float)sqrt( widthX*widthX + widthY*widthY + widthZ*widthZ );

	// If the cross product was degenerate (parallel vectors), use the "up" vector
	if (mag < 0.00001f) {
	 	renderer->GetUp( &UP );
		widthX = UP.x;
		widthY = UP.y;
		widthZ = UP.z;
		// mag = (float)sqrt( widthX*widthX + widthY*widthY + widthZ*widthZ );
		mag = 1.0f;
	}
	else
	{
		UP.x = widthX/mag;
		UP.y = widthY/mag;
		UP.z = widthZ/mag;
	}

	// Normalize the width vector, then scale it to 1/2 of the total width of the segment
	normalizer = scale * radius / mag;
	widthX *= normalizer;
	widthY *= normalizer;
	widthZ *= normalizer;


	// Compute the world space location of the two corners at the end of this segment
	left.x  = end->x - widthX;
	left.y  = end->y - widthY;
	left.z  = end->z - widthZ;
	right.x = end->x + widthX;
	right.y = end->y + widthY;
	right.z = end->z + widthZ;

	// Transform the two new corners
	renderer->TransformCameraCentricPoint( &left,  xformLeft  );
	renderer->TransformCameraCentricPoint( &right, xformRight );

	if ( !gTextured || sGreenMode )
		return;

	// test for highest LOD
	dx = (float)fabs(xformLeft->x - xformRight->x);
	dy = (float)fabs(xformLeft->y - xformRight->y);
	if ( dx > dy )
		dx = 2.0f * dx + dy;
	else
		dx = 2.0f * dy + dx;

	if ( dx < HIGH_LOD_VAL )
		return;

	// normalize vector down segment
	mag = (float)sqrt( AT.x*AT.x + AT.y*AT.y + AT.z*AT.z );
	AT.x /= mag;
	AT.y /= mag;
	AT.z /= mag;

	// get left vector relative to the UP vector and vector down
	// segment (don't need to normalize cuz AT and UP are at 90 deg
	// to each other)
	LEFT.x = UP.y * AT.z - UP.z * AT.y;
	LEFT.y = UP.z * AT.x - UP.x * AT.z;
	LEFT.z = UP.x * AT.y - UP.y * AT.x;

	// now transform the circle points on the UP and LEFT plane
	for ( i = 0; i < HALF_HIGH_LOD_VERTS; i++ )
	{
		widthX = gCircleVerts[i].y * LEFT.x + gCircleVerts[i].z * UP.x;
		widthY = gCircleVerts[i].y * LEFT.y + gCircleVerts[i].z * UP.y;
		widthZ = gCircleVerts[i].y * LEFT.z + gCircleVerts[i].z * UP.z;
	
		widthX *= scale *radius * 0.85f;
		widthY *= scale *radius * 0.85f;
		widthZ *= scale *radius * 0.9f;
	
		left.x  = end->x - widthX;
		left.y  = end->y - widthY;
		left.z  = end->z - widthZ;
		right.x = end->x + widthX;
		right.y = end->y + widthY;
		right.z = end->z + widthZ;
	
		// Transform the two new corners
		renderer->TransformCameraCentricPoint( &left,  &gV1[i]  );
		renderer->TransformCameraCentricPoint( &right, &gV1[i+HALF_HIGH_LOD_VERTS] );
	}

	// test for head-on....
	if ( dx < HIGH_LOD_VAL * 2.3f)
		return;

	// 1st: is the camera look vector very coincident with the seg vector
 	renderer->GetAt( &LOOK );
	mag = AT.x * LOOK.x + AT.y * LOOK.y + AT.z * LOOK.z;
	if ( fabs( mag ) < 0.87f )
		return;

	// 2nd: how close are we to the seg line?
	mag = end->x * AT.x + end->y * AT.y + end->z * AT.z;
	left.x = end->x - AT.x * mag;
	left.y = end->y - AT.y * mag;
	left.z = end->z - AT.z * mag;
	mag = ( left.x * left.x + left.y * left.y + left.z * left.z );
	if ( mag > scale * radius * scale * radius * 2.0f)
		return;

	Tpoint newend;
	float mag2;

	// we don't want to near clip, check start and end
	mag = end->x * LOOK.x + end->y * LOOK.y + end->z * LOOK.z;
	mag2 = start->x * LOOK.x + start->y * LOOK.y + start->z * LOOK.z;
	if ( mag < 1.0f && mag2 < 1.0f )
		return;

	if ( mag < 20.0f )
	{
		newend.x = end->x - LOOK.x * (mag-20.0f) + LOOK.x * 5.0f;
		newend.y = end->y - LOOK.y * (mag-20.0f) + LOOK.y * 5.0f;
		newend.z = end->z - LOOK.z * (mag-20.0f) + LOOK.z * 5.0f;
	}
	else
	{
		newend = *end;
	}

	

	// we're head on.....
	gHeadOn = TRUE;
 	renderer->GetUp( &UP );
 	renderer->GetLeft( &LEFT );

	newend.x += UP.x * NRAND * 3.0f;
	newend.y += UP.y * NRAND * 3.0f;
	newend.z += UP.z * NRAND * 3.0f;
	newend.x += LEFT.x * NRAND * 3.0f;
	newend.y += LEFT.y * NRAND * 3.0f;
	newend.z += LEFT.z * NRAND * 3.0f;

	renderer->TransformCameraCentricPoint( &newend,  &gVHead1[4]  );

	widthX = UP.x * scale * radius * 1.8f;
	widthY = UP.y * scale * radius * 1.8f;
	widthZ = UP.z * scale * radius * 1.8f;

	left.x  = newend.x - widthX;
	left.y  = newend.y - widthY;
	left.z  = newend.z - widthZ;
	right.x = newend.x + widthX;
	right.y = newend.y + widthY;
	right.z = newend.z + widthZ;

	renderer->TransformCameraCentricPoint( &left,  &gVHead1[0]  );
	renderer->TransformCameraCentricPoint( &right,  &gVHead1[2]  );

	widthX = LEFT.x * scale * radius * 1.8f;
	widthY = LEFT.y * scale * radius * 1.8f;
	widthZ = LEFT.z * scale * radius * 1.8f;

	left.x  = newend.x - widthX;
	left.y  = newend.y - widthY;
	left.z  = newend.z - widthZ;
	right.x = newend.x + widthX;
	right.y = newend.y + widthY;
	right.z = newend.z + widthZ;

	renderer->TransformCameraCentricPoint( &left,  &gVHead1[1]  );
	renderer->TransformCameraCentricPoint( &right,  &gVHead1[3]  );


}




/***************************************************************************\
    This function is called from the miscellanious texture loader function.
	It must be hardwired into that function.
\***************************************************************************/
//void DrawableTrail::SetupTexturesOnDevice( DWORD rc )
void DrawableTrail::SetupTexturesOnDevice(DXContext *rc)
{
	float alp;
	int intalp;
	int j;
	int r,g,b;

	float rad, radstep;
	rad = 0.0f;
	radstep = PI / (float)HALF_HIGH_LOD_VERTS;
	for ( j = 0; j < HALF_HIGH_LOD_VERTS; j++ )
	{
		gCircleVerts[j].y = -(float)sin(rad);
		gCircleVerts[j].z = (float)cos(rad);
		rad += radstep;
	}

	gVHead1[0].u = 0.0f;
	gVHead1[0].v = 0.0f;

	gVHead1[1].u = 1.0f;
	gVHead1[1].v = 0.0f;

	gVHead1[2].u = 1.0f;
	gVHead1[2].v = 1.0f;

	gVHead1[3].u = 0.0f;
	gVHead1[3].v = 1.0f;

	gVHead1[4].u = 0.5f;
	gVHead1[4].v = 0.5f;

	gVHead1[0].a = 0.0f;
	gVHead1[1].a = 0.0f;
	gVHead1[2].a = 0.0f;
	gVHead1[3].a = 0.0f;


	// Load our textures
	MissleTrailTexture.LoadAndCreate( "MisTrail.gif", MPR_TI_CHROMAKEY | MPR_TI_PALETTE | MPR_TI_ALPHA );
	for ( j = 0; j < 256; j++ )
	{
		intalp = (MissleTrailTexture.palette->paletteData[j] >> 24);
		MissleTrailTexture.palette->paletteData[j] &= 0x00ffffff;
		if ( j == 0 )
			continue;
		alp = (float)intalp;
		alp = alp * 0.3f + alp * 0.7f * NRANDPOS;
		intalp = FloatToInt32( alp );
		r = (MissleTrailTexture.palette->paletteData[j] & 0x000000ff);
		g = (MissleTrailTexture.palette->paletteData[j] & 0x0000ff00) >> 8;
		b = (MissleTrailTexture.palette->paletteData[j] & 0x00ff0000) >> 16;
		intalp = ( r + b + g )/3;

		/*
		alp = (float)r;
		alp = alp * 0.6f + alp * 0.4f * NRANDPOS;
		r = FloatToInt32( alp );
		alp = (float)g;
		alp = alp * 0.6f + alp * 0.4f * NRANDPOS;
		g = FloatToInt32( alp );
		alp = (float)b;
		alp = alp * 0.6f + alp * 0.4f * NRANDPOS;
		b = FloatToInt32( alp );
		*/
		MissleTrailTexture.palette->paletteData[j] = ( intalp << 24 ) |
													 ( b      << 16 ) |
													 ( g      << 8  ) | r ;
	}
	MissleTrailTexture.palette->UpdateMPR( MissleTrailTexture.palette->paletteData );

	FireTrailTexture.LoadAndCreate( "FireTrail.gif", MPR_TI_CHROMAKEY | MPR_TI_PALETTE| MPR_TI_ALPHA  );
	for ( j = 0; j < 256; j++ )
	{
		intalp = (FireTrailTexture.palette->paletteData[j] >> 24);
		FireTrailTexture.palette->paletteData[j] &= 0x00ffffff;
		if ( j == 0 )
			continue;
		alp = (float)intalp;
		alp = alp * 0.3f + alp * 0.7f * NRANDPOS;
		intalp = FloatToInt32( alp );
		r = (FireTrailTexture.palette->paletteData[j] & 0x000000ff);
		g = (FireTrailTexture.palette->paletteData[j] & 0x0000ff00) >> 8;
		b = (FireTrailTexture.palette->paletteData[j] & 0x00ff0000) >> 16;
		intalp = ( r + b + g )/3;

		/*
		alp = (float)r;
		alp = alp * 0.9f + alp * 0.1f * NRANDPOS;
		r = FloatToInt32( alp );
		alp = (float)g;
		alp = alp * 0.5f + alp * 0.3f * NRANDPOS;
		g = FloatToInt32( alp );
		alp = (float)b;
		alp = alp * 0.5f + alp * 0.3f * NRANDPOS;
		b = FloatToInt32( alp );
		*/

		FireTrailTexture.palette->paletteData[j] = ( intalp << 24 ) |
													 ( b      << 16 ) |
													 ( g      << 8  ) | r ;
	}
	FireTrailTexture.palette->UpdateMPR( FireTrailTexture.palette->paletteData );

	SmokeTrailTexture.LoadAndCreate( "SmokeTrail.gif", MPR_TI_CHROMAKEY | MPR_TI_PALETTE| MPR_TI_ALPHA  );
	for ( j = 0; j < 256; j++ )
	{
		intalp = (SmokeTrailTexture.palette->paletteData[j] >> 24);
		SmokeTrailTexture.palette->paletteData[j] &= 0x00ffffff;
		if ( j == 0 )
			continue;
		alp = (float)intalp;
		alp = alp * 0.3f + alp * 0.7f * NRANDPOS;
		intalp = FloatToInt32( alp );
		r = (SmokeTrailTexture.palette->paletteData[j] & 0x000000ff);
		g = (SmokeTrailTexture.palette->paletteData[j] & 0x0000ff00) >> 8;
		b = (SmokeTrailTexture.palette->paletteData[j] & 0x00ff0000) >> 16;

		// the draker the smoke, the higher the alpha
		// normalize from 0.0 to 1.0f
		alp = (255.0f - ( r + b + g )/3.0f)/255.0f;
		intalp = 100 + FloatToInt32( 85.0f * alp );
		SmokeTrailTexture.palette->paletteData[j] = ( intalp << 24 ) |
													 ( b      << 16 ) |
													 ( g      << 8  ) | r ;
	}
	SmokeTrailTexture.palette->UpdateMPR( SmokeTrailTexture.palette->paletteData );

	GunTrailTexture.LoadAndCreate( "GunTrail.apl", MPR_TI_CHROMAKEY | MPR_TI_PALETTE );
	for ( j = 0; j < 256; j++ )
	{
		intalp = (GunTrailTexture.palette->paletteData[j] >> 24);
		GunTrailTexture.palette->paletteData[j] &= 0x00ffffff;
		if ( j == 0 )
			continue;
		alp = (float)intalp;
		alp = alp * 0.3f + alp * 0.7f * NRANDPOS;
		intalp = FloatToInt32( alp );
		GunTrailTexture.palette->paletteData[j] |= ( intalp << 24 );
	}
	GunTrailTexture.palette->UpdateMPR( GunTrailTexture.palette->paletteData );


	// Intialize the lite colors for all trail types (handles those which aren't lite)
	for (int i = 0; i<nTypes; i++) {
		types[i].rLite = types[i].r, types[i].gLite = types[i].g, types[i].bLite = types[i].b;
	}

	// Initialize the lighting conditions and register for future time of day updates
	TimeUpdateCallback( NULL );
	TheTimeManager.RegisterTimeUpdateCB( TimeUpdateCallback, NULL );
}



/***************************************************************************\
    This function is called from the miscellanious texture cleanup function.
	It must be hardwired into that function.
\***************************************************************************/
//void DrawableTrail::ReleaseTexturesOnDevice( DWORD rc )
void DrawableTrail::ReleaseTexturesOnDevice(DXContext *rc)
{
	// Stop receiving time updates
	TheTimeManager.ReleaseTimeUpdateCB( TimeUpdateCallback, NULL );

	// Release our textures
	MissleTrailTexture.FreeAll();
	FireTrailTexture.FreeAll();
	SmokeTrailTexture.FreeAll();
	GunTrailTexture.FreeAll();
}



/***************************************************************************\
    Update the light level on the smoke and vapor trails.
	NOTE:  Since the textures are static, this function can also
		   be static, so the self parameter is ignored.
\***************************************************************************/
//void DrawableTrail::TimeUpdateCallback( void *self )
void DrawableTrail::TimeUpdateCallback( void *)
{
	int		i;

	// Get the light level from the time of day manager
	TheTimeOfDay.GetTextureLightingColor( &gLight );

	// Update the non-self illuminating colors in our type array
	for (i=0; i<nLiteTypes; i++) {
		liteTypes[i]->rLite = liteTypes[i]->r * gLight.r;
		liteTypes[i]->gLite = liteTypes[i]->g * gLight.g;
		liteTypes[i]->bLite = liteTypes[i]->b * gLight.b;
	}

	// Update all the textures which aren't self illuminating
	/*
	for (i=0; i<nLiteTextures; i++) {
		liteTextures[i]->palette->LightTexturePalette( &light );
	}
	*/
}
