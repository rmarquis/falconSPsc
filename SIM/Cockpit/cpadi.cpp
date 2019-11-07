#include "stdafx.h"
#include "falclib.h"
#include "dispcfg.h"
#include "cpadi.h"
#include "simbase.h"
#include "cphsi.h"
#include "dispcfg.h"
#include "Graphics\Include\grinline.h"
#include "dispopts.h"
#include "flightData.h"
#include "Graphics\Include\renderow.h"
#include "otwdrive.h"
#include "Graphics\Include\tod.h"
#include "FalcLib\include\playerop.h"
#include "aircrft.h"	//MI
#include "simdrive.h"	//MI

extern bool g_bRealisticAvionics;	//MI
extern bool g_bINS;	//MI

//====================================================//
// CPAdi::CPAdi
//====================================================//

CPAdi::CPAdi(ObjectInitStr *pobjectInitStr, ADIInitStr *padiInitStr) : CPObject(pobjectInitStr) {
	
	int				i;
	float				x;
	float				y;
	float				radiusSquared;
	int				arraySize;
	int				halfArraySize;
	float				halfCockpitWidth;
	float				halfCockpitHeight;

	//MI inialize
	mPitch = 0.0F;
	mRoll = 0.0F;

	mColor[0][0]	= padiInitStr->color0; //0xFF20A2C8;
	mColor[1][0]	= CalculateNVGColor(mColor[0][0]);
	mColor[0][1]	= padiInitStr->color1; //0xff808080;
	mColor[1][1]	= CalculateNVGColor(mColor[0][1]);
	mColor[0][2]	= padiInitStr->color2; //0xffffffff;
	mColor[1][2]	= CalculateNVGColor(mColor[0][2]);
	mColor[0][3]	= padiInitStr->color3; //0xFF6CF3F3;	ILS Bars, light yellow
	mColor[1][3]	= CalculateNVGColor(mColor[0][3]);
	mColor[0][4]	= padiInitStr->color4; //0xFF6CF3F3;	A/C reference symbol, light yellow
	mColor[1][4]	= CalculateNVGColor(mColor[0][4]);
	

 	mSrcRect			= padiInitStr->srcRect;
	mILSLimits		= padiInitStr->ilsLimits;

	mSrcHalfHeight	= (mSrcRect.bottom - mSrcRect.top + 1) / 2;

	mMinPitch		= -89.9F * DTR;
	mMaxPitch		= 89.9F * DTR;
	mTanVisibleBallHalfAngle = (float)tan(25.0F * DTR);


	// Setup the circle limits
	mRadius			= max(mWidth, mHeight);
	mRadius			= (mRadius + 1)/ 2;
	arraySize		= (int)mRadius * 4;

	#ifdef USE_SH_POOLS
	mpADICircle = (int *)MemAllocPtr(gCockMemPool,sizeof(int)*arraySize,FALSE);
	#else
 	mpADICircle		= new int[arraySize];
	#endif

	radiusSquared	= (float) mRadius * mRadius;

	halfArraySize	= arraySize / 2;
	for(i = 0; i < halfArraySize; i++) {

		y				= (float) abs(i - mRadius);
		x				= (float)sqrt(radiusSquared - y * y );
		mpADICircle[i * 2 + 1]	= mRadius + (int)x; //right
		mpADICircle[i * 2 + 0]	= mRadius - (int)x; //left
	}

	halfCockpitWidth	= (float) DisplayOptions.DispWidth * 0.5F;
	halfCockpitHeight	= (float) DisplayOptions.DispHeight * 0.5F;

	mLeft				= (mDestRect.left - halfCockpitWidth) / halfCockpitWidth;
   mRight			= (mDestRect.right - halfCockpitWidth) / halfCockpitWidth;
   mTop				= -(mDestRect.top - halfCockpitHeight) / halfCockpitHeight;
   mBottom			= -(mDestRect.bottom - halfCockpitHeight) / halfCockpitHeight;

	mpAircraftBarData[0][0] = 0.1F; mpAircraftBarData[0][1] = 0.5F;
	mpAircraftBarData[1][0] = 0.3F; mpAircraftBarData[1][1] = 0.5F;
	mpAircraftBarData[2][0] = 0.4F; mpAircraftBarData[2][1] = 0.6F;
	mpAircraftBarData[3][0] = 0.5F; mpAircraftBarData[3][1] = 0.5F;
	mpAircraftBarData[4][0] = 0.6F; mpAircraftBarData[4][1] = 0.6F;
	mpAircraftBarData[5][0] = 0.7F; mpAircraftBarData[5][1] = 0.5F;
	mpAircraftBarData[6][0] = 0.9F; mpAircraftBarData[6][1] = 0.5F;

	for(i = 0; i < NUM_AC_BAR_POINTS; i++) {
		mpAircraftBar[i][0] =  (unsigned) (mpAircraftBarData[i][0] * (float) mWidth * mScale) + mDestRect.left;
		mpAircraftBar[i][1] =  (unsigned) (mpAircraftBarData[i][1] * (float) mHeight * mScale) + mDestRect.top;
	}

	mLeftLimit		= mScale * mILSLimits.left;
	mRightLimit		= mScale * mILSLimits.right;
	mTopLimit		= mScale * mILSLimits.top;
	mBottomLimit	= mScale * mILSLimits.bottom;

	mHorizScale		= (float)(mRightLimit - mLeftLimit) / HORIZONTAL_SCALE;
	mVertScale		= (float)(mBottomLimit - mTopLimit) / VERTICAL_SCALE;

	mHorizCenter	= (unsigned) (abs(int(mRightLimit - mLeftLimit)) * 0.5 + mLeftLimit);
	mVertCenter		= (unsigned) (abs(int(mBottomLimit - mTopLimit)) * 0.5 + mTopLimit);

	mHorizBarPos	= mTopLimit;
	mVertBarPos		= mLeftLimit;

	mDoBackRect			= padiInitStr->doBackRect;


	mBackSrc.top		= padiInitStr->backSrc.top;
	mBackSrc.left		= padiInitStr->backSrc.left;
	mBackSrc.bottom	= padiInitStr->backSrc.bottom;
	mBackSrc.right		= padiInitStr->backSrc.right;

	mBackDest.top		= mScale * padiInitStr->backDest.top;
	mBackDest.left		= mScale * padiInitStr->backDest.left;
	mBackDest.bottom	= mScale * (padiInitStr->backDest.bottom + 1);
	mBackDest.right	= mScale * (padiInitStr->backDest.right + 1);

	mpSourceBuffer		= padiInitStr->pBackground;
	mpSurfaceBuffer	= NULL; 

	//MI
	Persistant = pobjectInitStr->persistant;
	LastMainADIPitch = 0.0F;
	LastMainADIRoll = 0.0F;
	LastBUPPitch = 0.0F;
	LastBUPRoll = 0.0F;
}

