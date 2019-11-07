#include "stdhdr.h"
#include "entity.h"
#include "object.h"
#include "sensors.h"
#include "PilotInputs.h"
#include "simmover.h"
#include "object.h"
#include "fsound.h"
#include "soundfx.h"
#include "radarDoppler.h"
#include "fack.h"
#include "aircrft.h"
#include "simdrive.h"
#include "hud.h"
#include "fcc.h"
#include "fakerand.h"

static int didDesignate = FALSE;
static int didDroptargetCmd = FALSE;
// 2001-02-21 MODIFIED BY S.G. APG68_BAR_WIDTH IS 2.2
//static const float APG68_BAR_WIDTH		= (2.0f * DTR);		// Here and in State.cpp
static const float APG68_BAR_WIDTH		= (2.2f * DTR);		// Here and in State.cpp
extern bool g_bMLU;

void RadarDopplerClass::ExecModes(int newDesignate, int newDrop)
{
   designateCmd = newDesignate && IsSOI();
   if (designateCmd)
      SetFlagBit(Designating);
   else
      ClearFlagBit(Designating);
   dropTrackCmd = newDrop;

   /*----------------------*/
   /* reacquisition marker */
   /*----------------------*/
   if (reacqFlag) reacqFlag--; 

   /*---------------------*/
   /* service radar modes */
   /*---------------------*/

   switch (mode)
   {
      /*----------------------------*/
      /* Range While Search         */
      /*----------------------------*/
      case RWS:   // Range While Search
      case LRS:
			RWSMode();
      break;

      case SAM:   // Situational Awareness mode
			SAMMode();
      break;

		case STT:   // Single Target Track
			STTMode();
		break;

		/*-----------------------*/
		/* Auto Aquisition modes */
		/*-----------------------*/
		case ACM_30x20:   // Normal ACM
		case ACM_SLEW:    // Slewable ACM
		case ACM_BORE:    // Boresight ACM
		case ACM_10x60:   // Vertical Search ACM
			ACMMode ();
		break;

		case VS:
			VSMode ();  // Velocity Search Mode
		break;

      case TWS:
         TWSMode();
      break;

      case GM:
      case GMT:
      case SEA:
         GMMode();
      break;
	}

   didDesignate = designateCmd;
   
}

void RadarDopplerClass::RWSMode(void)
{
SimObjectType* rdrObj;
SimObjectLocalData* rdrData;

	/*-----------------------*/
	/* Spotlight / Designate */
	/*-----------------------*/
   if (IsSet(Spotlight) && !designateCmd)
	   SetFlagBit (Designating);
   else
	   ClearFlagBit (Designating);

   if (!IsSet(Spotlight) && designateCmd)
   {
	   lastAzScan = azScan;
	   lastBars = bars;
	   azScan = 10.0F * DTR;
	   bars = 4;
	   ClearSensorTarget();
   }
   if (designateCmd)
	   SetFlagBit(Spotlight);
   else
	   ClearFlagBit(Spotlight);

   /*-------------------*/
   /* check all objects */
   /*-------------------*/
   rdrObj = platform->targetList;
   while (rdrObj)
   {
	   rdrData = rdrObj->localData;

	   /*-------------------------------*/
	   /* check for object in radar FOV */
	   /*-------------------------------*/
	   if (rdrData->painted)
	   {
		   /*----------------------*/
		   /* detection this frame */
		   /*----------------------*/
		   if (rdrData->rdrDetect & 0x10)
		   {
			   AddToHistory(rdrObj, Solid);
		   }
		   /*--------------*/
		   /* no detection */
		   /*--------------*/
		   else
			   ExtrapolateHistory(rdrObj);
	   }

	   /*-------------*/
	   /* Track State */
	   /*-------------*/
// 2002-03-25 MN add a check if the target is in our radar cone - if not, we not even have an UnreliableTrack
// This fixes the AI oscillating target acquisition and losing
	   if ((rdrData->rdrDetect & 0x1f) && (fabs(rdrData->ata) < radarData->ScanHalfAngle))
		   rdrData->sensorState[Radar] = UnreliableTrack;

	   /*--------------------------------*/
	   /* If designating, check for lock */
	   /*--------------------------------*/
	   if (IsSet(Designating) && (mode == RWS || mode == LRS) && rdrObj->BaseData()->Id() == targetUnderCursor)
	   {
		   // Always lock if it is bright green (detected last time around)
		   if (rdrData->rdrDetect & 0x10)
		   {
			   rdrData->rdrDetect = 0x1f;
			   SetSensorTarget(rdrObj);
			   ClearHistory(lockedTarget);
			   AddToHistory(lockedTarget, Track);
			   ClearFlagBit(Designating);
			   ChangeMode (SAM);
			   //   seekerAzCenter = rdrData->az;
			   //   seekerElCenter = rdrData->el;
			   //   beamAz   = 0.0F;
			   //   beamEl   = 0.0F;
			   SetScan();
			   break;
		   }
	   }

	   rdrObj = rdrObj->next;
   }

   /*-----------------------------------*/
   /* No target found, return to search */
   /*-----------------------------------*/
   if (IsSet(Designating))
   {
	   azScan = lastAzScan;
	   bars = lastBars;
   }
}

