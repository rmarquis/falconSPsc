#include "stdhdr.h"
#include "aircrft.h"
#include "airframe.h"
#include "acmi\src\include\acmirec.h"
#include "otwdrive.h"
#include "classtbl.h"
#include "ffeedbk.h"
#include "simdrive.h"
#include "dofsnswitches.h"
#include "simio.h"
#include "Graphics\Include\drawgrnd.h"
#include "soundfx.h"
#include "fsound.h"
#include "fakerand.h"

extern ACMISwitchRecord acmiSwitch;
extern ACMIDOFRecord DOFRec;
static int stallShake = FALSE;
extern bool g_bRealisticAvionics;
extern bool g_bNewFm;
extern bool g_bNewDamageEffects;

// JPO
// works out flap and aileron angles. Sometimes these are linked as in F-16 flapperons.
void AircraftClass::CalculateAileronAndFlap(float qfactor, float *al, float *ar, float *fl, float *fr)
{
    float stabAngle;
    float flapdelta = 0;
    
    // elevators always working
    stabAngle = af->rstick * qfactor * af->auxaeroData->aileronMaxAngle * DTR;

    switch (af->auxaeroData->hasTef) {
    case AUX_LEFTEF_MANUAL: // nothing special here, just set to what given
	flapdelta = af->tefPos * DTR;
	break;
    case AUX_LEFTEF_AOA: // nothing uses this yet. So not sure what happens
    case AUX_LEFTEF_MACH: // dependendant on vcas (not MACH despite the name)
	{
	    float gdelta;
	    float speeddelta;
	    
	    if (TEFExtend) // forcibly extended
		gdelta = 1;
	    else if (af->auxaeroData->flapGearRelative) // else dependent on gear deployment
		gdelta = max(0,af->gearPos);
	    else gdelta = 1; // else always
	    
	    // work out how much flap we would have at this vcas
	    speeddelta = (af->auxaeroData->maxFlapVcas - af->vcas) / 
		af->auxaeroData->flapVcasRange;
	    speeddelta = max(0.0F,min(1.0F, speeddelta)); //limit to 0-1 range

	    // max flaps * speed dependent factor (0-1) * gear delta (0-1)
	    flapdelta = af->auxaeroData->tefMaxAngle  * DTR * speeddelta * gdelta;
	    break;
	}
    }

    *al = stabAngle;
    *ar = -stabAngle;
    *fl = flapdelta;
    *fr = flapdelta;
}

void AircraftClass::CalculateLef(float qfactor)
{
	// LEF
	if (af->auxaeroData->hasLef == AUX_LEFTEF_MANUAL) 
	{
		// use pilot input
		leftLEFAngle = af->lefPos * DTR;
		rightLEFAngle = af->lefPos * DTR;
	}
	else if (af->auxaeroData->hasLef == AUX_LEFTEF_TEF) { // LEF controlled by TEF
	    if (af->tefPos > 0)
		af->LEFMax();
	    else af->LEFClose();
	    leftLEFAngle = af->lefPos * DTR;
	    rightLEFAngle = af->lefPos * DTR;
	}
	else 
	{
		if (!af->IsSet(AirframeClass::InAir))
		{
			leftLEFAngle = rightLEFAngle = af->auxaeroData->lefGround * DTR;
		}
		else if (!g_bNewFm && af->mach > af->auxaeroData->lefMaxMach)
		{
			leftLEFAngle = rightLEFAngle = 0;;
		}
		else if (af->auxaeroData->hasLef == AUX_LEFTEF_AOA)
		{
			leftLEFAngle = max (min(af->alpha,af->auxaeroData->lefMaxAngle)* DTR, 0.0f);//me123 lef is controled my aoa not mach
			rightLEFAngle = leftLEFAngle;
			//MI additions
			if(g_bRealisticAvionics && g_bNewFm)
			{
				if(g_bNewDamageEffects)
				{
					leftLEFAngle = CheckLEF(0);	//left
					rightLEFAngle = CheckLEF(1);	//right
				}
			}
		}					  	  
		else
		{
			leftLEFAngle = min ((af->auxaeroData->lefMaxMach - af->mach) / 0.2F, 1.0F) * af->auxaeroData->lefMaxAngle * DTR;
			rightLEFAngle = leftLEFAngle;
		}
		if(LEFLocked && g_bRealisticAvionics && g_bNewFm)
		{ 
			if (IsComplex()) {
				leftLEFAngle = GetDOFValue(COMP_LT_LEF);
				rightLEFAngle = GetDOFValue(COMP_RT_LEF);
			}
			else {
				leftLEFAngle = GetDOFValue(SIMP_LT_LEF);
				rightLEFAngle = GetDOFValue(SIMP_RT_LEF);
			}

		} 
	}
}

void AircraftClass::CalculateStab(float qfactor, float *sl, float *sr)
{
    if (af->auxaeroData->elevatorRolls) {
	*sl = max ( min (af->pstick - af->rstick, 1.0F), -1.0F) * qfactor * af->auxaeroData->elevonMaxAngle * DTR;
	*sr = max ( min (af->pstick + af->rstick, 1.0F), -1.0F) * qfactor * af->auxaeroData->elevonMaxAngle * DTR;
    }
    else {
    	*sr = *sl = max ( min (af->pstick, 1.0F), -1.0F) * qfactor * af->auxaeroData->elevonMaxAngle * DTR;
    }
}

float AircraftClass::CalculateRudder(float qfactor)
{
	return af->ypedal * af->auxaeroData->rudderMaxAngle * DTR * qfactor;
}

