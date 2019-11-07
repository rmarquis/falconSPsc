#include "stdhdr.h"
#include "digi.h"
#include "simveh.h"
#include "fcc.h"
#include "sms.h"
#include "campwp.h"
#include "PilotInputs.h"
#include "otwdrive.h"
#include "airframe.h"
#include "aircrft.h"
#include "missile.h"
#include "bomb.h"
#include "feature.h"
#include "f4vu.h"
#include "simbase.h"
#include "mavdisp.h"
#include "fsound.h"
#include "soundfx.h"
#include "simdrive.h"
#include "unit.h"
#include "flight.h"
#include "objectiv.h"
#include "classtbl.h"
#include "entity.h"
#include "MsgInc\WeaponFireMsg.h"
#include "object.h"
#include "guns.h"
#include "wingorder.h"
#include "team.h"
#include "radar.h"
#include "laserpod.h"
#include "harmpod.h"
#include "campwp.h"
/* S.G. FOR WEAPON SELECTION */ #include "simstatc.h"
/* S.G. 2001-06-16 FOR FINDING IF TARGET HAS MORE THAN ONE RADAR */ #include "battalion.h"

extern SensorClass* FindLaserPod (SimMoverClass* theObject);

extern int g_nMissileFix;
extern float g_fBombMissileAltitude;
extern int g_nGroundAttackTime;

// 2001-06-28 ADDED BY S.G. HELPER FUNCTION TO TEST IF IT'S A SEAD MISSION ON ITS MAIN TARGET OR NOT
int DigitalBrain::IsNotMainTargetSEAD()
{
	// SEADESCORT has no main target
	if ( missionType == AMIS_SEADESCORT )
		return TRUE;

	// Only for SEADS missions
	if ( missionType != AMIS_SEADSTRIKE )
		return FALSE;

	// If not ground target, we can't check if it's our main target, right? So assume it's not our main target
	if (!groundTargetPtr)
		return TRUE;

	// Get the target campaign object if it's a sim
	CampBaseClass *campBaseTarget = ((CampBaseClass *)groundTargetPtr->BaseData());
	if (((SimBaseClass *)groundTargetPtr->BaseData())->IsSim())
		campBaseTarget = ((SimBaseClass *)groundTargetPtr->BaseData())->GetCampaignObject();

	// We should always have a waypoint but if we don't, can't be our attack waypoint right?
	if (!self->curWaypoint)
		return TRUE;

	// If it's not our main target, it's ok to stop attacking it
	if (self->curWaypoint->GetWPTarget() != campBaseTarget)
		return TRUE;

	// It's our main target, keep pounding it to death...
	return FALSE;
}
// END OF ADDED SECTION

