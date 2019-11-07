#include "Graphics\Include\drawbsp.h"
#include "stdhdr.h"
#include "ClassTbl.h"
#include "Entity.h"
#include "helo.h"
#include "helimm.h"
#include "hdigi.h"
#include "campwp.h"
#include "initdata.h"
#include "fcc.h"
#include "sms.h"
#include "simdrive.h"
#include "otwdrive.h"
#include "hardpnt.h"
#include "PilotInputs.h"
#include "object.h"
#include "simobj.h"
#include "MsgInc\WingmanMsg.h"
#include "fsound.h"
#include "soundfx.h"
#include "Unit.h"
#include "MsgInc\LandingMessage.h"
#include "MsgInc\DamageMsg.h"
#include "radarDoppler.h"
#include "acmi\src\include\acmirec.h"
#include "fakerand.h"
#include "falcsess.h"
#include "guns.h"
#include "Graphics\Include\rviewpnt.h"
#include "team.h"
#include "dofsnswitches.h"

#ifdef USE_SH_POOLS
MEM_POOL	HelicopterClass::pool;
#endif

void SetLabel (SimBaseClass* theObject);

// these are offsets from the lead heli's tail for formation
// flying
#define	DSPREAD(n)		(150.0f * n)
Tpoint gFormationOffsets[] =
{
//	   X		 		Y			 			Z
	{ 0.0f,				0.0f,					0.0f },		/*  0 */
	{ DSPREAD(-1.0f),	DSPREAD(-1.0f),			0.0f },		/*  1 */
	{ DSPREAD( 1.0f),	DSPREAD(-1.0f),			0.0f },		/*  2 */
	{ DSPREAD(-2.0f),	DSPREAD(-2.0f),			0.0f },		/*  3 */
	{ DSPREAD(-1.0f),	DSPREAD(-2.0f),			0.0f },		/*  4 */
	{ DSPREAD( 1.0f),	DSPREAD(-2.0f),			0.0f },		/*  5 */
	{ DSPREAD( 2.0f),	DSPREAD(-2.0f),			0.0f },		/*  6 */
	{ DSPREAD(-3.0f),	DSPREAD(-3.0f),			0.0f },		/*  7 */
	{ DSPREAD(-2.0f),	DSPREAD(-3.0f),			0.0f },		/*  8 */
	{ DSPREAD(-1.0f),	DSPREAD(-3.0f),			0.0f },		/*  9 */
	{ DSPREAD( 1.0f),	DSPREAD(-3.0f),			0.0f },		/* 10 */
	{ DSPREAD( 2.0f),	DSPREAD(-3.0f),			0.0f },		/* 11 */
	{ DSPREAD( 3.0f),	DSPREAD(-3.0f),			0.0f },		/* 12 */
	{ DSPREAD(-4.0f),	DSPREAD(-4.0f),			0.0f },		/* 13 */
	{ DSPREAD(-3.0f),	DSPREAD(-4.0f),			0.0f },		/* 14 */
	{ DSPREAD(-2.0f),	DSPREAD(-4.0f),			0.0f },		/* 15 */
	{ DSPREAD(-1.0f),	DSPREAD(-4.0f),			0.0f },		/* 16 */
	{ DSPREAD( 1.0f),	DSPREAD(-4.0f),			0.0f },		/* 17 */
	{ DSPREAD( 2.0f),	DSPREAD(-4.0f),			0.0f },		/* 18 */
	{ DSPREAD( 3.0f),	DSPREAD(-4.0f),			0.0f },		/* 19 */
	{ DSPREAD( 4.0f),	DSPREAD(-4.0f),			0.0f },		/* 20 */
	{ DSPREAD(-5.0f),	DSPREAD(-5.0f),			0.0f },		/* 21 */
	{ DSPREAD(-4.0f),	DSPREAD(-5.0f),			0.0f },		/* 22 */
	{ DSPREAD(-3.0f),	DSPREAD(-5.0f),			0.0f },		/* 23 */
	{ DSPREAD(-2.0f),	DSPREAD(-5.0f),			0.0f },		/* 24 */
	{ DSPREAD(-1.0f),	DSPREAD(-5.0f),			0.0f },		/* 25 */
	{ DSPREAD( 1.0f),	DSPREAD(-5.0f),			0.0f },		/* 26 */
	{ DSPREAD( 2.0f),	DSPREAD(-5.0f),			0.0f },		/* 27 */
	{ DSPREAD( 3.0f),	DSPREAD(-5.0f),			0.0f },		/* 28 */
	{ DSPREAD( 4.0f),	DSPREAD(-5.0f),			0.0f },		/* 29 */
	{ DSPREAD( 5.0f),	DSPREAD(-5.0f),			0.0f },		/* 30 */
	{ DSPREAD(-6.0f),	DSPREAD(-6.0f),			0.0f },		/* 31 */
	{ DSPREAD(-5.0f),	DSPREAD(-6.0f),			0.0f },		/* 32 */
	{ DSPREAD(-4.0f),	DSPREAD(-6.0f),			0.0f },		/* 33 */
	{ DSPREAD(-3.0f),	DSPREAD(-6.0f),			0.0f },		/* 34 */
	{ DSPREAD(-2.0f),	DSPREAD(-6.0f),			0.0f },		/* 35 */
	{ DSPREAD(-1.0f),	DSPREAD(-6.0f),			0.0f },		/* 36 */
	{ DSPREAD( 1.0f),	DSPREAD(-6.0f),			0.0f },		/* 37 */
	{ DSPREAD( 2.0f),	DSPREAD(-6.0f),			0.0f },		/* 38 */
	{ DSPREAD( 3.0f),	DSPREAD(-6.0f),			0.0f },		/* 39 */
	{ DSPREAD( 4.0f),	DSPREAD(-6.0f),			0.0f },		/* 40 */
	{ DSPREAD( 5.0f),	DSPREAD(-6.0f),			0.0f },		/* 41 */
	{ DSPREAD( 6.0f),	DSPREAD(-6.0f),			0.0f },		/* 42 */
	{ DSPREAD(-3.0f),	DSPREAD(-7.0f),			0.0f },		/* 43 */
	{ DSPREAD(-2.0f),	DSPREAD(-7.0f),			0.0f },		/* 44 */
	{ DSPREAD(-1.0f),	DSPREAD(-7.0f),			0.0f },		/* 45 */
	{ DSPREAD( 1.0f),	DSPREAD(-7.0f),			0.0f },		/* 46 */
	{ DSPREAD( 2.0f),	DSPREAD(-7.0f),			0.0f },		/* 47 */
	{ DSPREAD( 3.0f),	DSPREAD(-7.0f),			0.0f },		/* 48 */
};

