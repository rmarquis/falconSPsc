/******************************************************************************
/*
/* CdbList managing routines
/*
/*****************************************************************************/

#include "cmpglobl.h"
#include "F4Vu.h"
#include "objectiv.h"
#include "listadt.h"
#include "CampList.h"
#include "gtmobj.h"
#include "team.h"
#include "falcgame.h"
#include "CampBase.h"
#include "Campaign.h"
#include "Find.h"
#include "AIInput.h"
#include "CmpClass.h"
#include "CampMap.h"
#include "classtbl.h"
#include "FalcSess.h"

// ==================================
// externals
// ==================================

extern void MarkObjectives (void);

// ==================================
// Unit specific filters
// ==================================

UnitFilter::UnitFilter(uchar p, uchar r, ushort h, uchar a)
	{
	parent = p;
	real = r;
	host = h;
	inactive = a;
	}

VU_BOOL UnitFilter::Test(VuEntity *e)
	{
	if (!(e->EntityType())->classInfo_[VU_DOMAIN] || (e->EntityType())->classInfo_[VU_CLASS] != CLASS_UNIT)
		return FALSE;
	if (parent && !((Unit)e)->Parent())
		return FALSE;
	if (real && !Real((e->EntityType())->classInfo_[VU_TYPE]))
		return FALSE;
	if (host && !e->IsLocal())
		return FALSE;
	if (!inactive && ((Unit)e)->Inactive())
		return FALSE;
	else if (inactive && !((Unit)e)->Inactive())
		return FALSE;
	return TRUE;
	}

VU_BOOL UnitFilter::RemoveTest(VuEntity *e)
	{
	if (!(e->EntityType())->classInfo_[VU_DOMAIN] || (e->EntityType())->classInfo_[VU_CLASS] != CLASS_UNIT)
		return FALSE;
	if (parent && !((Unit)e)->Parent())
		return FALSE;
	if (real && !Real((e->EntityType())->classInfo_[VU_TYPE]))
		return FALSE;
	if (host && !e->IsLocal())
		return FALSE;
	if (!inactive && ((Unit)e)->Inactive())
		return FALSE;
	else if (inactive && !((Unit)e)->Inactive())
		return FALSE;
	return TRUE;
	}

AirUnitFilter::AirUnitFilter(uchar p, uchar r, ushort h)
	{
	parent = p;
	real = r;
	host = h;
	}

VU_BOOL AirUnitFilter::Test(VuEntity *e)
	{
	if (!(e->EntityType())->classInfo_[VU_DOMAIN] || (e->EntityType())->classInfo_[VU_CLASS] != CLASS_UNIT)
		return FALSE;
	if (((Unit)e)->GetDomain() != DOMAIN_AIR)
		return FALSE;
	if (parent && !((Unit)e)->Parent())
		return FALSE;
	if (real && !Real((e->EntityType())->classInfo_[VU_TYPE]))
		return FALSE;
	if (host && !e->IsLocal())
		return FALSE;
	if (((Unit)e)->Inactive())
		return FALSE;
	return TRUE;
	}

VU_BOOL AirUnitFilter::RemoveTest(VuEntity *e)
	{
	if (!(e->EntityType())->classInfo_[VU_DOMAIN] || (e->EntityType())->classInfo_[VU_CLASS] != CLASS_UNIT)
		return FALSE;
	if (((Unit)e)->GetDomain() != DOMAIN_AIR)
		return FALSE;
	if (parent && !((Unit)e)->Parent())
		return FALSE;
	if (real && !Real((e->EntityType())->classInfo_[VU_TYPE]))
		return FALSE;
	if (host && !e->IsLocal())
		return FALSE;
	if (((Unit)e)->Inactive())
		return FALSE;
	return TRUE;
	}

GroundUnitFilter::GroundUnitFilter(uchar p, uchar r, ushort h)
	{
	parent = p;
	real = r;
	host = h;
	}

VU_BOOL GroundUnitFilter::Test(VuEntity *e)
	{
	if (!(e->EntityType())->classInfo_[VU_DOMAIN] || (e->EntityType())->classInfo_[VU_CLASS] != CLASS_UNIT)
		return FALSE;
	if (((Unit)e)->GetDomain() != DOMAIN_LAND)
		return FALSE;
	if (parent && !((Unit)e)->Parent())
		return FALSE;
	if (real && !Real((e->EntityType())->classInfo_[VU_TYPE]))
		return FALSE;
	if (host && !e->IsLocal())
		return FALSE;
	if (((Unit)e)->Inactive())
		return FALSE;
	return TRUE;
	}

