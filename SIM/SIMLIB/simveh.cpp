#include "stdhdr.h"
#include "classtbl.h"
#include "entity.h"
#include "simveh.h"
#include "otwdrive.h"
#include "initdata.h"
#include "simbrain.h"
#include "PilotInputs.h"
#include "object.h"
#include "sms.h"
#include "fcc.h"
#include "guns.h"
#include "misslist.h"
#include "bombfunc.h"
#include "gunsfunc.h"
#include "simvudrv.h"
#include "hardpnt.h"
#include "missile.h"
#include "mavdisp.h"
#include "hud.h"
#include "sensclas.h"
#include "simdrive.h"
#include "fsound.h"
#include "soundfx.h"
#include "MsgInc\DamageMsg.h"
#include "MsgInc\DeathMessage.h"
#include "MsgInc\WeaponFireMsg.h"
#include "MsgInc\RadioChatterMsg.h"
#include "campbase.h"
#include "sfx.h"
#include "wpndef.h"
#include "fakerand.h"
#include "eyeball.h"
#include "radarDigi.h"
#include "VehRwr.h"
#include "irst.h"
#include "acmi\src\include\acmirec.h"
#include "Graphics\Include\drawbsp.h"
#include "Graphics\Include\rviewpnt.h"
#include "playerop.h"
#include "camp2sim.h"
#include "falcsess.h"
#include "airframe.h"
#include "aircrft.h"
#include "camplist.h"
#include "Graphics\Include\terrtex.h"
#include "camp2sim.h"
#include "airunit.h"
#include "rules.h"
#include "falcsnd\voicemanager.h"
#include "campweap.h"
#include "drawable.h"

extern SensorClass* FindLaserPod (SimMoverClass* theObject);
extern VehicleClassDataType* GetVehicleClassData (int index);
void GetCallsign (int id, int num, _TCHAR* callsign); // from campui

extern bool g_bNewDamageEffects;	// JB 000816
extern bool g_bDisableFunkyChicken; // JB 000820
//extern bool g_bHardCoreReal; //me123	MI replaced with g_bRealisticAvionics
extern bool g_bRealisticAvionics;
#include "ui\include\uicomms.h" // JB 010104
extern UIComms *gCommsMgr; // JB 010104

#ifdef DEBUG
int SimVehicles = 0;
#endif

SimVehicleClass::SimVehicleClass(FILE* filePtr) : SimMoverClass (filePtr)
{
   InitData();
#ifdef DEBUG
SimVehicles++;
#endif
}

SimVehicleClass::SimVehicleClass(VU_BYTE** stream) : SimMoverClass (stream)
{
   InitData();
#ifdef DEBUG
SimVehicles++;
#endif
}

SimVehicleClass::SimVehicleClass(int type) : SimMoverClass (type)
{
   InitData();
#ifdef DEBUG
SimVehicles++;
#endif
}

void SimVehicleClass::InitData (void)
{
Falcon4EntityClassType* classPtr = (Falcon4EntityClassType*)EntityType();
VehicleClassDataType *vc;

   vc = (VehicleClassDataType *)classPtr->dataPtr;

   strength = maxStrength = (float)vc->HitPoints;
   pctStrength = 1.0;
   dyingTimer = -1.0f;
   sfxTimer = 0.0f;
   ioPerturb = 0.0f;
   irOutput  = 0.0f;//me123
	 // JB 000814
	 rBias = 0.0f;
	 pBias = 0.0f;
	 yBias = 0.0f;
	 // JB 000814

   theBrain = NULL;
   theInputs = NULL;
   deathMessage = NULL;
}

void SimVehicleClass::Init (SimInitDataClass* initData)
{
int i;

	SimMoverClass::Init(initData);

   if (!IsAirplane())
   {
		// Create Sensors
		ShiAssert(mvrDefinition);
		if (mvrDefinition)
			numSensors = mvrDefinition->numSensors;
		else
			numSensors = 0;
		
	   // Note, until there is a vehicle definition for radar vehicles
	   // do it as a special case here and below.
	   if (IsGroundVehicle() && GetRadarType() != RDR_NO_RADAR)
		   sensorArray = new SensorClass*[numSensors+1];
	   else
		   sensorArray = new SensorClass*[numSensors];
	   for (i = 0; i<numSensors && mvrDefinition && mvrDefinition->sensorData; i++)
		   {
		   switch (mvrDefinition->sensorData[i*2])
			   {
			   case SensorClass::Radar:
				   sensorArray[i] = new RadarDigiClass (GetRadarType(), this);
				   break;

			   case SensorClass::RWR:
				   sensorArray[i] = new VehRwrClass (mvrDefinition->sensorData[i*2 + 1], this);
				   break;

			   case SensorClass::IRST:
				   sensorArray[i] = new IrstClass (mvrDefinition->sensorData[i*2 + 1], this);
				   break;

			   case SensorClass::Visual:
				   sensorArray[i] = new EyeballClass (mvrDefinition->sensorData[i*2 + 1], this);
				   break;
			   default:
				   ShiWarning( "Unhandled sensor type during Init" );
				   break;
			   }
		   }

	   // Note, until there is a vehicle definition for radar vehicles
	   // do it as a special case here and above.
	   if (IsGroundVehicle() && GetRadarType() != RDR_NO_RADAR)
		   {
		   numSensors ++;
		   sensorArray[i] = new RadarDigiClass (GetRadarType(), this);
		   }
   }
}

int SimVehicleClass::Wake (void)
{
//int retval = 0;

	if (IsAwake())
		return 0;

	return SimMoverClass::Wake();
}

int SimVehicleClass::Sleep (void)
{
int retval = 0;

	if (!IsAwake())
		return retval;

	SimMoverClass::Sleep();

	if (theBrain)
	{
		theBrain->Sleep();
	}

	return retval;
}
	

void SimVehicleClass::MakeLocal (void)
	{
	// Nothing particularly interesting here right now.
	SimMoverClass::MakeLocal();
	}	

void SimVehicleClass::MakeRemote (void)
	{
	// Nothing particularly interesting here right now.
	SimMoverClass::MakeRemote();
	}

WayPointClass *SimVehicleClass::GetWayPointNo(int n)
{
    WayPointClass *wp;
	//MI check it
	if(n <= 0)
		return NULL;
    int wpno = n - 1;
	for(wp = waypoint; wp; wp = wp->GetNextWP())
	{
		if (wpno <= 0) 
			return wp;
		wpno --;
    }
    return NULL;	   
}

