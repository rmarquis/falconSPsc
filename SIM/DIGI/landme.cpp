#include <float.h>
#include "stdhdr.h"
#include "digi.h"
#include "simdrive.h"
#include "simveh.h"
#include "Find.h"
#include "aircrft.h"
#include "atcbrain.h"
#include "simfeat.h"
#include "MsgInc\RadioChatterMsg.h"
#include "MsgInc\ATCMsg.h"
#include "falcsnd\conv.h"
#include "falcsess.h"
#include "airframe.h"
#include "ptdata.h"
#include "airunit.h"
#include "otwdrive.h"
#include "camplist.h"
#include "Graphics\Include\drawBSP.h"
#include "simmath.h"
#include "phyconst.h"
#include "classtbl.h"
#include "Graphics\Include\tmap.h"
#include "Team.h"
#include "playerop.h"
#include "radar.h"
#include "sms.h"
#include "cpmanager.h"
#include "hud.h"

#ifdef DEBUG
extern long TeamSimColorList[NUM_TEAMS];
#endif

extern int gameCompressionRatio;
extern int g_nReagTimer;
extern int g_nShowDebugLabels;

#ifdef DAVE_DBG
extern void SetLabel (SimBaseClass* theObject);
#endif


void DigitalBrain::ResetATC (void)
{
	SetATCStatus(noATC);
	if (!(moreFlags & NewHomebase))	// we set a new airbase to head to (for example because of fumes fuel -> Actions.cpp)
		airbase = self->HomeAirbase();
	rwIndex = 0;
	rwtime = 0;
	waittimer = 0;
	curTaxiPoint = 0;
	desiredSpeed = 0;
	turnDist = 0;
#ifdef DAVE_DBG
	SetLabel(self);
#endif
}

void DigitalBrain::ResetTaxiState(void)
{
	int takeoffpt, runwaypt, rwindex;
	float x1, y1, x2, y2;
	float dx, dy, relx;
	//float deltaHdg;
	//ObjectiveClass *Airbase = (ObjectiveClass *)vuDatabase->Find(airbase);

	GridIndex X,Y;
				
	X = SimToGrid(af->y);
	Y = SimToGrid(af->x);
	
	Objective Airbase = FindNearbyAirbase (X, Y);

	if(!self->OnGround() || !Airbase)
	{
		if(atcstatus >= tReqTaxi)
			ResetATC();
		return;
	}

	float dist, closestDist =  4000000.0F;
	int taxiPoint, closest = curTaxiPoint;

	taxiPoint = GetFirstPt(rwIndex);
	while(taxiPoint)
	{
		TranslatePointData(Airbase, taxiPoint, &x1, &y1);
		dx = x1 - af->x;
		dy = y1 - af->y;
		dist = dx*dx + dy*dy;
		if ( dist < closestDist )
		{
			closestDist = dist;
			closest = taxiPoint;
		}
		taxiPoint = GetNextPt(taxiPoint);
	}

	if(closest != curTaxiPoint)
	{
		curTaxiPoint = closest;
		TranslatePointData(Airbase, curTaxiPoint, &trackX, &trackY);
	}

	if(self->AutopilotType() == AircraftClass::APOff)
		return;

	if( (atcstatus == tTakeRunway || atcstatus == tTakeoff) && 
		 (PtDataTable[curTaxiPoint].type == TakeoffPt || PtDataTable[curTaxiPoint].type == RunwayPt) )
	{
		rwindex = Airbase->brain->IsOnRunway(self);

		takeoffpt = Airbase->brain->FindTakeoffPt((Flight)self->GetCampaignObject(), self->vehicleInUnit, rwIndex, &x1, &y1);
		runwaypt = Airbase->brain->FindRunwayPt((Flight)self->GetCampaignObject(), self->vehicleInUnit, rwIndex, &x2, &y2);

		float cosAngle =	self->platformAngles->sinsig * PtHeaderDataTable[rwIndex].sinHeading + 
							self->platformAngles->cossig * PtHeaderDataTable[rwIndex].cosHeading;
	
		runwayStatsStruct *runwayStats = Airbase->brain->GetRunwayStats();
		dx = runwayStats[PtHeaderDataTable[rwIndex].runwayNum].centerX - self->XPos();
		dy = runwayStats[PtHeaderDataTable[rwIndex].runwayNum].centerY - self->YPos();
		
		relx = PtHeaderDataTable[rwIndex].cosHeading*dx + PtHeaderDataTable[rwIndex].sinHeading*dy;

		if(cosAngle > 0.99619F && PtHeaderDataTable[rwIndex].runwayNum == PtHeaderDataTable[rwindex].runwayNum &&
			runwayStats[PtHeaderDataTable[rwIndex].runwayNum].halfheight + relx > 3000.0F )
		{
			trackX = x2;
			trackY = y2;
			curTaxiPoint = runwaypt;				
		}
		else if( relx > 0.0F)
		{
			trackX = x1 - relx * PtHeaderDataTable[rwIndex].cosHeading;
			trackY = y1 - relx * PtHeaderDataTable[rwIndex].sinHeading;
			curTaxiPoint = takeoffpt;				
		}
		else
		{
			trackX = x1;
			trackY = y1;
			curTaxiPoint = takeoffpt;
		}
		atcstatus = tTakeRunway;
	}
	else
	{
		if(PtDataTable[curTaxiPoint].type == TakeoffPt || PtDataTable[curTaxiPoint].type == RunwayPt)
		{
			takeoffpt = Airbase->brain->FindTakeoffPt((Flight)self->GetCampaignObject(), self->vehicleInUnit, rwIndex, &trackX, &trackY);
		}
		else
		{
			TranslatePointData(Airbase, curTaxiPoint, &x1, &y1);
			dx = x1 - af->x;
			dy = y1 - af->y;
			relx	= self->platformAngles->cospsi * dx + self->platformAngles->sinpsi * dy;
			if(relx < 0.0F)
			{
				ChooseNextPoint(Airbase);		
			}
			else
			{
				trackX = x1;
				trackY = y1;	
			}
		}
	}

   // Make sure ground weapon list is up to date
   SelectGroundWeapon();
}

float DigitalBrain::CalculateNextTurnDistance(void)
{
	float curHeading=0.0F, newHeading=0.0F, cosAngle=1.0F, deltaHdg=0.0F;
	float baseX=0.0F, baseY=0.0F, finalX=0.0F, finalY=0.0F, dx=0.0F, dy=0.0F, vt=0.0F;
	ObjectiveClass *Airbase = (ObjectiveClass *)vuDatabase->Find(airbase);
	
	turnDist = 500.0F;

	if(Airbase && Airbase->brain)
	{
		//vt = sqrt(self->XDelta() * self->XDelta() + self->YDelta() * self->YDelta());
		vt = af->MinVcas() * KNOTS_TO_FTPSEC;

		cosAngle = Airbase->brain->DetermineAngle(self, rwIndex, atcstatus);

		switch(atcstatus)
		{
		case lFirstLeg:
			dx = self->XPos() - trackX;
			dy = self->YPos() - trackY;
			curHeading = (float)atan2(dy,dx);
			if(curHeading < 0.0F)
				curHeading += PI * 2.0F;

			Airbase->brain->FindFinalPt(self, rwIndex, &finalX, &finalY);
			if(cosAngle < 0.0F)
			{
				Airbase->brain->FindBasePt(self, rwIndex, finalX, finalY, &baseX, &baseY);
				dx = trackX - baseX;
				dy = trackY - baseY;
			}
			else
			{
				dx = trackX - finalX;
				dy = trackY - finalY;
			}
			newHeading = (float)atan2(dy,dx);
			if(newHeading < 0.0F)
				newHeading += PI * 2.0F;

			deltaHdg = newHeading - curHeading;
			if(deltaHdg > PI)
				deltaHdg = -(deltaHdg - PI);
			else if(deltaHdg < -PI)
				deltaHdg = -(deltaHdg + PI);
			turnDist = (float)fabs( deltaHdg * 12.15854203708F * vt );
			break;

		case lToBase:
			dx = self->XPos() - trackX;
			dy = self->YPos() - trackY;
			curHeading = (float)atan2(dy,dx);
			if(curHeading < 0.0F)
				curHeading += PI * 2.0F;

			Airbase->brain->FindFinalPt(self, rwIndex, &finalX, &finalY);
			dx = trackX - finalX;
			dy = trackY - finalY;
			newHeading = (float)atan2(dy,dx);
			if(newHeading < 0.0F)
				newHeading += PI * 2.0F;

			deltaHdg = newHeading - curHeading;
			if(deltaHdg > PI)
				deltaHdg = -(deltaHdg - PI);
			else if(deltaHdg < -PI)
				deltaHdg = -(deltaHdg + PI);
			turnDist = (float)fabs( deltaHdg * 12.15854203708F * vt );
			break;

		case lToFinal:
		case lOnFinal:
			dx = self->XPos() - trackX;
			dy = self->YPos() - trackY;
			curHeading = (float)atan2(dy,dx);
			if(curHeading < 0.0F)
				curHeading += PI * 2.0F;
			
			newHeading = PtHeaderDataTable[rwIndex].data * DTR;

			deltaHdg = newHeading - curHeading;
			if(deltaHdg > PI)
				deltaHdg = -(deltaHdg - PI);
			else if(deltaHdg < -PI)
				deltaHdg = -(deltaHdg + PI);
			turnDist = (float)fabs( deltaHdg * 12.15854203708F * vt );
			if(turnDist < 4000.0F)
				turnDist = 4000.0F;
			break;
		}
		turnDist += (0.5F * vt);
	}	

	if(self->IsPlayer())
	{
		if(turnDist < 400.0F)
			turnDist = 400.0F;
	}
	else if(turnDist < 300.0F)
	{
		turnDist = 300.0F;
	}

	return turnDist;
}

