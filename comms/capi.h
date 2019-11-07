/* capi.h - Copyright (c) Fri Dec 06 17:26:36 1996,  Spectrum HoloByte, Inc.  All Rights Reserved */
/******************************************************************************************************
CONFIGURATION notes.
within:  capiopt.h
  #define LOAD_DLLS for expicit LoadLibrary() calls to be used for WS2_32.DLL and DPLAYX.DLL   and OLE32.DLL
  If not #defined  application must link with WS2_32.LIB and DPLAYX.LIB  and OLE32.LIB
  otherwise       these LIBs are not needed                  
*******************************************************************************************************/

#ifndef _CAPI_H_
#define _CAPI_H_
#ifdef __cplusplus
extern "C" {
#endif


#define CAPI_VERSION		1
#define CAPI_REVISION		4
#define CAPI_PATCH		    0
#define CAPI_REVISION_DATE	"5/27/98"
#define CAPI_PATCH_DATE	    "5/27/98"



/* Protocol values for ComAPIEnumProtocols */
#define CAPI_UNKNOWN_PROTOCOL	       0
#define CAPI_UDP_PROTOCOL	          1
#define CAPI_IP_MULTICAST_PROTOCOL   2
#define CAPI_SERIAL_PROTOCOL	       3
#define CAPI_TEN_PROTOCOL	          4
#define CAPI_TCP_PROTOCOL            5
#define CAPI_DPLAY_MODEM_PROTOCOL    6
#define CAPI_DPLAY_SERIAL_PROTOCOL   7
#define CAPI_DPLAY_TCP_PROTOCOL      8
#define CAPI_DPLAY_IPX_PROTOCOL      9
#define CAPI_RUDP_PROTOCOL			10
#define CAPI_GROUP_PROTOCOL         11
#define CAPI_LAST_PROTOCOL           CAPI_GROUP_PROTOCOL

#define CAPI_HOST 1
#define CAPI_JOIN 0

/* COMAPI Error codes  to return to application */
#define COMAPI_BAD_HEADER           -1  /* COMAPI message header not correct */
#define COMAPI_OUT_OF_SYNC          -2  /* data not quite as we expected */
#define COMAPI_OVERRUN_ERROR        -3  /* trying to read too much */
#define COMAPI_BAD_MESSAGE_SIZE     -4  /* internal syncing error */
#define COMAPI_CONNECTION_CLOSED    -5  /* remote connection closed gracefully */
#define COMAPI_MESSAGE_TOO_BIG      -6 
#define COMAPI_CONNECTION_PENDING   -7
#define COMAPI_WOULDBLOCK           -8
#define COMAPI_EMPTYGROUP           -9
#define COMAPI_PLAYER_LEFT          -10
#define COMAPI_NOTAGROUP            -11
#define COMAPI_CONNECTION_TIMEOUT   -12
#define COMAPI_HOSTID_ERROR         -13
#define COMAPI_WINSOCKDLL_ERROR     -14     /* WS2_32.DLL */
#define COMAPI_DPLAYDLL_ERROR       -15     /* DPLAYX.DLL */
#define COMAPI_OLE32DLL_ERROR       -16     /* OLE32.DLL   for DirectPLay */  
#define COMAPI_TENDLL_ERROR         -17

/* ComAPIQuery() query types     */
#define COMAPI_MESSAGECOUNT          1
#define COMAPI_RECV_WOULDBLOCKCOUNT  2   
#define COMAPI_SEND_WOULDBLOCKCOUNT  3   
#define COMAPI_CONNECTION_ADDRESS    4   
#define COMAPI_RECEIVE_SOCKET        5
#define COMAPI_SEND_SOCKET           6
#define COMAPI_RELIABLE              7
#define COMAPI_RECV_MESSAGECOUNT     8
#define COMAPI_SEND_MESSAGECOUNT     9
#define COMAPI_UDP_CACHE_SIZE        10
#define COMAPI_RUDP_CACHE_SIZE       12
#define COMAPI_MAX_BUFFER_SIZE       13    /* 0 = no MAXIMUM =  stream */
#define COMAPI_ACTUAL_BUFFER_SIZE    14
#define COMAPI_PROTOCOL              15
#define COMAPI_STATE                 16
#define COMAPI_DPLAY_PLAYERID        17
#define COMAPI_DPLAY_REMOTEPLAYERID  18
#define COMAPI_SENDER                19
#define COMAPI_TCP_HEADER_OVERHEAD   20
#define COMAPI_UDP_HEADER_OVERHEAD   21
#define COMAPI_RUDP_HEADER_OVERHEAD  22
#define COMAPI_PING_TIME             23
#define COMAPI_BYTES_PENDING         24

#define COMAPI_STATE_CONNECTION_PENDING 0       
#define COMAPI_STATE_CONNECTED          1       
#define COMAPI_STATE_ACCEPTED           2       



#define CAPI_THREAD_PRIORITY_ABOVE_NORMAL   1
#define CAPI_THREAD_PRIORITY_BELOW_NORMAL   2 
#define CAPI_THREAD_PRIORITY_HIGHEST        3 
#define CAPI_THREAD_PRIORITY_IDLE           4 
#define CAPI_THREAD_PRIORITY_LOWEST         5
#define CAPI_THREAD_PRIORITY_NORMAL         6
#define CAPI_THREAD_PRIORITY_TIME_CRITICAL  7

#define CAPI_DPLAY_NOT_GUARANTEED           0
#define CAPI_DPLAY_GUARANTEED               1

typedef struct comapihandle *ComAPIHandle;
/* Init comms  optional - done automatically by any open, but needed if calling
utility functions defore opening a handle*/
int ComAPIInitComms(void);
void ComAPISetName (ComAPIHandle, char *);

/* Enumerate avalible protocols */
int ComAPIEnumProtocols(int *protocols, int maxprotocols);

/* RUDP specific opens .. Reliable User Datagram Protocol*/
/* begin RUDP comms session */
ComAPIHandle ComRUDPOpen(char *name, int buffersize, char *gamename, int udpPort, unsigned long IPaddress, int idealpacketsize);

/* UDP specific opens ..*/
/* begin UDP comms session */
/* IPaddress == 0 means broadcast Send. otherwise PointToPoint Send */
ComAPIHandle ComUDPOpen(char *name, int buffersize, char *gamename, int udpPort, unsigned long IPaddress);
ComAPIHandle ComIPMulticastOpen(int buffersize, char *gamename, int mc_scope);


/* TCP  specific open .. */
/* begin a TCP connection as a listener to wait for connections */
ComAPIHandle ComTCPOpenListen (int buffersize, char *gamename, int tcpPort ,void (*AcceptCallback)(ComAPIHandle c, int ret));

/* begin a TCP connection to a targeted listening TCP listener */
ComAPIHandle ComTCPOpenConnect(int buffersize, char *gamename, int tcpPort, unsigned long IPaddress,void (*ConnectCallback)(ComAPIHandle c, int ret),int timeoutsecs );

/* Get handle for an anticipated acception from target IP */
/* Fails if no Listening Handle has not been previously Opened on the port */
ComAPIHandle ComTCPOpenAccept(unsigned long IPaddress, int tcpPort ,int timeoutsecs );

/* get a handle to use with ComAPISend for sending data to all TCP connections */
/* will send to all open connections, either accept() or connect() type */
/* will ignore listen sockets */
ComAPIHandle ComTCPGetGroupHandle(int buffersize);

/***************

When calling  ComDPLAYOpen with CAPI_DPLAY_SERIAL_PROTOCOL
the string "address" parameter selects and configures the COM Port.
The order of the parameters in the string is unconstrained and case insensitive.

An  example string is:  "COM1 CBR_115200 NOPARITY ONESTOPBIT DPCPA_DTRFLOW"

Any/ALL values may be ommitted with defaults: COM1 CBR_56000 ONESTOPBIT  NOPARITY DPCPA_RTSDTRFLOW                           
                
                           
COM ports may be:             COM1  COM2  COM3 COM4


Baud Rate values may be:      CBR_110	CBR_300	CBR_600 
                              CBR_1200	CBR_2400	CBR_4800 
                              CBR_9600	CBR_14400	CBR_19200 
                              CBR_38400	CBR_56000	CBR_57600 
                              CBR_115200	CBR_128000	CBR_256000 


StopBits values may be:       ONESTOPBIT   ONE5STOPBITS  TWOSTOPBITS. 
                                 
Parity values may be:         NOPARITY  ODDPARITY  EVENPARITY  MARKPARITY. 

FlowControl values may be:    DPCPA_DTRFLOW 	    Indicates hardware flow control with DTR. 
                              DPCPA_NOFLOW 	    Indicates no flow control. 
                              DPCPA_RTSDTRFLOW 	 Indicates hardware flow control with RTS and DTR. 
                              DPCPA_RTSFLOW   	 Indicates hardware flow control with RTS. 
                              DPCPA_XONXOFFFLOW  Indicates software flow control (xon/xoff). 



***************/                

/* ComDPLAYOpen:
 protocol     
       CAPI_DPLAY_MODEM_PROTOCOL    
       CAPI_DPLAY_SERIAL_PROTOCOL   
       CAPI_DPLAY_TCP_PROTOCOL      
       CAPI_DPLAY_IPX_PROTOCOL      
mode   
       CAPI_HOST
       CAPI_JOIN  
address:
         for TCP   IP address string , ie: 130.22.3.55       
         for MODEM      telephone number
         for SERIAL    see above for

guid:   Micorsoft program GUID
         
*****/    
ComAPIHandle ComDPLAYOpen(int protocol,int mode, char *address, int buffersize, void *guid, void (*ConnectCallback)(ComAPIHandle c, int ret), int timeoutsecs);

void ComAPIDPLAYSendMode(ComAPIHandle c, int sendmode);   /* default is GUARANTEED */


/* end comms session */
void ComAPIClose(ComAPIHandle c);

/* send and recive data from a comms session */
int ComAPISendOOB(ComAPIHandle c, int msgsize, int posupd);
int ComAPISend(ComAPIHandle c, int msgsize);
int ComAPIGet(ComAPIHandle c);

void ComAPIRegisterInfoCallback (void (*func)(ComAPIHandle c, int send, int msgsize));

/* Set the bandwidth required for all communications
	- returns WOULDBLOCK if it would be exceeded */

void ComAPISetBandwidth (int bytes);
int ComAPIGetBandwidth (void);

/* set the group to send and recieve data from */
void ComAPIGroupSet(ComAPIHandle c, int group);

/* get the local hosts unique id */
int ComAPIHostIDLen(ComAPIHandle c);
int ComAPIHostIDGet(ComAPIHandle c, char *buf);

/* get the associated buffers */
char *ComAPISendBufferGet(ComAPIHandle c);
char *ComAPIRecvBufferGet(ComAPIHandle c);

/* query connection information - not all options are supported on all protocols */
/* refer to query types #defined above */
unsigned long ComAPIQuery(ComAPIHandle c,int querytype);

/* Group functions*/
ComAPIHandle ComAPICreateGroup(char *name, int messagesize,...);
/* Must call with 0 terminating parameter ie: ComCreateGroup(1024,0) 
   If called with comhandles , these will be used to determine Group Message size
   ie:   ComCreateGroup(1024,,ch1,ch2,0)  */


/* member may be a group handle or a connection handle */
int ComAPIAddToGroup(ComAPIHandle grouphandle, ComAPIHandle memberhandle);
int ComAPIDeleteFromGroup(ComAPIHandle grouphandle, ComAPIHandle memberhandle);
ComAPIHandle CAPIIsInGroup (ComAPIHandle grouphandle, unsigned long ipAddress);
/* Send to a group but exclude Xhandle */
int ComAPISendX(ComAPIHandle group, int msgsize, ComAPIHandle Xhandle );

/* Close all Open IP handles */
void ComAPICloseOpenHandles( void);

/* Convert host long IP address to string */
char *ComAPIinet_htoa(unsigned long ip);

/* Convert net long IP address to string */
char *ComAPIinet_ntoa(unsigned long ip);
/* Convert dotted string ipa ddress to host long */
unsigned long ComAPIinet_haddr(char * IPaddress);

/* reports last error for ComOPen calls */
unsigned long ComAPIGetLastError(void);

unsigned long ComAPIGetNetHostBySocket(int Socket);
unsigned long ComAPIGetNetHostByHandle(ComAPIHandle c);

/* TIMESTAMP functions */
/* OPTIONAL call to set the timestamp function */
void ComAPISetTimeStampFunction(unsigned long (*TimeStamp)(void));

/* get the timestamp associated with the most recent ComAPIGet() 
   returns 0 if no timestamp function has been defined */
unsigned long ComAPIGetTimeStamp(ComAPIHandle c);

/****  Protoype for TimeStamp callback 
    unsigned long ComAPIGetTimeStamp(ComAPIHandle c)
***/


/* sets the rececive thread priority for UDP .. receive thread .. if exists*/
void ComAPISetReceiveThreadPriority(ComAPIHandle c, int priority);


#ifdef __cplusplus
}
#endif
#endif

