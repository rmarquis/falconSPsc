#include "stdhdr.h"
#include "Graphics\Include\drawbsp.h"
#include "Graphics\Include\drawsgmt.h"
#include "Graphics\Include\drawpuff.h"
#include "Graphics\Include\RenderOW.h"
#include "falcmesg.h"
#include "aircrft.h"
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
#include "object.h"
#include "sms.h"
#include "sfx.h"
#include "fakerand.h"
#include "simeject.h"
#include "fault.h"
#include "fack.h"
#include "falcsess.h"
#include "Classtbl.h"
#include "SimEject.h"
#include "simweapn.h"
#include "playerop.h"
#include "weather.h"
#include "ffeedbk.h"
#include "dofsnswitches.h"	//MI
#include "IvibeData.h"

extern void *gSharedIntellivibe;

extern bool g_bDisableFunkyChicken; // JB 010104
#include "ui\include\uicomms.h" // JB 010107
extern UIComms *gCommsMgr; // JB 010107
#include "include\terrtex.h" // JB carrier
extern float g_fCarrierStartTolerance; // JB carrier

extern bool g_bNewDamageEffects;	//MI

void CalcTransformMatrix (SimBaseClass* theObject);
void DecomposeMatrix (Trotation* matrix, float* pitch, float* roll, float* yaw);

int flag_keep_smoke_trails = FALSE;
// wing tip vortex constants.
static const float minwingvortexalt = 0; // where vortex conditions start
static const float maxwingvortexalt = 10000; // where they end
static const float wingvortexalpha = 15; // what alpha you need to exceed
static const float wingvortexgs = 4; // what G you need to exceed


void AircraftClass::ApplyDamage (FalconDamageMessage* damageMessage)
{
float rx=0.0F, ry=0.0F, rz=0.0F;
float cosAta=1.0F, sinAta=0.0F;
int octant=0;
long failuresPossible=0;
int	failures = 0;
int startStrength=0, damageDone=0;
SimBaseClass *object=NULL;
float dx=0.0F;
float dy=0.0F;
float dz=0.0F;
float range=0.0F;

   if (IsDead() || IsExploding() )
      return;

   object = (SimBaseClass *)vuDatabase->Find(damageMessage->dataBlock.fEntityID);
   if(object)
   {
	   dx		= object->XPos() - XPos();
	   dy		= object->YPos() - YPos();
	   dz		= object->ZPos() - ZPos();
	   range	= (float)(dx * dx + dy * dy + dz * dz);

		// Find relative position
	   rx		= dmx[0][0] * dx + dmx[0][1] * dy + dmx[0][2] * dz;
	   ry		= dmx[1][0] * dx + dmx[1][1] * dy + dmx[1][2] * dz;
	   rz		= dmx[2][0] * dx + dmx[2][1] * dy + dmx[2][2] * dz;

      // Find the octant
      octant = 0;
      if (rx > 0.0F) // In front
         octant = 1;

      if (ry > 0.0F) // On right
         octant += 2;

      if (rz > 0.0F) // below
         octant += 4;
	
	   cosAta = rx/range;
	   sinAta = (float)sqrt(range-rx * rx)/range;
   }
	else
	{
		cosAta = 1.0F;
		sinAta = 0.0F;
		octant = FloatToInt32(PRANDFloatPos() * 7.0F);
	}

   if(damageMessage->dataBlock.damageType == FalconDamageType::ObjectCollisionDamage)
   {
		Tpoint Objvec={0.0F}, Myvec={0.0F}, relVec={0.0F};
		float relVel=0.0F, objMass=0.0F;
		
		float deltaSig=0.0F, deltaGmma=0.0F;
		
			

		Myvec.x = XDelta();
		Myvec.y = YDelta();
		Myvec.z = ZDelta();
		
		if (object && object->IsSim())
		{
			Objvec.x = object->XDelta();
			Objvec.y = object->YDelta();
			Objvec.z = object->ZDelta();
			objMass = object->Mass();			

			if(object->IsAirplane())
				deltaSig = ((AircraftClass*)object)->af->sigma - af->sigma;
			else
				deltaSig = object->Yaw() - af->sigma;

			if(deltaSig*RTD < -180.0F)
				deltaSig += 360.0F*DTR;
			else if(deltaSig*RTD > 180.0F)
				deltaSig -= 360.0F*DTR;

			af->sigma -= deltaSig*0.75F*sinAta* objMass/Mass();

			if(object->IsAirplane())
				deltaGmma = ((AircraftClass*)object)->af->gmma - af->gmma;
			else
				deltaGmma = object->Pitch() - af->gmma;

			if(deltaGmma*RTD < -90.0F)
				deltaGmma += 180.0F*DTR;
			else if(deltaGmma*RTD > 90.0F)
				deltaGmma -= 180.0F*DTR;

			af->gmma -= deltaGmma*0.75F*sinAta* objMass/Mass();

			af->ResetOrientation();			
		}
		else
		{
			Objvec.x = 0.0F;
			Objvec.y = 0.0F;
			Objvec.z = 0.0F;
			objMass = 2500.0F;
			cosAta = 1.0F;
		}

		relVec.x = Myvec.x - Objvec.x;
		relVec.y = Myvec.y - Objvec.y;
		relVec.z = Myvec.z - Objvec.z;

		relVel = (float)sqrt(relVec.x*relVec.x + relVec.y*relVec.y + relVec.z*relVec.z);

		// poke airframe data to simulate collisions
		af->x -= relVec.x * objMass/Mass()*cosAta;
		af->y -= relVec.y * objMass/Mass()*cosAta;
		af->z -= relVec.z * objMass/Mass()*cosAta;

		if(IsSetFalcFlag(FEC_INVULNERABLE))
		{
			af->vt *= 0.9F;
		}
		else
		{
			af->vt -= relVel * objMass/Mass()*cosAta;
			af->vt = max(0.0F, af->vt);
		}
   }
   else if(damageMessage->dataBlock.damageType == FalconDamageType::FeatureCollisionDamage)
   {
	   af->x -= af->xdot*cosAta;
	   af->y -= af->ydot*cosAta;
	   af->z -= af->zdot*cosAta;

	   if(IsSetFalcFlag(FEC_INVULNERABLE))
	   {
		   af->vt *= 0.9F;
	   }
	   else
	   {
		   af->vt = -0.01F;
	   }	   
   }
   else if(damageMessage->dataBlock.damageType == FalconDamageType::MissileDamage)
   {
      if (this == SimDriver.playerEntity)
      {
         JoystickPlayEffect (JoyHitEffect, FloatToInt32((float)atan2(ry, rx) * RTD));
      }
   }

   startStrength = FloatToInt32(strength);
   // call generic vehicle damage stuff
   SimVehicleClass::ApplyDamage (damageMessage);
   damageDone = startStrength - FloatToInt32(strength);
   if (this == FalconLocalSession->GetPlayerEntity()) {
       g_intellivibeData.lastdamage = octant + 1;
       g_intellivibeData.damageforce = damageDone;
       g_intellivibeData.whendamage = SimLibElapsedTime;
			 memcpy (gSharedIntellivibe, &g_intellivibeData, sizeof(g_intellivibeData));
   }

   // do any type specific stuff here:
//   if (IsLocal() && !(this == (SimBaseClass*)SimDriver.playerEntity && PlayerOptions.InvulnerableOn()))
   if (IsLocal() && !IsSetFalcFlag(FEC_INVULNERABLE))
   {
      // Randomly break something based on the damage inflicted
      // Find out who did it

      /* Fault types
         acmi - Air Combat maneuvering Instrumentation Pod, use when tape full?
         amux - Avionics Data Bus. If failed, fail all avionics (very rare) 
	      blkr - Interference Blanker. If failed, radio drives radar crazy
         cadc - Central Air Data Computer. If failed no baro altitude
	      dmux - Weapon Data Bus. If failed, no weapons fire from given station
         dlnk - Improved Data Modem. If failed no data transfer in
	      eng  - Engine. If failed usually means fire, but could mean hydraulics
	      fcc  - Fire Control Computer. If failed, no weapons solutions
	      fcr  - Fire Control Radar. If failed, no radar
         flcs - Digital Flight Control System. If failed one or more control surfaces stuck
	      fms  - Fuel Measurement System. If failed, fuel gauge stuck
         gear - Landing Gear. If failed one or more wheels stuck
	      hud  - Heads Up Display. If failed, no HUD
         iff  - Identification, Friend or Foe. If failed, no IFF
	      ins  - Inertial Navigation System. If failed, no change in waypoint data
         mfds - Multi Function Display Set. If an MFD fails, it shows noise
	      ralt - Radar Altimeter. If failed, no ALOW warning
         rwr  - Radar Warning Reciever. If failed, no rwr
	      sms  - Stores Management System. If failed, no weapon or inventory display, and no launch
         tcn  - TACAN. If failed no TACAN data
	      msl  - Missile Slave Loop. If failed, missile will not slave to radar
         ufc  - Up Front Controller. If failed, UFC/DED inoperative
      */
      switch (octant)
      {
         case 0: // Back, Left, Upper
            failuresPossible =
               (1 << FaultClass::eng_fault) +
               (1 << FaultClass::hud_fault) +
               (1 << FaultClass::fcc_fault) +
               (1 << FaultClass::flcs_fault) +
               (1 << FaultClass::mfds_fault) +
               (1 << FaultClass::rwr_fault);

				failures = 6;
//            MonoPrint ("Upper Left Back Damage\n");
         break;

         case 1: // Front, Left, Upper
            failuresPossible = 2 ^ FaultClass::NumFaultListSubSystems - 1 -
               (1 << FaultClass::dmux_fault) -
               (1 << FaultClass::eng_fault) -
               (1 << FaultClass::gear_fault) -
               (1 << FaultClass::ralt_fault) -
               (1 << FaultClass::msl_fault);
				failures = 6;
//            MonoPrint ("Upper Left Front Damage\n");
				//MI add LEF damage
				if(g_bRealisticAvionics && g_bNewDamageEffects)
				{
					//random canopy damage
					if(rand()%100 < 10)	//low chance
						CanopyDamaged = TRUE;
					if(rand()%100 < 25 && !LEFState(LT_LEF_OUT))	//25% we're taking damage
					{
						if(LEFState(LT_LEF_DAMAGED))
						{
							LEFOn(LT_LEF_OUT);
							LTLEFAOA = max(min(af->alpha,25.0f)* DTR, 0.0f);
						}
						else
						{
							LEFOn(LT_LEF_DAMAGED);
							LTLEFAOA = max(min(af->alpha,25.0f)* DTR, 0.0f);
							if(rand()%100 > 20)
								LEFOn(LT_LEF_OUT);
						}
					}
				}
         break;

         case 2:  // Back Right, Upper
            failuresPossible =
               (1 << FaultClass::eng_fault) +
               (1 << FaultClass::hud_fault) +
               (1 << FaultClass::fcc_fault) +
               (1 << FaultClass::flcs_fault) +
               (1 << FaultClass::mfds_fault) +
               (1 << FaultClass::rwr_fault);
				failures = 6;
//            MonoPrint ("Upper Right Back Damage\n");
         break;

         case 3:  // Front Right, Upper
            failuresPossible = 2 ^ FaultClass::NumFaultListSubSystems - 1 -
               (1 << FaultClass::dmux_fault) -
               (1 << FaultClass::eng_fault) -
               (1 << FaultClass::gear_fault) -
               (1 << FaultClass::ralt_fault) -
               (1 << FaultClass::msl_fault);
				failures = 6;
//            MonoPrint ("Upper Right Front Damage\n");
				//MI add LEF damage
				if(g_bRealisticAvionics && g_bNewDamageEffects)
				{
					if(rand()%100 < 10)	//low chance
						CanopyDamaged = TRUE;
					if(rand()%100 < 25 && !LEFState(RT_LEF_OUT))	//25% we're taking damage
					{
						if(LEFState(RT_LEF_DAMAGED))
						{
							LEFOn(RT_LEF_OUT);
							RTLEFAOA = max(min(af->alpha,25.0f)* DTR, 0.0f);
						}
						else
						{
							LEFOn(RT_LEF_DAMAGED);
							RTLEFAOA = max(min(af->alpha,25.0f)* DTR, 0.0f);
							if(rand()%100 > 20)
								LEFOn(RT_LEF_OUT);
						}
					}
				}
         break;

         case 4: // Back, Left, Upper
            failuresPossible = 2 ^ FaultClass::NumFaultListSubSystems - 1 -
               (1 << FaultClass::hud_fault) -
               (1 << FaultClass::mfds_fault) -
               (1 << FaultClass::ufc_fault);
				failures = 4;
//            MonoPrint ("Lower Left Back Damage\n");
         break;

         case 5: // Front, Left, Lower
            failuresPossible = 2 ^ FaultClass::NumFaultListSubSystems - 1 -
               (1 << FaultClass::eng_fault) -
               (1 << FaultClass::hud_fault) -
               (1 << FaultClass::mfds_fault) -
               (1 << FaultClass::ufc_fault);
				failures = 5;
//            MonoPrint ("Lower Left Front Damage\n");
				//MI add LEF damage
				if(g_bRealisticAvionics && g_bNewDamageEffects)
				{
					if(rand()%100 < 25 && !LEFState(LT_LEF_OUT))	//25% we're taking damage
					{
						if(LEFState(LT_LEF_DAMAGED))
						{
							LEFOn(LT_LEF_OUT);
							LTLEFAOA = max(min(af->alpha,25.0f)* DTR, 0.0f);
						}
						else
						{
							LEFOn(LT_LEF_DAMAGED);
							LTLEFAOA = max(min(af->alpha,25.0f)* DTR, 0.0f);
							if(rand()%100 > 20)
								LEFOn(LT_LEF_OUT);
						}
					}
				}
         break;

         case 6:  // Back Right, Lower
            failuresPossible = 2 ^ FaultClass::NumFaultListSubSystems - 1 -
               (1 << FaultClass::hud_fault) -
               (1 << FaultClass::mfds_fault) -
               (1 << FaultClass::ufc_fault);
				failures = 4;
//            MonoPrint ("Lower Right Back Damage\n");
         break;

         case 7:  // Front Right, Lower
			default:
            failuresPossible = 2 ^ FaultClass::NumFaultListSubSystems - 1 -
               (1 << FaultClass::eng_fault) -
               (1 << FaultClass::hud_fault) -
               (1 << FaultClass::mfds_fault) -
               (1 << FaultClass::ufc_fault);
				failures = 5;
//            MonoPrint ("Lower Right Front Damage\n");
				//MI add LEF damage
				if(g_bRealisticAvionics && g_bNewDamageEffects)
				{
					if(rand()%100 < 25 && !LEFState(RT_LEF_OUT))	//25% we're taking damage
					{
						if(LEFState(RT_LEF_DAMAGED))
						{
							LEFOn(RT_LEF_OUT);
							RTLEFAOA = GetDOFValue(IsComplex() ? COMP_LT_LEF : SIMP_LT_LEF);
						}
						else
						{
							LEFOn(RT_LEF_DAMAGED);
							RTLEFAOA = GetDOFValue(IsComplex() ? COMP_RT_LEF : SIMP_RT_LEF);
							if(rand()%100 > 20)
								LEFOn(RT_LEF_OUT);
						}
					}
				}
         break;
      }

      AddFault (failures, failuresPossible, damageDone/5, octant);
   }
}

