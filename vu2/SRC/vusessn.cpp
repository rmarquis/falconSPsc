// Copyright (c) 1998,  MicroProse, Inc.  All Rights Reserved

extern "C" {
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <windows.h> // JB 010318 Needed for CTD checks
//__declspec(dllimport) int __stdcall GetTickCount(void);
__declspec(dllimport) DWORD __stdcall GetTickCount(void);
};

#include "vu2.h"
#include "vu_priv.h"
#include "FalcSess.h"
//#define DEBUG_COMMS 1 
//#define DEBUG_SEND 1
#define LAG_COUNT_START 4

extern VuMainThread* vuMainThread;
extern int           F4LatencyPercentage;

VU_SESSION_ID vuNullSession(0);
VU_SESSION_ID vuKnownConnectionId(0);

//-----------------------------------------------------------------------------
//  packet & message header stuff
//-----------------------------------------------------------------------------
#if defined(VU_USE_COMMS)
struct VuPacketHeader {
  VuPacketHeader(VU_ID tid)
        : targetId_(tid) {}
  VuPacketHeader() {}

  VU_ID     targetId_;  // id of target (for destination filtering)
};

struct VuMessageHeader {
  VuMessageHeader(VU_MSG_TYPE type, ushort length)
                : type_(type), length_(length) {}
  VuMessageHeader() {}

    // data
  VU_MSG_TYPE type_;
  ushort  length_;
};

static int
ReadPacketHeader(VU_BYTE*        data, 
                 VuPacketHeader* hdr)
{
  if (vuKnownConnectionId != vuNullSession) {
    // read nothing  -- fill in from known data
    hdr->targetId_.creator_ = vuKnownConnectionId;
    hdr->targetId_.num_     = VU_SESSION_ENTITY_ID;
    return 0;
  }

  memcpy(&hdr->targetId_.creator_, data, sizeof(hdr->targetId_.creator_));
  data += sizeof(hdr->targetId_.creator_);
  memcpy(&hdr->targetId_.num_, data, sizeof(hdr->targetId_.num_));

  return PACKET_HDR_SIZE;
}

static int
WritePacketHeader(VU_BYTE*        data, 
                  VuPacketHeader* hdr)
{
  if (vuKnownConnectionId != vuNullSession) {
    // write nothing -- destination will fill in from known data
    return 0;
  }
  memcpy(data, &hdr->targetId_.creator_, sizeof(hdr->targetId_.creator_));
  data += sizeof(hdr->targetId_.creator_);
  memcpy(data, &hdr->targetId_.num_, sizeof(hdr->targetId_.num_));

  return PACKET_HDR_SIZE;
}

static int
ReadMessageHeader(VU_BYTE*         data, 
                  VuMessageHeader* hdr)
{
  int retsize;
  memcpy(&hdr->type_, data, sizeof(hdr->type_));
  data += sizeof(hdr->type_);

  if (*data & 0x80) {   // test for high bit set
    hdr->length_  = static_cast<ushort>(((data[0] << 8) | data[1]));
    hdr->length_ &= 0x7fff; // mask off huffman bit
    retsize       = MAX_MSG_HDR_SIZE;
  } 
  else {
    hdr->length_ = *data;
    retsize      = MIN_MSG_HDR_SIZE;
  }
  
  return retsize;
}

static int
WriteMessageHeader(VU_BYTE*         data, 
                   VuMessageHeader* hdr)
{
  int retsize;

  memcpy(data, &hdr->type_, sizeof(hdr->type_));
  data += sizeof(hdr->type_);

  if (hdr->length_ > 0x7f) {
    ushort huff = static_cast<ushort>(0x8000 | hdr->length_);
    data[0] = static_cast<VU_BYTE>((huff >> 8) & 0xff);
    data[1] = static_cast<VU_BYTE>(huff & 0xff);
    retsize = MAX_MSG_HDR_SIZE;
  }
  else {
    *data   = (VU_BYTE)hdr->length_;
    retsize = MIN_MSG_HDR_SIZE;
  }

  return retsize;
}

//-----------------------------------------------------------------------------
static int
MessageReceive(VU_ID       targetId, 
               VU_ID       senderid,
               VU_MSG_TYPE type,
               VU_BYTE**   data,
               int         size,
               VU_TIME     timestamp)
{
  VuMessage* event = 0;

#ifdef DEBUG_COMMS
  VU_PRINT("VU: Receive [%d]", type);
#endif

  if (vuLocalSessionEntity->InTarget(targetId)) {
    switch (type) {
    case VU_GET_REQUEST_MESSAGE:
      event = new VuGetRequest(senderid, targetId);
      break;
    case VU_PUSH_REQUEST_MESSAGE:
      event = new VuPushRequest(senderid, targetId);
      break;
    case VU_PULL_REQUEST_MESSAGE:
      event = new VuPullRequest(senderid, targetId);
      break;
    case VU_DELETE_EVENT:
      event = new VuDeleteEvent(senderid, targetId);
      break;
    case VU_UNMANAGE_EVENT:
      event = new VuUnmanageEvent(senderid, targetId);
      break;
    case VU_MANAGE_EVENT:
      event = new VuManageEvent(senderid, targetId);
      break;
    case VU_CREATE_EVENT:
      event = new VuCreateEvent(senderid, targetId);
      break;
    case VU_SESSION_EVENT:
      event = new VuSessionEvent(senderid, targetId);
      break;
    case VU_TRANSFER_EVENT:
      event = new VuTransferEvent(senderid, targetId);
      break;
    case VU_POSITION_UPDATE_EVENT:
      event = new VuPositionUpdateEvent(senderid, targetId);
      break;
    case VU_BROADCAST_GLOBAL_EVENT:
      event = new VuBroadcastGlobalEvent(senderid, targetId);
      break;
    case VU_FULL_UPDATE_EVENT:
      event = new VuFullUpdateEvent(senderid, targetId);
      break;
    case VU_ENTITY_COLLISION_EVENT:
      event = new VuEntityCollisionEvent(senderid, targetId);
      break;
    case VU_GROUND_COLLISION_EVENT:
      event = new VuGroundCollisionEvent(senderid, targetId);
      break;
    case VU_RESERVED_UPDATE_EVENT:
    case VU_UNKNOWN_MESSAGE:
    case VU_TIMER_EVENT:
    case VU_RELEASE_EVENT:
      // these are not net events... ignore
      break;
    case VU_ERROR_MESSAGE:
      event = new VuErrorMessage(senderid, targetId);
      break;

#if defined(VU_SIMPLE_LATENCY)
    case VU_TIMING_MESSAGE:
      event = new VuTimingMessage(senderid, targetId);
      break;
#endif
    }

    if (!event) {
      event = VuxCreateMessage(type, senderid, targetId);
    }
    if (event && event->Read(data, size)) {
#ifdef DEBUG_COMMS
      VU_PRINT(" (0x%x:%d): dest 0x%x, eid 0x%x",
          (short)event->Sender().creator_, event->Sender().id_,
          (short)targetId.creator_, (ulong)event->EntityId());
      if (type == VU_CREATE_EVENT || type == VU_FULL_UPDATE_EVENT) {
        VU_PRINT(", etype %d\n", ((VuCreateEvent *)event)->vutype_);
      } else {
        VU_PRINT("\n");
      }
#endif
      event->SetPostTime(timestamp);
      VuMessageQueue::PostVuMessage(event);
    }
  } 
  else {
    *data += size;
#if defined(DEBUG_COMMS)
    VU_PRINT(": dest = 0x%x -- not posted\n", (short)targetId.creator_);
#endif
  }

  return (event ? 1 : 0);
}

static int
MessagePoll(VuCommsContext* ctxt)
{
	static int		last_full_update = 0;
	int				now;
	int             count   = 0;
	int             length  = 0;
	ComAPIHandle    ch      = ctxt->handle_;
	char*           readBuf = ComAPIRecvBufferGet(ch);
	VU_BYTE*        data;
	VU_BYTE*        end;
	VuPacketHeader  phdr;
	VuMessageHeader mhdr;
	VU_ID           senderid;
	
#ifdef DEBUG
	VU_BYTE*        bufStart;
#endif

#pragma warning(disable : 4127)

	while (TRUE)
	{
		if ((length = ComAPIGet (ch)) > 0)
		{
//			VuMessage* event     = 0;
			VU_TIME    timestamp = ComAPIGetTimeStamp(ch);
			
			senderid.num_     = VU_SESSION_ENTITY_ID;
			senderid.creator_ = ComAPIQuery(ch, COMAPI_SENDER);
			
			VuEntity* sender = vuDatabase->Find(senderid);
			
			if (sender && sender->IsSession())
			{
				((VuSessionEntity*)sender)->SetKeepaliveTime(vuxRealTime);
			}
			
			data  = (VU_BYTE*)readBuf;
			end   = data + length;
			data += ReadPacketHeader(data, &phdr);
			
			while (data < end)
			{
				data += ReadMessageHeader(data, &mhdr);
	#ifdef DEBUG
				bufStart = data;
	#endif
				count += MessageReceive(phdr.targetId_, senderid, mhdr.type_, &data, mhdr.length_, timestamp);
	#ifdef DEBUG
				assert (((int)data - (int)bufStart) == mhdr.length_);
	#endif
			}
			
			readBuf = ComAPIRecvBufferGet(ch);
		}
		else if (length == -1)
		{
			count ++;
			
			break;
		}
		else if (length == -2)
		{
			// Still not synced up yet - send a full update
	//		MonoPrint ("Not Connected - sending full update\n");

			now = GetTickCount ();

			if (now - last_full_update > 2000)
			{
				VuFullUpdateEvent *msg = new VuFullUpdateEvent(vuLocalSessionEntity, vuGlobalGroup);
				msg->RequestOutOfBandTransmit ();
				VuMessageQueue::PostVuMessage(msg);

				last_full_update = now;
			}

			break;
		}
		else
		{
			break;
		}
	}

#pragma warning(default : 4127)

	return count;
}
#endif

//-----------------------------------------------------------------------------
class GlobalGroupFilter : public VuFilter {
public:
  GlobalGroupFilter();
  virtual ~GlobalGroupFilter();

  virtual VU_BOOL Test(VuEntity *ent);
  virtual VU_BOOL RemoveTest(VuEntity *ent);
  virtual int Compare(VuEntity *ent1, VuEntity *ent2);
  virtual VuFilter *Copy();

protected:
  GlobalGroupFilter(GlobalGroupFilter *other);

// DATA
protected:
  // none
};

//-----------------------------------------------------------------------------
// VuSessionsIterator
//-----------------------------------------------------------------------------
VuSessionsIterator::VuSessionsIterator(VuGroupEntity* group)
  : VuIterator(group ? group->sessionCollection_ :
          (vuLocalSessionEntity->Game() ? 
           vuLocalSessionEntity->Game()->sessionCollection_ : 0)),
    curr_(vuTailNode)
{
  vuCollectionManager->Register(this);
}

VuSessionsIterator::~VuSessionsIterator()
{
  vuCollectionManager->DeRegister(this);
}

VuEntity *
VuSessionsIterator::CurrEnt()
{
  return curr_->entity_;
}

VU_BOOL
VuSessionsIterator::IsReferenced(VuEntity* ent)
{
// 2002-02-04 MODIFIED BY S.G. If ent is false, then it can't be a valid entity, right? That's what I think too :-)
//return static_cast<VU_BOOL>((curr_->entity_ == ent) ? TRUE : FALSE);
  return static_cast<VU_BOOL>((ent && curr_->entity_ == ent) ? TRUE : FALSE);
}

VU_ERRCODE 
VuSessionsIterator::Cleanup()
{
  curr_ = vuTailNode;
  return VU_SUCCESS;
}

//-----------------------------------------------------------------------------
// VuSessionFilter
//-----------------------------------------------------------------------------

VuSessionFilter::VuSessionFilter(VU_ID groupId)
  : VuFilter(),
    groupId_(groupId)
{
  // empty
}

VuSessionFilter::VuSessionFilter(VuSessionFilter* other)
  : VuFilter(other),
    groupId_(other->groupId_)
{
  // empty
}

VuSessionFilter::~VuSessionFilter()
{
  // empty
}

VU_BOOL 
VuSessionFilter::Test(VuEntity* ent)
{
  return static_cast<VU_BOOL>((ent->IsSession() && ((VuSessionEntity*)ent)->GameId() == groupId_) ? TRUE : FALSE);
}

VU_BOOL 
VuSessionFilter::RemoveTest(VuEntity* ent)
{
  return ent->IsSession();
}

int 
VuSessionFilter::Compare(VuEntity* ent1, 
                         VuEntity* ent2)
{
  if ((VU_KEY)ent2->Id() > (VU_KEY)ent1->Id()) {
    return -1;
  } else if ((VU_KEY)ent2->Id() < (VU_KEY)ent1->Id()) {
    return 1;
  }
  return 0;
}

VU_BOOL 
VuSessionFilter::Notice(VuMessage* event)
{
  // danm_TBD: do we need VU_FULL_UPDATE event as well?
  if ((1<<event->Type()) & VU_TRANSFER_EVENT) {
    return TRUE;
  }
  return FALSE;
}

VuFilter*
VuSessionFilter::Copy()
{
  return new VuSessionFilter(this);
}

//-----------------------------------------------------------------------------
// GlobalGroupFilter
//-----------------------------------------------------------------------------

GlobalGroupFilter::GlobalGroupFilter()
  : VuFilter()
{
  // empty
}

GlobalGroupFilter::GlobalGroupFilter(GlobalGroupFilter* other)
  : VuFilter(other)
{
  // empty
}

GlobalGroupFilter::~GlobalGroupFilter()
{
  // empty
}

VU_BOOL 
GlobalGroupFilter::Test(VuEntity* ent)
{
  return ent->IsSession();
}

VU_BOOL 
GlobalGroupFilter::RemoveTest(VuEntity* ent)
{
  return ent->IsSession();
}

int 
GlobalGroupFilter::Compare(VuEntity* ent1,
                           VuEntity* ent2)
{
  if ((VU_KEY)ent2->Id() > (VU_KEY)ent1->Id()) {
    return -1;
  } else if ((VU_KEY)ent2->Id() < (VU_KEY)ent1->Id()) {
    return 1;
  }
  return 0;
}

VuFilter*
GlobalGroupFilter::Copy()
{
  return new GlobalGroupFilter(this);
}

#if defined(VU_USE_COMMS)
void
Init(VuCommsContext* ctxt)
{
  memset(ctxt, 0, sizeof(VuCommsContext));
  ctxt->status_ = VU_CONN_INACTIVE;
}

void
Cleanup(VuCommsContext* ctxt)
{
  delete [] ctxt->normalSendPacket_;
  delete [] ctxt->lowSendPacket_;
  delete [] ctxt->recBuffer_;
  Init(ctxt);   // just to be safe
}
#endif

//-----------------------------------------------------------------------------
// VuTargetEntity
//-----------------------------------------------------------------------------
VuTargetEntity::VuTargetEntity(int typeindex)
  : VuEntity(static_cast<short>(typeindex))
{
#ifdef VU_USE_COMMS
  Init(&bestEffortComms_);
  Init(&reliableComms_);
  reliableComms_.reliable_ = TRUE;
#endif VU_USE_COMMS
  dirty = 0;
}

