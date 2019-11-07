// vuevent.h - Copyright (c) Tue July 2 15:01:01 1996,  Spectrum HoloByte, Inc.  All Rights Reserved

#ifndef _VUEVENT_H_
#define _VUEVENT_H_

#ifdef USE_SH_POOLS
extern MEM_POOL gVuMsgMemPool;
#endif

#define VU_UNKNOWN_MESSAGE		0		// 0x00000001

// error message
#define VU_ERROR_MESSAGE		1		// 0x00000002

// request messages
#define VU_GET_REQUEST_MESSAGE		2		// 0x00000004
#define VU_PUSH_REQUEST_MESSAGE		3		// 0x00000008
#define VU_PULL_REQUEST_MESSAGE		4		// 0x00000010

// internal events
#define VU_TIMER_EVENT			5		// 0x00000020
#define VU_RELEASE_EVENT		6		// 0x00000040

// event messages
#define VU_DELETE_EVENT			7		// 0x00000080
#define VU_UNMANAGE_EVENT		8		// 0x00000100
#define VU_MANAGE_EVENT			9		// 0x00000200
#define VU_CREATE_EVENT			10		// 0x00000400
#define VU_SESSION_EVENT		11		// 0x00000800
#define VU_TRANSFER_EVENT		12		// 0x00001000
#define VU_BROADCAST_GLOBAL_EVENT	13		// 0x00002000
#define VU_POSITION_UPDATE_EVENT	14		// 0x00004000
#define VU_FULL_UPDATE_EVENT		15		// 0x00008000
#define VU_RESERVED_UPDATE_EVENT	16		// 0x00010000 ***
#define VU_ENTITY_COLLISION_EVENT	17		// 0x00020000 ***
#define VU_GROUND_COLLISION_EVENT	18		// 0x00040000 ***

// shutdown event
#define VU_SHUTDOWN_EVENT	        19		// 0x00080000

// latency/timing message
#define VU_TIMING_MESSAGE			20		// 0x00100000

#define VU_LAST_EVENT			20		// 0x00100000

// handy-dandy bit combinations
#define VU_VU_MESSAGE_BITS		0x001ffffe
#define VU_REQUEST_MSG_BITS		0x0000001c
#define VU_VU_EVENT_BITS		0x000fffe0
#define VU_DELETE_EVENT_BITS		0x000800c0
#define VU_CREATE_EVENT_BITS		0x00000600
#define VU_TIMER_EVENT_BITS		0x00000020
#define VU_INTERNAL_EVENT_BITS		0x00080060
#define VU_EXTERNAL_EVENT_BITS		0x0017ff82
#define VU_USER_MESSAGE_BITS		0xffe00000

// error messages
#define VU_UNKNOWN_ERROR				0
#define VU_NO_SUCH_ENTITY_ERROR			1
#define VU_CANT_MANAGE_ENTITY_ERROR		2	// for push request denial
#define VU_DONT_MANAGE_ENTITY_ERROR		3	// for pull request denial
#define VU_CANT_TRANSFER_ENTITY_ERROR	4	// for non-transferrable ents
#define VU_TRANSFER_ASSOCIATION_ERROR	5	// for association errors
#define VU_NOT_AVAILABLE_ERROR			6	// session too busy or exiting

#define VU_LAST_ERROR			99

// timer types
#define VU_UNKNOWN_TIMER		0
#define VU_DELETE_TIMER			1
#define VU_DELAY_TIMER			1
#define VU_LAST_TIMER			99

// message flags
#define VU_NORMAL_PRIORITY_MSG_FLAG	0x01	// send normal priority
#define VU_OUT_OF_BAND_MSG_FLAG		0x02	// send unbuffered
#define VU_KEEPALIVE_MSG_FLAG		0x04	// this is a keepalive msg
#define VU_RELIABLE_MSG_FLAG		0x08	// attempt to send reliably
#define VU_LOOPBACK_MSG_FLAG		0x10	// post msg to self as well
#define VU_REMOTE_MSG_FLAG		0x20	// msg came from outside
#define VU_SEND_FAILED_MSG_FLAG		0x40	// msg has been sent
#define VU_PROCESSED_MSG_FLAG		0x80	// msg has been processed

