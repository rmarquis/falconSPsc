///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include "vu2.h"
#include "vu_priv.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuRBNode::VuRBNode(VuLinkNode* link, VU_KEY key)
: parent_(0),
left_(0),
right_(0),
next_(0),
key_(key),
color_(BLACK)
{
	link->next_ = vuTailNode;
	head_ = link;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuRBNode::VuRBNode(VuEntity* ent, VU_KEY key)
: parent_(0),
left_(0),
right_(0),
next_(0),
key_(key),
color_(BLACK)
{
	head_ = new VuLinkNode(ent, vuTailNode);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuRBNode::VuRBNode(VuEntity* ent, 
                   VU_KEY    key, 
                   VuRBNode* parent,
                   int       side)
				   : parent_(parent),
				   left_(0),
				   right_(0),
				   next_(0),
				   key_(key),
				   color_(RED)
{
	head_ = new VuLinkNode(ent, vuTailNode);
	
	if (side == LEFT)
		parent->left_  = this;
	else
		parent->right_ = this;
	
	next_ = SuccessorViaWalk();
	VuRBNode* prevNode = Predecessor();
	if ( prevNode ) prevNode->next_ = this;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuRBNode::VuRBNode(VuLinkNode* link, VU_KEY key, VuRBNode* parent, int side)
: parent_(parent),
left_(0),
right_(0),
next_(0),
key_(key),
color_(RED)
{
	link->next_ = vuTailNode;
	head_       = link;
	
	if (side == LEFT)
		parent->left_ = this;
	else
		parent->right_ = this;
	
	next_ = SuccessorViaWalk();
	VuRBNode* prevNode = Predecessor();
	if ( prevNode ) prevNode->next_ = this;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuRBNode::~VuRBNode()
{
	// empty
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuRBNode *
VuRBNode::SuccessorViaWalk()
{
	if (right_ != NULL) {
		return right_->TreeMinimum();
	}
	else {
		VuRBNode *x = this;
		VuRBNode *y = parent_;
		
		while(y != NULL && x == y->right_) {
			x = y;
			y = y->parent_;
		}
		return y;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuRBNode *
VuRBNode::Predecessor()
{
	if(left_) {
		return left_->TreeMaximum();
	}
	else {
		VuRBNode *x = this;
		
		while(x->parent_) {
			if(x == x->parent_->right_) {
				return x->parent_;
			}
			x = x->parent_;
		}
		return NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuRBNode *
VuRBNode::Find(VU_KEY key)
{
	VuRBNode* x = this;
	
	while(x != NULL && key != x->key_) {
		if(key < x->key_)
			x = x->left_;
		else
			x = x->right_;
	}
	return x;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuRBNode *
VuRBNode::TreeMinimum()
{
	VuRBNode* x = this;
	while(x->left_ != NULL)
		x = x->left_;
	
	return x;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuRBNode *
VuRBNode::TreeMaximum()
{
	VuRBNode* x = this;
	while(x->right_ != NULL)
		x = x->right_;
	
	return x;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuRBNode*
VuRBNode::LowerBound(VU_KEY key)
{
	VuRBNode* x = this;
	VuRBNode* retval = 0;
	
	if (x->key_ == key) return x;
	
	while(x != NULL && key != x->key_) {
		if(key < x->key_) {
			retval = x;
			if (x->left_ == NULL)
				return x;
			x = x->left_;
		} 
		else {
			if (x->right_ == NULL)
				return retval;
			x = x->right_;
		}
	}
	return retval;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VuRBNode *
VuRBNode::UpperBound(VU_KEY key)
{
	VuRBNode* x = this;
	VuRBNode* retval = 0;
	
	if (x->key_ == key) return x;
	
	while(x != NULL && key != x->key_) {
		if(key < x->key_) {
			if (x->left_ == NULL)
				return retval;
			x = x->left_;
		} else {
			retval = x;
			if (x->right_ == NULL)
				return x;
			x = x->right_;
		}
	}
	return retval;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
