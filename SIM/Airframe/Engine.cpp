/******************************************************************************/
/*                                                                            */
/*  Unit Name : engine.cpp                                                    */
/*                                                                            */
/*  Abstract  : Models engine thrust.                                         */
/*                                                                            */
/*  Dependencies : Auto-Generated                                             */
/*                                                                            */
/*  Operating System : MS-DOS 6.2,                                            */
/*                                                                            */
/*  Compiler : WATCOM C/C++ V10                                               */
/*                                                                            */
/*  Naming Conventions :                                                      */
/*                                                                            */
/*      Public Functions    : Mixed case with no underscores                  */
/*      Private Functions   : Mixed case with no underscores                  */
/*      Public Functions    : Mixed case with no underscores                  */
/*      Global Variables    : Mixed case with no underscores                  */
/*      Classless Functions : Mixed case with no underscores                  */
/*      Classes             : All upper case seperated by an underscore       */
/*      Defined Constants   : All upper case seperated by an underscore       */
/*      Macros              : All upper case seperated by an underscore       */
/*      Structs/Types       : All upper case seperated by an underscore       */
/*      Private Variables   : All lower case seperated by an underscore       */
/*      Public Variables    : All lower case seperated by an underscore       */
/*      Local Variables     : All lower case seperated by an underscore       */
/*                                                                            */
/*  Development History :                                                     */
/*  Date      Programer           Description                                 */
/*----------------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                               */
/*                                                                            */
/******************************************************************************/
#include "stdhdr.h"
#include "airframe.h"
#include "aircrft.h"
#include "fack.h"
#include "MsgInc\DamageMsg.h"
#include "falcsess.h"
#include "campbase.h"
#include "fsound.h"
#include "soundfx.h"

static float fireTimer = 0.0F;
static const float eputime = 600.0f; // 10 minutes epu fuel
static const float jfsrechargetime = 60.0f; // xx minutes of JFS cranking
static const float ftitrate = 0.7f;
int supercruise = FALSE;
extern bool g_bRealisticAvionics;

#include "otwdrive.h"
#include "cpmanager.h"

