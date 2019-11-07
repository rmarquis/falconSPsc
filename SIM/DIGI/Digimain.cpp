#include "stdhdr.h"
#include "simobj.h"
#include "digi.h"
#include "simveh.h"
#include "mesg.h"
#include "object.h"
#include "MsgInc\WingmanMsg.h"
#include "MsgInc\ATCMsg.h"
#include "mission.h"
#include "unit.h"
#include "airframe.h"
#include "simdrive.h"
#include "object.h"
#include "classtbl.h"
#include "aircrft.h"
#include "falcsess.h"
#include "camp2sim.h"
#include "Graphics\Include\drawbsp.h"
#include "simfile.h"
#include "entity.h"
#include "atcbrain.h"
#include "Find.h"
#include "tankbrn.h"
#include "navsystem.h"
#include "package.h"
#include "flight.h"
#include "sms.h"

#ifdef USE_SH_POOLS
MEM_POOL	DigitalBrain::pool;
extern MEM_POOL gReadInMemPool;
#endif

#define MANEUVER_DATA_FILE   "sim\\acdata\\brain\\mnvrdata.dat"
DigitalBrain::ManeuverChoiceTable DigitalBrain::maneuverData[DigitalBrain::NumMnvrClasses][DigitalBrain::NumMnvrClasses] = {0, 0, 0, -1, -1, -1};
DigitalBrain::ManeuverClassData DigitalBrain::maneuverClassData[DigitalBrain::NumMnvrClasses] = {0};
FalconEntity* SpikeCheck (AircraftClass* self, FalconEntity *byHim = NULL, int *data = NULL);// 2002-02-10 S.G.

int WingmanTable[] = {1,0,3,2};

#ifdef CHECK_PROC_TIMES
static ulong gPart1 = 0;
static ulong gPart2 = 0;
static ulong gPart3 = 0;
static ulong gPart4 = 0;
static ulong gPart5 = 0;
static ulong gPart6 = 0;
static ulong gPart7 = 0;
static ulong gPart8 = 0;
static ulong gWhole = 0;
extern int gameCompressionRatio;
#endif

int	DigitalBrain::IsMyWingman(SimBaseClass* testEntity)
{
	return self->GetCampaignObject()->GetComponentNumber(WingmanTable[self->vehicleInUnit]) == testEntity;
}

SimBaseClass*	DigitalBrain::MyWingman(void)
{
	return self->GetCampaignObject()->GetComponentNumber(WingmanTable[self->vehicleInUnit]);
}

int	DigitalBrain::IsMyWingman(VU_ID testId)
{
	SimBaseClass *testEntity;
	testEntity = self->GetCampaignObject()->GetComponentNumber(WingmanTable[self->vehicleInUnit]);
	if(testEntity && testEntity->Id() == testId)
		return TRUE;
	return FALSE;
}

