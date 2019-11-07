#ifndef _SMS_H
#define _SMS_H

#include "hardpnt.h"
#include "fcc.h"	//MI

// Forward declaration of class pointers
class SimVehicleClass;
class SimBaseClass;
class FalconPrivateOrderedList;
class SmsDrawable;
class SimWeaponClass;
class GunClass;
class MissileClass;
class BombClass;
class AircraftClass;
class FireControlComputer;	//MI

// ==================================================================
// SMSBaseClass
// 
// By Kevin K, 6/23
// 
// This holds the minimum needed to keep track of weapons
// ==================================================================

class SMSBaseClass
{
   public:
#ifdef USE_SH_POOLS
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert( size == sizeof(SMSBaseClass) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(SMSBaseClass), 200, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif
      enum MasterArmState {Safe, Sim, Arm};
      enum CommandFlags
      {
         Loftable                = 0x0001,
         HasDisplay              = 0x0002,
         HasBurstHeight          = 0x0004,
         UnlimitedAmmoFlag       = 0x0008,
         EmergencyJettisonFlag   = 0x0010,
         Firing                  = 0x0020,
         LGBOnBoard              = 0x0040,
         HTSOnBoard              = 0x0080,
         SPJamOnBoard            = 0x0100,
         GunOnBoard              = 0x0200,
         Trainable               = 0x0400,
         TankJettisonFlag		 = 0x0800 // 2002-02-20 ADDED BY S.G. Flag it if we have jettisoned our tanks (mainly for digis)
      };
		BasicWeaponStation		**hardPoint;

		SMSBaseClass(SimVehicleClass *newOwnship, short *weapId, uchar *weapCnt, int advanced=FALSE);
		virtual ~SMSBaseClass();
		virtual void AddWeaponGraphics (void);
		virtual void FreeWeaponGraphics (void);
		virtual SimWeaponClass* GetCurrentWeapon (void);

		GunClass* GetGun (int hardpoint);
		MissileClass* GetMissile (int hardpoint);
		BombClass* GetBomb (int hardpoint);

		int GetCurrentWeaponHardpoint (void)			{ return curHardpoint; };
		short GetCurrentWeaponType (void)				{ return (short)hardPoint[curHardpoint]->weaponId; };
		short GetCurrentWeaponIndex (void);
		float GetCurrentWeaponRangeFeet (void);
		void LaunchWeapon (void);
		void DetachWeapon (int hardpoint, SimWeaponClass *theWeapon);
		int CurStationOK (void) { return StationOK(curHardpoint); };
      int StationOK (int n);
      int NumHardpoints (void) {return numHardpoints;};
      int CurHardpoint (void) {return curHardpoint;};
      int  NumCurrentWpn(void) {return numCurrentWpn;};
      void SetCurHardpoint (int newPoint) {curHardpoint = newPoint;}; // This should go one day
      SimVehicleClass* Ownship(void) {return ownship;};
      MasterArmState MasterArm(void) {return masterArm;};
      void SetMasterArm (MasterArmState newState) {masterArm = newState;};
      void StepMasterArm (void);
      void StepCatIII (void);
		void ReplaceMissile (int, MissileClass*);
		void ReplaceRocket (int, MissileClass*);
		void ReplaceBomb (int, BombClass*);

      void SetFlag (int newFlag) { flags |= newFlag; };
      void ClearFlag (int newFlag) { flags &= ~newFlag; };
      int  IsSet (int newFlag) { return flags & newFlag;};

		float GetWeaponRangeFeet (int hardpoint);

		void SelectBestWeapon (uchar *dam, int mt, int range_km, int guns_only = FALSE, int alt_feet = -1); // 2002-03-09 MODIFIED BY S.G. Added the alt_feet variable so it knows the altitude of the target as well as it range

		MasterArmState masterArm;	//MI moved from protected
		//MI
		bool BHOT;
		bool GndJett;
		bool FEDS;
		bool DrawFEDS;
		bool Powered;	//for mav's
		float MavCoolTimer;
		enum MavSubModes{ PRE, VIS, BORE};
		MavSubModes MavSubMode;
		void ToggleMavPower(void)	{Powered = !Powered;};
		void StepMavSubMode(bool init = FALSE);

   protected:
		int numHardpoints;
		int curHardpoint;
      int numCurrentWpn;
      int flags;
		SimVehicleClass* ownship;
};

// ==================================================================
// SMSClass
// 
// This is Leon's origional class, now used only for aircraft/helos
// ==================================================================

class SMSClass : public SMSBaseClass
{
#ifdef USE_SH_POOLS
   public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert( size == sizeof(SMSClass) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(SMSClass), 200, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif

   protected:

      char flash;
      int rippleInterval, pair, rippleCount, curRippleCount;
      float burstHeight;
      unsigned long nextDrop;
      void ReleaseCurWeapon(int stationUnderTest);
      void JettisonStation (int stationNum, int rippedOff = FALSE);
      void SetCurrentWeapon(int station, SimWeaponClass *weapon);
	  void SetupHardpointImage(BasicWeaponStation *hp, int count);
      
   public:
      int numOnBoard[wcNoWpn+1];
      int curWpnNum;
      int lastWpnStation, lastWpnNum;
      WeaponType curWeaponType;
      WeaponDomain curWeaponDomain;
      WeaponClass curWeaponClass;
      SmsDrawable* drawable;
      SimWeaponClass* curWeapon;
      short curWeaponId;

      SMSClass(SimVehicleClass *newOwnship, short *weapId, uchar *weapCnt);
      virtual ~SMSClass();

      virtual void AddWeaponGraphics (void);
      virtual void FreeWeaponGraphics (void);
      virtual SimWeaponClass* GetCurrentWeapon (void) {return curWeapon;};
	  
      void SetWeaponType (WeaponType newMode);
      void IncrementStores (WeaponClass wClass, int count);
      void DecrementStores (WeaponClass wClass, int count);
      void SelectiveJettison (void);
      void EmergencyJettison (void);
      void JettisonWeapon (int hardpoint);
      void RemoveWeapon (int hardpoint);
      void AGJettison (void);
	  int  DidEmergencyJettison (void)		{ return flags & EmergencyJettisonFlag; }
	  int  DidJettisonedTank (void)		{ return flags & TankJettisonFlag; } // 2002-02-20 ADDED BY S.G. Helper to know if our tanks where jettisoned
	  void TankJettison (void); // 2002-02-20 ADDED BY S.G. Will jettison the tanks (if empty) and set TankJettisonFlag
      void WeaponStep (int symFlag = FALSE);
      int  FindWeapon (int indexDesired);
      int  FindWeaponClass (WeaponClass weaponDesired, int needWeapon = TRUE);
      int  FindWeaponType (WeaponType weaponDesired);
      int  DropBomb (int allowRipple = TRUE);
      int  LaunchMissile (void);
      int  LaunchRocket (void);
      void SetUnlimitedGuns (int flag);
      int UnlimitedAmmo (void) {return flags & UnlimitedAmmoFlag;};
      void SetUnlimitedAmmo (int newFlag);
      int  HasHarm (void) {return (flags & HTSOnBoard ? TRUE : FALSE);};
      int  HasLGB (void) {return (flags & LGBOnBoard ? TRUE : FALSE);};
      int  HasTrainable (void);
      int  HasSPJammer (void) {return (flags & SPJamOnBoard ? TRUE : FALSE);};
      int  HasWeaponClass (WeaponClass classDesired);
      void FreeWeapons (void);
      void Exec (void);
      void SetPlayerSMS (int flag);
      void SetPair (int flag);
      void IncrementRippleCount(void);
      void DecrementRippleCount(void);
	  void SetRippleInterval (int rippledistance); // Marco Edit - for AI A2G
      void IncrementRippleInterval(void);
      void DecrementRippleInterval(void);
      void IncrementBurstHeight(void);
      void DecrementBurstHeight(void);
      void Incrementarmingdelay(void);//me123 status test. addet
      void ResetCurrentWeapon(void);
      void SetRippleCount (int newVal) {rippleCount = newVal;};
      int  RippleCount(void) {return rippleCount;};
			int  CurRippleCount(void) {return curRippleCount;}; // JB 010708
      int  RippleInterval(void) {return rippleInterval;};
      int  Pair(void) {return pair;};
      WeaponType GetNextWeapon (WeaponDomain);
      WeaponType GetNextWeaponSpecific (WeaponDomain);
      void SelectWeapon(WeaponType ntype, WeaponDomain domainDesired);
      void RemoveStore(int station, int storeId);
      void AddStore(int station, int storeId, int visible);
      void ChooseLimiterMode(int hardpoint);
      void RipOffWeapons( float noseAngle );
	  float armingdelay;//me123 status ok. armingdelay addet
	  int aim120id; // JPO Aim120 Id no.
      int AimId () { return aim120id+1; };
      void NextAimId () { aim120id = (aim120id + 1) % 4; };
      enum Aim9Mode { WARM, COOLING, COOL, WARMING } aim9mode;
      Aim9Mode GetCoolState() { return aim9mode; };
      void SetCoolState (Aim9Mode state) { aim9mode = state; };
	  // AIM9 time left for cooling
      /*VU_TIME aim9cooltime;
	  // AIM9 cooling time left - approx. 3.5 hours worth
	  VU_TIME aim9coolingtimeleft;
	  // AIM9 - time to warm up after removing coolant
	  VU_TIME aim9warmtime;*/
	  float aim9cooltime;
	  float aim9coolingtimeleft;
	  float aim9warmtime;

	  //MI
	  int Prof1RP, Prof2RP;
	  int Prof1RS, Prof2RS;
	  int Prof1NSTL, Prof2NSTL;
	  int C2BA;
	  int angle;
	  float C1AD1, C1AD2, C2AD;
	  bool Prof1Pair, Prof2Pair, Prof1;
	  FireControlComputer::FCCSubMode Prof1SubMode, Prof2SubMode;
	  void StepWeaponClass(void);

	  //MI
	  void FindAim120(int *stationUnderTest, WeaponType *newType, SimWeaponClass **found);
	  void FindAim9M(int *stationUnderTest, WeaponType *newType, SimWeaponClass **found);
	  void FindAim9P(int *stationUnderTest, WeaponType *newType, SimWeaponClass **found);
	  void FindGun(int *stationUnderTest, WeaponType *newType, SimWeaponClass **found);
	        
   friend SmsDrawable;
};

SimWeaponClass* InitWeaponList (FalconEntity* parent, ushort weapid, int weapClass, int num,
								SimWeaponClass* initFunc(FalconEntity* parent, ushort type, int slot)) ;

#endif
