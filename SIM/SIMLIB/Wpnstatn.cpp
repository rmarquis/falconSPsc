#include "stdhdr.h"
#include "hardpnt.h"
#include "otwdrive.h"
#include "SimWeapn.h"

// ========================================
// Basic weapon station data
// ========================================

#ifdef USE_SH_POOLS
MEM_POOL	BasicWeaponStation::pool;
MEM_POOL	AdvancedWeaponStation::pool;
#endif

BasicWeaponStation::BasicWeaponStation(void)
{
	weaponId = 0;
	weaponCount = 0;
	weaponPointer = NULL;
}

BasicWeaponStation::~BasicWeaponStation()
{
}

GunClass* BasicWeaponStation::GetGun (void)
{ 
	if (weaponPointer && weaponPointer->IsGun())
		return (GunClass*) weaponPointer;
	else 
		return NULL; 
}

// ========================================
// Advanced weapon station data
// ========================================

AdvancedWeaponStation::AdvancedWeaponStation(void)
{
   xPos = 0.0F;
   yPos = 0.0F;
   zPos = 0.0F;
   az   = 0.0F;
   el   = 0.0F;
   xSub  = NULL;
   ySub  = NULL;
   zSub  = NULL;
   azSub = NULL;
   elSub = NULL;
   aGun = NULL;
   theRack = NULL;
   numPoints = 0;
   memset(&weaponData, 0, sizeof weaponData); // JPO initialise
   weaponData.weaponClass = wcNoWpn;
   weaponData.domain = wdNoDomain;
   weaponType = wtNone;
}

AdvancedWeaponStation::~AdvancedWeaponStation(void)
{
	Cleanup();
}

void AdvancedWeaponStation::Cleanup (void)
{
// 2002-03-26 MN CTD fix, only delete when we used "new" below
   if (xSub != &xPos && numPoints != 1)
   {
	  if ( xSub )
      	delete [] xSub;
	  if ( ySub )
      	delete [] ySub;
	  if ( zSub )
      	delete [] zSub;
	  if ( azSub )
      	delete [] azSub;
	  if ( elSub )
      	delete [] elSub;
   }
}

void AdvancedWeaponStation::SetupPoints (int num)
{
   numPoints = num;

   if (numPoints == 1)
   {
      xSub  = &xPos;
      ySub  = &yPos;
      zSub  = &zPos;
      azSub = &az;
      elSub = &el;
   }
   else
   {
      xSub  = new float[numPoints];
      ySub  = new float[numPoints];
      zSub  = new float[numPoints];
      azSub = new float[numPoints];
      elSub = new float[numPoints];
   }
}
