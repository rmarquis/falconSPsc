/*
 * Machine Generated include file for message "Divert Message".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 28-August-1997 at 17:24:21
 * Generated from file EVENTS.XLS by MicroProse
 */

#ifndef _DIVERTMSG_H
#define _DIVERTMSG_H

/*
 * Required Include Files
 */
#include "F4vu.h"
#include "mission.h"

// ==============================
// Defines
// ==============================

#define DIVERT_NO_DIVERT		0
#define DIVERT_WAIT_FOR_REPLY	1
#define DIVERT_REPLY_NO			2
#define DIVERT_REPLY_YES		3

#define DIVERT_DENIGNED			-1			// AWACS refused to divert the flight
#define DIVERT_CANCLED			-2			// AWACS canceled the divert (enemy aborted or killed)
#define DIVERT_SUCCEEDED		-3			// AWACS letting you know the divert is over (enemy dead)

#pragma pack (1)

/*
 * Message Type Divert Message
 */
class FalconDivertMessage : public FalconEvent
{
   public:
      FalconDivertMessage(void);
      FalconDivertMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      FalconDivertMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~FalconDivertMessage(void);
      int Size (void) { return sizeof(dataBlock) + FalconEvent::Size();};
      int Decode (VU_BYTE **buf, int length)
         {
         int size;

            size = FalconEvent::Decode (buf, length);
            memcpy (&dataBlock, *buf, sizeof (dataBlock));
            *buf += sizeof (dataBlock);
            size += sizeof (dataBlock);
            return size;
         };
      int Encode (VU_BYTE **buf)
         {
         int size;

            size = FalconEvent::Encode (buf);
            memcpy (*buf, &dataBlock, sizeof (dataBlock));
            *buf += sizeof (dataBlock);
            size += sizeof (dataBlock);
            return size;
         };
      class DATA_BLOCK
      {
         public:
			CampaignTime	tot;
			ulong			flags;
			VU_ID			targetID;
			VU_ID			requesterID;
			GridIndex		tx,ty;
			uchar			mission;
			uchar			priority;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};
#pragma pack ()

extern int CheckDivertStatus (int reply);

extern void ApplyDivert (Flight flight, FalconDivertMessage *fdm);

extern void PlayDivertRadioCalls (CampEntity target, int mission, Flight flight, int broadcast);

#endif
