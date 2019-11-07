#include "stdhdr.h"
#include "inputs.h"
#include "simio.h"
#include "simmath.h"

PilotInputs UserStickInputs;
extern int	UseKeyboardThrottle;
extern float throttleOffset;
extern float rudderOffset;
extern float rudderOffsetRate;
extern float pitchStickOffset;
extern float rollStickOffset;
extern float pitchStickOffsetRate;
extern float rollStickOffsetRate;
extern float throttleOffsetRate;
extern int keyboardPickleOverride;
extern int keyboardTriggerOverride;
int PickleOverride;
int TriggerOverride;

PilotInputs::PilotInputs (void)
{
   pickleButton = Off;
   pitchOverride = Off;
   missileStep = Off;
   speedBrakeCmd = Center;
   missileOverride = Center;
   trigger = Center;
   tmsPos = Center;
   trimPos = Center;
   dmsPos = Center;
   cmmsPos = Center;
   cursorControl = Center;
   micPos = Center;
   manRange = 0.0F;
   antennaEl = 0.0F;
   pstick = 0.0F;
   rstick = 0.0F;
   throttle = 0.0F;
   rudder = 0.0F;
}

PilotInputs::~PilotInputs (void)
{
}

void PilotInputs::Update (void)
{
   if (IO.AnalogIsUsed(0))
   {
      pstick = Math.DeadBand(IO.ReadAnalog(1), -0.05F, 0.05F) * 1.05F;
   }
   else
   {
      pitchStickOffset += pitchStickOffsetRate * SimLibMajorFrameTime;
	  pitchStickOffset = max ( min (pitchStickOffset, 1.0F), -1.0F);
      pstick = pitchStickOffset;
      if (pitchStickOffsetRate == 0.0F)
         pitchStickOffset *= 0.9F;
   }
   pstick = max ( min (pstick, 1.0F), -1.0F);

   if (IO.AnalogIsUsed(1))
   {
      rstick = Math.DeadBand(IO.ReadAnalog(0), -0.05F, 0.05F) * 1.05F;
   }
   else
   {
      rollStickOffset += rollStickOffsetRate * SimLibMajorFrameTime;
	  rollStickOffset = max ( min (rollStickOffset, 1.0F), -1.0F);
      rstick = rollStickOffset;
      if (rollStickOffsetRate == 0.0F)
         rollStickOffset *= 0.9F;
   }
   rstick = max ( min (rstick, 1.0F), -1.0F);


   if (IO.AnalogIsUsed (2) && !UseKeyboardThrottle)
   {
     //throttle = 1.5F - (IO.ReadAnalog(2) * 1.05F + 1.0F) * 0.75F;
	 throttle = IO.ReadAnalog(2);
   }
   else
   {
      throttleOffset += throttleOffsetRate;	  
	  throttleOffset = max ( min (throttleOffset, 1.5F), 0.0F);
	  throttle = throttleOffset;
   }

   throttle = max ( min (throttle, 1.5F), 0.0F);

   if (IO.AnalogIsUsed (3))
      rudder = Math.DeadBand(-IO.ReadAnalog(3), -0.03F, 0.03F) * 1.05F;
	else
   {
      rudderOffset += rudderOffsetRate * SimLibMajorFrameTime;
	  rudderOffset = max ( min (rudderOffset, 1.0F), -1.0F);
      rudder = rudderOffset;
      if (rudderOffsetRate == 0.0F)
         rudderOffset *= 0.9F;
   }
   rudder = max ( min (rudder, 1.0F), -1.0F);
   //all joystick button functionality is now in sijoy
   //if (IO.ReadDigital(0) || keyboardTriggerOverride)
   if (keyboardTriggerOverride || TriggerOverride)
      trigger = Down;
   else
      trigger = Center;

   //if (IO.ReadDigital(1) || keyboardPickleOverride)
   if (keyboardPickleOverride || PickleOverride)
      pickleButton = On;
   else
      pickleButton = Off;
}


void PilotInputs::Reset(void)
{
	throttleOffset = 0.0F;
	rudderOffset = 0.0F;
	rudderOffsetRate = 0.0F;
	pitchStickOffset = 0.0F;
	rollStickOffset = 0.0F;
	pitchStickOffsetRate = 0.0F;
	rollStickOffsetRate = 0.0F;
	throttleOffsetRate = 0.0F;
	keyboardPickleOverride = 0;
	keyboardTriggerOverride = 0;
}