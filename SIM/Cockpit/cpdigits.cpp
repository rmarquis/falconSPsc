#include "stdafx.h"
#include "cpdigits.h"

//MI
extern bool g_bRealisticAvionics;

CPDigits::CPDigits(ObjectInitStr *pobjectInitStr, DigitsInitStr* pdigitsInitStr) : CPObject(pobjectInitStr)
{
	int i;

	mDestDigits		= pdigitsInitStr->numDestDigits;
	mpSrcRects		= pdigitsInitStr->psrcRects;
	mpDestRects		= pdigitsInitStr->pdestRects;

	for(i = 0; i < mDestDigits; i++) {
		mpDestRects[i].top		= mScale * mpDestRects[i].top;
		mpDestRects[i].left		= mScale * mpDestRects[i].left;
		mpDestRects[i].bottom	= mScale * mpDestRects[i].bottom;
		mpDestRects[i].right		= mScale * mpDestRects[i].right;	
	}

	mValue			= 0;

	#ifdef USE_SH_POOLS
	mpValues = (int *)MemAllocPtr(gCockMemPool,sizeof(int)*mDestDigits,FALSE);
	mpDestString = (char *)MemAllocPtr(gCockMemPool,sizeof(char)*(mDestDigits+1),FALSE);
	#else
	mpValues			= new int[mDestDigits];
	mpDestString	= new char[mDestDigits + 1];
	#endif

	memset(mpValues, 0, mDestDigits * sizeof(int));

	//MI
	active = TRUE;
}

CPDigits::~CPDigits()
{
	delete [] mpSrcRects;
	delete [] mpDestRects;
	delete [] mpValues;
	delete [] mpDestString;
}

void CPDigits::Exec(SimBaseClass* pOwnship) {

	mpOwnship = pOwnship;

	//get the value from ac model
	if(mExecCallback) {
		mExecCallback(this);
	}

	SetDirtyFlag(); //VWF FOR NOW
}

void CPDigits::DisplayBlit()
{
	int	i;
	mDirtyFlag = TRUE;

	if(!mDirtyFlag)
		return;

	//MI
	if(!g_bRealisticAvionics)
	{
		for(i = 0; i < mDestDigits; i++)
			mpOTWImage->Compose(mpTemplate, &mpSrcRects[mpValues[i]], &mpDestRects[i]);
	}
	else
	{
		for(i = 0; i < mDestDigits; i++)
		{
			if(active)
				mpOTWImage->Compose(mpTemplate, &mpSrcRects[mpValues[i]], &mpDestRects[i]);
		}
	}

	mDirtyFlag = FALSE;
}


void CPDigits::SetDigitValues(long value)
{
	int	i, j;
	int	fieldlen;

	if(mValue != value) {
		mValue = value;
		char tbuf[20]; // temporary copy - as it might be bigger
		fieldlen = sprintf(tbuf, "%ld", mValue);
		if (fieldlen > mDestDigits) { // fix up oversized values
		    strncpy (mpDestString, &tbuf[fieldlen-mDestDigits], mDestDigits);
		    fieldlen = mDestDigits;
		}
		else 
		    strncpy (mpDestString, tbuf, mDestDigits);

		memset(mpValues, 0, mDestDigits * sizeof(int));

		if(value > 0)
		{
		    for(i = mDestDigits - fieldlen, j = 0; i < mDestDigits; i++, j++) {
			ShiAssert(i >=0 && i < mDestDigits);
			mpValues[i] = mpDestString[j] - 0x30;
		    }
		}
	}
}
