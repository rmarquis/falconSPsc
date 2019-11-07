#ifndef _AIRCRAFT_CLASS_H
#define _AIRCRAFT_CLASS_H

#include "simVeh.h"
#include "hardpnt.h"

#define FLARE_STATION	0
#define CHAFF_STATION	1
#define DEBRIS_STATION	2

////////////////////////////////////////////
struct DamageF16PieceStructure {
	int	mask;
	int	index;
	int	damage;
	int sfxflag;
	int	sfxtype;
	float lifetime;
	float dx, dy, dz;
	float yd, pd, rd;
	float pitch, roll;	// resting angle
};
////////////////////////////////////////////

// Class pointers used
class GunClass;
class FireControlComputer;
class AirframeClass;
class SimObjectType;
class WeaponStation;
class DrawableTrail;
class CautionClass;
class FackClass;
class SMSClass;
class DigitalBrain;
class TankerBrain;
class DrawableGroundVehicle;
class ObjectiveClass;

struct Tpoint;

class AircraftClass : public SimVehicleClass
{
	
#ifdef USE_SH_POOLS
public:
	// Overload new/delete to use a SmartHeap fixed size pool
	void *operator new(size_t size) { ShiAssert( size == sizeof(AircraftClass) ); return MemAllocFS(pool);	};
	void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
	static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(AircraftClass), 200, 0 ); };
	static void ReleaseStorage()	{ MemPoolFree( pool ); };
	static MEM_POOL	pool;
#endif
	