//====================================================//
// CPAdi::~CPAdi
//====================================================//

CPAdi::~CPAdi(void) {

	delete [] mpADICircle;

	if(mpSurfaceBuffer) {
		mpSurfaceBuffer->Cleanup();	
		delete mpSurfaceBuffer;	
	}
	
	if(mDoBackRect) {
		glReleaseMemory((char*) mpSourceBuffer);
	}
}



//====================================================//
// CPAdi::CreateLit
//====================================================//

void CPAdi::CreateLit(void)
{
	const DWORD dwMaxTextureWidth = mpOTWImage->GetDisplayDevice()->GetDefaultRC()->m_pD3DHWDeviceDesc->dwMaxTextureWidth;
	const DWORD dwMaxTextureHeight = mpOTWImage->GetDisplayDevice()->GetDefaultRC()->m_pD3DHWDeviceDesc->dwMaxTextureHeight;

	// revert to legacy rendering if we can't use a single texture even if Fast 2D Cockpit is active
	if(!PlayerOptions.bFast2DCockpit ||
		(PlayerOptions.bFast2DCockpit &&
			(mDoBackRect && (dwMaxTextureWidth < mBackSrc.right || dwMaxTextureHeight < mBackSrc.bottom))))
	{
		if(mDoBackRect)
		{
			mpSurfaceBuffer = new ImageBuffer;

			MPRSurfaceType front = (FalconDisplay.theDisplayDevice.IsHardware() && PlayerOptions.bFast2DCockpit) ? LocalVideoMem : SystemMem;
			if(!mpSurfaceBuffer->Setup(&FalconDisplay.theDisplayDevice, mBackSrc.right, mBackSrc.bottom, front, None) && front == LocalVideoMem)
			{
				// Retry with system memory if ouf video memory
				#ifdef _DEBUG
				OutputDebugString("CPAdi::CreateLit - Probably out of video memory. Retrying with system memory)\n");
				#endif

				BOOL bResult = mpSurfaceBuffer->Setup(&FalconDisplay.theDisplayDevice, mBackSrc.right, mBackSrc.bottom, SystemMem, None);
				if(!bResult) return;
			}

			mpSurfaceBuffer->SetChromaKey(0xFFFF0000);
		}

		else
			mpSurfaceBuffer = NULL;
	}

	else
	{
		try
		{
			if(mDoBackRect)
			{
				m_pPalette = new PaletteHandle(mpOTWImage->GetDisplayDevice()->GetDefaultRC()->m_pDD, 32, 256);
				if(!m_pPalette)
					throw _com_error(E_OUTOFMEMORY);

				TextureHandle *pTex = new TextureHandle;
				if(!pTex)
					throw _com_error(E_OUTOFMEMORY);

				m_pPalette->AttachToTexture(pTex);
				if(!pTex->Create("CPAdi", MPR_TI_PALETTE | MPR_TI_CHROMAKEY, 8, mBackSrc.right, mBackSrc.bottom))
					throw _com_error(E_FAIL);

				if(!pTex->Load(0, 0xFFFF0000, (BYTE*) mpSourceBuffer, true, true))	// soon to be re-loaded by CPObject::Translate3D
					throw _com_error(E_FAIL);

				m_arrTex.push_back(pTex);
			}

			else
				mpSurfaceBuffer = NULL;
		}

		catch(_com_error e)
		{
			MonoPrint("CPAdi::CreateLit - Error 0x%X (%s)\n", e.Error(), e.ErrorMessage());
			DiscardLit();
		}
	}
}

