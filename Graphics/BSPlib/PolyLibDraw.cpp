/***************************************************************************\
    PolyLibDraw.cpp
    Scott Randolph
    February 9, 1998

    Provides drawing functions 3D polygons of various types.
\***************************************************************************/
#include "stdafx.h"
#include "StateStack.h"
#include "ColorBank.h"
#include "TexBank.h"
#include "PolyLib.h"
#include "mpr_light.h"

extern int verts;

// This helper function is used for all non-interpolated primitives.
// The Gouraud shaded primitives do their own thing...
static inline void SetForegroundColor( DWORD opFlag, int rgbaIdx, int IIdx )
{
	Pcolor		*rgba;
	Pcolor		foggedColor;
	Pintensity	I;
	DWORD		color = 0;
#ifdef DEBUG
	int	colorSet = TRUE;
#endif

	if (opFlag & PRIM_COLOP_COLOR) {
		ShiAssert( rgbaIdx >= 0 );
		rgba = &TheColorBank.ColorPool[rgbaIdx];

		if (opFlag & PRIM_COLOP_FOG) {
			foggedColor.r = rgba->r * TheStateStack.fogValue_inv + TheStateStack.fogColor_premul.r;
			foggedColor.g = rgba->g * TheStateStack.fogValue_inv + TheStateStack.fogColor_premul.g;
			foggedColor.b = rgba->b * TheStateStack.fogValue_inv + TheStateStack.fogColor_premul.b;
			foggedColor.a = rgba->a;
			rgba = &foggedColor;
		}

		if (opFlag & PRIM_COLOP_INTENSITY) {
			ShiAssert( IIdx >= 0 );
			I    = TheStateStack.IntensityPool[IIdx];
			color	 = FloatToInt32(rgba->r * I * 255.9f);
			color	|= FloatToInt32(rgba->g * I * 255.9f) << 8;
			color	|= FloatToInt32(rgba->b * I * 255.9f) << 16;
		} else {
			color	 = FloatToInt32(rgba->r * 255.9f);
			color	|= FloatToInt32(rgba->g * 255.9f) << 8;
			color	|= FloatToInt32(rgba->b * 255.9f) << 16;
		}
		color	|= FloatToInt32(rgba->a * 255.9f) << 24;
	} else if (opFlag & PRIM_COLOP_INTENSITY) {
		ShiAssert( IIdx >= 0 );

		color	 = FloatToInt32(TheStateStack.IntensityPool[IIdx] * 255.9f);
		color	|= color << 8;
		color	|= color << 8;

		if (opFlag & PRIM_COLOP_FOG) {
			color |= FloatToInt32(TheStateStack.fogValue * 255.9f) << 24;
		}

	} else if (opFlag & PRIM_COLOP_FOG) {
		color = FloatToInt32(TheStateStack.fogValue * 255.9f) << 24;
	}
#ifdef DEBUG
	else 
	{
		colorSet = FALSE;
	}
#endif
	ShiAssert(colorSet);
	TheStateStack.context->SelectForegroundColor( color );
}

static inline void pvtDraw2DPrim(PpolyType type, int nVerts, int *xyzIdxPtr)
{
	TheStateStack.context->RestoreState(STATE_SOLID);
	TheStateStack.context->DrawPrimitive2D(type, nVerts, xyzIdxPtr);
}

static inline void pvtDraw2DLine(int *xyzIdxPtr)
{
	TheStateStack.context->RestoreState(STATE_SOLID);
	TheStateStack.context->Draw2DLine(&TheStateStack.XformedPosPool[xyzIdxPtr[0]], &TheStateStack.XformedPosPool[xyzIdxPtr[1]]);
}

static inline void pvtDraw2DPoint(int *xyzIdxPtr)
{
	TheStateStack.context->RestoreState(STATE_SOLID);
	TheStateStack.context->Draw2DPoint(&TheStateStack.XformedPosPool[xyzIdxPtr[0]]);
}

