#ifndef _CPCHEV_H
#define _CPCHEV_H

#ifndef _WINDOWS_
#include <windows.h>
#endif
#include "stdhdr.h"
#include "cpobject.h"

#ifdef USE_SH_POOLS
extern MEM_POOL gCockMemPool;
#endif



typedef struct {
	float pan;
	float tilt;
	int	panLabel;
	int	tiltLabel;
} ChevronInitStr;

//====================================================//
// Structures used for Initialization
//====================================================//

class CPChevron : public CPObject
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(gCockMemPool,size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	struct ChevStruct {
		float x[3];	// three points
		float y[3]; // three points
	};

	float			pan, tilt;
	int			mNumCheverons;
	ChevStruct	mChevron[3];
	char			mString1[20];
	char			mString2[20];
	long			mColor[2];

public:

	CPChevron(ObjectInitStr*, ChevronInitStr*);
	virtual ~CPChevron();
	void	Exec(SimBaseClass*){};
	void DisplayDraw(void);
};

#endif
