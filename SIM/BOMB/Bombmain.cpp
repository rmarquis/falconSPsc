#include "Graphics\Include\drawbsp.h"
#include "Graphics\Include\rviewpnt.h"
#include "Graphics\Include\terrtex.h"
#include "stdhdr.h"
#include "bomb.h"
#include "falcmesg.h"
#include "otwdrive.h"
#include "initdata.h"
#include "waypoint.h"
#include "object.h"
#include "simdrive.h"
#include "fsound.h"
#include "soundfx.h"
#include "MsgInc\DamageMsg.h"
#include "MsgInc\MissileEndMsg.h"
#include "campBase.h"
#include "campweap.h"
#include "camp2sim.h"
#include "sfx.h"
#include "fakerand.h"
#include "aircrft.h"
#include "acmi\src\include\acmirec.h"
#include "classtbl.h"
#include "Feature.h"
#include "falcsess.h"
#include "persist.h"
#include "entity.h"
#include "camplist.h"
#include "weather.h"
#include "team.h"
#include "sms.h"//me123 status test. addet

/* 2001-09-07 S.G. RP5 */ extern bool g_bRP5Comp;
//#define WEAP_LGB_3RD_GEN 0x40 moved to campwp.h and changed to 0x80

// this means that every sec, the horizontal velocity loses
// this much:
float  BombClass::dragConstant =	140.0f;
//extern bool g_bArmingDelay;//me123	MI replaced with g_bRealisticAvionics
extern bool g_bRealisticAvionics;
extern bool g_bEnableWindsAloft;
#include "fcc.h"

extern float g_fNukeStrengthFactor;
extern float g_fNukeDamageMod;
extern float g_fNukeDamageRadius;

void CalcTransformMatrix (SimBaseClass* theObject);

#ifdef USE_SH_POOLS
MEM_POOL	BombClass::pool;
#endif

BombClass::BombClass (VU_BYTE** stream) : SimWeaponClass(stream)
{
   InitData();
   memcpy (&bombType, *stream, sizeof (int));
   *stream += sizeof (int);

   if (parent && !IsLocal())
   {
      flags |= FirstFrame;
      VuReferenceEntity (parent);
      parentReferenced = TRUE;
      SetYPR(parent->Yaw(), parent->Pitch(), parent->Roll());
   }
}

BombClass::BombClass (FILE* filePtr) : SimWeaponClass(filePtr)
{
   InitData();
   fread (&bombType, sizeof (int), 1, filePtr);
}

BombClass::BombClass (int type, BombType btype) : SimWeaponClass(type)
{
   bombType = btype;
   InitData();
}

int BombClass::SaveSize()
{
   return SimWeaponClass::SaveSize() + sizeof (BombType);
}

int BombClass::Save(VU_BYTE **stream)
{
int saveSize = SimWeaponClass::Save (stream);

   if (flags & IsChaff)
      bombType = Chaff;
   else if (flags & IsFlare)
      bombType = Flare;

   memcpy (*stream, &bombType, sizeof (int));
   *stream += sizeof (int);
   return (saveSize + sizeof (int));
}

int BombClass::Save(FILE *file)
{
int saveSize = SimWeaponClass::Save (file);

   if (flags & IsChaff)
      bombType = Chaff;
   else if (flags & IsFlare)
      bombType = Flare;

   fwrite (&bombType, sizeof (int), 1, file);
   return (saveSize + sizeof (int));
}

void BombClass::InitData (void)
{
   burstHeight = 0.0F;
   detonateHeight = 0.0F;
   timeOfDeath = 0;
   SetFlag (MOTION_BMB_AI);
   flags = 0;
   dragCoeff = 0.0f;

   desDxPrev = 0.0f;
	 desDyPrev = 0.0f;
	 desDzPrev = 0.0f;
}

BombClass::~BombClass (void)
{
}

void BombClass::Init (SimInitDataClass* initData)
{
   if (initData == NULL)
      Init();
}

void BombClass::Init (void)
{
DrawableObject* tmpObject;

   tmpObject = drawPointer;
   drawPointer = NULL;
   SimWeaponClass::Init ();
   if (tmpObject)
   {
      delete drawPointer;
      drawPointer = tmpObject;
   }


   // Am I an LGB
   if (EntityType()->classInfo_[VU_STYPE] == STYPE_BOMB_GUIDED)
      flags |= IsLGB;

}

int BombClass::Wake (void)
{
int retval = 0;

	// KCK: Sets up this object to become sim aware
	if (IsAwake())
		return retval;

	InitTrail();
	ExtraGraphics();
	SimWeaponClass::Wake();

   // Change the last update time to force and exec next frame;
   AlignTimeSubtract (1);
	return retval;
}

int BombClass::Sleep (void)
{
   return SimWeaponClass::Sleep();
}

