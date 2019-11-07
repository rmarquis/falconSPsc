#include "stdhdr.h"
#include "digi.h"
#include "aircrft.h"
#include "unit.h"
#include "sensors.h"
#include "object.h"
#include "Graphics\Include\drawbsp.h"
#include "simobj.h"
#include "simdrive.h"
#include "sms.h"
/* S.G. */ #include "vehrwr.h"
/* S.G. */ #include "RadarDigi.h"
/* S.G. */ #include "visual.h"
/* S.G. */ #include "irst.h"
/* S.G. FOR UP TO TWO MISSILES ON ITS WAY TO A TARGET */ #include "aircrft.h"
/* S.G. FOR UP TO TWO MISSILES ON ITS WAY TO A TARGET */ #include "airframe.h"
/* S.G. FOR UP TO TWO MISSILES ON ITS WAY TO A TARGET */ #include "simWeapn.h"
//#define DEBUG_TARGETING 

extern int g_nLowestSkillForGCI; // 2002-03-12 S.G. Replaces the hardcoded '3' for skill test
extern float g_fSearchSimTargetFromRangeSqr;	// 2002-03-15 S.G. Will lookup Sim target instead of using the campain target from this range

SimObjectType* MakeSimListFromVuList (AircraftClass *self, SimObjectType* targetList, VuFilteredList* vuList);
FalconEntity* SpikeCheck (AircraftClass* self, FalconEntity *byHim = NULL, int *data = NULL);// 2002-02-10 S.G.

#ifdef CHECK_PROC_TIMES
static ulong gPart1 = 0;
static ulong gPart2 = 0;
static ulong gPart3 = 0;
static ulong gPart4 = 0;
static ulong gPart5 = 0;
static ulong gPart6 = 0;
static ulong gPart7 = 0;
static ulong gPart8 = 0;
static ulong gWhole = 0;
extern int gameCompressionRatio;
#endif

