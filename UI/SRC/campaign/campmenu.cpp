// Campaign Menus

#include <windows.h>
#include "Graphics\Include\matrix.h"
#include "Graphics\Include\drawbsp.h"
#include "Graphics\Include\loader.h"
#include "entity.h"
#include "feature.h"
#include "vehicle.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "find.h"
#include "cmpclass.h"
#include "division.h"
#include "cmap.h"
#include "cbsplist.h"
#include "c3dview.h"
#include "userids.h"
#include "filters.h"
#include "gps.h"
#include "urefresh.h"
#include "CampStr.h"

void DeleteGroupList(long ID);
void AddObjectiveToTargetTree(Objective obj);
void SetBullsEye(C_Window *);
void SetSlantRange(C_Window *);
void SetHeading(C_Window *);
void PositionCamera(OBJECTINFO *Info,C_Window *win,long client);
void SetupUnitInfoWindow(VU_ID Id);
void SetupDivisionInfoWindow(long DivID,short owner);
void GetObjectivesNear(float x,float y,float range);
void GetGroundUnitsNear(float x,float y,float range);
void ReconArea(float x,float y,float range);
void BuildTargetList(float x,float y,float range);
void BuildSpecificTargetList(VU_ID targetID);
void set_waypoint_action (WayPoint wp, int action);
void refresh_waypoint (WayPointClass * wp);
void tactical_add_victory_condition (VU_ID id,C_Base *caller);
void tactical_add_squadron (VU_ID id);
void tactical_add_flight (VU_ID ID,C_Base *caller);
void tactical_add_package (VU_ID ID,C_Base *caller);
void tactical_add_battalion (VU_ID ID,C_Base *control);
void recalculate_waypoints (WayPointClass *wp);
void tactical_edit_package (VU_ID id,C_Base *caller);
void fixup_unit (Unit unit);
void SetupSquadronInfoWindow(VU_ID TheID);
void CloseAllRenderers(long openID);

C_TreeList *TargetTree=NULL;

extern C_Handler *gMainHandler;
extern C_Map *gMapMgr;
extern C_3dViewer *gUIViewer;
extern OBJECTINFO Recon;
extern GlobalPositioningSystem *gGps;
extern VU_ID gActiveFlightID,gCurrentFlightID;

extern int g_nUnidentifiedInUI; // 2002-02-24 S.G.
extern int gShowUnknown; // 2002-02-21 S.G.
#define MID_UNITS_SQUAD_UNKNOWN 86051 // 2002-02-21 S.G. Until I add it to userids.h
extern GlobalPositioningSystem *gGps; // 2002-02-21 S.G.

// Used for enabling & disabling menus based on TE/Camp/Edit modes
static long GameType;
static long EditMode;

void MenuToggleObjectiveCB(long ID,short,C_Base *control)
{
	switch(ID)
	{
		case MID_INST_AF:
			if(((C_PopupList *)control)->GetItemState(ID))
				gMapMgr->ShowObjectiveType(_OBTV_AIR_FIELDS);
			else
				gMapMgr->HideObjectiveType(_OBTV_AIR_FIELDS);
			break;
		case MID_INST_AD:
			if(((C_PopupList *)control)->GetItemState(ID))
				gMapMgr->ShowObjectiveType(_OBTV_AIR_DEFENSE);
			else
				gMapMgr->HideObjectiveType(_OBTV_AIR_DEFENSE);
			break;
		case MID_INST_ARMY:
			if(((C_PopupList *)control)->GetItemState(ID))
				gMapMgr->ShowObjectiveType(_OBTV_ARMY);
			else
				gMapMgr->HideObjectiveType(_OBTV_ARMY);
			break;
		case MID_INST_CCC:
			if(((C_PopupList *)control)->GetItemState(ID))
				gMapMgr->ShowObjectiveType(_OBTV_CCC);
			else
				gMapMgr->HideObjectiveType(_OBTV_CCC);
			break;
		case MID_INST_POLITICAL:
			if(((C_PopupList *)control)->GetItemState(ID))
				gMapMgr->ShowObjectiveType(_OBTV_POLITICAL);
			else
				gMapMgr->HideObjectiveType(_OBTV_POLITICAL);
			break;
		case MID_INST_INFRA:
			if(((C_PopupList *)control)->GetItemState(ID))
				gMapMgr->ShowObjectiveType(_OBTV_INFRASTRUCTURE);
			else
				gMapMgr->HideObjectiveType(_OBTV_INFRASTRUCTURE);
			break;
		case MID_INST_LOG:
			if(((C_PopupList *)control)->GetItemState(ID))
				gMapMgr->ShowObjectiveType(_OBTV_LOGISTICS);
			else
				gMapMgr->HideObjectiveType(_OBTV_LOGISTICS);
			break;
		case MID_INST_WARPROD:
			if(((C_PopupList *)control)->GetItemState(ID))
				gMapMgr->ShowObjectiveType(_OBTV_WAR_PRODUCTION);
			else
				gMapMgr->HideObjectiveType(_OBTV_WAR_PRODUCTION);
			break;
		case MID_INST_NAV:
			if(((C_PopupList *)control)->GetItemState(ID))
				gMapMgr->ShowObjectiveType(_OBTV_NAVIGATION);
			else
				gMapMgr->HideObjectiveType(_OBTV_NAVIGATION);
			break;
		case MID_INST_OTHER:
			if(((C_PopupList *)control)->GetItemState(ID))
				gMapMgr->ShowObjectiveType(_OBTV_OTHER);
			else
				gMapMgr->HideObjectiveType(_OBTV_OTHER);
			break;
		case MID_INST_NAVAL:
			if(((C_PopupList *)control)->GetItemState(ID))
				gMapMgr->ShowObjectiveType(_OBTV_NAVAL);
			else
				gMapMgr->HideObjectiveType(_OBTV_NAVAL);
			break;
		case MID_SHOW_VC:
			if(((C_PopupList *)control)->GetItemState(ID))
				gMapMgr->ShowObjectiveType(_VC_CONDITION_);
			else
				gMapMgr->HideObjectiveType(_VC_CONDITION_);
			break;
	}
	gMapMgr->DrawMap();
}

void MenuToggleUnitCB(long ID,short,C_Base *control)
{
	switch(ID)
	{
		case MID_UNITS_DIV:
			gMapMgr->SetUnitLevel(0);
			break;
		case MID_UNITS_BRIG:
			gMapMgr->SetUnitLevel(1);
			break;
		case MID_UNITS_BAT:
			gMapMgr->SetUnitLevel(2);
			break;
		case MID_UNITS_COMBAT:
			if(((C_PopupList *)control)->GetItemState(ID))
				gMapMgr->ShowUnitType(_UNIT_COMBAT);
			else
				gMapMgr->HideUnitType(_UNIT_COMBAT);
			break;
		case MID_UNITS_AD:
			if(((C_PopupList *)control)->GetItemState(ID))
				gMapMgr->ShowUnitType(_UNIT_AIR_DEFENSE);
			else
				gMapMgr->HideUnitType(_UNIT_AIR_DEFENSE);
			break;
		case MID_UNITS_SUPPORT:
			if(((C_PopupList *)control)->GetItemState(ID))
				gMapMgr->ShowUnitType(_UNIT_SUPPORT);
			else
				gMapMgr->HideUnitType(_UNIT_SUPPORT);
			break;
		case MID_UNITS_SQUAD_SQUADRON:
			if(((C_PopupList *)control)->GetItemState(ID))
				gMapMgr->ShowObjectiveType(_UNIT_SQUADRON);
			else
				gMapMgr->HideObjectiveType(_UNIT_SQUADRON);
			break;
		case MID_UNITS_SQUAD_PACKAGE:
			if(((C_PopupList *)control)->GetItemState(ID))
				gMapMgr->ShowObjectiveType(_UNIT_PACKAGE);
			else
				gMapMgr->HideObjectiveType(_UNIT_PACKAGE);
			break;
		case MID_UNITS_SQUAD_FIGHTER:
			if(((C_PopupList *)control)->GetItemState(ID))
				gMapMgr->ShowAirUnitType(_UNIT_FIGHTER);
			else
				gMapMgr->HideAirUnitType(_UNIT_FIGHTER);
			break;
		case MID_UNITS_SQUAD_ATTACK:
			if(((C_PopupList *)control)->GetItemState(ID))
				gMapMgr->ShowAirUnitType(_UNIT_ATTACK);
			else
				gMapMgr->HideAirUnitType(_UNIT_ATTACK);
			break;
		case MID_UNITS_SQUAD_BOMBER:
			if(((C_PopupList *)control)->GetItemState(ID))
				gMapMgr->ShowAirUnitType(_UNIT_BOMBER);
			else
				gMapMgr->HideAirUnitType(_UNIT_BOMBER);
			break;
		case MID_UNITS_SQUAD_SUPPORT:
			if(((C_PopupList *)control)->GetItemState(ID))
				gMapMgr->ShowAirUnitType(_UNIT_SUPPORT);
			else
				gMapMgr->HideAirUnitType(_UNIT_SUPPORT);
			break;
		case MID_UNITS_SQUAD_HELI:
			if(((C_PopupList *)control)->GetItemState(ID))
				gMapMgr->ShowAirUnitType(_UNIT_HELICOPTER);
			else
				gMapMgr->HideAirUnitType(_UNIT_HELICOPTER);
			break;
		// 2002-02-21 ADDED BY S.G. Our new 'Unknown' option to Flights page so we can display unknown type of flight as well as identified one
		case MID_UNITS_SQUAD_UNKNOWN:
			if(((C_PopupList *)control)->GetItemState(ID))
				gShowUnknown = 1;
			else
				gShowUnknown = 0;

			if (gGps)
				gGps->Update();
			gMapMgr->RefreshAllAirUnitType();
			break;
		case MID_UNITS_NAVY_COMBAT:
			if(((C_PopupList *)control)->GetItemState(ID))
				gMapMgr->ShowNavalUnitType(_UNIT_COMBAT);
			else
				gMapMgr->HideNavalUnitType(_UNIT_COMBAT);
			break;
		case MID_UNITS_NAVY_SUPPLY:
			if(((C_PopupList *)control)->GetItemState(ID))
				gMapMgr->ShowNavalUnitType(_UNIT_SUPPORT);
			else
				gMapMgr->HideNavalUnitType(_UNIT_SUPPORT);
			break;
	}
	gMapMgr->DrawMap();
}