DigitalBrain::DigitalBrain (AircraftClass *myPlatform, AirframeClass* myAf) : BaseBrain()
{
	af = myAf;
	self = myPlatform;
	flightLead = NULL;
	targetPtr = NULL;
	targetList = NULL;
	isWing = FALSE; // JPO initialise
	mCurFormation = -1; // JPO
	SetLeader (self);
	onStation = NotThereYet;
	mLastOrderTime = 0;
	wvrCurrTactic = WVR_NONE;
	wvrPrevTactic = WVR_NONE;
	wvrTacticTimer = 0;
	lastAta = 0;
	engagementTimer = 0;

// ADDED BY S.G. SO DIGI KNOWS IT HASN'T LAUNCHED A MISSILE YET (UNUSED BEFORE - NAME IS MEANINGLESS BUT WHAT THE HECK)
	missileFiredEntity = NULL;
	missileFiredTime = 0;
	rwIndex = 0;
	if (self->OnGround())
	    airbase = self->TakeoffAirbase(); // we take off from this base!
	else
	    airbase = self->HomeAirbase(); // original code.
	if(airbase == FalconNullId)
	{
		GridIndex	x,y;
		vector		pos;
		pos.x = self->XPos();
		pos.y = self->YPos();

		//find nearest airbase
		ConvertSimToGrid (&pos, &x, &y);
		Objective obj = FindNearestFriendlyAirbase (self->GetTeam(), x, y);
		//if(obj)
		if(obj && !F4IsBadReadPtr(obj, sizeof(ObjectiveClass))) // JB 010326 CTD
			airbase = obj->Id();
	}
	atcstatus = noATC;
	curTaxiPoint = 0;
	rwtime = 0;
	desiredSpeed = 0.0F;
	turnDist = 0.0F;
	atcFlags = 0;
	waittimer = 0;
	distAirbase = 0.0F;
	updateTime = 0;
	createTime = SimLibElapsedTime;

	if(self->OnGround())
	{
		SetATCFlag(Landed);
		SetATCFlag(RequestTakeoff);
		ClearATCFlag(PermitTakeoff);
		SetATCFlag(StopPlane);
	}
	else
	{
		SetATCFlag(PermitTakeoff);
		ClearATCFlag(Landed);
		ClearATCFlag(RequestTakeoff);
	}

   autoThrottle = 0.0F;
	velocitySlope = 0.0F;

	tankerId = FalconNullId;
	tnkposition = -1;
	refuelstatus = refNoTanker;
	tankerRelPositioning.x = 0.0F;
	tankerRelPositioning.y = 0.0F;
	tankerRelPositioning.z = -3.0F;
	lastBoomCommand = 0;

	Package package;
	Flight flight;
	escortFlightID = FalconNullId; // 2002-02-27 S.G.

	flight = (Flight)self->GetCampaignObject();
	if(flight)
	{
		package = flight->GetUnitPackage();
		if(package)
		{
			tankerId = package->GetTanker();

			// 2002-02-27 ADDED BY S.G. Lets save our escort flight pointer if we have one. It's going to come handy in BvrEngageCheck...
			for (UnitClass *un = package->GetFirstUnitElement(); un; un = package->GetNextUnitElement()) {
			   if (un->IsFlight()) {
				   if (((FlightClass *)un)->GetUnitMission() == AMIS_ESCORT)
					   escortFlightID = un->Id(); // We got one!
			   }
			}
			// END OF ADDED SECTION 2002-02-27
		}
	}


	trackX = self->XPos();
	trackY = self->YPos();
	trackZ = self->ZPos();

	agDoctrine = AGD_NONE;
	agImprovise = FALSE;;
	nextAGTargetTime = SimLibElapsedTime;
	missileShotTimer = SimLibElapsedTime;
	curMissile       = NULL;
	sentWingAGAttack = AG_ORDER_NONE;
	nextAttackCommandToSend = 0; // 2002-01-20 ADED BY S.G. Make sure it's initialized to something we can handle
	jinkTime         = 0;
	jammertime		 = FALSE;//ME123
	holdlongrangeshot = 0.0f;
   cornerSpeed      = af->CornerVcas();
   rangeToIP        = FLT_MAX;
   madeAGPass       = FALSE;
// 2001-05-21 ADDED BY S.G. INIT waitingForShot SO IT'S NOT GARBAGE TO START WITH
   waitingForShot = 0;
// END OF ADDED SECTION
   pastAta = 0;
   pastPipperAta = 0;

	
// 2001-10-12 ADDED BY S.G. INIT gndTargetHistory[2] SO IT'S NOT GARBAGE TO START WITH
   gndTargetHistory[0] = gndTargetHistory[1] = NULL;
// END OF ADDED SECTION
	
	// WingAi inits

	if(self->OnGround())
	{
		mpActionFlags[AI_FOLLOW_FORMATION]		= FALSE;
	}
	else {
		mpActionFlags[AI_FOLLOW_FORMATION]		= TRUE;
	}

	mLeaderTookOff									= FALSE;
	mpActionFlags[AI_ENGAGE_TARGET]			= AI_NONE;
	mpActionFlags[AI_EXECUTE_MANEUVER]		= FALSE;
	mpActionFlags[AI_USE_COMPLEX]          = FALSE;
	mpActionFlags[AI_RTB]					   = FALSE;
	mpActionFlags[AI_LANDING]		         = FALSE;
	
	mpSearchFlags[AI_SEARCH_FOR_TARGET]		= FALSE;
	mpSearchFlags[AI_MONITOR_TARGET]			= FALSE;
	mpSearchFlags[AI_FIXATE_ON_TARGET]		= FALSE;
	
	mCurrentManeuver								= FalconWingmanMsg::WMTotalMsg;
	mDesignatedObject								= FalconNullId;
	mFormation										= acFormationData->FindFormation(FalconWingmanMsg::WMWedge);
	mDesignatedType								= AI_NO_DESIGNATED;
	mSearchDomain									= DOMAIN_ABSTRACT;
	mWeaponsAction									= AI_WEAPONS_HOLD;
	mInPositionFlag								= FALSE;

	mFormRelativeAltitude						= 0.0F;
	mFormSide										= 1;
	mFormLateralSpaceFactor						= 1.0F;
	mSplitFlight									= FALSE;
	
	mLastReportTime								= 0;
	mpLastTargetPtr								= NULL;
	
	mAzErrInt										= 0.0F;
	mLeadGearDown									= -1;
	groundAvoidNeeded = FALSE;
	
	groundTargetPtr = NULL;
	airtargetPtr = NULL;
	preThreatPtr = NULL;
	threatPtr = NULL;
	threatTimer = 0.0f;
	
	pStick = 0.0F;
	rStick = 0.0F;
	throtl = 0.0F; // jpo - from 1 - start at 0
	yPedal = 0.0F;
	
	// Modes and rules
	maxGs = af->MaxGs();
   maxGs = max (maxGs, 2.5F);

	curMode = WaypointMode;
	lastMode = WaypointMode;
	nextMode = NoMode;
	trackMode = 0;
	// Init Target data
	targetPtr = NULL;
	lastTarget = NULL;
	
	// Setup BVR Stuff
	bvrCurrTactic = BvrNoIntercept;
	bvrPrevTactic = BvrNoIntercept;
	bvrTacticTimer = 0;
	spiked =0;
	MAR = 0.0f;
	TGTMAR = 0.0f;
	DOR = 0.0f;
	Isflightlead = false;
	IsElementlead = false;
	bvractionstep = 0;
	bvrCurrProfile = Pnone;
	offsetdir = 0;
	spikeseconds=0;
	spikesecondselement=0;
	spikeseconds = 0;
	missilelasttime = NULL;
	spiketframetime  = NULL;
	lastspikeent = NULL;
	spiketframetimewingie  = NULL;
	lastspikeentwingie = NULL;
	gammaHoldIError = 0;
	reactiont = 0;
	// start out assuming we at least have guns
	maxAAWpnRange = 6000.0f;
	
	// once we've bugged out of WVR our tactics will change
	buggedOut = FALSE;

   // edg missionType and missionClass are now in the digi class since
   // they're used a lot (rather than being locals everywhere)
   missionType = ((UnitClass*)(self->GetCampaignObject()))->GetUnitMission();
   switch (missionType)
   {
	  case AMIS_BARCAP:
	  case AMIS_BARCAP2:
        // 2002-03-05 ADDED BY S.G. These need to attack bombers as well and if OnSweep isn't set, SensorFusion will ignore them
        maxEngageRange = 45.0F * NM_TO_FT;//me123 from 20
	    missionClass = AAMission;
        SetATCFlag(OnSweep);
		break;
		// END OF ADDED SECTION 2002-03-05

	  case AMIS_HAVCAP:
	  case AMIS_TARCAP:
	  case AMIS_RESCAP:
	  case AMIS_AMBUSHCAP:
	  case AMIS_NONE:
        maxEngageRange = 45.0F * NM_TO_FT;//me123 from 20
	     missionClass = AAMission;
        ClearATCFlag(OnSweep);
     break;

	  case AMIS_SWEEP:
        maxEngageRange = 60.0F * NM_TO_FT;//me123 from 80
	     missionClass = AAMission;
        SetATCFlag(OnSweep);
     break;

	  case AMIS_ALERT:
	  case AMIS_INTERCEPT:
	  case AMIS_ESCORT:
        maxEngageRange = 45.0F * NM_TO_FT;//me123 from 30
	     missionClass = AAMission;
        ClearATCFlag(OnSweep);
     break;

     case AMIS_AIRLIFT:
        maxEngageRange = 40.0F * NM_TO_FT;//me123 from 10 bvrengage will now crank beam and drag defensive
        missionClass = AirliftMission;
        ClearATCFlag(OnSweep);
        break;

     case AMIS_TANKER:
     case AMIS_AWACS:
     case AMIS_JSTAR:
     case AMIS_ECM:
     case AMIS_SAR:
        maxEngageRange = 40.0F * NM_TO_FT;//me123 from 10bvrengage will now crank beam and drag defensive
        missionClass = SupportMission;
        ClearATCFlag(OnSweep);
        break;

	  default:
        maxEngageRange = 40.0F * NM_TO_FT;//me123 from 10
	     missionClass = AGMission;
        ClearATCFlag(OnSweep);

        // Engage sooner after mission complete
        
				// JB 010719 missionComplete has not been initialized yet.
				//if (missionComplete)
        //   maxEngageRange *= 1.2F;//me123 from 2.0
        break;
   }
	// 2002-02-27 ADDED BY S.G. What about flights on egress that deaggregates? Look up their Eval flags and set missionComplete accordingly...
    if (((FlightClass *)self->GetCampaignObject())->GetEvalFlags() & FEVAL_GOT_TO_TARGET)
		missionComplete = TRUE;
	else
	// END OF ADDED SECTION 2002-02-27
		missionComplete = FALSE;

   // Check for trainable guns
   if (self->Sms->HasTrainable())
      SetATCFlag (HasTrainable);

   moreFlags = 0; // 2002-03-08 ADDED BY S.G. (Before SelectGroundWeapon which will query it

   // Check for AG weapons
   SelectGroundWeapon();

   // Check for AA Weapons
   SetATCFlag(AceGunsEngage);
   // JPO - for startup actions.
   mActionIndex = 0;
   mNextPreflightAction = 0;
	 lastGndCampTarget = NULL;

	 destRoll = 0;
	 destPitch = 0;
	 currAlt = 0;

	// 2002-01-29 ADDED BY S.G. Init our targetSpot and associated
	targetSpotFlight = NULL;
	targetSpotFlightTarget = NULL;
	targetSpotFlightTimer = 0;
	targetSpotElement = NULL;
	targetSpotElementTarget = NULL;
	targetSpotElementTimer = 0;
	targetSpotWing = NULL;
	targetSpotWingTarget = NULL;
	targetSpotWingTimer = 0;
	// END OF ADDED SECTION
	radarModeTest = 0; // 2002-02-10 ADDED BY S.G.
	// 2002-02-24 MN
	pullupTimer = 0.0f;
	tiebreaker = 0;
	nextFuelCheck = SimLibElapsedTime; // 2002-03-02 MN fix airbase check NOT 0 - set the time here.. aargh...
	airbasediverted = 0;
}

