#include "stdhdr.h"
#include "aircrft.h"
#include "fack.h"
#include "airframe.h"
#include "soundfx.h"
#include "fsound.h"
#include "fcc.h"
#include "radar.h"
#include "falcmesg.h"
#include "simdrive.h"
#include "flightData.h"
#include "otwdrive.h"
#include "cpmanager.h"
#include "sms.h"
#include "playerop.h"
#include "limiters.h"
#include "IvibeData.h"

/*!GetFault(obs_wrn) && //never get's set currently*/

//extern bool g_bEnableAircraftLimits; //me123	MI replaced with g_bRealisticAvionics
extern bool g_bRealisticAvionics;

//MI
bool Warned = FALSE;
//extern bool g_bEnableCATIIIExtension;	//MI replaced with g_bRealisticAvionics
void AircraftClass::CautionCheck (void)
{
	if(!isDigital)
   {
      // Check fuel
      if (af->Fuel() + af->ExternalFuel() < bingoFuel && !mFaults->GetFault(FaultClass::fms_fault))
      {
		  if(g_bRealisticAvionics)
		  {
			  //MI added for ICP stuff.
			  //rewritten 04/21/01
#if 0
			  bingoFuel = bingoFuel * 0.5F;
			  //Update our ICP readout
			  if(OTWDriver.pCockpitManager->mpIcp->IsICPSet(ICPClass::BINGO_MODE))
				  OTWDriver.pCockpitManager->mpIcp->ExecBingo();
			  if (bingoFuel < 100.0F)
				  bingoFuel = -1.0F;
			  cockpitFlightData.SetLightBit(FlightData::FuelLow);
			  mFaults->SetFault(fuel_low_fault); 
			  mFaults->SetMasterCaution();
			  F4SoundFXSetDist( af->auxaeroData->sndBBBingo, TRUE, 0.0f, 1.0f );
#else
			  //Only warn us if we've not already been warned.
			  if(!mFaults->GetFault(fuel_low_fault))
			  {
				  cockpitFlightData.SetLightBit(FlightData::FuelLow);
				  //mFaults->SetFault(fuel_low_fault);
				  mFaults->SetWarning(fuel_low_fault);
				  if(F4SoundFXPlaying( af->auxaeroData->sndBBBingo))
					  F4SoundFXSetDist( af->auxaeroData->sndBBBingo, TRUE, 0.0f, 1.0f );
			  }
#endif
		  }
		  else
		  {
			  //me123 let's set a bingo manualy
			  bingoFuel =   100.0f;
			  if (af->Fuel() <=  100.0F)
				  bingoFuel = -10.0F;
			  cockpitFlightData.SetLightBit(FlightData::FuelLow);
			  mFaults->SetFault(fuel_low_fault); 
			  mFaults->SetMasterCaution();
			  if(!F4SoundFXPlaying(af->auxaeroData->sndBBBingo))
				  F4SoundFXSetDist( af->auxaeroData->sndBBBingo, TRUE, 0.0f, 1.0f );
		  }

      }
	  //MI reset our fuel low fault if we set a bingo value below our current level
	  else if(g_bRealisticAvionics && mFaults->GetFault(fuel_low_fault))
	  {
		  if(bingoFuel < af->Fuel() + af->ExternalFuel())
		  {
			  cockpitFlightData.ClearLightBit(FlightData::FuelLow);
			  mFaults->ClearFault(fuel_low_fault);
		  }
	  }

	  // Caution TO/LDG Config
	  //MI
	  if(IsF16())
	  {
		  if(ZPos() > -10000.0F && Kias() < 190.0F && ZDelta() * 60.0F >= 250.0F && af->gearPos != 1.0F)
		  {
			  if(!mFaults->GetFault(to_ldg_config))
			  {
				  if(!g_bRealisticAvionics)
					  mFaults->SetFault(to_ldg_config);
				  else
					  mFaults->SetWarning(to_ldg_config);
			  }
		  }
		  else
			  mFaults->ClearFault(to_ldg_config);
		  
		  // JPO check for trapped fuel
		  if (!mFaults->GetFault(FaultClass::fms_fault) && af->CheckTrapped())
		  {
			  if(!mFaults->GetFault(fuel_trapped))
			  {
				  if(!g_bRealisticAvionics)
					  mFaults->SetFault(fuel_trapped);				  
				  else
					  mFaults->SetWarning(fuel_trapped);
			  }
		  }
		  else 
			  mFaults->ClearFault(fuel_trapped);

		  //MI Fuel HOME warning		  
		  if(!mFaults->GetFault(FaultClass::fms_fault) && af->CheckHome())
		  {
			  if(!mFaults->GetFault(fuel_home))
			  {
				  if(!g_bRealisticAvionics)
					  mFaults->SetFault(fuel_home);
				  else
					  mFaults->SetWarning(fuel_home);
				  //Make noise
				  if(!F4SoundFXPlaying(af->auxaeroData->sndBBBingo))
					  F4SoundFXSetDist( af->auxaeroData->sndBBBingo, TRUE, 0.0f, 1.0f );
			  }
		  }
		  else 
			  mFaults->ClearFault(fuel_home);
	  }

/////////////me123 let's brake something if we fly too fast
	  //me123 OWLOOK switch here to enable aircraft limits (overg and max speed)
	  //if (g_bEnableAircraftLimits) {	MI
	  if(g_bRealisticAvionics)
	  {
		  // Marco Edit - OverG DOES NOT affect		!!!
		  // (at least not before the aircraft falls apart)
		  //MI put back in after discussing with Marco
		  CheckForOverG();
		  CheckForOverSpeed();
	  }
	  // save for later.
		int savewarn = mFaults->WarnReset();
		int savemc = mFaults->MasterCaution();
	  //// JPO - check hydraulics too.
///////////
		if((af->rpm * 37.0F) < 15.0F || mFaults->GetFault(FaultClass::eng_fault))
		{
			if(!mFaults->GetFault(oil_press))
			{
				if(!g_bRealisticAvionics)
					// less than 15 psi
					mFaults->SetFault(oil_press);
				else
					mFaults->SetWarning(oil_press);
			}
		}
		else 
			mFaults->ClearFault(oil_press);

		if(!af->HydraulicOK())
		{
			if(!mFaults->GetFault(hyd))
			{
				if(!g_bRealisticAvionics)
					mFaults->SetFault(hyd);
				else
					mFaults->SetWarning(hyd);
			}
		}
		else
			mFaults->ClearFault(hyd);

		// JPO Sec is active below 20% rpm
		if(af->rpm < 0.20F) 
		{
			if(!mFaults->GetFault(sec_fault))
			{
				if(!g_bRealisticAvionics)
					mFaults->SetFault(sec_fault);
				else
					mFaults->SetCaution(sec_fault);
			}
		}
		else 
		    mFaults->ClearFault(sec_fault);

		// this is a hack JPO
		// when starting up we don't want to set the warn/caution lights,
		// but we do want the indicator lights.
		// so we clear the cautions if nothing else had them set.
		if (af->rpm < 1e-2 && OnGround()) { 
		    if (savewarn == 0) mFaults->ClearWarnReset();
		    if (savemc == 0) mFaults->ClearMasterCaution();
		}

#if 0 // JPO: I don't think this makes any sense to me... me123????
///////////me123 changed 27000 to -27000
		if(ZPos() < -27000.0F && mFaults->GetFault(FaultClass::eng_fault)) 
		{
			if(!mFaults->GetFault(to_ldg_config))
				mFaults->SetFault(cabin_press_fault);
		}
		else
			mFaults->ClearFault(cabin_press_fault);
#endif
		// JPO - dump dumps cabin pressure, off means its not there anyway.
		// 10000 is a guess - thats where you requirte oxygen
		if (ZPos() < -10000 && (af->GetAirSource() == AirframeClass::AS_DUMP || 
		    af->GetAirSource() == AirframeClass::AS_OFF))
		{
			if(!mFaults->GetFault(cabin_press_fault))
			{
				if(!g_bRealisticAvionics)
					mFaults->SetFault(cabin_press_fault);
				else
					mFaults->SetCaution(cabin_press_fault);
			}
		}
		else
		    mFaults->ClearFault(cabin_press_fault);

		if(mFaults->GetFault(FaultClass::hud_fault) && mFaults->GetFault(FaultClass::fcc_fault))
		{
			if(!mFaults->GetFault(canopy))
			{
				if(!g_bRealisticAvionics)
					mFaults->SetFault(canopy);
				else
					mFaults->SetWarning(canopy);
			}
		}
		else
			mFaults->ClearFault(canopy);
///////////
		if(mFaults->GetFault(FaultClass::fcc_fault))
		{
			if(!mFaults->GetFault(dual_fc))
			{
				if(!g_bRealisticAvionics)
					mFaults->SetFault(dual_fc);
				else
					mFaults->SetWarning(dual_fc);
			}
		}
		else
			mFaults->ClearFault(dual_fc);
///////////
		if(mFaults->GetFault( FaultClass::amux_fault) || mFaults->GetFault(FaultClass::bmux_fault))
		{
			if(!g_bRealisticAvionics)
				mFaults->SetFault(avionics_fault);
			else
				mFaults->SetCaution(avionics_fault);
		}
		else 
		{
			mFaults->ClearFault(avionics_fault);
		}
////////////
		if(mFaults->GetFault(FaultClass::ralt_fault))
		{
			if(!mFaults->GetFault(radar_alt_fault))
			{
				if(!g_bRealisticAvionics)
					mFaults->SetFault(radar_alt_fault);
				else
					mFaults->SetCaution(radar_alt_fault);
			}
		}
		else
			mFaults->ClearFault(radar_alt_fault);
///////////////
		if(mFaults->GetFault(FaultClass::iff_fault))
		{
			if(!mFaults->GetFault(iff_fault))
			{
				if(!g_bRealisticAvionics)
					mFaults->SetFault(iff_fault);
				else
					mFaults->SetCaution(iff_fault);
			}
		}
		else
			mFaults->ClearFault(iff_fault);
///////////////
		if(mFaults->GetFault(FaultClass::rwr_fault))
		{
			if(!mFaults->GetFault(ecm_fault))
			{
				if(!g_bRealisticAvionics)
					mFaults->SetFault(ecm_fault);
				else
					mFaults->SetCaution(ecm_fault);
			}
		}
		else
			mFaults->ClearFault(ecm_fault);
///////////////
		if(mFaults->GetFault(FaultClass::rwr_fault))
		{
			if(!mFaults->GetFault(nws_fault))
			{
				if(!g_bRealisticAvionics)
					mFaults->SetFault(nws_fault);
				else
					mFaults->SetCaution(nws_fault);
			}
		}
		else
			mFaults->ClearFault(nws_fault);
/////////////
		//MI
		// Overheat Fault
		if (mFaults->GetFault(FaultClass::eng_fault) && af->rpm <= 0.75)
		{
			if(!mFaults->GetFault(overheat_fault))
			{
				if(!g_bRealisticAvionics)
					mFaults->SetFault(overheat_fault);
				else
					mFaults->SetCaution(overheat_fault);
			}
		}
		else
			mFaults->ClearFault(overheat_fault);


// if lg up and aoa and speed
// if airbrakes on
		//MI what kind of bullshit is this anyway?????
		if(!g_bRealisticAvionics)
		{
			if(mFaults->GetFault(FaultClass::rwr_fault))
			{
				mFaults->SetFault(to_ldg_config);
			}
			else {
				mFaults->ClearFault(to_ldg_config);
			}
		}
//////////////
		// Set external data if this is Ownship and player
	  if (this == SimDriver.playerEntity)
		  SetExternalData();

      // AMUX and BMUX combined failure forces FCC into NAV
		if(mFaults->GetFault(FaultClass::amux_fault) && mFaults->GetFault(FaultClass::bmux_fault))
		{
         FCC->SetMasterMode (FireControlComputer::Nav);
		} 

      // If blanker broken, no ECM
      if (mFaults->GetFault(FaultClass::blkr_fault))
      {
         SensorClass* theRwr = FindSensor(this, SensorClass::RWR);

         if (theRwr)
            theRwr->SetPower (FALSE);
         UnSetFlag(ECM_ON);
      }

      // Shut down radar when broken
      if (mFaults->GetFault(FaultClass::fcc_fault) == FaultClass::xmtr)
      {
         RadarClass* theRadar = (RadarClass*)FindSensor(this, SensorClass::Radar);

         if (theRadar)
            theRadar->SetEmitting (FALSE);
      }

      if (mFaults->GetFault(FaultClass::fcc_fault) == FaultClass::bus)
      {
         RadarClass* theRadar = (RadarClass*)FindSensor(this, SensorClass::Radar);

         if (theRadar)
            theRadar->SetPower (FALSE);
      }

      // Shut down rwr when broken
      if (mFaults->GetFault(FaultClass::rwr_fault))
      {
         SensorClass* theRwr = FindSensor(this, SensorClass::RWR);

         if (theRwr)
            theRwr->SetPower (FALSE);
      }

      // Shut down HTS when broken
      if (mFaults->GetFault(FaultClass::harm_fault))
      {
         SensorClass* theHTS = FindSensor(this, SensorClass::HTS);

         if (theHTS)
            theHTS->SetPower (FALSE);
      }
	}
   //MI new home of the wrong/correct CAT stuff
   //if(af->platform->IsPlayer() && g_bEnableCATIIIExtension)	MI
   if(IsPlayer() && g_bRealisticAvionics)
   {
		float MaxG = af->curMaxGs;
		float limitGs= 6.5f;
		Limiter *limiter = NULL; // JPO - use dynamic figure , not 6.5
		
		
		if( limiter = gLimiterMgr->GetLimiter(CatIIIMaxGs, af->VehicleIndex()) )
		    limitGs = limiter->Limit(0);

		if(MaxG <= limitGs)
		{
			//we need CATIII
			if(!af->IsSet(AirframeClass::CATLimiterIII))
				WrongCAT();
			else 
				CorrectCAT();
		}
		else
		{
			//we don't need CATIII
			if(af->IsSet(AirframeClass::CATLimiterIII))
				WrongCAT();
			else 
				CorrectCAT();
		}
   }
   //MI Seat Arm switch
   if(IsPlayer() && g_bRealisticAvionics)
   {
	   if(!SeatArmed)
	   {
		   if(!mFaults->GetFault(seat_notarmed_fault))
			   mFaults->SetCaution(seat_notarmed_fault);
	   }
	   else
		   mFaults->ClearFault(seat_notarmed_fault);
   }
   if(g_bRealisticAvionics)
   {
	   //MI WARN Reset stuff
		 //me123 loopign warnign sound is just T_LCFG i think
	   if(cockpitFlightData.IsSet(FlightData::T_L_CFG))	//this one gives continous warning
	   {
		   //sound
		   if(mFaults->WarnReset())
		   {
			   if(vuxGameTime >= WhenToPlayWarning)
			   {
				   if(!F4SoundFXPlaying( SFX_BB_WARNING))
					   F4SoundFXSetDist(SFX_BB_WARNING, TRUE, 0.0f, 1.0f );
			   }
		   }
	   }
	   else
	   {
		   if(mFaults->DidManWarn())
		   {
			   mFaults->ClearManWarnReset();
			   mFaults->ClearWarnReset();
		   }
	   }
	   //MI Caution sound
	   if(NeedsToPlayCaution)
	   {
		   if(vuxGameTime >= WhenToPlayCaution)
		   {
		       if(mFaults->MasterCaution())
		       {
			   if(!F4SoundFXPlaying( af->auxaeroData->sndBBCaution))
			       F4SoundFXSetDist( af->auxaeroData->sndBBCaution, TRUE, 0.0f, 1.0f );
		       }
			   NeedsToPlayCaution = FALSE;
		   }
	   }
	   if(NeedsToPlayWarning)
	   {
		   if(vuxGameTime >= WhenToPlayWarning)
		   {
			   if(mFaults->WarnReset())
			   {
				   if(!F4SoundFXPlaying(af->auxaeroData->sndBBWarning))
					   F4SoundFXSetDist(af->auxaeroData->sndBBWarning, TRUE, 0.0f, 1.0f );
			   }
			   NeedsToPlayWarning = FALSE;					
		   }
	   }
	   //MI RF In SILENT gives TF FAIL
	   if(RFState == 2)
	   {
		   if(!mFaults->GetFault(tf_fail))
			   mFaults->SetWarning(tf_fail);
	   }
	   else
		   mFaults->ClearFault(tf_fail);
   }
}

