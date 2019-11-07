// vu.h - Copyright (c) Mon Aug 5 15:01:01 1996,  Spectrum HoloByte, Inc.  All Rights Reserved

#ifndef _VU_PRIVATE_H_
#define _VU_PRIVATE_H_

#include "apitypes.h"
#include "vucoll.h"

// forward decls
class VuEntity;

extern VuCollectionManager *vuCollectionManager;
extern VuAntiDatabase *vuAntiDB;
#ifdef USE_SH_POOLS
extern MEM_POOL	vuRBNodepool;
#endif

extern VuLinkNode *vuTailNode;
extern VuRBNode *bogusNode;

//-----------------------------------------------------------------------------
class VuRBNode {
#ifdef USE_SH_POOLS
   public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { assert( size == sizeof(VuRBNode) ); return MemAllocFS(vuRBNodepool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ vuRBNodepool = MemPoolInitFS( sizeof(VuRBNode), 400, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( vuRBNodepool ); };
#endif
public:
  VuRBNode(VuLinkNode *link, VU_KEY key); // root
  VuRBNode(VuEntity *ent, VU_KEY key); // root
  VuRBNode(VuLinkNode *link, VU_KEY key, VuRBNode *parent, int side);
  VuRBNode(VuEntity *ent, VU_KEY key, VuRBNode *parent, int side);
  ~VuRBNode();

  VuRBNode *Successor() { return next_; }
  VuRBNode *SuccessorViaWalk();
  VuRBNode *Predecessor();
  VuRBNode *TreeMinimum();
  VuRBNode *TreeMaximum();
  VuRBNode *UpperBound(VU_KEY key);
  VuRBNode *LowerBound(VU_KEY key);
  VuRBNode *Find(VU_KEY key);

// data
  VuRBNode*   parent_;
  VuRBNode*   left_;
  VuRBNode*   right_;
  VuRBNode*   next_;
  VuLinkNode* head_;
  VU_KEY      key_;
  uchar  color_;
};

//-----------------------------------------------------------------------------
class VuCollectionManager {
public:
  VuCollectionManager();
  ~VuCollectionManager();

  void Register(VuIterator *iter);
  void DeRegister(VuIterator *iter);

  void RBRegister(VuRBIterator *iter);
  void RBDeRegister(VuRBIterator *iter);

  void Register(VuCollection *coll);
  void DeRegister(VuCollection *coll);

  void GridRegister(VuGridTree *grid);
  void GridDeRegister(VuGridTree *grid);

  void Add(VuEntity *ent);
  void Remove(VuEntity *ent);
  int HandleMove(VuEntity *ent, BIG_SCALAR coord1, BIG_SCALAR coord2);

  VU_ERRCODE Handle(VuMessage *msg);

  VU_BOOL IsReferenced(VuEntity *ent);
  VU_BOOL IsReferenced(VuRBNode *node);
  void Purge();
  void PutOnKillQueue(VuLinkNode *node, VU_BOOL killnow = FALSE);
  void PutOnKillQueue(VuRBNode *node, VU_BOOL killnow = FALSE);
  void Shutdown(VU_BOOL all);
  void SanitizeKillQueue();

  int FindEnt(VuEntity *ent);

  VuCollection *collcoll_;	// ;-)
  VuGridTree *gridcoll_;
  VuIterator *itercoll_;
  VuRBIterator *rbitercoll_;
  VuLinkNode *llkillhead_;
  VuRBNode *rbkillhead_;
};

//-----------------------------------------------------------------------------
class VuListMuckyIterator : public VuIterator {
public:
  VuListMuckyIterator(VuLinkedList *coll);
  virtual ~VuListMuckyIterator();

  VuEntity *GetFirst()
  	{ last_ = 0; curr_ = ((VuLinkedList *)collection_)->head_;
	  return curr_->entity_; }
  VuEntity *GetNext()
  	{ last_ = curr_; curr_ = curr_->next_; return curr_->entity_; }
  virtual void InsertCurrent(VuEntity *ent);
  virtual void RemoveCurrent();
  virtual VU_BOOL IsReferenced(VuEntity *ent);
  virtual VuEntity *CurrEnt();

  virtual VU_ERRCODE Cleanup();

  VuLinkNode *curr_;
  VuLinkNode *last_;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define RED    1
#define BLACK  2

#define LEFT   1
#define RIGHT  2

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#endif // _VU_PRIVATE_H_