DigitalBrain::~DigitalBrain (void)
{
	SetGroundTarget( NULL );
	SetTarget( NULL );
	if (preThreatPtr)
	{
		preThreatPtr->Release( SIM_OBJ_REF_ARGS );
		preThreatPtr = NULL;
	}
	// 2002-01-29 ADDED BY S.G. Clear our targetSpot and associated if we have created one (see AiSendPlayerCommand)
	if (targetSpotFlight) {
		// Kill the camera and clear out everything associated with this targetSpot
		for (int i = 0; i < FalconLocalSession->CameraCount(); i++) {
			if (FalconLocalSession->Camera(i) == targetSpotFlight) {
				FalconLocalSession->RemoveCamera(targetSpotFlight->Id());
				break;
			}
		}
		vuDatabase->Remove (targetSpotFlight); // Takes care of deleting the allocated memory and the driver allocation as well.
		if (targetSpotFlightTarget) // 2002-03-07 ADDED BY S.G. In case it's NULL. Shouldn't happen but happened
			VuDeReferenceEntity(targetSpotFlightTarget);
		targetSpotFlight = NULL;
		targetSpotFlightTarget = NULL;
		targetSpotFlightTimer = 0;
	}
	if (targetSpotElement) {
		// Kill the camera and clear out everything associated with this targetSpot
		for (int i = 0; i < FalconLocalSession->CameraCount(); i++) {
			if (FalconLocalSession->Camera(i) == targetSpotElement) {
				FalconLocalSession->RemoveCamera(targetSpotElement->Id());
				break;
			}
		}
		vuDatabase->Remove (targetSpotElement); // Takes care of deleting the allocated memory and the driver allocation as well.
		if (targetSpotElementTarget) // 2002-03-07 ADDED BY S.G. In case it's NULL. Shouldn't happen but happened
			VuDeReferenceEntity(targetSpotElementTarget);
		targetSpotElement = NULL;
		targetSpotElementTarget = NULL;
		targetSpotElementTimer = 0;
	}
	if (targetSpotWing) {
		// Kill the camera and clear out everything associated with this targetSpot
		for (int i = 0; i < FalconLocalSession->CameraCount(); i++) {
			if (FalconLocalSession->Camera(i) == targetSpotWing) {
				FalconLocalSession->RemoveCamera(targetSpotWing->Id());
				break;
			}
		}
		vuDatabase->Remove (targetSpotWing); // Takes care of deleting the allocated memory and the driver allocation as well.
		if (targetSpotWingTarget) // 2002-03-07 ADDED BY S.G. In case it's NULL. Shouldn't happen but happened
			VuDeReferenceEntity(targetSpotWingTarget);
		targetSpotWing = NULL;
		targetSpotWingTarget = NULL;
		targetSpotWingTimer = 0;
	}
	// END OF ADDED SECTION 2002-01-29

	// 2002-03-13 ADDED BY S.G. Must dereference it or it will cause memory leak...
	if (missileFiredEntity)
		VuDeReferenceEntity(missileFiredEntity);
	missileFiredEntity = NULL;
}