//MI make some noise when overstressing
void AircraftClass::DamageSounds(void)
{
	int sound = rand()%5;
	sound++;
	switch (sound)
	{
	case 1:
		F4SoundFXSetDist(SFX_HIT_5, FALSE, 0.0f, 1.0f );
		break;
	case 2:
		F4SoundFXSetDist(SFX_HIT_4, FALSE, 0.0f, 1.0f );
		break;
	case 3:
		F4SoundFXSetDist(SFX_HIT_3, FALSE, 0.0f, 1.0f );
		break;
	case 4:
		F4SoundFXSetDist(SFX_HIT_2, FALSE, 0.0f, 1.0f );
		break;
	case 5:
		F4SoundFXSetDist(SFX_HIT_1, FALSE, 0.0f, 1.0f );
		break;
	default:
		break;
	}
}

//MI warn us when in wrong config
void AircraftClass::WrongCAT(void)
{
	if(Warned)
		return;
	//set our fault
	mFaults->SetCaution(stores_config_fault);
	//mark that we've been here
	Warned = TRUE;
}

//MI we've switched to the correct CAT
void AircraftClass::CorrectCAT(void)
{
	if(!Warned)
		return;
	//clear our fault
	mFaults->ClearFault(stores_config_fault);
	//mark that we've been here
	Warned = FALSE;
}

