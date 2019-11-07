#ifndef _HARDPOINT_H
#define _HARDPOINT_H

class SimBaseClass;
class GunClass;
class DrawableBSP;
class SimWeaponClass;

// Public data types (used exclusively by AdvancedWeaponStation, however)
enum WeaponType { wtGuns, wtAim9, wtAim120, wtAgm88, wtAgm65, wtMk82, wtMk84, wtGBU, wtSAM, wtLAU, wtFixed, wtNone, wtGPS};
enum WeaponClass { wcAimWpn, wcRocketWpn, wcBombWpn, wcGunWpn, wcECM, wcTank, wcAgmWpn, wcHARMWpn, wcSamWpn, wcGbuWpn, wcCamera, wcNoWpn};
enum WeaponDomain { wdAir = 0x1, wdGround = 0x2, wdBoth = 0x3, wdNoDomain = 0};

typedef struct
	{
	int   flags;
	float cd;
	float weight;
	float area;
	float xEjection;
	float yEjection;
	float zEjection;
	char  mnemonic[8];
	WeaponClass weaponClass;
	WeaponDomain domain;
	} WeaponData;


// This is for ground vehicles and ships.
// They don't need a complex thingamabob
class BasicWeaponStation
{
#ifdef USE_SH_POOLS
   public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert( size == sizeof(BasicWeaponStation) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(BasicWeaponStation), 200, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif
	public:
		short			weaponId;
		short			weaponCount;
		SimWeaponClass	*weaponPointer;

	public:
		BasicWeaponStation(void);
		virtual ~BasicWeaponStation(void);

		virtual void Cleanup (void) {};

		// Virtualized access functions
		virtual void SetPosition (float, float, float)				{};
		virtual void SetRotation (float, float )						{};
		virtual void GetPosition (float*, float*, float*)				{};
		virtual void GetRotation (float*, float* )						{};
		virtual void SetSubPosition (int, float, float, float)		{};
		virtual void SetSubRotation (int, float, float )				{};
		virtual void GetSubPosition (int, float*, float*, float*)	{};
		virtual void GetSubRotation (int, float*, float* )			{};
		virtual void SetupPoints(int)									{};
		virtual int NumPoints(void)											{ return 0; };

		virtual DrawableBSP* GetRack(void)									{ return NULL; };
		virtual void SetRack (DrawableBSP*)							{};
		virtual int GetRackId (void)										{ return 0; };
		virtual void SetRackId (int)										{};
		virtual WeaponData* GetWeaponData (void)							{ return NULL; };
		virtual void SetWeaponData (WeaponData)							{};
		virtual WeaponType GetWeaponType (void)								{ return (WeaponType)0; };
		virtual void SetWeaponType (WeaponType)							{};
		virtual WeaponClass GetWeaponClass (void)							{ return (WeaponClass)0; };
      virtual WeaponDomain Domain (void)                          {return wdNoDomain;};
		virtual void SetWeaponClass (WeaponClass)						{};
		virtual GunClass* GetGun (void);
		virtual void SetGun (GunClass*)									{};
};

class AdvancedWeaponStation : public BasicWeaponStation
{
#ifdef USE_SH_POOLS
   public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert( size == sizeof(AdvancedWeaponStation) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(AdvancedWeaponStation), 200, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif
	private:
		float xPos;
		float yPos;
		float zPos;
		float az;
		float el;
		float* xSub;
		float* ySub;
		float* zSub;
		float* azSub;
		float* elSub;
		int numPoints;

		DrawableBSP* theRack;
		int rackId;
		GunClass* aGun;
		WeaponData weaponData;
		WeaponType weaponType;

	public:
		AdvancedWeaponStation(void);
		~AdvancedWeaponStation(void);

		virtual void Cleanup (void);

		// Virtualized access functions
		virtual void SetPosition (float x, float y, float z) { xPos = x; yPos = y; zPos = z;};
		virtual void SetRotation (float a, float e ) { az = a; el = e; };
		virtual void GetPosition (float* x, float* y, float* z){ *x = xPos; *y = yPos; *z = zPos;};
		virtual void GetRotation (float* a, float* e ) { *a = az; *e = el; };
		virtual void SetSubPosition (int i, float x, float y, float z) { xSub[i] = x; ySub[i] = y; zSub[i] = z;};
		virtual void SetSubRotation (int i, float a, float e ) { azSub[i] = a; elSub[i] = e; };
		virtual void GetSubPosition (int i, float* x, float* y, float* z){ *x = xSub[i]; *y = ySub[i]; *z = zSub[i];};
		virtual void GetSubRotation (int i, float* a, float* e ) { *a = azSub[i]; *e = elSub[i]; };
		virtual void SetupPoints(int num);
		virtual int NumPoints(void) { return numPoints; };

		virtual DrawableBSP* GetRack (void)									{ return theRack; };
		virtual void SetRack (DrawableBSP* rack)							{ theRack = rack; };
		virtual int GetRackId (void)										{ return rackId; };
		virtual void SetRackId (int id)										{ rackId = id; };
		virtual WeaponData* GetWeaponData (void)							{ return &weaponData; };
		virtual void SetWeaponData (WeaponData wd)							{ weaponData = wd; };
		virtual WeaponType GetWeaponType (void)								{ return weaponType; };
		virtual void SetWeaponType (WeaponType wt)							{ weaponType = wt; };
		virtual WeaponClass GetWeaponClass (void)							{ return weaponData.weaponClass; };
		virtual WeaponDomain Domain (void)							      { return weaponData.domain; };
		virtual void SetWeaponClass (WeaponClass wc)						{ weaponData.weaponClass = wc; };
		virtual GunClass* GetGun (void)										{ return aGun; };
		virtual void SetGun (GunClass* gun)									{ aGun = gun; };
};

#endif
