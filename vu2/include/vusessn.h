// vuentity.h - Copyright (c) Tue July 2 15:01:01 1996,  Spectrum HoloByte, Inc.  All Rights Reserved

#ifndef _VUSESSION_H_
#define _VUSESSION_H_

#include "vuentity.h"
#include "vuevent.h"
#include "vucoll.h"

#define VU_SESSION_NULL_CONNECTION	vuNullId
#define VU_SESSION_NULL_GROUP		vuNullId

#define VU_DEFAULT_PLAYER_NAME 		"anonymous"
#define VU_GAME_GROUP_NAME    	        "Vu2 Game"
#define VU_PLAYER_POOL_GROUP_NAME	"Player Pool"

#define PACKET_HDR_SIZE (vuKnownConnectionId ? 0 : sizeof(VU_SESSION_ID) + sizeof(VU_ID_NUMBER))
#define MIN_MSG_HDR_SIZE (2)     // 1 for type, 1 for min length
#define MAX_MSG_HDR_SIZE (MIN_MSG_HDR_SIZE + 1)
#define MSG_HDR_SIZE (MAX_MSG_HDR_SIZE) // assume larger
#define MIN_HDR_SIZE (PACKET_HDR_SIZE + MAX_MSG_HDR_SIZE)
#define TWO_PLAYER_HDR_SIZE (MSG_HDR_SIZE)

class VuGroupEntity;
class VuGameEntity;

enum VuSessionSync {
  VU_NO_SYNC,
  VU_SLAVE_SYNC,
  VU_MASTER_SYNC,
};

#ifdef VU_USE_COMMS
//----------------------------------------------------------------------
// VuCommsContext -- struct containing comms data
//----------------------------------------------------------------------
enum VuCommsConnectStatus {
  VU_CONN_INACTIVE,
  VU_CONN_PENDING,
  VU_CONN_ACTIVE,
  VU_CONN_ERROR,
};

struct VuCommsContext {
  ComAPIHandle          handle_;
  VuCommsConnectStatus  status_;
  VU_BOOL               reliable_;
  int                   maxMsgSize_;
  int					maxPackSize_;
  // outgoing data
  VU_BYTE               *normalSendPacket_;
  VU_BYTE               *normalSendPacketPtr_;
  VU_ID                 normalPendingSenderId_;
  VU_ID                 normalPendingSendTargetId_;
  // outgoing data
  VU_BYTE               *lowSendPacket_;
  VU_BYTE               *lowSendPacketPtr_;
  VU_ID                 lowPendingSenderId_;
  VU_ID                 lowPendingSendTargetId_;
  // incoming data
  VU_BYTE               *recBuffer_;
  // incoming message portion
  VU_MSG_TYPE           type_;
  int                   length_;
  ushort                msgid_;
  VU_ID                 targetId_;
  VU_TIME               timestamp_;
  // ping data
  int					ping;
};
#endif

//----------------------------------------------------------------------
// VuTargetEntity -- base class -- target for all messages
//----------------------------------------------------------------------
class VuTargetEntity : public VuEntity {
  friend class VuServerThread;
public:
  virtual ~VuTargetEntity();

  // virtual function interface
  virtual int SaveSize();
  virtual int Save(VU_BYTE **stream);
  virtual int Save(FILE *file);

  // Special VU type getters
  virtual VU_BOOL IsTarget();	// returns TRUE

  virtual VU_BOOL HasTarget(VU_ID id)=0; // TRUE --> id contains (or is) ent
  virtual VU_BOOL InTarget(VU_ID id)=0;  // TRUE --> ent contained by (or is) id

#ifdef VU_USE_COMMS
  virtual VuTargetEntity *ForwardingTarget(VuMessage *msg = 0);

  int FlushOutboundMessageBuffer();
  int SendMessage(VuMessage *msg);
  int GetMessages();
  int SendQueuedMessage ();

  void SetDirty (void);
  void ClearDirty (void);
  int IsDirty (void);
  
