#include "stdhdr.h"
#include "F4Vu.h"
#include "missile.h"
#include "MsgInc\TrackMsg.h"
#include "Graphics\Include\display.h"
#include "simveh.h"
#include "airunit.h"
#include "simdrive.h"
#include "aircrft.h"
#include "fcc.h"
#include "sms.h"
#include "object.h"
#include "team.h"
#include "entity.h"
#include "simmover.h"
#include "soundfx.h"
#include "classtbl.h"
#include "radar.h"
#include "handoff.h"
#include "HarmPod.h"
#include "otwdrive.h"

const float	RANGE_POSITION	= -0.8f; // offset from LHS of MFD
const float	RANGE_MIDPOINT	=  0.45F; // where the text should appear
const float	CURSOR_SIZE	=  0.05f;
extern float g_fCursorSpeed;


int HarmTargetingPod::flash = FALSE;	// Controls flashing display elements


HarmTargetingPod::HarmTargetingPod(int idx, SimMoverClass* self) : RwrClass (idx, self)
{
	sensorType			= HTS;
	cursorX				= 0.0F;
	cursorY				= 0.0F;
	displayRange		= 20;
//	emmitterList		= NULL;
}


HarmTargetingPod::~HarmTargetingPod (void)
{
}


void HarmTargetingPod::LockTargetUnderCursor( void )
{
	GroundListElement	*choice = FindTargetUnderCursor();

	// Will lock the sensor on the related entity (no change if NULL)
	LockListElement( choice );
}


void HarmTargetingPod::BoresightTarget(void)
{
    GroundListElement	*tmpElement;
	GroundListElement	*choice		= NULL;
	float			bestSoFar	= 0.5f;
	float			dx, dy, dz;
	float			cosATA;
	float			displayX, displayY;
	mlTrig			trig;


	// Convienience synonym for the "At" vector of the platform...
	const float atx	= platform->dmx[0][0];
	const float aty	= platform->dmx[0][1];
	const float atz	= platform->dmx[0][2];

	// Set up the trig functions of our current heading
	mlSinCos (&trig, platform->Yaw());

	FireControlComputer* FCC = ((SimVehicleClass*)platform) -> GetFCC();
	// Walk our list looking for the thing in range and nearest our nose
	for (tmpElement	= FCC->GetFirstGroundElement(); tmpElement; tmpElement = tmpElement->GetNext())
	{
	    if (tmpElement -> BaseObject() == NULL) continue;
		// Figure the relative geometry we need
		dx = tmpElement->BaseObject()->XPos() - platform->XPos();
		dy = tmpElement->BaseObject()->YPos() - platform->YPos();
		dz = tmpElement->BaseObject()->ZPos() - platform->ZPos();
		cosATA = (atx*dx + aty*dy + atz*dz) / (float)sqrt(dx*dx+dy*dy+dz*dz);

		// Rotate it into heading up space
		displayX = trig.cos * dy - trig.sin * dy;
		displayY = trig.sin * dx + trig.cos * dx;

		// Scale and shift for display range
		displayX *= FT_TO_NM / displayRange * HTS_DISPLAY_RADIUS;
		displayY *= FT_TO_NM / displayRange * HTS_DISPLAY_RADIUS;
		displayY += HTS_Y_OFFSET;

		// Skip this one if its off screen
		if ((fabs(displayX) > 1.0f) || (fabs(displayY) > 1.0f)) {
			continue;
		}

		// Is it our best candidate so far?
		if (cosATA > bestSoFar) {
			bestSoFar = cosATA;
			choice = tmpElement;
		}
	}

	// Will lock the sensor on the related entity (no change if NULL)
	LockListElement( choice );
}


