#include "stdhdr.h"
#include "sms.h"
#include "bomb.h"
#include "bombfunc.h"
#include "hardpnt.h"
#include "simveh.h"
#include "otwdrive.h"
#include "Graphics\Include\drawbsp.h"
#include "falcsess.h"
#include "entity.h"
#include "vehicle.h"
#include "aircrft.h"
#include "fack.h"
#include "MsgInc\WeaponFireMsg.h"
#include "MsgInc\TrackMsg.h"
#include "fcc.h"
#include "simdrive.h"
#include "ivibedata.h"
#include "digi.h" // 2002-02-26 S.G.

void CreateDrawable (SimBaseClass* theObject, float objectScale);

int SMSClass::DropBomb (int allowRipple)
{
BombClass *theBomb;
SimBaseClass* lastWeapon;
int retval = FALSE;
vector pos, posDelta;
float initXloc, initYloc, initZloc;
int matchingStation, i;
VehicleClassDataType* vc;
int visFlag;
float dragCoeff = 0.0f;
int slotId;


   // Check for SMS Failure or other reason not to drop
   if (!CurStationOK() || //Weapon Station failure
        ownship->OnGround() || // Weight on wheels inhibit
       !curWeapon || // No Weapon
       ownship->GetNz() < 0.0F || // Negative Gs
       MasterArm() != Arm) // Not in arm mode
   {
      ownship->GetFCC()->bombPickle = FALSE;
      return retval;
   }
   
	// edg: there was a problem.  Some ground vehicles do NOT have guns
	// at all.  All this code assumes a gun in Slot 0 and therefore subtracts
	// 1 from curhardpoint to get the right slot.  Don't do this if there
	// are no guns on board.
	if ( IsSet( GunOnBoard ) )
		slotId = max(0,curHardpoint - 1);
	else
		slotId = curHardpoint;

	// Verify curWeapon on curHardpoint
	F4Assert(curHardpoint >= 0);
	if (curHardpoint < 0 || hardPoint[curHardpoint]->weaponPointer != curWeapon)
	{
		for (i=0; i<numHardpoints; i++)
		{
			if(hardPoint[i]->weaponPointer == curWeapon)
			{
				MonoPrint ("Adjusting hardpoint to %d\n", i);
				curHardpoint = i;
				if ( IsSet( GunOnBoard ) )
					slotId = max(0,curHardpoint - 1);
				else
					slotId = curHardpoint;
				break;
			}
		}
	}

   if (curWeapon && fabs(ownship->Roll()) < 60.0F * DTR)
   {
      F4Assert(curWeapon->IsBomb());

      dragCoeff  = hardPoint[curHardpoint]->GetWeaponData()->cd;

      theBomb = (BombClass *)(curWeapon);
      // edg: it's been observed that theBomb will be NULL at times?!
      // leonr: This happens when you ask for too many release pulses.
      if ( theBomb == NULL )
         return retval;
      SetFlag (Firing);
      hardPoint[curHardpoint]->GetSubPosition(curWpnNum, &initXloc, &initYloc, &initZloc);
      pos.x = ownship->XPos() + ownship->dmx[0][0]*initXloc + ownship->dmx[1][0]*initYloc + ownship->dmx[2][0]*initZloc;
      pos.y = ownship->YPos() + ownship->dmx[0][1]*initXloc + ownship->dmx[1][1]*initYloc + ownship->dmx[2][1]*initZloc;
      pos.z = ownship->ZPos() + ownship->dmx[0][2]*initXloc + ownship->dmx[1][2]*initYloc + ownship->dmx[2][2]*initZloc;
      posDelta.x = ownship->XDelta();
      posDelta.y = ownship->YDelta();
      posDelta.z = ownship->ZDelta();
      theBomb->SetBurstHeight (burstHeight);
      theBomb->Start(&pos, &posDelta, dragCoeff, (ownship ? ((DigitalBrain *)ownship->Brain() ? ((DigitalBrain *)ownship->Brain())->GetGroundTarget() : NULL): NULL)); // 2002-02-26 MODIFIED BY S.G. Added the ownship... in case the target is aggregated and an AI is bombing it

	   vc = GetVehicleClassData(ownship->Type() - VU_LAST_ENTITY_TYPE);
	   visFlag = vc->VisibleFlags;

      if (visFlag & (1 << curHardpoint) && theBomb->drawPointer)
      {
         // Detach visual from parent
         if (hardPoint[curHardpoint]->GetRack())
         {
            OTWDriver.DetachObject(hardPoint[curHardpoint]->GetRack(), (DrawableBSP*)(theBomb->drawPointer), theBomb->GetRackSlot());
         }
         else if (ownship->drawPointer && curHardpoint && theBomb->drawPointer)
         {
            OTWDriver.DetachObject((DrawableBSP*)(ownship->drawPointer), (DrawableBSP*)(theBomb->drawPointer), slotId);
         }
      }

      if (UnlimitedAmmo())
      {
         ReplaceBomb(curHardpoint, theBomb);
      }
      else
      {
         DecrementStores(hardPoint[curHardpoint]->GetWeaponClass(), 1);
         hardPoint[curHardpoint]->weaponCount --;
         if (hardPoint[curHardpoint]->weaponCount >= hardPoint[curHardpoint]->NumPoints())
            ReplaceBomb(curHardpoint, theBomb);
         else
		 {
			 DetachWeapon(curHardpoint, theBomb);
			 
			 if (ownship->IsLocal ())
			 {
				 FalconTrackMessage* trackMsg = new FalconTrackMessage (1,ownship->Id (), FalconLocalGame);
				 trackMsg->dataBlock.trackType = Track_RemoveWeapon;
				 trackMsg->dataBlock.hardpoint = curHardpoint;
				 trackMsg->dataBlock.id = ownship->Id ();
				 
				 // Send our track list
				 FalconSendMessage (trackMsg, TRUE);
			 }
			 
		 }
      }

      RemoveStore(curHardpoint, hardPoint[curHardpoint]->weaponId);

      // Make it live
      vuDatabase->Insert(theBomb);

      if (ownship == FalconLocalSession->GetPlayerEntity())
	  g_intellivibeData.BombDropped++;
	  // Note: The drawable for this object has already been created!
      theBomb->Wake();
      // Record the drop
      ownship->SendFireMessage (theBomb, FalconWeaponsFire::BMB, TRUE, ownship->targetPtr);

      lastWeapon = curWeapon;
      matchingStation = curHardpoint;
      WeaponStep(TRUE);
      if (lastWeapon == curWeapon)
      {
         curWeapon      = NULL;
      }

      // Want to drop a pair - No pairs w/ only one rack of bombs unless from centerline
      if (pair && allowRipple && (curHardpoint != matchingStation || curHardpoint == (numHardpoints / 2 + 1)))
      {
         theBomb = (BombClass *)(curWeapon);

         if (theBomb)
         {
            hardPoint[curHardpoint]->GetSubPosition(curWpnNum, &initXloc, &initYloc, &initZloc);
            pos.x = ownship->XPos() + ownship->dmx[0][0]*initXloc + ownship->dmx[1][0]*initYloc + ownship->dmx[2][0]*initZloc;
            pos.y = ownship->YPos() + ownship->dmx[0][1]*initXloc + ownship->dmx[1][1]*initYloc + ownship->dmx[2][1]*initZloc;
            pos.z = ownship->ZPos() + ownship->dmx[0][2]*initXloc + ownship->dmx[1][2]*initYloc + ownship->dmx[2][2]*initZloc;
            posDelta.x = ownship->XDelta();
            posDelta.y = ownship->YDelta();
            posDelta.z = ownship->ZDelta();
            theBomb->SetBurstHeight (burstHeight);
            theBomb->Start(&pos, &posDelta, dragCoeff, (ownship ? ((DigitalBrain *)ownship->Brain() ? ((DigitalBrain *)ownship->Brain())->GetGroundTarget() : NULL): NULL)); // 2002-02-26 MODIFIED BY S.G. Added the ownship... in case the target is aggregated and an AI is bombing it

            if ( IsSet( GunOnBoard ) )
			    slotId = max(0,curHardpoint - 1);
            else
	            slotId = curHardpoint;

            // Detach visual from parent
            if (hardPoint[curHardpoint]->GetRack())
            {
               OTWDriver.DetachObject(hardPoint[curHardpoint]->GetRack(), (DrawableBSP*)(theBomb->drawPointer), theBomb->GetRackSlot());
            }
            else if (ownship->drawPointer && theBomb->drawPointer && curHardpoint) // Keeps from detaching hidden (in bomb bay)
            {
               OTWDriver.DetachObject((DrawableBSP*)(ownship->drawPointer), (DrawableBSP*)(theBomb->drawPointer), slotId);
            }

            if (UnlimitedAmmo())
            {
               ReplaceBomb(curHardpoint, theBomb);
            }
            else
            {
               DecrementStores(hardPoint[curHardpoint]->GetWeaponClass(), 1);
               hardPoint[curHardpoint]->weaponCount --;
               if (hardPoint[curHardpoint]->weaponCount >= hardPoint[curHardpoint]->NumPoints())
                  ReplaceBomb(curHardpoint, theBomb);
               else
			   {
				   DetachWeapon(curHardpoint, theBomb);
				   
				   if (ownship->IsLocal ())
				   {
					   FalconTrackMessage* trackMsg = new FalconTrackMessage (1,ownship->Id (), FalconLocalGame);
					   trackMsg->dataBlock.trackType = Track_RemoveWeapon;
					   trackMsg->dataBlock.hardpoint = curHardpoint;
					   trackMsg->dataBlock.id = ownship->Id ();
					   
					   // Send our track list
					   FalconSendMessage (trackMsg, TRUE);
				   }
			   }
            }

            RemoveStore(curHardpoint, hardPoint[curHardpoint]->weaponId);

            // Make it live
            vuDatabase->QuickInsert(theBomb);
            theBomb->Wake();

            // Record the drop
            ownship->SendFireMessage (theBomb, FalconWeaponsFire::BMB, TRUE, ownship->targetPtr);

            // Step again
            lastWeapon = curWeapon;
            WeaponStep(TRUE);
            if (lastWeapon == curWeapon)
            {
               curWeapon      = NULL;
            }
         }
      }

      retval = TRUE;

      if (!curRippleCount)
      {
         // Can we do a ripple drop?
         if (allowRipple)
            curRippleCount = max (min (rippleCount, numCurrentWpn - 1), 0);
// 2001-04-27 MODIFIED BY S.G. SO THE RIPPLE DISTANCE IS INCREASED WITH ALTITUDE (SINCE BOMBS SLOWS DOWN)
//         nextDrop = SimLibElapsedTime + FloatToInt32((rippleInterval)/(float)sqrt(ownship->XDelta()*ownship->XDelta() + ownship->YDelta()*ownship->YDelta()) * SEC_TO_MSEC + (float)sqrt(ownship->ZPos() * -4.0f));
// JB 011017 Use old nextDrop ripple code instead of altitude dependant code.  I don't see why bombs slowing down would have an effect on ripple distance
// If anything is effected it would cause bombs to fall short which is not happening.  
// Besides this really screws up the code that decides when to start the ripple, and all the bombs don't fall off properly.
       nextDrop = SimLibElapsedTime + FloatToInt32((rippleInterval)/
				(float)sqrt(ownship->XDelta()*ownship->XDelta() + ownship->YDelta()*ownship->YDelta()) * SEC_TO_MSEC);
         if (!curRippleCount)
         {
            ClearFlag (Firing);
         }
      }
      else
      {
         if (curRippleCount == 1) // Last bomb, ripple count will be decremented in ::Exec
         {
            ClearFlag (Firing);
         }
      }

   }

   return (retval);
}

