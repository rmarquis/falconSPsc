#include <windows.h>
#include <process.h>
#include "stdhdr.h"
#include "falcsess.h"
#include "f4thread.h"
#include "fsound.h"
#include "soundfx.h"
#include "f4vu.h"
#include "simveh.h"
#include "simfile.h"
#include "simio.h"
#include "PilotInputs.h"
#include "team.h"
#include "camp2sim.h"
#include "ClassTbl.h"
#include "CampLib.h"
#include "otwdrive.h"
#include "simdrive.h"
#include "falcmesg.h"
#include "f4error.h"
#include "initdata.h"
#include "simfiltr.h"
#include "fcc.h"
#include "datadir.h"
#include "dogfight.h"
#include "iaction.h"
#include "unit.h"
#include "division.h"
#include "objectiv.h"
#include "OwnResult.h"
#include "MsgInc\SimTimingMsg.h"
#include "ThreadMgr.h"
#include "mvrdef.h"
#include "aircrft.h"
#include "acmi\src\include\acmirec.h"
#include "simfeat.h"
#include "feature.h"
#include "PlayerOp.h"
#include "resource.h"
#include "sinput.h"
#include "commands.h"
#include "cpmanager.h"
#include "inpFunc.h"
#include "tacan.h"
#include "navsystem.h"
#include "missile.h"
#include "bomb.h"
#include "chaff.h"
#include "flare.h"
#include "debris.h"
#include "object.h"
#include "sfx.h"
#include "ground.h"
#include "guns.h"
#include "Persist.h"
#include "Find.h"
#include "TimerThread.h"
#include "rwr.h"		// Goes once the RWR data is in the class table (if that ever happens)
#include "falcsnd\voicemanager.h"
#include "Graphics\Include\draw2d.h"
#include "Graphics\Include\drawtrcr.h"
#include "Graphics\Include\drawgrnd.h"
#include "Graphics\Include\drawbldg.h"
#include "Graphics\Include\drawbsp.h"
#include "Graphics\Include\drawbrdg.h"
#include "Graphics\Include\drawovc.h"
#include "Graphics\Include\drawplat.h"
#include "Graphics\Include\drawrdbd.h"
#include "Graphics\Include\drawsgmt.h"
#include "Graphics\Include\drawpuff.h"
#include "Graphics\Include\drawshdw.h"
#include "Graphics\Include\drawpnt.h"
#include "Graphics\Include\drawguys.h"
#include "Graphics\Include\drawpole.h"
#include "airframe.h"
#include "digi.h"
#include "evtparse.h"
#include "voicecomunication\voicecom.h"//me123

// KCK added include stuff
#include "CmpClass.h"
#include "Weather.h"
#include "falcsess.h"
#include "Campaign.h"
#include "simloop.h"
#include "campwp.h"
#include "sms.h"
#include "hardpnt.h"
#include "GameMgr.h"
#include "digi.h"
#include "helo.h"
#include "hdigi.h"
#include "radar.h"
#include "fault.h"
#include "simvudrv.h"
#include "listadt.h"
#include "UI\INCLUDE\tac_class.h"
#include "UI\INCLUDE\te_defs.h"
#include "msginc\atcmsg.h"
//me123
#include "flight.h"
// OW FIXME: this had to be added after installing the DX8 Beta 1 SDK
#define DIRECTINPUT_VERSION 0x0700
#include "dinput.h"

#include <mbstring.h>
#include "simbase.h"
#include "codelib\tools\lists\lists.h"
#include "acmi\src\include\acmitape.h"

// for debugging memory leakage while sim is running
#ifdef DEBUG
// #define CHECK_LEAKAGE
#endif

#ifdef USE_SH_POOLS
MEM_POOL gTextMemPool;
MEM_POOL gObjMemPool;
MEM_POOL gReadInMemPool;
extern MEM_POOL gTacanMemPool;
#endif

#ifdef CHECK_LEAKAGE
extern MEM_BOOL MEM_CALLBACK errPrint(MEM_ERROR_INFO *errorInfo);
extern MEM_ERROR_FN lastErrorFn;
#endif

void GraphicsDataPoolInitializeStorage (void);
void GraphicsDataPoolReleaseStorage (void);

extern void SavePersistantList(char* scenario);
extern C_Handler *gMainHandler;
extern int gNumWeaponsInAir;
extern HWND mainMenuWnd;
extern int FileVerify (void);

extern ulong gBumpTime;
extern int	gBumpFlag;
// Control Defines
//#define DISPLAY_BLOW_COUNT

// Exported Globals
SimulationDriver SimDriver;

// Imported Variables
extern void CalcTransformMatrix (SimBaseClass* theObject);

// Static Variables
#ifdef DISPLAY_BLOW_COUNT
static int BlowCount = 0;
#endif
static int debugging = FALSE;
int EndFlightFlag = FALSE;
int gOutOfSimFlag = TRUE;

void ReadAllAirframeData(void);
void ReadAllMissileData(void);
void ReadDigitalBrainData(void);
void FreeAllAirframeData(void);
void FreeAllMissileData(void);
void FreeDigitalBrainData(void);
void JoystickStopAllEffects(void);

static unsigned int __stdcall SimExecWrapper (void* myself);
//static void ProximityCheck (SimBaseClass* testObj);

typedef struct NewRequestListItem
{
   VU_ID requestId;
   struct NewRequestListItem* next;
} NewRequestList;

static NewRequestList* requestList = NULL;

float SimLibLastMajorFrameTime;


#ifdef CHECK_PROC_TIMES
ulong gMaxAircraftProcTime = 0;
ulong gMaxGndProcTime = 0;
ulong gMaxATCProcTime = 0;
ulong gLastAircraftProcTime = 0;
ulong gLastGndProcTime = 0;
ulong gLastATCProcTime = 0;
ulong gAveAirProcTime = 0;
ulong gAveGndProcTime = 0;
int	numAircraft = 0;
int numGrndVeh = 0;
ulong gLastSoundFxTime = 0;
ulong gMaxSoundFxTime = 0;
ulong gLastSFXTime = 0;
ulong gMaxSFXTime = 0;
ulong gILSTime = 0;
ulong gMaxILSTime = 0;
ulong gOtherMaxTime = 0;
ulong gOtherLastTime = 0;
ulong gAveOtherProcTime = 0;
int numOther = 0;
ulong AllObjTime = 0;
static ulong gWhole = 0;

extern ulong gACMI;
extern ulong gSpecCase;
extern ulong gTurret;
extern ulong gLOD;
extern ulong gFire;
extern ulong gMove;
extern ulong gThink;
extern ulong gTarg;
extern ulong gRadar;
extern ulong gFireCont;
extern ulong gSimVeh;
extern ulong gSFX;

extern ulong gCommChooseTarg;
extern ulong gSelNewTarg;
extern ulong gRadarTarg;
extern ulong gConfirmTarg;
extern ulong gDoWeapons;
extern ulong gSelWeapon;

extern ulong gSelBestWeap;
extern int numCalls;

extern ulong gWeapRng;
extern int numWRng;
extern ulong gWeapHCh;
extern int numWHCh;
extern ulong gWeapScore;
extern int numWeapScore;
extern ulong gCanShoot;
extern int numCanShoot;

extern ulong gRotTurr;
extern ulong gTurrCalc;
extern ulong gKeepAlive;
extern ulong gFireTime;

extern ulong terrTime;
#endif

//MI
extern FireControlComputer::FCCMasterMode playerLastMasterMode;
extern FireControlComputer::FCCSubMode playerLastSubMode;
// ==========================================================

SimulationDriver::SimulationDriver (void)
{
   SimLibElapsedTime = vuxGameTime;
   SimLibFrameCount = 0;
   objectList = NULL;
   combinedList = NULL;
   combinedFeatureList = NULL;
   ObjsWithNoCampaignParentList = NULL;
   featureList = NULL;
   campUnitList = NULL;
   campObjList = NULL;
   motionOn = TRUE;
   playerEntity = NULL;
   doEvent = FALSE;
   doFile = FALSE;
   doExit = FALSE;
   doGraphicsExit = FALSE;
   facList = NULL;
   tankerList = NULL;
   atcList = NULL;
   avtrOn = FALSE;
   lastRealTime = 0;
}
  
SimulationDriver::~SimulationDriver (void)
{
}


// KCK: Startup is called on falcon startup before the loop calling Cycle is started.
void SimulationDriver::Startup (void)
{
FalconMessageFilter messageFilter(FalconEvent::SimThread, VU_DELETE_EVENT_BITS);



   SetFrameDescription (50, 1);
   motionOn = TRUE;

   // Prep database filtering
   objectList = NULL;
   campUnitList = NULL;
   campObjList = NULL;
   combinedList = NULL;
   combinedFeatureList = NULL;
   ObjsWithNoCampaignParentList = NULL;

   //Prep Object Data
	// Check file integrity
	FileVerify();

   SimMoverDefinition::ReadSimMoverDefinitionData();
   ReadDigitalBrainData();
   ReadAllMissileData();
   ReadAllAirframeData();
   ReadAllRadarData(); // JPO addition to read .dat files

   AllSimFilter				allSimFilter;
	SimDynamicTacanFilter	dynamicTacanFilter;

   SimObjectFilter objectFilter;
   objectList = new FalconPrivateOrderedList(&objectFilter);
   objectList->Init();

   UnitFilter	unitFilter(0,1,0,0);
   campUnitList = new FalconPrivateOrderedList(&unitFilter);
   campUnitList->Init();

   ObjFilter	objectiveFilter(0);
   campObjList = new FalconPrivateOrderedList(&objectiveFilter);
   campObjList->Init();

   SimFeatureFilter featureFilter;
   featureList = new FalconPrivateOrderedList(&featureFilter);
   featureList->Init();
   facList = new FalconPrivateList(&allSimFilter);
   facList->Init();

   SimAirfieldFilter	airbaseFilter;
   atcList = new VuFilteredList(&airbaseFilter);
   atcList->Init();

   tankerList = new VuFilteredList(&dynamicTacanFilter);
   tankerList->Init();


   CombinedSimFilter combinedFilter;
   combinedList = new FalconPrivateOrderedList(&combinedFilter);
   combinedList->Init();

   combinedFeatureList = new FalconPrivateOrderedList(&combinedFilter);
   combinedFeatureList->Init();

   ObjsWithNoCampaignParentList = new FalconPrivateList(&allSimFilter);
   ObjsWithNoCampaignParentList->Init();

   EndFlightFlag = FALSE;
#ifndef NO_TIMER_THREAD
   vuThread = new VuThread (&messageFilter, F4_EVENT_QUEUE_SIZE);
#endif

	last_elapsedTime = 0;
	lastRealTime = vuxGameTime;
	lastFlyState = -1;
	curFlyState = 0;
}