void DigitalBrain::FrameExec(SimObjectType* curTargetList, SimObjectType* curTarget)
{
#ifdef CHECK_PROC_TIMES
	gPart1 = 0;
	gPart2 = 0;
	gPart3 = 0;
	gPart4 = 0;
	gPart5 = 0;
	gPart6 = 0;
	gPart7 = 0;
	gPart8 = 0;
	gWhole = GetTickCount();
#endif
   // Modify max gs based on skill level
   // Rookies (level 0) has .5 max gs, top of the line has max gs
   // Keep max gs >= 2.5
   //maxGs = af->MaxGs()  * (SkillLevel() / 4.0F * 0.5F + 0.5F);
   //me123 they might be stupid , but hey give em the g's at least.
	maxGs = af->MaxGs();  //* (SkillLevel() / 4.0F * 0.5F + 0.8F);
   maxGs = max (maxGs, 2.5F);


#ifdef DAVE_DBG
   // Update the label in debug
   //char tmpStr[40];
/* ECTS HACK
   
   if (targetPtr && targetPtr->BaseData() == SpikeCheck(self))
      missionType = -missionType;
   sprintf (tmpStr, "%s %d-%d", ((VehicleClassDataType*)((Falcon4EntityClassType*)self->EntityType())->dataPtr)->Name,
      missionType, (self->curWaypoint ? self->curWaypoint->GetWPAction() : 0));
   if (targetPtr && targetPtr->BaseData()->IsSim())
      sprintf (tmpStr, "%s %s", tmpStr, 
      ((VehicleClassDataType*)((Falcon4EntityClassType*)targetPtr->BaseData()->EntityType())->dataPtr)->Name);

   if(atcstatus == noATC)
		sprintf (tmpStr,"noATC");
   else
   {
	   int delta = waittimer - Camp_GetCurrentTime();
	   int rwdelta = rwtime - Camp_GetCurrentTime();

	   sprintf (tmpStr,"%d  %d  %4.2f  %4.2f", self->share_.id_.num_, atcstatus, delta/1000.0F, rwdelta/1000.0F);
   }
   if ( self->drawPointer )
   	  ((DrawableBSP*)self->drawPointer)->SetLabel (tmpStr, ((DrawableBSP*)self->drawPointer)->LabelColor());
	  */
#endif
   
/*   // Turn on/off external lights according to 
	if (self->IsF16() && atcstatus == noATC)
		self->ExtlOff(self->Extl_Main_Power);
	else
		self->ExtlOn(self->Extl_Main_Power);
*/

// 2002-03-15 MN if we've flamed out and our speed is below minvcas, put the wheels out so that there is a chance to land
   if (self->af->Fuel() <= 0.0F && self->af->vcas < self->af->MinVcas())
   {
		// Set Landed flag now, so that RegroupAircraft can be called in Eom.cpp - have the maintenance crew tow us back to the airbase ;-)
		SetATCFlag(Landed);
		af->gearHandle = 1.0F;
   }
   else
   // make sure the wheels are up after takeoff
   if (self->curWaypoint && self->curWaypoint->GetWPAction() != WP_TAKEOFF)
		af->gearHandle = -1.0F;

   // assume we're not going to be firing in this frame
   ClearFlag (MslFireFlag | GunFireFlag);

   // check to see if our leader is dead and if not set leader to next in
   // flight (and/or ourself)
   CheckLead();

#ifdef CHECK_PROC_TIMES
	gPart1 = GetTickCount();
#endif
   // Find a threat/target
   DoTargeting();
#ifdef CHECK_PROC_TIMES
	gPart1 = GetTickCount() - gPart1;
	gPart2 = GetTickCount();
#endif
   // Make Decisions
   SetCurrentTactic();
#ifdef CHECK_PROC_TIMES
	gPart2 = GetTickCount() - gPart2;
	gPart3 = GetTickCount();
#endif
   // Set the controls 
   Actions();
#ifdef CHECK_PROC_TIMES
	gPart3 = GetTickCount() - gPart3;
	gPart4 = GetTickCount();
#endif
   // has the target changed?
   if (targetPtr != lastTarget)
   {
      lastTarget = targetPtr;
      ataddot = 10.0F;
      rangeddot = 10.0F;
   }

   // edg: check stick settings for bad values
   if ( rStick < -1.0f )
   		rStick = -1.0f;
   else if ( rStick > 1.0f )
   		rStick = 1.0f;

   if ( pStick < -1.0f )
   		pStick = -1.0f;
   else if ( pStick > 1.0f )
   		pStick = 1.0f;
   //me123 unload if roll input is 1 (to rool faster and eleveate the f4 bug)
   if (fabs(rStick) >0.9f && groundAvoidNeeded == FALSE)
	   pStick = 0.0f;

   if ( throtl < 0.0f )
   		throtl = 0.0f;
   else if ( throtl > 1.5f )
   		throtl = 1.5f;


#ifdef CHECK_PROC_TIMES
	gPart4 = GetTickCount() - gPart4;
	gWhole = GetTickCount() - gWhole;

	if(gameCompressionRatio && self->IsPlayer())
	{
		MonoPrint("Whole:  %3d  Targ: %3d  Tac: %3d Action: %3d  Rest: %3d\n", gWhole, gPart1, gPart2, gPart3, gPart4);
	}
#endif
}
void DigitalBrain::Sleep(void)
{
   SetLeader (NULL);
   ClearTarget();

   // NOTE: This is only legal if the platorms target list is already cleared.
   // Currently, SimVehicle::Sleep call SimMover::Sleep which clears the list,
   // then it calls theBrain::Sleep.  As long as this doesn't change this will
   // not cause a leak.
   if (groundTargetPtr)
   {
#ifdef DEBUG
      if (groundTargetPtr->prev || groundTargetPtr->next)
      {
         MonoPrint ("Ground target still in list at sleep\n");
      }
#endif
      groundTargetPtr->prev = NULL;
      groundTargetPtr->next = NULL;
   }

   SetGroundTarget( NULL );
}

