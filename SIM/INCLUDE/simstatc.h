#ifndef _SIMSTATIC_H
#define _SIMSTATIC_H

#include "simbase.h"

class SimStaticClass : public SimBaseClass
{
   protected:
      virtual void InitData(void);

   public:
      SimStaticClass(int type);
      SimStaticClass(VU_BYTE** stream);
      SimStaticClass(FILE* filePtr);
      virtual void Init (SimInitDataClass* initData);
      virtual ~SimStaticClass(void);

      // virtual function interface
      // serialization functions
      virtual int SaveSize();
      virtual int Save(VU_BYTE **stream);	// returns bytes written
      virtual int Save(FILE *file);		// returns bytes written

      // event handlers
      virtual int Handle(VuFullUpdateEvent *event);
      virtual int Handle(VuPositionUpdateEvent *event);
      virtual int Handle(VuTransferEvent *event);
      virtual int IsStatic (void) {return TRUE;};
};

#endif
