// Copyright (c) 1998,  MicroProse, Inc.  All Rights Reserved

extern "C" {
#include <stdio.h>
#include <string.h>
#include <assert.h>
};
#include <windows.h> // JB 010318 Needed for CTD checks
#include "vu2.h"
#include "vu_priv.h"



#ifdef USE_SH_POOLS
MEM_POOL gVuMsgMemPool;
#endif


#define VU_ENCODE_8(B,X)  ((*(char *)(*B))++ = *(char *)(X))
#define VU_ENCODE_16(B,X) ((*(short*)(*B))++ = *(short*)(X))
#define VU_ENCODE_32(B,X) ((*(int  *)(*B))++ = *(int  *)(X))

#define VU_DECODE_8(X,B)  (*(char *)(X) = (*(char *)(*B))++)
#define VU_DECODE_16(X,B) (*(short*)(X) = (*(short*)(*B))++)
#define VU_DECODE_32(X,B) (*(int  *)(X) = (*(int  *)(*B))++)


#if defined(VU_USE_QUATERNION)
#include "ml.h"
#endif

#define VU_PACK_POSITION

#ifdef _DEBUG
int vumessagecount;
int vumessagepeakcount;
#endif

extern int g_nVUMaxDeltaTime; // 2002-04-12 MN

//#define DEBUG_AUTO_DISPATCH
//#define DEBUG_ON
//#define DEBUG_COMMS

//--------------------------------------------------
// the VuResendMsgFilter is used to hold messages for which send failed:
//   - IsLocal() is true
//   - VU_SEND_FAILED_MSG_FLAG is set
//   - either VU_RELIABLE_MSG_FLAG or VU_KEEPALIVE_MSG_FLAG are set
class VuResendMsgFilter : public VuMessageFilter {
public:
  VuResendMsgFilter();
  virtual ~VuResendMsgFilter();
  virtual VU_BOOL Test(VuMessage *event);
  virtual VuMessageFilter *Copy();

protected:
  // no data
};

//--------------------------------------------------
static VuEntity*
VuCreateEntity(ushort   type, 
               ushort, 
               VU_BYTE* data)
{
  VuEntity* retval = 0;

  switch (type) {
  case VU_SESSION_ENTITY_TYPE:
    retval = new VuSessionEntity(&data);
    break;
  case VU_GROUP_ENTITY_TYPE:
    retval = new VuGroupEntity(&data);
    break;
  case VU_GAME_ENTITY_TYPE:
    retval = new VuGameEntity(&data);
    break;
  case VU_GLOBAL_GROUP_ENTITY_TYPE:
  case VU_PLAYER_POOL_GROUP_ENTITY_TYPE:
    retval = 0;
    break;
  }
  return retval;
}

//--------------------------------------------------
static VuEntity*
ResolveWinner(VuEntity* ent1, 
              VuEntity* ent2)
{
  VuEntity* retval = 0;

  if (ent1->EntityType()->createPriority_ >
      ent2->EntityType()->createPriority_) {
    retval = ent1;
  } else if (ent1->EntityType()->createPriority_ <
             ent2->EntityType()->createPriority_) {
    retval = ent2;
  } else if (ent1->OwnerId().creator_ == ent1->Id().creator_) {
    retval = ent1;
  } else if (ent2->OwnerId().creator_ == ent2->Id().creator_) {
    retval = ent2;
  } else if (ent1->OwnerId().creator_ < ent2->OwnerId().creator_) {
    retval = ent1;
  } else {
    retval = ent2;
  }

  return retval;
}

//--------------------------------------------------
VU_BOOL
VuNullMessageFilter::Test(VuMessage*)
{
  return TRUE;
}

VuMessageFilter*
VuNullMessageFilter::Copy()
{
  return new VuNullMessageFilter;
}

static VuNullMessageFilter vuNullFilter;

//--------------------------------------------------
VuMessageTypeFilter::VuMessageTypeFilter(ulong bitfield) : msgTypeBitfield_(bitfield)
{
  // emtpy
}

VuMessageTypeFilter::~VuMessageTypeFilter()
{
  // emtpy
}

VU_BOOL 
VuMessageTypeFilter::Test(VuMessage* event)
{
  return static_cast<VU_BOOL>((1<<event->Type() & msgTypeBitfield_) ? TRUE : FALSE);
}

VuMessageFilter*
VuMessageTypeFilter::Copy()
{
  return new VuMessageTypeFilter(msgTypeBitfield_);
}

//--------------------------------------------------
VuStandardMsgFilter::VuStandardMsgFilter(ulong bitfield) : msgTypeBitfield_(bitfield)
{
  // emtpy
}

VuStandardMsgFilter::~VuStandardMsgFilter()
{
  // emtpy
}

VU_BOOL 
VuStandardMsgFilter::Test(VuMessage* message)
{
  ulong eventBit = 1<<message->Type();
  if ((eventBit & msgTypeBitfield_) == 0) {
    return FALSE;
  }
  if (eventBit & (VU_DELETE_EVENT_BITS | VU_CREATE_EVENT_BITS )) {
    return TRUE;
  }
  if (message->Sender() == vuLocalSession) {
    return FALSE;
  }
  // test to see if entity was found in database
  if (message->Entity() != 0) {
    return TRUE;
  }
  return FALSE;
}

VuMessageFilter*
VuStandardMsgFilter::Copy()
{
  return new VuStandardMsgFilter(msgTypeBitfield_);
}

//--------------------------------------------------
VuResendMsgFilter::VuResendMsgFilter()
{
  // emtpy
}

VuResendMsgFilter::~VuResendMsgFilter()
{
  // emtpy
}

VU_BOOL 
VuResendMsgFilter::Test(VuMessage* message)
{
#if 0
  if ((message->Flags() & VU_SEND_FAILED_MSG_FLAG) &&
      ((message->Flags() & VU_RELIABLE_MSG_FLAG) ||
       (message->Flags() & VU_KEEPALIVE_MSG_FLAG)) ) {
    return TRUE;
  }
  return FALSE;
#else
  if ((message->Flags() & VU_SEND_FAILED_MSG_FLAG) &&
      (message->Flags() & (VU_RELIABLE_MSG_FLAG|VU_KEEPALIVE_MSG_FLAG)))
    return TRUE;
  else
    return FALSE;
#endif
}

VuMessageFilter*
VuResendMsgFilter::Copy()
{
  return new VuResendMsgFilter;
}

//--------------------------------------------------

VuMessageQueue* VuMessageQueue::queuecollhead_ = 0;

VuMessageQueue::VuMessageQueue(int              queueSize, 
                               VuMessageFilter* filter)
{
  head_  = new VuMessage*[queueSize];
  read_  = head_;
  write_ = head_;
  tail_  = head_ + queueSize;
#ifdef _DEBUG
    _count = 0;
#endif

  // initialize queue
  for (int i = 0; i < queueSize; i++) {
    head_[i] = 0;
  }
  if (!filter) {
    filter = &vuNullFilter;
  }
  filter_ = filter->Copy();

  // add this queue to list of queues
  VuEnterCriticalSection();
  nextqueue_     = queuecollhead_;
  queuecollhead_ = this;
  VuExitCriticalSection();
}

VuMessageQueue::~VuMessageQueue()
{
  delete [] head_;
  delete filter_;
  filter_ = 0;

  // delete this queue from list of queues
  VuEnterCriticalSection();
  VuMessageQueue* last = 0;
  VuMessageQueue* cur = queuecollhead_;

  while (cur) {
    if (this == cur) {
      if (last) {
	last->nextqueue_ = this->nextqueue_;
      } 
      else {
        queuecollhead_ = this->nextqueue_;
      }
      // we've removed it... break out of while() loop
      break;
    }
    last = cur;
    cur  = cur->nextqueue_;
  }
  VuExitCriticalSection();
}

// KCK: Sorry Dan, this is for testing if the critical
// section is held- Good place for it, but we need to
// know about the app's implimentation of critical sections
#if defined(FALCON4)
class F4CSECTIONHANDLE;
extern int CritialSectionCount(F4CSECTIONHANDLE* cx);
extern F4CSECTIONHANDLE* vuCritical;;
#endif

VuMessage*
VuMessageQueue::DispatchVuMessage(VU_BOOL autod)
{
  VuMessage* retval = 0;
  VuEnterCriticalSection();

  if (*read_) {
#if defined(DEBUG_ON)
    VU_PRINT("VU: Dispatching message: ");
#endif
    retval = *read_;
    *read_++ = 0;
#ifdef _DEBUG
    _count --;
#endif
    if (read_ == tail_) {
      read_ = head_;
    }
    retval->Ref();
#if defined(FALCON4)
	if (CritialSectionCount(vuCritical) != 1 && !autod) {
      VU_PRINT("VU: Dispatching message while holding a critical section.\n");
	}
#endif
    VuExitCriticalSection();
    retval->Dispatch(autod);
    retval->UnRef();
    retval->UnRef();
#if defined(DEBUG_ON)
    VU_PRINT("\n");
#endif
    return retval;
  }
  VuExitCriticalSection();
  return retval;
}

int 
VuMessageQueue::DispatchAllMessages(VU_BOOL autod)
{
#ifdef _DEBUG
    static int interestlevel = 20;
 //   if (_count > interestlevel)
//	MonoPrint ("Dispatching %d messages\n", _count);//me123
#endif
  int i = 0;
  while (DispatchVuMessage(autod)) {
    i++;
  }
#ifdef _DEBUG
  vumessagecount = i;
  if (vumessagecount > vumessagepeakcount)
	  vumessagepeakcount = vumessagecount;
#endif
  return i;
}

VU_BOOL
VuMessageQueue::ReallocQueue()
{
#if defined(REALLOC_QUEUES)
  int size = (tail_ - head_)*2;
  VuMessage	**newhead, **cp, **rp;
  
  newhead = new VuMessage*[size];
  cp      = newhead;

  for (rp = read_; rp != tail_; cp++,rp++)
    *cp = *rp;

  for (rp = head_; rp != write_; cp++,rp++)
    *cp = *rp;
  
  delete[] head_;
  head_  = newhead;
  read_  = head_;
  write_ = cp;
  tail_  = head_ + size;

  while (cp != tail_)
	  *cp++ = 0;

#if defined(DEBUG)
  FILE* fp = fopen ("vuerror.log","a");
  if (fp) {
    fprintf(fp,"Reallocating queue from size %d to size %d.\n",size/2,size);
    fclose(fp);
  }
#endif

  return TRUE;

#else
  return FALSE;
#endif
}

int 
VuMessageQueue::AddMessage(VuMessage* event)
{
	// JB 010121
	if (!event ||
		!filter_) // JB 010715
		return 0;

  int retval = 0;
  if (filter_->Test(event) && event->Type() != VU_TIMER_EVENT) {
    retval = 1;
    event->Ref();
    *write_++ = event;
    if (write_ == tail_) {
      write_ = head_;
    }
#ifdef _DEBUG
    _count ++;
#endif

#if defined(DEBUG_ON)
    VU_PRINT("VU: Added message\n");
#endif

    if (write_ == read_ && *read_) {
      if (!ReallocQueue() && write_ == read_ && *read_) {
        // do simple dispatch -- cannot be handled by user
        // danm_note: should we issue a warning here?
        DispatchVuMessage(TRUE);

#if defined(DEBUG_AUTO_DISPATCH)
        VU_PRINT("VU: Auto message dispatch: sender %d; msg id %u\n",
                 event->MsgId().num_, event->MsgId().id_);
#endif
      }
    }
  }
  return retval;
}

