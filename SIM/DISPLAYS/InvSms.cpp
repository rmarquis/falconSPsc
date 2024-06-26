#include "stdhdr.h"
#include "sms.h"
#include "hardpnt.h"
#include "guns.h"
#include "Graphics\Include\display.h"
#include "SmsDraw.h"
#include "aircrft.h"
#include "airframe.h"
#include "fcc.h"
#include "simdrive.h"

// JPO - taken a hatchet to all this stuff... some bits kept.
const static struct InvData {
    enum { NORACK = 0x1, R2SLOT = 0x2, RJETT = 0x4,};
    float text_x, text_y;
    float boxx, boxy;
    int flags;
} HpInvData[10] = {
	/*{ 0 }, // HP 0 is empty
    {-0.90f, -0.62f,	-0.95f, -0.60f, InvData::RJETT|InvData::NORACK|InvData::R2SLOT}, // HP 1
    {-0.90f, -0.40f,	-0.95f, -0.38f, InvData::R2SLOT}, // HP 2
    {-0.75f, -0.00f,	-0.77f, -0.02f, 0}, // HP 3
    {-0.60f,  0.40f,	-0.62f,  0.42f, 0}, // HP 4
    {-0.15f,  0.80f,	-0.17f,  0.82f, 0}, // HP 5
    { 0.10f,  0.40f,	 0.08f,  0.42f, 0}, // HP 6
    { 0.25f, -0.00f,	 0.23f,  0.02f, 0}, // HP 7
    { 0.40f, -0.40f,	 0.35f, -0.38f, InvData::R2SLOT}, // HP 8
    { 0.40f, -0.62f,	 0.35f, -0.60f, InvData::RJETT|InvData::NORACK|InvData::R2SLOT}, // HP 9*/
    { 0 }, // HP 0 is empty
	//MI tweaked values
    {-0.90f, -0.50f,	-0.95f, -0.50f, InvData::RJETT|InvData::NORACK|InvData::R2SLOT}, // HP 1
    {-0.90f, -0.25f,	-0.95f, -0.25f, InvData::R2SLOT}, // HP 2
    {-0.75f,  0.10f,	-0.77f,  0.10f, 0}, // HP 3
    {-0.60f,  0.40f,	-0.62f,  0.42f, 0}, // HP 4
    {-0.15f,  0.82f,	-0.15f,  0.84f, 0}, // HP 5
    { 0.10f,  0.40f,	 0.08f,  0.42f, 0}, // HP 6
    { 0.25f,  0.10f,	 0.25f,  0.10f, 0}, // HP 7
    { 0.40f, -0.25f,	 0.35f, -0.25f, InvData::R2SLOT}, // HP 8
    { 0.40f, -0.50f,	 0.35f, -0.50f, InvData::RJETT|InvData::NORACK|InvData::R2SLOT}, // HP 9
};

void SmsDrawable::InventoryDisplay (int jettOnly)
{
    char tmpStr1[12];
    char tmpStr2[sizeof (tmpStr1)];
    int i, numRounds = 0,rack = 1;
    
	for (i = 1; i < 10; i++)
	    InvDrawHp (i, jettOnly);

   if (!jettOnly)
   {
      if( ((AircraftClass*)Sms->ownship)->af->IsSet(AirframeClass::CATLimiterIII) )
      {
	      sprintf (tmpStr1, "CAT III");/*
	      limitGs = ((AircraftClass*)Sms->ownship)->af->curMaxGs;
	      if( limiter = gLimiterMgr->GetLimiter(CatIIIMaxGs, ((AircraftClass*)Sms->ownship)->af->VehicleIndex()) )
			   limitGs = limiter->Limit(0);*/
	      sprintf(tmpStr2, "%2.1f G", ((AircraftClass*)Sms->ownship)->af->curMaxGs);
	      
      }
      else
      {
	      sprintf (tmpStr1, "CAT I");
	      sprintf(tmpStr2, "%2.1f G", ((AircraftClass*)Sms->ownship)->af->curMaxGs);
      }

      ShiAssert (strlen(tmpStr1) < sizeof (tmpStr1));
      ShiAssert (strlen(tmpStr2) < sizeof (tmpStr1));
      display->TextCenter(0.0F,-0.5F,tmpStr1);
      display->TextCenter(0.0F,-0.6F,tmpStr2);

      FireControlComputer* pFCC = Sms->ownship->GetFCC();
      // Label the buttons
      TopRow(1);
      BottomRow();

      for (i=0; i<Sms->NumHardpoints(); i++)
      {
	     GunClass *gun = Sms->GetGun(i);
	     if (gun)
            numRounds += gun->numRoundsRemaining / 10;
      }
      sprintf (tmpStr1, "%02dGUN", numRounds / 10);
      ShiAssert (strlen(tmpStr1) < sizeof (tmpStr1));
      LabelButton (19, tmpStr1, "PGU28");
   }
}

// Show the weapon load
// Note - This is F16 specific and the following assumptions are made
// 1 - Aim9's are always on a LNCHW - no rack
// 2 - Aim120's are always on an MLRW - no rack
// 3 - Everything else uses a MAU
// 4 - If there's more than 1 use a TER


