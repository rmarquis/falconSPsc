#include "stdhdr.h"
#include "sensclas.h"
#include "object.h"
#include "geometry.h"
#include "handoff.h"
#include "Graphics\Include\display.h"
#include "simmover.h"

SensorClass::SensorClass(SimMoverClass* self)
{
   platform = self;
   lockedTarget = NULL;
   seekerAzCenter = 0.0f;
   seekerElCenter = 0.0f;
   isOn = TRUE;
	 RemoteBuggedTarget = NULL;
}


SensorClass::~SensorClass(void)
{
	ClearSensorTarget();
}


void SensorClass::SetDesiredTarget( SimObjectType* newTarget )
{
	SetSensorTarget( newTarget );
}


void SensorClass::SetSensorTarget( SimObjectType* newTarget )
{
	if (newTarget == lockedTarget)
		return;
	
	// Drop our previously locked target
	if (lockedTarget) 
	{
		lockedTarget->Release( SIM_OBJ_REF_ARGS );
	}
	
	// Aquire the new one
	if (newTarget) 
	{
		newTarget->Reference( SIM_OBJ_REF_ARGS );
	}

	lockedTarget = newTarget;
}


void SensorClass::SetSensorTargetHack( FalconEntity* newTarget )
{
	SimObjectType	*tgt;

	// Aquire the new one (up to others to keep relative geometry up to date)
	if (newTarget)
	{
#ifdef DEBUG
		tgt = new SimObjectType (OBJ_TAG, platform, newTarget);
#else
		tgt = new SimObjectType (newTarget);
#endif
	} else {
		tgt = NULL;
	}

	SetSensorTarget( tgt );
}


void SensorClass::ClearSensorTarget ( void )
{
	// Drop our previously locked target
	if (lockedTarget) 
	{
		lockedTarget->Release( SIM_OBJ_REF_ARGS );
		lockedTarget = NULL;
	}
}


/*
** Name: CheckLockedTarget
** Description:
**		If we have a current locked target do some checks on it: Is at
**		a campaign unit?, has it been removed from the bubble?  has it
**		been deagg'd/agg'd?.....  We may have to change the lock back and
**		forth between a camp unit and its components.....
*/
void SensorClass::CheckLockedTarget ( void )
{
	SimObjectType	*newTarget;

	// if no target, nothing to validate
	if (lockedTarget == NULL )
	{
		return;
	}

	// Run the handoff routine
	if (sensorType == HTS || sensorType == RWR) {
		newTarget = SimCampHandoff( lockedTarget, platform->targetList, HANDOFF_RADAR );
	} else {
		newTarget = SimCampHandoff( lockedTarget, platform->targetList, HANDOFF_RANDOM );
	}

	// Stop now if our current target is still fine
	if (newTarget == lockedTarget) {
		return;
	}

	// If we get here, we need to exchange targets
	SetSensorTarget( newTarget );
}


//Helper Function to find a given sensor
SensorClass* FindSensor (SimMoverClass* theObject, int sensorType)
{
	int i;
	SensorClass* retval = NULL;

	ShiAssert (theObject);

	if (theObject)
	{
		for (i=0; theObject && i < theObject->numSensors; i++)
		{
			ShiAssert( theObject->sensorArray[i] );
			if (theObject && (theObject->sensorArray[i]) && (theObject->sensorArray[i]->Type() == sensorType))
			{
				retval = theObject->sensorArray[i];
				break;
			}
		}
	}

	return retval;
}


