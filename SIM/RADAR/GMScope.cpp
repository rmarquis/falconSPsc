#include "stdhdr.h"
#include "Graphics\Include\gmComposit.h"
#include "Graphics\Include\Drawbsp.h"
#include "geometry.h"
#include "debuggr.h"
#include "object.h"
#include "radarDoppler.h"
#include "simbase.h"
#include "otwdrive.h"
#include "simdrive.h"
#include "simfiltr.h"
#include "simveh.h"
#include "campwp.h"
#include "sms.h"
#include "smsdraw.h"
#include "aircrft.h"
#include "fack.h"	//MI
#include "classtbl.h" // JB carrier
#include "fcc.h"	//MI
#include "laserpod.h"	//MI
#include "mavdisp.h"	//MI
#include "sms.h"	//MI
#include "missile.h"	//MI

#define  DEFAULT_OBJECT_RADIUS        50.0F

SensorClass* FindLaserPod (SimMoverClass* theObject);	//MI
extern float g_fCursorSpeed;
extern bool g_bRealisticAvionics;
extern bool g_bAGRadarFixes;
extern float g_fGMTMaxSpeed;
extern float g_fGMTMinSpeed;
extern float g_fEXPfactor;
extern float g_fDBS1factor;
extern float g_fDBS2factor;

// NOTE:  These being global mean that there can be only one RadarDopplerClass instance.  
// They could easily be members of the class if this is a problem...
static Tpoint GMat;
static Tpoint viewCenter;
static Tpoint viewOffsetRel = {0.0F, 0.0F, 0.0F};
static Tpoint viewOffsetInertial = {0.0F, 0.0F, 0.0F};
static Tpoint viewFrom = {0.0F, 0.0F, 0.0F};
static float headingForDisplay = 0.0F;

static const float DisplayAreaViewTop    =  0.75F;
static const float DisplayAreaViewBottom = -0.68F;
static const float DisplayAreaViewLeft   = -0.80F;
static const float DisplayAreaViewRight  =  0.72F;
static const float RADAR_CONE_ANGLE      = 0.32f * PI;
static const float SIN_RADAR_CONE_ANGLE  = (float)sin(RADAR_CONE_ANGLE);
static const float COS_RADAR_CONE_ANGLE  = (float)cos(RADAR_CONE_ANGLE);
static const float TAN_RADAR_CONE_ANGLE  = (float)tan(RADAR_CONE_ANGLE);

void CalcRelGeom (SimBaseClass* ownObject, SimObjectType* targetList, TransformMatrix vmat, float elapsedTimeInverse);


float cRangeSquaredGMScope;

