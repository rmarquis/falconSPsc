#ifndef _SIM_WEAPON_H
#define _SIM_WEAPON_H

#include "simmover.h"

class SensorClass;
class SimWeaponClass;

class SimWeaponClass : public SimMoverClass
{
	public:
      SimWeaponClass* nextOnRail;
      virtual int Sleep (void);
      virtual int Wake (void);

      virtual ~SimWeaponClass (void);
	
	public:
      char parentReferenced;
	  int rackSlot;
	  uchar shooterPilotSlot;				// The pilotSlot of the pilot who shot this weapon
      Float32 lethalRadiusSqrd;
      FalconEntity* parent;

      SimWeaponClass(int type);
      SimWeaponClass(VU_BYTE** stream);
      SimWeaponClass(FILE* filePtr);
      void InitData (void);
      virtual void Init (void);
      virtual int Exec (void);
      virtual void SetDead (int);

	  SimWeaponClass* GetNextOnRail(void) { return nextOnRail; };
      void SetRackSlot (int slot) { rackSlot = slot; };
      int GetRackSlot (void) { return rackSlot; };
      void SetParent(FalconEntity* newParent) {parent = newParent;};
      FalconEntity* Parent(void) {return parent;};

      // virtual function interface
      // serialization functions
      virtual int SaveSize();
      virtual int Save(VU_BYTE **stream);	// returns bytes written
      virtual int Save(FILE *file);		// returns bytes written

      // event handlers
      virtual int Handle(VuFullUpdateEvent *event);
      virtual int Handle(VuPositionUpdateEvent *event);
      virtual int Handle(VuTransferEvent *event);

	  // other stuff
	  void SendDamageMessage(FalconEntity *testObject, float rangeSquare, int damageType);

     virtual int IsWeapon (void) {return TRUE;};
	 virtual int GetRadarType (void);
};

#endif