void SMSBaseClass::ReplaceBomb(int station, BombClass *theBomb)
{
VehicleClassDataType* vc;
int visFlag;
SimWeaponClass *weapPtr,*newBomb,*lastPtr = NULL;

	newBomb = InitABomb (ownship, hardPoint[station]->weaponId, theBomb->GetRackSlot());

	vc = GetVehicleClassData(ownship->Type() - VU_LAST_ENTITY_TYPE);
	visFlag = vc->VisibleFlags;

   if (visFlag & 1 << curHardpoint)
   {
	   CreateDrawable(newBomb, OTWDriver.Scale());

	   // Fix up drawable parent/child relations
	   if (hardPoint[station]->GetRack())
		{
		   OTWDriver.AttachObject(hardPoint[station]->GetRack(), (DrawableBSP*)(newBomb->drawPointer), theBomb->GetRackSlot());
		}
	   else if (ownship->drawPointer)
		{
		   OTWDriver.AttachObject((DrawableBSP*)(ownship->drawPointer), (DrawableBSP*)(newBomb->drawPointer), max (station - 1, 0));
		}
   }

	weapPtr = hardPoint[station]->weaponPointer;
	while (weapPtr)
		{
		if (weapPtr == theBomb)
			{
			if (lastPtr)
				lastPtr->nextOnRail = newBomb;
			else
				hardPoint[station]->weaponPointer = newBomb;
			newBomb->nextOnRail = theBomb->nextOnRail;
			theBomb->nextOnRail = NULL;
			return;
			}
		lastPtr = weapPtr;
		weapPtr = weapPtr->nextOnRail;
		}
}