// JPO - routine to get the DOF to move to the desirted position
// happens at a given rate though. Optionally play SFX during the time.
void AircraftClass::MoveDof(int dof, float newval, float rate, int ssfx, int lsfx, int esfx)
{
    float changeval;
    float cdof = GetDOFValue(dof);
    bool doend = false;
    if (cdof == newval) return; // all done
    
    changeval = rate * DTR * SimLibMajorFrameTime;
    if (cdof > newval)
    {
	cdof -= changeval;
	if (cdof <= newval) {
	    cdof = newval;
	    doend = true;
	}
	SetDOF(dof, cdof);
    }
    else if (cdof < newval)
    {
	cdof += changeval;
	if (cdof >= newval) {
	    cdof = newval;
	    doend = true;
	}
	SetDOF(dof, cdof);
    }
    
    if (SFX_DEF && ssfx >= 0) { // something to play
	if (doend)
	    F4SoundFXSetPos( esfx, TRUE, af->x, af->y, af->z, 1.0f );
	else if (!F4SoundFXPlaying( ssfx) &&
	    !F4SoundFXPlaying( lsfx))
	    F4SoundFXSetPos( ssfx, TRUE, af->x, af->y, af->z, 1.0f );
	else
	    F4SoundFXSetPos(lsfx, TRUE, af->x, af->y, af->z, 1.0f );
    }
}

void AircraftClass::DeployDragChute(int type)
{
    if (af->vcas < 20.0f && af->dragChute == AirframeClass::DRAGC_DEPLOYED)
	af->dragChute = AirframeClass::DRAGC_TRAILING;
    if (af->dragChute == AirframeClass::DRAGC_DEPLOYED &&
	af->vcas > af->auxaeroData->dragChuteMaxSpeed) {
	if ((af->vcas - af->auxaeroData->dragChuteMaxSpeed) / 100 > PRANDFloatPos())
	    af->dragChute = AirframeClass::DRAGC_RIPPED;
    }
    if (af->dragChute != GetSwitch(type)) {
	if ( gACMIRec.IsRecording() )
	{
	    acmiSwitch.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
	    acmiSwitch.data.type = Type();
	    acmiSwitch.data.uniqueID = Id();
	    acmiSwitch.data.switchNum = type;
	    acmiSwitch.data.prevSwitchVal = GetSwitch (type);
	    acmiSwitch.data.switchVal = af->dragChute;
	    gACMIRec.SwitchRecord( &acmiSwitch );
	}
	if (af->dragChute == AirframeClass::DRAGC_DEPLOYED)
	    F4SoundFXSetPos( af->auxaeroData->sndDragChute, TRUE, af->x, af->y, af->z, 1.0f );
	
	SetSwitch(type, af->dragChute);
    }
}

