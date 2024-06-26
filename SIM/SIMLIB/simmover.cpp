#include "stdhdr.h"
#include "Graphics\Include\drawbsp.h"
#include "Graphics\Include\drawsgmt.h"
#include "classtbl.h"
#include "entity.h"
#include "object.h"
#include "otwdrive.h"
#include "initdata.h"
#include "falcmesg.h"
#include "MsgInc\ControlSurfaceMsg.h"
#include "simvudrv.h"
#include "sensclas.h"
#include "feature.h"
#include "sfx.h"
#include "falcsess.h"
#include "simdrive.h"
#include "campList.h"
#include "mvrdef.h"
#include "Unit.h"
#include "simmover.h"
#include "dofsnswitches.h"

#include "IVibeData.h"
extern IntellivibeData g_intellivibeData;

#ifdef USE_SH_POOLS
MEM_POOL graphicsDOFDataPool;
#else
// OW fucking hardcoded SH crap
HANDLE graphicsDOFDataPool;
#define MemAllocPtr MemAlloc2HeapAlloc
#define MemFreePtr(p) HeapFree(graphicsDOFDataPool, 0, p)

static inline LPVOID MemAlloc2HeapAlloc(HANDLE heap, DWORD dwSize, DWORD dwFlags)
{
	return HeapAlloc(heap, dwFlags, dwSize);
}
#endif

void GraphicsDataPoolInitializeStorage (void)
{
#ifdef USE_SH_POOLS
   graphicsDOFDataPool = MemPoolInit( MEM_POOL_DEFAULT | MEM_POOL_SERIALIZE);
#else
   graphicsDOFDataPool = HeapCreate(NULL, 0, 0);
#endif
}

void GraphicsDataPoolReleaseStorage (void)
{
#ifdef USE_SH_POOLS
   MemPoolFree (graphicsDOFDataPool);
#else
	HeapDestroy(graphicsDOFDataPool);
#endif
}

SimMoverClass::SimMoverClass(FILE* filePtr) : SimBaseClass (filePtr)
{
    DOFData = NULL;
    DOFType = NULL;
    VertexData = NULL;
    switchData = NULL;
    switchChange = NULL;
    InitData();

   fread (&numDofs, sizeof(int), 1, filePtr);
   DOFData = (float*)MemAllocPtr (graphicsDOFDataPool, sizeof (float) * numDofs, 0);
   fread (DOFData, sizeof(float), numDofs, filePtr);

   DOFType = (int*)MemAllocPtr (graphicsDOFDataPool, sizeof (int) * numDofs, 0);
   fread (DOFType, sizeof(float), numDofs, filePtr);

   fread (&numVertices, sizeof(int), 1, filePtr);
   if (numVertices)
   {
      VertexData = (float*)MemAllocPtr (graphicsDOFDataPool, sizeof (float) * numVertices, 0);
      fread (VertexData, sizeof(float), numVertices, filePtr);
   }
   else
   {
      VertexData = NULL;
   }

   fread (&numSwitches, sizeof(int), 1, filePtr);
   switchData = (int *)MemAllocPtr (graphicsDOFDataPool, sizeof (int) * numDofs, 0);
   fread (switchData, sizeof(int), numSwitches, filePtr);
   switchChange = (int *)MemAllocPtr (graphicsDOFDataPool, sizeof (int) * numDofs, 0);
   for (int i=0; i<numSwitches; i++)
      switchChange[i] = TRUE;
   targetPtr = NULL;
   pilotSlot = 255;
   vehicleInUnit = 0;
}

SimMoverClass::SimMoverClass(VU_BYTE** stream) : SimBaseClass (stream)
{
    DOFData = NULL;
    DOFType = NULL;
    VertexData = NULL;
    switchData = NULL;
    switchChange = NULL;
   InitData();

   memcpy (&numDofs, *stream, sizeof (int));
   *stream += sizeof (int);
   if (numDofs)
   {
      DOFData = (float*)MemAllocPtr (graphicsDOFDataPool, sizeof (float) * numDofs, 0);
      memcpy (DOFData, *stream, sizeof (float) * numDofs);
      *stream += sizeof (float) * numDofs;
      DOFType = (int*)MemAllocPtr (graphicsDOFDataPool, sizeof (int) * numDofs, 0);
      memcpy (DOFType, *stream, sizeof (int) * numDofs);
      *stream += sizeof (int) * numDofs;
   }
   else
   {
      DOFData = NULL;
      DOFType = NULL;
   }

   memcpy (&numVertices, *stream, sizeof (int));
   *stream += sizeof (int);
   if (numVertices)
   {
      VertexData = (float*)MemAllocPtr (graphicsDOFDataPool, sizeof (float) * numVertices, 0);
      memcpy (VertexData, *stream, sizeof (float) * numVertices);
      *stream += sizeof (float) * numVertices;
   }
   else
   {
      VertexData = NULL;
   }

   memcpy (&numSwitches, *stream, sizeof (int));
   *stream += sizeof (int);
   if (numSwitches)
   {
      switchData = (int*)MemAllocPtr (graphicsDOFDataPool, sizeof (int) * numSwitches, 0);
      memcpy (switchData, *stream, sizeof (int) * numSwitches);
      *stream += sizeof (int) * numSwitches;
      switchChange = (int*)MemAllocPtr (graphicsDOFDataPool, sizeof (int) * numSwitches, 0);
      for (int i=0; i<numSwitches; i++)
         switchChange[i] = TRUE;
   }
   else
   {
      switchData = NULL;
	  switchChange = NULL;
   }

   // KCK: This shit needs to be sent too!
   memcpy (&vehicleInUnit, *stream, sizeof (uchar));
   *stream += sizeof (uchar);
   memcpy (&pilotSlot, *stream, sizeof (uchar));
   *stream += sizeof (uchar);

   targetPtr = NULL;
}

