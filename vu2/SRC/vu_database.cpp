///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include "vu2.h"
#include "vu_priv.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuDatabase::VuDatabase(unsigned int tableSize, unsigned int key)
: VuHashTable(tableSize, key)
{
	// do init
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuDatabase::~VuDatabase()
{
	Purge();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int 
VuDatabase::Purge(VU_BOOL all)
{
	int retval = 0;
	unsigned int index = 0;
	VuLinkNode **entry = table_;
	VuLinkNode *ptr = *entry;
	VuLinkNode *next, *last = 0;
	
	VuEnterCriticalSection();
	
	while (index < capacity_) {
		last = 0;
		while (ptr->entity_) {
			next = ptr->next_;
			VuEntity* ent = ptr->entity_;
			
			if (!all && ((ent->IsPrivate()&&ent->IsPersistent()) || ent->IsGlobal())){
				ptr->next_ = vuTailNode;
				if (last) {
					last->next_ = ptr;
				} else {
					*entry = ptr;
				}
				last = ptr;
			} 
			else {
				vuCollectionManager->PutOnKillQueue(ptr, TRUE);
				ptr->entity_ = 0; // mark dead
				VuDeReferenceEntity(ent); // JPO - swap around so not detected in database
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
VuDatabase::Suspend(VU_BOOL all)
{
	int retval = 0;
	unsigned int index = 0;
	VuLinkNode **entry = table_;
	VuLinkNode *next, *last = 0;
	VuLinkNode *ptr;
	
	VuEnterCriticalSection();
	
	while (index < capacity_) {
		last = 0;
		ptr = *entry;
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
				if (last) {
					last->next_ = next;
				}
				else {
					*entry = next;
				}
				ptr->entity_->RemovalCallback();
				vuAntiDB->Insert(ptr->entity_);
				VuDeReferenceEntity(ptr->entity_);
				ptr->entity_ = NULL; // JPO mark dead.
				retval++;
				count_--;
			}
			ptr = next;
		}
		index++;
		entry = table_ + index;
	}
	VuExitCriticalSection();
	return retval;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE 
VuDatabase::Handle(VuMessage *msg)
{
	// note: this should work on Create & Delete messages, but those are
	// currently handled elsewhere... for now... just pass on to collection mgr
	vuCollectionManager->Handle(msg);
	return VU_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE VuDatabase::Insert(VuEntity *entity)
{
	if (Find(entity->Id())) {
		return VU_ERROR;
	}
	entity->SetVuState(VU_MEM_ACTIVE);
	VuHashTable::Insert(entity);
	VuReferenceEntity(entity);
	vuCollectionManager->Add(entity);
	if (entity->IsLocal() && (!entity->IsPrivate())) {
		VuCreateEvent *event = 0;
		VuTargetEntity *target = vuGlobalGroup;
		if (!entity->IsGlobal()) {
			target = vuLocalSessionEntity->Game();
		}
		event = new VuCreateEvent(entity, target);
		event->RequestReliableTransmit ();
		VuMessageQueue::PostVuMessage(event);
	}
	entity->InsertionCallback();
	return VU_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE VuDatabase::QuickInsert(VuEntity *entity)
{
	if (Find(entity->Id())) {
		return VU_ERROR;
	}
	entity->SetVuState(VU_MEM_ACTIVE);
	VuHashTable::Insert(entity);
	VuReferenceEntity(entity);
	vuCollectionManager->Add(entity);
	if (entity->IsLocal() && (!entity->IsPrivate())) {
		VuCreateEvent *event = 0;
		VuTargetEntity *target = vuGlobalGroup;
		if (!entity->IsGlobal()) {
			target = vuLocalSessionEntity->Game();
		}
		event = new VuCreateEvent(entity, target);
		event->RequestReliableTransmit ();
		event->RequestOutOfBandTransmit ();
		VuMessageQueue::PostVuMessage(event);
	}
	entity->InsertionCallback();
	return VU_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE
VuDatabase::SilentInsert(VuEntity *entity)
{
	if (Find(entity->Id())) {
		return VU_ERROR;
	}
	entity->SetVuState(VU_MEM_ACTIVE);
	VuHashTable::Insert(entity);
	VuReferenceEntity(entity);
	vuCollectionManager->Add(entity);
	entity->InsertionCallback();
	return VU_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE
VuDatabase::Remove(VuEntity *entity)
{
	if (VuHashTable::Remove(entity) && entity->VuState() == VU_MEM_ACTIVE) {
		entity->SetVuState(VU_MEM_PENDING_DELETE);
		vuCollectionManager->Remove(entity);
		entity->RemovalCallback();
		VuEvent *event;
		if (entity->IsLocal() && !entity->IsPrivate()) {
			event = new VuDeleteEvent(entity);
			event->RequestReliableTransmit();
//me123			event->RequestOutOfBandTransmit ();
		} else {
			event = new VuReleaseEvent(entity);
		}
		VuMessageQueue::PostVuMessage(event);
		return VU_SUCCESS;
	}
	return VU_NO_OP;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE
VuDatabase::SilentRemove(VuEntity *entity)
{
	if (VuHashTable::Remove(entity) && entity->VuState() == VU_MEM_ACTIVE) {
		entity->SetVuState(VU_MEM_PENDING_DELETE);
		vuCollectionManager->Remove(entity);
		entity->RemovalCallback();
		return VU_SUCCESS;
	}
	return VU_NO_OP;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE
VuDatabase::DeleteRemove(VuEntity *entity)
{
	if (VuHashTable::Remove(entity) && entity->VuState() == VU_MEM_ACTIVE) {
		entity->SetVuState(VU_MEM_PENDING_DELETE);
		vuCollectionManager->Remove(entity);
		entity->RemovalCallback();
		return VU_SUCCESS;
	}
	return VU_NO_OP;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE
VuDatabase::Remove(VU_ID entityId)
{
	VuEntity *ent = Find(entityId);
	
	if (ent)
	{
		return Remove(ent);
	}
	
	return VU_NO_OP;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE
VuDatabase::Save(char* filename, int (*headercb)(FILE*), int (*savecb)(FILE*, VuEntity*))
{
	if (!filename) return VU_ERROR;
	if (!savecb)   return VU_ERROR;
	
	FILE* file = fopen(filename, "w");
	if (!file) return VU_ERROR;
	
	if (headercb) {
		if ((*headercb)(file) < 0) {
			fclose(file);
			return VU_ERROR;
		}
	}
	
	// reverse order...
	VuLinkedList tmp;
	VuEntity *ent;
	VuDatabaseIterator dbiter;
	for (ent = dbiter.GetFirst(); ent; ent = dbiter.GetNext()) {
		tmp.Insert(ent);
	}
	VuListIterator lliter(&tmp);
	for (ent = lliter.GetFirst(); ent; ent = lliter.GetNext()) {
		if ((*savecb)(file, ent) >= 0) {
			ent->Save(file);
		}
	}
	int tail = 0;
	fwrite(&tail, sizeof(int), 1, file);
	fclose(file);
	return VU_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE
VuDatabase::Restore(char *filename, int (*headercb)(FILE *),
					VuEntity * (*restorecb)(FILE *))
{
	if (!filename) return VU_ERROR;
	if (!restorecb) return VU_ERROR;
	
	FILE *file = fopen(filename, "r");
	if (!file) return VU_ERROR;
	
	if (headercb) {
		if ((*headercb)(file) < 0) {
			fclose(file);
			return VU_ERROR;
		}
	}
	
	VuEntity *ent = (*restorecb)(file);
	while (ent) {
		Insert(ent);
		ent = (*restorecb)(file);
	}
	
	fclose(file);
	return VU_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int
VuDatabase::Type()
{
	return VU_DATABASE_COLLECTION;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
