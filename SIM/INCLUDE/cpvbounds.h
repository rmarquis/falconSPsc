#ifndef _CPVBOUNDS_H
#define _CPVBOUNDS_H

#include "windows.h"

#define BOUNDS_HUD		0
#define BOUNDS_RWR		1
#define BOUNDS_MFDLEFT	2
#define BOUNDS_MFDRIGHT	3	
#define BOUNDS_TOTAL		4

typedef struct {
			float			top;
			float			left;
			float			bottom;
			float			right;
			} ViewportBounds;

void ConvertRecttoVBounds(RECT*, ViewportBounds*, int, int, int);

#endif
