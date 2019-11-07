#include <stddef.h>
#include <fcntl.h>
#include <io.h>
#include "CmpGlobl.h"
#include "CampCell.h"
#include "CampTerr.h"
#include "Listadt.h"
#include "falcgame.h"
#include "CampBase.h"
#include "Team.h"
#include "Weather.h"
#include "CampList.h"
#include "Campaign.h"
#include "Battalion.h"
#include "classtbl.h"
#include "Tacan.h"
#include "falcsess.h"
#include "MsgInc\CampDataMsg.h"
#include "SimBase.h"
#include "Aircrft.h"
#include "dirtybits.h"
#include "uicomms.h"
#include "navsystem.h"
#include "MissEval.h"

// The value vu uses to assign VU_IDs
//extern VU_ID_NUMBER vuAssignmentId;

// ===================================
// Camp base globals
// ===================================

enum
{
	_FORCE_RADAR_OFF_=60,
};

uchar CampSearch[MAX_CAMP_ENTITIES];			// Search data
#ifdef DEBUG
uchar ReservedIds[MAX_CAMP_ENTITIES] = { 0 };	// Reserved data Ids
#endif

#ifdef CAMPTOOL
short CampIDRenameTable[MAX_CAMP_ENTITIES] = { 0 };
#endif

// ===================================
// Name space stuff
// ===================================

VU_ID_NUMBER lastObjectiveId = FIRST_OBJECTIVE_VU_ID_NUMBER;
VU_ID_NUMBER lastNonVolitileId = FIRST_NON_VOLITILE_VU_ID_NUMBER;
VU_ID_NUMBER lastFlightId = FIRST_LOW_VOLITILE_VU_ID_NUMBER;
VU_ID_NUMBER lastPackageId = FIRST_LOW_VOLITILE_VU_ID_NUMBER;
VU_ID_NUMBER lastVolitileId = FIRST_VOLITILE_VU_ID_NUMBER;

extern FILE
	*save_log,
	*load_log;

extern int
	start_save_stream,
	start_load_stream;

// ===================================
// Camp base class functions
// ===================================

CampBaseClass::CampBaseClass (int typeindex) : FalconEntity(typeindex)
	{
	SetTypeFlag(FalconCampaignEntity);
	spotted = base_flags = local_flags = 0;	
	owner = 0;	
	camp_id = FindUniqueID();
	pos_.z_ = 0.0F;
	components = NULL;
	deag_owner = FalconNullId;
	SetAggregate(1);
	SetAssociation(FalconLocalGame->Id());
	dirty_camp_base = 0;
	spotTime = 0; // JB 010719
	}

