#include "stdhdr.h"
#include "geometry.h"
#include "debuggr.h"
#include "object.h"
#include "radarDoppler.h"
#include "simmover.h"
#include "Graphics\Include\display.h"
#include "Graphics\Include\gmComposit.h"
#include "otwdrive.h"
#include "campwp.h"
#include "simveh.h"
#include "cmpclass.h"
#include "fcc.h"
#include "aircrft.h"
#include "simdrive.h"	//MI
#include "fack.h"		//MI
#include "sms.h"		//MI
#include "icp.h"		//MI
#include "cpmanager.h"	//MI
#include "hud.h"	//MI
#include "team.h"	//MI
#include "fault.h"	//MI
#include "campbase.h" // 2002-02-25 S.G.

#define SCH_ANG_INC  22.5F      /* velocity pointer angle increment     */
#define SCH_FACT   1600.0F      /* velocity pointer length is the ratio */
                                /* of vt/SCH_FACT                       */
#define DD_LENGTH   0.2F        /* donkey dick length                   */
#define NCTR_BAR_WIDTH        0.2F
#define SECOND_LINE_Y         0.8F
#define TICK_POS              0.75F

static const float DisplayAreaViewTop    =  0.75F;
static const float DisplayAreaViewBottom = -0.68F;
static float DisplayAreaViewLeft   = -0.80F;
static const float DisplayAreaViewRight  =  0.72F;

static float disDeg;
static float steerpoint[12][2] = {
   -0.05F, -0.03F,
   -0.05F, -0.01F,
   -0.03F, -0.01F,
	-0.03F,  0.03F,
	-0.01F,  0.03F,
	-0.01F,  0.05F,
	 0.01F,  0.05F,
	 0.01F,  0.03F,
	 0.03F,  0.03F,
	 0.03F, -0.01F,
	 0.05F, -0.01F,
	 0.05F, -0.03F};

static float elReacqMark[3][2];
static float elMark[4][2] = {0.0F, 0.02F, 0.0F, -0.02F, 0.0F, 0.0F, 0.04F, 0.0F};
static float cursor[12][2];
static int fpass = TRUE;

extern bool g_bEPAFRadarCues, g_bRadarJamChevrons;
extern float g_fRadarScale;
//MI
extern bool g_bMLU;
extern bool g_bIFF;
extern bool g_bINS;
void DrawBullseyeData (VirtualDisplay* display, float cursorX, float cursorY);
//MI
void DrawCursorBullseyeData(VirtualDisplay* display, float cursorX, float cursorY);
void DrawBullseyeCircle(VirtualDisplay* display, float cursorX, float cursorY);

void RadarDopplerClass::Display (VirtualDisplay* newDisplay)
{
    float cX, cY;
    float vpLeft, vpTop, vpRight, vpBottom;
    
    GetCursorPosition (&cX, &cY);
    display = newDisplay;
    if (fpass)
    {
	fpass = FALSE;
	
	/*----------------------------------------------------------*/
	/* Set the scale factor to convert degrees to display scale */
	/*----------------------------------------------------------*/
	disDeg = AZL / MAX_ANT_EL;
	
	elReacqMark[0][0] =  0.03F;
	elReacqMark[0][1] =  0.03F;
	elReacqMark[1][0] =  0.0F;
	elReacqMark[1][1] =  0.0F;
	elReacqMark[2][0] =  0.03F;
	elReacqMark[2][1] = -0.03F;
	
	cursor[0][0] = -0.04F;
	cursor[0][1] =  0.06F;
	cursor[1][0] =  0.04F;
	cursor[1][1] =  0.06F;
	cursor[2][0] =  0.02F;
	cursor[2][1] =  0.02F;
	cursor[3][0] =  0.06F;
	cursor[3][1] =  0.04F;
	cursor[4][0] =  0.06F;
	cursor[4][1] = -0.04F;
	cursor[5][0] =  0.02F;
	cursor[5][1] = -0.02F;
	cursor[6][0] =  0.04F;
	cursor[6][1] = -0.06F;
	cursor[7][0] = -0.04F;
	cursor[7][1] = -0.06F;
	cursor[8][0] = -0.02F;
	cursor[8][1] = -0.02F;
	cursor[9][0] = -0.06F;
	cursor[9][1] = -0.04F;
	cursor[10][0] = -0.06F;
	cursor[10][1] =  0.04F;
	cursor[11][0] = -0.02F;
	cursor[11][1] =  0.02F;

    }
    
	//MI this should be there in realisticAvionics....
	//MI 08/29/01 now done elsewhere
    //if (!g_bRealisticAvionics) {
	/*if (g_bRealisticAvionics) {
	FireControlComputer* pFCC = ((SimVehicleClass*)platform)->GetFCC();//me123 addet for ccip/DTOSS ground ranging check
	if (pFCC->GetSubMode() == FireControlComputer::CCIP || pFCC->GetSubMode() == FireControlComputer::DTOSS)
	{
		//MI make it a bit more realistic
		//LabelButton(0, "AGR");
		/*display->TextCenter (0.0F, 0.0F, "A/G RANGING");
	    display = NULL;
	    return;
	}
    }*/
    int tmpColor = display->Color();
    float x18, y18;
    GetButtonPos(18, &x18, &y18);

    DisplayAreaViewLeft = x18 + display->TextWidth("40");
    
    /*---------------------*/
    /* common to a/a modes */
    /*---------------------*/
    if (mode != GM && mode != GMT && mode != SEA)
    {
	DrawScanMarkers();
	DrawAzElTicks();
    }
    
    display->GetViewport (&vpLeft, &vpTop, &vpRight, &vpBottom);
    
    /*----------------------*/
    /* Mode dependent stuff */
    /*----------------------*/
    switch (mode)
    {
    case OFF: // JPO - new modes..
    case STBY:
	STBYDisplay();
	break;
    case TWS:
	DrawRangeTicks();
	if (IsAADclt(Arrows) == FALSE) DrawRangeArrows();
	if (IsAADclt(Rng) == FALSE) DrawRange();
	DrawBars();
	DrawWaterline();
	display->SetColor(GetMfdColor(MFD_BULLSEYE)); // JPO
	//MI we only get the bullseye readout in the corner if we selected it so
	//the info on the cursor get's drawn nontheless
	if(!g_bRealisticAvionics)
		DrawBullseyeData(display, cX, cY);
	else
	{
		if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp &&
			OTWDriver.pCockpitManager->mpIcp->ShowBullseyeInfo)
			DrawBullseyeCircle(display, cX, cY);
		else
			DrawReference(display);

		DrawCursorBullseyeData(display, cX, cY);
	}
	display->SetColor(tmpColor);
	TWSDisplay();
	DrawACQCursor();
	display->SetColor(GetMfdColor(MFD_LINES)); // JPO
	DrawSteerpoint();
	display->SetColor(tmpColor);
	DrawAzLimitMarkers();
	//MI
	if(g_bRealisticAvionics && DrawRCR)
		DrawRCRCount();
	break;
	
    case SAM:
	DrawRangeTicks();
	if (IsAADclt(Arrows) == FALSE) DrawRangeArrows();
	if (IsAADclt(Rng) == FALSE) DrawRange();
	DrawBars();
	DrawWaterline();
	display->SetColor(GetMfdColor(MFD_BULLSEYE)); // JPO
	//MI we only get the bullseye readout in the corner if we selected it so
	//the info on the cursor get's drawn nontheless
	if(!g_bRealisticAvionics)
		DrawBullseyeData(display, cX, cY);
	else
	{
		if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp &&
			OTWDriver.pCockpitManager->mpIcp->ShowBullseyeInfo)
			DrawBullseyeCircle(display, cX, cY);
		else
			DrawReference(display);

		DrawCursorBullseyeData(display, cX, cY);
	}
	display->SetColor(tmpColor);
	SAMDisplay();
	DrawACQCursor();
	display->SetColor(GetMfdColor(MFD_LINES)); // JPO
	DrawSteerpoint();
	display->SetColor(tmpColor);
	//MI
	if(g_bRealisticAvionics && DrawRCR)
		DrawRCRCount();
	break;
	
    case RWS:
    case LRS:
	DrawRangeTicks();
	if (IsAADclt(Arrows) == FALSE) DrawRangeArrows();
	if (IsAADclt(Rng) == FALSE) DrawRange();
	DrawBars();
	DrawWaterline();
	display->SetColor(GetMfdColor(MFD_BULLSEYE)); // JPO
	//MI we only get the bullseye readout in the corner if we selected it so
	//the info on the cursor get's drawn nontheless
	if(!g_bRealisticAvionics)
		DrawBullseyeData(display, cX, cY);
	else
	{
		if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp &&
			OTWDriver.pCockpitManager->mpIcp->ShowBullseyeInfo)
			DrawBullseyeCircle(display, cX, cY);
		else
			DrawReference(display);

		DrawCursorBullseyeData(display, cX, cY);
	}
	display->SetColor(tmpColor);
	RWSDisplay();
	DrawACQCursor();
	display->SetColor(GetMfdColor(MFD_LINES)); // JPO
	DrawSteerpoint();
	display->SetColor(tmpColor);
	DrawAzLimitMarkers();
	break;
	
    case STT:
	DrawRangeTicks();
	if (IsAADclt(Rng) == FALSE) DrawRange();
	DrawBars();
	DrawWaterline();
	display->SetColor(GetMfdColor(MFD_BULLSEYE)); // JPO
	//MI we only get the bullseye readout in the corner if we selected it so
	//the info on the cursor get's drawn nontheless
	if(!g_bRealisticAvionics)
		DrawBullseyeData(display, cX, cY);
	else
	{
		if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp &&
			OTWDriver.pCockpitManager->mpIcp->ShowBullseyeInfo)
			DrawBullseyeCircle(display, cX, cY);
		else
			DrawReference(display);

		DrawCursorBullseyeData(display, cX, cY);
	}
	display->SetColor(tmpColor);
	STTDisplay();
	display->SetColor(GetMfdColor(MFD_LINES)); // JPO
	DrawSteerpoint();
	display->SetColor(tmpColor);
	//MI
	if(g_bRealisticAvionics && DrawRCR)
		DrawRCRCount();
	break;
	
    case ACM_SLEW:
    case ACM_30x20:
    case ACM_10x60:
    case ACM_BORE:
	DrawRangeTicks();
	if (IsAADclt(Rng) == FALSE) DrawRange();
	DrawWaterline();
	display->SetColor(GetMfdColor(MFD_BULLSEYE)); // JPO
	//MI we only get the bullseye readout in the corner if we selected it so
	//the info on the cursor get's drawn nontheless
	if(!g_bRealisticAvionics)
		DrawBullseyeData(display, cX, cY);
	else
	{
		if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp &&
			OTWDriver.pCockpitManager->mpIcp->ShowBullseyeInfo)
			DrawBullseyeCircle(display, cX, cY);
		else
			DrawReference(display);

		DrawCursorBullseyeData(display, cX, cY);
	}
	display->SetColor(tmpColor);
	ACMDisplay();
	display->SetColor(GetMfdColor(MFD_LINES)); // JPO
	DrawSteerpoint();
	display->SetColor(tmpColor);
	break;
	
    case VS:
	DrawRangeTicks();
	if (IsAADclt(Arrows) == FALSE) DrawRangeArrows();
	if (IsAADclt(Rng) == FALSE) DrawRange();
	DrawBars();
	DrawWaterline();
	display->SetColor(GetMfdColor(MFD_BULLSEYE)); // JPO
	//MI we only get the bullseye readout in the corner if we selected it so
	//the info on the cursor get's drawn nontheless
	if(!g_bRealisticAvionics)
		DrawBullseyeData(display, cX, cY);
	else
	{
		if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp &&
			OTWDriver.pCockpitManager->mpIcp->ShowBullseyeInfo)
			DrawBullseyeCircle(display, cX, cY);
		else
			DrawReference(display);

		DrawCursorBullseyeData(display, cX, cY);
	}
	display->SetColor(tmpColor);
	VSDisplay();
	DrawACQCursor();
	display->SetColor(GetMfdColor(MFD_LINES)); // JPO
	DrawSteerpoint();
	display->SetColor(tmpColor);
	break;
	//MI changed
    case GM:
    case GMT:
    case SEA:
		if(g_bRealisticAvionics)
		{
			FireControlComputer* pFCC = ((SimVehicleClass*)platform)->GetFCC();//me123 addet for ccip/DTOSS ground ranging check
			if(pFCC->GetSubMode() == FireControlComputer::CCIP || pFCC->GetSubMode() == FireControlComputer::DTOSS ||
				pFCC->GetSubMode() == FireControlComputer::STRAF || pFCC->GetSubMode() == FireControlComputer::RCKT)
			{
				if(mode != AGR)
					mode = AGR;
				return;
			}
			else
			{
				GMDisplay();
				DrawWaterline();
				display->SetColor(GetMfdColor(MFD_BULLSEYE)); // JPO
				//MI we only get the bullseye readout in the corner if we selected it so
				//the info on the cursor get's drawn nontheless
				if(!g_bRealisticAvionics)
					DrawBullseyeData(display, cX, cY);
				else
				{
					if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp &&
						OTWDriver.pCockpitManager->mpIcp->ShowBullseyeInfo)
						DrawBullseyeCircle(display, cX, cY);
					else
						DrawReference(display);
					//MI No, not in Ground Radar mode
					//DrawCursorBullseyeData(display, cX, cY);
				}
				display->SetColor(tmpColor);
			}
		}
		else
		{
			GMDisplay();
			DrawWaterline();
			display->SetColor(GetMfdColor(MFD_BULLSEYE)); // JPO
			//MI we only get the bullseye readout in the corner if we selected it so
			//the info on the cursor get's drawn nontheless
			if(!g_bRealisticAvionics)
				DrawBullseyeData(display, cX, cY);
			else
			{
				if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp &&
					OTWDriver.pCockpitManager->mpIcp->ShowBullseyeInfo)
					DrawBullseyeCircle(display, cX, cY);
				else
					DrawReference(display);

				DrawCursorBullseyeData(display, cX, cY);
			}
			display->SetColor(tmpColor);
		}
	break;

	//MI
	case AGR:
		AGRangingDisplay();
	break;	
    default:
	DrawTargets();
	break;
   }
   
   display->SetViewport (vpLeft, vpTop, vpRight, vpBottom);
   // SOI/Radiate status
   if (!IsSOI())
   {
       display->SetColor(GetMfdColor(MFD_TEXT));
       display->TextCenter(0.0F, 0.4F, "NOT SOI");
   }
   else {
       display->SetColor(GetMfdColor(MFD_GREEN));
       DrawBorder(); // JPO SOI
   }
   
   if (!isEmitting && mode != OFF) // JPO 
   {
	   //MI not here in real
	   if(!g_bRealisticAvionics)
	   {
		   display->SetColor(GetMfdColor(MFD_TEXT));
		   display->TextCenter (0.0F, 0.0F, "NO RAD");
	   }
   }
   
   display = NULL;
}