void
VuMessageQueue::RepostMessage(VuMessage* msg, 
                              int        delay)
{
#if defined(DEBUG_COMMS)
  VU_PRINT("Send connection still pending... reposting at time %d\n", vuxRealTime+(delay * VU_TICS_PER_SECOND));
#endif
  msg->flags_ |= ~VU_LOOPBACK_MSG_FLAG;
  VuTimerEvent *timer = new VuTimerEvent(0, vuxRealTime + delay, VU_DELAY_TIMER, msg);
  VuMessageQueue::PostVuMessage(timer);
}
//me123 do some bandwidth checking here
static int bandwidth_time =0;
static int user_time =0;
static float bandwidth_used =0;
static int bandwidth_availeble =0;
static int users =0;
extern int MinClientBandwidth;
extern bool bwmode;
extern float clientbwforupdatesmodifyer;
extern float g_fclientbwforupdatesmodifyerMAX;
extern float g_fclientbwforupdatesmodifyerMIN;
extern float g_fReliablemsgwaitMAX;
int count =0;
//int timercount =0;

int 
VuMessageQueue::PostVuMessage(VuMessage* msg)
{
  int retval = 0;
/////////////////////////////////////////////////////////
	int
		delta_time,
		time;
	time = GetTickCount ();
	float bandwidthmodifyer = (1.0f - clientbwforupdatesmodifyer);
	if (user_time == 0 || time - user_time> 10000)	// every 10 seconds count the number of players
	{	
		user_time = time;
		users = 0;

		VuSessionsIterator iter(vuLocalSessionEntity->Game());
		VuSessionEntity*   sess;
		
		sess = iter.GetFirst();

		while (sess)
		{
			users ++;

			sess = iter.GetNext ();
		}

		users --; // don't count ourselves
		users = max (1, users);
	}
	if ( bandwidth_time != 0)
	{
		/* Decay the bandwidth used wrt. time */

		delta_time = time -  bandwidth_time;
		delta_time = min(delta_time, g_nVUMaxDeltaTime);

		if (delta_time > 100)
		{
			if (vuLocalSessionEntity->Game() && vuLocalSessionEntity->Game()->IsLocal())
			 bandwidth_used -= delta_time * MinClientBandwidth *  bandwidthmodifyer * (float)users / 1000.0f;	// me123 //  
			else  bandwidth_used -= delta_time * MinClientBandwidth *  bandwidthmodifyer / 1000.0f;	// me123 //  
			if ( bandwidth_used < 0)
			{
				 bandwidth_used = 0;
			}
			 bandwidth_time = time;
		}
		
	}
	else
	{
		 bandwidth_time = time;
		 bandwidth_used = 0;
	}
	int totalbw;
	bool factormessage = (
		msg->Target() && 
		msg->Target() != vuLocalSessionEntity &&
		msg->IsLocal() && 
		msg->Type() != VU_TIMER_EVENT);

	if (vuLocalSessionEntity->Game() && vuLocalSessionEntity->Game()->IsLocal())
		totalbw = (int)((float)MinClientBandwidth  *  (float)users *  bandwidthmodifyer) ;//me123 
	else totalbw = (int)((float)MinClientBandwidth  *  bandwidthmodifyer) ;//me123 

if (!bwmode) bandwidth_used =0;
	bandwidth_availeble = totalbw - (int)bandwidth_used;
			int delay =0;

// control the bw used for posupdates and otherdata.
	if (bandwidth_availeble < 0)
	{
	clientbwforupdatesmodifyer *= 0.99f;
	clientbwforupdatesmodifyer = max (g_fclientbwforupdatesmodifyerMIN,clientbwforupdatesmodifyer);
	}
	else if (bandwidth_availeble > totalbw/2)
	{
	clientbwforupdatesmodifyer *= 1.01f;
	clientbwforupdatesmodifyer = min (g_fclientbwforupdatesmodifyerMAX,clientbwforupdatesmodifyer);
	}


    if ( factormessage && bandwidth_availeble < min(totalbw,msg->Size())) 
	{

		if (msg->Flags() & VU_RELIABLE_MSG_FLAG)
		{	// resend it when we have bandwidth again
			float bwPrMsec;
			if (vuLocalSessionEntity->Game() && vuLocalSessionEntity->Game()->IsLocal())
			bwPrMsec  =  (float)MinClientBandwidth * bandwidthmodifyer *(float)users / 1000.0f;	// me123 //  
			else  bwPrMsec =  (float)MinClientBandwidth * bandwidthmodifyer / 1000.0f;	// me123 // 

			if (bandwidth_availeble < 0)delay = (int)(-bandwidth_availeble / bwPrMsec);
			delay = min(delay, (int)g_fReliablemsgwaitMAX);
			if (msg->Flags() & VU_OUT_OF_BAND_MSG_FLAG) delay =0;
			if (delay > 500)
			{// we are short on bw repost
				VuTimerEvent *timer = new VuTimerEvent(0, vuxRealTime + delay, VU_DELAY_TIMER, msg);
				VuMessageQueue::PostVuMessage(timer);
				bandwidth_used += msg->Size();
//				timercount++;
//MonoPrint("%d TIMER TIMER !!!  Type\n",msg->Type());//me123		
				return 0;
			}
			else 
			{			
				// go through
//MonoPrint("%d GO THROUGH THROUGH !!! \n",msg->Type());//me123		
			}
//			MonoPrint("%d ABORT ABORT ABORT !!! bwused %d, bwavail%d, type %d, size %d \n",count,bandwidth_used,bandwidth_availeble, msg->Type(), msg->Size ());//me123		
		}
		else if (msg->Type() != VU_POSITION_UPDATE_EVENT)
		{
//MonoPrint("%d TRASH TRASH !!! \n",msg->Type());//me123
			return 0;
		}
	}
	 if (factormessage && msg->IsLocal() && msg->Type() != VU_POSITION_UPDATE_EVENT) bandwidth_used += msg->Size();
//	MonoPrint("bwused %d, bwavail%d, type %d, size %d \n",bandwidth_used,bandwidth_availeble, msg->Type(), msg->Size ());//me123
/////////////////////////////////////

  if (msg) {
    retval = 1;
    // must enter critical section as this modifies multiple threads' queues

    VuEnterCriticalSection();
    msg->Ref(); // Ref/UnRef pair will delete unhandled messages

    if (msg->postTime_ == 0)
      msg->postTime_ = vuxRealTime;

    VuEntity* ent = msg->Entity();

    if (ent == 0)
      ent = vuDatabase->Find(msg->EntityId());

    msg->Activate(ent);

#if defined(VU_USE_COMMS)
    if (ent && !msg->IsLocal() && !ent->IsLocal()) {
      ent->SetTransmissionTime(msg->postTime_);
    }
    if (vuGlobalGroup && vuGlobalGroup->Connected() &&
        msg->Target() && msg->Target() != vuLocalSessionEntity &&
        msg->DoSend() && (!ent || !ent->IsPrivate()) &&
        vuLocalSession.creator_ != VU_SESSION_NULL_CONNECTION.creator_) {
      retval = msg->Send();
      if (retval == 0 && vuNormalSendQueue &&
          (msg->Flags() & VU_SEND_FAILED_MSG_FLAG) &&
          ((msg->Flags() & VU_RELIABLE_MSG_FLAG) ||
           (msg->Flags() & VU_KEEPALIVE_MSG_FLAG)) ) {
	if (msg->Flags() & VU_NORMAL_PRIORITY_MSG_FLAG)
	{
          vuNormalSendQueue->AddMessage(msg);
		 // MonoPrint("vuNormalSendQueue->AddMessage: %d,%d\n", msg->Type(), msg->Size ());//me123
#ifdef _DEBUG
		MonoPrint("vuNormalSendQueue->AddMessage: %d,%d\n", msg->Type(), msg->Size ());//me123
#endif
	}

	else
	{
	  vuLowSendQueue->AddMessage(msg);
        retval = msg->Size();
//	MonoPrint("vuLowSendQueue->AddMessage: %d,%d\n", msg->Type(), msg->Size ());//me123
#ifdef _DEBUG
		MonoPrint("vuLowSendQueue->AddMessage: %d,%d\n", msg->Type(), msg->Size ());//me123
#endif
	}
      }
    }
#endif
    if (!msg->IsLocal() ||
        ((msg->Flags() & VU_LOOPBACK_MSG_FLAG) && msg->IsLocal())) {
      VuMessageQueue* cur = queuecollhead_;
      while (cur) {

#ifdef _DEBUG
//		MonoPrint("cur->AddMessage: %d,%d\n", msg->Type(), msg->Size ());//me123
#endif
        cur->AddMessage(msg);
        cur = cur->nextqueue_;
      }
    }
    if (msg->refcnt_ == 1 &&
        (!msg->IsLocal() || (msg->Flags() & VU_LOOPBACK_MSG_FLAG))) {
      // indicated message was never added to any queue
      VuExitCriticalSection();
      msg->Process(TRUE);
      vuDatabase->Handle(msg);
      msg->UnRef();
      return retval;
    }
    msg->UnRef();
    VuExitCriticalSection();
  }

  return retval;
}

void
VuMessageQueue::FlushAllQueues()
{
  // must enter critical section as this modifies multiple threads' queues
  VuMessageQueue* cur = queuecollhead_;
  while (cur) {
    VuEnterCriticalSection();
    VuMessageQueue* next = cur->nextqueue_;
    VuExitCriticalSection();
    cur->DispatchAllMessages(TRUE);
    cur = next;
  }
}

int
VuMessageQueue::InvalidateQueueMessages(VU_BOOL (*evalFunc)(VuMessage*, void*), void *arg)
{
  VuEnterCriticalSection();
  int         count = 0;
  VuMessage** cur = read_;

  while (*cur) {
    if ((*evalFunc)(*cur, arg) == TRUE) {
#if defined(DEBUG_ON)
      VU_PRINT("VU: Removing message\n");
#endif
      (*cur)->UnRef();
      *cur = new VuUnknownMessage();
      (*cur)->Ref();
      count++;
    }
    *cur++;
    if (cur == tail_) {
      cur = head_;
    }
  }

  VuExitCriticalSection();
  return count;
}

int
VuMessageQueue::InvalidateMessages(VU_BOOL (*evalFunc)(VuMessage*, void*), void *arg)
{
  int count = 0;
  VuEnterCriticalSection();
  VuMessageQueue *cur = queuecollhead_;
  while (cur) {
    count += cur->InvalidateQueueMessages(evalFunc, arg);
    cur = cur->nextqueue_;
  }
  VuExitCriticalSection();
  return count;
}

//--------------------------------------------------
VuMainMessageQueue::VuMainMessageQueue(int              queueSize, 
                                       VuMessageFilter* filter)
  : VuMessageQueue(queueSize, filter)
{
  timerlisthead_ = 0;
}

VuMainMessageQueue::~VuMainMessageQueue()
{
  // empty
}