void RadarDopplerClass::GMMode (void)
{
float ownX = 0.0F;
float ownY = 0.0F;
float ownZ = 0.0F;
VuListIterator featureWalker (SimDriver.combinedFeatureList);
VuListIterator objectWalker (SimDriver.combinedList);
FalconEntity* testFeature = NULL;
SimBaseClass* testObject = NULL;
VuListIterator *walker = NULL;	//MI
GMList* curList = GMFeatureListRoot;
GMList* tmpList = NULL;
GMList* tmp2 = NULL;
GMList* lastList = NULL;
float range = 0.0F;
float radius = 0.0F;
float canSee = 0.0F;
float x = 0.0F;
float y = 0.0F;
float dx = 0.0F;
float dy = 0.0F;
Tpoint pos;
float radarHorizon = 0.0F;
mlTrig trig;

      mlSinCos (&trig, -platform->Yaw());

   // Find out where the beam hits the edge of the earth
   radius = EARTH_RADIUS_FT - platform->ZPos();
   radarHorizon = (float)sqrt (radius*radius - EARTH_RADIUS_FT*EARTH_RADIUS_FT);
   ownX = platform->XPos();
   ownY = platform->YPos();
   ownZ = platform->ZPos();

   // Parallel walk through the list of existing objects and the platforms
   // target list. Only draw the visible objects on the target list that
   // still exist. This test is to handle the case were an object is deleted
   // between the start of a refresh and it's visibility check.
   if (SimLibElapsedTime - lastFeatureUpdate > 500)
   {
      lastFeatureUpdate = SimLibElapsedTime;

      // Clear the head of the list of removed entities
      testFeature = (FalconEntity*)featureWalker.GetFirst();
      if (testFeature && curList)
      {
         while (curList && SimCompare (curList->Object(), testFeature) < 0)
         {
            tmpList = curList;
            curList = curList->next;
            tmpList->Release();
         }
      }

      GMFeatureListRoot = curList;
	  walker = &featureWalker;
      if (GMFeatureListRoot)
         GMFeatureListRoot->prev = NULL;

	  //MI
	  if(!testFeature && g_bAGRadarFixes && g_bRealisticAvionics)
	  {
		   testFeature = (SimBaseClass*)objectWalker.GetFirst();
		   walker = &objectWalker;
	  }
	  
      lastList = NULL;
      while (testFeature)
      {
         if (isEmitting)
         {
            range = (float)sqrt(
               (testFeature->XPos()-ownX)*(testFeature->XPos()-ownX) +
               (testFeature->YPos()-ownY)*(testFeature->YPos()-ownY) +
               (testFeature->ZPos()-ownZ)*(testFeature->ZPos()-ownZ));
            if (range < radarHorizon)
            {
               if (testFeature->IsSim())
               {
                  // Check for visibility
                  if (((SimBaseClass*)testFeature)->IsAwake())
                  {
                     radius = ((SimBaseClass*)testFeature)->drawPointer->Radius();
                     radius = radius*radius*radius*radius;
                     canSee = radius/range * tdisplayRange/groundMapRange;
                     ((SimBaseClass*)testFeature)->drawPointer->GetPosition(&pos);
                     testFeature->SetPosition(pos.x, pos.y, pos.z);

					 if(g_bRealisticAvionics && g_bAGRadarFixes)
					 {
						 if(walker == &objectWalker)
						 {
// 2002-04-03 MN removed IsBattalion check, added Drawable::Guys here
							 if(testFeature->Vt() > 1.0F || /*testFeature->IsBattalion()*/ 
							     ((SimBaseClass*)testFeature)->drawPointer && 
							     ((SimBaseClass*)testFeature)->drawPointer->GetClass() == DrawableObject::Guys)
							 {
								 radius = 0.0F;
								 //radius = radius*radius*radius*radius;
								 canSee = 0.0F; //radius/range * tdisplayRange/groundMapRange;
							 }
						 }
					 }
                  }
                  else
                  {
                     canSee = 0.0F;
                  }
               }
               else
               {
				   //MI
				   if(g_bRealisticAvionics && g_bAGRadarFixes)
				   {
					   if(walker == &objectWalker)
					   {
// 2002-04-03 MN testFeature is a CAMPAIGN object now !!! We can't do SimBaseClass stuff here. 
// Speed test however is valid, as it checks U_MOVING flag of unit
// As there are no campaign units that consist only of soldiers, no need to check for them here
						   if(testFeature->Vt() > 1.0F /*|| 
							   ((SimBaseClass*)testFeature)->drawPointer && 
							   ((SimBaseClass*)testFeature)->drawPointer->GetClass() == DrawableObject::Guys*/
							   )
						   {
							   radius = 0.0F;
							   //radius = radius*radius*radius*radius;
							   canSee = 0.0F;//radius/range * tdisplayRange/groundMapRange;
						   }
// 2002-04-03 MN a campaign unit only has two speed states - 0.0f and 40.0f for not moving/moving.
						   //else if(testFeature->Vt() < -1.0F)	//should never happen really.
						   else if (!testFeature->Vt())
						   {
							   radius = DEFAULT_OBJECT_RADIUS;
							   radius = radius*radius*radius*radius;
							   canSee = radius/range * tdisplayRange/groundMapRange;
						   }
					   }
					   else
					   {
						   radius = DEFAULT_OBJECT_RADIUS;
						   radius = radius*radius*radius*radius;
						   canSee = radius/range * tdisplayRange/groundMapRange;
					   }
				   }
				   else
				   {
					   radius = DEFAULT_OBJECT_RADIUS;
					   radius = radius*radius*radius*radius;
					   canSee = radius/range * tdisplayRange/groundMapRange;
				   }
               }
            }
            else
            {
               canSee = 0.0F;
            }

            // Check LOS
            if (canSee > 0.8F)
            {
               x = testFeature->XPos() - ownX;
               y = testFeature->YPos() - ownY;

               // Rotate for normalization
               dx = trig.cos*x - trig.sin*y;
               dy = trig.sin*x + trig.cos*y;

               // Check Angle off nose
               if (
                  (dy > 0.0F && dx > 0.5F * dy) || // Right side of nose
                  (dy < 0.0F && dx > 0.5F * -dy)   // Left side of nose
                  )
               {
                  // Actual LOS
                  if (!OTWDriver.CheckLOS( platform, testFeature))
                  {
                     canSee = 0.0F;  // LOS is blocked
                  }
               }
               else
               {
                  canSee = 0.0F;   // Outside of cone
               }
            }
         }
         else
         {
            canSee = 0.0F;
         }

         if (curList)
         {
            switch (SimCompare (curList->Object(), testFeature))
            {
               case 0:
                  if (canSee > 0.8F)
                  {
                     //Update
                     lastList = curList;
                     curList = curList->next;
                  }
                  else
                  {
                     // Object can't be seen, remove
                     if (curList->prev)
                        curList->prev->next = curList->next;
                     else
                     {
                        GMFeatureListRoot = curList->next;
                        if (GMFeatureListRoot)
                           GMFeatureListRoot->prev = NULL;
                     }

                     if(curList->next)
                        curList->next->prev = curList->prev;
                     tmpList = curList;
                     curList = curList->next;
                     tmpList->Release();
                  }
				  //MI
				  if(g_bRealisticAvionics && g_bAGRadarFixes)
				  {
					  if(walker == &featureWalker)
						  testFeature = (SimBaseClass*)featureWalker.GetNext();
					  else
						  testFeature = (SimBaseClass*)objectWalker.GetNext();
					  if(!testFeature && walker == &featureWalker)
					  { 
						  testFeature = (SimBaseClass*)objectWalker.GetFirst();
						  walker = &objectWalker;
					  } 
				  }
				  else
					  testFeature = (SimBaseClass*)featureWalker.GetNext();
               break;
               case 1: // testFeature > visObj -- Means the current allready deleted
                  if (curList->prev)
                     curList->prev->next = curList->next;
                  else
                  {
                     GMFeatureListRoot = curList->next;
                     if (GMFeatureListRoot)
                        GMFeatureListRoot->prev = NULL;
                  }

                  if(curList->next)
                     curList->next->prev = curList->prev;
                  tmpList = curList;
                  curList = curList->next;
                  tmpList->Release();
               break;

               case -1: // testFeature < visObj -- Means the current not added yet
				   bool filterthis = FALSE;
				   if(g_bRealisticAvionics && g_bAGRadarFixes)
				   {
					   if(walker == &objectWalker)
					   {
						   //bool here = true;
						   //float speed = testFeature->Vt();
// 2002-04-03 MN removed IsBattalion check, added Drawable::Guys here and IsSim() check - don't do simbase stuff on campaign objects
						   if(testFeature->Vt() > 1.0F || /*testFeature->IsBattalion()*/
							   testFeature->IsSim() &&
							   ((SimBaseClass*)testFeature)->drawPointer && 
							   ((SimBaseClass*)testFeature)->drawPointer->GetClass() == DrawableObject::Guys)
						   {
							   filterthis = TRUE;
						   }
					   }
				   }
				   if (canSee > 1.0F && !filterthis)
				   {
					   tmpList = new GMList (testFeature);
					   tmpList->next = curList;
					   tmpList->prev = lastList;
					   if (tmpList->next)
						   tmpList->next->prev = tmpList;
					   if (tmpList->prev)
						   tmpList->prev->next = tmpList;
					   if (curList == GMFeatureListRoot)
						   GMFeatureListRoot = tmpList;
					   lastList = tmpList;
				   }
				   //MI
				   if(g_bRealisticAvionics && g_bAGRadarFixes)
				   {
					   if(walker == &featureWalker)
						   testFeature = (SimBaseClass*)featureWalker.GetNext();
					   else
						   testFeature = (SimBaseClass*)objectWalker.GetNext();
					   if(!testFeature && walker == &featureWalker)
					   { 
						   testFeature = (SimBaseClass*)objectWalker.GetFirst();
						   walker = &objectWalker;
					   } 
				   }
				   else
					   testFeature = (SimBaseClass*)featureWalker.GetNext();
				   break;
            }
         } // curList
         else
         {
			 if (canSee > 1.0F)
			 {
				 curList = new GMList(testFeature);
				 curList->prev = lastList;

				 if (lastList)
					 lastList->next = curList;
				 else
				 {
					 GMFeatureListRoot = curList;
					 if (GMFeatureListRoot)
						 GMFeatureListRoot->prev = NULL;
				 }
				 lastList = curList;
				 curList = NULL;
			 }
			 //MI
			 if(g_bRealisticAvionics && g_bAGRadarFixes)
			 {
				 if(walker == &featureWalker)
					 testFeature = (SimBaseClass*)featureWalker.GetNext();
				 else
					 testFeature = (SimBaseClass*)objectWalker.GetNext();
				 if(!testFeature && walker == &featureWalker)
				 { 
					 testFeature = (SimBaseClass*)objectWalker.GetFirst();
					 walker = &objectWalker;
				 } 
			 }
			 else
				 testFeature = (SimBaseClass*)featureWalker.GetNext();
         }
      }

      // Delete anthing after curList
      tmpList = curList;
      if (tmpList && tmpList->prev)
		  tmpList->prev->next = NULL;

      if (tmpList == GMFeatureListRoot)
		  GMFeatureListRoot = NULL;

      while (tmpList)
      {
         tmp2 = tmpList;
         tmpList = tmpList->next;
         tmp2->Release();
      }

#ifdef DEBUG
      // Verify order and singleness of list
      tmpList = GMFeatureListRoot;
      while (tmpList) 
	  {
	      if (tmpList->next) 
		  {
			  //MI changed to get movers on the list
		      //F4Assert( SimCompare( tmpList->Object(), tmpList->next->Object() ) == 1 );
			  F4Assert((SimCompare(tmpList->Object(), tmpList->next->Object()) == 1) ||
				  (SimCompare(tmpList->Object(), tmpList->next->Object()) == -1));
	      }
	      tmpList = tmpList->next;
      }
#endif
   }
 
   // Now Do the movers
   curList = GMMoverListRoot;
   lastList = NULL;
   testObject = (SimBaseClass*)objectWalker.GetFirst();
   if (testObject && curList)
   {
      while (curList && SimCompare (curList->Object(), testObject) < 0)
      {
         tmpList = curList;
         curList = curList->next;
         tmpList->Release();
      }
   }

   GMMoverListRoot = curList;
   if (GMMoverListRoot)
      GMMoverListRoot->prev = NULL;

   while (testObject)
   {
      if (testObject->OnGround())
      {
         if (isEmitting)
         {
            // Check for visibility
            range = (float)sqrt(
               (testObject->XPos()-ownX)*(testObject->XPos()-ownX) +
               (testObject->YPos()-ownY)*(testObject->YPos()-ownY) +
               (testObject->ZPos()-ownZ)*(testObject->ZPos()-ownZ));

            if (range < radarHorizon)
            {
				//MI only show objects that are really moving
				if(g_bRealisticAvionics && g_bAGRadarFixes)
				{
					bool FilterThis = FALSE;
					if(testObject && testObject->IsSim() && testObject->drawPointer && 
						testObject->drawPointer->GetClass() == DrawableObject::Guys)
						FilterThis = TRUE;

					if(testObject->IsSim() && !FilterThis &&
						testObject->Vt() > g_fGMTMinSpeed && 
						testObject->Vt() < g_fGMTMaxSpeed)
					{
						if (testObject->IsAwake())
						{
							radius = 2.0F * testObject->drawPointer->Radius();
							/*  JB 010624 Why? Setting the position like this screws up multiplayer and entitys' movement
							if (testObject->GetDomain() != DOMAIN_SEA) // JB carrier (otherwise ships stop when you turn on your GM radar)
							{
							((SimBaseClass*)testObject)->drawPointer->GetPosition(&pos);
							testObject->SetPosition(pos.x, pos.y, pos.z);
							}*/
						}
						else
						{
							radius = 0.0F;
						}
					}
// 2002-04-03 MN added check for moving campaign objects
					else if (testObject->IsCampaign() && testObject->Vt()) // campaign units only return 40 or 0 knots, depending on U_MOVING flag
					{
						radius = DEFAULT_OBJECT_RADIUS;
					}
					else
					{
						radius = 0.0F;
					}
				}
				else
				{
					if (testObject->IsSim()) // NOTE this is for actually moving && testObject->Vt() > 10.0F * KNOTS_TO_FTPSEC &&
						//testObject->Vt() < 100.0F * KNOTS_TO_FTPSEC)
					{
						if (testObject->IsAwake())
						{
							radius = 2.0F * testObject->drawPointer->Radius();
							/*  JB 010624 Why? Setting the position like this screws up multiplayer and entitys' movement
							if (testObject->GetDomain() != DOMAIN_SEA) // JB carrier (otherwise ships stop when you turn on your GM radar)
							{
							((SimBaseClass*)testObject)->drawPointer->GetPosition(&pos);
							testObject->SetPosition(pos.x, pos.y, pos.z);
							}*/
						}
						else
						{
							radius = 0.0F;
						}
					}
					else 
					{
						radius = DEFAULT_OBJECT_RADIUS;
					}
				}

				radius = radius*radius*radius*radius*radius;
				canSee = radius/range * tdisplayRange/groundMapRange;

				// Check LOS
				if (canSee > 0.8F)
				{
					x = testObject->XPos() - ownX;
					y = testObject->YPos() - ownY;

					// Rotate for normalization
					dx = trig.cos*x - trig.sin*y;
					dy = trig.sin*x + trig.cos*y;

					// Check Angle off nose
					if((dy > 0.0F && dx > 0.5F * dy) || // Right side of nose
						(dy < 0.0F && dx > 0.5F * -dy))   // Left side of nose
					{
						// Actual LOS
						if (testObject->IsSim() && !OTWDriver.CheckLOS( platform, testObject))
						{
							canSee = 0.0F;  // LOS is blocked
						}
					}
					else
					{
						canSee = 0.0F;   // Outside of cone
					}
				}
            }
            else
            {
				canSee = 0.0F;  // Beyond radar horizon
            }
         }
         else
         {
			 canSee = 0.0F;  // Our radar is off
         }

         if (curList)
         {
			 if(testObject == curList->Object())
			 {
				 if (canSee > 0.8F)
				 {
					 //Update
					 lastList = curList;
					 curList = curList->next;
				 }
				 else
				 {
					 // Object can't be seen, remove
					 if (curList->prev)
						 curList->prev->next = curList->next;
					 else
					 {
						 GMMoverListRoot = curList->next;
						 if (GMMoverListRoot)
							 GMMoverListRoot->prev = NULL;
					 }
					 if(curList->next)
						 curList->next->prev = curList->prev;
					 tmpList = curList;
					 curList = curList->next;
					 tmpList->Release();
				 }
				 testObject = (SimBaseClass*)objectWalker.GetNext();
			 }
			 else
			 {
				 switch (SimCompare (curList->Object(), testObject))
				 {
				 case 0:
				 case 1: // testObject >= visObj -- Means the current allready deleted
                     if (curList->prev)
						 curList->prev->next = curList->next;
                     else
                     {
						 GMMoverListRoot = curList->next;
						 if (GMMoverListRoot)	// Don't point the thing before the end of the list to nothing if there isn't anything after this list - RH
						 {
							 GMMoverListRoot->prev = NULL;
						 }
                     }
                     if(curList->next)
						 curList->next->prev = curList->prev;
                     tmpList = curList;

                     curList = curList->next;
                     tmpList->Release();
					 break;

				 case -1: // testObject < visObj -- Means the current not added yet
                     if (canSee > 1.0F)
                     {
						 tmpList = new GMList (testObject);
						 tmpList->next = curList;
						 tmpList->prev = lastList;
						 if (tmpList->next)
							 tmpList->next->prev = tmpList;
						 if (tmpList->prev)
							 tmpList->prev->next = tmpList;
						 if (curList == GMMoverListRoot)
							 GMMoverListRoot = tmpList;
						 lastList = tmpList;
                     }
                     testObject = (SimBaseClass*)objectWalker.GetNext();
					 break;
				 }
			 } // inUse != testObject
         } // inUse
         else
         {
			 if (canSee > 1.0F)
			 {
				 curList = new GMList (testObject);
				 curList->prev = lastList;

				 if (lastList)
					 lastList->next = curList;
				 else
				 {
					 GMMoverListRoot = curList;
					 if (GMMoverListRoot)
						 GMMoverListRoot->prev = NULL;
				 }
				 lastList = curList;
				 curList = NULL;
			 }
			 testObject = (SimBaseClass*)objectWalker.GetNext();
         }
      }
      else
      {
		  testObject = (SimBaseClass*)objectWalker.GetNext();
      }
   }
   // Delete anthing after curList
   tmpList = curList;
   if (tmpList && tmpList->prev)
	   tmpList->prev->next = NULL;

   if (tmpList == GMMoverListRoot)
	   GMMoverListRoot = NULL;

   while (tmpList)
   {
	   tmp2 = tmpList;
	   tmpList = tmpList->next;
	   tmp2->Release();
   }

#ifdef DEBUG
   // Verify order and singleness of list
   tmpList = GMMoverListRoot;
   while (tmpList) 
   {
	   if (tmpList->next) 
	   {
		   F4Assert( SimCompare( tmpList->Object(), tmpList->next->Object() ) == 1 );
	   }
	   tmpList = tmpList->next;
   }
#endif
  
   if (IsSOI() && dropTrackCmd)
   {
      DropGMTrack();
   }
   //MI
   if(g_bRealisticAvionics && g_bAGRadarFixes)
   {
	   if(lockedTarget && lockedTarget->BaseData())
	   {
		   if(mode == GMT)
		   {
			   if(lockedTarget->BaseData()->IsSim() && (lockedTarget->BaseData()->Vt() < g_fGMTMinSpeed ||
				   lockedTarget->BaseData()->Vt() > g_fGMTMaxSpeed))
				   DropGMTrack();
			   else if(lockedTarget->BaseData()->IsCampaign() && lockedTarget->BaseData()->Vt() <= 0.0F)
				   DropGMTrack();
		   }
		   else if(mode == GM)
		   {
			   if(lockedTarget->BaseData()->IsSim() && lockedTarget->BaseData()->Vt() > 1.0F)
				   DropGMTrack();
			   else if(lockedTarget->BaseData()->IsCampaign() && lockedTarget->BaseData()->Vt() > 0.0F)
				   DropGMTrack();
		   }
	   }
   }

   //Build Track List
   if (lockedTarget)
   {
	   // WARNING:  This might do ALOT more work than you want.  CalcRelGeom
	   // will walk all children of lockedTarget (through the next pointer).
	   // If you don't want this, set is next pointer to NULL before calling
	   // and set it back upon return.
      CalcRelGeom(platform, lockedTarget, NULL, 1.0F / SimLibMajorFrameTime);
   }

   if (designateCmd)
   {
      if (mode == GM)
      {
	      DoGMDesignate (GMFeatureListRoot);
      }
		else if (mode == GMT || mode == SEA)
      {
	      DoGMDesignate (GMMoverListRoot);
      }
   }

   // Update groundSpot position
   GetAGCenter (&x, &y);
   ((AircraftClass*)platform)->Sms->drawable->SetGroundSpotPos(x, y, 0.0F);
}