void DigitalBrain::GroundAttackMode(void)
{
	FireControlComputer* FCC = self->FCC;
	SMSClass* Sms = self->Sms;
	RadarClass* theRadar = (RadarClass*) FindSensor (self, SensorClass::Radar);
	float approxRange, dx, dy;
	float pitchDesired;
	float  desSpeed;
	float xft, yft, zft;
	float rx, ata, ry;
	BOOL shootMissile;
	BOOL diveOK;
	float curGroundAlt;

// 2001-06-01 ADDED BY S.G. I'LL USE missileShotTimer TO PREVENT THIS ROUTINE FROM DOING ANYTHING. THE SIDE EFFECT IS WEAPONS HOLD SHOULD DO SOMETHING AS WELL
	if (SimLibElapsedTime < missileShotTimer)
		return;
// END OF ADDED SECTION

	// 2002-03-08 ADDED BY S.G. Turn off the lasing flag we were lasing (fail safe)
	if (SimLibElapsedTime > waitingForShot && (moreFlags & KeepLasing))
		moreFlags &= ~KeepLasing;
	// END OF ADDED SECTION 2002-03-08

// 2001-06-18 ADDED BY S.G. AI NEED TO RE-EVALUATE ITS TARGET FROM TIME TO TIME, UNLESS THE LEAD IS THE PLAYER
// 2001-06-18 ADDED BY S.G. NOPE, AI NEED TO RE-EVALUATE THEIR WEAPONS ONLY
// 2001-07-10 ADDED BY S.G. IF OUR TARGET IS A RADAR AND WE ARE NOT CARRYING HARMS, SEE IF SOMEBODY ELSE COULD DO THE JOB FOR US
// 2001-07-15 S.G. This was required when an non emitting radar could be targeted. Now check every 5 seconds if the campaign target has changed if we're the lead
	if (SimLibElapsedTime > nextAGTargetTime
		&& Sms->CurRippleCount() == 0) // JB 011017 Only reevaluate when we've dropped our ripple.
	{
		// 2001-07-20 S.G. ONLY CHANGE 'onStation' IF OUR WEAPON STATUS CHANGED...
		int tempWeapon = (hasHARM << 24) | (hasAGMissile << 16) | (hasGBU << 8) | hasBomb;

		SelectGroundWeapon();

		// So if we choose a different weapon, we'll set our FCC
		if (((hasHARM << 24) | (hasAGMissile << 16) | (hasGBU << 8) | hasBomb) != tempWeapon)
			if (onStation == Final)
				onStation = HoldInPlace;

		// 2001-07-12 S.G. Moved below the above code
		// 2001-07-12 S.G. Simply clear the target. Code below will do the selection (and a new attack profile for us)
		// See if we should change target if we're the lead of the flight (but first we must already got a target)
		if (!isWing && lastGndCampTarget) {
			UnitClass *campUnit = (UnitClass *)self->GetCampaignObject();
			if (campUnit) {
				campUnit->UnsetChecked();
				if (campUnit->ChooseTarget()) {
					if (lastGndCampTarget != campUnit->GetTarget())
						SetGroundTargetPtr(NULL);
				}
			}

		}

// 2001-07-15 S.G. This was required when an non emitting radar could be targeted. This is not the case anymore so skip the whole thing
		// Try again in 5 seconds.
		nextAGTargetTime = SimLibElapsedTime + 5000;
	}

	// 2001-07-12 S.G. Need to force a target reselection under some conditions. If we're in GroundAttackMode, it's because we are already in agDoctrine != AGD_NONE which mean we already got a target originally
	// This was done is some 'onStation' modes but I moved it here instead so all modes runs it
	if ( groundTargetPtr &&
		 ( ( groundTargetPtr->BaseData()->IsSim() && // For a SIM entity
		     ( groundTargetPtr->BaseData()->IsDead() ||
			   groundTargetPtr->BaseData()->IsExploding() ||
			   ( !isWing && IsNotMainTargetSEAD() &&
			     !((SimBaseClass *)groundTargetPtr->BaseData())->GetCampaignObject()->IsEmitting() ) ) ) || 
		   ( groundTargetPtr->BaseData()->IsCampaign() && // For a CAMPAIGN entity
		     ( !isWing && IsNotMainTargetSEAD() &&
			   !((CampBaseClass *)groundTargetPtr->BaseData())->IsEmitting() ) ) ) )
		SetGroundTarget( NULL );

	int reSetup = FALSE;
	if (!groundTargetPtr) {
		// Wings run a limited version of the target selection (so they don't switch campaign target)
		if (isWing)
			AiRunTargetSelection();
		else
			SelectGroundTarget(TARGET_ANYTHING);

		// Since 'AiRunTargetSelection' doesn't set it but 'SelectGroundTarget' does, force it to FALSE
		// Nope, let 'Final' and SetupAGMode deal with this
//		madeAGPass = FALSE;

		// Force a run of SetupAGMode if we finally got a target
		if (groundTargetPtr)
			reSetup = TRUE;
	}

	// If we got a deaggregated campaign object, find a sim object within
	if (groundTargetPtr && groundTargetPtr->BaseData()->IsCampaign() && !((CampBaseClass *)groundTargetPtr->BaseData())->IsAggregate()) {
		SelectGroundTarget( TARGET_ANYTHING );
		reSetup = TRUE;
	}

	if (reSetup) {
		nextAGTargetTime = 0;
		SetupAGMode(NULL, NULL);
		onStation = NotThereYet;
	}
// END OF ADDED SECTION

	// NOTES ON MODES (in approximate order):
	// 1) OnStation: Initial setup for ground run.  Track to IP.
	//	  Also determine if another run is to be made, or end ground run.
	// 2) CrossWind: head to IP
	// 3) HoldInPlace: Track Towards target
	// 4) Final: ready weapons and fire when within parms.
	// 5) Final1: release more weapoons if appropriate then go back to OnStation

    curGroundAlt = OTWDriver.GetGroundLevel( self->XPos(), self->YPos() );

    if (agDoctrine == AGD_NEED_SETUP)
    {
      SetupAGMode( NULL, NULL );
    }

// 2001-07-18 ADDED BY S.G. IF ON A LOOK_SHOOT_LOOK MODE WITH A TARGET, SEND AN ATTACK RIGHT AWAY AND DON'T WAIT TO BE CLOSE TO/ON FINAL TO DO IT BECAUSE YOU'LL MAKE SEVERAL PASS ANYWAY...

	if (groundTargetPtr && agDoctrine == AGD_LOOK_SHOOT_LOOK) {
		if (!isWing) {
			if (sentWingAGAttack == AG_ORDER_NONE ) {
				AiSendCommand (self, FalconWingmanMsg::WMTrail, AiFlight, FalconNullId);
				sentWingAGAttack = AG_ORDER_FORMUP;
				// 2002-01-20 ADDED BY S.G. Added the new nextAttackCommandToSend variable check to force the lead to reissue an attack in case wings went back into formation (can we say HACK?)
				nextAttackCommandToSend = SimLibElapsedTime + 60 * SEC_TO_MSEC;
			}
			// 2002-01-20 ADDED BY S.G. Either we haven't sent the attack order or we sent it a while ago, check if we should send it again
			else if  (sentWingAGAttack != AG_ORDER_ATTACK || SimLibElapsedTime > nextAttackCommandToSend) {
				 VU_ID targetId;

				 if (groundTargetPtr->BaseData()->IsSim())
				 {
					   targetId = ((SimBaseClass*)groundTargetPtr->BaseData())->GetCampaignObject()->Id();
				 }
				 else
					   targetId = groundTargetPtr->BaseData()->Id();
			
				 if (targetId != FalconNullId)
				 {
					// 2002-01-20 ADDED BY S.G. Check the wing's AI to see if they have weapon but in formation
					int sendAttack = FALSE;
					if (SimLibElapsedTime > nextAttackCommandToSend) { // timed out
						int i;
						int usComponents = self->GetCampaignObject()->NumberOfComponents();
						for (i = 0; i < usComponents; i++) {
							AircraftClass *flightMember = (AircraftClass *)self->GetCampaignObject()->GetComponentEntity(i);
							// This code is assuming the lead and the AI are on the same PC... Should be no problem unless another player is in Combat AP...
							if (flightMember && flightMember->DBrain() && flightMember->DBrain()->IsSetATC(IsNotMainTargetSEAD() ? HasAGWeapon : HasCanUseAGWeapon) && flightMember->DBrain()->agDoctrine == AGD_NONE) {
								sendAttack = TRUE;
								break;
							}
						}
					}
					else
						sendAttack = TRUE;

					if (sendAttack) {
						AiSendCommand (self, FalconWingmanMsg::WMAssignTarget, AiFlight, targetId);
						AiSendCommand (self, FalconWingmanMsg::WMShooterMode, AiFlight, targetId);
						sentWingAGAttack = AG_ORDER_ATTACK;
					}
					// 2002-01-20 ADDED BY S.G. Added the new nextAttackCommandToSend variable check to force the lead to reissue an attack in case wings went back into formation (can we say HACK?)
					nextAttackCommandToSend = SimLibElapsedTime + 60 * SEC_TO_MSEC;
				 }
			}
		}
	}
// END OF ADDED SECTION

	// modes for ground attack
   	switch (onStation)
   	{
		// protect against invalid state by going to our start condition
		// then fall thru
		default:
			onStation = NotThereYet;
		   // #1 setup....
		   // We Should have an insertion point set up at ipX,Y and Z.....
      	case NotThereYet:

			// have we already made an AG Attack run?
			if ( madeAGPass )
			{
// 2001-05-13 ADDED BY S.G. SO AI DROPS SOME COUNTER MEASURE AFTER A PASS
				self->DropProgramed();
// END OF ADDED SECTION

				// clear weapons and target
				FCC->SetMasterMode (FireControlComputer::Nav);
				SetGroundTarget( NULL );
				FCC->preDesignate = TRUE;

				// if we've already made 1 pass and the doctrine is
				// shoot_run, then we're done with this waypoint go to
				// next.  Otherwise we're in look_shoot_look.   We'll remain
				// at the current waypoint and allow the campaign to retarget
				// for us
				if ( agDoctrine == AGD_SHOOT_RUN )
				{
					if ( agImprovise == FALSE )
						SelectNextWaypoint();
					// go back to initial AG state
					agDoctrine = AGD_NONE;
					missionComplete = TRUE;
					// if we're a wingie, rejoin the lead
					if ( isWing )
					{
// 2001-05-03 ADDED BY S.G. WE WANT WEDGE AFTER GROUND PASS!
						mFormation = FalconWingmanMsg::WMWedge;
// END OF ADDED SECTION
						AiRejoin( NULL );
						// make sure wing's designated target is NULL'd out
						mDesignatedObject = FalconNullId;
					}
					return;
				}

// 2001-06-18 MODIFIED BY S.G. I'LL USE THIS TO HAVE THE AI FORCE A RETARGET AND 15 SECONDS IS TOO HIGH
//				nextAGTargetTime = SimLibElapsedTime + 15000;
				nextAGTargetTime = SimLibElapsedTime + 5000;

// 2001-07-12 ADDED BY S.G. SINCE WE'RE GOING AFTER SOMETHING NEW, GET A NEW TARGET RIGHT NOW SO agDoctrine DOESN'T GET SET TO AGD_NONE WHICH WILL END UP SetupAGMode BEING CALLED BEFORE THE AI HAS A CHANCE TO REACH ITS TURN POINT
				if (isWing)
					AiRunTargetSelection();
				else
					SelectGroundTarget(TARGET_ANYTHING);

				// Since 'AiRunTargetSelection' doesn't set it but 'SelectGroundTarget' does, force it to FALSE
				// Nope, let 'Final' and SetupAGMode deal with this
//				madeAGPass = FALSE;

				// If we got a deaggregated campaign object, find a sim object within
				if (groundTargetPtr && groundTargetPtr->BaseData()->IsCampaign() && !((CampBaseClass *)groundTargetPtr->BaseData())->IsAggregate())
					SelectGroundTarget( TARGET_ANYTHING );
			}

// 2001-07-12 REMOVED BY S.G. DONE ABOVE BEFORE DOING onStation SPECIFIC THINGS
			// choose how we are going to attack and whom....
//			SelectGroundTarget(TARGET_ANYTHING);

			// select weapons....
//			SelectGroundWeapon();

			if ( !isWing && sentWingAGAttack == AG_ORDER_NONE )
			{
				AiSendCommand (self, FalconWingmanMsg::WMTrail, AiFlight, FalconNullId);
				sentWingAGAttack = AG_ORDER_FORMUP;
				// 2002-01-20 ADDED BY S.G. Added the new nextAttackCommandToSend variable check to force the lead to reissue an attack in case wings went back into formation (can we say HACK?)
				nextAttackCommandToSend = SimLibElapsedTime + 60 * SEC_TO_MSEC;
			}


			// check to see if we're too close to target (if we've got one)
			// if so set up new IP
			if ( groundTargetPtr )
			{
				trackX = groundTargetPtr->BaseData()->XPos();
				trackY = groundTargetPtr->BaseData()->YPos();
				dx = ( self->XPos() - trackX );
				dy = ( self->YPos() - trackY );
				approxRange = (float)sqrt( dx * dx + dy * dy );
// 2001-07-24 MODIFIED BY S.G. DON'T DISCRIMINATE AGMISSILES. IT'S 3.5 NM FOR EVERY WEAPON
//				if ( (hasAGMissile && approxRange < 7.5f * NM_TO_FT) || approxRange < 3.5F * NM_TO_FT )
				if ( approxRange < 3.5F * NM_TO_FT )
				{
					dx /= approxRange;
					dy /= approxRange;
					ipX = trackX + dy * 6.5f * NM_TO_FT;
					ipY = trackY - dx * 6.5f * NM_TO_FT;
					ShiAssert (ipX > 0.0F);
				}
			}

			// track to insertion point
			trackX = ipX;
			trackY = ipY;
			trackZ = ipZ;


         FCC->SetMasterMode (FireControlComputer::Nav);
         FCC->preDesignate = TRUE;


		  	dx = (float)fabs( self->XPos() - trackX );
		  	dy = (float)fabs( self->YPos() - trackY );
			approxRange = (float)sqrt( dx * dx + dy * dy );


			// Terrain follow around 1000 ft
// 2001-07-12 MODIFIED BY S.G. SO SEAD STAY LOW UNTIL READY TO ATTACK
//			if ( agApproach == AGA_LOW )
			if ( agApproach == AGA_LOW || missionType == AMIS_SEADESCORT || missionType == AMIS_SEADSTRIKE)
			{
				// see if we're too close to set up the ground run
				// if so, we're going to head to a new point perpendicular to
				// our current direction and make a run from there
				// this is kind of a sanity check
			   trackZ = OTWDriver.GetGroundLevel( self->XPos() + self->XDelta(),
												  self->YPos() + self->YDelta() );
			   // if we're below track alt, kick us up a bit harder so we don't plow
			   // into steeper slopes
			   if ( self->ZPos() - trackZ > -1000.0f )
					trackZ = trackZ - 1000.0f - ( self->ZPos() - trackZ + 1000.0f ) * 2.0f;
			   else
					trackZ -= 1000.0f;
			   desSpeed = 650.0f;
			}
			else
			{
				// see if we're too close to set up the ground run
				// if so, we're going to head to a new point perpendicular to
				// our current direction and make a run from there
				// this is kind of a sanity check
			   desSpeed = 450.0f;
			   if ( self->ZPos() - curGroundAlt > -500.0f )
			   {
			   	   trackZ = OTWDriver.GetGroundLevel( self->XPos() + self->XDelta(),
												  self->YPos() + self->YDelta() );
				   if ( self->ZPos() - trackZ > -500.0f )
						trackZ = trackZ - 500.0f - ( self->ZPos() - trackZ + 500.0f ) * 2.0f;
// 2001-06-18 ADDED S.G. WHY DO THIS IF WE'RE GOING UP ALREADY?
				   else if (agApproach == AGA_HIGH && trackZ > ipZ) // Are we going up? (don't forget negative is UP)
					    trackZ = ipZ;
// END OF ADDED SECTION
				   else
						trackZ -= 500.0f;
			   }
			}
	
	
			// bombs dropped in pairs
	     	Sms->SetPair(TRUE);

// 2001-05-13 ADDED BY S.G. WITHOUT THIS, I WOULD GO BACK REDO THE CROSSWIND AFTER THE MISSION IS OVER! WAYPOINTS WILL OVERIDE trackXYZ SO DON'T WORRY ABOUT THEM
			if (!missionComplete)
// END OF ADDED SECTION
				// head to IP
				onStation = Crosswind;

			desSpeed = max(200.0F, min (desSpeed, 700.0F));	// Knots
			TrackPoint(0.0F, desSpeed * KNOTS_TO_FTPSEC);

      		break;

		   // #2 heading towards target -- we've reached the IP and are heading in
		   // set up out available weapons (pref to missiles) and head in
		   // also set up our final approach tracking
      	case HoldInPlace:

// 2001-07-12 REMOVED BY S.G. DONE ABOVE
//			if ( groundTargetPtr == NULL || groundTargetPtr->BaseData()->IsCampaign() )
//				SelectGroundTarget(TARGET_ANYTHING);

// 2001-07-12 S.G. Moved so a retargeting is not done
FinalSG: // 2001-06-24 ADDED BY S.G. JUMPS BACK HERE IF TOO CLOSE FOR HARMS AND TARGET NOT EMITTING
		  	dx = (float)fabs( self->XPos() - trackX );
		  	dy = (float)fabs( self->YPos() - trackY );
			approxRange = (float)sqrt(dx*dx + dy*dy);

			if ( groundTargetPtr )
			{
				trackX = groundTargetPtr->BaseData()->XPos();
				trackY = groundTargetPtr->BaseData()->YPos();
			}
			trackZ = OTWDriver.GetGroundLevel( trackX, trackY );

// 2001-10-31 ADDED by M.N. hope this fixes the SEAD circling bug
			if (groundTargetPtr)
			{
				float approxTargetRange;
				xft = trackX - self->XPos();
				yft = trackY - self->YPos();
				zft = trackZ - self->ZPos();
				approxTargetRange = (float)sqrt(xft*xft + yft*yft + zft*zft);
				approxTargetRange = max (approxTargetRange, 1.0F);

			
/*			xft = trackX - self->XPos();
			yft = trackY - self->YPos();
			zft = trackZ - self->ZPos();
			approxRange = (float)sqrt(xft*xft + yft*yft + zft*zft);
			approxRange = max (approxRange, 1.0F);
			rx = self->dmx[0][0]*xft + self->dmx[0][1]*yft + self->dmx[0][2]*zft;
			ry = self->dmx[1][0]*xft + self->dmx[1][1]*yft + self->dmx[1][2]*zft;
			ata =  (float)acos(rx/approxRange); */

// take ata into account ??
// need to know minimum engagement distance for a HARM...

					
				if (approxTargetRange < 3.0f * NM_TO_FT /*&& ata > 75.0f * DTR*/)
				{
					// Bail and try again
					dx = ( self->XPos() - trackX );
					dy = ( self->YPos() - trackY );
					approxRange = (float)sqrt( dx * dx + dy * dy );
					dx /= approxRange;
					dy /= approxRange;
					ipX = trackX + dy * 5.5f * NM_TO_FT;
					ipY = trackY - dx * 5.5f * NM_TO_FT;
					ShiAssert (ipX > 0.0F);
					// go to IP
					onStation = Crosswind;
					break;
				}
			}

// 2001-07-18 REMOVED BY S.G. agApproach SHOULDN'T MAKE A DIFFERENCE HERE
//			if ( hasAGMissile && agApproach != AGA_HIGH )
			if ( hasAGMissile)
			{
				FCC->SetMasterMode (FireControlComputer::AirGroundMissile);
				FCC->SetSubMode (FireControlComputer::SLAVE);
            FCC->preDesignate = TRUE;
            //MonoPrint ("Setup For Maverick\n");
			}
			else if ( hasHARM)
			{
				FCC->SetMasterMode (FireControlComputer::AirGroundHARM);
				FCC->SetSubMode (FireControlComputer::HTS);
            //MonoPrint ("Setup For HARM\n");
			}
         else if (hasGBU)
         {
   			FCC->SetMasterMode (FireControlComputer::AirGroundLaser);
				FCC->SetSubMode (FireControlComputer::SLAVE);
            FCC->preDesignate = TRUE;
            FCC->designateCmd = TRUE;
            //MonoPrint ("Setup For GBU\n");
         }
			else if (hasBomb)
			{
   			FCC->SetMasterMode (FireControlComputer::AirGroundBomb);
				FCC->SetSubMode (FireControlComputer::CCRP);

				FCC->groundDesignateX = trackX;
				FCC->groundDesignateY = trackX;
				FCC->groundDesignateZ = trackZ;
            //MonoPrint ("Setup For Iron Bomb\n");
			}

         // Point the radar at the target
	      if (theRadar) 
         {
// 2001-07-23 MODIFIED BY S.G. MOVERS ARE ONLY 3D ENTITIES WHILE BATTALIONS WILL INCLUDE 2D AND 3D VEHICLES...
//          if (groundTargetPtr && groundTargetPtr->BaseData()->IsMover())
            if (groundTargetPtr && ((groundTargetPtr->BaseData()->IsSim() && ((SimBaseClass *)groundTargetPtr->BaseData())->GetCampaignObject()->IsBattalion()) || (groundTargetPtr->BaseData()->IsCampaign() && groundTargetPtr->BaseData()->IsBattalion())))
               theRadar->SetMode(RadarClass::GMT);
            else
               theRadar->SetMode(RadarClass::GM);
		      theRadar->SetDesiredTarget(groundTargetPtr);
            theRadar->SetAGSnowPlow(TRUE);
         }

			// Terrain follow around 1000 ft
// 2001-07-12 MODIFIED BY S.G. SO SEAD STAY LOW UNTIL READY TO ATTACK
//			if ( agApproach == AGA_LOW )
			if ( agApproach == AGA_LOW || missionType == AMIS_SEADESCORT || missionType == AMIS_SEADSTRIKE)
			{
			   trackZ = OTWDriver.GetGroundLevel( self->XPos() + self->XDelta(),
												  self->YPos() + self->YDelta() );
			   // if we're below track alt, kick us up a bit harder so we don't plow
			   // into steeper slopes
			   if ( self->ZPos() - trackZ > -1000.0f )
					trackZ = trackZ - 1000.0f - ( self->ZPos() - trackZ + 1000.0f ) * 2.0f;
			   else
					trackZ -= 1000.0f;
			   desSpeed = 650.0f;
			}
			else if ( agApproach == AGA_DIVE )
			{
			   trackZ = OTWDriver.GetGroundLevel( trackX, trackY );
			   if ( Sms->curWeapon )
			   		trackZ -= 100.0f;
			   desSpeed = 450.0f;
			   if ( self->ZPos() - curGroundAlt > -1000.0f )
			   {
			   	   trackZ = OTWDriver.GetGroundLevel( self->XPos() + self->XDelta(),
												  self->YPos() + self->YDelta() );
				   if ( self->ZPos() - trackZ > -500.0f )
						trackZ = trackZ - 500.0f - ( self->ZPos() - trackZ + 500.0f ) * 2.0f;
				   else
						trackZ -= 500.0f;
			   }
			}
			else
			{
// 2002-03-28 MN fix for AI handling of JSOW/JDAM missiles - fire from defined altitude instead of 4000ft like Mavericks
				if ((g_nMissileFix & 0x01)&& Sms && Sms->curWeapon)
				{
					Falcon4EntityClassType* classPtr = NULL;
					WeaponClassDataType *wc = NULL;

					classPtr = (Falcon4EntityClassType*)Sms->curWeapon->EntityType();
					if (classPtr)
					{
						wc = (WeaponClassDataType*)classPtr->dataPtr;
						if (wc && (wc->Flags & WEAP_BOMBWARHEAD))
						{
							ipZ = -g_fBombMissileAltitude;
						}
					}
				}
			   trackZ = ipZ;
			   desSpeed = 450.0f;
			   if ( self->ZPos() - curGroundAlt > -500.0f )
			   {
			   	   trackZ = OTWDriver.GetGroundLevel( self->XPos() + self->XDelta(),
												  self->YPos() + self->YDelta() );
				   if ( self->ZPos() - trackZ > -500.0f )
						trackZ = trackZ - 500.0f - ( self->ZPos() - trackZ + 500.0f ) * 2.0f;
// 2001-06-18 ADDED S.G. WHY DO THIS IF WE'RE GOING UP ALREADY?
				   else if (trackZ > ipZ) // Are we going up? (don't forget negative is UP)
					    trackZ = ipZ;
// END OF ADDED SECTION
				   else
						trackZ -= 500.0f;
			   }
			}

			onStation = Final;
// 2001-05-21 ADDED BY S.G. ONLY DO THIS IF NOT WITHIN TIMEOUT PERIOD. TO BE SAFE, I'LL SET waitingForShot TO 0 IN digimain SO IT IS INITIALIZED
			if (waitingForShot < SimLibElapsedTime - 5000)
				// Say you can fire when ready
// END OF ADDED SECTION
				waitingForShot = SimLibElapsedTime - 1;

			TrackPoint(0.0F, desSpeed * KNOTS_TO_FTPSEC);
			// SimpleTrack(SimpleTrackSpd, desSpeed * KNOTS_TO_FTPSEC);

	   		break;

		// case 1a: head to good start location (IP)
      	case Crosswind:
			trackX = ipX;
			trackY = ipY;
			trackZ = ipZ;

		  	dx = (float)fabs( self->XPos() - trackX );
		  	dy = (float)fabs( self->YPos() - trackY );
			approxRange = (float)sqrt( dx * dx + dy * dy );

			// Terrain follow around 1000 ft
// 2001-07-12 MODIFIED BY S.G. SO SEAD STAY LOW UNTIL READY TO ATTACK
//			if ( agApproach == AGA_LOW )
			if ( agApproach == AGA_LOW || missionType == AMIS_SEADESCORT || missionType == AMIS_SEADSTRIKE)
			{
			   trackZ = OTWDriver.GetGroundLevel( self->XPos() + self->XDelta(),
												  self->YPos() + self->YDelta() );
			   // if we're below track alt, kick us up a bit harder so we don't plow
			   // into steeper slopes
			   if ( self->ZPos() - trackZ > -1000.0f )
					trackZ = trackZ - 1000.0f - ( self->ZPos() - trackZ + 1000.0f ) * 2.0f;
			   else
					trackZ -= 1000.0f;
			   desSpeed = 650.0f;
			}
			else
			{
			   desSpeed = 450.0f;
			   if ( self->ZPos() - curGroundAlt > -500.0f )
			   {
			   	   trackZ = OTWDriver.GetGroundLevel( self->XPos() + self->XDelta(),
												  self->YPos() + self->YDelta() );
				   if ( self->ZPos() - trackZ > -500.0f )
						trackZ = trackZ - 500.0f - ( self->ZPos() - trackZ + 500.0f ) * 2.0f;
// 2001-06-18 ADDED S.G. WHY DO THIS IF WE'RE GOING UP ALREADY?
				   else if (agApproach == AGA_HIGH && trackZ > ipZ) // Are we going up? (don't forget negative is UP)
					    trackZ = ipZ;
// END OF ADDED SECTION
				   else
						trackZ -= 500.0f;
			   }
			}

			TrackPoint(0.0F, desSpeed * KNOTS_TO_FTPSEC);

// 2001-05-05 ADDED BY S.G. THIS IS TO MAKE THE AI PULL AGGRESIVELY AFTER A PASS. I WOULD HAVE LIKE TESTING madeAGPass BUT IT IS CLEARED BEFORE :-(
			// Increase the gains on final approach
			rStick *= 3.0f;
			if ( rStick > 1.0f )
				rStick = 1.0f;
			else if ( rStick < -1.0f )
				rStick = -1.0f;
// END OF ADDED SECTION

// 2001-07-12 ADDED BY S.G. IF CLOSE TO THE FINAL POINT, SEND AN ATTACK COMMAND TO THE WINGS
			// tell our wing to attack
			if (groundTargetPtr && approxRange < 5.0f * NM_TO_FT) {
				if ( !isWing && sentWingAGAttack != AG_ORDER_ATTACK)
				{
					 VU_ID targetId;

					 if (groundTargetPtr->BaseData()->IsSim())
					 {
						   targetId = ((SimBaseClass*)groundTargetPtr->BaseData())->GetCampaignObject()->Id();
					 }
					 else
						   targetId = groundTargetPtr->BaseData()->Id();
				
					 if (targetId != FalconNullId)
					 {
						AiSendCommand (self, FalconWingmanMsg::WMAssignTarget, AiFlight, targetId);
						AiSendCommand (self, FalconWingmanMsg::WMShooterMode, AiFlight, targetId);
						sentWingAGAttack = AG_ORDER_ATTACK;
						// 2002-01-20 ADDED BY S.G. Added the new nextAttackCommandToSend variable check to force the lead to reissue an attack in case wings went back into formation (can we say HACK?)
						nextAttackCommandToSend = SimLibElapsedTime + 60 * SEC_TO_MSEC;
					 }
				}
			}
// END OF ADDED SECTION

			// are we about at our IP?
// 2001-07-23 MODIFIED BY S.G. IF NO WEAPON AVAIL FOR TARGET, SWITCH TO FINAL RIGHT AWAY
//			if ( approxRange < 1.3f * NM_TO_FT)
			if ( approxRange < 1.3f * NM_TO_FT || !IsSetATC(HasCanUseAGWeapon))
			{
				// next mode
				onStation = HoldInPlace;

// 2001-07-15 ADDED BY S.G. IF madeAGPass IS TRUE, WE MADE AN A2G PASS AND WAS TURNING AWAY. REDO AN ATTACK PROFILE FOR A NEW PASS
				if (madeAGPass) {
					madeAGPass = FALSE;
					onStation = NotThereYet;
				}

// 2001-07-12 ADDED BY S.G. MAKE SURE OUR IP ALTITUDE IS RIGHT BEFORE SWITCHING TO 'HoldInPlace' (WHICH IS A PRESET FOR Final)
		        SetupAGMode(NULL, NULL);
			}

      		break;

      	case Downwind:
      		break;

      	case Base:
      		break;

		// #3 -- final attack approach
      	case Final:

			ClearFlag( GunFireFlag );

			// double check to make sure ground target is still alive if
			// it's a sim target
// 2001-07-12 REMOVED BY S.G. DONE BEFORE THE switch STATEMENT
/*
			if ( groundTargetPtr &&
				 groundTargetPtr->BaseData()->IsSim() &&
				 ( groundTargetPtr->BaseData()->IsDead() ||
				   groundTargetPtr->BaseData()->IsExploding() ) )
			{
				SetGroundTarget( NULL );
			}

			if ( groundTargetPtr == NULL || groundTargetPtr->BaseData()->IsCampaign() )
				SelectGroundTarget(TARGET_ANYTHING);
*/
			if ( groundTargetPtr )
			{
				trackX = groundTargetPtr->BaseData()->XPos();
				trackY = groundTargetPtr->BaseData()->YPos();

				// tell our wing to attack
				if ( !isWing && sentWingAGAttack != AG_ORDER_ATTACK)
				{
					 VU_ID targetId;

					 if (groundTargetPtr->BaseData()->IsSim())
					 {
						   targetId = ((SimBaseClass*)groundTargetPtr->BaseData())->GetCampaignObject()->Id();
					 }
					 else
						   targetId = groundTargetPtr->BaseData()->Id();
			
					 if (targetId != FalconNullId)
					 {
						AiSendCommand (self, FalconWingmanMsg::WMAssignTarget, AiFlight, targetId);
						AiSendCommand (self, FalconWingmanMsg::WMShooterMode, AiFlight, targetId);
						sentWingAGAttack = AG_ORDER_ATTACK;
						// 2002-01-20 ADDED BY S.G. Added the new nextAttackCommandToSend variable check to force the lead to reissue an attack in case wings went back into formation (can we say HACK?)
						nextAttackCommandToSend = SimLibElapsedTime + 60 * SEC_TO_MSEC;
					 }
				}
			}

			diveOK = FALSE;

			// Terrain follow around 1000 ft
			if ( agApproach == AGA_LOW )
			{
			   // if we're below track alt, kick us up a bit harder so we don't plow
			   // into steeper slopes
			   if (Sms->curWeapon && !Sms->curWeapon->IsMissile() && Sms->curWeapon->IsBomb())
			   {
				   if ( self->ZPos() - curGroundAlt > -1000.0f )
				   {
					   trackZ = OTWDriver.GetGroundLevel( self->XPos() + self->XDelta(),
													  self->YPos() + self->YDelta() );
					   if ( self->ZPos() - trackZ > -1000.0f )
							trackZ = trackZ - 1000.0f - ( self->ZPos() - trackZ + 1000.0f ) * 2.0f;
					   else
							trackZ -= 1000.0f;
				   }
				   else
				   {
			   	   		trackZ = OTWDriver.GetGroundLevel( trackX, trackY ) - 4000.0f;
				   }
			   }
			   else
			   {
			   	   trackZ = OTWDriver.GetGroundLevel( self->XPos() + self->XDelta(),
												  self->YPos() + self->YDelta() );
				   if ( self->ZPos() - trackZ > -1000.0f )
						trackZ = trackZ - 1000.0f - ( self->ZPos() - trackZ + 1000.0f ) * 2.0f;
				   else
						trackZ -= 1000.0f;
			   }
			   if ( madeAGPass )
				   desSpeed = 450.0f;
			   else
				   desSpeed = 650.0f;
			}
			else if ( agApproach == AGA_DIVE )
			{
			   if ( self->ZPos() - curGroundAlt > -1000.0f )
			   {
			   	   trackZ = OTWDriver.GetGroundLevel( self->XPos() + self->XDelta(),
												  self->YPos() + self->YDelta() );
				   if ( self->ZPos() - trackZ > -500.0f )
						trackZ = trackZ - 500.0f - ( self->ZPos() - trackZ + 500.0f ) * 2.0f;
				   else
						trackZ -= 500.0f;
			   }
			   else
			   {
			   	   trackZ = OTWDriver.GetGroundLevel( trackX, trackY );
				   diveOK = TRUE;
				   if (Sms->curWeapon && !Sms->curWeapon->IsMissile() && Sms->curWeapon->IsBomb())
						trackZ -= 2000.0f;
				   else if ( Sms->curWeapon)
						trackZ -= 50.0f;
			   }
			   desSpeed = 450.0f;
			}
			else
			{
			   trackZ = ipZ;
			   desSpeed = 450.0f;
			   if ( self->ZPos() - curGroundAlt > -1000.0f )
			   {
			   	   trackZ = OTWDriver.GetGroundLevel( self->XPos() + self->XDelta(),
												  self->YPos() + self->YDelta() );
				   if ( self->ZPos() - trackZ > -500.0f )
						trackZ = trackZ - 500.0f - ( self->ZPos() - trackZ + 500.0f ) * 2.0f;
// 2001-06-18 ADDED S.G. WHY DO THIS IF WE'RE GOING UP ALREADY?
				   else if (trackZ > ipZ) // Are we going up? (don't forget negative is UP)
					    trackZ = ipZ;
// END OF ADDED SECTION
				   else
						trackZ -= 500.0f;
			   }
			}

			// if we've got a missile get r mmax and min (which should be
			// fairly accurate now.
			// also we're going to predetermine if we'll take a missile shot
			// or not (mostly for harms).
			shootMissile = FALSE;
			droppingBombs = FALSE;

			// get accurate range and ata to target
			xft = trackX - self->XPos();
			yft = trackY - self->YPos();
			zft = trackZ - self->ZPos();
			approxRange = (float)sqrt(xft*xft + yft*yft + zft*zft);
			approxRange = max (approxRange, 1.0F);

         // check for target
			if ( groundTargetPtr == NULL )
			{
				if ( approxRange < 1000.0f )
					onStation = NotThereYet;
				break;
			}
			
			rx = self->dmx[0][0]*xft + self->dmx[0][1]*yft + self->dmx[0][2]*zft;
			ry = self->dmx[1][0]*xft + self->dmx[1][1]*yft + self->dmx[1][2]*zft;
			ata =  (float)acos(rx/approxRange);

			// check for a photo mission
			if ((missionType == AMIS_BDA || missionType == AMIS_RECON) && hasCamera )
			{
				TakePicture(approxRange, ata);
			}
			// Might shoot a missile
			else
			{
			   // preference given to stand-off missiles unless our
			   // approach is high alt (bombing run)
// 2001-07-18 ADDED BY S.G. DON'T GO TO THE TARGET BUT DO ANOTHER PASS FROM IP IF YOU HAVE NO WEAPONS LEFT!
			   if (!IsSetATC(HasCanUseAGWeapon)) {
				   onStation = NotThereYet;
				   break;
			   }
// END OF ADDED SECTION

            if ((hasAGMissile || hasHARM) && groundTargetPtr )
			   {
               if (hasHARM)
				      shootMissile = HARMSetup(rx, ry, ata, approxRange);
               else
                  shootMissile = MaverickSetup(rx, ry, ata, approxRange, theRadar);

               // Fire when ready Gridley
// 2001-06-24 MODIFIED BY S.G. shootMissile is TRUE, YOU'RE CLEAR TO SHOOT
//			      if ( shootMissile )
			      if ( shootMissile == TRUE)
			      {
	                  FireAGMissile(approxRange, ata);
			      }
// 2001-06-24 ADDED BY S.G. IF shootMissile IS NOT FALSE (AND ALSO NOT TRUE), IF WE'RE CLOSE AND CAN'T LAUNCH HARM, SAY 'NO HARMS' AND TRY AGAIN RIGHT AWAY
				  else if ( shootMissile != FALSE) {
					  hasHARM = FALSE;
					  goto FinalSG;
				  }
// END OF ADDED SECTION
            }
            else if (hasGBU)
            {
               DropGBU(approxRange, ata, theRadar);
            }
            else if (hasBomb)
            {
               DropBomb(approxRange, ata, theRadar);
            }
			   // rocket strafe attack
			   else if (hasRocket)
			   {
               FireRocket(approxRange, ata);
			   }
			   // gun strafe attack
			   else if (hasGun && agApproach == AGA_DIVE )
			   {
               GunStrafe(approxRange, ata);
			   }
			   // too close ?
			   // we're within a certain range and our ATA is not good
			   else if ( approxRange < 1.2f * NM_TO_FT && ata > 75.0f * DTR)
			   {
				    waitingForShot = SimLibElapsedTime + 5000;
				    onStation = Final1;
			   }
         }

			TrackPoint(0.0F, desSpeed * KNOTS_TO_FTPSEC );

		  	dx = (float)fabs( self->XPos() - trackX );
		  	dy = (float)fabs( self->YPos() - trackY );
			approxRange = (float)sqrt( dx * dx + dy * dy );

			// Increase the gains on final approach
			rStick *= 3.0f;
			if ( rStick > 1.0f )
				rStick = 1.0f;
			else if ( rStick < -1.0f )
				rStick = -1.0f;

			// pitch stick setting is based on our desired angle normalized to
			// 90 deg when in a dive
			if ( agApproach == AGA_DIVE && diveOK )
			{
				// check hitting the ground and pull out of dive
				pitchDesired = (float)atan2( self->ZPos() - trackZ, approxRange );
				pitchDesired /= (90.0f * DTR);
		
				// keep stick at reasonable values.
				pStick = max( -0.7f, pitchDesired );
				pStick = min( 0.7f, pStick );
			}


      		break;

		// #4 -- final attack approach hold for a sec and then head to next
      	case Final1:

			if (Sms->CurRippleCount() > 0) // JB 011013
				break;

			ClearFlag( GunFireFlag );
			diveOK = FALSE;

			FCC->releaseConsent = PilotInputs::Off;

			// mark that we've completed an AG pass
			madeAGPass = TRUE;

			trackX = self->XPos() + self->dmx[0][0] * 1000.0f;
			trackY = self->YPos() + self->dmx[0][1] * 1000.0f;
			trackZ = self->ZPos();

			// Terrain follow around 1000 ft
			if ( agApproach == AGA_LOW )
			{
			   trackZ = OTWDriver.GetGroundLevel( self->XPos() + self->XDelta(),
												  self->YPos() + self->YDelta() );
			   // if we're below track alt, kick us up a bit harder so we don't plow
			   // into steeper slopes
			   if ( self->ZPos() - trackZ > -1000.0f )
					trackZ = trackZ - 1000.0f - ( self->ZPos() - trackZ + 1000.0f ) * 2.0f;
			   else
					trackZ -= 1000.0f;
			   desSpeed = 650.0f;
			}
			else if ( agApproach == AGA_DIVE )
			{
			   desSpeed = 450.0f;
			   if ( self->ZPos() - curGroundAlt > -1000.0f )
			   {
			   	   trackZ = OTWDriver.GetGroundLevel( self->XPos() + self->XDelta(),
												  self->YPos() + self->YDelta() );
				   if ( self->ZPos() - trackZ > -500.0f )
						trackZ = trackZ - 500.0f - ( self->ZPos() - trackZ + 500.0f ) * 2.0f;
				   else
						trackZ -= 500.0f;
			   }
			   else
			   {
			   	   trackZ = OTWDriver.GetGroundLevel( trackX, trackY );
				   diveOK = TRUE;
				   if (Sms->curWeapon && !Sms->curWeapon->IsMissile() && Sms->curWeapon->IsBomb())
						trackZ -= 2000.0f;
				   else if (Sms->curWeapon)
						trackZ -= 50.0f;
			   }
			}
			else
			{
			   // trackZ = ipZ;
			   desSpeed = 450.0f;
			   if ( self->ZPos() - curGroundAlt > -500.0f )
			   {
			   	   trackZ = OTWDriver.GetGroundLevel( self->XPos() + self->XDelta(),
												  self->YPos() + self->YDelta() );
				   if ( self->ZPos() - trackZ > -500.0f )
						trackZ = trackZ - 500.0f - ( self->ZPos() - trackZ + 500.0f ) * 2.0f;
				   else
						trackZ -= 500.0f;
			   }
			}

			SimpleTrackSpeed( desSpeed * KNOTS_TO_FTPSEC );

		  	// dx = ( self->XPos() - trackX );
		  	// dy = ( self->YPos() - trackY );
		  	// approxRange = sqrt( dx*dx + dy*dy );

		  	dx = (float)fabs( self->XPos() - trackX );
		  	dy = (float)fabs( self->YPos() - trackY );
			approxRange = (float)sqrt( dx * dx + dy * dy );



			// pitch stick setting is based on our desired angle normalized to
			// 90 deg
			if ( agApproach == AGA_DIVE && diveOK )
			{
				pitchDesired = (float)atan2( self->ZPos() - trackZ, approxRange );
				pitchDesired /= (90.0f * DTR);
	
				// keep stick at reasonable values.
				pStick = max( -0.7f, pitchDesired );
				pStick = min( 0.7f, pStick );
			}

			if ( missionType == AMIS_BDA || missionType == AMIS_RECON)
			{
				// clear and get new target next pass
            madeAGPass = TRUE;
				onStation = NotThereYet;
			}
			else if ( (hasAGMissile || hasHARM) && !droppingBombs )
			{
				if ( SimLibElapsedTime > waitingForShot )
				{
					if ( agDoctrine == AGD_SHOOT_RUN && groundTargetPtr )
					{
						// clear and get new target next pass
						SetGroundTarget( NULL );

						// this takes us back to missile fire
						onStation = Final;
					}
					else // LOOK_SHOOT_LOOK
					{
						// this takes us back to 1st state
						SetGroundTarget( NULL );
						onStation = NotThereYet;
					}
				}
			}
			else if (droppingBombs == wcBombWpn)
			{
            DropBomb(approxRange, 0.0F, theRadar);
			}
			else if (droppingBombs == wcGbuWpn)
			{
            DropGBU(approxRange, 0.0F, theRadar);
			}
			else if (SimLibElapsedTime > waitingForShot || !hasWeapons)
   		{
				// this takes us back to 1st state
				SetGroundTarget( NULL );
				onStation = NotThereYet;
   		}

			TrackPoint(0.0F, desSpeed * KNOTS_TO_FTPSEC );

      	break;

		// after bombing run, we come here to pull up
		case Stabalizing:
		  	dx = (float)fabs( self->XPos() - trackX );
		  	dy = (float)fabs( self->YPos() - trackY );
			approxRange = (float)sqrt( dx * dx + dy * dy );

			if ( approxRange < 1000.0f )
				onStation = NotThereYet;

			SetGroundTarget( NULL );

			TrackPoint(0.0F, 650.0f * KNOTS_TO_FTPSEC);

			break;

      	case Taxi:
      		break;
   	}

      // Been doing this long enough, go to the next waypoint
      if (groundTargetPtr && SimLibElapsedTime > mergeTimer)
      {
// 2001-06-04 ADDED BY S.G. FORGET YOU WERE ON A GROUND PASS OR YOU'LL KEEP CIRCLING!
		 agDoctrine = AGD_NONE;
// END OF ADDED SECTION
         SelectNextWaypoint();
      }
}

