#include "stdhdr.h"
#include "simbrain.h"
#include "object.h"

BaseBrain::BaseBrain (void)
{
	targetPtr = NULL;
	targetData = NULL;
	flags = 0;
   isWing = 0;
   skillLevel = 0;
}

BaseBrain::~BaseBrain (void)
{
}

void BaseBrain::SetTarget (SimObjectType* newTarget)
{
	if (newTarget == targetPtr)
		return;

	ClearTarget();
	if (newTarget)
		{
        ShiAssert( newTarget->BaseData() != (FalconEntity*)0xDDDDDDDD );
		newTarget->Reference( SIM_OBJ_REF_ARGS );
		targetData = newTarget->localData;
		}
	targetPtr = newTarget;
}

void BaseBrain::ClearTarget (void)
{
	if (targetPtr)
		targetPtr->Release( SIM_OBJ_REF_ARGS );
	targetPtr = NULL;
    targetData = NULL;
}

FeatureBrain::FeatureBrain (void)
{
}

FeatureBrain::~FeatureBrain (void)
{
}

