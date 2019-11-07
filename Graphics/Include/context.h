/***************************************************************************\
    Context.h
    Scott Randolph
    April 29, 1996

    Handle interaction with the MPR rendering context.  For packetized 
	requests, this class handles the command insertion.  For message based
	interactions, the application should use the RC to make MPR calls directly.
\***************************************************************************/
#ifndef _3DEJ_CONTEXT_H_
#define _3DEJ_CONTEXT_H_

//___________________________________________________________________________

#include "define.h"
#include "mpr_light.h"

struct IDirect3DDevice7;
struct IDirect3D7;
struct IDirectDraw7;
struct IDirectDrawSurface7;
struct IDirect3DDevice7;
struct IDirect3DVertexBuffer7;
struct _DDSURFACEDESC2;

class ImageBuffer;
class DXContext;
struct Poly;
struct Ptexcoord;
struct Tpoint;

enum PRIM_COLOR_OP
{
	PRIM_COLOP_NONE = 0,
	PRIM_COLOP_COLOR = 1,
	PRIM_COLOP_INTENSITY = 2,
	PRIM_COLOP_TEXTURE = 4,
	PRIM_COLOP_FOG = 8
};

//___________________________________________________________________________

// values for SetupMPRState flag argument
#define	CHECK_PREVIOUS_STATE							0x01
#define	ENABLE_DITHERING_STATE							0x02

enum {
	STATE_SOLID = 0,
    STATE_GOURAUD,
    STATE_TEXTURE,
    STATE_TEXTURE_PERSPECTIVE,
    STATE_TEXTURE_TRANSPARENCY,
    STATE_TEXTURE_TRANSPARENCY_PERSPECTIVE,
	STATE_TEXTURE_LIT,
    STATE_TEXTURE_LIT_PERSPECTIVE,
    STATE_TEXTURE_LIT_TRANSPARENCY,
    STATE_TEXTURE_LIT_TRANSPARENCY_PERSPECTIVE,
    STATE_TEXTURE_GOURAUD,
    STATE_TEXTURE_GOURAUD_TRANSPARENCY,
    STATE_TEXTURE_GOURAUD_PERSPECTIVE,
    STATE_TEXTURE_GOURAUD_TRANSPARENCY_PERSPECTIVE,

	STATE_ALPHA_SOLID,
    STATE_ALPHA_GOURAUD,
    STATE_ALPHA_TEXTURE,
    STATE_ALPHA_TEXTURE_PERSPECTIVE,
    STATE_ALPHA_TEXTURE_TRANSPARENCY,
    STATE_ALPHA_TEXTURE_TRANSPARENCY_PERSPECTIVE,
	STATE_ALPHA_TEXTURE_LIT,
    STATE_ALPHA_TEXTURE_LIT_PERSPECTIVE,
    STATE_ALPHA_TEXTURE_LIT_TRANSPARENCY,
    STATE_ALPHA_TEXTURE_LIT_TRANSPARENCY_PERSPECTIVE,
    STATE_ALPHA_TEXTURE_SMOOTH,
    STATE_ALPHA_TEXTURE_SMOOTH_TRANSPARENCY,
    STATE_ALPHA_TEXTURE_SMOOTH_PERSPECTIVE,
    STATE_ALPHA_TEXTURE_SMOOTH_TRANSPARENCY_PERSPECTIVE,
    STATE_ALPHA_TEXTURE_GOURAUD,
    STATE_ALPHA_TEXTURE_GOURAUD_PERSPECTIVE,
    STATE_ALPHA_TEXTURE_GOURAUD_TRANSPARENCY,
    STATE_ALPHA_TEXTURE_GOURAUD_TRANSPARENCY_PERSPECTIVE,