int 
VuMainMessageQueue::AddMessage(VuMessage* msg)
{
  int retval = 0;

  if (msg->Type() == VU_TIMER_EVENT) {
    msg->Ref();
    VuTimerEvent* insert = (VuTimerEvent*)msg;
    VuTimerEvent* last = 0;
    VuTimerEvent* cur = timerlisthead_;
    while (cur && cur->mark_ <= insert->mark_) {
      last = cur;
      cur  = cur->next_;
    }
    insert->next_ = cur;
#ifdef _DEBUG
    _count ++;
#endif

    if (last)
      last->next_    = insert;
    else
      timerlisthead_ = insert;

    retval = 1;
  } 
  else
    retval = VuMessageQueue::AddMessage(msg);

  return retval;
}
//int dispatchcount =0;
VuMessage* 
VuMainMessageQueue::DispatchVuMessage(VU_BOOL autod)
{
  VuMessage* retval = 0;

  VuEnterCriticalSection();

  while (timerlisthead_ && timerlisthead_->mark_ < vuxRealTime) {
    VuTimerEvent* oldhead = timerlisthead_;
    timerlisthead_ = timerlisthead_->next_;
//	dispatchcount++;
//	MonoPrint("DISPATCH timer count %d \n",dispatchcount);//me123
    oldhead->Dispatch(autod);
    oldhead->UnRef();
  }
  VuExitCriticalSection();
  retval = VuMessageQueue::DispatchVuMessage(autod);
  return retval;
}

static VuResendMsgFilter resendMsgFilter;
//--------------------------------------------------
VuPendingSendQueue::VuPendingSendQueue(int queueSize)
  : VuMessageQueue(queueSize, &resendMsgFilter)
{
  bytesPending_ = 0;
}

VuPendingSendQueue::~VuPendingSendQueue()
{
  DispatchAllMessages(TRUE);
}

int
VuPendingSendQueue::AddMessage(VuMessage* msg)
{
  int retval = VuMessageQueue::AddMessage(msg);
  if (retval > 0) {
    bytesPending_ += msg->Size();
  }
  return retval;
}

VuMessage *
VuPendingSendQueue::DispatchVuMessage(VU_BOOL autod)
{
  VuMessage* msg    = 0;
  VuMessage* retval = 0;

  VuEnterCriticalSection();

  if (*read_) {
#if defined(DEBUG_ON)
    VU_PRINT("VU: Resending message: ");
#endif
    msg = *read_;
    *read_++ = 0;
#ifdef _DEBUG
    _count --;
#endif
    if (read_ == tail_) {
      read_ = head_;
    }
    msg->Ref();
    bytesPending_ -= msg->Size();
    retval = msg;
    if (msg->DoSend()) {
      if (msg->Send() == 0) {
#if defined(DEBUG_ON)
        VU_PRINT("-- failed ");
#endif
        retval = 0;
        if (!autod) {
#if defined(DEBUG_ON)
          VU_PRINT("-- reposting ");
#endif
          // note: this puts the unsent message on the end of the send queue
#ifdef DEBUG
		  MonoPrint("autod -- reposting: %d,%d\n", msg->Type(), msg->Size ());//me123
#endif
          AddMessage(msg);
        }
      }
#if defined(DEBUG_ON)
    } else {
      VU_PRINT("-- aborted ");
#endif
    }
    msg->UnRef(); // list reference
    msg->UnRef(); // local reference
#if defined(DEBUG_ON)
    VU_PRINT("\n");
#endif
  }

  VuExitCriticalSection();
  return retval;
}

VU_BOOL
TargetInvalidateCheck(VuMessage* msg, 
                      void*      arg)
{
  if (msg->Target() == arg) {
    return TRUE;
  }
  return FALSE;
}

void
VuPendingSendQueue::RemoveTarget(VuTargetEntity* target)
{
  InvalidateQueueMessages(TargetInvalidateCheck, target);
}

//--------------------------------------------------
//--------------------------------------------------

VuMessage::VuMessage(VU_MSG_TYPE     type, 
                     VU_ID           entityId, 
                     VuTargetEntity* target, 
                     VU_BOOL         loopback)
  : refcnt_(0),
    type_(type),
    flags_(VU_NORMAL_PRIORITY_MSG_FLAG),
    entityId_(entityId),
    target_(target),
    postTime_(0),
    ent_(0)
{
  if (target == vuLocalSessionEntity) {
    loopback = TRUE;
  }
  if (target) {
    target_ = target;
  } else if (vuGlobalGroup) {
    target_ = vuGlobalGroup;
  } else {
    target_ = vuLocalSessionEntity;
  } 
  if (target_) {
    tgtid_ = target_->Id();
  }
  if (loopback) {
    flags_ |= VU_LOOPBACK_MSG_FLAG;
  }
    // note: msg id is set only for external messages which are sent out
  sender_.num_     = vuLocalSession.num_;
  sender_.creator_ = vuLocalSession.creator_;
}

VuMessage::VuMessage(VU_MSG_TYPE type, 
                     VU_ID       senderid,
                     VU_ID       target)
  : refcnt_(0),
    type_(type),
    flags_(VU_REMOTE_MSG_FLAG),
    sender_(senderid),
    tgtid_(target),
    entityId_(0,0),
    target_(0),
    ent_(0),
    postTime_(0)
{
}

VuMessage::~VuMessage()
{
#ifdef DEBUG
	// Check to make sure this is not in any queues
	VuMessageQueue	*cur;
	VuMessage		*retval = 0;
	VuMessage		*curm;

	VuEnterCriticalSection();

	cur = vuNormalSendQueue;
	curm = *(cur->read);
	while (curm)
		{
		assert (curm != this);
		curm++;
		if (curm == *(cur->tail_))
			curm = *(cur->head_);
		}
	cur = vuLowSendQueue;
	curm = *(cur->read);
	while (curm)
		{
		assert (curm != this);
		curm++;
		if (curm == *(cur->tail_))
			curm = *(cur->head_);
		}
	cur = queuecollhead_;
	while (cur) 
		{
		cur->AddMessage(msg);
		curm = *(cur->read);
		while (curm)
			{
			assert (curm != this);
			curm++;
			if (curm == *(cur->tail_))
				curm = *(cur->head_);
			}
		cur = cur->nextqueue_;
		}

	VuExitCriticalSection();
#endif
  SetEntity(0);
}

VU_BOOL 
VuMessage::DoSend()
{
  return TRUE;
}

VuEntity *
VuMessage::SetEntity(VuEntity* ent)
{
    // basically try and catch the bad case (ref/deref now swapped)
    // its ok if 0, cos nothing bad will happen
    // its ok if they are not the same entity as we don't care then
    // its ok if the refcount is more than 1, cos the deref wouldn't destroy it anyway
    assert(ent == 0 || ent != ent_ || ent->RefCount() > 1); // JPO test

  if (ent)
    VuReferenceEntity(ent);

  if (ent_)
    VuDeReferenceEntity(ent_);

  ent_ = ent;
  return ent_;
}

int 
VuMessage::Ref()
{
  return ++refcnt_;
}

int 
VuMessage::UnRef()
{
  // NOTE: must assign temp here as memory may be freed prior to return
  int retval = --refcnt_;
  if (refcnt_ <= 0)
    delete this;

  return retval;
}

int 
VuMessage::Read(VU_BYTE** buf, 
                int       length)
{
 	if (F4IsBadReadPtr(this, sizeof(VuMessage))) // JB 010318 CTD
		return 0; // JB 010318 CTD

 int retval = Decode(buf, length);
  assert (retval == length);

  refcnt_ = 0;
  SetEntity(0);
  return length;
}

int 
VuMessage::Write(VU_BYTE** buf)
{
  return Encode(buf);
}

int
VuMessage::Send()
{
  int retval = -1;
  if (Target() && Target() != vuLocalSessionEntity) {

#if defined(VU_USE_COMMS)
    retval = Target()->SendMessage(this);
#endif

    if (retval <= 0)
      flags_ |= VU_SEND_FAILED_MSG_FLAG;
  }
  return retval;
}

VU_ERRCODE 
VuMessage::Dispatch(VU_BOOL autod)
{
	 	if (F4IsBadReadPtr(this, sizeof(VuMessage))) // JB 010318 CTD
		return VU_ERROR; // JB 010318 CTD

  int retval = VU_NO_OP;

  if (!IsLocal() || (flags_ & VU_LOOPBACK_MSG_FLAG)) {
    assert(FALSE == F4IsBadReadPtr(vuDatabase, sizeof *vuDatabase));
    if (!Entity()) {
      // try to find ent again -- may have been in queue
      VuEntity *ent = vuDatabase->Find(entityId_);
      if (ent) {
        Activate(ent);
      }
    }
    retval = Process(autod);

		if (F4IsBadCodePtr((FARPROC) vuDatabase)) // JB 010404 CTD
			return VU_ERROR;

    vuDatabase->Handle(this);
    // mark as sent
    flags_ |= VU_PROCESSED_MSG_FLAG;
  }

  return retval;
}

VU_ERRCODE 
VuMessage::Activate(VuEntity* ent)
{
  SetEntity(ent);
  return VU_SUCCESS;
}


#define VUMESSAGE_LOCALSIZE (sizeof(entityId_.creator_)+sizeof(entityId_.num_))
int 
VuMessage::LocalSize()
{
  return (VUMESSAGE_LOCALSIZE);
}


#define VUMESSAGE_SIZE (VUMESSAGE_LOCALSIZE)
int 
VuMessage::Size()
{
  return (VUMESSAGE_SIZE);
}

int 
VuMessage::Decode(VU_BYTE** buf, int)
{
  memcpy(&entityId_.creator_, *buf, sizeof(entityId_.creator_)); *buf+=sizeof(entityId_.creator_);
  memcpy(&entityId_.num_,     *buf, sizeof(entityId_.num_));     *buf+=sizeof(entityId_.num_);

  return (VUMESSAGE_LOCALSIZE);
}

int 
VuMessage::Encode(VU_BYTE** buf)
{
  memcpy(*buf, &entityId_.creator_, sizeof(entityId_.creator_)); *buf += sizeof(entityId_.creator_);
  memcpy(*buf, &entityId_.num_,     sizeof(entityId_.num_));     *buf += sizeof(entityId_.num_);

  return (VUMESSAGE_LOCALSIZE);
}

//--------------------------------------------------
VuErrorMessage::VuErrorMessage(int             errorType, 
                               VU_ID           srcmsgid,
                               VU_ID           entityId,
                               VuTargetEntity* target)
  : VuMessage(VU_ERROR_MESSAGE, entityId, target, FALSE),
    srcmsgid_(srcmsgid),
    etype_(static_cast<short>(errorType))
{
  // empty
}

VuErrorMessage::VuErrorMessage(VU_ID     senderid,
                               VU_ID     target)
  : VuMessage(VU_ERROR_MESSAGE, senderid, target),
    etype_(VU_UNKNOWN_ERROR)
{
  srcmsgid_.num_     = 0;
  srcmsgid_.creator_ = 0;
}

VuErrorMessage::~VuErrorMessage()
{
  // empty
}

#define VUERRORMESSAGE_LOCALSIZE (sizeof(srcmsgid_)+sizeof(etype_))
int 
VuErrorMessage::LocalSize()
{
  return (VUERRORMESSAGE_LOCALSIZE);
}

#define VUERRORMESSAGE_SIZE (VUMESSAGE_SIZE+VUERRORMESSAGE_LOCALSIZE)
int 
VuErrorMessage::Size()
{
  return (VUERRORMESSAGE_SIZE);
}

int 
VuErrorMessage::Decode(VU_BYTE** buf, 
                       int       length)
{
  VuMessage::Decode(buf, length);

  memcpy(&srcmsgid_, *buf, sizeof(srcmsgid_)); *buf += sizeof(srcmsgid_);
  memcpy(&etype_,    *buf, sizeof(etype_));    *buf += sizeof(etype_);

  return (VUERRORMESSAGE_SIZE);
}

