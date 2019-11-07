///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include "vu2.h"
#include "vu_priv.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// VuFilter -- dummy base class
//-----------------------------------------------------------------------------

VuFilter::~VuFilter()
{
	// empty
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_BOOL VuFilter::RemoveTest(VuEntity *)
{
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_BOOL VuFilter::Notice(VuMessage *)
{
	//  if ((1<<event->Type()) & (VU_DELETE_EVENT_BITS | VU_CREATE_EVENT_BITS)) {
	//    return TRUE;
	//  }
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////