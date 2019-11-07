#include "stdhdr.h"
#include "mfd.h"
#include "hud.h"
#include "sms.h"
#include "SmsDraw.h"
#include "airframe.h"
#include "Aircrft.h"
#include "missile.h"
#include "radar.h"
#include "PlayerRwr.h"
#include "fcc.h"
#include "otwdrive.h"
#include "playerop.h"
#include "Graphics\Include\render2d.h"
#include "Graphics\Include\canvas3d.h"
#include "Graphics\Include\tviewpnt.h"
#include "dispcfg.h"
#include "simdrive.h"
#include "camp2sim.h"
#include "fack.h"
#include "dispopts.h"
#include "digi.h"
#include "lantirn.h"
#include "otwdrive.h"	//MI
#include "cpmanager.h"	//MI
#include "icp.h"		//MI
#include "aircrft.h"	//MI
#include "fcc.h"		//MI
#include "radardoppler.h" //MI
#include "object.h"
#include "popmenu.h"					// a.s. begin
#include "Graphics\Include\renderow.h"  // a.s.
#include "Graphics\Include\grinline.h"	// a.s. end


//MI
void DrawBullseyeCircle(VirtualDisplay* display, float cursorX, float cursorY);


extern bool g_bRealisticAvionics;
extern bool g_bINS;


extern bool g_bEnableMfdColors; // a.s. begin: makes Mfds transparent and colored
extern float g_fMfdTransparency;// a.s.
extern float g_fMfdRed;			// a.s.
extern float g_fMfdGreen;		// a.s.
extern float g_fMfdBlue;		// a.s.
unsigned long mycolor;			// a.s. end


// JB 010802
extern bool g_b3dMFDLeft;
extern bool g_b3dMFDRight;

MFDClass* MfdDisplay[NUM_MFDS];
SensorClass* FindSensor (SimMoverClass* theObject, int sensorType);
//DrawableClass* MFDClass::ownDraw = NULL;
//DrawableClass* MFDClass::offDraw = NULL;
//DrawableClass* MFDClass::lantirnDraw = NULL;
DrawableClass* MFDClass::mfdDraw[MaxPrivMode];


// location and dimensions of the 2 MFD's in the virtual cockpit
// JPO - set back - can now be overridden in the 3dckpit.dat file
Tpoint lMFDul = { 18.205f , -7.089f, 7.793f };
Tpoint lMFDur = { 18.205f , -2.770f, 7.793f };
Tpoint lMFDll = { 17.501f , -7.089f, 11.816f };

Tpoint rMFDul = { 18.181f , 2.834f, 7.883f };
Tpoint rMFDur = { 18.181f , 6.994f, 7.883f };
Tpoint rMFDll = { 17.475f , 2.834f, 11.978f };

extern int MfdSize;
extern RECT VirtualMFD[OTWDriverClass::NumPopups + 1];
#define NUM_MFD_INTENSITIES        5
#if 0
static int MFDColorTable[NUM_MFD_COLORS] = {
	0x00FF00,
	0x00B000,
	0x007F00,
	0x003F00,
	0x001F00
};
#endif
// JPO - these masks are used to decided the colour intensity
static int MFDMasks[NUM_MFD_INTENSITIES] = {
	0xffffff, // full intensity
	0xB0B0B0, 
	0x7F7F7F,
	0x3F3F3F,
	0x1F1F1F, // lowest intensity
};

char *MFDClass::ModeNames[MAXMODES] = {
    "  ", // blank
    "", // menu
    "TFR", "FLIR", "TEST", "DTE", "FLCS", "WPN", "TGP",
    "HSD", "FCR", "SMS", "RWR", "HUD",
};

MFDClass::MfdMode MFDClass::initialMfdL[MFDClass::MAXMM][3] = {
    {FCRMode, MfdOff, TestMode}, // AA
    {FCRMode, MfdOff, TestMode}, // AG
    {FCRMode, MfdOff, TestMode}, // NAV
    {FCRMode, MfdOff, TestMode}, // MSL
    {FCRMode, MfdOff, TestMode}, // DGFT
};
MFDClass::MfdMode MFDClass::initialMfdR[MFDClass::MAXMM][3] = {
    {SMSMode, FCCMode, WPNMode}, //AA
    {SMSMode, FCCMode, WPNMode}, //AG
    {FCCMode, SMSMode, FLCSMode}, //NAV
    {SMSMode, FCCMode, WPNMode}, //MSL
    {SMSMode, FCCMode, WPNMode}, //DGFT
};

