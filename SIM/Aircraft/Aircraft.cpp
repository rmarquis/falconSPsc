#include "stdhdr.h"
#include "Graphics\Include\drawbsp.h"
#include "Graphics\Include\rviewpnt.h"
#include "Graphics\Include\RenderOW.h"
#include "ClassTbl.h"
#include "Entity.h"
#include "simobj.h"
#include "PilotInputs.h"
#include "hud.h"
#include "fcc.h"
#include "digi.h"
#include "facbrain.h"
#include "tankbrn.h"
#include "radar.h"
#include "radarAGOnly.h"
#include "radarDigi.h"
#include "radar360.h"
#include "radarSuper.h"
#include "radarDoppler.h"
#include "irst.h"
#include "eyeball.h"
#include "sms.h"
#include "guns.h"
#include "airframe.h"
#include "initdata.h"
#include "object.h"
#include "aircrft.h"
#include "fsound.h"
#include "soundfx.h"
#include "otwdrive.h"
#include "simdrive.h"
#include "hardpnt.h"
#include "campwp.h"
#include "missile.h"
#include "misldisp.h"
#include "f4error.h"
#include "cpmanager.h" //VWF added 2/18/97
#include "campbase.h"
#include "fack.h"
#include "caution.h"
#include "unit.h"
#include "playerop.h"
#include "camp2sim.h"
#include "sfx.h"
#include "acdef.h"
#include "simeject.h"
#include "MsgInc\EjectMsg.h" // PJW 12/3/97
#include "MsgInc\DamageMsg.h" 
#include "acmi\src\include\acmirec.h"
#include "fakerand.h"
#include "navsystem.h"
#include "dogfight.h"
#include "falcsess.h"
#include "Graphics\Include\terrtex.h"
#include "laserpod.h"
#include "ui\include\logbook.h"
#include "MsgInc\TankerMsg.h"
#include "MsgInc\RadioChatterMsg.h"
#include "hardPnt.h"
#include "ptdata.h"
#include "atcbrain.h"
#include "ffeedbk.h"
#include "easyHts.h"
#include "VehRwr.h"
#include <mbstring.h>
#include "codelib\tools\lists\lists.h"
#include "acmi\src\include\acmitape.h"
#include "dofsnswitches.h"
#include "team.h"
#include "Graphics\Include\drawgrnd.h"
#include "Graphics\Include\matrix.h"
#include "Graphics\Include\tod.h"
#include "objectiv.h"
#include "lantirn.h"
#include "flight.h" /* MN added */
#include "Cmpclass.h" /* MN added */
#include "MissEval.h" /* MN added */

//MI for Radar Altimeter
extern bool g_bRealisticAvionics;
//static float Lasttime = SimLibElapsedTime * MSEC_TO_SEC; // 2002-01-29 MILOOK Not used but in case you decide to use it, shouldn't it be SEC_TO_MSEC?
bool OneSec = FALSE;	//Did one second pass?

extern bool g_bDarkHudFix;

//MI
#include "PlayerRWR.h"
#include "commands.h"
extern bool g_bINS;

void ResetVoices(void);

ACMISwitchRecord acmiSwitch;
ACMIDOFRecord DOFRec;

extern int testFlag;
extern int RequestSARMission (FlightClass* flight);
int gPlayerExitMenuShown = FALSE;

#ifdef USE_SH_POOLS
MEM_POOL	AircraftClass::pool;
#endif

#define FOLLOW_RATE		10.0F

extern float g_fCarrierStartTolerance; // JB carrier

extern float SimLibLastMajorFrameTime;

extern int g_nShowDebugLabels; // MN
extern bool g_bAIGloc; // MN
extern float g_fAIRefuelSpeed; // MN

void CalcTransformMatrix (SimBaseClass* theObject);

#ifdef CHECK_PROC_TIMES
static ulong gPart1 = 0;
static ulong gPart2 = 0;
static ulong gPart3 = 0;
static ulong gPart4 = 0;
static ulong gPart5 = 0;
static ulong gPart6 = 0;
static ulong gPart7 = 0;
static ulong gPart8 = 0;
static ulong gWhole = 0;
extern int gameCompressionRatio;
#endif

AircraftClass::AircraftClass (int flag, VU_BYTE** stream) : SimVehicleClass (stream)
{
	InitData(flag);
}

AircraftClass::AircraftClass (int flag, FILE* filePtr) : SimVehicleClass (filePtr)
{
	InitData(flag);
}

AircraftClass::AircraftClass (int flag, int type) : SimVehicleClass (type)
{
	InitData(flag);
}

void AircraftClass::InitData (int flag)
{
	isDigital = flag;
	autopilotType = CombatAP;
	lastapType = ThreeAxisAP;
	theBrain = NULL;
	Guns = NULL;
	dropProgrammedStep  = 0;
	dropProgrammedTimer = 0;
	
	/*----------------*/
	/* Init targeting */
	/*----------------*/
	targetPtr = NULL;
    acFlags = 0;	
	fireGun = FALSE;
	fireMissile = FALSE;
	lastPickle = FALSE;
	curWaypoint = 0;
	glocFactor = 0.0F;
	gLoadSeconds = 0.0F;
	playerSmokeOn = FALSE;
	
	af = NULL;
	waypoint = NULL;
	for (int stn = 0; stn < TRAIL_MAX; stn++)
	    smokeTrail[stn] = NULL;
	for (stn = 0; stn < MAXENGINES; stn++) {
	    conTrails[stn] = NULL;
	    engineTrails[stn] = NULL;
	}
	lwingvortex = rwingvortex = NULL;
	dropChaffCmd = dropFlareCmd = NULL;
	ejectTriggered = FALSE;
	ejectCountdown = 1.0;
	doEjectCountdown = FALSE;

	//MI
	EmerJettTriggered = FALSE;
	JettCountown = 1.0;
	doJettCountdown = FALSE;
	
	bingoFuel = 1500;
	
	MPOCmd = FALSE;
	
	mFaults = NULL;
	lastStatus = 0;
	UnSetFlag (PILOT_EJECTED);
	
	// timers for targeting
	nextGeomCalc = SimLibElapsedTime;
	nextTargetUpdate = SimLibElapsedTime;
	
	mCautionCheckTime = SimLibElapsedTime;
	
	if ( isDigital )
	{
		geomCalcRate = FloatToInt32(0.5F * SEC_TO_MSEC);
		targetUpdateRate = FloatToInt32((1.0f + 3.0f * (float)rand()/RAND_MAX) * SEC_TO_MSEC);
	}
	else
	{
		geomCalcRate = 0;
		targetUpdateRate = 0;
	}
	
	// If we're flying dogfight, we mark this entity as nontransferable
	if (SimDriver.RunningDogfight())
		share_.flags_.breakdown_.transfer_ = 0;

	pLandLitePool		= NULL;
	mInhibitLitePool	= TRUE;

	//used for safe deletion of sensor array when making a player vehicle
	tempSensorArray = NULL;
	tempNumSensors = 0;
	powerFlags = 0; // JPO
	RALTStatus = ROFF; // JPO initialise
	RALTCoolTime = 5.0F;

	//MI EWS
	ManualECM = FALSE;
	EWSProgNum = 0;
	APFlag = 0;

	//MI Seat Arm
	SeatArmed = FALSE;

	// JPO electrics
	mainPower = MainPowerOff;
	currentPower = PowerNone;
	elecLights = ElecNone;
	interiorLight = LT_OFF;
	instrumentLight = LT_OFF;
	spotLight = LT_OFF;

	//MI Caution stuff
	NeedsToPlayCaution = FALSE;
	NeedsToPlayWarning = FALSE;
	WhenToPlayWarning = NULL;
	WhenToPlayCaution = NULL;

	attachedEntity = NULL; // JB carrier

// 2001-10-21 ADDED BY S.G. INIT THESE SO AI REACTS TO THE FIRST MISSILE LAUNCH
	incomingMissile[0] = NULL;
	incomingMissile[1] = NULL;
	incomingMissileEvadeTimer = 10000000;
	incomingMissileRange = 500 * NM_TO_FT;

	//MI VMS stuff
	playBetty = TRUE;
	//MI RF Switch
	RFState = 0;
	//MI overSpeed and G stuff
	for(int i = 0; i < 3; i++)
	{
		switch(i)
		{
		case 0:
			//Level one, 60 - 90kts tolerance
			OverSpeedToleranceTanks[i] = 60 + rand() % 30;
			OverSpeedToleranceBombs[i] = 60 + rand() % 30;

			//0.5-1G's tolerance
			OverGToleranceTanks[i] = 5 + rand() % 5;

			//1.8 - 2.2 G tolerance, will be divided by 10
			OverGToleranceBombs[i] = 18 + rand() % 4;

			//assign them
			SpeedToleranceTanks = OverSpeedToleranceTanks[i];
			SpeedToleranceBombs = OverSpeedToleranceBombs[i];

			GToleranceTanks = float(OverGToleranceTanks[i]) / 10.0;
			GToleranceBombs = float(OverGToleranceBombs[i]) / 10.0;
			break;
		case 1:
			//level two,  90 - 120kts tolerance
			OverSpeedToleranceTanks[i] = 91 + rand() % 30;
			OverSpeedToleranceBombs[i] = 91 + rand() % 30;

			//1.1 - 1.6G's tolerance, will be divided by 10
			OverGToleranceTanks[i] = 11 + rand() % 5;

			//2.3 - 2.7 G tolerance
			OverGToleranceBombs[i] = 23 + rand() % 4;
			break;
		case 2:
			//level three, 120 - 150kts tolerance
			OverSpeedToleranceTanks[i] = 122 + rand() % 30;
			OverSpeedToleranceBombs[i] = 122 + rand() % 30;

			//1.7 - 2.2G's tolerance, will be divided by 10
			OverGToleranceTanks[i] = 17 + rand() % 5;

			//2.8 - 3.2 G tolerance
			OverGToleranceBombs[i] = 28 + rand() % 4;
			break;
		default:
			break;
		}
	}

	//MI autopilot
	//right switch to off (middle)
	ClearAPFlag(AircraftClass::AltHold);
	ClearAPFlag(AircraftClass::AttHold);
	//left switch to att hold (middle)
//me123	middle is not rollhold realy..besides sometimes you can't stear in mp after takeoff with this SetAPFlag(AircraftClass::RollHold);
	ClearAPFlag(AircraftClass::RollHold);
	ClearAPFlag(AircraftClass::HDGSel);
	ClearAPFlag(AircraftClass::StrgSel);

	//EWS init
	SetPGM(AircraftClass::EWSPGMSwitch::Off);

	//Exterior lights init
	ExteriorLights = 0;

	//MI emergency Jettison
	EmerJettTriggered = FALSE;
	JettCountown = 0;
	doJettCountdown = FALSE;

	//MI AVTR
	AVTRFlags = 0;
	AVTRCountown = 0;	//AVTR auto
	doAVTRCountdown = FALSE;
	AVTROn(AVTR_OFF);

	//MI INS
	INSFlags = 0;
	INSOn(INS_PowerOff);
	INSAlignmentTimer = NULL;
	INSAlignmentStart = vuxGameTime;
	INSAlign = FALSE;
	HasAligned = FALSE;
	INSStatus = 100;
	INSLatDrift = 0.0F;
	INSLongDrift = 0.0F;
	INSTimeDiff = 0.0F;
	INS60kts = FALSE;
	CheckUFC = TRUE;
	INSLatOffset = 0.0F;
	INSLongOffset = 0.0F;
	INSAltOffset = 0.0F;
	INSHDGOffset = 0.0F;
	INSDriftLatDirection = 0;
	INSDriftLongDirection = 0;
	INSDriftDirectionTimer = 0.0F;
	BUPADIEnergy = 0.0F;
	GSValid = TRUE;
	LOCValid = TRUE;
	
	//MI MAL and Indicator lights test button and some other stuff
	CanopyDamaged = FALSE;
	TestLights = FALSE;
	LEFLocked = FALSE;
	LEFFlags = 0;
	LTLEFAOA = 0.0F;
	RTLEFAOA = 0.0F;
	leftLEFAngle = 0.0F;
	rightLEFAngle = 0.0F;
	TrimAPDisc = FALSE;
	TEFExtend = FALSE;
	MissileVolume = 8;	//no volume
	ThreatVolume = 8;
	GunFire = FALSE;

	//TGP Cooling
	PodCooling = ((7 + rand()%9) * 60);	//7 - 15 Minutes to cool down.
	CockpitWingLight = FALSE;
	CockpitStrobeLight = FALSE;
	RALTStatus = ROFF;
	RALTCoolTime = 5.0F;
	FCC = NULL;
	AWACSsaidAbort = false;

	NightLight = FALSE;
	WideView = FALSE;
}

