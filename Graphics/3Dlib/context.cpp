/***************************************************************************\
    Context.cpp
    Scott Randolph
    April 29, 1996

    Handle interaction with the MPR rendering context.  For packetized 
	requests, this class handles the command insertion.  For message based
	interactions, the application should use the RC to make MPR calls directly.

	OW
	- Code now uses a streaming vertex buffer 
\***************************************************************************/

#include "stdafx.h"
#include "ImageBuf.h"
#include "context.h"

// OW
#include "polylib.h"
#include "StateStack.h"
#include "render3d.h"
#include "FalcLib\include\playerop.h"

// OW needed for restoring textures after task switch
#include "graphics\include\texbank.h"
#include "graphics\include\fartex.h"
#include "graphics\include\terrtex.h"

extern bool g_bSlowButSafe;
extern bool g_bUseMipMaps;
extern float g_fMipLodBias;
extern int g_nFogRenderState;
extern bool g_bAlwaysAnisotropic;


// TODO - Graphics
/////////////////////////////////////////////////////////////////////////////

// - STATE_ALPHA_GOURAUD problematic with ATI Rage Pro and Rage 128
// - vtune run
// - optimize Render3D::DrawTriangle and ClipAndDraw2DFan
// - Fix C_3dViewer::ViewGreyOTW() and enable filtering and add grayscale tex mode
// - Why is dobackrect not initialized correctly for the SAR bar
// - Take a closer look at bsplib\texbank.cpp
// - crash in VoiceFilter::PlayRadioMessage
// - crash in TextureHandle::Reload
// - crash in ObjectDisplayList::DrawBeyond
// - 20 sec stuttering (re-enable texture pools ?)
// - Blend16BitTransparent crashes (randomly)
// - filtered terrain objects?

// - Track interface leaks
// - Try texture compression
// - eliminate need for DDSCL_FPUPRESERVE 


/*
- make cockpit views use dedicated context each
- static var v2 mouse cursor lost when entering flight
- why are the textures created in vidmem ? (texture manage)

gpSimCursors[i].CursorBuffer->Setup(&FalconDisplay.theDisplayDevice, gpSimCursors[i].Width, gpSimCursors[i].Height, SystemMem, None);
coverImage->Setup( OTWImage->GetDisplayDevice(), coverWidth, coverHeight, SystemMem, None );
gpTemplateSurface->Setup(&FalconDisplay.theDisplayDevice,
mapImage.Setup( device, srcRect.right, srcRect.bottom, SystemMem, None );
privateImage->Setup( &FalconDisplay.theDisplayDevice, MfdSize, MfdSize, SystemMem, None );
mpSurfaceBuffer->Setup(&FalconDisplay.theDisplayDevice, mWidth, mHeight, SystemMem, None);
*/

// Notes
/////////////////////////////////////////////////////////////////////////////

// - Crash Function == search in map file for ":"<Crashlog Address - 00400000 - 0x1000>


// - Falcon 1.08i with (5 4 6 5 6 1 4) Instant action paused ~18 FPS


/*
3Dfx Voodoo1:
        Problem:        Non-transparent polygons using textures with alpha channel are invisible (for example, when the alpha is used as a gloss map)
        Cause:          ALPHATESTENABLE always true.  Discards pixels according to the ALPHAREF and ALPHAFUNC states even with ALPHATESTENABLE false.
        Solution:       Also set an ALPHAFUNC of D3DCMP_ALWAYS  when setting ALPHATESTENABLE to false.

        Problem:        Transparency - polygons either invisible or completely opaque
        Cause:          When using 1:5:5:5 format, the Voodoo1 treats the one bit alpha as 0 or 1 - on other cards it is treated as to 0 or 255 (much more useful, IMO).
        Solution:       When setting values for ALPHAREF, use 1 instead of 128.

3Dfx Voodoo Rush:
        Same problems as Voodoo1

3Dfx Voodoo2, Banshee and Voodoo3:
        No problems encountered.  Just watch out for the separate texture memories on the Voodoo2 when doing multitexturing.

Nvidia Riva 128:
        Problem:        Black appears where transparency is expected (only seen with Diamond Drivers)
        Cause:          Riva 128 Drivers are notoriously buggy and inconsistant.  One rev will work fine, a later one will be full of problems.
        Solution:       Use the reference driver from Nvidia.

Nvidia TNT/TNT2:
        Problem:        Mip mapping always on.
        Cause:          Driver bug
        Solution:       Set the mipMapCount to 1 when creating textures.

ATI Rage Pro:
        Problem:        Particle effects like smoke won't fade out.
        Cause:          Hardware can't do alpha modulate.
        Solution:       Make sure the texture has some built in alpha so that there is a least some transparency with this card.

        Problem:        Unexpected blending artifacts (I got white or fogged appearance instead of transparency)
        Cause:          Driver bug where setting ALPHATESTENABLE to true also sets ALPHABLENDENABLE to true.
        Solution:       Always explicitly set the ALPHABLENDENABLE state after setting ALPHATESTENABLE

        Problem:        Texture comes out too dark where vertex lighting is not required.
        Cause:          Card does modulate even if the D3DTEXTUREOP is set to D3DTOP_SELECTARG1
        Solution:       Make sure vertex color is set to 0xffffffff when it is not needed.

ATI Rage 128:
        Problem:        Strange transparencies and validate errors after using multitexturing
        Cause:          When you set a stage to D3DTOP_DISABLE, the driver doesn't also disable the following stages
        Solution:       Explicitly set D3DTOP_DISABLE on all unused stages.

        Problem:        Z buffer fighting when doing multipass texturing (but only when rendering in a window, not fullscreen)
        Cause:          Not 100% sure.  May be a subpixel accuracy problem similar to the Matrox G200
        Solution:       None.  Didn't affect full screen mode, so not a problem for our game.

Matrox G200:
        Problem:        Z fighting with multipass environment mapping. 
        Cause:          Setting WRAP_U or WRAP_V causes the driver to use a different pipeline without subpixel accuracy
        Solution:       Don't use WRAP_U or WRAP_V (just live with the artifacts, or disable the effect)

Matrox G400:
        Problem:        Strange transparencies and validate errors after using multitexturing
        Cause:          Same as ATI Rage 128.  Driver not setting D3DTOP_DISABLE properly
        Solution:       Explicitly set D3DTOP_DISABLE on all unused stages.

        Problem:        Missing textures.
        Cause:          Setting D3DTSS_COLOROP to D3DTOP_DISABLE is not enough to completely disable stage (unlike what the Docs say)
        Solution:       Set both D3DTSS_COLOROP and D3DTSS_ALPHAOP to D3DTOP_DISABLE to completely disable a stage

S3 Savage:
        Problem:        Mip mapping always on. 
        Cause:          Driver bug of some kind
        Solution:       None.  The trick of setting the mipMapCount to 1 doesn't work on this card.

S3 Savage4
        Problem:        Z fighting artifacts with multipass texturing
        Cause:          Setting MIN and MAG filters to POINT with the MIP filter set to LINEAR
        Solution:       Set MIP filter to POINT or NONE when MIN and MAG are set to POINT.

3DLabs Permedia 2
        Problem:        Alpha Blending not working right
        Cause:          This card has a VERY limited set of valid blends (I think only 3 combinations of SRC and DEST actually work)
        Solution:       Use DeviceId to detect this card and disable stuff appropriately

        Problem:        Untextured polys not working (ColorArg2 set to D3DTA_DIFFUSE and colorOp set to D3DTOP_SELECTARG2)
        Cause:          D3DTOP_SELECTARG2 is invalid.
        Solution:       I recommend using D3DTOP_MODULATE and setting the texture to NULL.  Setting Arg1 to D3DTA_DIFFUSE breaks on some other cards.

        Problem:        Some transparencies not working
        Cause:          Card cannot do alpha compares.
        Solution:       Check for alphatest caps, and use Alphablend instead of Alphatest (and hope you don't get zbuffer problems) if it is not supported.

3DLabs Permedia 3
        No problems seen.


I'll just finish with a few comments for IHVs:

I want to thank Matrox for having the most helpful and responsive dev
support people.  They even offered to add a special Interstate 82 driver
workaround for the G200 environment mapping problems (which I declined, in
the interests of keeping drivers clean).

The big wooden spoon goes to Nvidia for their 8-stage blending hack.  All I
needed was to modulate with diffuse color in stage 2 (ie after combining two
textures).  They suggested their 8 stage hack approach, which tells me that
the hardware CAN do it.  So why isn't supported properly in the driver?  Not
being able to modulate with the diffuse color prevents a lot of cool
multitexturing effects from working right, but hacking Direct3D is not the
way to give us that functionality.  If everyone did it, we would be back to
writing unique code for each card, which goes against the whole philosophy
of DirectX.  Microsoft should not be certifying drivers that abuse the API
like this.

---------------------------------------------------------------------------------

Short answer:
Voodoo1-Voodoo3 cannot use D3DTOP_BLENDDIFFUSEALPHA to blend textures.
I apologize for this caps bit being set.  I shall have a word with our
driver people.  A strong word.
You will need to use two passes for a Diffuse blend.

Long answer:
Like Tom, I suggest that looking at the Glide manual is not a bad idea.
There are diagrams in the Programmer's Guide that show the paths through
the hardware, somewhat twisty and turny, but they are there.  So, that's a
last resort, when you're out of cows.

Here are the magic beans for the day:

On Voodoo1->Voodoo3, diffuse values (i.e. iterated RGBA) are available only
on the last stage, only on the stage closest to the framebuffer.  This
changes with Voodoo4, but that's another story.

So, if you are using only one texture stage (0), you have access to
diffuse.  If you are using 2 stages (0,1), then the diffuse is only
available on stage 1.  And if you are using 3 stages (0,1,2), then there's
a new twist: stage 2 cannot have its own texture (but it can use current
from the upstream textures) but it can use diffuse.

So you can't use diffuse values to blend textures.
Same thing goes for constant (factor) values.

Why is this?  Well, once upon a time (back on Voodoo Graphics with an
Amethyst, if you were lucky, and certainly back on Voodoo2 for most of
you), each stage was actually on its own chip (we called them TMU's or
Texture Mapping Units, and the FBI or Frame Buffer Interface).  Information
and color flows toward the frame buffer, not upstream (toward the TMUs).
Sending the iterated colors upstream or iterating them on each chip would
have been costly, so the TMUs handled textures but not iterated colors or
alpha.  The textures got blended according to mip levels or their alpha
channels and passed down to the FBI and then on to the framebuffer.  We
have kept the same architecture even when all the chips blended into one
(Banshee and Voodoo3).

So when you are using one or two stages, the last stage is actually the
combination of the TMU and FBI (that's why it can use iterated RGBA).  When
you use 3 stages, that is each chip individually
   TMU1 -> TMU0 -> FBI -> framebuffer

*/

#ifdef _DEBUG
//	#define _CONTEXT_ENABLE_STATS
	#define _CONTEXT_CHECK_PRIMITIVE_VERTICES_LEFT
	//#define _CONTEXT_RECORD_USED_STATES
	// #define _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
	//#define _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT_REPLACE
	//#define _CONTEXT_FLUSH_EVERY_PRIMITIVE
	//#define _CONTEXT_TRACE_ALL
	#define _CONTEXT_USE_MANAGED_TEXTURES
	//#define _VALIDATE_DEVICE

	static int m_nInstCount = 0;
	#ifdef _CONTEXT_RECORD_USED_STATES
	#include <set>
	static std::set<int> m_setStatesUsed;
	#endif

	#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT_REPLACE
	static bool bEnableRenderStateHighlightReplace = false;
	static int bRenderStateHighlightReplaceTargetState = 0;
	#endif
#endif

UInt ContextMPR::StateTable[MAXIMUM_MPR_STATE];
ContextMPR::State ContextMPR::StateTableInternal[MAXIMUM_MPR_STATE];
int ContextMPR::StateSetupCounter = 0;

ContextMPR::ContextMPR()
{ 
	#ifdef _DEBUG
	m_nInstCount++;
	#endif

	m_pCtxDX = NULL;
	m_pDD = NULL;
	m_pD3DD = NULL;
	m_pVB = NULL;
	m_dwVBSize = 0;
	m_pIdx = NULL;
	m_dwNumVtx = 0;
	m_dwNumIdx = 0;
	m_dwStartVtx = 0;
	m_nCurPrimType = 0;
	m_VtxInfo = 0;
	m_pRenderTarget = NULL;
	m_bEnableScissors = false;
	m_pDDSP = NULL;
	m_bNoD3DStatsAvail = false;
	m_bUseSetStateInternal = false;
	m_pIB = NULL;
	m_nFrameDepth = 0;
	m_pVtx = NULL;
	m_bRenderTargetHasZBuffer = false;
	m_bViewportLocked = false;

	m_colFG = m_colBG = 0; // JPO initialise to something
	m_colFG_Raw = m_colBG_Raw = 0; // and again
	#ifdef _DEBUG
	m_pVtxEnd = NULL;
	#endif
}

ContextMPR::~ContextMPR()
{ 
	#ifdef _DEBUG
	m_nInstCount--;
	#endif

//	Cleanup();
}