// JPO - allocate stuff appropriate to complex 3-d aircraft models
void SimMoverClass::MakeComplex()
{
    numDofs = COMP_MAX_DOF;
    numSwitches = COMP_MAX_SWITCH;				   
    numVertices = AIRCRAFT_MAX_DVERTEX;
    AllocateSwitchAndDof();
    for (int i=6; i<9; i++)
    {
	DOFType[i] = TranslateDof;
    }
}

// JPO - allocate stuff appropriate to simple 3-d aircraft models
void SimMoverClass::MakeSimple()
{
    int i;
    numDofs = SIMP_MAX_DOF;
    numSwitches = SIMP_MAX_SWITCH;
    numVertices = AIRCRAFT_MAX_DVERTEX;
    AllocateSwitchAndDof();
    for (i = 2; i < 6 && i < numDofs; i++)
	DOFType[i] = NoDof;
}

// allocate relevant memory
void SimMoverClass::AllocateSwitchAndDof()
{
    int i;
    if ( DOFData )
	MemFreePtr (DOFData);
    if ( DOFType )
	MemFreePtr (DOFType);
    if ( switchData )
	MemFreePtr (switchData);
    if ( switchChange )
	MemFreePtr (switchChange);
    
    if (VertexData)
	MemFreePtr (VertexData); 
    
    if (numVertices)
    {
	VertexData = (float*)MemAllocPtr (graphicsDOFDataPool, sizeof (float) * numVertices, 0);
	for (i=0; i<numVertices; i++)
	{
	    VertexData[i] = 0.0F;
	}
    }
    else
	VertexData = NULL;
    
    if (numDofs){
	DOFData = (float*)MemAllocPtr (graphicsDOFDataPool, sizeof (float) * numDofs, 0);
	DOFType = (int*)MemAllocPtr (graphicsDOFDataPool, sizeof (float) * numDofs, 0);
	for (i=0; i<numDofs; i++)
	{
	    DOFData[i] = 0.0F;
	    DOFType[i] = AngleDof;
	}
    }
    else {
	DOFData = NULL;
	DOFType = NULL;
    }
    
    if (numSwitches){
	switchData = (int*)MemAllocPtr (graphicsDOFDataPool, sizeof (int) * numSwitches, 0);
	switchChange = (int*)MemAllocPtr (graphicsDOFDataPool, sizeof (int) * numSwitches, 0);
	
	for (i=0; i<numSwitches; i++)
	{
	    switchChange[i] = TRUE;
	    switchData[i] = 0;
	}
    }
    else {
	switchData = NULL;
	switchChange = NULL;
    }
}

SimMoverClass::SimMoverClass(int type) : SimBaseClass (type)
{
    DOFData = NULL;
    DOFType = NULL;
    VertexData = NULL;
    switchData = NULL;
    switchChange = NULL;
    InitData();
    
    //LRKLUDGE
   /* edg: try and prevent memory trash crash
   if (EntityType()->classInfo_[VU_STYPE] == STYPE_AIR_FIGHTER_BOMBER &&
       EntityType()->classInfo_[VU_SPTYPE] == SPTYPE_F16C)
   {
      numDofs = 25;
      numSwitches = 13;
      numVertices = 6;
   }
   else if (EntityType()->classInfo_[VU_TYPE] == TYPE_AIRPLANE)
   {
      numDofs = 20;
      numSwitches = 12;
      numVertices = 0;
   }
   else
   {
      numDofs = 5;
      numSwitches = 5;
      numVertices = 0;
   }
   

   if (EntityType()->classInfo_[VU_TYPE] == TYPE_AIRPLANE)
   {
      numDofs = 28;
      numSwitches = 18;
      numVertices = 10;
   }
   else
   {
      numDofs = 10;
      numSwitches = 10;
      numVertices = 10;
   }
   */
   numDofs = 0;
   numSwitches = 0;
   numVertices = 0;

   if (GetDomain() == DOMAIN_AIR )
   {
   	   if (GetClass() == CLASS_VEHICLE )
	   {
	       if (GetType() == TYPE_AIRPLANE)
	       {
		   if(GetSType() == STYPE_AIR_FIGHTER_BOMBER && GetSPType() == SPTYPE_F16C)
		   {
		       MakeComplex();
		   }
		   else
		   {
		       MakeSimple();
		   }
	       }
	       else if (GetType() == TYPE_HELICOPTER)
	       {
		   numDofs = HELI_MAX_DOF;
		   numSwitches = HELI_MAX_SWITCH;
		   numVertices = HELI_MAX_DVERTEX;
		   AllocateSwitchAndDof();
	       }
	   }
   }
   else if (GetClass() == CLASS_VEHICLE )
   {
       numDofs = AIRDEF_MAX_DOF;
       numSwitches = AIRDEF_MAX_SWITCH;
       numVertices = VECH_MAX_DVERTEX;
       AllocateSwitchAndDof();
   }
   pilotSlot = 255;
   vehicleInUnit = 0;
   targetPtr = NULL;
}