void DigitalBrain::SetLead (int flag)
{
	if (flag == TRUE)
	{
		isWing = FALSE;
		SetLeader (self);
	}
	else
	{
		isWing = self->GetCampaignObject()->GetComponentIndex(self);
		SetLeader (self->GetCampaignObject()->GetComponentLead());
	}
}

// Make sure our leader hasn't gone away without telling us.
void DigitalBrain::CheckLead(void)
{
	SimBaseClass *pobj;
	SimBaseClass* newLead = NULL;
	
	BOOL	done = FALSE;
	int	i = 0;

	if ( flightLead &&
		 flightLead->VuState() == VU_MEM_ACTIVE &&
		 !flightLead->IsDead() )
		 return;
	
	VuListIterator	cit(self->GetCampaignObject()->GetComponents());
	pobj = (SimBaseClass*)cit.GetFirst();
	while(!done)
	{	
		if(pobj && 
		   pobj->VuState() == VU_MEM_ACTIVE &&
		   !pobj->IsDead() )
		{			
			done = TRUE;
			newLead = pobj;
		}
		else if(i > 3)
		{
			done = TRUE;
			newLead = self;
		}

		i ++;
		pobj = (SimBaseClass*)cit.GetNext();
	}
	assert (newLead);
	SetLeader (newLead);
}