extern int testFlag;

void CalcTransformMatrix (SimBaseClass* theObject);

HelicopterClass::HelicopterClass (VU_BYTE** stream) : SimVehicleClass (stream)
{
   InitData();
}

HelicopterClass::HelicopterClass (FILE* filePtr) : SimVehicleClass (filePtr)
{
   InitData();
}

HelicopterClass::HelicopterClass (int type) : SimVehicleClass (type)
{
   InitData();
}

void HelicopterClass::InitData ()
{
   ACMISwitchRecord acmiSwitch;

   theBrain = NULL;
   hBrain = NULL;
   Guns = NULL;

   /*----------------*/
   /* Init targeting */
   /*----------------*/

   numGuns = 0;

   fireGun = FALSE;
   fireMissile = FALSE;
   lastPickle = FALSE;
   curWaypoint = 0;

   hf = NULL;
   waypoint = NULL;

	// switch 0 controls blade type -- fast or slow moving
	// dof 2 is main rotor, 4 is tail rotor
	SetSwitch( HELI_ROTORS, 1);
	SetDOF( HELI_MAIN_ROTOR, 0.0f );
	SetDOF( HELI_TAIL_ROTOR, 0.0f );
	if ( gACMIRec.IsRecording() )
	{
		acmiSwitch.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
		acmiSwitch.data.type = Type();
		acmiSwitch.data.uniqueID = ACMIIDTable->Add(Id(),NULL,0);//.num_;
		acmiSwitch.data.switchNum = HELI_ROTORS;
		acmiSwitch.data.switchVal = 1;
		acmiSwitch.data.prevSwitchVal = 1;
		gACMIRec.SwitchRecord( &acmiSwitch );
	}
}

HelicopterClass::~HelicopterClass(void)
{
	delete (hf);
	if (Sms)
		delete Sms;
	if (FCC)
		delete FCC;
	if ( hBrain )
		delete hBrain;
}