// SCR:  Called at exit of application after the loop calling "Cycle" has gone away.
void SimulationDriver::Cleanup (void)
{
   FreeAllAirframeData();
   FreeAllMissileData();
   FreeDigitalBrainData();
   SimMoverDefinition::FreeSimMoverDefinitionData();

   objectList->DeInit();
   delete objectList;
   featureList->DeInit();
   delete featureList;
   campUnitList->DeInit();
   delete campUnitList;
   campObjList->DeInit();
   delete campObjList;
   
   atcList->DeInit();
   delete atcList;
   facList->DeInit();
   delete facList;
   tankerList->DeInit();
   delete tankerList;

   combinedList->DeInit();
   delete combinedList;

   combinedFeatureList->DeInit();
   delete combinedFeatureList;

   ObjsWithNoCampaignParentList->DeInit();
   delete ObjsWithNoCampaignParentList;

   ObjsWithNoCampaignParentList = NULL;
   combinedFeatureList = NULL;
   combinedList = NULL;
   tankerList = NULL;
   objectList = NULL;
   featureList = NULL;
   campUnitList = NULL;
   campObjList = NULL;
#ifndef NO_TIMER_THREAD
   vuThread->Update();
   delete vuThread;
   vuThread = NULL;
#endif
}


// SCR:  To be called each time we go from UI to Sim (once code is unified)
void SimulationDriver::Enter (void)
{
	nextATCTime = 0;
	OwnResults.ClearData();
   gNumWeaponsInAir = 0;

	if (SimDriver.doEvent)
		F4EventFile = OpenCampFile("lastflt", "acm", "wb");
	else
		F4EventFile = NULL;

	if (IO.Init((char *)"joy1.dat") != SIMLIB_OK)
	{
		MonoPrint ("No Joystick Connected\n");
	}

	OTWDriver.SetActive(TRUE);

//	if(RunningDogfight()) {		// VWF added 2/2/99 as per RobinH's Request
//		VM->ChangeRadioFreq( rcfAll, 0 );
//		VM->ChangeRadioFreq( rcfAll, 1 );
//	}
}

// Called each time we go from Sim to UI
void SimulationDriver::Exit (void)
{

SimBaseClass* theObject;
VuListIterator objectWalker (objectList);

	// Kill All remaining awake sim Entities
	theObject = (SimBaseClass*)objectWalker.GetFirst();
	while (theObject)
		{
		ShiAssert (theObject->IsLocal());
		//ShiAssert (!theObject->GetCampaignObject());
		theObject->SetDead(TRUE);
		theObject = (SimBaseClass*)objectWalker.GetNext();
		}

#ifdef DEBUG
	VuListIterator featureWalker (featureList);
	theObject = (SimBaseClass*)featureWalker.GetFirst();
	while (theObject)
		{
		ShiAssert (theObject);
		theObject->Sleep();
		theObject = (SimBaseClass*)featureWalker.GetNext();
		}

	VuListIterator campUnitWalker (campUnitList);
	ShiAssert (!campUnitWalker.GetFirst());
	VuListIterator campObjWalker (campObjList);
	ShiAssert (!campObjWalker.GetFirst());

#endif

	if (F4EventFile)
		fclose(F4EventFile);
	F4EventFile = NULL;

	if ( gACMIRec.IsRecording() )
		gACMIRec.StopRecording();

	F4SoundFXEnd();
	F4SoundStop();
}

