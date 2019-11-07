#include "stdafx.h"
#include "cpdial.h"
#include "Graphics\Include\renderow.h"
#include "otwdrive.h"
#include "Graphics\Include\tod.h"

CPDial::CPDial(ObjectInitStr *pobjectInitStr, DialInitStr* pdialInitStr) : CPObject(pobjectInitStr) {
	int	i;
   mlTrig trig;

	mSrcRect				= pdialInitStr->srcRect;
	mRadius0				= pdialInitStr->radius0;
	mRadius1				= pdialInitStr->radius1;
	mRadius2				= pdialInitStr->radius2;

	mColor[0][0]		= pdialInitStr->color0;
	mColor[0][1]		= pdialInitStr->color1;
	mColor[0][2]		= pdialInitStr->color2;

	mColor[1][0]		= CalculateNVGColor(pdialInitStr->color0);
	mColor[1][1]		= CalculateNVGColor(pdialInitStr->color1);
	mColor[1][2]		= CalculateNVGColor(pdialInitStr->color2);

	mxCenter				= mSrcRect.left + mWidth / 2;
	myCenter				= mSrcRect.top	+ mHeight / 2;

	mEndPoints			= pdialInitStr->endPoints;
	mpValues				= pdialInitStr->pvalues;
	mpPoints				= pdialInitStr->ppoints;

	mDialValue			= 0.0F;

	#ifdef USE_SH_POOLS
	mpCosPoints = (float *)MemAllocPtr(gCockMemPool,sizeof(float)*mEndPoints,FALSE);
	mpSinPoints = (float *)MemAllocPtr(gCockMemPool,sizeof(float)*mEndPoints,FALSE);
	#else
	mpCosPoints			= new float[mEndPoints];
	mpSinPoints			= new float[mEndPoints];
	#endif
	for(i = 0; i < mEndPoints; i++) {
      mlSinCos (&trig, mpPoints[i]);

		mpCosPoints[i]	= trig.cos;
		mpSinPoints[i] = trig.sin;
	}
}

CPDial::~CPDial(){

	delete [] mpValues;
	delete [] mpPoints;
	delete [] mpSinPoints;
	delete [] mpCosPoints;
}