void AircraftClass::CheckForOverG(void)
{
	//check for bombs etc
	if(GetNz() > af->curMaxGs)
	{
		 if (this == FalconLocalSession->GetPlayerEntity())
		     g_intellivibeData.IsOverG = true;
		//let us know we're approaching overG
		GSounds();

		if (af->curMaxGs == 5.5 && GetNz() > af->curMaxGs + GToleranceBombs)
		{
			StoreToDamage(wcRocketWpn);
			StoreToDamage(wcBombWpn);
			StoreToDamage(wcAgmWpn);
			StoreToDamage(wcHARMWpn);
			StoreToDamage(wcSamWpn);
			StoreToDamage(wcGbuWpn);
		}

		//Tanks have 7G
		if ((af->curMaxGs == 7.0 || af->curMaxGs == 6.5) && GetNz() > af->curMaxGs + GToleranceTanks)
			if(GetNz() > af->curMaxGs + GToleranceTanks)
				StoreToDamage(wcTank);
	}
	else {
	    if (this == FalconLocalSession->GetPlayerEntity())
		g_intellivibeData.IsOverG = false;
	}
}

void AircraftClass::CheckForOverSpeed(void)
{
	if(af->curMaxGs == 5.5 && Kias() > af->curMaxStoreSpeed)
	{
		SSounds();
		
		if (Kias() > af->curMaxStoreSpeed + SpeedToleranceBombs)
		{
			StoreToDamage(wcRocketWpn);
			StoreToDamage(wcBombWpn);
			StoreToDamage(wcAgmWpn);
			StoreToDamage(wcHARMWpn);
			StoreToDamage(wcSamWpn);
			StoreToDamage(wcGbuWpn);
		}
	}

	if((af->curMaxGs == 7.0 || af->curMaxGs == 6.5) && Kias() > af->curMaxStoreSpeed)
	{
		GSounds();

		if(Kias() > af->curMaxStoreSpeed + SpeedToleranceTanks)
			StoreToDamage(wcTank);
	}
}