void SimulationDriver::Cycle (void)
{
#ifdef CHECK_PROC_TIMES
	gLastAircraftProcTime = 0;
	gLastGndProcTime = 0;
	gLastATCProcTime = 0;
	gAveAirProcTime = 0;
	gAveGndProcTime = 0;
	numAircraft = 0;
	numGrndVeh = 0;
	ulong startTime = 0;
	gLastSFXTime = 0;
	gMaxSFXTime = 0;

	gOtherMaxTime = 0;
	gOtherLastTime = 0;
	gAveOtherProcTime = 0;
	numOther = 0;
	AllObjTime = 0;
	gFireTime = 0;
	gWhole = GetTickCount();
#endif
	SimBaseClass	*theObject;
	VuListIterator	objectWalker (objectList);
	static int runFrame,runGraphics;
	long	elapsedTime;
	float	gndz;

	// Catch up for time elapsed
	//ShiAssert(lastRealTime <= vuxGameTime);
	if(lastRealTime > vuxGameTime)
		lastRealTime = vuxGameTime;
	elapsedTime = vuxGameTime - lastRealTime;
	curFlyState = FalconLocalSession->GetFlyState();
	RefreshVoiceFreqs();//me123
	if ((elapsedTime >= 10) && (gameCompressionRatio))
	//if (gameCompressionRatio)
	{
		// Check if the graphics are runnning and read inputs, if so.
		if (curFlyState == FLYSTATE_FLYING || curFlyState == FLYSTATE_DEAD)
		{
			UserStickInputs.Update();
			runGraphics = TRUE;
		}
		else
		{
			runGraphics = FALSE;
		}
	
		SimLibLastMajorFrameTime = SimLibMajorFrameTime;
		SimLibMajorFrameTime = max (0.01F, ((float)(elapsedTime)) / SEC_TO_MSEC);

#ifdef DEBUG
      SimLibMajorFrameTime = min (0.5F, SimLibMajorFrameTime);
#endif

		for ( SimLibMinorPerMajor = 1; SimLibMinorPerMajor < 100; SimLibMinorPerMajor++ )
		{
			SimLibMinorFrameTime = SimLibMajorFrameTime/(float)SimLibMinorPerMajor;
			if ( SimLibMinorFrameTime <= 0.05f )
				break;
		}

		runFrame = TRUE;

		SimLibMajorFrameRate = 1.0F / SimLibMajorFrameTime;
		SimLibMinorFrameRate = 1.0F / SimLibMinorFrameTime;

		lastRealTime = vuxGameTime;

#ifdef MAKE_MOVIE
		SetFrameDescription (66, 1);
#endif
	}
	else
	{
		runFrame = FALSE;
	}

	//lastRealTime = vuxGameTime;

	lastFlyState = curFlyState;
	last_elapsedTime = elapsedTime;


	// doFile is a flag that tells us a state change for ACMI recording
	// was made.  If TRUE, we need to toggle ACMI recording here.
	// we need to do this here because when recording starts it must
	// walk thru all objects (via a callback to SimDriver::InitACMIRecord)
	// and this must be thread safe
	//
	// SCR:  6/18/98  Can this go to a better place once the sim/graphics threads
	//                are merged?
	if ( doFile )
	{
		gACMIRec.ToggleRecording ();
		doFile = FALSE;
	}

	//me123 the following function is needed for host to send updates while in the ui
	//with the new mp code.
	int HostSendUpds = FALSE;
	if (vuLocalSessionEntity &&
		 vuLocalSessionEntity->Game() &&
		 vuLocalSessionEntity->Game()->OwnerId() == vuLocalSessionEntity->Id())
	{// we are hostign a game
		VuGameEntity* game;	game  = vuLocalSessionEntity->Game();
		VuSessionsIterator Sessioniter(game);
		VuSessionEntity*   sess;
		sess = Sessioniter.GetFirst();
		int flying = FALSE;
//FLYSTATE_IN_UI
		while (sess && !flying )
		{
			if (((FalconSessionEntity*)vuDatabase->Find(sess->Id()))->GetFlyState () != FLYSTATE_IN_UI)
			{
			flying = TRUE;
			}
			else sess = Sessioniter.GetNext ();
		}
		if (flying && !OTWDriver.IsActive() && FalconLocalSession->GetFlyState() == FLYSTATE_IN_UI 
			 && !curFlyState && !doGraphicsExit && !doExit &&!TheCampaign.IsSuspended())
		{
			Enter();
		}
		else if (flying && OTWDriver.IsActive() && FalconLocalSession->GetFlyState() == FLYSTATE_IN_UI 
			 && !curFlyState && !doGraphicsExit && !doExit &&!TheCampaign.IsSuspended())
		{	
		if (sess->Camera(0) && OTWDriver.GetViewpoint())
			{
			OTWDriver.SetOwnshipPosition (sess->Camera(0)->XPos(),sess->Camera(0)->YPos(),sess->Camera(0)->ZPos());
			OTWDriver.ServerSetviewPoint();
			}
		}
		if (0)
		{
			FlightClass		*flight;
			flight = FalconLocalSession->GetPlayerFlight();
			FalconLocalSession->AttachCamera(flight->Id());
			if (0) 
			{
				vuLocalSessionEntity->ClearCameras();
			}

		}
	}

	if (runFrame)
	{
#ifdef CHECK_PROC_TIMES
		gLastSFXTime = GetTickCount();
#endif
		// Update special effects
		OTWDriver.DoSfxActiveList();
#ifdef CHECK_PROC_TIMES
		gLastSFXTime = GetTickCount() - gLastSFXTime;
		if(gLastSFXTime > gMaxSFXTime)
			gMaxSFXTime = gLastSFXTime;
#endif

#ifdef CHECK_PROC_TIMES
		gACMI = 0;
		gSpecCase = 0;
		gTurret = 0;
		gLOD = 0;
		gFire = 0;
		gMove = 0;
		gThink = 0;
		gTarg = 0;
		gRadar = 0;
		gFireCont = 0;
		gSimVeh = 0;
		gSFX = 0;
		gCommChooseTarg = 0;
		gSelNewTarg = 0;
		gRadarTarg = 0;
		gConfirmTarg = 0;
		gDoWeapons = 0;
		gSelWeapon = 0;
		
		gRotTurr = 0;
		gTurrCalc = 0;
		gKeepAlive = 0;
		//gSelBestWeap = 0;
		//numCalls = 0;
		AllObjTime = GetTickCount();
#endif
		// Do each entity's thinking
		theObject = (SimBaseClass*)objectWalker.GetFirst();
		while (theObject)
		{
#ifdef CHECK_PROC_TIMES
			startTime = GetTickCount();
#endif
			if (theObject->EntityDriver())
			{
				if
				(
					(theObject->IsSetFalcFlag (FEC_PLAYER_ENTERING)) &&
					(theObject->IsLocal ()) &&
					(RunningDogfight ())
				)
				{
//					MonoPrint ("Not Executing %08x - Player is Entering\n", theObject);
				}
				else
				{
					theObject->EntityDriver()->Exec(vuxGameTime);
				}
#ifdef CHECK_PROC_TIMES
	ulong procTime = GetTickCount() - startTime;
	if(theObject->IsAirplane())
	{
		if(!theObject->IsPlayer())
		{
			if(procTime > gMaxAircraftProcTime && elapsedTime)
				gMaxAircraftProcTime = procTime;
			if(procTime > gLastAircraftProcTime)
				gLastAircraftProcTime = procTime;

			gAveAirProcTime += procTime;
			numAircraft++;
		}
	}
	else if(theObject->IsGroundVehicle())
	{
		if(procTime > gMaxGndProcTime && elapsedTime)
			gMaxGndProcTime = procTime;
		if(procTime > gLastGndProcTime)
			gLastGndProcTime = procTime;

		gAveGndProcTime += procTime;
		numGrndVeh++;
	}
	else
	{
		if(procTime > gOtherMaxTime && elapsedTime)
			gOtherMaxTime = procTime;
		if(procTime > gOtherLastTime)
			gOtherLastTime = procTime;

		gAveOtherProcTime += procTime;
		numOther++;
	}
#endif
			}

			if (!theObject->IsLocal())
			{
				CalcTransformMatrix (theObject);

     			//LRKLUDGE
				gndz = OTWDriver.GetGroundLevel(theObject->XPos(), theObject->YPos());
				if (theObject->ZPos() > gndz)
				{
//					MonoPrint ("Clamping underground position for remote entity\n");
					theObject->SetPosition (theObject->XPos(), theObject->YPos(), gndz);
				}

				//LRKLUDGE
				if (theObject->ZPos() < -200000.0F)
				{
//					MonoPrint ("Clamping underground position for remote entity\n");
					theObject->SetPosition (theObject->XPos(), theObject->YPos(), -200000.0F);
				}
			}
			theObject = (SimBaseClass*)objectWalker.GetNext();
		}
#ifdef CHECK_PROC_TIMES
			AllObjTime = GetTickCount() - AllObjTime;
#endif
		SimLibFrameCount ++;
		SimLibElapsedTime += FloatToInt32(SimLibMajorFrameTime*SEC_TO_MSEC + 0.5F);
		//ShiAssert(SimLibElapsedTime <= vuxGameTime + SimLibMajorFrameTime*SEC_TO_MSEC);
#ifdef DAVE_DBG
		static ulong lastTime;
		static ulong curTime = GetTickCount();
		lastTime = curTime;
		curTime = GetTickCount();
		ulong delta = curTime - lastTime;
#endif
	}
	
	if (RunningDogfight())
	{
		SimDogfight.UpdateDogfight();
	}

	// Things to run once per loop when sim is running
	if (runGraphics)
	{
		// if we're not in a cockpit view, play wind noise
		if (!OTWDriver.DisplayInCockpit())
		{
			//edg note: since this was moved down here from above, the current
			// frame count is now beyond what the positional sound driver expects
			// for the current frame.  I'm just going to brackett this call with
			// a dec/inc of framecount so that nothing else that may be effected
			// by framecount will be effected.
			SimLibFrameCount--;
			F4SoundFXSetDist( SFX_WIND, 0, 0.0f, 1.0f );
			SimLibFrameCount++;
		}

	}

#ifdef CHECK_PROC_TIMES
	gLastSoundFxTime = GetTickCount();
#endif
	if ( gameCompressionRatio )
	{
		// run positional Sound FX
	
		F4SoundFXPositionDriver( SimLibFrameCount - 2, SimLibFrameCount - 1 );
	}
	else
	{
		F4SoundFXPositionDriver( SimLibFrameCount+1, SimLibFrameCount+1  );
	}
#ifdef CHECK_PROC_TIMES
	gLastSoundFxTime = GetTickCount() - gLastSoundFxTime;
	if(gLastSoundFxTime > gMaxSoundFxTime)
		gMaxSoundFxTime = gLastSoundFxTime;
#endif

	// need to run while in UI for other players
	// Update ATC for each airbase
#ifdef CHECK_PROC_TIMES
	gLastATCProcTime = GetTickCount();
#endif
	UpdateATC();
#ifdef CHECK_PROC_TIMES
	gLastATCProcTime = GetTickCount() - gLastATCProcTime;
	if(gLastATCProcTime > gMaxATCProcTime && elapsedTime)
		gMaxATCProcTime = gLastATCProcTime;

	gILSTime = GetTickCount();
#endif

	if(gNavigationSys) gNavigationSys->ExecIls(); // Needed for the Hud and Adi

#ifdef CHECK_PROC_TIMES
	gILSTime = GetTickCount() - gILSTime;
	if(gILSTime > gMaxILSTime && elapsedTime)
		gMaxILSTime = gILSTime;
#endif

	// check if the simloop thread has told us the sim is to be shut down
	if ( doExit )
	{
		Exit();
		doExit = FALSE;
		SetEvent (SimulationLoopControl::wait_for_sim_cleanup);
	}
	// check if the simloop thread has told us the graphics are to be shut down
	if ( doGraphicsExit )
	{
		OTWDriver.Exit();
		doGraphicsExit = FALSE;
		SetEvent (SimulationLoopControl::wait_for_graphics_cleanup);
	}
#ifdef CHECK_PROC_TIMES
	if(elapsedTime)
	{		
		float ACMIAve = 0.0F, SpecCaseAve = 0.0F, TurrAve = 0.0F, LODAve = 0.0F, FireAve = 0.0F, MoveAve = 0.0F, ThinkAve = 0.0F;
		float TargAve = 0.0F, RadarAve = 0.0F, FireContAve = 0.0F, SimVehAve = 0.0F, SFXAve = 0.0F, SelBestWeapAve = 0.0F;
		float CommAve = 0.0F, SelNewAve = 0.0F, RadTargAve = 0.0F, ConTargAve = 0.0F, DoWeapAve = 0.0F, SelWeapAve = 0.0F;
		float WeapRngAve = 0.0F, WeapChAve = 0.0F, WeapScAve = 0.0F, CanShootAve = 0.0F;
		float TurrCalcAve = 0.0F, RotTurrAve = 0.0F, KeepAlAve = 0.0F, FireTimeAve = 0.0F; 

		gWhole = GetTickCount() - gWhole;
		//MonoPrint("AirMax:      %4d  GndMax:   %4d  ATCMax:     %4d\n", gMaxAircraftProcTime, gMaxGndProcTime, gMaxATCProcTime);
		MonoPrint("AirLast:     %4d  GndLast:  %4d  OtherLast:  %4d\n", gLastAircraftProcTime, gLastGndProcTime, gOtherLastTime);
		MonoPrint("AirTotal:    %4d  GndTotal: %4d  OtherTotal: %4d\n", gAveAirProcTime, gAveGndProcTime, gAveOtherProcTime);
		float AveAir = 0.0F, AveGnd = 0.0F, AveOther = 0.0F;
		if(numAircraft)
			AveAir = gAveAirProcTime /(float) numAircraft;
		if(numGrndVeh)
		{
			AveGnd = gAveGndProcTime /(float) numGrndVeh;
			ACMIAve = gACMI/(float) numGrndVeh;
			SpecCaseAve = gSpecCase/(float) numGrndVeh;
			TurrAve = gTurret/(float) numGrndVeh;
			LODAve = gLOD/(float) numGrndVeh;
			FireAve = gFire/(float) numGrndVeh;
			MoveAve = gMove/(float) numGrndVeh;
			ThinkAve = gThink/(float) numGrndVeh;
			TargAve = gTarg/(float) numGrndVeh;
			RadarAve = gRadar/(float) numGrndVeh;
			FireContAve = gFireCont/(float) numGrndVeh;
			SimVehAve = gSimVeh/(float) numGrndVeh;
			SFXAve = gSFX/(float) numGrndVeh;

			CommAve = gCommChooseTarg/(float) numGrndVeh;
			SelNewAve = gSelNewTarg/(float) numGrndVeh;
			RadTargAve = gRadarTarg/(float) numGrndVeh;
			ConTargAve = gConfirmTarg/(float) numGrndVeh;
			DoWeapAve = gDoWeapons/(float) numGrndVeh;
			SelWeapAve = gSelWeapon/(float) numGrndVeh;

			TurrCalcAve = gTurrCalc/(float) numGrndVeh;
			RotTurrAve = gRotTurr/(float) numGrndVeh;
			KeepAlAve = gKeepAlive/(float) numGrndVeh;
			FireTimeAve = gFireTime/(float)numGrndVeh;
		}
		if(numOther)
			AveOther = gAveOtherProcTime /(float) numOther;
		if(numCalls)
			SelBestWeapAve = gSelBestWeap /(float) numCalls;
		if(numWRng)
			WeapRngAve = gWeapRng/(float)numWRng;
		if(numWHCh)
			WeapChAve = gWeapHCh/(float)numWHCh;
		if(numWeapScore)
			WeapScAve = gWeapScore/(float)numWeapScore;
		if(numCanShoot)
			CanShootAve = gCanShoot/(float)numCanShoot;

		MonoPrint("AirAve:      %4.2f  GndAve:   %4.2f  OtherAve:   %4.2f\n", AveAir, AveGnd, AveOther);
		MonoPrint("NumAir:      %4d  NumGnd:   %4d  NumOther:   %4d\n\n", numAircraft, numGrndVeh, numOther);
		MonoPrint("Cycle:       %4d  AllObj:   %4d  SoundFx:    %4d  SFX:        %4d  ATC: %4d\n",gWhole, AllObjTime, gLastSoundFxTime, gLastSFXTime, gLastATCProcTime);
		//MonoPrint("ACMIAve:     %4.2f  SCasAve:  %4.2f  TurrAve:    %4.2f  LODAve:      %4.2f\n", ACMIAve, SpecCaseAve, TurrAve, LODAve);
		//MonoPrint("MoveAve:     %4.2f  ThinkAve: %4.2f  TargAve:    %4.2f  RadarAve:    %4.2f\n", MoveAve, ThinkAve, TargAve, RadarAve);
		//MonoPrint("SimVehAve:   %4.2f  SFXAve:   %4.2f  FireAve:    %4.2f  FireContAve: %4.2f\n\n", SimVehAve, SFXAve, FireAve, FireContAve);
		//MonoPrint("CommAve:     %4.2f  SelNewAve:%4.2f  RadTargAve: %4.2f  SelBWeapAve: %4.2f\n", CommAve, SelNewAve, RadTargAve, SelBestWeapAve);
		//MonoPrint("Whole:       %4.2f  DoWeapAve:%4.2f  SelWepAve:  %4.2f\n", FireTimeAve, DoWeapAve, SelWeapAve);
		//MonoPrint("TurrCalcAve: %4.2f  RotTurAve:%4.2f  KeepAlAve:  %4.2f\n\n", TurrCalcAve, RotTurrAve, KeepAlAve);

		//MonoPrint("SelBWeapAve: %4.2f  WRngAve:  %4.2f  WeapChAve:  %4.2f  WeapScAve: %4.2f  CanShootAve: %4.2f\n\n", SelBestWeapAve, WeapRngAve, WeapChAve, WeapScAve, CanShootAve);
	}
#endif
}


void SimulationDriver::NoPause(void)
{
   SetTimeCompression(1);
   F4HearVoices();
   SimLibElapsedTime = vuxGameTime;
}


void SimulationDriver::TogglePause(void)
{
	if (targetCompressionRatio)
	{
		SetTimeCompression(0);
		F4SilenceVoices();
	}
	else
	{
		SetTimeCompression(1);
		F4HearVoices();
	}
	
	motionOn = 1;
	
	SimLibElapsedTime = vuxGameTime;
}