// JPO - do stdby/off display
void RadarDopplerClass::STBYDisplay(void) 
{
	float cX, cY;
    display->SetColor(GetMfdColor(MFD_LABELS));
    if (g_bRealisticAvionics) 
	{
		LabelButton(0, mode == OFF ? "OFF" : "STBY");
		if (mode != OFF)
		{
			//LabelButton(3, "OVRD", NULL, !isEmitting);
			LabelButton(3, "OVRD");
			LabelButton(4, "CNTL", NULL, IsSet(CtlMode));
		}
		if (IsSet(MenuMode|CtlMode)) 
		{
			MENUDisplay();
			return;
		}
		LabelButton(5, "BARO");
		LabelButton(8, "CZ");
		if (mode != OFF) 
		{
			LabelButton(7, "SP");
			LabelButton(9, "STP");
		}
		DrawRange();
		DrawRangeArrows();
		DrawBars();
		GetCursorPosition (&cX, &cY);
		if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp &&
			OTWDriver.pCockpitManager->mpIcp->ShowBullseyeInfo)
			DrawBullseyeCircle(display, cX, cY);
		else
			DrawReference(display);

    }
	else 
	{
		LabelButton (5,  "GM ");
		LabelButton (6,  "GMT");
		LabelButton (7,  "SEA");
		LabelButton (16, "ACM");
		LabelButton (17, "VS ");
		LabelButton (18, "RWS");
		LabelButton (19, "TWS");
    }
    AABottomRow ();
}


void RadarDopplerClass::DrawTargets (void)
{
float az;
SimObjectType* theTarget = platform->targetList;

   while (theTarget)
	{
		if (theTarget->localData->sensorState[Radar] == SensorTrack)
		{
			az = TargetAz (platform, theTarget->BaseData()->XPos(),
            theTarget->BaseData()->YPos());

			display->AdjustOriginInViewport (az * disDeg,
				-1.0F + 2.0F * theTarget->localData->range / tdisplayRange);
			display->Line (-0.025F, -0.025F, -0.025F,  0.025F);
			display->Line ( 0.025F, -0.025F,  0.025F,  0.025F);
			display->Line (-0.025F, -0.025F,  0.025F, -0.025F);
			display->Line ( 0.025F,  0.025F, -0.025F,  0.025F);
			display->CenterOriginInViewport();
		}

      theTarget = theTarget->next;
	}
}
//MI
int RadarDopplerClass::GetInterogate(SimObjectType *rdrObj, SimObjectType *lockedTarget)
{
	int retval = -1;
	return retval;
}

void RadarDopplerClass::DrawAzElTicks(void)
{
int i;
float posStep;
float curPos;

    if (IsAADclt(AzBar)) return;
   /*------------------------------------------*/
   /* elevation ticks -30 deg to +30 by 10 deg */
   /*------------------------------------------*/
	display->SetColor(GetMfdColor(MFD_LABELS));
	//MI
	if(!g_bRealisticAvionics)
	{
		posStep = (DisplayAreaViewTop - DisplayAreaViewBottom) / 6.0F;
		curPos = DisplayAreaViewBottom;
		for (i=0; i<7; i++)
		{
			if (1 != i)
				display->Line (DisplayAreaViewLeft - 0.01F, curPos,
				DisplayAreaViewLeft - 0.04F, curPos);
			curPos += posStep;
		}
	}
	else
	{
		posStep = (DisplayAreaViewTop - DisplayAreaViewBottom) / 12.0F;
		curPos = 0;
		display->Line(DisplayAreaViewLeft + 0.05F, curPos, DisplayAreaViewLeft - 0.04F, curPos);
		for (i = 0; i < 4; i++)
		{
			display->Line(DisplayAreaViewLeft + 0.01F, curPos,
				DisplayAreaViewLeft - 0.04F, curPos);
			display->Line(DisplayAreaViewLeft + 0.01F, -curPos,
				DisplayAreaViewLeft - 0.04F, -curPos);
			curPos += posStep;
		}
	}

   /*----------------------------------------*/
   /* Azimuth ticks -30 deg to +30 by 10 deg */
   /*----------------------------------------*/
	if(!g_bRealisticAvionics)	//MI this is not correct
	{
		posStep = (DisplayAreaViewRight - DisplayAreaViewLeft) / 6.0F;
		curPos = DisplayAreaViewLeft;
		for (i=0; i<7; i++)
		{ 
			display->Line (curPos, DisplayAreaViewBottom - 0.01F, 
				curPos, DisplayAreaViewBottom - 0.04F);
			curPos += posStep;	
		}
	}
	else
	{
		//our scope has 120°, not only 60°
		posStep = (DisplayAreaViewRight - DisplayAreaViewLeft) / 12.0F;
		curPos = 0;
		display->Line(curPos, DisplayAreaViewBottom - 0.15F,
				curPos, DisplayAreaViewBottom - 0.06F);
		for(i = 0; i < 4; i++)
		{
			display->Line(curPos, DisplayAreaViewBottom - 0.15F,
				curPos, DisplayAreaViewBottom - 0.10F);
			display->Line(-curPos, DisplayAreaViewBottom - 0.15F,
				-curPos, DisplayAreaViewBottom - 0.10F);
			curPos += posStep;
		}
	}
	
}

void RadarDopplerClass::DrawBars(void)
{
    char str[32];
    if (IsSet(MenuMode|CtlMode)) return; // JPO special modes
    if (IsAADclt(AzBar)) return;
    display->SetColor(GetMfdColor(MFD_LABELS));
   /*----------------*/
   /* number of bars */
   /*----------------*/
   sprintf(str,"%d",(bars > 0 ? bars : -bars));
	ShiAssert (strlen(str) < sizeof(str));
   LabelButton (16, "B", str);

   /*--------------*/
   /* Azimuth Scan */
   /*--------------*/
   sprintf(str,"%.0f", displayAzScan * 0.1F * RTD);
   ShiAssert (strlen(str) < sizeof(str));
   LabelButton (17, "A", str);
}

void RadarDopplerClass::DrawWaterline(void)
{
	float yPos, theta;

	static const float	InsideEdge	= 0.08f;
	static const float	OutsideEdge	= 0.40f;
	static const float	Height		= 0.04f;

    display->SetColor(GetMfdColor(MFD_LABELS));
	theta  = platform->Pitch();
	if(theta > 45.0F * DTR)
		theta = 45.0F * DTR;
	else if(theta < -45.0F * DTR)
		theta = -45.0F * DTR;

	yPos = -disDeg * theta;							  

	display->AdjustOriginInViewport (0.0F, yPos);
	display->AdjustRotationAboutOrigin (-platform->Roll());

	display->Line(	OutsideEdge,	-Height,	OutsideEdge,	0.0f);
	display->Line(	OutsideEdge,	0.0f,		InsideEdge,		0.0f);
	display->Line(	-OutsideEdge,	-Height,	-OutsideEdge,	0.0f);
	display->Line(	-OutsideEdge,	0.0f,		-InsideEdge,	0.0f);

	display->ZeroRotationAboutOrigin();
	display->CenterOriginInViewport();
}

void RadarDopplerClass::DrawScanMarkers(void)
{
float yPos;
float curPos;
    if (IsAADclt(AzBar)) return;
    display->SetColor(GetMfdColor (MFD_ANTENNA_AZEL));
   /*----------------*/
   /* Az Scan Marker */
   /*----------------*/
   curPos = (DisplayAreaViewRight - DisplayAreaViewLeft) / (2.0F * MAX_ANT_EL) * (beamAz + seekerAzCenter);
   curPos += (DisplayAreaViewRight + DisplayAreaViewLeft) * 0.5F;
   //MI this is the other way around in reality
   if(!g_bRealisticAvionics)
   {
	   display->Line (curPos, DisplayAreaViewBottom - 0.04F, curPos, DisplayAreaViewBottom - 0.07F);
	   display->Line (curPos - 0.015F, DisplayAreaViewBottom - 0.07F, curPos + 0.015F, DisplayAreaViewBottom - 0.07F);
   }
   else
   {
	   const static float width = 0.02F;
	   //vertical line
	   display->Tri(curPos - width/2, DisplayAreaViewBottom - 0.15F, curPos + width/2, DisplayAreaViewBottom - 0.15F, curPos + width/2, DisplayAreaViewBottom - 0.22F);
	   display->Tri(curPos + width/2, DisplayAreaViewBottom - 0.22F, curPos - width/2, DisplayAreaViewBottom - 0.22F, curPos - width/2, DisplayAreaViewBottom - 0.15F);
	   //"T" Line
	   display->Tri(curPos - width*2, DisplayAreaViewBottom - 0.15F + width/2, curPos + width*2, DisplayAreaViewBottom - 0.15F + width/2, curPos + width*2, DisplayAreaViewBottom - 0.15F - width/2);
	   display->Tri(curPos + width*2, DisplayAreaViewBottom - 0.15F - width/2, curPos - width*2, DisplayAreaViewBottom - 0.15F - width/2, curPos - width*2, DisplayAreaViewBottom - 0.15F + width/2);
   }

   /*----------------*/
   /* El Scan Marker */
   /*----------------*/
   //MI this is the other way around in reality
   curPos = (DisplayAreaViewTop - DisplayAreaViewBottom) / (2.0F * MAX_ANT_EL) * (beamEl + seekerElCenter);
   curPos += (DisplayAreaViewTop + DisplayAreaViewBottom) * 0.5F;
   if(!g_bRealisticAvionics)
   {
	   display->Line (DisplayAreaViewLeft - 0.04F, curPos, DisplayAreaViewLeft - 0.07F, curPos);
	   display->Line (DisplayAreaViewLeft - 0.07F, curPos + 0.015F, DisplayAreaViewLeft - 0.07F, curPos - 0.015F);
   }
   else
   {
	   const static float width = 0.02F;
	   //horizontal line
	   display->Tri(DisplayAreaViewLeft - 0.04F, curPos + width/2, DisplayAreaViewLeft - 0.11F, curPos + width/2, DisplayAreaViewLeft - 0.04F, curPos - width/2);
	   display->Tri(DisplayAreaViewLeft - 0.04F, curPos - width/2, DisplayAreaViewLeft - 0.11F, curPos - width/2, DisplayAreaViewLeft - 0.11F, curPos + width/2);
	   //vertical line
	   display->Tri(DisplayAreaViewLeft - 0.04F + width/2, curPos + width*2, DisplayAreaViewLeft - 0.04F - width/2, curPos + width*2, DisplayAreaViewLeft - 0.04F + width/2, curPos - width*2);
	   display->Tri(DisplayAreaViewLeft - 0.04F + width/2, curPos - width*2, DisplayAreaViewLeft - 0.04F - width/2, curPos - width*2, DisplayAreaViewLeft - 0.04F - width/2, curPos + width*2);
   }
   /*--------------------------------------------------*/
   /* elevation reaquisition marker, remains on screen */
   /* for 10 sec after loss of target Track            */
   /*--------------------------------------------------*/
   if (reacqFlag)
   {
       display->SetColor(GetMfdColor (MFD_FCR_REAQ_IND));
      yPos = (DisplayAreaViewTop - DisplayAreaViewBottom) / (2.0F * MAX_ANT_EL) * reacqEl;
      yPos += (DisplayAreaViewTop + DisplayAreaViewBottom) * 0.5F;
   	display->AdjustOriginInViewport(DisplayAreaViewLeft, yPos);
		display->Line (elReacqMark[0][0], elReacqMark[0][1],
			elReacqMark[1][0], elReacqMark[1][1]);
		display->Line (elReacqMark[2][0], elReacqMark[2][1],
			elReacqMark[1][0], elReacqMark[1][1]);
		display->CenterOriginInViewport();
   }
}