void SimMoverClass::InitData (void)
{
 /*   DOFData = NULL;
    DOFType = NULL;			M.N. these initialisations don't belong here
    VertexData = NULL;		Dogfight calls this when recreating a shot down aircraft
    switchData = NULL;	    switchData crashes first
    switchChange = NULL;*/
    targetList = NULL;
    numSensors = 0;
    sensorArray = NULL;
    requestCount = 0;
    dataRequested = 0;
    nonLocalData = NULL;
    pilotSlot = 255;
    vehicleInUnit = 0;
    onFlatFeature = FALSE;
    vt = 0.0f;
    kias = 0.0f;
}

void SimMoverClass::Init (SimInitDataClass* initData)
{
	Falcon4EntityClassType* classPtr;
	SimVuDriver *oldd;

	if (initData)
	{
		SetCampaignObject(initData->campUnit);
		// pilotSlot is only different from vehicleInUnit in the
		// case of player owned aircraft
		pilotSlot = vehicleInUnit = static_cast<uchar>(initData->vehicleInUnit);
	}
	
	SimBaseClass::Init(initData);
	
	// Init mover data
	classPtr = (Falcon4EntityClassType*)EntityType();
	// 2002-03-02 ADDED BY S.G. If not enough vehicles in vehicle.lst compared to the value in vehicleDataIndex, it will overflow the array
	if (classPtr->vehicleDataIndex >= NumSimMoverDefinitions)
		mvrDefinition = NULL;
	else
	// END OF ADDED SECTION
		mvrDefinition = moverDefinitionData[classPtr->vehicleDataIndex];
	ShiAssert(mvrDefinition);

	// SetDelta(0.0F, 0.0F, 0.0F);
	// SetYPRDelta(0.0F, 0.0F, 0.0F);
	if (IsLocal())
	{
		oldd = (SimVuDriver *)SetDriver(new SimVuDriver(this));
	}
	else
	{
		oldd = (SimVuDriver *)SetDriver(new SimVuSlave(this));
	}
	if ( oldd )
		delete oldd;
}

SimMoverClass::~SimMoverClass(void)
{
   ShiAssert(!IsAwake());

   if ( DOFData )
   		MemFreePtr (DOFData);
   if ( DOFType )
   		MemFreePtr (DOFType);
   if ( switchData )
   		MemFreePtr (switchData);
   if ( switchChange )
   		MemFreePtr (switchChange);

   if (VertexData)
      MemFreePtr (VertexData);

   SimMoverClass::Cleanup();
}

void SimMoverClass::Cleanup()
{
	if (numSensors > 0 && (!sensorArray || F4IsBadReadPtr(sensorArray, sizeof(SensorClass*)))) // JB 010223 CTD
		return; // JB 010223 CTD

   for (int i = 0; i<numSensors; i++)
   {
		if (F4IsBadWritePtr(sensorArray[i], sizeof(SensorClass))) // JB 010223 CTD
			continue; // JB 010223 CTD

	  sensorArray[i]->SetPower( FALSE );
      delete (sensorArray[i]);
	  sensorArray[i] = NULL;
	  numSensors = 0; // 2002-02-01 ADDED BY S.G. Say we don't have any sensors
   }
   delete [] sensorArray;
   sensorArray = NULL;
   SimVuDriver *oldd = (SimVuDriver *)SetDriver(NULL);
   if ( oldd )
      delete oldd;

   // tear down nonlocal data
   if ( nonLocalData )
   {
	   // check for active smoketrail and send it to sfx for
	   // later removal
	   if ( nonLocalData->smokeTrail )
	   {
	      OTWDriver.AddSfxRequest(
			 new SfxClass ( 2.0f,		// time to live
			 nonLocalData->smokeTrail ) );

	      nonLocalData->smokeTrail = NULL;
	   }

	   delete nonLocalData;
	   nonLocalData = NULL;
   }
}