void SimulationDriver::SetPlayerEntity (SimBaseClass* newObject)
{
   if (playerEntity == newObject)
	   return;
   // JPO - part of the reason helicopters don't work... 
   ShiAssert(newObject == NULL || newObject->IsAirplane() || newObject->IsEject());
   if (newObject == NULL || newObject->IsAirplane() || newObject->IsEject())	// 2002-02-11 MODIFIED BY S.G. A player can also be a EjectedPilotClass
   {
       playerEntity = (AircraftClass*)newObject;
	   //MI reset our avionics
	   playerLastMasterMode = FireControlComputer::Nav;
	   playerLastSubMode = FireControlComputer::ETE;
   }


	///VWF HACK: The following is a hack to make the Tac Eng Instrument
	// Landing Mission agree with the Manual

	if(RunningTactical() && 
			current_tactical_mission &&
			current_tactical_mission->get_type() == tt_training &&
			!strcmpi(current_tactical_mission->get_title(), "10 Instrument Landing") &&
			gNavigationSys)
	{
		VU_ID ATCId;
		gNavigationSys->SetInstrumentMode(NavigationSystem::ILS_TACAN);
		if(gTacanList) {
			int range, type;
			float ilsf;

			gTacanList->GetVUIDFromChannel(101, TacanList::X, TacanList::AG, &ATCId, &range, &type, &ilsf);
		}
		if(ATCId != FalconNullId && playerEntity) {
			FalconATCMessage* atcMsg	= new FalconATCMessage (ATCId, FalconLocalGame);

			atcMsg->dataBlock.type	= 0;
			atcMsg->dataBlock.from	= playerEntity->Id();
			FalconSendMessage(atcMsg, FALSE);
		}
	}

   // If there is no player vehicle, turn off force feedback
   if (!playerEntity)
   {
      JoystickStopAllEffects();
   }
}


void SimulationDriver::UpdateIAStats (SimBaseClass* oldEntity)
{
   if ((oldEntity->IsSetFlag(MOTION_MSL_AI)) ||
       (oldEntity->IsSetFlag(MOTION_BMB_AI)))
   {
      //InstantAction.ExpendWeapons(oldEntity);
      return;
   }
   else if (!(oldEntity->IsSetFlag(MOTION_AIR_AI)) &&
       !(oldEntity->IsSetFlag(MOTION_GND_AI)) &&
       !(oldEntity->IsSetFlag(MOTION_HELO_AI)))
   {
      return;
   }

   //InstantAction.CountCoup(oldEntity);
}

void SimulationDriver::SetFrameDescription (int mSecPerFrame, int numMinorFrames)
{
   SimLibMinorPerMajor  = numMinorFrames;
   SimLibMajorFrameTime = (float)mSecPerFrame * 0.001F * numMinorFrames;
   SimLibMajorFrameRate = 1.0F / SimLibMajorFrameTime;
   SimLibMinorFrameTime = SimLibMajorFrameTime / (float)SimLibMinorPerMajor;
   SimLibMinorFrameRate = 1.0F / SimLibMinorFrameTime;
}

// =================================================
// KCK: Campaign Wake/Sleep functions
// These assume the entities have already been added
// during a deaggregate. these just make them sim-
// aware or sim-blind.
// =================================================

// KCK: This is called by the Campaign to wake a deaggregated objective/unit
// Basically, we do everything we need to to make these entities Sim-Ready/Aware.
// Add to lists, flights, create drawable object, etc.
void SimulationDriver::WakeCampaignFlight (int isUnit, CampBaseClass* baseEntity, TailInsertList *flightList)
{
	SimBaseClass* theObject = NULL;
	int	vehicles=0,last_to_add=0,woken=0;

	ShiAssert ( flightList );
	ShiAssert ( baseEntity );

	if (isUnit)
	{
		vehicles = ((Unit)baseEntity)->GetTotalVehicles();
		if (vehicles > 4) //PILOTS_PER_FLIGHT
		{
			int num;

			// edg: a better detail setting algorithm.  Minimum vehicles is 3.
			// use a quadratic function on slider percentage.
			// so, if the battalion is 48 units the slider settings will give:
			//	0 = 3
			//	1 = 4
			//	2 = 10
			//  3 = 20
			//  4 = 33
			//  5 = 48
			num = PlayerOptions.ObjectDeaggLevel();
			num *= num;
			last_to_add = 2 + vehicles * num / 10000;

//			MonoPrint ("Waking only %08x %d:%d\n", baseEntity, last_to_add, vehicles);
		}
		else
			last_to_add = vehicles;
	}

	// Add our list of objects.
	VuListIterator flit(flightList);
	theObject = (SimBaseClass*) flit.GetFirst();
	while (theObject)
	{
		
		// KCK: Decide not to wake some ground vehicles/features -
		if (isUnit)
		{
			// Wake vehicles by percentage
			if ((woken <= last_to_add) || (theObject->GetSlot() == ((Unit)baseEntity)->class_data->RadarVehicle))
			{
				WakeObject(theObject);
				woken++;
			}
		}
		else
		{
			// Wake features by detail level
			if (theObject->displayPriority <= PlayerOptions.BuildingDeaggLevel())
			{
				WakeObject(theObject);
				woken++;
			}
		}
		theObject->SetCampaignObject(baseEntity);
		theObject = (SimBaseClass*)flit.GetNext();
	}
}

// KCK: This is called from the Campaign.
// Sleep an entire sim flight
void SimulationDriver::SleepCampaignFlight (TailInsertList *flightList)
{
SimBaseClass*	theObject;
VuListIterator	flit(flightList);

	// Put all objects in this flight to sleep
	theObject = (SimBaseClass*)flit.GetFirst();
	ShiAssert(theObject == NULL || FALSE == F4IsBadReadPtr(theObject, sizeof *theObject));
	//while (theObject) // JB 010306 CTD
	while (theObject && !F4IsBadReadPtr(theObject, sizeof(SimBaseClass))) // JB 010306 CTD
	{
		theObject->Sleep();
		theObject = (SimBaseClass*)flit.GetNext();
	}
}

// ====================================================
// KCK: These are the general sim Wake/Sleep functions.
// They make the object/flight sim-aware/sim-blind.
// ====================================================

// This call makes the sim aware of this object
void SimulationDriver::WakeObject (SimBaseClass* theObject)
	{
	if (!theObject || theObject->IsAwake())
		return;

	// If it's a player only object, wait until a player actually attaches before allowing the wake.
	if (theObject->IsSetFalcFlag(FEC_PLAYERONLY))
		{
		GameManager.CheckPlayerStatus(theObject);
		if (!theObject->IsSetFalcFlag(FEC_HASPLAYERS))
			return;
		}

	theObject->Wake();
	}

// This call makes the sim ignore this object
void SimulationDriver::SleepObject (SimBaseClass* theObject)
	{
	if (!theObject || !theObject->IsAwake())
		return;

	theObject->Sleep();
	}

void SimulationDriver::UpdateRemoteData (void)
{
SimBaseClass* theObject;
VuListIterator updateWalker (objectList);

   theObject = (SimBaseClass*)updateWalker.GetFirst();
   while (theObject)
   {
      if (!theObject->IsLocal())
      {
         CalcTransformMatrix (theObject);
      	theObject->Exec();
         if (theObject->EntityDriver())
         {
            theObject->EntityDriver()->Exec(SimLibElapsedTime);
            //LRKLUDGE
            if (theObject->ZPos() > 100.0F)
            {
//               MonoPrint ("Clamping underground position for remote entity\n");
               theObject->SetPosition (theObject->XPos(), theObject->YPos(), 100.0F);
            }

            //LRKLUDGE
            if (theObject->ZPos() < -200000.0F)
            {
//               MonoPrint ("Clamping underground position for remote entity\n");
               theObject->SetPosition (theObject->XPos(), theObject->YPos(), -200000.0F);
            }
         }

         if (requestList && requestList->requestId == theObject->Id())
         {
         NewRequestList* tmpRequest;

            MonoPrint ("Clearing requested object %d\n", theObject->Id().num_);
            tmpRequest = requestList;
            requestList = requestList->next;
            delete tmpRequest;
         }
      }
      theObject = (SimBaseClass*)updateWalker.GetNext();
   }
}

void SimulationDriver::UpdateEntityLists (void)
{
}

SimBaseClass* SimulationDriver::FindNearestThreat (float* bearing, float* range, float* altitude)
{
SimBaseClass* retval = NULL;
SimBaseClass* theObject;
VuListIterator updateWalker (objectList);
float myX, myY, myZ;
float tmpRange;
Team myTeam;

   if (!playerEntity)
      return NULL;

   myX = ((SimBaseClass*)playerEntity)->XPos();
   myY = ((SimBaseClass*)playerEntity)->YPos();
   myZ = ((SimBaseClass*)playerEntity)->ZPos();
   myTeam = ((SimBaseClass*)playerEntity)->GetTeam();

   theObject = (SimBaseClass*)updateWalker.GetFirst();
   while (theObject)
   {
      if (	theObject->IsAirplane() && !theObject->IsDead() && !theObject->OnGround() && 
			GetTTRelations((Team)theObject->GetTeam(), myTeam) >= Hostile && theObject->GetCampaignObject()->GetSpotted(myTeam))
      {
		  if(	theObject->GetSType() == STYPE_AIR_FIGHTER ||
				theObject->GetSType() == STYPE_AIR_FIGHTER_BOMBER )
		  {
			 if (retval == NULL)
			 {
				*range = (theObject->XPos() - myX)*(theObject->XPos()-myX) +
				   (theObject->YPos() - myY)*(theObject->YPos()-myY);
				retval = theObject;
			 }
			 else
			 {
				tmpRange = (theObject->XPos() - myX)*(theObject->XPos()-myX) +
				   (theObject->YPos() - myY)*(theObject->YPos()-myY);

				if (tmpRange < *range)
				{
				   *range = tmpRange;
				   retval = theObject;
				}
			 }
		  }
      }
      theObject = (SimBaseClass*)updateWalker.GetNext();
   }

   if (retval)
   {
      *bearing = (float)atan2(retval->YPos() - myY, retval->XPos() - myX) * RTD;
      *range = (float)sqrt(*range);
      *altitude = -retval->ZPos();
   }

   return (retval);
}

