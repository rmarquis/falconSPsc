#include "stdhdr.h"
#include "Graphics\Include\drawbsp.h"
#include "simbase.h"
#include "otwdrive.h"
#include "initdata.h"
#include "digi.h"
#include "PilotInputs.h"
#include "object.h"
#include "sms.h"
#include "fcc.h"
#include "ClassTbl.h"
#include "Entity.h"
#include "guns.h"
#include "misslist.h"
#include "bombfunc.h"
#include "simvudrv.h"
#include "SimDrive.h"
#include "MsgInc\DamageMsg.h"
#include "MsgInc\WingmanMsg.h"
#include "MsgInc\DeathMessage.h"
#include "falcsess.h"
#include "CampBase.h"
#include "aircrft.h"
#include "helo.h"
#include "camp2sim.h"
#include "CampList.h"
#include "find.h"
#include "campbase.h"
#include "flight.h"
#include "Persist.h"
#include "dogfight.h"
#include "wingorder.h"
#include "GameMgr.h"
#include "fsound.h"
#include "falcsnd\voicemanager.h"
#include "missile.h"
#include "dofsnswitches.h"

/* S.G. NEED TO KNOW SO WE CAN SET oldp01[5] */#include "airframe.h"
/* S.G. NEED TO KNOW THE SKILL OF THE WINGMAN/AI */#include "digi.h"
/* S.G. NEED TO KNOW THE VISUAL SENSOR */#include "visual.h"

#ifdef DEBUG
int SimEntities	= 0;
#endif

#ifdef DEBUG
void SimObjCheckOwnership( FalconEntity * );
#endif


void CalcTransformMatrix (SimBaseClass* theObject);

SimBaseNonLocalData::SimBaseNonLocalData()
{
	dx = 0.0f;
	dy = 0.0f;
	dz = 0.0f;
}

SimBaseNonLocalData::~SimBaseNonLocalData()
{
}

SimBaseClass::SimBaseClass(FILE* filePtr) : FalconEntity (filePtr)
{
VU_ID camp_obj;
char flag;

	platformAngles = NULL;
   reinitData = NULL;
   campaignObject = NULL;
   InitData();
   fread (SpecialData(), sizeof (SimBaseSpecialData), 1, filePtr);
   fread (&flag, sizeof (char), 1, filePtr);
   if (flag)
   {
	  fread (&camp_obj, sizeof (VU_ID), 1, filePtr);
	  SetCampaignObject((CampBaseClass*)vuDatabase->Find(camp_obj));
   }
   else
   {
	  fread (&campaignObject, sizeof (int), 1, filePtr);
   }
   fread (&callsignIdx, sizeof (int), 1, filePtr);
   dirty_simbase = 0;

#ifdef DEBUG
   SimEntities++;
#endif

	nonLocalData = NULL;		// OW
}

SimBaseClass::SimBaseClass(VU_BYTE** stream) : FalconEntity (stream)
{
VU_ID	camp_object;
char flag;

	platformAngles = NULL;
   reinitData = NULL;
   campaignObject = NULL;
   InitData();
   memcpy (SpecialData(), *stream, sizeof (SimBaseSpecialData));
   *stream += sizeof (SimBaseSpecialData);
   memcpy (&flag, *stream, sizeof (char));
   *stream += sizeof (char);
   if (flag)
   {
	  memcpy (&camp_object, *stream, sizeof (VU_ID));
	  *stream += sizeof (VU_ID);
	  SetCampaignObject((CampBaseClass*)vuDatabase->Find(camp_object));
   }
   else
   {
	  memcpy (&campaignObject, *stream, sizeof (int));
	  *stream += sizeof (int);
   }
   memcpy (&callsignIdx, *stream, sizeof (int));
   *stream += sizeof (int);
   dirty_simbase = 0;
#ifdef DEBUG
   SimEntities++;
#endif

	nonLocalData = NULL;		// OW
}

SimBaseClass::SimBaseClass(int type) : FalconEntity (type)
{
	platformAngles = NULL;
   reinitData = NULL;
   campaignObject = NULL;
   InitData();
   dirty_simbase = 0;
#ifdef DEBUG
   SimEntities++;
#endif

	nonLocalData = NULL;		// OW
}

void SimBaseClass::InitData (void)
{
	strength = 0.0;
	maxStrength = 0.0;
	pctStrength = 0.0;
	dyingTimer = 0.0;
	sfxTimer = 0.0;
	timeOfDeath = 0;
	lastDamageTime = 0;
	explosionTimer = 0;
	
	drawPointer = NULL;
	displayPriority = 0;
	campaignFlags = 0;
	localFlags = 0;
	
// 2000-11-17 MODIFIED BY S.G. SO THE WHOLE incomingMissile ARRAY IS INITIALIZED
//	incomingMissile = NULL;
	for (int i = 0; i < sizeof (incomingMissile) / sizeof (SimBaseClass *); i++)
		incomingMissile[i] = NULL;
// END OF MODIFIED SECTION

	threatObj = NULL;
	threatType = THREAT_NONE;

	// edg: why wouldn't platform angles be NULL?  I'm suspicious that we
	// may be getting called 2x here and leaking mem
	if ( platformAngles == NULL )
		platformAngles  = new ObjectGeometry;
	else
		threatObj = NULL;
	
	F4Assert(platformAngles != NULL);
	
	SetStatus(0);
	SetSlot(0);
	SetCountry(0);
	SetCallsign(0);
	
	SetTypeFlag(FalconSimEntity);
	lastShooter = FalconNullId;
	
	lastChaff	= 0;
	lastFlare	= 0;
	
	dmx[0][0] = 1.0F;
	dmx[0][1] = 0.0F;
	dmx[0][2] = 0.0F;
	dmx[1][0] = 0.0F;
	dmx[1][1] = 1.0F;
	dmx[1][2] = 0.0F;
	dmx[2][0] = 0.0F;
	dmx[2][1] = 0.0F;
	dmx[2][2] = 1.0F;
	
	if (IsLocal())
	{
		SetPosition (0.0f, 0.0f, 0.0f );
		SetDelta (0.0f, 0.0f, 0.0f );
		SetYPR (0.0f, 0.0f, 0.0f );
		SetYPRDelta (0.0f, 0.0f, 0.0f );
	}
}

SimBaseClass::~SimBaseClass(void)
{
	SimBaseClass::Cleanup();

	if (reinitData)
		delete reinitData;
	reinitData = NULL;

#ifdef DEBUG
	// See if we still own any SimObjectType things.
	// This function could hammer them to stop memory leaks, BUT
	// that might cause evil over dereferencing problems, so for now
	// it just Asserts so we can fix the problems instead of hiding them.
	SimObjCheckOwnership( this );
#endif
#ifdef DEBUG
	SimEntities--;
#endif

	ShiAssert (!IsAwake ());
	// We should not be awake - this is the out of order messages stuff
	// REMOVE ASAP
	if (IsAwake ())
	{
		Sleep ();
	}
}