  // normal (best effort) comms handle
  VuCommsConnectStatus GetCommsStatus() { return bestEffortComms_.status_; }
  ComAPIHandle GetCommsHandle() { return bestEffortComms_.handle_; }
  void SetCommsStatus(VuCommsConnectStatus cs) { bestEffortComms_.status_ = cs;}
  void SetCommsHandle(ComAPIHandle ch, int bufSize=0, int packSize=0);

  // reliable comms handle
  VuCommsConnectStatus GetReliableCommsStatus() {return reliableComms_.status_;}
  ComAPIHandle GetReliableCommsHandle() { return reliableComms_.handle_; }
  void SetReliableCommsStatus(VuCommsConnectStatus cs) { reliableComms_.status_ = cs; }
  void SetReliableCommsHandle(ComAPIHandle ch, int bufSize=0, int packSize=0);

  int BytesPending();
  int MaxPacketSize();
  int MaxMessageSize();
  int MaxReliablePacketSize();
  int MaxReliableMessageSize();

protected:
  int Flush(VuCommsContext *ctxt);
  int FlushLow(VuCommsContext *ctxt);
  int SendOutOfBand(VuCommsContext *ctxt, VuMessage *msg);
  int SendNormalPriority(VuCommsContext *ctxt, VuMessage *msg);
  int SendLowPriority(VuCommsContext *ctxt, VuMessage *msg);
#endif

protected:
  VuTargetEntity(int type);
  VuTargetEntity(VU_BYTE **stream);
  VuTargetEntity(FILE *file);

private:
  int LocalSize();                      // returns local bytes written

//data
protected:
#ifdef VU_USE_COMMS
  VuCommsContext bestEffortComms_;
  VuCommsContext reliableComms_;
#endif
  int dirty;
};

//-----------------------------------------
struct VuGroupNode {
  VU_ID gid_;
  VuGroupNode *next_;
};
enum VU_GAME_ACTION {
  VU_NO_GAME_ACTION,
  VU_JOIN_GAME_ACTION,
  VU_CHANGE_GAME_ACTION,
  VU_LEAVE_GAME_ACTION
};
//-----------------------------------------
class VuSessionEntity : public VuTargetEntity {
friend class VuMainThread;
public:
  // constructors & destructor
  VuSessionEntity(ulong domainMask, char *callsign);
  VuSessionEntity(VU_BYTE **stream);
  VuSessionEntity(FILE *file);
  virtual ~VuSessionEntity();

  // virtual function interface
  virtual int SaveSize();
  virtual int Save(VU_BYTE **stream);
  virtual int Save(FILE *file);

  // accessors
  ulong DomainMask()		{ return domainMask_; }
  VU_SESSION_ID SessionId()	{ return sessionId_; }
  char *Callsign()		{ return callsign_; }
  VU_BYTE LoadMetric()		{ return loadMetric_; }
  VU_ID GameId()		{ return gameId_; }
  VuGameEntity *Game();
  int LastMessageReceived()	{ return lastMsgRecvd_; }
  VU_TIME KeepaliveTime();
  int BandWidth() { return bandwidth_;}//me123

  // setters
  void SetCallsign(char *callsign);
  void SetLoadMetric(VU_BYTE newMetric) { loadMetric_ = newMetric; }
  VU_ERRCODE JoinGroup(VuGroupEntity *newgroup);  //  < 0 retval ==> failure
  VU_ERRCODE LeaveGroup(VuGroupEntity *group);
  VU_ERRCODE LeaveAllGroups();
  VU_ERRCODE JoinGame(VuGameEntity *newgame);  //  < 0 retval ==> failure
  VU_GAME_ACTION GameAction() { return action_; }
  void SetLastMessageReceived(int id)	{ lastMsgRecvd_ = id; }
  void SetKeepaliveTime(VU_TIME ts);
  void SetBandWidth(int bandwidth) {bandwidth_ = bandwidth;}//me123

#ifdef VU_SIMPLE_LATENCY
  int TimeDelta()		{ return timeDelta_; }
  void SetTimeDelta(int delta) { timeDelta_ = delta; }
  int Latency()		{ return latency_; }
  void SetLatency(int latency) { latency_ = latency; }
#endif VU_SIMPLE_LATENCY
#ifdef VU_TRACK_LATENCY
  VU_BYTE TimeSyncState()	{ return timeSyncState_; }
  VU_TIME Latency()		{ return latency_; }
  void SetTimeSync(VU_BYTE newstate);
  void SetLatency(VU_TIME latency);
#endif VU_TRACK_LATENCY

