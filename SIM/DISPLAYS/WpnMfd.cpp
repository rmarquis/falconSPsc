#include "stdhdr.h"
#include "mfd.h"
#include "hud.h"
#include "stdhdr.h"
#include "mfd.h"
#include "hud.h"
#include "sms.h"
#include "simdrive.h"
#include "camp2sim.h"
#include "Graphics\Include\render2d.h"
#include "Graphics\Include\canvas3d.h"
#include "Graphics\Include\tviewpnt.h"
#include "Graphics\Include\renderir.h"
#include "otwdrive.h"	
#include "cpmanager.h"
#include "icp.h"	
#include "aircrft.h"
#include "fcc.h"	
#include "radardoppler.h"
#include "mavdisp.h"
#include "misldisp.h"
#include "airframe.h"
#include "simweapn.h"
#include "missile.h"

//MI
void DrawBullseyeCircle(VirtualDisplay* display, float cursorX, float cursorY);

WpnMfdDrawable::WpnMfdDrawable()
{
	self = NULL;
	pFCC = NULL;
	mavDisplay = NULL;
	Sms = NULL;
	theRadar = NULL;
}
void WpnMfdDrawable::DisplayInit (ImageBuffer* image)
{
    DisplayExit();
    
    privateDisplay = new RenderIR;
    ((RenderIR*)privateDisplay)->Setup (image, OTWDriver.GetViewpoint());
    
    privateDisplay->SetColor (0xff00ff00);
   	((Render3D*)privateDisplay)->SetFOV(6.0f*DTR);
}
VirtualDisplay* WpnMfdDrawable::GetDisplay(void)
{
	if(!SimDriver.playerEntity || !SimDriver.playerEntity->Sms || 
		!SimDriver.playerEntity->Sms->curWeapon)
		return privateDisplay;

	Sms = SimDriver.playerEntity->Sms;

	VirtualDisplay* retval= privateDisplay;
	MissileClass* theMissile = (MissileClass*)(Sms->curWeapon);
	float rx, ry, rz;
	Tpoint pos;
	
	if(Sms->CurHardpoint() < 0)
	{
		return retval;
	}
	
	if(Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponClass() == wcAgmWpn &&
		Sms->curWeaponType == wtAgm65)
	{
		if (theMissile && theMissile->IsMissile())
		{
			mavDisplay = (MaverickDisplayClass*)theMissile->display;
			if (mavDisplay)
			{
				if (!mavDisplay->GetDisplay())
				{
					if (privateDisplay)
					{
						mavDisplay->DisplayInit(((Render2D*)(privateDisplay))->GetImageBuffer());
					}
					mavDisplay->viewPoint = viewPoint;
					
					// Set missile initial position
					Sms->hardPoint[Sms->CurHardpoint()]->GetSubPosition(Sms->curWpnNum, &rx, &ry, &rz);
					rx += 5.0F;
					pos.x = Sms->Ownship()->XPos() + Sms->Ownship()->dmx[0][0]*rx + Sms->Ownship()->dmx[1][0]*ry +
						Sms->Ownship()->dmx[2][0]*rz;
					pos.y = Sms->Ownship()->YPos() + Sms->Ownship()->dmx[0][1]*rx + Sms->Ownship()->dmx[1][1]*ry +
						Sms->Ownship()->dmx[2][1]*rz;
					pos.z = Sms->Ownship()->ZPos() + Sms->Ownship()->dmx[0][2]*rx + Sms->Ownship()->dmx[1][2]*ry +
						Sms->Ownship()->dmx[2][2]*rz;
					mavDisplay->SetXYZ (pos.x, pos.y, pos.z);
				}
				retval = mavDisplay->GetDisplay();
			}
		}
	}
	return (retval);
}
void WpnMfdDrawable::Display (VirtualDisplay* newDisplay)
{
    display = newDisplay;


	
	float cX, cY;
	self = ((AircraftClass*)SimDriver.playerEntity);
	theRadar = (RadarDopplerClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);
	pFCC = SimDriver.playerEntity->Sms->Ownship()->GetFCC();
	Sms = SimDriver.playerEntity->Sms;
	mavDisplay = NULL;
	display = newDisplay;
	if(!theRadar || !pFCC || !self || !Sms)
	{
		ShiWarning("Oh Oh shouldn't be here without a radar or FCC or player or SMS!");
		return;
	}
		if(!g_bRealisticAvionics || !Sms->curWeapon || Sms->curWeaponType != wtAgm65)
	{
		OffMode(display);
		return;
	}
	if(Sms->curWeapon)
	{
	    ShiAssert(Sms->curWeapon->IsMissile());
		mavDisplay = (MaverickDisplayClass*)((MissileClass*)Sms->curWeapon)->display;

		if(mavDisplay && !((MissileClass*)Sms->curWeapon)->Covered && Sms->MavCoolTimer < 0.0F)
		{
			mavDisplay->SetIntensity(GetIntensity());
			mavDisplay->viewPoint = viewPoint;
			mavDisplay->Display(display);
			//DLZ
			DrawDLZ(display);
		}
		else if(Sms->MavCoolTimer > 0.0F && Sms->Powered)
		{
			display->TextCenter(0.0F, 0.7F, "NOT TIMED OUT");
		}
		DrawRALT(display);
	}
	
	//SMSMissiles
	DrawHDPT(display, Sms);

	//OSB's
	OSBLabels(display);

	//Reference symbol
	theRadar->GetCursorPosition (&cX, &cY);
	if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp &&
		OTWDriver.pCockpitManager->mpIcp->ShowBullseyeInfo)
		DrawBullseyeCircle(display, cX, cY);
	else
		DrawReference(MfdDisplay[OnMFD()]->GetOwnShip());
}
void WpnMfdDrawable::DrawDLZ(VirtualDisplay* display)
{
	if(!pFCC)
		return;

	float yOffset;
	float percentRange;
	float rMax, rMin;
	float range;
	float dx, dy, dz;

	if (pFCC->missileTarget)
	{
		// Range Carat / Closure
		rMax   = pFCC->missileRMax;
		rMin   = pFCC->missileRMin;
		
		// get range to ground designaate point
		dx = Sms->Ownship()->XPos() - pFCC->groundDesignateX;
		dy = Sms->Ownship()->YPos() - pFCC->groundDesignateY;
		dz = Sms->Ownship()->ZPos() - pFCC->groundDesignateZ;
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
void WpnMfdDrawable::PushButton(int whichButton, int whichMFD)
{
    FireControlComputer* pFCC = SimDriver.playerEntity->Sms->Ownship()->GetFCC();
    switch (whichButton)
    {
	case 0:
		if(!Sms->Powered)
			Sms->Powered = TRUE;
	break;
	case 1:
		Sms->StepMavSubMode();
	break;
    case 2:
		if(mavDisplay)
			mavDisplay->ToggleFOV();
	break;
	case 3:
	break;
	case 4:
		((MissileClass*)Sms->curWeapon)->HOC = !((MissileClass*)Sms->curWeapon)->HOC;
	break;
	case 10:
    case 11:	
    case 12:	
    case 13:
	case 14:
		MfdDrawable::PushButton(whichButton, whichMFD);
	break;
	case 19:
		//if(pFCC)
		//	pFCC->NextSubMode();
	break;
	default:
	break;
	}
}
void WpnMfdDrawable::OSBLabels(VirtualDisplay* display)
{
	char tempstr[10] = "";
	if(!Sms->Powered)
		LabelButton (0, "STBY");
	else
		LabelButton (0, "OPER");

	if(Sms->MavSubMode == SMSBaseClass::PRE)
		sprintf(tempstr,"PRE");
	else if(Sms->MavSubMode == SMSBaseClass::VIS)
		sprintf(tempstr, "VIS");
	else
		sprintf(tempstr,"BORE");
	LabelButton(1, tempstr);
		
	if (mavDisplay && mavDisplay->CurFOV() > (3.5F * DTR))
		LabelButton (2, "FOV");
	else
		LabelButton (2, "EXP", NULL, 1);
	if(Sms->curWeapon)
	{
		if(((MissileClass*)Sms->curWeapon)->HOC)
			LabelButton (4, "HOC");
		else
			LabelButton (4, "COH");
	}
	//Doc says this isn't there anymore in the real deal
	//LabelButton (19, pFCC->subModeString);
	char tmpStr[12];
    float width = display->TextWidth("M ");
    if (Sms->CurHardpoint() < 0)
		return;
    sprintf (tmpStr, "%d%s", Sms->NumCurrentWpn(), Sms->hardPoint[Sms->CurHardpoint()]->GetWeaponData()->mnemonic);
    ShiAssert(strlen(tmpStr) < sizeof (tmpStr));
    LabelButton(5, tmpStr);

	char *mode = "";
	if(Sms->Powered && Sms->MavCoolTimer <= 0.0F)
	{
		switch (Sms->MasterArm())
		{
		case SMSBaseClass::Safe:
			mode = "";
			break;
		case SMSBaseClass::Sim:
			mode = "SIM";
			break;
		case SMSBaseClass::Arm:
			mode = "RDY";
			break;
		}
 	}

    float x, y;
	GetButtonPos (12, &x, &y);
	display->TextCenter(x, y + display->TextHeight(), mode);
	BottomRow();
}
void WpnMfdDrawable::DrawRALT(VirtualDisplay* display)
{
	if(TheHud && !(self->mFaults && self->mFaults->GetFault(FaultClass::ralt_fault))
		&& self->af->platform->RaltReady() &&
		TheHud->FindRollAngle(-TheHud->hat) && TheHud->FindPitchAngle(-TheHud->hat))
	{
		float x,y = 0;
		GetButtonPos(5, &x, &y);
		y += display->TextHeight();
		x -= 0.05F;
		int RALT = (int)-TheHud->hat;
		char tempstr[10] = "";
		if(RALT > 9990)
		RALT = 9990;
		sprintf(tempstr, "%d", RALT);
		display->TextRight(x, y, tempstr);
	}
}
void WpnMfdDrawable::DrawHDPT(VirtualDisplay* display, SMSClass* Sms)
{
	float leftEdge;
    float width = display->TextWidth("M ");
	float x = 0, y = 0;
	GetButtonPos(8, &x, &y); // use button 8 as a reference (lower rhs)
	y -= display->TextHeight()*2;
	for(int i=0; i<Sms->NumHardpoints(); i++)
    {
		char c = HdptStationSym(i, Sms);
		if (c == ' ') continue; // Don't bother drawing blanks.
		Str[0] = c;
		Str[1] = '\0';
		if(i < 6)
		{
			leftEdge = -x + width * (i-1);
			display->TextLeft(leftEdge, y, Str, (Sms->CurHardpoint() == i ? 2 : 0));
		}
		else
		{
			leftEdge = x - width * (Sms->NumHardpoints() - i - 1);
			// Box the current station
			display->TextRight(leftEdge, y, Str, (Sms->CurHardpoint() == i ? 2 : 0));
		}
    }
}
char WpnMfdDrawable::HdptStationSym(int n, SMSClass* Sms) // JPO new routine
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
void WpnMfdDrawable::OffMode(VirtualDisplay* display)
{
	display->TextCenterVertical (0.0f, 0.2f, "Wpn");
	int ofont = display->CurFont();
	display->SetFont(2);
	display->TextCenterVertical (0.0f, 0.0f, "Off");
	display->SetFont(ofont);
	DrawReference (MfdDisplay[OnMFD()]->GetOwnShip());
	BottomRow();
}