/***************************************************************************\
    Render2D.cpp
    Scott Randolph
    December 29, 1995

    This class provides 2D drawing functions.
\***************************************************************************/
#include <math.h>
#include "falclib\include\debuggr.h"
#include "Image.h"
#include "Device.h"
#include "Render2D.h"
#include "GraphicsRes.h"
#include "Tex.h"
#include "GraphicsRes.h"

#ifdef USE_TEXTURE_FONT
Texture FontTexture[NUM_FONT_RESOLUTIONS];
#endif

/***************************************************************************\
	Setup the rendering context for this display
\***************************************************************************/
void Render2D::Setup( ImageBuffer *imageBuffer )
{
	BOOL	result;

	image = imageBuffer;

	// Create the MPR rendering context (frame buffer, etc.)

	// OW
	//result = context.Setup( (DWORD)imageBuffer->targetSurface(), (DWORD)imageBuffer->GetDisplayDevice()->GetMPRdevice());
	result = context.Setup( imageBuffer, imageBuffer->GetDisplayDevice()->GetDefaultRC());

	if ( !result ) {
		ShiError( "Failed to setup rendering context" );
	}

	// Store key properties of our target buffer
	xRes = image->targetXres();
	yRes = image->targetYres();


	// Set the renderer's default foreground and background colors
	SetColor( 0xFFFFFFFF );
	SetBackground( 0xFF000000 );

	// Call our base classes setup function (must come AFTER xRes and Yres have been set)
	VirtualDisplay::Setup();
}



/***************************************************************************\
    Shutdown the renderer.
\***************************************************************************/
void Render2D::Cleanup( void )
{
	context.Cleanup();

	VirtualDisplay::Cleanup();
}



/***************************************************************************\
    Replace the image buffer used by this renderer
\***************************************************************************/
void Render2D::SetImageBuffer( ImageBuffer *imageBuffer )
{
	// Remember who our new image buffer is, and tell MPR about the change
	image = imageBuffer;
	context.NewImageBuffer( (DWORD)imageBuffer->targetSurface() );

	// This shouldn't be required, but _might_ be
	context.InvalidateState();
	context.RestoreState( STATE_SOLID );

	// Store key properties of our target buffer
	xRes = image->targetXres();
	yRes = image->targetYres();

	// Setup the default viewport
	SetViewport( -1.0f, 1.0f, 1.0f, -1.0f );

	// Setup the default offset and rotation
	CenterOriginInViewport();
	ZeroRotationAboutOrigin();
}



/***************************************************************************\
    Shutdown the renderer.
\***************************************************************************/
void Render2D::StartFrame( void )
{
	ShiAssert( image );
	context.StartFrame();
}



/***************************************************************************\
    Shutdown the renderer.
\***************************************************************************/
void Render2D::FinishFrame( void )
{
	ShiAssert( image );
	context.FinishFrame( NULL );
}



/***************************************************************************\
	Set the dimensions and location of the viewport.
\***************************************************************************/
void Render2D::SetViewport( float l, float t, float r, float b )
{
	// First call the base classes version of this function
	VirtualDisplay::SetViewport( l, t, r, b );

	// Send the new clipping region to MPR
	// (top/right inclusive, bottom/left exclusive)
	context.SetState( MPR_STA_ENABLES, MPR_SE_SCISSORING );
	context.SetState( MPR_STA_SCISSOR_TOP,		FloatToInt32((float)floor(topPixel)) );
	context.SetState( MPR_STA_SCISSOR_LEFT,		FloatToInt32((float)floor(leftPixel)) );
	context.SetState( MPR_STA_SCISSOR_RIGHT,	FloatToInt32((float)ceil(rightPixel)) );
	context.SetState( MPR_STA_SCISSOR_BOTTOM,	FloatToInt32((float)ceil(bottomPixel)) );
}



