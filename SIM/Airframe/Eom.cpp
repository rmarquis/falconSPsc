#include "stdhdr.h"
#include "airframe.h"
#include "simbase.h"
#include "aircrft.h"
#include "otwdrive.h"
#include "fakerand.h"
#include "Graphics\Include\tmap.h"
#include "Graphics\Include\rviewpnt.h"  // to get ground type
#include "apitypes.h"
#include "PilotInputs.h"
#include "limiters.h"
#include "fack.h"
#include "falcsess.h"
#include "MsgInc\DamageMsg.h"
#include "MsgInc\LandingMessage.h"
#include "campbase.h"
#include "fsound.h"
#include "soundfx.h"
#include "playerop.h"
#include "simio.h"
#include "weather.h"
#include "objectiv.h"
#include "find.h"
#include "atcbrain.h"
#include "Graphics\Include\terrtex.h"
#include "ffeedbk.h"

// OW FIXME: this had to be added after installing the DX8 Beta 1 SDK
#define DIRECTINPUT_VERSION 0x0700
#include "dinput.h"

#include "flight.h"
#include "simdrive.h"
#include "digi.h"
#include "ptdata.h"
#include "dofsnswitches.h"
#include "Graphics\Include\drawbsp.h"
#include "classtbl.h"
#include <crtdbg.h> // JPO debug



extern VU_TIME vuxGameTime;
extern int gPlayerExitMenuShown;
//extern bool g_bHardCoreReal; //me123	MI replaced with g_bRealisticAvionics
extern bool g_bRealisticAvionics;

float gSpeedyGonzales = 1.0f;
static float lastVt = 0.0F;  // Only allows for one player A/C

float GROUND_TOLERANCE = 0.1F;
float ANG_RATE = 3.0F;

void AirframeClass::EquationsOfMotion(float dt)
{
	float xwind, wind, windfraction;
	float netAccel, mag;
   float feedbackData;
	mlTrig trigWind;
	

	if (IsSet(InAir)) {
		groundAnchorX	= x;
		groundAnchorY	= y;
		groundDeltaX	= 0.0f;
		groundDeltaY	= 0.0f;
	}
	else
		groundType = OTWDriver.GetGroundType ( x, y );

	/*--------------------*/
	/* Update Orientation */
	/*--------------------*/
	//if(vt)
	//{
		CalcBodyRates(dt);
		CalcBodyOrientation(dt);
	//}
	Trigenometry();
	/*-------------------*/
	/* velocity equation */
	/*-------------------*/
	xwind = xwaero + xwprop;
	
	vtDot = xwind - GRAVITY*platform->platformAngles->singam;
	
	netAccel = CalculateVt(dt);
	
	/*-------------------*/
	/* earth coordinates */
	/*-------------------*/
	
	if(stallMode != Crashing && stallMode < Spinning)
	{
		mlSinCos(&trigWind, ((WeatherClass*)TheWeather)->WindHeadingAt(&gndNormal));
		windfraction = 1.0f;//me123max(0.0F, min(nzcgs, 1.0F));
		if (platform->IsSetFlag(ON_GROUND))windfraction = 0.0f;

		wind =  ((WeatherClass*)TheWeather)->WindSpeedInFeetPerSecond(&gndNormal) * windfraction;
		xdot =  gSpeedyGonzales * vt*platform->platformAngles->cosgam * 
			platform->platformAngles->cossig + trigWind.cos * wind;
		ydot =  gSpeedyGonzales * vt*platform->platformAngles->cosgam *
			platform->platformAngles->sinsig + trigWind.sin * wind;
	}
	zdot = -gSpeedyGonzales * vt*platform->platformAngles->singam;
	
	/*-----------------*/
	/* Update Position */
	/*-----------------*/
	ShiAssert(!_isnan(xdot));
	ShiAssert(!_isnan(ydot));
	ShiAssert(!_isnan(zdot));

   if (!IsSet(InAir))
   {

		// JB carrier start
 		if (IsSet(AirframeClass::OnObject) && platform->attachedEntity)
		{
			xdot += platform->attachedEntity->XDelta();
			ydot += platform->attachedEntity->YDelta();
			zdot += platform->attachedEntity->ZDelta();
		}
		// JB carrier end

	// Accumulate our delta position relative to our start point on the ground
	groundDeltaX += xdot * dt;
	groundDeltaY += ydot * dt;

	// Compute the world space position using our offset and anchor point
	x = groundAnchorX + groundDeltaX;
	y = groundAnchorY + groundDeltaY;
	z += zdot * dt;
   }
   else
   {
	   x += xdot * dt;
	   y += ydot * dt;
	   z += zdot * dt;
   }
	ShiAssert(!_isnan(x));
	ShiAssert(!_isnan(y));
	ShiAssert(!_isnan(z));
	
	groundZ = OTWDriver.GetGroundLevel(x, y, &gndNormal);
	mag = (float)sqrt(gndNormal.x*gndNormal.x + gndNormal.y*gndNormal.y + gndNormal.z*gndNormal.z);
	gndNormal.x /= mag;
	gndNormal.y /= mag;
	gndNormal.z /= mag;
	
	vRot = (float)sqrt(weight / (1.75 * area * rho));
	
	/*----------------------*/
	/* set flight status
	/*----------------------*/
	if (!IsSet(InAir))
	{
		float gndGmma, relMu;
		
		CalculateGroundPlane(&gndGmma, &relMu);
		
		if (qsom*cnalpha < 0.5F)
		{
			SetFlag (Planted);
			SetGroundPosition(dt, netAccel, gndGmma, relMu);
		}
		else if (qsom*cnalpha > 0.55F)
		{
			ClearFlag (Planted);
			if (-zsaero > GRAVITY && gmma - gndGmma > 0.0F)
			{
			    platform->mFaults->AddTakeOff(SimLibElapsedTime);
				SetFlag(InAir);
				platform->UnSetFlag(ON_GROUND);
				if (platform == SimDriver.playerEntity)
				{
					JoystickStopEffect(JoyRunwayRumble1);
					JoystickStopEffect(JoyRunwayRumble2);
				}
			}
			else
				SetGroundPosition(dt, netAccel, gndGmma, relMu);
		}
		else
			SetGroundPosition(dt, netAccel, gndGmma, relMu);
	}
	else
	{
		CheckGroundImpact(dt);
	}
	
   // Force feedback for ownship
   if (platform == SimDriver.playerEntity && !IsSet(InAir))
   {
      if (vt > 1.0f && fabs (vt-lastVt) > 15.0F)
      {
         lastVt = vt;
         feedbackData = 400000.0F - min ((vt/(250.0F*KNOTS_TO_FTPSEC) * 400000.0F), 390000.0F);
         JoystickPlayEffect(JoyRunwayRumble1, FloatToInt32(feedbackData));
         JoystickPlayEffect(JoyRunwayRumble2, FloatToInt32(feedbackData*1.25F));
      }
      else
      {
         JoystickStopEffect(JoyRunwayRumble1);
         JoystickStopEffect(JoyRunwayRumble2);
      }
   }
}