void DigitalBrain::Land(void)
{
	SimBaseClass *inTheWay = NULL;
	ObjectiveClass *Airbase = (ObjectiveClass *)vuDatabase->Find(airbase);
	AircraftClass *leader = NULL;
	AircraftClass *component = NULL;
	float cosAngle, heading, deltaTime, testDist;
	ulong min, max;
	float baseX, baseY, finalX, finalY, finalZ, x, y, z, dx, dy, dist, speed, minSpeed, relx, rely, cosHdg, sinHdg;
	mlTrig	Trig;
	
	if(!Airbase)
	{
		//need to find something or we don't know where to go
		if ( self->af->GetSimpleMode())
		{
			SimpleGoToCurrentWaypoint();
		}
		else
		{
			GoToCurrentWaypoint();
		}
		return;
	}
	else if(!Airbase->IsObjective() || !Airbase->brain)
	{
		if(self->curWaypoint->GetWPAction() == WP_LAND && !self->IsPlayer())
		{
			dx = self->XPos() - trackX;
			dy = self->YPos() - trackY;
			if(dx*dx + dy*dy < 3000.0F*3000.0F)
			{
				//for carriers we just disappear when we get close enough
				//do carrier landings for F-18
				RegroupAircraft (self);
				return;
			}
		}
		if ( self->af->GetSimpleMode())
		{
			SimpleGoToCurrentWaypoint();
		}
		else
		{
			GoToCurrentWaypoint();
		}
		return;
	}

	if(self->IsSetFlag(ON_GROUND) && af->Fuel() <= 0.0F && af->vt < 5.0F)
	{
		if(!self->IsPlayer())
			RegroupAircraft (self); //no gas get him out of the way
	}
	SetDebugLabel(Airbase);

	speed = af->MinVcas() * KNOTS_TO_FTPSEC;

	dx = self->XPos() - trackX;
	dy = self->YPos() - trackY;
	if (rwIndex > 0) { // jpo - only valid if we have a runway.
		cosAngle = Airbase->brain->DetermineAngle(self, rwIndex, atcstatus);
		
		Airbase->brain->CalculateMinMaxTime(self, rwIndex, atcstatus, &min, &max, cosAngle);
	}
	else {
		cosAngle = 0;
		min = max = 0;
	}
	// edg: project out 1 sec to get alt for possible ground avoid
    float gAvoidZ = OTWDriver.GetGroundLevel( self->XPos() + self->XDelta(),
	   								  self->YPos() + self->YDelta() );
	float minZ = Airbase->brain->GetAltitude(self, atcstatus);

	switch(atcstatus)
	{
	case noATC:
		if((GetTTRelations(Airbase->GetTeam(),self->GetTeam()) >= Neutral))
		{
			airbase = self->DivertAirbase();
			Airbase = (ObjectiveClass *)vuDatabase->Find(airbase);
			if(!Airbase || (GetTTRelations(Airbase->GetTeam(),self->GetTeam()) >= Neutral))
			{
				GridIndex X,Y;
				
				X = SimToGrid(af->y);
				Y = SimToGrid(af->x);

				Airbase = FindNearestFriendlyRunway (self->GetTeam(), X, Y);
				if(Airbase)
					airbase = Airbase->Id();
				else
				{
					//need to find the airbase or we don't know where to go
					if ( self->af->GetSimpleMode())
					{
						SimpleGoToCurrentWaypoint();
					}
					else
					{
						GoToCurrentWaypoint();
					}
					return;
				}

			}
		}
		rwIndex = Airbase->brain->FindBestLandingRunway(self, TRUE);
		Airbase->brain->FindFinalPt(self, rwIndex, &trackX, &trackY);
		trackZ = Airbase->brain->GetAltitude(self, atcstatus);
		CalculateNextTurnDistance();
		waittimer = SimLibElapsedTime + 2 * TAKEOFF_TIME_DELTA;
		TrackPointLanding( af->CalcTASfromCAS(af->MinVcas() * 1.2F) * KNOTS_TO_FTPSEC);
		dx = self->XPos() - Airbase->XPos();
		dy = self->YPos() - Airbase->YPos();
		//me123
		if (curMode == LandingMode ||
			curMode == TakeoffMode ||
			curMode == WaypointMode ||
			curMode == RTBMode) ;
		else break;

		if(dx*dx + dy*dy < APPROACH_RANGE * NM_TO_FT * NM_TO_FT * 0.95F)
		{
			atcstatus = lReqClearance;
			if( !isWing )
				SendATCMsg(lReqClearance);
		}
		else
		{
			atcstatus = lIngressing;
			SendATCMsg(lIngressing);
		}
	    if ( self->ZPos() - gAvoidZ > minZ )
			trackZ = gAvoidZ + minZ - ( self->ZPos() - gAvoidZ - minZ ) * 2.0f;
		break;

	//////////////////////////////////////////////////////////////////////////////////////
	case lIngressing:
		dx = self->XPos() - Airbase->XPos();
		dy = self->YPos() - Airbase->YPos();

		if(dx*dx + dy*dy < APPROACH_RANGE * NM_TO_FT * NM_TO_FT * 0.95F)
		{
			rwIndex = Airbase->brain->FindBestLandingRunway(self, TRUE);
			Airbase->brain->FindFinalPt(self, rwIndex, &trackX, &trackY);
			trackZ = Airbase->brain->GetAltitude(self, atcstatus);
			CalculateNextTurnDistance();
			atcstatus = lReqClearance;
			if( !isWing )
				SendATCMsg(lReqClearance);
			waittimer = SimLibElapsedTime + LAND_TIME_DELTA;
			//if emergency
			//SendATCMsg(lReqEmerClearance);
		}
	    if ( self->ZPos() - gAvoidZ > minZ )
			trackZ = gAvoidZ + minZ - ( self->ZPos() - gAvoidZ - minZ) * 2.0f;
		//TrackPointLanding( af->MinVcas() * 1.2F * KNOTS_TO_FTPSEC);
		TrackPointLanding( af->CalcTASfromCAS(af->MinVcas() * 1.2F)*KNOTS_TO_FTPSEC);
		//TrackPoint( 0.0F, af->MinVcas() * 1.2F * KNOTS_TO_FTPSEC);

		break;

	//////////////////////////////////////////////////////////////////////////////////////
	case lTakingPosition:
		//need to drag out formation
		//the atc will clear us when we get within range
		dx = self->XPos() - Airbase->XPos();
		dy = self->YPos() - Airbase->YPos();
		if(dx*dx + dy*dy < TOWER_RANGE * NM_TO_FT * NM_TO_FT * 0.5F && waittimer < SimLibElapsedTime)
		{
			atcstatus = lReqClearance;
			if( !isWing )
				SendATCMsg(lReqClearance);
			waittimer = SimLibElapsedTime + LAND_TIME_DELTA;
		}
		speed = af->CalcTASfromCAS(af->MinVcas() * 1.2F)*KNOTS_TO_FTPSEC;

		if(((Unit)self->GetCampaignObject())->GetTotalVehicles() > 1)
		{
			VuListIterator	cit(self->GetCampaignObject()->GetComponents());
			component = (AircraftClass*)cit.GetFirst();
			while(component && component->vehicleInUnit != self->vehicleInUnit)
			{
				leader = component;
				component = (AircraftClass*)cit.GetNext();
			}

			if(leader
				&& !mpActionFlags[AI_RTB]) // JB 010527 (from MN)
			{
				dx = self->XPos() - leader->XPos();
				dy = self->YPos() - leader->YPos();
				dist = dx*dx + dy*dy;

				if(dist < NM_TO_FT * NM_TO_FT)
					speed = af->CalcTASfromCAS(af->MinVcas())*KNOTS_TO_FTPSEC;

				trackX = leader->XPos();
				trackY = leader->YPos();
				trackZ = leader->ZPos();
			}
			else
			{
				Airbase->brain->FindFinalPt(self, rwIndex, &trackX, &trackY);
				trackZ = Airbase->brain->GetAltitude(self, lTakingPosition);
			}
		}
	    if ( self->ZPos() - gAvoidZ > minZ )
			trackZ = gAvoidZ + minZ - ( self->ZPos() - gAvoidZ - minZ ) * 2.0f;
		TrackPointLanding( speed);
		break;

	//////////////////////////////////////////////////////////////////////////////////////
	case lReqClearance:
	case lReqEmerClearance:
	    if ( self->ZPos() - gAvoidZ > minZ )
			trackZ = gAvoidZ + minZ - ( self->ZPos() - gAvoidZ - minZ ) * 2.0f;
		if(SimLibElapsedTime > waittimer)
		{
			//we've been waiting too long, call again
			rwIndex = Airbase->brain->FindBestLandingRunway(self, TRUE);
			Airbase->brain->FindFinalPt(self, rwIndex, &trackX, &trackY);
			trackZ = Airbase->brain->GetAltitude(self, atcstatus);
	    	if ( self->ZPos() - gAvoidZ > minZ )
				trackZ = gAvoidZ + minZ - ( self->ZPos() - gAvoidZ - minZ ) * 2.0f;
			CalculateNextTurnDistance();

			// JB 010802 RTBing AI aircraft won't land.
			dx = self->XPos() - Airbase->XPos();
			dy = self->YPos() - Airbase->YPos();
			if(dx*dx + dy*dy < (TOWER_RANGE + 100) * NM_TO_FT * NM_TO_FT * 0.95F)
			{
				waittimer = SimLibElapsedTime + LAND_TIME_DELTA;
				SendATCMsg(lReqClearance);
			}
			else
				waittimer = SimLibElapsedTime + LAND_TIME_DELTA / 2;
		}
		//we're waiting to get a response back
		TrackPointLanding( af->CalcTASfromCAS(af->MinVcas() * 1.2F)*KNOTS_TO_FTPSEC);
		af->gearHandle = -1.0F;
		break;

	//////////////////////////////////////////////////////////////////////////////////////
	case lAborted:
	    if ( self->ZPos() - gAvoidZ > minZ )
			trackZ = gAvoidZ + minZ - ( self->ZPos() - gAvoidZ - minZ ) * 2.0f;

		if(dx*dx + dy*dy < 0.25F*NM_TO_FT*NM_TO_FT)
		{
			waittimer = SimLibElapsedTime + LAND_TIME_DELTA;			
			rwIndex = Airbase->brain->FindBestLandingRunway(self, TRUE);
			Airbase->brain->FindFinalPt(self, rwIndex, &trackX, &trackY);
			trackZ = Airbase->brain->GetAltitude(self, lReqClearance);
			z = TheMap.GetMEA(trackX, trackY);
	    	if ( self->ZPos() - gAvoidZ > minZ )
				trackZ = gAvoidZ + minZ - ( self->ZPos() - gAvoidZ - minZ ) * 2.0f;
			atcstatus = lReqClearance;
			SendATCMsg(lReqClearance);
		}

		TrackPoint( 0.0F, af->CalcTASfromCAS(af->MinVcas() * 1.2F)*KNOTS_TO_FTPSEC);
		af->gearHandle = -1.0F;
		break;

	//////////////////////////////////////////////////////////////////////////////////////
	case lEmerHold:
	   	if ( self->ZPos() - gAvoidZ > minZ )
			trackZ = gAvoidZ + minZ - ( self->ZPos() - gAvoidZ - minZ ) * 2.0f;

		if(waittimer < SimLibElapsedTime)
		{
			atcstatus = lEmerHold;
			SendATCMsg(lEmerHold);
			waittimer = SimLibElapsedTime + LAND_TIME_DELTA;
		}
		Loiter();
		af->gearHandle = -1.0F;
		break;

	//////////////////////////////////////////////////////////////////////////////////////
	case lHolding:
	   	if ( self->ZPos() - gAvoidZ > minZ )
			trackZ = gAvoidZ + minZ - ( self->ZPos() - gAvoidZ - minZ ) * 2.0f;

		if(rwtime < SimLibElapsedTime + max - CampaignSeconds * 5 )
		{
			Airbase->brain->FindFinalPt(self, rwIndex, &finalX, &finalY);
			waittimer = rwtime + CampaignSeconds * 15; 
			
			if(cosAngle < 0.0F)
			{
				Airbase->brain->FindBasePt(self, rwIndex, finalX, finalY, &baseX, &baseY);
				atcstatus = Airbase->brain->FindFirstLegPt(self, rwIndex, rwtime, baseX, baseY, TRUE, &trackX, &trackY);
				trackZ = Airbase->brain->GetAltitude(self, atcstatus);
				if(atcstatus != lHolding)
					SendATCMsg(atcstatus);
				CalculateNextTurnDistance();
			}
			else
			{
				atcstatus = Airbase->brain->FindFirstLegPt(self, rwIndex, rwtime, finalX, finalY, FALSE, &trackX, &trackY);
				trackZ = Airbase->brain->GetAltitude(self, atcstatus);
				if(atcstatus != lHolding)
					SendATCMsg(atcstatus);
				CalculateNextTurnDistance();
			}
	   		if ( self->ZPos() - gAvoidZ > minZ )
				trackZ = gAvoidZ + minZ - ( self->ZPos() - gAvoidZ - minZ ) * 2.0f;

			if(atcstatus == lHolding)
				Loiter();
		}
		else
		{
			Loiter();
		}
		af->gearHandle = -1.0F;
		break;

	//////////////////////////////////////////////////////////////////////////////////////
	case lFirstLeg:
		if(self->pctStrength < 0.4F)
			Airbase->brain->RequestEmerClearance(self);

	  	if ( self->ZPos() - gAvoidZ > minZ )
			trackZ = gAvoidZ + minZ - ( self->ZPos() - gAvoidZ - minZ ) * 2.0f;

		cosHdg = self->platformAngles->cossig;
		sinHdg = self->platformAngles->sinsig;	

		relx = (  cosHdg*dx + sinHdg*dy);
		rely = ( -sinHdg*dx + cosHdg*dy);

		if(fabs(relx) < turnDist && fabs(rely) < turnDist*3.0F)
		{
			Airbase->brain->FindFinalPt(self, rwIndex, &finalX, &finalY);
			if(cosAngle < 0.0F)
			{
				atcstatus = lToBase;
				Airbase->brain->FindBasePt(self, rwIndex, finalX, finalY, &baseX, &baseY);
				trackX = baseX;
				trackY = baseY;
				trackZ = Airbase->brain->GetAltitude(self, atcstatus);
				SendATCMsg(atcstatus);
				CalculateNextTurnDistance();
			}
			else
			{
				atcstatus = lToFinal;
				trackX = finalX;
				trackY = finalY;
				trackZ = Airbase->brain->GetAltitude(self, atcstatus);
				SendATCMsg(atcstatus);
				CalculateNextTurnDistance();
			}
	  		if ( self->ZPos() - gAvoidZ > minZ )
				trackZ = gAvoidZ + minZ - ( self->ZPos() - gAvoidZ - minZ ) * 2.0f;
		}
		
		TrackPointLanding( af->CalcTASfromCAS(af->MinVcas())*KNOTS_TO_FTPSEC);
		break;

	//////////////////////////////////////////////////////////////////////////////////////
	case lToBase:
		if(self->pctStrength < 0.4F)
			Airbase->brain->RequestEmerClearance(self);
	case lEmergencyToBase:
	
	  	if ( self->ZPos() - gAvoidZ > minZ )
			trackZ = gAvoidZ + minZ - ( self->ZPos() - gAvoidZ - minZ ) * 2.0f;

		//if(dx*dx + dy*dy < turnDist*turnDist)
		cosHdg = self->platformAngles->cossig;
		sinHdg = self->platformAngles->sinsig;	

		relx = (  cosHdg*dx + sinHdg*dy);
		rely = ( -sinHdg*dx + cosHdg*dy);

		if(fabs(relx) < turnDist && fabs(rely) < turnDist*3.0F)
		{
			Airbase->brain->FindFinalPt(self, rwIndex, &finalX, &finalY);
			if(atcstatus == lEmergencyToBase)
				atcstatus = lEmergencyToFinal;
			else
				atcstatus = lToFinal;
			trackX = finalX;
			trackY = finalY;
			trackZ = Airbase->brain->GetAltitude(self, atcstatus);
	  		if ( self->ZPos() - gAvoidZ > minZ )
				trackZ = gAvoidZ + minZ - ( self->ZPos() - gAvoidZ - minZ ) * 2.0f;
			SendATCMsg(atcstatus);
			CalculateNextTurnDistance();
		}
		
		if(rwtime > SimLibElapsedTime + FINAL_TIME + BASE_TIME)
		{
			deltaTime = (rwtime - SimLibElapsedTime - FINAL_TIME - BASE_TIME)/(float)CampaignSeconds;
			speed = (float)sqrt(dx*dx + dy*dy)/deltaTime;
			speed = min(af->CalcTASfromCAS(af->MaxVcas() * 0.8F)*KNOTS_TO_FTPSEC, 
							max(speed, af->CalcTASfromCAS(af->MinVcas() * 0.8F)*KNOTS_TO_FTPSEC));
		}
		else
			speed = af->CalcTASfromCAS(af->MaxVcas() * 0.8F)*KNOTS_TO_FTPSEC;
		
		TrackPointLanding(speed);
	/*	if(TrackPointLanding(speed) > speed + 50.0F)
		{
			atcstatus = lHolding;
			trackX = af->x;
			trackY = af->y;
			trackZ = Airbase->brain->GetAltitude(self, atcstatus);
			SendATCMsg(atcstatus);
		}*/
		break;

	//////////////////////////////////////////////////////////////////////////////////////
	case lToFinal:
		if(self->pctStrength < 0.4F)
			Airbase->brain->RequestEmerClearance(self);
	case lEmergencyToFinal:
	
	  	if ( self->ZPos() - gAvoidZ > minZ )
			trackZ = gAvoidZ + minZ - ( self->ZPos() - gAvoidZ - minZ ) * 2.0f;

		cosHdg = PtHeaderDataTable[rwIndex].cosHeading;
		sinHdg = PtHeaderDataTable[rwIndex].sinHeading;	

		relx = (  cosHdg*dx + sinHdg*dy);
		rely = ( -sinHdg*dx + cosHdg*dy);

		if(relx < 3.0F*NM_TO_FT && relx > -1.0F*NM_TO_FT && fabs(rely) < turnDist)
		{
			curTaxiPoint = GetFirstPt(rwIndex);
			TranslatePointData (Airbase, curTaxiPoint, &trackX, &trackY);
			atcstatus = lOnFinal;
			if(atcstatus == lEmergencyToFinal)
				atcstatus = lEmergencyOnFinal;
			else
				atcstatus = lOnFinal;

			trackZ = Airbase->brain->GetAltitude(self, atcstatus);
			SendATCMsg(atcstatus);
			af->gearHandle = 1.0F;
		}

		if(rwtime > SimLibElapsedTime + FINAL_TIME)
		{
			deltaTime = (rwtime - SimLibElapsedTime - FINAL_TIME)/(float)CampaignSeconds;
			speed = (float)sqrt(dx*dx + dy*dy)/deltaTime;
			speed = min(af->CalcTASfromCAS(af->MaxVcas() * 0.8F)*KNOTS_TO_FTPSEC, 
								max(speed, af->CalcTASfromCAS(af->MinVcas() * 0.8F)*KNOTS_TO_FTPSEC));
		}
		else
			speed = af->CalcTASfromCAS(af->MaxVcas() * 0.8F)*KNOTS_TO_FTPSEC;
		
		TrackPointLanding(speed);
	/*	if(TrackPointLanding(speed) > speed + 20.0F)
		{
			atcstatus = lHolding;
			trackX = af->x;
			trackY = af->y;
			trackZ = Airbase->brain->GetAltitude(self, atcstatus);
			SendATCMsg(atcstatus);
		}*/
		break;

	//////////////////////////////////////////////////////////////////////////////////////
	case lEmergencyOnFinal:
	case lOnFinal:
		// now we just wait until touchdown
		if ( self->IsSetFlag( ON_GROUND ) )
        {
			atcstatus = lLanded;
			SendATCMsg(atcstatus);
			ClearATCFlag(RequestTakeoff);
			SetATCFlag(Landed);
			curTaxiPoint = GetFirstPt(rwIndex);
			curTaxiPoint = GetNextPtLoop(curTaxiPoint);
			TranslatePointData(Airbase, curTaxiPoint, &trackX, &trackY);
			trackZ = af->groundZ;
			SimpleGroundTrack( 100.0F * KNOTS_TO_FTPSEC );
			break;
		}

		//don't do ground avoidance
		groundAvoidNeeded = FALSE;
		if(af->vt < af->CalcTASfromCAS(af->MinVcas() - 30.0F)*KNOTS_TO_FTPSEC ||
			(af->gearPos > 0.0F && af->vt < af->CalcTASfromCAS(af->MinVcas() + 10.0F)*KNOTS_TO_FTPSEC))
			af->gearHandle = 1.0F;
		if(cosAngle > 0.0F && af->groundZ - self->ZPos() > 50.0F)
		{
			curTaxiPoint = GetFirstPt(rwIndex);
			TranslatePointData (Airbase, curTaxiPoint, &trackX, &trackY);

			//until chris moves the landing points
			//trackX -= 500.0F * PtHeaderDataTable[rwIndex].cosHeading;
			//trackY -= 500.0F * PtHeaderDataTable[rwIndex].sinHeading;

			dx = trackX - self->XPos();
			dy = trackY - self->YPos();
			dist = (float)sqrt(dx*dx + dy*dy);

			//decelerate as we approach
			minSpeed = af->CalcTASfromCAS(af->MinVcas())*KNOTS_TO_FTPSEC;

			deltaTime = ((float)rwtime - (float)SimLibElapsedTime)/CampaignSeconds;
			testDist = (minSpeed*0.2F/(FINAL_TIME/CampaignSeconds)*deltaTime + minSpeed*0.6F)*deltaTime;			
			desiredSpeed = (minSpeed*0.4F/(FINAL_TIME/CampaignSeconds)*deltaTime + minSpeed*0.6F);

			if(dist > minSpeed*19.5F)
				desiredSpeed += (dist - testDist)/(testDist * 0.8F) * desiredSpeed;
			else if(desiredSpeed > af->vt)
				desiredSpeed = af->vt;
			
			desiredSpeed = max(min(minSpeed*1.2F, desiredSpeed), minSpeed*0.6F);
			finalZ = Airbase->brain->GetAltitude(self, lOnFinal);
			trackZ = finalZ - dist*TAN_THREE_DEG_GLIDE;

			//recalculate track point to help line up better
			trackX += dist * 0.8F * PtHeaderDataTable[rwIndex].cosHeading;
			trackY += dist * 0.8F * PtHeaderDataTable[rwIndex].sinHeading;

			testDist = 0;
			x = self->XPos();
			y = self->YPos();

			while(testDist < dist * 0.2F && dist > 3000.0F)
			{			
				x -= 200.0F * PtHeaderDataTable[rwIndex].cosHeading;
				y -= 200.0F * PtHeaderDataTable[rwIndex].sinHeading;

				z = OTWDriver.GetGroundLevel(x, y);
				if(dist < 6000.0F)
				{
					if(z - 100.0F < trackZ)
						trackZ = z - 100.0F;
				}
				else
				{
					if(z - 200.0F < trackZ)
						trackZ = z - 200.0F;
				}
				
				testDist += 200.0F;
			}
			if(af->groundZ - self->ZPos() > 200.0F)
				TrackPointLanding(desiredSpeed);
			else
			{
			    TrackPointLanding(af->GetLandingAoASpd());
#if 0 // JPO removed
				if(	self->GetSType() == STYPE_AIR_ATTACK ||
					self->GetSType() == STYPE_AIR_FIGHTER ||
					self->GetSType() == STYPE_AIR_FIGHTER_BOMBER ||
					self->GetSType() == STYPE_AIR_ECM)
						TrackPointLanding(af->CalcDesSpeed(12.5F));
				else
						TrackPointLanding(af->CalcDesSpeed(8.0F));
#endif
			}
		}
		else
		{
			//flare at the last minute so we hit softly
			curTaxiPoint = GetFirstPt(rwIndex);
			curTaxiPoint = GetNextPt(curTaxiPoint);
			TranslatePointData (Airbase, curTaxiPoint, &trackX, &trackY);
			trackZ = Airbase->brain->GetAltitude(self, atcstatus);	
			TrackPointLanding(af->GetLandingAoASpd());
#if 0
			if(	self->GetSType() == STYPE_AIR_ATTACK ||
				self->GetSType() == STYPE_AIR_FIGHTER ||
				self->GetSType() == STYPE_AIR_FIGHTER_BOMBER ||
				self->GetSType() == STYPE_AIR_ECM)
					TrackPointLanding(af->CalcDesSpeed(12.5F));
			else
					TrackPointLanding(af->CalcDesSpeed(8.0F));
#endif
			/*
			if(af->isF16)
				TrackPointLanding(af->MinVcas()* KNOTS_TO_FTPSEC*0.55F);
			else
				TrackPointLanding(af->MinVcas()* KNOTS_TO_FTPSEC*0.6F);
				*/
			pStick = - 0.01685393258427F;
		}
		break;

	//////////////////////////////////////////////////////////////////////////////////////
	case lLanded:
		// edg: If we're not on the ground, we're fucked (which has
		// been seen).  We can't stay in this state.  Go to aborted.
		if ( !self->OnGround() )
		{
			ShiWarning("Please show this to Dave P (x4373)");

			float rx	= self->dmx[0][0] * dx + self->dmx[0][1] * dy;

			if(rx > 3000.0F && af->IsSet(AirframeClass::OverRunway))
			{
				atcstatus = lOnFinal;
				SendATCMsg(atcstatus);
				ClearATCFlag(Landed);
				TrackPointLanding(af->MinVcas()* KNOTS_TO_FTPSEC*0.6F);
				pStick = - 0.01685393258427F;
			}
			else
			{
				atcstatus = lAborted;
				SendATCMsg(atcstatus);
				Airbase->brain->FindAbortPt(self, &trackX, &trackY, &trackZ);
				TrackPoint( 0.0F, af->MinVcas() * 1.2F * KNOTS_TO_FTPSEC);
				af->gearHandle = -1.0F;
			}
			break;
		}

		trackZ = af->groundZ;
		if (CloseToTrackPoint() )
		{
			// Are we there yet?
			switch(PtDataTable[GetNextPtLoop(curTaxiPoint)].type)
			{
			case TaxiPt:
				curTaxiPoint = GetNextPtLoop(curTaxiPoint);
				atcstatus = lTaxiOff;
				waittimer = 0;
				SendATCMsg(atcstatus);
				break;

			case TakeRunwayPt:
			case TakeoffPt:
				curTaxiPoint = GetNextPtLoop(curTaxiPoint);				
				break;

			case RunwayPt:
				//we shouldn't be here
				curTaxiPoint = GetNextPtLoop(curTaxiPoint);
				break;
			}

			trackZ = af->groundZ;
			TranslatePointData(Airbase, curTaxiPoint, &trackX, &trackY);
		}

		inTheWay = CheckTaxiTrackPoint();
		if ( inTheWay && inTheWay->Vt() < 10.0F)
		{
			heading = (float)atan2(trackX - self->XPos(), trackY - self->YPos() );
			mlSinCos(&Trig, heading);

			trackX += TAXI_CHECK_DIST * Trig.cos;
			trackY += TAXI_CHECK_DIST * Trig.sin;
		}

		// JPO pop the chute
		if (af->HasDragChute() && 
		    af->dragChute == AirframeClass::DRAGC_STOWED &&
		    af->vcas < af->DragChuteMaxSpeed()) {
		    af->dragChute = AirframeClass::DRAGC_DEPLOYED;
		}
		if(af->vt < af->MinVcas() * 0.5F) { // clean up
		    if (af->speedBrake > -1.0F)
			af->speedBrake = -1.0F;
		    if (af->tefPos > 0)
			af->TEFClose();
		    if (af->lefPos > 0)
			af->LEFClose();
			if (af->dragChute == AirframeClass::DRAGC_DEPLOYED)
				af->dragChute = AirframeClass::DRAGC_JETTISONNED;
		}
		

		dx = trackX - af->x;
		dy = trackY - af->y;
		if (dx*dx + dy*dy > 2000.0F * 2000.0F )
		{
			SimpleGroundTrack( 100.0F * KNOTS_TO_FTPSEC);
		}
		else if (dx*dx + dy*dy > 1000.0F * 1000.0F )
		{
			SimpleGroundTrack( 75.0F * KNOTS_TO_FTPSEC);
		}
		else
		{
			SimpleGroundTrack( 20.0F * KNOTS_TO_FTPSEC);
		}
		break;

	//////////////////////////////////////////////////////////////////////////////////////
	case lTaxiOff:

		// JPO - only halt when we really get there, so the above condition continues to fire.
		if(AtFinalTaxiPoint()) {
			if (waittimer == 0)
				waittimer = SimLibElapsedTime + g_nReagTimer * CampaignMinutes;
			dx = trackX - af->x;
			dy = trackY - af->y;
			if (dx*dx + dy * dy < 10 * 10)
				desiredSpeed = 0.0F;
			else
				desiredSpeed = 20.0F * KNOTS_TO_FTPSEC * (float)sqrt(dx*dx + dy* dy) / TAXI_CHECK_DIST;
			desiredSpeed = min(20.0F * KNOTS_TO_FTPSEC, desiredSpeed);
			if(waittimer < SimLibElapsedTime || (desiredSpeed == 0 && af->vt < 0.1f)) {
				// then clean up
				if (!af->canopyState) 
					af->CanopyToggle();
				else if (!af->IsSet(AirframeClass::EngineStopped)) {
					af->SetFlag(AirframeClass::EngineStopped);
				}
				else if (af->rpm < 0.05f &&
					self->MainPower() != AircraftClass::MainPowerOff) {
					self->DecMainPower();
				}
				else if (self != SimDriver.playerEntity && (g_nReagTimer <= 0 || waittimer < SimLibElapsedTime))				
					RegroupAircraft (self); //end of the line, time to pull you
			}
		}
		else if (CloseToTrackPoint()) // time to step along
		{
			switch(PtDataTable[GetNextPtLoop(curTaxiPoint)].type) {
			case TaxiPt: // nothing special
			default:
				curTaxiPoint = GetNextPtLoop(curTaxiPoint);
				break;
			case SmallParkPt:
			case LargeParkPt: // possible parking spot
				FindParkingSpot(Airbase);
				break;
			}
			// Are we there yet?
			switch(PtDataTable[curTaxiPoint].flags)
			{
			//this taxi point is in middle of list
			case 0:
				break;

			//this should be the runway pt, we shouldn't be here
			case PT_FIRST:
				break;

			case PT_LAST:
				if (waittimer == 0) // first time
					waittimer = SimLibElapsedTime + g_nReagTimer * CampaignMinutes;
				if(self != SimDriver.playerEntity && (g_nReagTimer <= 0 || waittimer < SimLibElapsedTime))
					RegroupAircraft (self); //end of the line, time to pull you
				break;
			}

			trackZ = af->groundZ;
			TranslatePointData(Airbase, curTaxiPoint, &trackX, &trackY);
			desiredSpeed = 20.0F * KNOTS_TO_FTPSEC;
		}
		else {
			desiredSpeed = 20.0F * KNOTS_TO_FTPSEC;
		}
		inTheWay = CheckTaxiTrackPoint();
		if ( inTheWay && inTheWay->Vt() < 10.0F) {
			switch (PtDataTable[curTaxiPoint].type ) {
			case SmallParkPt: //was a possible parking spot, alas no more.
			case LargeParkPt: // someone beat us to it.
				if (PtDataTable[curTaxiPoint].flags != PT_LAST) {
					FindParkingSpot(Airbase); // try again
					TranslatePointData(Airbase, curTaxiPoint, &trackX, &trackY);
					break;
				}
				// else fall
			default:
				OffsetTrackPoint(TAXI_CHECK_DIST, offRight); // JPO from offRight
				break;
			}

		}
		
		SimpleGroundTrack(desiredSpeed);
		break;

	default:
		break;
	}
}