void RadarDopplerClass::DrawRangeTicks(void)
{
	static const float Hstart	= 0.75f;
	static const float Hstop	= 0.85f;
    
	display->SetColor(GetMfdColor(MFD_ANTENNA_AZEL_SCALE));
	display->Line (	Hstart,	 0.0f,	Hstop,  0.0f );
	display->Line (	Hstart,	 0.5f,	Hstop,  0.5f );
	display->Line (	Hstart,	-0.5f,	Hstop, -0.5f );
}

void RadarDopplerClass::DrawRange(void)
{
    char str[8];
    if (IsSet(MenuMode|CtlMode)) return;
    
    display->SetColor(GetMfdColor(MFD_LABELS));
    float x18, y18;
    float x19, y19;
    GetButtonPos(18, &x18, &y18);
    GetButtonPos(19, &x19, &y19);
    sprintf(str,"%.0f",displayRange);
    ShiAssert (strlen(str) < sizeof(str));
    display->TextLeftVertical(x18, y18 + (y19-y18)/2, str);
}

void RadarDopplerClass::DrawRangeArrows(void)
{
    static const float arrowH = 0.0375f;
    static const float arrowW = 0.0433f;
    if (IsSet(MenuMode|CtlMode)) return; // JPO special modes
    
    float x18, y18;
    float x19, y19;
    GetButtonPos(18, &x18, &y18); // work out button positions.
    GetButtonPos(19, &x19, &y19);
    /*----------*/
    /* up arrow */
    /*----------*/
    display->SetColor(GetMfdColor(MFD_LABELS));
    display->AdjustOriginInViewport(x19 + arrowW, y19 + arrowH/2);
	//MI 
	if(g_bRealisticAvionics && (mode == GM || mode == GMT || mode == SEA))
	{
		if(mode == GMT || mode == SEA)
		{
			if(curRangeIdx < NUM_RANGES - 3)
				display->Tri (0.0F, arrowH, arrowW, -arrowH, -arrowW, -arrowH);
		}
		else if(mode == GM)
		{
			if(curRangeIdx < NUM_RANGES - 2)
				display->Tri (0.0F, arrowH, arrowW, -arrowH, -arrowW, -arrowH);
		}
	}
    else if (curRangeIdx < NUM_RANGES - 1) 
	{
		display->Tri (0.0F, arrowH, arrowW, -arrowH, -arrowW, -arrowH);
    }
    /*------------*/
    /* down arrow */
    /*------------*/
    if (curRangeIdx > 0) {
	display->CenterOriginInViewport();
	display->AdjustOriginInViewport( x18 + arrowW, y18 - arrowH/2);
	display->Tri (0.0F, -arrowH, arrowW, arrowH, -arrowW, arrowH);
    }
    display->CenterOriginInViewport();
}
//MI
void RadarDopplerClass::DrawIFFStatus(void)
{
}
int RadarDopplerClass::GetCurScanMode(int i)
{
	return 0;
}
void RadarDopplerClass::RWSDisplay(void)
{
int   i;
float xPos, yPos, alt;
char  str[12];
float ang, vt;
SimObjectType* rdrObj = platform->targetList;
SimObjectLocalData* rdrData;
int tmpColor = display->Color();
FireControlComputer* pFCC = ((SimVehicleClass*)platform)->GetFCC();
//MI
Pointer = 0;

	//MI added EXP FOV
	if (fovStepCmd)
    {
		fovStepCmd = 0;
		ToggleFlag(EXP);
    }
	//MI to disable it
	if(IsSet(EXP)&& !g_bMLU)
		ToggleFlag(EXP);

    display->SetColor(GetMfdColor(MFD_LABELS));
    // OSS Button Labels
    if (IsAADclt(MajorMode) == FALSE) 
	LabelButton (0, "CRM");
    if (IsAADclt(SubMode) == FALSE)
	{
		//MI
		if(lockedTarget)
			LabelButton (1, prevMode == LRS ? "LRS" : "RWS");
		else
			LabelButton (1, mode == LRS ? "LRS" : "RWS");
	}
    //MI added EXP mode to RWS
	if (IsAADclt(Fov) == FALSE)
       LabelButton (2, IsSet(EXP) ? "EXP" : "NRM", NULL, IsSet(EXP) ? (vuxRealTime & 0x080) : 0);
    if (IsAADclt(Ovrd) == FALSE) LabelButton (3, "OVRD", NULL, isEmitting == 0);
    if (IsAADclt(Cntl) == FALSE) LabelButton (4, "CTNL", NULL, IsSet(CtlMode));
    if (IsSet(MenuMode|CtlMode)) {
	MENUDisplay ();
    }
    else {
	AABottomRow ();
    }

	// Set the viewport
   display->SetViewportRelative (DisplayAreaViewLeft, DisplayAreaViewTop, DisplayAreaViewRight, DisplayAreaViewBottom);

	//MI
	float tgtx, tgty;
	float brange;
	if (IsSet(EXP)) 
	{
		if (lockedTargetData) 
		{
			// work out where on the display the target is
			TargetToXY(lockedTargetData, 0, tdisplayRange, &tgtx, &tgty);
		}
		else 
		{
			tgtx = cursorX;
			tgty = cursorY;
		}
		// draw the box
		brange = 2.0f * 2 * NM_TO_FT / tdisplayRange;
		display->Line(tgtx - brange, tgty - brange, tgtx + brange, tgty - brange);
		display->Line(tgtx + brange, tgty - brange, tgtx + brange, tgty + brange);
		display->Line(tgtx + brange, tgty + brange, tgtx - brange, tgty + brange);
		display->Line(tgtx - brange, tgty + brange, tgtx - brange, tgty - brange);
	}
	else 
	{
		brange = 0;
		tgtx = 0; tgty = 0;
	}

   /*-----------------*/
   /* for all objects */
   /*-----------------*/
   while (rdrObj)
   {
      rdrData = rdrObj->localData;
	  
	  if (F4IsBadCodePtr((FARPROC) rdrObj->BaseData())) // JB 010317 CTD
	  {
		  rdrObj = rdrObj->next;
		  continue;
	  }

      /*-------------------------*/
      /* show target and history */
      /*-------------------------*/
      for (i=histno-1; i>=0; i--) // JPO - history is now dynamic
      {
         if (rdrData->rdrY[i] > 1.0)
         {
			 TargetToXY(rdrData, i, tdisplayRange, &xPos, &yPos);
			 	//Mi
				Pointer++;
				if(Pointer > 19)
					Pointer = 0;
			 /*----------------------------*/
			 /* keep objects on the screen */
			 /*----------------------------*/
			 if (fabs(xPos) < AZL && fabs(yPos) < AZL)
			 {
				 if(IsSet(EXP))
				 {
					 float dx = xPos - tgtx;
					 float dy = yPos - tgty;
					 dx *= 4; // zoom factor
					 dy *= 4;
					 xPos = tgtx + dx;
					 yPos = tgty + dy;
					 if(fabs(xPos) > AZL || fabs(yPos) > AZL) 
					 {
						continue;
					 }
				 }
				 display->AdjustOriginInViewport (xPos, yPos);
				 if (rdrData->rdrSy[i] >= Track)
				 {
					 // This _should_ be right -- based on 2D angle between target velocity
					 // and our line of sight to target  SCR 10/27/97
					 ang = rdrObj->BaseData()->Yaw() - rdrData->rdrX[0] - platform->Yaw();
					 if (ang >= 0.0)
						 ang = SCH_ANG_INC*(float)floor(ang/(SCH_ANG_INC * DTR));
					 else
						 ang = SCH_ANG_INC*(float)ceil(ang/(SCH_ANG_INC * DTR));
					 display->AdjustRotationAboutOrigin (ang * DTR);
				 }
				 vt = rdrObj->BaseData()->Vt();
				 //MI we get all the AIM120 goodies in RWS too
				 if(g_bRealisticAvionics)
				 {
					 if (rdrObj == lockedTarget && 
						 pFCC->lastMissileImpactTime > 0.0F) 
					 { // Aim Target
						 if (pFCC->lastMissileImpactTime > pFCC->lastmissileActiveTime)
							 DrawSymbol(AimRel, vt/SCH_FACT, i);
						 else
							 DrawSymbol(AimFlash, vt/SCH_FACT, i);
					 }
					 else
						 DrawSymbol(rdrData->rdrSy[i], vt/SCH_FACT, i);
				 }
				 else
					 DrawSymbol(rdrData->rdrSy[i], vt/SCH_FACT, i);

				 display->ZeroRotationAboutOrigin();
				 
				 //MI
				 if(g_bRealisticAvionics)
				 {
					 if(i == 0)
					 {
						 alt  = -rdrObj->BaseData()->ZPos();
						 if(rdrObj == lockedTarget &&
							 pFCC->LastMissileWillMiss(lockedTargetData->range) &&
							 (vuxRealTime & 0x180))
						 {
							 sprintf (str, "LOSE");
							 ShiAssert (strlen(str) < sizeof(str));
							 display->TextCenter(0.0F, -0.05F, str);
						 }
						 else if(rdrObj == lockedTarget)
						 {
							 alt  = -rdrObj->BaseData()->ZPos();
							 sprintf(str,"%02d",(int)((alt + 500.0F)*0.001));
							 ShiAssert (strlen(str) < sizeof(str));
							 display->TextCenter(0.0F, -0.05F, str);
						 }
						 else
						 {
							 if ((rdrData->rdrSy[i] >= Track) || (rdrObj->BaseData()->Id() == targetUnderCursor))
							 { 
								 alt  = -rdrObj->BaseData()->ZPos();
								 sprintf(str,"%02d",(int)((alt + 500.0F)*0.001));
								 ShiAssert (strlen(str) < sizeof(str));
								 display->TextCenter(0.0F, -0.05F, str);
							 }
						 }
					 }
					 

					 // JPO - draw hit ind.
					 if (rdrObj == lockedTarget &&
						 pFCC->MissileImpactTimeFlash > SimLibElapsedTime) 
					 { // Draw X
						 if (pFCC->MissileImpactTimeFlash-SimLibElapsedTime  > 5.0f * CampaignSeconds ||
							 (vuxRealTime & 0x200)) 
						 {
							 DrawSymbol(HitInd, 0, 0);
						 } 
					 }
				 }
				 
				 if (i == 0)
				 {
					 //MI done above in realistic
					 if(!g_bRealisticAvionics)
					 {
						 /*---------------------------------------------*/
						 /* target under cursor or locked show altitude */
						 /*---------------------------------------------*/
						 if ((rdrData->rdrSy[i] >= Track) || (rdrObj->BaseData()->Id() == targetUnderCursor))
						 {
							 alt  = -rdrObj->BaseData()->ZPos();
							 sprintf(str,"%02d",(int)((alt + 500.0F)*0.001));
							 ShiAssert (strlen(str) < sizeof(str));
							 display->TextCenter(0.0F, -0.15F, str);
						 }
					 }
					 
					 display->SetColor(GetMfdColor(MFD_ATTACK_STEERING_CUE)); // JPO draw in yellow
					 // Show collision steering if this is a locked target
					 if (rdrObj == lockedTarget)
						 DrawCollisionSteering(rdrObj, xPos);
					 
					 // SCR 9/9/98  If target is jamming, indicate such
					 if (rdrObj->BaseData()->IsSPJamming())
					 {
						 DrawSymbol( Jam, 0.0f, 0 );
					 }
					 
					 display->SetColor(tmpColor);
				 }
					 display->CenterOriginInViewport();

			} 
		}
	}
	rdrObj = rdrObj->next;
   }
}

