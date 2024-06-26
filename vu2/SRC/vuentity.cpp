// Copyright (c) 1998,  MicroProse, Inc.  All Rights Reserved

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
};

#include "vu2.h"
#include "vu_priv.h"
#include <windows.h> // JB 010305 Needed for CTD checks
#include <float.h>
#define VU_FREAD(X)  (fread (&(X), sizeof(X), 1, file))
#define VU_FWRITE(X) (fwrite(&(X), sizeof(X), 1, file))

VU_ID_NUMBER vuAssignmentId   = VU_FIRST_ENTITY_ID;
VU_ID_NUMBER vuLowWrapNumber  = VU_FIRST_ENTITY_ID;
VU_ID_NUMBER vuHighWrapNumber = ~((VU_ID_NUMBER)0);

VU_ID vuNullId(0,0);

#ifdef _DEBUG
int vuentitycount;
int vuentitypeak;
#endif

static VuEntityType vuTypeTable[] = {
  { VU_UNKNOWN_ENTITY_TYPE,     // class id
    0, 0.0f,                    // collision type, radius
    {0,0,0,0,0,0,0,0},          // class info
  0, 0,                         // update rate, tolerance
  0.0f, 0.0f, 0.0f,             // fine update range/force/multiplier
  0, 0,                         // damage seed, hitpoints
  0, 0,                         // major/minor rev numbers
  9999,                         // create priority
  VU_GLOBAL_DOMAIN,             // management domain
    FALSE,                      //   transferrable
    TRUE,                       //   private
    FALSE,                      //   tangible
    FALSE,                      //   collidable
    FALSE,                      //   global
    FALSE                       //   persistent
},
{ VU_SESSION_ENTITY_TYPE,       // class id
    0, 0.0f,                    // collision type, radius
  {0,1,0,0,0,0,0,0},            // class info
  VU_TICS_PER_SECOND*5,         // update rate
  VU_TICS_PER_SECOND*15,        // update tolerance
  0.0f, 0.0f, 1.0f,             // fine update range/force/multiplier
  0, 0,                         // damage seed, hitpoints
  0, 0,                         // major/minor rev numbers
  1,                            // create priority
  VU_GLOBAL_DOMAIN,             // management domain
    FALSE,                      //   transferrable
    FALSE,                      //   private
    FALSE,                      //   tangible
    FALSE,                      //   collidable
    TRUE,                       //   global
    TRUE                        //   persistent
},
{ VU_GROUP_ENTITY_TYPE,         // class id
    0, 0.0f,                    // collision type, radius
  {0,2,0,0,0,0,0,0},            // class info
  VU_TICS_PER_SECOND*30,        // update rate
  VU_TICS_PER_SECOND*60,        // update tolerance
  0.0f, 0.0f, 0.0f,             // fine update range/force/multiplier
  0, 0,                         // damage seed, hitpoints
  0, 0,                         // major/minor rev numbers
  2,                            // create priority
  VU_GLOBAL_DOMAIN,             // management domain
    TRUE,                       //   transferrable
    FALSE,                      //   private
    FALSE,                      //   tangible
    FALSE,                      //   collidable
    TRUE,                       //   global
    FALSE                       //   persistent
},
{ VU_GLOBAL_GROUP_ENTITY_TYPE,  // class id
    0, 0.0f,                    // collision type, radius
  {0,3,0,0,0,0,0,0},            // class info
  VU_TICS_PER_SECOND*60,        // update rate
  VU_TICS_PER_SECOND*120,       // update tolerance
  0.0f, 0.0f, 0.0f,             // fine update range/force/multiplier
  0, 0,                         // damage seed, hitpoints
  0, 0,                         // major/minor rev numbers
  3,                            // create priority
  VU_GLOBAL_DOMAIN,             // management domain
    FALSE,                      //   transferrable
    TRUE,                       //   private
    FALSE,                      //   tangible
    FALSE,                      //   collidable
    FALSE,                      //   global
    TRUE                        //   persistent
},
{ VU_GAME_ENTITY_TYPE,          // class id
    0, 0.0f,                    // collision type, radius
  {0,4,0,0,0,0,0,0},            // class info
  VU_TICS_PER_SECOND*30,        // update rate
  VU_TICS_PER_SECOND*60,        // update tolerance
  0.0f, 0.0f, 0.0f,             // fine update range/force/multiplier
  0, 0,                         // damage seed, hitpoints
  0, 0,                         // major/minor rev numbers
  4,                            // create priority
  VU_GLOBAL_DOMAIN,             // management domain
    TRUE,                       //   transferrable
    FALSE,                      //   private
    FALSE,                      //   tangible
    FALSE,                      //   collidable
    TRUE,                       //   global
    FALSE                       //   persistent
},
{ VU_PLAYER_POOL_GROUP_ENTITY_TYPE, // class id
    0, 0.0f,                    // collision type, radius
  {0,5,0,0,0,0,0,0},            // class info
  VU_TICS_PER_SECOND*60,        // update rate
  VU_TICS_PER_SECOND*120,       // update tolerance
  0.0f, 0.0f, 0.0f,             // fine update range/force/multiplier
  0, 0,                         // damage seed, hitpoints
  0, 0,                         // major/minor rev numbers
  5,                            // create priority
  VU_GLOBAL_DOMAIN,             // management domain
    FALSE,                      //   transferrable
    TRUE,                       //   private
    FALSE,                      //   tangible
    FALSE,                      //   collidable
    FALSE,                      //   global
    TRUE                        //   persistent
},
};
static const int vuTypeTableSize = sizeof(vuTypeTable)/sizeof(vuTypeTable[0]);

