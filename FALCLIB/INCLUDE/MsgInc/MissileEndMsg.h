/*
 * Machine Generated include file for message "Missile Endgame".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 24-July-1997 at 18:35:47
 * Generated from file EVENTS.XLS by Kevin Klemmick
 */

#ifndef _MISSILEENDMSG_H
#define _MISSILEENDMSG_H

/*
 * Required Include Files
 */
#include "F4vu.h"
#include "mission.h"
#include "falcmesg.h"
#pragma pack (1)

/*
 * Message Type Missile Endgame
 */
class FalconMissileEndMessage : public FalconEvent
{
   public:
	   // note: bomb endcodes should FOLLOW missile
	   // endcodes
      enum MissileEndCode {
         NotDone,
         MissileKill,
         MinTime,
		 ArmingDelay, // added 2002-03-28 MN
         Missed,
         ExceedFOV,
         ExceedGimbal,
         GroundImpact,
         MinSpeed,
         ExceedTime,
		 FeatureImpact,
	     BombImpact};

      FalconMissileEndMessage(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      FalconMissileEndMessage(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~FalconMissileEndMessage(void);
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
            VU_ID dEntityID;
            ushort dCampID;
            uchar dPilotID;
            char dCampSlot;
            ushort dIndex;
            uchar dSide;
            VU_ID fEntityID;
            ushort fCampID;
            uchar fPilotID;
            char fCampSlot;
            ushort fIndex;
            uchar fSide;
            unsigned int endCode;
            VU_ID fWeaponUID;
			float x;
			float y;
			float z;
			float xDelta;
			float yDelta;
			float zDelta;
			char groundType;
			ushort wIndex;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};
#pragma pack ()

#endif
