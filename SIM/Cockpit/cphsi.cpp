#include "stdafx.h"
#include "stdhdr.h"
#include "Graphics\Include\display.h"
#include "airframe.h"
#include "campwp.h"
#include "simdrive.h"
#include "aircrft.h"
#include "cphsi.h"
#include "cpmanager.h"
#include "tacan.h"
#include "navsystem.h"
#include "dispopts.h"
#include "fack.h"
#include "otwdrive.h"
#include "Graphics\Include\renderow.h"
#include "Graphics\Include\tod.h"
#include "UI\INCLUDE\tac_class.h"
#include "ui\include\te_defs.h"
#include "fakerand.h"

//MI 02/02/02
extern bool g_bRealisticAvionics;
extern bool g_bINS;

//------------------------------------------------------
// CPHsi::CPHsi()
//------------------------------------------------------

CPHsi::CPHsi() {

	mpHsiStates[HSI_STA_CRS_STATE]				= NORMAL_HSI_CRS_STATE;
	mpHsiStates[HSI_STA_HDG_STATE]				= NORMAL_HSI_HDG_STATE;

	mpHsiFlags[HSI_FLAG_TO_TRUE]					= FALSE;
	mpHsiFlags[HSI_FLAG_ILS_WARN]					= FALSE;
	mpHsiFlags[HSI_FLAG_CRS_WARN]					= FALSE;
	mpHsiFlags[HSI_FLAG_INIT]						= FALSE;
	lastCheck = 0;
	lastResult = FALSE;

	///VWF HACK: The following is a hack to make the Tac Eng Instrument
	// Landing Mission agree with the Manual

	if(SimDriver.RunningTactical() && 
		current_tactical_mission &&
		current_tactical_mission->get_type() == tt_training &&
		SimDriver.playerEntity &&
		!strcmpi(current_tactical_mission->get_title(), "10 Instrument Landing"))
	{
		mpHsiValues[HSI_VAL_DESIRED_CRS]			= 340.0F;
	}
	else {
		mpHsiValues[HSI_VAL_DESIRED_CRS]			= 0.0F;
	}
	mpHsiValues[HSI_VAL_CRS_DEVIATION]			= 0.0F;
	mpHsiValues[HSI_VAL_DISTANCE_TO_BEACON]	= 0.0F;
	mpHsiValues[HSI_VAL_BEARING_TO_BEACON]		= 0.0F;
	mpHsiValues[HSI_VAL_CURRENT_HEADING]		= 0.0;
	mpHsiValues[HSI_VAL_DESIRED_HEADING]		= 0.0F;
	mpHsiValues[HSI_VAL_DEV_LIMIT]				= 10.0F;
	mpHsiValues[HSI_VAL_HALF_DEV_LIMIT]			= mpHsiValues[HSI_VAL_DEV_LIMIT] * 0.5F;
	mpHsiValues[HSI_VAL_LOCALIZER_CRS]			= 0.0F;
	mpHsiValues[HSI_VAL_AIRBASE_X]				= 0.0F;
	mpHsiValues[HSI_VAL_AIRBASE_Y]				= 0.0F;

	mLastWaypoint										= NULL;
	mLastMode											= NavigationSystem::TOTAL_MODES;

	//MI 02/02/02
	LastHSIHeading = 0.0F;
}
////////////////////////////////////////////////////////



//------------------------------------------------------
// CPHsi::GetState
//------------------------------------------------------

int CPHsi::GetState(HSIButtonStates stateIndex) {

	return mpHsiStates[stateIndex];
}
////////////////////////////////////////////////////////



//------------------------------------------------------
// CPHsi::GetValue
//------------------------------------------------------

float	CPHsi::GetValue(HSIValues valueIndex) {

	return mpHsiValues[valueIndex];
}
////////////////////////////////////////////////////////



//------------------------------------------------------
// CPHsi::GetFlag
//------------------------------------------------------

BOOL CPHsi::GetFlag(HSIFlags flagIndex) {

	return mpHsiFlags[flagIndex];
}
////////////////////////////////////////////////////////



//------------------------------------------------------
// CPHsi::IncState
//------------------------------------------------------

void CPHsi::IncState(HSIButtonStates state) {

	HSIValues	valueIndex;
	int			normalState;
	
	if(state == HSI_STA_CRS_STATE) {
		valueIndex	= HSI_VAL_DESIRED_CRS;
		normalState = NORMAL_HSI_CRS_STATE;
	}
	else {
		valueIndex	= HSI_VAL_DESIRED_HEADING;
		normalState = NORMAL_HSI_HDG_STATE;
	}

	mpHsiValues[valueIndex] += 5.0F;

	if(mpHsiValues[valueIndex] >= 360.0F) {
		mpHsiValues[valueIndex] = 0.0F;
	}
	
	if(mpHsiStates[state] == normalState) {
		mpHsiStates[state] = 0;
	}
	else {
		mpHsiStates[state]++;
	}
}
////////////////////////////////////////////////////////



//------------------------------------------------------
// CPHsi::DecState
//------------------------------------------------------

void CPHsi::DecState(HSIButtonStates state) {

	HSIValues	valueIndex;
	int			normalState;
	
	if(state == HSI_STA_CRS_STATE) {
		valueIndex	= HSI_VAL_DESIRED_CRS;
		normalState = NORMAL_HSI_CRS_STATE;
	}
	else {
		valueIndex	= HSI_VAL_DESIRED_HEADING;
		normalState = NORMAL_HSI_HDG_STATE;
	}

	mpHsiValues[valueIndex] -= 5.0F;

	if(mpHsiValues[valueIndex] < 0.0F) {
		mpHsiValues[valueIndex] = 360.0F;
	}
	
	if(mpHsiStates[state] == 0) {
		mpHsiStates[state] = normalState;
	}
	else {
		mpHsiStates[state]--;
	}
}
////////////////////////////////////////////////////////