void HarmTargetingPod::NextTarget(void)
{
	GroundListElement	*tmpElement;
	GroundListElement	*choice		= NULL;
	float			bestSoFar	= 1e6f;
	float			dx, dy;
	float			displayX, displayY;
	mlTrig			trig;
	float			range;
	float			currentRange;
	GroundListElement		*currentElement;


	// Get data on our current target (if any)
	if (lockedTarget) {
		currentElement = FindEmmitter( lockedTarget->BaseData() );
		dx = currentElement->BaseObject()->XPos() - platform->XPos();
		dy = currentElement->BaseObject()->YPos() - platform->YPos();
		currentRange = (float)sqrt( dx*dx + dy*dy );
	}
	else 
	{
		currentElement = NULL;
		currentRange = -1.0f;
	}

	// Set up the trig functions of our current heading
	mlSinCos (&trig, platform->Yaw());

	FireControlComputer* FCC = ((SimVehicleClass*)platform) -> GetFCC();

	// Walk our list looking for the nearest thing in range but farther than our current target
	for (tmpElement	= FCC->GetFirstGroundElement(); tmpElement; tmpElement = tmpElement->GetNext())
	{
	    if (tmpElement -> BaseObject() == NULL) continue;

		// Figure the relative geometry we need
		dx = tmpElement->BaseObject()->XPos() - platform->XPos();
		dy = tmpElement->BaseObject()->YPos() - platform->YPos();

		// Rotate it into heading up space
		displayX = trig.cos * dy - trig.sin * dy;
		displayY = trig.sin * dx + trig.cos * dx;

		// Scale and shift for display range
		displayX *= FT_TO_NM / displayRange * HTS_DISPLAY_RADIUS;
		displayY *= FT_TO_NM / displayRange * HTS_DISPLAY_RADIUS;
		displayY += HTS_Y_OFFSET;

		// Skip this one if its off screen
		if ((fabs(displayX) > 1.0f) || (fabs(displayY) > 1.0f)) {
			continue;
		}

		range = (float)sqrt( dx*dx + dy*dy );

		// Skip it if its too close
		if (range < currentRange) {
			continue;
		}

		// Is it our best candidate so far?
		if (range < bestSoFar) {
			// Don't choose the same one we've already got
			if (tmpElement != currentElement) {
				bestSoFar = range;
				choice = tmpElement;
			}
		}
	} 

	// Will lock the sensor on the related entity (no change if NULL)
	LockListElement( choice );
}


void HarmTargetingPod::PrevTarget(void)
{
	GroundListElement	*tmpElement;
	GroundListElement	*choice		= NULL;
	float			bestSoFar	= -1.0f;
	float			dx, dy;
	float			displayX, displayY;
	mlTrig			trig;
	float			range;
	float			currentRange;
	GroundListElement	*currentElement;

	// Get data on our current target (if any)
	if (lockedTarget) {
		currentElement = FindEmmitter( lockedTarget->BaseData() );
		dx = currentElement->BaseObject()->XPos() - platform->XPos();
		dy = currentElement->BaseObject()->YPos() - platform->YPos();
		currentRange = (float)sqrt( dx*dx + dy*dy );
	}
	else 
	{
		currentElement = NULL;
		currentRange = 1e6f;
	}

	// Set up the trig functions of our current heading
	mlSinCos (&trig, platform->Yaw());

	FireControlComputer* FCC = ((SimVehicleClass*)platform) -> GetFCC();

	// Walk our list looking for the farthest thing in range but nearer than our current target
	for (tmpElement	= FCC->GetFirstGroundElement(); tmpElement; tmpElement = tmpElement->GetNext())
	{
	    if (tmpElement -> BaseObject() == NULL) continue;
		// Figure the relative geometry we need
		dx = tmpElement->BaseObject()->XPos() - platform->XPos();
		dy = tmpElement->BaseObject()->YPos() - platform->YPos();

		// Rotate it into heading up space
		displayX = trig.cos * dy - trig.sin * dy;
		displayY = trig.sin * dx + trig.cos * dx;

		// Scale and shift for display range
		displayX *= FT_TO_NM / displayRange * HTS_DISPLAY_RADIUS;
		displayY *= FT_TO_NM / displayRange * HTS_DISPLAY_RADIUS;
		displayY += HTS_Y_OFFSET;

		// Skip this one if its off screen
		if ((fabs(displayX) > 1.0f) || (fabs(displayY) > 1.0f)) {
			continue;
		}

		range = (float)sqrt( dx*dx + dy*dy );

		// Skip it if its too far
		if (range > currentRange) {
			continue;
		}

		// Is it our best candidate so far?
		if (range > bestSoFar) {
			// Don't choose the same one we've already got
			if (tmpElement != currentElement) {
				bestSoFar = range;
				choice = tmpElement;
			}
		}
	}

	// Will lock the sensor on the related entity (no change if NULL)
	LockListElement( choice );
}