void AircraftClass::DoOverGSpeedDamage(int station)
{
	if(!mFaults)
		return;
	int damage = rand()%100;
	damage++;
	switch(station)
	{
	case 1:
		if(damage < 95 && !GetStationFailed(Station1_Degr) && !GetStationFailed(Station1_Fail))
		{
			mFaults->SetFault(FaultClass::sms_fault,FaultClass::sta1, FaultClass::degr, FALSE);
			StationFailed(Station1_Degr);
		}
		else
		{
			if(!GetStationFailed(Station1_Fail) && (GSoundsNFuel == 2 || GSoundsWFuel == 3))
				mFaults->SetFault(FaultClass::sms_fault,FaultClass::sta1, FaultClass::fail, FALSE);
			StationFailed(Station1_Fail);
		}
		break;
	case 2:
		damage = rand()%100;
		if(damage < 95 && !GetStationFailed(Station2_Degr) && !GetStationFailed(Station2_Fail))
		{
			mFaults->SetFault(FaultClass::sms_fault,FaultClass::sta2, FaultClass::degr, FALSE);
			StationFailed(Station2_Degr);
		}
		else
		{
			if(!GetStationFailed(Station2_Fail) && (GSoundsNFuel == 2 || GSoundsWFuel == 3))
				mFaults->SetFault(FaultClass::sms_fault,FaultClass::sta2, FaultClass::fail, FALSE);
			StationFailed(Station2_Fail);
		}
		break;
	case 3:
		damage = rand()%100;
		if(damage < 95 && !GetStationFailed(Station3_Degr) && !GetStationFailed(Station3_Fail))
		{
			mFaults->SetFault(FaultClass::sms_fault,FaultClass::sta3, FaultClass::degr, FALSE);
			StationFailed(Station3_Degr);
		}
		else
		{
			if(!GetStationFailed(Station3_Fail) && (GSoundsNFuel == 2 || GSoundsWFuel == 3))
				mFaults->SetFault(FaultClass::sms_fault,FaultClass::sta3, FaultClass::fail, FALSE);
			StationFailed(Station3_Fail);
		}
		break;
	case 4:
		damage = rand()%100;
		if(damage < 95 && !GetStationFailed(Station4_Degr) && !GetStationFailed(Station4_Fail))
		{
			mFaults->SetFault(FaultClass::sms_fault,FaultClass::sta4, FaultClass::degr, FALSE);
			StationFailed(Station4_Degr);
		}
		else
		{
			if(!GetStationFailed(Station4_Fail) && (GSoundsNFuel == 2 || GSoundsWFuel == 3))
				mFaults->SetFault(FaultClass::sms_fault,FaultClass::sta4, FaultClass::fail, FALSE);
			StationFailed(Station4_Fail);
		}
		break;
	case 5:
		damage = rand()%100;
		if(damage < 95 && !GetStationFailed(Station5_Degr) && !GetStationFailed(Station5_Fail))
		{
			mFaults->SetFault(FaultClass::sms_fault,FaultClass::sta5, FaultClass::degr, FALSE);
			StationFailed(Station5_Degr);
		}
		else
		{
			if(!GetStationFailed(Station5_Fail) && (GSoundsNFuel == 2 || GSoundsWFuel == 3))
				mFaults->SetFault(FaultClass::sms_fault,FaultClass::sta5, FaultClass::fail, FALSE);
			StationFailed(Station5_Fail);
		}
		break;
	case 6:
		damage = rand()%100;
		if(damage < 95 && !GetStationFailed(Station6_Degr) && !GetStationFailed(Station6_Fail))
		{
			mFaults->SetFault(FaultClass::sms_fault,FaultClass::sta6, FaultClass::degr, FALSE);
			StationFailed(Station6_Degr);
		}
		else
		{
			if(!GetStationFailed(Station6_Fail) && (GSoundsNFuel == 2 || GSoundsWFuel == 3)) 
				mFaults->SetFault(FaultClass::sms_fault,FaultClass::sta6, FaultClass::fail, FALSE);
			StationFailed(Station6_Fail);
		}
		break;
	case 7:
		damage = rand()%100;
		if(damage < 95 && !GetStationFailed(Station7_Degr) && !GetStationFailed(Station7_Fail))
		{
			mFaults->SetFault(FaultClass::sms_fault,FaultClass::sta7, FaultClass::degr, FALSE);
			StationFailed(Station7_Degr);
		}
		else
		{
			if(!GetStationFailed(Station7_Fail) && (GSoundsNFuel == 2 || GSoundsWFuel == 3))
				mFaults->SetFault(FaultClass::sms_fault,FaultClass::sta7, FaultClass::fail, FALSE);
			StationFailed(Station7_Fail);
		}
		break;
	case 8:
		damage = rand()%100;
		if(damage < 95  && !GetStationFailed(Station8_Degr) && !GetStationFailed(Station8_Fail))
		{
			mFaults->SetFault(FaultClass::sms_fault,FaultClass::sta8, FaultClass::degr, FALSE);
			StationFailed(Station8_Degr);
		}
		else
		{
			if(!GetStationFailed(Station8_Fail) && (GSoundsNFuel == 2 || GSoundsWFuel == 3))
				mFaults->SetFault(FaultClass::sms_fault,FaultClass::sta8, FaultClass::fail, FALSE);
			StationFailed(Station8_Fail);
		}
		break;
	case 9:
		damage = rand()%100;
		if(damage < 95 && !GetStationFailed(Station9_Degr) && !GetStationFailed(Station9_Fail))
		{
			mFaults->SetFault(FaultClass::sms_fault,FaultClass::sta9, FaultClass::degr, FALSE);
			StationFailed(Station9_Degr);
		}
		else
		{
			if(!GetStationFailed(Station9_Fail) && (GSoundsNFuel == 2 || GSoundsWFuel == 3))
				mFaults->SetFault(FaultClass::sms_fault,FaultClass::sta9, FaultClass::fail, FALSE);
			StationFailed(Station9_Fail);
		}
		break;
	default:
		break;
	}
}

