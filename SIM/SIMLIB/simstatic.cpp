#include "stdhdr.h"
#include "simstatc.h"
#include "initdata.h"

SimStaticClass::SimStaticClass(FILE* filePtr) : SimBaseClass (filePtr)
{
	// OW: redundant since base class constructor calls this and SimStaticClass::InitData does nothing 
	// InitData();
}

SimStaticClass::SimStaticClass(VU_BYTE** stream) : SimBaseClass (stream)
{
	// OW: redundant since base class constructor calls this and SimStaticClass::InitData does nothing 
	// InitData();
}

SimStaticClass::SimStaticClass(int type) : SimBaseClass (type)
{
	// OW: redundant since base class constructor calls this and SimStaticClass::InitData does nothing 
	// InitData();
}

void SimStaticClass::InitData (void)
{
   SimBaseClass::InitData();
}

void SimStaticClass::Init (SimInitDataClass* initData)
{
   SetTypeFlag(FalconSimObjective);

   if (initData)
   {
      SetCampaignObject (initData->campObj);
   }
   else
   {
      SetCampaignObject (NULL);
   }

   SimBaseClass::Init (initData);
}

SimStaticClass::~SimStaticClass(void)
{
}
 
// function interface
// serialization functions
int SimStaticClass::SaveSize()
{
   return SimBaseClass::SaveSize();
}

int SimStaticClass::Save(VU_BYTE **stream)
{
   return (SimBaseClass::Save (stream));
}

int SimStaticClass::Save(FILE *file)
{
   return (SimBaseClass::Save (file));
}

int SimStaticClass::Handle(VuFullUpdateEvent *event)
{
   return (SimBaseClass::Handle(event));
}

int SimStaticClass::Handle(VuPositionUpdateEvent *event)
{
   return (SimBaseClass::Handle(event));
}

int SimStaticClass::Handle(VuTransferEvent *event)
{
   return (SimBaseClass::Handle(event));
}