void AircraftClass::InitDamageStation (void)
{
}

void AircraftClass::CleanupDamageStation (void)
{
    int i;
    for (i=0; i<TRAIL_MAX; i++)
    {
	if (smokeTrail[i])
	{
	    OTWDriver.RemoveObject(smokeTrail[i], TRUE);
	    smokeTrail[i] = NULL;
	}
    }
    for (i = 0; i < MAXENGINES; i++) {
	if (conTrails[i])
	{
	    OTWDriver.RemoveObject(conTrails[i], TRUE);
	    conTrails[i] = NULL;
	}
	if (engineTrails[i])
	{
	    OTWDriver.RemoveObject(engineTrails[i], TRUE);
	    engineTrails[i] = NULL;
	}
    }
}

//////////////////////////////////////
#define	DAMAGEF16_NOSE_SLOTINDEX	0
#define	DAMAGEF16_FRONT_SLOTINDEX	1
#define	DAMAGEF16_BACK_SLOTINDEX	2
#define	DAMAGEF16_RWING_SLOTINDEX	3
#define	DAMAGEF16_LWING_SLOTINDEX	4
#define	DAMAGEF16_LSTAB_SLOTINDEX	5
#define	DAMAGEF16_RSTAB_SLOTINDEX	6

#define DAMAGEF16_RWING				0x01
#define DAMAGEF16_LWING				0x02
#define DAMAGEF16_LSTAB				0x04
#define DAMAGEF16_RSTAB				0x08
#define DAMAGEF16_BACK				0x10
#define DAMAGEF16_FRONT				0x20
#define DAMAGEF16_NOSE				0x40
#define DAMAGEF16_ALL				0x7f

#define DAMAGEF16_NOLEFT			0x79
#define DAMAGEF16_NOLEFTANDNOSE		0x39

#define DAMAGEF16_NORIGHT			0x76
#define DAMAGEF16_NORIGHTANDNOSE	0x36

#define DAMAGEF16_ONLYBODY			0x70
#define DAMAGEF16_NONOSE			0x3f
#define DAMAGEF16_BACKWITHWING		0x1f

#define DAMAGEF16_BACKWITHRIGHT		0x19
#define DAMAGEF16_BACKWITHLEFT		0x16