void BombClass::Start(vector* pos, vector* rate, float cD, SimObjectType *targetPtr )
{
Falcon4EntityClassType *classPtr;
WeaponClassDataType *wc;

   // 2002-02-26 ADDED BY S.G. If we passed a targetPtr, keep note of it in case the AI target is aggregated. That way we can send a Damage message to the 2D engine to take care of the target
   if (targetPtr)
	   SetTarget(targetPtr);
   // END OF ADDED SECTION

   if (parent)
   {
      flags |= FirstFrame;
      VuReferenceEntity (parent);
      parentReferenced = TRUE;
      SetYPR(parent->Yaw(), parent->Pitch(), parent->Roll());
   }
   else
   {
      SetYPR((float)atan2(rate->y, rate->x), 0.0F, 0.0F);
   }

   x = pos->x;
   y = pos->y;
   z = pos->z;

   dragCoeff = cD;
   // edg hack.  drag coeff of 1.0f we assume to be a durandal
   if ( cD >= 1.0f )
   		flags |= IsDurandal;

   SetPosition (x, y, z);
   CalcTransformMatrix (this);

	SetDelta (rate->x, rate->y, rate->z);
   SetYPRDelta(0.0F, 0.0F, 0.0F);

   // sound effect
   F4SoundFXSetPos( SFX_BOMBDROP, TRUE, pos->x, pos->y, pos->z, 1.0f );

   // if the bomb is altitude detonated (AGL), check to make sure we're
   // 2.0 sec above the detonation alt when dropped.  If not, we don't fuse and
   // do a ground impact....

	// hack for testing
	// burstHeight *= 1.2f;

   if ( burstHeight > 0.0f )
   {
   	   // get entity and weapon info
   	   classPtr = &Falcon4ClassTable[Type() - VU_LAST_ENTITY_TYPE];
   	   wc = (WeaponClassDataType *)classPtr->dataPtr;

	   // if we're not a cluster type, we should have no burst height
	   if ( !(wc->Flags & WEAP_CLUSTER) )
	   {
			burstHeight = 0.0f;
	   }
   }

   hitObj = NULL;

   // for alt fuse
   timeOfDeath = SimLibElapsedTime;
}

void BombClass::ExtraGraphics(void)
{
}