BOOL ContextMPR::Setup(ImageBuffer *pIB, DXContext *c)
{
	BOOL bRetval = FALSE;

	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::Setup(0x%X, 0x%X)\n", pIB, c);
	#endif

	try
	{
		m_pCtxDX = c;
//		m_pCtxDX->AddRef();

		if(!m_pCtxDX)
		{
			ShiWarning("Failed to create device!");
			return FALSE;
		}

		ShiAssert(m_pVtx == NULL);	// Cleanup() wasnt called if not zero

		ShiAssert(pIB);
		if(!pIB)
			return FALSE;

		m_pIB = pIB;
//		m_pIB->Lock(TRUE);
		IDirectDrawSurface7 *lpDDSBack = pIB->targetSurface();
		NewImageBuffer((UInt) lpDDSBack);

		// Keep own references to important interfaces
		m_pDD = m_pCtxDX->m_pDD;
//		m_pDD->AddRef();
		m_pD3DD = m_pCtxDX->m_pD3DD;
//		m_pD3DD->AddRef();

		// Get a pointer to the primary surface (for gamma control if supported)
/* Warning this generates an addref on the primary surface
		if(m_pCtxDX->m_pcapsDD->dwCaps2 & DDCAPS2_PRIMARYGAMMA)
			m_pDD->EnumSurfaces(DDENUMSURFACES_DOESEXIST | DDENUMSURFACES_ALL,
				NULL, this, EnumSurfacesCB2);
*/

		// Create the main VB
		IDirect3DVertexBuffer7Ptr p;
		D3DVERTEXBUFFERDESC vbdesc;
		ZeroMemory(&vbdesc, sizeof(vbdesc));
		vbdesc.dwSize = sizeof(vbdesc);
		vbdesc.dwFVF = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1;
		vbdesc.dwCaps = D3DVBCAPS_WRITEONLY | D3DVBCAPS_DONOTCLIP;

		if(m_pCtxDX->m_eDeviceCategory >= DXContext::D3DDeviceCategory_Hardware_TNL)
		{
		    //			m_dwVBSize = 1000;	// Any larger than 1000 and the driver refuses to put it in vidmem due to DMA restrictions
		    m_dwVBSize = min(2000, 0x10000 / sizeof(TTLVERTEX));
		}
		
		else
		{
		    m_dwVBSize = D3DMAXNUMVERTICES >> 2;
		    vbdesc.dwCaps |= D3DVBCAPS_SYSTEMMEMORY;  // Non HW TNL devices require that VBs reside in system memory
		}

		vbdesc.dwNumVertices = m_dwVBSize;	// MPR_MAX_VERTICES

		CheckHR(m_pCtxDX->m_pD3D->CreateVertexBuffer(&vbdesc, &m_pVB, NULL));

		// Alloc index buffer
		m_pIdx = new WORD[vbdesc.dwNumVertices * 3];	// we intend to sent triangle lists
		if(!m_pIdx) throw _com_error(E_OUTOFMEMORY);

		#ifdef _DEBUG
		D3DVERTEXBUFFERDESC ddsd;
		ZeroMemory(&ddsd, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		HRESULT hr = m_pVB->GetVertexBufferDesc(&ddsd);
		ShiAssert(SUCCEEDED(hr));

//		MonoPrint("ContextMPR::Setup - m_pVB created in %s memory (%d vtx)\n",
//			ddsd.dwCaps & D3DVBCAPS_SYSTEMMEMORY ? "SYSTEM" : "VIDEO",
//			ddsd.dwNumVertices);
		ShiAssert(ddsd.dwNumVertices == m_dwVBSize);
		#endif

		bool bCanDoAnisotropic = (m_pCtxDX->m_pD3DHWDeviceDesc->dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC) &&
			(m_pCtxDX->m_pD3DHWDeviceDesc->dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC);

		m_bLinear2Anisotropic = bCanDoAnisotropic && PlayerOptions.bHQFiltering;

// Hack for GF3 users - The D3D stuff doesn't see the GF3 as anisotropic capable with some drivers...
		if (g_bAlwaysAnisotropic)
		{
			bCanDoAnisotropic = true;
			if (PlayerOptions.bHQFiltering)
				m_bLinear2Anisotropic = true;
		}

		// Setup our set of cached rendering states
		SetupMPRState(CHECK_PREVIOUS_STATE);

		// Start out in simple flat shaded mode
		m_pD3DD->SetRenderState(D3DRENDERSTATE_COLORVERTEX, TRUE);
		m_pD3DD->SetRenderState(D3DRENDERSTATE_LIGHTING, FALSE);
		m_pD3DD->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
		m_pD3DD->SetRenderState(D3DRENDERSTATE_ZENABLE, FALSE);
		m_pD3DD->SetRenderState(D3DRENDERSTATE_FOGENABLE, FALSE);
		m_pD3DD->SetRenderState(D3DRENDERSTATE_STIPPLEDALPHA, FALSE);
		m_pD3DD->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE, FALSE);
		m_pD3DD->SetRenderState(D3DRENDERSTATE_STENCILENABLE, FALSE);   

		// Disable all stages
		for(int i=0;i<8;i++)
		{
			m_pD3DD->SetTextureStageState(i, D3DTSS_COLOROP, D3DTOP_DISABLE);
			m_pD3DD->SetTextureStageState(i, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		}

		// Setup stage 0
		m_pD3DD->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE);
		m_pD3DD->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT);
		m_pD3DD->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		m_pD3DD->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_CURRENT);

		if(g_bUseMipMaps)
		{
			m_pD3DD->SetTextureStageState(0, D3DTSS_MIPMAPLODBIAS, *((LPDWORD) (&g_fMipLodBias)));
			m_pD3DD->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTFP_LINEAR);
		}

		InvalidateState();
		RestoreState( STATE_SOLID );
		ZeroMemory(&m_rcVP, sizeof(m_rcVP));
		m_bViewportLocked = false;

		#ifdef _CONTEXT_ENABLE_STATS
		m_stats.Init();
		m_stats.StartBatch();
		#endif


		bRetval = TRUE;
	}

	catch(_com_error e)
	{
		MonoPrint("ContextMPR::Setup - Error 0x%X\n", e.Error());
	}

//	if(m_pIB)
//		m_pIB->Unlock();

	return bRetval;
}


void ContextMPR::Cleanup()
{
	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::Cleanup()\n");
	#endif

	#ifdef _DEBUG
	#ifdef _CONTEXT_ENABLE_STATS
	m_stats.Report();
	#endif
	#endif

/*
	if(m_nFrameDepth && m_pIB)
	{
		ShiAssert(false);	// our image buffer is still locked!!
		m_pIB->Unlock();
	}
*/

	if(StateSetupCounter)
		CleanupMPRState(CHECK_PREVIOUS_STATE);

	// Warning: The SIM code uses a shared DXContext which might be already toast when this function gets called!!
	// Under no circumstances access m_pCtxDX here
	// Btw: this was causing the infamous LGB CTD

	// JPO - now added reference counting so that it is shareable and releaseable - I think.
//	m_pCtxDX->Release();
	m_pCtxDX = NULL;



/*	Can be all toast now dont touch anything ;(  (Note this is causing VertexBufffer Memory leaks)
*/
#if 0
	if(m_pRenderTarget)
	{
		m_pRenderTarget->Release();
		m_pRenderTarget = NULL;
	}
	
	if(m_pVB)
	{
		m_pVB->Release();
		m_pVB = NULL;
	}

	if(m_pDDSP)
	{
		m_pDDSP->Release();
		m_pDDSP = NULL;
	}

	if(m_pD3DD)
	{
		DWORD dwRefCnt = m_pD3DD->Release();
		m_pD3DD = NULL;
	}

	if(m_pDD)
	{
		DWORD dwRefCnt = m_pDD->Release();
		m_pDD = NULL;
	}
#endif
// Only this release seems to work without problems, good that it is the largest one...
	if(m_pVB)
	{
		m_pVB->Release();
		m_pVB = NULL;
	}

	if(m_pIdx)
	{
		delete[] m_pIdx;
		m_pIdx = NULL;
	}

	m_pIdx = NULL;
	m_dwNumVtx = 0;
	m_dwNumIdx = 0;
	m_dwStartVtx = 0;
	m_nCurPrimType = 0;
	m_VtxInfo = 0;
	m_pIB = NULL;
	m_nFrameDepth = 0;
	m_pVtx = NULL;
	m_bRenderTargetHasZBuffer = false;

	#ifdef _DEBUG
	m_pVtxEnd = NULL;
	#endif

	#ifdef _CONTEXT_RECORD_USED_STATES
	MonoPrint("ContextMPR::Cleanup - Report of used states follows\n	");
	std::set<int>::iterator it;
	for(it = m_setStatesUsed.begin(); it != m_setStatesUsed.end(); it++)
		MonoPrint("%d, ", *it);
	m_setStatesUsed.clear();
	MonoPrint("\nContextMPR::Cleanup - End of report\n	");
	#endif
}

void ContextMPR::NewImageBuffer( UInt lpDDSBack)
{
	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::NewImageBuffer(0x%X)\n", lpDDSBack);
	#endif

	if(m_pRenderTarget)
	{
//		m_pRenderTarget->Release();
		m_pRenderTarget = NULL;
	}

	m_pRenderTarget = (IDirectDrawSurface7 *) lpDDSBack;
//	m_pRenderTarget->AddRef();

	// weird, some drivers (like the 3.68 detonators) implicitely create Z buffers
	if(m_pRenderTarget)
	{
		IDirectDrawSurface7Ptr pDDS;

		DDSCAPS2 ddscaps;
		ZeroMemory(&ddscaps, sizeof(ddscaps));
		ddscaps.dwCaps = DDSCAPS_ZBUFFER; 
		m_bRenderTargetHasZBuffer = SUCCEEDED(m_pRenderTarget->GetAttachedSurface(&ddscaps, &pDDS)); 
	}
}

void ContextMPR::ClearBuffers(WORD ClearInfo) 
{
	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::ClearBuffers(0x%X)\n", ClearInfo);
	#endif

	DWORD dwClearFlags = 0;
	if(ClearInfo & MPR_CI_DRAW_BUFFER) dwClearFlags |= D3DCLEAR_TARGET;
	if(ClearInfo & MPR_CI_ZBUFFER) dwClearFlags |= D3DCLEAR_ZBUFFER;

	HRESULT hr = m_pD3DD->Clear(NULL, NULL, dwClearFlags, m_colBG, 1.0f, NULL);
	ShiAssert(SUCCEEDED(hr));
}

void ContextMPR::StartFrame()
{
	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::StartFrame()\n");
	#endif

	// OW FIXME: is this ok?
	if(m_pCtxDX->SetRenderTarget(m_pRenderTarget))	// returns false if render target unchanged
		UpdateViewport();

	InvalidateState();
//	m_pIB->Lock(TRUE);

	HRESULT hr;
	
	if(m_bRenderTargetHasZBuffer)
		hr = m_pD3DD->Clear(NULL, NULL, D3DCLEAR_ZBUFFER, 0, 0.0f, NULL);   // Clear the ZBuffer

	hr = m_pD3DD->BeginScene();
	if(FAILED(hr))
	{
		MonoPrint("ContextMPR::FinishFrame - BeginScene failed 0x%X\n", hr);

		if(hr == DDERR_SURFACELOST)
		{
			MonoPrint("ContextMPR::StartFrame - Restoring all surfaces\n", hr);

			TheTextureBank.RestoreAll();
			TheTerrTextures.RestoreAll();
			TheFarTextures.RestoreAll();

			hr = m_pD3DD->EndScene();

			if(FAILED(hr))
			{
				MonoPrint("ContextMPR::StartFrame - Retry for BeginScene failed 0x%X\n", hr);
				return;
			}
		}
	}

	#if defined _DEBUG && defined _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT_REPLACE
	if(bEnableRenderStateHighlightReplace)
	{
		Sleep(1000);
		if(GetKeyState(VK_F4) & ~1)
			DebugBreak();
		bRenderStateHighlightReplaceTargetState++;
	}
	#endif

#if 0
#ifdef _DEBUG
	static int n = D3DTFP_LINEAR;
	m_pD3DD->SetTextureStageState(0, D3DTSS_MIPFILTER, n);
	static float f = 0;
	m_pD3DD->SetTextureStageState(0, D3DTSS_MIPMAPLODBIAS, *((LPDWORD) (&f)) );
#endif
#endif
}

void ContextMPR::FinishFrame(void *lpFnPtr)
{
	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::FinishFrame(0x%X)\n", lpFnPtr);
	#endif

	FlushVB();

	HRESULT hr = m_pD3DD->EndScene();
	//ShiAssert(SUCCEEDED(hr));
	if(FAILED(hr))
	{
		MonoPrint("ContextMPR::FinishFrame - EndScene failed 0x%X\n", hr);

		if(hr == DDERR_SURFACELOST)
		{
			MonoPrint("ContextMPR::FinishFrame - Restoring all surfaces\n", hr);

			TheTextureBank.RestoreAll();
			TheTerrTextures.RestoreAll();
			TheFarTextures.RestoreAll();

			hr = m_pD3DD->EndScene();

			if(FAILED(hr))
			{
				MonoPrint("ContextMPR::FinishFrame - Retry for EndScene failed 0x%X\n", hr);
				return;
			}
		}
	}

	#ifdef _CONTEXT_ENABLE_STATS
	m_stats.StartFrame();
	#if 0
	char buf[20];
	sprintf(buf, "%.2d FPS", m_stats.dwLastFPS);
	TextOut(0, 40, RGB(0xff, 0xff, 0xff), (char *) buf);
	#endif
	#endif

//	m_pIB->Unlock();

	ShiAssert(lpFnPtr == NULL);		// no idea what to do with it
	Stats();
}

void ContextMPR::SetColorCorrection( DWORD color, float percent )
{
	SetState( MPR_STA_RAMP_COLOR, color);
	SetState( MPR_STA_RAMP_PERCENT, *(DWORD*)(&percent));
}

