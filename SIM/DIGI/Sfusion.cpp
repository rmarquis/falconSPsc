#include "stdhdr.h"
#include "classtbl.h"
#include "digi.h"
#include "sensors.h"
#include "simveh.h"
#include "missile.h"
#include "object.h"
#include "sensclas.h"
#include "Entity.h"
#include "team.h"
#include "Aircrft.h"
/* 2001-03-15 S.G. */#include "campbase.h"
/* 2001-03-21 S.G. */#include "flight.h"
/* 2001-03-21 S.G. */#include "atm.h"

#include "RWR.h" // 2002-02-11 S.G.
#include "Radar.h" // 2002-02-11 S.G.
#include "simdrive.h" // 2002-02-17 S.G.

#define MAX_NCTR_RANGE    (60.0F * NM_TO_FT) // 2002-02-12 S.G. See RadarDoppler.h

/* 2001-09-07 S.G. RP5 */ extern bool g_bRP5Comp;
extern int g_nLowestSkillForGCI; // 2002-03-12 S.G. Replaces the hardcoded '3' for skill test
extern bool g_bUseNewCanEnage; // 2002-03-11 S.G.
int GuestimateCombatClass(AircraftClass *self, FalconEntity *baseObj); // 2002-03-11 S.G.

FalconEntity* SpikeCheck (AircraftClass* self, FalconEntity *byHim = NULL, int *data = NULL);// 2002-02-10 S.G.
void DigitalBrain::SensorFusion(void)
{
SimObjectType* obj = targetList;
float turnTime=0.0F,timeToRmax=0.0F,rmax=0.0F,tof=0.0F,totV=0.0F;
SimObjectLocalData* localData=NULL;
int relation=0, pcId= ID_NONE, canSee=FALSE, i=0;
FalconEntity* baseObj=NULL;

// 2002-04-18 REINSTATED BY S.G. After putting back '||' instead of '&&' before "localData->sensorLoopCount[self->sensorArray[i]->Type()] > delayTime" below, this is no longer required
// 2002-02-17 MODIFIED BY S.G. Sensor routines for AI runs less often than SensorFusion therefore the AI will time out his target after this delayTime as elapsed.
//                             By using the highest of both, I'm sure this will not happen...
int delayTime = SimLibElapsedTime - 6*SEC_TO_MSEC*(SkillLevel() + 1);
/* int delayTime;
unsigned int fromSkill = 6 * SEC_TO_MSEC * (SkillLevel() + 1);

	if (fromSkill > self->targetUpdateRate)
		delayTime = SimLibElapsedTime - fromSkill;
	else
		delayTime = SimLibElapsedTime - self->targetUpdateRate;
*/
   /*--------------------*/
   /* do for all objects */
   /*--------------------*/
   while (obj)
   {
		localData = obj->localData;
		baseObj = obj->BaseData();

		//if (F4IsBadCodePtr((FARPROC) baseObj)) // JB 010223 CTD
		if (F4IsBadCodePtr((FARPROC) baseObj) || F4IsBadReadPtr(baseObj, sizeof(FalconEntity))) // JB 010305 CTD
			break; // JB 010223 CTD

		// Check all sensors for contact
		canSee = FALSE;

      if (!g_bRP5Comp) {
		  // Aces get to use GCI
		  // Idiots find out about you inside 1 mile anyway
		  if (localData->range > 3.0F * NM_TO_FT && // gci is crap inside 3nm
				 (SkillLevel() >= 2 && 
			  localData->range < 25.0F * NM_TO_FT||
			  SkillLevel() >=3  && 
			  localData->range < 35.0F * NM_TO_FT||
			  SkillLevel() >=4  && 
			  localData->range < 55.0F * NM_TO_FT)
			  )//me123 not if no sensor has seen it || localData->range < 1.0F * NM_TO_FT)
		  {
			 canSee = TRUE;
		  }

		  // You can always see your designated target
		  if (baseObj->Id() == mDesignatedObject && localData->range > 8.0F * NM_TO_FT)
		  {
			 canSee = TRUE;//me123
		  }

		  for (i = 0; i<self->numSensors && !canSee; i++)
		  {
			 if (localData->sensorState[self->sensorArray[i]->Type()] > SensorClass::NoTrack ||
				 localData->sensorLoopCount[self->sensorArray[i]->Type()] > delayTime)
			 {
				canSee = TRUE;
				break;
			 }
		  }
	  }
	  else {
// 2001-03-21 REDONE BY S.G. SO SIM AIRPLANE WIL FLAG FLIGHT/AIRPLANE AS DETECTED AND WILL PROVIDE GCI
#if 0
		// Aces get to use GCI
		// Idiots find out about you inside 1 mile anyway
		if (SkillLevel() >= 3 && localData->range < 15.0F * NM_TO_FT || localData->range < 1.0F * NM_TO_FT)
		{
			canSee = TRUE;
		}

		// You can always see your designated target
		if (baseObj->Id() == mDesignatedObject && localData->range > 8.0F * NM_TO_FT)
		{
			canSee = TRUE;
		}

		for (i = 0; i<self->numSensors && !canSee; i++)
		{
			if (localData->sensorState[self->sensorArray[i]->Type()] > SensorClass::NoTrack ||
				localData->sensorLoopCount[self->sensorArray[i]->Type()] > delayTime)
			{
				canSee = TRUE;
				break;
			}
		}
#else
		// First I'll get the campaign object if it's for a sim since I use it at many places...
		CampBaseClass *campBaseObj = (CampBaseClass *)baseObj;

		if (baseObj->IsSim())
			campBaseObj = ((SimBaseClass*)baseObj)->GetCampaignObject();

		// If the object is a weapon, don't do GCI on it
		if (baseObj->IsWeapon())
			campBaseObj = NULL;

		// This is our GCI implementation... Ace and Veteran gets to use GCI.
		// Only if we have a valid base object...
		// This code is to make sure our GCI targets are prioritized, just like other targets
		if (campBaseObj && SkillLevel() >= g_nLowestSkillForGCI && localData->range < 30.0F * NM_TO_FT)
			if (campBaseObj->GetSpotted(self->GetTeam()))
				canSee = TRUE;
		// You can always see your designated target
		if (baseObj->Id() == mDesignatedObject && localData->range > 8.0F * NM_TO_FT)
			canSee = TRUE;

		if (SimDriver.RunningDogfight()) // 2002-02-17 ADDED BY S.G. If in dogfight, don't loose sight of your opponent.
			canSee = TRUE;

		// Go through all your sensors. If you 'see' the target and are bright enough, flag it as spotted and ask for an intercept if this FLIGHT is spotted for the first time...
		for (i = 0; i<self->numSensors; i++) {
			if (localData->sensorState[self->sensorArray[i]->Type()] > SensorClass::NoTrack || localData->sensorLoopCount[self->sensorArray[i]->Type()] > delayTime) { // 2002-04-18 MODIFIED BY S.G. Reverted to && instead of ||. *MY* logic was flawed. It gaves a 'delay' (grace period) after the sensor becomes 'NoLock'.
				if (campBaseObj && SkillLevel() >= g_nLowestSkillForGCI && !((UnitClass *)self->GetCampaignObject())->Broken()) {
					if (!campBaseObj->GetSpotted(self->GetTeam()) && campBaseObj->IsFlight())
						RequestIntercept((FlightClass *)campBaseObj, self->GetTeam());

					// 2002-02-11 ADDED BY S.G. If the sensor can identify the target, mark it identified as well
					int identified = FALSE;

					if (self->sensorArray[i]->Type() == SensorClass::RWR) {
						if (((RwrClass *)self->sensorArray[i])->GetTypeData()->flag & RWR_EXACT_TYPE)
							identified = TRUE;
					}
					else if (self->sensorArray[i]->Type() == SensorClass::Radar) {
						if (((RadarClass *)self->sensorArray[i])->GetRadarDatFile() && (((RadarClass *)self->sensorArray[i])->radarData->flag & RAD_NCTR) && localData->ataFrom < 45.0f * DTR && localData->range < ((RadarClass *)self->sensorArray[i])->GetRadarDatFile()->MaxNctrRange / (2.0f * (16.0f - (float)SkillLevel()) / 16.0f)) // 2002-03-05 MODIFIED BY S.G. target's aspect and skill used in the equation
							identified = TRUE;
					}
					else
						identified = TRUE;

					campBaseObj->SetSpotted(self->GetTeam(),TheCampaign.CurrentTime, identified);
				}
				canSee = TRUE;
				break;
			}
		}
#endif
	  }
      /*--------------------------------------------------*/
      /* Sensor id state                                  */
      /* RWR ids coming from RWR_INTERP can be incorrect. */ 
      /* Visual identification is 100% correct.           */
      /*--------------------------------------------------*/
      if (canSee)
      {
			if (baseObj->IsMissile())
         {
            pcId = ID_MISSILE;
         }
			else if (baseObj->IsBomb())
         {
            pcId = ID_NEUTRAL;
         }
         else
         {
						if (TeamInfo[self->GetTeam()]) // JB 010617 CTD
						{
							relation = TeamInfo[self->GetTeam()]->TStance(obj->BaseData()->GetTeam());
							switch (relation)
							{
								 case Hostile:
								 case War:
										pcId = ID_HOSTILE;
								 break;

								 case Allied:
								 case Friendly:
										pcId = ID_FRIENDLY;
								 break;

								 case Neutral:
      					pcId = ID_NEUTRAL;
								 break;
							}
						}
         }
      }

      /*----------------------------------------------------*/
      /* Threat determination                               */
      /* Assume threat has your own longest range missile.  */
      /* Hypothetical time before we're in the mort locker. */
      /* If its a missile calculate time to impact.         */
      /*---------------------------------------------------*/
      localData->threatTime = 2.0F * MAX_THREAT_TIME;
      if (canSee)
      {
         /*---------*/
         /* missile */
         /*---------*/
			if (baseObj->IsMissile())
         {
            /*--------------------*/
            /* known missile type */
            /*--------------------*/
            if (pcId == ID_MISSILE)
            {
               /*---------------------------------*/
               /* ignore your own side's missiles */
               /*---------------------------------*/
               if (obj->BaseData()->GetTeam() == self->GetTeam())
               {
                  localData->threatTime = 2.0F * MAX_THREAT_TIME;
               }
               else
               {
                  if (localData->sensorState[SensorClass::RWR] >= SensorClass::SensorTrack) 
                     localData->threatTime = localData->range / AVE_AIM120_VEL;
                  else
						   localData->threatTime = localData->range / AVE_AIM9L_VEL;
               }
            }
            else localData->threatTime = MAX_THREAT_TIME;
         }
         /*----------*/
         /* aircraft */
         /*----------*/
		 // 2002-03-05 MODIFIED BY S.G. Allow flights to be valid targets as well to let aircraft fight aggregated entity but ignore choppers
		 // 2002-03-11 MODIFIED BY S.G. Don't call CombatClass directly but through GuestimateCombatClass which doesn't assume you have an ID on the target
//       else if ((baseObj->IsAirplane() && (pcId != ID_NONE) && (pcId < ID_NEUTRAL) && // 2002-02-25 MODIFIED BY S.G. 
//          ((AircraftClass*)baseObj)->CombatClass() < MnvrClassA10)
         else if ((baseObj->IsAirplane() || (baseObj->IsFlight() && !baseObj->IsHelicopter())) && pcId != ID_NONE && pcId < ID_NEUTRAL && GuestimateCombatClass(self, baseObj) < MnvrClassA10) // 2002-02-25 MODIFIED BY S.G.  CombatClass is defined for FlightClass and AircraftClass now and is virtual in FalconEntity which will return 999 so no need to check the type of object anymore
//       else if (((baseObj->IsAirplane() && ((AircraftClass *)baseObj)->CombatClass() < MnvrClassA10) || (baseObj->IsFlight() && ((FlightClass *)baseObj)->CombatClass() < MnvrClassA10)) && (pcId != ID_NONE) && (pcId < ID_NEUTRAL))
         {
            /*-------------------------*/
            /* time to turn to ownship */
            /*-------------------------*/
            turnTime = localData->ataFrom / FIVE_G_TURN_RATE;

            /*------------------*/
            /* closing velocity */
            /*------------------*/
            totV = obj->BaseData()->Vt() + self->Vt()*(float)cos(localData->ata*DTR);

            /*------------*/
            /* 10 NM rmax *///me123 let's asume radar missiles 25nm range
            /*------------*/
			if (SpikeCheck(self) == obj->BaseData())//me123 addet
            rmax = 2.5f*60762.11F;   /* 25 NM */
			else
			rmax = 60762.11F;   /* 10 NM */

            /*-------------------------------------------*/
            /* calculate time to rmax and time of flight */
            /*-------------------------------------------*/
            if (localData->range > rmax) 
            {
			      if ( totV <= 0.0f )
			      {
               	timeToRmax = MAX_THREAT_TIME * 2.0f;
			      }
			      else
			      {
               	timeToRmax = (localData->range - rmax) / totV;
               	tof = rmax / AVE_AIM120_VEL;
			      }
            }
            else 
            {
               timeToRmax = 0.0F;
               tof = localData->range / AVE_AIM120_VEL;
            }

            /*-------------*/
            /* threat time */
            /*-------------*/
            localData->threatTime = turnTime + timeToRmax + tof;
         }
         else
         {
            localData->threatTime = 2.0F * MAX_THREAT_TIME;
         }
      }

      /*----------------------------------------------------*/
      /* Targetability determination                        */
      /* Use the longest range missile currently on board   */
      /* Hypothetical time before the tgt ac can be morted  */
      /*                                                    */
      /* Aircraft on own team are returned SENSOR_UNK       */
      /*----------------------------------------------------*/
// 2002-03-05 MODIFIED BY S.G.  CombatClass is defined for FlightClass and AircraftClass now and is virtual in FalconEntity which will return 999
// This code restrict the calculation of the missile range to either planes, chopper or flights. An aggregated chopper flight will have 'IsFlight' set so check if the 'AirUnitClass::IsHelicopter' function returned TRUE to screen them out from aircraft type test
// Have to be at war against us
// Chopper must be our assigned or mission target or we must be on sweep (not a AMIS_SWEEP but still has OnSweep set)
// Must be worth shooting at, unless it's our assigned or mission target (new addition so AI can go after an AWACS for example if it's their target...
//    if (canSee && baseObj->IsAirplane() && pcId < ID_NEUTRAL &&
//       (IsSetATC(OnSweep) || ((AircraftClass*)baseObj)->CombatClass() < MnvrClassA10))
// 2002-03-11 MODIFIED BY S.G. Don't call CombatClass directly but through GuestimateCombatClass which doesn't assume you have an ID on the target
	  // Since I'm going to check for this twice in the next if statement, do it once here but also do the 'canSee' test which is not CPU intensive and will prevent the test from being performed if can't see.
	  CampBaseClass *campObj;
	  if (baseObj->IsSim())
		  campObj = ((SimBaseClass *)baseObj)->GetCampaignObject();
	  else
		  campObj = (CampBaseClass *)baseObj;
	  int isMissionTarget = canSee && campObj && (((FlightClass *)(self->GetCampaignObject()))->GetUnitMissionTargetID() == campObj->Id() || ((FlightClass *)(self->GetCampaignObject()))->GetAssignedTarget() == campObj->Id());
		  
      if (canSee &&
		  (baseObj->IsAirplane() || (baseObj->IsFlight() && !baseObj->IsHelicopter()) || (baseObj->IsHelicopter() && ((missionType != AMIS_SWEEP && IsSetATC(OnSweep)) || isMissionTarget))) &&
		  pcId < ID_NEUTRAL &&
		  (GuestimateCombatClass(self, baseObj) < MnvrClassA10 || IsSetATC(OnSweep) || isMissionTarget)) // 2002-03-11 Don't assume you know the combat class
// END OF MODIFIED SECTION 2002-03-05
      {
         /*------------------*/
         /* closing velocity */
         /*------------------*/
         totV     = obj->BaseData()->Vt()*(float)cos(localData->ataFrom*DTR) + self->Vt();

         /*------------------------*/
         /* time to turn on target */
         /*------------------------*/
         turnTime = localData->ata / FIVE_G_TURN_RATE;

         /*-------------------*/
         /* digi has missiles */ 
         /*-------------------*/
         rmax = maxAAWpnRange;//me123 60762.11F;

         if (localData->range > rmax)
         {
			   if ( totV <= 0.0f )
			   {
               timeToRmax = MAX_TARGET_TIME * 2.0f;
			   }
			   else
			   {
               timeToRmax = (localData->range - rmax) / totV;
               tof = rmax / AVE_AIM120_VEL;
			   }
         }
         else 
         {
            timeToRmax = 0.0F;
            tof = localData->range / AVE_AIM120_VEL;
         }

         localData->targetTime = turnTime + timeToRmax + tof;
      }
      else
      {
         localData->targetTime = 2.0F * MAX_TARGET_TIME;
      }

      obj = obj->next;
   }
}