SimVehicleClass::~SimVehicleClass(void)
{
WayPointClass* tmpWaypoint;

	SimVehicleClass::Cleanup();

	// Dispose waypoints
	curWaypoint = waypoint;
	while (curWaypoint)
	{
		tmpWaypoint = curWaypoint;
		curWaypoint = curWaypoint->GetNextWP();
		delete (tmpWaypoint);
	}

#ifdef DEBUG
SimVehicles--;
#endif
}

void SimVehicleClass::Cleanup(void)
{
   delete (theBrain);
   theBrain = NULL;
   delete (theInputs);
   theInputs = NULL;
}


float SimVehicleClass::GetRCSFactor (void)
{
	VehicleClassDataType	*vc	= (VehicleClassDataType *)Falcon4ClassTable[Type() - VU_LAST_ENTITY_TYPE].dataPtr;
	ShiAssert( vc );
	
	return vc->RCSfactor;
}

float SimVehicleClass::GetIRFactor (void)
{
// 2000-11-24 REWRITTEN BY S.G. SO IT USES THE NEW IR FIELD AND THE NEW ENGINE TEMPERATURE. THIS ALSO MAKES IT COMPATIBLE WITH RP4 MP.
#if 1
//	if (!g_bHardCoreReal)	MI
	if(!g_bRealisticAvionics)
	{
		VehicleClassDataType	*vc	= (VehicleClassDataType *)Falcon4ClassTable[Type() - VU_LAST_ENTITY_TYPE].dataPtr;
		ShiAssert( vc );
		
		return vc->RCSfactor;
	}
	else 
	{ 
		VehicleClassDataType	*vc	= (VehicleClassDataType *)Falcon4ClassTable[Type() - VU_LAST_ENTITY_TYPE].dataPtr;
		ShiAssert( vc );
		// TODO:  THis needs to return IR data
	// The IR signature is stored as a byte at offset 0x9E of the falcon4.vcd structure.
	// This byte, as well as 0x9D and 0x9F are used for padding originally.
	// The value range will be 0 to 2 with increments of 0.0078125
	unsigned char *pIrSign = (unsigned char *)vc;
	int iIrSign = (unsigned)pIrSign[0x9E];
	float irSignature = 1.0f;
	if (iIrSign)
		irSignature = (float)iIrSign / 128.0f;

		if (PowerOutput()<=1.0)//me123 status test. differensiate between idle, mil and ab
		{
			// Marco *** IR Fix is here!!! 0.12 -> make it smaller to increase time for IR to drop (set to 0.04f)
			// 0.05 -> Make it larger to increase 'idle' IROutput (set to 0.20f)
			// Marco edit - * by 1.2 to help out IR missiles reach RPG status

			irOutput = irOutput + ( (( ((PowerOutput()-0.70)*2)/irOutput) -1.0f ) * SimLibMajorFrameTime *0.04f);
			irOutput = max(min (0.6f,irOutput), 0.05f);    //,0.167f);

//	MonoPrint("IR output %d .\n",irOutput);
			return max( (irOutput * irSignature) * 1.875f, 0.15);
		}
		else 
		{
			// irOutput = (PowerOutput()- 0.985f )*100.0f/2;// from 1.25 to 2.25 (me123'a Settings)
			irOutput = ((PowerOutput() - 1.0f) * 10.0f) + 1.4f;
			return min(5.0f,irOutput * irSignature);
			//return (max (2.0f, min(5.0f,irOutput * irSignature) ));
		}
	}
#else
	VehicleClassDataType	*vc	= (VehicleClassDataType *)Falcon4ClassTable[Type() - VU_LAST_ENTITY_TYPE].dataPtr;
	ShiAssert( vc );
	
	// TODO:  THis needs to return IR data
// 2000-09-15 MODIFIED BY S.G. THIS GIVES IT THE IR DATA IT NEEDS
//	return vc->RCSfactor * PowerOutput();

	// The IR signature is stored as a byte at offset 0x9E of the falcon4.vcd structure.
	// This byte, as well as 0x9D and 0x9F are used for padding originally.
	// The value range will be 0 to 2 with increments of 0.0078125
	unsigned char *pIrSign = (unsigned char *)vc;
	int iIrSign = (unsigned)pIrSign[0x9E];
	float irSignature = 1.0f;
	if (iIrSign)
		irSignature = (float)iIrSign / 128.0f;

	return irSignature * EngineTempOutput();
#endif

}


int SimVehicleClass::GetRadarType (void)
{
	VehicleClassDataType	*vc	= (VehicleClassDataType *)((Falcon4EntityClassType*)EntityType())->dataPtr;
	ShiAssert( vc );
	
	return vc->RadarType;
}


