#include "stdhdr.h"
#include "aircrft.h"
#include "PilotInputs.h"
#include "digi.h"
#include "airframe.h"
#include "fcc.h"
#include "sms.h"
#include "simio.h"
#include "playerOp.h"
#include "Object.h"
#include "fakerand.h"
#include "camp2sim.h"
#include "fack.h"
#include "ffeedbk.h"
#include "sinput.h"
#include "falcsess.h"
#include "otwdrive.h"
//MI added
#include "cpmanager.h"
#include "cphsi.h"
#include "dofsnswitches.h"
#include "lantirn.h"

extern int narrowFOV;
extern BOOL	playerFlightModelHack;
extern WeaponType playerLastWeaponType;
extern WeaponClass playerLastWeaponClass;
extern FireControlComputer::FCCMasterMode playerLastMasterMode;
extern FireControlComputer::FCCSubMode playerLastSubMode;

extern bool g_bNewDamageEffects;	// JB 000814
extern bool g_bDisableFunkyChicken; // JB 000820
extern int g_nMaxDebugLabel; // JB 020316

extern bool g_bRealisticAvionics;	//MI
extern bool g_bINS;	//MI

#define DEBUGLABEL
#ifdef DEBUGLABEL
#include "Graphics\include\drawbsp.h"
extern int g_nShowDebugLabels;
#endif

//MI
extern bool g_bTFRFixes;

