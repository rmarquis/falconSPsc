
#ifndef CAMPBASE_H
#define CAMPBASE_H

#include <tchar.h>
#include "Entity.h"
#include "FalcLib.h"
#include "F4Vu.h" 
#include "MsgInc\CampMsg.h"
#include "MsgInc\DeathMessage.h"
#include "MsgInc\CampWeaponFireMsg.h"
#include "FalcEnt.h"

// ===================================
// Camp base defines
// ===================================

// ===================================
// Base class flags
// ===================================

// Transmittable
#define CBC_EMITTING		0x01
#define CBC_JAMMED			0x04

// Local
#define CBC_CHECKED			0x001			// Used by mission planning to prevent repeated targetting
#define CBC_AWAKE			0x002			// Deaggregated on local machine
#define CBC_IN_PACKAGE		0x004			// This item is in our local package (only applicable to flights)
#define CBC_HAS_DELTA		0x008
#define CBC_IN_SIM_LIST		0x010			// In the sim's nearby campaign entity lists
#define CBC_INTEREST		0x020			// Some session still is interested in this entity
#define CBC_RESERVED_ONLY	0x040			// This entity is here only in order to reserve namespace
#define CBC_AGGREGATE		0x080
#define CBC_HAS_TACAN		0x100

// ===================================
// Name space shit
// ===================================

#define FIRST_OBJECTIVE_VU_ID_NUMBER	(VU_FIRST_ENTITY_ID)
#define LAST_OBJECTIVE_VU_ID_NUMBER		(VU_FIRST_ENTITY_ID+MAX_NUMBER_OF_OBJECTIVES)
#define FIRST_NON_VOLITILE_VU_ID_NUMBER	(LAST_OBJECTIVE_VU_ID_NUMBER+1)
#define LAST_NON_VOLITILE_VU_ID_NUMBER	(FIRST_NON_VOLITILE_VU_ID_NUMBER+(MAX_NUMBER_OF_UNITS))
#define FIRST_LOW_VOLITILE_VU_ID_NUMBER	(LAST_NON_VOLITILE_VU_ID_NUMBER+1)
#define LAST_LOW_VOLITILE_VU_ID_NUMBER	(FIRST_LOW_VOLITILE_VU_ID_NUMBER+(MAX_NUMBER_OF_VOLITILE_UNITS))
#define FIRST_VOLITILE_VU_ID_NUMBER		(LAST_LOW_VOLITILE_VU_ID_NUMBER+1)
#define LAST_VOLITILE_VU_ID_NUMBER		~((VU_ID_NUMBER)0);

extern VU_ID_NUMBER vuAssignmentId;
extern VU_ID_NUMBER vuLowWrapNumber;
extern VU_ID_NUMBER vuHighWrapNumber;
extern VU_ID_NUMBER lastObjectiveId;
extern VU_ID_NUMBER lastNonVolitileId;
extern VU_ID_NUMBER lastLowVolitileId;
extern VU_ID_NUMBER lastFlightId;
extern VU_ID_NUMBER lastPackageId;
extern VU_ID_NUMBER lastVolitileId;

// ===================================
// Camp base globals
// ===================================

extern uchar CampSearch[MAX_CAMP_ENTITIES];	// Search data - Could reduce to bitwise

// ===================================
// Camp base class
// ===================================

class TailInsertList;
class CampBaseClass;
typedef CampBaseClass* CampEntity;
class ObjectiveClass;
typedef ObjectiveClass* Objective;
class FalconSessionEntity;
extern Team GetTeam (Control country);
class SimBaseClass;