void SimBaseClass::Cleanup()
{
	ShiAssert(!IsAwake());

	if (drawPointer)
		{
		OTWDriver.RemoveObject(drawPointer, TRUE);
		drawPointer = NULL;
		}
	if ( platformAngles )
		delete platformAngles;
	platformAngles = NULL;

	// JB 010118
	SetIncomingMissile( NULL, TRUE );
	if (campaignObject)
	    VuDeReferenceEntity(campaignObject); // JPO/JB mem leak fix
	campaignObject = NULL;
}

void SimBaseClass::Init (SimInitDataClass* initData)
{
   if (initData)
   {
      SetPosition (initData->x, initData->y, initData->z);
      SetCampaignFlag(initData->flags);
      SetStatus (initData->status);
      displayPriority = initData->displayPriority;
      if (initData->createFlags & SIDC_FORCE_ID)
         share_.id_ = initData->forcedId;
      SetCallsign (initData->callsignIdx);
      SetSlot(initData->campSlot);
      SetCountry(initData->side);
   }

//   OTWDriver.CreateVisualObject(this, OTWDriver.Scale());
}

void SimBaseClass::ChangeOwner (VU_ID new_owner)
{
	ShiAssert ( new_owner );

	if (new_owner == OwnerId())
		return;		// Nothing to do!

	if (IsVehicle ())
	{
		Falcon4EntityClassType	*classPtr = (Falcon4EntityClassType*)this->EntityType();
		char					label[40] = {0};
		CampEntity				campObj;

		campObj = GetCampaignObject();
		if (campObj && campObj->IsFlight() && campObj->InPackage())
		{
			char		temp[40];
			GetCallsign(((Flight)campObj)->callsign_id,((Flight)campObj)->callsign_num,temp);
			sprintf (label, "%s%d",temp,((SimVehicleClass*)this)->vehicleInUnit+1);
		}
		else
		{
			sprintf (label, "%s", ((VehicleClassDataType*)(classPtr->dataPtr))->Name);
		}

//		MonoPrint("Changing owner of %s to %08x : ", label, new_owner.creator_);
	}

	// LEON TODO: Fill in various sub class's MakeRemote() functions
	if (IsLocal() && new_owner != FalconLocalSession->Id())
	{
		if (IsVehicle ())
		{
//			MonoPrint ("Make Remote %08x\n", this);
		}
		else
		{
//			MonoPrint ("NOT A VEHICLE\n");
		}

		MakeRemote();
	}

	// KCK: Changes VU owner id to the new session
	// Note, Also sends transfer message to the remote machine
	SetOwnerId(new_owner);

	// LEON TODO: Fill in various sub class's MakeLocal() functions
	if (IsLocal())
	{
		if (IsVehicle ())
		{
//			MonoPrint ("Make Local %08x\n", this);
		}
		else
		{
//			MonoPrint ("NOT A VEHICLE\n");
		}

		MakeLocal();
	}
}

void SimBaseClass::MakeLocal (void)
	{
	// LEON TODO: Need to do all necessary shit to convert from a local to a remote entity.
	}

void SimBaseClass::MakeRemote (void)
	{
	// LEON TODO: Need to do all necessary shit to convert from a remote to a local entity.
	}

uchar SimBaseClass::GetTeam (void)
{
	return (uchar)(::GetTeam((uchar)specialData.country));
}

short SimBaseClass::GetCampID (void)
{
	if (GetCampaignObject()) {
		return GetCampaignObject()->GetCampID();
	} else {
		return 0;
	}
}

int SimBaseClass::Wake (void)
{
	float scale;

	// KCK: Sets up this object to become sim aware
	ShiAssert (!IsAwake());

	// Join our flight, if we have one
	JoinFlight();

	// edg: sanity check scale -- I've had problems.  Don't ask....
	scale = OTWDriver.Scale();
	if ( scale < 1.0f )
		scale = 1.0f;

	ShiAssert (OTWDriver.IsActive());

	// Create a drawable object
	// Note:  Our child classes could have already created our drawable for us.
	if (!drawPointer)
		OTWDriver.CreateVisualObject (this, scale);

	ShiAssert (drawPointer);
	
	SetLocalFlag(OBJ_AWAKE);

	return 0;
}

int SimBaseClass::Sleep (void)
{
int retval = 0;

	// KCK: Stops this object from being sim aware
	if (!IsAwake())
		return retval;

// 2000-11-17 MODIFIED BY S.G. SO THE WHOLE incomingMissile ARRAY IS CLEARED
//  SetIncomingMissile( NULL );
	SetIncomingMissile( NULL, TRUE );
// END OF ADDED SECTION
    SetThreat( NULL, THREAT_NONE );

	UnSetLocalFlag(OBJ_AWAKE);

	// Remove the drawable object
	if (drawPointer)
		{
		OTWDriver.RemoveObject (drawPointer, TRUE);
		drawPointer = NULL;
		}

#if 0	// The code screws up IA by violently removing vehicles from the campaign unit prior to reagg, thus "losing" them.
/*
	if (SimDriver.RunningInstantAction() && !IsSetFlag(MOTION_OWNSHIP))
		{
		// This is an instant action game - We want to remove the object
		// and regenate a new one
		//if (Strength() < MaxStrength())
			//SimDriver.UpdateIAStats(this);
		//else
			//InstantAction.RegenTarget (this);
		if (!IsSetRemoveFlag())
			SetRemoveFlag();
		}
*/
#endif
	return retval;
}

MoveType SimBaseClass::GetMovementType (void)
	{
	if (campaignObject)
		return campaignObject->GetMovementType();
	else
		return NoMove;		// This should eventually never happen, but probably doesn't matter in IA or Dogfight
	}

// KCK: This actually removes the sim entity from the game,
// without actually setting it dead - used for both death
// and reaggregation.
void SimBaseClass::SetRemoveFlag (void)
{
// 2000-11-17 MODIFIED BY S.G. SO THE WHOLE incomingMissile ARRAY IS CLEARED
//  SetIncomingMissile( NULL );
	SetIncomingMissile( NULL, TRUE );
// END OF ADDED SECTION
    SetThreat( NULL, THREAT_NONE );

	// OW commented out after fixing feature memory leak (features now using this function too)
//	ShiAssert(!IsStatic());

	if (IsAwake())
		Sleep();

	// Detach any player who is attached to this entity
	if (IsPlayer())
		{
		FalconSessionEntity *session = (FalconSessionEntity*) vuDatabase->Find(OwnerId());
		GameManager.DetachPlayerFromVehicle (session, (SimMoverClass*)this);
		// KCK: One problem with this is that when the entity goes away, so does this
		// player's camera. I'm going to temporarily set the player's camera to the
		// player's session, and setting the session's location
		// This will get cleared on sim exit.
		if (session)
			{
			session->SetPosition(XPos(),YPos(),ZPos());
			session->ClearCameras();
			session->AttachCamera(session->Id());
			}
		// Pan out if it's the local player
		if (session == FalconLocalSession)
			{
			OTWDriver.SetEndFlightPoint(
				XPos() - dmx[0][0] * 1000.0f,
				YPos() - dmx[0][1] * 1000.0f,
				ZPos() - dmx[0][2] * 1000.0f );
			OTWDriver.SetEndFlightVec(
				-dmx[0][0],
				-dmx[0][1],
				-dmx[0][2] );
			}
		}
	localFlags |= REMOVE_NEXT_FRAME;

    SetFlag (OBJ_DEAD);

	vuDatabase->Remove(this);
}

