// Copyright 1998 (c) MicroProse, Inc.  All Rights Reserved

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
};

#include "vu2.h"
#include "vu_priv.h"
#include <windows.h> // JB 010304 Needed for CTD checks
#include "FalcSess.h"//me123
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C"
{
	int check_bandwidth (int size);
}
//#define DEBUG_KEEPALIVE 1 

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// statics & globals
static VuListIterator* vuTargetListIter     = 0;

VuFilteredList*        vuGameList           = 0;
VuFilteredList*        vuTargetList         = 0;
VuGlobalGroup*         vuGlobalGroup        = 0;
VuPlayerPoolGame*      vuPlayerPoolGroup    = 0;
VuSessionEntity*       vuLocalSessionEntity = 0;
VuMainThread*          vuMainThread         = 0;
VuPendingSendQueue*    vuNormalSendQueue    = 0;
VuPendingSendQueue*    vuLowSendQueue       = 0;
VU_TIME                vuTransmitTime       = 0;

VU_ID vuLocalSession(0, 0);

int VuMainThread::bytesSent_ = 0;
int SGpfSwitchVal = 100;	// pf switch

extern VU_TIME VuRandomTime(VU_TIME max);
extern VU_ID_NUMBER vuAssignmentId;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class VuGameFilter : public VuFilter
{

public:

	VuGameFilter();
	virtual ~VuGameFilter();
	
	virtual VU_BOOL Test(VuEntity *ent);
	virtual VU_BOOL RemoveTest(VuEntity *ent);
	virtual int Compare(VuEntity *ent1, VuEntity *ent2);
    // returns ent2->Id() - ent1->Id()
	virtual VuFilter *Copy();
	
protected:

	VuGameFilter(VuGameFilter* other);
	
	// DATA

protected:
	// none
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class VuTargetFilter : public VuFilter
{

public:

	VuTargetFilter();
	virtual ~VuTargetFilter();
	
	virtual VU_BOOL Test(VuEntity* ent);
	virtual VU_BOOL RemoveTest(VuEntity* ent);
	virtual int Compare(VuEntity* ent1, VuEntity* ent2);
    // returns ent2->Id() - ent1->Id()
	virtual VuFilter* Copy();
	
protected:

	VuTargetFilter(VuTargetFilter* other);
	
	// DATA

protected:
	// none
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuBaseThread::VuBaseThread()
{
	// empty
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuBaseThread::~VuBaseThread()
{
	if (messageQueue_)
	{
		messageQueue_->DispatchAllMessages(TRUE);   // flush queue
		delete messageQueue_;
		messageQueue_ = 0;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void VuBaseThread::Update()
{
	messageQueue_->DispatchAllMessages(FALSE);    // flush queue
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuThread::VuThread(VuMessageFilter* filter, int queueSize) : VuBaseThread()
{
	if (filter)
	{
		messageQueue_ = new VuMessageQueue(queueSize, filter);
	} 
	else
	{
		VuStandardMsgFilter smf;
		messageQueue_ = new VuMessageQueue(queueSize, &smf);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuThread::VuThread(int queueSize) : VuBaseThread()
{
	VuStandardMsgFilter smf;
	messageQueue_ = new VuMessageQueue(queueSize, &smf);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuThread::~VuThread()
{
	if (messageQueue_)
	{
		messageQueue_->DispatchAllMessages(TRUE);   // flush queue
		delete messageQueue_;
		messageQueue_ = 0;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuMainMessageQueue *VuThread::MainQueue()
{
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuMainThread::VuMainThread
(
	int dbSize,
	int queueSize,
	VuSessionEntity *(*sessionCtorFunc)(void)
) : VuBaseThread()
{
	VuStandardMsgFilter smf;
	messageQueue_ = new VuMainMessageQueue(queueSize, &smf);
	
#if defined(VU_AUTO_UPDATE)
	autoUpdateList_ = 0;
#endif 
	
	if (vuCollectionManager || vuDatabase)
	{
		VU_PRINT("VU: Warning:  creating second VuMainThread!\n");
	} 
	else
	{
		Init(dbSize, sessionCtorFunc);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuMainThread::VuMainThread
(
	int dbSize,
	VuMessageFilter *filter,
	int queueSize,
	VuSessionEntity *(*sessionCtorFunc)(void)
) : VuBaseThread()
{
	if (filter)
	{
		messageQueue_ = new VuMainMessageQueue(queueSize, filter);
	} 
	else
	{
		VuStandardMsgFilter smf;
		messageQueue_ = new VuMainMessageQueue(queueSize, &smf);
	}
	
#if defined(VU_AUTO_UPDATE)
	autoUpdateList_ = 0;
#endif

	if (vuCollectionManager || vuDatabase)
	{
		VU_PRINT("VU: Warning:  creating second VuMainThread!\n");
	}
	else
	{
		Init(dbSize, sessionCtorFunc);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void VuMainThread::Init(int dbSize, VuSessionEntity *(*sessionCtorFunc)(void))
{
	// set global, for sneaky internal use...
	vuMainThread = this;
	
	vuCollectionManager = new VuCollectionManager();
	vuDatabase          = new VuDatabase(dbSize);  // create global database
	vuAntiDB            = new VuAntiDatabase(1 + dbSize/8);
	vuAntiDB->Init();
	
	VuGameFilter gfilter;
	vuGameList = new VuOrderedList(&gfilter);
	vuGameList->Init();
	
	VuTargetFilter tfilter;
	vuTargetList     = new VuFilteredList(&tfilter);
	vuTargetList->Init();
	vuTargetListIter = new VuListIterator(vuTargetList);
	
	// create global group
	vuGlobalGroup = new VuGlobalGroup();
	vuDatabase->Insert(vuGlobalGroup);
	
	vuPlayerPoolGroup = 0;
	
	// randomize assignment id
	vuAssignmentId = (VU_ID_NUMBER)rand();
	if (vuAssignmentId < VU_FIRST_ENTITY_ID)
	{
		vuAssignmentId = VU_FIRST_ENTITY_ID;
	}
	
	// create local session
	if (sessionCtorFunc)
	{
		vuLocalSessionEntity = sessionCtorFunc();
	}
	else
	{
		vuLocalSessionEntity = new VuSessionEntity(vuxLocalDomain, "player");
	}

	vuLocalSession = vuLocalSessionEntity->OwnerId();
	vuDatabase->Insert(vuLocalSessionEntity);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuMainThread::~VuMainThread()
{
	// KCK Added. Probably a good idea
	JoinGame(vuPlayerPoolGroup);
	
	// must do this prior to destruction of database (below)
	messageQueue_->DispatchAllMessages(TRUE);   // flush queue
	delete messageQueue_;
	messageQueue_ = 0;
	vuGameList->DeInit();
	delete vuGameList;
	vuGameList = 0;
	delete vuTargetListIter;
	vuTargetListIter = 0;
	vuTargetList->DeInit();
	delete vuTargetList;
	vuTargetList = 0;
	
#ifdef VU_AUTO_UPDATE
	if (autoUpdateList_)
	{
		autoUpdateList_->DeInit();
		delete autoUpdateList_;
	}
#endif // VU_AUTO_UPDATE
	
	VuReferenceEntity(vuLocalSessionEntity);
	VuReferenceEntity(vuGlobalGroup);
	VuReferenceEntity(vuPlayerPoolGroup);
	vuGlobalGroup     = 0;
	vuPlayerPoolGroup = 0;
	vuAntiDB->Purge();
	vuDatabase->Purge();
	
	vuAntiDB->DeInit();
	delete vuAntiDB;
	vuAntiDB = 0;
	delete vuDatabase;
	vuDatabase = 0;
	delete vuCollectionManager;
	vuCollectionManager = 0;
	
	VuDeReferenceEntity(vuGlobalGroup);
	VuDeReferenceEntity(vuPlayerPoolGroup); 
	VuDeReferenceEntity(vuLocalSessionEntity);
	vuLocalSessionEntity = 0;
	
	vuMainThread = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuMainMessageQueue *VuMainThread::MainQueue()
{
	return (VuMainMessageQueue*)messageQueue_;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef VU_USE_COMMS
int VuMainThread::FlushOutboundMessages()
{
	VuTargetEntity* target;
	int retval = 0;
	int curret = 0;
	
	if (vuNormalSendQueue)
	{
		vuNormalSendQueue->DispatchAllMessages(TRUE);
	}
	
	if (vuLowSendQueue)
	{
		vuLowSendQueue->DispatchAllMessages(TRUE);
	}
	
	// VuTargetEntity* target = (VuTargetEntity*)vuTargetListIter->CurrEnt();
	
	//  if (!target)
	target = (VuTargetEntity*)vuTargetListIter->GetFirst();
	
	// attempt to send one packet for each comhandle
	while (target && (curret = target->FlushOutboundMessageBuffer()) != 0)
	{
		if (curret > 0)
		{
			retval += curret;
		}

		target = (VuTargetEntity*)vuTargetListIter->GetNext();
	}

	return retval;
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int VuMainThread::SendQueuedMessages (void)
{

#define MAX_TARGETS 256

	static int
		last_time = 0,
		lru_size[MAX_TARGETS];

	VuTargetEntity
		*targets[MAX_TARGETS];

	char
		used[MAX_TARGETS];

	int
		now,
		index,
		size,
		best,
		total,
		count;

	VuTargetEntity
		*target;

	VuListIterator
		iter(vuTargetList);

	assert(VuHasCriticalSection());
	total = 0;

	now = vuxRealTime;

	// Decay the LRU sizes

	if (now - last_time > 100)
	{
		last_time = now;

		for (index = 0; index < MAX_TARGETS; index ++)
		{
			lru_size[index] /= 2;
		}
	}

	// Build array of targets - for easy indexing later

	target = (VuTargetEntity*) iter.GetFirst ();
	index = 0;
	while (target)
	{
		if (target->IsSession ())
		{
			targets[index] = target;
			used[index] = FALSE;

			index ++;
		}

		target = (VuTargetEntity*) iter.GetNext ();
	}

	while (index < MAX_TARGETS)
	{
		targets[index] = 0;
		index ++;
	}

	// Now until we run out of stuff to send

	do
	{
		count = 0;

		// clear the used per iteration

		for (index = 0; index < MAX_TARGETS; index ++)
		{
			used[index] = FALSE;
		}

		do
		{
			// find the best - ie. the smallest lru_size
			best = -1;
			size = 0x7fffffff;

			for (index = 0; (targets[index]) && (index < MAX_TARGETS); index ++)
			{
				if ((!used[index]) && (lru_size[index] < size))
				{
					best = index;
					size = lru_size[index];
				}
			}

			// If we found a best target

			if (best >= 0)
			{
				// attempt to send one packet
				size = targets[best]->SendQueuedMessage ();

//				if (size)
//				{
//					MonoPrint ("Send %08x %3d %d\n", targets[best], size, lru_size[best]);
//				}

				// mark it as used
				used[best] = TRUE;

				// add into the lru size
				lru_size[best] += size;

				// remember that we send something
				count += size;
				total += size;
			}
		}
		while (best != -1);
	} while (count);

	return total;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Get messages from comms, in a round robin manner
// it is assumed that Target::GetMessage will also send
// anything it can in the send queue

int VuMainThread::GetMessages()
{
	int
		count;

	assert(VuHasCriticalSection());
	count = 0;

#if defined(VU_USE_COMMS)
	VuListIterator  iter(vuTargetList);
	VuTargetEntity* target;

	// Flush all outbound messages into various queues

	target = (VuTargetEntity*) iter.GetFirst ();
	
	while (target)
	{
		// attempt to send one packet of each type
		target->FlushOutboundMessageBuffer();
		count += target->GetMessages ();

		// Get the next target
		target = (VuTargetEntity*) iter.GetNext ();
	}

	// Get all messages - and send any in a queue

#endif
	
	return count;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void VuMainThread::UpdateGroupData(VuGroupEntity* group)
{
	int count = 0;

//	assert(VuHasCriticalSection()); // hmm apparently not required...
	VuSessionsIterator iter(group);
	VuSessionEntity*   sess = iter.GetFirst();

	while (sess)
	{
	    assert(FALSE == F4IsBadReadPtr(sess, sizeof *sess));
		if
		(
			(sess != vuLocalSessionEntity) &&
			(sess->GetReliableCommsStatus () == VU_CONN_ERROR)
		)
		{
#ifdef DEBUG_KEEPALIVE
			VU_PRINT("VU: timing out session id 0x%x: time = %d, last sig = %d\n",
			(VU_KEY)sess->Id(), vuxRealTime, sess->LastTransmissionTime());
#endif

			// time out this session
			sess->CloseSession();
		}
		else
		{
			count ++;
		}

		sess = iter.GetNext();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// S.G. NEW GLOBAL VARIABLE USED BY THE -pf SWITCH
static VU_TIME unkGlobal = 0;
    extern float clientbwforupdatesmodifyer;
	extern float MinBwForOtherData ;
	float Posupdsize = 64;
	bool bwmode = false; // mode 0 is ui(no pos upd mode mode 1 is pos upd mode
	int MinClientBandwidth = 3300;
void VuMainThread::Update()
{
	static int
		last_oob = 0;

	static int switcher = FALSE;

	int
		now;

	static int
		last_bg = 0;

	VuEntity* ent;

#if defined(VU_USE_COMMS)
	ResetXmit();
#endif
	messageQueue_->DispatchAllMessages(FALSE);    // flush queue

#if defined(VU_USE_COMMS)
#if defined(VU_QUEUE_DR_UPDATES)

	// Send at least one fine and one rough update per frame

	now = vuxRealTime;

#endif

	// Send normal messages

	if (vuNormalSendQueue)
	{
		vuNormalSendQueue->DispatchAllMessages(FALSE);
	}

#if defined(VU_QUEUE_DR_UPDATES)
//	VU_BOOL	active = TRUE;

#define MAX_GPU	49

	int
		UpdsSendToSess[256],// this counts updates pr pf for each session
		best_index[MAX_GPU],
		bestfine_index[MAX_GPU],
		copy_loop,
		loop,
		count;

	SM_SCALAR
		error,
		best_error[MAX_GPU],
		bestfine_error[MAX_GPU];

	VuEntity
		*best[MAX_GPU],
		*bestfine[MAX_GPU];

	VuGameEntity* game;	game  = vuLocalSessionEntity->Game();
	if (!game) return;
	VuSessionsIterator Sessioniter(game);
	VuSessionEntity*   sess;
	VuSessionEntity*  Host = (VuSessionEntity*)vuDatabase->Find (game->OwnerId());
	
	int HostBandWidth = 3302; if (Host) HostBandWidth = Host->BandWidth();
	int Roughitersize = 0;
	int Fineitersize = 0;
	int GAMEHOST = FALSE;if (vuLocalSessionEntity->Game()->OwnerId() == vuLocalSessionEntity->Id()) GAMEHOST = TRUE;
	VuListIterator iterfine(vuxDriverSettings->updateQueue_);
	VuListIterator iterrough(vuxDriverSettings->roughUpdateQueue_);
	 
	float RoughUpdRatioOfTotalUpds = 1.00;//	% rough updates of total
	float MinFineUpdToClientPrPf = 1.0f; // each client is garantied this amount of fine updates
	float updatesize = Posupdsize;// bytes pr update
	static int NumberOfClients = 98;
	static int LastNumberOfClients = 99;
	static int TotalClientBw;// the clientbw's addet up
	static int Clientpf;// the pf the clients will use based on min clientbw
	static VU_TIME LastBwset;
	static float TotalUpdFromClientPrPf = 3;
	static bool changebandwidth = FALSE;
	static bool changebandwidthforreal = FALSE;
	static bool PosUpdBwSet = false;
	static bool lastbwmode = false;
	static bool intbwmode = false;
	static float lastclientbwforupdatesmodifyer = 0;
	static float lastMinBwForOtherData =0;
	bool bwmodechange = false;
	static int counter = 50;
	float calculatedCBW = clientbwforupdatesmodifyer;
	
 	NumberOfClients = (game->SessionCount()-1);
	
	// ADDED BY S.G. TO RECREATE THE -pf VARIABLE
	if (SGpfSwitchVal == 0 || now - unkGlobal > (unsigned int)SGpfSwitchVal) {
	unsigned int TimeDelta = now - unkGlobal;
	unkGlobal = now;		
	if (TimeDelta > 800) TimeDelta = 800;
// S.G. THIS CODE HAS BEEN MOVED INSIDE OUR IF BODY



	if (MinClientBandwidth <=3300) Clientpf = 100;
	else if (MinClientBandwidth <=25600) Clientpf = 80;
    else if (MinClientBandwidth <=51200) Clientpf = 50;
	else Clientpf = 40;
	
	if (NumberOfClients != LastNumberOfClients)
		 changebandwidth = TRUE;

	if (changebandwidth)
	{// the number of NumberOfClients in the game have changed
	// we need to find and ajust the minclientbw and
	// set the comapibandwidth

// finding minclientbw
		sess = Sessioniter.GetFirst();
		MinClientBandwidth = 1000000;
		TotalClientBw =0;

		int sessionsnumber =0;
		while (sess )
		{
		sessionsnumber ++;
		if (sess->BandWidth() < MinClientBandwidth) 
			MinClientBandwidth = sess->BandWidth();

		if (sess != vuLocalSessionEntity) 
			TotalClientBw += sess->BandWidth();

		sess = Sessioniter.GetNext ();
		}
		//me123 we need to make sure all sessions are checked
		// sometimes game->sessioncount increases before the sessions are attached
		if (sessionsnumber > NumberOfClients) {
			 changebandwidth = FALSE;
			 changebandwidthforreal = TRUE;
			 LastNumberOfClients = NumberOfClients;
		}

// find MaxClientBandwidth
		if (NumberOfClients && HostBandWidth < MinClientBandwidth * NumberOfClients)
		  MinClientBandwidth = HostBandWidth * 8 / 10 / NumberOfClients;
	}
	//ajust updatemodifyer so we have atleast MinBwForOtherData boud in total for non position updates

	if (( MinClientBandwidth  * (1.0f - calculatedCBW)) < MinBwForOtherData)
	calculatedCBW = 1.0f - (MinBwForOtherData / (float)MinClientBandwidth);

// how many updates are the clients allowed to send to the host pr pf
	TotalUpdFromClientPrPf = (MinClientBandwidth* calculatedCBW /1000.0f * TimeDelta / updatesize);
//client total updates cabability pr pf
	float TotalUpdToClientPrPf = (MinClientBandwidth*calculatedCBW / 1000.0f * TimeDelta / updatesize);//default bw is a modem 33.6 = 4 pr pf
	
	//this the total possition updates allowed pr pf from the host
	float HostUpdCapabilityPrPf = (HostBandWidth  - ( MinClientBandwidth * NumberOfClients * (1.0f - calculatedCBW)) ) 
		*TimeDelta/ 1000 / updatesize  ;
	
	//limit the TotalUpdToClientPrPf based on HostUpdCapabilityPrPf
	if (TotalUpdToClientPrPf *NumberOfClients > HostUpdCapabilityPrPf)
		TotalUpdToClientPrPf = HostUpdCapabilityPrPf/NumberOfClients;

	if (TotalUpdToClientPrPf < 1)
		TotalUpdToClientPrPf = 1;
	
// rough is an proportion of client bw, but atleast xx amout of fine updates
	float RoughUpdToClientPrPf = TotalUpdToClientPrPf  * RoughUpdRatioOfTotalUpds;
	if (TotalUpdToClientPrPf-RoughUpdToClientPrPf < MinFineUpdToClientPrPf) RoughUpdToClientPrPf = TotalUpdToClientPrPf - MinFineUpdToClientPrPf;

// limit rough updates based on HostUpdCapabilityPrPf (see if the host can support it)
	if (NumberOfClients)
		{
		if (RoughUpdToClientPrPf < 1) RoughUpdToClientPrPf = 1;//minimum rough pr pf
		}

	if(NumberOfClients)
		  {
			//ajust the pf switch 
			if (GAMEHOST)
			{
				if ((TotalUpdToClientPrPf <= 2 && SGpfSwitchVal < 150))
				SGpfSwitchVal +=5;
				else if (SGpfSwitchVal > 40)
					 SGpfSwitchVal -=5;
				if (SGpfSwitchVal < 50)SGpfSwitchVal = 40;
				if (SGpfSwitchVal > 100)SGpfSwitchVal = 150;
			if (SGpfSwitchVal > Clientpf) 
 			SGpfSwitchVal=Clientpf; // if we don't do this the host will update slow when unbizzy enviroment
			}
			else SGpfSwitchVal=Clientpf;

		  }

	if (lastclientbwforupdatesmodifyer != clientbwforupdatesmodifyer ||
		MinBwForOtherData != lastMinBwForOtherData)
		bwmodechange = true;

	lastMinBwForOtherData = MinBwForOtherData;
	lastclientbwforupdatesmodifyer = clientbwforupdatesmodifyer;


	counter --;
	if (iterrough.GetFirst () || iterfine.GetFirst ()) intbwmode = true;
	if (counter == 0)
	{
		counter = 100;
		if (!intbwmode) bwmode = false;
		else bwmode = true;
		intbwmode = false;
	}
	if (bwmode != lastbwmode) bwmodechange = true;
	lastbwmode = bwmode;
		
		
	if (changebandwidthforreal || bwmodechange)
	{ // we are not sending possition updates so lets use our full bw for other crap
		if (!bwmode) ComAPISetBandwidth (vuLocalSessionEntity->BandWidth());
		else PosUpdBwSet = true;
		changebandwidthforreal = FALSE;
	}

 	if (PosUpdBwSet && bwmode)
		{
			PosUpdBwSet = false;
			LastBwset = now;

			// set the bw (this will reset current used bw to MAX)
			if (GAMEHOST && NumberOfClients)
				{
				ComAPISetBandwidth (HostBandWidth);//bw non updates
				}
			else if (NumberOfClients)
				{
				assert (MinClientBandwidth>1000);
				ComAPISetBandwidth (MinClientBandwidth);
				}
		}


	//initialice our session possition update counter
		count = 0;
		for ( sess = Sessioniter.GetFirst() ; sess ; sess = Sessioniter.GetNext ())
		{
			UpdsSendToSess[count] =0;
			count++;
		}
		
		memset(best_error, 0, sizeof best_error);
		memset(best, 0, sizeof best);

// first we prioritice the rough entitys
		count = 0;
		Roughitersize = 0;
		ent = iterrough.GetFirst ();
		while (ent)
		{
			if (ent->EntityDriver() )
			{
				if ( ent->EntityDriver()->Type() != VU_MASTER_DRIVER && !GAMEHOST)//this is another clients ent 
				{
					assert(!"why has this happend !!!");
 					vuxDriverSettings->roughUpdateQueue_->Remove(ent);
					if(ent->EntityDriver()->Type() == VU_MASTER_DRIVER )((VuMaster *)ent->EntityDriver())->SetpendingRoughUpdate (FALSE);
					else ((VuSlave *)ent->EntityDriver())->SetpendingRoughUpdate (FALSE); 
				}
				else// we are either host or the driving a clients ent
				{
					if (ent->EntityDriver()->Type() == VU_MASTER_DRIVER)
					error = ((VuMaster *)ent->EntityDriver())->CalcError ();
					else
					error = ((VuSlave *)ent->EntityDriver())->CalcError ();

					for (loop = 0; loop < MAX_GPU ; loop ++)
					{
						if (best_error[loop] < error)
						{
							for (copy_loop = MAX_GPU - 1; copy_loop > loop; copy_loop --)
							{
								best[copy_loop] = best[copy_loop - 1];
								best_error[copy_loop] = best_error[copy_loop - 1];
								best_index[copy_loop] = best_index[copy_loop - 1];
							}

							best[loop] = ent;
							best_error[loop] = error;
							best_index[loop] = count;

							count ++;
							break;
						}
					}
				}
			}
			Roughitersize ++;
			ent = iterrough.GetNext ();
		}
		
// let's ajust our rough update modifyer 
// so we get a resonable size of the entiter
		if (!GAMEHOST)// we are client 
		{
			if (Roughitersize > 10 && vuxDriverSettings->globalPosTolMod_ < 0.5)
			{
				vuxDriverSettings->globalPosTolMod_ += 0.024F;
				vuxDriverSettings->globalAngleTolMod_ += 0.008F;
			}
			else if (vuxDriverSettings->globalPosTolMod_ > 0.001F)
			{			
				vuxDriverSettings->globalPosTolMod_ -= 0.006F;
				vuxDriverSettings->globalAngleTolMod_ -= 0.002F;
			}
			 
		}// we are host
		else if (Roughitersize > RoughUpdToClientPrPf + 10 && vuxDriverSettings->globalPosTolMod_ < 0.5)
		{
			vuxDriverSettings->globalPosTolMod_ += 0.024F;
			vuxDriverSettings->globalAngleTolMod_ += 0.008F;
		}
		else if (vuxDriverSettings->globalPosTolMod_ > 0.001F)
		{			
			vuxDriverSettings->globalPosTolMod_ -= 0.006F;
			vuxDriverSettings->globalAngleTolMod_ -= 0.002F;
		}

			 



//Here We send the rough updates
		if (count)
		{
			for (loop = 0; (best[loop]) && (loop < Roughitersize &&  loop < MAX_GPU); loop ++)
			{
		   int LastRoughUpdateSend = FALSE;
			VuSessionEntity* DrivingSession = (VuSessionEntity*) vuDatabase->Find(best[loop]->OwnerId());
//////////////////
/// we are a client
// the client sends rough updates and only to the host and not about ownship (well send that fine)
				if (!GAMEHOST && best[loop]->EntityDriver()->Type() == VU_MASTER_DRIVER)// we are a client
					{
					 	if (loop != 0 && loop >= TotalUpdFromClientPrPf-1 )
					 	  {
								 LastRoughUpdateSend = TRUE;
								 break;// send rough updates until theres room for atleast one fine update too
					 	  }
						if (DrivingSession->Id() != vuLocalSessionEntity->Game()->OwnerId() &&
							best[loop] != vuLocalSessionEntity->Camera(0) &&
							((VuMaster *)best[loop]->EntityDriver())->GeneratePositionUpdate(FALSE,vuxGameTime, TRUE, (VuTargetEntity*)vuDatabase->Find(vuLocalSessionEntity->Game()->OwnerId())) == VU_SUCCESS)
				
						{
		  				  ((VuMaster *)best[loop]->EntityDriver())->UpdateDrdata (TRUE);
		  				  vuxDriverSettings->roughUpdateQueue_->Remove(best[loop]);
		  				  //MonoPrint ("Sending rough update as client. count %08x\n",  count);
						}
					}
////////////////////
// we are a host
// we will send rough updates to sessions
// which is inside xx nm of this entity
				else if (GAMEHOST)
					{	
						BIG_SCALAR
							xdist,
							ydist,
							zdist,
							maxroughdist= 80 * 6000,
							dist = maxroughdist;
							int updateagain = FALSE;
							int updatet = FALSE;
						sess = Sessioniter.GetFirst();
						count =0;// for session bw count

						if(UpdsSendToSess[count] + 1  >= RoughUpdToClientPrPf)
					 	{
					 		 LastRoughUpdateSend = TRUE;
					 		 break; // send rough updates until theres room for atleast one fine update too
					 	}

						while (sess)
						{
						int flying = FALSE;
						if (((FalconSessionEntity*)vuDatabase->Find(sess->Id()))->GetFlyState () ==  FLYSTATE_FLYING||
							 ((FalconSessionEntity*)vuDatabase->Find(sess->Id()))->GetFlyState () ==  FLYSTATE_DEAD) flying = TRUE;
	 		  			if (sess != DrivingSession && sess != vuLocalSessionEntity ||!flying)
					 		{//this code should exclude updates about object which are more then xxnm from this sessions bobble
					 		// but it cdt's sometimes and i cant figure out why
					 		if ( flying && sess->Camera(0)&& !sess->Camera(0)->IsLocal()&&
								 sess != vuLocalSessionEntity&&// don't send to us self
								 DrivingSession != sess ) // don't send a session that drives the ent
								 
					 			{	
									 xdist = vuabs(best[loop]->XPos() - sess->Camera(0)->XPos() );
									 ydist = vuabs(best[loop]->YPos() - sess->Camera(0)->YPos() );
									 zdist = vuabs(best[loop]->ZPos() - sess->Camera(0)->ZPos() );
									 dist  = vumax(vumax(xdist, ydist), zdist);

									 if (sess->Camera(0)->XPos() == 0 && sess->Camera(0)->YPos() == 0 && sess->Camera(0)->ZPos() == 0)
										dist = 0;
					 			}
							else if (sess != DrivingSession && sess != vuLocalSessionEntity)dist = 1;

					 			//////////// send to this session ?
					 		if (dist < maxroughdist) 
					 			{
								 	if (best[loop]->EntityDriver()->Type() != VU_MASTER_DRIVER) 		
								 	{
								 	if (((VuSlave *)best[loop]->EntityDriver())->GeneratePositionUpdate(FALSE,vuxGameTime, TRUE, ((VuTargetEntity*)sess)) == VU_SUCCESS)
								 		{
										 updatet = TRUE;
								 		 UpdsSendToSess[count] ++;						
									 	}
									else {
										 updateagain = TRUE;
//										 MonoPrint ("could not send rough update about slave ent");
											}
								 	}

									else if (((VuMaster *)best[loop]->EntityDriver())->GeneratePositionUpdate(FALSE,vuxGameTime, TRUE, ((VuTargetEntity*)sess)) == VU_SUCCESS)
									{
									updatet = TRUE;
									UpdsSendToSess[count] ++;
									}
									else 
									{
									updateagain = TRUE;
//									MonoPrint ("could not send rough update about master ent");
									}

									if (updatet && !updateagain) 
									{
					 				if(best[loop]->EntityDriver()->Type() == VU_MASTER_DRIVER )((VuMaster *)best[loop]->EntityDriver())->UpdateDrdata (TRUE);
					 				else ((VuSlave *)best[loop]->EntityDriver())->UpdateDrdata (TRUE); 
									vuxDriverSettings->roughUpdateQueue_->Remove(best[loop]);
									}
					 			}
					 		}
								dist = maxroughdist;
					 			sess = Sessioniter.GetNext ();
					 			count ++;
						}
					}
				if (LastRoughUpdateSend) break;
			}
		}

/////////////////////////////////////
	// NOW HANDLE THE FINE UPDATES

// first sort the fine updates
		memset(bestfine_error, 0, sizeof bestfine_error);
		memset(bestfine, 0, sizeof bestfine);
		count = 0;
		Fineitersize = 0;
		ent = iterfine.GetFirst ();

		while (ent)
		{	
			if (ent->EntityDriver())
			{
				if (ent->EntityDriver()->Type() != VU_MASTER_DRIVER && !GAMEHOST)		
				{
					assert(!"this should not happend");
					vuxDriverSettings->updateQueue_->Remove(ent);
					if(ent->EntityDriver()->Type() == VU_MASTER_DRIVER )((VuMaster *)ent->EntityDriver())->SetpendingUpdate (FALSE);
					else ((VuSlave *)ent->EntityDriver())->SetpendingUpdate (FALSE);
				}
				else// we are either host or driving 
				{
					if (ent->EntityDriver()->Type() == VU_MASTER_DRIVER)
					error = (float)(vuxRealTime - ((VuMaster *)ent->EntityDriver())->lastFinePositionUpdateTime ());
					else
					error = (float)(vuxRealTime - ((VuSlave *)ent->EntityDriver())->lastFinePositionUpdateTime ());

					for (loop = 0; loop < MAX_GPU ; loop ++)
					{
						if (bestfine_error[loop] < error)
						{
							for (copy_loop = MAX_GPU - 1; copy_loop > loop; copy_loop --)
							{
								bestfine[copy_loop] = bestfine[copy_loop - 1];
								bestfine_error[copy_loop] = bestfine_error[copy_loop - 1];
								bestfine_index[copy_loop] = bestfine_index[copy_loop - 1];
							}

							bestfine[loop] = ent;
							bestfine_error[loop] = error;
							bestfine_index[loop] = count;

							count ++;
							break;
						}
					}
				}
			}
			Fineitersize ++;
			ent = iterfine.GetNext ();
		}
	
		if (count)
		for (loop = 0; (bestfine[loop]) && (loop < Fineitersize &&  loop < MAX_GPU); loop ++)
		{ 		
			int LastFineUpdateSend = FALSE;	 
			VuSessionEntity* DrivingSession = (VuSessionEntity*) vuDatabase->Find(bestfine[loop]->OwnerId());
			if (bestfine[loop]->EntityDriver())
			{
			//	if (!check_bandwidth (64)) break;

//////////////////////////////////////
// we are a client
//only send fine update about us self
				if (!GAMEHOST)
					{
						if (bestfine[loop] == vuLocalSessionEntity->Camera(0) &&  
							 bestfine[loop]->EntityDriver()->Type() == VU_MASTER_DRIVER)
						{	
						//send an update to the host
						if (((VuMaster *)bestfine[loop]->EntityDriver())->GeneratePositionUpdate(FALSE,vuxGameTime, FALSE, (VuTargetEntity*)vuDatabase->Find(vuLocalSessionEntity->Game()->OwnerId())) == VU_SUCCESS)
							{
							vuxDriverSettings->updateQueue_->Remove(bestfine[loop]);
							LastFineUpdateSend = TRUE;
							//MonoPrint ("Sending fine update as client. count %08x\n",  count);
							}
						}
						else if (bestfine[loop] != vuLocalSessionEntity->Camera(0))
						{
							vuxDriverSettings->updateQueue_->Remove(bestfine[loop]);
							assert(!"this should not happend");
						}
					}

//////////////////////////////////////
// we are host
// the host sends fine updates to specific sessions which are inside 
// foreupd distance of this ent
				else 
				{
				if (game) 
					{
						sess = Sessioniter.GetFirst();
						count = 0;
						int updated = FALSE;
						int updateagain = FALSE;
						while (sess)
						{
							//if (!check_bandwidth (64)) 
							float UpdatesSend =0;
							
							for (int hop =0; NumberOfClients >= hop ; hop ++)
							{
								UpdatesSend += UpdsSendToSess[hop];
							}

							if (UpdatesSend + NumberOfClients >= HostUpdCapabilityPrPf)
							{
							LastFineUpdateSend = TRUE;
							break;
							}

							if (sess != DrivingSession && 
								 sess != vuLocalSessionEntity)
							{// this bestfine[loop] is in this sess fine upd
								if (bestfine[loop]->EntityDriver() && bestfine[loop]->EntityDriver()->Type() == VU_MASTER_DRIVER &&
									((VuMaster *)bestfine[loop]->EntityDriver())->CheckForceUpd(sess)||
									bestfine[loop]->EntityDriver() && bestfine[loop]->EntityDriver()->Type() != VU_MASTER_DRIVER&&
									((VuSlave *)bestfine[loop]->EntityDriver())->CheckForceUpd(sess))
								{
							float TotalUpdToThisClientPrPf = ( (sess->BandWidth() -(MinClientBandwidth * (1.0f - calculatedCBW))) / 1000.0f * TimeDelta / updatesize);
									if (UpdsSendToSess[count] + 1 < TotalUpdToThisClientPrPf)
									{
										if (bestfine[loop]->EntityDriver()->Type() != VU_MASTER_DRIVER) 		
										{
										if (((VuSlave *)bestfine[loop]->EntityDriver())->GeneratePositionUpdate(FALSE,vuxGameTime, FALSE, ((VuTargetEntity*)sess)) == VU_SUCCESS)
											{		
												UpdsSendToSess[count] ++;
												updated = TRUE;
												//MonoPrint ("Sending fine update to session  %08x\n",  UpdsSendToSess[count]);
											}									
										}

										else if (((VuMaster *)bestfine[loop]->EntityDriver())->GeneratePositionUpdate(FALSE,vuxGameTime, FALSE, ((VuTargetEntity*)sess)) == VU_SUCCESS)
										{		
											UpdsSendToSess[count] ++;
											updated = TRUE;
											//MonoPrint ("Sending fine update to session  %08x\n",  UpdsSendToSess[count]);
										}
									}
								}
							}
							count++;
							sess = Sessioniter.GetNext ();
						}
						if (updated) 
						{
						vuxDriverSettings->updateQueue_->Remove(bestfine[loop]);
						}

					}
				}
			}
			if (LastFineUpdateSend) break;
		}
//#define checkbandwidth
#ifdef checkbandwidth

static VU_TIME laststatcound =0;
if (vuxGameTime > laststatcound + 1000)
{
laststatcound = vuxGameTime;
int totalbwused = 0;
	for (count = 0; count <= NumberOfClients; count++)
	{
	MonoPrint("%d updsBW send to client, %d ", (int)(UpdsSendToSess[count]*updatesize), count);
	totalbwused += UpdsSendToSess[count]*(int)updatesize;
	}

	MonoPrint("upds pr sec %d", (int)(totalbwused*1000/TimeDelta));
}


#endif
	}
// S.G. END OF OUR IF BODY


#endif

	// Send low priority messages

	if (vuLowSendQueue)
	{
		vuLowSendQueue->DispatchAllMessages(FALSE);
	}

	VuEnterCriticalSection ();
	SendQueuedMessages ();

	// Get and dispatch all messages
	GetMessages();
	VuExitCriticalSection ();

	messageQueue_->DispatchAllMessages(FALSE);    // flush queue

	vuTransmitTime = vuxRealTime;

#if defined(VU_AUTO_UPDATE)
	if (autoUpdateList_)
	{
		VuEnterCriticalSection();

		VuRBIterator iter(autoUpdateList_);
		VuEntity*    cur = iter.GetFirst();

		while (cur && cur->LastTransmissionTime()+cur->UpdateRate() < vuxRealTime)
		{
			if (cur == vuLocalSessionEntity)
			{
				// since sessions don't collide, we'll use this time
				cur->SetCollisionCheckTime(vuxRealTime);
				UpdateGroupData(vuGlobalGroup);
				VuListIterator grp_iter(vuGameList);
				VuGameEntity* game = (VuGameEntity*)grp_iter.GetFirst();

				while (game)
				{
					UpdateGroupData(game);
					game = (VuGameEntity*)grp_iter.GetNext();
				}
			}

			VuTargetEntity*    target = vuGlobalGroup;
			VuFullUpdateEvent* update = 0;

			if (!cur->IsGlobal())
			{
				target = vuLocalSessionEntity->Game();
			}

			update = new VuFullUpdateEvent(cur, target);

			if (cur->IsTarget())
			{
				update->MarkAsKeepalive();
			}

			iter.RemoveCurrent();

			// note: the following is to preserve update spacing
			VU_TIME nextSched = vuxRealTime - ((vuxRealTime - cur->LastTransmissionTime()) % cur->UpdateRate());
			cur->SetTransmissionTime(nextSched);

#ifdef DEBUG_KEEPALIVE
			VU_PRINT("VU: sending periodic update on id 0x%x: time = %d; ",
			(VU_KEY)cur->Id(), vuxRealTime);
#endif

			VuMessageQueue::PostVuMessage(update);

#ifdef DEBUG_KEEPALIVE
			VU_PRINT("next = %d\n", (ulong)(cur->LastTransmissionTime() + cur->UpdateRate()));
#endif

			autoUpdateList_->Insert(cur);
			cur = iter.GetNext();
		}

		VuExitCriticalSection();
	}

#elif defined VU_AUTO_KEEPALIVE

	if (vuLocalSessionEntity->IsDirty ())
	{
		VuFullUpdateEvent *msg = new VuFullUpdateEvent(vuLocalSessionEntity, vuGlobalGroup);
		msg->RequestOutOfBandTransmit();
		msg->RequestReliableTransmit();
		VuMessageQueue::PostVuMessage(msg);

		vuLocalSessionEntity->ClearDirty ();
	}

	if (now - last_bg > 5000)
	{
		last_bg = now;

		vuLocalSessionEntity->SetTransmissionTime(vuxRealTime);

		VuBroadcastGlobalEvent *msg = new VuBroadcastGlobalEvent(vuLocalSessionEntity, vuGlobalGroup);
		msg->RequestOutOfBandTransmit();
		VuMessageQueue::PostVuMessage(msg);

		vuLocalSessionEntity->SetTransmissionTime(vuxRealTime);

		UpdateGroupData(vuGlobalGroup);
		VuListIterator grp_iter(vuGameList);
		VuGameEntity* game = (VuGameEntity*)grp_iter.GetFirst();

		while (game)
		{
			// KCK: Check for empty game
			if (game->IsLocal() && game != vuPlayerPoolGroup && !game->SessionCount())
			{
				vuDatabase->Remove(game);
			}

			// KCK: Broadcast group data
			if (game->IsLocal() && game->LastTransmissionTime() + game->UpdateRate() < vuxRealTime)
			{
				if (game->IsDirty ())
				{
					VuFullUpdateEvent *msg = new VuFullUpdateEvent(game, vuGlobalGroup);
					msg->RequestReliableTransmit();
					msg->RequestOutOfBandTransmit ();
					VuMessageQueue::PostVuMessage(msg);

					game->ClearDirty ();
				}
				else
				{
//					MonoPrint ("Game %08x\n", game->LastTransmissionTime () + game->UpdateRate ());
					VuBroadcastGlobalEvent *msg = new VuBroadcastGlobalEvent(game, vuGlobalGroup);
					msg->RequestOutOfBandTransmit ();
					VuMessageQueue::PostVuMessage(msg);
				}

				game->SetTransmissionTime(vuxRealTime);
			}

			UpdateGroupData(game);
			game = (VuGameEntity*)grp_iter.GetNext();
		}
	}

#endif //VU_AUTO_UPDATE
#endif // VU_USE_COMMS

	vuCollectionManager->Purge();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE VuMainThread::JoinGame(VuGameEntity* game)
{
	VU_ERRCODE retval = vuLocalSessionEntity->JoinGame(game);
	vuLocalSession    = vuLocalSessionEntity->Id();
	
#if defined(VU_AUTO_UPDATE)
	
	if (retval >= 0)
	{
		VuEnterCriticalSection();

		if (autoUpdateList_)
		{
			autoUpdateList_->DeInit();
			delete autoUpdateList_;
		}
		
		VuTransmissionFilter filter;
		autoUpdateList_ = new VuRedBlackTree(&filter);
		autoUpdateList_->Init();
		VuExitCriticalSection();
		
		vuTransmitTime = vuxRealTime;
		VuDatabaseIterator iter;
		VuEntity* ent = iter.GetFirst();
		
		while (ent)
		{
			ent->SetTransmissionTime(vuTransmitTime-VuRandomTime(ent->UpdateRate()));
			autoUpdateList_->Insert(ent);
			ent = iter.GetNext();
		}
	}
#endif

	return retval;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef VU_USE_COMMS
VU_ERRCODE VuMainThread::InitComms
(
	ComAPIHandle handle,
	int bufSize,
	int packSize,
	ComAPIHandle relhandle,
	int relBufSize,
	int relPackSize,
	int resendQueueSize
)
{
	if (vuGlobalGroup->GetCommsHandle()||vuPlayerPoolGroup||!handle)
	{
		return VU_ERROR;
	}
	
	if (!relhandle)
	{
		relhandle   = handle;
		relBufSize  = bufSize;
		relPackSize = packSize;
	}
	
	ResetXmit();
	
	// create player pool group(game)
	vuPlayerPoolGroup = new VuPlayerPoolGame(vuxLocalDomain);
	vuDatabase->Insert(vuPlayerPoolGroup);
	vuNormalSendQueue = new VuPendingSendQueue(resendQueueSize);
	vuLowSendQueue    = new VuPendingSendQueue(resendQueueSize);
	
	vuGlobalGroup->SetCommsHandle(handle, bufSize, packSize);
	vuGlobalGroup->SetCommsStatus(VU_CONN_ACTIVE);
	vuGlobalGroup->SetReliableCommsHandle(relhandle, relBufSize, relPackSize);
	vuGlobalGroup->SetReliableCommsStatus(VU_CONN_ACTIVE);
	vuGlobalGroup->SetConnected(TRUE);
	vuLocalSessionEntity->OpenSession();
	
	return JoinGame(vuPlayerPoolGroup);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE VuMainThread::DeinitComms()
{
	if (vuPlayerPoolGroup)
	{
		VuEnterCriticalSection();
		FlushOutboundMessages();
		ResetXmit();
		
		VuListIterator  iter(vuTargetList);
		VuTargetEntity* target = (VuTargetEntity*)iter.GetFirst();
		
		while (target)
		{
			if (target->IsSession())
			{
				((VuSessionEntity*)target)->CloseSession();
			}
			
			target = (VuTargetEntity*)iter.GetNext();
		}
		
		delete vuNormalSendQueue;
		delete vuLowSendQueue;
		
		vuNormalSendQueue = 0;
		vuLowSendQueue    = 0;
		
		vuGlobalGroup->SetConnected(FALSE);
		vuGlobalGroup->SetCommsStatus(VU_CONN_INACTIVE);
		vuGlobalGroup->SetCommsHandle(0, 0, 0);
		vuGlobalGroup->SetReliableCommsStatus(VU_CONN_INACTIVE);
		vuGlobalGroup->SetReliableCommsHandle(0, 0, 0);
		
		vuDatabase->Remove(vuPlayerPoolGroup);
		vuPlayerPoolGroup = 0;
		VuExitCriticalSection();
	}
	
	return VU_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void VuMainThread::ReportXmit(int bytesSent)
{
	bytesSent_ += bytesSent;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void VuMainThread::ResetXmit()
{
	bytesSent_ = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int VuMainThread::BytesSent()
{
	return bytesSent_;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int VuMainThread::BytesPending()
{
	VuGameEntity
		*game;

	int retval = 0;
	
	if (vuNormalSendQueue)
	{
		retval += vuNormalSendQueue->BytesPending();
	}
	
	if (vuLowSendQueue)
	{
		retval += vuLowSendQueue->BytesPending();
	}
	
	game = vuLocalSessionEntity->Game ();

	if (game)
	{
		VuSessionsIterator iter(game);
		VuTargetEntity* target = (VuTargetEntity*)iter.GetFirst();
		
		while (target)
		{
			retval += target->BytesPending();
			target  = (VuTargetEntity*)iter.GetNext();
		}
	}
	
	return retval;
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE VuMainThread::LeaveGame()
{
#if defined(VU_USE_COMMS)
	DeinitComms();
	vuLocalSessionEntity->CloseSession();
#endif
	
	vuLocalSession = vuLocalSessionEntity->Id();

	return VU_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// VuTransmissionFilter
//-----------------------------------------------------------------------------

VuTransmissionFilter::VuTransmissionFilter() : VuKeyFilter()
{
	// empty
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuTransmissionFilter::VuTransmissionFilter(VuTransmissionFilter* other) : VuKeyFilter(other)
{
	// empty
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuTransmissionFilter::~VuTransmissionFilter()
{
	// empty
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_BOOL VuTransmissionFilter::Notice(VuMessage* event)
{
	if (event->Type() == VU_TRANSFER_EVENT)
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

VU_BOOL VuTransmissionFilter::Test(VuEntity* ent)
{
  return (VU_BOOL)(((ent->IsLocal() && (ent->UpdateRate() > (VU_TIME)0)) ? TRUE : FALSE));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ulong VuTransmissionFilter::Key(VuEntity* ent)
{
	return (ulong)(ent->LastTransmissionTime() + ent->UpdateRate());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int VuTransmissionFilter::Compare(VuEntity* ent1, VuEntity* ent2)
{
	ulong time1 = Key(ent1);
	ulong time2 = Key(ent2);

	return (time1 > time2 ? (int)(time1-time2) : -(int)(time2-time1));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuFilter *VuTransmissionFilter::Copy()
{
	return new VuTransmissionFilter(this);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// VuGameFilter
//-----------------------------------------------------------------------------

VuGameFilter::VuGameFilter() : VuFilter()
{
	// empty
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuGameFilter::VuGameFilter(VuGameFilter* other) : VuFilter(other)
{
	// empty
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuGameFilter::~VuGameFilter()
{
	// empty
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_BOOL VuGameFilter::Test(VuEntity* ent)
{
	//  if (ent == vuPlayerPoolGroup) return FALSE;
	return ent->IsGame();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_BOOL VuGameFilter::RemoveTest(VuEntity* ent)
{
	return ent->IsGame();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int VuGameFilter::Compare (VuEntity* ent1, VuEntity* ent2)
{
	if ((VU_KEY)ent2->Id() > (VU_KEY)ent1->Id())
	{
		return -1;
	}
	else if ((VU_KEY)ent2->Id() < (VU_KEY)ent1->Id())
	{
		return 1;
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuFilter *VuGameFilter::Copy()
{
	return new VuGameFilter(this);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// VuTargetFilter
//-----------------------------------------------------------------------------

VuTargetFilter::VuTargetFilter() : VuFilter()
{
	// empty
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuTargetFilter::VuTargetFilter(VuTargetFilter* other) : VuFilter(other)
{
	// empty
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuTargetFilter::~VuTargetFilter()
{
	// empty
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_BOOL VuTargetFilter::Test(VuEntity* ent)
{
	return ent->IsTarget();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_BOOL VuTargetFilter::RemoveTest(VuEntity* ent)
{
	return ent->IsTarget();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int VuTargetFilter::Compare(VuEntity* ent1, VuEntity* ent2)
{
	if ((VU_KEY)ent2->Id() > (VU_KEY)ent1->Id())
	{
		return -1;
	}
	else if ((VU_KEY)ent2->Id() < (VU_KEY)ent1->Id())
	{
		return 1;
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuFilter *VuTargetFilter::Copy()
{
	return new VuTargetFilter(this);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
