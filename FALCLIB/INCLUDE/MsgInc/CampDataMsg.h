/*
 * Machine Generated include file for message "Camp Data Message".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 30-September-1997 at 16:41:13
 * Generated from file EVENTS.XLS by MicroProse
 */

#ifndef _CAMPDATAMSG_H
#define _CAMPDATAMSG_H

/*
 * Required Include Files
 */
#include "F4vu.h"
#include "mission.h"

#pragma pack (1)

/*
 * Message Type Camp Data Message
 */
class FalconCampDataMessage : public FalconEvent
{
   public:
      enum CampMsgType {
		 campDeaggregateStatusChangeData,
         campPriorityData,
		 campOrdersData,
		 };

      FalconCampDataMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      FalconCampDataMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~FalconCampDataMessage(void);
      int Size (void);
      int Decode (VU_BYTE **buf, int length);
      int Encode (VU_BYTE **buf);

      class DATA_BLOCK
      {
         public:
            unsigned int type;
            ushort size;
			uchar* data;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};

#pragma pack ()

#endif