MFDClass::MFDClass (int count, Render3D *r3d )
{
   id = count;
   drawable = NULL;
   viewPoint = NULL;
   ownship = NULL;
   curmm = 2;
   newmm = 2;
   if (count % 2 == 0) 
   {
       memcpy (primarySecondary, initialMfdL, sizeof initialMfdL); 
   }
   else 
   {
       memcpy (primarySecondary, initialMfdR, sizeof initialMfdR);
   }
   changeMode = FALSE;
   for (int i = 0; i < MAXMM; i++)
       cursel[i] = 0;
   privateImage = new ImageBuffer; 
   privateImage->Setup( &FalconDisplay.theDisplayDevice, MfdSize, MfdSize, SystemMem, None );
   privateImage->SetChromaKey (0x0);
   image = OTWDriver.OTWImage;
   color = 0x00ff00;
   intensity = 0;
   mode = primarySecondary[curmm][0];
   restoreMode = mode;
   
   // Set bounds based on resolution
   vTop = vRight = -1.0F;
   vLeft = vBottom = 1.0F;
   vLeft = 1.0F - VirtualMFD[id].top/(DisplayOptions.DispHeight * 0.5F);
   vRight = 1.0F - VirtualMFD[id].bottom/(DisplayOptions.DispHeight * 0.5F);
   vTop = -1.0F + VirtualMFD[id].left/(DisplayOptions.DispWidth * 0.5F);
   vBottom = -1.0F + VirtualMFD[id].right/(DisplayOptions.DispWidth * 0.5F);
   
   viewPoint     = OTWDriver.GetViewpoint();
   
   if (mfdDraw[MfdOff] == NULL) mfdDraw[MfdOff] = new BlankMfdDrawable;
   if (mfdDraw[MfdMenu] == NULL) mfdDraw[MfdMenu] = new MfdMenuDrawable;
   if (mfdDraw[TFRMode] == NULL) mfdDraw[TFRMode] = new LantirnDrawable;
   if (mfdDraw[FLIRMode] == NULL) mfdDraw[FLIRMode] = new FlirMfdDrawable;
   if (mfdDraw[TestMode] == NULL) mfdDraw[TestMode] = new TestMfdDrawable;
   if (mfdDraw[DTEMode] == NULL) mfdDraw[DTEMode] = new DteMfdDrawable;
   if (mfdDraw[FLCSMode] == NULL) mfdDraw[FLCSMode] = new FlcsMfdDrawable;
   if (mfdDraw[WPNMode] == NULL) mfdDraw[WPNMode] = new WpnMfdDrawable;
   if (mfdDraw[TGPMode]	== NULL) mfdDraw[TGPMode] = new TgpMfdDrawable;

   for (i = 0; i < MaxPrivMode; i++) {
	if (mfdDraw[i] == NULL)
	    mfdDraw[i] = new BlankMfdDrawable;
   }
   // set up the virtual cockpit MFD
   virtMFD = new Canvas3D;
   ShiAssert( virtMFD );
   virtMFD->Setup( r3d );
   
   // left or right
   if ( id == 0 )
   {
		if (g_b3dMFDLeft)
			virtMFD->SetCanvas( &lMFDul, &lMFDur, &lMFDll );
   }
   else
   {
		if (g_b3dMFDRight)
			virtMFD->SetCanvas( &rMFDul, &rMFDur, &rMFDll );
   }

   //MI init vars
   EmergStoreMode = FCCMode;
}


MFDClass::~MFDClass (void)
{
   FreeDrawable();

   if (privateImage)
   {
      privateImage->Cleanup();
      delete privateImage;
   }
   mode = MfdOff;
   drawable = NULL;
   for (int i = 0; i < MaxPrivMode; i++) {
       if (mfdDraw[i]) {
	   mfdDraw[i]->DisplayExit();
	   delete mfdDraw[i];
	   mfdDraw[i] = NULL;
       }
   }

   delete virtMFD;
   virtMFD = NULL;
}

char *MFDClass::ModeName (int n)
{
    ShiAssert(n >= 0 && n < MAXMODES);
    return ModeNames[n];
}

void MFDClass::SetNewRenderer(Render3D *r3d)
{
	virtMFD->ResetTargetRenderer(r3d);
}

void MFDClass::UpdateVirtualPosition( const Tpoint *loc, const Trotation *rot )
{
	virtMFD->Update( (struct Tpoint *) loc, (struct Trotation *) rot );
}

void MFDClass::SetOwnship (AircraftClass *newOwnship)
{
    if(SimDriver.playerEntity &&
	SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) &&
	ownship == SimDriver.playerEntity &&
	newOwnship != SimDriver.playerEntity) { // If we are jumping to another vehicle view ...
	restoreMode = mode;						// ... save the current MFD state
    }
    
    ownship = newOwnship;
    FreeDrawable();
    drawable = NULL;
    
    if(SimDriver.playerEntity &&
	SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) &&	// If we are jumping back into our F16
	ownship == SimDriver.playerEntity) {
	
	SetMode(restoreMode);									// restore the saved MFD state
    }
    else {
	SetMode (MfdOff);										// Otherwise turn the MFD off								
    }
}

void MFDClass::NextMode(void)
{
    if (g_bRealisticAvionics && id < 2)
    {
	cursel[curmm] --;
	if (cursel[curmm] < 0)
	    cursel[curmm] = 2;
	mode = primarySecondary[curmm][cursel[curmm]];
	//MI addition
	if(mode == MfdOff)
		NextMode();
	SetMode(mode);
    }
    else {
	switch (mode)
	{
	  case FCCMode:
		SetMode (FCRMode);
		break;

	  case FCRMode:
		SetMode (SMSMode);
		break;

	  case SMSMode:
		SetMode (RWRMode);
		break;

	  case RWRMode:
		  // M.N. added full realism mode
		if (PlayerOptions.GetAvionicsType() != ATRealistic && PlayerOptions.GetAvionicsType() != ATRealisticAV) {
			SetMode (MfdOff);
		} else {
			SetMode (HUDMode);
		}
		break;

	  case HUDMode:
		SetMode (MfdOff);
		break;

	  case MfdOff:
		  // M.N. added full realism mode
		if (PlayerOptions.GetAvionicsType() != ATRealistic && PlayerOptions.GetAvionicsType() != ATRealisticAV) {
			SetMode (FCCMode);
		} else {
			SetMode (MfdMenu);
		}
		break;

	  case MfdMenu:
	      if (theLantirn && theLantirn->IsEnabled())
		  SetMode (TFRMode);
	      else
		SetMode (FCCMode);
		break;
	  case TFRMode:
	      SetMode (FCCMode);
	      break;
	  default:
		SetMode (MfdMenu);
		break;
	}
    }
   // For in cockpit, you can't have two of the same thing
   if (mode != MfdOff)
   {
      if (id == 0)
      {
         if (MfdDisplay[1]->mode == mode)
            NextMode();
      }
      else if (id == 1)
      {
         if (MfdDisplay[0]->mode == mode)
            NextMode();
      }
   }
}

