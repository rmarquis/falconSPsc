#include "Graphics\Include\Render2D.h"
#include "Graphics\Include\DrawBsp.h"
#include "stdhdr.h"
#include "entity.h"
#include "PilotInputs.h"
#include "simveh.h"
#include "sms.h"
#include "airframe.h"
#include "object.h"
#include "fsound.h"
#include "soundfx.h"
#include "simdrive.h"
#include "mfd.h"
#include "radar.h"
#include "classtbl.h"
#include "playerop.h"
#include "navsystem.h"
#include "commands.h"
#include "hud.h"
#include "fcc.h"
#include "fault.h"
#include "fack.h"
#include "aircrft.h"
#include "smsdraw.h"
#include "airunit.h"
#include "handoff.h"
#include "otwdrive.h"	//MI
#include "radardoppler.h"	//MI
#include "missile.h"	//MI

extern bool g_bEnableColorMfd;
extern bool g_bRealisticAvionics;

const int	FireControlComputer::DATALINK_CYCLE	= 20;//JPO = 20 seconds
const float	FireControlComputer::MAXJSTARRANGESQ	= 200*200;//JPO = 200 nm
const float	FireControlComputer::EMITTERRANGE	= 60;//JPO = 40 km

const float FireControlComputer::CursorRate	= 0.15f; //MI added

FireControlComputer::FireControlComputer (SimVehicleClass* vehicle, int numHardpoints)
{
   platform = vehicle;
   airGroundDelayTime = 0.0F;
   airGroundRange     = 10.0F * NM_TO_FT;
   missileMaxTof = -1.0f;
   missileActiveTime = -1.0f;
   lastmissileActiveTime = -1.0f;
   bombPickle = FALSE;
   postDrop = FALSE;
   preDesignate = TRUE;
   tossAnticipationCue = NoCue;
   laddAnticipationCue = NoLADDCue;	//MI
   lastMasterMode = Nav;
   lastSubMode = ETE;
   lastAgSubMode = CCRP;//me123
   lastDFGunSubMode = EEGS;	//MI
   lastAAGunSubMode	= EEGS;	//MI
   strcpy (subModeString, "");
   playerFCC = FALSE;
   targetList = NULL;
   releaseConsent = FALSE;
   designateCmd = FALSE;
   dropTrackCmd = FALSE;
   targetPtr = NULL;
   missileCageCmd = FALSE;
	 missileTDBPCmd = FALSE;
   missileSpotScanCmd = FALSE;
   missileSlaveCmd = FALSE;
   cursorXCmd = 0;
   cursorYCmd = 0;
   waypointStepCmd = 127;	// Force an intial update (GM radar, at least, needs this)
   HSDRangeStepCmd = 0;
   HSDRange = 15.0F;
   HsdRangeIndex = 0; // JPO
   groundPipperAz = groundPipperEl = 0.0F;
   masterMode = Nav;
   subMode = ETE;
   dgftSubMode = Aim9; // JPO dogfight specific
   autoTarget = FALSE;
   missileWEZDisplayRange = 20.0F * NM_TO_FT;

	mSavedWayNumber	= 0;
	mpSavedWaypoint	= NULL;
	mStptMode			= FCCWaypoint;
	mNewStptMode		= mStptMode;
   bombReleaseOverride = FALSE;
   lastMissileShootRng = -1;
   missileLaunched = 0;
   lastMissileShootHeight = 0;
   lastMissileShootEnergy = 0;
   nextMissileImpactTime = -1.0F;
   lastMissileImpactTime = -1.0f;
	Height = 0;//me123
	targetspeed = 0;//me123
	hsdstates = 0; // JPO
	MissileImpactTimeFlash = 0; // JPO
	//me123 to remember weapon selected in overide modes
	weaponMisOvrdMode = wtAim120 ;
	weaponDogOvrdMode = wtAim9;
	weaponNoOvrdMode  = wtAim120;
	grndlist = NULL;
	BuildPrePlanned(); // JPO
	//MI
	LaserArm = FALSE;
	LaserFire = FALSE;
	ManualFire = FALSE;
	LaserWasFired = FALSE;
	CheckForLaserFire = FALSE;
	InhibitFire = FALSE;
	Timer = 0.0F;
	ImpactTime = 0.0F;
	LaserRange = 0.0F;
	SafetyDistance = 1 * NM_TO_FT;
	pitch = 0.0F;
	roll = 0.0F;
	yaw = 0.0F;
	time = 0;

	//MI SOI and HSD
	IsSOI = FALSE;
	CouldBeSOI = FALSE;
	HSDZoom = 0;
	HSDCursorXCmd = 0;
	HSDCursorYCmd = 0;
	xPos = 0;	//position of the curson on the scope
	yPos = 0;
	HSDDesignate = 0;
	curCursorRate = CursorRate;
	DispX = 0;
	DispY = 0;
	missileSeekerAz = missileSeekerEl = 0;
}

FireControlComputer::~FireControlComputer (void)
{
	ClearCurrentTarget();
	ClearPlanned(); // JPO
}

void FireControlComputer::SetPlayerFCC (int flag)
{
WayPointClass* tmpWaypoint;

	playerFCC = flag;
	Sms->SetPlayerSMS(flag);
	tmpWaypoint = platform->waypoint;
	TheHud->waypointNum = 0;
	while (tmpWaypoint && tmpWaypoint != platform->curWaypoint)
	{
		tmpWaypoint = tmpWaypoint->GetNextWP();
		TheHud->waypointNum ++;
	}
}

// JPO - just note it is launched.
void FireControlComputer::MissileLaunch()
{
    missileLaunched = 1;
    lastMissileShootTime = SimLibElapsedTime;
}