void AircraftClass::StoreToDamage(WeaponClass thing)
{
	if (!g_bRealisticAvionics || (PlayerOptions.Realism < 0.76 && !isDigital))
		return;

	if(!Sms || !mFaults || !af)
		return;
	//Check which station to fail
	int center = (Sms->NumHardpoints() - 1) / 2 + 1;
	for(int i = 0; i < Sms->NumHardpoints(); i++)
	{
		// if its a tank - try and guess which one.
		if (Sms->hardPoint[i] && 
			Sms->hardPoint[i]->GetWeaponClass() == thing)
		{
			//tanks cause our Fuel Management System to fail.
			if(thing == wcTank)
			{
				if(GetNz() > af->curMaxGs + GToleranceTanks)
				{
					AdjustTankG(1);
					DoOverGSpeedDamage(i);
					if(!mFaults->GetFault(FaultClass::fms_fault))
						mFaults->SetFault(FaultClass::fms_fault,FaultClass::bus, FaultClass::degr, FALSE);
					if(GSoundsWFuel == 0)
					{
						DamageSounds();
						GSoundsWFuel = 2;
					}
				}
				if(GetNz() > af->curMaxGs + GToleranceTanks)
				{
					AdjustTankG(2);
					DoOverGSpeedDamage(i);
					if(!mFaults->GetFault(FaultClass::fms_fault))
						mFaults->SetFault(FaultClass::fms_fault,FaultClass::bus, FaultClass::fail, FALSE);
					if(GSoundsWFuel == 2)
					{
						DamageSounds();
						GSoundsWFuel++;
					}
				}
				if(GetNz() >= af->curMaxGs + GToleranceTanks)
				{
					AdjustTankG(3);
					DoOverGSpeedDamage(i);
					if(!mFaults->GetFault(FaultClass::flcs_fault))
						mFaults->SetFault(FaultClass::flcs_fault,FaultClass::sngl, FaultClass::fail, FALSE);
					if(GSoundsWFuel == 3)
					{
						DamageSounds();
						GSoundsWFuel++;
					}
				}
				if(Kias() > af->curMaxStoreSpeed + SpeedToleranceTanks)
				{
					AdjustTankSpeed(1);
					DoOverGSpeedDamage(i);
					if(!mFaults->GetFault(FaultClass::fms_fault))
						mFaults->SetFault(FaultClass::fms_fault,FaultClass::bus, FaultClass::fail, FALSE);
					if(SpeedSoundsWFuel == 0)
					{
						  DamageSounds();
						  SpeedSoundsWFuel++;
					}
				}
				if(Kias() > af->curMaxStoreSpeed + SpeedToleranceTanks)
				{
					AdjustTankSpeed(2);
					DoOverGSpeedDamage(i);
					if(!mFaults->GetFault(FaultClass::fms_fault))
						mFaults->SetFault(FaultClass::fms_fault,FaultClass::bus, FaultClass::fail, FALSE);
					if(SpeedSoundsWFuel == 1)
					{
						  DamageSounds();
						  SpeedSoundsWFuel++;
					}
				}
				if(af->mach >= 2.05f || Kias() > af->curMaxStoreSpeed + SpeedToleranceTanks)
				{
					AdjustTankSpeed(3);
					DoOverGSpeedDamage(i);
					if(!mFaults->GetFault(FaultClass::flcs_fault))
						mFaults->SetFault(FaultClass::flcs_fault,FaultClass::sngl, FaultClass::fail, FALSE);
					if(!mFaults->GetFault(FaultClass::cadc_fault))
						mFaults->SetFault(FaultClass::cadc_fault,FaultClass::bus, FaultClass::degr, FALSE);
					if(SpeedSoundsWFuel == 2)
					{
						DamageSounds();
						SpeedSoundsWFuel++;
					}
				}
			}
			if(thing == wcBombWpn || thing == wcRocketWpn || thing == wcAgmWpn ||
				thing == wcHARMWpn || thing == wcSamWpn || thing ==wcGbuWpn)
			{
				if(GetNz() > af->curMaxGs + GToleranceBombs)
				{
					AdjustBombG(1);
					DoOverGSpeedDamage(i);
					if(!mFaults->GetFault(FaultClass::isa_fault))
						mFaults->SetFault(FaultClass::isa_fault,FaultClass::sngl, FaultClass::fail, FALSE);
					if(GSoundsNFuel == 0)
					{
						DamageSounds();
						GSoundsNFuel = 2;
					}
				}
				if(GetNz() >= af->curMaxGs + GToleranceBombs)
				{
					AdjustBombG(2);
					DoOverGSpeedDamage(i);
					if(!mFaults->GetFault(FaultClass::sms_fault))
						mFaults->SetFault(FaultClass::sms_fault,FaultClass::bus, FaultClass::degr, FALSE);
					if(GSoundsNFuel == 2)
					{
						DamageSounds();
						GSoundsNFuel++;
					}
				}
				if(GetNz() >= af->curMaxGs + GToleranceBombs)
				{
					AdjustBombG(3);
					DoOverGSpeedDamage(i);
					if(!mFaults->GetFault(FaultClass::flcs_fault))
						mFaults->SetFault(FaultClass::flcs_fault,FaultClass::dual, FaultClass::fail, FALSE);
					if(!mFaults->GetFault(FaultClass::sms_fault) & FaultClass::bus & FaultClass::fail)
						mFaults->SetFault(FaultClass::sms_fault,FaultClass::bus, FaultClass::fail, FALSE);
					if(GSoundsNFuel == 3)
					{
						DamageSounds();
						GSoundsNFuel++;
					}
				}
				if(Kias() > af->curMaxStoreSpeed + SpeedToleranceBombs)
				{
					AdjustBombSpeed(1);
					DoOverGSpeedDamage(i);
					if(SpeedSoundsNFuel == 0)
					{
						  DamageSounds();
						  SpeedSoundsNFuel++;
					}
				}
				if(Kias() > af->curMaxStoreSpeed + SpeedToleranceBombs)
				{
					AdjustBombSpeed(2);
					DoOverGSpeedDamage(i);
					if(!mFaults->GetFault(FaultClass::fms_fault))
						mFaults->SetFault(FaultClass::fms_fault,FaultClass::bus, FaultClass::fail, FALSE);
					if(!mFaults->GetFault(FaultClass::isa_fault))
						mFaults->SetFault(FaultClass::isa_fault,FaultClass::sngl, FaultClass::fail, FALSE);
					if(SpeedSoundsNFuel == 1)
					{
						  DamageSounds();
						  SpeedSoundsNFuel++;
					}
				}
				if(af->mach >= 2.05f || Kias() > af->curMaxStoreSpeed + SpeedToleranceBombs)
				{
					AdjustBombSpeed(3);
					DoOverGSpeedDamage(i);
					if(!mFaults->GetFault(FaultClass::flcs_fault))
						mFaults->SetFault(FaultClass::flcs_fault,FaultClass::sngl, FaultClass::fail, FALSE);
					if(!mFaults->GetFault(FaultClass::cadc_fault))
						mFaults->SetFault(FaultClass::cadc_fault,FaultClass::bus, FaultClass::degr, FALSE);
					if(!mFaults->GetFault(FaultClass::amux_fault))
						mFaults->SetFault(FaultClass::amux_fault,FaultClass::bus, FaultClass::degr, FALSE);
					if(!mFaults->GetFault(FaultClass::fcc_fault))
						mFaults->SetFault(FaultClass::fcc_fault,FaultClass::bus, FaultClass::degr, FALSE);
					if(SpeedSoundsNFuel == 2)
					{
						DamageSounds();
						SpeedSoundsNFuel++;
					}
				}
			}
		}
	}
}