//====================================================//
// CPAdi::DiscardLit
//====================================================//

void CPAdi::DiscardLit(void)
{
	if(mDoBackRect)
	{
		if(mpSurfaceBuffer)
			mpSurfaceBuffer->Cleanup();

		delete mpSurfaceBuffer;
		mpSurfaceBuffer = NULL;
	}

	CPObject::DiscardLit();
}

//====================================================//
// CPAdi::Exec
//====================================================//

void CPAdi::Exec(SimBaseClass *pSimBaseClass)
{
	float		PercentHalfXscale;

	mpOwnship	= pSimBaseClass;
	//MI
	if(g_bRealisticAvionics && g_bINS)
	{
		if(SimDriver.playerEntity && Persistant == 1)	//backup ADI, continue to function until out of energy
		{
			if(SimDriver.playerEntity->INSState(AircraftClass::BUP_ADI_OFF_IN))
			{
				//make a check for the BUP ADI energy here when ready
				mPitch	= cockpitFlightData.pitch;
				mRoll	= cockpitFlightData.roll;
				LastBUPPitch = mPitch;
				LastBUPRoll = mRoll;
			}
			else
			{
				mPitch = LastBUPPitch;
				mRoll = LastBUPRoll;
			}
		}
		else if(SimDriver.playerEntity && !SimDriver.playerEntity->INSState(AircraftClass::INS_ADI_OFF_IN))
		{
			//stay where you currently are
			mPitch = LastMainADIPitch;
			mRoll = LastMainADIRoll;
		}
		else
		{
			mPitch	= cockpitFlightData.pitch;
			mRoll	= cockpitFlightData.roll;
			LastMainADIPitch = mPitch;
			LastMainADIRoll = mRoll;
		}
	}
	else
	{
		mPitch	= cockpitFlightData.pitch;
		mRoll	= cockpitFlightData.roll;
	}

	// Bound the pitch angle so we don't go beyond +/- 30 deg for now
	mPitch		= max(mPitch, mMinPitch);
	mPitch		= min(mPitch, mMaxPitch);

	// Compute the slide distance based on the pitch angle
	PercentHalfXscale		= mPitch / (25.0F * DTR); //tan(mPitch) / mTanVisibleBallHalfAngle;
	mSlide					= mRadius * (float)PercentHalfXscale;

	if(gNavigationSys)
		ExecILS();

	SetDirtyFlag(); //VWF FOR NOW
}

