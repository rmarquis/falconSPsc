#include "stdafx.h"
#include "cpcb.h"
#include "cpmanager.h"
#include "button.h"
#include "soundfx.h"
#include "fsound.h"

//===================================
// Function Bodies for CPButtonObject
//===================================

//------------------------------------------------------------------
// CPButtonObject::CPButtonObject
//------------------------------------------------------------------

CPButtonObject::CPButtonObject(ButtonObjectInitStr *pInitStr) {

	mIdNum				= pInitStr->objectId;
	mCallbackSlot		= pInitStr->callbackSlot;
	mTotalStates		= pInitStr->totalStates;
	mNormalState		= pInitStr->normalState;

#ifndef _CPBUTTON_USE_STL_CONTAINERS
	mTotalViews			= pInitStr->totalViews;
	mViewSlot			= 0;
#endif

	mCursorIndex		= pInitStr->cursorIndex;
	mDelay				= pInitStr->delay;

	mDelayInc			= -1;
	mCurrentState		= mNormalState;

	mSound1				= pInitStr->sound1;
	mSound2				= pInitStr->sound2;

	// JPO - lets be careful out there
	if(mCallbackSlot >= 0 && mCallbackSlot < TOTAL_BUTTONCALLBACK_SLOTS) {

		mTransStateToAero	= ButtonCallbackArray[mCallbackSlot].TransStateToAero;
		mTransAeroToState	= ButtonCallbackArray[mCallbackSlot].TransAeroToState;
	}
	else {

		mTransStateToAero = NULL;
		mTransAeroToState = NULL;
	}


#ifndef _CPBUTTON_USE_STL_CONTAINERS
	#ifdef USE_SH_POOLS
	mpButtonView = (CPButtonView **)MemAllocPtr(gCockMemPool,sizeof(CPButtonView *)*mTotalViews,FALSE);
	#else
	mpButtonView		= new CPButtonView*[mTotalViews];
	#endif
	
	memset(mpButtonView, 0, mTotalViews * sizeof(CPButtonView*));
#endif
}


//------------------------------------------------------------------
// CPButtonObject::HandleEvent
//------------------------------------------------------------------

void CPButtonObject::HandleEvent(int event) {

	if(mTransAeroToState) {
		mTransAeroToState(this, event);			// translate aero to button click
	}					

	mDelayInc = mDelay;

	if(mSound1 >= 0) {
		F4SoundFXSetDist(mSound1, FALSE, 0.0f, 1.0f);
	}

	NotifyViews();
}

//------------------------------------------------------------------
// CPButtonObject::HandleMouseEvent
//------------------------------------------------------------------

void CPButtonObject::HandleMouseEvent(int event) {

	if(event == CP_MOUSE_BUTTON0 || 
		event == CP_MOUSE_BUTTON1) {	

		if(mTransStateToAero) {
			mTransStateToAero(this, event);     //translate button click to aero
		}

		HandleEvent(event);
	}
}

int CPButtonObject::GetSound(int which) {

	int returnVal = -1;

	F4Assert(which == 1 || which == 2);	//values for which can only be 1 or 2;

	if(which == 1) {
		returnVal = mSound1;
	}
	else if (which == 2) {
		returnVal = mSound2;
	}

	return returnVal;
}


void CPButtonObject::SetSound(int which, int index) {

	if(which == 1) {
		mSound1 = index;
	}
	else if (which == 2) {
		mSound2 = index;
	}
	else {
		ShiWarning("Which can only be 1 or 2");	//values for which can only be 1 or 2;
	}
}

//------------------------------------------------------------------
// CPButtonObject::DecrementDelay
//------------------------------------------------------------------

void CPButtonObject::DecrementDelay(void) {

	if(mDelayInc < 0) {
		return;
	}
	else if(mDelayInc == 0) {
		mDelayInc--;
		mCurrentState = mNormalState;
		NotifyViews();
		if(mSound2 >= 0) {
			F4SoundFXSetDist(mSound2, FALSE, 0.0f, 1.0f);
		}
	}
	else if(mDelayInc > 0) {
		mDelayInc--;
	}
}


//------------------------------------------------------------------
// CPButtonObject::~CPButtonObject
//------------------------------------------------------------------

CPButtonObject::~CPButtonObject()
{
#ifndef _CPBUTTON_USE_STL_CONTAINERS
	delete [] mpButtonView;
#endif
}

//------------------------------------------------------------------
// CPButtonObject::AddView
//------------------------------------------------------------------

void CPButtonObject::AddView(CPButtonView* pCPButtonView)
{
#ifndef _CPBUTTON_USE_STL_CONTAINERS
	mpButtonView[mViewSlot++] = pCPButtonView;
#else
	mpButtonView.push_back(pCPButtonView);
#endif

	pCPButtonView->SetParentButtonPointer(this);
}


//------------------------------------------------------------------
// CPButtonObject::DoBlit
//------------------------------------------------------------------

BOOL CPButtonObject::DoBlit(void) {

	return(mCurrentState != mNormalState);
}

//------------------------------------------------------------------
// CPButtonObject::NotifyViews
//------------------------------------------------------------------

