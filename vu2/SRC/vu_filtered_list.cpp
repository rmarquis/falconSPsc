///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include "vu2.h"
#include "vu_priv.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// VuFiltredList
//-----------------------------------------------------------------------------

VuFilteredList::VuFilteredList(VuFilter* filter) : VuLinkedList()
{
	filter_ = filter->Copy();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuFilteredList::~VuFilteredList()
{
	delete filter_;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE 
VuFilteredList::Handle(VuMessage* msg)
{
	if (filter_->Notice(msg)) {
		VuEntity* ent = msg->Entity();
		if (ent && filter_->RemoveTest(ent)) {
			if (Find(ent->Id())) {
				if (!filter_->Test(ent)) {
					// ent is in table, but doesn't belong there...
					Remove(ent);
				}
			} else if (filter_->Test(ent)) {
				// ent is not in table, but does belong there...
				Insert(ent);
			}
			return VU_SUCCESS;
		}
	}
	return VU_NO_OP;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE
VuFilteredList::ForcedInsert(VuEntity *entity)
{
	if (filter_->RemoveTest(entity))
		return VuLinkedList::Insert(entity);
	else
		return VU_NO_OP;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE
VuFilteredList::Insert(VuEntity *entity)
{
	if (filter_->Test(entity))
		return VuLinkedList::Insert(entity);
	else
		return VU_NO_OP;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE
VuFilteredList::Remove(VuEntity *entity)
{
	if (filter_->RemoveTest(entity))
		return VuLinkedList::Remove(entity);
	else
		return VU_NO_OP;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE
VuFilteredList::Remove(VU_ID entityId)
{
	VuEntity* ent = vuDatabase->Find(entityId);
	
	if (ent) {
		return Remove(ent);
	}
	
	return VuLinkedList::Remove(entityId);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int
VuFilteredList::Type()
{
	return VU_FILTERED_LIST_COLLECTION;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