void CPDial::Exec(SimBaseClass* pOwnship){
	BOOL	found = FALSE;
	int		i = 0;
	int		xOffset0=0;
	int		yOffset0=0;
	int		xOffset1=0;
	int		yOffset1=0;
	int		xOffset2=0;
	int		yOffset2=0;
	int		xOffset3=0;
	int		yOffset3=0;
	float	delta=0.0F;
	float	slope=0.0F;
	float	deflection=0.0F;
	float	cosDeflection=0.0F;
	float	sinDeflection=0.0F;
	float	cosSecondDeflection=0.0F;
	float	sinSecondDeflection=0.0F;
	mlTrig  trig;

	mpOwnship = pOwnship;

	//get the value from ac model
	if(mExecCallback) {
		mExecCallback(this);
	}

	do {
		if(mDialValue <= mpValues[0]) {
			found		= TRUE;
			xOffset0	= FloatToInt32	 (mRadius0 * mpCosPoints[0]);
			yOffset0	= -FloatToInt32 (mRadius0 * mpSinPoints[0]);
			xOffset1	= FloatToInt32	 (mRadius1 * mpCosPoints[0]);
			yOffset1	= -FloatToInt32 (mRadius1 * mpSinPoints[0]);

         mlSinCos (&trig, mpPoints[0] + 0.7854F);
			cosSecondDeflection = trig.cos;
			sinSecondDeflection = trig.sin;

			xOffset2 = FloatToInt32  (mRadius2 * trig.cos);
			yOffset2 = -FloatToInt32 (mRadius2 * trig.sin);

         mlSinCos (&trig, mpPoints[0] - 0.7854F);
			xOffset3 = FloatToInt32  (mRadius2 * trig.cos);
			yOffset3 = -FloatToInt32 (mRadius2 * trig.sin);
		}
		else if(mDialValue >= mpValues[mEndPoints - 1]) {
			found		= TRUE;
			xOffset0	= FloatToInt32	(mRadius0 * mpCosPoints[mEndPoints - 1]);
			yOffset0	= -FloatToInt32 (mRadius0 * mpSinPoints[mEndPoints - 1]);
			xOffset1	= FloatToInt32	(mRadius1 * mpCosPoints[mEndPoints - 1]);
			yOffset1	= -FloatToInt32 (mRadius1 * mpSinPoints[mEndPoints - 1]);

         mlSinCos (&trig, mpPoints[mEndPoints - 1] + 0.7854F);
			cosSecondDeflection = trig.cos;
			sinSecondDeflection = trig.sin;

			xOffset2 = FloatToInt32  (mRadius2 * trig.cos);
			yOffset2 = -FloatToInt32 (mRadius2 * trig.sin);

         mlSinCos (&trig, mpPoints[mEndPoints - 1] - 0.7854F);
			xOffset3 = FloatToInt32  (mRadius2 * trig.cos);
			yOffset3 = -FloatToInt32 (mRadius2 * trig.sin);


		}
		else if((mDialValue >= mpValues[i]) && (mDialValue < mpValues[i + 1])){
			found = TRUE;

			delta = mpPoints[i + 1] - mpPoints[i];
			if(delta > 0.0F){
				delta -= (2*PI);
			}
			
			slope			= delta / (mpValues[i + 1] - mpValues[i]);
			deflection	= (float)(mpPoints[i] + (slope * (mDialValue - mpValues[i])));

			if(deflection < -PI){
				deflection += (2*PI);
			}
			else if(deflection > PI){
				deflection -= (2*PI);
			}

         mlSinCos (&trig, deflection);
			cosDeflection	= trig.cos;
			sinDeflection	= trig.sin;
         mlSinCos (&trig, deflection + 0.7854F);
			cosSecondDeflection	= trig.cos;
			sinSecondDeflection	= trig.sin;


			xOffset0 = FloatToInt32	 (mRadius0 * cosDeflection);
			yOffset0 = -FloatToInt32 (mRadius0 * sinDeflection);
			xOffset1 = FloatToInt32	 (mRadius1 * cosDeflection);
			yOffset1 = -FloatToInt32 (mRadius1 * sinDeflection);

			xOffset2 = FloatToInt32  (mRadius2 * cosSecondDeflection);
			yOffset2 = -FloatToInt32 (mRadius2 * sinSecondDeflection);

         mlSinCos (&trig, deflection - 0.7854F);
			xOffset3 = FloatToInt32  (mRadius2 * trig.cos);
			yOffset3 = -FloatToInt32 (mRadius2 * trig.sin);

		}
		if(found){

				mxPos0 = (mxCenter + xOffset0) * mScale;
				myPos0 = (myCenter + yOffset0) * mScale;

				mxPos1 = (mxCenter + xOffset1) * mScale;
				myPos1 = (myCenter + yOffset1) * mScale;

				mxPos2 = (mxCenter + xOffset2) * mScale;
				myPos2 = (myCenter + yOffset2) * mScale;

				mxPos3 = (mxCenter + xOffset3) * mScale;
				myPos3 = (myCenter + yOffset3) * mScale;
		}
		else {
			i++;
		}
	} while((!found) && (i < mEndPoints));

	SetDirtyFlag(); //VWF FOR NOW
}

void CPDial::DisplayDraw(){

	mDirtyFlag = TRUE;

	if(!mDirtyFlag) {
		return;
	}

	OTWDriver.renderer->SetViewport(-1.0F, 1.0F, 1.0F, -1.0F);
	OTWDriver.renderer->SetColor(mColor[TheTimeOfDay.GetNVGmode() != 0][1]);
	OTWDriver.renderer->Render2DTri((float)mxPos0, (float)myPos0, (float)mxPos1,
      (float)myPos1, (float)mxPos2, (float)myPos2);
	OTWDriver.renderer->SetColor(mColor[TheTimeOfDay.GetNVGmode() != 0][2]);
	OTWDriver.renderer->Render2DTri((float)mxPos0, (float)myPos0, (float)mxPos1,
      (float)myPos1, (float)mxPos3, (float)myPos3);

	mDirtyFlag = FALSE;
}
