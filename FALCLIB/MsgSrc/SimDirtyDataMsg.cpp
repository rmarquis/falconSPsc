/*
 * Machine Generated source file for message "Sim Dirty Data".
 * NOTE: The functions here must be completed by hand.
 * Generated on 17-November-1998 at 20:52:31
 * Generated from file EVENTS.XLS by Robin Heydon
 */

#include "MsgInc\SimDirtyDataMsg.h"
#include "mesg.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

SimDirtyData::SimDirtyData(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (SimDirtyDataMsg, FalconEvent::SimThread, entityId, target, loopback)
{
	dataBlock.data = NULL;
	dataBlock.size = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

SimDirtyData::SimDirtyData(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (SimDirtyDataMsg, FalconEvent::SimThread, senderid, target)
{
	dataBlock.data = NULL;
	dataBlock.size = 0;
	type;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

SimDirtyData::~SimDirtyData(void)
{
	if (dataBlock.data)
	{
		delete dataBlock.data;
	}

	dataBlock.data = NULL;
	dataBlock.size = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int SimDirtyData::Size (void)
{
    ShiAssert(dataBlock.size >= 0);
	return
	(
		FalconEvent::Size() +
		sizeof(ushort) +		
		dataBlock.size
	);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int SimDirtyData::Decode (VU_BYTE **buf, int length)
{
	int
		size;

	size = FalconEvent::Decode (buf, length);
	memcpy (&dataBlock.size, *buf, sizeof(ushort));				*buf += sizeof(ushort);			size += sizeof(ushort);		
    ShiAssert(dataBlock.size >= 0);
	dataBlock.data = new uchar[dataBlock.size];
	memcpy (dataBlock.data, *buf, dataBlock.size);				*buf += dataBlock.size;			size += dataBlock.size;		

	ShiAssert (size == Size());

	return size;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int SimDirtyData::Encode (VU_BYTE **buf)
{
	int
		size;

    ShiAssert(dataBlock.size >= 0);
	size = FalconEvent::Encode (buf);
	memcpy (*buf, &dataBlock.size, sizeof(ushort));				*buf += sizeof(ushort);			size += sizeof(ushort);		
	memcpy (*buf, dataBlock.data, dataBlock.size);				*buf += dataBlock.size;			size += dataBlock.size;		

	ShiAssert (size == Size());

	return size;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int SimDirtyData::Process(uchar autodisp)
{
	FalconEntity
		*ent;

	unsigned char
		*data;
	
	ent = (FalconEntity*) vuDatabase->Find (EntityId ());

	if (!ent || autodisp)
		return 0;

	// Only accept data on remote entities
	if (!ent->IsLocal())
	{
		data = dataBlock.data;

		ent->DecodeDirty (&data);
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