#define	DAMAGEF16_TOLEFT			1
#define	DAMAGEF16_TORIGHT			2
#define	DAMAGEF16_TOLEFTRIGHT		3
#define	DAMAGEF16_TOFRONT			4

#define	DAMAGEF16_ID					VIS_CF16A
#define	DAMAGEF16_SWITCH				0
#define	DAMAGEF16_NOSEBREAK_SWITCH		1
#define	DAMAGEF16_FRONTBREAK_SWITCH		2
#define	DAMAGEF16_LWINGBREAK_SWITCH		3
#define	DAMAGEF16_RWINGBREAK_SWITCH		4
#define	DAMAGEF16_CANOPYBREAK_SWITCH	5

int AircraftClass::SetDamageF16PieceType (DamageF16PieceStructure *piece, int type, int flag, int mask, float speed)
{
	if (!(type & mask)) return 0;

	piece -> damage = MapVisId(DAMAGEF16_ID);
	piece -> mask = type;
	piece -> sfxtype = SFX_SMOKING_PART;
	piece -> sfxflag = SFX_F16CRASHLANDING | SFX_MOVES | SFX_BOUNCES;
	piece -> lifetime = 300.0f;

	piece -> pitch = 0.0f;
	piece -> roll = 0.0f;

	piece -> yd = 0.0f;
	piece -> pd = 0.0f;
	piece -> rd = 0.0f;

	piece -> dx = 1.0f;
	piece -> dy = 1.0f;
	piece -> dz = 1.0f;

	float angle=0.0F, angle1=0.0F, angle2=0.0F, roll=0.0F;

	if (PRANDFloatPos() > 0.5f)
		roll = 110.0f * DTR;
	else
		roll = 40.0f * DTR;

	angle1 = 0.0f;
	angle2 = 0.0f;
	switch (type) {
		case DAMAGEF16_NOLEFTANDNOSE:
		case DAMAGEF16_NOLEFT:
			piece -> sfxflag |= SFX_F16CRASH_OBJECT;
			piece -> roll = -roll;
			piece -> index = DAMAGEF16_FRONT_SLOTINDEX;
			speed += 20.0f;
			angle = 270.0f * DTR;
			piece -> dz = 2.0f;
			piece -> dx = 0.75f;
			break;

		case DAMAGEF16_NORIGHTANDNOSE:
		case DAMAGEF16_NORIGHT:
			piece -> sfxflag |= SFX_F16CRASH_OBJECT;
			piece -> roll = roll;
			piece -> index = DAMAGEF16_FRONT_SLOTINDEX;
			speed += 20.0f;
			angle = 270.0f * DTR;
			piece -> dz = 2.0f;
			piece -> dx = 0.75f;
			break;

		case DAMAGEF16_ONLYBODY:
			piece -> sfxflag |= SFX_F16CRASH_OBJECT;
			if (PRANDFloatPos() > 0.5f) roll = -roll;
			piece -> roll = roll;
			piece -> index = DAMAGEF16_FRONT_SLOTINDEX;
			speed += 20.0f;
			angle = 360.0f * DTR;
			piece -> dz = 1.5f;
			piece -> dx = 0.75f;
			break;

		case DAMAGEF16_NONOSE:
			piece -> sfxflag |= SFX_F16CRASH_OBJECT;
			piece -> index = DAMAGEF16_FRONT_SLOTINDEX;
			speed += 10.0f;
			angle = 270.0f * DTR;
			piece -> dz = 1.25f;
			piece -> dx = 0.5f;
			break;

		case DAMAGEF16_BACKWITHWING:
			piece -> index = DAMAGEF16_BACK_SLOTINDEX;
			speed += 15.0f;
			angle = 270.0f * DTR;
			piece -> dz = 2.0f;
			piece -> dx = 0.75f;
			break;

		case DAMAGEF16_BACKWITHLEFT:
			piece -> roll = roll;
			piece -> index = DAMAGEF16_BACK_SLOTINDEX;
			speed += 15.0f;
			angle = 270.0f * DTR;
			piece -> dz = 2.0f;
			piece -> dx = 0.75f;
			break;

		case DAMAGEF16_BACKWITHRIGHT:
			piece -> roll = -roll;
			piece -> index = DAMAGEF16_BACK_SLOTINDEX;
			speed += 15.0f;
			angle = 270.0f * DTR;
			piece -> dz = 2.0f;
			piece -> dx = 0.75f;
			break;

		case DAMAGEF16_BACK:
			if (PRANDFloatPos() > 0.5f) roll = -roll;
			piece -> roll = roll;
			piece -> index = DAMAGEF16_BACK_SLOTINDEX;
			speed += 25.0f;
			angle = 360.0f * DTR;
			piece -> dz = 1.25f;
			piece -> dx = 0.75f;
			break;

		case DAMAGEF16_FRONT:
			piece -> sfxflag |= SFX_F16CRASH_OBJECT;
			piece -> roll = 40.0f*DTR;
			piece -> index = DAMAGEF16_FRONT_SLOTINDEX;
			speed += 30.0f;
			angle = 360.0f * DTR;
			piece -> dz = 1.125f;
			break;

		case DAMAGEF16_NOSE:
			piece -> pitch = -5.0f*DTR;
			piece -> sfxflag |= SFX_BOUNCES;
			piece -> index = DAMAGEF16_NOSE_SLOTINDEX;
			speed += 50.0f;
			angle = 450.0f * DTR;
			piece -> dx = 1.5f;
			piece -> dx += PRANDFloatPos();
			if (flag & DAMAGEF16_TOLEFTRIGHT) {
				piece -> dy += PRANDFloatPos();
			}
			break;

		case DAMAGEF16_RWING:
			piece -> sfxflag |= SFX_BOUNCES;
			piece -> index = DAMAGEF16_RWING_SLOTINDEX;
			if (flag & DAMAGEF16_TOLEFT) {
				speed += 30.0f;
				piece -> dy = 0.75f;
			}
			else {
				speed += 40.0f;
				piece -> dy = 1.5f;
			}
			if (flag & DAMAGEF16_TOFRONT) {
				piece -> dx += PRANDFloatPos();
			}
			piece -> dy += PRANDFloatPos();
			angle = 360.0f * DTR;
			piece -> dz = 1.0625f;
			break;

		case DAMAGEF16_LWING:
			piece -> sfxflag |= SFX_BOUNCES;
			piece -> index = DAMAGEF16_LWING_SLOTINDEX;
			if (flag & DAMAGEF16_TORIGHT) {
				speed += 30.0f;
				piece -> dy = 0.75f;
			}
			else {
				speed += 40.0f;
				piece -> dy = 1.5f;
			}
			if (flag & DAMAGEF16_TOFRONT) {
				piece -> dx += PRANDFloatPos();
			}
			piece -> dy += PRANDFloatPos();
			angle = -360.0f * DTR;
			piece -> dz = 1.0625f;
			break;

		case DAMAGEF16_LSTAB:
			piece -> sfxflag |= SFX_BOUNCES;
			piece -> index = DAMAGEF16_LSTAB_SLOTINDEX;
			if (flag & DAMAGEF16_TORIGHT) {
				speed += 40.0f;
				piece -> dy = 0.75f;
			}
			else {
				speed += 50.0f;
				piece -> dy = 1.5f;
			}
			if (flag & DAMAGEF16_TOFRONT) {
				piece -> dx += PRANDFloatPos();
			}
			piece -> dy += PRANDFloatPos();
			angle = -450.0f * DTR;
			break;

		case DAMAGEF16_RSTAB:
			piece -> sfxflag |= SFX_BOUNCES;
			piece -> index = DAMAGEF16_RSTAB_SLOTINDEX;
			if (flag & DAMAGEF16_TOLEFT) {
				speed += 40.0f;
				piece -> dy = 0.75f;
			}
			else {
				speed += 50.0f;
				piece -> dy = 1.5f;
			}
			if (flag & DAMAGEF16_TOFRONT) {
				piece -> dx += PRANDFloatPos();
			}
			piece -> dy += PRANDFloatPos();
			angle = 450.0f * DTR;
			break;

		default:
			angle = 0.0F;
		break;
	}

	speed += 50.0f * PRANDFloatPos();

	angle1 = 180.0f * DTR;
	angle2 = 360.0f * DTR;
	float s = speed * 0.1f;
	if (s > 8.0f) s = 8.0f;
	s = 1.0f / (9.0f - s);
	angle *= s;
	angle1 *= s;
	angle2 *= s;
	if (flag & DAMAGEF16_TOLEFTRIGHT) {
		if (flag & DAMAGEF16_TOFRONT) {
			angle *= 2.5f;
			angle1 *= 2.0f;
			angle2 *= 1.5f;
		}
		if (flag & DAMAGEF16_TOLEFT) {
			angle = -angle;
			angle1 = -angle1;
			angle2 = -angle2;
		}
	}
	piece -> yd = angle;
	piece -> pd = angle1;
	piece -> rd = angle2;

	piece -> dx *= XDelta();
	piece -> dy *= YDelta();
	piece -> dz *= ZDelta();

	if ( this != SimDriver.playerEntity ) {
		piece -> sfxflag &= ~SFX_F16CRASH_OBJECT;
	}

	return 1;
}

