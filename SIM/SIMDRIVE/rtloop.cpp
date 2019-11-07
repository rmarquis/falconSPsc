#include <windows.h>
#include "stdhdr.h"
#include "simdrive.h"
#include "falcmesg.h"
#include "camp2sim.h"
#include "simbase.h"
#include "f4thread.h"
#include "f4error.h"
#include "otwdrive.h"
#include "simio.h"
#include "sinput.h"
#include "ThreadMgr.h"
#include "falcsess.h"
#include "F4Comms.h"

extern F4CSECTIONHANDLE* campCritical;
extern int EndFlightFlag;
#define MAJOR_FRAME_RESOLUTION		50

HANDLE hLoopEvent;

extern FalconPrivateList	*DanglingSessionsList;
extern int UpdateDanglingSessions (void);

//void RealTimeFunction (unsigned long longVal, void* ptr)
void RealTimeFunction (unsigned long, void*)
{
	static unsigned long
		update_time = 0,
		send_time = 0;

	// Check to see if some time events have occured (compression ratio change, etc)
	if ((vuxRealTime > send_time) && (FalconLocalGame))
	{
		ResyncTimes(FALSE);
		send_time = vuxRealTime + RESYNC_TIME;
	}
	
	// Run VU Thread and handle messages
	if (vuxRealTime > update_time)
	{
		F4EnterCriticalSection(campCritical);

		gMainThread->Update();

		F4LeaveCriticalSection(campCritical);
		
		if (FalconLocalSession->GetFlyState () == FLYSTATE_FLYING)
		{
			update_time = vuxRealTime + 10;		// 100 Hz - in Sim
		}
		else
		{
			update_time = vuxRealTime + 20;		// 50 Hz - in UI
		}
	}

	// KCK: handle messages from all of our 'dangling' sessions
	if (DanglingSessionsList)
		UpdateDanglingSessions();

	//LRKLUDGE to get out on ground impact
	if (EndFlightFlag)
	{
		EndFlightFlag = FALSE;
		OTWDriver.EndFlight();
	}
}

