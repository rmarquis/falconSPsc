/******************************************************************************/
/*                                                                            */
/*  Unit Name : axial.cpp                                                     */
/*                                                                            */
/*  Abstract  : Calculates addition axial forces                              */
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
#include "fack.h"
#include "aircrft.h"
#include "fsound.h"
#include "soundfx.h"
#include "otwdrive.h"
#include "fakerand.h"
#include "sfx.h"
#include "playerop.h"
#include "dofsnswitches.h"

/********************************************************************/
/*                                                                  */
/* Routine: void AirframeClass::Axial (void)                       */
/*                                                                  */
/* Description:                                                     */
/*    Calculate forces due to speed brake                           */
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
extern bool g_bRealisticAvionics;
void AirframeClass::Axial(float dt)
{
float probability;
float maxQbar, chance;
float dBrakeMax;

   /*---------------------*/
   /* speed brake command */
   /*---------------------*/

	//MI retracts our SBR if it's > 43° and gear down and locked
	if(gearPos == 1.0F && HydraulicA() != 0 && speedBrake == 0)
	{
		//if you hold the switch, they go to 60°
		if(speedBrake == 1.0F)
		{
			dbrake += 0.3F * dt * speedBrake;
		}
		//always stay where you are when on ground
		else if(platform->OnGround() && platform->Pitch() * RTD <= 0);
		else if(dbrake > (1.0F - (60.0F - 43.0F)/60.0F))
		{
			//Move the brake
			dbrake += 0.3F * dt * -1.0F;
			//Make some music
			F4SoundFXSetPos( auxaeroData->sndSpdBrakeLoop, TRUE, x, y, z, 1.0f );
		}
	}

   if (speedBrake != 0.0)
   {
      dbrake += 0.3F * dt * speedBrake;

      // Limit max speed brake to 43 degrees when gear is fully extended
	  //MI make it realistic
#if 0
        dBrakeMax = 1.0F - (60.0F - 43.0F)/60.0F * gearPos;
		dbrake = min ( max ( dbrake, 0.0F), dBrakeMax);
#else
		dBrakeMax = 1.0F;
		dbrake = min ( max ( dbrake, 0.0F), dBrakeMax);
#endif

		// do sound effect.  This has begin, end and loop pieces
		if ( speedBrake < 0.0f )
		{
			// closing brake
		 	if ( dbrake > 0.90f * dBrakeMax && dbrake < dBrakeMax &&
				 !F4SoundFXPlaying( auxaeroData->sndSpdBrakeStart) ) // JB 010425
			{
				F4SoundFXSetPos( auxaeroData->sndSpdBrakeStart, TRUE, x, y, z, 1.0f );
			}

		 	if ( dbrake < 0.10f && dbrake > 0.0f &&
				 !F4SoundFXPlaying( auxaeroData->sndSpdBrakeEnd) ) // JB 010425
			{
				F4SoundFXSetPos( auxaeroData->sndSpdBrakeEnd, TRUE, x, y, z, 1.0f );
			}

		}
		else
		{
			// opening brake
		 	if ( dbrake < 0.10f && dbrake > 0.0f &&
				 !F4SoundFXPlaying( auxaeroData->sndSpdBrakeStart) ) // JB 010425
			{
				F4SoundFXSetPos( auxaeroData->sndSpdBrakeStart, TRUE, x, y, z, 1.0f );
			}

		 	if ( dbrake > 0.90f * dBrakeMax && dbrake < dBrakeMax  &&
				 !F4SoundFXPlaying( auxaeroData->sndSpdBrakeEnd) ) // JB 010425
			{
				F4SoundFXSetPos( auxaeroData->sndSpdBrakeEnd, TRUE, x, y, z, 1.0f );
			}
		}

	 	if ( dbrake > 0.05f && dbrake < 0.95F * dBrakeMax )
			F4SoundFXSetPos( auxaeroData->sndSpdBrakeLoop, TRUE, x, y, z, 1.0f );

		//MI fix for "b" key
		if((dbrake == 1.0F || dbrake == 0) && speedBrake != 0 && BrakesToggle)
		{
			speedBrake = 0.0F;
			BrakesToggle = FALSE;
		}
   }

   if (gearHandle != 0 && !IsSet(GearBroken) && IsSet(InAir))
   {
      	gearPos += 0.3F * dt * gearHandle;
		gearPos = min ( max ( gearPos, 0.0F), 1.0F);

		// do sound effect.  This has begin, end and loop pieces
		if ( gearHandle < 0.0f )
		{
			// closing brake
		 	if ( gearPos > 0.90f && gearPos < 1.0f && !F4SoundFXPlaying(auxaeroData->sndGearCloseStart))
			{
				F4SoundFXSetPos( auxaeroData->sndGearCloseStart, TRUE, x, y, z, 1.0f );
			}

		 	if ( gearPos < 0.10f && gearPos > 0.0f && !F4SoundFXPlaying(auxaeroData->sndGearCloseEnd))
			{
				F4SoundFXSetPos( auxaeroData->sndGearCloseEnd, TRUE, x, y, z, 1.0f );
			}

		}
		else
		{
			// opening brake
		 	if ( gearPos < 0.10f && gearPos > 0.0f && !F4SoundFXPlaying(auxaeroData->sndGearOpenStart))
			{
				F4SoundFXSetPos( auxaeroData->sndGearOpenStart, TRUE, x, y, z, 1.0f );
			}

		 	if ( gearPos > 0.90f && gearPos < 1.0f && !F4SoundFXPlaying(auxaeroData->sndGearOpenEnd))
			{
				F4SoundFXSetPos( auxaeroData->sndGearOpenEnd, TRUE, x, y, z, 1.0f );
			}
		}

	 	if ( gearPos > 0.05f && gearPos < 0.95f )
			F4SoundFXSetPos( auxaeroData->sndGearLoop, TRUE, x, y, z, 1.0f );
   }

	// JB carrier start
	if (hookHandle != 0)
	{
		hookPos += 0.3F * dt * hookHandle;
		hookPos = min ( max ( hookPos, 0.0F), 1.0F);

		// do sound effect.  This has begin, end and loop pieces
		if ( hookHandle < 0.0f )
		{
			// closing hook
			if ( hookPos > 0.90f && hookPos < 1.0f && !F4SoundFXPlaying(auxaeroData->sndHookEnd))
			{
				F4SoundFXSetPos( auxaeroData->sndHookEnd, TRUE, x, y, z, 1.0f );
			}

			if ( hookPos < 0.10f && hookPos > 0.0f && !F4SoundFXPlaying(auxaeroData->sndHookStart))
			{
				F4SoundFXSetPos( auxaeroData->sndHookStart, TRUE, x, y, z, 1.0f );
			}
		}
		else
		{
			// opening hook
			if ( hookPos < 0.10f && hookPos > 0.0f && !F4SoundFXPlaying(auxaeroData->sndHookStart))
			{
				F4SoundFXSetPos( auxaeroData->sndHookStart, TRUE, x, y, z, 1.0f );
			}

			if ( hookPos > 0.90f && hookPos < 1.0f && !F4SoundFXPlaying(auxaeroData->sndHookEnd))
			{
				F4SoundFXSetPos( auxaeroData->sndHookEnd, TRUE, x, y, z, 1.0f );
			}
		}

		if ( hookPos > 0.05f && hookPos < 0.95f )
			F4SoundFXSetPos( auxaeroData->sndHookLoop, TRUE, x, y, z, 1.0f );
	}
	// JB carrier end

   //DSP hack until we can get the digis to stay slow until the gear come up
   if (!platform->IsSetFalcFlag(FEC_INVULNERABLE) && gearPos > 0.1F &&  !IsSet( GearBroken ) && !IsSet(IsDigital))
   {
	 if (gearPos > 0.9F)
      {
         maxQbar = 350.0F;
      }
	 else
	 {
		 maxQbar = 300.0F;
	 }

	 maxQbar += (IsSet(Simplified)*100.0F);

	 if (qbar > maxQbar && ((AircraftClass*)platform)->mFaults )
      {
		probability = (qbar - maxQbar)/75.0F*dt;
		chance = (float)rand()/(float)RAND_MAX;

         if ( chance < probability)
         {
				int i, numDebris=0;
			   if ( probability > 3.0F*dt)
			   {	   	   
				   ((AircraftClass*)platform)->mFaults->SetFault(FaultClass::gear_fault,
					   FaultClass::ldgr, FaultClass::fail, TRUE);
				   
				   numDebris = rand()%4 + 5;
				   
				   gearPos = 0.1F;
				   for(int i = 0; i < NumGear(); i++)
				   {
					   gear[i].flags |= GearData::GearProblem;
					   gear[i].flags |= GearData::DoorBroken;
					   if(platform->IsComplex())
					       platform->SetDOF(COMP_NOS_GEAR + i, 0.0F);
				   }
				   
				   SetFlag (GearBroken);
				   ClearFlag (WheelBrakes);
				   // gear breaks sound
				   F4SoundFXSetPos( auxaeroData->sndWheelBrakes, TRUE, x, y, z, 1.0f );

				   // gear breaks sound
				   F4SoundFXSetPos( SFX_GRNDHIT2, TRUE, x, y, z, 1.0f );
			   } 
			   else if(NumGear() > 1)
			   {
				   //we'll just bend or stick something
				   int which = rand()%6;
				   float newpos;
				
				   numDebris = rand()%2;			
				   
				   float dmg = (qbar - maxQbar)/3.0F + rand()%26;

				   switch(which)
				   {
				   case 0:
					//nose gear
				       if (platform->IsComplex())
					   newpos = platform->GetDOFValue(COMP_NOS_GEAR) - (float)rand()/(float)RAND_MAX * 5.0F*DTR;

					   gear[0].strength -= dmg;
					   if(gear[0].strength <= 0.0F)
					   {
							gear[0].flags |= GearData::GearBroken;
							if(platform->IsComplex())
							    platform->SetDOF(COMP_NOS_GEAR, 0.0F);
							F4SoundFXSetPos( auxaeroData->sndWheelBrakes, TRUE, x, y, z, 1.0f );
							SetFlag (GearDamaged);
							((AircraftClass*)platform)->mFaults->SetFault(FaultClass::gear_fault,
								FaultClass::ldgr, FaultClass::fail, TRUE);
					   }
					   else if(gear[0].strength < 50.0F)
					   {
							gear[0].flags |= GearData::GearStuck;
							platform->mFaults->SetFault(FaultClass::gear_fault,
								FaultClass::ldgr, FaultClass::fail, FALSE);
					   }

					   if(newpos > 20.0F*DTR && platform->IsComplex())
							platform->SetDOF(COMP_NOS_GEAR, newpos);
					   break;
				   case 1:
					//left gear
				       if(platform->IsComplex()) {
					       newpos = platform->GetDOFValue(COMP_LT_GEAR) + (float)rand()/(float)RAND_MAX * 5.0F*DTR;


					   if( newpos < (GetAeroData(AeroDataSet::LtGearRng) + 15.0F)*DTR )
							platform->SetDOF(COMP_LT_GEAR, newpos);
				       }

					   gear[1].strength -= dmg;
					   if(gear[1].strength <= 0.0F)
					   {
					       if(platform->IsComplex())
						   platform->SetDOF(COMP_LT_GEAR, 0.0F);
							gear[1].flags |= GearData::GearBroken;
							F4SoundFXSetPos( auxaeroData->sndWheelBrakes, TRUE, x, y, z, 1.0f );
							SetFlag (GearDamaged);
							((AircraftClass*)platform)->mFaults->SetFault(FaultClass::gear_fault,
								FaultClass::ldgr, FaultClass::fail, TRUE);
					   }
					   else if(gear[1].strength < 50.0F)
					   {
							gear[1].flags |= GearData::GearStuck;
							platform->mFaults->SetFault(FaultClass::gear_fault,
								FaultClass::ldgr, FaultClass::fail, FALSE);
					   }

					   if(platform->IsComplex() && platform->GetDOFValue(COMP_LT_GEAR) < platform->GetDOFValue(COMP_LT_GEAR_DR))
					   {
							platform->SetDOF(COMP_LT_GEAR_DR, max(platform->GetDOFValue(COMP_LT_GEAR) + (float)rand()/(float)RAND_MAX * 10.0F*DTR, 0.0F));
					   }
					   break;
				   case 2:
					//right gear
				       if (platform->IsComplex()) {
					   newpos = platform->GetDOFValue(COMP_RT_GEAR) + (float)rand()/(float)RAND_MAX * 5.0F*DTR;
					   
					   if( newpos < (GetAeroData(AeroDataSet::RtGearRng) + 15.0F)*DTR )
					       platform->SetDOF(COMP_RT_GEAR, newpos);
				       }
					   gear[2].strength -= dmg;
					   if(gear[2].strength <= 0.0F)
					   {
					       if(platform->IsComplex())
						   platform->SetDOF(COMP_RT_GEAR, 0.0F);
					       gear[2].flags |= GearData::GearBroken;
					       F4SoundFXSetPos( auxaeroData->sndWheelBrakes, TRUE, x, y, z, 1.0f );
					       SetFlag (GearDamaged);
					       ((AircraftClass*)platform)->mFaults->SetFault(FaultClass::gear_fault,
						   FaultClass::ldgr, FaultClass::fail, TRUE);
					   }
					   else if(gear[2].strength < 50.0F)
					   {
							gear[2].flags |= GearData::GearStuck;
							platform->mFaults->SetFault(FaultClass::gear_fault,
								FaultClass::ldgr, FaultClass::fail, FALSE);
					   }

					   if(platform->IsComplex() && platform->GetDOFValue(COMP_RT_GEAR_DR) < platform->GetDOFValue(COMP_RT_GEAR))
					   {
							platform->SetDOF(COMP_RT_GEAR_DR, max(platform->GetDOFValue(COMP_RT_GEAR) + (float)rand()/(float)RAND_MAX * 10.0F*DTR, 0.0F));
					   }
					   break;
				   case 3:
					//nose door
					   newpos = ((float)rand()/(float)RAND_MAX * 50.0F + 40.0F)*DTR;
					   
					   if(dmg > 25.0F + rand()%5)
						   gear[0].flags |= GearData::DoorBroken;
					   else if(dmg > 15.0F + rand()%5)
						   gear[0].flags |= GearData::DoorStuck;

					   if(platform->IsComplex() && newpos > platform->GetDOFValue(COMP_NOS_GEAR))
							platform->SetDOF(COMP_NOS_GEAR_DR, newpos);
					   break;
				   case 4:
					//left door
					   newpos = ((float)rand()/(float)RAND_MAX * 50.0F + 25.0F)*DTR;

					   if(dmg > 25.0F + rand()%5)
						   gear[1].flags |= GearData::DoorBroken;
					   else if(dmg > 15.0F + rand()%5)
						   gear[1].flags |= GearData::DoorStuck;

					   if(platform->IsComplex() && newpos > platform->GetDOFValue(COMP_NOS_GEAR))
							platform->SetDOF(COMP_LT_GEAR_DR, newpos);
					   break;
				   case 5:
					//right door
					   newpos = ((float)rand()/(float)RAND_MAX * 50.0F + 25.0F)*DTR;

					   if(dmg > 25.0F + rand()%5)
						   gear[2].flags |= GearData::DoorBroken;
					   else if(dmg > 15.0F + rand()%5)
						   gear[2].flags |= GearData::DoorStuck;

					   if(platform->IsComplex() && newpos > platform->GetDOFValue(COMP_RT_GEAR))
							platform->SetDOF(COMP_RT_GEAR_DR, newpos);
					   break;
				   default:
					   break;
				   }

				   if(	gear[0].flags & GearData::GearBroken && 
						gear[1].flags & GearData::GearBroken &&
						gear[2].flags & GearData::GearBroken)
				   {
					   ((AircraftClass*)platform)->mFaults->SetFault(FaultClass::gear_fault,
							FaultClass::ldgr, FaultClass::fail, TRUE);
					   SetFlag (GearBroken);
				   }

				   if(	gear[0].flags & GearData::GearStuck && 
						gear[1].flags & GearData::GearStuck &&
						gear[2].flags & GearData::GearStuck)
				   {
					   ((AircraftClass*)platform)->mFaults->SetFault(FaultClass::gear_fault,
							FaultClass::ldgr, FaultClass::fail, TRUE);					   
				   }

				   // gear breaks sound
				   switch(rand()%3)
				   {
				   case 0:
						F4SoundFXSetPos( SFX_GROUND_CRUNCH, TRUE, x, y, z, 1.0f );
						break;
				   case 1:
						F4SoundFXSetPos( SFX_HIT_1, TRUE, x, y, z, 1.0f );
						break;
				   case 2:
						F4SoundFXSetPos( SFX_HIT_5, TRUE, x, y, z, 1.0f );
						break;
				   }
			   }

			   if(numDebris)
			   {
				   Tpoint pos, vec;
   
				   // for now, we'll scatter some debris -- need BSP for wheels?
				   pos.x = x;
				   pos.y = y;
				   pos.z = z + 3.0f;
				   vec.z = zdot;
				   for ( i = 0; i < numDebris; i++ )
				   {
					   vec.x = xdot + PRANDFloat() * 30.0f;
					   vec.y = ydot + PRANDFloat() * 30.0f;
					   OTWDriver.AddSfxRequest(
						   new SfxClass( SFX_DARK_DEBRIS,		// type
						   SFX_MOVES | SFX_USES_GRAVITY | SFX_BOUNCES,
						   &pos,					// world pos
						   &vec,					// vel vector
						   3.0f,					// time to live
						   1.0f ) );				// scale
				   }
			   }
         }
      }
   }

   /*-------------------------*/
   /* compute thrust fraction */
   /*-------------------------*/
   anozl  = 1.0F - athrev;
   ethrst = anozl - athrev * 0.8666F;
}