SimObjectType* FireControlComputer::Exec (SimObjectType* curTarget, SimObjectType* newList,
                                PilotInputs* theInputs)
{
//me123 overtake needs to be calgulated the same way in MissileClass::GetTOF
static const float	MISSILE_ALTITUDE_BONUS = 23.0f;	//me123 addet here and in // JB 010215 changed from 24 to 23
static const float	MISSILE_SPEED = 1500.0f; // JB 010215 changed from 1300 to 1500
   if (playerFCC &&
      ((AircraftClass*)platform)->mFaults->GetFault(FaultClass::fcc_fault))
   {
      SetTarget (NULL);
   }
   else
   {
       if (SimDriver.MotionOn())
       {
	   if (!targetPtr)
	   {
	       MissileImpactTimeFlash = 0; // cancel flashing
		   lastMissileImpactTime = 0;
		   lastmissileActiveTime = 0;
	   }
	   if (targetPtr && missileLaunched) {
	       lastMissileShootRng = targetPtr->localData->range;
	       lastMissileImpactTime = nextMissileImpactTime;
	       lastMissileShootHeight = Height;
	       lastMissileShootEnergy = (platform->Vt() * FTPSEC_TO_KNOTS - 150.0f)/2 ; // JB 010215 changed from 250 to 150
	   }
	   missileLaunched = 0;
	   if (lastMissileImpactTime > 0.0F  && targetPtr )
	   { //me123 addet this stuff

	       //this is the missiles approximate overtake
	       //missilespeed + altitude bonus + target closure
	       
	       float overtake = lastMissileShootEnergy + MISSILE_SPEED +(lastMissileShootHeight/1000.0f * MISSILE_ALTITUDE_BONUS) + targetspeed  * (float)cos(targetPtr->localData->ataFrom);
	       
	       //this is the predicted range from the missile to the target
	       lastMissileShootRng = lastMissileShootRng - (overtake / SimLibMajorFrameRate);
	       
	       lastMissileImpactTime =	max (0.0F, lastMissileShootRng/overtake);	// this is TOF.  Counting on silent failure of divid by 0.0 here...
	       lastMissileImpactTime += -5.0 * (float) sin(.07 * lastMissileImpactTime); // JB 010215
	       if (lastMissileImpactTime == 0.0f) { // JPO - trigger flashing X
		   // 8 seconds steady, 5 seconds flash
		   MissileImpactTimeFlash = SimLibElapsedTime + (5+8) * CampaignSeconds;
		   lastMissileShootRng = -1.0f; // reset for next
	       }
	       else MissileImpactTimeFlash = 0;
	   }
       }

      SetTarget(curTarget);
      targetList = newList;

      NavMode();
      switch (masterMode)
      {
         case Gun:
	     if (GetSubMode() == STRAF) {
		 AirGroundMode();
		 break;
	     }
         case ILS:
         case Nav:
         break;

         case Dogfight:
         case MissileOverride:
         case Missile:
            AirAirMode();
            lastCage = missileCageCmd;
         break;

         case AirGroundBomb:
            AirGroundMode();
         break;

         case AirGroundMissile:
         case AirGroundHARM:
            AirGroundMissileMode();
         break;

         case AirGroundLaser:
            TargetingPodMode();
         break;
      }

      lastDesignate = designateCmd;
   }

   return (targetPtr);
}

