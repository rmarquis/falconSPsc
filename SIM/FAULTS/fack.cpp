#include "stdhdr.h"
#include "fault.h"
#include "fack.h"
#include "debuggr.h"
#include "soundfx.h"
#include "fsound.h"
#include "simdrive.h"
#include "aircrft.h"
#include "cmpclass.h"
#include "flightData.h"

extern bool g_bRealisticAvionics;
//-------------------------------------------------
// FackClass::FackClass
//-------------------------------------------------

FackClass::FackClass() {

   mMasterCaution = 0;
   NeedsWarnReset = 0;	//MI Warn reset switch
   DidManWarnReset = 0;	//MI Warn reset switch
	NeedAckAvioncFault = FALSE;
}

FackClass::~FackClass() {
}

//-------------------------------------------------
// FackClass::IsFlagSet
//-------------------------------------------------
	
BOOL	FackClass::IsFlagSet() {
	
	return mCautions.IsFlagSet();
}

//-------------------------------------------------
// FackClass::ClearFlag
//-------------------------------------------------
void	FackClass::ClearFlag() {
	MonoPrint("remove call\n");
}

//-------------------------------------------------
// FackClass::SetFault
//-------------------------------------------------

void	FackClass::SetFault(int systemBits, BOOL doWarningMsg)
{
FaultClass::type_FSubSystem subSystem = mFaults.PickSubSystem (systemBits);
FaultClass::type_FFunction	function = mFaults.PickFunction(subSystem);

   SetFault (subSystem, function, FaultClass::fail, doWarningMsg);
}

void	FackClass::SetFault(FaultClass::type_FSubSystem	subsystem,
								  FaultClass::type_FFunction	function,
								  FaultClass::type_FSeverity	severity,
								  BOOL								doWarningMsg) {
	if (!SimDriver.playerEntity) return;
	FaultClass::str_FEntry	entry;

	ShiAssert(doWarningMsg == 0 || SimDriver.playerEntity->mFaults == this); // should only apply to us
	GetFault(subsystem, &entry);

   // Set the fault
	mFaults.SetFault(subsystem, function, severity, doWarningMsg);

   // Adjust needed cautions
	if(entry.elFunction == FaultClass::nofault) 
	{
		if(subsystem == FaultClass::eng_fault) 
		{
			mCautions.SetCaution(engine);
		}
		else if(subsystem == FaultClass::iff_fault) 
		{
			mCautions.SetCaution(iff_fault);
		}		
	}
	if(!g_bRealisticAvionics)
	{
		mMasterCaution = TRUE;
		NeedsWarnReset = TRUE; //MI Warn Reset
	}
	else if (doWarningMsg)
	{
		if(subsystem == FaultClass::amux_fault || 
			subsystem == FaultClass::blkr_fault ||
			subsystem == FaultClass::bmux_fault ||
			subsystem == FaultClass::cadc_fault ||
			subsystem == FaultClass::cmds_fault ||
			subsystem == FaultClass::dlnk_fault ||
			subsystem == FaultClass::dmux_fault ||
			subsystem == FaultClass::dte_fault ||
			subsystem == FaultClass::eng_fault ||
			subsystem == FaultClass::epod_fault ||
			subsystem == FaultClass::fcc_fault ||
			subsystem == FaultClass::fcr_fault ||
			subsystem == FaultClass::flcs_fault ||
			subsystem == FaultClass::fms_fault ||
			subsystem == FaultClass::gear_fault ||
			subsystem == FaultClass::gps_fault ||
			subsystem == FaultClass::harm_fault ||
			subsystem == FaultClass::hud_fault ||
			subsystem == FaultClass::iff_fault ||
			subsystem == FaultClass::ins_fault ||
			subsystem == FaultClass::isa_fault ||
			subsystem == FaultClass::mfds_fault ||
			subsystem == FaultClass::msl_fault ||
			subsystem == FaultClass::ralt_fault ||
			subsystem == FaultClass::rwr_fault ||
			subsystem == FaultClass::sms_fault ||
			subsystem == FaultClass::tcn_fault ||
			subsystem == FaultClass::ufc_fault)
		{
			SimDriver.playerEntity->NeedsToPlayCaution = TRUE;//caution
			SetMasterCaution();	//set our MasterCaution immediately
			SimDriver.playerEntity->WhenToPlayCaution = vuxGameTime + 7*CampaignSeconds;
			NeedAckAvioncFault = TRUE;
		}
		else
		{
		/*//these are warnings
		if(function == FaultClass::dual ||
			function == FaultClass::efire ||
			function == FaultClass::hydr)
		{ */
			SimDriver.playerEntity->NeedsToPlayWarning = TRUE;// warning
			if(!SimDriver.playerEntity->NeedsToPlayWarning)
				SimDriver.playerEntity->WhenToPlayWarning = vuxGameTime + (unsigned long) 1.5*CampaignSeconds;
			SetWarnReset();
		}
	}
}

