/*
 * Machine Generated include file for message "Weather Message".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 01-April-1997 at 18:57:03
 * Generated from file EVENTS.XLS by Kevin Klemmick
 */

#ifndef _WEATHERMSG_H
#define _WEATHERMSG_H

/*
 * Required Include Files
 */
#include "F4vu.h"
#include "falcmesg.h"
#include "mission.h"
#pragma pack (1)

/*
 * Message Type Weather Message
 */
class FalconWeatherMessage : public FalconEvent
{
   public:
      FalconWeatherMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      FalconWeatherMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~FalconWeatherMessage(void);
      int Size (void);
      int Decode (VU_BYTE **buf, int length);
      int Encode (VU_BYTE **buf);

      class DATA_BLOCK
      {
         public:
            short dataSize;
			short seed;
            uchar updateSide;
            uchar updateType;
            uchar* cloudData;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};
#pragma pack ()

#endif
