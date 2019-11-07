// vuentity.h - Copyright (c) Tue July 2 15:01:01 1996,  Spectrum HoloByte, Inc.  All Rights Reserved

#ifndef _VUENTITY_H_
#define _VUENTITY_H_

extern "C" {
#include <stdio.h>
}

#include "apitypes.h"
#include "vu.h"

#define MAX_VU_STR_LEN	255

#define VU_UNKNOWN_ENTITY_TYPE			0
#define VU_SESSION_ENTITY_TYPE			1
#define VU_GROUP_ENTITY_TYPE			2
#define VU_GLOBAL_GROUP_ENTITY_TYPE	        3
#define VU_GAME_ENTITY_TYPE             	4
#define VU_PLAYER_POOL_GROUP_ENTITY_TYPE	5
#define VU_LAST_ENTITY_TYPE			100

#define VU_CREATE_PRIORITY_BASE		100

#define CLASS_NUM_BYTES                 8

// domains
#define VU_GLOBAL_DOMAIN		0

// Predefined entity ids
#define VU_NULL_ENTITY_ID		0
#define VU_GLOBAL_GROUP_ENTITY_ID	1
#define VU_PLAYER_POOL_ENTITY_ID	2
#define VU_SESSION_ENTITY_ID	        3
#define VU_FIRST_ENTITY_ID		4       // first generated

// Standard Collision type
#define VU_NON_COLLIDABLE		0
#define VU_DEFAULT_COLLIDABLE		1

// forward decls
class VuEntity;
class VuFilter;
class VuDriver;
class VuErrorMessage;
class VuPushRequest;
class VuPullRequest;
class VuEvent;
class VuCreateEvent;
class VuDeleteEvent;
class VuFullUpdateEvent;
class VuPositionUpdateEvent;
class VuEntityCollisionEvent;
class VuTransferEvent;
class VuSessionEvent;
class VuSessionEntity;

struct VuEntityType {
  ushort id_;
  ushort collisionType_;
  SM_SCALAR collisionRadius_;
  VU_BYTE classInfo_[CLASS_NUM_BYTES];
  VU_TIME updateRate_;
  VU_TIME updateTolerance_;
  SM_SCALAR fineUpdateRange_;	// max distance to send position updates
  SM_SCALAR fineUpdateForceRange_; // distance to force position updates
  SM_SCALAR fineUpdateMultiplier_; // multiplier for noticing position updates
  VU_DAMAGE damageSeed_;
  int hitpoints_;
  ushort majorRevisionNumber_;
  ushort minorRevisionNumber_;
  ushort createPriority_;
  VU_BYTE managementDomain_;
  VU_BOOL transferable_;
  VU_BOOL private_;
  VU_BOOL tangible_;
  VU_BOOL collidable_;
  VU_BOOL global_;
  VU_BOOL persistent_;
};

struct VuFlagBits {
  uint private_		: 1;	// 1 --> not public
  uint transfer_	: 1;	// 1 --> can be transferred
  uint tangible_	: 1;	// 1 --> can be seen/touched with
  uint collidable_	: 1;	// 1 --> put in auto collision table
  uint global_		: 1;	// 1 --> visible to all groups
  uint persistent_	: 1;	// 1 --> keep ent local across group joins
  uint pad_		: 10;	// unused
};

//-----------------------------------------

class VuEntity {

friend int VuReferenceEntity(VuEntity *ent);
friend int VuDeReferenceEntity(VuEntity *ent);	// note: may free ent
friend class VuDatabase;
friend class VuAntiDatabase;
friend class VuCreateEvent;
friend class VuDeleteEvent;
friend class VuReleaseEvent;

public:
  // constructors
  VuEntity(ushort typeindex);
  VuEntity(VU_BYTE **stream);
  VuEntity(FILE *file);