void RadarDopplerClass::SAMDisplay(void)
{
float ang;
char str[20];
FireControlComputer* pFCC = ((SimVehicleClass*)platform)->GetFCC();


   if (!lockedTarget)
      return;
   else if (IsSet(STTingTarget))
   {
      display->SetColor(GetMfdColor(MFD_LABELS));
      if(IsAADclt(MajorMode) == FALSE) LabelButton (0, "CRM");
      if (IsAADclt(SubMode) == FALSE)
	{
		//MI
		if(lockedTarget)
			LabelButton (1, prevMode == LRS ? "LRS" : "RWS");
		else
			LabelButton (1, mode == LRS ? "LRS" : "RWS");
	}
      //LabelButton (2, "NRM");
      if(IsAADclt(Ovrd) == FALSE) LabelButton (3, "OVRD", NULL, !isEmitting); // JPO 
      if(IsAADclt(Cntl) == FALSE) LabelButton (4, "CTNL", NULL, IsSet(CtlMode));
      if (IsSet(MenuMode|CtlMode)) {
	  MENUDisplay ();
      } else {
	  // OSS Button Labels
	  AABottomRow ();
      }
      STTDisplay();
   }
   else
   {
      // DLZ?
      if (pFCC->GetSubMode() == FireControlComputer::Aim120 && pFCC->missileTarget)
         DrawDLZSymbol();

	   /*---------------------*/
	   /* Sam Mode Track data */
	   /*---------------------*/
	   // Aspect
      ang = max ( min (lockedTargetData->aspect * RTD * 0.1F, 18.0F), -18.0F);
	   sprintf (str, "%02.0f%c", ang, (lockedTargetData->azFrom > 0.0F ? 'R' : 'L'));
	   ShiAssert( strlen(str) < sizeof(str) );
      display->TextLeft(-0.875F, SECOND_LINE_Y, str);

	   // Heading
	   ang = lockedTarget->BaseData()->Yaw() * RTD;
      if(ang < 0.0F)
         ang += 360.0F;
	   sprintf (str, "%03.0f", ang);
	   ShiAssert( strlen(str) < sizeof(str) );
	   display->TextLeft(-0.5F, SECOND_LINE_Y, str);


	   // Closure
	   display->SetColor(GetMfdColor (MFD_TGT_CLOSURE_RATE));
      ang = max ( min (-lockedTargetData->rangedot * FTPSEC_TO_KNOTS, 1500.0F), -1500.0F);
	   sprintf (str, "%03.0f", ang);
	   ShiAssert( strlen(str) < sizeof(str) );
      display->TextRight (0.875F, SECOND_LINE_Y, str);

	   // TAS
       ang = max ( min (lockedTarget->BaseData()->Kias(), 1000.0F), -1000.0F);
	   sprintf (str, "%03.0f", ang);
	   ShiAssert( strlen(str) < sizeof(str) );
      display->TextRight (0.45F, SECOND_LINE_Y, str);

	   // SAM Target Elevation
	   display->AdjustOriginInViewport (-0.67F, TargetEl(platform,
		   lockedTarget) * disDeg);
	   display->Line (elMark[0][0], elMark[0][1], elMark[1][0], elMark[1][1]);
	   display->Line (elMark[2][0], elMark[2][1], elMark[3][0], elMark[3][1]);
	   display->CenterOriginInViewport();

	   // SAM Target Azimuth
	   display->AdjustOriginInViewport (TargetAz(platform,
		   lockedTarget) * disDeg, -0.67F);
	   display->AdjustRotationAboutOrigin(90.0F * DTR);
	   display->Line (elMark[0][0], elMark[0][1], elMark[1][0], elMark[1][1]);
	   display->Line (elMark[2][0], elMark[2][1], elMark[3][0], elMark[3][1]);
	   display->ZeroRotationAboutOrigin();
	   display->CenterOriginInViewport();

	   // Add NCTR data for any bugged target
		if(lockedTargetData->rdrSy[0] == Track || lockedTargetData->rdrSy[0] ==FlashTrack)
		{
			DrawNCTR(true);
		}

	   if (subMode == SAM_AUTO_MODE && lockedTargetData->range)
	   {
	       TargetToXY(lockedTargetData, 0, tdisplayRange, &cursorX, &cursorY);
	       //cursorY += 0.09F;
	   }

      // Add the rest of the RWS blips
      RWSDisplay();

      // Add Azimuth limit markers
      DrawAzLimitMarkers();
   }
}

void RadarDopplerClass::ACMDisplay (void)
{
    display->SetColor(GetMfdColor (MFD_LABELS));
    
    // OSS Button Labels
    if(IsAADclt(MajorMode) == FALSE) LabelButton (0, "ACM");
    
    if(IsAADclt(SubMode) == FALSE) {
	switch (mode)
	{
	case ACM_30x20:
	    LabelButton(1, "20");
	    break;
	    
	case ACM_SLEW:
	    LabelButton(1, "SLEW");
	    break;
	    
	case ACM_BORE:
	    LabelButton(1, "BORE");
	    break;
	    
	case ACM_10x60:
	    LabelButton(1, "60");
	    break;
	}
    }
   if(IsAADclt(Ovrd) == FALSE) LabelButton (3, "OVRD", NULL, !isEmitting);
   if(IsAADclt(Cntl) == FALSE) LabelButton (4, "CTNL", NULL, IsSet(CtlMode));
   if (IsSet(MenuMode|CtlMode)) {
       MENUDisplay ();
   } else 
       AABottomRow ();


   if (IsSet(STTingTarget))
      STTDisplay();
   else
   {
      if (mode != ACM_BORE)
      {
         // Set the viewport
         display->SetViewportRelative (DisplayAreaViewLeft, DisplayAreaViewTop, DisplayAreaViewRight, DisplayAreaViewBottom);
         DrawAzLimitMarkers();
      }

      if (mode == ACM_SLEW)
			DrawSlewCursor();
   }
}

void RadarDopplerClass::TWSDisplay (void)
{
    float xPos, yPos, alt;
    char  str[12];
    float ang, vt;
    SimObjectType* rdrObj = platform->targetList;
    SimObjectLocalData* rdrData;
    FireControlComputer* pFCC = ((SimVehicleClass*)platform)->GetFCC();
    int tmpColor = display->Color();

	//MI
	Pointer = 0;

    if (fovStepCmd)
    {
		fovStepCmd = 0;
		ToggleFlag(EXP);
    }
   // Labels / Text
    if (lockedTargetData)
    {
		display->SetColor(GetMfdColor (MFD_TGT_CLOSURE_RATE));
		
		// Aspect
		ang = max ( min (lockedTargetData->aspect * RTD * 0.1F, 18.0F), -18.0F);
		sprintf (str, "%02.0f%c", ang, (lockedTargetData->azFrom > 0.0F ? 'R' : 'L'));
		ShiAssert (strlen(str) < sizeof(str));
		display->TextLeft(-0.875F, SECOND_LINE_Y, str);
		
		// Heading
		ang = lockedTarget->BaseData()->Yaw() * RTD;
		if (ang < 0.0)
			ang += 360.0F;
		sprintf (str, "%03.0f", ang);
		ShiAssert (strlen(str) < sizeof(str));
		display->TextLeft(-0.5F, SECOND_LINE_Y, str);
		
		// Closure
		ang = max( min( -lockedTargetData->rangedot * FTPSEC_TO_KNOTS, 1500.0F), -1500.0F);
		sprintf (str, "%03.0f", ang);
		ShiAssert (strlen(str) < sizeof(str));
		display->TextRight (0.875F, SECOND_LINE_Y, str);
		
		// TAS
		ang = max( min( lockedTarget->BaseData()->Kias(), 1500.0F), -1500.0F);
		sprintf (str, "%03.0f", ang);
		ShiAssert (strlen(str) < sizeof(str));
		display->TextRight (0.45F, SECOND_LINE_Y, str);
		
		// Add NCTR data for any bugged target
		if (lockedTargetData->rdrSy[0] == Bug || lockedTargetData->rdrSy[0] == FlashBug)
		{
			DrawNCTR(true);
			// JPO - also do AIM120 DLZ
			if (pFCC->GetSubMode() == FireControlComputer::Aim120 && pFCC->missileTarget)
			DrawDLZSymbol();
		}
    }

   display->SetColor(GetMfdColor (MFD_LABELS));

	// OSS Button Labels
   if(IsAADclt(MajorMode) == FALSE) LabelButton (0, "CRM");
   if(IsAADclt(SubMode) == FALSE) LabelButton (1, "TWS");
   if (IsAADclt(Fov) == FALSE)											//MI make it flash faster
       LabelButton (2, IsSet(EXP) ? "EXP" : "NRM", NULL, IsSet(EXP) ? (vuxRealTime & 0x080) : 0);
   if(IsAADclt(Ovrd) == FALSE) LabelButton (3, "OVRD", NULL, !isEmitting);
   if(IsAADclt(Cntl) == FALSE) LabelButton (4, "CTNL", NULL, IsSet(CtlMode));
   if (IsSet(MenuMode|CtlMode)) {
       MENUDisplay();
   } else
       AABottomRow ();

   

   // Draw in the drawing area only
   display->SetViewportRelative (DisplayAreaViewLeft, DisplayAreaViewTop, DisplayAreaViewRight, DisplayAreaViewBottom);

   float tgtx, tgty;
   float brange;
   if (IsSet(EXP)) {
       if (lockedTargetData) {
	   // work out where on the display the target is
	   TargetToXY(lockedTargetData, 0, tdisplayRange, &tgtx, &tgty);
       }
       else {
	   tgtx = cursorX;
	   tgty = cursorY;
       }
       // draw the box
       brange = 2.0f * 2 * NM_TO_FT / tdisplayRange;
       display->Line(tgtx - brange, tgty - brange, tgtx + brange, tgty - brange);
       display->Line(tgtx + brange, tgty - brange, tgtx + brange, tgty + brange);
       display->Line(tgtx + brange, tgty + brange, tgtx - brange, tgty + brange);
       display->Line(tgtx - brange, tgty + brange, tgtx - brange, tgty - brange);
   }
   else {
       brange = 0;
       tgtx = 0; tgty = 0;
   }
   /*-----------------*/
   /* for all objects */
   /*-----------------*/
   while (rdrObj)
   {
      rdrData = rdrObj->localData;

      /*-------------------------*/
      /* show target and history */
      /*-------------------------*/
      if (rdrData->rdrY[0] > 1.0)
      {
		  TargetToXY(rdrData, 0, tdisplayRange, &xPos, &yPos);
	  
         /*----------------------------*/
         /* keep objects on the screen */
         /*----------------------------*/
         if (fabs(xPos) < AZL && fabs(yPos) < AZL)
         {
			 if (IsSet(EXP)) 
#if 0	// check if its in the box
				 && 
				 xPos > tgtx - brange && xPos < tgtx + brange &&
				 yPos > tgty - brange && yPos < tgty + brange
#endif
			 { 
				 float dx = xPos - tgtx;
				 float dy = yPos - tgty;
				 dx *= 4; // zoom factor
				 dy *= 4;
				 xPos = tgtx + dx;
				 yPos = tgty + dy;
				 if (fabs(xPos) > AZL || fabs(yPos) > AZL) 
				 {
					 rdrObj = rdrObj->next;
					 continue;
				 }
			 }

			 //Mi
			Pointer++;
			if(Pointer > 19)
				Pointer = 0;
			
			display->AdjustOriginInViewport (xPos, yPos);
			if (rdrData->rdrSy[0] >= Track)
			{
				// This _should_ be right -- based on 2D angle between target velocity
				// and our line of sight to target  SCR 10/27/97
				ang = rdrObj->BaseData()->Yaw() - rdrData->rdrX[0] - platform->Yaw();
				if (ang >= 0.0)
					ang = SCH_ANG_INC*(float)floor(ang/(SCH_ANG_INC * DTR));
				else
					ang = SCH_ANG_INC*(float)ceil(ang/(SCH_ANG_INC * DTR));
				display->AdjustRotationAboutOrigin (ang * DTR);
			}
			vt = rdrObj->BaseData()->Vt();
			if (rdrObj == lockedTarget && 
				pFCC->lastMissileImpactTime > 0.0F) 
			{ // Aim Target
				if (pFCC->lastMissileImpactTime > pFCC->lastmissileActiveTime)
					DrawSymbol(AimRel, vt/SCH_FACT, 0);
				else
					DrawSymbol(AimFlash, vt/SCH_FACT, 0);
			}
			else	     
				DrawSymbol(rdrData->rdrSy[0], vt/SCH_FACT, 0);

			display->ZeroRotationAboutOrigin();
			
			if (rdrData->rdrSy[0] >= Track)
			{
				if (rdrData->rdrSy[0] >= Bug)
				{
					DrawCollisionSteering(rdrObj, xPos);
				}
				alt  = -rdrObj->BaseData()->ZPos();
				if (rdrObj == lockedTarget &&
					pFCC->LastMissileWillMiss(lockedTargetData->range) &&
					(vuxRealTime & 0x180))
					sprintf (str, "LOSE");
				else 
					sprintf(str,"%02d",(int)((alt + 500.0F)*0.001));
				ShiAssert (strlen(str) < sizeof(str));
				display->TextCenter(0.0F, -0.05F, str);

				// JPO - draw hit ind.
				if (rdrObj == lockedTarget &&
					pFCC->MissileImpactTimeFlash > SimLibElapsedTime) { // Draw X
					if (pFCC->MissileImpactTimeFlash-SimLibElapsedTime  > 5.0f * CampaignSeconds ||
						(vuxRealTime & 0x200)) 
					{
						DrawSymbol(HitInd, 0, 0);
					}
				}
			}
			
			display->SetColor(GetMfdColor(MFD_UNKNOWN));
			/*------------------------------------*/
			/* target under cursor, show altitude */
			/*------------------------------------*/
			if (rdrObj->BaseData()->Id() == targetUnderCursor)
			{
				alt  = -rdrObj->BaseData()->ZPos();
				sprintf(str,"%02d",(int)((alt + 500.0F)*0.001));
				ShiAssert (strlen(str) < sizeof(str));
				display->TextCenter(0.0F, -0.05F, str);
			}
			
			// SCR 9/9/98  If target is jamming, indicate such (radar hit or no)
			if (rdrObj->BaseData()->IsSPJamming())
			{
				DrawSymbol( Jam, 0.0f, 0);
			}
			display->CenterOriginInViewport();
			display->SetColor(tmpColor);		
         }
      }
      rdrObj = rdrObj->next;
   }
}