//------------------------------------------------------
// CPHsi::ExecNav()
//------------------------------------------------------

void CPHsi::ExecNav(void) {

	float				x;
	float				y; 
	float				z;
	WayPointClass	*pcurrentWaypoint;

	if(mpHsiFlags[HSI_FLAG_INIT] == FALSE) {

		mpHsiValues[HSI_VAL_DEV_LIMIT]		= 10.0F;
		mpHsiValues[HSI_VAL_HALF_DEV_LIMIT]	= mpHsiValues[HSI_VAL_DEV_LIMIT] * 0.5F;
		mpHsiFlags[HSI_FLAG_INIT]				= TRUE;
		mpHsiFlags[HSI_FLAG_TO_TRUE]			= FALSE;
	}
	
	if(SimDriver.playerEntity) {
	    pcurrentWaypoint	= SimDriver.playerEntity->curWaypoint;
	    if(F4IsBadReadPtr(pcurrentWaypoint, sizeof *pcurrentWaypoint)) { // JPO CTD check
		
		mpHsiFlags[HSI_FLAG_CRS_WARN]			= TRUE;
		ExecBeaconProximity(SimDriver.playerEntity->XPos(), SimDriver.playerEntity->YPos(), 0.0F, 0.0F);
		CalcTCNCrsDev(mpHsiValues[HSI_VAL_DESIRED_CRS]);
	    }
	    else {
		
		mpHsiFlags[HSI_FLAG_CRS_WARN]			= FALSE;
		pcurrentWaypoint->GetLocation(&x, &y, &z);
		ExecBeaconProximity(SimDriver.playerEntity->XPos(), SimDriver.playerEntity->YPos(), x, y);
		CalcTCNCrsDev(mpHsiValues[HSI_VAL_DESIRED_CRS]);
	    }
	if (mpHsiValues[HSI_VAL_CRS_DEVIATION] < 270 && mpHsiValues[HSI_VAL_CRS_DEVIATION] > 90)
		mpHsiFlags[HSI_FLAG_TO_TRUE] = 2;
	    else
		mpHsiFlags[HSI_FLAG_TO_TRUE] = TRUE;
	}

}
////////////////////////////////////////////////////////



//------------------------------------------------------
// CPHsi::ExecTacan
//------------------------------------------------------

void CPHsi::ExecTacan(void) {
    
    float							ownshipX;
    float							ownshipY;
    float							tacanX;
    float							tacanY, tacanZ;
    float							tacanRange;
    
    if(mpHsiFlags[HSI_FLAG_INIT] == FALSE) {
	
	mpHsiValues[HSI_VAL_DEV_LIMIT]		= 10.0F;
	mpHsiValues[HSI_VAL_HALF_DEV_LIMIT]	= mpHsiValues[HSI_VAL_DEV_LIMIT] * 0.5F;
	mpHsiFlags[HSI_FLAG_INIT]				= TRUE;
	mpHsiFlags[HSI_FLAG_ILS_WARN]			= TRUE;
	mpHsiFlags[HSI_FLAG_TO_TRUE]			= TRUE;
    }
    
    if(SimDriver.playerEntity) {
	
	ownshipX = SimDriver.playerEntity->XPos();
	ownshipY = SimDriver.playerEntity->YPos();
	
	mpHsiFlags[HSI_FLAG_CRS_WARN]	= !gNavigationSys->GetTCNPosition(&tacanX, &tacanY, &tacanZ);
	gNavigationSys->GetTCNAttribute(NavigationSystem::RANGE, &tacanRange);
	
	if(gNavigationSys->IsTCNTanker() || gNavigationSys->IsTCNAirbase() || gNavigationSys->IsTCNCarrier()) {	// now Carrier support
	    mpHsiFlags[HSI_FLAG_ILS_WARN]			= FALSE;
	}
	else {
	    mpHsiFlags[HSI_FLAG_ILS_WARN]			= 	mpHsiFlags[HSI_FLAG_CRS_WARN];
	}
	
	ExecBeaconProximity(ownshipX, ownshipY, tacanX, tacanY);
	if (BeaconInRange(mpHsiValues[HSI_VAL_DISTANCE_TO_BEACON], tacanRange)) {
	    CalcTCNCrsDev(mpHsiValues[HSI_VAL_DESIRED_CRS]);
	}
	else {
	    mpHsiValues[HSI_VAL_BEARING_TO_BEACON]=90; // fix at 90 deg off
	    mpHsiValues[HSI_VAL_DISTANCE_TO_BEACON] = 0;
	    mpHsiFlags[HSI_FLAG_ILS_WARN]			= TRUE;
	    mpHsiFlags[HSI_FLAG_CRS_WARN]			= TRUE;
	}
	if (mpHsiValues[HSI_VAL_CRS_DEVIATION] < 270 && mpHsiValues[HSI_VAL_CRS_DEVIATION] > 90)
	    mpHsiFlags[HSI_FLAG_TO_TRUE] = 2;
	else
	    mpHsiFlags[HSI_FLAG_TO_TRUE] = TRUE;	    
    }
}
////////////////////////////////////////////////////////



