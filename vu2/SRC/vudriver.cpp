// Copyright (c) 1998,  MicroProse, Inc.  All Rights Reserved

extern "C" {
#include <stdio.h>
#include <string.h>
};
#include <math.h>
#include "vu2.h"
#include "sim\include\stdhdr.h"

#ifdef VU_USE_QUATERNION
//#include "ml.h"
#endif

//#define DEBUG_SMOOTHING
//#define DEBUG_MASTER
#ifndef VU_USE_QUATERNION // only turn on if not using quaternions
//#define DEBUG_ROTATION
#endif

#define LAG_COUNT 8

SM_SCALAR vuTicsPerSec    = (SM_SCALAR)VU_TICS_PER_SECOND;
SM_SCALAR vuTicsPerSecInv = 1.0f/(SM_SCALAR)VU_TICS_PER_SECOND;
int PosUpdateInterval = 0;//me123
extern int SGpfSwitchVal;
extern bool g_bMPFix3;
extern bool g_bMPFix4;
VU_TIME LastPosUpdateround = 0;//m123

//-----------------------------------------
VuDriverSettings::VuDriverSettings
(
	SM_SCALAR x, 
	SM_SCALAR y, 
	SM_SCALAR z,
#if defined(VU_USE_QUATERNION)
	SM_SCALAR tol,
#else
	SM_SCALAR yaw, 
	SM_SCALAR pitch, 
	SM_SCALAR roll,
#endif
	SM_SCALAR maxJumpDist, 
	SM_SCALAR maxJumpAngle, 
	VU_TIME lookaheadTime
#if defined(VU_QUEUE_DR_UPDATES)
	,VuFilter* filter
#endif
)
{
  globalPosTolMod_   = 1.0f;
  globalAngleTolMod_ = 1.0f;
  xtol_  = x;
  ytol_  = y;
  ztol_  = z;

#if defined(VU_USE_QUATERNION)
  quattol_  = tol;
#else
  yawtol_   = yaw;
  pitchtol_ = pitch;
  rolltol_  = roll;
#endif

  maxJumpDist_   = maxJumpDist;
  maxJumpAngle_  = maxJumpAngle;
  lookaheadTime_ = lookaheadTime;

#if defined(VU_QUEUE_DR_UPDATES)
  VuOpaqueFilter defaultFilter;
  if (!filter) filter = &defaultFilter;
  updateQueue_ = new VuFifoQueue(filter);
  updateQueue_->Init();
  roughUpdateQueue_ = new VuFifoQueue(filter);//me123
  roughUpdateQueue_->Init();//me123
#endif
}

VuDriverSettings::~VuDriverSettings()
{
#if defined(VU_QUEUE_DR_UPDATES)
  updateQueue_->DeInit();
  delete updateQueue_;
  roughUpdateQueue_->DeInit();//me123
  delete roughUpdateQueue_;//me123
#endif
}

//-----------------------------------------
VuDriver::VuDriver(VuEntity* entity) : entity_(entity)
{
  // empty
}

VuDriver::~VuDriver()
{
	if (g_bMPFix4)
	{
// if (vuxDriverSettings->updateQueue_->Find(Entity()))
 vuxDriverSettings->updateQueue_->Remove(Entity());

// if(vuxDriverSettings->roughUpdateQueue_->Find(Entity()))
 vuxDriverSettings->roughUpdateQueue_->Remove(Entity());
	}

  // empty
}

int 
VuDriver::AiType()
{
  return 0;
}

VU_BOOL 
VuDriver::AiExec()
{
  return FALSE;
}

void *
VuDriver::AiPointer()
{
  return 0;
}

int
VuDriver::DebugString(char *str)
{
  *str = 0;
  return 0;
}

//-----------------------------------------
VuDeadReckon::VuDeadReckon(VuEntity* entity) : VuDriver(entity)
{
  drx_      = entity->XPos();
  dry_      = entity->YPos();
  drz_      = entity->ZPos();

//  assert( drx_ > -1e6F && dry_ > -1e6F && drz_ < 20000.0F);
 // assert( drx_ < 5e6 && dry_ < 5e6 && drz_ > -250000.0F);

  drxdelta_ = entity->XDelta();
  drydelta_ = entity->YDelta();
  drzdelta_ = entity->ZDelta();

#if defined(VU_USE_QUATERNION)
  VU_QUAT* quat = entity->Quat();
  ML_QuatCopy(drquat_, *quat);
  VU_VECT* dquat = entity->DeltaQuat();
  ML_VectCopy(drquatdelta_, *dquat);

  drtheta_      = entity->Theta();
#else
  dryaw_        = entity->Yaw();
  drpitch_      = entity->Pitch();
  drroll_       = entity->Roll();
//  dryawdelta_   = entity->YawDelta();
//  drpitchdelta_ = entity->PitchDelta();
//  drrolldelta_  = entity->RollDelta();
#endif

  lastUpdateTime_ = vuxGameTime;
}

VuDeadReckon::~VuDeadReckon()
{
  // empty
}

void
VuDeadReckon::AlignTimeAdd(VU_TIME dt)
{
  lastUpdateTime_ += dt;
}

