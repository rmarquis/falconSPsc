#include "stdhdr.h"
#include "tankbrn.h"
#include "simveh.h"
#include "unit.h"
#include "simdrive.h"
#include "object.h"
#include "falcmesg.h"
#include "MsgInc\TankerMsg.h"
#include "falcsess.h"
#include "Aircrft.h"
#include "Graphics\Include\drawbsp.h"
#include "classtbl.h"
#include "Graphics\Include\matrix.h"
#include "airframe.h"
#include "playerop.h"
#include "MsgInc\SimCampMsg.h"//me123
#include "Find.h" 
#include "Flight.h"

#ifdef DAVE_DBG
float boomAzTest = 0.0F, boomElTest = -4.0F *DTR, boomExtTest = 0.0F;
extern int MoveBoom;
#endif

extern bool g_bLightsKC135;
extern int g_nShowDebugLabels;

#define FOLLOW_RATE (10.0F * DTR)
static const int IL78BOOM = 1; // the basket to refuel from
static const float IL78EXT = 40.0f; // how far the basket goes

enum{
	MOVE_FORWARD,
	MOVE_BACK,
	MOVE_UP,
	MOVE_DOWN,
	MOVE_RIGHT,
	MOVE_LEFT
};

extern short gRackId_Single_Rack;
extern float SimLibLastMajorFrameTime;
extern bool g_bUseTankerTrack;
extern float g_fTankerTrackFactor;
extern float g_fTankerHeadsupDistance;
extern float g_fTankerBackupDistance;
extern float g_fHeadingStabilizeFactor;

TankerBrain::TankerBrain (AircraftClass *myPlatform, AirframeClass* myAf) : DigitalBrain (myPlatform, myAf)
{
	flags = 0;
	thirstyQ = new TailInsertList;
	thirstyQ->Init();
	waitQ = new HeadInsertList;
	waitQ->Init();
	curThirsty = NULL;
	tankingPtr = NULL;
	lastStabalize = 0;
	lastBoomCommand = 0;
	memset(&boom,0,sizeof(boom));
	memset(&rack,0,sizeof(rack));
	numBooms = 0;
	turnallow = false;
	HeadsUp = false;
	reachedFirstTrackpoint = false;
	// 2002-03-13 MN
	for (int i = 0; i < sizeof (TrackPoints) / sizeof (vector); i++)
	{
		TrackPoints[i].x = 0.0f; 
		TrackPoints[i].y = 0.0f;
		TrackPoints[i].z = 0.0f;
	}
}

TankerBrain::~TankerBrain (void)
{
	if (tankingPtr)
	{
		tankingPtr->Release( SIM_OBJ_REF_ARGS );
	}
	
	tankingPtr = NULL;
	curThirsty = NULL;
	
	thirstyQ->DeInit();
	delete thirstyQ;
	thirstyQ = NULL;
	waitQ->DeInit(); // another leak fixed JPO
	delete waitQ;
	waitQ = NULL;
	CleanupBoom();
}

void TankerBrain::CleanupBoom(void)
{
	int i;

	switch(type)
	{
	case TNKR_KC10:
	case TNKR_KC135:
		if(boom[0].drawPointer)
			((DrawableBSP*)self->drawPointer)->DetachChild(boom[0].drawPointer, 0);
		delete boom[0].drawPointer;
		boom[0].drawPointer = NULL;	
		break;

	case TNKR_IL78:
		for(i = 0; i < numBooms; i++)
		{
			if(boom[i].drawPointer)
				rack[i]->DetachChild(boom[i].drawPointer, 0);
			delete boom[i].drawPointer;
			boom[i].drawPointer = NULL;	

			if(rack[i])
				((DrawableBSP*)self->drawPointer)->DetachChild(rack[i], i);
			delete rack[i];
			rack[i] = NULL;
		}
		
		break;
	}
	
}

void TankerBrain::InitBoom(void)
{
	Tpoint rackLoc = {0.0F};
	Tpoint simLoc = {0.0F};
	switch(((DrawableBSP*)self->drawPointer)->GetID())
	{
	case VIS_KC10:
		numBooms = 1;
		type = TNKR_KC10;
		boom[0].drawPointer = new DrawableBSP( MapVisId(VIS_KC10BOOM), &simLoc, &IMatrix );
		((DrawableBSP*)self->drawPointer)->AttachChild(boom[0].drawPointer, 0);
		((DrawableBSP*)self->drawPointer)->GetChildOffset(0, &simLoc);
		boom[0].rx = simLoc.x;
		boom[0].ry = simLoc.y;
		boom[0].rz = simLoc.z;
		boom[0].az = 0.0F;
		boom[0].el = 0.0F;
		boom[0].ext = 0.0F;
		break;

	case VIS_KC135:
		numBooms = 1;
		type = TNKR_KC135;
		boom[0].drawPointer = new DrawableBSP( MapVisId(VIS_KC135BOOM), &simLoc, &IMatrix );
		((DrawableBSP*)self->drawPointer)->AttachChild(boom[0].drawPointer, 0);
		((DrawableBSP*)self->drawPointer)->GetChildOffset(0, &simLoc);
		boom[0].rx = simLoc.x;
		boom[0].ry = simLoc.y;
		boom[0].rz = simLoc.z;
		boom[0].az = 0.0F;
		boom[0].el = 0.0F;
		boom[0].ext = 0.0F;
		break;

	case VIS_IL78:
		numBooms = 3;
		type = TNKR_IL78;

		rack[0] = new DrawableBSP( MapVisId(VIS_SINGLE_RACK), &simLoc, &IMatrix );
		((DrawableBSP*)self->drawPointer)->AttachChild(rack[0], 0);
		((DrawableBSP*)self->drawPointer)->GetChildOffset(0, &rackLoc);

		boom[0].drawPointer = new DrawableBSP( MapVisId(VIS_RDROGUE), &simLoc, &IMatrix );
		rack[0]->AttachChild(boom[0].drawPointer, 0);
		rack[0]->GetChildOffset(0, &simLoc);

		boom[0].rx = simLoc.x + rackLoc.x;
		boom[0].ry = simLoc.y + rackLoc.y;
		boom[0].rz = simLoc.z + rackLoc.z;
		boom[0].az = 0.0F;
		boom[0].el = 0.0F;
		boom[0].ext = 0.0F;

		rack[1] = new DrawableBSP( MapVisId(VIS_SINGLE_RACK), &simLoc, &IMatrix );
		((DrawableBSP*)self->drawPointer)->AttachChild(rack[1], 1);
		((DrawableBSP*)self->drawPointer)->GetChildOffset(1, &rackLoc);

		boom[1].drawPointer = new DrawableBSP( MapVisId(VIS_RDROGUE), &simLoc, &IMatrix );
		rack[1]->AttachChild(boom[1].drawPointer, 0);
		rack[1]->GetChildOffset(0, &simLoc);

		boom[1].rx = simLoc.x + rackLoc.x;
		boom[1].ry = simLoc.y + rackLoc.y;
		boom[1].rz = simLoc.z + rackLoc.z;
		boom[1].az = 0.0F;
		boom[1].el = 0.0F;
		boom[1].ext = 0.0F;

		rack[2] = new DrawableBSP( MapVisId(VIS_SINGLE_RACK), &simLoc, &IMatrix );
		((DrawableBSP*)self->drawPointer)->AttachChild(rack[2], 2);
		((DrawableBSP*)self->drawPointer)->GetChildOffset(2, &rackLoc);

		boom[2].drawPointer = new DrawableBSP( MapVisId(VIS_RDROGUE), &simLoc, &IMatrix );
		rack[2]->AttachChild(boom[2].drawPointer, 0);
		rack[2]->GetChildOffset(0, &simLoc);

		boom[2].rx = simLoc.x + rackLoc.x;
		boom[2].ry = simLoc.y + rackLoc.y;
		boom[2].rz = simLoc.z + rackLoc.z;
		boom[2].az = 0.0F;
		boom[2].el = 0.0F;
		boom[2].ext = 0.0F;
		break;
	default:
		type = TNKR_UNKNOWN;
		break;
	}
	
	
}


