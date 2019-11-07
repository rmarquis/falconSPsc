#ifndef _SIMMOVER_H
#define _SIMMOVER_H

#include "simbase.h"

class SensorClass;
class SimMoverDefinition;

class SimMoverClass : public SimBaseClass
{
   private:
      int* switchData;
      int* switchChange;

   protected:
      enum DOFTypes {AngleDof, TranslateDof, NoDof};
      int numDofs;
      int numSwitches;
      int numVertices;
      float* DOFData;
      float* VertexData;
      int* DOFType;
      int dataRequested;
      int requestCount;
      SimMoverDefinition* mvrDefinition;
      virtual void InitData(void);
      virtual void Cleanup(void);
      float	vt, kias;
      void MakeComplex(void);
      void MakeSimple();
      void AllocateSwitchAndDof(void);

   public:
      // Sensors
      SensorClass** sensorArray;
      int numSensors;

      // Targeting
      SimObjectType* targetPtr;
      SimObjectType* targetList;

      // Other Data
      unsigned char vehicleInUnit;		// The (vehicleSlot)th vehicle in the unit
      unsigned char pilotSlot;			// The (pilotSlot)th pilot in the unit

      // if we've hit a flat feature (ie for landing)
      // use this pointer to record the fact
      BOOL onFlatFeature;

      virtual float GetP (void);
      virtual float GetQ (void);
      virtual float GetR (void);
      virtual float GetAlpha (void);
      virtual float GetBeta (void);
      virtual float GetNx(void);
      virtual float GetNy(void);
      virtual float GetNz(void);
      virtual float GetGamma(void);
      virtual float GetSigma(void);
	  virtual float	GetMu(void);
      virtual float Vt (void) {return vt;};
      virtual float Kias (void) {return kias;};
      virtual void SetVt (float new_vt);
      virtual void SetKias (float new_kias);

      SimMoverClass(int type);
      SimMoverClass(VU_BYTE** stream);
      SimMoverClass(FILE* filePtr);
      virtual ~SimMoverClass (void);
      virtual void Init (SimInitDataClass* initData);
      virtual int Exec (void);
      virtual void SetLead (int) {};
      virtual void SetDead (int);
      virtual int Sleep (void);
      virtual int Wake (void);
      virtual void MakeLocal (void);
      virtual void MakeRemote (void);

      // this function can be called for entities which aren't necessarily
      // exec'd in a frame (ie ground units), but need to have their
      // gun tracers and (possibly other) weapons serviced
      virtual void WeaponKeepAlive( void ) { return; };

      // virtual function interface
      // serialization functions
      virtual int SaveSize();
      virtual int Save(VU_BYTE **stream);	// returns bytes written
      virtual int Save(FILE *file);		// returns bytes written

      // event handlers
      virtual int Handle(VuFullUpdateEvent *event);
      virtual int Handle(VuPositionUpdateEvent *event);
      virtual int Handle(VuTransferEvent *event);

      // collision with feature
      virtual SimBaseClass *FeatureCollision( float groundZ );
	  virtual int CheckLOS(SimObjectType *obj);
	  virtual int CheckCompositeLOS(SimObjectType *obj);
	  void	UpdateLOS(SimObjectType *obj);

      void SetDOFs (float*);
      void SetSwitches (int*);
      void SetDOF (int dof, float val) { ShiAssert(dof < numDofs); if (dof < numDofs) { DOFData[dof] = val;}};
      void SetDOFInc (int dof, float val) { ShiAssert(dof < numDofs);  if (dof < numDofs) { DOFData[dof] += val;} };
      float GetDOFValue (int dof) { ShiAssert(dof < numDofs); return dof < numDofs ? DOFData[dof] : 0;};
      void SetDOFType (int dof, int type) { ShiAssert(dof < numDofs); DOFType[dof] = type;};
      void SetSwitch (int num, int val) { ShiAssert(num < numSwitches); if (num < numSwitches) { switchData[num] = val; switchChange[num] = TRUE;} };
      int GetSwitch (int num) { return num < numSwitches ? switchData[num] : 0; }
      void AddDataRequest (int flag);
      int DataRequested (void) {return dataRequested;};
      int GetNumSwitches (void ) { return numSwitches;};
      int GetNumDOFs (void ) { return numDofs;};
      void SetDataRequested (int flag) {dataRequested = flag;};
      void SetTarget (SimObjectType *newTarget);
      void ClearTarget (void);
      virtual VU_ERRCODE InsertionCallback(void);
      virtual VU_ERRCODE RemovalCallback(void);
      virtual int IsMover (void) {return TRUE;};
};

#endif