void RadarDopplerClass::DropGMTrack(void)
{
float cosAz, sinAz;
mlTrig trig;

   if (lockedTarget)
   {
	   if(g_bRealisticAvionics && g_bAGRadarFixes)
	   {
		   mlSinCos (&trig, headingForDisplay);			  
		   cosAz = trig.cos;			   
		   sinAz = trig.sin;

		   viewOffsetInertial.x = lockedTarget->BaseData()->XPos() -
			   (platform->XPos() + tdisplayRange * cosAz * 0.5F);
		   viewOffsetInertial.y = lockedTarget->BaseData()->YPos() -
			   (platform->YPos() + tdisplayRange * sinAz * 0.5F);

		   viewOffsetRel.x =  cosAz*viewOffsetInertial.x + sinAz*viewOffsetInertial.y;
		   viewOffsetRel.y = -sinAz*viewOffsetInertial.x + cosAz*viewOffsetInertial.y;

		   viewOffsetRel.x /= tdisplayRange * 0.5F;
		   viewOffsetRel.y /= tdisplayRange * 0.5F;
	   }
	   else
	   {
		   if (flags & NORM)
		   {
			   mlSinCos (&trig, headingForDisplay);
			   cosAz = trig.cos;
			   sinAz = trig.sin;

			   if (!(flags & SP))
			   {
				   GMat.x = platform->XPos() + tdisplayRange * cosAz * 0.5F;
				   GMat.y = platform->YPos() + tdisplayRange * sinAz * 0.5F;
			   }
			   else
			   {
				   viewOffsetInertial.x = lockedTarget->BaseData()->XPos() -
					   (platform->XPos() + tdisplayRange * cosAz * 0.5F);
				   viewOffsetInertial.y = lockedTarget->BaseData()->YPos() -
					   (platform->YPos() + tdisplayRange * sinAz * 0.5F);
			   }

			   viewOffsetRel.x =  cosAz*viewOffsetInertial.x + sinAz*viewOffsetInertial.y;
			   viewOffsetRel.y = -sinAz*viewOffsetInertial.x + cosAz*viewOffsetInertial.y;

			   viewOffsetRel.x /= tdisplayRange * 0.5F;
			   viewOffsetRel.y /= tdisplayRange * 0.5F;
		   }
	   }
	   ClearSensorTarget();
	   //MI add image noise again
	   if(g_bRealisticAvionics && g_bAGRadarFixes)
	   {
		   scanDir = ScanFwd;
	   }
   }
}

void RadarDopplerClass::SetAimPoint (float xCmd, float yCmd)
{
float sinAz, cosAz;
mlTrig trig;
float halfRange = tdisplayRange * 0.5F;

MaverickDisplayClass *mavDisplay = NULL;

//MI we also want this happening when in TGP mode, not ground stabilized and SOI
LaserPodClass* laserPod = (LaserPodClass* )FindLaserPod (SimDriver.playerEntity);
//MI same for MAV's
	if(SimDriver.playerEntity && SimDriver.playerEntity->Sms && SimDriver.playerEntity->Sms->curWeaponType == wtAgm65 &&
		SimDriver.playerEntity->Sms->curWeapon)
	{
		ShiAssert(SimDriver.playerEntity->Sms->curWeapon->IsMissile());
		mavDisplay = (MaverickDisplayClass*)((MissileClass*)SimDriver.playerEntity
			->Sms->curWeapon)->display;
	}
   if((IsSOI() && !lockedTarget) || (((laserPod && laserPod->IsSOI()) || (mavDisplay && mavDisplay->IsSOI())) && 
	  SimDriver.playerEntity && SimDriver.playerEntity->FCC && SimDriver.playerEntity->FCC->preDesignate) ||
	  ((SimDriver.playerEntity->FCC->GetSubMode() == FireControlComputer::CCRP)))
   {
	   //MI better cursor control
	   if(xCmd != 0.0F || yCmd != 0.0F)
	   {
			float CursorSpeed = g_fCursorSpeed;
			if (flags & DBS2)
				CursorSpeed *= g_fDBS2factor;
			else if (flags & DBS1)
				CursorSpeed *= g_fDBS1factor;
			else if (flags & EXP)
				CursorSpeed *= g_fEXPfactor;

			viewOffsetRel.x += yCmd * 0.5F * CursorSpeed * curCursorRate * SimLibMajorFrameTime;
			viewOffsetRel.y += xCmd * 0.5F * CursorSpeed * curCursorRate * SimLibMajorFrameTime;

			static float test = 0.0f;
			static float testa = 0.0f;
         curCursorRate = min(curCursorRate + CursorRate * SimLibMajorFrameTime * (4.0F+test), (6.5F+testa) * CursorRate);

      }
      else
         curCursorRate = CursorRate;
      //viewOffsetRel.x += yCmd * CursorRate*3 * SimLibMajorFrameTime *
      //     groundMapRange / halfRange;
      //viewOffsetRel.y += xCmd * CursorRate*3 * SimLibMajorFrameTime *
       //    groundMapRange / halfRange;

// 2002-04-04 MN fix for cursor movement on the radar cone borders, only restrict to SnowPlow
	if ( (flags & SP) && (float)fabs(viewOffsetRel.y) > (viewOffsetRel.x + 1.0F)*TAN_RADAR_CONE_ANGLE )
	{
		// set to middle axis when really close to it
		if (viewOffsetRel.y > -0.05f && viewOffsetRel.y < 0.05f)
		{
			viewOffsetRel.y = 0.0f;
		}
		else if (xCmd && yCmd ) // let the cursor stay at its position on the gimbal border
		{
			if (viewOffsetRel.y >= 0.0f)
			{
				viewOffsetRel.y = (float)(viewOffsetRel.x + 1.0F) * TAN_RADAR_CONE_ANGLE;
				viewOffsetRel.x = (float)(viewOffsetRel.y  / TAN_RADAR_CONE_ANGLE) - 1.0F;
			}
			else
			{
				viewOffsetRel.y = -((float)(viewOffsetRel.x + 1.0F) * TAN_RADAR_CONE_ANGLE);
				viewOffsetRel.x = -((float)(viewOffsetRel.y / TAN_RADAR_CONE_ANGLE) + 1.0F);
			}
		}
		else if (yCmd)
		{
			if (viewOffsetRel.y >= 0.0f)
// Tangens works a bit different ;-)
			//viewOffsetRel.y = (float)fabs(viewOffsetRel.x + 1.0F) / TAN_RADAR_CONE_ANGLE - 1.0F;
				viewOffsetRel.y = (float)(viewOffsetRel.x + 1.0F) * TAN_RADAR_CONE_ANGLE;
			else
				viewOffsetRel.y = -((float)(viewOffsetRel.x + 1.0F) * TAN_RADAR_CONE_ANGLE);
		}
		else if (xCmd)
		{
			if (viewOffsetRel.y >= 0.0F)
				viewOffsetRel.x = (float)(viewOffsetRel.y  / TAN_RADAR_CONE_ANGLE) - 1.0F;
			else
				viewOffsetRel.x = -(float)((viewOffsetRel.y / TAN_RADAR_CONE_ANGLE) + 1.0F);
		}
	}

	   viewOffsetRel.x = min ( max (viewOffsetRel.x, -0.975F), 0.975F);
	   viewOffsetRel.y = min ( max (viewOffsetRel.y, -0.975F), 0.975F);

    }



	

   mlSinCos (&trig, headingForDisplay);
   cosAz = trig.cos;
   sinAz = trig.sin;

   viewOffsetInertial.x = cosAz*viewOffsetRel.x - sinAz*viewOffsetRel.y;
   viewOffsetInertial.y = sinAz*viewOffsetRel.x + cosAz*viewOffsetRel.y;

   viewOffsetInertial.x *= halfRange;
   viewOffsetInertial.y *= halfRange;
}