void RadarDopplerClass::SAMMode(void)
{
int i, totHits, dropSAM = FALSE;
unsigned long detect;
float  tmpRange, tmpVal;
SimObjectType* rdrObj;
static bool justdidSTT = FALSE;
	// Drop Track, Revert to last mode
	if (lockedTarget == NULL)
	{
		//MI better do what the comment tells us
		//ChangeMode (RWS);
		ChangeMode(prevMode);
		return;
	}

	// Drop immediatly on leaving volume
	if (fabs(lockedTargetData->ata) > MAX_ANT_EL)//me123 ||
		//me123 fabs(lockedTargetData->el) > MAX_ANT_EL)
	{
		dropSAM = TRUE;
	}
	if (oldseekerElCenter && subMode != SAM_AUTO_MODE) 
	{
		seekerElCenter = min ( max (oldseekerElCenter, -MAX_ANT_EL + elScan), MAX_ANT_EL - elScan);
		oldseekerElCenter = 0.0f;
	}

	if (IsSet(STTingTarget))
	{
		justdidSTT = TRUE;
		if (dropSAM)
		{
			float elhack = oldseekerElCenter;
			azScan = lastSAMAzScan;
			bars = lastSAMBars;
			barWidth = APG68_BAR_WIDTH;
			ClearFlagBit(VerticalScan);
			SetFlagBit(HorizontalScan);
			scanDir  = ScanFwd;
			ClearFlagBit (STTingTarget);
			ChangeMode(SAM);
			oldseekerElCenter = elhack;
			seekerElCenter = elhack;

		}
		else
		{	
			STTMode();
		}
	}
	else
	{
		/*-------------------*/
		/* check all objects */
		/*-------------------*/
		rdrObj = platform->targetList;
		while (rdrObj)
		{
			if (rdrObj->BaseData()->Id() == targetUnderCursor && IsSet(Designating) && !didDesignate)
			{
				// Bug a target and go into STT
				if (rdrObj == lockedTarget)
				{
					SetSensorTarget(rdrObj);
					ClearFlagBit(Designating);
					seekerAzCenter = lockedTargetData->az;
					seekerElCenter = lockedTargetData->el;
					beamAz   = 0.0F;
					beamEl   = 0.0F;
					ClearFlagBit (SpaceStabalized);
					beamWidth = radarData->BeamHalfAngle;
					lastSAMAzScan = azScan;
					lastSAMBars = bars;
					azScan = 0.0F * DTR;
					bars = 1;
					barWidth = APG68_BAR_WIDTH;
					ClearFlagBit(VerticalScan);
					SetFlagBit(HorizontalScan);
					scanDir  = ScanNone;
					SetScan();
					SetFlagBit (STTingTarget);
					patternTime = 1;
				}
				else
				{
					// Move sam target to something else.
					// Lock anything detected at last chance
					if (rdrObj->localData->rdrDetect & 0x10)
					{
						if (lockedTarget)
						{
							ClearHistory(lockedTarget);
							AddToHistory(lockedTarget, Solid);
						}
						rdrObj->localData->rdrDetect = 0x1f;
						SetSensorTarget(rdrObj);
						ClearHistory(lockedTarget);
						AddToHistory(lockedTarget, Track);
						ClearFlagBit(Designating);
						//                seekerAzCenter = rdrObj->localData->az;
						//                seekerElCenter = rdrObj->localData->el;
						rdrObj->localData->sensorLoopCount[Radar] = SimLibElapsedTime;
						/*
						beamAz   = 0.0F;
						beamEl   = 0.0F;
						SetFlagBit (STTingTarget);
						patternTime = 1;
						*/
						break;
					}
				}
			}

			/*-------------------------------*/
			/* check for object in radar FOV */
			/*-------------------------------*/
			if (rdrObj->localData->painted)
			{
				/*----------------------*/
				/* detection this frame */
				/*----------------------*/
				if (rdrObj->localData->rdrDetect & 0x10)
				{
					AddToHistory(rdrObj, Solid);
				}
				/*--------------*/
				/* no detection */
				/*--------------*/
				else
					ExtrapolateHistory(rdrObj);
			}

			rdrObj = rdrObj->next;
		}

		/*------------*/
		/* SAM Target */
		/*------------*/
		totHits = 0;
		detect = lockedTargetData->rdrDetect;
		for (i=0; i<5; i++)
		{
			totHits += detect & 0x0001;
			detect = detect >> 1;
		}


		if (lockedTargetData->painted)
		{
			if (lockedTargetData->rdrDetect & 0x10)
			{
				lockedTargetData->sensorState[Radar] = SensorTrack;
				AddToHistory(lockedTarget, Track);
			}
			//         lockedTargetData->rdrSy[1] = None;
		}


		if (subMode == SAM_MANUAL_MODE)
		{
			// Recalculate az limit
			CalcSAMAzLimit();
		}
		else
		{
			oldseekerElCenter = seekerElCenter;//me123
			tmpVal = TargetAz(platform, lockedTarget);
			seekerAzCenter = min ( max (tmpVal ,-MAX_ANT_EL + azScan), MAX_ANT_EL - azScan);
			tmpVal = TargetEl(platform, lockedTarget);
			if (!oldseekerElCenter) 
				seekerElCenter = min ( max (tmpVal, -MAX_ANT_EL + elScan), MAX_ANT_EL - elScan);
		}

		/*------------------*/
		/* Auto Range Scale */
		/*------------------*/
		//me123 don't autorange in SAM MODE
		//	   if (lockedTargetData->range > 0.9F * tdisplayRange &&
		//		    curRangeIdx < NUM_RANGES - 1)
		//		   rangeChangeCmd = 1;
		//	   else if (lockedTargetData->range < 0.4F * tdisplayRange &&
		//		    curRangeIdx > 0)
		//		   rangeChangeCmd = -1;
		if (!dropTrackCmd) 
			justdidSTT = FALSE;
		if (totHits < HITS_FOR_TRACK|| dropSAM|| (dropTrackCmd && !justdidSTT)  )
		{
			if (platform == SimDriver.playerEntity && ((AircraftClass*)platform)->AutopilotType() == AircraftClass::CombatAP)
			{// nothing....this causes lock and brakelocs stream in mp
			}
			else 
			{
				rangeChangeCmd = 0;
				reacqEl = lockedTargetData->el;
				reacqFlag = (int)(ReacqusitionCount / SEC_TO_MSEC * SimLibMajorFrameRate);
				ClearHistory(lockedTarget);
				tmpRange = displayRange;
				//MI
				//ChangeMode (RWS);
				ChangeMode (prevMode);
				azScan = rwsAzs[rwsAzIdx];
				displayAzScan = rwsAzs[rwsAzIdx];
				bars = rwsBars[rwsBarIdx];
				displayRange = tmpRange;
				tdisplayRange = displayRange * NM_TO_FT;
				SetScan();
				ClearSensorTarget();
			}		   
		}
   }
}