void AirframeClass::CalcBodyRates(float dt)
{
	float qptchc = 0.0F;
	float turbFact;
	float tempVt, rateMod;
	float alpdelta = 0.0F, pdelta = 0.0F, rdelta = 0.0F;
	float leftMax = -PI, rightMax = PI, frontMax = -PI, backMax = PI;

	if(fabs(vt) > 4.0F)
		tempVt = vt;
	else if(vt < 0.0F)
		tempVt = -4.0F;
	else
		tempVt = 4.0F;
	
	if(!IsSet(InAir))
	{
		//check to see what's in contact with the ground and rotate aircraft appropriately
		float cgloc = GetAeroData(AeroDataSet::CGLoc);
		float length = GetAeroData(AeroDataSet::Length);
		float halfspan = GetAeroData(AeroDataSet::Span)/2.0F;
		float radius = GetAeroData(AeroDataSet::FusRadius);
		float gearHt = GetAeroData(AeroDataSet::NosGearZ) - radius;
		float tailHt = GetAeroData(AeroDataSet::TailHt);
			
		int front = 0;
		int back = 0;
		int left = 0;
		int right = 0;
		int body = 0;
		int allgear = 1;

		
		Tpoint PtWorldPos;
		Tpoint PtRelPos;
		float OldGearExt = 0.0F;
		float GearExt = 0.0F;
		
		float cosphi_lim = max(0.0F,platform->platformAngles->cosphi);

		if(platform->drawPointer)
		{
			PtRelPos.x = 0.0F;
			PtRelPos.y = radius*platform->platformAngles->sinphi;
			PtRelPos.z = radius*platform->platformAngles->cosphi;

			MatrixMult( &((DrawableBSP*)platform->drawPointer)->orientation, &PtRelPos, &PtWorldPos );

			if(z + PtWorldPos.z >= groundZ)
				body = 1;
		}
		else if(z + radius >= groundZ)
			body = 1;

		if(NumGear() > 1 && platform->drawPointer)
		{
			for(int i = 0; i < NumGear(); i++)
			{
			    if (platform->IsComplex())
				GearExt = platform->GetDOFValue(COMP_NOS_GEAR_COMP + i);
			    else GearExt = 0;
				OldGearExt = GearExt;
				if(!(gear[i].flags & GearData::GearBroken))
				{					
					gear[i].vel = gear[i].vel*0.3F - GearExt*0.2F/dt;
					if(fabs(gear[i].vel) < 0.005F)
						gear[i].vel = 0.0F;
					//else
					//	gear[i].vel = min(5.0F, max(-5.0F, gear[i].vel));
					if(fabs(GearExt) < 0.001F && gear[i].vel == 0.0F)
						GearExt = 0.0F;
					else
						GearExt = min(0.5F,max(-0.5F, GearExt + gear[i].vel*dt));
					
					float geardof;
					if(platform->IsComplex()) {
					    platform->SetDOF(COMP_NOS_GEAR_COMP + i, GearExt);
					    geardof = platform->GetDOFValue(COMP_NOS_GEAR + i);
					}
					else {
					    geardof = GetAeroData(AeroDataSet::NosGearRng + i*4)*DTR;
					}

					PtRelPos.x = cgloc - GetAeroData(AeroDataSet::NosGearX + i*4);
					PtRelPos.y = GetAeroData(AeroDataSet::NosGearY + i*4);
					PtRelPos.z = (GetAeroData(AeroDataSet::NosGearZ + i*4) + GearExt - radius)*geardof/(GetAeroData(AeroDataSet::NosGearRng + i*4)*DTR) + radius;

					MatrixMult( &((DrawableBSP*)platform->drawPointer)->orientation, &PtRelPos, &PtWorldPos );

					if(
						IsSet(OnObject) || // JB carrier
						(z + PtWorldPos.z >= groundZ - GROUND_TOLERANCE && GearExt - OldGearExt > -dt ))
					{
						if(geardof/(GetAeroData(AeroDataSet::NosGearRng + i*4)*DTR) < 0.85F)
							allgear = 0;

						if(PtRelPos.x > 0.0F)
							front = 1;
						else if(PtRelPos.x < 0.0F)
							back = 1;

						if(PtRelPos.y > 0.0F)
						{
							right = 1;
						}
						else if(PtRelPos.y < 0.0F)
						{
							left = 1;	
						}
					}
					else
					{
						allgear = 0;
						float temp;
						if(PtRelPos.x*platform->platformAngles->cosphi > 0.0F)
						{
							temp = (float)atan2(z + PtWorldPos.z - groundZ, fabs(PtRelPos.x*platform->platformAngles->costhe));
							frontMax = max(temp, frontMax);
						}
						else if(PtRelPos.x*platform->platformAngles->cosphi < 0.0F && IsSet(Planted))
						{
							temp = (float)atan2(groundZ - z - PtWorldPos.z, fabs(PtRelPos.x*platform->platformAngles->costhe));
							backMax = min(temp, backMax);
						}
						else if(PtRelPos.x > 0.0F)
						{
							temp = (float)atan2(z + PtWorldPos.z - groundZ, fabs(PtRelPos.x*platform->platformAngles->costhe));
							frontMax = max(temp, frontMax);
						}
						else if(PtRelPos.x < 0.0F && IsSet(Planted))
						{
							temp = (float)atan2(groundZ - z - PtWorldPos.z, fabs(PtRelPos.x*platform->platformAngles->costhe));
							backMax = min(temp, backMax);
						}

						if(PtRelPos.y > 0.0F)
						{
							temp = (float)atan2(groundZ - z - PtWorldPos.z, PtRelPos.y*platform->platformAngles->cosphi);
							rightMax = min(temp, rightMax);
						}
						else if(PtRelPos.y < 0.0F)
						{
							temp = (float)atan2(z + PtWorldPos.z - groundZ, -PtRelPos.y*platform->platformAngles->cosphi);
							leftMax = max(temp, leftMax);
						}
					}
				}
			}
		}
		else if(!IsSet(GearBroken) && platform->platformAngles->costhe * (cosphi_lim * (gearHt * gearPos + radius) ) + z > groundZ - GROUND_TOLERANCE)
		{
			if(fabs(platform->platformAngles->sinphi) > 0.001F)
				right = 1;
			else
				left = 1;

			if(platform->platformAngles->sinthe > 0.001F)
				back = 1;
			else if(platform->platformAngles->sinthe < -0.001F)
				front = 1;
		}
		
		float wingHt = platform->platformAngles->costhe * (float)fabs(platform->platformAngles->sinphi) * halfspan;
		if( wingHt + z > groundZ - GROUND_TOLERANCE)
		{
			allgear = 0;
			//wing hit ground
			if(platform->platformAngles->sinphi > 0.0F)
			{
				right = 1;
				rdelta += (0.2F + IsSet(Simplified)*0.2F) * (1.0F - nzcgs)*vt*0.2F*halfspan*dt*DTR;
			}
			else
			{
				left = 1;
				rdelta -= (0.2F + IsSet(Simplified)*0.2F) * (1.0F - nzcgs)*vt*0.2F*halfspan*dt*DTR;
			}

			DragBodypart();
		}		
		else
		{
			float temp;
			if(platform->platformAngles->sinphi > 0.0F)
			{
				temp = (float)atan2(groundZ - z + wingHt, halfspan*platform->platformAngles->cosphi);
				rightMax = min(temp, rightMax);
				temp = (float)atan2( z + wingHt - groundZ, halfspan*platform->platformAngles->cosphi);
				leftMax = max(temp, leftMax);
			}
			else
			{
				temp = (float)atan2(groundZ - z - wingHt, halfspan*platform->platformAngles->cosphi);
				rightMax = min(temp, rightMax);
				temp = (float)atan2( z - wingHt - groundZ, halfspan*platform->platformAngles->cosphi);
				leftMax = max(temp, leftMax);
			}
		}
		
		//tail might hit
		if(platform->drawPointer)
		{
			PtRelPos.x = cgloc - length;
			if(platform->platformAngles->cosphi > -0.03489F)
			{
				PtRelPos.y = radius*0.5F*platform->platformAngles->sinphi;
				PtRelPos.z = radius*0.5F*platform->platformAngles->cosphi;
			}
			else
			{
				PtRelPos.y = 0.0F;
				PtRelPos.z = tailHt;
			}

			MatrixMult( &((DrawableBSP*)platform->drawPointer)->orientation, &PtRelPos, &PtWorldPos );

			if(PtWorldPos.z + z > groundZ - GROUND_TOLERANCE)
			{
				allgear = 0;
				back = 1;
				DragBodypart();
			}
			else
			{
				float temp = (float)atan2(groundZ - z - PtWorldPos.z, -PtRelPos.x*platform->platformAngles->costhe);
				backMax = min(temp, backMax);
			}
		}
		else if(platform->platformAngles->sinthe*platform->platformAngles->cosphi*(length - cgloc) + z > groundZ - GROUND_TOLERANCE)
		{
			allgear = 0;
			back = 1;
			DragBodypart();
		}
		else
		{
			float temp = (float)atan2(groundZ - z - platform->platformAngles->sinthe*platform->platformAngles->cosphi*(length - cgloc), (length - cgloc)*platform->platformAngles->costhe);
			backMax = min(temp, backMax);
		}

		//nose might hit
		if(platform->drawPointer)
		{
			PtRelPos.x = cgloc;
			PtRelPos.y = radius*0.5F*platform->platformAngles->sinphi;
			PtRelPos.z = radius*0.5F*platform->platformAngles->cosphi;

			MatrixMult( &((DrawableBSP*)platform->drawPointer)->orientation, &PtRelPos, &PtWorldPos );

			if(PtWorldPos.z + z > groundZ - GROUND_TOLERANCE)
			{
				allgear = 0;
				front = 1;
				DragBodypart();
			}
			else
			{
				float temp = (float)atan2(z + PtWorldPos.z - groundZ, PtRelPos.x*platform->platformAngles->costhe);
				frontMax = max(temp, frontMax);
			}
		}
		else if(platform->platformAngles->sinthe*platform->platformAngles->cosphi*-cgloc + z > groundZ - GROUND_TOLERANCE)
		{
			allgear = 0;
			front = 1;
			DragBodypart();
		}
		else
		{
			float temp = (float)atan2(groundZ - z - platform->platformAngles->sinthe*platform->platformAngles->cosphi*-cgloc, -cgloc*platform->platformAngles->costhe);
			frontMax = max(temp, frontMax);
		}

		float zsaeroLim = max(-GRAVITY, zsaero*platform->platformAngles->cosmu);

		if(!(front + back) && body)
		{
			if(platform->platformAngles->costhe > 0.0F)
				alpdelta = (GRAVITY + zsaeroLim)*ANG_RATE*dt*dt;
			else
				alpdelta = (GRAVITY + zsaeroLim)*-ANG_RATE*dt*dt;
		}
		else if(allgear && front && back && vt == 0.0F)
		{
			alpha *= 0.5F;
			oldp03[0] = 0.0F;
			oldp03[1] = 0.0F;
			oldp03[2] = 0.0F;
			oldp03[3] = 0.0F;
		}
		else if(body)
			alpdelta = 0.0F;
		else
			alpdelta = (GRAVITY + zsaeroLim)*(front - back)*ANG_RATE*dt*dt;

		if(pstick > 0.0F && alpha > 0.0F)
			frontMax -= alpha*DTR;
		alpdelta = min(backMax*RTD, max(frontMax*RTD, alpdelta));

		if(!alpdelta && pstick > 0.0F && alpha > 0.0F)
			alpdelta = max(-alpha, (GRAVITY + zsaeroLim)*-ANG_RATE*dt*dt);
		alpha += alpdelta;
		oldp03[0] += alpdelta;
		oldp03[1] += alpdelta;

#ifdef DAVE_DBG
//		if(platform == SimDriver.playerEntity)
//			MonoPrint("a: %5.3f ad: %5.3f q: %5.3f FB: %2d P: %1d aoabias: %5.3f\n", alpha, alpdelta*RTD, q*RTD, front - back, IsSet(Planted), aoabias);
#endif
		//oldp03[2] += alpdelta;
		//oldp03[3] += alpdelta;

		aoabias = alpha;

		oldp02[0] = alpha;
		oldp02[1] = alpha;

		if(!(left + right) && body)
		{
			if(platform->platformAngles->sinphi > 0.0F)
				pdelta = (GRAVITY + zsaeroLim)*ANG_RATE*dt*DTR;
			else
				pdelta = (GRAVITY + zsaeroLim)*-ANG_RATE*dt*DTR;
		}
		else if(allgear && left && right && vt == 0.0F)
		{
			pdelta = 0.0F;
			mu *= 0.5F;
		}
		else if(body)
			pdelta = 0.0F;
		else
			pdelta = (GRAVITY + zsaeroLim)*(left - right)*ANG_RATE*dt*DTR;
	}
	/*--------*/
	/* Flying */
	/*--------*/
	if (!IsSet(Planted))
	{

		/*----------------------*/
		/* body axis pitch rate */
		/*----------------------*/
		
		if(gearPos < 1.0F)
			qptchc += (float)(atan(nzcgs*GRAVITY/tempVt) - atan(0.2F*gearPos*qsom/tempVt) + pitch*platform->platformAngles->cosbet);
		else
			qptchc += (float)(atan(nzcgs*GRAVITY/tempVt) - atan(0.1F*gearPos*qsom/tempVt) + pitch*platform->platformAngles->cosbet);
		
		// Bias nose down if going slow 
		if (tempVt < 0.5F* vRot && IsSet(InAir))
		{
			rateMod = 5.0F;
			
			if( tempVt > 0)
				rateMod = min(5.0F, vRot/tempVt );			
			
			if (platform->platformAngles->cosphi > 0.0F)
			{
				if(platform->platformAngles->sinthe > 0.9F)
					qptchc -= rateMod *(3.0F * DTR * (float)fabs(1 - platform->platformAngles->costhe) );
			}
			else
			{
				if(platform->platformAngles->sinthe > 0.9F)
					qptchc += rateMod *(3.0F * DTR * (float)fabs(1 - platform->platformAngles->costhe) );
			}
		}
		
		if(stallMode == EnteringDeepStall)
		{
			pitch *= 0.9F;
			
			if (platform->platformAngles->cosphi > 0.0F)
				qptchc -= 5.0F * DTR;
			else
				qptchc += 5.0F * DTR;
		}
		
		qptchc -= (float)atan(platform->platformAngles->cosmu * platform->platformAngles->cosgam*GRAVITY /tempVt);
		// JB 010714 mult by the elasticity
		q = Math.FLTust(qptchc,tp01 * auxaeroData->pitchElasticity,dt ,oldp05);
		ShiAssert(!_isnan(q));
		//if(!IsSet(InAir))
		//	q = max(0.0F, q);
		
		/*----------------------------------*/
		/* body axis roll rate and yaw rate */
		/*----------------------------------*/
		
		if(stallMode >= DeepStall && alpha < -10.0F)
		{
			slice += (slice*0.007F - ypedal*0.005F + (assymetry/weight)*0.002F);
			slice = max(min(slice,5.0F), -5.0F);
			if(fabs(slice) < 0.0015F)
				slice = 0.0F;
			
			p = pstab;
			
			if(stallMode >= Spinning)
			{
				if(stallMode == FlatSpin)
				{
					r = 0;
					p = 0;
					q = 0;
				}
				else
				{
					rstab  = (nycgw  + platform->platformAngles->cosgam * 
					platform->platformAngles->sinmu)*GRAVITY/tempVt;
				
					r = rstab;
				}

				gmma -= (90.0F*DTR + gmma)*dt/5.0F;
				ResetOrientation();
			}
			else
			{
				rstab  = (nycgw  + platform->platformAngles->cosgam * 
					platform->platformAngles->sinmu)*GRAVITY/tempVt;
				
				r = rstab;
			}
		}
		else
		{
			slice *= 0.97F;
			pitch *= 0.97F;
			
			rstab  = (nycgw  + platform->platformAngles->cosgam *
				platform->platformAngles->sinmu)*GRAVITY/tempVt;

			// JPO - add in experimental roll couple
			if (IsSet(InAir))
			    pstab += auxaeroData->rollCouple * fabs(ypedal) * ypedal;

			r = rstab;
			p = pstab;
		}
		
		// Bias nose down if going slow 
		if (tempVt < 0.5F* vRot && IsSet(InAir))
		{
			rateMod = 5.0F;
			
			if( tempVt > 0)
				rateMod = min(5.0F, vRot/tempVt );			
			
			if(platform->platformAngles->sinthe > 0.9F)
				r += rateMod *platform->platformAngles->sinphi*(3.0F * DTR * (float)fabs(1 - platform->platformAngles->costhe) );
		}

		r += slice*platform->platformAngles->cosalp;
		p += (slice*platform->platformAngles->sinalp + pitch*platform->platformAngles->sinalp);

		CalcGroundTurnRate(dt);
		
		if (!IsSet(IsDigital))
		{//me123 this is disabled distance is always returned as -5000
			//this is effection stic input and acts very wired.
			turbFact = (OTWDriver.DistanceFromCloudEdge() + 5000.0F) / 5000.0F;

			if (turbFact > 0.0F)
			{
				p *= PRANDFloat() * turbFact;
				q *= PRANDFloat() * turbFact;
				r *= PRANDFloat() * turbFact;
			}
		}
		
		ShiAssert(!_isnan(r));
/* REMOVED BY S.G. THIS CODE IS PART OF THE CLUPRIT WHY HOOKING TO THE TANKER DOESN'T WORK		
		
		// If on a boom, damp the rates
		if (IsSet(Refueling))
		{
			float rateDeadband = 0.05F*PlayerOptions.GetRefuelingMode();
			
			// Try to follow the tanker, unless rates are to high
			if (q > rateDeadband)
				q -= rateDeadband;
			else if (q < -rateDeadband)
				q += rateDeadband;
			else
				q = -gmma*0.75F;
			
			if (p > rateDeadband)
				p -= rateDeadband;
			else if (p < -rateDeadband)
				p += rateDeadband;
			else
				p = -phi*0.75F;
			
			if (r > rateDeadband)
				r -= rateDeadband;
			else if (r < -rateDeadband)
				r += rateDeadband;
			else
				r = (forcedHeading - sigma)*0.75F;
		}
*/	}
	else
	/*-----------------*/
	/* Ground Handling */
	/*-----------------*/
	{		
		p = 0.0F;
		//mu = 0.0F;
		
		CalcGroundTurnRate(dt);
		
		q = 0.0F;
		
		oldp05[0] = 0.0;
		oldp05[1] = 0.0;
		oldp05[2] = 0.0;
		oldp05[3] = 0.0;
	}

	if(IsSet(InAir))
		p += pdelta;
	else
	{
		p = min(rightMax*2.0F, max(leftMax*2.0F, p + pdelta));
		if(p == rightMax*2.0F || p == leftMax*2.0F)
		{
			//if we had to limit the roll it means we should stop rolling so zero integrator
			oldr01[0] = 0.0F;
			oldr01[1] = 0.0F;
			oldr01[2] = 0.0F;
			oldr01[3] = 0.0F;
		}
	}

	//r += rdelta;

	//if we rotate too fast the quaternions go nutty
	p = min(4.5F,max(p, -4.5F));
	q = min(3.0F,max(q, -3.0F));
	r = min(4.0F,max(r, -4.0F));

	startRoll += p * SimLibMinorFrameTime;
}

