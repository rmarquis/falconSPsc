///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include "vu2.h"
#include "vu_priv.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// VuHashTable
//-----------------------------------------------------------------------------

VuHashTable::VuHashTable(unsigned int tableSize, unsigned int key)
: VuCollection(),
count_(0),
capacity_(tableSize),
key_(key)
{
	table_ = new VuLinkNode*[tableSize+1];
	
	// clear the pointers
	VuLinkNode **ptr = table_;
	for (unsigned int i = 0; i < tableSize; i++) {
		*ptr++ = vuTailNode;
	}
	*ptr = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuHashTable::~VuHashTable()
{
	Purge();
	// do we need to clear this out?
	delete [] table_;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE 
VuHashTable::Insert(VuEntity *entity)
{
	if (!entity)
		return VU_SUCCESS;

	if (entity->VuState() != VU_MEM_ACTIVE) {
		return VU_ERROR;
	}
	unsigned int index = ((VU_KEY)entity->Id() * key_) % capacity_;
	VuLinkNode **entry = table_ + index;
	
	VuEnterCriticalSection();
	assert(Find(entity) == NULL); // should not be in already
	*entry = new VuLinkNode(entity, *entry);
	count_++;
	VuExitCriticalSection();
	
	return VU_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE 
VuHashTable::Remove(VuEntity *entity)
{
	if (!entity)
		return VU_SUCCESS;

	unsigned int index = ((VU_KEY)entity->Id() * key_) % capacity_;
	VuLinkNode **entry = table_ + index;
	VuLinkNode *last = 0;
	VuLinkNode *ptr;
	VU_ERRCODE retval = VU_NO_OP;
	
	VuEnterCriticalSection();
	
	ptr = *entry;
	if (ptr->entity_ == 0) {
		// not found
		VuExitCriticalSection();
		return VU_NO_OP;
	}
	if (ptr->freenext_ != 0) {
		// already done
		VuExitCriticalSection();
		return VU_NO_OP;
	}
	
	while (ptr->entity_ && ptr->entity_ != entity) {
		last = ptr;
		ptr = ptr->next_;
	}
	if (ptr->entity_) {
		if (last) {
			last->next_ = ptr->next_;
		} else {
			*entry = ptr->next_;
		}
		// put curr on VUs pending delete queue
		vuCollectionManager->PutOnKillQueue(ptr);
		count_--;
		retval = VU_SUCCESS;
	}
	
	VuExitCriticalSection();
	return retval;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE 
VuHashTable::Remove(VU_ID entityId)
{
	unsigned int index = ((VU_KEY)entityId * key_) % capacity_;
	VuLinkNode **entry = table_ + index;
	VuLinkNode *ptr = *entry;
	VuLinkNode *last = 0;
	VU_ERRCODE retval = VU_NO_OP;
	
	if (ptr == 0) {
		// not found
		return VU_NO_OP;
	}
	if (ptr->freenext_ != 0) {
		// already done
		return VU_NO_OP;
	}
	
	VuEnterCriticalSection();
	while (ptr->entity_ && ptr->entity_->Id() != entityId) {
		last = ptr;
		ptr = ptr->next_;
	}
	if (ptr->entity_) {
		if (last) {
			last->next_ = ptr->next_;
		} else {
			*entry = ptr->next_;
		}
		// put curr on VUs pending delete queue
		vuCollectionManager->PutOnKillQueue(ptr);
		count_--;
		retval = VU_SUCCESS;
	}
	VuExitCriticalSection();
	return retval;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int 
VuHashTable::Purge(VU_BOOL all)
{
	int retval = 0;
	unsigned int index = 0;
	VuLinkNode **entry = table_;
	VuLinkNode *ptr = *entry;
	VuLinkNode *next, *last;
	
	VuEnterCriticalSection();
	while (index < capacity_) {
		last = 0;
		while (ptr->entity_) {
			next = ptr->next_;
			VuEntity *ent = ptr->entity_;
			if (!all && ((ent->IsPrivate()&&ent->IsPersistent()) || ent->IsGlobal())){
				ptr->next_ = vuTailNode;
				if (last) {
					last->next_ = ptr;
				} else {
					*entry = ptr;
				}
				last = ptr;
			} else {
				vuCollectionManager->PutOnKillQueue(ptr, TRUE);
				retval++;
				count_--;
			}
			ptr = next;
		}
		if (!last) {
			*entry = vuTailNode;
		}
		index++;
		entry = table_ + index;
		ptr = *entry;
	}
	VuExitCriticalSection();
	return retval;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int 
VuHashTable::Count()
{
	return count_;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuEntity *
VuHashTable::Find(VU_ID entityId)
{
	unsigned int index = ((VU_KEY)entityId * key_) % capacity_;
	VuLinkNode *ptr = *(table_ + index);
	
	while (ptr->entity_ && ptr->entity_->Id() != entityId) {
		ptr = ptr->next_;
	}
	return ptr->entity_;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuEntity *
VuHashTable::Find(VuEntity *ent)
{
	unsigned int index = ((VU_KEY)ent->Id() * key_) % capacity_;
	VuLinkNode *ptr = *(table_ + index);
	
	while (ptr->entity_ && ptr->entity_->Id() != ent->Id()) {
		ptr = ptr->next_;
	}
	return ptr->entity_;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int
VuHashTable::Type()
{
	return VU_HASH_TABLE_COLLECTION;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

