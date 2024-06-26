/***************************************************************************\
    DrawBldg.cpp    Scott Randolph
    July 10, 1996

    Derived class to do special position processing for buildings on the
	ground.  (More precisly, any object which is to be placed on the 
	ground but not reoriented.)
\***************************************************************************/
#include "Matrix.h"
#include "RViewPnt.h"
#include "RenderOW.h"
#include "DrawBldg.h"

// edg just testing smoke stacks
/*
#include "stdhdr.h"
#include "otwdrive.h"
#include "sfx.h"
*/

#ifdef USE_SH_POOLS
MEM_POOL	DrawableBuilding::pool;
#endif



/***************************************************************************\
    Initialize a container for a BSP object to be drawn
\***************************************************************************/
DrawableBuilding::DrawableBuilding( int ID, Tpoint *pos, float heading, float s )
: DrawableBSP( s, ID )
{
	float	cosYaw;
	float	sinYaw;

	// Compute the sine and cosine of the objects desired heading
	cosYaw = (float)cos( heading );
	sinYaw = (float)sin( heading );

	// Store this objects properties
	scale = s;
	previousLOD = -1;
	drawClassID = Building;
	
	// Construct the rotation matrix to orient the object correctly
	orientation.M11 = cosYaw,	orientation.M12 = -sinYaw,	orientation.M13 = 0.0f;
	orientation.M21 = sinYaw,	orientation.M22 = cosYaw,	orientation.M23 = 0.0f;
	orientation.M31 = 0.0f,		orientation.M32 = 0.0f,		orientation.M33 = 1.0f;

	// Record our position (Z will be updated later)
	position = *pos;
}



/***************************************************************************\
    Make sure the object is placed on the ground then draw it.
\***************************************************************************/
void DrawableBuilding::Draw( class RenderOTW *renderer, int LOD )
{
	// See if we need to update our ground position
	if (LOD != previousLOD) {

		// Update our position to reflect the terrain beneath us
		position.z = renderer->viewpoint->GetGroundLevel( position.x, position.y );
		previousLOD = LOD;

	}

	// Tell our parent class to draw us now
	DrawableBSP::Draw( renderer, LOD );

	// testing smokestack stuff
	/*
	if ( OTWDriver.IsActive() )
	{
		Tpoint pos, vec, offset;
		int i, num, lodlvl;

		if ( id == 746 )
		{
			vec.x = 0.0f;
			vec.y = 0.0f;
			vec.z = -60.0f;
			lodlvl = ((0xf << LOD) | 0xf);

			num = GetNumSlots();
			for ( i = 0; i < num; i++ )
			{
				if ( ! ( ( rand() & lodlvl) == lodlvl ) )
					continue;

				GetChildOffset( i, &offset );

				pos.x = position.x + offset.x * orientation.M11 + offset.y * orientation.M12;
				pos.y = position.y + offset.x * orientation.M21 + offset.y * orientation.M22;
				pos.z = position.z + offset.z;

				OTWDriver.AddSfxRequest(
					new SfxClass (SFX_FIRESMOKE,				// type
					SFX_MOVES,
					&pos,							// world pos
					&vec,							// world pos
					2.5f,							// time to live
					40.0f ) );		// scale
			}
		}
	}
	*/
}