//--------------------------------------------------
VU_TIME
VuRandomTime(VU_TIME max)
{
  SM_SCALAR rval = (SM_SCALAR)rand();
  rval = rval / RAND_MAX; // value now 0-1
  rval = rval * max;    // value now 0-max
  return (VU_TIME)rval;
}

//--------------------------------------------------
void OutputRef (VuEntity *ent, int);
void OutputDeref (VuEntity *ent, int);
//--------------------------------------------------

int
VuReferenceEntity(VuEntity* ent)
{
  if (ent)
    return ++ent->refcount_;
  else
    return -1;
}

int
VuDeReferenceEntity(VuEntity* ent)
{
  assert(ent == NULL || FALSE == F4IsBadWritePtr(ent, sizeof(VuEntity)));
  //if (ent) { // JB 010305 CTD
	if (ent && !F4IsBadWritePtr(ent, sizeof(VuEntity))) { // JB 010305 CTD
    if (--ent->refcount_ <= 0) {
			//assert(vuDatabase == NULL || vuDatabase->Find(ent->Id()) == NULL);
			assert(vuCollectionManager == NULL || vuCollectionManager->FindEnt(ent) == 0);
      delete ent;
      return 0;
    }
    return ent->refcount_;
  }
  return -1;
}

//--------------------------------------------------
//--------------------------------------------------

VuEntity::~VuEntity()
{
  vuState_ = VU_MEM_DELETED;
  // delete the driver
  delete driver_;
  driver_ = 0; //me123
#ifdef _DEBUG
  vuentitycount --;
#endif
}

