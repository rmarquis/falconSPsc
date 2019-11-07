#ifndef _MISSILE_H
#define _MISSILE_H

#include "simmath.h"
#include "geometry.h"
#include "simweapn.h"
#include "MsgInc\MissileEndMsg.h"

// Forward declarations for class pointers
class SimInitDataClass;
class SimObjectType;
class DrawableClass;
class DrawableTrail;
class Drawable2D;
class DrawableBSP;


/*
** MissileInFlightData class will hold all the variables that the missile
** needs when its launched.  This should help improve the memory usage
** since there's a TON of variables and they aren't needed when the missile
** is pre-launch
*/
class MissileInFlightData
{
#ifdef USE_SH_POOLS
   public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert( size == sizeof(MissileInFlightData) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(MissileInFlightData), 20, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif
   public:
      MissileInFlightData (void);
      ~MissileInFlightData (void);

      // Gains
      float kp01, kp02, kp03, kp04, kp05, kp06, kp07;
      float tp01, tp02, tp03, tp04;
      float wp01;
      float zp01;
      float ky01, ky02, ky03, ky04, ky05, ky06, ky07;
      float ty01, ty02, ty03, ty04;
      float wy01;
      float zy01;

      // Geometry
      float alpdot, betdot;
      float gamma, sigma, mu;
	  ObjectGeometry geomData;

      // Guidance
      struct { float yaw, pitch; } augCommand;

      // State
      int burnIndex;
      float rstab, qstab;
      float e1, e2, e3, e4;

      // Save Arrays
	  SAVE_ARRAY olddx;
      SAVE_ARRAY olde1, olde2, olde3, olde4, oldx, oldy, oldz;
      SAVE_ARRAY oldimp, oldvt, olddu;
      SAVE_ARRAY oldp01, oldp02, oldp03, oldp04, oldp05;
      SAVE_ARRAY oldy01, oldy02, oldy03, oldy04, oldy05;
      int oldalp, oldalpdt, oldbet, oldbetdt;

      // Aero data
      int lastmach, lastalpha;
      float qsom, qovt, qbar;

      // Accels
      float xaero, yaero, zaero;
      float xsaero, ysaero, zsaero;
      float xwaero, ywaero, zwaero;
      float xprop, yprop, zprop;
      float xsprop, ysprop, zsprop;
      float xwprop, ywprop, zwprop;
      float nxcgw, nycgw, nzcgw;
      float nxcgb, nycgb, nzcgb;
      float nxcgs, nycgs, nzcgs;

      float clalph, cybeta;

      // Closest Approach
      long lastCMDeployed;
	  bool stage2gone; // 2nd stage jettisoned
};

/*------------------------*/
/* Global data structures */
/*------------------------*/
class MissileAeroData
{
   public:
      ~MissileAeroData (void) {delete mach; delete alpha; delete cx; delete cz; } ;
      int numMach;
      int numAlpha;
      float *mach;
      float *alpha;
      float *cx;
      float *cz;
};

class MissileEngineData
{
   public:
      ~MissileEngineData(void) { delete times; delete thrust; };
      int numBreaks;
      float *times;
      float *thrust;
};


class MissileRangeData
{
   public:
      ~MissileRangeData(void) {delete altBreakpoints; delete velBreakpoints;
         delete aspectBreakpoints; delete data; } ;
	   int numAltBreakpoints;
	   int numVelBreakpoints;
	   int numAspectBreakpoints;
	   float *altBreakpoints;
	   float *velBreakpoints;
	   float *aspectBreakpoints;
	   float *data;
};

class MissileInputData
{
   public:
      float maxTof;
      float mslpk;
      float wm0;
      float wp0;
      float totalImpulse;
      float area;
      float nozzleArea;
      float length;
      float aoamax;
      float aoamin;
      float betmax;
      float betmin;
      float mslVmin;
      float gimlim;
      float gmdmax;
      float atamax;
      float guidanceDelay;
      float mslBiasn;
      float mslGnav;
      float mslBwap;
      float mslLoftTime;
      float mslActiveTtg;
      int   seekerType, seekerVersion;
      int   displayType;
	  float boostguidesec;//me123 how many sec we are in boostguide mode
	  float terminalguiderange;//me123 what range we transfere to terminal guidence
	  float boostguideSensorPrecision;//me123
	  float	sustainguideSensorPrecision;//me123
	  float	terminalguideSensorPrecision; //me123
	  float boostguideLead;//me123
	  float	sustainguideLead;//me123
	  float	terminalguideLead;//me123
	  float boostguideGnav;//me123
	  float	sustainguideGnav;//me123
	  float	terminalguideGnav;//me123
	  float boostguideBwap;//me123
	  float	sustainguideBwap;//me123
	  float	terminalguideBwap;//me123
};

class MissileAuxData {
public:
    float maxGNormal; // manueveur limits in normal phase
    float maxGTerminal; // limits in the terminal phase
	float MinEngagementRange; // Min range for the missiles -> MN moved to radarData 2002-03-08 S.G. Reinstated here so it's more granular
	float MinEngagementAlt; //  2002-03-08 ADDED BY S.G. Instead of in radarData so it's more granular
	float ProximityfuseChange;
	float SecondStageTimer; // JPO - 2 stage missiles
	float SecondStageWeight; // JPO - 2 stage missile weight of discarded stage
	float deployableWingsTime;	// A.S. Time until wings deploy
	int mistrail; // MN missile specific trails
	int misengGlow;
	int misengGlowBSP;
	int misgroundGlow; // MN
	int proximityfuserange;
	int errorfromparrent;
};

class MissileClass : public SimWeaponClass
{
   public:
      MissileClass (VU_BYTE** stream);
      MissileClass (FILE* filePtr);
      MissileClass (int type);
      virtual ~MissileClass (void);

