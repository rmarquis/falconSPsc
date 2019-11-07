///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include "vu2.h"
#include "vu_priv.h"
#include <shi/ShiError.h>
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// VuRedBlackTree
//-----------------------------------------------------------------------------

VuRedBlackTree::VuRedBlackTree(VuKeyFilter *filter)
: VuCollection(),
root_(0)
{
	filter_ = (VuKeyFilter *)filter->Copy();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// VuGridTree interface
VuRedBlackTree::VuRedBlackTree()
: VuCollection(),
root_(0),
filter_(0)
{
	// empty (call Init later)
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void
VuRedBlackTree::InitFilter(VuKeyFilter *filter)
{
	filter_ = (VuKeyFilter *)filter->Copy();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuRedBlackTree::~VuRedBlackTree()
{
	Purge();
	delete filter_;
	filter_ = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE 
VuRedBlackTree::Handle(VuMessage *msg)
{
	if (filter_ && filter_->Notice(msg)) {
		VuEntity *ent = msg->Entity();
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

void VuRedBlackTree::Dump(VuRBNode *, int)
{
#ifdef DEBUG
	if (!node) return;
	
	Dump(node->left_, level+1);
	char c = (node->color_ == BLACK ? '-' : '+');
	for (int i = 0; i<level-1; i++) {
		VU_PRINT("  ");
	}
	if (level) VU_PRINT("%c%c", c, c);
	VU_PRINT("> id 0x%x, key %d:\t", (VU_KEY)node->head_->entity_->Id(), node->key_);
	if (level < 3) VU_PRINT("\t");
	VU_PRINT("parent %d; left %d; right %d; next %d\n",
		(node->parent_ ? node->parent_->key_ : 0),
		(node->left_ ? node->left_->key_ : 0),
		(node->right_ ? node->right_->key_ : 0),
		(node->next_ ? node->next_->key_ : 0) );
	Dump(node->right_, level+1);
#endif DEBUG
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void
VuRedBlackTree::Dump()
{
	Dump(root_, 0);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE
VuRedBlackTree::InsertFixUp(VuRBNode *newNode)
{
	VuRBNode *x, *y;
	
	x = newNode;
	while(x != root_ && x->parent_->color_ == RED) {
		if(x->parent_ == x->parent_->parent_->left_) {
			y = x->parent_->parent_->right_;
			if(y && y->color_ == RED) {
				x->parent_->color_ = BLACK;
				y->color_ = BLACK;
				x->parent_->parent_->color_ = RED;
				x = x->parent_->parent_;
			} else {
				if(x == x->parent_->right_) {
					x = x->parent_;
					RotateLeft(x);
				}
				x->parent_->color_ = BLACK;
				x->parent_->parent_->color_ = RED;
				RotateRight(x->parent_->parent_);
			}
		} else {
			y = x->parent_->parent_->left_;
			if(y && y->color_ == RED) {
				x->parent_->color_ = BLACK;
				y->color_ = BLACK;
				x->parent_->parent_->color_ = RED;
				x = x->parent_->parent_;
			} else {
				if(x == x->parent_->left_) {
					x = x->parent_;
					RotateRight(x);
				}
				x->parent_->color_ = BLACK;
				x->parent_->parent_->color_ = RED;
				RotateLeft(x->parent_->parent_);
			}
		}
	}
	root_->color_ = BLACK;
	
	return VU_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE
VuRedBlackTree::InsertNode(VuRBNode *newNode)
{
if(newNode->key_ == 0x90da || newNode->key_ == 0x90db)
{
	int i=0;
	i = 1;
}

	if (!newNode || !newNode->head_->entity_) {
		return VU_ERROR;
	}
	VuEntity *ent = newNode->head_->entity_;
	if (!filter_->Test(ent)) {
		return VU_NO_OP;
	}
	VuRBNode *curNode = root_;
	
	if (root_ == 0) {
		root_ = newNode;
		newNode->parent_ = 0;
		newNode->left_ = 0;
		newNode->right_ = 0;
		newNode->next_ = 0;
		newNode->color_ = BLACK;
		return VU_SUCCESS;
	}
	
	VuEnterCriticalSection();
	while(curNode) {
		if(newNode->key_ < curNode->key_) {
			if(curNode->left_) {
				curNode = curNode->left_;
			} else {
				newNode->parent_ = curNode;
				newNode->left_ = 0;
				newNode->right_ = 0;
				newNode->color_ = RED;
				curNode->left_ = newNode;
				newNode->next_ = newNode->SuccessorViaWalk();
				VuRBNode *prevNode = newNode->Predecessor();
				if (prevNode) {
					prevNode->next_ = newNode;
				}
				break;
			}
		} else if (newNode->key_ > curNode->key_) {
			if(curNode->right_) {
				curNode          = curNode->right_;
			} 
			else {
				newNode->parent_ = curNode;
				newNode->left_   = 0;
				newNode->right_  = 0;
				newNode->color_  = RED;
				curNode->right_  = newNode;
				newNode->next_   = newNode->SuccessorViaWalk();
				VuRBNode* prevNode = newNode->Predecessor();
				if (prevNode) {
					prevNode->next_ = newNode;
				}
				break;
			}
		} else {
			VuLinkNode* node = curNode->head_;
			while (node->next_->entity_) {
				node = node->next_;
			}
			node->next_    = newNode->head_;
			newNode->head_ = vuTailNode;
			vuCollectionManager->PutOnKillQueue(newNode);
			VuExitCriticalSection(); // JPO swapped this & previous lines.
			
			return VU_SUCCESS;
		}
	}
	
	VU_ERRCODE retval = InsertFixUp(newNode);
	
	VuExitCriticalSection();
	
	return retval;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE
VuRedBlackTree::InsertLink(VuLinkNode* link,
                           VU_KEY      key)
{
if(key == 0x90da || key == 0x90db)
{
	int i=0;
	i = 1;
}

	if (link) {
		if (link->entity_) {
			
			if (!filter_->Test(link->entity_)) {
				return VU_NO_OP;
			}
			
			VuRBNode *curNode = root_;
			
			if (root_ == 0) {
				root_ = new VuRBNode(link, key);
				return VU_SUCCESS;
			}
			
			VuEnterCriticalSection();
			while(curNode) {
				if(key < curNode->key_) {
					if(curNode->left_) {
						curNode = curNode->left_;
					} 
					else {
						curNode = new VuRBNode(link, key, curNode, LEFT);
						break;
					}
				} else if (key > curNode->key_) {
					if(curNode->right_) {
						curNode = curNode->right_;
					} 
					else {
						curNode = new VuRBNode(link, key, curNode, RIGHT);
						break;
					}
				} else {
					// insert at head...
					link->next_    = curNode->head_;
					curNode->head_ = link;
					VuExitCriticalSection();
					return VU_SUCCESS;
				}
			}
			
			VU_ERRCODE retval = InsertFixUp(curNode);
			
			VuExitCriticalSection();
			
			return retval;
		}
	}
	
	return VU_ERROR;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE
VuRedBlackTree::ForcedInsert(VuEntity* entity)
{
	if (entity->VuState() == VU_MEM_ACTIVE) {
		if (!filter_->RemoveTest(entity)) {
			return VU_NO_OP;
		}
		
		VuRBNode* curNode = root_;
		VU_KEY    key     = Key(entity);


if(key == 0x90da || key == 0x90db)
{
	int i=0;
	i = 1;
}


		
		if (root_ == 0) {
			root_   = new VuRBNode(entity, key);
			curNode = root_;
			return VU_SUCCESS;
		}
		
		VuEnterCriticalSection();
		
		while(curNode) {
			if(key < curNode->key_) {
				if(curNode->left_) {
					curNode = curNode->left_;
				} 
				else {
					curNode = new VuRBNode(entity, key, curNode, LEFT);
					break;
				}
			} 
			else if (key > curNode->key_) {
				if(curNode->right_) {
					curNode = curNode->right_;
				} 
				else {
					curNode = new VuRBNode(entity, key, curNode, RIGHT);
					break;
				}
			} 
			else {
				VuLinkNode *node = curNode->head_;
				while (node->next_->entity_) {
					node = node->next_;
				}
				node->next_ = new VuLinkNode(entity, vuTailNode);
				VuExitCriticalSection();
				return VU_SUCCESS;
			}
		}
		
		VU_ERRCODE retval = InsertFixUp(curNode);
		
		VuExitCriticalSection();
		
		return retval;
	}
	return VU_ERROR;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE
VuRedBlackTree::Insert(VuEntity* entity)
{
	if (!filter_->Test(entity)) {
		return VU_NO_OP;
	}
	return ForcedInsert(entity);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void
VuRedBlackTree::RotateLeft(VuRBNode* x)
{
	VuRBNode* y = x->right_;
	
	x->right_ = y->left_;
	
	if(y->left_ != NULL) {
		y->left_->parent_ = x;
	}
	
	y->parent_ = x->parent_;
	
	if(x->parent_ == NULL) {
		root_ = y;
	} 
	else {
		if(x == x->parent_->left_) {
			x->parent_->left_  = y;
		} else {
			x->parent_->right_ = y;
		}
	}
	y->left_   = x;
	x->parent_ = y;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void
VuRedBlackTree::RotateRight(VuRBNode* y)
{
	VuRBNode* x;
	
	x        = y->left_;
	y->left_ = x->right_;
	
	if(x->right_ != NULL) {
		x->right_->parent_ = y;
	}
	
	x->parent_ = y->parent_;
	
	if(y->parent_ == NULL) {
		root_ = x;
	} 
	else {
		if(y == y->parent_->right_) {
			y->parent_->right_ = x;
		} 
		else {
			y->parent_->left_ = x;
		}
	}
	
	x->right_  = y;
	y->parent_ = x;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuRBNode*
VuRedBlackTree::RemoveNode(VuRBNode* z)
{
	VuRBNode *x, *y, *prev;
	int ycolor;
	
	bogusNode->parent_ = 0;
	
	/* Fix next_ pointer */
	
	prev = z->Predecessor();
	
	if (prev) prev->next_ = z->next_;
	
	if(z->left_ == NULL || z->right_ == NULL) {
		y = z;
	} else {
		y = z->Successor();
	}
	
	ycolor = y->color_;
	
	if (y->left_ != NULL) {
		x = y->left_;
	} else if (y->right_ != NULL) {
		x = y->right_;
	} else {
		x = bogusNode;
	}
	
	x->parent_ = y->parent_;
	
	if (y->parent_ == NULL) {
		root_ = x;
	} else if (y == y->parent_->left_) {
		y->parent_->left_ = x;
	} else {
		y->parent_->right_ = x;
	}
	
	if (y != z) {
		if (z->parent_ != NULL) {
			if (z == z->parent_->left_) {
				z->parent_->left_ = y;
			} else if(z == z->parent_->right_) {
				z->parent_->right_ = y;
			}
		} else {
			root_ = y;
		}
		y->parent_ = z->parent_;
		y->color_  = z->color_;
		y->left_   = z->left_;
		y->right_  = z->right_;
		
		if(y->left_ != NULL) {
			y->left_->parent_ = y;
		}
		
		if(y->right_ != NULL) {
			y->right_->parent_ = y;
		}
	}
	
	if(ycolor == BLACK) {
		DeleteFixUp(x);
	}
	
	if (root_ == bogusNode) {
		root_ = NULL;  /*  tree is gone  */
	} else if (x == bogusNode) {
		if(x == x->parent_->left_) {
			x->parent_->left_ = NULL;
		} else if(x == x->parent_->right_) {
			x->parent_->right_ = NULL;
		}
		
		if(x->left_ != NULL || x->right_ != NULL) {
			return 0;  /* tree is trashed  */
		}
	}
	return z;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void 
VuRedBlackTree::DeleteFixUp(VuRBNode* x)
{
	VuRBNode* w;
	
	while(x != root_ && x->color_ == BLACK) {
		if(x == x->parent_->left_) {
			w = x->parent_->right_;
			
			if(w) {
				if(w && w->color_ == RED) {
					w->color_ = BLACK;
					x->parent_->color_ = RED;
					RotateLeft(x->parent_);
					w = x->parent_->right_;
				}
				
				if( (!w->left_  || w->left_->color_ == BLACK) &&
					(!w->right_ || w->right_->color_ == BLACK) ) {
					if(w) w->color_ = RED;
					x = x->parent_;
				} else {
					if(!w->right_ || w->right_->color_ == BLACK) {
						if(w->left_) w->left_->color_ = BLACK;
						if(w) w->color_ = RED;
						RotateRight(w);
						w = x->parent_->right_;  /* ok x != root */
					}
					if(w) w->color_ = x->parent_->color_;
					x->parent_->color_ = BLACK;
					if(w->right_) w->right_->color_ = BLACK;
					RotateLeft(x->parent_);
					x = root_;
				}
			}
		} 
		else {
			w = x->parent_->left_;
			if(w) {
				if(w && w->color_ == RED) {
					w->color_ = BLACK;
					x->parent_->color_ = RED;
					RotateRight(x->parent_);
					w = x->parent_->left_;
				}
				
				if( (!w->left_  || w->left_->color_ == BLACK) &&
					(!w->right_ || w->right_->color_ == BLACK) ) {
					if(w) w->color_ = RED;
					x = x->parent_;
				} else {
					if(!w->left_ || w->left_->color_ == BLACK) {
						if(w->right_) w->right_->color_ = BLACK;
						if(w) w->color_ = RED;
						RotateLeft(w);
						w = x->parent_->left_;
					}
					if(w) w->color_ = x->parent_->color_;
					x->parent_->color_ = BLACK;
					if(w->left_) w->left_->color_ = BLACK;
					RotateRight(x->parent_);
					x = root_;
				}
			}
		}
	}
	x->color_ = BLACK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE
VuRedBlackTree::Remove(VuEntity* entity, 
                       VU_BOOL   purge)
{
	VU_KEY key = Key(entity);

if(key == 0x90da || key == 0x90db)
{
	int i=0;
	i = 1;
}
	
	if (root_ == 0) return VU_NO_OP;
	
	if (filter_ && !filter_->RemoveTest(entity)) return VU_NO_OP;
	VuRBNode *rbnode = root_->Find(key);
	VuLinkNode *ptr = 0;
	
	if (rbnode) {
		ptr = rbnode->head_;
		if (ptr == 0 || ptr->entity_ == 0) {
			// fatal error...
			return VU_ERROR;
		}
		if (ptr->freenext_ != 0) {
			// already done
			return VU_NO_OP;
		}
		
		VuEnterCriticalSection();
		VuLinkNode *last = 0;
		while (ptr->entity_ && ptr->entity_ != entity) {
			last = ptr;
			ptr = ptr->next_;
		}
		if (ptr->entity_) {
			if (last) {
				last->next_ = ptr->next_;
			} else {
				rbnode->head_ = ptr->next_;
				if (rbnode->head_ == vuTailNode) {
					RemoveNode(rbnode);
					vuCollectionManager->PutOnKillQueue(rbnode, purge);
				}
			}
			// put curr on VUs pending delete queue
			vuCollectionManager->PutOnKillQueue(ptr, purge);
		}
		VuExitCriticalSection();
	}
	return (ptr ? VU_SUCCESS : VU_NO_OP);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VU_ERRCODE
VuRedBlackTree::Remove(VU_ID entityId)
{
	if (root_ == 0) return 0;
	if (filter_) {
		VuEntity *ent = vuDatabase->Find(entityId);
		if (ent) {
			return Remove(ent);
		} else {
			return VU_NO_OP;
		}
	}
	VU_KEY key = (VU_KEY)entityId;

if(key == 0x90da || key == 0x90db)
{
	int i=0;
	i = 1;
}

	VuRBNode *rbnode = root_->Find(key);
	VuLinkNode *ptr = 0;
	
	if (rbnode) {
		ptr = rbnode->head_;
		if (ptr == 0 || ptr->entity_ == 0) {
			// fatal error...
			return VU_ERROR;
		}
		if (ptr->freenext_ != 0) {
			// already done
			return VU_NO_OP;
		}
		
		VuEnterCriticalSection();
		VuLinkNode *last = 0;
		while (ptr->entity_ && ptr->entity_->Id() != entityId) {
			last = ptr;
			ptr = ptr->next_;
		}
		if (ptr->entity_) {
			if (last) {
				last->next_ = ptr->next_;
			} else {
				rbnode->head_ = ptr->next_;
				if (rbnode->head_ == vuTailNode) {
					RemoveNode(rbnode);
					vuCollectionManager->PutOnKillQueue(rbnode);
				}
			}
			// put curr on VUs pending delete queue
			vuCollectionManager->PutOnKillQueue(ptr);
		}
		VuExitCriticalSection();
	}
	return (ptr ? VU_SUCCESS : VU_NO_OP);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int
VuRedBlackTree::Purge(VU_BOOL all)
{
	int retval = 0;
	
	VuEnterCriticalSection();
	
	if (all) {

		while (root_) {
			retval++;
			if (Remove(root_->head_->entity_, TRUE) == VU_NO_OP) {
			    // JPO if this goes off the VU database has got corrupted
			    ShiWarning("VuRedBlackTree::Purge, failed to remove root id");
#ifdef _DEBUG
			    VU_PRINT ("VuRedBlackTree::Purge, failed to remove root id %d\n",
				root_->head_->entity_->Id());
#endif
			    break; // emergency fix - JPO
			}
		}
	}
	else {
		VuRBIterator iter(this);
		VuEntity* ent = iter.GetFirst();
		while (ent) {
			VuEntity *next = iter.GetNext();
			if (!(ent->IsPrivate() && ent->IsPersistent()) && !ent->IsGlobal()) {
				retval++;
				if (Remove(ent, TRUE) == VU_NO_OP) {
				    ShiWarning ("Purge !all failed to remove id");
#ifdef _DEBUG
				    VU_PRINT ("Purge !all failed to remove id %d\n",
					ent->Id());
#endif
				}
			}
			ent = next;
		}
	}
	VuExitCriticalSection();
	return retval;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int
VuRedBlackTree::Purge(VuLinkNode** repository)
{
	int retval = 0;
	
	VuEnterCriticalSection();
	VuRBNode *cur = root_, *next;
	
	while (cur) {
		if (cur->left_) {
			next = cur->left_;
			cur->left_ = 0;
		} else if (cur->right_) {
			next = cur->right_;
			cur->right_ = 0;
		} else {
			next = cur->parent_;
			while (cur->head_ != vuTailNode) {
				if (cur->head_->freenext_ == 0) { // ensure node isn't on kill queue
					cur->head_->freenext_ = *repository;
					*repository = cur->head_;
					retval++;
				}
				cur->head_ = cur->head_->next_;
			}
			vuCollectionManager->PutOnKillQueue(cur, TRUE);
		}
		cur = next;
	}
	root_ = 0;
	
	VuExitCriticalSection();
	return retval;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int
VuRedBlackTree::Count()
{
	VuRBIterator iter(this);
	int cnt = 0;
	
	for (VuEntity *ent = iter.GetFirst(); ent; ent = iter.GetNext()) {
		cnt++;
	}
	return cnt;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuEntity *
VuRedBlackTree::Find(VU_ID entityId)
{
	VuEntity *ent = vuDatabase->Find(entityId);
	if (ent) {
		return Find(ent);
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuEntity *
VuRedBlackTree::FindByKey(VU_KEY key)
{
	VuRBNode *rbnode = root_->Find(key);
	return (rbnode ? rbnode->head_->entity_ : 0);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuEntity *
VuRedBlackTree::Find(VuEntity* ent)
{
	VuRBNode* rbnode = root_->Find(Key(ent));
	VuEntity* retval = 0;
	
	if (rbnode) {
		VuLinkNode *lnode = rbnode->head_;
		while (lnode && lnode->entity_ && !retval) {
			if (lnode->entity_->Id() == ent->Id()) {
				retval = lnode->entity_;
			}
			lnode = lnode->next_;
		}
	}
	return retval;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int
VuRedBlackTree::Type()
{
	return VU_RED_BLACK_TREE_COLLECTION;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