void RadarDopplerClass::TWSMode(void)
{
int i, totHits;
unsigned long detect;
float  tmpVal;
SimObjectType* rdrObj;
SimObjectLocalData* rdrData;
static bool tgtenteredcursor = FALSE;
static bool justdidSTT = FALSE;
   // No TWS if sngl failure
   if (platform == SimDriver.playerEntity)
   {
	   if (((AircraftClass*)platform)->mFaults->GetFault(FaultClass::fcc_fault) == FaultClass::sngl)
	   {
		   ClearSensorTarget();
		   return;
	   }
   }

   if (oldseekerElCenter && !targetUnderCursor) //me123
   {
	   seekerElCenter = min ( max (oldseekerElCenter, -MAX_ANT_EL + elScan), MAX_ANT_EL - elScan);
	   oldseekerElCenter = 0.0f;
   }
   if (IsSet(STTingTarget))
   {
	   justdidSTT = TRUE;
	   STTMode();
	   //return;
   }

   //me123 addet spotlight to tws
   /*-----------------------*/
   /* Spotlight / Designate */
   /*-----------------------*/
   if (IsSet(Spotlight) && !designateCmd)
	      SetFlagBit (Designating);
   else
	      ClearFlagBit (Designating);

   if (!IsSet(Spotlight) && designateCmd)
   {
	   if (!tgtenteredcursor)
	   {
		   lastAzScan = azScan;
		   lastBars = bars;
		   azScan = 10.0F * DTR;
		   bars = 4;
		   ClearSensorTarget();
		   ClearFlagBit (STTingTarget);
	   }
	   else if (lockedTarget && lockedTarget->BaseData()->Id() == targetUnderCursor)
	   {
		   ClearFlagBit(Designating);
		   SetSensorTarget(lockedTarget);
		   seekerAzCenter = lockedTargetData->az;
		   seekerElCenter = lockedTargetData->el;
		   beamAz   = 0.0F;
		   beamEl   = 0.0F;
		   //			      ClearFlagBit (SpaceStabalized);
		   beamWidth = radarData->BeamHalfAngle;
		   lastSAMAzScan = azScan;
		   lastSAMBars = bars;
		   azScan = 0.0F * DTR;
		   bars = 1;
		   barWidth = APG68_BAR_WIDTH;
		   ClearFlagBit(VerticalScan);
		   SetFlagBit(HorizontalScan);
		   scanDir  = ScanNone;
		   SetScan();
		   SetFlagBit (STTingTarget);
		   patternTime = 1;
	   }
   }

   if (designateCmd)
	   SetFlagBit(Spotlight);
   else
	   ClearFlagBit(Spotlight);
   
   if (IsSet(Designating))
   {
	   azScan = lastAzScan;
	   bars = lastBars;
   }
   // max 30 az when tgt under cursor or "locked" target
   if (!IsSet(Spotlight) && !IsSet(Designating))
   {

	   if (targetUnderCursor || (lockedTargetData && !g_bMLU))
	   {
		   if (azScan >30.0F * DTR)
		   {
			   lastAzScan = azScan;
			   lastBars = bars;
			   azScan = 30.0F * DTR;
			   tgtenteredcursor = TRUE;
		   }
	   }
	   else if (tgtenteredcursor && lastAzScan >30.0F * DTR)
	   {
		   azScan = lastAzScan;
		   tgtenteredcursor = FALSE;
	   }
   }

   if (lasttargetUnderCursor)
   {
	   int attach = FALSE;
	   //check if the attached target is still in the targetlist
	   rdrObj = platform->targetList;
	   while (rdrObj && !attach)
	   {
		   if (rdrObj == lasttargetUnderCursor)
			   attach = TRUE;
		   rdrObj = rdrObj->next;
	   }

	   if (attach && SimDriver.playerEntity && SimDriver.playerEntity->FCC && SimDriver.playerEntity->FCC->cursorXCmd == 0 && SimDriver.playerEntity->FCC->cursorYCmd == 0)
	   {//me123 attach the cursor
		   TargetToXY(lasttargetUnderCursor->localData, 0, tdisplayRange, &cursorX, &cursorY);
		   if (!oldseekerElCenter) 
			   oldseekerElCenter = seekerElCenter;//me123
		   tmpVal = TargetAz(platform, lasttargetUnderCursor);
		   seekerAzCenter = min ( max (tmpVal ,-MAX_ANT_EL + azScan), MAX_ANT_EL - azScan);
		   tmpVal = TargetEl(platform, lasttargetUnderCursor);
		   seekerElCenter = min ( max (tmpVal, -MAX_ANT_EL + elScan), MAX_ANT_EL - elScan);
	   }
	   else 
		   lasttargetUnderCursor = NULL;  
   }
   

   /*-------------------*/
   /* check all objects */
   /*-------------------*/
   rdrObj = platform->targetList;
   while (rdrObj)
   {
	   rdrData = rdrObj->localData;

	   /*-------------------------------*/
	   /* check for target under cursor */
	   /*-------------------------------*/
	   if (rdrObj->BaseData()->Id() == targetUnderCursor)
	   {
		   if (!lasttargetUnderCursor)lasttargetUnderCursor = rdrObj;

		   totHits = 0;

		   detect = rdrData->rdrDetect;
		   for (i=0; i<5; i++)
		   {
			   totHits += detect & 0x0001;
			   detect = detect >> 1;
		   }

		   if (IsSet(Designating) && totHits > HITS_FOR_TRACK)
		   {
			   SetSensorTarget(rdrObj);
			   ClearFlagBit(Designating);
		   }
	   }

	   /*-------------------------------*/
	   /* check for object in radar FOV */
	   /*-------------------------------*/
	   if (rdrData->painted)
	   {
		   // count Hits
		   totHits = 0;
		   detect = rdrData->rdrDetect;
		   for (i=0; i<5; i++)
		   {
			   totHits += detect & 0x0001;
			   detect = detect >> 1;
		   }

		   /*----------------------*/
		   /* detection this frame */
		   /*----------------------*/
		   if (rdrData->rdrDetect & 0x10)
		   {
			   if (totHits > HITS_FOR_TRACK)
			   {
				   AddToHistory(rdrObj, Track);
			   }
			   else
			   {
				   AddToHistory(rdrObj, Solid);
			   }
		   }
	   }
	   rdrObj = rdrObj->next;
   }

   /*------------*/
   /* SAM Target */
   /*------------*/
   if (lockedTargetData)
   {	
	   if (lockedTargetData->painted)
	   {
		   if (lockedTargetData->rdrDetect & 0x10)
		   {
			   lockedTargetData->sensorState[Radar] = SensorTrack;
			   AddToHistory(lockedTarget, Bug);
		   }
	   }
	   static int test = 10;
	   test = 20;
	   //if (!oldseekerElCenter) oldseekerElCenter = seekerElCenter;//me123
	   tmpVal = TargetAz(platform, lockedTarget);
	   seekerAzCenter = min ( max (tmpVal ,-MAX_ANT_EL + azScan), MAX_ANT_EL - azScan);
	   tmpVal = TargetEl(platform, lockedTarget);
	   if (!g_bMLU) 
		   seekerElCenter = min ( max (tmpVal, -MAX_ANT_EL + elScan), MAX_ANT_EL - elScan);

	   /*------------------*/
	   /* Auto Range Scale */
	   /*------------------*/
	   if (lockedTargetData->range > 0.9F * tdisplayRange &&
		   curRangeIdx < NUM_RANGES - 1)
		   rangeChangeCmd = 1;

	   totHits = 0;
	   detect = lockedTargetData->rdrDetect;
	   for (i=0; i<5; i++)
	   {
		   totHits += detect & 0x0001;
		   detect = detect >> 1;
	   }
	   if (lockedTarget)
	   {
		   //me123 gimbal check addet
		   // Drop lock if the guy is outside our radar cone
		   if ((fabs( lockedTarget->localData->az ) > radarData->ScanHalfAngle) ||
			   (fabs( lockedTarget->localData->el ) > radarData->ScanHalfAngle) ) {
			   ClearSensorTarget();
		   }
	   }

	   if (!dropTrackCmd) 
		   justdidSTT = FALSE;
	   if (!IsSet(STTingTarget) && ((dropTrackCmd && !justdidSTT) || totHits < HITS_FOR_TRACK))
	   {
		   rangeChangeCmd = 0;
		   if (lockedTargetData) reacqEl = lockedTargetData->el;
		   reacqFlag = (int)(ReacqusitionCount / SEC_TO_MSEC * SimLibMajorFrameRate);
		   ClearSensorTarget();
		   //me123 dont do this
		   //         seekerAzCenter = 0.0F;
		   //         seekerElCenter = 0.0F;
	   }
   }
}

