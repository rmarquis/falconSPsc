#include "stdafx.h"

#include "falclib.h"
#include "cpsurface.h"
#include "cpmanager.h"
#include "dispcfg.h"
#include "Graphics\Include\grinline.h"
#include "FalcLib\include\playerop.h"
#include "Graphics\Include\renderow.h"


//====================================================//
// CPSurface::CPSurface
//====================================================//

CPSurface::CPSurface(SurfaceInitStr *psurfaceInitStr) {

	mIdNum				= psurfaceInitStr->idNum;

	mPersistant			= psurfaceInitStr->persistant;

	mSrcRect.top		= psurfaceInitStr->srcRect.top;	
	mSrcRect.left		= psurfaceInitStr->srcRect.left;
	mSrcRect.bottom	= psurfaceInitStr->srcRect.bottom;
	mSrcRect.right		= psurfaceInitStr->srcRect.right;

	mWidth				= psurfaceInitStr->srcRect.right - psurfaceInitStr->srcRect.left;
	mHeight				= psurfaceInitStr->srcRect.bottom - psurfaceInitStr->srcRect.top;

	mpOTWImage			= psurfaceInitStr->pOtwImage;
	mpSourceBuffer		= psurfaceInitStr->psrcBuffer;
	mpSurfaceBuffer	= NULL; 

	// OW
	m_pPalette = NULL;
}

//====================================================//
// CPSurface::~CPSurface
//====================================================//

CPSurface::~CPSurface()
{
	// OW
	for(int i=0;i<m_arrTex.size();i++) delete m_arrTex[i];
	m_arrTex.clear();

	if(m_pPalette) delete m_pPalette;

	// nota bene: the manager creates the buffer for us,
	// but we must clean it up
	
	if(mpSurfaceBuffer) {
		mpSurfaceBuffer->Cleanup();	
		delete mpSurfaceBuffer;	
	}

	glReleaseMemory((char*) mpSourceBuffer);
}

//====================================================//
// CPSurface::CreateLit
//====================================================//