void HarmTargetingPod::SetDesiredTarget( SimObjectType* newTarget )
{
	FireControlComputer* FCC = ((SimVehicleClass*)platform) -> GetFCC();
	GroundListElement* tmpElement = FCC->GetFirstGroundElement();
	
	// Before we tell the sensor about the target, make sure we see it
	while (tmpElement)
	{
		if (tmpElement->BaseObject() == newTarget->BaseData())
		{
			// Okay, we found it, so take the lock
			SetSensorTarget( newTarget );
			break;
		}
		tmpElement = tmpElement->GetNext();
	}

   // NOTE: when called from the AI ground attack routine this will create the element if not found !
   if (!tmpElement)
   {
		tmpElement = new GroundListElement( newTarget->BaseData() );
		tmpElement->next = FCC->GetFirstGroundElement();
		FCC->grndlist = tmpElement;
		SetSensorTarget( newTarget );
   }
}


void HarmTargetingPod::Display( VirtualDisplay* activeDisplay )
{
	static const float arrowH = 0.0375f;
	static const float arrowW = 0.0433f;

	display = activeDisplay;

	// Do we draw flashing things this frame ?
	flash = vuxRealTime & 0x200;
	
	display->SetColor( 0xFF00FF00 );

	display->AdjustOriginInViewport (0.0f, HTS_Y_OFFSET);

	// Ownship "airplane" marker
	static const float	NOSE	= 0.02f;
	static const float	TAIL	= 0.04f;
	static const float	WING	= 0.04f;
	display->Line( -WING,   0.00f,  WING,  0.00f );
	display->Line(  0.00f, -TAIL,   0.00f,  NOSE );

	// JB Draw 180 degree lines
	display->SetColor( 0xFF004000 );
	display->Line((float) -.95, 0.0, -WING - WING, 0.0);
	display->Line(WING + WING, 0.0, (float) .95, 0.0);
	display->Arc(0.0, 0.0, WING + WING, 180 * DTR, 0);
	display->SetColor( 0xFF00FF00 );

	display->AdjustOriginInViewport (0.0f, -HTS_Y_OFFSET);

	// Display the missile effective footprint
	ShiAssert( platform->IsAirplane() );
	if (((AircraftClass*)platform)->Sms->curWeapon &&
       ((AircraftClass*)platform)->Sms->curWeaponType == wtAgm88) {
		ShiAssert( ((AircraftClass*)platform)->Sms->curWeapon->IsMissile() );
		DrawWEZ( (MissileClass*)((AircraftClass*)platform)->Sms->curWeapon );
	}

	// Draw the cursors
	display->AdjustOriginInViewport (cursorX, cursorY + HTS_Y_OFFSET);
	display->Line (-CURSOR_SIZE, -CURSOR_SIZE, -CURSOR_SIZE, CURSOR_SIZE);
	display->Line ( CURSOR_SIZE, -CURSOR_SIZE,  CURSOR_SIZE, CURSOR_SIZE);
	display->CenterOriginInViewport ();
	
	
	float x18, y18;
	float x19, y19;
	GetButtonPos(18, &x18, &y18);
	GetButtonPos(19, &x19, &y19);
	float ymid = y18 + (y19-y18)/2;
	
	// Add range and arrows
	char str[8];
	sprintf(str,"%3d",displayRange);
	ShiAssert (strlen(str) < sizeof(str));
	display->TextLeftVertical(x18, ymid, str);
	
	// up arrow
	display->AdjustOriginInViewport(x19 + arrowW, y19 + arrowH/2);
	display->Tri (0.0F, arrowH, arrowW, -arrowH, -arrowW, -arrowH);
	
	// down arrow
	display->CenterOriginInViewport ();
	display->AdjustOriginInViewport( x18 + arrowW, y18 - arrowH/2);
	display->Tri (0.0F, -arrowH, arrowW, arrowH, -arrowW, arrowH);
	
	display->CenterOriginInViewport();

	//MI
	if(g_bRealisticAvionics)
	{
		if(((AircraftClass*)platform)->Sms->curWeapon && ((AircraftClass*)platform)->Sms->curWeapon->IsMissile())
		{
			if(((AircraftClass*)SimDriver.playerEntity)->GetSOI() == SimVehicleClass::SOI_WEAPON)
			{  
				display->SetColor(GetMfdColor(MFD_GREEN));
				DrawBorder(); // JPO SOI
			}  
		}
	}
}