int DigitalBrain::BestParkSpot(ObjectiveClass *Airbase)
{
	float x, y;
	int pktype = af->GetParkType();
	int npt, pt = curTaxiPoint;
	while ((npt = GetNextParkTypePt(pt, pktype)) != 0) // find last possible parking spot
		pt = npt;
	if (PtDataTable[pt].type != pktype) // just in case we found none
		return 0;
	while (pt > curTaxiPoint) {
		TranslatePointData(Airbase, pt, &x, &y);
		if (CheckPoint(x, y) == NULL) { // this is good?
			return pt;
		}
		pt = GetPrevParkTypePt(pt, pktype); // ok try another
	}
	return 0;
}

void DigitalBrain::FindParkingSpot(ObjectiveClass *Airbase)
{
#if 1
	int bestpt = BestParkSpot(Airbase);
	int pt = GetNextPtLoop(curTaxiPoint);
	while(PtDataTable[pt].flags != PT_LAST) {
		if (pt == bestpt) { // next taxi point is our favoured parking spot
			curTaxiPoint = pt;
			return;
		}
		else if (PtDataTable[pt].type == SmallParkPt || // this is not our aim, skip
			PtDataTable[pt].type == LargeParkPt) {
			pt = GetNextPtLoop(pt);
		}
		else {
			curTaxiPoint = pt; // keep on trucking
			return;
		}
	}
	curTaxiPoint = pt;
#else
	float x, y;
	int pt = GetNextPtLoop(curTaxiPoint);
	int pktype = af->GetParkType();

	while(PtDataTable[pt].flags != PT_LAST) {
		if (PtDataTable[pt].type == pktype) {
			TranslatePointData(Airbase, pt, &x, &y);
			if (CheckPoint(x, y) == NULL) {
				curTaxiPoint = pt;
				return;
			}
			pt ++; // try next
		}
		else if (PtDataTable[pt].type == SmallParkPt ||
			PtDataTable[pt].type == LargeParkPt) {
			pt = GetNextPtLoop(pt);
		}
		else { // not a parking spot, so carry on
			curTaxiPoint = pt;
			return;
		}
	}
	curTaxiPoint = pt;
#endif
}

// JPO - true if we're at last point, or at a suitable parking place
bool DigitalBrain::AtFinalTaxiPoint()
{
	// final point is one of these
	if (PtDataTable[curTaxiPoint].type == SmallParkPt ||
		PtDataTable[curTaxiPoint].type == LargeParkPt ||
		PtDataTable[curTaxiPoint].flags == PT_LAST) {
		return CloseToTrackPoint();
	}
	return false;
}

bool DigitalBrain::CloseToTrackPoint()
{
	if (fabs(trackX - af->x) < TAXI_CHECK_DIST && 
		fabs(trackY - af->y) < TAXI_CHECK_DIST) 
		return true;
	return false;
}

#ifdef DAVE_DBG
void DigitalBrain::SetDebugLabel(ObjectiveClass *Airbase)
{
   // Update the label in debug
   char tmpStr[40];
	runwayQueueStruct	*info = NULL;
	runwayQueueStruct	*testInfo = NULL;
	int pos = 0;

   if(atcstatus == noATC)
		sprintf (tmpStr,"noATC");
   else
   {
	   float xft, yft, rx;
	   int status;
	   //int delta = waittimer - SimLibElapsedTime;
	   int rwdelta = rwtime - SimLibElapsedTime;

	    xft = trackX - self->XPos();
		yft = trackY - self->YPos();
		// get relative position and az
		rx	= self->dmx[0][0] * xft + self->dmx[0][1] * yft;

		info = Airbase->brain->InList(self->Id());
		if(info)
		{
			status = info->status;
			if(info && info->rwindex)
			{
				runwayQueueStruct* testInfo = info;

				while(testInfo->prev)
				{
					testInfo = testInfo->prev;			
				}

				while(testInfo)
				{
					if(info->aircraftID == testInfo->aircraftID)
						break;
					pos++;
					testInfo = testInfo->next;
				}
			}
			//sprintf (tmpStr,"%d %d %d %d %4.2f %4.2f", pos,  curTaxiPoint, atcstatus, status, delta/1000.0F, rwdelta/1000.0F);
			sprintf (tmpStr,"%d %d %d %d %3.1f %3.1f %3.1f", pos,  curTaxiPoint, atcstatus, status, desiredSpeed, rwdelta/1000.0F, SimLibElapsedTime/60000.0F);
		}
		else
			sprintf (tmpStr,"%d %d %3.1f %3.1f %3.1f",  curTaxiPoint, atcstatus, desiredSpeed, rwdelta/1000.0F, SimLibElapsedTime/60000.0F);
			//sprintf (tmpStr,"%d %d %4.2f %4.2f",  curTaxiPoint, atcstatus, delta/1000.0F, rwdelta/1000.0F);
	   
   }
   if ( self->drawPointer )
   	  ((DrawableBSP*)self->drawPointer)->SetLabel (tmpStr, ((DrawableBSP*)self->drawPointer)->LabelColor());
}
#endif