void AircraftClass::GatherInputs(void)
{
	if(!HasPilot() || strength < 0.0F)
	{
      // Let it down
      af->SetSimpleMode(SIMPLE_MODE_OFF);
//      af->SetFlag(AirframeClass::SuperSimple);

		// bzzzt, thanks for playing.
       fireGun = FALSE;
       FCC->releaseConsent = FALSE;

	   // hm, about we just try pegging some values...
	   af->pstick = 0.1F;

      af->rstick = (180.0F * DTR - af->phi) * 0.01F;

	   af->ypedal = 0.0F;

      af->throtl = 0.0F;
   }
   else if (autopilotType != APOff)
   {
      // No autopilot for ownship if broken
      if (!IsSetFlag(MOTION_OWNSHIP) ||
         !(mFaults && mFaults->GetFault(FaultClass::flcs_fault) == FaultClass::a_p))
      {
         switch (autopilotType)
         {
		 case LantirnAP: // JPO - lantirn style autopilot
			 //MI no TFR for RF SILENT
			 if(theLantirn && theLantirn->GetTFRMode() == LantirnClass::TFR_STBY && g_bTFRFixes)
			 {
				 theLantirn->PID_MX = 0.0F;
				 theLantirn->PID_lastErr = 0.0F;
			 }

			 if(g_bRealisticAvionics)
			 {
				 if(this == FalconLocalSession->GetPlayerEntity())
				 {
					 if(RFState == 2 || !HasPower(AircraftClass::APPower)) //SILENT or no power
					 {						 
						 SetAutopilot(AircraftClass::APOff);
						 ClearAPFlag(AircraftClass::AttHold);
						 ClearAPFlag(AircraftClass::AltHold);
					 }
					 else
					 {
						 if(g_bINS)
						 {
							 if(INSState(AircraftClass::INS_HUD_FPM)) //No AP if INS not aligned and in NAV)
							 {
								 DBrain()->LantirnAP();
								 theBrain->throtl = UserStickInputs.throttle;
							 }
							 else
							 {
								 SetAutopilot(AircraftClass::APOff);
								 ClearAPFlag(AircraftClass::AttHold);
								 ClearAPFlag(AircraftClass::AltHold);
							 }
						 }
						 else
						 {
							 DBrain()->RealisticAP();
							 theBrain->throtl = UserStickInputs.throttle;
						 }
					 }
				 }
				 else
				 {
					 DBrain()->LantirnAP();
					 if(this == FalconLocalSession->GetPlayerEntity())
						 theBrain->throtl = UserStickInputs.throttle;
					 else
						 theBrain->throtl = 0.0F;
				 }
			 }
			 else
			 {				
				 DBrain()->LantirnAP();
				 if(this == FalconLocalSession->GetPlayerEntity())
					 theBrain->throtl = UserStickInputs.throttle;
				 else
					 theBrain->throtl = 0.0F;
			 }
			 break;			 
		 case ThreeAxisAP:
			 if(g_bRealisticAvionics)
			 {
				 if(this == FalconLocalSession->GetPlayerEntity())
				 {
					 //no AP if gearhandle down or FLCS fault or fuel door open
					if(af->gearHandle < 0.0F && !mFaults->GetFault(FaultClass::flcs_fault) &&
						!af->IsEngineFlag(AirframeClass::FuelDoorOpen) &&
						HasPower(AircraftClass::APPower) && !TrimAPDisc)
					{
						if(g_bINS)
						{
							if(INSState(AircraftClass::INS_HUD_FPM)) //No AP if INS not aligned and in NAV)
							{
								DBrain()->RealisticAP();
								theBrain->throtl = UserStickInputs.throttle;
							}
							else
							{
								SetAutopilot(AircraftClass::APOff);
								ClearAPFlag(AircraftClass::AttHold);
								ClearAPFlag(AircraftClass::AltHold);
							}
						}
						else
						{
							DBrain()->RealisticAP();
							theBrain->throtl = UserStickInputs.throttle;
						}
					}
					else
					{
						//me123 said the switches reset themselves. So here we go....
						SetAutopilot(AircraftClass::APOff);
						ClearAPFlag(AircraftClass::AttHold);
						ClearAPFlag(AircraftClass::AltHold);
						/*theBrain->rStick = UserStickInputs.rstick;
						theBrain->pStick = UserStickInputs.pstick;     
						theBrain->yPedal = UserStickInputs.rudder;
						theBrain->throtl = UserStickInputs.throttle;*/
					}
				 }
				 else
				 {
					DBrain()->ThreeAxisAP();
					if(this == FalconLocalSession->GetPlayerEntity())
						 theBrain->throtl = UserStickInputs.throttle;
					else
						 theBrain->throtl = 0.0F;
				 }
			 }
			 else
			 {
				DBrain()->ThreeAxisAP();
				if(this == FalconLocalSession->GetPlayerEntity())
					 theBrain->throtl = UserStickInputs.throttle;
				else
					 theBrain->throtl = 0.0F;
			 }
		 break;	 
		 case WaypointAP:
			 DBrain()->WaypointAP();
			 break;
			 
		 case CombatAP:
			 theBrain->FrameExec (targetList, targetPtr);
			 break;
		 }
 
	     // Use the brain's target...
         SetTarget(theBrain->targetPtr);

			if (theBrain->IsSetFlag(BaseBrain::MslFireFlag))
				FCC->releaseConsent = TRUE;
			else
				FCC->releaseConsent = FALSE;

         fireGun     = theBrain->IsSetFlag(BaseBrain::GunFireFlag);
		}

      if (this == FalconLocalSession->GetPlayerEntity())
      {
         if (UserStickInputs.pickleButton == PilotInputs::On)
            FCC->releaseConsent = TRUE;

         // Gun Trigger
         if (UserStickInputs.trigger == PilotInputs::Down)
            fireGun = TRUE;
	  }

      // Add GLOC effects
// 2002-02-17 MN a try on having the AI better handle GLOC
      if (acFlags & InRecovery)
      {
         if (glocFactor > 0.75F)
         {
            af->pstick *= 0.95F;
            af->rstick *= 0.95F;
            af->ypedal *= 0.95F;
         }
         else if (glocFactor > 0.6F)
		 {
            af->pstick = 0.75F * af->pstick + 0.25F * theBrain->pStick;
            af->rstick = 0.75F * af->rstick + 0.25F * theBrain->rStick;
            af->ypedal = 0.75F * af->ypedal + 0.25F * theBrain->yPedal;
         }
		 else if (glocFactor > 0.45F)
         {
            af->pstick = 0.5F * af->pstick + 0.5F * theBrain->pStick;
            af->rstick = 0.5F * af->rstick + 0.5F * theBrain->rStick;
            af->ypedal = 0.5F * af->ypedal + 0.5F * theBrain->yPedal;
         }
		 else
		 {
            af->pstick = 0.25F * af->pstick + 0.75F * theBrain->pStick;
            af->rstick = 0.25F * af->rstick + 0.75F * theBrain->rStick;
            af->ypedal = 0.25F * af->ypedal + 0.75F * theBrain->yPedal;
         }
		 af->throtl = 0.6F; // would we stay in afterburner when GLOC-ing ? surely not..
         if (glocFactor < 0.2F)
            acFlags &= ~InRecovery;
      }
      else
      {
         if (glocFactor >= 0.99F)
            acFlags |= InRecovery;

	     af->pstick = theBrain->pStick;
	     af->rstick = theBrain->rStick;
		 af->throtl = theBrain->throtl;
         af->ypedal = theBrain->yPedal;
      }
   }
   else
   {
	   if (this != FalconLocalSession->GetPlayerEntity())
	   {
			af->pstick = 0.0F;	
			af->rstick = 0.0F;
			af->ypedal = 0.0F;
			if(OnGround())
				af->throtl = 0.0F;
			else
				af->throtl = 0.9F;
	   }
	   else
	   {

		  // Turn off the SuperSimple flight model
		   if ( playerFlightModelHack == FALSE )
		  {
      		af->SetSimpleMode(SIMPLE_MODE_OFF);
	//      	af->ClearFlag(AirframeClass::SuperSimple);
		  }

		  if (acFlags & InRecovery)
		  {
			 if (glocFactor > 0.25F)
			 {
				af->pstick *= 0.95F;
				af->rstick *= 0.95F;
				af->ypedal *= 0.95F;
			 }
			 else
			 {
				af->pstick = 0.5F * af->pstick + 0.5F * UserStickInputs.pstick;
				af->rstick = 0.5F * af->rstick + 0.5F * UserStickInputs.rstick;
				af->ypedal = 0.5F * af->ypedal + 0.5F * UserStickInputs.rudder;
			 }

			 if (glocFactor < 0.1F && fabs (UserStickInputs.pstick) < 0.1F)
				acFlags &= ~InRecovery;
		  }
		  else
		  {
			 if (glocFactor >= 0.99F)
				acFlags |= InRecovery;

			 af->pstick = UserStickInputs.pstick;     
			   af->ypedal = UserStickInputs.rudder;
			   af->rstick = UserStickInputs.rstick;
		  }
		  af->ptrmcmd = UserStickInputs.ptrim;
		  af->rtrmcmd = UserStickInputs.rtrim;
		  af->ytrmcmd = UserStickInputs.ytrim;
		  af->throtl = UserStickInputs.throttle;
#ifdef _DEBUG
		  if (af -> ptrmcmd != 0.0f || af -> rtrmcmd != 0.0f || af -> ytrmcmd != 0.0f)  
		    MonoPrint("Trim %g %g %g\n", af->ptrmcmd, af->rtrmcmd, af->ytrmcmd);
#endif
		  // Test hack
	//      theBrain->FrameExec (targetList, targetPtr);
	//      af->SetSimpleMode(SIMPLE_MODE_OFF);
	//      af->ReSetMaxRoll();

		  if (narrowFOV)
		  {
			 af->pstick *= af->pstick;
			 if (UserStickInputs.pstick < 0.0F)
				af->pstick *= -1.0F;
			 af->rstick *= af->rstick;
			 if (UserStickInputs.rstick < 0.0F)
				af->rstick *= -1.0F;
			 af->ypedal *= af->ypedal;
			 if (UserStickInputs.rudder < 0.0F)
				af->ypedal *= -1.0F;
		  }		  

		  if(DBrain()->RefuelStatus() == DigitalBrain::refRefueling)
		  {
			  AircraftClass* tanker;
			  //we are attempting to refuel, if we're close enough we get some help
			  tanker = (AircraftClass*)vuDatabase->Find(DBrain()->Tanker());
			  if(tanker && tanker->IsAirplane())
			  {
				  DBrain()->HelpRefuel(tanker);
			  }
		  }

		  if (UserStickInputs.pickleButton == PilotInputs::On)
			 FCC->releaseConsent = TRUE;
		  else
			 FCC->releaseConsent = FALSE;

		  // Gun Trigger
		  if (UserStickInputs.trigger == PilotInputs::Down)
			 fireGun = TRUE;
		  else
			 fireGun = FALSE;
	   }
	   DBrain()->UpdateTaxipoint();
   }

#ifdef DEBUGLABEL
   if (g_nShowDebugLabels & 0x80)
   {
	   char label[40];
	   sprintf(label,"P%1.3f R%1.3f T%1.3f Y%1.3f",af->pstick, af->rstick, af->throtl, af->ypedal);
		if (g_nShowDebugLabels & 0x8000)
		{
			 if (af->GetSimpleMode())
				strcat(label, " SIMP");
		     else
				strcat(label, " COMP");
		}
	   if ( drawPointer )
			((DrawableBSP*)drawPointer)->SetLabel (label, ((DrawableBSP*)drawPointer)->LabelColor());
   }
   else if ((g_nShowDebugLabels & 0x100 || g_nShowDebugLabels & g_nMaxDebugLabel) && DBrain())
		DBrain()->ReSetLabel(this);
#endif


	DBrain()->AiCheckPlayerInPosition();

   // adjust for damage effects
   // if pctStrength is below zero we're just plane (sic) dying....
   if ( pctStrength > 0.0f)
   {
       // JPO Add in trim
       af->pstick += af->ptrmcmd;
       af->rstick += af->rtrmcmd;
       af->ypedal += af->ytrmcmd;
		if(af->GetSimpleMode() == SIMPLE_MODE_OFF)
		{
      float scaleFactor = (float)sqrt (sqrt(pctStrength));

			af->pstick *= scaleFactor;
			af->rstick *= scaleFactor;
			af->throtl *= scaleFactor;
			af->ypedal *= scaleFactor;
		}
   }
   else
   {
	   // bzzzt, thanks for playing.
       fireGun = FALSE;
       FCC->releaseConsent = FALSE;

	   // hm, about we just try pegging some values...
	   af->pstick = 0.5f;

	   if ( af->rstick < 0.0f )
	   		af->rstick = -0.5f;
	   else
	   		af->rstick = 0.5f;

	   af->ypedal = 0.0f;

		if(af->GetSimpleMode() == SIMPLE_MODE_OFF)
		{
	   		af->throtl = 0.0f;
		}
		else
		{
	   		af->throtl = 0.5f;
		}
   }

   // Shake due to loss of computer functionallity and speed
   if (IsSetFlag(MOTION_OWNSHIP))
   {
   int perturb = FALSE;
   float maxSpeed = 1.0F;

			// JB 000815 change == comparison to &
      if (mFaults && mFaults->GetFault(FaultClass::flcs_fault) & FaultClass::dual)
      {
         maxSpeed -=0.05F;
         perturb = TRUE;
      }

      if (mFaults && mFaults->GetFault(FaultClass::eng_fault) & FaultClass::efire)
      {
         maxSpeed -=0.05F;
         perturb = TRUE;
      }

      if (mFaults && mFaults->GetFault(FaultClass::eng_fault) & FaultClass::hydr)
      {
         maxSpeed -=0.05F;
         perturb = TRUE;
      }

      if (mFaults && mFaults->GetFault(FaultClass::isa_fault) & FaultClass::all)
      {
         maxSpeed -=0.05F;
         perturb = TRUE;
      }

      if (mFaults && mFaults->GetFault(FaultClass::isa_fault) & FaultClass::rudr)
      {
         af->ypedal = 0.0F;
      }
			// JB 000815 change == comparison to &

      // JPO - total hydraulic failure - no control.
      if (af->HydraulicA() == 0 && af -> HydraulicB() == 0) {
	  af->rstick = 0.0f;
	  af->pstick = 0.0f;
	  af->ypedal = 0.0f;
      }
      
			// JB 000820
			//if (perturb)
      if (perturb && !g_bDisableFunkyChicken)
			// JB 000820
      {
         ioPerturb += (af->mach - maxSpeed);
      }
   }

	 // JB 000814
	 // JB 010104 add CombatAP check
	 if (g_bNewDamageEffects && autopilotType != CombatAP)
	 {
		 af->ypedal += yBias;
		 af->rstick += rBias;
		 af->pstick += pBias;
	 }
	 // JB 000814

	 //MI asynchronous lift
	 if(g_bRealisticAvionics && g_bNewDamageEffects && autopilotType != CombatAP && !isDigital)
	 {
		 //produce asynchronous "lift"
		 if(LEFState(LEFSASYNCH))
		 {
		     int llef, rlef; // differentiate between simple complex models - JPO
		     if (IsComplex()) {
			 llef = COMP_LT_LEF;
			 rlef = COMP_RT_LEF;
		     }
		     else {
			 llef = SIMP_LT_LEF;
			 rlef = SIMP_RT_LEF;
		     }

			 if(mFaults && !mFaults->GetFault(lef_fault))
				 mFaults->SetCaution(lef_fault);

			 if(GetDOFValue(llef) != GetDOFValue(rlef))
			 {			 
				 LEFOn(LEFSASYNCH);

				 float left = GetDOFValue(llef);
				 float right = GetDOFValue(rlef);
				 float difference = left - right;
				 if(difference < 0)
					 difference *= -1;
				 difference = min(difference, 0.2F);
				 if(left > right)
				 {
					 //left gives more lift, roll right
					 af->rstick += difference;
				 }
				 else if(left < right)
				 {
					 //right gives more lift, roll left
					 af->rstick -= difference;
				 }
			 }
		 }
		 else
		 {
			 if(mFaults->GetFault(lef_fault) && !LEFLocked)
				 mFaults->ClearFault(lef_fault);

			 LEFOff(LEFSASYNCH);
		 }
	 }

   // apply any perturbations to stick input
   if ( ioPerturb > 0.0f && !OnGround())
   {
	   af->ypedal += PRANDFloat() * ioPerturb;
	   af->rstick += PRANDFloat() * ioPerturb;
	   af->pstick += PRANDFloat() * ioPerturb;
   }
}