VuEntity::VuEntity(ushort typeindex)
  : refcount_(0),
    driver_(0)
{
  pos_.x_  = 0.0f;
  pos_.y_  = 0.0f;
  pos_.z_  = 0.0f;
  pos_.dx_ = 0.0f;
  pos_.dy_ = 0.0f;
  pos_.dz_ = 0.0f;

#if defined(VU_USE_QUATERNION)
  for (int i = 0; i < 3; i++)
    orient_.quat_[i] = 0.0f;

  orient_.quat_[3] = 1.0f;

  for (i = 0; i < 3; i++)
    orient_.dquat_[i] = 0.0f;

  orient_.theta_ = 0.0f;
#else
  orient_.yaw_    = 0.0f;
  orient_.pitch_  = 0.0f;
  orient_.roll_   = 0.0f;
  //orient_.dyaw_   = 0.0f;
  //orient_.dpitch_ = 0.0f;
  //orient_.droll_  = 0.0f;
#endif

  vuState_ = VU_MEM_CREATED;

  SetEntityType(typeindex);
  share_.assoc_       = vuNullId;
  share_.ownerId_     = vuLocalSession; // need to fill in from id structure
  share_.id_.creator_ = share_.ownerId_.creator_;

  if (typeindex == VU_UNKNOWN_ENTITY_TYPE) {
    share_.id_.num_ = 0;
  } 
  else {
    VuEnterCriticalSection();
    if (vuAssignmentId < vuLowWrapNumber || vuAssignmentId > vuHighWrapNumber)
      vuAssignmentId = vuLowWrapNumber; // cover wrap
    share_.id_.num_ = vuAssignmentId++;
    while (vuDatabase->Find(Id()) || vuAntiDB->Find(Id())) {
      share_.id_.num_ = vuAssignmentId++;
    }
    VuExitCriticalSection();
  }

  lastUpdateTime_         = vuxGameTime;
  lastTransmissionTime_   = vuTransmitTime - VuRandomTime(UpdateRate());
  lastCollisionCheckTime_ = vuxGameTime;
#ifdef _DEBUG
  vuentitycount ++;
  if (vuentitycount > vuentitypeak)
      vuentitypeak = vuentitycount;
#endif
}


// restores entity from a file
VuEntity::VuEntity(FILE* file)
  : refcount_(0),
    driver_(0)
{
  fread(&share_.entityType_,       sizeof(share_.entityType_),       1, file);
  fread(&share_.flags_,            sizeof(share_.flags_),            1, file);
  fread(&share_.id_.creator_,      sizeof(share_.id_.creator_),      1, file);
  fread(&share_.id_.num_,          sizeof(share_.id_.num_),          1, file);
  fread(&share_.ownerId_.creator_, sizeof(share_.ownerId_.creator_), 1, file);
  fread(&share_.ownerId_.num_,     sizeof(share_.ownerId_.num_),     1, file);
  fread(&share_.assoc_.creator_,   sizeof(share_.assoc_.creator_),   1, file);
  fread(&share_.assoc_.num_,       sizeof(share_.assoc_.num_),       1, file);
  fread(&pos_,                     sizeof(pos_),                     1, file);
  fread(&orient_,                  sizeof(orient_),                  1, file);

  vuState_ = VU_MEM_CREATED;
  // note: this is explicitly set to local session because we can only 
  //       load it from a file if we manage it locally
  share_.ownerId_ = vuLocalSession;
  SetEntityType(share_.entityType_);
  lastUpdateTime_         = vuxGameTime;
  lastCollisionCheckTime_ = vuxGameTime;
#ifdef _DEBUG
  vuentitycount ++;
  if (vuentitycount > vuentitypeak)
      vuentitypeak = vuentitycount;
#endif
}