void RadarDopplerClass::STTMode(void)//me123 status test. multible changes
{
int i, totHits;
unsigned long detect;
static bool diddroptrack= FALSE;

	if (oldseekerElCenter && !lockedTargetData) //me123
	{
		seekerElCenter = min ( max (oldseekerElCenter, -MAX_ANT_EL + elScan), MAX_ANT_EL - elScan);
		oldseekerElCenter = 0.0f;
	}

	if (!lockedTarget)
	{
		ClearFlagBit(STTingTarget);
		if (mode != STT)
			ChangeMode (mode);
		else
			ChangeMode (prevMode);
		return;
	}
	else
	{ //me123 gimbal check addet

		// Drop lock if the guy is outside our radar cone
		if ((fabs( lockedTarget->localData->az ) > radarData->ScanHalfAngle) ||
			(fabs( lockedTarget->localData->el ) > radarData->ScanHalfAngle) ) {
			dropTrackCmd = TRUE;
		}
	}

	if (!lockedTargetData)
		return;
	
	if (!g_bMLU && (dropTrackCmd && !didDesignate && mode == SAM) ||
		g_bMLU && dropTrackCmd && !diddroptrack)//me123
	{
		reacqFlag = (int)(ReacqusitionCount / SEC_TO_MSEC * SimLibMajorFrameRate);
		reacqEl = lockedTargetData->el;
		//ClearHistory(lockedTarget);
		azScan = rwsAzs[rwsAzIdx];
		displayAzScan = rwsAzs[rwsAzIdx];
		bars = rwsBars[rwsBarIdx];
		barWidth = APG68_BAR_WIDTH;
		ClearFlagBit(VerticalScan);
		SetFlagBit(HorizontalScan);
		scanDir  = ScanFwd;
		SetScan();
		ClearFlagBit(STTingTarget);
		diddroptrack = TRUE;
		return;
	}
	diddroptrack = FALSE;
	if (!oldseekerElCenter) 
		oldseekerElCenter = seekerElCenter;//me123

	seekerAzCenter = max ( min (lockedTargetData->az, MAX_ANT_EL), -MAX_ANT_EL);
	seekerElCenter = max ( min (lockedTargetData->el, MAX_ANT_EL), -MAX_ANT_EL);
	bars = 1;
	azScan = 0.0F;

	/*--------------------------*/
	/* Hits to maintain a Track */
	/*--------------------------*/
	totHits = 0;
	detect = lockedTargetData->rdrDetect;
	for (i=0; i<5; i++)
	{
		totHits += detect & 0x0001;
		detect = detect >> 1;
	}

	if (lockedTargetData->painted && totHits > HITS_FOR_TRACK)
	{
		AddToHistory(lockedTarget, Track);
		lockedTargetData->sensorState[Radar] = SensorTrack;
	}

	/*------------------*/
	/* Auto Range Scale */
	/*------------------*/
	if (mode != VS)
	{
		if (lockedTargetData->range > 0.9F * tdisplayRange && curRangeIdx < NUM_RANGES - 1)
			rangeChangeCmd = 1;
		else if (lockedTargetData->range < 0.4F * tdisplayRange && curRangeIdx > 0)
			rangeChangeCmd = -1;
	}

	if (totHits < HITS_FOR_TRACK || dropTrackCmd )
	{
		ExtrapolateHistory(lockedTarget);
		
		if (((SimLibElapsedTime - lockedTarget->localData->rdrLastHit) > radarData->CoastTime) || dropTrackCmd )
		{
			//me123	rangeChangeCmd = 0;
			reacqFlag = (int)(ReacqusitionCount / SEC_TO_MSEC * SimLibMajorFrameRate);
			reacqEl = lockedTargetData->el;
			ClearHistory(lockedTarget);

			if (mode == ACM_30x20 || mode == ACM_SLEW
				|| mode == ACM_BORE || mode == ACM_10x60 || mode == VS)
			{
				lockedTarget->localData->rdrDetect = 0;
				ClearSensorTarget();
				ChangeMode (mode);
			}
			else if (mode == SAM)
			{
				azScan = rwsAzs[rwsAzIdx];
				displayAzScan = rwsAzs[rwsAzIdx];
				bars = rwsBars[rwsBarIdx];
				barWidth = APG68_BAR_WIDTH;
				ClearFlagBit(VerticalScan);
				SetFlagBit(HorizontalScan);
				scanDir  = ScanFwd;
				//MI
				ChangeMode(prevMode);
			}
			else
			{
			float elhack = oldseekerElCenter;
			ChangeMode (prevMode);
			oldseekerElCenter = elhack;
			seekerElCenter = elhack;
			}
			SetScan();
			ClearFlagBit(STTingTarget);
		}
	}
}

