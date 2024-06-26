#include "stdhdr.h"
#include "simbase.h"
#include "simdrive.h"
#include "find.h"
#include "flight.h"
#include "falcsess.h"
#include "simmover.h"
#include "wingorder.h"
#include "object.h"
#include "find.h"
#include "radar.h"
#include "classtbl.h"
#include "team.h"
#include "PlayerOp.h"

#define IGNORE_SENDER 0

char* gpAiExtentStr[AiTotalExtent] = {
	"AiWingman",
	"AiElement",
	"AiFlight",
	"AiPackage",
	"AiLeader",
	"AiAllButSender",
	"AiNoExtent"
};

VU_ID FindAircraftTarget (AircraftClass* theVehicle);

// 2002-01-29 ADDED BY S.G. Taken from the GroundSpot* in SmsDraw.cpp to create a spot to keep deaggregated
extern int F4FlyingEyeType;
extern int g_nTargetSpotTimeout;
#include "digi.h"

class TargetSpotDriver : public VuMaster
{
	VU_TIME		last_time;
	SM_SCALAR	lx;
	SM_SCALAR	ly;
	SM_SCALAR	lz;

public:
	
	TargetSpotDriver (VuEntity *entity);
	virtual void Exec (VU_TIME timestamp);
	virtual VU_BOOL ExecModel(VU_TIME timestamp);
};

class TargetSpotEntity : public FalconEntity
{
public:
	TargetSpotEntity (int type);
	TargetSpotEntity (VU_BYTE **);
	virtual int Sleep (void) {return 0;};
	virtual int Wake (void) {return 0;};
};

TargetSpotEntity::TargetSpotEntity (int type) : FalconEntity(type)
{
    SetYPRDelta(0,0, 0);
}

TargetSpotEntity::TargetSpotEntity (VU_BYTE ** data) : FalconEntity(data)
{
    SetYPRDelta(0,0, 0); // JPO - set to something.
}

TargetSpotDriver::TargetSpotDriver (VuEntity *entity) : VuMaster(entity)
{
	last_time = 0;
	lx = ly = lz = 0;
}

VU_BOOL TargetSpotDriver::ExecModel(VU_TIME)
{
	return TRUE;
}

void TargetSpotDriver::Exec (VU_TIME time)
{
	float
		dx,
		dy,
		dz;

	if (time - last_time > 2000)
	{
		last_time = time;

		dx = vuabs (lx - entity_->XPos ());
		dy = vuabs (ly - entity_->YPos ());
		dz = vuabs (lz - entity_->ZPos ());

		if (dx + dy + dz < 4000)
		{
			if (dx + dy + dz < 1.0)
			{
				entity_->SetDelta (0.0, 0.0, 0.0);
				
//				MonoPrint ("TargetSpot %f,%f,%f Stationary\n",
//					entity_->XPos (),
//					entity_->YPos (),
//					entity_->ZPos ());
			}
			else
			{
				entity_->SetDelta (0.001F, 0.001F, 0.001F);
				
//				MonoPrint ("TargetSpot %f,%f,%f Moving\n",
//					entity_->XPos (),
//					entity_->YPos (),
//					entity_->ZPos ());
			}
			
			VuMaster::Exec (time);
		}
		else
		{
//			MonoPrint ("TargetSpot Moving Fast %f,%f,%f %f\n", dx, dy, dz, dx + dy + dz);
		}

		lx = entity_->XPos ();
		ly = entity_->YPos ();
		lz = entity_->ZPos ();
	}
}

// END OF ADDED SECTION 2002-01-29

// --------------------------------------------------------------------
//
// AiSendPlayerCommand
//
// --------------------------------------------------------------------