void FireControlComputer::SetMasterMode (FCCMasterMode newMode)
{
RadarClass* theRadar = (RadarClass*) FindSensor (platform, SensorClass::Radar);
FCCMasterMode oldMode;



if(playerFCC && (masterMode == Dogfight || masterMode == MissileOverride) &&
	newMode != MissileOverride && newMode != Dogfight&&
	Sms->curWeaponType == wtAim9|| Sms->curWeaponType == wtAim120) 
	{
	if ( masterMode == MissileOverride ) weaponMisOvrdMode = Sms->curWeaponType;
	else if ( masterMode == Dogfight ) weaponDogOvrdMode = Sms->curWeaponType;
	else weaponNoOvrdMode = Sms->curWeaponType;
	}


   // Nav only if Amux and Bmux failed, no change if FCC fail
	if(playerFCC &&
      (
       (
        ((AircraftClass*)platform)->mFaults->GetFault(FaultClass::fcc_fault)
       )
       ||
       (
        ((AircraftClass*)platform)->mFaults->GetFault(FaultClass::amux_fault) &&
        ((AircraftClass*)platform)->mFaults->GetFault(FaultClass::bmux_fault) &&
        newMode != Nav
       )
      )
     )
      return;

	// It has been stated (by Leon R) that changing modes while releasing weapons is bad, so...
	if (bombPickle) {
		return;
	}


   if (playerFCC && 
		 masterMode != Dogfight &&
       masterMode != MissileOverride &&
       masterMode != ClearOveride
	   )
   {
      lastMasterMode = masterMode;
      lastSubMode = subMode;
   }

		if (theRadar && masterMode == ClearOveride)
		{
		  theRadar->ClearOverride();
		}
   
   // Clear any holdouts from previous modes
   switch (masterMode)
   {
      case AirGroundHARM:
         ((AircraftClass*)platform)->SetTarget (NULL);
      break;
   }

   oldMode = masterMode;
   if (masterMode != Dogfight && masterMode != MissileOverride) masterMode = newMode;//me123

   switch (masterMode)
   {
      case Gun:
         // Clear out any previous targets we had locked.
         ClearCurrentTarget();
		 //MI remember our last setting
		 if(!g_bRealisticAvionics)
			 SetSubMode (EEGS);
		 else
			 SetSubMode(lastAAGunSubMode);
         if (TheHud && playerFCC)
            TheHud->headingPos = HudClass::Low;
      break;

      case Dogfight:
         // Clear out any non-air-to-air targets we had locked.
         if (oldMode != Dogfight && oldMode != Missile && oldMode != MissileOverride)
			 ClearCurrentTarget();

         postDrop = FALSE;
		 //MI changed so it remembers last gun submode too
		 if(!g_bRealisticAvionics)
			 SetSubMode (EEGS);
		 else
			 SetSubMode(lastDFGunSubMode);

		 // Sms->FindWeaponType(wtAim9);
		 //MI this remembering thing was kinda porked.....
		 if(dgftSubMode == Aim9) 
		 {
			 Sms->SetWeaponType (wtAim9);
			 if (!Sms->FindWeaponType(wtAim9))
			 {
				 if(Sms->FindWeaponType(wtAim120))
				 {
					 SetDgftSubMode(FireControlComputer::Aim120);
				 }
				 else
				 {
					 Sms->SetWeaponType (wtNone);
				 }
			 }
			 else
			 {
				 SetDgftSubMode(FireControlComputer::Aim9);
			 }
		 }
		 else 
		 {
			 Sms->SetWeaponType(wtAim120);
			 if(!Sms->FindWeaponType(wtAim120))
			 {
				 if(Sms->FindWeaponType (wtAim9))
				 {
					 SetDgftSubMode(FireControlComputer::Aim9);
				 }
				 else
				 {
					 Sms->SetWeaponType (wtNone);
					 //SetDgftSubMode(FireControlComputer::Aim120);
				 }
			 }
			 else
			 {
				 SetDgftSubMode(FireControlComputer::Aim120);
			 }
		 }
		 if (theRadar && oldMode != Dogfight)
		 { 
			 theRadar->SetSRMOverride();
		 } 
		 if (TheHud && playerFCC)
		 { 
			 TheHud->headingPos = HudClass::Low;
		 } 
	  break;
      case MissileOverride://me123 multi changes here 
         // Clear out any non-air-to-air targets we had locked.
         if (oldMode != Dogfight && oldMode != Missile && oldMode != MissileOverride)
			ClearCurrentTarget();
		 postDrop = FALSE;

		 if(!playerFCC && Sms->FindWeaponType(wtAim120))
		 {
			 Sms->FindWeaponType(wtAim120);
			 Sms->SetWeaponType (wtAim120);
			 SetSubMode (Aim120);
		 }
		  
		 else
		 {	
		     if (Sms->curWeaponDomain != wdAir &&
			 Sms->FindWeaponType(weaponMisOvrdMode))
			 Sms->SetWeaponType (weaponMisOvrdMode);
		     
		     if (Sms->curWeaponType == wtAim120) SetSubMode (Aim120);
		     else if (Sms->curWeaponType == wtAim9) SetSubMode (Aim9);
		     else 
		     {
			 if (Sms->FindWeaponType(wtAim120))
			 {
			     Sms->SetWeaponType (wtAim120);
			     SetSubMode (Aim120);
			 }
			 else  if (Sms->FindWeaponType(wtAim9))
			 {
			     Sms->SetWeaponType (wtAim9);
			     SetSubMode (Aim9);
			 }
			 else
			 {
			     Sms->SetWeaponType (wtNone);
			     SetSubMode (ETE);//me123
			 }
		     }
		}
		 
		 if (theRadar )
		 {
            theRadar->SetMRMOverride();
		 }
         if (TheHud && playerFCC)
            TheHud->headingPos = HudClass::Low;
      break;

      case Missile:
         // Clear out any non-air-to-air targets we had locked.
         if (oldMode != Dogfight && oldMode != MissileOverride)
			 ClearCurrentTarget();
         postDrop = FALSE;
         if (Sms->curWeaponClass != wcAimWpn)
         {
			if (playerFCC)
			{//select our weapon
			Sms->SetWeaponType (weaponNoOvrdMode);//me123 remember programed weapon
			if (Sms->curWeaponType == wtAim120) SetSubMode (Aim120);
			else if (Sms->curWeaponType == wtAim9) SetSubMode (Aim9);

			else //we don't have the weapon type selected, find another one
				{
				if (Sms->FindWeaponType(wtAim9))
				   SetSubMode (Aim9);
				else  if (Sms->FindWeaponType(wtAim120))//me123 addet
				   SetSubMode (Aim120);
				// go nav mode we don't have missiles
				else  SetSubMode (ETE);//me123
				}
			}

			else //no player
			{
            if (Sms->FindWeaponType(wtAim9))
               SetSubMode (Aim9);
			else  if (Sms->FindWeaponType(wtAim120))//me123 addet
               SetSubMode (Aim120);
			}
         }
         if (TheHud && playerFCC)
            TheHud->headingPos = HudClass::Low;
      break;

      case ILS:
         // Clear out any previous targets we had locked.
         ClearCurrentTarget();
         Sms->SetWeaponType (wtNone);
         Sms->FindWeaponClass (wcNoWpn);
         if (TheHud && playerFCC)
            TheHud->headingPos = HudClass::High;
			platform->SOIManager (SimVehicleClass::SOI_RADAR);
      break;

      case Nav:
         // Clear out any previous targets we had locked.
         strcpy (subModeString, "NAV");
         ClearCurrentTarget();
         Sms->SetWeaponType (wtNone);
         Sms->FindWeaponClass (wcNoWpn);
         SetSubMode (ETE);
         releaseConsent = FALSE;
         postDrop = FALSE;
         preDesignate = TRUE;
         postDrop = FALSE;
         bombPickle = FALSE;
         // Find currentwaypoint
 		   if (TheHud && playerFCC)
		   {
			   TheHud->headingPos = HudClass::Low;
		   }

			// SOI is RADAR in NAV
			platform->SOIManager (SimVehicleClass::SOI_RADAR);
      break;

      case AirGroundBomb:
         // Clear out any previous targets we had locked.
         ClearCurrentTarget();
         preDesignate = TRUE;
         postDrop = FALSE;
         inRange = TRUE;
		 // Marco Edit - this is where Rocket cycling occurs
         if (Sms->curWeaponClass != wcBombWpn && Sms->curWeaponClass != wcRocketWpn)
         {
            if (Sms->FindWeaponClass (wcBombWpn) && Sms->CurHardpoint() >= 0) // JB 010805 Possible CTD check curhardpoint
               Sms->SetWeaponType(Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponType());
			else
				Sms->SetWeaponType (wtNone);
         }
		 if (Sms->curWeaponClass != wcRocketWpn)
		 {
			if (lastAgSubMode == CCIP || lastAgSubMode == CCRP || lastAgSubMode == DTOSS || lastAgSubMode == LADD || lastAgSubMode == MAN) SetSubMode (lastAgSubMode); //me123 let's remember the agsubmode
			else SetSubMode (CCRP);
		 }

         if (TheHud && playerFCC)
            TheHud->headingPos = HudClass::High;
      break;

      case AirGroundMissile:
         // Clear out any previous targets we had locked.
         ClearCurrentTarget();
         preDesignate = TRUE;
         postDrop = FALSE;
         inRange = TRUE;
         missileTarget = FALSE;
         if (Sms->curWeaponClass != wcAgmWpn)
         {
            if (Sms->FindWeaponClass (wcAgmWpn) && Sms->CurHardpoint() >= 0) // JB 010805 Possible CTD check curhardpoint
            {
               Sms->SetWeaponType(Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponType());
               switch (Sms->curWeaponType)
               {
                  case wtAgm65:
					  // M.N. added full realism mode
                     if (PlayerOptions.GetAvionicsType() == ATRealistic || PlayerOptions.GetAvionicsType() == ATRealisticAV)
                        SetSubMode (BSGT);
                     else
                        SetSubMode (SLAVE);
                  break;
               }
            }
            else
            {
               Sms->SetWeaponType (wtNone);
            }
         }
         if (TheHud && playerFCC)
            TheHud->headingPos = HudClass::High;
      break;

      case AirGroundHARM:
         // Clear out any previous targets we had locked.
         ClearCurrentTarget();
         preDesignate = TRUE;
         postDrop = FALSE;
         if (Sms->curWeaponClass != wcHARMWpn)
         {
            if (Sms->FindWeaponClass (wcHARMWpn, FALSE) && Sms->CurHardpoint() >= 0) // JB 010805 Possible CTD check curhardpoint
            {
               Sms->SetWeaponType(Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponType());
               switch (Sms->curWeaponType)
               {
                  case wtAgm88:
                     SetSubMode (HTS);
                  break;
               }
            }
            else
            {
               Sms->SetWeaponType (wtNone);
            }
         }
         if (TheHud && playerFCC)
            TheHud->headingPos = HudClass::High;
      break;

      case AirGroundLaser:
         // Clear out any previous targets we had locked.
         ClearCurrentTarget();
         preDesignate = TRUE;
         postDrop = FALSE;
         if (Sms->curWeaponClass != wcGbuWpn)
         {
            if (Sms->FindWeaponClass (wcGbuWpn, FALSE) && Sms->CurHardpoint() >= 0) // JB 010805 Possible CTD check curhardpoint
            {
               Sms->SetWeaponType(Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponType());
               switch (Sms->curWeaponType)
               {
                  case wtGBU:
					  //Mi this isn't true... doc states you start off in SLAVE
					  if(!g_bRealisticAvionics)
					  {
						  if (PlayerOptions.GetAvionicsType() == ATRealistic)
							  SetSubMode (BSGT);
						  else
							  SetSubMode (SLAVE);
					  }
					  else
					  {
						  InhibitFire = FALSE;
						  // M.N. added full realism mode
						  if(PlayerOptions.GetAvionicsType() != ATRealistic && PlayerOptions.GetAvionicsType() != ATRealisticAV)
							  SetSubMode (BSGT);
						  else
							  SetSubMode (SLAVE);
					  } 
                  break;
               }
            }
            else
            {
               Sms->SetWeaponType (wtNone);
            }
         }
         if (TheHud && playerFCC)
            TheHud->headingPos = HudClass::High;
      break;

      case AirGroundCamera:
         // Clear out any previous targets we had locked.
         ClearCurrentTarget();
         preDesignate = TRUE;
         postDrop = FALSE;
         if (Sms->curWeaponClass != wcCamera)
         {
            if (Sms->FindWeaponClass (wcCamera, FALSE) && Sms->CurHardpoint() >= 0) // JB 010805 Possible CTD check curhardpoint
            {
               Sms->SetWeaponType(Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponType());
            }
            else
            {
               Sms->SetWeaponType (wtNone);
            }
         }
         if (TheHud && playerFCC)
            TheHud->headingPos = HudClass::High;
         strcpy (subModeString, "RPOD");
			platform->SOIManager (SimVehicleClass::SOI_RADAR);
      break;
   }
}

