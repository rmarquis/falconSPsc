#include "stdhdr.h"
#include "mesg.h"
#include "otwdrive.h"
#include "initdata.h"
#include "waypoint.h"
#include "f4error.h"
#include "object.h"
#include "simobj.h"
#include "simdrive.h"
#include "Graphics\Include\drawsgmt.h"
#include "Graphics\Include\drawgrnd.h"
#include "Graphics\Include\drawguys.h"
#include "Graphics\Include\drawbsp.h"
#include "entity.h"
#include "classtbl.h"
#include "sms.h"
#include "fcc.h"
#include "PilotInputs.h"
#include "MsgInc\DamageMsg.h"
#include "guns.h"
#include "hardpnt.h"
#include "campwp.h"
#include "sfx.h"
#include "Unit.h"
#include "fsound.h"
#include "soundfx.h"
#include "fakerand.h"
#include "acmi\src\include\acmirec.h"
#include "ui\include\ui_ia.h"
#include "gndunit.h"
#include "radar.h"
#include "handoff.h"
#include "ground.h"
#include "Team.h"
#include "dofsnswitches.h"
/* 2001-03-21 S.G. */#include "atm.h"

#ifdef USE_SH_POOLS
MEM_POOL	GroundClass::pool;
#endif

void CalcTransformMatrix (SimBaseClass* theObject);
void Trigenometry(SimMoverClass *platform);
void SetLabel (SimBaseClass* theObject);
GNDAIClass *NewGroundAI (GroundClass *us, int position, BOOL isFirst, int skill);

// for debugging
#ifdef DEBUG
int gNumGroundInit = 0;
int gNumGroundConstruct = 0;
int gNumGroundDestruct = 0;
int gNumGroundSleep = 0;
int gNumGroundWake = 0;

int gNumGroundTrucks = 0;
int gNumGroundCrews = 0;
#endif
    
#ifdef CHECK_PROC_TIMES
ulong gACMI = 0;
ulong gSpecCase = 0;
ulong gTurret = 0;
ulong gLOD = 0;
ulong gFire = 0;
ulong gMove = 0;
ulong gThink = 0;
ulong gTarg = 0;
ulong gRadar = 0;
ulong gFireCont = 0;
ulong gSimVeh = 0;
ulong gSFX = 0;
#endif

extern bool g_bSAM2D3DHandover;

#define MAX_NCTR_RANGE    (60.0F * NM_TO_FT) // 2002-02-12 S.G. See RadarDoppler.h

// Re-written by KCK 2/11/98
void GroundClass::SetupGNDAI ( SimInitDataClass *idata )
{
	ShiAssert( idata );
	
	BOOL	isFirst		= (idata->vehicleInUnit == 0);
	int		position	= idata->campSlot*3 + idata->inSlot;
	int		skill		= idata->skill;
	
	gai = NewGroundAI(this, position, isFirst, skill);
	
#ifdef DEBUG
	gNumGroundInit++;
#endif
}

GroundClass::GroundClass (VU_BYTE** stream) : SimVehicleClass (stream)
	{
	InitData();
#ifdef DEBUG
	gNumGroundConstruct++;
#endif
	}

GroundClass::GroundClass (FILE* filePtr) : SimVehicleClass (filePtr)
	{
	InitData();
#ifdef DEBUG
	gNumGroundConstruct++;
#endif
	}

GroundClass::GroundClass (int type) : SimVehicleClass (type)
	{
	InitData();
#ifdef DEBUG
	gNumGroundConstruct++;
#endif
	}
    
void GroundClass::InitData (void)
{

	// KCK: These were getting reset to the leader's each frame, which is moronic.
	// Just set constantly here.
	// timer for target processing
//	processRate = (int)(1.0f + 3.0f * PRANDFloatPos()) * SEC_TO_MSEC;
	processRate = 2 * SEC_TO_MSEC;
	// timer for movement decisions and firing processing
//	thoughtRate = (int)(2.0f + 8.0f * PRANDFloatPos()) * SEC_TO_MSEC;
	thoughtRate = 6 * SEC_TO_MSEC;

	// Set last values to now
	lastProcess = SimLibElapsedTime;
	lastThought = SimLibElapsedTime;
	nextSamFireTime = SimLibElapsedTime;
	nextTargetUpdate = SimLibElapsedTime; // 2002-02-26 ADDED BY S.G. RadarDigi Exec interval (used to be per frame).
	allowSamFire = TRUE;

//	crewDrawable = NULL;
	truckDrawable = NULL;

	battalionFireControl = NULL;
}
    
GroundClass::~GroundClass (void)
    {
	// OTWDriver.RemoveObject(dustTrail, TRUE);
	delete gai;

#ifdef DEBUG
	gNumGroundDestruct++;
#endif

	if (Sms)
		delete Sms;

//	ShiAssert (crewDrawable == NULL && truckDrawable == NULL);
	}
    
