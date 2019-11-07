#ifndef _RADAR_MODEL_H
#define _RADAR_MODEL_H

#include "radar.h"

class SimBaseClass;

#define EL_CHANGE_RATE    0.005F//me123 status ok. changed from 0.01
#define NUM_RANGES        5
#define NUM_VELS          2
#define NUM_RWS_AZS       3
#define NUM_RWS_BARS      3
#define NUM_SAM_AZS       2
#define NUM_SAM_BARS      3
#define PFA               0.000001F
#define AZL               1.0F
#define MAX_ANT_EL        (60.0F * DTR)
#define HITS_FOR_LOCK     2
#define HITS_FOR_TRACK    1
#define MAX_NCTR_RANGE    (60.0F * NM_TO_FT)
#define NCTR_DELTA        0.1F
//MI
#define IFF_FRIENDLY		1
#define IFF_HOSTILE			2
#define IFF_UNKNOWN			3
#define MAX_CONTACTS		20 //for random IFF offset


class RadarDopplerClass : public RadarClass
{
  public:
	RadarDopplerClass (int idx, SimMoverClass* self);
	virtual ~RadarDopplerClass();

	virtual void DisplayInit (ImageBuffer* newImage);

	virtual SimObjectType* Exec( SimObjectType* targetList );
	virtual void ExecModes( int newDesignate, int newDrop );
	virtual void UpdateState( int cursorXCmd, int cursorYCmd );
	virtual void Display( VirtualDisplay* );

	// Queued -- call on any thread
	virtual void PushButton( int whichButton, int whichMFD = 0);
	void AGPushButton( int whichButton, int whichMFD);
	void AAPushButton( int whichButton, int whichMFD);
	void OtherPushButton( int whichButton, int whichMFD);
	void MenuPushButton( int whichButton, int whichMFD);
	void CtlPushButton( int whichButton, int whichMFD);
	virtual void RangeStep( int cmd )				{ rangeChangeCmd = cmd; };	// Step up/down in range

	virtual void StepAAmode( void );
	virtual void SetSRMOverride();
	virtual void SetMRMOverride();
	virtual void ClearOverride();

	virtual void SelectACMVertical()				{ modeDesiredCmd = ACM_10x60; };
	virtual void SelectACMBore()					{ modeDesiredCmd = ACM_BORE; };
	virtual void SelectACMSlew()					{ modeDesiredCmd = ACM_SLEW; };
	virtual void SelectACM30x20()					{ modeDesiredCmd = ACM_30x20; };
	//MI added
	//need the current RadarMode
	virtual RadarMode	GetRadarMode(void)	{return mode;};
	virtual void SelectTWS(void)					{ modeDesiredCmd = TWS;};
	virtual void SelectRWS(void)					{ modeDesiredCmd = RWS;};
	virtual void SelectSAM(void)					{ if(prevMode == LRS) modeDesiredCmd = LRS; else modeDesiredCmd = RWS; ClearFlagBit(STTingTarget);};
	bool DrawRCR;
	void DrawRCRCount(void);
	virtual void SelectGMT(void)					{ modeDesiredCmd = GMT;};
	virtual void SelectGM(void)						{ modeDesiredCmd = GM;};
	virtual void SelectAGR(void)					{ modeDesiredCmd = AGR;};
	virtual void SelectLastAGMode(void)				{ ChangeMode(LastAGMode);};
	//target step stuff
	void FindClosest(float MinRange);
	void StepAzimuth(float X, int Cmd);

	virtual void StepAAscanHeight()					{ scanHeightCmd = TRUE; };
	virtual void StepAAscanWidth()					{ scanWidthCmd = TRUE; };
	virtual void StepAAelvation( int cmd )			{ if (cmd) elSlewCmd = cmd; else centerCmd = TRUE; };

	virtual void StepAGmode( void );

	virtual void StepAGfov()						{ fovStepCmd = TRUE; };
	virtual void StepAGgain( int cmd )				{ gainCmd = (cmd>0) ? 1.25f : 0.8f; };

   void SetAGSnowPlow(int val);
   void SetAGFreeze(int val);
   virtual void SetAGSteerpoint(int val);
	virtual void ToggleAGfreeze();
	virtual void ToggleAGsnowPlow();
	virtual void ToggleAGcursorZero();
	virtual void DefaultAGMode (void);
	virtual void DefaultAAMode (void);