void RadarDopplerClass::ACMMode (void)
{
int i, totHits;
unsigned long detect;
SimObjectType* rdrObj = platform->targetList;

   /*----------*/
   /* Locked ? */
   /*----------*/
   if (lockedTarget)
   {
      TheHud->HudData.Clear(HudDataType::RadarSlew);
      TheHud->HudData.Clear(HudDataType::RadarBoresight);
      TheHud->HudData.Clear(HudDataType::RadarVertical);
      SetFlagBit (STTingTarget);
      STTMode();
   }
   else
   {
	   if (IsSet(STTingTarget))
	   {
		   ClearFlagBit (STTingTarget);
		   ChangeMode (mode);
	   }

	   if (scanDir == ScanNone)
		   scanDir = ScanFwd;

	   if (mode == ACM_BORE)
	   {
		   seekerAzCenter = 0.0F;
		   seekerElCenter = -3.0F * DTR;;
	   }

	   /*-------------------*/
	   /* check all objects */
	   /*-------------------*/
	   while (rdrObj)
	   {

		   /*--------------------------*/
		   /* Hits to maintain a Track */
		   /*--------------------------*/
		   totHits = 0;
		   detect = rdrObj->localData->rdrDetect;
		   for (i=0; i<5; i++)
		   {
			   totHits += detect & 0x0001;
			   detect = detect >> 1;
		   }

		   if (totHits > HITS_FOR_LOCK / 2 &&
			   rdrObj->localData->range < tdisplayRange && IsEmitting() && rdrObj->localData->painted)
		   {
			   // Play the lock message
			   F4SoundFXSetDist(SFX_BB_LOCK, 0, 0.0f, 1.0f );
			   SetSensorTarget(rdrObj);
			   ClearFlagBit(Designating);
			   seekerAzCenter = lockedTargetData->az;
			   seekerElCenter = lockedTargetData->el;
			   beamAz   = 0.0F;
			   beamEl   = 0.0F;
			   ClearFlagBit (SpaceStabalized);
			   beamWidth = radarData->BeamHalfAngle;
			   azScan = 0.0F * DTR;
			   bars = 1;
			   barWidth = APG68_BAR_WIDTH;
			   ClearFlagBit(VerticalScan);
			   SetFlagBit(HorizontalScan);
			   scanDir  = ScanNone;
			   SetScan();
			   patternTime = 1;
			   SetFlagBit (STTingTarget);
			   break;
		   }
		   else
			   rdrObj->localData->sensorState[Radar] = NoTrack;

		   rdrObj = rdrObj->next;
	   }

	   if (mode == ACM_BORE)
	   {
		   TheHud->HudData.Set(HudDataType::RadarBoresight);
	   }
	   else if (mode == ACM_SLEW)
	   {
		   TheHud->HudData.Set(HudDataType::RadarSlew);
	   }
	   else if (mode == ACM_10x60)
	   {
		   TheHud->HudData.Set(HudDataType::RadarVertical);
	   }
   }

   /*-------------------------*/// me123 so designate doesn't drop target if locked
   /* Select correct ACM Mode */// and drop target command returns to search if we have a lock, if we dont have a lock  it changes
   /*-------------------------*/// to 20/30 from all acm modes exept in 20/30 it goes to 10/60
   if (designateCmd && !lockedTarget) // don't drop the targer if it's locked me123
   {	
	   ChangeMode (ACM_BORE);
	   SetScan();
	   
   }
   else if (!lockedTarget && mode != ACM_SLEW && 
	   SimDriver.playerEntity && // JB 010113 CTD fix
	   (SimDriver.playerEntity->FCC->cursorYCmd != 0 ||
	   SimDriver.playerEntity->FCC->cursorXCmd !=0))
   {
	   ChangeMode (ACM_SLEW);  
	   SetScan();
   }
   else if (dropTrackCmd)
   {
	   if (lockedTarget)
	   {
		   ClearSensorTarget();  // me123 brake lock if locked
		   ChangeMode (mode);
		   SetScan();
		   lockedTarget = NULL;//me123
	   }
	   else if (!lockedTarget && !didDroptargetCmd)// me123 this toggles alright now, but way too fast. status "bad"
	   {	
		   //MI ACM Mode fix
#if 0
		   if (mode == ACM_10x60 || mode == ACM_BORE || mode == ACM_SLEW) // me123 status ok. if not locked change mode acm mode 
		   {
			   ChangeMode (ACM_30x20);  
			   SetScan();
			   ClearFlagBit(dropTrackCmd);
			   didDroptargetCmd = TRUE;
		   }
		   else	
		   {
			   ChangeMode (ACM_10x60); 
			   SetScan();
			   ClearFlagBit(dropTrackCmd);
			   didDroptargetCmd = TRUE;
		   }
#else
		   SetScan();
		   ClearFlagBit(dropTrackCmd);
		   didDroptargetCmd = TRUE;
#endif
	   }
   }
   else 
   {
	   didDroptargetCmd = FALSE;
   }
}