// session event subtypes
enum vuEventTypes {
  VU_SESSION_UNKNOWN_SUBTYPE	= 0,
  VU_SESSION_CLOSE,
  VU_SESSION_JOIN_GAME,
  VU_SESSION_CHANGE_GAME,
  VU_SESSION_JOIN_GROUP,
  VU_SESSION_LEAVE_GROUP,
  VU_SESSION_CHANGE_CALLSIGN,
  VU_SESSION_DISTRIBUTE_ENTITIES,
  VU_SESSION_TIME_SYNC,
  VU_SESSION_LATENCY_NOTICE,
};

class VuEntity;
class VuMessage;
class VuTimerEvent;

//--------------------------------------------------
class VuMessageFilter {
public:
  VuMessageFilter() { }
  virtual ~VuMessageFilter() { }
  virtual VU_BOOL Test(VuMessage *event) = 0;
  virtual VuMessageFilter *Copy() = 0;
};

// The VuNullMessageFilter lets everything through
class VuNullMessageFilter : public VuMessageFilter {
public:
  VuNullMessageFilter() : VuMessageFilter() { }
  virtual ~VuNullMessageFilter() { }
  virtual VU_BOOL Test(VuMessage *);
  virtual VuMessageFilter *Copy();
};

// provided default filters
class VuMessageTypeFilter : public VuMessageFilter {
public:
  VuMessageTypeFilter(ulong bitfield);
  virtual ~VuMessageTypeFilter();
  virtual VU_BOOL Test(VuMessage *event);
  virtual VuMessageFilter *Copy();

protected:
  ulong msgTypeBitfield_;
};

// the VuStandardMsgFilter allows only these events:
//   - VuEvent(s) from a remote session
//   - All Delete and Release events
//   - All Create, FullUpdate, and Manage events
// it filters out these messages:
//   - All update events on unknown entities
//   - All update events on local entities
//   - All non-event messages (though this can be overridden)
class VuStandardMsgFilter : public VuMessageFilter {
public:
  VuStandardMsgFilter(ulong bitfield=VU_VU_EVENT_BITS);
  virtual ~VuStandardMsgFilter();
  virtual VU_BOOL Test(VuMessage *event);
  virtual VuMessageFilter *Copy();

protected:
  ulong msgTypeBitfield_;
};

//--------------------------------------------------
class VuMessageQueue {
  friend class VuTargetEntity;
public:
  VuMessageQueue(int queueSize, VuMessageFilter *filter = 0);
  ~VuMessageQueue();

  VuMessage *PeekVuMessage() { return *read_; }
  virtual VuMessage *DispatchVuMessage(VU_BOOL autod = FALSE);
  int DispatchAllMessages(VU_BOOL autod = FALSE);
  static int PostVuMessage(VuMessage *event);
  static void FlushAllQueues();
  static int InvalidateMessages(VU_BOOL (*evalFunc)(VuMessage*, void*), void *);
  int InvalidateQueueMessages(VU_BOOL (*evalFunc)(VuMessage*, void*), void *);

protected:
  // called when queue is about to wrap -- default does nothing & returns FALSE
  virtual VU_BOOL ReallocQueue();

  virtual int AddMessage(VuMessage *event); // called only by PostVuMessage()
  static void RepostMessage(VuMessage *event, int delay);

// DATA
protected:
  VuMessage **head_;	// also queue mem store
  VuMessage **read_;
  VuMessage **write_;
  VuMessage **tail_;

  VuMessageFilter *filter_;
#ifdef _DEBUG
  int _count; // JPO - see what occupancy is like
#endif

private:
  static VuMessageQueue *queuecollhead_;
  VuMessageQueue *nextqueue_;
};

//--------------------------------------------------
class VuMainMessageQueue : public VuMessageQueue {
public:
  VuMainMessageQueue(int queueSize, VuMessageFilter *filter = 0);
  ~VuMainMessageQueue();

  virtual VuMessage *DispatchVuMessage(VU_BOOL autod = FALSE);

protected:
  virtual int AddMessage(VuMessage *event); // called only by PostVuMessage()

// DATA
protected:
  VuTimerEvent *timerlisthead_;
};

//--------------------------------------------------
class VuPendingSendQueue : public VuMessageQueue {
friend class VuMessageQueue;
public:
  VuPendingSendQueue(int queueSize);
  ~VuPendingSendQueue();

  virtual VuMessage *DispatchVuMessage(VU_BOOL autod = FALSE);
  void RemoveTarget(VuTargetEntity *target);