void TankerBrain::CallNext (void)
{
	VuListIterator thirstyWalker (thirstyQ);
	AircraftClass* aircraft;
	FalconTankerMessage* tankMsg;
	int count = 1;
	
	aircraft = (AircraftClass*)thirstyWalker.GetFirst();
	if (aircraft)
	{
		//me123 yea this is crap holdAlt = min ( max (-self->ZPos(), 15000.0F), 25000.0F);
//		  holdAlt = -self->ZPos();//me123
// 2002-02-17 holding current ZPos is crap - A10 can never reach tanker cruising altitude - use defined altitude from airframe class auxaerodata
		holdAlt = aircraft->af->GetRefuelAltitude();
		//it's much easier if the tanker isn't trying to change altitude
		//holdAlt = 20000.0F;
		curThirsty = aircraft;
		flags |= IsRefueling;
		
		// Say precontact message
		tankMsg = new FalconTankerMessage (self->Id(), FalconLocalGame);
		tankMsg->dataBlock.caller = curThirsty->Id();
		tankMsg->dataBlock.type = FalconTankerMessage::PreContact;
		tankMsg->dataBlock.data1 = tankMsg->dataBlock.data1 = 0;
		FalconSendMessage (tankMsg);
		
		aircraft = (AircraftClass*)thirstyWalker.GetNext();
		while(aircraft)
		{
			tankMsg = new FalconTankerMessage (self->Id(), FalconLocalGame);
			tankMsg->dataBlock.caller = aircraft->Id();
			tankMsg->dataBlock.type = FalconTankerMessage::PositionUpdate;
			tankMsg->dataBlock.data1 = tankMsg->dataBlock.data1 = (float)count;
			FalconSendMessage (tankMsg);

			count++;
			aircraft = (AircraftClass*)thirstyWalker.GetNext();
		}	
		
//me123 
		if (vuLocalSessionEntity &&
		 vuLocalSessionEntity->Game() &&
		 self->OwnerId() != curThirsty->OwnerId())
	 	{// we are hostign a game
	 	VuGameEntity* game;	game  = vuLocalSessionEntity->Game();
	 	VuSessionsIterator Sessioniter(game);
	 	VuSessionEntity*   sess;
	 	sess = Sessioniter.GetFirst();
	 	int foundone = FALSE;

	 	while (sess && !foundone)
	 		{
				if (sess->Camera(0) == curThirsty) 
				{
					 foundone = TRUE;
					 break;
				}
				else  
				{
					 sess = Sessioniter.GetNext ();
				}
	 		}
	 	if (foundone) 
	 		 {
	 		FalconSimCampMessage	*msg = new FalconSimCampMessage (self->GetCampaignObject()->Id(), FalconLocalGame); // target);
	 		msg->dataBlock.from = sess->Id();
	 		msg->dataBlock.message = FalconSimCampMessage::simcampChangeOwner;
	 		FalconSendMessage(msg);
	 		 }
		else
			{// must be an ai plane who's curthirsty so lets give the host the tanker
	 		FalconSimCampMessage	*msg = new FalconSimCampMessage (self->GetCampaignObject()->Id(), FalconLocalGame); // target);
	 		msg->dataBlock.from = curThirsty->OwnerId();
	 		msg->dataBlock.message = FalconSimCampMessage::simcampChangeOwner;
	 		FalconSendMessage(msg);
			}

	 	}

	}

}

void TankerBrain::DoneRefueling (void)
{
	/*
	if (tankingPtr)
	{
		tankingPtr->Release( SIM_OBJ_REF_ARGS );
		//if (self->IsInstantAction())
		//InstantAction.SetNoAdd(FALSE);
	}
	
	tankingPtr = NULL;
	*/
	holdAlt += 100.0F;
	af->ClearFlag(AirframeClass::Refueling);
	flags |= ClearingPlane;
	//curThirsty = NULL;
	flags &= ~GivingGas;
	flags &= ~PatternDefined;
	//flags &= ~IsRefueling;

//me123 transfere ownship back to host when we are done rf
	if (vuLocalSessionEntity &&
		 vuLocalSessionEntity->Game() &&
		 self->OwnerId() != vuLocalSessionEntity->Game()->OwnerId())
		 {
	 		FalconSimCampMessage	*msg = new FalconSimCampMessage (self->GetCampaignObject()->Id(), FalconLocalGame); // target);
	 		msg->dataBlock.from = vuLocalSessionEntity->Game()->OwnerId();
	 		msg->dataBlock.message = FalconSimCampMessage::simcampChangeOwner;
	 		FalconSendMessage(msg);
		 }
	turnallow = false;
	HeadsUp = true;
	reachedFirstTrackpoint = false;
}