void HelicopterClass::Init(SimInitDataClass* initData)
{
	int i;
	WayPointClass* atWaypoint;
	float wp1X, wp1Y, wp1Z;
	float wp2X, wp2Y, wp2Z;
	
	SimVehicleClass::Init (initData);
	
	hf = new HeliMMClass( this, SIMPLE );
	
	hf->isDigital   = TRUE;
	waypoint        = initData->waypointList;
	numWaypoints    = initData->numWaypoints;
	curWaypoint     = waypoint;
	atWaypoint      = waypoint;
	
	for (i=0; i<numWaypoints; i++)
		{
		curWaypoint->GetLocation (&wp1X, &wp1Y, &wp1Z);
		// sanity check helo waypoints....
		
		// below ground?
		if (wp1Z >= (OTWDriver.GetGroundLevel(wp1X, wp1Y)))
			{
			curWaypoint->SetLocation (wp1X, wp1Y, OTWDriver.GetGroundLevel(wp1X, wp1Y) - 5.0f );
			}
		// limit max alt for helo's to around 8000 ft AGL
		else if (wp1Z < (OTWDriver.GetGroundLevel(wp1X, wp1Y) - 8000.0f))
			{
			curWaypoint->SetLocation (wp1X, wp1Y, OTWDriver.GetGroundLevel(wp1X, wp1Y) - 8000.0f );
			}
			/**
			else
			curWaypoint->SetLocation (wp1X, wp1Y, OTWDriver.GetGroundLevel(wp1X, wp1Y) - 1000.0f );
		*/
		
		curWaypoint = curWaypoint->GetNextWP();
		}
	curWaypoint     = waypoint;
	
	for (i=0; i<initData->currentWaypoint; i++)
		{
		atWaypoint = curWaypoint;
		curWaypoint = curWaypoint->GetNextWP();
		}
	
	// Calculate current heading
	if (curWaypoint)
		{
		
		if (curWaypoint == atWaypoint)
			curWaypoint = curWaypoint->GetNextWP();
		
		atWaypoint->GetLocation (&wp1X, &wp1Y, &wp1Z);
		
		if (curWaypoint == NULL)
			{
			wp1X = initData->x;
			wp1Y = initData->y;
			curWaypoint = atWaypoint;
			}
		
		curWaypoint->GetLocation (&wp2X, &wp2Y, &wp2Z);
		}
	
	// edg: note helicopters havve a capped AGL
	if (initData->z < (OTWDriver.GetGroundLevel(initData->x, initData->y) - 8000.0f))
		initData->z = OTWDriver.GetGroundLevel(initData->x, initData->y) - 8000.0f;
	
	hf->Init( initData->x, initData->y, initData->z );
	hf->SetControls( 0.0f, 0.0f, 0.0f, 0.0f );
	hf->Exec();
	
	// Weapons Stuff
	Sms = new SMSClass (this, initData->weapon, initData->weapons);
	FCC = new FireControlComputer (this, Sms->NumHardpoints());
	FCC->Sms = Sms;
	FCC->SetMasterMode (FireControlComputer::Nav);
	numGuns = 0;
	
	if (Sms->NumHardpoints() > 0)
		{
		Guns = Sms->hardPoint[0]->GetGun();
		if ( Guns )
			Guns->SetPosition( 4.0f, 0.0f, 0.0f, 0.0f, 0.0f );
		}
	
	// allow helis unlimited for now
	// Sms->SetUnlimitedAmmo (TRUE);
		
	/*--------------------*/
	/* Update shared data */
	/*--------------------*/
	SetPosition (hf->XE.x, hf->XE.y, hf->XE.z);
	SetDelta (hf->VE.x, hf->VE.y, hf->VE.z);
	SetYPR (hf->XE.az, hf->XE.ay, hf->XE.ax);
	SetYPRDelta (hf->VE.az, hf->VE.ay, hf->VE.ax);
	SetVt(hf->vta);
	SetKias(hf->kias);
	
	theInputs   = new PilotInputs;
	
	// If I only had a brain
	hBrain    = new HeliBrain (this);
	
	// FCC->SetMasterMode(FireControlComputer::Missile);
	// FCC->SetSubMode(FireControlComputer::Aim9);
	
	CalcTransformMatrix (this);
	
	// timers for targeting
	nextGeomCalc = SimLibElapsedTime;
	nextTargetUpdate = SimLibElapsedTime;
	
	distLOD = 1.0f;
	useDistLOD = FALSE;
	flightIndex = 0;
	
	geomCalcRate = (int)(1.0f + 4.0f * PRANDFloatPos()) * SEC_TO_MSEC;
	targetUpdateRate = (int)(5.0f + 15.0f * PRANDFloatPos()) * SEC_TO_MSEC;
}

