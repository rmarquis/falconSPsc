#include "icp.h"
#include "simdrive.h"
#include "aircrft.h"
#include "navsystem.h"
#include "flightdata.h"
#include "Phyconst.h"
#include "fcc.h"
#include "hud.h"
#include "cpmanager.h"
#include "commands.h"

void ICPClass::ICPEnter(void)
{
	if(IsICPSet(ICPClass::MODE_CNI) && mICPSecondaryMode == ONE_BUTTON && Manual_Input)
		EnterTCN();
	else if(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == TWO_BUTTON && Manual_Input)
		EnterBingo();
	else if(IsICPSet(ICPClass::MODE_CNI) && mICPSecondaryMode == TWO_BUTTON && Manual_Input)
		EnterALOW();
	else if(IsICPSet(ICPClass::EDIT_LAT) && Manual_Input)
		EnterLat();					
	else if(IsICPSet(ICPClass::EDIT_LONG) && Manual_Input)
		EnterLong();
	else if(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == NONE_MODE)
	{
		ClearStrings();
		ClearFlags();
		SetICPFlag(ICPClass::MODE_DLINK);
		SimDriver.playerEntity->FCC->SetStptMode(FireControlComputer::FCCDLinkpoint);
		SimDriver.playerEntity->FCC->waypointStepCmd = 127;
		ExecDLINKMode();
	}
	else if(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == FIFE_BUTTON && Manual_Input)
		EnterWSpan();
	//VIP/VRP
	else if(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == THREE_BUTTON && Manual_Input)
		EnterVIP();
	else if(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == NINE_BUTTON && Manual_Input)
		EnterVRP();
	//EWS
	else if(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == EWS_MODE)
	{
		if(EWSMain && Manual_Input)
			EWSEnter();
		else if(Manual_Input)
		{
			if(BQ || BI)
				EnterBurst();
			else if(SQ || SI)
				EnterSalvo();
		}
	}
	else if((OA1 || OA2) && Manual_Input)
		EnterOA();

	//Mark
	else if(mICPSecondaryMode == SEVEN_BUTTON)
		ENTRUpdateMARKMode();

	//INTG
	else if((IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == 100) ||
		IsICPSet(ICPClass::MODE_IFF) && Manual_Input)
		EnterINTG();

	//INS
	else if(IsICPSet(ICPClass::MODE_LIST) && mICPSecondaryMode == SIX_BUTTON && Manual_Input)
		EnterINSStuff();

	//Laser
	else if(IsICPSet(ICPClass::MISC_MODE) && mICPSecondaryMode == FIFE_BUTTON && Manual_Input)
		EnterLaser();

	InputsMade = 0;
}
void ICPClass::EnterLat(void)
{
	if(!SimDriver.playerEntity)
		return;
	CheckDigits();
	//We just edited our latitude
	LATDegrees = ((Input_Digit1*100) + (Input_Digit2*10) + Input_Digit3);
	LATMinutes = (((Input_Digit4*10 + Input_Digit5) * 1.66666666F) / 100);
	LATSeconds = (((Input_Digit6*10 + Input_Digit7) * 0.02777777F) / 100);
	Lat = LATDegrees + LATMinutes + LATSeconds;	
	if(Lat > 90)
	{
		WrongInput();
		return;
	}
	Lat *= DTR;
	SetLat = (Lat * EARTH_RADIUS_FT) - (FALCON_ORIGIN_LAT * FT_PER_DEGREE);
	//Get this waypoints coords 
	SimDriver.playerEntity->curWaypoint->GetLocation(&xCurr, &yCurr, &zCurr);
	WPAlt = zCurr;
	SetLong = yCurr;
	//set our new waypoint
	SimDriver.playerEntity->curWaypoint->SetLocation(SetLat, SetLong, WPAlt);
	//confirm our change
	ResetInput();
}
void ICPClass::EnterLong(void)
{
	if(!SimDriver.playerEntity)
		return;
	CheckDigits();
	//find cosLatitude --> Get this waypoints coords
	SimDriver.playerEntity->curWaypoint->GetLocation(&xCurr, &yCurr, &zCurr);
	latitude	= (FALCON_ORIGIN_LAT * FT_PER_DEGREE + xCurr) / EARTH_RADIUS_FT;
	cosLat = (float)cos(latitude);

	//Get our entered Longitude
	LONGDegrees = ((Input_Digit1*100) + (Input_Digit2*10) + Input_Digit3);
	LONGMinutes = (((Input_Digit4*10 + Input_Digit5) * 1.66666666F) / 100);
	LONGSeconds = (((Input_Digit6*10 + Input_Digit7) * 0.02777777F) / 100);

	Long = LONGDegrees + LONGMinutes + LONGSeconds;
	if(Long > 180)
	{
		WrongInput();
		return;
	}
	Long *= DTR;

	SetLong = (Long * (EARTH_RADIUS_FT * cosLat)) - (FALCON_ORIGIN_LONG * DTR * EARTH_RADIUS_FT * cosLat);
	
	WPAlt = zCurr;
	SetLat = xCurr;
	//set our new waypoint
	SimDriver.playerEntity->curWaypoint->SetLocation(SetLat, SetLong, WPAlt);
	//confirm our change
	ResetInput();
}
void ICPClass::EnterALOW(void)
{
	if(!EDITMSLFLOOR && !TFADV)
	{
		//round to the next 10FT
		if(Input_Digit7 != 0)
		{
			if(Input_Digit6 > 10)
				Input_Digit6 = 0;
			Input_Digit6++;
			Input_Digit7 = 0;
			Input_Digit2 = 0;
			Input_Digit1 = 0;
		}
		ManualALOW = AddUp();
		if(ManualALOW < 50001)
		{
			TheHud->lowAltWarning = (float)ManualALOW;
			ResetInput();
		}
		else
			WrongInput();
	}
	else if(EDITMSLFLOOR)
	{
		TheHud->MSLFloor = AddUp();
		ResetInput();
	}
	else if(TFADV)
	{
		int ManualTF = AddUp();
		if(ManualTF < 80001)
		{
			TheHud->TFAdv = ManualTF;
			ResetInput();
		}
		else
			WrongInput();
	}
	else
		WrongInput();
}
void ICPClass::EnterTCN(void)
{	
	if(!gNavigationSys)
		return;
	if(Input_Digit7 == 0 && Input_Digit6 > 10)
	{
		gNavigationSys->StepTacanBand(NavigationSystem::ICP);
		//ClearDigits();
		ResetInput();
	}
	else
	{
		CurrChannel = AddUp();
		//Not correct
		if(CurrChannel > 126 || CurrChannel == 0)
			WrongInput();
		else
		{
			//Set our new channel
			if(gNavigationSys->GetTacanBand(NavigationSystem::ICP) == TacanList::X)
				gNavigationSys->SetTacanChannel(NavigationSystem::ICP, CurrChannel, TacanList::X);
			else
				gNavigationSys->SetTacanChannel(NavigationSystem::ICP, CurrChannel, TacanList::Y);
			//ClearDigits();
			ResetInput();
		}
	}
}
void ICPClass::EnterBingo(void)
{
	ManualBingo = AddUpLong();
	((AircraftClass*)(SimDriver.playerEntity))->SetBingoFuel((float)ManualBingo);
	ResetInput();	
}
void ICPClass::EnterBurst(void)
{
	if(PGMChaff)
	{
		if(BQ)
		{
			iCHAFF_BQ[CPI] = AddUp();
			ResetInput();
		}
		else if(BI)
		{
			fCHAFF_BI[CPI] = (AddUpFloat() / 1000);
			if(fCHAFF_BI[CPI] > 10)
				WrongInput();
			else
				ResetInput();
		}
	}
	else
	{
		if(BQ)
		{
			iFLARE_BQ[FPI] = AddUp();
			ResetInput();
		}
		else if(BI)
		{
			fFLARE_BI[FPI] = (AddUpFloat() / 1000);
			if(fFLARE_BI[FPI] > 10)
				WrongInput();
			else
				ResetInput();
		}
	}
}
void ICPClass::EnterSalvo(void)
{
	if(PGMChaff)
	{
		if(SQ)
		{
			iCHAFF_SQ[CPI] = AddUp();
			ResetInput();
		}
		else if(SI)
		{
			fCHAFF_SI[CPI] = (AddUpFloat() / 100);
			if(fCHAFF_SI[CPI] > 150)
				WrongInput();
			else
				ResetInput();
		}
	}
	else
	{
		if(SQ)
		{
			iFLARE_SQ[FPI] = AddUp();
			ResetInput();
		}
		else if(SI)
		{
			fFLARE_SI[FPI] = (AddUpFloat() / 100);
			if(fFLARE_SI[FPI] > 150)
				WrongInput();
			else
				ResetInput();
		}
	}
}
void ICPClass::EWSEnter(void)
{
	if(IsICPSet(ICPClass::CHAFF_BINGO))
	{
		tempvar = AddUp();
		if(tempvar > SimDriver.playerEntity->counterMeasureStation[CHAFF_STATION].weaponCount)
			WrongInput();
		else
		{
			ChaffBingo = tempvar;
			ResetInput();
		}
	}		
	else if(IsICPSet(ICPClass::FLARE_BINGO))
	{
		tempvar = AddUp();
		if(tempvar > SimDriver.playerEntity->counterMeasureStation[FLARE_STATION].weaponCount)
			WrongInput();
		else
		{
			FlareBingo = tempvar;
			ResetInput();
		}
	}
}
void ICPClass::EnterOA(void)
{
	if(OA1)
		tempvar1 = fOA_BRG;
	else
		tempvar1 = fOA_BRG2;
	if(OA_RNG)
	{
		if(OA1)
			iOA_RNG = AddUp();			
		else if(OA2)
			iOA_RNG2 = AddUp();
		ResetInput();
	}
	else if(OA_BRG)
	{
		if(OA1)
		{
			fOA_BRG = (AddUpFloat() / 10);
			if(fOA_BRG > 359.9)
				WrongInput();
			else
			{
				ResetInput();
			}
		}
		else if(OA2)
		{
			fOA_BRG2 = (AddUpFloat() / 10);
			if(fOA_BRG2 > 359.9)
				WrongInput();
			else
				ResetInput();
		}
	}
	else
	{
		if(OA1)
			iOA_ALT	= AddUp();
		else if(OA2)
			iOA_ALT2 = AddUp();
		ResetInput();
	}
	SetOA();
}
void ICPClass::EnterVIP(void)
{
	if(VIP_RNG)
	{
		iVIP_RNG = AddUp();			
		ResetInput();
	}
	else if(VIP_BRG)
	{
		fVIP_BRG = (AddUpFloat() / 10);
		if(fVIP_BRG > 359.9)
			WrongInput();
		else
			ResetInput();
	}
	else
	{
		iVIP_ALT	= AddUp();
		ResetInput();
	}
	SetVIP();
}
void ICPClass::EnterVRP(void)
{
	if(VRP_RNG)
	{
		iVRP_RNG = AddUp();			
		ResetInput();
	}
	else if(VRP_BRG)
	{
		fVRP_BRG = (AddUpFloat() / 10);
		if(fVRP_BRG > 359.9)
			WrongInput();
		else
			ResetInput();
	}
	else
	{
		iVRP_ALT	= AddUp();
		ResetInput();
	}
	SetVRP();
}
void ICPClass::EnterWSpan(void)
{
	ManualWSpan = AddUpFloat();
	if(ManualWSpan > 120 || ManualWSpan < 20)
		WrongInput();
	else
	{
		ManWSpan = ManualWSpan;
		ResetInput();
	}
}
void ICPClass::WrongInput(void)
{
	SetICPFlag(ICPClass::BLOCK_MODE);
	SetICPFlag(ICPClass::FLASH_FLAG);
}
void ICPClass::SetOA(void)
{
	if(!SimDriver.playerEntity->curWaypoint)
		return;
	//which offset do we want to set?
	if(OA1)
	{
		//if we don't have a different altitude or a range, we just take the current waypoint
		if(iOA_RNG == 0 && iOA_ALT == 0)
		{
			SimDriver.playerEntity->curWaypoint->GetLocation(&xCurr, &yCurr, &zCurr);
			gNavigationSys->SetDESTOAPoint(NavigationSystem::POS,xCurr,yCurr,iOA_ALT,1);
			return;
		}
		//First, let's get our current waypoint
		SimDriver.playerEntity->curWaypoint->GetLocation(&xCurr, &yCurr, &zCurr);
		//let's find the sides a and b of our triangle
		float a = 0.0F, b = 0.0F;
		float tempAng = fOA_BRG;
		GetValues(tempAng, &a, &b, iOA_RNG);
		b = fabs(b);
		a = fabs(a);
		if(fOA_BRG >= 0.0F && fOA_BRG <= 90.0F)
		{
			xCurr += b;
			yCurr += a;
		}
		else if(fOA_BRG > 90.0F && fOA_BRG <= 180.0F)
		{
			xCurr -= b;
			yCurr += a;
		}
		else if(fOA_BRG > 180.0F && fOA_BRG <= 270.0F)
		{
			xCurr -= b;
			yCurr -= a;
		}
		else
		{
			xCurr += b;
			yCurr -= a;
		}
		WPAlt = iOA_ALT;
		//set our new OffsetAimpoint
		gNavigationSys->SetDESTOAPoint(NavigationSystem::POS,xCurr,yCurr,WPAlt,1);		
	}
	else
	{
		//if we don't have a different altitude or a range, don't do anything
		if(iOA_RNG2 == 0 && iOA_ALT2 == 0)
		{
			SimDriver.playerEntity->curWaypoint->GetLocation(&xCurr, &yCurr, &zCurr);
			gNavigationSys->SetDESTOAPoint(NavigationSystem::POS,xCurr,yCurr,iOA_ALT2,2); //+200
			return;
		}
		//First, let's get our current waypoint
		SimDriver.playerEntity->curWaypoint->GetLocation(&xCurr, &yCurr, &zCurr);
		//let's find the sides a and b of our triangle
		float a = 0.0F, b = 0.0F;
		float tempAng = fOA_BRG2;
		GetValues(tempAng, &a, &b, iOA_RNG2);
		b = fabs(b);
		a = fabs(a);
		/*if(fOA_BRG2 >= 0.0F && fOA_BRG2 <= 90.0F)
		{
			xCurr += b;
			yCurr += a;
		}
		else if(fOA_BRG2 > 90.0F && fOA_BRG2 < 180.0F)
		{
			xCurr -= b;
			yCurr += a;
		}
		else if(fOA_BRG2 >= 180.0F && fOA_BRG2 < 270.0F)
		{
			xCurr -= a;
			yCurr -= b;
		}
		else
		{
			xCurr += b;
			yCurr -= a;
		}*/
		if(fOA_BRG2 >= 0.0F && fOA_BRG2 <= 90.0F)
		{
			xCurr += b;
			yCurr += a;
		}
		else if(fOA_BRG2 > 90.0F && fOA_BRG2 <= 180.0F)
		{
			xCurr -= b;
			yCurr += a;
		}
		else if(fOA_BRG2 > 180.0F && fOA_BRG2 <= 270.0F)
		{
			xCurr -= b;
			yCurr -= a;
		}
		else
		{
			xCurr += b;
			yCurr -= a;
		}
		WPAlt = iOA_ALT2;
		//set our new OffsetAimpoint
		gNavigationSys->SetDESTOAPoint(NavigationSystem::POS,xCurr,yCurr,WPAlt,2);
	}
}
void ICPClass::SetVIP(void)
{
	if(!SimDriver.playerEntity->curWaypoint)
		return;
	//if we don't have a different altitude or a range, we just take the current waypoint
	if(iVIP_RNG == 0 && iVIP_ALT == 0)
	{
		SimDriver.playerEntity->curWaypoint->GetLocation(&xCurr, &yCurr, &zCurr);
		gNavigationSys->SetVIPOAPoint(NavigationSystem::POS,xCurr,yCurr,iVIP_ALT, 1);// + 200,1);
		return;
	}
	//First, let's get our current waypoint
	SimDriver.playerEntity->curWaypoint->GetLocation(&xCurr, &yCurr, &zCurr);
	float wpX = xCurr;
	float wpY = yCurr;
	float wpZ = zCurr;
	//let's find the sides a and b of our triangle
	float a = 0.0F, b = 0.0F;
	float tempAng = fVIP_BRG;
	if(fVIP_BRG > 90.0F)
		tempAng = fVIP_BRG - 90;
	if(fVIP_BRG > 180.0F)
		tempAng = fVIP_BRG - 180;
	if(fVIP_BRG > 270.0F)
		tempAng = fVIP_BRG - 270;

	GetValues(tempAng, &a, &b, iVIP_RNG);
	b = fabs(b);
	a = fabs(a);
	if(fVIP_BRG >= 0.0F && fVIP_BRG <= 90.0F)
	{
		xCurr += b;
		yCurr += a;
	}
	else if(fVIP_BRG > 90.0F && fVIP_BRG <= 180.0F)
	{
		xCurr -= a;
		yCurr += b;
	}
	else if(fVIP_BRG > 180.0F && fVIP_BRG <= 270.0F)
	{
		xCurr -= b;
		yCurr -= a;
	}
	else
	{
		xCurr += a;
		yCurr -= b;
	}
	WPAlt = iVIP_ALT;
	//set our new OffsetAimpoint
	gNavigationSys->SetVIPOAPoint(NavigationSystem::POS,xCurr,yCurr,WPAlt,0);
}
void ICPClass::SetVRP(void)
{
	if(!SimDriver.playerEntity->curWaypoint)
		return;
	//if we don't have a different altitude or a range, we just take the current waypoint
	if(iVRP_RNG == 0 && iVRP_ALT == 0)
	{
		SimDriver.playerEntity->curWaypoint->GetLocation(&xCurr, &yCurr, &zCurr);
		gNavigationSys->SetVRPOAPoint(NavigationSystem::POS,xCurr,yCurr,iVRP_ALT,1);// + 200,1);
		return;
	}
	//First, let's get our current waypoint
	SimDriver.playerEntity->curWaypoint->GetLocation(&xCurr, &yCurr, &zCurr);
	//let's find the sides a and b of our triangle
	float a = 0.0F, b = 0.0F;
	float tempAng = fVRP_BRG;
	GetValues(tempAng, &a, &b, iVRP_RNG);
	b = fabs(b);
	a = fabs(a);
	if(fVRP_BRG >= 0.0F && fVRP_BRG <= 90.0F)
	{
		xCurr += b;
		yCurr += a;
	}
	else if(fVRP_BRG > 90.0F && fVRP_BRG <= 180.0F)
	{
		xCurr -= b;
		yCurr += a;
	}
	else if(fVRP_BRG > 180.0F && fVRP_BRG <= 270.0F)
	{
		xCurr -= b;
		yCurr -= a;
	}
	else
	{
		xCurr += b;
		yCurr -= a;
	}
	WPAlt = iVRP_ALT;
	//set our new OffsetAimpoint
	gNavigationSys->SetVRPOAPoint(NavigationSystem::POS,xCurr,yCurr,WPAlt,0);
}
void ICPClass::GetValues(float Angle, float *a, float *b, int Range)
{
	mlTrig trig;
	Angle *= DTR;
	mlSinCos(&trig, Angle);
	*a = trig.sin * Range;
	*b = trig.cos * Range;
}
void ICPClass::EnterINTG()
{
	//CheckDigits();
	int INTGCode = AddUp();
	if(INTGCode == 0)
		WrongInput();
	else if(INTGCode < 10 && INTGCode > 0)
	{
		if(INTGCode == 1)
			ToggleIFFFlag(ICPClass::MODE_1);
		else if(INTGCode == 2)
			ToggleIFFFlag(ICPClass::MODE_2);
		else if(INTGCode == 3)
			ToggleIFFFlag(ICPClass::MODE_3);
			else if(INTGCode == 4)
			ToggleIFFFlag(ICPClass::MODE_4);
		else if(INTGCode == 5)	//toggle mode C
			ToggleIFFFlag(ICPClass::MODE_C);
		else if(INTGCode == 6)	//toggle mode 4B
			ToggleIFFFlag(ICPClass::MODE_4B);
		else if(INTGCode == 7)
		{
			if(IsIFFSet(ICPClass::MODE_4OUT))
			{

				ClearIFFFlag(ICPClass::MODE_4OUT);
				SetIFFFlag(ICPClass::MODE_4LIT);
			}
			else if(IsIFFSet(ICPClass::MODE_4LIT))
			{
				ClearIFFFlag(ICPClass::MODE_4LIT);
				ClearIFFFlag(ICPClass::MODE_4OUT);
			}
			else
			{
				SetIFFFlag(ICPClass::MODE_4OUT);
			}
		}
		ResetInput();
	}
	else if(INTGCode >= 10 && INTGCode < 54)
	{
		Mode1Code = INTGCode;
		ResetInput();
	}
	else if(INTGCode > 999 && INTGCode <= 7777)
	{
		Mode3Code = INTGCode;
		ResetInput();
	}
	else if(INTGCode >= 21000 && INTGCode <= 27777)
	{
		Mode2Code = INTGCode - 20000;
		ResetInput();
	}
	else
		WrongInput();
}
void ICPClass::EnterINSStuff(void)
{
	float Curlatitude	= (FALCON_ORIGIN_LAT * FT_PER_DEGREE + cockpitFlightData.x) / EARTH_RADIUS_FT;
	float CurcosLatitude = (float)cos(Curlatitude);
	float Curlongitude	= ((FALCON_ORIGIN_LONG * DTR * EARTH_RADIUS_FT * CurcosLatitude) + cockpitFlightData.y) / (EARTH_RADIUS_FT * CurcosLatitude);
		
	Curlatitude	*= RTD;
	Curlongitude *= RTD;

	if(INSLine == 0)
	{
		CheckDigits();
		float INSLATDegrees = ((Input_Digit1*100) + (Input_Digit2*10) + Input_Digit3);
		float INSLATMinutes = (((Input_Digit4*10 + Input_Digit5) * 1.66666666F) / 100);
		float INSLATSeconds = (((Input_Digit6*10 + Input_Digit7) * 0.02777777F) / 100);
		float INSLatf = INSLATDegrees + INSLATMinutes + INSLATSeconds;	
		if(INSLatf > 90)
		{
			WrongInput();
			return;
		}
		INSLATDiff = INSLatf - Curlatitude;
		INSEnter = TRUE;
		///////////
		//LAT String
		INSLat[10] = '\0';
		INSLat[9] = '\'';
		if(Input_Digit7 < 10)
			INSLat[8] = '0' + Input_Digit7;
		else
			INSLat[8] = '0';
		if(Input_Digit6 < 10)
			INSLat[7] = '0' + Input_Digit6;
		else 
			INSLat[7] = '0';
		INSLat[6] = '.';
		if(Input_Digit5 < 10)
			INSLat[5] = '0' + Input_Digit5;		
		else
			INSLat[5] = '0';
		if(Input_Digit4 < 10)
			INSLat[4] = '0' + Input_Digit4;
		else
			INSLat[4] = '0';
		INSLat[3] = '*'; //this is a ° symbol
		if(Input_Digit3 < 10)
			INSLat[2] = '0' + Input_Digit3;
		else
			INSLat[2] = '0';
		if(Input_Digit2 < 10)
			INSLat[1] = '0' + Input_Digit2;
		else
			INSLat[1] = '0';
		//////////
		ResetInput();
	}
	else if(INSLine == 1)
	{
		CheckDigits();
		float INSLONGDegrees = ((Input_Digit1*100) + (Input_Digit2*10) + Input_Digit3);
		float INSLONGMinutes = (((Input_Digit4*10 + Input_Digit5) * 1.66666666F) / 100);
		float INSLONGSeconds = (((Input_Digit6*10 + Input_Digit7) * 0.02777777F) / 100);
		
		float INSLongf = INSLONGDegrees + INSLONGMinutes + INSLONGSeconds;
		if(INSLongf > 180)
		{
			WrongInput();
			return;
		}
		INSLONGDiff = INSLongf - Curlongitude;
		INSEnter = TRUE;
		//force it to display what we've entered
		INSLong[10] = '\0';
		INSLong[9] = '\'';
		if(Input_Digit7 < 10)
			INSLong[8] = '0' + Input_Digit7;
		else
			INSLong[8] = '0';
		if(Input_Digit6 < 10)
			INSLong[7] = '0' + Input_Digit6;
		else 
			INSLong[7] = '0';
		INSLong[6] = '.';
		if(Input_Digit5 < 10)
			INSLong[5] = '0' + Input_Digit5;
		else
			INSLong[5] = '0';
		if(Input_Digit4 < 10)
			INSLong[4] = '0' + Input_Digit4;
		else
			INSLong[4] = '0';
		INSLong[3] = '*'; //this is a ° symbol
		if(Input_Digit3 < 10)
			INSLong[2] = '0' + Input_Digit3;
		else
			INSLong[2] = '0';
		if(Input_Digit2 < 10)
			INSLong[1] = '0' + Input_Digit2;
		else
			INSLong[1] = '0';
		if(Input_Digit1 < 10)
			INSLong[0] = '0' + Input_Digit1;
		else
			INSLong[0] = '0';
		ResetInput();
	}
	else if(INSLine == 2)
	{
		//force it to display what we've entered
		altStr[7] = '\0';
		altStr[6] = 'T';
		altStr[5] = 'F';
		if(Input_Digit7 < 10)
			altStr[4] = '0' + Input_Digit7;
		else
			altStr[4] = ' ';
		if(Input_Digit6 < 10)
			altStr[3] = '0' + Input_Digit6;
		else 
			altStr[3] = ' ';
		if(Input_Digit5 < 10)
			altStr[2] = '0' + Input_Digit5;
		else
			altStr[2] = ' ';
		if(Input_Digit4 < 10)
			altStr[1] = '0' + Input_Digit4;
		else
			altStr[1] = ' ';
		if(Input_Digit3 < 10)
			altStr[0] = '0' + Input_Digit3;
		else
			altStr[0] = ' ';
		//calc the diff
		INSALTDiff = AddUp() - (long)-cockpitFlightData.z;
		INSEnter = TRUE;
		ResetInput();
	}
	else if(INSLine == 3)
	{
		//heading
		float head = cockpitFlightData.yaw;
		if(head < 0.0F)
			head += 2 * PI;
		head *= RTD;
		CheckDigits();
		float Input = 0.0F;
		Input += Input_Digit7;
		Input *= 0.1F;
		Input += Input_Digit6*1;
		Input += Input_Digit5*10;
		Input += Input_Digit4*100;
		if(Input > 360.0F)
		{
			WrongInput();
			return;
		}
		else
			sprintf(INSHead, "%3.1f*", Input);
		//calc the diff
		INSHDGDiff = Input - head;
		INSEnter = TRUE;
		ClearStrings();
		ResetInput();
	}
}
void ICPClass::EnterLaser(void)
{
	int temp = AddUp();
	if(LaserLine == 1)
	{
		if(temp < 2889 && temp > 1120)
		{
			LaserCode = temp;
			ResetInput();
		}
		else
			WrongInput();
	}
	else
	{
		if(temp < 177 && temp > 0)
		{
			LaserTime = temp;
			ResetInput();
		}
		else
			WrongInput();
	}
}