void AircraftClass::MoveSurfaces (void)
{
float qFactor, stabAngle, dofDesired;
int gFact;
int i, stage;

   qFactor = 100.0F / (max (af->vcas, 100.0F));


	// are we in easter-egg heli mode?
	if ( af && af->GetSimpleMode() == SIMPLE_MODE_HF )
	{
/*
      Switch and Dof settings for Helo
      sw0	bit1=fast moving blades; bit2=slow moving blades 	
      sw2	makes Muzzle flash visible	
      dof2	main rotor 	
      dof3	Muzzle Flash	
      dof4	tail or secondary rotor	
*/
		// animate rotors
		if ( GetSwitch(HELI_ROTORS) != 1 )
		{
			SetSwitch( HELI_ROTORS, 1);
			SetDOF( HELI_MAIN_ROTOR, 0.0f );
			SetDOF( HELI_TAIL_ROTOR, 0.0f );
		}

		// blades rotate at around 300 RPM which is 1800 DPS
		float curDOF = GetDOFValue( HELI_MAIN_ROTOR );
		float deltaDOF = 1800.0f * DTR * SimLibMajorFrameTime;
		curDOF += deltaDOF;
		if ( curDOF > 360.0f * DTR )
			curDOF -= 360.0f * DTR;

		SetDOF( HELI_MAIN_ROTOR, curDOF );
		SetDOF( HELI_TAIL_ROTOR, curDOF );
		return;
	}

   if (IsComplex())
   {
/*
      Switch and Dof definition for F16 aircraft
         Switches:
		         sw0 = Afterburner (to be linked to *.dyn file for animation)
		         sw1 = Landing Gear DOORS
		         sw2 = Landing Gear DOOR HOLES
		         sw3 = LANDING GEAR(Wing,Breast, and Strobe Lights)
		         sw4 = Front Landing Gear ROD
		         sw5 = Canopy
		         sw6 = Wing Root Vapor
		         sw7 = Lights(Volume Landing Light on right gear)
		         sw8 = Landing Lights(diamond on left landing gear)
		         sw9 = Muzzle Flash
		         sw10 = Inside burner cone
		         sw11 = TIRN pod on left Breast
		         sw12 = HTS pod on right Breast
		         sw13 = Refueling door	

         DOFs:
		         dof0 = Left Stabilizer
		         dof1 = Right Stabilizer
		         dof2 = Left Flap
		         dof3 = Right Flap
		         dof4 = Rudder
		         dof5 = Nosewheel Lateral
		         dof6 = Nosewheel Compression Translator
		         dof7 = Left Rear Gear Compression Translator
		         dof8 = Right Rear Gear Compression Translator
               dof9 = Left Leading Edge Flap
		         dof10 = Right Leading Edge Flap
		         dofs11-14 = NOTHING
               dof15 = Left Air brake TOP
		         dof16 = Left Air brake BOTTOM
		         dof17 = Right Air brake TOP
		         dof18 = Right Air brake BOTTOM
		         dof19 = Front Landing Gear
		         dof20 = Left Rear Wheel
		         dof21 = Right Rear Wheel
		         dof22 = Front Landing Gear DOOR
		         dof23 = Left Rear Landing Gear DOOR
		         dof24 = Right Rear Landing Gear DOOR
*/
      
      // Tail Surface
	   float leftStab, rightStab;
	   CalculateStab(qFactor, &leftStab, &rightStab);
      SetDOF(COMP_LT_STAB, -leftStab);
      SetDOF(COMP_RT_STAB, -rightStab);

      float aileronleft, aileronrt;
      float flapleft, flaprt;

      CalculateAileronAndFlap(qFactor, &aileronleft, &aileronrt, &flapleft, &flaprt);
      if (af->auxaeroData->hasFlapperons) 
	  {
		  MoveDof(COMP_LT_FLAP, aileronleft + flapleft, af->auxaeroData->tefRate);
		  MoveDof(COMP_RT_FLAP, aileronrt + flaprt,  af->auxaeroData->tefRate);
      }
      else 
	  {
		  MoveDof(COMP_LT_FLAP, aileronleft, af->auxaeroData->tefRate);
		  MoveDof(COMP_RT_FLAP, aileronrt, af->auxaeroData->tefRate);
		  if (af->auxaeroData->hasTef == AUX_LEFTEF_MANUAL) {
		  MoveDof(COMP_LT_TEF, flapleft, af->auxaeroData->tefRate, af->auxaeroData->sndFlapStart, af->auxaeroData->sndFlapLoop, af->auxaeroData->sndFlapEnd);
		  MoveDof(COMP_RT_TEF, flaprt, af->auxaeroData->tefRate, af->auxaeroData->sndFlapStart, af->auxaeroData->sndFlapLoop, af->auxaeroData->sndFlapEnd);
		  }
		  else {
		  MoveDof(COMP_LT_TEF, flapleft, af->auxaeroData->tefRate);
		  MoveDof(COMP_RT_TEF, flaprt, af->auxaeroData->tefRate);
		  }
	  }
	  
	  // Rudder
	  stabAngle = -CalculateRudder(qFactor);
	  SetDOF(COMP_RUDDER, stabAngle);

	  // LEF
	  if (af->auxaeroData->hasLef) 
	  {
		  
		  CalculateLef(qFactor);
		  
		  MoveDof(COMP_LT_LEF, leftLEFAngle, af->auxaeroData->lefRate);
		  MoveDof(COMP_RT_LEF, rightLEFAngle, af->auxaeroData->lefRate);
	  }
      // Set gear stuff
      RunGear();

      // Nozzle position
	  if (IsLocal ())
	  {
		  int ab =0;
		  if (af->rpm < 1.0F) // Mil or less
		  {
			  stage = FloatToInt32((af->rpm - 0.7F) * 13.3333F);
		  }
		  else
		  {
			  stage = FloatToInt32((af->rpm - 1.0F) * 233.0F) + 4;
			  ab = 11;
		  }

		  stage = max (0, stage);
//me123 only transmit this if it's changed.
		  if (ab != GetAfterburnerStage ())
		  SetAfterburnerStage (ab);
	  }
	  else
	  {
		  stage = GetAfterburnerStage ();
		  
		  if (stage >= 4)
		  {
			  af->rpm = (stage - 4) / 233.0F + 1.0F;
		  }
		  else
		  {
			  af->rpm = (stage / 13.3333F) + 0.7F;
		  }
	  }

      if ((af->rpm > 1.0F) != GetSwitch(COMP_AB))
      {
		 if ( gACMIRec.IsRecording() )
		 {
				acmiSwitch.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
				acmiSwitch.data.type = Type();
				acmiSwitch.data.uniqueID = Id();
				acmiSwitch.data.switchNum = COMP_AB;
				acmiSwitch.data.prevSwitchVal = GetSwitch (COMP_AB);
				acmiSwitch.data.switchVal = (af->rpm > 1.0f );
				gACMIRec.SwitchRecord( &acmiSwitch );
		 }
         SetSwitch (COMP_AB, (af->rpm > 1.0F));
      }

      // Afterbuner cone
      if (af->rpm >= 1.0F)
      {
         for (i=0; i<6; i++)
         {
            VertexData[i] = max (0.0F, 18.0F - 600.0F * (af->rpm - 1.0F));
         }
      }

      if (1 << stage != GetSwitch (COMP_EXH_NOZZLE))
      {
		  SetSwitch (COMP_EXH_NOZZLE, 1 << stage);
      }

	  if (IsLocal ())
	  {
		  if (af->dbrake < 0.4F)
		  {
			 UnSetFlag(AIR_BRAKES_OUT);
		  }
		  else if (af->dbrake > 0.6F)
		  {
			 SetFlag(AIR_BRAKES_OUT);
		  }
	  }
	  else
	  {
		  if (IsSetFlag (AIR_BRAKES_OUT))
		  {
			  af->dbrake = 1.0;
		  }
		  else
		  {
			  af->dbrake = 0.0;
		  }
	  }

	  stabAngle = af->dbrake * af->auxaeroData->airbrakeMaxAngle * DTR;

      SetDOF(COMP_LT_AIR_BRAKE_TOP, stabAngle);
      SetDOF(COMP_LT_AIR_BRAKE_BOT, stabAngle);
      SetDOF(COMP_RT_AIR_BRAKE_TOP, stabAngle);
      SetDOF(COMP_RT_AIR_BRAKE_BOT, stabAngle);

      gFact = min(7, max (0, FloatToInt32((af->nzcgb - 4.0F) * 1.5F)));
      if (gFact != GetSwitch (COMP_WING_VAPOR))
      {
		 if ( gACMIRec.IsRecording() )
		 {
				acmiSwitch.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
				acmiSwitch.data.type = Type();
				acmiSwitch.data.uniqueID = Id();
				acmiSwitch.data.switchNum = COMP_WING_VAPOR;
				acmiSwitch.data.prevSwitchVal = GetSwitch (COMP_WING_VAPOR);
				acmiSwitch.data.switchVal = gFact;
				gACMIRec.SwitchRecord( &acmiSwitch );
		 }
         SetSwitch (COMP_WING_VAPOR, gFact);
      }
      if (af->IsEngineFlag(AirframeClass::FuelDoorOpen) != GetSwitch(COMP_REFUEL_DR)) {
	  if ( gACMIRec.IsRecording() )
	  {
	      acmiSwitch.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
	      acmiSwitch.data.type = Type();
	      acmiSwitch.data.uniqueID = Id();
	      acmiSwitch.data.switchNum = COMP_REFUEL_DR;
	      acmiSwitch.data.prevSwitchVal = GetSwitch (COMP_REFUEL_DR);
	      acmiSwitch.data.switchVal = af->IsEngineFlag(AirframeClass::FuelDoorOpen);
	      gACMIRec.SwitchRecord( &acmiSwitch );
	  }
	  SetSwitch(COMP_REFUEL_DR, af->IsEngineFlag(AirframeClass::FuelDoorOpen));
      }
      if (af->auxaeroData->dragChuteCd > 0)
	  DeployDragChute(COMP_DRAGCHUTE);

      if (af->canopyState) { // canopy open
	  MoveDof (COMP_CANOPY_DOF, af->auxaeroData->canopyMaxAngle * DTR, af->auxaeroData->canopyRate);
      }
      else { // canopy shut
	  MoveDof (COMP_CANOPY_DOF, 0, af->auxaeroData->canopyRate);
      }

   }
   else
   {
       float leftStab, rightStab;
       CalculateStab(qFactor, &leftStab, &rightStab);
       SetDOF(SIMP_LT_STAB, -leftStab);
       SetDOF(SIMP_RT_STAB, -rightStab);
       
       float aileronleft, aileronrt;
       float flapleft, flaprt;
       CalculateAileronAndFlap(qFactor, &aileronleft, &aileronrt, &flapleft, &flaprt);
       //      stabAngle = af->rstick * qFactor * af->auxaeroData->aileronMaxAngle * DTR;
       if (af->auxaeroData->hasFlapperons) 
       {
	   SetDOF(SIMP_LT_AILERON, aileronleft + flapleft);
	   SetDOF(SIMP_RT_AILERON, aileronrt + flaprt);
       }
       else 
       {
	   SetDOF(SIMP_LT_AILERON, aileronleft);
	   SetDOF(SIMP_RT_AILERON, aileronrt);
	   if (af->auxaeroData->hasTef == AUX_LEFTEF_MANUAL) {
	       MoveDof(SIMP_LT_TEF, flapleft, af->auxaeroData->tefRate, af->auxaeroData->sndFlapStart, af->auxaeroData->sndFlapLoop, af->auxaeroData->sndFlapEnd);
	       MoveDof(SIMP_RT_TEF, flaprt, af->auxaeroData->tefRate, af->auxaeroData->sndFlapStart, af->auxaeroData->sndFlapLoop, af->auxaeroData->sndFlapEnd);
	   } else {
	       MoveDof(SIMP_LT_TEF, flapleft, af->auxaeroData->tefRate);
	       MoveDof(SIMP_RT_TEF, flaprt, af->auxaeroData->tefRate);
	   }
       }
      stabAngle = CalculateRudder(qFactor);
      SetDOF(SIMP_RUDDER_1, stabAngle);
      SetDOF(SIMP_RUDDER_2, stabAngle);

      stabAngle = af->dbrake * af->auxaeroData->airbrakeMaxAngle * DTR;
      SetDOF(SIMP_AIR_BRAKE, stabAngle);

	  if (af->auxaeroData->hasLef) {
		  CalculateLef(qFactor);
		  MoveDof(SIMP_LT_LEF, leftLEFAngle, af->auxaeroData->lefRate, -1);
		  MoveDof(SIMP_RT_LEF, rightLEFAngle, af->auxaeroData->lefRate, -1);
	  }

      if ((af->rpm > 1.0F) != GetSwitch (SIMP_AB))
      {
		 /*
		 ** Record AB for non f16's?
		 if ( gACMIRec.IsRecording() )
		 {
				acmiSwitch.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
				acmiSwitch.data.type = Type();
				acmiSwitch.data.uniqueID = Id();
				acmiSwitch.data.switchNum = SIMP_AB;
				acmiSwitch.data.prevSwitchVal = switchData[SIMP_AB];
				acmiSwitch.data.switchVal = (af->rpm > 1.0f );
				gACMIRec.SwitchRecord( &acmiSwitch );
		 }
		 */
         SetSwitch (SIMP_AB, af->rpm > 1.0F);
      }

      if ((af->gearPos > 0.5F) != GetSwitch (SIMP_GEAR))
      {
		 if ( gACMIRec.IsRecording() )
		 {
				acmiSwitch.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
				acmiSwitch.data.type = Type();
				acmiSwitch.data.uniqueID = Id();
				acmiSwitch.data.switchNum = SIMP_GEAR;
				acmiSwitch.data.prevSwitchVal = GetSwitch (SIMP_GEAR);
				acmiSwitch.data.switchVal = (af->gearPos > 0.5f );
				gACMIRec.SwitchRecord( &acmiSwitch );
		 }
         SetSwitch (SIMP_GEAR, af->gearPos > 0.5F);
      }

      gFact = min(7, max (0, FloatToInt32((af->nzcgb - 4.0F) * 1.5F)));
      if (gFact != GetSwitch (SIMP_WING_VAPOR))
      {
		 /*
		 ** record wing vapor for non-f16's?
		 if ( gACMIRec.IsRecording() )
		 {
				acmiSwitch.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
				acmiSwitch.data.type = Type();
				acmiSwitch.data.uniqueID = Id();
				acmiSwitch.data.switchNum = SIMP_WING_VAPOR;
				acmiSwitch.data.prevSwitchVal = switchData[SIMP_WING_VAPOR];
				acmiSwitch.data.switchVal = gFact;
				gACMIRec.SwitchRecord( &acmiSwitch );
		 }
		 */
         SetSwitch (SIMP_WING_VAPOR, gFact);
      }

	  /*
      switchData[5] = 1;
      switchChange[5] = TRUE;
	  if ( gACMIRec.IsRecording() )
	  {
			acmiSwitch.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
			acmiSwitch.data.type = Type();
			acmiSwitch.data.uniqueID = ACMIIDTable->Add(Id(),NULL,0);//.num_;
			acmiSwitch.data.switchNum = 5;
			acmiSwitch.data.switchVal = switchData[5];
			gACMIRec.SwitchRecord( &acmiSwitch );
	  }
	  */

      // Special Mig23 Wing Swing
      if (acFlags & hasSwing)
      {
	  static const int swdofs[] = 
	  {SIMP_SWING_WING_1, SIMP_SWING_WING_2, SIMP_SWING_WING_3,
	  SIMP_SWING_WING_4,SIMP_SWING_WING_5,SIMP_SWING_WING_6,
	  SIMP_SWING_WING_7,SIMP_SWING_WING_8};
         // Move Wings
         if (af->mach > 0.8F || (GetDOFValue(SIMP_SWING_WING_1) > 30.0F * DTR && af->mach > 0.7F))
         {
            dofDesired = 50.0F * DTR;
         }
         else if (af->mach > 0.6F || (GetDOFValue(SIMP_SWING_WING_1) < 5.0F * DTR && af->mach > 0.5F))
         {
            dofDesired = 25.0F * DTR;
         }
         else
         {
            dofDesired = 0.0F * DTR;
         }
	 MoveDof(SIMP_SWING_WING_1, dofDesired, 5.0f);
	 for (int i = 1; i < sizeof(swdofs)/sizeof(swdofs[0]); i++)
	     SetDOF(swdofs[i], GetDOFValue(SIMP_SWING_WING_1));
      }
      if (af->auxaeroData->dragChuteCd > 0 )
	  DeployDragChute(SIMP_DRAGCHUTE);
      if (af->canopyState) { // canopy open
	  MoveDof (SIMP_CANOPY_DOF, af->auxaeroData->canopyMaxAngle * DTR, af->auxaeroData->canopyRate);
      }
      else { // canopy shut
	  MoveDof (SIMP_CANOPY_DOF, 0, af->auxaeroData->canopyRate);
      }
   }

   // Check for stick shake
   if (this == SimDriver.playerEntity)
   {
      if (GetAlpha() > 15.0F && GetAlpha() < 20.0F)
      {
         if (!stallShake)
         {
            stallShake = TRUE;
            JoystickPlayEffect (JoyStall1, 0);
            JoystickPlayEffect (JoyStall2, 0);
         }
      }
      else if (stallShake)
      {
         stallShake = FALSE;
         JoystickStopEffect (JoyStall1);
         JoystickStopEffect (JoyStall2);
      }
   }
}