// the other MFD wants this mode.
void MFDClass::StealMode(MfdMode mode)
{
    if (mode == MfdOff) return;
    for (int i = 0; i < 3; i++) {
	if (primarySecondary[curmm][i] == mode)  {
	    if (i == cursel[curmm])
		SetNewMode(MfdOff);
	    primarySecondary[curmm][i] = MfdOff;
	}
    }
}

void MFDClass::SetMode(MfdMode newMode)
{
   if (ownship == NULL)
        return;

   MfdMode lastMode = mode;

   FreeDrawable();
   mode = newMode;

   switch (mode)
   {
      case FCRMode:
         drawable = FindSensor(ownship, SensorClass::Radar);
      break;

      case RWRMode:
         drawable = FindSensor(ownship, SensorClass::RWR);
      break;

      case SMSMode:
         drawable = ownship->FCC->Sms->drawable;
      break;

      case HUDMode:
         drawable = TheHud;
      break;

      case FCCMode:
         drawable = ownship->FCC;
      break;

      case MfdMenu: // special case cos its not really a mode
         drawable = mfdDraw[MfdMenu];
	 restoreMode = lastMode;
	 ((MfdMenuDrawable*)mfdDraw[MfdMenu])->SetCurMode(lastMode);
      break;

      case MfdOff:
      case TFRMode:
      case TestMode:
      case FLIRMode:
      case DTEMode:
      case FLCSMode:
      case WPNMode:
      case TGPMode:
	  drawable = mfdDraw[mode];
      break;
      
      default:
	  ShiWarning( "Bad MFD mode" );
	  drawable = NULL;
	  break;
   }
   if (mode != MfdMenu) {
       primarySecondary[curmm][cursel[curmm]] = mode;
       if (id == 0)
       {
	   MfdDisplay[1]->StealMode(mode);
       }
       else if (id == 1)
       {
	   MfdDisplay[0]->StealMode(mode);
       }
   }

   if (drawable)
   {
      drawable->SetMFD(id);
      drawable->viewPoint = viewPoint;

      if (mode != MfdOff)
         OTWDriver.SetPopup(id, TRUE);
      else
         OTWDriver.SetPopup(id, FALSE);
   }
   else
   {
      OTWDriver.SetPopup(id, FALSE);
   }
}

void MFDClass::FreeDrawable (void)
{
int i;
int inUse = FALSE;

    // JPO CTD check
    ShiAssert(drawable == NULL || FALSE == F4IsBadReadPtr(drawable, sizeof *drawable));
	// Don't bother if we don't have a drawable and a private display
	if (drawable && drawable->GetDisplay()) {

		if (mode != MfdOff)
		{
			for (i=0; i<NUM_MFDS; i++)
			{
				if (MfdDisplay[i] && this != MfdDisplay[i])
				{
					if (MfdDisplay[i]->mode == mode)
					{
						inUse = TRUE;
						break;
					}
				}
			}
		}


		if (!inUse)
		{
			drawable->DisplayExit();
		}

      // Drop our reference, even if someone else is drawing
      // NOTE:: This may cause a leak if no-one calls display
      //        exit.
		drawable = NULL;
		mode = MfdOff;
	}
}

void MFDClass::ButtonPushed(int whichButton, int whichMFD)
{
    if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) {
	if (drawable)
	{
	    drawable->PushButton(whichButton, whichMFD);
	}
    }
}

void MFDClass::SetNewMasterMode(int n)
{
    ShiAssert(n >= 0 && n < MAXMM);
    changeMode = TRUE_MODESWITCH;
    newmm = n;
}

void MFDClass::SetNewMode(MfdMode newMfdMode) {
    
    newMode = newMfdMode;
    changeMode = TRUE_ABSOLUTE;
}

void MFDClass::SetNewModeAndPos(int n, MfdMode newMfdMode)
{
    ShiAssert(n >= 0 && n < 3);
    cursel[curmm] = n;
    newMode = newMfdMode;
    changeMode = TRUE_ABSOLUTE;
}


