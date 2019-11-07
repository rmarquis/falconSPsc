/***************************************************************************\
    DrawOVC.h
    Scott Randolph
    Jan 16, 1997

    Derived class to handle drawing overcast cloud layers.
\***************************************************************************/
#ifndef _DRAWOVC_H_
#define _DRAWOVC_H_

#include "DrawObj.h"
#include "mpr_light.h"


class DrawableOvercast : public DrawableObject {
  public:
	DrawableOvercast( DWORD CloudTileCode, int row, int col );
	virtual ~DrawableOvercast();

	static void SetGreenMode( BOOL greenMode );

	void UpdateForDrift( int row, int col );

	virtual void Draw( class RenderOTW *renderer, int LOD );

	float GetLocalOpacity( Tpoint *point );
	void  GetLocalColor( Tpoint *point, Tcolor *color );
	float GetLocalFuzzDepth( Tpoint *point );

	float	GetTextureAlpha( float x, float y );
	DWORD	GetTileCode(void)	{ return tileCode; };
	float	GetBase(void)		{ return base; };
	float	GetTops(void)		{ return top; };

	float	GetNWtop(void)	{ return NWtop; };
	float	GetNEtop(void)	{ return NEtop; };
	float	GetSWtop(void)	{ return SWtop; };
	float	GetSEtop(void)	{ return SEtop; };

	float	GetNWbottom(void)	{ return NWbottom; };
	float	GetNEbottom(void)	{ return NEbottom; };
	float	GetSWbottom(void)	{ return SWbottom; };
	float	GetSEbottom(void)	{ return SEbottom; };

	static void SetupTexturesOnDevice( DXContext *rc );
	static void ReleaseTexturesOnDevice( DXContext *rc );

  protected:
	DWORD	tileCode;	// Identifies which of the texture tiles to use
	float	base;		// The altitude of the cloud bases
	float	top;		// The altitude of the cloud tops

	float	northEdge;			// World space edge locations for this cell
	float	southEdge;
	float	eastEdge;
	float	westEdge;

	float	NWtop, NWbottom;	// Height of cloud layer on top and bottom
	float	NEtop, NEbottom;
	float	SWtop, SWbottom;
	float	SEtop, SEbottom;

	static Tcolor	litCloudTopColor;		// Basic cloud color on top (shared by all instances)
	static Tcolor	litCloudBottomColor;	// Basic cloud color from below (shared by all instances)

  protected:
	static void	BuildFuzzDepthTable( void );

	// Handle time of day notifications
	static void TimeUpdateCallback( void *unused );


#ifdef USE_SH_POOLS
  public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert( size == sizeof(DrawableOvercast) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(DrawableOvercast), 10, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif
};

#endif // _DRAWOVC_H_