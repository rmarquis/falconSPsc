#include "stdhdr.h"
#include "missile.h"
#include "otwdrive.h"
#include "playerop.h"
#include "MsgInc\DamageMsg.h"
#include "camp2sim.h"
#include "object.h"
// Marco addition for missile arming delay
#include "entity.h"
//MI
#include "Classtbl.h"

extern int g_nMissileFix;
extern float g_fLethalRadiusModifier;

//static int miscount = 0;

// 2002-03-28 MN Modified SetStatus function, fixes floating missiles and adds support for "bombwarhead" missiles like JSOW/JDAM
void MissileClass::SetStatus(void)
{
SimBaseClass *hitObj;
float TToLive = 0.0f;
bool bombwarhead = false;
Falcon4EntityClassType* classPtr;
classPtr = (Falcon4EntityClassType*)EntityType();
WeaponClassDataType *wc = NULL;

	ShiAssert(classPtr);

	if (classPtr)
		wc = (WeaponClassDataType*)classPtr->dataPtr; // this is important

	ShiAssert(wc);

	if (wc && (wc->Flags & WEAP_BOMBWARHEAD) && (g_nMissileFix & 0x01))
		bombwarhead = true;

	ShiAssert(engineData);
	ShiAssert(inputData);

   /*--------------------------------------*/
   /* don't check status until off the rail*/  
   /*--------------------------------------*/
   if (launchState == Launching)
   {
      done = FalconMissileEndMessage::NotDone; 
   }
   /*----------------------*/
   /* within lethal radius */
   /*----------------------*/
// 2002-03-28 MN "Bomb" warheads on missiles with a big blast radius shall not 
// detonate when the target is within blast range (like JSOW)....
   else if ( ricept*ricept <= lethalRadiusSqrd && !bombwarhead) 
   {
	  // Marco Edit - check for our missile arming delay
	  if (wc)
	  {
		  TToLive = (float)((((unsigned char *)wc)[45]) & 7) ;  //  + 1) * 2)) ;
		  if (TToLive > 4.0)
			TToLive = TToLive * (float)2.0;
	  }
      /*----------------------------------*/
      /* warhead didn't have time to fuse */
      /*----------------------------------*/
      if (inputData && runTime < inputData->guidanceDelay) // possible CTD fix
      {
         done = FalconMissileEndMessage::MinTime;
/*		  miscount ++;
		  FILE *fp;
		  fp = fopen("C:\\MissileEnd.txt","at");
		  fprintf(fp,"guidanceDelayMinTime #%d\n",miscount);
		  fclose(fp);*/

      }
	  // Marco Edit - Check for Time to Warhead Armed
	  else if (runTime < TToLive)
	  {
		  if (g_nMissileFix & 0x04)
			done = FalconMissileEndMessage::ArmingDelay;
		  else
			done = FalconMissileEndMessage::MinTime;
/* 		  miscount ++;
		  FILE *fp;
		  fp = fopen("C:\\MissileEnd.txt","at");
		  fprintf(fp,"ArmingDelay #%d\n",miscount);
		  fclose(fp);*/
	  }
      /*------*/
      /* kill */
      /*------*/
      else
      {
/*		  miscount ++;
		  FILE *fp;
		  fp = fopen("C:\\MissileEnd.txt","at");
		  fprintf(fp,"riceptMissileKill #%d\n",miscount);
		  fclose(fp);*/
         done = FalconMissileEndMessage::MissileKill;
      }
   }
// 2002-04-06 MN if the missile sensor has lost track, we have no targetptr, and our range to interpolated
// target position is inside the lethal radius OR missile is higher than its maxalt,
// bring missile to an end. When we have missiles going high ballistic, intercept them at max altitude
// in case of lethalRadius, they might apply a bit of proximity damage to the target...
	else if (runTime > 1.50f && (flags & SensorLostLock) && !targetPtr && ((g_nMissileFix & 0x20) && 
		range * range < lethalRadiusSqrd || wc && wc->MaxAlt && (fabs(z) > fabs(wc->MaxAlt*1000))))
	{
/*		  miscount ++;
		  FILE *fp;
		  fp = fopen("C:\\MissileEnd.txt","at");
		  fprintf(fp,"missed Sensor Lock lost, g_n & 0x20 #%d\n",miscount);
		  fclose(fp);*/

// 2002-04-14 MN Try a modification here. This code could result in missing mavericks and Aim-9 and others.
// So when our last target lock was really close to lethalRadius - let's still do a MissileKill...
		if (range*range < lethalRadiusSqrd && ricept*ricept < lethalRadiusSqrd*g_fLethalRadiusModifier)
			done = FalconMissileEndMessage::MissileKill;
		else 
			done = FalconMissileEndMessage::Missed;
	}
   /*-------------------*/
   /* Missed the target */
   /*-------------------*/
// 2002-03-28 MN added range^2 check, as ricept is not updated anymore if we lost our target.
// Fixes floating missiles on the ground
   else if ((flags & ClosestApprch) && (ricept*ricept > lethalRadiusSqrd || ((g_nMissileFix & 0x02) && range*range > lethalRadiusSqrd)))
   {
      done = FalconMissileEndMessage::Missed;
/*	  miscount ++;
	  FILE *fp;
	  fp = fopen("C:\\MissileEnd.txt","at");
	  if (ricept*ricept>lethalRadiusSqrd && range*range > lethalRadiusSqrd)
		fprintf(fp,"MissedClosestApproach ricept & range #%d\n",miscount);
	  else if (ricept*ricept>lethalRadiusSqrd)
		  fprintf(fp,"MissedClosestApproach ricept #%d\n",miscount);
	  else fprintf(fp,"MissedClosestApproach range #%d\n",miscount);
	  fclose(fp);
*/
   }
   /*---------------*/
   /* ground impact */
   /*---------------*/
   else if ( z > groundZ && !bombwarhead) // bombwarhead missiles are handled below
   {
   	  // edg: this is somewhat of a hack, but I haven't had much luck fixing it
      // otherwise... if the parent is a ground unit and the missile has just
   	  // been launched( runTime ), don't do a groundimpact check yet
	  if ( (parent && parent->OnGround() && runTime < 1.50f) )
	  {
	  }
	  else
      	done = FalconMissileEndMessage::GroundImpact;
/*  		  miscount ++;
		  FILE *fp;
		  fp = fopen("C:\\MissileEnd.txt","at");
		  fprintf(fp,"GroundImpact #%d\n",miscount);
		  fclose(fp);*/

   }
   /*----------------*/
   /* feature impact */
   /*----------------*/
// 2002-03-28 MN handle bombwarhead missiles here
#ifndef MISSILE_TEST_PROG
   else if ( z - groundZ > -800.0f)	// only do feature impact if we are not a bomb
   {
		if (runTime > 1.50f) // JB 000819 //+ SAMS sitting on runways will hit the runway
		{ // JB 000819 //+
			hitObj = FeatureCollision( groundZ );
			if (hitObj)
			{
				if (bombwarhead)
				{
					// bombwarhead missile - we've not hit our target on our way:
					if (targetPtr && hitObj != targetPtr->BaseData())	// CTD fix
					{
						done = FalconMissileEndMessage::FeatureImpact;
						// might as well send the msg now....
						// apply proximity damage may send it more damage.  However this is
						// only from the base!
						SendDamageMessage(hitObj,0,FalconDamageType::MissileDamage);
					}
					else // bombwarhead missile has hit assigned target or hit something else
					{
						if (targetPtr && ricept*ricept < lethalRadiusSqrd)
						  done = FalconMissileEndMessage::BombImpact;
						else
					      done = FalconMissileEndMessage::GroundImpact;
					}
				}
						
				else // we're not a bombwarhead missile, do normal feature impact
				{
					/*		  miscount ++;
							  FILE *fp;
							  fp = fopen("C:\\MissileEnd.txt","at");
							  fprintf(fp,"Feature impact #%d\n",miscount);
								fclose(fp);*/

					done = FalconMissileEndMessage::FeatureImpact;
					// might as well send the msg now....
					// apply proximity damage may send it more damage.  However this is
					// only from the base!
					SendDamageMessage(hitObj,0,FalconDamageType::MissileDamage);
				}
			}
			if (z > groundZ)	// now we've hit the dust and not a feature (needed in case of g_bMissileFix == true...)
			{
				done = FalconMissileEndMessage::GroundImpact;
/*						  miscount ++;
		  FILE *fp;
		  fp = fopen("C:\\MissileEnd.txt","at");
		  fprintf(fp,"Ground impact #%d\n",miscount);
		  fclose(fp);*/
			}


		} // JB 000819 //+
   }
#endif
// 2002-03-28 MN changed "else if" to plain "if" for the next two conditions. 
// Min velocity and max life time can happen in *each* situation, so always check for them
// If we should still have floating missiles on the ground, this will bring them to an end some time for sure...
   /*--------------------------------*/
   /* min velocity only after burnout*/
   /*--------------------------------*/
   if (inputData && engineData && mach < inputData->mslVmin &&	// 2002-04-05 MN CTD fix
         runTime > engineData->times[engineData->numBreaks-1])
   {
      done = FalconMissileEndMessage::MinSpeed;
	  	
/*		  miscount ++;
		  FILE *fp;
		  fp = fopen("C:\\MissileEnd.txt","at");
		  fprintf(fp,"MinSpeed #%d\n",miscount);
		  fclose(fp);*/

   }
   /*----------*/
   /* max time */
   /*----------*/
   if (inputData && runTime > inputData->maxTof) // 2002-04-05 MN possible CTD fix
   {
/*	   	  miscount ++;
		  FILE *fp;
		  fp = fopen("C:\\MissileEnd.txt","at");
		  fprintf(fp,"ExceedTime #%d\n",miscount);
		  fclose(fp);*/

      done = FalconMissileEndMessage::ExceedTime;
   }
}


