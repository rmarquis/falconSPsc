/***************************************************************************\
    StateStack.h
    Scott Randolph
    February 9, 1998

    Provides state maintenance for the object hierarchy.
\***************************************************************************/
#ifndef _STATESTACK_H_
#define _STATESTACK_H_

#include "Matrix.h"
#include "PolyLib.h"
#include "ColorBank.h"
#include "BSPNodes.h"

class ObjectInstance;
class BSubTree;


// The one and only state stack.  This would need to be replaced
// by pointers to instances of StateStackClass passed to each call
// if more than one stack were to be simultaniously maintained.
extern class StateStackClass		TheStateStack;


static const int MAX_STATE_STACK_DEPTH				= 8;	// Arbitrary
static const int MAX_SLOT_AND_DYNAMIC_PER_OBJECT	= 64;	// Arbitrary
static const int MAX_TEXTURES_PER_OBJECT			= 128;	// Arbitrary
static const int MAX_CLIP_PLANES					= 6;	// 5 view volume, plus 1 extra
static const int MAX_VERTS_PER_POLYGON				= 32;	// Arbitrary
static const int MAX_VERT_POOL_SIZE					= 4096;	// Arbitrary
static const int MAX_VERTS_PER_CLIPPED_POLYGON		= MAX_VERTS_PER_POLYGON + MAX_CLIP_PLANES;
static const int MAX_CLIP_VERTS						= 2 * MAX_CLIP_PLANES;
static const int MAX_VERTS_PER_OBJECT_TREE			= MAX_VERT_POOL_SIZE - MAX_VERTS_PER_POLYGON - MAX_CLIP_VERTS;


typedef void (*TransformFp)( Ppoint *p, int n );


typedef struct StateStackFrame {
	Ppoint						*XformedPosPool;
	Pintensity					*IntensityPool;
	PclipInfo					*ClipInfoPool;

	Pmatrix						Rotation;
	Ppoint						Xlation;

	Ppoint						ObjSpaceEye;
	Ppoint						ObjSpaceLight;

	const int 					*CurrentTextureTable;
	class ObjectInstance		*CurrentInstance;
	const class ObjectLOD		*CurrentLOD;

	const DrawPrimFp			*DrawPrimJumpTable;
	TransformFp					Transform;
} StateStackFrame;


class StateStackClass {
  public:
	StateStackClass();
	~StateStackClass()	{ ShiAssert(stackDepth == 0); };

  public:
	// Called by application
	static void SetContext( ContextMPR *cntxt );
	static void	SetLight( float a, float d, Ppoint *v );
	static void SetCameraProperties( float ooTanHHAngle, float ooTanVHAngle, float sclx, float scly, float shftx, float shfty );
	static void SetLODBias( float bias )					{ LODBiasInv = 1.0f/bias; };
	static void SetTextureState( BOOL state );
	static void SetFog( float percent, Pcolor *color );
	static void SetCamera( const Ppoint *pos, const Pmatrix *rotWaspect, Pmatrix *Bill, Pmatrix *Tree );
	static void DrawObject( ObjectInstance *objInst, const Pmatrix *rot, const Ppoint *pos, const float scale=1.0f );
	static void DrawWarpedObject( ObjectInstance *objInst, const Pmatrix *rot, const Ppoint *pos, const float sx, const float sy, const float sz, const float scale=1.0f );

	// Called by BRoot nodes at draw time
	static void DrawSubObject( ObjectInstance *objInst, const Pmatrix *rot, const Ppoint *pos );
	static void	CompoundTransform( const Pmatrix *rot, const Ppoint *pos );
	static void	Light( const Pnormal *pNormals, int nNormals );
	static void SetTextureTable( const int *pTexIDs )		{ CurrentTextureTable = pTexIDs; };
	static void	PushAll(void);
	static void	PopAll(void);
	static void	PushVerts(void);
	static void	PopVerts(void);

	// This should be cleaned up and probably have special clip/noclip versions
	static void TransformBillboardWithClip( Ppoint *p, int n, BTransformType type );

	// Called by our own transformations and the clipper
    static float XtoPixel( float x ) { return (x * scaleX) + shiftX; };
    static float YtoPixel( float y ) { return (y * scaleY) + shiftY; };

	// For parameter validation during debug
	static BOOL IsValidPosIndex( int i );
	static BOOL IsValidIntensityIndex( int i );

  protected:
	static void	TransformNoClip( Ppoint *pCoords, int nCoords );
	static void	TransformWithClip( Ppoint *pCoords, int nCoords );

	static inline DWORD	CheckBoundingSphereClipping(void);
	static inline void	TransformInline( Ppoint *pCoords, int nCoords, const BOOL clip );

	static inline void	pvtDrawObject( DWORD operation, ObjectInstance *objInst, const Pmatrix *rot, const Ppoint *pos, const float sx, const float sy, const float sz, const float scale=1.0f );

  public:
	// Active transformation function (selects between with or without clipping)
	static TransformFp	Transform;

	// Computed data pools
	static Ppoint		*XformedPosPool;	// These point into global storage.  They will point
	static Pintensity	*IntensityPool;		// to the computed tables for each sub-object.
	static PclipInfo	*ClipInfoPool;
	static Ppoint		*XformedPosPoolNext;// These point into global storage.  They will point
	static Pintensity	*IntensityPoolNext;	// to at least MAX_CLIP_VERTS empty slots beyond 
	static PclipInfo	*ClipInfoPoolNext;	// the computed tables in use by the current sub-object.

	// Instance of the object we're drawing and its range normalized for resolution and FOV
	static class ObjectInstance		*CurrentInstance;
	static const class ObjectLOD	*CurrentLOD;
	static const int				*CurrentTextureTable;
	static float					LODRange;

	// Fog properties
	static float		fogValue;			// fog precent (set by SetFog())
	static float		fogValue_inv;		// 1.0f - fog precent (set by SetFog())
	static Pcolor		fogColor_premul;	// Fog color times fog percent (set by SetFog())

	// Final transform
	static Pmatrix		Rotation;			// These are the final camera transform
	static Ppoint		Xlation;			// including contributions from parent objects

	// Fudge factors for drawing
	static float		LODBiasInv;			// This times real range is LOD evaluation range

	// Object space points of interest
	static Ppoint		ObjSpaceEye;		// Eye point in object space (for BSP evaluation)
	static Ppoint		ObjSpaceLight;		// Light location in object space(for BSP evaluation)

	// Pointers to our clients billboard and tree matrices in case we need them
	static Pmatrix		*Tb;	// Billboard (always faces viewer)
	static Pmatrix		*Tt;	// Tree (always stands up straight and faces viewer)

	// Lighting properties for the BSP objects
	static float		LightAmbient;
	static float		LightDiffuse;
	static Ppoint		LightVector;

	// The context on which we'll draw
	static ContextMPR	*context;

  protected:
	static StateStackFrame	stack[MAX_STATE_STACK_DEPTH];
	static int				stackDepth;

	// Required for object culling
	static float		hAspectWidthCorrection;
	static float		hAspectDepthCorrection;
	static float		vAspectWidthCorrection;
	static float		vAspectDepthCorrection;

    // The parameters required to get from normalized screen space to pixel space
	static float	scaleX;
    static float	scaleY;
    static float	shiftX;
    static float	shiftY;
};

#endif // _STATESTACK_H_