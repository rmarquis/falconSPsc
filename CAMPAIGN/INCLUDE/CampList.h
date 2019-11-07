/******************************************************************************
*
* VuList managing routines
*
*****************************************************************************/

#ifndef CAMPLIST_H
#define CAMPLIST_H

#include "listadt.h"
#include "F4Vu.h"

// ==================================
// Unit specific filters
// ==================================

class UnitFilter : public VuFilter 
	{
	public:
		uchar		parent;					// Set if parents only
		uchar		real;					// Set if real only
		ushort		host;					// Set if this host only
		uchar		inactive;				// active or inactive units only

	public:
		UnitFilter(uchar p, uchar r, ushort h, uchar a);
		virtual ~UnitFilter(void)			{}

		virtual VU_BOOL Test(VuEntity *ent);	
		virtual VU_BOOL RemoveTest(VuEntity *ent);
		virtual int Compare(VuEntity *ent1, VuEntity *ent2)	{ return (SimCompare (ent1, ent2)); }
		virtual VuFilter *Copy()			{ return new UnitFilter(parent,real,host,inactive); }
	};

class AirUnitFilter : public VuFilter 
	{
	public:
		uchar		parent;					// Set if parents only
		uchar		real;					// Set if real only
		ushort		host;					// Set if this host only

	public:
		AirUnitFilter(uchar p, uchar r, ushort h);
		virtual ~AirUnitFilter(void)		{}

		virtual VU_BOOL Test(VuEntity *ent);	
		virtual VU_BOOL RemoveTest(VuEntity *ent);
		virtual int Compare(VuEntity *ent1, VuEntity *ent2)	{ return (SimCompare (ent1, ent2)); }
		virtual VuFilter *Copy()			{ return new AirUnitFilter(parent,real,host); }
	};

class GroundUnitFilter : public VuFilter 
	{
	public:
		uchar		parent;					// Set if parents only
		uchar		real;					// Set if real only
		ushort		host;					// Set if this host only

	public:
		GroundUnitFilter(uchar p, uchar r, ushort h);
		virtual ~GroundUnitFilter(void)		{}

		virtual VU_BOOL Test(VuEntity *ent);	
		virtual VU_BOOL RemoveTest(VuEntity *ent);
		virtual int Compare(VuEntity *ent1, VuEntity *ent2)	{ return (SimCompare (ent1, ent2)); }
		virtual VuFilter *Copy()			{ return new GroundUnitFilter(parent,real,host); }
	};

class NavalUnitFilter : public VuFilter 
	{
	public:
		uchar		parent;					// Set if parents only
		uchar		real;					// Set if real only
		ushort		host;					// Set if this host only

	public:
		NavalUnitFilter(uchar p, uchar r, ushort h);
		virtual ~NavalUnitFilter(void)		{}

		virtual VU_BOOL Test(VuEntity *ent);	
		virtual VU_BOOL RemoveTest(VuEntity *ent);
		virtual int Compare(VuEntity *ent1, VuEntity *ent2)	{ return (SimCompare (ent1, ent2)); }
		virtual VuFilter *Copy()			{ return new NavalUnitFilter(parent,real,host); }
	};

extern UnitFilter			AllUnitFilter;
extern AirUnitFilter		AllAirFilter;
extern GroundUnitFilter		AllGroundFilter;
extern NavalUnitFilter		AllNavalFilter;
extern UnitFilter			AllParentFilter;
extern UnitFilter			AllRealFilter;
extern UnitFilter			InactiveFilter;

// Unit proximity filters
class UnitProxFilter : public VuBiKeyFilter
	{
	public:
		float	xStep;
		float	yStep;
		uchar	real;					// Set if real only

	public:
		UnitProxFilter(int r);
		UnitProxFilter(UnitProxFilter *other, int r);
		virtual ~UnitProxFilter(void)					{}

		virtual VU_BOOL Test(VuEntity *ent);
		virtual VU_BOOL RemoveTest(VuEntity *ent);
		virtual VuFilter *Copy()						{ return new UnitProxFilter(real); }

#ifdef VU_GRID_TREE_Y_MAJOR
		virtual VU_KEY CoordToKey1(BIG_SCALAR coord)	{ return (VU_KEY)(yStep * coord); }
		virtual VU_KEY CoordToKey2(BIG_SCALAR coord)	{ return (VU_KEY)(xStep * coord); }
		virtual VU_KEY Distance1(BIG_SCALAR dist)		{ return (VU_KEY)(yStep * dist); }
		virtual VU_KEY Distance2(BIG_SCALAR dist)		{ return (VU_KEY)(xStep * dist); }
#else
		virtual VU_KEY CoordToKey1(BIG_SCALAR coord)	{ return (VU_KEY)(xStep * coord); }
		virtual VU_KEY CoordToKey2(BIG_SCALAR coord)	{ return (VU_KEY)(yStep * coord); }
		virtual VU_KEY Distance1(BIG_SCALAR dist)		{ return (VU_KEY)(xStep * dist); }
		virtual VU_KEY Distance2(BIG_SCALAR dist)		{ return (VU_KEY)(yStep * dist); }
#endif
	};