/********************************************************************/
/*                                                                  */
/* Routine: void AirframeClass::EngineModel (time )                 */
/*                                                                  */
/* Description:                                                     */
/*    Models engine thrust as a function of mach and altitude.      */
/*    Calcualtes body axis forces from thrust.                      */
/*                                                                  */
/* Inputs:                                                          */
/*    time                                                          */
/*                                                                  */
/* Outputs:                                                         */
/*    None                                                          */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
void AirframeClass::EngineModel(float dt)
{
float th1, th2;
float thrtb1;
float tgross;
float fuelFlowSS;
float ta01, rpmCmd;
int aburnLit;
float spoolrate = auxaeroData->normSpoolRate;

    // JPO should we switch on the EPU ?
    if (epuFuel > 0.0f) //only relevant if there is fuel
	{ 
		if (GetEpuSwitch() == ON) 
		{// pilot command
			GeneratorOn(GenEpu);
		} 
		// auto mode
		else if (!GeneratorRunning(GenMain) && !GeneratorRunning(GenStdby) && IsSet(InAir)) {
			GeneratorOn(GenEpu);
		}
		else if (GetEpuSwitch() == OFF) 
		{
			GeneratorOff(GenEpu);
			EpuClear();
		}
		else 
		{
			GeneratorOff(GenEpu);
			EpuClear();
		}
	}
	else 
	{
		GeneratorOff(GenEpu);
		EpuClear();
	}
    // check hydraulics and generators 
    if (GeneratorRunning(GenMain) || GeneratorRunning(GenStdby)) {
	HydrRestore(HYDR_ALL); // restore those systems we can
    }
    else if (GeneratorRunning(GenEpu)) {
	HydrDown (HYDR_B_SYSTEM); // B system now dead
	HydrRestore(HYDR_A_SYSTEM); // A still OK
    }
    else {
	HydrDown(HYDR_ALL); // all off
    }

    // JPO - if the epu is running - well its running!
    // this may be independant of the engine if the pilot wants to check
    if (GeneratorRunning(GenEpu)) {
	EpuClear();
	// below 80%rpm, epu burns fuel.
	if (rpm < 0.80f) {
	    EpuSetHydrazine();
	    epuFuel -= 100.0f * dt / auxaeroData->epuBurnTime; // burn some hydrazine
	    if (epuFuel <= 0.0f) {
		epuFuel = 0.0;
		GeneratorBreak(GenEpu); // well not broken, but done for.
		GeneratorOff(GenEpu);
	    }
	}
	EpuSetAir(); // always true, means running really.
    }

    // JPO: charge the JFS accumulators, up to 100% 
    if (rpm > auxaeroData->jfsMinRechargeRpm && jfsaccumulator < 100.0f /* 2002-04-11 ADDED BY S.G. If less than 0, don't recharge */ && jfsaccumulator >= 0.0f ) {
		jfsaccumulator += 100.0f * dt / auxaeroData->jfsRechargeTime;
		jfsaccumulator = min(jfsaccumulator, 100.0f);
    }
    // transfer fuel
    FuelTransfer(dt);

#if 0 // not ready yet.JPO, but basically it should work as normal, but ignore the throttle.
    if (IsSet(ThrottleCheck)) {
	rpm      = oldRpm[0]; // just remember where we were... JPO
	if(throtl >= 1.5F)
	    throtl = pwrlev;
	if (fabs (throtl - pwrlev) > 0.1F)
	    ClearFlag(ThrottleCheck);
	pwrlev = throtl;
    }
    else
    {
	if (fabs (throtl - pwrlev) > 0.1F)
            ClearFlag (EngineOff);
    }
#endif

    if (IsSet(EngineStopped)) {
	ftit = Math.FLTust(0.0f, 20.0f, dt, oldFtit); // cool down the engine
	float modRpm = (float)(rpm*rpm*rpm*rpm*sqrt(rpm));
	
	// Am I increasing the rpm (but not yet in afterburner)?
	if (rpm > oldp01[0])
	    Math.FLTust (modRpm, 4.0F, dt, oldp01);
	// Must be decreasing then...
	else {
	    // Now check if the 'heat' is still above 1.00 (100%)
	    if (oldp01[0] > 1.0F)
		Math.FLTust (modRpm, 7.0F, dt, oldp01);
	    else
		Math.FLTust (modRpm, 2.0F, dt, oldp01);
	}

	spoolrate = auxaeroData->flameoutSpoolRate; // spool down rate
	fireTimer = 0.0f; // reset - engine is switched off
	// switch all to 0.
	tgross = 0.0F;
	fuelFlow = 0.0F;
	thrtab = 0.0F;
	thrust = 0.0f;

	// broken engine - anything but a flame out?
	if((platform->mFaults->GetFault(FaultClass::eng_fault) & ~FaultClass::fl_out) != 0) {
	    rpmCmd = 0.0f; // engine must be seized, not going to start or windmill
	}
	else if (IsSet (JfsStart)) {
	    rpmCmd = 0.25f; // JFS should take us up to 25%
	    spoolrate = 15.0f;
		//decrease spin time
		JFSSpinTime -= SimLibMajorFrameTime;
		if(JFSSpinTime <= 0)
			ClearFlag(JfsStart);
	}
	else { // engine windmill (~12% at 450 knts) (me123 - this works on mine)
	    rpmCmd = platform->Kias() / 450.0f * 0.12f;
	}
	
	// attempt to spool up/down
	rpm = Math.FLTust (rpmCmd, spoolrate, dt, oldRpm);
    }
    else if (!IsSet(EngineOff))
    {
	/*------------------*/
	/* get gross thrust */
	/*------------------*/
	if(platform->mFaults->GetFault(FaultClass::eng_fault) & FaultClass::fl_out) {
	    SetFlag(EngineStopped); //JPO - engine is now stopped!
	    throtl = 0.0F;
	}
	pwrlev = throtl;
	// AB Failure



	//MI
	if(auxaeroData->DeepStallEngineStall)
	{
		//me123 if in deep stall and in ab lets stall the engine
		// JPO - 10% chance, each time we check...
		if (stallMode >= DeepStall && pwrlev >= 1.0F && rand() % 10 == 1)
		{ 
			SetFlag(EngineStopped);
			// mark it as a flame out
			platform->mFaults->SetFault(FaultClass::eng_fault, FaultClass::fl_out, FaultClass::fail, FALSE);
		}
	}

	if (platform->IsSetFlag(MOTION_OWNSHIP))
	{
	    if (platform->mFaults->GetFault(FaultClass::eng_fault) & FaultClass::a_b)
	    {
		pwrlev = min (pwrlev, 0.99F);
	    }

         if (platform->mFaults->GetFault(FaultClass::eng_fault) & FaultClass::efire)
         {
            pwrlev *= 0.5F;

            if (fireTimer >= 0.0F)
               fireTimer += max (throtl, 0.3F) * dt;

            // On fire long enough, blow up
            if (fireTimer > 60.0F) // 60 seconds at mil power
            {
               fireTimer = -1.0F;
					FalconDamageMessage* message;
					message = new FalconDamageMessage (platform->Id(), FalconLocalGame );
					message->dataBlock.fEntityID  = platform->Id();

					message->dataBlock.fCampID = platform->GetCampaignObject()->GetCampID();
					message->dataBlock.fSide   = platform->GetCampaignObject()->GetOwner();
					message->dataBlock.fPilotID   = ((AircraftClass*)platform)->pilotSlot;
					message->dataBlock.fIndex     = platform->Type();
					message->dataBlock.fWeaponID  = platform->Type();
					message->dataBlock.fWeaponUID = platform->Id();
					message->dataBlock.dEntityID  = message->dataBlock.fEntityID;
					message->dataBlock.dCampID = message->dataBlock.fCampID;
					message->dataBlock.dSide   = message->dataBlock.fSide;
					message->dataBlock.dPilotID   = message->dataBlock.fPilotID;
					message->dataBlock.dIndex     = message->dataBlock.fIndex;
					message->dataBlock.damageType = FalconDamageType::CollisionDamage;
					message->dataBlock.damageStrength = 2.0F * platform->MaxStrength();
					message->dataBlock.damageRandomFact = 1.5F;
					
					message->RequestOutOfBandTransmit ();
					FalconSendMessage (message,TRUE);
            }
         }
      }
			if (engineData->hasAB) // JB 010706
	      pwrlev = max (min (pwrlev, 1.5F), 0.0F);
			else
				pwrlev = max (min (pwrlev, 1.0F), 0.0F);

      if (rpm < 0.68f) { // below Idle
	rpmCmd = 0.7f;
	spoolrate = auxaeroData->lightupSpoolRate;
	thrtb1 = 0.0f;
	if (rpm > 0.5f)
	    ClearFlag(JfsStart);
	ftit = Math.FLTust(5.1F * (rpm/0.7f), ftitrate, dt, oldFtit);
      }
      else if (pwrlev <= 1.0)
      {
	   /*-------------------*/
	   /* Mil power or less */
	   /*-------------------*/
         th1 = Math.TwodInterp (-z, mach, engineData->alt,
            engineData->mach, engineData->thrust[0], engineData->numAlt,
            engineData->numMach, &curEngAltBreak, &curEngMachBreak);
         th2 = Math.TwodInterp (-z, mach, engineData->alt,
            engineData->mach, engineData->thrust[1], engineData->numAlt,
            engineData->numMach, &curEngAltBreak, &curEngMachBreak);

         aburnLit = FALSE;
         thrtb1 = ((th2 - th1)*pwrlev + th1) / mass;
         rpmCmd = 0.7F + 0.3F * pwrlev;
	 // ftit calculated
	 if (rpm < 0.9F)
	 {
	     ftit = Math.FLTust(5.1F + (rpm - 0.7F) / 0.2F * 1.0F, ftitrate, dt, oldFtit);
	 }
	 else if (rpm < 1.0F)
	 {
	     ftit = Math.FLTust(6.1F + (rpm - 0.9F) / 0.1F * 1.5F, ftitrate, dt, oldFtit);
	 }
      }
      else
	   /*--------------------------*/
	   /* Some stage of afterburner */
	   /*--------------------------*/
      {
         th1 = Math.TwodInterp (-z, mach, engineData->alt,
            engineData->mach, engineData->thrust[1], engineData->numAlt,
            engineData->numMach, &curEngAltBreak, &curEngMachBreak);
         th2 = Math.TwodInterp (-z, mach, engineData->alt,
            engineData->mach, engineData->thrust[2], engineData->numAlt,
            engineData->numMach, &curEngAltBreak, &curEngMachBreak);

         aburnLit = TRUE;
         thrtb1 = (2.0F*(th2 - th1)*(pwrlev-1.0F) + th1) / mass;
         rpmCmd = 1.0F + 0.06F * (pwrlev - 1.0F);
	 // ftit calculated
	 ftit = Math.FLTust(7.6F + (rpm - 1.0F) / 0.03F * 0.1F, ftitrate, dt, oldFtit);
      }

      /*--------------------------------*/
      /* scale thrust to reference area */
      /*--------------------------------*/
      thrtab = thrtb1 * engineData->thrustFactor;
      
      /*-----------------*/
      /* engine dynamics */
      /*-----------------*/
      if(IsSet(Trimming))// || simpleMode == SIMPLE_MODE_AF)
      {
         ethrst = 1.0F;
         tgross = thrtab;
         olda01[0] = tgross;
         olda01[1] = tgross;
         olda01[2] = tgross;
         olda01[3] = tgross;

         rpm = rpmCmd;
         oldRpm[0] = rpm;
         oldRpm[1] = rpm;
         oldRpm[2] = rpm;
         oldRpm[3] = rpm;
      }
      else
      {
         if(aburnLit)
         {
            /*----------------------*/
            /* thrust for afterburn */
            /*----------------------*/
            ta01 = auxaeroData->abSpoolRate;
            tgross = Math.FLTust(thrtab,ta01,dt,olda01);
         }
         else
         {
            /*----------------------*/
            /* thrust for Mil Power */
            /*----------------------*/
            if(pwrlev <= 1.0F)
               ta01 = auxaeroData->normSpoolRate;
            else
               ta01 = auxaeroData->abSpoolRate;

            tgross = Math.FLTust(thrtab,ta01,dt,olda01);
         }
      }

      /*-----------*/
      /*   burn fuel */
      /*-----------*/
      if (AvailableFuel() <= 0.0f || IsEngineFlag(MasterFuelOff)) { // no fuel - dead engine.
	  SetFlag(EngineStopped);
	  // mark it as a flame out
	  platform->mFaults->SetFault(FaultClass::eng_fault, FaultClass::fl_out, FaultClass::fail, FALSE);
      }
      else
      {
	  // JPO - back to basics... this stuff doesn't have to be complicated surely.
	  // fuel flow is proportional to thrust.
	  // thrust factor is already in, its just that tgross is thrust/mass, 
	  // so we get rid of the mass component again.

	  if (aburnLit)
	  {
	      fuelFlowSS =  auxaeroData->fuelFlowFactorAb * tgross  * mass;
	  }
	  else
	  { 
	      fuelFlowSS = auxaeroData->fuelFlowFactorNormal * tgross * mass;
	  }

         // For simplified model, burn less fuel
         if (IsSet(Simplified))
         {
            fuelFlowSS *= 0.75F;
         }

         if (!platform->IsSetFlag(MOTION_OWNSHIP))
         {
            fuelFlowSS *= 0.75F;
         }

         fuelFlow += (fuelFlowSS - fuelFlow) / 10;

         /*----------------------------------------------------------*/
         /* If fuel flow less < 100 lbs/min fuel flow == 100lbs/min) */
         /*----------------------------------------------------------*/
         if (fuelFlow < auxaeroData->minFuelFlow) fuelFlow = auxaeroData->minFuelFlow;//me123 from 1000
         if (fuelFlowSS < auxaeroData->minFuelFlow) fuelFlowSS = auxaeroData->minFuelFlow;//me123 from 1000

         if (!IsSet(NoFuelBurn))
         {
	   // JPO - fuel is now burnt and transferred.
	     BurnFuel(fuelFlowSS * dt / 3600.0F);
#if 0 // old code
	     if (externalFuel > 0.0)
		 externalFuel -= fuelFlowSS * dt / 3600.0F;
	     else
		 fuel = fuel - fuelFlowSS * dt / 3600.0F ;//me123 deleted + externalFuel;
#endif	     
	     weight -= fuelFlowSS * dt / 3600.0F;
	     mass    = weight / GRAVITY;
         }

		 /*
         if (IsSet(Refueling))
         {
            fuel += 3000.0F * dt;
            if (fuel > 12000.0F)
               fuel = 12000.0F;
         }
		 */
      }

      rpm = Math.FLTust (rpmCmd, spoolrate, dt, oldRpm);
      if (fuel > 0.0F)
         rpm = max (rpm, 0.01F);

// ADDED BY S.G. TO SIMULATE THE HEAT PRODUCED BY THE ENGINE
// I'M USING A PREVIOUSLY UNUSED ARRAY CALLED oldp01 FOR ENGINE HEAT TEMPERATURE
	  // Afterburner lit?
	  if (rpm > 100.0f)
		  Math.FLTust (rpm + 0.5f, 0.5F, dt, oldp01);
	  else {
		  // the 'modified rpm will be rpm^4.5
		  float modRpm = (float)(rpm*rpm*rpm*rpm*sqrt(rpm));

		  // Am I increasing the rpm (but not yet in afterburner)?
		  if (rpm > oldp01[0])
			  Math.FLTust (modRpm, 4.0F, dt, oldp01);
		  // Must be decreasing then...
		  else {
			  // Now check if the 'heat' is still above 1.00 (100%)
			  if (oldp01[0] > 1.0F)
				  Math.FLTust (modRpm, 7.0F, dt, oldp01);
			  else
				  Math.FLTust (modRpm, 2.0F, dt, oldp01);
		  }
	  }

// END OF ADDED SECTION
      /*------------*/
      /* net thrust */
      /*------------*/
	  if(platform->IsPlayer() && supercruise)
		thrust = tgross * ethrst*1.5F;
	  else
		thrust = tgross * ethrst;


	  if(stallMode >= EnteringDeepStall)
		  thrust *= 0.1f;

   }
   else
   {
      if (IsSet(ThrottleCheck))
      {
		  if(throtl >= 1.5F)
			  throtl = pwrlev;
		  if (fabs (throtl - pwrlev) > 0.1F)
			  ClearFlag(ThrottleCheck);
		  pwrlev = throtl;
      }
      else
      {
         if (fabs (throtl - pwrlev) > 0.1F)
            ClearFlag (EngineOff);
      }

      thrust   = 0.0F;
      tgross   = 0.0F;
      fuelFlow = 0.0F;
      rpm      = oldRpm[0]; // just remember where we were... JPO
      // Changed so the Oil light doesn't come on :-) - RH
      ftit     = 5.85f;
   }

   // turn on stdby generator
   if (rpm > auxaeroData->stbyGenRpm && platform->MainPowerOn()) {
       GeneratorOn(GenStdby);
   }
   else GeneratorOff(GenStdby);

   // tunr on main generator
   if (rpm > auxaeroData->mainGenRpm && platform->MainPowerOn()) {
       GeneratorOn(GenMain);
   }
   else GeneratorOff(GenMain);
   
   if (IsSet(OnObject) && vcas < 175) // JB carrier
	   thrust *= 4;
   
   /*------------------*/
   /* body axis accels */
   /*------------------*/
   if (nozzlePos == 0) { // normal case JPO
	   xprop =  thrust;
	   yprop =  0.0F;
	   zprop =  0.0F;
	   /*-----------------------*/
	   /* stability axis accels */
	   /*-----------------------*/
	   xsprop =  xprop *platform->platformAngles->cosalp;
	   ysprop =  yprop;
	   //   zsprop = 0.0F;		//assume flcs cancels this out? (makes life easier)
	   //   zsprop = -thrust*platform->platformAngles->sinalp * 0.001F; //why the 0.001F ?
	   //	zsprop = -thrust*platform->platformAngles->sinalp; // JPO previous
	   zsprop = - xprop * platform->platformAngles->sinalp;
   }
   else { // harrier fake stuff - doesn't really work.
	   mlTrig noz;
	   mlSinCos(&noz, nozzlePos);
	   xprop = thrust * noz.cos;
	   yprop =  0.0F;
	   zprop = -thrust * noz.sin;
	   /*-----------------------*/
	   /* stability axis accels */
	   /*-----------------------*/
	   xsprop =  xprop *platform->platformAngles->cosalp;
	   ysprop =  yprop;
	   //   zsprop = 0.0F;		//assume flcs cancels this out? (makes life easier)
	   //   zsprop = -thrust*platform->platformAngles->sinalp * 0.001F; //why the 0.001F ?
	   //	zsprop = -thrust*platform->platformAngles->sinalp; // JPO previous
	   zsprop = - xprop * platform->platformAngles->sinalp +
		   zprop * platform->platformAngles->cosalp;
	   
   }

	ShiAssert(!_isnan(platform->platformAngles->cosalp));
	ShiAssert(!_isnan(platform->platformAngles->sinalp));

   /*------------------*/
   /* wind axis accels */
   /*------------------*/
   xwprop =  xsprop*platform->platformAngles->cosbet +
      ysprop*platform->platformAngles->sinbet;
   ywprop = -xsprop*platform->platformAngles->sinbet +
      ysprop*platform->platformAngles->cosbet;
   zwprop =  zsprop;
}