void CPHsi::ExecILSNav(void) {
    float						ownshipX;
    float						ownshipY;
    WayPointClass			*pcurrentWaypoint;
    float						waypointX;
    float						waypointY;
    float						waypointZ;
    float						gpDew;
    VU_ID						ilsObj;
    
    if(mpHsiFlags[HSI_FLAG_INIT] == FALSE) {
	
	mpHsiValues[HSI_VAL_DEV_LIMIT]		= 10.0F;
	mpHsiValues[HSI_VAL_HALF_DEV_LIMIT]	= mpHsiValues[HSI_VAL_DEV_LIMIT] * 0.5F;
	mpHsiFlags[HSI_FLAG_INIT]				= TRUE;
	mpHsiFlags[HSI_FLAG_TO_TRUE]			= FALSE;
    }
    
    if(	SimDriver.playerEntity) {
	
	pcurrentWaypoint	= SimDriver.playerEntity->curWaypoint;
	ownshipX = SimDriver.playerEntity->XPos();
	ownshipY = SimDriver.playerEntity->YPos();
	
	if(pcurrentWaypoint == NULL) {
	    
	    mpHsiFlags[HSI_FLAG_CRS_WARN]			= TRUE;
	    ExecBeaconProximity(ownshipX,ownshipY, 0.0F, 0.0F);
	}
	else {
	    pcurrentWaypoint->GetLocation(&waypointX, &waypointY, &waypointZ);
	    ExecBeaconProximity(SimDriver.playerEntity->XPos(), SimDriver.playerEntity->YPos(), waypointX, waypointY);
	    mpHsiFlags[HSI_FLAG_ILS_WARN]			= !gNavigationSys->GetILSAttribute(NavigationSystem::GP_DEV, &gpDew);
	    CalcILSCrsDev(gpDew);
	}
	if (mpHsiValues[HSI_VAL_CRS_DEVIATION] < 270 && mpHsiValues[HSI_VAL_CRS_DEVIATION] > 90)
	    mpHsiFlags[HSI_FLAG_TO_TRUE] = 2;
	else
	    mpHsiFlags[HSI_FLAG_TO_TRUE] = TRUE;
    }
}


//------------------------------------------------------
// CPHsi::ExecBeaconProximity
//------------------------------------------------------

void CPHsi::ExecBeaconProximity(float x1, float y1, float x2, float y2) {
    
    double	xdiff, ydiff;
    double	bearingToBeacon;
    
    xdiff = x2 - x1;
    ydiff = y2 - y1;
    if(	SimDriver.playerEntity) {
	
	bearingToBeacon	= atan2(xdiff, ydiff); // radians +-pi, xaxis = 0deg
	
	mpHsiValues[HSI_VAL_BEARING_TO_BEACON]		= ConvertRadtoNav((float)bearingToBeacon);
	mpHsiValues[HSI_VAL_DISTANCE_TO_BEACON]	= (float)sqrt(xdiff * xdiff + ydiff * ydiff) * 0.0001666F; // in nautical miles (1 / 6000)
	
	if(mpHsiValues[HSI_VAL_DISTANCE_TO_BEACON] > 999.0F) {
	    mpHsiValues[HSI_VAL_DISTANCE_TO_BEACON]	= 999.0F;
	}
	else if((mpHsiValues[HSI_VAL_DISTANCE_TO_BEACON] - (float)floor(mpHsiValues[HSI_VAL_DISTANCE_TO_BEACON])) > 0.5F) {
	    mpHsiValues[HSI_VAL_DISTANCE_TO_BEACON]	= (float)ceil(mpHsiValues[HSI_VAL_DISTANCE_TO_BEACON]);
	}
	
	mpHsiValues[HSI_VAL_CURRENT_HEADING]		= SimDriver.playerEntity->af->psi * RTD;
	//update our value
	if(SimDriver.playerEntity && SimDriver.playerEntity->INSState(AircraftClass::INS_HSI_OFF_IN))
		LastHSIHeading = mpHsiValues[HSI_VAL_CURRENT_HEADING];

	if(g_bRealisticAvionics && g_bINS && SimDriver.playerEntity &&
		!SimDriver.playerEntity->INSState(AircraftClass::INS_HSI_OFF_IN))
	{
		mpHsiValues[HSI_VAL_CURRENT_HEADING]		= LastHSIHeading;
	}
	
	if(mpHsiValues[HSI_VAL_CURRENT_HEADING] < 0.0F) {
	    mpHsiValues[HSI_VAL_CURRENT_HEADING] += 360.0F;
	}
    }
}
////////////////////////////////////////////////////////


//------------------------------------------------------
// CPHsi::CalcTCNCrsDev
//------------------------------------------------------
void CPHsi::CalcTCNCrsDev(float course) {
    
    mpHsiValues[HSI_VAL_CRS_DEVIATION]	= course - mpHsiValues[HSI_VAL_BEARING_TO_BEACON]; // in degrees
    if(mpHsiValues[HSI_VAL_CRS_DEVIATION] < -180.0F) {
	mpHsiValues[HSI_VAL_CRS_DEVIATION]		+= 360.0F;
    }
}

//------------------------------------------------------
// CPHsi::BeaconInRange - is this beacon receivable
//------------------------------------------------------
BOOL CPHsi::BeaconInRange(float rangeToBeacon, float nominalBeaconrange) 
{
    // we check every 5 seconds, to see if in range, else use the last result
    if (lastCheck < SimLibElapsedTime) {
	lastCheck = SimLibElapsedTime + 5 * CampaignSeconds;
    }
    else {
	return lastResult;
    }
    if (nominalBeaconrange ==0) nominalBeaconrange = 1;
    float detectionChance = 1.0f -  (rangeToBeacon - nominalBeaconrange)  * 2.0f / nominalBeaconrange;
    if (detectionChance > 1) lastResult = TRUE; // definite receive
    else if (detectionChance <= 0) lastResult = FALSE; // definite no receive
    else if (detectionChance > PRANDFloatPos()) { // somewhere in between
	lastResult = TRUE;
    }
    else lastResult = FALSE;
    return lastResult;
}