void AirframeClass::CalcBodyOrientation(float dt)
{
	float e1dot, e2dot, e3dot, e4dot;
	float enorm;
	float e1temp, e2temp, e3temp, e4temp;
	
	/*-----------------------------------*/
	/* quaternion differential equations */
	/*-----------------------------------*/
	e1dot = (-e4*p - e3*q - e2*r)*0.5F;
	e2dot = (-e3*p + e4*q + e1*r)*0.5F;
	e3dot = ( e2*p + e1*q - e4*r)*0.5F;
	e4dot = ( e1*p - e2*q + e3*r)*0.5F;
	
	/*-----------------------*/
	/* integrate quaternions */
	/*-----------------------*/
	e1temp = e1 + e1dot*dt;
	e2temp = e2 + e2dot*dt;
	e3temp = e3 + e3dot*dt;
	e4temp = e4 + e4dot*dt;
	
	/*--------------------------*/
	/* quaternion normalization */
	/*--------------------------*/
	
	/* It's not strictly legal for the */
	/* compiler to do this for itself. */     
	
	enorm = (float)(1.0/sqrt(e1temp*e1temp + e2temp*e2temp +
		e3temp*e3temp + e4temp*e4temp));
	ShiAssert(!_isnan(enorm));
	e1    = e1temp*enorm;
	e2    = e2temp*enorm;
	e3    = e3temp*enorm;
	e4    = e4temp*enorm;
	
	/*--------------*/
	/* euler angles */
	/*--------------*/
	
	sigma   =  (float)atan2(2.0F*(e3*e4 + e1*e2), e1*e1 - e2*e2 - e3*e3 + e4*e4);
	gmma = -(float)atan2(2.0F*(e2*e4 - e1*e3), (float)sqrt(1.0-2.0F*(e2*e4 - e1*e3)*2.0F*(e2*e4 - e1*e3)));
	mu   =  (float)atan2(2.0F*(e2*e3 + e4*e1), e1*e1 + e2*e2 - e3*e3 - e4*e4);	
	ShiAssert(!_isnan(sigma));
	ShiAssert(!_isnan(gmma));
	ShiAssert(!_isnan(mu));
}