// restores entity from a stream or event
VuEntity::VuEntity(VU_BYTE **stream)
  : refcount_(0),
    driver_(0)
{
  memcpy(&share_.entityType_, *stream, sizeof(share_.entityType_));
        *stream += sizeof(share_.entityType_);
  memcpy(&share_.flags_, *stream, sizeof(share_.flags_));
        *stream += sizeof(share_.flags_);
  memcpy(&share_.id_.creator_, *stream, sizeof(share_.id_.creator_));
        *stream += sizeof(share_.id_.creator_);
  memcpy(&share_.id_.num_, *stream, sizeof(share_.id_.num_));
        *stream += sizeof(share_.id_.num_);
  memcpy(&share_.ownerId_.creator_, *stream, sizeof(share_.ownerId_.creator_));
        *stream += sizeof(share_.ownerId_.creator_);
  memcpy(&share_.ownerId_.num_, *stream, sizeof(share_.ownerId_.num_));
        *stream += sizeof(share_.ownerId_.num_);
  memcpy(&share_.assoc_.creator_, *stream, sizeof(share_.assoc_.creator_));
        *stream += sizeof(share_.assoc_.creator_);
  memcpy(&share_.assoc_.num_, *stream, sizeof(share_.assoc_.num_));
        *stream += sizeof(share_.assoc_.num_);
  memcpy(&pos_, *stream, sizeof(pos_));
        *stream += sizeof(pos_);
  memcpy(&orient_, *stream, sizeof(orient_));
        *stream += sizeof(orient_);

  vuState_ = VU_MEM_CREATED;
  SetEntityType(share_.entityType_);
  lastUpdateTime_= vuxGameTime;
  lastCollisionCheckTime_= vuxGameTime;
#ifdef _DEBUG
  vuentitycount ++;
  if (vuentitycount > vuentitypeak)
      vuentitypeak = vuentitycount;
#endif
}

void
VuEntity::AlignTimeAdd(VU_TIME dt)
{
  lastUpdateTime_ += dt;

  if (IsLocal())
    lastCollisionCheckTime_ += dt;

  if (driver_)
    driver_->AlignTimeAdd(dt);

}

void
VuEntity::AlignTimeSubtract(VU_TIME dt)
{
  lastUpdateTime_ -= dt;

  if (IsLocal())
    lastCollisionCheckTime_ -= dt;

  if (driver_)
    driver_->AlignTimeSubtract(dt);
}

void
VuEntity::SetEntityType(ushort entityType)
{
  share_.entityType_ = entityType;

  if (share_.entityType_ < vuTypeTableSize) // JPO - make sure its in the range (CTD) VU_LAST_ENTITY_TYPE == 100 
    entityTypePtr_ = &vuTypeTable[share_.entityType_];
  else if (share_.entityType_ >= VU_LAST_ENTITY_TYPE)
    entityTypePtr_ = VuxType(share_.entityType_);
  else {
	  assert(!"share_.entityType_ out of range");
	  entityTypePtr_ = 0;
  }
  
  if (entityTypePtr_ == 0)
    entityTypePtr_ = &vuTypeTable[VU_UNKNOWN_ENTITY_TYPE];
  
  share_.flags_.breakdown_.private_    = entityTypePtr_->private_;
  share_.flags_.breakdown_.transfer_   = entityTypePtr_->transferable_;
  share_.flags_.breakdown_.tangible_   = entityTypePtr_->tangible_;
  share_.flags_.breakdown_.collidable_ = entityTypePtr_->collidable_;
  share_.flags_.breakdown_.global_     = entityTypePtr_->global_;
  share_.flags_.breakdown_.persistent_ = entityTypePtr_->persistent_;

  if (share_.flags_.breakdown_.persistent_&&share_.flags_.breakdown_.transfer_)
    share_.flags_.breakdown_.transfer_ = 0;
  
  share_.flags_.breakdown_.pad_ = 1;
  domain_ = entityTypePtr_->managementDomain_;
  // cap domain at (bits/long - 1)
  if (domain_ > 31)
    domain_ = 31;

}

VU_TIME
VuEntity::DriveEntity(VU_TIME currentTime)
{
  VU_TIME retval = currentTime;
  if (driver_) {
    driver_->Exec(currentTime);
    retval += UpdateRate();
  }
  return retval;
}

void
VuEntity::AutoDriveEntity(VU_TIME currentTime)
{
  if (driver_)
    driver_->AutoExec(currentTime);
}

extern VuGridTree *vuxCollisionGrid;