      enum MissileFlags {
         EndGame             = 0x1,
         ClosestApprch       = 0x2,
         SensorLostLock      = 0x4,
      };

      int Flags (void) {return flags;};
      enum FlightState {PreLaunch, Launching, InFlight};
      enum DisplayType {DisplayNone, DisplayBW, DisplayIR, DisplayColor, DisplayHTS};
      int GetDisplayType (void) { return inputData->displayType;};
      int GetSeekerType (void) { return inputData->seekerType;};
      float GetmaxTof (void) { return inputData->maxTof;};//me123

	   DrawableTrail *trail;
	   Drawable2D *engGlow;
	   Drawable2D *groundGlow;
	   DrawableBSP *engGlowBSP1;
      DrawableClass* display;
      SimBaseClass* slaveTgt;
      FalconMissileEndMessage::MissileEndCode done;
      FlightState launchState;
      int isCaged;
	  int isSpot;			// Marco edit - SPOT/SCAN Mode
	  int isSlave;			// Marco edit - Boresight/Slaved
	  int isTD;				// Marco edit - TD/BP (Auto-uncage)
      float alpha, alphat, beta;
      float x, y, z, alt;
      float xdot, ydot, zdot;
      float theta, phi, psi;
	  float groundZ;
	  int lastRmaxAta;
	  int lastRmaxAlt, lastRmaxVt;

      void Init (void);
      virtual void Init (SimInitDataClass* initData);
      int Exec (void);
      void GetTransform(TransformMatrix vmat);
		void Start (SimObjectType* tgt);
      void UpdateTargetData(void);
      void SetTarget (SimObjectType* tgt);
      void ClearReferences (void);
      void DropTarget (void);
      int  SetSeekerPos(float* az, float* el);
      void GetSeekerPos(float* az, float* el);
      void RunSeeker (void);
      void InitTrail (void);
      void UpdatePosition (void);
      void SetTargetPosition (float newX, float newY, float newZ) {
         targetX = newX; targetY = newY; targetZ = newZ;};
      void GetTargetPosition (float* newX, float* newY, float* newZ) {
         *newX = targetX; *newY = targetY; *newZ = targetZ;};
      void SetTargetVelocity (float newDX, float newDY, float newDZ) {
         targetDX = newDX; targetDY = newDY; targetDZ = newDZ;};
      void GetTargetVelocity (float* newDX, float* newDY, float* newDZ) {
         *newDX = targetDX; *newDY = targetDY; *newDZ = targetDZ;};
		float GetRMax (float alt, float vt, float az, float targetVt, float ataFrom);
		float GetRMin (float alt, float vt, float ataFrom, float targetVt);
		float GetRNe  (float alt, float vt, float ataFrom, float targetVt);
		float GetTOF  (float alt, float vt, float ataFrom, float targetVt, float range);
		float GetASE  (float alt, float vt, float ataFrom, float targetVt, float range);
		float GetActiveTime(float alt, float vt, float ataFrom, float targetVt, float range);
		float GetActiveRange(float alt, float vt, float ataFrom, float targetVt, float range);
      void SetLaunchPosition (float nx, float ny, float nz) {initXLoc = nx; initYLoc = ny; initZLoc = nz;};
      void SetLaunchRotation (float az, float el) {initAz = az; initEl = el;};
      void SetLaunchData (void);
      void SetDead (int);
      virtual int Sleep (void);
      virtual int Wake (void);
      virtual void SetVuPosition (void);
      virtual int IsMissile (void) { return TRUE; };
	  BOOL FindRocketGroundImpact ( float *impactX,
	  								float *impactY,
									float *impactZ,
									float *impactTime );
	   //MI
   bool Covered;
   bool HOC;
   bool Pitbull, SaidPitbull;

   bool wentActive;

private:

      // Performance Data
      float weight, wprop, mass;
      float m0, mp0, mprop;

      MissileAeroData    *aeroData;
	  MissileInputData   *inputData;
	  MissileRangeData   *rangeData;
      MissileEngineData  *engineData;
      MissileAuxData *auxData;

      int flags;
      float runTime;
	  int guidencephase;//me123 0=boost 1=intercept 2=terminal 
	  float GuidenceTime;
      float vt, vtdot, mach, rho, ps, vcas;

      float targetX,  targetY,  targetZ;
      float targetDX, targetDY, targetDZ;

      // Launch Data
      float initXLoc, initYLoc, initZLoc;
      float initAz, initEl;
      float gimbal, gimdot, ataerr;
      float ata;
      float p, q, r;
      float timpct,ricept;
      float range ;

	  // pointer to class for in flight data
	  MissileInFlightData	*ifd;


      // Functions
      void Accelerometers(void);
      void Aerodynamics (void);
      void Atmosphere (void);
      void ClosestApproach (void);
      void CommandGuide(void);
	  void CheckGuidePhase(void);
      void Engine(void);
      void EquationsOfMotion(void);
      void Flight(void);
      void FlightControlSystem(void);
      void FlyMissile(void);
      void Gains(void);
	  void GoActive(void);
      void Init1(void);
      void Init2(void);
      void Launch (void);
      void LimitSeeker(float az, float el);
      void Pitch(void);
      void ReadAero(int idx);
      void ReadEngine(int idx);
      void ReadInput(int idx);
      void ReadRange(int idx);
      void ReadDegrade(int idx);
      void SetStatus(void);
      void Trigenometry (void);
      void Yaw(void);
      void UpdateTrail (void);
      void RemoveTrail (void);
      void InitData (void);
	  void EndMissile( void );
	  void ApplyProximityDamage( void );

#ifdef USE_SH_POOLS
   public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert( size == sizeof(MissileClass) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(MissileClass), 200, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif
};

#endif