void AirframeClass::CalcGroundTurnRate(float dt)
{
	float rCom, rMax, Mu_fric = 0.0F;

	if(IsSet(InAir) || platform->mFaults->GetFault(nws_fault))	//MI added faults check
		return;

	if(g_bRealisticAvionics && platform->Pitch() * RTD > 3 && IsSet(NoseSteerOn))
		ClearFlag(NoseSteerOn);

	if(gearPos >= 0.9F && !IsSet(GearBroken) && vt > 0.0F )
	{
		if(IsSet(NoseSteerOn) && !(gear[0].flags & GearData::GearStuck) 
			&& platform->OnGround())
		{
			//MI need to filter Trim out here
			float YPedal = ypedal - UserStickInputs.ytrim;
			float RStick = rstick - UserStickInputs.rtrim;

			if(IO.AnalogIsUsed(3) && !IsSet(IsDigital))
				//rCom =  vt/(13.167F/(float)sin(-ypedal * fabs(yshape) * 0.55856F));
				rCom =  vt/(13.167F/(float)sin(-YPedal * fabs(yshape) * 0.55856F));
			else
				//rCom =  vt/(13.167F/(float)sin(rstick * fabs(rshape) * 0.55856F));
				rCom =  vt/(13.167F/(float)sin(RStick * fabs(rshape) * 0.55856F));

			//rCom *= (0.5F + (80.0F*KNOTS_TO_FTPSEC - vt)/(160.0F * KNOTS_TO_FTPSEC));
			rCom *= max(0.01F, (0.5F + (80.0F*KNOTS_TO_FTPSEC - vt)/(160.0F * KNOTS_TO_FTPSEC))); // JB 010805 Reverse steers over 160 knots.

			Mu_fric += (0.6F - 0.3F*(!platform->onFlatFeature && groundType != COVERAGE_ROAD)  );

			if(!IsSet(OverRunway) )
			{
				Mu_fric -= 0.2F;
			}	

			rMax = Mu_fric * (1.0F - nzcgs) * GRAVITY / vt;

			if( fabs(rCom) > rMax)
			{
				if(rCom < 0.0F)
				{
					r = -rMax;
					beta -= (rCom + rMax)*RTD*dt*0.1F;
				}
				else
				{
					r = rMax;
					beta -= (rCom - rMax)*RTD*dt*0.1F;
				}
				beta = max(-45.0F, min(beta, 45.0F));
				beta *= 0.9F;
			}
			else
			{
				beta *= 0.6F;
				if(fabs(beta) < 0.1F)
					beta = 0.0F;
				r = rCom;
			}
		}
		else
		{
			if(IO.AnalogIsUsed(3) && !IsSet(IsDigital))
				r =  max(-0.5F, min (ypedal * (float)fabs(yshape)*wy01*cy*qsom * 0.5F, 0.5F));
			else
				r =  max(-0.5F, min (rstick * (float)fabs(rshape)*wy01*cy*qsom * 0.5F, 0.5F));	
			
			beta += rstick * (float)fabs(rshape)*wy01*cy*qsom*0.5F*RTD*dt;
			beta *= 0.9F;
			beta = max(-15.0F, min(beta, 15.0F));
		}
	}
	else
	{
		r = 0.0F;
		beta *= 0.8F;
	}

	float slip = (float)fabs(vt *platform->platformAngles->sinbet);

	if(slip > 3.0F && vt > 25.0F * KNOTS_TO_FTPSEC && platform == SimDriver.playerEntity &&
		(IsSet(OverRunway) || platform->onFlatFeature || groundType == COVERAGE_ROAD) )
	{
		float volume = max(0.0F, 2500.0F - slip*slip*100.0F);
		F4SoundFXSetPos( SFX_TIRE_SQUEAL, TRUE, x + 5.0F, y, z, 1.0F, volume );
	}

	oldy03[0] = beta;
	oldy03[1] = beta;
	oldy03[2] = beta;
	oldy03[3] = beta;
	oldy03[4] = beta;
	oldy03[5] = beta;
}

void AirframeClass::ResetOrientation(void)
{
	mlTrig trigGam, trigSig, trigMu;
	
	mlSinCos (&trigGam,	gmma	* 0.5F);
	mlSinCos (&trigSig,	sigma	* 0.5F);
	mlSinCos (&trigMu,	mu		* 0.5F);
	
	e1 =	trigSig.cos*trigGam.cos*trigMu.cos +
			trigSig.sin*trigGam.sin*trigMu.sin;
	
	e2 =	trigSig.sin*trigGam.cos*trigMu.cos -
			trigSig.cos*trigGam.sin*trigMu.sin;
	
	e3 =	trigSig.cos*trigGam.sin*trigMu.cos +
			trigSig.sin*trigGam.cos*trigMu.sin;
	
	e4 =	trigSig.cos*trigGam.cos*trigMu.sin -
			trigSig.sin*trigGam.sin*trigMu.cos;
}

