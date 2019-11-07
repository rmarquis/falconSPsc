/*
 * Machine Generated include file for message "Air AI Mode Change".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 01-April-1997 at 18:57:02
 * Generated from file EVENTS.XLS by Kevin Klemmick
 */

#ifndef _AIRAIMODECHANGE_H
#define _AIRAIMODECHANGE_H

/*
 * Required Include Files
 */
#include "F4vu.h"
#include "mission.h"

#pragma pack (1)

/*
 * Message Type Air AI Mode Change
 */
class AirAIModeMsg : public FalconEvent
{
   public:
      AirAIModeMsg(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      AirAIModeMsg(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~AirAIModeMsg(void);
      int Size (void) { return sizeof(dataBlock) + FalconEvent::Size();};
      int Decode (VU_BYTE **buf, int length)
         {
         int size;

            size = FalconEvent::Decode (buf, length);
            memcpy (&dataBlock, *buf, sizeof (dataBlock));
            *buf += sizeof (dataBlock);
            return size;
         };
      int Encode (VU_BYTE **buf)
         {
         int size;

            size = FalconEvent::Encode (buf);
            memcpy (*buf, &dataBlock, sizeof (dataBlock));
            *buf += sizeof (dataBlock);
            return size;
         };
      class DATA_BLOCK
      {
         public:

            VU_ID whoDidIt;
            int newMode;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};

#pragma pack ()

#endif