// This call will create a full entity
CampBaseClass::CampBaseClass (VU_BYTE **stream) : FalconEntity(VU_LAST_ENTITY_TYPE)
	{
	GridIndex	x,y;
	short tmp;

	if (load_log)
	{
		fprintf (load_log, "%08x CampBaseClass ", *stream - start_load_stream);
		fflush (load_log);
	}

	// Read vu stuff here
	memcpy(&share_.id_, *stream, sizeof(VU_ID));			*stream += sizeof(VU_ID);
#ifdef DEBUG
	// VU_ID_NUMBERs moved to 32 bits
	//share_.id_.num_ &= 0xffff;
#endif
	memcpy(&share_.entityType_, *stream, sizeof(ushort));	*stream += sizeof(ushort);
	if (load_log)
	{
		fprintf (load_log, "(%08x%08x %d) ", share_.id_, share_.entityType_);
		fflush (load_log);
	}

	memcpy(&x, *stream, sizeof(GridIndex));					*stream += sizeof(GridIndex);
	memcpy(&y, *stream, sizeof(GridIndex));					*stream += sizeof(GridIndex);
	SetLocation(x,y);

	if (gCampDataVersion < 70)
	{
		pos_.z_ = 0.0F;
	}
	else
	{
		memcpy (&pos_.z_, *stream, sizeof (float));			*stream += sizeof (float);
	}

	SetEntityType(share_.entityType_);
	SetTypeFlag(FalconCampaignEntity);

	memcpy(&spotTime, *stream, sizeof(CampaignTime));		*stream += sizeof(CampaignTime);
	memcpy(&spotted, *stream, sizeof(short));				*stream += sizeof(short);
	memcpy(&tmp, *stream, sizeof(short));					*stream += sizeof(short);
	base_flags = tmp;
	memcpy(&owner, *stream, sizeof(Control));				*stream += sizeof(Control);
	memcpy(&camp_id, *stream, sizeof(short));				*stream += sizeof(short);
	local_flags = CBC_AGGREGATE;
	deag_owner = FalconNullId;
	components = NULL;
	dirty_camp_base = 0;

#ifdef CAMPTOOL
   if (GetEntityByCampID(camp_id))
      {
      MonoPrint("Got duplicate camp ID #%d.\n",camp_id);
      for (int i=0; i<MAX_CAMP_ENTITIES; i++)
         {
         if (!CampIDRenameTable[i])
            {
            CampIDRenameTable[i] = camp_id;
            break;
            }
         }
      }
   if (vuDatabase->Find(Id()))
      {
      MonoPrint("Got duplicate VU_ID #%d.\n",Id().num_);
      }
#endif

#ifdef DEBUG
	// Clear out entities owned by non-existant teams
	// KCK NOTE: This doesn't work in multi-player remote, as we often don't have teams at this point
	if (FalconLocalGame && FalconLocalGame->IsLocal())
		{
		if (!TeamInfo[GetTeam()] || !TeamInfo[GetOwner()])
			{
				for (int i=0; i<NUM_TEAMS && !TeamInfo[i]; i++);
				SetOwner((uchar)i);
			}
		}
#endif

	// Set owner to game master and associate the entity
	ShiAssert (FalconLocalGame);
	if (FalconLocalGame)
		{
		share_.ownerId_ = FalconLocalGame->OwnerId();
		SetAssociation(FalconLocalGame->Id());
		}
	}

CampBaseClass::~CampBaseClass (void)
	{
	if (IsTacan())
		{
		SetTacan(0);
		}
	if (components)
		{
		components->DeInit();
		delete components;
		components = NULL;
		}
	}

int CampBaseClass::SaveSize(void)
	{
	return sizeof(VU_ID)
		+ sizeof(ushort)
		+ sizeof(GridIndex)
		+ sizeof(GridIndex)
		+ sizeof(float)
		+ sizeof(CampaignTime)
		+ sizeof(short)
		+ sizeof(short)
		+ sizeof(Control)
		+ sizeof(short);
	}

int CampBaseClass::Save(VU_BYTE **stream)
	{
	GridIndex	x,y;
	short tmp;

	GetLocation(&x,&y);

	if (save_log)
	{
		fprintf (save_log, "%08x CampBaseClass (%08x%08x %d) ", *stream - start_save_stream, share_.id_, share_.entityType_);
		fflush (save_log);
	}

	// Save VU stuff ourselves to keep from sending unneeded stuff
	memcpy(*stream, &share_.id_, sizeof(VU_ID));			*stream += sizeof(VU_ID);
	memcpy(*stream, &share_.entityType_, sizeof(ushort));	*stream += sizeof(ushort);
	memcpy(*stream, &x, sizeof(GridIndex));					*stream += sizeof(GridIndex);
	memcpy(*stream, &y, sizeof(GridIndex));					*stream += sizeof(GridIndex);
	memcpy(*stream, &pos_.z_, sizeof (float));				*stream += sizeof(float);

	// Now save our stuff
	memcpy(*stream, &spotTime, sizeof(CampaignTime));		*stream += sizeof(CampaignTime);
	memcpy(*stream, &spotted,	sizeof(short));				*stream += sizeof(short);
	tmp = base_flags;
	memcpy(*stream, &tmp,	sizeof(short));					*stream += sizeof(short);
	memcpy(*stream, &owner,	sizeof(Control));				*stream += sizeof(Control);
	memcpy(*stream, &camp_id,	sizeof(short));				*stream += sizeof(short);
	
	return CampBaseClass::SaveSize();
	}