int SimVehicleClass::Exec (void)
{
   Tpoint pos, vec;

   SimMoverClass::Exec();

   // update special effects timer
   // can be used in derived classes to control
   // various special effects they want to run
   // should be reset to 0 when effect has run
   sfxTimer += SimLibMajorFrameTime;

   // ioPerturb will result in  temporary perturbances in controls
   // when damage occurs (i.e missile hit, etc...).  Reduce this
   // every frame if > 0.0
   if ( ioPerturb > 0.0f )
   {
		// JB 010730
    if (ioPerturb > 0.5f && g_bDisableFunkyChicken && (rand() & 63) == 63)
		{
			float randamt  =   PRANDFloatPos();

			// run stuff here....
			pos.x = XPos();
			pos.y = YPos();
			pos.z = ZPos();
			OTWDriver.AddSfxRequest(
						new SfxClass( SFX_FIRE1,				// type
						&pos,							// world pos
						0.5f,							// time to live
						20.0f + 50.0f * randamt ) );		// scale
		}

	   ioPerturb -= 0.50f * SimLibMajorFrameTime;
	   if ( ioPerturb < 0.0f )
	   	ioPerturb = 0.0f;
   }


   // what's the strength?  if we're less than 0 we're dying.
   // bleed off strength when below 0.  This will allow time for
   // other effects (fire trails, ejection, etc...).  When we've reached
   // an absolute min, explode
   if ( pctStrength <= 0.0f && !IsExploding() )
   {
   		// debug non local
		/*
   		if ( !IsLocal() )
   		{
	   		MonoPrint( "NonLocal Dying: Pct Strength now: %f\n", pctStrength );
   		}
		else
		{
	   		MonoPrint( "Local Vehicle Dying: Pct Strength now: %f\n", pctStrength );
		}
		*/

   	   if ( gSfxLOD >= 0.5f )
	   {
		   // update dying timer
		   dyingTimer += SimLibMajorFrameTime;

		   // Do nothing for the 1st part of dying
		   if ( pctStrength > -0.07f )
		   {
		   }
		   // just chuck out debris until a certain point in death sequence
		   else if ( pctStrength > -0.5f )
		   {
				if ( dyingTimer > 0.3f )
				{
					pos.x = XPos() - XDelta() * SimLibMajorFrameTime;
					pos.y = YPos() - YDelta() * SimLibMajorFrameTime;
					pos.z = ZPos() - ZDelta() * SimLibMajorFrameTime;

					OTWDriver.AddSfxRequest(
							new SfxClass (SFX_SPARKS,			// type
							&pos,							// world pos
							1.0f,							// time to live
							12.0f ));		// scale
	
					/*
					for ( int i = 0; i < 3; i++ )
					{
						vec.x = XDelta() * 0.5f + PRANDFloat() * 20.0f;
						vec.y = YDelta() * 0.5f + PRANDFloat() * 20.0f;
						vec.z = ZDelta() * 0.5f + PRANDFloat() * 20.0f;
						OTWDriver.AddSfxRequest(
								new SfxClass (SFX_DARK_DEBRIS,			// type
								SFX_MOVES | SFX_USES_GRAVITY,						// flags
								&pos,							// world pos
								&vec,							// vector
								3.5f,							// time to live
								2.0f ));		// scale
					}
					*/

					// zero out
					dyingTimer = 0.0f;
				}
		   }
		   // if dyingTimer is greater than some value, we can run some
		   // sort of special effect such as tossing debris, ejection, etc...
		   else switch( dyingType )
		   {
			   case 5:
			   case SimVehicleClass::DIE_SMOKE:
				   if ( dyingTimer > 0.10f + ( 1.0f - gSfxLOD ) * 0.3f )
				   {
						vec.x = PRANDFloat() * 40.0f;
						vec.y = PRANDFloat() * 40.0f;
						vec.z = PRANDFloat() * 40.0f;
						// run stuff here....
						pos.x = XPos();
						pos.y = YPos();
						pos.z = ZPos();
						switch ( PRANDInt5() )
						{
							case 0:
								OTWDriver.AddSfxRequest(
									new SfxClass (SFX_SPARKS,			// type
									&pos,							// world pos
									4.5f,							// time to live
									7.5f ));		// scale
								break;
							case 1:
								OTWDriver.AddSfxRequest(
									new SfxClass (SFX_TRAILSMOKE,			// type
									SFX_MOVES,						// flags
									&pos,							// world pos
									&vec,							// vector
									4.5f,							// time to live
									20.5f ));		// scale
								break;
							default:
								OTWDriver.AddSfxRequest(
									new SfxClass (SFX_AIR_SMOKECLOUD,			// type
									SFX_MOVES,						// flags
									&pos,							// world pos
									&vec,							// vector
									4.5f,							// time to live
									10.5f ));		// scale
								break;
						}

	
					   // reset the timer
					   dyingTimer = 0.0f;
				   }
				   break;
			   case 6:
			   case SimVehicleClass::DIE_SHORT_FIREBALL:
				   if ( dyingTimer > 0.10f + ( 1.0f - gSfxLOD ) * 0.3f )
				   {
						if ( pctStrength > -0.7f )
						{
							float randamt  =   PRANDFloatPos();
				
							// run stuff here....
							pos.x = XPos();
							pos.y = YPos();
							pos.z = ZPos();
							OTWDriver.AddSfxRequest(
										new SfxClass( SFX_FIRE1,				// type
										&pos,							// world pos
										0.5f,							// time to live
										20.0f + 50.0f * randamt ) );		// scale
						}
						else
						{
							vec.x = PRANDFloat() * 40.0f;
							vec.y = PRANDFloat() * 40.0f;
							vec.z = PRANDFloat() * 40.0f;
							// run stuff here....
							pos.x = XPos() - XDelta() * SimLibMajorFrameTime;
							pos.y = YPos() - YDelta() * SimLibMajorFrameTime;
							pos.z = ZPos() - ZDelta() * SimLibMajorFrameTime;
							switch ( PRANDInt5() )
							{
								case 0:
									OTWDriver.AddSfxRequest(
										new SfxClass (SFX_SPARKS,			// type
										&pos,							// world pos
										4.5f,							// time to live
										12.5f ));		// scale
									break;
								case 1:
									OTWDriver.AddSfxRequest(
										new SfxClass (SFX_SPARKS,			// type
										&pos,							// world pos
										1.5f,							// time to live
										12.5f ));		// scale
									break;
								default:
									OTWDriver.AddSfxRequest(
										new SfxClass (SFX_TRAILSMOKE,			// type
										SFX_MOVES,						// flags
										&pos,							// world pos
										&vec,							// vector
										4.5f,							// time to live
										20.5f ));		// scale
									break;
							}
						}
	
					   // reset the timer
					   dyingTimer = 0.0f;
				   }
				   break;
			   case SimVehicleClass::DIE_INTERMITTENT_SMOKE:
				   if ( dyingTimer > 0.10f + ( 1.0f - gSfxLOD ) * 0.3f )
				   {
						vec.x = PRANDFloat() * 40.0f;
						vec.y = PRANDFloat() * 40.0f;
						vec.z = PRANDFloat() * 40.0f;
						// run stuff here....
						pos.x = XPos() - XDelta() * SimLibMajorFrameTime;
						pos.y = YPos() - YDelta() * SimLibMajorFrameTime;
						pos.z = ZPos() - ZDelta() * SimLibMajorFrameTime;
						switch ( PRANDInt5() )
						{
							case 0:
							case 1:
								OTWDriver.AddSfxRequest(
									new SfxClass (SFX_SPARKS,			// type
									&pos,							// world pos
									4.5f,							// time to live
									12.5f ));		// scale
								break;
							case 2:
								OTWDriver.AddSfxRequest(
									new SfxClass (SFX_SPARKS,			// type
									&pos,							// world pos
									1.5f,							// time to live
									12.5f ));		// scale
								break;
							default:
								OTWDriver.AddSfxRequest(
									new SfxClass (SFX_TRAILSMOKE,			// type
									SFX_MOVES,						// flags
									&pos,							// world pos
									&vec,							// vector
									4.5f,							// time to live
									20.5f ));		// scale
								break;
						}

						// reset the timer
						dyingTimer = 0.0f;
	
				   }
				   break;
			   case SimVehicleClass::DIE_FIREBALL:
				   if ( dyingTimer > 0.10f + ( 1.0f - gSfxLOD ) * 0.3f )
				   {
					   float randamt  =   PRANDFloatPos();
			
					   // run stuff here....
					   pos.x = XPos();
					   pos.y = YPos();
					   pos.z = ZPos();
					   OTWDriver.AddSfxRequest(
									new SfxClass( SFX_FIRE1,				// type
									&pos,							// world pos
									0.5f,							// time to live
									40.0f + 30.0f * randamt ) );		// scale
			
					   // reset the timer
					   dyingTimer = 0.0f;
				   }
				   break;
			   case SimVehicleClass::DIE_INTERMITTENT_FIRE:
			   default:
				   if ( dyingTimer > 0.10f + ( 1.0f - gSfxLOD ) * 0.3f )
				   {
						if ( pctStrength > -0.6f ||
							 pctStrength > -0.7f && pctStrength < -0.65 ||
							 pctStrength > -0.8f && pctStrength < -0.75 ||
							 pctStrength > -0.9f && pctStrength < -0.85 )
						{
							float randamt  =   PRANDFloatPos();
				
							// run stuff here....
							pos.x = XPos();
							pos.y = YPos();
							pos.z = ZPos();
							OTWDriver.AddSfxRequest(
										new SfxClass( SFX_FIRE1,				// type
										&pos,							// world pos
										0.5f,							// time to live
										20.0f + 50.0f * randamt ) );		// scale
						}
						else
						{
							vec.x = PRANDFloat() * 40.0f;
							vec.y = PRANDFloat() * 40.0f;
							vec.z = PRANDFloat() * 40.0f;
							// run stuff here....
							pos.x = XPos() - XDelta() * SimLibMajorFrameTime;
							pos.y = YPos() - YDelta() * SimLibMajorFrameTime;
							pos.z = ZPos() - ZDelta() * SimLibMajorFrameTime;
							switch ( PRANDInt5() )
							{
								case 0:
								case 1:
									OTWDriver.AddSfxRequest(
										new SfxClass (SFX_SPARKS,			// type
										&pos,							// world pos
										4.5f,							// time to live
										12.5f ));		// scale
									break;
								case 2:
									OTWDriver.AddSfxRequest(
										new SfxClass (SFX_SPARKS,			// type
										&pos,							// world pos
										1.5f,							// time to live
										12.5f ));		// scale
									break;
								default:
									OTWDriver.AddSfxRequest(
										new SfxClass (SFX_TRAILSMOKE,			// type
										SFX_MOVES,						// flags
										&pos,							// world pos
										&vec,							// vector
										4.5f,							// time to live
										20.5f ));		// scale
									break;
							}
						}
	
					   // reset the timer
					   dyingTimer = 0.0f;
				   }
				   break;
		   } // end switch
	   } // end if LOD

	   // bleed off strength at some rate....
	   strength -= maxStrength * 0.05f * SimLibMajorFrameTime;
	   pctStrength = strength/maxStrength;

	   // when we've bottomed out, kablooey!
	   if ( pctStrength <= -1.0f || OnGround() )
	   {
		   // damage anything around us when we explode
		   ApplyProximityDamage();

	   	   SetExploding( TRUE );

//           MonoPrint ("Vehicle %d DEAD! Local: %s, Has Death Msg: %s\n",
//		   				Id().num_,
//						IsLocal() ? "TRUE" : "FALSE",
//						deathMessage ? "TRUE" : "FALSE" );

		   // local entity sends death message out
		   if ( deathMessage )
		   {
      	   		FalconSendMessage (deathMessage,TRUE);
		   }
	   }

   } // end if pctStr < 0

   return TRUE;
}