SimBaseClass* SimulationDriver::FindNearestThreat (short *x, short *y, float* altitude)
{
SimBaseClass* retval = NULL;
SimBaseClass* theObject = NULL;
VuListIterator updateWalker (objectList);
float myX=0.0F, myY=0.0F, myZ=0.0F;
float tmpRange=0.0F,range=0.0F;
Team myTeam=0;

   if (!playerEntity)
      return NULL;

   myX = ((SimBaseClass*)playerEntity)->XPos();
   myY = ((SimBaseClass*)playerEntity)->YPos();
   myZ = ((SimBaseClass*)playerEntity)->ZPos();
   myTeam = ((SimBaseClass*)playerEntity)->GetTeam();

   theObject = (SimBaseClass*)updateWalker.GetFirst();
   while (theObject)
   {
      if (	theObject->IsAirplane() && !theObject->IsDead() && !theObject->OnGround() && 
			GetTTRelations((Team)theObject->GetTeam(), myTeam) >= Hostile && theObject->GetCampaignObject()->GetSpotted(myTeam))
      {
		  if(	theObject->GetSType() == STYPE_AIR_FIGHTER ||
				theObject->GetSType() == STYPE_AIR_FIGHTER_BOMBER )
		  {
			 if (retval == NULL)
			 {
				*x = SimToGrid(theObject->YPos());
				*y = SimToGrid(theObject->XPos());
				range = (theObject->XPos() - myX)*(theObject->XPos()-myX) +
				   (theObject->YPos() - myY)*(theObject->YPos()-myY);
				retval = theObject;
			 }
			 else
			 {
				tmpRange = (theObject->XPos() - myX)*(theObject->XPos()-myX) +
				   (theObject->YPos() - myY)*(theObject->YPos()-myY);

				if (tmpRange < range)
				{
					*x = SimToGrid(theObject->YPos());
					*y = SimToGrid(theObject->XPos());
				   range = tmpRange;
				   retval = theObject;
				}
			 }
		  }
      }
      theObject = (SimBaseClass*)updateWalker.GetNext();
   }

   if (retval)
   {
      *altitude = -retval->ZPos();
   }

   return (retval);
}

SimBaseClass* SimulationDriver::FindNearestThreat (AircraftClass* aircraft, short *x, short *y, float* altitude)
{
SimBaseClass* retval = NULL;
SimBaseClass* theObject = NULL;
VuListIterator updateWalker (objectList);
float myX=0.0F, myY=0.0F, myZ=0.0F;
float tmpRange=0.0F,range=0.0F;
Team myTeam=0;

   if (!playerEntity)
      return NULL;

   myX = aircraft->XPos();
   myY = aircraft->YPos();
   myZ = aircraft->ZPos();
   myTeam = aircraft->GetTeam();

   theObject = (SimBaseClass*)updateWalker.GetFirst();
   while (theObject)
   {
      if (	theObject->IsAirplane() && !theObject->IsDead() && !theObject->OnGround() && 
			GetTTRelations((Team)theObject->GetTeam(), myTeam) >= Hostile && theObject->GetCampaignObject()->GetSpotted(myTeam))
      {
		  if(	theObject->GetSType() == STYPE_AIR_FIGHTER ||
				theObject->GetSType() == STYPE_AIR_FIGHTER_BOMBER )
		  {
			 if (retval == NULL)
			 {
				*x = SimToGrid(theObject->YPos());
				*y = SimToGrid(theObject->XPos());
				range = (theObject->XPos() - myX)*(theObject->XPos()-myX) +
				   (theObject->YPos() - myY)*(theObject->YPos()-myY);
				retval = theObject;
			 }
			 else
			 {
				tmpRange = (theObject->XPos() - myX)*(theObject->XPos()-myX) +
				   (theObject->YPos() - myY)*(theObject->YPos()-myY);

				if (tmpRange < range)
				{
					*x = SimToGrid(theObject->YPos());
					*y = SimToGrid(theObject->XPos());
				   range = tmpRange;
				   retval = theObject;
				}
			 }
		  }
      }
      theObject = (SimBaseClass*)updateWalker.GetNext();
   }

   if (retval)
   {
      *altitude = -retval->ZPos();
   }

   return (retval);
}

SimBaseClass* SimulationDriver::FindNearestEnemyPlane (AircraftClass* aircraft, short *x, short *y, float* altitude)
{
SimBaseClass* retval = NULL;
SimBaseClass* theObject=NULL;
VuListIterator updateWalker (objectList);
float myX=0.0F, myY=0.0F, myZ=0.0F;
float tmpRange=0.0F,range=0.0F;
Team myTeam=0;

   if (!playerEntity)
      return NULL;

   myX = aircraft->XPos();
   myY = aircraft->YPos();
   myZ = aircraft->ZPos();
   myTeam = aircraft->GetTeam();

   theObject = (SimBaseClass*)updateWalker.GetFirst();
   while (theObject)
   {
      if (	theObject->IsAirplane() && !theObject->IsDead() && !theObject->OnGround() && 
			GetTTRelations((Team)theObject->GetTeam(), myTeam) >= Hostile && theObject->GetCampaignObject()->GetSpotted(myTeam))
      {
		 if (retval == NULL)
		 {
			*x = SimToGrid(theObject->YPos());
			*y = SimToGrid(theObject->XPos());
			range = (theObject->XPos() - myX)*(theObject->XPos()-myX) +
			   (theObject->YPos() - myY)*(theObject->YPos()-myY);
			retval = theObject;
		 }
		 else
		 {
			tmpRange = (theObject->XPos() - myX)*(theObject->XPos()-myX) +
			   (theObject->YPos() - myY)*(theObject->YPos()-myY);

			if (tmpRange < range)
			{
				*x = SimToGrid(theObject->YPos());
				*y = SimToGrid(theObject->XPos());
			   range = tmpRange;
			   retval = theObject;
			}
		 }
      }
      theObject = (SimBaseClass*)updateWalker.GetNext();
   }

   if (retval)
   {
      *altitude = -retval->ZPos();
   }

   return (retval);
}

CampBaseClass* SimulationDriver::FindNearestCampThreat (AircraftClass* aircraft, short *x, short *y, float* altitude)
{
CampBaseClass* retval = NULL;
CampBaseClass* theUnit=NULL;

float myX=0.0F, myY=0.0F, myZ=0.0F;
float tmpRange=0.0F,range=0.0F;
Team myTeam=0;

	if (!playerEntity)
		return NULL;

	// Don't look for campaign threats in dogfight. Everything is deaggregated anyway, and
	// this often finds people who havn't entered the sim yet.
	if (FalconLocalGame && FalconLocalGame->GetGameType() == game_Dogfight)
	   return NULL;

   myX = aircraft->XPos();
   myY = aircraft->YPos();
   myZ = aircraft->ZPos();
   myTeam = (Team)aircraft->GetTeam();

#ifndef VU_GRID_TREE_Y_MAJOR
	VuGridIterator	myit(RealUnitProxList,(BIG_SCALAR)myX,(BIG_SCALAR)myY,(BIG_SCALAR)GridToSim(100));
#else
	VuGridIterator	myit(RealUnitProxList,(BIG_SCALAR)myY,(BIG_SCALAR)myX,(BIG_SCALAR)GridToSim(100));
#endif

   theUnit = (CampBaseClass*)myit.GetFirst();
   while (theUnit)
   {
      if (theUnit->IsFlight() && !theUnit->IsDead() && GetTTRelations((Team)theUnit->GetTeam(), myTeam) >= Hostile && theUnit->GetSpotted(myTeam))
      {
		  if(	theUnit->GetSType() == STYPE_UNIT_FIGHTER ||
				theUnit->GetSType() == STYPE_UNIT_FIGHTER_BOMBER )
		  {
			 if (retval == NULL)
			 {
				*x = SimToGrid(theUnit->YPos());
				*y = SimToGrid(theUnit->XPos());
				range = (theUnit->XPos() - myX)*(theUnit->XPos()-myX) +
				   (theUnit->YPos() - myY)*(theUnit->YPos()-myY);
				retval = theUnit;
			 }
			 else
			 {
				tmpRange = (theUnit->XPos() - myX)*(theUnit->XPos()-myX) +
				   (theUnit->YPos() - myY)*(theUnit->YPos()-myY);

				if (tmpRange < range)
				{
					*x = SimToGrid(theUnit->YPos());
					*y = SimToGrid(theUnit->XPos());
				   range = tmpRange;
				   retval = theUnit;
				}
			 }
		  }
      }
      theUnit = (CampBaseClass*)myit.GetNext();
   }

   if (retval)
   {
      *altitude = -retval->ZPos();
   }

   return (retval);
}

CampBaseClass* SimulationDriver::FindNearestCampEnemy (AircraftClass* aircraft, short *x, short *y, float* altitude)
{
CampBaseClass* retval = NULL;
CampBaseClass* theUnit = NULL;

float myX=0.0F, myY=0.0F, myZ=0.0F;
float tmpRange=0.0F,range=0.0F;
Team myTeam=0;

	if (!playerEntity)
		return NULL;

	// Don't look for campaign threats in dogfight. Everything is deaggregated anyway, and
	// this often finds people who havn't entered the sim yet.
	if (FalconLocalGame && FalconLocalGame->GetGameType() == game_Dogfight)
	   return NULL;

   myX = aircraft->XPos();
   myY = aircraft->YPos();
   myZ = aircraft->ZPos();
   myTeam = aircraft->GetTeam();

#ifndef VU_GRID_TREE_Y_MAJOR
	VuGridIterator	myit(RealUnitProxList,(BIG_SCALAR)myX,(BIG_SCALAR)myY,(BIG_SCALAR)GridToSim(100));
#else
	VuGridIterator	myit(RealUnitProxList,(BIG_SCALAR)myY,(BIG_SCALAR)myX,(BIG_SCALAR)GridToSim(100));
#endif

   theUnit = (CampBaseClass*)myit.GetFirst();
   while (theUnit)
   {
      if (theUnit->IsFlight() && !theUnit->IsDead() && GetTTRelations((Team)theUnit->GetTeam(), myTeam) >= Hostile && theUnit->GetSpotted(myTeam))
      {
		 if (retval == NULL)
		 {
			*x = SimToGrid(theUnit->YPos());
			*y = SimToGrid(theUnit->XPos());
			range = (theUnit->XPos() - myX)*(theUnit->XPos()-myX) +
			   (theUnit->YPos() - myY)*(theUnit->YPos()-myY);
			retval = theUnit;
		 }
		 else
		 {
			tmpRange = (theUnit->XPos() - myX)*(theUnit->XPos()-myX) +
			   (theUnit->YPos() - myY)*(theUnit->YPos()-myY);

			if (tmpRange < range)
			{
				*x = SimToGrid(theUnit->YPos());
				*y = SimToGrid(theUnit->XPos());
			   range = tmpRange;
			   retval = theUnit;
			}
		 }
      }
      theUnit = (CampBaseClass*)myit.GetNext();
   }

   if (retval)
   {
      *altitude = -retval->ZPos();
   }

   return (retval);
}