	STATE_FOG_TEXTURE,
    STATE_FOG_TEXTURE_PERSPECTIVE,
    STATE_FOG_TEXTURE_TRANSPARENCY,
    STATE_FOG_TEXTURE_TRANSPARENCY_PERSPECTIVE,
	STATE_FOG_TEXTURE_LIT,
    STATE_FOG_TEXTURE_LIT_PERSPECTIVE,
    STATE_FOG_TEXTURE_LIT_TRANSPARENCY,
    STATE_FOG_TEXTURE_LIT_TRANSPARENCY_PERSPECTIVE,
    STATE_FOG_TEXTURE_SMOOTH,
    STATE_FOG_TEXTURE_SMOOTH_TRANSPARENCY,
    STATE_FOG_TEXTURE_SMOOTH_PERSPECTIVE,
    STATE_FOG_TEXTURE_SMOOTH_TRANSPARENCY_PERSPECTIVE,
	STATE_FOG_TEXTURE_GOURAUD,
    STATE_FOG_TEXTURE_GOURAUD_PERSPECTIVE,
    STATE_FOG_TEXTURE_GOURAUD_TRANSPARENCY,
    STATE_FOG_TEXTURE_GOURAUD_TRANSPARENCY_PERSPECTIVE,

    STATE_TEXTURE_FILTER,
    STATE_TEXTURE_FILTER_PERSPECTIVE,

	STATE_TEXTURE_POLYBLEND,
    STATE_TEXTURE_POLYBLEND_GOURAUD,
    STATE_TEXTURE_FILTER_POLYBLEND,
    STATE_TEXTURE_FILTER_POLYBLEND_GOURAUD,

    STATE_APT_TEXTURE_FILTER_PERSPECTIVE,
    STATE_APT_FOG_TEXTURE_FILTER_PERSPECTIVE,

	STATE_APT_FOG_TEXTURE_PERSPECTIVE,
	STATE_APT_FOG_TEXTURE_SMOOTH_PERSPECTIVE,

	// OW
	STATE_TEXTURE_LIT_FILTER,
	STATE_TEXTURE_GOURAUD_FILTER,
	STATE_TEXTURE_TRANSPARENCY_FILTER,
	STATE_TEXTURE_LIT_TRANSPARENCY_FILTER,
	STATE_TEXTURE_GOURAUD_TRANSPARENCY_FILTER,
	STATE_TEXTURE_GOURAUD_TRANSPARENCY_PERSPECTIVE_FILTER,
	STATE_TEXTURE_LIT_TRANSPARENCY_PERSPECTIVE_FILTER,
	STATE_TEXTURE_TRANSPARENCY_PERSPECTIVE_FILTER,
	STATE_TEXTURE_GOURAUD_PERSPECTIVE_FILTER,
	STATE_TEXTURE_LIT_PERSPECTIVE_FILTER,
	STATE_TEXTURE_PERSPECTIVE_FILTER,
	STATE_ALPHA_TEXTURE_FILTER,
	STATE_ALPHA_TEXTURE_LIT_FILTER,
	STATE_ALPHA_TEXTURE_SMOOTH_FILTER,
	STATE_ALPHA_TEXTURE_GOURAUD_FILTER,
	STATE_ALPHA_TEXTURE_TRANSPARENCY_FILTER,
	STATE_ALPHA_TEXTURE_LIT_TRANSPARENCY_FILTER,
	STATE_ALPHA_TEXTURE_SMOOTH_TRANSPARENCY_FILTER,
	STATE_ALPHA_TEXTURE_GOURAUD_TRANSPARENCY_FILTER,
	STATE_ALPHA_TEXTURE_PERSPECTIVE_FILTER,
	STATE_ALPHA_TEXTURE_LIT_PERSPECTIVE_FILTER,
	STATE_ALPHA_TEXTURE_SMOOTH_PERSPECTIVE_FILTER,
	STATE_ALPHA_TEXTURE_GOURAUD_PERSPECTIVE_FILTER,
	STATE_ALPHA_TEXTURE_TRANSPARENCY_PERSPECTIVE_FILTER,
	STATE_ALPHA_TEXTURE_LIT_TRANSPARENCY_PERSPECTIVE_FILTER,
	STATE_ALPHA_TEXTURE_SMOOTH_TRANSPARENCY_PERSPECTIVE_FILTER,
	STATE_ALPHA_TEXTURE_GOURAUD_TRANSPARENCY_PERSPECTIVE_FILTER,

	MAXIMUM_MPR_STATE											
};


#if (MAXIMUM_MPR_STATE >= 64)
#error "Can have at most 64 prestored states (#define'd inside MPR) & we need one free"
#endif