void TankerBrain::DriveBoom (void)
{
	float tmpAz, tmpRange, tempEl;
	
	FalconTankerMessage *tankMsg;
	
	if(type == TNKR_IL78 || type == TNKR_UNKNOWN)
	{
	    int range = 0;
	    if (tankingPtr && tankingPtr->localData->range < 800)
		range = -IL78EXT;
//	    for (int i = 0; i < numBooms; i++) { // only extend active one
	    int i = IL78BOOM;
		if (boom[i].drawPointer) {
		    if (boom[i].ext > range) {
			boom[i].ext -= 5.0F * SimLibLastMajorFrameTime;
		    }
		    else if (boom[i].ext < range) {
			boom[i].ext += 5.0F * SimLibLastMajorFrameTime;
		    }
		    boom[i].ext = max ( min (boom[i].ext, 1.0F), -40);
		    boom[i].drawPointer->SetDOFoffset(0, boom[i].ext);
		}
//	    }
	    // cut & paste job - no time to make it pretty.... (sigh)
	    int tmpRefuelMode;
	    
	    // Is it the player here
	    if (curThirsty == SimDriver.playerEntity)
		tmpRefuelMode = PlayerOptions.GetRefuelingMode();
	    // Nope, use 'very' easy refuelling
	    else
		tmpRefuelMode = 4;
	    
	    tmpAz = 0;
	    tempEl = 0;
	    if (!(flags & GivingGas) && (!tankingPtr || tankingPtr->localData->range > 800.0F) || (flags & ClearingPlane))
	    {
		tmpAz = 0.0F;
		tempEl = 0;
	    }
	    else if (!(flags & GivingGas) && (!tankingPtr || tankingPtr->localData->range - 33.5F > 30.0F))
	    {
		tmpAz = 0.0F;
		tempEl = 0;
	    }
	    else
	    {
		tempEl = tankingPtr->localData->el;	
		tmpAz = tankingPtr->localData->az;
	    }
	    
	    if(boom[IL78BOOM].az - tmpAz > FOLLOW_RATE * SimLibLastMajorFrameTime)
		boom[IL78BOOM].az = boom[IL78BOOM].az - FOLLOW_RATE * SimLibLastMajorFrameTime;
	    else if(boom[IL78BOOM].az - tmpAz < -FOLLOW_RATE * SimLibLastMajorFrameTime)
		boom[IL78BOOM].az = boom[IL78BOOM].az + FOLLOW_RATE * SimLibLastMajorFrameTime;
	    else
		boom[IL78BOOM].az = tmpAz;
	    
	    if(boom[IL78BOOM].el - tempEl > FOLLOW_RATE * SimLibLastMajorFrameTime)
		boom[IL78BOOM].el = boom[IL78BOOM].el - FOLLOW_RATE * SimLibLastMajorFrameTime;
	    else if(boom[IL78BOOM].el - tempEl < -FOLLOW_RATE * SimLibLastMajorFrameTime)
		boom[IL78BOOM].el = boom[IL78BOOM].el + FOLLOW_RATE * SimLibLastMajorFrameTime;
	    else
		boom[IL78BOOM].el = tempEl;

#if 0 // not done yet
	    if(boom[IL78BOOM].ext - tmpRange > 5.0F * SimLibLastMajorFrameTime)
		boom[IL78BOOM].ext = boom[IL78BOOM].ext - 5.0F * SimLibLastMajorFrameTime;
	    else if(boom[IL78BOOM].ext - tmpRange < -5.0F * SimLibLastMajorFrameTime)
		boom[IL78BOOM].ext = boom[IL78BOOM].ext + 5.0F * SimLibLastMajorFrameTime;
	    else
		boom[IL78BOOM].ext = tmpRange;	
#endif
	    tmpRange = boom[IL78BOOM].ext;
	    Tpoint minB, maxB;
	    if (curThirsty && curThirsty->drawPointer) {
		((DrawableBSP*)curThirsty->drawPointer)->GetBoundingBox(&minB, &maxB);
		tmpRange -= maxB.x;
	    }
	    
	    boom[IL78BOOM].az = max ( min (boom[IL78BOOM].az,  4.0F * DTR), -4.0F * DTR);
	    boom[IL78BOOM].el = max ( min (boom[IL78BOOM].el, 10.0f * DTR), -10.0F * DTR);
			if (boom[IL78BOOM].drawPointer)
			{
				boom[IL78BOOM].drawPointer->SetDOFangle(BOOM_AZIMUTH, -boom[IL78BOOM].az);
				boom[IL78BOOM].drawPointer->SetDOFangle(BOOM_ELEVATION, -boom[IL78BOOM].el);
			}
// 2002-03-08 MN HACK add in the drawpointer radius - this should fix IL78BOOM for each aircraft
// Tested with SU-27, MiG-29, MiG-23, MiG-19, TU-16 and IL-28
		float totalrange = 999.9F;
// 2002-03-09 MN changed to use aircraft datafile value - safer for fixing IL78 for each aircraft
 		if (tankingPtr && curThirsty)
 			totalrange = tmpRange + ((AircraftClass*)curThirsty)->af->GetIL78Factor() + tankingPtr->localData->range;
 
 		if (curThirsty && (g_nShowDebugLabels & 0x4000))
 		{
 			char label[32];
 			sprintf(label,"%f",totalrange);
 			((DrawableBSP*)curThirsty->drawPointer)->SetLabel(label, ((DrawableBSP*)curThirsty->drawPointer)->LabelColor());
 		}

	    if (!(flags & (GivingGas | ClearingPlane)) && tankingPtr)
	    {			
		if (fabs(totalrange) < 6.0F &&
		    fabs(boom[IL78BOOM].az - tmpAz)*RTD < 1.0F &&
		    fabs(boom[IL78BOOM].el - tankingPtr->localData->el)*RTD < 1.0F &&
		    boom[IL78BOOM].el*RTD < 5.0F &&
		    boom[IL78BOOM].el*RTD > -5.0F && 
		    fabs(boom[IL78BOOM].az)*RTD < 20.0F &&
		    fabs(tankingPtr->BaseData()->Roll())*RTD < 4.0F * /* S.G.*/ tmpRefuelMode &&
		    fabs(tankingPtr->BaseData()->Pitch())*RTD < 6.0F * /* S.G.*/ tmpRefuelMode)
		{
		    flags |= GivingGas;
		    tankMsg = new FalconTankerMessage (self->Id(), FalconLocalGame);
		    tankMsg->dataBlock.caller = curThirsty->Id();
		    tankMsg->dataBlock.type = FalconTankerMessage::Contact;
		    tankMsg->dataBlock.data1 = tankMsg->dataBlock.data2 = 0;
		    FalconSendMessage (tankMsg);
		}
	    }
	    else if(tankingPtr && !(flags & ClearingPlane))
	    {
		tmpAz = tankingPtr->localData->az;
		if (fabs(totalrange ) > 8.0F ||
		    fabs(boom[IL78BOOM].az - tmpAz)*RTD > 3.0F ||
		    fabs(boom[IL78BOOM].el - tankingPtr->localData->el)*RTD > 3.0F ||
		    fabs(tankingPtr->BaseData()->Roll())*RTD > 5.0F * /* S.G.*/ tmpRefuelMode ||
		    fabs(tankingPtr->BaseData()->Pitch())*RTD > 8.0F * /* S.G.*/ tmpRefuelMode ||
		    tankingPtr->localData->el*RTD > 5.0F || 
		    tankingPtr->localData->el*RTD < -5.0F || 
		    fabs(tankingPtr->localData->az)*RTD > 23.0F )
		{
		    flags &= ~GivingGas;
		    tankMsg = new FalconTankerMessage (self->Id(), FalconLocalGame);
		    tankMsg->dataBlock.caller = curThirsty->Id();
		    tankMsg->dataBlock.type = FalconTankerMessage::Disconnect;
		    tankMsg->dataBlock.data1 = tankMsg->dataBlock.data2 = 0;
		    FalconSendMessage (tankMsg);
		}
	    }
	    else if( (flags & ClearingPlane) && tankingPtr && 
		tankingPtr->localData->range > 0.04F * NM_TO_FT)
	    {
		VuEntity		*entity = NULL;
		VuListIterator	myit(thirstyQ);
		entity = myit.GetFirst();
		entity = myit.GetNext();
		if(entity && ((SimBaseClass*)entity)->GetCampaignObject() == curThirsty->GetCampaignObject())
		    AddToWaitQ(curThirsty);
		else 
		{
		    PurgeWaitQ();
		    FalconTankerMessage *tankMsg;
		    
		    tankMsg = new FalconTankerMessage (self->Id(), FalconLocalGame);
		    tankMsg->dataBlock.caller = curThirsty->Id();
		    tankMsg->dataBlock.type = FalconTankerMessage::PositionUpdate;
		    tankMsg->dataBlock.data1 = -1;
		    FalconSendMessage (tankMsg);
		}
		
		RemoveFromQ(curThirsty);
		tankingPtr->Release( SIM_OBJ_REF_ARGS );
		tankingPtr = NULL;
		flags &= ~ClearingPlane;
		flags &= ~IsRefueling;
	    }
	    else
	    {
		if(tankingPtr)
		    tankingPtr->Release( SIM_OBJ_REF_ARGS );
		tankingPtr = NULL;
		//flags &= ~ClearingPlane;
		//flags &= ~IsRefueling;
	    }
#if 0 // old code
	    if(tankingPtr && tankingPtr->localData->range < 100.0F)
	    {
		tankMsg = new FalconTankerMessage (self->Id(), FalconLocalGame);
		tankMsg->dataBlock.caller = curThirsty->Id();
		tankMsg->dataBlock.type = FalconTankerMessage::DoneRefueling;
		tankMsg->dataBlock.data1 = tankMsg->dataBlock.data2 = 0;
		FalconSendMessage (tankMsg);
	    }
#endif
	    return;
	}

	if(!boom[0].drawPointer)
		return;

	tmpRange = 0.0F;

	if (!(flags & GivingGas) && (!tankingPtr || tankingPtr->localData->range > 800.0F) || (flags & ClearingPlane))
	{
		tmpAz = 0.0F;
		tempEl = 8.0F*DTR;
		tmpRange = 0.0F;
	}
	else if (!(flags & GivingGas) && (!tankingPtr || tankingPtr->localData->range - 33.5F > 30.0F))
	{
		tmpAz = 0.0F;
		tempEl = -15.0F*DTR;
		tmpRange = 6.0F;
	}
	else
	{
		//lower and swivel boom first, then extend
		if(tankingPtr->localData->el < -22.0F*DTR)
			tempEl = tankingPtr->localData->el;	
		else
			tempEl = -15.0F;
				
		tmpAz = tankingPtr->localData->az;

		if(boom[0].el*RTD > -27.2F && !(flags & GivingGas))
			tmpRange = 6.0F; 
		else
			tmpRange = tankingPtr->localData->range - 33.5F; 
	}
		
	if(boom[0].az - tmpAz > FOLLOW_RATE * SimLibLastMajorFrameTime)
		boom[0].az = boom[0].az - FOLLOW_RATE * SimLibLastMajorFrameTime;
	else if(boom[0].az - tmpAz < -FOLLOW_RATE * SimLibLastMajorFrameTime)
		boom[0].az = boom[0].az + FOLLOW_RATE * SimLibLastMajorFrameTime;
	else
		boom[0].az = tmpAz;

	if(boom[0].el - tempEl > FOLLOW_RATE * SimLibLastMajorFrameTime)
		boom[0].el = boom[0].el - FOLLOW_RATE * SimLibLastMajorFrameTime;
	else if(boom[0].el - tempEl < -FOLLOW_RATE * SimLibLastMajorFrameTime)
		boom[0].el = boom[0].el + FOLLOW_RATE * SimLibLastMajorFrameTime;
	else
		boom[0].el = tempEl;

	if(boom[0].ext - tmpRange > 5.0F * SimLibLastMajorFrameTime)
		boom[0].ext = boom[0].ext - 5.0F * SimLibLastMajorFrameTime;
	else if(boom[0].ext - tmpRange < -5.0F * SimLibLastMajorFrameTime)
		boom[0].ext = boom[0].ext + 5.0F * SimLibLastMajorFrameTime;
	else
		boom[0].ext = tmpRange;	
	
	
	boom[0].az = max ( min (boom[0].az,  23.0F * DTR), -23.0F * DTR);
	boom[0].el = max ( min (boom[0].el, 10.0F * DTR), -40.0F * DTR);
	boom[0].ext = max ( min (boom[0].ext, 27.0F), 1.0F); 

#ifdef DAVE_DBG
	if(tankingPtr)
	{
		MonoPrint("El: %4.2f  Az: %4.2f  Ext: %4.2f  REl: %4.2f  RRng: %4.2f\n", boom[0].el*RTD, boom[0].az*RTD, boom[0].ext, tankingPtr->localData->el*RTD, tankingPtr->localData->range ); 
		MonoPrint("RngErr: %4.2f  ElErr: %4.2f  AzErr: %4.2f\n", (boom[0].ext - tankingPtr->localData->range + 33.5F), (boom[0].el - tankingPtr->localData->el)*RTD, (boom[0].az - tmpAz)*RTD);
	}
#endif
	/*		
	Tpoint boomtip, receptor, recWPos;
	BoomTipPosition(&boomtip);

	receptor.x = 0.0F;
	receptor.y = 0.0F;
	receptor.z = -3.0F;
	
	MatrixMult( &((DrawableBSP*)((SimVehicleClass*)tankingPtr->BaseData())->drawPointer)->orientation, &receptor, &recWPos );
	recWPos.x += tankingPtr->BaseData()->XPos();
	recWPos.y += tankingPtr->BaseData()->YPos();
	recWPos.z += tankingPtr->BaseData()->ZPos();
	*/
	//MonoPrint("Xdiff: %6.3f  Ydiff: %6.3f  Zdiff: %6.3f\n",boomtip.x - recWPos.x,boomtip.y - recWPos.y,boomtip.z - recWPos.z);
	//MonoPrint("Tkr Alt: %7.2f  Tkr Theta: %4.2f  Plane Alt: %7.2f  Plane Theta: %4.2f\n",
//				-self->ZPos(), self->af->theta*RTD, -tankingPtr->BaseData()->ZPos(), ((AircraftClass*)tankingPtr->BaseData())->af->theta*RTD);
	
	boom[0].drawPointer->SetDOFangle(BOOM_AZIMUTH, -boom[0].az);
	boom[0].drawPointer->SetDOFangle(BOOM_ELEVATION, -boom[0].el);
	boom[0].drawPointer->SetDOFoffset(BOOM_EXTENSION, boom[0].ext);

	// OW - sylvains refuelling fix
#if 1
// S.G. ADDED SO AI PILOTS HAVE AN EASIER JOB AT REFUELLING THAN THE PLAYERS
	int tmpRefuelMode;

	// Is it the player here
	if (self == SimDriver.playerEntity)
		tmpRefuelMode = PlayerOptions.GetRefuelingMode();
	// Nope, use 'very' easy refuelling
	else
		tmpRefuelMode = 4;

// THERE WILL BE FOUR USE OF tmpRefuelMode IN THE NEXT if/else if/else statement. THESE USED TO BE PlayerOptions.GetRefuelingMode()
// END OF ADDED SECTION

	if (!(flags & (GivingGas | ClearingPlane)) && tankingPtr)
	{			
		if (fabs(boom[0].ext - tankingPtr->localData->range + 33.5F) < 1.0F &&
			fabs(boom[0].az - tmpAz)*RTD < 1.0F &&
			fabs(boom[0].el - tankingPtr->localData->el)*RTD < 1.0F &&
			boom[0].el*RTD < -26.1F && boom[0].el*RTD > -38.9F && fabs(boom[0].az)*RTD < 20.0F &&
			fabs(tankingPtr->BaseData()->Roll())*RTD < 4.0F * /* S.G.*/ tmpRefuelMode &&
			fabs(tankingPtr->BaseData()->Pitch())*RTD < 4.0F * /* S.G.*/ tmpRefuelMode)
		{
			flags |= GivingGas;
			tankMsg = new FalconTankerMessage (self->Id(), FalconLocalGame);
			tankMsg->dataBlock.caller = curThirsty->Id();
			tankMsg->dataBlock.type = FalconTankerMessage::Contact;
			tankMsg->dataBlock.data1 = tankMsg->dataBlock.data2 = 0;
			FalconSendMessage (tankMsg);
		}
	}
	else if(tankingPtr && !(flags & ClearingPlane))
	{
		if (fabs(boom[0].ext - tankingPtr->localData->range + 33.5F) > 2.0F ||
			fabs(boom[0].az - tmpAz)*RTD > 2.0F ||
			fabs(boom[0].el - tankingPtr->localData->el)*RTD > 2.0F ||
			fabs(tankingPtr->BaseData()->Roll())*RTD > 5.0F * /* S.G.*/ tmpRefuelMode ||
			fabs(tankingPtr->BaseData()->Pitch())*RTD > 5.0F * /* S.G.*/ tmpRefuelMode ||
			tankingPtr->localData->el*RTD > -25.0F || tankingPtr->localData->el*RTD < -40.0F || fabs(tankingPtr->localData->az)*RTD > 23.0F )
		{
			flags &= ~GivingGas;
			tankMsg = new FalconTankerMessage (self->Id(), FalconLocalGame);
			tankMsg->dataBlock.caller = curThirsty->Id();
			tankMsg->dataBlock.type = FalconTankerMessage::Disconnect;
			tankMsg->dataBlock.data1 = tankMsg->dataBlock.data2 = 0;
			FalconSendMessage (tankMsg);
		}
	}
#else
	if (!(flags & (GivingGas | ClearingPlane)) && tankingPtr)
	{			
		if (fabs(boom[0].ext - tankingPtr->localData->range + 33.5F) < 1.0F &&
			fabs(boom[0].az - tmpAz)*RTD < 1.0F &&
			fabs(boom[0].el - tankingPtr->localData->el)*RTD < 1.0F &&
			boom[0].el*RTD < -26.1F && boom[0].el*RTD > -38.9F && fabs(boom[0].az)*RTD < 20.0F &&
			fabs(tankingPtr->BaseData()->Roll())*RTD < 4.0F * PlayerOptions.GetRefuelingMode() &&
			fabs(tankingPtr->BaseData()->Pitch())*RTD < 4.0F * PlayerOptions.GetRefuelingMode())
		{
			flags |= GivingGas;
			tankMsg = new FalconTankerMessage (self->Id(), FalconLocalGame);
			tankMsg->dataBlock.caller = curThirsty->Id();
			tankMsg->dataBlock.type = FalconTankerMessage::Contact;
			tankMsg->dataBlock.data1 = tankMsg->dataBlock.data2 = 0;
			FalconSendMessage (tankMsg);
		}
	}
	else if(tankingPtr && !(flags & ClearingPlane))
	{
		if (fabs(boom[0].ext - tankingPtr->localData->range + 33.5F) > 2.0F ||
			fabs(boom[0].az - tmpAz)*RTD > 2.0F ||
			fabs(boom[0].el - tankingPtr->localData->el)*RTD > 2.0F ||
			fabs(tankingPtr->BaseData()->Roll())*RTD > 5.0F * PlayerOptions.GetRefuelingMode() ||
			fabs(tankingPtr->BaseData()->Pitch())*RTD > 5.0F * PlayerOptions.GetRefuelingMode() ||
			tankingPtr->localData->el*RTD > -25.0F || tankingPtr->localData->el*RTD < -40.0F || fabs(tankingPtr->localData->az)*RTD > 23.0F )
		{
			flags &= ~GivingGas;
			tankMsg = new FalconTankerMessage (self->Id(), FalconLocalGame);
			tankMsg->dataBlock.caller = curThirsty->Id();
			tankMsg->dataBlock.type = FalconTankerMessage::Disconnect;
			tankMsg->dataBlock.data1 = tankMsg->dataBlock.data2 = 0;
			FalconSendMessage (tankMsg);
		}
	}
#endif
	else if( (flags & ClearingPlane) && tankingPtr && tankingPtr->localData->range > 0.04F * NM_TO_FT)
	{
		VuEntity		*entity = NULL;
		VuListIterator	myit(thirstyQ);
		entity = myit.GetFirst();
		entity = myit.GetNext();
		if(entity && ((SimBaseClass*)entity)->GetCampaignObject() == curThirsty->GetCampaignObject())
			AddToWaitQ(curThirsty);
		else 
		{
			PurgeWaitQ();
			FalconTankerMessage *tankMsg;

			tankMsg = new FalconTankerMessage (self->Id(), FalconLocalGame);
			tankMsg->dataBlock.caller = curThirsty->Id();
			tankMsg->dataBlock.type = FalconTankerMessage::PositionUpdate;
			tankMsg->dataBlock.data1 = -1;
			FalconSendMessage (tankMsg);
		}

		RemoveFromQ(curThirsty);
		tankingPtr->Release( SIM_OBJ_REF_ARGS );
		tankingPtr = NULL;
		flags &= ~ClearingPlane;
		flags &= ~IsRefueling;
	}
	else
	{
		if(tankingPtr)
			tankingPtr->Release( SIM_OBJ_REF_ARGS );
		tankingPtr = NULL;
		//flags &= ~ClearingPlane;
		//flags &= ~IsRefueling;
	}
}

