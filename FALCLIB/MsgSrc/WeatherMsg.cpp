/*
 * Machine Generated source file for message "Weather Message".
 * NOTE: The functions here must be completed by hand.
 * Generated on 06-February-1997 at 12:20:23
 * Generated from file EVENTS.XLS by Leon Rosenshein
 */

#include "MsgInc\WeatherMsg.h"
#include "mesg.h"
#include "Weather.h"
#include "CmpClass.h"

FalconWeatherMessage::FalconWeatherMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (WeatherMsg, FalconEvent::CampaignThread, entityId, target, loopback)
	{
	dataBlock.cloudData = NULL;
	dataBlock.dataSize = -1;
	}

FalconWeatherMessage::FalconWeatherMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (WeatherMsg, FalconEvent::CampaignThread, senderid, target)
	{
	dataBlock.cloudData = NULL;
	dataBlock.dataSize = -1;
	type;
	}

FalconWeatherMessage::~FalconWeatherMessage(void)
	{
	delete dataBlock.cloudData;
	}

int FalconWeatherMessage::Size (void) 
	{ 
	ShiAssert ( dataBlock.dataSize >= 0 );

	return sizeof(dataBlock) + dataBlock.dataSize + FalconEvent::Size();
	}

int FalconWeatherMessage::Decode (VU_BYTE **buf, int length)
	{
	int size = 0;
	long start = (long) *buf;

	ShiAssert ( dataBlock.dataSize >= 0 );
	memcpy (&dataBlock, *buf, sizeof(dataBlock));			*buf += sizeof(dataBlock);		size += sizeof (dataBlock);
	dataBlock.cloudData = new VU_BYTE[dataBlock.dataSize];
	memcpy (dataBlock.cloudData, *buf, dataBlock.dataSize);	*buf += dataBlock.dataSize;		size += dataBlock.dataSize;
	size += FalconEvent::Decode (buf, length);

	ShiAssert (size == (long) *buf - start);

	return size;
	}

int FalconWeatherMessage::Encode (VU_BYTE **buf)
	{
	int size = 0;

	ShiAssert ( dataBlock.dataSize >= 0 );
	memcpy (*buf, &dataBlock, sizeof(dataBlock));			*buf += sizeof(dataBlock);		size += sizeof (dataBlock);
	memcpy (*buf, dataBlock.cloudData, dataBlock.dataSize);	*buf += dataBlock.dataSize;		size += dataBlock.dataSize;
	size += FalconEvent::Encode (buf);

	ShiAssert (size == Size());

	return size;
	}

int FalconWeatherMessage::Process(uchar autodisp)
	{
	if (autodisp || !TheCampaign.IsPreLoaded())
		return -1;

	((WeatherClass*)TheWeather)->ReceiveWeather(this);
	return 0;
	}

