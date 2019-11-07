/***************************************************************************\
    DrawShdw.h    Scott Randolph
    July 2, 1997

    Derived class to draw BSP'ed objects with shadows (specificly for aircraft)
\***************************************************************************/
#include "Matrix.h"
#include "RViewPnt.h"
#include "RenderOW.h"
#include "DrawShdw.h"

#ifdef USE_SH_POOLS
MEM_POOL	DrawableShadowed::pool;
#endif



/***************************************************************************\
    Initialize a container for a BSP object to be drawn
\***************************************************************************/
DrawableShadowed::DrawableShadowed( int ID, const Tpoint *pos, const Trotation *rot, float s, int ShadowID )
: DrawableBSP( ID, pos, rot, s ), shadowInstance( ShadowID )
{
}



/***************************************************************************\
    Make sure the object is placed on the ground then draw it.
\***************************************************************************/
void DrawableShadowed::Draw( class RenderOTW *renderer, int LOD )
{
	// Draw our shadow if we're close to the ground
	if (position.z > renderer->viewpoint->GetTerrainCeiling()) {	// -Z is up!

		float		yaw;
		float		pitch;
		float		roll;
		float		sinYaw;
		float		cosYaw;
		float		sx;
		float		sy;
		Tpoint		pos;
		Trotation	rot;

		yaw			= (float)atan2( orientation.M21, orientation.M11 );
		pitch		= (float)-asin( orientation.M31 );
		roll		= (float)atan2( orientation.M32, orientation.M33 );
		sinYaw		= (float)sin( yaw );
		cosYaw		= (float)cos( yaw );

		pos.x = position.x;
		pos.y = position.y;
		pos.z = renderer->viewpoint->GetGroundLevel( position.x, position.y );

		sx = max( 0.3f, (float)fabs(cos( pitch )) );
		sy = max( 0.3f, (float)fabs(cos( roll  )) );

		rot.M11 = cosYaw;	rot.M12 = -sinYaw;	rot.M13 = 0.0f;
		rot.M21 = sinYaw;	rot.M22 = cosYaw;	rot.M23 = 0.0f;
		rot.M31 = 0.0f;		rot.M32 = 0.0f;		rot.M33 = 1.0f;

		TheStateStack.DrawWarpedObject( &shadowInstance, &rot, &pos, sx, sy, 1.0f, instance.Radius() );
	}

	// Tell our parent class to draw us now
	DrawableBSP::Draw( renderer, LOD );
}