void TankerBrain::DriveLights (void)
{
	
	int lightVal;
	
	// possible CTD fix
	if (!self->drawPointer)
		return;

	if (tankingPtr)
	{
		//float elOff = -tankingPtr->localData->el * RTD;
		//float rangeOff = tankingPtr->localData->range - 33.5F;

		// Up/Down light
		// Turn on the Labels
		lightVal = (1 << 6) + 1;
		
		Tpoint relPos;
		ReceptorRelPosition(&relPos, curThirsty);

// 2002-03-06 MN calculate relative position based on aircrafts refuel position
		Tpoint boompos;
		boompos.x = boompos.z = 0;

	    if (curThirsty && curThirsty->IsAirplane() && ((AircraftClass*)curThirsty)->af) 
		{
			((AircraftClass*)curThirsty)->af->GetRefuelPosition(&boompos);
	    }
	    // if nothing set, use F-16 default
	    if (boompos.x == 0 && boompos.y ==0 && boompos.z == 0) 
		{
			boompos.x = -39.63939795321F;
			boompos.z = 25.2530815923F;
	    }

		relPos.x -= boompos.x; // relPos.x += 39.63939795321F;
		relPos.z -= boompos.z; // relPos.z -= 25.2530815923F;
#if 0
		// Set the right arrow
		if (relPos.z < -25.0F)
			lightVal += (1 << 5);
		else if (relPos.z < -15.0F)
			lightVal += (3 << 4);
		else if (relPos.z < -8.0F)
			lightVal += (1 << 4);
		else if (relPos.z < -2.5F)
			lightVal += (3 << 3);
		else if (relPos.z < 2.5F)
			lightVal += (1 << 3);
		else if (relPos.z < 8.0F)
			lightVal += (3 << 2);
		else if (relPos.z < 15.0F)
			lightVal += (1 << 2);
		else if (relPos.z < 25.0F)
			lightVal += (3 << 1);
		else 
			lightVal += (1 << 1);
#else										// M.N. Turn around lights as IRL does it work this way ??
		if (relPos.z < -25.0F)
			lightVal += (3 << 1);
		else if (relPos.z < -15.0F)
			lightVal += (1 << 2);
		else if (relPos.z < -8.0F)
			lightVal += (3 << 2);
		else if (relPos.z < -2.5F)
			lightVal += (1 << 3);
		else if (relPos.z < 2.5F)
			lightVal += (3 << 3);
		else if (relPos.z < 8.0F)
			lightVal += (1 << 4);		
		else if (relPos.z < 15.0F)
			lightVal += (3 << 4);
		else if (relPos.z < 25.0F)
			lightVal += (1 << 5);
		else 
			lightVal += (1 << 1);
#endif

		/*
		if(tankingPtr->localData->az > 0.0F)
		{
			// Set the right arrow
			if (elOff < 26.1F)
				lightVal += (0x1 << 5);
			else if (elOff < 27.2F)
				lightVal += (0x11 << 4);
			else if (elOff < 28.4F)
				lightVal += (0x1 << 4);
			else if (elOff < 29.5F)
				lightVal += (0x11 << 3);
			else if (elOff < 35.5F)
				lightVal += (0x1 << 3);
			else if (elOff < 36.6F)
				lightVal += (0x11 << 2);
			else if (elOff < 37.8F)
				lightVal += (0x1 << 2);
			else if (elOff < 38.9F)
				lightVal += (0x11 << 1);
			else 
				lightVal += (0x1 << 1);
		}
		else
		{
			lightVal += (0x1 << 1);
		}*/
		
		((DrawableBSP*)self->drawPointer)->SetSwitchMask(0, lightVal);
		
		// Fore/Aft light
		// Turn on the Labels
		lightVal = (1 << 6) + 1;
#if 0
		// Set the right arrow
		if (relPos.x < -25.0F)
			lightVal += (1 << 5);
		else if (relPos.x < -15.0F)
			lightVal += (3 << 4);
		else if (relPos.x < -8.0F)
			lightVal += (1 << 4);
		else if (relPos.x < -2.5F)
			lightVal += (3 << 3);
		else if (relPos.x < 2.5F)
			lightVal += (1 << 3);
		else if (relPos.x < 8.0F)
			lightVal += (3 << 2);
		else if (relPos.x < 15.0F)
			lightVal += (1 << 2);
		else if (relPos.x < 25.0F)
			lightVal += (3 << 1);
		else 
			lightVal += (1 << 1);
#else 
				// Set the right arrow
		if (relPos.x < -25.0F)
			lightVal += (3 << 1);
		else if (relPos.x < -15.0F)
			lightVal += (1 << 2);
		else if (relPos.x < -8.0F)
			lightVal += (3 << 2);
		else if (relPos.x < -2.5F)
			lightVal += (1 << 3);
		else if (relPos.x < 2.5F)
			lightVal += (3 << 3);
		else if (relPos.x < 8.0F)
			lightVal += (1 << 4);
		else if (relPos.x < 15.0F)
			lightVal += (3 << 4);
		else if (relPos.x < 25.0F)
			lightVal += (1 << 5);
		else 
			lightVal += (1 << 1);
#endif
		/*
		// Set the right arrow
		if (rangeOff > 18.5F)
			lightVal += (0x1 << 5);
		else if (rangeOff > 17.25F)
			lightVal += (0x11 << 4);
		else if (rangeOff > 16.0F)
			lightVal += (0x1 << 4);
		else if (rangeOff > 14.75F)
			lightVal += (0x11 << 3);
		else if (rangeOff > 12.25F)
			lightVal += (0x1 << 3);
		else if (rangeOff > 11.0F)
			lightVal += (0x11 << 2);
		else if (rangeOff > 9.75F)
			lightVal += (0x1 << 2);
		else if (rangeOff > 8.55F)
			lightVal += (0x11 << 1);
		else 
			lightVal += (0x1 << 1);*/
		
		((DrawableBSP*)self->drawPointer)->SetSwitchMask(1, lightVal);
	}
	else
	{
		((DrawableBSP*)self->drawPointer)->SetSwitchMask(0, 0);
		((DrawableBSP*)self->drawPointer)->SetSwitchMask(1, 0);
	}
}