public:
	enum AutoPilotType	{ThreeAxisAP, WaypointAP, CombatAP, LantirnAP, APOff};
   enum ACFLAGS {
      isF16    = 0x1, // is an F-16 - hopefully historic usage only soon
      hasSwing = 0x2, // has swing wing
      isComplex = 0x4, // has a complex model (lots of dofs and switches)
      InRecovery = 0x8, // recovering from gloc
   };
      enum AvionicsPowerFlags {
	  NoPower = 0,
	    SMSPower = 0x1, FCCPower = 0x2, MFDPower = 0x4, UFCPower = 0x8, 
	    GPSPower = 0x10, DLPower = 0x20, MAPPower = 0x40,
	    LeftHptPower = 0x80, RightHptPower = 0x100,
	    TISLPower = 0x200, FCRPower = 0x400, HUDPower = 0x800,
		//MI
		EWSRWRPower = 0x1000, EWSJammerPower = 0x2000, EWSChaffPower = 0x4000,
		EWSFlarePower = 0x8000,
		//MI
		RaltPower = 0x10000, 
		RwrPower  = 0x20000,
		APPower = 0x40000,
		PFDPower = 0x80000,
		ChaffFlareCount = 0x100000,
		//MI
		IFFPower = 0x200000,
		// systems that don't have power normally.
		SpotLightPower = 0x20000000,
		InstrumentLightPower = 0x40000000, 
		InteriorLightPower   = 0x80000000, // start from the top down
	    AllPower = 0xffffff // all the systems have power normally.
      };
      enum MainPowerType { MainPowerOff, MainPowerBatt, MainPowerMain };
	// start of the power state matrix,
	// will get filled in more when I know what I'm talking about a little.
	enum PowerStates { PowerNone = 0, 
	    PowerFlcs,
	    PowerBattery,
	    PowerEmergencyBus,
	    PowerEssentialBus,
	    PowerNonEssentialBus,
	    PowerMaxState,
	};
	enum LightSwitch { LT_OFF, LT_LOW, LT_NORMAL };
	LightSwitch interiorLight, instrumentLight, spotLight;
	void SetInteriorLight(LightSwitch st) { interiorLight = st; };
	void SetInstrumentLight(LightSwitch st) { instrumentLight = st; };
	void SetSpotLight(LightSwitch st) { spotLight = st; };
	LightSwitch GetInteriorLight() { return interiorLight; };
	LightSwitch GetInstrumentLight() { return instrumentLight; };
	LightSwitch GetSpotLight() { return spotLight; };

	  //MI
	  enum EWSPGMSwitch { Off, Stby, Man, Semi, Auto };
	  //MI new OverG/Speed stuff
	  void CheckForOverG(void);
	  void CheckForOverSpeed(void);
	  void DoOverGSpeedDamage(int station);
	  void StoreToDamage(WeaponClass thing);
	  unsigned int StationsFailed;
	  enum StationFlags 
	  {
		Station1_Degr = 0x1,
		Station2_Degr = 0x2,
		Station3_Degr = 0x4,
		Station4_Degr = 0x8,
		Station5_Degr = 0x10,
		Station6_Degr = 0x20,
		Station7_Degr = 0x40,
		Station8_Degr = 0x80,
		Station9_Degr = 0x90,
		Station1_Fail = 0x100,
		Station2_Fail = 0x200,
		Station3_Fail = 0x400,
		Station4_Fail = 0x800,
		Station5_Fail = 0x1000,
		Station6_Fail = 0x2000,
		Station7_Fail = 0x4000,
		Station8_Fail = 0x8000,
		Station9_Fail = 0x9000,

	  };			
	  void StationFailed(StationFlags fl) { StationsFailed |= fl; };
	  int GetStationFailed(StationFlags fl) { return (StationsFailed & fl) == (unsigned int)fl ? 1 : 0; };

	  //MI
	  unsigned int ExteriorLights;
	  enum ExtlLightFlags 
	  {
		Extl_Main_Power		= 0x1,
		Extl_Anit_Coll		= 0x2,
		Extl_Steady_Flash	= 0x4,
		Extl_Wing_Tail		= 0x8,
	  };			
	  void ExtlOn(ExtlLightFlags fl) { ExteriorLights |= fl; };
	  void ExtlOff(ExtlLightFlags fl) { ExteriorLights &= ~fl; };
	  int ExtlState(ExtlLightFlags fl) { return (ExteriorLights & fl) == (unsigned int)fl ? 1 : 0; };

	  //MI
	  unsigned int INSFlags;
	  enum INSAlignFlags 
	  {
		INS_PowerOff		= 0x1,
		INS_AlignNorm		= 0x2,
		INS_AlignHead		= 0x4,
		INS_Cal				= 0x8,
		INS_AlignFlight		= 0x10,
		INS_Nav				= 0x20,
		INS_Aligned			= 0x40,
		INS_ADI_OFF_IN		= 0x80,
		INS_ADI_AUX_IN		= 0x100,
		INS_HUD_FPM			= 0x200,
		INS_HUD_STUFF		= 0x400,
		INS_HSI_OFF_IN		= 0x800,
		INS_HSD_STUFF		= 0x1000,
		BUP_ADI_OFF_IN		= 0x2000,
	  };			
	  void INSOn(INSAlignFlags fl) { INSFlags |= fl; };
	  void INSOff(INSAlignFlags fl) { INSFlags &= ~fl; };
	  int INSState(INSAlignFlags fl) { return (INSFlags & fl) == (unsigned int)fl ? 1 : 0; };

	  void RunINS(void);
	  void DoINSAlign(void);
	  void SwitchINSToAlign(void);
	  void SwitchINSToNav(void);
	  void SwitchINSToOff(void);
	  void SwitchINSToInFLT(void);
	  void CheckINSStatus(void);
	  void CalcINSDrift(void);
	  void CalcINSOffset(void);
	  float GetINSLatDrift(void)	{return (INSLatDrift + INSLatOffset);};
	  float GetINSLongDrift(void)	{return (INSLongDrift + INSLongOffset);};
	  float GetINSAltOffset(void)	{return INSAltOffset;};
	  float GetINSHDGOffset(void)	{return INSHDGOffset;};
	  float INSAlignmentTimer;
	  VU_TIME INSAlignmentStart;
	  bool INSAlign, HasAligned;
	  int INSStatus;
	  float INSLatDrift;
	  float INSLongDrift;
	  float INSTimeDiff;
	  bool INS60kts, CheckUFC;
	  float INSLatOffset, INSLongOffset, INSAltOffset, INSHDGOffset;
	  int INSDriftLatDirection, INSDriftLongDirection;
	  float INSDriftDirectionTimer;
	  float BUPADIEnergy;
	  bool GSValid, LOCValid;

	  //MI more realistic AVTR
	  unsigned int AVTRFlags;
	  float AVTRCountown;
	  bool doAVTRCountdown;
	  void AddAVTRSeconds(void);
	  enum AVTRStateFlags
	  {
		  AVTR_ON	= 0x1,
		  AVTR_AUTO = 0x2,
		  AVTR_OFF	= 0x4,
	  };
	  void AVTROn(AVTRStateFlags fl) { AVTRFlags |= fl; };
	  void AVTROff(AVTRStateFlags fl) { AVTRFlags &= ~fl; };
	  int AVTRState(AVTRStateFlags fl) { return (AVTRFlags & fl) == (unsigned int)fl ? 1 : 0; };


	  //MI Mal and Ind Lights test button
	  bool TestLights;
	  //MI some other stuff
	  bool TrimAPDisc;
	  bool TEFExtend;
	  bool CanopyDamaged;
	  bool LEFLocked;
	  float LTLEFAOA, RTLEFAOA;
	  unsigned int LEFFlags;
	  float leftLEFAngle;
	  float rightLEFAngle;
	  enum LEFStateFlags
	  {
		  LT_LEF_DAMAGED	= 0x1,
		  LT_LEF_OUT		= 0x2,
		  RT_LEF_DAMAGED	= 0x8,
		  RT_LEF_OUT		= 0x10,
		  LEFSASYNCH		= 0x20,
	  };
	  void LEFOn(LEFStateFlags fl) { LEFFlags |= fl; };
	  void LEFOff(LEFStateFlags fl) { LEFFlags &= ~fl; };
	  int LEFState(LEFStateFlags fl) { return (LEFFlags & fl) == (unsigned int)fl ? 1 : 0; };
	  float CheckLEF(int side);

	  int MissileVolume;
	  int ThreatVolume;
	  bool GunFire;
	  //targeting pod cooling time
	  float PodCooling;
	  bool CockpitWingLight;
	  bool CockpitStrobeLight;
	  void SetCockpitWingLight(bool state);
	  void SetCockpitStrobeLight(bool state);


	AircraftClass		(int flag, VU_BYTE** stream);
	AircraftClass		(int flag, FILE* filePtr);
	AircraftClass		(int flag, int type);
	virtual ~AircraftClass		(void);

	float					glocFactor;
	int						fireGun, fireMissile, lastPickle;
	FackClass*				mFaults;
	AirframeClass*			af;
	FireControlComputer*	FCC;
	SMSClass*				Sms;
	GunClass*				Guns;

	virtual void	Init (SimInitDataClass* initData);
	virtual int	   Wake(void);
	virtual int 	Sleep(void);
	virtual int		Exec (void);
	virtual void	JoinFlight (void);
	virtual void	InitData (int);
	virtual void	Cleanup(void);
	
	virtual int    CombatClass (void); // 2002-02-25 MODIFIED BY S.G. virtual added in front since FlightClass also have one now...
	void           SetAutopilot (AutoPilotType flag);
	AutoPilotType	AutopilotType (void) {return autopilotType;};
	VU_ID	         HomeAirbase(void);
	VU_ID	         TakeoffAirbase(void);
	VU_ID	         LandingAirbase(void);
	VU_ID	         DivertAirbase(void);
	void           DropProgramed (void);
   int            IsF16 (void) {return (acFlags & isF16 ? TRUE : FALSE);}
   int            IsComplex (void) {return ((acFlags & isComplex) ? TRUE : FALSE);}