void DigitalBrain::SetLeader (SimBaseClass* newLead)
{
	// edg: I've encountered some over-referencing problems that I think
	// is associated with flight lead setting.  So what I put in was a
	// check for self and not doing the reference -- shouldn;t be needed in
	// this case, right?
	if (flightLead != newLead)
	{
		if (flightLead && flightLead != self )
			VuDeReferenceEntity (flightLead);
		
		flightLead = newLead;
		
		if (flightLead && flightLead != self )
			VuReferenceEntity (flightLead);

		// edg: what a confusing mess... Why do we have SetLead and
		// SetLeader?!
		// anyway, overreferencing has been occurring on Sleep(), when
		// SetLeader( NULL ) is called.  We then called SetLead() below,
		// which resulted in a new flight lead!!
		// check for NULL flight lead and just return
		if ( flightLead != NULL )
        	SetLead (flightLead == self ? TRUE : FALSE);
	}
}

void DigitalBrain::JoinFlight (void)
{
	SimBaseClass* newLead = self->GetCampaignObject()->GetComponentLead();
	
	if (newLead == self)
	{
		SetLead (TRUE);
	}
	else
	{
		SetLead (FALSE);
	}
}

void DigitalBrain::ReadManeuverData (void)
{

SimlibFileClass* mnvrFile;
int i, j, k;
char fileType;

   /*---------------------*/
   /* open formation file */
   /*---------------------*/
   mnvrFile = SimlibFileClass::Open(MANEUVER_DATA_FILE, SIMLIB_READ);
   F4Assert (mnvrFile);
   mnvrFile->Read(&fileType, 1);

   if (fileType == 'B') // Binary
   {
      for (i=0; i<NumMnvrClasses; i++)
      {
         for (j=0; j<NumMnvrClasses; j++)
         {
         }
      }
   }
   else if (fileType == 'A') // Ascii
   {
      for (i=0; i<NumMnvrClasses; i++)
      {
         // Get the limits for this Maneuver type
         sscanf (mnvrFile->GetNext(), "%x", &maneuverClassData[i]);

         for (j=0; j<NumMnvrClasses; j++)
         {
				maneuverData[i][j].numIntercepts = (char)atoi (mnvrFile->GetNext());
            if (maneuverData[i][j].numIntercepts)
				{
					maneuverData[i][j].intercept     =
						#ifdef USE_SH_POOLS
						(BVRInterceptType *)MemAllocPtr(gReadInMemPool, sizeof(BVRInterceptType)*maneuverData[i][j].numIntercepts,0);
						#else
						new BVRInterceptType[maneuverData[i][j].numIntercepts];
						#endif
				}
				else
					maneuverData[i][j].intercept     = NULL;

				maneuverData[i][j].numMerges     = (char)atoi (mnvrFile->GetNext());
				if (maneuverData[i][j].numMerges)
				{
					maneuverData[i][j].merge         =
						#ifdef USE_SH_POOLS
						(WVRMergeManeuverType *)MemAllocPtr(gReadInMemPool, sizeof(WVRMergeManeuverType)*maneuverData[i][j].numMerges,0);
						#else
               			new WVRMergeManeuverType[maneuverData[i][j].numMerges];
						#endif
				}
				else
					maneuverData[i][j].merge         = NULL;

				maneuverData[i][j].numReacts     = (char)atoi (mnvrFile->GetNext());
				if (maneuverData[i][j].numReacts)
				{
					maneuverData[i][j].spikeReact    =
						#ifdef USE_SH_POOLS
						(SpikeReactionType *)MemAllocPtr(gReadInMemPool, sizeof(SpikeReactionType)*maneuverData[i][j].numReacts,0);
						#else
               			new SpikeReactionType[maneuverData[i][j].numReacts];
						#endif
				}
				else
					maneuverData[i][j].spikeReact    = NULL;

            for (k=0; k<maneuverData[i][j].numIntercepts; k++)
            {
               maneuverData[i][j].intercept[k] =
                  (BVRInterceptType)(atoi (mnvrFile->GetNext()) - 1);
            }

            for (k=0; k<maneuverData[i][j].numMerges; k++)
            {
               maneuverData[i][j].merge[k] =
                  (WVRMergeManeuverType)(atoi (mnvrFile->GetNext()) - 1);
            }

            for (k=0; k<maneuverData[i][j].numReacts; k++)
            {
               maneuverData[i][j].spikeReact[k] =
                  (SpikeReactionType)(atoi (mnvrFile->GetNext()) - 1);
            }
         }
      }
   }
   else
   {
      ShiWarning ("Bad Maneuver Data File Format");
   }
   mnvrFile->Close (); // JPO Handle/memory leak fix
   delete mnvrFile; 
   mnvrFile = NULL;
}

