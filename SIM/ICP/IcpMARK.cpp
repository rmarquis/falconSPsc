#include "stdlib.h"
#include "stdhdr.h"
#include "vu2.h"
#include "entity.h"
#include "fcc.h"
#include "aircrft.h"
#include "simdrive.h"
#include "icp.h"
#include "navsystem.h"
#include "mfd.h"
#include "radar.h"
#include "otwdrive.h"

//MI for ICP stuff
extern bool g_bRealisticAvionics;

void ICPClass::ExecMARKMode(void) 
{

	NavigationSystem::Point_Type	pointType;

	if(!g_bRealisticAvionics)
	{
		if(mUpdateFlags & MARK_UPDATE) 
		{

			mUpdateFlags &= !MARK_UPDATE;
			
			gNavigationSys->GetMarkPoint(&pointType, mpLine2, mpLine3);

			if(pointType == NavigationSystem::NODATA) 
			{
				sprintf(mpLine1, "MARK %2d", gNavigationSys->GetMarkIndex() + 1);
				sprintf(mpLine2, "NO MARK DATA");
				sprintf(mpLine3, "ENTR TO SET");
			}
			else 
			{
				sprintf(mpLine1, "MARK %2d      %s", gNavigationSys->GetMarkIndex() + 1, mpPointTypeNames[pointType]);
				gNavigationSys->GetMarkPoint(&pointType, mpLine2, mpLine3);
			}
		}
	}
	else
	{
		ClearStrings();

		gNavigationSys->GetMarkPoint(&pointType, latStr, longStr);
	
		if(pointType == NavigationSystem::NODATA) 
		{
			sprintf(tempstr, "MARK %2d", gNavigationSys->GetMarkIndex() + 1);
			FillDEDMatrix(0,12,tempstr);
			FillDEDMatrix(2,7, "NO MARK DATA");
			FillDEDMatrix(3,7, "ENTR TO SET");
		}
		else 
		{
			gNavigationSys->GetMarkPoint(&pointType, latStr, longStr);
			sprintf(tempstr, "MARK %2d  %s", gNavigationSys->GetMarkIndex() + 1, mpPointTypeNames[pointType]);
			//gNavigationSys->GetMarkPoint(&pointType, latStr, longStr);
			//Line1
			FillDEDMatrix(0,10,tempstr);
			//Line3
			FillDEDMatrix(2,2,latStr);
			//Line4
			FillDEDMatrix(3,2,longStr);
		}
	}
}


void ICPClass::ENTRUpdateMARKMode(void) {

	float		xprime, yprime;
	float		x, y, z;
	mlTrig	trig;

	if(SimDriver.playerEntity) {

		if(MfdDisplay[0]->mode == MFDClass::FCRMode && MfdDisplay[0]->drawable && ((RadarClass*)MfdDisplay[0]->drawable)->IsAG()) {

			((RadarClass*)MfdDisplay[0]->drawable)->GetCursorPosition (&xprime, &yprime);

			xprime *= NM_TO_FT;
			yprime *= NM_TO_FT;
			mlSinCos(&trig, ((AircraftClass*)(SimDriver.playerEntity))->Yaw());
			x = xprime * trig.cos - yprime * trig.sin + ((AircraftClass*)(SimDriver.playerEntity))->XPos();
			y = xprime * trig.sin + yprime * trig.cos + ((AircraftClass*)(SimDriver.playerEntity))->YPos();
			z = OTWDriver.GetGroundLevel(x, y);
			gNavigationSys->SetMarkPoint(NavigationSystem::GMPOINT, 
													x,
													y,
													z,
													SimLibElapsedTime);
		}
		else if(MfdDisplay[1]->mode == MFDClass::FCRMode && MfdDisplay[1]->drawable && ((RadarClass*)MfdDisplay[1]->drawable)->IsAG()) {

			((RadarClass*)MfdDisplay[0]->drawable)->GetCursorPosition (&xprime, &yprime);

			xprime *= NM_TO_FT;
			yprime *= NM_TO_FT;
			mlSinCos(&trig, ((AircraftClass*)(SimDriver.playerEntity))->Yaw());
			x = xprime * trig.cos - yprime * trig.sin + ((AircraftClass*)(SimDriver.playerEntity))->XPos();
			y = xprime * trig.sin + yprime * trig.cos + ((AircraftClass*)(SimDriver.playerEntity))->YPos();
			z = OTWDriver.GetGroundLevel(x, y);
			gNavigationSys->SetMarkPoint(NavigationSystem::GMPOINT, 
													x,
													y,
													z,
													SimLibElapsedTime);
		}
		else {
			gNavigationSys->SetMarkPoint(NavigationSystem::POS,
													((AircraftClass*)(SimDriver.playerEntity))->XPos(),
													((AircraftClass*)(SimDriver.playerEntity))->YPos(),
													((AircraftClass*)(SimDriver.playerEntity))->ZPos(),
													SimLibElapsedTime);

		}
		mUpdateFlags |= MARK_UPDATE;
	}
}

void ICPClass::PNUpdateMARKMode(int button, int) {

	if(button == PREV_BUTTON) {
      SimDriver.playerEntity->FCC->waypointStepCmd = -1;
   }
	else if(button == NEXT_BUTTON) {
      SimDriver.playerEntity->FCC->waypointStepCmd = 1;
   }

	mUpdateFlags |= MARK_UPDATE;
	mUpdateFlags |= CNI_UPDATE;
}