AircraftClass::~AircraftClass(void)
{
	AircraftClass::Cleanup();
}

void AircraftClass::Cleanup(void)
{
	delete (af);
	af = NULL;
	
	if(mFaults) {
		delete mFaults;
	}
	mFaults = NULL;
	
	if(SimDriver.playerEntity == this) {	//VWF
		gNavigationSys->DeleteMissionTacans();
	}
	
	if (Sms)
		delete Sms;
	Sms = NULL;
	if (FCC)
		delete FCC;
	FCC = NULL;
}


void AircraftClass::Init(SimInitDataClass* initData)
{
	int i;
	WayPointClass* atWaypoint;
	float wp1X, wp1Y, wp1Z;
	float wp2X, wp2Y, wp2Z;
	Falcon4EntityClassType* classPtr = (Falcon4EntityClassType*)EntityType();
	int afIdx;
	long dt;
	
	SimVehicleClass::Init (initData);
	
	if (initData)
	{
		if (GetSType() == STYPE_AIR_FIGHTER_BOMBER &&
			GetSPType() == SPTYPE_F16C)
		{
			acFlags |= isF16|isComplex;
			// Turn on the nozzle
			SetSwitch(COMP_EXH_NOZZLE, 1);
			if ( gACMIRec.IsRecording() )
			{
				acmiSwitch.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
				acmiSwitch.data.type = Type();
				acmiSwitch.data.uniqueID = Id();
				acmiSwitch.data.switchNum = COMP_EXH_NOZZLE;
				acmiSwitch.data.switchVal = TRUE;
				acmiSwitch.data.prevSwitchVal = TRUE;
				gACMIRec.SwitchRecord( &acmiSwitch );
			}
		}

		// Airframe Stuff
		af = new AirframeClass (this);
		af->SetFlag(AirframeClass::IsDigital);
		
		if (mFaults)
			delete mFaults;
		mFaults = new FackClass;
		if (isDigital)
		{
			if (SimDriver.RunningInstantAction())
				af->SetFlag(AirframeClass::NoFuelBurn);
		}
		else
		{
			// Unlimited fuel?
			if (PlayerOptions.UnlimitedFuel())
				af->SetFlag(AirframeClass::NoFuelBurn);
			
			// Flight model type
			if (PlayerOptions.GetFlightModelType() == FMSimplified)
				af->SetFlag(AirframeClass::Simplified);
		}
		
		ShiAssert (initData->z > -250000.0F && initData->z <= 10000.0F);
		
		af->initialX    = initData->x;
		af->initialY    = initData->y;
		af->initialZ    = initData->z;
		af->initialFuel = (float)initData->fuel;
		
		//KCK Kludge
		af->x = af->initialX;
		af->y = af->initialY;
		af->z = af->initialZ;
		
		// END Kludge
		waypoint        = initData->waypointList;
		numWaypoints    = initData->numWaypoints;
		curWaypoint     = waypoint;
		atWaypoint      = waypoint;
		
		for (i=0; i<numWaypoints; i++)
		{
			// Clear tactic for use as flags
			curWaypoint->GetLocation (&wp1X, &wp1Y, &wp1Z);
			if (wp1Z > (OTWDriver.GetGroundLevel(wp1X, wp1Y) - 500.0F))
				curWaypoint->SetLocation (wp1X, wp1Y, OTWDriver.GetGroundLevel(wp1X, wp1Y) + wp1Z);
			
			curWaypoint = curWaypoint->GetNextWP();
		}
		curWaypoint     = waypoint;
		VU_TIME st = waypoint->GetWPArrivalTime();
		if (SimLibElapsedTime < st)
		    st = SimLibElapsedTime;
		mFaults->SetStartTime(st); // set base time

		//commented this out because it is legally possible for this to happen i.e. C-130's
		// in which case the fix is worse than leaving it alone. DSP (w/Kevin's agreement)
		/*
		// Make sure the current waypoint is takeoff if we have a takeoff point
		// If we have a takeoff point we MUST be at the zeroth waypoint
		F4Assert (initData->ptIndex == 0 || initData->currentWaypoint == 0);
		// LRKLUDGE
		// Fix it if it's wrong
		if (initData->ptIndex != 0 && initData->currentWaypoint != 0)
			initData->currentWaypoint = 0;
		*/
		
		for (i=0; i<initData->currentWaypoint; i++)
		{
			atWaypoint = curWaypoint;
			curWaypoint = curWaypoint->GetNextWP();
		}
		
		// Calculate current heading
		if (curWaypoint)
		{
			// Are we waiting for takeoff ?
			if (curWaypoint->GetWPAction() == WP_TAKEOFF)
			{
				OnGroundInit(initData);
			}
			else
			{
			    mFaults->AddTakeOff(waypoint->GetWPArrivalTime());

				if (curWaypoint == atWaypoint)
					curWaypoint = curWaypoint->GetNextWP();
				
				atWaypoint->GetLocation (&wp1X, &wp1Y, &wp1Z);
				
				if (curWaypoint == NULL)
				{
					wp1X = initData->x;
					wp1Y = initData->y;
					curWaypoint = atWaypoint;
				}
				
				if (SimDriver.RunningInstantAction() && !isDigital)
				{
					af->initialPsi = 0.0F;
					af->initialMach = 0.7F;
				}
				else
				{
					curWaypoint->GetLocation (&wp2X, &wp2Y, &wp2Z);
					af->initialPsi = (float)atan2 (wp2Y - wp1Y, wp2X - wp1X);
					dt = curWaypoint->GetWPArrivalTime() - atWaypoint->GetWPDepartureTime();
					
					if (dt > 10 * SEC_TO_MSEC)
					{
						af->initialMach = (float)sqrt((wp2X - wp1X)*(wp2X - wp1X) + (wp2Y - wp1Y)*(wp2Y - wp1Y)) /
							(dt / SEC_TO_MSEC);
						af->initialMach = max (af->initialMach, 200.0F);
					}
					else
					{
						af->initialMach = 0.7F;
					}
				}
				
			}
		}
		else
		{
			af->initialPsi = 0.0F;
			af->initialMach = 0.7F;
		}
		
		// KCK Hack: To make EOM work ok...
		// REMOVE BEFORE FLIGHT
		if (af->initialMach < 0.01F)
			af->initialMach = 0.01F;					// Minimal speed to catch problems -
		if (af->initialMach > 2.0F && af->initialMach < 10.0F)
			af->initialMach = 2.0F;					// max mach speed
		//if (af->initialMach > 1000.0F)
		//af->initialMach = 1000.F;					// max fps
		
		
		afIdx = SimACDefTable[classPtr->vehicleDataIndex].airframeIdx;
		
		af->InitData(afIdx);
		// Weapons Stuff
		Sms = new SMSClass (this, initData->weapon, initData->weapons);
		FCC = new FireControlComputer (this, Sms->NumHardpoints());
		FCC->Sms = Sms;
		
		if (Sms->NumHardpoints() > 0)
		{
			Guns = Sms->hardPoint[0]->GetGun();
		}
		


		// edg: I moved this check down here to make ABSOLUTELY sure
		// that if we're in the air (ie haven't gone thru OnGroundInit() as
		// a result of being at a takeoff point), that we're above the terrain.
		// also the original check was inadequate.
		if ( !OnGround() && af->initialZ > OTWDriver.GetGroundLevel(af->x, af->y) - 500.0f )
		{
			af->initialZ = OTWDriver.GetGroundLevel( af->x, af->y ) - 500.0F;
			af->z = af->initialZ;
		}
		// Get the controls
		af->Init(afIdx);
		if (af->auxaeroData->hasSwingWing) {
		    acFlags |= hasSwing;
		}
		if (af->auxaeroData->isComplex) {
		    acFlags |= isComplex;
		    MakeComplex();
		}

		// Turn on the canopy.
		SetSwitch(COMP_CANOPY, TRUE);
		if ( gACMIRec.IsRecording() )
		{
			acmiSwitch.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
			acmiSwitch.data.type = Type();
			acmiSwitch.data.uniqueID = Id();
			acmiSwitch.data.switchNum = COMP_CANOPY;
			acmiSwitch.data.switchVal = TRUE;
			acmiSwitch.data.prevSwitchVal = TRUE;
			gACMIRec.SwitchRecord( &acmiSwitch );
		}

		// Add Laser designator and harm pod for F16's
		if (IsComplex())
		{
			SetSwitch(COMP_EXH_NOZZLE, 1);
			if ( gACMIRec.IsRecording() )
			{
				acmiSwitch.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
				acmiSwitch.data.type = Type();
				acmiSwitch.data.uniqueID = Id();
				acmiSwitch.data.switchNum = COMP_EXH_NOZZLE;
				acmiSwitch.data.switchVal = TRUE;
				acmiSwitch.data.prevSwitchVal = TRUE;
				gACMIRec.SwitchRecord( &acmiSwitch );
			}
			if (Sms->HasHarm())
			{
				//Flip the switch to show the HARM pod
				SetSwitch (COMP_HTS_POD, 1);
				//switchData[12] = 1;
				//switchChange[12] = TRUE;

				if ( gACMIRec.IsRecording() )
				{
					acmiSwitch.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
					acmiSwitch.data.type = Type();
					acmiSwitch.data.uniqueID = Id();
					acmiSwitch.data.switchNum = COMP_HTS_POD;
					acmiSwitch.data.switchVal = TRUE;
					acmiSwitch.data.prevSwitchVal = TRUE;
					gACMIRec.SwitchRecord( &acmiSwitch );
				}
			}
			
			if (Sms->HasLGB())
			{
				// Flip the switch to show the laser pod
				SetSwitch (COMP_TIRN_POD, 1);
				//switchData[11] = 1;
				//switchChange[11] = TRUE;
				if ( gACMIRec.IsRecording() )
				{
					acmiSwitch.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
					acmiSwitch.data.type = Type();
					acmiSwitch.data.uniqueID = Id();
					acmiSwitch.data.switchNum = COMP_TIRN_POD;
					acmiSwitch.data.switchVal = TRUE;
					acmiSwitch.data.prevSwitchVal = TRUE;
					gACMIRec.SwitchRecord( &acmiSwitch );
				}
			}
		}
		if (Guns) {
		    float x, y, z;
		    af->GetGunLocation(&x, &y, &z);
		    Guns->SetPosition (x, y, z, 0.0F, 0.0F);
		}
		// Add Sensor
		// Quick, count the sensors
		{
			int sensorType, sensorIdx, hasHarm, hasLGB, needsRadar;
			
			numSensors = 0;
			needsRadar = TRUE;
			for (i = 0; i<5; i++)
			{
				if (SimACDefTable[classPtr->vehicleDataIndex].sensorType[i] > 0)
				{
					numSensors++;
					//KLudge until new class table for bombers
					if (SimACDefTable[classPtr->vehicleDataIndex].sensorType[i] == SensorClass::Radar)
						needsRadar = FALSE;
				}
			}
			
			// Should we add a HARM pod?
			hasHarm = Sms->HasHarm();
			
			// Should we add a Targeting pod?
			hasLGB = Sms->HasLGB();
			numSensors += hasHarm + hasLGB;
			if (needsRadar)
				numSensors ++;
			
			sensorArray = new SensorClass*[numSensors];
			for (i = 0; i<numSensors -(hasHarm + hasLGB + needsRadar); i++)
			{
				sensorType = SimACDefTable[classPtr->vehicleDataIndex].sensorType[i];
				sensorIdx	= SimACDefTable[classPtr->vehicleDataIndex].sensorIdx[i];
				
				switch (sensorType)
				{
				case SensorClass::Radar:
					if (RadarDataTable[GetRadarType()].LookDownPenalty < 0.0)
						sensorArray[i] = new RadarAGOnlyClass (GetRadarType(), this);
					else
						sensorArray[i] = new RadarDigiClass (GetRadarType(), this);
					break;
					
				case SensorClass::RWR:
					sensorArray[i] = new VehRwrClass (sensorIdx, this);
					break;
					
				case SensorClass::IRST:
					sensorArray[i] = new IrstClass (sensorIdx, this);
					break;
					
				case SensorClass::Visual:
					sensorArray[i] = new EyeballClass (sensorIdx, this);
					break;
				default:
					ShiWarning( "Unhandled sensor type during Init" );
					break;
				}
			}
			
			//Kludge until class table fixed
			if (needsRadar)
			{
				sensorArray[i] = new RadarAGOnlyClass (1, this);
            i ++;
			}
			
			if (hasHarm)
			{
				sensorArray[i] = new EasyHarmTargetingPod (5, this);
				i ++;
			}
			
			if (hasLGB)
			{
				sensorArray[i] = new LaserPodClass (5, this);
			}
		}

		// If I only had a brain
		// we need the waypoint data initialized before we build the brain
		switch (((UnitClass*)GetCampaignObject())->GetUnitMission())
		{
		case AMIS_FAC:
			theBrain = new FACBrain (this, af);
			break;
			
		case AMIS_TANKER:
			theBrain = new TankerBrain (this, af);
			SetFlag( I_AM_A_TANKER );
			break;
			
		default:
			theBrain = new DigitalBrain (this, af);
			break;
		}

	  DBrain()->SetTaxiPoint(initData->ptIndex);
      DBrain()->SetSkill(initData->skill);
	  //DBrain()->SetSkill(4);
      // Modify search time based on skill level
      // Rookies (level 0) has 9x ave time, top of the line has 1x
      geomCalcRate = geomCalcRate * ((4 - DBrain()->SkillLevel()) * 2 + 1);
      targetUpdateRate = targetUpdateRate * ((4 - DBrain()->SkillLevel()) * 2 + 1);
		
		// DO NOT MOVE THIS CODE OR I WILL BREAK YOUR DAMN FINGERS, VWF 3/11/97
		if (isDigital)
		{
			af->SetSimpleMode( SIMPLE_MODE_AF );
// ADDED BY S.G. PILOTS WILL HAVE EITHER THEIR LIGHTS ON OR OFF DEPENDING IF THEY HAVE REACHED WAYPOINT 2 OR NOT
// REMOVED BECAUSE ONLY THE F16 HAS LIGHT SWITCHES :-(
/*			if (curWaypoint->GetWPAction() < WP_POSTASSEMBLE) {
			   SetSwitch (COMP_TAIL_STROBE, TRUE);
			   SetSwitch (COMP_NAV_LIGHTS, TRUE);
			}
			else {
			   SetSwitch (COMP_TAIL_STROBE, FALSE);
			   SetSwitch (COMP_NAV_LIGHTS, FALSE);
			} */
// END OF ADDED SECTION
		}
		// END OF WARNING
				
		/*--------------------*/
		/* Update shared data */
		/*--------------------*/
		if (!OnGround())
			SetPosition (af->x, af->y, af->z);
		else
		{
			Objective obj = (Objective)vuDatabase->Find(DBrain()->Airbase());
			int pt = initData->ptIndex;

			if (obj) 
				pt = FindBestSpawnPoint(obj, initData);
			DBrain()->SetTaxiPoint(pt);
			SetPosition (af->x, af->y, OTWDriver.GetGroundLevel(af->x, af->y) - 5.0F);
#if 0
			while(obj && CheckPointGlobal(this,af->x,af->y))
			{
				if(GetNextPt(initData->ptIndex))
				{
					initData->ptIndex = GetNextPt(initData->ptIndex);
					TranslatePointData(obj, initData->ptIndex, &af->x, &af->y);
				}
				else
					af->x += 50.0F;
			}
			DBrain()->SetTaxiPoint(initData->ptIndex);
			SetPosition (af->x, af->y, OTWDriver.GetGroundLevel(af->x, af->y) - 5.0F);
#endif
		}
		SetDelta (0.0F, 0.0F, 0.0F);
		SetYPR (af->psi, af->theta, af->phi);
		SetYPRDelta (0.0F, 0.0F, 0.0F);
		SetVt(af->vt);
		SetKias(af->vcas);
// 2000-11-17 MODIFIED BY S.G. SO OUR SetPowerOutput WITH ENGINE TEMP IS USED INSTEAD OF THE SimBase CLASS ONE
		SetPowerOutput(af->rpm);
//me123 changed back		SetPowerOutput(af->rpm, af->oldp01[0]);
// END OF MODIFIED SECTION
		
		CalcTransformMatrix (this);
		
		theInputs   = new PilotInputs;
		
		
		
		if (Sms->NumHardpoints() && isDigital)
		{
			Sms->SetUnlimitedAmmo (FALSE);
			FCC->SetMasterMode(FireControlComputer::Missile);
			FCC->SetSubMode(FireControlComputer::Aim9);
		}

		if (!g_bRealisticAvionics || !OnGround()) // JPO set up the systems
		PreFlight();
	}

   

	// Check unlimited guns in dogfight
	if (SimDriver.RunningDogfight() && SimDogfight.IsSetFlag (DF_UNLIMITED_GUNS))
	{
		Sms->SetUnlimitedGuns(TRUE);
	}

	UnSetFlag (PILOT_EJECTED);

	// timers for targeting
	nextGeomCalc = SimLibElapsedTime;
	nextTargetUpdate = SimLibElapsedTime;
   SOIManager (SimVehicleClass::SOI_RADAR);

	CleanupLitePool();

	//MI Reset our soundflag
	SpeedSoundsWFuel = 0;
	SpeedSoundsNFuel = 0;
	GSoundsWFuel = 0;
	GSoundsNFuel = 0;
	StationsFailed = 0;

	//MI INS
	INSFlags = 0;
	INSOn(INS_PowerOff);
	INSAlignmentTimer = NULL;
	INSAlignmentStart = vuxGameTime;
	INSAlign = FALSE;
	HasAligned = FALSE;
	INSStatus = 100;
	INSLatDrift = 0.0F;
	INSLongDrift = 0.0F;
	INSTimeDiff = 0.0F;
	INS60kts = FALSE;
	CheckUFC = TRUE;
	INSLatOffset = 0.0F;
	INSLongOffset = 0.0F;
	INSAltOffset = 0.0F;
	INSHDGOffset = 0.0F;
	INSDriftLatDirection = 0;
	INSDriftLongDirection = 0;
	INSDriftDirectionTimer = 0.0F;
	BUPADIEnergy = 0.0F;
	GSValid = TRUE;
	LOCValid = TRUE;

	//MI MAL and Indicator lights test button and some other stuff
	CanopyDamaged = FALSE;
	TestLights = FALSE;
	LEFLocked = FALSE;
	LEFFlags = 0;
	LTLEFAOA = 0.0F;
	RTLEFAOA = 0.0F;
	leftLEFAngle = 0.0F;
	rightLEFAngle = 0.0F;
	TrimAPDisc = FALSE;
	TEFExtend = FALSE;
	MissileVolume = 8;	//no volume
	ThreatVolume = 8;
	GunFire = FALSE;
	//Targeting pod cooling
	PodCooling = ((7 + rand()%9) * 60);	//7 - 15 Minutes to cool down.
	CockpitWingLight = FALSE;
	CockpitStrobeLight = FALSE;

	NightLight = FALSE;
	WideView = FALSE;
}