// function interface
// serialization functions
int SimVehicleClass::SaveSize()
{
   return SimMoverClass::SaveSize();
}

int SimVehicleClass::Save(VU_BYTE **stream)
{
int retval;

   retval = SimMoverClass::Save(stream);

   return retval;
}

int SimVehicleClass::Save(FILE *file)
{
int retval;

   retval = SimMoverClass::Save (file);

   return (retval);
}

int SimVehicleClass::Handle(VuFullUpdateEvent *event)
{
   return (SimMoverClass::Handle(event));
}


int SimVehicleClass::Handle(VuPositionUpdateEvent *event)
{
   return (SimMoverClass::Handle(event));
}

int SimVehicleClass::Handle(VuTransferEvent *event)
{
   return (SimMoverClass::Handle(event));
}

VU_ERRCODE SimVehicleClass::InsertionCallback(void)
{
//	KCK: If you're relying on the code below, something is
//	VERY, VERY wrong. Please talk to me before re-adding this code again
//	if (IsLocal())
//	{
//		Wake();
//	}

	if (theBrain)
		theBrain->PostInsert();

	return SimMoverClass::InsertionCallback();
}

void SimVehicleClass::SetDead (int flag)
{
	SimMoverClass::SetDead(flag);
}

//void SimVehicleClass::InitWeapons (ushort *type, ushort *num)
void SimVehicleClass::InitWeapons (ushort*, ushort*)
{
	// KCK: Moved to SMSClass constructor
}

