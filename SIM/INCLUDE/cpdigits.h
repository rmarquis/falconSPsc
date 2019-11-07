#ifndef _CPDIGITS_H
#define _CPDIGITS_H

#include "cpobject.h"

#ifdef USE_SH_POOLS
extern MEM_POOL gCockMemPool;
#endif


class CPObject;
//====================================================//
// Structures used for Initialization
//====================================================//

typedef struct {
					RECT*	psrcRects;
					int	numDestDigits;
					RECT*	pdestRects;
} DigitsInitStr;

//====================================================//
// CPObject Class defintion
//====================================================//

class CPDigits : public CPObject {
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(gCockMemPool,size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
public:

	long				mValue;
	int				*mpValues;
	//====================================================//
	// Dimensions and Locations
	//====================================================//

	RECT				*mpDestRects;
	RECT				*mpSrcRects;
	int				mDestDigits;
	char				*mpDestString;

	//====================================================//
	// Pointers to Runtime Member Functions
	//====================================================//

	void				Exec(void) {};
	void				Exec(SimBaseClass*);
	void				DisplayBlit(void);
	void				DisplayDraw(void){};
	void				SetDigitValues(long);
	//MI
	bool				active;

	//====================================================//
	// Constructors and Destructors
	//====================================================//

	CPDigits(ObjectInitStr*, DigitsInitStr*);
	virtual ~CPDigits();
};

#endif
