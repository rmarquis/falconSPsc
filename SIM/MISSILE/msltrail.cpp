#include "stdhdr.h"
#include "f4error.h"
#include "f4vu.h"
#include "missile.h"
#include "Graphics\Include\drawsgmt.h"
#include "Graphics\Include\drawbsp.h"
#include "Graphics\Include\draw2d.h"
#include "otwdrive.h"
#include "classtbl.h"
#include "sfx.h"
#include "Entity.h"
#include "weather.h"

extern int g_nmissiletrial;
void MissileClass::InitTrail (void)
{
#ifndef MISSILE_TEST_PROG
	Tpoint newPoint;
	Tpoint origPoint;
	Tpoint delta;
	float distSq;
	Trotation rot;
	Falcon4EntityClassType *classPtr;
	WeaponClassDataType *wc;

	newPoint.x = XPos();
	newPoint.y = YPos();
	newPoint.z = ZPos();

	origPoint = newPoint;

    rot.M11 = dmx[0][0];
    rot.M21 = dmx[0][1];
    rot.M31 = dmx[0][2];
    rot.M12 = dmx[1][0];
    rot.M22 = dmx[1][1];
    rot.M32 = dmx[1][2];
    rot.M13 = dmx[2][0];
    rot.M23 = dmx[2][1];
    rot.M33 = dmx[2][2];

	// placement a bit behind the missile
    newPoint.x += dmx[0][0]*-7.0f;
    newPoint.y += dmx[0][1]*-7.0f;
    newPoint.z += dmx[0][2]*-7.0f;

	// edg: Something's going wrong with position of head of missile trail
	// which results in crash in viewpoint.  On the theory that the matrix
	// may be fucked up, let's do some checking, asserts and cover ups...
	delta.x = newPoint.x - origPoint.x;
	delta.y = newPoint.y - origPoint.y;
	delta.z = newPoint.z - origPoint.z;

	distSq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
	// check distance sq.  Should be 7 * 7, but we'll allow more

	if ( distSq > 100.0f )
	{
		// the notification
		ShiWarning( "Something wrong with missile!  Contact Engineer!" );

		// the bandaid
		newPoint = origPoint;
	}




	classPtr = (Falcon4EntityClassType*)EntityType();
// 2002-03-28 MN if we need the engine to model "lift", don't display trails or engine glows...
	wc = (WeaponClassDataType*)classPtr->dataPtr;
	if (wc && (wc->Flags & WEAP_NO_TRAIL))
		return;

// 2002-03-28 MN externalised missile trail types
	int mistrail = 0, misengGlow = 0, misengGlowBSP = 0, misgroundGlow = 0;
	if (auxData)
	{
		mistrail = auxData->mistrail;
		misengGlow = auxData->misengGlow;
		misengGlowBSP = auxData->misengGlowBSP;
		misgroundGlow = auxData->misgroundGlow;
	}

	
	if (mistrail == 0 && misengGlow == 0 && misengGlowBSP == 0 && misgroundGlow == 0)
	{
		// differentiate trails and missile types
		if (g_nmissiletrial)
		{
		 		// biggish trail
	 			trail = new DrawableTrail( g_nmissiletrial );//me123 
	 			engGlow = new Drawable2D( DRAW2D_MISSILE_GLOW, 5.0, &newPoint );
		 		engGlowBSP1 = new DrawableBSP( MapVisId(VIS_MFLAME_L), &newPoint, &rot, 1.0f );
		}
		else if ( classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_ROCKET )
		{
			trail = new DrawableTrail( TRAIL_ROCKET );
			engGlow = new Drawable2D( DRAW2D_MISSILE_GLOW, 2.0, &newPoint );
			engGlowBSP1 = new DrawableBSP( MapVisId(VIS_MFLAME_S), &newPoint, &rot, 1.0f );
		}
		else if ( classPtr->vuClassData.classInfo_[VU_SPTYPE] == SPTYPE_AIM9M ||
				  classPtr->vuClassData.classInfo_[VU_SPTYPE] == SPTYPE_AIM9P ||
				  classPtr->vuClassData.classInfo_[VU_SPTYPE] == SPTYPE_AIM9R )
		{
			// smallish trail
			trail = new DrawableTrail( TRAIL_MISLTRAIL );
			engGlow = new Drawable2D( DRAW2D_MISSILE_GLOW, 5.0, &newPoint );
			engGlowBSP1 = new DrawableBSP( MapVisId(VIS_MFLAME_S), &newPoint, &rot, 1.0f );
		}
		else if (classPtr->vuClassData.classInfo_[VU_SPTYPE] == SPTYPE_AIM120||
			 classPtr->vuClassData.classInfo_[VU_SPTYPE] == SPTYPE_AA12 ||
			 classPtr->vuClassData.classInfo_[VU_SPTYPE] == SPTYPE_AA11 ||
			 classPtr->vuClassData.classInfo_[VU_SPTYPE] == SPTYPE_AA10C 
			 )
		{
			trail = new DrawableTrail( TRAIL_CONTRAIL );;
			engGlow = new Drawable2D( DRAW2D_MISSILE_GLOW, 5.0, &newPoint );
			engGlowBSP1 = new DrawableBSP( MapVisId(VIS_MFLAME_L), &newPoint, &rot, 1.0f );
		}	
		else 
		{
	 		// biggish trail
	 		trail = new DrawableTrail( TRAIL_AIM120 );
	 		engGlow = new Drawable2D( DRAW2D_MISSILE_GLOW, 5.0, &newPoint );
	 		engGlowBSP1 = new DrawableBSP( MapVisId(VIS_MFLAME_L), &newPoint, &rot, 1.0f );
		}
		groundGlow = new Drawable2D( DRAW2D_MISSILE_GROUND_GLOW, 1.0, &newPoint );
	}
	else
	{
		trail = new DrawableTrail( mistrail );
		engGlow = new Drawable2D( misengGlow, 5.0, &newPoint );
		engGlowBSP1 = new DrawableBSP( MapVisId(misengGlowBSP), &newPoint, &rot, 1.0f );
		if (misgroundGlow != -1)
			groundGlow = new Drawable2D( misgroundGlow, 1.0, &newPoint );
	}

	// add 1st point
    OTWDriver.AddTrailHead (trail, newPoint.x, newPoint.y, newPoint.z);
#endif
}

