#include "Graphics\Include\drawsgmt.h"
#include "Graphics\Include\drawbsp.h"
#include "Graphics\Include\draw2d.h"
#include "stdhdr.h"
#include "otwdrive.h"
#include "initdata.h"
#include "object.h"
#include "simdrive.h"
#include "classtbl.h"
#include "entity.h"
#include "flare.h"

#ifdef USE_SH_POOLS
MEM_POOL	FlareClass::pool;
#endif

FlareClass::FlareClass (VU_BYTE** stream) : BombClass(stream)
{
	bombType = Flare;
}

FlareClass::FlareClass (FILE* filePtr) : BombClass(filePtr)
{
	bombType = Flare;
}

FlareClass::FlareClass (int type) : BombClass(type)
{
	bombType = Flare;
}

int FlareClass::SaveSize()
{
   return BombClass::SaveSize();
}

int FlareClass::Save(VU_BYTE **stream)
{
int saveSize = BombClass::Save (stream);

   return saveSize;
}

int FlareClass::Save(FILE *file)
{
int saveSize = SimWeaponClass::Save (file);

   return saveSize;
}

void FlareClass::InitData (void)
{
}

FlareClass::~FlareClass (void)
{
}

void FlareClass::Init (SimInitDataClass* initData)
{
	BombClass::Init(initData);
}

void FlareClass::Init (void)
{
	BombClass::Init();
}

int FlareClass::Wake (void)
{
int retval = 0;

	if (IsAwake())
		return retval;

	BombClass::Wake();

   OTWDriver.InsertObject(trail);

	if ( parent )
	{
         x = parent->XPos();
         y = parent->YPos();
         z = parent->ZPos();

		ShiAssert( parent->IsSim() );
	}

	return retval;
}

int FlareClass::Sleep (void)
{
   RemoveTrail();
   return BombClass::Sleep();
}

void FlareClass::Start(vector* pos, vector* rate, float cD )
{
	BombClass::Start(pos, rate, cD);
   dragCoeff = 0.4f;
}

void FlareClass::ExtraGraphics(void)
{
Falcon4EntityClassType* classPtr;

	OTWDriver.RemoveObject(drawPointer, TRUE);
	drawPointer = NULL;
	classPtr = &Falcon4ClassTable[displayIndex];
	OTWDriver.CreateVisualObject (this, classPtr->visType[0], OTWDriver.Scale());

	BombClass::ExtraGraphics();
	OTWDriver.RemoveObject(drawPointer, TRUE);
	drawPointer = NULL;
	classPtr = &Falcon4ClassTable[displayIndex];
	OTWDriver.CreateVisualObject (this, classPtr->visType[0], OTWDriver.Scale());

   Tpoint newPoint = { XPos(), YPos(), ZPos() };

   OTWDriver.InsertObject(trailGlow);
   OTWDriver.InsertObject(trailSphere);
   OTWDriver.RemoveObject(drawPointer, TRUE);
   drawPointer = new Drawable2D( DRAW2D_FLARE, 2.0f, &newPoint );
   OTWDriver.InsertObject(drawPointer);
   timeOfDeath = SimLibElapsedTime;
}

int FlareClass::Exec (void)
{
   if (trail)
   {
      UpdateTrail();

      if (SimLibElapsedTime - timeOfDeath > 10 * SEC_TO_MSEC)
         SetDead (TRUE);
   }

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

void FlareClass::DoExplosion(void)
{
	SetFlag( SHOW_EXPLOSION );
	SetDead(TRUE);
}

void FlareClass::InitTrail (void)
{
	Falcon4EntityClassType* classPtr;

	trailGlow = NULL;
	trailSphere = NULL;

	flags |= (NeedTrail | IsFlare);
   displayIndex = GetClassID (DOMAIN_AIR, CLASS_SFX, TYPE_FLARE,
      STYPE_FLARE1, SPTYPE_ANY, VU_ANY, VU_ANY, VU_ANY);

   if (drawPointer)
   {
      OTWDriver.RemoveObject(drawPointer, TRUE);
      drawPointer = NULL;
   }

   classPtr = &Falcon4ClassTable[displayIndex];

   trail = new DrawableTrail( TRAIL_AIM120 );

   if (IsAwake())
   {
      OTWDriver.CreateVisualObject (this, classPtr->visType[0], OTWDriver.Scale());
      displayIndex = -1;
   }

	Tpoint newPoint = {0.0f, 0.0f, 0.0f};
 	trailGlow = new Drawable2D( DRAW2D_MISSILE_GLOW, 6.0f, &newPoint );
 	trailSphere = new Drawable2D( DRAW2D_EXPLCIRC_GLOW, 8.0f, &newPoint );
}

void FlareClass::UpdateTrail (void)
{
Tpoint newPoint;

	newPoint.x = XPos();
	newPoint.y = YPos();
	newPoint.z = ZPos();
	OTWDriver.AddTrailHead (trail, newPoint.x, newPoint.y, newPoint.z);
	OTWDriver.TrimTrail(trail, 25);
	trailGlow->SetPosition( &newPoint );
	trailSphere->SetPosition( &newPoint );
	((Drawable2D *)drawPointer)->SetPosition( &newPoint );

   if (SimLibElapsedTime - timeOfDeath > 10 * SEC_TO_MSEC)
      SetDead (TRUE);

	BombClass::UpdateTrail();
}

void FlareClass::RemoveTrail (void)
{
   if (trail)
   {
      OTWDriver.RemoveObject(trail, TRUE);
	  trail = NULL;
   }

   if (trailGlow)
   {
      OTWDriver.RemoveObject(trailGlow, TRUE);
	  trailGlow = NULL;
   }

   if (trailSphere)
   {
      OTWDriver.RemoveObject(trailSphere, TRUE);
	  trailSphere = NULL;
   }
}