	virtual void NextTarget (void);
	virtual void PrevTarget (void);
	virtual void SetGroundPoint (float rx, float ry, float rz);
	virtual int  IsAG (void);
	virtual void GetAGCenter (float* x, float* y);
	virtual float GetRange (void)					{ return displayRange; };
	virtual float GetVolume (void)					{ return azScan; };
	virtual void GetCursorPosition (float* xPos, float* yPos);
	virtual int GetBuggedData (float *x, float *y, float *dir, float *speed);

    virtual void SetMode (RadarMode cmd);

	//MI
	virtual void ClearModeDesiredCmd(void)	{modeDesiredCmd = -1;};
	//2002-04-04 MN
	virtual void RestoreAGCursor(void);

  protected:
	// Command queues -- These store commands until we're able to process them
	float	gainCmd;
	int   rangeChangeCmd, scanHeightCmd, scanWidthCmd, elSlewCmd;
	int   modeDesiredCmd, fovStepCmd;
	int   dropTrackCmd, designateCmd, centerCmd;

	virtual void ClearSensorTarget (void);
	virtual void SetSensorTarget (SimObjectType*);

	void AABottomRow (void);
	void AGBottomRow (void);

	//MI moved to public
  //protected:
  public:
	// State
	enum ModeFlags 
	{
		NORM = 0x1,
		EXP = 0x2,
		DBS1 = 0x4,
		DBS2 = 0x8,
		CZ = 0x10,
		FZ = 0x20,
		SP = 0x40,
		SpaceStabalized = 0x100,
		Designating = 0x200,
		Spotlight = 0x400,
		ChangingBars = 0x800,
		HorizontalScan = 0x1000,
		VerticalScan = 0x2000,
		WasMoving    = 0x4000,
		SAMingTarget = 0x8000,
		STTingTarget = 0x10000,
		HomingBeam   = 0x20000,
		AADecluttered= 0x40000,
	    AGDecluttered= 0x80000,
		MenuMode	 = 0x100000,
		CtlMode	     = 0x200000,
		AutoAGRange	 = 0x400000,
	};
  protected:
	enum Declutter 
	{
		MajorMode =	0x1,
		SubMode =	0x2,
		Fov =	0x4,
		Ovrd =	0x8,
		Cntl =	0x10, 
	    Alt =	0x20,
	    BupSen = Alt, // AG equiv
	    AttackStr = 0x40,
	    FzSp = AttackStr, // AG equiv
	    Dlz =	0x80,
	    Cz	= Dlz, // AG equiv
	    TgtData =	0x100,
	    SightPt = TgtData,
	    Dclt =	0x200,
	    Fmt1 =	0x400,
	    Fmt2 =	0x800,
	    Fmt3 =	0x1000,
	    Swap =	0x2000,
	    WpnStat =	0x4000,
	    AzBar =	0x8000,
	    Rng =	0x10000,
	    Arrows =	0x20000,
		DefaultAgDclt = Ovrd|Cntl|BupSen|FzSp|Cz|Dclt|Fmt1|Fmt2|Fmt3|Swap|Arrows,
	    DefaultAaDclt = Ovrd|Cntl|Dclt|Fmt1|Fmt2|Fmt3|Swap|Arrows,
	};

	class GMList
	{
      protected:
		   FalconEntity* object;
         int count;

      public:
         float x, y;
		   GMList* prev;
		   GMList* next;

         GMList (FalconEntity* obj);
         void Release(void);
         FalconEntity* Object(void) {return object;};
	};

	// State
	//MI moved to public
	public:
   int  IsSet (int newFlag) {return (newFlag & flags) ? TRUE : FALSE;}; //MI moved to public
   void SetFlagBit (int newFlag) {flags |= newFlag;};
	void ClearFlagBit (int newFlag) {flags &= ~newFlag;};
	void ToggleFlag (int flag) {flags ^= flag; };

	//MI
	class SimObjectLocalData* lockedTargetData;
	void TargetToXY(SimObjectLocalData *localData, int hist, float drange, float *x, float *y);
	float GetDisplayRange(void)	{return tdisplayRange;};

	void SetScanDir(float dir)	{scanDir = dir;};

	protected:

	// DCLT variables
	int  IsAADcltBit(int newFlag) {return (newFlag & aadclt) ? TRUE : FALSE;};
	int IsAADclt(int flg) { return IsSet(AADecluttered) && IsAADcltBit(flg); };
	void SetAADcltBit (int newFlag) {aadclt |= newFlag;};
	void ClearAADcltBit (int newFlag) {aadclt &= ~newFlag;};
	void ToggleAADclt(int flag) {aadclt ^= flag; };

	int IsAGDclt(int flg) { return IsSet(AGDecluttered) && IsAGDcltBit(flg); };
	int  IsAGDcltBit(int newFlag) {return (newFlag & agdclt) ? TRUE : FALSE;};
	void SetAGDcltBit (int newFlag) {agdclt |= newFlag;};
	void ClearAGDcltBit (int newFlag) {agdclt &= ~newFlag;};
	void ToggleAGDclt(int flag) {agdclt ^= flag; };

	void ChangeMode(int newMode);

	// State
	RadarMode mode;
	int bars;
   int reacqFlag;
	float beamAz, beamEl;
	float azScan, elScan;
	float displayRange, displayAzScan;
	float cursorX, cursorY;
   float curCursorRate;
   //MI moved to public
	//class SimObjectLocalData* lockedTargetData;

  protected :
	enum {ScanFwd = 1, ScanRev = -1, ScanNone = 0};
	enum DisplayShapes {None, Det, Solid, Track, FlashTrack, Prio, Schweem, Bug, FlashBug,
			Tail, SolidTrack, GMObj, GMTrack, GMBeacon, Jam,
			HitInd, AimRel, AimFlash,InterogateFoe, InterogateFriend, InterogateUnk // JPO new symbols
	};
	enum {TwsFlashTime = 8000, TwsExtrapolateTime = 8000, ReacqusitionCount = 13000};

	// Beam Movement
	float targetEl, targetAz;
	float barWidth;
	float scanDir, scanRate;
	float beamWidth, tdisplayRange, tbarWidth;
	float curScanTop, curScanBottom, curScanLeft, curScanRight;
	float scanCenterAlt;
	float lastAzScan, lastSAMAzScan;
	int   patternTime;
	int   lastBars, lastSAMBars;

	// Mode/State Data
	float rangeScales[NUM_RANGES], rwsAzs[NUM_RWS_AZS];
	float velScales[NUM_VELS];
	int   rwsBars[NUM_RWS_BARS];
	int   curRangeIdx, curBarIdx, vsVelIdx, curAzIdx;
   int   gmRangeIdx, airRangeIdx;
   int   rwsAzIdx, twsAzIdx, vsAzIdx, gmAzIdx, gmtAzIdx;
   int   rwsBarIdx, twsBarIdx, gmBarIdx, vsBarIdx;
	float groundDesignateX, groundDesignateY, groundDesignateZ;
	float groundLookAz, groundLookEl, groundMapRange;
	float cursRange;
	float reacqEl;
   float nctrData;
	unsigned int  flags;
	unsigned int aadclt;
	unsigned int agdclt;
	int  subMode;
	char groundMapLOD;
   long lastFeatureUpdate;
	GMList* GMFeatureListRoot;
	GMList* GMMoverListRoot;
   float GMXCenter, GMYCenter;
   void SetGroundTarget (FalconEntity* newTarget);

	// Detection Factors
	float signalFactor1, signalFactor2;

	// Functions
	void MoveBeam(void);
	void SetScan(void);
   void CalcSAMAzLimit(void);
	int LookingAtObject (SimObjectType*);
	int ObjectDetected (SimObjectType*);
	float GetRCS (SimObjectType*);
	float DopplerNotch (SimObjectType*);
	float Jamming (SimObjectType*);
	int InResCell (SimObjectType*, int, int *, int *, int*);
	// radar data stuff JPO
   int channelno;
   int histno;
   int level;
   int mkint;
   float bdelay;
   enum RadarModeFlags {
       AltTrack = 0x1,
	   PmMode	    = 0x2,
	   NaroBand	    = 0x4,
	   SpeedLo	    = 0x8,
   };
   unsigned int radarmodeflags;
   void SetModeFlag (int flag) { radarmodeflags |= flag; };
   void ClrModeFlag (int flag) { radarmodeflags &= ~flag; };
   void ToggleModeFlag (int flag) { radarmodeflags ^= flag; };
   int IsModeFlag(int flag) { return (radarmodeflags & flag) ? 1 : 0; };