int RadarDopplerClass::CheckGMBump(void)
{
int rangeChangeCmd = 0;
int maxIdx;
float cRangeSQ;
float tmpX = (viewOffsetRel.x + 1.0F) * 0.5F;   // Correct for 0.0 being the center of the scope
float topfactor = 0.0F;
float bottomfactor = 0.0F;

	int range = (int)rangeScales[curRangeIdx];
	switch(range)
	{
	case 10:
		if(flags & SP)	//SnowPlow
		{
			topfactor = 1.6F;
			bottomfactor = 0.0F;
		}
		else
		{
			topfactor = 0.48F;
			bottomfactor = 0.0F;
		}
	break;
	case 20:
		if(flags & SP)	//SnowPlow
		{
			topfactor = 1.7F;
			bottomfactor = 0.0078F;
		}
		else
		{
			topfactor = 1.797F;
			bottomfactor = 0.05F;
		}
	break;
	case 40:
		if(flags & SP)	//SnowPlow
		{
			topfactor = 1.7F;
			bottomfactor = 0.0088F;
		}
		else
		{
			topfactor = 1.80280F;
			bottomfactor = 0.236F;
		}
	break;
	case 80:
		if(flags & SP)	//SnowPlow
		{
			topfactor = 2.0F;
			bottomfactor = 0.014497F;
		}
		else
		{
			topfactor = 2.0F;
			bottomfactor = 0.4F;
		}
	break;
	default:
		ShiWarning("Should not get here")
	break;
	}


//   cRangeSQ =  tmpX*tmpX + viewOffsetRel.y*viewOffsetRel.y;
	cRangeSQ = tmpX*tmpX + tmpX*tmpX; // only change range when we exceed limits in y direction (viewOffsetRel.x = y direction)

	cRangeSquaredGMScope = cRangeSQ;

   //MI
   if(g_bRealisticAvionics && g_bAGRadarFixes)
   {
	   if(IsSet(AutoAGRange))
	   {
		   if (cRangeSQ > topfactor)
		   {
			   rangeChangeCmd = 1;
		   }
		   else if (cRangeSQ < bottomfactor) 
		   { 
			   rangeChangeCmd = -1;
		   }
	   }
   }
   else
   {
	   if (cRangeSQ > 0.95F * 0.95F)
	   {
		   rangeChangeCmd = 1;
	   }
	   else if (cRangeSQ < 0.425F * tmpX) 
	   {
		   rangeChangeCmd = -1;
	   }
   }

   // Max range available
   if (flags & (DBS1 | DBS2) || mode == GMT || mode == SEA)
   {
      maxIdx = NUM_RANGES - 2;
   }
   else
   {
      maxIdx = NUM_RANGES - 1;
   }

   if (curRangeIdx + rangeChangeCmd >= maxIdx)
      rangeChangeCmd = 0;
   else if (curRangeIdx + rangeChangeCmd < 0)
      rangeChangeCmd = 0;

   return rangeChangeCmd;
}

void RadarDopplerClass::AdjustGMOffset (int rangeChangeCmd)
{
   if (rangeChangeCmd > 0)
   {
	   //MI
	   if(g_bRealisticAvionics && g_bAGRadarFixes)
	   {
		   if(mode == GM || mode == SEA)
		   {
			   if(gmRangeIdx >= 4)
				   return;
		   }
		   else if(mode == GMT)
		   {
			   if(gmRangeIdx >= 3)
				   return;
		   }
	   }
      viewOffsetRel.y *= 0.5F;
      viewOffsetRel.x = ((viewOffsetRel.x + 1.0F) * 0.5F) - 1.0F;
   }
   else if (rangeChangeCmd < 0)
   {
	   //MI
	   if(g_bRealisticAvionics && g_bAGRadarFixes)
	   {
		   if(gmRangeIdx == 0)
			   return;
	   }
      viewOffsetRel.y *= 2.0F;
      viewOffsetRel.x = ((viewOffsetRel.x + 1.0F) * 2.0F) - 1.0F;
   }
   SetAimPoint (0.0F, 0.0F);
}


void RadarDopplerClass::SetGroundPoint (float xPos, float yPos, float zPos)
{
	viewCenter.x = xPos;
	viewCenter.y = yPos;
	viewCenter.z = zPos;
}

// 2002-04-04 MN this fixes old AG radar bug where when you were in STP mode, 
void RadarDopplerClass::RestoreAGCursor()
{
	viewOffsetInertial.x = 0.0f;
	viewOffsetInertial.y = 0.0f;
	viewOffsetInertial.z = 0.0f;

	viewOffsetRel.x = 0.0f;
	viewOffsetRel.y = 0.0f;
	viewOffsetRel.z = 0.0f;

// now check if steerpoint position is off of the current radar range, and adjust it appropriately
// only in NORM mode and STP mode
	if ((flags & NORM) && !(flags & SP) )
	{
		// Distance to GMat, only x and y
		float dx,dy,dist, dispRange;
		dx = platform->XPos() - viewCenter.x;
		dy = platform->YPos() - viewCenter.y;

		dist = dx*dx + dy*dy;

		// find correct radar scan distance
		for (int i=0; i<NUM_RANGES; i++)
		{
			dispRange = rangeScales[i] * NM_TO_FT;
			dispRange *= dispRange;
			if (dist < dispRange)
				break;
		}

		int maxIdx;
		// Reuse current range, within limits of course
		if (flags & (DBS1 | DBS2) || mode == GMT || mode == SEA)
		{
			maxIdx = NUM_RANGES - 3;
		}
		else
		{
			maxIdx = NUM_RANGES - 2;
		}

		if (mode != GM)
		{
			ClearFlagBit(DBS1);
			ClearFlagBit(DBS2);
		}

		curRangeIdx = i;
		if (curRangeIdx > maxIdx)
			curRangeIdx = maxIdx;
		else if (curRangeIdx < 0)
			curRangeIdx = 0;

		gmRangeIdx = curRangeIdx;
		displayRange = rangeScales[curRangeIdx];
		tdisplayRange = displayRange * NM_TO_FT;
		SetGMScan();
	}
}