int HelicopterClass::Wake(void)
{
int retval = 0;

	if (IsAwake())
		return retval;

	SimVehicleClass::Wake();

	InitDamageStation();

	if (Sms)
		Sms->AddWeaponGraphics();

	// every 5th heli in flight will LOD out
	if ( useDistLOD == TRUE )
	{
		drawPointer->SetLabel ("", 0xff00ff00);
	}

	if ( IsLocal() )
	{
		GetFormationPos( &hf->XE.x, &hf->XE.y, &hf->XE.z );
		SetPosition( hf->XE.x, hf->XE.y, hf->XE.z );
	}
	return retval;
}

int HelicopterClass::Sleep(void)
{
int retval = 0;

	if (!IsAwake())
		return retval;

	if ( hBrain )
		hBrain->Sleep();

	// Put away any weapon graphics
	if (Sms)
		{
		Sms->FreeWeaponGraphics();
		Sms->FindWeaponClass(wcNoWpn);
		}
	if (FCC)
		FCC->ClearCurrentTarget();

	SimVehicleClass::Sleep();

	CleanupDamageStation();
	return retval;
}

int HelicopterClass::Exec(void)
{
ACMIGenPositionRecord genPos;
ACMISwitchRecord acmiSwitch;
//ACMIDOFRecord DOFRec;
Tpoint newpos, vec;
float curDOF, deltaDOF;

	if ( IsDead() )
		return TRUE;

   SimVehicleClass::Exec();


   if(IsExploding())
   {
	  if ( !IsSetFlag( SHOW_EXPLOSION ) )
	  {
      		RunExplosion();
			SetFlag( SHOW_EXPLOSION );
			if ( IsLocal() && flightLead == this )
			{
				PromoteSubordinates();
			}
			SetDead(TRUE );
	  }
	  return TRUE;

   }
   else if (!IsDead())
   {
   	  ShowDamage();

	  // animate rotors
	  if ( GetSwitch(HELI_ROTORS) != 1 )
	  {
			SetSwitch( HELI_ROTORS, 1);
			SetDOF( HELI_MAIN_ROTOR, 0.0f );
			SetDOF( HELI_TAIL_ROTOR, 0.0f );
			if ( gACMIRec.IsRecording() )
			{
				acmiSwitch.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
				acmiSwitch.data.type = Type();
				acmiSwitch.data.uniqueID = ACMIIDTable->Add(Id(),NULL,0);//.num_;
				acmiSwitch.data.switchNum = HELI_ROTORS;
				acmiSwitch.data.switchVal = 1;
				acmiSwitch.data.prevSwitchVal = 1;
				gACMIRec.SwitchRecord( &acmiSwitch );
			}
      }
	  curDOF = GetDOFValue( HELI_MAIN_ROTOR );
	  // blades rotate at around 300 RPM which is 1800 DPS
	  deltaDOF = 1800.0f * DTR * SimLibMajorFrameTime;
	  curDOF += deltaDOF;
	  if ( curDOF > 360.0f * DTR )
	  	curDOF -= 360.0f * DTR;

	  SetDOF( HELI_MAIN_ROTOR, curDOF );
	  SetDOF( HELI_TAIL_ROTOR, curDOF );


	    // JPO - for engine noise
	  VehicleClassDataType *vc = GetVehicleClassData(Type() - VU_LAST_ENTITY_TYPE);
	  ShiAssert(FALSE == F4IsBadReadPtr(vc, sizeof *vc));
	  float dop = OTWDriver.GetDoppler( XPos(), YPos(), ZPos(), XDelta(), YDelta(), ZDelta() );
	  if (vc)
	      F4SoundFXSetPos(vc->EngineSound, 0, XPos(), YPos(), ZPos(), dop );
	  else
	      F4SoundFXSetPos( SFX_ENGHELI, 0, XPos(), YPos(), ZPos(), dop );

	  // ACMI Output
	  if (gACMIRec.IsRecording() && (SimLibFrameCount & 0x0f ) == 0)
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
// remove		  genPos.data.teamColor = TeamInfo[GetTeam()]->GetColor();
		  gACMIRec.GenPositionRecord( &genPos );
	  }


      if (!IsLocal())
      {
         return FALSE;
      }

	  // if we're the flight leader set our dist LOD
	  if ( flightLead == this )
	  {
		  SetDistLOD();
	  }
	  else if (flightLead)
	  {
		  distLOD = flightLead->distLOD;
	  }
		else
			distLOD = 0;

	  // does this helicopter LOD out?
	  if ( useDistLOD == TRUE && flightLead)
	  {
		  if ( !OnGround() && distLOD < 0.5f && !IsFiring() )
		  {
			  // should be hidden
			  SetLocalFlag( IS_HIDDEN );
			  GetFormationPos( &newpos.x, &newpos.y, &newpos.z );
			  vec.x = newpos.x - XPos();
			  vec.y = newpos.y - YPos();
			  vec.z = newpos.z - ZPos();
			  vec.x *= 0.1f;
			  vec.y *= 0.1f;
			  vec.z *= 0.1f;
			  SetDelta(
					vec.x,
					vec.y,
					vec.z );
			  SetPosition(
					XPos() + XDelta(),
					YPos() + YDelta(),
					ZPos() + ZDelta() );
			  SetYPR(
					flightLead->Yaw(),
					flightLead->Pitch(),
					flightLead->Roll() );
			  SetYPRDelta(
					flightLead->YawDelta(),
					flightLead->PitchDelta(),
					flightLead->RollDelta() );
			  return TRUE;

		  }
		  else
		  {
			  // shouldn't be hidden
			  if ( IsSetLocalFlag( IS_HIDDEN ) )
			  {
			  	  UnSetLocalFlag( IS_HIDDEN );
			  	  GetFormationPos( &newpos.x, &newpos.y, &newpos.z );
				  hf->XE.x = newpos.x;
				  hf->XE.y = newpos.y;
				  hf->XE.z = newpos.z;
				  hf->VE.x =flightLead->XDelta();
				  hf->VE.y =flightLead->YDelta();
				  hf->VE.z =flightLead->ZDelta();
				  hf->XE.ax =flightLead->Roll();
				  hf->XE.ay =flightLead->Pitch();
				  hf->XE.az =flightLead->Yaw();
			  }
		  }
	  } // use Dist LOD

	  // if ( flightLead == this && SimLibElapsedTime > nextTargetUpdate )
	  if ( SimLibElapsedTime > nextTargetUpdate )
	  {
			hBrain->TargetSelection();
			SetTarget( hBrain->targetPtr );
			// targetList = UpdateTargetList (targetList, this, SimDriver.combinedList);
			nextTargetUpdate = SimLibElapsedTime + targetUpdateRate;
	  }

      /*----------------------*/
      /* Do Relative geometry */
      /*----------------------*/
	  // if ( flightLead == this && SimLibElapsedTime > nextGeomCalc )
	  if ( SimLibElapsedTime > nextGeomCalc )
	  {
      		CalcRelGeom(this, targetPtr, vmat, 1.0F / SimLibMajorFrameTime);

   			// Sensors
      		RunSensors();

			/*
			if ( !hBrain->isWing )
			{
				hBrain->TargetSelection( targetList );
			}
			*/

			nextGeomCalc = SimLibElapsedTime + geomCalcRate;
	  }


      // Get the controls
      GatherInputs();

      // Weapons & targeting - Note: change target to the brain's selected target
      // FCC->Exec(flightLead->hBrain->targetPtr, flightLead->hBrain->targetPtr, theInputs);

	  // fire weapons if chosen
	  DoWeapons();

      /*------------------------*/        
      /* Fly the airframe.      */
      /*------------------------*/
      hf->SetControls( hBrain->pStick, hBrain->rStick, hBrain->throtl, hBrain->yPedal );
      if (!OnGround() && SimDriver.MotionOn())
      {
         hf->Exec();
      }

      Tpoint normal;
      float groundZ;
      groundZ = OTWDriver.GetGroundLevel(hf->XE.x, hf->XE.y, &normal);
      if (!OnGround() && hf->XE.z >= groundZ )
      {
         float tmp;
            
         // Normalize terrain normal
         tmp = (float)sqrt(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
         normal.z /= tmp;

		  // insure we're above ground!
		  hf->XE.z = OTWDriver.GetGroundLevel(hf->XE.x, hf->XE.y) - 5.0F;

          // if the z normal is beyond a certain value, ground is too sloped
		  // and we explode
          if (normal.z < -0.7f )
              LandingCheck();
          else
		  {
			  // apply damage
			  /*
			  ** edg NO DAMAGE
			  FalconDamageMessage* message;
	
			  VuTargetEntity *owner_session = (VuTargetEntity*)vuDatabase->Find(OwnerId());
			  message = new FalconDamageMessage (Id(), owner_session);
			  message->dataBlock.gameTime   = SimLibElapsedTime;
			  message->dataBlock.fEntityID  = Id();
			  if (!SimDriver.RunningCampaign() || GetCampaignObject() == NULL || IsDogfight())
			  {
				 message->dataBlock.fCampID = GetCallsignIdx();
				 message->dataBlock.fFlightID = (int)GetCampaignObject();
				 message->dataBlock.fSide   = GetCountry();
			  }
			  else
			  {
				 message->dataBlock.fCampID = ((CampBaseClass*)(GetCampaignObject()))->GetCampID();
				 message->dataBlock.fSide   = ((CampBaseClass*)(GetCampaignObject()))->GetOwner();
			  }
			  message->dataBlock.fPilotID   = pilotSlot;
			  message->dataBlock.fIndex     = Type();
			  message->dataBlock.fWeaponID  = Type();
			  message->dataBlock.fWeaponUID.num_ = 0;
	
			  message->dataBlock.dEntityID  = Id();
			  if (!SimDriver.RunningCampaign() || GetCampaignObject() == NULL || IsDogfight())
			  {
				 message->dataBlock.dCampID = GetCallsignIdx();
				 message->dataBlock.dFlightID = (int)GetCampaignObject();
				 message->dataBlock.dSide   = GetCountry();
			  }
			  else
			  {
				 message->dataBlock.dCampID = ((CampBaseClass*)(GetCampaignObject()))->GetCampID();
				 message->dataBlock.dSide   = ((CampBaseClass*)(GetCampaignObject()))->GetOwner();
			  }
			  message->dataBlock.dPilotID   = pilotSlot;
			  message->dataBlock.dIndex     = Type();
			  message->dataBlock.damageType = FalconDamageMessage::GroundCollisionDamage;
	
			  // for now use maxStrength as amount of damage.
			  // later we'll want to add other factors into the equation --
			  // on ground, speed, etc....
			  message->dataBlock.damageStrength = maxStrength * 0.50f;
			  message->dataBlock.damageRandomFact = PRANDFloat();
			  FalconSendMessage (message,FALSE);
			  */
	
			  // "bounce" the aircraft
			  // hf->XE.z = OTWDriver.GetGroundLevel(hf->XE.x, hf->XE.y) - 5.0F;
			  hf->VE.z = -30.0F;
		  }
      }


	   /*--------------------*/
	   /* Update shared data */
	   /*--------------------*/
      SetPosition (hf->XE.x, hf->XE.y, hf->XE.z);
      SetDelta (hf->VE.x, hf->VE.y, hf->VE.z);
      SetYPR (hf->XE.az, hf->XE.ay, hf->XE.ax);
      SetYPRDelta (hf->VE.az, hf->VE.ay, hf->VE.ax);
      SetVt(hf->vta);
      SetKias(hf->kias);
      SetPowerOutput (hBrain->throtl);

   }

   if ((GetCampaignObject() > (VuEntity*)MAX_IA_CAMP_UNIT) && !hBrain->isWing)
	{
      ((Unit)GetCampaignObject())->SimSetLocation (hf->XE.x, hf->XE.y, hf->XE.z);
	  // KCK note: no reason to do these..
//      GetCampaignObject()->SetDelta (hf->VE.x, hf->VE.y, hf->VE.z);
//      GetCampaignObject()->SetYPR (hf->XE.az, hf->XE.ay, hf->XE.ax);
//      GetCampaignObject()->SetYPRDelta (hf->VE.az, hf->VE.ay, hf->VE.ax);
	}
   return TRUE;
}

