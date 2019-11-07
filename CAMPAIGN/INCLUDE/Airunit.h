// 
// This includes the Flight and Squadron classes. See Package.cpp for Package class
//

#ifndef AIRUNIT_H
#define AIRUNIT_H

#include "unit.h"
#include "AIInput.h"

// =========================
// Types and Defines
// =========================

#define RESERVE_MINUTES				15					// How much extra fuel to load. Setable?

class AircraftClass;

// =========================
// Air unit Class 
// =========================

class AirUnitClass : public UnitClass
	{
	public:
	public:
		// constructors and serial functions
		AirUnitClass(int type);
		AirUnitClass(VU_BYTE **stream);
		virtual ~AirUnitClass();
		virtual int SaveSize (void);
		virtual int Save (VU_BYTE **stream);

		// event Handlers
		virtual VU_ERRCODE Handle(VuFullUpdateEvent *event);

		// Required pure virtuals handled by AirUnitClass
		virtual MoveType GetMovementType (void);
		virtual int GetUnitSpeed (void);
		virtual CampaignTime UpdateTime (void)					{ return AIR_UPDATE_CHECK_INTERVAL*CampaignSeconds; }
      	virtual float Vt (void)									{ return GetUnitSpeed() * KPH_TO_FPS; }
      	virtual float Kias (void)								{ return Vt() * FTPSEC_TO_KNOTS; }

		// core functions
		virtual int IsHelicopter (void);
		virtual int OnGround (void);
	};

#include "Flight.h"
#include "Squadron.h"
#include "Package.h"


// =========================================
// Air Unit functions
// =========================================

int GetUnitScore (Unit u, MoveType mt);

#endif