int BombClass::Exec (void)
{
// FalconDamageMessage* message;
float terrainHeight;
float delta;
VuListIterator objectWalker (SimDriver.objectList);
VuListIterator featureWalker (SimDriver.featureList);
ACMIGenPositionRecord genPos;
FalconMissileEndMessage* endMessage;
float radical, tFall, desDx, desDy;
float deltaX, deltaY, deltaZ, vt;
float rx, ry, rz, range;
mlTrig trigYaw, trigPitch;
float bheight;
float grav;
float armingdelay = 0.0f;//me123 done this way to awoid a crash when player dies in matchplay
    
if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
	armingdelay = SimDriver.playerEntity->Sms->armingdelay;//me123


   if (IsDead() || (flags & FirstFrame))
   {
      flags &= ~FirstFrame;
   	  return TRUE;
   }

   UpdateTrail();

  // ACMI Output
  if (gACMIRec.IsRecording() && (SimLibFrameCount & 0x07 ) == 0)
  {
		  genPos.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
		  genPos.data.type = Type();
		  genPos.data.uniqueID = ACMIIDTable->Add(Id(),NULL,TeamInfo[GetTeam()]->GetColor());//.num_;
		  genPos.data.x = XPos();
		  genPos.data.y = YPos();
		  genPos.data.z = ZPos();
		  genPos.data.roll = Roll();
		  genPos.data.pitch = Pitch();
		  genPos.data.yaw = Yaw();

		  if ( flags & IsFlare )
		  		gACMIRec.FlarePositionRecord( (ACMIFlarePositionRecord *)&genPos );
		  else if ( flags & IsChaff )
		  		gACMIRec.ChaffPositionRecord( (ACMIChaffPositionRecord *)&genPos );
		  else
		  		gACMIRec.GenPositionRecord( &genPos );
  }

   if (!IsLocal())
   {
      return FALSE;
   }

   if(IsExploding())
   {
		DoExplosion();
      return TRUE;
   }

	else if (SimDriver.MotionOn())
	{
		SpecialGraphics();

		x = XPos() + XDelta() * SimLibMajorFrameTime;
		y = YPos() + YDelta() * SimLibMajorFrameTime;
		z = ZPos() + ZDelta() * SimLibMajorFrameTime;
		
		float dx, dy, ratx, raty, vdrag, horiz;
		//dumb bomb algorithm
		/*y = Tan(theta) * x - (g * x^2)/(2(*v*cos(theta)^2)*/
		
		horiz = (float)sqrt( XDelta() * XDelta() + YDelta() * YDelta() + 0.1f);
		ratx = (float)fabs(XDelta())/horiz;
		raty = (float)fabs(YDelta())/horiz;
		
		// for the 1st sec don't apply drag
		grav = GRAVITY;
		if (SimLibElapsedTime - timeOfDeath > 1 * SEC_TO_MSEC )
		{
			vdrag = dragCoeff * dragConstant * SimLibMajorFrameTime;
			// super high drag will mean also reduced gravity due to
			// air friction....
			if ( dragCoeff >= 1.0f )
				grav = GRAVITY * 0.65F;
		}
		else
		{
			vdrag = 0.0f;
		}
		
		if ( fabs( XDelta() ) < vdrag * ratx)
		{
			dx = 0.0f;
		}
		else
		{
			if ( XDelta() > 0.0f )
				dx = XDelta() - vdrag * ratx;
			else
				dx = XDelta() + vdrag * ratx;
		}
		if ( fabs( YDelta() ) < vdrag * raty)
		{
			dy = 0.0f;
		}
		else
		{
			if ( YDelta() > 0.0f )
				dy = YDelta() - vdrag * raty;
			else
				dy = YDelta() + vdrag * raty;
		}
		//me123 lets add the wind change effect to the bomb fall
		//MI.... check for our variable...
		if(g_bEnableWindsAloft)
		{
			mlTrig trigWind;
			float wind;
			Tpoint			pos;
			pos.x = x;
			pos.y =y;
			pos.z = z;

			// current wind
			mlSinCos(&trigWind, ((WeatherClass*)TheWeather)->WindHeadingAt(&pos));
			wind =  ((WeatherClass*)TheWeather)->WindSpeedInFeetPerSecond(&pos);
			float winddx = trigWind.cos * wind;
			float winddy = trigWind.sin * wind;
			
			//the wind last time we checked
			pos.z = z -ZDelta() * SimLibMajorFrameTime;
			mlSinCos(&trigWind, ((WeatherClass*)TheWeather)->WindHeadingAt(&pos));
			wind =  ((WeatherClass*)TheWeather)->WindSpeedInFeetPerSecond(&pos);
			float lastwinddx = trigWind.cos * wind;
			float lastwinddy = trigWind.sin * wind;
			
			//factor in the change
			dx += (winddx-lastwinddx)*0.9f;//not all the wind since no inertie is factored in
			dy += (winddy-lastwinddy)*0.9f;//not all the wind since no inertie is factored in
		}
		

      // SetDelta (XDelta(), YDelta(), ZDelta() + GRAVITY * SimLibMajorFrameTime);
      SetDelta (dx, dy, ZDelta() + grav * SimLibMajorFrameTime);
      vt = (float)sqrt(XDelta()*XDelta() + YDelta()*YDelta() + ZDelta()*ZDelta());
      SetYPR ((float)atan2 (YDelta(), XDelta()), -(float)asin(ZDelta() / vt), Roll());

	  // NOTE:  Yaw never changes, so we could avoid much of this...
      mlSinCos (&trigYaw, Yaw());
      mlSinCos (&trigPitch, Pitch());

      dmx[0][0] = trigYaw.cos * trigPitch.cos;
      dmx[0][1] = trigYaw.sin * trigPitch.cos;
      dmx[0][2] = -trigPitch.sin;

      dmx[1][0] = -trigYaw.sin;
      dmx[1][1] = trigYaw.cos;
      dmx[1][2] = 0.0F;

      dmx[2][0] = trigYaw.cos * trigPitch.sin;
      dmx[2][1] = trigYaw.sin * trigPitch.sin;
      dmx[2][2] = trigPitch.cos;

	  // special case durandal -- when fired remove chute
	  if ( (flags & IsDurandal) &&
	  	   (flags & FireDurandal) &&
   	       drawPointer &&
		   ((DrawableBSP*)drawPointer)->GetNumSwitches() > 0 )
	  {
		  ((DrawableBSP *)drawPointer)->SetSwitchMask( 0, 0 );
	  }

	  // special case durandal.  If x and y vel reaches 0 we fire it
	  // by starting the special effect
	  if ( (flags & IsDurandal) &&
	  	  !(flags & FireDurandal) &&
		  dx == 0.0f &&
		  dy == 0.0f )
	  {
		  	flags |= FireDurandal;

		  	// accelerate towards ground
      	  	SetDelta (dx, dy, ZDelta() + 500.0f);
			endMessage = new FalconMissileEndMessage (Id(), FalconLocalGame);
			endMessage->RequestReliableTransmit ();
			endMessage->RequestOutOfBandTransmit ();
			endMessage->dataBlock.fEntityID  = parent->Id();
			endMessage->dataBlock.fCampID    = parent->GetCampID();
			endMessage->dataBlock.fSide      = parent->GetCountry();
			endMessage->dataBlock.fPilotID   = (uchar)shooterPilotSlot;
			endMessage->dataBlock.fIndex     = parent->Type();
	   		endMessage->dataBlock.dEntityID  = FalconNullId;
	   		endMessage->dataBlock.dCampID    = 0;
	   		endMessage->dataBlock.dSide      = 0;
	   		endMessage->dataBlock.dPilotID   = 0;
	   		endMessage->dataBlock.dIndex     = 0;
			endMessage->dataBlock.fWeaponUID = Id();
			endMessage->dataBlock.wIndex 	 = Type();
			endMessage->dataBlock.x    = XPos() + XDelta() * SimLibMajorFrameTime * 2.0f;
			endMessage->dataBlock.y    = YPos() + YDelta() * SimLibMajorFrameTime * 2.0f;
			endMessage->dataBlock.z    = ZPos() + ZDelta() * SimLibMajorFrameTime * 2.0f;
			endMessage->dataBlock.xDelta    = XDelta();
			endMessage->dataBlock.yDelta    = YDelta();
			endMessage->dataBlock.zDelta    = ZDelta();
			endMessage->dataBlock.groundType    = -1;
			endMessage->dataBlock.endCode    = FalconMissileEndMessage::BombImpact;

			FalconSendMessage (endMessage,FALSE);
	  }
	  //MI for refined LGB stuff
	  // 2002-01-06 MODIFIED BY S.G. In the odd chance that there is no parent, neither this or the realistic section would be ran. If no parent, run this
	  //                             There is no danger of a CTD if parent is NULL because the OR will have it enter the if statement without running the 'IsPlayer'.
//	  if(!g_bRealisticAvionics || ( parent && !((AircraftClass *)parent)->IsPlayer()))
	  if(!g_bRealisticAvionics || !parent || !((AircraftClass *)parent)->IsPlayer())
	  {
// 2001-04-17 MODIFIED BY S.G. NEED TO SEPARATE THE TWO IF CONDITION IN TWO IF STATEMENTS
//		  if ((flags & IsLGB) && targetPtr)
	      if (flags & IsLGB)
		  {
			  if (targetPtr)
			  {
				  // JPO - target might be higher than us...
				  if (targetPtr->BaseData()->ZPos() <= ZPos()) {
					  radical = 0;
				  } else {
					  radical = (float)sqrt (ZDelta()*ZDelta() + 2.0F * GRAVITY * (targetPtr->BaseData()->ZPos() - ZPos()));
				  }
				  tFall = -ZDelta() - radical;
				  if (tFall < 0.0F)
					  tFall = -ZDelta() + radical;			 
				  tFall /= GRAVITY;

				  deltaX = targetPtr->BaseData()->XPos() - XPos();
				  deltaY = targetPtr->BaseData()->YPos() - YPos();
				  deltaZ = (float)fabs(targetPtr->BaseData()->ZPos() - ZPos());

				  rx    = dmx[0][0] * deltaX + dmx[0][1] * deltaY + dmx[0][2] * deltaZ;
				  ry    = dmx[1][0] * deltaX + dmx[1][1] * deltaY + dmx[1][2] * deltaZ;
				  rz    = dmx[2][0] * deltaX + dmx[2][1] * deltaY + dmx[2][2] * deltaZ;
				  range = (float)sqrt(rx*rx + ry*ry + rz*rz);

				  // 45 degree limit on the seeker
				  // 2001-04-17 MODIFIED BY S.G. AGAIN, THEY MIXE UP DEGREES AND RADIAN BUT THIS TIME, THE OTHER WAY AROUND...
				  //            PLUS THEY FORGOT THAT atan2 RETURNS A SIGNED VALUE
				  //            END THEN, range-rx * rx is range - rx*rx which is ALWAYS NEGATIVE. CAN'T TAKE A SQUARE ROOT OF A NEGATIVE NUMBER. I'VE CODED WHAT I *THINK* THEY WERE TRYING TO DO...
				  //				  if (atan2(sqrt(range-rx * rx),rx) < 45.0F * RTD)
				  if (fabs(atan2(sqrt(range*range - rx*rx),rx)) < 45.0F * DTR)
				  {
					  desDx = (deltaX)/tFall;
					  desDy = (deltaY)/tFall;
					  // 2001-04-17 ADDED BY S.G. WE'LL KEEP OUR LAST desDx and desDy IN THE UNUSED tgtX and tgtY BombClass VARIABLE (RENAMED desDxPrev AND desDyPrev) TO IN CASE WE LOOSE LOCK LATER
					  desDxPrev = desDx;
					  desDyPrev = desDy;
					  // END OF ADDED SECTION
					  SetDelta (0.8F*XDelta() + 0.2F*desDx, 0.8F*YDelta() + 0.2F*desDy, ZDelta());
				  }
				  // 2002-01-05 ADDED BY S.G. Similarly to below. if the lgb cannot see the laser, it can't guide (forgot to do this).
				  else if (g_bRP5Comp) 
				  {
					  // 2001-04-17 ADDED BY S.G. WE'LL KEEP GOING WHERE WE WERE GOING...
					  Falcon4EntityClassType *classPtr = &Falcon4ClassTable[Type() - VU_LAST_ENTITY_TYPE];
					  WeaponClassDataType *wc = (WeaponClassDataType *)classPtr->dataPtr;

					  // If a 3rg gen LGB, fins are more precised, even when no longer lased...
					 //#define WEAP_LGB_3RD_GEN 0x40 moved to campwp.h and changed to 0x80
					  if (wc->Flags & WEAP_LGB_3RD_GEN) 
						  SetDelta (0.8F*XDelta() + 0.2f*desDxPrev, 0.8f*YDelta() + 0.2f*desDyPrev, ZDelta());
					  else // 2001-10-19 MODIFIED BY S.G. IT'S * 1.05f AND NOT * 2.0f!
						  SetDelta ((0.8F*XDelta() + 0.2f*desDxPrev) * 1.05f, (0.8f*YDelta() + 0.2f*desDyPrev) * 1.05f, ZDelta());
				  }

				  if (!((SimBaseClass*)(targetPtr->BaseData()))->IsSetFlag(IS_LASED))
				  {
					  targetPtr->Release( SIM_OBJ_REF_ARGS );
					  targetPtr = NULL;
				  }
			  }
			  // 2001-04-17 ADDED BY S.G. WE'LL KEEP GOING WHERE WE WERE GOING...
			  else if (g_bRP5Comp) 
			  {
				  // 2001-04-17 ADDED BY S.G. WE'LL KEEP GOING WHERE WE WERE GOING...
				  Falcon4EntityClassType *classPtr = &Falcon4ClassTable[Type() - VU_LAST_ENTITY_TYPE];
				  WeaponClassDataType *wc = (WeaponClassDataType *)classPtr->dataPtr;

				  // If a 3rg gen LGB, fins are more precised, even when no longer lased...
				 //#define WEAP_LGB_3RD_GEN 0x40 moved to campwp.h and changed to 0x80
				  if (wc->Flags & WEAP_LGB_3RD_GEN) 
					  SetDelta (0.8F*XDelta() + 0.2f*desDxPrev, 0.8f*YDelta() + 0.2f*desDyPrev, ZDelta());
				  else // 2001-10-19 MODIFIED BY S.G. IT'S * 1.05f AND NOT * 2.0f!
					  SetDelta ((0.8F*XDelta() + 0.2f*desDxPrev) * 1.05f, (0.8f*YDelta() + 0.2f*desDyPrev) * 1.05f, ZDelta());
			  }
		  }
	  }
	  else if((flags & IsLGB) && g_bRealisticAvionics)
	  {
		  //AI's don't need to keep a lock until impact
		  if(parent && ((AircraftClass *)parent)->IsPlayer())
		  {
			  radical = 0;
			  if(SimDriver.playerEntity->FCC->groundDesignateZ <= ZPos()) 
				  radical = 0;
			  else
				  if (targetPtr && targetPtr->BaseData())	// M.N. possible CTD fix
				  radical = (float)sqrt (ZDelta()*ZDelta() + 2.0F * GRAVITY * (targetPtr->BaseData()->ZPos() - ZPos()));

			  radical = (float)sqrt (ZDelta()*ZDelta() + 2.0F * GRAVITY * (SimDriver.playerEntity->FCC->groundDesignateZ - ZPos()));
			  tFall = -ZDelta() - radical;
			  if (tFall < 0.0F)
				  tFall = -ZDelta() + radical;
			  tFall /= GRAVITY;

			  deltaX = SimDriver.playerEntity->FCC->groundDesignateX - XPos();
			  deltaY = SimDriver.playerEntity->FCC->groundDesignateY - YPos();
			  deltaZ = SimDriver.playerEntity->FCC->groundDesignateZ - ZPos();

			  rx    = dmx[0][0] * deltaX + dmx[0][1] * deltaY + dmx[0][2] * deltaZ;
			  ry    = dmx[1][0] * deltaX + dmx[1][1] * deltaY + dmx[1][2] * deltaZ;
			  rz    = dmx[2][0] * deltaX + dmx[2][1] * deltaY + dmx[2][2] * deltaZ;
			  range = (float)sqrt(rx*rx + ry*ry + rz*rz);
			  float range1 = (float)sqrt(deltaX*deltaX + deltaY*deltaY+deltaZ*deltaZ);
			  float rate = (float)sqrt(XDelta()*XDelta()+YDelta()*YDelta()+ZDelta()*ZDelta());
			
			  //we need to find the time until impact
			  SimDriver.playerEntity->FCC->ImpactTime = tFall;

			  // 18 degree limit on the seeker	
			  if(fabs(atan2(sqrt(range-rx * rx),rx) <= 18.0F * DTR &&
			  SimDriver.playerEntity->FCC->LaserFire))
			  {
				  desDx = (deltaX)/tFall;
				  desDy = (deltaY)/tFall;
				  // 2001-04-17 ADDED BY S.G. WE'LL KEEP OUR LAST desDx and desDy IN THE UNUSED tgtX and tgtY BombClass VARIABLE (RENAMED desDxPrev AND desDyPrev) TO IN CASE WE LOOSE LOCK LATER
				  desDxPrev = desDx;
				  desDyPrev = desDy;
				  // END OF ADDED SECTION
				  SetDelta (0.8F*XDelta() + 0.2F*desDx, 0.8F*YDelta() + 0.2F*desDy, ZDelta());
			  }
			  else
			  {
				  if(!(desDxPrev == 0.0f && desDyPrev == 0.0f && desDzPrev == 0.0f)) 
				  {
					  // 2001-04-17 ADDED BY S.G. WE'LL KEEP GOING WHERE WE WERE GOING...
					  Falcon4EntityClassType *classPtr = &Falcon4ClassTable[Type() - VU_LAST_ENTITY_TYPE];
					  WeaponClassDataType *wc = (WeaponClassDataType *)classPtr->dataPtr;
					  // If a 3rg gen LGB, fins are more precised, even when no longer lased...
					  if(wc->Flags & WEAP_LGB_3RD_GEN) 
						  SetDelta(0.8F*XDelta() + 0.2f*desDxPrev, 0.8f*YDelta() + 0.2f*desDyPrev, ZDelta());
					  else // 2001-10-19 MODIFIED BY S.G. IT'S * 1.05f AND NOT * 2.0f!
						  SetDelta((0.8F*XDelta() + 0.2f*desDxPrev) * 1.05f, (0.8f*YDelta() + 0.2f*desDyPrev) * 1.05f, ZDelta());
				  }
			  }
			  if(targetPtr && targetPtr->BaseData())
			  {
				  if (!((SimBaseClass*)(targetPtr->BaseData()))->IsSetFlag(IS_LASED))
				  {
					  targetPtr->Release( SIM_OBJ_REF_ARGS );
					  targetPtr = NULL;
				  }
			  }
		  }
	  }

	  terrainHeight = OTWDriver.GetGroundLevel(x, y);

	  // 2 seconds from release until any alt detonation will fuse
    if (SimLibElapsedTime - timeOfDeath > 2 * SEC_TO_MSEC)
		{	
			bheight = burstHeight;
		}
	  else
  		bheight = 0.0f;
	  // check for feature collision impact  

	  if ( bombType == None &&  z - terrainHeight > -800.0f)
	  {
		  hitObj = FeatureCollision( terrainHeight);
		  if (hitObj)
		  {
			  //me123 OWLOOK make your armingdelay switch here.  
			  //MI
			  //if (g_bArmingDelay && (SimLibElapsedTime - timeOfDeath > armingdelay *10  || ((AircraftClass *)parent)->isDigital))
			  if(g_bRealisticAvionics && (SimLibElapsedTime - timeOfDeath > armingdelay *10  || (parent && ((AircraftClass *)parent)->isDigital)))
			  {
				  //me123 addet arming check, for now digi's dont's have arming delay, becourse they will fuck up the delivery
				  SendDamageMessage(hitObj,0,FalconDamageType::BombDamage);
				  // JB 000816 ApplyProximityDamage( terrainHeight, 0.0f ); // Cause of objects not blowing up on runways
				  ApplyProximityDamage( terrainHeight, detonateHeight );
				  edeltaX = XDelta();
				  edeltaY = YDelta();
				  edeltaZ = ZDelta();
				  SetDelta (0.0f, 0.0f, 0.0f );
				  SetExploding(TRUE);

				  // if we've hit a flat container, NULL it out now so that this is
				  // treated as a ground hit later on
				  if (hitObj->IsSetCampaignFlag (FEAT_FLAT_CONTAINER))
				  {
					  hitObj = NULL;
				  }
			  }
			  //else if (!g_bArmingDelay)	MI
			  else if(!g_bRealisticAvionics)
			  {
				  SendDamageMessage(hitObj,0,FalconDamageType::BombDamage);
				  // JB 000816 ApplyProximityDamage( terrainHeight, 0.0f ); // Cause of objects not blowing up on runways
				  ApplyProximityDamage( terrainHeight, detonateHeight );
				  edeltaX = XDelta();
				  edeltaY = YDelta();
				  edeltaZ = ZDelta();
				  SetDelta (0.0f, 0.0f, 0.0f );
				  SetExploding(TRUE);

				  // if we've hit a flat container, NULL it out now so that this is
				  // treated as a ground hit later on
				  if (hitObj->IsSetCampaignFlag (FEAT_FLAT_CONTAINER))
				  {
					  hitObj = NULL;
				  }
			  }
		  }
		  else if (z >= terrainHeight )
		  {
			  if (bombType == None && (SimLibElapsedTime - timeOfDeath > armingdelay *10.0f || (parent && ((AircraftClass *)parent)->isDigital)))//me123 addet arming check
			  {
				  // Interpolate
				  delta = (z - terrainHeight) / (z - ZPos());

				  x = x - delta * (x - XPos());
				  y = y - delta * (y - YPos());

				  edeltaX = XDelta();
				  edeltaY = YDelta();
				  edeltaZ = ZDelta();
				  SetDelta (0.0f, 0.0f, 0.0f );

				  SetYPR (Yaw() + (float)rand()/(float)RAND_MAX, 0.0F, 0.0F);

				  SetFlag (ON_GROUND);
				  z = terrainHeight;

				  SetExploding(TRUE);

				  ApplyProximityDamage( terrainHeight, detonateHeight );
			  }
			  else
			  {
				  SetDead (TRUE);
			  }
		  }
	  }
	  //MI this else is causing our CBU's to not burst with a BA < 900 because of the check above
	  //else
		if (bheight > 0 && z >= terrainHeight - bheight && !IsSetFlag( SHOW_EXPLOSION ) && bombType == BombClass::None )//me123 check addet to making flares stop exploding
	  {
		  // for altitude detonations we start the effect here
		  SetFlag( SHOW_EXPLOSION );


		  // 2002-02-26 ADDED BY S.G. If our target is an aggregated entity, send a 'SendDamageMessage' message to the target and let the 2D engine sort out what gets destroyed...
		  if (targetPtr) {
			  // First get the campaign object if it's still a sim entity
			  CampBaseClass *campBaseObj;
			  if (targetPtr->BaseData()->IsSim()) // If we're a SIM object, get our campaign object
				  campBaseObj = ((SimBaseClass*)targetPtr->BaseData())->GetCampaignObject();
			  else
				  campBaseObj = (CampBaseClass *)targetPtr->BaseData();
			  // Now find out if our campaign object is aggregated
			  if (campBaseObj && campBaseObj->IsAggregate()) {
				  // Yes, send a damage message right away otherwise the other code is not going to deal with it...
				  SendDamageMessage(campBaseObj,0,FalconDamageType::BombDamage);
			  }
		  }

		  endMessage = new FalconMissileEndMessage (Id(), FalconLocalGame);
		  endMessage->RequestReliableTransmit ();
		  endMessage->RequestOutOfBandTransmit ();
		  endMessage->dataBlock.fEntityID		= parent->Id();
		  endMessage->dataBlock.fCampID		= parent->GetCampID();
		  endMessage->dataBlock.fSide			= (uchar)parent->GetCountry();
		  endMessage->dataBlock.fPilotID		= shooterPilotSlot;
		  endMessage->dataBlock.fIndex		= parent->Type();
		  endMessage->dataBlock.dEntityID		= FalconNullId;
		  endMessage->dataBlock.dCampID		= 0;
		  endMessage->dataBlock.dSide			= 0;
		  endMessage->dataBlock.dPilotID		= 0;
		  endMessage->dataBlock.dIndex		= 0;
		  endMessage->dataBlock.fWeaponUID	= Id();
		  endMessage->dataBlock.wIndex 		= Type();
		  endMessage->dataBlock.x				= XPos() + XDelta() * SimLibMajorFrameTime * 2.0f;
		  endMessage->dataBlock.y				= YPos() + YDelta() * SimLibMajorFrameTime * 2.0f;
		  endMessage->dataBlock.z				= ZPos() + ZDelta() * SimLibMajorFrameTime * 2.0f;
		  endMessage->dataBlock.xDelta		= XDelta();
		  endMessage->dataBlock.yDelta		= YDelta();
		  endMessage->dataBlock.zDelta		= ZDelta();
		  endMessage->dataBlock.groundType    = -1;
		  endMessage->dataBlock.endCode		= FalconMissileEndMessage::BombImpact;
		  
		  FalconSendMessage (endMessage,FALSE);

		  // set height at which we detonated for applying
		  // proximity damage
		  detonateHeight = max( 0.0f, terrainHeight - z );
	  }
      SetPosition (x, y, z);	  
   }
   return TRUE;
}

