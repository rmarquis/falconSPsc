#include "SimDrive.h"
#include "stdhdr.h"
#include "simvudrv.h"
#include "simmover.h"
#include "mesg.h"
#include "airframe.h"
#include "aircrft.h"

extern float get_air_speed (float speed, int altitude);

#define MIN_ZDELTA_FOR_PITCH	20.0F	

#ifdef USE_SH_POOLS
MEM_POOL SimVuDriver::pool;
MEM_POOL SimVuSlave::pool;
#endif

VU_BOOL SimVuDriver::ExecModel(VU_TIME timestamp)
{
	void
		*ptr;

	ptr = (SimBaseClass*) entity_;

	VU_BOOL retval = (VU_BOOL)(((SimBaseClass*)entity_)->Exec());
	if (ptr != entity_)
	{
		MonoPrint ("Entity Ptr %08x %08x\n", ptr, entity_);
	}

	entity_->SetUpdateTime(timestamp);
	return retval;
}

void SimVuSlave::Exec(VU_TIME timestamp)
{
	VuDelaySlave::Exec(timestamp);
	
	// KCK: We need to copy our updated position and delta values into our motion model
	float vt = (float)sqrt(truexdelta_ * truexdelta_ + 
					trueydelta_ * trueydelta_ + 
					truezdelta_ * truezdelta_);

//	if ((int)entity_ == (int)SimDriver.playerEntity)
//	{
//		MonoPrint ("Player is Remote\n");
//	}

	((SimMoverClass*)entity_)->SetVt(vt);
	((SimMoverClass*)entity_)->SetKias(get_air_speed (vt * FTPSEC_TO_KNOTS, -1*FloatToInt32(entity_->ZPos())));
	
	((SimBaseClass*)entity_)->Exec();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE SimVuSlave::Handle(VuPositionUpdateEvent *event)
{
	VU_ERRCODE
		err = VU_SUCCESS;

	if (!((SimBaseClass*)entity_)->IsAwake ())
	{
		entity_->SetPosition(event->x_, event->y_, event->z_);
		entity_->SetDelta(event->dx_, event->dy_, event->dz_);
		entity_->SetYPR(event->yaw_, event->pitch_, event->roll_);

		float vt = (float)sqrt(entity_->XDelta() * entity_->XDelta() + 
						entity_->YDelta() * entity_->YDelta() + 
						entity_->ZDelta() * entity_->ZDelta());

		((SimMoverClass*)entity_)->SetVt(vt);
		((SimMoverClass*)entity_)->SetKias(get_air_speed (vt * FTPSEC_TO_KNOTS, -1*FloatToInt32(entity_->ZPos())));

		entity_->SetUpdateTime(vuxGameTime);

		// if we are not awake - (and also by implication remote)
		// then noexec because we are not actually in the simobjectlist

		NoExec (vuxGameTime);
	}
	else
	{
//		MonoPrint ("SL %08x %f,%f,%f\n", entity_, event->x_, event->y_, event->z_);
//		MonoPrint ("SL %08x %f,%f,%f\n", entity_, event->dx_, event->dy_, event->dz_);
//		MonoPrint ("SL %08x  %f,%f,%f\n", entity_, event->yaw_, event->pitch_, event->roll_);
//		MonoPrint ("dx,dy,dz %08x %f %f %f\n", entity_, entity_->XPos(), entity_->YPos(), entity_->ZPos());

		err = VuDelaySlave::Handle (event);
	}

	return err;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Driver messages
//--------------------------------------------------

