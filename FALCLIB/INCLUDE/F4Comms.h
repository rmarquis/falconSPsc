#ifndef _F4COMMS_STUFF_H
#define _F4COMMS_STUFF_H

#include "vusessn.h"

extern ComAPIHandle vuxComHandle;
extern ComAPIHandle tcpListenHandle;

enum FalconConnectionTypes {
  FCT_NoConnection=0,
  FCT_ModemToModem,						// Modem to modem only
  FCT_NullModem,
  FCT_LAN,								// LAN or simulated LAN (kali)
  FCT_WAN,								// Internet or modem to ISP
  FCT_Server,							// Internet,modem or LAN to server
  FCT_TEN,
  FCT_JetNet,
};

// ========================================================================
// Message sizing variables
// ========================================================================

// Ideal packet size we'll send over the wire;
extern int F4CommsIdealPacketSize;
// Corrisponding content sizes
extern int F4CommsIdealTCPPacketSize;
extern int F4CommsIdealUDPPacketSize;
// Corrisponding messages sizes
extern int F4CommsMaxTCPMessageSize;
extern int F4CommsMaxUDPMessageSize;
// Maximum sized message vu can accept
extern int F4VuMaxTCPMessageSize;
extern int F4VuMaxUDPMessageSize;
// Maximum sized packet vu will pack messages into
extern int F4VuMaxTCPPackSize;
extern int F4VuMaxUDPPackSize;
// debug bandwidth limiters
extern int F4CommsBandwidth;
extern int F4CommsLatency;
extern int F4CommsDropInterval;
extern int F4CommsSwapInterval;
extern int F4SessionUpdateTime;
extern int F4SessionAliveTimeout;
extern int F4LatencyPercentage;

// ========================================================================
// Some defines 
// ========================================================================

// Protocols available to Falcon
#define FCP_UDP_AVAILABLE		0x01	// We have UDP available
#define FCP_TCP_AVAILABLE		0x02	// We have TCP available
#define FCP_SERIAL_AVAILABLE	0x04	// We have a serial (via modem or null modem) connection
#define FCP_MULTICAST_AVAILABLE	0x08	// True multicast is available
#define FCP_RUDP_AVAILABLE		0x10	// We have RUDP available

// Virtual connection types available to Falcon
#define FCT_PTOP_AVAILABLE		0x01	// We can send point to point messages to multiple machines
#define FCT_BCAST_AVAILABLE		0x02	// We can send broadcast (to world) messages
#define FCT_SERVER_AVAILABLE	0x04	// We are connecting to an exploder server
#define FCT_SERIAL_AVAILABLE	0x08	// We are connected via modem or null modem to one other machine

// Error codes returned from InitCommsStuff()
#define F4COMMS_CONNECTED                        1
#define F4COMMS_PENDING							 2		// Seems successfull, but we're waiting for a connection
#define	F4COMMS_ERROR_TCP_NOT_AVAILABLE			-1
#define F4COMMS_ERROR_UDP_NOT_AVAILABLE			-2
#define F4COMMS_ERROR_MULTICAST_NOT_AVAILABLE	-3
#define F4COMMS_ERROR_FAILED_TO_CREATE_GAME		-4
#define F4COMMS_ERROR_COULDNT_CONNECT_TO_SERVER	-5

class ComDataClass;

// ========================================================================
// Function prototypes
// ========================================================================

extern int InitCommsStuff (ComDataClass *comData);

extern int EndCommsStuff (void);

extern BOOL SessionManagerProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);

extern void SetupMessageSizes (int protocol);

extern void ResyncTimes (int force);

#endif
