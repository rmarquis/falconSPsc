#ifndef _CPKNEEVIEW_H
#define _CPKNEEVIEW_H

#ifndef _WINDOWS_
#include <windows.h>
#endif
#include "cpobject.h"


#ifdef USE_SH_POOLS
extern MEM_POOL gCockMemPool;
#endif




//====================================================//
// CPLight Class Definition
//====================================================//

class CPKneeView : public CPObject {
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(gCockMemPool,size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
public:

	KneeBoard* mpKneeBoard;

	//====================================================//
	// Runtime Member Functions
	//====================================================//

	virtual void	Exec(SimBaseClass*);
	virtual void	DisplayBlit(void);
	virtual void	DisplayDraw(void);

	//====================================================//
	// Constructors and Destructors
	//====================================================//

	CPKneeView(ObjectInitStr*, KneeBoard*);
	virtual ~CPKneeView();
};

#endif

