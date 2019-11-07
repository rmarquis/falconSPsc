/***************************************************************************\
    types.h
    Scott Randolph
    November 24, 1994

    Basic data types used by the Terrain Overflight application.
    This file is intended to be platform specific.
\***************************************************************************/
#ifndef _TYPES_H_
#define _TYPES_H_

// Convienient values to have arround
#include "shi\ConvFtoI.h"
#include "shi\ShiError.h"

#ifdef USE_SMART_HEAP
#include <stdlib.h>
#include "SmartHeap\Include\smrtheap.hpp"
#endif

#include "Constant.h"


#if 0	// I don't think these are used anymore....
// Useful basic data types
typedef signed char		INT8;
typedef unsigned char	UINT8;
typedef short			INT16;
typedef unsigned short	UINT16;
typedef long			INT32;
typedef unsigned long	UINT32;
typedef UINT8			BYTE;
typedef UINT16			WORD;
typedef UINT32			DWORD;
#endif


// Three by three rotation matrix
typedef struct Trotation {
	float	M11, M12, M13;
	float	M21, M22, M23;
	float	M31, M32, M33;
} Trotation;

// Three space point
typedef struct Tpoint {
	float x, y, z;
} Tpoint;

// RGB color
typedef struct Tcolor {
	float	r;
	float	g;
	float	b;
} Tcolor;


// Math constants
#ifndef PI
#define PI			3.14159265359f
#endif
#define TWO_PI		6.28318530718f
#define PI_OVER_2	1.570796326795f
#define PI_OVER_4	0.7853981633974f


#endif /* _TYPES_H_ */
