#ifndef _BOMB_H
#define _BOMB_H

#include "simweapn.h"

// Forward declarations for class pointers
class SimInitDataClass;
class DrawableTrail;
class Drawable2D;
class GuidanceClass;

class BombClass : public SimWeaponClass
{
#ifdef USE_SH_POOLS
   public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert( size == sizeof(BombClass) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(BombClass), 200, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif

   public:
      enum BombType  {None, Chaff, Flare, Debris};

      enum {
         FirstFrame = 	0x00000001,
		 NeedTrail = 	0x00000002,
		 IsChaff = 		0x00000004,
		 IsDebris = 	0x00000008,
		 IsFlare = 		0x00000010,
       	 IsLGB =   		0x00000020,
       	 IsDurandal =   0x00000040,
       	 FireDurandal = 0x00000080,
		 IsGPS =		0x00000100,	//MI GPS
      };

	  static float dragConstant;

   protected:
		GuidanceClass* guidance;
      int displayIndex;
      int bombType;
      void InitData (void);
      virtual void UpdateTrail (void);
      virtual void RemoveTrail (void);
      virtual void InitTrail (void);
		virtual void DoExplosion (void);
      void ApplyProximityDamage( float groundZ, float detonateHeight );
      virtual void ExtraGraphics (void);
      virtual void SpecialGraphics (void);
// 2001-04-17 NOT USED SO I'LL RENAME THEM TO SOMETHING USEFULL FOR ME
//    float tgtX, tgtY, tgtZ;
      float desDxPrev, desDyPrev, desDzPrev;
      float burstHeight;
      float detonateHeight;
      float dragCoeff;
      SimBaseClass *hitObj;
      int flags;

   public:
      BombClass (VU_BYTE** stream);
      BombClass (FILE* filePtr);
      BombClass (int type, BombType = None);
      virtual ~BombClass (void);
      virtual int SaveSize();
      virtual int Save(VU_BYTE **stream);	// returns bytes written
      virtual int Save(FILE *file);		// returns bytes written
      virtual int Wake (void);
      virtual int Sleep (void);
      float x, y, z;
      float edeltaX, edeltaY, edeltaZ;
      float psi, theta, phi;
      virtual void Start(vector* pos, vector* rate, float cd, SimObjectType *targetPtr = NULL);
      void Init (void);
      virtual void Init (SimInitDataClass* initData);
      int Exec (void);
      void GetTransform(TransformMatrix vmat);
      void SetTarget (SimObjectType* newTarget);
      virtual void SetVuPosition (void);
      void SetBurstHeight (float newHeight) {burstHeight = newHeight;};
      virtual int IsBomb (void) { return TRUE; };
      void SetBombFlag (int newFlag) {flags |= newFlag;};
      void ClearBombFlag (int newFlag) {flags &= ~newFlag;};
      int IsSetBombFlag (int newFlag) {return flags & newFlag ? TRUE : FALSE;};
};

#endif