void DigitalBrain::DoTargeting(void)
{
#ifdef CHECK_PROC_TIMES
	gPart1 = 0;
	gPart2 = 0;
	gPart3 = 0;
	gPart4 = 0;
	gPart5 = 0;
	gPart6 = 0;
	gPart7 = 0;
	gPart8 = 0;
	gWhole = GetTickCount();
#endif
FalconEntity* campTarget = NULL;
float rngSqr = FLT_MAX;

   // Use parent target list for start
   targetList = self->targetList;

   /*-----------------------------*/
   /* Calculate relative geometry */
   /*-----------------------------*/
   if (targetPtr)
   {
      targetData = targetPtr->localData;
      ataDot  = (targetData->ata - lastAta) / SimLibMajorFrameTime;
      lastAta   = targetData->ata;
   }
   else
   {
      targetData = NULL;
   }

   // only flight leads update their target lists and scan.
   // furthermore they only do this when in waypoint mode and
   // we're in not in campaign or tactical
   // When they decide to engage, they'll fix a target (or flight)
   // and then begin continuous monitoring of that target only in
   // an engagement mode.  This will occur up until the engagement is
   // broken (or we die in engagement).  Returning to waypoint mode
   // will return to scanning.

   if (SimLibElapsedTime > self->nextTargetUpdate)
   {
      // If we are lead, or lead is player, look for a target
      if (!isWing || (mDesignatedObject == FalconNullId && flightLead && flightLead->IsSetFlag(MOTION_OWNSHIP)))
      {
         if (missionClass != AAMission && !missionComplete && agDoctrine == AGD_NONE)
            SelectGroundWeapon();

#ifdef CHECK_PROC_TIMES
	gPart1 = GetTickCount();
#endif
         campTarget = CampTargetSelection();
         if (campTarget)
         {
            // Use campaign target
            if ( campTarget->IsCampaign() && ((CampBaseClass*)campTarget)->GetComponents())
            {
               self->targetList = MakeSimListFromVuList (self, self->targetList, ((CampBaseClass*)campTarget)->GetComponents());
            }
// 2002-02-25 MODIFIED BY S.G. NO NO NO, AGGREGATED Campaign object should make it here as well otherwise AI will not target them until they enter the 20 NM limit below.
            // Campaign returned a sim entity, deal with it
//          else if (campTarget->IsSim())
            else if (campTarget->IsSim() || (campTarget->IsCampaign() && ((CampBaseClass *)campTarget)->IsAggregate()))
            {
				// Put it directly into our target list
#ifdef DEBUG
				SimObjectType *newTarg = new SimObjectType( OBJ_TAG, self, campTarget );
#else
				SimObjectType *newTarg = new SimObjectType( campTarget );
#endif
				self->targetList = InsertIntoTargetList (self->targetList, newTarg);
            }

            rngSqr = (campTarget->XPos() - self->XPos())*(campTarget->XPos() - self->XPos()) +
               (campTarget->YPos() - self->YPos())*(campTarget->YPos() - self->YPos());
         }
#ifdef CHECK_PROC_TIMES
	gPart1 = GetTickCount() - gPart1;
	gPart2 = GetTickCount();
#endif
         // If the campaign didn't give us a target or we're so close that campaign targeting isn't
		// reliable, then build our own target list.
		// TODO:  We should really put both the campaign suggested target and the local environment
		// objects into our target list and then make a weighted choice among all of the entities.
// 2000-11-17 MODIFIED BY S.G. WILL TRY DIFFERENT VALUES AND SEE HOW IT AFFECTS GAMEPLAY/FPS (5 NM IS THE DEFAULT). 20 NM SEEMS FINE (NO REAL FPS LOSS AND IMPROVED AI TARGET SORTING)
//       if (!campTarget || rngSqr < (5.0F * NM_TO_FT)*(5.0F * NM_TO_FT))
// SYLVAINLOOK in your RP5 code, this was brought back to 5 NM...
// 2002-03-15 MODIFIED BY S.G. Now uses g_fSearchSimTargetFromRangeSqr so we can tweak it
//       if (!campTarget || rngSqr < (20.0F * NM_TO_FT)*(20.0F * NM_TO_FT))
         if (!campTarget || rngSqr < g_fSearchSimTargetFromRangeSqr)
         {
            // Need a target list for threat checking
         		self->targetList = UpdateTargetList (self->targetList, self, SimDriver.combinedList);
         }
      }
      else //wingman
      {
         if (mDesignatedObject != FalconNullId)
         {
      		campTarget = (FalconEntity*) vuDatabase->Find(mDesignatedObject);	// Lookup target in database
            if (campTarget && campTarget->IsCampaign() && ((CampBaseClass*)campTarget)->GetComponents())
            {
               self->targetList = MakeSimListFromVuList (self, self->targetList, ((CampBaseClass*)campTarget)->GetComponents());
            }
            else if (campTarget && campTarget->IsSim())
            {
				// Put it directly into our target list
#ifdef DEBUG
				SimObjectType *newTarg = new SimObjectType( OBJ_TAG, self, campTarget );
#else
				SimObjectType *newTarg = new SimObjectType( campTarget );
#endif
				self->targetList = InsertIntoTargetList (self->targetList, newTarg);
            }
         }

         // Check for nearby threats and kill them
		// TODO:  Should we do a range check here like we do for leads above, or just trust the campaign?
         if (!campTarget)
         {
//me123 we need it for bvr reactions            if (missionClass == AAMission || missionComplete)
                  self->targetList = UpdateTargetList (self->targetList, self, SimDriver.combinedList);
         }

		 // edg: kruft check -- it has been observed that wingman's target
		 // lists are holding refs to sim objects that are no longer awake.
		 // This will remove them.
		 SimObjectType *simobj = self->targetList;
		 SimObjectType *tmpobj;
		 while ( simobj )
		 {
			 if ( simobj->BaseData()->IsSim() && !((SimBaseClass*)simobj->BaseData())->IsAwake() )
			 {
				 tmpobj = simobj->next;

				 // remove from chain
				 if ( simobj->prev )
				 	simobj->prev->next = simobj->next;
				 else
					 self->targetList = simobj->next;
				 if ( simobj->next )
				 	simobj->next->prev = simobj->prev;

				 simobj->next = NULL;
				 simobj->prev = NULL;
		 		 simobj->Release( SIM_OBJ_REF_ARGS );
				 simobj = tmpobj;

			 }
			 else
			 {
				simobj = simobj->next;
			 }
		 }
      }

      CalcRelGeom(self, self->targetList, ((AircraftClass*)self)->vmat, 1.0F / SimLibMajorFrameTime);

      targetList = self->targetList;
      // Sensors
      ((AircraftClass *)self)->RunSensors();
      self->nextTargetUpdate = SimLibElapsedTime + self->targetUpdateRate;

      // This is a timed event, so lets check gas here
      FuelCheck();

      // if wingy, check for reaching IP
      IPCheck();
   }

   // Merge the data for each threat/target
   SensorFusion();

#ifdef DEBUG_TARGETING
   // What is my target anyway?
   if (self->IsAwake())
   {
      if (targetPtr)
      {
         if (targetPtr->BaseData()->IsSim())
         {
            tmpDraw = (DrawableBSP*)((SimBaseClass*)targetPtr->BaseData())->drawPointer;
            if (tmpDraw)
               MonoPrint ("%s-%d set target %s\n", ((DrawableBSP*)self->drawPointer)->Label(), isWing + 1,
                  ((DrawableBSP*)tmpDraw)->Label());
         }
         else
         {
            MonoPrint ("%s-%d set camp target %s\n", ((DrawableBSP*)self->drawPointer)->Label(), isWing + 1,
               ((UnitClass*)targetPtr->BaseData())->GetUnitClassName());
         }
      }
      else
      {
         MonoPrint ("%s-%d no target\n", ((DrawableBSP*)self->drawPointer)->Label(), isWing + 1);
      }
   }
#endif
}