/*
** Description:
**		This function is called from ACMI when recording is toggled to ON.
**		We need to walk the object lists and get initial states and stuff
**		for them and write them out to recording file.
*/
void SimulationDriver::InitACMIRecord (void)
{
	int i;
	int numSwitches;
	SimFeatureClass* theObject;
	SimBaseClass *leadObject;
	SimMoverClass *theMover;
	VuListIterator objectWalker (objectList);
	VuListIterator featureWalker (featureList);
	ACMIGenPositionRecord genPos;
	ACMIMissilePositionRecord misPos;
	ACMIAircraftPositionRecord airPos;
	ACMISwitchRecord switchRec;
	ACMIDOFRecord DOFRec;
	ACMITodOffsetRecord todRec;
	ACMIFeaturePositionRecord featPos;
	ACMIFeatureStatusRecord featStat;

	// record the tod offset
   	todRec.hdr.time = OTWDriver.todOffset;
	gACMIRec.TodOffsetRecord( &todRec );

	// make sure the player's f16 is first in list
	if ( playerEntity )
	{
		theMover = playerEntity;
		airPos.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
		airPos.data.type = theMover->Type();

		if (!F4IsBadReadPtr((DrawableBSP*)(theMover->drawPointer), sizeof(DrawableBSP)) && theMover->GetTeam() >= 0 && !F4IsBadReadPtr(TeamInfo[theMover->GetTeam()], sizeof(TeamClass))) // JB 010326 CTD
			airPos.data.uniqueID = ACMIIDTable->Add(theMover->Id(),(char*) ((DrawableBSP*)(theMover->drawPointer))->Label(),TeamInfo[theMover->GetTeam()]->GetColor());//.num_;

		airPos.data.x = theMover->XPos();
		airPos.data.y = theMover->YPos();
		airPos.data.z = theMover->ZPos();
		airPos.data.roll = theMover->Roll();
		airPos.data.pitch = theMover->Pitch();
		airPos.data.yaw = theMover->Yaw();
		RadarClass *radar = (RadarClass*)FindSensor( theMover, SensorClass::Radar );
		if (  radar && radar->RemoteBuggedTarget)//me123 add record for online targets
		   airPos.RadarTarget = ACMIIDTable->Add(radar->RemoteBuggedTarget->Id(),NULL,0);//.num_;
		else	if (  radar && radar->CurrentTarget() )
			airPos.RadarTarget = ACMIIDTable->Add(radar->CurrentTarget()->BaseData()->Id(),NULL,0);//.num_;
		else
			airPos.RadarTarget = -1;
		gACMIRec.AircraftPositionRecord( &airPos );

		// now we need to save the switches
		numSwitches = theMover->GetNumSwitches();
		for ( i = 0; i < numSwitches; i++ )
		{
			switchRec.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
			switchRec.data.type = theMover->Type();
			switchRec.data.uniqueID = ACMIIDTable->Add(theMover->Id(),NULL,0);//.num_;
			switchRec.data.switchNum = i;
			switchRec.data.switchVal = 
			switchRec.data.prevSwitchVal = theMover->GetSwitch( i );
			gACMIRec.SwitchRecord( &switchRec );
		}

		// now we need to save the dofs
		numSwitches = theMover->GetNumDOFs();
		for ( i = 0; i < numSwitches; i++ )
		{
			DOFRec.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
			DOFRec.data.type = theMover->Type();
			DOFRec.data.uniqueID = ACMIIDTable->Add(theMover->Id(),NULL,0);//.num_;
			DOFRec.data.DOFNum = i;
			DOFRec.data.DOFVal =
			DOFRec.data.prevDOFVal = theMover->GetDOFValue( i );
			gACMIRec.DOFRecord( &DOFRec );
		}
	}

    // Do Each Entity (movers)
    theMover = (SimMoverClass*)objectWalker.GetFirst();
    while (theMover)
    {
		// we've already done the player's f16
		if ( theMover == playerEntity )
		{
      		theMover = (SimMoverClass*)objectWalker.GetNext();
			continue;
		}

		//
		// type of ACMI rec we write depends on type of object....
		//


		// missile
   		if ( theMover->IsMissile() )
		{
		   	misPos.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
		  	misPos.data.type = theMover->Type();
			misPos.data.uniqueID = ACMIIDTable->Add(theMover->Id(),NULL,TeamInfo[theMover->GetTeam()]->GetColor());//.num_;
			misPos.data.x = theMover->XPos();
			misPos.data.y = theMover->YPos();
			misPos.data.z = theMover->ZPos();
			misPos.data.roll = theMover->Roll();
			misPos.data.pitch = theMover->Pitch();
			misPos.data.yaw = theMover->Yaw();
// remove			strcpy(misPos.data.label,"");
// remove			misPos.data.teamColor = TeamInfo[theMover->GetTeam()]->GetColor();
			gACMIRec.MissilePositionRecord( &misPos );
		}
		// bombs
   		else if ( theMover->IsBomb() )
		{
		   	genPos.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
		  	genPos.data.type = theMover->Type();
			genPos.data.uniqueID = ACMIIDTable->Add(theMover->Id(),NULL,TeamInfo[theMover->GetTeam()]->GetColor());//.num_;
			genPos.data.x = theMover->XPos();
			genPos.data.y = theMover->YPos();
			genPos.data.z = theMover->ZPos();
			genPos.data.roll = theMover->Roll();
			genPos.data.pitch = theMover->Pitch();
			genPos.data.yaw = theMover->Yaw();
// remove			strcpy(genPos.data.label,"");// = NULL;
//VWF
// remove			genPos.data.teamColor = TeamInfo[theMover->GetTeam()]->GetColor();

			if (((BombClass *)theMover)->IsSetBombFlag(BombClass::IsFlare))
		  		gACMIRec.FlarePositionRecord( (ACMIFlarePositionRecord *)&genPos );
			else if (((BombClass *)theMover)->IsSetBombFlag(BombClass::IsChaff))
		  		gACMIRec.ChaffPositionRecord( (ACMIChaffPositionRecord *)&genPos );
		  	else
		  		gACMIRec.GenPositionRecord( &genPos );
		}
		// aircraft
		// TODO: anything special for ownship?
		// TODO: save DOF's and states?
   		else if ( theMover->IsAirplane() )
		{
		   	airPos.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
		  	airPos.data.type = theMover->Type();
			airPos.data.uniqueID = ACMIIDTable->Add(theMover->Id(), (char *)((DrawableBSP*)(theMover->drawPointer))->Label(),TeamInfo[theMover->GetTeam()]->GetColor());//.num_;
// remove			_mbsnbcpy((unsigned char*)airPos.data.label, (unsigned char *)((DrawableBSP*)(theMover->drawPointer))->Label(), ACMI_LABEL_LEN - 1);
// remove			airPos.data.teamColor = TeamInfo[theMover->GetTeam()]->GetColor();
			airPos.data.x = theMover->XPos();
			airPos.data.y = theMover->YPos();
			airPos.data.z = theMover->ZPos();
			airPos.data.roll = theMover->Roll();
			airPos.data.pitch = theMover->Pitch();
				airPos.data.yaw = theMover->Yaw();
			RadarClass *radar = (RadarClass*)FindSensor( theMover, SensorClass::Radar );
			if (  radar && radar->RemoteBuggedTarget)//me123 add record for online targets
			   airPos.RadarTarget = ACMIIDTable->Add(radar->RemoteBuggedTarget->Id(),NULL,0);//.num_;
			else if (  radar && radar->CurrentTarget() )
			   airPos.RadarTarget = ACMIIDTable->Add(radar->CurrentTarget()->BaseData()->Id(),NULL,0);//.num_;
			else
				airPos.RadarTarget = -1;
			gACMIRec.AircraftPositionRecord( &airPos );

			// now we need to save the switches
			numSwitches = theMover->GetNumSwitches();
			for ( i = 0; i < numSwitches; i++ )
			{
				switchRec.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
				switchRec.data.type = theMover->Type();
				switchRec.data.uniqueID = ACMIIDTable->Add(theMover->Id(),NULL,0);//.num_;
				switchRec.data.switchNum = i;
				switchRec.data.switchVal = 
				switchRec.data.prevSwitchVal = theMover->GetSwitch( i );
				gACMIRec.SwitchRecord( &switchRec );
			}

			// now we need to save the dofs
			numSwitches = theMover->GetNumDOFs();
			for ( i = 0; i < numSwitches; i++ )
			{
				DOFRec.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
				DOFRec.data.type = theMover->Type();
				DOFRec.data.uniqueID = ACMIIDTable->Add(theMover->Id(),NULL,0);//.num_;
				DOFRec.data.DOFNum = i;
				DOFRec.data.DOFVal =
				DOFRec.data.prevDOFVal = theMover->GetDOFValue( i );
				gACMIRec.DOFRecord( &DOFRec );
			}
		}

		// everything else (helos, ground vehicles...)
		else
		{
		   	genPos.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
		  	genPos.data.type = theMover->Type();
			genPos.data.uniqueID = ACMIIDTable->Add(theMover->Id(),NULL,TeamInfo[theMover->GetTeam()]->GetColor());//.num_;
			genPos.data.x = theMover->XPos();
			genPos.data.y = theMover->YPos();
			genPos.data.z = theMover->ZPos();
			genPos.data.roll = theMover->Roll();
			genPos.data.pitch = theMover->Pitch();
			genPos.data.yaw = theMover->Yaw();
// remove			genPos.data.teamColor = TeamInfo[theMover->GetTeam()]->GetColor();
			gACMIRec.GenPositionRecord( &genPos );
		}

		// next one in the loop
      	theMover = (SimMoverClass*)objectWalker.GetNext();
    }

    // Do Each feature 
    theObject = (SimFeatureClass*)featureWalker.GetFirst();
    while (theObject)
    {
		featPos.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
		featPos.data.type = theObject->Type();
		featPos.data.uniqueID = ACMIIDTable->Add(theObject->Id(),NULL,0);//.num_;
		featPos.data.x = theObject->XPos();
		featPos.data.y = theObject->YPos();
		featPos.data.z = theObject->ZPos();
		featPos.data.roll = theObject->Roll();
		featPos.data.pitch = theObject->Pitch();
		featPos.data.yaw = theObject->Yaw();
		featPos.data.slot = theObject->GetSlot();
		featPos.data.specialFlags = theObject->featureFlags;
		leadObject = theObject->GetCampaignObject()->GetComponentLead();
		if ( leadObject && leadObject->Id().num_ != theObject->Id().num_ )
			featPos.data.leadUniqueID = ACMIIDTable->Add(leadObject->Id(),NULL,0);//.num_;
		else
		   	featPos.data.leadUniqueID = -1;
		gACMIRec.FeaturePositionRecord( &featPos );

		// TODO: we'll probably need to write out what state it's in
		// once ACMI supports state change events
		featStat.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
		featStat.data.uniqueID = ACMIIDTable->Add(theObject->Id(),NULL,0);//.num_;
		featStat.data.newStatus = (theObject->Status() & VIS_TYPE_MASK);
		featStat.data.prevStatus = (theObject->Status() & VIS_TYPE_MASK);
		gACMIRec.FeatureStatusRecord( &featStat );

		// next one in the loop
      	theObject = (SimFeatureClass*)featureWalker.GetNext();
    }

}

