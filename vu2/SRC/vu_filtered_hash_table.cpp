///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include "vu2.h"
#include "vu_priv.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// VuFiltredHashTable
//-----------------------------------------------------------------------------

VuFilteredHashTable::VuFilteredHashTable
(
	VuFilter *filter,
	unsigned int tableSize,
	uint key
) : VuHashTable(tableSize, key)
{
	filter_ = filter->Copy();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuFilteredHashTable::~VuFilteredHashTable()
{
	delete filter_;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE VuFilteredHashTable::ForcedInsert (VuEntity *entity)
{
	if (!filter_->RemoveTest(entity))
	{
		return VU_NO_OP;
	}

	return VuHashTable::Insert(entity);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE VuFilteredHashTable::Insert (VuEntity *entity)
{
	if (!filter_->Test(entity))
	{
		return VU_NO_OP;
	}

	return VuHashTable::Insert(entity);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE VuFilteredHashTable::Handle (VuMessage *msg)
{
	if (filter_->Notice(msg))
	{
		VuEntity *ent = msg->Entity();

		if (ent && filter_->RemoveTest(ent))
		{
			if (Find(ent->Id()))
			{
				if (!filter_->Test(ent))
				{
					// ent is in table, but doesn't belong there...
					Remove(ent);
				}
			}
			else if (filter_->Test(ent))
			{
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

int VuFilteredHashTable::Type()
{
	return VU_FILTERED_HASH_TABLE_COLLECTION;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
