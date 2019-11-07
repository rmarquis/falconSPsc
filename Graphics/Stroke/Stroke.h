
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

#ifndef __STROKE_H__
#define __STROKE_H__

class ContextMPR;

typedef struct {
  float x,y;
  float scale;
} StrokePen_t;

//extern StrokePen_t* StrokePen;
extern StrokePen_t  StrokePen;

#define StrokePos(X,Y) StrokePen.x = (X); StrokePen.y = (Y)
#define StrokeScale(X) StrokePen.scale = ((X)*(1.f/26819.f));

//extern StrokePen_t* StrokePenGet (void);
//extern void         StrokePenSet (StrokePos_t*);

extern void         StrokeChar   (ContextMPR* ,char);
extern void         StrokePrint  (ContextMPR* ,char*);
extern int          StrokeCharWidth (char);

#endif  /*  __STROKE_H__ */