#if 0
void ProximityCheck (SimBaseClass* thisObj)
{
VuEntity* theObject;

   theFlight = SimFlight::RootSimFlight;
	while (theFlight)
	{
   	if (theFlight->campaignType == SimFlight::CampaignInstantAction)
      {
      VuListIterator updateWalker (theFlight->elements);
      int isClose = FALSE;

         theObject = updateWalker.GetFirst();
         while (theObject)
         {
            if (fabs(thisObj->XPos() - theObject->XPos()) < 20.0F * NM_TO_FT &&
                fabs(thisObj->YPos() - theObject->YPos()) < 20.0F * NM_TO_FT)
		      {
			      isClose = TRUE;
               break;
		      }
            theObject = updateWalker.GetNext();
         }

         if (!isClose)
         {
            theObject = updateWalker.GetFirst();
            while (theObject)
            {
               ((SimBaseClass*)theObject)->Sleep();
               theObject = updateWalker.GetNext();
            }
         }
      }
      theFlight = theFlight->next;
   }
}
#endif


void SimulationDriver::POVKludgeFunction(DWORD povHatAngle) { // VWF POV Kludge 11/24/97, Yeah its a mess, remove after demo
	
	//static DWORD previousAngle = -1;

	//if((povHatAngle == -1 && previousAngle != -1) ||
	//	(povHatAngle != -1 && previousAngle == -1)) {

		if(	OTWDriver.GetOTWDisplayMode() == OTWDriverClass::Mode3DCockpit	 || 
			OTWDriver.GetOTWDisplayMode() == OTWDriverClass::ModePadlockF3	 || // 2002-02-17 MODIFIED BY S.G. Needed now for the 'break lock by POV' to work
			OTWDriver.GetOTWDisplayMode() == OTWDriverClass::ModePadlockEFOV || // 2002-02-17 MODIFIED BY S.G. Needed now for the 'break lock by POV' to work
			OTWDriver.GetOTWDisplayMode() == OTWDriverClass::ModeOrbit		 ||
			OTWDriver.GetOTWDisplayMode() == OTWDriverClass::ModeChase		 ||
			OTWDriver.GetOTWDisplayMode() == OTWDriverClass::ModeSatellite) {
/*
			if(povHatAngle == -1) {
				OTWDriver.ViewTiltHold();
				OTWDriver.ViewSpinHold();
			}
			else if((povHatAngle > POV_NW && povHatAngle < 36000) || povHatAngle < POV_NE) {
				OTWViewUp(0, KEY_DOWN, NULL);
			}
			else if(povHatAngle > POV_NE && povHatAngle < POV_SE) {
				OTWViewRight(0, KEY_DOWN, NULL);
			}
			else if(povHatAngle > POV_SE && povHatAngle < POV_SW) {
				OTWViewDown(0, KEY_DOWN, NULL);
			}
			else if(povHatAngle > POV_SW && povHatAngle < POV_NW) {
				OTWViewLeft(0, KEY_DOWN, NULL);
			}*/

			if(povHatAngle == -1) {
				OTWDriver.ViewTiltHold();
				OTWDriver.ViewSpinHold();
			}
			else if(((povHatAngle >= (POV_NW + POV_HALF_RANGE)) && (povHatAngle < 36000)) || (povHatAngle < POV_N + POV_HALF_RANGE)) {
				OTWViewUp(0, KEY_DOWN, NULL);
			}
			else if((povHatAngle >= (POV_NE - POV_HALF_RANGE)) && (povHatAngle < POV_NE + POV_HALF_RANGE)) {
				OTWViewRight(0, KEY_DOWN, NULL);
				OTWViewUp(0, KEY_DOWN, NULL);
			}
			else if((povHatAngle >= (POV_E - POV_HALF_RANGE)) && (povHatAngle < POV_E + POV_HALF_RANGE)) {
				OTWViewRight(0, KEY_DOWN, NULL);
			}
			else if((povHatAngle >= (POV_SE - POV_HALF_RANGE)) && (povHatAngle < POV_SE + POV_HALF_RANGE)) {
				OTWViewRight(0, KEY_DOWN, NULL);
				OTWViewDown(0, KEY_DOWN, NULL);
			}
			else if((povHatAngle >= (POV_S - POV_HALF_RANGE)) && (povHatAngle < POV_S + POV_HALF_RANGE)) {
				OTWViewDown(0, KEY_DOWN, NULL);
			}
			else if((povHatAngle >= (POV_SW - POV_HALF_RANGE)) && (povHatAngle < POV_SW + POV_HALF_RANGE)) {
				OTWViewDown(0, KEY_DOWN, NULL);
				OTWViewLeft(0, KEY_DOWN, NULL);
			}
			else if((povHatAngle >= (POV_W - POV_HALF_RANGE)) && (povHatAngle < POV_W + POV_HALF_RANGE)) {
				OTWViewLeft(0, KEY_DOWN, NULL);
			}
			else if((povHatAngle >= (POV_NW - POV_HALF_RANGE)) && (povHatAngle < POV_NW + POV_HALF_RANGE)) {
				OTWViewLeft(0, KEY_DOWN, NULL);
				OTWViewUp(0, KEY_DOWN, NULL);
			}
		}
		else if(OTWDriver.GetOTWDisplayMode() == OTWDriverClass::Mode2DCockpit && povHatAngle != -1) {

			if(((povHatAngle >= (POV_NW + POV_HALF_RANGE)) && (povHatAngle < 36000)) || (povHatAngle < POV_N + POV_HALF_RANGE)) {
				gSelectedCursor = OTWDriver.pCockpitManager->POVDispatch(POV_N, gxPos, gyPos);
			}
			else if((povHatAngle >= (POV_NE - POV_HALF_RANGE)) && (povHatAngle < POV_NE + POV_HALF_RANGE)) {
				gSelectedCursor = OTWDriver.pCockpitManager->POVDispatch(POV_NE, gxPos, gyPos);
			}
			else if((povHatAngle >= (POV_E - POV_HALF_RANGE)) && (povHatAngle < POV_E + POV_HALF_RANGE)) {
				gSelectedCursor = OTWDriver.pCockpitManager->POVDispatch(POV_E, gxPos, gyPos);
			}
			else if((povHatAngle >= (POV_SE - POV_HALF_RANGE)) && (povHatAngle < POV_SE + POV_HALF_RANGE)) {
				gSelectedCursor = OTWDriver.pCockpitManager->POVDispatch(POV_SE, gxPos, gyPos);
			}
			else if((povHatAngle >= (POV_S - POV_HALF_RANGE)) && (povHatAngle < POV_S + POV_HALF_RANGE)) {
				gSelectedCursor = OTWDriver.pCockpitManager->POVDispatch(POV_S, gxPos, gyPos);
			}
			else if((povHatAngle >= (POV_SW - POV_HALF_RANGE)) && (povHatAngle < POV_SW + POV_HALF_RANGE)) {
				gSelectedCursor = OTWDriver.pCockpitManager->POVDispatch(POV_SW, gxPos, gyPos);
			}
			else if((povHatAngle >= (POV_W - POV_HALF_RANGE)) && (povHatAngle < POV_W + POV_HALF_RANGE)) {
				gSelectedCursor = OTWDriver.pCockpitManager->POVDispatch(POV_W, gxPos, gyPos);
			}
			else if((povHatAngle >= (POV_NW - POV_HALF_RANGE)) && (povHatAngle < POV_NW + POV_HALF_RANGE)) {
				gSelectedCursor = OTWDriver.pCockpitManager->POVDispatch(POV_NW, gxPos, gyPos);
			}

/*	
			if(((povHatAngle >= (POV_NW + POV_HALF_RANGE)) && (povHatAngle < 65535)) || (povHatAngle < POV_N + POV_HALF_RANGE)) {
				gSelectedCursor = OTWDriver.pCockpitManager->Dispatch(CP_MOUSE_BUTTON0, DisplayOptions.DispWidth / 2, 0);
			}
			else if((povHatAngle >= (POV_NE - POV_HALF_RANGE)) && (povHatAngle < POV_NE + POV_HALF_RANGE)) {
				gSelectedCursor = OTWDriver.pCockpitManager->Dispatch(CP_MOUSE_BUTTON0, DisplayOptions.DispWidth, 0);
			}
			else if((povHatAngle >= (POV_E - POV_HALF_RANGE)) && (povHatAngle < POV_E + POV_HALF_RANGE)) {
				gSelectedCursor = OTWDriver.pCockpitManager->Dispatch(CP_MOUSE_BUTTON0, DisplayOptions.DispWidth, DisplayOptions.DispHeight / 2);
			}
			else if((povHatAngle >= (POV_SE - POV_HALF_RANGE)) && (povHatAngle < POV_SE + POV_HALF_RANGE)) {
				gSelectedCursor = OTWDriver.pCockpitManager->Dispatch(CP_MOUSE_BUTTON0, DisplayOptions.DispWidth, DisplayOptions.DispHeight);
			}
			else if((povHatAngle >= (POV_S - POV_HALF_RANGE)) && (povHatAngle < POV_S + POV_HALF_RANGE)) {
				gSelectedCursor = OTWDriver.pCockpitManager->Dispatch(CP_MOUSE_BUTTON0, DisplayOptions.DispWidth / 2 + 2, DisplayOptions.DispHeight);
			}
			else if((povHatAngle >= (POV_SW - POV_HALF_RANGE)) && (povHatAngle < POV_SW + POV_HALF_RANGE)) {
				gSelectedCursor = OTWDriver.pCockpitManager->Dispatch(CP_MOUSE_BUTTON0, 0, DisplayOptions.DispHeight);
			}
			else if((povHatAngle >= (POV_W - POV_HALF_RANGE)) && (povHatAngle < POV_W + POV_HALF_RANGE)) {
				gSelectedCursor = OTWDriver.pCockpitManager->Dispatch(CP_MOUSE_BUTTON0, 0, DisplayOptions.DispHeight / 2);
			}
			else if((povHatAngle >= (POV_NW - POV_HALF_RANGE)) && (povHatAngle < POV_NW + POV_HALF_RANGE)) {
				gSelectedCursor = OTWDriver.pCockpitManager->Dispatch(CP_MOUSE_BUTTON0, 0, 0);
			}
*/
		}
	//}
	
	//previousAngle = povHatAngle;
}

void SimulationDriver::AddToObjectList (VuEntity* theObject)
{
   objectList->ForcedInsert(theObject);
   combinedList->ForcedInsert(theObject);
}

void SimulationDriver::AddToFeatureList (VuEntity* theObject)
{
   featureList->ForcedInsert(theObject);
   combinedFeatureList->ForcedInsert(theObject);
}