void DigitalBrain::TakeOff (void)
{
	SimBaseClass	*inTheWay = NULL;
	ObjectiveClass	*Airbase = (ObjectiveClass *)vuDatabase->Find(airbase);
	float			xft, yft, rx, distSq;

	if(!Airbase || !Airbase->IsObjective() || !Airbase->brain)
	{
		//need to find the airbase or we don't know where to go
		// JB carrier ShiWarning("Show this to Dave P. (no airbase)");
		return;
	}

	if(self->IsSetFlag(ON_GROUND) && af->Fuel() <= 0.0F && af->vt < 5.0F)
	{
		if(self != SimDriver.playerEntity)
				RegroupAircraft (self); //no gas get him out of the way
	}

	if(af->IsSet(AirframeClass::EngineOff))
		af->ClearFlag(AirframeClass::EngineOff);
	if(af->IsSet(AirframeClass::ThrottleCheck))
		af->ClearFlag(AirframeClass::ThrottleCheck);

	SetDebugLabel(Airbase);

//me123 make sure ap if off for player in multiplay
	if(self == SimDriver.playerEntity && IsSetATC(StopPlane))
		 {
			ClearATCFlag(StopPlane);
			af->ClearFlag(AirframeClass::WheelBrakes);
			self->SetAutopilot(AircraftClass::APOff);
			return;
		 }
	// JPO - should we run preflight checks...
	if (!IsSetATC(DonePreflight) && curTaxiPoint) {
		VU_TIME t2t; // time to takeoff
		if (rwtime > 0) // value given by ATC
			t2t = rwtime;
		else 
			t2t = self->curWaypoint->GetWPDepartureTime(); // else original scheduled time

		if (SimLibElapsedTime > t2t - 3 * CampaignMinutes) // emergency pre-flight
			QuickPreFlight();
		else if (SimLibElapsedTime < t2t - PlayerOptionsClass::RAMP_MINUTES * CampaignMinutes)
			return; // not time for startup yet
		else if (PtDataTable[curTaxiPoint].flags == PT_LAST ||
			PtDataTable[curTaxiPoint].type == SmallParkPt ||
			PtDataTable[curTaxiPoint].type == LargeParkPt) {
			if (!PreFlight()) // slow preflight
				return;
		}
		else 
			QuickPreFlight();
		SetATCFlag(DonePreflight);
	}

    // if we're damaged taxi back
	if ( self->pctStrength < 0.7f && self->IsSetFlag(ON_GROUND))
	{
		TaxiBack(Airbase);
		return;
	}

	xft = trackX - af->x;
	yft = trackY - af->y;
	rx	= self->dmx[0][0] * xft + self->dmx[0][1] * yft;
	distSq = xft*xft + yft*yft;

	groundAvoidNeeded = FALSE;

	if(!curTaxiPoint)
	{
		Flight flight = (Flight)self->GetCampaignObject();
		if(flight)
		{
			WayPoint	w;
	
			w = flight->GetFirstUnitWP();
			if (w)
			{
				curTaxiPoint = FindDesiredTaxiPoint(w->GetWPDepartureTime());
				TranslatePointData(Airbase, curTaxiPoint, &trackX, &trackY);
				CalcWaitTime(Airbase->brain);
			}
		}
	}

	if (g_nShowDebugLabels & 0x80000)
	{
		char label [32];
		sprintf(label,"TaxiPt %d, type: ",curTaxiPoint);
		switch (PtDataTable[curTaxiPoint].type)
		{
		case SmallParkPt:
			strcat(label,"SmallParkPt");
			break;
		case LargeParkPt:
			strcat(label,"LargeParkPt");
			break;
		case TakeoffPt:	
			strcat(label,"TakeoffPt");
			break;
		case RunwayPt:
			strcat(label,"RunwayPt");
			break;
		case TaxiPt:
			strcat(label,"TaxiPt");
			break;
		case CritTaxiPt:
			strcat(label,"CritTaxiPt");
			break;
		case TakeRunwayPt:
			strcat(label,"TakeRunwayPt");
			break;
		}
		if (self->drawPointer)
			((DrawableBSP*)self->drawPointer)->SetLabel(label,((DrawableBSP*)self->drawPointer)->LabelColor());
	}


	switch(atcstatus)
	{
	case noATC:
		if(!self->IsSetFlag(ON_GROUND))
		{	
//			ShiAssert(!"Show this to Dave P. (not on ground)");
			if(isWing)
				self->curWaypoint = self->curWaypoint->GetNextWP();
			else
				SelectNextWaypoint();			
			break;
		}
		
		trackZ = af->groundZ;
		ClearATCFlag(RequestTakeoff);
		ClearATCFlag(PermitRunway);
		ClearATCFlag(PermitTakeoff);
		
		switch(PtDataTable[curTaxiPoint].type)
		{
		case SmallParkPt:
		case LargeParkPt:
			atcstatus = tReqTakeoff;
			waittimer = SimLibElapsedTime + TAKEOFF_TIME_DELTA;
			TranslatePointData(Airbase, curTaxiPoint, &trackX, &trackY);
			if( !isWing )
			{
				SendATCMsg(tReqTakeoff);
			}
			break;

		case TakeoffPt:	
			atcstatus = tReqTakeoff;
			rwIndex = Airbase->brain->IsOnRunway(self);
			if(GetFirstPt(rwIndex) == curTaxiPoint - 1)
				curTaxiPoint = Airbase->brain->FindTakeoffPt((Flight)self->GetCampaignObject(), self->vehicleInUnit, rwIndex, &trackX, &trackY);
			else
			{
				rwIndex = Airbase->brain->GetOppositeRunway(rwIndex);
				curTaxiPoint = Airbase->brain->FindTakeoffPt((Flight)self->GetCampaignObject(), self->vehicleInUnit, rwIndex, &trackX, &trackY);
			}
			waittimer = SimLibElapsedTime;
			if( !isWing )
			{
				SendATCMsg(atcstatus);
			}
			break;

		case RunwayPt:
			atcstatus = tReqTakeoff;
			rwIndex = Airbase->brain->IsOnRunway(self);
			if(GetFirstPt(rwIndex) == curTaxiPoint)
				curTaxiPoint = Airbase->brain->FindRunwayPt((Flight)self->GetCampaignObject(), self->vehicleInUnit, rwIndex, &trackX, &trackY);
			else
			{
				rwIndex = Airbase->brain->GetOppositeRunway(rwIndex);
				curTaxiPoint = Airbase->brain->FindRunwayPt((Flight)self->GetCampaignObject(), self->vehicleInUnit, rwIndex, &trackX, &trackY);
			}
			waittimer = SimLibElapsedTime;
			if( !isWing )
			{
				SendATCMsg(tReqTakeoff);
			}
			break;

		case TaxiPt:
		case CritTaxiPt:
		case TakeRunwayPt:
		default:
			atcstatus = tReqTakeoff;
			TranslatePointData(Airbase, curTaxiPoint, &trackX, &trackY);
			waittimer = SimLibElapsedTime + TAKEOFF_TIME_DELTA;
			if( !isWing )
			{
				SendATCMsg(tReqTakeoff);
			}
			break;
		}

		SimpleGroundTrack(0.0F);		
		break;

	//////////////////////////////////////////////////////////////////////////////////////
	case tReqTakeoff:
	case tReqTaxi:
		if(SimLibElapsedTime > waittimer + TAKEOFF_TIME_DELTA)
		{
			//we've been waiting too long, call again
			SendATCMsg(atcstatus);
			waittimer = SimLibElapsedTime;
		}
		//we're waiting to get a response back
		SimpleGroundTrack(0.0F);	
		break;

	//////////////////////////////////////////////////////////////////////////////////////
	case tWait:
		desiredSpeed = 0.0F;
		if ( (distSq < TAXI_CHECK_DIST * TAXI_CHECK_DIST) || (rx < 1.0F && distSq < TAXI_CHECK_DIST*TAXI_CHECK_DIST*4.0F))
			ChooseNextPoint(Airbase);
		else
		{		
			inTheWay = CheckTaxiTrackPoint();
			if ( inTheWay )
			{
				if(isWing)
					DealWithBlocker(inTheWay, Airbase);
			}
			else
			{
				//default speed
				if(SimLibElapsedTime > waittimer + TAKEOFF_TIME_DELTA)
					CalculateTaxiSpeed(3.0F);
				else
					CalculateTaxiSpeed(5.0F);
			}
			trackZ = af->groundZ;
		}			
		
		SimpleGroundTrack( desiredSpeed );
		break;

	//////////////////////////////////////////////////////////////////////////////////////
	case tTaxi:		
		desiredSpeed = 0.0F;
		//if we haven't reached our desired taxi point, we need to move
		if ( (distSq < TAXI_CHECK_DIST * TAXI_CHECK_DIST) || (rx < 1.0F && distSq < TAXI_CHECK_DIST*TAXI_CHECK_DIST*4.0F))
			ChooseNextPoint(Airbase);
		else
		{	
			inTheWay = CheckTaxiTrackPoint();
			if ( inTheWay )
			{
				//someone is in the way
				DealWithBlocker(inTheWay, Airbase);
			}
			else
			{
				//default speed
				if(SimLibElapsedTime > waittimer + TAKEOFF_TIME_DELTA)
					CalculateTaxiSpeed(3.0F);
				else
					CalculateTaxiSpeed(5.0F);
			}
			trackZ = af->groundZ;
		}
		
		SimpleGroundTrack( desiredSpeed );
		break;

	//////////////////////////////////////////////////////////////////////////////////////
	case tHoldShort:
		desiredSpeed = 0.0F;
		if(rwtime < 5 * CampaignSeconds + SimLibElapsedTime && waittimer < SimLibElapsedTime)
		{
			SendATCMsg(atcstatus);
			waittimer = CalcWaitTime(Airbase->brain);
		}

		ChooseNextPoint(Airbase);

		if(desiredSpeed == 0.0F && Airbase->brain->IsOnRunway(trackX, trackY))
		{			
			OffsetTrackPoint(50.0F, rightRunway);
			CalculateTaxiSpeed(5.0F);
		}

		SimpleGroundTrack( desiredSpeed );
		break;

	//////////////////////////////////////////////////////////////////////////////////////
	case tTakeRunway:
		desiredSpeed = 0.0F;
		//if we haven't reached our desired taxi point, we need to move
		if ( (distSq < TAXI_CHECK_DIST * TAXI_CHECK_DIST) || (rx < 0.0F && distSq < TAXI_CHECK_DIST*TAXI_CHECK_DIST*4.0F))
		{
			if(PtDataTable[curTaxiPoint].type != RunwayPt)
				ChooseNextPoint(Airbase);
		}
		else
		{		
			inTheWay = CheckTaxiTrackPoint();
			if ( inTheWay )
				DealWithBlocker(inTheWay, Airbase);
			else 
				CalculateTaxiSpeed(5.0F);
			trackZ = af->groundZ;
		}

		if(PtDataTable[curTaxiPoint].type == RunwayPt)
		{
			if(isWing && self->af->IsSet(AirframeClass::OverRunway) && !WingmanTakeRunway(Airbase) )
			{
				curTaxiPoint = Airbase->brain->FindTakeoffPt((Flight)self->GetCampaignObject(), self->vehicleInUnit, rwIndex, &trackX, &trackY);
				OffsetTrackPoint(50.0F, rightRunway);
				CalculateTaxiSpeed(5.0F);
				curTaxiPoint++;
			}
			else if(!self->af->IsSet(AirframeClass::OverRunway))
			{
				OffsetTrackPoint(0.0F, centerRunway);
				curTaxiPoint++;
			}
			else
			{
				float cosAngle =	self->platformAngles->sinsig * PtHeaderDataTable[rwIndex].sinHeading + 
									self->platformAngles->cossig * PtHeaderDataTable[rwIndex].cosHeading;
			
				if(cosAngle >  0.99619F)
				{
					if(ReadyToGo())
					{
						waittimer = 0;
						atcstatus = tTakeoff;
						SendATCMsg(atcstatus);
						trackZ = af->groundZ - 500.0F;
					}
					else
						desiredSpeed = 0.0F;
				}
			}
		}

		SimpleGroundTrack( desiredSpeed );

		// edg: test for fuckupedness -- I've seen planes taking the runway
		// which are already in the air (bad trackX and Y?).  They never
		// get out of this take runway cycle.   If we find ourselves in
		// this state go to take off since we're off already....
		if ( !self->OnGround() )
		{
			ShiWarning("Show this to Dave P. (not on ground)");
			waittimer = 0;
			atcstatus = tTakeoff;
			SendATCMsg(atcstatus);
		}
		break;

	//////////////////////////////////////////////////////////////////////////////////////
	case tTakeoff:
		if(self->OnGround() && !self->af->IsSet(AirframeClass::OverRunway))
		{
			curTaxiPoint = Airbase->brain->FindTakeoffPt((Flight)self->GetCampaignObject(), self->vehicleInUnit, rwIndex, &trackX, &trackY);
			atcstatus = tTakeRunway;
			SendATCMsg(atcstatus);
			CalculateTaxiSpeed(5.0F);
			SimpleGroundTrack( desiredSpeed );
			return;
		}

		if(self->OnGround())
			SimpleGroundTrack(af->MinVcas() * KNOTS_TO_FTPSEC);
		else
			TrackPoint(0.0F, (af->MinVcas() + 50.0F) * KNOTS_TO_FTPSEC);

		if(af->z - af->groundZ < -20.0F && af->gearHandle > -1.0F)
			af->gearHandle = -1.0F;

		if (af->z - af->groundZ < -50.0F)
		{
			if(af->gearHandle > -1.0F)
				af->gearHandle = -1.0F;
			atcstatus = tFlyOut;
			SendATCMsg(atcstatus);
		}		
		break;

	//////////////////////////////////////////////////////////////////////////////////////
	case tFlyOut:
		int				elements;
		trackZ = af->groundZ - 500.0F;
		if(af->gearHandle > -1.0F) 
			af->gearHandle = -1.0F;

// 2001-10-16 added by M.N. #1 performs a 90° base leg to waypoint 2 at takeoff
		
// Needed so that lead that will perform the leg will first fly out before it starts the leg
		if ( af->z - af->groundZ > -200.0F || (fabs(xft) < 200.0F && fabs(yft) < 200.0F) )
			break;

		elements = self->GetCampaignObject()->NumberOfComponents();

		// 2001-10-16 M.N. added elemleader check -> perform a 90° base leg until element lead has taken off
		if (!isWing && elements > 1) // Code for the flightlead
		{
			AircraftClass *elemleader = (AircraftClass*)self->GetCampaignObject()->GetComponentNumber(elements == 2 ? 1 : 2); // wingy or lead
			if (elemleader) // is #3 in a 4- and 3-ship flight, #2 in a 2-ship flight
			{
				airbase = self->LandingAirbase(); // JPO - now we set to go home
				ObjectiveClass *Airbase = (ObjectiveClass *)vuDatabase->Find(airbase);
				if (!Airbase || elemleader->af->z - elemleader->af->groundZ < -50.0F) // #3 has taken off -> lead continue to next WP
				{
					onStation = NotThereYet;
					SelectNextWaypoint();
					atcstatus = noATC;
					SendATCMsg(atcstatus);
					
				}
				else 
				{	// #1 and #2 do a takeoff leg - find direction to next waypoint

					int dir;
					float			tx,ty,dx,dy,dz,dist;
					float			deltaHdg, hdgToPt, acHeading, legHeading;

					acHeading = self->Yaw(); // fix, use aircrafts real heading instead of runway heading

					WayPointClass* tmpWaypoint = self->curWaypoint;
					if (tmpWaypoint)
					{
						ShiAssert (tmpWaypoint->GetWPAction() == WP_TAKEOFF);
// add this if we have	if (tmpWaypoint->GetWPAction() != WP_TAKEOFF && tmpWaypoint->GetPrevWP())
// a failed assertion		tmpWaypoint = tmpWaypoint->GetPrevWP();
					
					tmpWaypoint = tmpWaypoint->GetNextWP();
					tmpWaypoint->GetLocation(&dx,&dy,&dz);
	
					tx = dx - Airbase->XPos();
					ty = dy - Airbase->YPos();
					hdgToPt = (float)atan2(ty, tx);
					if(hdgToPt < 0.0F)
						hdgToPt += PI * 2.0F;

	
					if(acHeading < 0.0F)
						acHeading += PI * 2.0F;

					deltaHdg = hdgToPt - acHeading;
					if(deltaHdg > PI)
						deltaHdg -= (2.0F*PI);
					else if(deltaHdg < -PI)
					deltaHdg += (2.0F*PI);
					if(deltaHdg < -PI)
						dir = 1;
					else if(deltaHdg > PI)
						dir = 0;
					else if(deltaHdg < 0.0F)
						dir = 0;//left
					else
						dir = 1;//right
					
					legHeading = hdgToPt;

					// MN CTD fix #2
					AircraftClass *wingman = NULL;
					FlightClass *flight = NULL;
					flight = (FlightClass*)self->GetCampaignObject();
					ShiAssert(!F4IsBadReadPtr(flight,sizeof(FlightClass)));
					float factor = 0.0F;
					if (flight)
					{
						wingman = (AircraftClass*)flight->GetComponentNumber(1); // my wingy = 1 in each case
						ShiAssert(!F4IsBadReadPtr(wingman,sizeof(AircraftClass)));
					
						if (wingman && wingman->af && (wingman->af->z - wingman->af->groundZ < -200.0F))	// My wingman has flown out
							factor = 45.0F * DTR;
					
						ShiAssert(Airbase->brain);
						if (Airbase->brain && Airbase->brain->UseSectionTakeoff(flight, rwIndex))	// If our wingman took off with us, stay on a 90° leg
							factor = 0.0F;
					}
					
					if (dir)
						legHeading = legHeading - (90.0F * DTR - factor);
					else
						legHeading = legHeading + (90.0F * DTR - factor);

					if (legHeading >=360.0F * DTR)
						legHeading -= 360.0F * DTR;
					if (legHeading < 0.0F)
						legHeading += 360.0F * DTR;

					dist = 10.0F * NM_TO_FT;

					// Set up a new trackpoint

					dx = Airbase->XPos() + dist * cos (legHeading);
					dy = Airbase->YPos() + dist * sin (legHeading);

					trackX = dx;
					trackY = dy;
					trackZ = -2000.0F + af->groundZ;

					SetMaxRollDelta(75.0F); // don't roll too much
					SimpleTrack(SimpleTrackSpd, (af->MinVcas() * 1.2)); // fly as slow as possible ~ 178 kts
					SetMaxRollDelta(100.0F);
					break;
					}

				}
			}
		}
		else
		// In the air and ready to go
		if ( af->z - af->groundZ < -200.0F || (fabs(xft) < 200.0F && fabs(yft) < 200.0F) )
		{
			 onStation = NotThereYet;
			 if(isWing)
				self->curWaypoint = self->curWaypoint->GetNextWP();
			else
				SelectNextWaypoint();
			 atcstatus = noATC;
			 SendATCMsg(atcstatus);
#ifdef DAVE_DBG
			 SetLabel(self);
#endif
			
			 if (isWing)
				AiRejoin(NULL, AI_TAKEOFF); // JPO actually a takeoff signal
			 airbase = self->LandingAirbase(); // JPO - now we set to go home
		}
		
		TrackPoint(0.0F, (af->MinVcas() + 50.0F) * KNOTS_TO_FTPSEC);
		break;

	//////////////////////////////////////////////////////////////////////////////////////
	case tEmerStop:
		desiredSpeed = 0.0F;
		if(waittimer < SimLibElapsedTime)
		{
			SendATCMsg(atcstatus);
			waittimer = CalcWaitTime(Airbase->brain);
		}

		if(!isWing || (flightLead && flightLead->OnGround()))
		{
			while(Airbase->brain->IsOnRunway(trackX, trackY))
			{			
				OffsetTrackPoint(20.0F, rightRunway);
				desiredSpeed = 20.0F;
			}

			if(Airbase->brain->IsOnRunway(self) && af->vt < 5.0F)
			{
				OffsetTrackPoint(20.0F, rightRunway);
				desiredSpeed = 20.0F;
			}
		}
		
		if (fabs(trackX - af->x) > TAXI_CHECK_DIST || fabs(trackY - af->y) > TAXI_CHECK_DIST )
		{
			inTheWay = CheckTaxiTrackPoint();
			if ( inTheWay )
			{
				if(isWing)
					DealWithBlocker(inTheWay, Airbase);
				else
					desiredSpeed = 0.0F;
			}
			else
			{
				//default speed
				CalculateTaxiSpeed(5.0F);
			}
		}
		else
			ChooseNextPoint(Airbase);

		SimpleGroundTrack(desiredSpeed);
		break;

	//////////////////////////////////////////////////////////////////////////////////////
	case tTaxiBack:
		//this will cause them to taxi back, will only occur if ordered by ATC
		TaxiBack(Airbase);
		break;

	default:
		break;
	}
}