  // Special VU type getters
  virtual VU_BOOL IsSession();	// returns TRUE

  virtual VU_BOOL HasTarget(VU_ID id);  // TRUE --> id contains (or is) ent
  virtual VU_BOOL InTarget(VU_ID id);   // TRUE --> ent contained by (or is) id

  VU_ERRCODE AddGroup(VU_ID gid);
  VU_ERRCODE RemoveGroup(VU_ID gid);
  VU_ERRCODE PurgeGroups();

#ifdef VU_USE_COMMS
  virtual VuTargetEntity *ForwardingTarget(VuMessage *msg = 0);
#endif

  // Rough dead reckoning interface
#if VU_MAX_SESSION_CAMERAS > 0
  int CameraCount() { return cameraCount_; }
  VuEntity *Camera(int index);
  int AttachCamera(VU_ID camera);
  int RemoveCamera(VU_ID camera);
  void ClearCameras(void);
#endif

  // event Handlers
#ifdef VU_LOW_WARNING_VERSION
  virtual VU_ERRCODE Handle(VuErrorMessage *error);
  virtual VU_ERRCODE Handle(VuPushRequest *msg);
  virtual VU_ERRCODE Handle(VuPullRequest *msg);
  virtual VU_ERRCODE Handle(VuEvent *event);
  virtual VU_ERRCODE Handle(VuFullUpdateEvent *event);
  virtual VU_ERRCODE Handle(VuPositionUpdateEvent *event);
  virtual VU_ERRCODE Handle(VuEntityCollisionEvent *event);
  virtual VU_ERRCODE Handle(VuTransferEvent *event);
  virtual VU_ERRCODE Handle(VuSessionEvent *event);
#else
  virtual VU_ERRCODE Handle(VuEvent *event);
  virtual VU_ERRCODE Handle(VuFullUpdateEvent *event);
  virtual VU_ERRCODE Handle(VuSessionEvent *event);
#endif VU_LOW_WARNING_VERSION

protected:
  VuSessionEntity(int typeindex, ulong domainMask, char *callsign);
  VU_SESSION_ID OpenSession();	// returns session id
  void CloseSession();
  virtual VU_ERRCODE InsertionCallback();
  virtual VU_ERRCODE RemovalCallback();

private:
  int LocalSize();                      // returns local bytes written

// DATA
protected:
  // shared data
  VU_SESSION_ID sessionId_;
  ulong domainMask_;
  char *callsign_;
  VU_BYTE loadMetric_;
  VU_ID gameId_;
  VU_BYTE groupCount_;
  VuGroupNode *groupHead_;
  int bandwidth_;//me123
#ifdef VU_SIMPLE_LATENCY
  int timeDelta_;
  int latency_;
#endif VU_SIMPLE_LATENCY;
#ifdef VU_TRACK_LATENCY
  VU_BYTE timeSyncState_;
  VU_TIME latency_;
  VU_TIME masterTime_;		// time from master
  VU_TIME masterTimePostTime_;	// local time of net msg post
  VU_TIME responseTime_;		// local time local msg post
  VU_SESSION_ID masterTimeOwner_;	// sender of master msg 
  // local data
  // time synchro statistics
  VU_TIME lagTotal_;
  int lagPackets_;
  int lagUpdate_;	// when packets > update, change latency value
#endif VU_TRACK_LATENCY
  // msg tracking
  int lastMsgRecvd_;
#if VU_MAX_SESSION_CAMERAS > 0
  VU_BYTE cameraCount_;
  VU_ID cameras_[VU_MAX_SESSION_CAMERAS];
#endif
  // local data
  VuGameEntity *game_;
  VU_GAME_ACTION action_;
};

