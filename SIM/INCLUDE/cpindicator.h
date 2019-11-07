#ifndef _CPINDICATOR_H
#define _CPINDICATOR_H

#include "cpobject.h"

#ifdef USE_SH_POOLS
extern MEM_POOL gCockMemPool;
#endif


#define IND_VERTICAL		0
#define IND_HORIZONTAL	1

class CPObject;
//====================================================//
// Structures used for Initialization
//====================================================//

typedef struct {
					int	numTapes;
					float	minVal;
					int*	minPos;
					float	maxVal;
					int*	maxPos;
					int	orientation;
					RECT*	pdestRect;
					RECT*	psrcRect;
					int calibrationVal;
					} IndicatorInitStr;

//====================================================//
// CPObject Class defintion
//====================================================//

class CPIndicator : public CPObject {
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(gCockMemPool,size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
public:

	//====================================================//
	// Tape Properties
	//====================================================//

	int	mNumTapes;
	float	mMinVal;
	float	mMaxVal;
	int	mOrientation;

	float	*mPixelSlope;
	float	*mPixelIntercept;

	//====================================================//
	// Dimensions and Locations
	//====================================================//

	RECT				*mpDestRects;
	RECT				*mpDestLocs;
	RECT				*mpSrcLocs;
	RECT				*mpSrcRects;

	int				*mHeightTapeRect;
	int				*mWidthTapeRect;

	//====================================================//
	// Dimensions and Locations
	//====================================================//

	float				*mpTapeValues;
	int				mCalibrationVal;

	//====================================================//
	// Pointers to Runtime Member Functions
	//====================================================//

	virtual void Exec(SimBaseClass*);
	virtual void DisplayBlit(void);

	//====================================================//
	// Constructors and Destructors
	//====================================================//

	CPIndicator(ObjectInitStr*, IndicatorInitStr*);
	virtual ~CPIndicator();
};

#endif