void MenuToggleNamesCB(long ID,short,C_Base *control)
{
	if(((C_PopupList *)control)->GetItemState(ID))
		gMapMgr->TurnOnNames();
	else
		gMapMgr->TurnOffNames();
	gMapMgr->DrawMap();
}

void MenuToggleBullseyeCB(long ID,short,C_Base *control)
{
	if(((C_PopupList *)control)->GetItemState(ID))
		gMapMgr->TurnOnBullseye();
	else
		gMapMgr->TurnOffBullseye();
	gMapMgr->DrawMap();
}
void MenuSetCirclesCB(long,short,C_Base *)
{
	C_PopupList *menu;

	menu=gPopupMgr->GetMenu(MAP_POP);
	if(menu)
	{
		if(menu->GetItemState(MID_CIRCLE_SAM_LOW))
			gMapMgr->ShowThreatType(_THR_SAM_LOW);
		else if(menu->GetItemState(MID_CIRCLE_SAM_HIGH))
			gMapMgr->ShowThreatType(_THR_SAM_HIGH);
		else if(menu->GetItemState(MID_CIRCLE_RADAR_LOW))
			gMapMgr->ShowThreatType(_THR_RADAR_LOW);
		else if(menu->GetItemState(MID_CIRCLE_RADAR_HIGH))
			gMapMgr->ShowThreatType(_THR_RADAR_HIGH);
		else
			gMapMgr->HideThreatType(_THR_SAM_LOW|_THR_SAM_HIGH|_THR_RADAR_LOW|_THR_RADAR_HIGH);
		gMapMgr->DrawMap();
	}
}

void MenuToggleTroupBoundariesCB(long,short,C_Base *)
{
}

void MenuToggleMovementArrowsCB(long,short,C_Base *)
{
}

// THE MAJOR CHANGE to this routine is making it support being called from
// either a C_MapIcon item OR a C_TreeList item
//
void MenuObjReconCB(long,short,C_Base *)
{
	Objective objective;
	C_Window *win;
	C_Base *caller;
	C_MapIcon *icon;
	C_DrawList *piggy;
	C_TreeList *tree;
	TREELIST *item;
	long iconid;
	UI_Refresher *urec=NULL;

	gPopupMgr->CloseMenu();

	caller=gPopupMgr->GetCallingControl();
	if(caller == NULL)
		return;

	if(caller->_GetCType_() == _CNTL_MAPICON_)
	{
		icon=(C_MapIcon *)caller;
		iconid=icon->GetIconID();
		urec=(UI_Refresher *)gGps->Find(iconid);
	}
	else if(caller->_GetCType_() == _CNTL_DRAWLIST_)
	{
		piggy=(C_DrawList *)caller;
		iconid=piggy->GetIconID();
		urec=(UI_Refresher *)gGps->Find(iconid);
	}
	else if(caller->_GetCType_() == _CNTL_TREELIST_)
	{
		tree=(C_TreeList *)caller;
		item=tree->GetLastItem();
		if(item)
			urec=(UI_Refresher *)gGps->Find(item->ID_);
	}

	gPopupMgr->CloseMenu();

	SetCursor(gCursors[CRSR_WAIT]);

	win=gMainHandler->FindWindow(RECON_WIN);
	if(win)
	{
		CloseAllRenderers(RECON_WIN);
		if(TargetTree)
			TargetTree->DeleteBranch(TargetTree->GetRoot());

		objective=(Objective)vuDatabase->Find(urec->GetID());
		if(objective == NULL)
			return;

		if(!objective->IsObjective())
			return;

		if(gUIViewer)
		{
			gUIViewer->Cleanup();
			delete gUIViewer;
		}

		gUIViewer=new C_3dViewer;
		gUIViewer->Setup();
		gUIViewer->Viewport(win,0); // use client 0 for this window

		Recon.Heading=0.0f;
		Recon.Pitch=70.0f;
		Recon.Distance=1000.0f;
		Recon.Direction=0.0f;

		Recon.MinDistance=250.0f;
		Recon.MaxDistance=30000.0f;
		Recon.MinPitch=5;
		Recon.MaxPitch=90;
		Recon.CheckPitch=TRUE;

		Recon.PosX=objective->XPos();
		Recon.PosY=objective->YPos();
		Recon.PosZ=objective->ZPos();

		AddObjectiveToTargetTree (objective);
		GetGroundUnitsNear(Recon.PosX,Recon.PosY,10000.0f);

		SetBullsEye(win);
		SetSlantRange(win);
		SetHeading(win);

		gUIViewer->SetPosition(Recon.PosX,Recon.PosY,Recon.PosZ);
		gUIViewer->InitOTW(30.0f,TRUE);
		gUIViewer->AddAllToView();

		win->ScanClientAreas();
		win->RefreshWindow();

		PositionCamera(&Recon,win,0);
		TheLoader.WaitForLoader();
		PositionCamera(&Recon,win,0);

		gMainHandler->ShowWindow(win);
		gMainHandler->WindowToFront(win);
	}
	win=gMainHandler->FindWindow(RECON_LIST_WIN);
	if(win)
	{
		if(TargetTree)
			TargetTree->RecalcSize();
		gMainHandler->ShowWindow(win);
		gMainHandler->WindowToFront(win);
	}
	SetCursor(gCursors[CRSR_F16]);
}

void MenuAlternateCB(long,short,C_Base *)
{
	C_Base *caller;
	C_MapIcon *icon;
	C_DrawList *piggy;
	C_TreeList *tree;
	TREELIST *item;
	long iconid;
	UI_Refresher *urec=NULL;

	gPopupMgr->CloseMenu();

	caller=gPopupMgr->GetCallingControl();
	if(caller == NULL)
		return;

	if(caller->_GetCType_() == _CNTL_MAPICON_)
	{
		icon=(C_MapIcon *)caller;
		iconid=icon->GetIconID();
		urec=(UI_Refresher *)gGps->Find(iconid);
	}
	else if(caller->_GetCType_() == _CNTL_DRAWLIST_)
	{
		piggy=(C_DrawList *)caller;
		iconid=piggy->GetIconID();
		urec=(UI_Refresher *)gGps->Find(iconid);
	}
	else if(caller->_GetCType_() == _CNTL_TREELIST_)
	{
		tree=(C_TreeList *)caller;
		item=tree->GetLastItem();
		if(item)
			urec=(UI_Refresher *)gGps->Find(item->ID_);
	}
}

WayPointClass* GetSelectedWayPoint (void)
	{
	C_Base			*caller;
	C_Base			*control;
	C_Waypoint		*cwp;
	VU_ID			*tmpID;
	Unit			unit;
	WayPoint		wp;
	int				i;

	caller=gPopupMgr->GetCallingControl();
	if(caller == NULL)
		return NULL;

	if (gPopupMgr->GetCallingType() == C_TYPE_CONTROL && caller->_GetCType_() == _CNTL_WAYPOINT_)
	{
		// Waypoint
		cwp = (C_Waypoint *)caller;
		if (cwp && cwp->GetLast())
		{
			control = cwp->GetLast()->Icon;
			if (!control)
				return NULL;

			tmpID=(VU_ID *)control->GetUserPtr(C_STATE_0);
			if (!tmpID) 
				return NULL;

			// Check if this is our current waypoint set, and make sure our
			// global ids match this information.
			if (gMapMgr->GetCurWP() == cwp)
			{
				VU_ID			*tmpID;

				tmpID=(VU_ID *)control->GetUserPtr(C_STATE_0);
				if (tmpID && *tmpID != gActiveFlightID)
				{
					gActiveFlightID = *tmpID;
					gCurrentFlightID = *tmpID;
				}
			}

			unit = FindUnit(*tmpID);
			if (unit && unit->IsFlight())
			{
				wp = unit->GetFirstUnitWP();
				i = 1;
				while(i < control->GetUserNumber(C_STATE_1) && wp)
				{
					wp=wp->GetNextWP();
					i++;
				}
				return wp;
			}
		}
	}
	return NULL;
}

void SteerPointMenuOpenCB (C_Base *,C_Base *)
	{
	C_PopupList *menu;
	WayPoint	wp;

	// We've opened a steerpoint's popup menu. Setup current values!
	wp = GetSelectedWayPoint();

	menu=gPopupMgr->GetMenu(STEERPOINT_POP);
	if(menu && wp)
		{
		if (wp->GetWPFlags() & WPF_TIME_LOCKED)
			menu->SetItemState(MID_LOCK_TOS,1);
		else
			menu->SetItemState(MID_LOCK_TOS,0);
		if (wp->GetWPFlags() & WPF_SPEED_LOCKED)
			menu->SetItemState(MID_LOCK_SPEED,1);
		else
			menu->SetItemState(MID_LOCK_SPEED,0);
		if (wp->GetWPFlags() & WPF_HOLDCURRENT)
			menu->SetItemState(CLIMB_DELAY,1);
		else
			menu->SetItemState(CLIMB_IMMEDIATE,1);
		menu->SetItemState(wp->GetWPFormation()+1,1);
		menu->SetItemState(wp->GetWPAction() | 0x200,1);
		menu->SetItemState(wp->GetWPRouteAction() | 0x100,1);
		}
	}