void SimVehicleClass::SOIManager (SOI newSOI)
{
SMSClass	*Sms = (SMSClass*)GetSMS();
MissileClass* theMissile = NULL;
MissileDisplayClass* mslDisplay = NULL;
SensorClass* tPodDisplay = NULL;
RadarClass* theRadar = (RadarClass*) FindSensor (this, SensorClass::Radar);
//MI
FireControlComputer* FCC = ((SimVehicleClass*)this)->GetFCC();

	if (Sms)
		theMissile = (MissileClass*)Sms->GetCurrentWeapon();

   

   if (theMissile && theMissile->IsMissile())
   {
      if (theMissile->GetDisplayType() == MissileClass::DisplayBW ||
          theMissile->GetDisplayType() == MissileClass::DisplayIR ||
          theMissile->GetDisplayType() == MissileClass::DisplayHTS)
      {
         mslDisplay = (MissileDisplayClass*)theMissile->display;
      }
   }

   tPodDisplay = FindLaserPod (this);

   curSOI = newSOI;

   switch (curSOI)
   {
      case SOI_HUD:
         if (theRadar)
            theRadar->SetSOI(FALSE);
		 if (TheHud)
			TheHud->SetSOI(TRUE);
         if (mslDisplay)
            mslDisplay->SetSOI(FALSE);
         if (tPodDisplay)
            tPodDisplay->SetSOI(FALSE);
		 if (FCC)	//MI
			 FCC->ClearSOIAll();
      break;

      case SOI_RADAR:
         if (TheHud)
            TheHud->SetSOI(FALSE);
         if (theRadar)
            theRadar->SetSOI(TRUE);
         if (mslDisplay)
            mslDisplay->SetSOI(FALSE);
         if (tPodDisplay)
            tPodDisplay->SetSOI(FALSE);
		 if (FCC)	//MI
			 FCC->ClearSOIAll();
      break;

      case SOI_WEAPON:
         if (theRadar)
            theRadar->SetSOI(FALSE);
				 if (TheHud) // JB/JPO 010614 CTD
	         TheHud->SetSOI(FALSE);
          if (mslDisplay)
            mslDisplay->SetSOI(TRUE);
         if (tPodDisplay)
            tPodDisplay->SetSOI(TRUE);
		 if (FCC)	//MI
			 FCC->ClearSOIAll();
     break;
	 //MI
	  case SOI_FCC:
	  {
		  if(theRadar)
			  theRadar->SetSOI(FALSE);
		  if (mslDisplay)
			  mslDisplay->SetSOI(FALSE);
		  if (tPodDisplay)
			  tPodDisplay->SetSOI(FALSE);
		  if (TheHud) // JB/JPO 010614 CTD
			  TheHud->SetSOI(FALSE);
		  if (FCC)
		  {
			  //reset our cursor position
			  FCC->xPos = 0.0F;
			  FCC->yPos = 0.0F;				
			  FCC->IsSOI = TRUE;
		  }
	  }
	  break;
   }
}