int	DigitalBrain::WingmanTakeRunway(ObjectiveClass	*Airbase)
{
	AircraftClass *leader = (AircraftClass*)self->GetCampaignObject()->GetComponentNumber(self->vehicleInUnit - 1);

	return WingmanTakeRunway(Airbase, (AircraftClass *)flightLead, leader);
}

int	DigitalBrain::WingmanTakeRunway(ObjectiveClass	*Airbase, AircraftClass *FlightLead, AircraftClass *leader)
{
	int pt;
	float tempX, tempY;
	//when this function is called, I already know that the point will not move me past any wingmen in front
	//of me unless they are gone or off the ground

	switch(self->vehicleInUnit)
	{
	case 0:
		ShiWarning("This should never happen");
		return TRUE;
		break;

	case 1:	
		pt = GetPrevPtLoop(curTaxiPoint);
		
		TranslatePointData(Airbase, pt, &tempX, &tempY);
		if(!Airbase->brain->IsOnRunway(tempX, tempY))
			return TRUE;
		else if(!FlightLead)
			return TRUE;
		else if(!FlightLead->OnGround())
			return TRUE;
		else if(Airbase->brain->IsOnRunway(FlightLead)  && Airbase->brain->UseSectionTakeoff((Flight)self->GetCampaignObject(),rwIndex))
			return TRUE;
		break;

	case 2:
		pt = GetPrevPtLoop(curTaxiPoint);
		
		TranslatePointData(Airbase, pt, &tempX, &tempY);
		if(!Airbase->brain->IsOnRunway(tempX, tempY))
			return TRUE;

		if(FlightLead && FlightLead->OnGround())
			return FALSE;

		if(leader && leader->OnGround())
			return FALSE;

		return TRUE;
		break;

	default:
	case 3:
		pt = GetPrevPtLoop(curTaxiPoint);
		
		TranslatePointData(Airbase, pt, &tempX, &tempY);
		if(!Airbase->brain->IsOnRunway(tempX, tempY))
			return TRUE;

		if(FlightLead && FlightLead->OnGround())
			return FALSE;

		FlightLead = (AircraftClass*)self->GetCampaignObject()->GetComponentNumber(1);
		if(FlightLead && FlightLead->OnGround())
			return FALSE;

		if(!leader)
			return TRUE;
		else if(!leader->OnGround())
			return TRUE;
		else if(Airbase->brain->IsOnRunway(leader) && Airbase->brain->UseSectionTakeoff((Flight)self->GetCampaignObject(),rwIndex))
			return TRUE;
		break;
	}

	return FALSE;
}

void DigitalBrain::DealWithBlocker(SimBaseClass *inTheWay, ObjectiveClass *Airbase)
{
	float tmpX=0.0F, tmpY=0.0F, ry=0.0F;
	int extraWait = 0;

	SimBaseClass *inTheWay2 = NULL;

	desiredSpeed = 0.0F;	
	
	if(inTheWay->GetCampaignObject() == self->GetCampaignObject() && self->vehicleInUnit > ((AircraftClass*)inTheWay)->vehicleInUnit)
	{
		return;//we never taxi around fellow flight members
	}

	switch(PtDataTable[curTaxiPoint].type)
	{	
	case TakeoffPt:
		curTaxiPoint = Airbase->brain->FindTakeoffPt((Flight)self->GetCampaignObject(), self->vehicleInUnit, rwIndex, &tmpX, &tmpY);
		break;
	case RunwayPt:
		curTaxiPoint = Airbase->brain->FindRunwayPt((Flight)self->GetCampaignObject(), self->vehicleInUnit, rwIndex, &trackX, &trackY);
		break;

	default:
	case TaxiPt:
		TranslatePointData(Airbase, curTaxiPoint, &tmpX, &tmpY);
		break;
	}

	if(tmpX != trackX || tmpY != trackY)
		inTheWay2 = CheckPoint (tmpX, tmpY);
	else
		inTheWay2 = inTheWay;

	if(atcstatus != tTakeRunway)
	{
		extraWait = FalconLocalGame->rules.AiPatience;

		if(rwtime > SimLibElapsedTime)
			extraWait += (rwtime - SimLibElapsedTime) / 10;
	}

	if(!inTheWay2 && (!inTheWay->IsAirplane() || ((AircraftClass*)inTheWay)->DBrain()->RwTime() > rwtime) )
	{
		trackX = tmpX;
		trackY = tmpY;
		waittimer = CalcWaitTime(Airbase->brain);
		if(SimLibElapsedTime > waittimer + TAKEOFF_TIME_DELTA)
			CalculateTaxiSpeed(3.0F);
		else
			CalculateTaxiSpeed(5.0F);
	}
	else if((isWing && (!inTheWay2 || inTheWay2->GetCampaignObject() != self->GetCampaignObject() || self->vehicleInUnit < ((AircraftClass*)inTheWay2)->vehicleInUnit)) || 
			(inTheWay->Vt() < 5.0F && SimLibElapsedTime > waittimer + extraWait) || 
			(inTheWay->IsAirplane() && ((AircraftClass*)inTheWay)->DBrain()->RwTime() > rwtime))
	{
		tmpX = inTheWay->XPos() - self->XPos();
		tmpY = inTheWay->YPos() - self->YPos();
		ry	= self->dmx[1][0] * tmpX + self->dmx[1][1] * tmpY;

		switch(PtDataTable[GetPrevPtLoop(curTaxiPoint)].type)
		{
		default:
		case CritTaxiPt:
		case TaxiPt:
			while(CheckTaxiTrackPoint() == inTheWay)
			{
				if(ry > 0.0F)
					OffsetTrackPoint(10.0F, offLeft);
				else
					OffsetTrackPoint(10.0F, offRight);
			}
			CalculateTaxiSpeed(5.0F);
			break;

		case TakeRunwayPt:
			//take runway if we have permission, else holdshort
			if(isWing || IsSetATC(PermitRunway) || IsSetATC(PermitTakeRunway))
			{					
				while(CheckTaxiTrackPoint() == inTheWay)
				{
					if(ry > 0.0F)
						OffsetTrackPoint(10.0F, offLeft);
					else
						OffsetTrackPoint(10.0F, offRight);
				}
				CalculateTaxiSpeed(3.0F);
			}
			else if(PtDataTable[curTaxiPoint].type == TakeRunwayPt)
			{
				while(CheckTaxiTrackPoint() == inTheWay)
				{
					if(ry > 0.0F)
						OffsetTrackPoint(10.0F, offLeft);
					else
						OffsetTrackPoint(10.0F, offRight);
				}
				CalculateTaxiSpeed(3.0F);
			}
			break;

		case RunwayPt:
		case TakeoffPt:		
			//take runway if we have permission, else holdshort
			if(isWing || IsSetATC(PermitRunway))
			{
				while(CheckTaxiTrackPoint() == inTheWay)
					OffsetTrackPoint(20.0F, downRunway);
				CalculateTaxiSpeed(3.0F);
			}
			break;
		}
	} 
}