void RadarDopplerClass::VSMode (void)
{
int i, totHits;
unsigned long detect;
float tmpRange;
SimObjectType* rdrObj;
SimObjectLocalData* rdrData;

   if (lockedTarget)
   {
      STTMode();
   }
   else
   {
	   /*-----------------------*/
	   /* Spotlight / Designate */
	   /*-----------------------*/
	   if (IsSet(Spotlight) && !designateCmd)
		   SetFlagBit (Designating);
	   else
		   ClearFlagBit (Designating);

	   if (!IsSet(Spotlight) && designateCmd)
	   {
		   lastAzScan = azScan;
		   lastBars = bars;
		   azScan = 10.0F * DTR;
		   bars = 4;
		   ClearSensorTarget();
	   }

	   if (designateCmd)
		   SetFlagBit(Spotlight);
	   else
		   ClearFlagBit(Spotlight);

	   /*-------------------*/
	   /* check all objects */
	   /*-------------------*/
	   rdrObj = platform->targetList;
	   while (rdrObj)
	   {
		   rdrData = rdrObj->localData;

		   /*-------------------------------*/
		   /* check for object in radar FOV */
		   /*-------------------------------*/
		   if (rdrData->painted)
		   {
			   /*----------------------*/
			   /* detection this frame */
			   /*----------------------*/
			   if (rdrData->rdrDetect & 0x10 && rdrData->rangedot < 0.0F)
			   {
				   tmpRange = rdrData->range;
				   rdrData->range = -rdrData->rangedot * FTPSEC_TO_KNOTS;
				   SetHistory(rdrObj, Det);
				   rdrData->range = tmpRange;
			   }
			   /*--------------*/
			   /* no detection */
			   /*--------------*/
			   else
				   AddToHistory(rdrObj, None);
		   }

		   /*--------------------------------*/
		   /* If designating, check for lock */
		   /*--------------------------------*/
		   if (IsSet(Designating))
		   {
			   totHits = 0;
			   detect = rdrData->rdrDetect;
			   for (i=0; i<5; i++)
			   {
				   totHits += detect & 0x0001;
				   detect = detect >> 1;
			   }

			   if (totHits >= HITS_FOR_LOCK &&
				   IsUnderVSCursor(rdrObj, platform->Yaw()))
			   {
				   SetSensorTarget(rdrObj);
				   ClearFlagBit(Designating);
				   seekerAzCenter = lockedTargetData->az;
				   seekerElCenter = lockedTargetData->el;
				   beamAz   = 0.0F;
				   beamEl   = 0.0F;
				   //			      ClearFlagBit (SpaceStabalized);
				   beamWidth = radarData->BeamHalfAngle;
				   azScan = 0.0F * DTR;
				   bars = 1;
				   barWidth = APG68_BAR_WIDTH;
				   ClearFlagBit(VerticalScan);
				   SetFlagBit(HorizontalScan);
				   scanDir  = ScanNone;
				   SetScan();
				   patternTime = 1;
				   SetFlagBit (STTingTarget);
				   break;
			   }
		   }

		   rdrObj = rdrObj->next;
	   }

	   /*-----------------------------------*/
	   /* No target found, return to search */
	   /*-----------------------------------*/
	   if (IsSet(Designating))
	   {
		   azScan = lastAzScan;
		   bars = lastBars;
	   }
   }
}