void SimulationDriver::AddToCampUnitList (VuEntity* theObject)
{
   campUnitList->ForcedInsert(theObject);
   combinedList->ForcedInsert(theObject);
   // MonoPrint ("Adding %d\n", theObject->Id().num_);
}

void SimulationDriver::AddToCampFeatList (VuEntity* theObject)
{
   campObjList->ForcedInsert(theObject);
   combinedFeatureList->ForcedInsert(theObject);
}

void SimulationDriver::AddToCombUnitList (VuEntity* theObject)
{
   combinedList->ForcedInsert(theObject);
}

void SimulationDriver::AddToCombFeatList (VuEntity* theObject)
{
   combinedFeatureList->ForcedInsert(theObject);
}


void SimulationDriver::RemoveFromFeatureList (VuEntity* theObject)
{
   featureList->Remove(theObject);
   combinedFeatureList->Remove(theObject);
}

void SimulationDriver::RemoveFromObjectList (VuEntity* theObject)
{
   objectList->Remove(theObject);
   combinedList->Remove(theObject);
}

void SimulationDriver::RemoveFromCampUnitList (VuEntity* theObject)
{
   campUnitList->Remove(theObject);
   combinedList->Remove(theObject);
   // MonoPrint ("Removeing %d\n", theObject->Id().num_);
}

void SimulationDriver::RemoveFromCampFeatList (VuEntity* theObject)
{
   campObjList->Remove(theObject);
   combinedFeatureList->Remove(theObject);
}

void SimulationDriver::InitializeSimMemoryPools (void)
{
   MonoPrint( "Initializing Sim Memory Pools....\n" );

	#ifdef CHECK_LEAKAGE
	dbgMemSetCheckpoint( 3 );
	// dbgMemSetSafetyLevel( MEM_SAFETY_DEBUG );
	#endif

   // Allocate Memory Pools
   #ifdef USE_SH_POOLS
   AirframeClass::InitializeStorage();
   AircraftClass::InitializeStorage();
   MissileClass::InitializeStorage();
   MissileInFlightData::InitializeStorage();
   BombClass::InitializeStorage();
   ChaffClass::InitializeStorage();
   FlareClass::InitializeStorage();
   DebrisClass::InitializeStorage();
   SimObjectType::InitializeStorage();
   SimObjectLocalData::InitializeStorage();
   SfxClass::InitializeStorage();
   GunClass::InitializeStorage();
   GroundClass::InitializeStorage();
   GNDAIClass::InitializeStorage();
   SimFeatureClass::InitializeStorage();

   Drawable2D::InitializeStorage();
   DrawableTracer::InitializeStorage();
   DrawableGroundVehicle::InitializeStorage();
   DrawableBuilding::InitializeStorage();
   DrawableBSP::InitializeStorage();
   DrawableShadowed::InitializeStorage();
   DrawableBridge::InitializeStorage();
   DrawableOvercast::InitializeStorage();

   // Don't need this: Scott has done some evil #define overloading DrawableOvercast
   //DrawableOvercast2::InitializeStorage();

   DrawablePlatform::InitializeStorage();
   DrawableRoadbed::InitializeStorage();
   DrawableTrail::InitializeStorage();
   TrailElement::InitializeStorage();
   DrawablePuff::InitializeStorage();
   DrawablePoint::InitializeStorage();
   DrawableGuys::InitializeStorage();

   displayList::InitializeStorage();
   drawPtrList::InitializeStorage();
   sfxRequest::InitializeStorage();

   WayPointClass::InitializeStorage();
   SMSBaseClass::InitializeStorage();
   SMSClass::InitializeStorage();
   BasicWeaponStation::InitializeStorage();
   AdvancedWeaponStation::InitializeStorage();
   DigitalBrain::InitializeStorage();
   HelicopterClass::InitializeStorage();
   HeliBrain::InitializeStorage();
   SimVuDriver::InitializeStorage();
   SimVuSlave::InitializeStorage();

   ObjectGeometry::InitializeStorage();

   ListElementClass::InitializeStorage();
   ListClass::InitializeStorage();

   UnitDeaggregationData::InitializeStorage();
   MissionRequestClass::InitializeStorage();
   DivisionClass::InitializeStorage();
   EventElement::InitializeStorage();
   ATCBrain::InitializeStorage();

   gFaultMemPool = MemPoolInit( 0 );
   gTextMemPool = MemPoolInit( 0 );
   gObjMemPool = MemPoolInit( 0 );
   gReadInMemPool = MemPoolInit( 0 );
   gCockMemPool = MemPoolInit( 0 );
   gTacanMemPool = MemPoolInit( 0 );
   #endif

	// OW
   GraphicsDataPoolInitializeStorage();
}

void SimulationDriver::ReleaseSimMemoryPools (void)
{
   MonoPrint( "Releasing Sim Memory Pools....\n" );

	#ifdef CHECK_LEAKAGE
	dbgMemSetCheckpoint( 4 );
	if ( lastErrorFn )
	{
		dbgMemReportLeakage( MemDefaultPool, 3, 4  );
     	MemSetErrorHandler(lastErrorFn);
		dbgMemSetDefaultErrorOutput( DBGMEM_OUTPUT_FILE, "simleak.txt" );
		dbgMemReportLeakage( MemDefaultPool, 3, 4  );
     	MemSetErrorHandler(errPrint);
	}
	else
	{
		dbgMemSetDefaultErrorOutput( DBGMEM_OUTPUT_FILE, "simleak.txt" );
		dbgMemReportLeakage( MemDefaultPool, 3, 4  );
	}
	#endif

   // Free Memory Pools
   #ifdef USE_SH_POOLS
   AirframeClass::ReleaseStorage();
   AircraftClass::ReleaseStorage();
   DigitalBrain::ReleaseStorage();
   MissileClass::ReleaseStorage();
   MissileInFlightData::ReleaseStorage();
   BombClass::ReleaseStorage();
   ChaffClass::ReleaseStorage();
   FlareClass::ReleaseStorage();
   DebrisClass::ReleaseStorage();
   SimObjectType::ReleaseStorage();
   SimObjectLocalData::ReleaseStorage();
   SfxClass::ReleaseStorage();
   GunClass::ReleaseStorage();
   GroundClass::ReleaseStorage();
   GNDAIClass::ReleaseStorage();
   SimFeatureClass::ReleaseStorage();

   Drawable2D::ReleaseStorage();
   DrawableTracer::ReleaseStorage();
   DrawableGroundVehicle::ReleaseStorage();
   DrawableBuilding::ReleaseStorage();
   DrawableBSP::ReleaseStorage();
   DrawableShadowed::ReleaseStorage();
   DrawableBridge::ReleaseStorage();
   DrawableOvercast::ReleaseStorage();

   // Don't need this: Scott has done some evil #define overloading DrawableOvercast
   //DrawableOvercast2::ReleaseStorage();
   DrawablePlatform::ReleaseStorage();
   DrawableRoadbed::ReleaseStorage();
   DrawableTrail::ReleaseStorage();
   TrailElement::ReleaseStorage();
   DrawablePuff::ReleaseStorage();
   DrawablePoint::ReleaseStorage();
   DrawableGuys::ReleaseStorage();

   displayList::ReleaseStorage();
   drawPtrList::ReleaseStorage();
   sfxRequest::ReleaseStorage();

   WayPointClass::ReleaseStorage();
   SMSBaseClass::ReleaseStorage();
   SMSClass::ReleaseStorage();
   BasicWeaponStation::ReleaseStorage();
   AdvancedWeaponStation::ReleaseStorage();

   HelicopterClass::ReleaseStorage();
   HeliBrain::ReleaseStorage();

   SimVuDriver::ReleaseStorage();
   SimVuSlave::ReleaseStorage();

   ObjectGeometry::ReleaseStorage();

   ListElementClass::ReleaseStorage();
   ListClass::ReleaseStorage();

   UnitDeaggregationData::ReleaseStorage();
   MissionRequestClass::ReleaseStorage();
   DivisionClass::ReleaseStorage();

   EventElement::ReleaseStorage();
   ATCBrain::ReleaseStorage();

   MemPoolFree( gFaultMemPool );
   MemPoolFree( gTextMemPool );
   MemPoolFree( gObjMemPool );
   MemPoolFree( gReadInMemPool );
   MemPoolFree( gCockMemPool );
   MemPoolFree( gTacanMemPool );
   #endif

	// OW
   GraphicsDataPoolReleaseStorage();
}


// called by graphics shutdown (OTWDriver.Exit()).  We may want to do
// this more inteligently.....
//extern MEM_POOL glMemPool;
void SimulationDriver::ShrinkSimMemoryPools (void)
{
   #if 0
   // Free Memory Pools
   #ifdef USE_SH_POOLS
   i += MemPoolShrink( MemDefaultPool );
// i += MemPoolShrink( glMemPool );
   i += MemPoolShrink( AirframeClass::pool );
   i += MemPoolShrink( AircraftClass::pool );
   i += MemPoolShrink( MissileClass::pool );
   i += MemPoolShrink( BombClass::pool );
   i += MemPoolShrink( SimObjectType::pool );
   i += MemPoolShrink( SimObjectLocalData::pool );
   i += MemPoolShrink( SfxClass::pool );
   i += MemPoolShrink( GunClass::pool );
   i += MemPoolShrink( GroundClass::pool );
   i += MemPoolShrink( GNDAIClass::pool );
   i += MemPoolShrink( SimFeatureClass::pool );

   i += MemPoolShrink( Drawable2D::pool );
   i += MemPoolShrink( DrawableTracer::pool );
   i += MemPoolShrink( DrawableGroundVehicle::pool );
   i += MemPoolShrink( DrawableBuilding::pool );
   i += MemPoolShrink( DrawableBSP::pool );
   i += MemPoolShrink( DrawableShadowed::pool );
   i += MemPoolShrink( DrawableBridge::pool );
   i += MemPoolShrink( DrawableOvercast::pool );

   i += MemPoolShrink( DrawablePlatform::pool );
   i += MemPoolShrink( DrawableRoadbed::pool );
   i += MemPoolShrink( DrawableTrail::pool );
   i += MemPoolShrink( TrailElement::pool );
   i += MemPoolShrink( DrawablePuff::pool );
   i += MemPoolShrink( DrawablePoint::pool );
   i += MemPoolShrink( DrawableGuys::pool );

   i += MemPoolShrink( displayList::pool );
   i += MemPoolShrink( drawPtrList::pool );
   i += MemPoolShrink( sfxRequest::pool );

   i += MemPoolShrink( WayPointClass::pool );
   i += MemPoolShrink( SMSBaseClass::pool );
   i += MemPoolShrink( SMSClass::pool );
   i += MemPoolShrink( BasicWeaponStation::pool );
   i += MemPoolShrink( AdvancedWeaponStation::pool );

   MonoPrint( "Sim Memory Pools shrunk by %d bytes\n", i );
   #endif
   #endif
}