// 2000-11-17 ADDED BY S.G. SO AIRCRAFT CAN HAVE A ENGINE TEMPERATURE AS WELL AS 'POWER' (RPM) OUTPUT   
	void SetPowerOutput (float powerOutput);//me123 changed back
// END OF ADDED SECTION	
    virtual void	SetVt(float newvt);
	virtual void	SetKias(float newkias);
	void			ResetFuel (void);
	virtual long	GetTotalFuel (void);
	virtual void	GetTransform(TransformMatrix vmat);
	virtual void	ApplyDamage (FalconDamageMessage *damageMessage);
	virtual void	SetLead (int flag);
	virtual void	ReceiveOrders (FalconEvent* newOrder);
	virtual float	GetP (void);
	virtual float	GetQ (void);
	virtual float	GetR (void);
	virtual float	GetAlpha (void);
	virtual float	GetBeta (void);
	virtual float	GetNx(void);
	virtual float	GetNy(void);
	virtual float	GetNz(void);
	virtual float	GetGamma(void);
	virtual float	GetSigma(void);
	virtual float	GetMu(void);
	virtual void	MakePlayerVehicle(void);
	virtual void	MakeNonPlayerVehicle(void);
	virtual void	MakeLocal (void);
	virtual void	MakeRemote (void);
	virtual void	ConfigurePlayerAvionics(void);
	virtual void	SetVuPosition (void);
	virtual void	Regenerate (float x, float y, float z, float yaw);
	virtual			FireControlComputer* GetFCC(void) { return FCC; };
	virtual SMSBaseClass* GetSMS(void) { return (SMSBaseClass*)Sms; };
   virtual int HasSPJamming (void);
   virtual int HasAreaJamming (void);

