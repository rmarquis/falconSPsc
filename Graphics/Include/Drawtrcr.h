/***************************************************************************\
    DrawTrcr.h
    Derived class to handle drawing tracers (bullets)
\***************************************************************************/
#ifndef _DRAWTRCR_H_
#define _DRAWTRCR_H_

#include "DrawObj.h"


class DrawableTracer : public DrawableObject {
#ifdef USE_SH_POOLS
  public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert( size == sizeof(DrawableTracer) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(DrawableTracer), 200, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif
  public:
	DrawableTracer( void );
	DrawableTracer( float w );
	DrawableTracer( Tpoint *p, float w );
	virtual ~DrawableTracer();

	void Update( Tpoint *head, Tpoint *tail );

	virtual void Draw( class RenderOTW *renderer, int LOD );

	void SetAlpha( float a ) { alpha = a; };
	float GetAlpha( void ) { return alpha; };
	void GetRGB( float *R, float *G, float *B ) { *R = r; *G = g; *B = b; };
	void SetWidth( float w ) { width = w; };
	void SetType( int t ) { type = t; };
	void SetRGB( float R, float G, float B ) { r = R; g = G; b = B; };

  protected:
	BOOL ConstructWidth( RenderOTW *renderer,
						 Tpoint *start,
						 Tpoint *end,
						 struct ThreeDVertex *xformLeft,
						 struct ThreeDVertex *xformRight,
						 struct ThreeDVertex *xformLefte,
						 struct ThreeDVertex *xformRighte );
  	Tpoint tailEnd;
	float width;
	float alpha;
	float r,g,b;
	int type;
		#define TRACER_TYPE_TRACER		0
		#define TRACER_TYPE_BALL		1

};

#endif // _DRAWTRCR_H_