// JPO new support routines for hydraulics
void AirframeClass::HydrBreak(int sys)
{
    if (sys & HYDR_A_SYSTEM) { // mark A system as down and broke
	hydrAB &= ~ HYDR_A_SYSTEM;
	hydrAB |= HYDR_A_BROKE;
    }
    if (sys & HYDR_B_SYSTEM) { // mark A system as down and broke
	hydrAB &= ~ HYDR_B_SYSTEM;
	hydrAB |= HYDR_B_BROKE;
    }

}

void AirframeClass::HydrRestore(int sys) 
{
    // restore A if not broke
    if ((sys & HYDR_A_SYSTEM) && (hydrAB & HYDR_A_BROKE) == 0) {
	hydrAB |= HYDR_A_SYSTEM;
    }
    // restore B if not broke
    if ((sys & HYDR_B_SYSTEM) && (hydrAB & HYDR_B_BROKE) == 0) {
	hydrAB |= HYDR_B_SYSTEM;
    }
}

void AirframeClass::StepEpuSwitch()
{
    switch (epuState) {
    case OFF:
	epuState = AUTO;
	break;
    case AUTO:
	epuState = ON;
	break;
    case ON:
	epuState = OFF;
	break;
    }
}

void AirframeClass::JfsEngineStart ()
{
    if (jfsaccumulator < 90.0f) { // not charged
	return;
    }
    F4SoundFXSetPos( SFX_VULEND, 0, x, y, z, 1.0f );
    jfsaccumulator = 0.0f; // all used up
    if (fuel <= 0.0f) return; // nothing to run JFS off.
    
    // attempting JFS start - only works below 400kias and 20,000ft
    if (platform->Kias() > 400.0f) {
	if (platform->Kias() - 400.0f > rand()%100) { // one shot failed
	    return;
	}
    }
    if (-z > 20000.0f) { 
	if ( (-z) - 20000.0f > rand() % 5000) {
	    return;
	}
    }
    SetFlag(AirframeClass::JfsStart);
	//MI add in JFS spin time
	JFSSpinTime = 240;	//4 minutes available


}