// event handlers
int CampBaseClass::Handle(VuEvent *event)
	{
	return (FalconEntity::Handle(event));
	}

int CampBaseClass::Handle(VuFullUpdateEvent *event)
	{
	GridIndex		x,y;

	// copy data from temp entity to current entity
	CampBaseClass* tmp_ent = (CampBaseClass*)(event->expandedData_);

	// Make sure the host owns this
	share_.ownerId_ = FalconLocalGame->OwnerId();

	// In the case of force on force TE, this is actually ok -
	// The host will receive the full update and MAKE this entity
	// local in the line above
//	ShiAssert ( !IsLocal() );

	memcpy(&share_.entityType_, &tmp_ent->share_.entityType_, sizeof(ushort));
	tmp_ent->GetLocation(&x,&y);
	SetLocation(x,y);
	SetEntityType(share_.entityType_);

	memcpy(&spotTime, &tmp_ent->spotTime, sizeof(CampaignTime));
	memcpy(&spotted, &tmp_ent->spotted, sizeof(short));			
	memcpy(&owner, &tmp_ent->owner, sizeof(Control));
	base_flags = tmp_ent->base_flags;

	return (FalconEntity::Handle(event));
	}

int CampBaseClass::Handle(VuPositionUpdateEvent *event)
	{
	return (FalconEntity::Handle(event));
	}

int CampBaseClass::Handle(VuEntityCollisionEvent *event)
	{
	return (FalconEntity::Handle(event));
	}

int CampBaseClass::Handle(VuTransferEvent *event)
	{
	return (FalconEntity::Handle(event));
	}

int CampBaseClass::Handle(VuSessionEvent *event)
	{
	return (FalconEntity::Handle(event));
	}

void CampBaseClass::SendMessage (VU_ID id, short msg, short d1, short d2, short d3, short d4)
	{
	VuTargetEntity* target = (VuTargetEntity*) vuDatabase->Find(OwnerId());
	FalconCampMessage* cm = new FalconCampMessage(Id(), target);

	cm->dataBlock.from = id;
	cm->dataBlock.message = msg;
	cm->dataBlock.data1 = d1;
	cm->dataBlock.data2 = d2;
	cm->dataBlock.data3 = d3;
	cm->dataBlock.data4 = d4;
	FalconSendMessage(cm,TRUE);
	}

void CampBaseClass::BroadcastMessage (VU_ID id, short msg, short d1, short d2, short d3, short d4)
	{
	FalconCampMessage* cm = new FalconCampMessage(Id(), FalconLocalGame);

	cm->dataBlock.from = id;
	cm->dataBlock.message = msg;
	cm->dataBlock.data1 = d1;
	cm->dataBlock.data2 = d2;
	cm->dataBlock.data3 = d3;
	cm->dataBlock.data4 = d4;
	FalconSendMessage(cm,TRUE);
	}

// JPO - change to return vu error code - well why not.
VU_ERRCODE CampBaseClass::Remove (void)
	{
	if (IsTacan())
		{
		SetTacan(0);
		}
	return vuDatabase->Remove(this);
	}

