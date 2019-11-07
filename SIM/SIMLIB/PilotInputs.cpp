#include "stdhdr.h"
#include "PilotInputs.h"
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
extern float pitchElevatorTrimRate;
extern float pitchAileronTrimRate;
extern float pitchRudderTrimRate;
extern float pitchManualTrim;	//MI
extern float yawManualTrim;		//MI
extern float rollManualTrim;	//MI
int PickleOverride;
int TriggerOverride;

// Keyboard stuff
extern float g_frollStickOffset;
extern float g_fpitchStickOffset;
extern float g_frudderOffset;

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
   ptrim = 0.0f;
   rtrim = 0.0f;
   ytrim = 0.0f;
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
//         pitchStickOffset *= 0.9F;
		pitchStickOffset *= g_fpitchStickOffset;
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
//         rollStickOffset *= 0.9F;
			rollStickOffset *= g_frollStickOffset;
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
 //        rudderOffset *= 0.9F;
			rudderOffset *= g_frudderOffset;
   }
   rudder = max ( min (rudder, 1.0F), -1.0F);

   ptrim += pitchElevatorTrimRate * SimLibMajorFrameTime;
   ptrim += pitchManualTrim * SimLibMajorFrameTime;	//MI
   pitchManualTrim = 0.0F;	//MI
   ptrim = max( min(ptrim, 0.5f), -0.5f);
   
   rtrim += pitchAileronTrimRate * SimLibMajorFrameTime;
   rtrim += rollManualTrim * SimLibMajorFrameTime;	//MI
   rollManualTrim = 0.0F;	//MI
   rtrim = max( min(rtrim, 0.5f), -0.5f);

   ytrim += pitchRudderTrimRate * SimLibMajorFrameTime;
   ytrim += yawManualTrim * SimLibMajorFrameTime;	//MI
   yawManualTrim = 0.0F;	//MI
   ytrim = max( min(ytrim, 0.5f), -0.5f);

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
	pitchElevatorTrimRate = 0.0f;
	pitchAileronTrimRate = 0.0f;
	pitchRudderTrimRate = 0.0f;
	keyboardPickleOverride = 0;
	keyboardTriggerOverride = 0;
	ptrim = ytrim = rtrim = 0.0f;
}