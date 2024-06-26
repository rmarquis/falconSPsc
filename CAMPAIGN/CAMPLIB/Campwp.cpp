
#include "campwp.h"
#include "listadt.h"
#include "find.h"
#include "tactics.h"
#include "Graphics\Include\TMap.h"

#define		WP_HAVE_DEPTIME		0x01
#define		WP_HAVE_TARGET		0x02

#ifdef CAMPTOOL
// Renaming tool stuff
extern VU_ID_NUMBER RenameTable[65536];
extern int gRenameIds;
#endif

#ifdef USE_SH_POOLS
MEM_POOL	WayPointClass::pool;
#endif

WayPointClass::WayPointClass (void)
	{
  	GridX = GridY = 0;
	GridZ = 0;
  	Arrive = 0;
  	Depart = 0;
  	Action = 0;
	RouteAction = 0;
	Formation = 0;
	TargetBuilding = 255;
	TargetID = FalconNullId;
	Flags = 0;
	Speed = 0.0;
	PrevWP = NULL;
  	NextWP = NULL;
	Tactic = 0;
	}

WayPointClass::WayPointClass (GridIndex x, GridIndex y, int alt, int speed, CampaignTime arr, CampaignTime station, uchar action, int flags)
	{
	GridX = x;
	GridY = y;
	if (alt > GRIDZ_SCALE_FACTOR)
		GridZ = (short)(alt/GRIDZ_SCALE_FACTOR);
	else
		GridZ = (short)alt;
	Arrive = arr;
	Depart = arr + station;
	Action = RouteAction = action;
	TargetBuilding = 255;
	Formation = 0;
	Flags = (ushort)flags;
	Speed = (float)speed;
	PrevWP = NULL;
	NextWP = NULL;
	Tactic = 0;
	}

WayPointClass::WayPointClass (VU_BYTE **stream)
	{
	uchar	haves;
	memcpy(&haves, *stream, sizeof(uchar));				*stream += sizeof(uchar);
	memcpy(&GridX, *stream, sizeof(GridIndex));			*stream += sizeof(GridIndex);
	memcpy(&GridY, *stream, sizeof(GridIndex));			*stream += sizeof(GridIndex);
	memcpy(&GridZ, *stream, sizeof(short));				*stream += sizeof(short);
	memcpy(&Arrive, *stream, sizeof(CampaignTime));		*stream += sizeof(CampaignTime);
	memcpy(&Action, *stream, sizeof(uchar));			*stream += sizeof(uchar);
	memcpy(&RouteAction, *stream, sizeof(uchar));		*stream += sizeof(uchar);
	memcpy(&Formation, *stream, sizeof(uchar));			*stream += sizeof(uchar);
	if (gCampDataVersion < 72) {
	    Flags = 0;
		memcpy(&Flags, *stream, sizeof(short));			*stream += sizeof(short);
	}
	else {
		memcpy(&Flags, *stream, sizeof(ulong));			*stream  += sizeof(ulong);
	}
	if (haves & WP_HAVE_TARGET)
		{
		memcpy(&TargetID, *stream, sizeof(VU_ID));			*stream += sizeof(VU_ID);
#ifdef DEBUG
		TargetID.num_ &= 0xffff;
#endif
		memcpy(&TargetBuilding, *stream, sizeof(uchar));	*stream += sizeof(uchar);
		}
	else
		{
		TargetID = FalconNullId;
		TargetBuilding = 255;
		}
	if (haves & WP_HAVE_DEPTIME)
		{
		memcpy(&Depart, *stream, sizeof(CampaignTime));		*stream += sizeof(CampaignTime);
		}
	else
		Depart = Arrive;
	PrevWP = NULL;
  	NextWP = NULL;
	Tactic = 0;
	}