int AircraftClass::CreateDamageF16Piece (DamageF16PieceStructure *piece, int *mask)
{
	float p = (float)asin(dmx[0][2]);
	if (p > 70.0f*DTR) return 0;	

	float r = (float)atan2(dmx[1][2], dmx[2][2]);
	float range = 5.0f * DTR;
	float range1 = 15.0f * DTR;
	float range2 = 50.0f * DTR;
	int	damagetype = 0;

	if (r > range) {
		damagetype |= 0x2;		// right
		if (r > range2) damagetype |= 0x10;
	}
	else if (r < -range) {
		damagetype |= 0x1;		// left
		if (r < -range2) damagetype |= 0x10;
	}

	if (p > range) {
		if (p > range2) damagetype |= 0x1c;
		else if (p > range1) damagetype |= 0x8;
		else damagetype |= 0x4;
	}
	float speed = (float)sqrt(XDelta()*XDelta()+YDelta()*YDelta()+ZDelta()*ZDelta());
	if (damagetype) {
		if (speed > 200.0f && speed < 500.0f) {
			if (damagetype & 0x4) damagetype |= 8;
		}
		else if (speed > 500.0f) damagetype |= 0x10;
	}

	int dirflag = DAMAGEF16_TOFRONT;
	int	numpiece = 0;
	if (damagetype & 0x10) 
	{			// full damage
		if (damagetype & 0xc) {			// damage to the front
			dirflag = DAMAGEF16_TOFRONT;
			numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_LWING, dirflag, *mask, speed);
			numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_LSTAB, dirflag, *mask, speed);
			numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_RWING, dirflag, *mask, speed);
			numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_RSTAB, dirflag, *mask, speed);
			numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_NOSE,  dirflag, *mask, speed);
			numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_FRONT, dirflag, *mask, speed);
			numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_BACK,  dirflag, *mask, speed);
			*mask &= DAMAGEF16_BACK;
		}
		else if (damagetype & 0x3)
		{	// damage to the left or right
			if (damagetype & 0x1)
				dirflag = DAMAGEF16_TOLEFT;
			else
				dirflag = DAMAGEF16_TORIGHT;

			numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_LWING, dirflag, *mask, speed);
			numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_LSTAB, dirflag, *mask, speed);
			numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_RWING, dirflag, *mask, speed);
			numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_RSTAB, dirflag, *mask, speed);
			numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_ONLYBODY, dirflag, *mask, speed);
			*mask &= DAMAGEF16_ONLYBODY;
		}
	}
	else
	{
		if (damagetype & 0x1)
		{				// damage to the left
			dirflag = DAMAGEF16_TOLEFT;
			if (damagetype & 0x4) {			// damage to the nose
				dirflag |= DAMAGEF16_TOFRONT;
				numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_LWING, dirflag, *mask, speed);
				numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_LSTAB, dirflag, *mask, speed);
				numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_NOSE, dirflag, *mask, speed);
				numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_NOLEFTANDNOSE, dirflag, *mask, speed);
				*mask &= DAMAGEF16_NOLEFTANDNOSE;
			}
			else if (damagetype & 0x8) {	// damage to the front
				dirflag |= DAMAGEF16_TOFRONT;
				numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_LWING, dirflag, *mask, speed);
				numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_LSTAB, dirflag, *mask, speed);
				numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_NOSE, dirflag, *mask, speed);
				numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_FRONT, dirflag, *mask, speed);
				numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_BACKWITHRIGHT, dirflag, *mask, speed);
				*mask &= DAMAGEF16_BACKWITHRIGHT;
			}
			else {
				numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_LWING, dirflag, *mask, speed);
				numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_LSTAB, dirflag, *mask, speed);
				numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_NOLEFT, dirflag, *mask, speed);
				*mask &= DAMAGEF16_NOLEFT;
			}
		}
		else if (damagetype & 0x2)
		{		// damage to the right
			dirflag = DAMAGEF16_TORIGHT;
			if (damagetype & 0x4) {			// damage to the nose
				dirflag |= DAMAGEF16_TOFRONT;
				numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_RWING, dirflag, *mask, speed);
				numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_RSTAB, dirflag, *mask, speed);
				numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_NOSE, dirflag, *mask, speed);
				numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_NORIGHTANDNOSE, dirflag, *mask, speed);
				*mask &= DAMAGEF16_NORIGHTANDNOSE;
			}
			else if (damagetype & 0x8) {	// damage to the front
				dirflag |= DAMAGEF16_TOFRONT;
				numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_RWING, dirflag, *mask, speed);
				numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_RSTAB, dirflag, *mask, speed);
				numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_NOSE, dirflag, *mask, speed);
				numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_FRONT, dirflag, *mask, speed);
				numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_BACKWITHLEFT, dirflag, *mask, speed);
				*mask &= DAMAGEF16_BACKWITHLEFT;
			}
			else {
				numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_RWING, dirflag, *mask, speed);
				numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_RSTAB, dirflag, *mask, speed);
				numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_NORIGHT, dirflag, *mask, speed);
				*mask &= DAMAGEF16_NORIGHT;
			}
		}
		else if (damagetype & 0x4)
		{		// damage to the nose
			dirflag = DAMAGEF16_TOFRONT;
			numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_NOSE, dirflag, *mask, speed);
			numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_NONOSE, dirflag, *mask, speed);
			*mask &= DAMAGEF16_NONOSE;
		}
		else if (damagetype & 0x8)
		{		// damage to the front
			dirflag = DAMAGEF16_TOFRONT;
			numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_NOSE, dirflag, *mask, speed);
			numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_FRONT, dirflag, *mask, speed);
			numpiece += SetDamageF16PieceType (&(piece[numpiece]), DAMAGEF16_BACKWITHWING, dirflag, *mask, speed);
			*mask &= DAMAGEF16_BACKWITHWING;
		}
	}

	if (!numpiece) {	// if no damage, randomized damage parts
		int i = DAMAGEF16_BACK;
		if (p > 0.0f) {
			if (PRANDFloatPos() > 0.5f) {
				dirflag |= DAMAGEF16_TOFRONT;
				i |= DAMAGEF16_FRONT;
				if (PRANDFloatPos() > 0.5f) {
					i |= DAMAGEF16_NOSE;
				}
			}
		}
		if (r > 0.0f) {
			if (PRANDFloatPos() > 0.5f) {
				dirflag |= DAMAGEF16_TORIGHT;
				i |= DAMAGEF16_LWING | DAMAGEF16_LSTAB;
			}
			else i |= DAMAGEF16_RWING | DAMAGEF16_RSTAB;
		}
		else {
			if (PRANDFloatPos() > 0.5f) {
				dirflag |= DAMAGEF16_TOLEFT;
				i |= DAMAGEF16_RWING | DAMAGEF16_RSTAB;
			}
			else i |= DAMAGEF16_LWING | DAMAGEF16_LSTAB;
		}
		int j = i ^ 0x7f;
		int k;
		int l = 1;
		for (k=0; k < 7; k++) {
			if (l & j) {
				numpiece += SetDamageF16PieceType (&(piece[numpiece]), l, dirflag, *mask, speed);
			}
			l <<= 1;
		}
		numpiece += SetDamageF16PieceType (&(piece[numpiece]), i, dirflag, *mask, speed);
		*mask &= i;
	}
	return numpiece;
}