//------------------------------------------------------
// CPHsi::CalcILSCrsDev
//------------------------------------------------------
void CPHsi::CalcILSCrsDev(float dev) {
    
    mpHsiValues[HSI_VAL_CRS_DEVIATION]	= -dev * RTD;
    //if(mpHsiValues[HSI_VAL_CRS_DEVIATION] < -180.0F) {
    //mpHsiValues[HSI_VAL_CRS_DEVIATION]		+= 360.0F;
    //}
}

//------------------------------------------------------
// CPHsi::Exec()
//------------------------------------------------------

void CPHsi::Exec(void) {
    
    NavigationSystem::Instrument_Mode	mode;
    
    // Check for HSI Failure
    if (SimDriver.playerEntity &&
	SimDriver.playerEntity->mFaults &&
	SimDriver.playerEntity->mFaults->GetFault(FaultClass::tcn_fault))
    {
	return;
    }
    
    if (gNavigationSys)
    {
	mode = gNavigationSys->GetInstrumentMode();
    }
    else
    {
	return;
    }
    
    if(mode != mLastMode) {
	mpHsiFlags[HSI_FLAG_INIT] = FALSE;
    }
    
    mLastMode = mode;
    
    switch(mode) {
	
    case NavigationSystem::NAV:
	ExecNav();
	break;
	
    case NavigationSystem::ILS_NAV:
	ExecILSNav();
	break;
	
    case NavigationSystem::TACAN:
	ExecTacan();
	break;
	
    case NavigationSystem::ILS_TACAN:
	ExecILSTacan();
	break;
	
    default:
	break;
	
    }
}
////////////////////////////////////////////////////////



//------------------------------------------------------
// CPHsi::ExecILSTacan
//------------------------------------------------------

void CPHsi::ExecILSTacan(void) {
    
    float							ownshipX;
    float							ownshipY;
    float							tacanX;
    float							tacanY, tacanZ;
    float							gpDew;
    float							range;
    
    if(mpHsiFlags[HSI_FLAG_INIT] == FALSE) {
	
	mpHsiValues[HSI_VAL_DEV_LIMIT]		= 10.0F;
	mpHsiValues[HSI_VAL_HALF_DEV_LIMIT]	= mpHsiValues[HSI_VAL_DEV_LIMIT] * 0.5F;
	mpHsiFlags[HSI_FLAG_INIT]				= TRUE;
	mpHsiFlags[HSI_FLAG_ILS_WARN]			= FALSE;
	mpHsiFlags[HSI_FLAG_TO_TRUE]			= FALSE;
    }
    
    if(SimDriver.playerEntity) {
	
	ownshipX = SimDriver.playerEntity->XPos();
	ownshipY = SimDriver.playerEntity->YPos();
	
	mpHsiFlags[HSI_FLAG_CRS_WARN]	= !gNavigationSys->GetTCNPosition(&tacanX, &tacanY, &tacanZ);
	gNavigationSys->GetTCNAttribute(NavigationSystem::RANGE, &range);
	
	mpHsiFlags[HSI_FLAG_ILS_WARN]	= !gNavigationSys->GetILSAttribute(NavigationSystem::GP_DEV, &gpDew);
	ExecBeaconProximity(ownshipX, ownshipY, tacanX, tacanY);
	if (BeaconInRange(mpHsiValues[HSI_VAL_DISTANCE_TO_BEACON], range)) {
	    CalcILSCrsDev(gpDew);
	}
	else {
	    mpHsiFlags[HSI_FLAG_CRS_WARN] = TRUE;
	    mpHsiFlags[HSI_FLAG_ILS_WARN] = TRUE;
	    mpHsiValues[HSI_VAL_DISTANCE_TO_BEACON] = 0;
	    // mpHsiValues[HSI_VAL_CRS_DEVIATION] = 0; // last value?
	}
	if (mpHsiValues[HSI_VAL_CRS_DEVIATION] < 270 && mpHsiValues[HSI_VAL_CRS_DEVIATION] > 90)
	    mpHsiFlags[HSI_FLAG_TO_TRUE] = 2;
	else
	    mpHsiFlags[HSI_FLAG_TO_TRUE] = TRUE;
    }
}

///////////////////////////////////////////////////////////


//------------------------------------------------------
// CPHsiView::CPHsiView()
//------------------------------------------------------

