// vucoll.h - Copyright (c) Tue July 2 15:01:01 1996,  Spectrum HoloByte, Inc.  All Rights Reserved

#ifndef _VUCOLLECTION_H_
#define _VUCOLLECTION_H_

extern "C"
{
	#include <assert.h>
};

#include "apitypes.h"
#include "vuentity.h"

#ifdef USE_SH_POOLS
extern MEM_POOL gVuFilterMemPool;
#endif

class VuMessage;

// declare the vu database pointer
extern class VuDatabase *vuDatabase;

#define VU_DEFAULT_HASH_KEY	59

#define VU_UNKNOWN_COLLECTION			0x000

#define VU_HASH_TABLE_FAMILY			0x100
#define VU_HASH_TABLE_COLLECTION		0x101
#define VU_FILTERED_HASH_TABLE_COLLECTION	0x102
#define VU_DATABASE_COLLECTION			0x103
#define VU_ANTI_DATABASE_COLLECTION		0x104

#define VU_LINKED_LIST_FAMILY			0x200
#define VU_LINKED_LIST_COLLECTION		0x201
#define VU_FILTERED_LIST_COLLECTION		0x202
#define VU_ORDERED_LIST_COLLECTION		0x203
#define VU_LIFO_QUEUE_COLLECTION		0x204
#define VU_FIFO_QUEUE_COLLECTION		0x205

#define VU_RED_BLACK_TREE_FAMILY		0x400
#define VU_RED_BLACK_TREE_COLLECTION		0x401

#define VU_GRID_TREE_FAMILY			0x800
#define VU_GRID_TREE_COLLECTION			0x801

//-----------------------------------------------------------------------------
// note that VuLinkNode is actually not public, but is declared here because 
// it is needed by inlines (below)
class VuLinkNode {
#ifdef USE_SH_POOLS
   public:
      // Overload new/delete to use a SmartHeap fixed size pool
      void *operator new(size_t size) { assert( size == sizeof(VuLinkNode) ); return MemAllocFS(pool);	};
      void operator delete(void *mem) { if (mem) MemFreeFS(mem); };
      static void InitializeStorage()	{ pool = MemPoolInitFS( sizeof(VuLinkNode), 400, 0 ); };
      static void ReleaseStorage()	{ MemPoolFree( pool ); };
      static MEM_POOL	pool;
#endif
public:
  VuLinkNode(VuEntity *entity, VuLinkNode *next);
  ~VuLinkNode();

// data
  VuLinkNode *freenext_;	// used only for mem management
  VuEntity *entity_;
  VuLinkNode *next_;
};

class VuRBNode;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class VuFilter {
#ifdef USE_SH_POOLS
public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(gVuFilterMemPool,size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
public:
  virtual ~VuFilter();

  virtual int Compare(VuEntity *ent1, VuEntity *ent2) = 0;
  	//  < 0 --> ent1  < ent2
  	// == 0 --> ent1 == ent2
  	//  > 0 --> ent1  > ent2
  virtual VU_BOOL Test(VuEntity *ent) = 0;
  	// TRUE --> ent in sub-set
  	// FALSE --> ent not in sub-set
  virtual VU_BOOL RemoveTest(VuEntity *ent);
  	// TRUE --> ent might be in sub-set
  	// FALSE --> ent could never have been in sub-set
	//   -- default implementation returns True()
  virtual VU_BOOL Notice(VuMessage *event);
  	// TRUE --> event may cause a change to result of Test()
	// FALSE --> event will never cause a change to result of Test()
	//   -- default implementation returns FALSE

  virtual VuFilter *Copy() = 0;		// allocates and returns a copy

protected:
  // base is empty
  VuFilter() {}
  // copy constructor
  VuFilter(VuFilter *) {}

// DATA
 // implementation (and members) empty
 // --> must be provided by sub-classes
};

//-----------------------------------------------------------------------------
class VuKeyFilter : public VuFilter {
public:
  virtual ~VuKeyFilter();

  virtual VU_BOOL Test(VuEntity *ent) = 0;
  virtual int Compare(VuEntity *ent1, VuEntity *ent2);
	//  uses Key()...
  	//  < 0 --> ent1  < ent2
  	// == 0 --> ent1 == ent2
  	//  > 0 --> ent1  > ent2
  virtual VuFilter *Copy() = 0;