void TankerBrain::BreakAway (void)
{
	FalconTankerMessage* tankMsg;
	
	tankMsg = new FalconTankerMessage (self->Id(), FalconLocalGame);
	tankMsg->dataBlock.caller = curThirsty->Id();
	tankMsg->dataBlock.type = FalconTankerMessage::Breakaway;
	tankMsg->dataBlock.data1 = tankMsg->dataBlock.data2 = 0;
	FalconSendMessage (tankMsg);
	turnallow = false;
	HeadsUp = true;
	reachedFirstTrackpoint = false;	
}

void TankerBrain::TurnTo (float newHeading)
{
	// M.N. 2001-11-28
	float dist, dx, dy;	

	// Set up new trackpoint
	dist = 200.0F * NM_TO_FT;

	dx = self->XPos() + dist * sin (newHeading);
	dy = self->YPos() + dist * cos (newHeading);

	trackX = dx;
	trackY = dy;
}

void TankerBrain::TurnToTrackPoint (int trackPoint)
{
	// Set up new trackpoint
	trackX = TrackPoints[trackPoint].x;
	trackY = TrackPoints[trackPoint].y;
}


void TankerBrain::FollowThirsty (void)
{
	float xyRange, dist, dx, dy, heading;
	float oldAz, oldEl, oldRange;
	FalconTankerMessage* tankMsg;
	
		

	// Find the thirsty one
	if (!tankingPtr || tankingPtr->BaseData() != curThirsty)
	   {
		if (tankingPtr)
			tankingPtr->Release( SIM_OBJ_REF_ARGS );
#ifdef DEBUG
		tankingPtr = new SimObjectType( OBJ_TAG, self, curThirsty );
#else
		tankingPtr = new SimObjectType(curThirsty);
#endif
		tankingPtr->Reference( SIM_OBJ_REF_ARGS );
		dist = DistanceToFront(SimToGrid(self->YPos()),SimToGrid(self->XPos()));		
		if (!g_bUseTankerTrack && dist > 60.0F && dist < 100.0F)
		{
			turnallow = true;	// allow a new turn
			HeadsUp = true;
		}
	   }
	
	if (tankingPtr)
	{
		float inverseTimeDelta = 1.0F / SimLibLastMajorFrameTime;

		Tpoint relPos;
		ReceptorRelPosition(&relPos, curThirsty);

		oldRange = tankingPtr->localData->range;
		tankingPtr->localData->range = (float)sqrt(relPos.x*relPos.x + relPos.y*relPos.y + relPos.z*relPos.z);
		tankingPtr->localData->rangedot = (tankingPtr->localData->range - oldRange) * inverseTimeDelta;		
		tankingPtr->localData->range = max (tankingPtr->localData->range, 0.01F);
		xyRange = (float)sqrt(relPos.x*relPos.x + relPos.y*relPos.y);

		/*-------*/
		/* az    */
		/*-------*/
		oldAz = tankingPtr->localData->az;
		tankingPtr->localData->az = (float)atan2(relPos.y,-relPos.x);
		tankingPtr->localData->azFromdot = (tankingPtr->localData->az - oldAz) * inverseTimeDelta;
		
		/*-------*/
		/* elev  */
		/*-------*/
		oldEl = tankingPtr->localData->el;
		if (xyRange != 0.0)
			tankingPtr->localData->el = (float)atan(-relPos.z/xyRange);
		else
			tankingPtr->localData->el = (relPos.z < 0.0F ? -90.0F * DTR : 90.0F * DTR);
		tankingPtr->localData->elFromdot = (tankingPtr->localData->el - oldEl) * inverseTimeDelta;

		trackZ = -holdAlt;

		if(flags & ClearingPlane)
			desSpeed = af->CalcTASfromCAS(335.0F)*KNOTS_TO_FTPSEC;
		else 	// use refuel speed for this aircraft
			desSpeed = af->CalcTASfromCAS(((AircraftClass*)curThirsty)->af->GetRefuelSpeed())*KNOTS_TO_FTPSEC;

// 2002-03-13 MN box tanker track
		if (g_bUseTankerTrack)
		{
/* 
	1) Check distance to our current Trackpoint
	2) If distance is < g_fTankerHeadsupDistance, make Headsup call
	3) If distance is < g_fTankerTrackFactor or distance to current trackpoint increases again, head towards the next trackPoint

	If advancedirection is set true, then tanker was outside his max range envelope when
	called for refueling. In this case when switching from Trackpoint 0 to Trackpoint 1,
	tanker would do a 180° turn - so just reverse the order from 0->1->2->3 to 0->3->2->1
*/
			dist = DistSqu(self->XPos(), self->YPos(), TrackPoints[currentTP].x, TrackPoints[currentTP].y);

// trackPointDistance is always the closest distance to current trackpoint when close to it.
// If tanker doesn't get "the curve" to catch the trackpoint at g_fTankerTrackFactor distance,
// which means dist > trackPointDistance again, do the turn to next trackpoint nevertheless
// (sort of backup function to keep the tanker turning in a track...)
			if (dist < (g_fTankerBackupDistance) * NM_TO_FT * (g_fTankerBackupDistance) * NM_TO_FT )
			{
				if (dist < trackPointDistance)
					trackPointDistance = dist;
			}
			else
				trackPointDistance = (0.5f + g_fTankerBackupDistance) * NM_TO_FT * (0.5f + g_fTankerBackupDistance) * NM_TO_FT;

			if ((dist < (g_fTankerHeadsupDistance) * NM_TO_FT * (g_fTankerHeadsupDistance) * NM_TO_FT ) && HeadsUp)
			{
				reachedFirstTrackpoint = true; // from now on limit rStick, pStick and flight model (afsimple.cpp) until refueling is done
				HeadsUp = false;
				turnallow = true;
				tankMsg = new FalconTankerMessage (self->Id(), FalconLocalGame);
				tankMsg->dataBlock.caller = curThirsty->Id();
				tankMsg->dataBlock.type = FalconTankerMessage::TankerTurn;
				tankMsg->dataBlock.data1 = tankMsg->dataBlock.data2 = 0;
				FalconSendMessage (tankMsg);
			}
// distance to trackpoint increases again or we are closer than g_fTankerTrackFactor nm ? -> switch to next trackpoint
			if ((dist > trackPointDistance || (dist < (g_fTankerTrackFactor) * NM_TO_FT * (g_fTankerTrackFactor) * NM_TO_FT)) && turnallow)
			{
     			HeadsUp = true;
				turnallow = false;
				if (advancedirection)
					currentTP--;
				else 
					currentTP++;
				if (currentTP > 3)
					currentTP = 0;
				if (currentTP < 0)
					currentTP = 3;
				TurnToTrackPoint(currentTP);
			}
		}
		else
		{
			dist = DistanceToFront(SimToGrid(self->YPos()),SimToGrid(self->XPos()));		

			if (dist > 45.0F && dist < 140.0F || dist > 200.0F)
			{
				HeadsUp = true;		// and a HeadsUp call
			}

			if ((dist <= 36.0F || dist >= 149.0F) && HeadsUp)
			{
				HeadsUp = false;
				turnallow = true;
				tankMsg = new FalconTankerMessage (self->Id(), FalconLocalGame);
				tankMsg->dataBlock.caller = curThirsty->Id();
				tankMsg->dataBlock.type = FalconTankerMessage::TankerTurn;
				tankMsg->dataBlock.data1 = tankMsg->dataBlock.data2 = 0;
				FalconSendMessage (tankMsg);
			}
	
			if (dist < 35.0F || dist > 150.0F)	// refuel between 35 and 150 nm distance to the FLOT
			{
				if (turnallow)
				{
					turnallow = false;
					dx = trackX - self->XPos();
					dy = trackY - self->YPos();
					heading = (float)atan2(dy,dx);
					if(heading < 0.0F)
						heading += PI * 2.0F;
					heading += PI;	// add 180°
					if (heading > PI * 2.0F)
						heading -= PI * 2.0F;
					TurnTo(heading);	// set up new trackpoint and make a radio call announcing the turn
				}
			}
		}

		SimpleTrack(SimpleTrackTanker, desSpeed);

		if (g_bUseTankerTrack)
		{
			dy = TrackPoints[currentTP].y - self->YPos();
			dx = TrackPoints[currentTP].x - self->XPos();

			heading = (float)atan2(dy,dx);
			
			if (fabs(self->Yaw() - heading) < g_fHeadingStabilizeFactor) // when our course is close to trackpoint's direction
				rStick = 0.0F;						  // stabilize Tanker's heading
		}

		if (g_nShowDebugLabels & 0x800)
		{
			char tmpchr[32];
			dist = (float)sqrt(dist);
			dist *= FT_TO_NM;
			float yaw = self->Yaw();
			if (yaw < 0.0f)
				yaw += 360.0F * DTR;
			yaw *= RTD;

			sprintf(tmpchr,"%3.2f %5.1f   %3.0f %3.2f TP %d",af->vcas, -self->ZPos(),yaw, dist,currentTP);
			if (self->drawPointer)
				((DrawableBSP*)self->drawPointer)->SetLabel (tmpchr, ((DrawableBSP*)self->drawPointer)->LabelColor());
		}
		
		// Set the lights
		if(type == TNKR_KC10)
			DriveLights();	
		else if(type == TNKR_KC135 && g_bLightsKC135)	// when we have the lights on the KC-135 model
			DriveLights();
		
		if (xyRange < 500.0F && !(flags & PrecontactPos) &&
			fabs(tankingPtr->localData->rangedot) < 100.0F &&
			fabs(tankingPtr->localData->az) < 35.0F * DTR)
		{
			flags |= PrecontactPos;
			// Call into contact position
			tankMsg = new FalconTankerMessage (self->Id(), FalconLocalGame);
			tankMsg->dataBlock.caller = curThirsty->Id();
			tankMsg->dataBlock.type = FalconTankerMessage::ClearToContact;
			tankMsg->dataBlock.data1 = tankMsg->dataBlock.data2 = 0;
			FalconSendMessage (tankMsg);
		}
		
		if(type != TNKR_IL78 && !(flags & ClearingPlane))
		{
			if (xyRange < 200.0F && !(flags & GivingGas) &&
				(SimLibElapsedTime - lastBoomCommand) > 10000)
			{
				lastBoomCommand = SimLibElapsedTime;
				// Call boom commands
				tankMsg = new FalconTankerMessage (self->Id(), FalconLocalGame);
				tankMsg->dataBlock.caller = curThirsty->Id();
				tankMsg->dataBlock.type = FalconTankerMessage::BoomCommand;
				
// 2002-03-06 MN calculate relative position based on aircrafts refuel position
				Tpoint boompos;
				boompos.x = boompos.z = 0;

			    if (curThirsty && curThirsty->IsAirplane() && ((AircraftClass*)curThirsty)->af) 
				{
					((AircraftClass*)curThirsty)->af->GetRefuelPosition(&boompos);
				}
			    // if nothing set, use F-16 default
			    if (boompos.x == 0 && boompos.y ==0 && boompos.z == 0) 
				{
					boompos.x = -39.63939795321F;
					boompos.z = 25.2530815923F;
			    }

				relPos.x -= boompos.x; // relPos.x += 39.63939795321F;
				relPos.z -= boompos.z; // relPos.z -= 25.2530815923F;

				// pos rx is towards the front of the tanker
				// pos ry is to the right of the tanker
				// pos rz is to the bottom of the tanker
				if(fabs(relPos.x) > fabs(relPos.y) && fabs(relPos.x) > fabs(relPos.z))
				{
					if(relPos.x < 0.0f)
						tankMsg->dataBlock.data1 = MOVE_FORWARD; 
					else
						tankMsg->dataBlock.data1 = MOVE_BACK; 
				}
				else if(fabs(relPos.z) > fabs(relPos.y))
				{
					if(relPos.z < 0.0f)
						tankMsg->dataBlock.data1 = MOVE_DOWN;
					else
						tankMsg->dataBlock.data1 = MOVE_UP;
				}
				else
				{
					if(relPos.y > 0.0f)
						tankMsg->dataBlock.data1 = MOVE_LEFT; 
					else
						tankMsg->dataBlock.data1 = MOVE_RIGHT;
				}
				
				tankMsg->dataBlock.data2 = 0;
				FalconSendMessage (tankMsg);
			}
		}
		
		// Too Eratic?
		if ( !(flags & ClearingPlane) && (flags & GivingGas) && (SimLibElapsedTime - lastStabalize) > 15000 &&
			fabs (tankingPtr->localData->azFromdot) > 10.0F * DTR && fabs (tankingPtr->localData->elFromdot) > 10.0F * DTR)
		{
			lastStabalize = SimLibElapsedTime;
			// Call stablize command
			tankMsg = new FalconTankerMessage ( self->Id(), FalconLocalGame);
			tankMsg->dataBlock.caller = curThirsty->Id();
			tankMsg->dataBlock.type = FalconTankerMessage::Stabalize;
			tankMsg->dataBlock.data1 = tankMsg->dataBlock.data2 = 0;
			FalconSendMessage (tankMsg);
		}
	}
	else
	{
		DoneRefueling();
	}

	// Stick it
	DriveBoom();
}