void BombClass::SetTarget (SimObjectType* newTarget)
{
	if (F4IsBadReadPtr(this, sizeof(BombClass))) // JB 010317 CTD
		return;

   if (newTarget == targetPtr)
		return;

   if ( targetPtr )
   {
	   targetPtr->Release( SIM_OBJ_REF_ARGS );
	   targetPtr = NULL;
   }

   if (newTarget)
   {
      ShiAssert( newTarget->BaseData() != (FalconEntity*)0xDDDDDDDD );

	  #ifdef DEBUG
	  targetPtr = newTarget->Copy(OBJ_TAG, this);
	  #else
      targetPtr = newTarget->Copy();
      #endif
	  targetPtr->Reference( SIM_OBJ_REF_ARGS );
   }
}

void BombClass::GetTransform(TransformMatrix vmat)
{
mlTrig trig;
float xyDelta = XDelta()*XDelta() + YDelta()*YDelta();
float vt = (float)sqrt(xyDelta + ZDelta()*ZDelta());
float costha, sintha;

   xyDelta = (float)sqrt(xyDelta);
   mlSinCos (&trig, Yaw());
   costha = xyDelta / vt;
   sintha = ZDelta() / vt;

   vmat[0][0] = trig.cos * costha;
   vmat[0][1] = trig.sin * costha;
   vmat[0][2] = -sintha;

   vmat[1][0] = -trig.sin;
   vmat[1][1] = trig.cos;
   vmat[1][2] = 0.0F;

   vmat[2][0] = trig.cos * sintha;
   vmat[2][1] = trig.sin * sintha;
   vmat[2][2] = costha;
}

