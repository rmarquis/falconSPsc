// vu.h - Copyright (c) Mon July 15 15:01:01 1996,  Spectrum HoloByte, Inc.  All Rights Reserved

#ifndef _VU_H_
#define _VU_H_

#pragma warning(disable : 4514)

#include "vu2.h"
#include "IsBad.h"

//-------------------------------------------------------------------------
// The following three #defines are used to identify the Vu release number.
// VU_VERSION is defined as 2 for this architecture of Vu.
// VU_REVISION will be incremented each time a significant change is made
// 	(such as changing composition or organization of class data)
// VU_PATCH will be begin at 0 for each Revision, and be incremented by 1
// 	each time any change is made to Vu2
// VU_REVISION_DATE is a string which indicates the date of the latest revision
// VU_PATCH_DATE is a string which indicates the date of the latest patch
//-------------------------------------------------------------------------
#define VU_VERSION		2
#define VU_REVISION		23
#define VU_PATCH		2
#define VU_REVISION_DATE	"8/13/98"
#define VU_PATCH_DATE		"11/03/98"
//-------------------------------------------------------------------------
// The following structure and variable is primarily for use by debuggers
// to display the above version info.  The Version info is made static so
// the discrepancies between modules version usage can be tracked.
//-------------------------------------------------------------------------
#ifndef NDEBUG
static struct {
  int version_;
  int revision_;
  int patch_;
  char *revisionDate_;
  char *patchDate_;
} vuReleaseInfo = {
  VU_VERSION,
  VU_REVISION,
  VU_PATCH,
  VU_REVISION_DATE,
  VU_PATCH_DATE,
};
#endif
//-------------------------------------------------------------------------

#define VU_DEFAULT_QUEUE_SIZE		100

#include "apitypes.h"

struct VuEntityType;

class VuEntity;
class VuSessionEntity;
class VuGroupEntity;
class VuGameEntity;
class VuGlobalGroup;
class VuPlayerPoolGame;

class VuMessage;
class VuMessageFilter;
class VuMessageQueue;
class VuRedBlackTree;
class VuFilteredList;
class VuDriverSettings;

class VuMessageQueue;
class VuMainMessageQueue;
class VuPendingSendQueue;

// globals
//   VU provided
extern VuSessionEntity *vuLocalSessionEntity;
extern VuGlobalGroup *vuGlobalGroup;
extern VuPlayerPoolGame *vuPlayerPoolGroup;
extern VuFilteredList *vuGameList;
extern VuFilteredList *vuTargetList;

extern VuPendingSendQueue *vuNormalSendQueue;
extern VuPendingSendQueue *vuLowSendQueue;

extern VU_SESSION_ID vuKnownConnectionId;       // 0 --> not known (usual case)
extern VU_SESSION_ID vuNullSession;
extern VU_ID vuLocalSession;
extern VU_ID vuNullId;
extern VU_TIME vuTransmitTime;

//   app provided globals
extern int SGpfSwitchVal;
extern char *vuxWorldName;
extern ulong vuxLocalDomain;
extern VU_TIME vuxGameTime;
extern VU_TIME vuxRealTime;
extern VuDriverSettings *vuxDriverSettings;
#ifdef VU_AUTO_COLLISION
extern VuGridTree *vuxCollisionGrid;
#endif // VU_AUTO_COLLISION

// app supplied functions
extern VuEntityType *VuxType(ushort id);

// these invoke appropriate NEW func with VU_BYTE stream, & return pointer
//    		VuxCreateEntity creates an entity
extern VuEntity *VuxCreateEntity(ushort type, ushort size, VU_BYTE *data);
//    		VuxCreateMessage creates a VuMessage
extern VuMessage *VuxCreateMessage(VU_MSG_TYPE type, VU_ID senderid,
                                   VU_ID targetid);
