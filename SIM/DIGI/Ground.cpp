#include "stdhdr.h"
#include "simveh.h"
#include "digi.h"
#include "otwdrive.h"
#include "simbase.h"
#include "airframe.h"
#include "Aircrft.h"
#include "Graphics\Include\tmap.h"

#include "limiters.h"

#define MIN_ALT 1500.0F //me123 from 1500

extern float g_MaximumTheaterAltitude;
extern float g_fPullupTime;
extern bool g_bOtherGroundCheck; // = OldGroundCheck function

// JB 011023 Rewritten -- use complex ground checking for all AI
// JB 020313 Rewritten again
void DigitalBrain::GroundCheck(void)
{
float turnRadius, num, alt;
float groundAlt;
float minAlt = 0;


if (g_bOtherGroundCheck)
{
	float denom;
   // Let it follow waypoints close to the ground
   if ( ((//curMode == WaypointMode ||	// also perform ground checks for leaders
//         curMode == LandingMode ||	// removed this, check in LandingMode because of hilly terrain around airbases
		 (curMode == WaypointMode && agDoctrine != AGD_NONE) || // 2002-03-11 ADDED BY S.G. GroundAttackMode has its own ground avoidance code so don't do it here.
		 curMode == TakeoffMode )
		 && threatPtr == NULL) || self->OnGround())
   {
	   // edg: do we really ever need to do ground avoidance if we're in
	   // waypoint mode!?  The waypoint code should be smart enough....
      // minAlt = 500.0F;
      groundAvoidNeeded = FALSE;
      ResetMaxRoll();
      SetMaxRollDelta (100.0F);
      return;
   }
/*   else if(((AircraftClass*) self)->af->GetSimpleMode())
   {
   		// Look ahead 1/2 second
   		groundAlt = -TheMap.GetMEA(self->XPos(), self->YPos());
		if ( self->ZPos() - groundAlt > -100.0f )
		{
			groundAlt = OTWDriver.GetGroundLevel( self->XPos(), self->YPos() );
			if ( self->ZPos() - groundAlt > -100.0f )
			{
				groundAvoidNeeded = TRUE;
      		SetMaxRollDelta (100.0F);
			}
			else
			{
      			groundAvoidNeeded = FALSE;
      			ResetMaxRoll();
      			SetMaxRollDelta (100.0F);
			}
		}
		else
		{
      		groundAvoidNeeded = FALSE;
      		ResetMaxRoll();
      		SetMaxRollDelta (100.0F);
		}
		return;
   }
   else*/	// Still do full ground avoid check for all 
   // 2002-03-11 ADDED BY S.G. If in WaypointMode or WingyMode, drop the min altitude before pullup to 500 feet AGL or -trackZ, whichever is smaller but never below 100.0f AGL
   if (curMode == WaypointMode || curMode == WingyMode)
	   minAlt = (trackZ > -500.0f ? (trackZ > -100.0f ? 100.0f : -trackZ) : 500.0f);
   else
   // END OF ADDED SECTION 2002-03-11
	   minAlt = MIN_ALT;

   // Allow full roll freedom
   ResetMaxRoll();
   SetMaxRollDelta (100.0F);

   /*----------------------------------------------------------------*/
   /* If gamma is positive ground avoidance is not needed regardless */
   /* of altitude.                                                   */
   /*----------------------------------------------------------------*/
   if ((groundAvoidNeeded && self->platformAngles->singam >= 10.0 * DTR) ||
 // 2000-09-30 MODIFIED BY S.G. ZPos IS NEGATIVE! CHECK FOR -10000, NOT 10000!
//     (!groundAvoidNeeded && (self->platformAngles->singam >= 0.0 || self->ZPos() > 10000.0F)))
       (!groundAvoidNeeded && (self->platformAngles->singam >= 0.3 || self->ZPos() < g_MaximumTheaterAltitude))) // g_MaximumTheaterAltitude is negative
   {
      groundAvoidNeeded = FALSE;
      return;
   }

   // Look ahead 1/2 second
// 2000-09-29 MODIFIED BY S.G. WE WILL REALLY LOOK 1 SEC AHEAD NOT OUR CURRENT POSITION!
//   groundAlt = TheMap.GetMEA(self->XPos(), self->YPos());
		// JB 010709 Look 2 ahead
   groundAlt = TheMap.GetMEA( self->XPos() + self->XDelta() * 2, self->YPos() + self->YDelta() * 2);
   if ( self->ZPos() > -groundAlt - minAlt )
   {
// 2000-09-29 MODIFIED BY S.G. WE WILL REALLY LOOK 1 SEC AHEAD NOT OUR CURRENT POSITION!
//	   groundAlt = -OTWDriver.GetGroundLevel( self->XPos(), self->YPos() );
	   groundAlt = -OTWDriver.GetGroundLevel( self->XPos() + self->XDelta() * 2, self->YPos() + self->YDelta() * 2);
   }

   /*--------------------------------*/
   /* Find current state turn radius */
   /*--------------------------------*/
   gaGs = af->SustainedGs(TRUE);
   gaGs = min(maxGs, gaGs);
   num = gaGs*gaGs*0.85F*0.85F - 1.0F;

   if (num < 0.0)
      denom = 0.1F;
   else
      denom = GRAVITY * (float)sqrt(num);

   // turn Radius, corrected for current gamma (i.e. max at -90.0, min at 0.0
   turnRadius = self->Vt()*self->Vt() / denom;
//   turnRadius *= -self->platformAngles->singam;

   // Modify based on roll. if right side up, no change
//   if (fabs (self->Roll()) > 90.0F * DTR)
//   {
//      turnRadius *= 1.0F - self->platformAngles->cosphi*self->platformAngles->cosgam;
//   }

   // Keep the turn radius above the hard deck
   alt = -(self->ZPos())- groundAlt - minAlt;
	 float pullupmult = alt / (-minAlt * 0.5);

 // 2000-08-27 MODIFIED BY S.G. SO AI CAN BE MORE AGRESSIVE WHEN FIGHTING (NOT AS SCARED OF THE GROUND)
// if (alt > turnRadius)
   float a = 100000.0f;
   //  Original            we are going up already                 
   if ( 
		self->ZPos() < -groundAlt && ( // JB 010709 AI crash into the ground when following a target
		(alt > turnRadius || self->ZDelta() <= 0.0f) || 
			(self->ZDelta() > 0.0f && //we are going down  
				(
				-self->ZPos() + (self->ZDelta() *2)	//1 SEC AHEAD
				)	
				 - groundAlt > 500.0f && //our altitude AGL is above 500 ft  
				(
					(self->ZPos()- groundAlt) * (self->ZPos()- groundAlt) / a > self->ZDelta()
				)
			)
			)
		)//our altitude above the deck squared / 100000 is above (10 ft/sec at 1000 ft)
   {
      groundAvoidNeeded = FALSE;
      ResetMaxRoll();
      SetMaxRollDelta (100.0F);
   }
   else
   {
      num = alt / turnRadius;
      /*------------------------------------*/
      /* find roll limit and pitch altitude */
      /*------------------------------------*/
      if (alt > 0.0F)
         gaRoll = 120.0F - (float)atan2(sqrt(1-num*num), num ) * RTD;
      else
         gaRoll = 45.0F;

	  if ( gaRoll < 0.0f )
	  	gaRoll = 0.0f;

//MonoPrint ("Alt %8.2f turn %8.2f roll %8.2f\n", alt, turnRadius, gaRoll);

      groundAvoidNeeded = TRUE;
   }

// 2002-03-01 added by MN
	if (groundAvoidNeeded)
		pullupTimer = SimLibElapsedTime + ((unsigned long)(g_fPullupTime * pullupmult * CampaignSeconds)); // configureable for how long we pull at least once entered groundAvoidNeeded mode


}
else
{


// 2001-10-21 Modified by M.N. When Leader is in WaypointMode, Wings are always in Wingymode.
// so they do terrain based ground checks for the wingies, but not for leads - makes no sense.
   // Let it follow waypoints close to the ground
   if ( //( curMode == WaypointMode ||		// also perform GroundCheck for leaders
         //curMode == LandingMode ||		// airbases with hilly terrain around need GroundCheck
		 (curMode == WaypointMode && agDoctrine != AGD_NONE) || // 2002-03-11 ADDED BY S.G. GroundAttackMode has its own ground avoidance code
		 curMode == TakeoffMode //)
		 && threatPtr == NULL)
   {
	   // edg: do we really ever need to do ground avoidance if we're in
	   // waypoint mode!?  The waypoint code should be smart enough....
      // minAlt = 500.0F;
      groundAvoidNeeded = FALSE;
      ResetMaxRoll();
      SetMaxRollDelta (100.0F);
      return;
   }

   // 2002-03-11 ADDED BY S.G. If in WaypointMode or WingyMode, drop the min altitude before pullup to 500 feet AGL or -trackZ, whichever is smaller but never below 100.0f AGL
   if (curMode == WaypointMode || curMode == WingyMode)
	   minAlt = (trackZ > -500.0f ? (trackZ > -100.0f ? 100.0f : -trackZ) : 500.0f);
   else
   // END OF ADDED SECTION 2002-03-11
	   minAlt = MIN_ALT;

   // Allow full roll freedom
   ResetMaxRoll();
   SetMaxRollDelta (100.0F);

   /*----------------------------------------------------------------*/
   /* If gamma is positive ground avoidance is not needed regardless */
   /* of altitude.                                                   */
   /*----------------------------------------------------------------*/
   if (self->ZPos() < g_MaximumTheaterAltitude) // changed for theater compatibility
   {
      groundAvoidNeeded = FALSE;
      return;
   }

	 // Calculate the time it takes to level the wings.
	 float maxcurrentrollrate = af->GetMaxCurrentRollRate();
	 float timetorolllevel = 1.0F;
	 if (maxcurrentrollrate > 1.0F)
		 timetorolllevel = fabs(self->Roll() * RTD) / maxcurrentrollrate;

	 // Sanity check
	 timetorolllevel = min(timetorolllevel, 60.0F);

   /*--------------------------------*/
   /* Find current state turn radius */
   /*--------------------------------*/
   gaGs = max(af->SustainedGs(TRUE), 2.0F);
   gaGs = min(maxGs, gaGs);

   // turn Radius, corrected for current gamma (i.e. max at -90.0, min at 0.0
   turnRadius = self->Vt()*self->Vt() / (GRAVITY * (gaGs - 1.75F));

	 // turn Radius needs to take into account the time it takes to roll level so we can start pulling.
	 turnRadius += timetorolllevel * self->Vt();

	 float lookahead = max(2.0F, turnRadius / self->Vt() + 1);
	 if (lookahead < 0)
		lookahead = -lookahead;

	 // Sanity check
	 lookahead = min(lookahead, 120.0F);

	 bool groundavoid = false;
	 float pitchadjust;
	 // At what angle are we headed to the ground.  Take that into account so we can try to project a recovery path.
	 float pitchturnfactor = max(0.0, sin(-self->Pitch()) * turnRadius);
	 // If we need to guess at a recovery path, adjust for different delta leg sizes.
	 float directionalfactor = fabs(self->XDelta() / self->YDelta());
	 float xsign = self->XDelta() > 0.0 ? 1 : -1;
	 float ysign = self->YDelta() > 0.0 ? 1 : -1;

	 // Are we even close to hitting the ground?
   groundAlt = TheMap.GetMEA( self->XPos() + self->XDelta(), self->YPos() + self->YDelta());
   if ( self->ZPos() > -groundAlt - max(turnRadius * 2, minAlt) )
   {
	   groundAlt = -OTWDriver.GetGroundLevel( self->XPos(), self->YPos());

		 // Test the terrain along the likely path we will take.  Take into account the deltas along the ground
		 // and the fact that we may be headed straight down so the deltas could be very small.  Since we still
		 // need to test the terrain in case we're diving into a valley, test the terrain along the recovery path.
		 for (float i = 0.0; i < lookahead && !groundavoid; i += 0.5)
		 {
			 // Project a recovery path based on our pitch.
			 pitchadjust = i / lookahead * pitchturnfactor;

			 groundAlt = max(groundAlt, 
				-OTWDriver.GetGroundLevel(self->XPos() + i * self->XDelta() + xsign * pitchadjust * directionalfactor, 
					self->YPos() + i * self->YDelta() + ysign * pitchadjust / directionalfactor));
		
			  // 2002-04-17 MN Start ground avoid check only if ground distances is below a threshold
			 if (-self->ZPos() - groundAlt < self->af->GetStartGroundAvoidCheck())
			 {
				 // Check to be sure we're not going to hit the ground in the next time frame and double check that our worst case
				 // recovery path along the ground to recover from high pitch angles won't take us into the ground either.
				 if (-self->ZPos() - (i * self->ZDelta()) < groundAlt + 100.0F || 
					-self->ZPos() - pitchturnfactor * 1.25 < groundAlt + 100.0F)
				 {
					 groundavoid = true;
				 }
			 }
		 }
   }

   if (self->Vt() < 0.1f || !groundavoid)
   {
      groundAvoidNeeded = FALSE;
      ResetMaxRoll();
      SetMaxRollDelta (100.0F);
   }
   else
   {
		  // Keep the turn radius above the hard deck
		  alt = -(self->ZPos())- groundAlt - minAlt;
      
			num = alt / turnRadius;
      /*------------------------------------*/
      /* find roll limit and pitch altitude */
      /*------------------------------------*/
      if (alt > 0.0F)
         gaRoll = 120.0F - (float)atan2(sqrt(1-num*num), num ) * RTD;
      else
         gaRoll = 45.0F;

			if ( gaRoll < 0.0f )
	  		gaRoll = 0.0f;

      groundAvoidNeeded = TRUE;
   }

// 2002-02-24 added by MN
	if (groundAvoidNeeded)
		pullupTimer = SimLibElapsedTime + ((unsigned long)(g_fPullupTime * CampaignSeconds)); // configureable for how long we pull at least once entered groundAvoidNeeded mode
}

}

