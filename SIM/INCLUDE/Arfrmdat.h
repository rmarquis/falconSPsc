#ifndef _AIRFRAME_DATA
#define _AIRFRAME_DATA

class RollData;
class AeroData;
class AuxAeroData; // JB 010714
class EngineData;
class SimlibFileClass;

class AeroDataSet
{
   public:
      enum {
         EmptyWeight	= 0,
         Area			= 1,
         InternalFuel	= 2,
         AOAMax			= 3	,
         AOAMin			= 4,
         BetaMax		= 5,
         BetaMin		= 6,
         MaxGs			= 7,
         MaxRoll		= 8,
         MinVcas		= 9,
         MaxVcas		= 10,
         CornerVcas		= 11,
         ThetaMax		= 12,
		 NumGear		= 13,
		 NosGearX		= 14,
		 NosGearY		= 15,
		 NosGearZ		= 16,
		 NosGearRng		= 17,
		 LtGearX		= 18,
		 LtGearY		= 19,
		 LtGearZ		= 20,
		 LtGearRng		= 21,
		 RtGearX		= 22,
		 RtGearY		= 23,
		 RtGearZ		= 24,
		 RtGearRng		= 25,
		 CGLoc			= 26,
		 Length			= 27,
		 Span			= 28,
		 FusRadius		= 29,
		 TailHt			= 30,
         NumInputParams};

      ~AeroDataSet(void);
	   RollData *fcsData;
	   AeroData *aeroData;
		 AuxAeroData *auxaeroData; // JB 010714
	   EngineData *engineData;
      float inputData[NumInputParams];
};

void ReadAllAirframeData (void);
void FreeAllAirframeData (void);
AeroData *AirframeAeroRead(SimlibFileClass* inputFile);
AuxAeroData *AirframeAuxAeroRead(SimlibFileClass* inputFile); // JB 010714
EngineData *AirframeEngineRead(SimlibFileClass* inputFile);
RollData *AirframeFcsRead(SimlibFileClass* inputFile);
void ReadData(float* inputData, SimlibFileClass* inputFile);

extern AeroDataSet *aeroDataset;
#endif