void HarmTargetingPod::DrawWEZ( MissileClass *theMissile )
{
	// If we don't have a missile, quit now
	if (!theMissile ) {
		return;
	}

	display->AdjustOriginInViewport (0.0f, HTS_Y_OFFSET);

	// Foot print
#if 0
	float mxRng;
	float mnRng;
	if (platform->ZPos() > -30000.0f) {
		mxRng = 25.0f + 3.0f/5000.0f*(-5000.0f-platform->ZPos());
		mnRng = -15.0f;
	} else if (platform->ZPos() > -40000.0f) {
		mxRng = 25.0f + 3.0f/5000.0f*(25000.0f);
		mnRng = -2.0f - 13.0f*(40000.0f+platform->ZPos())/10000.0f;
	} else {
		mxRng = 25.0f + 3.0f/5000.0f*(25000.0f);
		mnRng = -2.0f;
	}
	float footprintRatio = 2.0f/1.84f;	// Empirically determined from flyout tests.  SCR 10/19/98
#else
	// This would be very nice, but would require correct data (and a missile)
	float mxRng  =  theMissile->GetRMax (-platform->ZPos(), platform->Vt(),   0.0f,     0.0f, 0.0f) * FT_TO_NM;
// 2000-11-24 MODIFIED BY S.G, GetRMax NOW uses AtaFrom INSTEAD OF AZ. PASS THE AZ PARAMETER TO AtaFrom AS WELL
//	float mnRng  = -theMissile->GetRMax (-platform->ZPos(), platform->Vt(), 180.0f*DTR, 0.0f,   0.0f*DTR) * FT_TO_NM;
//	float latRng =  theMissile->GetRMax (-platform->ZPos(), platform->Vt(),  90.0f*DTR, 0.0f,   0.0f*DTR) * FT_TO_NM;
	float mnRng  = -theMissile->GetRMax (-platform->ZPos(), platform->Vt(), 180.0f*DTR, 0.0f, 180.0f*DTR) * FT_TO_NM;
	float latRng =  theMissile->GetRMax (-platform->ZPos(), platform->Vt(),  90.0f*DTR, 0.0f,  90.0f*DTR) * FT_TO_NM;
// END OF MODIFIED SECTION
	float footprintRatio = 2.0f * latRng/(mxRng - mnRng);
#endif
	// Shrink the displayed WEZ a little to represent a good launch zone instead of strictly Rmax
	mxRng *= 0.8f;
	mnRng *= 0.8f;

	float footprintCtr = (mxRng+mnRng)/(2.0f*displayRange);
	float footprintRad = mxRng/displayRange;

	// Choose a dim color for the WEZ
	display->SetColor( 0xFF004000 );

	// If the WEZ is too big for the current display scale, clamp it and use "dashes"
	if (footprintRad > 1.0f) {
		footprintCtr /= footprintRad;
		footprintRad = 1.0f;
		static const float A1 = 20.0f * DTR;
		static const float A2 = 35.0f * DTR;
		display->OvalArc (  0.0f, footprintCtr, footprintRatio*footprintRad, footprintRad,  A1,            A2 );
		display->OvalArc (  0.0f, footprintCtr, footprintRatio*footprintRad, footprintRad,  90.0f*DTR-A2,  90.0f*DTR-A1 );
		display->OvalArc (  0.0f, footprintCtr, footprintRatio*footprintRad, footprintRad,  90.0f*DTR+A1,  90.0f*DTR+A2 );
		display->OvalArc (  0.0f, footprintCtr, footprintRatio*footprintRad, footprintRad, 180.0f*DTR-A2, 180.0f*DTR-A1 );
		display->OvalArc (  0.0f, footprintCtr, footprintRatio*footprintRad, footprintRad, 180.0f*DTR+A1, 180.0f*DTR+A2 );
		display->OvalArc (  0.0f, footprintCtr, footprintRatio*footprintRad, footprintRad, 270.0f*DTR-A2, 270.0f*DTR-A1 );
		display->OvalArc (  0.0f, footprintCtr, footprintRatio*footprintRad, footprintRad, 270.0f*DTR+A1, 270.0f*DTR+A2 );
		display->OvalArc (  0.0f, footprintCtr, footprintRatio*footprintRad, footprintRad, 360.0f*DTR-A2, 360.0f*DTR-A1 );
	} else {
		display->Oval (  0.0f, footprintCtr, footprintRatio*footprintRad, footprintRad);
	}

	// Restore the default full intensity green color
	display->SetColor( 0xFF00FF00 );
	display->AdjustOriginInViewport (0.0f, -HTS_Y_OFFSET);
}