void SimBaseClass::SetRemoveSilentFlag (void)
{
// 2000-11-17 MODIFIED BY S.G. SO THE WHOLE incomingMissile ARRAY IS CLEARED
//  SetIncomingMissile( NULL );
	SetIncomingMissile( NULL, TRUE );
// END OF ADDED SECTION
    SetThreat( NULL, THREAT_NONE );

	if (IsAwake())
		Sleep();

	// Detach any player who is attached to this entity
	if (IsPlayer())
		{
		FalconSessionEntity *session = (FalconSessionEntity*) vuDatabase->Find(OwnerId());
		GameManager.DetachPlayerFromVehicle (session, (SimMoverClass*)this);
		// KCK: One problem with this is that when the entity goes away, so does this
		// player's camera. I'm going to temporarily set the player's camera to the
		// player's session, and setting the session's location
		// This will get cleared on sim exit.
		if (session)
			{
			session->SetPosition(XPos(),YPos(),ZPos());
			session->ClearCameras();
			session->AttachCamera(session->Id());
			}
		// Pan out if it's the local player
		if (session == FalconLocalSession)
			{
			OTWDriver.SetEndFlightPoint(
				XPos() - dmx[0][0] * 1000.0f,
				YPos() - dmx[0][1] * 1000.0f,
				ZPos() - dmx[0][2] * 1000.0f );
			OTWDriver.SetEndFlightVec(
				-dmx[0][0],
				-dmx[0][1],
				-dmx[0][2] );
			}
		}
	localFlags |= REMOVE_NEXT_FRAME;

    SetFlag (OBJ_DEAD);

	vuDatabase->SilentRemove(this);
}

void SimBaseClass::SetExploding(int flag)
{
// 2000-11-17 MODIFIED BY S.G. SO THE WHOLE incomingMissile ARRAY IS CLEARED
//  SetIncomingMissile( NULL );
	SetIncomingMissile( NULL, TRUE );
// END OF ADDED SECTION
   SetThreat( NULL, THREAT_NONE );
   if (flag)
   {
      SetFlag (OBJ_EXPLODING);
   }
   else
   {
	   UnSetFlag (OBJ_EXPLODING);
   }
}


void SimBaseClass::SetDead(int flag)
{
// 2000-11-17 MODIFIED BY S.G. SO THE WHOLE incomingMissile ARRAY IS CLEARED
//  SetIncomingMissile( NULL );
	SetIncomingMissile( NULL, TRUE );
// END OF ADDED SECTION
	SetThreat( NULL, THREAT_NONE );
	if (flag)
	{
		// Pdromote another flight member, if this is the lead
		if (campaignObject && campaignObject->GetComponents () && campaignObject->GetComponentLead() == this)
		{
			//			MonoPrint ("Sending Promote Message\n");
			
			FalconWingmanMsg* wingCommand			= new FalconWingmanMsg (campaignObject->Id(), vuLocalSessionEntity);
			wingCommand->dataBlock.from			= Id();
			wingCommand->dataBlock.to				= AiAllButSender;
			wingCommand->dataBlock.command		= FalconWingmanMsg::WMPromote;
			wingCommand->dataBlock.newTarget		= FalconNullId;
			
			FalconSendMessage (wingCommand,TRUE);
		}
		SetFlag (OBJ_DEAD);
		
		// Check for undead aircraft, and regenerate instead of removing
		if (IsSetFalcFlag(FEC_REGENERATING))
		{
// 2000-11-17 MODIFIED BY S.G. SO THE WHOLE incomingMissile ARRAY IS CLEARED
//			SetIncomingMissile( NULL );
			SetIncomingMissile( NULL, TRUE );
// END OF ADDED SECTION
			SetThreat( NULL, THREAT_NONE );
			
			if (IsAwake())
			{
				Sleep();
			}
			
			if (IsLocal())
			{
				SimDogfight.RegenerateAircraft((AircraftClass*)this);
				if (this == SimDriver.playerEntity)
				{
					SimDriver.SetPlayerEntity (NULL);
				}
			}
		}
		else if (!IsSetRemoveFlag())
		{
			if (this == FalconLocalSession->GetPlayerEntity())
			{
				OTWDriver.StartExitMenuCountdown();
			}

			// OTWDriver.SetExitMenu (TRUE);
			SetRemoveFlag();
		}
	}
	else
	{
		UnSetFlag (OBJ_DEAD);
	}
}


void SimBaseClass::SetFiring(int flag)
{
   if (flag)
      SetFlag (OBJ_FIRING_GUN);
   else
      UnSetFlag (OBJ_FIRING_GUN);
}

void SimBaseClass::SetCampaignObject (CampBaseClass *ent)
{
	if (campaignObject == ent)
		return;

   if ((int)ent > MAX_IA_CAMP_UNIT)
   {
	   if (campaignObject)
		{
		   VuDeReferenceEntity(campaignObject);
		   campaignObject = NULL;
		}
	   if (ent)
		{
		   VuReferenceEntity(ent);
		   campaignObject = ent;
		}
   }
}

int SimBaseClass::SaveSize(void)
{
int size = FalconEntity::SaveSize() +
	   sizeof (SimBaseSpecialData); // Special Data for each frame
	if ((int)campaignObject > MAX_IA_CAMP_UNIT)
		size += sizeof (VU_ID);
	else
		size += sizeof (int);
	size += sizeof (int);                 // Callsign idx
	size += sizeof (char);                // id type flag
	return size;
}

int SimBaseClass::Save(VU_BYTE **stream)
{
int saveSize = FalconEntity::Save (stream);
char flag;

   memcpy (*stream, SpecialData(), sizeof (SimBaseSpecialData));
   *stream += sizeof (SimBaseSpecialData);
   VU_ID	camp_object;
   if ((int)campaignObject > MAX_IA_CAMP_UNIT)
   {
	   flag = 1;
	   memcpy (*stream, &flag, sizeof (char));
	   *stream += sizeof (char);
	   camp_object = campaignObject->Id();
	   memcpy (*stream, &camp_object, sizeof (VU_ID));
	   *stream += sizeof (VU_ID);
   }
   else
   {
	   flag = 0;
	   memcpy (*stream, &flag, sizeof (char));
	   *stream += sizeof (char);
	   memcpy (*stream, &campaignObject, sizeof (int));
	   *stream += sizeof (int);
   }
   memcpy (*stream, &callsignIdx, sizeof (int));
   *stream += sizeof (int);

   return (saveSize + sizeof (SimBaseSpecialData) + sizeof (char) +
	   sizeof (VU_ID) + sizeof (int));
}