int SimMoverClass::Wake (void)
{
	int
		i;

	SimBaseClass::Wake();

	if (EntityDriver())
	{
		EntityDriver()->ResetLastUpdateTime (vuxGameTime);
	}
	
	// SetDelta(0.0F, 0.0F, 0.0F);
	// SetYPRDelta(0.0F, 0.0F, 0.0F);

	if (drawPointer && !IsExploding())
	{
		for (i=0; i<numSwitches; i++)
		{
			if (switchChange[i])
			{
				((DrawableBSP*)drawPointer)->SetSwitchMask(i, switchData[i]);
				switchChange[i] = FALSE;
			}
		}
		for (i=0; i<numDofs; i++)
		{
			if (DOFType[i] == AngleDof)
				((DrawableBSP*)drawPointer)->SetDOFangle(i, DOFData[i]);
			else if (DOFType[i] == TranslateDof)
				((DrawableBSP*)drawPointer)->SetDOFoffset(i, DOFData[i]);
		}
		
		for (i=0; i<numVertices; i++)
		{
			((DrawableBSP*)drawPointer)->SetDynamicVertex(i, VertexData[i], 0.0F, 0.0F);
		}
	}
	
	SimDriver.AddToObjectList(this);

	return 0;
}

int SimMoverClass::Sleep (void)
{
	int numRef,i;

	SimObjectType* tmpObject;
	
	SimDriver.RemoveFromObjectList(this);
	
	SimBaseClass::Sleep();
	
	while (targetList)
	{
		tmpObject = targetList;
		targetList = targetList->next;
		tmpObject->prev = NULL;
		tmpObject->next = NULL;

		// use for debugging
		numRef = tmpObject->IsReferenced();

		tmpObject->Release( SIM_OBJ_REF_ARGS );
		tmpObject = NULL;
	}
	ClearTarget();
	
	for ( i=0; i<numSensors; i++)
	{
		if (sensorArray[i])
			sensorArray[i]->SetPower( FALSE );
	}

	// SetDelta(0.0F, 0.0F, 0.0F);
	// SetYPRDelta(0.0F, 0.0F, 0.0F);
	return 0;
}

void SimMoverClass::MakeLocal (void)
{
	// LEON TODO: Need to do all necessary shit to convert from a remote to a local entity.
	SimBaseClass::MakeLocal();
	
	// Clear all nonLocal data
	if (nonLocalData)
	{
		// check for active smoketrail and send it to sfx for later removal
		if (nonLocalData->smokeTrail)
		{
			OTWDriver.AddSfxRequest(new SfxClass ( 2.0f, nonLocalData->smokeTrail ) );
			nonLocalData->smokeTrail = NULL;
		}
		delete nonLocalData;
		nonLocalData = NULL;
	}
	
	// The local (Master) driver.
	// SetDelta(0.0F, 0.0F, 0.0F);
	// SetYPRDelta(0.0F, 0.0F, 0.0F);

	SimVuDriver *drive = new SimVuDriver(this);
	drive->ExecDR(vuxGameTime);
	SimVuDriver *oldd = (SimVuDriver *)SetDriver (drive);
	if ( oldd )
		delete oldd;
}

void SimMoverClass::MakeRemote (void)
{
	// LEON TODO: Need to do all necessary shit to convert from a remote to a local entity.
	SimBaseClass::MakeRemote();
	
	// Allocate and init nonLocal Data
	nonLocalData = new SimBaseNonLocalData;
	nonLocalData->flags = 0;
	nonLocalData->smokeTrail = NULL;
	nonLocalData->timer3 = (float)SimLibElapsedTime / SEC_TO_MSEC;
	
	// The remote (slave) driver
	// SetDelta(0.0F, 0.0F, 0.0F);
	// SetYPRDelta(0.0F, 0.0F, 0.0F);

	SimVuDriver *oldd = (SimVuDriver *)SetDriver(new SimVuSlave(this));
	if ( oldd )
		delete oldd;
}

