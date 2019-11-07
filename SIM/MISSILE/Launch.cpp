#include "stdhdr.h"
#include "missile.h"
#include "otwdrive.h"
#include "sfx.h"
#include "acmi\src\include\acmirec.h"
#include "simveh.h"
#include "team.h"

extern ACMIMissilePositionRecord misPos;

void MissileClass::Launch(void)
{
float ubody, vbody, wbody; /* Body axis forces */
float xbreak;              /* x force needed to begin moving on rail */
float xforce, deltu, deltx;
mlTrig trigAlpha, trigBeta;


   /*--------------------------------*/
   /* pass aircraft state to missile */ 
   /*--------------------------------*/
   SetLaunchData();

	if (!ifd)
		return; // JB 010803

	//MI CTD?
	ShiAssert(FALSE==F4IsBadReadPtr(ifd, sizeof *ifd));

   /*--------------------------------*/
   /* missile velocity during launch */
   /*--------------------------------*/
   mlSinCos (&trigAlpha, alpha*DTR);
   mlSinCos (&trigBeta,  beta*DTR);

   ubody = vt*trigAlpha.cos * trigBeta.cos +
           q*initZLoc - r*initYLoc + (float)ifd->olddu[1];

   vbody = vt*trigBeta.sin + r*initXLoc -
           p*initZLoc;

   wbody = vt*trigAlpha.sin * trigBeta.cos +
           p*initYLoc - q*initXLoc;

   vt    = ubody*ubody + wbody*wbody;

   alpha = (float)atan2(wbody, ubody)*RTD;

   if (vt > 1.0F)
      beta = (float)atan2(vbody,sqrt(vt))*RTD;
   else
      beta = 0.0F;

   vt    = (float)sqrt(vt + vbody*vbody);

   /*----------------------------*/
   /* aero and propulsive forces */
   /*----------------------------*/
   Atmosphere();
   Trigenometry();
   Aerodynamics();
   Engine();

   /*-------------------------------*/
   /* force breakout on launch rail */
   /*-------------------------------*/
   xforce = ifd->xaero + ifd->xprop;
   xbreak = 200.0F/(m0 + mp0);
   if(xforce <= xbreak)
   {
      xforce = 0.0F;
   }

   /*--------------------------------------------*/
   /* increment in forward velocity and position */
   /*--------------------------------------------*/
   deltu = Math.FITust(xforce,SimLibMinorFrameTime,ifd->olddu);
   deltx = Math.FITust(deltu ,SimLibMinorFrameTime,ifd->olddx);

   /*-------------------*/
   /* earth coordinates */
   /*-------------------*/
   xdot =  vt*ifd->geomData.cosgam*ifd->geomData.cossig;
   ydot =  vt*ifd->geomData.cosgam*ifd->geomData.sinsig;
   zdot = -vt*ifd->geomData.singam;
   alt  = -z;

   /*---------------------------------------------------*/
   /* applied body axis accels on missile during launch */
   /*---------------------------------------------------*/
   ifd->nxcgb  = ifd->nxcgb + (xforce - initXLoc*(q*q + r*r) +
              initYLoc*(p*q) + initZLoc*(p*r))/GRAVITY;

   ifd->nycgb  = ifd->nycgb + (initXLoc*(p*q) - initYLoc*(r*r +
            p*p) + initZLoc*(q*r))/GRAVITY;

   ifd->nzcgb  = ifd->nzcgb + (initZLoc*(q*q + p*p) - initXLoc*(r*p) -
            initYLoc*(q*r))/GRAVITY;

   /*-------------------------------*/
   /* applied stability axis accels */
   /*-------------------------------*/
   ifd->nxcgs  = ifd->nxcgb*ifd->geomData.cosalp - ifd->nzcgb*ifd->geomData.sinalp;
   ifd->nycgs  = ifd->nycgb;
   ifd->nzcgs  = ifd->nzcgb*ifd->geomData.cosalp + ifd->nxcgb*ifd->geomData.sinalp;

   /*------------------*/
   /* wind axis accels */
   /*------------------*/
   ifd->nxcgw  =  ifd->nxcgs*ifd->geomData.cosbet + ifd->nycgs*ifd->geomData.sinbet;
   ifd->nycgw  = -ifd->nxcgs*ifd->geomData.sinbet + ifd->nycgs*ifd->geomData.cosbet;
   ifd->nzcgw  =  ifd->nzcgs;

   /*-----------------*/
   /* velocity vector */
   /*-----------------*/
   vtdot  = GRAVITY*(ifd->nxcgw - ifd->geomData.singam);

   ifd->alpdot = q - (p*ifd->geomData.cosalp + r*ifd->geomData.sinalp)*ifd->geomData.tanbet - GRAVITY*(ifd->nzcgw -
            ifd->geomData.cosgam*ifd->geomData.cosmu)/(vt*ifd->geomData.cosbet);

   ifd->betdot = p*ifd->geomData.sinalp - r*ifd->geomData.cosalp + GRAVITY *
            (ifd->nycgw + ifd->geomData.cosgam*ifd->geomData.sinmu)/vt;

   /*--------------------------------*/
   /* test for free flight condition */ 
   /*--------------------------------*/
   if(deltx > 0.6667*inputData->length)
   {
	  
      launchState = InFlight;
	  Init2();

	  // missile launch effect
	  Tpoint pos, vec;
	 
	  pos.x = x;
	  pos.y = y;
	  pos.z = z;

	  
	  // test for drawpointer because we may be calling this
	  // via FindRocketGroundImpact
	  if ( drawPointer )
	  {
		  OTWDriver.AddSfxRequest(
				new SfxClass ( SFX_LIGHT_CLOUD,		// type
				0,
				&pos,					// world pos
				&vec,					// vel vector
				5.3F,					// time to live
				5.0f ) );				// scale
	
			// ACMI Output
			if (gACMIRec.IsRecording() )
			{
				misPos.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
				misPos.data.type = Type();
				misPos.data.uniqueID = ACMIIDTable->Add(Id(),NULL,TeamInfo[GetTeam()]->GetColor());//.num_;
				misPos.data.x = x;
				misPos.data.y = y;
				misPos.data.z = z;
				misPos.data.roll = phi;
				misPos.data.pitch = theta;
				misPos.data.yaw = psi;
// remove				misPos.data.teamColor = TeamInfo[GetTeam()]->GetColor();

				gACMIRec.MissilePositionRecord( &misPos );
			}
	  }

   }
}