int SimBaseClass::Save(FILE *file)
{
int saveSize = FalconEntity::Save (file);
VU_ID	camp_object;
char flag;

   fwrite (SpecialData(), sizeof (SimBaseSpecialData), 1, file);

   if ((int)campaignObject > MAX_IA_CAMP_UNIT)
   {
	   flag = 1;
	   camp_object = campaignObject->Id();
	   fwrite (&flag, sizeof (char), 1, file);
	   fwrite (&camp_object, sizeof (VU_ID), 1, file);
   }
   else
   {
	   flag = 0;
	   fwrite (&flag, sizeof (char), 1, file);
	   fwrite (&campaignObject, sizeof (int), 1, file);
   }

   fwrite (&callsignIdx, sizeof (int), 1, file);
   return (saveSize + sizeof (SimBaseSpecialData) + sizeof (char) + 
	   sizeof (VU_ID) + sizeof (int));
}

int SimBaseClass::Handle(VuFullUpdateEvent *event)
{
   return (VuEntity::Handle(event));
}

int SimBaseClass::Handle(VuPositionUpdateEvent *event)
{
	return VuEntity::Handle(event);
}

int SimBaseClass::Handle(VuTransferEvent *event)
{
	ChangeOwner(event->newOwnerId_);
	return 1;
}

void SimBaseClass::ApplyDamage (FalconDamageMessage* damageMessage)
{
   lastShooter = damageMessage->dataBlock.fEntityID;
}

void SimBaseClass::ApplyDeathMessage (FalconDeathMessage* deathMessage)
{
	// we want to set pctStrength to new value here....
	// do this when pctStrenght from the death message is less than the
	// current pctStrength.  This should help sync dying on different
	// hosts on the net
	if ( pctStrength > deathMessage->dataBlock.deathPctStrength )
		pctStrength = deathMessage->dataBlock.deathPctStrength;

//    MonoPrint ("Entity %d Got Death Msg! Local: %s, Show Explode: %s\n",
//		   		Id().num_,
//				IsLocal() ? "TRUE" : "FALSE",
//				IsSetFlag( SHOW_EXPLOSION ) ? "TRUE" : "FALSE" );

	if (VM)
		VM->RemoveRadioCalls(Id());

   	// debug non local
   	if ( !IsLocal() )
   	{
//	 	MonoPrint( "NonLocal Apply Death Message: Pct Strength now: %f\n", pctStrength );

		// OK, this is a big hack for the time being.  Explosions aren't
		// showing for nonlocal entities.  Let's force them right now:
		if ( !IsSetFlag( SHOW_EXPLOSION ) )
		{
			if (IsAwake())
			{
				if ( IsAirplane() )
				{
					((AircraftClass *)this)->RunExplosion();
				}
				else if ( IsHelicopter() )
				{
					((HelicopterClass *)this)->RunExplosion();
				}
			}
			SetExploding( TRUE ); // make sure
			SetFlag( SHOW_EXPLOSION );
		}

		if ((!IsStatic ()))
		{
			SetDead (TRUE);
		}
   	}
}

void SimBaseClass::GetFocusPoint
(
	BIG_SCALAR &x,
	BIG_SCALAR &y,
	BIG_SCALAR &z
)
{
	x = XPos();
	y = YPos();
	z = ZPos();
}

VU_ERRCODE SimBaseClass::InsertionCallback(void)
{
	CalcTransformMatrix (this);

	if (!campaignObject)
		{
		// This is not a campaign controlled object
		// put it in our list of objects with no campaign parent
		SimDriver.ObjsWithNoCampaignParentList->ForcedInsert(this);
		}

	return FalconEntity::InsertionCallback();
}

VU_ERRCODE SimBaseClass::RemovalCallback(void)
{
	if (IsAwake())
		Sleep();

	SimDriver.ObjsWithNoCampaignParentList->Remove(this);
	return FalconEntity::RemovalCallback();
}

int SimBaseClass::IsSPJamming (void)
{
   return IsSetFlag(ECM_ON);
}

int SimBaseClass::IsAreaJamming (void)
{
   return FALSE;
}

int SimBaseClass::HasSPJamming (void)
{
	// This is clearly wrong -- we can return TRUE to IsSPJamming, while not admitting to
	// HAVING SPJamming....
   return FALSE;
}

int SimBaseClass::HasAreaJamming (void)
{
   return FALSE;
}

int SimBaseClass::GetPilotVoiceId(void)
{
	if(campaignObject > (VuEntity*)MAX_IA_CAMP_UNIT) {
		return ((FlightClass*) campaignObject)->GetPilotVoiceID(slotNumber);
	}
	else {
		return slotNumber;
	}
}

int SimBaseClass::GetFlightCallsign(void)
{
	return (callsignIdx - 1) * 4 + slotNumber;
}


