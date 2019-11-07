/***************************************************************************\
    StateStack.cpp
    Scott Randolph
    February 9, 1998

    Provides state maintenance for the object hierarchy.
\***************************************************************************/
#include "stdafx.h"
#include <math.h>
#include "ColorBank.h"
#include "ObjectInstance.h"
#include "ClipFlags.h"
#include "StateStack.h"
#include "mpr_light.h"
#include "FalcLib\include\playerop.h"

extern "C" {
	void MonoPrint( char *string, ... );
}

#define PERSP_CORR_RADIUS_MULTIPLIER	3.0f

extern bool g_bSlowButSafe;
extern int g_nFogRenderState;

StateStackClass		TheStateStack;

int verts = 0;

/********************************************\
	Create the global storage for our static
	members.
\********************************************/
TransformFp	StateStackClass::Transform;
float		StateStackClass::LODRange;
float		StateStackClass::LODBiasInv;
float		StateStackClass::fogValue;
float		StateStackClass::fogValue_inv;
Pcolor		StateStackClass::fogColor_premul;
Pmatrix		StateStackClass::Rotation;
Ppoint		StateStackClass::Xlation;
Ppoint		StateStackClass::ObjSpaceEye;
Ppoint		StateStackClass::ObjSpaceLight;
ContextMPR	*StateStackClass::context;
const int	*StateStackClass::CurrentTextureTable;
Ppoint		*StateStackClass::XformedPosPool;
Pintensity	*StateStackClass::IntensityPool;
PclipInfo	*StateStackClass::ClipInfoPool;
Ppoint		*StateStackClass::XformedPosPoolNext;
Pintensity	*StateStackClass::IntensityPoolNext;
PclipInfo	*StateStackClass::ClipInfoPoolNext;
class ObjectInstance	*StateStackClass::CurrentInstance;
const class ObjectLOD	*StateStackClass::CurrentLOD;
StateStackFrame			StateStackClass::stack[MAX_STATE_STACK_DEPTH];
int			StateStackClass::stackDepth;
float		StateStackClass::hAspectWidthCorrection;
float		StateStackClass::hAspectDepthCorrection;
float		StateStackClass::vAspectWidthCorrection;
float		StateStackClass::vAspectDepthCorrection;
float		StateStackClass::scaleX;
float		StateStackClass::scaleY;
float		StateStackClass::shiftX;
float		StateStackClass::shiftY;
Pmatrix		*StateStackClass::Tt;
Pmatrix		*StateStackClass::Tb;
float		StateStackClass::LightAmbient	= 0.5f;
float		StateStackClass::LightDiffuse	= 0.5f;
Ppoint		StateStackClass::LightVector	= { 0.0f, 0.0f, -LightDiffuse };  // Prescale!



/********************************************\
	Reserved storage space for computed values.
\********************************************/
static Ppoint		XformedPosPoolBuffer[ MAX_VERT_POOL_SIZE ];
static Pintensity	IntensityPoolBuffer[ MAX_VERT_POOL_SIZE ];
static PclipInfo	ClipInfoPoolBuffer[ MAX_VERT_POOL_SIZE ];


/********************************************\
	Functions used to maintain the states
	above.
\********************************************/
StateStackClass::StateStackClass()
{
	stackDepth			= 0;
	XformedPosPoolNext	= XformedPosPoolBuffer;
	IntensityPoolNext	= IntensityPoolBuffer;
	ClipInfoPoolNext	= ClipInfoPoolBuffer;
	LODBiasInv			= 1.0f;
	SetTextureState( TRUE );
	SetFog( 0.0f, NULL );
}


void StateStackClass::SetContext( ContextMPR *cntxt )
{
	context = cntxt;
}


void StateStackClass::SetLight( float a, float d, Ppoint *atLight )
{
	LightAmbient = a;
	LightDiffuse = d;

	// Prescale the light vector with the diffuse multiplier
	LightVector.x = atLight->x * d;
	LightVector.y = atLight->y * d;
	LightVector.z = atLight->z * d;

	// Intialize the light postion in world space
	ObjSpaceLight = LightVector;
}