//-----------------------------------------
class VuGroupEntity : public VuTargetEntity {

friend class VuSessionsIterator;
friend class VuSessionEntity;

public:
  // constructors & destructor
  VuGroupEntity(char *groupname);
  VuGroupEntity(VU_BYTE **stream);
  VuGroupEntity(FILE *file);
  virtual ~VuGroupEntity();

  // virtual function interface
  virtual int SaveSize();
  virtual int Save(VU_BYTE **stream);
  virtual int Save(FILE *file);

  char *GroupName()     { return groupName_; }
  ushort MaxSessions()  { return sessionMax_; }
  int SessionCount() { return sessionCollection_->Count(); }

  // setters
  void SetGroupName(char *groupname);
  void SetMaxSessions(ushort max) { sessionMax_ = max; }

  virtual VU_BOOL HasTarget(VU_ID id);
  virtual VU_BOOL InTarget(VU_ID id);
  VU_BOOL SessionInGroup(VuSessionEntity *session);
  virtual VU_ERRCODE AddSession(VuSessionEntity *session);
  VU_ERRCODE AddSession(VU_ID sessionId);
  virtual VU_ERRCODE RemoveSession(VuSessionEntity *session);
  VU_ERRCODE RemoveSession(VU_ID sessionId);

  virtual VU_BOOL IsGroup();	// returns TRUE

  // event Handlers
#ifdef VU_LOW_WARNING_VERSION
  virtual VU_ERRCODE Handle(VuErrorMessage *error);
  virtual VU_ERRCODE Handle(VuPushRequest *msg);
  virtual VU_ERRCODE Handle(VuPullRequest *msg);
  virtual VU_ERRCODE Handle(VuEvent *event);
  virtual VU_ERRCODE Handle(VuFullUpdateEvent *event);
  virtual VU_ERRCODE Handle(VuPositionUpdateEvent *event);
  virtual VU_ERRCODE Handle(VuEntityCollisionEvent *event);
  virtual VU_ERRCODE Handle(VuTransferEvent *event);
  virtual VU_ERRCODE Handle(VuSessionEvent *event);
#else
  virtual VU_ERRCODE Handle(VuSessionEvent *event);
  virtual VU_ERRCODE Handle(VuFullUpdateEvent *event);
#endif VU_LOW_WARNING_VERSION

protected:
  VuGroupEntity(int type, char *groupname, VuFilter *filter=0);
  virtual VU_ERRCODE Distribute(VuSessionEntity *ent);
  virtual VU_ERRCODE InsertionCallback();
  virtual VU_ERRCODE RemovalCallback();

private:
  int LocalSize();                      // returns local bytes written

// DATA
protected:
  char *groupName_;
  ushort sessionMax_;
  VuOrderedList *sessionCollection_;

  // scratch data
  int selfIndex_; 
};

//-----------------------------------------
class VuGameEntity : public VuGroupEntity {

friend class VuSessionsIterator;
friend class VuSessionEntity;

public:
  // constructors & destructor
  VuGameEntity(ulong domainMask, char *gamename);
  VuGameEntity(VU_BYTE **stream);
  VuGameEntity(FILE *file);
  virtual ~VuGameEntity();

  // virtual function interface
  virtual int SaveSize();
  virtual int Save(VU_BYTE **stream);
  virtual int Save(FILE *file);

  // accessors
  ulong DomainMask()		{ return domainMask_; }
  char *GameName()		{ return gameName_; }
  ushort MaxSessions() { return sessionMax_; }
  int SessionCount() { return sessionCollection_->Count(); }

  // setters
  void SetGameName(char *groupname);
  void SetMaxSessions(ushort max) { sessionMax_ = max; }

  virtual VU_ERRCODE AddSession(VuSessionEntity *session);
  virtual VU_ERRCODE RemoveSession(VuSessionEntity *session);

  virtual VU_BOOL IsGame();	// returns TRUE

