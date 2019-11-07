#include "Graphics\Include\drawbsp.h"
#include "Graphics\Include\drawsgmt.h"
#include "Graphics\Include\drawpuff.h"
#include "stdhdr.h"
#include "falcmesg.h"
#include "helo.h"
#include "fsound.h"
#include "soundfx.h"
#include "MsgInc\DamageMsg.h"
#include "MsgInc\DeathMessage.h"
#include "campbase.h"
#include "simdrive.h"
#include "hardpnt.h"
#include "camp2sim.h"
#include "digi.h"
#include "MsgInc\WingmanMsg.h"
#include "airframe.h"
#include "otwdrive.h"
#include "sms.h"
#include "sfx.h"
#include "fakerand.h"

void CalcTransformMatrix (SimBaseClass* theObject);
void DecomposeMatrix (Trotation* matrix, float* pitch, float* roll, float* yaw);

void HelicopterClass::ApplyDamage (FalconDamageMessage* damageMessage)
{
   if (IsDead() || IsExploding())
      return;

   // call generic vehicle damage stuff
   SimVehicleClass::ApplyDamage (damageMessage);

   // do any type specific stuff here:

   // damage smoke trails
   /*
   for (int i=0; i<numSmokeTrails; i++)
   {
      if (pctStrength < (1.0F - (1.0F/numSmokeTrails) * i))
      {
         if (!smokeTrail[i]->InDisplayList())
            OTWDriver.InsertObject (smokeTrail[i]);
      }
   }
   */
}

void HelicopterClass::InitDamageStation (void)
{
#if 0		// KCK Removed on 6/23 - This stuff isn't really necessary and breaks a unification assumption
Falcon4EntityClassType* classPtr;
VuEntityType* eclassPtr = NULL;
int i, debrisvis;
Tpoint    simView = {0.0F, 0.0F, 0.0F};
Trotation viewRotation = IMatrix;

   // current parts are:
   //		fuselage
   //		2 wings
   //		tail
   numBodyParts = 4;

   damageStation = new WeaponStation;
   damageStation->onRail = new SimBaseClass*[numBodyParts];
   damageStation->SetupPoints(numBodyParts);


   // get the aircraft's class info
   // for each aircraft, visType[1] should indicate the specific type for
   // the aircraft's damaged version
   eclassPtr = EntityType();
   F4Assert (eclassPtr);
   classPtr = &(Falcon4ClassTable[
      GetClassID(
	  		eclassPtr->classInfo_[VU_DOMAIN],
			eclassPtr->classInfo_[VU_CLASS],
			eclassPtr->classInfo_[VU_TYPE],
			eclassPtr->classInfo_[VU_STYPE],
      		eclassPtr->classInfo_[VU_SPTYPE],
			VU_ANY, VU_ANY, VU_ANY)
	  ]);
   /*
   classPtr = &(Falcon4ClassTable[
      GetClassID(
	  		eclassPtr->classInfo_[VU_DOMAIN],
			eclassPtr->classInfo_[VU_CLASS],
			eclassPtr->classInfo_[VU_TYPE],
			eclassPtr->classInfo_[VU_STYPE],
      		classPtr->visType[3],
			VU_ANY, VU_ANY, VU_ANY)
	  ]);
   */

   for (i=0, debrisvis=0; i<numBodyParts; i++)
   {
      damageStation->onRail[i] = new SimBaseClass(Type());
      CalcTransformMatrix (damageStation->onRail[i]);
      OTWDriver.CreateVisualObject(damageStation->onRail[i], classPtr->visType[i+2], &simView, &viewRotation, OTWDriver.Scale());

	  /*
	  if ( i > 0 )
	  {
      	((DrawableBSP*)(damageStation->onRail[0]->drawPointer))->GetChildOffset(i, &simView);
      	((DrawableBSP*)(damageStation->onRail[0]->drawPointer))->AttachChild(((DrawableBSP*)(damageStation->onRail[i]->drawPointer)), i);
	  }
      damageStation->SetSubPosition(i, simView.x, simView.y, simView.z);
      damageStation->SetSubRotation(i, 0.0F, 0.0F);
	  */
   }


   // Init smoke trails
   numSmokeTrails = 4;
   smokeTrail = new DrawableTrail*[numSmokeTrails];
   for (i=0; i<numSmokeTrails; i++)
      smokeTrail[i] = new DrawableTrail(TRAIL_SMOKE);
#endif
}

void HelicopterClass::CleanupDamageStation (void)
{
}