// All keyboard Ai commands get funneled thru here
void AiSendPlayerCommand (int command, int extent, VU_ID targetId) 
{
   // 2002-01-29 ADDED BY S.G. Need them later on
   DigitalBrain *myBrain;
   DigitalBrain *wingBrain = NULL;
   FalconEntity *wingTgt = NULL;
   AircraftClass *wingPlane;
   int i;

   switch (command)
   {
      case FalconWingmanMsg::WMSpread:
      case FalconWingmanMsg::WMWedge:
      case FalconWingmanMsg::WMTrail:
      case FalconWingmanMsg::WMLadder:
      case FalconWingmanMsg::WMStack:
      case FalconWingmanMsg::WMResCell:
      case FalconWingmanMsg::WMBox:
      case FalconWingmanMsg::WMArrowHead:
      case FalconWingmanMsg::WMFluidFour:
	  case FalconWingmanMsg::WMVic:
	  case FalconWingmanMsg::WMFinger4:
	  case FalconWingmanMsg::WMEchelon:
	  case FalconWingmanMsg::WMForm1:
	  case FalconWingmanMsg::WMForm2:
	  case FalconWingmanMsg::WMForm3:
	  case FalconWingmanMsg::WMForm4:
	  break;

      case FalconWingmanMsg::WMPince:
      case FalconWingmanMsg::WMSSOffset:
      case FalconWingmanMsg::WMPosthole:
      case FalconWingmanMsg::WMChainsaw:
      case FalconWingmanMsg::WMAssignTarget:
      case FalconWingmanMsg::WMAssignGroup:
			if(targetId == FalconNullId) {
				targetId = AiDesignateTarget(SimDriver.playerEntity);
			}
			if(targetId == FalconNullId) {
				return;
			}
			// 2002-01-29 ADDED BY S.G. Now that we have a target, lets attach a camera to it so it stays deaggregated for a while...
			// We keep three targetSpot, one per command since we can direct our wing on something then or element on something else
			// AircraftClass::Exec do the actual update of the targetSpots

			// If we have no timer count, act as if the player wants nothing to do with our little bubbles	
			if (!g_nTargetSpotTimeout)
				break;

			// For short hand, get a pointer to my brain
			myBrain = (DigitalBrain *)(SimDriver.playerEntity->Brain());

			switch (extent) {
				case AiWingman:
					// First lets create one if not done already
					if (!myBrain->targetSpotWing) {
						myBrain->targetSpotWing = new TargetSpotEntity (F4FlyingEyeType+VU_LAST_ENTITY_TYPE);
						vuDatabase->QuickInsert(myBrain->targetSpotWing);
						myBrain->targetSpotWing->SetDriver (new TargetSpotDriver (myBrain->targetSpotWing));
					}
					else {
						// Kill the current camera
						for (i = 0; i < FalconLocalSession->CameraCount(); i++) {
							if (FalconLocalSession->Camera(i) == myBrain->targetSpotWing) {
								FalconLocalSession->RemoveCamera(myBrain->targetSpotWing->Id());
								break;
							}
						}
					}

					// Now init for our new target
					myBrain->targetSpotWingTarget = (FalconEntity*) vuDatabase->Find(targetId);
					myBrain->targetSpotWingTimer = SimLibElapsedTime + g_nTargetSpotTimeout;
					VuReferenceEntity(myBrain->targetSpotWingTarget);

					// Attach a camera to our target spot
					for (i = 0; i < FalconLocalSession->CameraCount(); i++)
						if (FalconLocalSession->Camera(i) == myBrain->targetSpotWing)
							break;
					if (i == FalconLocalSession->CameraCount())
						FalconLocalSession->AttachCamera(myBrain->targetSpotWing->Id());
					break;

				case AiElement:
					// First lets create one if not done already
					if (!myBrain->targetSpotElement) {
						myBrain->targetSpotElement = new TargetSpotEntity (F4FlyingEyeType+VU_LAST_ENTITY_TYPE);
						vuDatabase->QuickInsert(myBrain->targetSpotElement);
						myBrain->targetSpotElement->SetDriver (new TargetSpotDriver (myBrain->targetSpotElement));
					}
					else {
						// Kill the current camera
						for (i = 0; i < FalconLocalSession->CameraCount(); i++) {
							if (FalconLocalSession->Camera(i) == myBrain->targetSpotElement) {
								FalconLocalSession->RemoveCamera(myBrain->targetSpotElement->Id());
								break;
							}
						}
					}

					// Now init for our new target
					myBrain->targetSpotElementTarget = (FalconEntity*) vuDatabase->Find(targetId);
					myBrain->targetSpotElementTimer = SimLibElapsedTime + g_nTargetSpotTimeout;
					VuReferenceEntity(myBrain->targetSpotElementTarget);

					// Attach a camera to our target spot
					for (i = 0; i < FalconLocalSession->CameraCount(); i++)
						if (FalconLocalSession->Camera(i) == myBrain->targetSpotElement)
							break;
					if (i == FalconLocalSession->CameraCount())
						FalconLocalSession->AttachCamera(myBrain->targetSpotElement->Id());
					break;

				case AiFlight:
					// First lets create one if not done already
					if (!myBrain->targetSpotFlight) {
						myBrain->targetSpotFlight = new TargetSpotEntity (F4FlyingEyeType+VU_LAST_ENTITY_TYPE);
						vuDatabase->QuickInsert(myBrain->targetSpotFlight);
						myBrain->targetSpotFlight->SetDriver (new TargetSpotDriver (myBrain->targetSpotFlight));
					}
					else {
						// Kill the current camera
						for (i = 0; i < FalconLocalSession->CameraCount(); i++) {
							if (FalconLocalSession->Camera(i) == myBrain->targetSpotFlight) {
								FalconLocalSession->RemoveCamera(myBrain->targetSpotFlight->Id());
								break;
							}
						}
					}

					// Now init for our new target
					myBrain->targetSpotFlightTarget = (FalconEntity*) vuDatabase->Find(targetId);
					myBrain->targetSpotFlightTimer = SimLibElapsedTime + g_nTargetSpotTimeout;
					VuReferenceEntity(myBrain->targetSpotFlightTarget);

					// Attach a camera to our target spot
					for (i = 0; i < FalconLocalSession->CameraCount(); i++)
						if (FalconLocalSession->Camera(i) == myBrain->targetSpotFlight)
							break;
					if (i == FalconLocalSession->CameraCount())
						FalconLocalSession->AttachCamera(myBrain->targetSpotFlight->Id());

					// A flight command overides other so kill their myBrain->targetSpots and associated
					if (myBrain->targetSpotElement) {
						// Kill the camera and clear out everything associated with this myBrain->targetSpot
						for (i = 0; i < FalconLocalSession->CameraCount(); i++) {
							if (FalconLocalSession->Camera(i) == myBrain->targetSpotElement) {
								FalconLocalSession->RemoveCamera(myBrain->targetSpotElement->Id());
								break;
							}
						}
						vuDatabase->Remove (myBrain->targetSpotElement); // Takes care of deleting the allocated memory and the driver allocation as well.
						if (myBrain->targetSpotElementTarget) // 2002-03-07 ADDED BY S.G. In case it's NULL. Shouldn't happen but happened
							VuDeReferenceEntity(myBrain->targetSpotElementTarget);
						myBrain->targetSpotElement = NULL;
						myBrain->targetSpotElementTarget = NULL;
						myBrain->targetSpotElementTimer = 0;
					}
					if (myBrain->targetSpotWing) {
						// Kill the camera and clear out everything associated with this myBrain->targetSpot
						for (i = 0; i < FalconLocalSession->CameraCount(); i++) {
							if (FalconLocalSession->Camera(i) == myBrain->targetSpotWing) {
								FalconLocalSession->RemoveCamera(myBrain->targetSpotWing->Id());
								break;
							}
						}
						vuDatabase->Remove (myBrain->targetSpotWing); // Takes care of deleting the allocated memory and the driver allocation as well.
						if (myBrain->targetSpotWingTarget) // 2002-03-07 ADDED BY S.G. In case it's NULL. Shouldn't happen but happened
							VuDeReferenceEntity(myBrain->targetSpotWingTarget);
						myBrain->targetSpotWing = NULL;
						myBrain->targetSpotWingTarget = NULL;
						myBrain->targetSpotWingTimer = 0;
					}
			}

      break;

      case FalconWingmanMsg::WMClearSix:
			targetId	= AiCheckForThreat(SimDriver.playerEntity, DOMAIN_AIR, 1);
		break;

		
		case FalconWingmanMsg::WMCheckSix:
		case FalconWingmanMsg::WMShooterMode:
      case FalconWingmanMsg::WMCoverMode:
      case FalconWingmanMsg::WMWeaponsHold:
		  break;
	  // 2002-02-08 ADDED BY S.G. "Weapon Free" creates target bubble as well
      case FalconWingmanMsg::WMWeaponsFree:
		  // If we have no timer count, act as if the player wants nothing to do with our little bubbles	
		  if (!g_nTargetSpotTimeout)
			  break;

		  // For short hand, get a pointer to my brain
		  myBrain = (DigitalBrain *)(SimDriver.playerEntity->Brain());

		  // First we will attach a bubble on the target of who we are controlling
		  switch (extent) {
			  case AiWingman:
				  // If we already have a camera, don't break it by a weapon free command
				  if (myBrain->targetSpotWing)
					  break;

				  // Get the brain of the corresponding wing AI, depending if we're the flight lead or element lead
				  if (myBrain->isWing == AiFlightLead)
					  wingPlane = ((AircraftClass *)SimDriver.playerEntity->GetCampaignObject()->GetComponentNumber(1));
				  else
					  wingPlane = ((AircraftClass *)SimDriver.playerEntity->GetCampaignObject()->GetComponentNumber(3));

				  // Our wing is not brain dead ;-)
				  if (wingPlane && !wingPlane->IsDead())
					  wingBrain = wingPlane->DBrain();

				  // If our wing has a brain and a that brain has a target (priority to ground target)
				  if (wingBrain && wingBrain->GetGroundTarget())
					  wingTgt = wingBrain->GetGroundTarget()->BaseData();
				  else if (wingBrain && wingBrain->targetPtr)
					  wingTgt = wingBrain->targetPtr->BaseData();

				  // After all this, if we have a target, assign a 'bubble' around it
				  if (wingTgt) {
					// First lets create one if not done already
					if (!myBrain->targetSpotWing) {
						myBrain->targetSpotWing = new TargetSpotEntity (F4FlyingEyeType+VU_LAST_ENTITY_TYPE);
						vuDatabase->QuickInsert(myBrain->targetSpotWing);
						myBrain->targetSpotWing->SetDriver (new TargetSpotDriver (myBrain->targetSpotWing));
					}
					else {
						// Kill the current camera
						for (i = 1; i < FalconLocalSession->CameraCount(); i++) {
							if (FalconLocalSession->Camera(i) == myBrain->targetSpotWing) {
								FalconLocalSession->RemoveCamera(myBrain->targetSpotWing->Id());
								break;
							}
						}
					}

					// Now init for our new target
					myBrain->targetSpotWingTarget = (FalconEntity*) wingTgt;
					myBrain->targetSpotWingTimer = SimLibElapsedTime + g_nTargetSpotTimeout;
					VuReferenceEntity(myBrain->targetSpotWingTarget);

					// Attach a camera to our target spot
					for (i = 0; i < FalconLocalSession->CameraCount(); i++)
						if (FalconLocalSession->Camera(i) == myBrain->targetSpotWing)
							break;
					if (i == FalconLocalSession->CameraCount())
						FalconLocalSession->AttachCamera(myBrain->targetSpotWing->Id());
				  }

				  break;
			  case AiElement:
				  // If we already have a camera, don't break it by a weapon free command
				  if (myBrain->targetSpotElement)
					  break;

				  // I'm giving a weapon free to my element. Since I have only one bubble for the element,
				  // choose the element lead target if avail otherwise choose his wing's target
				  wingPlane = ((AircraftClass *)SimDriver.playerEntity->GetCampaignObject()->GetComponentNumber(2));
				  if (wingPlane && !wingPlane->IsDead())
					  wingBrain = wingPlane->DBrain();

				  // If our wing has a brain and a that brain has a target (priority to ground target)
				  if (wingBrain && wingBrain->GetGroundTarget())
					  wingTgt = wingBrain->GetGroundTarget()->BaseData();
				  else if (wingBrain && wingBrain->targetPtr)
					  wingTgt = wingBrain->targetPtr->BaseData();

				  // If the element lead has none, check his wing
				  if (!wingTgt) {
					  wingPlane = ((AircraftClass *)SimDriver.playerEntity->GetCampaignObject()->GetComponentNumber(3));
					  if (wingPlane && !wingPlane->IsDead())
						  wingBrain = wingPlane->DBrain();

					  // If our wing has a brain and a that brain has a target (priority to ground target)
					  if (wingBrain && wingBrain->GetGroundTarget())
						  wingTgt = wingBrain->GetGroundTarget()->BaseData();
					  else if (wingBrain && wingBrain->targetPtr)
						  wingTgt = wingBrain->targetPtr->BaseData();
				  }

				  // After all this, if we have a target, assign a 'bubble' around it
				  if (wingTgt) {
					// First lets create one if not done already
					if (!myBrain->targetSpotElement) {
						myBrain->targetSpotElement = new TargetSpotEntity (F4FlyingEyeType+VU_LAST_ENTITY_TYPE);
						vuDatabase->QuickInsert(myBrain->targetSpotElement);
						myBrain->targetSpotElement->SetDriver (new TargetSpotDriver (myBrain->targetSpotElement));
					}
					else {
						// Kill the current camera
						for (i = 0; i < FalconLocalSession->CameraCount(); i++) {
							if (FalconLocalSession->Camera(i) == myBrain->targetSpotElement) {
								FalconLocalSession->RemoveCamera(myBrain->targetSpotElement->Id());
								break;
							}
						}
					}

					// Now init for our new target
					myBrain->targetSpotElementTarget = (FalconEntity*) wingTgt;;
					myBrain->targetSpotElementTimer = SimLibElapsedTime + g_nTargetSpotTimeout;
					VuReferenceEntity(myBrain->targetSpotElementTarget);

					// Attach a camera to our target spot
					for (i = 0; i < FalconLocalSession->CameraCount(); i++)
						if (FalconLocalSession->Camera(i) == myBrain->targetSpotElement)
							break;
					if (i == FalconLocalSession->CameraCount())
						FalconLocalSession->AttachCamera(myBrain->targetSpotElement->Id());
				  }

				  break;
			  case AiFlight:
				  // If we already have a camera, don't break it by a weapon free command
				  if (myBrain->targetSpotFlight)
					  break;

				  // The whole flight, check all of the wing's target but only create one bubble, around the first target we find
				  for (i = 0; !wingTgt && i < SimDriver.playerEntity->GetCampaignObject()->NumberOfComponents(); i++) {
					  wingPlane = ((AircraftClass *)SimDriver.playerEntity->GetCampaignObject()->GetComponentNumber(i));
					  if (wingPlane && !wingPlane->IsDead())
						  wingBrain = wingPlane->DBrain();

					  // If our wing has a brain and a that brain has a target (priority to ground target)
					  if (wingBrain && wingBrain->GetGroundTarget())
						  wingTgt = wingBrain->GetGroundTarget()->BaseData();
					  else if (wingBrain && wingBrain->targetPtr)
						  wingTgt = wingBrain->targetPtr->BaseData();
				  }

				  if (wingTgt) {
					// First lets create one if not done already
					if (!myBrain->targetSpotFlight) {
						myBrain->targetSpotFlight = new TargetSpotEntity (F4FlyingEyeType+VU_LAST_ENTITY_TYPE);
						vuDatabase->QuickInsert(myBrain->targetSpotFlight);
						myBrain->targetSpotFlight->SetDriver (new TargetSpotDriver (myBrain->targetSpotFlight));
					}
					else {
						// Kill the current camera
						for (i = 0; i < FalconLocalSession->CameraCount(); i++) {
							if (FalconLocalSession->Camera(i) == myBrain->targetSpotFlight) {
								FalconLocalSession->RemoveCamera(myBrain->targetSpotFlight->Id());
								break;
							}
						}
					}

					// Now init for our new target
					myBrain->targetSpotFlightTarget = (FalconEntity*) wingTgt;
					myBrain->targetSpotFlightTimer = SimLibElapsedTime + g_nTargetSpotTimeout;
					VuReferenceEntity(myBrain->targetSpotFlightTarget);

					// Attach a camera to our target spot
					for (i = 0; i < FalconLocalSession->CameraCount(); i++)
						if (FalconLocalSession->Camera(i) == myBrain->targetSpotFlight)
							break;
					if (i == FalconLocalSession->CameraCount())
						FalconLocalSession->AttachCamera(myBrain->targetSpotFlight->Id());
				  }

				  break;
		  }

		  break;

	  // END OF ADDED SECTION 2002-02-08
      case FalconWingmanMsg::WMBreakRight:
      case FalconWingmanMsg::WMBreakLeft:
      case FalconWingmanMsg::WMFlex:
	  // 2002-01-29 ADDED BY S.G. Timeout quickly on a rejoin command
		  break;

      case FalconWingmanMsg::WMRejoin:
		  myBrain = (DigitalBrain *)(SimDriver.playerEntity->Brain());

		  switch (extent) {
			  case AiWingman:
				  if (myBrain->targetSpotWing)
					  myBrain->targetSpotWingTimer = SimLibElapsedTime + 15 * SEC_TO_MSEC;
				  break;

			  case AiElement:
				  if (myBrain->targetSpotElement)
					  myBrain->targetSpotElementTimer = SimLibElapsedTime + 15 * SEC_TO_MSEC;
				  break;

			  case AiFlight:
				  if (myBrain->targetSpotFlight)
					  myBrain->targetSpotFlightTimer = SimLibElapsedTime + 15 * SEC_TO_MSEC;
				  if (myBrain->targetSpotWing)
					  myBrain->targetSpotWingTimer = SimLibElapsedTime + 15 * SEC_TO_MSEC;
				  if (myBrain->targetSpotElement)
					  myBrain->targetSpotElementTimer = SimLibElapsedTime + 15 * SEC_TO_MSEC;
				  break;
		  }
		  break;

	  // END OF ADDED SECTION 2002-01-29
      case FalconWingmanMsg::WMResumeNormal:
      case FalconWingmanMsg::WMSearchGround:
      case FalconWingmanMsg::WMSearchAir:
      case FalconWingmanMsg::WMKickout:
      case FalconWingmanMsg::WMCloseup:
      case FalconWingmanMsg::WMToggleSide:
      case FalconWingmanMsg::WMIncreaseRelAlt:
      case FalconWingmanMsg::WMDecreaseRelAlt:
      case FalconWingmanMsg::WMGiveBra:
      case FalconWingmanMsg::WMGiveStatus:
      case FalconWingmanMsg::WMGiveDamageReport:
      case FalconWingmanMsg::WMGiveFuelState:
      case FalconWingmanMsg::WMGiveWeaponsCheck:
      case FalconWingmanMsg::WMRTB:
      case FalconWingmanMsg::WMFree:
      case FalconWingmanMsg::WMPromote:
      case FalconWingmanMsg::WMRadarStby:
      case FalconWingmanMsg::WMRadarOn:
	  case FalconWingmanMsg::WMDropStores:
	  case FalconWingmanMsg::WMECMOn:
	  case FalconWingmanMsg::WMECMOff:
	  // 2002-03-15 ADDED BY S.G. BVR profiles that the player can send to his element
	  case FalconWingmanMsg::WMPlevel1a:
	  case FalconWingmanMsg::WMPlevel2a:
	  case FalconWingmanMsg::WMPlevel3a:
	  case FalconWingmanMsg::WMPlevel1b:
	  case FalconWingmanMsg::WMPlevel2b:
	  case FalconWingmanMsg::WMPlevel3b:
	  case FalconWingmanMsg::WMPlevel1c:
	  case FalconWingmanMsg::WMPlevel2c:
	  case FalconWingmanMsg::WMPlevel3c:
	  case FalconWingmanMsg::WMPbeamdeploy:
	  case FalconWingmanMsg::WMPbeambeam:
	  case FalconWingmanMsg::WMPwall:
	  case FalconWingmanMsg::WMPgrinder:
	  case FalconWingmanMsg::WMPwideazimuth:
	  case FalconWingmanMsg::WMPshortazimuth:
	  case FalconWingmanMsg::WMPwideLT:
	  case FalconWingmanMsg::WMPShortLT:
	  case FalconWingmanMsg::WMPDefensive:
	  // END OF ADDED SECTION 2002-03-15
      default:
      break;
   }

	AiSendCommand ((SimBaseClass*) SimDriver.playerEntity, command, extent, targetId);
}