// JPO - generalise into one routine.
void SmsDrawable::InvDrawHp (int hp, int jettOnly)
{
    ShiAssert (hp > 0 && hp < 10);
    char tmpStr1[12];
    char tmpStr2[sizeof (tmpStr1)];
    char tmpStr3[sizeof (tmpStr1)];
    char* countStr;
    
    if (Sms->NumHardpoints() > hp && ((HpInvData[hp].flags & InvData::RJETT) ==0 || !jettOnly))
    {
		int rack;
		if(Sms->hardPoint[hp]->GetRack() || (HpInvData[hp].flags & InvData::NORACK))
			rack = 1;
		else
			rack = 0;
		
		if (Sms->hardPoint[hp]->GetWeaponType() == wtAim9)
		{
			sprintf (tmpStr1, "%d LNCHW", rack);
			sprintf (tmpStr2, "%d %s", Sms->hardPoint[hp]->weaponCount, Sms->hardPoint[hp]->GetWeaponData()->mnemonic);
			sprintf (tmpStr3, "-------");
		}
		else if (Sms->hardPoint[hp]->GetWeaponType() == wtAim120)
		{
			sprintf (tmpStr1, "%d MLRW", rack);
			sprintf (tmpStr2, "%d %s", Sms->hardPoint[hp]->weaponCount, Sms->hardPoint[hp]->GetWeaponData()->mnemonic);
			sprintf (tmpStr3, "-------");
		}
		else if (Sms->hardPoint[hp]->GetWeaponType() == wtNone)
		{
			sprintf (tmpStr1, "-------");
			sprintf (tmpStr2, "-------");
			sprintf (tmpStr3, "-------");
		}
		// fuel tanks are directly on the HP.
		else if (Sms->hardPoint[hp]->GetWeaponClass() == wcTank) {
			sprintf (tmpStr1, "%d %s", Sms->hardPoint[hp]->weaponCount, Sms->hardPoint[hp]->GetWeaponData()->mnemonic);
			sprintf (tmpStr2, "-------");
			sprintf (tmpStr3, "-------");
		}
		else
		{
			if (rack)
				sprintf (tmpStr1, "%d MAU",rack);
			else
				strcpy(tmpStr1, "");

			if (Sms->hardPoint[hp]->NumPoints() > 1)
			{
				sprintf (tmpStr2, "%d TER",rack);
				countStr = tmpStr3;
			}
			else
			{
				countStr = tmpStr2;
				sprintf (tmpStr3, "-------");
			}
			sprintf (countStr, "%d %s", Sms->hardPoint[hp]->weaponCount, Sms->hardPoint[hp]->GetWeaponData()->mnemonic);
		}
    }
    else
    {
		sprintf (tmpStr1, "-------");
		sprintf (tmpStr2, "-------");
		sprintf (tmpStr3, "-------");
    }
    
    ShiAssert (strlen(tmpStr1) < sizeof (tmpStr1));
    ShiAssert (strlen(tmpStr2) < sizeof (tmpStr1));
    ShiAssert (strlen(tmpStr3) < sizeof (tmpStr1));
    display->TextLeft (HpInvData[hp].text_x, HpInvData[hp].text_y, tmpStr1);
    display->TextLeft (HpInvData[hp].text_x, HpInvData[hp].text_y - 0.1f, tmpStr2);
    if ((HpInvData[hp].flags & InvData::R2SLOT) == 0)
		display->TextLeft (HpInvData[hp].text_x, HpInvData[hp].text_y - 0.2f, tmpStr3);
    
	//MI changed
	if(!g_bRealisticAvionics)
	{
		if (hardPointSelected & (1 << hp) || (Sms->curHardpoint == hp && !jettOnly))
		{
			float bx = HpInvData[hp].boxx;
			float by = HpInvData[hp].boxy;
			float dy = 0.25f;
			if ((HpInvData[hp].flags & InvData::R2SLOT) == 0)
				dy += 0.1f;
			display->Line (bx, by, bx, by - dy);
			display->Line (bx + 0.6f, by, bx + 0.6f, by - dy);
			display->Line (bx, by, bx + 0.6f, by);
			display->Line (bx + 0.6f, by - dy, bx, by - dy);
		}
	}
	else
	{
		if(hardPointSelected & (1 << hp) || (Sms->curHardpoint == hp && !jettOnly)
			&& Sms->hardPoint[hp]->GetWeaponType() != wtNone)
		{
			float bx = HpInvData[hp].boxx;
			float by = HpInvData[hp].boxy;
			float dy = 0.23f;
			//float dy = 0.25f;
			if ((HpInvData[hp].flags & InvData::R2SLOT) == 0)
				dy += 0.1f;
			//tweaked
			display->Line (bx - 0.05F, by, bx - 0.05F, by - dy);
			display->Line (bx + 0.55F, by, bx + 0.55F, by - dy);
			display->Line (bx - 0.05F, by, bx + 0.55F, by);
			display->Line (bx + 0.55F, by - dy, bx - 0.05F, by - dy);
		}
	}
}
void SmsDrawable::InvPushButton (int whichButton, int whichMFD)
{
    
    FireControlComputer* pFCC = Sms->ownship->GetFCC();

    switch (whichButton)
    {
    case 3:
	if (!pFCC->IsNavMasterMode())
	    SetDisplayMode(Wpn);
	break;
	
    case 10:
	{
	    SetDisplayMode (SelJet);
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
	
   }
   // 2000-08-26 ADDED BY S.G. SO WE SAVE THE JETTISON SELECTION
   //	if (displayMode == SelJet)	// Until I fixe oldp01 being private
   //		((AircraftClass *)Sms->ownship)->af->oldp01[5] = hardPointSelected;
}
