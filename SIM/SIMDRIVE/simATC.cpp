#include "stdhdr.h"
#include "simdrive.h"
#include "atcbrain.h"
#include "falclist.h"
#include "objectiv.h"
#include "camplib.h"

void SimulationDriver::UpdateATC(void)
{
	
ObjectiveClass* curObj;
//VuListIterator findWalker (ATCBrain::atcList);
VuListIterator findWalker (atcList);

   if (nextATCTime < SimLibElapsedTime)
   {
      curObj = (ObjectiveClass*)findWalker.GetFirst();
      while (curObj)
      {
         if (curObj->IsLocal() && curObj->brain)
         {
            curObj->brain->Exec();
         }
         curObj = (ObjectiveClass*)findWalker.GetNext();
      }

      nextATCTime = SimLibElapsedTime + (CampaignSeconds/4);
   }
   
}