void SimVehicleClass::ApplyDamage (FalconDamageMessage* damageMessage)
{
	int soundIdx = -1;
	VehicleClassDataType *vc;
	WeaponClassDataType *wc;
	FalconEntity *lastToHit;
	float hitPoints = 0.0f;
	int groundType;
	int i;
	Tpoint pos, mvec;

	// call base class damage function
	SimBaseClass::ApplyDamage (damageMessage);

	switch (damageMessage->dataBlock.damageType)
		{
		// Collisions
		case FalconDamageType::GroundCollisionDamage:
		case FalconDamageType::FeatureCollisionDamage:
		case FalconDamageType::ObjectCollisionDamage:
		case FalconDamageType::CollisionDamage:
		case FalconDamageType::DebrisDamage:
			hitPoints = damageMessage->dataBlock.damageStrength;
			break;

		// Auto-death
		case FalconDamageType::FODDamage:
			hitPoints = maxStrength;
			break;

		// Proximity Damage
		case FalconDamageType::ProximityDamage:
			// Same as weapon damage, but we pre-calculate the damage strength
			// get vehicle data
			vc = (VehicleClassDataType *)Falcon4ClassTable[ Type() - VU_LAST_ENTITY_TYPE].dataPtr;
			// get strength of weapon and calc damage based on vehicle mod
			hitPoints = (float)damageMessage->dataBlock.damageStrength * ((float)vc->DamageMod[HighExplosiveDam])/100.0f;
			break;

		// Weapon Hits
		default:
			// get vehicle data
			vc = (VehicleClassDataType *)Falcon4ClassTable[ Type() - VU_LAST_ENTITY_TYPE].dataPtr;
			// get weapon data
			wc = (WeaponClassDataType *)Falcon4ClassTable[ damageMessage->dataBlock.fWeaponID - VU_LAST_ENTITY_TYPE].dataPtr;
			// get strength of weapon and calc damage based on vehicle mod
			// also, check for ownship and invincibiliy flag
			hitPoints = (float)wc->Strength * ((float)vc->DamageMod[ wc->DamageType ])/100.0f;
			break;
		}

	switch (damageMessage->dataBlock.damageType)
		{
		case FalconDamageType::BulletDamage:
			soundIdx = SFX_RICOCHET1 + PRANDInt5();
			ioPerturb = 0.5f;
			hitPoints -= hitPoints * 0.7f * damageMessage->dataBlock.damageRandomFact;
			break;

		case FalconDamageType::MissileDamage:
			soundIdx = SFX_BOOMA1 + PRANDInt5();
			ioPerturb = 2.0f;
			hitPoints += hitPoints * 0.25f * damageMessage->dataBlock.damageRandomFact;
			break;

		case FalconDamageType::BombDamage:
			soundIdx = SFX_BOOMG1 + PRANDInt5();
			ioPerturb = 2.0f;
			hitPoints += hitPoints * 0.25f * damageMessage->dataBlock.damageRandomFact;
			break;

		case FalconDamageType::ProximityDamage:
			// KCK Question: Should we have secondary explosion sound effects here?
			ioPerturb = 2.0f;
			hitPoints += hitPoints * 0.25f * damageMessage->dataBlock.damageRandomFact;
			break;

		case FalconDamageType::GroundCollisionDamage:
			// if we're exploding at this point we've smacked into a
			// large brown,green,blue globe....
			hitPoints += hitPoints * 0.25f * damageMessage->dataBlock.damageRandomFact;
			ioPerturb = 2.0f;
			if (OTWDriver.GetViewpoint())
			    groundType = (OTWDriver.GetViewpoint())->GetGroundType ( XPos(), YPos() );
			else
				groundType = COVERAGE_PLAINS;

			// are we blowing up?
			if (  hitPoints > strength )
			{
				// death sounds
				if ( !(groundType == COVERAGE_WATER || groundType == COVERAGE_RIVER)  )
					soundIdx = SFX_DIRTDART;
				else
					soundIdx = SFX_H2ODART;
			}
			else
			{
				// damage sounds and effects

				pos.x = XPos();
				pos.y = YPos();
				pos.z = ZPos();
				mvec.x = XDelta();
				mvec.y = YDelta();
				mvec.z = ZDelta() - 40.0f;

				if ( !(groundType == COVERAGE_WATER || groundType == COVERAGE_RIVER) )
				{
					soundIdx = SFX_BOOMA1;
					OTWDriver.AddSfxRequest(
						new SfxClass( SFX_GROUND_DUSTCLOUD,		// type
						SFX_MOVES,
						&pos,					// world pos
						&mvec,					// vel vector
						3.0,					// time to live
						8.0f ) );				// scale

				}
				else
				{
					soundIdx = SFX_SPLASH;
					OTWDriver.AddSfxRequest(
						new SfxClass( SFX_WATER_CLOUD,		// type
						SFX_MOVES,
						&pos,					// world pos
						&mvec,					// vel vector
						3.0,					// time to live
						10.0f ) );				// scale
				}
			}

			// trail sparks when hit ground
			for ( i = 0; i < 5; i++ )
			{
				pos.x = XPos() - XDelta() * SimLibMajorFrameTime * PRANDFloatPos() * 2.0f;
				pos.y = YPos() - YDelta() * SimLibMajorFrameTime * PRANDFloatPos() * 2.0f;
				pos.z = ZPos();
				// mvec.x = PRANDFloat() * 60.0f;
				// mvec.y = PRANDFloat() * 60.0f;
				// mvec.z = -40.0f;

				OTWDriver.AddSfxRequest(
					new SfxClass( SFX_SPARKS,		// type
					&pos,					// world pos
					2.9f,					// time to live
					14.3f ) );				// scale
			}
			break;

		case FalconDamageType::CollisionDamage:
		case FalconDamageType::FeatureCollisionDamage:
		case FalconDamageType::ObjectCollisionDamage:
			hitPoints += hitPoints * 0.25f * damageMessage->dataBlock.damageRandomFact;
			soundIdx = SFX_BOOMG1 + PRANDInt5();
			ioPerturb = 2.0f;
			break;

		case FalconDamageType::DebrisDamage:
			// we probably want thump sounds here.....
			soundIdx = SFX_BOOMA1 + PRANDInt5();
			hitPoints += hitPoints * 0.25f * damageMessage->dataBlock.damageRandomFact;
			ioPerturb = 2.0f;
			break;

		default:
			break;
		}

   // what percent strength is left?
   // NOTE: A missile strike effect to an aircraft looks like a missile
   // burst with the plane flying thru it OK for a little while and trailing
   // some debris.  For missile damage, don't let pctStrength hit neg val
   // until it's been set to 0.  This will allow time to generate debris
   if ( strength == maxStrength &&
   		hitPoints > strength &&
		   IsAirplane() &&
	    damageMessage->dataBlock.damageType == FalconDamageType::MissileDamage )
   {
   		hitPoints = strength;
   }

   // check for ownship and invincibiliy flag
   if (IsSetFalcFlag(FEC_INVULNERABLE))
   {
      hitPoints = 0;
   }

   strength -= hitPoints;
   pctStrength = strength/maxStrength;

		// JB 000816
		// If damaged has been sustained then:
		// If this is our own aircraft and if we have disabled the damage jitter or a 25% chance
		// Or otherwise a 13% chance (AI controlled aircraft won't be able to fly with a bias so we want the % to be low)
		if (hitPoints > 0 && 
			((IsSetFlag(MOTION_OWNSHIP) && (g_bDisableFunkyChicken || (rand() & 0x3) == 0x3)) ||
			(rand() & 0x7) == 0x7)) 
		{
			rBias += ioPerturb / 2 * PRANDFloat();
			pBias += ioPerturb / 2 * PRANDFloat();
			yBias += ioPerturb / 2 * PRANDFloat();
		}

// JB 010121 adjusted to work in MP
		if (g_bNewDamageEffects && IsSetFlag(MOTION_OWNSHIP) && // hitPoints > 0 && 2002-04-11 REMOVED BY S.G. Done below after ->af since it's now externalized
//			SimDriver.playerEntity && SimDriver.playerEntity->AutopilotType() != AircraftClass::CombatAP && 
//			!(gCommsMgr && gCommsMgr->Online()) && 
			IsAirplane() &&
//			(rand() & 0x7) == 0x7 && // 13% chance 2002-04-11 MOVED BY S.G. After the ->af and used the external var now
			((AircraftClass*)this)->af &&
			((AircraftClass*)this)->af->GetEngineDamageHitThreshold() < hitPoints && // 2002-04-11 ADDED BY S.G. hitPoints 'theshold' is no longer 1 or above but externalized
			((AircraftClass*)this)->af->GetEngineDamageStopThreshold() > rand() % 100 && // 2002-04-11 ADDED BY S.G. instead of a fixed 13%, now uses an aiframe aux var
			((AircraftClass*)this)->AutopilotType() != AircraftClass::CombatAP
			)
		{
			((AircraftClass*)this)->af->SetFlag(AirframeClass::EngineStopped);
//			((AircraftClass*)this)->af->SetFlag(AirframeClass::EpuRunning);

			// MODIFIED BY S.G. Instead of 50%, now uses an aiframe aux var
			// if ((rand() & 0x1) == 0x1) // JB 010115 half the time (13%/2) you won't be able to restart.
			if (((AircraftClass*)this)->af->GetEngineDamageNoRestartThreshold() > rand() % 100)
				((AircraftClass*)this)->af->jfsaccumulator = -3600;
		}
		// JB 000816

   // debug non local
   if ( !IsLocal() )
   {
//	   MonoPrint( "NonLocal Apply Damage: Pct Strength now: %f\n", pctStrength );
   }

   if ( soundIdx != -1 )
	   F4SoundFXSetPos( soundIdx, TRUE, XPos(), YPos(), ZPos(), 1.0f );

   // send out death message if strength below zero
   if (pctStrength <= 0.0F && dyingTimer < 0.0F)
   {
	  // ground vehicles die immediately
	  if ( IsGroundVehicle() )
	  		pctStrength = -1.0f;

      // only the local entity sends the death message
	  if ( IsLocal() )
	  {
			deathMessage = new FalconDeathMessage (Id(), FalconLocalGame);
			deathMessage->dataBlock.damageType = damageMessage->dataBlock.damageType;

			deathMessage->dataBlock.dEntityID  = Id();
			ShiAssert (GetCampaignObject())
			deathMessage->dataBlock.dCampID		= ((CampBaseClass*)GetCampaignObject())->GetCampID();
			deathMessage->dataBlock.dSide		= ((CampBaseClass*)GetCampaignObject())->GetOwner();
			deathMessage->dataBlock.dPilotID   = pilotSlot;
			deathMessage->dataBlock.dIndex     = Type();
	
			lastToHit = (SimVehicleClass*)vuDatabase->Find( LastShooter() );
			if (lastToHit && !lastToHit->IsEject() && lastToHit->Id() != damageMessage->dataBlock.fEntityID)
				{
				deathMessage->dataBlock.fEntityID		= lastToHit->Id();
				deathMessage->dataBlock.fIndex			= lastToHit->Type();
				if (lastToHit->IsSim())
					{
					deathMessage->dataBlock.fCampID		= ((SimVehicleClass*)lastToHit)->GetCampaignObject()->GetCampID();
					deathMessage->dataBlock.fPilotID	= ((SimVehicleClass*)lastToHit)->pilotSlot;
					deathMessage->dataBlock.fSide		= ((SimVehicleClass*)lastToHit)->GetCampaignObject()->GetOwner();
					}
				else
					{
					deathMessage->dataBlock.fCampID		= ((CampBaseClass*)lastToHit)->GetCampID();
					deathMessage->dataBlock.fPilotID	= 0;
					deathMessage->dataBlock.fSide		= ((CampBaseClass*)lastToHit)->GetOwner();
					}
				// We really don't know what weapon did the killing. Pick a non-existant one
				deathMessage->dataBlock.fWeaponID  = 0xffff;
				deathMessage->dataBlock.fWeaponUID = FalconNullId;
				}
			else
				{
				// If aircraft is undamaged, the death message we send is 'ground collision',
				// since the aircraft would probably get there eventually
				deathMessage->dataBlock.fEntityID  = damageMessage->dataBlock.fEntityID;
				deathMessage->dataBlock.fIndex     = damageMessage->dataBlock.fIndex;
				deathMessage->dataBlock.fCampID    = damageMessage->dataBlock.fCampID;
				deathMessage->dataBlock.fPilotID   = damageMessage->dataBlock.fPilotID;
				deathMessage->dataBlock.fSide      = damageMessage->dataBlock.fSide;
				deathMessage->dataBlock.fWeaponID  = damageMessage->dataBlock.fWeaponID;
				deathMessage->dataBlock.fWeaponUID = damageMessage->dataBlock.fWeaponUID;
				}
			deathMessage->dataBlock.deathPctStrength = pctStrength;
//	me123		deathMessage->RequestOutOfBandTransmit ();
	  }
	  // this is now done in the exec() function when hitpoints reaches
	  // the appropriate value
      // FalconSendMessage (deathMessage,FALSE);

	  // short delay before vehicle starts death throw effects
	  dyingTimer = 0.0f;
	  // dyingType = PRANDInt5();
	  dyingType = rand() & 0x07;

      //MonoPrint ("Vehicle %d in death throws at %8ld\n", Id().num_, SimLibElapsedTime);
      //MonoPrint ("Dying Type = %d\n", dyingType);
   }
}