VuTargetEntity::VuTargetEntity(VU_BYTE** stream)
  : VuEntity((ushort)VU_UNKNOWN_ENTITY_TYPE)
{
#ifdef VU_USE_COMMS
  Init(&bestEffortComms_);
  Init(&reliableComms_);
  reliableComms_.reliable_ = TRUE;
#endif VU_USE_COMMS
  memcpy(&share_.entityType_,  *stream, sizeof(share_.entityType_));  *stream += sizeof(share_.entityType_);
  memcpy(&share_.flags_,       *stream, sizeof(share_.flags_));       *stream += sizeof(share_.flags_);
  memcpy(&share_.id_.creator_, *stream, sizeof(share_.id_.creator_)); *stream += sizeof(share_.id_.creator_);
  memcpy(&share_.id_.num_,     *stream, sizeof(share_.id_.num_));     *stream += sizeof(share_.id_.num_);

  SetEntityType(share_.entityType_);
  dirty = 0;
}

VuTargetEntity::VuTargetEntity(FILE* file)
  : VuEntity((ushort)VU_UNKNOWN_ENTITY_TYPE)
{
#ifdef VU_USE_COMMS
  Init(&bestEffortComms_);
  Init(&reliableComms_);
  reliableComms_.reliable_ = TRUE;
#endif VU_USE_COMMS
  fread(&share_.entityType_,  sizeof(share_.entityType_),  1, file);
  fread(&share_.flags_,       sizeof(share_.flags_),       1, file);
  fread(&share_.id_.creator_, sizeof(share_.id_.creator_), 1, file);
  fread(&share_.id_.num_,     sizeof(share_.id_.num_),     1, file);

  SetEntityType(share_.entityType_);
  dirty = 0;
}

VuTargetEntity::~VuTargetEntity()
{
#ifdef VU_USE_COMMS
  if (vuNormalSendQueue)
    vuNormalSendQueue->RemoveTarget(this);

  if (vuLowSendQueue)
    vuLowSendQueue->RemoveTarget(this);

  Cleanup(&bestEffortComms_);
  Cleanup(&reliableComms_);
#endif VU_USE_COMMS
}

int
VuTargetEntity::LocalSize()
{
  return sizeof(share_.entityType_)
       + sizeof(share_.flags_)
       + sizeof(share_.id_.creator_)
       + sizeof(share_.id_.num_)
       ;
}

int
VuTargetEntity::SaveSize()
{
  return LocalSize();
}

int
VuTargetEntity::Save(VU_BYTE** stream)
{
  memcpy(*stream, &share_.entityType_,  sizeof(share_.entityType_));  *stream += sizeof(share_.entityType_);
  memcpy(*stream, &share_.flags_,       sizeof(share_.flags_));       *stream += sizeof(share_.flags_);
  memcpy(*stream, &share_.id_.creator_, sizeof(share_.id_.creator_)); *stream += sizeof(share_.id_.creator_);
  memcpy(*stream, &share_.id_.num_,     sizeof(share_.id_.num_));     *stream += sizeof(share_.id_.num_);

  return VuTargetEntity::LocalSize();
}

int
VuTargetEntity::Save(FILE* file)
{
  int retval = 0;

  if (file) {
    retval += fwrite(&share_.entityType_,  sizeof(share_.entityType_),  1, file);
    retval += fwrite(&share_.flags_,       sizeof(share_.flags_),       1, file);
    retval += fwrite(&share_.id_.creator_, sizeof(share_.id_.creator_), 1, file);
    retval += fwrite(&share_.id_.num_,     sizeof(share_.id_.num_),     1, file);
  }
  return retval;
}

VU_BOOL 
VuTargetEntity::IsTarget()
{
  return TRUE;
}

#ifdef VU_USE_COMMS
int
VuTargetEntity::BytesPending()
{
  int retval = 0;
/*
  if (GetCommsStatus() != VU_CONN_INACTIVE && GetCommsHandle())
  {
    retval += bestEffortComms_.normalSendPacketPtr_ - bestEffortComms_.normalSendPacket_;
    retval += bestEffortComms_.lowSendPacketPtr_    - bestEffortComms_.lowSendPacket_;
  }
*/
  if (GetReliableCommsStatus() != VU_CONN_INACTIVE && GetReliableCommsHandle())
  {
    // retval += reliableComms_.normalSendPacketPtr_ - reliableComms_.normalSendPacket_;
    // retval += reliableComms_.lowSendPacketPtr_    - reliableComms_.lowSendPacket_;
	retval += ComAPIQuery (reliableComms_.handle_, COMAPI_BYTES_PENDING);
  }
  return retval;
}

int 
VuTargetEntity::MaxPacketSize()
{
  if (GetCommsStatus() == VU_CONN_INACTIVE) {
    return 0;
  }
  if (!GetCommsHandle()) {
    VuTargetEntity *forward = ForwardingTarget();
    if (forward && forward != this) {
      return forward->MaxPacketSize();
    }
    return 0;
  }
  // return size of largest message to fit in one packet
  return bestEffortComms_.maxMsgSize_ + MIN_HDR_SIZE;
}

int 
VuTargetEntity::MaxMessageSize()
{
  if (GetCommsStatus() == VU_CONN_INACTIVE) {
    return 0;
  }
  if (!GetCommsHandle()) {
    VuTargetEntity *forward = ForwardingTarget();
    if (forward && forward != this) {
      return forward->MaxMessageSize();
    }
    return 0;
  }
  return bestEffortComms_.maxMsgSize_;
}

int 
VuTargetEntity::MaxReliablePacketSize()
{
  if (GetReliableCommsStatus() == VU_CONN_INACTIVE) {
    return 0;
  }
  if (!GetReliableCommsHandle()) {
    VuTargetEntity *forward = ForwardingTarget();
    if (forward && forward != this) {
      return forward->MaxReliablePacketSize();
    }
    return 0;
  }
  // return size of largest message to fit in one packet
  return reliableComms_.maxMsgSize_ + MIN_HDR_SIZE;
}

int 
VuTargetEntity::MaxReliableMessageSize()
{
  if (GetReliableCommsStatus() == VU_CONN_INACTIVE) {
    return 0;
  }
  if (!GetReliableCommsHandle()) {
    VuTargetEntity *forward = ForwardingTarget();
    if (forward && forward != this) {
      return forward->MaxReliableMessageSize();
    }
    return 0;
  }
  return reliableComms_.maxMsgSize_;
}

void
VuTargetEntity::SetCommsHandle(ComAPIHandle ch,
                               int          bufSize,
                               int          packSize)
{
  unsigned long chbufsize = 0; 

  if (ch)
    chbufsize = ComAPIQuery(ch, COMAPI_ACTUAL_BUFFER_SIZE);
  
  if (bufSize <= 0)
    bufSize = chbufsize - MIN_HDR_SIZE;
  
  if (bufSize > 0x7fff)
    bufSize = 0x7fff;   // max size w/ huffman encoding
  
  delete [] bestEffortComms_.normalSendPacket_;
  delete [] bestEffortComms_.lowSendPacket_;
  delete [] bestEffortComms_.recBuffer_;

  if (ch && bufSize > 0) {
    bestEffortComms_.handle_     = ch;
    bestEffortComms_.maxMsgSize_ = bufSize;
    if (packSize)
      bestEffortComms_.maxPackSize_ = packSize;
    else
      bestEffortComms_.maxPackSize_ = bufSize;
    bestEffortComms_.normalSendPacket_ = new VU_BYTE[bufSize+MIN_HDR_SIZE];
    bestEffortComms_.lowSendPacket_    = new VU_BYTE[bufSize+MIN_HDR_SIZE];
    bestEffortComms_.recBuffer_        = new VU_BYTE[bufSize];
  } 
  else {
    bestEffortComms_.handle_           = 0;
    bestEffortComms_.maxMsgSize_       = 0;
    bestEffortComms_.normalSendPacket_ = 0;
    bestEffortComms_.lowSendPacket_    = 0;
    bestEffortComms_.recBuffer_        = 0;
  }
  bestEffortComms_.normalSendPacketPtr_ = bestEffortComms_.normalSendPacket_;
  bestEffortComms_.lowSendPacketPtr_    = bestEffortComms_.lowSendPacket_;
}

void
VuTargetEntity::SetReliableCommsHandle(ComAPIHandle ch, 
                                       int          bufSize,
                                       int          packSize)
{
  unsigned long chbufsize = 0; 

  if (ch)
    chbufsize = ComAPIQuery(ch, COMAPI_ACTUAL_BUFFER_SIZE);
  
  if (bufSize <= 0)
    bufSize = chbufsize - MIN_HDR_SIZE;
  
  if (bufSize > 0x7fff)
    bufSize = 0x7fff;   // max size w/ huffman encoding
  
  delete [] reliableComms_.normalSendPacket_;
  delete [] reliableComms_.lowSendPacket_;
  delete [] reliableComms_.recBuffer_;
  
  if (ch && bufSize > 0) {
    reliableComms_.handle_           = ch;
    reliableComms_.maxMsgSize_       = bufSize;
    reliableComms_.maxPackSize_      = packSize;
    reliableComms_.normalSendPacket_ = new VU_BYTE[bufSize+MIN_HDR_SIZE];
    reliableComms_.lowSendPacket_    = new VU_BYTE[bufSize+MIN_HDR_SIZE];
    reliableComms_.recBuffer_        = new VU_BYTE[bufSize];
  } 
  else {
    reliableComms_.handle_           = 0;
    reliableComms_.maxMsgSize_       = 0;
    reliableComms_.normalSendPacket_ = 0;
    reliableComms_.lowSendPacket_    = 0;
    reliableComms_.recBuffer_        = 0;
  }
  reliableComms_.normalSendPacketPtr_ = reliableComms_.normalSendPacket_;
  reliableComms_.lowSendPacketPtr_    = reliableComms_.lowSendPacket_;
}