void AircraftClass::SetExternalData(void)
{
	// Master Caution
	if (OTWDriver.pCockpitManager->mMiscStates.GetMasterCautionLight())
		cockpitFlightData.SetLightBit(FlightData::MasterCaution);
	else
		cockpitFlightData.ClearLightBit(FlightData::MasterCaution);

	// Oil Pressure
	if((af->rpm * 37.0F) < 15.0F || mFaults->GetFault(FaultClass::eng_fault))
		cockpitFlightData.SetLightBit(FlightData::OIL);
	else
		cockpitFlightData.ClearLightBit(FlightData::OIL);

	if(!af->HydraulicOK())
		cockpitFlightData.SetLightBit(FlightData::HYD);
	else
		cockpitFlightData.ClearLightBit(FlightData::HYD);

	// Cabin Pressure
	if(ZPos() < 27000.0F && mFaults->GetFault(FaultClass::eng_fault))
		cockpitFlightData.SetLightBit(FlightData::CabinPress);
	else
		cockpitFlightData.ClearLightBit(FlightData::CabinPress);

	// Canopy Light
	if(mFaults->GetFault(FaultClass::hud_fault) && mFaults->GetFault(FaultClass::fcc_fault))
		cockpitFlightData.SetLightBit(FlightData::CAN);
	else
		cockpitFlightData.ClearLightBit(FlightData::CAN);

	// FLCS
	if(mFaults->GetFault(FaultClass::fcc_fault))
		cockpitFlightData.SetLightBit(FlightData::DUAL);
	else
		cockpitFlightData.ClearLightBit(FlightData::DUAL);

	// Avioncs Caution
	if(!g_bRealisticAvionics)
	{
		if(mFaults->GetFault(FaultClass::amux_fault) || mFaults->GetFault(FaultClass::bmux_fault))
			cockpitFlightData.SetLightBit(FlightData::Avionics);
		else
			cockpitFlightData.ClearLightBit(FlightData::Avionics);
	}
	else
	{
		if(mFaults->NeedAckAvioncFault)
			cockpitFlightData.SetLightBit(FlightData::Avionics);
		else
			cockpitFlightData.ClearLightBit(FlightData::Avionics);
	}

	// Radar altimeter
	if(mFaults->GetFault(radar_alt_fault))
		cockpitFlightData.SetLightBit(FlightData::RadarAlt);
	else
		cockpitFlightData.ClearLightBit(FlightData::RadarAlt);

	// IFF Fault
	if(mFaults->GetFault(FaultClass::iff_fault))
		cockpitFlightData.SetLightBit(FlightData::IFF);
	else
		cockpitFlightData.ClearLightBit(FlightData::IFF);
	
	// AOA Indicator lights
	if(cockpitFlightData.alpha >= 14.0F)	   
	{
		cockpitFlightData.SetLightBit(FlightData::AOAAbove);
		cockpitFlightData.ClearLightBit(FlightData::AOAOn);
		cockpitFlightData.ClearLightBit(FlightData::AOABelow);
	}
	else if((cockpitFlightData.alpha < 14.0F) && (cockpitFlightData.alpha >= 11.5F))
	{
		cockpitFlightData.ClearLightBit(FlightData::AOAAbove);
		cockpitFlightData.SetLightBit(FlightData::AOAOn);
		cockpitFlightData.ClearLightBit(FlightData::AOABelow);
	}
	else
	{
		cockpitFlightData.ClearLightBit(FlightData::AOAAbove);
		cockpitFlightData.ClearLightBit(FlightData::AOAOn);
		cockpitFlightData.SetLightBit(FlightData::AOABelow);
	}
	//MI only operational with gear down
	if(g_bRealisticAvionics && af && af->gearPos < 0.8F)
	{
		cockpitFlightData.ClearLightBit(FlightData::AOAOn);
		cockpitFlightData.ClearLightBit(FlightData::AOABelow);
		cockpitFlightData.ClearLightBit(FlightData::AOAAbove);
	}
	
	// Nose Wheel Steering
	if(af->IsSet(AirframeClass::NoseSteerOn))
	{
		cockpitFlightData.ClearLightBit(FlightData::RefuelRDY);
		cockpitFlightData.SetLightBit(FlightData::RefuelAR);
		cockpitFlightData.ClearLightBit(FlightData::RefuelDSC);	   
	}
	else
	{
		cockpitFlightData.ClearLightBit(FlightData::RefuelRDY);
		cockpitFlightData.ClearLightBit(FlightData::RefuelAR);
		cockpitFlightData.ClearLightBit(FlightData::RefuelDSC);
	}

	// FLCS
	if(mFaults->GetFault(FaultClass::flcs_fault))
		cockpitFlightData.SetLightBit(FlightData::FltControlSys);
	else
		cockpitFlightData.ClearLightBit(FlightData::FltControlSys);

	// Engine Fault
	if (mFaults->GetFault(FaultClass::eng_fault))
		cockpitFlightData.SetLightBit(FlightData::EngineFault);
	else
		cockpitFlightData.ClearLightBit(FlightData::EngineFault);

    // Overheat Fault
	if (mFaults->GetFault(FaultClass::eng_fault) && af->rpm <= 0.75)
		cockpitFlightData.SetLightBit(FlightData::Overheat);
	else
		cockpitFlightData.ClearLightBit(FlightData::Overheat);
	
	// These are not faults, they are cautions
	if (mFaults->GetFault(FaultClass::eng_fault) & FaultClass::efire)
		cockpitFlightData.SetLightBit(FlightData::ENG_FIRE);
	else
		cockpitFlightData.ClearLightBit(FlightData::ENG_FIRE);

	//MI
	if(mFaults->GetFault(stores_config_fault))
		cockpitFlightData.SetLightBit(FlightData::CONFIG);
	else
		cockpitFlightData.ClearLightBit(FlightData::CONFIG);

	// Caution TO/LDG Config		
	if(ZPos() > -10000.0F && Kias() < 190.0F && ZDelta() * 60.0F >= 250.0F && af->gearPos != 1.0F)
        cockpitFlightData.SetLightBit(FlightData::T_L_CFG);
    else
		cockpitFlightData.ClearLightBit(FlightData::T_L_CFG);

	if(mFaults->GetFault(nws_fault))
		cockpitFlightData.SetLightBit(FlightData::NWSFail);
	else
		cockpitFlightData.ClearLightBit(FlightData::NWSFail);

	if(mFaults->GetFault(cabin_press_fault))
		cockpitFlightData.SetLightBit(FlightData::CabinPress);
	else
		cockpitFlightData.ClearLightBit(FlightData::CabinPress);

	if(mFaults->GetFault(oxy_low_fault))
		cockpitFlightData.SetLightBit2(FlightData::OXY_LOW);
	else
		cockpitFlightData.ClearLightBit2(FlightData::OXY_LOW);

	if(mFaults->GetFault(fwd_fuel_low_fault))
		cockpitFlightData.SetLightBit2(FlightData::FwdFuelLow);
	else
		cockpitFlightData.ClearLightBit2(FlightData::FwdFuelLow);
	
	if(mFaults->GetFault(aft_fuel_low_fault))
		cockpitFlightData.SetLightBit2(FlightData::AftFuelLow);
	else
		cockpitFlightData.ClearLightBit2(FlightData::AftFuelLow);

	if(mFaults->GetFault(sec_fault))
		cockpitFlightData.SetLightBit2(FlightData::SEC);
	else
		cockpitFlightData.ClearLightBit2(FlightData::SEC);

	if(mFaults->GetFault(probeheat_fault))
		cockpitFlightData.SetLightBit2(FlightData::PROBEHEAT);
	else
		cockpitFlightData.ClearLightBit2(FlightData::PROBEHEAT);
	
	if(mFaults->GetFault(buc_fault))
		cockpitFlightData.SetLightBit2(FlightData::BUC);
	else
		cockpitFlightData.ClearLightBit2(FlightData::BUC);
	
	if(mFaults->GetFault(fueloil_hot_fault))
		cockpitFlightData.SetLightBit2(FlightData::FUEL_OIL_HOT);
	else
		cockpitFlightData.ClearLightBit2(FlightData::FUEL_OIL_HOT);
			
	if(mFaults->GetFault(anti_skid_fault))
		cockpitFlightData.SetLightBit2(FlightData::ANTI_SKID);
	else
		cockpitFlightData.ClearLightBit2(FlightData::ANTI_SKID);

	if(mFaults->GetFault(seat_notarmed_fault))
		cockpitFlightData.SetLightBit2(FlightData::SEAT_ARM);
	else
		cockpitFlightData.ClearLightBit2(FlightData::SEAT_ARM);

	if(af->EpuIsAir())
		cockpitFlightData.SetLightBit3(FlightData::Air);
    else
		cockpitFlightData.ClearLightBit3(FlightData::Air);

	if(af->EpuIsHydrazine())
		cockpitFlightData.SetLightBit3(FlightData::Hydrazine);
    else
		cockpitFlightData.ClearLightBit3(FlightData::Hydrazine);

	if(ElecIsSet(AircraftClass::ElecFlcsPmg))
		cockpitFlightData.SetLightBit3(FlightData::FlcsPmg);
    else
		cockpitFlightData.ClearLightBit3(FlightData::FlcsPmg);

    if(ElecIsSet(AircraftClass::ElecEpuGen))
		cockpitFlightData.SetLightBit3(FlightData::EpuGen);
    else
		cockpitFlightData.ClearLightBit3(FlightData::EpuGen);

    if(ElecIsSet(AircraftClass::ElecEpuPmg))
		cockpitFlightData.SetLightBit3(FlightData::EpuPmg);
    else
		cockpitFlightData.ClearLightBit3(FlightData::EpuPmg);

    if(ElecIsSet(AircraftClass::ElecToFlcs))
		cockpitFlightData.SetLightBit3(FlightData::ToFlcs);
    else
		cockpitFlightData.ClearLightBit3(FlightData::ToFlcs);

    if(ElecIsSet(AircraftClass::ElecFlcsRly))
		cockpitFlightData.SetLightBit3(FlightData::FlcsRly);
    else
		cockpitFlightData.ClearLightBit3(FlightData::FlcsRly);

    if(ElecIsSet(AircraftClass::ElecBatteryFail))
		cockpitFlightData.SetLightBit3(FlightData::BatFail);
	else
		cockpitFlightData.ClearLightBit3(FlightData::BatFail);

	if(af->IsSet(AirframeClass::JfsStart)) 
		cockpitFlightData.SetLightBit2(FlightData::JFSOn);
	else
		cockpitFlightData.ClearLightBit2(FlightData::JFSOn);

	// JPO Is EPU running.
	if(af->GeneratorRunning(AirframeClass::GenEpu))
	    cockpitFlightData.SetLightBit2(FlightData::EPUOn);
	else
	    cockpitFlightData.ClearLightBit2(FlightData::EPUOn);

	if(af->GeneratorRunning(AirframeClass::GenMain))
		cockpitFlightData.SetLightBit3(FlightData::MainGen);
	else
		cockpitFlightData.ClearLightBit3(FlightData::MainGen);
	
    if(af->GeneratorRunning(AirframeClass::GenStdby))
		cockpitFlightData.SetLightBit3(FlightData::StbyGen);
	else
		cockpitFlightData.ClearLightBit3(FlightData::StbyGen);

	//MI RF In SILENT gives TF FAIL
	if(RFState == 2)
		cockpitFlightData.SetLightBit(FlightData::TF);
	else
		cockpitFlightData.ClearLightBit(FlightData::TF);

	if(mFaults->GetFault(elec_fault))
		cockpitFlightData.SetLightBit3(FlightData::Elec_Fault);
	else
		cockpitFlightData.ClearLightBit3(FlightData::Elec_Fault);

	if(mFaults->GetFault(alt_low))
		cockpitFlightData.SetLightBit(FlightData::ALT);
	else
		cockpitFlightData.ClearLightBit(FlightData::ALT);

	if(mFaults->GetFault(le_flaps_fault))
		cockpitFlightData.SetLightBit(FlightData::LEFlaps);
	else
		cockpitFlightData.ClearLightBit(FlightData::LEFlaps);

	if(mFaults->GetFault(ecm_fault))
		cockpitFlightData.SetLightBit(FlightData::ECM);
	else
		cockpitFlightData.ClearLightBit(FlightData::ECM);

	if(mFaults->GetFault(hook_fault))
		cockpitFlightData.SetLightBit(FlightData::Hook);
	else
		cockpitFlightData.ClearLightBit(FlightData::Hook);

	if(mFaults->GetFault(lef_fault))
		cockpitFlightData.SetLightBit3(FlightData::Lef_Fault);
	else
		cockpitFlightData.ClearLightBit3(FlightData::Lef_Fault);
}