// Generic fire message used by all platforms
void SimVehicleClass::SendFireMessage (SimWeaponClass* curWeapon, int type, int startFlag, SimObjectType* targetPtr, VU_ID targetId)
{
FalconWeaponsFire* fireMsg;
#ifdef DEBUG
static int fireCount = 0;

  // MonoPrint ("Firing %d at %d. mesg count %d\n", type, SimLibElapsedTime, fireCount);
   fireCount ++;
#endif

   fireMsg = new FalconWeaponsFire (Id(), FalconLocalGame);
   fireMsg->dataBlock.fEntityID  = Id();
   fireMsg->dataBlock.weaponType  = type;
   ShiAssert (GetCampaignObject());
   fireMsg->dataBlock.fCampID = ((CampBaseClass*)GetCampaignObject())->GetCampID();
   fireMsg->dataBlock.fSide = ((CampBaseClass*)GetCampaignObject())->GetOwner();
   fireMsg->dataBlock.fPilotID   = pilotSlot;
   fireMsg->dataBlock.fIndex     = Type();

   // For rockets, get the pod id instead of the weapon ID
		// JB 010104 Marco Edit 
   //if (IsAirplane() && type == FalconWeaponsFire::Rocket)
   //   fireMsg->dataBlock.fWeaponID = static_cast<ushort>(((AircraftClass*)this)->Sms->hardPoint[((AircraftClass*)this)->Sms->CurHardpoint()]->GetRackId());
   //else
   // JB 010104 Marco Edit 
		  fireMsg->dataBlock.fWeaponID  = curWeapon->Type();

   fireMsg->dataBlock.dx = 0.0f;
   fireMsg->dataBlock.dy = 0.0f;
   fireMsg->dataBlock.dz = 0.0f;
   if (type == FalconWeaponsFire::GUN)
   {
	  if ( ((GunClass *)curWeapon)->IsTracer() )
	  {
			if ( startFlag )
			{     
 // MonoPrint (" startFlag\n" );
				((GunClass*)curWeapon)->NewBurst();
				fireMsg->dataBlock.fWeaponUID.num_ = ((GunClass*)curWeapon)->GetCurrentBurst();
				fireMsg->dataBlock.dx = ((GunClass*)curWeapon)->bullet[0].xdot;
				fireMsg->dataBlock.dy = ((GunClass*)curWeapon)->bullet[0].ydot;
				fireMsg->dataBlock.dz = ((GunClass*)curWeapon)->bullet[0].zdot;
			}
			else
			{
 // MonoPrint (" endFlag\n" );
				// Null Id means were stopping gun fire.
				fireMsg->dataBlock.fWeaponUID.num_ = 0;
				fireMsg->RequestReliableTransmit ();
			}
	  }
	  else
	  {
 // MonoPrint ("GUN BURST\n" );
			((GunClass*)curWeapon)->NewBurst();
			fireMsg->dataBlock.fWeaponUID.num_ = ((GunClass*)curWeapon)->GetCurrentBurst();
	  }

   }
   else
   {
   	fireMsg->RequestReliableTransmit ();
   	fireMsg->RequestOutOfBandTransmit ();
  //  MonoPrint ("OTHER WEAPON \n" );
      // Make sure the weapon's shooter pilot slot is the same as the current pilot
      curWeapon->shooterPilotSlot = pilotSlot;
      fireMsg->dataBlock.fWeaponUID = curWeapon->Id();
   }

   if (targetPtr)
      fireMsg->dataBlock.targetId   = targetPtr->BaseData()->Id();
   else if (targetId != vuNullId)
      fireMsg->dataBlock.targetId   = targetId;
   else
      fireMsg->dataBlock.targetId   = vuNullId;
   FalconSendMessage (fireMsg);
}