int AircraftClass::Exec(void)
{
	//float groundZ = 0.0f;
	//Falcon4EntityClassType* classPtr;
	ACMIAircraftPositionRecord airPos;
	Flight flight = NULL;
	int flightIdx;

    if (g_nShowDebugLabels & 0x2000)
    {
		if ( drawPointer )
		{
			char tmpchr[1024];
			sprintf(tmpchr,"%f",af->Fuel() + af->ExternalFuel());
			((DrawableBSP*)drawPointer)->SetLabel (tmpchr, ((DrawableBSP*)drawPointer)->LabelColor());
		}
    }	

	if ( IsDead() )
		return TRUE;
#ifdef CHECK_PROC_TIMES
	gPart1 = 0;
	gPart2 = 0;
	gPart3 = 0;
	gPart4 = 0;
	gPart5 = 0;
	gPart6 = 0;
	gPart7 = 0;
	gPart8 = 0;
	gWhole = GetTickCount();
#endif
	// only do ejection on local entity
	if ( IsLocal() )
	{
		// When doing a QuickPreflight, it can happen that the HUD is not yet setup and
		// it stays dark when entering the simulation.
		if(g_bDarkHudFix && TheHud && this == SimDriver.playerEntity && 
			DBrain() && !(DBrain()->moreFlags & DigitalBrain::HUDSetup) &&
			PlayerOptions.GetStartFlag() != PlayerOptionsClass::START_RAMP)
		{
			TheHud->SymWheelPos = 1.0F;
			TheHud->SetLightLevel();
			DBrain()->moreFlags |= DigitalBrain::HUDSetup; // set the flag so we don't go in here again
		}
		
		
		if(doEjectCountdown && !ejectTriggered && ejectCountdown < 0.0 && fabs(glocFactor) < 0.95F)
		{
			MonoPrint
				(
				"Initiating ejection sequence at player's request.\n"
				);
			
			ejectTriggered = TRUE;
			//Make sure the autopilot is off;
			SetAutopilot(APOff);
			//no radio if you eject :)
			ResetVoices();
		}
		
		ejectCountdown -= SimLibMajorFrameTime;
		
		// ejection: use a small pctStrength damage window to trigger;
		if 
		( 
			(
				(
					pctStrength < -0.4f &&
					pctStrength > -0.6f &&
					(dyingType == SimVehicleClass::DIE_INTERMITTENT_SMOKE ||
					dyingType == SimVehicleClass::DIE_SHORT_FIREBALL ||
					dyingType == 5 ||
					dyingType == 6 ||
					dyingType == SimVehicleClass::DIE_SMOKE) &&
					isDigital
				) ||
				(
					ejectTriggered
				)
			) &&
			(
				!IsSetFlag( PILOT_EJECTED )
			)
		)
		{
			Eject();
			F4SoundFXSetPos( af->auxaeroData->sndEject, TRUE, XPos(), YPos(), ZPos(), 1.0f );
		}

		//MI Emergency jettison
		if(g_bRealisticAvionics)
		{
			if(doJettCountdown && !EmerJettTriggered && JettCountown < 0.0)
			{
				MonoPrint("Initiating Jettison sequence at player's request.\n");
				EmerJettTriggered = TRUE;
			}
			// MN fix: reset EmerJettTriggered
			if(EmerJettTriggered && Sms)
			{
				Sms->EmergencyJettison();
				EmerJettTriggered = false;
			}
			
			JettCountown -= SimLibMajorFrameTime;

			//AVTR
			if(AVTRState(AircraftClass::AVTRStateFlags::AVTR_AUTO) && SimDriver.AVTROn())
			{
				if(AVTRCountown > 0.0F)
					AVTRCountown -= SimLibMajorFrameTime;
				else if (AVTRCountown <= 0.0F)	//turn it off
				{
					ACMIToggleRecording(0, KEY_DOWN, NULL);
					SimDriver.SetAVTR(FALSE);
				}
			}

			//MI INS
			if(g_bINS && !isDigital)
			{
				RunINS();
				//Backup ADI engergy
				if(MainPower() == MainPowerMain)
				{
					if(BUPADIEnergy < 540.0F)	//9 minutes
						BUPADIEnergy += (SimLibMajorFrameTime * 9 * 2);	//full in 1 min
				}
				else if(BUPADIEnergy > 0.0F)
					BUPADIEnergy -= SimLibMajorFrameTime;

				if(BUPADIEnergy > 0.0F) //goes away immediately
					INSOn(BUP_ADI_OFF_IN);
				else
					INSOff(BUP_ADI_OFF_IN);	//visibile

				//TargetingPod Cooling
				if(HasPower(AircraftClass::RightHptPower) && PodCooling > -1.0F)
					PodCooling -= SimLibMajorFrameTime;
				else	//warm it up if we don't have power
					PodCooling += SimLibMajorFrameTime;
			}
			//RALT stuff
			if(HasPower(RaltPower) && RALTCoolTime > -1.0F && RALTStatus != RaltStatus::ROFF) 
				RALTCoolTime -= SimLibMajorFrameTime;
			else
				RALTCoolTime += SimLibMajorFrameTime;
		}
	}
	else
	{
//		if (!ejectTriggered && IsSetFlag( PILOT_EJECTED ))
//		{
//			ejectTriggered = TRUE;
//			Eject();
//			F4SoundFXSetPos( SFX_EJECT, TRUE, XPos(), YPos(), ZPos(), 1.0f );
//		}

      // LRKLUDGE
      //if (ZPos() < OTWDriver.GetGroundLevel(XPos(), YPos()))
      //   UnSetFlag (ON_GROUND);
      //else
      //   SetFlag (ON_GROUND);
	}

#ifdef CHECK_PROC_TIMES
	gPart1 = GetTickCount();
#endif
	
	// Base Class Exec
	SimVehicleClass::Exec();

#ifdef CHECK_PROC_TIMES
	gPart1 = GetTickCount() - gPart1;
	gPart2 = GetTickCount();
#endif

	if(IsExploding())
	{
		if ( !IsSetFlag( SHOW_EXPLOSION ) )
		{
			RunExplosion();
			SetFlag(SHOW_EXPLOSION);

			if (this == SimDriver.playerEntity)
			{
				JoystickPlayEffect (JoyHitEffect, 0);
			}

			SetDead(TRUE);
		}
		return TRUE;
	}
	else if (!IsDead())
	{	
		// Sound effects....
		if ( pctStrength > 0.0f )
		{
			float dop;
			
			VehicleClassDataType *vc = GetVehicleClassData(Type() - VU_LAST_ENTITY_TYPE);
			ShiAssert(FALSE == F4IsBadReadPtr(vc, sizeof *vc));

			// total hack here -- need to better identify which planes are props
			//classPtr = (Falcon4EntityClassType*)EntityType();
			/*
			if (classPtr->vuClassData.classInfo_[VU_STYPE] == STYPE_AIR_TANKER ||
			classPtr->vuClassData.classInfo_[VU_STYPE] == STYPE_AIR_AWACS ||
			classPtr->vuClassData.classInfo_[VU_STYPE] == STYPE_AIR_ASW ||
			classPtr->vuClassData.classInfo_[VU_STYPE] == STYPE_AIR_RECON )
			*/
			// total hack for demo -- an2 is the only prop
			//if (classPtr->vuClassData.classInfo_[VU_STYPE] == STYPE_AIR_TRANSPORT &&
			//	classPtr->vuClassData.classInfo_[VU_SPTYPE] == SPTYPE_AN2)

			int inckpt = OTWDriver.DisplayInCockpit(); // JPO save this
			// JPO - if special case...
			// easter egg: need to check af NULL since we may be non-local
			if ( af && af->GetSimpleMode() == SIMPLE_MODE_HF )
			{
				F4SoundFXSetPos( SFX_ENGHELI, 0, XPos(), YPos(), ZPos(), 0.5f + af->rpm/3.0F );
			}
			else if (IsF16()) { // another special case
			    // should all be factored out, and the same done for each model
			    if ( !isDigital &&
				inckpt &&
				IsLocal())
			    {
				//if (PowerOutput() > 0.2F) // JB 000816
				if (PowerOutput() > 0.01F)
				{
				    F4SoundFXSetPos( vc->EngineSound, 0, XPos(), YPos(), ZPos(), 0.01f + PowerOutput() );
				    
				    if (PowerOutput() > 1.0F)
				    {
					F4SoundFXSetPos( af->auxaeroData->sndAbInt, 0, XPos(), YPos(), ZPos(), PowerOutput() - 0.25f );
				    }
				}
				
				// wind noise
				F4SoundFXSetPos( SFX_ENGINEA, 0, XPos(), YPos(), ZPos(), Kias()/450.0f );
			    }
			    else
			    {
				dop = OTWDriver.GetDoppler( XPos(), YPos(), ZPos(), XDelta(), YDelta(), ZDelta() );
				if ( PowerOutput() > 0.01f )
				    F4SoundFXSetPos( SFX_F16EXT, 0, XPos(), YPos(), ZPos(), dop * (0.01f + PowerOutput()) );

				if (PowerOutput() > 1.0F)
				{
				    F4SoundFXSetPos( af->auxaeroData->sndAbExt, 0, XPos(), YPos(), ZPos(), dop * (PowerOutput() - 0.25f) );
				}
			    }
			}
			else if (vc)
			{
			    if (inckpt) {
				if (PowerOutput() > 0.01F)
				    F4SoundFXSetPos( vc->EngineSound, 0, XPos(), YPos(), ZPos(), 0.01f + PowerOutput()  );
				// wind noise
				F4SoundFXSetPos( af->auxaeroData->sndWind, 0, XPos(), YPos(), ZPos(), Kias()/450.0f );
				if (PowerOutput() > 1.0F)
				{
				    F4SoundFXSetPos( inckpt ? af->auxaeroData->sndAbInt: af->auxaeroData->sndAbExt, 0, XPos(), YPos(), ZPos(), PowerOutput() - 0.25f);
				}
			    }
			    else {
				dop = OTWDriver.GetDoppler( XPos(), YPos(), ZPos(), XDelta(), YDelta(), ZDelta() );
				if ( PowerOutput() > 0.01f )
				    F4SoundFXSetPos( af->auxaeroData->sndExt, 0, XPos(), YPos(), ZPos(), dop * (0.2f + PowerOutput()) );
				if (PowerOutput() > 1.0F)
				{
				    F4SoundFXSetPos( inckpt ? af->auxaeroData->sndAbInt: af->auxaeroData->sndAbExt, 0, XPos(), YPos(), ZPos(), dop * (PowerOutput() - 0.25f) );
				}
			    }
			}
			else {
				dop = OTWDriver.GetDoppler( XPos(), YPos(), ZPos(), XDelta(), YDelta(), ZDelta() );
				F4SoundFXSetPos( SFX_F16EXT, 0, XPos(), YPos(), ZPos(), dop * (0.2f + PowerOutput()) );
				if (PowerOutput() > 1.0F)
				{
				    F4SoundFXSetPos( inckpt ? SFX_BURNERI : SFX_BURNERE, 0, XPos(), YPos(), ZPos(), dop * (PowerOutput() - 0.25f) );
				}
			}

		}
		else
		{
			// plane is going down: strength < 0
			F4SoundFXSetPos( SFX_FIRELOOP, 0, XPos(), YPos(), ZPos(), 1.0f + pctStrength * 0.2f );
			if ( af )
				af->SetSimpleMode( SIMPLE_MODE_OFF );
		}
	
	
		if (IsF16()) {
			//MI added check for player
			if(IsPlayer())
			{
				if(	pLandLitePool == NULL && af->gearPos == 1.0F && 
					TheTimeOfDay.GetLightLevel() < 0.65f && !(af->gear[1].flags & GearData::GearBroken) && af->LLON) 
				{
					Tpoint pos;
					pos.x = XPos();
					pos.y = YPos();
					pos.z = ZPos();

					pLandLitePool = new DrawableGroundVehicle(MapVisId(VIS_LITEPOOL), &pos, Yaw());
					mInhibitLitePool = TRUE;
					OTWDriver.InsertObject(pLandLitePool);
				}
				else if( pLandLitePool && (af->gearPos != 1.0F || TheTimeOfDay.GetLightLevel() >= 0.65f) || !af->LLON)
					CleanupLitePool();
			}
			else
			{
				if(	pLandLitePool == NULL && af->gearPos == 1.0F && 
					TheTimeOfDay.GetLightLevel() < 0.65f && !(af->gear[1].flags & GearData::GearBroken) ) 
				{
					Tpoint pos;
					pos.x = XPos();
					pos.y = YPos();
					pos.z = ZPos();

					pLandLitePool = new DrawableGroundVehicle(MapVisId(VIS_LITEPOOL), &pos, Yaw());
					mInhibitLitePool = TRUE;
					OTWDriver.InsertObject(pLandLitePool);
				}
				else if( pLandLitePool && (af->gearPos != 1.0F || TheTimeOfDay.GetLightLevel() >= 0.65f)) { 

					CleanupLitePool();
				}
			}

			if(pLandLitePool) {

				Trotation	rot;
				Tpoint		relPos;
				Tpoint		rotPos;
				float			scale;
				float			groundLevel;

				rot.M11		= dmx[0][0];
				rot.M21		= dmx[0][1];
				rot.M31		= dmx[0][2];
				
				rot.M12		= dmx[1][0];
				rot.M22		= dmx[1][1];
				rot.M32		= dmx[1][2];
				
				rot.M13		= dmx[2][0];
				rot.M23		= dmx[2][1];
				rot.M33		= dmx[2][2];
				
				// Set the position of the pool relative
				// to the aircraft with a normalized vector
				
				relPos.x	= 1.0F;
				relPos.y	= 0.0F;
				relPos.z	= 0.0524F; // tangent of 3 degrees (down)
				
				MatrixMult (&rot, &relPos, &rotPos);

				groundLevel = OTWDriver.GetGroundLevel (XPos(), YPos());
				scale			= -(ZPos() - groundLevel)/rotPos.z;


				if(rotPos.z <= 0.0F) {
					mInhibitLitePool = TRUE;
				}
				else {
					mInhibitLitePool = FALSE;

					rotPos.x	= XPos() + rotPos.x * scale;
					rotPos.y	= YPos() + rotPos.y * scale;
					rotPos.z	= ZPos() + rotPos.z * scale;

					pLandLitePool->Update (&rotPos, 0.0F);
				}

				pLandLitePool->SetInhibitFlag(mInhibitLitePool);
			}
		}

		// ACMI Output
		if (gACMIRec.IsRecording() && (SimLibFrameCount & 0x0000000f ) == 0)
		{
			airPos.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
			airPos.data.type = Type();
			airPos.data.uniqueID = ACMIIDTable->Add(Id(),(char *) ((DrawableBSP*)drawPointer)->Label(),TeamInfo[GetTeam()]->GetColor());//.num_;
			airPos.data.x = XPos();
			airPos.data.y = YPos();
			airPos.data.z = ZPos();
			airPos.data.roll = Roll();
			airPos.data.pitch = Pitch();
			airPos.data.yaw = Yaw();
			RadarClass *radar = (RadarClass*)FindSensor( this, SensorClass::Radar );
			if (  radar && radar->RemoteBuggedTarget)//me123 add record for online targets
			   airPos.RadarTarget = ACMIIDTable->Add(radar->RemoteBuggedTarget->Id(),NULL,0);//.num_;
			else if (  radar && radar->CurrentTarget() )
			   airPos.RadarTarget = ACMIIDTable->Add(radar->CurrentTarget()->BaseData()->Id(),NULL,0);//.num_;
			else
				airPos.RadarTarget = -1;
			gACMIRec.AircraftPositionRecord( &airPos );
		}
#ifdef CHECK_PROC_TIMES
	gPart2 = GetTickCount() - gPart2;
#endif
		
		if (!IsLocal())
		{
			ShowDamage();
			af->RemoteUpdate();
			return FALSE;
		}
		
		// Increment the SendStatus Timer
		requestCount ++;
		
		// edg note: this seems to have no ill effects as far as I can
		// tell.  However I'm commenting it out for the time being until
		// a better solution for targetlist updates is done.  Also, it should
		// be done on some sort of timer anyway....
		
		// edg note: For AI's I've moved the target list stuff into the digi
		// code.  I want to synchronize doing this and geom calcs with running
		// sensor fusion
		//if ( IsSetFlag( MOTION_OWNSHIP ) )
		if(autopilotType != CombatAP && HasPilot())
		{
#ifdef CHECK_PROC_TIMES
	gPart3 = GetTickCount();
#endif
			targetList = UpdateTargetList (targetList, this, SimDriver.combinedList);
#ifdef CHECK_PROC_TIMES
	gPart3 = GetTickCount() - gPart3;
	gPart4 = GetTickCount();
#endif
			// edg: all aircraft now get combined list
			CalcRelGeom(this, targetList, vmat, 1.0F / SimLibMajorFrameTime);
#ifdef CHECK_PROC_TIMES
	gPart4 = GetTickCount() - gPart4;
	gPart5 = GetTickCount();
#endif
			// Sensors
			RunSensors();
			// electrics
#ifdef CHECK_PROC_TIMES
	gPart5 = GetTickCount() - gPart5;
#endif
			// Have Missiles?
			if (Sms->HasWeaponClass(wcAimWpn))
			{
				SetFlag(HAS_MISSILES);
			}
			else
			{
				UnSetFlag(HAS_MISSILES);
			}
		}
#ifdef CHECK_PROC_TIMES
	gPart6 = GetTickCount();
#endif
	DoElectrics(); // JPO - well it has to go somewhere

	if (OTWDriver.IsActive() // JPO - practice safe viewpoints people :-)
		&& !OTWDriver.IsShutdown()) // JB - come on, practice safer viewpoints ;-)
	{
		// JB carrier start
		RViewPoint* viewp = OTWDriver.GetViewpoint();
		if (ZPos() > -g_fCarrierStartTolerance && af && af->vcas < .01 && viewp && viewp->GetGroundType( XPos(), YPos() ) == COVERAGE_WATER)
		{
			// We just started inside the carrier
			if (isDigital)
			{
				af->z = -500 + rand() % 200;
				af->vcas = 250 + rand() % 100;
				af->vt = 367 + rand() % 100;
				af->ClearFlag(AirframeClass::OverRunway);
				af->ClearFlag(AirframeClass::OnObject);
				af->ClearFlag(AirframeClass::Planted);
				af->platform->UnSetFlag( ON_GROUND );
				af->SetFlag (AirframeClass::InAir);
				PreFlight();
				af->ClearFlag(AirframeClass::EngineOff);
			}
			else
				targetList = UpdateTargetList (targetList, this, SimDriver.combinedList);
		}
		// JB carrier end
		else if (isDigital && DBrain() && !DBrain()->IsSetATC(DigitalBrain::DonePreflight))
		{//in multiplay this sometimes gets fucked up for the player flight
		    // JPO - changed > to < here --- otherwise everything start started up
			if (curWaypoint && curWaypoint->GetWPArrivalTime() - 2*CampaignMinutes < TheCampaign.CurrentTime)
			{//me123 this should never happan, btu it does sometimes in MP
			PreFlight();
			//MI if we don't tell our brain that we've preflighted, we keep on looping in that function.....
			if(isDigital && DBrain() && !DBrain()->IsSetATC(DigitalBrain::DonePreflight))
				DBrain()->SetATCFlag(DigitalBrain::DonePreflight);
			// Turn on the canopy.
			SetSwitch(COMP_CANOPY, TRUE);
			
			if ( gACMIRec.IsRecording() )
				{
					acmiSwitch.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
					acmiSwitch.data.type = Type();
					acmiSwitch.data.uniqueID = Id();
					acmiSwitch.data.switchNum = COMP_CANOPY;
					acmiSwitch.data.switchVal = TRUE;
					acmiSwitch.data.prevSwitchVal = TRUE;
					gACMIRec.SwitchRecord( &acmiSwitch );
				}
			}
		}
	}
		// after geometry has been calculated, we can check collisions
		// digitals don't check collisions.  they just avoid.
		// this shouldn't really matter much, but it can be easily turned on
		
		//if ( !isDigital && PlayerOptions.CollisionsOn() ) // JB carrier
		if (!isDigital) // JB carrier
			CheckObjectCollision();

#ifdef CHECK_PROC_TIMES
	gPart6 = GetTickCount() - gPart6;
	gPart7 = GetTickCount();
#endif
		// Get the controls
		GatherInputs();
#ifdef CHECK_PROC_TIMES
	gPart7 = GetTickCount() - gPart7;
	gPart8 = GetTickCount();
#endif
		if(tempSensorArray)
		{
			//when we need to change the sensors for the player we just set tempSensorArray equal
			//to sensorarray to save them for deletion at this time, and then 
			for (int i=0; i<tempNumSensors; i++)
			{
				delete tempSensorArray[i];
				tempSensorArray[i] = NULL;
			}
			delete [] tempSensorArray;
			tempSensorArray = NULL;
			tempNumSensors = 0;
		}

		// Predict GLOC
		// 2002-01-27 MN AI can't handle GLOC at all which causes them to lawndart at full afterburner
		// for now we remove GLOC for AI and only apply it on the player's aircraft
// 2002-03-08 make AI Gloc configurable for testing
		if (SimDriver.playerEntity == this || g_bAIGloc)
		{
			if(!PlayerOptions.BlackoutOn())
				glocFactor = 0.0F;
			else
				glocFactor = GlocPrediction();
		}
		
		// 2002-02-12 MN Check if our mission target has been occupied by our ground troops and let AWACs say something useful
// 2002-03-03 MN fix, only check strike mission flights
		flight = (Flight)GetCampaignObject();
		if (flight && (flight->GetUnitMission() > AMIS_SEADESCORT && flight->GetUnitMission() < AMIS_FAC) )
		{
			flightIdx = flight->GetComponentIndex(this);
			if (!AWACSsaidAbort && !flightIdx) // only for flight leaders of course
			{
				WayPoint w = flight->GetFirstUnitWP();
				while (w)
				{
					if (!(w->GetWPFlags() & WPF_TARGET))
					{
						w = w->GetNextWP();
						continue;
					}
					CampEntity target;
					target = w->GetWPTarget();
					if (target && (target->GetTeam() == GetTeam()))
					{
						AWACSsaidAbort = true;
						FalconRadioChatterMessage *radioMessage;
						radioMessage = CreateCallFromAwacs(flight, rcAWACSTARGETOCCUPIED); // let AWACS bring the message that we've occupied the target ;)
						FalconSendMessage(radioMessage, FALSE);
						TheCampaign.MissionEvaluator->Register3DAWACSabort(flight);
					}
					break;
				}
			}
		}
		
		// 2002-01-29 ADDED BY S.G. Run our targetSpot update code...
		if (SimDriver.playerEntity == this && DBrain()) {
			if (DBrain()->targetSpotWing) {
				// First See if the timer has elapsed or the target is died
				if (SimLibElapsedTime > DBrain()->targetSpotWingTimer || !DBrain()->targetSpotWingTarget || DBrain()->targetSpotWingTarget->IsDead()) { // 2002-03-07 MODIFIDED BY S.G. Added '|| !DBrain()->targetSpotWingTarget' but should NOT be required!
					// If so, kill the camera and clear out everything associated with this targetSpot
					for (int i = 0; i < FalconLocalSession->CameraCount(); i++) {
						if (FalconLocalSession->Camera(i) == DBrain()->targetSpotWing) {
							FalconLocalSession->RemoveCamera(DBrain()->targetSpotWing->Id());
							break;
						}
					}

					vuDatabase->Remove (DBrain()->targetSpotWing); // Takes care of deleting the allocated memory and the driver allocation as well.
					if (DBrain()->targetSpotWingTarget) // 2002-03-07 ADDED BY S.G. In case it's NULL. Shouldn't happen but happened
						VuDeReferenceEntity(DBrain()->targetSpotWingTarget);
					DBrain()->targetSpotWing = NULL;
					DBrain()->targetSpotWingTarget = NULL;
					DBrain()->targetSpotWingTimer = 0;
				}
				else {
					// Update the position and exec the driver
					DBrain()->targetSpotWing->SetPosition(DBrain()->targetSpotWingTarget->XPos(), DBrain()->targetSpotWingTarget->YPos(), DBrain()->targetSpotWingTarget->ZPos());
					DBrain()->targetSpotWing->EntityDriver()->Exec(vuxGameTime);
				}
			}

			if (DBrain()->targetSpotElement) {
				// First See if the timer has elapsed or the target is died
				if (SimLibElapsedTime > DBrain()->targetSpotElementTimer || !DBrain()->targetSpotElementTarget || DBrain()->targetSpotElementTarget->IsDead()) { // 2002-03-07 MODIFIDED BY S.G. Added '|| !DBrain()->targetSpotElementTarget' but should NOT be required!
					// If so, kill the camera and clear out everything associated with this targetSpot
					for (int i = 0; i < FalconLocalSession->CameraCount(); i++) {
						if (FalconLocalSession->Camera(i) == DBrain()->targetSpotElement) {
							FalconLocalSession->RemoveCamera(DBrain()->targetSpotElement->Id());
							break;
						}
					}

					vuDatabase->Remove (DBrain()->targetSpotElement); // Takes care of deleting the allocated memory and the driver allocation as well.
					if (DBrain()->targetSpotElementTarget) // 2002-03-07 ADDED BY S.G. In case it's NULL. Shouldn't happen but happened
						VuDeReferenceEntity(DBrain()->targetSpotElementTarget);
					DBrain()->targetSpotElement = NULL;
					DBrain()->targetSpotElementTarget = NULL;
					DBrain()->targetSpotElementTimer = 0;
				}
				else {
					// Update the position and exec the driver
					DBrain()->targetSpotElement->SetPosition(DBrain()->targetSpotElementTarget->XPos(), DBrain()->targetSpotElementTarget->YPos(), DBrain()->targetSpotElementTarget->ZPos());
					DBrain()->targetSpotElement->EntityDriver()->Exec(vuxGameTime);
				}
			}

			if (DBrain()->targetSpotFlight) {
				// First See if the timer has elapsed or the target is died
				if (SimLibElapsedTime > DBrain()->targetSpotFlightTimer || !DBrain()->targetSpotFlightTarget || DBrain()->targetSpotFlightTarget->IsDead()) { // 2002-03-07 MODIFIDED BY S.G. Added '|| !DBrain()->targetSpotFlightTarget' but should NOT be required!
					// If so, kill the camera and clear out everything associated with this targetSpot
					for (int i = 0; i < FalconLocalSession->CameraCount(); i++) {
						if (FalconLocalSession->Camera(i) == DBrain()->targetSpotFlight) {
							FalconLocalSession->RemoveCamera(DBrain()->targetSpotFlight->Id());
							break;
						}
					}

					vuDatabase->Remove (DBrain()->targetSpotFlight); // Takes care of deleting the allocated memory and the driver allocation as well.
					if (DBrain()->targetSpotFlightTarget) // 2002-03-07 ADDED BY S.G. In case it's NULL. Shouldn't happen but happened
						VuDeReferenceEntity(DBrain()->targetSpotFlightTarget);
					DBrain()->targetSpotFlight = NULL;
					DBrain()->targetSpotFlightTarget = NULL;
					DBrain()->targetSpotFlightTimer = 0;
				}
				else {
					// Update the position and exec the driver
					DBrain()->targetSpotFlight->SetPosition(DBrain()->targetSpotFlightTarget->XPos(), DBrain()->targetSpotFlightTarget->YPos(), DBrain()->targetSpotFlightTarget->ZPos());
					DBrain()->targetSpotFlight->EntityDriver()->Exec(vuxGameTime);
				}
			}
		}

		/*------------------------*/        
		/* Fly the airframe.      */
		/*------------------------*/
		
		// edg TODO: we're calculating ground level in too many places (the
		// airframe does this too).  There should be some place this is
		// done 1 time every frame to optimize.  Att the moment I'm just trying
		// to move LandingCheck out of here and into the airframe class
		//af->groundZ = OTWDriver.GetGroundLevel(af->x, af->y, &af->gndNormal);
		
		// bb pullup goes here?
		// factors from manual:
		//	greater than 50ft AGL
		//	descent rate > 960 fpm
		//	landing gear up
		if ( !isDigital &&
			ZPos() - af->groundZ < -50.0f &&
			af->gearPos <= 0.0f &&
			af->zdot > 960.0f/60.0f &&
			Pitch() < 0.0f
			)
		{
			float turnRadius,denom, gs, num, alt;
			float groundAlt;
			float clearanceBuffer;
			
			groundAlt = -af->groundZ;
			
			/*--------------------------------*/
			/* Find current state turn radius */
			/*--------------------------------*/
			gs = af->MaxGs() - 0.5F;
			gs = min (gs, 7.0F);
			num = gs*gs - 1.0F;
			
			if (num <= 0.0)
				denom = 0.1F;
			else
				denom = GRAVITY * (float)sqrt(num);
			
			// get turn radius length nased on our speed and gs
			turnRadius = Vt()*Vt() / denom;
			
			// turnRadius should be equivalent to the drop in altitude
			// we'd make assuming we were pointed straight down and trying
			// to reach level flight.  modify the radius based on our
			// current pitch.  Remember our pitch should be negative at this
			// point
			turnRadius *= -Pitch()/( 90.0f * DTR );
			
			// clearance buffer depends on vcas
			if ( af->vcas <= 325.0f )
			{
				clearanceBuffer = 150.0f;
			}
			else if ( af->vcas >= 375.0f )
			{
				clearanceBuffer = 50.0f;
			}
			else // between 325 and 375
			{
				clearanceBuffer = 150.0f - 2.0f * (af->vcas - 325.0f);
			}
			
			// get alt AGL (positive)
			alt = (-ZPos()) - groundAlt - clearanceBuffer;
			
			// num = alt / (2.0F*turnRadius);
			// if (num > 1.0F)
			
			// if our AGL is greater than our predicted turn radius plus some
			// cushion,
			// the clear the fault.  otherwise issue warning...
			if ( alt > turnRadius * 1.15f )
			{
				// no pullup needed
				mFaults->ClearFault(alt_low);
			}
			else
			{
				F4SoundFXSetDist( af->auxaeroData->sndBBPullup, FALSE, 0.0f, 1.0f );
				mFaults->SetFault(alt_low);
				
			}
		}
		else if(!isDigital)
		{
			mFaults->ClearFault(alt_low);
		}
		
		// check for collision with feature
		if ( af->vt > 0.0f && !isDigital && PlayerOptions.CollisionsOn() )
			GroundFeatureCheck( af->groundZ );
		else
			FeatureCollision( af->groundZ ); //need onFlatFeature to be correct for landings
		//onFlatFeature = TRUE;
		
		if (OnGround())
		{
			//MI
			if(IsPlayer() && g_bRealisticAvionics)
			{
				UnSetFlag(ECM_ON);
			}
			// Check for special IA end condition
			if (SimDriver.RunningInstantAction() && this == SimDriver.playerEntity)
			{
				if (af->vt < 5.0F && af->throtl < 0.1F)
				{
               if (!gPlayerExitMenuShown)
               {
                  gPlayerExitMenuShown = TRUE;
					   OTWDriver.EndFlight();
               }
				}
            else
            {
               gPlayerExitMenuShown = FALSE;
            }
			}
		}
		else
		{
			
			// fun stuff: if between 10 and 80 ft of ground, stirr up
			// dust or water

			if ( OTWDriver.renderer && OTWDriver.renderer->GetAlphaMode() )
			{
				if (ZPos() - af->groundZ >= -80.0f &&
					ZPos() - af->groundZ <= -10.0f )
				{
					//if we are over a runway, we don't want to kick up dust
					if(!af->IsSet(AirframeClass::OverRunway) && af->gearPos <= 0.7F)
					{				
						int groundType;
						// Check ground type 
						groundType = OTWDriver.GetGroundType( XPos(), YPos() );
						if(groundType != COVERAGE_ROAD)
						{
							Tpoint mvec;
							Tpoint pos;
		
							pos.x = XPos();
							pos.y = YPos();
							pos.z = af->groundZ - 3.0f;
								
							mvec.x = 20.0f * PRANDFloat();
							mvec.y = 20.0f * PRANDFloat();
							mvec.z = -30.0f;
							
							if ( !( groundType == COVERAGE_WATER || groundType == COVERAGE_RIVER ) )
							{
								OTWDriver.AddSfxRequest(
									new SfxClass( SFX_AIR_DUSTCLOUD,		// type
									SFX_MOVES | SFX_USES_GRAVITY | SFX_NO_DOWN_VECTOR,
									&pos,					// world pos
									&mvec,					// vel vector
									3.0,					// time to live
									8.0f ) );				// scale
							}
							else
							{
								OTWDriver.AddSfxRequest(
									new SfxClass( SFX_WATER_CLOUD,		// type
									SFX_MOVES | SFX_USES_GRAVITY | SFX_NO_DOWN_VECTOR,
									&pos,					// world pos
									&mvec,					// vel vector
									3.0,					// time to live
									10.0f ) );				// scale
							}
						}
					}
				}
			}
		}		
		
		if (SimDriver.MotionOn())
			af->Exec();		

		// JPO - current home of lantirn execution
		// JB 010325 Only do this for the player 
		//if (theLantirn && theLantirn->IsEnabled() && IsSetFlag(MOTION_OWNSHIP))
		if (theLantirn && theLantirn->IsEnabled() && !isDigital)
		    theLantirn->Exec(this);
		
		// Spped brake sound
		if ( af->dbrake > 0.1f && this == SimDriver.playerEntity && Kias() > 100.0F)
		{
			// F4SoundFXSetPos( SFX_BRAKWIND, 0, XPos(), YPos(), ZPos(), Kias()/450.0f );
			F4SoundFXSetDist( af->auxaeroData->sndSpdBrakeWind, 0, (1.0F - af->speedBrake) * 100.0F, Kias()/450.0f);
		}
		
		if (af->IsSet(AirframeClass::Refueling) && DBrain() && !DBrain()->IsTanker())
		{
			AircraftClass *tanker = NULL;
			tanker = (AircraftClass*)vuDatabase->Find(DBrain()->Tanker());
			float refuelRate = af->GetRefuelRate();
			float refuelHelp = (float)PlayerOptions.GetRefuelingMode();
			if (this != SimDriver.playerEntity && g_fAIRefuelSpeed)
				refuelHelp = g_fAIRefuelSpeed;
			if(!tanker || !tanker->IsAirplane() || !af->AddFuel(refuelRate * SimLibMajorFrameTime * refuelHelp * refuelHelp))
			{
				FalconTankerMessage *TankerMsg;

				if(tanker)
					TankerMsg	= new FalconTankerMessage (tanker->Id(), FalconLocalGame);
				else
					TankerMsg	= new FalconTankerMessage (FalconNullId, FalconLocalGame);

				TankerMsg->dataBlock.type	= FalconTankerMessage::DoneRefueling;
				TankerMsg->dataBlock.caller	= Id();
				TankerMsg->dataBlock.data1  = 0;
				FalconSendMessage(TankerMsg);
			}
		}
		/*--------------------*/
		/* Update shared data */
		/*--------------------*/
//		MonoPrint ("AF %f,%f,%f\n", af->x, af->y, af->z);

		SetPosition (af->x, af->y, af->z);
		SetDelta (af->xdot, af->ydot, af->zdot);

		SetYPR (af->psi, af->theta, af->phi);
		SetYPRDelta (af->r, af->q, af->p);
		SetVt (af->vt);
		SetKias(af->vcas);
// 2000-11-17 MODIFIED BY S.G. SO OUR SetPowerOutput WITH ENGINE TEMP IS USED INSTEAD OF THE SimBase CLASS ONE
		SetPowerOutput(af->rpm);
//me123 back		SetPowerOutput(af->rpm, af->oldp01[0]);
// END OF MODIFIED SECTION
		
		// Weapons
		if ( autopilotType != CombatAP )
		{
			SetTarget(FCC->Exec(targetPtr, targetList, theInputs));
		}
		else
		{
			// edg: emperical hack.  The FCC is exec'd in digi anyway
			// when bombing run.  Doing it here as well also screws up
			// digi's releasing bombs
         SimObjectType* tmpTarget = targetPtr;

         if (FCC->GetMasterMode() == FireControlComputer::AirGroundMissile && DBrain())
            tmpTarget = DBrain()->GetGroundTarget();
         else if (FCC->GetMasterMode() == FireControlComputer::AirGroundLaser)
            tmpTarget = NULL;

//   			if (FCC->GetMasterMode() != FireControlComputer::AirGroundBomb)
   				SetTarget(FCC->Exec(tmpTarget, targetList, theInputs));
		}
		DoWeapons();
		
		// Handle missiles launched but still on the rail
		Sms->Exec();
		
		if (TheHud && TheHud->Ownship() == this && Guns != NULL)
		{
			TheHud->SetEEGSData(af->x, af->y, af->z, af->gmma, af->sigma,
				af->theta, af->psi, af->vt);
		}
		
		if (IsDying ())
			SetDead(TRUE);
		
		// Do countermeasures
		DoCountermeasures();
		
		// KCK: Really, this is the only safe way to set campaign positions -
		UnitClass	*campObj = (UnitClass*) GetCampaignObject();
		if (campObj && campObj->IsLocal() && campObj->GetComponentLead() == this)
		{
			campObj->SimSetLocation(af->x, af->y, af->z);
			if (campObj->IsFlight())
				campObj->SimSetOrientation (af->psi, af->theta, af->phi);
		}
		
		//Check cautions
		CautionCheck();
   }

   // If ownship update control surfaces
   if (!IsExploding())
      MoveSurfaces();

   if ( !IsDead() )
      ShowDamage();

#ifdef CHECK_PROC_TIMES
	gPart8 = GetTickCount() - gPart8;
	gWhole = GetTickCount() - gWhole;

	if(gameCompressionRatio && IsPlayer())
	{
		MonoPrint("Whole:  %3d  SimVeh: %3d  UpTar: %3d CaReGe: %3d\n", gWhole, gPart1, gPart3, gPart4);
		MonoPrint("RunSen: %3d  ChColl: %3d  GaInp: %3d End:    %3d\n\n", gPart5, gPart6, gPart7, gPart8);
	}
#endif

	//MI no parking brake while in Air
	if(af && af->IsSet(AirframeClass::InAir) && af->PBON)
		af->PBON = FALSE;

   return TRUE;
}