#ifdef _DEBUG
	virtual void	SetDead (int);
#endif // _DEBUG

	// private:
public:
	long				mCautionCheckTime;
	BOOL				MPOCmd;
	char				dropChaffCmd;
	char				dropFlareCmd;
	int            acFlags;
	AutoPilotType		autopilotType;
	AutoPilotType		lastapType;

	VU_TIME        dropProgrammedTimer;
	unsigned short		dropProgrammedStep;
	int					isDigital;
	float				bingoFuel;
	//MI taking these functions for the ICP stuff, made some changes
	float GetBingoFuel (void){return bingoFuel;};//me123
	void SetBingoFuel (float newbingo){bingoFuel = newbingo;};//me123
	void DamageSounds(void);
	unsigned int SpeedSoundsWFuel;
	unsigned int SpeedSoundsNFuel;
	unsigned int GSoundsWFuel;
	unsigned int GSoundsNFuel;
	void WrongCAT(void);
	void CorrectCAT(void);
	//MI for RALT stuff
	enum RaltStatus { ROFF, RSTANDBY, RON } RALTStatus;
	float RALTCoolTime;	//Cooling is in progress
	int RaltReady() { return (RALTCoolTime < 0.0F && RALTStatus == RaltStatus::RON) ? 1 : 0; };
	void RaltOn() {RALTStatus = RaltStatus::RON;};
	void RaltStdby () {RALTStatus = RaltStatus::RSTANDBY;};
	void RaltOff () { RALTStatus = RaltStatus::ROFF; };
	//MI for EWS stuff
	void DropEWS(void);
	void EWSChaffBurst(void);
	void EWSFlareBurst(void);
	void ReleaseManualProgram(void);
	bool ManualECM;
	int FlareCount, ChaffCount, ChaffSalvoCount, FlareSalvoCount;
	VU_TIME ChaffBurstInterval, FlareBurstInterval, ChaffSalvoInterval, FlareSalvoInterval;
	//MI Autopilot
	enum APFlags	{AltHold = 0x1, //Right switch up
						AttHold = 0x2, //Right Switch down
						StrgSel = 0x4,	//Left switch down
						RollHold = 0x8, //Left switch middle
						HDGSel = 0x10,	//Left switch up
						Override = 0x20};	//Paddle switch
	unsigned int APFlag;
	int IsOn (APFlags flag) {return APFlag & flag ? 1 : 0;};
	void SetAPFlag (APFlags flag) {APFlag |= flag;};
	void ClearAPFlag (APFlags flag) {APFlag &= ~flag;};
	void SetAPParameters(void);
	void SetNewRoll(void);
	void SetNewPitch(void);
	void SetNewAlt(void);
	//MI seatArm
	bool SeatArmed;
	void StepSeatArm(void);

	TransformMatrix		vmat;
	float				gLoadSeconds;
	long				lastStatus;
	BasicWeaponStation	counterMeasureStation[3];
	enum { // what trail is used for what
	    TRAIL_DAMAGE = 0, // we've been hit
		TRAIL_ENGINE1,
		TRAIL_ENGINE2,
		TRAIL_MAX,
		MAXENGINES = 4,

	};
	DrawableTrail*		smokeTrail[TRAIL_MAX];
	DrawableTrail	    *conTrails[MAXENGINES];
	DrawableTrail	    *engineTrails[MAXENGINES];
	DrawableTrail	*rwingvortex, *lwingvortex;
	BOOL				playerSmokeOn;
	DrawableGroundVehicle* pLandLitePool;
	BOOL				mInhibitLitePool;
	void				CleanupLitePool(void);
	void	AddEngineTrails(int ttype, DrawableTrail **tlist);
	void	CancelEngineTrails(DrawableTrail **tlist);

	// JPO Avionics power settings;
	unsigned int powerFlags;
	void PowerOn (AvionicsPowerFlags fl) { powerFlags |= fl; };
	int HasPower(AvionicsPowerFlags fl);
	void PowerOff (AvionicsPowerFlags fl) { powerFlags &= ~fl; };
	void PowerToggle (AvionicsPowerFlags fl) { powerFlags ^= fl; };
	int PowerSwitchOn(AvionicsPowerFlags fl) { return (powerFlags & fl) ? TRUE : FALSE; };

	void PreFlight (); // JPO - do preflight checks.

	// JPPO Main Power
	MainPowerType mainPower;
	MainPowerType MainPower() { return mainPower; };
	BOOL MainPowerOn() { return mainPower == MainPowerMain; };
	void SetMainPower (MainPowerType t) { mainPower = t; };
	void IncMainPower ();
	void DecMainPower ();
	PowerStates currentPower;
	enum ElectricLights {
	    ElecNone = 0x0,
		ElecFlcsPmg = 0x1,
		ElecEpuGen = 0x2,
		ElecEpuPmg = 0x4,
		ElecToFlcs = 0x8,
		ElecFlcsRly = 0x10,
		ElecBatteryFail = 0x20,
	};
	unsigned int elecLights;
	bool ElecIsSet(ElectricLights lt) { return (elecLights & lt) ? true : false; };
	void ElecSet(ElectricLights lt) { elecLights |= lt; };
	void ElecClear(ElectricLights lt) { elecLights &= ~lt; };
	void DoElectrics();
	static const unsigned long systemStates[PowerMaxState];

	//MI EWS PGM Switch
	EWSPGMSwitch EWSPgm;
	EWSPGMSwitch EWSPGM() { return EWSPgm; };
	void SetPGM (EWSPGMSwitch t) { EWSPgm = t; };
	void IncEWSPGM();
	void DecEWSPGM();
	void DecEWSProg();
	void IncEWSProg();
	//Prog select switch
	unsigned int EWSProgNum;
	//MI caution stuff
	bool NeedsToPlayCaution;
	bool NeedsToPlayWarning;
	VU_TIME WhenToPlayCaution;
	VU_TIME WhenToPlayWarning;
	void SetExternalData(void);
	//MI Inhibit VMS Switch
	bool playBetty;
	void ToggleBetty(void);
	//MI RF switch
	int RFState;	//0 = NORM; 1 = QUIET; 2 = SILENT
	void GSounds(void);
	void SSounds(void);
	int SpeedToleranceTanks;
	int SpeedToleranceBombs;
	float GToleranceTanks;
	float GToleranceBombs;
	int OverSpeedToleranceTanks[3];	//3 levels of OverSpeed for tanks
	int OverSpeedToleranceBombs[3]; //3 levels of OverSpeed for bombs
	int OverGToleranceTanks[3];	//3 levels of OverG for tanks
	int OverGToleranceBombs[3]; //3 levels of OverG for bombs
	void AdjustTankSpeed(int level);
	void AdjustBombSpeed(int level);
	void AdjustTankG(int level);
	void AdjustBombG(int level);

	void	DoWeapons (void);
	float	GlocPrediction (void);
	void	InitCountermeasures(void);
	void	CleanupCountermeasures(void);
	void	InitDamageStation(void);
	void	CleanupDamageStation (void);
	void	DoCountermeasures(void);
	void	DropChaff(void);
	void	DropFlare(void);
	void	GatherInputs(void);
	void	RunSensors(void);
	BOOL	LandingCheck( float noseAngle, float impactAngle, int groundType );
	void	GroundFeatureCheck( float groundZ );
	void	RunExplosion(void);
	void	ShowDamage (void);
	void	CleanupDamage(void);
	void	MoveSurfaces(void);
	void	RunGear (void);
	void	ToggleAutopilot (void);
	void	OnGroundInit (SimInitDataClass* initData);
	void	CheckObjectCollision( void );
	void	CheckPersistantCollision( void );
	void	CautionCheck (void);
	
	DigitalBrain *DBrain(void)			{return (DigitalBrain *)theBrain;}
	TankerBrain *TBrain(void)			{return (TankerBrain *)theBrain;}
	// so we can discover we have an aircraft at the falcentity level
	virtual int IsAirplane(void) {return TRUE;}
	virtual float	Mass(void);
	
	// Has the player triggered the ejection sequence?
	BOOL	ejectTriggered;
	float	ejectCountdown;
	BOOL	doEjectCountdown;

	//MI Emergency jettison
	bool EmerJettTriggered;
	float JettCountown;
	bool doJettCountdown; 

	//MI Cockpit nightlighting
	bool NightLight, WideView;

	
	void RemovePilot(void);
	void RunCautionChecks(void);
	
	// Run the ejection sequence
	void Eject(void);
   virtual int HasPilot (void) {return (IsSetFlag(PILOT_EJECTED) ? FALSE : TRUE);};

	virtual float GetKias();

   // Public for debug
	void AddFault(int failures, unsigned int failuresPossible, int numToBreak, int sourceOctant);
	
private:

	//used for safe deletion of sensor array when making a player vehicle
	SensorClass** tempSensorArray;
    int tempNumSensors;

protected:
int SetDamageF16PieceType (DamageF16PieceStructure *piece, int type, int flag, int mask, float speed);
int CreateDamageF16Piece (DamageF16PieceStructure *piece, int *mask);
int CreateDamageF16Effects ();
void SetupDamageF16Effects (DamageF16PieceStructure *piece);

public:
	VuEntity *attachedEntity; // JB carrier
	bool AWACSsaidAbort;		// MN when target got occupied, AWACS says something useful
private:
    void CalculateAileronAndFlap(float qf, float *al, float *ar, float *fl, float *fr);
    void CalculateLef(float qfactor);
    void CalculateStab(float qfactor, float *sl, float *sr);
    float CalculateRudder(float qfactor);
    void MoveDof(int dof, float newvalue, float rate, int ssfx = -1, int lsfx = -1, int esfx = -1);
    void DeployDragChute(int n);
	int FindBestSpawnPoint(ObjectiveClass *obj, SimInitDataClass* initData);
};

#endif

