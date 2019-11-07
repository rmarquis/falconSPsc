/******************************************************************************/
/*                                                                            */
/*  Unit Name : roll.cpp                                                      */
/*                                                                            */
/*  Abstract  : Roll axis control system                                      */
/*                                                                            */
/*  Dependencies : Auto-Generated                                             */
/*                                                                            */
/*  Operating System : MS-DOS 6.2,                                            */
/*                                                                            */
/*  Compiler : WATCOM C/C++ V10                                               */
/*                                                                            */
/*  Naming Conventions :                                                      */
/*                                                                            */
/*      Public Functions    : Mixed case with no underscores                  */
/*      Private Functions   : Mixed case with no underscores                  */
/*      Public Functions    : Mixed case with no underscores                  */
/*      Global Variables    : Mixed case with no underscores                  */
/*      Classless Functions : Mixed case with no underscores                  */
/*      Classes             : All upper case seperated by an underscore       */
/*      Defined Constants   : All upper case seperated by an underscore       */
/*      Macros              : All upper case seperated by an underscore       */
/*      Structs/Types       : All upper case seperated by an underscore       */
/*      Private Variables   : All lower case seperated by an underscore       */
/*      Public Variables    : All lower case seperated by an underscore       */
/*      Local Variables     : All lower case seperated by an underscore       */
/*                                                                            */
/*  Development History :                                                     */
/*  Date      Programer           Description                                 */
/*----------------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                               */
/*                                                                            */
/******************************************************************************/
#include "stdhdr.h"
#include "airframe.h"
#include "debuggr.h"
#include "Simbase.h"
#include "limiters.h"
#include "aircrft.H"
#include "arfrmdat.h"

extern AeroDataSet *aeroDataset;

/********************************************************************/
/*                                                                  */
/* Routine: void AirframeClass::Roll(void)                         */
/*                                                                  */
/* Description:                                                     */
/*    Calculate roll rate based on max available rate and user      */
/*    intput.                                                       */
/*                                                                  */
/* Inputs:                                                          */
/*    None                                                          */
/*                                                                  */
/* Outputs:                                                         */
/*    None                                                          */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
extern bool g_bNewFm;
void AirframeClass::Roll(void)
{
float pscmd, alphaError;
Limiter *limiter = NULL;
	
	if(IsSet(Planted))
		return;
	
	alphaError = 0.0F;

   /*--------------*/
   /* command path */
   /*--------------*/
   pscmd  = Math.Limit((rshape*kr01),-kr01,kr01);
	pscmd = min ( max (pscmd, -kr01), kr01);

	if( ((AircraftClass *)platform)->IsF16() )
	{
		alphaError = (float)(fabs(q*0.5F) + fabs(p))*0.1F*RTD;

		limiter = gLimiterMgr->GetLimiter(RollRateLimiter,vehicleIndex);
		if(limiter)
			pscmd *= limiter->Limit(alpha - alphaError);

		float speedswitch = 250.0f;
		
		if (g_bNewFm)
			speedswitch = 220.0f;

		if(vcas < speedswitch)
			pscmd *= (vcas/speedswitch);

		if(gearPos && g_bNewFm)
			pscmd *= 0.6f;
		else if (gearPos)
		   pscmd *= 0.5f;

		if(IsSet(CATLimiterIII))
		{
			limiter = gLimiterMgr->GetLimiter(CatIIIRollRateLimiter,vehicleIndex);
			if(limiter)
				pscmd = limiter->Limit(pscmd);
		}

		if(fabs(pscmd) < 0.001)
			pscmd = -p;
	}
	else 
	{
		limiter = gLimiterMgr->GetLimiter(RollRateLimiter,vehicleIndex);
		if(limiter)
			pscmd *= limiter->Limit(alpha);
	}

	
	if(!IsSet(Simplified))
	{		
		switch(stallMode)
		{
		case None:
			if(assymetry * platform->platformAngles->cosmu > 0.0F)
				pscmd +=  max((assymetry/weight)* 0.04F, 0.0F)*(nzcgs - 1.0F);
			else
				pscmd +=  min((assymetry/weight)* 0.04F, 0.0F)*(nzcgs - 1.0F);
			break;

		case DeepStall:
			if(platform->platformAngles->cosphi > 0.0F)
				pscmd = platform->platformAngles->sinphi*-5.0F*DTR;
			else
				pscmd = platform->platformAngles->sinphi*5.0F*DTR;

			pscmd += (oscillationTimer * 40.0F*max(0.0F,(0.4F - (float)fabs(r))*2.5F)*DTR*(max(0.0F,loadingFraction - 1.3F)) );
			break;

		case EnteringDeepStall:
			if(platform->platformAngles->cosphi > 0.0F)
				pscmd = platform->platformAngles->sinphi*-40.0F*DTR + rshape*kr01 *0.1F;
			else
				pscmd = platform->platformAngles->sinphi*40.0F*DTR + rshape*kr01 *0.1F;
			break;
		case Spinning:
			pscmd = oscillationTimer*DTR;
			break;
		case FlatSpin:
			pscmd = platform->platformAngles->sinphi*5.0F*DTR;
			break;
		}

	}

   /*---------------------------------*/
   /* closed loop roll response model */
   /*---------------------------------*/

   RollIt(pscmd, SimLibMinorFrameTime);
}

void AirframeClass::RollIt(float pscmd, float dt)
{
   // Limit the command w/ roll limit
   if (maxRoll < aeroDataset[vehicleIndex].inputData[8])
   {
      if (phi > maxRoll)
      {
         pscmd = (maxRoll - phi) * kr01;
      }
      else if (phi < -maxRoll)
      {
         pscmd = (maxRoll - phi) * kr01;
      }

      // Scale cmd based on max delta;
      if (maxRollDelta == 0.0F)
         pscmd = 0.0F;
      else
         pscmd *= 1.0F - startRoll/maxRollDelta;
   }
		
	// JB 010714 mult by the momentum
   pstab  = Math.FLTust(pscmd,tr01 * auxaeroData->rollMomentum,dt,oldr01);
}

float AirframeClass::GetMaxCurrentRollRate()
{
	float maxcurrentrollrate = Math.TwodInterp (alpha, qbar, rollCmd->alpha, rollCmd->qbar, 
		rollCmd->roll, rollCmd->numAlpha, rollCmd->numQbar,
		&curRollAlphaBreak, &curRollQbarBreak);

	return maxcurrentrollrate;
}