// --------------------------------------------------------------------
//
// AiSendCommand
//
// --------------------------------------------------------------------

void AiSendCommand (SimBaseClass* p_sender, int command, int extent, VU_ID targetid)
{
    if (PlayerOptions.PlayerRadioVoice)
	AiMakeRadioCall( p_sender, command, extent, targetid );	
    AiMakeCommandMsg( p_sender, command, extent, targetid);
}



// --------------------------------------------------------------------
//
// AiMakeCommandMsg
//
// --------------------------------------------------------------------
extern bool g_bMPFix2;
void AiMakeCommandMsg (SimBaseClass* p_sender, int command, int extent, VU_ID targetid)
{
	FalconWingmanMsg*							p_commandMsgs;
	if (g_bMPFix2)
	p_commandMsgs	= new FalconWingmanMsg (p_sender->GetCampaignObject()->Id(), (VuTargetEntity*) vuDatabase->Find(vuLocalSessionEntity->Game()->OwnerId()));
	else
	p_commandMsgs	= new FalconWingmanMsg (p_sender->GetCampaignObject()->Id(), FalconLocalGame);

	p_commandMsgs->dataBlock.from			= p_sender->Id();
	p_commandMsgs->dataBlock.to			= extent;
	p_commandMsgs->dataBlock.command		= command;
	p_commandMsgs->dataBlock.newTarget	= targetid;

	FalconSendMessage (p_commandMsgs, TRUE);
}


