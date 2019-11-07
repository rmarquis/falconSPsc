#include "Graphics\Include\drawbsp.h"
#include "stdhdr.h"
#include "otwdrive.h"
#include "initdata.h"
#include "object.h"
#include "simdrive.h"
#include "classtbl.h"
#include "entity.h"
#include "chaff.h"

#ifdef USE_SH_POOLS
MEM_POOL	ChaffClass::pool;
#endif

ChaffClass::ChaffClass (VU_BYTE** stream) : BombClass(stream)
{
	bombType = Chaff;
}

ChaffClass::ChaffClass (FILE* filePtr) : BombClass(filePtr)
{
	bombType = Chaff;
}

ChaffClass::ChaffClass (int type) : BombClass(type)
{
	bombType = Chaff;
}

int ChaffClass::SaveSize()
{
   return BombClass::SaveSize();
}

int ChaffClass::Save(VU_BYTE **stream)
{
int saveSize = BombClass::Save (stream);

   return saveSize;
}

int ChaffClass::Save(FILE *file)
{
int saveSize = SimWeaponClass::Save (file);

   return saveSize;
}

void ChaffClass::InitData (void)
{
}

ChaffClass::~ChaffClass (void)
{
}

void ChaffClass::Init (SimInitDataClass* initData)
{
	BombClass::Init(initData);
}

void ChaffClass::Init (void)
{
	BombClass::Init();
}

int ChaffClass::Wake (void)
{
int retval = 0;

	if (IsAwake())
		return retval;

	BombClass::Wake();

	if ( parent )
	{
         x = parent->XPos();
         y = parent->YPos();
         z = parent->ZPos();

		ShiAssert( parent->IsSim() );
	}

	return retval;
}

int ChaffClass::Sleep (void)
{
   return SimWeaponClass::Sleep();
}

void ChaffClass::Start(vector* pos, vector* rate, float cD )
{
	BombClass::Start(pos, rate, cD);

   dragCoeff = 1.0f;
}

void ChaffClass::ExtraGraphics(void)
{
Falcon4EntityClassType* classPtr;

	OTWDriver.RemoveObject(drawPointer, TRUE);
	drawPointer = NULL;
	classPtr = &Falcon4ClassTable[displayIndex];
	OTWDriver.CreateVisualObject (this, classPtr->visType[0], OTWDriver.Scale());

	BombClass::ExtraGraphics();
   timeOfDeath = SimLibElapsedTime;
}

int ChaffClass::Exec (void)
{
	BombClass::Exec();

   if(IsExploding())
   {
      SetFlag( SHOW_EXPLOSION );
      SetDead(TRUE);
      return TRUE;
   }

   SetDelta (XDelta() * 0.99F, YDelta() * 0.99F, ZDelta() * 0.99F);
   return TRUE;
}

void ChaffClass::DoExplosion(void)
{
	SetFlag( SHOW_EXPLOSION );
	SetDead(TRUE);
}


void ChaffClass::SpecialGraphics(void)
{
// OW
#if 1
	// OW: drawPointer will be zero if we are sleeping
	if(IsAwake())
	{
		int mask, frame;

		frame = (SimLibElapsedTime - timeOfDeath) / 250;

		if (frame > 15)
		{
			frame = 15;
		}

		mask = 1 << frame;

		((DrawableBSP*)drawPointer)->SetSwitchMask (0, mask);
	}

#else
	int mask, frame;

	frame = (SimLibElapsedTime - timeOfDeath) / 250;

	if (frame > 15)
	{
		frame = 15;
	}

	mask = 1 << frame;

	((DrawableBSP*)drawPointer)->SetSwitchMask (0, mask);
#endif
}

void ChaffClass::InitTrail (void)
{
Falcon4EntityClassType* classPtr;

	flags |= IsChaff;
   displayIndex = GetClassID (DOMAIN_AIR, CLASS_SFX, TYPE_CHAFF,
         STYPE_CHAFF, SPTYPE_CHAFF1, VU_ANY, VU_ANY, VU_ANY);

   if (drawPointer)
   {
      OTWDriver.RemoveObject(drawPointer, TRUE);
      drawPointer = NULL;
   }

   classPtr = &Falcon4ClassTable[displayIndex];

   if (IsAwake())
   {
      OTWDriver.CreateVisualObject (this, classPtr->visType[0], OTWDriver.Scale());
      displayIndex = -1;
   }
}

void ChaffClass::UpdateTrail (void)
{
	BombClass::UpdateTrail();

   if (SimLibElapsedTime - timeOfDeath > 10 * SEC_TO_MSEC)
      SetDead (TRUE);
}

void ChaffClass::RemoveTrail (void)
{
	BombClass::RemoveTrail();
}