void StateStackClass::SetCameraProperties( float ooTanHHAngle, float ooTanVHAngle, float sclx, float scly, float shftx, float shfty )
{
	float	rx2;

	rx2 = (ooTanHHAngle*ooTanHHAngle);
	hAspectDepthCorrection = 1.0f / (float)sqrt(rx2 + 1.0f);
	hAspectWidthCorrection = rx2 * hAspectDepthCorrection;

	rx2 = (ooTanVHAngle*ooTanVHAngle);
	vAspectDepthCorrection = 1.0f / (float)sqrt(rx2 + 1.0f);
	vAspectWidthCorrection = rx2 * vAspectDepthCorrection;

	scaleX = sclx;
	scaleY = scly;
	shiftX = shftx;
	shiftY = shfty;
}

void StateStackClass::SetTextureState(BOOL state)
{
	if(!PlayerOptions.bFilteredObjects)
	{
		// OW original code

		if(state)
		{
			RenderStateTableNoFogPC			= RenderStateTableWithPCTex;
			RenderStateTableNoFogNPC		= RenderStateTableWithNPCTex;
			DrawPrimNoFogNoClipJumpTable	= DrawPrimNoClipWithTexJumpTable;
			ClipPrimNoFogJumpTable			= ClipPrimWithTexJumpTable;

			RenderStateTableFogPC			= RenderStateTableFogWithPCTex;
			RenderStateTableFogNPC			= RenderStateTableFogWithNPCTex;
			DrawPrimFogNoClipJumpTable		= DrawPrimFogNoClipWithTexJumpTable;
			ClipPrimFogJumpTable			= ClipPrimFogWithTexJumpTable;
		}

		else
		{
			RenderStateTableNoFogPC			= RenderStateTableNoTex;
			RenderStateTableNoFogNPC		= RenderStateTableNoTex;
			DrawPrimNoFogNoClipJumpTable	= DrawPrimNoClipNoTexJumpTable;
			ClipPrimNoFogJumpTable			= ClipPrimNoTexJumpTable;

			RenderStateTableFogPC			= RenderStateTableNoTex;
			RenderStateTableFogNPC			= RenderStateTableNoTex;
			DrawPrimFogNoClipJumpTable		= DrawPrimFogNoClipNoTexJumpTable;
			ClipPrimFogJumpTable			= ClipPrimFogNoTexJumpTable;
		}

		if(fogValue <= 0.0f)
		{
			RenderStateTablePC		= RenderStateTableNoFogPC;
			RenderStateTableNPC		= RenderStateTableNoFogNPC;
			DrawPrimNoClipJumpTable = DrawPrimNoFogNoClipJumpTable;
			ClipPrimJumpTable		= ClipPrimNoFogJumpTable;
		}

		else
		{
			RenderStateTablePC		= RenderStateTableFogPC;
			RenderStateTableNPC		= RenderStateTableFogNPC;
			DrawPrimNoClipJumpTable = DrawPrimFogNoClipJumpTable;
			ClipPrimJumpTable		= ClipPrimFogJumpTable;
		}
	}

	else	// OW
	{
		if(state)
		{
			RenderStateTableNoFogPC			= RenderStateTableWithPCTex_Filter;
			RenderStateTableNoFogNPC		= RenderStateTableWithNPCTex_Filter;
			DrawPrimNoFogNoClipJumpTable	= DrawPrimNoClipWithTexJumpTable;
			ClipPrimNoFogJumpTable			= ClipPrimWithTexJumpTable;

			RenderStateTableFogPC			= RenderStateTableFogWithPCTex_Filter;
			RenderStateTableFogNPC			= RenderStateTableFogWithNPCTex_Filter;
			DrawPrimFogNoClipJumpTable		= DrawPrimFogNoClipWithTexJumpTable;
			ClipPrimFogJumpTable			= ClipPrimFogWithTexJumpTable;
		}

		else
		{
			RenderStateTableNoFogPC			= RenderStateTableNoTex_Filter;
			RenderStateTableNoFogNPC		= RenderStateTableNoTex_Filter;
			DrawPrimNoFogNoClipJumpTable	= DrawPrimNoClipNoTexJumpTable;
			ClipPrimNoFogJumpTable			= ClipPrimNoTexJumpTable;

			RenderStateTableFogPC			= RenderStateTableNoTex_Filter;
			RenderStateTableFogNPC			= RenderStateTableNoTex_Filter;
			DrawPrimFogNoClipJumpTable		= DrawPrimFogNoClipNoTexJumpTable;
			ClipPrimFogJumpTable			= ClipPrimFogNoTexJumpTable;
		}

		if(fogValue <= 0.0f)
		{
			RenderStateTablePC		= RenderStateTableNoFogPC;
			RenderStateTableNPC		= RenderStateTableNoFogNPC;
			DrawPrimNoClipJumpTable = DrawPrimNoFogNoClipJumpTable;
			ClipPrimJumpTable		= ClipPrimNoFogJumpTable;
		}

		else
		{
			RenderStateTablePC		= RenderStateTableFogPC;
			RenderStateTableNPC		= RenderStateTableFogNPC;
			DrawPrimNoClipJumpTable = DrawPrimFogNoClipJumpTable;
			ClipPrimJumpTable		= ClipPrimFogJumpTable;
		}
	}
}