void MFDClass::Exec(int clearFrame, int virtualCockpit)
{
	float vpLeft, vpTop, vpRight, vpBottom;

	if (changeMode == TRUE_NEXT)
	{
		NextMode();
		changeMode = FALSE;
	}
	else if(changeMode == TRUE_ABSOLUTE)
	{
		SetMode (newMode);
		changeMode = FALSE;
	}
	else if (changeMode == TRUE_MODESWITCH)
	{
		curmm = newmm;
		SetMode(primarySecondary[curmm][cursel[curmm]]);
		changeMode = FALSE;
	}

	if (ownship && ownship->mFaults &&
		(
		(ownship->mFaults->GetFault(FaultClass::flcs_fault) & FaultClass::dmux) ||
		ownship->mFaults->GetFault(FaultClass::dmux_fault) ||
		((ownship->mFaults->GetFault(FaultClass::mfds_fault) & FaultClass::lfwd) && (id % 2 == 0)) ||
		((ownship->mFaults->GetFault(FaultClass::mfds_fault) & FaultClass::rfwd) && (id % 2 == 1))
		)
		)
	{
		return;
	}
	
	if (!ownship->HasPower(AircraftClass::MFDPower))
		return;

	if ( virtualCockpit )
	{
		if ( drawable )
		{
			// edg: I *think* ground map radar is in FCR mode, for now
			// ignore this mode since we can't do any 3d rendering to
			// the canvas
			if ( mode != MfdOff )
			{
				if (mode == RWRMode)
				{
					((PlayerRwrClass*)drawable)->SetGridVisible(TRUE);
				}

				//		virtMFD->SetIntensity(GetIntensity());
				virtMFD->SetColor (Color());
				drawable->SetIntensity(GetIntensity()); // JPO set intensity
				drawable->Display( virtMFD );
			}
		}
	}
	else
	{
		if (drawable && !(mode == MfdOff && clearFrame))
		{
			drawable->SetMFD(id);
			if (drawable->GetDisplay())
			{
				if (((Render2D*)(drawable->GetDisplay()))->GetImageBuffer() != image)
					((Render2D*)(drawable->GetDisplay()))->SetImageBuffer(image);

				((Render2D*)(drawable->GetDisplay()))->GetViewport (&vpLeft, &vpTop, &vpRight, &vpBottom);
				((Render2D*)(drawable->GetDisplay()))->SetViewport(vTop, vLeft, vBottom, vRight);
				OTWDriver.SetPopup(id, TRUE);

				drawable->GetDisplay()->StartFrame();

				if(clearFrame)
				{
					if (g_bEnableMfdColors) // a.s. begin - makes Mfds transparent and colored
					{
						mycolor = (int)((g_fMfdRed/100.0F)*255.0F) + (int)((g_fMfdGreen/100.0F)*255.0F) * 0x100 + (int)((g_fMfdBlue/100.0F)*255.0F) * 0x10000 + (int)((g_fMfdTransparency/100.0F)*255.0F) * 0x1000000;
						if (mode == FCCMode ) 
						{	
							drawable->Display(drawable->GetDisplay());

							OTWDriver.renderer->context.RestoreState( STATE_SOLID ); 
							drawable->GetDisplay()->SetColor (mycolor);	
							OTWDriver.renderer->context.RestoreState( STATE_ALPHA_SOLID );
							drawable->GetDisplay()->Tri (-1.0F, -1.0F, -1.0F, 1.0F, 1.0F, 1.0F);  
							drawable->GetDisplay()->Tri (-1.0F, -1.0F, 1.0F, -1.0F, 1.0F, 1.0F);  		

							drawable->SetIntensity(0xffffffff); 
							drawable->GetDisplay()->SetColor (Color());	

							drawable->Display(drawable->GetDisplay()); // second Display is necessary for FCC
							drawable->GetDisplay()->FinishFrame();
							((Render2D*)(drawable->GetDisplay()))->SetViewport (vpLeft, vpTop, vpRight, vpBottom); // EFOV bugfix 

							return;
						}
					} // a.s. end

					drawable->GetDisplay()->SetColor (0xff000000);

					if (g_bEnableMfdColors) // a.s. begin 
					{
						OTWDriver.renderer->context.RestoreState( STATE_SOLID );
						drawable->GetDisplay()->SetColor (mycolor);	
						OTWDriver.renderer->context.RestoreState( STATE_ALPHA_SOLID );
					} // a.s. end

					drawable->GetDisplay()->Tri (-1.0F, -1.0F, -1.0F, 1.0F, 1.0F, 1.0F);
					drawable->GetDisplay()->Tri (-1.0F, -1.0F, 1.0F, -1.0F, 1.0F, 1.0F);
				}

				drawable->SetIntensity(GetIntensity()); // JPO set intensity
				drawable->GetDisplay()->SetColor (Color());

				if (mode == RWRMode)
				{
					if (g_bEnableMfdColors) // a.s. begin
					{
						drawable->GetDisplay()->SetColor (0xff00ff00);
					} // a.s. end

					((PlayerRwrClass*)drawable)->SetGridVisible(TRUE);
				}

				drawable->Display(drawable->GetDisplay());

				drawable->GetDisplay()->FinishFrame();

				((Render2D*)(drawable->GetDisplay()))->SetViewport (vpLeft, vpTop, vpRight, vpBottom);
			}
			else
			{
				drawable->DisplayInit (image);
			}
		}
		else
		{
			OTWDriver.SetPopup(id, FALSE);
		}
	}
}

void MFDClass::Exit(void)
{
}

void MFDClass::SetImageBuffer (ImageBuffer* newImage, float top,float left,
                               float bottom, float right)
{
Render2D* theDisplay;

   if (newImage == NULL)
      image = privateImage;
   else
      image = newImage;

   if (top == bottom)
   {
      vLeft = 1.0F - VirtualMFD[id].top/(DisplayOptions.DispHeight * 0.5F);
      vRight = 1.0F - VirtualMFD[id].bottom/(DisplayOptions.DispHeight * 0.5F);
      vTop = -1.0F + VirtualMFD[id].left/(DisplayOptions.DispWidth * 0.5F);
      vBottom = -1.0F + VirtualMFD[id].right/(DisplayOptions.DispWidth * 0.5F);
   }
   else
   {
      vTop = top;
      vBottom = bottom;
      vLeft = left;
      vRight = right;
   }

   if (drawable)
   {
      theDisplay = (Render2D*)drawable->GetDisplay();
      if (theDisplay)
      {
         if (theDisplay->GetImageBuffer() != image)
            theDisplay->SetImageBuffer(image);
//         theDisplay->SetViewport(vTop, vLeft, vBottom, vRight);
      }
   }
}

void MFDClass::IncreaseBrightness (void)
{
    intensity = max (intensity - 1, 0);
}

void MFDClass::DecreaseBrightness(void)
{
    intensity = min (intensity + 1, NUM_MFD_INTENSITIES - 1);
}

