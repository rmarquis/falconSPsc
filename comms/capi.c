///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#pragma optimize( "", off ) // JB 010718

#include <stdio.h>
#include <assert.h>
#include "capiopt.h"
#include "capi.h"
#include "capipriv.h"
#include "ws2init.h"
#include "wsprotos.h"
#include "tcp.h"
#include "debuggr.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/*
Bandwidth Bytes is the total bandwidth on this system
Bandwidth always send is the lower limit of bandwidth available when the message is always sent
Bandwidth used is the actual computed bandwidth used in the last second - more accurately the number of bytes we think are waiting in the network queue
Bandwidth time is the time when we last computed the bandwidth

  We decay bandwidth used by the bandwidth bytes / second.
*/

extern int g_nBWCheckDeltaTime; // 2002-04-12 MN late deagg fix
extern int g_nBWMaxDeltaTime;



int user_bandwidth_bytes = 1000;
int bandwidth_bytes = -1;
int bandwidth_used = 0;
int rudp_bandwidth_required = 0;
int rudp_bandwidth_used = 0;
int bandwidth_time = 0;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static struct sockaddr_in comBroadcastAddr, comRecvAddr;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern int ComDPLAYEnumProtocols(int *protocols, int maxprotocols);
DWProc_t CAPI_TimeStamp = NULL;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

unsigned long ComAPILastError = 0;

static void (*info_callback) (ComAPIHandle c, int send, int msgsize) = NULL;