void ContextMPR::SetState( WORD State, DWORD Value )
{
	if(m_bUseSetStateInternal)
	{
		SetStateInternal(State, Value);
		return;
	}

	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::SetState(%d, 0x%X)\n", State, Value);
	#endif
	ShiAssert(FALSE == F4IsBadReadPtr(m_pD3DD, sizeof *m_pD3DD));
	ShiAssert (State != MPR_STA_TEX_ID);
	ShiAssert (State != MPR_STA_FG_COLOR);

	if (!m_pD3DD)
		return;

	switch(State)
	{
		case MPR_STA_ENABLES:               /* use MPR_SE_...   */
		{
			if(Value & MPR_SE_MODULATION)
			{
				FlushVB();

				// Handled via FVF change 
				m_pD3DD->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
			}

			if(Value & MPR_SE_CLIPPING)
			{
				FlushVB();

				m_pD3DD->SetRenderState(D3DRENDERSTATE_CLIPPING, TRUE);
			}

			if(Value & MPR_SE_BLENDING)
			{
				FlushVB();

				m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
				m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, FALSE);

				// Also modulate alpha from vertex and texture
// OW, removed 23-07-2000 to fix cloud rendering problem when the transparency options was enabled in falcon
//				m_pD3DD->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
			}

			if(Value & MPR_SE_TRANSPARENCY)
			{
				FlushVB();

				// Use alpha testing to speed it up (if supported) - note: this is not redundant
				if(m_pCtxDX->m_pD3DHWDeviceDesc->dpcTriCaps.dwAlphaCmpCaps & D3DCMP_GREATEREQUAL)
				{
					m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, TRUE);
					m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHAREF, (DWORD) 1);
					m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHAFUNC, D3DCMP_GREATEREQUAL);
				}

				m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
				m_pD3DD->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
				m_pD3DD->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
			}

			if(Value & MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE)
			{
				FlushVB();

				m_pD3DD->SetRenderState(D3DRENDERSTATE_TEXTUREPERSPECTIVE , FALSE);
			}

			if(Value & MPR_SE_SHADING)
			{
				FlushVB();

				m_pD3DD->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD);
			}

			if(Value & MPR_SE_DITHERING)
			{
				FlushVB();

				m_pD3DD->SetRenderState(D3DRENDERSTATE_DITHERENABLE , TRUE);

				// see MPRcfg.h (MPR_BILINEAR_CLAMPED)
				m_pD3DD->SetTextureStageState(0, D3DTSS_ADDRESS, D3DTADDRESS_CLAMP);
			}

			if((Value & MPR_SE_SCISSORING) && !m_bEnableScissors)
			{
				FlushVB();

				m_bEnableScissors = true;
				UpdateViewport();
			}

			#ifdef _DEBUG
			if(Value & MPR_SE_TEXTURING)
			{
				// Enabled by selecting a texture into the device
			}

			if(Value & MPR_SE_FOG)
			{
#if 0
				m_pD3DD->SetRenderState(D3DRENDERSTATE_FOGENABLE, TRUE);

				float fStart = 0.1f;    // for linear mode
				float fEnd   = 0.8f;
				float fDensity = 0.66;  // for exponential modes
				m_pD3DD->SetRenderState(D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_LINEAR);	// D3DFOG_EXP, D3DFOG_EXP2
				m_pD3DD->SetRenderState(D3DRENDERSTATE_FOGSTART, *(DWORD *)(&fStart));
				m_pD3DD->SetRenderState(D3DRENDERSTATE_FOGEND,   *(DWORD *)(&fEnd));
#endif
			}

			if(Value & MPR_SE_Z_BUFFERING)
			{
				// FlushVB();

				ShiAssert(false);		// we do not create Z buffers for now
				// HRESULT hr = m_pD3DD->SetRenderState(D3DRENDERSTATE_ZENABLE, TRUE);
				// ShiAssert(SUCCEEDED(hr));
			}

			if(Value & MPR_SE_FILTERING)
			{
				// Handled by filter function setting
			}

			if(Value & MPR_SE_PIXEL_MASKING)
			{
				ShiAssert(false);
			}

			if(Value & MPR_SE_Z_MASKING)
			{
				ShiAssert(false);
			}

			if(Value & MPR_SE_PATTERNING)
			{
				ShiAssert(false);
			}

			if(Value & MPR_SE_NON_SUBPIXEL_PRECISION_MODE)
			{
				ShiAssert(false);
			}

			if(Value & MPR_SE_HARDWARE_OFF)
			{
				ShiAssert(false);
			}

			if(Value & MPR_SE_PALETTIZED_TEXTURES)
			{
				// OW FIXME: No idea
			}

			if(Value & MPR_SE_DECAL_TEXTURES)
			{
				ShiAssert(false);
			}

			if(Value & MPR_SE_ANTIALIASING)
			{
				ShiAssert(false);
			}
			#endif

			break;
		}

		case MPR_STA_DISABLES:              /* use MPR_SE_...   */
		{
			if(Value & MPR_SE_MODULATION)
			{
				FlushVB();

				m_pD3DD->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
			}

			if(Value & MPR_SE_TEXTURING)
			{
				FlushVB();

				m_pD3DD->SetTexture(0, NULL);
			}

			if(Value & MPR_SE_SHADING)
			{
				FlushVB();

				m_pD3DD->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_FLAT);
			}

			if(Value & MPR_SE_BLENDING)
			{
				FlushVB();

				m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);

				// Only use texture alpha
				m_pD3DD->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
			}

			if(Value & MPR_SE_FILTERING)
			{
				FlushVB();

				m_pD3DD->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_POINT);
				m_pD3DD->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFN_POINT);
			}

			if(Value & MPR_SE_TRANSPARENCY)
			{
				FlushVB();

				// Use alpha testing to speed it up (if supported)
				if(m_pCtxDX->m_pD3DHWDeviceDesc->dpcTriCaps.dwAlphaCmpCaps & D3DCMP_GREATEREQUAL)
				{
					m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, FALSE);

					// For Voodoo 1
					m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHAFUNC, D3DCMP_ALWAYS);
				}

				m_pD3DD->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
			}

			if(Value & MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE)
			{
				FlushVB();

				m_pD3DD->SetRenderState(D3DRENDERSTATE_TEXTUREPERSPECTIVE, TRUE);
			}

			if(Value & MPR_SE_DITHERING)
			{
				FlushVB();

				m_pD3DD->SetRenderState(D3DRENDERSTATE_DITHERENABLE , FALSE);
			}

			if((Value & MPR_SE_SCISSORING) && m_bEnableScissors)
			{
				FlushVB();

				m_bEnableScissors = false;
				UpdateViewport();
			}

			if(Value & MPR_SE_CLIPPING)
			{
				FlushVB();

				m_pD3DD->SetRenderState(D3DRENDERSTATE_CLIPPING, FALSE);
			}

			if(Value & MPR_SE_FOG)
			{
				FlushVB();

				m_pD3DD->SetRenderState(D3DRENDERSTATE_FOGENABLE, FALSE);
			}

			if(Value & MPR_SE_Z_BUFFERING)
			{
				FlushVB();

				m_pD3DD->SetRenderState(D3DRENDERSTATE_ZENABLE, FALSE);
			}

			#ifdef _DEBUG
			if(Value & MPR_SE_PIXEL_MASKING)
			{
				ShiAssert(false);
			}

			if(Value & MPR_SE_Z_MASKING)
			{
				ShiAssert(false);
			}

			if(Value & MPR_SE_PATTERNING)
			{
				ShiAssert(false);
			}

			if(Value & MPR_SE_NON_SUBPIXEL_PRECISION_MODE)
			{
				ShiAssert(false);
			}

			if(Value & MPR_SE_HARDWARE_OFF)
			{
				ShiAssert(false);
			}

			if(Value & MPR_SE_PALETTIZED_TEXTURES)
			{
				// OW FIXME: No idea
			}

			if(Value & MPR_SE_DECAL_TEXTURES)
			{
				ShiAssert(false);
			}

			if(Value & MPR_SE_ANTIALIASING)
			{
				ShiAssert(false);
			}
			#endif

			break;
		}

		case MPR_STA_SRC_BLEND_FUNCTION:    /* use MPR_BF_...   */
		{
			FlushVB();

			m_pD3DD->SetRenderState(D3DRENDERSTATE_SRCBLEND, Value);

			break;
		}

		case MPR_STA_DST_BLEND_FUNCTION:    /* use MPR_BF_...   */
		{
			FlushVB();

			m_pD3DD->SetRenderState(D3DRENDERSTATE_DESTBLEND, Value);

			break;
		}

		case MPR_STA_TEX_FILTER:            /* use MPR_TX_...   */
		{
			FlushVB();

			switch(Value)
			{
				case MPR_TX_NONE:
				case MPR_TX_DITHER:                /* Dither the colors */
				{
					m_pD3DD->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_POINT);
					m_pD3DD->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFN_POINT);
					break;
				}

				case MPR_TX_BILINEAR:              /* interpolate 4 pixels     */
				case MPR_TX_BILINEAR_NOCLAMP:              /* interpolate 4 pixels     */
				{
					if(m_bLinear2Anisotropic)
					{
						m_pD3DD->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_ANISOTROPIC);
						m_pD3DD->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFN_ANISOTROPIC);
						m_pD3DD->SetTextureStageState(0, D3DTSS_MAXANISOTROPY, m_pCtxDX->m_pD3DHWDeviceDesc->dwMaxAnisotropy );
					}

					else
					{
						m_pD3DD->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
						m_pD3DD->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
					}

					if(Value == MPR_TX_BILINEAR)
					{
						// see MPRcfg.h (MPR_BILINEAR_CLAMPED)
						m_pD3DD->SetTextureStageState(0, D3DTSS_ADDRESS, D3DTADDRESS_CLAMP);
					}

					break;
				}

				case MPR_TX_MIPMAP_NEAREST:   /* nearest mipmap       */
				{
					m_pD3DD->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTFP_POINT);
					break;
				}

				case MPR_TX_MIPMAP_LINEAR:         /* interpolate between mipmaps  */
				case MPR_TX_MIPMAP_BILINEAR:       /* interpolate 4x within mipmap */
				case MPR_TX_MIPMAP_TRILINEAR:      /* interpolate mipmaps,4 pixels */
				{
					m_pD3DD->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTFP_LINEAR);
					break;
				}
			}

			break;
		}

		case MPR_STA_TEX_FUNCTION:          /* use MPR_TF_...   */
		{
			FlushVB();

			switch(Value)
			{
				case MPR_TF_MULTIPLY:
				{
					m_pD3DD->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
					break;
				}

				case MPR_TF_ALPHA:
				{
					m_pD3DD->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_BLENDDIFFUSEALPHA);
					break;
				}

				case MPR_TF_NONE:
				case MPR_TF_ADD:
				case MPR_TF_FOO:
				case MPR_TF_SHADE:
				{
					ShiAssert(false);		// unused
					break;
				}
			}

			break;
		}

		#if defined(MPR_MASKING_ENABLED)
		case MPR_STA_AREA_MASK:             /* Area pattern bitmask */
		{
			ShiAssert(false);
			break;
		}

		case MPR_STA_LINE_MASK:             /* Line pattern bitmask */
		{
			ShiAssert(false);
			break;
		}

		case MPR_STA_PIXEL_MASK:            /* RGBA or bitmask  */
		{
			ShiAssert(false);
			break;
		}

		#endif
		case MPR_STA_FG_COLOR:              /* long: RGBA or index  */
		{
			SelectForegroundColor(Value);
			break;
		}

		case MPR_STA_BG_COLOR:              /* long: RGBA or index  */
		{
			SelectBackgroundColor(Value);
			break;
		}


		#if defined(MPR_DEPTH_BUFFER_ENABLED)
		case MPR_STA_Z_FUNCTION:            /* use MPR_ZF_...   */
		{
			ShiAssert(false);
			break;
		}

		case MPR_STA_FG_DEPTH:              /* FIXED 16.16 for raster*/
		{
			ShiAssert(false);
			break;
		}

		case MPR_STA_BG_DEPTH:              /* FIXED 16.16 for zclear*/
		{
			ShiAssert(false);
			break;
		}

		#endif

		case MPR_STA_TEX_ID:                /* Handle for current texture.*/
		{
			ShiAssert(false);
			break;
		}

		case MPR_STA_FOG_COLOR:             /* long: RGBA       */
		{
			FlushVB();
			// OW FIXME: hmm whats the correlation of this state with the fogColor value used in TheStateStack.fogValue ???
			if (g_nFogRenderState & 0x01)
				m_pD3DD->SetRenderState(D3DRENDERSTATE_FOGCOLOR, MPRColor2D3DRGBA(Value));
			break;
		}

		case MPR_STA_HARDWARE_ON:           /* Read only - set if hardware supports mode */
		{
			ShiAssert(false);
			break;
		}

		//#if !defined(MPR_GAMMA_FAKE)
		case MPR_STA_GAMMA_RED:             /* Gamma correction term for red (set before blue)  */
		{
// called for software devices
//			ShiAssert(false);
			break;
		}

		case MPR_STA_GAMMA_GREEN:           /* Gamma correction term for green (set before blue) */
		{
// called for software devices
//			ShiAssert(false);
			break;
		}

		case MPR_STA_GAMMA_BLUE:            /* Gamma correction term for blue (set last) */
		{
// called for software devices
//			ShiAssert(false);
			break;
		}

		//#else
		case MPR_STA_RAMP_COLOR:           /* Packed color for the ramp table        */
		{
			/*
			IDirectDrawGammaControlPtr p(m_pDDSP);

			if(p)
			{
			}
			*/

			break;
		}

		case MPR_STA_RAMP_PERCENT:         /* fractional (0.0=normal: 1.0=saturated) */
		{
			// OW FIXME: todo
			break;
		}

		//#endif

		case MPR_STA_SCISSOR_LEFT:  
		{
			if(Value != m_rcVP.left)
			{
				FlushVB();

				m_rcVP.left = Value;
				UpdateViewport();
			}
			break;
		}

		case MPR_STA_SCISSOR_TOP:  
		{
			if(Value != m_rcVP.top)
			{
				FlushVB();

				m_rcVP.top = Value;
				UpdateViewport();
			}

			break;
		}

		case MPR_STA_SCISSOR_RIGHT:         /* right:bottom: not inclusive*/
		{
			if(Value != m_rcVP.right)
			{
				FlushVB();

				m_rcVP.right = Value;	// not inclusive
				UpdateViewport();
			}

			break;
		}

		case MPR_STA_SCISSOR_BOTTOM:        /* Validity Check done here.    */
		{
			if(Value != m_rcVP.bottom)
			{
				FlushVB();

				m_rcVP.bottom = Value;	// not inclusive
				UpdateViewport();
			}

			break;
		}

		case MPR_STA_NONE:
		{
			break;
		}
	}
}

void ContextMPR::SetStateInternal(WORD State, DWORD Value )
{
	// This is for internal state tracking - currently only enable/disable is tracked
	ShiAssert (State != MPR_STA_TEX_ID);
	ShiAssert (State != MPR_STA_FG_COLOR);

	switch(State)
	{
		case MPR_STA_NONE:
		{
			break;
		}

		case MPR_STA_DISABLES:              /* use MPR_SE_...   */
		case MPR_STA_ENABLES:               /* use MPR_SE_...   */
		{
			bool bNewVal = (State == MPR_STA_ENABLES) ? true : false;

			if(Value & MPR_SE_SCISSORING)
				StateTableInternal[currentState].SE_SCISSORING = bNewVal;

			if(Value & MPR_SE_MODULATION)
				StateTableInternal[currentState].SE_MODULATION = bNewVal;

			if(Value & MPR_SE_TEXTURING)
				StateTableInternal[currentState].SE_TEXTURING = bNewVal;

			if(Value & MPR_SE_SHADING)
				StateTableInternal[currentState].SE_SHADING = bNewVal;

			if(Value & MPR_SE_FOG)
				StateTableInternal[currentState].SE_FOG = bNewVal;

			if(Value & MPR_SE_PIXEL_MASKING)
				StateTableInternal[currentState].SE_PIXEL_MASKING = bNewVal;

			if(Value & MPR_SE_Z_BUFFERING)
				StateTableInternal[currentState].SE_Z_BUFFERING = bNewVal;

			if(Value & MPR_SE_Z_MASKING)
				StateTableInternal[currentState].SE_Z_MASKING = bNewVal;

			if(Value & MPR_SE_PATTERNING)
				StateTableInternal[currentState].SE_PATTERNING = bNewVal;

			if(Value & MPR_SE_CLIPPING)
				StateTableInternal[currentState].SE_CLIPPING = bNewVal;

			if(Value & MPR_SE_BLENDING)
				StateTableInternal[currentState].SE_BLENDING = bNewVal;

			if(Value & MPR_SE_FILTERING)
				StateTableInternal[currentState].SE_FILTERING = bNewVal;

			if(Value & MPR_SE_TRANSPARENCY)
				StateTableInternal[currentState].SE_TRANSPARENCY = bNewVal;

			if(Value & MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE)
				StateTableInternal[currentState].SE_NON_PERSPECTIVE_CORRECTION_MODE = bNewVal;

			if(Value & MPR_SE_NON_SUBPIXEL_PRECISION_MODE)
				StateTableInternal[currentState].SE_NON_SUBPIXEL_PRECISION_MODE = bNewVal;

			if(Value & MPR_SE_HARDWARE_OFF)
				StateTableInternal[currentState].SE_HARDWARE_OFF = bNewVal;

			if(Value & MPR_SE_PALETTIZED_TEXTURES)
				StateTableInternal[currentState].SE_PALETTIZED_TEXTURES = bNewVal;

			if(Value & MPR_SE_DECAL_TEXTURES)
				StateTableInternal[currentState].SE_DECAL_TEXTURES = bNewVal;

			if(Value & MPR_SE_ANTIALIASING)
				StateTableInternal[currentState].SE_ANTIALIASING = bNewVal;

			if(Value & MPR_SE_DITHERING)
				StateTableInternal[currentState].SE_DITHERING = bNewVal;

			break;
		}
	}
}

