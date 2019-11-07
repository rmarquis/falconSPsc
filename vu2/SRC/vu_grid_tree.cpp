///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include "vu2.h"
#include "vu_priv.h"
#include <shi/ShiError.h>
#include <float.h>

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// VuGridTree
//-----------------------------------------------------------------------------

VuGridTree::VuGridTree(VuBiKeyFilter* filter,
                       unsigned int   numrows, 
                       BIG_SCALAR     center,
                       BIG_SCALAR     radius,
                       VU_BOOL        wrap)
					   : VuCollection(),
					   rowcount_(numrows),
					   wrap_(wrap),
					   suspendUpdates_(FALSE),
					   nextgrid_(0)
{
	filter_       = (VuBiKeyFilter*)filter->Copy();
	ulong icenter = filter->CoordToKey1(center);
	ulong iradius = filter->Distance1(radius);
	bottom_       = icenter - iradius;
	top_          = icenter + iradius;
	rowheight_    = (top_ - bottom_)/rowcount_;
	invrowheight_ = 1.0f/(float)((top_ - bottom_)/rowcount_);
	table_        = new VuRedBlackTree[numrows];
	
	VuRedBlackTree* currow = table_;
	
	for (unsigned int i = 0; i < numrows; i++) {
		currow->InitFilter(filter_);
		currow++;
	}
	
	vuCollectionManager->GridRegister(this);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuGridTree::~VuGridTree()
{
	Purge();
	delete [] table_;
	delete filter_;
	filter_ = 0;
	vuCollectionManager->GridDeRegister(this);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuRedBlackTree* 
VuGridTree::Row(VU_KEY key1)
{
	int index = 0;
	
	ShiAssert(FTOL(58.9f) == 58); // jpo - detects a slip in fpu state
	// if this goes off - the FPU is rounding to nearest rather than down.
	_controlfp(_RC_CHOP,MCW_RC);

	if (wrap_) {
		if (key1 < bottom_) {
			key1 = top_ - ((bottom_ - key1)%(top_ - bottom_));
		} else {
			key1 = ((key1 - bottom_)%(top_ - bottom_)) + bottom_;
		}
	}
	
	if (key1 > bottom_) {
		index = FTOL((key1 - bottom_)*invrowheight_);
	}
	
	if (index > rowcount_-1) {
		index = rowcount_-1;
	}
	
	return table_ + index;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE 
VuGridTree::ForcedInsert(VuEntity* entity)
{
	if (entity->VuState() == VU_MEM_ACTIVE) {
	    assert(Find(entity) == NULL); // JPO check its not there already
		if (!filter_->RemoveTest(entity)) return VU_NO_OP;
		VuRedBlackTree *row = Row(filter_->Key1(entity));
		
		return row->ForcedInsert(entity);
	}
	return VU_ERROR;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE 
VuGridTree::Insert(VuEntity *entity)
{
	if (entity->VuState() == VU_MEM_ACTIVE) {
	    assert(Find(entity) == NULL); // JPO check its not there already
		if (!filter_->Test(entity)) return VU_NO_OP;
		VuRedBlackTree *row = Row(filter_->Key1(entity));
		return row->Insert(entity);
	}
	return VU_ERROR;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE 
VuGridTree::Remove(VuEntity *entity)
{
	if (filter_->RemoveTest(entity)) {
		VuRedBlackTree* row = Row(filter_->Key1(entity));
		VU_ERRCODE res =  row->Remove(entity);
		assert(Find(entity) == NULL); // JPO should be gone now.
		return res;
	}
	return VU_NO_OP;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE 
VuGridTree::Remove(VU_ID entityId)
{
	VuEntity *ent = vuDatabase->Find(entityId);
	if (ent) {
		return Remove(ent);
	}
	return VU_NO_OP;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE
VuGridTree::Move(VuEntity *ent, BIG_SCALAR coord1, BIG_SCALAR coord2)
{
	if (filter_->RemoveTest(ent)) {
		
		VU_KEY key1 = filter_->Key1(ent);
		VU_KEY key2 = filter_->Key2(ent);
		
		VuRedBlackTree* row = Row(key1);
		
		if (row->root_) {
			VuRBNode* node = row->root_->Find(key2);
			
			if (node) {
				VuLinkNode* linknode     = node->head_;
				VuLinkNode* lastlinknode = 0;
				
				while (linknode->entity_ && linknode->entity_ != ent) {
					lastlinknode = linknode;
					linknode     = linknode->next_;
				}
				
				if (linknode->entity_) {
					VU_KEY newKey1 = filter_->CoordToKey1(coord1);
					VU_KEY newKey2 = filter_->CoordToKey2(coord2);
					
					if (key1 == newKey1 && key2 == newKey2) {
						// we didn't move
						return VU_SUCCESS;
					}
					
					VuRedBlackTree* newrow = Row(newKey1);
					if (lastlinknode || linknode->next_ != vuTailNode) {
						// we have multiple ents on this RBNode
						if (key2 == newKey2 && row == newrow) {
							// we didn't move
							return VU_SUCCESS;
						}
						VuEnterCriticalSection();
						if (lastlinknode) {
							lastlinknode->next_ = linknode->next_;
						} else {
							node->head_ = linknode->next_;
						}
						if (newrow->InsertLink(linknode, newKey2) != VU_SUCCESS) {
							vuCollectionManager->PutOnKillQueue(linknode);
						}
						VuExitCriticalSection();
						return VU_SUCCESS;
					}
					
					VuEnterCriticalSection();
					row->RemoveNode(node);
					node->key_ = newKey2;
					newrow->InsertNode(node);
					VuExitCriticalSection();
					
					return VU_SUCCESS;
				}
			}
			else {	// JPO if this goes off the VU database has got corrupted

#ifdef _DEBUG
			    //ShiWarning("Entity out of position on key2");
//			    VU_PRINT ("Entity out of position on key2 %d id %d\n", key2, ent->Id());
#endif
			}
		}
		else {	// JPO if this goes off the VU database has got corrupted
#ifdef _DEBUG
		    //ShiWarning("Entity out of position on key1");
//		    VU_PRINT ("Entity out of position on key1 %d id %d\n", key1, ent->Id());
#endif
		}
	}
	return VU_NO_OP;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE 
VuGridTree::Rebuild()
{
	VuEnterCriticalSection();
	VuLinkNode* holding = vuTailNode, *next;;
	int entcount=0;
	
	// step 1: dismantle...
	VuRedBlackTree* currow = table_;
	
	for (int i = 0; i < rowcount_; i++) {
		entcount += currow->Purge(&holding);
		currow++;
	}
	// step 2: rebuild...
	while (holding != vuTailNode) {
		next = holding->freenext_;
		Insert(holding->entity_);
		vuCollectionManager->PutOnKillQueue(holding, TRUE);
		holding = next;
	}
	
	VuExitCriticalSection();
	return VU_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int 
VuGridTree::Purge(VU_BOOL all)
{
	int retval = 0;
	VuEnterCriticalSection();
	VuRedBlackTree *currow = table_;
	for (int i = 0; i < rowcount_; i++) {
		retval += currow->Purge(all);
		currow++;
	}
	VuExitCriticalSection();
	
	return retval;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int 
VuGridTree::Count()
{
	int count = 0;
	VuRedBlackTree *currow = table_;
	for (int i = 0; i < rowcount_; i++) {
		count += currow->Count();
		currow++;
	}
	return count;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuEntity*
VuGridTree::Find(VU_ID entityId)
{
	VuEntity* ent = vuDatabase->Find(entityId);
	if (ent) {
		return Find(ent);
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuEntity *
VuGridTree::Find(VuEntity* ent)
{
	VuRedBlackTree* row = Row(filter_->Key1(ent));
	return row->Find(ent);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuEntity *
VuGridTree::FindByKey(VU_KEY key1, 
                      VU_KEY key2)
{
	VuRedBlackTree* row = Row(key1);
	return row->FindByKey(key2);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int VuGridTree::CheckIntegrity(VuRBNode *node , int col)
{
	int retval = 0;
	
#if defined(_DEBUG)
	if (node) {
		retval += CheckIntegrity(node->left_, col);
		VuLinkNode *link = node->head_;
		int i = 0;
		while (link->entity_) {
			if (filter_->Key(link->entity_) != node->key_) {
				VU_PRINT("CheckIntegrity(0x%x, col %d): nodekey = %d, entkey = %d, depth = %d\n", 
					(VU_KEY)link->entity_->Id(), col, node->key_, filter_->Key(link->entity_), i);
				retval++;
			}
			link = link->next_;
			i++;
		}
		retval += CheckIntegrity(node->right_, col);
	}
#endif
	return retval;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void
VuGridTree::DebugString(char *buf)
{
#if defined(_DEBUG)
	int count = 0;
	
	VuRedBlackTree* currow = table_;
	for (int i = 0; i < rowcount_; i++) {
		if (CheckIntegrity(currow->root_, i) > 0) {
			assert(0);
		}
		count = currow->Count();
		if (count < 0) {
			*buf++ = '-';
		} else if (count == 0) {
			*buf++ = '.';
		} else if (count < 10) {
			*buf++ = '1' + count - 1;
		} else {
			*buf++ = '*';
		}
		currow++;
	}
	*buf++ = '\0';
#else
	buf[0] = '\0';
#endif
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int
VuGridTree::Type()
{
	return VU_GRID_TREE_COLLECTION;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
