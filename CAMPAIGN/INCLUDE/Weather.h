// ====================
// Campaign Terrain ADT
// ====================

#ifndef WEATHER_H
#define WEATHER_H

#include "MsgInc\WeatherMsg.h"
#include "CmpGlobl.h"
#include "CampCell.h"
#include "CampTerr.h"
#include "Graphics\Include\WxMap.h"

// ---------------------------------------
// Type and External Function Declarations
// ---------------------------------------

class WeatherClass : public WeatherMap {
	private:
	public:
		float				WindHeading;
		float				WindSpeed;
		float				Temp;									// Temperature
		short				CellSizeGrid;							// Cell size, in grids
		short				WeatherDay;								// Last day we chose weather for
		uchar				TodaysTemp;
		uchar				TodaysWind;
		uchar				TodaysBase;								// Cloud base for today
		uchar				TodaysConLow;							// Contrail level for today
		uchar				TodaysConHigh;							// (thousands of feet)
		CampaignTime		LastCheck;								// Cloud delta time stamp
		CampaignTime		LastChange;								// Wind change time stamp
	public:
		WeatherClass(void);
		~WeatherClass(void);
		void Init(void);
		void UpdateWeather(void);									// Updates weather deltas
		void ProcessWeather(void);									// Shifts weather, generates new clouds
		int CampLoad(char *name, int type);
		int Save(char *name);
		void SendWeather (VuTargetEntity* dest);
		void SendEdge (int side, int seed);							// Send a weather update message for a side
		void ReceiveWeather (FalconWeatherMessage* message);
		int GetCloudCover(GridIndex x, GridIndex y);
		int GetCloudLevel(GridIndex x, GridIndex y);
		void SetCloudCover(GridIndex x, GridIndex y, int cov);
		void SetCloudLevel(GridIndex x, GridIndex y, int lev);
		void ShiftWeather(int heading, int seed);   
		void UpdateWind(CampaignTime delta);
		void SetWind(float heading, float speed);
		void SetTemp(int temp);
		void FormClouds (int x, int y, int cb, int seed, int h);
		float WindSpeedInFeetPerSecond(const Tpoint *pos);
		float WindHeadingAt(const Tpoint *pos);
		virtual float TemperatureAt(const Tpoint *pos);
	};

#endif