//-------------------------------------------------
// FackClass::SetFault
//-------------------------------------------------

void	FackClass::SetFault(type_CSubSystem subsystem) 
{
	if (!SimDriver.playerEntity) 
		return;

	ShiAssert(SimDriver.playerEntity->mFaults == this); // should only apply to us
	if(!mCautions.GetCaution(subsystem)) 
	{
		mCautions.SetCaution(subsystem);

		// No Master Caution for low_altitude warming - just bitchin' betty :-) - RH
		if (subsystem != alt_low)
		{
			if(!g_bRealisticAvionics)
			{
				mMasterCaution = TRUE;
				NeedsWarnReset = TRUE;
			}
			else
			{
	 			//these are warnings
				if (GetFault(tf_fail) ||	//never get's set currently
					GetFault(obs_wrn) || //never get's set currently
					GetFault(eng_fire) ||
					GetFault(hyd) ||
					GetFault(oil_press) ||
					GetFault(dual_fc) ||
					GetFault(to_ldg_config))
				{
					if(!SimDriver.playerEntity->NeedsToPlayWarning)
						SimDriver.playerEntity->WhenToPlayWarning = vuxGameTime + (unsigned long) 1.5*CampaignSeconds;
					SimDriver.playerEntity->NeedsToPlayWarning = TRUE;// warning
					SetWarnReset();
				}
				//these are actually cautions
				if(subsystem != fuel_low_fault)
				{
					if(GetFault(stores_config_fault) ||
						GetFault(flt_cont_fault) ||
						GetFault(le_flaps_fault) ||
						GetFault(engine) ||
						GetFault(overheat_fault) ||
						GetFault(avionics_fault) ||
						GetFault(radar_alt_fault) ||
						GetFault(iff_fault) ||
						GetFault(ecm_fault) ||
						GetFault(hook_fault) ||
						GetFault(nws_fault) ||
						GetFault(cabin_press_fault) ||
						GetFault(fwd_fuel_low_fault) ||
						GetFault(aft_fuel_low_fault) ||
						GetFault(probeheat_fault) ||
						GetFault(seat_notarmed_fault) ||
						GetFault(buc_fault) ||
						GetFault(fueloil_hot_fault) ||
						GetFault(anti_skid_fault) ||
						GetFault(nws_fault) ||
						GetFault(oxy_low_fault)||
						GetFault(sec_fault) ||
						GetFault(lef_fault))
					{
						if(!SimDriver.playerEntity->NeedsToPlayCaution && 
							!cockpitFlightData.IsSet(FlightData::MasterCaution))
							{ 
								SimDriver.playerEntity->WhenToPlayCaution = vuxGameTime + 7*CampaignSeconds;
							}
						SimDriver.playerEntity->NeedsToPlayCaution = TRUE;//caution
						SetMasterCaution();	//set our MasterCaution immediately
					}
				}
				if(subsystem == fuel_low_fault)	//MI need flashing on HUD
					SetWarnReset();
			}
		}
	}
}

//-------------------------------------------------
// FackClass::ClearFault
//-------------------------------------------------

void	FackClass::ClearFault(FaultClass::type_FSubSystem subsystem) {

	mFaults.ClearFault(subsystem);

	if(subsystem == FaultClass::eng_fault) {
		mCautions.ClearCaution(engine);
	}
	else if(subsystem == FaultClass::iff_fault) {
		mCautions.ClearCaution(iff_fault);
	}
}

//-------------------------------------------------
// FackClass::ClearFault
//-------------------------------------------------
void	FackClass::ClearFault(type_CSubSystem subsystem) {

	if(g_bRealisticAvionics)
	{
		//warnings
		if(!GetFault(tf_fail) &&	//never get's set currently
			!GetFault(obs_wrn) && //never get's set currently
			!GetFault(eng_fire) &&
			!GetFault(hyd) &&
			!GetFault(oil_press) &&
			!GetFault(dual_fc) &&
			!GetFault(to_ldg_config) &&
			!GetFault(fuel_low_fault) && 
			!GetFault(fuel_trapped) && 
			!GetFault(fuel_home))
		{
			ClearWarnReset();
		}
		//Cautions
		if(!GetFault(stores_config_fault) &&
			!GetFault(flt_cont_fault) &&
			!GetFault(le_flaps_fault) &&
			!GetFault(engine) &&
			!GetFault(overheat_fault) &&
			!GetFault(avionics_fault) &&
			!GetFault(radar_alt_fault) &&
			!GetFault(iff_fault) &&
			!GetFault(ecm_fault) &&
			!GetFault(hook_fault) &&
			!GetFault(nws_fault) &&
			!GetFault(cabin_press_fault) &&
			!GetFault(fwd_fuel_low_fault) &&
			!GetFault(aft_fuel_low_fault) &&
			!GetFault(probeheat_fault) &&
			!GetFault(seat_notarmed_fault) &&
			!GetFault(buc_fault) &&
			!GetFault(fueloil_hot_fault) &&
			!GetFault(anti_skid_fault) &&
			!GetFault(nws_fault) &&
			!GetFault(oxy_low_fault)&&
			!GetFault(sec_fault) &&
			!GetFault(elec_fault) &&
			!GetFault(lef_fault) &&
			!NeedAckAvioncFault) 
		{
			ClearMasterCaution();
			
		}
	}
	mCautions.ClearCaution(subsystem);
}