void BombClass::SetVuPosition (void)
{
   SetPosition (x, y, z);
}

/*
** Name: ApplyProximityDamage
** Description:
**		Cycles thru objectList check for proximity.
**		Cycles thru all objectives, and checks vs individual features
**        if it's within the objective's bounds.
*/
void
BombClass::ApplyProximityDamage ( float groundZ, float detonateHeight )
{
float tmpX, tmpY, tmpZ;
float rangeSquare;
SimBaseClass* testObject;
CampBaseClass* objective;
float damageRadiusSqrd;
float strength, damageMod;
WeaponClassDataType* wc;
wc = (WeaponClassDataType *)(Falcon4ClassTable[Type() - VU_LAST_ENTITY_TYPE].dataPtr);
float modifier = 1.0F;
if (wc && wc->DamageType == NuclearDam)
	modifier = g_fNukeDamageRadius;
#ifdef VU_GRID_TREE_Y_MAJOR
VuGridIterator gridIt(ObjProxList, YPos(), XPos(), NM_TO_FT * (3.5F * modifier));
#else
VuGridIterator gridIt(ObjProxList, XPos(), YPos(), NM_TO_FT * (3.5F * modifier));
#endif

//MI
float hat = 0;

	// for altitude detonations (cluster bomb), the damage radius must
	// be changed depending on the detonation height
	if ( burstHeight > 0.0f )
	{
		// at about 3000.0f you get the full damage radius
		// JB 000816 // Give it about the min damage of a MK-82
		//damageRadiusSqrd = max( 1000.0f, lethalRadiusSqrd * detonateHeight/3000.0f );
		damageRadiusSqrd = max( lethalRadiusSqrd / 20, lethalRadiusSqrd * detonateHeight/3000.0f ); 
		// JB 000816

		// we'll alter the power of the damage by adjusting rangeSquare
		// (make it bigger) by using a strength factor.  The higher the
		// detonation, the lower the concentrated strength
		strength = 1.0f + 2.0f * detonateHeight/3000.0f;
	}
	else
	{
		damageRadiusSqrd = lethalRadiusSqrd;
		strength = 1.0f;
	}

	if (parentReferenced && SimDriver.objectList)
		{
		VuListIterator objectWalker(SimDriver.objectList);

      // Damage multiplier for damage type

      switch (wc->DamageType)
      {
			case PenetrationDam:
			case HeaveDam:
			case KineticDam:
			case IncendairyDam:
			case ChemicalDam:
            damageMod = 0.25F;
			break;
			case HighExplosiveDam:
			case ProximityDam:
			case HydrostaticDam:
			//case NuclearDam:					
	     case OtherDam:
         case NoDamage:
         default:
            damageMod = 1.0F;
		break;
/*		//MI give nukes a higher damage
		 case NuclearDam:
			 damageMod = 10000.0F;			// externalised
		 break; */ 
      }

		// Check vs vehicles
		testObject = (SimBaseClass*) objectWalker.GetFirst();
		while (testObject)
			{

			// until digi's are smarter about thier bombing, prevent them
			// from dying in their own blast
// 2002-04-21 MN check for damage type and only skip if it is not a nuclear
			if ( wc->DamageType != NuclearDam && (testObject == parent &&
				 parent && parent->IsAirplane() &&
				 ( ((AircraftClass *)parent)->isDigital ||
				 ((AircraftClass *)parent)->AutopilotType() == AircraftClass::CombatAP  ) ))
			{
				testObject = (SimBaseClass*) objectWalker.GetNext();
				continue;
			}

			if (testObject != this)
				{
				tmpX = testObject->XPos() - XPos();
				tmpY = testObject->YPos() - YPos();
				tmpZ = testObject->ZPos() - groundZ;

				rangeSquare = tmpX*tmpX + tmpY*tmpY + tmpZ*tmpZ;

				// Height Above Terrain
				if(parent)
					hat = ((AircraftClass*)parent)->ZPos() - OTWDriver.GetGroundLevel(((AircraftClass*)parent)->XPos(), ((AircraftClass*)parent)->YPos());
				//MI special case for airplane. Use the "MaxAlt" field to determine if you blow up or not
				if(testObject && testObject->IsAirplane() && wc && wc->DamageType == NuclearDam)
				{
					//if you're below the entered setting, you're screwed
					if(fabs((wc->MaxAlt)*1000) >= fabs(hat))
// 2002-03-25 MN removed damageMod, as the higher this value, the less the chance to hit
						SendDamageMessage(testObject,rangeSquare*strength * /*damageMod*/ g_fNukeStrengthFactor,FalconDamageType::ProximityDamage);
				}
// 2002-03-25 MN some more fixes for nukes
				else if (wc && wc->DamageType == NuclearDam)
				{
					if (rangeSquare < damageRadiusSqrd * g_fNukeDamageMod)
					{
						SendDamageMessage(testObject,rangeSquare*strength*g_fNukeStrengthFactor,FalconDamageType::ProximityDamage);					
					}
				}
				else if (rangeSquare < damageRadiusSqrd * damageMod)
						SendDamageMessage(testObject,rangeSquare*strength,FalconDamageType::ProximityDamage);
				}
			testObject = (SimBaseClass*) objectWalker.GetNext();
			}
		}

	// get the 1st objective that contains the bomb
	objective = (CampBaseClass*)gridIt.GetFirst();

	// main loop through objectives
	while ( objective )
		{
		if (objective->GetComponents())
			{
			// loop thru each element in the objective
			VuListIterator	featureWalker(objective->GetComponents());
			testObject = (SimBaseClass*) featureWalker.GetFirst();
			while (testObject)
				{
				if (!testObject->IsSetCampaignFlag(FEAT_CONTAINER_TOP))
					{
					tmpX = testObject->XPos() - XPos();
					tmpY = testObject->YPos() - YPos();
//					tmpZ = testObject->ZPos() - ZPos();		// Features are at ground level, and so is this bomb

					rangeSquare = tmpX*tmpX + tmpY*tmpY;;	// + tmpZ*tmpZ;
					if (wc && wc->DamageType == NuclearDam)
						{
						if (rangeSquare < damageRadiusSqrd * g_fNukeDamageMod)
							{
							SendDamageMessage(testObject,rangeSquare*strength*g_fNukeStrengthFactor,FalconDamageType::ProximityDamage);					
							}
						}
					else if (rangeSquare < damageRadiusSqrd * damageMod)	//MI added *damageMod
						{
							SendDamageMessage(testObject,rangeSquare * strength,FalconDamageType::ProximityDamage);
						} // end if within lethal radius
					}
				testObject = (SimBaseClass*) featureWalker.GetNext();
				}
			}

		// get the next objective that contains the bomb
	   objective = (CampBaseClass*)gridIt.GetNext();
		} // end objective loop

}

