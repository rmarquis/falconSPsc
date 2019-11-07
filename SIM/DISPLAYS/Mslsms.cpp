#include "stdhdr.h"
#include "sms.h"
#include "object.h"
#include "missile.h"
#include "misldisp.h"
#include "misslist.h"
#include "fcc.h"
#include "hardpnt.h"
#include "simveh.h"
#include "otwdrive.h"
#include "classtbl.h"
#include "Simdrive.h"
#include "BeamRider.h"
#include "fcc.h"
#include "falcsess.h"
#include "Graphics\Include\drawbsp.h"
#include "Vehicle.h"
#include "aircrft.h"
#include "fack.h"
#include "falcmesg.h"
#include "MsgInc\TrackMsg.h"
#include "Fsound.h" // MN
#include "SoundFX.h" // MN

extern short NumRocketTypes;	// M.N.
extern int g_nMissileFix; // MN

void CreateDrawable (SimBaseClass* theObject, float objectScale);

int SMSClass::LaunchMissile (void)
{
	MissileClass* theMissile;
	int retval = FALSE;
	SimBaseClass* lastWeapon;
	FireControlComputer *FCC = ownship->GetFCC();
	SimObjectType* tmpTargetPtr;
	int isCaged, wpnStation, wpnNum, isSpot, isSlave, isTD;
	float x, y, z, az, el;
	VehicleClassDataType* vc;
	int visFlag;
	
	if (!FCC)
	{
		ClearFlag (Firing);
		return retval;
	}
	
	// Check for SMS Failure or other reason not to launch
	if (!CurStationOK() ||
       !curWeapon ||
       Ownship()->OnGround() ||
       MasterArm() != Arm)
	{
		return retval;
	}
	
	
	tmpTargetPtr = FCC->TargetPtr();
	if (curWeapon)
	{
		F4Assert(curWeapon->IsMissile());
		SetFlag (Firing);
		theMissile = (MissileClass *)curWeapon;

		//MI only fire Mav's if they are powered
		//M.N. these conditionals were also true for AA-2's for example, added Type and SType check
		//always use all three checks, as the same values of stype and sptype are not unique!
		//M.N. don't need to uncage or power when in combat AP mode
		if(g_bRealisticAvionics) 
		{
			if((curWeapon->parent &&
			((AircraftClass *)curWeapon->parent)->IsPlayer() &&
			!(((AircraftClass *)curWeapon->parent)->AutopilotType() == AircraftClass::CombatAP) &&
			!Powered) && curWeapon->GetType() == TYPE_MISSILE &&
			curWeapon->GetSType() == STYPE_MISSILE_AIR_GROUND &&
			(curWeapon->GetSPType() == SPTYPE_AGM65A ||
			curWeapon->GetSPType() == SPTYPE_AGM65B ||
			curWeapon->GetSPType() == SPTYPE_AGM65D ||
			curWeapon->GetSPType() == SPTYPE_AGM65G))
			return FALSE;
		}
				
		
		// JB 010109 CTD sanity check
		//if (theMissile->launchState == MissileClass::PreLaunch)
		if (theMissile->launchState == MissileClass::PreLaunch && curHardpoint != -1)
		// JB 010109 CTD sanity check
		{
			// Set the missile position on the AC
			wpnNum = min (hardPoint[curHardpoint]->NumPoints() - 1, curWpnNum);
			hardPoint[curHardpoint]->GetSubPosition(wpnNum, &x, &y, &z);
			hardPoint[curHardpoint]->GetSubRotation(wpnNum, &az, &el);
			theMissile->SetLaunchPosition (x, y, z);
			theMissile->SetLaunchRotation (az, el);
			
// 2001-03-02 MOVED HERE BY S.G.
			if ( theMissile->sensorArray && theMissile->sensorArray[0]->Type() == SensorClass::RadarHoming )
			{
				// Have the missile use the launcher's radar for guidance
				((BeamRiderClass*)theMissile->sensorArray[0])->SetGuidancePlatform( ownship );
			}
// END OF MOVED SECTION

			// Don't hand off ground targets to radar guided air to air missiles
			if (curWeaponType != wtAim120 || (tmpTargetPtr && !tmpTargetPtr->BaseData()->OnGround()))
			{
				theMissile->Start (tmpTargetPtr);
			}
			else
			{
				theMissile->Start (NULL);
			}
			
/* 2001-03-02 MOVED BY S.G. ABOVE THE ABOVE IF. I NEED radarPlatform TO BE SET BEFORE I CAN CALL theMissile->Start SINCE I'LL CALL THE BeamRiderClass SendTrackMsg MESSAGE WHICH REQUIRES IT
			// Need to give beam riders a pointer to the illuminating radar platform
			if ( theMissile->sensorArray && theMissile->sensorArray[0]->Type() == SensorClass::RadarHoming )
			{
				// Have the missile use the launcher's radar for guidance
				((BeamRiderClass*)theMissile->sensorArray[0])->SetGuidancePlatform( ownship );
			}
*/			

// 2001-04-14 MN moved here from AircraftClass::DoWeapons - play bomb drop sound when flag is set and we are an AG missile
			if (theMissile && theMissile->parent && FCC && (FCC->GetMasterMode() == FireControlComputer::AirGroundMissile ||
						FCC->GetMasterMode() == FireControlComputer::AirGroundHARM))
			{
				  if (g_nMissileFix & 0x80)
				  {
						Falcon4EntityClassType* classPtr;
						classPtr = (Falcon4EntityClassType*)theMissile->EntityType();
						WeaponClassDataType *wc = NULL;
	
						if (classPtr)
							wc = (WeaponClassDataType*)classPtr->dataPtr; // this is important

						if (wc && (wc->Flags & WEAP_BOMBDROPSOUND))	// for JSOW, JDAM...
							F4SoundFXSetPos( SFX_BOMBDROP, TRUE, theMissile->parent->XPos(), theMissile->parent->YPos(), theMissile->parent->ZPos(), 1.0f );
						else
							F4SoundFXSetPos( SFX_MISSILE2, TRUE, theMissile->parent->XPos(), theMissile->parent->YPos(), theMissile->parent->ZPos(), 1.0f );
				  }
				  else 
					  F4SoundFXSetPos( SFX_MISSILE2, TRUE, theMissile->parent->XPos(), theMissile->parent->YPos(), theMissile->parent->ZPos(), 1.0f );
            }
			else if (theMissile && !theMissile->parent)
				F4SoundFXSetPos( SFX_MISSILE2, TRUE, theMissile->XPos(), theMissile->YPos(), theMissile->ZPos(), 1.0f );



			// Leonr moved here to avoid per frame database search
			// edg: moved insertion into vudatabse here.  I need
			// the missile to be retrievable from the db when
			// processing the weapon fire message
			vuDatabase->QuickInsert(theMissile);
			theMissile->Wake();
			
			FCC->lastMissileImpactTime = FCC->nextMissileImpactTime;
			
			lastWeapon = curWeapon;
			isCaged = theMissile->isCaged;
			isSpot = theMissile->isSpot;	// Marco Edit - SPOT/SCAN Support
			isSlave = theMissile->isSlave;	// Marco Edit - BORE/SLAVE Support
			isTD = theMissile->isTD;		// Marco Edit - TD/BP Support

			wpnStation = curHardpoint;
			wpnNum     = curWpnNum;
			WeaponStep(TRUE);
			if (lastWeapon == curWeapon)
			{
				SetCurrentWeapon(-1, NULL);
			}
			else
			{
				//MI if we launch one uncaged, the next one's caged again!
				if (curWeapon)
				{
					((MissileClass*)curWeapon)->isCaged = TRUE;
					((MissileClass*)curWeapon)->isSpot = isSpot;
					((MissileClass*)curWeapon)->isSlave = isSlave;
					((MissileClass*)curWeapon)->isTD = isTD;

				}
			}
			retval = TRUE;
			
			vc = GetVehicleClassData(ownship->Type() - VU_LAST_ENTITY_TYPE);
			visFlag = vc->VisibleFlags;
			
			if (visFlag & (1 << curHardpoint) && theMissile->drawPointer)
			{
				// Detach visual from parent
				if (hardPoint[wpnStation]->GetRack())
				{
					OTWDriver.DetachObject(hardPoint[wpnStation]->GetRack(),	(DrawableBSP*)(theMissile->drawPointer), theMissile->GetRackSlot());
				}
				else if (ownship->drawPointer && wpnStation)
				{
					OTWDriver.DetachObject((DrawableBSP*)(ownship->drawPointer), (DrawableBSP*)(theMissile->drawPointer), wpnStation-1);
				}
			}
			
			if (UnlimitedAmmo())
				ReplaceMissile(wpnStation, theMissile);
			else
			{
				DecrementStores(hardPoint[wpnStation]->GetWeaponClass(), 1);
				RemoveStore(wpnStation, hardPoint[wpnStation]->weaponId);
				hardPoint[wpnStation]->weaponCount --;
				if (hardPoint[wpnStation]->weaponCount >= hardPoint[wpnStation]->NumPoints())
				{
					ReplaceMissile(wpnStation, theMissile);
				}
				else
				{
					DetachWeapon(wpnStation, theMissile);

					if (ownship->IsLocal ())
					{
						FalconTrackMessage* trackMsg = new FalconTrackMessage (1,ownship->Id (), FalconLocalGame);
						trackMsg->dataBlock.trackType = Track_RemoveWeapon;
						trackMsg->dataBlock.hardpoint = wpnStation;
						trackMsg->dataBlock.id = ownship->Id ();
						
						// Send our track list
						FalconSendMessage (trackMsg, TRUE);
					}
				}
			}			
		}
		
		ClearFlag (Firing);
	}

	//MI SOI after firing Mav
	if(g_bRealisticAvionics) 
	{
		if(curWeapon && curWeapon->parent &&
		((AircraftClass *)curWeapon->parent)->IsPlayer() &&
		(curWeapon->GetSPType() == SPTYPE_AGM65A ||
		curWeapon->GetSPType() == SPTYPE_AGM65B ||
		curWeapon->GetSPType() == SPTYPE_AGM65D ||
		curWeapon->GetSPType() == SPTYPE_AGM65G))
		{
			if(SimDriver.playerEntity)
			{
				SimDriver.playerEntity->SOIManager(SimVehicleClass::SOI_WEAPON);
			}
		}
	}
	
	return (retval);
}