/*
** Name: SetGroundTarget
** Description:
**		Creates a SimObjectType struct for the entity, sets the groundTargetPtr,
**		References the target.  Any pre-existing target is dereferenced.
*/
void DigitalBrain::SetGroundTarget( FalconEntity *obj )
{
	if ( obj != NULL )
	{
		if ( groundTargetPtr != NULL )
		{
			// release existing target data if different object
			if ( groundTargetPtr->BaseData() != obj )
			{
				groundTargetPtr->Release( SIM_OBJ_REF_ARGS );
				groundTargetPtr = NULL;
			}
			else
			{
				// already targeting this object
				return;
			}
		}

		// create new target data and reference it
		#ifdef DEBUG
		groundTargetPtr = new SimObjectType( OBJ_TAG, self, obj );
		#else
		groundTargetPtr = new SimObjectType( obj );
		#endif
		groundTargetPtr->Reference( SIM_OBJ_REF_ARGS );
		// SetTarget( groundTargetPtr );
	}
	else // obj is null
	{
		if ( groundTargetPtr != NULL )
		{
			groundTargetPtr->Release( SIM_OBJ_REF_ARGS );
			groundTargetPtr = NULL;
		}
	}
}

/*
** Name: SetGroundTargetPtr
** Description:
**	Sets GroundTargetPtr witch supplied SimObjectType
*/
void DigitalBrain::SetGroundTargetPtr( SimObjectType *obj )
{
	if ( obj != NULL )
	{
		if ( groundTargetPtr != NULL )
		{
			// release existing target data if different object
			if ( groundTargetPtr != obj )
			{
				groundTargetPtr->Release( SIM_OBJ_REF_ARGS );
				groundTargetPtr = NULL;
			}
			else
			{
				// already targeting this object
				return;
			}
		}

		// set and reference
		groundTargetPtr = obj;
		groundTargetPtr->Reference( SIM_OBJ_REF_ARGS );
		// SetTarget( groundTargetPtr );
	}
	else // obj is null
	{
		if ( groundTargetPtr != NULL )
		{
			groundTargetPtr->Release( SIM_OBJ_REF_ARGS );
			groundTargetPtr = NULL;
		}
	}
}