// Getters
int CampBaseClass::GetSpotted (Team t)
	{
	MoveType	mt = GetMovementType();

	if (Camp_GetCurrentTime() - spotTime > ReconLossTime[mt])
	{
		spotted = 0;
	}
	ShiAssert(TeamInfo[t]); // 2002-03-06 MN try to cath this
	if ((spotted >> t) & 0x01)
		return 1;
	switch (mt)
		{
		case Air:
		case LowAir:
		case Foot:
			return 0;
			break;
		case Tracked:
		case Wheeled:
		case Rail:
			return 0;
			break;
		case Naval:
			if (TeamInfo[t] && TeamInfo[t]->HasSatelites()) // 2002-03-06 MN CTD fix
				{
				// Check for cloud cover
				GridIndex	x,y;
				GetLocation(&x,&y);
				if (((WeatherClass*)TheWeather)->GetCloudCover(x,y) < (MAX_CLOUD_TYPE-(MAX_CLOUD_TYPE/4)))
					{
					SetSpotted(t,Camp_GetCurrentTime());
					return 1;
					}
				}
			break;
		default:
			// KCK: Experimental - Autospotted during first 12 hours of combat
			if (Camp_GetCurrentTime() < CampaignDay/2)
				return 1;
			else if (TeamInfo[t] && TeamInfo[t]->HasSatelites()) // 2002-03-06 MN CTD fix
				{
				// Check for cloud cover
				GridIndex	x,y;
				GetLocation(&x,&y);
				if (((WeatherClass*)TheWeather)->GetCloudCover(x,y) < (MAX_CLOUD_TYPE-(MAX_CLOUD_TYPE/4)))
					{
					SetSpotted(t,Camp_GetCurrentTime());
					return 1;
					}
				}
			break;
		}
	return 0;
	}

// Setters
void CampBaseClass::SetLocation (GridIndex x, GridIndex y)
{
	GridIndex	cx,cy;

	// Check if flight has moved, and evaluate current situation if so
	GetLocation(&cx,&cy);
	if (cx != x || cy != y)
	{
		vector		v;

//		ShiAssert (x >= 0 && y >= 0 && x < Map_Max_X && y < Map_Max_Y)
		if ((x < 1) || (x >= Map_Max_X-1) || (y < 1) || (y >= Map_Max_Y-1))
			{
			// KCK Hack: Teleport units off map back onto map
			if (x < 1)
				x = 1;
			if (y < 1)
				y = 1;
			if (x >= Map_Max_X-1)
				x = (GridIndex)(Map_Max_X - 2);
			if (y >= Map_Max_Y-1)
				y = (GridIndex)(Map_Max_Y - 2);
			}
			
		ConvertGridToSim(x,y,&v);
		SetPosition(v.x,v.y,ZPos());

//		MakeCampBaseDirty (DIRTY_POSITION, SEND_SOON);
		MakeCampBaseDirty (DIRTY_POSITION, DDP[0].priority);
	}
}

void CampBaseClass::SetAltitude (int alt)
	{
	SetPosition(XPos(),YPos(),(float)(alt*-1));

	MakeCampBaseDirty (DIRTY_ALTITUDE, DDP[1].priority);
//	MakeCampBaseDirty (DIRTY_ALTITUDE, SEND_SOON);
	}

void CampBaseClass::SetSpotted (Team t, CampaignTime time, int identified) // 2002-02-11 ADDED S.G. Added identified which defaults to 1
{
	// Make this dirty if we wern't previously spotted or our time has expired
	if (ReSpot() || !((spotted >> t) & 0x01) || (!((spotted >> (t + 8)) & 0x01) && identified)) // 2002-02-11 MODIFIED BY S.G. Or we were not identified and now we are
	{
		spotTime = time;
		// 2002-04-02 ADDED BY S.G. Need to send sooner if it gets identified.
		if (!((spotted >> (t + 8)) & 0x01) && identified)
			MakeCampBaseDirty (DIRTY_SPOTTED, DDP[2].priority);
//			MakeCampBaseDirty (DIRTY_SPOTTED, SEND_RELIABLE);
		else
		// END OF ADDED SECTION
			MakeCampBaseDirty (DIRTY_SPOTTED, DDP[3].priority);
//			MakeCampBaseDirty (DIRTY_SPOTTED, SEND_SOMETIME);
	}
	
	spotted |= (0x01 << t);

	// 2002-02-11 ADDED BY S.G. The upper 8 bits of spotted is now used to know if the target has been identified by that team.
	spotted |= ((identified & 0x01) << (t + 8)); // The only time you lose your identification is when you lose your spotting
}