// JPO Default color - green tempered by mask
int MFDClass::Color (void)
{
    return 0xff00 & MFDMasks[intensity];
}

// default intensity mask
unsigned int MFDClass::GetIntensity()
{ 
    return MFDMasks[intensity]; 
}

void MFDSwapDisplays (void)
{
    DrawableClass* tmpDrawable = MfdDisplay[0]->drawable;
    MFDClass::MfdMode tmpMode = MfdDisplay[0]->mode;
    
    MfdDisplay[0]->drawable = MfdDisplay[1]->drawable;
    MfdDisplay[0]->mode = MfdDisplay[1]->mode;
    
    MfdDisplay[1]->drawable = tmpDrawable;
    MfdDisplay[1]->mode = tmpMode;

    tmpMode = MfdDisplay[0]->restoreMode;
    MfdDisplay[0]->restoreMode = MfdDisplay[1]->restoreMode;
    MfdDisplay[1]->restoreMode = tmpMode;

    for (int mm = 0; mm < MFDClass::MAXMM; mm++) {
	int tmp = MfdDisplay[0]->cursel[mm];
	MfdDisplay[0]->cursel[mm] = MfdDisplay[1]->cursel[mm];
	MfdDisplay[1]->cursel[mm] = tmp;
	
	for (int i = 0; i < 3; i++) {
	    tmpMode = MfdDisplay[0]->primarySecondary[mm][i];
	    MfdDisplay[0]->primarySecondary[mm][i] = MfdDisplay[1]->primarySecondary[mm][i];
	    MfdDisplay[1]->primarySecondary[mm][i] = tmpMode;
	}
    }
    int t = MfdDisplay[0]->curmm;
    MfdDisplay[0]->curmm = MfdDisplay[1]->curmm;
    MfdDisplay[1]->curmm = t;
    if (MfdDisplay[0]->drawable)
	MfdDisplay[0]->drawable->SetMFD(0);
    if (MfdDisplay[1]->drawable)
	MfdDisplay[1]->drawable->SetMFD(1);
}

// Drawable Support
MfdDrawable::~MfdDrawable(void)
{
}

void MfdDrawable::DisplayInit (ImageBuffer* image)
{
   DisplayExit();

   privateDisplay = new Render2D;
   ((Render2D*)privateDisplay)->Setup (image);

   privateDisplay->SetColor (0xff00ff00);
}

void MfdDrawable::PushButton (int whichButton, int whichMFD)
{
	//MI
	FireControlComputer *FCC = MfdDisplay[whichMFD]->GetOwnShip()->GetFCC();
    ShiAssert(FCC != NULL);

    MFDClass::MfdMode mode;
    int otherMfd = 1 - whichMFD;

    switch(whichButton) {
    case 10:
	break;
    case 11:
    case 12:
    case 13:
	mode = MfdDisplay[whichMFD]->GetSP(whichButton-11);
	if (mode == MfdDisplay[whichMFD]->mode) 
	    mode = MFDClass::MfdMenu;
        // Check other MFD if needed;
	if (mode == MFDClass::MfdOff || otherMfd < 0 || MfdDisplay[otherMfd]->mode != mode)
	    MfdDisplay[whichMFD]->SetNewModeAndPos(whichButton-11, mode);
	break;
	    
    case 14:
	MFDSwapDisplays ();
	break;
    }
}

void MfdDrawable::DefaultLabel(int button)
{
    int id = OnMFD();
    MFDClass *mfd = MfdDisplay[id];

	//MI
	FireControlComputer *FCC = MfdDisplay[id]->GetOwnShip()->GetFCC();
    ShiAssert(FCC != NULL);

    switch (button) {
    case 10:
	if (mfd->mode == MFDClass::MfdMode::SMSMode) {
	    LabelButton(button, "S-J");
	}
	else
	    LabelButton(button, "DCLT");
	break;
    case 11:
			LabelButton(button, MFDClass::ModeName(mfd->GetSP(0)),
			NULL, mfd->cursel[mfd->curmm] == 0);
	break;
    case 12:
			LabelButton(button, MFDClass::ModeName(mfd->GetSP(1)),
			NULL, mfd->cursel[mfd->curmm] == 1);
	break;
    case 13:
			LabelButton(button, MFDClass::ModeName(mfd->GetSP(2)),
			NULL, mfd->cursel[mfd->curmm] == 2);
	break;
    case 14:
	LabelButton(button, "SWAP");
	break;
    }
}

void MfdDrawable::BottomRow()
{
    for (int i = 10; i < 15; i++)
	DefaultLabel(i);
}

/*
 * JPO Draw a -\/-\/- type thingy
 */
