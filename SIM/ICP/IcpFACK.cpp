#include "stdhdr.h"
#include "aircrft.h"
#include "fack.h"
#include "icp.h"
#include "simdrive.h"

//MI for ICP stuff
extern bool g_bRealisticAvionics;

void ICPClass::ExecFACKMode(void) {
	
	if(!g_bRealisticAvionics)
	{
		//MI original Code

	   FaultClass::str_FNames	faultNames;
		int			faultCount;

		if(!SimDriver.playerEntity) {
			return;
		}

		faultCount	= SimDriver.playerEntity->mFaults->GetFFaultCount();

		if(mUpdateFlags & FACK_UPDATE || (!(mUpdateFlags & FACK_UPDATE) && faultCount)) {

			mUpdateFlags &= !FACK_UPDATE;

			if(!faultCount) {

				sprintf(mpLine1, "      NO FAULTS");
				sprintf(mpLine2, "");
				sprintf(mpLine3, "      ALL SYS OK");
			}
			else {

				SimDriver.playerEntity->mFaults->GetFaultNames((FaultClass::type_FSubSystem)mFaultNum, mFaultFunc, &faultNames);

				sprintf(mpLine1, "        FAULT");
				sprintf(mpLine2, "");
				sprintf(mpLine3, "   %4s %4s %4s", faultNames.elpFSubSystemNames,
															faultNames.elpFFunctionNames,
															faultNames.elpFSeverityNames);
			}
		}
	}
	else
	{
		if(!SimDriver.playerEntity) 
		{
			return;
		}

		if(SimDriver.playerEntity->mFaults->GetFFaultCount() <= 0)
		{
		    PflNoFaults();
		}
		else 
		{
		    PflFault((FaultClass::type_FSubSystem)mFaultNum, (FaultClass::type_FFunction)mFaultFunc);
		}
	}
}

void ICPClass::ExecPfl()
{
    if ((mUpdateFlags & FACK_UPDATE) == 0) // nothing to update;
    ClearPFLLines(); // reset the display
    mUpdateFlags &= ~FACK_UPDATE; // we'll have updated.
    if (m_FaultDisplay == false || !SimDriver.playerEntity || !SimDriver.playerEntity->mFaults)
	return; // nothing to show

    if (SimDriver.playerEntity->mFaults->GetFFaultCount() <= 0) {
	PflNoFaults();
    }
    else {
	if (m_function == FaultClass::nofault)
		SimDriver.playerEntity->mFaults->GetFirstFault(&m_subsystem, &m_function);
	PflFault(m_subsystem, m_function);
	} 
}

void ICPClass::PflNoFaults()
{
    //Line1
    FillPFLMatrix(0,10, "NO FAULTS");
    //Line3
    FillPFLMatrix(2,9, "ALL SYS OK");
}

void ICPClass::PflFault(FaultClass::type_FSubSystem sys, int func)
{
    char tempstr[40];
    FaultClass::str_FNames	faultNames;

    SimDriver.playerEntity->mFaults->GetFaultNames(sys, func, &faultNames);
    //Line1
    FillPFLMatrix(0,12,"FAULT");
    //Line3
    //fix (better hack) to prevent strange stuff beeing written onto the PFL
    sprintf(tempstr, "%4s %4s %4s", 
	faultNames.elpFSubSystemNames,
	faultNames.elpFFunctionNames,
	faultNames.elpFSeverityNames);

    FillPFLMatrix(2,((25-strlen(tempstr))/2), tempstr);
}

void ICPClass::PNUpdateFACKMode(int button, int) {

	int faultIdx;
   int failedFuncs;
   int funcIdx;
   int testFunc;

	if(!SimDriver.playerEntity) {
		return;
	}

	if (button == PREV_BUTTON && SimDriver.playerEntity->mFaults->GetFFaultCount() >= 1) {
		
		faultIdx = mFaultNum;
      failedFuncs = SimDriver.playerEntity->mFaults->GetFault((FaultClass::type_FSubSystem) faultIdx);

      // previous failures on the System?
      testFunc = mFaultFunc - 1;
      if (mFaultFunc && (failedFuncs & ((1 << testFunc)- 1)) > 0)
      {
         mFaultFunc -= 2;
         funcIdx = (1 << mFaultFunc);
         while ((failedFuncs & funcIdx) == 0)
         {
            mFaultFunc --;
            funcIdx = funcIdx >> 1;

         }
      }
      else
      {
         // Find the previous sub-subsystem
		   do {
			   faultIdx--;
			   if(faultIdx < 0) {
				   faultIdx = FaultClass::NumFaultListSubSystems - 1;
			   }
			   failedFuncs = SimDriver.playerEntity->mFaults->GetFault((FaultClass::type_FSubSystem) faultIdx);
		   }	   
		   while(!failedFuncs && faultIdx != mFaultNum);
		   

         // Find highest failed sub-system
         funcIdx = (1 << 31);
         mFaultFunc = 31;
         while ((failedFuncs & funcIdx) == 0)
         {
            mFaultFunc --;
            funcIdx = funcIdx >> 1;
         }
      }

		mFaultNum = faultIdx;
      mFaultFunc ++;
	}
	else if(button == NEXT_BUTTON && SimDriver.playerEntity->mFaults->GetFFaultCount() >= 1) {

		faultIdx = mFaultNum;
      failedFuncs = SimDriver.playerEntity->mFaults->GetFault((FaultClass::type_FSubSystem) faultIdx);

      // next failures on the System?
      if ((failedFuncs & ~((1 << mFaultFunc)- 1)) > 0)
      {
         funcIdx = (1 << mFaultFunc);
         while ((failedFuncs & funcIdx) == 0)
         {
            mFaultFunc ++;
            funcIdx = funcIdx << 1;
         }
      }
      else
      {
		  do
		  {
			  faultIdx++;
			  if(faultIdx >= FaultClass::NumFaultListSubSystems)
			  {
				  faultIdx = 0;
			  }
			  failedFuncs = SimDriver.playerEntity->mFaults->GetFault((FaultClass::type_FSubSystem) faultIdx);
		  }		  
		  while(!failedFuncs && faultIdx != mFaultNum);
			  
		  // Find lowest failed sub-system
		  funcIdx = 1;
		  mFaultFunc = 0;
		  while ((failedFuncs & funcIdx) == 0)
		  {
			  mFaultFunc ++;
			  funcIdx = funcIdx << 1;
		  }
      }

		mFaultNum = faultIdx;
      mFaultFunc ++;
	}

	mUpdateFlags |= FACK_UPDATE;
}
