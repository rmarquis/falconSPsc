#include "stdafx.h"
#include "kneeboard.h"
#include "cpkneeview.h"
#include "dispcfg.h"
#include "otwdrive.h"
#include "Graphics\Include\renderow.h"

//====================================================//
// CPKneeView::CPKneeView
//====================================================//

CPKneeView::CPKneeView(ObjectInitStr *pobjectInitStr, KneeBoard *pboard) : CPObject(pobjectInitStr)
{
	mpKneeBoard = pboard;
	mpKneeBoard->Setup( &FalconDisplay.theDisplayDevice, 
								mDestRect.top, 
								mDestRect.left, 
								mDestRect.bottom,
								mDestRect.right);


}

//====================================================//
// CPKneeView::~CPKneeView
//====================================================//

CPKneeView::~CPKneeView()
{
	if(mpKneeBoard) {
		mpKneeBoard->Cleanup();
	}
}

//====================================================//
// CPKneeView::Exec
//====================================================//

void CPKneeView::Exec(SimBaseClass* pOwnship) { 

	ShiAssert( mpKneeBoard );
	mpOwnship = pOwnship;
	mpKneeBoard->Refresh( (SimVehicleClass*)mpOwnship );
}

//====================================================//	
// CPKneeView::DisplayBlit
//====================================================//

void CPKneeView::DisplayBlit(void)
{	

	ShiAssert( mpKneeBoard );
	mpKneeBoard->DisplayBlit( mpOTWImage, OTWDriver.renderer, (SimVehicleClass*)mpOwnship );
}

//====================================================//	
// CPKneeView::DisplayDraw
//====================================================//

void CPKneeView::DisplayDraw(void) 
{

	ShiAssert( mpKneeBoard );
	mpKneeBoard->DisplayDraw( mpOTWImage, OTWDriver.renderer, (SimVehicleClass*)mpOwnship );
}
