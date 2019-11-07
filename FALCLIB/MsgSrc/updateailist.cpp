/*
 * Machine Generated source file for message "Update DF Ai list".
 * NOTE: The functions here must be completed by hand.
 * Generated on 20-January-1998 at 22:41:31
 * Generated from file EVENTS.XLS by Peter
 */

#include "MsgInc\UpdateAIList.h"
#include "mesg.h"

UI_UpdateAIList::UI_UpdateAIList(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (UpdateAIList, FalconEvent::SimThread, entityId, target, loopback)
{
	dataBlock.gameID=FalconNullId;
	dataBlock.count=0;
	dataBlock.size=0;
	dataBlock.data=NULL;
}

UI_UpdateAIList::UI_UpdateAIList(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (UpdateAIList, FalconEvent::SimThread, senderid, target)
{
	dataBlock.gameID=FalconNullId;
	dataBlock.count=0;
	dataBlock.size=0;
	dataBlock.data=NULL;
}

UI_UpdateAIList::~UI_UpdateAIList(void)
{
	if(dataBlock.data)
		delete dataBlock.data;
}

int UI_UpdateAIList::Size (void)
	{
    ShiAssert(datablock.size >= 0);
	return FalconEvent::Size() +
		sizeof(VU_ID) +
		sizeof(ushort) + 
		sizeof(ushort) +		
		dataBlock.size;
	}

int UI_UpdateAIList::Decode (VU_BYTE **buf, int length)
	{
	int size = 0;

	memcpy (&dataBlock.gameID, *buf, sizeof(VU_ID));			*buf += sizeof(VU_ID);			size += sizeof(VU_ID);
	memcpy (&dataBlock.count, *buf, sizeof(ushort));			*buf += sizeof(ushort);			size += sizeof(ushort);		
	memcpy (&dataBlock.size, *buf, sizeof(ushort));				*buf += sizeof(ushort);			size += sizeof(ushort);		
    ShiAssert(datablock.size >= 0);
	dataBlock.data = new uchar[dataBlock.size];
	memcpy (dataBlock.data, *buf, dataBlock.size);				*buf += dataBlock.size;			size += dataBlock.size;		
	size += FalconEvent::Decode (buf, length);

	ShiAssert (size == Size());

	return size;
	}

int UI_UpdateAIList::Encode (VU_BYTE **buf)
	{
	int size = 0;
    ShiAssert(datablock.size >= 0);

	memcpy (*buf, &dataBlock.gameID, sizeof(VU_ID));			*buf += sizeof(VU_ID);	size += sizeof(VU_ID);
	memcpy (*buf, &dataBlock.count, sizeof(ushort));			*buf += sizeof(ushort); size += sizeof(ushort);
	memcpy (*buf, &dataBlock.size, sizeof(ushort));				*buf += sizeof(ushort);	size += sizeof(ushort);		
	memcpy (*buf, dataBlock.data, dataBlock.size);				*buf += dataBlock.size;	size += dataBlock.size;		
	size += FalconEvent::Encode (buf);

	ShiAssert (size == Size());

	return size;
	}

int UI_UpdateAIList::Process(uchar autodisp)
{
	int i;
	long ID;
	DFAITRANSFER *list;

	if(gCommsMgr && dataBlock.data && dataBlock.count)
	{
		if(gCommsMgr->GetTargetGameID() != dataBlock.gameID) // We don't want it anymore
			return(0);

		if (!FalconLocalGame)
			gCommsMgr->RemoveAIList();

		list=(DFAITRANSFER *)dataBlock.data;
		for(i=0;i<dataBlock.count;i++)
		{
			ID=list[i].Team << 24;
			ID |= list[i].Callsign << 16;
			ID |= list[i].Flight << 8;
			ID |= list[i].Slot;

			if(!gCommsMgr->FindAIAircraft(ID)) 
				gCommsMgr->AddAI(list[i].UI_ID,list[i].Team,list[i].Callsign,list[i].Flight,list[i].Slot,list[i].Skill,list[i].Deaths,list[i].TeamKills,list[i].Score);
		}
	}
	return 0;
}