VU_BOOL GroundUnitFilter::RemoveTest(VuEntity *e)
	{
	if (!(e->EntityType())->classInfo_[VU_DOMAIN] || (e->EntityType())->classInfo_[VU_CLASS] != CLASS_UNIT)
		return FALSE;
	if (((Unit)e)->GetDomain() != DOMAIN_LAND)
		return FALSE;
	if (parent && !((Unit)e)->Parent())
		return FALSE;
	if (real && !Real((e->EntityType())->classInfo_[VU_TYPE]))
		return FALSE;
	if (host && !e->IsLocal())
		return FALSE;
	if (((Unit)e)->Inactive())
		return FALSE;
	return TRUE;
	}

NavalUnitFilter::NavalUnitFilter(uchar p, uchar r, ushort h)
	{
	parent = p;
	real = r;
	host = h;
	}

VU_BOOL NavalUnitFilter::Test(VuEntity *e)
	{
	if (!(e->EntityType())->classInfo_[VU_DOMAIN] || (e->EntityType())->classInfo_[VU_CLASS] != CLASS_UNIT)
		return FALSE;
	if (((Unit)e)->GetDomain() != DOMAIN_SEA)
		return FALSE;
	if (parent && !((Unit)e)->Parent())
		return FALSE;
	if (real && !Real((e->EntityType())->classInfo_[VU_TYPE]))
		return FALSE;
	if (host && !e->IsLocal())
		return FALSE;
	if (((Unit)e)->Inactive())
		return FALSE;
	return TRUE;
	}

VU_BOOL NavalUnitFilter::RemoveTest(VuEntity *e)
	{
	if (!(e->EntityType())->classInfo_[VU_DOMAIN] || (e->EntityType())->classInfo_[VU_CLASS] != CLASS_UNIT)
		return FALSE;
	if (((Unit)e)->GetDomain() != DOMAIN_SEA)
		return FALSE;
	if (parent && !((Unit)e)->Parent())
		return FALSE;
	if (real && !Real((e->EntityType())->classInfo_[VU_TYPE]))
		return FALSE;
	if (host && !e->IsLocal())
		return FALSE;
	if (((Unit)e)->Inactive())
		return FALSE;
	return TRUE;
	}

UnitFilter			AllUnitFilter(0,0,0,0);
AirUnitFilter		AllAirFilter(0,0,0);
GroundUnitFilter	AllGroundFilter(0,0,0);
NavalUnitFilter		AllNavalFilter(0,0,0);
UnitFilter			AllParentFilter(TRUE,0,0,0);
UnitFilter			AllRealFilter(0,TRUE,0,0);
UnitFilter			InactiveFilter(0,0,0,1);
VuOpaqueFilter		AllOpaqueFilter;

// Proximity Unit Filter
UnitProxFilter::UnitProxFilter(int r) : VuBiKeyFilter()
	{
	// KCK NOTE: Using this max will cause errors, since we'll
	// have more percision than the sim coordinates we're deriving from
//	VU_KEY	max = ~0;
	VU_KEY	max = 0xFFFF;
	xStep = (float) (max / (GRID_SIZE_FT*(Map_Max_Y+1)));
	yStep = (float) (max / (GRID_SIZE_FT*(Map_Max_X+1)));
	real = (uchar) r;
	}

UnitProxFilter::UnitProxFilter(UnitProxFilter *other, int r) : VuBiKeyFilter(other)
	{
	// KCK NOTE: Using this max will cause errors, since we'll
	// have more percision than the sim coordinates we're deriving from
//	VU_KEY	max = ~0;
	VU_KEY	max = 0xFFFF;
	xStep = (float) (max / (GRID_SIZE_FT*(Map_Max_Y+1)));
	yStep = (float) (max / (GRID_SIZE_FT*(Map_Max_X+1)));
	real = (uchar) r;
	}

VU_BOOL UnitProxFilter::Test(VuEntity *ent)
	{
	if (!(ent->EntityType())->classInfo_[VU_DOMAIN] || (ent->EntityType())->classInfo_[VU_CLASS] != CLASS_UNIT)
		return FALSE;
	if (real && !Real((ent->EntityType())->classInfo_[VU_TYPE]))
		return FALSE;
	if (((Unit)ent)->Inactive())
		return FALSE;
	return TRUE;
	}