// flag & 0x01  --> skip StateSetupCount checking --> reset/set state
// flag & 0x02  --> enable texture filtering
// flag & 0x04  --> enable dithering
void ContextMPR::SetCurrentState (GLint state, GLint flag)
{
	UInt32	i = 0;

	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::SetCurrentState (%d, 0x%X)\n", state, flag);
	#endif

	ShiAssert(FALSE == F4IsBadReadPtr(m_pD3DD, sizeof *m_pD3DD));

	// OW FIXME: optimize
	// see MPRcfg.h (MPR_BILINEAR_CLAMPED)
	m_pD3DD->SetTextureStageState(0, D3DTSS_ADDRESS, D3DTADDRESS_WRAP);

	switch(state)
	{
		case STATE_SOLID:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_TEXTURING | 
				MPR_SE_MODULATION | 
				MPR_SE_SHADING |
				MPR_SE_FOG );

			// Never dither in flat shaded mode...
			SetState (MPR_STA_DISABLES, MPR_SE_DITHERING);
			break;

		case STATE_GOURAUD:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_TEXTURING | 
				MPR_SE_MODULATION | 
				MPR_SE_FOG );

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_ENABLES, MPR_SE_SHADING);
			break;

		case STATE_TEXTURE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING;
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_TEXTURE_LIT:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_MODULATION;
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_TEXTURE_LIT_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_MODULATION;
			SetState ( MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE );
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_TEXTURE_GOURAUD:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_SHADING | MPR_SE_MODULATION;
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_TEXTURE_TRANSPARENCY:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_TRANSPARENCY;
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_TEXTURE_LIT_TRANSPARENCY:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_TRANSPARENCY | MPR_SE_MODULATION;
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_TEXTURE_LIT_TRANSPARENCY_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_TRANSPARENCY | MPR_SE_MODULATION;
			SetState ( MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE );
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_TEXTURE_GOURAUD_TRANSPARENCY:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_TRANSPARENCY | 
				MPR_SE_SHADING | MPR_SE_MODULATION;
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_TEXTURE_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING;
			SetState (	MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_TEXTURE_GOURAUD_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_SHADING | MPR_SE_MODULATION;
			SetState (	MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_TEXTURE_TRANSPARENCY_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_TRANSPARENCY;
			SetState (	MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_TEXTURE_GOURAUD_TRANSPARENCY_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_TRANSPARENCY | 
				MPR_SE_SHADING | MPR_SE_MODULATION;
			SetState (	MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_ALPHA_SOLID:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_TEXTURING | 
				MPR_SE_MODULATION | 
				MPR_SE_SHADING |
				MPR_SE_FOG );

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING;
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_ALPHA_GOURAUD:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_TEXTURING | 
				MPR_SE_MODULATION | 
				MPR_SE_FOG );

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING;
			i |= MPR_SE_SHADING;
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_ALPHA_TEXTURE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING;
			i |= MPR_SE_TEXTURING;
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_ALPHA_TEXTURE_LIT:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING;
			i |= MPR_SE_TEXTURING | MPR_SE_MODULATION;
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_ALPHA_TEXTURE_LIT_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING;
			i |= MPR_SE_TEXTURING | MPR_SE_MODULATION;
			SetState (	MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_ALPHA_TEXTURE_SMOOTH:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING| MPR_SE_TEXTURING | MPR_SE_SHADING;
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_ALPHA_TEXTURE_SMOOTH_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING| MPR_SE_TEXTURING | MPR_SE_SHADING;
			SetState (	MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_ALPHA_TEXTURE_SMOOTH_TRANSPARENCY:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING| MPR_SE_TEXTURING | MPR_SE_SHADING | MPR_SE_TRANSPARENCY;
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_ALPHA_TEXTURE_SMOOTH_TRANSPARENCY_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING| MPR_SE_TEXTURING | MPR_SE_SHADING | MPR_SE_TRANSPARENCY;
			SetState (	MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_ALPHA_TEXTURE_GOURAUD:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING;
			i |= MPR_SE_TEXTURING | MPR_SE_SHADING | MPR_SE_MODULATION;
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_ALPHA_TEXTURE_TRANSPARENCY:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING | MPR_SE_TEXTURING | MPR_SE_TRANSPARENCY;
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_ALPHA_TEXTURE_LIT_TRANSPARENCY:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING | MPR_SE_TEXTURING | MPR_SE_TRANSPARENCY;
			i |= MPR_SE_MODULATION;
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_ALPHA_TEXTURE_LIT_TRANSPARENCY_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING | MPR_SE_TEXTURING | MPR_SE_TRANSPARENCY;
			i |= MPR_SE_MODULATION;
			SetState (	MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_ALPHA_TEXTURE_GOURAUD_TRANSPARENCY:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING;
			i |= MPR_SE_TEXTURING | MPR_SE_TRANSPARENCY;
			i |= MPR_SE_MODULATION;
			i |= MPR_SE_SHADING;
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_ALPHA_TEXTURE_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING;
			i |= MPR_SE_TEXTURING;
			SetState (MPR_STA_ENABLES, i);
			SetState ( MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE );
			break;

		case STATE_ALPHA_TEXTURE_GOURAUD_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING;
			i |= MPR_SE_TEXTURING | MPR_SE_SHADING | MPR_SE_MODULATION;
			SetState (MPR_STA_ENABLES, i);
			SetState ( MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE );

// OW, added 23-07-2000 to fix cloud rendering problem and dont break missile trails
			m_pD3DD->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
			break;

		case STATE_ALPHA_TEXTURE_TRANSPARENCY_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING;
			i |= MPR_SE_TEXTURING | MPR_SE_TRANSPARENCY;
			SetState (MPR_STA_ENABLES, i);
			SetState ( MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE );
			break;

		case STATE_ALPHA_TEXTURE_GOURAUD_TRANSPARENCY_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING;
			i |= MPR_SE_TEXTURING | MPR_SE_TRANSPARENCY;
			i |= MPR_SE_MODULATION;
			i |= MPR_SE_SHADING;
			SetState (MPR_STA_ENABLES, i);
			SetState ( MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE );
			break;

		case STATE_FOG_TEXTURE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING);

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_FOG;
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_FOG_TEXTURE_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING);

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_FOG;
			SetState (MPR_STA_ENABLES, i);
			SetState (	MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);
			break;

		case STATE_FOG_TEXTURE_TRANSPARENCY:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING);

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_TRANSPARENCY | MPR_SE_FOG;
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_FOG_TEXTURE_TRANSPARENCY_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING);

			SetState (MPR_STA_ENABLES,	MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_TRANSPARENCY | MPR_SE_FOG;
			SetState (MPR_STA_ENABLES, i);
			SetState ( MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE );
			break;

		case STATE_FOG_TEXTURE_LIT:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_SHADING);

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_MODULATION | MPR_SE_FOG;
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_FOG_TEXTURE_LIT_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_SHADING);

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_MODULATION | MPR_SE_FOG;
			SetState (MPR_STA_ENABLES, i);
			SetState (	MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);
			break;

		case STATE_FOG_TEXTURE_LIT_TRANSPARENCY:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_SHADING);

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_MODULATION | MPR_SE_TRANSPARENCY | MPR_SE_FOG;
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_FOG_TEXTURE_LIT_TRANSPARENCY_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_SHADING);

			SetState (MPR_STA_ENABLES,	MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_MODULATION | MPR_SE_TRANSPARENCY | MPR_SE_FOG;
			SetState (MPR_STA_ENABLES, i);
			SetState ( MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE );
			break;

		case STATE_FOG_TEXTURE_SMOOTH:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING);

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_SHADING | MPR_SE_FOG;
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_FOG_TEXTURE_SMOOTH_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING);

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_SHADING | MPR_SE_FOG;
			SetState (	MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_FOG_TEXTURE_SMOOTH_TRANSPARENCY:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING);

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_TRANSPARENCY | MPR_SE_SHADING | MPR_SE_FOG;
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_FOG_TEXTURE_SMOOTH_TRANSPARENCY_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING);

			SetState (MPR_STA_ENABLES,	MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_TRANSPARENCY | MPR_SE_SHADING | MPR_SE_FOG;
			SetState (MPR_STA_ENABLES, i);
			SetState ( MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE );
			break;

		case STATE_FOG_TEXTURE_GOURAUD:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY);

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_SHADING | MPR_SE_MODULATION | MPR_SE_FOG;
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_FOG_TEXTURE_GOURAUD_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY);

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_SHADING | MPR_SE_MODULATION | MPR_SE_FOG;
			SetState (	MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_FOG_TEXTURE_GOURAUD_TRANSPARENCY:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING);

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_TRANSPARENCY | MPR_SE_SHADING | MPR_SE_MODULATION | MPR_SE_FOG;
			SetState (MPR_STA_ENABLES, i);
			break;

		case STATE_FOG_TEXTURE_GOURAUD_TRANSPARENCY_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING);

			SetState (MPR_STA_ENABLES,	MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_TRANSPARENCY | MPR_SE_SHADING | MPR_SE_MODULATION | MPR_SE_FOG;
			SetState (MPR_STA_ENABLES, i);
			SetState ( MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE );
			break;

		case STATE_TEXTURE_FILTER:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING;
			i |= MPR_SE_PALETTIZED_TEXTURES;
			i |= MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE;
			i |= MPR_SE_FILTERING;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FILTER, MPR_TX_BILINEAR);
			break;

		case STATE_TEXTURE_FILTER_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING;
			i |= MPR_SE_FILTERING;
			SetState ( MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FILTER, MPR_TX_BILINEAR);
			SetState ( MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE );
			break;


		case STATE_TEXTURE_POLYBLEND:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING;
			i |= MPR_SE_MODULATION;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_ALPHA );
			break;

		case STATE_TEXTURE_POLYBLEND_GOURAUD:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING;
			i |= MPR_SE_SHADING;
			i |= MPR_SE_MODULATION;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_ALPHA );
			break;

		case STATE_TEXTURE_FILTER_POLYBLEND:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING;
			i |= MPR_SE_FILTERING;
			i |= MPR_SE_MODULATION;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_ALPHA );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_BILINEAR);
			break;

		case STATE_TEXTURE_FILTER_POLYBLEND_GOURAUD:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING;
			i |= MPR_SE_SHADING;
			i |= MPR_SE_FILTERING;
			i |= MPR_SE_MODULATION;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_ALPHA );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_BILINEAR);
			break;

		case STATE_APT_TEXTURE_FILTER_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING;
			i |= MPR_SE_BLENDING;
			i |= MPR_SE_FILTERING;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FILTER, MPR_TX_BILINEAR);
			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			SetState (MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);
			break;

		case STATE_APT_FOG_TEXTURE_FILTER_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING);

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING;
			i |= MPR_SE_BLENDING;
			i |= MPR_SE_FILTERING;
			i |= MPR_SE_FOG;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FILTER, MPR_TX_BILINEAR);
			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			SetState (MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);
			break;

		case STATE_APT_FOG_TEXTURE_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING);

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING;
			i |= MPR_SE_BLENDING;
			i |= MPR_SE_FOG;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			SetState (MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);
			break;

		case STATE_APT_FOG_TEXTURE_SMOOTH_PERSPECTIVE:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING);

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING;
			i |= MPR_SE_BLENDING;
			i |= MPR_SE_SHADING;
			i |= MPR_SE_FOG;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			SetState (MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);
			break;

		// OW
		case STATE_TEXTURE_LIT_FILTER:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_MODULATION;
			i |= MPR_SE_FILTERING;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FILTER, MPR_TX_BILINEAR_NOCLAMP);
			break;

		case STATE_TEXTURE_GOURAUD_FILTER:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_SHADING | MPR_SE_MODULATION;
			i |= MPR_SE_FILTERING;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FILTER, MPR_TX_BILINEAR_NOCLAMP);
			break;

		case STATE_TEXTURE_TRANSPARENCY_FILTER:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_TRANSPARENCY;
			i |= MPR_SE_FILTERING;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FILTER, MPR_TX_BILINEAR_NOCLAMP);
			break;

		case STATE_TEXTURE_LIT_TRANSPARENCY_FILTER:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_TRANSPARENCY | MPR_SE_MODULATION;
			i |= MPR_SE_FILTERING;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FILTER, MPR_TX_BILINEAR_NOCLAMP);
			break;

		case STATE_TEXTURE_GOURAUD_TRANSPARENCY_FILTER:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_TRANSPARENCY | 
				MPR_SE_SHADING | MPR_SE_MODULATION;
			i |= MPR_SE_FILTERING;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FILTER, MPR_TX_BILINEAR_NOCLAMP);
			break;

		case STATE_TEXTURE_GOURAUD_TRANSPARENCY_PERSPECTIVE_FILTER:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_TRANSPARENCY | 
				MPR_SE_SHADING | MPR_SE_MODULATION;
			SetState (	MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);
			i |= MPR_SE_FILTERING;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FILTER, MPR_TX_BILINEAR_NOCLAMP);
			break;

		case STATE_TEXTURE_LIT_TRANSPARENCY_PERSPECTIVE_FILTER:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_TRANSPARENCY | MPR_SE_MODULATION;
			SetState ( MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE );
			i |= MPR_SE_FILTERING;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FILTER, MPR_TX_BILINEAR_NOCLAMP);

		case STATE_TEXTURE_TRANSPARENCY_PERSPECTIVE_FILTER:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_TRANSPARENCY;
			SetState (	MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);
			i |= MPR_SE_FILTERING;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FILTER, MPR_TX_BILINEAR_NOCLAMP);
			break;

		case STATE_TEXTURE_GOURAUD_PERSPECTIVE_FILTER:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_SHADING | MPR_SE_MODULATION;
			SetState (	MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);
			i |= MPR_SE_FILTERING;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FILTER, MPR_TX_BILINEAR_NOCLAMP);
			break;

		case STATE_TEXTURE_LIT_PERSPECTIVE_FILTER:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING | MPR_SE_MODULATION;
			SetState ( MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE );
			i |= MPR_SE_FILTERING;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FILTER, MPR_TX_BILINEAR_NOCLAMP);
			break;

		case STATE_TEXTURE_PERSPECTIVE_FILTER:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			i |= MPR_SE_TEXTURING;
			SetState (	MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);
			i |= MPR_SE_FILTERING;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FILTER, MPR_TX_BILINEAR_NOCLAMP);
			break;

		case STATE_ALPHA_TEXTURE_FILTER:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING;
			i |= MPR_SE_TEXTURING;
			i |= MPR_SE_FILTERING;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FILTER, MPR_TX_BILINEAR_NOCLAMP);
			break;

		case STATE_ALPHA_TEXTURE_LIT_FILTER:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING;
			i |= MPR_SE_TEXTURING | MPR_SE_MODULATION;
			i |= MPR_SE_FILTERING;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FILTER, MPR_TX_BILINEAR_NOCLAMP);
			break;

		case STATE_ALPHA_TEXTURE_SMOOTH_FILTER:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING| MPR_SE_TEXTURING | MPR_SE_SHADING;
			i |= MPR_SE_FILTERING;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FILTER, MPR_TX_BILINEAR_NOCLAMP);
			break;

		case STATE_ALPHA_TEXTURE_GOURAUD_FILTER:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING;
			i |= MPR_SE_TEXTURING | MPR_SE_SHADING | MPR_SE_MODULATION;
			i |= MPR_SE_FILTERING;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FILTER, MPR_TX_BILINEAR_NOCLAMP);
			break;

		case STATE_ALPHA_TEXTURE_TRANSPARENCY_FILTER:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING | MPR_SE_TEXTURING | MPR_SE_TRANSPARENCY;
			i |= MPR_SE_FILTERING;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FILTER, MPR_TX_BILINEAR_NOCLAMP);
			break;

		case STATE_ALPHA_TEXTURE_LIT_TRANSPARENCY_FILTER:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING | MPR_SE_TEXTURING | MPR_SE_TRANSPARENCY;
			i |= MPR_SE_MODULATION;
			i |= MPR_SE_FILTERING;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FILTER, MPR_TX_BILINEAR_NOCLAMP);
			break;

		case STATE_ALPHA_TEXTURE_SMOOTH_TRANSPARENCY_FILTER:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING| MPR_SE_TEXTURING | MPR_SE_SHADING | MPR_SE_TRANSPARENCY;
			i |= MPR_SE_FILTERING;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FILTER, MPR_TX_BILINEAR_NOCLAMP);
			break;

		case STATE_ALPHA_TEXTURE_GOURAUD_TRANSPARENCY_FILTER:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING;
			i |= MPR_SE_TEXTURING | MPR_SE_TRANSPARENCY;
			i |= MPR_SE_MODULATION;
			i |= MPR_SE_SHADING;
			i |= MPR_SE_FILTERING;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FILTER, MPR_TX_BILINEAR_NOCLAMP);
			break;

		case STATE_ALPHA_TEXTURE_PERSPECTIVE_FILTER:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING;
			i |= MPR_SE_TEXTURING;
			SetState ( MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE );
			i |= MPR_SE_FILTERING;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FILTER, MPR_TX_BILINEAR_NOCLAMP);
			break;

		case STATE_ALPHA_TEXTURE_LIT_PERSPECTIVE_FILTER:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING;
			i |= MPR_SE_TEXTURING | MPR_SE_MODULATION;
			SetState (	MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);
			i |= MPR_SE_FILTERING;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FILTER, MPR_TX_BILINEAR_NOCLAMP);
			break;

		case STATE_ALPHA_TEXTURE_SMOOTH_PERSPECTIVE_FILTER:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING| MPR_SE_TEXTURING | MPR_SE_SHADING;
			SetState (	MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);
			i |= MPR_SE_FILTERING;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FILTER, MPR_TX_BILINEAR_NOCLAMP);
			break;

		case STATE_ALPHA_TEXTURE_GOURAUD_PERSPECTIVE_FILTER:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING;
			i |= MPR_SE_TEXTURING | MPR_SE_SHADING | MPR_SE_MODULATION;
			SetState ( MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE );
			i |= MPR_SE_FILTERING;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FILTER, MPR_TX_BILINEAR_NOCLAMP);
			break;

		case STATE_ALPHA_TEXTURE_TRANSPARENCY_PERSPECTIVE_FILTER:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_TRANSPARENCY |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING;
			i |= MPR_SE_TEXTURING | MPR_SE_SHADING | MPR_SE_MODULATION;
			SetState ( MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE );
			i |= MPR_SE_FILTERING;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FILTER, MPR_TX_BILINEAR_NOCLAMP);
			break;

		case STATE_ALPHA_TEXTURE_LIT_TRANSPARENCY_PERSPECTIVE_FILTER:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING | MPR_SE_TEXTURING | MPR_SE_TRANSPARENCY;
			i |= MPR_SE_MODULATION;
			SetState (	MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);
			i |= MPR_SE_FILTERING;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FILTER, MPR_TX_BILINEAR_NOCLAMP);
			break;

		case STATE_ALPHA_TEXTURE_SMOOTH_TRANSPARENCY_PERSPECTIVE_FILTER:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_MODULATION | 
				MPR_SE_SHADING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE | MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING| MPR_SE_TEXTURING | MPR_SE_SHADING | MPR_SE_TRANSPARENCY;
			SetState (	MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE);
			i |= MPR_SE_FILTERING;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FILTER, MPR_TX_BILINEAR_NOCLAMP);
			break;

		case STATE_ALPHA_TEXTURE_GOURAUD_TRANSPARENCY_PERSPECTIVE_FILTER:
			SetState(MPR_STA_DISABLES, 
				MPR_SE_Z_BUFFERING |
				MPR_SE_CLIPPING |
				MPR_SE_BLENDING |
				MPR_SE_DITHERING |
				MPR_SE_FILTERING |
				MPR_SE_FOG );

			SetState (MPR_STA_ENABLES,	MPR_SE_PALETTIZED_TEXTURES );
			SetState (MPR_STA_TEX_FILTER, MPR_TX_MIPMAP_NEAREST);
			SetState (MPR_STA_TEX_FUNCTION, MPR_TF_MULTIPLY);

			if(flag & ENABLE_DITHERING_STATE)
				SetState (MPR_STA_ENABLES, MPR_SE_DITHERING);

			SetState (MPR_STA_SRC_BLEND_FUNCTION, MPR_BF_SRC_ALPHA);
			SetState (MPR_STA_DST_BLEND_FUNCTION, MPR_BF_SRC_ALPHA_INV);
			i |= MPR_SE_BLENDING;
			i |= MPR_SE_TEXTURING | MPR_SE_TRANSPARENCY;
			i |= MPR_SE_MODULATION;
			i |= MPR_SE_SHADING;
			SetState ( MPR_STA_DISABLES, MPR_SE_NON_PERSPECTIVE_CORRECTION_MODE );
			i |= MPR_SE_FILTERING;
			SetState (MPR_STA_ENABLES, i);
			SetState (MPR_STA_TEX_FILTER, MPR_TX_BILINEAR_NOCLAMP);
			break;

		default:
			ShiWarning( "BAD OR MISSING CONTEXT STATE" );	// Should never get here!
	}
}

