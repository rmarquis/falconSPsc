#include "stdafx.h"
#include "cpchev.h"
#include "cpmanager.h"
#include "Graphics\Include\renderow.h"
#include "otwdrive.h"
#include "Graphics\Include\tod.h"

CPChevron::CPChevron(ObjectInitStr* pobjectInitStr, ChevronInitStr* liftInitStr) : CPObject(pobjectInitStr)
{
	float startx;
	float starty;
	float offset = 0.05F;
	float r;
	float sin, cos;
	int	i, j;
	float zerox, zeroy;
	float x, y;

	mColor[0] = 0xFF0000FF;
	mColor[1] = CalculateNVGColor(mColor[0]);

	pan	= liftInitStr->pan;
	tilt	= -liftInitStr->tilt;

	if(pan < 0.0F) {
		sprintf(mString1, "PAN:  %d * left", abs(liftInitStr->panLabel));
	}
	else {
		sprintf(mString1, "PAN:  %d * right", abs(liftInitStr->panLabel));
	}
	sprintf(mString2, "TILT: %d * up", abs(liftInitStr->tiltLabel));

	r					= (float)sqrt(tilt * tilt + pan * pan);
	sin				= tilt / r;
	cos				= pan / r;
	mNumCheverons	= abs(((int)pan) / 45);

	if(pan > 0.0F) {
		startx = 0.80f;
		starty = -0.47f;


		for(i = 0; i < mNumCheverons; i++) {
			mChevron[i].x[0]	= 0.0F;
			mChevron[i].y[0]	= 0.0F;
			mChevron[i].x[1]	= mChevron[i].x[0] + offset;
			mChevron[i].y[1]	= mChevron[i].y[0] + offset;
			mChevron[i].x[2]	= mChevron[i].x[0] + offset;
			mChevron[i].y[2]	= mChevron[i].y[0] - offset;

			zerox = offset * i * cos;
			zeroy = offset * i * sin;

			for(j = 0; j < 3; j++) {
				x	= mChevron[i].x[j] * cos - mChevron[i].y[j] * sin + startx + zerox;
				y	= mChevron[i].x[j] * sin + mChevron[i].y[j] * cos + starty + zeroy;
			
				mChevron[i].x[j]	= x;
				mChevron[i].y[j]	= y;
			}
		}
	}
	else {
		sin = -sin;
		cos = -cos;

		startx = 0.80f + 3.0F * offset;
		starty = -0.47f;

		for(i = 0; i < mNumCheverons; i++) {
			mChevron[i].x[0]	= 0.0F;
			mChevron[i].y[0]	= 0.0F;
			mChevron[i].x[1]	= mChevron[i].x[0] - offset;
			mChevron[i].y[1]	= mChevron[i].y[0] + offset;
			mChevron[i].x[2]	= mChevron[i].x[0] - offset;
			mChevron[i].y[2]	= mChevron[i].y[0] - offset;

			zerox = offset * i * -cos;
			zeroy = offset * i * -sin;

			for(j = 0; j < 3; j++) {
				x	= mChevron[i].x[j] * cos - mChevron[i].y[j] * sin + startx + zerox;
				y	= mChevron[i].x[j] * sin + mChevron[i].y[j] * cos + starty + zeroy;
			
				mChevron[i].x[j]	= x;
				mChevron[i].y[j]	= y;
			}
		}
	}
}


CPChevron::~CPChevron()
{
}

void CPChevron::DisplayDraw(void)
{
	int i, j;
	int oldFont = VirtualDisplay::CurFont();

	OTWDriver.renderer->SetViewport( 0.75F, 1.0F, 1.0F, 0.87F );
	OTWDriver.renderer->SetBackground(0x00000000);
	OTWDriver.renderer->ClearFrame();

	OTWDriver.renderer->SetViewport(-1.0F, 1.0F, 1.0F, -1.0F);
	OTWDriver.renderer->SetColor(mColor[TheTimeOfDay.GetNVGmode() != 0]);

	for(i = 0; i < mNumCheverons; i++) {
		for(j = 1; j <= 2; j++) {
			OTWDriver.renderer->Line(mChevron[i].x[0], mChevron[i].y[0], mChevron[i].x[j], mChevron[i].y[j]);
		}
	}

	OTWDriver.renderer->SetColor(0xFF00FF00);
	VirtualDisplay::SetFont(mpCPManager->SABoxFont());
	OTWDriver.renderer->TextLeft( 0.78F, 0.97F, mString1);
	OTWDriver.renderer->TextLeft( 0.78F, 0.92F, mString2);
	VirtualDisplay::SetFont(oldFont);
}