void DigitalBrain::TargetSelection(void)
{
float threatTime = MAX_THREAT_TIME - 1;
float targetTime = MAX_TARGET_TIME - 1;

FalconEntity* curSpike = NULL;
SimObjectType* objectPtr;
SimObjectType* maxTargetPtr;
SimObjectType* maxThreatPtr;
BOOL foundSpike = FALSE;

   if (missionClass != AAMission && !missionComplete)
   {
  		threatTime *= 0.25f;
  		targetTime *= 0.25f;
   }

   // stay on current target
   if ( targetPtr &&
      (targetPtr->BaseData()->IsExploding() || targetPtr->BaseData()->IsDead() ||
      (targetPtr->BaseData()->IsSim() && ((SimBaseClass*)targetPtr->BaseData())->IsSetFlag(PILOT_EJECTED))))
   {
		ClearTarget();		
   }

	maxTargetPtr = NULL;
	maxThreatPtr = NULL;

   // Check for spike, but not for rookies
   if (SkillLevel() != 0)
      curSpike = SpikeCheck (self);

   objectPtr = targetList;
   //while (objectPtr) // JB 010224 CTD
	 while (objectPtr && !F4IsBadReadPtr(objectPtr, sizeof(SimObjectType))) // JB 010224 CTD
	{
		if ( objectPtr->BaseData() == NULL ||
					F4IsBadCodePtr((FARPROC) objectPtr->BaseData()) || // JB 010224 CTD
		     objectPtr->BaseData()->IsSim() &&
           (objectPtr->BaseData()->IsWeapon() || objectPtr->BaseData()->IsEject() ||
           ((SimBaseClass*)objectPtr->BaseData())->IsSetFlag(PILOT_EJECTED)) ||
           objectPtr->localData->range > maxEngageRange)
		{
			objectPtr = objectPtr->next;
			continue;
		}
// S.G.ADDED SECTION. MAKE SUR OUR SENSOR IS SEEING HIM BEFORE WE DO ANYTHING WITH HIM...
		SimObjectType* tmpLock = NULL;                            // WILL HOLD THE LOCKED OBJECT IF THE SENSOR COULD LOCK IT
		for (int i=0; i<self->numSensors; i++) {
			ShiAssert( self->sensorArray[i] );
			if (objectPtr->localData->sensorState[self->sensorArray[i]->Type()] > SensorClass::NoTrack) {// CAN SEE AND DETECT IT
				tmpLock = objectPtr;                       // YES, THEN LOCK ONTO THE TARGET
				break;
			}
		}
		if (!tmpLock) {                              // IF NO SENSOR IS SEEING THIS GUY, HOW CAN WE TRACK HIM?
// 2001-03-16 ADDED BY S.G. THIS IS OUR GCI IMPLEMENTATION... EVEN IF NO SENSORS ON HIM, ACE AND VETERAN GETS TO USE GCI 
			if (SkillLevel() < g_nLowestSkillForGCI || objectPtr->localData->range >= 40.0F * NM_TO_FT || !((objectPtr->BaseData()->IsSim() && ((SimBaseClass*)objectPtr->BaseData())->GetCampaignObject()->GetSpotted(self->GetTeam())) || (objectPtr->BaseData()->IsCampaign() && ((CampBaseClass*)objectPtr->BaseData())->GetSpotted(self->GetTeam())))) {
// END OF ADDED SECTION
				objectPtr = objectPtr->next;
				continue;
			}
		}
// END OF ADDED SECTION

// 2000-09-21 ADDED BY S.G. IF THE TARGET IS DYING, FIND ANOTHER ONE
//		if (((SimBaseClass *)objectPtr->BaseData())->pctStrength <= 0.0f) // Dying target have a damage less than 0.0f
// 2001-08-04 MODIFIED BY S.G. objectPtr CAN BE A CAMPAIGN OBJECT. NEED TO ACCOUNT FOR THIS
		if (objectPtr->BaseData()->IsSim() && ((SimBaseClass *)objectPtr->BaseData())->pctStrength <= 0.0f) // Dying target have a damage less than 0.0f
		{
			objectPtr = objectPtr->next;
			continue;
		}
// END OF ADDED SECTION EXCEPT FOR THE ELSE ADDED TO THE NEXT 'if' STATEMENT
		// Increase priority of spike
		else if (objectPtr->BaseData() == curSpike)
		{
			objectPtr->localData->threatTime *= 0.8F + (1.0F - SkillLevel()/4.0F) * 0.8F;
			foundSpike = TRUE;
		}

// 2000-09-12 ADDED BY S.G. IF THE TARGET ALREADY HAS A MISSILES ON ITS WAY AND WE DIDN'T SHOOT IT, DECREASE ITS PRIORITY BY 4. THIS WILL MAKE THE CURRENT LESS LIKEABLE 
// 2001-08-04 MODIFIED BY S.G. objectPtr CAN BE A CAMPAIGN OBJECT. NEED TO ACCOUNT FOR THIS
//		else if (((SimBaseClass *)objectPtr->BaseData())->platformAngles && // JB 010119 CTD sanity check -- if platformAngles is NULL, something is seriously screwed up and we'll likely CTD checking incomingMissile.
//			     ((SimBaseClass *)objectPtr->BaseData())->incomingMissile[0] && ((SimWeaponClass *)((SimBaseClass *)objectPtr->BaseData())->incomingMissile[0])->parent != self)
		else if (objectPtr->BaseData()->IsSim() && ((SimBaseClass *)objectPtr->BaseData())->incomingMissile[0] && ((SimWeaponClass *)((SimBaseClass *)objectPtr->BaseData())->incomingMissile[0])->parent != self)
			objectPtr->localData->targetTime *= 4.0f;
// END OF ADDED SECTION

      // Increase priority of current target
      if (targetPtr && objectPtr->BaseData() == targetPtr->BaseData()) {
         objectPtr->localData->targetTime *= 0.5f;
		 objectPtr->localData->threatTime *= 0.5f;

      }

		if (objectPtr->localData->threatTime < threatTime)
		{
			threatTime = objectPtr->localData->threatTime;
			maxThreatPtr = objectPtr;
		}

		if (objectPtr->localData->targetTime < targetTime)
		{
			targetTime = objectPtr->localData->targetTime;
			maxTargetPtr = objectPtr;
		}

      objectPtr = objectPtr->next;
	}

	if (missionType != AMIS_AIRCAV )//me123missionClass == AAMission || missionComplete)
   {
//me123 we target stuff now even on non a-a missions
// the bvr rutine will make the corect action (defensive, engage or waypoint)
	   if (threatTime < targetTime && maxThreatPtr && !maxThreatPtr->BaseData()->OnGround() )
		  SetTarget(maxThreatPtr);
	   else if (targetTime < MAX_TARGET_TIME && maxTargetPtr && !maxTargetPtr->BaseData()->OnGround() )
		  SetTarget(maxTargetPtr);
	   else if (curSpike && !curSpike->OnGround() && !foundSpike)
	   {
		SetThreat (curSpike);
	   }
	   else
	   {
		  ClearTarget();
		  AddMode (WaypointMode);
	   }
   }
   else
   {
	   // airground mission, if pickup and drop off completely ignore
	   // threats
	   if ( missionType == AMIS_AIRCAV )
	   {
		  ClearTarget();
		  AddMode (WaypointMode);
	   }
	   else if (threatTime < targetTime &&
	   	   maxThreatPtr &&
		   maxThreatPtr->BaseData()->IsAirplane() &&
		   !maxThreatPtr->BaseData()->OnGround() &&
		   maxThreatPtr->localData->range < 5.0f * NM_TO_FT )
	   {
		  SetTarget(maxThreatPtr);
	   }
	   else if (targetTime < MAX_TARGET_TIME &&
	            maxTargetPtr &&
				maxTargetPtr->BaseData()->IsAirplane() &&
				!maxTargetPtr->BaseData()->OnGround() &&
				maxTargetPtr->localData->range < 5.0f * NM_TO_FT )
	   {
		  SetTarget(maxTargetPtr);
	   }
	   else if (curSpike &&
	   			curSpike->IsAirplane() &&
				!curSpike->OnGround() &&
				!foundSpike)
	   {
		  float dx, dy, dist;

		  dx = self->XPos() - curSpike->XPos();
		  dy = self->YPos() - curSpike->YPos();

		  dist = dx * dx + dy * dy;

		  if ( dist < 5.0f * NM_TO_FT * 5.0f * NM_TO_FT ) 
		   SetThreat (curSpike);
		  else
		  {
			  ClearTarget();
		      AddMode (WaypointMode);
		  }
	   }
	   else
	   {
		  ClearTarget();
		  AddMode (WaypointMode);
	   }
   }

   // Turn on jamming if possible
   if (curSpike && !jammertime || (flightLead && flightLead->IsSPJamming()))
   {
      if (self->HasSPJamming())
      {
         self->SetFlag(ECM_ON);
		 jammertime = SimLibElapsedTime + 60000.0f;
      }
   }
   else if (jammertime && jammertime < SimLibElapsedTime)
   {
	  jammertime = 0;
      self->UnSetFlag(ECM_ON);
   }
}

