#ifndef _SIMDRIVE_H
#define _SIMDRIVE_H

#ifndef _WINDOWS_
#include <windows.h>
#endif
#include "f4vu.h"
#include "SimLoop.h"
#include "FalcSess.h"


#define MAX_IA_CAMP_UNIT 0x10000

class SimBaseClass;
class SimInitDataClass;
class FalconSessionEntity;
class CampBaseClass;
class TailInsertList;
class FalconPrivateOrderedList;
class FalconPrivateList;
class SimMoverClass;
class CampBaseClass;
class AircraftClass;

class SimulationDriver
{
  public:
	SimulationDriver(void);
	~SimulationDriver(void);

	void Startup(void);		// One time setup (at application start)
	void Cleanup (void);	// One time shutdown (at application exit)

	void Exec (void);		// The master thread loop -- runs from Startup to Cleanup
	void Cycle (void);		// One SIM cycle (could be multiple time steps)

	void Enter (void);		// Enter the SIM from the UI
	void Exit (void);		// Set up sim for exiting

  public:
	  int InSim (void)						{ return SimulationLoopControl::InSim();}
	int RunningInstantAction (void)			{ return FalconLocalGame->GetGameType() == game_InstantAction;}
	int RunningDogfight (void)				{ return FalconLocalGame->GetGameType() == game_Dogfight;}
	int RunningTactical (void)				{ return FalconLocalGame->GetGameType() == game_TacticalEngagement;}
	int RunningCampaign (void)				{ return FalconLocalGame->GetGameType() == game_Campaign;}
	int RunningCampaignOrTactical (void)	{ return FalconLocalGame->GetGameType() == game_Campaign ||
													 FalconLocalGame->GetGameType() == game_TacticalEngagement;}

	void NoPause(void);	// Pause time in the sim
	void TogglePause(void);	// Pause time in the sim

	void SetFrameDescription (int mSecPerFrame, int numMinorFrames);
	void SetPlayerEntity (SimBaseClass* newObject);
	void UpdateIAStats (SimBaseClass*);
	SimBaseClass* FindNearestThreat (float*, float*, float*);
	SimBaseClass* FindNearestThreat (short *x, short *y, float *alt);
	SimBaseClass* FindNearestThreat (AircraftClass *aircraft, short *x, short *y, float *alt);
	SimBaseClass* FindNearestEnemyPlane (AircraftClass *aircraft, short *x, short *y, float *alt);	
	CampBaseClass* FindNearestCampThreat (AircraftClass *aircraft, short *x, short *y, float *alt);
	CampBaseClass* FindNearestCampEnemy (AircraftClass *aircraft, short *x, short *y, float *alt);

	void UpdateRemoteData (void);
	SimBaseClass* FindFac (SimBaseClass*);
	FlightClass* FindTanker (SimBaseClass*);
	SimBaseClass* FindATC (VU_ID);
	int MotionOn(void) {return motionOn;};
	void SetMotion (int newFlag) {motionOn = newFlag;};
	int AVTROn (void) {return avtrOn;};
	void SetAVTR (int newFlag) {avtrOn = newFlag; };
	void AddToFeatureList (VuEntity* theObject);
	void AddToObjectList (VuEntity* theObject);
	void AddToCampUnitList (VuEntity* theObject);
	void AddToCampFeatList (VuEntity* theObject);
	void AddToCombUnitList (VuEntity* theObject);
	void AddToCombFeatList (VuEntity* theObject);
	void RemoveFromFeatureList (VuEntity* theObject);
	void RemoveFromObjectList (VuEntity* theObject);
	void RemoveFromCampUnitList (VuEntity* theObject);
	void RemoveFromCampFeatList (VuEntity* theObject);
	void InitACMIRecord( void );
	void POVKludgeFunction(DWORD);
	void InitializeSimMemoryPools( void );
	void ReleaseSimMemoryPools( void );
	void ShrinkSimMemoryPools( void );
   void WakeCampaignFlight (int ctype, CampBaseClass* baseEntity, TailInsertList *flightList);
   void WakeObject (SimBaseClass* theObject);
   void SleepCampaignFlight (TailInsertList *flightList);
	void SleepObject (SimBaseClass* theObject);
	void NotifyExit( void ) { doExit = TRUE; };
	void NotifyGraphicsExit( void ) { doGraphicsExit = TRUE; };

  public:
	AircraftClass* playerEntity;

	FalconPrivateOrderedList* objectList;		// List of locally deaggregated sim vehicles
	FalconPrivateOrderedList* featureList;		// List of locally deaggregated sim features
	FalconPrivateOrderedList* campUnitList;		// List of nearby aggregated campaign units
	FalconPrivateOrderedList* campObjList;		// List of nearby aggregated campaign objectives
	FalconPrivateOrderedList* combinedList;		// List of everything nearby
	FalconPrivateOrderedList* combinedFeatureList;	// List of everything nearby
	FalconPrivateList* ObjsWithNoCampaignParentList;
	FalconPrivateList* facList;
	VuFilteredList*		atcList;
	VuFilteredList*		tankerList;

	int doFile;
	int doEvent;
	uint eventReadPointer;

	unsigned long	lastRealTime;
  private:
	// SCR:  These used to be local to the loop function, but moved
	// here when the loop got broken out into a cycle per function call.
	// Some or all of these may be unnecessary, but I'll keep them for now...
	
	unsigned long	last_elapsedTime;
	int			lastFlyState;
	int			curFlyState;
	BOOL 		doExit;
	BOOL 		doGraphicsExit;

	VuThread* vuThread;
	int curIALevel;
	char dataName[_MAX_PATH];
	int motionOn;
	int avtrOn;
	unsigned long nextATCTime;

	void UpdateEntityLists (void);
	void ReaggregateAllFlights (void);
	SimBaseClass* FindNearest(SimBaseClass*, VuLinkedList*);
	void UpdateATC(void);
};
	
extern SimulationDriver SimDriver;

#ifdef DAVE_DBG
#define CHECK_PROC_TIMES
#endif

#endif