//-------------------------------------------------
// FackClass::GetFault
//-------------------------------------------------

void	FackClass::GetFault(FaultClass::type_FSubSystem subsystem, FaultClass::str_FEntry* pentry) {

	mFaults.GetFault(subsystem, pentry);
}

//-------------------------------------------------
// FackClass::GetFault
//-------------------------------------------------

BOOL	FackClass::GetFault(FaultClass::type_FSubSystem subsystem) {

	return mFaults.GetFault(subsystem);
}

//-------------------------------------------------
// FackClass::GetFault
//-------------------------------------------------

BOOL	FackClass::GetFault(type_CSubSystem subsystem) {
	
	return mCautions.GetCaution(subsystem);
}

//-------------------------------------------------
// FackClass::GetFaultNames
//-------------------------------------------------

void	FackClass::GetFaultNames(FaultClass::type_FSubSystem subsystem, int funcNum, FaultClass::str_FNames* pnames) {

	mFaults.GetFaultNames(subsystem, funcNum, pnames);
}

void	FackClass::TotalPowerFailure ()
{
    mFaults.TotalPowerFailure(); // JPO
	//MI need to route these thru the appropriate function
	//since we have electrics in non realistic mode, we have to go this way....
	if(g_bRealisticAvionics)
	{
		SetCaution(radar_alt_fault);
		SetCaution(le_flaps_fault);
		SetCaution(hook_fault);
		SetCaution(nws_fault);
		SetCaution(ecm_fault);
		SetCaution(iff_fault);
	}
	else
	{
		mCautions.SetCaution(radar_alt_fault);
		mCautions.SetCaution(le_flaps_fault);
		mCautions.SetCaution(hook_fault);
		mCautions.SetCaution(nws_fault);
		mCautions.SetCaution(ecm_fault);
		mCautions.SetCaution(iff_fault);
	}

	if(!g_bRealisticAvionics)
		mMasterCaution = TRUE;
	/*if(!SimDriver.playerEntity->NeedsToPlayCaution)
		SimDriver.playerEntity->WhenToPlayCaution = vuxGameTime + 7*CampaignSeconds;
	SimDriver.playerEntity->NeedsToPlayCaution = TRUE;*/
}
//MI
void	FackClass::SetWarning(type_CSubSystem subsystem) 
{
	if (!SimDriver.playerEntity) 
		return;

	ShiAssert(SimDriver.playerEntity->mFaults == this); // should only apply to us
	if(!mCautions.GetCaution(subsystem)) 
	{
		mCautions.SetCaution(subsystem);

		if(!SimDriver.playerEntity->NeedsToPlayWarning)
			SimDriver.playerEntity->WhenToPlayWarning = vuxGameTime + (unsigned long) 1.5*CampaignSeconds;
		if(!GetFault(fuel_low_fault) &&
			!GetFault(fuel_home))//no betty for bingo
			SimDriver.playerEntity->NeedsToPlayWarning = TRUE;// warning
		SetWarnReset();
	}
}
void	FackClass::SetCaution(type_CSubSystem subsystem) 
{
	if (!SimDriver.playerEntity) 
		return;

	ShiAssert(SimDriver.playerEntity->mFaults == this); // should only apply to us
	if(!mCautions.GetCaution(subsystem)) 
	{
		mCautions.SetCaution(subsystem);

		if(!SimDriver.playerEntity->NeedsToPlayCaution && 
			!cockpitFlightData.IsSet(FlightData::MasterCaution))
		{ 
			SimDriver.playerEntity->WhenToPlayCaution = vuxGameTime + 7*CampaignSeconds;
		}
		SimDriver.playerEntity->NeedsToPlayCaution = TRUE;//caution
		SetMasterCaution();	//set our MasterCaution immediately
	}
}