/***************************************************************************\
	Put a pixel on the display.
\***************************************************************************/
void Render2D::Render2DPoint( float x1, float y1 )
{
#if 1
	context.Draw2DPoint(x1, y1);
#else
	MPRVtx_t	vert;

#if 0 // This should be here, but I need to do some fixes first...
   //Clip test
   if (
		!((x1 <= rightPixel)  &&
		(x1 >= leftPixel)   &&
		(y1 <= bottomPixel) &&
		(y1 >= topPixel))
      )
	  return;

   ShiAssert( 
      (x1 <= rightPixel)  &&
      (x1 >= leftPixel)   &&
      (y1 <= bottomPixel) &&
      (y1 >= topPixel)
      )
#endif

	// Package up the point's coordinates
	vert.x = x1;
	vert.y = y1;

	// Draw the point
	context.DrawPrimitive(MPR_PRM_POINTS, 0, 1, &vert, sizeof(vert));
#endif
}



/***************************************************************************\
	Put a straight line on the display.
\***************************************************************************/
void Render2D::Render2DLine( float x1, float y1, float x2, float y2 )
{
#if 1
	context.Draw2DLine(x1, y1, x2, y2);
#else
	MPRVtx_t	verts[2];

#if 0 // This should be here, but I need to do some fixes first...
   //Clip test
   if ( 
          !((max (x1, x2) <= rightPixel)  &&
          (min (x1, x2) >= leftPixel)   &&
          (max (y1, y2) <= bottomPixel) &&
          (min (y1, y2) >= topPixel))
      )
	  return;
   ShiAssert( 
      (max (x1, x2) <= rightPixel)  &&
      (min (x1, x2) >= leftPixel)   &&
      (max (y1, y2) <= bottomPixel) &&
      (min (y1, y2) >= topPixel)
      )
#endif

	// Package up the lines's coordinates
	verts[0].x = x1;
	verts[0].y = y1;
	verts[1].x = x2;
	verts[1].y = y2;

	// Draw the line
	context.DrawPrimitive(MPR_PRM_LINES, 0, 2, verts, sizeof(verts[0]));
#endif
}



/***************************************************************************\
	Put a mono-colored screen space triangle on the display.
\***************************************************************************/
void Render2D::Render2DTri( float x1, float y1, float x2, float y2, float x3, float y3 )
{
	MPRVtx_t	verts[3];

   //Clip test
   if (
      (max (max (x1, x2), x3) > rightPixel)		||
      (min (min (x1, x2), x3) < leftPixel)		||
      (max (max (y1, y2), y3) > bottomPixel)	||
      (min (min (y1, y2), y3) < topPixel)
      )
      return;

	// Package up the tri's coordinates
	verts[0].x = x1;
	verts[0].y = y1;
	verts[1].x = x2;
	verts[1].y = y2;
	verts[2].x = x3;
	verts[2].y = y3;

	// Draw the triangle
//	context.RestoreState( STATE_ALPHA_SOLID );
	context.DrawPrimitive(MPR_PRM_TRIANGLES, 0, 3, verts, sizeof(verts[0]));
}



/***************************************************************************\
	Put a portion of a caller supplied 32 bit bitmap on the display.
	The pixels should be of the form 0x00BBGGRR
	Chroma keying is not supported
\***************************************************************************/
void Render2D::Render2DBitmap( int sX, int sY, int dX, int dY, int w, int h, int totalWidth, DWORD *source )
{
// OW
#if 0
	// Flush the message queue since this call will bypass the queue
	context.SendCurrentPacket();

	// Copy the required portion of the image to the draw buffer
	MPRBitmap(  context.rc, 32, 
					(short)w,
					(short)h,
					(unsigned short)(4*totalWidth), 
					(short)sX, (short)sY,
					(short)dX, (short)dY,
					(unsigned char *)source );
#else
	context.Render2DBitmap(sX, sY, dX, dY, w, h, totalWidth, source);
#endif
}