  int BytesPending() { return bytesPending_; }

protected:
  virtual int AddMessage(VuMessage *event); // called only by PostVuMessage()

// DATA
protected:
  int bytesPending_;
};

//--------------------------------------------------
//--------------------------------------------------
class VuMessage {
friend class VuTargetEntity;
friend class VuMessageQueue;
#ifdef USE_SH_POOLS
public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(gVuMsgMemPool,size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
public:
  virtual ~VuMessage();

  VU_MSG_TYPE Type() { return type_; }
  VU_ID Sender() { return sender_; }
  VU_ID Destination() { return tgtid_; }
  VU_BOOL IsLocal() { return (VU_BOOL)(sender_.creator_==vuLocalSession.creator_ ?
      					TRUE : FALSE); }
  VU_ID EntityId() { return entityId_; }
  VU_BYTE Flags() { return flags_; }
  VuEntity *Entity() { return ent_; }
  VU_TIME PostTime() { return postTime_; }
  VuTargetEntity *Target() { return target_; }

  void SetPostTime(VU_TIME posttime) { postTime_ = posttime; }
  virtual int Size();
  int Read(VU_BYTE **buf, int length);
  int Write(VU_BYTE **buf);
  VU_ERRCODE Dispatch(VU_BOOL autod);
  int Send();

  void RequestLoopback() { flags_ |= VU_LOOPBACK_MSG_FLAG; }
  void RequestReliableTransmit() { flags_ |= VU_RELIABLE_MSG_FLAG; }
  void RequestOutOfBandTransmit() { flags_ |= VU_OUT_OF_BAND_MSG_FLAG; }
  void RequestLowPriorityTransmit() { flags_ &= 0xf0; }

// app needs to Ref & UnRef messages they keep around
// 	most often this need not be done
  int Ref();
  int UnRef();

  // the following determines just prior to sending message whether or not
  // it goes out on the wire (default is TRUE, of course)
  virtual VU_BOOL DoSend();

protected:
  VuMessage(VU_MSG_TYPE type, VU_ID entityId, VuTargetEntity *target,
                VU_BOOL loopback);
  VuMessage(VU_MSG_TYPE type, VU_ID sender, VU_ID target);
  virtual VU_ERRCODE Activate(VuEntity *ent);
  virtual VU_ERRCODE Process(VU_BOOL autod) = 0;
  virtual int Encode(VU_BYTE **buf);
  virtual int Decode(VU_BYTE **buf, int length);
  VuEntity *SetEntity(VuEntity *ent);

private:
  int LocalSize();

private:
  VU_BYTE refcnt_;		// vu references

protected:
  VU_MSG_TYPE type_;
  VU_BYTE flags_;		// misc flags
  VU_ID sender_;
  VU_ID tgtid_;
  VU_ID entityId_;
  // scratch variables (not networked)
  VuTargetEntity *target_;
  VU_TIME postTime_;
private:
  VuEntity *ent_;
};

//--------------------------------------------------
class VuErrorMessage : public VuMessage {
public:
  VuErrorMessage(int errorType, VU_ID senderid, VU_ID entityid, VuTargetEntity *target);
  VuErrorMessage(VU_ID senderid, VU_ID targetid);
  virtual ~VuErrorMessage();

  virtual int Size();
  virtual int Decode(VU_BYTE **buf, int length);
  virtual int Encode(VU_BYTE **buf);
  ushort ErrorType() { return etype_; }

protected:
  virtual VU_ERRCODE Process(VU_BOOL autod);

private:
  int LocalSize();

protected:
  VU_ID srcmsgid_;
  short etype_;
};

//--------------------------------------------------
class VuRequestMessage : public VuMessage {
public:
  virtual ~VuRequestMessage();

protected:
  VuRequestMessage(VU_MSG_TYPE type, VU_ID entityid, VuTargetEntity *target);
  VuRequestMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID entityid);
  virtual VU_ERRCODE Process(VU_BOOL autod) = 0;

// DATA
protected:
  // none
};

//--------------------------------------------------
enum VU_SPECIAL_GET_TYPE {
  VU_GET_GAME_ENTS,
  VU_GET_GLOBAL_ENTS
};

class VuGetRequest : public VuRequestMessage {
public:
  VuGetRequest(VU_SPECIAL_GET_TYPE gettype, VuSessionEntity *sess = 0);
  VuGetRequest(VU_ID entityId, VuTargetEntity *target);
  VuGetRequest(VU_ID senderid, VU_ID target);
  virtual ~VuGetRequest();

protected:
  virtual VU_ERRCODE Process(VU_BOOL autod);
};