extern UnitProxFilter*	AllUnitProxFilter;
extern UnitProxFilter*	RealUnitProxFilter;

// ==============================
// Manager Filters
// ==============================

//extern UnitFilter  AllTMFilter;
//extern UnitFilter  MyTMFilter;

// ==============================
// Objective specific filters
// ==============================

// Standard Objective filter
class ObjFilter : public VuFilter 
	{
	public:
		ushort		host;					// Set if this host only

	public:
		ObjFilter(ushort h);
		virtual ~ObjFilter(void)			{}

		virtual VU_BOOL Test(VuEntity *ent);	
		virtual VU_BOOL RemoveTest(VuEntity *ent);
		virtual int Compare(VuEntity *ent1, VuEntity *ent2)	{ return (SimCompare (ent1, ent2)); }
		virtual VuFilter *Copy()			{ return new ObjFilter(host); }
	};

extern ObjFilter  AllObjFilter;

// Objective Proximity filter
class ObjProxFilter : public VuBiKeyFilter
	{
	public:
	  float xStep;
	  float yStep;

	public:
		ObjProxFilter(void);
		ObjProxFilter(ObjProxFilter *other);
		virtual ~ObjProxFilter(void)					{}

		virtual VU_BOOL Test(VuEntity *ent);
		virtual VU_BOOL RemoveTest(VuEntity *ent);
		virtual VuFilter *Copy()						{ return new ObjProxFilter(); }

#ifdef VU_GRID_TREE_Y_MAJOR
		virtual VU_KEY CoordToKey1(BIG_SCALAR coord)	{ return (VU_KEY)(yStep * coord); }
		virtual VU_KEY CoordToKey2(BIG_SCALAR coord)	{ return (VU_KEY)(xStep * coord); }
		virtual VU_KEY Distance1(BIG_SCALAR dist)		{ return (VU_KEY)(yStep * dist); }
		virtual VU_KEY Distance2(BIG_SCALAR dist)		{ return (VU_KEY)(xStep * dist); }
#else
		virtual VU_KEY CoordToKey1(BIG_SCALAR coord)	{ return (VU_KEY)(xStep * coord); }
		virtual VU_KEY CoordToKey2(BIG_SCALAR coord)	{ return (VU_KEY)(yStep * coord); }
		virtual VU_KEY Distance1(BIG_SCALAR dist)		{ return (VU_KEY)(xStep * dist); }
		virtual VU_KEY Distance2(BIG_SCALAR dist)		{ return (VU_KEY)(yStep * dist); }
#endif
	};

extern ObjProxFilter* AllObjProxFilter;

// ==============================
// General Filters
// ==============================

class CampBaseFilter : public VuFilter 
	{
	public:

	public:
		CampBaseFilter(void)				{}
		virtual ~CampBaseFilter(void)		{}

		virtual VU_BOOL Test(VuEntity *ent);
		virtual VU_BOOL RemoveTest(VuEntity *ent);
		virtual int Compare(VuEntity *ent1, VuEntity *ent2)	{ return (SimCompare (ent1, ent2)); }
		virtual VuFilter *Copy()			{ return new CampBaseFilter(); }
	};

extern CampBaseFilter CampFilter;

// ==============================
// Registered Lists
// ==============================

extern VuFilteredList *AllUnitList;		// All units
extern VuFilteredList *AllAirList;		// All air units
extern VuFilteredList *AllRealList;		// All ground units
extern VuFilteredList *AllParentList;	// All parent units
extern VuFilteredList *AllObjList;		// All objectives
extern VuFilteredList *AllCampList;		// All campaign entities
extern VuFilteredList *InactiveList;	// Inactive units (reinforcements)

// ==============================
// Maintained Lists
// ==============================

#define MAX_DIRTY_BUCKETS	9 //me123 from 8

extern F4PFList FrontList;				// Frontline objectives
extern F4POList POList;					// Primary objective list
extern F4PFList SOList;					// Secondary objective list
extern F4PFList AirDefenseList;			// All air defenses
extern F4PFList EmitterList;			// All emitters
extern F4PFList DeaggregateList;		// All deaggregated entities
extern TailInsertList *DirtyBucket[MAX_DIRTY_BUCKETS];	// Dirty entities

// ==============================
// Objective data lists
// ==============================

extern List PODataList;

// Front Line List
extern List FLOTList;

// ==============================
// Proximity Lists
// ==============================

extern VuGridTree* ObjProxList;			// Proximity list of all objectives
extern VuGridTree* RealUnitProxList;	// Proximity list of all real units

// ==============================
// Global Iterators
// ==============================

// ==============================
// List maintenance routines
// ==============================

extern void InitLists (void);

extern void InitProximityLists (void);

extern void InitIALists (void);

extern void DisposeLists (void);

extern void DisposeProxLists (void);

extern void DisposeIALists (void);

extern int RebuildFrontList (int do_barcaps, int incremental);

extern int RebuildObjectiveLists(void);

extern int RebuildParentsList (void);

extern int RebuildEmitterList (void);

extern void StandardRebuild (void);

#endif
