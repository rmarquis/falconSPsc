#include "stdafx.h"
#include "cpindicator.h"

CPIndicator::CPIndicator(ObjectInitStr *pobjectInitStr, IndicatorInitStr* pindicatorInitStr) : CPObject(pobjectInitStr) {
	int	i;

	mNumTapes			= pindicatorInitStr->numTapes;
	mMinVal				= pindicatorInitStr->minVal;
	mMaxVal				= pindicatorInitStr->maxVal;
	mOrientation		= pindicatorInitStr->orientation;
	mCalibrationVal	= pindicatorInitStr->calibrationVal;
							
	#ifdef USE_SH_POOLS
	mpSrcLocs = (RECT *)MemAllocPtr(gCockMemPool,sizeof(RECT)*mNumTapes,FALSE);
	mpDestLocs = (RECT *)MemAllocPtr(gCockMemPool,sizeof(RECT)*mNumTapes,FALSE);
	mpDestRects = (RECT *)MemAllocPtr(gCockMemPool,sizeof(RECT)*mNumTapes,FALSE);
	mpSrcRects = (RECT *)MemAllocPtr(gCockMemPool,sizeof(RECT)*mNumTapes,FALSE);
	mpTapeValues = (float *)MemAllocPtr(gCockMemPool,sizeof(float)*mNumTapes,FALSE);
	mPixelSlope = (float *)MemAllocPtr(gCockMemPool,sizeof(float)*mNumTapes,FALSE);
	mPixelIntercept = (float *)MemAllocPtr(gCockMemPool,sizeof(float)*mNumTapes,FALSE);
	mHeightTapeRect = (int *)MemAllocPtr(gCockMemPool,sizeof(int)*mNumTapes,FALSE);
	mWidthTapeRect = (int *)MemAllocPtr(gCockMemPool,sizeof(int)*mNumTapes,FALSE);
	#else
	mpSrcLocs			= new RECT[mNumTapes];
	mpDestLocs			= new RECT[mNumTapes];
	mpDestRects			= new RECT[mNumTapes];
	mpSrcRects			= new RECT[mNumTapes];
	mpTapeValues		= new float[mNumTapes];
	mPixelSlope       = new float[mNumTapes];
	mPixelIntercept   = new float[mNumTapes];
	mHeightTapeRect   = new int[mNumTapes];
	mWidthTapeRect    = new int[mNumTapes];
	#endif

	for(i = 0; i < mNumTapes; i++) {
		mpSrcLocs[i]	= pindicatorInitStr->psrcRect[i];
		mpSrcRects[i]	= pindicatorInitStr->psrcRect[i];
		mpDestRects[i] = pindicatorInitStr->pdestRect[i];

		// Determine how many pixels represent a unit, (units/pixel)
		mPixelSlope[i]			= (float)(pindicatorInitStr->maxPos[i] - pindicatorInitStr->minPos[i])/ (float)(mMaxVal - mMinVal);
		mPixelIntercept[i]	= pindicatorInitStr->maxPos[i] - (mPixelSlope[i] * mMaxVal);

		if(mOrientation == IND_VERTICAL) {
			mHeightTapeRect[i]	= mpDestRects[i].bottom - mpDestRects[i].top + 1;
		}
		else {
			mWidthTapeRect[i]		= mpDestRects[i].right - mpDestRects[i].left + 1;
		}
	}
}

CPIndicator::~CPIndicator() {

	delete [] mpTapeValues;
	delete [] mpSrcLocs;
	delete [] mpSrcRects;
	delete [] mpDestLocs;
	delete [] mpDestRects;
	delete [] mPixelSlope;
	delete [] mPixelIntercept;
	delete [] mHeightTapeRect;
	delete [] mWidthTapeRect;
}

void CPIndicator::Exec(SimBaseClass* pOwnship) {

	float		tapePosition;
	int		i;

	mpOwnship = pOwnship;


	//get the value from ac model
	if(mExecCallback) {
		mExecCallback(this);
	}

	for(i = 0; i < mNumTapes; i++) {
		// Limit the current value
		if(mpTapeValues[i] > mMaxVal){
			mpTapeValues[i] = mMaxVal;
		}
		else if(mpTapeValues[i] < mMinVal){
			mpTapeValues[i] = mMinVal;
		}

		// Find the tape position
		tapePosition			= (mPixelSlope[i] * 	mpTapeValues[i]) + mPixelIntercept[i] + mCalibrationVal;

		if(mOrientation == IND_HORIZONTAL) {

			mpSrcLocs[i].left		= (int) tapePosition - (mWidthTapeRect[i]/2);
			mpSrcLocs[i].right	= (int) tapePosition + (mWidthTapeRect[i]/2);

			mpDestLocs[i]			= mpDestRects[i];

			if(mpSrcLocs[i].left < mpSrcRects[i].left) {

				mpSrcLocs[i].left = mpSrcRects[i].left;
				mpDestLocs[i].left = mpDestRects[i].left + (mpSrcRects[i].left - mpSrcLocs[i].left);
			}
			else if(mpSrcLocs[i].right > mpSrcRects[i].right) {

				mpSrcLocs[i].right = mpSrcRects[i].right;
				mpDestLocs[i].right = mpDestRects[i].right - (mpSrcLocs[i].right - mpSrcRects[i].right);
			}		
		}
		else {

			mpSrcLocs[i].top			= (int) tapePosition - (mHeightTapeRect[i]/2);
			mpSrcLocs[i].bottom		= (int) tapePosition + (mHeightTapeRect[i]/2);
			mpDestLocs[i]				= mpDestRects[i];

			if(mpSrcLocs[i].top	< mpSrcRects[i].top) {

				mpSrcLocs[i].top		= mpSrcRects[i].top;
				mpDestLocs[i].top		= mpDestRects[i].top + (mpSrcRects[i].top - mpSrcLocs[i].top);
			}
			else if(mpSrcLocs[i].bottom > mpSrcRects[i].bottom) {

				mpSrcLocs[i].bottom	= mpSrcRects[i].bottom;
				mpDestLocs[i].bottom = mpDestRects[i].bottom - (mpSrcLocs[i].bottom - mpSrcRects[i].bottom);
			}		
		}
	}

	SetDirtyFlag(); //VWF FOR NOW
}



void	CPIndicator::DisplayBlit(void) {
	int	i;
   RECT temp;

	mDirtyFlag = TRUE;

	if(!mDirtyFlag) {
		return;
	}

	for(i = 0; i < mNumTapes; i++) {
		temp = mpDestLocs[i];

		temp.top		= temp.top * mScale;
		temp.left	= temp.left * mScale;
		temp.bottom	= temp.top + mScale * (mpSrcLocs[i].bottom - mpSrcLocs[i].top);
		temp.right	= temp.left + mScale * (mpSrcLocs[i].right - mpSrcLocs[i].left);
		mpOTWImage->Compose(mpTemplate, &mpSrcLocs[i], &temp);
	}

	mDirtyFlag = FALSE;
}