void FireControlComputer::SetSubMode (FCCSubMode newSubMode)
{
	if (newSubMode == CCRP && Sms && Sms->Ownship() && ((AircraftClass *)(Sms->Ownship()))->af &&
		!((AircraftClass *)Sms->Ownship())->af->IsSet(AirframeClass::IsDigital) && 
		platform && RadarDataTable[platform->GetRadarType()].NominalRange == 0.0) // JB 011018
	{
		newSubMode = CCIP;
	}

	// It has been stated (by Leon R) that changing modes while releasing weapons is bad, so...
	if (bombPickle) {
		return;
	}

   if (masterMode != Dogfight &&
       masterMode != MissileOverride)
   {
      lastSubMode = subMode;
   }

   if (lastSubMode == BSGT ||
	   lastSubMode == SLAVE ||
      lastSubMode == HTS)
   {
       platform->SOIManager (SimVehicleClass::SOI_RADAR);
   }

   subMode = newSubMode;
   switch (subMode)
   {
      case SAM:
         strcpy (subModeString, "SAM");
         Sms->SetWeaponType (wtNone);
         Sms->FindWeaponClass (wcSamWpn);
      break;

      case Aim9:
         strcpy (subModeString, "SRM");
         if (Sms->curWeaponType != wtAim9)
         {
            Sms->SetWeaponType (wtAim9);
            if (!Sms->FindWeaponType (wtAim9))
               Sms->SetWeaponType (wtNone);
//               NextSubMode();
         }
	 // gross hack... default digital is aim9 mode, so aim9's are always cold by the time you see them
     // Marco Edit - Auto cooling if in Master Arm only
		 
		 if(Sms && Sms->Ownship() && ((AircraftClass *)Sms->Ownship())->AutopilotType() == AircraftClass::CombatAP  ) 
		 {
			 if(Sms->GetCoolState() == SMSClass::WARM && Sms->MasterArm() == SMSClass::Arm)
			 { // JPO aim9 cooling
				 //Sms->aim9cooltime = SimLibElapsedTime + 3 * CampaignSeconds; // in 3 seconds
				 Sms->SetCoolState(SMSClass::COOLING);
			 }
			 /*if (((AircraftClass *)Sms->Ownship())->af->IsSet(AirframeClass::IsDigital))
			 {
				 Sms->SetCoolState(SMSClass::COOL);
			 }*/
		 }
	 /*if (!((AircraftClass *)Sms->Ownship())->af->IsSet(AirframeClass::IsDigital) &&
		 Sms->GetCoolState() == SMSClass::WARM && Sms->MasterArm() == SMSClass::Arm) 
	 { // JPO aim9 cooling
	     Sms->aim9cooltime = SimLibElapsedTime + 3 * CampaignSeconds; // in 3 seconds
	     Sms->SetCoolState(SMSClass::COOLING);
	 }
	 if (((AircraftClass *)Sms->Ownship())->af->IsSet(AirframeClass::IsDigital))
	 {
		Sms->SetCoolState(SMSClass::COOL);
	 }*/

         platform->SOIManager (SimVehicleClass::SOI_RADAR);
      break;

      case Aim120:
         strcpy (subModeString, "MRM");
         if (Sms->curWeaponType != wtAim120)
         {
            Sms->SetWeaponType (wtAim120);
            if (!Sms->FindWeaponType (wtAim120))
				 Sms->SetWeaponType (wtNone);
//               NextSubMode();
         }
         platform->SOIManager (SimVehicleClass::SOI_RADAR);
      break;

      case EEGS:
         strcpy (subModeString, "EEGS");
         if (masterMode != Dogfight)
         {
            Sms->SetWeaponType (wtGuns);
            if ( !Sms->FindWeaponClass (wcGunWpn))
               Sms->SetWeaponType (wtNone);
//               NextSubMode();
         }
         platform->SOIManager (SimVehicleClass::SOI_RADAR);
      break;

      case LCOS:
         strcpy (subModeString, "LCOS");
         if (masterMode != Dogfight)
         {
	     Sms->SetWeaponType (wtGuns);
	     if (!Sms->FindWeaponClass (wcGunWpn))
		 Sms->SetWeaponType (wtNone);
	     //            NextSubMode();
	 }
         platform->SOIManager (SimVehicleClass::SOI_RADAR);
      break;

      case Snapshot:
         strcpy (subModeString, "SNAP");
	 if (masterMode != Dogfight)
         {
	     
	     Sms->SetWeaponType (wtGuns);
	     if (!Sms->FindWeaponClass (wcGunWpn))
		 Sms->SetWeaponType (wtNone);
	     //            NextSubMode();
	 }
         platform->SOIManager (SimVehicleClass::SOI_RADAR);
      break;

      case CCIP:
         preDesignate = TRUE;
         
         if (!Sms->FindWeaponClass (wcBombWpn))
         {
            Sms->SetWeaponType (wtNone);
//            NextSubMode();
         }
         else if (Sms->CurHardpoint() >= 0) // JB 010805 Possible CTD check curhardpoint
		 {
			Sms->SetWeaponType(Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponType());
			strcpy (subModeString, "CCIP");//me123 moved so we don't write this with no bombs
		 }
         platform->SOIManager (SimVehicleClass::SOI_RADAR);
      break;

      case CCRP:
         preDesignate = FALSE;

         if (!Sms->FindWeaponClass (wcBombWpn))
         {
            Sms->SetWeaponType (wtNone);
//            NextSubMode();
         }
         else if (Sms->CurHardpoint() >= 0) // JB 010805 Possible CTD check curhardpoint
		 {
			Sms->SetWeaponType(Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponType());
			strcpy (subModeString, "CCRP");//me123 moved so we don't write this with no bombs
		 }


         platform->SOIManager (SimVehicleClass::SOI_RADAR);
// 2001-04-18 ADDED BY S.G. I'LL SET MY RANDOM NUMBER FOR CCRP BOMBING INNACURACY NOW
		 // Since autoTarget is a byte :-(  I'm limiting to just use the same value for both x and y offset :-(
//		 autoTarget = (rand() & 0x3f) - 32; // In RP5, I'm limited to the variables I can use
		 xBombAccuracy = (rand() & 0x3f) - 32;
		 yBombAccuracy = (rand() & 0x3f) - 32;
      break;

      case DTOSS:
         preDesignate = TRUE;
         groundPipperAz = 0.0F;
         groundPipperEl = 0.0F;

         platform->SOIManager (SimVehicleClass::SOI_HUD);
         if (!Sms->FindWeaponClass (wcBombWpn))
         {
            Sms->SetWeaponType (wtNone);
//            NextSubMode();
         }
         else if (Sms->CurHardpoint() >= 0) // JB 010805 Possible CTD check curhardpoint
		 {
			strcpy (subModeString, "DTOS");//me123 moved so we don't write this with no bombs
            Sms->SetWeaponType(Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponType());
		 }
      break;

      case LADD:
         preDesignate = FALSE;
         groundPipperAz = 0.0F;
         groundPipperEl = 0.0F;

         platform->SOIManager (SimVehicleClass::SOI_RADAR);
         if (!Sms->FindWeaponClass (wcBombWpn))
         {
            Sms->SetWeaponType (wtNone);
//            NextSubMode();
         }
         else if (Sms->CurHardpoint() >= 0) // JB 010805 Possible CTD check curhardpoint
		 {
			strcpy (subModeString, "LADD");//me123 moved so we don't write this with no bombs
            Sms->SetWeaponType(Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponType());
		 }
      break;

      case MAN: // JPO
         preDesignate = TRUE;
         strcpy (subModeString, "MAN");
         if (!Sms->FindWeaponClass (wcBombWpn))
         {
            Sms->SetWeaponType (wtNone);
//            NextSubMode();
         }
         else if (Sms->CurHardpoint() >= 0) // JB 010805 Possible CTD check curhardpoint
            Sms->SetWeaponType(Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponType());
         platform->SOIManager (SimVehicleClass::SOI_RADAR);
      break;

      case RCKT:
         preDesignate = TRUE;
         strcpy (subModeString, "RCKT");
         // if (!Sms->FindWeaponClass (wcRocketWpn))
		 if (Sms->GetCurrentWeaponHardpoint() >= 0 && !Sms->hardPoint[Sms->GetCurrentWeaponHardpoint()]->weaponPointer)
         {
            Sms->SetWeaponType (wtNone);
//            NextSubMode();
         }
         else if (Sms->CurHardpoint() >= 0) // JB 010805 Possible CTD check curhardpoint
            Sms->SetWeaponType(Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponType());
         platform->SOIManager (SimVehicleClass::SOI_RADAR);
      break;

      case STRAF:
         preDesignate = TRUE;
         Sms->SetWeaponType (wtGuns);
         strcpy (subModeString, "STRF");
         if (!Sms->FindWeaponClass (wcGunWpn))
            Sms->SetWeaponType (wtNone);
//            NextSubMode();
         platform->SOIManager (SimVehicleClass::SOI_RADAR);
      break;

      case BSGT:
         preDesignate = TRUE;
         groundPipperAz = 0.0F;
         groundPipperEl = 0.0F;
         strcpy (subModeString, "BSGT");
//         if (!Sms->curWeapon)
//            Sms->FindWeaponClass (wcAgmWpn);
         platform->SOIManager (SimVehicleClass::SOI_HUD);
      break;

      case SLAVE:
         preDesignate = TRUE;
         groundPipperAz = 0.0F;
         groundPipperEl = 0.0F;
         strcpy (subModeString, "SLAV");
//         if (!Sms->curWeapon)
//            Sms->FindWeaponClass (wcAgmWpn);
         platform->SOIManager (SimVehicleClass::SOI_RADAR);
      break;

      case HTS:
         preDesignate = TRUE;
         groundPipperAz = 0.0F;
         groundPipperEl = 0.0F;
         strcpy (subModeString, "HTS");
         // if (Sms->FindWeaponClass (wcHARMWpn, FALSE))
	 // Marco edit - for cycling bug
	 if (Sms->GetCurrentWeaponHardpoint() >= 0  && // JPO CTD fix
			Sms->CurHardpoint() >= 0 && // JB 010805 Possible CTD check curhardpoint
	     Sms->hardPoint[Sms->GetCurrentWeaponHardpoint()] != NULL)
	     //Sms->hardPoint[Sms->GetCurrentWeaponHardpoint()]->weaponPointer) // JB 010726 why is a weaponPointer required?
         {
	     Sms->SetWeaponType(Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponType());
	     platform->SOIManager (SimVehicleClass::SOI_WEAPON);
         }
         else
	     Sms->SetWeaponType (wtNone);
      break;

      case TargetingPod:
         preDesignate = TRUE;
         groundPipperAz = 0.0F;
         groundPipperEl = 0.0F;
         strcpy (subModeString, "GBU");
         // if (Sms->FindWeaponClass (wcGbuWpn, FALSE))
		 // Marco Edit - for cycling bug
		 if (Sms->GetCurrentWeaponHardpoint() >= 0  && // JPO CTD fix
			 Sms->CurHardpoint() >= 0 && // JB 010805 Possible CTD check curhardpoint
	     Sms->hardPoint[Sms->GetCurrentWeaponHardpoint()] != NULL &&
		     Sms->hardPoint[Sms->GetCurrentWeaponHardpoint()]->weaponPointer)
            Sms->SetWeaponType(Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponType());
         else
            Sms->SetWeaponType (wtNone);
         platform->SOIManager (SimVehicleClass::SOI_RADAR);
      break;

	  //MI
	  //case GPS:
	  //break;

      case TimeToGo:
      case ETE:
      case ETA:
         platform->SOIManager (SimVehicleClass::SOI_RADAR);
      break;
   }

   // Make sure string is correct in override modes
   switch (masterMode)
   {
      case Dogfight:
         strcpy (subModeString, "DGFT");
      break;

      case MissileOverride:
         strcpy (subModeString, "MSL");
      break;
   }
}

