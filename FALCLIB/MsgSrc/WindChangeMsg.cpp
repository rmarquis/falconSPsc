/* Machine Generated source file for message "Wind Change Message".
 * NOTE: The functions here must be completed by hand.
 * Generated on 21-October-1996 at 19:43:18
 * Generated from file EVENTS.XLS by KEVINK
 */

#include "MsgInc\WindChangeMsg.h"
#include "mesg.h"
#include "Weather.h"

FalconWindChangeMessage::FalconWindChangeMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (WindChangeMsg, FalconEvent::CampaignThread, entityId, target, loopback)
	{
	// Your Code Goes Here
	}

FalconWindChangeMessage::FalconWindChangeMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (WindChangeMsg, FalconEvent::CampaignThread, senderid, target)
	{
	// Your Code Goes Here
	type;
	}

FalconWindChangeMessage::~FalconWindChangeMessage(void)
	{
	// Your Code Goes Here
	}

int FalconWindChangeMessage::Process(uchar autodisp)
	{
	if (autodisp)
		return 0;

	if (!TheCampaign.IsMaster())
		{
		((WeatherClass*)TheWeather)->LastChange = vuxGameTime;
		((WeatherClass*)TheWeather)->Temp = dataBlock.temp ;
		((WeatherClass*)TheWeather)->WindHeading = dataBlock.windHeading;
		((WeatherClass*)TheWeather)->WindSpeed = dataBlock.windSpeed;
		TheWeather->xOffset = dataBlock.xOffset;
		TheWeather->yOffset = dataBlock.yOffset;
		}
	return 0;
	}