/*
** Name: SetIncomingMissile
** Description:
**		Called by FalconWeaponsFire message.  Sets and references an
**		incoming missile shot at us.  If there's one already set and
**		it's not dead, do nothing.  If it's dead or arg is NULL deref
**		the missile.
**
** Used to inform the player of missile launches with a wingman message
**
*/
// 2000-11-17 MODIFIED BY S.G. SO THE SetIncomingMissile HAS TWO PARAMTER (LAST ONE OPTIONAL INSTEAD OF ONE)
//void SimBaseClass::SetIncomingMissile (SimBaseClass *missile )
void SimBaseClass::SetIncomingMissile (SimBaseClass *missile, BOOL clearAll)
{
FalconRadioChatterMessage	*radioMessage=NULL;
short mesgType=0;
short data0=-1, data1=-1, data2=-1;
SimBaseClass* speaker=NULL;
float dx=0.0F, dy=0.0F;

   // Is the target the player
   if (missile && this == SimDriver.playerEntity)
	{
      // Does he have a wingman?
      // Component list is zero based
      if (SimDriver.RunningInstantAction())
         speaker = this;
      else
         speaker = SimDriver.playerEntity->GetCampaignObject()->GetComponentNumber(1);

      if (speaker && incomingMissile[0] != missile)
      {
// 2000-10-02 ADDED BY S.G. Wingman NEEDS TO BE CLOSE TO THE SHOOTER AND NOT DEFENSIVE (UNLESS ACE) BEFORE A LAUNCH WARNING IS SAID
		float dx, dy, dz, rangeSquare;

		// 2000-10-10 Instead of using the distance between us and our wingman, I'll use the distance between our wingman and the missile.
		// He must be able to see (we won't bother doing frame occlusion here though) it before he can warn the player.
		// We will assume if the missile was launch from 6 NM or closer, he WILL see it unless other conditions prevent him from seeing it
		// This will get the REAL range between our wingman and us.
		dx = speaker->XPos() - missile->XPos();
		dy = speaker->YPos() - missile->YPos();
		dz = speaker->ZPos() - missile->ZPos();
		rangeSquare = dx*dx + dy*dy + dz*dz;

		if (rangeSquare < 6.0f * NM_TO_FT * 6.0f * NM_TO_FT &&				// Range is below 6 NM AND 
			(SimDriver.RunningInstantAction() ||							// We're in instant action (in thise case rangeSquare will be 0 because it's the player) OR
			 ((SimVehicleClass *)speaker)->Brain()->SkillLevel() == 4 ||		// Skill of wingman is ace OR
			 (((DigitalBrain *)((SimVehicleClass *)speaker)->Brain())->GetCurrentMode() != DigitalBrain::GunsJinkMode &&		// Wingman not defensive
			 ((DigitalBrain *)((SimVehicleClass *)speaker)->Brain())->GetCurrentMode()  != DigitalBrain::MissileDefeatMode &&
			 ((DigitalBrain *)((SimVehicleClass *)speaker)->Brain())->GetCurrentMode()  != DigitalBrain::DefensiveModes))) {

// END OF ADDED SECTION EXCEPT FOR THE INDENTATION OF THE FOLLOWING CODE
			 if (missile->EntityType()->classInfo_[VU_STYPE] == STYPE_MISSILE_AIR_AIR)
			 {
   			  mesgType = rcINBOUND;
				data0 = (short)(missile->Type() - VU_LAST_ENTITY_TYPE);
//            MonoPrint ("Playing Air Missile message %d\n", data0);
			 }
			 else if (missile->EntityType()->classInfo_[VU_STYPE] == STYPE_MISSILE_SURF_AIR)
			 {
			     ShiAssert(missile->IsMissile());
   			  mesgType = rcSAM;
				data0 = ((FlightClass*)speaker->GetCampaignObject())->callsign_id;
				data1 = (short)((((FlightClass*)speaker->GetCampaignObject())->callsign_num - 1) * 4 + 2);
				// Find location of launch
				// Note missile vu entity doesn't actually have a position yet
				dx = ((MissileClass*)missile)->x - speaker->XPos();
				dy = ((MissileClass*)missile)->y - speaker->YPos();
				dx = (float)atan2 (dy, dx);
				if (dx < -157.5F * DTR)
				   data2 = 5; // South
				else if (dx < -112.5F * DTR)
				   data2 = 6; // Southwest
				else if (dx < -67.5F * DTR)
				   data2 = 7; // West
				else if (dx < -22.5F * DTR)
				   data2 = 0; // Northwest
				else if (dx <  22.5F * DTR)
				   data2 = 1; // North
				else if (dx <  67.5F * DTR)
				   data2 = 2; // Northeast
				else if (dx <  112.5F * DTR)
				   data2 = 3; // East;
				else if (dx <  157.5F * DTR)
				   data2 = 4; // Southeast
				else
				   data2 = 5; // South again
				//MonoPrint ("Playing SAM Missile message %d %d %d\n", data0, data1, data2);
			 }
			 else
   			  mesgType = 0;

			 if (mesgType)
			 {
				 radioMessage = new FalconRadioChatterMessage( Id() , FalconLocalGame );
				 radioMessage->dataBlock.edata[0] = data0;
				 radioMessage->dataBlock.edata[1] = data1;
				 radioMessage->dataBlock.edata[2] = data2;
//				 radioMessage->dataBlock.voice_id = (uchar)speaker->GetPilotVoiceId();
				 radioMessage->dataBlock.voice_id = ((FlightClass*)(((AircraftClass*)speaker)->GetCampaignObject()))->GetPilotVoiceID(((AircraftClass*)speaker)->GetCampaignObject()->GetComponentIndex(((AircraftClass*)speaker)));
				 radioMessage->dataBlock.from = speaker->GetCampaignObject()->Id();
				 radioMessage->dataBlock.to = MESSAGE_FOR_FLIGHT;
				 radioMessage->dataBlock.message = mesgType;
				 radioMessage->dataBlock.time_to_play = 50;

				radioMessage->dataBlock.time_to_play *= (rand() % 5) + 1;

				FalconSendMessage (radioMessage, FALSE);
			 } // SAM or AIM
		} // END OF ADDED INDENTATION
      } // Has wingman
   } // Is player

/* 2000-09-05 REWRITTEN BY S.G.
	if ( incomingMissile && (missile == NULL || incomingMissile->IsDead()) )
	{
		VuDeReferenceEntity( incomingMissile );
		incomingMissile = NULL;
	}

	if ( missile && incomingMissile == NULL)
	{
		incomingMissile = missile;
		VuReferenceEntity( incomingMissile );
	}
*/
// 2000-11-17 ADDED BY S.G. SO IF WE HAVE TRUE FOR clearAll, THAT'S WHAT WE DO...
	if (clearAll == TRUE) {
		// Clean up the array and that's it...
		for (int i = 0; i < sizeof (incomingMissile) / sizeof (SimBaseClass *); i++) {
			if (incomingMissile[i]) {
				VuDeReferenceEntity( incomingMissile[i]);
				incomingMissile[i] = NULL;
			}
		}
		return;
	}
	
	// If it's not an airplane or the target is the player, no need to do our fancy tests...
	if (!IsAirplane() || this == SimDriver.playerEntity) {
		if ( incomingMissile[0] && (missile == NULL || incomingMissile[0]->IsDead()) )
		{
			VuDeReferenceEntity( incomingMissile[0] );
			incomingMissile[0] = NULL;
		}

		if ( missile && incomingMissile[0] == NULL)
		{
			incomingMissile[0] = missile;
			VuReferenceEntity( incomingMissile[0] );
		}
	}
	// So it is an AI airplane after all :-)
	else {
		// Because F4 doesn't clean up its act, need to make sure exploded missiles are not accounted for
		// We do the holding spot first
		if (incomingMissile[1] && incomingMissile[1]->IsDead()) {
			VuDeReferenceEntity( incomingMissile[1] );
			incomingMissile[1] = NULL;
		}

		// then the incoming missile spot, this one also check is missile is NULL, then it uses what's in our holding spot for the incoming. That's why we tested it first
		if (incomingMissile[0] && (missile == NULL || incomingMissile[0]->IsDead())) {
			VuDeReferenceEntity( incomingMissile[0] );

			// Now restore our previous missile (if we had one).
			incomingMissile[0] = incomingMissile[1];
			incomingMissile[1] = NULL;
			incomingMissileEvadeTimer = 10000000; // 1645 NM should be high enough for a starting distance
			incomingMissileRange = 500 * NM_TO_FT; //initialize
		}

		// Now that the explosed missile were taken care of, check if we have a missile hurdling toward us...
		if (missile) {
			// 2000-10-02 BEFORE WE DO ANYTHING HERE, WE NEED TO KNOW IF THE AI CAN ACTUALLY SEE THE MISSILE IF IT'S AN IR MISSILE
			// Only IRST missiles need to be picked up visually. Other missile type are handled by MissileDefeatCheck
			if (((MissileClass *)missile)->GetSeekerType() == SensorClass::IRST) {
				int canSee = FALSE;

				// Now we must know if our airplane is eyeball capable...
				VisualClass *eyeball = (VisualClass*)FindSensor((SimMoverClass *)this, SensorClass::Visual );
				if (eyeball) {
					float az, el, ata, ataFrom, droll;

					CalcRelValues (this, missile, &az, &el, &ata, &ataFrom, &droll);

					// Now this will tell us if the missile can be seen
					canSee = eyeball->CanSeeObject(az, el);

					if (canSee) {
						float ourFOV, ourCenter;

						// If we can theoritically see it, now see if we are looking in its direction...
						if (((SimMoverClass *)this)->targetPtr) {
							// We are looking at someone, limit our field of view to 50 to 90 degrees (both side)
							ourFOV = (((SimVehicleClass *)this)->Brain()->SkillLevel() + 5.0f) * 10.0f * DTR;
							ourCenter = ((SimMoverClass *)this)->targetPtr->localData->ata;	// Where we are looking
						}
						else {
							// We are scanning the horizon, we scan from 75 to 135 degress (both side)
							ourFOV = (((SimVehicleClass *)this)->Brain()->SkillLevel() + 5.0f) * 15.0f * DTR;
							ourCenter = 0.0f; // dead center
						}

						// If the missiles angle is further than our (fixated) field of view, we cannot see the missile
						if (fabs(ata - ourCenter) > ourFOV)
							canSee = FALSE;

					}
				}

				// If the missile was launched caged...
				// Marco Edit - Boresight instead of caged
				// 2002-01-02 MODIFIED BY S.G. Removed the '!' so it goes in if slaved.
				if (((MissileClass *)missile)->isSlave) {
					// And we cannot see it...
					if (!canSee) {
						// Roll the dice to see if he will 'see' it based on his skill
						if (((SimVehicleClass *)this)->Brain()->SkillLevel() + 5 > rand() % 10)
							canSee = TRUE;
					}
				}
				// The missile was launched uncaged and we cannot see it and the pilot is either an ace or a veteran...
				else if (!canSee && ((SimVehicleClass *)this)->Brain()->SkillLevel() > 2) {
					// Roll the dice to see if he will 'see' it based on his skill
					if (4 - ((SimVehicleClass *)this)->Brain()->SkillLevel() + 8 <= rand() % 10)
						canSee = TRUE;
				}

				if (!canSee)
					return;
			}


			// Do we have a missile ALREADY hurdling toward us?
			if (incomingMissile[0]) {
				float dx, dy, dz, missileRangeSquared, incomingMissileRangeSquared;

				// This will get the REAL range between the missile and us. targetPtr->localData->range might not point at us or targetPtr might even be NULL. Not good.
				dx = missile->XPos() - this->XPos();
				dy = missile->YPos() - this->YPos();
				dz = missile->ZPos() - this->ZPos();
				missileRangeSquared = dx*dx + dy*dy + dz*dz;

				dx = incomingMissile[0]->XPos() - this->XPos();
				dy = incomingMissile[0]->YPos() - this->YPos();
				dz = incomingMissile[0]->ZPos() - this->ZPos();
				incomingMissileRangeSquared = dx*dx + dy*dy + dz*dz;

				// Is the range of this missile closer than what we are currently avoiding?
				if (missileRangeSquared < incomingMissileRangeSquared) {
					// Yes. so de-reference the missile in the holding queue (must be further than incomingMissile if it's there anyway) if there is one
					if (incomingMissile[1])
						VuDeReferenceEntity( incomingMissile[1] );

					// The currently incoming missile will be moved to the holding spot...
					incomingMissile[1] = incomingMissile[0];

					// And this missile is our new immediate threat
					incomingMissile[0] = missile;
					VuReferenceEntity( missile );
					incomingMissileEvadeTimer = 10000000; // 1645 NM should be high enough for a starting distance
				}
				// If we have a missile already in our holding spot, see if the new one is closer. If it is, it will replace the misile in the holding spot.
				else if (incomingMissile[1]) {
					float holdingMissileRangeSquared;

					dx = incomingMissile[1]->XPos() - this->XPos();
					dy = incomingMissile[1]->YPos() - this->YPos();
					dz = incomingMissile[1]->ZPos() - this->ZPos();
					holdingMissileRangeSquared = dx*dx + dy*dy + dz*dz;

					// If it's closer, de-reference the missile in the holding spot and use this one.
					if (missileRangeSquared < holdingMissileRangeSquared) {
						VuDeReferenceEntity( incomingMissile[1] );

						// Our 'holding' missile will be this one. The other one will be ignored though :-( Once we make a incoming missile queue, this problem will be gone :-)
						incomingMissile[1] = missile;
						VuReferenceEntity( missile );
					}
				}
				// So we don't have a missile in the holding spot yet, make this one that missile.
				else {
						incomingMissile[1] = missile;
						VuReferenceEntity( missile );
				}
			}
			// We don't have an incoming missile, so this one becomes our incoming missile
			else {
				incomingMissile[0] = missile;
				VuReferenceEntity( incomingMissile[0] );
				incomingMissileEvadeTimer = 10000000; // 1645 NM should be high enough for a starting distance
			}
		}
	}
// END OF REWRITTEN SECTION
}