// --------------------------------------------------------------------
// 
// AiDesignateTarget
//
// --------------------------------------------------------------------

VU_ID AiDesignateTarget(AircraftClass* paircraft) {
	
	VU_ID targetId = FindAircraftTarget (paircraft);

	return targetId;
}

// --------------------------------------------------------------------
// 
// AiCheckForThreat
//
// --------------------------------------------------------------------

// position; 0 = ahead, 1 = behind

VU_ID AiCheckForThreat(AircraftClass* paircraft, char domain, int position, float* az) 
{
	
	SimObjectType		*pobjectPtr;
	SimObjectType		*pthreatPtr = NULL;
	VuEntityType		*pclassPtr;
	char					vuDomain;
	char					vuClass;
	char					vuType;
	char					vuSType; //JB 010527 (from MN) ///////// New Variable for subtype
	BOOL					inSideATA = FALSE;

   pobjectPtr = paircraft->targetList;
   while (pobjectPtr)
	{
		pclassPtr	= pobjectPtr->BaseData()->EntityType();
		vuDomain		= pclassPtr->classInfo_[VU_DOMAIN];
		vuClass		= pclassPtr->classInfo_[VU_CLASS];
		vuType		= pclassPtr->classInfo_[VU_TYPE];
		vuSType		= pclassPtr->classInfo_[VU_STYPE]; //JB 052701 (from MN)

		if(position == 1 && pobjectPtr->localData->ata >= 90.0F * DTR) {
			inSideATA = TRUE; // if something is behind us and we are looking there
		}
		else if(position == 0 && pobjectPtr->localData->ata <= 90.0F * DTR) {
			inSideATA = TRUE;// if something is ahead of us and we are looking ther
		}

		//////// Only change needed here for subtype check => AiCheckForThreat is called from both "Check" and "Clear my 6" !! //JB 052701 (from MN)
		//if((vuDomain == DOMAIN_AIR && vuClass == CLASS_VEHICLE && (vuType == TYPE_AIRPLANE || vuType == TYPE_HELICOPTER))  //JB 052701 (from MN)
		if((vuDomain == DOMAIN_AIR && vuClass == CLASS_VEHICLE && (vuType == TYPE_AIRPLANE) && (vuSType == STYPE_AIR_FIGHTER_BOMBER || vuSType == STYPE_AIR_FIGHTER)) // Removed Helicopters as threat (|| vuType == TYPE_HELICOPTER), added subtype check //JB 052701 (from MN)
			&& inSideATA && pobjectPtr->localData->threatTime <= 60.0F) {
			if(pthreatPtr == NULL || pobjectPtr->localData->threatTime < pthreatPtr->localData->threatTime) {
				if(GetTTRelations(pobjectPtr->BaseData()->GetTeam(), paircraft->GetTeam()) >= Hostile) {
					pthreatPtr = pobjectPtr;
				}
			}
		}

      pobjectPtr = pobjectPtr->next;
	}

	if(pthreatPtr) {
		if(az) {
			*az = pthreatPtr->localData->az;
		}
		return pthreatPtr->BaseData()->Id();
	}
	else {
		if(az) {
			*az = 0.0F;
		}
		return FalconNullId;
	}
}

