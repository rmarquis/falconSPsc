#include "stdhdr.h"
#include "hud.h"
#include "fcc.h"
#include "object.h"
#include "aircrft.h"
#include "playerop.h"
#include "Graphics\Include\display.h"
#include "simbase.h"
#include "sms.h"
#include "simweapn.h"
#include "simdrive.h"
#include "missile.h"

#include "campbase.h"
#include "falcent.h"
#include "classtbl.h"

//MI
#include "airframe.h"

#define SRM_RETICLE_SIZE		 0.35F
#define SRM_UNCAGE_RETICLE_SIZE  0.25F
#define SRM_REARAA_RETICLE_SIZE  0.13F

#define HARM_RETICLE_SIZE		 0.4F
#define MRM_RETICLE_SIZE		 0.6F
#define MSL_OVERRIDE_SIZE		 0.5F

#define AIM120ASECX              45.0f * DTR

extern bool g_bRealistivAvionics;

float FindMinDistance (vector* a, vector* c, vector* b, vector* d);

void HudClass::DrawDogfight()
{
    float ang;
    char str[12];
    // JPO dogfight can use aim120.
    if (FCC->GetDgftSubMode() == FireControlComputer::Aim120) 
	{
		if (targetPtr)
		{
			/*if ( flash || targetData->range > FCC->missileRneMax || targetData->range < FCC->missileRneMin)
			{
				DrawMissileReticle(FCC->Aim120ASECRadius(targetData->range), FALSE, TRUE);
			}*/
			//MI changed
			if(!g_bRealisticAvionics)
				DrawTDBox();
			//else
			//	DrawAATDBox();
	    
			DrawAim120DLZ(true);
			DrawAim120ASE();
			// with a target locked
			DrawAim120Diamond();
		}
		else 
		{
		    //MI make it dependant on missil bore/slave
			if(SimDriver.playerEntity && SimDriver.playerEntity->Sms && SimDriver.playerEntity->Sms->curWeapon && 
				((MissileClass*)SimDriver.playerEntity->Sms->curWeapon)->isSlave)
				DrawMissileReticle(0.3F, FALSE, TRUE);
			else
				DrawMissileReticle(MRM_RETICLE_SIZE, FALSE, TRUE);
			DrawAim120Diamond();
		}
    }
    else 
	{
	
		if (targetPtr)
		{
		    //MI changed
			if(!g_bRealisticAvionics)
				DrawTDBox();
// Marco Edit - not TD Box in Dogfight mode
//			else
//				DrawAATDBox();
			DrawAim9DLZ();  
			DrawAim9Diamond();//me123 addet
		}
		else
		{
		    DrawAim9Diamond();
		}
    }
    DrawGuns();
    CheckBreakX();
    
    // Add target aspect angle
    if (targetPtr)
    {
	ang = max ( min (targetData->aspect * RTD * 0.1F, 18.0F), -18.0F);
	sprintf (str, "AA %02.0f%c", ang, (targetData->azFrom > 0.0F ? 'R' : 'L'));
	display->TextCenter (
	    hudWinX[BORESIGHT_CROSS_WINDOW] + hudWinWidth[BORESIGHT_CROSS_WINDOW] * 0.5F,
	    hudWinY[BORESIGHT_CROSS_WINDOW] + hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F + 0.05F +
	    1.2F * display->TextHeight(), str);
    }
}

void HudClass::DrawMissileOverride()//me123 addet aim9/120 check
{
	SimWeaponClass* wpn; 
	wpn = SimDriver.playerEntity->Sms->curWeapon; // Marco Edit - for Aim9 Reticle Size
	
#ifdef DEBUG
	if(wpn)
		ShiAssert(wpn->IsMissile());
#endif

    switch (FCC->GetSubMode())
    {
    case FireControlComputer::Aim9:
	if (targetPtr)
	{
	    //MI changed
		if(!g_bRealisticAvionics)
			DrawTDBox();
		else
			DrawAATDBox();
	    DrawAim9DLZ();
	    DrawAim9Diamond();//me123 addet
	}
	else
	{
			    
	    // Marco Edit - check for our missile type (ie. REAR ASPECT)
	    // if (wc && wc->Flags & WEAP_REAR_ASPECT)
		// Marco Edit - hack - check for 9P specifically)
		if (g_bRealisticAvionics && wpn && ((CampBaseClass*)wpn)->GetSPType() == SPTYPE_AIM9P)
		{
			DrawMissileReticle(SRM_REARAA_RETICLE_SIZE, TRUE, TRUE);

		}
		else
		{
			// if (((MissileClass*)wpn)->isCaged && !((MissileClass*)wpn)->isSpot)
			if (g_bRealisticAvionics && wpn && (((MissileClass*)wpn)->isSpot || ( !((MissileClass*)wpn)->isCaged && ((MissileClass*)wpn)->targetPtr )))
			{
				DrawMissileReticle(SRM_UNCAGE_RETICLE_SIZE, TRUE, TRUE);
			}
			else
			{
				DrawMissileReticle(SRM_RETICLE_SIZE, TRUE, TRUE);
			}
		}
	    DrawAim9Diamond();
	}
	break;
	
    case FireControlComputer::Aim120:
	if (targetPtr)
	{
	  // JPO flashing resizing reticle
	    if ( flash ||
		targetData->range > FCC->missileRneMax ||
		targetData->range < FCC->missileRneMin)
		DrawMissileReticle(FCC->Aim120ASECRadius(targetData->range), FALSE, TRUE);
	    //MI changed
		if(!g_bRealisticAvionics)
			DrawTDBox();
		else
			DrawAATDBox();
	    DrawAim120DLZ(false);
	    DrawAim120ASE();
	}
	else {
	  // else static one
		//MI make it dependant on missil bore/slave
		if(SimDriver.playerEntity && SimDriver.playerEntity->Sms && SimDriver.playerEntity->Sms->curWeapon && 
			((MissileClass*)SimDriver.playerEntity->Sms->curWeapon)->isSlave)
			DrawMissileReticle(0.3F, FALSE, TRUE);
		else
			DrawMissileReticle(MRM_RETICLE_SIZE, FALSE, TRUE);
	    DrawAim120Diamond();
	}
	break;
	//MI
	case FireControlComputer::EEGS:
	case FireControlComputer::LCOS:
	case FireControlComputer::Snapshot:
		if(g_bRealisticAvionics)
		{
			DrawGuns();
			DrawAATDBox();
		}
	break;
    }
    
    // Add waypoint info
    if (waypointValid)
    {
	TimeToSteerpoint();
	RangeToSteerpoint();
    }
    
    CheckBreakX();
}