int 
VuErrorMessage::Encode(VU_BYTE** buf)
{
  VuMessage::Encode(buf);

  memcpy(*buf, &srcmsgid_, sizeof(srcmsgid_)); *buf += sizeof(srcmsgid_);
  memcpy(*buf, &etype_,    sizeof(etype_));    *buf += sizeof(etype_);

  return (VUERRORMESSAGE_SIZE);
}

VU_ERRCODE 
VuErrorMessage::Process(VU_BOOL)
{
  if (Entity()) {
    Entity()->Handle(this);
    return VU_SUCCESS;
  }
  return VU_NO_OP;
}

//--------------------------------------------------
VuRequestMessage::VuRequestMessage(VU_MSG_TYPE     type, 
                                   VU_ID           entityId,
                                   VuTargetEntity* target)
  : VuMessage(type, entityId, target, FALSE)
{
  // empty
}

VuRequestMessage::VuRequestMessage(VU_MSG_TYPE type, 
                                   VU_ID       senderid,
                                   VU_ID       dest)
  : VuMessage(type, senderid, dest)
{
  // empty
}

VuRequestMessage::~VuRequestMessage()
{
  // empty
}

//--------------------------------------------------
VuGetRequest::VuGetRequest(VU_SPECIAL_GET_TYPE sgt,
                           VuSessionEntity*    sess)
  : VuRequestMessage(VU_GET_REQUEST_MESSAGE, vuNullId,
                     (sess ? sess
                      : ((sgt==VU_GET_GLOBAL_ENTS) ? (VuTargetEntity*)vuGlobalGroup
                         : (VuTargetEntity*)vuLocalSessionEntity->Game())))
{
}

VuGetRequest::VuGetRequest(VU_ID           entityId, 
                           VuTargetEntity* target)
  : VuRequestMessage(VU_GET_REQUEST_MESSAGE, entityId, target)
{
  // empty
}

VuGetRequest::VuGetRequest(VU_ID     senderid, 
                           VU_ID     target)
  : VuRequestMessage(VU_GET_REQUEST_MESSAGE, senderid, target)
{
  // empty
}