	// Mode Functions
	void TWSMode(void);
	void RWSMode(void);
	void SAMMode(void);
	void VSMode(void);
	void STTMode(void);
	void ACMMode(void);
	void GMMode(void);
	void AddToHistory(SimObjectType* ptr, int sy);
	void SetHistory(SimObjectType* ptr, int sy);
	void ClearHistory(SimObjectType* ptr);
	void ClearAllHistory (void);
	void SlipHistory(SimObjectType* ptr);
	void ExtrapolateHistory(SimObjectType* ptr);

	// Bscope Functions
	void DrawRangeTicks(void);
	void DrawRangeArrows(void);
	void DrawRange(void);
	void DrawAzLimitMarkers(void);
	void DrawACQCursor(void);
	void DrawSlewCursor(void);
	void DrawCollisionSteering (SimObjectType* buggedTarget, float curX);
	void DrawSteerpoint(void);
	void DrawBars(void);
	void RWSDisplay(void);
	void TWSDisplay(void);
	void STTDisplay(void);
	void VSModeDisplay(void);
	void VSDisplay(void);
	void ACMDisplay(void);
	void SAMDisplay(void);
	void GMDisplay(void);
	void MENUDisplay(void);
	void STBYDisplay(void);
	//MI
	void AGRangingDisplay(void);
	void DrawReference(VirtualDisplay* display);
   void DrawDLZSymbol(void);
	void DrawSymbol(int type, float schweemLen, int age, float x = 0, float y = 0, int temp = 0, SimObjectType *rdrObj = NULL);	//MI added X and Y and temp and rdrObj
	int  IsUnderCursor (SimObjectType* , float heading);
	int  IsUnderVSCursor (SimObjectType* , float heading);
	void DrawWaterline(void);
	void DrawScanMarkers(void);
	void DrawCursor(void);
	void DrawAzElTicks(void);
	void DrawTargets(void);
	//MI changed
	public:
	int GetInterogate(SimObjectType *rdrObj, SimObjectType *lockedTarget = NULL);
	void SetInterogateTimer(int Dir = 0);
	bool LOS, SCAN, CmdLOS;
	float InterogateTimer;
	float InterogateDelay;
	int InterogateModus;
	float Input;
	int Pointer;
	bool CountUp;
	bool DoInterogate;
	bool Mode1, Mode2, Mode3, Mode4, ModeC;
	float randoffsetx[MAX_CONTACTS];
	float randoffsety[MAX_CONTACTS];
	int randsideoffsetx[MAX_CONTACTS];
	int randsideoffsety[MAX_CONTACTS];
	void DrawIFFStatus(void);
	void UpdateLOSScan(void);
	int GetCurScanMode(int i);
	void GetBuggedIFF(float *x, float *y, int *type);
	protected:
	void SetGMScan(void);
	void SetAimPoint (float, float);
   void AdjustGMOffset (int rangeChangeCmd);
   int  CheckGMBump (void);
   void DropGMTrack(void);
	static void AddTargetReturnCallback( void* self, class RenderGMRadar* renderer );
	void AddTargetReturns( class RenderGMRadar* renderer );
	void AddTargetReturnsOldStyle (GMList*);
	void DoGMDesignate (GMList*);
	void FreeGMList (GMList* theList);
	//MI moved to public
	//void TargetToXY(SimObjectLocalData *localData, int hist, float drange, float *x, float *y);
	void DrawNCTR(bool TWS);
	//MI remember our last MissOvr range
	int MissOvrRangeIdx, lastAirRangeIdx;
	int LastAGModes;	//MI for remembering SnowPlow and that stuff
	bool WasAutoAGRange;
	float GainPos;
	float curgain;
	bool InitGain;
//	float GMTSlowSpeedReject; MN externalised F4Config.cpp
//	float GMTHighSpeedReject;
};

/* KLUDGE MACRO */
#define RES180(a)  ((a) > 180.0F*DTR ? (a) - 360.0F*DTR :\
	((a) < -180.0F * DTR ? (a) + 360.0F * DTR : (a)))

#endif