void RadarDopplerClass::VSModeDisplay (void)
{
    display->SetColor(GetMfdColor (MFD_LABELS));
	// OSS Button Labels
   if(IsAADclt(MajorMode) == FALSE) LabelButton (0, "CRM");
   //MI this is labeled VSR, not VS
   if(!g_bRealisticAvionics)
   {
	   if(IsAADclt(SubMode) == FALSE) LabelButton (1, "VS");
   }
   else
   {
	   if(IsAADclt(SubMode) == FALSE) LabelButton (1, "VSR");
   }

   if(IsAADclt(Ovrd) == FALSE) LabelButton (3, "OVRD", NULL, !isEmitting);
   if(IsAADclt(Cntl) == FALSE) LabelButton (4, "CTNL", NULL, IsSet(CtlMode));
   if (IsSet(MenuMode|CtlMode))
       MENUDisplay();
   else
       AABottomRow ();

   if (IsSet(STTingTarget))
      STTDisplay();
}

void RadarDopplerClass::STTDisplay(void)
{
float xPos, yPos, alt;
char  str[12];
float ang, vt;
FireControlComputer* pFCC = ((SimVehicleClass*)platform)->GetFCC();
int tmpColor = display->Color();
//MI
Pointer = 0;

	if (!lockedTarget)
		return;

   if (pFCC->GetSubMode() == FireControlComputer::Aim120 && pFCC->missileTarget)
      DrawDLZSymbol();

   display->SetColor(GetMfdColor (MFD_TGT_CLOSURE_RATE));

	// Aspect 
	sprintf (str, "%02.0f%c", (lockedTargetData->aspect * RTD) * 0.1F,
		(lockedTargetData->azFrom > 0.0F ? 'R' : 'L'));
   display->TextLeft(-0.875F, SECOND_LINE_Y, str);
   ShiAssert (strlen(str) < sizeof(str));

	// Heading 
	ang = lockedTarget->BaseData()->Yaw() * RTD;
	 if (ang < 0.0)
		ang += 360.0F;
	sprintf (str, "%03.0f", ang);
   ShiAssert (strlen(str) < sizeof(str));
   display->TextLeft(-0.5F, SECOND_LINE_Y, str);

	// Closure 
   sprintf (str, "%03.0f", -lockedTargetData->rangedot * FTPSEC_TO_KNOTS);
   ShiAssert (strlen(str) < sizeof(str));
   display->TextRight (0.875F, SECOND_LINE_Y, str);

	// TAS 
	 sprintf (str, "%03.0f", lockedTarget->BaseData()->Kias());
   ShiAssert (strlen(str) < sizeof(str));
   display->TextRight (0.45F, SECOND_LINE_Y, str);

   // Add NCTR data for any bugged target //me123 addet check on ground & jamming
   if (!lockedTarget->BaseData()->OnGround() || !lockedTarget->BaseData()->IsSPJamming())
   {
       DrawNCTR(false);
   }
   // Set the viewport
   display->SetViewportRelative (DisplayAreaViewLeft, DisplayAreaViewTop, DisplayAreaViewRight, DisplayAreaViewBottom);

	/*-------------------------*/
   /* show target and history */
   /*-------------------------*/
   if (lockedTargetData->rdrY[0] > 1.0)
   {
       TargetToXY(lockedTargetData, 0, tdisplayRange, &xPos, &yPos);
       
       /*----------------------------*/
       /* keep objects on the screen */
       /*----------------------------*/
       if (fabs(xPos) < AZL && fabs(yPos) < AZL)
       {
	   display->AdjustOriginInViewport (xPos, yPos);
	   // This _should_ be right -- based on 2D angle between target velocity
	   // and our line of sight to target  SCR 10/27/97
	   ang = lockedTarget->BaseData()->Yaw() - lockedTargetData->rdrX[0] - platform->Yaw();
	   if (ang >= 0.0)
	       ang = SCH_ANG_INC*(float)floor(ang/(SCH_ANG_INC * DTR));
	   else
	       ang = SCH_ANG_INC*(float)ceil(ang/(SCH_ANG_INC * DTR));
	   display->AdjustRotationAboutOrigin (ang * DTR);
	   
	   vt = lockedTarget->BaseData()->Vt();
	   display->SetColor(GetMfdColor(MFD_UNKNOWN));
	   if (pFCC->lastMissileImpactTime > 0.0F) 
	   { // Aim Target
		   if (pFCC->lastMissileImpactTime > pFCC->lastmissileActiveTime)
			   DrawSymbol(AimRel, vt/SCH_FACT, 0);
		   else		   DrawSymbol(AimFlash, vt/SCH_FACT, 0);
	   }
	   else	     
		   DrawSymbol(Bug, vt/SCH_FACT, 0);
	   	   
	   int bcol = display->Color();
	   // Marco Edit - Rotation is rotating the CATA symbol
	   display->ZeroRotationAboutOrigin();
	   DrawCollisionSteering(lockedTarget, xPos);
	   //display->ZeroRotationAboutOrigin();

	   alt  = -lockedTarget->BaseData()->ZPos();
	   // draw lose indication
	   if (pFCC->LastMissileWillMiss(lockedTargetData->range) &&
	       (vuxRealTime & 0x180))
	       sprintf (str, "LOSE");
	   else 
	       sprintf(str,"%02d",(int)((alt + 500.0F)*0.001));
	   ShiAssert (strlen(str) < sizeof(str));
	   display->SetColor(bcol);
	   display->TextCenter(0.0F, -0.15F, str);
	   // draw hit indication
	   if (pFCC->MissileImpactTimeFlash > SimLibElapsedTime) { // Draw X
	       if (pFCC->MissileImpactTimeFlash-SimLibElapsedTime > 5.0f * CampaignSeconds ||
		   (vuxRealTime & 0x200)) {
		   DrawSymbol(HitInd, 0, 0);
	       }
	   }

	   // SCR 9/9/98  If target is jamming, indicate such
	   if (lockedTarget->BaseData()->IsSPJamming())
	   {
	       DrawSymbol( Jam, 0.0f, 0);
	   }
	   display->CenterOriginInViewport();
	   display->SetColor(tmpColor);
	   
       }
   }
   //MI
   Pointer++;
   if(Pointer > 19)
	   Pointer = 0;
}

void RadarDopplerClass::VSDisplay(void)
{
int   i;
float xPos, yPos;
SimObjectType* rdrObj = platform->targetList;
SimObjectLocalData* rdrData;
float alt;
char str[8];
int tmpColor = display->Color();
FireControlComputer* pFCC = ((SimVehicleClass*)platform)->GetFCC();
//MI
Pointer = 0;

    display->SetColor(GetMfdColor (MFD_LABELS));

	// OSS Button Labels
   
   if(IsAADclt(MajorMode) == FALSE) LabelButton (0, "CRM");
   //MI this is VSR, not VS
   if(!g_bRealisticAvionics)
   {
	   if(IsAADclt(SubMode) == FALSE) LabelButton (1, "VS");
   }
   else
   {
	   if(IsAADclt(SubMode) == FALSE) LabelButton (1, "VSR");
   }

   if(IsAADclt(Ovrd) == FALSE) LabelButton (3, "OVRD", NULL, !isEmitting);
   if(IsAADclt(Cntl) == FALSE) LabelButton (4, "CTNL", NULL, IsSet(CtlMode));
   if (IsSet(MenuMode|CtlMode)) 
       MENUDisplay();
   else
       AABottomRow ();

   if (IsSet(STTingTarget))
   {
      STTDisplay();
   }
   else
   {
      // Set the viewport
      display->SetViewportRelative (DisplayAreaViewLeft, DisplayAreaViewTop, DisplayAreaViewRight, DisplayAreaViewBottom);
		DrawAzLimitMarkers();

      /*-----------------*/
      /* for all objects */
      /*-----------------*/
	   while (rdrObj)
      {
         rdrData = rdrObj->localData;

         /*-------------------------*/
         /* show target and history */
         /*-------------------------*/
         for (i=histno-1; i>=0; i--)// dynamic history
         {
            if (rdrData->rdrY[i] > 1.0)
            {
				//MI
				Pointer++;
				if(Pointer > 19)
					Pointer = 0;
				TargetToXY(rdrData, i, displayRange, &xPos, &yPos);
               /*----------------------------*/
               /* keep objects on the screen */
               /*----------------------------*/
               if (fabs(xPos) < AZL && fabs(yPos) < AZL)
               {
				   display->AdjustOriginInViewport (xPos, yPos);
				   // JPO work out the track color by reducing each component in turn
				   DrawSymbol(rdrData->rdrSy[i], 0.0F, i);
				   display->SetColor(GetMfdColor(MFD_UNKNOWN)); // JPO draw in yellow
		   
				   if (rdrObj->BaseData()->Id() == targetUnderCursor)
				   {
					   alt  = -rdrObj->BaseData()->ZPos();
					   sprintf(str,"%02d",(int)((alt + 500.0F)*0.001));
					   ShiAssert (strlen(str) < sizeof(str));
					   display->TextCenter(0.0F, -0.05F, str);
				   }
				   
				   // SCR 9/9/98  If target is jamming, indicate such
				   if (rdrObj->BaseData()->IsSPJamming())
				   {
					   DrawSymbol( Jam, 0.0f, 0 );
				   }
				   display->SetColor(tmpColor);
				   display->CenterOriginInViewport();
               }
            }
         }
         rdrObj = rdrObj->next;
      }
   }
}

void RadarDopplerClass::DrawACQCursor(void)
{
	//MI
	//static const float CursorSize = 0.03f;
	static float CursorSize;
	if(!g_bRealisticAvionics)
		CursorSize = 0.03F;
	else
		CursorSize = 0.06F;

	static float TextLeftPos;
	if(!g_bRealisticAvionics)
		TextLeftPos = 0.03F;
	else
		TextLeftPos = 0.11F;

float  up,lw,z,ang,height,theta;
char   str[8];
FireControlComputer* pFCC = ((SimVehicleClass*)platform)->GetFCC();//me123 addet for ccip/DTOSS ground ranging check
    int tmpColor = display->Color();

   up = lw = 0.0F;

   /* angle of radar scan center to the horizon */
	ang = seekerElCenter;
   theta  = platform->Pitch();

   if (!IsSet(SpaceStabalized))
   {
	   //MI this was messed up when our platform was rolling
	   //ang += theta;
	   mlTrig trig;
	   mlSinCos(&trig, platform->Roll());
	   ang = theta + seekerElCenter * trig.cos - seekerAzCenter * trig.sin;
   }
	else if (mode == TWS && lockedTarget)
   {
      theta = 0.0F;
   }
   else
	{
      if (theta > MAX_ANT_EL)
         ang += (theta - MAX_ANT_EL);
      else if (theta < -MAX_ANT_EL)
         ang += (theta + MAX_ANT_EL);
	}

   /* cursor range */
   cursRange = tdisplayRange * (cursorY+AZL) / (2.0F*AZL);

   /* altitude of the scan center above ground */
   scanCenterAlt = -platform->ZPos() +
   	cursRange * (float)sin(ang);

   /*---------------------*/
   /* find height of scan */
   /*---------------------*/
   /* height of the scan volume in degrees */
   height = ((bars > 0 ? bars : -bars) - 1) * tbarWidth + beamWidth * 2.0F;

   /* vertical dimension of the scan volume in feet */
   z = cursRange * (float)sin(height*0.5);

   /* upper scan limit in thousands */
   up = (scanCenterAlt + z) * 0.001F;
   /* lower scan limit in thousands */
   lw = (scanCenterAlt - z) * 0.001F;

   /*---------------------------------*/
   /* altitudes must be 00 <= x <= 99 */
   /*---------------------------------*/
	up = min ( max (up, -99.0F), 99.0F);
	lw = min ( max (lw, -99.0F), 99.0F);

   /*-------------*/
   /* draw cursor */
   /*-------------*/
	display->AdjustOriginInViewport (cursorX, cursorY);
	display->SetColor(GetMfdColor(MFD_CURSOR));
	display->Line (-CursorSize, CursorSize, -CursorSize, -CursorSize);
	display->Line ( CursorSize, CursorSize,  CursorSize, -CursorSize);

   /*----------------------*/
   /* add elevation limits */
   /*----------------------*/
   if (mode != VS)
   {
	   if(g_bMLU)
	   {
		   if (up > -1.0f)
		   {
			   sprintf(str,"%02d",(int)up);
			   ShiAssert (strlen(str) < sizeof(str));
			   display->TextLeftVertical(TextLeftPos+display->TextWidth("-"), 2.0F*CursorSize, str);
		   }
		   else
		   {
			   sprintf(str,"%03d",(int)up);
			   ShiAssert (strlen(str) < sizeof(str));
			   display->TextLeftVertical(TextLeftPos, 2.0F*CursorSize, str);
		   }

		   if (lw > -1.0f)
		   {
			   sprintf(str,"%02d",(int)lw);
			   ShiAssert (strlen(str) < sizeof(str));
			   display->TextLeftVertical(TextLeftPos+display->TextWidth("-"),-2.0F*CursorSize, str);
		   }
		   else
		   {
			   sprintf(str,"%03d",(int)lw);
			   ShiAssert (strlen(str) < sizeof(str));
			   display->TextLeftVertical(TextLeftPos,-2.0F*CursorSize, str);
		   }
	   }
	   else
	   {
		   up = min ( max (up, 0.0F), 99.0F);
		   lw = min ( max (lw, 0.0F), 99.0F);

		   sprintf(str,"%02d",(int)up);
		   ShiAssert (strlen(str) < sizeof(str));
		   display->TextLeftVertical(TextLeftPos, 2.0F*CursorSize, str);

		   sprintf(str,"%02d",(int)lw);
		   ShiAssert (strlen(str) < sizeof(str));
		   display->TextLeftVertical(TextLeftPos,-2.0F*CursorSize, str);
	   }
   }
   display->SetColor(tmpColor);
   display->CenterOriginInViewport();
}