// JPO start the engine quickly - for deaggregation purposes.
void AirframeClass::QuickEngineStart ()
{
    ClearFlag (EngineStopped);
    ClearFlag(JfsStart);
    GeneratorOn(GenMain);
    GeneratorOn(GenStdby);
    rpm = 0.75f;
    oldRpm[0] = rpm;
    oldRpm[1] = rpm;
    oldRpm[2] = rpm;
    oldRpm[3] = rpm;
    HydrRestore(HYDR_ALL);
}

// fuel management stuff
// all fairly F16 specific currently.
// Tank F2 is ignored - assumed to be part of F1.

// clear down fuel tanks.
void AirframeClass::ClearFuel ()
{
    for (int i = 0; i < MAX_FUEL; i++)
	m_tanks[i] = 0.0f;
}

// split a fuel load across the tanks.
// allocate in priority order.
void AirframeClass::AllocateFuel (float totalfuel)
{
		totalfuel = max(totalfuel, 0.0);

    ClearFuel();
    m_tanks[TANK_AFTRES] = min(totalfuel/2.0f, m_tankcap[TANK_AFTRES]);
    m_tanks[TANK_FWDRES] = min(totalfuel/2.0f, m_tankcap[TANK_FWDRES]);
    totalfuel -= m_tanks[TANK_AFTRES] + m_tanks[TANK_FWDRES];

    m_tanks[TANK_A1] = min(totalfuel/2.0f, m_tankcap[TANK_A1]);
    m_tanks[TANK_F1] = min(totalfuel/2.0f, m_tankcap[TANK_F1]);
    totalfuel -= m_tanks[TANK_F1] + m_tanks[TANK_A1];

    m_tanks[TANK_WINGFR] = min(totalfuel/2.0f, m_tankcap[TANK_WINGFR]);
    m_tanks[TANK_WINGAL] = min(totalfuel/2.0f, m_tankcap[TANK_WINGAL]);
    totalfuel -= m_tanks[TANK_WINGFR] + m_tanks[TANK_WINGAL];

    // fill drop tanks now.
    m_tanks[TANK_LEXT] = min(m_tankcap[TANK_LEXT], totalfuel/2.0f);
    m_tanks[TANK_REXT] = min(m_tankcap[TANK_REXT], totalfuel/2.0f);
    totalfuel -= m_tanks[TANK_REXT];
    totalfuel -= m_tanks[TANK_LEXT];
    m_tanks[TANK_CLINE] = min(m_tankcap[TANK_CLINE], totalfuel);
    totalfuel -= m_tanks[TANK_CLINE];
    if (totalfuel > 1.0)
		{
			totalfuel = totalfuel; // JB 010506 Do something for release build
			ShiWarning("Too much fuel for the plane");
		}
    // recompute internal/external ammounts
    RecalculateFuel ();
}