void RadarDopplerClass::AddToHistory(SimObjectType* ptr, int sy)
{
int i;
SimObjectLocalData* rdrData = ptr->localData;

   rdrData->aspect = 180.0F * DTR - rdrData->ataFrom;      /* target aspect  */
   if (rdrData->aspect > 180.0F * DTR)
      rdrData->aspect -= 360.0F * DTR;

   for (i=NUM_RADAR_HISTORY-1; i>0; i--)
   {
      rdrData->rdrX[i]  = rdrData->rdrX[i-1];
      rdrData->rdrY[i]  = rdrData->rdrY[i-1];
      rdrData->rdrHd[i] = rdrData->rdrHd[i-1];
      rdrData->rdrSy[i] = rdrData->rdrSy[i-1];
   }

   rdrData->rdrX[0]  = TargetAz (platform, ptr);
   rdrData->rdrY[0]  = rdrData->range;
   // if its jamming and we can't burn through - its a guess where it is.
   if (ptr->BaseData()->IsSPJamming() && ReturnStrength(ptr) < 1.0f) {
       float delta = rdrData->range/10.0f; // range may be out by up to 1/10th
       rdrData->rdrY[0] += delta*PRANDFloat(); // +/- the delta
   }
   rdrData->rdrHd[0] = platform->Yaw();
   rdrData->rdrSy[0] = sy;

   if (sy != None)
   {
      rdrData->rdrLastHit  = SimLibElapsedTime;
//      UpdateObjectData(ptr);
   }
}