// Make sure we have the latest data for each track, and age them if they get old
//SimObjectType* HarmTargetingPod::Exec (SimObjectType* targetList)
SimObjectType* HarmTargetingPod::Exec (SimObjectType*)
{
	FireControlComputer* FCC = ((SimVehicleClass*)platform)->GetFCC();
	GroundListElement* tmpElement;
	GroundListElement* prevElement = NULL;
	SimObjectType* curObj;
	SimBaseClass* curSimObj;
	SimObjectLocalData* localData;
	VuListIterator emitters (EmitterList);
	CampBaseClass* curEmitter;
	int trackType;
	
	// Validate our locked target
	CheckLockedTarget();

	// Cursor Control
	//MI
	if(!g_bRealisticAvionics)
	{
		cursorX += FCC->cursorXCmd * g_fCursorSpeed * HTS_CURSOR_RATE * SimLibMajorFrameTime;
		cursorY += FCC->cursorYCmd * g_fCursorSpeed * HTS_CURSOR_RATE * SimLibMajorFrameTime;
	}
	else
	{
		if(((AircraftClass*)SimDriver.playerEntity) && ((AircraftClass*)SimDriver.playerEntity)->GetSOI() == SimVehicleClass::SOI_WEAPON)
		{
			cursorX += FCC->cursorXCmd * g_fCursorSpeed * HTS_CURSOR_RATE * SimLibMajorFrameTime;
			cursorY += FCC->cursorYCmd * g_fCursorSpeed * HTS_CURSOR_RATE * SimLibMajorFrameTime;
		}
	}

	cursorX = min ( max (cursorX, -1.0F), 1.0F);
	cursorY = min ( max (cursorY, -1.0F), 1.0F);
	
	FCC->UpdatePlanned();
	tmpElement = FCC->GetFirstGroundElement();
	// Go through the list of emmitters and update the data
	while (tmpElement)
	{
		// Handle sim/camp hand off
		tmpElement->HandoffBaseObject();

		// Removed?  (We really shouldn't do this -- once detected, things shouldn't disappear, just never emit)
		if (!tmpElement->BaseObject())
		{
		    tmpElement = tmpElement->next;
			continue;
		}
		
		// Time out our guidance flag
		if (SimLibElapsedTime > tmpElement->lastHit + RadarClass::TrackUpdateTime * 2.5f)
		{
		    tmpElement->ClearFlag(GroundListElement::Launch);
		}
		
		// Time out our track flag
		if (SimLibElapsedTime > tmpElement->lastHit + TRACK_CYCLE)
		{
		    tmpElement->ClearFlag(GroundListElement::Track);
		}
		
		// Time out our radiate flag
		if (SimLibElapsedTime > tmpElement->lastHit + RADIATE_CYCLE)
		{
		    tmpElement->ClearFlag(GroundListElement::Radiate);
		}
		
		prevElement = tmpElement;
		tmpElement = tmpElement->next;
	}

	// Walk our list marking things as unchecked
	for (tmpElement = FCC->GetFirstGroundElement(); tmpElement; tmpElement = tmpElement->GetNext())
	{
	    tmpElement->SetFlag(GroundListElement::UnChecked);
   }

	// Check the target list for 'pings'
	curObj = platform->targetList;
	while (curObj)
	{
		if (F4IsBadReadPtr(curObj, sizeof(SimObjectType)) || F4IsBadCodePtr((FARPROC) curObj->BaseData())) // JB 010318 CTD
		{
			curObj = curObj->next;
			continue;
		} // JB 010318 CTD

		if (curObj->BaseData()->IsSim() && curObj->BaseData()->GetRadarType())
		{
			// Localize the info
			curSimObj = (SimBaseClass*)curObj->BaseData();
			localData = curObj->localData;
			
			// Is it time to hear this one again?
			if (SimLibElapsedTime > localData->sensorLoopCount[HTS] + curSimObj->RdrCycleTime() * SEC_TO_MSEC)
			{
				// Can we hear it?
				if (BeingPainted(curObj) && CanDetectObject(curObj))
				{
					ObjectDetected(curObj->BaseData(), Track_Ping);
					localData->sensorLoopCount[HTS] = SimLibElapsedTime;
				}
			}
			else
			{
			    tmpElement = FindEmmitter( curSimObj );
			    if (tmpElement)
				tmpElement->ClearFlag(GroundListElement::UnChecked);
			}
		}
		curObj = curObj->next;
	}
	
	// Check the emitter list
	for (curEmitter = (CampBaseClass*)emitters.GetFirst(); curEmitter; curEmitter = (CampBaseClass*)emitters.GetNext())
	{
		// Check if aggregated unit can detect
		if (curEmitter->IsAggregate() &&                    // A campaign thing
          curEmitter->CanDetect(platform) &&              // That has us spotted
          curEmitter->GetRadarMode() != FEC_RADAR_OFF &&  // And is emmitting
          // JB 011016 CanDetectObject (platform))                     // And there is line of sight
          CanDetectObject (curEmitter))                     // And there is line of sight // JB 011016 
		{
			// What type of hit is this?
			switch (curEmitter->GetRadarMode())
			{
			  case FEC_RADAR_SEARCH_1:
			  case FEC_RADAR_SEARCH_2:
			  case FEC_RADAR_SEARCH_3:
			  case FEC_RADAR_SEARCH_100:
				trackType = Track_Ping;
				break;
				
			  case FEC_RADAR_AQUIRE:
				trackType = Track_Lock;
				break;
				
			  case FEC_RADAR_GUIDE:
				trackType = Track_Launch;
				break;
				
			  default:
				// Probably means its off...
				trackType = Track_Unlock;
				break;
			}
			
			// Add it to the list (if the list isn't full)
			ObjectDetected(curEmitter, trackType);
		}
	}

	// Walk our list looking for unchecked Sim things
	for (tmpElement = FCC->GetFirstGroundElement(); tmpElement; tmpElement = tmpElement->GetNext())
	{
		if (tmpElement->BaseObject() &&
		    tmpElement->BaseObject()->IsSim() &&
		    (tmpElement->IsSet(GroundListElement::UnChecked)) &&
			SimLibElapsedTime > tmpElement->lastHit + ((SimBaseClass*)(tmpElement->BaseObject()))->RdrCycleTime() * SEC_TO_MSEC)
		{
			curSimObj = (SimBaseClass*)(tmpElement->BaseObject());
			if (curSimObj->RdrRng())
			{
				float top, bottom;
				
				// Check LOS
				// See if the target is near the ground
				OTWDriver.GetAreaFloorAndCeiling (&bottom, &top);
				if (curSimObj->ZPos() < top || OTWDriver.CheckLOS( platform, curSimObj))
				{
					ObjectDetected(curSimObj, Track_Ping);
				}
			}
		}
	}
	
	return NULL;
}