float AirframeClass::CalcMuFric(int groundType)
{
	float Mu_fric;
	if(IsSet(GearBroken) || gearPos <= 0.3F || platform->platformAngles->cosphi < 0.9659F )
	{
		Mu_fric = (0.6F + 0.3F * (!platform->onFlatFeature && groundType != COVERAGE_ROAD) + 0.1F*IsSet(OverRunway));
	}
	else
	{
		//MI check for parking brake
		if(PBON)			
			SetFlag(WheelBrakes);
		//if we've got more then 87% RPM, PB get's unset
		if(PBON && (rpm >= 0.87))
			TogglePB();

		Mu_fric = 0.0F;
		float wheelbrakes;
		if(NumGear() > 1)
		{
			wheelbrakes = IsSet(WheelBrakes)*( (!(gear[1].flags & GearData::GearBroken) && TRUE) + (!(gear[2].flags & GearData::GearBroken) && TRUE) )*0.5F;
		}
		else
			wheelbrakes = (float)IsSet(WheelBrakes);

		if(!IsSet(OverRunway) )
			Mu_fric += 0.04F - 0.1F *wheelbrakes;

		if(IsSet(OnObject) && IsSet(Hook)) // JB carrier
		{
			if (vt < 40.0F * KNOTS_TO_FTPSEC)
			{
				alpha = 0.0F;
				beta = 0.0F;
				mu = 0.0F;
			
				ResetOrientation();
				Trigenometry();
			}

			Mu_fric = 20; // JB carrier
		}

		if(vt <= 0.1F)
			Mu_fric += (0.06F + 0.4F * platform->platformAngles->sinbet + (0.44F + 0.2F*IsSet(OverAirStrip)) *wheelbrakes + (!platform->onFlatFeature && groundType != COVERAGE_ROAD)*(0.4F - 0.1F *IsSet(WheelBrakes)) );
		else
			Mu_fric += (0.04F + 0.5F * platform->platformAngles->sinbet + (0.36F + 0.2F*IsSet(OverAirStrip))*wheelbrakes + (!platform->onFlatFeature && groundType != COVERAGE_ROAD)*(0.4F - 0.1F *IsSet(WheelBrakes)) );

		if(platform->AutopilotType() == AircraftClass::CombatAP)
			Mu_fric += 0.4F *IsSet(WheelBrakes);
	}
	return Mu_fric;
}