void SMSBaseClass::ReplaceMissile(int station, MissileClass *theMissile)
{
	SimWeaponClass	*weapPtr,*newMissile,*lastPtr = NULL;
	int				visFlag;
	int				slotId;

	if ( IsSet( GunOnBoard ) )
	    slotId = max(0,station - 1);
	else
		slotId = station;
	
	newMissile = InitAMissile (ownship, hardPoint[station]->weaponId, theMissile->GetRackSlot());
	((MissileClass *)(newMissile))->isCaged = TRUE;
	((MissileClass *)(newMissile))->isSpot = FALSE;
	((MissileClass *)(newMissile))->isSlave = TRUE;
	((MissileClass *)(newMissile))->isTD = FALSE;
	visFlag = GetVehicleClassData(ownship->Type() - VU_LAST_ENTITY_TYPE)->VisibleFlags;
	if (visFlag & (1 << station))
		{
		ShiAssert( slotId >= 0 );

		CreateDrawable(newMissile, OTWDriver.Scale());
	
		// Fix up drawable parent/child relations
		if (hardPoint[station]->GetRack())
			OTWDriver.AttachObject(hardPoint[station]->GetRack(), (DrawableBSP*)(newMissile->drawPointer), theMissile->GetRackSlot());
		else if (ownship->drawPointer)
			OTWDriver.AttachObject((DrawableBSP*)(ownship->drawPointer), (DrawableBSP*)(newMissile->drawPointer), slotId );
		}
	
	weapPtr = hardPoint[station]->weaponPointer;
	while (weapPtr)
	{
		if (weapPtr == theMissile)
		{
			if (lastPtr)
				lastPtr->nextOnRail = newMissile;
			else
				hardPoint[station]->weaponPointer = newMissile;
			newMissile->nextOnRail = theMissile->nextOnRail;
			theMissile->nextOnRail = NULL;
			return;
		}
		lastPtr = weapPtr;
		weapPtr = weapPtr->nextOnRail;
	}

	//edg: we neglected to account for the case where the replaced missile
	//is already detatched (as it is in SMS BAse Launch Weapon) and thereby
	// leak missile mem.  If we got here, we need to attach the missile at
	// the end of the chain
	newMissile->nextOnRail = NULL;
	if (lastPtr)
		lastPtr->nextOnRail = newMissile;
	else
		hardPoint[station]->weaponPointer = newMissile;
}