VU_ID HarmTargetingPod::FindIDUnderCursor(void)
{
GroundListElement	*choice = FindTargetUnderCursor();
VU_ID tgtId = FalconNullId;

   if (choice && choice->BaseObject())
      tgtId = choice->BaseObject()->Id();

   return tgtId;
}

GroundListElement* HarmTargetingPod::FindTargetUnderCursor(void)
{
	GroundListElement		*tmpElement;
	GroundListElement		*choice		= NULL;
	float			bestSoFar	= 10.0f;
	float			x, y;
	float			displayX, displayY;
	mlTrig			trig;
	float			delta;
	
	mlSinCos (&trig, platform->Yaw());

	FireControlComputer* FCC = ((SimVehicleClass*)platform)->GetFCC();
	
	// Walk our list looking for the thing in range and nearest the center of the cursors
	for (tmpElement	= FCC->GetFirstGroundElement(); tmpElement; tmpElement = tmpElement->GetNext())
	{
	    if (tmpElement->BaseObject() == NULL) continue;
		// Convert to normalized display space with heading up
		y = (tmpElement->BaseObject()->XPos() - platform->XPos()) * FT_TO_NM / displayRange * HTS_DISPLAY_RADIUS;
		x = (tmpElement->BaseObject()->YPos() - platform->YPos()) * FT_TO_NM / displayRange * HTS_DISPLAY_RADIUS;
		displayX = trig.cos * x - trig.sin * y;
		displayY = trig.sin * x + trig.cos * y;
		
		// See if this is the closest to the cursor point so far
		delta = (float)max( fabs(displayX - cursorX), fabs(displayY - cursorY) );
		if (delta < bestSoFar) {
			bestSoFar = delta;
			choice = tmpElement;
		}
	}
	
	// See if this is inside the cursor region
	if (bestSoFar < CURSOR_SIZE) {
		return choice;
	} else {
		return NULL;
	}
}