CPHsiView::CPHsiView(ObjectInitStr *pobjectInitStr, HsiInitStr *phsiInitStr) : CPObject(pobjectInitStr) {

	int	radiusSquared;
	int	arraySize, halfArraySize;
	int	i;
	float	x, y;
	int	halfHeight;
	int	halfWidth;

	mWarnFlag.top					= phsiInitStr->warnFlag.top * mScale;
	mWarnFlag.left					= phsiInitStr->warnFlag.left * mScale;
	mWarnFlag.bottom				= phsiInitStr->warnFlag.bottom * mScale;
	mWarnFlag.right				= phsiInitStr->warnFlag.right * mScale;

	mpHsi								= phsiInitStr->pHsi;

	mCompassTransparencyType	= phsiInitStr->compassTransparencyType;

	mCompassSrc						= phsiInitStr->compassSrc;

	mCompassDest					= phsiInitStr->compassDest;
	mCompassDest.top				*= mScale;
	mCompassDest.left				*= mScale;
	mCompassDest.bottom			*= mScale;
	mCompassDest.right			*= mScale;

	mDevSrc							= phsiInitStr->devSrc;

	mDevDest							= phsiInitStr->devDest;
	mDevDest.top					*= mScale;
	mDevDest.left					*= mScale;
	mDevDest.bottom				*= mScale;
	mDevDest.right					*= mScale;

	mCompassWidth					= mCompassSrc.right - mCompassSrc.left;
	mCompassHeight					= mCompassSrc.bottom - mCompassSrc.top;

//	mCompassWidth					= mCompassDest.right - mCompassDest.left;
//	mCompassHeight					= mCompassDest.bottom - mCompassDest.top;

	mCompassXCenter				= mCompassDest.left + mCompassWidth / 2;
	mCompassYCenter				= mCompassDest.top + mCompassHeight / 2;

	// Setup the compass circle limits
	mRadius							= max(mCompassWidth, mCompassHeight);
	mRadius							= (mRadius + 1)/ 2;
	arraySize						= mRadius * 4;

	#ifdef USE_SH_POOLS
	mpCompassCircle = (int *)MemAllocPtr(gCockMemPool,sizeof(int)*arraySize,FALSE);
	#else
 	mpCompassCircle				= new int[arraySize];
	#endif

	radiusSquared					= mRadius * mRadius;

	halfArraySize					= arraySize / 2;
	for(i = 0; i < halfArraySize; i++) {

		y								= (float)fabs((float)i - mRadius);
		x								= (float)sqrt(radiusSquared - y * y );
		mpCompassCircle[i * 2 + 1]	= mRadius + (int)x; //right
		mpCompassCircle[i * 2 + 0]	= mRadius - (int)x; //left
	}

	halfHeight						= DisplayOptions.DispHeight / 2;
	halfWidth						= DisplayOptions.DispWidth / 2;
	mTop								=	(float) (halfHeight - mDevDest.top) / halfHeight;
	mLeft								=	(float) (mDevDest.left - halfWidth) / halfWidth;
	mBottom							=	(float) (halfHeight - mDevDest.bottom ) / halfHeight;
	mRight							=	(float) (mDevDest.right - halfWidth) / halfWidth;

	for (i = 0; i < HSI_COLOR_TOTAL; i++) {
	    mColor[0][i] = phsiInitStr->colors[i];
	    mColor[1][i] = CalculateNVGColor(mColor[0][i]);
	}
}
////////////////////////////////////////////////////////



//------------------------------------------------------
// CPHsiView::~CPHsiView()
//------------------------------------------------------

CPHsiView::~CPHsiView(void) {

	delete mpCompassCircle;
}
////////////////////////////////////////////////////////



//------------------------------------------------------
// CPHsiView::DisplayBlit
//------------------------------------------------------

void CPHsiView::DisplayBlit() {	

	mDirtyFlag = TRUE;	
	if(!mDirtyFlag) {
		return;
	}

	float	angle;
	float	currentHeading = mpHsi->GetValue(CPHsi::HSI_VAL_CURRENT_HEADING);

	angle = currentHeading + 90.0F;
	if(angle > 360.0F) {
		angle -= 360.0F;
	}

 	// Make the rotating blt call
	mpOTWImage->ComposeRoundRot(mpTemplate, &mCompassSrc, &mCompassDest, ConvertNavtoRad(angle), mpCompassCircle);

	mDirtyFlag = FALSE;
}
////////////////////////////////////////////////////////



//==================================================//
//	CPHsiView::Exec
//==================================================//

void CPHsiView::Exec(SimBaseClass*) { 

	OTWDriver.pCockpitManager->mpHsi->Exec();
}
////////////////////////////////////////////////////////



//------------------------------------------------------
// CPHsiView::Display()
//------------------------------------------------------

void CPHsiView::DisplayDraw() {	
static BOOL monoYes;
static init = 0;

if(!init) {
	monoYes = 0;
	init = 1;
}
	mDirtyFlag = TRUE;
	if(!mDirtyFlag) {
		return;
	}

	float	desiredCourse		= mpHsi->GetValue(CPHsi::HSI_VAL_DESIRED_CRS);
	float	desiredHeading		= mpHsi->GetValue(CPHsi::HSI_VAL_DESIRED_HEADING);
	float	courseDeviation	= mpHsi->GetValue(CPHsi::HSI_VAL_CRS_DEVIATION);
	float	bearingToBeacon	= mpHsi->GetValue(CPHsi::HSI_VAL_BEARING_TO_BEACON);
	BOOL	crsWarnFlag			= mpHsi->GetFlag(CPHsi::HSI_FLAG_CRS_WARN);


	if(monoYes) {
		MonoPrint("bearingToBeacon = %f\n", bearingToBeacon);
	}


	OTWDriver.renderer->SetViewport(mLeft, mTop, mRight, mBottom);

	DrawCourse(desiredCourse, courseDeviation);
	DrawStationBearing(bearingToBeacon);
	DrawHeadingMarker(desiredHeading);

	DrawAircraftSymbol();

	if(crsWarnFlag) {
		DrawCourseWarning();
	}

	mDirtyFlag = FALSE;
}
////////////////////////////////////////////////////////



//------------------------------------------------------
// CPHsiView::DrawToFrom()
//------------------------------------------------------

