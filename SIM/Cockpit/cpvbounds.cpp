#include "stdafx.h"

#include "cpvbounds.h"


void ConvertRecttoVBounds(RECT *rect, ViewportBounds *vbounds, int width, int height, int scale) {

		float		halfWidth;
		float		halfHeight;

		halfWidth			= width * 0.5F;
		halfHeight			= height * 0.5F;

		vbounds->top		= -(rect->top * scale - halfHeight) / halfHeight;
		vbounds->left		= (rect->left * scale - halfWidth) / halfWidth;
		vbounds->bottom	= -(rect->bottom * scale - halfHeight) / halfHeight;
		vbounds->right		= (rect->right * scale - halfWidth) / halfWidth;
}