void CampBaseClass::SetEmitting (int e)
{
//	if (IsObjective())
//	{
//		if(((Objective)this)->GetObjectiveStatus() < _FORCE_RADAR_OFF_)
//		{
//			e=0;
//		}
//	}
	if (e)
	{
		if (!IsEmitting())
		{
			base_flags |= CBC_EMITTING;
			MakeCampBaseDirty (DIRTY_BASE_FLAGS, DDP[4].priority);
//			MakeCampBaseDirty (DIRTY_BASE_FLAGS, SEND_SOMETIME);
		}

		if (!EmitterList->Find(Id()))
		{
			if (IsBattalion() || IsObjective())
				EmitterList->ForcedInsert(this);
			if (GetRadarMode() == FEC_RADAR_OFF)
				SetRadarMode(FEC_RADAR_SEARCH_1);//me123 + rand()%3);
//			ReturnToSearch();
		}
	}
	else if (IsEmitting())
	{		base_flags &= ~CBC_EMITTING;
		if (IsBattalion() || IsObjective())
			EmitterList->Remove(this);
		SetRadarMode(FEC_RADAR_OFF);
		MakeCampBaseDirty (DIRTY_BASE_FLAGS, DDP[5].priority);
//		MakeCampBaseDirty (DIRTY_BASE_FLAGS, SEND_SOMETIME);
	}
}

void CampBaseClass::SetAggregate (int a)
   {
   local_flags |= CBC_AGGREGATE;
   if (!a)
      local_flags ^= CBC_AGGREGATE;
   }

void CampBaseClass::SetJammed (int j)
{
	if (j)
	{
		if (!(base_flags & CBC_JAMMED))
		{
			base_flags |= CBC_JAMMED;
			MakeCampBaseDirty (DIRTY_BASE_FLAGS, DDP[6].priority);
//			MakeCampBaseDirty (DIRTY_BASE_FLAGS, SEND_SOMETIME);
		}
	}
	else
	{
		if (base_flags & CBC_JAMMED)
		{
			base_flags &= ~CBC_JAMMED;
			MakeCampBaseDirty (DIRTY_BASE_FLAGS, DDP[7].priority);
//			MakeCampBaseDirty (DIRTY_BASE_FLAGS, SEND_SOMETIME);
		}
	}
}

void CampBaseClass::SetTacan (int t)
   {

	if(!t && IsTacan() && gTacanList)
		{
			if(IsObjective() && GetType() == TYPE_AIRBASE) 
			{
				gTacanList->RemoveTacan(Id(), NavigationSystem::AIRBASE);
			}
			else if(EntityType()->classInfo_[VU_CLASS] == CLASS_UNIT && EntityType()->classInfo_[VU_TYPE] == TYPE_FLIGHT)
			{
				gTacanList->RemoveTacan(Id(), NavigationSystem::TANKER);
			}
			else if(EntityType()->classInfo_[VU_CLASS] == CLASS_UNIT && EntityType()->classInfo_[VU_TYPE] == TYPE_TASKFORCE && EntityType()->classInfo_[VU_STYPE])
			{
				gTacanList->RemoveTacan(Id(), NavigationSystem::CARRIER);
			}
		}
	else if(t && (!IsTacan() || (IsObjective() && GetType() == TYPE_AIRBASE)) && gTacanList)
		{
		gTacanList->AddTacan(this);
		}

   local_flags |= CBC_HAS_TACAN;
   if (!t)
      local_flags ^= CBC_HAS_TACAN;
   }
/*
void CampBaseClass::SetChecked (int t)
   {
   local_flags |= CBC_CHECKED;
   if (!t)
      local_flags ^= CBC_CHECKED;
   }
*/
void CampBaseClass::SetAwake (int d)
	{
	local_flags |= CBC_AWAKE;
	if (!d)
		local_flags ^= CBC_AWAKE;
	}

void CampBaseClass::SetInPackage (int p)
	{
	local_flags |= CBC_IN_PACKAGE;
	if (!p)
	   local_flags ^= CBC_IN_PACKAGE;
	}