int SimMoverClass::Exec (void)
{
	//FalconControlSurfaceMsg* newControlData;
	int i;
	
	// JB 011231 Set the last move so the campaign honors the movement of the unit while deaggregated.  
	// Without setting this, when the unit reaggregates, the unit will replay its mission from the time that
	// it deaggregated so that movement not following the waypoints of the unit's mission is lost.
	if (GetCampaignObject())
		((UnitClass*)GetCampaignObject())->SetUnitLastMove(TheCampaign.CurrentTime); 

	if (drawPointer && !IsExploding())
	{
		for (i=0; i<numSwitches; i++)
		{
			if (switchChange[i])
			{
				((DrawableBSP*)drawPointer)->SetSwitchMask(i, switchData[i]);
				switchChange[i] = FALSE;
			}
		}
		for (i=0; i<numDofs; i++)
		{
			if (DOFType[i] == AngleDof)
				((DrawableBSP*)drawPointer)->SetDOFangle(i, DOFData[i]);
			else if (DOFType[i] == TranslateDof)
				((DrawableBSP*)drawPointer)->SetDOFoffset(i, DOFData[i]);
		}
		
		for (i=0; i<numVertices; i++)
		{
			((DrawableBSP*)drawPointer)->SetDynamicVertex(i, VertexData[i], 0.0F, 0.0F);
		}
	}
	
	if (IsLocal()) {
		if ((requestCount > 0 && (SimLibFrameCount & 0x2F) == 0) || ((SimLibFrameCount & 0x1FF) == 0) )
		{
			//newControlData = new FalconControlSurfaceMsg(Id(), FalconLocalGame);
			//newControlData->dataBlock.gameTime = SimLibElapsedTime;
			//newControlData->dataBlock.entityID = Id();
			//newControlData->dataBlock.numDofs = numDofs;
			//newControlData->dataBlock.numSwitches = numSwitches;
			//newControlData->dataBlock.DOFData = DOFData;
			//newControlData->dataBlock.DOFType = DOFType;
			//newControlData->dataBlock.switchData = switchData;
			//newControlData->dataBlock.specialData = SpecialData();
			// FalconSendMessage (newControlData,FALSE);
		}
	}
	else // Isn't local
	{
		// non local execution
		if ( nonLocalData )
		{
			Tpoint pos, vec;
			
			// IsFiring will be set\unset when a message is received from
			// machine controlling the entity locally
			if ( IsFiring() )
			{
				if ( nonLocalData->flags & NONLOCAL_GUNS_FIRING )
				{
					// already firing, is it time to stop or fire another?
					if ( nonLocalData->timer2 <= SimLibElapsedTime )
					{
						// stop firing
						nonLocalData->flags &= ~NONLOCAL_GUNS_FIRING;

						// check for active smoketrail and send it to sfx for
						// later removal
						if ( nonLocalData->smokeTrail )
						{
							OTWDriver.AddSfxRequest(
								new SfxClass ( 2.0f,		// time to live
								nonLocalData->smokeTrail ) );
							
							nonLocalData->smokeTrail = NULL;
						}
					}
					else if ( nonLocalData->timer1 >= SimLibElapsedTime )
					{
						
						// fire another....
						nonLocalData->timer1 = SimLibElapsedTime + 500.0f;
						pos.x = XPos();
						pos.y = YPos();
						pos.z = ZPos();
						vec.x = XDelta() * ((900 + rand()%200)/1000) ;
						vec.y = YDelta() * ((900 + rand()%200)/1000);
						vec.z = ZDelta() * ((900 + rand()%200)/1000);
						OTWDriver.AddTrailHead( nonLocalData->smokeTrail, pos.x, pos.y, pos.z );
						
						// for the moment (at least), bullets only go in direction
						// object is pointing
						// vec.x += dmx[0][0]*3000.0f;
						// vec.y += dmx[0][1]*3000.0f;
						// vec.z += dmx[0][2]*3000.0f;
						vec.x += nonLocalData->dx * ((900 + rand()%200)/1000);
						vec.y += nonLocalData->dy * ((900 + rand()%200)/1000);
						vec.z += nonLocalData->dz * ((900 + rand()%200)/1000);
						pos.x += vec.x*SimLibMajorFrameTime;
						pos.y += vec.y*SimLibMajorFrameTime;
						pos.z += vec.z*SimLibMajorFrameTime;
						
						// add a tracer
						OTWDriver.AddSfxRequest(
							new SfxClass(SFX_GUN_TRACER,				// type
							SFX_MOVES | SFX_USES_GRAVITY,						// flags
							&pos,							// world pos
							&vec,							// vector
							3.0f,							// time to live
							1.0f));							// scale
					}
				}
				else if (!nonLocalData->timer2)
				{
					// we haven't yet started firing....
					// will be set when recieving a fire message nonLocalData->flags |= NONLOCAL_GUNS_FIRING;
					
					// timer1 is the time to fire next tracer
					// timer2 is the maximum time we'll allow firing to continue
					// (we may not receive an end fire message)
					nonLocalData->timer1 = SimLibElapsedTime + 500.0f;
					nonLocalData->timer2 = SimLibElapsedTime + 20000.0f;
					
					// set up the smoke trail
					pos.x = XPos();
					pos.y = YPos();
					pos.z = ZPos();
					vec.x = XDelta();
					vec.y = YDelta();
					vec.z = ZDelta();
					nonLocalData->smokeTrail = new DrawableTrail(TRAIL_GUN);
					OTWDriver.InsertObject( nonLocalData->smokeTrail );
					OTWDriver.AddTrailHead( nonLocalData->smokeTrail, pos.x, pos.y, pos.z );
					
					// for the moment (at least), bullets only go in direction
					// object is pointing
					// vec.x += dmx[0][0]*3000.0f;
					// vec.y += dmx[0][1]*3000.0f;
					// vec.z += dmx[0][2]*3000.0f;
					vec.x += nonLocalData->dx;
					vec.y += nonLocalData->dy;
					vec.z += nonLocalData->dz;
					pos.x += vec.x*SimLibMajorFrameTime;
					pos.y += vec.y*SimLibMajorFrameTime;
					pos.z += vec.z*SimLibMajorFrameTime;
					
					// add a tracer
					OTWDriver.AddSfxRequest(
						new SfxClass(SFX_GUN_TRACER,				// type
						SFX_MOVES | SFX_USES_GRAVITY,						// flags
						&pos,							// world pos
						&vec,							// vector
						3.0f,							// time to live
						1.0f));							// scale
				}
			}
			else // not firing
			{
				if ( nonLocalData->flags & NONLOCAL_GUNS_FIRING )
				{
					// stop firing
					nonLocalData->flags &= ~NONLOCAL_GUNS_FIRING;
					// check for active smoketrail and send it to sfx for
					// later removal
					if ( nonLocalData->smokeTrail )
					{
						OTWDriver.AddSfxRequest(
							new SfxClass ( 2.0f,		// time to live
							nonLocalData->smokeTrail ) );
						
						nonLocalData->smokeTrail = NULL;
					}
				}
			}
		}
	} // end of non-local condition

	return (IsLocal());
}