void GroundClass::Init (SimInitDataClass* initData)
{
float nextX, nextY;
float range, velocity;
float wp1X, wp1Y, wp1Z;
float wp2X, wp2Y, wp2Z;
int i;
WayPointClass* atWaypoint;
mlTrig trig;
VehicleClassDataType* vc;

	vc = GetVehicleClassData(Type() - VU_LAST_ENTITY_TYPE);
	ShiAssert (vc);

    // dustTrail = new DrawableTrail(TRAIL_DUST);
	isFootSquad		= FALSE;
	isEmitter		= FALSE;
	needKeepAlive	= FALSE;

	hasCrew			= (vc->Flags & VEH_HAS_CREW)		? TRUE : FALSE;
	isTowed			= (vc->Flags & VEH_IS_TOWED)		? TRUE : FALSE;
	isShip			= (GetDomain() == DOMAIN_SEA)		? TRUE : FALSE;

	// check for radar emitter
	if ( vc->RadarType != RDR_NO_RADAR )
	{
		isEmitter = TRUE;
	}

/*	// KCK: This is done in iaction.cpp now.. However, it currently only eliminates SAMs from 
	// SAM Battalions (i.e: Not the random SA-7 loaded here or there).
	// We may wish to do both just to be safe.
	// if running instant action, filter weapons
	if ( SimDriver.RunningInstantAction() )
	{
		for (i=0; i<HARDPOINT_MAX; i++)
		{
			if (initData->weapons[i])
			{
				classPtr = &(Falcon4ClassTable[GetWeaponDescriptionIndex(initData->weapon[i])]);
			
				if (!InstantActionSettings.SamSites && classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_MISSILE)
				{
					initData->weapon[i] = 0;
					initData->weapons[i] = 0;
				 }
			
				if (!InstantActionSettings.AAASites && classPtr->vuClassData.classInfo_[VU_TYPE] == TYPE_GUN)
				{
					initData->weapon[i] = 0;
					initData->weapons[i] = 0;
				}
			}
		}
	}
*/

	SimVehicleClass::Init (initData);

	// 2002-01-20 ADDED BY S.G. At time of creation, the radar will take the mode of the battalion instead of being off until it finds a target by itself (and can it find it if its radar is off!).
    //                          SimVehicleClass::Init created the radar so it's safe to do it here...
	if (isEmitter) {
		if (GetCampaignObject()->GetRadarMode() != FEC_RADAR_OFF) {
			RadarClass *radar = NULL;
			radar = (RadarClass*)FindSensor( this, SensorClass::Radar );
			ShiAssert( radar );
			radar->SetEmitting(TRUE);

			
// 2002-04-22 MN last fix for FalconSP3 - this was a good intention to keep a 2D target targetted by a deaggregating unit,
// however - it doesn't work this way. The campaign air target derived falconentity does not correlate with the deaggregated aircraft.
// The SAM's radars would stay stuck at TRACK S1 or TRACK S3 and won't engage. Symptom was the not changing range to the target (.label 4)
// Now with this code removed, SAMs should work correctly again. As we have large SAM bubble sizes - it doesn't really matter if we
// need to go through all search states in the SIM again - because SAM's are faster in GUIDE mode than in maximum missile range.
			if (g_bSAM2D3DHandover)
			{
// 2002-03-21 ADDED BY S.G. In addition, we need to set our radar's target RFN (right f*cking now) and run a sensor sweep on it so it's valid by the time TargetProcessing is called.
				FalconEntity	*campTargetEntity = ((UnitClass *)GetCampaignObject())->GetAirTarget();
				if (campTargetEntity)
				{
#ifdef DEBUG
					SetTarget( new SimObjectType(OBJ_TAG, this, campTargetEntity) );
#else	
					SetTarget( new SimObjectType(campTargetEntity) );
#endif
					CalcRelAzElRangeAta(this, targetPtr);

					radar->SetDesiredTarget(targetPtr);
					radar->SetFlag(RadarClass::FirstSweep);
					radar->Exec(targetList);
				}
			// END OF ADDED SECTION 2002-03-21
			}
		}
	}
	// END OF ADDED SECTION 2002-01-20

	SetFlag(ON_GROUND);
	SetPowerOutput(1.0F);	// Assume our motor is running all the time

	SetPosition (initData->x, initData->y, OTWDriver.GetGroundLevel(initData->x, initData->y));
	SetYPR(initData->heading, 0.0F, 0.0F);

	SetupGNDAI (initData);
    
	if (initData->ptIndex)
		{
		// Don't move if we've got an assigned point
		gai->moveState = GNDAI_MOVE_HALTED;
		gai->moveFlags |= GNDAI_MOVE_FIXED_POSITIONS;
		}

	CalcTransformMatrix (this);

	strength        = 100.0F;

	// Check for Campaign mode
	// we don't follow waypoints here
	switch (gai->moveState)
		{
		case GNDAI_MOVE_GENERAL:
			{
			waypoint = curWaypoint = NULL;
			numWaypoints = 0;
			DeleteWPList(initData->waypointList);
			InitFromCampaignUnit();
			}
			break;
		case GNDAI_MOVE_WAYPOINT:
			{
			waypoint        = initData->waypointList;
			numWaypoints    = initData->numWaypoints;
			curWaypoint     = waypoint;
			if (curWaypoint)
				{
				// Corrent initial heading/velocity
				// Find the waypoint to go to.
				atWaypoint = curWaypoint;
				for (i=0; i<initData->currentWaypoint; i++)
					{
					atWaypoint = curWaypoint;
					curWaypoint = curWaypoint->GetNextWP();
					}

				// If current is the on we're at, set for the next one.
				if (curWaypoint == atWaypoint)
					curWaypoint = curWaypoint->GetNextWP();

				atWaypoint->GetLocation (&wp1X, &wp1Y, &wp1Z);

				if (curWaypoint == NULL)
					{
					wp1X = initData->x;
					wp1Y = initData->y;
					curWaypoint = atWaypoint;
					}

				if (curWaypoint)
					{
					curWaypoint->GetLocation (&wp2X, &wp2Y, &wp2Z);

					SetYPR ((float)atan2 (wp2Y - wp1Y, wp2X - wp1X), 0.0F, 0.0F);

					nextX = wp2X;
					nextY = wp2Y;

					range = (float)sqrt((wp1X - nextX) * (wp1X - nextX) + (wp1Y - nextY) * (wp1Y - nextY));
					velocity = range / ((curWaypoint->GetWPArrivalTime() - SimLibElapsedTime) / SEC_TO_MSEC);

					if ((curWaypoint->GetWPArrivalTime() - SimLibElapsedTime) < 1 * SEC_TO_MSEC)
						velocity = 0.0F;

					SetVt(velocity);
					SetKias(velocity * FTPSEC_TO_KNOTS);
					mlSinCos (&trig, Yaw());
					SetDelta (velocity * trig.cos, velocity * trig.sin, 0.0F);
					SetYPRDelta (0.0F, 0.0F, 0.0F);
					}
				else
					{
					SetDelta (0.0F, 0.0F, 0.0F);
					SetVt(0.0F);
					SetKias(0.0F);
					SetYPRDelta (0.0F, 0.0F, 0.0F);
					}
				}
			}
			break;
		default:
			{
			SetDelta (0.0F, 0.0F, 0.0F);
			SetVt(0.0F);
			SetKias(0.0F);
			SetYPRDelta (0.0F, 0.0F, 0.0F);
			gai->moveState = GNDAI_MOVE_HALTED;
			waypoint = curWaypoint = NULL;
			numWaypoints = 0;
			DeleteWPList(initData->waypointList);
			InitFromCampaignUnit();
			}
			break;
		}

	theInputs   = new PilotInputs;

	// Creat our SMS
	Sms = new SMSBaseClass (this, initData->weapon,initData->weapons);

	uchar dam[10] = {100};
	for(i = 0; i < 10; i++)
		dam[i] = 100;
	Sms->SelectBestWeapon (dam, LowAir, -1);
	if (Sms->CurHardpoint() != -1)
		isAirCapable = TRUE;
	else
		isAirCapable = FALSE;

	Sms->SelectBestWeapon (dam, NoMove, -1);
	if (Sms->CurHardpoint() != -1)
		isGroundCapable = TRUE;
	else
		isGroundCapable = FALSE;
	Sms->SetCurHardpoint(-1);

//	if (initData && !initData->campSlot && !initData->inSlot && (int)GetCampaignObject() > MAX_IA_CAMP_UNIT)
//		MonoPrint("New battalion, head:%3.3f, form:%d, ihead:%3.3f, moving:%3.3f, dx:%3.3f, dy:%3.3f\n",Yaw(),((Unit)GetCampaignObject())->GetUnitFormation(),gai->ideal_h,atan2(YDelta(),XDelta()),gai->ideal_x-XPos(),gai->ideal_y-YPos());

	if ((GetType() == TYPE_WHEELED && GetSType() == STYPE_WHEELED_AIR_DEFENSE) ||
		(GetType() == TYPE_WHEELED && GetSType() == STYPE_WHEELED_AAA) ||
		(GetType() == TYPE_TRACKED && GetSType() == STYPE_TRACKED_AIR_DEFENSE) ||
		(GetType() == TYPE_TRACKED && GetSType() == STYPE_TRACKED_AAA) ||
		(GetType() == TYPE_TOWED && GetSType() == STYPE_TOWED_AAA))
	{
		isAirDefense = TRUE;
		// If we're an airdefense thingy, elevate our gun, and point in a random direction
		SetDOF(AIRDEF_ELEV, 60.0f * DTR);
		SetDOF(AIRDEF_ELEV2, 60.0f * DTR);
		SetDOF(AIRDEF_AZIMUTH, 180.0F*DTR - rand()/(float)RAND_MAX * 360.0F*DTR);
	}
	else
		isAirDefense = FALSE;
}
    
