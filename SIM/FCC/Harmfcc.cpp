#include "stdhdr.h"
#include "fcc.h"
#include "sms.h"
#include "missile.h"
#include "harmpod.h"
#include "object.h"
#include "geometry.h"
#include "simveh.h"
#include "Graphics\Include\drawbsp.h"
#include "otwdrive.h"
#include "radar.h"

//MI
extern bool g_bRealisticAvionics;

void FireControlComputer::HarmMode(void)
{
	MissileClass		*theMissile;
	HarmTargetingPod	*theHTS;
	Tpoint				pos;
	
	theHTS = (HarmTargetingPod*)FindSensor(platform, SensorClass::HTS);
	theMissile = (MissileClass *)(Sms->curWeapon);

	if (theHTS)
	{
		theHTS->SetRange(FloatToInt32(HSDRange));
		
		if (designateCmd && !lastDesignate)
		{
			theHTS->LockTargetUnderCursor();
		}
		else if (dropTrackCmd)
		{
			dropTrackCmd = FALSE; // JB 010726
			theHTS->ClearSensorTarget();
			ClearCurrentTarget();
         if (theMissile)
			   theMissile->SetTargetPosition (-1.0F, -1.0F, -1.0F);
		}
		//MI why is this here??
		if(!g_bRealisticAvionics)
			platform->SOIManager (SimVehicleClass::SOI_WEAPON);

	   if (theHTS->CurrentTarget())
	   {
		   if (!targetPtr || targetPtr->BaseData() != theHTS->CurrentTarget()->BaseData())
		   {
			   SetTarget (theHTS->CurrentTarget());
		   }
		   
		   if (theMissile)
		   {
			   /* JB 010624 Why? Setting the position like this screws up multiplayer and entitys' movement
				 if (targetPtr->BaseData()->IsSim() && ((SimBaseClass*)targetPtr->BaseData())->IsAwake())
			   {
				   ((SimBaseClass*)targetPtr->BaseData())->drawPointer->GetPosition (&pos);
				   ((SimBaseClass*)targetPtr->BaseData())->SetPosition (pos.x, pos.y, pos.z);
			   }
				 */

			   // TODO:  Just pass the target to the missile and let it deal with things...
			   pos.x = targetPtr->BaseData()->XPos();
			   pos.y = targetPtr->BaseData()->YPos();

			   if (targetPtr->BaseData()->IsSim()) {
				   pos.z = targetPtr->BaseData()->ZPos();
			   } else {
				   pos.z = OTWDriver.GetGroundLevel( pos.x, pos.y );
			   }
			   theMissile->SetTargetPosition( pos.x, pos.y, pos.z );
		   
			   CalcRelGeom(platform, targetPtr, NULL, 1.0F / SimLibMajorFrameTime);
			   missileRMax   = theMissile->GetRMax (-platform->ZPos(), platform->Vt(), targetPtr->localData->az, 0.0f, 0.0f);
			   missileActiveRange = 0.0f;
			   missileActiveTime  = -1.0f;

			   //LRKLUDGE
			   missileRMin   = 0.075F * missileRMax;
			   missileRneMax = 0.8F * missileRMax;
			   missileRneMin = 0.2F * missileRMax;
			   missileTOF    = theMissile->GetTOF(-platform->ZPos(), platform->Vt(), 0.0f, 0.0f, targetPtr->localData->range);
			   missileSeekerAz = targetPtr->localData->az;
			   missileSeekerEl = targetPtr->localData->el;
			   
			   missileWEZDisplayRange = theHTS->GetRange() * NM_TO_FT;

			   missileTarget = TRUE;
		   } else {
			   missileTarget = FALSE;
		   }
      }
      else
      {
         if (theMissile)
         {
            theHTS->GetAGCenter (&pos.x, &pos.y);
				pos.z = OTWDriver.GetGroundLevel( pos.x, pos.y );
			   theMissile->SetTargetPosition( pos.x, pos.y, pos.z );
         }
      }
	}
	else
	{
		if (targetPtr)
		{
			if (theHTS)
				theHTS->ClearSensorTarget();
			ClearCurrentTarget();
		}
		missileTarget = FALSE;
	}
	
	if (!releaseConsent)
	{
		postDrop = FALSE;
	}
}