void AircraftClass::RunSensors(void)
{
	SimObjectType* object;
	SensorClass *sensor = NULL;
	int i;

	// Now run all our sensors
	for (i=0; i<numSensors; i++)
	{		
		//if we are in the middle of resetting our sensors bail
		if( numSensors)
			sensor = sensorArray[i];
		else
			return;
		// Do player control processing
		if (!isDigital || autopilotType == CombatAP)
		{
			sensor->ExecModes(FCC->designateCmd, FCC->dropTrackCmd);
			sensor->UpdateState(FCC->cursorXCmd, FCC->cursorYCmd);
		}

// 2000-09-25 ADDED BY S.G. SO 'LockedTarget' IS AT LEAST FOLLOWING OR 'targetPtr'
		// Before we run our sensor, we 'push' our targetPtr as its lockedTarget if we are in CombatAP, its lockedTarget is NULL and it's not the targeting pod (from SetTarget code) and we HAVE a target
		// I had to do this because once the radar gimbals out, AI won't reacquire its lockedTarget once it can see it again...
		// If it cannot see it, it will be reset anyway...
		// TODO: Do we need to check if it's an AI? Will it affect the player's radar? Update: Nope
		if (autopilotType == CombatAP && sensor->CurrentTarget() == NULL && sensor->Type() != SensorClass::TargetingPod && targetPtr != NULL)
			sensor->SetDesiredTarget( targetPtr );
// END OF ADDED SECTION

		// Run the sensor model
		object = sensor->Exec(targetList);

		// Update the system target if necessary
		// NOTE:  This is only part of the story.  Many FCC modes also set the system
		// target directly.  Sad but true...
		if (sensor->Type() == SensorClass::Radar)
		{
			if (FCC->GetMasterMode() != FireControlComputer::AirGroundBomb && 
				FCC->GetMasterMode() != FireControlComputer::AirGroundMissile &&
				FCC->GetMasterMode() != FireControlComputer::AirGroundLaser &&
				FCC->GetMasterMode() != FireControlComputer::AirGroundHARM)
			{
				// Our system target becomes our radar target
				SetTarget(object);
			}
			else if (PlayerOptions.GetAvionicsType() == ATEasy &&
					 FCC->GetSubMode() == FireControlComputer::CCIP)
			{
				// Let easy mode players get a TD box even in CCIP
				SetTarget(object);
			}
		}
	}
}