void RadarDopplerClass::SetGMScan (void)
{
float scanBottom, scanTop;
float nearEdge, farEdge;
static const float TwoRootTwo = 2.0f * (float)sqrt( 2.0f );


   if (flags & EXP)
   {
	   groundMapRange = tdisplayRange * 0.125F;
	   if (displayRange <= 20.1f) 
	   {
		   groundMapLOD = 2;
	   } else 
	   {
		   groundMapLOD = 3;
	   }
	   //azScan = 60.0F * DTR;		// Radar still scans full volume, but only displays a subset...
	   //MI az is set thru the OSB now
	  if(!g_bRealisticAvionics || !g_bAGRadarFixes)
		  azScan = 60.0F * DTR;
	  else if(g_bRealisticAvionics && g_bAGRadarFixes)
	  {
		  if(mode == GM || mode == SEA)
			  curAzIdx = gmAzIdx;
		  else
			  curAzIdx = gmtAzIdx;				 
		  azScan = rwsAzs[curAzIdx];
	  }
   }
   else if (flags & DBS1)
   {
	   groundMapRange = tdisplayRange * 0.125F;
	   if (displayRange <= 20.1f) 
	   {
		   groundMapLOD = 1;
	   } 
	   else 
	   {
		   groundMapLOD = 2;
	   }

//	  azScan = atan2( TwoRootTwo*groundMapRange, distance from platform to GMat );
      //azScan = 15.0F * DTR;
	   //MI az is set thru the OSB now
	  if(!g_bRealisticAvionics || !g_bAGRadarFixes)
		  azScan = 15.0F * DTR;
	  else if(g_bRealisticAvionics && g_bAGRadarFixes)
	  {
		  if(mode == GM || mode == SEA)
			  curAzIdx = gmAzIdx;
		  else
			  curAzIdx = gmtAzIdx;				 
		  azScan = rwsAzs[curAzIdx];
	  }
   }
   else if (flags & DBS2)
   {
      groundMapRange = tdisplayRange * 0.0625F;
      groundMapRange = min ( max (groundMapRange, 1.0F * NM_TO_FT), 7.0F * NM_TO_FT);
	  if (displayRange <= 20.1f) {
		groundMapLOD = 0;
	  } else {
		groundMapLOD = 1;
	  }
//	  azScan = atan2( TwoRootTwo*groundMapRange, distance from platform to GMat );
      //azScan = 5.0F * DTR;
	  //MI az is set thru the OSB now
	  if(!g_bRealisticAvionics || !g_bAGRadarFixes)
		  azScan = 5.0F * DTR;
	  else if(g_bRealisticAvionics && g_bAGRadarFixes)
	  {
		  if(mode == GM || mode == SEA)
			  curAzIdx = gmAzIdx;
		  else
			  curAzIdx = gmtAzIdx;				 
		  azScan = rwsAzs[curAzIdx];
	  }
   }
   else	// NORM
   {
	  ShiAssert( flags & NORM );	// If not, then what mode is this???
      groundMapRange = tdisplayRange * 0.5F;
	  if (displayRange <= 20.1f) 
	  {
		groundMapLOD = 3;
	  } else 
	  {
		groundMapLOD = 4;
	  }
	  //MI az is set thru the OSB now
	  if(!g_bRealisticAvionics || !g_bAGRadarFixes)
		  azScan = MAX_ANT_EL;
	  else if(g_bRealisticAvionics && g_bAGRadarFixes)
	  {
		  if(mode == GM || mode == SEA)
			  curAzIdx = gmAzIdx;
		  else
			  curAzIdx = gmtAzIdx;				 
		  azScan = rwsAzs[curAzIdx];
	  }
   }

   nearEdge = displayRange * NM_TO_FT - groundMapRange;
   farEdge = displayRange * NM_TO_FT + groundMapRange;
   scanTop = -(float)atan (-platform->ZPos() / farEdge);
   scanBottom = max (-MAX_ANT_EL, -(float)atan (-platform->ZPos() / nearEdge));
   // JB 010707 
	 //bars = (int)((scanTop - scanBottom) / (barWidth * 2.0F) + 0.5F);
	 bars = max(1, (int)((scanTop - scanBottom) / (barWidth * 2.0F) + 0.5F));

	// For expanded modes, you'll need to set "at" to the center
	// of the expanded region and reduce range to the size of the footprint
	// to be expanded.  "center" is when the MFD center should fall in world space.
	//
	// SetGimbalLimit should also be done only when the value changes (typically at setup only)
	// It allows you to provide a max ATA beyond which the image is clipped (approximatly)
	//
    // Just use privateDisplay since that is the one and only GM type renderer
	// and we want it in the right mode when it is used again even if it isn't current.
	if( privateDisplay ) {
		((RenderGMComposite*)privateDisplay)->SetRange( groundMapRange, groundMapLOD );
//		((RenderGMComposite*)privateDisplay)->SetGimbalLimit( azScan );
		//MI take the azimuth we've selected thru the OSB
		if(!g_bRealisticAvionics || !g_bAGRadarFixes)
			((RenderGMComposite*)privateDisplay)->SetGimbalLimit( MAX_ANT_EL );
		else
			((RenderGMComposite*)privateDisplay)->SetGimbalLimit(azScan);
	}
}