// what fuel do we have available (resevoirs only)
float AirframeClass::AvailableFuel()
{
    if (!g_bRealisticAvionics) 
	return externalFuel + fuel;
    else if (IsEngineFlag(MasterFuelOff))
	return 0.0f;
    else if (fuelPump == FP_OFF && nzcgb < -0.5f) // -ve G
	return 0.0f;
    switch (fuelPump) {
    case FP_FWD:
	return m_tanks[TANK_FWDRES];
    case FP_AFT:
	return m_tanks[TANK_AFTRES];
    default:
	return m_tanks[TANK_AFTRES] + m_tanks[TANK_FWDRES];
    }
}

// burn fuel from the two possible resevoirs
int AirframeClass::BurnFuel (float bfuel)
{
    FuelPump tfp = fuelPump;
    if (tfp == FP_NORM)  { // deal with empty tanks
	if (m_tanks[TANK_AFTRES] <= 0.0f)
	    tfp = FP_FWD;
	else if (m_tanks[TANK_FWDRES] <= 0.0f)
	    tfp = FP_AFT;
    }

    // TODO: broken FFP if hydrB offline ... erratic transfer rates.
    // TODO C of G calculations
    switch (tfp) {
    case FP_OFF: // XXX or should no flow occur?
    case FP_NORM:
	m_tanks[TANK_AFTRES] -= bfuel/2.0f;
	m_tanks[TANK_FWDRES] -= bfuel/2.0f;
	break;
    case FP_FWD:
	if (m_tanks[TANK_FWDRES] <= 0.0f)
	    return 0;
	m_tanks[TANK_FWDRES] -= bfuel;
	break;
    case FP_AFT:
	if (m_tanks[TANK_AFTRES] <= 0.0f)
	    return 0;
	m_tanks[TANK_AFTRES] -= bfuel;
	break;
    }
    return 1;
}

