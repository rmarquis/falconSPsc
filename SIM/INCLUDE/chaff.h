#ifndef _CHAFF_H
#define _CHAFF_H

#include "bomb.h"

class ChaffClass : public BombClass
{
#ifdef USE_SH_POOLS
   public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert( size == sizeof(ChaffClass) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(ChaffClass), 200, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif

   private:
      void InitData (void);
      virtual void UpdateTrail (void);
      virtual void RemoveTrail (void);
      virtual void InitTrail (void);
      virtual void ExtraGraphics (void);
      virtual void DoExplosion (void);
      virtual void SpecialGraphics (void);

   public:
      ChaffClass (VU_BYTE** stream);
      ChaffClass (FILE* filePtr);
      ChaffClass (int type);
      virtual ~ChaffClass (void);
      virtual int SaveSize();
      virtual int Save(VU_BYTE **stream);	// returns bytes written
      virtual int Save(FILE *file);		// returns bytes written
      virtual int Wake (void);
      virtual int Sleep (void);
      virtual void Init (SimInitDataClass* initData);
      virtual void Init (void);
      virtual int Exec (void);
      virtual void Start(vector* pos, vector* rate, float cd);
};

#endif