void RadarDopplerClass::GMDisplay(void)
{
Tpoint			center={0.0F};
int				i=0;
float			len=0.0F;
int				curFov=0;
float			dx=0.0F, dy=0.0F, dz=0.0F, cosAz=0.0F, sinAz=0.0F, rx=0.0F, ry=0.0F;
float			groundLookEl=0.0F, baseAz=0.0F, baseEl=0.0F;
int				beamPercent=0;
mlTrig			trig={0.0F};
float			vpLeft=0.0F, vpTop=0.0F, vpRight=0.0F, vpBottom=0.0F;
int				tmpColor = display->Color();

   // Mode Step ?
   if (fovStepCmd)
   {
      fovStepCmd = 0;
      curFov = flags & 0x0f;
      flags -= curFov;
      curFov = curFov << 1;
      if (mode == GM && displayRange <= 40.0F)
      {
         if (curFov > DBS2)
            curFov = NORM;
      }
      else
      {
         if (curFov > EXP)
            curFov = NORM;
      }
      flags += curFov;
      SetGMScan();
   }

   // Find lookat point
   if (!(flags & FZ))
   {
      viewFrom.x = platform->XPos();
      viewFrom.y = platform->YPos();
      viewFrom.z = platform->ZPos();

	  headingForDisplay = platform->Yaw();
      mlSinCos (&trig, headingForDisplay);
      if (!lockedTarget)
      {
         if (flags & SP)
         {
			// We're in snowplow, so look out in front of the aircraft
            GMat.x = viewFrom.x + tdisplayRange * 0.5F * trig.cos;
            GMat.y = viewFrom.y + tdisplayRange * 0.5F * trig.sin;
         }
         else
         {
			// We're in steer point mode, so look there
            GMat.x = viewCenter.x;
            GMat.y = viewCenter.y;
         }

         if (!(flags & NORM))
         {
            // We're zoomed in, so track the cursors
            GMat.x += viewOffsetInertial.x;
            GMat.y += viewOffsetInertial.y;
         }
      }
      else 
      {
         if (!(flags & NORM))
         {
			// We're zoomed in, so look at the target
            GMat.x = lockedTarget->BaseData()->XPos();
            GMat.y = lockedTarget->BaseData()->YPos();
         }
         else if (flags & SP)
         {
			// We're in snowplow, so look out in front of the aircraft
            GMat.x = viewFrom.x + tdisplayRange * 0.5F * trig.cos;
            GMat.y = viewFrom.y + tdisplayRange * 0.5F * trig.sin;
            viewOffsetInertial.x = lockedTarget->BaseData()->XPos() - GMat.x;
            viewOffsetInertial.y = lockedTarget->BaseData()->YPos() - GMat.y;
         }
         else
         {
	   		// We're in steer point NORM mode, so look there
            GMat.x = viewCenter.x;
            GMat.y = viewCenter.y;
         }
      }

      GMat.z  = OTWDriver.GetGroundLevel (GMat.x, GMat.y);

      groundDesignateX = GMat.x;
      groundDesignateY = GMat.y;
      groundDesignateZ = GMat.z;
   }
   else
   {
      mlSinCos (&trig, headingForDisplay);
   }

   // We now now where the radar is looking.  Now decide where the display is centered
   if (flags & NORM)
   {
		center.x = viewFrom.x + tdisplayRange * 0.5F * trig.cos;
		center.y = viewFrom.y + tdisplayRange * 0.5F * trig.sin;
   }
   else
   {
		center.x = GMat.x;
		center.y = GMat.y;
   }

   // Find inertial display center
   cosAz = trig.cos;
   sinAz = trig.sin;
   dx = GMat.x - viewFrom.x;
   dy = GMat.y - viewFrom.y;
   dz = GMat.z - viewFrom.z;

   groundLookAz = (float)atan2 (dy, dx);
   groundLookEl = (float)atan (-dz / (float)sqrt(dy*dy + dx*dx+.1));

   // Update the deltas if the cursor is off center
   if (flags & NORM)
   {
      dx += viewOffsetInertial.x; 
      dy += viewOffsetInertial.y;
      dz  = OTWDriver.GetGroundLevel (GMat.x + viewOffsetInertial.x, GMat.y + viewOffsetInertial.y) - viewFrom.z;

      // position the seeker volume center
      baseAz = (float)atan2 (dy, dx);
      baseEl = (float)atan (-dz / (float)sqrt(dy*dy + dx*dx+.1));
   } else {
      // position the seeker volume center
      baseAz = groundLookAz;
      baseEl = groundLookEl;
   }


   // Position the cursor
   cursorY = ( cosAz*dx + sinAz*dy) / (tdisplayRange * 0.5F) - 1.0F;
   cursorX = (-sinAz*dx + cosAz*dy) / (tdisplayRange * 0.5F);

   cursorX = max ( min (cursorX, 0.95F), -0.95F);
   cursorY = max ( min (cursorY, 0.95F), -0.95F);

   // remove body rotations
   seekerAzCenter =  baseAz - platform->Yaw();
   if (seekerAzCenter > 180.0F * DTR)
      seekerAzCenter -= 360.0F * DTR;
   else if (seekerAzCenter < -180.0F * DTR)
      seekerAzCenter += 360.0F * DTR;
   seekerElCenter = baseEl - platform->Pitch();
/*
MonoPrint ("Cursor %12.2f %12.2f   Center %12.2f\n", cursorX, cursorY, seekerAzCenter * RTD);
MonoPrint ("  GMat %12.2f %12.2f  Look Az %12.2f\n", GMat.x, GMat.y, groundLookAz * RTD);
MonoPrint ("   Rel %12.2f %12.2f Inertial %12.2f %12.2f\n", viewOffsetRel.x, viewOffsetRel.y, viewOffsetInertial.x, viewOffsetInertial.y);
*/

   // Blown antenna limit?
   if (fabs(seekerAzCenter) > MAX_ANT_EL)
   {
	   //MI why would we want to do this?
	  if(!g_bRealisticAvionics || !g_bAGRadarFixes)
	  {
		  viewOffsetRel.x = 0.0F;
		  viewOffsetRel.y = 0.0F;
		  SetAimPoint (0.0F, 0.0F);
		  seekerAzCenter = 0.0F;
	  }
	  else if(g_bRealisticAvionics && g_bAGRadarFixes)
	  {
		  if(seekerAzCenter > 0.0F)
			  seekerAzCenter = MAX_ANT_EL;
		  else
			  seekerAzCenter = -MAX_ANT_EL;
	  }
      curFov = flags & 0x0f;
      flags -= curFov;
      flags += NORM;
      SetGMScan();
      DropGMTrack();
   }

   // Draw the GM Display
   if (display)
   {
	   // Draw the actual image in the center
	   display->GetViewport (&vpLeft, &vpTop, &vpRight, &vpBottom);
	   display->SetViewportRelative (DisplayAreaViewLeft, DisplayAreaViewTop, DisplayAreaViewRight, DisplayAreaViewBottom);

	   // edg: DON'T DRAW when the display is a CANVAS
	   if (display->type != VirtualDisplay::DISPLAY_CANVAS )
	   {
		   if (gainCmd != 0.0F)
		   {
			   if(gainCmd < 1)
				   GainPos -= 0.5F;
			   else
				   GainPos += 0.5F;
			   if (GainPos < -5) 
				   GainPos = -5;
			   if(GainPos > 20)
				   GainPos = 20;
			   if (GainPos < 0) 
				   curgain = 1 * pow(0.8, - GainPos);
			   else if (GainPos > 0)
				   curgain = 1 * pow(1.25, GainPos);
			   else 
				   curgain = 1;
			   
			   //MI
			   if(!g_bRealisticAvionics || !g_bAGRadarFixes)
				   ((RenderGMComposite*)display)->SetGain(((RenderGMComposite*)display)->GetGain()*gainCmd);
			   else
				   ((RenderGMComposite*)display)->SetGain(curgain);
			   
			   gainCmd = 0.0F;
		   }
		   if(InitGain && g_bRealisticAvionics && g_bAGRadarFixes)
		   {
			    curgain = 1 * pow(1.25, GainPos);
				((RenderGMComposite*)display)->SetGain(curgain);
				InitGain = FALSE;
		   }

		   if (!(flags & FZ))
		   {
			   // Decide how far along the beam scan is
			   if (beamAz >  azScan) 
				   beamAz =  azScan;
			   if (beamAz < -azScan) 
				   beamAz = -azScan;
			   float tempres = beamAz/azScan;
			   beamPercent = FloatToInt32((1.0f + beamAz/azScan) * 50.0f);
			   if (scanDir == ScanRev)
			   {
				   beamPercent = 100 - beamPercent;
			   }

			   // OW
			   ((RenderGMComposite*)display)->FinishFrame();

			   // Advance the beam
			   ShiAssert( beamPercent <= 100 );
			   // From, At, Center == Ownship, Look Point, Center of MFD in world
			   
// 2002-04-03 MN send the seekers current center to SetBeam to modify the gimbal borders
			   float cursorAngle = 0.0F;
			   if(g_bRealisticAvionics && g_bAGRadarFixes)
			   {
				   float value = 60.0F * DTR;
				   if(azScan < value)
					   cursorAngle = seekerAzCenter;
			   }

			   ((RenderGMComposite*)display)->SetBeam( &viewFrom, &GMat, &center, headingForDisplay, baseAz+beamAz, beamPercent, cursorAngle, (scanDir==ScanFwd) );
		   }

		   // OW - restore render target and start new scene
		   ((RenderGMComposite*)display)->StartFrame();

		   // Generate the radar imagery
		   //MI
		   if(g_bRealisticAvionics && g_bAGRadarFixes)
		   {
			   if(!lockedTarget)
				   ((RenderGMComposite*)display)->DrawComposite( &center, headingForDisplay );
		   }
		   else
			   ((RenderGMComposite*)display)->DrawComposite( &center, headingForDisplay );


//			((RenderGMComposite*)display)->FinishFrame();
//			((RenderGMComposite*)display)->DebugDrawRadarImage( OTWDriver.OTWImage );
//			((RenderGMComposite*)display)->StartFrame();
//			((RenderGMComposite*)display)->DebugDrawLeftTexture( OTWDriver.renderer );

#if 0
			{
				float	dx, dy;
				char	string[80];

				dx = GMat.x-viewFrom.x;
				dy = GMat.y-viewFrom.y;

				sprintf( string, "beamAz %0f, beamDir %0d, beamPercent %0d", beamAz * 180.0f/PI, (scanDir==ScanFwd)?1:-1, beamPercent );
				display->ScreenText( 320.0f, 10.0f, string );

				sprintf( string, "from <%0f,%0f>  at <%0f,%0f>", viewFrom.x, viewFrom.y, GMat.x, GMat.y );
				display->ScreenText( 320.0f, 18.0f, string );

				sprintf( string, "dx %0f, dy%0f, lookAngle %0f", dx, dy, atan2(dx,dy)*180.0f/PI );
				display->ScreenText( 320.0f, 26.0f, string );
			}
#endif

		} 
		else
		{
			display->SetColor (tmpColor);

			// Add Features
			if (mode == GM)
			{
				AddTargetReturnsOldStyle (GMFeatureListRoot);
			}
			// Add Movers
			else if (mode == GMT || mode == SEA)
			{
				AddTargetReturnsOldStyle (GMMoverListRoot);
			}
		}

      // Make sure to draw in green
		display->SetColor (tmpColor);

      // Add the Airplane if in freeze mode
      if (flags & FZ)
      {
         // Note the axis switch from NED to screen
         dx = platform->XPos() - GMXCenter;
         dy = platform->YPos() - GMYCenter;

         ry = trig.cos * dx + trig.sin * dy;//me123 from - to +
         rx = -trig.sin * dx + trig.cos * dy;//me123 from + to -

         rx /= groundMapRange;
         ry /= groundMapRange;

         display->AdjustOriginInViewport (rx, ry);
         display->AdjustRotationAboutOrigin(platform->Yaw() - headingForDisplay);
         display->Line (0.1F, 0.0F, -0.1F, 0.0F);
         display->Line (0.0F, 0.1F, 0.0F, -0.2F);
         display->ZeroRotationAboutOrigin();
         display->AdjustOriginInViewport (-rx, -ry);
      }

      if (flags & NORM)
      {
         // Add FTT Diamond if needed
         if (lockedTarget)
         {
			 //MI
			 if(g_bRealisticAvionics && g_bAGRadarFixes)
			 {
				 static const float size = 0.065F;
				 display->Tri(cursorX, cursorY, cursorX + size, cursorY, cursorX, cursorY + size);
				 display->Tri(cursorX, cursorY, cursorX - size, cursorY, cursorX, cursorY - size);
				 display->Tri(cursorX, cursorY, cursorX + size, cursorY, cursorX, cursorY - size);
				 display->Tri(cursorX, cursorY, cursorX - size, cursorY, cursorX, cursorY + size);
			 }
			 else
			 {
				 display->Line (cursorX+0.1F, cursorY, cursorX, cursorY+0.1F);
				 display->Line (cursorX+0.1F, cursorY, cursorX, cursorY-0.1F);
				 display->Line (cursorX-0.1F, cursorY, cursorX, cursorY+0.1F);
				 display->Line (cursorX-0.1F, cursorY, cursorX, cursorY-0.1F);
			 }
         }

         // Add Cursor
         display->Line (-1.0F, cursorY, 1.0F, cursorY);
         display->Line (cursorX, -1.0F, cursorX, 1.0F);

         // Expansion Cues
		 if(g_bRealisticAvionics && g_bAGRadarFixes)
		 {			
			 float len = 0.065F;
			 display->Line (cursorX+0.25F, cursorY+len, cursorX+0.25F, cursorY-len);
			 display->Line (cursorX-0.25F, cursorY+len, cursorX-0.25F, cursorY-len);
			 display->Line (cursorX+len, cursorY+0.25F, cursorX-len, cursorY+0.25F);
			 display->Line (cursorX+len, cursorY-0.25F, cursorX-len, cursorY-0.25F);
		 }
		 else
		 {
			 display->Line (cursorX+0.25F, cursorY+0.1F, cursorX+0.25F, cursorY-0.1F);
			 display->Line (cursorX-0.25F, cursorY+0.1F, cursorX-0.25F, cursorY-0.1F);
			 display->Line (cursorX+0.1F, cursorY+0.25F, cursorX-0.1F, cursorY+0.25F);
			 display->Line (cursorX+0.1F, cursorY-0.25F, cursorX-0.1F, cursorY-0.25F);
		 }

         // Add the Arcs
		 //MI
		 if(g_bRealisticAvionics && g_bAGRadarFixes)
		 {
			 if(!lockedTarget)
			 {
				 if (displayRange > 10.0F)
				 {
					 for (i=0; i<3; i++)
					 {
						 display->Arc(0.0F, -1.0F, (i+1)*0.5F, 213.0F*DTR, 333.0F * DTR);
					 }
				 }
				 else
				 {
					 display->Arc(0.0F, -1.0F, 1.0F, 213.0F*DTR, 333.0F * DTR);
				 }
			 }
		 }
		 else
		 {
			 if (displayRange > 10.0F)
			 {
				 for (i=0; i<3; i++)
				 {
					 display->Arc(0.0F, -1.0F, (i+1)*0.5F, 213.0F*DTR, 333.0F * DTR);
				 }
			 }
			 else
			 {
				 display->Arc(0.0F, -1.0F, 1.0F, 213.0F*DTR, 333.0F * DTR);
			 }
			 // Lines are at 60 degrees
			 display->Line (0.0F, -1.0F,  1.0F, -0.5F);
			 display->Line (0.0F, -1.0F, -1.0F, -0.5F);
		 }
      }
      else
      {
         // Add True Cursor
         display->Line (cursorX-0.1F, cursorY, cursorX+0.1F, cursorY);
         display->Line (cursorX, cursorY+0.1F, cursorX, cursorY-0.1F);

         // Add FTT Diamond if needed
         if (lockedTarget)
         {
			 //MI
			 if(g_bRealisticAvionics && g_bAGRadarFixes)
			 {
				 static const float size = 0.065F;
				 display->Tri(0.0F, 0.0F, size, 0.0F, 0.0F, size);
				 display->Tri(0.0F, 0.0F, -size, 0.0F, 0.0F, size);
				 display->Tri(0.0F, 0.0F, size, 0.0F, 0.0F, -size);
				 display->Tri(0.0F, 0.0F, -size, 0.0F, 0.0F, -size);
			 }
			 else
			 {
				 display->Line ( 0.1F, 0.0F, 0.0F,  0.1F);
				 display->Line ( 0.1F, 0.0F, 0.0F, -0.1F);
				 display->Line (-0.1F, 0.0F, 0.0F,  0.1F);
				 display->Line (-0.1F, 0.0F, 0.0F, -0.1F);
			 }
         }

         display->Line (-1.0F, 0.0F, 1.0F, 0.0F);
         display->Line (0.0F, -1.0F, 0.0F, 1.0F);

         // Add scale reference
         len = 1500.0F / groundMapRange;
         display->Line (-0.75F, 0.75F, -0.75F, 0.8F);
         display->Line (-0.75F, 0.8F, -0.75F + len, 0.8F);
         display->Line (-0.75F + len, 0.8F, -0.75F + len, 0.75F);

		 if(g_bRealisticAvionics && g_bAGRadarFixes)
		 {	
			 if(flags & DBS1)
			 {
				 float len = 0.065F;
				 display->Line (0.25F, -len, 0.25F, len);
				 display->Line (-0.25F, -len, -0.25F, len);
				 display->Line (len, 0.25F, -len, 0.25F);
				 display->Line (len, -0.25F, -len, -0.25F);
			 }
		 }
		 else
		 {
			 if (flags & DBS1)
			 {
				 // Expansion Cues
				 display->Line ( 0.25F,  0.1F,  0.25F, -0.1F);
				 display->Line (-0.25F,  0.1F, -0.25F, -0.1F);
				 display->Line ( 0.1F,  0.25F, -0.1F,  0.25F);
				 display->Line ( 0.1F, -0.25F, -0.1F, -0.25F);
			 }
		 }
	  }
	  
	  display->SetViewport (vpLeft, vpTop, vpRight, vpBottom);
	  //MI
	  if(g_bRealisticAvionics && g_bAGRadarFixes)
	  {
		  static float MAX_GAIN = 25.0F;
		  float x, y = 0;
		  GetButtonPos(19, &x, &y);
		  x += 0.02F;
		  y = 0.95F;
		  float y1 = 0.2F;
		  float diff = y - y1;
		  float step = y1 / MAX_GAIN;
		  float pos = GainPos * step + (5*step);
		  //add the gain range
		  display->Line(x, y,x + 0.07F, y);
		  display->Line(x, y,x, y - y1);
		  display->Line(x, y - y1,x + 0.07F, y - y1);
		  //Gain
		  mlTrig trig;
		  float Angle = 45.0F * DTR;
		  float lenght = 0.08F;
		  mlSinCos(&trig, Angle);
		  float pos1 = trig.sin * lenght;
		  display->AdjustOriginInViewport(x,diff + pos);
		  display->Line(0.0F,0.0F,pos1, pos1);
		  display->Line(0.0F,0.0F,pos1,-pos1);
		  display->CenterOriginInViewport();
	  } 

	  // Common Radar Stuff
	  DrawScanMarkers();
	  DrawAzElTicks();

	  // Draw Range arrows
	  if (IsAGDclt(Arrows) == FALSE) 
		  DrawRangeArrows();
	  if (IsAADclt(Rng) == FALSE) 
		  DrawRange();

	  display->SetColor(tmpColor);
	  if (IsAGDclt(MajorMode) == FALSE) 
	  {
		  // Add Buttons
		  switch (mode)
		  {
		  case GM:
			  LabelButton (0, "GM");
			  break;
			  
		  case GMT:
			  LabelButton (0, "GMT");
			  break;
			  
		  case SEA:
			  LabelButton (0, "SEA");
			  break;
		  }
	  }
	  if(IsAGDclt(SubMode) == FALSE)
	  {
		  if(g_bRealisticAvionics && g_bAGRadarFixes)
		  {
			  if(IsSet(AutoAGRange))
				  LabelButton(1,"AUTO");
			  else
				  LabelButton (1, "MAN");
		  }
		  else
			  LabelButton(1, "MAN");
	  } 
	  if (IsAGDclt(Fov) == FALSE) 
	  {
		  if (flags & NORM)
		  {
			  //MI
			  if(!g_bRealisticAvionics || !g_bAGRadarFixes)
				  LabelButton (2, "NRM");
			  else
				  LabelButton (2, "NORM");
		  }
		  else
		  {
			  if (flags & EXP)
				  LabelButton (2, "EXP");
			  else if (flags & DBS1)
				  LabelButton (2, "DBS1");
			  else if (flags & DBS2)
				  LabelButton (2, "DBS2");
		  }
	  }
	  if (IsAGDclt(Ovrd) == FALSE) 
		  LabelButton (3, "OVRD", NULL, !IsEmitting());
	  if (IsAGDclt(Cntl) == FALSE) 
		  LabelButton (4, "CNTL", NULL, IsSet(CtlMode));

	  if (IsSet(MenuMode|CtlMode)) 
		  MENUDisplay();
	  else 
	  {
		  if(IsAGDclt(BupSen) == FALSE) 
			  LabelButton (5, "BARO");
		  if (IsAGDclt(FzSp) == FALSE) 
		  {
			  LabelButton (6, "FZ", NULL, IsSet(FZ));	  
			  LabelButton (7, "SP", NULL, IsSet(SP));
			  LabelButton (9, "STP", NULL, !IsSet(SP));
		  }
		  if (IsAGDclt(Cz) == FALSE) 
			  LabelButton (8, "CZ");
		  
		  AGBottomRow ();
		  
		  if (IsAGDclt(AzBar) == FALSE) 
		  {
			  //MI
			  if(g_bRealisticAvionics && g_bAGRadarFixes)
			  {
				  char str[10] = "";
				  sprintf(str,"%.0f", displayAzScan * 0.1F * RTD);
				  ShiAssert (strlen(str) < sizeof(str));
				  LabelButton (17, "A", str);
			  }
			  else
				  LabelButton (17, "A", "6");
		  }
	  }
   }

   /*------------------*/
   /* Auto Range Scale */
   /*------------------*/
   if (lockedTarget)
   {
	   //MI
	   if(g_bRealisticAvionics && g_bAGRadarFixes)
	   {
		   if(IsSet(AutoAGRange))
		   {
			   if (lockedTarget->localData->range > 0.9F * tdisplayRange && curRangeIdx < NUM_RANGES - 1)
				   rangeChangeCmd = 1;
			   else if (lockedTarget->localData->range < 0.4F * tdisplayRange && curRangeIdx > 0)
				   rangeChangeCmd = -1;
		   }
	   }
	   else
	   {
		   if (lockedTarget->localData->range > 0.9F * tdisplayRange && curRangeIdx < NUM_RANGES - 1)
			   rangeChangeCmd = 1;
		   else if (lockedTarget->localData->range < 0.4F * tdisplayRange && curRangeIdx > 0)
			   rangeChangeCmd = -1;
	   }
	   //MI
	   if(g_bRealisticAvionics && g_bAGRadarFixes)
	   {
		   scanDir = ScanNone;
		   beamAz = lockedTarget->localData->az;
		   beamEl = lockedTarget->localData->el;
	   }
   }
}