extern void VuxRetireEntity(VuEntity *ent);
#ifdef VU_TRACK_LATENCY
extern void VuxAdjustLatency(VU_TIME newlatency, VU_TIME oldlatency);
#endif VU_TRACK_LATENCY
#ifdef VU_USE_COMMS
extern void VuxAddDanglingSession (VU_SESSION_ID id);
extern int VuxSessionConnect(VuSessionEntity *session);
extern void VuxSessionDisconnect(VuSessionEntity *session);
extern int VuxGroupConnect(VuGroupEntity *group);
extern void VuxGroupDisconnect(VuGroupEntity *group);
extern int VuxGroupAddSession(VuGroupEntity *group, VuSessionEntity *session);
extern int VuxGroupRemoveSession(VuGroupEntity *group,VuSessionEntity *session);
#endif

#ifdef VU_THREAD_SAFE
extern void VuEnterCriticalSection();
extern void VuExitCriticalSection();
extern bool VuHasCriticalSection();
#else // !VU_THREAD_SAFE
inline void VuEnterCriticalSection() {}
inline void VuExitCriticalSection() {}
inline bool VuHasCriticalSection() { return true; };
#endif // VU_THREAD_SAFE

// memory state defines
#define VU_MEM_CREATED			0x01
#define VU_MEM_ACTIVE			0x02
#define VU_MEM_SUSPENDED		0x03       // lives only in AntiDatabase
#define VU_MEM_PENDING_DELETE	0x04
#define VU_MEM_INACTIVE			0x05
#define VU_MEM_DELETED			0xdd

class VuBaseThread {
public:
  VuBaseThread();
  virtual ~VuBaseThread();

  VuMessageQueue *Queue() { return messageQueue_; }
  virtual VuMainMessageQueue *MainQueue() = 0;

  virtual void Update();

// data
protected:
  VuMessageQueue *messageQueue_;
};

class VuThread : public VuBaseThread {
public:
  VuThread(VuMessageFilter *filter, int queueSize = VU_DEFAULT_QUEUE_SIZE);
  VuThread(int queueSize = VU_DEFAULT_QUEUE_SIZE);
  virtual ~VuThread();

  virtual VuMainMessageQueue *MainQueue();

// data
protected:
  // none
};

class VuMainThread : public VuBaseThread {
public:
    // initializes database, etc.
  VuMainThread(int dbSize, VuMessageFilter *filter,
      int queueSize = VU_DEFAULT_QUEUE_SIZE,
      VuSessionEntity *(*sessionCtorFunc)(void) = 0);
  VuMainThread(int dbSize, int queueSize = VU_DEFAULT_QUEUE_SIZE,
      VuSessionEntity *(*sessionCtorFunc)(void) = 0);
  virtual ~VuMainThread();

  virtual VuMainMessageQueue *MainQueue();
  virtual void Update();

  VU_ERRCODE JoinGame(VuGameEntity *ent);
  VU_ERRCODE LeaveGame();
#ifdef VU_USE_COMMS
  VU_ERRCODE InitComms(ComAPIHandle handle, int bufSize=0, int packSize=0,
                       ComAPIHandle reliablehandle = NULL,
                       int relBufSize=0, int relPackSize=0,
                       int resendQueueSize = VU_DEFAULT_QUEUE_SIZE );
  VU_ERRCODE DeinitComms();

  static int FlushOutboundMessages();
  static void ReportXmit(int bytesSent);
  static void ResetXmit();
  static int BytesSent();
  static int BytesPending();
#endif

protected:
  void Init(int dbSize, VuSessionEntity *(*sessionCtorFunc)(void));
  void UpdateGroupData(VuGroupEntity *group);
  int GetMessages();
  int SendQueuedMessages ();

// data
protected:
#ifdef VU_USE_COMMS
  static int bytesSent_;
#endif VU_USE_COMMS
#ifdef VU_AUTO_UPDATE
  VuRedBlackTree * autoUpdateList_;
#endif VU_AUTO_UPDATE
};

#endif // _VU_H_
