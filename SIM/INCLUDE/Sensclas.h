#ifndef _SENSOR_CLASS_H
#define _SENSOR_CLASS_H

#include "mfd.h"

class SimObjectType;
class SimMoverClass;

class SensorClass : public MfdDrawable
{
  public:
	SensorClass(SimMoverClass* newPlatform);
	virtual ~SensorClass(void);

  public:
	enum SensorType {IRST, Radar, RWR, Visual, HTS, TargetingPod, RadarHoming, NumSensorTypes};
	enum TrackTypes {NoTrack, Detection, UnreliableTrack, SensorTrack};
	enum SensorData {NoInfo, LooseBearing, RangeAndBearing, ExactPosition, PositionAndOrientation};
	typedef struct
	{
		float az, el, ata, droll;
		float azFrom, elFrom, ataFrom;
		float x, y, z;
		float range, vt;
	} SensorUpdateType;

	SensorData		dataProvided;
	SimMoverClass*	platform;

  public:
	SensorType	Type(void)			{ return sensorType; };

	virtual void	SetPower (BOOL state)	{ isOn = state; if (!isOn) ClearSensorTarget(); };
	virtual BOOL			IsOn(void)				{ return isOn; };

	virtual void SetDesiredTarget( SimObjectType* newTarget );		// AI Uses this to command a lock
	virtual void ClearSensorTarget ( void );						// Drop our target lock (if any)
	SimObjectType* CurrentTarget(void)	{ return lockedTarget; };

	virtual SimObjectType* Exec (SimObjectType* curTargetList) = 0;
	virtual void ExecModes(int, int)						{};
	virtual void UpdateState(int, int)						{};
	virtual void SetSeekerPos (float newAz, float newEl)	{ seekerAzCenter = newAz; seekerElCenter = newEl; };
	virtual VU_ID TargetUnderCursor (void)		{ return targetUnderCursor; };	// ID of the thing under the cursor, or FalconNullId

	VirtualDisplay*	GetDisplay (void)						{ return privateDisplay; };
	float			SeekerAz (void)							{ return seekerAzCenter; };
	float			SeekerEl (void)							{ return seekerElCenter; };
	FalconEntity	*RemoteBuggedTarget;//me123
  protected:
	virtual void SetSensorTarget( SimObjectType* newTarget );		// Used for things in the target list
	virtual void SetSensorTargetHack( FalconEntity* newTarget );	// Creates a "dummy" SimObjectType whose rel geom isn't updated
	virtual void CheckLockedTarget( void );							// Handle sim/camp handoff and target death

	SimObjectType	*FindNextDetected(SimObjectType*);
	SimObjectType	*FindPrevDetected(SimObjectType*);
	VU_ID					targetUnderCursor;		// ID of the target under the player's cursors
	SimObjectType*		lasttargetUnderCursor;		// target under the player's cursors last scan

  protected:
	// Targeting
	SimObjectType	*lockedTarget;			// Current "primary" target -- who we're locked onto
	int				isOn;					// Is this sensor turned on?

	// Sensor state
	float		seekerAzCenter, seekerElCenter,oldseekerElCenter;
	SensorType	sensorType;
};

SensorClass* FindSensor (SimMoverClass* theObject, int sensorType);

#endif