void DigitalBrain::ChooseNextPoint(ObjectiveClass	*Airbase)
{
	desiredSpeed = 0.0F;
	AircraftClass *leader = NULL;
	int minPoint = GetFirstPt(rwIndex);	
	//we arrived at our point what next?
	if(isWing)
	{
		leader = (AircraftClass*)self->GetCampaignObject()->GetComponentNumber(self->vehicleInUnit - 1);
		if(leader && leader->OnGround() && leader->DBrain()->ATCStatus() != tTaxiBack)
			minPoint = leader->DBrain()->GetTaxiPoint();

		if(	flightLead && flightLead->OnGround() && 
			((AircraftClass*)flightLead)->DBrain()->GetTaxiPoint() > minPoint && 
			((AircraftClass*)flightLead)->DBrain()->ATCStatus() != tTaxiBack	)
				minPoint = ((AircraftClass*)flightLead)->DBrain()->GetTaxiPoint();
	}
	else if( SimLibElapsedTime < waittimer && !IsSetATC(PermitRunway) && !IsSetATC(PermitTakeRunway))
		return;

	switch(PtDataTable[GetPrevPtLoop(curTaxiPoint)].type)
	{
	case LargeParkPt:
	case SmallParkPt: // skip these on taxi
		if(isWing && GetPrevPtLoop(curTaxiPoint) <= minPoint)
			return;
		else {
			//just taxi along
			int pt = GetPrevTaxiPt(curTaxiPoint);
			if (pt == 0) return;
			curTaxiPoint = pt;
			TranslatePointData(Airbase, curTaxiPoint, &trackX, &trackY);
			CalculateTaxiSpeed(5.0F);
			waittimer = CalcWaitTime(Airbase->brain);
		}
		break;
		
	default:
	case CritTaxiPt:
	case TaxiPt:
		if(isWing && GetPrevPtLoop(curTaxiPoint) <= minPoint)
			return;
		//just taxi along
		curTaxiPoint = GetPrevPtLoop(curTaxiPoint);
		TranslatePointData(Airbase, curTaxiPoint, &trackX, &trackY);
		CalculateTaxiSpeed(5.0F);
		waittimer = CalcWaitTime(Airbase->brain);
		break;

	case TakeRunwayPt:
		//take runway if we have permission, else holdshort
		if(isWing)
		{
			if(GetPrevPtLoop(curTaxiPoint) == minPoint && !IsSetATC(PermitRunway))
				return;
			if(GetPrevPtLoop(curTaxiPoint) < minPoint)
				return;
			if(WingmanTakeRunway(Airbase, (AircraftClass*)flightLead, leader))
			{
				curTaxiPoint = GetPrevPtLoop(curTaxiPoint);
				TranslatePointData(Airbase, curTaxiPoint, &trackX, &trackY);
				CalculateTaxiSpeed(5.0F);
				waittimer = CalcWaitTime(Airbase->brain);
			}
			else if(!Airbase->brain->IsOnRunway(GetPrevPtLoop(GetPrevPtLoop(curTaxiPoint))))
			{
				curTaxiPoint = GetPrevPtLoop(GetPrevPtLoop(curTaxiPoint));
				TranslatePointData(Airbase, curTaxiPoint, &trackX, &trackY);
				CalculateTaxiSpeed(3.0F);
				waittimer = CalcWaitTime(Airbase->brain);
			}
			else if(self->af->IsSet(AirframeClass::OverRunway))
			{
				OffsetTrackPoint(50.0F, rightRunway);
				CalculateTaxiSpeed(5.0F);
			}
		}
		else if( IsSetATC(PermitRunway) && !self->IsSetFalcFlag(FEC_HOLDSHORT) )
		{					
			atcstatus = tTakeRunway;
			curTaxiPoint = GetPrevPtLoop(curTaxiPoint);
			TranslatePointData(Airbase, curTaxiPoint, &trackX, &trackY);
			CalculateTaxiSpeed(5.0F);
			waittimer = CalcWaitTime(Airbase->brain);
		}
		else if( IsSetATC(PermitTakeRunway) && !self->IsSetFalcFlag(FEC_HOLDSHORT) )
		{
			int pt = GetPrevPtLoop(curTaxiPoint);
			float tempX, tempY;
			TranslatePointData(Airbase, pt, &tempX, &tempY);
			if(!Airbase->brain->IsOnRunway(tempX, tempY))
			{
				curTaxiPoint = pt;
				trackX = tempX;
				trackY = tempY;
				CalculateTaxiSpeed(5.0F);
				waittimer = CalcWaitTime(Airbase->brain);
			}
		}
		else if(PtDataTable[curTaxiPoint].type == TakeRunwayPt && !self->IsSetFalcFlag(FEC_HOLDSHORT) )
		{
			SetATCFlag(PermitTakeRunway);					
			curTaxiPoint = GetPrevPtLoop(curTaxiPoint);
			TranslatePointData(Airbase, curTaxiPoint, &trackX, &trackY);
			CalculateTaxiSpeed(5.0F);	
			if(atcstatus != tTakeRunway)
			{
				SendATCMsg(tPrepToTakeRunway);
				atcstatus = tTaxi;
			}
			waittimer = CalcWaitTime(Airbase->brain);
		}
		else if(!Airbase->brain->IsOnRunway(GetPrevPtLoop(GetPrevPtLoop(curTaxiPoint))))
		{
			curTaxiPoint = GetPrevPtLoop(GetPrevPtLoop(curTaxiPoint));
			TranslatePointData(Airbase, curTaxiPoint, &trackX, &trackY);
			CalculateTaxiSpeed(3.0F);
			waittimer = CalcWaitTime(Airbase->brain);
		}
		else if(self->af->IsSet(AirframeClass::OverRunway))
		{
			OffsetTrackPoint(50.0F, rightRunway);
			CalculateTaxiSpeed(5.0F);
		}
		else
		{
			if(atcstatus != tTakeRunway && !self->IsSetFalcFlag(FEC_HOLDSHORT))
			{
				atcstatus = tHoldShort;
				SendATCMsg(atcstatus);
			}
		}
		break;

	case TakeoffPt:
		//take runway if we have permission, else holdshort
		if(isWing)
		{
			if(GetPrevPtLoop(curTaxiPoint) < minPoint)
				return;

			if(WingmanTakeRunway(Airbase, (AircraftClass*)flightLead, leader))
			{
				curTaxiPoint = Airbase->brain->FindTakeoffPt((Flight)self->GetCampaignObject(), self->vehicleInUnit, rwIndex, &trackX, &trackY);
				CalculateTaxiSpeed(5.0F);
				if(atcstatus != tTakeRunway)
				{
					atcstatus = tTakeRunway;
					SendATCMsg(atcstatus);
				}
				waittimer = CalcWaitTime(Airbase->brain);
			}
			else if(self->af->IsSet(AirframeClass::OverRunway))
			{
				OffsetTrackPoint(50.0F, rightRunway);
				CalculateTaxiSpeed(5.0F);
			}
		}
		else if(IsSetATC(PermitRunway) && !self->IsSetFalcFlag(FEC_HOLDSHORT))
		{
			SetATCFlag(PermitRunway);					
			curTaxiPoint = Airbase->brain->FindTakeoffPt((Flight)self->GetCampaignObject(), self->vehicleInUnit, rwIndex, &trackX, &trackY);
			CalculateTaxiSpeed(5.0F);
			if(atcstatus != tTakeRunway)
			{
				atcstatus = tTakeRunway;
				SendATCMsg(atcstatus);
			}
			waittimer = CalcWaitTime(Airbase->brain);
		}
		else if(self->af->IsSet(AirframeClass::OverRunway))
		{
			OffsetTrackPoint(50.0F, rightRunway);
			CalculateTaxiSpeed(5.0F);
		}
		else
		{
			if(	PtDataTable[curTaxiPoint].type != TakeRunwayPt && !IsSetATC(PermitTakeRunway) && 
				!self->IsSetFalcFlag(FEC_HOLDSHORT))
			{
				atcstatus = tHoldShort;
				SendATCMsg(atcstatus);
			}
		}
		break;

	case RunwayPt:
		if(isWing && !WingmanTakeRunway(Airbase, (AircraftClass*)flightLead, leader) && self->af->IsSet(AirframeClass::OverRunway))
		{
			OffsetTrackPoint(50.0F, rightRunway);
			CalculateTaxiSpeed(5.0F);
			break;
		}
		else if(!isWing && !IsSetATC(PermitRunway))
		{
			if(self->af->IsSet(AirframeClass::OverRunway))
			{
				OffsetTrackPoint(50.0F, rightRunway);
				CalculateTaxiSpeed(5.0F);
			}
			break;
		}
		//takeoff and get out of the way
		SetATCFlag(PermitRunway);
		if(atcstatus != tTakeRunway && atcstatus != tTakeoff)
		{
			atcstatus = tTakeRunway;
		}
		curTaxiPoint = Airbase->brain->FindRunwayPt((Flight)self->GetCampaignObject(), self->vehicleInUnit, rwIndex, &trackX, &trackY);
		desiredSpeed = 30.0F * KNOTS_TO_FTPSEC;
		waittimer = CalcWaitTime(Airbase->brain);
		break;
	}
}

void DigitalBrain::TaxiBack(ObjectiveClass *Airbase)
{
	if(atcstatus != lTaxiOff)
	{
		atcstatus = lTaxiOff;
		SendATCMsg(noATC);
	}

	switch(PtDataTable[curTaxiPoint].type)
	{
	case TakeRunwayPt:
	case TakeoffPt:
	case RunwayPt:
		if(curTaxiPoint)
		{
			curTaxiPoint = GetNextTaxiPt(curTaxiPoint);
			TranslatePointData(Airbase, curTaxiPoint, &trackX, &trackY);
			OffsetTrackPoint(120.0F, offRight);
		}
		break;
	}

	if (fabs(trackX - af->x) < TAXI_CHECK_DIST && fabs(trackY - af->y) < TAXI_CHECK_DIST )
	{
		curTaxiPoint = GetNextTaxiPt(curTaxiPoint);
		if(curTaxiPoint)
		{
			TranslatePointData(Airbase, curTaxiPoint, &trackX, &trackY);
			OffsetTrackPoint(120.0F, offRight);
		}
	}

	if(self != SimDriver.playerEntity)
	{
		if(!curTaxiPoint || PtDataTable[curTaxiPoint].flags & 2)
		{
			RegroupAircraft(self);
		}
	}

	if (CheckTaxiTrackPoint())
	{
		OffsetTrackPoint(80.0F, offRight);
	}
	CalculateTaxiSpeed(10.0F);
	SimpleGroundTrack( desiredSpeed );
}
extern bool g_bMPFix;
void DigitalBrain::SendATCMsg(AtcStatusEnum msg)
{
#ifdef DAVE_DBG

	//MonoPrint("From Digi: Aircraft: %p  Wingman: %p  Status: %d\n", self, MyWingman(), (int)atcstatus);
#endif
	//atcstatus = msg;
	//hack so we don't send atc messages to taskforces
	CampBaseClass *atc = (CampBaseClass*)vuDatabase->Find(airbase);
	if(!atc || !atc->IsObjective())
		return;

	FalconATCMessage* ATCMessage;
	if (g_bMPFix)
	ATCMessage = new FalconATCMessage( airbase, (VuTargetEntity*) vuDatabase->Find(vuLocalSessionEntity->Game()->OwnerId()) );
	else
	ATCMessage = new FalconATCMessage( airbase, FalconLocalGame );

	ATCMessage->dataBlock.from		= self->Id();
	ATCMessage->dataBlock.status	= (short)msg;

	switch(msg)
	{
	case lReqClearance:
		ATCMessage->dataBlock.type		= FalconATCMessage::RequestClearance;
		break;
	case lTakingPosition:
		ATCMessage->dataBlock.type		= FalconATCMessage::ContactApproach;
		break;		
	case lReqEmerClearance:
		ATCMessage->dataBlock.type		= FalconATCMessage::RequestEmerClearance;
		break;
	case tReqTaxi:
		ATCMessage->dataBlock.type		= FalconATCMessage::RequestTaxi;
		break;
	case tReqTakeoff:
		ATCMessage->dataBlock.type		= FalconATCMessage::RequestTakeoff;
		break;

	case tEmerStop:	
	case lAborted:
	case lIngressing:
	case lHolding:
	case lFirstLeg:
	case lToBase:
	case lToFinal:
	case lOnFinal:
	case lLanded:
	case lTaxiOff:
	case lEmerHold:
	case lEmergencyToBase:
	case lEmergencyToFinal:
	case lEmergencyOnFinal:
	case lCrashed:
	case tTaxi:
	case tHoldShort:
	case tPrepToTakeRunway:
	case tTakeRunway:
	case tTakeoff:
	case tFlyOut:
	case noATC:
	case tTaxiBack:
		ATCMessage->dataBlock.type		= FalconATCMessage::UpdateStatus;
		break;
	default:
		//we shouldn't get here
		ShiWarning("Sending unknown ATC message type");
	}

	FalconSendMessage(ATCMessage, TRUE);
}

float DigitalBrain::CalculateTaxiSpeed(float time)
{
	ObjectiveClass *Airbase = (ObjectiveClass *)vuDatabase->Find(airbase);

	ShiAssert(Airbase);

	float prevX, prevY, dx, dy;
	//float nextX, nextY;
	int point;
	point = GetNextPt(curTaxiPoint);

	if(point && time)
	{
		//TranslatePointData(Airbase, curTaxiPoint, &nextX, &nextY);
		TranslatePointData(Airbase, point, &prevX, &prevY);
		//dx = prevX - nextX;
		//dy = prevY - nextY;
		dx = prevX - trackX;
		dy = prevY - trackY;

		//how fast do we go to cover the distance in (time) seconds?
		desiredSpeed = (float)sqrt(dx*dx + dy*dy)/time;

		ShiAssert(!_isnan(desiredSpeed))
	}
	else
		desiredSpeed = 5.0F * KNOTS_TO_FTPSEC;

	//no matter how late, we don't taxi at more than 30 knots or less than 5
	desiredSpeed = max(5.0F*KNOTS_TO_FTPSEC, min(desiredSpeed,30.0F*KNOTS_TO_FTPSEC));

	return desiredSpeed;
}

void DigitalBrain::OffsetTrackPoint(float offDist, int dir)
{
	float dx=0.0F, dy=0.0F, dist=0.0F, relx=0.0F, x1=0.0F, y1=0.0F;
	float cosHdg=1.0F, sinHdg=0.0F;
	int point=0;
	float tmpX=0.0F, tmpY=0.0F;
	ObjectiveClass *Airbase = NULL;
	runwayStatsStruct *runwayStats = NULL;

	if(dir == centerRunway)
	{
		Airbase = (ObjectiveClass *)vuDatabase->Find(airbase);
		if(Airbase)
		{
			int queue = GetQueue(rwIndex);
			runwayStats = Airbase->brain->GetRunwayStats();
			float length = runwayStats[queue].halfheight;
			//TranslatePointData(Airbase, pt, &x1, &y1);
			x1 = runwayStats[queue].centerX;
			y1 = runwayStats[queue].centerY;
			
			dx = x1 - self->XPos();
			dy = y1 - self->YPos();
			
			relx = (	PtHeaderDataTable[rwIndex].cosHeading*dx + 
						PtHeaderDataTable[rwIndex].sinHeading*dy);

			relx = max(min(relx,length - TAXI_CHECK_DIST),0.0F);

			trackX = x1 - relx * PtHeaderDataTable[rwIndex].cosHeading;
			trackY = y1 - relx * PtHeaderDataTable[rwIndex].sinHeading;
		}
		return;
	}

	dx = trackX - self->XPos();
	dy = trackY - self->YPos();
	dist = (float)sqrt(dx*dx + dy*dy);
	
	//these are cos and sin of hdg to offset point along
	switch(dir)
	{
	case offForward: //forward
		cosHdg = dx/dist;
		sinHdg = dy/dist;
		break;
	case offRight: //right
		cosHdg = -dy/dist;
		sinHdg = dx/dist;
		break;
	case offBack: //back
		cosHdg = -dx/dist;
		sinHdg = -dy/dist;
		break;
	case offLeft: //left
		cosHdg = dy/dist;
		sinHdg = -dx/dist;
		break;
	case downRunway:
		cosHdg = PtHeaderDataTable[rwIndex].cosHeading;
		sinHdg = PtHeaderDataTable[rwIndex].sinHeading;
		break;
	case upRunway:
		cosHdg = -PtHeaderDataTable[rwIndex].cosHeading;
		sinHdg = -PtHeaderDataTable[rwIndex].sinHeading;
		break;
	case rightRunway:
		cosHdg = -PtHeaderDataTable[rwIndex].sinHeading;
		sinHdg = PtHeaderDataTable[rwIndex].cosHeading;
		break;
	case leftRunway:
		cosHdg = PtHeaderDataTable[rwIndex].sinHeading;
		sinHdg = -PtHeaderDataTable[rwIndex].cosHeading;
		break;
	case taxiLeft:
		Airbase = (ObjectiveClass *)vuDatabase->Find(airbase);
		if(Airbase)
		{
			point = GetPrevPtLoop(curTaxiPoint);
			TranslatePointData (Airbase, point, &tmpX, &tmpY);
			dx = tmpX - trackX;
			dy = tmpY - trackY;
			dist = (float)sqrt(dx*dx + dy*dy);
			cosHdg = dy/dist;
			sinHdg = -dx/dist;
		}
		break;
	case taxiRight:
		Airbase = (ObjectiveClass *)vuDatabase->Find(airbase);
		if(Airbase)
		{
			if(PtDataTable[curTaxiPoint].type == RunwayPt)
				point = GetNextPtLoop(curTaxiPoint);
			else
				point = GetPrevPtLoop(curTaxiPoint);
			TranslatePointData (Airbase, point, &tmpX, &tmpY);
			dx = tmpX - trackX;
			dy = tmpY - trackY;
			dist = (float)sqrt(dx*dx + dy*dy);
			cosHdg = -dy/dist;
			sinHdg = dx/dist;
		}
		break;
	}

	trackX = trackX + cosHdg*offDist;
	trackY = trackY + sinHdg*offDist;
}

int DigitalBrain::FindDesiredTaxiPoint(ulong takeoffTime)
{
	int			time_til_takeoff=0,tp, prevPt;			// in 15 second blocks
	
	if(takeoffTime > SimLibElapsedTime)
		time_til_takeoff = (takeoffTime - Camp_GetCurrentTime()) / (TAKEOFF_TIME_DELTA);
	if (time_til_takeoff < 0)
		time_til_takeoff = 0;
	tp = GetFirstPt(rwIndex);
	prevPt = tp;
	tp = GetNextPt(tp);	
	while (tp && time_til_takeoff)
	{
		prevPt = tp;
		tp = GetNextTaxiPt(tp);
		
		if(PtDataTable[tp].type == CritTaxiPt)
			break;
		
		time_til_takeoff--;
	}
	if(tp)
		return tp;
	else
		return prevPt;
}

int DigitalBrain::CalcWaitTime(ATCBrain *Atc)
{
	VU_TIME count = 0;
	VU_TIME time = rwtime;

	switch(atcstatus)
	{
	case tPrepToTakeRunway:
	case tTakeRunway:
	case tTakeoff:
		time = SimLibElapsedTime + 5 * CampaignSeconds;
		break;
			
	case tEmerStop:
		time = SimLibElapsedTime + 2 * TAKEOFF_TIME_DELTA;
		break;

	case tHoldShort:
	case tReqTaxi:
	case tReqTakeoff:
		time = SimLibElapsedTime + TAKEOFF_TIME_DELTA;
		break;
	
	case tTaxi:		
	default:
		count = GetTaxiPosition(curTaxiPoint, rwIndex);
		if(rwtime > count * TAKEOFF_TIME_DELTA/* + SimLibElapsedTime*/)
			time = rwtime - (count * TAKEOFF_TIME_DELTA /*+ SimLibElapsedTime*/);
		else if(PtDataTable[curTaxiPoint].type <= TakeoffPt)
			time = SimLibElapsedTime + 5*CampaignSeconds;
		else
			time = SimLibElapsedTime + TAKEOFF_TIME_DELTA;

		if( (isWing == 1 || isWing == 3) && Atc->UseSectionTakeoff((Flight)self->GetCampaignObject(),  rwIndex) )
			time += TAKEOFF_TIME_DELTA;
		break;
	}

	return time;
}