void AircraftClass::RunGear(void)
{
	//this function is only valid for airplanes with complex gear
	int switch1 = GetSwitch(COMP_NOS_GEAR_SW);
	int switch2 = GetSwitch(COMP_LT_GEAR_SW);
	int switch3 = GetSwitch(COMP_RT_GEAR_SW);
	int switch4 = GetSwitch(COMP_NOS_GEAR_ROD);
	int switch7 = GetSwitch(COMP_TAIL_STROBE);
	int switch8 = GetSwitch(COMP_NAV_LIGHTS);
	int switch9 = GetSwitch(COMP_LAND_LIGHTS);
	int switch14 = GetSwitch(COMP_NOS_GEAR_DR_SW);
	int switch15 = GetSwitch(COMP_LT_GEAR_DR_SW);
	int switch16 = GetSwitch(COMP_RT_GEAR_DR_SW);
	int switch17 = GetSwitch(COMP_NOS_GEAR_HOLE);
	int switch18 = GetSwitch(COMP_LT_GEAR_HOLE);
	int switch19 = GetSwitch(COMP_RT_GEAR_HOLE);
	//int switch20 = GetSwitch(COMP_BROKEN_NOS_GEAR);
	//int switch21 = GetSwitch(COMP_BROKEN_LT_GEAR);
	//int switch22 = GetSwitch(COMP_BROKEN_RT_GEAR);

	float dof19 = GetDOFValue(COMP_NOS_GEAR);
	float dof20 = GetDOFValue(COMP_LT_GEAR);
	float dof21 = GetDOFValue(COMP_RT_GEAR);
	float dof22 = GetDOFValue(COMP_NOS_GEAR_DR);
	float dof23 = GetDOFValue(COMP_LT_GEAR_DR);
	float dof24 = GetDOFValue(COMP_RT_GEAR_DR);

	int i;

	if (IsLocal ())
	{
		if ((af->gearHandle > 0.0F || OnGround()) && !af->IsSet(AirframeClass::GearBroken))
		{
		if (VuState() == VU_MEM_ACTIVE) SetStatusBit (STATUS_GEAR_DOWN);
		}
		else
		{
			ClearStatusBit (STATUS_GEAR_DOWN);
		}
	}
		
	for(i = 0; i < af->NumGear(); i++)
	{			
		//move the door
		if(!(af->gear[i].flags & GearData::DoorStuck))
		{
			SetDOF(COMP_NOS_GEAR_DR + i, min(1.0F, af->gearPos*2.0F) * af->GetAeroData(AeroDataSet::NosGearRng + i*4)*DTR);
			SetDOF(COMP_NOS_GEAR_DR + i, max(GetDOFValue(COMP_NOS_GEAR_DR + i), GetDOFValue(COMP_NOS_GEAR + i)));
		}

		//move the gear
		if(!(af->gear[i].flags & GearData::GearStuck))
		{
			SetDOF(COMP_NOS_GEAR + i, max( (af->gearPos - 0.5F)*2.0F, 0.0F ) * af->GetAeroData(AeroDataSet::NosGearRng + i*4)*DTR);
			SetDOF(COMP_NOS_GEAR + i, min(GetDOFValue(COMP_NOS_GEAR + i), GetDOFValue(COMP_NOS_GEAR_DR + i)));
		}
		
		if(af->gear[i].flags & GearData::DoorBroken || GetDOFValue(COMP_NOS_GEAR_DR + i) < 5.0F*DTR)
			SetSwitch (COMP_NOS_GEAR_DR_SW + i, FALSE);
		else
			SetSwitch (COMP_NOS_GEAR_DR_SW + i, TRUE);

		if(GetDOFValue(COMP_NOS_GEAR_DR + i) > 5.0F*DTR || af->gear[i].flags & GearData::DoorBroken)
			SetSwitch (COMP_NOS_GEAR_HOLE + i, TRUE);
		else
			SetSwitch (COMP_NOS_GEAR_HOLE + i, FALSE);

		if(GetDOFValue(COMP_NOS_GEAR + i) > 5.0F*DTR && !(af->gear[i].flags & GearData::GearBroken))
			SetSwitch (COMP_NOS_GEAR_SW + i, TRUE);
		else
			SetSwitch (COMP_NOS_GEAR_SW + i, FALSE);
	}

	if(af->IsSet(AirframeClass::NoseSteerOn))
	{
		if(!(af->gear[0].flags & GearData::GearStuck))
		{
			if(IO.AnalogIsUsed(3) && !af->IsSet(AirframeClass::IsDigital))
				SetDOF(COMP_NOS_GEAR_ROT, -af->ypedal*30.0F*DTR*(0.5F + (80.0F*KNOTS_TO_FTPSEC - af->vt)/(160.0F * KNOTS_TO_FTPSEC)));
			else
				SetDOF(COMP_NOS_GEAR_ROT, af->rstick*30.0F*DTR*(0.5F + (80.0F*KNOTS_TO_FTPSEC - af->vt)/(160.0F * KNOTS_TO_FTPSEC)));
		}
	}
	else 
	{
		SetDOF(COMP_NOS_GEAR_ROT,GetDOFValue(COMP_NOS_GEAR_ROT)*0.9F);
	}


	//MI flashing WingLights
	if(IsPlayer() && g_bRealisticAvionics)
	{
		//Power on
		if(ExtlState(AircraftClass::ExtlLightFlags::Extl_Main_Power))
		{
			//Wing Light
			if(ExtlState(AircraftClass::ExtlLightFlags::Extl_Wing_Tail))
			{
				if(ExtlState(AircraftClass::ExtlLightFlags::Extl_Steady_Flash))
				{
					if(vuxGameTime & 0x200)
					{
						ClearStatusBit(STATUS_EXT_NAVLIGHTS);
					}
					else
					{
						SetStatusBit(STATUS_EXT_NAVLIGHTS);
					}
				}
				else
				{
					SetStatusBit(STATUS_EXT_NAVLIGHTS);
				}
			}
			else
			{
				ClearStatusBit(STATUS_EXT_NAVLIGHTS);
			}
			//Strobe Light
			if(ExtlState(AircraftClass::ExtlLightFlags::Extl_Anit_Coll))
			{
				SetStatusBit(STATUS_EXT_TAILSTROBE);
			}
			else
			{
				ClearStatusBit(STATUS_EXT_TAILSTROBE);
			}
		}
		else
		{
			ClearStatusBit(STATUS_EXT_NAVLIGHTS|STATUS_EXT_TAILSTROBE);
		}
		//Landing Light
		if(!(af->gear[1].flags & GearData::GearBroken) && af->LLON && af->gearPos == 1.0F)
		{
			SetSwitch(COMP_LAND_LIGHTS, TRUE);
			SetStatusBit(STATUS_EXT_LIGHTS|STATUS_EXT_LANDINGLIGHT);
		}
		else 
		{
			SetSwitch (COMP_LAND_LIGHTS, FALSE);
			ClearStatusBit(STATUS_EXT_LANDINGLIGHT);
		}
		if(GetSwitch(COMP_TAIL_STROBE) == 0 && (Status() & STATUS_EXT_TAILSTROBE))
		{
			SetSwitch(COMP_TAIL_STROBE, TRUE);	// Strobe
			SetCockpitStrobeLight(TRUE);
		}
		else
		{
			SetSwitch (COMP_TAIL_STROBE, FALSE);	// Strobe			
			SetCockpitStrobeLight(FALSE);
		}
		if(Status() & STATUS_EXT_NAVLIGHTS)
		{
			SetSwitch(COMP_NAV_LIGHTS, TRUE);
			SetCockpitWingLight(TRUE);
		}
		else
		{
			SetSwitch(COMP_NAV_LIGHTS, FALSE);
			SetCockpitWingLight(FALSE);
		}		
	}
	//AI or non realistic stuff
	else
	{
		if (af->gearPos == 1.0F) 
		{
			SetSwitch (COMP_NOS_GEAR_ROD, TRUE);
			if(GetSwitch(COMP_TAIL_STROBE) == 0)
				SetSwitch(COMP_TAIL_STROBE, TRUE);
			
			SetSwitch(COMP_NAV_LIGHTS, TRUE);

		
		    if(!(af->gear[1].flags & GearData::GearBroken)) 
			{
				SetSwitch (COMP_LAND_LIGHTS, TRUE); // Landing Lights
				SetStatusBit (STATUS_EXT_LIGHTS|STATUS_EXT_LANDINGLIGHT);
			}
		    else
			{
				SetSwitch (COMP_LAND_LIGHTS, FALSE); // Landing Lights
				ClearStatusBit(STATUS_EXT_LANDINGLIGHT);
				if ((Status() & STATUS_EXT_LIGHTMASK) == STATUS_EXT_LIGHTS)
					ClearStatusBit(STATUS_EXT_LIGHTS);
			}
		}
		else 
		{
			SetSwitch (COMP_TAIL_STROBE, FALSE);
			SetSwitch (COMP_NAV_LIGHTS, FALSE);

			SetSwitch (COMP_NOS_GEAR_ROD, FALSE);
			SetSwitch (COMP_LAND_LIGHTS, FALSE);
		}
	}

	if (GetDOFValue(COMP_NOS_GEAR + i) == af->GetAeroData(AeroDataSet::NosGearRng)*DTR && !(af->gear[0].flags & GearData::DoorBroken))
		SetSwitch (COMP_NOS_GEAR_ROD, TRUE);
	else
		SetSwitch (COMP_NOS_GEAR_ROD, FALSE);


	
	if ( gACMIRec.IsRecording() )
	{
		acmiSwitch.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
		acmiSwitch.data.type = Type();
		acmiSwitch.data.uniqueID = ACMIIDTable->Add(Id(),NULL,0);//.num_;
		DOFRec.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
		DOFRec.data.type = Type();
		DOFRec.data.uniqueID = ACMIIDTable->Add(Id(),NULL,0);//.num_;
		
		if ( switch1 != GetSwitch(COMP_NOS_GEAR_SW) )
		{
			acmiSwitch.data.switchNum = COMP_NOS_GEAR_SW;
			acmiSwitch.data.switchVal = GetSwitch(COMP_NOS_GEAR_SW);
			acmiSwitch.data.prevSwitchVal = switch1;
			gACMIRec.SwitchRecord( &acmiSwitch );
		}
		if ( switch2 != GetSwitch(COMP_LT_GEAR_SW) )
		{
			acmiSwitch.data.switchNum = COMP_LT_GEAR_SW;
			acmiSwitch.data.switchVal = GetSwitch(COMP_LT_GEAR_SW);
			acmiSwitch.data.prevSwitchVal = switch2;
			gACMIRec.SwitchRecord( &acmiSwitch );
		}
		if ( switch3 != GetSwitch(COMP_RT_GEAR_SW) )
		{
			acmiSwitch.data.switchNum = COMP_RT_GEAR_SW;
			acmiSwitch.data.switchVal = GetSwitch(COMP_RT_GEAR_SW);
			acmiSwitch.data.prevSwitchVal = switch3;
			gACMIRec.SwitchRecord( &acmiSwitch );
		}
		if ( switch4 != GetSwitch(COMP_NOS_GEAR_ROD) )
		{
			acmiSwitch.data.switchNum = COMP_NOS_GEAR_ROD;
			acmiSwitch.data.switchVal = GetSwitch(COMP_NOS_GEAR_ROD);
			acmiSwitch.data.prevSwitchVal = switch4;
			gACMIRec.SwitchRecord( &acmiSwitch );
		}
		if ( switch7 != GetSwitch(COMP_TAIL_STROBE) )
		{
			acmiSwitch.data.switchNum = COMP_TAIL_STROBE;
			acmiSwitch.data.switchVal = GetSwitch(COMP_TAIL_STROBE);
			acmiSwitch.data.prevSwitchVal = switch7;
			gACMIRec.SwitchRecord( &acmiSwitch );
		}
		if ( switch8 != GetSwitch(COMP_NAV_LIGHTS) )
		{
			acmiSwitch.data.switchNum = COMP_NAV_LIGHTS;
			acmiSwitch.data.switchVal = GetSwitch(COMP_NAV_LIGHTS);
			acmiSwitch.data.prevSwitchVal = switch8;
			gACMIRec.SwitchRecord( &acmiSwitch );
		}
		if ( switch9 != GetSwitch(COMP_LAND_LIGHTS) )
		{
			acmiSwitch.data.switchNum = COMP_LAND_LIGHTS;
			acmiSwitch.data.switchVal = GetSwitch(COMP_LAND_LIGHTS);
			acmiSwitch.data.prevSwitchVal = switch9;
			gACMIRec.SwitchRecord( &acmiSwitch );
		}
		if ( switch14 != GetSwitch(COMP_NOS_GEAR_DR_SW) )
		{
			acmiSwitch.data.switchNum = COMP_NOS_GEAR_DR_SW;
			acmiSwitch.data.switchVal = GetSwitch(COMP_NOS_GEAR_DR_SW);
			acmiSwitch.data.prevSwitchVal = switch14;
			gACMIRec.SwitchRecord( &acmiSwitch );
		}
		if ( switch15 != GetSwitch(COMP_LT_GEAR_DR_SW) )
		{
			acmiSwitch.data.switchNum = COMP_LT_GEAR_DR_SW;
			acmiSwitch.data.switchVal = GetSwitch(COMP_LT_GEAR_DR_SW);
			acmiSwitch.data.prevSwitchVal = switch15;
			gACMIRec.SwitchRecord( &acmiSwitch );
		}
		if ( switch16 != GetSwitch(COMP_RT_GEAR_DR_SW) )
		{
			acmiSwitch.data.switchNum = COMP_RT_GEAR_DR_SW;
			acmiSwitch.data.switchVal = GetSwitch(COMP_RT_GEAR_DR_SW);
			acmiSwitch.data.prevSwitchVal = switch16;
			gACMIRec.SwitchRecord( &acmiSwitch );
		}
		if ( switch17 != GetSwitch(COMP_NOS_GEAR_HOLE) )
		{
			acmiSwitch.data.switchNum = COMP_NOS_GEAR_HOLE;
			acmiSwitch.data.switchVal = GetSwitch(COMP_NOS_GEAR_HOLE);
			acmiSwitch.data.prevSwitchVal = switch17;
			gACMIRec.SwitchRecord( &acmiSwitch );
		}
		if ( switch18 != GetSwitch(COMP_LT_GEAR_HOLE) )
		{
			acmiSwitch.data.switchNum = COMP_LT_GEAR_HOLE;
			acmiSwitch.data.switchVal = GetSwitch(COMP_LT_GEAR_HOLE);
			acmiSwitch.data.prevSwitchVal = switch18;
			gACMIRec.SwitchRecord( &acmiSwitch );
		}
		if ( switch19 != GetSwitch(COMP_RT_GEAR_HOLE) )
		{
			acmiSwitch.data.switchNum = COMP_RT_GEAR_HOLE;
			acmiSwitch.data.switchVal = GetSwitch(COMP_RT_GEAR_HOLE);
			acmiSwitch.data.prevSwitchVal = switch19;
			gACMIRec.SwitchRecord( &acmiSwitch );
		}
/*		if ( switch20 != GetSwitch(COMP_BROKEN_NOS_GEAR) )
		{
			acmiSwitch.data.switchNum = COMP_BROKEN_NOS_GEAR;
			acmiSwitch.data.switchVal = GetSwitch(COMP_BROKEN_NOS_GEAR);
			acmiSwitch.data.prevSwitchVal = switch20;
			gACMIRec.SwitchRecord( &acmiSwitch );
		}
		if ( switch21 != GetSwitch(COMP_BROKEN_LT_GEAR) )
		{
			acmiSwitch.data.switchNum = COMP_BROKEN_LT_GEAR;
			acmiSwitch.data.switchVal = GetSwitch(COMP_BROKEN_LT_GEAR);
			acmiSwitch.data.prevSwitchVal = switch21;
			gACMIRec.SwitchRecord( &acmiSwitch );
		}
		if ( switch22 != GetSwitch(COMP_BROKEN_RT_GEAR) )
		{
			acmiSwitch.data.switchNum = COMP_BROKEN_RT_GEAR;
			acmiSwitch.data.switchVal = GetSwitch(COMP_BROKEN_RT_GEAR);
			acmiSwitch.data.prevSwitchVal = switch22;
			gACMIRec.SwitchRecord( &acmiSwitch );
		}*/
		
		if ( dof19 != GetDOFValue(COMP_NOS_GEAR) )
		{
			DOFRec.data.DOFNum = COMP_NOS_GEAR;
			DOFRec.data.DOFVal = GetDOFValue(COMP_NOS_GEAR);
			DOFRec.data.prevDOFVal = dof19;
			gACMIRec.DOFRecord( &DOFRec );
		}
		if ( dof20 != GetDOFValue(COMP_LT_GEAR) )
		{
			DOFRec.data.DOFNum = COMP_LT_GEAR;
			DOFRec.data.DOFVal = GetDOFValue(COMP_LT_GEAR);
			DOFRec.data.prevDOFVal = dof20;
			gACMIRec.DOFRecord( &DOFRec );
		}
		if ( dof21 != GetDOFValue(COMP_RT_GEAR) )
		{
			DOFRec.data.DOFNum = COMP_RT_GEAR;
			DOFRec.data.DOFVal = GetDOFValue(COMP_RT_GEAR);
			DOFRec.data.prevDOFVal = dof21;
			gACMIRec.DOFRecord( &DOFRec );
		}
		if ( dof22 != GetDOFValue(COMP_NOS_GEAR_DR) )
		{
			DOFRec.data.DOFNum = COMP_NOS_GEAR_DR;
			DOFRec.data.DOFVal = GetDOFValue(COMP_NOS_GEAR_DR);
			DOFRec.data.prevDOFVal = dof22;
			gACMIRec.DOFRecord( &DOFRec );
		}
		if ( dof23 != GetDOFValue(COMP_LT_GEAR_DR) )
		{
			DOFRec.data.DOFNum = COMP_LT_GEAR_DR;
			DOFRec.data.DOFVal = GetDOFValue(COMP_LT_GEAR_DR);
			DOFRec.data.prevDOFVal = dof23;
			gACMIRec.DOFRecord( &DOFRec );
		}
		if ( dof24 != GetDOFValue(COMP_RT_GEAR_DR) )
		{
			DOFRec.data.DOFNum = COMP_RT_GEAR_DR;
			DOFRec.data.DOFVal = GetDOFValue(COMP_RT_GEAR_DR);
			DOFRec.data.prevDOFVal = dof24;
			gACMIRec.DOFRecord( &DOFRec );
		}
		
	}
}
//MI
float AircraftClass::CheckLEF(int side)
{
	float rLEF;
	float lLEF;
	if (IsComplex()) {
		rLEF = GetDOFValue(COMP_RT_LEF);
		lLEF = GetDOFValue(COMP_LT_LEF);
	}
	else {
		rLEF = GetDOFValue(SIMP_RT_LEF);
		lLEF = GetDOFValue(SIMP_LT_LEF);
	}
	//side 0 = left
	switch(side)
	{
	case 0:
		//Left LEF
		if(LEFState(LT_LEF_OUT) || LEFState(LEFSASYNCH))	//can't work anymore
			leftLEFAngle = LTLEFAOA;
		else	//normal operation
		{
			if(LEFLocked)	
				leftLEFAngle = lLEF;
		}
		return leftLEFAngle;
	break;
	case 1:
		//Right LEF
		if(LEFState(RT_LEF_OUT) || LEFState(LEFSASYNCH))	//can't work anymore
			rightLEFAngle = RTLEFAOA;	//got set when we took hit
		else	//normal operation
		{
			if(LEFLocked)
				rightLEFAngle = rLEF;
		}

		return rightLEFAngle;
	break;
	default:
		return 0.0F;
	break;
	}
	return 0.0F;
}