void
VuEntity::SetPosition(BIG_SCALAR x, 
                      BIG_SCALAR y, 
                      BIG_SCALAR z)
{
    assert(!_isnan(x));
    assert(!_isnan(y));
    assert(!_isnan(z));
  if (z > 0.0F) {
    z = 0.0F;
  }

#ifdef VU_GRID_TREE_Y_MAJOR
  vuCollectionManager->HandleMove(this, y, x);
#else
  vuCollectionManager->HandleMove(this, x, y);
#endif

//  assert( x > -1e6F && y > -1e6F && z < 20000.0F);
//  assert( x < 5e6 && y < 5e6 && z > -250000.0F);

  if (!IsLocal ()) {
    static BIG_SCALAR lx, ly, lz;
    
    //MonoPrint ("%08x %3.3f %3.3f %3.3f\n", Id().num_, x - lx, y - ly, z - lz);
    lx = x;
    ly = y;
    lz = z;
  }

//  MonoPrint ("%08x %3.3f %3.3f %3.3f\n", this, x, y, z);
  pos_.x_ = x;
  pos_.y_ = y;
  pos_.z_ = z;
}

void VuEntity::SetDelta(SM_SCALAR dx, SM_SCALAR dy, SM_SCALAR dz)
{
//	 assert( dx > -10000.0F && dy > -10000.0F && dz > -10000.0F);
//   assert( dx <  10000.0F && dy <  10000.0F && dz <  10000.0F);
    assert(!_isnan(dx));
    assert(!_isnan(dy));
    assert(!_isnan(dz));

	pos_.dx_ = dx;
	pos_.dy_ = dy;
	pos_.dz_= dz;
}

VuDriver*
VuEntity::SetDriver(VuDriver* newdriver)
{
  VuDriver* olddriver = driver_;
  driver_ = newdriver;
  return olddriver;
}

void
VuEntity::SetOwnerId(VU_ID ownerId)
{
  // New owner sends xfer message: check for locality after set
  if (vuLocalSession == ownerId && share_.ownerId_ != ownerId) {
    share_.ownerId_ = ownerId;
    VuTargetEntity* target = vuGlobalGroup;
    
    if (!IsGlobal())
      target = vuLocalSessionEntity->Game();
    
    VuTransferEvent* event = new VuTransferEvent(this, target);
    event->Ref();
    event->RequestReliableTransmit();
    VuMessageQueue::PostVuMessage(event);
    // force immediate local handling of this
    Handle(event);
    vuDatabase->Handle(event);
    event->UnRef();
  }
  share_.ownerId_ = ownerId;
}

#define VUENTITY_LOCALSIZE            \
  (sizeof(share_.entityType_)       + \
   sizeof(share_.flags_)            + \
   sizeof(share_.id_.creator_)      + \
   sizeof(share_.id_.num_)          + \
   sizeof(share_.ownerId_.creator_) + \
   sizeof(share_.ownerId_.num_)     + \
   sizeof(share_.assoc_.creator_)   + \
   sizeof(share_.assoc_.num_)       + \
   sizeof(pos_)                     + \
   sizeof(orient_)) 

int
VuEntity::LocalSize()
{
  return(VUENTITY_LOCALSIZE);
}

int
VuEntity::SaveSize()
{
  return(VUENTITY_LOCALSIZE);
}


int 
VuEntity::Save(FILE* file)
{
  int retval = 0;

  if (file) {
    retval += fwrite(&share_.entityType_,       sizeof(share_.entityType_),       1, file);
    retval += fwrite(&share_.flags_,            sizeof(share_.flags_),            1, file);
    retval += fwrite(&share_.id_.creator_,      sizeof(share_.id_.creator_),      1, file);
    retval += fwrite(&share_.id_.num_,          sizeof(share_.id_.num_),          1, file);
    retval += fwrite(&share_.ownerId_.creator_, sizeof(share_.ownerId_.creator_), 1, file);
    retval += fwrite(&share_.ownerId_.num_,     sizeof(share_.ownerId_.num_),     1, file);
    retval += fwrite(&share_.assoc_.creator_,   sizeof(share_.assoc_.creator_),   1, file);
    retval += fwrite(&share_.assoc_.num_,       sizeof(share_.assoc_.num_),       1, file);
    retval += fwrite(&pos_,                     sizeof(pos_),                     1, file);
    retval += fwrite(&orient_,                  sizeof(orient_),                  1, file);
  }

  return retval;
}