/*
** Name: CampTargetSelection
** Description:
**		When running in the campaign, we get our marching orders (targets)
**		from the campaign entities.
**		NOTE: This function is intended only for Air-Air.  Air-Ground is
**		another function using a different target ptr
*/
FalconEntity* DigitalBrain::CampTargetSelection(void)
{
	UnitClass *campUnit = (UnitClass *)self->GetCampaignObject();
	FalconEntity *target;

	// at this point we have no target, we're going to ask the campaign
	// to find out what we're supposed to hit
	// tell unit we haven't done any checking on it yet
	campUnit->UnsetChecked();

	// choose target.  I assume if this returns 0, no target....
	if ( !campUnit->ChooseTarget() )
		return NULL;

	// get the target
	target = campUnit->GetTarget();

	// do we have a target?
	if ( !target )
		return NULL;


	// get tactic -- not doing anything with it now
	campUnit->ChooseTactic();
	campTactic = campUnit->GetUnitTactic();

	// set ground target pointer if on ground!
	// never, ever set targetPtr to ground object
// 2000-09-27 MODIFIED BY S.G. AI NEED TO SET ITS TARGET POINTER IF IT HAS REACHED ITS IP WAYPOINT AS WELL
//	if ( target->OnGround() && (missionClass == AAMission || missionComplete) && hasWeapons)
	if ( target->OnGround() && (missionClass == AAMission || missionComplete || IsSetATC(ReachedIP)) && hasWeapons)
	{
		if ( !groundTargetPtr )
		{
			SetGroundTarget( target );
			SetupAGMode( NULL, NULL );
		}
		return NULL;
	}

#ifdef DEBUG_TARGETING
{
float rng;

   rng = (float)sqrt((campBaseTarg->XPos()-self->XPos())*(campBaseTarg->XPos()-self->XPos()) +
         (campBaseTarg->YPos()-self->YPos())*(campBaseTarg->YPos()-self->YPos()));
   MonoPrint ("%s-%d set camp target %s : range = %.2f\n", ((DrawableBSP*)self->drawPointer)->Label(), isWing + 1,
      ((UnitClass*)targetPtr->BaseData())->GetUnitClassName(), rng * FT_TO_NM);
}
#endif

	// Return the selected campaign target
	return target;
}