float AirframeClass::CalculateVt(float dt)
{
	float netAccel = 0.0F;
	float newVt, Mu_fric = 0;
	float pitch, volume;
	
	GridIndex X,Y;
				
	X = SimToGrid(y);
	Y = SimToGrid(x);
	
	Objective airbase = FindNearbyAirbase (X, Y);
	if(
		IsSet(OnObject) || // JB carrier
		(airbase && 
		airbase->IsObjective() && // JB 060114 CTD
		airbase->brain->IsOverRunway(platform) ))
	{
		SetFlag(OverRunway);
	}
	else
		ClearFlag(OverRunway);

	if(airbase && airbase->GetType() == TYPE_AIRSTRIP)
		SetFlag(OverAirStrip);
	else
		ClearFlag(OverAirStrip);

	if (vt > 10.0F)
		gPlayerExitMenuShown = TRUE;

	if (!IsSet(InAir))
	{
		mlTrig Trig;
		mlSinCos(&Trig, (float)SimLibElapsedTime*vt/100000.0f);
		oscillationTimer = Trig.sin;
		oscillationSlope = Trig.cos;
		//MI
		//if (IsSet(IsDigital) || !g_bHardCoreReal)//me123 check addet, humans are doing it with missile step now
		if(IsSet(IsDigital) || !g_bRealisticAvionics)
		{
		if(vt < 80.0F * KNOTS_TO_FTPSEC && theta < 1.0F * DTR  )
			SetFlag(NoseSteerOn);
		else
			ClearFlag(NoseSteerOn);
		}
		//from Raymer: Aircraft Design mu = 0.04 w/brakes mu = 0.4
		if(nzcgs < 1.0F)
		{
			FalconDamageMessage* message;
			
			if ( ( groundType == COVERAGE_WATER || groundType == COVERAGE_RIVER ) && vt < 5.0F
				&& !IsSet(OnObject)) // JB carrier
			{
				message = CreateGroundCollisionMessage(platform, FloatToInt32(platform->MaxStrength()));			
				FalconSendMessage (message,TRUE);
			}
			else if(
				!IsSet(OnObject) && // JB carrier
				(IsSet(GearBroken) || gearPos <= 0.3F || platform->platformAngles->cosphi < 0.9659F ||
				groundType == COVERAGE_WATER || groundType == COVERAGE_RIVER))
			{
				int dmgStrength;
				
				if (	
					!IsSet(OnObject) && ( // JB carrier
					groundType == COVERAGE_WATER		|| groundType == COVERAGE_RIVER || groundType == COVERAGE_THINFOREST || 
					groundType == COVERAGE_THICKFOREST	|| groundType == COVERAGE_ROCKY	|| groundType == COVERAGE_URBAN)
					) // JB carrier
				{
					dmgStrength = FloatToInt32(max(0.0F,vt * 0.02F*(1.0F - nzcgs)*(float)rand()/(float)RAND_MAX));
				}
				else
					dmgStrength = FloatToInt32(max(0.0F,vt * 0.01F*(1.0F - nzcgs)*(float)rand()/(float)RAND_MAX));
				
				if(
					!IsSet(OnObject) && ( // JB carrier
					groundType == COVERAGE_WATER || groundType == COVERAGE_RIVER)
					) // JB carrier
				{
					for(int i = 0; i < NumGear(); i++)
					{
					    if (platform->IsComplex())
						platform->SetDOF(COMP_NOS_GEAR + i, 0.0F);
					    gear[i].flags |= GearData::GearBroken | GearData::DoorBroken;
					}
					SetFlag (GearBroken);
				}

				if(dmgStrength)
				{
					message = CreateGroundCollisionMessage(platform, dmgStrength);		
					FalconSendMessage (message,FALSE);
				}

				if(dmgStrength > 1)
				{
					pitch = max(0.5F, min(100.0F/vt, 2.0F));
					volume = max(0.0F, min(40000.0F - vt*vt, 4000000.0F));
					F4SoundFXSetPos( SFX_HIT_2 + rand()%4, TRUE, x + 5.0F, y, z, pitch, volume );
					bumpthe += (1.0F - 2.0F*(float)rand()/(float)RAND_MAX) * dmgStrength * vt/400.0F;
					bumpphi += (1.0F - 2.0F*(float)rand()/(float)RAND_MAX) * dmgStrength * vt/200.0F *DTR;
					bumpyaw += (1.0F - 2.0F*(float)rand()/(float)RAND_MAX) * dmgStrength * vt/200.0F *DTR;
				}
				
				if (!IsSet(OnObject)) // JB carrier
				{ // JB carrier
					SetFlag(EngineOff);
					platform->mFaults->SetFault(FaultClass::eng_fault,
									FaultClass::fl_out, FaultClass::fail, FALSE);
				} // JB carrier
				
				if(vt > 1.0F)
				{
					pitch = max(0.2F, min(vt/200.0F, 2.0F));
					volume = max(0.0F, min(2500.0F - vt*vt, 4000000.0F));
					if(
						!IsSet(OnObject) && // JB carrier
						!platform->onFlatFeature && groundType != COVERAGE_ROAD)
					{
						pitch = min(pitch, 1.0F);
						F4SoundFXSetPos( SFX_GRND_RUMBLE, TRUE, x + 5.0F, y, z, pitch, volume );	
					}
					else
						F4SoundFXSetPos( SFX_TAILSCRAPE, TRUE, x + 5.0F, y, z, pitch, volume );					
				}
				
				if (!IsSet(OnObject)) // JB carrier
					Mu_fric = CalcMuFric(groundType);
				else // JB carrier
					Mu_fric = CalcMuFric(COVERAGE_RUNWAY); // JB carrier
				//vtDot -= (0.4F + 0.3F * !platform->onFlatFeature) *(1.0F - nzcgs)*GRAVITY;

				
				if(vt < 1.0F && platform->DBrain()->IsSetATC(DigitalBrain::Landed))
				{
					if(platform == SimDriver.playerEntity)
						{
						if (!gPlayerExitMenuShown)
							{
							gPlayerExitMenuShown = TRUE;
							OTWDriver.SetExitMenu(TRUE);
							}
						}
					else if(SimLibElapsedTime > platform->DBrain()->WaitTime())
						RegroupAircraft(platform);
				}

			}
			else
			{
				float speedMods = 1.0F + IsSet(Simplified)*0.25F + IsSet(IsDigital)*0.25F - (!platform->onFlatFeature && groundType != COVERAGE_ROAD) * 0.3F;
									
				float gearLimitSpeed;

				if(!IsSet(OverRunway) )
					speedMods -= 0.4F;

				gearLimitSpeed = minVcas * KNOTS_TO_FTPSEC * speedMods * speedMods;

				if((vt - gearLimitSpeed * speedMods)/(5.0F * KNOTS_TO_FTPSEC * speedMods)*dt > (float)rand()/(float)RAND_MAX )
				{
					pitch = max(0.5F, min(100.0F/vt, 2.0F));
					volume = max(0.0F, min(40000.0F - vt*vt, 4000000.0F));
					F4SoundFXSetPos( SFX_TAXI_THUMP, TRUE, x + 5.0F, y, z, pitch, volume );
					bumpthe += (1.0F - 2.0F*(float)rand()/(float)RAND_MAX) * vt/(2000.0F*speedMods);
					bumpphi += (1.0F - 2.0F*(float)rand()/(float)RAND_MAX) * vt/(2000.0F*speedMods) *DTR;
					bumpyaw += (1.0F - 2.0F*(float)rand()/(float)RAND_MAX) * vt/(2000.0F*speedMods) *DTR;
				}

				if(!platform->IsSetFalcFlag(FEC_INVULNERABLE) && vt > gearLimitSpeed )
				{	
					float newpos;
					int which = rand()%NumGear();
					float dmg;
					if((vt - gearLimitSpeed)/(5.0F * KNOTS_TO_FTPSEC)*dt > (float)rand()/(float)RAND_MAX )
						dmg = (float)rand()/(float)RAND_MAX * (vt - gearLimitSpeed)/(10.0F * KNOTS_TO_FTPSEC) * (dt/0.1f);
					else
						dmg = 0.0F;

					if(theta <= 1.0F * DTR || which > 0)
					{
						gear[which].strength -= dmg;
					}

					if(gear[which].strength < 50.0F)
					{
						platform->mFaults->SetFault(FaultClass::gear_fault,
							FaultClass::ldgr, FaultClass::fail, FALSE);

						gear[which].flags |= GearData::GearStuck;
						if(NumGear() > 1 && platform->IsComplex())
						{
							newpos = (float)rand()/(float)RAND_MAX * 50.0F*DTR;

						   if(newpos < platform->GetDOFValue(COMP_NOS_GEAR + which))
								platform->SetDOF(COMP_NOS_GEAR + which, newpos);
						}
					}
					else if(gear[which].strength < 0.0F)
					{
						if(NumGear() > 1 && platform->IsComplex())
						{
							platform->SetDOF(COMP_NOS_GEAR + which, 0.0F);
							gear[which].flags |= GearData::GearBroken | GearData::DoorBroken;
						}
						// gear breaks sound
						F4SoundFXSetPos( auxaeroData->sndWheelBrakes, TRUE, x, y, z, 1.0f );
					}

					if(dmg > 2.0F)
					{
						pitch = max(0.5F, min(100.0F/vt, 2.0F));
						volume = max(0.0F, min(40000.0F - vt*vt, 4000000.0F));
						F4SoundFXSetPos( SFX_HIT_2 + rand()%4, TRUE, x + 5.0F, y, z, pitch, volume );
						bumpthe += (float)rand()/(float)RAND_MAX * dmg * vt/400.0F;
						bumpphi += (1.0F - 2.0F*(float)rand()/(float)RAND_MAX) * dmg * vt/400.0F *DTR;
						bumpyaw += (1.0F - 2.0F*(float)rand()/(float)RAND_MAX) * dmg * vt/400.0F *DTR;
					}
				}

				pitch = max(0.1F, min(vt/100.0F, 1.0F));
				volume = max(0.0F, min(22500.0F - vt*vt, 4000000.0F));
				F4SoundFXSetPos( SFX_GRND_RUMBLE, TRUE, x + 5.0F, y, z, pitch, volume );
				
				Mu_fric = CalcMuFric(groundType);
			}
		}
		
		netAccel = vtDot * dt - (0.8F*Mu_fric + (float)fabs(0.8F*platform->platformAngles->sinbet))*(1.0F - nzcgs)*GRAVITY*dt;
		newVt = max(0.0F, vt + netAccel);
		netAccel = (newVt - vt)/dt;
#ifdef DAVE_DBG
		//if(platform == SimDriver.playerEntity)
		//	MonoPrint("netAccel: %5.3f vel0: %5.3f vel1: %5.3f pos0: %5.3f pos1: %5.3f alpha: %5.3f\n", netAccel, gear[0].vel, gear[1].vel, -platform->GetDOFValue(COMP_NOS_GEAR_COMP), -platform->GetDOFValue(COMP_NOS_GEAR_COMP + 1), alpha);
#endif
		
		//if(IsSet(WheelBrakes) && vt > 20.0F*KNOTS_TO_FTPSEC && platform == SimDriver.playerEntity)
#if 0
		//MI original code
		if(IsSet(WheelBrakes) && platform == SimDriver.playerEntity && netAccel - vtDot * dt < -20.0F*KNOTS_TO_FTPSEC*dt &&
			!IsSet(GearBroken) && gearPos >= 0.8F && platform->platformAngles->cosphi > 0.9659F)
		{
			float volume = max(0.0F, 2000.0F - netAccel/dt*100.0F);
			F4SoundFXSetPos( SFX_TIRE_SQUEAL, TRUE, x + 5.0F, y, z, 1.0F, volume );
		}
#else
		//MI modified so brakesound only get's played above 80kts
		if(IsSet(WheelBrakes) && platform == SimDriver.playerEntity && netAccel - vtDot * dt < -20.0F*KNOTS_TO_FTPSEC*dt && 
			vt > 80.0*KNOTS_TO_FTPSEC && !IsSet(GearBroken) && gearPos >= 0.8F && platform->platformAngles->cosphi > 0.9659F)
		{
			float volume = max(0.0F, 2000.0F - netAccel/dt*100.0F);
			F4SoundFXSetPos( SFX_TIRE_SQUEAL, TRUE, x + 5.0F, y, z, 1.0F, volume );
		}
#endif

		vt = newVt;
	}
	else
	{
		ClearFlag(NoseSteerOn);
/* REMOVED BY S.G. THIS CODE IS PART OF THE CLUPRIT WHY HOOKING TO THE TANKER DOESN'T WORK			
		// If on a boom, damp the rates
		if (IsSet(Refueling))
		{
			float vtDeadband = 5.0F*PlayerOptions.GetRefuelingMode();
			
			if (vtDot > vtDeadband)
				vtDot -= vtDeadband;
			else if (vtDot < -vtDeadband)
				vtDot += vtDeadband;
			else
				vtDot = 0.5F * (forcedSpeed - vt);
		}
*/		
		//      newVt    = Math.FITust(vtDot,dt,oldvt);
		newVt = vt + vtDot * dt;
		netAccel = vtDot *dt;
		
		if (newVt != 0.0F)
			vt = newVt;
		else
			vt = 0.01F;
	}
	
	return netAccel;
}

void AirframeClass::CalculateGroundPlane(float *gndGmma, float *relMu)
{
	float gndPitch, xyGnd, cosHdgDiff, sinHdgDiff;
	
	xyGnd = (float)sqrt(gndNormal.x*gndNormal.x + gndNormal.y*gndNormal.y);
	
	gndPitch = (float)atan(xyGnd/-gndNormal.z);
	
	if(xyGnd)
	{
		cosHdgDiff = gndNormal.x/xyGnd*platform->platformAngles->cossig + gndNormal.y/xyGnd*platform->platformAngles->sinsig;
		sinHdgDiff = gndNormal.x/xyGnd*platform->platformAngles->sinsig - gndNormal.y/xyGnd*platform->platformAngles->cossig;
	}
	else
	{
		cosHdgDiff = 0.0F;
		sinHdgDiff = 0.0F;
	}
	
	*gndGmma = gndPitch * -cosHdgDiff;
	
	*relMu = - gndPitch * sinHdgDiff;
}

