/*
 * Machine Generated source file for message "Request Team Info".
 * NOTE: The functions here must be completed by hand.
 * Generated on 01-April-1997 at 14:44:37
 * Generated from file EVENTS.XLS by Kevin Klemmick
 */

#include "MsgInc\RequestTeamInfoMsg.h"
#include "mesg.h"

FalconRequestTeamInfoMessage::FalconRequestTeamInfoMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback) : FalconEvent (RequestTeamInfoMsg, FalconEvent::CampaignThread, entityId, target, loopback)
{
   // Your Code Goes Here
}

FalconRequestTeamInfoMessage::FalconRequestTeamInfoMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target) : FalconEvent (RequestTeamInfoMsg, FalconEvent::CampaignThread, senderid, target)
{
   // Your Code Goes Here
}

FalconRequestTeamInfoMessage::~FalconRequestTeamInfoMessage(void)
{
   // Your Code Goes Here
}

int FalconRequestTeamInfoMessage::Process(void)
{
   // Your Code Goes Here
   return 0;
}