void CampBaseClass::SetDelta (int d)
	{
	local_flags |= CBC_HAS_DELTA;
	if (!d)
	   local_flags ^= CBC_HAS_DELTA;
	}

void CampBaseClass::SetInSimLists (int l)
	{
	local_flags |= CBC_IN_SIM_LIST;
	if (!l)
	   local_flags ^= CBC_IN_SIM_LIST;
	}

void CampBaseClass::SetReserved (int r)
	{
	local_flags |= CBC_RESERVED_ONLY;
	if (!r)
	   local_flags ^= CBC_RESERVED_ONLY;
	}

int CampBaseClass::ReSpot (void)
	{
	if (Camp_GetCurrentTime() - spotTime > ReconLossTime[GetMovementType()]/2)
		return 1;
	return 0;
	}

// Component accessers (Sim Flight emulators)
int CampBaseClass::GetComponentIndex (VuEntity* me)							// My call
	{
	if (!components)
		return 0;
	else
		{	
		VuListIterator	cit(components);
		int				idx = 0;
		VuEntity		*cur;
		
		cur = cit.GetFirst();
		while (cur && cur != me)
			{
			idx ++;
			cur = cit.GetNext();
			}

		return idx;
		}
	}

SimBaseClass* CampBaseClass::GetComponentEntity (int idx)					// My call
	{
	if (components)
		{
		VuListIterator	cit(components);
		int				count = 0;
		VuEntity		*cur;

		cur = cit.GetFirst();
		while (cur)
			{
			if (count == idx)
				return (SimBaseClass*)cur;
			count++;
			cur = cit.GetNext();
			}
		}
	return NULL;
	}

SimBaseClass* CampBaseClass::GetComponentLead(void)							// My call
	{
	if (components)
		{
		VuListIterator	cit(components);
		return (SimBaseClass*) cit.GetFirst();
		}
	return NULL;
	}

SimBaseClass* CampBaseClass::GetComponentNumber (int component)
	{
	// components shouldn't be null if this camp unit is deagg'd!
	if (components)
		{
		VuListIterator	cit(components);
		SimBaseClass	*entity;

		entity = (SimBaseClass *)cit.GetFirst();
		while( entity )
			{	
			// if it's our requested component, return it.
			// 'component' means aircraft # for Aircraft, but otherwise just slot number
			if (entity->IsAirplane())
				{
				if (((AircraftClass*)entity)->vehicleInUnit == component)
					return entity;
				}
			else if (entity->GetSlot() == component)
				return entity;

			entity = (SimBaseClass *)cit.GetNext();
			}
		}
	return NULL;
	}

int CampBaseClass::NumberOfComponents(void)									// My call
	{
	int					count = 0;
	// I hate to do this - but it's next to impossible to keep an accurate count
	// when VU can pull these guys out from under me.
	if (components)
		{
		VuEntity*			cur;
		VuListIterator		cit(components);

		cur = cit.GetFirst();
		while (cur)
			{
			count++;
			cur = cit.GetNext();
			}
		}
	return count;
	}

FalconSessionEntity* CampBaseClass::GetDeaggregateOwner (void)
	{
	return (FalconSessionEntity*) vuDatabase->Find(deag_owner);
	}

// ===========================
// Global functions
// ===========================

CampEntity GetFirstEntity (F4LIt l)
	{
	VuEntity*	e;

	e = l->GetFirst();
	while (e)
		{
		if (e->VuState() != VU_MEM_DELETED)
			if (GetEntityClass(e) == CLASS_UNIT || GetEntityClass(e) == CLASS_OBJECTIVE)
				return (CampEntity)e;
		e = l->GetNext();
		}
	return NULL;
	}
		
CampEntity GetNextEntity (F4LIt l)
   {
	VuEntity*	e;

	e = l->GetNext();
	while (e)
		{
		if (e->VuState() != VU_MEM_DELETED)
			if (GetEntityClass(e) == CLASS_UNIT || GetEntityClass(e) == CLASS_OBJECTIVE)
				return (CampEntity)e;
		e = l->GetNext();
		}
	return NULL;
   }