/*
** Name: SelectGroundTarget
** Description:
*/
void DigitalBrain::SelectGroundTarget( int targetFilter )
{
CampBaseClass* campTarg;
Tpoint pos;
VuGridIterator* gridIt = NULL;
UnitClass *campUnit = (UnitClass *)self->GetCampaignObject();
WayPointClass *dwp, *cwp;
int relation;

   	// No targeting when RTB
   	if (curMode == RTBMode)
   	{
   		SetGroundTarget( NULL );
      	return;
   	}

	// 1st let camp select target
   	SelectCampGroundTarget();


	// if we've got one we're done
	if ( groundTargetPtr )
		return;

	// if we're not on interdiction type mission, return....
	if ( missionType != AMIS_SAD && missionType != AMIS_INT && missionType != AMIS_BAI )
		return;

	if ( !campUnit )
		return;

	// divert waypoint overrides everything else
	dwp = ((FlightClass *)campUnit)->GetOverrideWP();
	if ( !dwp )
		cwp = self->curWaypoint;
	else
		cwp = dwp;

	if ( !cwp || cwp->GetWPAction() != WP_SAD )
		return;


	// choose how we are going to attack and whom....
	cwp->GetLocation ( &pos.x, &pos.y, &pos.z );

	#ifdef VU_GRID_TREE_Y_MAJOR
    gridIt = new VuGridIterator(RealUnitProxList, pos.y, pos.x, 5.0F * NM_TO_FT);
	#else
    gridIt = new VuGridIterator(RealUnitProxList, pos.x, pos.y, 5.0F * NM_TO_FT);
	#endif


	// get the 1st objective that contains the bomb
	campTarg = (CampBaseClass*)gridIt->GetFirst();
	while ( campTarg != NULL )
	{
        relation = TeamInfo[self->GetTeam()]->TStance(campTarg->GetTeam());
		if ( relation == Hostile || relation == War )
		{
			break;
		}
   		campTarg = (CampBaseClass*)gridIt->GetNext();
	}

	SetGroundTarget( campTarg );

    if (gridIt)
       delete gridIt;
	return;
}

/*
** Name: SelectGroundWeapon
** Description:
*/
void DigitalBrain::SelectGroundWeapon( void )
{
int i;
Falcon4EntityClassType* classPtr;
int runAway = FALSE;
SMSClass* Sms = self->Sms;

   hasAGMissile = FALSE;
   hasBomb = FALSE;
   hasHARM = FALSE;
   hasGun = FALSE;
   hasCamera = FALSE;
   hasRocket = FALSE;
   hasGBU = FALSE;

	// always make sure the FCC is in a weapons neutral mode when a
	// weapon selection is made.  Potentially we may be out of missiles
	// and have a SMS current bomb selected by this function and be in
	// FCC air-air mode which will cause a crash.
   self->FCC->SetMasterMode (FireControlComputer::Nav);
   self->FCC->preDesignate = TRUE;

   // Set no AG weapon, set true if found
   ClearATCFlag (HasAGWeapon);

	// look for a bomb and/or a missile
	for (i=0; i<self->Sms->NumHardpoints(); i++)
	{
      // Check for AG Missiles
      if (!hasAGMissile && Sms->hardPoint[i]->weaponPointer && Sms->hardPoint[i]->GetWeaponClass() == wcAgmWpn)
      {
         hasAGMissile = TRUE;
      }
      else if (!hasHARM && Sms->hardPoint[i]->weaponPointer && Sms->hardPoint[i]->GetWeaponClass() == wcHARMWpn)
      {
         hasHARM = TRUE;
      }
      else if (!hasBomb && Sms->hardPoint[i]->weaponPointer && Sms->hardPoint[i]->GetWeaponClass() == wcBombWpn)
      {
// 2001-07-11 ADDED BY S.G. SO DURANDAL AND CLUSTER ARE ACCOUNTED FOR DIFFERENTLY THAN NORMAL BOMBS
//		 hasBomb = TRUE;

		 hasBomb = TRUE;
		 if (Sms->hardPoint[i]->GetWeaponData()->cd >= 0.9f) // S.G. used edg kludge: drag coeff >= 1.0 is a durandal (w/chute) BUT 0.9 is hardcode for high drag :-(
			hasBomb = TRUE + 1;
		 else if (Sms->hardPoint[i]->GetWeaponData()->flags & SMSClass::HasBurstHeight) // S.G. If it has burst height, it's a cluster bomb
			hasBomb = TRUE + 2;
// END OF MODIFIED SECTION (except for the indent of the next line)
      }
      else if (!hasGBU && Sms->hardPoint[i]->weaponPointer && Sms->hardPoint[i]->GetWeaponClass() == wcGbuWpn)
      {
         hasGBU = TRUE;
      }
      else if (!hasRocket && Sms->hardPoint[i]->weaponPointer && Sms->hardPoint[i]->GetWeaponClass() == wcRocketWpn)
      {
         hasRocket = TRUE;
      }
      else if (!hasCamera && Sms->hardPoint[i]->weaponPointer && Sms->hardPoint[i]->GetWeaponClass() == wcCamera)
      {
         hasCamera = TRUE;
      }
   }


	// finally look for guns
	// only the A-10 and SU-25 are guns-capable for A-G
	classPtr = (Falcon4EntityClassType*)self->EntityType();
	if (classPtr->vuClassData.classInfo_[VU_STYPE] == STYPE_AIR_ATTACK &&
		(classPtr->vuClassData.classInfo_[VU_SPTYPE] == SPTYPE_A10 ||
		classPtr->vuClassData.classInfo_[VU_SPTYPE] == SPTYPE_SU25 ) )
	{
		if ( self->Guns &&
			 self->Guns->numRoundsRemaining )
		{
			hasGun = TRUE;
		}
	}

   if (hasAGMissile | hasBomb | hasHARM | hasRocket | hasGBU)
   {
      SetATCFlag(HasAGWeapon);
// 2001-05-27 ADDED BY S.G. IF WE HAVE HARMS AND OUR TARGET IS NOT EMITTING, CLEAR hasHARM ONLY IF WE HAVE SOMETHING ELSE ON BOARD
// 2001-06-20 MODIFIED BY S.G. EVEN IF ONLY HAVE HARMS, DO THIS. HOPEFULLY WING WILL REJOIN AND LEAD WILL TERMINATE IF ON SEAD STRIKES
//	  if (hasHARM && groundTargetPtr && (hasAGMissile | hasBomb | hasRocket | hasGBU)) {
	  if (hasHARM && groundTargetPtr) {
		  // If it's a sim entity, look at its radar type)
		  if (groundTargetPtr->BaseData()->IsSim()) {
// 2001-06-20 MODIFIED BY S.G. IT DOESN'T MATTER AT THIS POINT IF IT'S EMITTING OR NOT. ALL THAT MATTERS IS - IS IT A RADAR VEHICLE/FEATURE FOR A SIM OR DOES IT HAVE A RADAR VEHICLE IF A CAMPAIGN ENTITY
			  // No need to check if they exists because if they got selected, it's because they exists. Only check if they have a radar
			  if (!((groundTargetPtr->BaseData()->IsVehicle() && ((SimVehicleClass *)groundTargetPtr->BaseData())->GetRadarType() != RDR_NO_RADAR) ||	// It's a vehicle and it has a radar
				  (groundTargetPtr->BaseData()->IsStatic()  &&  ((SimStaticClass *)groundTargetPtr->BaseData())->GetRadarType() != RDR_NO_RADAR)))	// It's a feature and it has a radar
				  hasHARM = FALSE;
		  }
		  // NOPE - It's a campaign object, if it's aggregated, can't use HARM unless no one has chosen it yet.
		  else if (((CampBaseClass *)groundTargetPtr->BaseData())->IsAggregate()) {
			  // 2002-01-20 ADDED BY S.G. Unless it's an AAA since it has more than one radar.
			  if (!groundTargetPtr->BaseData()->IsBattalion() || ((BattalionClass *)groundTargetPtr->BaseData())->class_data->RadarVehicle < 16) {
			  // END OF ADDED SECTION 2002-01-20
				 if (((FlightClass *)self->GetCampaignObject())->shotAt != groundTargetPtr->BaseData()) {
				    ((FlightClass *)self->GetCampaignObject())->shotAt = groundTargetPtr->BaseData();
				    ((FlightClass *)self->GetCampaignObject())->whoShot = self;
				 }
				 else if (((FlightClass *)self->GetCampaignObject())->whoShot != self)
				     hasHARM = FALSE;
			  }
		  }
	  }
// END OF ADDED SECTION
   }

   // 2001-06-30 ADDED BY S.G. SO THE TRUE WEAPON STATE IS KEPT...
   if (hasAGMissile | hasBomb | hasHARM | hasRocket | hasGBU)
      SetATCFlag(HasCanUseAGWeapon);
   else
      ClearATCFlag(HasCanUseAGWeapon);
// END OF ADDED SECTION

   hasWeapons = hasAGMissile | hasBomb | hasHARM | hasRocket | hasGun | hasGBU;

// 2001-06-20 ADDED BY S.G. LEAD WILL TAKE ITS WING WEAPON LOADOUT INTO CONSIDERATION BEFORE ABORTING
// 2001-06-30 MODIFIED BY S.G. IF NOT A ENROUTE SEAD TARGET, SKIP HARMS AS AVAILABLE IF IT CAN'T BE FIRED
   int ret;
   if (!hasWeapons && !isWing && ((ret = IsNotMainTargetSEAD()) || sentWingAGAttack != AG_ORDER_ATTACK)) {
		int i;
		int usComponents = self->GetCampaignObject()->NumberOfComponents();
		for (i = 0; i < usComponents; i++) {
			AircraftClass *flightMember = (AircraftClass *)self->GetCampaignObject()->GetComponentEntity(i);
			if (flightMember && flightMember->DBrain() && flightMember->DBrain()->IsSetATC(ret ? HasAGWeapon : HasCanUseAGWeapon)) {
				hasWeapons = TRUE;
				SetATCFlag(HasAGWeapon);
				break;
			}
		}
   }
// END OF ADDED SECTION
	// make sure, if we're guns or rockets only, that are approach is
	// a dive
	if ( !hasBomb && !hasGBU && !hasAGMissile && !hasHARM && (hasGun || hasRocket))
	{
		agApproach = AGA_DIVE;
		ipZ = -7000.0f;
	}

   // Check for run-away case
   if (missionType == AMIS_BDA || missionType == AMIS_RECON)
   {
      if (!hasCamera)
      {
         runAway = TRUE;
      }
   }
// 2001-06-20 MODIFIED BY S.G. SO AI DO NOT RUN AWAY IF YOU STILL HAVE HARMS AND ON A SEAD TYPE MISSION
// else if (!hasWeapons)
   else if (!hasWeapons && !(IsSetATC(HasAGWeapon) && IsNotMainTargetSEAD()))
   {
      runAway = TRUE;
   }

   // 2002-03-08 ADDED BY S.G. Don't run away if designating...
   if ((moreFlags & KeepLasing) && runAway == TRUE)
	   runAway = FALSE;
   // END OF ADDED SECTION 2002-03-08

   if (runAway && missionClass == AGMission)// Nothing to attack with or Recon/BDA mission w/o camera
   {
		// no AG weapons, next waypoint....
		agDoctrine = AGD_NONE;
// 2001-08-04 MODIFIED BY S.G. SET missionComplete ONLY ONCE WE TEST IT (ADDED THAT TEST FOR THE IF AS WELL)
//		missionComplete = TRUE;
      self->FCC->SetMasterMode (FireControlComputer::Nav);
      self->FCC->preDesignate = TRUE;
		SetGroundTarget( NULL );
		if ( /*S.G.*/!missionComplete && /*S.G.*/agImprovise == FALSE && !self->OnGround())
		{
			// JB 020315 Only skip to the waypoint after the target waypoint. Otherwise we may go into landing mode too early.
			 WayPointClass* tmpWaypoint = self->curWaypoint;
			 while (tmpWaypoint)
			 {
				 if (tmpWaypoint->GetWPFlags() & WPF_TARGET)
				 {
					 tmpWaypoint = tmpWaypoint->GetNextWP();
					 break;
				 }
				 tmpWaypoint = tmpWaypoint->GetNextWP();
			 }

			if (tmpWaypoint)
				SelectNextWaypoint();
		}

		missionComplete = TRUE; /*S.G.*/
		// if we're a wingie, rejoin the lead
		if ( isWing )
		{
// 2001-05-03 ADDED BY S.G. WE WANT WEDGE AFTER GROUND PASS!
			mFormation = FalconWingmanMsg::WMWedge;
// END OF ADDED SECTION
			AiRejoin( NULL );
			// make sure wing's designated target is NULL'd out
			mDesignatedObject = FalconNullId;
		}
	}
}

/*
** Name: SelectCampGroundTarget
** Description:
**		Uses the campaign functions to set ground targets
*/
void DigitalBrain::SelectCampGroundTarget( void )
{
	UnitClass *campUnit = (UnitClass *)self->GetCampaignObject();
	FalconEntity *target = NULL;
	// int i, numComponents;
	SimBaseClass *simTarg;
	WayPointClass *dwp;

// 2001-07-15 REMOVED BY S.G. THIS IS CLEARED IN THE 'Final' STAGE ONLY
//	madeAGPass = FALSE;

	agImprovise = FALSE;

	// sanity check
	if ( !campUnit )
		return;

	// divert waypoint overrides everything else
	dwp = ((FlightClass *)campUnit)->GetOverrideWP();

	// check to see if our current ground target is a sim and exploding or
	// dead, if so let's get a new target from the campaign
	if ( groundTargetPtr &&
		 groundTargetPtr->BaseData()->IsSim() &&
		 ( groundTargetPtr->BaseData()->IsExploding() ||
		 !((SimBaseClass *)groundTargetPtr->BaseData())->IsAwake() ) )
	{
		SetGroundTarget( NULL );
	}

	// see if we've already got a target
	if ( groundTargetPtr )
	{
		target = groundTargetPtr->BaseData();

		// is it a campaign object? if not we can return....
		if (target->IsSim() )
		{
			return;
		}

		// itsa campaign object.  Check to see if its deagg'd
		if (((CampBaseClass*)target)->IsAggregate() )
		{
			// still aggregated, return
			return;
		}

		// the campaign object is now deaggregated, choose a sim entity
		// to target on it
// 2001-04-11 MODIFIED BY S.G. SO LEAD USES THE ASSIGNED TARGET IF IT'S AN OBJECTIVE AND MAKES A BETTER SELECTION ON MOVERS
/*		numComponents = ((CampBaseClass*)target)->NumberOfComponents();

		for ( i = 0; i < numComponents; i++ )
		{
			simTarg = ((CampBaseClass*)target)->GetComponentEntity( rand() % numComponents );
			if ( !simTarg ) //sanity check
				continue;

			// don't target runways (yet)
			if ( // !simTarg->IsSetCampaignFlag (FEAT_FLAT_CONTAINER) &&
				!simTarg->IsExploding() &&
				!simTarg->IsDead() &&
				simTarg->pctStrength > 0.0f )
			{
				SetGroundTarget( simTarg );
				break;
			}
		} // end for # components
*/
		int targetNum = 0;

		// First, the lead will go for the assigned target, if any...
		if (!isWing && target->IsObjective()) {
			FalconEntity *wpTarget = NULL;		
			WayPointClass *twp = self->curWaypoint;

			// First prioritize the divert waypoint target
			if (dwp)
				wpTarget = dwp->GetWPTarget();

			// If wpTarget is not NULL, our waypoint will be the divert waypoint
			if (wpTarget)
				twp = dwp;
			else {
				// Our target will be the current waypoint target if any
				if (self->curWaypoint)
					wpTarget = self->curWaypoint->GetWPTarget();
			}

			// If we have a waypoint target and it is our current target
			if (wpTarget && wpTarget == target)
				// Our feature is the one assigned to us by the mission planner
				targetNum = twp->GetWPTargetBuilding();
		}

		// Use our helper function
		simTarg = FindSimGroundTarget((CampBaseClass*)target, ((CampBaseClass*)target)->NumberOfComponents(), targetNum);

		// Hopefully, we have one...
		SetGroundTarget(simTarg);
// END OF ADDED SECTION

		return;

	} // end if already targetPtr

	// priority goes to the waypoint target
	if ( dwp )
	{
		target = dwp->GetWPTarget();
		if ( !target )
		{
			dwp = NULL;
			if ( self->curWaypoint )
			{
				target = self->curWaypoint->GetWPTarget();
			}
		}
	}
	else if ( self->curWaypoint )
	{
		target = self->curWaypoint->GetWPTarget();
	}

	if ( target && target->OnGround() )
	{
		SetGroundTarget( target );
		return;
	}


	// at this point we have no target, we're going to ask the campaign
	// to find out what we're supposed to hit

	// tell unit we haven't done any checking on it yet
	campUnit->UnsetChecked();

	// choose target.  I assume if this returns 0, no target....
// 2001-06-09 MODIFIED BY S.G. NEED TO SEE IF WE ARE CHANGING CAMPAIGN TARGET AND ON A SEAD ESCORT MISSION. IF SO, DEAL WITH IT
/*	if ( !campUnit->ChooseTarget() )
	{
		// alternately try and choose the waypoint's target
		// SetGroundTarget( self->curWaypoint->GetWPTarget() );
		return;
	}

	// get the target
	target = campUnit->GetTarget();
*/
	// Choose and get this target
	int ret;
	ret = campUnit->ChooseTarget();
	target = campUnit->GetTarget();

	// If ChooseTarget returned FALSE or we changed campaign target and we're the lead  (but we must had a previous campaign target first)
	if (!isWing && lastGndCampTarget && (!ret || target != lastGndCampTarget)) {
		agDoctrine = AGD_NONE;														// Need to setup our next ground attack
		onStation = NotThereYet;													// Need to do a new pass next time
		sentWingAGAttack = AG_ORDER_NONE;											// Next time, direct wingmen on target
		lastGndCampTarget = NULL;													// No previous campaign target
		AiSendCommand (self, FalconWingmanMsg::WMWedge, AiFlight, FalconNullId);	// Ask wingmen to resume a wedge formation
		AiSendCommand (self, FalconWingmanMsg::WMRejoin, AiFlight, FalconNullId);	// Ask wingmen to rejoin
		AiSendCommand (self, FalconWingmanMsg::WMCoverMode, AiFlight, FalconNullId);// And stop what they were doing
	}
	else
		// Keep track of this campaign target
		lastGndCampTarget = (CampBaseClass *)target;

	if (!ret)
		return;

// END OF MODIFIED SECTION

	// get tactic -- not doing anything with it now
	campUnit->ChooseTactic();
	campTactic = campUnit->GetUnitTactic();

	// sanity check and make sure its on ground, what to do if not?!...
	if ( !target ||
		 !target->OnGround() ||
		 (campTactic != ATACTIC_ENGAGE_STRIKE &&
		  campTactic != ATACTIC_ENGAGE_SURFACE &&
		  campTactic != ATACTIC_ENGAGE_DEF &&
		  campTactic != ATACTIC_ENGAGE_NAVAL ) )
		return;


	// set it as our target
	SetGroundTarget( target );

}