  // setters
  void SetPosition(BIG_SCALAR x, BIG_SCALAR y, BIG_SCALAR z);
  void SetDelta(SM_SCALAR dx, SM_SCALAR dy, SM_SCALAR dz);
#ifdef VU_USE_QUATERNION
  void SetQuat(VU_QUAT quat)
	{ orient_.quat_[0] = quat[0]; orient_.quat_[1] = quat[1];
	  orient_.quat_[2] = quat[2]; orient_.quat_[3] = quat[3]; }
  void SetQuatDelta(VU_VECT vect, SM_SCALAR theta)
	{ orient_.dquat_[0] = vect[0]; orient_.dquat_[1] = vect[1];
	  orient_.dquat_[2] = vect[2]; orient_.theta_ = theta; }
#else // !VU_USE_QUATERNION
  void SetYPR(SM_SCALAR yaw, SM_SCALAR pitch, SM_SCALAR roll)
	{ orient_.yaw_ = yaw; orient_.pitch_ = pitch; orient_.roll_ = roll; }
  void SetYPRDelta(SM_SCALAR dyaw, SM_SCALAR dpitch,SM_SCALAR droll)
	{ orient_.dyaw_ = dyaw; orient_.dpitch_ = dpitch; orient_.droll_=droll;}
#endif // VU_USE_QUATERNION
  void SetUpdateTime(VU_TIME currentTime)
	{ lastUpdateTime_ = currentTime; }
  void SetTransmissionTime(VU_TIME currentTime)
	{ lastTransmissionTime_ = currentTime; }
  void SetCollisionCheckTime(VU_TIME currentTime)
	{ lastCollisionCheckTime_ = currentTime; }
  void SetOwnerId(VU_ID ownerId);
  void SetEntityType(ushort entityType);
  void SetAssociation(VU_ID assoc) { share_.assoc_ = assoc; }

  void AlignTimeAdd(VU_TIME timeDelta);
  void AlignTimeSubtract(VU_TIME timeDelta);

  // getters
  VU_ID Id()		{ return share_.id_; }
  VU_BYTE Domain()	{ return domain_; }
  VU_BOOL IsPrivate()	{ return (VU_BOOL)share_.flags_.breakdown_.private_; }
  VU_BOOL IsTransferrable(){return (VU_BOOL)share_.flags_.breakdown_.transfer_;}
  VU_BOOL IsTangible()	{ return (VU_BOOL)share_.flags_.breakdown_.tangible_; }
  VU_BOOL IsCollidable(){ return (VU_BOOL)share_.flags_.breakdown_.collidable_;}
  VU_BOOL IsGlobal()	{ return (VU_BOOL)share_.flags_.breakdown_.global_;}
  VU_BOOL IsPersistent(){ return (VU_BOOL)share_.flags_.breakdown_.persistent_;}
  VuFlagBits Flags()	{ return share_.flags_.breakdown_; }
  ushort FlagValue()	{ return share_.flags_.value_; }
  VU_ID OwnerId() 	{ return share_.ownerId_; }
  ushort VuState()	{ return vuState_; }
  ushort Type()		{ return share_.entityType_; }
  VU_BOOL IsLocal()	{ return (VU_BOOL)((vuLocalSession == OwnerId()) ? TRUE : FALSE);}
  VU_ID Association()	{ return share_.assoc_; }

  BIG_SCALAR XPos()	{ return pos_.x_; }
  BIG_SCALAR YPos()	{ return pos_.y_; }
  BIG_SCALAR ZPos()	{ return pos_.z_; }
  SM_SCALAR XDelta()	{ return pos_.dx_; }
  SM_SCALAR YDelta()	{ return pos_.dy_; }
  SM_SCALAR ZDelta()	{ return pos_.dz_; }
#ifdef VU_USE_QUATERNION
  VU_QUAT *Quat() { return &orient_.quat_; }
  VU_VECT *DeltaQuat() { return &orient_.dquat_; }
  SM_SCALAR Theta() { return orient_.theta_; }
#else // !VU_USE_QUATERNION
  SM_SCALAR Yaw()	{ return orient_.yaw_; }
  SM_SCALAR Pitch()	{ return orient_.pitch_; }
  SM_SCALAR Roll()	{ return orient_.roll_; }
  SM_SCALAR YawDelta()	{ return orient_.dyaw_; }
  SM_SCALAR PitchDelta(){ return orient_.dpitch_; }
  SM_SCALAR RollDelta()	{ return orient_.droll_; }
#endif

  VU_TIME UpdateRate()  { return entityTypePtr_->updateRate_; }
  VU_TIME LastUpdateTime()  { return lastUpdateTime_; }
  VU_TIME LastTransmissionTime()  { return lastTransmissionTime_; }
  VU_TIME LastCollisionCheckTime()  { return lastCollisionCheckTime_; }