void AircraftClass::ReceiveOrders (FalconEvent* theEvent)
{
	if (theBrain)
		theBrain->ReceiveOrders(theEvent);
}

void AircraftClass::RemovePilot(void)
{
	SetFlag (PILOT_EJECTED);
}

void AircraftClass::Eject(void)
{
	int					i;
	EjectedPilotClass	*epc;
	FalconEjectMessage	*ejectMessage;	// PJW... Yell at me for this :) 12/3/97
	FalconDeathMessage	*deathMessage;	// SCR/KCK 12/10/98
	FalconEntity		*lastToHit;
	
	if (isDigital)
	{
		//MonoPrint ("Aircraft %d DIGI PILOT EJECTED at %8ld\n", Id().num_, SimLibElapsedTime);
	}
	else
	{
		//MonoPrint ("Aircraft %d PLAYER PILOT EJECTED at %8ld\n", Id().num_, SimLibElapsedTime);
	}
	
	if (IsSetFalcFlag (FEC_REGENERATING))
	{
		// In Dogfight - eject is a suicide - regenerates
		// Send an eject message and a death message right away
		ShiAssert(GetCampaignObject() != NULL);
		
		ejectMessage=new FalconEjectMessage(Id(), FalconLocalGame);
		ShiAssert( ejectMessage );
		ejectMessage->dataBlock.ePlaneID		= Id();
//		ejectMessage->dataBlock.eEjectID		= FalconNullId;
		ejectMessage->dataBlock.eCampID			= ((CampBaseClass*)GetCampaignObject())->GetCampID();
		ejectMessage->dataBlock.eFlightID   = GetCampaignObject()->Id();
//		ejectMessage->dataBlock.eSide			= ((CampBaseClass*)GetCampaignObject())->GetOwner();
		ejectMessage->dataBlock.ePilotID		= pilotSlot;
//		ejectMessage->dataBlock.eIndex			= Type();
		ejectMessage->dataBlock.hadLastShooter	= (uchar)(LastShooter() != FalconNullId);
		
		FalconSendMessage(ejectMessage, TRUE);
		
		
		deathMessage = new FalconDeathMessage (Id(), FalconLocalGame);
		ShiAssert( deathMessage );
		
		deathMessage->dataBlock.dEntityID			= Id();
		deathMessage->dataBlock.dCampID				= ((CampBaseClass*)GetCampaignObject())->GetCampID();
		deathMessage->dataBlock.dSide				= ((CampBaseClass*)GetCampaignObject())->GetOwner();
		deathMessage->dataBlock.dPilotID			= pilotSlot;
		deathMessage->dataBlock.dIndex				= Type();
		deathMessage->dataBlock.fWeaponID			= Type();
		deathMessage->dataBlock.fWeaponUID			= FalconNullId;
		deathMessage->dataBlock.deathPctStrength	= pctStrength;
		
		lastToHit = (SimVehicleClass*)vuDatabase->Find( LastShooter() );
		if (lastToHit && !lastToHit->IsEject() )
		{
			deathMessage->dataBlock.damageType		= FalconDamageType::OtherDamage;
			if (lastToHit->IsSim())
			{
				deathMessage->dataBlock.fPilotID	= ((SimVehicleClass*)lastToHit)->pilotSlot;
				deathMessage->dataBlock.fCampID		= ((SimVehicleClass*)lastToHit)->GetCampaignObject()->GetCampID();
				deathMessage->dataBlock.fSide		= ((SimVehicleClass*)lastToHit)->GetCampaignObject()->GetOwner();
			}
			else
			{
				deathMessage->dataBlock.fPilotID	= 0;
				deathMessage->dataBlock.fCampID		= ((CampBaseClass*)lastToHit)->GetCampID();
				deathMessage->dataBlock.fSide		= ((CampBaseClass*)lastToHit)->GetOwner();
			}
			deathMessage->dataBlock.fEntityID		= lastToHit->Id();
			deathMessage->dataBlock.fIndex			= lastToHit->Type();
		}
		else
		{
			// If aircraft is undamaged, the death message we send is 'ground collision',
			// since the aircraft would probably get there eventually
			deathMessage->dataBlock.damageType		= FalconDamageType::OtherDamage;
			deathMessage->dataBlock.fPilotID		= pilotSlot;
			deathMessage->dataBlock.fCampID			= GetCampaignObject()->GetCampID();
			deathMessage->dataBlock.fSide			= GetCampaignObject()->GetOwner();
			deathMessage->dataBlock.fEntityID		= Id();
			deathMessage->dataBlock.fIndex			= Type();
		}
		
		deathMessage->RequestOutOfBandTransmit ();
		
		FalconSendMessage (deathMessage, TRUE);
		
		
		SetDead (TRUE);
	}
	else
	{
		// make sure we're no longer in simple model
		if ( af )
			af->SetSimpleMode( SIMPLE_MODE_OFF );
		
		SetFlag( PILOT_EJECTED );
		VehicleClassDataType *vc = GetVehicleClassData(Type() - VU_LAST_ENTITY_TYPE);

		for (int pilotno = 0; pilotno < vc->NumberOfPilots; pilotno++) {
		// Create a new ejected pilot sim object.
		epc = NULL;
		epc = new EjectedPilotClass(this, EM_F16_MODE1, pilotno);
		
		if (epc != NULL)
		{
			vuDatabase->Insert(epc);
			epc->Wake();
			// KCK: We should really Wake() this with an overloaded wake function,
			// but the Insertion callback will do most of our waking stuff.
			
			// Play with this.
			epc->SetTransmissionTime
				(
				vuxRealTime + (unsigned long)((float)rand()/RAND_MAX * epc->UpdateRate())
				);
		}
		if (pilotno == 0 && IsLocal())
		{
			// PJW 12/3/97... added Eject message broadcast
			ShiAssert(GetCampaignObject() != NULL)
				
				ejectMessage=new FalconEjectMessage(epc->Id(), FalconLocalGame);
			if(ejectMessage != NULL)
			{
				ejectMessage->dataBlock.ePlaneID		= Id();
//				ejectMessage->dataBlock.eEjectID		= epc->Id();
				ejectMessage->dataBlock.eCampID			= ((CampBaseClass*)GetCampaignObject())->GetCampID();
				ejectMessage->dataBlock.eFlightID   = GetCampaignObject()->Id();
//				ejectMessage->dataBlock.eSide			= ((CampBaseClass*)GetCampaignObject())->GetOwner();
				ejectMessage->dataBlock.ePilotID		= pilotSlot;
//				ejectMessage->dataBlock.eIndex			= Type();
				ejectMessage->dataBlock.hadLastShooter	= (uchar)(LastShooter() != FalconNullId);
				
				FalconSendMessage(ejectMessage, TRUE);
			}
			
			// If there is a wingman that is not the player send the airman down message, and ask for sar
			for (i=0; i<GetCampaignObject()->NumberOfComponents(); i++)
			{
				if (GetCampaignObject()->GetComponentEntity(i) != this &&
					!((SimBaseClass*)GetCampaignObject()->GetComponentEntity(i))->IsSetFlag(MOTION_OWNSHIP))
				{
					SimBaseClass* sender = GetCampaignObject()->GetComponentEntity(i);
					FalconRadioChatterMessage* msg;
					
					msg = new FalconRadioChatterMessage(sender->Id(), FalconLocalGame);
					msg->dataBlock.message = rcAIRMANDOWNA;
					msg->dataBlock.from = Id();
					msg->dataBlock.to = MESSAGE_FOR_TEAM;
					msg->dataBlock.voice_id = (uchar)sender->GetPilotVoiceId();
					msg->dataBlock.edata[0] = (short)sender->GetCallsignIdx();
					msg->dataBlock.edata[1] = (short)sender->GetSlot();
					msg->dataBlock.time_to_play = 2 * CampaignSeconds;
					FalconSendMessage(msg, FALSE);
					// Another...
					msg = new FalconRadioChatterMessage(sender->Id(), FalconLocalGame);
					msg->dataBlock.message = rcAIRMANDOWNF; // was rcAIRMANDOWNE - totally wrong !! Is now: "setup RESCAP"
					msg->dataBlock.from = sender->Id();
					msg->dataBlock.to = MESSAGE_FOR_TEAM;
					msg->dataBlock.voice_id = (uchar)sender->GetPilotVoiceId();
					msg->dataBlock.time_to_play = 4 * CampaignSeconds;
					msg->dataBlock.edata[0] = -1;
					msg->dataBlock.edata[1] = -1;
					// Using X and Y position doesn't work...we need bearing and distance...
					//GetCampaignObject()->GetLocation(&msg->dataBlock.edata[0],&msg->dataBlock.edata[1]);
					FalconSendMessage(msg, FALSE); // Added - why not send when we've set it up ?
					//delete msg; // JB 011130 Mem leak FalconSendMessage deletes the memory now
					if (!(rand()%5) && RequestSARMission ((FlightClass*)GetCampaignObject()))
					{
						// Generate a SAR radio call from awacs
						FalconRadioChatterMessage* radioMessage;
						radioMessage = CreateCallFromAwacs((FlightClass*)GetCampaignObject(), rcSARENROUTE);
						radioMessage->dataBlock.time_to_play = (5 + rand() % 5) * CampaignSeconds;
						FalconSendMessage(radioMessage, FALSE);
					}
					break;
				}
			}
		}
		}
	}
}