void StateStackClass::SetFog( float percent, Pcolor *color )
{
	ShiAssert( percent <= 1.0f );

	fogValue = percent;

	if (percent <= 0.0f) {
		// Turn fog off
		RenderStateTablePC		= RenderStateTableNoFogPC;
		RenderStateTableNPC		= RenderStateTableNoFogNPC;
		DrawPrimNoClipJumpTable = DrawPrimNoFogNoClipJumpTable;
		ClipPrimJumpTable		= ClipPrimNoFogJumpTable;

	} else {
		// Turn fog on
		RenderStateTablePC		= RenderStateTableFogPC;
		RenderStateTableNPC		= RenderStateTableFogNPC;
		DrawPrimNoClipJumpTable = DrawPrimFogNoClipJumpTable;
		ClipPrimJumpTable		= ClipPrimFogJumpTable;

		// We store the color pre-scaled by the percent of fog to save time later.
		ShiAssert( color );
		fogColor_premul.r = color->r * percent;
		fogColor_premul.g = color->g * percent;
		fogColor_premul.b = color->b * percent;

		// We store 1-percent to save time later.
		fogValue_inv	= 1.0f - percent;

// SCR 6/30/98:  We properly should set MPR's notion of the fog color here
// but it's a little messy to use a context here, and besides,
// Falcon can count on the RendererOTW to set it (right?)
// Therefore:  this should be normally off (turned on only for testing)
#if 1
		if (g_nFogRenderState & 0x02)
		{
			UInt32 c;
			c  = FloatToInt32(color->r * 255.9f);
			c |= FloatToInt32(color->g * 255.9f) << 8;
			c |= FloatToInt32(color->b * 255.9f) << 16;
			context->SetState( MPR_STA_FOG_COLOR, c );
		}
#endif
	}
}


void StateStackClass::SetCamera( const Ppoint *pos, const Pmatrix *rotWaspect, Pmatrix *Bill, Pmatrix *Tree )
{
	ShiAssert(stackDepth == 0);

	// Store our rotation from world to camera space (including aspect scale effects)
	Rotation = *rotWaspect;	

	// Compute the vector from the camera to the origin rotated into camera space
	Xlation.x = -pos->x * Rotation.M11 - pos->y * Rotation.M12 - pos->z * Rotation.M13;
	Xlation.y = -pos->x * Rotation.M21 - pos->y * Rotation.M22 - pos->z * Rotation.M23;
	Xlation.z = -pos->x * Rotation.M31 - pos->y * Rotation.M32 - pos->z * Rotation.M33;

	// Intialize the eye postion in world space
	ObjSpaceEye = *pos;

	// Store pointers out to the billboard and tree matrices in case we need them
	Tb = Bill;
	Tt = Tree;
}


void StateStackClass::CompoundTransform( const Pmatrix *rot, const Ppoint *pos )
{
	Ppoint	tempP, tempP2;
	Pmatrix	tempM;

	// Compute the rotated translation vector for this object
	Xlation.x += pos->x * Rotation.M11 + pos->y * Rotation.M12 + pos->z * Rotation.M13;
	Xlation.y += pos->x * Rotation.M21 + pos->y * Rotation.M22 + pos->z * Rotation.M23;
	Xlation.z += pos->x * Rotation.M31 + pos->y * Rotation.M32 + pos->z * Rotation.M33;

	tempM = Rotation;
	tempP.x = ObjSpaceEye.x - pos->x;
	tempP.y = ObjSpaceEye.y - pos->y;
	tempP.z = ObjSpaceEye.z - pos->z;
	tempP2 = ObjSpaceLight;

	// Composit the camera matrix with the object rotation
	MatrixMult( &tempM, rot, &Rotation );

	// Compute the eye point in object space
	MatrixMultTranspose( rot, &tempP, &ObjSpaceEye );

	// Compute the light direction in object space.
	MatrixMultTranspose( rot, &tempP2, &ObjSpaceLight );
}