void MfdDrawable::DrawReference(AircraftClass *self)
{
    const float yref = -0.8f;
    const float xref = -0.9f;
    //const float deltax[] = { 0.05f,  0.05f, 0.05f, 0.03f,  0.05f, 0.05f, 0.05f };
	//MI tweaked values
	const float deltax[] = { 0.05f,  0.02f, 0.02f, 0.005f,  0.02f, 0.02f, 0.05f };
    const float deltay[] = { 0.00f, -0.05f, 0.05f, 0.00f, -0.05f, 0.05f, 0.00f };
    const float RefAngle = 45.0f * DTR;
    float x = xref, y = yref;

    float offset = 0.0f;

    ShiAssert(self != NULL && TheHud != NULL);
    FireControlComputer *FCC = self->GetFCC();
    ShiAssert(FCC != NULL);
    switch (FCC->GetMasterMode() )
    {
		case FireControlComputer::AirGroundBomb:
		case FireControlComputer::AirGroundLaser:
		case FireControlComputer::AirGroundCamera:
			if (FCC->inRange) 
			{
				offset = FCC->airGroundBearing / RefAngle;
			}
		break;
		case FireControlComputer::Dogfight:
		case FireControlComputer::MissileOverride:
		case (FireControlComputer::Gun && FCC->GetSubMode() != FireControlComputer::STRAF):
		{
			//MI target
			RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);
			if(theRadar && theRadar->CurrentTarget() && theRadar->CurrentTarget()->BaseData() &&
				!FCC->IsAGMasterMode())
			{
				float   dx, dy, xPos,tgtx, yPos = 0.0F;
				vector  collPoint;//newTarget->localData
				theRadar->TargetToXY(theRadar->CurrentTarget()->localData, 0, 
					theRadar->GetDisplayRange(), &tgtx, &yPos);
				if (FindCollisionPoint ((SimBaseClass*)theRadar->CurrentTarget()->BaseData(), 
					self, &collPoint))
				{
					collPoint.x -= self->XPos();
					collPoint.y -= self->YPos();
					collPoint.z -= self->ZPos();

					dx = collPoint.x;
					dy = collPoint.y;

					xPos = (float)atan2 (dy,dx) - self->Yaw();

					if (xPos > (180.0F * DTR))
						xPos -= 360.0F * DTR;
					else if (xPos < (-180.0F * DTR))
						xPos += 360.0F * DTR;
				}

				/*---------------------------*/
				/* steering point screen x,y */
				/*---------------------------*/
				if (fabs(xPos) < (60.0F * DTR))
				{
					xPos = xPos / (60.0F * DTR);
					xPos += tgtx;
					
					offset = xPos;
					offset -= TargetAz (self, theRadar->CurrentTarget()->BaseData()->XPos(),
						theRadar->CurrentTarget()->BaseData()->YPos());
				}
				else if(xPos > (60 * DTR))
					offset = 1;
				else
					offset = -1;
			}
		}
		break;
		case FireControlComputer::Nav:
		case FireControlComputer::ILS:
		default:	//Catch all the other stuff
			if (TheHud->waypointValid) 
			{ 
				offset = TheHud->waypointBearing / RefAngle;
			} 
		break;
	}
    offset = min ( max (offset, -1.0F), 1.0F);
    for (int i = 0; i < sizeof(deltax)/sizeof(deltax[0]); i++) 
	{
		display->Line(x, y, x+deltax[i], y+deltay[i]);
		x += deltax[i]; y += deltay[i];
    }
    float xlen = (x - xref);
    x = xref + xlen/2 + offset * xlen/2.0f;
	if(g_bINS)
	{
		if(SimDriver.playerEntity && !SimDriver.playerEntity->INSState(AircraftClass::INS_HSD_STUFF))
			return;
	}
	display->Line(x, yref + 0.086f, x, yref - 0.13f);
}

#if 0
// JPO old code...
#define NOENTRY { NULL, NULL, MfdMenuButtons::ModeNoop}

struct MfdMenuButtons {
    char *label1, *label2;
    enum { ModeNoop = -1,  // do nothing
	ModeSwap = -2, // swap L & R
	ModeTest1 = -3, ModeTest2 = -4, // two test sub modes
	ModeReset = -5,  // reset mode
	ModePrev = -6, // previous FCC mode
	ModeWPN = -7,	// weapon mode
	ModeRECON = -8, // recon pod
	ModeTGP = -9,	// targeting pod.
	ModeNAV = -10, // navigation HTS
	ModeTFR = -11, // Terrain Following Radar
	ModeFLIR = -12, // FLIR Lantirn stuff
    }; 
    int nextMode;
};

static const MfdMenuButtons mainpage[20] =
{
    {"BLANK", NULL, MFDClass::MfdOff}, //1
    {"HUD", NULL, MFDClass::HUDMode},
    {"RWR", NULL, MFDClass::RWRMode},
    {"RCCE", NULL, MfdMenuButtons::ModeRECON},
    {"RESET", "MENU", MfdMenuButtons::ModeReset},	// 5
    {"SMS", NULL, MFDClass::SMSMode},
    {"HSD", NULL, MfdMenuButtons::ModeNAV},
    {"DTE", NULL, MfdMenuButtons::ModeNoop},
    {"TEST", NULL, MfdMenuButtons::ModeTest1},
    {"FLCS", NULL, MfdMenuButtons::ModeNoop},	//10
    {"DCLT", NULL, MfdMenuButtons::ModeNoop},
    NOENTRY,
    NOENTRY,
    {NULL, NULL, MfdMenuButtons::ModePrev}, // current mode
    {"SWAP", NULL, MfdMenuButtons::ModeSwap},	// 15
    {"FLIR", NULL, MfdMenuButtons::ModeFLIR},
    {"TFR", NULL, MfdMenuButtons::ModeTFR},
    {"WPN", NULL, MfdMenuButtons::ModeWPN},
    {"TGP", NULL, MfdMenuButtons::ModeTGP},
    {"FCR", NULL, MFDClass::FCRMode},	// 20
};