int TankerBrain::TankingPosition(SimVehicleClass* thirstyOne)
{
	VuEntity		*entity = NULL;
	VuListIterator	myit(thirstyQ);
	int count = 0;

	entity = myit.GetFirst();
	while(entity)
	{
		if(entity == thirstyOne)
		{
			return count;
		}
		
		count++;
		entity = myit.GetNext();
	}
	return -1;
}

int TankerBrain::AddToQ (SimVehicleClass* thirstyOne)
{
	VuEntity		*entity = NULL;
	int count = 0;

	ShiAssert (thirstyQ);

	if (thirstyQ)
	{
		VuListIterator	myit(thirstyQ);

		entity = myit.GetFirst();
		while(entity)
		{
			if(entity == thirstyOne)
			{
				if(count == 0)
					return -1;
				else
					return count;
			}
			
			count++;
			entity = myit.GetNext();
		}
		thirstyQ->ForcedInsert(thirstyOne);
		if(count != 0)
		{
			FalconTankerMessage *tankMsg;

			tankMsg = new FalconTankerMessage (self->Id(), FalconLocalGame);
			tankMsg->dataBlock.caller = thirstyOne->Id();
			tankMsg->dataBlock.type = FalconTankerMessage::PositionUpdate;
			tankMsg->dataBlock.data1 = tankMsg->dataBlock.data1 = (float)count;
			FalconSendMessage (tankMsg);
		}
	}

	return count;
}

void TankerBrain::RemoveFromQ (SimVehicleClass* thirstyOne)
{
	if(thirstyOne == curThirsty)
		curThirsty = NULL;

	thirstyQ->Remove(thirstyOne);
}

int TankerBrain::AddToWaitQ (SimVehicleClass* doneOne)
{
	VuEntity		*entity = NULL;
	VuListIterator	myit(waitQ);
	int count = 1;

	entity = myit.GetFirst();
	while(entity)
	{
		if(entity == doneOne)
		{
			return count;
		}		
		count++;
		entity = myit.GetNext();
	}
	waitQ->ForcedInsert(doneOne);

	count = 1;
	entity = myit.GetFirst();
	while(entity)
	{
		FalconTankerMessage *tankMsg;

		tankMsg = new FalconTankerMessage (self->Id(), FalconLocalGame);
		tankMsg->dataBlock.caller = doneOne->Id();
		tankMsg->dataBlock.type = FalconTankerMessage::PositionUpdate;
		tankMsg->dataBlock.data1 = tankMsg->dataBlock.data1 = (float)count;
		FalconSendMessage (tankMsg);
		count++;
		entity = myit.GetNext();
	}
	
	return 1;
}