// The asymetric scale factors MUST be <= 1.0f.
// The global scale factor can be any positive value.
// The effects of the scales are multiplicative.
static const UInt32	OP_NONE	= 0;
static const UInt32	OP_FOG	= 1;
static const UInt32	OP_WARP	= 2;
inline void StateStackClass::pvtDrawObject( UInt32 operation, ObjectInstance *objInst, const Pmatrix *rot, const Ppoint *pos, const float sx, const float sy, const float sz, const float scale )
{
	UInt32 clipFlag;

	static int
		in = 0;

	int
		LODused;

	float
		MaxLODRange;

	ShiAssert( objInst );

	PushAll();

	// Set up our transformations
	CompoundTransform( rot, pos );

	// Special code to impose an asymetric warp on the object
	// (Will screw up if any child transforms are encountered)
	if (operation & OP_WARP)
	{
		// Put the stretch into the transformation matrix
		ShiAssert( (sx > 0.0f) && (sx <= 1.0f) );
		ShiAssert( (sy > 0.0f) && (sy <= 1.0f) );
		ShiAssert( (sz > 0.0f) && (sz <= 1.0f) );
		Pmatrix	tempM;
		Pmatrix	stretchM = {	sx,   0.0f, 0.0f,
								0.0f, sy,   0.0f,
								0.0f, 0.0f, sz };
		tempM = Rotation;
		MatrixMult( &tempM, &stretchM, &Rotation );
	}

	// Special code to impose a scale on an object
	if (scale != 1.0f)
	{
		register float inv = 1.0f / scale;
		Xlation.x		*= inv;
		Xlation.y		*= inv;
		Xlation.z		*= inv;
		ObjSpaceEye.x	*= inv;
		ObjSpaceEye.y	*= inv;
		ObjSpaceEye.z	*= inv;
	}

	// Store the adjusted range for LOD determinations
	LODRange = Xlation.x * LODBiasInv;

	// Choose the appropriate LOD of the object to be drawn
	CurrentInstance = objInst;

	if (objInst->ParentObject)
	{
		if (g_bSlowButSafe && F4IsBadCodePtr((FARPROC) objInst->ParentObject)) // JB 010220 CTD (too much CPU)
			CurrentLOD = 0; // JB 010220 CTD
		else // JB 010220 CTD
		if (objInst->id < 0 || objInst->id >= TheObjectListLength || objInst->TextureSet < 0) // JB 010705 CTD second try
		{
			ShiAssert(FALSE);
			CurrentLOD = 0;
		}
		else 
			CurrentLOD		= objInst->ParentObject->ChooseLOD(LODRange, &LODused, &MaxLODRange);

		if (CurrentLOD)
		{
			// Decide if we need clipping, or if the object is totally off screen
			clipFlag = CheckBoundingSphereClipping();

			// Continue only if some part of the bounding volume is on screen
			if (clipFlag != OFF_SCREEN)
			{
				// Set the jump pointers to turn on/off clipping
				if (clipFlag == ON_SCREEN)
				{
					Transform = TransformNoClip;
					DrawPrimJumpTable = DrawPrimNoClipJumpTable;
				}
				else
				{
					Transform = TransformWithClip;
					DrawPrimJumpTable = DrawPrimWithClipJumpTable;
				}

				// Choose perspective correction or not
				if ((Xlation.x > CurrentInstance->Radius() * PERSP_CORR_RADIUS_MULTIPLIER) && 
					!(CurrentLOD->flags & ObjectLOD::PERSP_CORR))
				{
					RenderStateTable = RenderStateTableNPC;
				}
				else
				{
					RenderStateTable = RenderStateTablePC;
				}

				in ++;

				if (in == 1)
				{
					verts = 0;
				}

				// Draw the object
				CurrentLOD->Draw();

//				if (in == 1)
//				{
//					if (verts)
//					{
//						MonoPrint ("Obj %d:%d %d : %d\n", objInst->id, LODused, (int) MaxLODRange, verts);
//					}
//				}

				in --;
			}
		}
	}

	PopAll();
}