//====================================================//
// CPAdi::ExecILS
//====================================================//

void CPAdi::ExecILS()
{
	float		gpDeviation;
	float		gsDeviation;

	if(mpCPManager->mHiddenFlag && (gNavigationSys->GetInstrumentMode() == NavigationSystem::TACAN || gNavigationSys->GetInstrumentMode() == NavigationSystem::NAV))
		return;

	if((gNavigationSys->GetInstrumentMode() == NavigationSystem::ILS_TACAN ||
		gNavigationSys->GetInstrumentMode() == NavigationSystem::ILS_NAV) &&
		gNavigationSys->GetILSAttribute(NavigationSystem::GP_DEV, &gpDeviation))
	{
		gNavigationSys->GetILSAttribute(NavigationSystem::GS_DEV, &gsDeviation);

		gpDeviation							= min(max(gpDeviation, -HORIZONTAL_SCALE * 0.5F), HORIZONTAL_SCALE * 0.5F);
		gsDeviation							= min(max(gsDeviation, -VERTICAL_SCALE * 0.5F), VERTICAL_SCALE * 0.5F);

		mpCPManager->ADIGpDevReading	= mpCPManager->ADIGpDevReading + 0.1F * (gpDeviation - mpCPManager->ADIGpDevReading);
		mpCPManager->ADIGsDevReading	= mpCPManager->ADIGsDevReading + 0.1F * (gsDeviation - mpCPManager->ADIGsDevReading);

		mpCPManager->ADIGpDevReading	= min(max(mpCPManager->ADIGpDevReading, -HORIZONTAL_SCALE * 0.5F), HORIZONTAL_SCALE * 0.5F);
		mpCPManager->ADIGsDevReading	= min(max(mpCPManager->ADIGsDevReading, -VERTICAL_SCALE * 0.5F), VERTICAL_SCALE * 0.5F);

		mVertBarPos							= (unsigned) abs(FloatToInt32(mHorizScale * mpCPManager->ADIGpDevReading + mHorizCenter));		// Calc location of deviation bars
		mHorizBarPos						= (unsigned) abs(FloatToInt32(mVertScale * -mpCPManager->ADIGsDevReading + mVertCenter));

		mVertBarPos							= max(mVertBarPos, mLeftLimit);						// Bound the positions
		mVertBarPos							= min(mVertBarPos, mRightLimit);

		mHorizBarPos						= min(mHorizBarPos, mBottomLimit);
		mHorizBarPos						= max(mHorizBarPos, mTopLimit);

		mpCPManager->mHiddenFlag = FALSE;
	}

	else
	{
		if(mpCPManager->mHiddenFlag)
			return;

		else if(mVertBarPos <= mDestRect.left && mHorizBarPos >= mDestRect.bottom)
		{
			mpCPManager->mHiddenFlag		= TRUE;
			mVertBarPos							= mDestRect.left;
			mHorizBarPos						= mDestRect.bottom;
			mpCPManager->ADIGpDevReading	=	-HORIZONTAL_SCALE;
			mpCPManager->ADIGsDevReading	=	-VERTICAL_SCALE;
		}

		else
		{
			mpCPManager->ADIGpDevReading	= mpCPManager->ADIGpDevReading + 0.1F * (-HORIZONTAL_SCALE * 1.5F - mpCPManager->ADIGpDevReading);
			mpCPManager->ADIGsDevReading	= mpCPManager->ADIGsDevReading + 0.1F * (-VERTICAL_SCALE * 1.5F - mpCPManager->ADIGsDevReading);

			mVertBarPos				= (unsigned) abs(FloatToInt32(mHorizScale * mpCPManager->ADIGpDevReading + mHorizCenter));		// Calc location of deviation bars
			mHorizBarPos			= (unsigned) abs(FloatToInt32(mVertScale * -mpCPManager->ADIGsDevReading + mVertCenter));

			mVertBarPos				= max(mVertBarPos, mDestRect.left);						// Bound the positions
			mVertBarPos				= min(mVertBarPos, mDestRect.right);

			mHorizBarPos			= min(mHorizBarPos, mDestRect.bottom);
			mHorizBarPos			= max(mHorizBarPos, mDestRect.top);
		}
	}
}