void FireControlComputer::ClearOverrideMode (void)
{
	FCCSubMode tmpSub = lastSubMode;

	if ((GetMasterMode() == Dogfight) ||
		(GetMasterMode() == MissileOverride)) {
		masterMode = ClearOveride;//me123 to allow leaving an overide mode
		SetMasterMode (lastMasterMode);
		SetSubMode (tmpSub);

	}
}


void FireControlComputer::NextSubMode (void)
{
	//MI
	RadarDopplerClass* pradar = (RadarDopplerClass*) FindSensor (platform, SensorClass::Radar);
   switch (masterMode)
   {
      case Gun:
         switch (subMode)
         {
            case EEGS:
				if (PlayerOptions.GetAvionicsType() != ATEasy) {
	               SetSubMode (LCOS);
				}
            break;

            case LCOS:
               SetSubMode (Snapshot);
            break;

            case Snapshot:
               SetSubMode (EEGS);
            break;
         }
      break;

      case Missile:
         switch (subMode)
         {
            case Aim9:
               if (Sms->FindWeaponType(wtAim120))
                  SetSubMode (Aim120);
            break;

            case Aim120:
               if (Sms->FindWeaponType(wtAim9))
                  SetSubMode (Aim9);
            break;
         }
      break;

      case Nav:
         switch (subMode)
         {
            case ETE:
				if (PlayerOptions.GetAvionicsType() != ATEasy)
            {
               SetSubMode (TimeToGo);
            }
            break;

            case TimeToGo:
	               SetSubMode (ETA);
            break;

            case ETA:
               SetSubMode (ETE);
            break;
         }
      break;

      case AirGroundBomb:
		  // Marco edit - don't want to set CCIP/CCRP/DTOS with rockets selected
		 if (Sms->CurHardpoint() < 0 || // JPO - just in case.
		     Sms->hardPoint[Sms->CurHardpoint()] == NULL ||
		     Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponType() != wtLAU)
		 {
			switch (subMode)
			{
	            case CCIP:
					if (PlayerOptions.GetAvionicsType() != ATEasy) {
						SetSubMode (DTOSS);
						lastAgSubMode = DTOSS;//me123
						//MI Set our SOI
						platform->SOIManager(SimVehicleClass::SOI_HUD);
					}
				break;
	
				case CCRP:
					SetSubMode (CCIP);
					lastAgSubMode = CCIP;//me123
					//MI Set our SOI
					platform->SOIManager(SimVehicleClass::SOI_HUD);
				break;

				case DTOSS:
					SetSubMode (CCRP);
					lastAgSubMode = CCRP;//me123
					//MI
					if(g_bRealisticAvionics && pradar)
					{
						pradar->SetScanDir(1.0F);
						pradar->SelectLastAGMode();
					}
					//MI Set our SOI
					platform->SOIManager(SimVehicleClass::SOI_RADAR);
				break;
				default: // catch LADD MAN etc
					SetSubMode(CCRP);
					lastAgSubMode = CCRP;
				break;
			}
		 }
      break;

      case AirGroundMissile:
      case AirGroundLaser:
		  //MI
		  if(masterMode == AirGroundLaser && g_bRealisticAvionics)
		  {
			  SetSubMode(SLAVE);
			  break;
		  }
         switch (subMode)
         {
            case SLAVE:
				if (PlayerOptions.GetAvionicsType() != ATEasy) {
					SetSubMode (BSGT);
				}
            break;

            case BSGT:
               SetSubMode (SLAVE);
            break;
         }
      break;
   }
}


