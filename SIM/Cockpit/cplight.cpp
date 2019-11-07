#include "stdafx.h"
#include "cpmanager.h"
#include "cplight.h"
//MI
#include "aircrft.h"
#include "simdrive.h"

//====================================================//
// CPLight::CPLight
//====================================================//

CPLight::CPLight(ObjectInitStr *pobjectInitStr, LightButtonInitStr *plightInitStr) : CPObject(pobjectInitStr) {
	
	mStates		= plightInitStr->states;
	mpSrcRect	= plightInitStr->psrcRect;
	mState		= plightInitStr->initialState;
	//MI
	WasPersistant = FALSE;
}

//====================================================//
// CPLight::~CPLight
//====================================================//

CPLight::~CPLight() {

	delete [] mpSrcRect;
}

//====================================================//
// CPLight::Exec
//====================================================//

void CPLight::Exec(SimBaseClass* pOwnship) { 

	mpOwnship = pOwnship;

#if 1
	if(mExecCallback) {
		mExecCallback(this);
	}
#else		// For testing
	if(mState == CPLIGHT_ON) {
		mState = CPLIGHT_OFF;
	}
	else {
		mState = CPLIGHT_ON;
	}
#endif

	SetDirtyFlag(); //VWF FOR NOW
}

//====================================================//	
// CPLight::Display
//====================================================//

void CPLight::DisplayBlit(void) {

	mDirtyFlag = TRUE;

	if(!mDirtyFlag || !SimDriver.playerEntity) {
		return;
	}

	if(mState >= mStates) {
		mState = 0;
	}

//F4Assert(mState < mStates);
	// Non-Persistant and If the state is not off, 
	// i.e. not CPLIGHT_OFF, CPLIGHT_AOA_OFF or 
	// CPLIGHT_AR_NWS_OFF

	//MI check for electrics
	if((((AircraftClass*)(SimDriver.playerEntity))->MainPower() == AircraftClass::MainPowerOff)
		&& mPersistant == 3)
	{
		//restore our original state
		if(mState)
		{ 
			if(mTransparencyType == CPTRANSPARENT) 
			{
				mpOTWImage->ComposeTransparent(mpTemplate, &mpSrcRect[mState], &mDestRect);
			}
			else
			{
				mpOTWImage->Compose(mpTemplate, &mpSrcRect[mState], &mDestRect);
			}
		} 
		return;
	}
	else if((((AircraftClass*)(SimDriver.playerEntity))->MainPower() == AircraftClass::MainPowerOff)
		&& mPersistant != 3)
	{
		//restore our original state
		if(WasPersistant)
		{
			mPersistant = 3;
			WasPersistant = FALSE;
		}
		return;
	}
	else if((((AircraftClass*)(SimDriver.playerEntity))->MainPower() == AircraftClass::MainPowerBatt))
	{
		if(mState)
		{ 
			if(mTransparencyType == CPTRANSPARENT) 
			{
				mpOTWImage->ComposeTransparent(mpTemplate, &mpSrcRect[mState], &mDestRect);
			}
			else
			{
				mpOTWImage->Compose(mpTemplate, &mpSrcRect[mState], &mDestRect);
			}
		} 
		return;
	}
	else if((((AircraftClass*)(SimDriver.playerEntity))->MainPower() == AircraftClass::MainPowerMain)
		&& mPersistant == 3)
	{
		//make them go away
		mPersistant = 0;
		//remember which one it was
		WasPersistant = TRUE;
	}

	if(((AircraftClass*)(SimDriver.playerEntity))->TestLights && mPersistant !=3 && !WasPersistant)
	{
		mState = TRUE;
	}
	
	if(mState || mPersistant)
	{
		if(mTransparencyType == CPTRANSPARENT) 
		{
			mpOTWImage->ComposeTransparent(mpTemplate, &mpSrcRect[mState], &mDestRect);
		}
		else
		{
			mpOTWImage->Compose(mpTemplate, &mpSrcRect[mState], &mDestRect);
		}
	}

	mDirtyFlag = FALSE;
}