VU_BOOL UnitProxFilter::RemoveTest(VuEntity *ent)
	{
	if (!(ent->EntityType())->classInfo_[VU_DOMAIN] || (ent->EntityType())->classInfo_[VU_CLASS] != CLASS_UNIT)
		return FALSE;
	if (real && !Real((ent->EntityType())->classInfo_[VU_TYPE]))
		return FALSE;
	return TRUE;
	}

/*
VU_KEY UnitProxFilter::Key(VuEntity *ent)
	{
#ifdef VU_GRID_TREE_Y_MAJOR
	return CoordToKey2(ent->XPos());
#else
	return CoordToKey2(ent->YPos());
#endif
	}

VU_KEY UnitProxFilter::Key1(VuEntity *ent)
	{
#ifdef VU_GRID_TREE_Y_MAJOR
	return CoordToKey1(ent->YPos());
#else
	return CoordToKey1(ent->XPos());
#endif
	}

VU_KEY UnitProxFilter::Key2(VuEntity *ent)
	{
#ifdef VU_GRID_TREE_Y_MAJOR
	return CoordToKey2(ent->XPos());
#else
	return CoordToKey2(ent->YPos());
#endif
	}
*/

UnitProxFilter*	AllUnitProxFilter = NULL;
UnitProxFilter*	RealUnitProxFilter = NULL;

// ==============================
// Objective specific filters
// ==============================

// Standard Objective Filter
ObjFilter::ObjFilter(ushort h)
	{
	host = h;
	}

VU_BOOL ObjFilter::Test(VuEntity *e)
	{
	if (!(e->EntityType())->classInfo_[VU_DOMAIN] || (e->EntityType())->classInfo_[VU_CLASS] != CLASS_OBJECTIVE)
		return FALSE;
	if (host && !e->IsLocal())
		return FALSE;
	return TRUE;
	}

VU_BOOL ObjFilter::RemoveTest(VuEntity *e)
	{
	if (!(e->EntityType())->classInfo_[VU_DOMAIN] || (e->EntityType())->classInfo_[VU_CLASS] != CLASS_OBJECTIVE)
		return FALSE;
	if (host && !e->IsLocal())
		return FALSE;
	return TRUE;
	}

ObjFilter		AllObjFilter(0);

// Proximity Objective Filter
ObjProxFilter::ObjProxFilter(void) : VuBiKeyFilter()
	{
	// KCK NOTE: Using this max will cause errors, since we'll
	// have more percision than the sim coordinates we're deriving from
//	VU_KEY	max = ~0;
	VU_KEY	max = 0xFFFF;
	xStep = (float) (max / (GRID_SIZE_FT*(Map_Max_Y+1)));
	yStep = (float) (max / (GRID_SIZE_FT*(Map_Max_X+1)));
	}

ObjProxFilter::ObjProxFilter(ObjProxFilter *other) : VuBiKeyFilter(other)
	{
	// KCK NOTE: Using this max will cause errors, since we'll
	// have more percision than the sim coordinates we're deriving from
//	VU_KEY	max = ~0;
	VU_KEY	max = 0xFFFF;
	xStep = (float) (max / (GRID_SIZE_FT*(Map_Max_Y+1)));
	yStep = (float) (max / (GRID_SIZE_FT*(Map_Max_X+1)));
	}

VU_BOOL ObjProxFilter::Test(VuEntity *ent)
	{
	if ((ent->EntityType())->classInfo_[VU_DOMAIN] && (ent->EntityType())->classInfo_[VU_CLASS] != CLASS_OBJECTIVE)
		return FALSE;
	return TRUE;
	}

VU_BOOL ObjProxFilter::RemoveTest(VuEntity *ent)
	{
	if ((ent->EntityType())->classInfo_[VU_DOMAIN] && (ent->EntityType())->classInfo_[VU_CLASS] != CLASS_OBJECTIVE)
		return FALSE;
	return TRUE;
	}