void RadarDopplerClass::DrawSlewCursor(void)
{
float  up,lw,z,ang,height,theta;
char   str[8];
FireControlComputer* pFCC = ((SimVehicleClass*)platform)->GetFCC();//me123 addet for ccip/DTOSS ground ranging check
    int tmpColor = display->Color();

   up = lw = 0.0F;

	ang = seekerElCenter;
   theta  = platform->Pitch();
	ang += theta;

   /* altitude of the scan center above ground */
   scanCenterAlt = -platform->ZPos() + 5.0F * NM_TO_FT * (float)sin(ang);

   /*---------------------*/
   /* find height of scan */
   /*---------------------*/
   /* height of the scan volume in degrees */
   height = ((bars > 0 ? bars : -bars) - 1) * tbarWidth + beamWidth * 2.0F;

   /* vertical dimension of the scan volume in feet */
   z = cursRange * (float)sin(height*0.5);

   /* upper scan limit in thousands */
   up = (scanCenterAlt + z) * 0.001F;
   /* lower scan limit in thousands */
   lw = (scanCenterAlt - z) * 0.001F;

   /*---------------------------------*/
   /* altitudes must be 00 <= x <= 99 */
   /*---------------------------------*/
	up = min ( max (up, 0.0F), 99.0F);
	lw = min ( max (lw, 0.0F), 99.0F);

   /*-------------*/
   /* draw cursor */
   /*-------------*/
	display->AdjustOriginInViewport (cursorX, cursorY);
	display->SetColor(GetMfdColor(MFD_CURSOR));
	display->Line (cursor[0][0],  cursor[0][1],  cursor[1][0],  cursor[1][1]);
	display->Line (cursor[1][0],  cursor[1][1],  cursor[2][0],  cursor[2][1]);
	display->Line (cursor[2][0],  cursor[2][1],  cursor[3][0],  cursor[3][1]);
	display->Line (cursor[3][0],  cursor[3][1],  cursor[4][0],  cursor[4][1]);
	display->Line (cursor[4][0],  cursor[4][1],  cursor[5][0],  cursor[5][1]);
	display->Line (cursor[5][0],  cursor[5][1],  cursor[6][0],  cursor[6][1]);
	display->Line (cursor[6][0],  cursor[6][1],  cursor[7][0],  cursor[7][1]);
	display->Line (cursor[7][0],  cursor[7][1],  cursor[8][0],  cursor[8][1]);
	display->Line (cursor[8][0],  cursor[8][1],  cursor[9][0],  cursor[9][1]);
	display->Line (cursor[9][0],  cursor[9][1],  cursor[10][0], cursor[10][1]);
	display->Line (cursor[10][0], cursor[10][1], cursor[11][0], cursor[11][1]);
	display->Line (cursor[11][0], cursor[11][1], cursor[0][0],  cursor[0][1]);
   display->CenterOriginInViewport();

   /*----------------------*/
   /* add elevation limits */
   /*----------------------*/
   sprintf(str,"%02d",(int)up);
	ShiAssert (strlen(str) < sizeof(str));
   display->TextLeft(0.04F, 0.035F, str);

   sprintf(str,"%02d",(int)lw);
	ShiAssert (strlen(str) < sizeof(str));
   display->TextLeft(0.04F,-0.035F, str);
    display->SetColor(tmpColor);
}

void RadarDopplerClass::DrawAzLimitMarkers(void)
{
    if (IsAADclt(AzBar)) return;
	if (IsSet(STTingTarget)) return;
float x;
	display->SetColor(GetMfdColor(MFD_FCR_AZIMUTH_SCAN_LIM));
   // az limit marker
   if (azScan < MAX_ANT_EL - beamWidth)
   {
      x = min (disDeg * (seekerAzCenter + (azScan+beamWidth)), 0.99F);
      display->Line (x , 1.0F, x, -1.0F);

      x = max (disDeg * (seekerAzCenter - (azScan+beamWidth)), -0.99F);
      display->Line (x , 1.0F, x, -1.0F);
   }
}

// JPo - redone for new symbols and new colours.
void RadarDopplerClass::DrawSymbol(int type, float schweemLen, int age, float x, float y, int temp,  SimObjectType *rdrObj)
{
    static const float tgtSize = 0.04f;
    static const float jamSizeW = 0.12f;
    static const float jamSizeH = 0.16f;
    static const float jamNewSizeH = 0.06f;
    static const float jamNewSizeV = 0.04f;
    static const float jamNewDelta = 0.06f;
    static const float hitSizeW = 0.08f;
    static const float hitSizeH = 0.1f;
    static const float trackScale = 0.1f;
    static const float trackTriH = trackScale * (float)cos( DTR * 30.0f );
    static const float trackTriV = trackScale * (float)sin( DTR * 30.0f );
    int flash = (vuxRealTime & 0x200);
    /*-------------*/
    /* draw symbol */
    /*-------------*/
    if ((type == FlashBug || type == FlashTrack) && flash)
	return;

	switch(type)
    {
    case FlashBug:
    case AimFlash:
    case AimRel:
	display->SetColor(GetAgedMfdColor(MFD_FCR_BUGGED_FLASH_TAIL, age));
	display->Circle(0.0F, 0.0F, trackScale);
	break;
    case Bug:
	display->SetColor(GetAgedMfdColor(MFD_FCR_BUGGED, age));
	display->Circle(0.0F, 0.0F, trackScale);
	break;
    case HitInd:
	display->SetColor(GetAgedMfdColor(MFD_KILL_X, age));
	break;
    case FlashTrack:
	display->SetColor(GetAgedMfdColor(MFD_FCR_UNK_TRACK_FLASH, age));
	break;
    case Track:
	display->SetColor(GetAgedMfdColor(MFD_FCR_UNK_TRACK, age));
	break;
    case  Solid:
    case Jam:
    case  Det:
	display->SetColor(GetAgedMfdColor(MFD_FCR_UNK_TRACK, age));
	break;
    } /*switch*/
    switch(type)
    {
    case AimFlash: // jpo draw filled square
    case AimRel:
		if (g_bRealisticAvionics && (type == AimRel || vuxRealTime & 0x080)) {	//MI changed from || flash
	    display->Tri(g_fRadarScale * trackTriH/2.0f, g_fRadarScale * -trackTriV, //tail flashes faster then flash
		g_fRadarScale * trackTriH/2.0f, g_fRadarScale * -trackTriV-0.035, 
		g_fRadarScale * -trackTriH/2.0f, g_fRadarScale * -trackTriV);
	    display->Tri(g_fRadarScale * -trackTriH/2.0f, g_fRadarScale * -trackTriV, 
		g_fRadarScale * -trackTriH/2.0f, g_fRadarScale * -trackTriV-0.035, 
		g_fRadarScale * trackTriH/2.0f, g_fRadarScale * -trackTriV-0.035);
	}
	// fall
    case FlashBug:
    case Bug:
    case FlashTrack:
    case Track:
	if (g_bRealisticAvionics) {
	    if (g_bEPAFRadarCues) { // draw a Square.
		display->Line(g_fRadarScale * tgtSize, g_fRadarScale * tgtSize, g_fRadarScale * -tgtSize, g_fRadarScale * tgtSize);
		display->Line(g_fRadarScale *  -tgtSize, g_fRadarScale * tgtSize, g_fRadarScale * -tgtSize, g_fRadarScale * -tgtSize),
		display->Line(g_fRadarScale *  -tgtSize, g_fRadarScale * -tgtSize, g_fRadarScale * tgtSize, g_fRadarScale * -tgtSize),
		display->Line(g_fRadarScale *  tgtSize, g_fRadarScale * -tgtSize, g_fRadarScale * tgtSize, g_fRadarScale * tgtSize);
		display->Line (0.0f, g_fRadarScale * tgtSize, 0.0f, g_fRadarScale * (tgtSize + DD_LENGTH*schweemLen));
	    }
	    else { // draw a hollow triange
		display->Line(0.0f, g_fRadarScale * trackScale, g_fRadarScale * trackTriH, g_fRadarScale * -trackTriV);
		display->Line(g_fRadarScale * trackTriH, g_fRadarScale * -trackTriV, g_fRadarScale * -trackTriH, g_fRadarScale * -trackTriV);
		display->Line(g_fRadarScale * -trackTriH, g_fRadarScale * -trackTriV, 0.0f, g_fRadarScale * trackScale);
		display->Line (0.0f, g_fRadarScale * trackScale, 0.0f, g_fRadarScale * (trackScale + DD_LENGTH*schweemLen));
	    }
	}
	else {
	    display->Tri (0.0f, g_fRadarScale * trackScale, g_fRadarScale * trackTriH, g_fRadarScale * -trackTriV, g_fRadarScale * -trackTriH, g_fRadarScale * -trackTriV);
	    display->Line (0.0f, g_fRadarScale * trackScale, 0.0f, g_fRadarScale * (trackScale + DD_LENGTH*schweemLen));
	}
	break;
    case  Solid:
	display->Tri (g_fRadarScale * tgtSize, g_fRadarScale * tgtSize, g_fRadarScale * -tgtSize, g_fRadarScale * tgtSize, g_fRadarScale * -tgtSize, g_fRadarScale * -tgtSize);
	display->Tri (g_fRadarScale * tgtSize, g_fRadarScale * tgtSize, g_fRadarScale * tgtSize, g_fRadarScale * -tgtSize, g_fRadarScale * -tgtSize, g_fRadarScale * -tgtSize);
	break;
	
    case HitInd: // new symbol
	display->Line( g_fRadarScale * -hitSizeW,  g_fRadarScale * hitSizeH, g_fRadarScale * hitSizeW, g_fRadarScale * -hitSizeH );
	display->Line( g_fRadarScale *  hitSizeW,  g_fRadarScale * hitSizeH, g_fRadarScale * -hitSizeW, g_fRadarScale * -hitSizeH );
	break;

    case Jam:
	 if (g_bRadarJamChevrons) { // JPO two chevrons pointing up.
	    display->Line(g_fRadarScale * -jamNewSizeH, g_fRadarScale * -jamNewSizeV, 0, 0);
	    display->Line(0, 0, g_fRadarScale * jamNewSizeH, g_fRadarScale * -jamNewSizeV);
	    display->Line(g_fRadarScale * -jamNewSizeH, g_fRadarScale * (-jamNewSizeV+jamNewDelta), 0, g_fRadarScale * jamNewDelta);
	    display->Line(0, g_fRadarScale * jamNewDelta, g_fRadarScale * jamNewSizeH, g_fRadarScale * -jamNewSizeV+jamNewDelta);
	}
	else {
	    display->Line( g_fRadarScale * -jamSizeW,  g_fRadarScale * jamSizeH,  g_fRadarScale * jamSizeW, g_fRadarScale * -jamSizeH );
	    display->Line( g_fRadarScale *  jamSizeW,  g_fRadarScale * jamSizeH, g_fRadarScale * -jamSizeW, g_fRadarScale * -jamSizeH );
	}
	
    case  Det:
		if(!g_bRealisticAvionics)
		{
			display->Line (g_fRadarScale * -tgtSize, 0.0f, g_fRadarScale * tgtSize, 0.0f);
		}
		else
		{
			//MI we get squares here too
			display->Tri (g_fRadarScale * tgtSize, g_fRadarScale * tgtSize, g_fRadarScale * -tgtSize, g_fRadarScale * tgtSize, g_fRadarScale * -tgtSize, g_fRadarScale * -tgtSize);
			display->Tri (g_fRadarScale * tgtSize, g_fRadarScale * tgtSize, g_fRadarScale * tgtSize, g_fRadarScale * -tgtSize, g_fRadarScale * -tgtSize, g_fRadarScale * -tgtSize);
		}

	break;
	case InterogateUnk:
	case InterogateFoe:
	break;
	case InterogateFriend:
	break;
    } /*switch*/
}

int RadarDopplerClass::IsUnderCursor (SimObjectType* rdrObj, float heading)
{
float az, xPos, yPos;
int retval = FALSE;

   if (rdrObj->localData->rdrDetect)
   {
      /*-----------------------------------------------*/
      /* azimuth corrected for ownship heading changes */
      /*-----------------------------------------------*/
      az = rdrObj->localData->rdrX[0] + (rdrObj->localData->rdrHd[0] - heading);
      az = RES180(az);

      /*---------------------------------*/
      /* find x and y location on bscope */
      /*---------------------------------*/
      xPos = disDeg*az;
      yPos = -AZL + 2.0F*AZL*(rdrObj->localData->rdrY[0] / tdisplayRange);
	   if (xPos > cursorX - 0.02F  && xPos < cursorX + 0.02F  &&
		    yPos > cursorY - 0.04F  && yPos < cursorY + 0.04F )
		   retval = TRUE;
   }

	return (retval);
}