/***************************************************************************\
	Put a portion of a bitmap from a file on disk on the display.
	Chroma keying is not supported
\***************************************************************************/
void Render2D::Render2DBitmap( int sX, int sY, int dX, int dY, int w, int h, char *filename )
{
	int					result;
	CImageFileMemory 	texFile;
	int					totalWidth;
	DWORD				*dataptr;

	
	// Make sure we recognize this file type
	texFile.imageType = CheckImageType( filename );
	ShiAssert( texFile.imageType != IMAGE_TYPE_UNKNOWN );

	// Open the input file
	result = texFile.glOpenFileMem( filename );
	ShiAssert( result == 1 );

	// Read the image data (note that ReadTextureImage will close texFile for us)
	texFile.glReadFileMem();
	result = ReadTextureImage( &texFile );
	if (result != GOOD_READ) {
		ShiError( "Failed to read bitmap.  CD Error?" );
	}

	// Store the image size (check it in debug mode)
	totalWidth = texFile.image.width;
	ShiAssert( sX+w <= texFile.image.width );
	ShiAssert( sY+h <= texFile.image.height );

	// Force the data into 32 bit color
	dataptr = (DWORD*)ConvertImage( &texFile.image, COLOR_16M, NULL );
	ShiAssert( dataptr );

	// Release the unconverted image data
	// edg: I've seen palette be NULL
	if ( texFile.image.palette )
			glReleaseMemory( (char*)texFile.image.palette );
	glReleaseMemory( (char*)texFile.image.image );

	// Pass the bitmap data into the bitmap display function
	Render2DBitmap( sX, sY, dX, dY, w, h, totalWidth, dataptr );

	// Release the converted image data
	glReleaseMemory( dataptr );
}