int SMSClass::LaunchRocket (void)
{
	MissileClass* theMissile;
	int retval = FALSE;
	SimBaseClass* lastWeapon;
	FireControlComputer *FCC = ownship->GetFCC();
	SimObjectType* tmpTargetPtr;
	int wpnNum;
	static count = 0;
	float az, el, x, y, z;
	
	if (!FCC)
   {
      ClearFlag (Firing);
		return retval;
   }

   // Check for SMS Failure
   if (!CurStationOK() ||
       !curWeapon || 
        Ownship()->OnGround() ||
        MasterArm() != Arm)
   {
      return retval;
   }
	
	tmpTargetPtr = FCC->TargetPtr();
	ShiAssert(hardPoint[curHardpoint]->weaponPointer->IsMissile());
	theMissile = (MissileClass*)(hardPoint[curHardpoint]->weaponPointer);
	
	if (theMissile && count == 0)
	{
      if (hardPoint[curHardpoint]->GetRack())
         hardPoint[curHardpoint]->GetRack()->SetSwitchMask(0, 0);
      SetFlag(Firing);
		count = 2;
		wpnNum = min (hardPoint[curHardpoint]->NumPoints() - 1, curWpnNum);
		hardPoint[curHardpoint]->GetSubPosition(wpnNum, &x, &y, &z);
		hardPoint[curHardpoint]->GetSubRotation(wpnNum, &az, &el);
		theMissile->SetLaunchPosition (x, y, z);
		theMissile->SetLaunchRotation (az, el);
		if (tmpTargetPtr)
		{
			theMissile->Start (tmpTargetPtr);
		}
		else
		{
			theMissile->Start (NULL);
		}
		
		// Leonr moved here to avoid per frame database search
		// edg: moved insertion into vudatabse here.  I need
		// the missile to be retrievable from the db when
		// processing the weapon fire message
		vuDatabase->QuickInsert(theMissile);
		// KCK: WARNING: these things already have drawables BEFORE wake is called.
		// Make Wake() not assign new drawables if one exists?
		theMissile->Wake();
		
//		MonoPrint ("Inserting rocket %p\n", theMissile);
		FCC->lastMissileImpactTime = FCC->nextMissileImpactTime;
		
		// Remove it from the parent
		hardPoint[curHardpoint]->weaponCount --;
		DetachWeapon(curHardpoint, theMissile);
		
      curWeapon = hardPoint[curHardpoint]->weaponPointer;
		// Only the last rocket has a nametag
		if (hardPoint[curHardpoint]->weaponPointer)
			theMissile->drawPointer->SetLabel("", 0xff00ff00);
		// End TYPE_ROCKET section
	}
	count --;
	
	if (hardPoint[curHardpoint]->weaponCount == 0)
	{
		retval = TRUE;
      wpnNum = curHardpoint;
      
		// Reload if possible / wanted
		if (theMissile && UnlimitedAmmo())
			ReplaceRocket(wpnNum, theMissile);

		lastWeapon = curWeapon;
		WeaponStep(TRUE);
		if (lastWeapon == curWeapon)
		{
			ReleaseCurWeapon(-1);
		}
      ClearFlag (Firing);
	}
	return (retval);
}