/*
** Name: SetThreat
** Description:
**		Called by FalconWeaponsFire message.  
** 
*/
//void SimBaseClass::SetThreat (FalconEntity *threat, int type )
void SimBaseClass::SetThreat (FalconEntity*, int )
{
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SimBaseClass::WriteDirty (uchar **stream)
{
	ShiAssert( (dirty_simbase & 0xFFFF0000) == 0 );	// Make sure we didn't exceed the capacity of our flag field

	*(short*)*stream = (short)dirty_simbase;
	*stream += sizeof(short);

	if (dirty_simbase & DIRTY_SIM_FLAGS)
	{
//MonoPrint ("DIRTY_SIM_FLAGS DIRTY_SIM_FLAGS");
		*(int*)*stream = specialData.flags;
		*stream += sizeof(specialData.flags);
	}

	if (dirty_simbase & DIRTY_SIM_COUNTRY)
	{
//MonoPrint ("WriteDirtyDIRTY_SIM_COUNTRY ");
		*(int*)*stream = specialData.country;
		*stream += sizeof(specialData.country);
	}

	if (dirty_simbase & DIRTY_SIM_CHAFF)
	{
	//	MonoPrint ("WriteDirty DIRTY_SIM_CHAFF");
		*(VU_ID*)*stream = specialData.ChaffID;
		*stream += sizeof(specialData.ChaffID);
	}

	if (dirty_simbase & DIRTY_SIM_FLARE)
	{
	//	MonoPrint ("WriteDirty DIRTY_SIM_FLARE");
		*(VU_ID*)*stream = specialData.FlareID;
		*stream += sizeof(specialData.FlareID);
	}

	if (dirty_simbase & DIRTY_SIM_STATUS)
	{
//		MonoPrint ("WriteDirty DIRTY_SIM_STATUS");
		*(long*)*stream = specialData.status;
		*stream += sizeof(specialData.status);
	}

	if (dirty_simbase & DIRTY_SIM_POWER_OUTPUT)
	{
//		MonoPrint ("WriteDirty DIRTY_SIM_POWER_OUTPUT");
		*(unsigned char*)*stream = specialData.powerOutputNet;
		*stream += sizeof(specialData.powerOutputNet);
// 2000-11-17 ADDED BY S.G. I NEED TO SEND THE ENGINE TEMP AS WELL
//		*(unsigned char*)*stream = specialData.engineHeatOutputNet;
//		*stream += sizeof(specialData.engineHeatOutputNet);
// END OF ADDED SECTION
	}

	if (dirty_simbase & DIRTY_SIM_RADAR)
	{
	//	MonoPrint ("WriteDirty DIRTY_SIM_RADAR");
		*(float*)*stream = specialData.rdrAz;
		*stream += sizeof(specialData.rdrAz);
		*(float*)*stream = specialData.rdrEl;
		*stream += sizeof(specialData.rdrEl);
// This _SHOULD_ be used by the RWR but isn't currently...
//		*(float*)*stream = specialData.rdrAzCenter;
//		*stream += sizeof(specialData.rdrAzCenter);
//		*(float*)*stream = specialData.rdrElCenter;
//		*stream += sizeof(specialData.rdrElCenter);
	}

	if (dirty_simbase & DIRTY_SIM_RADAR_SLOW)
	{
	//	MonoPrint ("WriteDirty DIRTY_SIM_RADAR_SLOW");
		*(float*)*stream = specialData.rdrNominalRng;
		*stream += sizeof(specialData.rdrNominalRng);
		*(float*)*stream = specialData.rdrCycleTime;
		*stream += sizeof(specialData.rdrCycleTime);
	}

	if (dirty_simbase & DIRTY_SIM_AFTERBURNER)
	{
	//	MonoPrint ("WriteDirty DIRTY_SIM_AFTERBURNER");
		*(unsigned char*)*stream = specialData.afterburner_stage;
		*stream += sizeof (specialData.afterburner_stage);
	}

	dirty_simbase = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SimBaseClass::ReadDirty (uchar **stream)
{
	short	bits;

	bits = *(short*)*stream;
	*stream += sizeof(bits);

	if (bits & DIRTY_SIM_FLAGS)
	{
	//	MonoPrint ("ReadDirty DIRTY_SIM_FLAGS");
		specialData.flags = *(int*)*stream;
		*stream += sizeof(specialData.flags);
	}

	if (bits & DIRTY_SIM_COUNTRY)
	{	//	MonoPrint ("ReadDirty DIRTY_SIM_COUNTRY");
		specialData.country = *(int*)*stream;
		*stream += sizeof(specialData.country);
	}

	if (bits & DIRTY_SIM_CHAFF)
	{	//	MonoPrint ("ReadDirty DIRTY_SIM_CHAFF");
		specialData.ChaffID = *(VU_ID*)*stream;
		*stream += sizeof(specialData.ChaffID);
	}

	if (bits & DIRTY_SIM_FLARE)
	{	//	MonoPrint ("ReadDirty DIRTY_SIM_FLARE");
		specialData.FlareID = *(VU_ID*)*stream;
		*stream += sizeof(specialData.FlareID);
	}

	if (bits & DIRTY_SIM_STATUS)
	{		//MonoPrint ("ReadDirty DIRTY_SIM_STATUS");
		specialData.status = *(long*)*stream;
		*stream += sizeof(specialData.status);

		if (IsAirplane ())
		{
		    AircraftClass *ac = (AircraftClass *)this;
		    // only complex aircraft have these switches.
		    if (ac->IsComplex()) {
			switch(specialData.status & STATUS_EXT_LIGHTMASK) {
			case STATUS_EXT_LIGHTS: // compat - light on
			    ac->SetSwitch(COMP_NAV_LIGHTS, TRUE);
			    ac->SetSwitch(COMP_TAIL_STROBE, TRUE);
			    break;
			case 0: // compat - lights off
			    ac->SetSwitch(COMP_NAV_LIGHTS, FALSE);
			    ac->SetSwitch(COMP_TAIL_STROBE, FALSE);
			    break;
			default:
			    ac->SetSwitch(COMP_NAV_LIGHTS, (specialData.status & STATUS_EXT_NAVLIGHTS));
			    ac->SetSwitch(COMP_TAIL_STROBE, (specialData.status & STATUS_EXT_TAILSTROBE));
			    ac->SetSwitch(COMP_LAND_LIGHTS, (specialData.status & STATUS_EXT_LANDINGLIGHT));
			    break;
			}
		    }
		}
	}

	if (bits & DIRTY_SIM_POWER_OUTPUT)
	{	//	MonoPrint ("ReadDirty DIRTY_SIM_POWER_OUTPUT");
		specialData.powerOutputNet = *(unsigned char*)*stream;
		specialData.powerOutput = (float)specialData.powerOutputNet * 1.5f/255.0f;
		*stream += sizeof(specialData.powerOutputNet);
// 2000-11-17 ADDED BY S.G. I NEED TO READ THE ENGINE TEMP AS WELL
//		specialData.engineHeatOutputNet = *(unsigned char*)*stream;
//		specialData.engineHeatOutput = (float)specialData.engineHeatOutputNet * 1.6f/255.0f;
//		*stream += sizeof(specialData.engineHeatOutputNet);
// END OF ADDED SECTION
	}

	if (bits & DIRTY_SIM_RADAR)
	{		//MonoPrint ("ReadDirty DIRTY_SIM_RADAR");
		specialData.rdrAz = *(float*)*stream;
		*stream += sizeof(specialData.rdrAz);
		specialData.rdrEl = *(float*)*stream;
		*stream += sizeof(specialData.rdrEl);
// This _SHOULD_ be used by the RWR but isn't currently...
//		specialData.rdrAzCenter = *(float*)*stream;
//		*stream += sizeof(specialData.rdrAzCenter);
//		specialData.rdrElCenter = *(float*)*stream;
//		*stream += sizeof(specialData.rdrElCenter);
	}

	if (bits & DIRTY_SIM_RADAR_SLOW)
	{		//MonoPrint ("ReadDirty DIRTY_SIM_RADAR_SLOW");
		specialData.rdrNominalRng = *(float*)*stream;
		*stream += sizeof(specialData.rdrNominalRng);
		specialData.rdrCycleTime = *(float*)*stream;
		*stream += sizeof(specialData.rdrCycleTime);
	}

	if (bits & DIRTY_SIM_AFTERBURNER)
	{	//	MonoPrint ("ReadDirty DIRTY_SIM_AFTERBURNER");
		specialData.afterburner_stage = *(unsigned char*)*stream;
		*stream += sizeof (specialData.afterburner_stage);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SimBaseClass::MakeSimBaseDirty (Dirty_Sim_Base bits, Dirtyness score)
{
	if (!IsLocal())
		return;

	if (VuState() != VU_MEM_ACTIVE)
		return;

//	MonoPrint ("SimBaseDirty %08x %08x\n", bits, score);

	dirty_simbase |= bits;

	MakeDirty (DIRTY_SIM_BASE, score);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SimBaseClass::SetFlag (int flag)
{
	if (!(specialData.flags & flag))
	{
		specialData.flags |= flag;

		MakeSimBaseDirty (DIRTY_SIM_FLAGS, DDP[163].priority);
		//		MakeSimBaseDirty (DIRTY_SIM_FLAGS, SEND_RELIABLE);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SimBaseClass::UnSetFlag (int flag)
{
	if (specialData.flags & flag)
	{
		specialData.flags &= ~(flag);

		MakeSimBaseDirty (DIRTY_SIM_FLAGS, DDP[164].priority);
		//		MakeSimBaseDirty (DIRTY_SIM_FLAGS, SEND_RELIABLE);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SimBaseClass::SetFlagSilent (int flag)
{
	if (!(specialData.flags & flag))
	{
		specialData.flags |= flag;

		//MakeSimBaseDirty (DIRTY_SIM_FLAGS, SEND_NOW);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SimBaseClass::UnSetFlagSilent (int flag)
{
	if (specialData.flags & flag)
	{
		specialData.flags &= ~(flag);

		//MakeSimBaseDirty (DIRTY_SIM_FLAGS, SEND_NOW);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SimBaseClass::SetCountry (int newSide)
{
	specialData.country = newSide;

	MakeSimBaseDirty (DIRTY_SIM_COUNTRY, DDP[165].priority);
	//	MakeSimBaseDirty (DIRTY_SIM_COUNTRY, SEND_SOON);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SimBaseClass::SetNewestChaffID (VU_ID id)
{
	if (specialData.ChaffID != id)
	{
		specialData.ChaffID = id;
		MakeSimBaseDirty (DIRTY_SIM_CHAFF, DDP[166].priority);
		//		MakeSimBaseDirty (DIRTY_SIM_CHAFF, SEND_NOW);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SimBaseClass::SetNewestFlareID (VU_ID id)
{
	if (specialData.FlareID != id)
	{
		specialData.FlareID = id;
		MakeSimBaseDirty (DIRTY_SIM_FLARE, DDP[167].priority);
		//		MakeSimBaseDirty (DIRTY_SIM_FLARE, SEND_NOW);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SimBaseClass::SetStatus (int status)
{
	if (specialData.status != status)
	{
//		MonoPrint ("SetStatus %08x %08x\n", this, status);

		specialData.status = status;

		if (IsAirplane ())
		{
			MakeSimBaseDirty (DIRTY_SIM_STATUS, DDP[168].priority);
			//			MakeSimBaseDirty (DIRTY_SIM_STATUS, SEND_SOON);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SimBaseClass::SetStatusBit (int status)
{
	if (!(specialData.status & status))
	{
//		MonoPrint ("SetStatusBit %08x %08x\n", this, status);

		specialData.status |= status;

		if (IsAirplane ())
		{
			MakeSimBaseDirty (DIRTY_SIM_STATUS, DDP[169].priority);
			//			MakeSimBaseDirty (DIRTY_SIM_STATUS, SEND_RELIABLE);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SimBaseClass::ClearStatusBit (int status)
{
	if (specialData.status & status)
	{
//		MonoPrint ("ClearStatusBit %08x %08x\n", this, status);

		specialData.status &= ~status;

		if (IsAirplane ())
		{
			MakeSimBaseDirty (DIRTY_SIM_STATUS, DDP[170].priority);
			//			MakeSimBaseDirty (DIRTY_SIM_STATUS, SEND_RELIABLE);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SimBaseClass::SetPowerOutput (float powerOutput)
{
	int
		diff,
		value;

	value = FloatToInt32(powerOutput / 1.5f * 255.0f);

	ShiAssert( value >= 0    );
	ShiAssert( value <= 0xFF );

	specialData.powerOutput = powerOutput;

	diff = specialData.powerOutputNet - value;

// 2000-11-17 ADDED BY S.G. SO OTHER VEHICLE USES THE RPM IN THEIR ENGINE TEMP
//	specialData.engineHeatOutput = powerOutput;
// END OF ADDED SECTION

	if ((diff < -16) || (diff > 16))
	{
		//MonoPrint ("%08x SPO %f\n", this, powerOutput);

		specialData.powerOutputNet = static_cast<uchar>(value);
// 2000-11-17 ADDED BY S.G. SO OTHER VEHICLE USES THE RPM IN THEIR ENGINE TEMP
//		specialData.engineHeatOutputNet = static_cast<uchar>(value);
// END OF ADDED SECTION

		MakeSimBaseDirty (DIRTY_SIM_POWER_OUTPUT, DDP[171].priority);
		//		MakeSimBaseDirty (DIRTY_SIM_POWER_OUTPUT, SEND_SOMETIME);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SimBaseClass::SetRdrAz (float az)
{
	if (specialData.rdrAz != az)
	{
		specialData.rdrAz = az;

		MakeSimBaseDirty (DIRTY_SIM_RADAR, DDP[172].priority);
		//		MakeSimBaseDirty (DIRTY_SIM_RADAR, SEND_SOON);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SimBaseClass::SetRdrEl (float el)
{
	if (specialData.rdrEl != el)
	{
		specialData.rdrEl = el;

		MakeSimBaseDirty (DIRTY_SIM_RADAR, DDP[173].priority);
		//		MakeSimBaseDirty (DIRTY_SIM_RADAR, SEND_SOON);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SimBaseClass::SetRdrAzCenter (float az)
{
// This _SHOULD_ be used by the RWR but isn't currently...
//	if (specialData.rdrAzCenter != az)
//	{
//		MakeSimBaseDirty (DIRTY_SIM_RADAR, SEND_SOON);
//	}

	specialData.rdrAzCenter = az;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SimBaseClass::SetRdrElCenter (float el)
{
// This _SHOULD_ be used by the RWR but isn't currently...
//	if (specialData.rdrElCenter != el)
//	{
//		MakeSimBaseDirty (DIRTY_SIM_RADAR, SEND_SOON);
//	}

	specialData.rdrElCenter = el;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SimBaseClass::SetRdrCycleTime (float cycle)
{
	if (specialData.rdrCycleTime != cycle)
	{
		specialData.rdrCycleTime = cycle;

		MakeSimBaseDirty (DIRTY_SIM_RADAR_SLOW, DDP[174].priority);
		//		MakeSimBaseDirty (DIRTY_SIM_RADAR_SLOW, SEND_LATER);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SimBaseClass::SetRdrRng (float rng)
{
	if (specialData.rdrNominalRng != rng)
	{
		specialData.rdrNominalRng = rng;

		MakeSimBaseDirty (DIRTY_SIM_RADAR_SLOW, DDP[175].priority);
		//		MakeSimBaseDirty (DIRTY_SIM_RADAR_SLOW, SEND_LATER);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//void SimBaseClass::SetVt (float vt)
void SimBaseClass::SetVt (float)
{
	// empty
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//void SimBaseClass::SetKias (float kias)
void SimBaseClass::SetKias (float)
{
	// empty
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SimBaseClass::SetAfterburnerStage (int s)
{
	if (specialData.afterburner_stage != s)
	{
//		MonoPrint ("Afterburner %08x %d\n", this, s);

		specialData.afterburner_stage = static_cast<uchar>(s);

		MakeSimBaseDirty (DIRTY_SIM_AFTERBURNER, DDP[176].priority);
		//		MakeSimBaseDirty (DIRTY_SIM_AFTERBURNER, SEND_SOON);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int SimBaseClass::GetAfterburnerStage (void)
{
	return specialData.afterburner_stage;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