/***************************************************************************\
	Put a mono-colored string of text on the display in screen space.
	(The location given is used as the upper left corner of the text in units of pixels)
\***************************************************************************/
void Render2D::ScreenText( float xLeft, float yTop, char *string, int boxed )
{
#ifdef USE_ORIGINAL_FONT
	int			x, y;
	int			width;
	unsigned	num;

	x = FloatToInt32( xLeft );
	y = FloatToInt32( yTop );

	// If this string falls off the top or bottom of the display, don't draw it
	if ((y+8 > FloatToInt32(bottomPixel)) || (y-1 < FloatToInt32(topPixel))) {
		return;
	}

	// Draw the string one character at a time
	while (*string)
	{

		// Get the font array offset
		num = FontLUT[*(unsigned char *)string];
		ShiAssert( num < FontLength );

		// Get the width of this character
		width = Font[num][8];

		// Draw only if we're on the screen
		if ((x + width+1 > FloatToInt32(rightPixel)) || (x-1 < FloatToInt32(leftPixel)))
		{
			// Skip
		} else
		{

			context.SendCurrentPacket();	/* Flush the queue since this will bypass it */


			if (boxed == 2)
			{
				const unsigned char *map = InvFont[num];
				MPRBitmap( context.rc,
						   1,				// Bits per pixel
						   width+2,			// width in pixels
						   8,				// height in pixels
						   1,				// width of source image in bytes
						   0, 0,			// source upper left
						   x-1, y-1,		// destination upper left
						   (unsigned char *)map );
			}
			else
			{
				const unsigned char *map = Font[num];
				MPRBitmap( context.rc,
						   1,		// Bits per pixel
						   width,	// width in pixels
						   6,		// height in pixels
						   1,		// width of source image in bytes
						   0, 1,	// source upper left
						   x, y,	// destination upper left
						   (unsigned char *)map );
			}
		}

		// Update the target location for the next character
		string++;
		x += width+1;
	}

	// Go back and box the string if necessary
	if (boxed == 1)
	{
		float x1 = xLeft - 2.0f;
		float y1 = yTop  - 2.0f;
		float x2 = (float)(x + 1);
		float y2 = yTop +  7.0f;
		
		// Only draw the box if it is entirely on screen
		if ((x1 > leftPixel) && (x2 < rightPixel) && (y1 > topPixel) && (y2 < bottomPixel)) {
			Render2DLine (x1, y1, x2, y1);
			Render2DLine (x2, y1, x2, y2);
			Render2DLine (x2, y2, x1, y2);
			Render2DLine (x1, y2, x1, y1);
		}
	}

   // Left Arrow
   if (boxed == 0x4)
   {
      float x0 = xLeft - 5.0F;
      float y0 = yTop + 2.0F;
		float x1 = xLeft - 2.0f;
		float y1 = yTop  - 2.0f;
		float x2 = (float)(x + 1);
		float y2 = yTop +  7.0f;

      x1 = max (x1, leftPixel + 1.0F);
		
		// Only draw the box if it is entirely on screen
		if ((x1 > leftPixel) && (x2 < rightPixel) && (y1 > topPixel) && (y2 < bottomPixel)) {
         Render2DLine (x0, y0, x1, y1);
         Render2DLine (x0, y0, x1, y2);
			Render2DLine (x1, y1, x2, y1);
			Render2DLine (x2, y1, x2, y2);
			Render2DLine (x2, y2, x1, y2);
		}
   }

   // Right Arrow
   if (boxed == 0x8)
   {
      float x0 = float (x + 4);
      float y0 = yTop + 2.0F;
		float x1 = xLeft - 2.0f;
		float y1 = yTop  - 2.0f;
		float x2 = (float)(x + 1);
		float y2 = yTop +  7.0f;
		
		// Only draw the box if it is entirely on screen
      x1 = max (x1, leftPixel + 1.0F);
		if ((x1 > leftPixel) && (x2 < rightPixel) && (y1 > topPixel) && (y2 < bottomPixel)) {
         Render2DLine (x0, y0, x2, y1);
         Render2DLine (x0, y0, x2, y2);
			Render2DLine (x1, y1, x2, y1);
			Render2DLine (x1, y1, x1, y2);
			Render2DLine (x2, y2, x1, y2);
		}
   }
#elif defined USE_TEXTURE_FONT

// OW this was formerly a major peformance killer
#if 1
float			x, y;
TwoDVertex vert[4 * 256];
int color;
float r, g, b;

	x = (float)floor(xLeft);
	y = (float)floor(yTop);

	// Select font texture here

	color = Color();

	// Draw two tris to make a square;
	if (boxed != 2)
	{
		r = (color & 0xFF)/255.0F;
		g = ((color & 0xFF00) >> 8)/255.0F;
		b = ((color & 0xFF0000) >> 16)/255.0F;
	}
	else
	{
		// boxed == 2 means inverse text, so draw the square
		r = 0.0F;
		g = 0.0F;
		b = 0.0F;
		vert[0].x = x - 1.8F;	//MI changed from - 2.0F
		vert[0].y = y;
		vert[1].x = vert[0].x;
		vert[1].y = vert[0].y + FontData[FontNum][32].pixelHeight - 1; //MI added -1
		vert[2].x = vert[0].x + ScreenTextWidth(string) + 1.8F;	//MI changed from +4.0F
		vert[2].y = vert[1].y;
		vert[3].x = vert[2].x;
		vert[3].y = vert[0].y;
		context.RestoreState( STATE_SOLID );
		context.DrawPrimitive(MPR_PRM_TRIFAN, 0, 4, vert, sizeof(vert[0]));
	}

	context.RestoreState( STATE_TEXTURE_GOURAUD_TRANSPARENCY );
	context.SelectTexture( FontTexture[FontNum].TexHandle() );

	TwoDVertex *pVtx = vert;

	int n = 0;

	while (*string)
	{
		// Top Left 1
		pVtx[0].x = x;
		pVtx[0].y = y;
		pVtx[0].r = r;
		pVtx[0].g = g;
		pVtx[0].b = b;
		pVtx[0].a = 1.0F;
		pVtx[0].u = FontData[FontNum][*string].left;
		pVtx[0].v = FontData[FontNum][*string].top;
		pVtx[0].q = 1.0F;

		// Top Right 1
		pVtx[1].x = x + (FontData[FontNum][*string].width * 256.0f);
		pVtx[1].y = y;
		pVtx[1].r = r;
		pVtx[1].g = g;
		pVtx[1].b = b;
		pVtx[1].a = 1.0F;
		pVtx[1].u = FontData[FontNum][*string].left + FontData[FontNum][*string].width;
		pVtx[1].v = FontData[FontNum][*string].top;
		pVtx[1].q = 1.0F;

		// Bottom Left 1
		pVtx[2].x = x;
		pVtx[2].y = y + FontData[FontNum][*string].pixelHeight;
		pVtx[2].r = r;
		pVtx[2].g = g;
		pVtx[2].b = b;
		pVtx[2].a = 1.0F;
		pVtx[2].u = FontData[FontNum][*string].left;
		pVtx[2].v = FontData[FontNum][*string].top + FontData[FontNum][*string].height;
		pVtx[2].q = 1.0F;

		// Bottom Left 2
		pVtx[3] = pVtx[2];

		// Top Right 2
		pVtx[4] = pVtx[1];

		// Bottom Right 2
		pVtx[5].x = x + (FontData[FontNum][*string].width * 256.0f);
		pVtx[5].y = y + FontData[FontNum][*string].pixelHeight;
		pVtx[5].r = r;
		pVtx[5].g = g;
		pVtx[5].b = b;
		pVtx[5].a = 1.0F;
		pVtx[5].u = FontData[FontNum][*string].left + FontData[FontNum][*string].width;
		pVtx[5].v = FontData[FontNum][*string].top + FontData[FontNum][*string].height;
		pVtx[5].q = 1.0F;

		// Do a block clip
		if(!(pVtx[0].x <= rightPixel && pVtx[0].x >= leftPixel && pVtx[0].y <= bottomPixel && pVtx[0].y >= topPixel))
			break;
		if(!(pVtx[1].x <= rightPixel && pVtx[1].x >= leftPixel && pVtx[1].y <= bottomPixel && pVtx[1].y >= topPixel))
			break;
		if(!(pVtx[2].x <= rightPixel && pVtx[2].x >= leftPixel && pVtx[2].y <= bottomPixel && pVtx[2].y >= topPixel))
			break;
		if(!(pVtx[5].x <= rightPixel && pVtx[5].x >= leftPixel && pVtx[5].y <= bottomPixel && pVtx[5].y >= topPixel))
			break;

		x += FontData[FontNum][*string].pixelWidth;
		string++;
		n++;
		pVtx += 6;
	}

	ShiAssert(n < 256);
	if(n) context.DrawPrimitive(MPR_PRM_TRIANGLES, MPR_VI_COLOR | MPR_VI_TEXTURE, n * 6, vert, sizeof(vert[0]));
	context.RestoreState( STATE_SOLID );

	// Go back and box the string if necessary
	if (boxed == 1)
	{
		float x1 = xLeft - 2.0f;
		float y1 = yTop  - 2.0f;
		float x2 = (float)(x + 1);
		float y2 = yTop + FontData[FontNum][32].pixelHeight;
		
		// Only draw the box if it is entirely on screen
		if ((x1 > leftPixel) && (x2 < rightPixel) && (y1 > topPixel) && (y2 < bottomPixel)) {
			Render2DLine (x1, y1, x2, y1);
			Render2DLine (x2, y1, x2, y2);
			Render2DLine (x2, y2, x1, y2);
			Render2DLine (x1, y2, x1, y1);
		}
	}

   // Left Arrow
   if (boxed == 0x4)
   {
	   //MI
#if 0
      float x0 = xLeft - 5.0F;
	  float y0 = yTop + 2.0F;
		float x1 = xLeft - 2.0f;
		float y1 = yTop  - 2.0f;
		float x2 = (float)(x + 1);
		float y2 = yTop + FontData[FontNum][32].pixelHeight;
#else
		float x0 = xLeft - 5.0F;
		float y0 = yTop + (FontData[FontNum][32].pixelHeight / 2);
		float x1 = xLeft - 2.0f;
		float y1 = yTop;
		float x2 = (float)(x + 1);
		float y2 = yTop + FontData[FontNum][32].pixelHeight;
#endif

      x1 = max (x1, leftPixel + 1.0F);
		
		// Only draw the box if it is entirely on screen
		if ((x1 > leftPixel) && (x2 < rightPixel) && (y1 > topPixel) && (y2 < bottomPixel)) {
         Render2DLine (x0, y0, x1, y1);
         Render2DLine (x0, y0, x1, y2);
			Render2DLine (x1, y1, x2, y1);
			Render2DLine (x2, y1, x2, y2);
			Render2DLine (x2, y2, x1, y2);
		}
   }

   // Right Arrow
   if (boxed == 0x8)
   {
	   //MI
#if 0
	   float x0 = float (x + 4);
	   float y0 = yTop + 2.0F;
	   float x1 = xLeft - 2.0f;
	   float y1 = yTop  - 2.0f;
	   float x2 = (float)(x + 1);
	   float y2 = yTop + FontData[FontNum][32].pixelHeight;
#else
	   float x0 = float (x + 4);
	   float y0 = yTop + (FontData[FontNum][32].pixelHeight / 2);
	   float x1 = xLeft - 2.0f;
	   float y1 = yTop;
	   float x2 = (float)(x + 1);
	   float y2 = yTop + FontData[FontNum][32].pixelHeight;
#endif
		
		// Only draw the box if it is entirely on screen
      x1 = max (x1, leftPixel + 1.0F);
		if ((x1 > leftPixel) && (x2 < rightPixel) && (y1 > topPixel) && (y2 < bottomPixel)) {
         Render2DLine (x0, y0, x2, y1);
         Render2DLine (x0, y0, x2, y2);
			Render2DLine (x1, y1, x2, y1);
			Render2DLine (x1, y1, x1, y2);
			Render2DLine (x2, y2, x1, y2);
		}
   }
#else
float			x, y;
TwoDVertex vert[4];
int color;
float r, g, b;

	x = (float)floor(xLeft);
	y = (float)floor(yTop);

	// Select font texture here

	color = Color();

	// Draw two tris to make a square;
	if (boxed != 2)
	{
		r = (color & 0xFF)/255.0F;
		g = ((color & 0xFF00) >> 8)/255.0F;
		b = ((color & 0xFF0000) >> 16)/255.0F;
	}
	else
	{
		// boxed == 2 means inverse text, so draw the square
		r = 0.0F;
		g = 0.0F;
		b = 0.0F;

		vert[0].x = x - 2.0F;
		vert[0].y = y;
		vert[1].x = vert[0].x;
		vert[1].y = vert[0].y + FontData[FontNum][32].pixelHeight;
		vert[2].x = vert[0].x + ScreenTextWidth(string) + 4.0F;
		vert[2].y = vert[1].y;
		vert[3].x = vert[2].x;
		vert[3].y = vert[0].y;
		context.RestoreState( STATE_SOLID );
		context.DrawPrimitive(MPR_PRM_TRIFAN, 0, 4, vert, sizeof(vert[0]));
	}

	context.RestoreState( STATE_TEXTURE_GOURAUD_TRANSPARENCY );
	context.SelectTexture( FontTexture[FontNum].TexHandle() );

	vert[0].r = r;
	vert[0].g = g;
	vert[0].b = b;
	vert[0].q = 1.0F;
	vert[0].a = 1.0F;
	vert[1].r = r;
	vert[1].g = g;
	vert[1].b = b;
	vert[1].q = 1.0F;
	vert[1].a = 1.0F;
	vert[2].r = r;
	vert[2].g = g;
	vert[2].b = b;
	vert[2].q = 1.0F;
	vert[2].a = 1.0F;
	vert[3].r = r;
	vert[3].g = g;
	vert[3].b = b;
	vert[3].q = 1.0F;
	vert[3].a = 1.0F;

	while (*string)
	{
		vert[0].x = x;
		vert[0].y = y;
		vert[0].u = FontData[FontNum][*string].left;
		vert[0].v = FontData[FontNum][*string].top;

		vert[1].x = vert[0].x;
		vert[1].y = vert[0].y + FontData[FontNum][*string].pixelHeight;
		vert[1].u = vert[0].u;
		vert[1].v = vert[0].v + FontData[FontNum][*string].height;

		vert[2].x = vert[0].x + FontData[FontNum][*string].width * 256;
		vert[2].y = vert[1].y;
		vert[2].u = vert[0].u + FontData[FontNum][*string].width;
		vert[2].v = vert[1].v;

		vert[3].x = vert[2].x;
		vert[3].y = vert[0].y;
		vert[3].u = vert[2].u;
		vert[3].v = vert[0].v;

		// Do a block clip
		if (
			(vert[2].x <= rightPixel) &&
			(vert[3].x <= rightPixel) &&
			(vert[0].x >= leftPixel) &&
			(vert[1].x >= leftPixel) &&
			(vert[1].y <= bottomPixel) &&
			(vert[2].y <= bottomPixel) &&
			(vert[0].y >= topPixel) &&
			(vert[3].y >= topPixel)
			)
		{
			context.DrawPrimitive(MPR_PRM_TRIFAN, MPR_VI_COLOR | MPR_VI_TEXTURE, 4, vert, sizeof(vert[0]));
		}

		x += FontData[FontNum][*string].pixelWidth;
		string++;
	}
	context.RestoreState( STATE_SOLID );

	// Go back and box the string if necessary
	if (boxed == 1)
	{
		float x1 = xLeft - 2.0f;
		float y1 = yTop  - 2.0f;
		float x2 = (float)(x + 1);
		float y2 = yTop + FontData[FontNum][32].pixelHeight;
		
		// Only draw the box if it is entirely on screen
		if ((x1 > leftPixel) && (x2 < rightPixel) && (y1 > topPixel) && (y2 < bottomPixel)) {
			Render2DLine (x1, y1, x2, y1);
			Render2DLine (x2, y1, x2, y2);
			Render2DLine (x2, y2, x1, y2);
			Render2DLine (x1, y2, x1, y1);
		}
	}

   // Left Arrow
   if (boxed == 0x4)
   {
      float x0 = xLeft - 5.0F;
      float y0 = yTop + 2.0F;
		float x1 = xLeft - 2.0f;
		float y1 = yTop  - 2.0f;
		float x2 = (float)(x + 1);
		float y2 = yTop + FontData[FontNum][32].pixelHeight;

      x1 = max (x1, leftPixel + 1.0F);
		
		// Only draw the box if it is entirely on screen
		if ((x1 > leftPixel) && (x2 < rightPixel) && (y1 > topPixel) && (y2 < bottomPixel)) {
         Render2DLine (x0, y0, x1, y1);
         Render2DLine (x0, y0, x1, y2);
			Render2DLine (x1, y1, x2, y1);
			Render2DLine (x2, y1, x2, y2);
			Render2DLine (x2, y2, x1, y2);
		}
   }

   // Right Arrow
   if (boxed == 0x8)
   {
      float x0 = float (x + 4);
      float y0 = yTop + 2.0F;
		float x1 = xLeft - 2.0f;
		float y1 = yTop  - 2.0f;
		float x2 = (float)(x + 1);
		float y2 = yTop + FontData[FontNum][32].pixelHeight;
		
		// Only draw the box if it is entirely on screen
      x1 = max (x1, leftPixel + 1.0F);
		if ((x1 > leftPixel) && (x2 < rightPixel) && (y1 > topPixel) && (y2 < bottomPixel)) {
         Render2DLine (x0, y0, x2, y1);
         Render2DLine (x0, y0, x2, y2);
			Render2DLine (x1, y1, x2, y1);
			Render2DLine (x1, y1, x1, y2);
			Render2DLine (x2, y2, x1, y2);
		}
   }

#endif

#else
#error "You must define a font drawing method"
#endif
}

