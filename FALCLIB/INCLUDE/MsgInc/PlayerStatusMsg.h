/*
 * Machine Generated include file for message "Player Enter/Exit Sim".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 23-December-1997 at 17:45:47
 * Generated from file EVENTS.XLS by Peter
 */

#ifndef _PLAYERSTATUSMSG_H
#define _PLAYERSTATUSMSG_H

/*
 * Required Include Files
 */
#include "F4vu.h"
#include "mission.h"
#pragma pack (1)

/*
 * State defines
 */

#define PSM_STATE_JOINED_GAME		1		// Player joined the game (not entered sim)
#define PSM_STATE_ENTERED_SIM		2		// Player has entered the arena/sim
#define PSM_STATE_LEFT_GAME			3
#define PSM_STATE_LEFT_SIM			4
#define PSM_STATE_TRANSFERED		5		// Player transfered between entities

/*
 * Message Type Player Enter/Exit Sim
 */
class FalconPlayerStatusMessage : public FalconEvent
{
   public:
      FalconPlayerStatusMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      FalconPlayerStatusMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~FalconPlayerStatusMessage(void);
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
			VU_ID  playerID;
			VU_ID  oldID;
			_TCHAR callsign[20];
			ushort campID; // flight # in Dogfight
			uchar  vehicleID;
			uchar  pilotID; // Slot # in Dogfight
			uchar  side;
			int   state;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};
#pragma pack ()

#endif