void HelicopterClass::RunSensors(void)
{
SimObjectType* tmpTarget;
int i;

   for (i=0; i<numSensors; i++)
   {
      tmpTarget = sensorArray[i]->Exec(targetPtr);

	  // This is really quite wrong.  Should take any sensors target.
	  // If they conflict, then rank them...  SCR 10/14/98
	  /*
      if (sensorArray[i]->Type() == SensorClass::Radar)
      {
         SetTarget(tmpTarget);
      }
	  */
   }
}

#ifdef _DEBUG
void HelicopterClass::SetDead (int flag)
{
	if (flag)
	{
//		MonoPrint ("Helo %d dead at %8ld\n", Id().num_, SimLibElapsedTime);
	}
	SimVehicleClass::SetDead(flag);
}
#endif // _DEBUG

void HelicopterClass::JoinFlight (void)
{
   if (hBrain)
      hBrain->JoinFlight ();

	// every 5th heli in flight will LOD out on heli battalions
	if ( GetCampaignObject()->NumberOfComponents() > 4 && (flightIndex % 5 ) != 0 )
	{
		useDistLOD = TRUE;
	}
}


void HelicopterClass::SetLead (int flag)
{
   if (hBrain)
      hBrain->SetLead (flag);
}

void HelicopterClass::ReceiveOrders (FalconEvent* theEvent)
{
   if (hBrain)
      hBrain->ReceiveOrders(theEvent);
}

