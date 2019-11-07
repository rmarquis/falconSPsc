#ifndef _RAIL_INFO_H_
#define _RAIL_INFO_H_

struct RailInfo
{
	short rackID;
	short weaponID;
	short startBits;
	short currentBits;
};

struct RailList
{
	RailInfo rail[HARDPOINT_MAX];
};

#endif