void DigitalBrain::PullUp(void)
{
float fact;
float lastPstick = pStick;
float gaMoo = self->Roll() * RTD;
float cushAlt;
float myAlt;

if (g_bOtherGroundCheck)
{
   /*-----------*/
   /* MACH HOLD */
   /*-----------*/
   MachHold(cornerSpeed,self->Kias(), TRUE);

   /*-----------------------------------------*/
   /* negative gamma decreases the roll limit */
   /*-----------------------------------------*/
   fact = 1.0F - self->platformAngles->singam;
   fact = 1.0F;

   /*---------------------------------------------------------*/
   /* Get roll error to the roll limit. Set RSTICK and PSTICK */
   /*---------------------------------------------------------*/
   if (gaMoo > gaRoll/fact) 
   {
      SetMaxRollDelta (gaRoll/fact - gaMoo);
   }
   else if (gaMoo < -gaRoll/fact) 
   {
      SetMaxRollDelta (-gaRoll/fact - gaMoo);
   }
   else
   {
      if (rStick > 0.0F)
         SetMaxRollDelta (100.0F);
      else
         SetMaxRollDelta (-100.0F);
   }
   SetMaxRoll (min (af->MaxRoll() * 0.99F, gaRoll/fact));

   cushAlt = -OTWDriver.GetGroundLevel(self->XPos(), self->YPos()) + MIN_ALT;
   myAlt = -self->ZPos();
   if ( myAlt > cushAlt )
   {
   		// Pull the gs to avoid the ground
   		SetPstick (max (gaGs, 6.0F), af->MaxGs(), AirframeClass::GCommand);
   		pStick = max (lastPstick, pStick);
   }
   else
   {
	   cushAlt -= MIN_ALT * 0.5f;
	   pStick = 1.0f - ( myAlt - cushAlt )/(MIN_ALT * 0.5f);
	   if ( pStick > 1.0f )
	   		pStick = 1.0f;
   }


	if (pullupTimer < SimLibElapsedTime)
		pullupTimer = 0; // This says us "stop pull up"
}
else
{

   /*-----------*/
   /* MACH HOLD */
   /*-----------*/
   // We really want the smallest turn radius speed.  Try corner - 100.
   MachHold(cornerSpeed - 100, self->Kias(), TRUE);

   /*-----------------------------------------*/
   /* negative gamma decreases the roll limit */
   /*-----------------------------------------*/
   fact = 1.0F - self->platformAngles->singam;
   fact = 1.0F;

   /*---------------------------------------------------------*/
   /* Get roll error to the roll limit. Set RSTICK and PSTICK */
   /*---------------------------------------------------------*/
   if (gaMoo > gaRoll/fact) 
   {
      SetMaxRollDelta (gaRoll/fact - gaMoo);
   }
   else if (gaMoo < -gaRoll/fact) 
   {
      SetMaxRollDelta (-gaRoll/fact - gaMoo);
   }
   else
   {
      if (rStick > 0.0F)
         SetMaxRollDelta (100.0F);
      else
         SetMaxRollDelta (-100.0F);
   }

   cushAlt = -OTWDriver.GetGroundLevel(self->XPos(), self->YPos()) + MIN_ALT * 0.75;
   myAlt = -self->ZPos();
   if ( myAlt > cushAlt )
	    SetMaxRoll (min (af->MaxRoll() * 0.99F, gaRoll/fact));
	 else
	   SetMaxRoll (0.0f);

	// The ground avoidance calculations tell us when we need to pull up NOW.  So pull back NOW.

// 2002-04-17 MN make it aircraft dependent which method of PStick change we want to use
// ( a C-130 doing full P-stick pulls looks ridiculous...)

   if (self->af->LimitPstick())
   {
	   if ( myAlt > cushAlt )
	   {
   			// Pull the gs to avoid the ground
   			SetPstick (max (gaGs, 6.0F), af->MaxGs(), AirframeClass::GCommand);
   			pStick = max (lastPstick, pStick);
	   }
	   else
	   {
			cushAlt -= MIN_ALT * 0.5f;
			pStick = 1.0f - ( myAlt - cushAlt )/(MIN_ALT * 0.5f);
			if ( pStick > 1.0f )
	   			pStick = 1.0f;
	   }
   }
   else
	  pStick = 1.0f;

	if (pullupTimer < SimLibElapsedTime)
		pullupTimer = 0; // This says us "stop pull up"
}
}