void CPSurface::CreateLit(void)
{
	if(!PlayerOptions.bFast2DCockpit)
	{
		mpSurfaceBuffer = new ImageBuffer;

	// OW
	//	mpSurfaceBuffer->Setup(&FalconDisplay.theDisplayDevice, mWidth, mHeight, SystemMem, None);

		MPRSurfaceType front = (FalconDisplay.theDisplayDevice.IsHardware() && PlayerOptions.bFast2DCockpit) ? LocalVideoMem : SystemMem;
		if(!mpSurfaceBuffer->Setup(&FalconDisplay.theDisplayDevice, mWidth, mHeight, front,None) && front == LocalVideoMem)
		{
			// Retry with system memory if ouf video memory
			#ifdef _DEBUG
			MonoPrint("CPSurface::CreateLit - Probably out of video memory. Retrying with system memory)\n");
			#endif

			BOOL bResult = mpSurfaceBuffer->Setup(&FalconDisplay.theDisplayDevice, mWidth, mHeight, SystemMem, None);
			if(!bResult) return;
		}

		mpSurfaceBuffer->SetChromaKey(0xFFFF0000);
	}

	else
	{
		try
		{
			const DWORD dwMaxTextureWidth = mpOTWImage->GetDisplayDevice()->GetDefaultRC()->m_pD3DHWDeviceDesc->dwMaxTextureWidth;
			const DWORD dwMaxTextureHeight = mpOTWImage->GetDisplayDevice()->GetDefaultRC()->m_pD3DHWDeviceDesc->dwMaxTextureHeight;

			m_pPalette = new PaletteHandle(mpOTWImage->GetDisplayDevice()->GetDefaultRC()->m_pDD, 32, 256);
			if(!m_pPalette)
				throw _com_error(E_OUTOFMEMORY);

			// Check if we can use a single texture
			if(dwMaxTextureWidth >= mWidth && dwMaxTextureHeight >= mHeight) 
			{
				TextureHandle *pTex = new TextureHandle;
				if(!pTex)
					throw _com_error(E_OUTOFMEMORY);

				m_pPalette->AttachToTexture(pTex);
				if(!pTex->Create("CPSurface", MPR_TI_PALETTE | MPR_TI_CHROMAKEY, 8, mWidth, mHeight))
					throw _com_error(E_FAIL);

				if(!pTex->Load(0, 0xFFFF0000, (BYTE*) mpSourceBuffer, true, true))	// soon to be re-loaded by CPSurface::Translate3D
					throw _com_error(E_FAIL);

				m_arrTex.push_back(pTex);
			}

			else
			{
				// Create tiles
				int nRows = 0; // JB 010220 CTD
				if (dwMaxTextureHeight) // JB 010220 CTD
				{
					nRows = mHeight / dwMaxTextureHeight; // JB 010220 CTD
					if(mHeight % dwMaxTextureHeight) nRows++; // JB 010404 CTD enclosed in brackets
				}
				int nColumns = 0; // JB 010220 CTD
				if (dwMaxTextureWidth) // JB 010220 CTD
				{ 
					nColumns = mWidth / dwMaxTextureWidth;
					if(mWidth % dwMaxTextureWidth) nColumns++; // JB 010404 CTD enclosed in brackets
				}

				DWORD dwHeightRemaining = mHeight;

				for(int y=0;y<nRows;y++)
				{
					DWORD dwWidthRemaining = mWidth;

					for(int x=0;x<nColumns;x++)
					{
						TextureHandle *pTex = new TextureHandle;
						if(!pTex)
							throw _com_error(E_OUTOFMEMORY);

						m_pPalette->AttachToTexture(pTex);
						if(!pTex->Create("CPSurface - Tile", MPR_TI_PALETTE | MPR_TI_CHROMAKEY, 8,
							min(dwMaxTextureWidth, dwWidthRemaining), min(dwMaxTextureHeight, dwHeightRemaining)))
							throw _com_error(E_FAIL);

						DWORD dwOffset = (y * dwMaxTextureHeight) * mWidth;
						dwOffset += x * dwMaxTextureWidth;

						if(!pTex->Load(0, 0xFFFF0000, (BYTE*) mpSourceBuffer + dwOffset, true, true, mWidth))	// soon to be re-loaded by CPSurface::Translate3D
							throw _com_error(E_FAIL);

						m_arrTex.push_back(pTex);
						dwWidthRemaining -= dwMaxTextureWidth;
					}

					dwHeightRemaining -= dwMaxTextureHeight;
				}
			}
		}

		catch(_com_error e)
		{
			MonoPrint("CPSurface::CreateLit - Error 0x%X (%s)\n", e.Error(), e.ErrorMessage());
			DiscardLit();
		}
	}
}

//====================================================//
// CPSurface::DiscardLit
//====================================================//

void CPSurface::DiscardLit(void)
{
	if(mpSurfaceBuffer)
		mpSurfaceBuffer->Cleanup();

	delete mpSurfaceBuffer;
	mpSurfaceBuffer = NULL;

	for(int i=0;i<m_arrTex.size();i++) delete m_arrTex[i];
	m_arrTex.clear();

	if(m_pPalette)
	{
	  delete m_pPalette; // JPO - memory leak fix
		m_pPalette = NULL;
	}
}



//====================================================//
// CPSurface::Display
//====================================================//

void CPSurface::DisplayBlit(BYTE blitType, BOOL Persistance, RECT *pDestRect, int xPanelOffset, int yPanelOffset)
{
	if(m_arrTex.size())
		return;		// handled in DisplayBlit3D
 	
	RECT blitRect;
	RECT destRect;
	
	destRect.top		= pDestRect->top;
	destRect.left		= pDestRect->left;
	destRect.bottom	= pDestRect->bottom;
	destRect.right		= pDestRect->right;

	blitRect.top		= 0;
	blitRect.left		= 0;
	blitRect.bottom	= mHeight;
	blitRect.right		= mWidth;

	destRect.top		= OTWDriver.pCockpitManager->mScale * (destRect.top + yPanelOffset);
	destRect.left		= OTWDriver.pCockpitManager->mScale * (destRect.left + xPanelOffset);
	destRect.bottom	= OTWDriver.pCockpitManager->mScale * (destRect.bottom + yPanelOffset + 1);
	destRect.right		= OTWDriver.pCockpitManager->mScale * (destRect.right + xPanelOffset + 1);

	if(Persistance == NONPERSISTANT)
	{
		if(blitType == TRANSPARENT)
			mpOTWImage->ComposeTransparent(mpSurfaceBuffer, &blitRect, &destRect);

		else
			mpOTWImage->Compose(mpSurfaceBuffer, &blitRect, &destRect);
	}

	else {	/* do it to both buffers */ }
}