//--------------------------------------------------
class VuPushRequest : public VuRequestMessage {
public:
  VuPushRequest(VU_ID entityId, VuTargetEntity *target);
  VuPushRequest(VU_ID senderid, VU_ID target);
  virtual ~VuPushRequest();

protected:
  virtual VU_ERRCODE Process(VU_BOOL autod);
};

//--------------------------------------------------
class VuPullRequest : public VuRequestMessage {
public:
  VuPullRequest(VU_ID entityId, VuTargetEntity *target);
  VuPullRequest(VU_ID senderid, VU_ID target);
  virtual ~VuPullRequest();

protected:
  virtual VU_ERRCODE Process(VU_BOOL autod);
};

//--------------------------------------------------
class VuEvent : public VuMessage {
public:
  virtual ~VuEvent();

  virtual int Size();
  virtual int Decode(VU_BYTE **buf, int length);
  virtual int Encode(VU_BYTE **buf);

protected:
  VuEvent(VU_MSG_TYPE type, VU_ID entityId, VuTargetEntity *target,
                VU_BOOL loopback=FALSE);
  VuEvent(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
  virtual VU_ERRCODE Activate(VuEntity *ent);
  virtual VU_ERRCODE Process(VU_BOOL autod) = 0;

private:
  int LocalSize();

// DATA
public:
  // these fields are filled in on Activate()
  VU_TIME updateTime_; 
};

//--------------------------------------------------
class VuCreateEvent : public VuEvent {
public:
  VuCreateEvent(VuEntity *entity, VuTargetEntity *target, VU_BOOL loopback=FALSE);
  VuCreateEvent(VU_ID senderid, VU_ID target);
  virtual ~VuCreateEvent();

  virtual int Size();
  virtual int Decode(VU_BYTE **buf, int length);
  virtual int Encode(VU_BYTE **buf);
  virtual VU_BOOL DoSend();     // returns TRUE if ent is in database

  VuEntity *EventData() { return expandedData_; }

protected:
  VuCreateEvent(VU_MSG_TYPE type, VuEntity *entity, VuTargetEntity *target,
                        VU_BOOL loopback=FALSE);
  VuCreateEvent(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);

  virtual VU_ERRCODE Activate(VuEntity *ent);
  virtual VU_ERRCODE Process(VU_BOOL autod);

private:
  int LocalSize();

// data
//protected:
public:
  VuEntity *expandedData_;
#ifdef VU_USE_CLASS_INFO
  VU_BYTE classInfo_[CLASS_NUM_BYTES];	// entity class type
#endif
  ushort vutype_;			// entity type
  ushort size_;
  VU_BYTE *data_;
};

//--------------------------------------------------
class VuManageEvent : public VuCreateEvent {
public:
  VuManageEvent(VuEntity *entity, VuTargetEntity *target, VU_BOOL loopback=FALSE);
  VuManageEvent(VU_ID senderid, VU_ID target);
  virtual ~VuManageEvent();

// data
protected:
  // none
};

//--------------------------------------------------
class VuDeleteEvent : public VuEvent {
public:
  VuDeleteEvent(VuEntity *entity);
  VuDeleteEvent(VU_ID senderid, VU_ID target);
  virtual ~VuDeleteEvent();

protected:
  virtual VU_ERRCODE Process(VU_BOOL autod);
  virtual VU_ERRCODE Activate(VuEntity *ent);

// data
protected:
  // none
};

//--------------------------------------------------
class VuUnmanageEvent : public VuEvent {
public:
  VuUnmanageEvent(VuEntity *entity, VuTargetEntity *target,
                        VU_TIME mark, VU_BOOL loopback = FALSE);
  VuUnmanageEvent(VU_ID senderid, VU_ID target);
  virtual ~VuUnmanageEvent();

  virtual int Size();
  virtual int Decode(VU_BYTE **buf, int length);
  virtual int Encode(VU_BYTE **buf);

protected:
  virtual VU_ERRCODE Process(VU_BOOL autod);

private:
  int LocalSize();

// data
public:
  // time of removal
  VU_TIME mark_;
};

//--------------------------------------------------
class VuReleaseEvent : public VuEvent {
public:
  VuReleaseEvent(VuEntity *entity);
  virtual ~VuReleaseEvent();