void FireControlComputer::WeaponStep(void)
{
	//MI
	RadarDopplerClass* pradar = (RadarDopplerClass*) FindSensor (platform, SensorClass::Radar);
	switch (masterMode)
	{
	  case Dogfight:
	  case MissileOverride:
	  case Missile:
	  case AirGroundMissile:
	  case AirGroundHARM:
        Sms->WeaponStep();
		if(pradar && (masterMode == AirGroundMissile || masterMode == AirGroundHARM))	//MI fix
		{
			pradar->SetScanDir(1.0F);
			pradar->SelectLastAGMode();
		}
		break;
		
	  case AirGroundBomb:
		// Marco edit - don't want to set CCIP/CCRP/DTOS with rockets selected
			//me123 ctd fix ..addet Sms->curWeapon  check
		if (Sms->curWeapon && Sms->CurHardpoint() >= 0 && Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponType() != wtLAU) // JB 010805 Possible CTD check curhardpoint
		{
			switch (subMode)
			{
			case CCIP:
				SetSubMode (DTOSS);
				lastAgSubMode = DTOSS;//me123
				//MI Set our SOI
				platform->SOIManager(SimVehicleClass::SOI_HUD);
				if(Sms)
				{
					if(Sms->Prof1)
						Sms->Prof1SubMode = DTOSS;
					else
						Sms->Prof2SubMode = DTOSS;
				}
				break;	  
			case CCRP:
				SetSubMode (CCIP);
				lastAgSubMode = CCIP;//me123
				//MI Set our SOI
				platform->SOIManager(SimVehicleClass::SOI_HUD);
				if(Sms)
				{
					if(Sms->Prof1)
						Sms->Prof1SubMode = CCIP;
					else
						Sms->Prof2SubMode = CCIP;
				}
				break;
			case DTOSS:				
				SetSubMode (CCRP);//me123 from CCRP
				lastAgSubMode = CCRP;//me123
				if(pradar)	//MI fix
					pradar->SetScanDir(1.0F);
				//MI Set our SOI
				platform->SOIManager(SimVehicleClass::SOI_RADAR);
				if(Sms)
				{
					if(Sms->Prof1)
						Sms->Prof1SubMode = CCRP;
					else
						Sms->Prof2SubMode = CCRP;
				}
				pradar->SetScanDir(1.0F);
				pradar->SelectLastAGMode();
				break;
			default: // catch LADD MAN etc
				SetSubMode(CCRP);
				lastAgSubMode = CCRP;
				if(pradar)	//MI fix
					pradar->SetScanDir(1.0F);
				//MI Set our SOI
				platform->SOIManager(SimVehicleClass::SOI_RADAR);
				if(Sms)
				{
					if(Sms->Prof1)
						Sms->Prof1SubMode = CCRP;
					else
						Sms->Prof2SubMode = CCRP;
				}
				break;
			}
		}
		break;
	}
}