void CPButtonObject::NotifyViews() {

	int		i;

#ifndef _CPBUTTON_USE_STL_CONTAINERS
	for(i = 0; i < mTotalViews; i++) {
#else
	for(i = 0; i < mpButtonView.size(); i++) {
#endif
		mpButtonView[i]->SetDirtyFlag();
	}
}

//------------------------------------------------------------------
// CPButtonObject::GetCurrentState
//------------------------------------------------------------------

int CPButtonObject::	GetCurrentState() {

	return mCurrentState;
}


//------------------------------------------------------------------
// CPButtonObject::GetNormalState
//------------------------------------------------------------------
	
int CPButtonObject::GetNormalState() {

	return mNormalState;
}

//------------------------------------------------------------------
// CPButtonObject::GetId
//------------------------------------------------------------------

int CPButtonObject::GetId() {

	return mIdNum;
}

//------------------------------------------------------------------
// CPButtonObject::GetCursorIndex
//------------------------------------------------------------------
	
int CPButtonObject::GetCursorIndex() {

	return mCursorIndex;
}

//------------------------------------------------------------------
// CPButtonObject::SetCurrentState
//------------------------------------------------------------------

void CPButtonObject::SetCurrentState(int newState) {


	if(newState < 0 || newState >= mTotalStates) {	// If getting passed a bad state
	//	F4Assert(newState < mTotalStates);
		mCurrentState = mNormalState;
	}
	else {
		mCurrentState = newState;
	}
}

// call the update function if we have one.
void CPButtonObject::UpdateStatus()
{
    if(mTransAeroToState) { 
	// JPO special call so we can distinguish between action and check - if we care
	mTransAeroToState(this, CP_CHECK_EVENT); // translate aero to button click
    }					
}


//===================================
// Function Bodies for CPButtonView
//===================================

//------------------------------------------------------------------
// CPButtonView::CPButtonView
//------------------------------------------------------------------

CPButtonView::CPButtonView(ButtonViewInitStr* pButtonViewInitStr) {

	mIdNum				= pButtonViewInitStr->objectId;
	mParentButton		= pButtonViewInitStr->parentButton;
	mTransparencyType	= pButtonViewInitStr->transparencyType;
	mDestRect			= pButtonViewInitStr->destRect;
	mpSrcRect			= pButtonViewInitStr->pSrcRect;	// List of Rects
	mpOTWImage			= pButtonViewInitStr->pOTWImage;
	mpTemplate			= pButtonViewInitStr->pTemplate;
	mPersistant			= pButtonViewInitStr->persistant;
	mStates				= pButtonViewInitStr->states;
	mScale				= pButtonViewInitStr->scale;

	mDestRect.top		= mDestRect.top * mScale;
	mDestRect.left		= mDestRect.left * mScale;
	mDestRect.bottom	= mDestRect.bottom * mScale;
	mDestRect.right	= mDestRect.right * mScale;
}

//------------------------------------------------------------------
// CPButtonView::~CPButtonView
//------------------------------------------------------------------

CPButtonView::~CPButtonView() {

	if(mpSrcRect) {
		delete [] mpSrcRect;
	}
}

//------------------------------------------------------------------
// CPButtonView::Display
//------------------------------------------------------------------

void CPButtonView::DisplayBlit(void) {
	
#if _ALWAYS_DIRTY
	mDirtyFlag = TRUE;
#endif

	if(!mDirtyFlag) {
		return;
	}

	if(mpButtonObject->DoBlit() && mStates) {

		if(mTransparencyType == CPTRANSPARENT) {

			mpOTWImage->ComposeTransparent(mpTemplate, &mpSrcRect[mpButtonObject->GetCurrentState()], &mDestRect);
		}
		else {

			mpOTWImage->Compose(mpTemplate, &mpSrcRect[mpButtonObject->GetCurrentState()], &mDestRect);
		}
	}

	mpButtonObject->DecrementDelay();

	mDirtyFlag = FALSE;
}

//------------------------------------------------------------------
// CPButtonView::GetId
//------------------------------------------------------------------

int CPButtonView::GetId() {

	return mIdNum;
}

//------------------------------------------------------------------
// CPButtonView::GetTransparencyType
//------------------------------------------------------------------

int CPButtonView::GetTransparencyType() {

	return mTransparencyType;
}

//------------------------------------------------------------------
// CPButtonView::GetParentButton
//------------------------------------------------------------------

int CPButtonView::GetParentButton(void) {

	return mParentButton;
}

//------------------------------------------------------------------
// CPButtonView::GetParentButton
//------------------------------------------------------------------

void CPButtonView::SetParentButtonPointer(CPButtonObject* pButtonObject) {

	mpButtonObject = pButtonObject;
}



//------------------------------------------------------------------
// CPButtonView::HandleEvent
//------------------------------------------------------------------

BOOL CPButtonView::HandleEvent(int* cursorIndex, int event, int xpos, int ypos) {


	BOOL isTarget = FALSE;

	if(xpos >= mDestRect.left && 
		xpos <= mDestRect.right && 
		ypos >= mDestRect.top &&
		ypos <= mDestRect.bottom) {

		isTarget = TRUE;
		*cursorIndex = mpButtonObject->GetCursorIndex();

		mpButtonObject->HandleMouseEvent(event);
	}

	return isTarget;
}

void CPButtonView::UpdateView()
{
    mpButtonObject->UpdateStatus();
}