void AircraftClass::ToggleAutopilot (void)
{
	DBrain()->AiSetInPosition();
   if (autopilotType == APOff)
	{

      switch (PlayerOptions.GetAutopilotMode())
      {
         case APIntelligent:
            SetAutopilot (CombatAP);
         break;

         case APEnhanced:
            SetAutopilot (WaypointAP);
         break;

         case APNormal:
            SetAutopilot (ThreeAxisAP);
         break;
      }      
   }
   else
   {
      SetAutopilot (APOff);
   }
}

void AircraftClass::SetAutopilot(AutoPilotType flag)
{
AutoPilotType lastType = autopilotType;

   autopilotType = flag;

   switch (flag)
   {
      case APOff:		  

         // Reset weapons for combat AP
         if (lastType == CombatAP)
         {
            FCC->SetMasterMode (playerLastMasterMode);
            FCC->SetSubMode (playerLastSubMode);
            Sms->SetWeaponType (playerLastWeaponType);
            Sms->FindWeaponClass (playerLastWeaponClass);
            DBrain()->ClearCurrentMissile();
         }
         af->SetMaxRoll(190.0F);
         af->ClearFlag(AirframeClass::WheelBrakes);
         af->SetFlag(AirframeClass::AutoCommand);
         af->ClearFlag(AirframeClass::GCommand);
         af->ClearFlag(AirframeClass::AlphaCommand);
         if(IsSetFlag(ON_GROUND))
         {
            if(IO.AnalogIsUsed (2))
            {				  
               af->SetFlag(AirframeClass::EngineOff);
               af->SetFlag(AirframeClass::ThrottleCheck);
               af->pwrlev = af->throtl = DBrain()->throtl = UserStickInputs.throttle = ReadThrottle();
            }
            else
            {
               af->ClearFlag(AirframeClass::EngineOff);
               af->ClearFlag(AirframeClass::ThrottleCheck);
               UserStickInputs.Reset();
            }
            DBrain()->ResetTaxiState();
         }
      break;

      // JPO - lantirn specific stuff... bit like ThreeAxis
	  case LantirnAP:
		  //MI only those who have it
#ifndef _DEBUG
		  if(!af->HasTFR())
		  {
			  SetAutopilot(AircraftClass::APOff);
			  return;
		  }
#endif
          ((DigitalBrain*)theBrain)->SetHoldAltitude(-ZPos() - -OTWDriver.GetGroundLevel( XPos(), YPos()) );
          ((DigitalBrain*)theBrain)->SetHoldHeading(Yaw());
         af->ClearFlag(AirframeClass::EngineOff);
         af->ClearFlag(AirframeClass::ThrottleCheck);
         playerLastWeaponType = Sms->curWeaponType;
         playerLastWeaponClass = Sms->curWeaponClass;
         playerLastMasterMode = FCC->GetMasterMode();
         playerLastSubMode = FCC->GetSubMode();
		 if(g_bTFRFixes && theLantirn)
		 {
			 theLantirn->PID_MX = 0.0F;
			 theLantirn->PID_lastErr = 0.0F;
		 }
		 break;
      case ThreeAxisAP:
		   ((DigitalBrain*)theBrain)->SetHoldAltitude(-ZPos());
		   ((DigitalBrain*)theBrain)->SetHoldHeading(Yaw());
	  case WaypointAP:
         af->ClearFlag(AirframeClass::EngineOff);
         af->ClearFlag(AirframeClass::ThrottleCheck);
         playerLastWeaponType = Sms->curWeaponType;
         playerLastWeaponClass = Sms->curWeaponClass;
         playerLastMasterMode = FCC->GetMasterMode();
         playerLastSubMode = FCC->GetSubMode();
      break;

      case CombatAP:
         af->ClearFlag(AirframeClass::EngineOff);
         af->ClearFlag(AirframeClass::ThrottleCheck);
         playerLastWeaponType = Sms->curWeaponType;
         playerLastWeaponClass = Sms->curWeaponClass;
         playerLastMasterMode = FCC->GetMasterMode();
         playerLastSubMode = FCC->GetSubMode();
         DBrain()->ResetTaxiState();
      break;
   }
}
void AircraftClass::SetAPParameters(void)
{
	//Is our AP on?
	if(IsOn(AircraftClass::AltHold) || IsOn(AircraftClass::AttHold))
	{
		autopilotType = ThreeAxisAP;
	}
	else
		autopilotType = APOff;
}
void AircraftClass::SetNewRoll(void)
{
	if(Roll() * RTD > 60.0F)
		((DigitalBrain*)theBrain)->destRoll = 60.0F;
	else if(Roll() * RTD < -60.0F)
		((DigitalBrain*)theBrain)->destRoll = -60.0F;
	else
		((DigitalBrain*)theBrain)->destRoll = Roll() * RTD;
}
void AircraftClass::SetNewPitch(void)
{
	((DigitalBrain*)theBrain)->destPitch = Pitch() * RTD;
}
void AircraftClass::SetNewAlt(void)
{
	((DigitalBrain*)theBrain)->currAlt = -ZPos();
}