int 
VuEntity::Save(VU_BYTE** stream)
{
  if (stream) {
    if (*stream) {
      memcpy(*stream, &share_.entityType_,  sizeof(share_.entityType_));    *stream += sizeof(share_.entityType_);
      memcpy(*stream, &share_.flags_,       sizeof(share_.flags_));         *stream += sizeof(share_.flags_);
      memcpy(*stream, &share_.id_.creator_, sizeof(share_.id_.creator_));   *stream += sizeof(share_.id_.creator_);
      memcpy(*stream, &share_.id_.num_, sizeof(share_.id_.num_));           *stream += sizeof(share_.id_.num_);
      memcpy(*stream, &share_.ownerId_.creator_, sizeof(share_.ownerId_.creator_)); *stream += sizeof(share_.ownerId_.creator_);
      memcpy(*stream, &share_.ownerId_.num_, sizeof(share_.ownerId_.num_)); *stream += sizeof(share_.ownerId_.num_);
      memcpy(*stream, &share_.assoc_.creator_, sizeof(share_.assoc_.creator_)); *stream += sizeof(share_.assoc_.creator_);
      memcpy(*stream, &share_.assoc_.num_, sizeof(share_.assoc_.num_));     *stream += sizeof(share_.assoc_.num_);
      memcpy(*stream, &pos_,               sizeof(pos_));                   *stream += sizeof(pos_);
      memcpy(*stream, &orient_,            sizeof(orient_));                *stream += sizeof(orient_);
      
      return(VUENTITY_LOCALSIZE);
    }
  }
  
  return 0;
}

void
VuEntity::ChangeId(VuEntity* other)
{
  VuEnterCriticalSection();
  VuMessageQueue::FlushAllQueues();
  VuReferenceEntity(this);
  vuDatabase->Remove(this);
  VuMessageQueue::FlushAllQueues();

  if (vuAssignmentId < vuLowWrapNumber || vuAssignmentId > vuHighWrapNumber)
    vuAssignmentId = vuLowWrapNumber; // cover wrap
  share_.id_.num_ = vuAssignmentId++;

  while (vuDatabase->Find(Id()) || vuAntiDB->Find(Id()) || Id() == other->Id())
    share_.id_.num_ = vuAssignmentId++;
  
  SetVuState(VU_MEM_ACTIVE);
  vuDatabase->Insert(this);
  VuDeReferenceEntity(this);
  VuExitCriticalSection();
}

VU_ERRCODE 
VuEntity::InsertionCallback()
{
  return VU_NO_OP;
}

VU_ERRCODE 
VuEntity::RemovalCallback()
{
  return VU_NO_OP;
}

