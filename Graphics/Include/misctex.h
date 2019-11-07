/***************************************************************************\
    MiscTex.h
    Scott Randolph
    March 27, 1997

	Hold information on miscellanious textures used by graphics system.
\***************************************************************************/
#ifndef _MISCTEX_H_
#define _MISCTEX_H_

#include "shi\ShiError.h"
#include "grtypes.h"


// The one and only miscellanious texture management database object
extern class MiscTextureDB	TheMiscTextures;


class MiscTextureDB {
  public:
	MiscTextureDB()		{};
	~MiscTextureDB()	{};

	void	Setup( class DisplayDevice *device, char *path );
	void	Cleanup( class DisplayDevice *device );
};

#endif // _MISCTEX_H_