VU_ID AircraftClass::HomeAirbase(void)
{
	WayPointClass* tmpWaypoint = waypoint;
	VU_ID airbase = FalconNullId;
	
	while (tmpWaypoint)
	{
		if (tmpWaypoint->GetWPAction() == WP_LAND)
		{
			airbase = tmpWaypoint->GetWPTargetID();
			break;
		}
		tmpWaypoint = tmpWaypoint->GetNextWP();
	}
	
	return airbase;
}

VU_ID AircraftClass::TakeoffAirbase(void)
{
	WayPointClass* tmpWaypoint = waypoint;
	VU_ID airbase = FalconNullId;
	
	while (tmpWaypoint)
	{
		if (tmpWaypoint->GetWPAction() == WP_TAKEOFF)
		{
			airbase = tmpWaypoint->GetWPTargetID();
			break;
		}
		tmpWaypoint = tmpWaypoint->GetNextWP();
	}
	
	return airbase;
}

VU_ID AircraftClass::LandingAirbase(void)
{
	WayPointClass* tmpWaypoint = waypoint;
	VU_ID airbase = FalconNullId;
	
	while (tmpWaypoint)
	{
		if (tmpWaypoint->GetWPAction() == WP_LAND)
		{
			airbase = tmpWaypoint->GetWPTargetID();
			break;
		}
		tmpWaypoint = tmpWaypoint->GetNextWP();
	}
	
	return airbase;
}