/*
** Name: SetupAGMode
** Description:
**		Prior to entering an air to ground attack mode, we do some
**		setup to determine what our attack doctrine is going to be and
**		how we will approach the target.
**		When agDoctrine is set to other than AGD_NONE, this signals that
**		Setup has been completed for the AG attack.
**		SEAD and CASCP waypoints are special cases in that targets aren't
**		necessarily tied to the waypoint but the entire flight route -- we
**		check for these targets periodically.
**
**		The 1st arg is current waypoint (should be an IP)
**		2nd arg is next WP (presumably where action is)
*/
// 2001-07-11 REDONE BY S.G.
#if 1
// This routine is called until an attack profile can be established, then it is no longer call unless agDoctrine is reset to AGD_NONE
void DigitalBrain::SetupAGMode( WayPointClass *cwp, WayPointClass *wp )
{
	int wpAction;	
	UnitClass *campUnit = (UnitClass *)self->GetCampaignObject();
	CampBaseClass *campBaseTarg;
	float dx, dy, dz, range;
	Falcon4EntityClassType* classPtr;
	WayPointClass *dwp;

	// So we don't think our mission is complete and forget to go to CrossWind from 'NotThereYet'
    missionComplete = FALSE;
	agImprovise = FALSE;

	// First, lets get a target if we're the lead, otherwise use the target provided by the lead...
	
	if (!isWing) {
		dwp = NULL;
		if (campUnit)
			dwp = ((FlightClass *)campUnit)->GetOverrideWP();

		// If we were passed a target wayp
		if (wp) {
			// First have the lead fly toward the IP waypoint until he can see a target
			if (cwp != wp) {
				cwp->GetLocation(&ipX, &ipY, &ipZ);

				// Next waypoint is our target waypoint
				SelectNextWaypoint();

				// If we have HARM on board (even if we can't use them), start your attack from here
				if (hasHARM) {
					ipX = self->XPos();
					ipY = self->YPos();
				}
			}
			else {
				if (dwp)
					dwp->GetLocation(&ipX, &ipY, &ipZ);
				else
					wp->GetLocation(&ipX, &ipY, &ipZ);
				
			}

			wpAction = wp->GetWPAction();
			// If we have nothing, look at our enroute action
			if (wpAction == WP_NOTHING)
				wpAction = wp->GetWPRouteAction();

			// If it's a SEAD or CASCP waypoint, do the following...
			if ((wpAction == WP_SEAD || wpAction == WP_CASCP) && cwp == wp) {
				// But only if it is time to retarget, otherwise stay quiet
				if (SimLibElapsedTime > nextAGTargetTime) {
					// Next retarget in 5 seconds
					nextAGTargetTime = SimLibElapsedTime + 5000;

					// First, lets release our current target and target history
					SetGroundTarget( NULL );
					gndTargetHistory[0] = NULL;

					// The first call should get a campaign entity while the second one will fetch a sim entity within
					SelectGroundTarget( TARGET_ANYTHING );
					if (groundTargetPtr == NULL)
						return;
					if (groundTargetPtr->BaseData()->IsCampaign() && !((CampBaseClass *)groundTargetPtr->BaseData())->IsAggregate())
						SelectGroundTarget( TARGET_ANYTHING );

				}
				else
					return;
			}
			else {
				// divert waypoint overrides everything else
				if (dwp) {
					campBaseTarg = dwp->GetWPTarget();
					if (!campBaseTarg) {
						dwp = NULL;
						campBaseTarg = wp->GetWPTarget();
					}
				}
				else
					campBaseTarg = wp->GetWPTarget();

				// See if we got a target waypoint target, if not, try and see if we can select one by using the campaign target selector
				if (campBaseTarg == NULL) {
					if (SimLibElapsedTime > nextAGTargetTime) {
						// Next retarget in 5 seconds
						nextAGTargetTime = SimLibElapsedTime + 5000;

						// First, lets release our current target and target history
						SetGroundTarget( NULL );
						gndTargetHistory[0] = NULL;

						// The first call should get a campaign entity while the second one will fetch a sim entity within
						SelectGroundTarget( TARGET_ANYTHING );
						if (groundTargetPtr == NULL)
							return;
						if (groundTargetPtr->BaseData()->IsCampaign() && !((CampBaseClass *)groundTargetPtr->BaseData())->IsAggregate())
							SelectGroundTarget( TARGET_ANYTHING );
					}
					else
						return;
				}
				// set ground target to camp base if ground target is NULL at this point
				else if ( groundTargetPtr == NULL )
				{
					SetGroundTarget( campBaseTarg );

					if (groundTargetPtr == NULL)
						return;
// 2001-10-26 ADDED by M.N. If player changed mission type in TE planner, and below the target WP
// we find a package object, it will become a ground target. If a package is engaged, CTD.
					if (groundTargetPtr->BaseData()->IsPackage())
					{
						SetGroundTarget( NULL );
						gndTargetHistory[0] = NULL;
						SelectGroundTarget( TARGET_ANYTHING );	// choose something else
					}
// END of added section

					// Then get a sim entity from it
					if (groundTargetPtr->BaseData()->IsCampaign() && !((CampBaseClass *)groundTargetPtr->BaseData())->IsAggregate())
						SelectGroundTarget( TARGET_ANYTHING );
				}
			}
		}
		// It's not from a waypoint action (could be a target of opportunity, even from A2A mission if it has A2G weapons as well)
		else  if (SimLibElapsedTime > nextAGTargetTime) {
			// Next retarget in 5 seconds
			nextAGTargetTime = SimLibElapsedTime + 5000;

			// First, lets release our current target and target history
			SetGroundTarget( NULL );
			gndTargetHistory[0] = NULL;

			// The first call should get a campaign entity while the second one will fetch a sim entity within
			SelectGroundTarget( TARGET_ANYTHING );
			if (groundTargetPtr == NULL)
				return;
			SelectGroundTarget( TARGET_ANYTHING );

			// Don't ask me, that's how they had it in the orininal code
			agImprovise = TRUE;
		}
		// Not the time to retarget, so get out
		else
			return;
	}
	// Don't ask me, that's how they had it in the orininal code
	else
		agImprovise = TRUE;

	// No ground target? do nothing
	if (!groundTargetPtr)
		return;

	// After all this, make sure we have a sim target if we can
	if (groundTargetPtr->BaseData()->IsCampaign() && !((CampBaseClass *)groundTargetPtr->BaseData())->IsAggregate())
		SelectGroundTarget( TARGET_ANYTHING );

	// do we have any ground weapons?
	SelectGroundWeapon();
	if (!IsSetATC(HasAGWeapon)) {
		// Nope, can't really attack can't we? so bail out
		SetGroundTarget(NULL);
		agDoctrine = AGD_NONE;
		return;
	}

	// Better be safe than sory...
	if (groundTargetPtr == NULL) {
		// Nope, somehow we lost our target so bail out...
		agDoctrine = AGD_NONE;
		return;
	}

	// Tell the AI it hasn't done a ground pass yet so it can redo its attack profile
	madeAGPass = FALSE;

	// set doctrine and approach to default value, calc an insertion point loc
	agDoctrine = AGD_LOOK_SHOOT_LOOK;
	if (missionType == AMIS_SEADESCORT || missionType == AMIS_SEADSTRIKE) {
		agApproach = AGA_LOW;
		ipZ = 0.0f;
	}
	else {
		agApproach = AGA_DIVE;
		ipZ = -7000.0f;
	}

	dx = groundTargetPtr->BaseData()->XPos() - self->XPos();
	dy = groundTargetPtr->BaseData()->YPos() - self->YPos();
	dz = groundTargetPtr->BaseData()->ZPos() - self->ZPos();

	// x-y get range
	range = (float)sqrt( dx * dx + dy * dy ) + 0.1f;

	// normalize the x and y vector
	dx /= range;
	dy /= range;

	// see if we're too close in and set ipX/ipY accordingly
	if ( range < 5.0f * NM_TO_FT )
	{
		// too close, get IP at a perpendicular point to current loc
		ipX = groundTargetPtr->BaseData()->XPos() + dy * 7.0f * NM_TO_FT;
		ipY = groundTargetPtr->BaseData()->YPos() - dx * 7.0f * NM_TO_FT;
		ShiAssert (ipX > 0.0F);
	}
	else
	{
		// get point between us and target
		ipX = groundTargetPtr->BaseData()->XPos() - dx * 4.0f * NM_TO_FT;
		ipY = groundTargetPtr->BaseData()->YPos() - dy * 4.0f * NM_TO_FT;
		ShiAssert (ipX > 0.0F);
	}

	// Depending on the type of plane, adjust our attack profile
	classPtr = (Falcon4EntityClassType*)self->EntityType();
	if (classPtr->vuClassData.classInfo_[VU_STYPE] == STYPE_AIR_BOMBER )
	{
		agApproach = AGA_BOMBER;
		agDoctrine = AGD_SHOOT_RUN;
		ipZ = self->ZPos();
	}
	else
	{
		// Order of fire is: HARMs, AGMissiles, GBUs, bombs, rockets then gun so do it similarely
		if (hasHARM) {
			agApproach = AGA_LOW;
            agDoctrine = AGD_LOOK_SHOOT_LOOK;
			ipX = self->XPos();
			ipY = self->YPos();
			ipZ = 0.0f;
		}
		else if (hasAGMissile) {
			agApproach = AGA_HIGH;
			agDoctrine = AGD_LOOK_SHOOT_LOOK;

			// Wings shoots Mavericks as soon as asked.
			ipX = self->XPos();
			ipY = self->YPos();
			ipZ = -4000.0f;
		}
		else if (hasGBU) {
			agApproach = AGA_HIGH;
			agDoctrine = AGD_SHOOT_RUN;
			ipZ = -13000.0f;
		}
		else if (hasBomb) {
			if (hasBomb == TRUE + 1) {		// It's a durandal
				agApproach = AGA_HIGH;		// Because if 'low', he will 'pop up' on final...
				ipZ = -250.0f;
			}
			else {
				if (hasBomb == TRUE + 2) {	// It's a cluster bomb
					agApproach = AGA_HIGH;
					ipZ = -5000.0f;
				}
				else {						// It's any other type of bombs
					agApproach = AGA_HIGH;
					ipZ = -11000.0f;
				}

				// Now check if we are within the plane's min/max altitude and snap if not...
				VehicleClassDataType *vc = GetVehicleClassData(self->Type() - VU_LAST_ENTITY_TYPE);
				ipZ = max(vc->HighAlt * -100.0f, ipZ);
				ipZ = min(vc->LowAlt * -100.0f, ipZ);
			}
			agDoctrine = AGD_SHOOT_RUN;
		}
		else if (hasGun || hasRocket) {
			agDoctrine = AGD_LOOK_SHOOT_LOOK;
			agApproach = AGA_DIVE;
			ipZ = -7000.0f;
		}

		// For these, it's always a LOOK SHOOT LOOK
		if ( missionType == AMIS_SAD || missionType == AMIS_INT || missionType == AMIS_BAI || hasAGMissile || hasHARM || IsNotMainTargetSEAD())
			agDoctrine = AGD_LOOK_SHOOT_LOOK;

		// Just in case it was changed by a weapon type earlier
		if (missionType == AMIS_SEADESCORT || missionType == AMIS_SEADSTRIKE) {
			agApproach = AGA_LOW;
			ipZ = 0.0f;
		}

		// Then if we have a camera, 
		if ((missionType == AMIS_BDA || missionType == AMIS_RECON) && hasCamera) {
			ipZ = -7000.0f;
			agDoctrine = AGD_SHOOT_RUN;
			agApproach = AGA_DIVE;
		}
	}

	// Time this attack, and don't stick around too long
	mergeTimer = SimLibElapsedTime + g_nGroundAttackTime * 60 * SEC_TO_MSEC;
}
#else
void DigitalBrain::SetupAGMode( WayPointClass *cwp, WayPointClass *wp )
{
	int wpAction;	
	UnitClass *campUnit = (UnitClass *)self->GetCampaignObject();
	CampBaseClass *campBaseTarg;
	float dx, dy, dz, range;
	int detectRange;
	Falcon4EntityClassType* classPtr;
	UnitClassDataType *uc;
	ObjClassDataType *oc;
	WayPointClass *dwp;
   float saveZ;

	// divert waypoint overrides everything else
	dwp = NULL;
	if ( campUnit )
	{
		dwp = ((FlightClass *)campUnit)->GetOverrideWP();
	}

	// this case occurs if we're not on an air-ground mission and
	// we're told to attack a ground target
	if ( cwp == NULL || wp == NULL )
	{
		// if we don't have a ground target already we do nothing
		//if ( !groundTargetPtr )
		if ( !groundTargetPtr || F4IsBadReadPtr(groundTargetPtr, sizeof(SimObjectType)) || F4IsBadReadPtr(groundTargetPtr->BaseData(), sizeof(FalconEntity))) // JB 010326 CTD
			return;

		// do we have any ground weapons?
		SelectGroundWeapon();
		if ( !hasWeapons )
		{
			SetGroundTarget( NULL );
         agDoctrine = AGD_NONE;
			return;
		}

	   madeAGPass = FALSE;
	   agImprovise = FALSE;

		// set doctrine and approach, calc an insertion point loc
		agDoctrine = AGD_LOOK_SHOOT_LOOK;
		agApproach = AGA_DIVE;
		dx = groundTargetPtr->BaseData()->XPos() - self->XPos();
		dy = groundTargetPtr->BaseData()->YPos() - self->YPos();
		dz = groundTargetPtr->BaseData()->ZPos() - self->ZPos();

		// x-y get range
		range = (float)sqrt( dx * dx + dy * dy ) + 0.1f;

		// normalize the x and y vector
		dx /= range;
		dy /= range;


		// see if we're too close in
		if ( range < 5.0f * NM_TO_FT )
		{
			// too close, get IP at a perpendicular point to current loc
			ipX = groundTargetPtr->BaseData()->XPos() + dy * 7.0f * NM_TO_FT;
			ipY = groundTargetPtr->BaseData()->YPos() - dx * 7.0f * NM_TO_FT;
			ShiAssert (ipX > 0.0F);
		}
		else
		{
			// get point between us and target
			ipX = groundTargetPtr->BaseData()->XPos() - dx * 4.0f * NM_TO_FT;
			ipY = groundTargetPtr->BaseData()->YPos() - dy * 4.0f * NM_TO_FT;
			ShiAssert (ipX > 0.0F);
		}

    	classPtr = (Falcon4EntityClassType*)self->EntityType();
    	if (classPtr->vuClassData.classInfo_[VU_STYPE] == STYPE_AIR_BOMBER )
		{
			agApproach = AGA_BOMBER;
			agDoctrine = AGD_SHOOT_RUN;
			ipZ = self->ZPos();
		}
		else
		{
			ipZ = -7000.0f;
		}

      if (agApproach != AGA_BOMBER)
      {
         if (hasHARM)
         {
		      agApproach = AGA_HIGH;
            agDoctrine = AGD_LOOK_SHOOT_LOOK;
				ipX = self->XPos();
				ipY = self->YPos();
         }
         else if (hasAGMissile)
         {
		      agApproach = AGA_LOW;
		      agDoctrine = AGD_SHOOT_RUN;
				ipX = self->XPos();
				ipY = self->YPos();
         }
         else if (hasGBU || hasBomb)
         {
		      agApproach = AGA_HIGH;
		      agDoctrine = AGD_SHOOT_RUN;
         }
      }

		agImprovise = TRUE;

      // Time this attack, and don't stick around too long
      mergeTimer = SimLibElapsedTime + 600 * SEC_TO_MSEC;

		return;
	}

   // Time this attack, and don't stick around too long
	madeAGPass = FALSE;
	agImprovise = FALSE;
   mergeTimer = SimLibElapsedTime + 600 * SEC_TO_MSEC;

	wpAction = wp->GetWPAction();

	// zero out any potentially preexisting target ( should be NULL already)
	SetGroundTarget( NULL );

	// special case SEAD and CAS
	if ( ( wpAction == WP_SEAD || wpAction == WP_CASCP ) &&
		  SimLibElapsedTime > nextAGTargetTime &&
		  cwp == wp )
	{
		// set time of when to check next
		nextAGTargetTime = SimLibElapsedTime + 10000;

		// see if we've got a target
		SelectGroundTarget( TARGET_ANYTHING );
		if ( groundTargetPtr )
		{
			// got target, select doctrine, approach and IP location

			// NOTE: at the moment we're going to make all our
			// approaches LOW and doctrine is LOOK_SHOOT_LOOK
			agDoctrine = AGD_LOOK_SHOOT_LOOK;
			agApproach = AGA_LOW;

			dx = groundTargetPtr->BaseData()->XPos() - self->XPos();
			dy = groundTargetPtr->BaseData()->YPos() - self->YPos();
			dz = groundTargetPtr->BaseData()->ZPos() - self->ZPos();

			// x-y get range
			range = (float)sqrt( dx * dx + dy * dy ) + 0.1f;

			// normalize the x and y vector
			dx /= range;
			dy /= range;

			// see if we're too close in
			if ( range < 5.0f * NM_TO_FT )
			{
				// too close, get IP at a perpendicular point to current loc
				ipX = groundTargetPtr->BaseData()->XPos() + dy * 7.0f * NM_TO_FT;
				ipY = groundTargetPtr->BaseData()->YPos() - dx * 7.0f * NM_TO_FT;
				ShiAssert (ipX > 0.0F);
			}
			else
			{
				// get point between us and target
				ipX = groundTargetPtr->BaseData()->XPos() - dx * 4.0f * NM_TO_FT;
				ipY = groundTargetPtr->BaseData()->YPos() - dy * 4.0f * NM_TO_FT;
				ShiAssert (ipX > 0.0F);
			}

			// low alt use terrain following
			ipZ = 0.0f;

			/*
			if ( cwp != wp )
			{
				SelectNextWaypoint();
			}
			*/
		}
      else
      {
         agDoctrine = AGD_NONE;
      }

		return;
	}

	// not in SEAD or CAS....

	// see if we can get a target from the waypoint
	if ( dwp )
	{
		campBaseTarg = dwp->GetWPTarget();
		if ( !campBaseTarg )
		{
			dwp = NULL;
			campBaseTarg = wp->GetWPTarget();
		}
	}
	else
	{
		campBaseTarg = wp->GetWPTarget();
	}

	// if not, try and see if we can select one by our parent camp object
	if ( campBaseTarg == NULL )
	{
		if (  SimLibElapsedTime > nextAGTargetTime  )
		{
			// set time of when to check next
			nextAGTargetTime = SimLibElapsedTime + 10000;
			SelectGroundTarget( TARGET_ANYTHING );

			if ( groundTargetPtr )
			{
				campBaseTarg = (CampBaseClass *)groundTargetPtr->BaseData();
			}
			else
			{
				// we can't yet determine our attack profile
				return;
			}
		}
		else
		{
			return;
		}
	}
	// set ground target to camp base if ground target is NULL at this point
	else if ( groundTargetPtr == NULL )
	{
		SetGroundTarget( campBaseTarg );
	}

	// we have a target, choose doctrine, approach
	agDoctrine = AGD_SHOOT_RUN;

	// check our current vs action waypoint, if different, We'll use
	// current to get our IP loc and the bump up to the action waypoint
	// if same choose an IP based on our current range to the WP
	if ( cwp != wp )
	{
		cwp->GetLocation (
				&ipX,
				&ipY,
				&ipZ );
		SelectNextWaypoint();
		if (hasHARM)
		{
			ipX = self->XPos();
			ipY = self->YPos();
		}
	}
	else
	{
		if ( dwp )
		{
			dwp->GetLocation (
				&ipX,
				&ipY,
				&ipZ );
		}
		else
		{
			wp->GetLocation (
				&ipX,
				&ipY,
				&ipZ );
		}
		
		// override waypoint loc if we've got a target
		if ( groundTargetPtr )
		{
			ipX = groundTargetPtr->BaseData()->XPos();
			ipY = groundTargetPtr->BaseData()->YPos();
			ShiAssert (ipX > 0.0F);
		}

		dx = ipX - self->XPos();
		dy = ipY - self->YPos();

		// x-y get range
		range = (float)sqrt( dx * dx + dy * dy ) + 0.1f;

		// normalize the x and y vector
		dx /= range;
		dy /= range;

		// see if we're too close in
		if ( range < 5.0f * NM_TO_FT )
		{
			// too close, get IP at a perpendicular point to current loc
			ipX += dy * 7.0f * NM_TO_FT;
			ipY -= dx * 7.0f * NM_TO_FT;
			ShiAssert (ipX > 0.0F);
		}
		else
		{
			// get point between us and target
			ipX -= dx * 4.0f * NM_TO_FT;
			ipY -= dy * 4.0f * NM_TO_FT;
			ShiAssert (ipX > 0.0F);
		}
	}

	// edg: I'm not sure what the best way to determine approach is, but
	// I'm going to try to use detection range of the ground target.  If
	// the detection range is nonzero use HIGH or DIVE, otherwise
	// use LOW.  Also, if we're a bomber always use HIGH

	// get unit class data
	if ( campBaseTarg->IsUnit() )
	{
		uc = ((UnitClass *)campBaseTarg)->GetUnitClassData();
		detectRange = uc->Detection[ campUnit->GetMovementType() ];
	}
	else
	{
		oc = ((ObjectiveClass *)campBaseTarg)->GetObjectiveClassData();
		detectRange = oc->Detection[ campUnit->GetMovementType() ];	
	}

	float groundZ = OTWDriver.GetGroundLevel( ipX, ipY );

   saveZ = ipZ;
   classPtr = (Falcon4EntityClassType*)self->EntityType();
   if (classPtr->vuClassData.classInfo_[VU_STYPE] == STYPE_AIR_BOMBER )
	{
		agApproach = AGA_BOMBER;
		ipZ = min( ipZ, groundZ-2000.0f );
	}
	else if ( detectRange == 0 )
	{
		// no air radar
		agApproach = AGA_DIVE;
		ipZ = min( ipZ, groundZ-2000.0f );
	}
	else
	{
		// has radar
		agApproach = AGA_LOW;
		ipZ = 0.0f;
	}

   // Pick the weapon of choice
	SelectGroundWeapon();

   // Don't do dive attack if using LGB's, Stay low
   if (agApproach == AGA_DIVE && (hasGBU || hasBomb) && !(hasAGMissile || hasHARM))
   {
// 2001-05-27 MODIFIED BY S.G. hasHARM CAN *NEVER* BE TRUE HERE BECAUSE OF THE TEST ABOVE. THEY PROBABLY MEANS hasBomb SINCE THEY WANT LGB TO STAY LOW...
//    if (hasHARM)
      if (hasBomb)
      {
         agApproach = AGA_HIGH;
         agDoctrine = AGD_LOOK_SHOOT_LOOK;
      }
      else
         agApproach = AGA_LOW;
      ipZ = saveZ;
   }

   // Don't do low w/ HARMS
   if (agApproach == AGA_LOW && hasHARM)
   {
      agDoctrine = AGD_LOOK_SHOOT_LOOK;
      agApproach = AGA_HIGH;
      ipZ = saveZ;
   }

	// finally, check for guns/rocket-only mode
	if ( !hasAGMissile && !hasHARM && !hasBomb && !hasGBU && (hasGun || hasRocket) )
	{
		// no air radar
		agDoctrine = AGD_LOOK_SHOOT_LOOK;
		agApproach = AGA_DIVE;
		ipZ = -7000.0f;
	}

	// if we're on interdiction type mission LOOK SHOOT LOOK
	if ( missionType == AMIS_SAD && missionType == AMIS_INT && missionType == AMIS_BAI )
		agDoctrine = AGD_LOOK_SHOOT_LOOK;

	if ((missionType == AMIS_BDA || missionType == AMIS_RECON) && hasCamera )
	{
		agDoctrine = AGD_SHOOT_RUN;
		agApproach = AGA_DIVE;
	}
	// any weapons?
	else if ( !hasWeapons )
	{
		// no air radar
		agDoctrine = AGD_NONE;
		SetGroundTarget( NULL );
	}
}
#endif