  VuEntityType *EntityType() { return entityTypePtr_; }

  // entity driver
  VU_TIME DriveEntity(VU_TIME currentTime);	// returns TIME of next update
  void AutoDriveEntity(VU_TIME currentTime);	// runs DR rather than model
  VuDriver *EntityDriver()	{ return driver_; }
  VuDriver *SetDriver(VuDriver *newdriver);	// returns old driver (for del)

  VU_BOOL CollisionCheck(VuEntity *other, SM_SCALAR deltatime);	// uses built-in
  virtual VU_BOOL CustomCollisionCheck(VuEntity *other, SM_SCALAR deltatime);
  virtual VU_BOOL TerrainCollisionCheck();		// default returns false
  VU_BOOL LineCollisionCheck( BIG_SCALAR x1, BIG_SCALAR y1, BIG_SCALAR z1,
			   BIG_SCALAR x2, BIG_SCALAR y2, BIG_SCALAR z2,
			   SM_SCALAR timeDelta, SM_SCALAR sizeFactor );
  // Special VU type getters
  virtual VU_BOOL IsTarget();	// returns FALSE
  virtual VU_BOOL IsSession();	// returns FALSE
  virtual VU_BOOL IsGroup();	// returns FALSE
  virtual VU_BOOL IsGame();	// returns FALSE
  // not really a type, but a utility nonetheless
  virtual VU_BOOL IsCamera();	// returns FALSE

  // virtual function interface
    // serialization functions
  virtual int SaveSize();
  virtual int Save(VU_BYTE **stream);	// returns bytes written
  virtual int Save(FILE *file);		// returns bytes written

    // event handlers
  virtual VU_ERRCODE Handle(VuErrorMessage *error);
  virtual VU_ERRCODE Handle(VuPushRequest *msg);
  virtual VU_ERRCODE Handle(VuPullRequest *msg);
  virtual VU_ERRCODE Handle(VuEvent *event);
  virtual VU_ERRCODE Handle(VuFullUpdateEvent *event);
  virtual VU_ERRCODE Handle(VuPositionUpdateEvent *event);
  virtual VU_ERRCODE Handle(VuEntityCollisionEvent *event);
  virtual VU_ERRCODE Handle(VuTransferEvent *event);
  virtual VU_ERRCODE Handle(VuSessionEvent *event);

  int RefCount() { return refcount_; };
protected:
  // destructor
  virtual ~VuEntity();
  virtual void ChangeId(VuEntity *other);
  void SetVuState(VU_BYTE newState) { vuState_ = newState; }
  virtual VU_ERRCODE InsertionCallback();
  virtual VU_ERRCODE RemovalCallback();

private:
  int LocalSize();                      // returns local bytes written

// DATA
protected:
  // shared data
  struct ShareData {
    ushort entityType_;	// id (table index)
    union {
      ushort value_;
      VuFlagBits breakdown_;
    } flags_;
    VU_ID id_;
    VU_ID ownerId_;	// owning session
    VU_ID assoc_;	// id of ent which must be local to this ent
  } share_;
  struct PositionData {
    BIG_SCALAR x_, y_, z_;
    SM_SCALAR dx_, dy_, dz_;
  } pos_;
  struct OrientationData {
#ifdef VU_USE_QUATERNION
    VU_QUAT quat_;	// quaternion indicating current facing
    VU_VECT dquat_;	// unit vector expressing quaternion delta
    SM_SCALAR theta_;	// scalar indicating rate of above delta
#else // !VU_USE_QUATERNION
    SM_SCALAR yaw_, pitch_, roll_;
    SM_SCALAR dyaw_, dpitch_, droll_;
#endif
  } orient_;

  // local data
  ushort refcount_;	// entity reference count
  VU_BYTE vuState_;		
  VU_BYTE domain_;		
  VU_TIME lastUpdateTime_; 
  VU_TIME lastCollisionCheckTime_; 
  VU_TIME lastTransmissionTime_; 
  VuEntityType *entityTypePtr_;
  VuDriver *driver_;
};

#endif // _VUENTITY_H_