void DigitalBrain::FreeManeuverData (void)
{
int i, j;

   for (i=0; i<DigitalBrain::NumMnvrClasses; i++)
   {
      for (j=0; j<DigitalBrain::NumMnvrClasses; j++)
      {
         delete maneuverData[i][j].intercept;
         delete maneuverData[i][j].merge;
         delete maneuverData[i][j].spikeReact;
         maneuverData[i][j].intercept  = NULL;
         maneuverData[i][j].merge      = NULL;
         maneuverData[i][j].spikeReact = NULL;
      }
   }

}

void DigitalBrain::GetTrackPoint(float *x, float *y, float *z)
{
	*x = trackX;
	*y = trackY;
	*z = trackZ;
}

void DigitalBrain::SetTrackPoint(float x, float y, float z)
{
	trackX = x;
	trackY = y;
	trackZ = z;
}

void DigitalBrain::SetRunwayInfo(VU_ID Airbase, int rwindex, unsigned long time)
{
	airbase	= Airbase;
	rwIndex	= rwindex;
	rwtime	= time;


	if(self == SimDriver.playerEntity) { // vwf
		gNavigationSys->SetIlsData(airbase, rwIndex);
	}
}

#define DEBUGLABEL
#ifdef DEBUGLABEL
void DigitalBrain::ReSetLabel (SimBaseClass* theObject)
{
	Falcon4EntityClassType	*classPtr = (Falcon4EntityClassType*)theObject->EntityType();
	CampEntity				campObj;
	char					label[40] = {0};
	long					labelColor = 0xff0000ff;

	if (!theObject->IsExploding())
	{
		if (classPtr->dataType == DTYPE_VEHICLE)
		{
			FlightClass		*flight;
			flight = FalconLocalSession->GetPlayerFlight();
			campObj = theObject->GetCampaignObject();
			if (campObj && campObj->IsFlight() && !campObj->IsAggregate() /*&& campObj->InPackage()*/
				// 2001-10-31 M.N. show flight names of our team
				&& flight && flight->GetTeam() == campObj->GetTeam())
			{
				char		temp[40];
				GetCallsign(((Flight)campObj)->callsign_id,((Flight)campObj)->callsign_num,temp);
				sprintf (label, "%s%d",temp,((SimVehicleClass*)theObject)->vehicleInUnit+1);
			}
			else
				sprintf (label, "%s", ((VehicleClassDataType*)(classPtr->dataPtr))->Name);
		}
		else if (classPtr->dataType == DTYPE_WEAPON)
			sprintf (label, "%s", ((WeaponClassDataType*)(classPtr->dataPtr))->Name);
	}

	// Change the label to a player, if there is one
	if (theObject->IsSetFalcFlag(FEC_HASPLAYERS))
	{
		// Find the player's callsign
		VuSessionsIterator		sessionWalker(FalconLocalGame);
		FalconSessionEntity		*session;

		session = (FalconSessionEntity*)sessionWalker.GetFirst();
		while (session)
		{
			if (session->GetPlayerEntity() == theObject)
				sprintf(label, "%s", session->GetPlayerCallsign());
			session = (FalconSessionEntity*)sessionWalker.GetNext();
		}
	}

	// Change the label color by team color
	//ShiAssert(TeamInfo[theObject->GetTeam()]);
	
	
	// KCK: This uses the UI's colors. For a while these didn't work well in Sim
	// They may be ok now, though - KCK: As of 10/25, still looked bad
//	labelColor = TeamColorList[TeamInfo[theObject->GetTeam()]->GetColor()];

	if (theObject->drawPointer)
		theObject->drawPointer->SetLabel(label, ((DrawableBSP*)theObject->drawPointer)->LabelColor());
	
}
#endif