SimObjectType* FireControlComputer::TargetStep (SimObjectType* startObject, int checkFeature)
{
VuListIterator featureWalker (SimDriver.featureList);
VuEntity* testObject = NULL;
VuEntity* groundTarget = NULL;
SimObjectType* curObject = NULL;
SimObjectType* retObject = NULL;
float angOff;

   // Starting in the object list
   if (startObject == NULL || startObject != targetPtr)
   {
      // Start at next and go on
      if (startObject)
      {
         curObject = startObject->next;
         while (curObject)
         {
            if (curObject->localData->ata < 60.0F * DTR)
            {
               retObject = curObject;
               break;
            }
            curObject = curObject->next;
         }
      }

      // Did we go off the End of the objects?
      if (!retObject && checkFeature)
      {
         // Check features
         testObject = featureWalker.GetFirst();
         while (testObject)
         {
            angOff = (float)atan2 (testObject->YPos() - platform->YPos(),
               testObject->XPos() - platform->XPos()) - platform->Yaw();
            if (fabs(angOff) < 60.0F * DTR)
            {
               break;
            }
            testObject = featureWalker.GetNext();
         }

         if (testObject)
         {
			groundTarget = testObject;
			// KCK NOTE: Uh.. why are we doing this?
//			if (retObject)
//				{
//              Tpoint pos;
//	            targetPtr->BaseData()->drawPointer->GetPosition (&pos);
//		        targetPtr->BaseData()->SetPosition (pos.x, pos.y, pos.z);
//				}
            groundDesignateX = testObject->XPos();
            groundDesignateY = testObject->YPos();
            groundDesignateZ = testObject->ZPos();
         }
      }

      // Did we go off the end of the Features?
      if (!retObject && !groundTarget)
      {
         // Check the head of the object list
         curObject = targetList;
         if (curObject)
         {
            if (curObject->localData->ata < 60.0F * DTR)
            {
               retObject = curObject;
            }
            else
            {
               while (curObject != startObject)
               {
                  curObject = curObject->next;
                  if (curObject && curObject->localData->ata < 60.0F * DTR)
                  {
                     retObject = curObject;
                     break;
                  }
               }
            }
         }
      }
   }
   else if (startObject == targetPtr)
   {
      // Find the current object
      if (checkFeature)
      {
         testObject = featureWalker.GetFirst();
		 // Iterate up to our position in the list.
         while (testObject && testObject != targetPtr->BaseData())
            testObject = featureWalker.GetNext();
		 // And then get the next object
         if (testObject)
            testObject = featureWalker.GetNext();

         // Is there anything after the current object?
         while (testObject)
         {
            angOff = (float)atan2 (testObject->YPos() - platform->YPos(),
               testObject->XPos() - platform->XPos()) - platform->Yaw();
            if (fabs(angOff) < 60.0F * DTR)
            {
               break;
            }
            testObject = featureWalker.GetNext();
         }

         // Found one, so use it
         if (testObject)
         {
			groundTarget = testObject;
			// KCK: Why are we doing this?
//			Tpoint pos;
//          targetPtr->BaseData()->drawPointer->GetPosition (&pos);
//          targetPtr->BaseData()->SetPosition (pos.x, pos.y, pos.z);
            groundDesignateX = testObject->XPos();
            groundDesignateY = testObject->YPos();
            groundDesignateZ = testObject->ZPos();
         }
      }

      // Off the end of the feature list?
      if (!retObject && !groundTarget)
      {
         // Check the head of the object list
         curObject = targetList;
         while (curObject)
         {
            if (curObject->localData->ata < 60.0F * DTR)
            {
               retObject = curObject;
               break;
            }
            curObject = curObject->next;
         }
      }

      // Of the End of the object list ?
      if (!retObject && checkFeature && !groundTarget)
      {
         // Check features
         testObject = featureWalker.GetFirst();
         while (testObject && testObject != targetPtr->BaseData())
         {
            angOff = (float)atan2 (testObject->YPos() - platform->YPos(),
               testObject->XPos() - platform->XPos()) - platform->Yaw();
            if (fabs(angOff) < 60.0F * DTR)
            {
               break;
            }
            testObject = featureWalker.GetNext();
         }

         if (testObject)
         {
			groundTarget = testObject;
            groundDesignateX = testObject->XPos();
            groundDesignateY = testObject->YPos();
            groundDesignateZ = testObject->ZPos();
         }
      }
   }

   if (groundTarget)
	   {
	   // We're targeting a feature thing - make a new SimObjectType
	   #ifdef DEBUG
	   retObject = new SimObjectType(OBJ_TAG, platform, (SimBaseClass*)groundTarget);
	   #else
	   retObject = new SimObjectType((SimBaseClass*)groundTarget);
	   #endif
       retObject->localData->ataFrom = 180.0F * DTR;
	   }
   SetTarget(retObject);

   return retObject;      
}

void FireControlComputer::ClearCurrentTarget (void)
	{
	if (targetPtr)
		targetPtr->Release( SIM_OBJ_REF_ARGS );
	targetPtr = NULL;
	}

void FireControlComputer::SetTarget (SimObjectType* newTarget)
	{
	if (newTarget == targetPtr)
		return;

	ClearCurrentTarget();
	if (newTarget)
	{
        ShiAssert( newTarget->BaseData() != (FalconEntity*)0xDDDDDDDD );
		newTarget->Reference( SIM_OBJ_REF_ARGS );
	}

	targetPtr = newTarget;
	}

void FireControlComputer::DisplayInit (ImageBuffer* image)
{
   DisplayExit();

   privateDisplay = new Render2D;
   ((Render2D*)privateDisplay)->Setup (image);

   privateDisplay->SetColor (0xff00ff00);
}

void FireControlComputer::Display (VirtualDisplay* newDisplay)
{
   display = newDisplay;

   // JPO intercept for now FCC power...
   if (!((AircraftClass*)platform)->HasPower(AircraftClass::FCCPower)) {
       BottomRow();
       display->TextCenter(0.0f, 0.2f, "FCC");
       int ofont = display->CurFont();
       display->SetFont(2);
       display->TextCenterVertical (0.0f, 0.0f, "OFF");
       display->SetFont(ofont);
       return;
   }
   NavDisplay();
}

void FireControlComputer::PushButton(int whichButton, int whichMFD)
{
    ShiAssert(whichButton < 20);
    ShiAssert(whichMFD < 4);

    if (IsHsdState(HSDCNTL)) 
	{
		if (hsdcntlcfg[whichButton].mode != HSDNONE) 
		{
			ToggleHsdState(hsdcntlcfg[whichButton].mode);
			return;
		}
	}

	//MI
	if(g_bRealisticAvionics)
	{
		if(SimDriver.playerEntity)
		{
			if(IsSOI && (whichButton >=11 && whichButton <= 13))
				platform->StepSOI(2);
		}
	}
    switch (whichButton)
    {
    case 0: // DEP - JPO
	if (g_bRealisticAvionics) {
	    ToggleHsdState (HSDCEN);
	}
	break;
    case 1: // DCPL - JPO
	if (g_bRealisticAvionics) {
	    ToggleHsdState (HSDCPL);
	}
	break;
	//MI
	case 2:
		if(g_bRealisticAvionics)
			ToggleHSDZoom();
	break;
    case 4: // CTRL
	if (g_bRealisticAvionics) {
	    ToggleHsdState (HSDCNTL);
	}
	break;
    case 6: // FRZ - JPO
	if (g_bRealisticAvionics) {
	    frz_x = platform->XPos();
	    frz_y = platform->YPos();
	    frz_dir = platform->Yaw();
	    ToggleHsdState(HSDFRZ);
	}
	break;
    case 10: 
	if (g_bRealisticAvionics) {
	    MfdDrawable::PushButton(whichButton, whichMFD);
	}
	break;
    case 11: // SMS
	if (g_bRealisticAvionics) 
	    MfdDrawable::PushButton(whichButton, whichMFD);
	else
	    MfdDisplay[whichMFD]->SetNewMode(MFDClass::SMSMode);
	break;
    case 12: // jpo 
	if (g_bRealisticAvionics) 
	    MfdDrawable::PushButton(whichButton, whichMFD);
	break;

    case 13: // HSD
	if (g_bRealisticAvionics) 
	    MfdDrawable::PushButton(whichButton, whichMFD);
	else
	    MfdDisplay[whichMFD]->SetNewMode(MFDClass::MfdMenu);
	break;
	
    case 14: // SWAP
	if (g_bRealisticAvionics) 
	    MfdDrawable::PushButton(whichButton, whichMFD);
	else
	    MFDSwapDisplays();
	break;
	
    case 18: // Down
		if(!g_bRealisticAvionics)
			SimHSDRangeStepDown (0, KEY_DOWN, NULL);
		else
		{
			//MI
			if(HSDZoom == 0)
				SimHSDRangeStepDown (0, KEY_DOWN, NULL);
		}		
	break;
	
    case 19: // UP
		if(!g_bRealisticAvionics)
			SimHSDRangeStepUp (0, KEY_DOWN, NULL);
		else
		{
			//MI
			if(HSDZoom == 0)
				SimHSDRangeStepUp (0, KEY_DOWN, NULL);
		}	
	break;
    }
}

