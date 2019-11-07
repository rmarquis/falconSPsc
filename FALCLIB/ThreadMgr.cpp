#include    <stdio.h>
#include    <stdlib.h>
#include    "ThreadMgr.h"
#include    "debuggr.h"
#include    "F4Thread.h"
#include    "Falclib.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int ThreadManager::initialized = FALSE;
HANDLE ThreadManager::campaign_wait_event;
HANDLE ThreadManager::sim_wait_event;
ThreadInfo ThreadManager::campaign_thread;
ThreadInfo ThreadManager::sim_thread;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ThreadManager::setup (void)
{
    if (initialized)
        return;

    initialized = TRUE;

	campaign_wait_event = CreateEvent (0,0,0,0);
	sim_wait_event = CreateEvent (0,0,0,0);

    memset (&campaign_thread, 0, sizeof (campaign_thread));
    memset (&sim_thread, 0, sizeof (sim_thread));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ThreadManager::start_campaign_thread (UFUNCTION function)
{
    ShiAssert( campaign_thread.handle == NULL );
    
	campaign_thread.status |= THREAD_STATUS_ACTIVE;
    
	campaign_thread.handle = ( HANDLE ) CreateThread
	(
		NULL,
		0,
		(unsigned long (__stdcall *)(void*))function,
		0,
		0,
        &campaign_thread.id
	);

	ShiAssert (campaign_thread.handle);

	fast_campaign ();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ThreadManager::campaign_wait_for_sim (DWORD maxwait)
{
	ResetEvent (campaign_wait_event);

	WaitForSingleObject (campaign_wait_event, maxwait);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ThreadManager::sim_signal_campaign (void)
{
	SetEvent (campaign_wait_event);
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ThreadManager::sim_wait_for_campaign (DWORD maxwait)
{
	ResetEvent (sim_wait_event);

	WaitForSingleObject (sim_wait_event, maxwait);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ThreadManager::campaign_signal_sim (void)
{
	SetEvent (sim_wait_event);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ThreadManager::start_sim_thread (UFUNCTION function)
{
	ShiAssert (sim_thread.handle == NULL);
	
	sim_thread.status |= THREAD_STATUS_ACTIVE;

	sim_thread.handle = (HANDLE) CreateThread
	(
		NULL,
		0,
		(unsigned long (__stdcall *)(void*))function,
		0,
		0,
		&sim_thread.id
	);
	
	ShiAssert (sim_thread.handle);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ThreadManager::stop_campaign_thread (void)
{
    ShiAssert (campaign_thread.handle);

    campaign_thread.status &= ~THREAD_STATUS_ACTIVE;

    WaitForSingleObject (campaign_thread.handle, INFINITE);

    CloseHandle (campaign_thread.handle);

    campaign_thread.handle = NULL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ThreadManager::stop_sim_thread (void)
{
    ShiAssert (sim_thread.handle);

    sim_thread.status &= ~THREAD_STATUS_ACTIVE;

    WaitForSingleObject (sim_thread.handle, INFINITE);

    CloseHandle (sim_thread.handle);

    sim_thread.handle = NULL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ThreadManager::slow_campaign (void)
{
	SetThreadPriority (campaign_thread.handle, THREAD_PRIORITY_IDLE);
	// MonoPrint("Campaign set to SLOW.\n");
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ThreadManager::fast_campaign (void)
{
	SetThreadPriority (campaign_thread.handle, THREAD_PRIORITY_NORMAL);
	// MonoPrint("Campaign set to NORMAL.\n");
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ThreadManager::very_fast_campaign (void)
{
	SetThreadPriority (campaign_thread.handle, THREAD_PRIORITY_ABOVE_NORMAL);
	// MonoPrint("Campaign set to FAST.\n");
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