// cross feed tank 1 from tank 2
void AirframeClass::FeedTank(int t1, int t2, float dt)
{
    // room for some more?
    float delta = m_tankcap[t1] - m_tanks[t1];
    delta = min(delta, m_tanks[t2]); // limit to amount in tank2

    float maxtrans = m_trate[t2] * dt; // limit to max trans rate
    delta = min(delta, maxtrans);

    if (delta > 0) { // transfer
	m_tanks[t1] += delta;
	m_tanks[t2] -= delta;
    }
    if (m_tanks[t2] < 0.0f) // sanity check
	m_tanks[t2] = 0.0f;
}

// do the overall tank transfers.
void AirframeClass::FuelTransfer (float dt)
{
    FeedTank(TANK_FWDRES, TANK_F1, dt);
    FeedTank(TANK_F1, TANK_WINGFR, dt);

    FeedTank(TANK_AFTRES, TANK_A1, dt);
    FeedTank(TANK_A1, TANK_WINGAL, dt);

    // transfer wing externals if
    // switch set, or cline empty and
    // we have some fuel to transfer
    // only happens if externals are pressurized.
    if (airSource == AS_NORM || airSource == AS_DUMP) {
	if (((engineFlags & WingFirst) ||
	    m_tanks[TANK_CLINE] <= 0.0f) &&
	    (m_tanks[TANK_REXT] > 0.0f ||
	    m_tanks[TANK_LEXT] > 0.0f)) {
	    FeedTank(TANK_WINGFR, TANK_REXT, dt);
	    FeedTank(TANK_WINGAL, TANK_LEXT, dt);
	}
	else if (m_tanks[TANK_CLINE] > 0.0f) {
	    FeedTank(TANK_WINGFR, TANK_CLINE, dt/2.0f);
	    FeedTank(TANK_WINGAL, TANK_CLINE, dt/2.0f);
	}
    }
    if (!platform->isDigital) {
	if (m_tanks[TANK_FWDRES] < auxaeroData->fuelMinFwd)
	{
	    if(!g_bRealisticAvionics)
		platform->mFaults->SetFault(fwd_fuel_low_fault);
	    else
		platform->mFaults->SetCaution(fwd_fuel_low_fault);
	}
	else if (fuelSwitch != FS_TEST && platform->mFaults->GetFault(fwd_fuel_low_fault))
	    platform->mFaults->ClearFault(fwd_fuel_low_fault);
	if (m_tanks[TANK_AFTRES] < auxaeroData->fuelMinAft)
	{
	    if(!g_bRealisticAvionics)
		//platform->mFaults->SetFault(fwd_fuel_low_fault);	//MI should probably be AFT tank
		platform->mFaults->SetFault(aft_fuel_low_fault);
	    else
		platform->mFaults->SetCaution(aft_fuel_low_fault);
	}
	else if (fuelSwitch != FS_TEST && platform->mFaults->GetFault(aft_fuel_low_fault))
	    platform->mFaults->ClearFault(aft_fuel_low_fault);
    }
    // recompute internal/external ammounts
    RecalculateFuel ();
}