void TankerBrain::PurgeWaitQ (void)
{
	VuEntity		*entity = NULL;
	VuListIterator	myit(waitQ);
	entity = myit.GetFirst();
	while(entity)
	{
		FalconTankerMessage *tankMsg;

		tankMsg = new FalconTankerMessage (self->Id(), FalconLocalGame);
		tankMsg->dataBlock.caller = entity->Id();
		tankMsg->dataBlock.type = FalconTankerMessage::PositionUpdate;
		tankMsg->dataBlock.data1 = tankMsg->dataBlock.data1 = -1;
		FalconSendMessage (tankMsg);
		entity = myit.GetNext();
	}

	waitQ->Purge();
}

void TankerBrain::FrameExec (SimObjectType* tList, SimObjectType* tPtr)
{
	if (!(flags & (IsRefueling|ClearingPlane)))
	{
		DigitalBrain::FrameExec(tList, tPtr);
		// Lights Off
		if(type == TNKR_KC10)
			DriveLights();	
		else if(type == TNKR_KC135 && g_bLightsKC135)	// when we have the lights on the KC-135 model
			DriveLights();
#ifdef DAVE_DBG	 
		if(MoveBoom && type != TNKR_IL78)
		{
			boomAzTest = max ( min (boomAzTest,  23.0F * DTR), -23.0F * DTR);
			boomElTest = max ( min (boomElTest, 4.0F * DTR), -40.0F * DTR);
			boomExtTest = max ( min (boomExtTest, 21.0F), 0.0F); 

			boom[0].drawPointer->SetDOFangle(BOOM_AZIMUTH, -boomAzTest);
			boom[0].drawPointer->SetDOFangle(BOOM_ELEVATION, -boomElTest);
			boom[0].drawPointer->SetDOFoffset(BOOM_EXTENSION, boomExtTest);
		}
		else
			DriveBoom();
		
#else
		// Boom Away
		DriveBoom();
#endif
		
		
		CallNext();
	}
	else
	{
		if (g_bUseTankerTrack)
		{
			if (directionsetup)
			{
/*
	1.) Find heading to FLOT
	2.) Get distance to FLOT
	3.) if Distance is too close (MINIMUM_TANKER_DISTANCE, Falcon4.AII), 
		or too far (MAXIMUM_TANKER_DISTANCE, Falcon4.AII), set first tanker trackpoint as the first "Tanker" waypoint
	4.) In normal case: set up TrackPoints array: 
			TP[].x = XPos
			TP[].y = YPos
			TP[].z = Heading (no need to calculate this over and over again...)

			TP[0] = Current location
			TP[1] = 90° 25 nm	<- this is a 180° heading away from the FLOT
			TP[2] = 90° 60 nm	<- parallel to the FLOT
			TP[3] = 90° 25 nm	<- towards the FLOT

			boxside decides if we do a left or right box, according to heading to FLOT

			set currentTP to point 1

	5.) if one of 3.) is true:
			TP[0] = farther away/closer to the FLOT
			TP[1] = 90° 25 nm	<- this is a 180° heading away from the FLOT
			TP[2] = 90° 60 nm	<- parallel to the FLOT
			TP[3] = 90° 25 nm	<- towards the FLOT

			boxside decides if we do a left or right box
			
			set currentTP to point 0

			check if we go from TP 0 to TP 3 or from TP 0 to TP 1
*/
				float dist, dx, dy, longleg, shortleg, heading, distance;
				bool boxside = false;
				int farthercloser = 0;

				reachedFirstTrackpoint = false;

				directionsetup = 0;
				HeadsUp = true;
				advancedirection = false; // means go trackpoint 0->1->2->3

				ShiAssert(curThirsty);
				if (curThirsty)
				{
					longleg = ((AircraftClass*)curThirsty)->af->GetTankerLongLeg();
					shortleg = ((AircraftClass*)curThirsty)->af->GetTankerShortLeg();
				}

				distance = DistanceToFront(SimToGrid(self->YPos()),SimToGrid(self->XPos()));

				if (distance < (float)(MINIMUM_TANKER_DISTANCE - 5))
				{
					farthercloser = 1;
				}
				else if (distance > (float)(MAXIMUM_TANKER_DISTANCE + 5))
				{
					farthercloser = 2;
					advancedirection = true; // go trackpoint 0->3->2->1
				}

				if (farthercloser)
				{

					// Get the first tanker waypoint as first normal trackpoint
					WayPoint w = NULL;
					Flight flight = (Flight)self->GetCampaignObject();
					if (flight)
						w = flight->GetFirstUnitWP(); // this is takeoff time

					// use location of first "Tanker" waypoint if we're outside our min/max FLOT range
					while (w)
					{
						if (w->GetWPAction() == WP_TANKER)
							break;
						w = w->GetNextWP();
					}

					float x,y,z;
					if (w)
						w->GetLocation(&x,&y,&z);
					else
					{
// we have no waypoints - like in refuel training mission
						x = self->XPos();
						y = self->YPos();
					}
					
					heading = DirectionToFront(SimToGrid(y),SimToGrid(x)); // of course from our waypoints location...
				
					if (heading<0.0f)
						heading += PI * 2;

					TrackPoints[0].x = x;
					TrackPoints[0].y = y;
					
					currentTP = 0;
				}
				else
				// tanker is within min/max range to FLOT, so use current position for trackpoint 0
				{
					heading = DirectionToFront(SimToGrid(self->YPos()),SimToGrid(self->XPos()));
				
					if (heading<0.0f)
						heading += PI * 2;

					// TrackPoint[0], current location
					TrackPoints[0].x = self->XPos();
					TrackPoints[0].y = self->YPos();

					currentTP = 1;
				}

				if (heading >= 0.0f && heading < PI/2.0f || heading > PI && heading < PI + PI/2.0f)	// the closest FLOT point is "east" of us
					boxside = true;						// this creates a track box to the right, which should be away from the FLOT, false = to the left

				heading += PI;	// turn now 180° away from the FLOT
				if (heading > PI * 2.0F)
					heading -= PI * 2.0F;

//				TrackPoints[0].z = heading;

				// TrackPoint[1], initial direction
				dist = shortleg * NM_TO_FT;

				dx = TrackPoints[0].x + dist * cos (heading);
				dy = TrackPoints[0].y + dist * sin (heading);
					
				TrackPoints[1].x = dx;
				TrackPoints[1].y = dy;

				// TrackPoint[2] add 90° heading
				if (boxside)
				{
					heading -= PI/2.0f;
					if (heading<0.0f)
						heading += PI * 2.0f;
				}
				else
				{
					heading += PI/2.0f;
					if (heading > PI * 2.0F)
						heading -= PI * 2.0F;
				}

//				TrackPoints[1].z = heading;

				dist = longleg * NM_TO_FT;

				dx = TrackPoints[1].x + dist * cos (heading);
				dy = TrackPoints[1].y + dist * sin (heading);
					
				TrackPoints[2].x = dx;
				TrackPoints[2].y = dy;

				// TrackPoint[3] add another 90° heading
				if (boxside)
				{
					heading -= PI/2.0f;
					if (heading<0.0f)
						heading += PI * 2.0f;
				}
				else
				{
					heading += PI/2.0f;
					if (heading > PI * 2.0F)
						heading -= PI * 2.0F;
				}

//				TrackPoints[2].z = heading;

				dist = shortleg * NM_TO_FT;

				dx = TrackPoints[2].x + dist * cos (heading);
				dy = TrackPoints[2].y + dist * sin (heading);
					
				TrackPoints[3].x = dx;
				TrackPoints[3].y = dy;
/*
				if (boxside)
				{
					heading -= PI/2.0f;
					if (heading<0.0f)
						heading += PI * 2.0f;
				}
				else
				{
					heading += PI/2.0f;
					if (heading > PI * 2.0F)
						heading -= PI * 2.0F;
				}

				TrackPoints[3].z = heading;*/
/* MN that was a BS idea... now we do if we are too close to FLOT, continue Trackpoints 0-1-2-3, if we were too far, do TP 0-3-2-1
				// check our angle from Trackpoint 0 to Trackpoint 1 and 3 - which ever is less, head towards it
				float x,y,fx,fy,r,a1,a2;
				fx = TrackPoints[0].y;
				fy = TrackPoints[0].x;
				x = self->YPos();
				y = self->XPos();

				if (farthercloser) 	// angle to first trackpoint
					r = (float)atan2((float)(fx-x),(float)(fy-y));
				else 
					r = self->Yaw(); // current heading

				if (r < 0.0f)
					r += PI * 2.0f;

				a1 = r - TrackPoints[0].z;
				if (a1 < 0.0f)
					a1 += PI * 2.0f;

				a2 = r - (TrackPoints[3].z + PI); // +PI as we want the angle from TP0 to TP3 and not TP3 to TP0...
				if (a2 < 0.0f)
					a2 += PI * 2.0f;
				if (a2 > PI * 2.0f)
					a2 -= PI * 2.0f;

				if (a2 < a1)
					advancedirection = true; // go from 0->3->2->1
*/
				// finally head towards our first trackpoint
				TurnToTrackPoint(currentTP);

			}
		}
		else
		{
			// Added by M.N.
			if (directionsetup)	// at tankercall, do an initial turn away from the FLOT, only reset by "RequestFuel" message
			{
				directionsetup = 0;
				float heading = DirectionToFront(SimToGrid(self->YPos()),SimToGrid(self->XPos()));
				if (heading<0.0f)
					heading += PI * 2;
				heading += PI;	// turn 180° away from the FLOT
				if (heading > PI * 2.0F)
						heading -= PI * 2.0F;
				TurnTo(heading);
			}
		}
		FollowThirsty();
#ifdef DAVE_DBG	  
		if(MoveBoom && type != TNKR_IL78)
		{
			boomAzTest = max ( min (boomAzTest,  23.0F * DTR), -23.0F * DTR);
			boomElTest = max ( min (boomElTest, 4.0F * DTR), -40.0F * DTR);
			boomExtTest = max ( min (boomExtTest, 21.0F), 0.0F); // PJW... was 21 & 6

			boom[0].drawPointer->SetDOFangle(BOOM_AZIMUTH, -boomAzTest);
			boom[0].drawPointer->SetDOFangle(BOOM_ELEVATION, -boomElTest);
			boom[0].drawPointer->SetDOFoffset(BOOM_EXTENSION, boomExtTest);
		}
#endif
	}
}