float SimMoverClass:: GetP (void)
{
   return (0.0F);
}

float SimMoverClass:: GetQ (void)
{
   return (0.0F);
}

float SimMoverClass:: GetR (void)
{
   return (0.0F);
}

float SimMoverClass:: GetAlpha (void)
{
   return (0.0F);
}

float SimMoverClass:: GetBeta (void)
{
   return (0.0F);
}

float SimMoverClass:: GetNx(void)
{
   return (0.0F);
}

float SimMoverClass:: GetNy(void)
{
   return (0.0F);
}

float SimMoverClass:: GetNz(void)
{
   return (0.0F);
}

float SimMoverClass:: GetGamma (void)
{
   return (0.0F);
}

float SimMoverClass::GetSigma (void)
{
   return (0.0F);
}

float SimMoverClass::GetMu (void)
{
   return (0.0F);
}

// function interface
// serialization functions
int SimMoverClass::SaveSize()
{
   return SimBaseClass::SaveSize() +
      numDofs * (sizeof (float) + sizeof (int)) + // DOFData and DofType
      numSwitches * sizeof (int) + // SwitchData
      numVertices * sizeof (float) + // VertexData
      3 * sizeof (int) + // numDofs, numSwitches, numVertices;
	  2 * sizeof (uchar); // pilotSlot and aircraftSlot;
//   return SimBaseClass::SaveSize() + numDofs * (sizeof (float) + sizeof (int)) +
//      (numSwitches + 3) * sizeof (int) + + numVertices * sizeof(float);
}

int SimMoverClass::Save(VU_BYTE **stream)
{
   SimBaseClass::Save(stream);

   memcpy (*stream, &numDofs, sizeof (int));
   *stream += sizeof (int);
   memcpy (*stream, DOFData, sizeof (float) * numDofs);
   *stream += sizeof (float) * numDofs;

   memcpy (*stream, DOFType, sizeof (int) * numDofs);
   *stream += sizeof (int) * numDofs;

   memcpy (*stream, &numVertices, sizeof (int));
   *stream += sizeof (int);
   if (numVertices)
   {
      memcpy (*stream, VertexData, sizeof (float) * numVertices);
      *stream += sizeof (float) * numVertices;
   }

   memcpy (*stream, &numSwitches, sizeof (int));
   *stream += sizeof (int);
   memcpy (*stream, switchData, sizeof (int) * numSwitches);
   *stream += sizeof (int) * numSwitches;

   memcpy (*stream, &vehicleInUnit, sizeof (uchar));
   *stream += sizeof (uchar);
   memcpy (*stream, &pilotSlot, sizeof (uchar));
   *stream += sizeof (uchar);

   return numDofs * sizeof (float) + numSwitches * sizeof (int) + 2 * sizeof (int) + 2 * sizeof (uchar);
}

int SimMoverClass::Save(FILE *file)
{
int retval;

   retval = SimBaseClass::Save (file);

   retval += fwrite (&numDofs, sizeof(int), 1, file);
   retval += fwrite (DOFData, sizeof(float), numDofs, file);
   retval += fwrite (DOFType, sizeof(int), numDofs, file);

   retval += fwrite (&numVertices, sizeof(int), 1, file);
   if (numVertices)
   {
      retval += fwrite (VertexData, sizeof(float), numVertices, file);
   }

   retval += fwrite (&numSwitches, sizeof(int), 1, file);
   retval += fwrite (switchData, sizeof(int), numSwitches, file);

   return (retval);
}