void HelicopterClass::RunExplosion (void)
{
	int i;
	Tpoint    pos;
	Falcon4EntityClassType *classPtr;
	SimBaseClass	*tmpSimBase;
	Tpoint tp = Origin;
	Trotation tr = IMatrix;

    // F4PlaySound (SFX_DEF[SFX_OWNSHIP_BOOM].handle);
	F4SoundFXSetPos( SFX_BOOMA1 + PRANDInt5(), TRUE, XPos(), YPos(), ZPos(), 1.0f );

	// 1st do primary explosion
    pos.x = XPos();
    pos.y = YPos();
    pos.z = ZPos();

	if ( OnGround( ) )
	{
		pos.z = OTWDriver.GetGroundLevel( pos.x, pos.y ) - 4.0f;
		SetDelta( XDelta() * 0.1f, YDelta() * 0.1f, -50.0f );
    	OTWDriver.AddSfxRequest(
  			new SfxClass (SFX_GROUND_EXPLOSION,				// type
			&pos,							// world pos
			1.2f,							// time to live
			100.0f ) );		// scale
	}
	else
	{
    	OTWDriver.AddSfxRequest(
  			new SfxClass (SFX_AIR_HANGING_EXPLOSION,				// type
			&pos,							// world pos
			2.0f,							// time to live
			200.0f + 200 * PRANDFloatPos() ) );		// scale
	}
	classPtr = (Falcon4EntityClassType*)EntityType();

	// Add the parts (appairently hardcoded at 4)
	// Recoded by KCK on 6/23 to remove damage station BS
	for (i=0; i<4; i++)
		{
		tmpSimBase = new SimBaseClass(Type());
		CalcTransformMatrix (tmpSimBase);
		OTWDriver.CreateVisualObject(tmpSimBase, classPtr->visType[i+2], &tp, &tr, OTWDriver.Scale());
		tmpSimBase->SetPosition (pos.x, pos.y, pos.z);

		if (!i)
			{
			tmpSimBase->SetDelta (XDelta(), YDelta(), ZDelta());
			}
		if (!OnGround())
			{
			tmpSimBase->SetDelta (	XDelta() + 50.0f * PRANDFloat(),
									YDelta() + 50.0f * PRANDFloat(),
									ZDelta() + 50.0f * PRANDFloat() );
			}
		else
			{
			tmpSimBase->SetDelta (	XDelta() + 50.0f * PRANDFloat(),
									YDelta() + 50.0f * PRANDFloat(),
									ZDelta() - 50.0f * PRANDFloatPos() );
			}
		tmpSimBase->SetYPR (Yaw(), Pitch(), Roll());

		if (!i)
			{
			// First peice is more steady and is flaming
			tmpSimBase->SetYPRDelta ( 0.0F, 0.0F, 10.0F + PRANDFloat() * 30.0F * DTR);
			OTWDriver.AddSfxRequest(
  			new SfxClass (SFX_FLAMING_PART,				// type
				SFX_MOVES | SFX_USES_GRAVITY | SFX_EXPLODE_WHEN_DONE,
				tmpSimBase,								// sim base *
				3.0f + PRANDFloatPos() * 4.0F,			// time to live
				1.0F ) );								// scale
			}
		else
			{
			// spin piece a random amount
			tmpSimBase->SetYPRDelta (	PRANDFloat() * 30.0F * DTR,
										PRANDFloat() * 30.0F * DTR,
										PRANDFloat() * 30.0F * DTR);
			OTWDriver.AddSfxRequest(
				new SfxClass (SFX_SMOKING_PART,			// type
				SFX_MOVES | SFX_USES_GRAVITY | SFX_BOUNCES | SFX_EXPLODE_WHEN_DONE,
				tmpSimBase,								// sim base *
				4.0f * PRANDFloatPos() + (float)((i+1)*(i+1)),	// time to live
				1.0 ) );								// scale
			}
		}
}

void HelicopterClass::ShowDamage (void)
{
   if ( pctStrength < 0.65f && pctStrength > 0.0f)
   {
	   if ( sfxTimer > ( max( pctStrength, 0.1f) + PRANDFloatPos() * 0.3f ) )
	   {
			  Tpoint pos, vec;

			  // reset the timer
			  sfxTimer = 0.0f;

			  pos.x = XPos();
			  pos.y = YPos();
			  pos.z = ZPos();
			  vec.x = PRANDFloat() * 40.0f;
			  vec.y = PRANDFloat() * 40.0f;
			  vec.z = PRANDFloat() * 40.0f;
			  OTWDriver.AddSfxRequest(
		   			new SfxClass(SFX_TRAILSMOKE,				// type
					SFX_MOVES,						// flags
					&pos,							// world pos
					&vec,							// vector
					2.5f,							// time to live
					10.5f + (1.0F-pctStrength)*30.0f ) );		// scale
	   }
   }
}