int RadarDopplerClass::IsUnderVSCursor (SimObjectType* rdrObj, float heading)
{
float az, xPos, yPos;
int retval = FALSE;

   if (rdrObj->localData->rdrDetect)
   {
      /*-----------------------------------------------*/
      /* azimuth corrected for ownship heading changes */
      /*-----------------------------------------------*/
      az = rdrObj->localData->rdrX[0] + (rdrObj->localData->rdrHd[0] - heading);
      az = RES180(az);

      /*---------------------------------*/
      /* find x and y location on bscope */
      /*---------------------------------*/
      xPos = disDeg*az;
      yPos = -AZL + 2.0F*AZL*(rdrObj->localData->rdrY[0] / displayRange);
	   if (fabs (xPos - cursorX) < 0.04 && fabs (yPos - cursorY) < 0.04)
		   retval = TRUE;
   }

	return (retval);
}

void RadarDopplerClass::DrawCollisionSteering (SimObjectType* buggedTarget, float curX)
{
float   dx, dy, xPos = 0.0F;
vector  collPoint;

   if (!buggedTarget || !buggedTarget->BaseData()->IsSim())
      return;
   if (IsAADclt(AttackStr)) return;

   if (FindCollisionPoint ((SimBaseClass*)buggedTarget->BaseData(), platform, &collPoint))
   {// me123 status ok. Looks like collision point is returned in World Coords.  We need to
	// make it relative to ownship so subtract out ownship pos 1st....

//me123 addet next three lines
	  collPoint.x -= platform->XPos();
      collPoint.y -= platform->YPos();
     collPoint.z -= platform->ZPos();

      dx = collPoint.x;
      dy = collPoint.y;



      xPos = (float)atan2 (dy,dx) - platform->Yaw();

      if (xPos > (180.0F * DTR))
         xPos -= 360.0F * DTR;
      else if (xPos < (-180.0F * DTR))
         xPos += 360.0F * DTR;
   }

   /*---------------------------*/
   /* steering point screen x,y */
   /*---------------------------*/
   if (fabs(xPos) < (60.0F * DTR))
   {
      xPos = xPos / (60.0F * DTR);
      xPos -= curX;

      /*-------------*/
      /* draw symbol */
      /*-------------*/
       display->SetColor(GetMfdColor (MFD_ATTACK_STEERING_CUE));
      display->AdjustOriginInViewport(xPos, 0.0F);
      if (g_bRealisticAvionics) { // draw maltese cross
	  static const float MaxPos = 0.045f;
	  static const float MinPos = 0.015f;
	  static const float OverLap = 0.0075f;
	  display->Tri(-MinPos, -MaxPos,  MinPos, -MaxPos, 0, OverLap); // triangle one down
	  display->Tri(-MinPos,  MaxPos,  MinPos,  MaxPos, 0, -OverLap); // triangle two up
	  display->Tri(-MaxPos, -MinPos, -MaxPos,  MinPos, OverLap, 0); // triangle three left
	  display->Tri( MaxPos, -MinPos,  MaxPos,  MinPos, -OverLap, 0); // triangle four right
      } else
      {
	  display->Circle(0.0F, 0.0F, 0.03F);
      }
      display->AdjustOriginInViewport(-xPos, 0.0F);
   }
}

void RadarDopplerClass::DrawSteerpoint (void)
{
float az, range, xPos, yPos;
float curSteerpointX, curSteerpointY, curSteerpointZ;

   if (((SimVehicleClass*)platform)->curWaypoint)
   {
      ((SimVehicleClass*)platform)->curWaypoint->GetLocation (&curSteerpointX, &curSteerpointY, &curSteerpointZ);
	   az = TargetAz (platform, curSteerpointX, curSteerpointY);
	   range = (float)sqrt(
		   (curSteerpointX - platform->XPos()) * (curSteerpointX - platform->XPos()) +
		   (curSteerpointY - platform->YPos()) * (curSteerpointY - platform->YPos()));

      /*---------------------------------*/
      /* find x and y location on bscope */
      /*---------------------------------*/
      xPos = disDeg*az;
      yPos = -AZL + 2.0F*AZL*(range / tdisplayRange);

      /*----------------------------*/
      /* keep objects on the screen */
      /*----------------------------*/
      if (fabs(xPos) < AZL && fabs(yPos) < AZL)
      {
	  	   display->SetColor(GetMfdColor (MFD_CUR_STPT));

		   display->AdjustOriginInViewport (xPos, yPos);
         display->Tri(steerpoint[0][0],  steerpoint[0][1],
			   steerpoint[1][0],  steerpoint[1][1],
			   steerpoint[10][0],  steerpoint[10][1]);
         display->Tri(steerpoint[0][0],  steerpoint[0][1],
			   steerpoint[11][0],  steerpoint[11][1],
			   steerpoint[10][0],  steerpoint[10][1]);
         display->Tri(steerpoint[2][0],  steerpoint[2][1],
			   steerpoint[3][0],  steerpoint[3][1],
			   steerpoint[8][0],  steerpoint[8][1]);
         display->Tri(steerpoint[2][0],  steerpoint[2][1],
			   steerpoint[9][0],  steerpoint[9][1],
			   steerpoint[8][0],  steerpoint[8][1]);
         display->Tri(steerpoint[4][0],  steerpoint[4][1],
			   steerpoint[5][0],  steerpoint[5][1],
			   steerpoint[6][0],  steerpoint[6][1]);
         display->Tri(steerpoint[4][0],  steerpoint[4][1],
			   steerpoint[7][0],  steerpoint[7][1],
			   steerpoint[6][0],  steerpoint[6][1]);
		   display->AdjustOriginInViewport (-xPos, -yPos);
	   }
   }
    display->SetColor (GetMfdColor(MFD_BULLSEYE));
   // Add the bullseye
   TheCampaign.GetBullseyeSimLocation (&xPos, &yPos);
	az = TargetAz (platform, xPos, yPos);
	range = (float)sqrt(
		(xPos - platform->XPos()) * (xPos - platform->XPos()) +
		(yPos - platform->YPos()) * (yPos - platform->YPos()));

   /*---------------------------------*/
   /* find x and y location on bscope */
   /*---------------------------------*/
   xPos = disDeg*az;
   yPos = -AZL + 2.0F*AZL*(range / tdisplayRange);

   /*----------------------------*/
   /* keep objects on the screen */
   /*----------------------------*/
   if (fabs(xPos) < AZL && fabs(yPos) < AZL)
   {
	   // Possible CATA Circle
      display->Circle (xPos, yPos, 0.07F);
      display->Circle (xPos, yPos, 0.04F);
      display->Circle (xPos, yPos, 0.01F);
   }
}

void RadarDopplerClass::DrawDLZSymbol(void)
{
    if (IsAADclt(Dlz)) return;
FireControlComputer* pFCC = ((SimVehicleClass*)platform)->GetFCC();
char  tmpStr[8];
float rMin;
float rMax;
float rNeMin;
float rNeMax;
float yOffset;
static float leftEdge   = 0.9F;
static float bottomEdge = -0.8F;
static float width      = 0.05F;
static float height     = 1.4F;
static float rangeInv = 1.0F / tdisplayRange;
float textbottom;
int color = display->Color();
   rMax   = pFCC->missileRMax * rangeInv;
   rMin   = pFCC->missileRMin * rangeInv;
   rNeMax = pFCC->missileRneMax * rangeInv; // Marco Edit * 0.70f;//me123 addet *0.70 ;
   rNeMin = pFCC->missileRneMin * rangeInv;
    
   ShiAssert(lockedTargetData != NULL);
   // Clamp in place
   rMin = min (rMin, 1.0F);
   rMax = min (rMax, 1.0F);
   rNeMin = min (rNeMin, 1.0F);
   rNeMax = min (rNeMax, 1.0F);
    display -> SetColor(GetMfdColor(MFD_DLZ));
   // Rmin/Rmax
    textbottom = bottomEdge + rMin * height;
   display->Line (leftEdge, textbottom, leftEdge + width, bottomEdge + rMin * height);
   display->Line (leftEdge, textbottom, leftEdge, bottomEdge + rMax * height);
   display->Line (leftEdge, bottomEdge + rMax * height, leftEdge + width, bottomEdge + rMax * height);

   // Range Caret
   yOffset = bottomEdge + lockedTargetData->range * rangeInv * height;
   if (g_bRealisticAvionics) { // draw a >
       display->Line (leftEdge, yOffset, leftEdge - 0.05F, yOffset - 0.05F);
       display->Line (leftEdge, yOffset, leftEdge - 0.05F, yOffset + 0.05F);
   }
   else { // draw a |-
       display->Line (leftEdge, yOffset, leftEdge - 0.05F, yOffset);
       display->Line (leftEdge - 0.05F, yOffset + 0.05F, leftEdge - 0.05F, yOffset - 0.05F);
   }
   sprintf (tmpStr, "%.0f ", max ( min (-lockedTargetData->rangedot * FTPSEC_TO_KNOTS, 1500.0F), -1500.0F));
   display->TextRightVertical (leftEdge - 0.05F, yOffset, tmpStr);

   // No Escape Zone
	display->Line (leftEdge, bottomEdge + rNeMin * height, leftEdge + width, bottomEdge + rNeMin * height);
	display->Line (leftEdge, bottomEdge + rNeMax * height, leftEdge + width, bottomEdge + rNeMax * height);
	display->Line (leftEdge + width, bottomEdge + rNeMin * height, leftEdge + width, bottomEdge + rNeMax * height);

   // Range for immediate Active
   //LRKLUDGE
   yOffset = pFCC->missileActiveRange * rangeInv;
   yOffset = min ( max ( 0.0F, yOffset), 1.0F);
   yOffset = bottomEdge + yOffset * height;
    display -> SetColor(GetMfdColor(MFD_STEER_ERROR_CUE));

   display->Circle (leftEdge, yOffset, 0.02F);

   
   // Draw the ASEC symbol. if its flashing, or outside the NE zone JPO
   if (g_bRealisticAvionics &&
       ((vuxRealTime & 0x200) ||
       lockedTargetData->range > pFCC->missileRneMax*0.7f ||
       lockedTargetData->range < pFCC->missileRneMin)) {
       display -> SetColor(GetMfdColor(MFD_STEER_ERROR_CUE));
       display->Circle(0.0f, 0.0f, pFCC->Aim120ASECRadius(lockedTargetData->range));
   }
   
    display -> SetColor(GetMfdColor(MFD_DLZ));
   // Draw Time to die strings
   if (pFCC->nextMissileImpactTime > 0.0F)
   {
       if (pFCC->nextMissileImpactTime > pFCC->lastmissileActiveTime)
       {
	   sprintf (tmpStr, "A%.0f", pFCC->nextMissileImpactTime - pFCC->lastmissileActiveTime);
       }
       else
       {
	   sprintf (tmpStr, "T%.0f", pFCC->nextMissileImpactTime);
       }
       ShiAssert (strlen(tmpStr) < sizeof(tmpStr));
//       display->TextRight (leftEdge + width, textbottom - display->TextHeight(), tmpStr);
       display->TextRight (leftEdge + width -0.08F, bottomEdge + 0.8F * display->TextHeight(), tmpStr);

   }

   if (pFCC->lastMissileImpactTime > 0.0F)
   {
     // lose indications
       if (pFCC->LastMissileWillMiss(lockedTargetData->range)) { // JPO lose indication
	   sprintf (tmpStr, "L%.0f", pFCC->lastMissileImpactTime);
       }
       else if (pFCC->lastMissileImpactTime > pFCC->lastmissileActiveTime)
       {
	   sprintf (tmpStr, "A%.0f", pFCC->lastMissileImpactTime - pFCC->lastmissileActiveTime);
       }
       else
       {
	   sprintf (tmpStr, "T%.0f", pFCC->lastMissileImpactTime);
       }
       ShiAssert (strlen(tmpStr) < sizeof(tmpStr));
//       display->TextRight (leftEdge + width, textbottom - 2.1 * display->TextHeight(), tmpStr);
       display->TextRight (leftEdge + width - 0.08F, bottomEdge, tmpStr);
   }
   display->SetColor(color);
}
	
// JPO fetch bugged target
int RadarDopplerClass::GetBuggedData (float *x, float *y, float *dir, float *speed)
{
    if (lockedTarget == NULL)
	return FALSE;
	//MI I haven't seen a rdrSy[0] state of Bug.. thus we only get it in STT.
    //if (lockedTargetData->rdrSy[0] >= Bug ||
	if(lockedTarget ||
	(IsSet(STTingTarget) && !lockedTarget->BaseData()->OnGround())) 
	{
		*x = lockedTarget->BaseData()->XPos();
		*y = lockedTarget->BaseData()->YPos();
		*dir = lockedTarget->BaseData()->Yaw() - lockedTargetData->rdrX[0];
		*speed = lockedTarget->BaseData()->Vt();
		return TRUE;
    }
    else 
		return FALSE;
}