static const MfdMenuButtons testpage1[20] =
{ // test page menu
    {"BIT1", NULL, MfdMenuButtons::ModeTest2},    // 1
	NOENTRY,
    {"CLR", NULL, MfdMenuButtons::ModeNoop},
    NOENTRY,
    NOENTRY,    // 5
    {"MFDS", NULL, MfdMenuButtons::ModeNoop},
    {"RALT", "500", MfdMenuButtons::ModeNoop},
    {"TGP", NULL, MfdMenuButtons::ModeNoop},
    {"FINS", NULL, MfdMenuButtons::ModeNoop},
    {"TFR", NULL, MfdMenuButtons::ModeNoop},    // 10
    NOENTRY,
    NOENTRY,
    NOENTRY,
    {NULL, NULL, MfdMenuButtons::ModePrev}, // current mode
    {"SWAP", NULL, MfdMenuButtons::ModeSwap},    // 15
    {"RSU", NULL, MfdMenuButtons::ModeNoop},
    {"INS", NULL, MfdMenuButtons::ModeNoop},
    {"SMS", NULL, MfdMenuButtons::ModeNoop},
    {"FCR", NULL, MfdMenuButtons::ModeNoop},
    {"DTE", NULL, MfdMenuButtons::ModeNoop},    // 20
};

static const MfdMenuButtons testpage2[20] =
{ // test page menu
    {"BIT2", NULL, MfdMenuButtons::ModeTest1},    // 1
	NOENTRY,
    {"CLR", NULL, MfdMenuButtons::ModeNoop},
    NOENTRY,
    NOENTRY,    // 5
    {"IFF1", NULL, MfdMenuButtons::ModeNoop},
    {"IFF2", NULL, MfdMenuButtons::ModeNoop},
    {"IFF3", NULL, MfdMenuButtons::ModeNoop},
    {"IFFC", NULL, MfdMenuButtons::ModeNoop},
    {"TCN", NULL, MfdMenuButtons::ModeNoop},    // 10
    NOENTRY,
    NOENTRY,
    NOENTRY,
    {NULL, NULL, MfdMenuButtons::ModePrev}, // current mode
    {"SWAP", NULL, MfdMenuButtons::ModeSwap},    // 15
    {NULL, NULL, MfdMenuButtons::ModeNoop},
    {NULL, NULL, MfdMenuButtons::ModeNoop},
    {NULL, NULL, MfdMenuButtons::ModeNoop},
    {"TISL", NULL, MfdMenuButtons::ModeNoop},
    {"UFC", NULL, MfdMenuButtons::ModeNoop},    // 20
};

static const MfdMenuButtons resetpage[20] =
{ // reset page menu
    {"BLANK", NULL, MFDClass::MfdOff},    // 1
	NOENTRY,
    NOENTRY,
    NOENTRY,
    {"RESET", "MENU", MfdMenuButtons::ModeReset},    // 5
    {"SBC DAY", "RESET", MfdMenuButtons::ModeNoop},
    {"SBC NIGHT", "RESET", MfdMenuButtons::ModeNoop},
    {"SBC DFLT", "RESET", MfdMenuButtons::ModeNoop},
    {"SBC DAY", "SET", MfdMenuButtons::ModeNoop},
    {"SBC NIGHT", "SET", MfdMenuButtons::ModeNoop},    // 10
    {"DCLT", NULL, MfdMenuButtons::ModeNoop},
    NOENTRY,
    NOENTRY,
    {NULL, NULL, MfdMenuButtons::ModePrev}, // current mode
    {"SWAP", NULL, MfdMenuButtons::ModeSwap},    // 15
    {NULL, NULL, MfdMenuButtons::ModeNoop},
    {NULL, NULL, MfdMenuButtons::ModeNoop},
    {"NVIS", "OVRD", MfdMenuButtons::ModeNoop},
    {"PROG DCLT", "RESET", MfdMenuButtons::ModeNoop},
    {"MSMD", "RESET", MfdMenuButtons::ModeNoop},    // 20
};

struct MfdPage {
    int type;
    const MfdMenuButtons *buttons;
};


static const MfdPage mfdpages[] = {
    {MfdDrawable::MfdMenu, mainpage},
    {MfdDrawable::TestMenu, testpage1},
    {MfdDrawable::TestMenu, testpage2},
    {MfdDrawable::ResetMenu, resetpage},
};
static const int NMFDPAGES = sizeof(mfdpages) / sizeof(mfdpages[0]);

#undef NOENTRY