//void AirframeClass::SetGroundPosition(float dt, float netAccel, float gndGmma, float relMu)
void AirframeClass::SetGroundPosition(float dt, float netAccel, float gndGmma, float)
{
	float minHeight = CheckHeight();
	z = groundZ - minHeight;

	if (IsSet(OnObject)) // JB carrier
		z = onObjectHeight; // JB carrier

	if(IsSet(GearBroken))
	{
		if( netAccel < 0.0F)
		{
			float alpdelta = -1.0F*dt;
			alpha += alpdelta;
			oldp03[0] += alpdelta;
			oldp03[1] += alpdelta;
		}
	}
	else 
	{
		float tempVt = max(10.0F,vt);
/*
		if(netAccel < 0.0F)
			netAccel *= 20.0F*IsSet(WheelBrakes);
		else 
			tempVt *= 2.0F;*/
		
		//grndphi = max(-2.0F*DTR,min( grndphi*0.9F + vt*r/(DTR*20000.0F), 2.0F*DTR));
		//grndthe = max(-2.5F, min(grndthe*0.97F + netAccel/tempVt ,1.0F));

		if(NumGear() > 1 && platform->drawPointer)
		{
			if(netAccel > 0.0F)
				netAccel *= (min(0.05F,1.0F/tempVt) );
			else
				netAccel *= (max(tempVt*0.003F,0.3F) + 0.4F*IsSet(WheelBrakes));

			gear[0].vel += netAccel;
			gear[1].vel -= netAccel + vt*r*0.3F;
			gear[2].vel -= netAccel - vt*r*0.3F;
		}
	}/*
	else
	{
		//grndphi *= 0.9F;	
		if(vtDot > 0.0F)
			grndthe = min(grndthe*0.95F + vtDot/20.0F, 0.75F*DTR);
		else
			grndthe *= 0.95F;	
	}	*/
	//alpha += (grndthe + bumpthe*oscillationTimer);
	if(IsSet(Planted))
		gmma = gndGmma;
	else
		gmma = max (gmma, gndGmma);
	
	//mu += grndphi + bumpphi*oscillationTimer;
	
	bumpphi *= 0.95F;
	bumpthe *= 0.95F;

	sigma += bumpyaw;
	bumpyaw = 0.0F;

	ResetOrientation();
	Trigenometry();
}

void AirframeClass::CheckGroundImpact(float dt)
{
	Tpoint velocity;
	Tpoint noseDir;
	float impactAngle, noseAngle;
	float tmp;
	float aoacmd,betcmd,pscmd;
   
	// JB 010120
	if (!platform)
		return;
	// JB 010120

   if(platform->drawPointer && z < groundZ - platform->drawPointer->Radius()*2.0F)
	   return;

	float minHeight = CheckHeight();

	if (z > groundZ - minHeight + GROUND_TOLERANCE)
	{			
		groundType = OTWDriver.GetGroundType ( x, y );

		velocity.x = xdot/vt;
		velocity.y = ydot/vt;
		velocity.z = zdot/vt;
		
		ShiAssert(FALSE == F4IsBadReadPtr(platform->platformAngles, sizeof(*platform->platformAngles)));
		// JB 010120
		if (!platform->platformAngles)
			return;
		// JB 010120

		noseDir.x = platform->platformAngles->costhe * platform->platformAngles->cospsi;
		noseDir.y = platform->platformAngles->costhe * platform->platformAngles->sinpsi;
		noseDir.z = -platform->platformAngles->sinthe;
		tmp = (float)sqrt(noseDir.x*noseDir.x + noseDir.y*noseDir.y + noseDir.z*noseDir.z);
		noseDir.x /= tmp;
		noseDir.y /= tmp;
		noseDir.z /= tmp;
		
		noseAngle = gndNormal.x*noseDir.x + gndNormal.y*noseDir.y + gndNormal.z*noseDir.z;
		impactAngle = gndNormal.x*velocity.x + gndNormal.y*velocity.y + gndNormal.z*velocity.z;
		
		impactAngle = (float)fabs(impactAngle);
		
		// do the landing check
		if ( platform->LandingCheck( noseAngle, impactAngle, groundType) )
		{
			// the eagle has landed
			ClearFlag (InAir);
			platform->mFaults->AddLanding(SimLibElapsedTime);
			 // Set our anchor so that when we're moving slowly we can accumulate our position in high precision
			 groundAnchorX = x;
			 groundAnchorY = y;
			 groundDeltaX = 0.0f;
			 groundDeltaY = 0.0f;
			platform->SetFlag( ON_GROUND );
			
			float gndGmma, relMu;
			
			CalculateGroundPlane(&gndGmma, &relMu);
			SetGroundPosition(dt, -(vt*KNOTS_TO_FTPSEC)/30.0F, gndGmma, relMu);
			
			stallMode = None;
			slice = 0.0F;
			pitch = 0.0F;

			if( IsSet(GearBroken) || gearPos <= 0.1F )
			{
				// edg note: discovered a crash here.  I'm going to fix it elsewhere, but
				// protect against it here.  The prob: aircraft is getting init'd, isn't
				// taking off, and is very close to the ground.  It gets here and doesn't
				// have a brain yet.   Check here for brain and fix the alt check in
				// ownmain init.
				if ( platform->DBrain() && !platform->IsSetFalcFlag(FEC_INVULNERABLE))
				{
					platform->DBrain()->SetATCFlag(DigitalBrain::Landed);
					platform->DBrain()->SetATCStatus(lCrashed);
					// KCK NOTE:: Don't set timer for players
					if(platform != SimDriver.playerEntity)
						platform->DBrain()->SetWaitTimer(SimLibElapsedTime + 1 * CampaignMinutes);
				}

				int Runway = 0;
				GridIndex X,Y;
		
				X = SimToGrid(y);
				Y = SimToGrid(x);
				
				Objective airbase = FindNearbyAirbase (X, Y);
				if(airbase )
				{
					Runway = airbase->brain->IsOverRunway(platform);
					if( Runway )
					{
						platform->DBrain()->SendATCMsg(lCrashed);
					}
				}
			}

			// Send a landing message
			// KCK NOTE: I'm only sending this for members with the package flag set.
			// This means all package elements in single player, but non-necessarily in 
			// multi-player. But in multi-player we'll at least get all players.
			if (platform->GetCampaignObject() && platform->GetCampaignObject()->InPackage())
				{
				FalconLandingMessage *lmsg = new FalconLandingMessage(platform->Id(), FalconLocalGame);
				lmsg->dataBlock.campID = platform->GetCampaignObject()->GetCampID();
				lmsg->dataBlock.pilotID = platform->pilotSlot;
				FalconSendMessage(lmsg,TRUE);
				}
		}
		else
		{
			// we presumably have hit too hard (and taken damage/destruction)
			// apply some bounce
			z = groundZ - minHeight -  (vt * impactAngle)/ 20.0F*(1.0F - 0.5F*IsSet(GearBroken));
			
			if(!platform->IsSetFalcFlag(FEC_INVULNERABLE))
			{				
				
				aoacmd = max(-90.0F, min(90.0F, alpha +  (float)fabs(platform->platformAngles->cosbet * platform->platformAngles->sinthe) * platform->platformAngles->cosphi * 0.1F * vt));//  + q * RTD * dt));
				betcmd = max(-90.0F,min(90.0F,beta + (float)fabs(platform->platformAngles->cosalp * platform->platformAngles->sinthe) * platform->platformAngles->sinphi * 0.1F * vt));// + r *RTD* dt));
				pscmd = max(-225.0F * DTR, min(225.0F * DTR, r - platform->platformAngles->sinbet * platform->platformAngles->sinalp * platform->platformAngles->sinphi * platform->platformAngles->costhe * 0.1F * vt));// + p * RTD));
				
				YawIt(betcmd, dt);
				PitchIt(aoacmd, dt);
				RollIt(pscmd, dt);
				
				slice += (float)fabs(platform->platformAngles->cosalp * platform->platformAngles->sinthe) * platform->platformAngles->sinphi * 0.005F * vt;
				pitch += (float)fabs(platform->platformAngles->cosbet * platform->platformAngles->sinthe) * platform->platformAngles->cosphi * -0.005F * vt;
				
				float sinImpactAngle;
				
				if (impactAngle > 1.0f) // nuke bug type thing
				    impactAngle = 1.0f; // JPO - else it goes -ve next line
				sinImpactAngle = (float)sqrt(1.0F - impactAngle * impactAngle);
				
				//reduce velocity according to impact angle
				float decelFactor = min(0.99F, (sinImpactAngle * 0.9F + impactAngle * 0.2F));  
				vt = max(0.001F, decelFactor * vt);
			
				if(fabs(slice) > 0.6F || fabs(pitch) > 0.6F)
				{
					stallMode = Crashing;
					
					xdot *= decelFactor;
					ydot *= decelFactor;
					ShiAssert(!_isnan(xdot));
					ShiAssert(!_isnan(ydot));
				}
				
				if(vt < 5.0F)
				{
					stallMode = None;
					stallMagnitude = 10.0F;
					vt = 0.001F;
					ClearFlag (InAir);
					// Set our anchor so that when we're moving slowly we can accumulate our position in high precision
					groundAnchorX = x;
					groundAnchorY = y;
					groundDeltaX = 0.0f;
					groundDeltaX = 0.0f;
					platform->SetFlag( ON_GROUND );
					slice = 0.0F;
					pitch = 0.0F;
					if(fuel <= 0.0F)
						SetFlag(EngineOff);
				}
				gmma = (float)fabs( gmma/(2.0F + !IsSet(GearBroken)) );
				CalcBodyRates(dt);
				CalcBodyOrientation(dt);
				Trigenometry();
				
			}
			else
			{
				z = groundZ - minHeight - 10.0F;
				
				if(stallMode > None)
					gmma = 20.0F * DTR;
				
				stallMode = None;
				stallMagnitude = 10.0F;
				
				slice = 0.0F;
				pitch = 0.0F;
				
				if(fuel <= 0.0F)
				{
					gmma = 0.0F;
					ClearFlag (InAir);
					// Set our anchor so that when we're moving slowly we can accumulate our position in high precision
					groundAnchorX = x;
					groundAnchorY = y;
					groundDeltaX = 0.0f;
					groundDeltaY = 0.0f;
					platform->SetFlag( ON_GROUND );
					SetFlag(EngineOff);
				}
				else
				{
					gmma = max ((float)fabs(gmma/2.0F), 20.0F * DTR);
				}
				
				vt = max(vt * 0.75F, 500.0F);
				
				ResetIntegrators();
				
				alpha = 5.0F;
				beta = 0.0F;
				mu = 0.0F;
				
				ResetOrientation();
				Trigenometry();
			}
		}
	}
}