  virtual int Size();
  // all these are stubbed out here, as this is not a net message
  virtual int Decode(VU_BYTE **buf, int length);
  virtual int Encode(VU_BYTE **buf);
  virtual VU_BOOL DoSend();     // returns FALSE

protected:
  virtual VU_ERRCODE Activate(VuEntity *ent);
  virtual VU_ERRCODE Process(VU_BOOL autod);

// data
protected:
  // none
};

//--------------------------------------------------
class VuTransferEvent : public VuEvent {
public:
  VuTransferEvent(VuEntity *entity, VuTargetEntity *target, VU_BOOL loopback=FALSE);
  VuTransferEvent(VU_ID senderid, VU_ID target);
  virtual ~VuTransferEvent();

  virtual int Size();
  virtual int Decode(VU_BYTE **buf, int length);
  virtual int Encode(VU_BYTE **buf);

protected:
  virtual VU_ERRCODE Activate(VuEntity *ent);
  virtual VU_ERRCODE Process(VU_BOOL autod);

private:
  int LocalSize();

// data
public:
  VU_ID newOwnerId_;
};

//--------------------------------------------------
class VuPositionUpdateEvent : public VuEvent {
public:
  VuPositionUpdateEvent(VuEntity *entity, VuTargetEntity *target,
                        VU_BOOL loopback=FALSE);
  VuPositionUpdateEvent(VU_ID senderid, VU_ID target);
  virtual ~VuPositionUpdateEvent();

  virtual int Size();
  virtual int Decode(VU_BYTE **buf, int length);
  virtual int Encode(VU_BYTE **buf);

  virtual VU_BOOL DoSend();

protected:
  virtual VU_ERRCODE Process(VU_BOOL autod);

private:
  int LocalSize();

// data
public:
#ifdef VU_USE_QUATERNION
  VU_QUAT quat_;	// quaternion indicating current facing
  VU_VECT dquat_;	// unit vector expressing quaternion delta
  SM_SCALAR theta_;	// scalar indicating rate of above delta
#else // !VU_USE_QUATERNION
  SM_SCALAR yaw_, pitch_, roll_;
  SM_SCALAR dyaw_, dpitch_, droll_;
#endif
  BIG_SCALAR x_, y_, z_;
  SM_SCALAR dx_, dy_, dz_;
};

//--------------------------------------------------
class VuBroadcastGlobalEvent : public VuEvent {
public:
  VuBroadcastGlobalEvent(VuEntity *entity, VuTargetEntity *target, VU_BOOL loopback=FALSE);
  VuBroadcastGlobalEvent(VU_ID senderid, VU_ID target);
  virtual ~VuBroadcastGlobalEvent();

  void MarkAsKeepalive() { flags_ |= VU_KEEPALIVE_MSG_FLAG; }

protected:
  virtual int Size();
  virtual int Decode(VU_BYTE **buf, int length);
  virtual int Encode(VU_BYTE **buf);

  virtual VU_ERRCODE Activate(VuEntity *ent);
  virtual VU_ERRCODE Process(VU_BOOL autod);

// data
protected:

#ifdef VU_USE_CLASS_INFO
  VU_BYTE classInfo_[CLASS_NUM_BYTES];	// entity class type
#endif
  ushort vutype_;			// entity type

};

//--------------------------------------------------
class VuFullUpdateEvent : public VuCreateEvent {
public:
  VuFullUpdateEvent(VuEntity *entity, VuTargetEntity *target, VU_BOOL loopback=FALSE);
  VuFullUpdateEvent(VU_ID senderid, VU_ID target);
  virtual ~VuFullUpdateEvent();

  VU_BOOL WasCreated() { return (VU_BOOL)(Entity() == expandedData_ ? TRUE : FALSE); }
  void MarkAsKeepalive() { flags_ |= VU_KEEPALIVE_MSG_FLAG; }

protected:
  virtual VU_ERRCODE Activate(VuEntity *ent);

// data
protected:
  // none
};

//--------------------------------------------------
class VuEntityCollisionEvent : public VuEvent {
public:
  VuEntityCollisionEvent(VuEntity *entity, VU_ID otherId,
                         VU_DAMAGE hitLocation, int hitEffect,
                         VuTargetEntity *target, VU_BOOL loopback=FALSE);
  VuEntityCollisionEvent(VU_ID senderid, VU_ID target);