void HudClass::DrawAirMissile (void)
{
	SimWeaponClass* wpn; 
	wpn = SimDriver.playerEntity->Sms->curWeapon; // Marco Edit - for Aim9 Reticle Size
	ShiAssert(wpn == NULL || wpn->IsMissile());
	switch (FCC->GetSubMode())
	{
	case FireControlComputer::Aim9:
		if (targetPtr)
		{
			//MI changed
			if(!g_bRealisticAvionics)
				DrawTDBox();
			else
				DrawAATDBox();
			DrawAim9DLZ();
			DrawAim9Diamond();//me123 addet
		}
		else
		{
			if (wpn)
			{
				// Marco Edit - check for our missile type (ie. REAR ASPECT)
				// if (wc && wc->Flags & WEAP_REAR_ASPECT)
				// Marco Edit - hack - check for 9P specifically)
				wpn = SimDriver.playerEntity->Sms->curWeapon; // Marco Edit - for Aim9 Reticle Size
				if (g_bRealisticAvionics && ((CampBaseClass*)wpn)->GetSPType() == SPTYPE_AIM9P)
				{
					DrawMissileReticle(SRM_REARAA_RETICLE_SIZE, TRUE, TRUE);
		
				}
				else
				{
					// if (((MissileClass*)wpn)->isCaged && !((MissileClass*)wpn)->isSpot)
					if ( g_bRealisticAvionics && ((MissileClass*)wpn)->isSpot || ( !((MissileClass*)wpn)->isCaged && ((MissileClass*)wpn)->targetPtr ) )
					{
						DrawMissileReticle(SRM_UNCAGE_RETICLE_SIZE, TRUE, TRUE);
					}
					else
					{
						DrawMissileReticle(SRM_RETICLE_SIZE, TRUE, TRUE);
					}
				}
			}
		}
		DrawAim9Diamond();
		break;
		
	case FireControlComputer::Aim120:
	    if (targetPtr)
	    {
		if ( flash ||
		    targetData->range > FCC->missileRneMax ||
		    targetData->range < FCC->missileRneMin)
		    DrawMissileReticle(FCC->Aim120ASECRadius(targetData->range), FALSE, TRUE);
		//MI changed
		if(!g_bRealisticAvionics)
			DrawTDBox();
		else
			DrawAATDBox();
		DrawAim120DLZ(false);
		DrawAim120ASE();
	    }
	    else {
		//MI make it dependant on missil bore/slave
		if(SimDriver.playerEntity && SimDriver.playerEntity->Sms && SimDriver.playerEntity->Sms->curWeapon && 
			((MissileClass*)SimDriver.playerEntity->Sms->curWeapon)->isSlave)
			DrawMissileReticle(0.3F, FALSE, TRUE);
		else
			DrawMissileReticle(MRM_RETICLE_SIZE, FALSE, TRUE);
		DrawAim120Diamond();
	    }
	    break;
	}
	
	// Add waypoint info
	if (waypointValid)
	{
		TimeToSteerpoint();
		RangeToSteerpoint();
	}
	
	CheckBreakX();
}


