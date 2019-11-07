#ifndef _SIM_FEATURE_H
#define _SIM_FEATURE_H

#include "simstatc.h"

struct Tpoint;
class FeatureBrain;
class DrawableObject;

class SimFeatureClass : public SimStaticClass
{
   protected:
      FeatureBrain* theBrain;

   public:
      SimFeatureClass (VU_BYTE** stream);
      SimFeatureClass (FILE* filePtr);
      SimFeatureClass (int type);
      virtual ~SimFeatureClass (void);
      void InitData (void);
      void Init (SimInitDataClass* initData);
      int  Wake (void);
      int  Sleep (void);
      void ApplyDamage (FalconDamageMessage *damageMessage);
      void JoinFlight (void);
      FeatureBrain* Brain(void) {return theBrain;};
	  static BOOL ApplyChainReaction ( Tpoint *pos, float radius );

	  virtual float	GetIRFactor(void)	{ return 5.0f; };
      virtual int	GetRadarType(void);

      DrawableObject* baseObject;
      long featureFlags;
      virtual VU_ERRCODE InsertionCallback(void);


#ifdef USE_SH_POOLS
   public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert( size == sizeof(SimFeatureClass) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(SimFeatureClass), 200, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif
};

#endif