void HelicopterClass::GetTransform (TransformMatrix tMat)
{
   memcpy (tMat, dmx, sizeof (TransformMatrix));
}

float HelicopterClass:: GetP (void)
{
   return (RollDelta());
}

float HelicopterClass:: GetQ (void)
{
   return (PitchDelta());
}

float HelicopterClass:: GetR (void)
{
   return (YawDelta());
}

float HelicopterClass:: GetGamma (void)
{
   return (hf->gmma);
}

float HelicopterClass:: GetSigma (void)
{
   return (hf->sigma);
}

float HelicopterClass:: GetMu (void)
{
   return (hf->mu);
}

void HelicopterClass::LandingCheck(void)
{
   // Check for landing
   if (ZDelta() < 20.0F )
   {
      // hf->XE.z = OTWDriver.GetGroundLevel(hf->XE.x, hf->XE.y) - 5.0F;
      hf->VE.z = 0.0F;
      hf->VE.x = 0.0F;
      hf->VE.y = 0.0F;
      hf->VE.az = 0.0F;
      hf->VE.ax = 0.0F;
      hf->VE.ay = 0.0F;
      
      if (SimDriver.RunningInstantAction())
      {
         SetRemoveFlag();
      }

		// Send a landing message
		// KCK NOTE: I'm only sending this for members with the package flag set.
		// This means all package elements in single player, but non-necessarily in 
		// multi-player. But in multi-player we'll at least get all players.
		if (GetCampaignObject() && GetCampaignObject()->InPackage())
			{
			FalconLandingMessage* landingMessage;
			landingMessage = new FalconLandingMessage (Id(), FalconLocalGame);
			ShiAssert(GetCampaignObject());
			landingMessage->dataBlock.campID = ((CampBaseClass*)GetCampaignObject())->GetCampID();
			landingMessage->dataBlock.pilotID   = pilotSlot;
			FalconSendMessage (landingMessage,FALSE);
			}

	SetFlag (ON_GROUND);
	}
   else
   {
	    // we're taking damage.....
		/*
		** edg: NO DAMAGE!
   		FalconDamageMessage* message;

		  VuTargetEntity *owner_session = (VuTargetEntity*)vuDatabase->Find(OwnerId());
		  message = new FalconDamageMessage (Id(), owner_session);
		  message->dataBlock.gameTime   = SimLibElapsedTime;
		  message->dataBlock.fEntityID  = Id();
		  if (!SimDriver.RunningCampaign() || GetCampaignObject() == NULL || IsDogfight())
		  {
			 message->dataBlock.fCampID = GetCallsignIdx();
			 message->dataBlock.fFlightID = (int)GetCampaignObject();
			 message->dataBlock.fSide   = GetCountry();
		  }
		  else
		  {
			 message->dataBlock.fCampID = ((CampBaseClass*)(GetCampaignObject()))->GetCampID();
			 message->dataBlock.fSide   = ((CampBaseClass*)(GetCampaignObject()))->GetOwner();
		  }
		  message->dataBlock.fPilotID   = pilotSlot;
		  message->dataBlock.fIndex     = Type();
		  message->dataBlock.fWeaponID  = Type();
		  message->dataBlock.fWeaponUID.num_ = 0;

		  message->dataBlock.dEntityID  = Id();
		  if (!SimDriver.RunningCampaign() || GetCampaignObject() == NULL || IsDogfight())
		  {
			 message->dataBlock.dCampID = GetCallsignIdx();
			 message->dataBlock.dFlightID = (int)GetCampaignObject();
			 message->dataBlock.dSide   = GetCountry();
		  }
		  else
		  {
			 message->dataBlock.dCampID = ((CampBaseClass*)(GetCampaignObject()))->GetCampID();
			 message->dataBlock.dSide   = ((CampBaseClass*)(GetCampaignObject()))->GetOwner();
		  }
		  message->dataBlock.dPilotID   = pilotSlot;
		  message->dataBlock.dIndex     = Type();
		  message->dataBlock.damageType = FalconDamageMessage::GroundCollisionDamage;

		  // for now use maxStrength as amount of damage.
		  // later we'll want to add other factors into the equation --
		  // on ground, speed, etc....
		  message->dataBlock.damageStrength = maxStrength * 0.50f;
		  message->dataBlock.damageRandomFact = PRANDFloat();
		  FalconSendMessage (message,FALSE);
		  */

		  // "bounce" the aircraft
      	  // hf->XE.z = OTWDriver.GetGroundLevel(hf->XE.x, hf->XE.y) - 5.0F;
      	  hf->VE.z = -30.0F;
   }
}

