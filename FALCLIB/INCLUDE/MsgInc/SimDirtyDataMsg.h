/*
 * Machine Generated include file for message "Sim Dirty Data".
 * NOTE: This file is read only. DO NOT ATTEMPT TO MODIFY IT BY HAND.
 * Generated on 17-November-1998 at 20:52:31
 * Generated from file EVENTS.XLS by Robin Heydon
 */

#ifndef _SIMDIRTYDATA_H
#define _SIMDIRTYDATA_H

/*
 * Required Include Files
 */
#include "F4vu.h"
#include "FalcMesg.h"
#include "mission.h"
#pragma pack (1)

/*
 * Message Type Sim Dirty Data
 */
class SimDirtyData : public FalconEvent
{
   public:
      SimDirtyData(VU_ID entityId, VuTargetEntity *target, VU_BOOL loopback=TRUE);
      SimDirtyData(VU_MSG_TYPE type, VU_ID senderid, VU_ID target);
      ~SimDirtyData(void);
	  int Size (void);
	  int Decode (VU_BYTE **, int);
	  int Encode (VU_BYTE **);
      class DATA_BLOCK
      {
         public:

            long size;
            uchar* data;
      } dataBlock;

   protected:
      int Process(uchar autodisp);
};
#pragma pack ()

#endif
