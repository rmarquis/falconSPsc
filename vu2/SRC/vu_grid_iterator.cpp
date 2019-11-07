///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include "vu2.h"
#include "vu_priv.h"
#include <windows.h> // JB 010318 Needed for CTD checks

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// VuGridIterator
//-----------------------------------------------------------------------------

VuGridIterator::VuGridIterator
(
	VuGridTree* coll,
	VuEntity*   origin, 
	BIG_SCALAR  radius
) : VuRBIterator(coll)
{
	curRB_ = 0;
	key1min_ = 0;
	key1max_ = static_cast<VU_KEY>(~0);
	key2min_ = 0;
	key2max_ = static_cast<VU_KEY>(~0);

	VuEnterCriticalSection(); // JB 010719

	VU_KEY key1origin = coll->filter_->Key1(origin);
	VU_KEY key2origin = coll->filter_->Key2(origin);
	VU_KEY key1radius = coll->filter_->Distance1(radius);
	VU_KEY key2radius = coll->filter_->Distance2(radius);
	key1minRB_        = coll->table_;
	key1maxRB_        = coll->table_ + coll->rowcount_;
	
	if (key1origin > key1radius) 
	{
		key1min_ = key1origin - key1radius;
	}

	if (key2origin > key2radius)
	{
		key2min_ = key2origin - key2radius;
	}

	key1max_ = key1origin + key1radius;
	key2max_ = key2origin + key2radius;
	
	VuGridTree* gt = (VuGridTree *)collection_;
	
	if (!gt->wrap_)
	{
		if (key1min_ < gt->bottom_)
		{
			key1min_ = gt->bottom_ + ((gt->bottom_ - key1min_)%gt->rowheight_);
		}

		if (key1max_ > gt->top_)
		{
			key1max_ = gt->top_;
		}
	}

	VuExitCriticalSection(); // JB 010719
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuGridIterator::VuGridIterator
(
	VuGridTree* coll,
	BIG_SCALAR  xPos,
	BIG_SCALAR  yPos,
	BIG_SCALAR  radius
) : VuRBIterator(coll)
{

	curRB_ = 0;
	key1min_ = 0;
	key1max_ = static_cast<VU_KEY>(~0);
	key2min_ = 0;
	key2max_ = static_cast<VU_KEY>(~0);

	VuEnterCriticalSection(); // JB 010719

	VU_KEY key1origin = coll->filter_->CoordToKey1(xPos);
	VU_KEY key2origin = coll->filter_->CoordToKey2(yPos);
	VU_KEY key1radius = coll->filter_->Distance1(radius);
	VU_KEY key2radius = coll->filter_->Distance2(radius);
	key1minRB_        = coll->table_;
	key1maxRB_         = coll->table_ + coll->rowcount_;
	
	if (key1origin > key1radius)
	{
		key1min_ = key1origin - key1radius;
	}

	if (key2origin > key2radius)
	{
		key2min_ = key2origin - key2radius;
	}
	
	key1max_ = key1origin + key1radius;
	key2max_ = key2origin + key2radius;
	
	VuGridTree* gt = (VuGridTree *)collection_;
	
	if (!gt->wrap_)
	{
		if (key1min_ < gt->bottom_)
		{
			key1min_ = gt->bottom_ + ((gt->bottom_ - key1min_)%gt->rowheight_);
		}

		if (key1max_ > gt->top_)
		{
			key1max_ = gt->top_;
		}
	}

	VuExitCriticalSection(); // JB 010719
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuGridIterator::~VuGridIterator()
{
	// empty
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuEntity *VuGridIterator::GetFirst()
{
	if (collection_)
	{
		curlink_ = vuTailNode;
		key1cur_ = key1min_;

		VuEnterCriticalSection(); // JPO lock access
		curRB_   = ((VuGridTree *)collection_)->Row(key1cur_);
		curnode_ = curRB_->root_;
		
		if (curnode_)
		{
			curnode_ = curnode_->LowerBound(key2min_);

			if (curnode_ && curnode_->head_ && curnode_->key_ < key2max_)
			{
				curlink_ = curnode_->head_;
				VuExitCriticalSection();
				return curlink_->entity_;
			}
		}
		
		VuEntity *res = GetNext();
		VuExitCriticalSection();
		return res;

	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuEntity *VuGridIterator::GetNext()
{
    VuEnterCriticalSection(); // JPO lock access
	while (curnode_ == 0 && key1cur_ < key1max_)
	{
		// danm_TBD: what about non-wrapping edges?
		key1cur_ += ((VuGridTree *)collection_)->rowheight_;
		curRB_++;  if(curRB_==key1maxRB_) curRB_=key1minRB_;
		
		//if (!F4IsBadReadPtr(curRB_, sizeof(VuRedBlackTree))) // JB 011205 (too much CPU) // JB 010318 CTD 
		if (curRB_ != NULL) // JB 011205
			curnode_ = curRB_->root_;
		else
			curnode_ = NULL; // JB 010318 CTD

		if (curnode_)
		{
			curnode_ = curnode_->LowerBound(key2min_);

			if (curnode_ && curnode_->head_ && curnode_->key_ < key2max_)
			{
				curlink_ = curnode_->head_;

				VuExitCriticalSection();
				//if (curlink_) // JB 010222 CTD
				if (curlink_ && !F4IsBadReadPtr(curlink_, sizeof(VuLinkNode))) // JB 010404 CTD
					return curlink_->entity_;
				else
					return 0; // JB 010222 CTD
			}
		}
	}
	
	if (curnode_ == 0)
	{
		VuExitCriticalSection();
		return 0;
	}
	
	curlink_ = curlink_->next_;

	if (curlink_ == vuTailNode)
	{
		curnode_ = curnode_->next_;
		if (curnode_ == 0 || curnode_->key_ > key2max_)
		{
			// skip to next row
			curlink_ = vuTailNode;
			curnode_ = 0;
			VuExitCriticalSection();

			return GetNext();
		}

		curlink_ = curnode_->head_;
	}
	VuExitCriticalSection();

	//if (curlink_) // JB 010222 CTD
	if (curlink_ && !F4IsBadReadPtr(curlink_, sizeof(VuLinkNode))) // JB 010404 CTD
		return curlink_->entity_;
	else
		return 0; // JB 010222 CTD
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuEntity *VuGridIterator::GetFirst (VuFilter* filter)
{
	VuEntity* retval = GetFirst();
	
	if (retval == 0 || filter->Test(retval))
	{
		return retval;
	}

	return GetNext(filter);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuEntity *VuGridIterator::GetNext (VuFilter *filter)
{
	VuEntity* retval = 0;

	while ((retval = GetNext()) != 0)
	{
		if (filter->Test(retval))
		{
			return retval;
		}
	}

	return retval;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