void MenuUnitDeleteCB(long,short,C_Base *)
{
	C_Base *caller;
	C_MapIcon *icon;
	C_DrawList *piggy;
	C_TreeList *tree;
	TREELIST *item;
	long iconid,count;
	UI_Refresher *urec=NULL;
	Unit unit,sec;
	Package pkg;

	gPopupMgr->CloseMenu();

	caller=gPopupMgr->GetCallingControl();
	if(caller == NULL)
		return;

	if(gPopupMgr->GetCallingType() == C_TYPE_CONTROL)
	{
		// Recon this + very close Objects
		if(caller->_GetCType_() == _CNTL_MAPICON_)
		{
			icon=(C_MapIcon *)caller;
			iconid=icon->GetIconID();
			urec=(UI_Refresher *)gGps->Find(iconid);
		}
		else if(caller->_GetCType_() == _CNTL_DRAWLIST_)
		{
			piggy=(C_DrawList *)caller;
			iconid=piggy->GetIconID();
			urec=(UI_Refresher *)gGps->Find(iconid);
		}
		else if(caller->_GetCType_() == _CNTL_TREELIST_)
		{
			tree=(C_TreeList *)caller;
			item=tree->GetLastItem();
			if(item)
				urec=(UI_Refresher *)gGps->Find(item->ID_);
		}
		if(urec)
		{
			unit=(Unit)vuDatabase->Find(urec->GetID());
			if(unit && unit->IsUnit())
			{
				pkg=NULL;
				count=0;
				if(unit->IsFlight())
				{
					pkg=(Package)unit->GetUnitParent();
					if(pkg && pkg->IsPackage())
					{
						sec=pkg->GetFirstUnitElement();
						while(sec)
						{
							if(sec != unit)
								count++;
							sec=pkg->GetNextUnitElement();
						}
					}
					else
						pkg=NULL;
				}
				unit->Remove();
				if(pkg && !count)
					pkg->Remove();
			}
		}
	}
}

void MenuReconCB(long,short,C_Base *)
{
	C_Base *caller;
	C_MapIcon *icon;
	C_DrawList *piggy;
	C_TreeList *tree;
	C_Waypoint *wp;
	TREELIST *item;
	WAYPOINTLIST *wpicon;
	long iconid;
	UI_Refresher *urec=NULL;
	float x=0.0f,y=0.0f,range=0.0f;
	short relx,rely;
	float maxy, scale;
	CampEntity ent;

	gPopupMgr->CloseMenu();

	caller=gPopupMgr->GetCallingControl();
	if(caller == NULL)
		return;

	if(gPopupMgr->GetCallingType() == C_TYPE_CONTROL)
	{
		// Recon this + very close Objects
		if(caller->_GetCType_() == _CNTL_MAPICON_)
		{
			icon=(C_MapIcon *)caller;
			iconid=icon->GetIconID();
			urec=(UI_Refresher *)gGps->Find(iconid);
		}
		else if(caller->_GetCType_() == _CNTL_WAYPOINT_)
		{
			wp=(C_Waypoint *)caller;
			if(wp)
			{
				wpicon=wp->GetLast();
				if(wpicon)
				{
					x=caller->GetUserNumber(0)-wpicon->worldy;
					y=wpicon->worldx;
					range=18000.0f;
				}
			}
		}
		else if(caller->_GetCType_() == _CNTL_DRAWLIST_)
		{
			piggy=(C_DrawList *)caller;
			iconid=piggy->GetIconID();
			urec=(UI_Refresher *)gGps->Find(iconid);
		}
		else if(caller->_GetCType_() == _CNTL_TREELIST_)
		{
			tree=(C_TreeList *)caller;
			item=tree->GetLastItem();
			if(item)
				urec=(UI_Refresher *)gGps->Find(item->ID_);
		}
		else if(caller->_GetCType_() == _CNTL_MAP_MOVER_)
		{
			// Recon Area...
			range=18000.0f;

			relx=static_cast<short>(((C_MapMover*)caller)->GetRelX() + caller->GetX() + caller->Parent_->GetX());
			rely=static_cast<short>(((C_MapMover*)caller)->GetRelY() + caller->GetY() + caller->Parent_->GetY());
			gMapMgr->GetMapRelativeXY (&relx, &rely);

			scale = gMapMgr->GetMapScale ();
			maxy = gMapMgr->GetMaxY ();

			// x & y are reversed for SIM
			y = relx / scale;
			x = maxy - rely / scale;
		}

		if(urec)
		{
			ent=(CampEntity)vuDatabase->Find(urec->GetID());
			if(ent)
			{
				// 2002-02-21 ADDED BY S.G. If not spotted by the player's team or not editing a TE, can't recon...
				if (!(TheCampaign.Flags & CAMP_TACTICAL_EDIT) && ent->IsFlight() && gGps->GetTeamNo() != ent->GetTeam() && !ent->GetIdentified(gGps->GetTeamNo()))
					range=0.0f;
				else {
				// END OF ADDED SECTION 2002-02-21
					range=6000.0f;
					x=ent->XPos();
					y=ent->YPos();
				}
			}
		}
	}
	if(range > 1.0f)
		ReconArea(x,y,range);
}

void MenuStatusCB(long,short,C_Base *)
{
	C_Base *caller;
	C_MapIcon *icon;
	C_DrawList *piggy;
	C_TreeList *tree;
	TREELIST *item;
	long iconid;
	UI_Refresher *urec=NULL;
	CampEntity ent;

	gPopupMgr->CloseMenu();

	caller=gPopupMgr->GetCallingControl();
	if(caller == NULL)
		return;

	if(gPopupMgr->GetCallingType() == C_TYPE_CONTROL)
	{
		// Recon this + very close Objects
		if(caller->_GetCType_() == _CNTL_MAPICON_)
		{
			icon=(C_MapIcon *)caller;
			iconid=icon->GetIconID();
			urec=(UI_Refresher *)gGps->Find(iconid);
		}
		else if(caller->_GetCType_() == _CNTL_DRAWLIST_)
		{
			piggy=(C_DrawList *)caller;
			iconid=piggy->GetIconID();
			urec=(UI_Refresher *)gGps->Find(iconid);
		}
		else if(caller->_GetCType_() == _CNTL_TREELIST_)
		{
			tree=(C_TreeList *)caller;
			item=tree->GetLastItem();
			if(item)
				urec=(UI_Refresher *)gGps->Find(item->ID_);
		}
		if(urec)
		{
			ent=(CampEntity)vuDatabase->Find(urec->GetID());
			if(ent)
			{
				BuildSpecificTargetList(ent->Id());
			}
		}
	}
}

void MenuUnitStatusCB(long,short,C_Base *)
{
	C_Window *win;
	C_Base *caller;
	C_MapIcon *icon;
	C_DrawList *piggy;
	C_TreeList *tree;
	long iconid=0;
	TREELIST *item;
	CampEntity ent;
	UI_Refresher *urec=NULL;

	gPopupMgr->CloseMenu();

	caller=gPopupMgr->GetCallingControl();
	if(caller == NULL)
		return;

	if(caller->_GetCType_() == _CNTL_MAPICON_)
	{
		icon=(C_MapIcon *)caller;
		iconid=icon->GetIconID();
		urec=(UI_Refresher *)gGps->Find(iconid);
	}
	else if(caller->_GetCType_() == _CNTL_DRAWLIST_)
	{
		piggy=(C_DrawList *)caller;
		iconid=piggy->GetIconID();
		urec=(UI_Refresher *)gGps->Find(iconid);
	}
	else if(caller->_GetCType_() == _CNTL_TREELIST_)
	{
		tree=(C_TreeList *)caller;
		item=tree->GetLastItem();
		if(item)
		{
			iconid=item->ID_;
			urec=(UI_Refresher *)gGps->Find(item->ID_ & 0x00ffffff); // strip off team (incase it is a division)
		}
	}
	if(urec)
	{
		win=gMainHandler->FindWindow(UNIT_WIN);
		if(win)
		{
			if(urec && urec->GetType() == GPS_DIVISION)
				SetupDivisionInfoWindow(urec->GetDivID(),urec->GetSide()); // Map & Tree save team # in top 8 bits (Needed to find division by team)
			else
			{
				ent=(CampEntity)vuDatabase->Find(urec->GetID());
				if(ent && ent->IsUnit())
				{
					if(ent->IsFlight() || ent->IsSquadron())
					{
						// 2002-02-21 ADDED BY S.G. If not spotted by the player's team or not editing a TE, can't get its status...
						if ((TheCampaign.Flags & CAMP_TACTICAL_EDIT) || gGps->GetTeamNo() == ent->GetTeam() || ent->GetIdentified(gGps->GetTeamNo()))
						// END OF ADDED SECTION 2002-02-21
							SetupSquadronInfoWindow(urec->GetID());

					}
					else if(ent->IsTaskForce())
					{
						BuildSpecificTargetList(urec->GetID());
					}
					else
						SetupUnitInfoWindow(urec->GetID());
				}
			}
		}
	}
}

void MenuShowSquadronsCB(long,short,C_Base *)
{
}

void MenuOpenWpWindowCB(long,short,C_Base *)
{
}

void MenuLockCB(long ID,short,C_Base *)
	{
	WayPoint	wp = NULL;
	
	gPopupMgr->CloseMenu();

	wp = GetSelectedWayPoint();
	if (wp)
		{
		if (ID == MID_LOCK_TOS)
			wp->SetWPFlag(WPF_TIME_LOCKED);
		if (ID == MID_LOCK_SPEED)
			wp->SetWPFlag(WPF_SPEED_LOCKED);
		refresh_waypoint (wp);
		}
	}

void MenuClimbCB(long ID,short,C_Base *)
	{
	WayPoint	wp = NULL;
	
	gPopupMgr->CloseMenu();

	wp = GetSelectedWayPoint();
	if (wp)
		{
		if (ID == CLIMB_DELAY)
			wp->SetWPFlag(WPF_HOLDCURRENT);
		else
			wp->UnSetWPFlag(WPF_HOLDCURRENT);
		refresh_waypoint (wp);
		}
	}