// JB 010718 start
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static int init_cs = FALSE;
//static int cs_count = 0;
static CRITICAL_SECTION cs;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void enter_cs (void)
{
//	char
//		buffer[100];

	if (!init_cs)
	{
		InitializeCriticalSection (&cs);

		init_cs = TRUE;
	}

	EnterCriticalSection (&cs);

//	cs_count ++;
//	sprintf (buffer, "EnterCS %08x %d\n", GetCurrentThreadId (), cs_count);
//	OutputDebugString (buffer);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void leave_cs (void)
{
//	char
//		buffer[100];

	if (init_cs)
	{
//		cs_count --;
//		sprintf (buffer, "LeaveCS %08x %d\n", GetCurrentThreadId (), cs_count);
//		OutputDebugString (buffer);

		LeaveCriticalSection (&cs);
	}
}
// JB 010718 end

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ComAPIClose(ComAPIHandle c)
{
	enter_cs (); // JB 010718

	if (c)
	{
		(*c->close_func)(c);
	}

	leave_cs (); // JB 010718
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void use_bandwidth (int size)
{
	if (size > 0)
	{
		assert (size < bandwidth_bytes);

		if (size > bandwidth_bytes)
		{
			size = bandwidth_bytes;
		}

		bandwidth_used += size;

		if (bandwidth_bytes > user_bandwidth_bytes)
		{
			bandwidth_bytes = user_bandwidth_bytes;
		}
	}
	else
	{
		assert (size < 0);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// RUDP requires more bandwidth than is available - reduce UDP

void rudp_bandwidth (int size)
{
	rudp_bandwidth_required += 2 * size;
	
	if (rudp_bandwidth_required > bandwidth_bytes * 1 / 2)
	{
		rudp_bandwidth_required = bandwidth_bytes * 1 / 2;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int check_rudp_bandwidth (int size)
{
	int
		delta_time,
		time;
	
	if (bandwidth_bytes >= 0) /* Do we care about bandwidth */
	{
		if (size < 0)
		{
			return FALSE;
		}
		
		time = GetTickCount ();
		
		if (bandwidth_time != 0)
		{
			/* Decay the bandwidth used wrt. time */
			
			delta_time = time - bandwidth_time;

// 2002-04-12 MN fix When these functions are not called regularly, like in single player, 
// delta_time and subsequently bandwidth_used can get enourmously high. 
// Also in multiplayer this can be a problem, because of the signed nature of bandwidth_used, 
// which then can result in bandwidth_used values of more than 1 million !!!
// To fix that, we check if delta time is more than g_nCheckDeltaTime seconds, and reset
// bandwidthtime to current tickcount in that case and delta_time and bandwidth_used to 0 so 
// next cycle we will have a fine calculation of all variables :-)
// - or to a minimum value and let's calculate - don't know what is better yet, must be tested so,
// leave in both with config file switch. Also adapted in falcent.cpp

			if (g_nBWMaxDeltaTime)
			{
				delta_time = min(delta_time, g_nBWCheckDeltaTime);

			}
			else if (delta_time > g_nBWCheckDeltaTime)
			{
				delta_time = 0;
				bandwidth_time = time;
				bandwidth_used = 0;
				return FALSE;
			}
						
			if (delta_time > 100)
			{
				bandwidth_used -= delta_time * bandwidth_bytes / 1000;
				// if (bandwidth_used <0) bandwidth_used =0;//me123 is done below already
				rudp_bandwidth_required -= delta_time * bandwidth_bytes / 2000;
				
				if (bandwidth_used < 0)
				{
					bandwidth_used = 0;
				}
				
				if (rudp_bandwidth_required < bandwidth_bytes / 5)
				{
					rudp_bandwidth_required = bandwidth_bytes / 5;
				}
				
				bandwidth_time = time;
			}
		}
		else
		{
			bandwidth_used = 0;
			
			bandwidth_time = time;
		}
		
		/* Ok, we need to block is the bandwidth_used > minimum AND the next message overflow's the bandwidth */
		/* We always send if bandwidth_used is less than always send OR the next message fits into bandwidth */
		
		if (bandwidth_used + size > bandwidth_bytes)
		{
			//MonoPrint ("%d:%d\n", bandwidth_used, bandwidth_bytes);
			return FALSE;
		}
	}
	
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int check_bandwidth (int size)
{
	int
		delta_time,
		time;
	//MonoPrint ("****** CHECK BANDWIDTH size: %d ******", size);
	if (bandwidth_bytes >= 0) /* Do we care about bandwidth */
	{
		//MonoPrint("***** Entered bandwidth_bytes >= 0");
		if (size < 0)
		{
			return FALSE;
		}
		
		time = GetTickCount ();
		
		if (bandwidth_time != 0)
		{
			//MonoPrint("**** Bandwidth_time != 0 : %d ****", bandwidth_time);
			/* Decay the bandwidth used wrt. time */
			
			delta_time = time - bandwidth_time;
			//MonoPrint("**** DeltaTime: %d", delta_time);

// 2002-04-12 MN When these functions are not called regularly, like in single player, 
// delta_time and subsequently bandwidth_used can get enourmously high. Result was delayed deaggregation.
// Also in multiplayer this can be a problem, because of the signed nature of bandwidth_used, 
// which then can result in bandwidth_used values of more than *** 1 million ms ***, and nearly 
// never deaggregating objects !!!
// To fix that, we restrict delta time to a maximum value or set it to 0 and back out - not sure
// what is better, so make it configurable for now.

			if (g_nBWMaxDeltaTime)
			{
				delta_time = min(delta_time, g_nBWCheckDeltaTime);

			}
			else if (delta_time > g_nBWCheckDeltaTime)
			{
				delta_time = 0;
				bandwidth_time = time;
				bandwidth_used = 0;
				return FALSE;
			}
			

			if (delta_time > 100)
			{
//					MonoPrint("**** Bandwidth_Used before delta calculation: %d", bandwidth_used);
				bandwidth_used -= delta_time * bandwidth_bytes / 1000;
				// if (bandwidth_used <0) bandwidth_used =0;//me123 is done below already
				rudp_bandwidth_required -= delta_time * bandwidth_bytes / 2000;
				
/*				MonoPrint("*** bandwidth_bytes: %d",bandwidth_bytes);
				MonoPrint("*** bandwidth_used:  %d",bandwidth_used);
				MonoPrint("*** deltaTime:       %lu",delta_time);
				MonoPrint("*** rudp_bw_requir:  %d",rudp_bandwidth_required);
*/
				if (bandwidth_used < 0)
				{
					bandwidth_used = 0;
				}
				
				if (rudp_bandwidth_required < bandwidth_bytes / 5)
				{
					rudp_bandwidth_required = bandwidth_bytes / 5;
				}
				
				bandwidth_time = time;
			}
		}
		else
		{
			bandwidth_time = time;
			
			bandwidth_used = 0;
/*			MonoPrint("****** bandwidth_time was 0, has been set to current tickcount !!!");
			MonoPrint("****** bandwidth_used: 0");
			MonoPrint("****** bandwidth_bytes: %d",bandwidth_bytes);
			MonoPrint("****** rudp_bandwidth_required + size: %u", rudp_bandwidth_required+size);
*/

		}
		
		/* Ok, we need to block is the bandwidth_used > minimum AND the next message overflow's the bandwidth */
		/* We always send if bandwidth_used is less than always send OR the next message fits into bandwidth */
		
		if (bandwidth_used + rudp_bandwidth_required + size > bandwidth_bytes)
		{
			//MonoPrint("***** bw_used + rudp_requ+ size check FAILED *****");
			return FALSE;
		}
	}
	//MonoPrint("******* WE HAVE BANDWIDTH ******");
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int get_bandwidth_available (void)
{
	return bandwidth_used;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void cut_bandwidth (void)
{
	// Cut bandwidth by setting the bandwidth_used to the bandwidth_bytes
	// instead of reducing the bandwidth_bytes available.
	
	// bandwidth_bytes = bandwidth_bytes * 3 / 4;	// cut by 25%
	
	//MonoPrint ("Cut Bandwidth %d\n", bandwidth_bytes); // JB 010718

	bandwidth_used = bandwidth_bytes;
	
	bandwidth_time = GetTickCount ();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/* send data from a comms session */

int ComAPISend(ComAPIHandle c, int msgsize)
{
	int
		rc = 0;

	enter_cs ();
	
	//if (c && !IsBadReadPtr(c, sizeof(ComAPI))) // JB 010223 CTD
	if (c && !IsBadReadPtr(c, sizeof(ComAPI)) && !IsBadCodePtr((FARPROC) (*c->send_func))) // JB 010306 CTD
	{
	    assert(FALSE == IsBadReadPtr(c, sizeof *c));
	    assert(FALSE == IsBadCodePtr((FARPROC)c->send_func));
		if (info_callback)
		{
			info_callback (c, 1, msgsize);
		}
		
		if (c->send_func)
		{
			rc = (*c->send_func)(c, msgsize, FALSE);
		}
	}

	leave_cs ();

	return rc;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/* send data from a comms session */

int ComAPISendOOB(ComAPIHandle c, int msgsize, int posupd)
{
	int
		rc = 0;

	enter_cs (); // JB 010718

	if(c)
	{
	    assert(FALSE == IsBadReadPtr(c, sizeof *c));
	    assert(FALSE == IsBadCodePtr((FARPROC)c->send_func));
		if (info_callback)
		{
			info_callback (c, 1, msgsize);
		}
		
		if (c->send_func) // JB 010718
		{
			if (posupd)
			rc = (*c->send_func)(c, msgsize, 2);
			else
			rc = (*c->send_func)(c, msgsize, TRUE);
		}
	}

	leave_cs ();

	return rc;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int ComAPISendX(ComAPIHandle c, int msgsize,ComAPIHandle Xcom)
{
	int
		rc = 0;
	
	enter_cs ();

	if(c)
	{
	    assert(FALSE == IsBadReadPtr(c, sizeof *c));
	    assert(FALSE == IsBadCodePtr((FARPROC)c->sendX_func));
		if (info_callback)
		{
			info_callback (c, 2, msgsize);
		}
		
		rc = (*c->sendX_func)(c, msgsize,Xcom);
		
		if (rc > 0)
		{
			use_bandwidth (rc);
		}
	}

	leave_cs ();
	
	return rc;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/* recive data from a comms session */

int ComAPIGet(ComAPIHandle c)
{
	int
		size = 0;
	
	enter_cs ();
	
#ifdef _DEBUG
	if(c)
#else
	if(c && !IsBadReadPtr(c, sizeof(ComAPI)) && !IsBadCodePtr((FARPROC) (*c->recv_func))) // JB 010404 CTD
#endif
	{
	    assert(FALSE == IsBadReadPtr(c, sizeof *c));
	    assert(FALSE == IsBadCodePtr((FARPROC)c->recv_func));
		size = (*c->recv_func)(c);
		
		if (info_callback)
		{
			info_callback (c, 0, size);
		}		
	}

	leave_cs ();
	
	return size;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/* set the group to send and recieve data from */

void ComAPIGroupSet(ComAPIHandle c, int group)
{
	c;
	group;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/* get the local hosts unique id len */

int ComAPIHostIDLen(ComAPIHandle c)
{
	if(c)
    {
		return 4;
    }
	else
    {
		return 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/* get the local hosts unique id */

int ComAPIHostIDGet(ComAPIHandle c, char *buf)
{
	int
		ret_val = 0;
	
	enter_cs ();
	
	if(c)
	{
		assert(FALSE == IsBadReadPtr(c, sizeof *c));
		assert(FALSE == IsBadCodePtr((FARPROC)c->addr_func));
		ret_val = (*c->addr_func)(c, buf);
	}

	leave_cs ();
	
	return ret_val;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/* get the associated write buffer */

char *ComAPISendBufferGet(ComAPIHandle c)
{
	char
		*ret_val = 0;

	enter_cs ();

	if (c)
  {
		assert(FALSE == IsBadReadPtr(c, sizeof *c));
		assert(FALSE == IsBadCodePtr((FARPROC)c->send_buf_func));
		ret_val = (*c->send_buf_func)(c);
	}

	leave_cs ();

	return ret_val;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

char *ComAPIRecvBufferGet(ComAPIHandle c)
{
	char
		*ret_val = 0;

	enter_cs ();

#ifdef _DEBUG
	if (c)
#else
	if (c && !IsBadReadPtr(c, sizeof(ComAPI)) && !IsBadCodePtr((FARPROC) (*c->recv_buf_func))) // JB 010326 CTD
#endif
    {
	    assert(FALSE == IsBadReadPtr(c, sizeof *c));
	    assert(FALSE == IsBadCodePtr((FARPROC)c->send_buf_func));
		ret_val = (*c->recv_buf_func)(c);
    }
	
	leave_cs ();

	return ret_val;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/* get the current send+receive message count */

unsigned long ComAPIQuery(ComAPIHandle c, int querytype)
{
	unsigned long
		ret_val = 0;

	enter_cs ();

	if(c)
	{
	    assert(FALSE == IsBadReadPtr(c, sizeof *c));
	    assert(FALSE == IsBadCodePtr((FARPROC)c->query_func));
		ret_val = (*c->query_func)(c,querytype);
	}
	else
	{
		switch (querytype)
		{
			case COMAPI_TCP_HEADER_OVERHEAD:
			{
				ret_val = sizeof(tcpHeader) + 40;	// Size of underlying header.
				break;
			}

			case COMAPI_UDP_HEADER_OVERHEAD:
			{
				ret_val = sizeof(ComAPIHeader);
				break;
			}

			case COMAPI_RUDP_HEADER_OVERHEAD:
			{
				ret_val = MAX_RUDP_HEADER_SIZE;
				break;
			}
		}
	}

	leave_cs ();

	return ret_val;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int ComAPIEnumProtocols(int *protocols, int maxprotocols)
{
//	int i=0,size=0, numprotos=0;
	WSADATA wsaData;
	int ourprotos=0;
	
	if (InitWS2(&wsaData) == 0)
	{
		return 0;
	}
		
	if(maxprotocols >= 2)
	{
		*protocols = CAPI_TCP_PROTOCOL;
		ourprotos++;
		protocols++;
		*protocols = CAPI_UDP_PROTOCOL;
		ourprotos++;
	}
	
	/* Now use DPLAY to enumerate it's avaliable protocols */
	ourprotos += ComDPLAYEnumProtocols(protocols, maxprotocols - ourprotos);

	return ourprotos;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/* convert host ip address to string */

char *ComAPIinet_htoa(u_long ip)
{
	struct in_addr addr;
	
	if(CAPI_htonl == NULL)
	{
		return NULL;
	}

	addr.s_addr = CAPI_htonl(ip);
	
	if(CAPI_inet_ntoa == NULL)
	{
		return NULL;
	}

	return CAPI_inet_ntoa(addr);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/* convert net ipaddress to string */

char *ComAPIinet_ntoa(u_long ip)
{
	struct in_addr addr;
	
	addr.s_addr = ip;
	
	if(CAPI_inet_ntoa == NULL)
	{
		return NULL;
	}

	return CAPI_inet_ntoa(addr);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/* convert net ipaddress to string */

unsigned long ComAPIinet_haddr(char * IPAddress)
{
	unsigned long ipaddress;
	
	if(CAPI_inet_addr == NULL)
	{
		return 0;
	}

	ipaddress = CAPI_inet_addr(IPAddress);
	
	if(CAPI_ntohl == NULL)
	{
		return 0;
	}

	return CAPI_ntohl(ipaddress);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

unsigned long ComAPIGetLastError(void)
{
	return ComAPILastError;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ComAPISetTimeStampFunction(unsigned long (*TimeStamp)(void))
{
	CAPI_TimeStamp = TimeStamp;
    CAPI_TimeStamp ();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

unsigned long ComAPIGetTimeStamp(ComAPIHandle c)
{
	unsigned long
		ret_val = 0;

	enter_cs ();

	if (c)
    {
	    assert(FALSE == IsBadReadPtr(c, sizeof *c));
	    assert(FALSE == IsBadCodePtr((FARPROC)c->get_timestamp_func));
		ret_val = (c->get_timestamp_func)(c);
    }

	leave_cs ();

	return ret_val;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ComAPIRegisterInfoCallback (void (*func)(ComAPIHandle, int, int))
{
	info_callback = func;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ComAPISetBandwidth (int bytes)
{
	user_bandwidth_bytes = bytes;
	bandwidth_bytes = bytes;
	bandwidth_used = 0;
	bandwidth_time = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int ComAPIGetBandwidth (void)
{
	if (bandwidth_bytes > 0)
	{
		return bandwidth_bytes;
	}
	else
	{
		return 0;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int ComAPIInitComms(void)
{
	WSADATA wsaData;
	int ret=1;
	
	if(!WS2Connections)
	{
		ret = InitWS2(&wsaData);
		WS2Connections--;

		/* if No more connections then WSACleanup() */
		if (!WS2Connections)
		{
            CAPI_WSACleanup();
		}
		
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