void HudClass::DrawGroundMissile (void)
{
	// If we have a target, draw the DLZ
	if (targetPtr)
	{
		DrawAGMDLZ();
	}

	// Draw the TD box
	
	display->AdjustOriginInViewport (0.0F, (hudWinY[BORESIGHT_CROSS_WINDOW] +
		hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
	
	// Marco edit - change size of Mav TD to approx DTOS size. (copied from DTOS code) - 6 Apr 2001 by Mirv's Suggestion
	// DrawDesignateMarker (Square, FCC->groundDesignateAz, FCC->groundDesignateEl, FCC->groundDesignateDroll);
	if(!g_bRealisticAvionics)
		DrawDesignateMarker(Square, FCC->groundDesignateAz, FCC->groundDesignateEl, FCC->groundDesignateDroll);
	else
		DrawTDMarker(FCC->groundDesignateAz, FCC->groundDesignateEl, FCC->groundDesignateDroll,0.03F);
	// End Marco Edit
	
	display->AdjustOriginInViewport (0.0F, -(hudWinY[BORESIGHT_CROSS_WINDOW] +
		hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
	
	// Add waypoint info
	if (waypointValid)
	{
		TimeToSteerpoint();
		RangeToSteerpoint();
	}
	
	CheckBreakX();
	//MI
	if(g_bRealisticAvionics && !FCC->preDesignate)
		DrawBearing();
}
//MI
void HudClass::DrawBearing(void)
{
	float steeringLineX = 0.0F;
	// Steering Line
	if(SimDriver.playerEntity && SimDriver.playerEntity->Sms)
	{
		if(SimDriver.playerEntity->Sms->MavSubMode == SMSBaseClass::PRE)
		{
			steeringLineX = FCC->groundDesignateAz / (20.0F * DTR);
			steeringLineX += betaHudUnits;
			steeringLineX = min ( max (steeringLineX , -1.0F), 1.0F);
			display->Line(steeringLineX, 1.0F, steeringLineX, -1.0F);
		}
	}					
}
void HudClass::DrawHarm (void)
{
	if (targetPtr)
	{
		DrawTDBox();
		DrawHTSDLZ();
	}
	else
	{
		DrawMissileReticle(HARM_RETICLE_SIZE, FALSE, FALSE);
	}

	// Add waypoint info
	if (waypointValid)
	{
		TimeToSteerpoint();
		//MI
		if(g_bRealisticAvionics)
		{
			if(FCC->nextMissileImpactTime <= 0.0F)
				RangeToSteerpoint();
		}
		else
			RangeToSteerpoint();
	}
	
	CheckBreakX();
}


void HudClass::DrawMissileReticle(float radius, int showRange, int showAspect)
{
float rangeTic1X, rangeTic1Y;
float rangeTic2X, rangeTic2Y;
float angle, tmpRange;
char tmpStr[12];
mlTrig trig;

	if (targetPtr == NULL)
   {
      display->Circle (0.0F, hudWinY[BORESIGHT_CROSS_WINDOW] +
      hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F + MISSILE_RETICLE_OFFSET, radius);
   }
   else
   {
      display->AdjustOriginInViewport (0.0F, hudWinY[BORESIGHT_CROSS_WINDOW] +
         hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F + MISSILE_RETICLE_OFFSET);

      // Range Tick
      angle = (targetData->range/12000.0F) * 360.0F * DTR;

      if (targetData->range < 12000.0F)
      {
         if (showRange)
         {
            if (angle >= (90.0F * DTR))
            {
               display->Arc (0.0F, 0.0F, radius, 0.0F, angle - (90.0F * DTR));
               display->Arc (0.0F, 0.0F, radius, 270.0F * DTR, 360.0F * DTR);
            }
            else
            {
               display->Arc (0.0F, 0.0F, radius, 270.0F * DTR, 270.0F * DTR + angle);
            }
         }
         else
         {
            display->Circle (0.0F, 0.0F, radius);
         }

         mlSinCos (&trig, angle);
         rangeTic1X = radius * trig.sin;
         rangeTic1Y = radius * trig.cos;
         rangeTic2X = 0.9F * rangeTic1X;
         rangeTic2Y = 0.9F * rangeTic1Y;
         display->Line (rangeTic1X, rangeTic1Y, rangeTic2X, rangeTic2Y);
      }
      else
      {
         display->Circle (0.0F, 0.0F, radius);
      }

      // Reference Ticks
	  // M.N. added full realism mode
      if (targetData->range < 12000.0F || (PlayerOptions.GetAvionicsType() != ATRealistic && PlayerOptions.GetAvionicsType() != ATRealisticAV))
      {
         display->Line (0.0F,  radius, 0.0F,  radius + 0.04F);
         display->Line (0.0F, -radius, 0.0F, -radius - 0.04F);
         display->Line ( radius, 0.0F,  radius + 0.04F, 0.0F);
         display->Line (-radius, 0.0F, -radius - 0.04F, 0.0F);
      }

	  // Aspect caret
	  if (showAspect)
     {
        // If you are on his right, draw it on the left
        if (targetData->azFrom < 0.0F)
		     display->AdjustRotationAboutOrigin (targetData->ataFrom);
        else
		     display->AdjustRotationAboutOrigin (-targetData->ataFrom);
		  display->Line ( 0.0F, radius,  -0.04F, radius + 0.04F);
		  display->Line ( 0.0F, radius,   0.04F, radius + 0.04F);
		  display->ZeroRotationAboutOrigin ();
	  }

      if (targetData->range > 1.0F * NM_TO_FT)
      {
         tmpRange = min (targetData->range * FT_TO_NM, 1000.0F);
         sprintf (tmpStr, "F %4.1f", tmpRange);
      }
      else
      {
         tmpRange = min (targetData->range * 0.01F, 10000.0F);
         sprintf (tmpStr, "F  %03.0f", targetData->range * 0.01F);
      }
      ShiAssert (strlen(tmpStr) < sizeof(tmpStr));
      display->AdjustOriginInViewport (0.0F, -(hudWinY[BORESIGHT_CROSS_WINDOW] +
         hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F + MISSILE_RETICLE_OFFSET));
//      DrawWindowString(10, tmpStr);
   }
}
const static float small = 0.03F;
const static float large = 0.06F;
void HudClass::DrawAim9Diamond(void)
{

	float xPos, yPos;
	SimWeaponClass *wpn; // JPO fix up


   wpn = SimDriver.playerEntity->Sms->curWeapon;
   // Marco - this shouldn't happen
   if (!wpn)
	   return ;
    ShiAssert(wpn->IsMissile());
   display->AdjustOriginInViewport (0.0F, hudWinY[BORESIGHT_CROSS_WINDOW] +
      hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F);

   if (1)//me123 (FCC->missileTarget)
   {
	   xPos = RadToHudUnits(FCC->missileSeekerAz);
	   yPos = RadToHudUnits(FCC->missileSeekerEl);

	   // Marco Edit - if seeker is Slave and/or SPOT and no target, then vibrate
	   if (g_bRealisticAvionics && SimDriver.playerEntity->Sms->curWeaponType == wtAim9)
	   {
 			if (!((MissileClass*)wpn)->targetPtr && (((MissileClass*)wpn)->isCaged || ((MissileClass*)wpn)->isSpot))
			{
				if (FCC->Aim9AtGround)
				{
					xPos = xPos + (( (float)rand()/(float)RAND_MAX) - 0.5f) * 0.015f;
					yPos = yPos + (( (float)rand()/(float)RAND_MAX) - 0.5f) * 0.015f;
				}
				else
				{
					xPos = xPos + (( (float)rand()/(float)RAND_MAX) - 0.5f) * 0.008f;
					yPos = yPos + (( (float)rand()/(float)RAND_MAX) - 0.5f) * 0.008f;
				}
			}
			if (((MissileClass*)wpn)->targetPtr)// && !((MissileClass*)wpn)->isCaged )
			{
				// Marco - here we have an uncaged seeker with a target locked
		   		xPos = xPos + (( (float)rand()/(float)RAND_MAX)  - 0.5f) * 0.01f;
				yPos = yPos + (( (float)rand()/(float)RAND_MAX)  - 0.5f) * 0.01f;
			}
	   }

   }

   else
   {
      xPos = 0.0F;
      yPos =  -(hudWinY[BORESIGHT_CROSS_WINDOW] +
         hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F) -1.0F;
   }

   // Marco Edit - AIM9P diamond stays in the centre of the HUD
   /*wpn = SimDriver.playerEntity->Sms->curWeapon;
   if (g_bRealisticAvionics && ((CampBaseClass*)wpn)->GetSPType() == SPTYPE_AIM9P)
   {
      xPos = RadToHudUnits( 0.0F );
      yPos =  RadToHudUnits( -6.0F * DTR );
   }*/


   if (fabs (xPos) < 0.90F && fabs(yPos + hudWinY[BORESIGHT_CROSS_WINDOW] +
      hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F) < 0.90F)
   {
	   //MI draw our diammond depending on seeker state (cage or not)
	   if(!g_bRealisticAvionics)
	   {
		   display->AdjustOriginInViewport(xPos, yPos);
		   display->Line (0.0F,  0.03F,  0.03F, 0.0F);//me123 from 0.05 to 0.03
		   display->Line (0.0F,  0.03F, -0.03F, 0.0F);
		   display->Line (0.0F, -0.03F,  0.03F, 0.0F);
		   display->Line (0.0F, -0.03F, -0.03F, 0.0F);
		   display->AdjustOriginInViewport(-xPos, -yPos);
	   }
	   else
	   {
		   //MI
		   if(wpn && wpn->IsMissile() && ((MissileClass*)wpn)->isCaged)
		   {
			   //small
			   display->AdjustOriginInViewport(xPos, yPos);
			   display->Line (0.0F,  small,  small, 0.0F);
			   display->Line (0.0F,  small, -small, 0.0F);
			   display->Line (0.0F, -small,  small, 0.0F);
			   display->Line (0.0F, -small, -small, 0.0F);
			   display->AdjustOriginInViewport(-xPos, -yPos);
		   }
		   // Marco Edit - flashing uncaged and locked diamond
		   else if(wpn && wpn->IsMissile() && !((MissileClass*)wpn)->isCaged)
		   {
			   if (!((MissileClass*)wpn)->targetPtr || vuxRealTime & 0x100)
			   {
				    //large
					display->AdjustOriginInViewport(xPos, yPos);
					display->Line (0.0F,  large,  large, 0.0F);
					display->Line (0.0F,  large, -large, 0.0F);
					display->Line (0.0F, -large,  large, 0.0F);
					display->Line (0.0F, -large, -large, 0.0F);
					display->AdjustOriginInViewport(-xPos, -yPos);
			   }
		   }
	   }
   }

   display->AdjustOriginInViewport (0.0F, -(hudWinY[BORESIGHT_CROSS_WINDOW] +
      hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
}

void HudClass::DrawAim9DLZ(void)
{
char tmpStr[12];
float percentRange;
float rMax, rMin;
float rNeMin, rNeMax;
float rDot;

	SimWeaponClass* wpn; 
	wpn = SimDriver.playerEntity->Sms->curWeapon; // Marco Edit - for Aim9 Reticle Size

	//MI prevent some strange things writte on the HUD
   if (FCC->missileTarget && SimDriver.playerEntity && SimDriver.playerEntity->Sms && 
	   SimDriver.playerEntity->Sms->curWeapon)
   {
      // Range Carat / Closure
      rMax   = FCC->missileRMax;
      rMin   = FCC->missileRMin;
      rNeMax = FCC->missileRneMax / FCC->missileWEZDisplayRange;
      rNeMin = FCC->missileRneMin / FCC->missileWEZDisplayRange;

      if (targetData->range < rMin || targetData->range > rNeMax || flash)
      {
         if (FCC->GetSubMode() == FireControlComputer::Aim9)
		 {
			 // Marco Edit - check for our missile type (ie. REAR ASPECT)
			 // if (wc && wc->Flags & WEAP_REAR_ASPECT)
			 // Marco Edit - hack - check for 9P specifically)
			 if (wpn)
			 {
				 if (((CampBaseClass*)wpn)->GetSPType() == SPTYPE_AIM9P)
				{
					DrawMissileReticle(SRM_REARAA_RETICLE_SIZE, TRUE, TRUE);
				}
				else
				{
					// if (((MissileClass*)wpn)->isCaged && !((MissileClass*)wpn)->isSpot)
					if ( ((MissileClass*)wpn)->isSpot || ( !((MissileClass*)wpn)->isCaged && ((MissileClass*)wpn)->targetPtr ))
					{
						DrawMissileReticle(SRM_UNCAGE_RETICLE_SIZE, TRUE, TRUE);
					}
					else
					{
						DrawMissileReticle(SRM_RETICLE_SIZE, TRUE, TRUE);
					}
				}
			}
		 }
	  }

      if (targetData->range < rMin || targetData->range > rMax || flash)
      {
         DrawAim9Diamond();
      }

      percentRange = targetData->range / FCC->missileWEZDisplayRange;

      rDot = max ( min (-targetData->rangedot * FTPSEC_TO_KNOTS, 1500.0F), -1500.0F);
      sprintf (tmpStr, "%.0f", rDot);
      ShiAssert (strlen(tmpStr) < sizeof(tmpStr));

      // RNeMin/RNeMax scale
      rMin = rMin / FCC->missileWEZDisplayRange;
      rMax = rMax / FCC->missileWEZDisplayRange;

      DrawDLZSymbol(percentRange, tmpStr, rMin, rMax, rNeMin, rNeMax, TRUE,0);

      sprintf (tmpStr, "%.0f", FCC->missileTOF);
      ShiAssert (strlen(tmpStr) < sizeof (tmpStr));
      DrawWindowString (32, tmpStr);
   }

   // Draw Time to die strings
   if (FCC->lastMissileImpactTime > 0.0F)
   {
      sprintf (tmpStr, "%.0f", FCC->lastMissileImpactTime);
      ShiAssert (strlen(tmpStr) < sizeof(tmpStr));
      DrawWindowString (37, tmpStr);
   }
}

void HudClass::DrawAim120Diamond(void)
{
float xPos, yPos;

   display->AdjustOriginInViewport (0.0F, hudWinY[BORESIGHT_CROSS_WINDOW] +
      hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F);

   if (FCC->missileTarget)
   {
	   xPos = RadToHudUnits(FCC->missileSeekerAz);
	   yPos = RadToHudUnits(FCC->missileSeekerEl);
   }
   else
   {
      xPos = 0.0F;
      yPos = MISSILE_RETICLE_OFFSET;
   }

   if (fabs (xPos) < 0.90F && fabs(yPos + hudWinY[BORESIGHT_CROSS_WINDOW] +
      hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F) < 0.90F)
   {
      display->AdjustOriginInViewport(xPos, yPos);
      display->Line (0.0F,  0.025F,  0.025F, 0.0F);
      display->Line (0.0F,  0.025F, -0.025F, 0.0F);
      display->Line (0.0F, -0.025F,  0.025F, 0.0F);
      display->Line (0.0F, -0.025F, -0.025F, 0.0F);

      display->Line ( 0.05F,  0.00F,  0.025F,  0.0F);
      display->Line (-0.05F,  0.00F, -0.025F,  0.0F);
      display->Line ( 0.0F, -0.025F,  0.00F, -0.05F);
      display->Line ( 0.0F,  0.025F,  0.00F,  0.05F);
      display->AdjustOriginInViewport(-xPos, -yPos);
   }

   display->AdjustOriginInViewport (0.0F, -(hudWinY[BORESIGHT_CROSS_WINDOW] +
      hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
}

void HudClass::DrawAim120DLZ(bool dfgt)
{
char tmpStr[12];
float percentRange;
float yOffset, rDot;
float rMax, rMin;
float rNeMin, rNeMax;

   if (FCC->missileTarget)
   {
      // Range Carat / Closure
      rMax   = FCC->missileRMax;
      rMin   = FCC->missileRMin;
      rNeMax = (FCC->missileRneMax); // Marco Edit -  * 0.70f);//me123 addet *0.70 
      rNeMin = FCC->missileRneMin;
      // JPO compare raw data first for ranging
      if (targetData->range < rNeMin || targetData->range > rNeMax || flash)
      {
         if (FCC->GetSubMode() == FireControlComputer::Aim120)
            DrawMissileReticle(FCC->Aim120ASECRadius(targetData->range), FALSE, TRUE);
      }

      if (targetData->range < rMin || targetData->range > rMax || flash && !dfgt)
      {
         DrawAim120Diamond();
      }

      percentRange = targetData->range / FCC->missileWEZDisplayRange;
      rDot = max ( min (-targetData->rangedot * FTPSEC_TO_KNOTS, 1500.0F), -1500.0F);
      sprintf (tmpStr, "%.0f", rDot);
      ShiAssert (strlen(tmpStr) < sizeof(tmpStr));

      // RNeMin/RNeMax scale
      rNeMax /= FCC->missileWEZDisplayRange;
      rNeMin /= FCC->missileWEZDisplayRange;
      rMin /= FCC->missileWEZDisplayRange;
      rMax /= FCC->missileWEZDisplayRange;
      DrawDLZSymbol(percentRange, tmpStr, rMin, rMax, rNeMin, rNeMax, TRUE,NULL);

      // Range for immediate Active
      percentRange = FCC->missileActiveRange / FCC->missileWEZDisplayRange;
      percentRange = min ( max ( rMin, percentRange), rMax);
      yOffset = hudWinY[DLZ_WINDOW] + percentRange * 0.8F * hudWinHeight[DLZ_WINDOW] +
         0.1F * hudWinHeight[DLZ_WINDOW];
      display->Circle (hudWinX[DLZ_WINDOW], yOffset, 0.02F);
   }
// Draw "LOOSE" string
   if (FCC->LastMissileWillMiss(targetData->range) && flash)
   {
       DrawWindowString (12, "LOSE");
   }


   //MI Draw "HOJ" string, feature of RP4, Home on Jamming
   else if (FCC->TargetPtr() && FCC->TargetPtr()->BaseData() &&  // JB 010708 CTD
		FCC->TargetPtr()->BaseData()->IsSPJamming())
   {
	   //only draw it when there is a missile in the air
	   if (FCC->lastMissileImpactTime > 0) 
		   DrawWindowString (12, "HOJ");
   }

   // Draw Time to die strings
   if (FCC->nextMissileImpactTime > 0.0F) 
   {
      if (FCC->nextMissileImpactTime >= FCC->missileActiveTime)
      {
         sprintf (tmpStr, "A%.0f", FCC->nextMissileImpactTime);
      }
      else
      {
         sprintf (tmpStr, "T%.0f", FCC->nextMissileImpactTime);
      }
      ShiAssert (strlen(tmpStr) < sizeof(tmpStr));
      DrawWindowString (32, tmpStr);
   }

   if (FCC->lastMissileImpactTime > 0.0F)
   {
       if (FCC->LastMissileWillMiss(targetData->range)) {
	   sprintf (tmpStr, "L%.0f", FCC->lastMissileImpactTime);
       }
       else if (FCC->lastMissileImpactTime >= FCC->missileActiveTime)
       {
	   sprintf (tmpStr, "A%.0f", FCC->lastMissileImpactTime );
       }
       else
       {
	   sprintf (tmpStr, "T%.0f", FCC->lastMissileImpactTime);
       }
      ShiAssert (strlen(tmpStr) < sizeof(tmpStr));
      DrawWindowString (37, tmpStr);
   }
}

void HudClass::DrawAim120ASE (void)
{
float rx, ry, rz;
float az, el;
vector collPoint;
float yCenter;
float rMax;

   yCenter = hudWinY[BORESIGHT_CROSS_WINDOW] +
			 hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F+ MISSILE_RETICLE_OFFSET;

   //MI possible CTD
   if(!FCC)
	   return;
   display->AdjustOriginInViewport (0.0F, yCenter );
   display->AdjustOriginInViewport (0.0F, MISSILE_RETICLE_OFFSET);
   // Add missile avg speed to collision calc
   if (targetPtr->BaseData()->IsSim() && FindCollisionPoint ((SimBaseClass*)targetPtr->BaseData(), ownship, &collPoint, 2500.0F))//me123 from speed 1500
   {
	   // edg: fix ASE.  Looks like collision point is returned in World Coords.  We need to
	   // make it relative to ownship so subtract out ownship pos 1st....
		rx =    collPoint.x -= ownship->XPos();
		ry =    collPoint.y -= ownship->YPos();
		rz =    collPoint.z -= ownship->ZPos();

		collPoint.z -= targetData->range/5.0f;
		collPoint.z -= (ownship->ZPos()+ 50000)*(targetData->range*FT_TO_NM /40.0f);

		rx =	ownship->dmx[0][0]*collPoint.x +
				ownship->dmx[0][1]*collPoint.y +
				ownship->dmx[0][2]*collPoint.z;
		ry =	ownship->dmx[1][0]*collPoint.x +
				ownship->dmx[1][1]*collPoint.y +
				ownship->dmx[1][2]*collPoint.z;
		rz =	ownship->dmx[2][0]*collPoint.x +
				ownship->dmx[2][1]*collPoint.y +
				ownship->dmx[2][2]*collPoint.z;

	  
      az = RadToHudUnits(((float)atan2 (ry,rx)));
	  az = az /6.0f;
      el = RadToHudUnits(((float)atan ((-rz)/(float)sqrt(rx*rx+ry*ry+.0001))));
	  el = el /6.0f;
//	  el -= MISSILE_RETICLE_OFFSET;
      if (fabs (az) < 0.9F && fabs (el  /*+ yCenter*/) < 0.9F)
      {
         display->Circle (az, el , MISSILE_ASE_SIZE);
		 rMax   = FCC->missileRMax;
//		 if ((float)fabs(FCC->missileSeekerAz) > AIM120ASECX || (float)fabs(FCC->missileSeekerEl) > AIM120ASECX || targetData->range > rMax)
		 if ((float)fabs(az) >= 0.9f || (float)fabs(el) >= 0.9f || targetData->range > rMax)
		 {
			// Display ASEC X
			display->Line(az - 0.6f*MISSILE_ASE_SIZE, el - 0.6f*MISSILE_ASE_SIZE,
				          az + 0.6f*MISSILE_ASE_SIZE, el + 0.6f*MISSILE_ASE_SIZE);
			display->Line(az - 0.6f*MISSILE_ASE_SIZE, el + 0.6f*MISSILE_ASE_SIZE,
				          az + 0.6f*MISSILE_ASE_SIZE, el - 0.6f*MISSILE_ASE_SIZE);
		 }
      }
      else if (flash)
      {
		 az = max( min( az, 0.9f ), -0.9f );
		 el = max( min( el, 0.9f ), -0.9f );
/*		 el +=  yCenter; 
		 if ( el < -0.9f )
			 el = -0.9f - yCenter;
		 else if ( el > 0.9f )
			 el = 0.9f - yCenter;
		 else
			 el -= yCenter;
*/
         display->Circle (az, el, MISSILE_ASE_SIZE);
		 rMax   = FCC->missileRMax;
//		 if ((float)fabs(FCC->missileSeekerAz) > AIM120ASECX || (float)fabs(FCC->missileSeekerEl) > AIM120ASECX || targetData->range > rMax)
		 if ((float)fabs(az) >= 0.9f || (float)fabs(el) >= 0.9f || targetData->range > rMax)

		 {
			// Display ASEC X
			display->Line(az - 0.6f*MISSILE_ASE_SIZE, el - 0.6f*MISSILE_ASE_SIZE,
				          az + 0.6f*MISSILE_ASE_SIZE, el + 0.6f*MISSILE_ASE_SIZE);
			display->Line(az - 0.6f*MISSILE_ASE_SIZE, el + 0.6f*MISSILE_ASE_SIZE,
				          az + 0.6f*MISSILE_ASE_SIZE, el - 0.6f*MISSILE_ASE_SIZE);
		 }
      }
   }
   else if (flash)
   {
      display->Circle (0.0F, MISSILE_RETICLE_OFFSET, MISSILE_ASE_SIZE); 
   }
   display->AdjustOriginInViewport (0.0F, -MISSILE_RETICLE_OFFSET);
   display->AdjustOriginInViewport (0.0F, -yCenter );
 
}

void HudClass::CheckBreakX(void)
{
vector a, b, c, d;

	// Quit now if nothing to do
	if (!targetPtr || flash)
		return;

   // Check the next 5 seconds.
   // Note, change sign on rates to project into the future
   a.x = ownship->XPos();
   a.y = ownship->YPos();
   a.z = ownship->ZPos();
   b.x = ownship->XDelta() * SimLibMajorFrameTime * -5.0F;
   b.y = ownship->YDelta() * SimLibMajorFrameTime * -5.0F;
   b.z = ownship->ZDelta() * SimLibMajorFrameTime * -5.0F;
   c.x = targetPtr->BaseData()->XPos();
   c.y = targetPtr->BaseData()->YPos();
   c.z = targetPtr->BaseData()->ZPos();
   d.x = (targetPtr->BaseData()->XDelta() + 0.5F) * SimLibMajorFrameTime * -5.0F;
   d.y = targetPtr->BaseData()->YDelta() * SimLibMajorFrameTime * -5.0F;
   d.z = targetPtr->BaseData()->ZDelta() * SimLibMajorFrameTime * -5.0F;

   if (FindMinDistance(&a, &b, &c, &d) < 500.0F)
   {
      display->AdjustOriginInViewport (0.0F, hudWinY[BORESIGHT_CROSS_WINDOW] +
         hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F);
      display->AdjustOriginInViewport (0.0F, MISSILE_RETICLE_OFFSET);
      display->Line (0.4F,  0.4F, -0.4F, -0.4F);
      display->Line (0.4F, -0.4F, -0.4F,  0.4F);
      display->AdjustOriginInViewport (0.0F, -MISSILE_RETICLE_OFFSET);
      display->AdjustOriginInViewport (0.0F, -(hudWinY[BORESIGHT_CROSS_WINDOW] +
         hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
   }
}

void HudClass::DrawAGMDLZ(void)
{
char tmpStr[12];
float percentRange;
float rMax, rMin;
float rNeMax, rNeMin;
float range;
float dx, dy, dz;

   if (FCC->missileTarget)
   {
     // Range Carat / Closure
     rMax   = FCC->missileRMax;
     rMin   = FCC->missileRMin;

	  // get range to ground designaate point
	  dx = ownship->XPos() - FCC->groundDesignateX;
	  dy = ownship->YPos() - FCC->groundDesignateY;
	  dz = ownship->ZPos() - FCC->groundDesignateZ;
	  range = (float)sqrt( dx*dx + dy*dy + dz*dz );

	  // normalize vector to ground designate
	  dx /= range;
	  dy /= range;
	  dz /= range;


      // Normailze the ranges for DLZ display
      percentRange = range / FCC->missileWEZDisplayRange;
      rMin = rMin / FCC->missileWEZDisplayRange;
      rMax = rMax / FCC->missileWEZDisplayRange;
      rNeMax = FCC->missileRneMax / FCC->missileWEZDisplayRange;
      rNeMin = FCC->missileRneMin / FCC->missileWEZDisplayRange;

      DrawDLZSymbol(percentRange, "", rMin , rMax, rNeMin, rNeMax, FALSE,0);

      sprintf (tmpStr, "%.0f", FCC->missileTOF);
      ShiAssert (strlen(tmpStr) < sizeof (tmpStr));
      DrawWindowString (32, tmpStr);
   }

   // Draw Time to die strings
   if (FCC->nextMissileImpactTime > 0.0F)
   {
      sprintf (tmpStr, "%.0f", FCC->nextMissileImpactTime);
      ShiAssert (strlen(tmpStr) < sizeof(tmpStr));
      DrawWindowString (37, tmpStr);
   }
}

void HudClass::DrawHTSDLZ(void)
{
char tmpStr[12];
float rMax, rMin;
float rNeMax, rNeMin;
float percentRange, xPos, yPos;
float boresightOffset;

   ShiAssert( targetData );

   // Range values
   rMax   = FCC->missileRMax;
   rMin   = FCC->missileRMin;

   if (targetData->range < rMin || targetData->range > rMax || flash)
   {
      boresightOffset = hudWinY[BORESIGHT_CROSS_WINDOW] + hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F;
	   xPos = RadToHudUnits(FCC->missileSeekerAz);
	   yPos = RadToHudUnits(FCC->missileSeekerEl) + boresightOffset;

      if (fabs (xPos) < 1.0F && fabs(yPos) < 1.0F)
      {
        display->Circle(xPos, yPos, 0.05F);
      }
      DrawMissileReticle(SRM_RETICLE_SIZE, FALSE, FALSE);
   }


   // Normailze the ranges for DLZ display
	// JB 010710 The DLZ should not be dependant on the HTS range
   /*
	 percentRange = targetData->range / FCC->missileWEZDisplayRange;
   rMin = rMin / FCC->missileWEZDisplayRange;
   rMax = rMax / FCC->missileWEZDisplayRange;
   rNeMax = FCC->missileRneMax / FCC->missileWEZDisplayRange;
   rNeMin = FCC->missileRneMin / FCC->missileWEZDisplayRange;
	*/
	 percentRange = targetData->range / FCC->missileRMax;
   rMin = rMin / FCC->missileRMax;
   rMax = 1;
   rNeMax = FCC->missileRneMax / FCC->missileRMax;
   rNeMin = FCC->missileRneMin / FCC->missileRMax;

   DrawDLZSymbol(percentRange, "", rMin, rMax, rNeMin, rNeMax, FALSE,0);


   // Draw Time to die strings
   if (FCC->nextMissileImpactTime > 0.0F)
   {
      sprintf (tmpStr, "%.0f", FCC->nextMissileImpactTime);
      ShiAssert (strlen(tmpStr) < sizeof(tmpStr));
	  //MI
	  if(g_bRealisticAvionics)
		  display->TextLeft(0.40F, -0.36F, tmpStr);
	  else
		  DrawWindowString (37, tmpStr);
   }
}

static void DrawShootCue(VirtualDisplay *display)
{
	static const float vSize = 0.10f;
	static const float hSize = 0.05f;

	struct Corner {
		float	x, y;
	};

	static const struct Corner S[] = {
		{ hSize,	 vSize},
		{-hSize,	 vSize},
		{-hSize,	 0.0f},
		{ hSize,	 0.0f},
		{ hSize,	-vSize},
		{-hSize,	-vSize}
	};

	static const struct Corner H1[] = {
		{hSize,		 vSize},
		{hSize,		-vSize},
	};
	static const struct Corner H2[] = {
		{-hSize,	 0.0f},
		{ hSize,	 0.0f},
	};
	static const struct Corner H3[] = {
		{-hSize,	 vSize},
		{-hSize,	-vSize}
	};

	static const struct Corner O[] = {
		{ hSize,	 vSize},
		{-hSize,	 vSize},
		{-hSize,	-vSize},
		{ hSize,	-vSize},
		{ hSize,	 vSize},
	};

	static const struct Corner T1[] = {
		{ hSize,	 vSize},
		{-hSize,	 vSize},
	};
	static const struct Corner T2[] = {
		{0.0f,		 vSize},
		{0.0f,		-vSize},
	};


	const struct Corner	*letter;
	int					i;

	display->AdjustOriginInViewport( -0.3f, 0.0f );
	letter = S;
	for (i=1; i<sizeof(S)/sizeof(Corner); i++) {
		display->Line( letter[i].x, letter[i].y, letter[i-1].x, letter[i-1].y );
	}

	display->AdjustOriginInViewport( 0.15f, 0.0f );
	letter = H1;
	for (i=1; i<sizeof(H1)/sizeof(Corner); i++) {
		display->Line( letter[i].x, letter[i].y, letter[i-1].x, letter[i-1].y );
	}
	letter = H2;
	for (i=1; i<sizeof(H2)/sizeof(Corner); i++) {
		display->Line( letter[i].x, letter[i].y, letter[i-1].x, letter[i-1].y );
	}
	letter = H3;
	for (i=1; i<sizeof(H3)/sizeof(Corner); i++) {
		display->Line( letter[i].x, letter[i].y, letter[i-1].x, letter[i-1].y );
	}

	display->AdjustOriginInViewport( 0.15f, 0.0f );
	letter = O;
	for (i=1; i<sizeof(O)/sizeof(Corner); i++) {
		display->Line( letter[i].x, letter[i].y, letter[i-1].x, letter[i-1].y );
	}

	display->AdjustOriginInViewport( 0.15f, 0.0f );
	letter = O;
	for (i=1; i<sizeof(O)/sizeof(Corner); i++) {
		display->Line( letter[i].x, letter[i].y, letter[i-1].x, letter[i-1].y );
	}

	display->AdjustOriginInViewport( 0.15f, 0.0f );
	letter = T1;
	for (i=1; i<sizeof(T1)/sizeof(Corner); i++) {
		display->Line( letter[i].x, letter[i].y, letter[i-1].x, letter[i-1].y );
	}
	letter = T2;
	for (i=1; i<sizeof(T2)/sizeof(Corner); i++) {
		display->Line( letter[i].x, letter[i].y, letter[i-1].x, letter[i-1].y );
	}

	display->AdjustOriginInViewport( -0.3f, 0.0f );
}

void HudClass::DrawDLZSymbol(float percentRange, char* tmpStr, float rMin, float rMax, float rNeMin, float rNeMax, BOOL aaMode,char* tmpStrpole)
{
float yOffset;
char wezStr[12];

   // Clamp in place
   rMin = min (rMin, 1.0F);
   rMax = min (rMax, 1.0F);
   rNeMin = min (rNeMin, 1.0F);
   rNeMax = min (rNeMax, 1.0F);

   sprintf (wezStr, "%.0f", FCC->missileWEZDisplayRange * FT_TO_NM);
   ShiAssert (strlen(wezStr) < sizeof(wezStr));
   display->TextCenter (hudWinX[DLZ_WINDOW] + hudWinWidth[DLZ_WINDOW]*0.5F,
      hudWinY[DLZ_WINDOW] + hudWinHeight[DLZ_WINDOW] + 1.2F * display->TextHeight(),
      wezStr);


   // Draw the symbol

   // Rmin/Rmax
   display->Line (
      hudWinX[DLZ_WINDOW],
      hudWinY[DLZ_WINDOW] + rMin * hudWinHeight[DLZ_WINDOW],
      hudWinX[DLZ_WINDOW] + hudWinWidth[DLZ_WINDOW],
      hudWinY[DLZ_WINDOW] + rMin * hudWinHeight[DLZ_WINDOW]);
   display->Line (
      hudWinX[DLZ_WINDOW],
      hudWinY[DLZ_WINDOW] + rMin * hudWinHeight[DLZ_WINDOW],
      hudWinX[DLZ_WINDOW],
      hudWinY[DLZ_WINDOW] + rMax * hudWinHeight[DLZ_WINDOW]);
   display->Line (
      hudWinX[DLZ_WINDOW],
      hudWinY[DLZ_WINDOW] + rMax * hudWinHeight[DLZ_WINDOW],
      hudWinX[DLZ_WINDOW] + hudWinWidth[DLZ_WINDOW],
      hudWinY[DLZ_WINDOW] + rMax * hudWinHeight[DLZ_WINDOW]);

   // Range Caret
   yOffset = hudWinY[DLZ_WINDOW] + percentRange * hudWinHeight[DLZ_WINDOW];

   display->Line (hudWinX[DLZ_WINDOW], yOffset,
      hudWinX[DLZ_WINDOW] - 0.03F, yOffset + 0.03F);
   display->Line (hudWinX[DLZ_WINDOW], yOffset,
      hudWinX[DLZ_WINDOW] - 0.03F, yOffset - 0.03F);

   display->TextRight (hudWinX[DLZ_WINDOW] - 0.035F, yOffset, tmpStr);
   if (tmpStrpole)
   display->TextRight (hudWinX[DLZ_WINDOW] - 0.035F, yOffset- display->TextHeight(), tmpStrpole);//me123

   // No Escape Zone
   if ( aaMode )
   {
	   display->Line (
		  hudWinX[DLZ_WINDOW],
		  hudWinY[DLZ_WINDOW] + rNeMin * hudWinHeight[DLZ_WINDOW],
		  hudWinX[DLZ_WINDOW] + hudWinWidth[DLZ_WINDOW],
		  hudWinY[DLZ_WINDOW] + rNeMin * hudWinHeight[DLZ_WINDOW]);
	   display->Line (
		  hudWinX[DLZ_WINDOW],
		  hudWinY[DLZ_WINDOW] + rNeMax * hudWinHeight[DLZ_WINDOW],
		  hudWinX[DLZ_WINDOW] + hudWinWidth[DLZ_WINDOW],
		  hudWinY[DLZ_WINDOW] + rNeMax * hudWinHeight[DLZ_WINDOW]);
	   display->Line (
		  hudWinX[DLZ_WINDOW] + hudWinWidth[DLZ_WINDOW],
		  hudWinY[DLZ_WINDOW] + rNeMin * hudWinHeight[DLZ_WINDOW],
		  hudWinX[DLZ_WINDOW] + hudWinWidth[DLZ_WINDOW],
		  hudWinY[DLZ_WINDOW] + rNeMax * hudWinHeight[DLZ_WINDOW]);
   }

   // In easy mode we generate a SHOOT prompt when the target is in the no escape zone
   if (PlayerOptions.GetAvionicsType() == ATEasy)
   {
	   if (flash) {
		   if ((percentRange < rNeMax) && (percentRange > rNeMin)) {
			   DrawShootCue(display);
		   }
	   }
   }
}