/*
VU_KEY ObjProxFilter::Key(VuEntity *ent)
	{
#ifdef VU_GRID_TREE_Y_MAJOR
	return CoordToKey2(ent->XPos());
#else
	return CoordToKey2(ent->YPos());
#endif
	}

VU_KEY ObjProxFilter::Key1(VuEntity *ent)
	{
#ifdef VU_GRID_TREE_Y_MAJOR
	return CoordToKey1(ent->YPos());
#else
	return CoordToKey1(ent->XPos());
#endif
	}

VU_KEY ObjProxFilter::Key2(VuEntity *ent)
	{
#ifdef VU_GRID_TREE_Y_MAJOR
	return CoordToKey2(ent->XPos());
#else
	return CoordToKey2(ent->YPos());
#endif
	}
*/

ObjProxFilter*	AllObjProxFilter = NULL;

// ==============================
// General Filters
// ==============================

VU_BOOL CampBaseFilter::Test(VuEntity *e)
	{
	if ((e->EntityType())->classInfo_[VU_DOMAIN] && 
		((e->EntityType())->classInfo_[VU_CLASS] == CLASS_UNIT || 
		 (e->EntityType())->classInfo_[VU_CLASS] == CLASS_OBJECTIVE))
		return TRUE;
	return FALSE;
	}

VU_BOOL CampBaseFilter::RemoveTest(VuEntity *e)
	{
	if ((e->EntityType())->classInfo_[VU_DOMAIN] && 
		((e->EntityType())->classInfo_[VU_CLASS] == CLASS_UNIT || 
		 (e->EntityType())->classInfo_[VU_CLASS] == CLASS_OBJECTIVE))
		return TRUE;
	return FALSE;
	}

CampBaseFilter CampFilter;

// ==============================
// Registered Collections
// ==============================

VuFilteredList* AllUnitList = NULL;		// All units
VuFilteredList* AllAirList = NULL;		// All air units
VuFilteredList* AllParentList = NULL;	// All parent units
VuFilteredList* AllRealList = NULL;		// All real units
VuFilteredList* AllObjList = NULL;		// All objectives
VuFilteredList* AllCampList = NULL;		// All campaign entities
VuFilteredList* InactiveList = NULL;	// Inactive units (reinforcements)

// ==============================
// Maintained Collections
// ==============================

F4PFList FrontList = NULL;				// Frontline objectives
F4POList POList = NULL;					// Primary objective list
F4PFList SOList = NULL;					// Secondary objective list
F4PFList AirDefenseList = NULL;			// All air defenses
F4PFList EmitterList = NULL;			// All emitters
F4PFList DeaggregateList = NULL;		// All deaggregated entities
TailInsertList *DirtyBucket[MAX_DIRTY_BUCKETS] = {NULL};		// Dirty entities
																			
// ==============================
// Objective data lists
// ==============================

List PODataList = NULL;
List FLOTList = NULL;					// A List of PackXY points defining the Forward Line Of Troops.

// ==============================
// Proximity Lists
// ==============================

VuGridTree* ObjProxList = NULL;			// Proximity list of all objectives
VuGridTree* RealUnitProxList = NULL;	// Proximity list of all units

// ==============================
// List maintenance routines
// ==============================

// All versions of campaign use these lists
void InitBaseLists (void)
	{
	AllCampList = new VuFilteredList (&CampFilter);			
	AllCampList->Init();
	DeaggregateList = new FalconPrivateList (&CampFilter);
	DeaggregateList->Init();
	for (int loop = 0; loop < MAX_DIRTY_BUCKETS; loop ++)
	{
		DirtyBucket[loop] = new TailInsertList (&AllOpaqueFilter);
		DirtyBucket[loop]->Init();
	}
	EmitterList = new FalconPrivateList(&CampFilter);
	EmitterList->Init();
	}