// 2000-09-27 FUNCTION REWROTE BY S.G. THERE IS NO SUCH THING AS AN EASY FIX I GUESS :-(
#if 0
void DigitalBrain::IPCheck (void)
{
WayPointClass* tmpWaypoint = self->waypoint;
float wpX, wpY, wpZ;
float dX, dY, dZ;
float rangeSq;
short edata[6];
int response;

   // Periodically check for IP and if so, ask for permission to engage
   if (!IsSetATC(ReachedIP) &&
      flightLead->IsSetFlag(MOTION_OWNSHIP))
   {
      // Find the IP waypoint
      while (tmpWaypoint)
      {
         if (tmpWaypoint->GetWPFlags() & WPF_TARGET)
            break;
         tmpWaypoint = tmpWaypoint->GetNextWP();
      }

		// From the target, find the IP
      if (tmpWaypoint)
			tmpWaypoint = tmpWaypoint->GetPrevWP();

      // Have an IP
      if (tmpWaypoint)
      {
         tmpWaypoint->GetLocation (&wpX, &wpY, &wpZ);
         dX = self->XPos() - wpX;
         dY = self->YPos() - wpY;
         dZ = self->ZPos() - wpZ;

         // Check for within 5 NM of IP
         rangeSq = dX*dX + dY*dY + dZ*dZ;
         if (rangeSq < (5.0F * NM_TO_FT)*(5.0F * NM_TO_FT))
         {
            // In Range and getting closer
            if (rangeSq < rangeToIP)
            {
               rangeToIP = rangeSq;
            }
            else
            {
               SetATCFlag(ReachedIP);
               SetATCFlag(WaitingPermission);
               // At the IP, should we ask for permission
               if (missionClass == AGMission && !missionComplete &&
                   !mpActionFlags[AI_ENGAGE_TARGET])
               {
                  //Closest approach, ask for permission to engage
                  SetATCFlag (AskedToEngage);
			         edata[0]	= ((FlightClass*)self->GetCampaignObject())->callsign_id;
			         edata[1]	= (((FlightClass*)self->GetCampaignObject())->callsign_num - 1) * 4 + isWing + 1;
			         response = rcREQUESTTOENGAGE;
   		         AiMakeRadioResponse( self, response, edata );
               }
            }
         }
      }
   }
}
#else

void DigitalBrain::IPCheck (void)
{
	WayPointClass* tmpWaypoint = self->waypoint;
	float wpX, wpY, wpZ;
	float dX, dY;
	float rangeSq;
	short edata[6];
	int response;

	// Only for the player's wingmen
	// JB 020315 All aircraft will now all check to see if they have passed the IP.  
	// Only checking for the IP if the lead is a player is silly.  
	// Things depend on the ReachedIP flag being set properly.
	//if (flightLead && flightLead->IsSetFlag(MOTION_OWNSHIP)) 
	{
		// Periodically check for IP and if so, ask for permission to engage
		if (!IsSetATC(ReachedIP)) {
			// Find the IP waypoint
			while (tmpWaypoint) {
				if (tmpWaypoint->GetWPFlags() & WPF_TARGET)
					break;
				tmpWaypoint = tmpWaypoint->GetNextWP();
			}

			// From the target, find the IP
			if (tmpWaypoint)
				tmpWaypoint = tmpWaypoint->GetPrevWP();

			// Have an IP
			if (tmpWaypoint) {
				tmpWaypoint->GetLocation (&wpX, &wpY, &wpZ);
				dX = self->XPos() - wpX;
				dY = self->YPos() - wpY;

				// Original code was looking for the SLANT distance, I'm not...
				// Check for within 5 NM of IP
				rangeSq = dX*dX + dY*dY;
				if (rangeSq < (5.0F * NM_TO_FT)*(5.0F * NM_TO_FT)) {
					// In Range and getting closer?
					if (rangeSq < rangeToIP) {
						// Yes, update our range to IP
						rangeToIP = rangeSq;
					}
					else {
						// The range is INCREASING so we assume (may be wronly if we turned away from the IP waypoint) we've reached it, say so and wait for a target
						SetATCFlag(ReachedIP);
						// JB 020315 Only set the WaitForTarget flag if the lead is a player.
						if (flightLead && flightLead->IsSetFlag(MOTION_OWNSHIP)) 
							SetATCFlag(WaitForTarget);
					}
				}
			}
		}
		// We have reached our IP waypoint, are we waiting for a target?
		else if (IsSetATC(WaitForTarget)) {
			// Yes, ask for permission if we are an incomplete AGMission that doesn't already have a designated target and it holds a ground target
			if (missionClass == AGMission && !missionComplete && !mpActionFlags[AI_ENGAGE_TARGET] && groundTargetPtr)
			{
				// Flag we are waiting for permission and we have a ground target to shoot at
				SetATCFlag(WaitingPermission);
				ClearATCFlag(WaitForTarget);

				// Ask for permission to engage
				SetATCFlag (AskedToEngage);
				edata[0]	= ((FlightClass*)self->GetCampaignObject())->callsign_id;
				edata[1]	= (((FlightClass*)self->GetCampaignObject())->callsign_num - 1) * 4 + isWing + 1;
				response = rcREQUESTTOENGAGE;
				AiMakeRadioResponse( self, response, edata );
			}
		}
	}
}
#endif

void DigitalBrain::TakePicture(float approxRange, float ata)
{
FireControlComputer* FCC = self->FCC;

   // Go to camera mode
   if (FCC->GetMasterMode() != FireControlComputer::AirGroundCamera)
   {
		FCC->SetMasterMode (FireControlComputer::AirGroundCamera);
   }

	if ( groundTargetPtr == NULL )
	{
		 onStation = Final1;
	}
	else if ( approxRange < 3.0f * NM_TO_FT && ata < 10.0f * DTR)
	{
		 // take picture
		 waitingForShot = SimLibElapsedTime;
		 onStation = Final1;
       SetFlag (MslFireFlag);
	}
}