void RadarDopplerClass::ClearHistory(SimObjectType* ptr)
{
int i;
SimObjectLocalData* rdrData = ptr->localData;

	rdrData->aspect = 0.0F;

	for (i=0; i<NUM_RADAR_HISTORY; i++)
	{
		rdrData->rdrX[i]  = 0.0F;
		rdrData->rdrY[i]  = 0.0F;
		rdrData->rdrHd[i] = 0.0F;
		rdrData->rdrSy[i] = 0;
	}
}

void RadarDopplerClass::SlipHistory(SimObjectType* ptr)
{
int i;
SimObjectLocalData* rdrData = ptr->localData;

   for (i=NUM_RADAR_HISTORY-1; i>0; i--)
   {
      rdrData->rdrX[i]  = rdrData->rdrX[i-1];
      rdrData->rdrY[i]  = rdrData->rdrY[i-1];
      rdrData->rdrHd[i] = rdrData->rdrHd[i-1];
      rdrData->rdrSy[i] = rdrData->rdrSy[i-1];
   }
   rdrData->rdrX[0]  = 0.0F;
   rdrData->rdrY[0]  = 0.0F;
   rdrData->rdrHd[0] = 0.0F;
   rdrData->rdrSy[0] = 0;
}

void RadarDopplerClass::ExtrapolateHistory(SimObjectType* ptr)
{
int i;
SimObjectLocalData* rdrData = ptr->localData;

   for (i=NUM_RADAR_HISTORY-1; i>0; i--)
   {
      rdrData->rdrX[i]  = rdrData->rdrX[i-1];
      rdrData->rdrY[i]  = rdrData->rdrY[i-1];
      rdrData->rdrHd[i] = rdrData->rdrHd[i-1];
      rdrData->rdrSy[i] = rdrData->rdrSy[i-1];
   }

   // Change range
   rdrData->rdrY[0]  += (rdrData->rdrY[1] - rdrData->rdrY[2]);

   if ((SimLibElapsedTime - rdrData->rdrLastHit) < TwsFlashTime)
   {
      if (rdrData->rdrSy[0] == Bug)
         rdrData->rdrSy[0] = FlashBug;
      else if (rdrData->rdrSy[0] == Track)
         rdrData->rdrSy[0] = FlashTrack;
   }
}

void RadarDopplerClass::ClearAllHistory (void)
{
SimObjectType* rdrObj = platform->targetList;

   while (rdrObj)
   {
      ClearHistory(rdrObj);
      rdrObj = rdrObj->next;
   }
}

void RadarDopplerClass::SetHistory(SimObjectType* ptr, int sy)
{
int i;
SimObjectLocalData* rdrData = ptr->localData;

   rdrData->aspect = 180.0F * DTR - rdrData->ataFrom;      /* target aspect  */
   if (rdrData->aspect > 180.0F * DTR)
      rdrData->aspect -= 360.0F * DTR;

   for (i=NUM_RADAR_HISTORY-1; i>0; i--)
   {
      rdrData->rdrX[i]  = 0.0F;
      rdrData->rdrY[i]  = 0.0F;
      rdrData->rdrHd[i] = 0.0F;
      rdrData->rdrSy[i] = 0;
   }

   rdrData->rdrX[0]  = TargetAz (platform, ptr);
   rdrData->rdrY[0]  = rdrData->range;
   rdrData->rdrHd[0] = platform->Yaw();
   rdrData->rdrSy[0] = sy;

   if (sy != None)
   {
//	  UpdateObjectData(ptr);
   }
}