typedef struct _TTVERTEX
{ 
	float sx; 
	float sy; 
	float sz; 
	float rhw; 
	float tu; 
	float tv; 
} TLVERTEX; 

typedef struct _TTLVERTEX
{ 
	float sx; 
	float sy; 
	float sz; 
	float rhw; 
	DWORD color; 
	float tu; 
	float tv; 
} TTLVERTEX; 

//___________________________________________________________________________

class ContextMPR
{
	public:
	ContextMPR ();
	virtual ~ContextMPR();

	BOOL Setup(ImageBuffer *pIB, DXContext *c);
	void Cleanup( void );
	void NewImageBuffer( UInt lpDDSBack );
	void SetState(WORD State, DWORD Value);
	void SetStateInternal(WORD State, DWORD Value);
	void ClearBuffers( WORD ClearInfo );
	void SwapBuffers( WORD SwapInfo );
	void StartFrame( void );
	void FinishFrame( void *lpFnPtr );
	void SetColorCorrection( DWORD color, float percent );
	void SetupMPRState(GLint flag=0);
	void SelectForegroundColor (GLint color);
	void SelectBackgroundColor (GLint color);
	void SelectTexture(GLint texID);
	void RestoreState(GLint state);
	void InvalidateState () { currentTexture = -1; m_colFG_Raw = 0x00ffffff; currentState = -1; }; // JPO -1 for FG is white, which happens!
	int CurrentForegroundColor(void) {return m_colFG_Raw;};
	void Render2DBitmap( int sX, int sY, int dX, int dY, int w, int h, int totalWidth, DWORD *source );

	public:	// This should be protected, but we need it for debugging elsewhere (SCR 12/5/97)
	static	GLint	StateSetupCounter;

	class State
	{
		public:
		State() { ZeroMemory(this, sizeof(*this)); }

		bool SE_SHADING;
		bool SE_TEXTURING;
		bool SE_MODULATION;
		bool SE_Z_BUFFERING;
		bool SE_FOG;
		bool SE_PIXEL_MASKING;
		bool SE_Z_MASKING;
		bool SE_PATTERNING;
		bool SE_SCISSORING;
		bool SE_CLIPPING;
		bool SE_BLENDING;
		bool SE_FILTERING;
		bool SE_TRANSPARENCY;
		bool SE_NON_PERSPECTIVE_CORRECTION_MODE;
		bool SE_NON_SUBPIXEL_PRECISION_MODE;
		bool SE_HARDWARE_OFF;
		bool SE_PALETTIZED_TEXTURES;
		bool SE_DECAL_TEXTURES;
		bool SE_ANTIALIASING;
		bool SE_DITHERING;
	};

	static UInt StateTable[MAXIMUM_MPR_STATE];			// D3D State block table
	static State StateTableInternal[MAXIMUM_MPR_STATE];		// Internal state table
	bool m_bUseSetStateInternal;

	GLint m_colFG_Raw;
	GLint m_colBG_Raw;
	GLint currentState;
	GLint currentTexture;
 
	protected:
	inline void SetStateTable (GLint state, GLint flag);
	inline void ClearStateTable (GLint state);
	void SetCurrentState (GLint state, GLint flag);
	void SetCurrentStateInternal(GLint state, GLint flag);
	void CleanupMPRState (GLint flag=0);
	inline void SetPrimitiveType(int nType);

	// OW
	//////////////////////////////////////