WayPointClass::WayPointClass (FILE* fp)
	{
	uchar	haves;
	fread(&haves, sizeof(uchar), 1, fp);		
	fread(&GridX, sizeof(GridIndex), 1, fp);		
	fread(&GridY, sizeof(GridIndex), 1, fp);		
	fread(&GridZ, sizeof(short), 1, fp);		
	fread(&Arrive, sizeof(CampaignTime), 1, fp);	
	fread(&Action, sizeof(uchar), 1, fp);			
	fread(&RouteAction, sizeof(uchar), 1, fp);	
	fread(&Formation, sizeof(uchar), 1, fp);		
	if (gCampDataVersion < 72) {
	    Flags = 0;
	    fread(&Flags, sizeof(short), 1, fp);
	}
	else {
	    fread(&Flags, sizeof(ulong), 1, fp);
	}
	if (haves & WP_HAVE_TARGET)
		{
		fread(&TargetID, sizeof(VU_ID), 1, fp);		
#ifdef DEBUG
		TargetID.num_ &= 0xffff;
#endif
		fread(&TargetBuilding, sizeof(uchar), 1, fp);
		}
	else
		{
		TargetID = FalconNullId;
		TargetBuilding = 255;
		}
	if (haves & WP_HAVE_DEPTIME)
		fread(&Depart, sizeof(CampaignTime), 1, fp);	
	else
		Depart = Arrive;
//	fread(&Tactic, sizeof(short), 1, fp);			
	::SetWPSpeed(this);
  	PrevWP = NULL;
	NextWP = NULL;
	Tactic = 0;
	}

int WayPointClass::SaveSize (void)
	{
	int		size = 0;
	if (TargetID != FalconNullId)
		size += sizeof(VU_ID) + sizeof(uchar);
	if (Depart != Arrive)
		size += sizeof(CampaignTime);
	size += sizeof(uchar)
		+ sizeof(GridIndex)
		+ sizeof(GridIndex)
		+ sizeof(short)
		+ sizeof(CampaignTime)
		+ sizeof(uchar)
		+ sizeof(uchar)
		+ sizeof(uchar)
		+ sizeof(ulong);
	return size;
	}

int WayPointClass::Save (VU_BYTE **stream)
	{
#ifdef _DEBUG
	VU_BYTE *start = *stream;
#endif

	uchar	haves = 0;
	if (TargetID != FalconNullId)
		haves |= WP_HAVE_TARGET;
	if (Depart != Arrive)
		haves |= WP_HAVE_DEPTIME;
	memcpy(*stream, &haves, sizeof(uchar));					*stream += sizeof(uchar);
	memcpy(*stream, &GridX, sizeof(GridIndex));				*stream += sizeof(GridIndex);
	memcpy(*stream, &GridY, sizeof(GridIndex));				*stream += sizeof(GridIndex);
	memcpy(*stream, &GridZ, sizeof(short));					*stream += sizeof(short);
	memcpy(*stream, &Arrive, sizeof(CampaignTime));			*stream += sizeof(CampaignTime);
	memcpy(*stream, &Action, sizeof(uchar));				*stream += sizeof(uchar);
	memcpy(*stream, &RouteAction, sizeof(uchar));			*stream += sizeof(uchar);
	memcpy(*stream, &Formation, sizeof(uchar));				*stream += sizeof(uchar);
        memcpy(*stream, &Flags, sizeof(ulong));			*stream  += sizeof(ulong);


	if (haves & WP_HAVE_TARGET)
		{
#ifdef CAMPTOOL
		if (gRenameIds)
			TargetID.num_ = RenameTable[TargetID.num_];
#endif
		memcpy(*stream, &TargetID, sizeof(VU_ID));			*stream += sizeof(VU_ID);
		memcpy(*stream, &TargetBuilding, sizeof(uchar));	*stream += sizeof(uchar);
		}
	if (haves & WP_HAVE_DEPTIME)
		{
		memcpy(*stream, &Depart, sizeof(CampaignTime));		*stream += sizeof(CampaignTime);
		}
//	memcpy(*stream, &Tactic, sizeof(short));			*stream += sizeof(short);
#ifdef _DEBUG
	ShiAssert(*stream - start == SaveSize()); // keep us honest JPO
#endif
	return SaveSize();
	}