/*
** GroundClass Exec() function.
** NOTE: returns TRUE if we've processed this frame.  FALSE if we're to do
** dead reckoning (in VU)
*/
int GroundClass::Exec (void)
{
	Tpoint pos;
    Tpoint vec;
	float speedScale;
	float groundZ;
	float	labelLOD;
	float	drawLOD;
	RadarClass *radar = NULL;
	
#ifdef CHECK_PROC_TIMES
	ulong procTime = 0;
	procTime = GetTickCount();
#endif

	// dead? -- we do nothing
	if ( IsDead() )
		return FALSE;
	
    // if damaged 
	if ( pctStrength < 0.5f )
	{
		if ( sfxTimer > 1.5f - gai->distLOD * 1.3 )
		{
			// reset the timer
			sfxTimer = 0.0f;
			
			pos.x = XPos();
			pos.y = YPos();
			pos.z = OTWDriver.GetGroundLevel( pos.x, pos.y ) - 10.0f;

			vec.x = PRANDFloat() * 20.0f;
			vec.y = PRANDFloat() * 20.0f;
			vec.z = PRANDFloat() * 20.0f;
			
			OTWDriver.AddSfxRequest(
				new SfxClass (SFX_TRAILSMOKE,			// type
				SFX_MOVES | SFX_NO_GROUND_CHECK,						// flags
				&pos,							// world pos
				&vec,							// vector
				3.5f,							// time to live
				4.5f ));		// scale
		}
	}
	
	if (IsExploding())
	{
		// KCK: I've never seen this section of code executed. Maybe it gets hit, but I doubt
		// it.
		if ( !IsSetFlag( SHOW_EXPLOSION ) )
		{
			// Show the explosion
			Tpoint pos, vec;
			Falcon4EntityClassType *classPtr = (Falcon4EntityClassType *)EntityType();
			DrawableGroundVehicle *destroyedPtr;
			
			pos.x = XPos();
			pos.y = YPos();
			pos.z = OTWDriver.GetGroundLevel( pos.x, pos.y ) - 10.0f;
			
			vec.x = 0.0f;
			vec.y = 0.0f;
			vec.z = 0.0f;
			
			// create a new drawable for destroyed vehicle
			// sometimes.....
			if ( rand() & 1 )
			{
				destroyedPtr = new DrawableGroundVehicle(
					classPtr->visType[3],
					&pos,
					Yaw(),
					1.0f );
				
				groundZ = PRANDFloatPos() * 60.0f + 15.0f;
				
				OTWDriver.AddSfxRequest(
					new SfxClass (SFX_BURNING_PART,				// type
					&pos,							// world pos
					&vec,							// 
					(DrawableBSP *)destroyedPtr,
					groundZ,							// time to live
					1.0f ) );		// scale
				
				pos.z += 10.0f;
				OTWDriver.AddSfxRequest(
					new SfxClass (SFX_GROUND_GLOW,				// type
					&pos,							// world pos
					groundZ,							// time to live
					100.0f ) );		// scale
			}
			
			
			pos.z -= 20.0f;
			OTWDriver.AddSfxRequest(
				new SfxClass (SFX_HIT_EXPLOSION_DEBRISTRAIL,				// type
				&pos,							// world pos
				1.5f,							// time to live
				100.0f ) );		// scale
			
			
			// make sure we don't do it again...
			SetFlag( SHOW_EXPLOSION );
			
//			MonoPrint ("Ground %d dead at %8ld\n", Id().num_, SimLibElapsedTime);
			
			// we can now kill it immediately
			SetDead(TRUE);  
		}
		return FALSE;
	}
#ifdef CHECK_PROC_TIMES
	procTime = GetTickCount() - procTime;
	gSFX += procTime;
	procTime = GetTickCount();
#endif
	
	// exec any base functionality
	SimVehicleClass::Exec();
	
#ifdef CHECK_PROC_TIMES
	procTime = GetTickCount() - procTime;
	gSimVeh += procTime;
	procTime = GetTickCount();
#endif
	// edg: I dunno.  It seems like the Z value for drawpointer is frequently
	// not in the correct place
	if ( drawPointer )
		drawPointer->GetPosition( &pos );
	else
		return FALSE;
	groundZ = pos.z;		// - 0.7f; KCK: WTF is this?
	
	
	
	// Movement/Targeting for local entities
	if (IsLocal() && SimDriver.MotionOn())
	{
		//I commented this out, because it is done in gai->ProcessTargeting down below DSP 4/30/99
		// Refresh our target pointer (if any)
		//SetTarget( SimCampHandoff( targetPtr, targetList, HANDOFF_RANDOM ) );
		
		// Look for someone to do radar fire control for us
		FindBattalionFireControl();
		
#ifdef CHECK_PROC_TIMES
	procTime = GetTickCount() - procTime;
	gFireCont += procTime;
	procTime = GetTickCount();
#endif

// 2001-03-26 ADDED BY S.G. NEED TO KNOW IF THE RADAR CALLED SetSpotted
		int spottedSet = FALSE;
// END OF ADDED SECTION

		// 2002-03-21 ADDED BY S.G. If localData only has zeros, there is a good chance they are not valid (should not happen here though)... 
		if (targetPtr) {
			SimObjectLocalData* localData= targetPtr->localData;

			if (localData->ataFrom == 0.0f && localData->az == 0.0f  && localData->el == 0.0f && localData->range == 0.0f) {
//				ShiAssert(!"localData values are all zeros!");
				CalcRelAzElRangeAta(this, targetPtr);
			}
		}
		// END OF ADDED SECTION 2002-03-21

		// check for sending radar emmisions
		if ( isEmitter && nextTargetUpdate < SimLibElapsedTime) // 2002-02-26 MODIFIED BY S.G. Added the nextTargetUpdate check to prevent the radar code to run on every frame!
		{
			nextTargetUpdate = SimLibElapsedTime + (5 - gai->skillLevel) * SEC_TO_MSEC; // // 2002-02-26 ADDED BY S.G. Next radar scan is 1 sec for aces, 2 for vets, etc ...

			radar = (RadarClass*)FindSensor( this, SensorClass::Radar );
			ShiAssert( radar );
			radar->Exec( targetList );

// 2001-03-26 ADDED BY S.G. IF WE CAN SEE THE RADAR'S TARGET AND WE ARE A AIR DEFENSE THINGY NOT IN A BKOGEN MORAL STATE, MARK IT AS SPOTTED IF WE'RE BRIGHT ENOUGH
			if (radar->CurrentTarget() && gai->skillLevel >= 3 && ((UnitClass *)GetCampaignObject())->GetSType() == STYPE_UNIT_AIR_DEFENSE && !((UnitClass *)GetCampaignObject())->Broken()) {
				CampBaseClass *campBaseObj;
				if (radar->CurrentTarget()->BaseData()->IsSim())
					campBaseObj = ((SimBaseClass *)radar->CurrentTarget()->BaseData())->GetCampaignObject();
				else
					campBaseObj = (CampBaseClass *)radar->CurrentTarget()->BaseData();

				// JB 011002 If campBaseObj is NULL the target may be chaff

				if (campBaseObj && !(campBaseObj->GetSpotted(GetTeam())) && campBaseObj->IsFlight())
					RequestIntercept((FlightClass *)campBaseObj, GetTeam());

				spottedSet = TRUE;
				if (campBaseObj && radar->GetRadarDatFile())
					campBaseObj->SetSpotted(GetTeam(),TheCampaign.CurrentTime, (radar->radarData->flag & RAD_NCTR) != 0 && radar->CurrentTarget()->localData && radar->CurrentTarget()->localData->ataFrom < 45.0f * DTR && radar->CurrentTarget()->localData->range < radar->GetRadarDatFile()->MaxNctrRange / (2.0f * (16.0f - (float)gai->skillLevel) / 16.0f)); // 2002-03-05 MODIFIED BY S.G. target's aspect and skill used in the equation
			}
// END OF ADDED SECTION
		}

// 2001-03-26 ADDED BY S.G. IF THE BATTALION LEAD HAS LOS ON IT AND WE ARE A AIR DEFENSE THINGY NOT IN A BKOGEN MORAL STATE, MARK IT AS SPOTTED IF WE'RE BRIGHT ENOUGH
// 2002-02-11 MODIFED BY S.G. Since I only identify visually, need to perform this even if spotted by radar in case I can ID it.
		if (/*!spottedSet && */ gai->skillLevel >= 3 && ((UnitClass *)GetCampaignObject())->GetSType() == STYPE_UNIT_AIR_DEFENSE && gai == gai->battalionCommand && !((UnitClass *)GetCampaignObject())->Broken() && gai->GetAirTargetPtr() && CheckLOS(gai->GetAirTargetPtr())) {
			CampBaseClass *campBaseObj;
			if (gai->GetAirTargetPtr()->BaseData()->IsSim())
				campBaseObj = ((SimBaseClass *)gai->GetAirTargetPtr()->BaseData())->GetCampaignObject();
			else
				campBaseObj = (CampBaseClass *)gai->GetAirTargetPtr()->BaseData();

			// JB 011002 If campBaseObj is NULL the target may be chaff

			if (!spottedSet && campBaseObj && !(campBaseObj->GetSpotted(GetTeam())) && campBaseObj->IsFlight())
				RequestIntercept((FlightClass *)campBaseObj, GetTeam());

			if (campBaseObj)
				campBaseObj->SetSpotted(GetTeam(),TheCampaign.CurrentTime, 1); // 2002-02-11 MODIFIED BY S.G. Visual detection means identified as well
		}
// END OF ADDED SECTION

#ifdef CHECK_PROC_TIMES
	procTime = GetTickCount() - procTime;
	gRadar += procTime;
	procTime = GetTickCount();
#endif

		// KCK: When should we run a target update cycle?
		if (SimLibElapsedTime > lastProcess)
		{
			gai->ProcessTargeting();
			lastProcess = SimLibElapsedTime + processRate;
		}

#ifdef CHECK_PROC_TIMES
	procTime = GetTickCount() - procTime;
	gTarg += procTime;
	procTime = GetTickCount();
#endif
		// KCK: Check if it's ok to think
		if (SimLibElapsedTime > lastThought )
		{
			// do movement and (possibly) firing....
			gai->Process ();
			lastThought = SimLibElapsedTime + thoughtRate;
		}

		if(SimLibElapsedTime > nextSamFireTime  && !allowSamFire)
		{
			allowSamFire = TRUE;
		}

#ifdef CHECK_PROC_TIMES
	procTime = GetTickCount() - procTime;
	gThink += procTime;
	procTime = GetTickCount();
#endif
		// Move and update delta;
		gai->Move_Towards_Dest();
		
		// edg: always insure that our Z position is valid for the entity.
		// the draw pointer should have this value
		// KCK NOTE: The Z we have is actually LAST FRAME's Z. Probably not a big deal.
		SetPosition(
			XPos() + XDelta() * SimLibMajorFrameTime,
			YPos() + YDelta() * SimLibMajorFrameTime,
			groundZ );
#ifdef CHECK_PROC_TIMES
	procTime = GetTickCount() - procTime;
	gMove += procTime;
	procTime = GetTickCount();
#endif
		// do firing
		// this also does weapon keep alive
		if ( Sms )
			gai->Fire();
#ifdef CHECK_PROC_TIMES
	procTime = GetTickCount() - procTime;
	gFire += procTime;
	procTime = GetTickCount();
#endif
	}
	

	// KCK: I simplified this some. This is now speed squared.
	speedScale = XDelta()*XDelta() + YDelta()*YDelta();
	
	// set our level of detail
	if ( gai == gai->battalionCommand )
		gai->SetDistLOD();
	else
		gai->distLOD = gai->battalionCommand->distLOD;
	
	// do some extra LOD stuff: if the unit is not a lead veh amd the
	// distLOD is less than a certain value, remove it from the draw
	// list.
	if (drawPointer && gai->rank != GNDAI_BATTALION_COMMANDER)
	{
		// distLOD cutoff by ranking (KCK: This is explicit for testing, could be a formula/table)
		if (gai->rank & GNDAI_COMPANY_LEADER)
		{
			labelLOD = .5F;
			drawLOD = .25F;
		}
		else if (gai->rank & GNDAI_PLATOON_LEADER)
		{
			labelLOD = .925F;
			drawLOD = .5F;
		}
		else
		{
			labelLOD = 1.1F;
			drawLOD = .75F;
		}
		// Determine wether to draw label or not
		if (gai->distLOD < labelLOD)
		{
			if (!IsSetLocalFlag(NOT_LABELED))
			{
				drawPointer->SetLabel ("", 0xff00ff00);		// Don't label
				SetLocalFlag(NOT_LABELED);
			}
		}
		else if (IsSetLocalFlag(NOT_LABELED))
		{
			SetLabel(this);
			UnSetLocalFlag(NOT_LABELED);
		}
	}
	
#ifdef CHECK_PROC_TIMES
	procTime = GetTickCount() - procTime;
	gLOD += procTime;
	procTime = GetTickCount();
#endif

	if(!targetPtr)
	{
		//rotate turret to be pointing forward again
		float maxAz = TURRET_ROTATE_RATE * SimLibMajorFrameTime;
		float maxEl = TURRET_ELEVATE_RATE * SimLibMajorFrameTime;
		float newEl;
		if (isAirDefense)
			newEl = 60.0F*DTR;
		else 
			newEl = 0.0F;

		float delta = newEl - GetDOFValue(AIRDEF_ELEV);
		if(delta > 180.0F*DTR)
			delta -= 180.0F*DTR;
		else if(delta < -180.0F*DTR)
			delta += 180.0F*DTR;

		// Do elevation adjustments
		if (delta > maxEl)
		    SetDOFInc(AIRDEF_ELEV, maxEl);
		else if (delta < -maxEl)
		    SetDOFInc(AIRDEF_ELEV, -maxEl);
		else
		    SetDOF(AIRDEF_ELEV, newEl);

		SetDOF(AIRDEF_ELEV, min(85.0F*DTR, max(GetDOFValue(AIRDEF_ELEV), 0.0F)));
		SetDOF(AIRDEF_ELEV2, GetDOFValue(AIRDEF_ELEV));
		
		delta = 0.0F - GetDOFValue(AIRDEF_AZIMUTH);
		if(delta > 180.0F*DTR)
		    delta -= 180.0F*DTR;
		else if(delta < -180.0F*DTR)
		    delta += 180.0F*DTR;
		
		// Now do the azmuth adjustments
		if (delta > maxAz)
		    SetDOFInc(AIRDEF_AZIMUTH, maxAz);
		else if (delta < -maxAz)
		    SetDOFInc(AIRDEF_AZIMUTH, -maxAz);
		else
		    SetDOF(AIRDEF_AZIMUTH, 0.0F);
	}
	
#ifdef CHECK_PROC_TIMES
	procTime = GetTickCount() - procTime;
	gTurret += procTime;
	procTime = GetTickCount();
#endif

	// Special shit by ground type
	if ( isFootSquad )
	{
		ShiAssert( drawPointer->GetClass() == DrawableObject::Guys );
		if ( speedScale > 0.0f )
		{
			// Couldn't this be done in the drawable class's update function???
			((DrawableGuys*)drawPointer)->SetSquadMoving( TRUE );
		}
		else
		{
			// Couldn't this be done in the drawable class's update function???
			((DrawableGuys*)drawPointer)->SetSquadMoving( FALSE );
		}
		
		// If we're less than 80% of the way from "FAR" toward the viewer, just draw one guy
		// otherwise, put 5 guys in a squad.
		if (gai->distLOD < 0.8f) {
			((DrawableGuys*)drawPointer)->SetNumInSquad( 1 );
		} else {
			((DrawableGuys*)drawPointer)->SetNumInSquad( 5 );
		}
	} 
	// We're not a foot squad, so do the vehicle stuff
	else if ( !IsSetLocalFlag( IS_HIDDEN ) && speedScale > 300.0f )
	{
		// speedScale /= ( 900.0f * KPH_TO_FPS * KPH_TO_FPS);		// essentially 1.0F at 30 mph

	    // JPO - for engine noise
	    VehicleClassDataType *vc = GetVehicleClassData(Type() - VU_LAST_ENTITY_TYPE);
	    ShiAssert(FALSE == F4IsBadReadPtr(vc, sizeof *vc));
		// (a) Make sound:
		// everything sounds like a tank right now
		if ( GetCampaignObject()->IsBattalion() )
		{
		    if (vc)
			F4SoundFXSetPos( vc->EngineSound, 0, XPos(), YPos(), ZPos(), 1.0f );
		    else
			F4SoundFXSetPos( SFX_TANK, 0, XPos(), YPos(), ZPos(), 1.0f );
			
			// (b) Make dust
			// dustTimer += SimLibMajorFrameTime;
			// if ( dustTimer > max( 0.2f,  4.5f - speedScale - gai->distLOD * 3.3f ) )
			if ( ((rand() & 7) == 7) &&
				gSfxCount[ SFX_GROUND_DUSTCLOUD ] < gSfxLODCutoff &&
				gTotSfx < gSfxLODTotCutoff )
			{
				// reset the timer
				// dustTimer = 0.0f;
				
				pos.x += PRANDFloat() * 5.0f;
				pos.y += PRANDFloat() * 5.0f;
				pos.z = groundZ;
				vec.x = PRANDFloat() * 5.0f;
				vec.y = PRANDFloat() * 5.0f;
				vec.z = -20.0f;
				
				OTWDriver.AddSfxRequest(
					new SfxClass (SFX_GROUND_DUSTCLOUD,			// type
					SFX_USES_GRAVITY | SFX_NO_DOWN_VECTOR | SFX_MOVES | SFX_NO_GROUND_CHECK,
					&pos,							// world pos
					&vec,
					1.0f,							// time to live
					8.5f ));		// scale
				
			}
			
			// (c) Make sure we're using our 'Moving' model (i.e. Trucked artillery, APC, etc)
			if (truckDrawable)
			{
				// Keep truck 20 feet behind us (HACK HACK)
				Tpoint		truckPos;
				mlTrig		trig;
				mlSinCos (&trig, Yaw());
				truckPos.x = XPos()-20.0F*trig.cos;
				truckPos.y = YPos()-20.0F*trig.sin;
				truckPos.z = ZPos();
				truckDrawable->Update(&truckPos, Yaw()+PI);
			}
			if (isTowed || hasCrew)
				SetSwitch(0,0x2);
		}
		else // itsa task force
		{
		    if (vc)
			F4SoundFXSetPos( vc->EngineSound, 0, XPos(), YPos(), ZPos(), 1.0f );
		    else
			F4SoundFXSetPos( SFX_SHIP, 0, XPos(), YPos(), ZPos(), 1.0f );
			static int thesfx = SFX_WATER_WAKE;
			if ( ((rand() & 7) == 7) &&
				gSfxCount[ thesfx ] < gSfxLODCutoff &&
				gTotSfx < gSfxLODTotCutoff )
			{
				// reset the timer
				// dustTimer = 0.0f;
				float radius;
				float ttl;
				static float trailspd = 5.0f;
				static float bowfx = 0.92f;
				static float sternfx = 0.75f;
				float spdratio = Vt() / ((UnitClass*)GetCampaignObject())->GetMaxSpeed();
				if ( drawPointer )
					radius = drawPointer->Radius(); // JPO from 0.15 - now done inline
				else
					radius = 90.0f;
				
				pos.x += PRANDFloat() * 5.0f;
				pos.y += PRANDFloat() * 5.0f;
				pos.z = groundZ;
				// sometimes locate foam at bow, other times at stern
				if ( (rand() & 3) == 0 ) // bow
				{
					pos.x += dmx[0][0] * radius * bowfx;
					pos.y += dmx[0][1] * radius * bowfx;
					ttl = spdratio * PRANDFloatPos() * 15.0f;
				}
				else
				{
					pos.x -= dmx[0][0] * radius * sternfx;
					pos.y -= dmx[0][1] * radius * sternfx;
					ttl = spdratio * PRANDFloatPos() * 50.0f;
				}
				// JPO - think this is sideways on.
				vec.x = dmx[1][0] * spdratio * PRANDFloat() * trailspd;
				vec.y = dmx[1][1] * spdratio * PRANDFloat() * trailspd;
				vec.z = 0.5f; // from -20 JPO
				
				OTWDriver.AddSfxRequest(
					new SfxClass (thesfx,			// type
					SFX_USES_GRAVITY | SFX_NO_DOWN_VECTOR | SFX_MOVES | SFX_NO_GROUND_CHECK,
					&pos,							// world pos
					&vec,
					ttl,							// time to live
					5.5f ));		// scale JPO from 20.5
				
			}
		}
	}
	// Otherwise, we're not moving or are hidden. Do some stuff
	else
	{
		// (b) Make sure we're using our 'Holding' model (i.e. Unlimbered artillery, troops prone, etc)
		if (truckDrawable)
		{
			// Once we stop, our truck doesn't move at all - but sits further away than when moving
			Tpoint truckPos;
			truckPos.x = XPos() + 40.0F;
			truckPos.y = YPos();
			truckPos.z = 0.0F;
			truckDrawable->Update(&truckPos, Yaw());
		}
		if (isTowed || hasCrew)
			SetSwitch(0,0x1);
	}
	
#ifdef CHECK_PROC_TIMES
	procTime = GetTickCount() - procTime;
	gSpecCase += procTime;
	procTime = GetTickCount();
#endif

	// ACMI Output
    if (gACMIRec.IsRecording() && (SimLibFrameCount & 0x0f ) == 0)
	{
		ACMIGenPositionRecord genPos;
		
		genPos.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
		genPos.data.type = Type();
		genPos.data.uniqueID = ACMIIDTable->Add(Id(),NULL,TeamInfo[GetTeam()]->GetColor());//.num_;
		genPos.data.x = XPos();
		genPos.data.y = YPos();
		genPos.data.z = ZPos();
		genPos.data.roll = Roll();
		genPos.data.pitch = Pitch();
		genPos.data.yaw = Yaw();
// Remove		genPos.data.teamColor = TeamInfo[GetTeam()]->GetColor();
		gACMIRec.GenPositionRecord( &genPos );
	}

#ifdef CHECK_PROC_TIMES
	procTime = GetTickCount() - procTime;
	gACMI += procTime;
	procTime = GetTickCount();
#endif

	return IsLocal();
}
    