void MenuFormationCB(long ID,short,C_Base *)
	{
	WayPoint	wp = NULL;
	int			formation;
	
	gPopupMgr->CloseMenu();

	wp = GetSelectedWayPoint();
	if (wp)
		{
		formation = ID & 0xff;
		wp->SetWPFormation(formation-1);
		refresh_waypoint (wp);
		}
	}

void MenuEnrouteCB(long ID,short,C_Base *)
	{
	WayPoint	wp = NULL;
	int			action;
	
	gPopupMgr->CloseMenu();

	wp = GetSelectedWayPoint();
	if (wp)
		{
		action = ID & 0xff;
		wp->SetWPRouteAction(action);
		refresh_waypoint (wp);
		}
	}

void MenuActionCB(long ID,short,C_Base *)
	{
	WayPoint	wp = NULL;
	int			action;
	
	gPopupMgr->CloseMenu();

	wp = GetSelectedWayPoint();
	if (wp)
		{
		action = ID & 0xff;
		set_waypoint_action(wp,action);
		refresh_waypoint (wp);
		}
	}

void MenuAddWPCB(long,short,C_Base *)
{
	C_Waypoint
		*cwp;

	WayPoint
		wp;

	WAYPOINTLIST
		*wps;

	Unit
		un;

	int
		num;

	cwp = (C_Waypoint *) gPopupMgr->GetCallingControl ();

	un = (Unit)vuDatabase->Find(gMapMgr->GetCurWPID());
	if(!un)
		return;

	wp = un->GetFirstUnitWP ();
	// if(un->GetCurrentUnitWP() != wp)
	//	return;

	wps = cwp->GetLast ();

	num = wps->ID & 0xff;

	while ((num > 1) && (wp))
	{
		wp = wp->GetNextWP ();
		num --;
	}

	wp->SplitWP ();

	gMapMgr->SetCurrentWaypointList (un->Id ());
	gPopupMgr->CloseMenu ();
}

void MenuDeleteWPCB(long,short hittype,C_Base *)
{
	WayPoint		wp,rw;
	Unit			un;

	un = (Unit)vuDatabase->Find(gMapMgr->GetCurWPID());
	if(!un)
		return;

	if(hittype != C_TYPE_LMOUSEUP)
		return;

	gPopupMgr->CloseMenu();

	wp = GetSelectedWayPoint();
	if (wp && (wp->GetPrevWP()) && (wp->GetNextWP()))
		{
		rw = wp->GetPrevWP();
		un->DeleteUnitWP(wp);
		recalculate_waypoints(rw);
		}

	gMapMgr->SetCurrentWaypointList (un->Id());

	if ((TheCampaign.Flags & CAMP_TACTICAL_EDIT) && un->IsFlight())
	{
		fixup_unit (un);
		gGps->Update();
		gMapMgr->DrawMap();
	}
}

void MenuEditPackageCB(long,short,C_Base *control)
{
	C_Base *caller;
	C_MapIcon *icon;
	C_DrawList *piggy;
	C_TreeList *tree;
	TREELIST *item;
	long iconid=0;
	UI_Refresher *urec=NULL;

	gPopupMgr->CloseMenu();

	caller=gPopupMgr->GetCallingControl();
	if(caller == NULL)
		return;

	if(caller->_GetCType_() == _CNTL_MAPICON_)
	{
		icon=(C_MapIcon *)caller;
		iconid=icon->GetIconID();
		urec=(UI_Refresher *)gGps->Find(iconid);
	}
	else if(caller->_GetCType_() == _CNTL_DRAWLIST_)
	{
		piggy=(C_DrawList *)caller;
		iconid=piggy->GetIconID();
		urec=(UI_Refresher *)gGps->Find(iconid);
	}
	else if(caller->_GetCType_() == _CNTL_TREELIST_)
	{
		tree=(C_TreeList *)caller;
		item=tree->GetLastItem();
		if(item)
		{
			iconid=item->ID_;
			urec=(UI_Refresher *)gGps->Find(item->ID_ & 0x00ffffff); // strip off team (incase it is a division)
		}
	}
	if(urec)
	{
		tactical_edit_package(urec->GetID(),control);
	}
}

void MenuSetOwnerCB(long ID,short,C_Base *)
{
	C_Base *caller;
	C_MapIcon *icon;
	C_DrawList *piggy;
	C_TreeList *tree;
	long iconid;
	TREELIST *item;
	UI_Refresher *urec;
	
	urec = NULL;

	caller = gPopupMgr->GetCallingControl ();

	if(caller == NULL)
	{
		return;
	}

	gPopupMgr->CloseMenu ();

	if (caller->_GetCType_() == _CNTL_MAPICON_)
	{
		icon = (C_MapIcon *) caller;
		iconid = icon->GetIconID ();
		urec = (UI_Refresher *) gGps->Find (iconid);
	}
	else if (caller->_GetCType_() == _CNTL_DRAWLIST_)
	{
		piggy = (C_DrawList *) caller;
		iconid = piggy->GetIconID ();
		urec = (UI_Refresher *) gGps->Find (iconid);
	}
	else if (caller->_GetCType_() == _CNTL_TREELIST_)
	{
		tree = (C_TreeList *) caller;
		item = tree->GetLastItem ();
		if (item)
		{
			urec = (UI_Refresher *) gGps->Find (item->ID_);
		}
	}
	if(urec)
	{
		CampEntity ent;
		short teamid=0;

		switch(ID)
		{
			case MID_TEAM_1:
				teamid=1;
				break;
			case MID_TEAM_2:
				teamid=2;
				break;
			case MID_TEAM_3:
				teamid=3;
				break;
			case MID_TEAM_4:
				teamid=4;
				break;
			case MID_TEAM_5:
				teamid=5;
				break;
			case MID_TEAM_6:
				teamid=6;
				break;
			case MID_TEAM_7:
				teamid=7;
				break;
			default:
				teamid=0;
				break;
		}
		ent=(CampEntity)vuDatabase->Find(urec->GetID());
		if(ent)
		{
			ent->SetOwner(static_cast<uchar>(teamid));
		}
	}
}

void MenuAddUnitCB(long ID,short,C_Base *control)
{
	C_Base			*caller;
	C_MapIcon		*icon;
	C_DrawList		*piggy;
	C_TreeList		*tree;
	long			iconid;
	TREELIST		*item;
	UI_Refresher	*urec;
	VU_ID			vid = FalconNullId;
	
	urec = NULL;
	caller = gPopupMgr->GetCallingControl ();
	if(!caller)
		return;

	if (caller->_GetCType_() == _CNTL_MAPICON_)
	{
		icon = (C_MapIcon *) caller;
		iconid = icon->GetIconID ();
		urec = (UI_Refresher *) gGps->Find (iconid);
		if (urec)
			vid = urec->GetID();
	}
	else if (caller->_GetCType_() == _CNTL_DRAWLIST_)
	{
		piggy = (C_DrawList *) caller;
		iconid = piggy->GetIconID ();
		urec = (UI_Refresher *) gGps->Find (iconid);
		if (urec)
			vid = urec->GetID();
	}
	else if (caller->_GetCType_() == _CNTL_TREELIST_)
	{
		tree = (C_TreeList *) caller;
		item = tree->GetLastItem ();
		if (item)
			{
			urec = (UI_Refresher *) gGps->Find (item->ID_);
			if (urec)
				vid = urec->GetID();
			}
	}

	switch (ID)
		{
		case MID_ADD_FLIGHT:
			tactical_add_flight (vid,control);
			break;
		case MID_ADD_PACKAGE:
			tactical_add_package (vid,control);
			break;
		case MID_ADD_BATTALION:
			tactical_add_battalion (vid,control);
			break;
		case MID_ADD_SQUADRON:
			tactical_add_squadron (vid);
			break;
		}

	gPopupMgr->CloseMenu ();
}

void MenuAddVCCB(long, short, C_Base *)
{
	C_Base *caller;
	C_MapIcon *icon;
	C_DrawList *piggy;
	C_TreeList *tree;
	long iconid;
	TREELIST *item;
	UI_Refresher *urec;
	
	urec = NULL;

	caller = gPopupMgr->GetCallingControl ();

	if(caller == NULL)
	{
		return;
	}

	if (caller->_GetCType_() == _CNTL_MAPICON_)
	{
		icon = (C_MapIcon *) caller;

		iconid = icon->GetIconID ();

		urec = (UI_Refresher *) gGps->Find (iconid);
	}
	else if (caller->_GetCType_() == _CNTL_DRAWLIST_)
	{
		piggy = (C_DrawList *) caller;

		iconid = piggy->GetIconID ();

		urec = (UI_Refresher *) gGps->Find (iconid);
	}
	else if (caller->_GetCType_() == _CNTL_TREELIST_)
	{
		tree = (C_TreeList *) caller;

		item = tree->GetLastItem ();

		if (item)
		{
			urec = (UI_Refresher *) gGps->Find (item->ID_);
		}
	}

	tactical_add_victory_condition (urec->GetID (),NULL);

	gPopupMgr->CloseMenu ();
}