// This call is for the application to call to draw an instance of an object.
void StateStackClass::DrawObject( ObjectInstance *objInst, const Pmatrix *rot, const Ppoint *pos, const float scale )
{
	pvtDrawObject( OP_NONE, 
					objInst, 
					rot,	pos, 
					1.0f,	1.0f, 	1.0f, 
					scale );
}


// This call is for the BSPlib to call to draw a child instance attached to a slot.
void StateStackClass::DrawSubObject( ObjectInstance *objInst, const Pmatrix *rot, const Ppoint *pos )
{
	pvtDrawObject( OP_NONE, 
					objInst, 
					rot,	pos, 
					1.0f,	1.0f, 	1.0f, 
					1.0f );
}


// This call is rather specialized.  It is intended for use in drawing shadows which  
// are simple objects (no slots, dofs, etc) but require asymetric scaling in x and y 
// to simulate orientation changes of the object casting the shadow.
void StateStackClass::DrawWarpedObject( ObjectInstance *objInst, const Pmatrix *rot, const Ppoint *pos, const float sx, const float sy, const float sz, const float scale )
{
	pvtDrawObject( OP_WARP, 
					objInst, 
					rot,	pos, 
					sx,		sy, 	sz, 
					scale );
}


inline UInt32 StateStackClass::CheckBoundingSphereClipping(void)
{
	// Decide if we need clipping, or if the object is totally off screen
	// REMEMBER:  Xlation is camera oriented, but still X front, Y right, Z down
	//			  so range from viewer is in the X term.
	// NOTE:  We compute "d", the distance from the viewer at which the bounding
	//		  sphere should intersect the view frustum.  We use .707 since the
	//		  rotation matrix already normalized us to a 45 degree half angle.
	//		  We do have to adjust the radius shift by the FOV correction factors,
	//		  though, since it didn't go through the matix.
	// NOTE2: We could develop the complete set of clip flags here by continuing to 
	//        check other edges instead of returning in the clipped cases.  For now,
	//        we only need to know if it IS clipped or not, so we terminate early.
	// TODO:  We should roll the radius of any attached slot children into the check radius
	//		  to ensure that we don't reject a parent object whose _children_ may be on screen.
	//        (though this should be fairly rare in practice)
	float	rd;
	float	rh;
//	UInt32	clipFlag = ON_SCREEN;

	rd = CurrentInstance->Radius() * vAspectDepthCorrection;
	rh = CurrentInstance->Radius() * vAspectWidthCorrection;
	if (-(Xlation.z - rh) >= Xlation.x - rd) {
		if (-(Xlation.z + rh) > Xlation.x + rd) {
			return OFF_SCREEN;			// Trivial reject top
		}
//		clipFlag = CLIP_TOP;
		return CLIP_TOP;
	}
	if (Xlation.z + rh >= Xlation.x - rd) {
		if (Xlation.z - rh > Xlation.x + rd) {
			return OFF_SCREEN;			// Trivial reject bottom
		}
//		clipFlag |= CLIP_BOTTOM;
		return CLIP_BOTTOM;
	}

	rd = CurrentInstance->Radius() * hAspectDepthCorrection;
	rh = CurrentInstance->Radius() * hAspectWidthCorrection;
	if (-(Xlation.y - rh) >= Xlation.x - rd) {
		if (-(Xlation.x + rh) > Xlation.x + rd) {
			return OFF_SCREEN;			// Trivial reject left
		}
//		clipFlag |= CLIP_LEFT;
		return CLIP_LEFT;
	}
	if (Xlation.y + rh >= Xlation.x - rd) {
		if (Xlation.y - rh > Xlation.x + rd) {
			return OFF_SCREEN;			// Trivial reject right
		}
//		clipFlag |= CLIP_RIGHT;
		return CLIP_RIGHT;
	}

	rh = CurrentInstance->Radius();
	if (Xlation.x - rh < NEAR_CLIP_DISTANCE) {
		if (Xlation.x + rh < NEAR_CLIP_DISTANCE) {
			return OFF_SCREEN;			// Trivial reject near
		}
//		clipFlag |= CLIP_NEAR;
		return CLIP_NEAR;
	}

//	return clipFlag;
	return ON_SCREEN;
}