class CampBaseClass : public FalconEntity 
	{
 	private:
		CampaignTime		spotTime;		// Last time this entity was spotted
		short				spotted;		// Bitwise array of spotting data, by team
		volatile short		base_flags;		// Various user flags
		short				camp_id;		// Unique campaign id
		Control          	owner;			// Controlling Country
		// Don't transmit below this line
		volatile short		local_flags;	// Non transmitted flags
		TailInsertList		*components;	// List of deaggregated sim entities
		VU_ID				deag_owner;		// Owner of deaggregated components
		VU_ID				new_deag_owner;	// Who is most interrested in this guy
		int					dirty_camp_base;

	public:
		// Access Functions
		CampaignTime GetSpotTime (void)				{	return spotTime; }
		short GetSpotted (void)						{	return spotted; }
		short GetBaseFlags (void)					{	return base_flags; }
		short GetCampId (void)						{	return camp_id; }
		short GetLocalFlags (void)					{	return local_flags; }
		TailInsertList *GetComponents (void)		{	return components; }
		VU_ID GetDeagOwner (void)					{	return deag_owner; }

		void SetBaseFlags (short);
		virtual void SetOwner (Control);
		void SetCampId (short);
		void SetLocalFlags (void);
		void SetComponents (TailInsertList *);
		void SetDeagOwner (VU_ID);

		// Dirty Functions
		void MakeCampBaseDirty (Dirty_Campaign_Base bits, Dirtyness score);
		void WriteDirty (unsigned char **stream);
		void ReadDirty (unsigned char **stream);

		// Constructors and serial functions
		CampBaseClass (int typeindex);
		CampBaseClass (VU_BYTE **stream);
		virtual ~CampBaseClass (void);
		virtual int SaveSize (void);
		virtual int Save (VU_BYTE **stream);

		// event handlers
		virtual int Handle(VuEvent *event);
		virtual int Handle(VuFullUpdateEvent *event);
		virtual int Handle(VuPositionUpdateEvent *event);
		virtual int Handle(VuEntityCollisionEvent *event);
		virtual int Handle(VuTransferEvent *event);
		virtual int Handle(VuSessionEvent *event);

		// Required pure virtuals
		virtual void SendDeaggregateData (VuTargetEntity *)	{}
		virtual int RecordCurrentState (FalconSessionEntity*, int)	{	return 0; }
		virtual int Deaggregate (FalconSessionEntity*)		{	return 0; }
		virtual int Reaggregate (FalconSessionEntity*)		{	return 0; }
		virtual int TransferOwnership (FalconSessionEntity*){	return 0; }
		virtual int Wake (void)				{	return 0; }
		virtual int Sleep (void)			{	return 0; }
		virtual void InsertInSimLists (float , float){}
		virtual void RemoveFromSimLists (void)						{}
		virtual void DeaggregateFromData (int, uchar*)	{	return; }
		virtual void ReaggregateFromData (int, uchar*)	{	return; }
		virtual void TransferOwnershipFromData (int, uchar*)		{	return; }
		virtual int ApplyDamage (FalconCampWeaponsFire *,uchar)	{	return 0; }
		virtual int ApplyDamage (DamType, int*, int, short)			{	return 0; }
		virtual int DecodeDamageData (uchar*, Unit, FalconDeathMessage*)	{	return 0; }
		virtual int CollectWeapons(uchar*, MoveType, short [], uchar [], int)	{	return 0; }
		virtual _TCHAR* GetName (_TCHAR*, int, int)					{	return "None"; }
		virtual _TCHAR* GetFullName (_TCHAR*, int, int)				{	return "None"; }
		virtual _TCHAR* GetDivisionName (_TCHAR*, int, int)			{	return "None"; }
		virtual int GetHitChance (int, int)				{	return 0; }
		virtual int GetAproxHitChance (int, int)			{	return 0; }
		virtual int GetCombatStrength (int, int)			{	return 0; }
		virtual int GetAproxCombatStrength (int, int)		{	return 0; }
		virtual int GetWeaponRange (int, FalconEntity *target = NULL)			{	return 0; } // 2008-03-08 ADDED SECOND DEFAULT PARM
		virtual int GetAproxWeaponRange (int)					{	return 0; }
		virtual int GetDetectionRange (int)						{	return 0; }			// Takes into account emitter status
		virtual int GetElectronicDetectionRange (int)			{	return 0; }			// Full range, regardless of emitter
		virtual int CanDetect (FalconEntity*)					{	return 0; }			// Nonzero if this entity can see ent
      	virtual int OnGround (void)									{	return FALSE; }
      	virtual float Vt (void)										{	return 0.0F; }
      	virtual float Kias (void)									{	return 0.0F; }
		virtual short GetCampID (void)								{	return camp_id; }
		virtual uchar GetTeam (void)								{	return ::GetTeam(owner); }
		virtual uchar GetCountry (void)								{	return owner; }		// New FalcEnt friendly form
		virtual int StepRadar (int t,int d, float range)								{	return FEC_RADAR_OFF; }
		Control GetOwner (void)										{	return owner; }			// Old form

		// These are only really relevant for sam/airdefense/radar entities
		virtual int GetNumberOfArcs (void)							{	return 1; }
		virtual float GetArcRatio (int)						{	return 0.0F; }
		virtual float GetArcRange (int)						{	return 0.0F; }
		virtual void GetArcAngle (int, float* a1, float *a2)	{	*a1 = 0.0F; *a2 = 2*PI; }

		// Core functions
		void SendMessage (VU_ID id, short msg, short d1, short d2, short d3, short d4);
		void BroadcastMessage (VU_ID id, short msg, short d1, short d2, short d3, short d4);
		VU_ERRCODE Remove (void);
		int ReSpot (void);
		FalconSessionEntity* GetDeaggregateOwner (void);

		// Component accessers (Sim Flight emulators)
		int GetComponentIndex (VuEntity* me);						
		SimBaseClass* GetComponentEntity (int idx);					
		SimBaseClass* GetComponentLead(void);						
		SimBaseClass* GetComponentNumber (int component);
		int NumberOfComponents(void);								
		uchar Domain(void)											{	return GetDomain();	}

		// Queries
		virtual int IsEmitting (void)				{	return base_flags & CBC_EMITTING; }
		int IsJammed (void)							{	return base_flags & CBC_JAMMED; }
		// Local flag access
		int IsChecked (void)						{	return local_flags & CBC_CHECKED; }
		int IsAwake (void)							{	return local_flags & CBC_AWAKE; }
		int InPackage (void)						{	return local_flags & CBC_IN_PACKAGE; }		
		int InSimLists (void)						{	return local_flags & CBC_IN_SIM_LIST; }
		int IsInterested (void)						{	return local_flags & CBC_INTEREST; }
		int	IsReserved (void)						{	return local_flags & CBC_RESERVED_ONLY; }
		int IsAggregate (void)						{	return local_flags & CBC_AGGREGATE; }
		int IsTacan (void)							{	return local_flags & CBC_HAS_TACAN; }
		int HasDelta (void)							{	return local_flags & CBC_HAS_DELTA; }

		// Getters
		uchar GetDomain (void)						{	return (EntityType())->classInfo_[VU_DOMAIN]; }
		uchar GetClass (void)						{	return (EntityType())->classInfo_[VU_CLASS]; }
		uchar GetType (void)						{	return (EntityType())->classInfo_[VU_TYPE]; }
		uchar GetSType (void)						{	return (EntityType())->classInfo_[VU_STYPE]; }
		uchar GetSPType (void)						{	return (EntityType())->classInfo_[VU_SPTYPE]; }
		CampaignTime GetSpottedTime (void)			{	return spotTime; }
		int GetSpotted (Team t);
		int GetIdentified (Team t)					{   return (spotted >> (t + 8)) & 0x01; } // 2002-02-11 ADDED BY S.G. Getter to know if the target is identified or not.

		// Setters
		void SetLocation (GridIndex x, GridIndex y);
		void SetAltitude (int alt);
		void SetSpottedTime (CampaignTime t)		{	spotTime = t; }
		void SetSpotted (Team t, CampaignTime time, int identified = 0); // 2002-02-11 ADDED S.G. Added identified which defaults to 0 (not identified or don't change)
		void SetEmitting (int e);
		void SetAggregate (int a);
		void SetJammed (int j);	
		void SetTacan (int t);
		void SetChecked (void)						{	local_flags |= CBC_CHECKED; }
		void UnsetChecked (void)					{	local_flags &= ~CBC_CHECKED; }
		void SetInterest (void)						{	local_flags |= CBC_INTEREST; }
		void UnsetInterest (void)					{	local_flags &= ~CBC_INTEREST; }
		void SetAwake (int d);
		void SetInPackage (int p);
		void SetDelta (int d);
		void SetInSimLists (int l);
		void SetReserved (int r);
	};

// ===========================
// Global functions
// ===========================

CampEntity GetFirstEntity (F4LIt list);

CampEntity GetNextEntity (F4LIt list);

int Parent (CampEntity e);

int Real (int type);

short GetEntityClass (VuEntity* h);

short GetEntityDomain (VuEntity* h);

Unit GetEntityUnit (VuEntity* h);

Objective GetEntityObjective (VuEntity* h);

short FindUniqueID (void);

int GetVisualDetectionRange (int mt);

#endif