int SimMoverClass::Handle(VuFullUpdateEvent *event)
{
SimMoverClass* tmpMover = (SimMoverClass*)(event->expandedData_);
int i;

   // Copy Switch and DOF data
   memcpy (&numDofs, &tmpMover->numDofs, sizeof (int));
   memcpy (DOFData, tmpMover->DOFData, sizeof (float) * numDofs);
   memcpy (DOFType, tmpMover->DOFType, sizeof (int) * numDofs);

   memcpy (&numSwitches, &tmpMover->numSwitches, sizeof (int));

   for (i=0; i<numSwitches; i++)
   {
      if (switchData[i] != tmpMover->switchData[i])
      {
         switchData[i] = tmpMover->switchData[i];
         switchChange[i] = TRUE;
      }
   }

   return (SimBaseClass::Handle(event));
}

int SimMoverClass::Handle(VuPositionUpdateEvent *event)
{
	UnitClass	*campObj = (UnitClass*) GetCampaignObject();

	if (campObj && campObj->IsLocal() && campObj->GetComponentLead() == this)
		{
		campObj->SimSetLocation(event->x_, event->y_, event->z_);
		if (campObj->IsFlight())
			campObj->SimSetOrientation (event->yaw_, event->pitch_, event->roll_);
		}

	return (SimBaseClass::Handle(event));
}

int SimMoverClass::Handle(VuTransferEvent *event)
{
	return (SimBaseClass::Handle(event));
}

void SimMoverClass::SetDead (int flag)
{
   SimBaseClass::SetDead(flag);
   /*
   ** edg: moved to sleep function
   if (flag)
   {
      while (targetList)
      {
         tmpObject = targetList;
         targetList = targetList->next;
		 tmpObject->prev = NULL;
		 tmpObject->next = NULL;
         tmpObject->Release();
		 tmpObject = NULL;
      }
      targetList = NULL;
      ClearTarget();

      for (int i=0; i<numSensors; i++)
      {
         if (sensorArray[i])
            sensorArray[i]->SetPower( FALSE );
      }
   }
   */
}

VU_ERRCODE SimMoverClass::InsertionCallback(void)
{
	return SimBaseClass::InsertionCallback();
}

VU_ERRCODE SimMoverClass::RemovalCallback(void)
{
	return SimBaseClass::RemovalCallback();
}

void SimMoverClass::SetDOFs (float* newData)
{
   memcpy (DOFData, newData, sizeof(float) * numDofs);
   memcpy (DOFType, newData, sizeof(float) * numDofs);
}

void SimMoverClass::SetSwitches (int* newData)
{
   memcpy (switchData, newData, sizeof(int) * numSwitches);
}

void SimMoverClass::AddDataRequest (int flag)
{
   requestCount += flag;
}

void SimMoverClass::SetTarget (SimObjectType *newTarget)
{
	if (newTarget == targetPtr)
		return;
	
	if (targetPtr)
		targetPtr->Release( SIM_OBJ_REF_ARGS );
	
	if (newTarget)
	{
        ShiAssert( newTarget->BaseData() != (FalconEntity*)0xDDDDDDDD );
		newTarget->Reference( SIM_OBJ_REF_ARGS );
	}
	
	targetPtr = newTarget;
}

void SimMoverClass::ClearTarget (void)
{
	if (targetPtr) {
		targetPtr->Release( SIM_OBJ_REF_ARGS );
		targetPtr = NULL;
	}
}

void SimMoverClass::UpdateLOS(SimObjectType *obj)
{
	float top, bottom;

	// See if the target is near the ground
	OTWDriver.GetAreaFloorAndCeiling (&bottom, &top);
	if(ZPos() < top && obj->BaseData()->ZPos() < top)
		obj->localData->SetTerrainLOS(TRUE);
	else if(OTWDriver.CheckLOS(this, obj->BaseData()))
		obj->localData->SetTerrainLOS(TRUE);
	else
		obj->localData->SetTerrainLOS(FALSE);

	if(OTWDriver.CheckCloudLOS(this, obj->BaseData()))
		obj->localData->SetCloudLOS(TRUE);
	else
		obj->localData->SetCloudLOS(FALSE);

	if(!OnGround())
		obj->localData->nextLOSCheck = SimLibElapsedTime + 200;
	else if(!obj->BaseData()->OnGround())
		obj->localData->nextLOSCheck = SimLibElapsedTime + 1000;
	else
		obj->localData->nextLOSCheck = SimLibElapsedTime + 5000;
}

int SimMoverClass::CheckLOS(SimObjectType *obj)
{
	if(!obj || !obj->BaseData())
		return FALSE;

	if(SimLibElapsedTime > obj->localData->nextLOSCheck)
		UpdateLOS(obj);

	return obj->localData->TerrainLOS();
}

int SimMoverClass::CheckCompositeLOS(SimObjectType *obj)
{
	if(!obj || !obj->BaseData())
		return FALSE;

	if(SimLibElapsedTime > obj->localData->nextLOSCheck)
		UpdateLOS(obj);

	if(obj->localData->CloudLOS())
		return obj->localData->TerrainLOS();
	else
		return FALSE;
}