// ---------------
// event handlers
// ---------------
VU_ERRCODE
VuEntity::Handle(VuPushRequest* msg)
{
    assert(FALSE == F4IsBadReadPtr(vuDatabase, sizeof *vuDatabase));
  VuTargetEntity* sender = (VuTargetEntity*)vuDatabase->Find(msg->Sender());

  if (sender) {
    if (sender->IsTarget()) {
			if (share_.flags_.breakdown_.transfer_ == 0) {
        VuMessage* resp = new VuErrorMessage(VU_CANT_TRANSFER_ENTITY_ERROR,
                                             msg->Sender(), Id(), sender);
        resp->RequestReliableTransmit();
        VuMessageQueue::PostVuMessage(resp);
      } 
      else if ((1<<Domain()) & vuLocalSessionEntity->DomainMask()) {
        VuEntity* assoc = 0;
        if (Association() != vuNullId &&
            (assoc = vuDatabase->Find(Association())) != 0 &&
            assoc->OwnerId() != vuLocalSession) {
          // entity has association, and we do not manage it
          VuMessage* resp = new VuErrorMessage(VU_TRANSFER_ASSOCIATION_ERROR, msg->Sender(), Id(), sender);
          resp->RequestReliableTransmit();
          VuMessageQueue::PostVuMessage(resp);
        } 
        else { 
					SetOwnerId(vuLocalSession);
        }
      } 
      else {
        // we can't manage the entity, so send the error response
        VuMessage* resp = new VuErrorMessage(VU_CANT_MANAGE_ENTITY_ERROR,
                                             msg->Sender(), Id(), sender);
        resp->RequestReliableTransmit();
        VuMessageQueue::PostVuMessage(resp);
      }
      return VU_SUCCESS;
    }
  } 
  return VU_ERROR;
}


VU_ERRCODE
VuEntity::Handle(VuPullRequest* msg)
{
  VuTargetEntity* sender = (VuTargetEntity*)vuDatabase->Find(msg->Sender());
  VuMessage*      resp   = 0;

  if (sender) {
    if (sender->IsTarget()) {

      if (share_.flags_.breakdown_.transfer_ == 0) {
        resp = new VuErrorMessage(VU_CANT_TRANSFER_ENTITY_ERROR,
                                  msg->Sender(), Id(), sender);
      } 
      else if (OwnerId() == vuLocalSession) {
        VuEntity* assoc = 0;
        if (Association() != vuNullId &&
            (assoc = vuDatabase->Find(Association())) != 0 &&
            assoc->OwnerId() != msg->Sender()) {
          // entity has association, and target does not manage it
          resp = new VuErrorMessage(VU_TRANSFER_ASSOCIATION_ERROR,
                                    msg->Sender(), Id(), sender);
        } 
        else {
          SetOwnerId(msg->Sender());
          // need to make message as SetOwnerId() only generates update if new
          // owner is local
          VuTargetEntity* target = vuGlobalGroup;

          if (!IsGlobal())
            target = vuLocalSessionEntity->Game();

          resp = new VuTransferEvent(this, target);
        }
      } 
      else {
        // we don't manage the entity, so send the error response
        resp = new VuErrorMessage(VU_DONT_MANAGE_ENTITY_ERROR, msg->Sender(), Id(), sender);
      }
      resp->RequestReliableTransmit();
      VuMessageQueue::PostVuMessage(resp);

      return VU_SUCCESS;
    }
  }
  return VU_ERROR;
}

VU_ERRCODE
VuEntity::Handle(VuErrorMessage*)
{
  // default implementation stub
  return VU_NO_OP;
}

VU_ERRCODE
VuEntity::Handle(VuEvent*)
{
  // default implementation stub
  return VU_NO_OP;
}

VU_ERRCODE
VuEntity::Handle(VuFullUpdateEvent* event)
{
  // default implementation 
  if (driver_) {
    if (driver_->Handle(event) > 0)
      return VU_SUCCESS;
  } 
  else if (event->Entity() && event->Entity() != this) {
    share_  = event->Entity()->share_;
    pos_    = event->Entity()->pos_;
    orient_ = event->Entity()->orient_;
    SetUpdateTime(vuxGameTime);
  }
  return VU_NO_OP;
}