int DigitalBrain::ReadyToGo(void)
{
	int retval = FALSE, runway;
	FlightClass *flight = (FlightClass*)self->GetCampaignObject();
	ObjectiveClass	*Airbase = (ObjectiveClass *)vuDatabase->Find(airbase);
	AircraftClass *wingman = NULL;

	if(!Airbase)
		return TRUE;

	if(!isWing && !IsSetATC(PermitTakeoff))
		return FALSE;

	if( (!isWing || self->vehicleInUnit == 2) && 
		rwtime + WINGMAN_WAIT_TIME < SimLibElapsedTime && 
		waittimer <= SimLibElapsedTime  )
	{
		retval = TRUE;
	}
	else if(self->af->vt < 2.0F && waittimer <= SimLibElapsedTime)
	{
		if (Airbase->brain->UseSectionTakeoff(flight, rwIndex) )
		{
			wingman = (AircraftClass*)MyWingman();				

			if(wingman && Airbase->brain)
			{
				runway = Airbase->brain->IsOnRunway(wingman);
				if( !wingman->OnGround() )
					retval = TRUE;
				else if(wingman->af->vt > 40.0F * KNOTS_TO_FTPSEC && (runway == rwIndex || Airbase->brain->GetOppositeRunway(runway) == rwIndex))
					retval = TRUE;

				if(isWing == 0 || self->vehicleInUnit == 2)
				{					
					if(wingman->af->vt < 2.0F && (runway == rwIndex || Airbase->brain->GetOppositeRunway(runway) == rwIndex) )
						retval = TRUE;
				}
			}
			else 
				retval = TRUE;
		}
		else 
			retval = TRUE;
	}

	if(wingman && retval && !IsSetATC(WingmanReady))
	{
		SetATCFlag(WingmanReady);
		waittimer = CampaignSeconds + SimLibElapsedTime;

		retval = FALSE;
	}

	return retval;
}


/*
** Name: CheckTaxiTrackPoint
** Description:
**		Looks to see if another object is occupying our next tracking point
**		and returns TRUE if so.
*/
SimBaseClass* DigitalBrain::CheckTaxiTrackPoint (void)
{
	return CheckPoint (trackX, trackY);
}

SimBaseClass* DigitalBrain::CheckPoint (float x, float y)
{
	return CheckTaxiPointGlobal(self,x,y);
}



/*
** Name: CheckTaxiCollision
** Description:
**		Looks out from our current heading to see if we're going to collide
**		with anything.  If so returns TRUE.
*/
BOOL DigitalBrain::CheckTaxiCollision (void)
{
	Tpoint org, pos, vec;
	float rangeSquare;
	SimBaseClass* testObject;
	CampBaseClass* campBase;
	#ifdef VU_GRID_TREE_Y_MAJOR
	VuGridIterator gridIt(RealUnitProxList, af->y, af->x, NM_TO_FT * 3.0F);
	#else
	VuGridIterator gridIt(RealUnitProxList, af->x, af->y, NM_TO_FT * 3.0F);
	#endif

	// get the 1st objective that contains the bomb
    campBase = (CampBaseClass*) gridIt.GetFirst();

	// main loop through campaign units
	while ( campBase )
	{
		// skip campaign unit if no sim components
		if (!campBase->GetComponents())
		{
			campBase = (CampBaseClass*) gridIt.GetNext();
			continue;
		}
		// loop thru each element in the objective
		VuListIterator	unitWalker(campBase->GetComponents());
		testObject = (SimBaseClass*) unitWalker.GetFirst();

		while (testObject)
		{
			// ignore objects under these conditions:
			//		Ourself
			//		Not on ground
			//		no drawpointer
			if ( !testObject->OnGround() ||
				 testObject == self ||
				 !testObject->drawPointer )
			{
				testObject = (SimBaseClass*) unitWalker.GetNext();
				continue;
			}

			// range from tracking point to object
			pos.x = testObject->XPos() - af->x;
			pos.y = testObject->YPos() - af->y;
			rangeSquare = pos.x*pos.x + pos.y*pos.y;

			// if object is greater than given range don't check
			// also, perhaps a degenerate case, if too close and overlapping
			if ( rangeSquare > 80.0f * 80.0f || rangeSquare < 10.0f * 10.0f )
			{
				testObject = (SimBaseClass*) unitWalker.GetNext();
				continue;
			}

			// origin of ray
			rangeSquare = self->drawPointer->Radius();
			org.x = af->x + self->dmx[0][0] * rangeSquare * 1.1f;
			org.y = af->y + self->dmx[0][1] * rangeSquare * 1.1f;
			org.z = af->z + self->dmx[0][2] * rangeSquare * 1.1f;

			// vector of ray
			vec.x = self->dmx[0][0] * 80.0f;
			vec.y = self->dmx[0][1] * 80.0f;
			vec.z = self->dmx[0][2] * 80.0f;

			// do ray, box intersection test
		    if ( testObject->drawPointer->GetRayHit( &org, &vec, &pos, 1.0f ) )
			{
				return TRUE;
			}

			testObject = (SimBaseClass*) unitWalker.GetNext();
		}

		// get the next objective that contains the bomb
		campBase = (CampBaseClass*) gridIt.GetNext();

	} // end objective loop

	return FALSE;
}


/*
** Name: SimpleGroundTrack
** Description:
**		Wraps Simple Track Speed.  Then it checks objects around it and
**		uses a potential field to modify the steering to (hopefully)
**		avoid other objects.
*/
BOOL DigitalBrain::SimpleGroundTrack (float speed)
{
	float tmpX, tmpY;
	float rx, ry;
	float az, azErr;
	float stickError;
	SimBaseClass* testObject;
	int numOnLeft, numOnRight;
	float myRad, testRad, xft, yft, dist;
	float minAz = 1000.0f;	
	
	af->ClearFlag(AirframeClass::WheelBrakes);
	xft = trackX - af->x;
	yft = trackY - af->y;
	// get relative position and az
	rx	= self->dmx[0][0] * xft + self->dmx[0][1] * yft;
	ry	= self->dmx[1][0] * xft + self->dmx[1][1] * yft;

	if(self == SimDriver.playerEntity && IsSetATC(StopPlane))
	{
		// call simple track to set the stick
		if(rx < 10.0F)
		{
			int taxiPoint = GetPrevPtLoop(curTaxiPoint);
			ObjectiveClass	*Airbase = (ObjectiveClass *)vuDatabase->Find(airbase);

			TranslatePointData(Airbase, taxiPoint, &tmpX, &tmpY);
			xft = tmpX - af->x;
			yft = tmpY - af->y;
			// get relative position and az
			rx	= self->dmx[0][0] * xft + self->dmx[0][1] * yft;
			ry	= self->dmx[1][0] * xft + self->dmx[1][1] * yft;
			if(gameCompressionRatio)
				// JB 020315 Why divide by gameCompressionRation?  It screws up taxing for one thing.
				rStick = SimpleTrackAzimuth(rx , ry, self->Vt());///gameCompressionRatio;
			else
				rStick = 0.0f;
			pStick = 0.0F;	
			throtl = 0.0F;
			af->SetFlag(AirframeClass::WheelBrakes);
		}
		else
			TrackPoint(0.0f, 0.0F);

		if(self->af->vt < 1.0F)
		{
			ClearATCFlag(StopPlane);
			af->ClearFlag(AirframeClass::WheelBrakes);
			self->SetAutopilot(AircraftClass::APOff);
		}
		return FALSE;
	}

	if ( atcstatus == tTakeoff )
	{
		// once we're taking off just do it....
		//aim for a five degree climb	
	    af->LEFTakeoff();
	    af->TEFTakeoff();
		rStick = SimpleTrackAzimuth(rx + 1000.0F, ry, self->Vt());

		pStick = 5 * DTR;
		throtl = 1.5f;
	}
	else
	{
		// cheat a bit so we don't chase around in a circle
		// if we're getting close to our track point, slow and
		// rotate towards it
		
		dist = xft * xft + yft * yft;
		if ( dist < TAXI_CHECK_DIST * TAXI_CHECK_DIST * 4.0F)
		{
			az	= (float) atan2 (ry,rx);
			if ( fabs( az ) > 30.0f * DTR )
				speed *= 0.5f;
		}

		// call simple track to set the stick
		TrackPoint(0.0f, speed);
	}	

	if ( speed == 0.0f )
	{
		// if no speed we're done
		if(rx < 10.0F)
		{
			int taxiPoint = GetPrevPtLoop(curTaxiPoint);
			ObjectiveClass	*Airbase = (ObjectiveClass *)vuDatabase->Find(airbase);

			TranslatePointData(Airbase, taxiPoint, &tmpX, &tmpY);
			xft = tmpX - af->x;
			yft = tmpY - af->y;
			// get relative position and az
			rx	= self->dmx[0][0] * xft + self->dmx[0][1] * yft;
			ry	= self->dmx[1][0] * xft + self->dmx[1][1] * yft;
			if(gameCompressionRatio)
				// JB 020315 Why divide by gameCompressionRation?  It screws up taxing for one thing.
				rStick = SimpleTrackAzimuth(rx , ry, self->Vt());///gameCompressionRatio;
			else
				rStick = 0.0f;
			pStick = 0.0F;			
		}
		return FALSE;
	}

	if(!self->OnGround())
		return FALSE;

	// init the stick error
	stickError = 0.0f;
	numOnLeft = 0;
	numOnRight = 0;

	if ( self->drawPointer )
		myRad = self->drawPointer->Radius();
	else
		myRad = 40.0f;

	// loop thru all sim objects
	VuListIterator	unitWalker(SimDriver.objectList);
	testObject = (SimBaseClass*) unitWalker.GetFirst();

	while (testObject)
	{
		// ignore objects under these conditions:
		//		Ourself
		//		Not on ground
		if ( !testObject->OnGround() ||
			 testObject == self )
		{
			testObject = (SimBaseClass*) unitWalker.GetNext();
			continue;
		}

		// range from us to object
		tmpX = testObject->XPos() - af->x;
		tmpY = testObject->YPos() - af->y;

		if ( testObject->drawPointer )
			testRad = testObject->drawPointer->Radius() + myRad;
		else
			testRad = 40.0f + myRad;

		dist = (float)sqrt(tmpX*tmpX + tmpY*tmpY);
		float range = dist - testRad - MAX_RANGE_COLL;

		//rangeSquare = tmpX*tmpX + tmpY*tmpY - testRad * testRad - MAX_RANGE_SQ;
		
		// if object is greater than 2 x max range continue to next
		//if ( rangeSquare > MAX_RANGE_SQ )
		if(range > MAX_RANGE_COLL)
		{
			testObject = (SimBaseClass*) unitWalker.GetNext();
			continue;
		}

		// get relative position and az
		rx	= self->dmx[0][0] * tmpX + self->dmx[0][1] * tmpY;
		ry	= self->dmx[1][0] * tmpX + self->dmx[1][1] * tmpY;

		az	= (float) atan2 (ry,rx);		

		// reject anything more than 90 deg off our nose
		if ( fabs( az ) > MAX_AZ )
		{
			testObject = (SimBaseClass*) unitWalker.GetNext();
			continue;
		}

		if(rx > 0.0F && fabs(ry) > testRad && range < 0.0F && 
			testObject->GetCampaignObject() == self->GetCampaignObject() && 
			self->vehicleInUnit > ((AircraftClass*)testObject)->vehicleInUnit)
		{
			xft = trackX - af->x;
			yft = trackY - af->y;
			// get relative position and az
			rx	= self->dmx[0][0] * xft + self->dmx[0][1] * yft;
			ry	= self->dmx[1][0] * xft + self->dmx[1][1] * yft;
			if(rx < 10.0F)
			{
				int taxiPoint = GetPrevPtLoop(curTaxiPoint);
				ObjectiveClass	*Airbase = (ObjectiveClass *)vuDatabase->Find(airbase);

				TranslatePointData(Airbase, taxiPoint, &tmpX, &tmpY);
				xft = tmpX - af->x;
				yft = tmpY - af->y;
				// get relative position and az
				rx	= self->dmx[0][0] * xft + self->dmx[0][1] * yft;
				ry	= self->dmx[1][0] * xft + self->dmx[1][1] * yft;
				if(gameCompressionRatio)
					// JB 020315 Why divide by gameCompressionRation?  It screws up taxing for one thing.
					rStick = SimpleTrackAzimuth(rx , ry, self->Vt());///gameCompressionRatio;
				else
					rStick = 0.0f;
				pStick = 0.0F;		
				throtl = SimpleGroundTrackSpeed(0.0F);	
			}
			else
				TrackPoint(0.0f, speed);
			return TRUE;
		}

		if ( fabs(az) < fabs(minAz) )
		{
			minAz = az;
		}
		// have we reached a situation where it's impossible to
		// move forward without colliding?
		if ( fabs(az) < 80.0F*DTR && rx > 5.0f && rx < testRad*1.5F && fabs( ry ) < testRad*1.25F )
		{
			// count the number of blocks to our left and right
			// to be used later
			if ( ry > 0.0f )
				numOnRight++;
			else
				numOnLeft++;

			testObject = (SimBaseClass*) unitWalker.GetNext();
			continue;
		}

		// OK, we've got an object in front of our 3-6 line and
		// within range.  The potential field will work by deflecting
		// our nose (rstick).  The closer the object and the more towards
		// our nose, the stronger is the deflection

		
		if ( az > 0.0f )
			azErr = -1.0F + ry/dist;
		else if ( az < 0.0f )
			azErr = 1.0F + ry/dist;
		else if ( rStick > 0.0f )
			azErr = 1.0f;
		else
			azErr = -1.0f;

		// this deflection is now modulated by the proximity -- the closer
		// the stronger the force
		// weight the range higher
		azErr *= ( MAX_RANGE_COLL  - range ) / (MAX_RANGE_COLL * 0.4F);

		// now accumulate the stick error
		stickError += azErr;


		testObject = (SimBaseClass*) unitWalker.GetNext();
	}

	// test blockages directly in front
	// cheat: if we get stuck just rotate in place
	if( (numOnLeft || numOnRight) && fabs(minAz) > 70.0f * DTR)
	{
		rStick = 0.0f;
		pStick = 0.0f;
		throtl *= 0.5f;
		tiebreaker = 0;
	}
	else if (tiebreaker > 10) {
		af->sigma -= 10.0f * DTR * SimLibMajorFrameTime;
		af->SetFlag(AirframeClass::WheelBrakes);
		throtl = 0.0f;
		if (tiebreaker ++ - 10 > rand() % 20)
			tiebreaker = 0;
		return TRUE;
	}
	else if ( numOnLeft && numOnRight )
	{
		if(fabs(minAz) > 50.0f * DTR)
		{
			rStick = 0.0f;
			pStick = 0.0f;
			throtl *= 0.5f;
		}
		else
		{
			af->sigma -= 10.0f * DTR * SimLibMajorFrameTime;
			af->SetFlag(AirframeClass::WheelBrakes);
			throtl = 0.0f;
		}
		tiebreaker ++;
		return TRUE;
	}
	else if ( numOnRight )
	{
		af->sigma -= 10.0f * DTR * SimLibMajorFrameTime;
		af->SetFlag(AirframeClass::WheelBrakes);
		throtl = 0.0f;
		tiebreaker ++;
		return TRUE;
	}
	else if ( numOnLeft )
	{
		af->sigma += 10.0f * DTR * SimLibMajorFrameTime;
		af->SetFlag(AirframeClass::WheelBrakes);
		throtl = 0.0f;
		tiebreaker ++;
		return TRUE;
	}
	tiebreaker = 0;
	// we now apply the stick error to rstick
	// make sure we clamp rstick
	rStick += stickError;
	if ( rStick > 1.0f )
		rStick = 1.0f;
	else if ( rStick < -1.0f )
		rStick = -1.0f;

	// readjust throttle if our stick error is large
	if ( fabs( stickError ) > 0.5f )
	{
		throtl = SimpleGroundTrackSpeed(speed * 0.75f);	
	}

	return FALSE;
}