#if 0
// old function
void MissileClass::SetStatus(void)
{
SimBaseClass *hitObj;

float TToLive ;

   /*--------------------------------------*/
   /* don't check status until off the rail*/  
   /*--------------------------------------*/
   if (launchState == Launching)
   {
      done = FalconMissileEndMessage::NotDone; 
   }
   /*----------------------*/
   /* within lethal radius */
   /*----------------------*/
   else if ( ricept*ricept < lethalRadiusSqrd )
   {
	  Falcon4EntityClassType* classPtr;
	  WeaponClassDataType* wc;
	  classPtr = (Falcon4EntityClassType*)EntityType();
	  wc = (WeaponClassDataType*)classPtr->dataPtr; // this is important

	  // Marco Edit - check for our missile arming delay
	  TToLive = (float)((((unsigned char *)wc)[45]) & 7) ;  //  + 1) * 2)) ;
	  if (TToLive > 4.0)
			TToLive = TToLive * (float)2.0;

      /*----------------------------------*/
      /* warhead didn't have time to fuse */
      /*----------------------------------*/
      if (runTime < inputData->guidanceDelay)
      {
         done = FalconMissileEndMessage::MinTime;
      }
	  // Marco Edit - Check for Time to Warhead Armed
	  else if (runTime < TToLive)
	  {
		 done = FalconMissileEndMessage::MinTime;
	  }
      /*------*/
      /* kill */
      /*------*/
      else
      {
         done = FalconMissileEndMessage::MissileKill;
      }
   }
   /*-------------------*/
   /* Missed the target */
   /*-------------------*/
   else if ((flags & ClosestApprch) && ricept*ricept > lethalRadiusSqrd)
   {
      done = FalconMissileEndMessage::Missed;
   }
   /*---------------*/
   /* ground impact */
   /*---------------*/
   else if ( z > groundZ )
   {
   	  // edg: this is somewhat of a hack, but I haven't had much luck fixing it
      // otherwise... if the parent is a ground unit and the missile has just
   	  // been launched( runTime ), don't do a groundimpact check yet
	  if ( (parent->OnGround() && runTime < 1.50f) )
	  {
	  }
	  else
      	done = FalconMissileEndMessage::GroundImpact;
   }
   /*---------------*/
   /* feature impact */
   /*---------------*/
#ifndef MISSILE_TEST_PROG
   else if ( z - groundZ > -800.0f )
   {
		if (runTime > 1.50f) // JB 000819 //+ SAMS sitting on runways will hit the runway
		{ // JB 000819 //+
			hitObj = FeatureCollision( groundZ );
			if (hitObj)
			{
				done = FalconMissileEndMessage::FeatureImpact;
				// might as well send the msg now....
				// apply proximity damage may send it more damage.  However this is
				// only from the base!
				SendDamageMessage(hitObj,0,FalconDamageType::MissileDamage);
			}
		} // JB 000819 //+
   }
#endif
   /*--------------------------------*/
   /* min velocity only after burnout*/
   /*--------------------------------*/
   else if (mach < inputData->mslVmin &&
         runTime > engineData->times[engineData->numBreaks-1])
   {
      done = FalconMissileEndMessage::MinSpeed;
   }
   /*----------*/
   /* max time */
   /*----------*/
   else if (runTime > inputData->maxTof)
   {
      done = FalconMissileEndMessage::ExceedTime;
   }
}
#endif