  virtual VU_KEY Key(VuEntity *ent);
  	// translates ent into a VU_KEY... used in Compare (above)
	// default implemetation returns id coerced to VU_KEY

protected:
  VuKeyFilter() {}
  VuKeyFilter(VuKeyFilter *) {}

// DATA
  // none
};

//-----------------------------------------------------------------------------
class VuBiKeyFilter : public VuKeyFilter {
public:
  virtual ~VuBiKeyFilter();

  virtual VU_BOOL Test(VuEntity *ent) = 0;
  virtual VuFilter *Copy() = 0;

  virtual VU_KEY Key(VuEntity *ent);
  	// translates ent into a VU_KEY... used in Compare (above)
	// default implemetation calls Key2(ent);
  virtual VU_KEY Key1(VuEntity *ent);
  virtual VU_KEY Key2(VuEntity *ent);
  virtual VU_KEY CoordToKey1(BIG_SCALAR coord) = 0;
  virtual VU_KEY CoordToKey2(BIG_SCALAR coord) = 0;
  virtual VU_KEY Distance1(BIG_SCALAR dist) = 0;
  virtual VU_KEY Distance2(BIG_SCALAR dist) = 0;

protected:
  VuBiKeyFilter() {}
  VuBiKeyFilter(VuBiKeyFilter *) {}

// DATA
  // none
};

//-----------------------------------------------------------------------------
class VuTransmissionFilter : public VuKeyFilter {
public:
  VuTransmissionFilter();
  VuTransmissionFilter(VuTransmissionFilter *other);
  virtual ~VuTransmissionFilter();

  virtual VU_BOOL Test(VuEntity *ent);
  	// TRUE --> ent in sub-set
  	// FALSE --> ent not in sub-set
  virtual int Compare(VuEntity *ent1, VuEntity *ent2);
  	//  < 0 --> ent1  < ent2
  	// == 0 --> ent1 == ent2
  	//  > 0 --> ent1  > ent2
  virtual VuFilter *Copy();

  virtual VU_BOOL Notice(VuMessage *event);

  virtual VU_KEY Key(VuEntity *ent);

protected:

// DATA
  // none
};

//-----------------------------------------------------------------------------
class VuStandardFilter : public VuFilter {
public:
  VuStandardFilter(VuFlagBits mask, VU_TRI_STATE localSession = DONT_CARE);
  VuStandardFilter(ushort mask, VU_TRI_STATE localSession = DONT_CARE);
  virtual ~VuStandardFilter();

  virtual VU_BOOL Test(VuEntity *ent);
  virtual int Compare(VuEntity *ent1, VuEntity *ent2);
  	// returns ent2->Id() - ent1->Id()
  virtual VuFilter *Copy();

  virtual VU_BOOL Notice(VuMessage *event);

protected:
  VuStandardFilter(VuStandardFilter *other);

// DATA
protected:
  union {
    VuFlagBits breakdown_;
    ushort val_;
  } idmask_;
  VU_TRI_STATE localSession_;
};

//-----------------------------------------------------------------------------
class VuAssociationFilter : public VuFilter {
public:
  VuAssociationFilter(VU_ID association);
  virtual ~VuAssociationFilter();

  virtual VU_BOOL Test(VuEntity *ent);
  virtual int Compare(VuEntity *ent1, VuEntity *ent2);
  	// returns ent2->Id() - ent1->Id()
  virtual VuFilter *Copy();

protected:
  VuAssociationFilter(VuAssociationFilter *other);

// DATA
protected:
  VU_ID association_;
};

//-----------------------------------------------------------------------------
//  note: this class assumes immutable types
class VuTypeFilter : public VuFilter {
public:
  VuTypeFilter(ushort type);
  virtual ~VuTypeFilter();

  virtual VU_BOOL Test(VuEntity *ent);
  virtual VU_BOOL RemoveTest(VuEntity *ent);
  virtual int Compare(VuEntity *ent1, VuEntity *ent2);
  	// returns ent2->Id() - ent1->Id()
  virtual VuFilter *Copy();

protected:
  VuTypeFilter(VuTypeFilter *other);

// DATA
protected:
  ushort type_;
};

//-----------------------------------------------------------------------------
class VuOpaqueFilter : public VuFilter {
public:
  VuOpaqueFilter();
  virtual ~VuOpaqueFilter();

