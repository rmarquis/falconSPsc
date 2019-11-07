/***************************************************************************\
    MiscTex.cpp
    Scott Randolph
    March 27, 1997

	Hold information on miscellanious textures used by graphics system.
\***************************************************************************/
#include "Device.h"
#include "Texture.h"
#include "MiscTex.h"

#include "DrawSgmt.h"
#include "DrawOVC.h"
#include "Draw2d.h"
#include "RenderOW.h"


// The one and only miscellanious texture management database object
MiscTextureDB	TheMiscTextures;


/***************************************************************************\
	Initialize the miscellanious texture list
\***************************************************************************/
void MiscTextureDB::Setup( DisplayDevice *device, char *path )
{
	ShiAssert( device );
	ShiAssert( path );


	// Initialize the texture loader code
	Texture::SetupForDevice( device->GetDefaultRC(), path );


	// Setup the record for each texture in turn.
	Drawable2D::SetupTexturesOnDevice( device->GetDefaultRC() );
	DrawableTrail::SetupTexturesOnDevice( device->GetDefaultRC() );
	DrawableOvercast::SetupTexturesOnDevice( device->GetDefaultRC() );
	RenderOTW::SetupTexturesOnDevice( device->GetDefaultRC() );
}


/***************************************************************************\
	Clean up the miscellanious texture list
\***************************************************************************/
void MiscTextureDB::Cleanup( DisplayDevice *device )
{
	// Setup the record for each texture in turn.
	Drawable2D::ReleaseTexturesOnDevice( device->GetDefaultRC() );
	DrawableTrail::ReleaseTexturesOnDevice( device->GetDefaultRC() );
	DrawableOvercast::ReleaseTexturesOnDevice( device->GetDefaultRC() );
	RenderOTW::ReleaseTexturesOnDevice( device->GetDefaultRC() );
}

