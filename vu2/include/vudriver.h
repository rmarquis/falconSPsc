// vudriver.h - Copyright (c) Tue July 2 15:01:01 1996,  Spectrum HoloByte, Inc.  All Rights Reserved

#ifndef _VUDRIVER_H_
#define _VUDRIVER_H_

// forward decls
class VuEntity;
class VuEvent;

// externs
extern SM_SCALAR vuTicsPerSec;
extern SM_SCALAR vuTicsPerSecInv;

// types
#define VU_UNKNOWN_DRIVER		0
#define VU_MASTER_DRIVER	        1
#define VU_SLAVE_DRIVER			2
#define VU_DELAYED_SLAVE_DRIVER		3

//-----------------------------------------
class VuDriverSettings {
public:
  VuDriverSettings(
			SM_SCALAR x, SM_SCALAR y, SM_SCALAR z,
#ifdef VU_USE_QUATERNION
			SM_SCALAR quatDist,
#else // !VU_USE_QUATERNION
  			SM_SCALAR yaw, SM_SCALAR pitch, SM_SCALAR roll,
#endif
			SM_SCALAR maxJumpDist, SM_SCALAR maxJumpAngle,
			VU_TIME lookaheadTime
#ifdef VU_QUEUE_DR_UPDATES
            , VuFilter *filter = 0
#endif
                        );
  ~VuDriverSettings();

  SM_SCALAR xtol_, ytol_, ztol_;	// fine tolerance values
#ifdef VU_USE_QUATERNION
  SM_SCALAR quattol_;
#else // !VU_USE_QUATERNION
  SM_SCALAR yawtol_, pitchtol_, rolltol_;
#endif
  SM_SCALAR maxJumpDist_;
  SM_SCALAR maxJumpAngle_;
  SM_SCALAR globalPosTolMod_;
  SM_SCALAR globalAngleTolMod_;
  VU_TIME lookaheadTime_;
#ifdef VU_QUEUE_DR_UPDATES
  VuFifoQueue *updateQueue_;
  VuFifoQueue *roughUpdateQueue_;
#endif
};

//-----------------------------------------
class VuDriver {
public:

  virtual ~VuDriver();

  virtual int Type() = 0;

  virtual void NoExec(VU_TIME timestamp) = 0;
  virtual void Exec(VU_TIME timestamp) = 0;
  virtual void AutoExec(VU_TIME timestamp) = 0;

  virtual VU_ERRCODE Handle(VuEvent *event) = 0;
  virtual VU_ERRCODE Handle(VuFullUpdateEvent *event) = 0;
  virtual VU_ERRCODE Handle(VuPositionUpdateEvent *event) = 0;

  virtual void AlignTimeAdd(VU_TIME timeDelta) = 0;
  virtual void AlignTimeSubtract(VU_TIME timeDelta) = 0;
  virtual void ResetLastUpdateTime (VU_TIME time) = 0;

  // Accessor
  VuEntity *Entity() { return entity_; }

  // AI hooks
  virtual int AiType();		// returns 0 if no AI is present
  virtual VU_BOOL AiExec();	// executes Ai component
  virtual void *AiPointer();	// returns pointer to Ai data (game specific)

  // debug hooks
  virtual int DebugString(char *str);

protected:
  VuDriver(VuEntity *entity);

// Data
protected:
  VuEntity *entity_;
};

//-----------------------------------------
class VuDeadReckon : public VuDriver {
public:
  VuDeadReckon(VuEntity *entity);
  virtual ~VuDeadReckon();

  virtual int Type() = 0;

  virtual VU_ERRCODE Handle(VuEvent *event) = 0;
  virtual VU_ERRCODE Handle(VuFullUpdateEvent *event) = 0;
  virtual VU_ERRCODE Handle(VuPositionUpdateEvent *event) = 0;

  virtual void AlignTimeAdd(VU_TIME timeDelta);
  virtual void AlignTimeSubtract(VU_TIME timeDelta);
  virtual void ResetLastUpdateTime (VU_TIME time);

  virtual void NoExec(VU_TIME timestamp);
  virtual void Exec(VU_TIME timestamp);
  virtual void AutoExec(VU_TIME timestamp);

  virtual void ExecDR(VU_TIME timestamp);

// DATA
protected:
  // dead reckoning
  BIG_SCALAR drx_, dry_, drz_;
  SM_SCALAR drxdelta_, drydelta_, drzdelta_;
#ifdef VU_USE_QUATERNION
  VU_QUAT drquat_;	// quaternion indicating current facing
  VU_VECT drquatdelta_;	// unit vector expressing quaternion delta
  SM_SCALAR drtheta_;	// scalar indicating rate of above delta
#else // !VU_USE_QUATERNION
  SM_SCALAR dryaw_, drpitch_, drroll_;
  //SM_SCALAR dryawdelta_, drpitchdelta_, drrolldelta_;
#endif

  VU_TIME lastUpdateTime_;
};

//-----------------------------------------
class VuMaster : public VuDeadReckon {
public:
  VuMaster(VuEntity *entity);
  virtual ~VuMaster();

  virtual int Type();
  virtual void NoExec(VU_TIME timestamp);
  virtual void Exec(VU_TIME timestamp);

  int CheckTolerance ();
  SM_SCALAR CalcError (void);
  SM_SCALAR CheckForceUpd (VuSessionEntity*);//me123 010115
  VU_TIME lastFinePositionUpdateTime (){ return lastFinePositionUpdateTime_; }
  void SetpendingUpdate(VU_BOOL pendingUpdate) {pendingUpdate_ = pendingUpdate;}//me123
  void SetpendingRoughUpdate(VU_BOOL pendingRoughUpdate) {pendingRoughUpdate_ = pendingRoughUpdate;}//me123
  void UpdateDrdata (VU_BOOL registrer);//me123
  void SetRoughPosTolerance(SM_SCALAR x, SM_SCALAR y, SM_SCALAR z);
  void SetPosTolerance(SM_SCALAR x, SM_SCALAR y, SM_SCALAR z);
#ifdef VU_USE_QUATERNION
  void SetQuatTolerance(SM_SCALAR tol);
#else // !VU_USE_QUATERNION
  void SetRotTolerance(SM_SCALAR yaw, SM_SCALAR pitch, SM_SCALAR roll);
#endif