void RadarDopplerClass::AddTargetReturnCallback( void* self, RenderGMRadar* renderer )
{
	((RadarDopplerClass*)self)->AddTargetReturns(renderer);
}


void RadarDopplerClass::AddTargetReturns( RenderGMRadar* renderer )
{
float			dx, dy;
float			rx, ry;
float			cosAz, sinAz;
mlTrig			trig;
GMList			*curList;
float       minDist = 0.05F;

   mlSinCos (&trig, headingForDisplay);
   cosAz =  trig.cos;
   sinAz = -trig.sin;

   // Offset the spots correctly
   if (!(flags & FZ))
   {
      if (flags & NORM)
      {
         // Find center of scope
         GMXCenter = platform->XPos() + tdisplayRange * trig.cos * 0.5F;
         GMYCenter = platform->YPos() + tdisplayRange * trig.sin * 0.5F;
      }
      else
      {
         GMXCenter = GMat.x;
         GMYCenter = GMat.y;
      }
   }

   // Draw the appropriate type of targets
   // TODO:  We should select _any_ large targets for GM, and only moving targets for GMT/SEA
   if (mode == GM) {
      curList = GMFeatureListRoot;
   } else {	// GMT or SEA
      curList = GMMoverListRoot;
   }

   // Clear target under cursor;
   targetUnderCursor = FalconNullId;

   while (curList)
   {
      dx = curList->Object()->XPos() - GMXCenter;
      dy = curList->Object()->YPos() - GMYCenter;

      // Note the axis switch from NED to screen
      ry = cosAz * dx - sinAz * dy;
      rx = sinAz * dx + cosAz * dy;

      if (fabs(rx) > groundMapRange ||
          fabs(ry) > groundMapRange)
      {
         curList = curList->next;
         continue;
      }
      rx /= groundMapRange;
      ry /= groundMapRange;

      // Check for scan width NOTE 0.57 = tan(30) (90.0 - the azimuth limit)
      if ((ry + 1.0F) / (fabs (rx) + 0.001F) > 0.57F || fabs(rx) > 1.0F)
      {
		  if (curList->Object()->IsSim() && ((SimBaseClass*)curList->Object())->IsAwake()) 
		  {
			  DrawableObject *drawable = ((SimBaseClass*)curList->Object())->drawPointer;
			  //MI
			  if(g_bRealisticAvionics && g_bAGRadarFixes)
			  {
				  if(!lockedTarget)
					  renderer->DrawBlip( drawable );
			  }
			  else
				  renderer->DrawBlip( drawable );
		  } 
		  else 
		  {
			  //MI
			  if(g_bRealisticAvionics && g_bAGRadarFixes)
			  {
				  if(!lockedTarget)
					  renderer->DrawBlip( curList->Object()->XPos(), curList->Object()->YPos() );
			  }
			  else
				  renderer->DrawBlip( curList->Object()->XPos(), curList->Object()->YPos() );
		  }
      }
      else
      {
         if (lockedTarget && curList->Object() == lockedTarget->BaseData())
         {
            DropGMTrack();
         }
      }

      if (flags & NORM)
      {
         ry -= cursorY;
         rx -= cursorX;
      }

      if (fabs(rx) < minDist && fabs (ry) < minDist)
      {
         minDist = min ( min (minDist, (float)fabs(rx)), (float)fabs(ry));
         targetUnderCursor = curList->Object()->Id();
      }

      curList = curList->next;
   }
}