void
VuDeadReckon::AlignTimeSubtract(VU_TIME dt)
{
  lastUpdateTime_ -= dt;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void VuDeadReckon::ResetLastUpdateTime (VU_TIME time)
{
	lastUpdateTime_ = time;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void VuDeadReckon::NoExec (VU_TIME time)
{
	drxdelta_ = entity_->XDelta ();
	drydelta_ = entity_->YDelta ();
	drzdelta_ = entity_->ZDelta ();
	drx_ = entity_->XPos ();
	dry_ = entity_->YPos ();
	drz_ = entity_->ZPos ();
	dryaw_ = entity_->Yaw ();
	drpitch_ = entity_->Pitch ();
	drroll_ = entity_->Roll ();

	lastUpdateTime_ = time;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// by default, this function just calls ExecDR
void 
VuDeadReckon::Exec(VU_TIME timestamp)
{
  ExecDR(timestamp);
}

void 
VuDeadReckon::AutoExec(VU_TIME timestamp)
{
  ExecDR(timestamp);
  entity_->SetPosition(drx_, dry_, drz_);

#if defined(VU_USE_QUATERNION)
  entity_->SetQuat(drquat_);
#else
  entity_->SetYPR(dryaw_, drpitch_, drroll_);
#endif

  entity_->SetUpdateTime(timestamp);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// This function does the dead reckon computation
void
VuDeadReckon::ExecDR(VU_TIME timestamp)
{
  SM_SCALAR dt =((SM_SCALAR)timestamp - (SM_SCALAR)lastUpdateTime_)*vuTicsPerSecInv;

  drx_  += drxdelta_  * dt;
  dry_  += drydelta_  * dt;
  drz_  += drzdelta_  * dt;

  if (drz_ > 0.0) drz_ = 0.0;

// assert( drx_ > -1e6F && dry_ > -1e6F && drz_ < 20000.0F);
 // assert( drx_ < 5e6 && dry_ < 5e6 && drz_ > -250000.0F);

#if defined(VU_USE_QUATERNION)
  SM_SCALAR timeTheta = drtheta_ * dt * 0.5f;
  SM_SCALAR vmult     = sin(timeTheta);
  VU_QUAT   nq;
  nq[ML_X] = drquatdelta_[ML_X] * vmult;
  nq[ML_Y] = drquatdelta_[ML_Y] * vmult;
  nq[ML_Z] = drquatdelta_[ML_Z] * vmult;
  nq[ML_W] = cos(timeTheta);
  ML_QuatMul(drquat_, drquat_, nq);
#else

#if 0
  dryaw_   += dryawdelta_   * dt;
  drpitch_ += drpitchdelta_ * dt;
  drroll_  += drrolldelta_  * dt;

  if (drpitch_ > VU_PI/2) {
    drpitch_ -= VU_PI;
    if (dryaw_ > 0)
      dryaw_ -= VU_PI;
    else 
      dryaw_ += VU_PI;
    if (drroll_ > 0)
      drroll_ -= VU_PI;
    else
      drroll_ += VU_PI;
  } else if (drpitch_ < -VU_PI) {
    drpitch_ += VU_PI;
    if (dryaw_ > 0)
      dryaw_ -= VU_PI;
    else 
      dryaw_ += VU_PI;
    if (drroll_ > 0)
      drroll_ -= VU_PI;
    else
      drroll_ += VU_PI;
  }
  if (drroll_ > VU_PI) {
    drroll_ -= VU_TWOPI;
  } else if (drroll_ < -VU_PI) {
    drroll_ += VU_TWOPI;
  }
  if (dryaw_ > VU_PI) {
    dryaw_ -= VU_TWOPI;
  } else if (dryaw_ < -VU_PI) {
    dryaw_ += VU_TWOPI;
  }
#endif
#endif

  lastUpdateTime_ = timestamp;
}

//-----------------------------------------
VuMaster::VuMaster(VuEntity* entity) : VuDeadReckon(entity)
{
  lastPositionUpdateTime_ = 0;
  lastFinePositionUpdateTime_ = 0;
  pendingUpdate_      = FALSE;
  pendingRoughUpdate_ = FALSE;
  xtol_  = vuxDriverSettings->xtol_;
  ytol_  = vuxDriverSettings->ytol_;
  ztol_  = vuxDriverSettings->ztol_;

#if defined(VU_USE_QUATERNION)
  quattol_  = vuxDriverSettings->quattol_;
#else
  yawtol_   = vuxDriverSettings->yawtol_;
  pitchtol_ = vuxDriverSettings->pitchtol_;
  rolltol_  = vuxDriverSettings->rolltol_;
#endif
}

VuMaster::~VuMaster()
{
  // empty
}

int
VuMaster::Type()
{
  return VU_MASTER_DRIVER;
}

int
VuMaster::DebugString(char *str)
{
  return sprintf(str, "id 0x%x, dr(%.0f, %.0f, %.0f), drd(%.2f, %.2f, %.2f)\n",
        (unsigned)entity_->Id(), drx_,dry_,drz_,drxdelta_,drydelta_,drzdelta_);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int 
VuMaster::CheckTolerance()
{
	SM_SCALAR scale = 1.0f;
	SM_SCALAR fineupdatescale = 1.0f;
	
#if VU_MAX_SESSION_CAMERAS > 0
	
	// danm_NOTE: this is an obvious case for using a grid tree
	//         -- If a camera grid tree is added in the future, use it here!
	
	VuGameEntity* game;
	BIG_SCALAR    range;
	BIG_SCALAR    m;
	BIG_SCALAR    xdist;
	BIG_SCALAR    ydist;
	BIG_SCALAR    zdist;
	BIG_SCALAR    dist = entity_->EntityType()->fineUpdateRange_ * 8.0f;
	int result = 0;

	range = entity_->EntityType()->fineUpdateRange_ * 8.0f;
	game  = vuLocalSessionEntity->Game();
	
	if (game) {
		VuSessionsIterator iter(game);
		VuSessionEntity*   sess;
		
		sess = iter.GetFirst();
		
		while (sess) {
			if (sess != vuLocalSessionEntity){
				// assume omnicience...
				//        if (sess->CameraCount() == 0) {
				//		      range = entity_->EntityType()->fineUpdateForceRange_;
				//	    }
				
				for (int i = 0; i < 1 /*sess->CameraCount()*/; i++) {
					VuEntity *camera = sess->Camera(i);
					if (camera && camera !=entity_)
					{
						m = camera->EntityType()->fineUpdateMultiplier_;
						
						if (m <= 0.0f)
						{
							m = 1.0f;
						}
						
						xdist = vuabs(entity_->XPos() - camera->XPos()) - vuabs(entity_->XDelta() + camera->XDelta());
						ydist = vuabs(entity_->YPos() - camera->YPos()) - vuabs(entity_->YDelta() + camera->YDelta());
						zdist = vuabs(entity_->ZPos() - camera->ZPos()) - vuabs(entity_->ZDelta() + camera->ZDelta());
						dist  = vumax(vumax(xdist, ydist), zdist);
						dist  = dist/m;

						if (camera->XPos() == 0 && camera->YPos() == 0 && camera->ZPos() == 0)
							dist = entity_->EntityType()->fineUpdateForceRange_ + 1;

						if (dist < range)
							range = dist;
					}
				}
			}
			
			sess = iter.GetNext();
		}
	}

	if (range <= entity_->EntityType()->fineUpdateForceRange_) {
		scale = static_cast<float>(0.001);
	}
	else if (range < entity_->EntityType()->fineUpdateRange_* 8.0f &&
		range > entity_->EntityType()->fineUpdateForceRange_) {
		scale =
			(range - entity_->EntityType()->fineUpdateForceRange_) /
			(        entity_->EntityType()->fineUpdateRange_* 8.0f -
			entity_->EntityType()->fineUpdateForceRange_);
		if (scale < 0.1f) 
		{
			fineupdatescale = scale;
			scale = 0.1f;
		}
	}
	else
	{
		return 0 ;
	}

		if ( _isnan ( entity_->XPos() ) || !_finite( entity_->XPos() ) ||
      	  _isnan ( entity_->YPos() ) || !_finite( entity_->YPos() ) ||
      	  _isnan ( entity_->ZPos() ) || !_finite( entity_->ZPos() ) )
				return 0; //me123 sanity check
#endif
// has the possition moved
	if((vuabs(entity_->XPos() - drx_) > ((scale*vuxDriverSettings->xtol_) *vuxDriverSettings->globalPosTolMod_ )) ||
		(vuabs(entity_->YPos() - dry_) > ((scale*vuxDriverSettings->ytol_) *vuxDriverSettings->globalPosTolMod_ )) ||
		(vuabs(entity_->ZPos() - drz_) > ((scale*vuxDriverSettings->ztol_) *vuxDriverSettings->globalPosTolMod_ )) )
   {
	result = 1;
   }
	
	SM_SCALAR rotdelta = vuabs(entity_->Yaw() - dryaw_);
	
	if (rotdelta > VU_PI)
		rotdelta = VU_TWOPI - rotdelta;
// has yaw moved	
	if (rotdelta > (scale*vuxDriverSettings->yawtol_ *vuxDriverSettings->globalAngleTolMod_))
	result = 1;
	



	if (fineupdatescale <0.1f && (entity_->XDelta() + entity_->YDelta() + entity_->ZDelta() != 0.0f))
	result = result + 2;//me123
	

	return result;
}



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

SM_SCALAR VuMaster::CalcError (void)
{
	VuGameEntity
		*game;

	VU_TIME
		dt;

	BIG_SCALAR
		xdist,
		ydist,
		zdist,
		error,
		dist,
		range;

	game  = vuLocalSessionEntity->Game();

	if (game)
	{
		VuSessionsIterator iter(game);
		VuSessionEntity*   sess;
		
		sess = iter.GetFirst();

		range = entity_->EntityType()->fineUpdateRange_ * 6.0f;
	
		while (sess)
		{
			if (sess != vuLocalSessionEntity)
			{
				for (int i = 0; i < 1 /*sess->CameraCount()*/; i++)
				{
					VuEntity *camera = sess->Camera(i);
					if (camera&& camera !=entity_)
					{
						xdist = vuabs(entity_->XPos() - camera->XPos()) - vuabs(entity_->XDelta() + camera->XDelta());
						ydist = vuabs(entity_->YPos() - camera->YPos()) - vuabs(entity_->YDelta() + camera->YDelta());
						zdist = vuabs(entity_->ZPos() - camera->ZPos()) - vuabs(entity_->ZDelta() + camera->ZDelta());
						dist  = vumax(vumax(xdist, ydist), zdist);

						if (camera->XPos() == 0 && camera->YPos() == 0 && camera->ZPos() == 0)
							dist = entity_->EntityType()->fineUpdateForceRange_ + 1;

						if (dist < range)
							range = dist;
					}
				}
			}

			sess = iter.GetNext ();
		}

		dt = vuxRealTime - lastPositionUpdateTime_;

		error = (vuabs(entity_->XPos() - drx_)) + (vuabs(entity_->YPos() - dry_)) + (vuabs(entity_->ZPos() - drz_));

		return error * dt / range;
	}

	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

SM_SCALAR VuMaster::CheckForceUpd (VuSessionEntity* sess)
{

	BIG_SCALAR
		xdist,
		ydist,
		zdist,
		dist,
		range;

		range = entity_->EntityType()->fineUpdateRange_;
	
			if (sess != vuLocalSessionEntity)
			{
				for (int i = 0; i < 1 /*sess->CameraCount()*/; i++)
				{
					VuEntity *camera = sess->Camera(i);
					if (camera&& camera !=entity_)
					{
						xdist = vuabs(entity_->XPos() - camera->XPos()) - vuabs(entity_->XDelta() + camera->XDelta());
						ydist = vuabs(entity_->YPos() - camera->YPos()) - vuabs(entity_->YDelta() + camera->YDelta());
						zdist = vuabs(entity_->ZPos() - camera->ZPos()) - vuabs(entity_->ZDelta() + camera->ZDelta());
						dist  = vumax(vumax(xdist, ydist), zdist);

						if (camera->XPos() == 0 && camera->YPos() == 0 && camera->ZPos() == 0)
							dist = entity_->EntityType()->fineUpdateForceRange_ + 1;

						if (dist < range)
							range = dist;
					}
				}
				if (range < entity_->EntityType()->fineUpdateRange_)
				return  entity_->EntityType()->fineUpdateRange_/range;// we are inside force upd
				else return 0.0f;// outside
			}
	return 0.0f;// no camera
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void
VuMaster::ExecModelWithDR()
{
  entity_->SetPosition(drx_, dry_, drz_);

#if defined(VU_USE_QUATERNION)
  entity_->SetQuat(drquat_);
#else
  entity_->SetYPR(dryaw_, drpitch_, drroll_);
#endif

  entity_->SetUpdateTime(lastUpdateTime_);
}

VU_ERRCODE 
VuMaster::GeneratePositionUpdate(int oob,VU_TIME, VU_BOOL registrer, VuTargetEntity *target)
{
	VU_ERRCODE retval = VU_SUCCESS;
	
	// send message
	{
	VuPositionUpdateEvent *event = new VuPositionUpdateEvent(entity_,target);
if (registrer)	
		 {
			//  if (lastPositionUpdateTime_ ==0) event->RequestReliableTransmit ();//make sure the first (maby only) update is recieved
			  lastPositionUpdateTime_ = vuxRealTime;
		 }
else lastFinePositionUpdateTime_ = vuxRealTime;

	
/*
		if
		(
			(entity_->XDelta() == 0.0) &&
			(entity_->YDelta() == 0.0) &&
			(entity_->ZDelta() == 0.0)
		)
		{
			event->RequestReliableTransmit ();
		}
		else
*/
		
//	me123 it was never used anyway	if (oob)
		{
if (oob && vuLocalSessionEntity->Game()->OwnerId() == vuLocalSessionEntity->Id()) // host do oob updates
			event->RequestOutOfBandTransmit ();
		}

#ifdef DEBUG_MASTER
		VU_PRINT("Sending position update on entity 0x%x:\n", (int)entity_->Id());
		VU_PRINT("  event->x_ = %.3f (%.3f)\n", event->x_, drx_);
		VU_PRINT("  event->y_ = %.3f (%.3f)\n", event->y_, dry_);
		VU_PRINT("  event->z_ = %.3f (%.3f)\n", event->z_, drz_);
#if 0
		VU_PRINT("  event->dx_ = %.3f (%.3f)\n", event->dx_, drxdelta_);
		VU_PRINT("  event->dy_ = %.3f (%.3f)\n", event->dy_, drydelta_);
		VU_PRINT("  event->dz_ = %.3f (%.3f)\n", event->dz_, drzdelta_);
#endif
		
#ifdef DEBUG_ROTATION
		VU_PRINT("  event->yaw_ = %.3f\n",    event->yaw_);
		VU_PRINT("  event->pitch_ = %.3f\n",  event->pitch_);
		VU_PRINT("  event->roll_ = %.3f\n",   event->roll_);
		//VU_PRINT("  event->dyaw_ = %.3f\n",   event->dyaw_);
		//VU_PRINT("  event->dpitch_ = %.3f\n", event->dpitch_);
		//VU_PRINT("  event->droll_ = %.3f\n",  event->droll_);
#endif
		VU_PRINT(" lastupdatetime = %d  ent->Update() = %d \t timestamp_ = %d\n",
			lastUpdateTime_, entity_->LastUpdateTime(), event->updateTime_);
#endif
		if (VuMessageQueue::PostVuMessage(event) <= 0) {
//			MonoPrint ("Not sending Positional Update\n");
#ifdef VU_QUEUE_DR_UPDATES
#ifdef DEBUG_MASTER
			VU_PRINT(">>>> send failed! -- will resend later...\n");
#endif
			return VU_ERROR; // don't update DR data
#else
			retval = VU_ERROR; // do nothing
#endif
		}
	}

if (!registrer)	pendingUpdate_      = FALSE;

	return retval;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void VuMaster::UpdateDrdata (VU_BOOL registrer)

{	
	pendingRoughUpdate_ = FALSE;    // We need to reset this so we remove it from the rough update list at the same time.
	
	// update dr data
	drx_      = entity_->XPos();
	dry_      = entity_->YPos();
	drz_      = entity_->ZPos();
	
//	assert( drx_ > -1e6F && dry_ > -1e6F && drz_ < 20000.0F);
//	assert( drx_ < 5e6 && dry_ < 5e6 && drz_ > -250000.0F);
	
	drxdelta_ = entity_->XDelta();
	drydelta_ = entity_->YDelta();
	drzdelta_ = entity_->ZDelta();
	
#if defined(VU_USE_QUATERNION)
	VU_QUAT* quat = entity_->Quat();
	ML_QuatCopy(drquat_, *quat);
	VU_VECT* dquat = entity_->DeltaQuat();
	ML_VectCopy(drquatdelta_, *dquat);
	drtheta_ = entity_->Theta();
#else
	dryaw_        = entity_->Yaw();
	drpitch_      = entity_->Pitch();
	drroll_       = entity_->Roll();
//	dryawdelta_   = entity_->YawDelta();
//	drpitchdelta_ = entity_->PitchDelta();
//	drrolldelta_  = entity_->RollDelta();
#endif
	
}
void VuMaster::NoExec(VU_TIME)
{
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void 
VuMaster::Exec(VU_TIME timestamp)
{
	if(timestamp <= lastUpdateTime_)
		return;
	
	VU_BOOL wasMoving = FALSE;
	
	if (entity_->XDelta() + entity_->YDelta() + entity_->ZDelta() != 0.0f)
		wasMoving = TRUE;
	
	VU_BOOL wasTurning = FALSE;
	
#if !defined(VU_USE_QUATERNION)
	if (entity_->YawDelta()+entity_->PitchDelta()+entity_->RollDelta() != 0.0f)
		wasTurning = TRUE;
#endif
	
	VU_BOOL ranModel = ExecModel(timestamp); 
	VU_BOOL isMoving = FALSE;
	
	if (entity_->XDelta() + entity_->YDelta() + entity_->ZDelta() != 0.0f)
		isMoving = TRUE;
	
	VU_BOOL isTurning = FALSE;
	
#if !defined(VU_USE_QUATERNION)
	if (entity_->YawDelta()+entity_->PitchDelta()+entity_->RollDelta() != 0.0f)
		isTurning = TRUE;
#endif
	
	ExecDR(timestamp);

	if (ranModel)
	{
		 { 
			 LastPosUpdateround = timestamp;
				if (!entity_->IsPrivate() && vuGlobalGroup->Connected())
				{
					if (entity_->EntityType()->fineUpdateRange_ > 0.0f&&
						vuLocalSessionEntity->Game()->SessionCount() >1 &&
						(entity_->EntityDriver()->Type() == VU_MASTER_DRIVER)
						)
					{		
#if defined(VU_QUEUE_DR_UPDATES)

						 //dobble check cameras
						if (!g_bMPFix3)
						{
						if (entity_ == vuLocalSessionEntity->Camera(0) &&  pendingUpdate_ && !vuxDriverSettings->updateQueue_->Find(vuLocalSessionEntity->Camera(0)))
						  assert(!"player not in que");//							 pendingUpdate_ = FALSE;
						if (entity_ == vuLocalSessionEntity->Camera(0) && pendingRoughUpdate_ && !vuxDriverSettings->roughUpdateQueue_->Find(vuLocalSessionEntity->Camera(0)))
						  assert(!"player not in que");//							pendingRoughUpdate_ = FALSE;
						}

						int tolerance = CheckTolerance ();
						if ((tolerance == 1.0f || tolerance == 3.0f)&& ( g_bMPFix3 || !vuxDriverSettings->roughUpdateQueue_->Find(Entity())))
						{// rough update to all players
							if(!pendingRoughUpdate_)
							{
							pendingRoughUpdate_ = TRUE;
							vuxDriverSettings->roughUpdateQueue_->Push(Entity());
							}
						}//fine update to specific targets

						if (vuLocalSessionEntity->Game() && vuLocalSessionEntity->Game()->OwnerId() == vuLocalSessionEntity->Id() &&
							tolerance > 1.0f && !pendingUpdate_ && (g_bMPFix3 || !vuxDriverSettings->updateQueue_->Find(Entity())))
						{ 
							vuxDriverSettings->updateQueue_->Push(Entity());
							pendingUpdate_ = TRUE;
						}

						// send a fine update about us self to the host if we are not the host ourself
						else if (  (g_bMPFix3 ||!vuxDriverSettings->updateQueue_->Find(Entity())) &&
									entity_ == vuLocalSessionEntity->Camera(0) && !pendingUpdate_ &&
									vuLocalSessionEntity->Game()&&
									vuLocalSessionEntity->Game()->OwnerId() != vuLocalSessionEntity->Id())
						{ 
							vuxDriverSettings->updateQueue_->Push(Entity());
							pendingUpdate_ = TRUE;
						}
#else	
/*						if (CheckTolerance ())
						{
							GeneratePositionUpdate(timestamp, FALSE, vuLocalSessionEntity->Game());
						}
*/
#endif
						
					}
				}
		 }
	}
	else
	{
		ExecModelWithDR();
	}
}

VU_ERRCODE 
VuMaster::Handle(VuEvent*)
{
  // default is to not handle
  return VU_NO_OP;
}

VU_ERRCODE 
VuMaster::Handle(VuFullUpdateEvent*)
{
  // default is to not handle, as Master is source of data
  // in fact, this function should really never be called.
  return VU_NO_OP;
}

VU_ERRCODE 
VuMaster::Handle(VuPositionUpdateEvent*)
{
  // default is to not handle, as Master is source of data
  return VU_NO_OP;
}

void 
VuMaster::SetPosTolerance(SM_SCALAR x, SM_SCALAR y, SM_SCALAR z)
{
  xtol_ = x;
  ytol_ = y;
  ztol_ = z;
}

#ifdef VU_USE_QUATERNION
void 
VuMaster::SetQuatTolerance(SM_SCALAR tol)
{
  quattol_ = tol;
}
#else // !VU_USE_QUATERNION
void 
VuMaster::SetRotTolerance(SM_SCALAR yaw, 
                          SM_SCALAR pitch,
                          SM_SCALAR roll)
{
  yawtol_   = yaw;
  pitchtol_ = pitch;
  rolltol_  = roll;
}
#endif

//-----------------------------------------
VuSlave::VuSlave(VuEntity* entity) : VuDeadReckon(entity)
{
  lastPositionUpdateTime_ = 0;
  lastFinePositionUpdateTime_ = 0;
  pendingUpdate_      = FALSE;
  pendingRoughUpdate_ = FALSE;
  truex_      = entity->XPos();
  truey_      = entity->YPos();
  truez_      = entity->ZPos();
  truexdelta_ = entity->XDelta();
  trueydelta_ = entity->YDelta();
  truezdelta_ = entity->ZDelta();

#if defined(VU_USE_QUATERNION)
  VU_QUAT* quat = entity->Quat();
  ML_QuatCopy(truequat_, *quat);
  VU_VECT* dquat = entity->DeltaQuat();
  ML_VectCopy(truequatdelta_, *dquat);
  truetheta_ = entity->Theta();
#else
  trueyaw_        = entity->Yaw();
  truepitch_      = entity->Pitch();
  trueroll_       = entity->Roll();
  //trueyawdelta_   = entity->YawDelta();
  //truepitchdelta_ = entity->PitchDelta();
  //truerolldelta_  = entity->RollDelta();
#endif

  lastSmoothTime_       = 0;
  lastRemoteUpdateTime_ = 0;
}

VuSlave::~VuSlave()
{
  // empty
}

int
VuSlave::Type()
{
  return VU_SLAVE_DRIVER;
}

int
VuSlave::DebugString(char* str)
{
  return sprintf(str, "id 0x%x, dr(%.0f,%.0f,%.0f), drd(%.2f,%.2f,%.2f)\ntdr(%.0f,%.0f,%.0f), tdrd(%.2f,%.2f,%.2f)\n",
        (unsigned)entity_->Id(), drx_,dry_,drz_,drxdelta_,drydelta_,drzdelta_,
        truex_,truey_,truez_,truexdelta_,trueydelta_,truezdelta_);
}

BIG_SCALAR 
VuSlave::LinearError(BIG_SCALAR value, 
                     BIG_SCALAR truevalue)
{
  BIG_SCALAR retval = 0.0f;
  BIG_SCALAR error  = vuabs(truevalue-value);

  if (error > vuxDriverSettings->maxJumpDist_)
    retval = (truevalue - value);
 
  return retval;
}

#ifdef VU_USE_QUATERNION
SM_SCALAR 
VuSlave::QuatError(VU_QUAT value, 
                   VU_QUAT truevalue)
{
  SM_SCALAR retval = 0.0f;
  SM_SCALAR error  = vuabs(ML_QuatDot(value, truevalue));

  if (error > vuxDriverSettings->maxJumpAngle_)
    retval = error;

  return retval;
}
#else
SM_SCALAR 
VuSlave::AngleError(SM_SCALAR value, 
                    SM_SCALAR truevalue)
{
  SM_SCALAR retval = 0.0f;
  SM_SCALAR error  = vuabs(truevalue - value);

  if (error > VU_PI) error = VU_TWOPI - error;

  if (error > vuxDriverSettings->maxJumpAngle_) {
    retval = truevalue - value;
    if (retval > VU_PI) {
      retval -= VU_TWOPI;
    } else if (retval < -VU_PI) {
      retval += VU_TWOPI;
    }
  }
  return retval;
}
#endif


SM_SCALAR
VuSlave::SmoothLinear(BIG_SCALAR value, 
                      BIG_SCALAR truevalue, 
                      SM_SCALAR  truedelta, 
                      SM_SCALAR  timeInverse)
{
  SM_SCALAR retval;

  retval = truedelta + (truevalue - value)*timeInverse;
  return retval;
}

#ifndef VU_USE_QUATERNION
SM_SCALAR
VuSlave::SmoothAngle(SM_SCALAR value, 
                     SM_SCALAR truevalue, 
                     SM_SCALAR truedelta, 
                     SM_SCALAR timeInverse)
{
  SM_SCALAR retval;

  retval = truedelta + (truevalue - value) * timeInverse;

  if (retval > VU_PI) {
    retval -= VU_TWOPI;
  } else if (retval < -VU_PI) {
    retval += VU_TWOPI;
  }

  return retval;
}
#endif

// KCK: I think I'm basically doing what Dan had in mind here,
// that is - adding to or subtracting from the entity's deltas
// in order that it will be where it thinks it is supposed to
// be in lookahead ms.
// i.e: We're heading west at 10 ft/s, but we're 20 ft to far east.
// Given a look ahead of 2 seconds, in 2 seconds we should be 20 ft
// further west. However, that's 40 ft from our current position,
// (10 ft/s * 2 s + 20 ft)
// so in order to get there in 2 seconds we'll have to go 20 ft/s
// instead of 10.
// (10 ft/s + 20 ft / 2 s) or (40 ft / 2 s)
// The same principal applies to angular deltas

VU_BOOL
VuSlave::DoSmoothing(VU_TIME lookahead, 
                     VU_TIME timestamp)
{
   if (lookahead <= 0) {
	  drxdelta_ = truexdelta_;
	  drydelta_ = trueydelta_;
	  drzdelta_ = truezdelta_;

	  return FALSE;
   }

   // KCK: I'm going to always do smoothing
   SM_SCALAR ti = vuTicsPerSec/(SM_SCALAR)lookahead;

	if (vuLocalSessionEntity&&
	 vuLocalSessionEntity->Game()&&
	 vuLocalSessionEntity->Game()->OwnerId() != vuLocalSessionEntity->Id() )
	{
	   drxdelta_ = SmoothLinear(drx_, truex_, truexdelta_, ti);
	   drydelta_ = SmoothLinear(dry_, truey_, trueydelta_, ti);
	   drzdelta_ = SmoothLinear(drz_, truez_, truezdelta_, ti);
	}

   // Time stamp last smooth time. Our true values are valid only at this time
   lastSmoothTime_ = timestamp;

   return TRUE;
	
}

void 
VuSlave::AutoExec(VU_TIME timestamp)
{
  Exec(timestamp);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
VuSlave::CheckTolerance()
{
	SM_SCALAR scale = 1.0f;
	SM_SCALAR fineupdatescale	= 1.0f;

#if VU_MAX_SESSION_CAMERAS > 0
	
	// danm_NOTE: this is an obvious case for using a grid tree
	//         -- If a camera grid tree is added in the future, use it here!
	
	VuGameEntity* game;
	BIG_SCALAR    range;
	BIG_SCALAR    m;
	BIG_SCALAR    xdist;
	BIG_SCALAR    ydist;
	BIG_SCALAR    zdist;
	BIG_SCALAR    dist = entity_->EntityType()->fineUpdateRange_ * 8.0f;
	int result = 0;

	range = entity_->EntityType()->fineUpdateRange_ * 8.0f;
	game  = vuLocalSessionEntity->Game();
	
	if (game) {
		VuSessionsIterator iter(game);
		VuSessionEntity*   sess;
		
		sess = iter.GetFirst();
		
		while (sess) {
			if (sess != vuLocalSessionEntity &&
				 sess != (VuSessionEntity*) vuDatabase->Find(entity_->OwnerId())){
				// assume omnicience...
				//        if (sess->CameraCount() == 0) {
				//		      range = entity_->EntityType()->fineUpdateForceRange_;
				//	    }
				
				for (int i = 0; i < 1 /*sess->CameraCount()*/; i++) {
					VuEntity *camera = sess->Camera(i);
					if (camera && camera !=entity_)
					{
						m = camera->EntityType()->fineUpdateMultiplier_;
						
						if (m <= 0.0f)
						{
							m = 1.0f;
						}
						
						xdist = vuabs(entity_->XPos() - camera->XPos()) - vuabs(entity_->XDelta() + camera->XDelta());
						ydist = vuabs(entity_->YPos() - camera->YPos()) - vuabs(entity_->YDelta() + camera->YDelta());
						zdist = vuabs(entity_->ZPos() - camera->ZPos()) - vuabs(entity_->ZDelta() + camera->ZDelta());
						dist  = vumax(vumax(xdist, ydist), zdist);
						dist  = dist/m;
						
						if (camera->XPos() == 0 && camera->YPos() == 0 && camera->ZPos() == 0)
							dist = entity_->EntityType()->fineUpdateForceRange_ + 1;

						if (dist < range)
							range = dist;
					}
				}
			}
			
			sess = iter.GetNext();
		}
	}

	if (range <= entity_->EntityType()->fineUpdateForceRange_) {
		scale = static_cast<float>(0.001);
	}
	else if (range < entity_->EntityType()->fineUpdateRange_* 8.0f &&
		range > entity_->EntityType()->fineUpdateForceRange_) {
		scale =
			(range - entity_->EntityType()->fineUpdateForceRange_) /
			(        entity_->EntityType()->fineUpdateRange_* 8.0f -
			entity_->EntityType()->fineUpdateForceRange_);
		if (scale < 0.1f) 
		{
			fineupdatescale = scale;
			scale = 0.1f;
		}
	}
	else
	{
	return 0;
	}

#endif
// has the possition moved
	if((vuabs(truex_ - drx_) > ((scale*vuxDriverSettings->xtol_) *vuxDriverSettings->globalPosTolMod_ )) ||
		(vuabs(truey_ - dry_) > ((scale*vuxDriverSettings->ytol_) *vuxDriverSettings->globalPosTolMod_ )) ||
		(vuabs(truez_ - drz_) > ((scale*vuxDriverSettings->ztol_) *vuxDriverSettings->globalPosTolMod_ )) )
    {
	 result = 1;
    }
	

	SM_SCALAR rotdelta = vuabs(entity_->Yaw() - dryaw_);
	
	if (rotdelta > VU_PI)
		rotdelta = VU_TWOPI - rotdelta;
// has yaw moved	
	if (rotdelta > (scale*vuxDriverSettings->yawtol_ *vuxDriverSettings->globalAngleTolMod_))
	result = 1;


	if (fineupdatescale < 0.1f && (entity_->XDelta() + entity_->YDelta() + entity_->ZDelta() != 0.0f) )
	result = result + 2;//me123

	return result;


}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

SM_SCALAR VuSlave::CalcError (void)
{
	VuGameEntity
		*game;

	VU_TIME
		dt;

	BIG_SCALAR
		xdist,
		ydist,
		zdist,
		error,
		dist,
		range;

	game  = vuLocalSessionEntity->Game();

	if (game)
	{
		VuSessionsIterator iter(game);
		VuSessionEntity*   sess;
		
		sess = iter.GetFirst();

		range = entity_->EntityType()->fineUpdateRange_ * 6.0f;
	
		while (sess)
		{
			if (sess != vuLocalSessionEntity)
			{
				for (int i = 0; i < 1 /*sess->CameraCount()*/; i++)
				{
					VuEntity *camera = sess->Camera(i);
					if (camera&& camera !=entity_)
					{
						xdist = vuabs(entity_->XPos() - camera->XPos()) - vuabs(entity_->XDelta() + camera->XDelta());
						ydist = vuabs(entity_->YPos() - camera->YPos()) - vuabs(entity_->YDelta() + camera->YDelta());
						zdist = vuabs(entity_->ZPos() - camera->ZPos()) - vuabs(entity_->ZDelta() + camera->ZDelta());
						dist  = vumax(vumax(xdist, ydist), zdist);

						if (camera->XPos() == 0 && camera->YPos() == 0 && camera->ZPos() == 0)
							dist = entity_->EntityType()->fineUpdateForceRange_ + 1;

						if (dist < range)
							range = dist;
					}
				}
			}

			sess = iter.GetNext ();
		}

		dt = vuxRealTime - lastPositionUpdateTime_;

		error = (vuabs(truex_ - drx_ )) + (vuabs(truey_ - dry_ )) + (vuabs( truez_- drz_ ));

		return error * dt / range;
	}

	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

SM_SCALAR VuSlave::CheckForceUpd (VuSessionEntity* sess)
{

	BIG_SCALAR
		xdist,
		ydist,
		zdist,
		dist,
		range;

		range = entity_->EntityType()->fineUpdateRange_;
	
			if (sess != vuLocalSessionEntity)
			{
				for (int i = 0; i < 1 /*sess->CameraCount()*/; i++)
				{
					VuEntity *camera = sess->Camera(i);
					if (camera&& camera !=entity_)
					{
						xdist = vuabs(entity_->XPos() - camera->XPos()) - vuabs(entity_->XDelta() + camera->XDelta());
						ydist = vuabs(entity_->YPos() - camera->YPos()) - vuabs(entity_->YDelta() + camera->YDelta());
						zdist = vuabs(entity_->ZPos() - camera->ZPos()) - vuabs(entity_->ZDelta() + camera->ZDelta());
						dist  = vumax(vumax(xdist, ydist), zdist);

						if (camera->XPos() == 0 && camera->YPos() == 0 && camera->ZPos() == 0)
							dist = entity_->EntityType()->fineUpdateForceRange_ + 1;

						if (dist < range)
							range = dist;
					}
				}
				if (range < entity_->EntityType()->fineUpdateRange_)
				return  entity_->EntityType()->fineUpdateRange_/range;// we are inside force upd
				else return 0.0f;// outside
			}
	return 0.0f;// no camera
} 

VuSlave::GeneratePositionUpdate(int oob,VU_TIME, VU_BOOL registrer, VuTargetEntity *target)
{
	VU_ERRCODE retval = VU_SUCCESS;
	
	// send message
	{

		VuPositionUpdateEvent *event = new VuPositionUpdateEvent(entity_,target);

if (registrer)		
		 {
			  lastPositionUpdateTime_ = vuxRealTime;
		 }
else lastFinePositionUpdateTime_ = vuxRealTime;

if (oob && vuLocalSessionEntity->Game()->OwnerId() == vuLocalSessionEntity->Id()) // host do oob updates
event->RequestOutOfBandTransmit ();

#ifdef DEBUG_MASTER
		VU_PRINT("Sending position update on entity 0x%x:\n", (int)entity_->Id());
		VU_PRINT("  event->x_ = %.3f (%.3f)\n", event->x_, drx_);
		VU_PRINT("  event->y_ = %.3f (%.3f)\n", event->y_, dry_);
		VU_PRINT("  event->z_ = %.3f (%.3f)\n", event->z_, drz_);
#if 0
		VU_PRINT("  event->dx_ = %.3f (%.3f)\n", event->dx_, drxdelta_);
		VU_PRINT("  event->dy_ = %.3f (%.3f)\n", event->dy_, drydelta_);
		VU_PRINT("  event->dz_ = %.3f (%.3f)\n", event->dz_, drzdelta_);
#endif
		
#ifdef DEBUG_ROTATION
		VU_PRINT("  event->yaw_ = %.3f\n",    event->yaw_);
		VU_PRINT("  event->pitch_ = %.3f\n",  event->pitch_);
		VU_PRINT("  event->roll_ = %.3f\n",   event->roll_);
		//VU_PRINT("  event->dyaw_ = %.3f\n",   event->dyaw_);
		//VU_PRINT("  event->dpitch_ = %.3f\n", event->dpitch_);
		//VU_PRINT("  event->droll_ = %.3f\n",  event->droll_);
#endif
		VU_PRINT(" lastupdatetime = %d  ent->Update() = %d \t timestamp_ = %d\n",
			lastUpdateTime_, entity_->LastUpdateTime(), event->updateTime_);
#endif
		if (VuMessageQueue::PostVuMessage(event) <= 0) {
//			MonoPrint ("Not sending Positional Update\n");
#ifdef VU_QUEUE_DR_UPDATES
#ifdef DEBUG_MASTER
			VU_PRINT(">>>> send failed! -- will resend later...\n");
#endif
			return VU_ERROR; // don't update DR data
#else
			retval = VU_ERROR; // do nothing
#endif
		}
	}


if (!registrer)	pendingUpdate_      = FALSE;

	return retval;
}

void VuSlave::UpdateDrdata (VU_BOOL registrer)

{	
	pendingRoughUpdate_ = FALSE;    // We need to reset this so we remove it from the rough update list at the same time.
	
	// update dr data
	drx_      = entity_->XPos();
	dry_      = entity_->YPos();
	drz_      = entity_->ZPos();
	
//	assert( drx_ > -1e6F && dry_ > -1e6F && drz_ < 20000.0F);
//	assert( drx_ < 5e6 && dry_ < 5e6 && drz_ > -250000.0F);
	
	drxdelta_ = entity_->XDelta();
	drydelta_ = entity_->YDelta();
	drzdelta_ = entity_->ZDelta();
	
#if defined(VU_USE_QUATERNION)
	VU_QUAT* quat = entity_->Quat();
	ML_QuatCopy(drquat_, *quat);
	VU_VECT* dquat = entity_->DeltaQuat();
	ML_VectCopy(drquatdelta_, *dquat);
	drtheta_ = entity_->Theta();
#else
	dryaw_        = entity_->Yaw();
	drpitch_      = entity_->Pitch();
	drroll_       = entity_->Roll();
//	dryawdelta_   = entity_->YawDelta();
//	drpitchdelta_ = entity_->PitchDelta();
//	drrolldelta_  = entity_->RollDelta();
#endif
	
	
}

void VuSlave::NoExec (VU_TIME time)
{

	truexdelta_	= entity_->XDelta ();
	trueydelta_	= entity_->YDelta ();
	truezdelta_	= entity_->ZDelta ();
	truex_		= entity_->XPos ();
	truey_		= entity_->YPos ();
	truez_		= entity_->ZPos ();

	trueyaw_	= entity_->Yaw ();
	truepitch_	= entity_->Pitch ();
	trueroll_	= entity_->Roll ();

	lastSmoothTime_ = time;
	lastRemoteUpdateTime_ = time;

	VuDeadReckon::NoExec (time);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void 
VuSlave::Exec(VU_TIME timestamp)
{
//me123 no op
}

VU_ERRCODE 
VuSlave::Handle(VuEvent*)
{
  // by default, don't handle
  return VU_NO_OP;
}

VU_ERRCODE 
VuSlave::Handle(VuFullUpdateEvent* event)
{
	return VU_NO_OP;
 //me123 no op
}

VU_ERRCODE VuSlave::Handle(VuPositionUpdateEvent* event)
{
  return VU_NO_OP;
//me123 no op
}

#ifdef VU_SIMPLE_LATENCY

//-----------------------------------------
VuDelaySlave::VuDelaySlave(VuEntity* entity) : VuSlave(entity)
{
  ddrxdelta_ = 0.0f;
  ddrydelta_ = 0.0f;
  ddrzdelta_ = 0.0f;
}

VuDelaySlave::~VuDelaySlave()
{
  // empty
}

int
VuDelaySlave::Type()
{
  return VU_DELAYED_SLAVE_DRIVER;
}

void 
VuDelaySlave::AutoExec(VU_TIME timestamp)
{
  Exec(timestamp);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void VuDelaySlave::NoExec (VU_TIME time)
{

	 	ddrxdelta_	= entity_->XDelta ();
	 	ddrydelta_	= entity_->YDelta ();
	 	ddrzdelta_	= entity_->ZDelta ();
		 
	VuSlave::NoExec (time);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void 
VuDelaySlave::Exec(VU_TIME timestamp)
{
  SM_SCALAR dt = 0.0f;

  dt =((SM_SCALAR)timestamp - (SM_SCALAR)lastUpdateTime_)*vuTicsPerSecInv;


if (vuLocalSessionEntity&&
	 vuLocalSessionEntity->Game()&&
	 vuLocalSessionEntity->Game()->OwnerId() == vuLocalSessionEntity->Id() )
	 {//me123 if we are the host drive our slave ents too
		if (timestamp >= lastSmoothTime_ + vuxDriverSettings->lookaheadTime_)
		{// we have smoothed to the corect possition now set the corect deltas.
		truexdelta_ = drxdelta_;//me123 the real deltas are stored in the dr data...the true values are smoothed...a little messy
		trueydelta_ = drydelta_;
		truezdelta_ = drzdelta_;
		}
	 truex_ += truexdelta_ * dt;
	 truey_ += trueydelta_ * dt;
	 truez_ += truezdelta_ * dt;    
	 entity_->SetPosition(truex_, truey_, truez_);
	 }


else if (lastRemoteUpdateTime_)//make sure we have atleast one possition update send to us ot the dr data is bad.
{
	if (timestamp >= lastSmoothTime_ + vuxDriverSettings->lookaheadTime_)
	{// we have smoothed to the corect possition now set the corect deltas.
	drxdelta_ = truexdelta_;
	drydelta_ = trueydelta_;
	drzdelta_ = truezdelta_;
	}
	ExecDR(timestamp);
	entity_->SetPosition(drx_, dry_, drz_);
}
else 
{
	if (timestamp >= lastSmoothTime_ + vuxDriverSettings->lookaheadTime_)
	{// we have smoothed to the corect possition now set the corect deltas.
	drxdelta_ = truexdelta_;
	drydelta_ = trueydelta_;
	drzdelta_ = truezdelta_;
	}
	ExecDR(timestamp);
	entity_->SetPosition(truex_, truey_, truez_);
}

#if defined(VU_USE_QUATERNION)
  entity_->SetQuat(drquat_);
#else
  entity_->SetYPR(trueyaw_, truepitch_, trueroll_);
// entity_->SetYPR(dryaw_, drpitch_, drroll_);
#endif

  lastUpdateTime_ = timestamp;
  entity_->SetUpdateTime(timestamp);
  // if we are host then we also take care of sending
// slave entitys
  		if (!entity_->IsPrivate() && vuGlobalGroup->Connected()&&
			entity_->EntityType()->fineUpdateRange_ > 0.0f)
			{
  				if (vuLocalSessionEntity&&
					vuLocalSessionEntity->Game()&&
					(vuLocalSessionEntity->Game()->OwnerId() == vuLocalSessionEntity->Id()))
				{
					if (vuLocalSessionEntity->Game()->SessionCount() >1)
					{		
						int tolerance = CheckTolerance ();
						if (tolerance == 1.0f || tolerance == 3.0f)
						{// rough update to all players
							if(	!pendingRoughUpdate_ && (g_bMPFix3 || !vuxDriverSettings->roughUpdateQueue_->Find(Entity())))
							{
							pendingRoughUpdate_ = TRUE;
							vuxDriverSettings->roughUpdateQueue_->Push(Entity());
							}
						}//fine update to specific targets
						if (tolerance > 1.0f && !pendingUpdate_ && (g_bMPFix3 || !vuxDriverSettings->updateQueue_->Find(Entity())))
						{ 
							vuxDriverSettings->updateQueue_->Push(Entity());
							pendingUpdate_ = TRUE;
						}
						
					}
				}
			}
}

/*
VU_ERRCODE 
VuDelaySlave::Handle(VuFullUpdateEvent *event)
{
  VuSlave::Handle(event);
}
*/

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE VuDelaySlave::Handle(VuPositionUpdateEvent* event)
{
	VuSessionEntity* session = (VuSessionEntity*)vuDatabase->Find(entity_->OwnerId());
	SM_SCALAR        dt;

	BIG_SCALAR
	xdist,
	ydist,
	zdist,
	dist;
	extern float g_nMpdelaytweakfactor;
	assert (vuLocalSessionEntity->Id() != event->Sender());// dont send updates to us self
	assert (!session || session->Id() == event->Sender() || //only clients may recieve updates from other then the driver
		vuLocalSessionEntity&&						//and then they got to be from the host
		vuLocalSessionEntity->Game()&&
		vuLocalSessionEntity->Game()->OwnerId() != vuLocalSessionEntity->Id() &&
		vuLocalSessionEntity->Game()->OwnerId() == event->Sender());

	assert (event->dx_ > -10000.0F && event->dy_ > -10000.0F && event->dz_ > -10000.0F &&
		event->x_ > -1e6F && event->y_ > -1e6F && event->z_ < 20000.0F &&
		event->x_ < 5e6 && event->y_ < 5e6 && event->z_ > -250000.0F);

	if (session)
	{
		//		MonoPrint ("VuDS %08x %08x\n", event->updateTime_, lastRemoteUpdateTime_);

		if (event->updateTime_ >= lastRemoteUpdateTime_)
		{
			lastRemoteUpdateTime_ = event->updateTime_;

			// dt is the conversion factor from the remote session's
			// time to our time.
			float tweakfactor = g_nMpdelaytweakfactor;
			dt = (vuxGameTime - (event->updateTime_ + tweakfactor)) * vuTicsPerSecInv * -1.0F;
			dt = vumin(4.0, vumax(-4.0, dt));

			// whats the distance between the true and the update possition ?
			xdist = vuabs(truex_ - event->x_) - vuabs(truexdelta_ + event->dx_);
			ydist = vuabs(truey_ - event->y_) - vuabs(trueydelta_ + event->dy_);
			zdist = vuabs(truez_ - event->z_) - vuabs(truezdelta_ + event->dz_);
			dist  = vumax(vumax(xdist, ydist), zdist);


			// Calculate where we should be now (true position)
			if (vuLocalSessionEntity&&
				vuLocalSessionEntity->Game()&&
				vuLocalSessionEntity->Game()->OwnerId() != vuLocalSessionEntity->Id() )
			{
				assert( event->dx_ > -10000.0F && event->dy_ > -10000.0F && event->dz_ > -10000.0F);
				truexdelta_ = event->dx_;
				trueydelta_ = event->dy_;
				truezdelta_ = event->dz_;
				truex_      = event->x_ + (truexdelta_*dt);
				truey_      = event->y_ + (trueydelta_*dt);
				truez_      = event->z_ + (truezdelta_*dt);
			}

			else if (dist < 5 *6000.0f) // if warp is less then 5nm then smooth
			{
				truexdelta_ = SmoothLinear(truex_, event->x_+ (event->dx_*dt), event->dx_, vuTicsPerSec/(SM_SCALAR) vuxDriverSettings->lookaheadTime_);
				trueydelta_ = SmoothLinear(truey_, event->y_+ (event->dy_*dt), event->dy_, vuTicsPerSec/(SM_SCALAR) vuxDriverSettings->lookaheadTime_);
				truezdelta_ = SmoothLinear(truez_, event->z_+ (event->dz_*dt), event->dz_, vuTicsPerSec/(SM_SCALAR) vuxDriverSettings->lookaheadTime_);
				drxdelta_ = event->dx_;//me123 using thiese to store the real deltas....a little messy ;(
				drydelta_ = event->dy_;
				drzdelta_ = event->dz_;
			}
			else
			{
				assert( event->dx_ > -10000.0F && event->dy_ > -10000.0F && event->dz_ > -10000.0F);
				truexdelta_ = event->dx_;
				trueydelta_ = event->dy_;
				truezdelta_ = event->dz_;
				truex_      = event->x_ + (truexdelta_*dt);
				truey_      = event->y_ + (trueydelta_*dt);
				truez_      = event->z_ + (truezdelta_*dt);
				drxdelta_ = event->dx_;//me123 using thiese to store the real deltas....a little messy ;(
				drydelta_ = event->dy_;
				drzdelta_ = event->dz_;
			}


			#if defined(VU_USE_QUATERNION)
				ML_QuatCopy(drquat_, event->quat_);
				ML_VectCopy(drquatdelta_, event->dquat_);
				drtheta_ = event->theta_;
			#else
				trueyaw_        = event->yaw_;
				truepitch_      = event->pitch_;
				trueroll_       = event->roll_;
			#endif

			// Set new deltas
			lastSmoothTime_ = 0;
			DoSmoothing(vuxDriverSettings->lookaheadTime_, vuxGameTime);

			if (vuLocalSessionEntity&&
			vuLocalSessionEntity->Game()&&
			vuLocalSessionEntity->Game()->OwnerId() != vuLocalSessionEntity->Id() )
			entity_->SetDelta(drxdelta_, drydelta_, drzdelta_);
			else entity_->SetDelta(truexdelta_, trueydelta_, truezdelta_);

			#if !defined(VU_USE_QUATERNION)
				//entity_->SetYPRDelta(dryawdelta_, drpitchdelta_, drrolldelta_);
			#endif
		}

		return VU_SUCCESS;
	}
	assert (!"pos update recived of ent with no owner !");
	return VU_ERROR;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_BOOL VuDelaySlave::DoSmoothing(VU_TIME lookahead, VU_TIME timestamp)
{
//	float drdx = drxdelta_;
//	float drdy = drydelta_;
//	float drdz = drzdelta_;

	if (VuSlave::DoSmoothing(lookahead, timestamp))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#endif VU_SIMPLE_LATENCY