  // ExecModel must update Pos, YPR & deltas
  //   returns whether model was run
  virtual VU_BOOL ExecModel(VU_TIME timestamp) = 0;

  void ExecModelWithDR();

  virtual VU_ERRCODE Handle(VuEvent *event);
  virtual VU_ERRCODE Handle(VuFullUpdateEvent *event);
  virtual VU_ERRCODE Handle(VuPositionUpdateEvent *event);

  virtual VU_ERRCODE GeneratePositionUpdate(int oob, VU_TIME timestamp, VU_BOOL registrer, VuTargetEntity*);

  // debug hooks
  virtual int DebugString(char *str);

// DATA
protected:
  VU_TIME lastPositionUpdateTime_;
  VU_TIME lastFinePositionUpdateTime_;
  // dead reckoning tolerances
  VU_BOOL pendingUpdate_;
  VU_BOOL pendingRoughUpdate_;
  BIG_SCALAR xtol_, ytol_, ztol_;
#ifdef VU_USE_QUATERNION
  SM_SCALAR quattol_;
#else // !VU_USE_QUATERNION
  SM_SCALAR yawtol_, pitchtol_, rolltol_;
#endif
};

//-----------------------------------------
class VuSlave : public VuDeadReckon {
protected:
enum DeadReckonMode {
  ROUGH,
  TRANSITION,
  FINE,
};
  VU_TIME lastPositionUpdateTime_;
  VU_TIME lastFinePositionUpdateTime_;
  VU_BOOL pendingUpdate_;
  VU_BOOL pendingRoughUpdate_;
  int CheckTolerance ();
  

public:
  VU_TIME lastFinePositionUpdateTime (){ return lastFinePositionUpdateTime_; }
	SM_SCALAR CheckForceUpd (VuSessionEntity*);//me123 010115
	SM_SCALAR CalcError (void);
  void SetpendingUpdate(VU_BOOL pendingUpdate) {pendingUpdate_ = pendingUpdate;}//me123
  void SetpendingRoughUpdate(VU_BOOL pendingRoughUpdate) {pendingRoughUpdate_ = pendingRoughUpdate;}//me123
  void UpdateDrdata (VU_BOOL registrer);//me123
   virtual VU_ERRCODE GeneratePositionUpdate(int oob,VU_TIME timestamp, VU_BOOL registrer, VuTargetEntity*);
  VuSlave(VuEntity *entity);
  virtual ~VuSlave();
  virtual int Type();
  BIG_SCALAR LinearError(BIG_SCALAR value, BIG_SCALAR truevalue);
  SM_SCALAR SmoothLinear(BIG_SCALAR value, 
	      BIG_SCALAR truevalue, SM_SCALAR truedelta, SM_SCALAR timeInverse);
#ifdef VU_USE_QUATERNION
  SM_SCALAR QuatError(VU_QUAT value, VU_QUAT truevalue);
#else // !VU_USE_QUATERNION
  SM_SCALAR AngleError(SM_SCALAR value, SM_SCALAR truevalue);
  SM_SCALAR SmoothAngle(SM_SCALAR value, SM_SCALAR truevalue,
      			SM_SCALAR truedelta, SM_SCALAR timeInverse);
#endif

  virtual void NoExec(VU_TIME timestamp);
  virtual void Exec(VU_TIME timestamp);
  virtual void AutoExec(VU_TIME timestamp);

  virtual VU_ERRCODE Handle(VuEvent *event);
  virtual VU_ERRCODE Handle(VuFullUpdateEvent *event);
  virtual VU_ERRCODE Handle(VuPositionUpdateEvent *event);

  // debug hooks
  virtual int DebugString(char *str);

protected:
  virtual VU_BOOL DoSmoothing(VU_TIME lookahead, VU_TIME timestamp);

// DATA
protected:
  // smoothing
  BIG_SCALAR truex_, truey_, truez_;
  SM_SCALAR truexdelta_, trueydelta_, truezdelta_;
#ifdef VU_USE_QUATERNION
  VU_QUAT truequat_;	// quaternion indicating current facing
  VU_VECT truequatdelta_; // unit vector expressing quaternion delta
  SM_SCALAR truetheta_;	// scalar indicating rate of above delta
#else // !VU_USE_QUATERNION
  SM_SCALAR trueyaw_, truepitch_, trueroll_;
  //SM_SCALAR trueyawdelta_, truepitchdelta_, truerolldelta_;
#endif
  VU_TIME lastSmoothTime_;
  VU_TIME lastRemoteUpdateTime_;
};

//-----------------------------------------
class VuDelaySlave : public VuSlave {
public:
  VuDelaySlave(VuEntity *entity);
  virtual ~VuDelaySlave();

  virtual int Type();

  virtual void NoExec(VU_TIME timestamp);
  virtual void Exec(VU_TIME timestamp);
  virtual void AutoExec(VU_TIME timestamp);

  virtual VU_ERRCODE Handle(VuPositionUpdateEvent *event);

protected:
  virtual VU_BOOL DoSmoothing(VU_TIME lookahead, VU_TIME timestamp);

// DATA
protected:
  SM_SCALAR ddrxdelta_, ddrydelta_, ddrzdelta_;        // accel values
};

#endif // _VUDRIVER_H_