// Insert 1 target into a sorted target list. Maintain sort order
SimObjectType* DigitalBrain::InsertIntoTargetList (SimObjectType* root, SimObjectType* newObj)
{
SimObjectType	*tmpPtr;
SimObjectType	*last = NULL;

	// This new object had better NOT be in someone elses list
	ShiAssert(newObj->next == NULL);
	ShiAssert(newObj->prev == NULL);

   // Stuff the new sim thing into the list
   tmpPtr = root;

   // No list, so make it the list
   if (tmpPtr == NULL)
   {
      root = newObj;
      newObj->Reference( SIM_OBJ_REF_ARGS );
   }
   else
   {
      while (tmpPtr && SimCompare (tmpPtr->BaseData(), newObj->BaseData()) < 0)
      {
         last = tmpPtr;
         tmpPtr = tmpPtr->next;
      }

      if (!last && (tmpPtr->BaseData() != newObj->BaseData()))
      {
         F4Assert (tmpPtr != newObj);
         // Goes at the front
         newObj->next = root;
         root->prev = newObj;
         root = newObj;
         newObj->Reference( SIM_OBJ_REF_ARGS );
      }
      // Goes at the end
      else if (tmpPtr == NULL)
      {
         last->next = newObj;
         newObj->prev = last;
         newObj->Reference( SIM_OBJ_REF_ARGS );
      }
      // Somewhere in the middle, but not already in there
      else if (tmpPtr->BaseData() != newObj->BaseData())
      {
         last->next = newObj;
         newObj->prev = last;

         tmpPtr->prev = newObj;
         newObj->next = tmpPtr;
         newObj->Reference( SIM_OBJ_REF_ARGS );
      }
      // Must already be in the list
      else if (tmpPtr != newObj)
      {
         F4Assert (tmpPtr->BaseData() == newObj->BaseData());
		 if ( !tmpPtr->BaseData()->OnGround() )
         	SetTarget (tmpPtr);

		 // we don't need this any more -- and it shouldn't have any refs
		 newObj->Reference( SIM_OBJ_REF_ARGS );
		 newObj->Release( SIM_OBJ_REF_ARGS );

      }
	  else
	  {
		 // how did we get here?
		 // we don't need this any more
		 newObj->Reference( SIM_OBJ_REF_ARGS );
		 newObj->Release( SIM_OBJ_REF_ARGS );
	  }
   }

   return root;
}