// Only used by virtual cockpit now...
void RadarDopplerClass::AddTargetReturnsOldStyle (GMList* curList)
{
float dx=0.0F, dy=0.0F;
float rx=0.0F, ry=0.0F;
float cosAz=0.0F, sinAz=0.0F;
mlTrig trig={0.0F};
float minDist = groundMapRange;

   mlSinCos (&trig, headingForDisplay);
   cosAz =  trig.cos;
   sinAz = -trig.sin;

   // Offset the spots correctly
   if (!(flags & FZ))
   {
      if (flags & NORM)
      {
         // Find center of scope
         GMXCenter = platform->XPos() + tdisplayRange * trig.cos * 0.5F;
         GMYCenter = platform->YPos() + tdisplayRange * trig.sin * 0.5F;
      }
      else
      {
         GMXCenter = GMat.x;
         GMYCenter = GMat.y;
      }
   }

   // Draw the spots
   while (curList)
   {
      // Check distance from radar patch center
      dx = curList->Object()->XPos() - GMat.x;
      dy = curList->Object()->YPos() - GMat.y;
      if (fabs(dx) > groundMapRange ||
          fabs(dy) > groundMapRange)
      {
         curList = curList->next;
         continue;
      }

      // Now check position on screen
      dx = curList->Object()->XPos() - GMXCenter;
      dy = curList->Object()->YPos() - GMYCenter;

      // Note the axis switch from NED to screen
      ry = cosAz * dx - sinAz * dy;
      rx = sinAz * dx + cosAz * dy;

      if (fabs(rx) > groundMapRange ||
          fabs(ry) > groundMapRange)
      {
         curList = curList->next;
         continue;
      }
      rx /= groundMapRange;
      ry /= groundMapRange;

      // Check for scan width NOTE 0.57 = tan(30) (90.0 - the azimuth limit)
      if ((ry + 1.0F) / (fabs (rx) + 0.001F) > 0.57F || fabs(rx) > 1.0F)
      {
		  //MI
		  if(g_bRealisticAvionics && g_bAGRadarFixes)
		  {
			  if(!lockedTarget)
			  {
				  display->AdjustOriginInViewport (rx, ry);
				  DrawSymbol (Solid, 0.0F, 0);
				  display->AdjustOriginInViewport (-rx, -ry); 
			  }
		  }
		  else
		  {
			  display->AdjustOriginInViewport (rx, ry);
			  DrawSymbol (Solid, 0.0F, 0);
			  display->AdjustOriginInViewport (-rx, -ry);      
		  }
	  }
      else
      {
         if (lockedTarget && curList->Object() == lockedTarget->BaseData())
         {
            DropGMTrack();
         }
      }

      if (flags & NORM)
      {
         ry -= cursorY;
         rx -= cursorX;
      }

      if (fabs(rx) < minDist && fabs (ry) < minDist)
      {
         minDist = (float)min ( min (minDist, fabs(rx)), fabs(ry));
         targetUnderCursor = curList->Object()->Id();
      }

      curList = curList->next;
   }
}

void RadarDopplerClass::DoGMDesignate (GMList* curList)
{
float dx, dy;
float rx, ry;
float cosAz, sinAz;
mlTrig trig;
float minDist = 0.05F;

   mlSinCos (&trig, headingForDisplay);
   cosAz =  trig.cos;
   sinAz = -trig.sin;

   // Offset the spots correctly
   if (!(flags & FZ))
   {
      if (flags & NORM)
      {
         // Find center of scope
         GMXCenter = platform->XPos() + tdisplayRange * trig.cos * 0.5F;
         GMYCenter = platform->YPos() + tdisplayRange * trig.sin * 0.5F;
      }
      else
      {
         GMXCenter = GMat.x;
         GMYCenter = GMat.y;
      }
   }

   // Draw the spots
   while (curList)
   {
      dx = curList->Object()->XPos() - GMXCenter;
      dy = curList->Object()->YPos() - GMYCenter;

      // Note the axis switch from NED to screen
      ry = cosAz * dx - sinAz * dy;
      rx = sinAz * dx + cosAz * dy;

      if (fabs(rx) > groundMapRange ||
          fabs(ry) > groundMapRange)
      {
         curList = curList->next;
         continue;
      }
      rx /= groundMapRange;
      ry /= groundMapRange;

      if (flags & NORM)
      {
         ry -= cursorY;
         rx -= cursorX;
      }

      if (fabs(rx) < minDist && fabs (ry) < minDist)
      {
         minDist = min ( min (minDist, (float)fabs(rx)), (float)fabs(ry));
         if (designateCmd)
         {
			   SetGroundTarget(curList->Object());
         }

         targetUnderCursor = curList->Object()->Id();
      }

      curList = curList->next;
   }
}

void RadarDopplerClass::FreeGMList (GMList* theList)
{
GMList* tmp;

   while (theList)
   {
      tmp = theList;
      theList = theList->next;
      tmp->Release();
   }
}

RadarDopplerClass::GMList::GMList (FalconEntity* obj)
{
   F4Assert (obj);
   next = NULL;
   prev = NULL;
   object = obj;
   VuReferenceEntity (object);
   count = 1;
}

void RadarDopplerClass::GMList::Release(void)
{
   VuDeReferenceEntity (object);
   count --;

   if (count == 0)
      delete (this);
}

void RadarDopplerClass::SetGroundTarget (FalconEntity* newTarget)
{
float cosAz, sinAz;
mlTrig trig;

	if (lockedTarget && newTarget == lockedTarget->BaseData())
		return;

	SetSensorTargetHack( newTarget );

	if (newTarget)
	{
      if (flags & NORM)
      {
         mlSinCos (&trig, groundLookAz);
         cosAz = trig.cos;
         sinAz = trig.sin;

         viewOffsetInertial.x = lockedTarget->BaseData()->XPos() - GMat.x;
         viewOffsetInertial.y = lockedTarget->BaseData()->YPos() - GMat.y;

         viewOffsetRel.x =  cosAz*viewOffsetInertial.x + sinAz*viewOffsetInertial.y;
         viewOffsetRel.y = -sinAz*viewOffsetInertial.x + cosAz*viewOffsetInertial.y;

         viewOffsetRel.x /= tdisplayRange * 0.5F;
         viewOffsetRel.y /= tdisplayRange * 0.5F;
      }
      else
      {
         GMat.x = lockedTarget->BaseData()->XPos();
         GMat.y = lockedTarget->BaseData()->YPos();
         viewOffsetInertial.x = viewOffsetRel.x = 0.0F;
         viewOffsetInertial.y = viewOffsetRel.y = 0.0F;
      }
   }
}

void RadarDopplerClass::SetAGFreeze (int val)
{
   if (val)
      SetFlagBit (FZ);
   else
      ClearFlagBit(FZ);
}

void RadarDopplerClass::SetAGSnowPlow (int val)
{
   if (val)
      SetFlagBit (SP);
   else
      ClearFlagBit(SP);
}

void RadarDopplerClass::SetAGSteerpoint (int val)
{
   if (!val)
      SetFlagBit (SP);
   else
      ClearFlagBit(SP);
}

void RadarDopplerClass::ToggleAGfreeze()
{
   if (flags & FZ)
      ClearFlagBit (FZ);
   else
      SetFlagBit(FZ);
}

void RadarDopplerClass::ToggleAGsnowPlow()
{
   if (flags & SP)
      ClearFlagBit (SP);
   else
      SetFlagBit(SP);
}

void RadarDopplerClass::ToggleAGcursorZero()
{
   viewOffsetInertial.x = viewOffsetInertial.y = 0.0F;
   viewOffsetRel.x = viewOffsetRel.y = 0.0F;
}

int RadarDopplerClass::IsAG (void)
{
int retval;

   if (mode == GM || mode == GMT || mode == SEA)
// 2000-10-04 MODIFIED BY S.G. NEED TO KNOW WHICH MODE WE ARE IN
//    retval = TRUE;
      retval = mode;
   else
      retval = FALSE;

   return retval;
}

void RadarDopplerClass::GetAGCenter (float* x, float* y)
{
   *x = GMat.x;
   *y = GMat.y;

   if ((flags & NORM))
   {
      *x += viewOffsetInertial.x;
      *y += viewOffsetInertial.y;
   }
}

// JPO - bottom row of MFD buttons.
void RadarDopplerClass::AGBottomRow ()
{
   if (g_bRealisticAvionics) {
       if (IsAGDclt(Dclt) == FALSE) LabelButton(10, "DCLT", NULL, IsSet(AGDecluttered));
       if (IsAGDclt(Fmt1) == FALSE) DefaultLabel(11);
       //if (IsAGDclt(Fmt2) == FALSE) DefaultLabel(12);	MI moved downwards
       if (IsAGDclt(Fmt3) == FALSE) DefaultLabel(13);
       if (IsAGDclt(Swap) == FALSE) DefaultLabel(14);
	   //MI RF Switch info
	   if(SimDriver.playerEntity && (SimDriver.playerEntity->RFState == 1 ||
			SimDriver.playerEntity->RFState == 2))
	   {
		   FackClass* mFaults = ((AircraftClass*)(SimDriver.playerEntity))->mFaults;
		   if(mFaults && !(mFaults->GetFault(FaultClass::fcc_fault) == FaultClass::xmtr))
		   {
			   if(SimDriver.playerEntity->RFState == 1)
				   LabelButton(12, "RDY", "QUIET");
			   else
				   LabelButton(12, "RDY", "SILENT");
		   }
	   }
	   else
		   if (IsAGDclt(Fmt2) == FALSE) DefaultLabel(12);
   } else {
       LabelButton (10, "DCLT", NULL, IsSet (AGDecluttered));
       LabelButton (11, "SMS");
       LabelButton (13, "MENU", NULL, 1);//me123
       LabelButton (14, "SWAP");
   }
}