int Real (int type)
	{
	if (type == TYPE_BATTALION || type == TYPE_FLIGHT || type == TYPE_TASKFORCE)
		return 1;
	return 0;
	}

short GetEntityClass (VuEntity* e)
	{
	if (!e)
		return 0;
	return (e->EntityType())->classInfo_[VU_CLASS];
	}

short GetEntityDomain (VuEntity* e)
	{
	if (!e)
		return 0;
	return (e->EntityType())->classInfo_[VU_DOMAIN];
	}

Unit GetEntityUnit (VuEntity* e)
	{
	if (GetEntityClass(e) == CLASS_UNIT)
		return (Unit)e;
	return NULL;
	}

Objective GetEntityObjective (VuEntity* e)
	{
	if (GetEntityClass(e) == CLASS_OBJECTIVE)
		return (Objective)e;
	return NULL;
	}


// My global for last assigned id
short	gLastId = 32767;

short FindUniqueID (void)
	{
	VuListIterator	myit(AllCampList);
	CampEntity		e;
	short			id,eid;

	if (gLastId < MAX_CAMP_ENTITIES-1)
		{
		// simple algorythm to find a unique id
		gLastId++;
		id = gLastId;
		return id;
		}
	else
		{
		// more complex algorythm if we're out of space
		short		highest=0;
		memset(CampSearch,0,sizeof(uchar)*MAX_CAMP_ENTITIES);
		e = (CampEntity) myit.GetFirst();
		while (e)
			{
			eid = e->GetCampID();
			ShiAssert (eid < MAX_CAMP_ENTITIES);
			CampSearch[eid] = 1;
			if (e->GetCampID() > highest)
				{
				gLastId = e->GetCampID();
				highest = gLastId;
				}
			e = (CampEntity) myit.GetNext();
			}
		for (id=1; id < MAX_CAMP_ENTITIES; id++)
			{
#ifdef DEBUG
			if (!CampSearch[id] && !ReservedIds[id])
				return id;
#else
			if (!CampSearch[id])
				return id;
#endif
			}
		MonoPrint("Error! Exceeded max entity count!\n");
		}
	return 0;
	}

void ResetNamespace (void)
	{
	VuEnterCriticalSection();
	lastObjectiveId = FIRST_OBJECTIVE_VU_ID_NUMBER;
	lastNonVolitileId = FIRST_NON_VOLITILE_VU_ID_NUMBER;
	lastPackageId = FIRST_LOW_VOLITILE_VU_ID_NUMBER;
	lastFlightId = (FIRST_LOW_VOLITILE_VU_ID_NUMBER + LAST_LOW_VOLITILE_VU_ID_NUMBER)/2;
	lastVolitileId = FIRST_VOLITILE_VU_ID_NUMBER;
	VuExitCriticalSection();
	}