void InitCampaignLists (void)
	{
	AllUnitList = new VuFilteredList (&AllUnitFilter);		
	AllUnitList->Init();
	AllAirList = new VuFilteredList (&AllAirFilter);			
	AllAirList->Init();
	AllParentList = new VuFilteredList (&AllParentFilter);	
	AllParentList->Init();
	AllRealList = new VuFilteredList (&AllRealFilter);		
	AllRealList->Init();
	AllObjList = new VuFilteredList (&AllObjFilter);			
	AllObjList->Init();
	InactiveList = new VuFilteredList (&InactiveFilter);			
	InactiveList->Init();
	FrontList = new FalconPrivateList(&AllObjFilter);
	FrontList->Init();
	AirDefenseList = new FalconPrivateList(&CampFilter);
	AirDefenseList->Init();
	POList = new FalconPrivateOrderedList(&AllObjFilter);		
	POList->Init();
	SOList = new FalconPrivateList(&AllObjFilter);
	SOList->Init();
	if (PODataList == NULL)
		PODataList = new ListClass();
	else
		PODataList->Purge();
	if (FLOTList == NULL)
		FLOTList = new ListClass(LADT_SORTED_LIST);
	else
		FLOTList->Purge();
	RealUnitProxFilter = new UnitProxFilter(1);
#ifdef VU_GRID_TREE_Y_MAJOR
	RealUnitProxList = new VuGridTree(RealUnitProxFilter, 100, (BIG_SCALAR)(GRID_SIZE_FT*Map_Max_X/2), (BIG_SCALAR)(GRID_SIZE_FT*Map_Max_X/2), FALSE);
#else
	RealUnitProxList = new VuGridTree(RealUnitProxFilter, 100, (BIG_SCALAR)(GRID_SIZE_FT*Map_Max_Y/2), (BIG_SCALAR)(GRID_SIZE_FT*Map_Max_Y/2), FALSE);
#endif
	RealUnitProxList->Init();
	}

void InitTheaterLists (void)
	{
	AllObjProxFilter = new ObjProxFilter();
#ifdef VU_GRID_TREE_Y_MAJOR
	ObjProxList = new VuGridTree(AllObjProxFilter, 100, (BIG_SCALAR)(GRID_SIZE_FT*Map_Max_X/2), (BIG_SCALAR)(GRID_SIZE_FT*Map_Max_X/2), FALSE);
#else
	ObjProxList = new VuGridTree(AllObjProxFilter, 100, (BIG_SCALAR)(GRID_SIZE_FT*Map_Max_Y/2), (BIG_SCALAR)(GRID_SIZE_FT*Map_Max_Y/2), FALSE);
#endif
	ObjProxList->Init();
	}

void DisposeBaseLists (void)
	{
	AllCampList->DeInit();
	delete AllCampList;
	AllCampList = NULL;
	DeaggregateList->DeInit();
	delete DeaggregateList;
	DeaggregateList = NULL;
	for (int loop = 0; loop < MAX_DIRTY_BUCKETS; loop ++)
	{
		DirtyBucket[loop]->DeInit();
		delete DirtyBucket[loop];
		DirtyBucket[loop] = NULL;
	}
	EmitterList->DeInit();
	delete EmitterList;
	}

// Called on campaign end
void DisposeCampaignLists (void)
	{
	DisposeObjList();
	if (AllUnitList)
	{
		AllUnitList->DeInit();
		delete AllUnitList;
		AllUnitList = NULL;
	}
	if (AllAirList)
	{
	    AllAirList->DeInit();
	    delete AllAirList;
	    AllAirList = NULL;
	}
	if (AllParentList)
	{
	    AllParentList->DeInit();
	    delete AllParentList;
	    AllParentList = NULL;
	}
	if (AllRealList)
	{
		AllRealList->DeInit();
		delete AllRealList;
		AllRealList = NULL;
	}
	if (AllObjList)
	{
		AllObjList->DeInit();
		delete AllObjList;
		AllObjList = NULL;
	}
	if (InactiveList)
	{
		InactiveList->DeInit();
		delete InactiveList;
		InactiveList = NULL;
	}
	if (FrontList)
	{
		FrontList->DeInit();
		delete FrontList;
		FrontList = NULL;
	}
	if (AirDefenseList)
	{
		AirDefenseList->DeInit();
		delete AirDefenseList;
		AirDefenseList = NULL;
	}
	if (POList)
	{
		POList->DeInit();
		delete POList;
		POList = NULL;
	}
	if (SOList)
	{
		SOList->DeInit();
		delete SOList;
		SOList = NULL;
	}
	if (RealUnitProxList)
	{
		RealUnitProxList->DeInit();
		delete RealUnitProxList;
	RealUnitProxList= NULL;
	}
	if (RealUnitProxFilter)
	{
		delete RealUnitProxFilter;
	RealUnitProxFilter = NULL;
	}
	}