void MissileClass::SetLaunchData (void)
{
   vt    = parent->Vt();

   if (parent && parent->IsSim())
   {
      SimBaseClass *ptr = (SimBaseClass*)parent;

	  // Transform position from parent's object space into world space
      x = parent->XPos() + ptr->dmx[0][0]*initXLoc + ptr->dmx[1][0]*initYLoc + ptr->dmx[2][0]*initZLoc;
      y = parent->YPos() + ptr->dmx[0][1]*initXLoc + ptr->dmx[1][1]*initYLoc + ptr->dmx[2][1]*initZLoc;
      z = parent->ZPos() + ptr->dmx[0][2]*initXLoc + ptr->dmx[1][2]*initYLoc + ptr->dmx[2][2]*initZLoc;

      theta = ((SimVehicleClass*)parent)->Pitch() + initEl;
      phi   = ((SimVehicleClass*)parent)->Roll();
      psi   = ((SimVehicleClass*)parent)->Yaw() + initAz;
// M.N. add vt > 200.0F check fixes hovering helicopters firing missiles not going ballistic
	  if ( !parent->OnGround() && vt > 200.0F)
	  {
		  p     = ((SimVehicleClass*)parent)->GetP();
		  q     = ((SimVehicleClass*)parent)->GetQ();
		  r     = ((SimVehicleClass*)parent)->GetR();
		  ifd->nxcgb = ((SimVehicleClass*)parent)->GetNx();
		  ifd->nycgb = ((SimVehicleClass*)parent)->GetNy();
		  ifd->nzcgb = ((SimVehicleClass*)parent)->GetNz();
		  alpha = ((SimVehicleClass*)parent)->GetAlpha();
		  beta  = ((SimVehicleClass*)parent)->GetBeta();
	
		  if (parent->Kias() < 250.0F)
		  {
			 alpha *= parent->Kias() / 250.0F;
			 q *= parent->Kias() / 250.0F;
		  }
	  }
	  else
	  {
		  // edg: give ground launched missiles some extra oomph at
		  // launch since this seems to cause problems
		  vt = 200.0f;
		  p     = 0.0F;
		  q     = 0.0F;
		  r     = 0.0F;
		  ifd->nxcgb = 0.0F;
		  ifd->nycgb = 0.0F;
		  ifd->nzcgb = 0.0F;
		  alpha = 0.0F;
		  beta  = 0.0F;
	  }
   }
   else
   {
      // Use parent's world space position corrected for terrain height
      x = parent->XPos();
      y = parent->YPos();
      z = parent->ZPos() + OTWDriver.GetGroundLevel( x, y );

      theta = initEl;
      phi   = 0.0F;
      psi   = initAz;
      p     = 0.0F;
      q     = 0.0F;
      r     = 0.0F;
      ifd->nxcgb = 0.0F;
      ifd->nycgb = 0.0F;
      ifd->nzcgb = 0.0F;
      alpha = 0.0F;
      beta  = 0.0F;
   }
}
