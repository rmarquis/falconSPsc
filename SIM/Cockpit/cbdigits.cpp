#include "stdafx.h"
#include "stdhdr.h"
#include "airframe.h"
#include "aircrft.h"
#include "cpmanager.h"
#include "cpdigits.h"
#include "simdrive.h"
#include "cphsi.h"
#include "navsystem.h"
#include "fsound.h"
#include "falcsnd\voicemanager.h"
#include "flightdata.h"	//MI

#define FUEL_INTANK_DIGITS 5
#define MAX_INTANKS_VAL 80000

//MI
extern bool g_bRealisticAvionics;

void CBEChaffCount(void * pObject) {

	CPDigits*		pCPDigits	= (CPDigits*) pObject;

	if(g_bRealisticAvionics)
	{
		if(((AircraftClass*)(SimDriver.playerEntity))->HasPower(AircraftClass::ChaffFlareCount))
			pCPDigits->active = TRUE;
		else
			pCPDigits->active = FALSE;
	}
	pCPDigits->SetDigitValues(((AircraftClass*)(pCPDigits->mpCPManager->mpOwnship))->counterMeasureStation[CHAFF_STATION].weaponCount);
}


void CBEFlareCount(void * pObject) {

	CPDigits*	pCPDigits = (CPDigits*) pObject;

	if(g_bRealisticAvionics)
	{
		if(((AircraftClass*)(SimDriver.playerEntity))->HasPower(AircraftClass::ChaffFlareCount))
			pCPDigits->active = TRUE;
		else
			pCPDigits->active = FALSE;
	}

	pCPDigits->SetDigitValues(((AircraftClass*)(pCPDigits->mpCPManager->mpOwnship))->counterMeasureStation[FLARE_STATION].weaponCount);
}

void CBEHSIRange(void * pObject) {

	CPDigits*	pCPDigits = (CPDigits*) pObject;

	pCPDigits->SetDigitValues((long) (OTWDriver.pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_DISTANCE_TO_BEACON)));
}

void CBEHSISelectedCourse(void * pObject) {

	CPDigits*	pCPDigits = (CPDigits*) pObject;

	pCPDigits->SetDigitValues((long) (OTWDriver.pCockpitManager->mpHsi->GetValue(CPHsi::HSI_VAL_DESIRED_CRS)));
}


void CBETotalFuel(void * pObject) {

	CPDigits*		pCPDigits = (CPDigits*) pObject;
#if 0
	float				totalFuel;

	totalFuel		= ((AircraftClass *)pCPDigits->mpOwnship)->af->ExternalFuel() + ((AircraftClass *)pCPDigits->mpOwnship)->af->Fuel();

	if(totalFuel > MAX_INTANKS_VAL){
		pCPDigits->SetDigitValues(80000);
	}
	else{
		pCPDigits->SetDigitValues((long) totalFuel);
	}
#else // JPO get new fuel values.
	float  fwd, aft, total;
	((AircraftClass *)pCPDigits->mpOwnship)->af->GetFuel(&fwd, &aft, &total);
	//MI extracting
	((AircraftClass *)pCPDigits->mpOwnship)->af->GetFuel(&cockpitFlightData.fwd, 
		&cockpitFlightData.aft, &cockpitFlightData.total);
	pCPDigits->SetDigitValues((long) total);
#endif
}


void CBEUHFDigit(void * pObject) {

	CPDigits*		pCPDigits = (CPDigits*) pObject;
	long				channel;
	long				filter;

	if(VM)
	{
		if(OTWDriver.pCockpitManager->mpIcp->GetICPTertiaryMode() == COMM1_MODE) {
			filter = VM->GetRadioFreq(0);
		}
		else {
			filter = VM->GetRadioFreq(1);
		}
	}
	else
		filter = 0;

	if(OTWDriver.pCockpitManager->mpIcp->GetICPTertiaryMode() == COMM1_MODE) {
		channel = 10;
	}
	else {
		channel = 20;
	}
	
	pCPDigits->SetDigitValues(channel + filter);
}
