#include "stdhdr.h"
#include "otwdrive.h"
#include "simdrive.h"
#include "mfd.h"
#include "rwr.h"
#include "hud.h"
#include "Graphics\Include\render2d.h"
#include "Graphics\Include\renderow.h"
#include "cpmanager.h"
#include "aircrft.h"
#include "lantirn.h"

extern RECT VirtualMFD[OTWDriverClass::NumPopups + 1];
ViewportBounds	hudViewportBounds;

void OTWDriverClass::DoPopUps(void)
{
int i;

   for (i=0; i<NumPopups; i++)
   {
      /*
      if (popupHas[i] == 2)
      {
         // Draw RWR Here
      }
      else
      */
      if (popupHas[i] >= 0)
      {
//         OTWImage->Compose (MfdDisplay[popupHas[i]]->image, &(VirtualMFD[NumPopups]), &(VirtualMFD[i]));
      }
   }
}



void OTWDriverClass::Draw2DHud(void)
{
    int oldFont;
    if (theLantirn && theLantirn->IsFLIR()) {
#if DO_HIRESCOCK_HACK
	if(!gDoCockpitHack && (mOTWDisplayMode == ModeHud || mOTWDisplayMode == ModePadlockEFOV ||
	    mOTWDisplayMode == Mode2DCockpit && pCockpitManager)) {
#else
	    if(mOTWDisplayMode == ModeHud || mOTWDisplayMode == ModePadlockEFOV ||
		mOTWDisplayMode == Mode2DCockpit && pCockpitManager) {
#endif		
		if(pCockpitManager->GetViewportBounds(&hudViewportBounds,BOUNDS_HUD))
		{
		    theLantirn->DisplayInit (renderer->GetImageBuffer());
		    renderer->FinishFrame();
		    theLantirn->GetDisplay()->SetViewport(hudViewportBounds.left,
			hudViewportBounds.top,
			hudViewportBounds.right,
			hudViewportBounds.bottom);
		    theLantirn->SetFOV (60.0f*DTR*(hudViewportBounds.right-hudViewportBounds.left)/2.0f);
		    theLantirn->Display(theLantirn->GetDisplay());
		    renderer->StartFrame();
		}
	    }
	    
	}
	renderer->SetColor(TheHud->GetHudColor());
#if DO_HIRESCOCK_HACK
	if(!gDoCockpitHack && (mOTWDisplayMode == ModeHud || mOTWDisplayMode == ModePadlockEFOV ||
		mOTWDisplayMode == Mode2DCockpit && pCockpitManager)) {
#else
	if(mOTWDisplayMode == ModeHud || mOTWDisplayMode == ModePadlockEFOV ||
		mOTWDisplayMode == Mode2DCockpit && pCockpitManager) {
#endif		
	    if(pCockpitManager->GetViewportBounds(&hudViewportBounds,BOUNDS_HUD))
	    {
		oldFont = VirtualDisplay::CurFont();
		ShiAssert( otwPlatform );		// If we don't have this we could pass NULL for TheHud target...
		if (TheHud->Ownship())
		    TheHud->SetTarget( TheHud->Ownship()->targetPtr );	
		else
		    TheHud->SetTarget( NULL);	
		
		renderer->SetViewport(hudViewportBounds.left,
		    hudViewportBounds.top,
		    hudViewportBounds.right,
		    hudViewportBounds.bottom);
		
		// Should probably assert that hudViewportBounds.left = -hudViewportBounds.right
		// since we're assuming the HUD is horizontally centered in the display when it is active in 2D...
		TheHud->SetHalfAngle( (float)atan( hudViewportBounds.right * tan(renderer->GetFOV()/2.0f) ) * RTD );
		
		VirtualDisplay::SetFont(pCockpitManager->HudFont());
		if(TheHud->Ownship())
		{
		    TheHud->Display(renderer);
		}
		
		VirtualDisplay::SetFont(oldFont);
	    }
	}
	else {
	    ShiAssert( otwPlatform );		// If we don't have this we could pass NULL for TheHud target...
	    TheHud->SetTarget( TheHud->Ownship()->targetPtr );
	    
	    renderer->SetViewport(-0.4675F, 0.25F, 0.47F, -1.0F);
	    hudViewportBounds.left   = -0.4675F;
	    hudViewportBounds.top    =  0.25F;
	    hudViewportBounds.right  =  0.47F;
	    hudViewportBounds.bottom = -1.0F;
	    
	    TheHud->SetHalfAngle(15.1434F);
	    TheHud->Display(renderer);
	}

	renderer->SetColor(0xff00ff00);
}