void MfdDrawable::PushButton (int whichButton, int whichMFD)
{
    int otherMfd;
    MFDClass::MfdMode nextMode = MFDClass::MfdOff;
    
    // For in cockpit, you can't have two of the same thing
    if (whichMFD == 0)
	otherMfd = 1;
    else if (whichMFD == 1)
	otherMfd = 0;
    else
	otherMfd = -1;
    
    if (g_bRealisticAvionics) {
	ShiAssert(whichButton >= 0 && whichButton < 20);
	if (mfdDisplayType < 0 || mfdDisplayType >= NMFDPAGES)
	    return;
	int nm = mfdpages[mfdDisplayType].buttons[whichButton].nextMode;
	if (nm < 0) { // special instructions
	    switch (nm) {
	    default:
	    case MfdMenuButtons::ModeNoop: // do nothing
		return;
	    case MfdMenuButtons::ModeSwap: // swap displays
		MFDSwapDisplays();
		return;
	    case MfdMenuButtons::ModeTest1: // go to test1 page
		mfdDisplayType = TestMenu;
		return;
	    case MfdMenuButtons::ModeTest2: // goto test2 page
		mfdDisplayType = TestMenu2;
		return;
	    case MfdMenuButtons::ModeReset: // goto reset page
		mfdDisplayType = mfdDisplayType == ResetMenu ? MfdMenu : ResetMenu;
		return;
	    case MfdMenuButtons::ModePrev: // goto previous mode
		if (mfdDisplayType != MfdMenu) {
		    mfdDisplayType = MfdMenu;
		    return;
		}
		else
		    nextMode = curmode;
		break;
	    case MfdMenuButtons::ModeRECON:
		if (SimDriver.playerEntity->Sms->HasWeaponClass(wcCamera)) {
		    SMSClass *Sms = SimDriver.playerEntity->Sms;
		    if (Sms->FindWeaponClass (wcCamera, FALSE))
		    {
			Sms->SetWeaponType(Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponType());
		    }
		    else
		    {
			Sms->SetWeaponType (wtNone);
		    }
		    nextMode = MFDClass::SMSMode;
		    Sms->drawable->SetDisplayMode(SmsDrawable::Wpn);
		    break;
		}
		return;
	    case MfdMenuButtons::ModeNAV:
		nextMode = MFDClass::FCCMode;
		SimDriver.playerEntity->FCC->SetMasterMode(FireControlComputer::Nav);
		break;
	    case MfdMenuButtons::ModeTGP: // can't figure this one out
		//SimDriver.playerEntity->Sms->drawable->SetDisplayMode(SmsDrawable::Wpn);
		return;
		//break;
	    case MfdMenuButtons::ModeWPN:
		nextMode = MFDClass::SMSMode;
		SimDriver.playerEntity->Sms->drawable->SetDisplayMode(SmsDrawable::Wpn);
		break;
	    case MfdMenuButtons::ModeTFR:
		nextMode = MFDClass::TFRMode;
		break;
	    case MfdMenuButtons::ModeFLIR:
		return; 
	    }
	}
	else {
	    nextMode = (MFDClass::MfdMode)nm;
	    if (nextMode == MFDClass::MfdOff || otherMfd < 0 || MfdDisplay[otherMfd]->mode != nextMode)
		MfdDisplay[whichMFD]->SetNewMode(nextMode);
	    return;
	}
    }
    else {
	switch (whichButton)
	{
	case 1:
	    nextMode = MFDClass::FCCMode;
	    break;
	    
	case 2:
	    nextMode = MFDClass::FCRMode;
	    break;
	    
	case 3:
	    nextMode = MFDClass::SMSMode;
	    break;
	    
	case 11:
	    nextMode = MFDClass::RWRMode;
	    break;
	    
	case 12:
	    nextMode = MFDClass::HUDMode;
	    break;
	    
	case 15:
	    if (!g_bRealisticAvionics) {
		float bingo = SimDriver.playerEntity->bingoFuel;
		
		if ( bingo >= 0 )
		{
		    if (bingo <= 1000.0f)
			SimDriver.playerEntity->bingoFuel +=  100.0f;
		    else if (bingo < 3000.0f)
			SimDriver.playerEntity->bingoFuel += 200.0f;
		    else if (bingo < 10000.0f)
			SimDriver.playerEntity->bingoFuel+= 500.0f;
		    else
			SimDriver.playerEntity->bingoFuel = 0.0f;
		}
	    }
	    break;
	}
    }
   // Check other MFD if needed;
   if (nextMode != MFDClass::MfdOff && (otherMfd < 0 || MfdDisplay[otherMfd]->mode != nextMode))
		MfdDisplay[whichMFD]->SetNewMode(nextMode);
}

void MfdDrawable::Display (VirtualDisplay* newDisplay)
{
    display = newDisplay;
    
    if (mfdDisplayType == MfdOff) {
	display->TextCenterVertical (0.0F, 0.0F, "OFF");
    }
    else if (g_bRealisticAvionics) {
	const MfdMenuButtons *mb = mfdpages[mfdDisplayType].buttons;
	display->SetColor(GetMfdColor(MFD_LABELS));
	for (int i = 0; i < 20; i++) {
	    if (mb[i].label1)
		LabelButton(i, mb[i].label1, mb[i].label2, curmode == mb[i].nextMode);
	}
	switch(mfdDisplayType) { // button 14 - is the current mode
	case TestMenu:
	case TestMenu2:
	    LabelButton (13, "TEST", NULL, 1);
	    break;
	default:
	    switch(curmode) {
	    case MFDClass::FCRMode:
		LabelButton (13, "FCR", NULL, 1);
		break;
	    case MFDClass::SMSMode:
		LabelButton (13, "SMS", NULL, 1);
		break;
	    case MFDClass::FCCMode:
		LabelButton (13, "HSD", NULL, 1);
		break;
	    case MFDClass::HUDMode:
		LabelButton (13, "HUD", NULL, 1);
		break;
	    case MFDClass::RWRMode:
		LabelButton (13, "RWR", NULL, 1);
		break;
	    case MFDClass::TFRMode:
		LabelButton(13, "TFR", NULL, 1);
		break;
	    }
	}

    }
    else
    {
    char tmpStr[12];//me123 addet becourse of bingo display
      LabelButton (1, "HSD");
      LabelButton (2, "FCR");
      LabelButton (3, "SMS");
      if (!g_bRealisticAvionics) {
	  sprintf (tmpStr, "BINGO %.0f", SimDriver.playerEntity->bingoFuel);//me123 status ok
	  LabelButton (15,  tmpStr);//me123 status ok
      }
      LabelButton (11, "RWR");
      LabelButton (12, "HUD");
      LabelButton (13, "MENU", NULL, 1);
/*
      LabelButton (0, "111");
      LabelButton (1, "111");
      LabelButton (2, "111");
      LabelButton (3, "111");
      LabelButton (4, "111");
      LabelButton (5, "111");
      LabelButton (6, "111");
      LabelButton (7, "111");
      LabelButton (8, "111");
      LabelButton (9, "111");
      LabelButton (10, "111");
      LabelButton (11, "111");
      LabelButton (12, "111");
      LabelButton (13, "111");
      LabelButton (14, "111");
      LabelButton (15, "111");
      LabelButton (16, "111");
      LabelButton (17, "111");
      LabelButton (18, "111");
      LabelButton (19, "111");
*/
   }
}
#endif