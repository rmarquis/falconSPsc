/***************************************************************************\
    WXCell.h
    Scott Randolph
    July 19, 1996

	Base class for a cloud tile.
\***************************************************************************/
#ifndef _WXCell_H_
#define _WXCell_H_

#include "ObjList.h"
#include "DrawOVC.h"
#include "grTypes.h"


// TODO:  Roll all this into the drawable class itself and eliminate the middle man.
class WeatherCell {
  public:
	WeatherCell()	{ drawable = NULL; };
	~WeatherCell()	{ ShiAssert( drawable == NULL ); };

	void	Setup( int row, int col, ObjectDisplayList* objList );
	void	Cleanup( ObjectDisplayList* objList );
	void	Reset( int row, int col, ObjectDisplayList* objList ) { Setup(row, col, objList); };

	void	UpdateForDrift( int cellRow, int cellCol )	{ if (drawable) drawable->UpdateForDrift( cellRow, cellCol ); };

	float	GetBase( void )		{ return base; };
	float	GetTop( void )		{ return top; };

	float	GetNWtop(void)	{ return drawable ? drawable->GetNWtop() : top; };
	float	GetNEtop(void)	{ return drawable ? drawable->GetNEtop() : top; };
	float	GetSWtop(void)	{ return drawable ? drawable->GetSWtop() : top; };
	float	GetSEtop(void)	{ return drawable ? drawable->GetSEtop() : top; };

	float	GetNWbottom(void)	{ return drawable ? drawable->GetNWbottom() : base; };
	float	GetNEbottom(void)	{ return drawable ? drawable->GetNEbottom() : base; };
	float	GetSWbottom(void)	{ return drawable ? drawable->GetSWbottom() : base; };
	float	GetSEbottom(void)	{ return drawable ? drawable->GetSEbottom() : base; };

	BOOL	IsEmpty(void)	{ return drawable == NULL; };
	float	GetAlpha( float x, float y )		{ return drawable ? drawable->GetTextureAlpha(x,y) : 0.0f; };

	float GetRainFactor() { return rainFactor; };
	float GetVisibility() { return visDistance; };
	bool GetLightning();

  protected:
	DrawableOvercast	*drawable;

	float	base;
	float	top;
// jpo additions
	float	visDistance;
	float rainFactor;
	bool lightning;
	float ltimer;
};

#endif // _WXCell_H_