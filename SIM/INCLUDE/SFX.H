#ifndef _SFX_H
#define _SFX_H

// special effects type defines
#define SFX_AIR_HANGING_EXPLOSION	0
#define SFX_SMALL_HIT_EXPLOSION		1
#define SFX_AIR_SMOKECLOUD			2
#define SFX_SMOKING_PART			3
#define SFX_FLAMING_PART			4
#define SFX_GROUND_EXPLOSION		5
#define SFX_TRAIL_SMOKECLOUD		6
#define SFX_TRAIL_FIREBALL			7
#define SFX_MISSILE_BURST			8
#define SFX_CLUSTER_BURST			9
#define SFX_AIR_EXPLOSION			10
#define SFX_EJECT1					11
#define SFX_EJECT2					12
#define SFX_SMOKERING				13
#define SFX_AIR_DUSTCLOUD			14
#define SFX_GUNSMOKE				15
#define SFX_AIR_SMOKECLOUD2			16
#define SFX_NOTRAIL_FLARE			17
#define SFX_FEATURE_CHAIN_REACTION	18
#define SFX_WATER_EXPLOSION			19			
#define SFX_SAM_LAUNCH				20
#define SFX_MISSILE_LAUNCH			21
#define SFX_DUST1					22
#define SFX_EXPLCROSS_GLOW			23
#define SFX_EXPLCIRC_GLOW			24
#define SFX_TIMER					25
#define SFX_DIST_AIRBURSTS			26
#define SFX_DIST_GROUNDBURSTS		27
#define SFX_DIST_SAMLAUNCHES		28
#define SFX_DIST_AALAUNCHES			29
#define SFX_EXPLSTAR_GLOW			30
#define SFX_RAND_CRATER				31
#define SFX_GROUND_EXPLOSION_NO_CRATER		32
#define SFX_MOVING_BSP           33
#define SFX_DIST_ARMOR           34
#define SFX_DIST_INFANTRY           35
#define SFX_TRACER_FIRE           36
#define SFX_FIRE           37
#define SFX_GROUND_STRIKE           38
#define SFX_WATER_STRIKE           39
#define SFX_VERTICAL_SMOKE           40
#define SFX_TRAIL_FIRE	           41
#define SFX_BILLOWING_SMOKE           42
#define SFX_HIT_EXPLOSION           43
#define SFX_SPARKS		           44
#define SFX_ARTILLERY_EXPLOSION		           45
#define SFX_SHOCK_RING		           46
#define SFX_NAPALM		           47
#define SFX_AIRBURST			48
#define SFX_GROUNDBURST		49
#define SFX_GROUND_STRIKE_NOFIRE           50
#define SFX_LONG_HANGING_SMOKE           51
#define SFX_SMOKETRAIL           52
#define SFX_DEBRISTRAIL           53
#define SFX_HIT_EXPLOSION_DEBRISTRAIL           54
#define SFX_RISING_GROUNDHIT_EXPLOSION_DEBRISTRAIL           55
#define SFX_FIRETRAIL           56
#define SFX_FIRE_NOSMOKE           57
#define SFX_LIGHT_CLOUD           58
#define SFX_WATER_CLOUD           59
#define SFX_WATERTRAIL           60
#define SFX_GUN_TRACER           61
#define SFX_DARK_DEBRIS           62
#define SFX_FIRE_DEBRIS           63
#define SFX_LIGHT_DEBRIS           64
#define SFX_SPARKS_NO_DEBRIS           65
#define SFX_BURNING_PART           66
#define SFX_AIR_EXPLOSION_NOGLOW			67
#define SFX_HIT_EXPLOSION_NOGLOW          68
#define SFX_SHOCK_RING_SMALL		           69
#define SFX_FAST_FADING_SMOKE		           70
#define SFX_LONG_HANGING_SMOKE2           71
#define SFX_SMALL_AIR_EXPLOSION           72
#define SFX_FLAME		            73
#define SFX_AIR_PENETRATION		    74
#define SFX_GROUND_PENETRATION		75
#define SFX_DEBRISTRAIL_DUST        76
#define SFX_FIRE_EXPAND           	77
#define SFX_FIRE_EXPAND_NOSMOKE     78
#define SFX_GROUND_DUSTCLOUD		79
#define SFX_SHAPED_FIRE_DEBRIS		80
#define SFX_FIRE_HOT				81
#define SFX_FIRE_MED				82
#define SFX_FIRE_COOL				83
#define SFX_FIREBALL				84
#define SFX_FIRE1					85
#define SFX_FIRE2					86
#define SFX_FIRE3					87
#define SFX_FIRE4					88
#define SFX_FIRE5					89
#define SFX_FIRE6					90
#define SFX_FIRESMOKE				91
#define SFX_TRAILSMOKE				92
#define SFX_TRAILDUST				93
#define SFX_FIRE7					94
#define SFX_BLUE_CLOUD				95
#define SFX_WATER_FIREBALL			96
#define SFX_LINKED_PERSISTANT		97			// Added by KCK for AddSFXMessage
#define SFX_TIMED_PERSISTANT		98			// Added by KCK for AddSFXMessage
#define SFX_CLUSTER_BOMB			99			// Added by KCK for AddSFXMessage
#define SFX_SMOKING_FEATURE			100			// special type -- smoke on smokestacks
#define SFX_STEAMING_FEATURE		101			// special type -- smoke on smokestacks
#define SFX_STEAM_CLOUD				102
#define SFX_GROUND_FLASH			103
#define SFX_GROUND_GLOW				104
#define SFX_MESSAGE_TIMER			105
#define SFX_DURANDAL				106
#define SFX_CRATER2					107			// these craters are only used
#define SFX_CRATER3					108			// in ACMI
#define SFX_CRATER4					109
#define SFX_BIG_SMOKE				110
#define SFX_BIG_DUST				111
#define SFX_HIT_EXPLOSION_NOSMOKE	112
#define SFX_ROCKET_BURST			113
#define SFX_CAMP_HIT_EXPLOSION_DEBRISTRAIL           114
#define SFX_CAMP_FIRE           115
#define SFX_INCENDIARY_EXPLOSION           116
#define SFX_SPARK_TRACER           117
#define SFX_WATER_WAKE				118
#define SFX_NUM_TYPES				119