void AirframeClass::ResetIntegrators(void)
{
	memset(oldp01,0,sizeof(SAVE_ARRAY));
	memset(oldp02,0,sizeof(SAVE_ARRAY));
	memset(oldp03,0,sizeof(SAVE_ARRAY));
	memset(oldp04,0,sizeof(SAVE_ARRAY));
	memset(oldp05,0,sizeof(SAVE_ARRAY));
	memset(oldr01,0,sizeof(SAVE_ARRAY));
	memset(oldy01,0,sizeof(SAVE_ARRAY));
	//memset(oldy02,0,sizeof(SAVE_ARRAY));
	memset(oldy03,0,sizeof(SAVE_ARRAY));
	//memset(oldy04,0,sizeof(SAVE_ARRAY));
	memset(olda01,0,sizeof(SAVE_ARRAY));
}

float AirframeClass::CheckHeight(void)
{
    ShiAssert(FALSE == F4IsBadReadPtr(platform, sizeof *platform));
	// JB 010120
	if (!platform || !platform->platformAngles)
		return 0;
	// JB 010120

	if (F4IsBadReadPtr(platform, sizeof(AircraftClass))) // JB 010317 CTD
		return 0;

	float cgloc = GetAeroData(AeroDataSet::CGLoc);
	float length = GetAeroData(AeroDataSet::Length);
	float halfspan = GetAeroData(AeroDataSet::Span)/2.0F;
	float radius = GetAeroData(AeroDataSet::FusRadius);
	float gearHt = GetAeroData(AeroDataSet::NosGearZ) - radius;
		
	float deltz = 0.0F;
	float deltzNose = 0.0F;
	float deltzWing = 0.0F;
	float deltzGear = 0.0F;
	float deltzBody = 0.0F;

	Tpoint PtWorldPos;
	Tpoint PtRelPos;
	
	ShiAssert(platform->platformAngles != NULL);
	float cosphi_lim = max(0.0F,platform->platformAngles->cosphi);

	if(NumGear() > 1 && platform->drawPointer)
	{
		float best = 0.0F;		
		ShiAssert(FALSE == F4IsBadReadPtr(platform->drawPointer, sizeof(*platform->drawPointer)));
		ShiAssert(FALSE == F4IsBadReadPtr(gear, sizeof(*gear) * NumGear()));
		for(int i = 0; i < NumGear(); i++)
		{
			if(!(gear[i].flags & GearData::GearBroken))
			{
				PtRelPos.x = cgloc - GetAeroData(AeroDataSet::NosGearX + i*4);
				PtRelPos.y = GetAeroData(AeroDataSet::NosGearY + i*4);
				if (platform->IsComplex())
				    PtRelPos.z = (GetAeroData(AeroDataSet::NosGearZ + i*4) + platform->GetDOFValue(COMP_NOS_GEAR_COMP + i) - radius)*platform->GetDOFValue(COMP_NOS_GEAR + i)/(GetAeroData(AeroDataSet::NosGearRng + i*4)*DTR) + radius;
				else 
				    PtRelPos.z = GetAeroData(AeroDataSet::NosGearZ + i*4);

				MatrixMult( &((DrawableBSP*)platform->drawPointer)->orientation, &PtRelPos, &PtWorldPos );

				if(PtWorldPos.z > best)
					best = PtWorldPos.z;
			}
		}
		deltzGear = best;
	}
	else
		deltzGear = platform->platformAngles->costhe * (cosphi_lim * (gearHt * gearPos * !IsSet(GearBroken) + radius) );
	
	deltzWing = platform->platformAngles->costhe * (float)fabs(platform->platformAngles->sinphi) * halfspan;
	
	if(platform->drawPointer)
	{
		ShiAssert(_CrtIsValidPointer(platform->drawPointer, sizeof(*platform->drawPointer), TRUE));
		PtRelPos.x = 0.0F;
		PtRelPos.y = radius*platform->platformAngles->sinphi;
		PtRelPos.z = radius*platform->platformAngles->cosphi;

		MatrixMult( &((DrawableBSP*)platform->drawPointer)->orientation, &PtRelPos, &PtWorldPos );

		deltzBody = PtWorldPos.z;
	}
	else
		deltzBody = radius;

	if(platform->platformAngles->sinthe > 0.0F)
	{
		if(platform->drawPointer)
		{
			PtRelPos.x = length - cgloc;
			PtRelPos.y = 0.0F;
			PtRelPos.z = radius;

			MatrixMult( &((DrawableBSP*)platform->drawPointer)->orientation, &PtRelPos, &PtWorldPos );

			deltzNose = PtWorldPos.z;
		}
		else
			deltzNose = platform->platformAngles->sinthe*(length - cgloc);
	}
	else
	{
		if(platform->drawPointer)
		{
			PtRelPos.x = -cgloc;
			PtRelPos.y = 0.0F;
			PtRelPos.z = radius;

			MatrixMult( &((DrawableBSP*)platform->drawPointer)->orientation, &PtRelPos, &PtWorldPos );

			deltzNose = PtWorldPos.z;
		}
		else
			deltzNose = platform->platformAngles->sinthe*cgloc;
	}
	
	if(deltz < deltzWing)
		deltz = deltzWing;
	
	if(deltz < deltzNose)
		deltz = deltzNose;
	
	if(deltz < deltzGear)
		deltz = deltzGear;

	if(deltz < deltzBody)
		deltz = deltzBody;
	
	return deltz - GROUND_TOLERANCE;
}

void AirframeClass::DragBodypart(void)
{
	if(vt > 1.0F)
	{
		float pitch, volume;
		pitch = max(0.2F, min(vt/70.0F, 2.0F));
		volume = max(0.0F, min(160000.0F - vt*vt, 4000000.0F));
		F4SoundFXSetDist( SFX_TAILSCRAPE, TRUE, volume, pitch  );

		if(!IsSet(Simplified))
		{
			if(platform->pctStrength > 0.5F)
			{
				int dmgStrength = FloatToInt32(max(0.0F,vt * 0.01F*(1.0F - nzcgs))*rand()/(float)RAND_MAX);
				
				if(dmgStrength)
				{
					FalconDamageMessage *message = CreateGroundCollisionMessage(platform, dmgStrength);			
					FalconSendMessage (message,FALSE);
				}
			}
			xwaero -= 0.4F * (1.0F - nzcgs)*GRAVITY;
		}
		else
			xwaero -= 0.2F * (1.0F - nzcgs)*GRAVITY;
	}
	else if(vt <= 0.0F && platform->DBrain()->ATCStatus() != lCrashed)
	{
		platform->DBrain()->SetATCStatus(lCrashed);
		if(platform != SimDriver.playerEntity)
			platform->DBrain()->SetWaitTimer(SimLibElapsedTime + 1 * CampaignMinutes);

		int Runway = 0;
		GridIndex X,Y;

		X = SimToGrid(y);
		Y = SimToGrid(x);
		
		Objective airbase = FindNearbyAirbase (X, Y);
		if(airbase)
		{
			Runway = airbase->brain->IsOverRunway(platform);
			if( Runway )
			{
				platform->DBrain()->SendATCMsg(lCrashed);
			}
		}
	}	
}