SimObjectType* MakeSimListFromVuList (AircraftClass *self, SimObjectType* targetList, VuFilteredList* vuList)
{
	SimObjectType* rootObject;
	SimObjectType* curObject;
	SimObjectType* tmpObject;

	if (vuList)
	{
		// If we have a list then make the targetList has the same objects

		VuListIterator updateWalker (vuList);
		SimObjectType* lastObject;
		VuEntity* curEntity;
		
		curObject = targetList;
		rootObject = targetList;
		lastObject = NULL;
		curEntity = updateWalker.GetFirst();
		
		while (curEntity)
		{
			if (curObject)
			{
				if (curObject->BaseData() == curEntity)
				{
					// Step both lists
					lastObject = curObject;
					curObject = curObject->next;
					curEntity = updateWalker.GetNext();
				}
				else
				{
					// Delete current object
					if (curObject->prev)
					{
						curObject->prev->next = curObject->next;
					}
					else
					{
						rootObject = curObject->next;
					}
					
					if (curObject->next)
					{
						curObject->next->prev = curObject->prev;
					}
					
					tmpObject = curObject;
					curObject = curObject->next;
               		tmpObject->next = NULL;
               		tmpObject->prev = NULL;
					tmpObject->Release( SIM_OBJ_REF_ARGS );
					tmpObject = NULL;
				}
			}
			else
			{
				// Append new entry to sim list and increment vu list
				#ifdef DEBUG
				tmpObject = new SimObjectType(OBJ_TAG, self, (FalconEntity*)curEntity);
				#else
				tmpObject = new SimObjectType((FalconEntity*)curEntity);
				#endif
				tmpObject->Reference( SIM_OBJ_REF_ARGS );
				tmpObject->localData->range = 0.0F;
				tmpObject->localData->ataFrom = 180.0F * DTR;
				tmpObject->localData->aspect = 0.0F;
				tmpObject->next = NULL;
				tmpObject->prev = lastObject;
				
				if (lastObject)
					lastObject->next = tmpObject;
				
				lastObject = tmpObject;
				
				// Set head if needed
				if (!rootObject)
					rootObject = tmpObject;
				
				// Step vu list
				curEntity = updateWalker.GetNext();
			}
		}
	}
	else
	{
		// PETER HACK
		// PETER: curObject has NOT been set by anything (garbage off the stack)
		// I am assuming this is what you meant :)
		// HOWEVER, this causes crashes later on so I am just returning
//		return(targetList);
		curObject = targetList;
		rootObject = targetList;
		// PETER END HACK

		// We have no list, so we should have no objects
		while (curObject)
		{
			// Delete current object
			if (curObject->prev)
			{
				curObject->prev->next = curObject->next;
			}
			else
			{
				rootObject = curObject->next;
			}
			
			if (curObject->next)
			{
				curObject->next->prev = curObject->prev;
			}
			
			tmpObject = curObject;
			curObject = curObject->next;
         tmpObject->next = NULL;
         tmpObject->prev = NULL;
			tmpObject->Release( SIM_OBJ_REF_ARGS );
			tmpObject = NULL;
		}

		rootObject = NULL;
	}

	return rootObject;
}