extern "C" void cut_bandwidth (void);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int VuTargetEntity::SendQueuedMessage (void)
{
	int
		size;

	if (reliableComms_.handle_ && reliableComms_.status_ == VU_CONN_ACTIVE)
	{
		size = ComAPISend (reliableComms_.handle_, 0);

		if (size == -2)
		{
			// Still not synced up yet - send a full update
//			MonoPrint ("Not Connected - sending full update\n");

			VuFullUpdateEvent *msg = new VuFullUpdateEvent(vuLocalSessionEntity, vuGlobalGroup);
			msg->RequestOutOfBandTransmit ();
			VuMessageQueue::PostVuMessage(msg);
		}
		else if (size > 0)
		{
			return size;
		}
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int VuTargetEntity::GetMessages()
{
	int count = 0;
	int ping = 0;

	if (reliableComms_.handle_ && reliableComms_.status_ == VU_CONN_ACTIVE)
	{
		count += MessagePoll(&reliableComms_);

		ping = ComAPIQuery (reliableComms_.handle_, COMAPI_PING_TIME);

		if (ping == -1)
		{
#ifdef DEBUG
//			MonoPrint ("Connection Error -1\n");
#endif
			reliableComms_.status_ = VU_CONN_ERROR;
			bestEffortComms_.status_ = VU_CONN_ERROR;
			return -1;
		}
		else if (ping == -2)
		{
#ifdef DEBUG
//			MonoPrint ("Connection Error -2\n");
#endif
			reliableComms_.status_ = VU_CONN_ERROR;
			bestEffortComms_.status_ = VU_CONN_ERROR;
			return -2;
		}
		else if (ping > reliableComms_.ping)
		{
		//if we are a host checking a client set that clients bw
		VuSessionEntity* session = (VuSessionEntity*)vuDatabase->Find(this->OwnerId());
		if (vuLocalSessionEntity->Game()->OwnerId() != vuLocalSessionEntity->Id() || session == vuLocalSessionEntity)
		session = NULL;//we are not host or its us self
		if (session&&((FalconSessionEntity*)vuDatabase->Find(this->Id()))->GetFlyState () !=  FLYSTATE_FLYING)
		session = NULL;//session is not in cockpit
		if (session && ((FalconSessionEntity*)vuDatabase->Find(vuLocalSessionEntity->Game()->OwnerId()))->GetFlyState () !=  FLYSTATE_FLYING)
		session = NULL;//host is not in cockpit
			
			if (static_cast<VU_TIME>(ping) > EntityType()->updateTolerance_)
			{
#ifdef DEBUG
//				MonoPrint ("Dropping connection %d\n", ping);
#endif
				reliableComms_.status_ = VU_CONN_ERROR;
				bestEffortComms_.status_ = VU_CONN_ERROR;
				return -1;
			}
		}

		else
		{
			reliableComms_.ping = ping;
		}
	}

	if (bestEffortComms_.handle_ && bestEffortComms_.status_ == VU_CONN_ACTIVE)
	{
		count += MessagePoll(&bestEffortComms_);
	}

	return count;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void VuTargetEntity::SetDirty (void)
{
//	MonoPrint ("Set Dirty\n");
	dirty = 1;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void VuTargetEntity::ClearDirty (void)
{
	dirty = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int VuTargetEntity::IsDirty (void)
{
	return dirty;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuTargetEntity* VuTargetEntity::ForwardingTarget(VuMessage *)
{
  return vuGlobalGroup;
}

int
VuTargetEntity::Flush(VuCommsContext* ctxt)
{
  VuEnterCriticalSection();

  int retval = 0;

  if (ctxt->normalSendPacketPtr_ != ctxt->normalSendPacket_) {
    int size = ctxt->normalSendPacketPtr_ - ctxt->normalSendPacket_;
    if (size > 0) {
      VU_BYTE * buffer = (VU_BYTE *)ComAPISendBufferGet(ctxt->handle_);
      memcpy(buffer, ctxt->normalSendPacket_, size);

      retval = ComAPISend(ctxt->handle_, size);
//#define debugme123
#ifdef debugme123
if (retval <=0)
{

	static  VU_TIME lastcheck  =0;
	static  VU_TIME firstcheck  =vuxRealTime;
	static int sendbuffertotal =0;
	static int messagecount =0;
	static int counter =0;
	messagecount ++;
	sendbuffertotal += size;

	if (vuxRealTime > lastcheck + 1000)
	{
		lastcheck = vuxRealTime;
		counter++;
		if ((int)((vuxRealTime-firstcheck)/1000))MonoPrint ("Flush normal Avg size %d avarage BW used pr sec %d \n",(sendbuffertotal/messagecount), (sendbuffertotal/((vuxRealTime-firstcheck)/1000)));
		if (counter > 20) 
		{
			counter =0;
			sendbuffertotal=0;
			messagecount=0;
			firstcheck  =vuxRealTime;
		}
	}
}
#endif
      if (retval > 0) {
        // message sent successfully
        ctxt->normalSendPacketPtr_ = ctxt->normalSendPacket_;
      } else if (retval != COMAPI_WOULDBLOCK) {
        // non-recoverable error
#ifdef DEBUG_SEND
        switch (retval) {
        case COMAPI_BAD_HEADER:
          VU_PRINT ("VUNRE: BAD HEADER\n");
          break;
        case COMAPI_OUT_OF_SYNC:
          VU_PRINT ("VUNRE: OUT OF SYNC\n");
          break;
        case COMAPI_OVERRUN_ERROR:
          VU_PRINT ("VUNRE: OVERRUN ERROR\n");
          break;
        case COMAPI_BAD_MESSAGE_SIZE:
          VU_PRINT ("VUNRE: BAD MESSAGE SIZE\n");
          break;
        case COMAPI_CONNECTION_CLOSED:
          VU_PRINT ("VUNRE: CONNECTION CLOSED\n");
          break;
        case COMAPI_MESSAGE_TOO_BIG:
          VU_PRINT ("VUNRE: MESSAGE TOO BIG\n");
          break;
        case COMAPI_CONNECTION_PENDING:
          VU_PRINT ("VUNRE: CONNECTION PENDING\n");
          break;
        case COMAPI_EMPTYGROUP:
          VU_PRINT ("VUNRE: EMPTY GROUP\n");
          break;
        case COMAPI_PLAYER_LEFT:
          VU_PRINT ("VUNRE: PLAYER LEFT\n");
          break;
        case COMAPI_NOTAGROUP:
          VU_PRINT ("VUNRE: NOT A GROUP\n");
          break;
        case COMAPI_CONNECTION_TIMEOUT:
          VU_PRINT ("VUNRE: CONNECTION TIMEOUT\n");
          break;
        case COMAPI_HOSTID_ERROR:
          VU_PRINT ("VUNRE: HOST ID ERROR\n");
          break;
        case COMAPI_WINSOCKDLL_ERROR:
          VU_PRINT ("VUNRE: WINSOCKDLL ERROR\n");
          break;
        case COMAPI_DPLAYDLL_ERROR:
          VU_PRINT ("VUNRE: DPLAYDLL ERROR\n");
        break;
        case COMAPI_OLE32DLL_ERROR:
          VU_PRINT ("VUNRE: OLE32DLL ERROR\n");
          break;
        case COMAPI_TENDLL_ERROR:
          VU_PRINT ("VUNRE: TENDLL ERROR\n");
          break;
        }
#endif
        ctxt->normalSendPacketPtr_ = ctxt->normalSendPacket_;
      }
	  else
	  {
//		  MonoPrint ("Would Block\n");
	  }
    }
  }

  VuExitCriticalSection();
  return retval;
}

int
VuTargetEntity::FlushLow(VuCommsContext* ctxt)
{
  VuEnterCriticalSection();

  int retval = 0;

  if (ctxt->lowSendPacketPtr_ != ctxt->lowSendPacket_) {
    int size = ctxt->lowSendPacketPtr_ - ctxt->lowSendPacket_;

    if (size > 0) {
      VU_BYTE*  buffer = (VU_BYTE *)ComAPISendBufferGet(ctxt->handle_);
      memcpy(buffer, ctxt->lowSendPacket_, size);

      retval = ComAPISend(ctxt->handle_, size);
//#define debugme123
#ifdef debugme123
if (retval <=0)
{

	static  VU_TIME lastcheck  =0;
	static int sendbuffertotal =0;
	static int messagecount =0;
	static int counter =0;
	messagecount ++;
	sendbuffertotal += size;

	if (vuxRealTime > lastcheck + 1000)
	{
		counter++;
		MonoPrint ("Flush Low Avg size %d avarage BW used pr sec %d \n",(sendbuffertotal/messagecount), (sendbuffertotal/counter));
		if (counter > 20) 
		{
			counter =0;
			sendbuffertotal=0;
			messagecount=0;
		}
	}
}
#endif
      if (retval > 0) {
        // message sent successfully
        ctxt->lowSendPacketPtr_ = ctxt->lowSendPacket_;
      } 
      else if (retval != COMAPI_WOULDBLOCK) {
        // non-recoverable error
#ifdef DEBUG_SEND
        switch (retval) {
        case COMAPI_BAD_HEADER:
          VU_PRINT ("VUNRE: BAD HEADER\n");
          break;
        case COMAPI_OUT_OF_SYNC:
          VU_PRINT ("VUNRE: OUT OF SYNC\n");
          break;
        case COMAPI_OVERRUN_ERROR:
          VU_PRINT ("VUNRE: OVERRUN ERROR\n");
          break;
        case COMAPI_BAD_MESSAGE_SIZE:
          VU_PRINT ("VUNRE: BAD MESSAGE SIZE\n");
          break;
        case COMAPI_CONNECTION_CLOSED:
          VU_PRINT ("VUNRE: CONNECTION CLOSED\n");
          break;
        case COMAPI_MESSAGE_TOO_BIG:
          VU_PRINT ("VUNRE: MESSAGE TOO BIG\n");
          break;
        case COMAPI_CONNECTION_PENDING:
          VU_PRINT ("VUNRE: CONNECTION PENDING\n");
          break;
        case COMAPI_EMPTYGROUP:
          VU_PRINT ("VUNRE: EMPTY GROUP\n");
          break;
        case COMAPI_PLAYER_LEFT:
          VU_PRINT ("VUNRE: PLAYER LEFT\n");
          break;
        case COMAPI_NOTAGROUP:
          VU_PRINT ("VUNRE: NOT A GROUP\n");
          break;
        case COMAPI_CONNECTION_TIMEOUT:
          VU_PRINT ("VUNRE: CONNECTION TIMEOUT\n");
          break;
        case COMAPI_HOSTID_ERROR:
          VU_PRINT ("VUNRE: HOST ID ERROR\n");
          break;
        case COMAPI_WINSOCKDLL_ERROR:
          VU_PRINT ("VUNRE: WINSOCKDLL ERROR\n");
          break;
        case COMAPI_DPLAYDLL_ERROR:
          VU_PRINT ("VUNRE: DPLAYDLL ERROR\n");
          break;
        case COMAPI_OLE32DLL_ERROR:
          VU_PRINT ("VUNRE: OLE32DLL ERROR\n");
          break;
        case COMAPI_TENDLL_ERROR:
          VU_PRINT ("VUNRE: TENDLL ERROR\n");
          break;
        }
#endif
        ctxt->lowSendPacketPtr_ = ctxt->lowSendPacket_;
      }
    }
  }
  VuExitCriticalSection();
  return retval;
}

// Return values:
//  >  0 --> flush succeeded (all contexts sent data)
//  == 0 --> no data was pending;
//   < 0 --> flush failed (likely send buffer full)
int VuTargetEntity::FlushOutboundMessageBuffer()
{
	VuEnterCriticalSection();
	
	int retval = -1;
	
	if (GetReliableCommsStatus() == VU_CONN_ACTIVE && GetReliableCommsHandle()) {
		retval = Flush(&reliableComms_);
	}
	
	if (GetCommsStatus() == VU_CONN_ACTIVE && GetCommsHandle()) {
		int retval2 = Flush(&bestEffortComms_);
		if (retval2 > 0) {
			retval += retval2;
		}
	}
	
	if (retval > 0) {
		VuMainThread::ReportXmit(retval);
	}
	
	VuExitCriticalSection();
	return retval;
}

int
VuTargetEntity::SendOutOfBand(VuCommsContext* ctxt, 
                              VuMessage*      msg)
{
  int retval = 0;
  
  VuPacketHeader phdr(msg->Destination());
  
  VU_BYTE* start_buffer = (VU_BYTE *)ComAPISendBufferGet(ctxt->handle_);
  VU_BYTE* buffer       = start_buffer;

  buffer += WritePacketHeader(buffer, &phdr);

  int size = msg->Size();
  VuMessageHeader mhdr(msg->Type(), static_cast<short>(size));

  buffer += WriteMessageHeader(buffer, &mhdr);

#ifdef DEBUG_SEND
  VU_BYTE * oldptr = buffer;
#endif
  msg->Write(&buffer);
#ifdef DEBUG_SEND

  if (buffer - oldptr != size) {
    VU_PRINT("::SendOutOfBand(%d) ERROR -- size = %d, bufchange = %d\n", msg->Type(), size, buffer - oldptr);
    assert("::SendOutOfBand() ERROR" == 0);
  }
#endif

  if (size > 0)
  {
	  if (msg->type_ == VU_POSITION_UPDATE_EVENT)//me123 it's a possition update
	  retval = ComAPISendOOB(ctxt->handle_, buffer - start_buffer, true);
	  else
	  retval = ComAPISendOOB(ctxt->handle_, buffer - start_buffer, false);
	  static lastcheck = 0;
	  static lastStatcheck = 0;
	  static int BWprSec = 0;
	  BWprSec += buffer - start_buffer;
//#define DEBUG_
#ifdef DEBUG_
	  MonoPrint("OOB req send type %d, size %d, timedelta %d",msg->type_ , buffer - start_buffer, GetTickCount () - lastcheck );
	  lastcheck = GetTickCount ();
	  if ((GetTickCount()-lastStatcheck)>1000)
	  {
		 
		  MonoPrint("OOB req stat %d Prsec",BWprSec*1000/(GetTickCount()-lastStatcheck));
		  lastStatcheck = GetTickCount ();
		  BWprSec = 0;
	  }
#endif
  }
  
  if (retval > 0)
    VuMainThread::ReportXmit(retval);
  
  return retval;
}

int
VuTargetEntity::SendNormalPriority(VuCommsContext* ctxt,
                                   VuMessage*      msg)
{
	int retval = -3;
	VuEnterCriticalSection();
	int totsize,size = msg->Size();         
	
	totsize = size + PACKET_HDR_SIZE + MAX_MSG_HDR_SIZE;
	
	if (totsize > ctxt->maxMsgSize_)
	{ // write would exceed buffer
#ifdef DEBUG_SEND
		assert(0);
#endif
		retval = COMAPI_MESSAGE_TOO_BIG;
	} 
	else if (ctxt->normalSendPacketPtr_ != ctxt->normalSendPacket_ && // data is waiting in queue
		(totsize + ctxt->normalSendPacketPtr_ - ctxt->normalSendPacket_ > ctxt->maxPackSize_ || // write would exceed packet size
		(ctxt->normalSendPacketPtr_ != ctxt->normalSendPacket_ && // different origins
		(msg->Sender().creator_ != ctxt->normalPendingSenderId_.creator_ ||
		msg->Destination() != ctxt->normalPendingSendTargetId_))))
	{
		if (Flush(ctxt) > 0 && ctxt->normalSendPacketPtr_ == ctxt->normalSendPacket_)
		{
			// try resending...
			retval = SendNormalPriority(ctxt, msg);
		}
		else
		{
			// cannot send, return error
			// retval = COMAPI_WOULDBLOCK;
		}
	} 
	else
	{      // write fits within buffer -- add to/begin packet
		if (ctxt->normalSendPacketPtr_ == ctxt->normalSendPacket_)
		{ // begin a new packet
			VuPacketHeader phdr(msg->Destination());
			ctxt->normalSendPacketPtr_ += WritePacketHeader(ctxt->normalSendPacketPtr_, &phdr);
		}
		
		VuMessageHeader mhdr(msg->Type(), static_cast<short>(size));
		ctxt->normalSendPacketPtr_           += WriteMessageHeader(ctxt->normalSendPacketPtr_, &mhdr);
		ctxt->normalPendingSenderId_.creator_ = msg->Sender().creator_;
		ctxt->normalPendingSenderId_.num_     = msg->Sender().num_;
		ctxt->normalPendingSendTargetId_      = msg->Destination();
		// return number of bytes written to buffer
		VU_BYTE* ptr    = ctxt->normalSendPacketPtr_;

#ifdef DEBUG_SEND

		VU_BYTE* oldptr = ptr;
#endif
		
		retval = msg->Write(&ptr);

#ifdef DEBUG_SEND
		if (ptr - oldptr != msg->Size())
		{
			VU_PRINT("::SendNormalPriority(%d) (pkt) ERROR -- size = %d, bufchange = %d\n", msg->Type(), msg->Size(), ptr - oldptr);
			assert("::SendNormalPriority(%d) (pkt) ERROR" == 0);
		}
#endif
		ctxt->normalSendPacketPtr_ = ptr;
		// update msg count
		//*ctxt->sendPacket_ += 1;
	}
	VuExitCriticalSection();
	return retval;
}

int 
VuTargetEntity::SendLowPriority (VuCommsContext* ctxt,
                                 VuMessage*      msg)
{
  int retval  = -3;
  int totsize;
  int size;

  VuEnterCriticalSection();
  
  size    = msg->Size();         
  totsize = size + PACKET_HDR_SIZE + MAX_MSG_HDR_SIZE;
  
  if (totsize > ctxt->maxMsgSize_) { // write would exceed buffer
    assert(0);
    retval = COMAPI_MESSAGE_TOO_BIG;
  }
  else if (ctxt->lowSendPacketPtr_ != ctxt->lowSendPacket_ && // data is waiting in queue
           (totsize + ctxt->lowSendPacketPtr_ - ctxt->lowSendPacket_ > ctxt->maxPackSize_ || // write would exceed packet size
            (
             ctxt->lowSendPacketPtr_ != ctxt->lowSendPacket_ && // different origins
             (
              msg->Sender().creator_ != ctxt->lowPendingSenderId_.creator_ ||
              msg->Destination() != ctxt->lowPendingSendTargetId_
              )
             )
            )
           )
    {
      if (FlushLow(ctxt) > 0 && ctxt->lowSendPacketPtr_ == ctxt->lowSendPacket_) {
        // try resending...
        retval = SendLowPriority(ctxt, msg);
      }
      else {
        // cannot send, return error
        retval = COMAPI_WOULDBLOCK;
      }
    }
  else {      // write fits within buffer -- add to/begin packet
    if (ctxt->lowSendPacketPtr_ == ctxt->lowSendPacket_) { // begin a new packet
      VuPacketHeader
        phdr (msg->Destination());
      
      ctxt->lowSendPacketPtr_ += WritePacketHeader(ctxt->lowSendPacketPtr_, &phdr);
    }
    
    VuMessageHeader mhdr (msg->Type(), static_cast<short>(size));
    
    ctxt->lowSendPacketPtr_           += WriteMessageHeader(ctxt->lowSendPacketPtr_, &mhdr);
    ctxt->lowPendingSenderId_.creator_ = msg->Sender().creator_;
    ctxt->lowPendingSenderId_.num_     = msg->Sender().num_;
    ctxt->lowPendingSendTargetId_      = msg->Destination();
    
    // return number of bytes written to buffer
    VU_BYTE* ptr    = ctxt->lowSendPacketPtr_;

#ifdef DEBUG_SEND

    VU_BYTE* oldptr = ptr;

#endif
    
    retval = msg->Write(&ptr);
    
#ifdef DEBUG_SEND
    if (ptr - oldptr != msg->Size()) {
      VU_PRINT("::SendLowPriority(%d) (pkt) ERROR -- size = %d, bufchange = %d\n", msg->Type(), msg->Size(), ptr - oldptr);
      assert("::SendLowPriority(%d) (pkt) ERROR" == 0);
    }
#endif
    
    ctxt->lowSendPacketPtr_ = ptr;
  }
  
  VuExitCriticalSection();
  
  return retval;
}

int
VuTargetEntity::SendMessage(VuMessage* msg)
{
  int retval = 0;

  // find the comms handle to send message
  VuCommsContext* ctxt = 0;
  if (msg->Flags() & VU_RELIABLE_MSG_FLAG) {
//  if (GetReliableCommsStatus() == VU_CONN_INACTIVE) {
//    return 0;
//  }
    if (GetReliableCommsHandle()) {
      ctxt = &reliableComms_;
    }
  } else {
//  if (GetCommsStatus() == VU_CONN_INACTIVE) {
//    return 0;
//  }
    if (GetCommsHandle()) {
      ctxt = &bestEffortComms_;
    }
  }
  if (!ctxt) {
    VuTargetEntity *forward = ForwardingTarget(msg);
    if (forward && forward != this) {
      return forward->SendMessage(msg);
    }
    // cannot send 
    return 0;
  }
    // set the message id...
    // note: this counts events which are overwritten by Read()
    // note2: don't reassign id's for resubmitted messages (delay)
    // note3: 5/27/98 -- probably not needed anymore
#ifdef DEBUG_COMMS
  VU_PRINT("VU:Send [%d] (0x%x:%d):",
      msg->Type(), (short)msg->msgid_.creator_, msg->msgid_.id_);
  VU_PRINT(" dest 0x%x%s, eid 0x%x, etype %d\n",
      (short)msg->Destination().creator_, ctxt->reliable_ ? " (r)" : "",
      (int)msg->entityId_, (msg->ent_ ? msg->ent_->Type() : 0));
#endif
  if (msg->Flags() & VU_OUT_OF_BAND_MSG_FLAG) {
    retval = SendOutOfBand(ctxt, msg);
  } else if (msg->Flags() & VU_NORMAL_PRIORITY_MSG_FLAG) {
    retval = SendNormalPriority(ctxt, msg);
  } else {
    retval = SendLowPriority(ctxt, msg);
  }
  
  if ((msg->Flags() & VU_RELIABLE_MSG_FLAG) && 
      (retval == COMAPI_WOULDBLOCK || retval == COMAPI_CONNECTION_PENDING)) {
    // recoverable error
#ifdef DEBUG_SEND
    // receiver not ready for message... try again in one sec.
    VU_PRINT("VU: re-posting reliable message in a sec (send not ready)\n");
#endif
    VuMessageQueue::RepostMessage(msg, 100);	// 100 ms delay
    return 0;
  }
#ifdef DEBUG_SEND
  if (retval <= 0) {
    VU_PRINT("VU: ComAPISend() failed with %d\n", retval);
  }
#endif // DEBUG_COMMS
  return retval;
}
#endif

//-----------------------------------------------------------------------------
// VuSessionEntity
//-----------------------------------------------------------------------------
VuSessionEntity::VuSessionEntity(int typeindex, ulong domainMask,
            char *callsign)
  : VuTargetEntity(typeindex),
    sessionId_(VU_SESSION_NULL_CONNECTION), // assigned on session open
    domainMask_(domainMask),
    callsign_(0),
    loadMetric_(1),
    gameId_(VU_SESSION_NULL_GROUP), // assigned on session open
    groupCount_(0),
    groupHead_(0),
	bandwidth_(33600),
#ifdef VU_SIMPLE_LATENCY
  timeDelta_(0),
  latency_(0),
#endif VU_SIMPLE_LATENCY
#ifdef VU_TRACK_LATENCY
    timeSyncState_(VU_NO_SYNC),
    latency_(0),
    masterTime_(0),
    masterTimePostTime_(0),
    responseTime_(0),
    masterTimeOwner_(0),
    lagTotal_(0),
    lagPackets_(0),
    lagUpdate_(LAG_COUNT_START),
#endif VU_TRACK_LATENCY
    lastMsgRecvd_(0),
    game_(0),
    action_(VU_NO_GAME_ACTION)
{
#if VU_MAX_SESSION_CAMERAS > 0
  cameraCount_ = 0;
  for (int j = 0; j < VU_MAX_SESSION_CAMERAS; j++) {
    cameras_[j] = vuNullId;
  }
#endif

  share_.id_.creator_ = sessionId_;
  share_.id_.num_     = VU_SESSION_ENTITY_ID;
  share_.ownerId_     = share_.id_;   // need to reset this
  SetCallsign(callsign);
  SetKeepaliveTime(vuxRealTime);
}

VuSessionEntity::VuSessionEntity(ulong domainMask,
                                 char* callsign)
  : VuTargetEntity(VU_SESSION_ENTITY_TYPE),
    sessionId_(VU_SESSION_NULL_CONNECTION), // assigned on session open
    domainMask_(domainMask),
    callsign_(0),
    loadMetric_(1),
    gameId_(VU_SESSION_NULL_GROUP), // assigned on session open
    groupCount_(0),
    groupHead_(0),
	bandwidth_(33600),//me123
#ifdef VU_SIMPLE_LATENCY
  timeDelta_(0),
  latency_(0),
#endif VU_SIMPLE_LATENCY
#ifdef VU_TRACK_LATENCY
    timeSyncState_(VU_NO_SYNC),
    latency_(0),
    masterTime_(0),
    masterTimePostTime_(0),
    responseTime_(0),
    masterTimeOwner_(0),
    lagTotal_(0),
    lagPackets_(0),
    lagUpdate_(LAG_COUNT_START),
#endif VU_TRACK_LATENCY
    lastMsgRecvd_(0),
    game_(0),
    action_(VU_NO_GAME_ACTION)
{
#if VU_MAX_SESSION_CAMERAS > 0
  cameraCount_ = 0;
  for (int j = 0; j < VU_MAX_SESSION_CAMERAS; j++) {
    cameras_[j] = vuNullId;
  }
#endif

  share_.id_.creator_ = sessionId_;
  share_.id_.num_     = VU_SESSION_ENTITY_ID;
  share_.ownerId_     = share_.id_;   // need to reset this
  SetCallsign(callsign);
  SetKeepaliveTime(vuxRealTime);
}

VuSessionEntity::VuSessionEntity(VU_BYTE** stream)
  : VuTargetEntity(stream),
    sessionId_(0),
    gameId_(0, 0),
    groupCount_(0),
    groupHead_(0),
	bandwidth_(33600),//me123
#ifdef VU_SIMPLE_LATENCY
    timeDelta_(0),
    latency_(0),
#endif VU_SIMPLE_LATENCY
#ifdef VU_TRACK_LATENCY
    latency_(0),
    masterTime_(0),
    masterTimePostTime_(0),
    responseTime_(0),
    masterTimeOwner_(0),
    lagTotal_(0),
    lagPackets_(0),
    lagUpdate_(LAG_COUNT_START),
#endif VU_TRACK_LATENCY
    lastMsgRecvd_(0),
    game_(0),
    action_(VU_NO_GAME_ACTION)
{
  VU_BYTE len;

  share_.ownerId_ = Id();

  memcpy(&lastCollisionCheckTime_,  *stream, sizeof(VU_TIME));       *stream += sizeof(VU_TIME);
  memcpy(&sessionId_,               *stream, sizeof(VU_SESSION_ID)); *stream += sizeof(VU_SESSION_ID);
  memcpy(&domainMask_,              *stream, sizeof(ulong));         *stream += sizeof(ulong);
  memcpy(&loadMetric_,              *stream, sizeof(VU_BYTE));       *stream += sizeof(VU_BYTE);
  memcpy(&gameId_,                  *stream, sizeof(VU_ID));         *stream += sizeof(VU_ID);
  memcpy(&len,                      *stream, sizeof(len));           *stream += sizeof(len);

  VU_ID id;

  for (int i = 0; i < groupCount_; i++) {
    memcpy(&id, *stream, sizeof(id)); *stream += sizeof(id);
    AddGroup(id);
  }
  memcpy(&bandwidth_,          *stream, sizeof(bandwidth_));          *stream += sizeof(bandwidth_);//me123 

#ifdef VU_SIMPLE_LATENCY
  VuSessionEntity *session = (VuSessionEntity*)vuDatabase->Find(Id());
  VU_TIME     rt,gt;
  memcpy(&rt, *stream, sizeof(VU_TIME));  *stream += sizeof(VU_TIME);
  memcpy(&gt, *stream, sizeof(VU_TIME));  *stream += sizeof(VU_TIME);

  if (session) {
    // Respond to this session with a timing message so it can determine latency
    VuTimingMessage* msg = new VuTimingMessage(vuLocalSession,(VuTargetEntity*)session);
    msg->sessionRealSendTime_ = rt;
    msg->sessionGameSendTime_ = gt;
    msg->remoteGameTime_      = vuxGameTime;
    VuMessageQueue::PostVuMessage(msg);
  }


#endif VU_SIMPLE_LATENCY
#ifdef VU_TRACK_LATENCY
  memcpy(&timeSyncState_,      *stream, sizeof(timeSyncState_));      *stream += sizeof(timeSyncState_);
  memcpy(&latency_,            *stream, sizeof(latency_));            *stream += sizeof(latency_);
  memcpy(&masterTime_,         *stream, sizeof(masterTime_));         *stream += sizeof(masterTime_);
  memcpy(&masterTimePostTime_, *stream, sizeof(masterTimePostTime_)); *stream += sizeof(masterTimePostTime_);
  memcpy(&responseTime_,       *stream, sizeof(responseTime_));       *stream += sizeof(responseTime_);
  memcpy(&masterTimeOwner_,    *stream, sizeof(masterTimeOwner_));    *stream += sizeof(masterTimeOwner_);
#endif VU_TRACK_LATENCY
  len = (VU_BYTE)**stream;      *stream += sizeof(VU_BYTE);
  callsign_ = new char[len + 1];
  memcpy(callsign_, *stream, len);      *stream += len;
  callsign_[len] = '\0';  // null terminate
#if VU_MAX_SESSION_CAMERAS > 0
  memcpy(&cameraCount_, *stream, sizeof(cameraCount_)); *stream += sizeof(cameraCount_);
  if (cameraCount_ > 0 && cameraCount_ <= VU_MAX_SESSION_CAMERAS) {
    memcpy(&cameras_, *stream, sizeof(VU_ID) * cameraCount_); *stream += sizeof(VU_ID) * cameraCount_;
  }
#else
  int ccnt;
  memcpy(&ccnt, *stream, sizeof(ccnt)); *stream += sizeof(ccnt);
  if (ccnt > 0) {
    *stream += sizeof(VU_ID) * ccnt;
  }
#endif
}

VuSessionEntity::VuSessionEntity(FILE* file)
  : VuTargetEntity(file),
    sessionId_(0),
    gameId_(0, 0),
    groupCount_(0),
    groupHead_(0),
	bandwidth_(33600),//me123
#ifdef VU_TRACK_LATENCY
    latency_(0),
    masterTime_(0),
    masterTimePostTime_(0),
    responseTime_(0),
    masterTimeOwner_(0),
    lagTotal_(0),
    lagPackets_(0),
    lagUpdate_(LAG_COUNT_START),
#endif VU_TRACK_LATENCY
    lastMsgRecvd_(0),
    game_(0),
    action_(VU_NO_GAME_ACTION)
{
  VU_BYTE len = 0;
  fread(&lastCollisionCheckTime_, sizeof(VU_TIME),       1, file);
  fread(&sessionId_,              sizeof(VU_SESSION_ID), 1, file);
  fread(&domainMask_,             sizeof(ulong),         1, file);
  fread(&loadMetric_,             sizeof(VU_BYTE),       1, file);
  fread(&gameId_,                 sizeof(VU_ID),         1, file);
  fread(&len,                     sizeof(len),           1, file);

  VU_ID id;

  for (int i = 0; i < groupCount_; i++) {
    fread(&id, sizeof(id), 1, file);
    AddGroup(id);
  }
  fread (&bandwidth_,		  sizeof(bandwidth_),		   1, file);//me123
#ifdef VU_TRACK_LATENCY
  fread(&timeSyncState_,      sizeof(timeSyncState_),      1, file);
  fread(&latency_,            sizeof(latency_),            1, file);
  fread(&masterTime_,         sizeof(masterTime_),         1, file);
  fread(&masterTimePostTime_, sizeof(masterTimePostTime_), 1, file);
  fread(&responseTime_,       sizeof(responseTime_),       1, file);
  fread(&masterTimeOwner_,    sizeof(masterTimeOwner_),    1, file);
#endif

  fread(&len,     sizeof(VU_BYTE), 1, file);
  callsign_ = new char[len + 1];
  fread(callsign_,    len, 1, file);
  callsign_[len] = '\0';  // null terminate

#if VU_MAX_SESSION_CAMERAS > 0
  fread(&cameraCount_, sizeof(cameraCount_), 1, file);

  if (cameraCount_ > 0 && cameraCount_ <= VU_MAX_SESSION_CAMERAS)
    fread(cameras_, sizeof(VU_ID), cameraCount_, file);
#else
  char buf[1024];
  int ccnt;
  fread(&ccnt, sizeof(ccnt), 1, file);

  if (ccnt > 0)
    fread(buf, sizeof(VU_ID), ccnt, file);
#endif
}

VuSessionEntity::~VuSessionEntity()
{
  PurgeGroups();
  delete [] callsign_;
}

int
VuSessionEntity::LocalSize()
{
  return sizeof(lastCollisionCheckTime_)
    + sizeof(sessionId_)
    + sizeof(domainMask_)
    + sizeof(loadMetric_)
    + sizeof(gameId_)
    + sizeof(groupCount_)
    + (groupCount_ * sizeof(VU_ID))
	+ sizeof(bandwidth_)//me123
#ifdef VU_SIMPLE_LATENCY
    + sizeof(VU_TIME)
    + sizeof(VU_TIME)
#endif

#ifdef VU_TRACK_LATENCY
    + sizeof(timeSyncState_)
    + sizeof(latency_)    
    + sizeof(masterTime_)
    + sizeof(masterTimePostTime_)
    + sizeof(responseTime_)
    + sizeof(masterTimeOwner_)
#endif
    + strlen(callsign_)+1
#if VU_MAX_SESSION_CAMERAS > 0
    + sizeof(cameraCount_)
    + (cameraCount_ * sizeof(VU_ID))
#else
    + sizeof(int) // bogus camera count insert
#endif
    ;
}

int
VuSessionEntity::SaveSize()
{
  return VuTargetEntity::SaveSize() + VuSessionEntity::LocalSize();
}

int
VuSessionEntity::Save(VU_BYTE** stream)
{
  int     retval = 0;
  VU_BYTE len    = static_cast<VU_BYTE>(strlen(callsign_));

  SetKeepaliveTime(vuxRealTime);
  retval = VuTargetEntity::Save(stream);

  memcpy(*stream, &lastCollisionCheckTime_, sizeof(VU_TIME));       *stream += sizeof(VU_TIME);
  memcpy(*stream, &sessionId_,              sizeof(VU_SESSION_ID)); *stream += sizeof(VU_SESSION_ID);
  memcpy(*stream, &domainMask_,             sizeof(ulong));         *stream += sizeof(ulong);
  memcpy(*stream, &loadMetric_,             sizeof(VU_BYTE));       *stream += sizeof(VU_BYTE);
  memcpy(*stream, &gameId_,                 sizeof(VU_ID));         *stream += sizeof(VU_ID);
  memcpy(*stream, &groupCount_,             sizeof(groupCount_));   *stream += sizeof(groupCount_);

  VuGroupNode* gnode = groupHead_;

  while (gnode) {
    memcpy(*stream, &gnode->gid_, sizeof(gnode->gid_));  *stream += sizeof(gnode->gid_);
    gnode = gnode->next_;
  }
  memcpy(*stream, &bandwidth_,    sizeof(bandwidth_));   *stream += sizeof(bandwidth_);//me123

#ifdef VU_SIMPLE_LATENCY
  memcpy(*stream, &vuxRealTime, sizeof(VU_TIME)); *stream += sizeof(VU_TIME);
  memcpy(*stream, &vuxGameTime, sizeof(VU_TIME)); *stream += sizeof(VU_TIME);
#endif

#ifdef VU_TRACK_LATENCY
  if (TimeSyncState() == VU_MASTER_SYNC) {
    masterTime_ = vuxRealTime;
  }
  responseTime_ = vuxRealTime;
  memcpy(*stream, &timeSyncState_,      sizeof(timeSyncState_));      *stream += sizeof(timeSyncState_);
  memcpy(*stream, &latency_,            sizeof(latency_));            *stream += sizeof(latency_);
  memcpy(*stream, &masterTime_,         sizeof(masterTime_));         *stream += sizeof(masterTime_);
  memcpy(*stream, &masterTimePostTime_, sizeof(masterTimePostTime_)); *stream += sizeof(masterTimePostTime_);
  memcpy(*stream, &responseTime_,       sizeof(responseTime_));       *stream += sizeof(responseTime_);
  memcpy(*stream, &masterTimeOwner_,    sizeof(masterTimeOwner_));    *stream += sizeof(masterTimeOwner_);
#endif

  **stream = len;         *stream += sizeof(VU_BYTE);
  memcpy(*stream, callsign_, len);      *stream += len;

#if VU_MAX_SESSION_CAMERAS > 0
  memcpy(*stream, &cameraCount_, sizeof(cameraCount_)); *stream += sizeof(cameraCount_);
  if (cameraCount_ > 0 && cameraCount_ <= VU_MAX_SESSION_CAMERAS) {
    memcpy(*stream, &cameras_, sizeof(VU_ID) * cameraCount_); *stream += sizeof(VU_ID) * cameraCount_;
  }
#else
  int ccnt = 0;
  memcpy(*stream, &ccnt, sizeof(ccnt)); *stream += sizeof(ccnt);
#endif

  retval += VuSessionEntity::LocalSize();
  return retval;
}

int
VuSessionEntity::Save(FILE* file)
{
  int     retval = 0;
  VU_BYTE len    = 0;

  if (file) {
    SetKeepaliveTime(vuxRealTime);

    retval  = VuTargetEntity::Save(file);
    retval += fwrite(&lastCollisionCheckTime_, sizeof(VU_TIME),       1, file);
    retval += fwrite(&sessionId_,              sizeof(VU_SESSION_ID), 1, file);
    retval += fwrite(&domainMask_,             sizeof(ulong),         1, file);
    retval += fwrite(&loadMetric_,             sizeof(VU_BYTE),       1, file);
    retval += fwrite(&gameId_,                 sizeof(VU_ID),         1, file);
    retval += fwrite(&groupCount_,             sizeof(groupCount_),   1, file);

    VuGroupNode* gnode = groupHead_;

    while (gnode) {
      retval += fwrite(&gnode->gid_, sizeof(gnode->gid_), 1, file);
      gnode = gnode->next_;
    }
    retval += fwrite(&bandwidth_,          sizeof(bandwidth_),            1, file);//me123
#ifdef VU_TRACK_LATENCY
    responseTime_ = vuxRealTime;
    retval += fwrite(&timeSyncState_,      sizeof(timeSyncState_),      1, file);
    retval += fwrite(&latency_,            sizeof(latency_),            1, file);
    retval += fwrite(&masterTime_,         sizeof(masterTime_),         1, file);
    retval += fwrite(&masterTimePostTime_, sizeof(masterTimePostTime_), 1, file);
    retval += fwrite(&responseTime_,       sizeof(responseTime_),       1, file);
    retval += fwrite(&masterTimeOwner_,    sizeof(masterTimeOwner_),    1, file);
    len     = strlen(callsign_);
#endif
    retval += fwrite(&len,                 sizeof(VU_BYTE),             1, file);
    retval += fwrite(callsign_,            len,                         1, file);

#if VU_MAX_SESSION_CAMERAS > 0
    retval += fwrite(&cameraCount_, sizeof(cameraCount_), 1, file);

    if (cameraCount_ > 0)
      retval += fwrite(cameras_, sizeof(VU_ID), cameraCount_, file);
#else
    int ccnt = 0;
    retval += fwrite(&ccnt, sizeof(ccnt), 1, file);
#endif
  }

  return retval;
}

VU_BOOL 
VuSessionEntity::IsSession()
{
  return TRUE;
}

VU_BOOL 
VuSessionEntity::HasTarget(VU_ID id)
{
  if (id == Id()) {
    return TRUE;
  }
  return FALSE;
}

VU_BOOL 
VuSessionEntity::InTarget(VU_ID id)
{
  if (id == Id() || id == GameId() || id == vuGlobalGroup->Id()) {
    return TRUE;
  }
  VuEnterCriticalSection();
  VuGroupNode* gnode = groupHead_;
  while (gnode) {
    if (gnode->gid_ == id) {
      VuExitCriticalSection();
      return TRUE;
    }
    gnode = gnode->next_;
  }
  VuExitCriticalSection();
  return FALSE;
}

#if VU_MAX_SESSION_CAMERAS > 0
VuEntity*
VuSessionEntity::Camera(int index)
{
  VuEntity *retval = 0;
  if (index < cameraCount_) {
    retval = vuDatabase->Find(cameras_[index]);
  }
  return retval;
}

int 
VuSessionEntity::AttachCamera(VU_ID camera)
{
  int retval = -1;
  if (cameraCount_ < VU_MAX_SESSION_CAMERAS) {
    cameras_[cameraCount_] = camera;
    cameraCount_++;
    retval = 1;
    SetDirty ();
  }
  return retval;
}

int 
VuSessionEntity::RemoveCamera(VU_ID camera)
{
  int retval = -1;
  for (int i = 0; i < cameraCount_; i++) {
    // leonr - added another = to the comaprison
    if (camera == cameras_[i]) {
      cameraCount_--;
      for (; i < cameraCount_; i++) {
        cameras_[i] = cameras_[i+1];
      }
      // KCK: this cleared the game_ field if it was our last camera - fixed 7/27/98
      cameras_[cameraCount_] = vuNullId;
      retval = 1;
      SetDirty ();
    }
  }
  return retval;
}

void
VuSessionEntity::ClearCameras(void)
{
  for (int i = 0; i < cameraCount_; i++) {
    cameras_[i] = vuNullId;
  }
  cameraCount_ = 0;
  SetDirty ();
}
#endif

VU_ERRCODE
VuSessionEntity::AddGroup(VU_ID gid)
{
  VU_ERRCODE retval = VU_ERROR;
  if (vuGlobalGroup && gid == vuGlobalGroup->Id()) {
    return VU_NO_OP;
  }
  VuEnterCriticalSection();
  VuGroupNode* gnode = groupHead_;
  // make certain group isn't already here

  while (gnode && gnode->gid_ != gid) {
    gnode = gnode->next_;
  }

  if (!gnode) {
    gnode        = new VuGroupNode;
    gnode->gid_  = gid;
    gnode->next_ = groupHead_;
    groupHead_   = gnode;
    groupCount_++;

    retval = VU_SUCCESS;
  }
  VuExitCriticalSection();
  return retval;
}

VU_ERRCODE
VuSessionEntity::RemoveGroup(VU_ID gid)
{
  VU_ERRCODE retval = VU_ERROR;
  VuEnterCriticalSection();

  VuGroupNode* gnode    = groupHead_;
  VuGroupNode* lastnode = 0;

  while (gnode && gnode->gid_ != gid) {
    lastnode = gnode;
    gnode    = gnode->next_;
  }
  if (gnode) { // found group
    if (lastnode) {
      lastnode->next_ = gnode->next_;
    } 
    else { // node was head
      groupHead_ = gnode->next_;
    }
    groupCount_--;
    delete gnode;
    retval = VU_SUCCESS;
  }

  VuExitCriticalSection();
  return retval;
}

VU_ERRCODE
VuSessionEntity::PurgeGroups()
{
  VU_ERRCODE retval = VU_NO_OP;
  VuEnterCriticalSection();
  VuGroupNode *gnode;
  while (groupHead_) {
    gnode      = groupHead_;
    groupHead_ = groupHead_->next_;
    delete gnode;
    retval     = VU_SUCCESS;
  }
  groupCount_ = 0;
  VuExitCriticalSection();
  return retval;
}

#ifdef VU_USE_COMMS
VuTargetEntity * VuSessionEntity::ForwardingTarget(VuMessage *)
{
  VuTargetEntity* retval = Game();
  if (!retval) 
    retval = vuGlobalGroup;
  return retval;
}
#endif

void
VuSessionEntity::SetCallsign(char *callsign)
{
  char* oldsign = callsign_;

  SetDirty ();

  if (callsign) {
    int len = strlen(callsign);

    if (len < MAX_VU_STR_LEN) {
      callsign_ = new char[len + 1];
      strcpy(callsign_, callsign);
    }
    else {
      callsign_ = new char[MAX_VU_STR_LEN + 1];
      strncpy(callsign_, callsign, MAX_VU_STR_LEN);
    }
  } 
  else {
    callsign_ = new char[strlen(VU_DEFAULT_PLAYER_NAME) + 1];
    strcpy(callsign_, VU_DEFAULT_PLAYER_NAME);
  }
  if (oldsign) {
    delete [] oldsign;
    if (this == vuLocalSessionEntity) {
      VuSessionEvent *event =
    new VuSessionEvent(this, VU_SESSION_CHANGE_CALLSIGN, vuGlobalGroup);
      VuMessageQueue::PostVuMessage(event);
    }
  }
}

VU_ERRCODE 
VuSessionEntity::InsertionCallback()
{
  if (this != vuLocalSessionEntity) {
	vuLocalSessionEntity->SetDirty ();
    VuxSessionConnect(this);
    if (Game()) {
      action_ = VU_JOIN_GAME_ACTION;
      Game()->AddSession(this);
    }

    VuEnterCriticalSection();
    VuGroupNode* gnode = groupHead_;

    while (gnode) {
      VuGroupEntity* group = (VuGroupEntity*)vuDatabase->Find(gnode->gid_);
      if (group && group->IsGroup()) {
        group->AddSession(this);
      }
      gnode = gnode->next_;
    }
    VuExitCriticalSection();
    return VU_SUCCESS;
  }
  return VU_NO_OP;
}

VU_ERRCODE 
VuSessionEntity::RemovalCallback()
{
  if (this != vuLocalSessionEntity) {
    VuEnterCriticalSection();
    VuGroupNode* gnode = groupHead_;

    while (gnode) {
      VuGroupEntity* group = (VuGroupEntity*)vuDatabase->Find(gnode->gid_);
      if (group && group->IsGroup()) {
        VuxGroupRemoveSession(group, this);
      }
      gnode = gnode->next_;
    }
    VuExitCriticalSection();

    if (Game()) {
      VuxGroupRemoveSession(Game(), this);
    }
    VuxSessionDisconnect(this);
    return VU_SUCCESS;
  }
  return VU_NO_OP;
}

VU_SESSION_ID
VuSessionEntity::OpenSession()
{
#ifdef VU_USE_COMMS
  if (!IsLocal() || !vuGlobalGroup || !vuGlobalGroup->Connected()) {
    return 0;
  }
  ComAPIHandle ch = vuGlobalGroup->GetCommsHandle();
  if (!ch) {
    ch = vuGlobalGroup->GetReliableCommsHandle();
  }
  if (!ch) {
    return 0;
  }
  
  int idLen = ComAPIHostIDLen(ch);
  if (idLen > sizeof(VU_SESSION_ID)) {
#pragma warning(disable : 4130)
    assert("Session id length is too large!\n" == 0);
#pragma warning(default : 4130)
  }
  char buf[sizeof(VU_SESSION_ID)];
  ComAPIHostIDGet(ch, buf);
  char *ptr = (char *)&sessionId_;
  for (int i = 0; i < idLen; i++) {
    ptr[sizeof(VU_SESSION_ID) - 1 - i] = buf[i];
  }
#else
  sessionId_ = 1;
#endif

  if (sessionId_ != vuLocalSession.creator_) {
    VuReferenceEntity(this);
    // temporarily make private to prevent sending of bogus delete message
    share_.flags_.breakdown_.private_ = 1;
    vuDatabase->Remove(this);
    share_.flags_.breakdown_.private_ = 0;

    share_.ownerId_.creator_ = sessionId_;

    //  - reset ownerId for all local ent's
    VuDatabaseIterator iter;
    VuEntity *ent = 0;
    for (ent = iter.GetFirst(); ent; ent = iter.GetNext()) {
      if (ent->OwnerId() == vuLocalSession && ent != vuPlayerPoolGroup
                                           && ent != vuGlobalGroup) {
        ent->SetOwnerId(OwnerId());
      }
    }
    VuMessageQueue::FlushAllQueues();
    share_.id_.creator_ = sessionId_;
    vuLocalSession = OwnerId();
    SetVuState(VU_MEM_ACTIVE);
    vuDatabase->Insert(this);
    VuDeReferenceEntity(this);
  } 
  else {
    VuMessage *dummyCreateMessage = new VuCreateEvent(this, vuGlobalGroup);
    dummyCreateMessage->RequestReliableTransmit();
    VuMessageQueue::PostVuMessage(dummyCreateMessage);
  }
//  danm_note: not needed, as vuGlobalGroup membership is implicit
//  JoinGroup(vuGlobalGroup);
  JoinGame(vuPlayerPoolGroup);
  return sessionId_;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void VuSessionEntity::CloseSession()
{
	VuGroupEntity* game = Game();
	
	if (!game)
	{
		// session is not open 
		if (!IsLocal())
		{
			vuDatabase->Remove(this);
		}

		return;
	}

	action_ = VU_LEAVE_GAME_ACTION;
	
	game->RemoveSession(this);

	game->Distribute(this);
	
	if (IsLocal())
	{
#ifdef VU_TRACK_LATENCY
		SetTimeSync(VU_NO_SYNC);
#endif
		VuSessionEvent* event = new VuSessionEvent(this, VU_SESSION_CLOSE, vuGlobalGroup);
		VuMessageQueue::PostVuMessage(event);
		VuMessageQueue::FlushAllQueues();
		
#ifdef VU_USE_COMMS
		VuMainThread::FlushOutboundMessages();
#endif
		
		gameId_ = VU_SESSION_NULL_GROUP;
		game_   = 0;
	} 
	else
	{
		int count = game_->SessionCount();

		VuSessionEntity *sess = 0;

		if (count >= 1)
		{
			VuSessionsIterator iter(Game());

			sess = iter.GetFirst();

			while (sess == this)
			{
				sess = iter.GetNext();
			}
		}

		VuDatabaseIterator iter;
		VuEntity* ent = iter.GetFirst();

		while (ent)
		{
			if (ent != this && ent->OwnerId() == Id() && ent->IsGlobal())
			{
				if (!sess || !ent->IsTransferrable() || ((ent->IsGame() && ((VuGameEntity *)ent)->SessionCount()==0)))
				{
					vuDatabase->Remove(ent);
				} 
				else
				{
					ent->SetOwnerId(sess->Id());
				}
			}

			ent = iter.GetNext();
		}

		vuDatabase->Remove(this);
	}

	LeaveAllGroups();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE
VuSessionEntity::JoinGroup(VuGroupEntity* newgroup)
{
  VU_ERRCODE retval = VU_NO_OP;

  if (!newgroup) {
    return VU_ERROR;
  }

  retval = AddGroup(newgroup->Id());

  if (retval != VU_ERROR) {
    if (IsLocal()) {
      VuSessionEvent *event = 0;
      if (!newgroup->IsLocal() && newgroup->SessionCount() == 0 &&
          newgroup != vuGlobalGroup) {
        // we need to transfer it here...
        VuTargetEntity* target = (VuTargetEntity*)vuDatabase->Find(newgroup->OwnerId());
        if (target && target->IsTarget()) {
          VuMessage* pull = new VuPullRequest(newgroup->Id(), target);
          // this would likely result in replicated p2p (evil!)
          // pull->RequestReliableTransport();
          VuMessageQueue::PostVuMessage(pull);
        }
      }
      event = new VuSessionEvent(this, VU_SESSION_JOIN_GROUP, newgroup);
      VuMessageQueue::PostVuMessage(event);
    }
    retval = newgroup->AddSession(this);
  }
  return retval;
}

VU_ERRCODE
VuSessionEntity::LeaveGroup(VuGroupEntity* group)
{
  VU_ERRCODE retval = VU_ERROR;

  if (group) {
    retval = RemoveGroup(group->Id());
    if (retval == VU_SUCCESS && group->RemoveSession(this) >= 0 && IsLocal()) {
      VuSessionEvent *event = new VuSessionEvent(this, VU_SESSION_LEAVE_GROUP, vuGlobalGroup);
      VuMessageQueue::PostVuMessage(event);
      retval = VU_SUCCESS;
    }
  }
  return retval;
}

VU_ERRCODE
VuSessionEntity::LeaveAllGroups()
{
  VuEnterCriticalSection();
  VuGroupNode* gnode  = groupHead_;
  VuGroupNode* ngnode = (gnode ? gnode->next_ : 0);

  while (gnode) {
    VuGroupEntity* gent = (VuGroupEntity*)vuDatabase->Find(gnode->gid_);
    if (gent && gent->IsGroup()) {
      LeaveGroup(gent);
    } 
    else {
      RemoveGroup(gnode->gid_);
    }
    gnode = ngnode;
    if (ngnode) {
      ngnode = ngnode->next_;
    }
  }

  VuExitCriticalSection();
  return VU_SUCCESS;
}

VU_ERRCODE
VuSessionEntity::JoinGame(VuGameEntity* newgame)
{
  VU_ERRCODE retval = VU_NO_OP;
  VuGameEntity *game = Game();
  if (newgame && game == newgame) {
    return VU_NO_OP;
  }
  if (newgame && !game) {
    action_ = VU_JOIN_GAME_ACTION;
  } else if (newgame && game) {
    action_ = VU_CHANGE_GAME_ACTION;
  } else {
    action_ = VU_LEAVE_GAME_ACTION;
  }
#ifdef VU_TRACK_LATENCY
  SetTimeSync(VU_NO_SYNC);
#endif
  VuSessionEvent* event = 0;

  if (game) {
    if (game->RemoveSession(this) >= 0) {
      LeaveAllGroups();
      if (IsLocal()) {
        VuMessageQueue::FlushAllQueues();
        if (game->IsLocal()) {
          // we need to transfer it away...
          VuSessionsIterator iter(Game());
          VuSessionEntity *sess = iter.GetFirst();
          while (sess == this) {
            sess = iter.GetNext();
          }
          if (sess) {
            VuMessage *push = new VuPushRequest(game->Id(), sess);
            VuMessageQueue::PostVuMessage(push);
          }
        }
        if (newgame) {
          event         = new VuSessionEvent(this, VU_SESSION_CHANGE_GAME, vuGlobalGroup);
          event->group_ = newgame->Id();
        } 
        else {
          event   = new VuSessionEvent(this, VU_SESSION_CLOSE, vuGlobalGroup);
          gameId_ = VU_SESSION_NULL_GROUP;
          game_   = 0;
        }
        
        if (this == vuLocalSessionEntity) {
          VuMessage* msg = new VuShutdownEvent(FALSE);
          VuMessageQueue::PostVuMessage(msg);
          vuMainThread->Update();
          
          VuMessageQueue::FlushAllQueues();
        }
      }
    }
    else {
      return VU_ERROR;
    }
  }

  if (newgame) {
    game_   = newgame;
    gameId_ = newgame->Id();

    if (IsLocal()) {
      if (!newgame->IsLocal() && newgame->SessionCount() == 0) {
        // we need to transfer it here...
        VuTargetEntity* target =
          (VuTargetEntity*)vuDatabase->Find(newgame->OwnerId());
        if (target && target->IsTarget()) {
          VuMessage* pull = new VuPullRequest(newgame->Id(), target);
          VuMessageQueue::PostVuMessage(pull);
        }
      }
      if (!game) {
        event = new VuSessionEvent(this, VU_SESSION_JOIN_GAME, vuGlobalGroup);
        event->RequestReliableTransmit();
      }
    }
    retval = newgame->AddSession(this);
  }
  if (event) {
    VuMessageQueue::PostVuMessage(event);
  }
  if (newgame == 0 && IsLocal() && vuPlayerPoolGroup) {
    retval = JoinGame(vuPlayerPoolGroup);
  }
  return retval;
}

VuGameEntity*
VuSessionEntity::Game()
{
  if (!game_) {
    VuEntity* ent = vuDatabase->Find(gameId_);
    if (ent && ent->IsGame()) {
      game_ = (VuGameEntity*)ent;
    } else {
      game_ = 0;
    }
  }
  return game_;
}

#ifdef VU_TRACK_LATENCY
void
VuSessionEntity::SetTimeSync(VU_BYTE newstate)
{
  VU_BYTE oldstate = timeSyncState_;
  timeSyncState_   = newstate;

  if (timeSyncState_ == VU_NO_SYNC) {
    latency_            = 0;
    masterTime_         = 0;
    masterTimePostTime_ = 0;
    masterTimeOwner_    = 0;
  }

  if (timeSyncState_ != oldstate && timeSyncState_ != VU_NO_SYNC && this == vuLocalSessionEntity) {
    VuMessage* msg = 0;
    if (timeSyncState_ == VU_MASTER_SYNC && Game() && Game()->OwnerId() != vuLocalSession) {
      masterTimeOwner_ = Id().creator_;
      // transfer game ownership to new master
      VuTargetEntity* target = (VuTargetEntity*)vuDatabase->Find(Game()->OwnerId());
      
      if (target && target->IsTarget()) {
        msg = new VuPullRequest(GameId(), target);
        msg->RequestReliableTransmit();
        VuMessageQueue::PostVuMessage(msg);
      }
      // reset statistics
      VuSessionsIterator iter(Game());
      VuSessionEntity*   ent = iter.GetFirst();

      while (ent) {
        ent->lagTotal_   = 0;
        ent->lagPackets_ = 0;
        ent->lagUpdate_  = LAG_COUNT_START;
        ent = iter.GetNext();
      }
    }
    msg = new VuSessionEvent(this, VU_SESSION_TIME_SYNC, vuGlobalGroup);
    VuMessageQueue::PostVuMessage(msg);
  }
}

void
VuSessionEntity::SetLatency(VU_TIME latency)
{
  if (this == vuLocalSessionEntity && latency != latency_) {
    VU_TIME oldlatency = latency_;
    latency_ = latency;
    VuxAdjustLatency(latency_, oldlatency);
  }
}
#endif VU_TRACK_LATENCY

VU_ERRCODE 
VuSessionEntity::Handle(VuEvent* event)
{
  int retval = VU_NO_OP;

  switch(event->Type()) {
  case VU_DELETE_EVENT:
  case VU_RELEASE_EVENT:
    if (Game() && this != vuLocalSessionEntity) {
      JoinGame(vuPlayerPoolGroup);
    }
    retval = VU_SUCCESS;
    break;
  default:
    // do nothing
    break;
  }
  return retval;
}

void
VuSessionEntity::SetKeepaliveTime(VU_TIME ts)
{
  SetCollisionCheckTime(ts);
}

VU_TIME
VuSessionEntity::KeepaliveTime()
{
  return LastCollisionCheckTime();
}

VU_ERRCODE
VuSessionEntity::Handle(VuFullUpdateEvent* event)
{
  if (event->EventData()) {
    SetKeepaliveTime(vuxRealTime);
    if (this != vuLocalSessionEntity) {
      VuSessionEntity* sData = (VuSessionEntity*)event->EventData();
#ifdef VU_TRACK_LATENCY
      if (vuLocalSessionEntity->TimeSyncState() == VU_MASTER_SYNC) {
        // gather statistics
        if (vuLocalSessionEntity->masterTimeOwner_ ==
            sData->masterTimeOwner_ &&
            latency_ == sData->Latency()) {

          int lag = ( (event->PostTime() - sData->masterTime_) -
                      (sData->responseTime_ - sData->masterTimePostTime_));
          lag = lag/2;
          
          if (lag < 0) lag = 0;
          
          lagTotal_ += lag;
          
          lagPackets_++;
          if (lagPackets_ >= lagUpdate_) {
            VU_TIME newlatency = lagTotal_ / lagPackets_;
            if (newlatency != latency_) {
              VuSessionEvent *event =
                new VuSessionEvent(this, VU_SESSION_LATENCY_NOTICE, Game());
              event->gameTime_ = newlatency;
              VuMessageQueue::PostVuMessage(event);
            }

            latency_    = newlatency;
            lagTotal_   = 0;
            lagPackets_ = 0;
            lagUpdate_  = lagUpdate_ * 2;
          }
        }
      } 
      else if (sData->TimeSyncState() == VU_MASTER_SYNC &&
                 vuLocalSessionEntity->GameId() == GameId()) {
        vuLocalSessionEntity->masterTime_         = sData->masterTime_;
        vuLocalSessionEntity->masterTimePostTime_ = event->PostTime();
        vuLocalSessionEntity->masterTimeOwner_    = Id().creator_;
      }
#endif VU_TRACK_LATENCY
      if (strcmp(callsign_, sData->callsign_)) {
        SetCallsign(sData->callsign_);
      }
#if VU_MAX_SESSION_CAMERAS > 0
      cameraCount_ = sData->cameraCount_;
      if (cameraCount_ > 0 && cameraCount_ <= VU_MAX_SESSION_CAMERAS) {
        memcpy(&cameras_, &sData->cameras_, sizeof(VU_ID) * cameraCount_);
      }
#endif
    }
  }
  return VuEntity::Handle(event);
}

VU_ERRCODE VuSessionEntity::Handle(VuSessionEvent* event)
{
	int retval = VU_SUCCESS;
	SetKeepaliveTime(vuxRealTime);
	
	switch(event->subtype_)
	{
		case VU_SESSION_CLOSE:
		{
			CloseSession();
			break;
		}
			
		case VU_SESSION_JOIN_GAME:
		{
			VuGameEntity* game = (VuGameEntity*)vuDatabase->Find(event->group_);

			if (game && game->IsGame())
			{
				game->Distribute(this);
				JoinGame(game);
			} 
			else
			{
				retval = 0;
			}

			break;
		}
			
		case VU_SESSION_CHANGE_GAME:
		{
			retval = 0;

			if (Game()
				&& !F4IsBadReadPtr(Game(), sizeof(VuGameEntity)) // JB 010318 CTD
				)
			{
				Game()->Distribute(this);
				VuGameEntity* game = (VuGameEntity*)vuDatabase->Find(event->group_);
				if (game)
				{
					if (game->IsGame())
					{
						retval = 1;
						game->Distribute(this);
						JoinGame(game);
					}
				}
				else
				{
//					MonoPrint ("Session refers to a game that does not exist\n");
					VuTimerEvent *timer = new VuTimerEvent(0, vuxRealTime + 1000, VU_DELAY_TIMER, event);
					VuMessageQueue::PostVuMessage(timer);
				}
			}
			break;
		}
			
		case VU_SESSION_JOIN_GROUP:
		{
			VuGroupEntity* group = (VuGroupEntity*)vuDatabase->Find(event->group_);

			if (group && group->IsGroup())
			{
				JoinGroup(group);
			}
			else
			{
				retval = 0;
			}
			break;
		}
			
		case VU_SESSION_LEAVE_GROUP:
		{
			VuGroupEntity* group = (VuGroupEntity*)vuDatabase->Find(event->group_);

			if (group && group->IsGroup())
			{
				LeaveGroup(group);
			}
			else
			{
				retval = 0;
			}
			break;
		}
			
		case VU_SESSION_DISTRIBUTE_ENTITIES:
		{
			VuGameEntity* game = (VuGameEntity*)vuDatabase->Find(event->group_);

			if (game)
			{
				game->Distribute(this);
			}
			break;
		}
			
		case VU_SESSION_CHANGE_CALLSIGN:
		{
			SetCallsign(event->callsign_);

			break;
		}

#ifdef VU_TRACK_LATENCY
			
		case VU_SESSION_TIME_SYNC:
		{
			SetTimeSync(event->syncState_);

			if (event->syncState_ == VU_MASTER_SYNC &&
				vuLocalSessionEntity->Game() == Game() &&
				vuLocalSessionEntity->TimeSyncState() != VU_MASTER_SYNC)
			{
				vuLocalSessionEntity->masterTimeOwner_ = Id().creator_;
			}
			break;
		}
			
		case VU_SESSION_LATENCY_NOTICE:
		{
			SetLatency(event->gameTime_);
			break;
		}

#endif
			
		default:
		{
			// do nothing
			retval = 0;
			break;
		}
	}
	
	return retval;
}

#ifdef VU_LOW_WARNING_VERSION
// --------------------------- begin stupid section ---------------------------
// all these are really not needed, but make the stupid icl compiler happy
VU_ERRCODE
VuSessionEntity::Handle(VuErrorMessage* error)
{
  return VuEntity::Handle(error);
}

VU_ERRCODE
VuSessionEntity::Handle(VuPushRequest* msg)
{
  return VuEntity::Handle(msg);
}

VU_ERRCODE
VuSessionEntity::Handle(VuPullRequest* msg)
{
  return VuEntity::Handle(msg);
}

VU_ERRCODE
VuSessionEntity::Handle(VuPositionUpdateEvent* event)
{
  return VuEntity::Handle(event);
}

VU_ERRCODE
VuSessionEntity::Handle(VuEntityCollisionEvent* event)
{
  return VuEntity::Handle(event);
}

VU_ERRCODE
VuSessionEntity::Handle(VuTransferEvent* event)
{
  return VuEntity::Handle(event);
}
// --------------------------- end stupid section ---------------------------
#endif VU_LOW_WARNING_VERSION

//-----------------------------------------------------------------------------
// VuGroupEntity
//-----------------------------------------------------------------------------


VuGroupEntity::VuGroupEntity(char* groupname)
  : VuTargetEntity(VU_GROUP_ENTITY_TYPE),
    groupName_(0),
    sessionMax_(VU_DEFAULT_GROUP_SIZE)
{
  groupName_ = new char[strlen(groupname) + 1];
  strcpy(groupName_, groupname);
  VuSessionFilter filter(Id());
  sessionCollection_ = new VuOrderedList(&filter);
  sessionCollection_->Init();
}


VuGroupEntity::VuGroupEntity(int       type, 
                             char*     gamename, 
                             VuFilter* filter)
  : VuTargetEntity(type),
    groupName_(0),
    sessionMax_(VU_DEFAULT_GROUP_SIZE)
{
  groupName_ = new char[strlen(gamename) + 1];
  strcpy(groupName_, gamename);
  VuSessionFilter sfilter(Id());

  if (!filter) filter = &sfilter;

  sessionCollection_ = new VuOrderedList(filter);
  sessionCollection_->Init();
}

VuGroupEntity::VuGroupEntity(VU_BYTE** stream)
  : VuTargetEntity(stream),
    selfIndex_(-1)
{
  VU_BYTE len;
  VU_ID sessionid(0, 0);
  VuSessionFilter filter(Id());
  sessionCollection_ = new VuOrderedList(&filter);
  sessionCollection_->Init();

  // VuEntity part
  memcpy(&share_.ownerId_.creator_, *stream, sizeof(share_.ownerId_.creator_));
  *stream += sizeof(share_.ownerId_.creator_);
  memcpy(&share_.ownerId_.num_, *stream, sizeof(share_.ownerId_.num_));
  *stream += sizeof(share_.ownerId_.num_);
  memcpy(&share_.assoc_.creator_, *stream, sizeof(share_.assoc_.creator_));
  *stream += sizeof(share_.assoc_.creator_);
  memcpy(&share_.assoc_.num_, *stream, sizeof(share_.assoc_.num_));
  *stream += sizeof(share_.assoc_.num_);
  
  // vuGroupEntity part
  len = (VU_BYTE)**stream;      *stream += sizeof(VU_BYTE);
  groupName_ = new char[len + 1];
  memcpy(groupName_, *stream, len);     *stream += len;
  groupName_[len] = '\0'; // null terminate
  memcpy(&sessionMax_, *stream, sizeof(ushort));  *stream += sizeof(ushort);
  short count = 0;
  memcpy(&count, *stream, sizeof(short)); *stream += sizeof(short);
  for (int i = 0; i < count; i++) {
    memcpy(&sessionid, *stream, sizeof(VU_ID));  *stream += sizeof(VU_ID);
    if (sessionid == vuLocalSession) {
      selfIndex_ = i;
    }
    AddSession(sessionid);
  }
}

VuGroupEntity::VuGroupEntity(FILE* file)
  : VuTargetEntity(file)
{
  VU_BYTE len=0;
  VU_ID sessionid(0, 0);
  VuSessionFilter filter(Id());
  sessionCollection_ = new VuOrderedList(&filter);
  sessionCollection_->Init();

  // VuEntity part
  fread(&share_.ownerId_.creator_, sizeof(share_.ownerId_.creator_), 1, file);
  fread(&share_.ownerId_.num_,     sizeof(share_.ownerId_.num_),     1, file);
  fread(&share_.assoc_.creator_,   sizeof(share_.assoc_.creator_),   1, file);
  fread(&share_.assoc_.num_,       sizeof(share_.assoc_.num_),       1, file);

  // vuGroupEntity part
  fread(&len, sizeof(VU_BYTE), 1, file);
  groupName_ = new char[len + 1];
  fread(groupName_, len, 1, file);  
  groupName_[len] = '\0'; // null terminate
  fread(&sessionMax_, sizeof(ushort), 1, file);
  short count = 0;
  fread(&count, sizeof(short), 1, file);
  for (int i = 0; i < count; i++) {
    fread(&sessionid, sizeof(VU_ID), 1, file);
    AddSession(sessionid);
  }
}

VuGroupEntity::~VuGroupEntity()
{
  delete [] groupName_;
  sessionCollection_->Purge();
  sessionCollection_->DeInit();
  delete sessionCollection_;
}

int 
VuGroupEntity::LocalSize()
{
  short count = static_cast<short>(sessionCollection_->Count());

  return 
    sizeof(share_.ownerId_.creator_)
    + sizeof(share_.ownerId_.num_)
    + sizeof(share_.assoc_.creator_)
    + sizeof(share_.assoc_.num_)
    + strlen(groupName_) + 1
    + sizeof(sessionMax_)
    + sizeof(count)
    + (sizeof(VU_ID) * count)  // sessions
    ;
}

int 
VuGroupEntity::SaveSize()
{
  return VuTargetEntity::SaveSize() + VuGroupEntity::LocalSize();
}

int 
VuGroupEntity::Save(VU_BYTE** stream)
{
  VU_BYTE len;

  int retval = VuTargetEntity::Save(stream);

  // VuEntity part
  memcpy(*stream, &share_.ownerId_.creator_, sizeof(share_.ownerId_.creator_));
        *stream += sizeof(share_.ownerId_.creator_);
  memcpy(*stream, &share_.ownerId_.num_, sizeof(share_.ownerId_.num_));
        *stream += sizeof(share_.ownerId_.num_);
  memcpy(*stream, &share_.assoc_.creator_, sizeof(share_.assoc_.creator_));
        *stream += sizeof(share_.assoc_.creator_);
  memcpy(*stream, &share_.assoc_.num_, sizeof(share_.assoc_.num_));
        *stream += sizeof(share_.assoc_.num_);

  // VuGroupEntity part
  len = static_cast<VU_BYTE>(strlen(groupName_));
  **stream = len;       *stream += sizeof(VU_BYTE);

  memcpy(*stream, groupName_, len);   *stream += len;
  memcpy(*stream, &sessionMax_, sizeof(ushort));  *stream += sizeof(ushort);
  short count = static_cast<short>(sessionCollection_->Count());
  memcpy(*stream, &count, sizeof(short)); *stream += sizeof(short);
  VuSessionsIterator iter(this);
  VuSessionEntity *ent = iter.GetFirst();

  while (ent) {
    VU_ID id = ent->Id();
    memcpy(*stream, &id, sizeof(VU_ID)); *stream += sizeof(VU_ID);
    ent = iter.GetNext();
  }

  retval += LocalSize();
  return retval;
}

int 
VuGroupEntity::Save(FILE* file)
{
  int retval = 0;
  VU_BYTE len;

  if (file) {
    retval = VuTargetEntity::Save(file);

    // VuEntity part
    retval += fwrite(&share_.ownerId_.creator_, sizeof(share_.ownerId_.creator_), 1, file);
    retval += fwrite(&share_.ownerId_.num_,     sizeof(share_.ownerId_.num_),     1, file);
    retval += fwrite(&share_.assoc_.creator_,   sizeof(share_.assoc_.creator_),   1, file);
    retval += fwrite(&share_.assoc_.num_,       sizeof(share_.assoc_.num_),       1, file);

    len     = static_cast<VU_BYTE>(strlen(groupName_));

    retval += fwrite(&len,         sizeof(VU_BYTE), 1, file);
    retval += fwrite(groupName_,   len,             1, file);  
    retval += fwrite(&sessionMax_, sizeof(ushort),  1, file);

    short count = static_cast<short>(sessionCollection_->Count());
    retval += fwrite(&count, sizeof(ushort), 1, file);
    VuSessionsIterator iter(this);
    VuSessionEntity* ent = iter.GetFirst();

    while (ent) {
      VU_ID id = ent->Id();
      retval += fwrite(&id, sizeof(VU_ID), 1, file);
      ent = iter.GetNext();
    }
  } 

  return retval;
}

VU_BOOL 
VuGroupEntity::IsGroup()
{
  return TRUE;
}

void 
VuGroupEntity::SetGroupName(char* groupname)
{
  delete [] groupName_;
  groupName_ = new char[strlen(groupname)+1];
  strcpy(groupName_, groupname);
  if (IsLocal()) {
    VuSessionEvent* event =
  new VuSessionEvent(this, VU_SESSION_CHANGE_CALLSIGN, vuGlobalGroup);
    VuMessageQueue::PostVuMessage(event);
  }
}

VU_BOOL 
VuGroupEntity::HasTarget(VU_ID id)
{
  if (id == Id()) {
    return TRUE;
  }
  VuSessionEntity* session = (VuSessionEntity*)vuDatabase->Find(id);
  if (session && session->IsSession()) {
    return SessionInGroup(session);
  }
  return FALSE;
}

VU_BOOL 
VuGroupEntity::InTarget(VU_ID id)
{
  // supports one level of group nesting: groups are In global group
  if (id == Id() || id == vuGlobalGroup->Id()) {
    return TRUE;
  }
  return FALSE;
}

VU_BOOL 
VuGroupEntity::SessionInGroup(VuSessionEntity* session)
{
  VuSessionsIterator iter(this);
  VuEntity* ent = iter.GetFirst();
  while (ent) {
    if (ent == session) {
      return TRUE;
    }
    ent = iter.GetNext();
  }
  return FALSE;
}

VU_ERRCODE VuGroupEntity::AddSession(VuSessionEntity *session)
{
  short count = static_cast<short>(sessionCollection_->Count());
  if (count >= sessionMax_) {
    return VU_ERROR;
  }

#if defined(VU_USE_COMMS)
  VuxGroupAddSession(this, session);
#endif

  if (!sessionCollection_->Find(session->Id())) {
    return sessionCollection_->Insert(session);
  }
  return VU_NO_OP;
}

VU_ERRCODE 
VuGroupEntity::AddSession(VU_ID sessionId)
{
  VuSessionEntity* session = (VuSessionEntity*)vuDatabase->Find(sessionId);
  if (session && session->IsSession()) {
    return AddSession(session);
  }
  return VU_ERROR;
}

VU_ERRCODE 
VuGroupEntity::RemoveSession(VuSessionEntity* session)
{
#if defined(VU_USE_COMMS)
  if (session != vuLocalSessionEntity) {
    VuxGroupRemoveSession(this, session);
  }
#endif

  return sessionCollection_->Remove(session);
}

VU_ERRCODE 
VuGroupEntity::RemoveSession(VU_ID sessionId)
{
  VuSessionEntity* session = (VuSessionEntity*)vuDatabase->Find(sessionId);
  if (session && session->IsSession()) {
    return RemoveSession(session);
  }
  return VU_ERROR;
}

VU_ERRCODE
VuGroupEntity::Handle(VuSessionEvent* event)
{
  int retval = VU_NO_OP;
  switch(event->subtype_) {
  case VU_SESSION_CHANGE_CALLSIGN:
    SetGroupName(event->callsign_);
    retval = VU_SUCCESS;
    break;
  case VU_SESSION_DISTRIBUTE_ENTITIES:
    retval = Distribute(0);
    break;
  default:
    break;
  }
  return retval;
}

VU_ERRCODE
VuGroupEntity::Handle(VuFullUpdateEvent* event)
{
  // update groups?
  return VuEntity::Handle(event);
}

#ifdef VU_LOW_WARNING_VERSION
// --------------------------- begin stupid section ---------------------------
// all these are really not needed, but make the stupid icl compiler happy
VU_ERRCODE 
VuGroupEntity::Handle(VuEvent* event)
{
  return VuEntity::Handle(event);
}

VU_ERRCODE
VuGroupEntity::Handle(VuErrorMessage* error)
{
  return VuEntity::Handle(error);
}

VU_ERRCODE
VuGroupEntity::Handle(VuPushRequest* msg)
{
  return VuEntity::Handle(msg);
}

VU_ERRCODE
VuGroupEntity::Handle(VuPullRequest* msg)
{
  return VuEntity::Handle(msg);
}

VU_ERRCODE
VuGroupEntity::Handle(VuPositionUpdateEvent* event)
{
  return VuEntity::Handle(event);
}

VU_ERRCODE
VuGroupEntity::Handle(VuEntityCollisionEvent* event)
{
  return VuEntity::Handle(event);
}

VU_ERRCODE
VuGroupEntity::Handle(VuTransferEvent* event)
{
  return VuEntity::Handle(event);
}
// --------------------------- end stupid section ---------------------------

#endif VU_LOW_WARNING_VERSION

VU_ERRCODE VuGroupEntity::Distribute(VuSessionEntity *)
{
  // do nothing
  return VU_NO_OP;
}

VU_ERRCODE 
VuGroupEntity::InsertionCallback()
{
  if (this != vuGlobalGroup) {
    VuxGroupConnect(this);
    VuSessionsIterator iter(this);
    VuSessionEntity*   sess = iter.GetFirst();
    while (sess) {
      AddSession(sess);
      sess = iter.GetNext();
    }
    return VU_SUCCESS;
  }
  return VU_NO_OP;
}

VU_ERRCODE 
VuGroupEntity::RemovalCallback()
{
  VuSessionsIterator iter(this);
  VuSessionEntity* sess = iter.GetFirst();

  while (sess) {
    RemoveSession(sess);
    sess = iter.GetNext();
  }

  VuxGroupDisconnect(this);
  return VU_SUCCESS;
}

//-----------------------------------------------------------------------------
// VuGlobalGroup
//-----------------------------------------------------------------------------

GlobalGroupFilter globalGrpFilter;

VuGlobalGroup::VuGlobalGroup()
  : VuGroupEntity(VU_GLOBAL_GROUP_ENTITY_TYPE, vuxWorldName, &globalGrpFilter)
{
  // make certain owner is NULL session
  share_.ownerId_     = VU_SESSION_NULL_CONNECTION;
  share_.id_.creator_ = 0;
  share_.id_.num_     = VU_GLOBAL_GROUP_ENTITY_ID;
  sessionMax_         = 1024;
  connected_          = FALSE;
}

VuGlobalGroup::~VuGlobalGroup()
{
  // empty
}

VU_BOOL VuGlobalGroup::HasTarget(VU_ID)
{
  // global group includes everybody
  return TRUE;
}

// virtual function interface -- stubbed out here
int 
VuGlobalGroup::SaveSize()
{
  return 0;
}

int 
VuGlobalGroup::Save(VU_BYTE **)
{
  return 0;
}

int 
VuGlobalGroup::Save(FILE *)
{
  return 0;
}

//-----------------------------------------------------------------------------
// VuGameEntity
//-----------------------------------------------------------------------------

VuGameEntity::VuGameEntity(ulong domainMask,
                           char* gamename)
  : VuGroupEntity(VU_GAME_ENTITY_TYPE, VU_GAME_GROUP_NAME),
    domainMask_(domainMask),
    gameName_(0)
{
  gameName_ = new char[strlen(gamename) + 1];
  strcpy(gameName_, gamename);
}

VuGameEntity::VuGameEntity(int   type,
                           ulong domainMask,
                           char* gamename, 
                           char* groupname)
  : VuGroupEntity(type, groupname),
    domainMask_(domainMask),
    gameName_(0)
{
  gameName_ = new char[strlen(gamename) + 1];
  strcpy(gameName_, gamename);
}

VuGameEntity::VuGameEntity(VU_BYTE** stream)
  : VuGroupEntity(stream)
{
  VU_BYTE len;

  memcpy(&domainMask_, *stream, sizeof(ulong));  *stream += sizeof(ulong);
  len = (VU_BYTE)**stream;      *stream += sizeof(VU_BYTE);
  gameName_ = new char[len + 1];
  memcpy(gameName_, *stream, len);      *stream += len;
  gameName_[len] = '\0';  // null terminate
#ifdef VU_TRACK_LATENCY
  short count = 0;
  VU_TIME latency;
  memcpy(&count, *stream, sizeof(short)); *stream += sizeof(short);
  for (int i = 0; i < count; i++) {
    memcpy(&latency, *stream, sizeof(latency));  *stream += sizeof(latency);
    if (selfIndex_ == i && this == vuLocalSessionEntity->Game()) {
      vuLocalSessionEntity->SetLatency(latency);
    }
  }
#endif VU_TRACK_LATENCY
}

VuGameEntity::VuGameEntity(FILE* file)
  : VuGroupEntity(file)
{
  VU_BYTE len=0;

  fread(&domainMask_, sizeof(ulong), 1, file);
  fread(&len, sizeof(VU_BYTE), 1, file);
  gameName_ = new char[len + 1];
  fread(gameName_, len, 1, file);  
  gameName_[len] = '\0';  // null terminate

#if defined(VU_TRACK_LATENCY)
  short count = 0;
  VU_TIME latency;

  fread(&count, sizeof(count), 1, file);

  for (int i = 0; i < count; i++)
    // just need to read...
    fread(&latency, sizeof(latency), 1, file);
#endif
}

VuGameEntity::~VuGameEntity()
{
  delete [] gameName_;
}

int 
VuGameEntity::LocalSize()
{
#ifdef VU_TRACK_LATENCY
  short count = sessionCollection_->Count();
#endif

  return sizeof(domainMask_)
       + strlen(gameName_) + 1
#ifdef VU_TRACK_LATENCY
       + sizeof(count)
       + (sizeof(VU_TIME) * count)  // session latency
#endif VU_TRACK_LATENCY
      ;
}

int 
VuGameEntity::SaveSize()
{
  return VuGroupEntity::SaveSize() + VuGameEntity::LocalSize();
}

int 
VuGameEntity::Save(VU_BYTE** stream)
{
  VU_BYTE len;

  int retval = VuGroupEntity::Save(stream);
  memcpy(*stream, &domainMask_, sizeof(ulong));  *stream += sizeof(ulong);
  len = static_cast<VU_BYTE>(strlen(gameName_));
  **stream = len;       *stream += sizeof(VU_BYTE);
  memcpy(*stream, gameName_, len);    *stream += len;

#ifdef VU_TRACK_LATENCY
  short count = sessionCollection_->Count();
  memcpy(*stream, &count, sizeof(short)); *stream += sizeof(short);
  VuSessionsIterator iter(this);
  VuSessionEntity *ent = iter.GetFirst();
  while (ent) {
    VU_TIME latency = ent->Latency();
    memcpy(*stream, &latency, sizeof(latency)); *stream += sizeof(latency);
    ent = iter.GetNext();
  }
#endif

  retval += LocalSize();
  return retval;
}

int 
VuGameEntity::Save(FILE* file)
{
  int retval = 0;
  VU_BYTE len;

  if (file) {
    retval  = VuGroupEntity::Save(file);
    retval += fwrite(&domainMask_, sizeof(ulong), 1, file);
    len = static_cast<VU_BYTE>(strlen(gameName_));
    retval += fwrite(&len, sizeof(VU_BYTE), 1, file);
    retval += fwrite(gameName_, len, 1, file);  

#ifdef VU_TRACK_LATENCY
    VuEnterCriticalSection();
    short count = sessionCollection_->Count();
    retval += fwrite(&count, sizeof(ushort), 1, file);
    VuSessionsIterator iter(this);
    VuSessionEntity *ent = iter.GetFirst();
    while (ent) {
      VU_TIME latency = ent->Latency();
      retval += fwrite(&latency, sizeof(latency), 1, file);
      ent = iter.GetNext();
    }
    VuExitCriticalSection();
#endif
  }
  return retval;
}

VU_BOOL 
VuGameEntity::IsGame()
{
  return TRUE;
}

void 
VuGameEntity::SetGameName(char* gamename)
{
  delete [] gameName_;
  gameName_ = new char[strlen(gamename)+1];
  strcpy(gameName_, gamename);
  if (IsLocal()) {
    VuSessionEvent* event = new VuSessionEvent(this, VU_SESSION_CHANGE_CALLSIGN, vuGlobalGroup);
    VuMessageQueue::PostVuMessage(event);
  }
}

VU_ERRCODE 
VuGameEntity::AddSession(VuSessionEntity* session)
{
  short count = static_cast<short>(sessionCollection_->Count());
  if (count >= sessionMax_) {
    return VU_ERROR;
  }

#ifdef VU_USE_COMMS
  if (session->IsLocal()) {
    // connect to all sessions in the group
    VuSessionsIterator siter(this);
    for (VuSessionEntity *s = siter.GetFirst(); s; s = siter.GetNext()) {
      if (s != vuLocalSessionEntity) {
        VuxSessionConnect(s);
      }
    }
  } else if (vuLocalSessionEntity->Game() == this) {
    // connect to particular session
    VuxSessionConnect(session);
  }
  VuxGroupAddSession(this, session);
#endif

  if (!sessionCollection_->Find(session->Id())) {
    return sessionCollection_->Insert(session);
  }
  return VU_NO_OP;
}

VU_ERRCODE 
VuGameEntity::RemoveSession(VuSessionEntity *session)
{
#ifdef VU_USE_COMMS
  if (session->IsLocal()) {
    // disconnect all sessions
    VuSessionsIterator siter(this);
    for (VuSessionEntity *s = siter.GetFirst(); s; s = siter.GetNext()) {
      VuxSessionDisconnect(s);
    }
//    VuxGroupDisconnect(this);
  } else {
    // just disconnect from particular session
    VuxSessionDisconnect(session);
  }
  VuxGroupRemoveSession(this, session);
#endif

  return sessionCollection_->Remove(session);
}

VU_ERRCODE
VuGameEntity::Handle(VuSessionEvent* event)
{
  int retval = VU_NO_OP;
  switch(event->subtype_) {
  case VU_SESSION_CHANGE_CALLSIGN:
    SetGameName(event->callsign_);
    retval = VU_SUCCESS;
    break;
  case VU_SESSION_DISTRIBUTE_ENTITIES:
    retval = Distribute(0);
    break;
  default:
    break;
  }
  return retval;
}

VU_ERRCODE
VuGameEntity::Handle(VuFullUpdateEvent* event)
{
  return VuEntity::Handle(event);
}

VU_ERRCODE 
VuGameEntity::RemovalCallback()
{
  // take care of sessions we still think are in this game...
  VuSessionsIterator iter(this);
  VuSessionEntity* sess = iter.GetFirst();
  while (sess && sess->Game() == this) {
    sess->JoinGame(vuPlayerPoolGroup);
    sess = iter.GetNext();
  }
  VuxGroupDisconnect(this);
  return VU_SUCCESS;
}

#ifdef VU_LOW_WARNING_VERSION
// --------------------------- begin stupid section ---------------------------
// all these are really not needed, but make the stupid icl compiler happy
VU_ERRCODE 
VuGameEntity::Handle(VuEvent* event)
{
  return VuEntity::Handle(event);
}

VU_ERRCODE
VuGameEntity::Handle(VuErrorMessage* error)
{
  return VuEntity::Handle(error);
}

VU_ERRCODE
VuGameEntity::Handle(VuPushRequest* msg)
{
  return VuEntity::Handle(msg);
}

VU_ERRCODE
VuGameEntity::Handle(VuPullRequest* msg)
{
  return VuEntity::Handle(msg);
}

VU_ERRCODE
VuGameEntity::Handle(VuPositionUpdateEvent* event)
{
  return VuEntity::Handle(event);
}

VU_ERRCODE
VuGameEntity::Handle(VuEntityCollisionEvent* event)
{
  return VuEntity::Handle(event);
}

VU_ERRCODE
VuGameEntity::Handle(VuTransferEvent* event)
{
  return VuEntity::Handle(event);
}
// --------------------------- end stupid section ---------------------------

#endif VU_LOW_WARNING_VERSION

VU_ERRCODE
VuGameEntity::Distribute(VuSessionEntity* sess)
{
  if (!sess || (Id() == sess->GameId())) {
    int i;
    int count[32];
    int totalcount = 0;
    int myseedlower[32];
    int myseedupper[32];
    for (i = 0; i < 32; i++) {
      count[i] = 0;
      myseedlower[i] = 0;
      myseedupper[i] = vuLocalSessionEntity->LoadMetric() - 1;
    }
    
    VuSessionsIterator siter(this);
    VuSessionEntity *cs = siter.GetFirst();
    while (cs) {
      if (!sess || sess->Id() != cs->Id()) {
        for (i = 0; i < 32; i++) {
          if (1<<i & cs->DomainMask()) {
            count[i] += cs->LoadMetric();
            totalcount++;
          }
        }
      }
      cs = siter.GetNext();
    }
    if (totalcount == 0) {
      // nothing to do!  just return...
      return VU_NO_OP;
    }
    cs = siter.GetFirst();
    while (cs && cs != vuLocalSessionEntity) {
      if (!sess || sess->Id() != cs->Id()) {
        for (i = 0; i < 32; i++) {
          if (1<<i & cs->DomainMask()) {
            myseedlower[i]+=cs->LoadMetric();
            myseedupper[i]+=cs->LoadMetric();
          }
        }
      }
      cs = siter.GetNext();
    }
    if (!cs) {
      if (!sess) {
        // note: something is terribly amiss... so bail...
        return VU_ERROR;
      } else {
        // ... most likely, we did not find any viable sessions
        VuDatabaseIterator dbiter;
        VuEntity *ent = dbiter.GetFirst();
        while (ent) {
          if ((ent->OwnerId().creator_ == sess->SessionId()) && (sess!=ent) &&
              (sess->VuState() != VU_MEM_ACTIVE ||
               (ent->IsTransferrable() && !ent->IsGlobal()))){
            vuDatabase->Remove(ent);
          }
          ent = dbiter.GetNext();
        }
      }
    } else if (Id() == vuLocalSessionEntity->GameId()) {
      VuDatabaseIterator dbiter;
      VuEntity *ent = dbiter.GetFirst();
      VuEntity *test_ent = ent;
      VuEntity *ent2;
      int index;
      while (ent) {
        if (!ent->IsTransferrable() && 
            sess && sess->VuState() != VU_MEM_ACTIVE  &&
            ent->OwnerId().creator_ == sess->SessionId()) {
          vuDatabase->Remove(ent);
        } else if ((!sess || (ent->OwnerId().creator_ == sess->SessionId())) &&
                   ((1<<ent->Domain()) & vuLocalSessionEntity->DomainMask())) {
          if (ent->Association() != vuNullId &&
              (ent2 = vuDatabase->Find(ent->Association())) != 0) {
            test_ent = ent2;
          }
          index = test_ent->Id().num_ % count[test_ent->Domain()];
          if (index >= myseedlower[test_ent->Domain()] &&
              index <= myseedupper[test_ent->Domain()] &&
              ent->IsTransferrable() && !ent->IsGlobal()) {
            ent->SetOwnerId(vuLocalSession);
          }
        }
        ent = dbiter.GetNext();
        test_ent = ent;
      }
    } 
    else {
      VuListIterator grpiter(vuGameList);
      VuEntity* ent      = grpiter.GetFirst();
      VuEntity* test_ent = ent;
      VuEntity* ent2;
      int index;
      while (ent) {
        if (!ent->IsTransferrable() &&
            sess && sess->VuState() != VU_MEM_ACTIVE  &&
            ent->OwnerId().creator_ == sess->SessionId()) {
          vuDatabase->Remove(ent);
        } else if ((!sess || (ent->OwnerId().creator_ == sess->SessionId())) &&
                   ((1<<ent->Domain()) & vuLocalSessionEntity->DomainMask())) {
          if (ent->Association() != vuNullId &&
              (ent2 = vuDatabase->Find(ent->Association())) != 0) {
            test_ent = ent2;
          }
          index = test_ent->Id().num_ % count[test_ent->Domain()];
          if (index >= myseedlower[test_ent->Domain()] &&
              index <= myseedupper[test_ent->Domain()] &&
              ent->IsTransferrable() && !ent->IsGlobal() &&
              (!sess || sess->VuState() == VU_MEM_ACTIVE) ) {
            ent->SetOwnerId(vuLocalSession);
          }
        }
        ent      = grpiter.GetNext();
        test_ent = ent;
      }
    }
  }
  return VU_SUCCESS;
}

//-----------------------------------------------------------------------------
// VuPlayerPoolGame
//-----------------------------------------------------------------------------

VuPlayerPoolGame::VuPlayerPoolGame(ulong domainMask)
  : VuGameEntity(VU_PLAYER_POOL_GROUP_ENTITY_TYPE, domainMask,
      VU_PLAYER_POOL_GROUP_NAME, vuxWorldName)
{
  // make certain owner is NULL session
  share_.ownerId_     = VU_SESSION_NULL_CONNECTION;
  share_.id_.creator_ = 0;
  share_.id_.num_     = VU_PLAYER_POOL_ENTITY_ID;
  sessionMax_         = 255;

  // hack, hack, hack up a lung
  sessionCollection_->DeInit();
  delete sessionCollection_;
  VuSessionFilter filter(Id());
  sessionCollection_ = new VuOrderedList(&filter);
  sessionCollection_->Init();
}

VuPlayerPoolGame::~VuPlayerPoolGame()
{
  // empty
}

// virtual function interface -- stubbed out here
int 
VuPlayerPoolGame::SaveSize()
{
  return 0;
}

int 
VuPlayerPoolGame::Save(VU_BYTE**)
{
  return 0;
}

int 
VuPlayerPoolGame::Save(FILE*)
{
  return 0;
}

VU_ERRCODE
VuPlayerPoolGame::Distribute(VuSessionEntity *sess)
{
  // just remove all ents managed by session
  if (sess && (Id() == sess->GameId())) {
    VuDatabaseIterator dbiter;
    VuEntity *ent = dbiter.GetFirst();
    while (ent) {
      if ((ent->OwnerId().creator_ == sess->SessionId()) &&
          (!ent->IsPersistent()) &&
          (sess->VuState() != VU_MEM_ACTIVE ||
           (ent->IsTransferrable() && !ent->IsGlobal()))) {
        vuDatabase->Remove(ent);
      }
      ent = dbiter.GetNext();
    }
  }
  return VU_SUCCESS;
}






