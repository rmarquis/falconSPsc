#include "stdhdr.h"
#include "Graphics\Include\Render2D.h"
#include "radar.h"
#include "mfd.h"
#include "missile.h"
#include "misldisp.h"
#include "mavdisp.h"
#include "laserpod.h"
#include "harmpod.h"
#include "fcc.h"
#include "hardpnt.h"
#include "simveh.h"
#include "falcsess.h"
#include "playerop.h"
#include "commands.h"
#include "SmsDraw.h"
#include "sms.h"
#include "otwdrive.h"
#include "vu2.h"
/* ADDED BY S.G. FOR SELECTIVE JETISSON */ #include "aircrft.h"
/* ADDED BY S.G. FOR SELECTIVE JETISSON */ #include "airframe.h"
#include "campbase.h" // Marco for AIM9P
#include "classtbl.h"
#include "otwdrive.h"	//MI
#include "cpmanager.h"	//MI
#include "icp.h"		//MI
#include "aircrft.h"	//MI
#include "fcc.h"		//MI
#include "radardoppler.h" //MI
#include "simdrive.h"	//MI
#include "hud.h"	//MI
#include "fault.h"	//MI
#include "radardoppler.h"	//MI

//MI
void DrawBullseyeCircle(VirtualDisplay* display, float cursorX, float cursorY);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

SensorClass* FindLaserPod (SimMoverClass* theObject);
extern int F4FlyingEyeType;
//extern bool g_bArmingDelay;//me123	MI replaced with g_bRealisticAvionics
extern bool g_bRealisticAvionics;
extern bool g_bMLU;
//extern bool g_bHardCoreReal; //me123	MI replaced with g_bRealisticAvionics


//MI
int	SmsDrawable::flash = FALSE;
int SmsDrawable::InputFlash = FALSE;
//M.N.
int maxripple;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class GroundSpotDriver : public VuMaster
{
	VU_TIME		last_time;
	SM_SCALAR	lx;
	SM_SCALAR	ly;
	SM_SCALAR	lz;

public:
	
	GroundSpotDriver (VuEntity *entity);
	virtual void Exec (VU_TIME timestamp);
	virtual VU_BOOL ExecModel(VU_TIME timestamp);
};

