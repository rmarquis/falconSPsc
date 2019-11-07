#ifndef _ADVHARMPOD_H
#define _ADVHARMPOD_H

#include "HarmPod.h"


class AdvancedHarmTargetingPod : public HarmTargetingPod
{
  public:
	AdvancedHarmTargetingPod(int idx, SimMoverClass* newPlatform);
	virtual ~AdvancedHarmTargetingPod(void);
	
	virtual void			Display (VirtualDisplay *newDisplay);
};

#endif // _ADVHARMPOD_H