int WayPointClass::Save (FILE* fp)
	{
	uchar	haves = 0;

	if (!fp)
		return 0;
	if (TargetID != FalconNullId)
		haves |= WP_HAVE_TARGET;
	if (Depart != Arrive)
		haves |= WP_HAVE_DEPTIME;
	fwrite(&haves, sizeof(uchar), 1, fp);
	fwrite(&GridX, sizeof(GridIndex), 1, fp);		
	fwrite(&GridY, sizeof(GridIndex), 1, fp);		
	fwrite(&GridZ, sizeof(short), 1, fp);		
	fwrite(&Arrive, sizeof(CampaignTime), 1, fp);	
	fwrite(&Action, sizeof(uchar), 1, fp);			
	fwrite(&RouteAction, sizeof(uchar), 1, fp);	
	fwrite(&Formation, sizeof(uchar), 1, fp);		
	fwrite(&Flags, sizeof(ulong), 1, fp);			
	if (haves & WP_HAVE_TARGET)
		{
#ifdef CAMPTOOL
		if (gRenameIds)
			TargetID.num_ = RenameTable[TargetID.num_];
#endif
		fwrite(&TargetID, sizeof(VU_ID), 1, fp);		
		fwrite(&TargetBuilding, sizeof(uchar), 1, fp);
		}
	if (haves & WP_HAVE_DEPTIME)
		fwrite(&Depart, sizeof(CampaignTime), 1, fp);	
//	fwrite(&Tactic, sizeof(short), 1, fp);			
	return 1;
	}

void WayPointClass::SetWPTimes (CampaignTime t)
	{
	Depart = t + (Depart - Arrive);
	Arrive = t;
	}

void WayPointClass::CloneWP (WayPoint w)
{
	GridX = w->GridX;
	GridY = w->GridY;
	GridZ = w->GridZ;
	Arrive = w->Arrive;
	Depart = w->Depart;
	Action = w->Action;
	RouteAction = w->RouteAction;
	Formation = w->Formation;
	Tactic = w->Tactic;
	Flags = w->Flags;
	Speed = 0.0;
	PrevWP = NULL;
	NextWP = NULL;
	TargetBuilding = w->TargetBuilding;
	TargetID = w->TargetID;
}