//====================================================//
// CPAdi::ExecILSNone
//====================================================//

void CPAdi::ExecILSNone()
{
	mHorizBarPos		= mTopLimit;
	mVertBarPos			= mLeftLimit;
}

//====================================================//
// CPAdi::DisplayBlit
//====================================================//

void CPAdi::DisplayBlit3D()
{
	if(!mDirtyFlag)
		return;

	if(!m_arrTex.size())
		return;		// handled in DisplayBlit

	if(mDoBackRect)
	{
		TextureHandle *pTex = m_arrTex[0];

		// Setup vertices
		float fStartU = 0;
		float fMaxU = (float) pTex->m_nWidth / (float) pTex->m_nActualWidth;
		fMaxU -= fStartU;

		float fStartV = 0;
		float fMaxV = (float) pTex->m_nHeight / (float) pTex->m_nActualHeight;
		fMaxV -= fStartV;

		TwoDVertex pVtx[4];
		ZeroMemory(pVtx, sizeof(pVtx));
		pVtx[0].x = mBackDest.left; pVtx[0].y = mBackDest.top; pVtx[0].u = fStartU; pVtx[0].v = fStartV;
		pVtx[0].r = pVtx[0].g = pVtx[0].b = pVtx[0].a = 1.0f;
		pVtx[1].x = mBackDest.right; pVtx[1].y = mBackDest.top; pVtx[1].u = fMaxU; pVtx[1].v = fStartV;
		pVtx[1].r = pVtx[1].g = pVtx[1].b = pVtx[1].a = 1.0f;
		pVtx[2].x = mBackDest.right; pVtx[2].y = mBackDest.bottom; pVtx[2].u = fMaxU; pVtx[2].v = fMaxV;
		pVtx[2].r = pVtx[2].g = pVtx[2].b = pVtx[2].a = 1.0f;
		pVtx[3].x = mBackDest.left; pVtx[3].y = mBackDest.bottom; pVtx[3].u = fStartU; pVtx[3].v = fMaxV;
		pVtx[3].r = pVtx[3].g = pVtx[3].b = pVtx[3].a = 1.0f;

		// Setup state
		OTWDriver.renderer->context.RestoreState(STATE_TEXTURE_TRANSPARENCY);
		OTWDriver.renderer->context.SelectTexture((GLint) pTex);

		// Render it (finally)
		OTWDriver.renderer->context.DrawPrimitive(MPR_PRM_TRIFAN, MPR_VI_COLOR | MPR_VI_TEXTURE, 4, pVtx, sizeof(pVtx[0]));
	}
}

void CPAdi::DisplayBlit(void)
{
	RECT srcRect;
	mDirtyFlag = TRUE;

	if(!mDirtyFlag)
		return;

	if(mDoBackRect && m_arrTex.size() == 0)
	{
		if(mpSurfaceBuffer)
			mpOTWImage->Compose(mpSurfaceBuffer, &mBackSrc, &mBackDest);
	}

	// Build the source rectangle
	srcRect.top			= mSrcRect.top + mSrcHalfHeight - mRadius - (int) mSlide;
	srcRect.left		= mSrcRect.left;
	srcRect.bottom		= srcRect.top + (mDestRect.bottom - mDestRect.top) / mScale;
	srcRect.right		= srcRect.left + (mDestRect.right - mDestRect.left) / mScale;

	mpOTWImage->ComposeRoundRot(mpTemplate, &srcRect, &mDestRect, -mRoll, mpADICircle);

	mDirtyFlag = FALSE;
}

//====================================================//
// CPAdi::Display
//====================================================//