// Global Texture mapped font stuff
void Load2DFontTextures ()
{
#ifdef USE_TEXTURE_FONT
	if (FontTexture[0].LoadImage( "art\\ckptart\\6x4font.gif", MPR_TI_CHROMAKEY | MPR_TI_PALETTE, FALSE ))
		FontTexture[0].CreateTexture("6x4font.gif");
	if (FontTexture[1].LoadImage( "art\\ckptart\\8x6font.gif", MPR_TI_CHROMAKEY | MPR_TI_PALETTE, FALSE ))
		FontTexture[1].CreateTexture("8x6font.gif");
	if (FontTexture[2].LoadImage( "art\\ckptart\\10x7font.gif", MPR_TI_CHROMAKEY | MPR_TI_PALETTE, FALSE ))
		FontTexture[2].CreateTexture("10x7font.gif");

	ReadFontMetrics(0, "art\\ckptart\\6x4font.rct");
	ReadFontMetrics(1, "art\\ckptart\\8x6font.rct");
	ReadFontMetrics(2, "art\\ckptart\\10x7font.rct");
	TotalFont = 3; // JPO new font.
	if (ReadFontMetrics(3, "art\\ckptart\\warn_font.rct") &&
	    FontTexture[3].LoadImage ("art\\ckptart\\warn_font.gif", MPR_TI_CHROMAKEY | MPR_TI_PALETTE, FALSE )) {
	    FontTexture[3].CreateTexture("warn_font.gif");
	    TotalFont = 4;
	}
#endif
}