void StateStackClass::PushAll( void )
{
	ShiAssert( stackDepth < MAX_STATE_STACK_DEPTH );

	stack[stackDepth].XformedPosPool		= XformedPosPool;
	stack[stackDepth].IntensityPool			= IntensityPool;
	stack[stackDepth].ClipInfoPool			= ClipInfoPool;

	stack[stackDepth].Rotation				= Rotation;
	stack[stackDepth].Xlation				= Xlation;
	
	stack[stackDepth].ObjSpaceEye			= ObjSpaceEye;
	stack[stackDepth].ObjSpaceLight			= ObjSpaceLight;
	
	stack[stackDepth].CurrentInstance		= CurrentInstance;
	stack[stackDepth].CurrentLOD			= CurrentLOD;
	stack[stackDepth].CurrentTextureTable	= CurrentTextureTable;

	stack[stackDepth].DrawPrimJumpTable		= DrawPrimJumpTable;
	stack[stackDepth].Transform				= Transform;

	XformedPosPool							= XformedPosPoolNext;
	IntensityPool							= IntensityPoolNext;
	ClipInfoPool							= ClipInfoPoolNext;

	stackDepth++;
}


void StateStackClass::PopAll(void)
{
	stackDepth--;

	XformedPosPoolNext	= XformedPosPool;
	IntensityPoolNext	= IntensityPool;
	ClipInfoPoolNext	= ClipInfoPool;

	XformedPosPool		= stack[stackDepth].XformedPosPool;
	IntensityPool		= stack[stackDepth].IntensityPool;
	ClipInfoPool		= stack[stackDepth].ClipInfoPool;

	Rotation			= stack[stackDepth].Rotation;
	Xlation				= stack[stackDepth].Xlation;
	
	ObjSpaceEye			= stack[stackDepth].ObjSpaceEye;
	ObjSpaceLight		= stack[stackDepth].ObjSpaceLight;
	
	CurrentInstance		= stack[stackDepth].CurrentInstance;
	CurrentLOD			= stack[stackDepth].CurrentLOD;
	CurrentTextureTable	= stack[stackDepth].CurrentTextureTable;

	DrawPrimJumpTable	= stack[stackDepth].DrawPrimJumpTable;
	Transform			= stack[stackDepth].Transform;
}


void StateStackClass::PushVerts( void )
{
	ShiAssert( stackDepth < MAX_STATE_STACK_DEPTH );

	stack[stackDepth].XformedPosPool	= XformedPosPool;
	stack[stackDepth].IntensityPool		= IntensityPool;
	stack[stackDepth].ClipInfoPool		= ClipInfoPool;

	XformedPosPool						= XformedPosPoolNext;
	IntensityPool						= IntensityPoolNext;
	ClipInfoPool						= ClipInfoPoolNext;

	stackDepth++;
}


void StateStackClass::PopVerts(void)
{
	stackDepth--;

	XformedPosPoolNext	= XformedPosPool;
	IntensityPoolNext	= IntensityPool;
	ClipInfoPoolNext	= ClipInfoPool;

	XformedPosPool		= stack[stackDepth].XformedPosPool;
	IntensityPool		= stack[stackDepth].IntensityPool;
	ClipInfoPool		= stack[stackDepth].ClipInfoPool;
}


void StateStackClass::Light( const Pnormal *p, int n )
{
	// Make sure we've got enough room in the light value pool
	ShiAssert( IsValidIntensityIndex( n-1 ) );
	
	while( n-- ) {
		// light intensity = ambient + diffuse*(lightVector dot normal)
		// and we already scaled the lightVector by the diffuse intensity.
		*IntensityPoolNext = p->i*ObjSpaceLight.x + p->j*ObjSpaceLight.y + p->k*ObjSpaceLight.z;
		if (*IntensityPoolNext > 0.0f) {
			*IntensityPoolNext += LightAmbient;
		} else {
			*IntensityPoolNext = LightAmbient;
		}

		p++;
		IntensityPoolNext++;
	}
}