  virtual VU_BOOL Test(VuEntity *ent);		// returns FALSE
  virtual int Compare(VuEntity *ent1, VuEntity *ent2);
  	// returns ent2->Id() - ent1->Id()
  virtual VuFilter *Copy();

protected:
  VuOpaqueFilter(VuOpaqueFilter *other);

// DATA
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class VuCollection {

friend class VuCollectionManager;

public:
  virtual ~VuCollection();
  virtual void Init();
  virtual void DeInit();

  virtual VU_ERRCODE Handle(VuMessage *msg);

  virtual VU_ERRCODE Insert(VuEntity *entity) = 0;
  virtual VU_ERRCODE Remove(VuEntity *entity) = 0;
  virtual VU_ERRCODE Remove(VU_ID entityId) = 0;
  virtual int Purge(VU_BOOL all = TRUE) = 0;
  virtual int Count() = 0;
  virtual VuEntity *Find(VU_ID entityId) = 0;
  virtual VuEntity *Find(VuEntity *entity) = 0;
  virtual int Type() = 0;

protected:
  VuCollection();

// DATA
private:
  // for use by VuCollectionManager only!
  VuCollection *nextcoll_;
  VU_BOOL initialized_;
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class VuHashTable : public VuCollection {

friend class VuHashIterator;

public:
  VuHashTable(uint tableSize, uint key = VU_DEFAULT_HASH_KEY);
  virtual ~VuHashTable();

  virtual VU_ERRCODE Insert(VuEntity *entity);
  virtual VU_ERRCODE Remove(VuEntity *entity);
  virtual VU_ERRCODE Remove(VU_ID entityId);
  virtual int Purge(VU_BOOL all = TRUE);
  virtual int Count();
  virtual VuEntity *Find(VU_ID entityId);
  virtual VuEntity *Find(VuEntity *ent);
  virtual int Type();

protected:

// DATA
protected:
  uint count_;
  uint capacity_;
  uint key_;
  VuLinkNode **table_;
};

//-----------------------------------------------------------------------------
class VuFilteredHashTable : public VuHashTable {
public:
  VuFilteredHashTable(VuFilter *filter, uint tableSize,
      				uint key = VU_DEFAULT_HASH_KEY);
  virtual ~VuFilteredHashTable();

  virtual VU_ERRCODE Handle(VuMessage *msg);

  virtual VU_ERRCODE ForcedInsert(VuEntity *entity);
  virtual VU_ERRCODE Insert(VuEntity *entity);

  virtual int Type();

protected:
  VuFilter *filter_;
};

//-----------------------------------------------------------------------------
class VuDatabase : public VuHashTable {
  friend class VuCollectionManager;
  friend class VuMainThread;
public:
  VuDatabase(uint tableSize, uint key = VU_DEFAULT_HASH_KEY);
  virtual ~VuDatabase();

  virtual VU_ERRCODE Handle(VuMessage *msg);

  virtual VU_ERRCODE QuickInsert(VuEntity *entity);
  virtual VU_ERRCODE Insert(VuEntity *entity);
  virtual VU_ERRCODE SilentInsert(VuEntity *entity);    // don't send create
  virtual VU_ERRCODE Remove(VuEntity *entity);
  virtual VU_ERRCODE SilentRemove(VuEntity *entity);
  virtual VU_ERRCODE Remove(VU_ID entityId);
  VU_ERRCODE Save(char *filename,
  	VU_ERRCODE (*headercb)(FILE *), 	  // >= 0 --> continue
	VU_ERRCODE (*savecb)(FILE *, VuEntity *)); // > 0 --> save it
  VU_ERRCODE Restore(char *filename,
  	VU_ERRCODE (*headercb)(FILE *),	  // >= 0 --> continue
	VuEntity * (*restorecb)(FILE *)); // returns pointer to alloc'ed ent
  virtual int Type();

  // to be called by delete event only!
  VU_ERRCODE DeleteRemove(VuEntity *entity);
private:
  virtual int Purge(VU_BOOL all = TRUE);  // purges all from database
  virtual int Suspend(VU_BOOL all = TRUE);  // migrates all to antiDB
};

//-----------------------------------------------------------------------------
class VuAntiDatabase : public VuHashTable {
public:
  VuAntiDatabase(uint tableSize, uint key = VU_DEFAULT_HASH_KEY);
  virtual ~VuAntiDatabase();

  virtual VU_ERRCODE Insert(VuEntity *entity);
  virtual VU_ERRCODE Remove(VuEntity *entity);
  virtual VU_ERRCODE Remove(VU_ID entityId);

  virtual int Type();
  virtual int Purge(VU_BOOL all = TRUE);  // purges all from database
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class VuRedBlackTree : public VuCollection {

friend class VuRBIterator;
friend class VuGridTree;
friend class VuGridIterator;
friend class VuFullGridIterator;
friend class VuLineIterator;

public:
  VuRedBlackTree(VuKeyFilter *filter);
  virtual ~VuRedBlackTree();

  virtual VU_ERRCODE Handle(VuMessage *msg);

  virtual VU_ERRCODE ForcedInsert(VuEntity *entity);
  virtual VU_ERRCODE Insert(VuEntity *entity);
  virtual VU_ERRCODE Remove(VuEntity *entity) { return Remove(entity, FALSE); }
  virtual VU_ERRCODE Remove(VU_ID entityId);
  virtual VU_ERRCODE Purge(VU_BOOL all = TRUE);
  virtual VU_ERRCODE Count();
  virtual VuEntity *Find(VU_ID entityId);
  virtual VuEntity *FindByKey(VU_KEY key);
  virtual VuEntity *Find(VuEntity *ent);

  // for debugging
  void Dump();
  virtual int Type();

protected:
  void Dump(VuRBNode *node, int level);

  VU_ERRCODE Purge(VuLinkNode **repository);
  VuRBNode *RemoveNode(VuRBNode *delNode);
  VU_ERRCODE InsertLink(VuLinkNode *link, VU_KEY key);
  VU_ERRCODE InsertNode(VuRBNode *node);
  VU_ERRCODE InsertFixUp(VuRBNode *newNode);
  void DeleteFixUp(VuRBNode *delNode);
  void RotateLeft(VuRBNode *x);
  void RotateRight(VuRBNode *y);
  VU_KEY Key(VuEntity *ent)
      { return filter_ ? filter_->Key(ent) : (VU_KEY)ent->Id(); }


  // VuGridTree interface
private:
  VuRedBlackTree();
  VU_ERRCODE Remove(VuEntity *entity, VU_BOOL purge);
  void InitFilter(VuKeyFilter *filter);

// DATA
protected:
  VuRBNode *root_;
  VuKeyFilter *filter_;
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class VuGridTree : public VuCollection {

friend class VuGridIterator;
friend class VuFullGridIterator;
friend class VuLineIterator;
friend class VuCollectionManager;

public:
  VuGridTree(VuBiKeyFilter *filter, uint numrows,
      BIG_SCALAR center, BIG_SCALAR radius, VU_BOOL wrap = FALSE);
  virtual ~VuGridTree();

  virtual VU_ERRCODE ForcedInsert(VuEntity *entity);
  virtual VU_ERRCODE Insert(VuEntity *entity);
  virtual VU_ERRCODE Remove(VuEntity *entity);
  virtual VU_ERRCODE Remove(VU_ID entityId);
  virtual VU_ERRCODE Move(VuEntity *entity,BIG_SCALAR coord1,BIG_SCALAR coord2);
  virtual int Purge(VU_BOOL all = TRUE);
  virtual int Count();
  virtual VuEntity *Find(VU_ID entityId);
  virtual VuEntity *FindByKey(VU_KEY key1, VU_KEY key2);
  virtual VuEntity *Find(VuEntity *ent);

  void SuspendUpdates() { suspendUpdates_ = TRUE; }
  void ResumeUpdates() { suspendUpdates_ = FALSE; }
  VU_ERRCODE Rebuild();

  void DebugString(char *buf);
  virtual int Type();

protected:
  VuRedBlackTree *Row(VU_KEY key1);
  int CheckIntegrity(VuRBNode *node, int column);

// DATA
protected:
  VuRedBlackTree *table_;
  VuBiKeyFilter *filter_;
  int rowcount_;
  VU_KEY bottom_;
  VU_KEY top_;
  VU_KEY rowheight_;
  SM_SCALAR invrowheight_;
  VU_BOOL wrap_;
  VU_BOOL suspendUpdates_;
private:
  VuGridTree *nextgrid_;
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class VuLinkedList : public VuCollection {

friend class VuListIterator;
friend class VuListMuckyIterator;
friend class VuSessionsIterator;

public:
  VuLinkedList();
  virtual ~VuLinkedList();

  virtual VU_ERRCODE Insert(VuEntity *entity);
  virtual VU_ERRCODE Remove(VuEntity *entity);
  virtual VU_ERRCODE Remove(VU_ID entityId);
  virtual int Purge(VU_BOOL all = TRUE);
  virtual int Count();
  virtual VuEntity *Find(VU_ID entityId);
  virtual VuEntity *Find(VuEntity *ent);
  virtual int Type();

protected:

// DATA
protected:
  VuLinkNode *head_;
  VuLinkNode *tail_;
};

//-----------------------------------------------------------------------------
class VuFilteredList : public VuLinkedList {
public:
  VuFilteredList(VuFilter *filter);
  virtual ~VuFilteredList();

  virtual VU_ERRCODE Handle(VuMessage *msg);

  virtual VU_ERRCODE ForcedInsert(VuEntity *entity);
  virtual VU_ERRCODE Insert(VuEntity *entity);
  virtual VU_ERRCODE Remove(VuEntity *entity);
  virtual VU_ERRCODE Remove(VU_ID entityId);
  virtual int Type();

protected:
  VuFilter *filter_;
};

//-----------------------------------------------------------------------------
class VuOrderedList : public VuFilteredList {
public:
  VuOrderedList(VuFilter *filter);
  virtual ~VuOrderedList();

  virtual VU_ERRCODE ForcedInsert(VuEntity *entity);
  virtual VU_ERRCODE Insert(VuEntity *entity);
  virtual int Type();
};

//-----------------------------------------------------------------------------
class VuLifoQueue : public VuFilteredList {
public:
  VuLifoQueue(VuFilter *filter);
  virtual ~VuLifoQueue();

  virtual int Type();

  VU_ERRCODE Push(VuEntity *entity) { return ForcedInsert(entity); }
  VuEntity *Peek();
  VuEntity *Pop();
};

//-----------------------------------------------------------------------------
class VuFifoQueue : public VuFilteredList {
public:
  VuFifoQueue(VuFilter *filter);
  virtual ~VuFifoQueue();

  virtual VU_ERRCODE ForcedInsert(VuEntity *entity);
  virtual VU_ERRCODE Insert(VuEntity *entity);
  virtual VU_ERRCODE Remove(VuEntity *entity);
  virtual VU_ERRCODE Remove(VU_ID entityId);
  virtual int Purge(VU_BOOL all = TRUE);
  virtual int Type();

  VU_ERRCODE Push(VuEntity *entity) { return ForcedInsert(entity); }
  VuEntity *Peek();
  VuEntity *Pop();

protected:
  VuLinkNode *last_;
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class VuIterator {

friend class VuCollectionManager;

public:
  VuIterator(VuCollection *coll);
  virtual ~VuIterator();

  virtual VuEntity *CurrEnt() = 0;
  virtual VU_BOOL IsReferenced(VuEntity *ent) = 0;
  virtual VU_ERRCODE Cleanup();

protected:
  VuCollection *collection_;
private:
  // for use by VuCollectionManager only!
  VuIterator *nextiter_;
};

//-----------------------------------------------------------------------------
class VuListIterator : public VuIterator {
public:
  VuListIterator(VuLinkedList *coll);
  virtual ~VuListIterator();

  VuEntity *GetFirst()
    { if (collection_) curr_ = ((VuLinkedList *)collection_)->head_; return curr_->entity_; }
  VuEntity *GetNext() { curr_ = curr_->next_; return curr_->entity_; }
  VuEntity *GetFirst(VuFilter *filter);
  VuEntity *GetNext(VuFilter *filter);
  virtual VuEntity *CurrEnt();

  virtual VU_BOOL IsReferenced(VuEntity *ent);
  virtual VU_ERRCODE Cleanup();

protected:
  VuLinkNode *curr_;
};

//-----------------------------------------------------------------------------
class VuRBIterator : public VuIterator {

friend class VuCollectionManager;

public:
  VuRBIterator(VuRedBlackTree *coll);
  virtual ~VuRBIterator();

  VuEntity *GetFirst();
  VuEntity *GetFirst(VU_KEY min);
  VuEntity *GetNext();
  VuEntity *GetFirst(VuFilter *filter);
  VuEntity *GetNext(VuFilter *filter);
  virtual VuEntity *CurrEnt();

  virtual VU_BOOL IsReferenced(VuEntity *ent);
  virtual VU_ERRCODE Cleanup();

  void RemoveCurrent();

protected:
  VuRBIterator(VuCollection *coll);

protected:
  VuRBNode *curnode_;
  VuLinkNode *curlink_;
  VuRBIterator *rbnextiter_;
};

//-----------------------------------------------------------------------------
class VuGridIterator : public VuRBIterator {

friend class VuCollectionManager;

public:
  VuGridIterator(VuGridTree *coll, VuEntity *origin, BIG_SCALAR radius);
  VuGridIterator(VuGridTree *coll,
      	BIG_SCALAR xPos, BIG_SCALAR yPos, BIG_SCALAR radius);
  virtual ~VuGridIterator();

  // note: these implementations HIDE the RBIterator methods, which
  //       is intended, but some compilers will flag this as a warning
  VuEntity *GetFirst();
  VuEntity *GetNext();
  VuEntity *GetFirst(VuFilter *filter);
  VuEntity *GetNext(VuFilter *filter);

protected:
  VuRedBlackTree *curRB_;
  VuRedBlackTree *key1minRB_;
  VuRedBlackTree *key1maxRB_;
  VU_KEY key1min_;
  VU_KEY key1max_;
  VU_KEY key2min_;
  VU_KEY key2max_;
  VU_KEY key1cur_;
};

//-----------------------------------------------------------------------------
class VuFullGridIterator : public VuRBIterator {

friend class VuCollectionManager;

public:
  VuFullGridIterator(VuGridTree *coll);
  virtual ~VuFullGridIterator();

  // note: these implementations HIDE the RBIterator methods, which
  //       is intended, but some compilers will flag this as a warning
  VuEntity *GetFirst();
  VuEntity *GetNext();
  VuEntity *GetFirst(VuFilter *filter);
  VuEntity *GetNext(VuFilter *filter);

protected:
  VuRedBlackTree *curRB_;
  int currow_;
};

//-----------------------------------------------------------------------------
class VuLineIterator : public VuRBIterator {

friend class VuCollectionManager;

public:
  VuLineIterator(VuGridTree *coll, VuEntity *origin, VuEntity *destination, BIG_SCALAR radius);
  VuLineIterator(VuGridTree *coll,
      	BIG_SCALAR xPos0, BIG_SCALAR yPos0,
      	BIG_SCALAR xPos1, BIG_SCALAR yPos1, BIG_SCALAR radius);
  virtual ~VuLineIterator();

  // note: these implementations HIDE the RBIterator methods, which
  //       is intended, but some compilers will flag this as a warning
  VuEntity *GetFirst();
  VuEntity *GetNext();
  VuEntity *GetFirst(VuFilter *filter);
  VuEntity *GetNext(VuFilter *filter);

protected:
  VuRedBlackTree *curRB_;
  VU_KEY key1min_;
  VU_KEY key1max_;
  VU_KEY key2min_;
  VU_KEY key2max_;
  VU_KEY key1cur_;
  VU_KEY lineA_;
  VU_KEY lineB_;
  VU_KEY lineC_;
};

//-----------------------------------------------------------------------------
class VuHashIterator : public VuIterator {
public:
  VuHashIterator(VuHashTable *coll);
  virtual ~VuHashIterator();

  VuEntity *GetFirst();
  VuEntity *GetNext();
  VuEntity *GetFirst(VuFilter *filter);
  VuEntity *GetNext(VuFilter *filter);
  virtual VuEntity *CurrEnt();

  virtual VU_BOOL IsReferenced(VuEntity *ent);
  virtual VU_ERRCODE Cleanup();

protected:
  VuLinkNode **entry_;
  VuLinkNode *curr_;
};

//-----------------------------------------------------------------------------
class VuDatabaseIterator : public VuHashIterator {
public:
  VuDatabaseIterator();
  virtual ~VuDatabaseIterator();
};


#endif // _VUCOLLECTION_H_