void CPSurface::DisplayBlit3D(BYTE blitType, BOOL Persistance, RECT *pDestRect, int xPanelOffset, int yPanelOffset)
{
	if(!m_arrTex.size())
		return;		// handled in DisplayBlit

	RECT destRect;
	
	destRect.top		= pDestRect->top;
	destRect.left		= pDestRect->left;
	destRect.bottom	= pDestRect->bottom;
	destRect.right		= pDestRect->right;

	destRect.top		= OTWDriver.pCockpitManager->mScale * (destRect.top + yPanelOffset);
	destRect.left		= OTWDriver.pCockpitManager->mScale * (destRect.left + xPanelOffset);
	destRect.bottom	= OTWDriver.pCockpitManager->mScale * (destRect.bottom + yPanelOffset + 1);
	destRect.right		= OTWDriver.pCockpitManager->mScale * (destRect.right + xPanelOffset + 1);

	if(Persistance == NONPERSISTANT)
	{
		if(m_arrTex.size() == 1)
		{
			// One pass
			TextureHandle *pTex = m_arrTex[0];

			// Setup vertices
			float fStartU = 0;
			float fMaxU = (float) pTex->m_nWidth / (float) pTex->m_nActualWidth;
			fMaxU -= fStartU;

			float fStartV = 0;
			float fMaxV = (float) pTex->m_nHeight / (float) pTex->m_nActualHeight;
			fMaxV -= fStartV;

			TwoDVertex pVtx[4];
			ZeroMemory(pVtx, sizeof(pVtx));
			pVtx[0].x = destRect.left; pVtx[0].y = destRect.top; pVtx[0].u = fStartU; pVtx[0].v = fStartV;
			pVtx[0].r = pVtx[0].g = pVtx[0].b = pVtx[0].a = 1.0f;
			pVtx[1].x = destRect.right; pVtx[1].y = destRect.top; pVtx[1].u = fMaxU; pVtx[1].v = fStartV;
			pVtx[1].r = pVtx[1].g = pVtx[1].b = pVtx[1].a = 1.0f;
			pVtx[2].x = destRect.right; pVtx[2].y = destRect.bottom; pVtx[2].u = fMaxU; pVtx[2].v = fMaxV;
			pVtx[2].r = pVtx[2].g = pVtx[2].b = pVtx[2].a = 1.0f;
			pVtx[3].x = destRect.left; pVtx[3].y = destRect.bottom; pVtx[3].u = fStartU; pVtx[3].v = fMaxV;
			pVtx[3].r = pVtx[3].g = pVtx[3].b = pVtx[3].a = 1.0f;

			// Setup state
			if(blitType == TRANSPARENT)
				OTWDriver.renderer->context.RestoreState(STATE_TEXTURE_TRANSPARENCY);
			else
				OTWDriver.renderer->context.RestoreState(STATE_TEXTURE);

			OTWDriver.renderer->context.SelectTexture((GLint) pTex);

			// Render it (finally)
			OTWDriver.renderer->context.DrawPrimitive(MPR_PRM_TRIFAN, MPR_VI_COLOR | MPR_VI_TEXTURE, 4, pVtx, sizeof(pVtx[0]));
		}

		else
		{
			// Tile

			// Setup state
			if(blitType == TRANSPARENT)
				OTWDriver.renderer->context.RestoreState(STATE_TEXTURE_TRANSPARENCY);
			else
				OTWDriver.renderer->context.RestoreState(STATE_TEXTURE);

			const DWORD dwMaxTextureWidth = mpOTWImage->GetDisplayDevice()->GetDefaultRC()->m_pD3DHWDeviceDesc->dwMaxTextureWidth;
			const DWORD dwMaxTextureHeight = mpOTWImage->GetDisplayDevice()->GetDefaultRC()->m_pD3DHWDeviceDesc->dwMaxTextureHeight;

			int nRows = mHeight / dwMaxTextureHeight;
			if(mHeight % dwMaxTextureHeight) nRows++;
			int nColumns = mWidth / dwMaxTextureWidth;
			if(mWidth % dwMaxTextureWidth) nColumns++;

			TwoDVertex pVtx[4];
			ZeroMemory(pVtx, sizeof(pVtx));
			TextureHandle *pTex;
			int left, right, top, bottom;

			DWORD dwHeightRemaining = mHeight;
			top = destRect.top;

			for(int y=0;y<nRows;y++)
			{
				DWORD dwWidthRemaining = mWidth;
				left = destRect.left;
				bottom = top + min(dwMaxTextureHeight, dwHeightRemaining);
				
				for(int x=0;x<nColumns;x++)
				{
					pTex = m_arrTex[y * nColumns + x];
					right = left + min(dwMaxTextureWidth, dwWidthRemaining);
	
					// Setup vertices
					float fStartU = 0;
					float fMaxU = (float) pTex->m_nWidth / (float) pTex->m_nActualWidth;
					fMaxU -= fStartU;

					float fStartV = 0;
					float fMaxV = (float) pTex->m_nHeight / (float) pTex->m_nActualHeight;
					fMaxV -= fStartV;

					pVtx[0].x = left; pVtx[0].y = top; pVtx[0].u = fStartU; pVtx[0].v = fStartV;
					pVtx[0].r = pVtx[0].g = pVtx[0].b = pVtx[0].a = 1.0f;
					pVtx[1].x = right; pVtx[1].y = top; pVtx[1].u = fMaxU; pVtx[1].v = fStartV;
					pVtx[1].r = pVtx[1].g = pVtx[1].b = pVtx[1].a = 1.0f;
					pVtx[2].x = right; pVtx[2].y = bottom; pVtx[2].u = fMaxU; pVtx[2].v = fMaxV;
					pVtx[2].r = pVtx[2].g = pVtx[2].b = pVtx[2].a = 1.0f;
					pVtx[3].x = left; pVtx[3].y = bottom; pVtx[3].u = fStartU; pVtx[3].v = fMaxV;
					pVtx[3].r = pVtx[3].g = pVtx[3].b = pVtx[3].a = 1.0f;

					OTWDriver.renderer->context.SelectTexture((GLint) pTex);

					// Render it (finally)
					OTWDriver.renderer->context.DrawPrimitive(MPR_PRM_TRIFAN, MPR_VI_COLOR | MPR_VI_TEXTURE, 4, pVtx, sizeof(pVtx[0]));

					dwWidthRemaining -= dwMaxTextureWidth;
					left += dwMaxTextureWidth;
				}

				dwHeightRemaining -= dwMaxTextureHeight;
				top += dwMaxTextureHeight;
			}
		}
	}

	else {	/* do it to both buffers */ }
}


void CPSurface::Translate(WORD* palette16)
{
	if(mpSurfaceBuffer)
		Translate8to16(palette16, mpSourceBuffer, mpSurfaceBuffer); // 8 bit color indexes of individual surfaces
}																					// 16 bit ImageBuffers

// OW
void CPSurface::Translate(DWORD* palette32)
{
	if(mpSurfaceBuffer)
		Translate8to32(palette32, mpSourceBuffer, mpSurfaceBuffer); // 8 bit color indexes of individual surfaces
}

void CPSurface::Translate3D(DWORD* palette32)
{
	if(m_pPalette)
		m_pPalette->Load(MPR_TI_PALETTE, 32, 0, 256, (BYTE*) palette32);
}