void SetMapSettings()
{
	C_PopupList *menu;

	menu=gPopupMgr->GetMenu(MAP_POP);
	if(menu)
	{
		// Legend stuff
		MenuToggleNamesCB(MID_LEG_NAMES,C_TYPE_LMOUSEUP,menu);
		MenuToggleBullseyeCB(MID_LEG_BULLSEYE,C_TYPE_LMOUSEUP,menu);

		// Objectives
		MenuToggleObjectiveCB(MID_INST_AF,C_TYPE_LMOUSEUP,menu);
		MenuToggleObjectiveCB(MID_INST_AD,C_TYPE_LMOUSEUP,menu);
		MenuToggleObjectiveCB(MID_INST_ARMY,C_TYPE_LMOUSEUP,menu);
		MenuToggleObjectiveCB(MID_INST_CCC,C_TYPE_LMOUSEUP,menu);
		MenuToggleObjectiveCB(MID_INST_POLITICAL,C_TYPE_LMOUSEUP,menu);
		MenuToggleObjectiveCB(MID_INST_INFRA,C_TYPE_LMOUSEUP,menu);
		MenuToggleObjectiveCB(MID_INST_LOG,C_TYPE_LMOUSEUP,menu);
		MenuToggleObjectiveCB(MID_INST_WARPROD,C_TYPE_LMOUSEUP,menu);
		MenuToggleObjectiveCB(MID_INST_NAV,C_TYPE_LMOUSEUP,menu);
		MenuToggleObjectiveCB(MID_INST_OTHER,C_TYPE_LMOUSEUP,menu);
		MenuToggleObjectiveCB(MID_INST_NAVAL,C_TYPE_LMOUSEUP,menu);
		MenuToggleObjectiveCB(MID_SHOW_VC,C_TYPE_LMOUSEUP,menu);

		// Units
		MenuToggleUnitCB(MID_UNITS_SQUAD_SQUADRON,C_TYPE_LMOUSEUP,menu);
		MenuToggleUnitCB(MID_UNITS_SQUAD_PACKAGE,C_TYPE_LMOUSEUP,menu);
		MenuToggleUnitCB(MID_UNITS_DIV,C_TYPE_LMOUSEUP,menu);
		MenuToggleUnitCB(MID_UNITS_BRIG,C_TYPE_LMOUSEUP,menu);
		MenuToggleUnitCB(MID_UNITS_BAT,C_TYPE_LMOUSEUP,menu);
		MenuToggleUnitCB(MID_UNITS_COMBAT,C_TYPE_LMOUSEUP,menu);
		MenuToggleUnitCB(MID_UNITS_AD,C_TYPE_LMOUSEUP,menu);
		MenuToggleUnitCB(MID_UNITS_SUPPORT,C_TYPE_LMOUSEUP,menu);
		MenuToggleUnitCB(MID_UNITS_SQUAD_FIGHTER,C_TYPE_LMOUSEUP,menu);
		MenuToggleUnitCB(MID_UNITS_SQUAD_FIGHTBOMB,C_TYPE_LMOUSEUP,menu);
		MenuToggleUnitCB(MID_UNITS_SQUAD_ATTACK,C_TYPE_LMOUSEUP,menu);
		MenuToggleUnitCB(MID_UNITS_SQUAD_BOMBER,C_TYPE_LMOUSEUP,menu);
		MenuToggleUnitCB(MID_UNITS_SQUAD_SUPPORT,C_TYPE_LMOUSEUP,menu);
		MenuToggleUnitCB(MID_UNITS_SQUAD_HELI,C_TYPE_LMOUSEUP,menu);
		if (g_nUnidentifiedInUI) MenuToggleUnitCB(MID_UNITS_SQUAD_UNKNOWN,C_TYPE_LMOUSEUP,menu); // 2002-02-21 ADDED BY S.G. For 'Unknown' type of flight
		MenuToggleUnitCB(MID_UNITS_NAVY_COMBAT,C_TYPE_LMOUSEUP,menu);
		MenuToggleUnitCB(MID_UNITS_NAVY_SUPPLY,C_TYPE_LMOUSEUP,menu);

		// Sams/Radar
		MenuSetCirclesCB(MID_OFF,C_TYPE_LMOUSEUP,menu);
	}
}