// special effects flags
#define SFX_MOVES					0x00000001
#define SFX_USES_GRAVITY			0x00000002
#define SFX_EXPLODE_WHEN_DONE		0x00000004
#define SFX_SECONDARY_DRIVER		0x00000008
#define SFX_NO_GROUND_CHECK			0x00000010
#define SFX_BOUNCES					0x00000020
#define SFX_BOUNCES_HARD			0x00000040
#define SFX_NO_DOWN_VECTOR			0x00000080
#define SFX_TIMER_FLAG				0x00000100
#define SFX_TRAJECTORY				0x00000200

#define SFX_F16CRASHLANDING			0x80000000
#define SFX_F16CRASH_OBJECT			0x40000000
#define SFX_F16CRASH_HITGROUND		0x20000000
#define SFX_F16CRASH_STOP			0x10000000
#define SFX_F16CRASH_ADJUSTANGLE	0x08000000
#define SFX_F16CRASH_SKIPGRAVITY	0x04000000


class SimBaseClass;
class RViewPoint;
class DrawableObject;
class DrawableTrail;
class Drawable2D;
class DrawableBSP;
class DrawableTracer;
class FalconDamageMessage;
class FalconMissileEndMessage;

class SfxClass
{

#ifdef USE_SH_POOLS
   public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert( size == sizeof(SfxClass) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(SfxClass), 200, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif


/////////////
// f16 crash landing
	protected:
	float RestPitch, RestRoll;
	Tpoint CrashSlot, CrashPos;

	static void CalculateRestingObjectMatrix (float pitch, float roll, float mat[3][3]);
	static void CalculateGroundMatrix (Tpoint *normal, float yaw, float mat[3][3]);
	static void MultiplyMatrix (float result[3][3], float mat[3][3], float mat1[3][3]);
	static void TransformPoint (Tpoint *result, Tpoint *point, float mat[3][3]);
	static void CopyMatrix (float result[3][3], float mat[3][3]);
	static void GetOrientation (float mat[3][3], float *yaw, float *pitch, float *roll);
	static float AdjustAngle180 (float angle);
	static int RestPiece (float *angle, float rest, float multiplier=0.125f, float min=0.02f );

   	public:
	SfxClass (	int typeSfx, int flagsSfx,SimBaseClass *baseobjSfx, 
				float timeToLiveSfx, float scaleSfx, 
				Tpoint *slot, float restpitch, float restroll);
/////////////

	protected:

	int	type;					// type of special effect
	int 	flags;				// modifiers
	Tpoint pos;					// world position
	Tpoint vec;					// world movement vector
	Trotation rot;				// orientation
	float timeToLive;			// in seconds
	float scale;				// size of effect
	float travelDist;			// distance to travel used by some sfx
	int secondaryCount;			// effects may start other effects
	int initSecondaryCount;		// initial count of secondaries
	float secondaryInterval;	// in seconds
	float secondaryTimer;		// in seconds
	float distTimer;			// in seconds
	float approxDist;			// to viewer position
	float lastACMItime;			// in seconds
	float startACMItime;			// in seconds
	FalconMissileEndMessage *endMessage;
	FalconDamageMessage *damMessage;
	// effect may be composed of one or more objects
	Drawable2D 	 *obj2d; 		// the object to draw
	DrawableTrail	 *objTrail; // the object to draw
	DrawableBSP	 *objBSP; 		// the object to draw
	DrawableTracer	 *objTracer; 	// the object to draw
	SimBaseClass	 *baseObj; 	// the object to draw

    RViewPoint* viewPoint;
	BOOL		inACMI;			// running from ACMI

	void GetApproxViewDist( float currTime );
	void RunSecondarySfx( void );
	void RunSfxCompletion( BOOL hitGround, float groundZ, int groundType );
	void GroundReflection( void );
	void StartRandomDebris( void );

   	public:

	  /*
	  ** overloaded sfx constructors
	  */

	  // Message timer
	  SfxClass( FalconMissileEndMessage *endM,
	  		  FalconDamageMessage *damM );

	  // non-moving effect
	  SfxClass( int type,
	  		  Tpoint *pos,
			  float timeToLive,
			  float scale );

	  // Secondary effect driver effect
	  SfxClass( int type,
	  		  Tpoint *pos,
			  int count,
			  float interval );

	  // Secondary effect driver effect with vector
	  SfxClass( int type,
	  		  Tpoint *pos,
	  		  Tpoint *vec,
			  int count,
			  float interval );

	  // moving effect
	  SfxClass( int type,
	  		  int flags,
	  		  Tpoint *pos,
	  		  Tpoint *vec,
			  float timeToLive,
			  float scale );

	  // moving effect with rotation
	  SfxClass( int type,
	  		  int flags,
	  		  Tpoint *pos,
	  		  Trotation *rot,
	  		  Tpoint *vec,
			  float timeToLive,
			  float scale );

	  // BSP effect may or may not move based on deltas and ypr deltas
	  SfxClass( int type,
	  		  int flags,
			  SimBaseClass *baseobj,
			  float timeToLive,
			  float scale );

	  // this just times the life of a trail then deletes it
	  SfxClass( float timeToLive,
			  DrawableTrail *trail );

	  // moving BSP
	  SfxClass( int type,
	  		  Tpoint *pos,
	  		  Tpoint *vec,
           DrawableBSP* theObject,
			  float timeToLive,
			  float scale );

	  // destructor
	  ~SfxClass( void );

	  // start executing function
	  void	Start( void );

	  // execute function
	  BOOL	Exec( void );
	  BOOL	Draw( void );

	  // for ACMI
	  // start executing function
	  void	ACMIStart( RViewPoint *acmiView, float startTime, float currTime );

	  // execute function
	  BOOL	ACMIExec( float currTime );

	  // set detail level
	  static void SetLOD( float objDetail );

	  // type accessor
	  int GetType( void )	{ return type; };
};

// externals
extern DWORD lastViewTime;
extern float sfxFrameTime;

// detail level of sfx ( 0.0 - 1.0 )
extern float gSfxLOD;

// global counters
extern int gTotSfx ;
extern int gSfxLODCutoff ;
extern int gSfxLODDistCutoff ;
extern int gSfxLODTotCutoff ;
extern int gTotHighWaterSfx ;
extern int gSfxCount[ SFX_NUM_TYPES ] ;
extern int gSfxHighWater[ SFX_NUM_TYPES ] ;

#endif