  // event Handlers
#ifdef VU_LOW_WARNING_VERSION
  virtual VU_ERRCODE Handle(VuErrorMessage *error);
  virtual VU_ERRCODE Handle(VuPushRequest *msg);
  virtual VU_ERRCODE Handle(VuPullRequest *msg);
  virtual VU_ERRCODE Handle(VuEvent *event);
  virtual VU_ERRCODE Handle(VuFullUpdateEvent *event);
  virtual VU_ERRCODE Handle(VuPositionUpdateEvent *event);
  virtual VU_ERRCODE Handle(VuEntityCollisionEvent *event);
  virtual VU_ERRCODE Handle(VuTransferEvent *event);
  virtual VU_ERRCODE Handle(VuSessionEvent *event);
#else
  virtual VU_ERRCODE Handle(VuSessionEvent *event);
  virtual VU_ERRCODE Handle(VuFullUpdateEvent *event);
#endif VU_LOW_WARNING_VERSION

protected:
  VuGameEntity(int type, ulong domainMask, char *gamename, char *groupname);
  virtual VU_ERRCODE Distribute(VuSessionEntity *ent);
  virtual VU_ERRCODE RemovalCallback();

private:
  int LocalSize();                      // returns local bytes written

// DATA
protected:
  // shared data
  ulong domainMask_;
  char *gameName_;
};

//-----------------------------------------
class VuPlayerPoolGame : public VuGameEntity {
public:
  // constructors & destructor
  VuPlayerPoolGame(ulong domainMask);

  virtual ~VuPlayerPoolGame();

  // virtual function interface	-- stubbed out here
  virtual int SaveSize();
  virtual int Save(VU_BYTE **stream);
  virtual int Save(FILE *file);

  // do nothing...
  virtual VU_ERRCODE Distribute(VuSessionEntity *ent);

private:
  VuPlayerPoolGame(VU_BYTE **stream);
  VuPlayerPoolGame(FILE *file);

// DATA
protected:
  // none
};

//-----------------------------------------
class VuGlobalGroup : public VuGroupEntity {
public:
  // constructors & destructor
  VuGlobalGroup();

  virtual ~VuGlobalGroup();

  // virtual function interface	-- stubbed out here
  virtual int SaveSize();
  virtual int Save(VU_BYTE **stream);
  virtual int Save(FILE *file);

  virtual VU_BOOL HasTarget(VU_ID id);          // always returns TRUE
  VU_BOOL Connected() { return connected_; }
  void SetConnected(VU_BOOL conn) { connected_ = conn; }

private:
  VuGlobalGroup(VU_BYTE **stream);
  VuGlobalGroup(FILE *file);

// DATA
protected:
  VU_BOOL connected_;
};

//-----------------------------------------------------------------------------
class VuSessionFilter : public VuFilter {
public:
  VuSessionFilter(VU_ID groupId);
  virtual ~VuSessionFilter();

  virtual VU_BOOL Test(VuEntity *ent);
  virtual VU_BOOL RemoveTest(VuEntity *ent);
  virtual int Compare(VuEntity *ent1, VuEntity *ent2);
  virtual VU_BOOL Notice(VuMessage *event);
  virtual VuFilter *Copy();

protected:
  VuSessionFilter(VuSessionFilter *other);

// DATA
protected:
  VU_ID groupId_;
};

//-----------------------------------------------------------------------------
class VuSessionsIterator : public VuIterator {
public:
  VuSessionsIterator(VuGroupEntity *group=0);
  virtual ~VuSessionsIterator();

  VuSessionEntity *GetFirst()
    { if (collection_) curr_ = ((VuLinkedList *)collection_)->head_;
      return (VuSessionEntity *)curr_->entity_; }
  VuSessionEntity *GetNext()
    { curr_ = curr_->next_; return (VuSessionEntity *)curr_->entity_; }
  virtual VuEntity *CurrEnt();

  virtual VU_BOOL IsReferenced(VuEntity *ent);
  virtual VU_ERRCODE Cleanup();

protected:
  VuLinkNode *curr_;
};


#endif // _VUSESSION_H_