void DigitalBrain::DropBomb(float approxRange, float ata, RadarClass* theRadar)
{
FireControlComputer* FCC = self->FCC;
SMSClass* Sms = self->Sms;
float dx, dy;

   F4Assert (!Sms->curWeapon || Sms->curWeapon->IsBomb());

   // Make sure the FCC is in the right mode/sub mode
	if ( FCC->GetMasterMode() != FireControlComputer::AirGroundBomb ||
		 FCC->GetSubMode() != FireControlComputer::CCRP)
   {
   	FCC->SetMasterMode (FireControlComputer::AirGroundBomb);
		FCC->SetSubMode (FireControlComputer::CCRP);
   }

	if (!Sms->curWeapon || !Sms->curWeapon->IsBomb())
	{
        if (Sms->FindWeaponClass (wcBombWpn))
           Sms->SetWeaponType(Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponType());
        else
           Sms->SetWeaponType (wtNone);
	}

   // Point the radar at the target
	if (theRadar) 
   {
      if (groundTargetPtr && groundTargetPtr->BaseData()->IsMover())
         theRadar->SetMode(RadarClass::GMT);
      else
         theRadar->SetMode(RadarClass::GM);
		theRadar->SetDesiredTarget(groundTargetPtr);
      theRadar->SetAGSnowPlow(TRUE);
   }

   // Mode the SMS
   Sms->SetPair(TRUE);

   // Give the FCC permission to release if in parameters
   SetFlag (MslFireFlag);

   // Adjust for wind/etc
   if (fabs(FCC->airGroundBearing) < 5.0F * DTR)
   {
      trackX = 2.0F * FCC->groundDesignateX - FCC->groundImpactX;
      trackY = 2.0F * FCC->groundDesignateY - FCC->groundImpactY;
   }

   if (agApproach == AGA_BOMBER)
   {
      if (!droppingBombs)
      {
         // Try to put the middle drop on target
         Sms->SetRippleCount (int(Sms->NumCurrentWpn() / 2.0F + 0.5F) - 1);
				 
				 int rcount = Sms->RippleCount() + 1;
				 if (!(rcount & 1)) // If not odd
					 rcount--;
					 					
         //if (FCC->airGroundRange < Sms->NumCurrentWpn() * 2.0F * Sms->RippleInterval())
				 if (FCC->airGroundRange < (rcount * Sms->RippleInterval()) / 2) // JB 010408 010708 Space the ripples correctly over the target
         {
   		   droppingBombs = wcBombWpn;
            FCC->SetBombReleaseOverride(TRUE);
            onStation = Final1;
         }
      }
      else
      {

         if (Sms->NumCurrentWpn() == 0)
         {
            FCC->SetBombReleaseOverride(FALSE);
            // Out of this weapon, find another/get out of dodge
            agDoctrine = AGD_LOOK_SHOOT_LOOK;
            hasRocket = FALSE;
            hasGun = FALSE;
            hasBomb = FALSE;
            hasGBU = FALSE;

            // Start over again
            madeAGPass = TRUE;
		      onStation = NotThereYet;
         }
	      // too close ?
	      // we're within a certain range and our ATA is not good
	      else if ( approxRange < 1.2f * NM_TO_FT && ata > 75.0f * DTR)
	      {
		       waitingForShot = SimLibElapsedTime + 5000;
		       onStation = Final1;
	      }
      }

   }
   else
   {
		// JB 010708 start Drop all your dumb bombs of the current type or if 
		// the lead is a player (not in autopilot) use the lead's ripple setting.
		if (!droppingBombs)
		{
			if (flightLead && ((AircraftClass*)flightLead)->AutopilotType() != AircraftClass::CombatAP && ((AircraftClass*)flightLead)->Sms)
				Sms->SetRippleCount (min(((AircraftClass*)flightLead)->Sms->RippleCount(), int(Sms->NumCurrentWpn() / 2.0F + 0.5F) - 1));
			else
				Sms->SetRippleCount (int(Sms->NumCurrentWpn() / 2.0F + 0.5F) - 1);

		  int rcount = Sms->RippleCount() + 1;
			if (!(rcount & 1)) // If not odd
			  rcount--;

			if (Sms->RippleCount() > 0 && FCC->airGroundRange < (rcount * Sms->RippleInterval()) / 2)
			{
				droppingBombs = wcBombWpn;
				FCC->SetBombReleaseOverride(TRUE);
				onStation = Final1;
			}

// 2001-10-24 ADDED BY M.N. Planes can start to circle around their target if we don't do
// a range & ata check to the target here.

			if (approxRange < 1.2f * NM_TO_FT && ata > 75.0f * DTR)
			{
            // Bail and try again
			    dx = ( self->XPos() - trackX );
			    dy = ( self->YPos() - trackY );
			    approxRange = (float)sqrt( dx * dx + dy * dy );
			    dx /= approxRange;
			    dy /= approxRange;
			    ipX = trackX + dy * 5.5f * NM_TO_FT;
			    ipY = trackY - dx * 5.5f * NM_TO_FT;
				ShiAssert (ipX > 0.0F);

            // Try bombing run again
				onStation = Crosswind;
			}
// End of added section
		}
		// JB 010708 end
		else
		{
			 // if ( FCC->postDrop)
			 if ( FCC->postDrop && Sms->CurRippleCount() == 0) // JB 010708
			 {
           FCC->SetBombReleaseOverride(FALSE); // JB 010708
					 droppingBombs = wcBombWpn;

					 // Out of this weapon, find another/get out of dodge
					 agDoctrine = AGD_LOOK_SHOOT_LOOK;
					 hasRocket = FALSE;
					 hasGun = FALSE;
					 hasBomb = FALSE;
					 hasGBU = FALSE;

					 // Start over again
					 madeAGPass = TRUE;
				 onStation = NotThereYet;
			 }
			 // too close ?
			 // we're within a certain range and our ATA is not good
			 if ( approxRange < 1.2f * NM_TO_FT && ata > 75.0f * DTR)
			 {
					waitingForShot = SimLibElapsedTime + 5000;
					onStation = Final1;
			 }
		 }
   }
}

void DigitalBrain::DropGBU(float approxRange, float ata, RadarClass* theRadar)
{
LaserPodClass* targetingPod = NULL;
FireControlComputer* FCC = self->FCC;
SMSClass* Sms = self->Sms;
float dx, dy, angle;
mlTrig trig;

   F4Assert (!Sms->curWeapon || Sms->curWeapon->IsBomb());

   // Don't stop in the middle
	droppingBombs = wcGbuWpn;

   // Make sure the FCC is in the right mode/sub mode
	if ( FCC->GetMasterMode() != FireControlComputer::AirGroundLaser ||
		 FCC->GetSubMode() != FireControlComputer::SLAVE)
   {
   		FCC->SetMasterMode (FireControlComputer::AirGroundLaser);
		FCC->SetSubMode (FireControlComputer::SLAVE);
   }

	if (!Sms->curWeapon || !Sms->curWeapon->IsBomb())
	{
        if (Sms->FindWeaponClass (wcGbuWpn, FALSE))
        {
           Sms->SetWeaponType(Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponType());
        }
        else
        {
           Sms->SetWeaponType (wtNone);
        }
	}


   // Get the targeting pod locked on to the target
   targetingPod = (LaserPodClass*) FindLaserPod (self);
   if (targetingPod && targetingPod->CurrentTarget())
   {
      if (!targetingPod->IsLocked())
      {
         // Designate needs to go down then up then down to make it work
         if (FCC->designateCmd)
            FCC->designateCmd = FALSE;
         else
            FCC->designateCmd = TRUE;
      }
      else
      {
         FCC->designateCmd = FALSE;
      }

      FCC->preDesignate = FALSE;
   }

   // Point the radar at the target
	if (theRadar) 
   {
      if (groundTargetPtr && groundTargetPtr->BaseData()->IsMover())
         theRadar->SetMode(RadarClass::GMT);
      else
         theRadar->SetMode(RadarClass::GM);
		theRadar->SetDesiredTarget(groundTargetPtr);
      theRadar->SetAGSnowPlow(TRUE);
   }

   // Mode the SMS
   Sms->SetPair(FALSE);
   Sms->SetRippleCount(0);

   // Adjust for wind/etc
   if (fabs(FCC->airGroundBearing) < 10.0F * DTR)
   {
      trackX = 2.0F * FCC->groundDesignateX - FCC->groundImpactX;
      trackY = 2.0F * FCC->groundDesignateY - FCC->groundImpactY;
   }

   // Give the FCC permission to release if in parameters
   if (onStation == Final)
   {
      if(SimLibElapsedTime > waitingForShot)
      {
// 2001-08-31 REMOVED BY S.G. NOT USED ANYWAY AND I NEED THE FLAG FOR SOMETHING ELSE
//			if (approxRange < 1.2F * FCC->airGroundRange)
//				SetATCFlag(InhibitDefensive);

         // Check for too close
         if (approxRange < 0.5F * FCC->airGroundRange)
         {
            // Bail and try again
			   dx = ( self->XPos() - trackX );
			   dy = ( self->YPos() - trackY );
			   approxRange = (float)sqrt( dx * dx + dy * dy );
			   dx /= approxRange;
			   dy /= approxRange;
			   ipX = trackX + dy * 7.5f * NM_TO_FT;
			   ipY = trackY - dx * 7.5f * NM_TO_FT;
				ShiAssert (ipX > 0.0F);

            // Start again
            onStation = Crosswind;
            //MonoPrint ("Too close to GBU, head to IP and try again\n");
         }

         SetFlag (MslFireFlag);

         if (FCC->postDrop)
         {
            // 1/2 second between bombs, 10 seconds after last bomb
            if (Sms->NumCurrentWpn() % 2 != 0)
            {
               waitingForShot = SimLibElapsedTime + (SEC_TO_MSEC/2);
            }
            else
            {
               // Keep Lasing
               madeAGPass = TRUE;
		         onStation = Final1;
               waitingForShot = SimLibElapsedTime;
            }
         }
      }
      else // Are we too close?
      {     
         if (onStation == Final && approxRange < 0.7F * FCC->airGroundRange || ata > 60.0F * DTR)
         {
            // Bail and try again
			   dx = ( self->XPos() - trackX );
			   dy = ( self->YPos() - trackY );
			   approxRange = (float)sqrt( dx * dx + dy * dy );
			   dx /= approxRange;
			   dy /= approxRange;
			   ipX = trackX + dy * 7.5f * NM_TO_FT;
			   ipY = trackY - dx * 7.5f * NM_TO_FT;
				ShiAssert (ipX > 0.0F);

            // Start again
            onStation = Crosswind;
            //MonoPrint ("Too close to bomb, head to IP and try again\n");
         }
      }
   }

   // Out of this weapon, find another/get out of dodge
   if (onStation == Final1)
   {
      if (SimLibElapsedTime > waitingForShot) // Bomb has had time to fall.
      {
         agDoctrine = AGD_LOOK_SHOOT_LOOK;
         hasRocket = FALSE;
         hasGun = FALSE;
         hasBomb = FALSE;
         hasGBU = FALSE;
         droppingBombs = FALSE;

         // Force a weapon/target selection
         madeAGPass = TRUE;
		 onStation = NotThereYet;
		 moreFlags &= ~KeepLasing; // 2002-03-08 ADDED BY S.G. Not lasing anymore
      }
      else if (SimLibElapsedTime == waitingForShot) // Turn but keep designating
      {
			dx = ( trackX - self->XPos() );
			dy = ( trackY - self->YPos() );
			angle = 45.0F * DTR + (float)atan2(dy, dx);
			mlSinCos (&trig, angle);
// 2001-07-10 MODIFIED BY S.G. SINCE WE DROP FROM HIGHER, NEED TO DESIGNATE LONGER
//			ipX = trackX = trackX + trig.cos * 6.5f * NM_TO_FT;
//			ipY = trackY = trackY + trig.sin * 6.5f * NM_TO_FT;
			ipX = trackX = trackX + trig.cos * 7.5f * NM_TO_FT;
			ipY = trackY = trackY + trig.sin * 7.5f * NM_TO_FT;
			ipZ = self->ZPos();
//			waitingForShot = SimLibElapsedTime + 20 * SEC_TO_MSEC;
			waitingForShot = SimLibElapsedTime + 27 * SEC_TO_MSEC;
			ShiAssert (ipX > 0.0F);
			moreFlags |= KeepLasing; // 2002-03-08 ADDED BY S.G. Flag this AI as lasing so he sticks to it
      }
      else
      {
         trackX = ipX;
         trackY = ipY;
         trackZ = ipZ;
      }
   }
   //MI NULL out our Target
   if ( groundTargetPtr &&
	   groundTargetPtr->BaseData()->IsSim() &&
	   ( groundTargetPtr->BaseData()->IsDead() ||
	   groundTargetPtr->BaseData()->IsExploding() ) )
   {
	   SetGroundTarget( NULL );
   }

}

void DigitalBrain::FireAGMissile(float approxRange, float ata)
{
SMSClass* Sms = self->Sms;

   F4Assert (!Sms->curWeapon || Sms->curWeapon->IsMissile());

	// Check Timer
// 2001-07-12 MODIFIED BY S.G. DON'T LAUNCH UNTIL CLOSE TO OUR ATTACK ALTITUDE
//	if ( SimLibElapsedTime >= waitingForShot )
	if ( SimLibElapsedTime >= waitingForShot && self->ZPos() - trackZ >= -500.0f )
	{
      SetFlag (MslFireFlag);

		// if we're out of missiles and bombs and our
		// doctrine is look shoot look, we don't want to
		// continue with guns/rockets only -- reset
		// agDoctrine if this is the case
// 2001-05-03 MODIFIED BY S.G. WE STOP FIRING WHEN WE HAVE AN ODD NUMBER OF MISSILE LEFT (MEANT WE FIRED ONE ALREADY) THIS WILL LIMIT IT TO 2 MISSILES PER TARGET
//		if (Sms->NumCurrentWpn() == 1 )
		if (Sms->NumCurrentWpn() & 1 )
		{
			hasRocket = FALSE;
			hasGun = FALSE;
			hasHARM = FALSE;
			hasAGMissile = FALSE;

			// Force a weapon/target selection
			madeAGPass = TRUE;
			onStation = NotThereYet;

// 2001-06-01 ADDED BY S.G. THAT WAY, AI WILL KEEP GOING STRAIGHT FOR A SECOND BEFORE PULLING
			missileShotTimer = SimLibElapsedTime + 1000;
// END OF ADDED SECTION
// 2001-06-16 ADDED BY S.G. THAT WAY, AI WILL NOT RETURN TO THEIR IP ONCE THEY FIRED BUT WILL GO "PERPENDICULAR FOR 2 NM"...
// 2001-07-16 MODIFIED BY S.G. DEPENDING IF WE HAVE WEAPONS LEFT, PULL MORE OR LESS
			if (groundTargetPtr) {
				float dx = groundTargetPtr->BaseData()->XPos() - self->XPos();
				float dy = groundTargetPtr->BaseData()->YPos() - self->YPos();

				// x-y get range
				float range = (float)sqrt( dx * dx + dy * dy ) + 0.1f;

				// normalize the x and y vector
				dx /= range;
				dy /= range;

				if (Sms->NumCurrentWpn() == 1) {
					ipX = self->XPos() + dy * 5.0f * NM_TO_FT - dx * 1.5f * NM_TO_FT;
					ipY = self->YPos() - dx * 5.0f * NM_TO_FT - dy * 1.5f * NM_TO_FT;
				}
				else {
					ipX = self->XPos() + dy * 3.0f * NM_TO_FT - dx * 1.5f * NM_TO_FT;
					ipY = self->YPos() - dx * 3.0f * NM_TO_FT - dy * 1.5f * NM_TO_FT;
				}
			}
// END OF ADDED SECTION
		}

		// determine if we shoot and run or not
		waitingForShot = SimLibElapsedTime + 5000;
	}
	// too close or already fired, try again
	// we're within a certain range and our ATA is not good
	else if ( approxRange < 1.2f * NM_TO_FT && ata > 75.0f * DTR)
	{
		 waitingForShot = SimLibElapsedTime + 5000;
		 onStation = Final1;
	}
}

void DigitalBrain::FireRocket(float approxRange, float ata)
{
FireControlComputer* FCC = self->FCC;
SMSClass* Sms = self->Sms;

   if (FCC->GetMasterMode() != FireControlComputer::AirGroundBomb ||
	   FCC->GetSubMode() != FireControlComputer::RCKT )
   {
	   FCC->SetMasterMode (FireControlComputer::AirGroundBomb);
	   FCC->SetSubMode (FireControlComputer::RCKT);
   }

   // JB 000102 from Marco
	 // the following lines are commented out -- AI will now fire rockets
	 //if (Sms->NumCurrentWpn() <= 0)
   //{
   //   hasRocket = FALSE;
   //   madeAGPass = TRUE;
   //  onStation = NotThereYet;
   //}
   // JB 000102 from Marco

   if ( ata < 5.0f * DTR && approxRange < 1.8f * NM_TO_FT )
   {
      SetFlag (MslFireFlag);
   }
   else if (approxRange < 1.2f * NM_TO_FT && ata > 75.0f * DTR)
   {
	   waitingForShot = SimLibElapsedTime + 5000;
	   onStation = Final1;
   }
}