void DigitalBrain::UpdateTaxipoint(void)
{
	if(!self->OnGround())
		return;

	ObjectiveClass *Airbase = (ObjectiveClass *)vuDatabase->Find(airbase);
	float tmpX, tmpY, dx, dy, dist, closestDist =  4000000.0F;
	int taxiPoint, i, closest = curTaxiPoint;

	SetDebugLabel(Airbase);

	taxiPoint = GetPrevPtLoop(curTaxiPoint);
	for(i = 0; i < 3; i++)
	{
		TranslatePointData(Airbase, taxiPoint, &tmpX, &tmpY);
		dx = tmpX - af->x;
		dy = tmpY - af->y;
		dist = dx*dx + dy*dy;
		if ( dist < closestDist )
		{
			closestDist = dist;
			closest = taxiPoint;
		}
		taxiPoint = GetNextPtLoop(taxiPoint);
	}

	if(closest != curTaxiPoint)
	{
		if(closest == GetNextPtLoop(curTaxiPoint) && self->AutopilotType() == AircraftClass::APOff)
		{
			if( IsSetATC(CheckTaxiBack) && atcstatus != tTaxiBack)
			{
				atcstatus = tTaxiBack;
				SendATCMsg(atcstatus);
			}
			else
				SetATCFlag(CheckTaxiBack);
		}
		else if(closest == GetPrevPtLoop(curTaxiPoint) && atcstatus == tTaxiBack)
		{
			if(IsSetATC(PermitRunway))
				atcstatus = tTakeRunway;
			else
				atcstatus = tTaxi;
			SendATCMsg(atcstatus);
			ClearATCFlag(CheckTaxiBack);
		}
		curTaxiPoint = closest;
		TranslatePointData(Airbase, curTaxiPoint, &trackX, &trackY);
		CalculateTaxiSpeed(5.0F);
		waittimer = CalcWaitTime(Airbase->brain);
	}
}


AircraftClass* DigitalBrain::GetLeader(void)
{
	VuListIterator	cit(self->GetCampaignObject()->GetComponents());
	VuEntity		*cur;
	VuEntity		*prev = NULL;

	cur = cit.GetFirst();
	while (cur)
	{
		if(cur == self)
		{
			if(prev)
				return ((AircraftClass*)prev);
			else 
				return NULL;
		}
		prev = cur;
		
		cur = cit.GetNext();
	}
	return NULL;
}

// JPO - whole bunch of preflight stuff here.
typedef int (*PreFlightFnx)(AircraftClass *self);

static int EngineStart(AircraftClass *self)
{
    if (self->af->IsSet(AirframeClass::EngineStopped)) {
	if (self->af->rpm > 0.2f) { // Jfs is runnning time to light
	    self->af->SetThrotl(0.1f); // advance the throttle to light.
	    self->af->ClearFlag(AirframeClass::EngineStopped);
	}
	else if (self->af->IsSet(AirframeClass::JfsStart)) { // nothing much happening
	}
	else { // start the JFS
	    self->af->JfsEngineStart();
	}
	return 0;
    }
    else {
	if (self->af->rpm > 0.69f) { // at idle
	    self->af->SetThrotl(0.0f);
	    return 1; // finished engine start up
	}
	else if (self->af->rpm > 0.5f) { // engine now running
	    self->af->SetThrotl(0.0f);
	}
    }
    return 0;
}
static struct PreflightActions {
    enum { CANOPY, FUEL1, FUEL2, FNX, RALTON, POWERON, AFFLAGS, RADAR, SEAT, MAINPOWER, EWSPGM, 
		MASTERARM, EXTLON, INS, VOLUME, FLAPS };
    int action; // what to do, 
    int data;	// any data
    int timedelay; // how many seconds til next one
    PreFlightFnx fnx;
} PreFlightTable[] = {
    { PreflightActions::CANOPY, 0, 1, NULL},
    { PreflightActions::FUEL1, 0, 1, NULL},
    { PreflightActions::FUEL2, 0, 1, NULL},
    { PreflightActions::MAINPOWER, 0, 1, NULL},
    { PreflightActions::FNX, 0, 0, EngineStart},
    { PreflightActions::RALTON, 0, 1, NULL },
    { PreflightActions::POWERON, AircraftClass::HUDPower, 6, NULL},
    { PreflightActions::POWERON, AircraftClass::MFDPower, 2, NULL},
    { PreflightActions::POWERON, AircraftClass::FCCPower, 1, NULL},
    { PreflightActions::POWERON, AircraftClass::SMSPower, 1, NULL},
    { PreflightActions::POWERON, AircraftClass::UFCPower, 1, NULL},
    { PreflightActions::POWERON, AircraftClass::FCRPower, 3, NULL},
    { PreflightActions::POWERON, AircraftClass::TISLPower, 1, NULL},
    { PreflightActions::POWERON, AircraftClass::LeftHptPower, 1, NULL},
    { PreflightActions::POWERON, AircraftClass::RightHptPower, 1, NULL},
    { PreflightActions::POWERON, AircraftClass::GPSPower, 1, NULL},
    { PreflightActions::POWERON, AircraftClass::DLPower, 1, NULL},
    { PreflightActions::POWERON, AircraftClass::MAPPower, 1, NULL},
    { PreflightActions::POWERON, AircraftClass::RwrPower, 1, NULL},
    { PreflightActions::RADAR, 0, 1, NULL},
	{ PreflightActions::POWERON, AircraftClass::EWSRWRPower, 2, NULL},
	{ PreflightActions::POWERON, AircraftClass::EWSJammerPower, 2, NULL},
	{ PreflightActions::POWERON, AircraftClass::EWSChaffPower, 2, NULL},
	{ PreflightActions::POWERON, AircraftClass::EWSFlarePower, 2, NULL},
	{ PreflightActions::POWERON, AircraftClass::IFFPower, 2, NULL},
	{ PreflightActions::EWSPGM, 0,1,NULL},
	{ PreflightActions::EXTLON, AircraftClass::Extl_Main_Power, 1, NULL},	//exterior lighting
	{ PreflightActions::EXTLON, AircraftClass::Extl_Anit_Coll, 1, NULL},	//exterior lighting
	{ PreflightActions::EXTLON, AircraftClass::Extl_Wing_Tail, 1, NULL},	//exterior lighting
	{ PreflightActions::INS, AircraftClass::INS_AlignNorm, 485, NULL},	//time to align
	{ PreflightActions::INS, AircraftClass::INS_Nav,0, NULL},
	{ PreflightActions::VOLUME, 1,1, NULL},
	{ PreflightActions::VOLUME, 2,1, NULL},
	{ PreflightActions::AFFLAGS, AirframeClass::NoseSteerOn, 3, NULL }, //shortly before taxi
	{ PreflightActions::MASTERARM, 0, 1, NULL},	//MasterArm
	{ PreflightActions::SEAT, 0, 1, NULL},	//this comes all at the end
    { PreflightActions::FLAPS, 0, 1, NULL},
};
static const int MAX_PF_ACTIONS = sizeof(PreFlightTable) / sizeof(PreFlightTable[0]);

// JPO - go through start up steps.
int DigitalBrain::PreFlight ()
{
    ShiAssert(af != NULL);
    if (SimLibElapsedTime < mNextPreflightAction) return 0;
    
    switch (PreFlightTable[mActionIndex].action) {
    case PreflightActions::FNX:
	if ((*PreFlightTable[mActionIndex].fnx)(self) == 0)
	    return 0;
	mNextPreflightAction = SimLibElapsedTime + PreFlightTable[mActionIndex].timedelay * CampaignSeconds;
	break;
	
    case PreflightActions::CANOPY:
		if (af->canopyState) {
			af->CanopyToggle();
			mNextPreflightAction = SimLibElapsedTime + PreFlightTable[mActionIndex].timedelay * CampaignSeconds;
		}
		break;
    case PreflightActions::FUEL1:
	if (af->IsEngineFlag(AirframeClass::MasterFuelOff)) {
	    af->ClearEngineFlag(AirframeClass::MasterFuelOff);
	    mNextPreflightAction = SimLibElapsedTime + PreFlightTable[mActionIndex].timedelay * CampaignSeconds;
	}
	break;
    case PreflightActions::MAINPOWER:
	if (self->MainPower() != AircraftClass::MainPowerMain) {
	    self->IncMainPower();
	    mNextPreflightAction = SimLibElapsedTime + PreFlightTable[mActionIndex].timedelay * CampaignSeconds;
	    return 0; //keep clicking til we get to the right state
	}
	break;
    case PreflightActions::FUEL2:
	af->SetFuelPump(AirframeClass::FP_NORM);
	af->SetFuelSwitch(AirframeClass::FS_NORM);
	af->ClearEngineFlag(AirframeClass::WingFirst);
	af->SetAirSource(AirframeClass::AS_NORM);
	mNextPreflightAction = SimLibElapsedTime + PreFlightTable[mActionIndex].timedelay * CampaignSeconds;
	break;
    case PreflightActions::RALTON:
	if (self->RALTStatus == AircraftClass::RaltStatus::ROFF) {
	    self->RaltOn();
	    mNextPreflightAction = SimLibElapsedTime + PreFlightTable[mActionIndex].timedelay * CampaignSeconds;
	}
	break;
    case PreflightActions::AFFLAGS:
	if (!af->IsSet(PreFlightTable[mActionIndex].data)) {
	    af->SetFlag(PreFlightTable[mActionIndex].data);
	    mNextPreflightAction = SimLibElapsedTime + PreFlightTable[mActionIndex].timedelay * CampaignSeconds;
	}
	break;
    case PreflightActions::POWERON:
	if (!self->PowerSwitchOn((AircraftClass::AvionicsPowerFlags)PreFlightTable[mActionIndex].data)) {
	    self->PowerOn((AircraftClass::AvionicsPowerFlags)PreFlightTable[mActionIndex].data);
		//MI additional check for HUD, now that the dial indicates the status
		if(PreFlightTable[mActionIndex].data == AircraftClass::HUDPower)
		{
			if(TheHud && self == SimDriver.playerEntity)
				TheHud->SymWheelPos = 1.0F;
		}
	    mNextPreflightAction = SimLibElapsedTime + PreFlightTable[mActionIndex].timedelay * CampaignSeconds;
	}
	break;
    case PreflightActions::RADAR:
	{
	    RadarClass* theRadar = (RadarClass*) FindSensor (self, SensorClass::Radar);
	    if (theRadar)
		theRadar->SetMode (RadarClass::AA);
	    mNextPreflightAction = SimLibElapsedTime + PreFlightTable[mActionIndex].timedelay * CampaignSeconds;
	}
	break;
	case PreflightActions::SEAT:
	{
		self->SeatArmed = TRUE;
		mNextPreflightAction = SimLibElapsedTime + PreFlightTable[mActionIndex].timedelay * CampaignSeconds;
	}
	break;
	case PreflightActions::EWSPGM:
	{
		self->SetPGM(AircraftClass::EWSPGMSwitch::Man);
	    mNextPreflightAction = SimLibElapsedTime + PreFlightTable[mActionIndex].timedelay * CampaignSeconds;
	}
	break;
	case PreflightActions::MASTERARM:
	{
		self->Sms->SetMasterArm(SMSBaseClass::Arm);
		mNextPreflightAction = SimLibElapsedTime + PreFlightTable[mActionIndex].timedelay * CampaignSeconds;
	}
	break;
	case PreflightActions::EXTLON:
	if (!self->ExtlState((AircraftClass::ExtlLightFlags)PreFlightTable[mActionIndex].data)) {
	    self->ExtlOn((AircraftClass::ExtlLightFlags)PreFlightTable[mActionIndex].data);
	    mNextPreflightAction = SimLibElapsedTime + PreFlightTable[mActionIndex].timedelay * CampaignSeconds;
	}
	break;
	case PreflightActions::INS:
		//turn on aligning flag
		if(PreFlightTable[mActionIndex].data == AircraftClass::INS_AlignNorm) 
		{
			self->INSOff(AircraftClass::INS_PowerOff);
			if(self == SimDriver.playerEntity && OTWDriver.pCockpitManager)
			{
				self->SwitchINSToAlign();
				self->INSAlignmentTimer = 0.0F;
				self->HasAligned = FALSE;
				//Set the UFC
				OTWDriver.pCockpitManager->mpIcp->ClearStrings();
				OTWDriver.pCockpitManager->mpIcp->LeaveCNI();
				OTWDriver.pCockpitManager->mpIcp->SetICPFlag(ICPClass::MODE_LIST);
				OTWDriver.pCockpitManager->mpIcp->SetICPSecondaryMode(23);	//SIX Button, INS Page
			}
		}
		self->INSOn((AircraftClass::INSAlignFlags)PreFlightTable[mActionIndex].data);
		mNextPreflightAction = SimLibElapsedTime + PreFlightTable[mActionIndex].timedelay * CampaignSeconds;
		
		if(self->INSState(AircraftClass::INS_Aligned) && self->INSState(AircraftClass::INS_AlignNorm))
			self->INSOff(AircraftClass::INS_AlignNorm);

		if(self == SimDriver.playerEntity && OTWDriver.pCockpitManager &&
			PreFlightTable[mActionIndex].data == AircraftClass::INS_Nav)
		{
			//CNI page
			OTWDriver.pCockpitManager->mpIcp->ChangeToCNI();
			//Mark us as aligned
			self->SwitchINSToNav();
		}
	break;
	case PreflightActions::VOLUME:
		if(PreFlightTable[mActionIndex].data == 1)
			self->MissileVolume = 0;
		else if(PreFlightTable[mActionIndex].data == 2)
			self->ThreatVolume = 0;
	break;
	case PreflightActions::FLAPS:
	    af->TEFTakeoff();
	    af->LEFTakeoff();
	    break;
    default:
	ShiWarning("Bad Preflight Table");
	break;
    }
    // force switch positions.
    if (self == SimDriver.playerEntity && OTWDriver.pCockpitManager)
       OTWDriver.pCockpitManager->InitialiseInstruments();

    mActionIndex ++;
    if (mActionIndex < MAX_PF_ACTIONS)
	return 0;
    mActionIndex = 0;
    mNextPreflightAction = 0;
    return 1;
}

void DigitalBrain::QuickPreFlight()
{
    ShiAssert(af != NULL && self != NULL);
    self->PreFlight();
}

// JPO - work out if we are further along the time line.
int DigitalBrain::IsAtFirstTaxipoint()
{
    ShiAssert(self != NULL);
    Flight flight = (Flight)self->GetCampaignObject();
    ShiAssert(flight != NULL);
    WayPoint	w = flight->GetFirstUnitWP(); // this is takeoff time
    ObjectiveClass *Airbase = (ObjectiveClass *)vuDatabase->Find(airbase);
    ShiAssert(Airbase);
    // so takeoff time, - deag time (taxi point time) - if we're past that - we're ready.
    if (SimLibElapsedTime > w->GetWPDepartureTime() - Airbase->brain->MinDeagTime()) {
	return 0;
    }
    else
	return 1;

}