void GroundClass::ApplyDamage (FalconDamageMessage *damageMessage)
{
   if (IsDead())
	  return;

   SimVehicleClass::ApplyDamage (damageMessage);
   lastDamageTime = SimLibElapsedTime;
}

void GroundClass::SetDead (int flag)
{
    SimVehicleClass::SetDead(flag);
}

#ifdef DEBUG
static int CALLS_TO_ADD_WEAPONS		= 0;
static int CALLS_TO_FREE_WEAPONS	= 0;
#endif

int GroundClass::Wake(void)
{
int retval = 0;

	if (IsAwake())
		return retval;

	Tpoint pos;

#ifdef DEBUG
	gNumGroundWake++;
#endif

	SimVehicleClass::Wake();

	if (Sms) {
		Sms->AddWeaponGraphics();
#ifdef DEBUG
		CALLS_TO_ADD_WEAPONS++;
#endif
	}

	// Pick a groupId. KCK: Is this even being used?
	ShiAssert (GetCampaignObject());
	groupId = GetCampaignObject()->Id().num_;

	if ( !gai->rank )
	{
		drawPointer->SetLabel ("", 0xff00ff00);
	}

	// edg: I'm not sure if drawpointer has valid position yet?
	drawPointer->GetPosition( &pos );
    SetPosition (XPos(), YPos(), pos.z - 0.7f );
	ShiAssert(XPos() > 0.0F && YPos() > 0.0F )

	// determine if its a foot squad or not -- Real thinking done in addobj.cpp
	isFootSquad = (drawPointer->GetClass() == DrawableObject::Guys);

//	ShiAssert (crewDrawable == NULL && truckDrawable == NULL);

	// Determine if this vehicle has a truck and create it, if needed
	if (drawPointer && isTowed)
		{
		// Place truck 20 feet behind us
		Tpoint		simView;
		int			vistype;
		mlTrig		trig;

		if (TeamInfo[GetCountry()] && (TeamInfo[GetCountry()]->equipment == toe_us || TeamInfo[GetCountry()]->equipment == toe_rok))
			vistype = Falcon4ClassTable[F4GenericUSTruckType].visType[Status() & VIS_TYPE_MASK];
		else
			vistype = Falcon4ClassTable[F4GenericTruckType].visType[Status() & VIS_TYPE_MASK];
		mlSinCos (&trig, Yaw());
		simView.x = XPos()-20.0F*trig.cos;
		simView.y = YPos()-20.0F*trig.sin;
		simView.z = ZPos();
		if (vistype > 0)
			{
			truckDrawable = new DrawableGroundVehicle(vistype, &simView, Yaw()+PI, drawPointer->GetScale());
#ifdef DEBUG
			gNumGroundTrucks++;
#endif
			}
		}

	// when we wake the object, default it to not labeled unless
	// it's the main guy
	if (drawPointer && gai->rank != GNDAI_BATTALION_COMMANDER)
	{
		drawPointer->SetLabel ("", 0xff00ff00);		// Don't label
		SetLocalFlag(NOT_LABELED);
	}

	return retval;

}