void BombClass::DoExplosion(void)
{
ACMIStationarySfxRecord acmiStatSfx;
FalconMissileEndMessage* endMessage;
float groundZ;

	if ( !IsSetFlag( SHOW_EXPLOSION ))
	{
		// edg note: all special effects are now handled in the
		// missile end message process method
		endMessage = new FalconMissileEndMessage (Id(), FalconLocalGame);
		endMessage->RequestReliableTransmit ();
		endMessage->RequestOutOfBandTransmit ();
		endMessage->dataBlock.fEntityID		= parent ? parent->Id() : Id();
		endMessage->dataBlock.fCampID		= parent ? parent->GetCampID() : 0;
		endMessage->dataBlock.fSide			= parent ? (uchar)parent->GetCountry() : 0;
		endMessage->dataBlock.fPilotID		= shooterPilotSlot;
		endMessage->dataBlock.fIndex		= parent ? parent->Type() : 0;
	   endMessage->dataBlock.dEntityID		= FalconNullId;
	   endMessage->dataBlock.dCampID		= 0;
	   endMessage->dataBlock.dSide			= 0;
	   endMessage->dataBlock.dPilotID		= 0;
	   endMessage->dataBlock.dIndex		= 0;
		endMessage->dataBlock.fWeaponUID	= Id();
		endMessage->dataBlock.wIndex 		= Type();
		endMessage->dataBlock.x				= XPos();
		endMessage->dataBlock.y				= YPos();
		endMessage->dataBlock.z				= ZPos();
		endMessage->dataBlock.xDelta		= edeltaX;
		endMessage->dataBlock.yDelta		= edeltaY;
		endMessage->dataBlock.zDelta		= edeltaZ;

		// add crater depending on ground type and closeness to ground
      	groundZ = OTWDriver.GetGroundLevel(XPos(), YPos());

		if ( hitObj )
			endMessage->dataBlock.endCode    = FalconMissileEndMessage::FeatureImpact;
		else
			endMessage->dataBlock.endCode    = FalconMissileEndMessage::BombImpact;

		endMessage->dataBlock.groundType    =
			   	(char)OTWDriver.GetGroundType ( XPos(), YPos() );

		FalconSendMessage (endMessage,FALSE);


		if ( hitObj == NULL &&
			  !(endMessage->dataBlock.groundType == COVERAGE_WATER ||
				endMessage->dataBlock.groundType == COVERAGE_RIVER )
				) //&&( ZPos() - groundZ ) > -40.0f ) // JB 010710 craters weren't showing up
		{
			// AddToTimedPersistantList (VIS_CRATER2 + PRANDInt3(), Camp_GetCurrentTime() + CRATER_REMOVAL_TIME, XPos(), YPos());
			AddToTimedPersistantList (MapVisId(VIS_CRATER2 + 2), Camp_GetCurrentTime() + CRATER_REMOVAL_TIME, XPos(), YPos());

			// add crater to ACMI as special effect
			if ( gACMIRec.IsRecording() )
			{

				acmiStatSfx.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
				acmiStatSfx.data.type = SFX_CRATER4;
				acmiStatSfx.data.x = XPos();
				acmiStatSfx.data.y = YPos();
				acmiStatSfx.data.z = ZPos();
				acmiStatSfx.data.timeToLive = 180.0f;
				acmiStatSfx.data.scale = 1.0f;
				gACMIRec.StationarySfxRecord( &acmiStatSfx );
			}
		}

		 // make sure we don't do it again...
		 SetFlag( SHOW_EXPLOSION );
	}
	else if ( !IsDead() )
	{
		 // we can now kill it immediately
		 SetDead(TRUE);
	}
}

void BombClass::SpecialGraphics (void)
{
	if(SimLibElapsedTime - timeOfDeath > 1 * SEC_TO_MSEC)
	{
		if (((DrawableBSP*)drawPointer)->GetNumSwitches() > 0)
		{
			((DrawableBSP *)drawPointer)->SetSwitchMask( 0, 1 );
		}
	}
}

void BombClass::InitTrail (void)
{
}

void BombClass::UpdateTrail (void)
{
}

void BombClass::RemoveTrail (void)
{
}