void DisposeTheaterLists (void)
	{
	if (AllObjProxFilter)
		delete AllObjProxFilter;
	if (ObjProxList)
		{
		ObjProxList->DeInit();
		delete ObjProxList;
		}
	AllObjProxFilter = NULL;
	ObjProxList = NULL;
	}

// ============================================
// List rebuilders (Called at various intervals
// ============================================

int RebuildFrontList (int do_barcaps, int incremental)
	{
	MissionRequestClass	mis;
	Objective			o,n;
	ulong				fseed;
	VuListIterator		myit(AllObjList);
	static CampaignTime	lastRequest=0;
	int					i,front,isolated,bok=0,dirty=0;

	// HACK to allow furball to work
	if (FalconLocalGame && FalconLocalGame->GetGameType() == game_Dogfight)
		return 0;

	if (do_barcaps && lastRequest - Camp_GetCurrentTime() > (unsigned int)BARCAP_REQUEST_INTERVAL)
		{
		lastRequest = Camp_GetCurrentTime();
		bok = 1;
		}

	if (!incremental)
		FrontList->Purge();

	o = GetFirstObjective(&myit);
	while (o)
		{
		front = 0;
		fseed = 0;
		isolated = 1;
		for (i=0,n=o; i<o->static_data.links && n && (!front || isolated); i++)
			{
			n = o->GetNeighbor(i);
			if (n)
				{
				if (GetTTRelations(o->GetTeam(),n->GetTeam()) > Neutral)
					front = n->GetOwner();
				else if (isolated && !GetRoE(n->GetTeam(),o->GetTeam(),ROE_GROUND_CAPTURE)) 
					isolated = 0;
				if (bok && front && GetRoE(o->GetTeam(), n->GetTeam(), ROE_AIR_ENGAGE))
					{
					fseed = o->Id() + n->Id();
					mis.vs = n->GetTeam();
					}
				}
			}
		if (front && isolated && !o->IsSecondary())
			{
			// This objective has been cut off
			Unit		u;
			GridIndex	x,y;
			o->GetLocation(&x,&y);
			u = FindNearestRealUnit(x,y,NULL,5);
			if (u && GetRoE(u->GetTeam(), o->GetTeam(), ROE_GROUND_CAPTURE))
				front = u->GetOwner();
			if (!u || (u && u->GetTeam() != o->GetTeam()))
				{
				// Enemy units are in control, send a captured message
				CaptureObjective(o, (Control)front, NULL);
				}
			}
		if (!front)
			{
			if (o->IsFrontline())
				{
				dirty = 1;
				if (incremental)
					FrontList->Remove(o);
				o->ClearObjFlags (O_FRONTLINE | O_SECONDLINE | O_THIRDLINE);
				}
			o->SetAbandoned(0);
			fseed = 0;
			}
		else if (front)
			{
			if (!o->IsFrontline())
				{
				dirty = 1;
				o->SetObjFlags (O_FRONTLINE);
				o->ClearObjFlags (O_SECONDLINE | O_THIRDLINE);
				}
			if (!incremental || !o->IsFrontline())
				FrontList->ForcedInsert(o);
			}
		if (fseed && do_barcaps)
			{
			// Request a low priority BARCAP mission (each side should take care of their own)
			mis.requesterID = o->Id();
			o->GetLocation(&mis.tx,&mis.ty);
			mis.who = o->GetTeam();
			mis.vs = 0;
			// Try to base TOT on the combined ID of the two frontline objectives - to try and get both teams here at same time
			mis.tot = Camp_GetCurrentTime() + (60+fseed%60)*CampaignMinutes;
			mis.tot_type = TYPE_EQ;
			mis.mission = AMIS_BARCAP2;
			mis.context = hostileAircraftPresent;
			mis.roe_check = ROE_AIR_ENGAGE;
			mis.flags = 0;
			mis.priority = 0;
			mis.RequestMission();
			}
		o = GetNextObjective(&myit);
		}
	if (dirty)
		{
		MarkObjectives();
		RebuildParentsList();
		}
	return dirty;
	}