VU_ID AircraftClass::DivertAirbase(void)
{
	WayPointClass* tmpWaypoint = waypoint;
	VU_ID airbase = FalconNullId;
	
	while (tmpWaypoint)
	{
		if (tmpWaypoint->GetWPFlags() & WPF_ALTERNATE)
		{
			airbase = tmpWaypoint->GetWPTargetID();
			break;
		}
		tmpWaypoint = tmpWaypoint->GetNextWP();
	}
	
	return airbase;
}

int AircraftClass::CombatClass (void)
{
   return SimACDefTable[((Falcon4EntityClassType*)EntityType())->vehicleDataIndex].combatClass;
}

float AircraftClass::Mass (void)
{
	return af->Mass();
}


void AircraftClass::CleanupLitePool(void)
{
	if(pLandLitePool) {
		OTWDriver.RemoveObject(pLandLitePool);
		delete pLandLitePool;
		mInhibitLitePool = TRUE;
		pLandLitePool = NULL;
	}
}

// 2000-11-17 ADDED BY S.G. SO THE ENGINE TEMPERATURE IS SENT AS WELL AS THE RPM
//me123 changed back to original
void AircraftClass::SetPowerOutput (float powerOutput)
{
	int send = FALSE,
		diff,
		value;

	// RPM
	value = FloatToInt32(powerOutput / 1.5f * 255.0f);

	ShiAssert( value >= 0    );
	ShiAssert( value <= 0xFF );

	specialData.powerOutput = powerOutput;

	diff = specialData.powerOutputNet - value;

	if ((diff < -16) || (diff > 16))
	{
		//MonoPrint ("%08x SPO %f\n", this, powerOutput);

		specialData.powerOutputNet = static_cast<uchar>(value);
		send = TRUE;
	}
/*// disabled me123
	// ENGINE TEMP
	value = FloatToInt32(engineTempOutput / 1.6f * 255.0f);

	ShiAssert( value >= 0    );
	ShiAssert( value <= 0xFF );

	specialData.engineHeatOutput = engineTempOutput;

	diff = specialData.engineHeatOutputNet - value;

	if ((diff < -16) || (diff > 16))
	{
		//MonoPrint ("%08x SPO %f\n", this, powerOutput);

		specialData.engineHeatOutputNet = static_cast<uchar>(value);
		send = TRUE;
	}
*/
	// Send if one of the value changed
	if (send)
		MakeSimBaseDirty (DIRTY_SIM_POWER_OUTPUT, DDP[162].priority);
//			MakeSimBaseDirty (DIRTY_SIM_POWER_OUTPUT, SEND_SOMETIME);
}
// END OF ADDED SECTION