class GroundSpotEntity : public FalconEntity
{
public:
	GroundSpotEntity (int type);
	GroundSpotEntity (VU_BYTE **);
	virtual int Sleep (void) {return 0;};
	virtual int Wake (void) {return 0;};
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

SmsDrawable::SmsDrawable (SMSClass* self)
{
	lastMode = displayMode = Inv;
	Sms = self;
	groundSpot = new GroundSpotEntity (F4FlyingEyeType+VU_LAST_ENTITY_TYPE);
	vuDatabase->Insert(groundSpot);
	frameCount = 0;
	isDisplayed = FALSE;
	sjHardPointSelected = hardPointSelected = 0;
	groundSpot->SetDriver (new GroundSpotDriver (groundSpot));
	hadCamera = FALSE;
	needCamera = FALSE;
	flags = 0;
	//MI new stuff
	Sms->BHOT = TRUE;
	EmergStoreMode = Inv;
	DataInputMode = Inv;
	lastInputMode = Inv;
	PossibleInputs = 0;
	InputModus = NONE;
	for(int i = 0; i < MAX_DIGITS; i++)
		Input_Digits[i] = 25;
	Manual_Input = FALSE;
	for(i = 0; i < STR_LEN; i++)
		inputstr[i] = ' ';
	inputstr[STR_LEN - 1] = '\0';
	wrong = FALSE;
	InputsMade =0;
	C1Weap = FALSE;
	C2Weap = FALSE;
	C3Weap = FALSE;
	C4Weap = FALSE;
	InputLine = 0;
	MaxInputLines = 0;

	//Init some stuff
	Sms->Prof1 = !Sms->Prof1;
	FireControlComputer *pFCC = Sms->ownship->GetFCC();
	if(Sms->Prof1)
	{
		Sms->rippleCount = Sms->Prof1RP;
		Sms->rippleInterval = Sms->Prof1RS;
		Sms->SetPair(Sms->Prof1Pair);
		if(pFCC)
			pFCC->SetSubMode(Sms->Prof1SubMode);
	}
	else
	{
		Sms->rippleCount = Sms->Prof2RP;
		Sms->rippleInterval = Sms->Prof2RS;
		Sms->SetPair(Sms->Prof2Pair);
		if(pFCC)
			pFCC->SetSubMode(Sms->Prof2SubMode);
	}

	// 2002-01-28 ADDED BY S.G. Init our stuff to keep the target deaggregated until last missile impact
	thePrevMissile = NULL;
	thePrevMissileIsRef = FALSE; // Stupid shit because theMissile isn't inserted into VU (therefore not referenced) until launched
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

SmsDrawable::~SmsDrawable (void)
{
	vuDatabase->Remove (groundSpot);

	// 2002-01-28 ADDED BY S.G. Clean up our act once we get destroyed
	if (thePrevMissile && thePrevMissileIsRef)
		VuDeReferenceEntity((VuEntity *)thePrevMissile);
	thePrevMissileIsRef = FALSE;
	thePrevMissile = NULL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Find out what to draw

VirtualDisplay* SmsDrawable::GetDisplay (void)
{
	VirtualDisplay* retval= privateDisplay;
	MissileClass* theMissile = (MissileClass*)(Sms->curWeapon);
	MaverickDisplayClass* theDisplay;
	float rx, ry, rz;
	Tpoint pos;
	
	if (Sms->curHardpoint <0 || displayMode == Off)
	{
		return retval;
	}
	
	switch (Sms->hardPoint[Sms->curHardpoint]->GetWeaponClass())
	{
	case wcAgmWpn:
		if (Sms->curWeaponType == wtAgm65)
		{
            if (theMissile && theMissile->IsMissile())
            {
				theDisplay = (MaverickDisplayClass*)theMissile->display;
				if (theDisplay)
				{
					if (!theDisplay->GetDisplay())
					{
						if (privateDisplay)
						{
							theDisplay->DisplayInit(((Render2D*)(privateDisplay))->GetImageBuffer());
						}
						theDisplay->viewPoint = viewPoint;
						
						// Set missile initial position
						Sms->hardPoint[Sms->curHardpoint]->GetSubPosition(Sms->curWpnNum, &rx, &ry, &rz);
						rx += 5.0F;
						pos.x = Sms->ownship->XPos() + Sms->ownship->dmx[0][0]*rx + Sms->ownship->dmx[1][0]*ry +
							Sms->ownship->dmx[2][0]*rz;
						pos.y = Sms->ownship->YPos() + Sms->ownship->dmx[0][1]*rx + Sms->ownship->dmx[1][1]*ry +
							Sms->ownship->dmx[2][1]*rz;
						pos.z = Sms->ownship->ZPos() + Sms->ownship->dmx[0][2]*rx + Sms->ownship->dmx[1][2]*ry +
							Sms->ownship->dmx[2][2]*rz;
						theDisplay->SetXYZ (pos.x, pos.y, pos.z);
					}
					retval = theDisplay->GetDisplay();
				}
            }
		}
		break;
		
	case wcGbuWpn:
		{
			SensorClass* laserPod = FindLaserPod (Sms->ownship);
			
			if (laserPod)
			{
				if (!laserPod->GetDisplay())
				{
					if (privateDisplay)
					{
 						laserPod->DisplayInit(((Render2D*)(privateDisplay))->GetImageBuffer());
					}
					laserPod->viewPoint = viewPoint;
				}
				retval = laserPod->GetDisplay();
			}
		}
		break;
		
	default:
		if (hadCamera)
            needCamera = TRUE;
		hadCamera = FALSE;
		break;
	}
	return (retval);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::UpdateGroundSpot(void)
{
	int i;
	float rx, ry, rz;
	MissileClass* theMissile = (MissileClass*)(Sms->curWeapon);

	// 2002-01-28 ADDED BY S.G. Keep a note on the previous missile if we currently have no missile selected...
	if (thePrevMissile) {
		// If it's a new missile, switch to it
		if (theMissile && thePrevMissile != theMissile) {
			if (thePrevMissileIsRef)
				VuDeReferenceEntity((VuEntity *)thePrevMissile);
			thePrevMissile = theMissile;
			if (thePrevMissile->RefCount() > 0) {
				VuReferenceEntity((VuEntity *)thePrevMissile);
				thePrevMissileIsRef = TRUE;
			}
			else
				thePrevMissileIsRef = FALSE;
		}
	}
	// If we have a missile now, keep note of it for later
	else if (theMissile) {
		thePrevMissile = theMissile;
		if (theMissile->RefCount() > 0) {
			VuReferenceEntity((VuEntity *)thePrevMissile);
			thePrevMissileIsRef = TRUE;
		}
	}

	// If our previous missile is now dead
	if (thePrevMissile && thePrevMissile != theMissile && thePrevMissile->IsDead()) {
		if (thePrevMissileIsRef)
			VuDeReferenceEntity((VuEntity *)thePrevMissile);
		thePrevMissileIsRef = FALSE;
		thePrevMissile = NULL;
	}

	// Now check if it's time to reference our thePrevMissile
	if (thePrevMissile && thePrevMissileIsRef == FALSE && thePrevMissile->RefCount() > 0) {
		VuReferenceEntity((VuEntity *)thePrevMissile);
		thePrevMissileIsRef = TRUE;
	}

	// If we don't currently have a missile but had one, use it.
	if (thePrevMissile && !theMissile)
		theMissile = thePrevMissile;
	// END OF ADDED SECTION

	// If a weapon has the spot, use it
	if (Sms->curHardpoint > 0)
	{
		switch (Sms->hardPoint[Sms->curHardpoint]->GetWeaponClass())
		{
		case wcAgmWpn:
            if (Sms->curWeaponType == wtAgm65)
            {
				if (theMissile && theMissile->IsMissile())
				{
					theMissile->GetTargetPosition (&rx, &ry, &rz);
					if (rx < 0.0F || ry < 0.0F)
						SetGroundSpotPos (Sms->ownship->XPos(), Sms->ownship->YPos(), Sms->ownship->ZPos());
					else
						SetGroundSpotPos (rx, ry, rz);
					hadCamera = TRUE;
				}
            }
			break;
			
		case wcHARMWpn:
			if (theMissile && theMissile->IsMissile())
			{
				theMissile->GetTargetPosition (&rx, &ry, &rz);
				if (rx < 0.0F)
					SetGroundSpotPos (Sms->ownship->XPos(), Sms->ownship->YPos(), Sms->ownship->ZPos());
				else
					SetGroundSpotPos (rx, ry, rz);
				hadCamera = TRUE;
			}
			break;
			
		case wcGbuWpn:
			{
				SensorClass* laserPod = FindLaserPod (Sms->ownship);
				
				if (laserPod)
				{
					((LaserPodClass*)laserPod)->GetTargetPosition (&rx, &ry, &rz);
					if (rx < 0.0F || ry < 0.0F)
						SetGroundSpotPos (Sms->ownship->XPos(), Sms->ownship->YPos(), Sms->ownship->ZPos());
					else
						SetGroundSpotPos (rx, ry, rz);
					hadCamera = TRUE;
				}
			}
			break;
			
		default:
            if (hadCamera)
				needCamera = TRUE;
            hadCamera = FALSE;
			break;
		}
	}
	
	// Add the camera if needed
	// edg: only do it if the display is in the cockpit
	if (needCamera && OTWDriver.DisplayInCockpit() )
	{
		needCamera = FALSE;
		groundSpot->EntityDriver()->Exec (vuxGameTime);
		for (i=0; i<FalconLocalSession->CameraCount(); i++)
		{
			if (FalconLocalSession->Camera(i) == groundSpot)
				break;
		}
		
		if (i == FalconLocalSession->CameraCount())
		{
			FalconLocalSession->AttachCamera (groundSpot->Id());
		}
	}
	else
	{
		for (i=0; i<FalconLocalSession->CameraCount(); i++)
		{
			if (FalconLocalSession->Camera(i) == groundSpot)
			{
				FalconLocalSession->RemoveCamera(groundSpot->Id());
				break;
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::SetGroundSpotPos (float x, float y, float z)
{
	F4Assert (groundSpot);
	
	groundSpot->SetPosition (x, y, z);
	
	needCamera = TRUE;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::DisplayInit (ImageBuffer* image)
{
	DisplayExit();
	
	privateDisplay = new Render2D;
	((Render2D*)privateDisplay)->Setup (image);
	
	privateDisplay->SetColor (0xff00ff00);
	isDisplayed = TRUE;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::DisplayExit (void)
{
	isDisplayed = FALSE;
	
	DrawableClass::DisplayExit();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::Display (VirtualDisplay* newDisplay)
{
    display = newDisplay;
    FireControlComputer *FCC = Sms->ownship->GetFCC();

// MN get aircrafts maximum ripple count
	maxripple = ((AircraftClass*)(Sms->ownship))->af->GetMaxRippleCount();

    if (!((AircraftClass*)(Sms->ownship))->HasPower(AircraftClass::SMSPower)) {
	if (displayMode != Off) {
	    lastMode = displayMode;
	    displayMode = Off;
	}
    }
    else {
	if (displayMode == Off)
	    displayMode = lastMode;
    }

    switch (displayMode)
    {
    case Off:
	{
	display->TextCenter(0.0f, 0.2f, "SMS");
	int ofont = display->CurFont();
	display->SetFont(2);
	display->TextCenter(0.0f, -0.2f, "OFF");
	display->SetFont(ofont);
	BottomRow ();
	}
	break;
    case Inv:
	InventoryDisplay(FALSE);
	break;
	
    case SelJet:
	SelectiveJettison();
	break;

	//MI
	case EmergJet:
		EmergJetDisplay();
	break;

	//MI
	case InputMode:
		InputDisplay();
	break;
	
    case Wpn:
	// common stuff.
	if (FCC->IsNavMasterMode()) {// shouldn't happened really
	    SetDisplayMode(Inv);
	    InventoryDisplay(FALSE);
	}
	else if (FCC->GetMasterMode() == FireControlComputer::Dogfight)
            DogfightDisplay();
	else if (FCC->GetMasterMode() == FireControlComputer::MissileOverride)
            MissileOverrideDisplay();
	else
	{
            if (Sms->curHardpoint < 0)
            {
		TopRow(0);
		LabelButton (5, "AAM");
		LabelButton (6, "AGM");
		LabelButton (7, "A-G");
		LabelButton (8, "GUN");
		BottomRow ();
            }
            else
            {
		switch (Sms->hardPoint[Sms->curHardpoint]->GetWeaponClass())
		{
		case wcAimWpn:
		case wcAgmWpn:
		case wcSamWpn:
		case wcHARMWpn:
		    MissileDisplay();
		    break;
		    
		case wcGunWpn:
		    GunDisplay();
		    break;
		    
		case wcBombWpn:
		case wcRocketWpn:
		    BombDisplay();
		    break;
		    
		case wcGbuWpn:
			if(!g_bRealisticAvionics)
			    GBUDisplay();	
			else
				BombDisplay();
		    break;
		    
		case wcCamera:
		    CameraDisplay();
		    break;
		    
		default:
		    TopRow(0);
		    LabelButton (5, "AAM");
		    LabelButton (6, "AGM");
		    LabelButton (7, "A-G");
		    LabelButton (8, "GUN");
		    BottomRow ();
		    break;
		}
            }
	}
	break;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::SetDisplayMode (SmsDisplayMode newDisplayMode)
{
    // 2000-08-26 MODIFIED BY S.G. SO WE SAVE THE JETTISON SELECTION
    // hardPointSelected = 0; MOVED WITHIN THE CASE
    // EXCEPT FOR SelJet WERE WE READ oldp01[5] INSTEAD
    switch (newDisplayMode)
    {
    case Inv:
	hardPointSelected = 0;
	break;
	
    case Wpn:
	hardPointSelected = 0;
	break;
	
    case SelJet:
	lastMode = displayMode;
	//		hardPointSelected = ((AircraftClass *)Sms->ownship)->af->oldp01[5];
	hardPointSelected = sjHardPointSelected;	// Until I fixe oldp01 being private
	break;
    }
    //	hardPointSelected = 0;
    displayMode = newDisplayMode;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::StepDisplayMode (void)
{
// 2000-08-26 MODIFIED BY S.G. SO WE SAVE THE JETTISON SELECTION
// hardPointSelected = 0; MOVED WITHIN THE CASE
// case SelJet ADDED AS WELL
// EXCEPT FOR SelJet WERE WE READ oldp01[5] INSTEAD
	switch (displayMode)
	{
	case Off:
	    break;
	case Wpn:
		displayMode = Inv;
		hardPointSelected = 0;
		break;
		
	case SelJet:
//		hardPointSelected = ((AircraftClass *)Sms->ownship)->af->oldp01[5];
		hardPointSelected = sjHardPointSelected;	// Until I fixe oldp01 being private
		break;

	case Inv:
	default:
		displayMode = Wpn;
		hardPointSelected = 0;
		break;
	}
//	hardPointSelected = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::PushButton (int whichButton, int whichMFD)
{
    switch (displayMode) {
    case Off:
	MfdDrawable::PushButton(whichButton, whichMFD);
	break;
    case Wpn:
	WpnPushButton (whichButton, whichMFD);
	break;
    case Inv:
	InvPushButton (whichButton, whichMFD);
	break;
    case SelJet:
	SJPushButton (whichButton, whichMFD);
	break;
	//MI
	case InputMode:
	InputPushButton(whichButton, whichMFD);
	break;
    }
}



// Weapon main handling - handles common cases and then hands off
void SmsDrawable::WpnPushButton (int whichButton, int whichMFD)
{
    
    FireControlComputer* pFCC = Sms->ownship->GetFCC();
    switch(whichButton) { // common controls.
	//MI
	case 1:
		if(g_bRealisticAvionics)
		{
			if(Sms->curWeaponType == wtAgm65 && Sms->curWeapon)
			{
				Sms->StepMavSubMode();
			}
			else if(pFCC->GetMasterMode() == FireControlComputer::Dogfight || pFCC->GetMasterMode() ==
				FireControlComputer::MissileOverride)
			{
				WpnOtherPushButton(whichButton, whichMFD);
			}
			else if(pFCC->GetMainMasterMode() == MM_AA)
				WpnAAPushButton(whichButton, whichMFD);
			else if(pFCC->GetMainMasterMode() == MM_AG)
			{
				if (IsSet(MENUMODE))
					WpnAGMenu(whichButton, whichMFD);
				else
					WpnAGPushButton(whichButton, whichMFD);
			}
		}
		else
		{
			switch(pFCC->GetMainMasterMode()) 
			{
			case MM_AG:
				if (IsSet(MENUMODE))
					WpnAGMenu(whichButton, whichMFD);
				else
					WpnAGPushButton(whichButton, whichMFD);
			break;
			case MM_AA:
				WpnAAPushButton(whichButton, whichMFD);
			break;
			case MM_NAV:
				WpnNavPushButton(whichButton, whichMFD);
			break;
			default:
				WpnOtherPushButton(whichButton, whichMFD);
			break;
			}
		} 
	break;
    case 3:
	UnsetFlag(MENUMODE); // force out of menu mode JPO
	SetDisplayMode(Inv);
	break;
    case 10:
	UnsetFlag(MENUMODE); // force out of menu mode JPO
	SetDisplayMode (SelJet);
	break;
    case 11:
	if (g_bRealisticAvionics) {
	    MfdDrawable::PushButton(whichButton, whichMFD);
	}
	break;
	
    case 12:
	if (g_bRealisticAvionics) {
	    MfdDrawable::PushButton(whichButton, whichMFD);
	}
	else 
	{
	    MfdDisplay[whichMFD]->SetNewMode(MFDClass::FCRMode);
	}
	break;
	
    case 13:
	if (g_bRealisticAvionics) {
	    MfdDrawable::PushButton(whichButton, whichMFD);
	}
	else if(pFCC->GetMasterMode() == FireControlComputer::ILS ||
	    pFCC->GetMasterMode() == FireControlComputer::Nav)
	{
	    MfdDisplay[whichMFD]->SetNewMode(MFDClass::MfdMenu);
	}
	else
	{	
	    SetDisplayMode(Wpn);
	    pFCC->SetMasterMode( FireControlComputer::Nav );
	}
	break;
	
    case 14:
	MfdDrawable::PushButton(whichButton, whichMFD);
	break;
	//MI
	case 19:
		if(g_bRealisticAvionics)
		{
			if(Sms->FEDS)
				Sms->FEDS = FALSE;
			else
				Sms->FEDS = TRUE;
		}
    default:
	switch(pFCC->GetMainMasterMode()) {
	case MM_AG:
	    if (IsSet(MENUMODE))
		WpnAGMenu(whichButton, whichMFD);
	    else
		WpnAGPushButton(whichButton, whichMFD);
	    break;
	case MM_AA:
	    WpnAAPushButton(whichButton, whichMFD);
	    break;
	case MM_NAV:
	    WpnNavPushButton(whichButton, whichMFD);
	    break;
	default:
	    WpnOtherPushButton(whichButton, whichMFD);
	    break;
	}
    }
}

// Air to Ground buttons
void SmsDrawable::WpnAGPushButton (int whichButton, int whichMFD)
{
    FireControlComputer* pFCC = Sms->ownship->GetFCC();
    switch (whichButton)
    {
    case 0:   // in AG mode, toggle between guns and bombs
		if (pFCC->GetMasterMode() == FireControlComputer::Gun)
		{
			Sms->GetNextWeapon(wdGround);
			//MI
			if(g_bRealisticAvionics)
			{
				SetWeapParams();
			}
		}
		else if (Sms->FindWeaponClass(wcGunWpn)) 
		{
			pFCC->SetMasterMode(FireControlComputer::Gun);
			pFCC->SetSubMode(FireControlComputer::STRAF);
		}
	break;
    case 1:
	if (g_bRealisticAvionics)
	{
		//MI temporary until we can get the LGB's going
		if(pFCC->GetMasterMode() == FireControlComputer::AirGroundLaser)
			break;
				
	    ToggleFlag(MENUMODE);
	}
	else
	    pFCC->NextSubMode();
	break;
	
    case 2:
	if (Sms->curWeaponType == wtAgm65 && Sms->curWeapon)
	{
	    ShiAssert(Sms->curWeapon->IsMissile());
	    MaverickDisplayClass* mavDisplay =
		(MaverickDisplayClass*)((MissileClass*)Sms->curWeapon)->display;
	    
            if (mavDisplay)
		mavDisplay->ToggleFOV();
	}
	else if (Sms->curWeaponClass == wcGbuWpn)
	{
	    LaserPodClass* laserPod = (LaserPodClass* )FindLaserPod (Sms->ownship);
	    
            if (laserPod)
            {
		laserPod->ToggleFOV();
            }
	}
	break;

	//MI
	case 4:
		if(g_bRealisticAvionics)
		{
			if((pFCC->GetMasterMode() == FireControlComputer::AirGroundBomb) || //me123 status test. addet four lines
				(pFCC->GetMasterMode() == FireControlComputer::AirGroundLaser))
			{
				ChangeToInput(whichButton);
			}
		}
	break;
	
    case 5:
	Sms->GetNextWeapon(wdGround);
	//MI
	if(g_bRealisticAvionics)
	{
		SetWeapParams();
	}
	break;

	//MI
	case 6:	
		if(!g_bRealisticAvionics || Sms->CurHardpoint() < 0)
			break;

		if((pFCC->GetMasterMode() == FireControlComputer::AirGroundBomb) || //me123 status test. addet four lines
			(pFCC->GetMasterMode() == FireControlComputer::AirGroundLaser))
		{ 
			ChangeProf();
			if(Sms->Prof1)
			{
				Sms->rippleCount = Sms->Prof1RP;
				Sms->rippleInterval = Sms->Prof1RS;
				Sms->SetPair(Sms->Prof1Pair);
				SetWeapParams();
			}
			else
			{
				Sms->rippleCount = Sms->Prof2RP;
				Sms->rippleInterval = Sms->Prof2RS;
				Sms->SetPair(Sms->Prof2Pair);
				SetWeapParams();
			} 
		}
		else if(pFCC->GetMasterMode() == FireControlComputer::AirGroundMissile &&
			Sms->curWeaponType == wtAgm65)
		{
			Sms->ToggleMavPower();
		}
    break;
    case 7:
		if(g_bRealisticAvionics && Sms->CurHardpoint() >= 0)	//MI
		{
			if((pFCC->GetMasterMode() == FireControlComputer::AirGroundBomb) || //me123 status test. addet four lines
				(pFCC->GetMasterMode() == FireControlComputer::AirGroundLaser))
			{
				if (Sms->pair)
				{
					if(Sms->Prof1)
						Sms->Prof1Pair = FALSE;
					else
						Sms->Prof2Pair = FALSE;

					Sms->SetPair(FALSE);
				}
				else
				{
					if(Sms->Prof1)
						Sms->Prof1Pair = TRUE;
					else
						Sms->Prof2Pair = TRUE;

					Sms->SetPair(TRUE);
				}
			}
		}
		else
		{
			if(pFCC->GetMasterMode() == FireControlComputer::AirGroundBomb) //me123 status test. addet four lines
			{
				if (Sms->pair)
					Sms->SetPair(FALSE);
				else
					Sms->SetPair(TRUE);
			}
		}
	break;
	
    case 8:
		if(g_bRealisticAvionics && Sms->CurHardpoint() >= 0)	//MI
		{
			if((pFCC->GetMasterMode() == FireControlComputer::AirGroundBomb) || //me123 status test. addet four lines
				(pFCC->GetMasterMode() == FireControlComputer::AirGroundLaser))
			{
				ChangeToInput(whichButton);
			}
		}
		else
		{
			if(pFCC->GetMasterMode() == FireControlComputer::AirGroundBomb) //me123 status test. addet four lines
				Sms->IncrementRippleInterval();
		}
	break;
	
    case 9:
		if(g_bRealisticAvionics && Sms->CurHardpoint() >= 0)	//MI
		{
			if(pFCC->GetMasterMode() == FireControlComputer::AirGroundMissile)
				pFCC->WeaponStep();
			else if((pFCC->GetMasterMode() == FireControlComputer::AirGroundBomb) || //me123 status test. addet four lines
				(pFCC->GetMasterMode() == FireControlComputer::AirGroundLaser))
			{
				ChangeToInput(whichButton);
			}
		}
		else
		{
			if(pFCC->GetMasterMode() == FireControlComputer::AirGroundMissile)
				pFCC->WeaponStep();
			else if(pFCC->GetMasterMode() == FireControlComputer::AirGroundBomb)
				Sms->IncrementRippleCount();
		}
	break;

    case 15:
		if(g_bRealisticAvionics && Sms->CurHardpoint() >= 0)	//MI
		{
			if(pFCC->GetMasterMode() == FireControlComputer::AirGroundMissile)
				pFCC->WeaponStep();
			else if((pFCC->GetMasterMode() == FireControlComputer::AirGroundBomb) || //me123 status test. addet four lines
				(pFCC->GetMasterMode() == FireControlComputer::AirGroundLaser)) //MI LGB's
			{
				if(g_bMLU)
					ChangeToInput(whichButton);
			}
		}
		else
		{
			if(pFCC->GetMasterMode() == FireControlComputer::AirGroundMissile)
				pFCC->WeaponStep();
			else if(pFCC->GetMasterMode() == FireControlComputer::AirGroundBomb) //me123 status test. addet four lines
				Sms->Incrementarmingdelay();
		}
	break;
	//MI
	case 17:
		if(g_bRealisticAvionics && Sms->CurHardpoint() >= 0)
		{
			if(pFCC->GetMasterMode() == FireControlComputer::AirGroundBomb)
			{
				if(Sms->Prof1)
				{
					Sms->Prof1NSTL++;
					if(Sms->Prof1NSTL > 2)
						Sms->Prof1NSTL = 0;
				}
				else
				{
					Sms->Prof2NSTL++;
					if(Sms->Prof2NSTL > 2)
						Sms->Prof2NSTL = 0;
				}
				SetWeapParams();
			}
		}
	break;	
    case 18:
	if (Sms->curHardpoint >= 0 &&
	    Sms->hardPoint[Sms->curHardpoint]->GetWeaponData()->flags & SMSClass::HasBurstHeight)
	{
		//MI not here in real
		if(g_bRealisticAvionics)
			break;
		Sms->IncrementBurstHeight();
	}
	else if (Sms->curWeaponType == wtAgm88)
	{
	    // HTS used HSD display range for now, so...
	    SimHSDRangeStepDown (0, KEY_DOWN, NULL);
	}
	else if(g_bRealisticAvionics && pFCC->GetMasterMode() == FireControlComputer::AirGroundMissile && Sms->curWeaponType ==
		wtAgm65)
		pFCC->WeaponStep();
	break;
	
    case 19:
		//MI
		if(g_bRealisticAvionics)
		{
			if(pFCC->GetMasterMode() == FireControlComputer::AirGroundMissile)
			{ 
				//Doc says it's been deleted from the real thing
				//pFCC->NextSubMode();
			}
			else if (Sms->curWeaponType == wtAgm88)
			{ 
				// HTS used HSD display range for now, so...
				SimHSDRangeStepUp (0, KEY_DOWN, NULL);
			}
			break;
		}
		if (pFCC->GetMasterMode() == FireControlComputer::AirGroundMissile ||
			(pFCC->GetMasterMode() == FireControlComputer::AirGroundLaser &&
			Sms->curWeaponClass == wcGbuWpn))
		{
			pFCC->NextSubMode();
		}
		else if (Sms->curWeaponType == wtAgm88)
		{
			// HTS used HSD display range for now, so...
			SimHSDRangeStepUp (0, KEY_DOWN, NULL);
		}
   break;
   }
}

// Handle Menu mode
void SmsDrawable::WpnAGMenu (int whichButton, int whichMFD)
{
	RadarDopplerClass* pradar = (RadarDopplerClass*) FindSensor (SimDriver.playerEntity, SensorClass::Radar);
    FireControlComputer* pFCC = Sms->ownship->GetFCC();
	if(pradar)	//MI fix
		pradar->SetScanDir(1.0F);
    switch(whichButton) {
    case 19:
	pFCC->SetSubMode(FireControlComputer::CCIP);
	//MI store where we are
	pFCC->lastAgSubMode = FireControlComputer::CCIP;
	if(Sms->Prof1)
		Sms->Prof1SubMode = FireControlComputer::CCIP;
	else
		Sms->Prof2SubMode = FireControlComputer::CCIP;
	break;
    case 18:
	pFCC->SetSubMode(FireControlComputer::CCRP);
	//MI store where we are
	pFCC->lastAgSubMode = FireControlComputer::CCRP;
	if(Sms->Prof1)
		Sms->Prof1SubMode = FireControlComputer::CCRP;
	else
		Sms->Prof2SubMode = FireControlComputer::CCRP;
	pradar->SelectLastAGMode();
	break;
    case 17:
	pFCC->SetSubMode(FireControlComputer::DTOSS);
	//MI store where we are
	pFCC->lastAgSubMode = FireControlComputer::DTOSS;
	if(Sms->Prof1)
		Sms->Prof1SubMode = FireControlComputer::DTOSS;
	else
		Sms->Prof2SubMode = FireControlComputer::DTOSS;
	break;
    case 16:
	pFCC->SetSubMode(FireControlComputer::LADD);
	//MI store where we are
	pFCC->lastAgSubMode = FireControlComputer::LADD;
	if(Sms->Prof1)
		Sms->Prof1SubMode = FireControlComputer::LADD;
	else
		Sms->Prof2SubMode = FireControlComputer::LADD;
	break;
    case 15:
	pFCC->SetSubMode(FireControlComputer::MAN);
	//MI store where we are
	pFCC->lastAgSubMode = FireControlComputer::MAN;
	if(Sms->Prof1)
		Sms->Prof1SubMode = FireControlComputer::MAN;
	else
		Sms->Prof2SubMode = FireControlComputer::MAN;
	break;
    default:
	return; // nothing interesting
    }
    UnsetFlag(MENUMODE);
}

// Handle other cases - basically missile override and dogfight.
void SmsDrawable::WpnOtherPushButton (int whichButton, int whichMFD)
{
    FireControlComputer* pFCC = Sms->ownship->GetFCC();

    switch (whichButton)
    {
    case 0:
	break;
    case 1:
	if(pFCC->GetMasterMode() == FireControlComputer::MissileOverride && g_bRealisticAvionics)
	{
		if(Sms->curWeaponType == wtGuns)
		{
			if(pFCC->GetSubMode() == FireControlComputer::EEGS)
				pFCC->SetSubMode(FireControlComputer::LCOS);
			else if(pFCC->GetSubMode() == FireControlComputer::LCOS)
				pFCC->SetSubMode(FireControlComputer::Snapshot);
			else
				pFCC->SetSubMode(FireControlComputer::EEGS);
			//MI remember this
			pFCC->lastAAGunSubMode = pFCC->GetSubMode();
		}
		break;
	}
	if(pFCC->GetSubMode() == FireControlComputer::EEGS)
	{
		pFCC->SetSubMode(FireControlComputer::LCOS);
		//MI remember what we have
		pFCC->lastDFGunSubMode = FireControlComputer::LCOS;
	}
	//MI we want SNAP here too
	else if(pFCC->GetSubMode() == FireControlComputer::LCOS)
	{
		pFCC->SetSubMode(FireControlComputer::Snapshot);
		//MI remember what we have
		pFCC->lastDFGunSubMode = FireControlComputer::Snapshot;
	}
	else
	{
		pFCC->SetSubMode(FireControlComputer::EEGS);
		//MI remember what we have
		pFCC->lastDFGunSubMode = FireControlComputer::EEGS;
	}
	break;
	
    case 5:
	if (pFCC->GetMasterMode() == FireControlComputer::Dogfight)
	    break; // its a gun in this mode.
	//MI 30/01/02
//	Sms->GetNextWeaponSpecific(wdAir);
	//MI 30/01/02
	if(!g_bRealisticAvionics || !(Sms && Sms->Ownship() && ((AircraftClass *)Sms->ownship)->IsF16()))
		Sms->GetNextWeapon(wdAir);
	else
	{
		Sms->StepWeaponClass();
		if(Sms->curWeaponType == wtAim120)
			pFCC->SetSubMode(FireControlComputer::Aim120);
		else if(Sms->curWeaponType == wtAim9)
			pFCC->SetSubMode(FireControlComputer::Aim9);
		else if(Sms->curWeaponType == wtGuns)
			pFCC->SetSubMode(pFCC->lastAAGunSubMode);
	}
	break;
	
    case 6:
	if (pFCC->GetMasterMode() == FireControlComputer::MissileOverride)
	    break; // nothing here.
	//MI 30/01/02
	if(Sms && Sms->Ownship() && ((AircraftClass *)Sms->ownship)->IsF16())
	{
		Sms->StepWeaponClass();
		if(Sms->curWeaponType == wtAim120)
			pFCC->SetDgftSubMode(FireControlComputer::Aim120);
		else if(Sms->curWeaponType == wtAim9)
			pFCC->SetDgftSubMode(FireControlComputer::Aim9);
	}
	else
	{
		if(Sms->curWeaponType == wtAim120) 
		{
			if(!Sms->FindWeaponType(wtAim9))
			{
				Sms->SetWeaponType(wtAim120);
				//remember our mode!
				pFCC->SetDgftSubMode(FireControlComputer::Aim120);
			}
			else
			{
				Sms->SetWeaponType(wtAim9);
				pFCC->SetDgftSubMode(FireControlComputer::Aim9);
			}
		}
		else 
		{
			if (!Sms->FindWeaponType(wtAim120))
			{
				Sms->SetWeaponType(wtAim9);
				pFCC->SetDgftSubMode(FireControlComputer::Aim9);
			}
			else
			{
				Sms->SetWeaponType(wtAim120);
				pFCC->SetDgftSubMode(FireControlComputer::Aim120);
			}
		}
	}
	break;
	
    case 9:
    case 15:
        pFCC->WeaponStep();
	break;
	
    default:
	WpnAAMissileButton(whichButton, whichMFD);
	break;
   }
}

// Navigation mode - should never happen
void SmsDrawable::WpnNavPushButton (int whichButton, int whichMFD)
{
    MonoPrint("Sms Weapon in Nav mode\n");
    // shouldn't happen.
}

// AA mode.
void SmsDrawable::WpnAAPushButton (int whichButton, int whichMFD)
{
    FireControlComputer* pFCC = Sms->ownship->GetFCC();

    switch (whichButton)
    {
    case 0: // in AA between guns and missiles.
	if (pFCC->GetMasterMode() == FireControlComputer::Gun)
	    Sms->GetNextWeapon(wdAir);
	else
	    pFCC->SetMasterMode(FireControlComputer::Gun);
	break;
    case 1:
	if (pFCC->GetMasterMode() == FireControlComputer::Gun)
	{
	    pFCC->NextSubMode();
		//MI remember this
		pFCC->lastAAGunSubMode = pFCC->GetSubMode();
	}
	break;
	
    case 5: // move to next weapon type
		//MI 18/01/02 this appears to be wrong, as I can't get out of GUN mode when I'm in there
//	if(pFCC->GetMasterMode() == FireControlComputer::Missile)
		//MI 30/01/02
		// 2002-02-08 MODIFIED BY S.G. Add if curHardpoint < 0, call Sms->GetNextWeapon(wdAir) to find something if air type since Sms->StepWeaponClass() isn't handling that case
		if((Sms && Sms->CurHardpoint() < 0) || !g_bRealisticAvionics || !(Sms && Sms->Ownship() && ((AircraftClass *)Sms->ownship)->IsF16()))
			Sms->GetNextWeapon(wdAir);
		else
			Sms->StepWeaponClass();
	break;
	
    case 9:
    case 15:
	if(pFCC->GetMasterMode() == FireControlComputer::Missile)
	    pFCC->WeaponStep();
	break;
	
    default:
	if(pFCC->GetMasterMode() == FireControlComputer::Missile)
	    WpnAAMissileButton(whichButton, whichMFD);
	break;
   }
}

void SmsDrawable::SJPushButton (int whichButton, int whichMFD)
{
    
    FireControlComputer* pFCC = Sms->ownship->GetFCC();

    switch (whichButton)
    {
    case 2:
	if (Sms->hardPoint[5]->GetWeaponClass() != wcECM &&
	    Sms->hardPoint[5]->GetWeaponClass() != wcCamera)
	{
            hardPointSelected ^= (1 << 5);
			sjHardPointSelected = hardPointSelected;
	}
	break;
	
    case 6:
	hardPointSelected ^= (1 << 6);
	sjHardPointSelected = hardPointSelected;
	break;
	
    case 7:
	hardPointSelected ^= (1 << 7);
	sjHardPointSelected = hardPointSelected;
	break;
	
    case 8:
	hardPointSelected ^= (1 << 8);
	sjHardPointSelected = hardPointSelected;
	break;
	
    case 10:
	{
	    SetDisplayMode (lastMode);
	}
	break;
    case 11:
	if (g_bRealisticAvionics) {
	    MfdDrawable::PushButton(whichButton, whichMFD);
	}
	break;
	
    case 12:
	if (g_bRealisticAvionics) {
	    MfdDrawable::PushButton(whichButton, whichMFD);
	}
	else
	{
	    SetDisplayMode(Wpn);
	}
	break;
	
    case 13:
	if (g_bRealisticAvionics) {
	    MfdDrawable::PushButton(whichButton, whichMFD);
	}
	else if(pFCC->GetMasterMode() == FireControlComputer::ILS ||
	    pFCC->GetMasterMode() == FireControlComputer::Nav)
	{
	    SetDisplayMode(Wpn);
	    pFCC->SetMasterMode( FireControlComputer::Nav );
	}
	else
	{	
	    SetDisplayMode(Wpn);
	    pFCC->SetMasterMode( FireControlComputer::Nav );
	}
	break;
	
    case 14:
	MfdDrawable::PushButton(whichButton, whichMFD);
	break;
	
    case 15:
	break;
	
    case 16:
	    hardPointSelected ^= (1 << 2);
		sjHardPointSelected = hardPointSelected;
	break;
	
    case 17:
	    hardPointSelected ^= (1 << 3);
		sjHardPointSelected = hardPointSelected;
	break;
	
    case 18:
	    hardPointSelected ^= (1 << 4);
		sjHardPointSelected = hardPointSelected;
	break;
	
    case 19:
	break;
   }
	//MI commented out to fix SJ remembering stuff
	//sjHardPointSelected = hardPointSelected;
   // 2000-08-26 ADDED BY S.G. SO WE SAVE THE JETTISON SELECTION
   //	if (displayMode == SelJet)	// Until I fixe oldp01 being private
   //		((AircraftClass *)Sms->ownship)->af->oldp01[5] = hardPointSelected;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::DogfightDisplay (void)
{
	GunDisplay();
	ShowMissiles(6);
	// Marco Edit - SLAVE/BORE Mode
    SimWeaponClass* wpn; 
	wpn = Sms->curWeapon;
	
	//MI 29/01/02 changed to be more accurate
	if(g_bRealisticAvionics)
	{
		ShiAssert(Sms->curWeapon && Sms->curWeapon->IsMissile());
		if(Sms->curWeaponType == wtAim9 && ((CampBaseClass*)wpn)->GetSPType() != SPTYPE_AIM9P)
		{
			// JPO new Aim9 code
			switch(Sms->GetCoolState()) 
			{
			case SMSClass::WARM:
				LabelButton (7, "WARM"); // needs cooling
				break;
			case SMSClass::COOLING:
				LabelButton (7, "WARM", NULL, 1); // is cooling
				break;
			case SMSClass::COOL:
				LabelButton (7, "COOL");
				break;
			case SMSClass::WARMING:
				LabelButton (7, "COOL", NULL, 1); // Is warming back up
				break;
			}
			
			if(((MissileClass*)Sms->curWeapon)->isSlave)
			{
				LabelButton (18, "SLAV");
			}
			else
			{
				LabelButton (18, "BORE");
			}
			// Marco Edit - TD/BP Mode
			if(((MissileClass*)Sms->curWeapon)->isTD && ((CampBaseClass*)wpn)->GetSPType() != SPTYPE_AIM9P)
			{
				LabelButton (17, "TD");
			}
			else if(((CampBaseClass*)wpn)->GetSPType() != SPTYPE_AIM9P)
			{
				LabelButton (17, "BP");
			}

			// Marco Edit - SPOT/SCAN Mode
			if(((MissileClass*)Sms->curWeapon)->isSpot && ((CampBaseClass*)wpn)->GetSPType() != SPTYPE_AIM9P)
			{
				LabelButton (2, "SPOT");
			}
			else if(((CampBaseClass*)wpn)->GetSPType() != SPTYPE_AIM9P)
			{
				LabelButton (2, "SCAN");
			}
		}
		else if(Sms->curWeaponType == wtAim120)
		{
			if (Sms->curWeaponType == wtAim120 &&
				Sms->MasterArm() == SMSBaseClass::Safe ||
				Sms->MasterArm() == SMSBaseClass::Sim) 
			{ // JPO new AIM120 code
				LabelButton(7, "BIT");
				LabelButton(8, "ALBIT");
			}
			char idnum[10];
			sprintf(idnum, "%d", Sms->AimId());
			LabelButton (16, "ID", idnum);
			LabelButton (17, "TM", "ON");

			if(Sms && Sms->curWeapon && ((MissileClass*)Sms->curWeapon)->isSlave)
			{
				LabelButton (18, "SLAV");
			}
			else
			{
				LabelButton (18, "BORE");
			}
		}
	}
	// AIM9Ps only have Bore/Slave to worry about
	else if (Sms->curWeapon && ((CampBaseClass*)wpn)->GetSPType() == SPTYPE_AIM9P )
	{
	    ShiAssert(Sms->curWeapon->IsMissile());

		if (((MissileClass*)Sms->curWeapon)->isSlave)
		{
			LabelButton (18, "SLAV");
		}
		else
		{
			LabelButton (18, "BORE");
		}
	}
	else if (Sms->curWeapon && ((CampBaseClass*)wpn)->GetSPType() != SPTYPE_AIM9P )
	{
		// Keep non-full avionics mode as is display wise
		ShiAssert(Sms->curWeapon->IsMissile());

		if (((MissileClass*)Sms->curWeapon)->isCaged)
		{
			LabelButton (18, "SLAV");
		}
		else
		{
			LabelButton (18, "BORE");
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::MissileOverrideDisplay (void)
{
    TopRow(0);
    BottomRow ();
	//MI 19/01/02
	// JPO ID and Telemetry readouts.
	// Marco Edit - SLAVE/BORE Mode
    SimWeaponClass* wpn = NULL;
	ShiAssert(Sms);
	wpn = Sms->curWeapon;
	
	//MI changed 29/01/02 to be more accurate
	if(g_bRealisticAvionics)
	{
		if(Sms->curWeaponType == wtAim120)
		{
			char idnum[10];
			sprintf(idnum, "%d", Sms->AimId());
			LabelButton (16, "ID", idnum);
			LabelButton (17, "TM", "ON");
			if(Sms->curWeapon)
			{
				ShiAssert(Sms->curWeapon->IsMissile());
				if(((MissileClass*)Sms->curWeapon)->isSlave) // && Sms->curWeaponType == wtAim9)
				{
					LabelButton (18, "SLAV");
				}
				else // if (((MissileClass*)Sms->curWeapon)->isCaged)
				{
					LabelButton (18, "BORE");
				}
			}
		}
		else if(Sms->curWeaponType == wtAim9 && wpn && ((CampBaseClass*)wpn)->GetSPType() != SPTYPE_AIM9P)
		{
			ShiAssert(Sms->curWeapon && Sms->curWeapon->IsMissile());
			// JPO new Aim9 code
			switch(Sms->GetCoolState()) 
			{
			case SMSClass::WARM:
				LabelButton (7, "WARM"); // needs cooling
				break;
			case SMSClass::COOLING:
				LabelButton (7, "WARM", NULL, 1); // is cooling
				break;
			case SMSClass::COOL:
				LabelButton (7, "COOL");
				break;
			case SMSClass::WARMING:
				LabelButton (7, "COOL", NULL, 1); // Is warming back up
				break;
			}
			
			if(Sms && Sms->curWeapon && ((MissileClass*)Sms->curWeapon)->isSlave)
			{
				LabelButton (18, "SLAV");
			}
			else
			{
				LabelButton (18, "BORE");
			}
			// Marco Edit - TD/BP Mode
			if(Sms && Sms->curWeapon && ((MissileClass*)Sms->curWeapon)->isTD && wpn && ((CampBaseClass*)wpn)->GetSPType() != SPTYPE_AIM9P)
			{
				LabelButton (17, "TD");
			}
			else if(wpn && ((CampBaseClass*)wpn)->GetSPType() != SPTYPE_AIM9P)
			{
				LabelButton (17, "BP");
			}

			// Marco Edit - SPOT/SCAN Mode
			if(Sms && Sms->curWeapon && ((MissileClass*)Sms->curWeapon)->isSpot && wpn && ((CampBaseClass*)wpn)->GetSPType() != SPTYPE_AIM9P)
			{
				LabelButton (2, "SPOT");
			}
			else if(wpn && ((CampBaseClass*)wpn)->GetSPType() != SPTYPE_AIM9P)
			{
				LabelButton (2, "SCAN");
			}
		}
		else if(Sms->curWeaponType == wtGuns)
		{
			GunDisplay();
		}
	}
	//MI
	if(g_bRealisticAvionics)
	{
		if(Sms->curWeaponType != wtGuns)
			ShowMissiles(5);
	}
	else
		ShowMissiles(5);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::MissileDisplay (void)
{
	display->SetColor (GetMfdColor(MFD_LABELS));

	switch (Sms->curWeaponType)
	{
	case wtAgm65:
		//MI
		if(!g_bRealisticAvionics)
			MaverickDisplay();
		else
			MavSMSDisplay();
		break;
		
	case wtAgm88:
		HarmDisplay();
		break;
		
	case wtAim9:
	case wtAim120:
	    TopRow(0);
	    BottomRow();
	    AAMDisplay();
		break;
	}
	
	ShowMissiles(5);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
char SmsDrawable::HdptStationSym(int n) // JPO new routine
{
    if (Sms->hardPoint[n] == NULL) return ' '; // empty hp
	if(Sms->hardPoint[n]->weaponCount <= 0) return ' ';	//MI don't bother drawing empty hardpoints
    if (Sms->StationOK(n) == FALSE) return 'F'; // malfunction on  HP
    if (Sms->hardPoint[n]->GetWeaponType() == Sms->curWeaponType)
	return '0' + n; // exact match for weapon
    if (Sms->hardPoint[n]->GetWeaponClass() == Sms->curWeaponClass)
	return 'M'; // weapon of similar class
    return ' ';
}

void SmsDrawable::ShowMissiles (int buttonNum)
{
    char tmpStr[12];
    float leftEdge;
    int i;
    float width = display->TextWidth("M ");
    
    if (Sms->curHardpoint < 0)
	return;
    
    sprintf (tmpStr, "%d%s", Sms->NumCurrentWpn(), Sms->hardPoint[Sms->curHardpoint]->GetWeaponData()->mnemonic);
    ShiAssert (strlen(tmpStr) < sizeof (tmpStr));
    LabelButton (buttonNum, tmpStr);
    if (Sms->CurStationOK())
    {
	switch (Sms->MasterArm())
	{
	case SMSBaseClass::Safe:
		//MI not here in real
		if(!g_bRealisticAvionics)
            sprintf (tmpStr, "SAF");
		else
			sprintf(tmpStr, "");
	    break;
	    
	case SMSBaseClass::Sim:
            sprintf (tmpStr, "SIM");
	    break;
	    
	case SMSBaseClass::Arm:
            sprintf (tmpStr, "RDY");
	    break;
	}
    }
    else
    {
	sprintf (tmpStr, "MAL");
    }
    float x, y;
	GetButtonPos(buttonNum, &x, &y);
	//MI
	if(g_bRealisticAvionics && Sms->curWeaponType == wtAgm65 && Sms->curWeapon)
	{
		if(!Sms->Powered || Sms->MavCoolTimer > 0.0F)
			sprintf(tmpStr,"");
	}
    display->TextCenter (0.3F, y, tmpStr);
    GetButtonPos(9, &x, &y); // use button 9 as a reference (lower rhs)
    // List stations with Current Missile
    for (i=0; i<Sms->NumHardpoints(); i++)
    {
	// JPO redone to use this routine. Returns the appropriate character
	char c = HdptStationSym(i);
	if (c == ' ') continue; // Don't bother drawing blanks.
	tmpStr[0] = c;
	tmpStr[1] = '\0';
	if (i < 6)
	{
	    leftEdge = -x + width * (i-1);
	    display->TextLeft (leftEdge, y, tmpStr, (Sms->curHardpoint == i ? 2 : 0));
	}
	else
	{
	    leftEdge = x - width * (Sms->NumHardpoints() - i - 1);
	    // Box the current station
	    display->TextRight (leftEdge, y, tmpStr, (Sms->curHardpoint == i ? 2 : 0));
	}
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::MaverickDisplay(void)
{
	FireControlComputer* pFCC = Sms->ownship->GetFCC();
	MaverickDisplayClass* mavDisplay = NULL;
	//MI
	AircraftClass *self = ((AircraftClass*)SimDriver.playerEntity);
	if(!self)
		return;
	float yOffset;
	float percentRange;
	float rMax, rMin;
	float range;
	float dx, dy, dz;
	
	if (Sms->curWeapon)
	{
	    ShiAssert(Sms->curWeapon->IsMissile());
		mavDisplay = (MaverickDisplayClass*)((MissileClass*)Sms->curWeapon)->display;

		if (mavDisplay)
		{
			mavDisplay->SetIntensity(GetIntensity());
			mavDisplay->viewPoint = viewPoint;
			mavDisplay->Display(display);
		}
		
		if (mavDisplay->IsSOI())
			LabelButton (1, "VIS", "");
		else
			LabelButton (1, "PRE", "");
	}
	
	// OSS Button Labels
	LabelButton (0, "OPER");
	//MI
	//if (mavDisplay && mavDisplay->CurFOV() > (3.0F * DTR))
	if (mavDisplay && mavDisplay->CurFOV() > (3.5F * DTR))
		LabelButton (2, "FOV");
	else
		LabelButton (2, "EXP", NULL, 1);
	LabelButton (4, "HOC");
	BottomRow ();
	LabelButton (19, pFCC->subModeString);
	
	// Mav DLZ
	if (pFCC->missileTarget)
	{
		// Range Carat / Closure
		rMax   = pFCC->missileRMax;
		rMin   = pFCC->missileRMin;
		
		// get range to ground designaate point
		dx = Sms->ownship->XPos() - pFCC->groundDesignateX;
		dy = Sms->ownship->YPos() - pFCC->groundDesignateY;
		dz = Sms->ownship->ZPos() - pFCC->groundDesignateZ;
		range = (float)sqrt( dx*dx + dy*dy + dz*dz );
		
		// Normailze the ranges for DLZ display
		percentRange = range / pFCC->missileWEZDisplayRange;
		rMin /= pFCC->missileWEZDisplayRange;
		rMax /= pFCC->missileWEZDisplayRange;
		
		// Clamp in place
		rMin = min (rMin, 1.0F);
		rMax = min (rMax, 1.0F);
		
		// Draw the symbol
		
		// Rmin/Rmax
		display->Line (0.9F, -0.8F + rMin * 1.6F, 0.95F, -0.8F + rMin * 1.6F);
		display->Line (0.9F, -0.8F + rMin * 1.6F, 0.9F,  -0.8F + rMax * 1.6F);
		display->Line (0.9F, -0.8F + rMax * 1.6F, 0.95F, -0.8F + rMax * 1.6F);
		
		// Range Caret
		yOffset = min (-0.8F + percentRange * 1.6F, 1.0F);
		
		display->Line (0.9F, yOffset, 0.9F - 0.03F, yOffset + 0.03F);
		display->Line (0.9F, yOffset, 0.9F - 0.03F, yOffset - 0.03F);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::GBUDisplay(void)
{
	flash = (vuxRealTime & 0x200);

	LaserPodClass* laserPod = (LaserPodClass*)FindLaserPod (Sms->ownship);
	FireControlComputer* pFCC = Sms->ownship->GetFCC();
		
	if (laserPod)
	{
		laserPod->SetIntensity(GetIntensity());
		laserPod->Display(display);
	}
	
	// OSS Button Labels
	LabelButton (0, "OPER");
	//MI
	//if (laserPod && laserPod->CurFOV() < (3.0F * DTR))
	if(!g_bRealisticAvionics)
	{
		if (laserPod && laserPod->CurFOV() < (3.5F * DTR))
			LabelButton (2, "EXP", NULL, 1);
		else
			LabelButton (2, "FOV");
	}
	else
	{
		if(laserPod && laserPod->CurFOV() < (3.5F * DTR))
			LabelButton (2, "NARO", NULL);
		else
			LabelButton (2, "WIDE", NULL);
	}

	BottomRow ();
	LabelButton (19, pFCC->subModeString, NULL);
	
	// Show Available wpns
	ShowMissiles(5);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::AAMDisplay(void)
{
	// Marco Edit - SLAVE/BORE Mode
    SimWeaponClass* wpn = NULL; 
	wpn = Sms->curWeapon;

	ShiAssert(wpn != NULL);
// MN return for now if no weapon found...CTD "fix"
	if (!wpn)
		return;

	if (Sms->curWeaponType == wtAim120 &&
	    Sms->MasterArm() == SMSBaseClass::Safe ||
		Sms->MasterArm() == SMSBaseClass::Sim) { // JPO new AIM120 code
	    LabelButton(7, "BIT");
	    LabelButton(8, "ALBIT");
	}
	else if (Sms->curWeaponType == wtAim9 && ((CampBaseClass*)wpn)->GetSPType() != SPTYPE_AIM9P) 
	{ // JPO new Aim9 code
	    switch (Sms->GetCoolState() ) 
		{
	    case SMSClass::WARM:
			LabelButton (7, "WARM"); // needs cooling
			break;
	    case SMSClass::COOLING:
			LabelButton (7, "WARM", NULL, 1); // is cooling
			break;
	    case SMSClass::COOL:
			LabelButton (7, "COOL");
			break;
		case SMSClass::WARMING:
			LabelButton (7, "COOL", NULL, 1); // Is warming back up
			break;
	    }	    
	}
	// JPO ID and Telemetry readouts.
	if (Sms->curWeaponType == wtAim120) {
	    char idnum[10];
	    sprintf (idnum, "%d", Sms->AimId());
	    LabelButton (16, "ID", idnum);
	    LabelButton (17, "TM", "ON");
	}
	if (g_bRealisticAvionics && Sms->curWeapon)
	{
	    ShiAssert(Sms->curWeapon->IsMissile());
		if (((MissileClass*)Sms->curWeapon)->isSlave && ((CampBaseClass*)wpn)->GetSPType() != SPTYPE_AIM9P) // && Sms->curWeaponType == wtAim9)
		{
			LabelButton (18, "SLAV");
		}
		else if(Sms->curWeaponType != wtAim9 || ((CampBaseClass*)wpn)->GetSPType() != SPTYPE_AIM9P)
		{
			LabelButton (18, "BORE");
		}
		// Marco Edit - TD/BP Mode
		if (((MissileClass*)Sms->curWeapon)->isTD && Sms->curWeaponType == wtAim9 && ((CampBaseClass*)wpn)->GetSPType() != SPTYPE_AIM9P)
		{
			LabelButton (17, "TD");
		}
		else if (Sms->curWeaponType == wtAim9 && ((CampBaseClass*)wpn)->GetSPType() != SPTYPE_AIM9P)
		{
			LabelButton (17, "BP");
		}

		// Marco Edit - SPOT/SCAN Mode
		if (((MissileClass*)Sms->curWeapon)->isSpot && Sms->curWeaponType == wtAim9 && ((CampBaseClass*)wpn)->GetSPType() != SPTYPE_AIM9P)
		{
			LabelButton (2, "SPOT");
		}
		else if (Sms->curWeaponType == wtAim9 && ((CampBaseClass*)wpn)->GetSPType() != SPTYPE_AIM9P)
		{
			LabelButton (2, "SCAN");
		}
	}
}

// generic missile handling
void SmsDrawable::WpnAAMissileButton(int whichButton, int whichMfd)
{
    switch(whichButton) {

	// Marco Edit - SPOT/SCAN Mode
	case 2:
	    if (g_bRealisticAvionics && Sms->curWeapon && Sms->curWeaponType == wtAim9) 
		{
			ShiAssert(Sms->curWeapon->IsMissile());
			//MI fixup... only toggle for the 9M
			if(((CampBaseClass*)Sms->curWeapon)->GetSPType() != SPTYPE_AIM9P)
				((MissileClass*)Sms->curWeapon)->isSpot = !((MissileClass*)Sms->curWeapon)->isSpot;
	    }
	break;

    case 7:
	if (Sms->curWeaponType == wtAim120) {
	    // code for AIM120 BIT test
	}
	//MI only with M's
	else if (Sms->curWeaponType == wtAim9 && ((CampBaseClass*)Sms->curWeapon)->GetSPType() != SPTYPE_AIM9P) 
	{
	    if (Sms->GetCoolState() == SMSClass::WARM || Sms->GetCoolState() == SMSClass::WARMING) 
		{
			/*if (Sms->aim9warmtime != 0.0)
			{
				Sms->aim9cooltime = (SimLibElapsedTime + 3 * CampaignSeconds) - ((Sms->aim9warmtime - SimLibElapsedTime) / 20); // 20 = 60/3
				Sms->aim9warmtime = 0.0;
			}
			else
			{
				Sms->aim9cooltime = SimLibElapsedTime + 3 * CampaignSeconds; // in 3 seconds
			}*/
			Sms->SetCoolState(SMSClass::COOLING);
	    }
		if (Sms->GetCoolState() == SMSClass::COOL)
		{
			//Sms->aim9warmtime = SimLibElapsedTime + 60 * CampaignSeconds; // in 60 seconds
			Sms->SetCoolState(SMSClass::WARMING);
		}
	}
	
	break;
	
    case 8:
	if (Sms->curWeaponType == wtAim120) {
	    // code for AIM120 ALBIT test
	}
	break;
	

    case 16:
	if (Sms->curWeaponType == wtAim120) {
	    Sms->NextAimId(); // JPO bump the AIM 120 id.
	}
	break;
	
    case 17:
	if (Sms->curWeaponType == wtAim120) {
	    // code for AIM120 Telemetry toggle
	}
	if (Sms->curWeaponType == wtAim9)
	{
		// Marco Edit - Auto Uncage TD/BP Mode
	    if (g_bRealisticAvionics && Sms->curWeapon && Sms->curWeaponType == wtAim9) 
		{
			ShiAssert(Sms->curWeapon->IsMissile());
			//MI fixup... only toggle for pM
			if(((CampBaseClass*)Sms->curWeapon)->GetSPType() != SPTYPE_AIM9P)
				((MissileClass*)Sms->curWeapon)->isTD = !((MissileClass*)Sms->curWeapon)->isTD;
	    }
	}
	break;
    case 18:
		//MI fixup... only for the M
	if (g_bRealisticAvionics && Sms->curWeapon && ((CampBaseClass*)Sms->curWeapon)->GetSPType() != SPTYPE_AIM9P) {
	    ShiAssert(Sms->curWeapon->IsMissile());
	    ((MissileClass*)Sms->curWeapon)->isSlave = !((MissileClass*)Sms->curWeapon)->isSlave;
	}
	break;
   }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::HarmDisplay (void)
{
    HarmTargetingPod* harmPod = (HarmTargetingPod*)FindSensor(Sms->ownship, SensorClass::HTS);
    
    if (harmPod)
    {
	harmPod->SetIntensity(GetIntensity());
	harmPod->Display(display);
    }
    
    LabelButton (0,  "HTS");
    LabelButton (1,  "TBL1");
    LabelButton (6,  "PWR", "ON");
    BottomRow();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::BombDisplay (void)
{
	char tmpStr[12];
	FireControlComputer* pFCC = Sms->ownship->GetFCC();
	TopRow(0);
	BottomRow();
	if(IsSet(MENUMODE)) {
	    AGMenuDisplay();
	    return;
	}
	// Count number of current weapons
	sprintf (tmpStr, "%d%s", Sms->NumCurrentWpn(), Sms->hardPoint[Sms->curHardpoint]->GetWeaponData()->mnemonic);
	ShiAssert(strlen(tmpStr) < 12);
	LabelButton (5,  tmpStr, "");
	
	if(g_bRealisticAvionics)
		SetWeapParams();
	// OSS Button Labels
	LabelButton (4,  "CNTL");
	//MI
	if(!g_bRealisticAvionics)
		LabelButton (6,  "PROF 1");
	else
	{
		if(Sms->Prof1)
			LabelButton (6,  "PROF 1");
		else
			LabelButton (6,  "PROF 2");
	}
	if (Sms->curWeaponClass != wcRocketWpn)
	{
		if(!g_bRealisticAvionics)
		{
			if (Sms->pair)
				LabelButton (7,  "PAIR");
			else
				LabelButton (7,  "SGL");
		}
		else
		{
			if(Sms->pair)
				sprintf(tmpStr, "%d PAIR", Sms->rippleCount + 1);
			else
				sprintf(tmpStr, "%d SGL", Sms->rippleCount + 1);

			LabelButton(7, tmpStr);
		}
		//MI
		if(!g_bRealisticAvionics)
			sprintf (tmpStr, "%d FT", Sms->rippleInterval);
		else
			sprintf (tmpStr, "%dFT", Sms->rippleInterval);
		LabelButton (8,  tmpStr);
		if(!g_bRealisticAvionics)
		{
			sprintf (tmpStr, "RP %d", Sms->rippleCount + 1);
			LabelButton (9,  tmpStr);
		}
		else
		{
			sprintf(tmpStr, "%d", Sms->rippleCount + 1);
			LabelButton(9, "RP", tmpStr);
		}
	}

	if(g_bRealisticAvionics && !g_bMLU)
	{
		char tempstr[10];
		sprintf(tempstr, "AD %.2fSEC", Sms->armingdelay / 100);
		
		display->TextLeft(-0.3F, 0.2F, tempstr);

		if(pFCC && (pFCC->GetSubMode() == FireControlComputer::LADD ||
			pFCC->GetSubMode() == FireControlComputer::CCRP ||
			pFCC->GetSubMode() == FireControlComputer::DTOSS))
		{
			sprintf(tempstr, "REL ANG %d", Sms->angle);
			display->TextLeft(-0.3F, 0.0F, tempstr);
		}
		if (Sms->curHardpoint >= 0 &&
			Sms->hardPoint[Sms->curHardpoint]->GetWeaponData()->flags & SMSClass::HasBurstHeight)
		{ 
			sprintf (tempstr, "BA %.0fFT", Sms->burstHeight);
			display->TextLeft(-0.3F, 0.1F, tempstr);
		}
		if(pFCC && pFCC->GetSubMode() == FireControlComputer::LADD)
		{
			display->TextCenter(-0.3F, -0.1F, "PR 25000");
			display->TextCenter(-0.3F, -0.2F, "TOF 28.00");
			display->TextCenter(-0.3F, -0.3F, "MRA 1105");
		}
	}


	//OWLOOK we need a switch here for arming delay
	//if (g_bArmingDelay)	MI
	if(g_bRealisticAvionics && g_bMLU)
	{
		sprintf (tmpStr, "AD %.0f", Sms->armingdelay);//me123 
		LabelButton (15,  tmpStr);//me123 
	}
	
	//MI
	if(g_bRealisticAvionics)
	{
		if(Sms->Prof1)
		{
			if(Sms->Prof1NSTL == 0)
				LabelButton (17, "NSTL");
			else if(Sms->Prof1NSTL == 1)
				LabelButton (17, "NOSE");
			else
				LabelButton (17, "TAIL");
		}
		else
		{
			if(Sms->Prof2NSTL == 0)
				LabelButton (17, "NSTL");
			else if(Sms->Prof2NSTL == 1)
				LabelButton (17, "NOSE");
			else
				LabelButton (17, "TAIL");
		}
	}
	else
		LabelButton (17, "NSTL");
	//MI not here in real
	if(!g_bRealisticAvionics)
	{
		if (Sms->curHardpoint >= 0 &&
			Sms->hardPoint[Sms->curHardpoint]->GetWeaponData()->flags & SMSClass::HasBurstHeight)
		{
			sprintf (tmpStr, "BA %.0f", Sms->burstHeight);
			LabelButton (18,  tmpStr);
		}
	}
}

// JPO - daw the menu options
void SmsDrawable::AGMenuDisplay (void)
{
    FireControlComputer* pFCC = Sms->ownship->GetFCC();
    ShiAssert(FALSE==F4IsBadReadPtr(pFCC, sizeof *pFCC));
    LabelButton(19, "CCIP", NULL, pFCC->GetSubMode() == FireControlComputer::CCIP);
    LabelButton(18, "CCRP", NULL, pFCC->GetSubMode() == FireControlComputer::CCRP);
    LabelButton(17, "DTOS", NULL, pFCC->GetSubMode() == FireControlComputer::DTOSS);
    LabelButton(16, "LADD", NULL, pFCC->GetSubMode() == FireControlComputer::LADD);
    LabelButton(15, "MAN", NULL, pFCC->GetSubMode() == FireControlComputer::MAN);
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::CameraDisplay (void)
{
	char tmpStr[12];
	TopRow(0);
	if (!Sms->CurStationOK())
	{
		LabelButton (1, "MAL");
	}
	else if (Sms->ownship->OnGround())
	{
		LabelButton (1, "RDY");
	}
	else
	{
		LabelButton (1, "RUN");
	}
	
	BottomRow ();
	
	sprintf (tmpStr, "IDX %d", frameCount);
	LabelButton (19, tmpStr);
	
	LabelButton (5, "RPOD");
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::SelectiveJettison(void)
{
	LabelButton (1, "S-J", NULL, 1);
	
	BottomRow ();
	
	InventoryDisplay (TRUE);
}

void SmsDrawable::TopRow(int isinv)
{
    FireControlComputer *FCC = Sms->ownship->GetFCC();
    
    if (isinv)  {
	if (!FCC->IsNavMasterMode())
	    LabelButton(3, "INV", NULL, 1);
    }
    else
	LabelButton(3, "INV");
    
    switch (FCC->GetMasterMode()) {
    case FireControlComputer::ILS:
    case FireControlComputer::Nav:
	LabelButton(0, "STBY");
	break;
    case FireControlComputer::MissileOverride:
	LabelButton(0, "MSL");
	//MI
	if(g_bRealisticAvionics && Sms->curWeaponType == wtGuns)
	{ 
		switch(FCC->GetSubMode()) 
		{
		case FireControlComputer::EEGS:
			LabelButton(1, "EEGS");
			break;
		case FireControlComputer::LCOS:
			LabelButton(1, "LCOS");
			break;
		case FireControlComputer::Snapshot:
			LabelButton(1, "SNAP");
			break;
		}
	}
	break;
    case FireControlComputer::Dogfight:
        LabelButton(0, "DGFT");
		if(!g_bRealisticAvionics)
		{
			switch(FCC->GetSubMode()) 
			{
			case FireControlComputer::EEGS:
				LabelButton(1, "EEGS");
				break;
			case FireControlComputer::LCOS:
				LabelButton(1, "LCOS");
				break;
			case FireControlComputer::Snapshot:
				LabelButton(1, "SNAP");
				break;
			}
		}
		else
		{
			switch(FCC->GetDgftGunSubMode())
			{
			case FireControlComputer::EEGS:
				LabelButton(1, "EEGS");
				break;
			case FireControlComputer::LCOS:
				LabelButton(1, "LCOS");
				break;
			case FireControlComputer::Snapshot:
				LabelButton(1, "SNAP");
				break;
			}
		}
		break;
    case FireControlComputer::Gun:
        LabelButton(0, "GUN");
		
	LabelButton(1, FCC->subModeString);
	break;
    default:
        if (FCC->IsAGMasterMode()) 
		{
			LabelButton(0, "A-G");
			//MI temporary till we get LGB's going
			if(g_bRealisticAvionics)
			{
				if(FCC->GetMasterMode() == FireControlComputer::AirGroundLaser || Sms && Sms->curWeaponDomain != wdGround)
					LabelButton(1, "");
				else
					LabelButton(1, FCC->subModeString);
			}
			else
				LabelButton(1, FCC->subModeString);
		} 
		else
	    LabelButton(0, "AAM");
    break;
    }
}

void SmsDrawable::BottomRow(void)
{
	//MI
	float cX, cY = 0;
	RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);
	if(!theRadar)
	{
		ShiWarning("Oh Oh shouldn't be here without a radar!");
		return;
	}
	else
		theRadar->GetCursorPosition (&cX, &cY);
    char *mode = "";
    switch (Sms->MasterArm())
 	{
 	case SMSBaseClass::Safe:
 		//MI not here in real
 		if(!g_bRealisticAvionics)
 			mode = "SAF";
 		else
 			mode = "";
 	break;
 	case SMSBaseClass::Sim:
 	    mode = "SIM";
 	break;
 	case SMSBaseClass::Arm:
		mode = "RDY";
	break;
 	}

    if (g_bRealisticAvionics) 
	{
		float x, y;
		GetButtonPos (12, &x, &y);
		// JPO - do this ourselves, so we can pass the rest off to the superclass.
		display->TextCenter(x, y + display->TextHeight(), mode);
		MfdDrawable::BottomRow();
		//MI changed
		if(g_bRealisticAvionics)
		{
			if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp &&
				OTWDriver.pCockpitManager->mpIcp->ShowBullseyeInfo)
			{
				DrawBullseyeCircle(display, cX, cY);
			}
			else
				MfdDrawable::DrawReference((AircraftClass*)Sms->Ownship());
		}
		else
			MfdDrawable::DrawReference((AircraftClass*)Sms->Ownship());
    }
    else 
    {
		LabelButton (10, "S-J");
		LabelButton (12, "FCR", mode);
		LabelButton (13, "MENU", NULL, 1);//me123
		LabelButton (14, "SWAP");
    }
}
//MI
void SmsDrawable::EmergJetDisplay(void)
{
	int curStation = 0;
	LabelButton(0, "E-J", NULL);
	LabelButton(4, "CLR", NULL);
	InventoryDisplay(2);
	for(curStation = Sms->numHardpoints-1; curStation>0; curStation--)
	{
// OW Jettison fix
#if 0
		if( !(((AircraftClass *)Sms->ownship)->IsF16() &&
         (curStation == 1 || curStation == 9 || hardPoint[curStation]->GetWeaponClass() == wcECM)) &&
         hardPoint[curStation]->GetRack())
#else
		if( !(((AircraftClass *)Sms->ownship)->IsF16() &&
         (curStation == 1 || curStation == 9 || Sms->hardPoint[curStation]->GetWeaponClass() == wcECM || Sms->hardPoint[curStation]->GetWeaponClass() == wcAimWpn)) &&
         (Sms->hardPoint[curStation]->GetRack() || curStation == 5 && Sms->hardPoint[curStation]->GetWeaponClass() == wcTank))//me123 in the line above addet a check so we don't emergency jettison a-a missiles
#endif
		{
			hardPointSelected |= (1 << curStation);
		}
	}
	BottomRow();
}
//MI
void SmsDrawable::ChangeToInput(int button)
{
	ClearDigits();
	lastInputMode = displayMode;
	SetDisplayMode(InputMode);
	InputLine = 0;
	MaxInputLines = 1;
	InputsMade = 0;
	switch(button)
	{
	case 4:
		PossibleInputs = 4;
		InputModus = CONTROL_PAGE;
		if(Sms->curHardpoint >= 0 &&
			Sms->hardPoint[Sms->curHardpoint]->GetWeaponData()->flags & SMSClass::HasBurstHeight)
		{
			C1Weap = FALSE;
			C2Weap = TRUE;
			C3Weap = FALSE;
			C4Weap = FALSE;
		}
		else
		{
			C1Weap = TRUE;
			C2Weap = FALSE;
			C3Weap = FALSE;
			C4Weap = FALSE;
		}
	break;
	case 8:
		PossibleInputs = 3;
		InputModus = RELEASE_SPACE;
	break;
	case 9:
		PossibleInputs = 2;
		InputModus = RELEASE_PULSE;
	break;
	case 15:
		PossibleInputs = 4;
		InputModus = ARMING_DELAY;
	break;
	case 18:
		PossibleInputs = 4;
		InputModus = BURST_ALT;
	break;
	default:
	break;
	}
}
void SmsDrawable::ChangeProf(void)
{
	RadarDopplerClass* pradar = (RadarDopplerClass*) FindSensor (SimDriver.playerEntity, SensorClass::Radar);
	if(pradar)	//MI fix
	{
		pradar->SetScanDir(1.0F);
		pradar->SelectLastAGMode();
	}
	//change profiles
	Sms->Prof1 = !Sms->Prof1;
	FireControlComputer *pFCC = Sms->ownship->GetFCC();
	if(Sms->Prof1)
	{
		Sms->rippleCount = Sms->Prof1RP;
		Sms->rippleInterval = Sms->Prof1RS;
		Sms->SetPair(Sms->Prof1Pair);
		if(pFCC)
		{
			if(pFCC->GetMasterMode() == FireControlComputer::AirGroundLaser)
				pFCC->SetSubMode(FireControlComputer::SLAVE);
			else
			{
				pFCC->SetSubMode(Sms->Prof1SubMode);
				pFCC->lastAgSubMode = Sms->Prof1SubMode;
			}
		}
	}
	else
	{
		Sms->rippleCount = Sms->Prof2RP;
		Sms->rippleInterval = Sms->Prof2RS;
		Sms->SetPair(Sms->Prof2Pair);
		if(pFCC)
		{
			if(pFCC->GetMasterMode() == FireControlComputer::AirGroundLaser)
				pFCC->SetSubMode(FireControlComputer::SLAVE);
			else
			{
				pFCC->SetSubMode(Sms->Prof2SubMode);
				pFCC->lastAgSubMode = Sms->Prof2SubMode;
			}
		}
	}
}
void SmsDrawable::SetWeapParams(void)
{
	if(Sms->curHardpoint >= 0)
	{
		if(Sms->hardPoint[Sms->curHardpoint]->GetWeaponData()->flags & SMSClass::HasBurstHeight)
		{
			Sms->armingdelay = Sms->C2AD;
			Sms->burstHeight = Sms->C2BA;
		}
		else
		{
			if(Sms->Prof1)
			{ 
				Sms->rippleCount = Sms->Prof1RP;
				Sms->rippleInterval = Sms->Prof1RS;
				Sms->SetPair(Sms->Prof1Pair);
				if(Sms->Prof1NSTL == 0 || Sms->Prof1NSTL == 2)
					Sms->armingdelay = Sms->C1AD2;
				else
					Sms->armingdelay = Sms->C1AD1;
			}
			else
			{
				Sms->rippleCount = Sms->Prof2RP;
				Sms->rippleInterval = Sms->Prof2RS;
				Sms->SetPair(Sms->Prof2Pair);
				if(Sms->Prof2NSTL == 0 || Sms->Prof2NSTL == 2)
					Sms->armingdelay = Sms->C1AD2;
				else
					Sms->armingdelay = Sms->C1AD1;
			} 
		}
	}
}
void SmsDrawable::MavSMSDisplay(void)
{
	FireControlComputer *pFCC = Sms->ownship->GetFCC();
	if(!pFCC)
		return;
	char tempstr[10] = "";

	LabelButton(0, "A-G");
	if(Sms->MavSubMode == SMSBaseClass::PRE)
		sprintf(tempstr,"PRE");
	else if(Sms->MavSubMode == SMSBaseClass::VIS)
		sprintf(tempstr, "VIS");
	else
		sprintf(tempstr,"BORE");
	LabelButton(1,tempstr);
	LabelButton(3, "INV");

	if(Sms->Powered)
		LabelButton(6, "PWR", "ON");
	else
		LabelButton(6, "PWR", "OFF");
	LabelButton(7, "RP", "1");
	LabelButton(18, "STEP");
	//not here in the real deal
	//LabelButton(19, pFCC->subModeString);
	BottomRow();
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

GroundSpotEntity::GroundSpotEntity (int type) : FalconEntity(type)
{
    SetYPRDelta(0,0, 0);
}

GroundSpotEntity::GroundSpotEntity (VU_BYTE ** data) : FalconEntity(data)
{
    SetYPRDelta(0,0, 0); // JPO - set to something.
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

GroundSpotDriver::GroundSpotDriver (VuEntity *entity) : VuMaster(entity)
{
	last_time = 0;
	lx = ly = lz = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_BOOL GroundSpotDriver::ExecModel(VU_TIME)
{
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void GroundSpotDriver::Exec (VU_TIME time)
{
	float
		dx,
		dy,
		dz;

	if (time - last_time > 2000)
	{
		last_time = time;

		dx = vuabs (lx - entity_->XPos ());
		dy = vuabs (ly - entity_->YPos ());
		dz = vuabs (lz - entity_->ZPos ());

		if (dx + dy + dz < 4000)
		{
			if (dx + dy + dz < 1.0)
			{
				entity_->SetDelta (0.0, 0.0, 0.0);
				
//				MonoPrint ("GroundSpot %f,%f,%f Stationary\n",
//					entity_->XPos (),
//					entity_->YPos (),
//					entity_->ZPos ());
			}
			else
			{
				entity_->SetDelta (0.001F, 0.001F, 0.001F);
				
//				MonoPrint ("GroundSpot %f,%f,%f Moving\n",
//					entity_->XPos (),
//					entity_->YPos (),
//					entity_->ZPos ());
			}
			
			VuMaster::Exec (time);
		}
		else
		{
//			MonoPrint ("GroundSpot Moving Fast %f,%f,%f %f\n", dx, dy, dz, dx + dy + dz);
		}

		lx = entity_->XPos ();
		ly = entity_->YPos ();
		lz = entity_->ZPos ();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