void AircraftClass::GSounds(void)
{
	//not if we're going down
	if(!IsExploding() && !IsDead())
		F4SoundFXSetDist(af->auxaeroData->sndOverSpeed1, TRUE, 0.0f, 1.0f);
}

void AircraftClass::SSounds(void)
{
	if(!IsExploding() && !IsDead())
		F4SoundFXSetDist(af->auxaeroData->sndOverSpeed2, TRUE, 0.0f, (Kias() - af->curMaxStoreSpeed) / 25);
}

void AircraftClass::AdjustTankSpeed(int level)
{
	//adjust for OverG/Speed
	switch(level)
	{
	case 1:
		SpeedToleranceTanks = OverSpeedToleranceTanks[1];
		break;
	case 2:
		SpeedToleranceTanks = OverSpeedToleranceTanks[2];
		break;
	case 3:
		SpeedToleranceTanks = 100;	//No more damage
		break;
	default:
		break;
	}
}

void AircraftClass::AdjustBombSpeed(int level)
{
	//adjust for OverG/Speed
	switch(level)
	{
	case 1:
		SpeedToleranceBombs = OverSpeedToleranceBombs[1];
		break;
	case 2:
		SpeedToleranceBombs = OverSpeedToleranceBombs[2];
		break;
	case 3:
		SpeedToleranceBombs = 100;	//No more damage
		break;
	default:
		break;
	}
}

void AircraftClass::AdjustTankG(int level)
{
	//adjust for OverG/Speed
	switch(level)
	{
	case 1:
		GToleranceTanks = float(OverGToleranceTanks[1]) / 10.0;
		break;
	case 2:
		GToleranceTanks = float(OverGToleranceTanks[2]) / 10.0;
		break;
	case 3:
		GToleranceTanks = 100;	//No more damage
		break;
	default:
		break;
	}
}

void AircraftClass::AdjustBombG(int level)
{
	//adjust for OverG/Speed
	switch(level)
	{
	case 1:
		GToleranceBombs = float(OverGToleranceBombs[1]) / 10.0;
		break;
	case 2:
		GToleranceBombs = float(OverGToleranceBombs[2]) / 10.0;
		break;
	case 3:
		GToleranceBombs = 100;	//No more damage
		break;
	default:
		break;
	}
}