void RebuildFLOTList (void)
	{
	VuListIterator		frontit(FrontList);
	Objective			o,n;
	int					i,found;
	ListElementClass	*lp;
	void				*data;
	GridIndex			ox,oy,nx,ny,fx,fy,x,y;

	// KCK WARNING: This list sorts for WEST TO EAST. This will look very bad in some situations.
	// I need to think of an algorythm to sort based on the relative geometry between frontline objectives.
	FLOTList->Purge();
	o = (Objective) frontit.GetFirst();
	while (o)
		{
		o->GetLocation(&ox,&oy);
		for (i=0; i<o->NumLinks(); i++)
			{
			n = o->GetNeighbor(i);
			if (n && n->IsFrontline() && o->GetTeam() != n->GetTeam())
				{
				n->GetLocation(&nx,&ny);
				fx = (short)((ox+nx)/2);
				fy = (short)((oy+ny)/2);
				lp = FLOTList->GetFirstElement();
				data = PackXY(fx,fy);
				found = 0;
				while (lp && !found)
					{
					UnpackXY(lp->GetUserData(),&x,&y);
					if (DistSqu(x,y,fx,fy) < 900.0F)		// Min 30 km between points
						found = 1;
					lp = lp->GetNext();
					}
				if (!found)
					{
					if (FLOTSortDirection)
						FLOTList->InsertNewElement(fy, data, 0);
					else
						FLOTList->InsertNewElement(fx, data, 0);
					}
				}
			}
		o = (Objective) frontit.GetNext();
		}
	}

// This will set flags for secondline and thirdline objectives, using the FrontList
void MarkObjectives (void)
	{
	FalconPrivateList	secondlist(&AllObjFilter);
	VuListIterator	frontit(FrontList);
	VuListIterator	secondit(&secondlist);
	VuListIterator	myit(AllObjList);
	Objective		o,n;
	int				i;

	// KCK: It seems like this should be able to be avoided - but I can't
	// think of another way to clear out old secondline and thirdline flags
	o = GetFirstObjective(&myit);
	while (o != NULL)
		{
		o->ClearObjFlags (O_SECONDLINE | O_THIRDLINE);
		o = GetNextObjective(&myit);
		}

	o = GetFirstObjective(&frontit);
	while (o != NULL)
		{
		for (i=0; i<o->NumLinks(); i++)
			{
			n = o->GetNeighbor(i);
			if (n)
				{
				if (!n->IsFrontline() && o->GetTeam()==n->GetTeam())
					{
					n->SetObjFlags (O_SECONDLINE);
					secondlist.ForcedInsert(n);
					}
//				else
//					n->obj_data.flags &= ~(O_SECONDLINE | O_THIRDLINE);
				}
			}
		o = GetNextObjective(&frontit);
		}

	o = GetFirstObjective(&secondit);
	while (o != NULL)
		{
		for (i=0; i<o->NumLinks(); i++)
			{
			n = o->GetNeighbor(i);
			if (n)
				{
				if (!n->IsFrontline() && !n->IsSecondline())
					n->SetObjFlags (O_THIRDLINE);
//				else
//					n->obj_data.flags &= ~O_THIRDLINE;
				}
			}
		o = GetNextObjective(&secondit);
		}
	}

// This only needs to be called at the start of the campaign (after objs loaded)
// or if any new objectives have been added
int RebuildObjectiveLists(void)
	{
	Objective		o;
	VuListIterator	myit(AllObjList);

	POList->Purge();
	SOList->Purge();

	ShiAssert(PODataList);
	ShiAssert(FLOTList);
	
	o = GetFirstObjective(&myit);
	while (o != NULL)
		{
		if (o->IsPrimary())
			POList->ForcedInsert(o);
		if (o->IsSecondary())
			SOList->ForcedInsert(o);
		o = GetNextObjective(&myit);
		}
	CleanupObjList();
	return 1;
	}

// This recalculates which secondary objectives belong to which primaries
// Assumes RebuildObjectiveLists has been called
int RebuildParentsList (void)
	{
	Objective			o;
	VuListIterator		myit(AllObjList);
	
	o = GetFirstObjective(&myit);
	while (o)
		{
		o->RecalculateParent();
		o = GetNextObjective(&myit);
		}
	return 1;
	}