void TankerBrain::BoomWorldPosition(Tpoint *pos)
{
	Trotation *orientation = &((DrawableBSP*)self->drawPointer)->orientation;

	if(type != TNKR_IL78)
	{
		pos->x = orientation->M11*boom[0].rx + orientation->M12*boom[0].ry + orientation->M13*boom[0].rz + self->XPos();
		pos->y = orientation->M21*boom[0].rx + orientation->M22*boom[0].ry + orientation->M23*boom[0].rz + self->YPos();
		pos->z = orientation->M31*boom[0].rx + orientation->M32*boom[0].ry + orientation->M33*boom[0].rz + self->ZPos();
	}
	else
	{
		pos->x = orientation->M11*boom[IL78BOOM].rx + orientation->M12*boom[IL78BOOM].ry + orientation->M13*boom[IL78BOOM].rz + self->XPos();
		pos->y = orientation->M21*boom[IL78BOOM].rx + orientation->M22*boom[IL78BOOM].ry + orientation->M23*boom[IL78BOOM].rz + self->YPos();
		pos->z = orientation->M31*boom[IL78BOOM].rx + orientation->M32*boom[IL78BOOM].ry + orientation->M33*boom[IL78BOOM].rz + self->ZPos();
	}
	return;
}

void TankerBrain::ReceptorRelPosition(Tpoint *pos, SimVehicleClass *thirsty)
{
	if(!thirsty->drawPointer)
	{
		pos->x = thirsty->XPos()- self->XPos();
		pos->y = thirsty->YPos()- self->YPos();
		pos->z = thirsty->ZPos()- self->ZPos();
		return;
	}

	Tpoint recWPos;
	Tpoint recThirstyRelPos;

	//will use hardpoint data to get this position later
	recThirstyRelPos.x = 0.0F;
	recThirstyRelPos.y = 0.0F;
	recThirstyRelPos.z = -3.0F;

	MatrixMult( &((DrawableBSP*)thirsty->drawPointer)->orientation, &recThirstyRelPos, &recWPos );
	recWPos.x += thirsty->XPos()- self->XPos();
	recWPos.y += thirsty->YPos()- self->YPos();
	recWPos.z += thirsty->ZPos()- self->ZPos();

	if (thirsty->LastUpdateTime() == vuxGameTime)
	{
		// Correct for different frame rates
		recWPos.x -= thirsty->XDelta()*SimLibMajorFrameTime;
		recWPos.y -= thirsty->YDelta()*SimLibMajorFrameTime;
	}

	MatrixMultTranspose( &((DrawableBSP*)self->drawPointer)->orientation, &recWPos, pos );

	if(type != TNKR_IL78)
	{
		pos->x -= boom[0].rx;
		pos->y -= boom[0].ry;
		pos->z -= boom[0].rz;
	}
	else
	{
		pos->x -= boom[IL78BOOM].rx;
		pos->y -= boom[IL78BOOM].ry;
		pos->z -= boom[IL78BOOM].rz;
	}
}

void TankerBrain::BoomTipPosition(Tpoint *pos)
{
	Trotation *orientation = &((DrawableBSP*)self->drawPointer)->orientation;
	Tpoint boompos;

	if(type != TNKR_IL78)
	{
		mlTrig TrigAz, TrigEl;

		mlSinCos(&TrigAz, boom[0].az);
		mlSinCos(&TrigEl, boom[0].el);

		boompos.x = -(33.5F + boom[0].ext)*TrigAz.cos*TrigEl.cos;
		boompos.y = -(33.5F + boom[0].ext)*TrigAz.sin*TrigEl.cos;
		boompos.z = -(33.5F + boom[0].ext)*TrigEl.sin;

		pos->x = orientation->M11*(boom[0].rx + boompos.x) + orientation->M12*(boom[0].ry + boompos.y) + orientation->M13*(boom[0].rz + boompos.z) + self->XPos();
		pos->y = orientation->M21*(boom[0].rx + boompos.x) + orientation->M22*(boom[0].ry + boompos.y) + orientation->M23*(boom[0].rz + boompos.z) + self->YPos();
		pos->z = orientation->M31*(boom[0].rx + boompos.x) + orientation->M32*(boom[0].ry + boompos.y) + orientation->M33*(boom[0].rz + boompos.z) + self->ZPos();
	}
	else
	{
		boompos.x = -boom[IL78BOOM].ext;
		Tpoint minB, maxB;
		if (curThirsty && curThirsty->drawPointer) {
		    ((DrawableBSP*)curThirsty->drawPointer)->GetBoundingBox(&minB, &maxB);
		    boompos.x -= maxB.x;
		}
		boompos.y = 0.0F;
		boompos.z = 0.0F;

		pos->x = orientation->M11*(boom[IL78BOOM].rx + boompos.x) + orientation->M12*(boom[IL78BOOM].ry + boompos.y) + orientation->M13*(boom[IL78BOOM].rz + boompos.z) + self->XPos();
		pos->y = orientation->M21*(boom[IL78BOOM].rx + boompos.x) + orientation->M22*(boom[IL78BOOM].ry + boompos.y) + orientation->M23*(boom[IL78BOOM].rz + boompos.z) + self->YPos();
		pos->z = orientation->M31*(boom[IL78BOOM].rx + boompos.x) + orientation->M32*(boom[IL78BOOM].ry + boompos.y) + orientation->M33*(boom[IL78BOOM].rz + boompos.z) + self->ZPos();
	}

	return;
}

void TankerBrain::OptTankingPosition(Tpoint *pos)
{
	Tpoint boompos;
	boompos.x = boompos.y = boompos.z = 0;

	if(!self->drawPointer)
	{
		pos->x = self->XPos();
		pos->y = self->YPos();
		pos->z = self->ZPos();
		return;
	}
	Trotation *orientation = &((DrawableBSP*)self->drawPointer)->orientation;
	if(type != TNKR_IL78)
	{
	    if (curThirsty && curThirsty->IsAirplane() && ((AircraftClass*)curThirsty)->af) {
		((AircraftClass*)curThirsty)->af->GetRefuelPosition(&boompos);
	    }
	    // if nothing set, use F-16 default
	    if (boompos.x == 0 && boompos.y ==0 && boompos.z == 0) {
		boompos.x = -39.63939795321F;
		boompos.y = 0.0F;
		boompos.z = 25.2530815923F;
	    }

		pos->x = orientation->M11*(boom[0].rx + boompos.x) + orientation->M12*(boom[0].ry + boompos.y) + orientation->M13*(boom[0].rz + boompos.z) + self->XPos();
		pos->y = orientation->M21*(boom[0].rx + boompos.x) + orientation->M22*(boom[0].ry + boompos.y) + orientation->M23*(boom[0].rz + boompos.z) + self->YPos();
		pos->z = orientation->M31*(boom[0].rx + boompos.x) + orientation->M32*(boom[0].ry + boompos.y) + orientation->M33*(boom[0].rz + boompos.z) + self->ZPos();

		if (self->LastUpdateTime() == vuxGameTime)
		{
			// Correct for different frame rates
			pos->x -= self->XDelta()*SimLibMajorFrameTime;
			pos->y -= self->YDelta()*SimLibMajorFrameTime;
		}
	}
	else
	{
	    if (curThirsty && curThirsty->IsAirplane()) {
		((AircraftClass*)curThirsty)->af->GetRefuelPosition(&boompos);
	    }
	    else {
	        Tpoint minB, maxB;
		if (curThirsty && curThirsty->drawPointer) {
		    ((DrawableBSP*)curThirsty->drawPointer)->GetBoundingBox(&minB, &maxB);
		    boompos.x -= maxB.x;
		}
	    }
	    boompos.x = - IL78EXT;
	    
	    pos->x = orientation->M11*(boom[IL78BOOM].rx + boompos.x) + orientation->M12*(boom[IL78BOOM].ry + boompos.y) + orientation->M13*(boom[IL78BOOM].rz + boompos.z) + self->XPos();
	    pos->y = orientation->M21*(boom[IL78BOOM].rx + boompos.x) + orientation->M22*(boom[IL78BOOM].ry + boompos.y) + orientation->M23*(boom[IL78BOOM].rz + boompos.z) + self->YPos();
	    pos->z = orientation->M31*(boom[IL78BOOM].rx + boompos.x) + orientation->M32*(boom[IL78BOOM].ry + boompos.y) + orientation->M33*(boom[IL78BOOM].rz + boompos.z) + self->ZPos();
	    
	    if (self->LastUpdateTime() == vuxGameTime)
	    {
		// Correct for different frame rates
		pos->x -= self->XDelta()*SimLibMajorFrameTime;
		pos->y -= self->YDelta()*SimLibMajorFrameTime;
	    }
	}
	return;
}