void Release2DFontTextures ()
{
#ifdef USE_TEXTURE_FONT
	FontTexture[0].FreeAll();
	FontTexture[1].FreeAll();
	FontTexture[2].FreeAll();
	if (TotalFont > 3)
	    FontTexture[3].FreeAll();
#endif
}

#ifdef USE_TEXTURE_FONT

extern FILE *ResFOpen (char *, char *);

int ReadFontMetrics(int index, char*fileName) // JPO return status
{
	int
		file,
		size,
		idx, 
		top,
		left,
		width,
		height,
		lead,
		trail;

	char
		*str;

	static char
		buffer[16000];

	ShiAssert(index < NUM_FONT_RESOLUTIONS && index >= 0);
	ShiAssert(FALSE == IsBadStringPtr(fileName, _MAX_PATH));

	file = GR_OPEN (fileName, O_RDONLY);

	if (file >= 0)
	{
		size = GR_READ (file, buffer, 15999);

		buffer[size] = 0;

		str = buffer;

		while (str && *str)
		{
			int n = sscanf (str, "%d %d %d %d %d %d %d", &idx, &left, &top, &width, &height, &lead, &trail);
			ShiAssert(n == 7);

#if 1
			// OW: shift u,v by a half texel. if you dont do that and the card filters it fetches the wrong texels
			// because if you specify 1.0 you're saying that you want the far-right edge of this texel
			// and not the center of the texel like you thought
			float fOffset = (1.0f / 256.0F) / 2.0f;

			FontData[index][idx].top = top / 256.0F;
			FontData[index][idx].top += fOffset;

			FontData[index][idx].left = left / 256.0F;
			FontData[index][idx].left += fOffset;

			FontData[index][idx].width = width / 256.0F;
			FontData[index][idx].width -= fOffset;

			FontData[index][idx].height = height / 256.0F;
			FontData[index][idx].height -= fOffset;

			FontData[index][idx].pixelHeight = (float)height;
			FontData[index][idx].pixelWidth = (float)(width + lead);
#else
			FontData[index][idx].top = top / 256.0F;
			FontData[index][idx].left = left / 256.0F;
			FontData[index][idx].width = width / 256.0F;
			FontData[index][idx].height = height / 256.0F;
			FontData[index][idx].pixelHeight = (float)height;
			FontData[index][idx].pixelWidth = (float)(width + lead);
#endif
			str = strstr (str, "\n");

			while ((str) && ((*str == '\n') || (*str == '\r')))
			{
				str ++;
			}
		}

		GR_CLOSE (file);
		return 1;
	}
	return 0;
}
#endif