int GroundClass::Sleep (void)
{
int retval = 0;
	if (!IsAwake())
		return retval;

	// NULL any targets our ai might have
	if ( gai )
	{
		gai->SetGroundTarget( NULL );
		gai->SetAirTarget( NULL );
	}

	if ( battalionFireControl )
	{
		VuDeReferenceEntity( battalionFireControl );
		battalionFireControl = NULL;
	}

	// Put away any weapon graphics
	if (Sms) {
		Sms->FreeWeaponGraphics();
#ifdef DEBUG
		CALLS_TO_FREE_WEAPONS++;
#endif
	}

#ifdef DEBUG
	gNumGroundSleep++;
#endif

	SimVehicleClass::Sleep();

	// Dispose of our truck, if we have one.
	if (truckDrawable)
		{
		OTWDriver.RemoveObject(truckDrawable, TRUE);
		truckDrawable = NULL;
#ifdef DEBUG
		gNumGroundTrucks--;
#endif
		}
	return retval;
}

VU_ERRCODE GroundClass::InsertionCallback(void)
{
	return SimMoverClass::InsertionCallback();
}

VU_ERRCODE GroundClass::RemovalCallback(void)
{
	// Put this here, since it will get called for both single and multiplayer cases
	gai->PromoteSubordinates();

	// Error checking - trying to find a bug where we've still got pointers
	// to this.
#ifdef DEBUG
	VuListIterator	vehicleWalker( GetCampaignObject()->GetComponents() );
	GroundClass*	theObj = (GroundClass*)vehicleWalker.GetFirst();
	
	while (theObj)
		{
		ShiAssert( theObj->gai != gai );
		ShiAssert( theObj->gai->battalionCommand != gai );
		ShiAssert( theObj->gai->leader != gai );
		theObj = (GroundClass*)vehicleWalker.GetNext();
		}
#endif

	return SimMoverClass::RemovalCallback();
}