void MissileClass::UpdateTrail (void)
{
#ifndef MISSILE_TEST_PROG
Tpoint newPoint;
Trotation rot = IMatrix;
float radius;
float agl;
Falcon4EntityClassType *classPtr;

   if ( drawPointer )
		radius = drawPointer->Radius();
   else
	    radius = 5.0f;

   classPtr = (Falcon4EntityClassType*)EntityType();
    ShiAssert(classPtr);
   if (trail)
   {
      if (PowerOutput() > 0.25F || g_nmissiletrial)
      {
	      newPoint.x = XPos();
	      newPoint.y = YPos();
	      newPoint.z = ZPos();


		 // engGlow->Update( &newPoint, &((DrawableBSP*)drawPointer)->orientation  );
    	 rot.M11 = dmx[0][0];
    	 rot.M21 = dmx[0][1];
    	 rot.M31 = dmx[0][2];
    	 rot.M12 = dmx[1][0];
    	 rot.M22 = dmx[1][1];
    	 rot.M32 = dmx[1][2];
    	 rot.M13 = dmx[2][0];
    	 rot.M23 = dmx[2][1];
    	 rot.M33 = dmx[2][2];
	     // placement a bit behind the missile
    	 newPoint.x += dmx[0][0]*-radius;
    	 newPoint.y += dmx[0][1]*-radius;
    	 newPoint.z += dmx[0][2]*-radius;
		 engGlow->Update( &newPoint, &rot  );
		 engGlowBSP1->Update( &newPoint, &rot  );
		 newPoint.x += dmx[0][0]*-30.0f;
		 newPoint.y += dmx[0][1]*-30.0f;
		 newPoint.z += dmx[0][2]*-30.0f;

		 // JPO - add contrails if appropriate
		 float objAlt = -ZPos() * 0.001F;
		 bool contrail = false;
		 if (objAlt > ((WeatherClass*)TheWeather)->TodaysConLow &&
			 objAlt < ((WeatherClass*)TheWeather)->TodaysConHigh)
			contrail = true;

		 if (classPtr->vuClassData.classInfo_[VU_SPTYPE] != SPTYPE_AIM120 ||
			 contrail ||
			 g_nmissiletrial)
		     // add new head to trail
		     OTWDriver.AddTrailHead (trail, newPoint.x, newPoint.y, newPoint.z);

		 // do the ground glow if near to ground
		 agl = max ( 0.0f, groundZ - ZPos() );
		 if ( agl < 1000.0f )
		 {
		 	 if ( !groundGlow->InDisplayList() )
		 	 {
				 OTWDriver.InsertObject(groundGlow );
		 	 }
			 // alpha fades with height, radius increases with height
			 groundGlow->SetAlpha( 0.3f * ( 1000.0f - agl ) / 1000.0f );
			 groundGlow->SetRadius( 20.0f + 350.0f * ( agl ) / 1000.0f );
			 newPoint.x = XPos();
			 newPoint.y = YPos();
			 newPoint.z = groundZ;
			 groundGlow->SetPosition( &newPoint );
		 }
		 else if ( groundGlow->InDisplayList() )
		 {
			OTWDriver.RemoveObject(groundGlow, FALSE );
		 }
      }
      else if (trail->GetHead())
      {
		 if ( engGlow->InDisplayList() )
		 {
			OTWDriver.RemoveObject(engGlow, FALSE );
		 }
		 if ( engGlowBSP1->InDisplayList() )
		 {
			OTWDriver.RemoveObject(engGlowBSP1, FALSE );
		 }
		 if ( groundGlow->InDisplayList() )
		 {
			OTWDriver.RemoveObject(groundGlow, FALSE );
		 }
      }
   }
   else if (!IsExploding())
   {
      InitTrail ();
	  if (trail)
		OTWDriver.InsertObject(trail);
	  if (engGlow)
		OTWDriver.InsertObject(engGlow);
	  if (engGlowBSP1)
		OTWDriver.InsertObject(engGlowBSP1);
   }
#endif
}

void MissileClass::RemoveTrail (void)
{
#ifndef MISSILE_TEST_PROG
   if (trail)
   {
      if (OTWDriver.IsActive())
      {
         // hand trail destruction over to sfx driver after a
         // time to live period
         OTWDriver.AddSfxRequest( new SfxClass(20.0f, trail) );
      }
      else 
      {
         OTWDriver.RemoveObject(trail, TRUE);
      }
      trail = NULL;
   }

   if (engGlow)
   {
      OTWDriver.RemoveObject(engGlow, TRUE);
      engGlow = NULL;
   }

   if (groundGlow)
   {
      OTWDriver.RemoveObject(groundGlow, TRUE);
      groundGlow = NULL;
   }

   if (engGlowBSP1)
   {
      OTWDriver.RemoveObject(engGlowBSP1, TRUE);
      engGlowBSP1 = NULL;
   }

#endif
}