void DrawPrimPoint( PrimPointFC *point )
{
	ShiAssert( point->type == PointF );
	ShiAssert( point->nVerts >= 1 );

	verts += point->nVerts;

	SetForegroundColor( PRIM_COLOP_COLOR, point->rgba, -1 );
	pvtDraw2DPrim( PointF, point->nVerts, point->xyz );
}


void DrawPrimFPoint( PrimPointFC *point )
{
	ShiAssert( point->type == PointF );
	ShiAssert( point->nVerts >= 1 );

	verts += point->nVerts;

	SetForegroundColor( PRIM_COLOP_COLOR | PRIM_COLOP_FOG, point->rgba, -1 );
	pvtDraw2DPrim( PointF, point->nVerts, point->xyz );
}


void DrawPrimLine( PrimLineFC *line )
{
	ShiAssert( line->type == LineF );
	ShiAssert( line->nVerts >= 2 );

	verts += line->nVerts;

	SetForegroundColor( PRIM_COLOP_COLOR, line->rgba, -1 );
	ShiAssert(line->nVerts == 2);
//	pvtDraw2DPrim( LineF, line->nVerts, line->xyz );
	pvtDraw2DLine(line->xyz);
}


void DrawPrimFLine( PrimLineFC *line )
{
	ShiAssert( line->type == LineF );
	ShiAssert( line->nVerts >= 2 );

	verts += line->nVerts;

	SetForegroundColor( PRIM_COLOP_COLOR | PRIM_COLOP_FOG, line->rgba, -1 );
	ShiAssert(line->nVerts == 2);

//	pvtDraw2DPrim( LineF, line->nVerts, line->xyz );
	pvtDraw2DLine(line->xyz);
}

void DrawPoly( PolyFC *poly )
{
	verts += poly->nVerts;
	TheStateStack.context->RestoreState(RenderStateTable[poly->type]);
	SetForegroundColor( PRIM_COLOP_COLOR, poly->rgba, -1 );
	TheStateStack.context->DrawPoly( PRIM_COLOP_NONE, poly, poly->xyz, NULL, NULL, NULL, true);
}


void DrawPolyF( PolyFC *poly )
{
	verts += poly->nVerts;

	TheStateStack.context->RestoreState(RenderStateTable[poly->type]);
	SetForegroundColor( PRIM_COLOP_COLOR | PRIM_COLOP_FOG, poly->rgba, -1 );
	TheStateStack.context->DrawPoly( PRIM_COLOP_NONE, poly, poly->xyz, NULL, NULL, NULL, true);
}


void DrawPolyL( PolyFCN *poly )
{
	verts += poly->nVerts;
	TheStateStack.context->RestoreState(RenderStateTable[poly->type]);
	SetForegroundColor( PRIM_COLOP_COLOR | PRIM_COLOP_INTENSITY, poly->rgba, poly->I );
	TheStateStack.context->DrawPoly( PRIM_COLOP_NONE, poly, poly->xyz, NULL, NULL, NULL, true);
}


void DrawPolyFL( PolyFCN *poly )
{
	verts += poly->nVerts;

	TheStateStack.context->RestoreState(RenderStateTable[poly->type]);
	SetForegroundColor( PRIM_COLOP_COLOR | PRIM_COLOP_INTENSITY | PRIM_COLOP_FOG, poly->rgba, poly->I );
	TheStateStack.context->DrawPoly( PRIM_COLOP_NONE, poly, poly->xyz, NULL, NULL, NULL, true);
}


void DrawPolyG( PolyVC *poly )
{
	verts += poly->nVerts;

	TheStateStack.context->RestoreState(RenderStateTable[poly->type]);
	TheStateStack.context->DrawPoly( PRIM_COLOP_COLOR, poly, poly->xyz, poly->rgba, NULL, NULL );
}


void DrawPolyFG( PolyVC *poly )
{
	verts += poly->nVerts;

	TheStateStack.context->RestoreState(RenderStateTable[poly->type]);
	TheStateStack.context->DrawPoly( PRIM_COLOP_COLOR | PRIM_COLOP_FOG, poly, poly->xyz, poly->rgba, NULL, NULL );
}