void AircraftClass::SetupDamageF16Effects (DamageF16PieceStructure *piece)
{
	Tpoint slot;
	Tpoint piececenter;
	piececenter.x = XPos();
	piececenter.y = YPos();
	piececenter.z = ZPos();

	SimBaseClass *tmpSimBase = new SimBaseClass(Type());
	tmpSimBase -> SetYPR (Yaw(), 0.0f, 0.0f);
	CalcTransformMatrix (tmpSimBase);
	OTWDriver.CreateVisualObject(tmpSimBase, piece->damage, &piececenter, (Trotation *) &tmpSimBase->dmx, OTWDriver.Scale());

	DrawableBSP *ptr = (DrawableBSP *) tmpSimBase -> drawPointer;
	ptr -> SetSwitchMask(DAMAGEF16_SWITCH, piece->mask);
	if (piece -> mask & DAMAGEF16_BACK) {
		if (piece -> mask & DAMAGEF16_FRONT) {
			if (!(piece -> mask & DAMAGEF16_NOSE)) {
				ptr -> SetSwitchMask(DAMAGEF16_NOSEBREAK_SWITCH, 1);
			}
//			ptr -> SetSwitchMask(DAMAGEF16_CANOPYBREAK_SWITCH, 1);
		}
		else {
			ptr -> SetSwitchMask(DAMAGEF16_FRONTBREAK_SWITCH, 1);
		}
		if (!(piece -> mask & DAMAGEF16_RWING)) {
			ptr -> SetSwitchMask(DAMAGEF16_RWINGBREAK_SWITCH, 1);
		}
		if (!(piece -> mask & DAMAGEF16_LWING)) {
			ptr -> SetSwitchMask(DAMAGEF16_LWINGBREAK_SWITCH, 1);
		}
	}
	else if (piece -> mask & DAMAGEF16_FRONT) {
		if (!(piece -> mask & DAMAGEF16_NOSE)) {
			ptr -> SetSwitchMask(DAMAGEF16_NOSEBREAK_SWITCH, 1);
		}
		ptr -> SetSwitchMask(DAMAGEF16_FRONTBREAK_SWITCH, 1);
//		ptr -> SetSwitchMask(DAMAGEF16_CANOPYBREAK_SWITCH, 1);
	}
	else if (piece -> mask & DAMAGEF16_NOSE) {
		ptr -> SetSwitchMask(DAMAGEF16_NOSEBREAK_SWITCH, 1);
	}

	ptr -> GetChildOffset(piece->index, &slot);
	slot.x = -slot.x;
	slot.y = -slot.y;
	slot.z = -slot.z;

	tmpSimBase -> SetPosition (piececenter.x, piececenter.y, piececenter.z);
	tmpSimBase->SetDelta (piece->dx, piece->dy, piece->dz);
	tmpSimBase->SetYPRDelta (piece->yd, piece->pd, piece->rd);
	OTWDriver.AddSfxRequest( new SfxClass(
										piece->sfxtype, piece->sfxflag, tmpSimBase, 
										piece->lifetime, OTWDriver.Scale(),
										&slot, piece -> pitch, piece -> roll
										  )
							);
}

int AircraftClass::CreateDamageF16Effects ()
{
	if (!IsF16()) return 0;

	float groundZ = OTWDriver.GetApproxGroundLevel (XPos(), YPos());
	if (  ZPos() - groundZ  < -500.0f ) return 0;	// not ground explosion

	DamageF16PieceStructure piece[7];
	int	i = DAMAGEF16_ALL;
	int numpiece = CreateDamageF16Piece (piece, &i);
	if (!numpiece) return 0;

	for (i=0; i < numpiece; i++) 
		SetupDamageF16Effects (&(piece[i]));

	return 1;
}
//////////////////////////////////////