float WayPointClass::DistanceTo (WayPoint w)
	{
	return Distance(GridX, GridY, w->GridX, w->GridY);
	}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void WayPointClass::UnlinkNextWP (void)
{
	if (NextWP)
	{
		NextWP->PrevWP = NULL;
		NextWP = NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void WayPointClass::SetNextWP (WayPointClass *w)
{
	float
		dx, dy, dz,
		sx, sy, sz,
		delta_x, delta_y,
		time,
		dist,
		speed;

	if (NextWP)
	{
		MonoPrint ("Trying to Set Next WP on a waypoint that already has a next waypoint\n");
	}
	else
	{
		NextWP = w;

		if (w)
		{
			w->PrevWP = this;

			if (NextWP)
			{
				NextWP->GetLocation (&dx, &dy, &dz);
				GetLocation (&sx, &sy, &sz);

				delta_x = dx-sx;
				delta_y = dy-sy;

				dist = (float)sqrt (delta_x * delta_x + delta_y * delta_y);
				time = (float)(NextWP->GetWPArrivalTime() - GetWPArrivalTime()) / CampaignHours; // Hours

				if (time != 0.0) // JB 010413 CTD
					speed = (dist / time) / NM_TO_FT;
				else // JB 010413 CTD
					speed = 0.0; // JB 010413 CTD

				if (speed > 0)
				{
					NextWP->Speed = speed;
				}
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void WayPointClass::SetPrevWP (WayPointClass *w)
{
	if (PrevWP)
	{
		MonoPrint ("Trying to Set Prev WP on a waypoint that already has a previous waypoint\n");
	}
	else
	{
		PrevWP = w;

		if (w)
		{
			w->NextWP = this;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void WayPointClass::SplitWP (void)
{
	WayPointClass
		*first_wp,
		*new_wp,
		*second_wp;

	GridIndex
		x,
		y,
		z;

	CampaignTime
		time;

	if (!NextWP)
	{
		first_wp = PrevWP;
		second_wp = this;
	}
	else
	{
		first_wp = this;
		second_wp = NextWP;
	}

	x = (short)((first_wp->GridX + second_wp->GridX) / 2);
	y = (short)((first_wp->GridY + second_wp->GridY) / 2);
	if (second_wp->GetWPFlags() & WPF_HOLDCURRENT)
		z = first_wp->GridZ;
	else
		z = second_wp->GridZ;
	time = (second_wp->GetWPArrivalTime() - first_wp->GetWPDepartureTime()) / 2;

	new_wp = new WayPointClass;

	new_wp->PrevWP = first_wp;
	first_wp->NextWP = new_wp;

	new_wp->NextWP = second_wp;
	second_wp->PrevWP = new_wp;

	new_wp->GridX = x;
	new_wp->GridY = y;
	new_wp->GridZ = z;
	new_wp->SetWPTimes(time);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Insert a waypoint after this.

void WayPointClass::InsertWP (WayPointClass *new_wp)
{
	WayPointClass	*last = new_wp;

	new_wp->PrevWP = this;

	// If new_wp has a next, find the last in the list
	while (last->NextWP)
		last = last->NextWP;

	last->NextWP = NextWP;

	if (NextWP)
		NextWP->PrevWP = last;

	NextWP = new_wp;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// delete this.

void WayPointClass::DeleteWP (void)
{
	if (PrevWP)
	{
		PrevWP->NextWP = NextWP;
	}

	if (NextWP)
	{
		NextWP->PrevWP = PrevWP;
	}

	delete this;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// =============================================
// Global functions 
// =============================================

void DeleteWPList (WayPoint w)
	{
	WayPoint		t;

	while (w != NULL)
		{
		t = w;
		w = w->GetNextWP();
		delete t;
		}
	}

// Sets a set of waypoint times to start at waypoint w at time start.
CampaignTime SetWPTimes (WayPoint w, CampaignTime start, int speed, int flags)
	{
	CampaignTime	mission_time,length,land=0;
	GridIndex		x,y,nx,ny;
	CampaignTime	station;

	if (!w)
		return 0;

	// If the first waypoint passed is an alternate - assume we want it's time set
	if (w->GetWPFlags() & WPF_ALTERNATE)
		flags |= WPTS_SET_ALTERNATE_TIMES;

	w->GetWPLocation(&x,&y);
	mission_time = length = start;
	while (w)
		{
		w->GetWPLocation(&nx,&ny);
		mission_time += TimeToArrive(Distance(x,y,nx,ny),(float)speed);
		if (flags & WPTS_KEEP_DEPARTURE_TIMES)
			{
			w->SetWPArrive(mission_time);
			if (w->GetWPDepartureTime() < w->GetWPArrivalTime())
				w->SetWPDepartTime(mission_time);
			}
		else
			{
			station = w->GetWPStationTime();
			w->SetWPArrive(mission_time);
			w->SetWPStationTime(station);
			}
		mission_time = w->GetWPDepartureTime();
		if (mission_time > land && (w->GetWPFlags() & WPF_LAND))
			land = mission_time;
		if ((w->GetWPFlags() & WPF_ALTERNATE) && !(flags & WPTS_SET_ALTERNATE_TIMES))
			w->SetWPTimes(0);
		w = w->GetNextWP();
		x = nx; y = ny;
		}
	length = mission_time - length;
	return length;
	}

// Shifts a set of waypoints by time delta.
CampaignTime SetWPTimes (WayPoint w, long delta, int flags)
	{
	CampaignTime	mission_time,length,land=0;
	int				station;

	if (!w)
		return 0;

	// If the first waypoint passed is an alternate - assume we want it's time set
	if (w->GetWPFlags() & WPF_ALTERNATE)
		flags |= WPTS_SET_ALTERNATE_TIMES;
	
	length = w->GetWPArrivalTime() + delta;
	while (w)
		{
		mission_time = w->GetWPArrivalTime() + delta;
		if (flags & WPTS_KEEP_DEPARTURE_TIMES)
			{
			w->SetWPArrive(mission_time);
			if (w->GetWPDepartureTime() < w->GetWPArrivalTime())
				w->SetWPDepartTime(mission_time);
			}
		else
			{
			station = w->GetWPStationTime();
			w->SetWPArrive(mission_time);
			w->SetWPStationTime(station);
			}
		mission_time = w->GetWPDepartureTime();
		if (mission_time > land && (w->GetWPFlags() & WPF_LAND))
			land = mission_time;
		if ((w->GetWPFlags() & WPF_ALTERNATE) && !(flags & WPTS_SET_ALTERNATE_TIMES))
			w->SetWPTimes(0);
		w = w->GetNextWP();
		}
	length = land - length;
	return length;
	}

// Sets a set of waypoint times to start at waypoint w as soon as we can get there from x,y.
CampaignTime SetWPTimes (WayPoint w, GridIndex x, GridIndex y, int speed, int flags)
	{
	CampaignTime	mission_time,length;
	GridIndex		nx,ny;

	if (!w)
		return 0;
	w->GetWPLocation(&nx,&ny);
	mission_time = TimeToArrive(Distance(x,y,nx,ny),(float)speed);
	length = SetWPTimes(w, mission_time + Camp_GetCurrentTime(), speed, flags) + mission_time;
	return length;
	}

WayPoint CloneWPToList (WayPoint w, WayPoint stop)
	{
	WayPoint		nw,lw,list;

	lw = list = NULL;
	while ((w) && (w != stop))
		{
		nw = new WayPointClass();
		nw->CloneWP(w);
		if (!list)
			list = nw;
		if (lw)
			lw->InsertWP(nw);
		lw = nw;
		w = w->GetNextWP();
		}
	return list;
	}

WayPoint CloneWPList (WayPoint w)
	{
	WayPoint		nw,lw,list;

	lw = list = NULL;
	while (w)
		{
		nw = new WayPointClass();
		nw->CloneWP(w);
		if (!list)
			list = nw;
		if (lw)
			lw->InsertWP(nw);
		lw = nw;
		w = w->GetNextWP();
		}
	return list;
	}

WayPoint CloneWPList (WayPointClass wps[], int waypoints)
	{
	WayPoint		w,nw,lw,list;
	int			i;

	nw = w = lw = list = NULL;
	if (waypoints > 0)
		{
		for (i=0; i<waypoints; i++)
			{
			w = &wps[i];
			nw = new WayPointClass();
			nw->CloneWP(w);
			if (!list)
				list = nw;
			if (lw)
				lw->SetNextWP(nw);
			lw = nw;
			}
		}
	return list;
	}

// This assumes the graphic's altitude map is loaded. Z is the wp's stored altitude
float AdjustAltitudeForMSL_AGL (float x, float y, float z)
	{
	float		terrain_level = TheMap.GetMEA(x,y);

// 2001-03-24 MODIFIED BY S.G. THIS IS COMPLETELY FOOBAR...
#if 1 // NOT TESTED SO BROUGHT BACK TO ORIGINAL. FOR LATER I GUESS
	if (z < 0.0F && z <= MINIMUM_ASL_ALTITUDE)						// AGL
		return z - terrain_level;
	else if (z < 0.0F && z - terrain_level < 500.0F)				// Avoid hills
		return -500.0F - terrain_level;
	else															// MSL
		return z;
#else
	// Do we need to test for z being negative, it should anyhow. Also, MINIMUM_ASL_ALTITUDE is POSITIVE!
	// terrain_level IS ALSO POSITIVE!
	if (z < 0.0F && z > -MINIMUM_ASL_ALTITUDE)						// z is AGL if it's 'below' this altitude
		return z - terrain_level;									// This returns the altitude above MSL
	else if (z < 0.0F && z + terrain_level > -500.0F)				// If our -MSL height plus the terrain height is less than -500
		return -500.0F - terrain_level;								// return 500 feet 'above' the terrain
	else															// We ok (not flagged AGL and above high hills), use the MSL height 'as is'
		return z;
#endif
	}

// This will calculate and set this waypoint's speed from it's distance and time values
float SetWPSpeed (WayPoint wp)
	{
	WayPoint	pw;
	float		speed = 0.0F;
	GridIndex	x,y,px,py;
	CampaignTime	time;

	pw = wp->GetPrevWP();
	if (pw)
		{
		wp->GetWPLocation(&x,&y);
		pw->GetWPLocation(&px,&py);
		time = wp->GetWPArrivalTime() - pw->GetWPDepartureTime();
		if (time != 0)
			speed = (Distance(x,y,px,py) * CampaignHours) / time;
		}
	wp->SetWPSpeed(speed);
	return speed;
	}