void DigitalBrain::GunStrafe(float approxRange, float ata)
{
FireControlComputer* FCC = self->FCC;

   if (FCC->GetMasterMode() != FireControlComputer::Gun ||
	   FCC->GetSubMode() != FireControlComputer::STRAF )
   {
	   FCC->SetMasterMode (FireControlComputer::Gun);
	   FCC->SetSubMode (FireControlComputer::STRAF);
   }

	if ( ata < 5.0f * DTR && approxRange < 1.2f * NM_TO_FT )
	{
		SetFlag( GunFireFlag );
	}
	else if ( approxRange < 1.2f * NM_TO_FT && ata > 75.0f * DTR )
	{
		waitingForShot = SimLibElapsedTime + 5000;
		onStation = Final1;
	}
}

int DigitalBrain::MaverickSetup (float rx, float ry, float ata, float approxRange, RadarClass* theRadar)
{
FireControlComputer* FCC = self->FCC;
SMSClass* Sms = self->Sms;
float dx, dy, az, rMin, rMax;
int retval = TRUE;

   if (FCC->postDrop)
   {
      // Force a retarget
      if (groundTargetPtr && groundTargetPtr->BaseData()->IsSim())
      {
         SetGroundTarget (((SimBaseClass*)groundTargetPtr->BaseData())->GetCampaignObject());
      }
      SelectGroundTarget(TARGET_ANYTHING);
      retval = FALSE;
   }
   else
   {
      // Point the radar at the target
	   if (theRadar) 
      {
// 2001-07-23 MODIFIED BY S.G. MOVERS ARE ONLY 3D ENTITIES WHILE BATTALIONS WILL INCLUDE 2D AND 3D VEHICLES...
//       if (groundTargetPtr && groundTargetPtr->BaseData()->IsMover())
         if (groundTargetPtr && (groundTargetPtr->BaseData()->IsSim() && ((SimBaseClass *)groundTargetPtr->BaseData())->GetCampaignObject()->IsBattalion()) || (groundTargetPtr->BaseData()->IsCampaign() && groundTargetPtr->BaseData()->IsBattalion()))
            theRadar->SetMode(RadarClass::GMT);
         else
            theRadar->SetMode(RadarClass::GM);
		   theRadar->SetDesiredTarget(groundTargetPtr);
         theRadar->SetAGSnowPlow(TRUE);
      }

	   F4Assert (!Sms->curWeapon || Sms->curWeapon->IsMissile());

   
      // Set up FCC for maverick shot
      if (FCC->GetMasterMode() != FireControlComputer::AirGroundMissile)
      {
			FCC->SetMasterMode (FireControlComputer::AirGroundMissile);
			FCC->SetSubMode (FireControlComputer::SLAVE);
			FCC->designateCmd = FALSE; 
			FCC->preDesignate = TRUE;
      }
// 2001-07-23 REMOVED BY S.G. DO LIKE THE HARMSetup. ONCE SET, DO YOUR STUFF
//    else
      {
         // Has the current weapon locked?
         if (Sms->curWeapon)
         {
            if (Sms->curWeapon->targetPtr)
            {
               if (self->curSOI != SimVehicleClass::SOI_WEAPON)
               {
                  FCC->designateCmd = TRUE;
               }
               else
               {
                  FCC->designateCmd = FALSE;
               }

               FCC->preDesignate = FALSE;
            }
// 2001-07-23 ADDED BY S.G. DON'T LAUNCH IF OUR MISSILE DO NOT HAVE A LOCK!
			else
				retval = FALSE;

	         // fcc target needs to be set cuz that's the target
	         // that will be used in sms launch missile
            FCC->SetTarget(groundTargetPtr);

	         az	= (float)atan2(ry,rx);
		 ShiAssert(Sms->curWeapon->IsMissile());
	         rMax = ((MissileClass*)Sms->curWeapon)->GetRMax(-self->ZPos(), self->Vt(), az, 0.0f, 0.0f );

// 2001-08-31 REMOVED BY S.G. NOT USED ANYWAY AND I NEED THE FLAG FOR SOMETHING ELSE
//				if (approxRange < rMax)
//					SetATCFlag(InhibitDefensive);

	         // rmin is just a percent of rmax
	         rMin = rMax * 0.1f;
	         // get the sweet spot
	         rMax *= 0.8f;

            // Check for firing solution
            if ( !( ata < 15.0f * DTR && approxRange > rMin && approxRange < rMax ) )
	         {
		         retval = FALSE;
	         }
// 2001-07-23 ADDED BY S.G. MAKE SURE WE CAN SEE THE TARGET
			else if (Sms->curWeapon) {
				// First make sure the relative geometry is valid
				if (Sms->curWeapon->targetPtr) {
					CalcRelGeom(self, Sms->curWeapon->targetPtr, NULL, 1.0F / SimLibMajorFrameTime);
					// Then run the seeker if we already have a target
					((MissileClass *)Sms->curWeapon)->RunSeeker();
				}

				// If we have no target, don't shoot!
				if (!Sms->curWeapon || !Sms->curWeapon->targetPtr)
					retval = FALSE;
			}
// END OF ADDED SECTION

            // Check for Min Range
            if (approxRange < 1.1F * rMin || ata > 165.0F * DTR)
            {
               // Bail and try again
		         dx = ( self->XPos() - trackX );

               dy = ( self->YPos() - trackY );
		         approxRange = (float)sqrt( dx * dx + dy * dy );
		         dx /= approxRange;
		         dy /= approxRange;
		         ipX = trackX + dy * 7.5f * NM_TO_FT;
		         ipY = trackY - dx * 7.5f * NM_TO_FT;
					ShiAssert (ipX > 0.0F);

               // Start again
               onStation = Crosswind;
               //MonoPrint ("Too close to Maverick, head to IP and try again\n");
					retval = FALSE;
            }
         }
         else
         {
            retval = FALSE;
         }
      }
   }

   return retval;
}

int DigitalBrain::HARMSetup (float rx, float ry, float ata, float approxRange)
{
FireControlComputer* FCC = self->FCC;
SMSClass* Sms = self->Sms;
float dx, dy, az, rMin, rMax;
int retval = TRUE;
HarmTargetingPod	*theHTS;
	
	theHTS = (HarmTargetingPod*)FindSensor(self, SensorClass::HTS);

	F4Assert (!Sms->curWeapon || Sms->curWeapon->IsMissile());
   // Set up FCC for harm shot
   if (FCC->GetMasterMode() != FireControlComputer::AirGroundHARM)
   {
		FCC->SetMasterMode (FireControlComputer::AirGroundHARM);
		FCC->SetSubMode (FireControlComputer::HTS);
   }

	// fcc target needs to be set cuz that's the target
	// that will be used in sms launch missile
   FCC->SetTarget(groundTargetPtr);

	az	= (float)atan2(ry,rx);

	// Got this null in multiplayer - RH
	ShiAssert (Sms->curWeapon);

	if (Sms->curWeapon)
	{
	    ShiAssert(Sms->curWeapon->IsMissile());
		rMax = ((MissileClass*)Sms->curWeapon)->GetRMax(-self->ZPos(), self->Vt(), az, 0.0f, 0.0f );
	}
	else
	{
		rMax = 0.1F;
	}

	// 2002-01-21 ADDED BY S.G. GetRMax is not enough, need to see if the HARM seeker will see the target as well
	//                          Adjust rMax accordingly.
	int radarRange;
	if (groundTargetPtr->BaseData()->IsSim())
		radarRange = ((SimBaseClass*)groundTargetPtr->BaseData())->RdrRng();
	else {
		if (groundTargetPtr->BaseData()->IsEmitting())
			radarRange = RadarDataTable[groundTargetPtr->BaseData()->GetRadarType()].NominalRange;
		else
			radarRange = 0.0f;
	}
	rMax = min(rMax, radarRange);
	rMax = max(rMax, 0.1f);
	// END OF ADDED SECTION 2002-01-21

// 2001-08-31 REMOVED BY S.G. NOT USED ANYWAY AND I NEED THE FLAG FOR SOMETHING ELSE
//	if (approxRange < rMax)
//		SetATCFlag(InhibitDefensive);

	// rmin is just a percent of rmax
	rMin = rMax * 0.1f;
	// get the sweet spot
	rMax *= 0.8f;

   // Make sure the HTS has the target
   if (theHTS)
   {
      theHTS->SetDesiredTarget (groundTargetPtr);
      if (!theHTS->CurrentTarget())
         retval = FALSE;
			
      // JB 020121 Make sure the HTS can detect the target otherwise HARMs will immediately fail to guide.
      // 2002-01-21 REMOVED BY S.G. Done differently above, CanDetectObject calls CheckLOS which I'm doing below as well.
      /*if (!theHTS->CanDetectObject(groundTargetPtr))
         retval = FALSE; */

// 2001-06-18 ADDED BY S.G. JUST DO A LOS CHECK :-( I CAN'T RELIABLY GET THE POD LOCK STATUS
	  if (!self->CheckLOS(groundTargetPtr))
		  retval = FALSE;
// END OF ADDED SECTION
   }

   // Check for firing conditions
// 2001-07-05 MODIFIED BY S.G. HARMS CAN BE FIRED BACKWARD BUT LETS LIMIT AI TO A MORE REALISTIC LAUNCH PARAMETER
// if ( !( ata < 15.0f * DTR && approxRange > rMin && approxRange < rMax ) )
   if ( !( ata < 60.0f * DTR && approxRange > rMin && approxRange < rMax ) )
	{
		retval = FALSE;
	}

   // Check for Min Range
// 2001-07-05 MODIFIED BY S.G. USE ONLY rMin AND ata DOESN'T MATTER
// if (approxRange < 1.1F * rMin || ata > 135.0F * DTR)
   if (approxRange < rMin)
   {
      // Bail and try again
		dx = ( self->XPos() - trackX );
		dy = ( self->YPos() - trackY );
		approxRange = (float)sqrt( dx * dx + dy * dy );
		dx /= approxRange;
		dy /= approxRange;
// 2001-07-05 MODIFIED BY S.G. USE A FRACTION OF rMax INSTEAD OF A FIX VALUE
//	   ipX = trackX + dy * 15.0f * NM_TO_FT;
//	   ipY = trackY - dx * 15.0f * NM_TO_FT;
	   ipX = trackX + dy * rMax * 0.5f;
	   ipY = trackY - dx * rMax * 0.5f;
		ShiAssert (ipX > 0.0F);

      // Start again
      onStation = Crosswind;
      //MonoPrint ("Too close to HARM, head to IP and try again\n");
   }

	// we want to see what the target campaign
	// entity is doing
	if ( groundTargetPtr->BaseData()->IsSim() )
	{
// 2001-06-25 ADDED BY S.G. IF I HAVE SOMETHING IN shotAt, IT COULD MEAN SOMEONE SHOT WHILE THE TARGET WAS AGGREGATED. DEAL WITH THIS
		// If shotAt has something, someone is/was targeting the aggregated entity. If it wasn't me, don't fire at it once it is deaggregated as well.
		if (((FlightClass *)self->GetCampaignObject())->shotAt == ((SimBaseClass *)groundTargetPtr->BaseData())->GetCampaignObject() && ((FlightClass *)self->GetCampaignObject())->whoShot != self)
			retval = FALSE - 1;
// END OF ADDED SECTION
// 2001-05-27 MODIFIED BY S.G. LAUNCH AT A CLOSER RANGE IF NOT EMITTING (AND IT'S THE ONLY WEAPONS ON BOARD - TESTED SOMEWHERE ELSE)
//		if ( !((SimBaseClass *)groundTargetPtr->BaseData())->GetCampaignObject()->IsEmitting() && approxRange > 0.5F * rMax)
//			retval = FALSE;
		// 2002-01-20 MODIFIED BY S.G. If RdrRng() is zero, this means the radar is off. Can't fire at it if it's off! (only valid for sim object)
//		if ( !((SimBaseClass *)groundTargetPtr->BaseData())->GetCampaignObject()->IsEmitting())
		if ( !((SimBaseClass *)groundTargetPtr->BaseData())->GetCampaignObject()->IsEmitting() || ((SimBaseClass *)groundTargetPtr->BaseData())->RdrRng() == 0)
		{
			retval = FALSE;
// 2001-07-02 MODIFIED BY S.G. IT'S NOW 0.25 SO TWICE AS CLOSE AS BEFORE
//			if (approxRange < 0.5F * rMax)
			if (approxRange < 0.25F * rMax)
			{
// 2001-06-24 ADDED BY S.G. TRY WITH SOMETHING ELSE IF YOU CAN
				if (hasAGMissile | hasBomb | hasRocket | hasGun | hasGBU)
					retval = FALSE - 1;
				else {
// END OF ADDED SECTION
					agDoctrine = AGD_NONE;
					missionComplete = TRUE;
					self->FCC->SetMasterMode (FireControlComputer::Nav);
					self->FCC->preDesignate = TRUE;
					SetGroundTarget( NULL );
					SelectNextWaypoint();
					// if we're a wingie, rejoin the lead
					if ( isWing ) {
						mFormation = FalconWingmanMsg::WMWedge;
						AiRejoin( NULL );
						// make sure wing's designated target is NULL'd out
						mDesignatedObject = FalconNullId;
					}
					else // So the player's wingmen still know they have something
						hasWeapons = FALSE; // Got here so nothing else than HARMS was available anyway
				}
			}
		}
// END OF MODIFIED SECTION
	}
	else
	{
		// campaign entity
// 2001-06-25 ADDED BY S.G. IF IT IS AGGREGATED, ONLY ONE PLANE CAN SHOOT AT IT WITH HARMS
		if (((CampBaseClass *)groundTargetPtr->BaseData())->IsAggregate()) {
			// 2002-01-20 ADDED BY S.G. Unless it's an AAA since it has more than one radar.
			if (!groundTargetPtr->BaseData()->IsBattalion() || ((BattalionClass *)groundTargetPtr->BaseData())->class_data->RadarVehicle < 16) {
			// END OF ADDED SECTION 2002-01-20
				// If it's not at what we shot last, then it's valid
				if (((FlightClass *)self->GetCampaignObject())->shotAt != groundTargetPtr->BaseData()) {
					((FlightClass *)self->GetCampaignObject())->shotAt = groundTargetPtr->BaseData();
					((FlightClass *)self->GetCampaignObject())->whoShot = self;
				}
				// If one of us is shooting, make sure it's me, otherwise no HARMS for me please.
				else if (((FlightClass *)self->GetCampaignObject())->whoShot != self)
					retval = FALSE - 1;
			}
		}
// END OF ADDED SECTION
// 2001-05-27 MODIFIED BY S.G. LAUNCH AT A CLOSER RANGE IF NOT EMITTING (AND IT'S THE ONLY WEAPONS ON BOARD - TESTED SOMEWHERE ELSE)
//		if ( !groundTargetPtr->BaseData()->IsEmitting() && approxRange > 0.5F * rMax)
//			retval = FALSE;
// 2001-06-05 MODIFIED BY S.G. THAT'S IT IF YOU CAN CONNECT WITH IT...
// 2001-06-21 MODIFIED BY S.G. EVEN IF EMITTING, IF IT'S NOT AGGREGATED, DON'T FIRE (IE retval = FALSE)
		if ( !groundTargetPtr->BaseData()->IsEmitting() || !((CampBaseClass *)groundTargetPtr->BaseData())->IsAggregate() ) {
			retval = FALSE;
// 2001-07-02 MODIFIED BY S.G. IT'S NOW 0.25 SO TWICE AS CLOSE AS BEFORE
//			if (approxRange < 0.5F * rMax) {
			if (approxRange < 0.25F * rMax) {
// 2001-06-24 ADDED BY S.G. TRY WITH SOMETHING ELSE IF YOU CAN
				if (hasAGMissile | hasBomb | hasRocket | hasGun | hasGBU)
					retval = FALSE - 1;
				else {
// END OF ADDED SECTION
					agDoctrine = AGD_NONE;
					missionComplete = TRUE;
					self->FCC->SetMasterMode (FireControlComputer::Nav);
					self->FCC->preDesignate = TRUE;
					SetGroundTarget( NULL );
					SelectNextWaypoint();
					// if we're a wingie, rejoin the lead
					if ( isWing ) {
						mFormation = FalconWingmanMsg::WMWedge;
						AiRejoin( NULL );
						// make sure wing's designated target is NULL'd out
						mDesignatedObject = FalconNullId;
					}
					else // So the player's wingmen still know they have something
						hasWeapons = FALSE; // Got here so nothing else than HARMS was available anyway
				}
			}
		}
// END OF MODIFIED SECTION
	}

	// if we use missiles we don't drop bombs
	// unless we shot a harm
	if (agDoctrine != AGD_LOOK_SHOOT_LOOK )
   {
		hasBomb = FALSE;
      hasGBU = FALSE;
      hasRocket = FALSE;
   }

   return retval;
}
