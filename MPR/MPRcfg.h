/* Copyright (C) 1998 MicroProse, Inc.  All rights reserved */

#ifndef __MPRCFG_H__
#define __MPRCFG_H__

//#define  MPR_PALETTE_MODULATION_WITH_FOG    // constant color modulation with constant fog.
//#define  MPR_DEPTH_BUFFER_ENABLED

// Causes gouraud color values to be correct
// on a per-pixel basis...slower and doesn't
// improve quality much.
#undef   MPR_GOURAUD_PIXEL_PERFECT

// Should enable mip-mapping...only partially implemented
#undef   MPR_MIP_MAP_ENABLE

// Enables the "do-everything" rasterizer
#undef   MPR_DEFAULT_RASTERIZER_ENABLED

#define  MPR_DITHER_ENABLE
#define  MPR_JITTER_ENABLE
#define  MPR_DIFFUSION_ENABLE

#define  MPR_PERSPECTIVE_CORRECT_ENABLED

// Bilinear and dithering clamp to edges
// instead of wrap (this is a bad thing...
// causes seaming...but if your art's fuck..)
#define  MPR_BILINEAR_CLAMPED

// Alpha modulated texture always dither if on
#define  MPR_ALPHA_MODULATE_DITHER_ALWAYS

// Linear ramp instead of real gamma.  Little
// CPU trade-off...just depends on how you want
// to perform screen-wide blending effects.
#define  MPR_GAMMA_FAKE       

// I really need to perform some error-analysis...
// this method is fucked
#define   MPR_PERSPECTIVE_CORRECT_Q 1.3F

/** NO NEED TO EDIT BELOW HERE -- DERIVED MACROS **/

#if defined(MPR_MIP_MAP_ENABLE)
#define MPR_MIP_MAP_LINE(X) X
#else
#define MPR_MIP_MAP_LINE(X)
#endif

#endif  // __MPRCFG_H__
