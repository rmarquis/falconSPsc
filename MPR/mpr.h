/* Copyright (C) 1998 MicroProse, Inc.  All rights reserved */

#ifndef _MPR_H_
#define _MPR_H_




#if !defined(MPR_INTERNAL)
#include <windows.h>
#endif

//#include <objbase.h>


#ifdef  __cplusplus
extern  "C"{
#endif  /* __cplusplus */


#ifndef UNUSED_PARAMETER
#define UNUSED_PARAMETER(x)     x = x
#endif


// Get compile time switches
#include "MPRcfg.h"


/*
// Dictionary of Acronyms, Keywords etc.,
//
//  MPR - MPR
//
//  RC  - Rendering Context
//  Pt  - Point, as in PtLong_T
//  Clr - Color, as in ClrLong_T
//  Ptr - Pointer, as in LongPtrToByte_T
//  Prm - Primitive, as in MPRPrmType_T
//  Vtx - Vertex, as in MPRVtxType_T
//  SRC - Source, as in MPR_STATE_SRC_BLEND
//  DST - Destination, as in MPR_STATE_DST_BLEND
//  Msg - Message, as in MPRMsgType
//  MSG - Message, as in MPR_MSG_NOP
//  Pkt - Packet, as in MPRPktType
//  PKT - Packet, as in MPR_PKT_END
//  Hdr - Header, as in MsgHdrType
//  Gen - Generic, as in MsgGenHdr
//  Bmp - Bitmap, as in MsgBmpHdr
//  Sta - State, as in PktStaType
//  STA - STATE, as in MPR_STA_ENABLES
//  DB  - DoubleBuffer, as in  MPR_DB_FRONT
//  ZF  - Z buffer Function, as in MPR_ZF_NEVER 
//  LT  - Less Then, as in MPR_ZF_LT
//  EQ  - EQual to, as in MPR_ZF_EQ
//  LE  - Less then or Equal to, as in MPR_ZF_LE
//  GT  - Greater Then, as in MPR_ZF_GT
//  NE  - Not Equal to, as in MPR_ZF_NE
//  GE  - Greater then or Equal to, as in MPR_ZF_GE
//  BL  - Blending Function, as in MPR_BL_SRC_INV 
//  INV - INVerse, as in MPR_BL_SRC_INV
//  RP  - Raster oPeration function, as in MPR_RP_AND 
//  TF  - Texture Filter function, as in MPR_TF_MIPMAP 
//  TM  - Texture Modulation function, as in MPR_TF_MIPMAP 
//  TT  - Texture Type information , as in MPR_TT_KEEP 
//  BT  - Bitmap Type information , as in MPR_BT_ALPHA 
//  ST  - State Type information , as in MPR_ST_LEFT 
//  CT  - Clearbuffer Type information , as in MPR_CT_DRAWBUFFER 
*/

// Typedefs used commonly by mpr.

#ifndef _SHI__INT_H_
typedef unsigned int        UInt;
typedef unsigned char       UInt8;
typedef unsigned short      UInt16;
typedef unsigned long       UInt32;

typedef signed   int        Int;
typedef signed   char       Int8;
typedef signed   short      Int16;
typedef signed   long       Int32;
#endif

typedef UInt  (*DWProc_t)();
typedef UInt  MPRHandle_t;
typedef float Float_t;

// MPR data types - Points

/* New (reduced calorie) structures */

/* NOTE: Care is needed when mucking with  */
/* these structures.  Some of the asm code */
/* assumes groupings and alignments.       */

/** START OF SPECIAL "CARE" SECTION **/
typedef struct { 
  Float_t x, y;             
} MPRVtx_t;

typedef struct { 
  Float_t x, y; 
  Float_t r, g;
  Float_t b, a;
#if defined(MP_DEPTH_BUFFER_ENABLED)
  Float_t z;
#endif
} MPRVtxClr_t;

typedef struct { 
  Float_t x, y; 
  Float_t r, g;
  Float_t b, a;   
  Float_t u, v;
  Float_t q;
#if defined(MPR_DEPTH_BUFFER_ENABLED)
  Float_t z;
#endif
} MPRVtxTexClr_t;
/** END OF SPECIAL "CARE" SECTION **/


// MPR data types - Message Header
typedef struct {
  UInt        MsgHdrType;
  UInt        MsgHdrSize;
  MPRHandle_t MsgHdrRC;
} MPRMsgHdr_t;


// MPR data types - Generic Message definition
typedef struct {
  MPRMsgHdr_t MsgGenHdr;
  UInt        MsgGenInfo0;
  UInt        MsgGenData;         /* Buffer-wide Modifier */
  UInt        MsgGenhDev;         /* Handle To Device     */
  UInt        MsgGenBuffer[1];    /* variable size data   */
} MPRMsgGen_t; 


// MPR data types - Texture Create Message definition
typedef struct {
  MPRMsgHdr_t   MsgTexHdr;      
  UInt16        MsgTexInfo;
  UInt16        MsgTexBitsPixel;
  UInt16        MsgTexWidth;
  UInt16        MsgTexRows;     
} MPRMsgNewTex_t;

    
// MPR data types - Texture Load Message definition
typedef struct {
  MPRMsgHdr_t MsgTexHdr;      
  UInt16      MsgTexBitsPixel;
  UInt16      MsgTexMipLevel; 
  UInt16      MsgTexWidth;
  UInt16      MsgTexRows;     
  Int16       MsgTexWidthBytes;
  UInt        MsgTexId;
  UInt        MsgTexChromaColor;  /* Buffer-wide Modifier */
  UInt8*      MsgTexPtr;          /* variable size data ptr */
} MPRMsgLoadTex_t;
    

// MPR data types - Texture Palette Message definition
typedef struct {
  MPRMsgHdr_t MsgPalHdr;      
  UInt16      MsgPalInfo;
  UInt16      MsgPalBitsPerEntry;
  UInt16      MsgPalStartIndex;   
  UInt16      MsgPalNumEntries;
  UInt        MsgPalId;
  UInt        MsgTexId;
  UInt8*      MsgPalPtr;              /* variable size data ptr */
} MPRMsgTexPal_t;
    

// MPR data types - Bitmap Message definition
typedef struct {
  MPRMsgHdr_t MsgBmpHdr;  
  UInt16      MsgBmpBitsPixel; 
  UInt16      MsgBmpWidth;
  UInt16      MsgBmpHeight; 
  Int16       MsgBmpWidthBytes; 
  Int16       MsgBmpSrcX;
  Int16       MsgBmpSrcY;
  Int16       MsgBmpDstX;
  Int16       MsgBmpDstY;
  UInt8*      MsgBmpImage;
} MPRMsgBmp_t; 


// MPR data types - Packet definition
typedef struct {
  MPRMsgHdr_t MsgPktHdr;
  UInt        MsgPktBuffer[1];    /* variable size data   */
} MPRMsgPkt_t; 


// MPR data types - Packet (of primitives) definition
typedef struct {
  UInt   PktPrmPktType;
  UInt   PktPrmVtxNum;
  UInt   PktPrmVtxInfo;
  UInt   PktPrmVtxSize;
  UInt   PktPrmBuffer[1];        /* variable size data   */
} MPRPktPrm_t; 


// MPR data types - Packet (of State Information) definition
typedef struct {
  UInt   PktStaPktType;
  UInt   PktStaState;
  UInt   PktStaData;
} MPRPktSta_t; 


// MPR data types - Packet (of Vertices) definition
typedef struct {
  UInt   PktGenPktType;
  UInt   PktGenInfo;
} MPRPktGen_t; 


// MPR data types - Packet (of Ddraw Information) definition
typedef struct {
  UInt    PktDDrawPktType;
  UInt    PktDDrawData0;
} MPRPktDDraw_t; 


typedef enum MPRSurfaceType {
  SystemMem, VideoMem,	// Valid for front or back buffer specifier
  Primary, 		// Valid for front buffer specifier only
  Flip, None		// Valid for back buffer specifier only
} MPRSurfaceType;


typedef enum MPRLanguageNum {
  MPR_English = 0, 
  MPR_German,	
  MPR_Portuguese, 		
  MPR_Spanish, 
  MPR_French,		
  MPR_Italian,
  MPR_Japanese
} MPRLanguageNum;


typedef struct MPRSurfaceDescription {
  UInt height;
  UInt width;
  UInt pitch;
  UInt RGBBitCount; 
  UInt RBitMask; 
  UInt GBitMask; 
  UInt BBitMask; 
} MPRSurfaceDescription;


typedef enum {
  HOOK_CREATESURFACE = 0,   
  HOOK_RELEASESURFACE,             
  HOOK_GETSURFACEDESRIPTION,       
  HOOK_LOCK,           
  HOOK_UNLOCK,          
  HOOK_BLT,        
  HOOK_BLTCHROMA,        
  HOOK_SETCHROMA,               
  HOOK_SWAPSURFACE,
  HOOK_SETWINDOWRECT,
  HOOK_RESTORESURFACE,
  HOOK_DEACTIVATESURFACE,
  
  MAXSURFACEHOOKS,  //MUST BE LAST
} mprSurfaceHookNames;



#ifdef  MPR_TYPES_ONLY
#else   /* MPR_TYPES_ONLY */

// MPR - Definitions

#define MPR_MAX_DEVICES                  8

// These are the various packet identifers used by MPR. 
//  These are used to create general packets.
//  These values are used in MPRMsgPtr_t.
// Pick ONE of ...

typedef enum {
  /*********************************************************************/
  MPR_PKT_END = 0,              /* MUST BE SAME AS HOOK_END            */
  MPR_PKT_SETSTATE,             /* MUST BE SAME AS HOOK_SETSTATE       */
  MPR_PKT_RESTORESTATEID,       /* MUST BE SAME AS HOOK_RESTORESTATEID */
  MPR_PKT_STARTFRAME,           /* MUST BE SAME AS HOOK_STARTFRAME     */
  MPR_PKT_FINISHFRAME,          /* MUST BE SAME AS HOOK_FINISHFRAME    */
  MPR_PKT_CLEAR_BUFFERS,        /* MUST BE SAME AS HOOK_CLEAR...       */
  MPR_PKT_SWAP_BUFFERS,         /* MUST BE SAME AS HOOK_SWAP...        */
  MPR_PKT_POINTS,               /* MUST BE SAME AS HOOK_POINTS         */
  MPR_PKT_LINES,                /* MUST BE SAME AS HOOK_LINES          */
  MPR_PKT_POLYLINE,             /* MUST BE SAME AS HOOK_POLYLINE       */
  MPR_PKT_TRIANGLES,            /* MUST BE SAME AS HOOK_TRIANGLES      */
  MPR_PKT_TRISTRIP,             /* MUST BE SAME AS HOOK_TRISTRIP       */
  MPR_PKT_TRIFAN,               /* MUST BE SAME AS HOOK_TRIFAN         */
  /*********************************************************************/

  MPR_PKT_ID_COUNT,             /* MUST BE LAST */
} MPRPacketID;


// These are used by MPRPrimitive()
// Pick ONE of ...

#define MPR_PRM_POINTS              MPR_PKT_POINTS
#define MPR_PRM_LINES               MPR_PKT_LINES
#define MPR_PRM_POLYLINE            MPR_PKT_POLYLINE 
#define MPR_PRM_TRIANGLES           MPR_PKT_TRIANGLES 
#define MPR_PRM_TRISTRIP            MPR_PKT_TRISTRIP
#define MPR_PRM_TRIFAN              MPR_PKT_TRIFAN

// These are used by MPRGetState() and MPRSetState()
// Pick ONE of ...

// Enumerated states. 
// >> Possible values mentioned in the comments section.
// Pick ONE of ...

enum {
  MPR_STA_NONE,
  MPR_STA_ENABLES,               /* use MPR_SE_...   */
  MPR_STA_DISABLES,              /* use MPR_SE_...   */

  MPR_STA_SRC_BLEND_FUNCTION,    /* use MPR_BF_...   */
  MPR_STA_DST_BLEND_FUNCTION,    /* use MPR_BF_...   */

  MPR_STA_TEX_FILTER,            /* use MPR_TX_...   */
  MPR_STA_TEX_FUNCTION,          /* use MPR_TF_...   */

#if defined(MPR_MASKING_ENABLED)
  MPR_STA_AREA_MASK,             /* Area pattern bitmask */
  MPR_STA_LINE_MASK,             /* Line pattern bitmask */
  MPR_STA_PIXEL_MASK,            /* RGBA or bitmask  */
#endif
  MPR_STA_FG_COLOR,              /* long, RGBA or index  */
  MPR_STA_BG_COLOR,              /* long, RGBA or index  */

#if defined(MPR_DEPTH_BUFFER_ENABLED)
  MPR_STA_Z_FUNCTION,            /* use MPR_ZF_...   */
  MPR_STA_FG_DEPTH,              /* FIXED 16.16 for raster*/
  MPR_STA_BG_DEPTH,              /* FIXED 16.16 for zclear*/
#endif

  MPR_STA_TEX_ID,                /* Handle for current texture.*/

  MPR_STA_FOG_COLOR,             /* long, RGBA       */
  MPR_STA_HARDWARE_ON,           /* Read only - set if hardware supports mode */

//#if !defined(MPR_GAMMA_FAKE)
  MPR_STA_GAMMA_RED,             /* Gamma correction term for red (set before blue)  */
  MPR_STA_GAMMA_GREEN,           /* Gamma correction term for green (set before blue) */
  MPR_STA_GAMMA_BLUE,            /* Gamma correction term for blue (set last) */
//#else
  MPR_STA_RAMP_COLOR,           /* Packed color for the ramp table        */
  MPR_STA_RAMP_PERCENT,         /* fractional (0.0=normal, 1.0=saturated) */
//#endif
  
  MPR_STA_SCISSOR_LEFT,  
  MPR_STA_SCISSOR_TOP,  
  MPR_STA_SCISSOR_RIGHT,         /* right,bottom, not inclusive*/
  MPR_STA_SCISSOR_BOTTOM,        /* Validity Check done here.    */

};

// These are MPR message type definitions
// Pick ONE of ...

typedef enum {
  MPR_MSG_NOP,
  MPR_MSG_CREATE_RC,
  MPR_MSG_DELETE_RC,
  MPR_MSG_GET_STATE,
  MPR_MSG_NEW_TEXTURE,
  MPR_MSG_LOAD_TEXTURE,
  MPR_MSG_FREE_TEXTURE,
  MPR_MSG_PACKET,
  MPR_MSG_BITMAP,
  MPR_MSG_NEW_TEXTURE_PAL,
  MPR_MSG_LOAD_TEXTURE_PAL,
  MPR_MSG_FREE_TEXTURE_PAL,
  MPR_MSG_ATTACH_TEXTURE_PAL,
  MPR_MSG_STORE_STATEID,
  MPR_MSG_GET_STATEID,
  MPR_MSG_FREE_STATEID,
  MPR_MSG_ATTACH_SURFACE,
  MPR_MSG_OPEN_DEVICE,
  MPR_MSG_CLOSE_DEVICE
} mprMsgID;


// Possible values for: MPR_STA_ENABLES 
//          MPR_STA_DISABLES 
// Logical OR of ...
#define MPR_SE_SHADING              0x00000001L /* interpolate r,g,b, a       */
#define MPR_SE_TEXTURING            0x00000002L /* interpolate u,v,w          */
#define MPR_SE_MODULATION           0x00000004L /* modulate color & texture   */
#define MPR_SE_Z_BUFFERING          0x00000008L /* interpolate z and compare  */
#define MPR_SE_FOG                  0x00000010L /* interpolate fog            */
#define MPR_SE_PIXEL_MASKING        0x00000020L /* selective pixel update     */

#define MPR_SE_Z_MASKING            0x00000040L /* selective z update         */
#define MPR_SE_PATTERNING           0x00000080L /* selective pixel & z update */
#define MPR_SE_SCISSORING           0x00000100L /* selective pixel & z update */
#define MPR_SE_CLIPPING             0x00000200L /* selective pixel & z update */
#define MPR_SE_BLENDING             0x00000400L /* per-pixel operation        */

#define MPR_SE_FILTERING            0x00002000L /* texture filter control     */
#define MPR_SE_TRANSPARENCY         0x00004000L /* texel use control          */

#define MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE  0x00200000L
#define MPR_SE_NON_SUBPIXEL_PRECISION_MODE      0x00400000L
#define MPR_SE_HARDWARE_OFF                     0x00800000L
#define MPR_SE_PALETTIZED_TEXTURES              0x01000000L
#define MPR_SE_DECAL_TEXTURES                   0x04000000L 
#define MPR_SE_ANTIALIASING                     0x08000000L
#define MPR_SE_DITHERING                        0x10000000L


#define MPR_DRAW_BUFFER     0x0080  /* draw buffer      */
#define MPR_DEPTH_BUFFER    0x0400  /* z buffer.        */
#define MPR_TEXTURE_BUFFER  0x0800  /* texture.         */


// Possible values for: MPR_STA_Z_FUNCTION
// Pick one of ...

enum {
  MPR_ZF_NEVER,
  MPR_ZF_LT,
  MPR_ZF_EQ,
  MPR_ZF_LE,
  MPR_ZF_GT,
  MPR_ZF_NE,
  MPR_ZF_GE,
  MPR_ZF_ALWAYS,
};

// Possible values for: MPR_STA_SRC_BLEND_FUNCTION
//          MPR_STA_DST_BLEND_FUNCTION
// Pick one of ...
enum {
 MPR_BF_SRC_ALPHA,
 MPR_BF_SRC_ALPHA_INV,

#if 1	// These are still referenced by the D3D driver.  Needs to be fixed...
//#if defined(MPR_BLEND_FUNCTIONS_ENABLED)
 MPR_BF_ZERO,
 MPR_BF_ONE,
 MPR_BF_SRC,                    /* Only MPR_STA_DST_BLEND_FUNCTION */
 MPR_BF_SRC_INV,                /* Only MPR_STA_DST_BLEND_FUNCTION */
 MPR_BF_DST,                    /* Only MPR_STA_SRC_BLEND_FUNCTION */
 MPR_BF_DST_INV,                /* Only MPR_STA_SRC_BLEND_FUNCTION */
 MPR_BF_DST_ALPHA,
 MPR_BF_DST_ALPHA_INV,
#endif
};

// Possible values for: MPR_STA_TEX_FUNCTION
// Pick ONE of ...

enum {
  MPR_TF_NONE,
  MPR_TF_FOO,
  MPR_TF_MULTIPLY,
  MPR_TF_SHADE,
  MPR_TF_ALPHA,
  MPR_TF_ADD,
};

// Possible values for: MPR_STA_TEX_FILTER
// Pick ONE of ...

enum {
  MPR_TX_NONE,
  MPR_TX_BILINEAR,              /* interpolate 4 pixels     */
  MPR_TX_DITHER,                /* Dither the colors */
  MPR_TX_MIPMAP_NEAREST = 10,   /* nearest mipmap       */
  MPR_TX_MIPMAP_LINEAR,         /* interpolate between mipmaps  */
  MPR_TX_MIPMAP_BILINEAR,       /* interpolate 4x within mipmap */
  MPR_TX_MIPMAP_TRILINEAR,      /* interpolate mipmaps,4 pixels */
};


// Possible values for : VtxInfo in the MPRPrimitive() prototype below
// Logical OR of ...
#define MPR_VI_COLOR                0x0002
#define MPR_VI_TEXTURE              0x0004  
    
// Possible values for: BmpInfo in the MPRBitmap() prototype below
// Logical OR of ...
#define MPR_BI_INDEX                0x0001
#define MPR_BI_CHROMAKEY            0x0002
#define MPR_BI_ALPHA                0x0004

// Possible values for: SwapInfo in the MPRSwapBuffers() prototype below
// Logical OR of ...
#define MPR_SI_SCISSOR              0x0004


// Possible values for: ClearInfo in the MPRClearBuffers() prototype below
// Logical OR of ...
#define MPR_CI_DRAW_BUFFER          0x0001
#define MPR_CI_ZBUFFER              0x0004  


// Possible values for: TexInfo in the MPRNewTexture() prototype below.
// Logical OR of ...
#define MPR_TI_DEFAULT              0x0000
#define MPR_TI_MIPMAP               0x0001
#define MPR_TI_CHROMAKEY            0x0020
#define MPR_TI_ALPHA	            0x0040
#define MPR_TI_PALETTE              0x0080

// The following flags are used internally by MPR but stored in the same
// memory as the above TI_ "user" flags, so we'll define them here to ensure
// they retain mutually exclusive values.
#define MPR_TI_RESERVED_DRTY		0x0100


// Possible values for: MPRMsgHdr.MsgHdrVersion field
// Pick ONE of ...
#define MPR_MSG_VERSION             1

// Maximum No of vertices to be passed using MPRPrimitive()
#define MPR_MAX_VERTICES            256

//  MPR Function Prototypes :

#if !defined(MPR_INTERNAL)

UInt __stdcall  
MPRCreateRC     (MPRHandle_t hDev);

UInt __stdcall  
MPRDeleteRC     (MPRHandle_t hRC);

void __stdcall 
MPRPrimitive    (MPRHandle_t hRC, 
                 UInt        type, 
                 UInt        count, 
                 UInt        info, 
                 UInt        size,
                 UInt8*      ptr);

UInt __stdcall 
MPRGetState     (MPRHandle_t hRC,
                 UInt        state);

void  __stdcall 
MPRSetState     (MPRHandle_t hRC,
                 UInt        state, 
                 UInt        data);

UInt __stdcall 
MPRStoreStateID (MPRHandle_t hRC);

void  __stdcall 
MPRRestoreStateID(MPRHandle_t hRC,
                  UInt        ID);

UInt __stdcall 
MPRGetStateID   (MPRHandle_t hRC);

UInt __stdcall 
MPRFreeStateID  (MPRHandle_t hRC,
                 UInt        ID);

UInt __stdcall 
MPRNewTexture   (MPRHandle_t hRC, 
                 UInt16      info,
                 UInt16      bits,
                 UInt16      width, 
                 UInt16      height);

UInt __stdcall 
MPRLoadTexture  (MPRHandle_t hRC, 
                 UInt16      bitsPixel, 
                 UInt16      mip, 
                 UInt16      width, 
                 UInt16      rows, 
                 Int16       TexWidthBytes, 
                 UInt        ID, 
                 UInt        chroma,
                 UInt8*      TexBuffer);

void  __stdcall 
MPRFreeTexture  (MPRHandle_t hRC, 
                 MPRHandle_t ID);

UInt __stdcall 
MPRNewTexturePalette (MPRHandle_t hRC, 
                      UInt16      PalBitsPerEntry,
                      UInt16      PalNumEntries);

UInt __stdcall 
MPRLoadTexturePalette (MPRHandle_t hRC,
                       UInt        ID,
                       UInt16      info,
                       UInt16      PalBitsPerEntry,
                       UInt16      index,
                       UInt16      entries,
                       UInt8*      PalBuffer);

void  __stdcall 
MPRFreeTexturePalette   (MPRHandle_t hRC, 
                         MPRHandle_t ID);

void  __stdcall 
MPRAttachTexturePalette (MPRHandle_t hRC, 
                         UInt        TexID,
                         UInt        PalID);

UInt __stdcall 
MPRSetTextureAttributes (MPRHandle_t hRC, 
                         UInt        ID,
                         UInt        info,    
                         UInt        size,
                         UInt8*      data);

UInt __stdcall
MPRGetTextureAttributes (MPRHandle_t hRC, 
                         UInt        ID,
                         UInt        info,        
                         UInt8*      data);

void  __stdcall 
MPRBitmap       (MPRHandle_t hRC, 
                 UInt16      BmpBitsPixel,
                 UInt16      BmpWidth, 
                 UInt16      BmpHeight, 
                 Int16       BmpWidthBytes, 
                 Int16       BmpSrcX, 
                 Int16       BmpSrcY,
                 Int16       BmpDstX, 
                 Int16       BmpDstY,
                 UInt8*      BmpBuffer);

void  __stdcall
MPRSwapBuffers  (MPRHandle_t hRC, 
                 UInt        lpSurface);

void  __stdcall  
MPRClearBuffers (MPRHandle_t hRC, 
                 UInt16      ClearInfo);

UInt __stdcall 
MPRMessage      (MPRHandle_t hRC, 
                 UInt        InDataSize, 
                 UInt8*      InDataPtr, 
                 UInt        OutDataSize, 
                 UInt8*      OutDataPtr);

UInt __stdcall
MPRAttachSurface(MPRHandle_t hRC, 
                 UInt        lpDDSurface,
                 UInt16      info);

void  __stdcall
MPRStartFrame   (MPRHandle_t hRC);

void  __stdcall
MPRFinishFrame  (MPRHandle_t hRC,
                 UInt        func); 

UInt __stdcall  
MPRGetNumContexts (UInt hDev);

Int  __stdcall
MPROpenDevice   (UInt8* drvName, 
                 UInt   devicenum,
                 UInt resnum,
                 UInt Fullscreen);

Int  __stdcall
MPRCloseDevice  (UInt hDev); 

__declspec(dllexport) UInt __cdecl MPRIsDisplay(UInt drivernum, UInt devnum);


__declspec(dllexport) UInt __cdecl MPRGetDeviceResolution(UInt drivernum, UInt devnum, UInt resnum, UInt *width, UInt *height,
                             UInt *RBitMask, UInt *GBitMask, UInt *BBitMask, UInt *RGBBitCount );

__declspec(dllexport) UInt __cdecl MPRGetDeviceName(UInt drivernum, UInt devnum, char *devicename, UInt maxlen);
__declspec(dllexport) UInt __cdecl MPRGetDeviceMaxTextureWidth(UInt drivernum, UInt devnum);

__declspec(dllexport) UInt __cdecl MPRGetDriverName(UInt drivernum, char *drivername, UInt maxlen);

__declspec(dllexport) void    __cdecl MPRInitialize(UInt languageNum);
__declspec(dllexport) void    __cdecl MPRTerminate(void);


__declspec(dllexport) UInt __cdecl MPRCreateSurface(UInt DevID, UInt width, UInt height, 
													  MPRSurfaceType front, MPRSurfaceType back, 
													  HWND win, UInt clip, UInt fullscreen);
__declspec(dllexport) UInt __cdecl MPRGetBackbuffer(UInt handle);
__declspec(dllexport) UInt __cdecl MPRReleaseSurface(UInt handle);
__declspec(dllexport) UInt __cdecl MPRLockSurface(UInt handle);
__declspec(dllexport) UInt __cdecl MPRUnlockSurface(UInt handle);
__declspec(dllexport) UInt __cdecl MPRGetSurfaceDescription(UInt handle, MPRSurfaceDescription *surfdesc);
__declspec(dllexport) UInt __cdecl MPRSetChromaKey(UInt handle, UInt rgba , UInt colorkeybitsperpixel);
__declspec(dllexport) UInt __cdecl MPRBlt( UInt srcSurface, UInt dstSurface, RECT *srcRect, RECT *dstRect  );
__declspec(dllexport) UInt __cdecl MPRBltTransparent(UInt srcSurface, UInt dstSurface, RECT *srcRect, RECT *dstRect);
__declspec(dllexport) UInt __cdecl MPRSwapSurface(UInt rc, UInt surface);
__declspec(dllexport) UInt __cdecl MPRSetWindowRect(UInt surface, RECT *rect);
__declspec(dllexport) UInt __cdecl MPRRestoreSurface(UInt surface);
__declspec(dllexport) UInt __cdecl MPRRestoreDisplayMode(UInt surface);




#endif

#endif  /* MPR_TYPES_ONLY */

#ifdef  USE_MPR_PACKETS

#define MPR_BUF_SIZE        0x8000
#define MPR_BUF_PAD         0x0100
#define MPR_MAX_PKT_SIZE    (MPR_BUF_SIZE - MPR_BUF_PAD)

#define MPRPacketsInit( )                                                     \
        UInt8 MPRBufData[MPR_BUF_SIZE];                                       \
        UInt8 *MPRBufPtr   = MPRBufData + sizeof(MPRMsgHdr_t);                \
        UInt8 *MPRBufStart = MPRBufPtr;                                       \
        UInt8 *MPRBufLimit = MPRBufData + MPR_MAX_PKT_SIZE;                   \

#if 0

#define MPRPacketsExternInit()                                                \
extern  UInt8 MPRBufData[];                                                   \
extern  UInt8 *MPRBufPtr;                                                     \
extern  UInt8 *MPRBufStart;                                                   \
extern  UInt8 *MPRBufLimit;

#else

extern  UInt8 MPRBufData[];
extern  UInt8 *MPRBufPtr;
extern  UInt8 *MPRBufStart;
extern  UInt8 *MPRBufLimit;

#endif

//  The following macros are for the support of the macros below.
//  Note MPRMemCopy() is a simple byte-at-a-time copy function.
#define MPRMemCopy(DstPtr, SrcPtr, Size)                                    \
             memcpy((void*)DstPtr, (void*)SrcPtr, Size);
        

#define IsNotEnoughRoomInCurrentPacket(size)                                \
    (((UInt8*)(MPRBufPtr) + ((UInt)(size))) >                           \
                ((UInt8*)MPRBufLimit)) 

#define MPRSendCurrentPacket(hRC)                                           \
{                                                                           \
    MPRMsgHdr_t*  hPtr = (MPRMsgHdr_t*)(UInt8*)(MPRBufData);                \
    MPRPktGen_t*  ePtr = (MPRPktGen_t*)(UInt8*)(MPRBufPtr);                 \
    UInt          currentPktSize;                                           \
                                                                            \
    currentPktSize = (UInt)((UInt8*)(MPRBufPtr) -                           \
                               (UInt8*)(MPRBufData)) +                      \
                                sizeof(MPRPktGen_t);                        \
                                                                            \
    hPtr->MsgHdrType    = MPR_MSG_PACKET;                                   \
    hPtr->MsgHdrSize    = currentPktSize;                                   \
    hPtr->MsgHdrRC      = (MPRHandle_t)(hRC);                               \
                                                                            \
    ePtr->PktGenPktType = MPR_PKT_END;                                      \
    ePtr->PktGenInfo    = 0;                                                \
                                                                            \
    MPRMessage( (UInt) (hRC),                                               \
                (UInt) currentPktSize,                                      \
                (UInt8*) (MPRBufData),                                      \
                (UInt) 8,                                                   \
                (UInt8*) (MPRBufData));                                     \
                                                                            \
    (UInt8*)(MPRBufPtr) = (UInt8*)MPRBufStart;                              \
}

// The following macros are intended to be used in exactly the same way that
// the API functions of the same name are used. One can consider the API 
// functions to be functions that create tiny messages just big enough to 
// contain the primitive in use. These following macros will fill the 
// message buffer until it reaches a high water mark, or, until the 
// MPRSwapBuffers() macro is used.
#define MPRSetState(hRC, State, Value)                                      \
{                                                                           \
    MPRPktSta_t* StaPtr = (MPRPktSta_t*) (MPRBufPtr);                       \
                                                                            \
    if (IsNotEnoughRoomInCurrentPacket(sizeof(MPRPktSta_t))) {              \
        MPRSendCurrentPacket(hRC);                                          \
        StaPtr  = (MPRPktSta_t*) (MPRBufPtr);                               \
    }                                                                       \
                                                                            \
    StaPtr->PktStaPktType   = MPR_PKT_SETSTATE;                             \
    StaPtr->PktStaState     = (UInt)(State);                                \
    StaPtr->PktStaData      = (UInt)(Value);                                \
    (UInt8*)(MPRBufPtr)   += sizeof(MPRPktSta_t);                           \
                                                                            \
}

#define MPRRestoreStateID(hRC, StateID)                                     \
{                                                                           \
    MPRPktSta_t* StaPtr = (MPRPktSta_t*)(MPRBufPtr);                        \
                                                                            \
    if (IsNotEnoughRoomInCurrentPacket(sizeof(MPRPktSta_t))) {              \
        MPRSendCurrentPacket(hRC);                                          \
        StaPtr = (MPRPktSta_t*)(MPRBufPtr);                                 \
    }                                                                       \
                                                                            \
    StaPtr->PktStaPktType   = MPR_PKT_RESTORESTATEID;                       \
    StaPtr->PktStaData      = (UInt)(StateID);                           \
    (UInt8*)(MPRBufPtr)   += sizeof(MPRPktSta_t);                          \
                                                                            \
}

#define MPRPrimitive(hRC, Prim, VtxInfo, Count, VtxSize, VtxDataPtr)        \
{                                                                           \
    MPRPktPrm_t *PrmPtr        = (MPRPktPrm_t*)(MPRBufPtr);                 \
    UInt        curVtxSize    = (((UInt)(Count))*((UInt)(VtxSize)));        \
    UInt        curPktSize    = ((sizeof(MPRPktPrm_t) - 4) +  curVtxSize);  \
                                                                            \
    if (IsNotEnoughRoomInCurrentPacket(curPktSize)) {                       \
        MPRSendCurrentPacket(hRC);                                          \
        PrmPtr  = (MPRPktPrm_t*)(MPRBufPtr);                                \
    }                                                                       \
                                                                            \
    PrmPtr->PktPrmPktType   = (Prim);                                       \
    PrmPtr->PktPrmVtxInfo   = (VtxInfo);                                    \
    PrmPtr->PktPrmVtxNum    = (Count);                                      \
    PrmPtr->PktPrmVtxSize   = (VtxSize);                                    \
                                                                            \
    MPRMemCopy((UInt8*) PrmPtr->PktPrmBuffer,                               \
               (UInt8*)(VtxDataPtr),                                        \
               (UInt) curVtxSize);                                          \
    (UInt8*)(MPRBufPtr)   += curPktSize;                                    \
}

#define MPRClearBuffers(hRC, ClearInfo)                                     \
{                                                                           \
    MPRPktGen_t* GenPktPtr     = (MPRPktGen_t*)(MPRBufPtr);                 \
                                                                            \
    if (IsNotEnoughRoomInCurrentPacket(sizeof(MPRPktGen_t))) {              \
        MPRSendCurrentPacket(hRC);                                          \
        GenPktPtr = (MPRPktGen_t*) (MPRBufPtr);                             \
    }                                                                       \
                                                                            \
    GenPktPtr->PktGenPktType    = MPR_PKT_CLEAR_BUFFERS;                    \
    GenPktPtr->PktGenInfo       = (UInt)ClearInfo;                          \
    (UInt8*)(MPRBufPtr)       += sizeof(MPRPktGen_t);                       \
}

#if 0
// This macro does not work with the new interface - 08/14/98.
#define MPRSwapBuffers(hRC, lpSurface)                                      \
{                                                                           \
    MPRPktGen_t* GenPktPtr = (MPRPktGen_t*)(MPRBufPtr);                     \
                                                                            \
    if (IsNotEnoughRoomInCurrentPacket(sizeof(MPRPktGen_t))) {              \
        MPRSendCurrentPacket(hRC);                                          \
        GenPktPtr = (MPRPktGen_t*)(MPRBufPtr);                              \
    }                                                                       \
                                                                            \
    GenPktPtr->PktGenPktType    = MPR_PKT_SWAP_BUFFERS;                     \
    GenPktPtr->PktGenInfo       = (UInt)lpSurface;                          \
    (UInt8*)(MPRBufPtr)        += sizeof(MPRPktGen_t);                      \
                                                                            \
    MPRSendCurrentPacket(hRC);                                              \
}
#endif

#define MPRSwapBuffers( hRC, lpSurface )                                    \
{                                                                           \
    if( ( void * ) MPRBufPtr != ( void * ) MPRBufStart )                    \
        MPRSendCurrentPacket( hRC );                                        \
                                                                            \
    MPRSwapSurface( hRC, lpSurface );                                       \
}

#define MPRStartFrame(hRC)                                                  \
{                                                                           \
    MPRPktDDraw_t* DDrawPtr = (MPRPktDDraw_t*)(MPRBufPtr);                  \
                                                                            \
    if (IsNotEnoughRoomInCurrentPacket(sizeof(MPRPktDDraw_t))) {            \
        MPRSendCurrentPacket(hRC);                                          \
        DDrawPtr    = (MPRPktDDraw_t*)(MPRBufPtr);                          \
    }                                                                       \
                                                                            \
    DDrawPtr->PktDDrawPktType   = MPR_PKT_STARTFRAME;                       \
                                                                            \
    (UInt8*)(MPRBufPtr)   += sizeof(MPRPktDDraw_t);                         \
}

#define MPRFinishFrame(hRC, lpFnPtr)                                        \
{                                                                           \
    MPRPktDDraw_t* DDrawPtr = (MPRPktDDraw_t*) (MPRBufPtr);                 \
                                                                            \
    if (IsNotEnoughRoomInCurrentPacket(sizeof(MPRPktDDraw_t))) {            \
        MPRSendCurrentPacket(hRC);                                          \
        DDrawPtr    = (MPRPktDDraw_t*)(MPRBufPtr);                          \
    }                                                                       \
                                                                            \
    DDrawPtr->PktDDrawPktType   = MPR_PKT_FINISHFRAME;                      \
    DDrawPtr->PktDDrawData0     = (UInt)(lpFnPtr);                          \
                                                                            \
    (UInt8*)(MPRBufPtr)   += sizeof(MPRPktDDraw_t);                         \
                                                                            \
    MPRSendCurrentPacket(hRC);                                              \
}

// The following macros are intended to be used where it is known that the
// full size of each primitive fits within the remaining space in the buffer.
// In particular the MPRPrimitiveFAST() macro stuffs the header info for the
// primitive, the user MUST then fill the rest of the primitive structure
// out with vertex information etc.
#define MPRSetStateFAST(StaPtr, State, Value)                               \
    ((MPRPktSta_t*)(StaPtr))->PktStaPktType    = MPR_PKT_SETSTATE;          \
    ((MPRPktSta_t*)(StaPtr))->PktStaState      = (UInt)(State);          \
    ((MPRPktSta_t*)(StaPtr))->PktStaData       = (UInt)(Value);          \
    (UInt8*)(StaPtr)  += sizeof(MPRPktSta_t);

#define MPRRestoreStateIDFAST(StaPtr, StateID)                              \
    ((MPRPktSta_t*)(StaPtr))->PktStaPktType    = MPR_PKT_RESTORESTATEID;    \
    ((MPRPktSta_t*)(StaPtr))->PktStaData       = (UInt)(StateID);        \
    (UInt8*)(StaPtr)  += sizeof(MPRPktSta_t);

#define MPRPrimitiveFAST(PrmPtr, Prim, VtxInfo, Count, VtxSize)             \
    ((MPRPktPrm_t*)(PrmPtr))->PktPrmPktType    = (UInt) (Prim);          \
    ((MPRPktPrm_t*)(PrmPtr))->PktPrmVtxInfo    = (UInt) (VtxInfo);       \
    ((MPRPktPrm_t*)(PrmPtr))->PktPrmVtxNum     = (UInt) (Count);         \
    ((MPRPktPrm_t*)(PrmPtr))->PktPrmVtxSize    = (UInt) (VtxSize);       \
    (UInt8*)(PrmPtr)  += (sizeof(MPRPktPrm_t) - 4);   

#define MPRClearBuffersFAST(GenPktPtr, ClearInfo)                           \
    ((MPRPktGen_t*)(GenPktPtr))->PktGenPktType = MPR_PKT_CLEAR_BUFFERS;     \
    ((MPRPktGen_t*)(GenPktPtr))->PktGenInfo    = (UInt)(ClearInfo);      \
    (UInt8*)(GenPktPtr) += sizeof(MPRPktGen_t);

#define MPRSwapBuffersFAST(GenPktPtr, hRC, lpSurface)                       \
    ((MPRPktGen_t*)(GenPktPtr))->PktGenPktType = MPR_PKT_SWAP_BUFFERS;      \
    ((MPRPktGen_t*)(GenPktPtr))->PktGenInfo    = (UInt)(lpSurface);      \
    (UInt8*)(GenPktPtr)   += sizeof(MPRPktGen_t);                          \
    MPRSendCurrentPacket(hRC);

#define MPRStartFrameFAST(DDrawPtr)                                         \
    ((MPRPktDDraw_t*)(DDrawPtr))->PktDDrawPktType  = MPR_PKT_STARTFRAME;    \
    (UInt8*)(DDrawPtr)    += sizeof(MPRPktDDraw_t);                        \

#define MPRFinishFrameFAST(DDrawPtr, hRC, lpFnPtr)                          \
    ((MPRPktDDraw_t*)(DDrawPtr))->PktDDrawPktType = MPR_PKT_FINISHFRAME;    \
    ((MPRPktDDraw_t*)(DDrawPtr))->PktDDrawData0   = (UInt)(lpFnPtr);     \
    (UInt8*)(DDrawPtr)    += sizeof(MPRPktDDraw_t);                        \
    MPRSendCurrentPacket(hRC);

#else

#define MPRPacketsInit( )

#endif  /* USE_MPR_PACKETS */

#define MPRFalconRunning			"MPRFalconRunning"
#define MPRGlideInit				"MPRGlideInit"
#define MPRD3DMaxTextureWidth		"MPRD3DMaxTextureWidth"
#define MPRVoodooDevice				"MPRVoodooDevice"
#define MPRVoodooMaxResolution		"MPRVoodooMaxResolution"
#ifdef STBOF
#define MPRKey "SOFTWARE\\MicroProse\\Star Trek: BOF\\MPR"
#else
#define MPRKey "SOFTWARE\\MicroProse\\Falcon\\4.0\\MPR"
#endif


#ifdef  __cplusplus
};
#endif  /* __ cplusplus */

#endif /* _MPR_H_ */