void SetupCampaignMenus()
{
	C_PopupList *menu;

	GameType=1;
	EditMode=0;
	// Map Menu
	menu=gPopupMgr->GetMenu(MAP_POP);
	if(menu)
	{
		menu->SetItemFlagBitOn(MID_ADD_FLIGHT,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOn(MID_ADD_PACKAGE,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOn(MID_ADD_BATTALION,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOn(MID_SHOW_VC,C_BIT_INVISIBLE);
	}

	// Objective Menu
	menu=gPopupMgr->GetMenu(OBJECTIVE_POP);
	if(menu)
	{
		menu->SetItemFlagBitOn(MID_ADD_FLIGHT,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOn(MID_ADD_PACKAGE,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOn(MID_ADD_BATTALION,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOn(MID_ADD_VC,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOn(MID_SET_OWNER,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOn(MID_SQUADRONS,C_BIT_INVISIBLE);
	}

	// Unit Menu
	menu=gPopupMgr->GetMenu(UNIT_POP);
	if(menu)
	{
		menu->SetItemFlagBitOn(MID_DELETE_UNIT,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOn(MID_ADD_FLIGHT,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOn(MID_ADD_PACKAGE,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOn(MID_ADD_BATTALION,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOn(MID_ADD_VC,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOn(MID_SET_OWNER,C_BIT_INVISIBLE);
	}

	// Unit Menu
	menu=gPopupMgr->GetMenu(NAVAL_POP);
	if(menu)
	{
		menu->SetItemFlagBitOn(MID_DELETE_UNIT,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOn(MID_ADD_FLIGHT,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOn(MID_ADD_PACKAGE,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOn(MID_ADD_BATTALION,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOn(MID_ADD_VC,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOn(MID_SET_OWNER,C_BIT_INVISIBLE);
	}

	// Unit Menu
	menu=gPopupMgr->GetMenu(AIRUNIT_MENU);
	if(menu)
	{
		menu->SetItemFlagBitOn(MID_DELETE_UNIT,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOn(MID_ADD_FLIGHT,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOn(MID_ADD_PACKAGE,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOn(MID_ADD_VC,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOn(MID_SET_OWNER,C_BIT_INVISIBLE);
	}

	// Package Menu
	menu=gPopupMgr->GetMenu(PACKAGE_POP);
	if(menu)
	{
		menu->SetItemFlagBitOn(MID_DELETE_UNIT,C_BIT_INVISIBLE);
	}

}

// Mode 0=Play,1=Edit)
void SetupTacEngMenus(short Mode)
{
	C_PopupList *menu;

	GameType=2;
	EditMode=Mode;

	// Map Menu
	menu=gPopupMgr->GetMenu(MAP_POP);
	if(menu)
	{
		menu->SetItemFlagBitOff(MID_ADD_FLIGHT,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOff(MID_ADD_PACKAGE,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOn(MID_ADD_FLIGHT,C_BIT_ENABLED);
		menu->SetItemFlagBitOn(MID_ADD_PACKAGE,C_BIT_ENABLED);

		menu->SetItemFlagBitOff(MID_SHOW_VC,C_BIT_INVISIBLE);
	}

	// Objective Menu
	menu=gPopupMgr->GetMenu(OBJECTIVE_POP);
	if(menu)
	{
		menu->SetItemFlagBitOff(MID_ADD_FLIGHT,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOff(MID_ADD_PACKAGE,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOn(MID_ADD_FLIGHT,C_BIT_ENABLED);
		menu->SetItemFlagBitOn(MID_ADD_PACKAGE,C_BIT_ENABLED);

		menu->SetItemFlagBitOff(MID_ADD_VC,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOff(MID_SET_OWNER,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOn(MID_SQUADRONS,C_BIT_INVISIBLE);
		if(EditMode)
		{
			menu->SetItemFlagBitOn(MID_ADD_VC,C_BIT_ENABLED);
			menu->SetItemFlagBitOn(MID_SET_OWNER,C_BIT_ENABLED);
			menu->SetItemFlagBitOff(MID_ADD_BATTALION,C_BIT_INVISIBLE);
		}
		else
		{
			menu->SetItemFlagBitOff(MID_ADD_VC,C_BIT_ENABLED);
			menu->SetItemFlagBitOff(MID_SET_OWNER,C_BIT_ENABLED);
			menu->SetItemFlagBitOn(MID_ADD_BATTALION,C_BIT_INVISIBLE);
		}
	}

	// Unit Menu
	menu=gPopupMgr->GetMenu(UNIT_POP);
	if(menu)
	{
		menu->SetItemFlagBitOff(MID_ADD_FLIGHT,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOff(MID_ADD_PACKAGE,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOn(MID_ADD_FLIGHT,C_BIT_ENABLED);
		menu->SetItemFlagBitOn(MID_ADD_PACKAGE,C_BIT_ENABLED);

		menu->SetItemFlagBitOff(MID_DELETE_UNIT,C_BIT_INVISIBLE);
		if(EditMode)
		{
			menu->SetItemFlagBitOn(MID_DELETE_UNIT,C_BIT_ENABLED);
			menu->SetItemFlagBitOn(MID_ADD_VC,C_BIT_ENABLED);
			menu->SetItemFlagBitOn(MID_SET_OWNER,C_BIT_ENABLED);
			menu->SetItemFlagBitOff(MID_ADD_BATTALION,C_BIT_INVISIBLE);
		}
		else
		{
			menu->SetItemFlagBitOff(MID_DELETE_UNIT,C_BIT_ENABLED);
			menu->SetItemFlagBitOff(MID_ADD_VC,C_BIT_ENABLED);
			menu->SetItemFlagBitOff(MID_SET_OWNER,C_BIT_ENABLED);
			menu->SetItemFlagBitOn(MID_ADD_BATTALION,C_BIT_INVISIBLE);
		}
	}

	// Unit Menu
	menu=gPopupMgr->GetMenu(NAVAL_POP);
	if(menu)
	{
		menu->SetItemFlagBitOff(MID_ADD_FLIGHT,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOff(MID_ADD_PACKAGE,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOn(MID_ADD_FLIGHT,C_BIT_ENABLED);
		menu->SetItemFlagBitOn(MID_ADD_PACKAGE,C_BIT_ENABLED);

		menu->SetItemFlagBitOff(MID_DELETE_UNIT,C_BIT_INVISIBLE);
		if(EditMode)
		{
			menu->SetItemFlagBitOn(MID_DELETE_UNIT,C_BIT_ENABLED);
			menu->SetItemFlagBitOn(MID_ADD_VC,C_BIT_ENABLED);
			menu->SetItemFlagBitOn(MID_SET_OWNER,C_BIT_ENABLED);
			menu->SetItemFlagBitOff(MID_ADD_BATTALION,C_BIT_INVISIBLE);
		}
		else
		{
			menu->SetItemFlagBitOff(MID_DELETE_UNIT,C_BIT_ENABLED);
			menu->SetItemFlagBitOff(MID_ADD_VC,C_BIT_ENABLED);
			menu->SetItemFlagBitOff(MID_SET_OWNER,C_BIT_ENABLED);
			menu->SetItemFlagBitOn(MID_ADD_BATTALION,C_BIT_INVISIBLE);
		}
	}

	// Unit Menu
	menu=gPopupMgr->GetMenu(AIRUNIT_MENU);
	if(menu)
	{
		menu->SetItemFlagBitOff(MID_ADD_FLIGHT,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOff(MID_ADD_PACKAGE,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOn(MID_ADD_FLIGHT,C_BIT_ENABLED);
		menu->SetItemFlagBitOn(MID_ADD_PACKAGE,C_BIT_ENABLED);

		menu->SetItemFlagBitOff(MID_DELETE_UNIT,C_BIT_INVISIBLE);
		if(EditMode)
		{
			menu->SetItemFlagBitOn(MID_DELETE_UNIT,C_BIT_ENABLED);
			menu->SetItemFlagBitOn(MID_ADD_VC,C_BIT_ENABLED);
			menu->SetItemFlagBitOn(MID_SET_OWNER,C_BIT_ENABLED);
			menu->SetItemFlagBitOff(MID_ADD_BATTALION,C_BIT_INVISIBLE);
		}
		else
		{
			menu->SetItemFlagBitOff(MID_DELETE_UNIT,C_BIT_ENABLED);
			menu->SetItemFlagBitOff(MID_ADD_VC,C_BIT_ENABLED);
			menu->SetItemFlagBitOff(MID_SET_OWNER,C_BIT_ENABLED);
			menu->SetItemFlagBitOn(MID_ADD_BATTALION,C_BIT_INVISIBLE);
		}
	}

	// Package Menu
	menu=gPopupMgr->GetMenu(PACKAGE_POP);
	if(menu)
	{
		menu->SetItemFlagBitOff(MID_DELETE_UNIT,C_BIT_INVISIBLE);
		if(EditMode)
			menu->SetItemFlagBitOn(MID_DELETE_UNIT,C_BIT_ENABLED);
		else
			menu->SetItemFlagBitOff(MID_DELETE_UNIT,C_BIT_ENABLED);
	}

	// VC Menu
	menu=gPopupMgr->GetMenu(VC_POP);
	if(menu)
	{
		if(EditMode)
		{
			menu->SetItemFlagBitOn(MID_DELETE_UNIT,C_BIT_ENABLED);
			menu->SetItemFlagBitOn(MID_SET_OWNER,C_BIT_ENABLED);
		}
		else
		{
			menu->SetItemFlagBitOff(MID_DELETE_UNIT,C_BIT_ENABLED);
			menu->SetItemFlagBitOff(MID_SET_OWNER,C_BIT_ENABLED);
		}
	}
}

void MapMenuOpenCB(C_Base *themenu,C_Base *caller)
{
	C_PopupList *menu;

	if(!themenu || !caller || !caller->Parent_)
		return;

	menu=(C_PopupList*)themenu;

	// Enable certain stuff for TE VC window
	if(caller->Parent_->GetID() == TAC_VC_WIN)
	{
		menu->SetItemFlagBitOn(MID_ADD_VC,C_BIT_ENABLED);
		menu->SetItemFlagBitOff(MID_ADD_FLIGHT,C_BIT_ENABLED);
		menu->SetItemFlagBitOff(MID_ADD_PACKAGE,C_BIT_ENABLED);
		menu->SetItemFlagBitOff(MID_ADD_BATTALION,C_BIT_ENABLED);
	}
	else
	{
		if(EditMode)
		{
			menu->SetItemFlagBitOn(MID_ADD_VC,C_BIT_ENABLED);
			menu->SetItemFlagBitOn(MID_ADD_FLIGHT,C_BIT_ENABLED);
			menu->SetItemFlagBitOn(MID_ADD_PACKAGE,C_BIT_ENABLED);
			menu->SetItemFlagBitOn(MID_ADD_BATTALION,C_BIT_ENABLED);
		}
		else
		{
			menu->SetItemFlagBitOff(MID_ADD_VC,C_BIT_ENABLED);
			menu->SetItemFlagBitOff(MID_ADD_FLIGHT,C_BIT_ENABLED);
			menu->SetItemFlagBitOn(MID_ADD_PACKAGE,C_BIT_ENABLED);
			menu->SetItemFlagBitOff(MID_ADD_BATTALION,C_BIT_ENABLED);
		}
	}
}

void OpenUnitMenuCB(C_Base *themenu,C_Base *caller)
{
	C_PopupList *menu;

	if(!themenu || !caller || !caller->Parent_)
		return;

	menu=(C_PopupList*)themenu;
	if(menu)
	{
		if(TeamInfo[1])
		{
			menu->SetItemLabel(MID_TEAM_1,TeamInfo[1]->GetName());
			menu->SetItemFlagBitOff(MID_TEAM_1,C_BIT_INVISIBLE);
		}
		else
			menu->SetItemFlagBitOn(MID_TEAM_1,C_BIT_INVISIBLE);
		if(TeamInfo[2])
		{
			menu->SetItemLabel(MID_TEAM_2,TeamInfo[2]->GetName());
			menu->SetItemFlagBitOff(MID_TEAM_2,C_BIT_INVISIBLE);
		}
		else
			menu->SetItemFlagBitOn(MID_TEAM_2,C_BIT_INVISIBLE);
		if(TeamInfo[3])
		{
			menu->SetItemLabel(MID_TEAM_3,TeamInfo[3]->GetName());
			menu->SetItemFlagBitOff(MID_TEAM_3,C_BIT_INVISIBLE);
		}
		else
			menu->SetItemFlagBitOn(MID_TEAM_3,C_BIT_INVISIBLE);
		if(TeamInfo[4])
		{
			menu->SetItemLabel(MID_TEAM_4,TeamInfo[4]->GetName());
			menu->SetItemFlagBitOff(MID_TEAM_4,C_BIT_INVISIBLE);
		}
		else
			menu->SetItemFlagBitOn(MID_TEAM_4,C_BIT_INVISIBLE);
		if(TeamInfo[5])
		{
			menu->SetItemLabel(MID_TEAM_5,TeamInfo[5]->GetName());
			menu->SetItemFlagBitOff(MID_TEAM_5,C_BIT_INVISIBLE);
		}
		else
			menu->SetItemFlagBitOn(MID_TEAM_5,C_BIT_INVISIBLE);
		if(TeamInfo[6])
		{
			menu->SetItemLabel(MID_TEAM_6,TeamInfo[6]->GetName());
			menu->SetItemFlagBitOff(MID_TEAM_6,C_BIT_INVISIBLE);
		}
		else
			menu->SetItemFlagBitOn(MID_TEAM_6,C_BIT_INVISIBLE);
		if(TeamInfo[7])
		{
			menu->SetItemLabel(MID_TEAM_7,TeamInfo[7]->GetName());
			menu->SetItemFlagBitOff(MID_TEAM_7,C_BIT_INVISIBLE);
		}
		else
			menu->SetItemFlagBitOn(MID_TEAM_7,C_BIT_INVISIBLE);
	}
}

void OpenNavalMenuCB(C_Base *themenu,C_Base *caller)
{
	C_PopupList *menu;

	if(!themenu || !caller || !caller->Parent_)
		return;

	menu=(C_PopupList*)themenu;
	if(menu)
	{
		if(TeamInfo[1])
		{
			menu->SetItemLabel(MID_TEAM_1,TeamInfo[1]->GetName());
			menu->SetItemFlagBitOff(MID_TEAM_1,C_BIT_INVISIBLE);
		}
		else
			menu->SetItemFlagBitOn(MID_TEAM_1,C_BIT_INVISIBLE);
		if(TeamInfo[2])
		{
			menu->SetItemLabel(MID_TEAM_2,TeamInfo[2]->GetName());
			menu->SetItemFlagBitOff(MID_TEAM_2,C_BIT_INVISIBLE);
		}
		else
			menu->SetItemFlagBitOn(MID_TEAM_2,C_BIT_INVISIBLE);
		if(TeamInfo[3])
		{
			menu->SetItemLabel(MID_TEAM_3,TeamInfo[3]->GetName());
			menu->SetItemFlagBitOff(MID_TEAM_3,C_BIT_INVISIBLE);
		}
		else
			menu->SetItemFlagBitOn(MID_TEAM_3,C_BIT_INVISIBLE);
		if(TeamInfo[4])
		{
			menu->SetItemLabel(MID_TEAM_4,TeamInfo[4]->GetName());
			menu->SetItemFlagBitOff(MID_TEAM_4,C_BIT_INVISIBLE);
		}
		else
			menu->SetItemFlagBitOn(MID_TEAM_4,C_BIT_INVISIBLE);
		if(TeamInfo[5])
		{
			menu->SetItemLabel(MID_TEAM_5,TeamInfo[5]->GetName());
			menu->SetItemFlagBitOff(MID_TEAM_5,C_BIT_INVISIBLE);
		}
		else
			menu->SetItemFlagBitOn(MID_TEAM_5,C_BIT_INVISIBLE);
		if(TeamInfo[6])
		{
			menu->SetItemLabel(MID_TEAM_6,TeamInfo[6]->GetName());
			menu->SetItemFlagBitOff(MID_TEAM_6,C_BIT_INVISIBLE);
		}
		else
			menu->SetItemFlagBitOn(MID_TEAM_6,C_BIT_INVISIBLE);
		if(TeamInfo[7])
		{
			menu->SetItemLabel(MID_TEAM_7,TeamInfo[7]->GetName());
			menu->SetItemFlagBitOff(MID_TEAM_7,C_BIT_INVISIBLE);
		}
		else
			menu->SetItemFlagBitOn(MID_TEAM_7,C_BIT_INVISIBLE);
	}
}

void ObjMenuOpenCB(C_Base *themenu,C_Base *caller)
{
	C_PopupList *menu;
	short isairbase=0;

	if(!themenu || !caller || !caller->Parent_)
		return;

	if(caller->_GetCType_() == _CNTL_DRAWLIST_)
	{
		MAPICONLIST *icon;

		icon=((C_DrawList*)caller)->GetLastItem();
		if(icon && icon->Owner)
			if(icon->Owner->GetType() == 1)
				isairbase=1;
	}
	else if(caller->_GetCType_() == _CNTL_MAPICON_)
	{
		if(caller->GetType() == 1)
			isairbase=1;
	}
	else if(caller->_GetCType_() == _CNTL_TREELIST_)
	{
	}

	menu=(C_PopupList*)themenu;

	if(isairbase) // Airbase
	{
		menu->SetItemFlagBitOn(MID_SQUADRONS,C_BIT_ENABLED);
		menu->SetItemFlagBitOff(MID_SQUADRONS,C_BIT_INVISIBLE);
		if(GameType == 1 || !EditMode)
			menu->SetItemFlagBitOff(MID_ADD_SQUADRON,C_BIT_ENABLED);
		else
			menu->SetItemFlagBitOn(MID_ADD_SQUADRON,C_BIT_ENABLED);
	}
	else
	{
		menu->SetItemFlagBitOff(MID_SQUADRONS,C_BIT_ENABLED);
		menu->SetItemFlagBitOn(MID_SQUADRONS,C_BIT_INVISIBLE);
		menu->SetItemFlagBitOff(MID_ADD_SQUADRON,C_BIT_ENABLED);
	}
	if(TeamInfo[1])
	{
		menu->SetItemLabel(MID_TEAM_1,TeamInfo[1]->GetName());
		menu->SetItemFlagBitOff(MID_TEAM_1,C_BIT_INVISIBLE);
	}
	else
		menu->SetItemFlagBitOn(MID_TEAM_1,C_BIT_INVISIBLE);
	if(TeamInfo[2])
	{
		menu->SetItemLabel(MID_TEAM_2,TeamInfo[2]->GetName());
		menu->SetItemFlagBitOff(MID_TEAM_2,C_BIT_INVISIBLE);
	}
	else
		menu->SetItemFlagBitOn(MID_TEAM_2,C_BIT_INVISIBLE);
	if(TeamInfo[3])
	{
		menu->SetItemLabel(MID_TEAM_3,TeamInfo[3]->GetName());
		menu->SetItemFlagBitOff(MID_TEAM_3,C_BIT_INVISIBLE);
	}
	else
		menu->SetItemFlagBitOn(MID_TEAM_3,C_BIT_INVISIBLE);
	if(TeamInfo[4])
	{
		menu->SetItemLabel(MID_TEAM_4,TeamInfo[4]->GetName());
		menu->SetItemFlagBitOff(MID_TEAM_4,C_BIT_INVISIBLE);
	}
	else
		menu->SetItemFlagBitOn(MID_TEAM_4,C_BIT_INVISIBLE);
	if(TeamInfo[5])
	{
		menu->SetItemLabel(MID_TEAM_5,TeamInfo[5]->GetName());
		menu->SetItemFlagBitOff(MID_TEAM_5,C_BIT_INVISIBLE);
	}
	else
		menu->SetItemFlagBitOn(MID_TEAM_5,C_BIT_INVISIBLE);
	if(TeamInfo[6])
	{
		menu->SetItemLabel(MID_TEAM_6,TeamInfo[6]->GetName());
		menu->SetItemFlagBitOff(MID_TEAM_6,C_BIT_INVISIBLE);
	}
	else
		menu->SetItemFlagBitOn(MID_TEAM_6,C_BIT_INVISIBLE);
	if(TeamInfo[7])
	{
		menu->SetItemLabel(MID_TEAM_7,TeamInfo[7]->GetName());
		menu->SetItemFlagBitOff(MID_TEAM_7,C_BIT_INVISIBLE);
	}
	else
		menu->SetItemFlagBitOn(MID_TEAM_7,C_BIT_INVISIBLE);
}

void HookupCampaignMenus()
{
	C_PopupList *menu;
	int			i;

	menu=gPopupMgr->GetMenu(MAP_POP);
	if(menu)
	{
		menu->SetOpenCallback(MapMenuOpenCB);

		menu->SetCallback(MID_RECON,MenuReconCB);
		menu->SetCallback(MID_ADD_FLIGHT,MenuAddUnitCB);
		menu->SetCallback(MID_ADD_PACKAGE,MenuAddUnitCB);
		menu->SetCallback(MID_ADD_BATTALION,MenuAddUnitCB);
		// Legend stuff
		menu->SetCallback(MID_LEG_NAMES,MenuToggleNamesCB);
		menu->SetCallback(MID_LEG_BULLSEYE,MenuToggleBullseyeCB);

		// Objectives
		menu->SetCallback(MID_INST_AF,MenuToggleObjectiveCB);
		menu->SetCallback(MID_INST_AD,MenuToggleObjectiveCB);
		menu->SetCallback(MID_INST_ARMY,MenuToggleObjectiveCB);
		menu->SetCallback(MID_INST_CCC,MenuToggleObjectiveCB);
		menu->SetCallback(MID_INST_POLITICAL,MenuToggleObjectiveCB);
		menu->SetCallback(MID_INST_INFRA,MenuToggleObjectiveCB);
		menu->SetCallback(MID_INST_LOG,MenuToggleObjectiveCB);
		menu->SetCallback(MID_INST_WARPROD,MenuToggleObjectiveCB);
		menu->SetCallback(MID_INST_NAV,MenuToggleObjectiveCB);
		menu->SetCallback(MID_INST_OTHER,MenuToggleObjectiveCB);
		menu->SetCallback(MID_INST_NAVAL,MenuToggleObjectiveCB);
		menu->SetCallback(MID_SHOW_VC,MenuToggleObjectiveCB);

		// Units
		menu->SetCallback(MID_UNITS_SQUAD_SQUADRON,MenuToggleUnitCB);
		menu->SetCallback(MID_UNITS_SQUAD_PACKAGE,MenuToggleUnitCB);
		menu->SetCallback(MID_UNITS_DIV,MenuToggleUnitCB);
		menu->SetCallback(MID_UNITS_BRIG,MenuToggleUnitCB);
		menu->SetCallback(MID_UNITS_BAT,MenuToggleUnitCB);
		menu->SetCallback(MID_UNITS_COMBAT,MenuToggleUnitCB);
		menu->SetCallback(MID_UNITS_AD,MenuToggleUnitCB);
		menu->SetCallback(MID_UNITS_SUPPORT,MenuToggleUnitCB);
		menu->SetCallback(MID_UNITS_SQUAD_FIGHTER,MenuToggleUnitCB);
		menu->SetCallback(MID_UNITS_SQUAD_FIGHTBOMB,MenuToggleUnitCB);
		menu->SetCallback(MID_UNITS_SQUAD_ATTACK,MenuToggleUnitCB);
		menu->SetCallback(MID_UNITS_SQUAD_BOMBER,MenuToggleUnitCB);
		menu->SetCallback(MID_UNITS_SQUAD_SUPPORT,MenuToggleUnitCB);
		menu->SetCallback(MID_UNITS_SQUAD_HELI,MenuToggleUnitCB);
		if (g_nUnidentifiedInUI) menu->SetCallback(MID_UNITS_SQUAD_UNKNOWN,MenuToggleUnitCB);// 2002-02-21 ADDED BY S.G. For 'Unknown' type of flight
		menu->SetCallback(MID_UNITS_NAVY_COMBAT,MenuToggleUnitCB);
		menu->SetCallback(MID_UNITS_NAVY_SUPPLY,MenuToggleUnitCB);

		// Sams/Radar
		menu->SetCallback(MID_OFF,MenuSetCirclesCB);
		menu->SetCallback(MID_CIRCLE_SAM_LOW,MenuSetCirclesCB);
		menu->SetCallback(MID_CIRCLE_SAM_HIGH,MenuSetCirclesCB);
		menu->SetCallback(MID_CIRCLE_RADAR_LOW,MenuSetCirclesCB);
		menu->SetCallback(MID_CIRCLE_RADAR_HIGH,MenuSetCirclesCB);
	}

	menu=gPopupMgr->GetMenu(OBJECTIVE_POP);
	if(menu)
	{
		menu->SetOpenCallback(ObjMenuOpenCB);

		menu->SetCallback(MID_RECON,MenuReconCB);
		menu->SetCallback(MID_STATUS,MenuStatusCB);
		menu->SetCallback(MID_ADD_FLIGHT,MenuAddUnitCB);
		menu->SetCallback(MID_ADD_PACKAGE,MenuAddUnitCB);
		menu->SetCallback(MID_ADD_BATTALION,MenuAddUnitCB);
		menu->SetCallback(MID_ADD_SQUADRON,MenuAddUnitCB);
		menu->SetCallback(MID_ADD_VC,MenuAddVCCB);
		menu->SetCallback(MID_TEAM_0,MenuSetOwnerCB);
		menu->SetCallback(MID_TEAM_1,MenuSetOwnerCB);
		menu->SetCallback(MID_TEAM_2,MenuSetOwnerCB);
		menu->SetCallback(MID_TEAM_3,MenuSetOwnerCB);
		menu->SetCallback(MID_TEAM_4,MenuSetOwnerCB);
		menu->SetCallback(MID_TEAM_5,MenuSetOwnerCB);
		menu->SetCallback(MID_TEAM_6,MenuSetOwnerCB);
		menu->SetCallback(MID_TEAM_7,MenuSetOwnerCB);
	}

	menu=gPopupMgr->GetMenu(SQUADRON_POP);
	if(menu)
	{
		menu->SetCallback(MID_RECON,MenuReconCB);
		menu->SetCallback(MID_ADD_FLIGHT,MenuAddUnitCB);
		menu->SetCallback(MID_ADD_PACKAGE,MenuAddUnitCB);
		menu->SetCallback(MID_ADD_BATTALION,MenuAddUnitCB);
		menu->SetCallback(MID_STATUS,MenuUnitStatusCB);
		menu->SetCallback(MID_DELETE_UNIT,MenuUnitDeleteCB);
		menu->SetCallback(MID_ADD_VC,MenuAddVCCB);
	}

	menu=gPopupMgr->GetMenu(UNIT_POP);
	if(menu)
	{
		menu->SetOpenCallback(OpenUnitMenuCB);
		menu->SetCallback(MID_RECON,MenuReconCB);
		menu->SetCallback(MID_ADD_FLIGHT,MenuAddUnitCB);
		menu->SetCallback(MID_ADD_PACKAGE,MenuAddUnitCB);
		menu->SetCallback(MID_ADD_BATTALION,MenuAddUnitCB);
		menu->SetCallback(MID_STATUS,MenuUnitStatusCB);
		menu->SetCallback(MID_DELETE_UNIT,MenuUnitDeleteCB);
		menu->SetCallback(MID_ADD_VC,MenuAddVCCB);
		menu->SetCallback(MID_TEAM_0,MenuSetOwnerCB);
		menu->SetCallback(MID_TEAM_1,MenuSetOwnerCB);
		menu->SetCallback(MID_TEAM_2,MenuSetOwnerCB);
		menu->SetCallback(MID_TEAM_3,MenuSetOwnerCB);
		menu->SetCallback(MID_TEAM_4,MenuSetOwnerCB);
		menu->SetCallback(MID_TEAM_5,MenuSetOwnerCB);
		menu->SetCallback(MID_TEAM_6,MenuSetOwnerCB);
		menu->SetCallback(MID_TEAM_7,MenuSetOwnerCB);
	}

	menu=gPopupMgr->GetMenu(AIRUNIT_MENU);
	if(menu)
	{
		menu->SetOpenCallback(OpenUnitMenuCB);
		menu->SetCallback(MID_RECON,MenuReconCB);
		menu->SetCallback(MID_ADD_FLIGHT,MenuAddUnitCB);
		menu->SetCallback(MID_ADD_PACKAGE,MenuAddUnitCB);
		menu->SetCallback(MID_ADD_BATTALION,MenuAddUnitCB);
		menu->SetCallback(MID_STATUS,MenuUnitStatusCB);
		menu->SetCallback(MID_DELETE_UNIT,MenuUnitDeleteCB);
		menu->SetCallback(MID_ADD_VC,MenuAddVCCB);
		menu->SetCallback(MID_TEAM_0,MenuSetOwnerCB);
		menu->SetCallback(MID_TEAM_1,MenuSetOwnerCB);
		menu->SetCallback(MID_TEAM_2,MenuSetOwnerCB);
		menu->SetCallback(MID_TEAM_3,MenuSetOwnerCB);
		menu->SetCallback(MID_TEAM_4,MenuSetOwnerCB);
		menu->SetCallback(MID_TEAM_5,MenuSetOwnerCB);
		menu->SetCallback(MID_TEAM_6,MenuSetOwnerCB);
		menu->SetCallback(MID_TEAM_7,MenuSetOwnerCB);
	}

	menu=gPopupMgr->GetMenu(NAVAL_POP);
	if(menu)
	{
		menu->SetOpenCallback(OpenNavalMenuCB);
		menu->SetCallback(MID_RECON,MenuReconCB);
		menu->SetCallback(MID_ADD_FLIGHT,MenuAddUnitCB);
		menu->SetCallback(MID_ADD_PACKAGE,MenuAddUnitCB);
		menu->SetCallback(MID_ADD_BATTALION,MenuAddUnitCB);
		menu->SetCallback(MID_STATUS,MenuUnitStatusCB);
		menu->SetCallback(MID_DELETE_UNIT,MenuUnitDeleteCB);
		menu->SetCallback(MID_ADD_VC,MenuAddVCCB);
		menu->SetCallback(MID_TEAM_0,MenuSetOwnerCB);
		menu->SetCallback(MID_TEAM_1,MenuSetOwnerCB);
		menu->SetCallback(MID_TEAM_2,MenuSetOwnerCB);
		menu->SetCallback(MID_TEAM_3,MenuSetOwnerCB);
		menu->SetCallback(MID_TEAM_4,MenuSetOwnerCB);
		menu->SetCallback(MID_TEAM_5,MenuSetOwnerCB);
		menu->SetCallback(MID_TEAM_6,MenuSetOwnerCB);
		menu->SetCallback(MID_TEAM_7,MenuSetOwnerCB);
	}

	menu=gPopupMgr->GetMenu(PACKAGE_POP);
	if(menu)
	{
		menu->SetCallback(MID_RECON,MenuReconCB);
		menu->SetCallback(MID_SHOW_FLIGHTS,MenuEditPackageCB);
		menu->SetCallback(MID_DELETE_UNIT,MenuUnitDeleteCB);
		menu->SetCallback(MID_ADD_FLIGHT,MenuAddUnitCB);
		menu->SetCallback(MID_ADD_PACKAGE,MenuAddUnitCB);
		menu->SetCallback(MID_ADD_VC,MenuAddVCCB);
	}
	menu=gPopupMgr->GetMenu(STEERPOINT_POP);
	if(menu)
	{
		menu->SetOpenCallback(SteerPointMenuOpenCB);
		
		menu->SetCallback(MID_ADD_STPT,MenuAddWPCB);
		menu->SetCallback(MID_DELETE_STPT,MenuDeleteWPCB);
		menu->SetCallback(MID_RECON,MenuReconCB);

		// Main
		menu->SetCallback(MID_DETAILS,MenuOpenWpWindowCB);
		menu->SetCallback(MID_LOCK_TOS,MenuLockCB);
		menu->SetCallback(MID_LOCK_SPEED,MenuLockCB);

		// Climb Menu
		menu->SetCallback(CLIMB_IMMEDIATE,MenuClimbCB);
		menu->SetCallback(CLIMB_DELAY,MenuClimbCB);

		// Formation Menu
		for (i=1; i<9; i++)
			menu->SetCallback(i,MenuFormationCB);

		// Enroute menu (hand add the valid ones)
		menu->AddItem(WP_NOTHING | 0x100, C_TYPE_RADIO, WPActStr[39], MID_ENR_ACTION);
		menu->SetCallback(WP_NOTHING | 0x100,MenuEnrouteCB);
		menu->SetItemGroup(WP_NOTHING | 0x100,3);
		menu->AddItem(WP_CA | 0x100, C_TYPE_RADIO, WPActStr[WP_CA], MID_ENR_ACTION);
		menu->SetCallback(WP_CA | 0x100,MenuEnrouteCB);
		menu->SetItemGroup(WP_CA | 0x100,3);
		menu->AddItem(WP_ESCORT | 0x100, C_TYPE_RADIO, WPActStr[WP_ESCORT], MID_ENR_ACTION);
		menu->SetCallback(WP_ESCORT | 0x100,MenuEnrouteCB);
		menu->SetItemGroup(WP_ESCORT | 0x100,3);
		menu->AddItem(WP_SEAD | 0x100, C_TYPE_RADIO, WPActStr[WP_SEAD], MID_ENR_ACTION);
		menu->SetCallback(WP_SEAD | 0x100,MenuEnrouteCB);
		menu->SetItemGroup(WP_SEAD | 0x100,3);
		menu->AddItem(WP_SAD | 0x100, C_TYPE_RADIO, WPActStr[WP_SAD], MID_ENR_ACTION);
		menu->SetCallback(WP_SAD | 0x100,MenuEnrouteCB);
		menu->SetItemGroup(WP_SAD | 0x100,3);
		menu->AddItem(WP_ELINT | 0x100, C_TYPE_RADIO, WPActStr[WP_ELINT], MID_ENR_ACTION);
		menu->SetCallback(WP_ELINT | 0x100,MenuEnrouteCB);
		menu->SetItemGroup(WP_ELINT | 0x100,3);
		menu->AddItem(WP_TANKER | 0x100, C_TYPE_RADIO, WPActStr[WP_TANKER], MID_ENR_ACTION);
		menu->SetCallback(WP_TANKER | 0x100,MenuEnrouteCB);
		menu->SetItemGroup(WP_TANKER | 0x100,3);
		menu->AddItem(WP_JAM | 0x100, C_TYPE_RADIO, WPActStr[WP_JAM], MID_ENR_ACTION);
		menu->SetCallback(WP_JAM | 0x100,MenuEnrouteCB);
		menu->SetItemGroup(WP_JAM | 0x100,3);
		menu->AddItem(WP_ASW | 0x100, C_TYPE_RADIO, WPActStr[WP_ASW], MID_ENR_ACTION);
		menu->SetCallback(WP_ASW | 0x100,MenuEnrouteCB);
		menu->SetItemGroup(WP_ASW | 0x100,3);
		menu->AddItem(WP_RECON | 0x100, C_TYPE_RADIO, WPActStr[WP_RECON], MID_ENR_ACTION);
		menu->SetCallback(WP_RECON | 0x100,MenuEnrouteCB);
		menu->SetItemGroup(WP_RECON | 0x100,3);

		// Action Menu
		for (i=0; i<=WP_FAC; i++)
		{
			if (!i)
				menu->AddItem(i | 0x200, C_TYPE_RADIO, WPActStr[39], MID_ACTION);
			else
				menu->AddItem(i | 0x200, C_TYPE_RADIO, WPActStr[i], MID_ACTION);
			menu->SetCallback(i | 0x200,MenuActionCB);
			menu->SetItemGroup(i | 0x200,4);
		}
	}
}