  virtual ~VuEntityCollisionEvent();

  virtual int Size();
  virtual int Decode(VU_BYTE **buf, int length);
  virtual int Encode(VU_BYTE **buf);

protected:
  virtual VU_ERRCODE Process(VU_BOOL autod);

private:
  int LocalSize();

// data
public:
  VU_ID otherId_;
  VU_DAMAGE hitLocation_;	// affects damage
  int hitEffect_;		// affects hitpoints/health
};

//--------------------------------------------------
class VuGroundCollisionEvent : public VuEvent {
public:
  VuGroundCollisionEvent(VuEntity *entity, VuTargetEntity *target, VU_BOOL loopback=FALSE);
  VuGroundCollisionEvent(VU_ID senderid, VU_ID target);
  virtual ~VuGroundCollisionEvent();

protected:
  virtual VU_ERRCODE Process(VU_BOOL autod);

// data
};

//--------------------------------------------------
class VuSessionEvent : public VuEvent {
public:
  VuSessionEvent(VuEntity *entity, ushort subtype, VuTargetEntity *target,
                 VU_BOOL loopback=FALSE);
  VuSessionEvent(VU_ID senderid, VU_ID target);
  virtual ~VuSessionEvent();

  virtual int Size();
  virtual int Decode(VU_BYTE **buf, int length);
  virtual int Encode(VU_BYTE **buf);

protected:
  virtual VU_ERRCODE Process(VU_BOOL autod);

private:
  int LocalSize();

// data
public:
  ushort subtype_;
  VU_ID group_;
  char *callsign_;
  VU_BYTE syncState_;
  VU_TIME gameTime_;
};

//--------------------------------------------------
class VuTimerEvent : public VuEvent {
friend class VuMainMessageQueue;
public:
  VuTimerEvent(VuEntity *entity, VU_TIME mark, ushort type, VuMessage *event=0);
  virtual ~VuTimerEvent();

  virtual int Size();
  // all these are stubbed out here, as this is not a net message
  virtual int Decode(VU_BYTE **buf, int length);
  virtual int Encode(VU_BYTE **buf);
  virtual VU_BOOL DoSend();     // returns FALSE

protected:
  virtual VU_ERRCODE Process(VU_BOOL autod);

// data
public:
  // time of firing
  VU_TIME mark_;
  ushort timertype_;
  // event to launch on firing
  VuMessage *event_;

private:
  VuTimerEvent *next_;
};

//--------------------------------------------------
class VuShutdownEvent : public VuEvent {
public:
  VuShutdownEvent(VU_BOOL all = FALSE);
  virtual ~VuShutdownEvent();

  virtual int Size();
  // all these are stubbed out here, as this is not a net message
  virtual int Decode(VU_BYTE **buf, int length);
  virtual int Encode(VU_BYTE **buf);
  virtual VU_BOOL DoSend();     // returns FALSE

protected:
  virtual VU_ERRCODE Process(VU_BOOL autod);

// data
public:
  VU_BOOL shutdownAll_;
  VU_BOOL done_;
};

#ifdef VU_SIMPLE_LATENCY
//--------------------------------------------------
class VuTimingMessage : public VuMessage {
public:
  VuTimingMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=FALSE);
  VuTimingMessage(VU_ID senderid, VU_ID target);
  virtual ~VuTimingMessage();

  virtual int Size();
  virtual int Decode(VU_BYTE **buf, int length);
  virtual int Encode(VU_BYTE **buf);

protected:
  virtual VU_ERRCODE Process(VU_BOOL autod);

// data
public:
	VU_TIME	sessionRealSendTime_;
	VU_TIME sessionGameSendTime_;
	VU_TIME remoteGameTime_;
};
#endif VU_SIMPLE_LATENCY

//--------------------------------------------------
class VuUnknownMessage : public VuMessage {
public:
  VuUnknownMessage();
  virtual ~VuUnknownMessage();

  virtual int Size();
  // all these are stubbed out here, as this is not a net message
  virtual int Decode(VU_BYTE **buf, int length);
  virtual int Encode(VU_BYTE **buf);
  virtual VU_BOOL DoSend();     // returns FALSE

protected:
  virtual VU_ERRCODE Process(VU_BOOL autod);

// data
public:
  // none
};

#endif // _VUEVENT_H_