// JPO all relevant things to on.
void AircraftClass::PreFlight ()
{
	RALTCoolTime = -1.0F;
	RALTStatus = AircraftClass::RaltStatus::RON;
    af->ClearEngineFlag(AirframeClass::MasterFuelOff);
    af->SetFuelSwitch(AirframeClass::FS_NORM);
    af->SetFuelPump (AirframeClass::FP_NORM);
    af->SetAirSource(AirframeClass::AS_NORM);
	af->canopyState = false;
    af->QuickEngineStart();
    if (OnGround()) {
	af->TEFTakeoff();
	af->LEFTakeoff();
    }
	//MI
	SeatArmed = TRUE;
	mainPower = MainPowerMain;
	currentPower = PowerNonEssentialBus;
	EWSPgm = Man;
	if(Sms)
		Sms->SetMasterArm(SMSBaseClass::Arm);	//set it to arm
    PowerOn(AllPower);
	//MI lighting
	ExtlOn(Extl_Main_Power);
	ExtlOn(Extl_Anit_Coll);
	ExtlOn(Extl_Wing_Tail);
	SetStatusBit(STATUS_EXT_LIGHTS|STATUS_EXT_NAVLIGHTS|STATUS_EXT_TAILSTROBE);
	if (IsComplex()) { // only for those that support it.
	    SetSwitch(COMP_NAV_LIGHTS, TRUE);
	    SetSwitch(COMP_TAIL_STROBE, TRUE);
	}
	//MI INS
	INSOn(INS_Nav);
	INSOff(INS_PowerOff);
	INSOff(INS_AlignNorm);
	if(!g_bINS)
	{
		INSOn(AircraftClass::INS_ADI_OFF_IN);
		INSOn(AircraftClass::BUP_ADI_OFF_IN);
		INSOn(AircraftClass::INS_ADI_AUX_IN);
		INSOn(AircraftClass::INS_HUD_STUFF);
		INSOn(AircraftClass::INS_HSI_OFF_IN);
		INSOn(AircraftClass::INS_HSD_STUFF);
	}
	HasAligned = TRUE;
	INSStatus = 10;
	INSAlignmentTimer = 480.0F;
	INSLatDrift = 0.0F;
	INSAlignmentStart = vuxGameTime;
	BUPADIEnergy = 540.0F;	//9 minutes useable
	//Set the missile and threat volume
	MissileVolume = 0;	//max vol
	ThreatVolume = 0;
	//Targeting Pod cooled
	PodCooling = 0.0F;
	//MI HUD
	if(TheHud && this == SimDriver.playerEntity && (!OnGround() || 
		PlayerOptions.GetStartFlag() != PlayerOptionsClass::START_RAMP))
	{
		TheHud->SymWheelPos = 1.0F;
		TheHud->SetLightLevel();
	}

	if(PlayerOptions.GetStartFlag() != PlayerOptionsClass::START_RAMP && OnGround())
		af->SetFlag(AirframeClass::NoseSteerOn);

	//MI voice volume stuff
	if(IsPlayer() && OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp)
	{
		OTWDriver.pCockpitManager->mpIcp->Comm1Volume = 0;	//max vol
		OTWDriver.pCockpitManager->mpIcp->Comm2Volume = 0;
	}
}
void AircraftClass::StepSeatArm(void)
{
	if(SeatArmed)
		SeatArmed = FALSE;
	else
		SeatArmed = TRUE;
}

void AircraftClass::IncMainPower()
{
    switch (mainPower) {
    case MainPowerOff:
	mainPower = MainPowerBatt;
	//MI
	if(!g_bINS)
	{
		INSOn(AircraftClass::INS_ADI_OFF_IN);
		INSOn(AircraftClass::INS_ADI_AUX_IN);;
		INSOn(AircraftClass::INS_HUD_STUFF);
		INSOn(AircraftClass::INS_HSI_OFF_IN);
		INSOn(AircraftClass::INS_HSD_STUFF);
		INSOn(AircraftClass::BUP_ADI_OFF_IN);
		GSValid = TRUE;
		LOCValid = TRUE;
	}
	break;
    case MainPowerBatt:
	mainPower = MainPowerMain;
	break;
    case MainPowerMain: // cant go any further
	break;
    }
}

void AircraftClass::DecMainPower()
{
    switch (mainPower) {
    case MainPowerOff:
	break;
    case MainPowerBatt:
	mainPower = MainPowerOff;
	//MI
	if(!g_bINS)
	{
		INSOff(AircraftClass::INS_ADI_OFF_IN);
		INSOff(AircraftClass::INS_ADI_AUX_IN);;
		INSOff(AircraftClass::INS_HUD_STUFF);
		INSOff(AircraftClass::INS_HSI_OFF_IN);
		INSOff(AircraftClass::INS_HSD_STUFF);
		INSOff(AircraftClass::BUP_ADI_OFF_IN);
		GSValid = FALSE;
		LOCValid = FALSE;
	}
	break;
    case MainPowerMain:
	mainPower = MainPowerBatt;
	break;
    }
}
void AircraftClass::IncEWSPGM()
{
	PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(this, SensorClass::RWR);

    switch (EWSPgm) 
	{
    case Off:
		EWSPgm = Stby;
		if(theRwr)
		{
			theRwr->InEWSLoop = FALSE;
			theRwr->ReleaseManual = FALSE;
		}
		break;
	case Stby:
		EWSPgm = Man;
		if(theRwr)
		{
			theRwr->InEWSLoop = FALSE;
			theRwr->ReleaseManual = FALSE;
		}
		break;
	case Man:
		EWSPgm = Semi;
		break;
    case Semi:
		EWSPgm = Auto;
		break;
    case Auto: // cant go any further
	break;
    }
}

void AircraftClass::DecEWSPGM()
{
	PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(this, SensorClass::RWR);

    switch (EWSPgm) 
	{
    case Off:
		if(theRwr)
		{
			theRwr->InEWSLoop = FALSE;
			theRwr->ReleaseManual = FALSE;
		}
		break;
    case Auto:
		EWSPgm = Semi;
		break;
    case Semi:
		EWSPgm = Man;
		break;
	case Man:
		EWSPgm = Stby;
		if(theRwr)
		{
			theRwr->InEWSLoop = FALSE;
			theRwr->ReleaseManual = FALSE;
		}
		break;
	case Stby:
		EWSPgm = Off;
		if(theRwr)
		{
			theRwr->InEWSLoop = FALSE;
			theRwr->ReleaseManual = FALSE;
		}
		break;
    }
}
void AircraftClass::IncEWSProg()
{
	PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(this, SensorClass::RWR);

	if(theRwr)
	{
		theRwr->InEWSLoop = FALSE;
		theRwr->ReleaseManual = FALSE;
	}

    if(EWSProgNum < 3)
		EWSProgNum++;
}

void AircraftClass::DecEWSProg()
{
	PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(this, SensorClass::RWR);

	if(theRwr)
	{
		theRwr->InEWSLoop = FALSE;
		theRwr->ReleaseManual = FALSE;
	} 

	if(EWSProgNum > 0)
		EWSProgNum--;	
}
void AircraftClass::ToggleBetty(void)
{
	if(playBetty)
		playBetty = FALSE;
	else
		playBetty = TRUE;
}
//MI
void AircraftClass::AddAVTRSeconds(void)
{
	if(AVTRState(AircraftClass::AVTRStateFlags::AVTR_AUTO))
		AVTRCountown = 30.0F;
}
void AircraftClass::SetCockpitWingLight(bool state)
{
	CockpitWingLight = state;
}
void AircraftClass::SetCockpitStrobeLight(bool state)
{
	CockpitStrobeLight = state;
}

// JPO 
// this replicates the stuff in flight 
// I think it should only be in one place, but time is pressing.
// it always was in two places, just that this version didn't work too well.
int AircraftClass::FindBestSpawnPoint(Objective obj, SimInitDataClass* initData)
{
	int pt = initData->ptIndex;
	int pktype = af->GetParkType();
	if (GetNextTaxiPt(initData->ptIndex)== 0) { // means were at first point, try and find a parking spot
		int npt;
		// first find last (nearest runway) parking spot
		while ((npt = GetPrevParkTypePt(pt, pktype)) != 0) {
			pt = npt;
		}
		// now go back through them, looking for first available.
		while (pt) {
			TranslatePointData(obj, pt, &af->x, &af->y);
			if (CheckPointGlobal(initData->campUnit,af->x,af->y) == NULL)
				break; // not in use
			pt = GetNextParkTypePt(pt, pktype);
		}
		//MonoPrint ("Vehicle %d at Pt %d Type %d %f,%f\n", initData->vehicleInUnit, pt, PtDataTable[pt].type, af->x, af->y);
		if (pt) {// found somewhere
			npt = GetPrevTaxiPt(pt); // this is next place to go to
			float x, y;
			TranslatePointData(obj, npt, &x, &y);
			af->initialPsi = af->psi = af->sigma = (float)atan2 ( (y - af->y), (x - af->x) );
			af->initialX = af->groundAnchorX = af->x;
			af->initialY = af->groundAnchorY = af->y;
			return pt;
		}
	}
	// fall back to the old mechanism.
	TranslatePointData(obj, initData->ptIndex, &af->x, &af->y);
	runwayQueueStruct *info = 0;
	if (obj->brain && initData->campUnit)
		info = obj->brain->InList(initData->campUnit->Id());
	int rwindex = 0;
	if(info)
		rwindex = info->rwindex;

	// otherwise, back to the old idea
	while(CheckPointGlobal(initData->campUnit,af->x,af->y))
	{
		if(GetNextPt(initData->ptIndex))
		{
			initData->ptIndex = GetNextPt(initData->ptIndex);
			TranslatePointData(obj, initData->ptIndex, &af->x, &af->y);
		}
		else {
			float wid = max(50, af->GetAeroData(AeroDataSet::Span)+10);
			if(!rwindex)
				af->x -= wid;
			else if(PtHeaderDataTable[rwindex].ltrt < 0)
			{
				af->x -= -PtHeaderDataTable[rwindex].sinHeading * wid;
				af->y -= PtHeaderDataTable[rwindex].cosHeading * wid;
			}
			else
			{
				af->x -= PtHeaderDataTable[rwindex].sinHeading * wid;
				af->y -= -PtHeaderDataTable[rwindex].cosHeading * wid;
			}
			
		}
	}
	af->initialX = af->groundAnchorX = af->x;
	af->initialY = af->groundAnchorY = af->y;
	return initData->ptIndex;
}
