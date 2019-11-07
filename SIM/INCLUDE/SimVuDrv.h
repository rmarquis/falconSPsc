#ifndef _SIM_VUDRIVER_H
#define _SIM_VUDRIVER_H

#include "f4vu.h"
#include "vudriver.h"
#include "vuevent.h"

class SimVuDriver : public VuMaster
{
   public:
      SimVuDriver (VuEntity* theEnt) : VuMaster (theEnt) {};
      ~SimVuDriver (void) {};
      virtual VU_BOOL ExecModel(VU_TIME timestamp);
#ifdef USE_SH_POOLS
   public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert( size == sizeof(SimVuDriver) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(SimVuDriver), 100, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif
};


class SimVuSlave : public VuDelaySlave
{
   public:
      SimVuSlave(VuEntity *entity) : VuDelaySlave (entity) {};
      virtual ~SimVuSlave() {};
      virtual void Exec(VU_TIME timestamp);

	  virtual VU_ERRCODE Handle(VuPositionUpdateEvent *event);

#ifdef USE_SH_POOLS
   public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { ShiAssert( size == sizeof(SimVuSlave) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(SimVuSlave), 20, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif
};

#endif