void SMSBaseClass::ReplaceRocket(int station, MissileClass *theMissile)
{
/*
	hardPoint[station]->weaponCount = 19;
	hardPoint[station]->weaponPointer = InitWeaponList (ownship, hardPoint[station]->weaponId,
      hardPoint[station]->GetWeaponClass(), hardPoint[station]->weaponCount, InitAMissile);
*/


	// Change hacks to datafile: terrdata\objects\falcon4.rkt
	int entryfound = 0;
	for (int j=0; j<NumRocketTypes; j++)
	{
		if (hardPoint[station]->weaponId == RocketDataTable[j].weaponId)
		{
			hardPoint[station]->weaponCount = RocketDataTable[j].weaponCount;
			hardPoint[station]->weaponPointer = InitWeaponList (ownship, hardPoint[station]->weaponId,
			  hardPoint[station]->GetWeaponClass(), hardPoint[station]->weaponCount, InitAMissile);
			entryfound = 1;
			break;
		}
	}
	if (!entryfound)	// use generic 2.75mm rocket
	{
		hardPoint[station]->weaponCount = 19;
		hardPoint[station]->weaponPointer = InitWeaponList (ownship, hardPoint[station]->weaponId,
		  hardPoint[station]->GetWeaponClass(), hardPoint[station]->weaponCount, InitAMissile);
	}
}