#define LOD_MAX_DIST		20000.0f

/*
** Name: GetApproxViewDist
** Description:
**		Sets the distLOD var based on distance from camera
*/
void
HelicopterClass::SetDistLOD ( void )
{
	float absmax, absmid, absmin, tmp;
	Tpoint viewLoc;
	float approxDist;

	// get view pos
	if ( OTWDriver.GetViewpoint() )
	{
		OTWDriver.GetViewpoint()->GetPos( &viewLoc );
	}
	else
	{
		distLOD = 0.01f;
		return;
	}


	absmax = (float)fabs(viewLoc.x - XPos() );
	absmid = (float)fabs(viewLoc.y - YPos() );
	absmin = (float)fabs(viewLoc.z - ZPos() );

	// find the max
	if ( absmax < absmid )
	{
		//swap
		tmp = absmid;
		absmid = absmax;
		absmax = tmp;
	}

	if ( absmax < absmin )
	{
		// absmin is actually the max
		approxDist =  absmin +  ( absmax + absmid ) * 0.5f ;
	}

	approxDist =  absmax + ( absmin + absmid ) * 0.5f ;

	distLOD = max( 0.0f, (LOD_MAX_DIST - approxDist)/LOD_MAX_DIST );

	distLOD *= distLOD;
}


/*
** Name: GetFormationPos
** Description:
**		Gets the position this vehicle should be in in the formation
*/
void
HelicopterClass::GetFormationPos ( float *x, float *y, float *z )
{
	Tpoint *p;


	// if we're the leader just return our own position
	if ( flightLead == this || !flightLead)
	{
		*x = XPos();
		*y = YPos();
		*z = ZPos();
		return;
	}

	// get offset based on our flight index
	p = &gFormationOffsets[ flightIndex ];

	*x = p->x * flightLead->dmx[0][0] - p->y * flightLead->dmx[0][1];
	*y = p->x * flightLead->dmx[0][1] + p->y * flightLead->dmx[0][0];
	*z = p->z;

	*x += flightLead->XPos();
	*y += flightLead->YPos();
	*z += flightLead->ZPos();

}