// Sets up Emitting flag for entities with detection capibilities
int RebuildEmitterList (void)
	{
	CampEntity	e,a;
	int			range,d,r,emit,team,rl,change=0;
	MoveType	mt;
	GridIndex	x,y,ex,ey;
	VuListIterator		campit(AllCampList);
	VuListIterator		airit(EmitterList);

	EmitterList->Purge();
	AirDefenseList->Purge();

	e = (CampEntity)campit.GetFirst();
	while (e)
		{
		if (e->GetDomain() == DOMAIN_LAND && (!e->IsUnit() || (!((Unit)e)->Inactive() && ((Unit)e)->Real())))
			{
			rl = e->GetElectronicDetectionRange(LowAir);
			range = e->GetElectronicDetectionRange(Air);
			if (rl > range)
				{
				range = rl;
				mt = LowAir;
				}
			else
				mt = Air;
			if (range)
				{
				// It's part of the IADS, check if we're needed
				emit = 1;
				e->GetLocation(&x,&y);
				team = e->GetTeam();
				if (!e->IsObjective())
				{
					a = (CampEntity)airit.GetFirst();
					while (a && emit)
						{
						if (a->GetTeam() == team && !a->IsObjective())
							{
							a->GetLocation(&ex,&ey);
							d =  FloatToInt32(Distance(x,y,ex,ey));
							r = a->GetElectronicDetectionRange(mt);
							if (r > d + range)
								emit = 0;
							else if ((range > d + r) &&
								(a->GetRadarMode() < FEC_RADAR_SEARCH_1))
								a->SetEmitting(0);
							}
						a = (CampEntity)airit.GetNext();
						}
				}
				if (e->IsBattalion() && e->GetSType() == STYPE_UNIT_AIR_DEFENSE)
//				if (e->GetAproxHitChance(LowAir,0) > 0)
					AirDefenseList->ForcedInsert(e);
				if (!change && ((emit && !e->IsEmitting()) || (!emit && e->IsEmitting())))
					change = 1;
				e->SetEmitting(emit);
				if (emit && e->GetRadarMode() < FEC_RADAR_AQUIRE)
					e->SetSearchMode(FEC_RADAR_SEARCH_1);//me123 + rand()%3);
				else if (!emit)
					e->SetSearchMode(FEC_RADAR_OFF);			// Our "search mode" is off
				}
			}
		e = (CampEntity)campit.GetNext();
		}
	return change;
	}

// This will rebuild all changing lists
void StandardRebuild (void)
	{
	static int	build=0;
	RebuildFrontList(TRUE, FALSE);
	RebuildEmitterList();
	if (!build || !TheCampaign.SamMapData)
		TheCampaign.MakeCampMap(MAP_SAMCOVERAGE);
	else if (build == 1 || !TheCampaign.RadarMapData)
		TheCampaign.MakeCampMap(MAP_RADARCOVERAGE);
	else
		build = -1;
	if (!TheCampaign.CampMapData)
		TheCampaign.MakeCampMap(MAP_OWNERSHIP);
	build++;
	}



// JPO - some debug stuff - to check if lists are ok
int CheckObjProxyOK(int X, int Y)
{
    if (ObjProxList == NULL) return 1;
#ifdef VU_GRID_TREE_Y_MAJOR
    VuGridIterator myit(ObjProxList,(BIG_SCALAR)GridToSim(X),(BIG_SCALAR)GridToSim(Y),(BIG_SCALAR)GridToSim(100));
#else
    VuGridIterator myit(ObjProxList,(BIG_SCALAR)GridToSim(Y),(BIG_SCALAR)GridToSim(X),(BIG_SCALAR)GridToSim(100));
#endif
    Objective	o;
    
    for (o = (Objective) myit.GetFirst();
	    o != NULL;
	    o = (Objective) myit.GetNext()) {
	if (F4IsBadReadPtr(o, sizeof *o))
	    return 0; // gone bad
    }
    return 1;
}

int CheckUnitProxyOK(int X, int Y)
{
    if (RealUnitProxList == NULL) return 1;
#ifdef VU_GRID_TREE_Y_MAJOR
    VuGridIterator myit(RealUnitProxList,(BIG_SCALAR)GridToSim(X),(BIG_SCALAR)GridToSim(Y),(BIG_SCALAR)GridToSim(100));
#else
    VuGridIterator myit(RealUnitProxList,(BIG_SCALAR)GridToSim(Y),(BIG_SCALAR)GridToSim(X),(BIG_SCALAR)GridToSim(100));
#endif
    Unit u;
    for (u = (Unit) myit.GetFirst();
	u != NULL;
	u = (Unit) myit.GetNext()) {
	if (F4IsBadReadPtr(u, sizeof *u))
	    return 0;
    }
    return 1;
}