void CPHsiView::DrawToFrom(void) {
    
    static const float	toArrow[3][2] = {
	{ 0.055F,  0.12F - 0.25f},
	{ 0.157F,  0.00F - 0.25f},
	{ 0.055F, -0.12F - 0.25f},
    };
    BOOL	crsToTrueFlag		= mpHsi->GetFlag(CPHsi::HSI_FLAG_TO_TRUE);

	//MI
	if(g_bRealisticAvionics)
	{
		if(gNavigationSys)
		{
			if(gNavigationSys->GetInstrumentMode() == NavigationSystem::NAV)
				crsToTrueFlag = FALSE;
		}
	}
    
    if(crsToTrueFlag == TRUE) {					// Draw To-From Arrows
	OTWDriver.renderer->SetColor(mColor[TheTimeOfDay.GetNVGmode() != 0][HSI_COLOR_ARROWS]);
	OTWDriver.renderer->Tri(toArrow[0][0], toArrow[0][1], toArrow[1][0], toArrow[1][1],
	    toArrow[2][0], toArrow[2][1]);
	// draw ghost arrow
	OTWDriver.renderer->SetColor(mColor[TheTimeOfDay.GetNVGmode() != 0][HSI_COLOR_ARROWGHOST]);
	OTWDriver.renderer->Line(-toArrow[0][0], toArrow[0][1], -toArrow[1][0], toArrow[1][1]);
	OTWDriver.renderer->Line(-toArrow[1][0], toArrow[1][1], -toArrow[2][0], toArrow[2][1]);
	OTWDriver.renderer->Line(-toArrow[2][0], toArrow[2][1], -toArrow[0][0], toArrow[0][1]);
	
    }
    else if (crsToTrueFlag == 2) { // from
	OTWDriver.renderer->SetColor(mColor[TheTimeOfDay.GetNVGmode() != 0][HSI_COLOR_ARROWS]);
	OTWDriver.renderer->Tri(-toArrow[0][0], toArrow[0][1], -toArrow[1][0], toArrow[1][1],
	    -toArrow[2][0], toArrow[2][1]);
	// draw ghost arrow
	OTWDriver.renderer->SetColor(mColor[TheTimeOfDay.GetNVGmode() != 0][HSI_COLOR_ARROWGHOST]);
	OTWDriver.renderer->Line(toArrow[0][0], toArrow[0][1], toArrow[1][0], toArrow[1][1]);
	OTWDriver.renderer->Line(toArrow[1][0], toArrow[1][1], toArrow[2][0], toArrow[2][1]);
	OTWDriver.renderer->Line(toArrow[2][0], toArrow[2][1], toArrow[0][0], toArrow[0][1]);
    }
    else {
	OTWDriver.renderer->SetColor(mColor[TheTimeOfDay.GetNVGmode() != 0][HSI_COLOR_ARROWGHOST]);
	// draw ghost arrow
	OTWDriver.renderer->Line(-toArrow[0][0], toArrow[0][1], -toArrow[1][0], toArrow[1][1]);
	OTWDriver.renderer->Line(-toArrow[1][0], toArrow[1][1], -toArrow[2][0], toArrow[2][1]);
	OTWDriver.renderer->Line(-toArrow[2][0], toArrow[2][1], -toArrow[0][0], toArrow[0][1]);
	// draw ghost arrow
	OTWDriver.renderer->Line(toArrow[0][0], toArrow[0][1], toArrow[1][0], toArrow[1][1]);
	OTWDriver.renderer->Line(toArrow[1][0], toArrow[1][1], toArrow[2][0], toArrow[2][1]);
	OTWDriver.renderer->Line(toArrow[2][0], toArrow[2][1], toArrow[0][0], toArrow[0][1]);
    }
}
////////////////////////////////////////////////////////



//------------------------------------------------------
// CPHsiView::DrawCourseWarning()
//------------------------------------------------------

void CPHsiView::DrawCourseWarning(void) {

    OTWDriver.renderer->SetViewport(-1.0F, 1.0F, 1.0F, -1.0F);
    OTWDriver.renderer->SetColor(mColor[TheTimeOfDay.GetNVGmode() != 0][HSI_COLOR_COURSEWARN]);
    OTWDriver.renderer->Render2DTri((float)mWarnFlag.left, (float)mWarnFlag.top,
	(float)mWarnFlag.right, (float)mWarnFlag.top,
	(float)mWarnFlag.right, (float)mWarnFlag.bottom);
    OTWDriver.renderer->Render2DTri((float)mWarnFlag.left, (float)mWarnFlag.top,
	(float)mWarnFlag.left, (float)mWarnFlag.bottom,
	(float)mWarnFlag.right, (float)mWarnFlag.bottom);
    
}
////////////////////////////////////////////////////////



//------------------------------------------------------
// CPHsiView::DrawHeadingMarker();
//------------------------------------------------------

void CPHsiView::DrawHeadingMarker(float desiredHeading) {

	static const float	headingMarker[2][2] = {
	    {0.56F, 0.04F},
	    {0.61F, 0.04F},
	};
	float	currentHeading = mpHsi->GetValue(CPHsi::HSI_VAL_CURRENT_HEADING);

	MoveToCompassCenter();


	OTWDriver.renderer->AdjustRotationAboutOrigin(-ConvertNavtoRad(360.0F - currentHeading + desiredHeading));
	OTWDriver.renderer->SetColor(mColor[TheTimeOfDay.GetNVGmode() != 0][HSI_COLOR_HEADINGMARK]);
	
	// draw tail
	OTWDriver.renderer->Tri(headingMarker[0][0], headingMarker[0][1], headingMarker[1][0], headingMarker[1][1],
		headingMarker[1][0], -headingMarker[1][1]);
	OTWDriver.renderer->Tri(headingMarker[0][0], headingMarker[0][1], headingMarker[1][0], -headingMarker[1][1],
		headingMarker[0][0], -headingMarker[0][1]);

}
////////////////////////////////////////////////////////



