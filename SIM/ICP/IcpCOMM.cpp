#include "icp.h"
#include "simdrive.h"
#include "aircrft.h"
#include "fsound.h"
#include "falcsnd\voicemanager.h"
#include "navsystem.h"

//MI for ICP stuff
extern bool g_bRealisticAvionics;

#if 0
char *RadioStrings[8] = 
{
				"OFF",
				"FLIGHT",				
				"PACKAGE",
				"FROM PACKAGE",
				"PROXIMITY",
				"GUARD",			
				"BROADCAST",
				"TOWER"
};

void ICPClass::ExecCOMMMode(void) 
{
	
	if(mUpdateFlags & COMM_UPDATE) 
	{
		mUpdateFlags &= !COMM_UPDATE;

		FormatRadioString();
	}
}
#endif

void ICPClass::ExecCOMM1Mode(void)
{

	//change our active radio
	WhichRadio = 0;
	if(VM)
	{
		if(VM->radiofilter[0] == rcfOff)
			CommChannel = 0;
		else if(VM->radiofilter[0] == rcfFlight)
			CommChannel = 1;
		else if(VM->radiofilter[0] == rcfPackage)
			CommChannel = 2;
		else if(VM->radiofilter[0] == rcfFromPackage)
			CommChannel = 3;
		else if(VM->radiofilter[0] == rcfProx)
			CommChannel = 4;
		else if(VM->radiofilter[0] == rcfTeam)
			CommChannel = 5;
		else if(VM->radiofilter[0] == rcfAll)
			CommChannel = 6;
		else if(VM->radiofilter[0] == rcfTower)
			CommChannel = 7;
	}
	else
		CommChannel = 8;

	if(PREUHF == 1)
		UHFChann = 125.32;
	else if(PREUHF == 2)
		UHFChann = 106.95;
	else if(PREUHF == 3)
		UHFChann = 140.75;
	else if(PREUHF == 4)
		UHFChann = 123.62;
	else if(PREUHF == 5)
		UHFChann = 142.02;
	else if(PREUHF == 6)
		UHFChann = 185.62;
	else if(PREUHF == 7)
		UHFChann = 162.80;
		

	if(gNavigationSys->GetControlSrc() == NavigationSystem::AUXCOMM)
		UHFBackup();
	else
	{
		//Line1
		FillDEDMatrix(0,10,"UFH  BOTH");
		//Line2
		sprintf(tempstr,"%d",CommChannel);
		FillDEDMatrix(1,9,tempstr);
		//Line3
		PossibleInputs = 5;
		ScratchPad(2,15,22);
		//Line4
		FillDEDMatrix(3,2,"PRE");
		sprintf(tempstr, "%1.0f", PREUHF);
		FillDEDMatrix(3,10,tempstr);
		FillDEDMatrix(3,13,"\x01");
		FillDEDMatrix(3,19, "TOD");
		//Line5
		sprintf(tempstr,"%3.2f",UHFChann);
		FillDEDMatrix(4,6,tempstr);
		FillDEDMatrix(4,20,"NB");
	}
}
void ICPClass::ExecCOMM2Mode(void)
{
	//change our active radio
	WhichRadio = 1;
	if(VM)
	{
		if(VM->radiofilter[1] == rcfOff)
			CommChannel = 0;
		else if(VM->radiofilter[1] == rcfFlight)
		{
			VHFChann = 145.27;
			CommChannel = 1;
		}
		else if(VM->radiofilter[1] == rcfPackage)
		{
			VHFChann = 222.10;
			CommChannel = 2;
		}
		else if(VM->radiofilter[1] == rcfFromPackage)
		{
			VHFChann = 147.25;
			CommChannel = 3;
		}
		else if(VM->radiofilter[1] == rcfProx)
		{
			VHFChann = 256.32;
			CommChannel = 4;
		}
		else if(VM->radiofilter[1] == rcfTeam)
		{
			VHFChann = 186.35;
			CommChannel = 5;
		}
		else if(VM->radiofilter[1] == rcfAll)
		{
			VHFChann = 198.97;
			CommChannel = 6;
		}
		else if(VM->radiofilter[1] == rcfTower)
		{
			VHFChann = 176.35;
			CommChannel = 7;
		}
	}
	else
		CommChannel = 8;

	if(PREVHF == 1)
		VHFChann = 40.39;
	else if(PREVHF == 2)
		VHFChann = 36.72;
	else if(PREVHF == 3)
		VHFChann = 29.05;
	else if(PREVHF == 4)
		VHFChann = 45.62;
	else if(PREVHF == 5)
		VHFChann = 27.02;
	else if(PREVHF == 6)
		VHFChann = 53.62;
	else if(PREVHF == 7)
		VHFChann = 58.80;

	if(gNavigationSys->GetControlSrc() == NavigationSystem::AUXCOMM)
		VHFBackup();
	else
	{
		//Line1
		FillDEDMatrix(0,10,"VHF  ON");
		//Line2
		FillDEDMatrix(1,3,"512.26");
		//Line3
		PossibleInputs = 5;
		ScratchPad(2,15,22);
		//Line4
		FillDEDMatrix(3,2,"PRE");
		sprintf(tempstr, "%1.0f", PREVHF);
		FillDEDMatrix(3,10,tempstr);
		FillDEDMatrix(3,13,"\x01");
		//Line5
		sprintf(tempstr, "%3.2f", VHFChann);
		FillDEDMatrix(4,6,tempstr);
		FillDEDMatrix(4,20,"WB");
	}
}
void ICPClass::PNUpdateCOMMMode(int button, int)
{
	if(!g_bRealisticAvionics)
	{
		//MI Original Code
		if(button == PREV_BUTTON) 
		{
			if(gNavigationSys->GetUHFSrc() == NavigationSystem::UHF_NORM) {

				if(GetICPTertiaryMode() == COMM1_MODE) 
				{
					VM->BackwardCycleFreq(0);
				}
				else 
				{
					VM->BackwardCycleFreq(1);
				}
			}
		}
		else 
		{
			if(gNavigationSys->GetUHFSrc() == NavigationSystem::UHF_NORM) 
			{
				if(GetICPTertiaryMode() == COMM1_MODE) 
				{
					VM->ForwardCycleFreq(0);
				}
				else 
				{
					VM->ForwardCycleFreq(1);
				}
			}
		}

		mUpdateFlags |= CNI_UPDATE;
	}
	else
	{
		//MI Modified stuff
		if(button == PREV_BUTTON) 
		{
			if(gNavigationSys->GetUHFSrc() == NavigationSystem::UHF_NORM) 
			{
				if(IsICPSet(ICPClass::EDIT_UHF))
					VM->BackwardCycleFreq(0);
				else if(IsICPSet(ICPClass::EDIT_VHF))
					VM->BackwardCycleFreq(1);
			}
		}
		else 
		{
			if(gNavigationSys->GetUHFSrc() == NavigationSystem::UHF_NORM) 
			{
				if(IsICPSet(ICPClass::EDIT_UHF))
					VM->ForwardCycleFreq(0);
				else if(IsICPSet(ICPClass::EDIT_VHF))
					VM->ForwardCycleFreq(1);
			}
		}
	}
}
void ICPClass::ENTRUpdateCOMMMode() 
{

}