void HarmTargetingPod::GetAGCenter (float* x, float* y)
{
mlTrig yawTrig;
float range = displayRange * NM_TO_FT/HTS_DISPLAY_RADIUS;

	mlSinCos( &yawTrig, platform->Yaw() );
	
	*x = (cursorY*yawTrig.cos - cursorX*yawTrig.sin) * range + platform->XPos();
	*y = (cursorY*yawTrig.sin + cursorX*yawTrig.cos) * range + platform->YPos();
}

#if 0
void HarmTargetingPod::BuildPreplannedTargetList (void)
{
	FlightClass* theFlight = (FlightClass*)(platform->GetCampaignObject());
	FalconPrivateList* knownEmmitters = NULL;
	ListElement* tmpElement;
	ListElement* curElement = NULL;
	FalconEntity* eHeader;
	
	if (SimDriver.RunningCampaignOrTactical() && theFlight)
		knownEmmitters = theFlight->GetKnownEmitters();
	
	if (knownEmmitters)
	{
		VuListIterator elementWalker (knownEmmitters);
		
		eHeader = (FalconEntity*)elementWalker.GetFirst();
		while (eHeader)
		{
			tmpElement = new ListElement (eHeader);
			
			if (emmitterList == NULL)
				emmitterList = tmpElement;
			else
				curElement->next = tmpElement;
			curElement = tmpElement;
			eHeader = (FalconEntity*)elementWalker.GetNext();
		}
		
		knownEmmitters->DeInit();
		delete knownEmmitters;
	}
}
#endif