void DrawPolyGL( PolyVCN *poly )
{
	verts += poly->nVerts;

	TheStateStack.context->RestoreState(RenderStateTable[poly->type]);
	TheStateStack.context->DrawPoly( PRIM_COLOP_COLOR | PRIM_COLOP_INTENSITY, poly, poly->xyz, poly->rgba, poly->I, NULL );
}


void DrawPolyFGL( PolyVCN *poly )
{
	verts += poly->nVerts;

	TheStateStack.context->RestoreState(RenderStateTable[poly->type]);
	TheStateStack.context->DrawPoly( PRIM_COLOP_COLOR | PRIM_COLOP_INTENSITY | PRIM_COLOP_FOG, poly, poly->xyz, poly->rgba, poly->I, NULL );
}


void DrawPolyT( PolyTexFC *poly )
{
	verts += poly->nVerts;

	TheStateStack.context->RestoreState(RenderStateTable[poly->type]);
	TheTextureBank.Select( TheStateStack.CurrentTextureTable[poly->texIndex] );
	TheStateStack.context->DrawPoly( PRIM_COLOP_TEXTURE, poly, poly->xyz, NULL, NULL, poly->uv );
}


void DrawPolyFT( PolyTexFC *poly )
{
	verts += poly->nVerts;

	TheStateStack.context->RestoreState(RenderStateTable[poly->type]);
	SetForegroundColor( PRIM_COLOP_FOG, -1, -1 );
	TheTextureBank.Select( TheStateStack.CurrentTextureTable[poly->texIndex] );
	TheStateStack.context->DrawPoly( PRIM_COLOP_TEXTURE, poly, poly->xyz, NULL, NULL, poly->uv, true);
}


void DrawPolyGT( PolyTexFC *poly )
{
	verts += poly->nVerts;

	TheStateStack.context->RestoreState(RenderStateTable[poly->type]);
	SetForegroundColor( PRIM_COLOP_FOG, -1, -1 );
	TheTextureBank.Select( TheStateStack.CurrentTextureTable[poly->texIndex] );
	TheStateStack.context->DrawPoly( PRIM_COLOP_TEXTURE, poly, poly->xyz, NULL, NULL, poly->uv, true);
}


void DrawPolyAT( PolyTexFC *poly )
{
	verts += poly->nVerts;

	TheStateStack.context->RestoreState(RenderStateTable[poly->type]);
	SetForegroundColor( PRIM_COLOP_COLOR, poly->rgba, -1 );
	TheTextureBank.Select( TheStateStack.CurrentTextureTable[poly->texIndex] );
	TheStateStack.context->DrawPoly( PRIM_COLOP_TEXTURE, poly, poly->xyz, NULL, NULL, poly->uv, true);
}


void DrawPolyTL( PolyTexFCN *poly )
{
	verts += poly->nVerts;

	TheStateStack.context->RestoreState(RenderStateTable[poly->type]);
	SetForegroundColor( PRIM_COLOP_INTENSITY, -1, poly->I );
	TheTextureBank.Select( TheStateStack.CurrentTextureTable[poly->texIndex] );
	TheStateStack.context->DrawPoly( PRIM_COLOP_TEXTURE, poly, poly->xyz, NULL, NULL, poly->uv, true);
}


void DrawPolyFTL( PolyTexFCN *poly )
{
	verts += poly->nVerts;

	TheStateStack.context->RestoreState(RenderStateTable[poly->type]);
	SetForegroundColor( PRIM_COLOP_INTENSITY | PRIM_COLOP_FOG, -1, poly->I );
	TheTextureBank.Select( TheStateStack.CurrentTextureTable[poly->texIndex] );
	TheStateStack.context->DrawPoly( PRIM_COLOP_TEXTURE, poly, poly->xyz, NULL, NULL, poly->uv, true);
}