int GuestimateCombatClass(AircraftClass *self, FalconEntity *baseObj)
{
	// Fail safe
	if (!baseObj)
		return 8;

	// If asked to use the old code, then honor the request
	if (!g_bUseNewCanEnage)
		return baseObj->CombatClass();

	// First I'll get the campaign object if it's for a sim since I use it at many places...
	CampBaseClass *campBaseObj;
	if (baseObj->IsSim())
		campBaseObj = ((SimBaseClass*)baseObj)->GetCampaignObject();
	else
		campBaseObj = ((CampBaseClass *)baseObj);

	// If the object is a weapon, no point
	if (baseObj->IsWeapon())
		return 8;

	// If it doesn't have a campaign object or it's identified...
	if (!campBaseObj || campBaseObj->GetIdentified(self->GetTeam())) {
		// Yes, now you can get its combat class!
		return baseObj->CombatClass();
	}
	else {
		// No :-( Then guestimate it... (from RIK's BVR code)
		if ((baseObj->Vt() * FTPSEC_TO_KNOTS > 300.0f || baseObj->ZPos() < -10000.0f))  {
			//this might be a combat jet.. asume the worst
			return  4;
		}
		else if (baseObj->Vt() * FTPSEC_TO_KNOTS > 250.0f) {
			// this could be a a-a capable thingy, but if it's is it's low level so it's a-a long range shoot capabilitys are not great
			return  1;
		}
		else {
			// this must be something unthreatening...it's below 250 knots but it's still unidentified so...
			return  0;
		}
	}
}
