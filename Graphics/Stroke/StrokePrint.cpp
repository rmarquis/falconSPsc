
/********************************************************************/
/*  Copyright (C) 1998 MicroProse, Inc. All rights reserved         */
/*                                                                  */
/*  Programs, statements and coded instructions within this file    */
/*  contain unpublished and proprietary information of MicroProse,  */
/*  Inc. and are thus protected by the Federal and International    */
/*  laws.  They may not be copied, duplicated or disclosed to third */
/*  parties in any form, in whole or in part, without the prior     */
/*  written consent of MicroProse, Inc.                             */
/********************************************************************/

#include <stdio.h>
#include "mpr\mpr.h"
#include "Stroke.h"
#include "internal.h"
#include "f4stroke.h"
#include "strokeroman.h"
#include "context.h"

void dbgPrintf(  LPSTR dbgFormat, ...);

StrokePen_t StrokePen = {200., 200., 0.00024F};


void StrokeChar(ContextMPR* context, char c)
{
  UInt pathC, path;
  UInt vert, vertC;
  UInt count1, count2;
  UInt vertB;
  UInt curChar = toupper (c);

  MPRVtx_t	vtx[2];

  path  = StrokeGlyphRoman[curChar].start;
  pathC = StrokeGlyphRoman[curChar].count;
  vertB = StrokePathRoman[path].start;
  vertC = 0;

  for (count1=0; count1<pathC; count1++, path++) {
    vertC += StrokePathRoman[path].count;
    vert   = StrokePathRoman[path].start + vertC;
    
    for(count2=StrokePathRoman[path].start; (int)count2<StrokePathRoman[path].start +
          StrokePathRoman[path].count-1; count2++) {
      
      vtx[0].x = StrokePen.x + StrokeVertexRoman[count2  ].x * StrokePen.scale;
      vtx[0].y = StrokePen.y - (StrokeVertexRoman[count2  ].y - 25600) * StrokePen.scale;
      vtx[1].x = StrokePen.x + StrokeVertexRoman[count2+1].x * StrokePen.scale;
      vtx[1].y = StrokePen.y - (StrokeVertexRoman[count2+1].y - 25600) * StrokePen.scale;

	context->Primitive( MPR_PRM_LINES, 0, 2, sizeof(vtx[0]), (unsigned char*)&vtx );

    }
  }
  
#if 0
  StrokePen.x += StrokePen.dx * StrokeGlyphRoman[curChar].width;
  StrokePen.y += StrokePen.dy * StrokeGlyphRoman[curChar].width; 
#else
  StrokePen.x += StrokeCharWidth(c);
#endif

}


void StrokePrint(ContextMPR* context, char* str)
{

  while (*str) {
    StrokeChar(context, *str);
    //StrokePen.x += StrokePen.dx;
    //StrokePen.y += StrokePen.dy;
    str++;
  }
}

int StrokeCharWidth (char c)
{
int curChar = toupper(c);

	return int(StrokePen.scale * StrokeGlyphRoman[curChar].width + 0.5);
//	return (int)(StrokePen.scale * 1800 + 0.5);
}