// loose a tank.
void AirframeClass::DropTank (int n)
{
    ShiAssert(n>=0 && n < MAX_FUEL);
    m_tanks[n] = 0.0f;
    m_tankcap[n] = 0.0f;
    float fuelBefore = externalFuel;
    RecalculateFuel();
    float fuelDropped = fuelBefore - externalFuel;
    weight -= fuelDropped;
    mass    = weight / GRAVITY;
}

// recalculate the quick access.
void AirframeClass::RecalculateFuel ()
{
	//MI fix for refueling not filling up tanks
	if(IsSet(AirframeClass::Refueling))
		return;
    // recompute internal/external ammounts
    fuel = 0.0f;
    for (int i = 0; i <= TANK_MAXINTERNAL; i++)
	fuel += m_tanks[i];
    externalFuel = 0.0f;
    for (i = TANK_MAXINTERNAL+1; i < MAX_FUEL; i++)
	externalFuel += m_tanks[i];
}

// fuel dial stuff
void AirframeClass::GetFuel(float *fwdp, float *aftp, float *total)
{
    if (!g_bRealisticAvionics) {
	*fwdp = fuel;
	*aftp = externalFuel;
	*total = fuel + externalFuel;
	return;
    }
    else {
	float mply = 1;
	if(platform->IsF16()) mply = 10;

	*total = fuel + externalFuel;
	//MI fuel's in 100's of lbs
	*total = (((int)*total + 50) / 100) * 100;
	switch (fuelSwitch) {
	case FS_TEST:
	    *fwdp = *aftp = 2000*mply;
	    *total = 6000;
	    break;
	default:
	case FS_NORM:
	    *fwdp = m_tanks[TANK_FWDRES] + m_tanks[TANK_F1] + m_tanks[TANK_WINGFR];
	    *aftp = m_tanks[TANK_AFTRES] + m_tanks[TANK_A1] + m_tanks[TANK_WINGAL];
	    *fwdp *= mply;
	    *aftp *= mply;
	    break;
	case FS_RESV:
	    *fwdp = m_tanks[TANK_FWDRES];
	    *aftp = m_tanks[TANK_AFTRES];
	    *fwdp *= mply;
	    *aftp *= mply;
	    break;
	case FS_WINGINT:
	    *fwdp = m_tanks[TANK_WINGFR];
	    *aftp = m_tanks[TANK_WINGAL];
	    *fwdp *= mply;
	    *aftp *= mply;
	    break;
	case FS_WINGEXT:
	    *fwdp = m_tanks[TANK_REXT];
	    *aftp = m_tanks[TANK_LEXT];
	    *fwdp *= mply;
	    *aftp *= mply;
	    break;
	case FS_CENTEREXT:
	    *fwdp = m_tanks[TANK_CLINE];
	    *aftp = 0.0f;
	    *fwdp *= mply;
	    *aftp *= mply;
	    break;
	}
    }
}

// fuel display switch
void AirframeClass::IncFuelSwitch()
{
    if (fuelSwitch == FS_LAST)
	fuelSwitch = FS_FIRST;
    else fuelSwitch = (FuelSwitch)(((int)fuelSwitch)+1);
    if (fuelSwitch == FS_TEST) {
		if(!g_bRealisticAvionics)
		{
			platform->mFaults->SetFault(fwd_fuel_low_fault);
			platform->mFaults->SetFault(aft_fuel_low_fault);
		}
		else
		{
			platform->mFaults->SetCaution(fwd_fuel_low_fault);
			platform->mFaults->SetCaution(aft_fuel_low_fault);
		}
    }
}