void DrawPolyATL( PolyTexFCN *poly )
{
	verts += poly->nVerts;

	TheStateStack.context->RestoreState(RenderStateTable[poly->type]);
	SetForegroundColor( PRIM_COLOP_COLOR | PRIM_COLOP_INTENSITY, poly->rgba, poly->I );
	TheTextureBank.Select( TheStateStack.CurrentTextureTable[poly->texIndex] );
	TheStateStack.context->DrawPoly( PRIM_COLOP_TEXTURE, poly, poly->xyz, NULL, NULL, poly->uv, true);
}


void DrawPolyTG( PolyTexVC *poly )
{
	verts += poly->nVerts;

	TheStateStack.context->RestoreState(RenderStateTable[poly->type]);
	ShiAssert(( poly->type == TexG ) || ( poly->type == CTexG ));
	TheTextureBank.Select( TheStateStack.CurrentTextureTable[poly->texIndex] );
	TheStateStack.context->DrawPoly( PRIM_COLOP_TEXTURE, poly, poly->xyz, NULL, NULL, poly->uv );
}


void DrawPolyFTG( PolyTexVC *poly )
{
	verts += poly->nVerts;

	TheStateStack.context->RestoreState(RenderStateTable[poly->type]);
	ShiAssert(( poly->type == TexG ) || ( poly->type == CTexG ));
	SetForegroundColor( PRIM_COLOP_FOG, -1, -1 );
	TheTextureBank.Select( TheStateStack.CurrentTextureTable[poly->texIndex] );
	TheStateStack.context->DrawPoly( PRIM_COLOP_TEXTURE, poly, poly->xyz, NULL, NULL, poly->uv, true);
}


void DrawPolyATG( PolyTexVC *poly )
{
	verts += poly->nVerts;

	TheStateStack.context->RestoreState(RenderStateTable[poly->type]);
	ShiAssert(( poly->type == ATexG ) || ( poly->type == CATexG ));
	TheTextureBank.Select( TheStateStack.CurrentTextureTable[poly->texIndex] );
	TheStateStack.context->DrawPoly( PRIM_COLOP_COLOR | PRIM_COLOP_TEXTURE, poly, poly->xyz, poly->rgba, NULL, poly->uv );
}


void DrawPolyTGL( PolyTexVCN *poly )
{
	verts += poly->nVerts;

	TheStateStack.context->RestoreState(RenderStateTable[poly->type]);
	TheTextureBank.Select( TheStateStack.CurrentTextureTable[poly->texIndex] );
	TheStateStack.context->DrawPoly( PRIM_COLOP_INTENSITY | PRIM_COLOP_TEXTURE, poly, poly->xyz, NULL, poly->I, poly->uv );
}


void DrawPolyFTGL( PolyTexVCN *poly )
{
	verts += poly->nVerts;

	TheStateStack.context->RestoreState(RenderStateTable[poly->type]);
	TheTextureBank.Select( TheStateStack.CurrentTextureTable[poly->texIndex] );
	TheStateStack.context->DrawPoly( PRIM_COLOP_INTENSITY | PRIM_COLOP_TEXTURE | PRIM_COLOP_FOG, poly, poly->xyz, NULL, poly->I, poly->uv );
}


void DrawPolyATGL( PolyTexVCN *poly )
{
	verts += poly->nVerts;

	TheStateStack.context->RestoreState(RenderStateTable[poly->type]);
	TheTextureBank.Select( TheStateStack.CurrentTextureTable[poly->texIndex] );
	TheStateStack.context->DrawPoly( PRIM_COLOP_COLOR | PRIM_COLOP_INTENSITY | PRIM_COLOP_TEXTURE, poly, poly->xyz, poly->rgba, poly->I, poly->uv );
}

/*
DWORD dwTicks = GetTickCount();

for(int i=0;i<1000000;i++)
{
	verts += poly->nVerts;
	SetForegroundColor( PRIM_COLOP_COLOR, poly->rgba, -1 );
	TheStateStack.context->DrawPoly( PRIM_COLOP_NONE, poly, poly->xyz, NULL, NULL, NULL, true);
}

DWORD dwDelta = GetTickCount() - dwTicks;
*/