int GetVisualDetectionRange (int mt)
	{
	int		dr;

	dr = VisualDetectionRange[mt];
// 2001-04-10 MODIFIED BY S.G. TimeOfDay RETURNS THE TIME OF THE DAY IN MILISECONDS! THAT'S NOT WHAT WE WANT... TimeOfDayGeneral WILL DO WHAT WE WANT
//	switch (TimeOfDay())			// Time of day modifiers
	switch (TimeOfDayGeneral())		// Time of day modifiers
		{
		case TOD_NIGHT:
			dr = (dr+3)/4;
			break;
		case TOD_DAWNDUSK:
			dr = (dr+1)/2;
			break;
		default:
			break;
		}
	return dr;
	}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void CampBaseClass::SetOwner (Control new_owner)
{
	owner = new_owner;
	//MakeCampBaseDirty (DIRTY_OWNER, 1);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void CampBaseClass::SetComponents (TailInsertList *new_list)
{
	components = new_list;
	//MakeCampBaseDirty (DIRTY_COMPONENTS, 1);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void CampBaseClass::SetDeagOwner (VU_ID new_id)
{
	deag_owner = new_id;
	//MakeCampBaseDirty (DIRTY_DEAG_OWNER, 1);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void CampBaseClass::SetCampId (short new_camp_id)
{
	camp_id = new_camp_id;
	//MakeCampBaseDirty (DIRTY_CAMP_ID, 1);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void CampBaseClass::SetBaseFlags (short flags)
{
	if (base_flags != flags)
	{
		base_flags = flags;
		MakeCampBaseDirty (DIRTY_BASE_FLAGS, DDP[8].priority);
//		MakeCampBaseDirty (DIRTY_BASE_FLAGS, SEND_EVENTUALLY);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void CampBaseClass::MakeCampBaseDirty (Dirty_Campaign_Base bits, Dirtyness score)
{
	if (!IsLocal())
		return;

	if (VuState() != VU_MEM_ACTIVE)
		return;


	if (!IsAggregate())
	{
		score = (Dirtyness) ((int) score * 10);
	}

	dirty_camp_base |= bits;

	MakeDirty (DIRTY_CAMPAIGN_BASE, score);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void CampBaseClass::WriteDirty (unsigned char **stream)
{
	unsigned char
		*start,
		*ptr;

	start = *stream;
	ptr = start;

//	MonoPrint ("CB %08x\n", dirty_camp_base);

	// Encode it up

	*ptr = (unsigned char) dirty_camp_base;
	ptr += sizeof (unsigned char);

	if (dirty_camp_base & DIRTY_POSITION)
	{
		GridIndex
			x,
			y;

		GetLocation(&x,&y);

		*(short*)ptr = x;
		ptr += sizeof (short);
		*(short*)ptr = y;
		ptr += sizeof (short);
	}

	if (dirty_camp_base & DIRTY_ALTITUDE)
	{
		*(float*)ptr = ZPos();
		ptr += sizeof (float);
	}

	if (dirty_camp_base & DIRTY_SPOTTED)
	{
		*(short*)ptr = spotted;
		ptr += sizeof (short);
		*(CampaignTime*)ptr = spotTime;
		ptr += sizeof (CampaignTime);
	}

	if (dirty_camp_base & DIRTY_BASE_FLAGS)
	{
		*(short*)ptr = base_flags;
		ptr += sizeof (short);
	}

	dirty_camp_base = 0;

	*stream = ptr;

	//MonoPrint ("(%d)", *stream - start);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void CampBaseClass::ReadDirty (unsigned char **stream)
{
	unsigned char
		*start,
		*ptr,
		bits;

	start = *stream;
	ptr = start;

	bits = *ptr;
	ptr += sizeof (unsigned char);

	//MonoPrint ("  CB %08x", bits);

	if (bits & DIRTY_POSITION)
	{
		GridIndex
			x,
			y;

		x = *(short*) ptr;
		ptr += sizeof (short);

		y = *(short*) ptr;
		ptr += sizeof (short);

		vector		v;

		ConvertGridToSim(x,y,&v);
		SetPosition(v.x,v.y,ZPos());
		if (IsFlight ())
		{
			TheCampaign.MissionEvaluator->RegisterMove((Flight)this);
		}
	}

	if (bits & DIRTY_ALTITUDE)
	{
		float		z;

		z = *(float*) ptr;
		ptr += sizeof (float);
		SetPosition(XPos(),YPos(),z);
		if (IsFlight ())
		{
			TheCampaign.MissionEvaluator->RegisterMove((Flight)this);
		}
	}

	if (bits & DIRTY_SPOTTED)
	{
		spotted = *(short*)ptr;
		ptr += sizeof (short);
		spotTime = *(CampaignTime*)ptr;
		ptr += sizeof (CampaignTime);
	}

	if (bits & DIRTY_BASE_FLAGS)
	{
		base_flags = *(short*)ptr;
		ptr += sizeof (short);
	}

	*stream = ptr;

	//MonoPrint ("(%d)", *stream - start);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