void ContextMPR::Render2DBitmap( int sX, int sY, int dX, int dY, int w, int h, int totalWidth, DWORD *pSrc)
{
	DWORD *pDst = NULL;

	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::Render2DBitmap(%d, %d, %d, %d, %d, %d, %d, 0x%X)\n", sX, sY, dX, dY, w, h, totalWidth, pSrc);
	#endif
	ShiAssert(FALSE == F4IsBadReadPtr(m_pD3DD, sizeof *m_pD3DD));
	try
	{
		// Convert from ABGR to ARGB ;(
		pSrc = (DWORD *) (((BYTE *) pSrc) + (sY * (totalWidth << 2)) + sX);
		pDst = new DWORD[w * h];
		if(!pDst) throw _com_error(E_OUTOFMEMORY);

		for(int y=0;y<h;y++)
		{
			for(int x=0;x<w;x++)
				pDst[y * w + x] = RGBA_MAKE(RGBA_GETBLUE(pSrc[x]), RGBA_GETGREEN(pSrc[x]),
					RGBA_GETRED(pSrc[x]), RGBA_GETALPHA(pSrc[x]));

			pSrc = (DWORD *) (((BYTE *) pSrc) + (totalWidth << 2));
		}

		// Create tmp texture
		DWORD dwFlags = D3DX_TEXTURE_NOMIPMAP;
		DWORD dwActualWidth = w;
		DWORD dwActualHeight = h;
		D3DX_SURFACEFORMAT fmt = D3DX_SF_A8R8G8B8;
		DWORD dwNumMipMaps = 0;

		IDirectDrawSurface7Ptr pDDSTex;
		CheckHR(D3DXCreateTexture(m_pD3DD, &dwFlags, &dwActualWidth, &dwActualHeight,
			&fmt, NULL, &pDDSTex, &dwNumMipMaps));
		ShiAssert(FALSE==F4IsBadReadPtr(pDDSTex, sizeof *pDDSTex));
		CheckHR(D3DXLoadTextureFromMemory(m_pD3DD, pDDSTex, 0, pDst, NULL,
			D3DX_SF_A8R8G8B8, w << 2, NULL, D3DX_FT_LINEAR));

		// Setup vertices
		TwoDVertex pVtx[4];
		ZeroMemory(pVtx, sizeof(pVtx));
		pVtx[0].x = dX; pVtx[0].y = dY; pVtx[0].u = 0.0f; pVtx[0].v = 0.0f;
		pVtx[1].x = dX + w; pVtx[1].y = dY; pVtx[1].u = 1.0f; pVtx[1].v = 0.0f;
		pVtx[2].x = dX + w; pVtx[2].y = dY + h; pVtx[2].u = 1.0f; pVtx[2].v = 1.0f;
		pVtx[3].x = dX; pVtx[3].y = dY + h; pVtx[3].u = 0.0f; pVtx[3].v = 1.0f;

		// Setup state
		RestoreState(STATE_TEXTURE_FILTER);
		CheckHR(m_pD3DD->SetTexture(0, pDDSTex));

		// Render it (finally)
		DrawPrimitive(MPR_PRM_TRIFAN, MPR_VI_COLOR | MPR_VI_TEXTURE, 4, pVtx, sizeof(pVtx[0]));

		FlushVB();
		InvalidateState();
	}

	catch(_com_error e)
	{
		MonoPrint("ContextMPR::Render2DBitmap - Error 0x%X\n", e.Error());
	}

	if(pDst) delete[] pDst;
}

inline void ContextMPR::SetStateTable (GLint state, GLint flag)
{
	if (!m_pD3DD)
		return;
		
	// Record a stateblock
	HRESULT hr = m_pD3DD->BeginStateBlock();
	ShiAssert(SUCCEEDED(hr));

	SetCurrentState (state, flag);

	hr = m_pD3DD->EndStateBlock((DWORD *) &StateTable[state]);
	ShiAssert(SUCCEEDED(hr) && StateTable[state]);

	// OW - record internal state
	m_bUseSetStateInternal = true;
	SetCurrentState(state, flag);
	m_bUseSetStateInternal = false;
}

inline void ContextMPR::ClearStateTable(GLint state)
{
	HRESULT hr = m_pD3DD->DeleteStateBlock(StateTable[state]);
	ShiAssert(SUCCEEDED(hr));

	StateTable[state] = 0;
}


void ContextMPR::SetupMPRState(GLint flag)
{
	if(flag & CHECK_PREVIOUS_STATE)
	{
		StateSetupCounter++;
		if(StateSetupCounter > 1)
			return;
	}

	else if(StateSetupCounter)
		CleanupMPRState();

	// Record one stateblock per poly type
	MonoPrint("ContextMPR - Setting up state table\n");
	for (currentState=STATE_SOLID; currentState<MAXIMUM_MPR_STATE; currentState++)
		SetStateTable(currentState, flag );

	InvalidateState ();
}

void ContextMPR::CleanupMPRState (GLint flag)
{
	if(!StateSetupCounter)
	{
		ShiWarning("MPR not initialized!");
		return;	// mpr is not initialized yet
	}

	if(flag & CHECK_PREVIOUS_STATE)
	{
		StateSetupCounter--;
		if(StateSetupCounter > 0)
			return;
	}

	MonoPrint("ContextMPR - Clearing state table\n");
	for(int i=STATE_SOLID; i<MAXIMUM_MPR_STATE; i++)
		ClearStateTable(i);
}


void ContextMPR::SelectTexture (GLint texID)
{
	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::SelectTexture(0x%X)\n", texID);
	#endif

	if (g_bSlowButSafe && F4IsBadReadPtr((TextureHandle *) texID, sizeof(TextureHandle))) // JB 010326 CTD (too much CPU)
		return;

	if(texID)
		texID = (GLint) ((TextureHandle *) texID)->m_pDDS;

	if(texID != currentTexture)
	{
		FlushVB();

		#ifdef _CONTEXT_ENABLE_STATS
		m_stats.PutTexture(false);
		#endif // _CONTEXT_ENABLE_STATS

		currentTexture = texID;

		// m_pD3DD->PreLoad((IDirectDrawSurface7 *) texID);

		HRESULT hr = m_pD3DD->SetTexture(0, (IDirectDrawSurface7 *) texID); 	// Make sure all textures get released
		ShiAssert(SUCCEEDED(hr));
	}

	#ifdef _CONTEXT_ENABLE_STATS
	else m_stats.PutTexture(true);
	#endif // _CONTEXT_ENABLE_STATS
}

void ContextMPR::SelectForegroundColor(GLint color)
{
	if(color != m_colFG_Raw)
	{
		m_colFG_Raw = color;
		m_colFG = MPRColor2D3DRGBA(color);
	}
}

void ContextMPR::SelectBackgroundColor(GLint color)
{
	if(color != m_colBG_Raw)
	{
		m_colBG_Raw = color;
		m_colBG = MPRColor2D3DRGBA(color);
	}
}

void ContextMPR::RestoreState(GLint state)
{
	ShiAssert(state != -1);		// Use InvalidateState() !
	ShiAssert(state >= 0 && state < MAXIMUM_MPR_STATE);

	#if defined _DEBUG && defined _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT_REPLACE
	if(GetKeyState(VK_F4) & ~1)
	{
		if(!bEnableRenderStateHighlightReplace)
			bEnableRenderStateHighlightReplace = true;
	}
	
	if(bEnableRenderStateHighlightReplace && (state == bRenderStateHighlightReplaceTargetState))
	{
		state = STATE_SOLID;
		m_colFG = 0xffff0000;
		currentState = -1;
	}
	#endif

	if(state != currentState)
	{
		#ifdef _CONTEXT_TRACE_ALL
		MonoPrint("ContextMPR::RestoreState(%d)\n", state);
		#endif

		#ifdef _CONTEXT_RECORD_USED_STATES
		m_setStatesUsed.insert(state);
		#endif

		FlushVB();

		// if texturing gets disabled by this state change, force texture reload
		if(currentState == -1 || (StateTableInternal[currentState].SE_TEXTURING && !StateTableInternal[state].SE_TEXTURING))
			currentTexture = -1;

		currentState = state;

		HRESULT hr = m_pD3DD->ApplyStateBlock(StateTable[currentState]);
		ShiAssert(SUCCEEDED(hr));
	}
}

DWORD ContextMPR::MPRColor2D3DRGBA(GLint color)
{
	return RGBA_MAKE(RGBA_GETBLUE(color), RGBA_GETGREEN(color), RGBA_GETRED(color), RGBA_GETALPHA(color));
}