//------------------------------------------------------
// CPHsiView::DrawStationBearing()
//------------------------------------------------------

void CPHsiView::DrawStationBearing(float bearing) { // in nav units

	static const float	bearingArrow[2][2] = {
	    {0.69F, 0.05F},
	    {0.80F, 0.0F},
	};
	static const float	bearingTail[2][2] = {
	    {-0.80F, 0.02F},
	    {-0.69F, 0.02F},
	};
	float	currentHeading = mpHsi->GetValue(CPHsi::HSI_VAL_CURRENT_HEADING);


	MoveToCompassCenter();

	OTWDriver.renderer->AdjustRotationAboutOrigin(-ConvertNavtoRad(360.0F - currentHeading + bearing));
	OTWDriver.renderer->SetColor(mColor[TheTimeOfDay.GetNVGmode() != 0][HSI_COLOR_STATIONBEARING]);

	// draw arrow
	OTWDriver.renderer->Tri(bearingArrow[0][0], bearingArrow[0][1], bearingArrow[1][0], bearingArrow[1][1],
		bearingArrow[0][0], -bearingArrow[0][1]);

	// draw tail
	//MI this is not here in real
	if(!g_bRealisticAvionics)
	{
		OTWDriver.renderer->Tri(bearingTail[0][0], bearingTail[0][1], bearingTail[1][0], bearingTail[1][1],
			bearingTail[1][0], -bearingTail[1][1]);
		OTWDriver.renderer->Tri(bearingTail[0][0], bearingTail[0][1], bearingTail[1][0], -bearingTail[1][1],
			bearingTail[0][0], -bearingTail[0][1]);
	}
}
////////////////////////////////////////////////////////



//------------------------------------------------------
// CPHsiView::DrawCourse()
//------------------------------------------------------

void CPHsiView::DrawCourse(float desiredCourse, float deviaiton) 
{

	static const float	courseArrow[4][2] = {
	    {0.610F, 0.000F},
	    {0.50F, 0.050F},
	    {0.50F, 0.020F},
	    {0.25F, 0.020F},
	};
	static const float	courseTail[2][2] = {
	    {-0.40F, 0.020F},
	    {-0.60F, 0.020F},
	};
	static const float	courseDevScale[3] = {0.0000F, 0.24F, 0.04F };
	static const float	courseDevBar[2][2] = {
	    {-0.39F,0.020F},
	    { 0.24F,0.020F},
	};
	static const float	courseDevWarn[2] = { -0.04F, 0.08F};

	float				devBarCenter[2]; 
	float				startPoint;
	float				r;
	float				theta;
	int				i;
	float				currentHeading			= mpHsi->GetValue(CPHsi::HSI_VAL_CURRENT_HEADING);
	float				deviationLimit			= mpHsi->GetValue(CPHsi::HSI_VAL_DEV_LIMIT);
	float				halfDeviationLimit	= mpHsi->GetValue(CPHsi::HSI_VAL_HALF_DEV_LIMIT);
	BOOL				ilsWarnFlag			= mpHsi->GetFlag(CPHsi::HSI_FLAG_ILS_WARN);
	mlTrig         trig;
	
	MoveToCompassCenter();
	desiredCourse	= 360.0F - currentHeading + desiredCourse;

	if(desiredCourse > 360.0F) {
		desiredCourse -= 360.0F;
	}

	desiredCourse	= ConvertNavtoRad(desiredCourse);


	// Draw Arrow
	OTWDriver.renderer->AdjustRotationAboutOrigin(-desiredCourse);

//	BOOL	toFromFlag			= mpHsi->GetFlag(CPHsi::HSI_FLAG_TO_TRUE);

	DrawToFrom();

	OTWDriver.renderer->SetColor(mColor[TheTimeOfDay.GetNVGmode() != 0][HSI_COLOR_COURSE]);

	OTWDriver.renderer->Tri(courseArrow[1][0], courseArrow[1][1], courseArrow[1][0], -courseArrow[1][1], 
		courseArrow[0][0], courseArrow[0][1]);
	OTWDriver.renderer->Tri(courseArrow[3][0], courseArrow[3][1], courseArrow[2][0], -courseArrow[2][1],
		courseArrow[2][0], courseArrow[2][1]);
	OTWDriver.renderer->Tri(courseArrow[3][0], courseArrow[3][1], courseArrow[3][0], -courseArrow[3][1],
		courseArrow[2][0], -courseArrow[2][1]);
	
	// Draw Tail
	OTWDriver.renderer->Tri(courseTail[0][0], courseTail[0][1], courseTail[1][0], courseTail[1][1],
		courseTail[1][0], -courseTail[1][1]);
	OTWDriver.renderer->Tri(courseTail[0][0], courseTail[0][1], courseTail[1][0], -courseTail[1][1],
		courseTail[0][0], -courseTail[0][1]);

	OTWDriver.renderer->SetColor(mColor[TheTimeOfDay.GetNVGmode() != 0][HSI_COLOR_CIRCLES]);

	// Draw Scale start drawing the course 
	// deviation scale from bottom circle, 
	// draw four equally spaced circles
	startPoint = -2.0F * courseDevScale[1];

	for(i = 0; i < 2; i++) {
		OTWDriver.renderer->Circle(courseDevScale[0], -startPoint, courseDevScale[2]);
		OTWDriver.renderer->Circle(courseDevScale[0], startPoint, courseDevScale[2]);
		startPoint += courseDevScale[1];
	}

	// JPO - normalise to account for flying away.
	if (deviaiton > 90) deviaiton = 180 - deviaiton;
	if (deviaiton < -90) deviaiton = - (180 + deviaiton);
	// If the deviaion is > 10% or < -10%, the bar will be pinned
	if(deviaiton > deviationLimit){
		startPoint		= 2.0F * courseDevScale[1];
	}
	else if(deviaiton < -deviationLimit){
		startPoint		= -2.0F * courseDevScale[1];
	}
	else{
		startPoint		= deviaiton / halfDeviationLimit * courseDevScale[1];
	}

   mlSinCos (&trig, HALFPI + desiredCourse);
	devBarCenter[0]	= startPoint * trig.cos;
	devBarCenter[1]	= startPoint * trig.sin;

	MoveToCompassCenter();

	OTWDriver.renderer->AdjustOriginInViewport(devBarCenter[0], devBarCenter[1]);
	OTWDriver.renderer->AdjustRotationAboutOrigin(-desiredCourse);

	// Draw the course dev bar
	OTWDriver.renderer->SetColor(mColor[TheTimeOfDay.GetNVGmode() != 0][HSI_COLOR_DEVBAR]);
	OTWDriver.renderer->Tri(courseDevBar[0][0], courseDevBar[0][1], courseDevBar[1][0], courseDevBar[1][1], 
		courseDevBar[1][0], -courseDevBar[1][1]);
	OTWDriver.renderer->Tri(courseDevBar[0][0], courseDevBar[0][1], courseDevBar[1][0], -courseDevBar[1][1], 
		courseDevBar[0][0], -courseDevBar[0][1]);


	if(ilsWarnFlag) {// && (gNavigationSys->GetInstrumentMode() == NavigationSystem::ILS_TACAN || gNavigationSys->GetInstrumentMode() == NavigationSystem::ILS_NAV)) {
	    
	    r = (float) sqrt(courseDevScale[1] * courseDevScale[1] + 0.2 * 0.2);
	    theta = (float)atan2(courseDevScale[1], 0.2F);
	    MoveToCompassCenter();
	    mlSinCos (&trig, theta + desiredCourse);
	    OTWDriver.renderer->AdjustOriginInViewport(r * trig.cos, r * trig.sin);
	    OTWDriver.renderer->AdjustRotationAboutOrigin(-desiredCourse);
	    
	    OTWDriver.renderer->SetColor(mColor[TheTimeOfDay.GetNVGmode() != 0][HSI_COLOR_ILSDEVWARN]);
	    OTWDriver.renderer->Tri(courseDevWarn[0], courseDevWarn[1], -courseDevWarn[0], courseDevWarn[1],
		-courseDevWarn[0], -courseDevWarn[1]);
	    OTWDriver.renderer->Tri(courseDevWarn[0], courseDevWarn[1], -courseDevWarn[0], -courseDevWarn[1],
		courseDevWarn[0], -courseDevWarn[1]);
	}	
}
////////////////////////////////////////////////////////