GroundListElement* HarmTargetingPod::FindEmmitter( FalconEntity *entity )
{
	FireControlComputer* FCC = ((SimVehicleClass*)platform)->GetFCC();
	GroundListElement* tmpElement = FCC->GetFirstGroundElement();
	
	while (tmpElement)
	{
		if (tmpElement->BaseObject() == entity)
			break;
		tmpElement = tmpElement->GetNext();
	}
	
	return (tmpElement);
}


int HarmTargetingPod::ObjectDetected (FalconEntity* newEmmitter, int trackType, int dummy) // 2002-02-09 MODIFIED BY S.G. Added the unused dummy var
{
   int retval = FALSE;
	GroundListElement		*tmpElement;

	// We're only tracking ground things right now...
	if (newEmmitter->OnGround())
	{
		// See if this one is already in our list
		tmpElement = FindEmmitter( newEmmitter );
		if (!tmpElement)
		{
		    	FireControlComputer* FCC = ((SimVehicleClass*)platform)->GetFCC();
			// Add the new one at the head;
			tmpElement = new GroundListElement( newEmmitter );
			FCC->AddGroundElement(tmpElement);
         retval = TRUE;
		}

      // Make sure it is marked as checked
		tmpElement->ClearFlag(GroundListElement::UnChecked);

		switch (trackType) {
		  // Note:  It is intentional that these cases fall through...
		  case Track_Unlock:
		      tmpElement->ClearFlag(GroundListElement::Track);

		  case Track_LaunchEnd:
		      tmpElement->ClearFlag(GroundListElement::Launch);

		  // Break here....
			break;

		  // Note:  It is intentional that these cases fall through...
		  case Track_Launch:
		      tmpElement->SetFlag(GroundListElement::Launch);

		  case Track_Lock:
			tmpElement->SetFlag(GroundListElement::Track);

		  default:
			tmpElement->SetFlag(GroundListElement::Radiate);
		}

		// Update the hit time;
		tmpElement->lastHit = SimLibElapsedTime;
	}

   return retval;
}


// Will lock the sensor on the related entity (no change if NULL)
void HarmTargetingPod::LockListElement( GroundListElement *choice )
{
	SimObjectType	*obj;


	// If we have a candidate, look for it in the platform's target list
	if (choice) {
		for (obj = platform->targetList; obj; obj = obj->next)
		{
			if (obj->BaseData() == choice->BaseObject()) {
				// We found a match!
				SetSensorTarget( obj );
				break;
			}
		}

		// If we didn't find it in the target list, force the issue anyway...
		if (!obj) {
			SetSensorTargetHack( choice->BaseObject() );
		}
	}
}

#if 0
// List elements of the HarmTargetingPod
HarmTargetingPod::ListElement::ListElement (FalconEntity* newEntity)
{
	F4Assert (newEntity);
	
	baseObject	= newEntity;
	symbol		= RadarDataTable[newEntity->GetRadarType()].RWRsymbol;
	flags		= 0;
	next		= NULL;
	lastHit		= SimLibElapsedTime;
	VuReferenceEntity(newEntity);
}


HarmTargetingPod::ListElement::~ListElement ()
{
	VuDeReferenceEntity(baseObject);
}


void HarmTargetingPod::ListElement::HandoffBaseObject(void)
{
	FalconEntity	*newBase;

	ShiAssert( baseObject );

	newBase = SimCampHandoff( baseObject, HANDOFF_RADAR );

	if (newBase != baseObject) {
		VuDeReferenceEntity(baseObject);

		baseObject = newBase;

		if (baseObject) {
			VuReferenceEntity(baseObject);
		}
	}
}
#endif
