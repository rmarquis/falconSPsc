/***************************************************************************\
    DrawSgmt.h
    Scott Randolph
    May 3, 1996

    Derived class to handle drawing segmented trails (like contrails).
\***************************************************************************/
#ifndef _DRAWSGMT_H_
#define _DRAWSGMT_H_

#include "DrawObj.h"
#include "Falclib\Include\IsBad.h"

#ifdef USE_SH_POOLS
#include "SmartHeap\Include\smrtheap.h"
#endif

#include "mpr_light.h"

// Used to draw segmented trails (like missile trails)
class TrailElement {
#ifdef USE_SH_POOLS
  public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert( size == sizeof(TrailElement) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(TrailElement), 100, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif
  public:
	TrailElement( Tpoint *p, DWORD now )	{ point = *p; time = now; };
	~TrailElement()							
	{ 
		if (!F4IsBadWritePtr(next, sizeof(TrailElement))) // JB 010304 CTD
		{	delete next; next = NULL; }
	};

	Tpoint			point;		// World space location of this point on the trail
	DWORD			time;		// For head this is insertion time, for others its deltaT

	TrailElement	*next;
};

enum TrailType {
    TRAIL_CONTRAIL, //0
	TRAIL_VORTEX, //1
	TRAIL_AIM120, //2
	TRAIL_TRACER1, //3
	TRAIL_TRACER2, //4
	TRAIL_SMOKE, //5
	TRAIL_FIRE1, //6
	TRAIL_EXPPIECE, //7
	TRAIL_THINFIRE, //8
	TRAIL_DISTMISSILE, //9
	TRAIL_DUST, //10
	TRAIL_GUN, //11
	TRAIL_LWING, //12
	TRAIL_RWING, //13
	TRAIL_ROCKET, //14
	TRAIL_MISLSMOKE, //15
	TRAIL_MISLTRAIL, //16
	TRAIL_DARKSMOKE, //17
	TRAIL_FIRE2, //18
	TRAIL_WINGTIPVTX, //19
	TRAIL_MAX
};

class DrawableTrail : public DrawableObject {
#ifdef USE_SH_POOLS
  public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert( size == sizeof(DrawableTrail) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(DrawableTrail), 40, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif
  public:
	DrawableTrail( int trailType, float scale = 1.0f );
	virtual ~DrawableTrail();

	void	AddPointAtHead( Tpoint *p, DWORD now );
	int		RewindTrail( DWORD now );
	void	TrimTrail( int len );
	void	KeepStaleSegs( BOOL val ) { keepStaleSegs = val; };

	virtual void Draw( class RenderOTW *renderer, int LOD );

	TrailElement*	GetHead()	{ return head; };

  protected:
	int					type;
	TrailElement		*head;
	BOOL				keepStaleSegs;	// for ACMI

  protected:
	void ConstructSegmentEnd( RenderOTW *renderer, 
							  Tpoint *start, Tpoint *end, 
							  struct ThreeDVertex *xformLeft, ThreeDVertex *xformRight );

	// Handle time of day notifications
	static void TimeUpdateCallback( void *unused );

  public:
	static void SetupTexturesOnDevice( DXContext *rc );
	static void ReleaseTexturesOnDevice( DXContext *rc );
};

#endif // _DRAWSGMT_H_