// STUFF Copied from Harm - now merged. JPO
// JPO
// This routine builds the initial list of preplanned targets.
// this will remain unchanged if there is no dlink/jstar access
void FireControlComputer::BuildPrePlanned()
{
    FlightClass* theFlight = (FlightClass*)(platform->GetCampaignObject());
    FalconPrivateList* knownEmmitters = NULL;
    GroundListElement* tmpElement;
    GroundListElement* curElement = NULL;
    FalconEntity* eHeader;
    
    // this is all based around waypoints.
    if (SimDriver.RunningCampaignOrTactical() && theFlight)
	knownEmmitters = theFlight->GetKnownEmitters();
    
    if (knownEmmitters)
    {
	VuListIterator elementWalker (knownEmmitters);
	
	eHeader = (FalconEntity*)elementWalker.GetFirst();
	while (eHeader)
	{
	    tmpElement = new GroundListElement (eHeader);
	    
	    if (grndlist == NULL)
		grndlist = tmpElement;
	    else
		curElement->next = tmpElement;
	    curElement = tmpElement;
	    eHeader = (FalconEntity*)elementWalker.GetNext();
	}
	
	knownEmmitters->DeInit();
	delete knownEmmitters;
    }
    nextDlUpdate = SimLibElapsedTime + CampaignSeconds * DATALINK_CYCLE;
}

// update - every so often from a JSTAR platform if available.
void FireControlComputer::UpdatePlanned()
{
    if (nextDlUpdate < SimLibElapsedTime)
	return;
    nextDlUpdate = SimLibElapsedTime + CampaignSeconds * DATALINK_CYCLE;
    if (((AircraftClass*)platform)->mFaults->GetFault(FaultClass::dlnk_fault) ||
	!((AircraftClass*)platform)->HasPower(AircraftClass::DLPower))
	return;

    FlightClass* theFlight = (FlightClass*)(platform->GetCampaignObject());
    if (!theFlight) return;
    // see if we have a jstar.
    Flight jstar = theFlight->GetJSTARFlight();
    if (!jstar) {
	PruneList ();
	return;
    }
    // check distance from Jstar.
    float myx = platform->XPos();
    float myy = platform->YPos();
    float xdist = jstar->XPos() - myx;
    float ydist = jstar->YPos() - myy;
    if (xdist * xdist + ydist*ydist < MAXJSTARRANGESQ) {
	PruneList ();
	return; // out of range
    }

    // completely new list please
    ClearPlanned();

    CampEntity		e;
    GroundListElement* tmpElement;
    GroundListElement* curElement = NULL;
    VuListIterator	myit(EmitterList);
    Team us = theFlight->GetTeam();

    for (e = (CampEntity) myit.GetFirst(); e; e = (Unit) myit.GetNext()) {
	if (e->GetTeam() != us && e->GetSpotted(us) && 
	    (!e->IsUnit() || !((Unit)e)->Moving())  && e->GetElectronicDetectionRange(Air))
	{
	    float ex = e -> XPos();
	    float ey = e -> YPos();
	    if (Distance(ex,ey,myx,myy) * FT_TO_NM < EMITTERRANGE)
	    {
		tmpElement = new GroundListElement (e);
		tmpElement->SetFlag(GroundListElement::DataLink);
		if (grndlist == NULL)
		    grndlist = tmpElement;
		else
		    curElement->next = tmpElement;
		curElement = tmpElement;
	    }
	}
    }
    
}

// every so often - remove dead targets
void FireControlComputer::PruneList()
{
    GroundListElement  **gpp;

    for (gpp = &grndlist; *gpp; ) {
	if ((*gpp)->BaseObject() == NULL) { // delete this one
	    GroundListElement *gp = *gpp;
	    *gpp = gp -> next;
	    delete gp;
	}
	else gpp = &(*gpp)->next;
    }
}

GroundListElement::GroundListElement(FalconEntity* newEntity)
{
	F4Assert (newEntity);

	baseObject	= newEntity;
	VuReferenceEntity(newEntity);
	symbol		= RadarDataTable[newEntity->GetRadarType()].RWRsymbol;
	if (newEntity->IsCampaign())
	    range	= ((CampBaseClass*)newEntity)->GetAproxWeaponRange(Air);
	else range	= 0;
	flags		= RangeRing;
	next		= NULL;
	lastHit		= SimLibElapsedTime;
}

GroundListElement::~GroundListElement()
{
    VuDeReferenceEntity(baseObject);
}

void GroundListElement::HandoffBaseObject()
{
    FalconEntity	*newBase;
    
    if (baseObject == NULL) return;

    newBase = SimCampHandoff( baseObject, HANDOFF_RADAR);
    
    if (newBase != baseObject) {
	VuDeReferenceEntity(baseObject);
	
	baseObject = newBase;
	
	if (baseObject) {
	    VuReferenceEntity(baseObject);
	}
    }
}

void FireControlComputer::ClearPlanned()
{
    GroundListElement* tmpElement;
    
    while (grndlist)
    {
	tmpElement = grndlist;
	grndlist = tmpElement->next;
	delete tmpElement;
    }
}

MASTERMODES FireControlComputer::GetMainMasterMode()
{
    switch (masterMode) {
    case Missile:
	return MM_AA;
    case ILS:
    case Nav:
    default:
	return MM_NAV;
    case AirGroundBomb:
    case AirGroundMissile:
    case AirGroundHARM:
    case AirGroundLaser:
	return MM_AG;
    case Gun:
	if (subMode == STRAF)
	    return MM_AG;
	else return MM_AA;
    case Dogfight:
	return MM_DGFT;
    case MissileOverride:
	return MM_MSL;
    }
}

int FireControlComputer::LastMissileWillMiss(float range)
{
/*	if (lastMissileImpactTime >0 && range > missileRMax )
   {
        return 1;
   }
	else return 0;
  */  //me123 if the predicted total TOF is over xx seconds the missile is considered out of energy

   if (lastMissileImpactTime >0 && //me123 let's make sure there is a missile in the air
       lastMissileImpactTime - ((lastMissileShootTime - SimLibElapsedTime) /1000.0f)  >= 80.0F)
       return 1;
   return 0;
}

float FireControlComputer::Aim120ASECRadius(float range)
{
    float asecradius = 0.6f;
    static const float bestmaxrange = 0.8f; // upper bound
    if (!g_bRealisticAvionics) return asecradius;

    if (range > bestmaxrange*missileRMax) 
	{ // above best range
	float dr = (range - bestmaxrange*missileRMax) / (0.2f*missileRMax);
	dr = 1.0 - dr;
	asecradius *= dr;
    }
    else if (range < (missileRneMax - missileRneMin)/2.0f) 
	{
	float dr = (range - missileRMin);
	dr /= (missileRneMax - missileRneMin)/2.0f - missileRMin;
	asecradius *= dr;
    }
	//MI make the size dependant on missile mode
	if(Sms && Sms->curWeapon && ((MissileClass*)Sms->curWeapon)->isSlave)
		asecradius = max(min(0.3f, asecradius), 0.1);
	else
		asecradius = max(min(0.6f, asecradius), 0.1);
    return asecradius;
}