inline void StateStackClass::TransformInline( Ppoint *p, int n, const BOOL clip )
{
	float	scratch_x, scratch_y, scratch_z;

	// Make sure we've got enough room in the transformed position pool
	ShiAssert( IsValidPosIndex( n-1 ) );

	while( n-- ) {
		scratch_z = Rotation.M11 * p->x + Rotation.M12 * p->y + Rotation.M13 * p->z + Xlation.x;
		scratch_x = Rotation.M21 * p->x + Rotation.M22 * p->y + Rotation.M23 * p->z + Xlation.y;
		scratch_y = Rotation.M31 * p->x + Rotation.M32 * p->y + Rotation.M33 * p->z + Xlation.z;


		if (clip) {
			ClipInfoPoolNext->clipFlag = ON_SCREEN;

			if ( scratch_z < NEAR_CLIP_DISTANCE ) {
				ClipInfoPoolNext->clipFlag |= CLIP_NEAR;
			}

			if ( fabs(scratch_y) > scratch_z ) {
				if ( scratch_y > scratch_z ) {
					ClipInfoPoolNext->clipFlag |= CLIP_BOTTOM;
				} else {
					ClipInfoPoolNext->clipFlag |= CLIP_TOP;
				}
			}

			if ( fabs(scratch_x) > scratch_z ) {
				if ( scratch_x > scratch_z ) {
					ClipInfoPoolNext->clipFlag |= CLIP_RIGHT;
				} else {
					ClipInfoPoolNext->clipFlag |= CLIP_LEFT;
				}
			}

			ClipInfoPoolNext->csX = scratch_x;
			ClipInfoPoolNext->csY = scratch_y;
			ClipInfoPoolNext->csZ = scratch_z;

			ClipInfoPoolNext++;
		}


		register float OneOverZ = 1.0f/scratch_z;
		p++;

		XformedPosPoolNext->z = scratch_z;
		XformedPosPoolNext->x = XtoPixel( scratch_x * OneOverZ );
		XformedPosPoolNext->y = YtoPixel( scratch_y * OneOverZ );
		XformedPosPoolNext++;
	}
}


void StateStackClass::TransformNoClip( Ppoint *p, int n )
{
	TransformInline( p, n, FALSE );
}


void StateStackClass::TransformWithClip( Ppoint *p, int n )
{
	// TODO:  Need to make sure we're not going to walk off the end...
	TransformInline( p, n, TRUE );
}


void StateStackClass::TransformBillboardWithClip( Ppoint *p, int n, BTransformType type )
{
	float	scratch_x, scratch_y, scratch_z;
	Pmatrix	*T;

	// Make sure we've got enough room in the transformed position pool
	ShiAssert( IsValidPosIndex( n-1 ) );

	if (type == Tree) {
		T = Tt;
	} else {
		T = Tb;
	}

	while( n-- ) {
		scratch_z = T->M11 * p->x + T->M12 * p->y + T->M13 * p->z + Xlation.x;
		scratch_x = T->M21 * p->x + T->M22 * p->y + T->M23 * p->z + Xlation.y;
		scratch_y = T->M31 * p->x + T->M32 * p->y + T->M33 * p->z + Xlation.z;

		ClipInfoPoolNext->clipFlag = ON_SCREEN;

		if ( scratch_z < NEAR_CLIP_DISTANCE ) {
			ClipInfoPoolNext->clipFlag |= CLIP_NEAR;
		}

		if ( fabs(scratch_y) > scratch_z ) {
			if ( scratch_y > scratch_z ) {
				ClipInfoPoolNext->clipFlag |= CLIP_BOTTOM;
			} else {
				ClipInfoPoolNext->clipFlag |= CLIP_TOP;
			}
		}

		if ( fabs(scratch_x) > scratch_z ) {
			if ( scratch_x > scratch_z ) {
				ClipInfoPoolNext->clipFlag |= CLIP_RIGHT;
			} else {
				ClipInfoPoolNext->clipFlag |= CLIP_LEFT;
			}
		}

		ClipInfoPoolNext->csX = scratch_x;
		ClipInfoPoolNext->csY = scratch_y;
		ClipInfoPoolNext->csZ = scratch_z;

		ClipInfoPoolNext++;

		register float OneOverZ = 1.0f/scratch_z;
		p++;

		XformedPosPoolNext->z = scratch_z;
		XformedPosPoolNext->x = XtoPixel( scratch_x * OneOverZ );
		XformedPosPoolNext->y = YtoPixel( scratch_y * OneOverZ );
		XformedPosPoolNext++;
	}
}


BOOL StateStackClass::IsValidPosIndex( int i )
{
	return (i+XformedPosPool < XformedPosPoolBuffer+MAX_VERT_POOL_SIZE);
}


BOOL StateStackClass::IsValidIntensityIndex( int i )
{
	return (i+IntensityPool  < IntensityPoolBuffer+MAX_VERT_POOL_SIZE);
}