/*
** Name: FeatureCollision
** Description:
**		Check to see if the we've collided with any features.
**		Gross level check is objective bubble follwed by some
**		sort of more refined test with elements in the objective bubble.
**		Returns pointer of base class feature struck
*/
SimBaseClass *
SimMoverClass::FeatureCollision (float groundZ)
{
CampBaseClass* objective;
#ifdef VU_GRID_TREE_Y_MAJOR
VuGridIterator gridIt(ObjProxList, YPos(), XPos(), 3.0F * NM_TO_FT);
#else
VuGridIterator gridIt(ObjProxList, XPos(), YPos(), 3.0F * NM_TO_FT);
#endif
SimBaseClass *foundFeature = NULL;
SimBaseClass *testFeature;
float radius;
Tpoint pos, fpos, vec, p3, collide;
BOOL firstFeature;
WeaponClassDataType* wc = NULL;
const float deltat = 1.0f; // JPO time to look ahead for flat surfaces - arrived at by experiment.
// this required to land on highway strips - that are in forest or similar.

if ( IsWeapon() )
{
	wc = (WeaponClassDataType*)Falcon4ClassTable[Type() - VU_LAST_ENTITY_TYPE].dataPtr;
}

	onFlatFeature = FALSE;

	// get the 1st objective that contains the bomb
	objective = (CampBaseClass*)gridIt.GetFirst();
	while ( objective )
	{
      if (objective->GetComponents())
      {
		   pos.x = XPos();
		   pos.y = YPos();
		   pos.z = ZPos();
	   
		   // check out some time in the future (was without multiplier JPO)
		   vec.x = XDelta() * SimLibMajorFrameTime * deltat;
		   vec.y = YDelta() * SimLibMajorFrameTime * deltat;
		   vec.z = ZDelta() * SimLibMajorFrameTime * deltat;
	   
		   p3.x = (float)fabs( vec.x );
		   p3.y = (float)fabs( vec.y );
		   p3.z = (float)fabs( vec.z );
	   
			// loop thru each element in the objective
			VuListIterator	featureWalker(objective->GetComponents());
			testFeature = (SimBaseClass*) featureWalker.GetFirst();
			firstFeature = TRUE;
			while (testFeature)
         {
			   if (testFeature->drawPointer)
            {
			      // get feature's radius and position
			      radius = testFeature->drawPointer->Radius();
				  if(drawPointer)
					  radius += drawPointer->Radius();
			      testFeature->drawPointer->GetPosition( &fpos );
	      
			      // test with gross level bounds of object
			      if (fabs (pos.x - fpos.x) < radius + p3.x &&
				      fabs (pos.y - fpos.y) < radius + p3.y &&
				      fabs (pos.z - fpos.z) < radius + p3.z )
			      {
				      // if we're on the ground make sure we have a downward vector if
				      // we're testing a flat container so we detect a collision
				      if ( OnGround())
				      {
						  if(testFeature->IsSetCampaignFlag (FEAT_FLAT_CONTAINER))
						  {
							  vec.z = 1500.0f;
							  pos.z = groundZ - 50.0f;
						  }
						  else
						  {
							  vec.z = 0.0f;
							  pos.z = groundZ - 0.5f;
						  }
				      }
				      else
				      {
					      vec.z = ZDelta() * SimLibMajorFrameTime * deltat;
					      pos.z = ZPos();
				      }
					      
				      if ( testFeature->drawPointer->GetRayHit( &pos, &vec, &collide, 1.0f ) )
				      {
						  if ( IsWeapon() )
						  {
							  FeatureClassDataType* fc = GetFeatureClassData(testFeature->Type() - VU_LAST_ENTITY_TYPE);

							  if(fc->DamageMod[wc->DamageType])
								  return testFeature;

							  foundFeature = testFeature;
						  }
						  else
						  {
							  // if we're a bomb and we've hit a flat thingy, it's OK to
							  // return detect a hit on the feature so return it....
							  // other wise we just note if we're on top of a flat feature
							  if (testFeature->IsSetCampaignFlag (FEAT_FLAT_CONTAINER))
							  {
								  onFlatFeature = TRUE;
							  }
							  else
								{
									if (FalconLocalSession && this == FalconLocalSession->GetPlayerEntity())
										g_intellivibeData.CollisionCounter++;

								  return testFeature;
								}
						  }
				      }
			      }
            }
   			testFeature = (SimBaseClass*) featureWalker.GetNext();
			firstFeature = FALSE;
   		}
      }

      objective = (CampBaseClass*)gridIt.GetNext();
	}

	if (foundFeature && FalconLocalSession && this == FalconLocalSession->GetPlayerEntity())
		g_intellivibeData.CollisionCounter++;

	return foundFeature;
}

void SimMoverClass::SetVt (float new_vt)
{
	vt = new_vt; 
}

void SimMoverClass::SetKias (float new_kias)
{ 
	kias = new_kias; 
}