/*
** Name: ApplyProximityDamage
** Description:
**		Cycles thru objectList check for proximity.
**		Cycles thru all objectives, and checks vs individual features
**        if it's within the objective's bounds.
*/
void
SimVehicleClass::ApplyProximityDamage ( void )
{
float tmpX, tmpY, tmpZ;
float rangeSquare;
SimBaseClass* testObject;
float damageRadiusSqrd;
#ifdef VU_GRID_TREE_Y_MAJOR
VuGridIterator gridIt(ObjProxList, YPos(), XPos(), NM_TO_FT * 3.5F);
#else
VuGridIterator gridIt(ObjProxList, XPos(), YPos(), NM_TO_FT * 3.5F);
#endif

    FalconDamageMessage* message;
    float normBlastDist;

	// when on ground, damage radius is smaller
	if ( OnGround() )
	{
		damageRadiusSqrd = 150.0f * 150.0f;
	}
	else
	{
		damageRadiusSqrd = 300.0f * 300.0f;
	}

	if (!SimDriver.objectList)
	{
		return;
	}

	VuListIterator objectWalker(SimDriver.objectList);

	// Check vs vehicles
	testObject = (SimBaseClass*) objectWalker.GetFirst();
	while (testObject)
	{
		// don't send msg to objects already dead or dying
		// no damge to camp units
		if ( testObject->IsCampaign() || testObject->pctStrength < 0.0f )
		{
			testObject = (SimBaseClass*) objectWalker.GetNext();
			continue;
		}

		if (testObject != this)
		{
			tmpX = testObject->XPos() - XPos();
			tmpY = testObject->YPos() - YPos();
			tmpZ = testObject->ZPos() - ZPos();

			rangeSquare = tmpX*tmpX + tmpY*tmpY + tmpZ*tmpZ;

			if (rangeSquare < damageRadiusSqrd)
			{
				// edg: calculate a normalized blast Dist
				normBlastDist = ( damageRadiusSqrd - rangeSquare )/( damageRadiusSqrd );
			
				// quadratic dropoff
				normBlastDist *= normBlastDist;

				message = new FalconDamageMessage (testObject->Id(), FalconLocalGame);
				message->dataBlock.fEntityID  = Id();
				message->dataBlock.fCampID = GetCampID();
				message->dataBlock.fSide   = static_cast<uchar>(GetCountry());
				message->dataBlock.fPilotID   = pilotSlot;
				message->dataBlock.fIndex     = Type();
				message->dataBlock.fWeaponID  = Type();
				message->dataBlock.fWeaponUID = Id();
			
				message->dataBlock.dEntityID  = testObject->Id();
				message->dataBlock.dCampID = testObject->GetCampID();
				message->dataBlock.dSide   = static_cast<uchar>(testObject->GetCountry());
			
				if (testObject->IsSimObjective())
					message->dataBlock.dPilotID   = 255;
				else
					message->dataBlock.dPilotID   = ((SimMoverClass*)testObject)->pilotSlot;
				message->dataBlock.dIndex     = testObject->Type();
			
				message->dataBlock.damageRandomFact = 1.0f;
				message->dataBlock.damageStrength = normBlastDist * 120.0f;
				message->dataBlock.damageType = FalconDamageType::ProximityDamage;
//	me123			message->RequestOutOfBandTransmit ();
				FalconSendMessage (message,TRUE);

			}
		}
		testObject = (SimBaseClass*) objectWalker.GetNext();
	}
}
//MI
void SimVehicleClass::StepSOI(int dir)
{
	SMSClass	*Sms = (SMSClass*)GetSMS();
	MissileClass* theMissile = NULL;
	MissileDisplayClass* mslDisplay = NULL;
	SensorClass* tPodDisplay = NULL;
	RadarClass* theRadar = (RadarClass*) FindSensor (this, SensorClass::Radar);
	//MI
	FireControlComputer* FCC = ((SimVehicleClass*)this)->GetFCC();

	if(!theRadar || !FCC || !Sms || !SimDriver.playerEntity)
		return;

	if (Sms)
		theMissile = (MissileClass*)Sms->GetCurrentWeapon();

   

   if (theMissile && theMissile->IsMissile())
   {
      if (theMissile->GetDisplayType() == MissileClass::DisplayBW ||
          theMissile->GetDisplayType() == MissileClass::DisplayIR ||
          theMissile->GetDisplayType() == MissileClass::DisplayHTS)
      {
         mslDisplay = (MissileDisplayClass*)theMissile->display;
      }
   }

   tPodDisplay = FindLaserPod (this);
	
	//dir 1 = up
	//dir 2 = down
	switch(dir)
	{
	case 1:
		//Can the SOI move to the HUD?
		//ok let's see... AGR can't be the SOI
		//info from Snako
/*o	If CCIP, CCIP rockets, STRF, DTOS, or EO-VIS is selected, the forward position moves the 
SOI to the HUD and makes it the SOI.  When LADD, EO PRE, or CCRP submode is selected along with 
the IP or RP, the SOI designations will move to the HUD when the DMS is moved forward.  When TWS
 is selected, TWS AUTO or MAN submode will be selected.
o	The UP direction wouldn't have any effect on A2A master modes.
o	If the aft (down) position is selected, the SOI moves to the MFD of the highest priority.  
Subsequent aft depressions move the SOI to the opposite MFD.
*/
		if(FCC->GetSubMode() == FireControlComputer::CCIP || 
			FCC->GetSubMode() == FireControlComputer::DTOSS ||
			FCC->GetSubMode() == FireControlComputer::RCKT ||
			FCC->GetSubMode() == FireControlComputer::STRAF ||
			FCC->GetSubMode() == FireControlComputer::CCRP ||
			FCC->GetSubMode() == FireControlComputer::LADD)
		{ 
			SOIManager(SOI_HUD);
		}
		break;
	case 2:
		//Toggles between the MFD's.
		//Check if we are currently in HSD SOI
		if(FCC->IsSOI)
		{
			//Yes, can our radar be the SOI?
			//if we are in AG MasterMode, there's not many possibilities
			if(FCC->IsAGMasterMode())
			{
				if(FCC->GetSubMode() != FireControlComputer::CCIP && FCC->GetSubMode() != FireControlComputer::DTOSS)
				{
					SOIManager(SOI_RADAR);
				}
			}
			else if(FCC->IsAAMasterMode() || FCC->IsNavMasterMode() || FCC->GetMasterMode() ==
				FireControlComputer::MissileOverride || FCC->GetMasterMode() == 
				FireControlComputer::Dogfight)
			{
				SOIManager(SOI_RADAR);
			}
		}
		else
		{
			//No, currently not SOI. If we have the HSD visible, we're going to set it there
			if(FCC->CouldBeSOI)
				SOIManager(SOI_FCC);
			else if((mslDisplay && !mslDisplay->IsSOI()) || (tPodDisplay && !tPodDisplay->IsSOI()))
				SOIManager(SOI_WEAPON);
			else
				SOIManager(SOI_RADAR);
		}
		break;
	default:
		ShiWarning("Wrong SOI Direction!");
		break;
	}
}