//------------------------------------------------------
// CPHsiView::MoveToCompassCenter()
//------------------------------------------------------

void CPHsiView::MoveToCompassCenter(void) {
	OTWDriver.renderer->CenterOriginInViewport();
	OTWDriver.renderer->ZeroRotationAboutOrigin();
	OTWDriver.renderer->AdjustOriginInViewport(COMPASS_X_CENTER, COMPASS_Y_CENTER);
}
////////////////////////////////////////////////////////



//------------------------------------------------------
// CPHsiView::DrawAircraftSymbol()
//------------------------------------------------------

void CPHsiView::DrawAircraftSymbol(void) {

	static const float	aircraftSymbol[10][2] = {
	    {0.017F, 0.133F},
	    {0.133F, 0.017F},
	    {0.067F, -0.133F},
	    {0.067F, -0.158F},
	    {0.017F, -0.2F},
	};


	MoveToCompassCenter();

	OTWDriver.renderer->SetColor(mColor[TheTimeOfDay.GetNVGmode() != 0][HSI_COLOR_AIRCRAFT]);

	// draw the aircraft symbol
	OTWDriver.renderer->Tri(-aircraftSymbol[0][0], aircraftSymbol[0][1], aircraftSymbol[0][0], aircraftSymbol[0][1],
		aircraftSymbol[4][0], aircraftSymbol[4][1]);
	OTWDriver.renderer->Tri(-aircraftSymbol[0][0], aircraftSymbol[0][1], aircraftSymbol[4][0], aircraftSymbol[4][1],
		-aircraftSymbol[4][0], aircraftSymbol[4][1]);
	OTWDriver.renderer->Tri(-aircraftSymbol[1][0], aircraftSymbol[1][1], aircraftSymbol[1][0], aircraftSymbol[1][1],
		aircraftSymbol[1][0], -aircraftSymbol[1][1]);
	OTWDriver.renderer->Tri(-aircraftSymbol[1][0], aircraftSymbol[1][1], aircraftSymbol[1][0], -aircraftSymbol[1][1],
		-aircraftSymbol[1][0], -aircraftSymbol[1][1]);
	OTWDriver.renderer->Tri(-aircraftSymbol[2][0], aircraftSymbol[2][1], aircraftSymbol[2][0], aircraftSymbol[2][1],
		aircraftSymbol[3][0], aircraftSymbol[3][1]);
	OTWDriver.renderer->Tri(-aircraftSymbol[2][0], aircraftSymbol[2][1], aircraftSymbol[3][0], aircraftSymbol[3][1],
		-aircraftSymbol[3][0], aircraftSymbol[3][1]);
}
/////////////////////////////////////////////////////////////////////////////////////