VU_ERRCODE
VuEntity::Handle(VuPositionUpdateEvent* event)
{
  if (!driver_ || driver_->Handle(event) <= 0) {
    SetPosition(event->x_, event->y_, event->z_);
    SetDelta(event->dx_, event->dy_, event->dz_);

#if defined(VU_USE_QUATERNION)
    SetQuat(event->quat_);
    SetQuatDelta(event->dquat_, event->theta_);
#else
    SetYPR(event->yaw_, event->pitch_, event->roll_);
    //SetYPRDelta(event->dyaw_, event->dpitch_, event->droll_);
#endif

    SetUpdateTime(vuxGameTime);
  }
  return VU_SUCCESS;
}

VU_ERRCODE
VuEntity::Handle(VuEntityCollisionEvent*)
{
  // default implementation stub
  return VU_NO_OP;
}

VU_ERRCODE
VuEntity::Handle(VuTransferEvent* event)
{
  SetOwnerId(event->newOwnerId_);
  return VU_SUCCESS;
}

VU_ERRCODE
VuEntity::Handle(VuSessionEvent*)
{
  // default does nothing
  return VU_NO_OP;
}

// ---------------
// collision check
// ---------------

VU_BOOL
VuEntity::CollisionCheck(VuEntity* other, 
                         SM_SCALAR dt)
{
  SM_SCALAR size = EntityType()->collisionRadius_ +  other->EntityType()->collisionRadius_;
  // object does not collide with itself

  if (Id() != other->Id()) {

    // Cull based on simple bounding box rejection test, on a per axis basis
    if (vuabs(ZPos() - other->ZPos()) > size + vuabs(dt * (ZDelta() - other->ZDelta())))
      return FALSE;
    
    if (vuabs(YPos() - other->YPos()) > size + vuabs(dt * (YDelta() - other->YDelta())))
      return FALSE;
    
    if (vuabs(XPos() - other->XPos()) > size + vuabs(dt * (XDelta() - other->XDelta())))
      return FALSE;
    
    return TRUE;
  }
  
  return(FALSE);
}

VU_BOOL
VuEntity::CustomCollisionCheck(VuEntity* other, 
                               SM_SCALAR dt)
{
  // default just performs standard collision check
  return CollisionCheck(other, dt);
}

VU_BOOL
VuEntity::TerrainCollisionCheck()
{
  // default returns FALSE
  return FALSE;
}

static inline VU_BOOL
collisionLineEntity1D(BIG_SCALAR d1, 
                      BIG_SCALAR d2,
                      BIG_SCALAR objd,
                      BIG_SCALAR radius)
{
  if ( ( (d1 < d2) && (d2 > (objd - radius)) && (d1 < (objd + radius)) ) ||
       ( (d1 >= d2) && (d1 >= (objd - radius)) && (d2 <= (objd + radius)) ) )
    {
      return TRUE;
    }
  return FALSE;
}

VU_BOOL
VuEntity::LineCollisionCheck(BIG_SCALAR x1,
                             BIG_SCALAR y1,
                             BIG_SCALAR z1,
                             BIG_SCALAR x2, 
                             BIG_SCALAR y2,
                             BIG_SCALAR z2,
                             SM_SCALAR  dt,
                             SM_SCALAR  sizeFactor )
{
  BIG_SCALAR mult = sizeFactor * dt;
  SM_SCALAR  size = EntityType()->collisionRadius_;

  if (collisionLineEntity1D(z1, z2, ZPos(), (size + vuabs(mult * ZDelta()))) &&
      collisionLineEntity1D(y1, y2, YPos(), (size + vuabs(mult * YDelta()))) &&
      collisionLineEntity1D(x1, x2, XPos(), (size + vuabs(mult * XDelta()))) ) {
    return TRUE;
  }
  return FALSE;
}

VU_BOOL 
VuEntity::IsTarget()
{
  return FALSE;
}

VU_BOOL 
VuEntity::IsSession()
{
  return FALSE;
}

VU_BOOL 
VuEntity::IsGroup()
{
  return FALSE;
}

VU_BOOL 
VuEntity::IsGame()
{
  return FALSE;
}

VU_BOOL 
VuEntity::IsCamera()
{
  return FALSE;
}