void AirframeClass::DecFuelSwitch()
{
    if (fuelSwitch == FS_FIRST)
	fuelSwitch = FS_LAST;
    else 
	fuelSwitch = (FuelSwitch)(((int)fuelSwitch)-1);
    if (fuelSwitch == FS_TEST) {
		if(!g_bRealisticAvionics)
		{
			platform->mFaults->SetFault(fwd_fuel_low_fault);
			platform->mFaults->SetFault(aft_fuel_low_fault);
		}
		else
		{
			platform->mFaults->SetCaution(fwd_fuel_low_fault);
			platform->mFaults->SetCaution(aft_fuel_low_fault);
		}
    }
}
// fuel pump switch
void AirframeClass::IncFuelPump()
{
    if (fuelPump == FP_LAST)
	fuelPump = FP_FIRST;
    else 
	fuelPump = (FuelPump)(((int)fuelPump)+1);
}

void AirframeClass::DecFuelPump()
{
    if (fuelPump == FP_FIRST)
	fuelPump = FP_LAST;
    else fuelPump = (FuelPump)(((int)fuelPump)-1);
}

// air source switch
void AirframeClass::IncAirSource()
{
    if (airSource == AS_LAST)
	airSource = AS_FIRST;
    else 
	airSource = (AirSource)(((int)airSource)+1);
}

void AirframeClass::DecAirSource()
{
    if (airSource == AS_FIRST)
	airSource = AS_LAST;
    else airSource = (AirSource)(((int)airSource)-1);
}

// JPO check for trapped fuel
// 5 conditions to be met on the real jet, lets see how close we can get.
// 1. Fuel display must be in normal
// 2. Air refueling not happened in the last 90 seconds
// 3. Fuselage fuel 500lbs < capacity for 30 seconds
// 4. Total fuel 500lbs > fuselage fuel for 30 seconds
// 5. Fuel Flow < 18000pph for 30 seconds.
int AirframeClass::CheckTrapped() 
{
    if (fuelSwitch != FS_NORM ) return 0; // cond 1
    if (externalFuel < 500) return 0; // cond 4
    if (fuelFlow > 18000) return 0; // cond 5

    float fuscap = m_tankcap[TANK_FWDRES] + m_tankcap[TANK_F1] + m_tankcap[TANK_WINGFR]
	+ m_tankcap[TANK_AFTRES] + m_tankcap[TANK_A1] + m_tankcap[TANK_WINGAL];
    if (fuel > fuscap - 500) return 0; // cond 3
    // TODO 30 second timer 
    return 1;
}
//MI Home fuel
int AirframeClass::CheckHome(void)
{
	//Calc how much fuel we have at our selected homepoint
	if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp)
	{
		WayPointClass *wp = platform->GetWayPointNo(
			OTWDriver.pCockpitManager->mpIcp->HomeWP);
		float wpX, wpY, wpZ;
		if(wp)
		{
			wp->GetLocation(&wpX, &wpY, &wpZ);
			//Calculate the distance to it
			float deltaX = wpX - platform->XPos();
			float deltaY = wpY - platform->YPos();
			float distanceToSta	= (float)sqrt(deltaX * deltaX + deltaY * deltaY);
			float fuelConsumed;
			if (!IsSet(InAir)) // JPO - when we're on the runway or something.
					fuelConsumed = 0;
			else
					fuelConsumed = distanceToSta / platform->Vt() * FuelFlow() / 3600.0F;
			HomeFuel =  (int)(platform->GetTotalFuel() - fuelConsumed);

			if(platform->IsF16())
			{
	 			int	fuelOnStation;
	 			float fuelConsumed	= distanceToSta / 6000.0f * 10.0f * 0.67f;
				fuelConsumed += min(1,distanceToSta / 6000.0f / 80.0f) *(500.0f - (-platform->ZPos()) /40.0f*0.5f);
	 			fuelOnStation = (int)(platform->GetTotalFuel() - fuelConsumed);
				HomeFuel = fuelOnStation;
			}

			if(HomeFuel < 800)
				return 1;	//here we get a warning
			else
				return 0;	//here not
		 }
	}
	return 0;	//dummy
}

float AirframeClass::GetJoker()
{
	float jokerfactor = auxaeroData->jokerFactor;
    return GetAeroData(AeroDataSet::InternalFuel) / jokerfactor; // default 2.0 = about 3500 for F-16
}

float AirframeClass::GetBingo()
{
	float bingofactor = auxaeroData->bingoFactor;
    return GetAeroData(AeroDataSet::InternalFuel) / bingofactor;// default 5.0 = about 1500 for F-16
}

float AirframeClass::GetFumes()
{
	float fumesfactor = auxaeroData->fumesFactor;
    return GetAeroData(AeroDataSet::InternalFuel) / fumesfactor; // default 15.0 = about 500 for F-16
}