	// Attributes
	public:
	DXContext *m_pCtxDX;
	protected:
	int m_nFrameDepth;		// EndScene is called when this value reaches zero
	ImageBuffer *m_pIB;
	IDirectDraw7 *m_pDD;
	IDirect3DDevice7 *m_pD3DD;
	IDirect3DVertexBuffer7 *m_pVB;
	IDirectDrawSurface7 *m_pRenderTarget;
	DWORD m_colFG;
	DWORD m_colBG;
	RECT m_rcVP;
	bool m_bEnableScissors;
	DWORD m_dwVBSize;
	WORD *m_pIdx;
	DWORD m_dwNumVtx;
	DWORD m_dwNumIdx;
	DWORD m_dwStartVtx;
	DWORD m_dwStartIdx;
	short m_nCurPrimType;
	WORD m_VtxInfo;		// copy of argument for Primitive() (used during Primitive->StorePrimitiveVertexData phase)
	TTLVERTEX *m_pVtx;	// points to locked VB (only used with Primitive() call)
	#ifdef _DEBUG
	BYTE *m_pVtxEnd;	// points to end of locked VB (only used with Primitive() call)
	#endif
	IDirectDrawSurface7 *m_pDDSP;	// pointer to primary surface
	bool m_bNoD3DStatsAvail;	// D3D Texture management stats (dx debug runtime only)
	bool m_bLinear2Anisotropic;		// Set anisotropic filtering when bilinear is requested
	bool m_bRenderTargetHasZBuffer;
	bool m_bViewportLocked;

	// Stats
	class Stats
	{
		public:
		Stats();

		// Attributes
		public:
		DWORD dwLastFPS;
		DWORD dwMinFPS;
		DWORD dwMaxFPS;
		DWORD dwAverageFPS;
		DWORD dwTotalPrimitives;
		DWORD arrPrimitives[6];
		DWORD dwMaxVtxBatchSize;
		DWORD dwAvgVtxBatchSize;
		DWORD dwMaxPrimBatchSize;
		DWORD dwAvgPrimBatchSize;
		DWORD dwMaxVtxCountPerSecond;
		DWORD dwAvgVtxCountPerSecond;
		DWORD dwMaxPrimCountPerSecond;
		DWORD dwAvgPrimCountPerSecond;
		DWORD dwPutTextureTotal;
		DWORD dwPutTextureCached;

		protected:
		DWORD dwTicks;
		DWORD dwTotalSeconds;
		DWORD dwCurrentFPS;
		DWORD dwTotalFPS;
		__int64 dwTotalVtxCount;
		__int64 dwTotalPrimCount;
		__int64 dwTotalVtxBatchSize;
		__int64 dwTotalPrimBatchSize;
		DWORD dwTotalBatches;
		DWORD dwCurVtxBatchSize;
		DWORD dwCurPrimBatchSize;
		DWORD dwCurVtxCountPerSecond;
		DWORD dwCurPrimCountPerSecond;

		// Implementation
		protected:
		void Check();

		public:
		void Init();
		void StartFrame();
		void StartBatch();
		void Primitive(DWORD dwType, DWORD dwNumVtx);
		void PutTexture(bool bCached);
		void Report();
	};

	Stats m_stats;

	// Implementation
	inline DWORD MPRColor2D3DRGBA(GLint color);
	inline void UpdateViewport();
	inline bool LockVB(int nVtxCount, void **p);
	inline void UnlockVB();
	inline void FlushVB();
	static HRESULT WINAPI EnumSurfacesCB2(IDirectDrawSurface7 *lpDDSurface, struct _DDSURFACEDESC2 *lpDDSurfaceDesc, LPVOID lpContext);
	void Stats();

	// New Interface
	public:
	void DrawPoly(DWORD opFlag, Poly *poly, int *xyzIdxPtr, int *rgbaIdxPtr, int *IIdxPtr, Ptexcoord *uv, bool bUseFGColor = false);
	void Draw2DPoint(Tpoint *v0);
	void Draw2DPoint(float x, float y);
	void Draw2DLine(Tpoint *v0, Tpoint *v1);
	void Draw2DLine(float x0, float y0, float x1, float y1);
	void DrawPrimitive2D(int type, int nVerts, int *xyzIdxPtr);
	void DrawPrimitive(int type, WORD VtxInfo, WORD Count, MPRVtx_t *data, WORD Stride);
	void DrawPrimitive(int type, WORD VtxInfo, WORD Count, MPRVtxTexClr_t *data, WORD Stride);
	void DrawPrimitive(int type, WORD VtxInfo, WORD Count, MPRVtxTexClr_t **data);
	void TextOut(short x, short y, DWORD col, LPSTR str);
	void SetViewportAbs(int nLeft, int nTop, int nRight, int nBottom);
	void LockViewport();
	void UnlockViewport();
	void GetViewport(RECT *prc);
};

#endif // _3DEJ_CONTEXT_H_