// JPO - bottom row of MFD buttons.
void RadarDopplerClass::AABottomRow ()
{
   if (g_bRealisticAvionics) 
   {
       if (IsAADclt(Dclt) == FALSE) LabelButton(10, "DCLT", NULL, IsSet(AADecluttered));
       if (IsAADclt(Fmt1) == FALSE) DefaultLabel(11);
       if (IsAADclt(Fmt2) == FALSE) DefaultLabel(12);
       if (IsAADclt(Fmt3) == FALSE) DefaultLabel(13);
       if (IsAADclt(Swap) == FALSE) DefaultLabel(14);
	   
	   FackClass* mFaults = ((AircraftClass*)(SimDriver.playerEntity))->mFaults;
	   if(mFaults && !(mFaults->GetFault(FaultClass::fcc_fault) == FaultClass::xmtr))
	   {
		   float x, y;
		   char *mode = "";
		   GetButtonPos (12, &x, &y);
		   display->TextCenter(x, y + display->TextHeight(), mode);
		   //MI add in MasterArm state and missile status
		   if(SimDriver.playerEntity->Sms->MasterArm() == SMSBaseClass::Sim)
			   mode = "SIM";
		   else if(SimDriver.playerEntity->Sms->MasterArm() == SMSBaseClass::Arm)
		   {
			   if(SimDriver.playerEntity->Sms->curWeapon && SimDriver.playerEntity->Sms->CurStationOK())
				   mode = "RDY";
			   else if(SimDriver.playerEntity->Sms->curWeapon && !SimDriver.playerEntity->Sms->CurStationOK())
				   mode = "MAL";
		   }
		   if(SimDriver.playerEntity->RFState == 1)
			   mode = "QUIET";
		   else if(SimDriver.playerEntity->RFState == 2)
			   mode = "SILENT";
		   
		   display->TextCenter(x, y + display->TextHeight(), mode);
	   }
	   else
		   if (IsAADclt(Fmt2) == FALSE) DefaultLabel(12);
   }
   else {
       LabelButton (10, "DCLT", NULL, IsSet (AADecluttered));
       LabelButton (11, "SMS");
       LabelButton (13, "MENU", NULL, 1);//me123
       LabelButton (14, "SWAP");
   }
}

void RadarDopplerClass::TargetToXY(SimObjectLocalData *localData, int hist, 
				   float drange, float *x, float *y)
{
		// JB 010730 cursor position gets messed up.
 		if (localData->rdrX[hist] == 0 && localData->rdrY[hist] == 0)
			return;

    float az;
    /*-----------------------------------------------*/
    /* azimuth corrected for ownship heading changes */
    /*-----------------------------------------------*/
    az = localData->rdrX[hist] + (localData->rdrHd[hist] - platform->Yaw());
    az = RES180(az);
    
    /*---------------------------------*/
    /* find x and y location on bscope */
    /*---------------------------------*/
    *x = disDeg*az;
    *y = -AZL + 2.0F*AZL*(localData->rdrY[hist] / drange);
}


void RadarDopplerClass::DrawNCTR(bool TWS)
{
	// 2002-02-25 ADDED BY S.G. If not capable of handling NCTR, then don't do it!
	if (!(radarData->flag & RAD_NCTR))
		return;
	// END OF ADDED SECTION 2002-02-25

    display->SetColor(GetMfdColor (MFD_DEFAULT));
#if 0 // old code
    display->Tri(0.0F, 0.8F, 0.0F, 0.75F, nctrData*NCTR_BAR_WIDTH, 0.8F);
    display->Tri(0.0F, 0.75F, nctrData*NCTR_BAR_WIDTH, 0.75F, nctrData*NCTR_BAR_WIDTH, 0.8F);
#else
    // JPO start with some checks.
    ShiAssert(FALSE == F4IsBadReadPtr(lockedTarget, sizeof *lockedTarget));
    ShiAssert(lockedTarget->BaseData() != NULL);

    // Marco Edit - here we display our NCTR data INSTEAD of the bar
    // This was grabbed from Radar360.cpp (Easy Avionics radar/*	  
    Falcon4EntityClassType *classPtr;
    char string[24];
    classPtr = (Falcon4EntityClassType*)lockedTarget->BaseData()->EntityType();
    ShiAssert(FALSE == F4IsBadReadPtr(classPtr, sizeof *classPtr));

	// NCTR strength > 2.5 for TWS, 1.9 for NCTR
    if (lockedTarget->BaseData()->IsSim() && 
		!((SimBaseClass*)lockedTarget->BaseData())->IsExploding() && 
		((!TWS && ReturnStrength(lockedTarget) > 1.9f)
		|| (TWS && ReturnStrength(lockedTarget) > 2.5f)))
	{
		if(
#if 1 // original marco
			(lockedTargetData->ataFrom * RTD > -25.0 && 
			lockedTargetData->ataFrom * RTD < 25.0) &&
			(lockedTargetData->elFrom * RTD > -25.0 && 
			lockedTargetData->elFrom * RTD < 25.0) )
#else // me123 suggestion
			lockedTargetData->ataFrom*RTD < 45.0f)
#endif
		{
			// 5 = DTYPE_VEHICLE
			if (classPtr->dataType == 5) 
			{
				ShiAssert(FALSE == F4IsBadReadPtr(classPtr->dataPtr, sizeof(VehicleClassDataType)));
				sprintf (string, "%.4s", &((VehicleClassDataType*)(classPtr->dataPtr))->Name[15]);
				// 2002-02-25 ADDED BY S.G. If we get the type of the vehicle, then we've identified it (this code is not CPU intensive, even if ran on every frame)
				CampBaseClass *campBase;
				if (lockedTarget->BaseData()->IsSim())
					campBase = ((SimBaseClass *)lockedTarget->BaseData())->GetCampaignObject();
				else
					campBase = ((CampBaseClass *)lockedTarget->BaseData());
				campBase->SetSpotted(platform->GetTeam(), TheCampaign.CurrentTime, 1);
			// END OF ADDED SECTION 2002-02-25
			}
			else
				sprintf(string, "%s", "UNKN");
		}
		else
			sprintf(string, "%s", "UNKN");
	}
	else 
	{
		sprintf (string, "%s", "WAIT");
    }
    ShiAssert( strlen(string) < sizeof(string) );
    display->TextCenter(0.0F, 0.75F, string);
    // End Marco Edit
#endif
}
//MI
void RadarDopplerClass::AGRangingDisplay(void)
{
	//if we have a lock, we loose it here
	DropGMTrack();
	float cX, cY;
   
    GetCursorPosition (&cX, &cY);

	FireControlComputer* FCC = ((SimVehicleClass*)platform)->GetFCC();
	if(!FCC)
		return;
	display->SetColor(GetMfdColor(MFD_LABELS));
    LabelButton(0, "AGR");
	if(!IsSOI())
		display->TextCenter(0.0F, 0.4F, "NOT SOI");
	else
		DrawBorder();
	DrawWaterline();
	display->SetColor(GetMfdColor (MFD_ATTACK_STEERING_CUE));
	display->AdjustOriginInViewport(FCC->groundPipperAz, FCC->groundPipperEl);
	// draw maltese cross
	static const float MaxPos = 0.045f;
	static const float MinPos = 0.015f;
	static const float OverLap = 0.0075f;
	display->Tri(-MinPos, -MaxPos,  MinPos, -MaxPos, 0, OverLap); // triangle one down
	display->Tri(-MinPos,  MaxPos,  MinPos,  MaxPos, 0, -OverLap); // triangle two up
	display->Tri(-MaxPos, -MinPos, -MaxPos,  MinPos, OverLap, 0); // triangle three left
	display->Tri( MaxPos, -MinPos,  MaxPos,  MinPos, -OverLap, 0); // triangle four right
	display->AdjustOriginInViewport(-FCC->groundPipperAz, -FCC->groundPipperEl);
	
	display->SetColor(GetMfdColor(MFD_LABELS));
	LabelButton(3, "OVRD", NULL, !isEmitting);
	
	LabelButton(4, "CNTL", NULL, IsSet(CtlMode));

	if (IsSet(MenuMode|CtlMode)) 
	{
		MENUDisplay();
		return;
	}
	LabelButton(5, "BARO");
	LabelButton(8, "CZ");
	LabelButton(19,"","10");
	LabelButton(17,"A","1");
	if (mode != OFF) 
	{
		LabelButton(7, "SP");
		LabelButton(9, "STP");
	}

	DrawAzElTicks();
	DrawScanMarkers();
	display->SetColor(GetMfdColor(MFD_LABELS));
    AABottomRow ();
	scanDir = ScanNone;
	beamAz = FCC->groundPipperAz;
	beamEl = FCC->groundPipperEl;

	display->SetColor(GetMfdColor(MFD_BULLSEYE));
	if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp &&
		OTWDriver.pCockpitManager->mpIcp->ShowBullseyeInfo)
		DrawBullseyeCircle(display, cX, cY);
	else
		DrawReference(display);
}
//MI
void RadarDopplerClass::DrawReference(VirtualDisplay* display)
{
	AircraftClass* self = SimDriver.playerEntity;
	if(!self)
		return;

	const float yref = -0.8f;
    const float xref = -0.9f;
    const float deltax[] = { 0.05f,  0.02f, 0.02f, 0.005f,  0.02f, 0.02f, 0.05f };
    const float deltay[] = { 0.00f, -0.05f, 0.05f, 0.00f, -0.05f, 0.05f, 0.00f };
    const float RefAngle = 45.0f * DTR;
    float x = xref, y = yref;

    float offset = 0.0f;

    ShiAssert(self != NULL && TheHud != NULL);
    if (TheHud->waypointValid) 
	{
		offset = TheHud->waypointBearing / RefAngle;
    }
    FireControlComputer *FCC = self->GetFCC();
    ShiAssert(FCC != NULL);
    switch (FCC->GetMasterMode() )
    {
		case FireControlComputer::AirGroundBomb:
		case FireControlComputer::AirGroundLaser:
		case FireControlComputer::AirGroundCamera:
		if (FCC->inRange) 
		{
			offset = FCC->airGroundBearing / RefAngle;
		}
		break;
		case FireControlComputer::Dogfight:
		case FireControlComputer::MissileOverride:
		case (FireControlComputer::Gun && FCC->GetSubMode() != FireControlComputer::STRAF):
		{
			if(lockedTarget && lockedTarget->BaseData() && !FCC->IsAGMasterMode())
			{ 
				float   dx, dy, xPos,tgtx, yPos = 0.0F;
				vector  collPoint;
				TargetToXY(lockedTargetData, 0, tdisplayRange, &tgtx, &yPos);
				if (FindCollisionPoint ((SimBaseClass*)lockedTarget->BaseData(), platform, &collPoint))
				{// me123 status ok. Looks like collision point is returned in World Coords.  We need to
					// make it relative to ownship so subtract out ownship pos 1st....

					//me123 addet next three lines
					collPoint.x -= platform->XPos();
					collPoint.y -= platform->YPos();
					collPoint.z -= platform->ZPos();

					dx = collPoint.x;
					dy = collPoint.y;



					xPos = (float)atan2 (dy,dx) - platform->Yaw();

					if (xPos > (180.0F * DTR))
						xPos -= 360.0F * DTR;
					else if (xPos < (-180.0F * DTR))
						xPos += 360.0F * DTR;
				}

				/*---------------------------*/
				/* steering point screen x,y */
				/*---------------------------*/
				if (fabs(xPos) < (60.0F * DTR))
				{
					xPos = xPos / (60.0F * DTR);
					xPos += tgtx;
					
					offset = xPos;
					offset -= TargetAz (platform, lockedTarget->BaseData()->XPos(),
					lockedTarget->BaseData()->YPos());
				}
				else if(xPos > (60 * DTR))
					offset = 1;
				else
					offset = -1;
			}
		}
		break;
		case FireControlComputer::Nav:
		case FireControlComputer::ILS:
		default:	//Catch all the other stuff
			if (TheHud->waypointValid) 
			{ 
				offset = TheHud->waypointBearing / RefAngle;
			} 
		break;
    }
	
    offset = min ( max (offset, -1.0F), 1.0F);
    for (int i = 0; i < sizeof(deltax)/sizeof(deltax[0]); i++) 
	{
		display->Line(x, y, x+deltax[i], y+deltay[i]);
		x += deltax[i]; y += deltay[i];
    }
    float xlen = (x - xref);
    x = xref + xlen/2 + offset * xlen/2.0f;
	if(g_bINS)
	{
		if(SimDriver.playerEntity && !SimDriver.playerEntity->INSState(AircraftClass::INS_HSD_STUFF))
			return;
	}
    display->Line(x, yref + 0.086f, x, yref - 0.13f);
}
//MI
void RadarDopplerClass::SetInterogateTimer(int Dir)
{
}
//MI
void RadarDopplerClass::UpdateLOSScan(void)
{
}
//MI
void RadarDopplerClass::GetBuggedIFF(float *x, float *y, int *type)
{
}