HRESULT WINAPI ContextMPR::EnumSurfacesCB2(IDirectDrawSurface7 *lpDDSurface, struct _DDSURFACEDESC2 *lpDDSurfaceDesc, LPVOID lpContext)
{
	ContextMPR *pThis = (ContextMPR *) lpContext;
	ShiAssert(FALSE == F4IsBadReadPtr(pThis, sizeof *pThis));
	ShiAssert(FALSE == F4IsBadReadPtr(lpDDSurfaceDesc, sizeof *lpDDSurfaceDesc));

	if(lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
	{
		pThis->m_pDDSP = lpDDSurface;	// already addref'd by ddraw
		return DDENUMRET_CANCEL;	// we got what we wanted
	}

	return DDENUMRET_OK;	// continue
}

void ContextMPR::UpdateViewport()
{
	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::UpdateViewport()\n");
	#endif

	if(m_bViewportLocked || !m_pD3DD)
		return;

	// get current viewport
	D3DVIEWPORT7 vp;
	HRESULT hr = m_pD3DD->GetViewport(&vp);
	ShiAssert(SUCCEEDED(hr));
	if(FAILED(hr)) return;	

	if(m_bEnableScissors)
	{
		// Set the viewport to the specified dimensions
		vp.dwX = m_rcVP.left;
		vp.dwY = m_rcVP.top;
		vp.dwWidth = m_rcVP.right - m_rcVP.left;
		vp.dwHeight = m_rcVP.bottom - m_rcVP.top;

		if(!vp.dwWidth || !vp.dwHeight)
			return;		// incomplete
	}

	else
	{
		// Set the viewport to the full target surface dimensions
		DDSURFACEDESC2 ddsd;
		ZeroMemory(&ddsd, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		hr = m_pRenderTarget->GetSurfaceDesc(&ddsd);
		ShiAssert(SUCCEEDED(hr));
		if(FAILED(hr)) return;	

		vp.dwX = 0;
		vp.dwY = 0;
		vp.dwWidth = ddsd.dwWidth;
		vp.dwHeight = ddsd.dwHeight;
	}	

	hr = m_pD3DD->SetViewport(&vp);
	ShiAssert(SUCCEEDED(hr));
}

void ContextMPR::SetViewportAbs(int nLeft, int nTop, int nRight, int nBottom)
{
	if(m_bViewportLocked)
		return;

	m_rcVP.left = nLeft;
	m_rcVP.right = nRight;
	m_rcVP.top = nTop;
	m_rcVP.bottom = nBottom;

	UpdateViewport();
}

void ContextMPR::LockViewport()
{
//	ShiAssert(!m_bViewportLocked);
	m_bViewportLocked = true;
}

void ContextMPR::UnlockViewport()
{
//	ShiAssert(m_bViewportLocked);
	m_bViewportLocked = false;
}

void ContextMPR::GetViewport(RECT *prc)
{
    ShiAssert(FALSE == F4IsBadWritePtr(prc, sizeof *prc));
	*prc = m_rcVP;
}

void ContextMPR::Stats()
{
	#ifdef _DEBUG
	if(m_bNoD3DStatsAvail)
		return;

	HRESULT hr;
	#ifdef _CONTEXT_USE_MANAGED_TEXTURES
	D3DDEVINFO_TEXTUREMANAGER ditexman;
	hr = m_pD3DD->GetInfo(D3DDEVINFOID_TEXTUREMANAGER, &ditexman, sizeof(ditexman));
	m_bNoD3DStatsAvail = FAILED(hr) || hr == S_FALSE;
	#endif

	D3DDEVINFO_TEXTURING ditex;
	hr = m_pD3DD->GetInfo(D3DDEVINFOID_TEXTURING, &ditex, sizeof(ditex));
	m_bNoD3DStatsAvail = FAILED(hr) || hr == S_FALSE;
	#endif
}

void ContextMPR::TextOut(short x, short y, DWORD col, LPSTR str)
{
	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::TextOut(%d, %d, 0x%X, %s)\n", x, y, col, str);
	#endif

	if(!str) return;

	try
	{
		HDC hdc;
		CheckHR(m_pRenderTarget->GetDC(&hdc));	// Get GDI Device context for Surface

		if(hdc)
		{
			::SetBkMode(hdc, TRANSPARENT);
			::SetTextColor(hdc, col);
			::MoveToEx(hdc, x, y, NULL);

			::DrawText(hdc, str, strlen(str), &m_rcVP, DT_LEFT);

			CheckHR(m_pRenderTarget->ReleaseDC(hdc));
		}
	}

	catch(_com_error e)
	{
	}
}

bool ContextMPR::LockVB(int nVtxCount, void **p)
{
	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::LockVB(%d, 0x%X) (m_dwStartVtx = %d, m_dwNumVtx = %d)\n", nVtxCount, p, m_dwStartVtx, m_dwNumVtx);
	#endif

	HRESULT hr;
	DWORD dwSize = 0;

	ShiAssert(FALSE == F4IsBadReadPtr (m_pVB, sizeof *m_pVB));
	// Check for VB overflow
	if((m_dwStartVtx + m_dwNumVtx + nVtxCount) >= m_dwVBSize)
	{
		// would overflow
		FlushVB();
		m_dwStartVtx = 0;

		// we are done with this VB, hint driver that he can use a another memory block to prevent breaking DMA activity
		hr = m_pVB->Lock(DDLOCK_SURFACEMEMORYPTR | DDLOCK_WRITEONLY | DDLOCK_WAIT | 
			DDLOCK_DISCARDCONTENTS, p, &dwSize);
	}

	else if(m_pVtx)
	{
		// already locked, excellent
		return true;
	}

	else
	{
		// we will only append data, dont interrupt DMA
		if(m_dwStartVtx) hr = m_pVB->Lock(DDLOCK_SURFACEMEMORYPTR | DDLOCK_WRITEONLY | DDLOCK_WAIT | 
				DDLOCK_NOOVERWRITE, p, &dwSize);

		// ok this is the first lock
		else hr = m_pVB->Lock(DDLOCK_SURFACEMEMORYPTR | DDLOCK_WRITEONLY | DDLOCK_WAIT | 
				DDLOCK_DISCARDCONTENTS, p, &dwSize);
	}

	ShiAssert(SUCCEEDED(hr));

	#ifdef _DEBUG
	if(SUCCEEDED(hr)) m_pVtxEnd = (BYTE *) *p + dwSize;
	else m_pVtxEnd = NULL;
	#endif

	return SUCCEEDED(hr);
}

void ContextMPR::UnlockVB()
{
	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::UnlockVB()\n");
	#endif

	// Unlock VB
	HRESULT hr = m_pVB->Unlock();
	ShiAssert(SUCCEEDED(hr));
	m_pVtx = NULL;
}

void ContextMPR::FlushVB()
{
	if(!m_dwNumVtx) return;
	ShiAssert(m_nCurPrimType != 0);

	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::FlushVB()\n");
	#endif

	int nPrimType = m_nCurPrimType;

	// Convert triangle fans to triangle lists to make them batchable
	if(nPrimType == D3DPT_TRIANGLEFAN)
	{
		nPrimType = D3DPT_TRIANGLELIST;
		ShiAssert(m_dwNumIdx);
	}

	// Convert line strips to line lists to make them batchable
	else if(nPrimType == D3DPT_LINESTRIP)
	{
		nPrimType = D3DPT_LINELIST;
		ShiAssert(m_dwNumIdx);
	}

	HRESULT hr;

	UnlockVB();

	#ifdef _VALIDATE_DEVICE
	if(!m_pCtxDX->ValidateD3DDevice())
		MonoPrint("ContextMPR::FlushVB() - Validate Device failed - currentState=%d, currentTexture=0x%\n",
			currentState, currentTexture);
	#endif

	#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
	//if(currentState == STATE_ALPHA_GOURAUD)
	{
		hr = m_pD3DD->ApplyStateBlock(StateTable[STATE_SOLID]);
		ShiAssert(SUCCEEDED(hr));
	}
	#endif

	if(m_dwNumIdx) hr = m_pD3DD->DrawIndexedPrimitiveVB((D3DPRIMITIVETYPE) nPrimType,
		m_pVB, m_dwStartVtx, m_dwNumVtx, m_pIdx, m_dwNumIdx, NULL);

	else hr = m_pD3DD->DrawPrimitiveVB((D3DPRIMITIVETYPE) nPrimType,
		m_pVB, m_dwStartVtx, m_dwNumVtx, NULL);

	ShiAssert(SUCCEEDED(hr));

	#ifdef _CONTEXT_ENABLE_STATS
	m_stats.StartBatch();
	#endif // _CONTEXT_ENABLE_STATS

	m_dwStartVtx += m_dwNumVtx;
	m_dwNumVtx = 0;
	m_dwNumIdx = 0;
	m_VtxInfo = 0;		// invalid
}

void ContextMPR::SetPrimitiveType(int nType)
{
	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::SetPrimitiveType(%d)\n", nType);
	#endif

	if(m_nCurPrimType != nType)
	{
		// Flush on changed primitive type
		FlushVB();
		m_nCurPrimType = nType;
	}
}

void ContextMPR::DrawPoly(DWORD opFlag, Poly *poly, int *xyzIdxPtr, int *rgbaIdxPtr, int *IIdxPtr, Ptexcoord *uv, bool bUseFGColor)
{
	// Note: incoming type is always MPR_PRM_TRIFAN
	// OW FIXME: optimize loop
	ShiAssert(FALSE == F4IsBadReadPtr(poly, sizeof *poly));
	ShiAssert(poly->nVerts >= 3);
	ShiAssert(xyzIdxPtr);
	ShiAssert(!bUseFGColor || (bUseFGColor && rgbaIdxPtr == NULL));

	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::DrawPoly(0x%X, 0x%X, 0x%X, 0x%X, 0x%X, 0x%X, %s)\n",
		opFlag, poly, xyzIdxPtr, rgbaIdxPtr, IIdxPtr, uv, bUseFGColor ? "true" : "false");
	#endif

	SetPrimitiveType(D3DPT_TRIANGLEFAN);

	#ifdef _CONTEXT_ENABLE_STATS
	m_stats.Primitive(m_nCurPrimType, poly->nVerts);
	#endif // _CONTEXT_ENABLE_STATS

	TTLVERTEX *pVtx;	// points to locked VB
	if(!LockVB(poly->nVerts, (void **) &m_pVtx)) return; 	// Lock VB

	Pcolor *rgba;
	Ppoint *xyz;
	float *I;
	Pcolor foggedColor;

	ShiAssert(FALSE == F4IsBadWritePtr(m_pVtx, sizeof *m_pVtx));	
	ShiAssert(m_dwStartVtx < m_dwVBSize);
	pVtx = &m_pVtx[m_dwStartVtx + m_dwNumVtx];
	ShiAssert(FALSE == F4IsBadWritePtr(pVtx, poly->nVerts * sizeof *pVtx));

	if (!pVtx) // JB 011124 CTD
		return;

	// Iterate for each vertex
	for(int i=0;i<poly->nVerts;i++)
	{
		ShiAssert((BYTE *) pVtx < m_pVtxEnd);	// check for overrun

		/*if (F4IsBadWritePtr(pVtx, sizeof(TTLVERTEX))) // JB 010222 CTD (too much CPU)
		{
			pVtx++;
			continue;
		}*/

		xyz  = &TheStateStack.XformedPosPool[*xyzIdxPtr++];

		/*if (F4IsBadReadPtr(xyz, sizeof(Ppoint))) // JB 010317 CTD (too much CPU)
		{
			pVtx++;
			continue;
		}*/

		pVtx->sx = xyz->x;
		pVtx->sy = xyz->y;
		pVtx->sz = 1.0;
		if (xyz->z) // JB 010222
			pVtx->rhw = 1.0f / xyz->z;
		pVtx->color = 0xffffffff;	// do not remove
		ShiAssert(xyz->z != 0.0f);

		if(opFlag & PRIM_COLOP_COLOR)
		{
			ShiAssert(rgbaIdxPtr);
			rgba = &TheColorBank.ColorPool[*rgbaIdxPtr++];
			
			ShiAssert(rgba);
			if (rgba)
			{
				if((opFlag & PRIM_COLOP_FOG) && !(opFlag & PRIM_COLOP_TEXTURE))
				{
					foggedColor.r = rgba->r * TheStateStack.fogValue_inv + TheStateStack.fogColor_premul.r;
					foggedColor.g = rgba->g * TheStateStack.fogValue_inv + TheStateStack.fogColor_premul.g;
					foggedColor.b = rgba->b * TheStateStack.fogValue_inv + TheStateStack.fogColor_premul.b;
					foggedColor.a = rgba->a;
					rgba = &foggedColor;
				}

				if(opFlag & PRIM_COLOP_INTENSITY)
				{
					ShiAssert(IIdxPtr);
					I = &TheStateStack.IntensityPool[*IIdxPtr++];
					// ShiAssert((rgba->r * *I <= 1.0f) && (rgba->g * *I <= 1.0f) && (rgba->b * *I <= 1.0f) && (rgba->a <= 1.0f));

					pVtx->color = D3DRGBA(rgba->r * *I, rgba->g * *I, rgba->b * *I, rgba->a);
				}

				else
				{
					// ShiAssert((rgba->r <= 1.0f) && (rgba->g <= 1.0f) && (rgba->b <= 1.0f) && (rgba->a <= 1.0f));
					pVtx->color = D3DRGBA(rgba->r, rgba->g, rgba->b, rgba->a);
				}
			}
		}

		else if(opFlag & PRIM_COLOP_INTENSITY)
		{
			ShiAssert(IIdxPtr);

			I = &TheStateStack.IntensityPool[*IIdxPtr++];
			// ShiAssert(*I <= 1.0f);
			pVtx->color = D3DRGBA(*I, *I, *I, 1.0f);
		}

		if(opFlag & PRIM_COLOP_TEXTURE)
		{
			ShiAssert(uv);

			// OW FIXME
			// pVtx->q = xyz->z;
			pVtx->tu = uv->u;
			pVtx->tv = uv->v;
			uv++;

			if(opFlag & PRIM_COLOP_FOG) 
				*(3+ (BYTE *) &pVtx->color) = (BYTE) (TheStateStack.fogValue * 255);	// VB locked write only!!
		}

		else
		{
			// OW FIXME: should be
			pVtx->tu = 0;
			pVtx->tv = 0;
		}

		if(bUseFGColor)
			pVtx->color = m_colFG;

		#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
		pVtx->color = currentState != -1 ? RGBA_MAKE((currentState << 1) + 50, (currentState << 1) + 50, (currentState << 1) + 50, (currentState << 1) + 50) : D3DRGBA(1.0f, 1.0f, 1.0f, 1.0f);
		#endif

		pVtx++;
	}

	// Generate Indices
	WORD *pIdx = &m_pIdx[m_dwNumIdx];

	for(int x=0;x<poly->nVerts-2;x++)
	{
		pIdx[0] = m_dwNumVtx;
		pIdx[1] = m_dwNumVtx + x + 1;
		pIdx[2] = m_dwNumVtx + x + 2;
		pIdx += 3;
	}

	m_dwNumIdx += pIdx - &m_pIdx[m_dwNumIdx];
	m_dwNumVtx += poly->nVerts;

	#ifdef _CONTEXT_FLUSH_EVERY_PRIMITIVE
	FlushVB();
	#endif
}

void ContextMPR::Draw2DPoint(Tpoint *v0)
{
	ShiAssert(v0);

	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::Draw2DPoint(0x%X)\n", v0);
	#endif

	SetPrimitiveType(D3DPT_POINTLIST);

	#ifdef _CONTEXT_ENABLE_STATS
	m_stats.Primitive(m_nCurPrimType, 1);
	#endif // _CONTEXT_ENABLE_STATS

	TTLVERTEX *pVtx;	// points to locked VB
	if(!LockVB(1, (void **) &m_pVtx)) return; 	// Lock VB

	ShiAssert(FALSE == F4IsBadWritePtr(m_pVtx, sizeof *m_pVtx));	
	ShiAssert(m_dwStartVtx < m_dwVBSize);
	pVtx = &m_pVtx[m_dwStartVtx + m_dwNumVtx];
	ShiAssert(FALSE == F4IsBadWritePtr(pVtx, sizeof *pVtx));

	ShiAssert((BYTE *) pVtx < m_pVtxEnd);	// check for overrun

	pVtx->sx = v0->x;
	pVtx->sy = v0->y;
	pVtx->sz = 1.0;
	pVtx->rhw = 1.0f;
	pVtx->color = m_colFG;
	pVtx->tu = 0;
	pVtx->tv = 0;

	#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
	pVtx->color = currentState != -1 ? RGBA_MAKE((currentState << 1) + 50, (currentState << 1) + 50, (currentState << 1) + 50, (currentState << 1) + 50) : D3DRGBA(1.0f, 1.0f, 1.0f, 1.0f);
	#endif

	m_dwNumVtx++;

	#ifdef _CONTEXT_FLUSH_EVERY_PRIMITIVE
	FlushVB();
	#endif
}

void ContextMPR::Draw2DPoint(float x, float y)
{
	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::Draw2DPoint(%f, %f)\n", x, y);
	#endif

	SetPrimitiveType(D3DPT_POINTLIST);

	#ifdef _CONTEXT_ENABLE_STATS
	m_stats.Primitive(m_nCurPrimType, 1);
	#endif // _CONTEXT_ENABLE_STATS

	TTLVERTEX *pVtx;	// points to locked VB
	if(!LockVB(1, (void **) &m_pVtx)) return; 	// Lock VB

	ShiAssert(FALSE == F4IsBadWritePtr(m_pVtx, sizeof *m_pVtx));	
	ShiAssert(m_dwStartVtx < m_dwVBSize);
	pVtx = &m_pVtx[m_dwStartVtx + m_dwNumVtx];

	ShiAssert((BYTE *) pVtx < m_pVtxEnd);	// check for overrun
	ShiAssert(FALSE == F4IsBadWritePtr(pVtx, sizeof *pVtx));

	pVtx->sx = x;
	pVtx->sy = y;
	pVtx->sz = 1.0;
	pVtx->rhw = 1.0f;
	pVtx->color = m_colFG;
	pVtx->tu = 0;
	pVtx->tv = 0;

	#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
	pVtx->color = currentState != -1 ? RGBA_MAKE((currentState << 1) + 50, (currentState << 1) + 50, (currentState << 1) + 50, (currentState << 1) + 50) : D3DRGBA(1.0f, 1.0f, 1.0f, 1.0f);
	#endif

	m_dwNumVtx++;

	#ifdef _CONTEXT_FLUSH_EVERY_PRIMITIVE
	FlushVB();
	#endif
}

void ContextMPR::Draw2DLine(Tpoint *v0, Tpoint *v1)
{
	ShiAssert(v0 && v1);

	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::Draw2DLine(0x%X, 0x%X)\n", v0, v1);
	#endif

	SetPrimitiveType(D3DPT_LINESTRIP);

	#ifdef _CONTEXT_ENABLE_STATS
	m_stats.Primitive(m_nCurPrimType, 2);
	#endif // _CONTEXT_ENABLE_STATS

	TTLVERTEX *pVtx;	// points to locked VB
	if(!LockVB(2, (void **) &m_pVtx)) return; 	// Lock VB

	ShiAssert(FALSE == F4IsBadWritePtr(m_pVtx, sizeof *m_pVtx));	
	ShiAssert(m_dwStartVtx < m_dwVBSize);
	pVtx = &m_pVtx[m_dwStartVtx + m_dwNumVtx];

	ShiAssert((BYTE *) pVtx < m_pVtxEnd);	// check for overrun
	ShiAssert(FALSE == F4IsBadWritePtr(pVtx, 2*sizeof *pVtx));

	/*if (F4IsBadWritePtr(pVtx, sizeof(TTLVERTEX)) || F4IsBadReadPtr(v0, sizeof(Tpoint))) // JB 010305 CTD (too much CPU)
		return;*/

	pVtx->sx = v0->x;
	pVtx->sy = v0->y;
	pVtx->sz = 1.0;
	if (v0->z) // JB 010220 CTD
		pVtx->rhw = 1.0f / v0->z;
	pVtx->color = m_colFG;
	pVtx->tu = 0;
	pVtx->tv = 0;
	pVtx++;

	#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
	pVtx->color = currentState != -1 ? RGBA_MAKE((currentState << 1) + 50, (currentState << 1) + 50, (currentState << 1) + 50, (currentState << 1) + 50) : D3DRGBA(1.0f, 1.0f, 1.0f, 1.0f);
	#endif

	pVtx->sx = v1->x;
	pVtx->sy = v1->y;
	pVtx->sz = 1.0;
	pVtx->rhw = 1.0f / v1->z;
	pVtx->color = m_colFG;
	pVtx->tu = 0;
	pVtx->tv = 0;

	#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
	pVtx->color = currentState != -1 ? RGBA_MAKE((currentState << 1) + 50, (currentState << 1) + 50, (currentState << 1) + 50, (currentState << 1) + 50) : D3DRGBA(1.0f, 1.0f, 1.0f, 1.0f);
	#endif

	WORD *pIdx = &m_pIdx[m_dwNumIdx];
	*pIdx++ = m_dwNumVtx;
	*pIdx++ = m_dwNumVtx + 1;

	m_dwNumIdx += 2;
	m_dwNumVtx += 2;

	#ifdef _CONTEXT_FLUSH_EVERY_PRIMITIVE
	FlushVB();
	#endif
}


void ContextMPR::Draw2DLine(float x0, float y0, float x1, float y1)
{
	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::Draw2DLine(0x%X, 0x%X)\n", v0, v1);
	#endif

	SetPrimitiveType(D3DPT_LINESTRIP);

	#ifdef _CONTEXT_ENABLE_STATS
	m_stats.Primitive(m_nCurPrimType, 2);
	#endif // _CONTEXT_ENABLE_STATS

	TTLVERTEX *pVtx;	// points to locked VB
	if(!LockVB(2, (void **) &m_pVtx)) return; 	// Lock VB

	ShiAssert(FALSE == F4IsBadWritePtr(m_pVtx, sizeof *m_pVtx));	
	ShiAssert(m_dwStartVtx < m_dwVBSize);
	pVtx = &m_pVtx[m_dwStartVtx + m_dwNumVtx];
	ShiAssert(FALSE == F4IsBadWritePtr(pVtx, 2* sizeof *pVtx));

	ShiAssert((BYTE *) pVtx < m_pVtxEnd);	// check for overrun

	pVtx->sx = x0;
	pVtx->sy = y0;
	pVtx->sz = 1.0;
	pVtx->rhw = 1.0f;
	pVtx->color = m_colFG;
	pVtx->tu = 0;
	pVtx->tv = 0;
	pVtx++;

	/*if (F4IsBadWritePtr(pVtx, sizeof(TTLVERTEX))) // JB 010222 CTD (too much CPU)
		return; // JB 010222 CTD*/

	#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
	pVtx->color = currentState != -1 ? RGBA_MAKE((currentState << 1) + 50, (currentState << 1) + 50, (currentState << 1) + 50, (currentState << 1) + 50) : D3DRGBA(1.0f, 1.0f, 1.0f, 1.0f);
	#endif

	pVtx->sx = x1;
	pVtx->sy = y1;
	pVtx->sz = 1.0;
	pVtx->rhw = 1.0f;
	pVtx->color = m_colFG;
	pVtx->tu = 0;
	pVtx->tv = 0;

	#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
	pVtx->color = currentState != -1 ? RGBA_MAKE((currentState << 1) + 50, (currentState << 1) + 50, (currentState << 1) + 50, (currentState << 1) + 50) : D3DRGBA(1.0f, 1.0f, 1.0f, 1.0f);
	#endif

	WORD *pIdx = &m_pIdx[m_dwNumIdx];
	*pIdx++ = m_dwNumVtx;
	*pIdx++ = m_dwNumVtx + 1;

	m_dwNumIdx += 2;
	m_dwNumVtx += 2;

	#ifdef _CONTEXT_FLUSH_EVERY_PRIMITIVE
	FlushVB();
	#endif
}

void ContextMPR::DrawPrimitive2D(int type, int nVerts, int *xyzIdxPtr)
{
	ShiAssert(xyzIdxPtr);

	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::DrawPrimitive2D(%d, %d, 0x%X)\n", type, nVerts, xyzIdxPtr);
	#endif

	SetPrimitiveType(type == LineF ? D3DPT_LINESTRIP : D3DPT_POINTLIST);

	#ifdef _CONTEXT_ENABLE_STATS
	m_stats.Primitive(m_nCurPrimType, nVerts);
	#endif // _CONTEXT_ENABLE_STATS

	TTLVERTEX *pVtx;	// points to locked VB
	if(!LockVB(nVerts, (void **) &m_pVtx)) return; 	// Lock VB

	ShiAssert(FALSE == F4IsBadWritePtr(m_pVtx, sizeof *m_pVtx));	
	ShiAssert(m_dwStartVtx < m_dwVBSize);
	pVtx = &m_pVtx[m_dwStartVtx + m_dwNumVtx];
	ShiAssert(FALSE == F4IsBadWritePtr(pVtx, nVerts * sizeof *pVtx));

	Ppoint *xyz;

	// Iterate for each vertex
	for(int i=0;i<nVerts;i++)
	{
	    ShiAssert(*xyzIdxPtr < MAX_VERT_POOL_SIZE);
		xyz = &TheStateStack.XformedPosPool[*xyzIdxPtr++];
		ShiAssert((BYTE *) pVtx < m_pVtxEnd);	// check for overrun

		/*if (F4IsBadWritePtr(pVtx, sizeof(TTLVERTEX)) || F4IsBadReadPtr(xyz, sizeof(Ppoint))) // JB 010305 CTD (too much CPU)
		{
			pVtx++;
			continue;
		}*/

		pVtx->sx = xyz->x;
		pVtx->sy = xyz->y;
		pVtx->sz = 1.0;
		if (xyz->z) // JB 010305
			pVtx->rhw = 1.0f / xyz->z;
		pVtx->color = m_colFG;
		pVtx->tu = 0;
		pVtx->tv = 0;

		#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
		pVtx->color = currentState != -1 ? RGBA_MAKE((currentState << 1) + 50, (currentState << 1) + 50, (currentState << 1) + 50, (currentState << 1) + 50) : D3DRGBA(1.0f, 1.0f, 1.0f, 1.0f);
		#endif

		pVtx++;
	}

	// Generate Indices
	if(m_nCurPrimType == D3DPT_LINESTRIP)
	{
		WORD *pIdx = &m_pIdx[m_dwNumIdx];

		for(int x=0;x<nVerts;x++)
			*pIdx++ = m_dwNumVtx + x;

		m_dwNumIdx += nVerts;
	}

	m_dwNumVtx += nVerts;

	#ifdef _CONTEXT_FLUSH_EVERY_PRIMITIVE
	FlushVB();
	#endif
}

void ContextMPR::DrawPrimitive(int nPrimType, WORD VtxInfo, WORD nVerts, MPRVtx_t *pData, WORD Stride)
{
	ShiAssert(!(VtxInfo & MPR_VI_COLOR));	// impossible
	ShiAssert((nVerts >=3) || (nPrimType==MPR_PRM_POINTS && nVerts >=1) || (nPrimType<=MPR_PRM_POLYLINE && nVerts >=2));		// Ensure no degenerate nPrimTypeitives

	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::DrawPrimitive(%d, 0x%X, %d, 0x%X, %d)\n", nPrimType, VtxInfo, nVerts, pData, Stride);
	#endif

	SetPrimitiveType(nPrimType);

	#ifdef _CONTEXT_ENABLE_STATS
	m_stats.Primitive(m_nCurPrimType, nVerts);
	#endif // _CONTEXT_ENABLE_STATS

	TTLVERTEX *pVtx;	// points to locked VB
	if(!LockVB(nVerts, (void **) &m_pVtx)) return; 	// Lock VB

	ShiAssert(FALSE == F4IsBadWritePtr(m_pVtx, sizeof *m_pVtx));	
	ShiAssert(m_dwStartVtx < m_dwVBSize);
	pVtx = &m_pVtx[m_dwStartVtx + m_dwNumVtx];
	ShiAssert(FALSE == F4IsBadWritePtr(pVtx, nVerts * sizeof *pVtx));

	// Iterate for each vertex
	for(int i=0;i<nVerts;i++)
	{
		ShiAssert((BYTE *) pVtx < m_pVtxEnd);	// check for overrun

		pVtx->sx = pData->x;
		pVtx->sy = pData->y;
		pVtx->sz = 0.0f;
		pVtx->rhw = 1.0f;
		pVtx->color = m_colFG;
		pVtx->tu = 0;
		pVtx->tv = 0;

		#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
		pVtx->color = currentState != -1 ? RGBA_MAKE((currentState << 1) + 50, (currentState << 1) + 50, (currentState << 1) + 50, (currentState << 1) + 50) : D3DRGBA(1.0f, 1.0f, 1.0f, 1.0f);
		#endif

		pVtx++;
		pData = (MPRVtx_t *) ((BYTE *) pData + Stride);
	}

	// Generate Indices
	if(m_nCurPrimType == D3DPT_TRIANGLEFAN)
	{
		WORD *pIdx = &m_pIdx[m_dwNumIdx];

		for(int x=0;x<nVerts-2;x++)
		{
			pIdx[0] = m_dwNumVtx;
			pIdx[1] = m_dwNumVtx + x + 1;
			pIdx[2] = m_dwNumVtx + x + 2;
			pIdx += 3;
		}

		m_dwNumIdx += pIdx - &m_pIdx[m_dwNumIdx];
	}

	else if(m_nCurPrimType == D3DPT_LINESTRIP)
	{
		WORD *pIdx = &m_pIdx[m_dwNumIdx];

		for(int x=0;x<nVerts;x++)
			*pIdx++ = m_dwNumVtx + x;

		m_dwNumIdx += nVerts;
	}

	m_dwNumVtx += nVerts;

	#ifdef _CONTEXT_FLUSH_EVERY_PRIMITIVE
	FlushVB();
	#endif
}

void ContextMPR::DrawPrimitive(int nPrimType, WORD VtxInfo, WORD nVerts, MPRVtxTexClr_t *pData, WORD Stride)
{
	ShiAssert((nVerts >=3) || (nPrimType==MPR_PRM_POINTS && nVerts >=1) || (nPrimType<=MPR_PRM_POLYLINE && nVerts >=2));		// Ensure no degenerate nPrimTypeitives

	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::DrawPrimitive2(%d, 0x%X, %d, 0x%X, %d)\n", nPrimType, VtxInfo, nVerts, pData, Stride);
	#endif

	SetPrimitiveType(nPrimType);

	#ifdef _CONTEXT_ENABLE_STATS
	m_stats.Primitive(m_nCurPrimType, nVerts);
	#endif // _CONTEXT_ENABLE_STATS

	TTLVERTEX *pVtx;	// points to locked VB
	if(!LockVB(nVerts, (void **) &m_pVtx)) return; 	// Lock VB

	ShiAssert(FALSE == F4IsBadWritePtr(m_pVtx, sizeof *m_pVtx));	
	ShiAssert(m_dwStartVtx < m_dwVBSize);
	pVtx = &m_pVtx[m_dwStartVtx + m_dwNumVtx];

	ShiAssert(FALSE == F4IsBadWritePtr(pVtx, nVerts * sizeof *pVtx));

	if (!pVtx) // JB 011124 CTD
		return;

	switch(VtxInfo)
	{
		case MPR_VI_COLOR | MPR_VI_TEXTURE:
		{
			// Iterate for each vertex
			for(int i=0;i<nVerts;i++)
			{
				ShiAssert((BYTE *) pVtx < m_pVtxEnd);	// check for overrun

				/*if (F4IsBadWritePtr(pVtx, sizeof(TTLVERTEX)) || F4IsBadReadPtr(pData, sizeof(MPRVtxTexClr_t))) // JB 010305 CTD (too much CPU)
				{
					pVtx++;
					pData = (MPRVtxTexClr_t *) ((BYTE *) pData + Stride);
					continue;
				}*/

				pVtx->sx = pData->x;
				pVtx->sy = pData->y;
				pVtx->sz = 0.0f;
				pVtx->rhw = 1.0f;	// OW FIXME: this should be 1.0f / pData->z
				pVtx->color = D3DRGBA(pData->r, pData->g, pData->b, pData->a);
				pVtx->tu = pData->u;
				pVtx->tv = pData->v;

				#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
				pVtx->color = currentState != -1 ? RGBA_MAKE((currentState << 1) + 50, (currentState << 1) + 50, (currentState << 1) + 50, (currentState << 1) + 50) : D3DRGBA(1.0f, 1.0f, 1.0f, 1.0f);
				#endif

				pVtx++;
				pData = (MPRVtxTexClr_t *) ((BYTE *) pData + Stride);
			}

			break;
		}

		case MPR_VI_COLOR:
		{
			// Iterate for each vertex
			for(int i=0;i<nVerts;i++)
			{
				ShiAssert((BYTE *) pVtx < m_pVtxEnd);	// check for overrun

				pVtx->sx = pData->x;
				pVtx->sy = pData->y;
				pVtx->sz = 0.0f;
				pVtx->rhw = 1.0f;	// OW FIXME: this should be 1.0f / pData->z
				pVtx->color = D3DRGBA(pData->r, pData->g, pData->b, pData->a);

				#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
				pVtx->color = currentState != -1 ? RGBA_MAKE((currentState << 1) + 50, (currentState << 1) + 50, (currentState << 1) + 50, (currentState << 1) + 50) : D3DRGBA(1.0f, 1.0f, 1.0f, 1.0f);
				#endif

				pVtx++;
				pData = (MPRVtxTexClr_t *) ((BYTE *) pData + Stride);
			}

			break;
		}

		default:
		{
			ShiAssert(VtxInfo == NULL);

			// Iterate for each vertex
			for(int i=0;i<nVerts;i++)
			{
				ShiAssert((BYTE *) pVtx < m_pVtxEnd);	// check for overrun

				pVtx->sx = pData->x;
				pVtx->sy = pData->y;
				pVtx->sz = 0.0f;
				pVtx->rhw = 1.0f;	// OW FIXME: this should be 1.0f / pData->z
				pVtx->color = m_colFG;

				#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
				pVtx->color = currentState != -1 ? RGBA_MAKE((currentState << 1) + 50, (currentState << 1) + 50, (currentState << 1) + 50, (currentState << 1) + 50) : D3DRGBA(1.0f, 1.0f, 1.0f, 1.0f);
				#endif

				pVtx++;
				pData = (MPRVtxTexClr_t *) ((BYTE *) pData + Stride);
			}

			break;
		}
	}

	// Generate Indices (in advance)
	if(m_nCurPrimType == D3DPT_TRIANGLEFAN)
	{
		WORD *pIdx = &m_pIdx[m_dwNumIdx];

		for(int x=0;x<nVerts-2;x++)
		{
			pIdx[0] = m_dwNumVtx;
			pIdx[1] = m_dwNumVtx + x + 1;
			pIdx[2] = m_dwNumVtx + x + 2;
			pIdx += 3;
		}

		m_dwNumIdx += pIdx - &m_pIdx[m_dwNumIdx];
	}

	else if(m_nCurPrimType == D3DPT_LINESTRIP)
	{
		WORD *pIdx = &m_pIdx[m_dwNumIdx];

		for(int x=0;x<nVerts;x++)
			*pIdx++ = m_dwNumVtx + x;

		m_dwNumIdx += nVerts;
	}

	m_dwNumVtx += nVerts;

	#ifdef _CONTEXT_FLUSH_EVERY_PRIMITIVE
	FlushVB();
	#endif
}

void ContextMPR::DrawPrimitive(int nPrimType, WORD VtxInfo, WORD nVerts, MPRVtxTexClr_t **pData)
{
	ShiAssert((nVerts >=3) || (nPrimType==MPR_PRM_POINTS && nVerts >=1) || (nPrimType<=MPR_PRM_POLYLINE && nVerts >=2));		// Ensure no degenerate nPrimTypeitives

	#ifdef _CONTEXT_TRACE_ALL
	MonoPrint("ContextMPR::DrawPrimitive3(%d, 0x%X, %d, 0x%X)\n", nPrimType, VtxInfo, nVerts, pData);
	#endif

	SetPrimitiveType(nPrimType);

	#ifdef _CONTEXT_ENABLE_STATS
	m_stats.Primitive(m_nCurPrimType, nVerts);
	#endif // _CONTEXT_ENABLE_STATS

	TTLVERTEX *pVtx;	// points to locked VB
	if(!LockVB(nVerts, (void **) &m_pVtx)) return; 	// Lock VB

	ShiAssert(FALSE == F4IsBadWritePtr(m_pVtx, sizeof *m_pVtx));	
	ShiAssert(m_dwStartVtx < m_dwVBSize);
	pVtx = &m_pVtx[m_dwStartVtx + m_dwNumVtx];

	ShiAssert(FALSE == F4IsBadWritePtr(pVtx, nVerts * sizeof *pVtx));

	if (!pVtx) // JB 011124 CTD
		return;

	switch(VtxInfo)
	{
		case MPR_VI_COLOR | MPR_VI_TEXTURE:
		{
			// Iterate for each vertex
			for(int i=0;i<nVerts;i++)
			{
				ShiAssert((BYTE *) pVtx < m_pVtxEnd);	// check for overrun

				/*if (F4IsBadWritePtr(pVtx, sizeof(TTLVERTEX))) // JB 010222 CTD (too much CPU)
				{
					pVtx++;
					continue;
				}*/
				if (!pData[i]) // JB 010712 CTD second try
					break;

				pVtx->sx = pData[i]->x;
				pVtx->sy = pData[i]->y;
				pVtx->sz = 0.0f;
				pVtx->rhw = pData[i]->q > 0.0f ? 1.0f / (pData[i]->q / Q_SCALE) : 1.0f;
				pVtx->color = D3DRGBA(pData[i]->r, pData[i]->g, pData[i]->b, pData[i]->a);
				pVtx->tu = pData[i]->u;
				pVtx->tv = pData[i]->v;

				#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
				pVtx->color = currentState != -1 ? RGBA_MAKE((currentState << 1) + 50, (currentState << 1) + 50, (currentState << 1) + 50, (currentState << 1) + 50) : D3DRGBA(1.0f, 1.0f, 1.0f, 1.0f);
				#endif

				pVtx++;
			}

			break;
		}

		case MPR_VI_COLOR:
		{
			// Iterate for each vertex
			for(int i=0;i<nVerts;i++)
			{
				ShiAssert((BYTE *) pVtx < m_pVtxEnd);	// check for overrun

				pVtx->sx = pData[i]->x;
				pVtx->sy = pData[i]->y;
				pVtx->sz = 0.0f;
				pVtx->rhw = pData[i]->q > 0.0f ? 1.0f / (pData[i]->q / Q_SCALE) : 1.0f;
				pVtx->color = D3DRGBA(pData[i]->r, pData[i]->g, pData[i]->b, pData[i]->a);

				#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
				pVtx->color = currentState != -1 ? RGBA_MAKE((currentState << 1) + 50, (currentState << 1) + 50, (currentState << 1) + 50, (currentState << 1) + 50) : D3DRGBA(1.0f, 1.0f, 1.0f, 1.0f);
				#endif

				pVtx++;
			}

			break;
		}

		default:
		{
			ShiAssert(VtxInfo == NULL);

			// Iterate for each vertex
			for(int i=0;i<nVerts;i++)
			{
				ShiAssert((BYTE *) pVtx < m_pVtxEnd);	// check for overrun

				pVtx->sx = pData[i]->x;
				pVtx->sy = pData[i]->y;
				pVtx->sz = 0.0f;
				pVtx->rhw = pData[i]->q > 0.0f ? 1.0f / (pData[i]->q / Q_SCALE) : 1.0f;
				pVtx->color = m_colFG;

				#ifdef _CONTEXT_ENABLE_RENDERSTATE_HIGHLIGHT
				pVtx->color = currentState != -1 ? RGBA_MAKE((currentState << 1) + 50, (currentState << 1) + 50, (currentState << 1) + 50, (currentState << 1) + 50) : D3DRGBA(1.0f, 1.0f, 1.0f, 1.0f);
				#endif

				pVtx++;
			}

			break;
		}
	}

	// Generate Indices (in advance)
	if(m_nCurPrimType == D3DPT_TRIANGLEFAN)
	{
		WORD *pIdx = &m_pIdx[m_dwNumIdx];

		for(int x=0;x<nVerts-2;x++)
		{
			pIdx[0] = m_dwNumVtx;
			pIdx[1] = m_dwNumVtx + x + 1;
			pIdx[2] = m_dwNumVtx + x + 2;
			pIdx += 3;
		}

		m_dwNumIdx += pIdx - &m_pIdx[m_dwNumIdx];
	}

	else if(m_nCurPrimType == D3DPT_LINESTRIP)
	{
		WORD *pIdx = &m_pIdx[m_dwNumIdx];

		for(int x=0;x<nVerts;x++)
			*pIdx++ = m_dwNumVtx + x;

		m_dwNumIdx += nVerts;
	}

	m_dwNumVtx += nVerts;

	#ifdef _CONTEXT_FLUSH_EVERY_PRIMITIVE
	FlushVB();
	#endif
}

/////////////////////////////////////////////////////////////////////////////
// ContextMPR::Stats

ContextMPR::Stats::Stats()
{
	#ifdef _CONTEXT_ENABLE_STATS
	Init();
	#endif
}

#ifdef _CONTEXT_ENABLE_STATS
void ContextMPR::Stats::Check()
{
	DWORD Ticks = ::GetTickCount();

	if(Ticks - dwTicks > 1000)
	{
		dwTicks = Ticks;
		dwLastFPS = dwCurrentFPS;
		dwCurrentFPS = 0;

		if(dwCurPrimCountPerSecond > dwMaxPrimCountPerSecond)
			dwMaxPrimCountPerSecond = dwCurPrimCountPerSecond;

		if(dwCurVtxCountPerSecond > dwMaxVtxCountPerSecond)
			dwMaxVtxCountPerSecond = dwCurVtxCountPerSecond;

		if(dwTotalSeconds)
		{
			dwAvgVtxCountPerSecond = dwTotalVtxCount / dwTotalSeconds;
			dwAvgPrimCountPerSecond = dwTotalPrimCount / dwTotalSeconds;
		}

		if(dwTotalBatches)
		{
			dwAvgVtxBatchSize = dwTotalVtxBatchSize / dwTotalBatches;
			dwAvgPrimBatchSize = dwTotalPrimBatchSize / dwTotalBatches;
		}

		if(dwLastFPS < dwMinFPS)
			dwMinFPS = dwLastFPS;
		else if(dwLastFPS > dwMaxFPS)
			dwMaxFPS = dwLastFPS;

		dwTotalFPS += dwLastFPS;
		dwTotalSeconds++;
		dwAverageFPS = dwTotalFPS / dwTotalSeconds;
	}
}

void ContextMPR::Stats::Init()
{
	ZeroMemory(this, sizeof(*this));
}

void ContextMPR::Stats::StartFrame()
{
	dwCurrentFPS++;
	Check();
}

void ContextMPR::Stats::StartBatch()
{
	dwTotalBatches++;

	if(dwCurVtxBatchSize > dwMaxVtxBatchSize)
		dwMaxVtxBatchSize = dwCurVtxBatchSize;
	dwTotalVtxBatchSize += dwCurVtxBatchSize;
	dwCurVtxBatchSize = 0;

	if(dwCurPrimBatchSize > dwMaxPrimBatchSize)
		dwMaxPrimBatchSize = dwCurPrimBatchSize;
	dwTotalPrimBatchSize += dwCurPrimBatchSize;
	dwCurPrimBatchSize = 0;
}

void ContextMPR::Stats::Primitive(DWORD dwType, DWORD dwNumVtx)
{
	arrPrimitives[dwType - 1]++;
	dwTotalPrimitives++;
	dwCurPrimCountPerSecond++;
	dwCurVtxCountPerSecond += dwNumVtx;
	dwCurVtxBatchSize += dwNumVtx;
	dwCurPrimBatchSize++;
	dwTotalPrimCount++;
	dwTotalVtxCount += dwNumVtx;
}

void ContextMPR::Stats::PutTexture(bool bCached)
{
	dwPutTextureTotal++;
	if(bCached) dwPutTextureCached++;
}

void ContextMPR::Stats::Report()
{
	MonoPrint("Stats report follows\n");

	float fT = dwTotalPrimitives / 100.0f;

	MonoPrint("	MinFPS: %d\n", dwMinFPS);
	MonoPrint("	MaxFPS: %d\n", dwMaxFPS);
	MonoPrint("	AverageFPS: %d\n", dwAverageFPS);
	MonoPrint("	TotalPrimitives: %d\n", dwTotalPrimitives);
	MonoPrint("	Triangle Lists: %d (%.2f %%)\n", arrPrimitives[3], arrPrimitives[3] / fT);
	MonoPrint("	Triangle Strips: %d (%.2f %%)\n", arrPrimitives[4], arrPrimitives[4] / fT);
	MonoPrint("	Triangle Fans: %d (%.2f %%)\n", arrPrimitives[5], arrPrimitives[5] / fT);
	MonoPrint("	Point Lists: %d (%.2f %%)\n", arrPrimitives[0], arrPrimitives[0] / fT);
	MonoPrint("	Line Lists: %d (%.2f %%)\n", arrPrimitives[1], arrPrimitives[1] / fT);
	MonoPrint("	Line Strips: %d (%.2f %%)\n", arrPrimitives[2], arrPrimitives[2] / fT);
	MonoPrint("	AvgVtxBatchSize: %d\n", dwAvgVtxBatchSize);
	MonoPrint("	MaxVtxBatchSize: %d\n", dwMaxVtxBatchSize);
	MonoPrint("	AvgPrimBatchSize: %d\n", dwAvgPrimBatchSize);
	MonoPrint("	MaxPrimBatchSize: %d\n", dwMaxPrimBatchSize);
	MonoPrint("	AvgVtxCountPerSecond: %d\n", dwAvgVtxCountPerSecond);
	MonoPrint("	MaxVtxCountPerSecond: %d\n", dwMaxVtxCountPerSecond);
	MonoPrint("	AvgPrimCountPerSecond: %d\n", dwAvgPrimCountPerSecond);
	MonoPrint("	MaxPrimCountPerSecond: %d\n", dwMaxPrimCountPerSecond);
	MonoPrint("	TextureChangesTotal: %d\n", dwPutTextureTotal);
	MonoPrint("	TextureChangesCached: %d (%.2f %%)\n", dwPutTextureCached, (float) dwPutTextureCached / (dwPutTextureTotal / 100.0f));

	MonoPrint("End of stats report\n");
}

#endif