void AircraftClass::RunExplosion (void)
{
	int				i;
	Tpoint			pos;
	SimBaseClass	*tmpSimBase;
	Falcon4EntityClassType *classPtr;
	Tpoint			tpo = Origin;
	Trotation		tpim = IMatrix;

    // F4PlaySound (SFX_DEF[SFX_OWNSHIP_BOOM].handle);
	F4SoundFXSetPos( SFX_BOOMA1 + PRANDInt5(), TRUE, XPos(), YPos(), ZPos(), 1.0f );


////////////////////
	if ( this == SimDriver.playerEntity ) {
		float az = (float)atan2 (dmx[0][1], dmx[0][0]);
		OTWDriver.SetChaseAzEl(az, 0.0f);
	}
	if (CreateDamageF16Effects ()) return;
////////////////////


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

	// Add the parts (appairently hardcoded at 4)
	// Recoded by KCK on 6/23 to remove damage station BS
	// KCK NOTE: Why are we creating SimBaseClass entities here?
	// Can't we just pass the stinking drawable object?
	// Ed seems to think not, if we want to keep the rotation deltas.
	for (i=0; i<4; i++)
		{
		tmpSimBase = new SimBaseClass(Type());
		classPtr = &Falcon4ClassTable[Type()-VU_LAST_ENTITY_TYPE];
		CalcTransformMatrix (tmpSimBase);
		OTWDriver.CreateVisualObject(tmpSimBase, classPtr->visType[i+2], &tpo, &tpim, OTWDriver.Scale());
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

void	AircraftClass::AddEngineTrails(int ttype, DrawableTrail **tlist)
{
	// 2002-02-16 ADDED BY S.G. It's been seen that drawPointer is NULL here and &((DrawableBSP*)drawPointer)->orientation is simply drawPointer+0x2C hence why orientation is never NULL
	if (!drawPointer)
		return;

	Tpoint pos;
	Trotation *orientation = &((DrawableBSP*)drawPointer)->orientation;

	ShiAssert(orientation);
	if (!orientation)
		return;

	int nEngines = min(MAXENGINES, af->auxaeroData->nEngines);
	for(int i = 0; i < nEngines; i++) {
		if(tlist[i] == NULL) {
			tlist[i] = new DrawableTrail(ttype);
			OTWDriver.InsertObject(tlist[i]);
		}
		Tpoint *tp = &af->auxaeroData->engineLocation[i];

		ShiAssert(tp)
		if (!tp)
			return;

		pos.x = orientation->M11*tp->x + orientation->M12*tp->y + orientation->M13*tp->z;
		pos.y = orientation->M21*tp->x + orientation->M22*tp->y + orientation->M23*tp->z;
		pos.z = orientation->M31*tp->x + orientation->M32*tp->y + orientation->M33*tp->z;

		pos.x += XPos();
		pos.y += YPos();
		pos.z += ZPos();

		OTWDriver.AddTrailHead (tlist[i], pos.x, pos.y, pos.z );
		tlist[i]->KeepStaleSegs (flag_keep_smoke_trails);
	}
}

void AircraftClass::CancelEngineTrails(DrawableTrail **tlist)
{
    int nEngines = min(MAXENGINES, af->auxaeroData->nEngines);
    for(int i = 0; i < nEngines; i++) {
	if (tlist[i]) {
	    OTWDriver.AddSfxRequest(
		new SfxClass (
		21.2f,							// time to live
		tlist[i]) );		// scale
	    tlist[i] = NULL;
	}
    }
}

void AircraftClass::ShowDamage (void)
{
   BOOL hasMilSmoke = FALSE;
   float radius;
   Tpoint pos, rearOffset;

   // handle case when moving slow and/or on ground -- no trails
   if ( Vt() < 40.0f )
   {
       for (int i = 0; i < TRAIL_MAX; i++) {
	   if ( smokeTrail[i] )
	   {
	       OTWDriver.AddSfxRequest(
		   new SfxClass (
		   15.2f,							// time to live
		   smokeTrail[i]) );		// scale
	       smokeTrail[i] = NULL;
	   }
       }
       CancelEngineTrails(conTrails);
       CancelEngineTrails(engineTrails);

       // no do puffy smoke if damaged
	   if ( pctStrength < 0.50f )
	   {
			rearOffset.x = PRANDFloat() * 20.0f;
			rearOffset.y = PRANDFloat() * 20.0f;
			rearOffset.z = -PRANDFloatPos() * 20.0f;

	   		pos.x = XPos();
	   		pos.y = YPos();
	   		pos.z = ZPos();

			OTWDriver.AddSfxRequest(
					new SfxClass (SFX_TRAILSMOKE,			// type
					SFX_MOVES | SFX_NO_GROUND_CHECK,						// flags
					&pos,							// world pos
					&rearOffset,							// vector
					3.5f,							// time to live
					4.5f ));		// scale
	   }
	   return;
   }

   // mig29's, f4's and f5's all smoke when at MIL power
   // it's not damage, but since we handle the smoke here anyway.....

   if (!OnGround() &&
       af->EngineTrail() >= 0)
   {
       if ( OTWDriver.renderer && OTWDriver.renderer->GetAlphaMode() )
	   hasMilSmoke = af->EngineTrail();
   }

   // get rear offset behind the plane
   if ( drawPointer )
   		radius = drawPointer->Radius();
   else
   		radius = 25.0f;
   rearOffset.x = -dmx[0][0]*radius;
   rearOffset.y = -dmx[0][1]*radius;
   rearOffset.z = -dmx[0][2]*radius;

   // check for contrail alt
   // also this can be toggled by the player
   if ((ZPos()*-0.001F > ((WeatherClass*)TheWeather)->TodaysConLow &&
        ZPos()*-0.001F < ((WeatherClass*)TheWeather)->TodaysConHigh) || playerSmokeOn )
   {
       AddEngineTrails(TRAIL_CONTRAIL, conTrails); // Whitish smoke
   }
   else 
   {
       CancelEngineTrails(conTrails);
   }

   // JPO - maybe some wing tip vortexes
   if ( af->auxaeroData->wingTipLocation.y != 0 &&
       OTWDriver.renderer && OTWDriver.renderer->GetAlphaMode() &&
       -ZPos() > minwingvortexalt && -ZPos() < maxwingvortexalt &&
       af->alpha > wingvortexalpha && af->nzcgb > wingvortexgs) 
	{
		Tpoint pos;
		Trotation *orientation = &((DrawableBSP*)drawPointer)->orientation;

		ShiAssert(orientation);
		if (orientation)
		{
			if(lwingvortex == NULL) 
			{
				lwingvortex = new DrawableTrail(TRAIL_WINGTIPVTX);
				OTWDriver.InsertObject(lwingvortex);
			}

			Tpoint *tp = &af->auxaeroData->wingTipLocation;
			pos.x = orientation->M11*tp->x + orientation->M12*tp->y + orientation->M13*tp->z;
			pos.y = orientation->M21*tp->x + orientation->M22*tp->y + orientation->M23*tp->z;
			pos.z = orientation->M31*tp->x + orientation->M32*tp->y + orientation->M33*tp->z;

			pos.x += XPos();
			pos.y += YPos();
			pos.z += ZPos();

			OTWDriver.AddTrailHead (lwingvortex, pos.x, pos.y, pos.z );

			if(rwingvortex == NULL)
			{
				rwingvortex = new DrawableTrail(TRAIL_WINGTIPVTX);
				OTWDriver.InsertObject(rwingvortex);
			}

			tp = &af->auxaeroData->wingTipLocation;
			pos.x = orientation->M11*tp->x + orientation->M12*-tp->y + orientation->M13*tp->z;
			pos.y = orientation->M21*tp->x + orientation->M22*-tp->y + orientation->M23*tp->z;
			pos.z = orientation->M31*tp->x + orientation->M32*-tp->y + orientation->M33*tp->z;

			pos.x += XPos();
			pos.y += YPos();
			pos.z += ZPos();

			OTWDriver.AddTrailHead (rwingvortex, pos.x, pos.y, pos.z );
		}
	}
   else {
       	if (lwingvortex) {
	    OTWDriver.AddSfxRequest(
		new SfxClass (
		21.2f,			// time to live
		lwingvortex) );		// scale
	    lwingvortex = NULL;
	}

       	if (rwingvortex) {
	    OTWDriver.AddSfxRequest(
		new SfxClass (
		21.2f,			// time to live
		rwingvortex) );		// scale
	    rwingvortex = NULL;
	}
   }

   // do military power smoke if its that type of craft
   // PowerOutput() runs from 0.7 (flight idle) to 1.5 (max ab) with 1.0 being mil power
   // M.N. added Poweroutput > 0.65, stops smoke trails when engine is shut down.
   if ( !OnGround() && af->EngineTrail() >= 0 && OTWDriver.renderer && OTWDriver.renderer->GetAlphaMode()) {
       if (PowerOutput() <= 1.0f && PowerOutput() > 0.65f)
       {
	   AddEngineTrails(af->EngineTrail(), engineTrails); // smoke
       }
       else CancelEngineTrails(engineTrails);
   }
	if ( pctStrength > 0.50f )
	{
		if (!hasMilSmoke && smokeTrail[TRAIL_DAMAGE] && smokeTrail[TRAIL_DAMAGE]->InDisplayList() )
		{
			OTWDriver.AddSfxRequest(
			new SfxClass (
			11.2f,							// time to live
			smokeTrail[TRAIL_DAMAGE]) );		// scale
			smokeTrail[TRAIL_DAMAGE] = NULL;
		}

		return;
	}

   // if we're dying, just maitain the status quo...
   if ( pctStrength < 0.0f )
   {
	    radius = 3.0f;

	    if ( smokeTrail[TRAIL_DAMAGE] )
		{
		   pos.x = XPos() + rearOffset.x;
		   pos.y = YPos() + rearOffset.y;
		   pos.z = ZPos() + rearOffset.z;
		   OTWDriver.AddTrailHead (smokeTrail[TRAIL_DAMAGE], pos.x, pos.y, pos.z );
		}

	    if ( smokeTrail[TRAIL_ENGINE1] )
		{
	   		pos.x = dmx[1][0]*radius + XPos() + rearOffset.x * 0.75f;
	   		pos.y = dmx[1][1]*radius + YPos() + rearOffset.y * 0.75f;
	   		pos.z = dmx[1][2]*radius + ZPos() + rearOffset.z * 0.75f;
		    OTWDriver.AddTrailHead (smokeTrail[TRAIL_ENGINE1], pos.x, pos.y, pos.z );
		}

	    if ( smokeTrail[TRAIL_ENGINE2] )
		{
	   		pos.x = -dmx[1][0]*radius + XPos() + rearOffset.x * 0.75f;
	   		pos.y = -dmx[1][1]*radius + YPos() + rearOffset.y * 0.75f;
	   		pos.z = -dmx[1][2]*radius + ZPos() + rearOffset.z * 0.75f;
		    OTWDriver.AddTrailHead (smokeTrail[TRAIL_ENGINE2], pos.x, pos.y, pos.z );
		}
	    return;
   }

   // at this point we know we've got enough damage for 1 trail in
   // the center
   if (!smokeTrail[TRAIL_DAMAGE])
   {
		smokeTrail[TRAIL_DAMAGE] = new DrawableTrail(TRAIL_SMOKE);	// smoke
		OTWDriver.InsertObject(smokeTrail[TRAIL_DAMAGE]);
   }

   pos.x = XPos() + rearOffset.x;
   pos.y = YPos() + rearOffset.y;
   pos.z = ZPos() + rearOffset.z;
   OTWDriver.AddTrailHead (smokeTrail[TRAIL_DAMAGE], pos.x, pos.y, pos.z );

   // occasionalyy add a smoke cloud
   /*
   if ( sfxTimer > 0.2f && (rand() & 0x3) == 0x3 )
   {
	   sfxTimer = 0.0f;
	   pos.x = XPos();
	   pos.y = YPos();
	   pos.z = ZPos();
	   OTWDriver.AddSfxRequest(
		   	new SfxClass (SFX_TRAILSMOKE,				// type
			&pos,							// world pos
			2.5f,							// time to live
			2.0f ) );		// scale
   }
   */

   // now test for additional damage and add smoke trails on the left and right
   if ( pctStrength < 0.35f )
   {
	   radius = 3.0f;

	   pos.x = dmx[1][0]*radius + XPos() + rearOffset.x * 0.75f;
	   pos.y = dmx[1][1]*radius + YPos() + rearOffset.y * 0.75f;
	   pos.z = dmx[1][2]*radius + ZPos() + rearOffset.z * 0.75f;

	   if (!smokeTrail[TRAIL_ENGINE1])
	   {
			smokeTrail[TRAIL_ENGINE1] = new DrawableTrail(TRAIL_SMOKE);	// smoke
			OTWDriver.InsertObject(smokeTrail[TRAIL_ENGINE1]);
	   }
	   OTWDriver.AddTrailHead (smokeTrail[TRAIL_ENGINE1],  pos.x, pos.y, pos.z );
	
	   // occasionalyy add a smoke cloud
	   /*
	   if ( sfxTimer > 0.2f && (rand() & 0x3) == 0x3 )
	   {
		   sfxTimer = 0.0f;
		   OTWDriver.AddSfxRequest(
				new SfxClass (SFX_TRAILSMOKE,				// type
				&pos,							// world pos
				2.5f,							// time to live
				2.0f ) );		// scale
	   }
	   */
   }

   if ( pctStrength < 0.15f )
   {
		 pos.x = -dmx[1][0]*radius + XPos() + rearOffset.x * 0.75f;
		 pos.y = -dmx[1][1]*radius + YPos() + rearOffset.y * 0.75f;
		 pos.z = -dmx[1][2]*radius + ZPos() + rearOffset.z * 0.75f;

		 if (!smokeTrail[TRAIL_ENGINE2])
		 {
			smokeTrail[TRAIL_ENGINE2] = new DrawableTrail(TRAIL_SMOKE);	// smoke
			OTWDriver.InsertObject(smokeTrail[TRAIL_ENGINE2]);
		 }
		 OTWDriver.AddTrailHead (smokeTrail[TRAIL_ENGINE2],  pos.x, pos.y, pos.z );

	   // occasionalyy add a smoke cloud
	   /*
	   if ( sfxTimer > 0.2f && (rand() & 0x3) == 0x3 )
	   {
		   sfxTimer = 0.0f;
		   OTWDriver.AddSfxRequest(
				new SfxClass (SFX_TRAILSMOKE,				// type
				&pos,							// world pos
				2.5f,							// time to live
				2.0f ) );		// scale
	   }
	   */
   }

   // occasionally, perturb the controls
	 // JB 010104
   //if ( pctStrength < 0.5f && (rand() & 0xf) == 0xf)
	 if (!g_bDisableFunkyChicken && pctStrength < 0.5f && (rand() & 0xf) == 0xf)
	 // JB 010104
   {
		  ioPerturb = 0.5f + ( 1.0f - pctStrength );
		  // good place also to stick in a damage, clunky sound....
   }

}


void AircraftClass::CleanupDamage (void)
{
    for (int i = 0; i < TRAIL_MAX; i++) {
        if ( smokeTrail[i] && smokeTrail[i]->InDisplayList() )
	{
		OTWDriver.AddSfxRequest(
			new SfxClass (
			11.2f,							// time to live
			smokeTrail[i]) );		// scale
		smokeTrail[i] = NULL;
	}
    }
    return;
}


/*
** Name: CheckObjectCollision
** Description:
**		Crawls thru the target list for the aircraft.  Checks the range
**		to the target and compares it with the radii of both objects as well
**		as the rangedot.  if rangedot is moving closer and range is less
**		than the sum of the 2 radii, damage messages are sent to both.
**
**		A caveat, since we're using targetList, is that non-ownships only
**		detect collisions with the enemy.
*/
void
AircraftClass::CheckObjectCollision (void)
{
	SimObjectType *obj;
	SimBaseClass *theObject;
	SimObjectLocalData *objData;
	FalconDamageMessage *message;
	bool setOnObject = false; // JB carrier

	// no detection on ground when not moving
	if ( 
		!af->IsSet(AirframeClass::OnObject) && // JB carrier
		OnGround() && af->vt == 0.0f )
	{
		return;
	}

	if (
		!af->IsSet(AirframeClass::OnObject) && // JB carrier
		OnGround() && af->vcas <= 50.0f  && gCommsMgr && gCommsMgr->Online()) // JB 010107
	{
		return; // JB 010107
	}

	// loop thru all targets
    for( obj = targetList; obj; obj = obj->next )
    {
      	objData = obj->localData;
		   theObject = (SimBaseClass*)obj->BaseData();

		// Throw out inappropriate collision partners
		// edg: allow collisions with ground objects
      	// if ( theObject == NULL || theObject->OnGround() || this == theObject->Parent() )
      	if (theObject == NULL ||
					F4IsBadReadPtr(theObject, sizeof(SimBaseClass)) || // JB 010305 CTD
			(theObject->IsWeapon() && this == ((SimWeaponClass*)theObject)->Parent()) ||	//	-> Why would a missile's parent be in it's target list?
                                                                  // It typically wouldn't, but then, this is not the missiles target list.
			!theObject->IsSim() ||
			(OnGround() && !theObject->OnGround()) ||
			theObject->drawPointer == NULL )
      	{
         	continue;
      	}

		// Stop now if the spheres don't overlap
		// special case the tanker -- we want to be able to get in closer
		if ( 
			af->IsSet(AirframeClass::OnObject) || // JB carrier
			!OnGround() )
		{
			if ( IsSetFlag( I_AM_A_TANKER ) || theObject->IsSetFlag( I_AM_A_TANKER ) )
			{
				/*
				if ( objData->range >  0.1f * theObject->drawPointer->Radius()){// + drawPointer->Radius() ) // PJW
					continue;
				}
				*/
				
				if ( objData->range > 0.2F * theObject->drawPointer->Radius() + drawPointer->Radius() ) 
					continue;

				Tpoint org, vec, pos;

				org.x = XPos();
				org.y = YPos();
				org.z = ZPos();
				vec.x = XDelta() - theObject->XDelta();
				vec.y = YDelta() - theObject->YDelta();
				vec.z = ZDelta() - theObject->ZDelta();
				// we're within the range of the object's radius.
				// for ships, this may be a BIG radius -- longer than
				// high.  Let's also check the bounding box
				// scale the box so we might possibly miss it
				if ( !theObject->drawPointer->GetRayHit( &org, &vec, &pos, 0.1f ) )
					continue;
				

 			} else
			{
				if ( objData->range > theObject->drawPointer->Radius() + drawPointer->Radius())
				{
					continue;
				}
				//else if ( theObject->OnGround() )
				//{
					Tpoint org, vec, pos;

					org.x = XPos();
					org.y = YPos();
					if (af->IsSet(AirframeClass::OnObject)) // JB carrier
						org.z = ZPos() + 5; // Make sure its detecting that we are on top // JB carrier
					else // JB carrier
						org.z = ZPos();
					vec.x = XDelta() - theObject->XDelta();
					vec.y = YDelta() - theObject->YDelta();
					vec.z = ZDelta() - theObject->ZDelta();
					// we're within the range of the object's radius.
					// for ships, this may be a BIG radius -- longer than
					// high.  Let's also check the bounding box
				    if ( !theObject->drawPointer->GetRayHit( &org, &vec, &pos, 1.0f ) )
						continue;
				//}
			}
		}
		else // on ground
		{
			if ( objData->range > theObject->drawPointer->Radius()){// + drawPointer->Radius() ) { // PJW
					continue;
			}
		}

		// Don't collide ejecting pilots with their aircraft
		if (theObject->IsEject())
		{
			if (((EjectedPilotClass*)theObject)->GetParentAircraft() == this)
				continue;
		}

		//***********************************************
		// If we get here, we've decided we've collided!
		//***********************************************

		// JB carrier start
		if (IsAirplane() && theObject->GetType() == TYPE_CAPITAL_SHIP)
		{
			Tpoint minB; Tpoint maxB;
			((DrawableBSP*) theObject->drawPointer)->GetBoundingBox(&minB, &maxB);
			
			// JB 010731 Hack for unfixed hitboxes.
			if (minB.z < -193 && minB.z > -194)
				minB.z = -72;

			if((ZPos() <= minB.z * .96 && ZPos() > minB.z * 1.02) || (ZPos() > -g_fCarrierStartTolerance && af->vcas < .01))
			{	
				// the eagle has landed
				attachedEntity = theObject;
				af->SetFlag(AirframeClass::OnObject);
				af->SetFlag(AirframeClass::OverRunway);
				setOnObject = true;
				
				// Set our anchor so that when we're moving slowly we can accumulate our position in high precision
				af->groundAnchorX = af->x;
				af->groundAnchorY = af->y;
				af->groundDeltaX = 0.0f;
				af->groundDeltaY = 0.0f;
				af->platform->SetFlag( ON_GROUND );
				
				float gndGmma, relMu;
				
				af->CalculateGroundPlane(&gndGmma, &relMu);
				
				af->stallMode = AirframeClass::None;
				af->slice = 0.0F;
				af->pitch = 0.0F;

				if( af->IsSet(af->GearBroken) || af->gearPos <= 0.1F )
				{
					if ( af->platform->DBrain() && !af->platform->IsSetFalcFlag(FEC_INVULNERABLE))
					{
						af->platform->DBrain()->SetATCFlag(DigitalBrain::Landed);
						af->platform->DBrain()->SetATCStatus(lCrashed);
						// KCK NOTE:: Don't set timer for players
						if(af->platform != SimDriver.playerEntity)
							af->platform->DBrain()->SetWaitTimer(SimLibElapsedTime + 1 * CampaignMinutes);
					}
				}

				Tpoint velocity;
				Tpoint noseDir;
				float impactAngle, noseAngle;
				float tmp;

				velocity.x = af->xdot/vt;
				velocity.y = af->ydot/vt;
				velocity.z = af->zdot/vt;
				
				noseDir.x = af->platform->platformAngles->costhe * af->platform->platformAngles->cospsi;
				noseDir.y = af->platform->platformAngles->costhe * af->platform->platformAngles->sinpsi;
				noseDir.z = -af->platform->platformAngles->sinthe;
				tmp = (float)sqrt(noseDir.x*noseDir.x + noseDir.y*noseDir.y + noseDir.z*noseDir.z);
				noseDir.x /= tmp;
				noseDir.y /= tmp;
				noseDir.z /= tmp;
				
				noseAngle = af->gndNormal.x*noseDir.x + af->gndNormal.y*noseDir.y + af->gndNormal.z*noseDir.z;
				impactAngle = af->gndNormal.x*velocity.x + af->gndNormal.y*velocity.y + af->gndNormal.z*velocity.z;
				
				impactAngle = (float)fabs(impactAngle);

				if (ZPos() > -g_fCarrierStartTolerance)
				{
					// We just started inside the carrier
					SetAutopilot(APOff);
					af->onObjectHeight = minB.z;
					noseAngle = 0;
					impactAngle = 0;
				}
				else
					af->onObjectHeight = ZPos();

				if (vt == 0.0)
					continue;

				// do the landing check (no damage)
				if (!af->IsSet(AirframeClass::InAir) || af->platform->LandingCheck( noseAngle, impactAngle, COVERAGE_OBJECT) )
				{
					af->ClearFlag (AirframeClass::InAir);
					continue;
				}
			}
			else if(ZPos() <= minB.z * .96)
				continue;
		}

		if (isDigital || !PlayerOptions.CollisionsOn())
			continue;
		// JB carrier end

// 2002-04-17 MN fix for killer chaff / flare
		if (theObject->GetType() == TYPE_BOMB && 
			(theObject->GetSType() == STYPE_CHAFF || theObject->GetSType() == STYPE_FLARE1) &&
			(theObject->GetSPType() == SPTYPE_CHAFF1 || theObject->GetSPType() == SPTYPE_CHAFF1+1))
			continue;
		
		if (!isDigital)
			g_intellivibeData.CollisionCounter++;

		// send message to self
		// VuTargetEntity *owner_session = (VuTargetEntity*)vuDatabase->Find(OwnerId());
		message = new FalconDamageMessage (Id(), FalconLocalGame);
		message->dataBlock.fEntityID  = theObject->Id();

        message->dataBlock.fCampID    = theObject->GetCampID();
        message->dataBlock.fSide      = theObject->GetCountry();

		if (theObject->IsAirplane())
		   message->dataBlock.fPilotID   = ((SimMoverClass*)theObject)->pilotSlot;
		else
		   message->dataBlock.fPilotID   = 255;
		message->dataBlock.fIndex     = theObject->Type();
		message->dataBlock.fWeaponID  = theObject->Type();
		message->dataBlock.fWeaponUID = theObject->Id();

		message->dataBlock.dEntityID  = Id();
		ShiAssert(GetCampaignObject())
		message->dataBlock.dCampID = GetCampID();
		message->dataBlock.dSide   = GetCountry();
		if (IsAirplane())
		   message->dataBlock.dPilotID   = pilotSlot;
		else
		   message->dataBlock.dPilotID   = 255;
		message->dataBlock.dIndex     = Type();
		message->dataBlock.damageType = FalconDamageType::ObjectCollisionDamage;

		Tpoint Objvec, Myvec, relVec;
		float relVel;

		Myvec.x = XDelta();
		Myvec.y = YDelta();
		Myvec.z = ZDelta();

		Objvec.x = theObject->XDelta();
		Objvec.y = theObject->YDelta();
		Objvec.z = theObject->ZDelta();

		relVec.x = Myvec.x - Objvec.x;
		relVec.y = Myvec.y - Objvec.y;
		relVec.z = Myvec.z - Objvec.z;

		relVel = (float)sqrt(relVec.x*relVec.x + relVec.y*relVec.y + relVec.z*relVec.z);

		// for now use maxStrength as amount of damage.
		// later we'll want to add other factors into the equation --
		// on ground, speed, etc....	
		
		message->dataBlock.damageRandomFact = 1.0f;

		message->dataBlock.damageStrength = min(1000.0F, relVel * theObject->Mass() * 0.0001F + relVel * relVel * theObject->Mass()* 0.000002F);
	
		message->RequestOutOfBandTransmit ();

		if (message->dataBlock.damageStrength > 0.0f) // JB carrier
			FalconSendMessage (message,TRUE);

		// send message to other ship
		// owner_session = (VuTargetEntity*)vuDatabase->Find(theObject->OwnerId());
		message = new FalconDamageMessage (theObject->Id(), FalconLocalGame);
		message->dataBlock.fEntityID  = Id();
		ShiAssert(GetCampaignObject())
		message->dataBlock.fCampID = GetCampID();
		message->dataBlock.fSide   = GetCountry();
		message->dataBlock.fPilotID   = pilotSlot;
		message->dataBlock.fIndex     = Type();
		message->dataBlock.fWeaponID  = Type();
		message->dataBlock.fWeaponUID = theObject->Id();

		message->dataBlock.dEntityID  = theObject->Id();
		message->dataBlock.dCampID = theObject->GetCampID();
		message->dataBlock.dSide   = theObject->GetCountry();
		if (theObject->IsAirplane())
		   message->dataBlock.dPilotID   = ((SimMoverClass*)theObject)->pilotSlot;
		else
		   message->dataBlock.dPilotID   = 255;
		message->dataBlock.dIndex     = theObject->Type();
		// for now use maxStrength as amount of damage.
		// later we'll want to add other factors into the equation --
		// on ground, speed, etc....
		
		message->dataBlock.damageRandomFact = 1.0f;
		message->dataBlock.damageStrength = min(1000.0F, relVel * Mass() * 0.0001F + relVel * relVel * Mass()* 0.000002F);
				
		message->dataBlock.damageType = FalconDamageType::ObjectCollisionDamage;
		message->RequestOutOfBandTransmit ();
	
		if (message->dataBlock.damageStrength > 0.0f) // JB carrier
			FalconSendMessage (message,TRUE);
	} // end target list loop

	// JB carrier start
	if (!setOnObject && af->IsSet(AirframeClass::OnObject))
	{
		attachedEntity = NULL;
		af->ClearFlag(AirframeClass::OverRunway);
		af->ClearFlag(AirframeClass::OnObject);
		af->ClearFlag(AirframeClass::Planted);
		af->platform->UnSetFlag( ON_GROUND );
		af->SetFlag (AirframeClass::InAir);
	}
	// JB carrier end
}

//void AircraftClass::AddFault(int failures, unsigned int failuresPossible, int numToBreak, int sourceOctant)
void AircraftClass::AddFault(int failures, unsigned int failuresPossible, int, int)
{
    int i;
    
    //	failures = numToBreak * (float)rand() / (float)RAND_MAX;
    
    for(i = 0; i < failures; i++)
    {
	mFaults->SetFault(failuresPossible, !isDigital);
    }
    // JPO - break hydraulics occasionally
    if ((failuresPossible & FaultClass::eng_fault) &&
	(mFaults->GetFault(FaultClass::eng_fault) & FaultClass::hydr))  {
	if (rand() % 100 < 20) { // 20% failure chance of A system
	    af->HydrBreak (AirframeClass::HYDR_A_SYSTEM);
	}
	if (rand() % 100 < 20) { // 20% failure chance of B system
	    af->HydrBreak (AirframeClass::HYDR_B_SYSTEM);
	}
    }
    // also break the generators now and then
    if (failuresPossible & FaultClass::eng_fault) {
	if (rand() % 7 == 1)
	    af->GeneratorBreak(AirframeClass::GenStdby);
	if (rand() % 7 == 1)
	    af->GeneratorBreak(AirframeClass::GenMain);
    }
#if 0
int failedThing;
int i, j = 0;
int failedThings[FaultClass::NumFaultListSubSystems];
BOOL Found;
int canFail;
int numFunctions = 0;
int failedFunc;

	failures = numToBreak * (float)rand() / (float)RAND_MAX;

	for(i = 0; i < FaultClass::NumFaultListSubSystems; i++) {
      if(failuresPossible & (1 << i)) {
			failedThings[j] = i;
			j++;
		}
	}
   
	for(i = 0; i < failures; i++) {
		Found = FALSE;

		do {
			failedThing = j * (float)rand() / (float)RAND_MAX;
         numFunctions = 0;

			if(failedThings[failedThing] != -1) {
            // FLCS is a special case, as it has 1 informational fault which MUST happen first
            if ((FaultClass::type_FSubSystem)failedThings[failedThing] == FaultClass::flcs_fault &&
               !mFaults->GetFault(FaultClass::flcs_fault))
            {
               failedFunc = 1;
               numFunctions = -1;
               while (failedFunc)
               {
                  numFunctions ++;
                  if (FaultClass::sngl & (1 << numFunctions))
                     failedFunc --;
               }
            }
            else
            {
               canFail   = mFaults->Breakable((FaultClass::type_FSubSystem)failedThings[failedThing]);

               // How many functions?
               while (canFail)
               {
                  if (canFail & 0x1)
                     numFunctions ++;
                  canFail = canFail >> 1;
               }

               // pick 1 of canFail things
               failedFunc = numFunctions * (float)rand() / (float)RAND_MAX + 1;

               // Find that function
               canFail   = mFaults->Breakable((FaultClass::type_FSubSystem)failedThings[failedThing]);
               numFunctions = -1;
               while (failedFunc)
               {
                  numFunctions ++;
                  if (canFail & (1 << numFunctions))
                     failedFunc --;
               }
            }

				mFaults->SetFault((FaultClass::type_FSubSystem)failedThings[failedThing],
						(FaultClass::type_FFunction) (1 << numFunctions),	
						(FaultClass::type_FSeverity) FaultClass::fail,
						!isDigital);	// none, fail for now
	if (failedThings[failedThing] == FaultClass::eng_fault &&
		(1 << numFunctions) == hydr) {
		if (rand() % 100 < 20) { // 20% failure chance of A system
			af->HydrBreak (Airframe::HYDR_A_SYSTEM);
		}
		if (rand() % 100 < 20) { // 20% failure chance of B system
			af->HydrBreak (Airframe::HYDR_B_SYSTEM);
		}
	}

            MonoPrint ("Failed %s %s\n", FaultClass::mpFSubSystemNames[failedThings[failedThing]],
               FaultClass::mpFFunctionNames[numFunctions+1]);
				failedThings[failedThing] = -1;
				Found = TRUE;
			}
		}
		while(Found = FALSE);
	}
#endif
}

void AircraftClass::RunCautionChecks(void)
{
}
