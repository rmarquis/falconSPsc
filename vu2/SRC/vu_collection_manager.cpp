///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include "vu2.h"
#include "vu_priv.h"
#include <windows.h> // JB 010318 Needed for CTD checks

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuCollectionManager::VuCollectionManager()
: collcoll_(0),
gridcoll_(0),
itercoll_(0),
rbitercoll_(0),
llkillhead_(vuTailNode),
rbkillhead_(0)
{
	bogusNode = new VuRBNode((VuEntity *)0, ~(ulong)0);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuCollectionManager::~VuCollectionManager()
{
	Purge();
	VuLinkNode *curr;
	while ((curr = llkillhead_) != vuTailNode) {
		llkillhead_ = llkillhead_->freenext_;
		delete curr;
	}
	VuRBNode *rbcurr;
	while ((rbcurr = rbkillhead_) != 0) {
		rbkillhead_ = rbkillhead_->parent_;
		delete rbcurr;
	}
	delete bogusNode->head_;
	delete bogusNode;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void
VuCollectionManager::Register(VuIterator* iter)
{
	VuEnterCriticalSection();
	iter->nextiter_ = itercoll_;
	itercoll_       = iter;
	VuExitCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void
VuCollectionManager::DeRegister(VuIterator* iter)
{
	VuEnterCriticalSection();
	VuIterator* curr = itercoll_;
	VuIterator* last = 0;
	
	while (curr) {
	    assert(FALSE == F4IsBadReadPtr(curr, sizeof *curr)); // JPO CTD
		if (curr == iter) {
			if (!last) {
				itercoll_ = curr->nextiter_;
			} else {
				last->nextiter_ = curr->nextiter_;
			}
			curr->nextiter_ = 0;
			break;
		}
		last = curr;
		curr = curr->nextiter_;
	}
	VuExitCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void
VuCollectionManager::RBRegister(VuRBIterator* iter)
{
	VuEnterCriticalSection();
	iter->rbnextiter_ = rbitercoll_;
	rbitercoll_       = iter;
	VuExitCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void
VuCollectionManager::RBDeRegister(VuRBIterator* iter)
{
	VuEnterCriticalSection();
	VuRBIterator* curr = rbitercoll_;
	VuRBIterator* last = 0;
	
	while (curr) {
	    assert(FALSE == F4IsBadReadPtr(curr, sizeof *curr)); // JPO
		if (curr == iter) {
			if (!last) {
				rbitercoll_ = curr->rbnextiter_;
			} else {
				last->rbnextiter_ = curr->rbnextiter_;
			}
			curr->rbnextiter_ = 0;
			break;
		}
		last = curr;
		curr = curr->rbnextiter_;
	}
	
	VuExitCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void
VuCollectionManager::Register(VuCollection* coll)
{
    VuEnterCriticalSection(); // JPO protect the whole thing
	if (coll != vuDatabase) {
		coll->nextcoll_ = collcoll_;
		collcoll_ = coll;
	}
	VuExitCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void
VuCollectionManager::DeRegister(VuCollection* coll)
{
	VuEnterCriticalSection();
	
	VuCollection* curr = collcoll_;
	VuCollection* last = 0;
	
	while (curr) {
		if (curr == coll) {
			if (!last) {
				collcoll_ = curr->nextcoll_;
			} else {
				last->nextcoll_ = curr->nextcoll_;
			}
			curr->nextcoll_ = 0;
			break;
		}
		last = curr;
		curr = curr->nextcoll_;
	}
	
	VuExitCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void
VuCollectionManager::GridRegister(VuGridTree* grid)
{
	VuEnterCriticalSection();
	grid->nextgrid_ = gridcoll_;
	gridcoll_       = grid;
	VuExitCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void
VuCollectionManager::GridDeRegister(VuGridTree *grid)
{
	VuEnterCriticalSection();
	
	VuGridTree* curr = gridcoll_;
	VuGridTree* last = 0;
	
	while (curr) {
	    assert(FALSE == F4IsBadReadPtr(curr, sizeof *curr)); // JPO
		if (curr == grid) {
			if (!last) {
				gridcoll_ = curr->nextgrid_;
			} else {
				last->nextgrid_ = curr->nextgrid_;
			}
			curr->nextgrid_ = 0;
			break;
		}
		last = curr;
		curr = curr->nextgrid_;
	}
	
	VuExitCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_BOOL
VuCollectionManager::IsReferenced(VuEntity* ent)
{
	VuIterator* curr = itercoll_;
	
	while(curr) {
	    assert(FALSE == F4IsBadReadPtr(curr, sizeof *curr)); // JPO
		if (curr->IsReferenced(ent)) {
			return TRUE;
		}
		curr = curr->nextiter_;
	}
	
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_BOOL
VuCollectionManager::IsReferenced(VuRBNode* node)
{
	VuRBIterator* curr = rbitercoll_;

	// 2002-02-04 ADDED BY S.G. If node is false, then it can't be a valid entity, right? That's what I think too :-)
	if (!node)
		return FALSE;

	while(curr) {
	    assert(FALSE == F4IsBadReadPtr(curr, sizeof *curr)); // JPO
		if (curr->curnode_ == node) {
			return TRUE;
		}
		curr = curr->rbnextiter_;
	}
	
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void
VuCollectionManager::Purge()
{
	VuEnterCriticalSection();
	
	// LinkNode cleanup
	VuLinkNode  firstlast(0, 0);
	VuLinkNode* curr;
	VuLinkNode* last = &firstlast;
	
	firstlast.freenext_ = llkillhead_;
	curr                = llkillhead_;
	
	while (
		curr && //!F4IsBadReadPtr(curr, sizeof(VuLinkNode)) && // JB 010318 CTD (too much CPU)
		curr != vuTailNode) {
	    assert(FALSE == F4IsBadReadPtr(curr, sizeof *curr));
		if (IsReferenced(curr->entity_)) {
			last = curr;
			curr = curr->freenext_;
		} 
		else {
			last->freenext_ = curr->freenext_;
			delete curr;
			curr = last->freenext_;
		}
	}
	llkillhead_ = firstlast.freenext_;
	
	// RBNode cleanup
//	VuRBNode* rbnewhead = 0;
	VuRBNode* rblast    = 0;
	VuRBNode* tmp       = 0;
	VuRBNode* rbcurr;
	
	rbcurr      = rbkillhead_;
	rbkillhead_ = 0;
	
	while (rbcurr != 0) {
		if (IsReferenced(rbcurr)) {
			if (rbkillhead_ == 0) {
				rbkillhead_ = rbcurr;
			}
			rblast = rbcurr;
			rbcurr = rbcurr->parent_;
		} else {
			if (rblast != 0) {
				rblast->parent_ = rbcurr->parent_;
			}
			tmp = rbcurr->parent_;
			delete rbcurr;
			rbcurr = tmp;
		}
	}
	
	VuExitCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void
VuCollectionManager::Add(VuEntity* ent)
{
	VuEnterCriticalSection();
	for (VuCollection* curr = collcoll_; curr; curr = curr->nextcoll_) {
		if (curr != vuDatabase) {
			curr->Insert(ent);
		}
	}
	VuExitCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void
VuCollectionManager::Remove(VuEntity* ent)
{
	VuEnterCriticalSection();
	
	for (VuCollection *curr = collcoll_; curr; curr = curr->nextcoll_) {
	    assert(FALSE == F4IsBadReadPtr(curr, sizeof *curr)); // JPO
		if (curr != vuDatabase) {
			curr->Remove(ent);
		}
	}
	
	VuExitCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int
VuCollectionManager::HandleMove(VuEntity*  ent,
								BIG_SCALAR coord1,
                                BIG_SCALAR coord2)
{
	int retval = 0;
	VuEnterCriticalSection();
	
	for (VuGridTree *curr = gridcoll_; curr; curr = curr->nextgrid_) {
	    assert(FALSE == F4IsBadReadPtr(curr, sizeof *curr)); // JPO
		if (!curr->suspendUpdates_) {
			curr->Move(ent, coord1, coord2);
		}
	}
	
	VuExitCriticalSection();
	return retval;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int
VuCollectionManager::Handle(VuMessage* msg)
{
	VuEnterCriticalSection();
	int retval = VU_NO_OP;
	
	for (VuCollection* curr = collcoll_; curr; curr = curr->nextcoll_) {
	    assert(FALSE == F4IsBadReadPtr(curr, sizeof *curr)); // JPO
		if (curr != vuDatabase) {
			if (curr->Handle(msg) == VU_SUCCESS) {
				retval = VU_SUCCESS;
			}
		}
	}
	
	VuExitCriticalSection();
	return retval;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void
VuCollectionManager::PutOnKillQueue(VuLinkNode* node, 
                                    VU_BOOL     killnow)
{
	assert(VuHasCriticalSection()); // JB 010614

	if (killnow) {
		node->next_ = vuTailNode;
	} 
	else {
		// need to ensure that newly killed node has no dead references
		VuLinkNode *curr = llkillhead_;
		if (curr == node)
			return; // JB already in the queue

		while (curr != vuTailNode) {
			assert(FALSE == F4IsBadReadPtr(curr, sizeof *curr)); // JPO
			if (curr->next_ == node) {
				curr->next_ = node->next_;
			}

			if (curr == curr->freenext_) // JB 010607 infinite loop
			{
				assert(FALSE);
				break;
			}

			curr = curr->freenext_;
		}
	}
	node->freenext_ = llkillhead_;
	llkillhead_ = node;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void
VuCollectionManager::PutOnKillQueue(VuRBNode* node, 
                                    VU_BOOL   killnow)
{
	assert(VuHasCriticalSection()); // JB 010614

	if (killnow) {
		node->next_ = NULL;
	} 
	else {
		// need to ensure that newly killed node has no dead references
		VuRBNode* curr = rbkillhead_;

		if (curr == node)
			return; // JB already in the queue

		while (curr) {
	    assert(FALSE == F4IsBadReadPtr(curr, sizeof *curr)); // JPO
			if (curr->next_ == node) {
				curr->next_ = node->next_;
			}

			if (curr == curr->parent_) // JB 010607 infinite loop
			{
				assert(FALSE);
				break;
			}

			curr = curr->parent_;
		}
	}
	node->parent_ = rbkillhead_;
	rbkillhead_   = node;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int
VuCollectionManager::FindEnt(VuEntity* ent)
{
	int retval = 0;
	VuEnterCriticalSection();
	
	for (VuCollection *curr = collcoll_; curr; curr = curr->nextcoll_) {
	    assert(FALSE == F4IsBadReadPtr(curr, sizeof *curr)); // JPO
		VuEntity *ent2 = curr->Find(ent);
		if (ent2 && ent2 == ent) {
			
#ifdef DEBUG
			VU_PRINT("--> collection type 0x%x: Found entity 0x%x\n",	curr->Type(), (int)ent->Id());
#endif
			
			retval++;
		}
	}
	VuExitCriticalSection();
	return retval;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void
VuCollectionManager::Shutdown(VU_BOOL all)
{
	VuEnterCriticalSection();
	SanitizeKillQueue();
	for (VuCollection* curr = collcoll_; curr; curr = curr->nextcoll_) {
	    assert(FALSE == F4IsBadReadPtr(curr, sizeof *curr)); // JPO
		if (curr != vuDatabase) {
			curr->Purge(all);
		}
	}
	vuDatabase->Suspend(all);
	VuExitCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void
VuCollectionManager::SanitizeKillQueue()
{
	VuEnterCriticalSection();
	
	// LinkNode sanitize
	VuLinkNode *curr;
	
	curr = llkillhead_;
	while (curr != vuTailNode) {
	    assert(FALSE == F4IsBadReadPtr(curr, sizeof *curr)); // JPO
		curr->next_ = vuTailNode;
		curr = curr->freenext_;
	}
	
	// RBNode sanitize
	VuRBNode *rbcurr;
	
	rbcurr = rbkillhead_;
	while (rbcurr != 0) {
		rbcurr->next_ = 0;
		rbcurr = rbcurr->parent_;
	}
	
	VuExitCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