void CPAdi::DisplayDraw(void)
{
	int		i;
	float	width = 2;

	//MI make them a bit smaller
	if(g_bRealisticAvionics)
		width = 1.0F;

	mDirtyFlag = TRUE;

	if(!mDirtyFlag)
		return;

	OTWDriver.renderer->SetViewport(-1.0F, 1.0F, 1.0F, -1.0F);

	//OTWDriver.renderer->SetColor(mColor[TheTimeOfDay.GetNVGmode() != 0][0]);
	//MI to make the A/C reference symbol on the STBY ADI white, yellow on the main ADI
	if(Persistant == 1)
		OTWDriver.renderer->SetColor(mColor[TheTimeOfDay.GetNVGmode() != 0][2]);
	else
		OTWDriver.renderer->SetColor(mColor[TheTimeOfDay.GetNVGmode() != 0][4]);
	for(i = 0; i < NUM_AC_BAR_POINTS - 1; i++)
	{
		OTWDriver.renderer->Render2DLine((float)mpAircraftBar[i][0], 
													(float)mpAircraftBar[i][1],
													(float)mpAircraftBar[i + 1][0],  
													(float)mpAircraftBar[i + 1][1]);
	}

	if(!mpCPManager->mHiddenFlag)
	{
		//OTWDriver.renderer->SetColor(mColor[TheTimeOfDay.GetNVGmode() != 0][1]);
		OTWDriver.renderer->Render2DTri((float)mVertBarPos, (float)mTopLimit - 3.0F * width, (float)mVertBarPos + width,
         (float)mTopLimit - 3.0F * width, (float)mVertBarPos + width, (float)mBottomLimit + 3.0F * width);
		OTWDriver.renderer->Render2DTri((float)mVertBarPos, (float)mTopLimit - 3.0F * width, (float)mVertBarPos,
         (float)mBottomLimit + 3.0F * width, (float)mVertBarPos + width, (float)mBottomLimit + 3.0F * width);
		//OTWDriver.renderer->SetColor(mColor[TheTimeOfDay.GetNVGmode() != 0][2]);
		OTWDriver.renderer->Render2DTri((float)mVertBarPos, (float)mTopLimit - 3.0F * width, (float)mVertBarPos - width,
         (float)mTopLimit - 3.0F * width, (float)mVertBarPos - width, (float)mBottomLimit + 3.0F * width);
		OTWDriver.renderer->Render2DTri((float)mVertBarPos, (float)mTopLimit - 3.0F * width, (float)mVertBarPos,
         (float)mBottomLimit + 3.0F * width, (float)mVertBarPos - width, (float)mBottomLimit + 3.0F * width);
		//OTWDriver.renderer->SetColor(mColor[TheTimeOfDay.GetNVGmode() != 0][1]);
		OTWDriver.renderer->Render2DTri((float)mLeftLimit - 3.0F * width, (float)mHorizBarPos, (float)mLeftLimit - 3.0F * width,
         (float)mHorizBarPos + width,  (float)mRightLimit + 3.0F * width, (float)mHorizBarPos + width);
		OTWDriver.renderer->Render2DTri((float)mLeftLimit - 3.0F * width, (float)mHorizBarPos, (float)mRightLimit + 3.0F * width,
         (float)mHorizBarPos,  (float)mRightLimit + 3.0F * width, (float)mHorizBarPos + width);
		//OTWDriver.renderer->SetColor(mColor[TheTimeOfDay.GetNVGmode() != 0][2]);
		OTWDriver.renderer->Render2DTri((float)mLeftLimit - 3.0F * width, (float)mHorizBarPos, (float)mLeftLimit - 3.0F * width,
         (float)mHorizBarPos - 2.0F,  (float)mRightLimit + 3.0F * width, (float)mHorizBarPos - width); //MI (float)mHorizBarPos - 2.0F);
		OTWDriver.renderer->Render2DTri((float)mLeftLimit - 3.0F * width, (float)mHorizBarPos, (float)mRightLimit + 3.0F * width,
         (float)mHorizBarPos,  (float)mRightLimit + 3.0F * width, (float)mHorizBarPos - width); //MI (float)mHorizBarPos - 2.0F);
		 
   }

	mDirtyFlag = FALSE;
}


void CPAdi::Translate(WORD* palette16)
{
	if(mDoBackRect)
	{
		if(mpSurfaceBuffer)
			Translate8to16(palette16, mpSourceBuffer, mpSurfaceBuffer); // 8 bit color indexes of individual surfaces
	}																					// 16 bit ImageBuffers
}				

void CPAdi::Translate(DWORD* palette32)
{
	if(mDoBackRect)
	{
		if(mpSurfaceBuffer)
			Translate8to32(palette32, mpSourceBuffer, mpSurfaceBuffer); // 8 bit color indexes of individual surfaces
	}																					// 32 bit ImageBuffers
}				
