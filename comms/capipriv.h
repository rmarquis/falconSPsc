/* capipriv.h - Copyright (c) Fri Dec 06 22:41:25 1996,  Spectrum HoloByte, Inc.  All Rights Reserved */
#ifndef _CAPIPRIV_H_
#define _CAPIPRIV_H_
#ifdef __cplusplus
extern "C" {
#endif
	
#include <winsock.h>
	
typedef struct _WSABUF {
	u_long      len;     /* the length of the buffer */
	char FAR *  buf;     /* the pointer to the buffer */
} WSABUF, FAR * LPWSABUF;


/***  define LOAD_DLLS for expicit LoadLibrary() calls will be used for WS2_32.DLL and DPLAYX.DLL ***/
/***  If not defined  application must link with WS2_32.LIB and DPLAYX.LIB                        ***/
/* #define LOAD_DLLS       */
/***********************************************************************************************/


//#define GAME_NAME_LENGTH 16
#define GAME_NAME_LENGTH 4

/* extern GLOBAL WSA startup reference count, defined in  WS2Init() */
extern int WS2Connections;
extern HINSTANCE  hWinSockDLL;

typedef			 unsigned long	(*DWProc_t)();

typedef struct comapiheader {
	char gamename[GAME_NAME_LENGTH];
} ComAPIHeader;

#define MAX_RUDP_HEADER_SIZE	(sizeof(ComAPIHeader)+9)		// Maximum amount of crap we'll take on our packets

typedef struct capilist {
	struct capilist      *next;
	char				 *name;
	ComAPIHandle          com;
#ifdef CAPI_NET_DEBUG_FEATURES
	void                 *data;
	int                   size;
	int                   sendtime;
#endif
} CAPIList;

typedef struct comapihandle {
	char			*name;
	int protocol;
	int (*send_func)(struct comapihandle *c, int msgsize, int oob);
	int (*recv_func)(struct comapihandle *c);
	char * (*send_buf_func)(struct comapihandle *c);
	char * (*recv_buf_func)(struct comapihandle *c);
	int (*addr_func)(struct comapihandle *c, char *buf);
	void (*close_func)(struct comapihandle *c);
	unsigned long (*query_func)(struct comapihandle *c, int querytype);
	int (*sendX_func)(struct comapihandle *c, int msgsize, struct comapihandle *Xcom );
	unsigned long (*get_timestamp_func)(struct comapihandle *c);
} ComAPI;

typedef struct reliable_packet
{
	unsigned long last_sent_at;				/* time this packet was last sent */
	unsigned short sequence_number;
	unsigned short message_number;
	unsigned short size;
	unsigned char send_count;
	unsigned char oob;
	unsigned char message_slot;
	unsigned char message_parts;
	unsigned char dispatched;				/* has the application seen us yet? */
	unsigned char acknowledged;				/* have we sent the ack out of sequence */
	
	char *data;
	
	struct reliable_packet *next;
} Reliable_Packet;


typedef struct reliable_data
{
	unsigned short sequence_number;		/* sequence number for sending */
	unsigned short oob_sequence_number;	/* sequence number for sending */

	short message_number;				/* message number for sending */
	Reliable_Packet *sending;			/* list of sent packets */
	Reliable_Packet *last_sent;			/* last of the sending packets */
	
	int reset_send;						/* What is the reset stage we are in */
	
	int last_sequence;					/* other's last seen sequence number */
	int last_received;					/* my last received sequential sequence number for ack.*/
	int last_sent_received;				/* the last last_received that I acknowledged */
	int send_ack;						/* we need to send an ack packet */

	int last_dispatched;				/* the last packet that I dispatched */
	Reliable_Packet *receiving;			/* list of received packets */

	int sent_received;					/* what the last received I sent */
	unsigned long last_send_time;		/* last time we checked for ack */

	Reliable_Packet *oob_sending;		/* list of sent packets */
	Reliable_Packet *oob_last_sent;			/* last of the sending packets */

	int last_oob_sequence;				/* other's last seen sequence number */
	int last_oob_received;				/* my last received sequential sequence number for ack.*/
	int last_oob_sent_received;			/* the last last_received that I acknowledged */
	int send_oob_ack;					/* we need to send an ack packet */

	int last_oob_dispatched;			/* the last packet that I dispatched */
	Reliable_Packet *oob_receiving;		/* list of received packets */
	
	int sent_oob_received;				/* what the last received I sent */
	unsigned long last_oob_send_time;	/* last time we checked for ack */
	
	long last_ping_send_time;			/* when was the last time I sent a ping */
	long last_ping_recv_time;			/* the time we received on a ping */
	char *real_send_buffer;				/* buffer actually sent down the wire (after encoding) */
} Reliable_Data;


typedef struct comiphandle {
	struct comapihandle apiheader;

	int buffer_size;
	int max_buffer_size;
	int ideal_packet_size;
	WSABUF send_buffer;
	WSABUF recv_buffer;
	WSADATA wsaData;
	
#ifdef COMPRESS_DATA
	char * compression_buffer;
#endif
	
	struct sockaddr_in address;
	
	SOCKET send_sock;
	SOCKET recv_sock;
	unsigned long recvmessagecount;
	unsigned long sendmessagecount;
	unsigned long recvwouldblockcount;
	unsigned long sendwouldblockcount;
	
	HANDLE lock;
	HANDLE ThreadHandle;
	short ThreadActive;
	short current_get_index;
	short current_store_index;
	short last_gotten_index;
	short wrapped;
	char  *message_cache;
	short *bytes_recvd;
	struct comiphandle *parent;
	int referencecount;
	int BroadcastModeOn;
	int NeedBroadcastMode;
	unsigned int whoami;
	unsigned long lastsender;
	unsigned long timestamp;
	unsigned long *timestamps;
	unsigned long  *senders;
	Reliable_Data rudp_data;
} ComIP;


typedef struct comgrouphandle {
	struct comapihandle apiheader;
	int            buffer_size;
	unsigned int   HostID;
	CAPIList      *GroupHead;
	char          *send_buffer;
	int            max_header;
	char           TCP_buffer_shift;
	char           UDP_buffer_shift;
	char           RUDP_buffer_shift;
	char           DPLAY_buffer_shift;
} ComGROUP;


#define CAPI_SENDBUFSIZE 1024
#define CAPI_RECVBUFSIZE 65536

void use_bandwidth (int size);
int check_bandwidth (int size);

void use_recv_bandwidth (int size);
int check_recv_bandwidth (int size);

#ifdef __cplusplus
}
#endif
#endif