VuGetRequest::~VuGetRequest()
{
  // empty
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE VuGetRequest::Process(VU_BOOL autod)
{
	VuTargetEntity* sender = (VuTargetEntity *)vuDatabase->Find(Sender());

	if (!IsLocal())
	{
//		MonoPrint ("Get Request %08x\n", entityId_);

		if (sender && sender->IsTarget())
		{
			VuMessage *resp = 0;
			if (autod) 
			{
				resp = new VuErrorMessage(VU_NOT_AVAILABLE_ERROR, Sender(), EntityId(), sender);
			} 
			else if (entityId_ == vuNullId)
			{
				// get ALL ents
				if (tgtid_ == vuGlobalGroup->Id() || tgtid_ == vuLocalSession)
				{
					// get all _global_ ents
					VuDatabaseIterator iter;
					VuEntity* ent = iter.GetFirst();

					while (ent)
					{
						if (!ent->IsPrivate() && ent->IsGlobal())
						{// ent->IsLocal() && 
							if (ent->Id() != sender->Id())
							{
								if (ent->IsLocal ())
								{
//									MonoPrint("Get Request: Sending Full Update on %08x to %08x\n", ent->Id().creator_.value_, sender->Id().creator_.value_);
									resp = new VuFullUpdateEvent(ent, sender);
									resp->RequestOutOfBandTransmit ();
									resp->RequestReliableTransmit();
									VuMessageQueue::PostVuMessage(resp);
								}
								else
								{
//									MonoPrint("Get Request: Sending Broadcast Global on %08x to %08x\n", ent->Id().creator_.value_, sender->Id().creator_.value_);
									resp = new VuBroadcastGlobalEvent(ent, sender);
									resp->RequestReliableTransmit();
									resp->RequestOutOfBandTransmit ();
									VuMessageQueue::PostVuMessage(resp);
								}
							}
						}

						ent = iter.GetNext();
					}
					return VU_SUCCESS;
				} 
				else if (tgtid_ == vuLocalSessionEntity->GameId())
				{
					// get all _game_ ents
					VuDatabaseIterator iter;
					VuEntity* ent = iter.GetFirst();

					while (ent)
					{
						if (!ent->IsPrivate() && ent->IsLocal() && !ent->IsGlobal())
						{
							if (ent->Id() != sender->Id())
							{
								resp = new VuFullUpdateEvent(ent, sender);
								resp->RequestReliableTransmit();
								//					resp->RequestLowPriorityTransmit();
								VuMessageQueue::PostVuMessage(resp);
							}
						}
						ent = iter.GetNext();
					}
					return VU_SUCCESS;
				}
			} 
			else if (Entity() && Entity()->OwnerId() == vuLocalSession)
			{
				resp = new VuFullUpdateEvent(Entity(), sender);
			} 
			else if (Destination() == vuLocalSession)
			{
				// we were asked specifically, so send the error response
				resp = new VuErrorMessage(VU_NO_SUCH_ENTITY_ERROR, Sender(), EntityId(), sender);
			}
			if (resp)
			{
				resp->RequestReliableTransmit();
				VuMessageQueue::PostVuMessage(resp);
				return VU_SUCCESS;
			}
		}
		else
		{
			if (entityId_ == vuNullId)
			{
				// get all _global_ ents
				VuDatabaseIterator iter;
				VuEntity* ent = iter.GetFirst();
				VuMessage *resp = 0;

				while (ent)
				{
					if (!ent->IsPrivate() && ent->IsGlobal())
					{// ent->IsLocal() && 
						if ((sender) && (ent->Id() != sender->Id()))
						{
							if (ent->IsLocal ())
							{
//								MonoPrint("Get Request: Sending Full Update on %08x to %08x\n", ent->Id().creator_.value_, sender->Id().creator_.value_);
								resp = new VuFullUpdateEvent(ent, sender);
								resp->RequestOutOfBandTransmit ();
								VuMessageQueue::PostVuMessage(resp);
							}
							else
							{
//								MonoPrint("Get Request: Sending Broadcast Global on %08x to %08x\n", ent->Id().creator_.value_, sender->Id().creator_.value_);
								resp = new VuBroadcastGlobalEvent(ent, sender);
								resp->RequestOutOfBandTransmit ();
								VuMessageQueue::PostVuMessage(resp);
							}
						}
					}

					ent = iter.GetNext();
				}
				return VU_SUCCESS;
			}
			else if ((Entity()) && (Entity ()->IsLocal ()))
			{
				VuMessage *resp = 0;

				resp = new VuFullUpdateEvent(Entity (), sender);
				VuMessageQueue::PostVuMessage(resp);
				return VU_SUCCESS;
			}
		}
	}
	return VU_NO_OP;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuPushRequest::VuPushRequest(VU_ID           entityId, 
                             VuTargetEntity* target)
  : VuRequestMessage(VU_PUSH_REQUEST_MESSAGE, entityId, target)
{
  // empty
}

VuPushRequest::VuPushRequest(VU_ID     senderid, 
                             VU_ID     target)
  : VuRequestMessage(VU_PUSH_REQUEST_MESSAGE, senderid, target)
{
  // empty
}

VuPushRequest::~VuPushRequest()
{
  // empty
}

VU_ERRCODE 
VuPushRequest::Process(VU_BOOL)
{
  int retval = VU_NO_OP;

  if (!IsLocal() && Destination() == vuLocalSession) {
    if (Entity()) {
      retval = Entity()->Handle(this);
    }
    else {
      VuTargetEntity* sender = (VuTargetEntity*)vuDatabase->Find(Sender());
      if (sender && sender->IsTarget()) {
        VuMessage* resp = new VuErrorMessage(VU_NO_SUCH_ENTITY_ERROR, Sender(),
	  			EntityId(), sender);
        resp->RequestReliableTransmit();
        VuMessageQueue::PostVuMessage(resp);
        retval = VU_SUCCESS;
      }
    }
  }
  return retval;
}

//--------------------------------------------------
VuPullRequest::VuPullRequest(VU_ID           entityId, 
                             VuTargetEntity* target)
  : VuRequestMessage(VU_PULL_REQUEST_MESSAGE, entityId, target)
{
  // empty
}

VuPullRequest::VuPullRequest(VU_ID     senderid,
                             VU_ID     target)
  : VuRequestMessage(VU_PUSH_REQUEST_MESSAGE, senderid, target)
{
  // empty
}

VuPullRequest::~VuPullRequest()
{
  // empty
}

VU_ERRCODE 
VuPullRequest::Process(VU_BOOL)
{
  int retval = VU_NO_OP;

  if (!IsLocal() && Destination() == vuLocalSession) {
    if (Entity()) {
      retval = Entity()->Handle(this);
    }
    else {
      VuTargetEntity* sender = (VuTargetEntity*)vuDatabase->Find(Sender());
      if (sender && sender->IsTarget()) {
        VuMessage* resp = new VuErrorMessage(VU_NO_SUCH_ENTITY_ERROR, Sender(),
                                  EntityId(), sender);
        resp->RequestReliableTransmit();
        VuMessageQueue::PostVuMessage(resp);
        retval = VU_SUCCESS;
      }
    }
  }

  return retval;
}

//--------------------------------------------------

VuEvent::VuEvent(VU_MSG_TYPE     type, 
                 VU_ID           entityId, 
                 VuTargetEntity* target,
                 VU_BOOL         loopback)
  : VuMessage(type, entityId, target, loopback),
    updateTime_(vuxGameTime)
{
  // empty
}

VuEvent::VuEvent(VU_MSG_TYPE type, 
                 VU_ID       senderid,
                 VU_ID       target)
  : VuMessage(type, senderid, target),
    updateTime_(vuxGameTime)
{
  // empty
}

VuEvent::~VuEvent()
{
  // empty
}

int 
VuEvent::Activate(VuEntity* ent)
{
  SetEntity(ent);
  if (IsLocal() && ent)
    updateTime_ = ent->LastUpdateTime();

//  vuDatabase->Handle(this);
  return VU_SUCCESS;
}


#define VUEVENT_LOCALSIZE (sizeof(updateTime_))
int 
VuEvent::LocalSize()
{
  return (VUEVENT_LOCALSIZE);
}

#define VUEVENT_SIZE (VUMESSAGE_SIZE+VUEVENT_LOCALSIZE)
int 
VuEvent::Size()
{
  return (VUEVENT_SIZE);
}

int 
VuEvent::Decode(VU_BYTE** buf, 
                int       length)
{
  VuMessage::Decode(buf, length);

  memcpy(&updateTime_, *buf, sizeof(updateTime_)); *buf += sizeof(updateTime_);

  return (VUEVENT_SIZE);
}

int 
VuEvent::Encode(VU_BYTE** buf)
{
  VuMessage::Encode(buf);

  memcpy(*buf, &updateTime_, sizeof(updateTime_)); *buf += sizeof(updateTime_);

  return (VUEVENT_SIZE);
}

//--------------------------------------------------
VuCreateEvent::VuCreateEvent(VuEntity*       entity, 
                             VuTargetEntity* target,
                             VU_BOOL         loopback)
  : VuEvent(VU_CREATE_EVENT, entity->Id(), target, loopback)
{
#if defined(VU_USE_CLASS_INFO)
  memcpy(classInfo_, entity->EntityType()->classInfo_, CLASS_NUM_BYTES);
#endif
  vutype_ = entity->Type();
  size_   = 0;
  data_   = 0;
  expandedData_ = 0;
}

VuCreateEvent::VuCreateEvent(VU_ID     senderid, 
                             VU_ID     target)
  : VuEvent(VU_CREATE_EVENT, senderid, target)
{
#if defined(VU_USE_CLASS_INFO)
  memset(classInfo_, '\0', CLASS_NUM_BYTES);
#endif
  size_   = 0;
  data_   = 0;
  vutype_ = 0;
  expandedData_ = 0;
}

VuCreateEvent::VuCreateEvent(VU_MSG_TYPE     type, 
                             VuEntity*       ent,
                             VuTargetEntity* target, 
                             VU_BOOL         loopback)
  : VuEvent(type, ent->Id(), target, loopback)
{
  SetEntity(ent);
#if defined(VU_USE_CLASS_INFO)
  memcpy(classInfo_, ent->EntityType()->classInfo_, CLASS_NUM_BYTES);
#endif
  vutype_ = ent->Type();
  size_ = 0;
  data_ = 0;
  expandedData_ = 0;

#ifdef _DEBUG
//  MonoPrint("VuCreateEvent id target loopback %d,%d,%d\n", entity->Id(), target,loopback);//me123
#endif
}

VuCreateEvent::VuCreateEvent(VU_MSG_TYPE type, 
                             VU_ID       senderid,
                             VU_ID       target)
  : VuEvent(type, senderid, target)
{
#if defined(VU_USE_CLASS_INFO)
  memset(classInfo_, '\0', CLASS_NUM_BYTES);
#endif
  size_   = 0;
  data_   = 0;
  vutype_ = 0;
  expandedData_ = 0;

#ifdef _DEBUG
//MonoPrint("VuCreateEvent1 senderid target %d,%d\n", senderid, target);//me123
#endif
}

VuCreateEvent::~VuCreateEvent()
{
  delete [] data_;
  if (Entity() &&					// we have an ent ...
      Entity()->VuState() == VU_MEM_ACTIVE &&	// & have not yet removed it...
      Entity()->Id().creator_ == vuLocalSession.creator_ && 
      						// & it has our session tag...
      vuDatabase->Find(Entity()->Id()) == 0) {	// & it's not in the DB...
    vuAntiDB->Insert(Entity());			// ==> put it in the anti DB
  }
  VuDeReferenceEntity(expandedData_);
}

int 
VuCreateEvent::LocalSize()
{
  ushort size = size_;
  if (Entity()) {
    size = static_cast<ushort>(Entity()->SaveSize());
  }
  return 
#if defined(VU_USE_CLASS_INFO)
	 CLASS_NUM_BYTES +
#endif
	 sizeof(vutype_) +
	 sizeof(size_) +
	 size;
}

int 
VuCreateEvent::Size()
{
  return VUEVENT_SIZE + VuCreateEvent::LocalSize();
}

int 
VuCreateEvent::Decode(VU_BYTE** buf, 
                      int       length)
{
  ushort oldsize = size_;
  int retval = VuEvent::Decode(buf, length);
#if defined(VU_USE_CLASS_INFO)
  memcpy(classInfo_, *buf, CLASS_NUM_BYTES); *buf += CLASS_NUM_BYTES;
#endif
  memcpy(&vutype_,   *buf, sizeof(vutype_)); *buf += sizeof(vutype_);
  memcpy(&size_,     *buf, sizeof(size_));   *buf += sizeof(size_);

  if (!data_ || oldsize != size_) {
    delete [] data_;
    data_ = new VU_BYTE[size_];
  }
  memcpy(data_, *buf, size_);		 *buf += size_;

  // note: this MUST be called after capturing size_ (above)
  return (retval + VuCreateEvent::LocalSize());
}

VU_BOOL 
VuCreateEvent::DoSend()
{
  if (Entity() && Entity()->VuState() == VU_MEM_ACTIVE) {
    return TRUE;
  }
  return FALSE;
}

int 
VuCreateEvent::Encode(VU_BYTE** buf)
{
#if defined(DEBUG)
  VU_BYTE *start = *buf;
#endif
  ushort oldsize = size_;
  if (Entity()) {
    size_ = static_cast<ushort>(Entity()->SaveSize());
  }
  if (Entity() && size_) {
    // copying ent in save allows multiple send's of same event
    if (size_ > oldsize) {
      if (data_) {
        delete [] data_;
      }
      data_ = new VU_BYTE[size_];
    }
    VU_BYTE *ptr = data_;
    Entity()->Save(&ptr);
  }
  
  int retval = VuEvent::Encode(buf);
  
#if defined(VU_USE_CLASS_INFO)
  memcpy(*buf, classInfo_, CLASS_NUM_BYTES); *buf += CLASS_NUM_BYTES;
#endif
  memcpy(*buf, &vutype_,  sizeof(vutype_));  *buf += sizeof(vutype_);
  memcpy(*buf, &size_,    sizeof(size_));    *buf += sizeof(size_);
  memcpy(*buf, data_,     size_);	     *buf += size_;
  retval += VuCreateEvent::LocalSize();
#if defined(DEBUG)
  assert ((int)(*buf) - (int)(start) == retval);
#endif

  return retval;
}

VU_ERRCODE 
VuCreateEvent::Activate(VuEntity* ent)
{
  return VuEvent::Activate(ent);
}

VU_ERRCODE 
VuCreateEvent::Process(VU_BOOL)
{
  if (expandedData_) {
    return VU_NO_OP;    // already done...
  }
  if (vutype_ < VU_LAST_ENTITY_TYPE) {
    expandedData_ = VuCreateEntity(vutype_, size_, data_);
  }
  else {
    expandedData_ = VuxCreateEntity(vutype_, size_, data_);
  }
  if (expandedData_) {
    VuReferenceEntity(expandedData_);
    if (!expandedData_->IsLocal()) {
      expandedData_->SetTransmissionTime(postTime_);
    }
    if (Entity() && (Entity()->OwnerId() != expandedData_->OwnerId()) && 
		Entity() != expandedData_) {
      if (Entity()->IsPrivate()) {
	Entity()->ChangeId(expandedData_);
	SetEntity(0);
      } 
      else {
	VuEntity* winner = ResolveWinner(Entity(), expandedData_);
	if (winner == Entity()) {
	  // this will prevent a db insert of expandedData
	  SetEntity(expandedData_);
	} 
        else if (winner == expandedData_) {
	  Entity()->SetOwnerId(expandedData_->OwnerId());
	  if (Entity()->Type() == expandedData_->Type()) {
	    // if we have the same type, then just transfer to winner
            VuTargetEntity *dest = 0;
            if (Entity()->IsGlobal()) {
              dest = vuGlobalGroup;
            }
            else {
              dest = vuLocalSessionEntity->Game();
            }
	    VuTransferEvent *event = new VuTransferEvent(Entity(),dest);
	    event->Ref();
	    VuMessageQueue::PostVuMessage(event);
	    Entity()->Handle(event);
	    vuDatabase->Handle(event);
	    event->UnRef();
	    SetEntity(expandedData_);
	  } 
          else {
	    type_ = VU_CREATE_EVENT;
	    if (Entity()->VuState() == VU_MEM_ACTIVE) {
	      // note: this will cause a memory leak! (but is extrememly rare)
              //   Basically, we have two ents with the same id, and we cannot
              //   keep track of both, even to know when it is safe to delete
              //   the abandoned entity -- so we remove it from VU, but don't
              //   call its destructor... the last thing we do with it is call 
              //   VuxRetireEntity, leaving ultimate cleanup up to the app
	      VuReferenceEntity(Entity());
	      vuDatabase->Remove(Entity());
	      vuAntiDB->Remove(Entity());
	    }
	    VuxRetireEntity(Entity());
	    SetEntity(0);
	  }
	}
      }
    }
    if (Entity() && type_ == VU_FULL_UPDATE_EVENT) {
      Entity()->Handle((VuFullUpdateEvent *)this);
      return VU_SUCCESS;
    }
    else if (!Entity()) {
      SetEntity(expandedData_);

// OW: me123 MP Fix
#if 0
      vuDatabase->Insert(Entity());
#else
  	  vuDatabase->SilentInsert(Entity());	 //me123 to silent otherwise this will
#endif

      return VU_SUCCESS;
    }
    return VU_NO_OP;
  }
  return VU_ERROR;
}

//--------------------------------------------------
VuManageEvent::VuManageEvent(VuEntity*       entity, 
                             VuTargetEntity* target,
                             VU_BOOL         loopback)
  : VuCreateEvent(VU_MANAGE_EVENT, entity, target, loopback)
{
  // empty
}

VuManageEvent::VuManageEvent(VU_ID     senderid, 
                             VU_ID     target)
  : VuCreateEvent(VU_MANAGE_EVENT, senderid, target)
{
  // empty
}

VuManageEvent::~VuManageEvent()
{
  // empty
}

//--------------------------------------------------
VuDeleteEvent::VuDeleteEvent(VuEntity* entity)
  : VuEvent(VU_DELETE_EVENT, entity->Id(),
      entity->IsGlobal() ? (VuTargetEntity*)vuGlobalGroup
                         : (VuTargetEntity*)vuLocalSessionEntity->Game(), TRUE)
{
  SetEntity(entity);
}

VuDeleteEvent::VuDeleteEvent(VU_ID     senderid,
                             VU_ID     target)
  : VuEvent(VU_DELETE_EVENT, senderid, target)
{
  // empty
}

VuDeleteEvent::~VuDeleteEvent()
{
  if (Entity()) {
//    Entity()->SetVuState(VU_MEM_INACTIVE);
    VuDeReferenceEntity(Entity());
  }
}

VU_ERRCODE 
VuDeleteEvent::Activate(VuEntity* ent)
{
  if (ent) {
    SetEntity(ent);
  }
  if (Entity()) {
    if (Entity()->VuState() == VU_MEM_ACTIVE) {
      vuDatabase->DeleteRemove(Entity());
      return VU_SUCCESS;
    } else if (vuAntiDB->Find(entityId_)) {
      vuAntiDB->Remove(entityId_);
      return VU_SUCCESS;
    } else if (!Entity()->IsLocal()) {
      // prevent duplicate delete event from remote source
      SetEntity(0);
    }
  }
  return VU_NO_OP;
}

VU_ERRCODE 
VuDeleteEvent::Process(VU_BOOL)
{
  if (Entity()) {
    Entity()->Handle(this);
    return VU_SUCCESS;
  }
  return VU_NO_OP;
}

//--------------------------------------------------
VuUnmanageEvent::VuUnmanageEvent(VuEntity*       entity, 
                                 VuTargetEntity* target,
                                 VU_TIME         mark,
                                 VU_BOOL         loopback)
  : VuEvent(VU_UNMANAGE_EVENT, entity->Id(), target, loopback),
    mark_(mark)
{
  SetEntity(entity);
  // set kill fuse
  VuReleaseEvent* release = new VuReleaseEvent(entity);
  VuTimerEvent*   timer   = new VuTimerEvent(0, mark, VU_DELETE_TIMER, release);
  VuMessageQueue::PostVuMessage(timer);
}

VuUnmanageEvent::VuUnmanageEvent(VU_ID     senderid,
                                 VU_ID     target)
  : VuEvent(VU_UNMANAGE_EVENT, senderid, target),
    mark_(0)
{
  // empty
}

VuUnmanageEvent::~VuUnmanageEvent()
{
  // emtpy
}

#define VUUNMANAGEEVENT_LOCALSIZE (sizeof(mark_))
int 
VuUnmanageEvent::LocalSize()
{
  return(VUUNMANAGEEVENT_LOCALSIZE);
}

#define VUUNMANAGEEVENT_SIZE (VUEVENT_SIZE+VUUNMANAGEEVENT_LOCALSIZE)
int 
VuUnmanageEvent::Size()
{
  return (VUUNMANAGEEVENT_SIZE);
}

int 
VuUnmanageEvent::Decode(VU_BYTE** buf,
                        int       length)
{
  VuEvent::Decode(buf, length);

  memcpy(&mark_, *buf, sizeof(mark_)); *buf += sizeof(mark_);

  return (VUUNMANAGEEVENT_SIZE);
}

int 
VuUnmanageEvent::Encode(VU_BYTE** buf)
{
  VuEvent::Encode(buf);

  memcpy(*buf, &mark_, sizeof(mark_)); *buf += sizeof(mark_);

  return (VUUNMANAGEEVENT_SIZE);
}

VU_ERRCODE 
VuUnmanageEvent::Process(VU_BOOL)
{
  if (Entity()) {
    Entity()->Handle(this);
    return VU_SUCCESS;
  }
  return VU_NO_OP;
}

//--------------------------------------------------
VuReleaseEvent::VuReleaseEvent(VuEntity* entity)
  : VuEvent(VU_RELEASE_EVENT, entity->Id(), vuLocalSessionEntity, TRUE)
{
  SetEntity(entity);
}

VuReleaseEvent::~VuReleaseEvent()
{
  VuDeReferenceEntity(Entity());
}

VU_BOOL 
VuReleaseEvent::DoSend()
{
  return FALSE;
}

int 
VuReleaseEvent::Size()
{
  return 0;
}

int 
VuReleaseEvent::Decode(VU_BYTE **, int)
{
  // not a net event, so just return
  return 0;
}

int 
VuReleaseEvent::Encode(VU_BYTE **)
{
  // not a net event, so just return
  return 0;
}

VU_ERRCODE 
VuReleaseEvent::Activate(VuEntity* ent)
{
  if (ent) {
    SetEntity(ent);
  }
  if (Entity() && Entity()->VuState() == VU_MEM_ACTIVE) {
    if (Entity()->Id().creator_ == vuLocalSession.creator_) {
      // anti db will check for presense in db prior to insert
      vuAntiDB->Insert(Entity());
    }
    Entity()->share_.ownerId_ = vuNullId;
    vuDatabase->DeleteRemove(Entity());
    return VU_SUCCESS;
  }
  return VU_NO_OP;
}

VU_ERRCODE 
VuReleaseEvent::Process(VU_BOOL)
{
  if (Entity()) {
    Entity()->Handle(this);
    return VU_SUCCESS;
  }
  return VU_NO_OP;
}

//--------------------------------------------------

VuTransferEvent::VuTransferEvent(VuEntity*       entity,
                                 VuTargetEntity* target,
                                 VU_BOOL         loopback)
  : VuEvent(VU_TRANSFER_EVENT, entity->Id(), target, loopback),
    newOwnerId_(entity->OwnerId())
{
  SetEntity(entity);
}

VuTransferEvent::VuTransferEvent(VU_ID senderid, VU_ID target)
  : VuEvent(VU_TRANSFER_EVENT, senderid, target),
    newOwnerId_(vuNullId)
{
  // empty
}

VuTransferEvent::~VuTransferEvent()
{
  // empty
}

#define VUTRANSFEREVENT_LOCALSIZE (sizeof(newOwnerId_))
int 
VuTransferEvent::LocalSize()
{
  return(VUTRANSFEREVENT_LOCALSIZE);
}

#define VUTRANSFEREVENT_SIZE (VUEVENT_SIZE+VUTRANSFEREVENT_LOCALSIZE)
int 
VuTransferEvent::Size()
{
  return(VUTRANSFEREVENT_SIZE);
}

int 
VuTransferEvent::Decode(VU_BYTE** buf, 
                        int       length)
{
  VuEvent::Decode(buf, length);

  memcpy(&newOwnerId_, *buf, sizeof(newOwnerId_)); *buf += sizeof(newOwnerId_);

  return(VUTRANSFEREVENT_SIZE);
}

int 
VuTransferEvent::Encode(VU_BYTE** buf)
{
  VuEvent::Encode(buf);

  memcpy(*buf, &newOwnerId_, sizeof(newOwnerId_)); *buf += sizeof(newOwnerId_);

  return(VUTRANSFEREVENT_SIZE);
}

VU_ERRCODE 
VuTransferEvent::Activate(VuEntity* ent)
{
  return VuEvent::Activate(ent);
}

VU_ERRCODE 
VuTransferEvent::Process(VU_BOOL)
{
  if (Entity()) {
    Entity()->Handle(this);
    return VU_SUCCESS;
  }
  return VU_NO_OP;
}

//--------------------------------------------------
VuPositionUpdateEvent::VuPositionUpdateEvent(VuEntity*       entity,
                                             VuTargetEntity* target,
                                             VU_BOOL         loopback)
  : VuEvent(VU_POSITION_UPDATE_EVENT, entity->Id(), target, loopback)
{
  if (entity) {
    SetEntity(entity);
    updateTime_ = entity->LastUpdateTime();

    x_  = entity->XPos();
    y_  = entity->YPos();
    z_  = entity->ZPos();

    dx_ = entity->XDelta();
    dy_ = entity->YDelta();
    dz_ = entity->ZDelta();

#if defined(VU_USE_QUATERNION)
    VU_QUAT *quat = entity->Quat();
    ML_QuatCopy(quat_, *quat);
    VU_VECT *dquat = entity->DeltaQuat();
    ML_VectCopy(dquat_, *dquat);
    theta_ = entity->Theta();
#else
    yaw_    = entity->Yaw();
    pitch_  = entity->Pitch();
    roll_   = entity->Roll();
    
    //dyaw_   = entity->YawDelta();
    //dpitch_ = entity->PitchDelta();
    //droll_  = entity->RollDelta();
    //VU_PRINT("yaw=%3.3f, pitch=%3.3f, roll=%3.3f, dyaw=%3.3f, dpitch=%3.3f, droll=%3.3f\n", yaw_, pitch_, roll_, dyaw_, dpitch_, droll_);
#endif
  }
}

VuPositionUpdateEvent::VuPositionUpdateEvent(VU_ID     senderid,
                                             VU_ID     target)
  : VuEvent(VU_POSITION_UPDATE_EVENT, senderid, target)
{
  // empty
}

VuPositionUpdateEvent::~VuPositionUpdateEvent()
{
  // empty
}

VU_BOOL 
VuPositionUpdateEvent::DoSend()
{
  // test is done in vudriver.cpp, prior to generation of event
  return TRUE;
}

#if defined(VU_USE_QUATERNION)

#if defined(VU_PACK_POSITION)
#define VUPOSITIONUPDATEEVENT_LOCALSIZE (sizeof(x_)+sizeof(y_)+sizeof(z_)+sizeof(dx_)+sizeof(dy_)+sizeof(dz_)+sizeof(quat_ )+sizeof(dquat_)+sizeof(theta_))
#else
#define VUPOSITIONUPDATEEVENT_LOCALSIZE (sizeof(x_)+sizeof(y_)+sizeof(z_)+sizeof(dx_)+sizeof(dy_)+sizeof(dz_)+sizeof(quat_ )+sizeof(dquat_)+sizeof(theta_))
#endif

#else

#if defined(VU_PACK_POSITION)
#define VUPOSITIONUPDATEEVENT_LOCALSIZE (sizeof(x_)+sizeof(y_)+sizeof(z_)+sizeof(dx_)+sizeof(dy_)+sizeof(dz_)+sizeof(yaw_)+sizeof(pitch_)+sizeof(roll_))
#else
#define VUPOSITIONUPDATEEVENT_LOCALSIZE (sizeof(x_)+sizeof(y_)+sizeof(z_)+sizeof(dx_)+sizeof(dy_)+sizeof(dz_)+sizeof(yaw_)+sizeof(pitch_)+sizeof(roll_))
#endif

#endif

int 
VuPositionUpdateEvent::LocalSize()
{
  return(VUPOSITIONUPDATEEVENT_LOCALSIZE);
}

#define VUPOSITIONUPDATEEVENT_SIZE (VUEVENT_SIZE+VUPOSITIONUPDATEEVENT_LOCALSIZE)
int 
VuPositionUpdateEvent::Size()
{
  return (VUPOSITIONUPDATEEVENT_SIZE);
}

int 
VuPositionUpdateEvent::Decode(VU_BYTE** buf,
                              int       length)
{
  VuEvent::Decode(buf, length);

#if defined(VU_PACK_POSITION)
  // Insert packed positions and deltas here
  memcpy(&x_,  *buf, sizeof(x_));  *buf += sizeof(x_);
  memcpy(&y_,  *buf, sizeof(y_));  *buf += sizeof(y_);
  memcpy(&z_,  *buf, sizeof(z_));  *buf += sizeof(z_);
  memcpy(&dx_, *buf, sizeof(dx_)); *buf += sizeof(dx_);
  memcpy(&dy_, *buf, sizeof(dy_)); *buf += sizeof(dy_);
  memcpy(&dz_, *buf, sizeof(dz_)); *buf += sizeof(dz_);
#else
  memcpy(&x_,  *buf, sizeof(x_));  *buf += sizeof(x_);
  memcpy(&y_,  *buf, sizeof(y_));  *buf += sizeof(y_);
  memcpy(&z_,  *buf, sizeof(z_));  *buf += sizeof(z_);
  memcpy(&dx_, *buf, sizeof(dx_)); *buf += sizeof(dx_);
  memcpy(&dy_, *buf, sizeof(dy_)); *buf += sizeof(dy_);
  memcpy(&dz_, *buf, sizeof(dz_)); *buf += sizeof(dz_);
#endif
    
#if defined(VU_USE_QUATERNION)
  memcpy(&quat_,   *buf, sizeof(quat_));   *buf += sizeof(quat_);
  memcpy(&dquat_,  *buf, sizeof(dquat_));  *buf += sizeof(dquat_);
  memcpy(&theta_,  *buf, sizeof(theta_));  *buf += sizeof(theta_);
#else
#if defined(VU_PACK_POSITION)
  memcpy(&yaw_,    *buf, sizeof(yaw_));    *buf += sizeof(yaw_);
  memcpy(&pitch_,  *buf, sizeof(pitch_));  *buf += sizeof(pitch_);
  memcpy(&roll_,   *buf, sizeof(roll_));   *buf += sizeof(roll_);

  //memcpy(&yaw_,    *buf, sizeof(yaw_));    *buf += sizeof(yaw_);
  //memcpy(&pitch_,  *buf, sizeof(pitch_));  *buf += sizeof(pitch_);
  //memcpy(&roll_,   *buf, sizeof(roll_));   *buf += sizeof(roll_);
#else
  memcpy(&yaw_,    *buf, sizeof(yaw_));    *buf += sizeof(yaw_);
  memcpy(&pitch_,  *buf, sizeof(pitch_));  *buf += sizeof(pitch_);
  memcpy(&roll_,   *buf, sizeof(roll_));   *buf += sizeof(roll_);
  //memcpy(&dyaw_,   *buf, sizeof(dyaw_));   *buf += sizeof(dyaw_);
  //memcpy(&dpitch_, *buf, sizeof(dpitch_)); *buf += sizeof(dpitch_);
  //memcpy(&droll_,  *buf, sizeof(droll_));  *buf += sizeof(droll_);
#endif
#endif

  return(VUPOSITIONUPDATEEVENT_SIZE);
}


int
VuPositionUpdateEvent::Encode(VU_BYTE** buf)
{
  VuEvent::Encode(buf);
  
#if defined(VU_PACK_POSITION)
  // insert packed positions and deltas here
  memcpy(*buf, &x_,  sizeof(x_));  *buf += sizeof(x_);
  memcpy(*buf, &y_,  sizeof(y_));  *buf += sizeof(y_);
  memcpy(*buf, &z_,  sizeof(z_));  *buf += sizeof(z_);
  memcpy(*buf, &dx_, sizeof(dx_)); *buf += sizeof(dx_);
  memcpy(*buf, &dy_, sizeof(dy_)); *buf += sizeof(dy_);
  memcpy(*buf, &dz_, sizeof(dz_)); *buf += sizeof(dz_);
#else
  memcpy(*buf, &x_,  sizeof(x_));  *buf += sizeof(x_);
  memcpy(*buf, &y_,  sizeof(y_));  *buf += sizeof(y_);
  memcpy(*buf, &z_,  sizeof(z_));  *buf += sizeof(z_);
  memcpy(*buf, &dx_, sizeof(dx_)); *buf += sizeof(dx_);
  memcpy(*buf, &dy_, sizeof(dy_)); *buf += sizeof(dy_);
  memcpy(*buf, &dz_, sizeof(dz_)); *buf += sizeof(dz_);
#endif

#if defined(VU_USE_QUATERNION)
  memcpy(*buf, &quat_,   sizeof(quat_));   *buf += sizeof(quat_);
  memcpy(*buf, &dquat_,  sizeof(dquat_));  *buf += sizeof(dquat_);
  memcpy(*buf, &theta_,  sizeof(theta_));  *buf += sizeof(theta_);
#else
#if defined(VU_PACK_POSITION)
  memcpy(*buf, &yaw_,    sizeof(yaw_));    *buf += sizeof(yaw_);
  memcpy(*buf, &pitch_,  sizeof(pitch_));  *buf += sizeof(pitch_);
  memcpy(*buf, &roll_,   sizeof(roll_));   *buf += sizeof(roll_);
#else
  memcpy(*buf, &yaw_,    sizeof(yaw_));    *buf += sizeof(yaw_);
  memcpy(*buf, &pitch_,  sizeof(pitch_));  *buf += sizeof(pitch_);
  memcpy(*buf, &roll_,   sizeof(roll_));   *buf += sizeof(roll_);
  //memcpy(*buf, &dyaw_,   sizeof(dyaw_));   *buf += sizeof(dyaw_);
  //memcpy(*buf, &dpitch_, sizeof(dpitch_)); *buf += sizeof(dpitch_);
  //memcpy(*buf, &droll_,  sizeof(droll_));  *buf += sizeof(droll_);
#endif
#endif
  
  return(VUPOSITIONUPDATEEVENT_SIZE);
}

VU_ERRCODE 
VuPositionUpdateEvent::Process(VU_BOOL)
{
  if (Entity()) {
    Entity()->Handle(this);
    return VU_SUCCESS;
  }
  return VU_NO_OP;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuBroadcastGlobalEvent::VuBroadcastGlobalEvent (VuEntity *entity, VuTargetEntity* target, VU_BOOL loopback) :
	VuEvent(VU_BROADCAST_GLOBAL_EVENT, entity->Id(), target, loopback)
{

#if defined(VU_USE_CLASS_INFO)
	memcpy(classInfo_, entity->EntityType()->classInfo_, CLASS_NUM_BYTES);
#endif


	vutype_ = entity->Type();

	SetEntity(entity);
	updateTime_ = entity->LastUpdateTime();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuBroadcastGlobalEvent::VuBroadcastGlobalEvent (VU_ID senderid, VU_ID target) :
	VuEvent(VU_BROADCAST_GLOBAL_EVENT, senderid, target)
{
#if defined(VU_USE_CLASS_INFO)
	memset(classInfo_, '\0', CLASS_NUM_BYTES);
#endif
	vutype_ = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuBroadcastGlobalEvent::~VuBroadcastGlobalEvent ()
{
	// empty
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int VuBroadcastGlobalEvent::Size (void)
{
	return VUEVENT_SIZE;

//#if defined(VU_USE_CLASS_INFO)
//	 CLASS_NUM_BYTES +
//#endif
//	 sizeof (vutype_);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int VuBroadcastGlobalEvent::Decode (VU_BYTE** buf, int length)
{
//	ushort oldsize = size_;
//	VU_BYTE *start = *buf;
	int retval = VuEvent::Decode(buf, length);

//#if defined(VU_USE_CLASS_INFO)
//	memcpy(classInfo_, *buf, CLASS_NUM_BYTES); *buf += CLASS_NUM_BYTES;
//#endif
//	memcpy(&vutype_,   *buf, sizeof(vutype_)); *buf += sizeof(vutype_);

//	return *buf - start;

	return retval;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int VuBroadcastGlobalEvent::Encode (VU_BYTE** buf)
{
//	VU_BYTE *start = *buf;
//	ushort oldsize = size_;
	int retval = VuEvent::Encode(buf);

//#if defined(VU_USE_CLASS_INFO)
//	memcpy(*buf, classInfo_, CLASS_NUM_BYTES); *buf += CLASS_NUM_BYTES;
//#endif
//	memcpy(*buf, &vutype_,  sizeof(vutype_));  *buf += sizeof(vutype_);

//	return *buf - start;;

	return retval;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE VuBroadcastGlobalEvent::Activate(VuEntity* ent)
{
	return VuEvent::Activate(ent);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE VuBroadcastGlobalEvent::Process(VU_BOOL autod)
{
	if (autod)
	{
		return 0;
	}
	
//	MonoPrint ("BroadcastGlobal::Process : %08x%08x %08x\n", EntityId (), Entity());

	if (!Entity())
	{
		// Add this session to the global group - so we can send the stuff below...
		VuxAddDanglingSession (EntityId ().creator_);

		if (EntityId ().num_ != VU_SESSION_ENTITY_ID)
		{
			// Send a get request for the entity if its not a session
			VuGetRequest *msg = new VuGetRequest (EntityId (), vuGlobalGroup);
			msg->RequestOutOfBandTransmit();
			msg->Send ();
		}
		else
		{
			{
				// Send a get request incase we have a bad link - and that packet was lost
//				VuGetRequest *msg = new VuGetRequest (VU_GET_GLOBAL_ENTS);
//				msg->RequestOutOfBandTransmit();
//				msg->Send ();
			}

			{
				// Tell everybody we know, that we exist
				// MonoPrint ("BroadcastGlobal::Process : %08x%08x\n", EntityId ());
//				VuBroadcastGlobalEvent *msg = new VuBroadcastGlobalEvent (vuLocalSessionEntity, vuGlobalGroup);
//				msg->RequestOutOfBandTransmit();
//				msg->Send ();
			}

			vuLocalSessionEntity->SetDirty ();	// Send Reliably later - but for now...

			{
				// MonoPrint ("Sending Full Update Unreliably\n");
				// Send full update unreliably
				VuFullUpdateEvent *msg = new VuFullUpdateEvent (vuLocalSessionEntity, vuGlobalGroup);
				msg->RequestOutOfBandTransmit();
				msg->Send ();
			}
		}
	}

	return 1;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuFullUpdateEvent::VuFullUpdateEvent(VuEntity*       entity,
                                     VuTargetEntity* target,
                                     VU_BOOL         loopback)
  : VuCreateEvent(VU_FULL_UPDATE_EVENT, entity, target, loopback)
{
  SetEntity(entity);
  updateTime_ = entity->LastUpdateTime();
}

VuFullUpdateEvent::VuFullUpdateEvent(VU_ID     senderid, 
                                     VU_ID     target)
  : VuCreateEvent(VU_FULL_UPDATE_EVENT, senderid, target)
{
  // empty
}

VuFullUpdateEvent::~VuFullUpdateEvent()
{
  // empty
}

VU_ERRCODE 
VuFullUpdateEvent::Activate(VuEntity* ent)
{
  if (!ent) {
    // morph this into a create event
    type_ = VU_CREATE_EVENT;
  }
  return VuEvent::Activate(ent);
}

//--------------------------------------------------
VuEntityCollisionEvent::VuEntityCollisionEvent(VuEntity*       entity,
                                               VU_ID           otherId,
                                               VU_DAMAGE       hitLocation,
                                               int             hitEffect,
                                               VuTargetEntity* target,
                                               VU_BOOL         loopback)
  : VuEvent(VU_ENTITY_COLLISION_EVENT, entity->Id(), target, loopback),
    otherId_(otherId),
    hitLocation_(hitLocation),
    hitEffect_(hitEffect)
{
  SetEntity(entity);
}

VuEntityCollisionEvent::VuEntityCollisionEvent(VU_ID     senderid, 
                                               VU_ID     target)
  : VuEvent(VU_ENTITY_COLLISION_EVENT, senderid, target),
    otherId_(0, 0)
{
  // empty
}

VuEntityCollisionEvent::~VuEntityCollisionEvent()
{
  // empty
}

#define VUENTITYCOLLISIONEVENT_LOCALSIZE (sizeof(otherId_)+sizeof(hitLocation_)+sizeof(hitEffect_))
int 
VuEntityCollisionEvent::LocalSize()
{
  return (VUENTITYCOLLISIONEVENT_LOCALSIZE);
}

#define VUENTITYCOLLISIONEVENT_SIZE (VUEVENT_SIZE+VUENTITYCOLLISIONEVENT_LOCALSIZE)
int 
VuEntityCollisionEvent::Size()
{
  return (VUENTITYCOLLISIONEVENT_SIZE);
}

int 
VuEntityCollisionEvent::Decode(VU_BYTE** buf,
                               int       length)
{
  VuEvent::Decode(buf, length);

  memcpy(&otherId_,     *buf, sizeof(otherId_));     *buf += sizeof(otherId_);
  memcpy(&hitLocation_, *buf, sizeof(hitLocation_)); *buf += sizeof(hitLocation_);
  memcpy(&hitEffect_,   *buf, sizeof(hitEffect_));   *buf += sizeof(hitEffect_);

  return (VUENTITYCOLLISIONEVENT_SIZE);
}

int 
VuEntityCollisionEvent::Encode(VU_BYTE** buf)
{
  VuEvent::Encode(buf);

  memcpy(*buf, &otherId_,     sizeof(otherId_));     *buf += sizeof(otherId_);
  memcpy(*buf, &hitLocation_, sizeof(hitLocation_)); *buf += sizeof(hitLocation_);
  memcpy(*buf, &hitEffect_,   sizeof(hitEffect_));   *buf += sizeof(hitEffect_);

  return (VUENTITYCOLLISIONEVENT_SIZE);
}

VU_ERRCODE 
VuEntityCollisionEvent::Process(VU_BOOL)
{
  if (Entity()) {
    Entity()->Handle(this);
    return VU_SUCCESS;
  }
  return VU_NO_OP;
}

//--------------------------------------------------
VuGroundCollisionEvent::VuGroundCollisionEvent(VuEntity*       entity,
                                               VuTargetEntity* target,
                                               VU_BOOL         loopback)
  : VuEvent(VU_GROUND_COLLISION_EVENT, entity->Id(), target, loopback)
{
  // empty
}

VuGroundCollisionEvent::VuGroundCollisionEvent(VU_ID     senderid,
                                               VU_ID     target)
  : VuEvent(VU_GROUND_COLLISION_EVENT, senderid, target)
{
  // empty
}

VuGroundCollisionEvent::~VuGroundCollisionEvent()
{
  // empty
}

VU_ERRCODE 
VuGroundCollisionEvent::Process(VU_BOOL)
{
  if (Entity()) {
    Entity()->Handle(this);
    return VU_SUCCESS;
  }
  return VU_NO_OP;
}

//--------------------------------------------------
VuSessionEvent::VuSessionEvent(VuEntity*       ent, 
                               ushort          subtype,
                               VuTargetEntity* target, 
                               VU_BOOL         loopback)
  : VuEvent(VU_SESSION_EVENT, ent->Id(), target, loopback),
    subtype_(subtype),
    group_(0,0),
    callsign_(0),
    syncState_(VU_NO_SYNC),
    gameTime_(vuxGameTime)
{
  char *name = "bad session";
  if (ent->IsSession()) {
    name   = ((VuSessionEntity*)ent)->Callsign();
    group_ = ((VuSessionEntity*)ent)->GameId();

#if defined(VU_TRACK_LATENCY)
    syncState_ = ((VuSessionEntity*)ent)->TimeSyncState();
#endif

  } 
  else if (ent->IsGroup()) {
    name = ((VuGroupEntity*)ent)->GroupName();
  }
  int len   = strlen(name);
  callsign_ = new char[len+1];
  strcpy(callsign_, name);

  SetEntity(ent);
  RequestReliableTransmit ();
  RequestOutOfBandTransmit ();
}

VuSessionEvent::VuSessionEvent(VU_ID     senderid, 
                               VU_ID     target)
  : VuEvent(VU_SESSION_EVENT, senderid, target),
    subtype_(VU_SESSION_UNKNOWN_SUBTYPE),
    group_(0,0),
    callsign_(0),
    syncState_(VU_NO_SYNC),
    gameTime_(vuxGameTime)
{
  //empty
  RequestReliableTransmit ();
}

VuSessionEvent::~VuSessionEvent()
{
  delete [] callsign_;
}

int 
VuSessionEvent::LocalSize()
{
  return sizeof(sender_) +
	 sizeof(entityId_) +
	 sizeof(group_) +
	 sizeof(subtype_) +
	 sizeof(syncState_) +
	 sizeof(gameTime_) +
	 (callsign_ ? strlen(callsign_)+1 : 1);
}

int 
VuSessionEvent::Size()
{
  return VuSessionEvent::LocalSize();
}

int 
VuSessionEvent::Decode(VU_BYTE** buf, int)
{
  VU_BYTE len = 0;

  memcpy(&sender_,   *buf, sizeof(sender_));   *buf += sizeof(sender_);
  memcpy(&entityId_, *buf, sizeof(entityId_)); *buf += sizeof(entityId_);
  memcpy(&group_,    *buf, sizeof(group_));    *buf += sizeof(group_);
  memcpy(&subtype_,  *buf, sizeof(subtype_));  *buf += sizeof(subtype_);
  memcpy(&len,       *buf, sizeof(VU_BYTE));   *buf += sizeof(VU_BYTE);

  if (len) {
    delete callsign_;
    callsign_ = new char[len + 1];
    memcpy(callsign_, *buf, len); *buf += len;
    callsign_[len] = '\0';
  }

  memcpy(&syncState_, *buf, sizeof(syncState_)); *buf += sizeof(syncState_);
  memcpy(&gameTime_,  *buf, sizeof(gameTime_));  *buf += sizeof(gameTime_);

  int retval = VuSessionEvent::LocalSize();

  return retval;
}

int 
VuSessionEvent::Encode(VU_BYTE** buf)
{
  VU_BYTE len;

  if (callsign_)
    len = static_cast<VU_BYTE>(strlen(callsign_));
  else
    len = 0;

  memcpy(*buf, &sender_,    sizeof(sender_));    *buf += sizeof(sender_);
  memcpy(*buf, &entityId_,  sizeof(entityId_));  *buf += sizeof(entityId_);
  memcpy(*buf, &group_,     sizeof(group_));     *buf += sizeof(group_);
  memcpy(*buf, &subtype_,   sizeof(subtype_));   *buf += sizeof(subtype_);
  memcpy(*buf, &len,        sizeof(VU_BYTE));    *buf += sizeof(VU_BYTE);
  memcpy(*buf, callsign_,   len);                *buf += len;
  memcpy(*buf, &syncState_, sizeof(syncState_)); *buf += sizeof(syncState_);
  memcpy(*buf, &gameTime_,  sizeof(gameTime_));  *buf += sizeof(gameTime_);

  int retval = VuSessionEvent::LocalSize();

  return retval;
}

VU_ERRCODE 
VuSessionEvent::Process(VU_BOOL)
{
  if (Entity()) {
    Entity()->Handle(this);
    return VU_SUCCESS;
  }
  return VU_NO_OP;
}

//--------------------------------------------------
VuTimerEvent::VuTimerEvent(VuEntity*  entity,
                           VU_TIME    mark,
                           ushort     timertype,
                           VuMessage* event)
  : VuEvent(VU_TIMER_EVENT, (entity ? entity->Id() : vuNullId), vuLocalSessionEntity, TRUE),
    mark_(mark),
    timertype_(timertype),
    event_(event),
    next_(0)
{
  SetEntity(entity);
  if (event_) {
    event_->Ref();
  }
}

VuTimerEvent::~VuTimerEvent()
{
  // empty
}

VU_BOOL 
VuTimerEvent::DoSend()
{
  return FALSE;
}

int 
VuTimerEvent::Size()
{
  return 0;
}

int 
VuTimerEvent::Decode(VU_BYTE**, int)
{
  // not a net event, so just return
  return 0;
}

int 
VuTimerEvent::Encode(VU_BYTE**)
{
  // not a net event, so just return
  return 0;
}

VU_ERRCODE 
VuTimerEvent::Process(VU_BOOL)
{
  int retval = VU_NO_OP;

  if (event_) {
#if defined(DEBUG_COMMS)
    VU_PRINT("VU: Posting timer delayed event id %d -- time = %d\n", msgid_.id_, vuxRealTime);
#endif

	if (event_->Target() && event_->Target() != vuLocalSessionEntity)//me123 from Target() to event_->Target()
	{
		retval = event_->Send ();
	}
	else
	{
		retval = event_->Dispatch (FALSE);
	}

	//VuMessageQueue::PostVuMessage(event_);
	event_->UnRef();
	retval = VU_SUCCESS;
  }

  if (EntityId() != vuNullId) {
    if (Entity()) {
      Entity()->Handle(this);
      retval++;
    }
  }
  return retval;
}

//--------------------------------------------------
VuShutdownEvent::VuShutdownEvent(VU_BOOL all)
  : VuEvent(VU_SHUTDOWN_EVENT, vuNullId, vuLocalSessionEntity, TRUE),
    shutdownAll_(all),
    done_(FALSE)
{
  // empty
}

VuShutdownEvent::~VuShutdownEvent()
{
  vuAntiDB->Purge();
}

VU_BOOL 
VuShutdownEvent::DoSend()
{
  return FALSE;
}

int 
VuShutdownEvent::Size()
{
  return 0;
}

int 
VuShutdownEvent::Decode(VU_BYTE **, int)
{
  // not a net event, so just return
  return 0;
}

int 
VuShutdownEvent::Encode(VU_BYTE **)
{
  // not a net event, so just return
  return 0;
}

VU_ERRCODE 
VuShutdownEvent::Process(VU_BOOL)
{
  if (!done_) {
    done_ = TRUE;
    vuCollectionManager->Shutdown(shutdownAll_);
    return VU_SUCCESS;
  }
  return VU_NO_OP;
}

#if defined(VU_SIMPLE_LATENCY)
//--------------------------------------------------
VuTimingMessage::VuTimingMessage(VU_ID           entityId, 
                                 VuTargetEntity* target, 
                                 VU_BOOL)
  : VuMessage(VU_TIMING_MESSAGE, entityId, target, FALSE)
{
  // empty
  RequestOutOfBandTransmit();
}

VuTimingMessage::VuTimingMessage(VU_ID     senderid, 
                                 VU_ID     target)
  : VuMessage(VU_TIMING_MESSAGE, senderid, target)
{
}

VuTimingMessage::~VuTimingMessage()
{
  // empty
}

#define VUTIMINGMESSAGE_SIZE (VUMESSAGE_SIZE+sizeof(VU_TIME)+sizeof(VU_TIME)+sizeof(VU_TIME))
int 
VuTimingMessage::Size()
{
  return VuMessage::Size() + sizeof(VU_TIME) + sizeof(VU_TIME) + sizeof(VU_TIME);
}

int 
VuTimingMessage::Decode(VU_BYTE** buf, 
                        int       length)
{
  VU_BYTE	*start = *buf;
  VuSessionEntity	*session;

  VuMessage::Decode(buf,length);

  memcpy(&sessionRealSendTime_, *buf, sizeof(VU_TIME));  *buf += sizeof(VU_TIME);
  memcpy(&sessionGameSendTime_, *buf, sizeof(VU_TIME));  *buf += sizeof(VU_TIME);
  memcpy(&remoteGameTime_,      *buf, sizeof(VU_TIME));  *buf += sizeof(VU_TIME);

  session = (VuSessionEntity*) vuDatabase->Find(EntityId());

  if (session) {
    float	compression = 0.0F;
    int		deltal,delta1,delta2;
    // Apply the new latency
    session->SetLatency((vuxRealTime - sessionRealSendTime_)/2);
    // Determine game delta - KCK: This is overly complex for now, as I wanted to
    // check some assumptions.
    // Determine time compression
    if (vuxRealTime - sessionRealSendTime_)	{
      compression = static_cast<float>((vuxGameTime - sessionGameSendTime_) / (vuxRealTime - sessionRealSendTime_)); 
    }
    // Determine time deltas due to latency
    deltal = FTOL(-1.0F * session->Latency() * compression);
    // Add time deltas due to time differences
    delta1 = (vuxGameTime - remoteGameTime_) - deltal;
    delta2 = -1 * ((remoteGameTime_ - sessionGameSendTime_) - deltal);
    // Apply the new time delta - average the two delta we have
    session->SetTimeDelta((delta1+delta2)/2);
  }
  
  return (int)(*buf-start);
}

int 
VuTimingMessage::Encode(VU_BYTE** buf)
{
  VU_BYTE *start = *buf;

  VuMessage::Encode(buf);

  memcpy(*buf, &sessionRealSendTime_, sizeof(VU_TIME));  *buf += sizeof(VU_TIME);
  memcpy(*buf, &sessionGameSendTime_, sizeof(VU_TIME));  *buf += sizeof(VU_TIME);
  memcpy(*buf, &remoteGameTime_,      sizeof(VU_TIME));  *buf += sizeof(VU_TIME);

  return (int)(*buf-start);
}

VU_ERRCODE 
VuTimingMessage::Process(VU_BOOL)
{
  return VU_NO_OP;
}

#endif VU_SIMPLE_LATENCY

//--------------------------------------------------
VuUnknownMessage::VuUnknownMessage()
  : VuMessage(VU_UNKNOWN_MESSAGE, vuNullId, vuLocalSessionEntity, TRUE)
{
  // empty
}

VuUnknownMessage::~VuUnknownMessage()
{
  // empty
}

VU_BOOL 
VuUnknownMessage::DoSend()
{
  return FALSE;
}

int 
VuUnknownMessage::Size()
{
  return 0;
}

int 
VuUnknownMessage::Decode(VU_BYTE **, int)
{
  // not a net event, so just return
  return 0;
}

int 
VuUnknownMessage::Encode(VU_BYTE **)
{
  // Not a net event, so just return
  return 0;
}

VU_ERRCODE 
VuUnknownMessage::Process(VU_BOOL)
{
  return VU_NO_OP;
}