/*
** This function is called when the a unit dies and is a leader.
** We need to try and find a new leader to promote and set others
** to point at him.
*/
void HelicopterClass::PromoteSubordinates (void)
{
	int i;
	HelicopterClass *theObj;
	HelicopterClass *newLead = NULL;

	MonoPrint("*** Helicopter *** \n");
	MonoPrint("Need to Promote Subordinates!\n");

	if ( !GetCampaignObject()->GetComponents() )
	{
		MonoPrint("No Flight Pointer to determine promotion\n");
		MonoPrint("************** \n");
		return;
	}

	// loop thru elements in flight
	for ( i = 0; i < GetCampaignObject()->NumberOfComponents(); i++ )
	{
		theObj = (HelicopterClass *)GetCampaignObject()->GetComponentEntity( i );

		// num in flight may not match what's actually there!
		if ( theObj == NULL )
			break;

		// do we promote this guy?
		if ( theObj != this && newLead == NULL )
		{
			// edg: observed drawPointer being NULL (!?)
			if ( theObj->drawPointer )
				SetLabel(theObj);
//				theObj->drawPointer->SetLabel (((DrawableBSP *)drawPointer)->Label(), 0xff00ff00);
			newLead = theObj;
			theObj->flightLead = theObj;
			theObj->useDistLOD = FALSE;
			theObj->UnSetLocalFlag( IS_HIDDEN );
			MonoPrint("Heli New Leader Promoted!\n");
			continue;
		}

		// do we set others to newly promoted leader?
		if ( theObj != this && newLead != NULL )
		{
			// yup!
			theObj->flightLead = newLead;
			MonoPrint("Heli Subordinate set to New Leader!\n");
		}
	}

	MonoPrint("************** \n");
}

float HelicopterClass::Mass (void)
{
	return hf->mass;
}

// 2002-02-25 ADDED BY S.G. FlightClass needs to have a combat class like aircrafts.
int HelicopterClass::CombatClass (void)
{
	return SimACDefTable[Falcon4ClassTable[Type() - VU_LAST_ENTITY_TYPE].vehicleDataIndex].combatClass;
}
