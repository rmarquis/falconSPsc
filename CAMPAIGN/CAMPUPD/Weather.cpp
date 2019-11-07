// ====================
// Campaign Terrain ADT
// ====================

#include "CmpGlobl.h"
#include "CampCell.h"
#include "CampTerr.h"
#include "Weather.h"
#include "math.h"
#include "F4Find.h"
#include "Entity.h"
#include "Campaign.h"
#include "Falcmesg.h"
#include "AIInput.h"
#include "F4Thread.h"
#include "CmpClass.h"
#include "MsgInc\WeatherMsg.h"
#include "MsgInc\WindChangeMsg.h"
#include "falcsess.h"
#include "F4Comms.h"
#include "otwdrive.h"//me123
#include "PlayerOp.h"

int RandomCloudRange = 200;		// Cloud level range

// OW Weather extensions
extern bool g_bEnableWeatherExtensions;
extern bool g_bEnableWindsAloft;
float windgust = 0.0f;//me123
float WeatherClass::WxAltShift	= -2000.0f;
extern float g_fCloudMinHeight;
extern int weatherPattern;
extern int NumWeatherPatterns;
extern WeatherPatternDataType* weatherPatternData;

// ============================ 
// External Function Prototypes
// ============================

// ===================================
// Local Functions/variables
// ===================================

int SRand (int *seed, int max);

int gMaxWeatherMessageSize = 0;

// ===================================
// Class Functions
// ===================================

WeatherClass::WeatherClass (void) : WeatherMap ()
	{
	WeatherDay = -1;	
	TodaysBase = 0;
	TodaysTemp = 0;
	TodaysWind = 0;
	LastChange = 0;
	TodaysConLow = 30;
	TodaysConHigh = 36;
	if (g_fCloudMinHeight >= 0) 
	    WxAltShift = - g_fCloudMinHeight;
	}

WeatherClass::~WeatherClass (void)
	{
	}

void WeatherClass::Init (void)
	{
	UINT	nw,nh;

	WindHeading = 0.0F;
	WindSpeed = 10.0F;
	WeatherDay = -1;	
	TodaysBase = 0;
	TodaysTemp = rand()%8+16;
	TodaysWind = 0;
	TodaysConLow = 30;
	TodaysConHigh = 36;
	xOffset = 0.0F;
	yOffset = 0.0F;
	rowShiftHistory = 0;
	colShiftHistory = 0;
	CellSizeGrid = (short)(CLOUD_CELL_SIZE);
	Temp = 20.0F;						// Centigrade
	LastCheck = Camp_GetCurrentTime();
	LastChange = 0;
	nw = (Map_Max_X+CellSizeGrid-1)/CellSizeGrid;
	nh = (Map_Max_Y+CellSizeGrid-1)/CellSizeGrid;
	if (nw != w || nh != h || !map)
		Setup();
	else
		{
		// Intialize the map to "clear"
		for (DWORD k = 0; k < w*h; k++)
			{
			map[k].Type = 0;
			map[k].BaseAltitude = 0xFF;
			}
		}

	gMaxWeatherMessageSize = F4VuMaxTCPMessageSize - sizeof (FalconWeatherMessage);
	}

// This is the main weather call- Update weather deltas since last check
void WeatherClass::UpdateWeather (void)
	{
	CampaignTime	time,tdelta;
	float			distance,delta;
	mlTrig			trig;

	// Check time delta
	time = Camp_GetCurrentTime();
	tdelta = time - LastCheck;
	LastCheck = time;
	
	// Accept no delta greater than 2 minutes.. 
	if (tdelta > 2 * CampaignMinutes)
		tdelta = 2 * CampaignMinutes;
	if (tdelta <= 0)
		return;

	// Update cloud deltas and shift clouds, if necessary
	delta = (float)tdelta / (float)CampaignHours;
	distance = (float)(delta*(WindSpeed+40.0F));			// + is to speed clouds along while having reasonable wind speeds
	mlSinCos (&trig, WindHeading);
	yOffset += trig.sin*distance;
	xOffset += trig.cos*distance;

	// The master session has a few extra things to do
	if (TheCampaign.IsMaster())
		{
		// Shift wind, if necessary
		if (time > LastChange + CampaignDay)	// A wind message a minute is not good ? Changed to once a day - RH
			{
			UpdateWind(time-LastChange);
			LastChange = time;
			}
		// Process weather shifts, if any
		ProcessWeather();
		}
	}

// This is the main Campaign weather call- shifts clouds and generates new ones, if necessary
void WeatherClass::ProcessWeather (void)
	{
	int		shifting = 1;
	int		initial_seed = rand();

	while (shifting)
		{
		if (yOffset > CLOUD_CELL_SIZE)
			{
			yOffset -= CLOUD_CELL_SIZE;
			colShiftHistory++;
			if (xOffset > CLOUD_CELL_SIZE)
				{
				xOffset -= CLOUD_CELL_SIZE;
				rowShiftHistory++;
				ShiftWeather(1,initial_seed);
				if (TheCampaign.IsOnline())
					{
					SendEdge(0,initial_seed);
					SendEdge(2,initial_seed);
					}
				}
			else if (xOffset < -CLOUD_CELL_SIZE)
				{
				xOffset += CLOUD_CELL_SIZE;
				rowShiftHistory--;
				ShiftWeather(3,initial_seed);
				if (TheCampaign.IsOnline())
					{
					SendEdge(2,initial_seed);
					SendEdge(4,initial_seed);
					}
				}
			else
				{
				ShiftWeather(2,initial_seed);
				if (TheCampaign.IsOnline())
					SendEdge(2,initial_seed);
				}
			}
		else if (yOffset < -CLOUD_CELL_SIZE)
			{
			yOffset += CLOUD_CELL_SIZE;
			colShiftHistory--;
			if (xOffset > CLOUD_CELL_SIZE)
				{
				xOffset -= CLOUD_CELL_SIZE;
				rowShiftHistory++;
				ShiftWeather(7,initial_seed);
				if (TheCampaign.IsOnline())
					{
					SendEdge(6,initial_seed);
					SendEdge(0,initial_seed);
					}
				}
			else if (xOffset < -CLOUD_CELL_SIZE)
				{
				xOffset += CLOUD_CELL_SIZE;
				rowShiftHistory--;
				ShiftWeather(5,initial_seed);
				if (TheCampaign.IsOnline())
					{
					SendEdge(6,initial_seed);
					SendEdge(4,initial_seed);
					}
				}
			else
				{
				ShiftWeather(6,initial_seed);
				if (TheCampaign.IsOnline())
					SendEdge(6,initial_seed);
				}
			}
		else if (xOffset > CLOUD_CELL_SIZE)
			{
			xOffset -= CLOUD_CELL_SIZE;
			rowShiftHistory++;
			ShiftWeather(0,initial_seed);
			if (TheCampaign.IsOnline())
				SendEdge(0,initial_seed);
			}
		else if (xOffset < -CLOUD_CELL_SIZE)
			{
			xOffset += CLOUD_CELL_SIZE;
			rowShiftHistory--;
			ShiftWeather(4,initial_seed);
			if (TheCampaign.IsOnline())
				SendEdge(4,initial_seed);
			}
		else
			shifting = 0;
		}
	}

void WeatherClass::SendWeather (VuTargetEntity *target)
	{
	FalconWeatherMessage	*message;
	int						size = w*h*sizeof(CellState);
	int						msgCount=0;

	while (size > 0)
		{
		// Send weather data in "gMaxWeatherMessageSize" size messages
		message = new FalconWeatherMessage(vuLocalSessionEntity->Id(),target);
		message->dataBlock.updateSide = 8 + msgCount;				// Not a side, it's the whole deal
		if (size > gMaxWeatherMessageSize)
			message->dataBlock.dataSize = gMaxWeatherMessageSize;
		else
			message->dataBlock.dataSize = size;
		message->dataBlock.cloudData = new uchar[message->dataBlock.dataSize];
		memcpy(message->dataBlock.cloudData,((uchar*)map) + gMaxWeatherMessageSize*msgCount, message->dataBlock.dataSize);
		FalconSendMessage(message,TRUE);
		size -= gMaxWeatherMessageSize;
		msgCount++;
		}
	}

void WeatherClass::SendEdge (int heading, int seed)
	{
	FalconWeatherMessage*	message = new FalconWeatherMessage(vuLocalSessionEntity->Id(),FalconLocalGame);

	message->dataBlock.updateSide = heading;
	message->dataBlock.seed = seed;
	message->dataBlock.dataSize = 0;
	message->dataBlock.cloudData = NULL;
	FalconSendMessage(message,TRUE);
	}

void WeatherClass::ReceiveWeather (FalconWeatherMessage* message)
	{
	int		msgCount=0;

	if (message->dataBlock.updateSide >= 8)
		{
		msgCount = message->dataBlock.updateSide - 8;
		message->dataBlock.updateSide = 8;
		}
	LastCheck = Camp_GetCurrentTime();
	switch (message->dataBlock.updateSide)
		{
		case 0:
			xOffset -= CLOUD_CELL_SIZE;
			ShiftWeather(message->dataBlock.updateSide, message->dataBlock.seed);
			break;
		case 2:
			yOffset -= CLOUD_CELL_SIZE;
			ShiftWeather(message->dataBlock.updateSide, message->dataBlock.seed);
			break;
		case 4:
			xOffset += CLOUD_CELL_SIZE;
			ShiftWeather(message->dataBlock.updateSide, message->dataBlock.seed);
			break;
		case 6:
			yOffset += CLOUD_CELL_SIZE;
			ShiftWeather(message->dataBlock.updateSide, message->dataBlock.seed);
			break;
		case 8:
			memcpy(((uchar*)map) + gMaxWeatherMessageSize*msgCount, message->dataBlock.cloudData, message->dataBlock.dataSize);
			// KCK: Actually, this is only one bit of the weather.. but probably
			// good enough for our purposes.
			TheCampaign.Flags &= ~CAMP_NEED_WEATHER;
			TheCampaign.GotJoinData();
			break;
		}
	}

int WeatherClass::CampLoad (char* name, int type)
	{
	char	*data, *data_ptr, rawfile[_MAX_PATH];
	UINT	nw,nh;

	Init();
	mapType = type;

	if (weatherPattern == 0)		// load default pattern from camp file
	{
		if ((data = ReadCampFile (name, "wth")) == NULL)
			return 0;
	}
	else if (NumWeatherPatterns)
	{
		if (weatherPattern == -1) 	// load random pattern from a raw weather file
		{
			weatherPattern = rand() % NumWeatherPatterns;
		}

		FILE *fp;
		sprintf( rawfile, "%s\\weather\\raw\\%s", FalconTerrainDataDir, weatherPatternData[weatherPattern-1].filename);
		if (!(fp = fopen (rawfile, "rb")))
		{
			sprintf(rawfile,"Raw weather file %s missing !!",weatherPatternData[weatherPattern-1].filename);
			ShiWarning (rawfile);
			return 0;
		}


		if (fp)
		{
			fseek (fp, 0, 2);
			int size = ftell (fp);
			fseek (fp, 0, 0);
			data = new char [size + 1];
			fread (data, size, 1, fp);
			data[size] = 0;
			fclose (fp);
		}
	}

	data_ptr = data;
	WindHeading = *((float *) data_ptr); data_ptr += sizeof (float);
	WindSpeed = *((float *) data_ptr); data_ptr += sizeof (float);
	LastCheck = *((CampaignTime *) data_ptr); data_ptr += sizeof (CampaignTime);

	Temp = *((float *) data_ptr); data_ptr += sizeof (float);
	TodaysTemp = *((uchar *) data_ptr); data_ptr += sizeof (uchar);
	TodaysWind = *((uchar *) data_ptr); data_ptr += sizeof (uchar);
	TodaysBase = *((uchar *) data_ptr); data_ptr += sizeof (uchar);

	if (gCampDataVersion >= 27)
		{
		TodaysConLow = *((uchar *) data_ptr); data_ptr += sizeof (uchar);
		TodaysConHigh = *((uchar *) data_ptr); data_ptr += sizeof (uchar);
		}

	xOffset = *((float *) data_ptr); data_ptr += sizeof (float);
	yOffset = *((float *) data_ptr); data_ptr += sizeof (float);

	nw = *((UINT *) data_ptr); data_ptr += sizeof (UINT);
	nh = *((UINT *) data_ptr); data_ptr += sizeof (UINT);

	if (nw == w && nh == h)
	{
		memcpy (map, data_ptr, sizeof (CellState) * w * h);
		data_ptr += sizeof (CellState) * w * h;
	}

	delete data;

	rowShiftHistory = 0;
	colShiftHistory = 0;

	return 1;

	
	
	}


int WeatherClass::Save (char* name)
	{
	FILE	*fp;

	if (!map)
		return 0;
	if ((fp = OpenCampFile (name, "wth", "wb")) == NULL)
		return 0;
	fwrite(&WindHeading,sizeof(float),1,fp);
	fwrite(&WindSpeed,sizeof(float),1,fp);
	fwrite(&LastCheck,sizeof(CampaignTime),1,fp);
	fwrite(&Temp,sizeof(float),1,fp);
	fwrite(&TodaysTemp,sizeof(uchar),1,fp);
	fwrite(&TodaysWind,sizeof(uchar),1,fp);
	fwrite(&TodaysBase,sizeof(uchar),1,fp);
	fwrite(&TodaysConLow,sizeof(uchar),1,fp);
	fwrite(&TodaysConHigh,sizeof(uchar),1,fp);
	fwrite(&xOffset,sizeof(float),1,fp);
	fwrite(&yOffset,sizeof(float),1,fp);
	fwrite(&w,sizeof(UINT),1,fp);
	fwrite(&h,sizeof(UINT),1,fp);
	fwrite(map,sizeof(CellState),w*h,fp);
	CloseCampFile(fp);
	return 1;
	}

int WeatherClass::GetCloudCover(GridIndex x, GridIndex y)
	{
	UINT		i;

	i = (y/CellSizeGrid)*w + (x/CellSizeGrid);
	if (i >= 0 && i < w*h)
		return map[i].Type;
	return 0;
	}

int WeatherClass::GetCloudLevel(GridIndex x, GridIndex y)
	{
	UINT		i;

	i = (y/CellSizeGrid)*w + (x/CellSizeGrid);
	if (i >= 0 && i < w*h)
		return map[i].BaseAltitude;
	return 0;
	}

void WeatherClass::SetCloudCover(GridIndex x, GridIndex y, int cov)
	{
	UINT		i;

	i = (y/CellSizeGrid)*w + (x/CellSizeGrid);
	if (i >= 0 && i < w*h)
		map[i].Type = cov;
	}

void WeatherClass::SetCloudLevel(GridIndex x, GridIndex y, int lev)
	{
	UINT		i;

	i = (y/CellSizeGrid)*w + (x/CellSizeGrid);
	if (i >= 0 && i < w*h)
		map[i].BaseAltitude = lev;
	}

void WeatherClass::UpdateWind(CampaignTime tdelta)
	{
	FalconWindChangeMessage*	message = new FalconWindChangeMessage(vuLocalSessionEntity->Id(),FalconLocalGame);
	float						seed,delta;
	int							h,tempbonus;
	static						lasttod=TOD_NIGHT;

	// Determine today's weather, if it's a new day
	if (WeatherDay != TheCampaign.GetCurrentDay())
		{
		WeatherDay = TheCampaign.GetCurrentDay();
		tempbonus = 21 - TodaysTemp;
		TodaysTemp = rand()%10 + 16 + tempbonus;						// Temp we'll shoot for at Midday
		if (TodaysTemp < 16)
			TodaysTemp = 16;
		if (TodaysTemp > 25)
			TodaysTemp = 25;

		// OW Weather extensions
		if(g_bEnableWeatherExtensions)
			TodaysBase = rand()%RandomCloudRange + 1;	// Target cloud base for today
		else
			TodaysBase = rand()%RandomCloudRange + 32;	// Target cloud base for today

		TodaysWind = rand()%20 + 5;					// Target wind speed
		TodaysConLow = 30 + rand()%10;
		TodaysConHigh = TodaysConLow + 2 + rand()%8;
		}

	// Recalculate Temperature, and slow-speed wind
	delta = (float)tdelta / (float)CampaignHours;
	if (delta > 0.5F)
		delta = 0.5F;
	seed = (float)(rand()%100)/100.0F;
// 2001-04-10 MODIFIED BY S.G. TimeOfDay RETURNS THE TIME OF THE DAY IN MILISECONDS! THAT'S NOT WHAT WE WANT... TimeOfDayGeneral WILL DO WHAT WE WANT
//	switch (TimeOfDay())			// Time of day modifiers
	switch (TimeOfDayGeneral())		// Time of day modifiers
		{
		case TOD_NIGHT:
			if (Temp > 12.0F)
				Temp -= (float)delta*seed*2.0F;
			if (WindSpeed > 0.0F)
				WindSpeed -= (float)delta*seed*4.0F;
			lasttod = TOD_NIGHT;
			break;
		case TOD_DAWNDUSK:
			if (lasttod == TOD_NIGHT)
				{
				// Good morning, Korea! - heat up/speed up if it's to cold/slow
				if (Temp < TodaysTemp - 2)
					Temp += (float)delta*seed*4.0F;
				if (WindSpeed < TodaysWind - 5)
					WindSpeed += (float)delta*seed;
				}
			else
				{
				// Good evening - cool down and slow wind if it's hot/high
				if (Temp > 16.0F)
					Temp -= (float)delta*seed*4.0F;
				if (WindSpeed > 5.0F)
					WindSpeed -= (float)delta*seed*4.0F;
				}
			break;
		default:
			if (Temp < TodaysTemp)
				Temp += (float)delta*seed*2.0F;
			if (WindSpeed < TodaysWind)
				WindSpeed += (float)delta*seed*4.0F;
			lasttod = TOD_DAY;
			break;
		}
	
	// Shift wind (Lean towards a 90 deg heading)
	h = FloatToInt32((WindHeading - 0.5F*PI)*3.0F);
	if (rand()%8 > 4+h)
		WindHeading = WindHeading + (float)delta/3;
	else
		WindHeading = WindHeading - (float)delta/3;
	if (WindHeading < 0.0F)
		WindHeading += (float)(2.0F*PI);
	if (WindHeading > 2.0F*PI)
		WindHeading -= (float)(2.0F*PI);

	// Send a wind change event
	message->dataBlock.temp = Temp;
	message->dataBlock.windHeading = WindHeading;
	message->dataBlock.windSpeed = WindSpeed;
	message->dataBlock.xOffset = xOffset;
	message->dataBlock.yOffset = yOffset;
  	FalconSendMessage(message,TRUE);
	}

void WeatherClass::ShiftWeather(int heading, int seed)
	{
	int		shift=0,start=0,left=0,right=0,top=0,bottom=0,cb=0;
	UINT	x=0,y=0;

	switch (heading)
		{
		case 0:
			shift = 0;
			start = w;
			bottom = 1;
			break;
		case 1:
			shift = 0;
			start = w + 1;
			left = 1;
			bottom = 1;
			break;
		case 2:
			shift = 0;
			start = 1;
			left = 1;
			break;
		case 3:
			shift = w - 1;
			start = 0;
			top = 1;
			left = 1;
			break;
		case 4:
			shift = w;
			start = 0;
			top = 1;
			break;
		case 5:
			shift = w + 1;
			start = 0;
			top = 1;
			right = 1;
			break;
		case 6:
			shift = 1;
			start = 0;
			right = 1;
			break;
		case 7:
			shift = 0;
			start = w - 1;
			bottom = 1;
			right = 1;
			break;
		default:
			shift = 0;
			break;
		}
	memmove(map+start,map+shift,(w*h-(shift+start))*sizeof(CellState));

	// Generate new clouds along edges (Only get real cloud bonus for west edge)
	cb = FloatToInt32((15.0F-Temp)*5.0F);
	if (top)
		{
		for (x=0; x<w; x++)
			FormClouds(x,h-1,0,SRand(&seed,100),4);
		}
	if (bottom)
		{
		for (x=0; x<w; x++)
			FormClouds(x,0,0,SRand(&seed,100),0);
		}
	if (left)
		{
		for (y=0; y<h; y++)
			FormClouds(0,y,cb,SRand(&seed,100),2);
		}
	if (right)
		{
		for (y=0; y<h; y++)
			FormClouds(w-1,y,0,SRand(&seed,100),6);
		}
	}

void WeatherClass::SetWind (float heading, float speed)
	{
	WindHeading = heading;
	WindSpeed = speed;
	}

void WeatherClass::SetTemp (int temp)
	{
	Temp = (float)temp;
	}

void WeatherClass::FormClouds (int x, int y, int cb, int seed, int h)
	{
	int		val,lev,c,gx,gy,vals,thickup=0,tl;

	for (c=0,val=0,lev=0,vals=0; c<3; c++)
		{
		gx = (x+dx[(h+7+c)%8])*CellSizeGrid;
		gy = (y+dy[(h+7+c)%8])*CellSizeGrid;
		if (gx < 0 || gy < 0 || gx >= Map_Max_X || gy >= Map_Max_Y)
			continue;
		tl = GetCloudLevel(gx,gy);
		if (tl < 255)
			{
			vals++;
			val += GetCloudCover(gx,gy);
			lev += tl;
			}
		}
	if (vals)
		{
		val = FloatToInt32(((float)val/vals)+0.75F);
		lev = FloatToInt32(((float)lev/vals)+0.33F);
		}
	else
		{
		val = 0;
		lev = 255;
		}

	// Grow/Shrink cloud cover
	c = seed + cb;
	if (val > MAX_CLOUD_TYPE)
		val = MAX_CLOUD_TYPE;
	if (c > 100-NEW_CLOUD_CHANCE && val < MAX_CLOUD_TYPE)
		{
		val++;
		thickup = 1;
		}
	if (c < DEL_CLOUD_CHANCE && val > 0)
		{
		val--;
		if (val > FIRST_OVC_TYPE + 3)
			val--;
		thickup = -1;
		}
	
	// Raise/Lower cloud level
	c = seed + (lev - TodaysBase);
	if (lev < 0 || lev == 255 )
		lev = TodaysBase;
	else if (c < 30 && lev < 254 && thickup != 1 && lev < TodaysBase+20)
		lev++;
	else if (c > 70 && lev > 0 && thickup != -1 && lev > TodaysBase-20)
		lev--;
	SetCloudCover(x*CellSizeGrid,y*CellSizeGrid,val);
	SetCloudLevel(x*CellSizeGrid,y*CellSizeGrid,lev);
	}

float WeatherClass::WindSpeedInFeetPerSecond(const Tpoint *pos)
{
	// OW Weather extensions
//#if 0
	if(g_bEnableWindsAloft)
	{
		 float alt = -pos->z/1000;
		 float TempWindSpeed;

		// WindSpeed is in km/hr
		// 1 km/hr == 0.9113 ft/s
		windgust = windgust + ((rand()*4/(float)RAND_MAX)-2.0f);
		windgust = max (-20.0f, min (20.0f,windgust));

	 TempWindSpeed = WindSpeed ;		 
	 //first layer increase from 8 to 13 k
	 if (alt > 1) TempWindSpeed += (alt-1.0f) * (2.0f+WindSpeed/2*FTPSEC_TO_KNOTS)*KNOTS_TO_FTPSEC;	
	 if (alt > 4) TempWindSpeed -= (alt-4.0f) * (2.0f+WindSpeed/2* FTPSEC_TO_KNOTS)*KNOTS_TO_FTPSEC;
	 //first layer increase from 8 to 13 k
	 if (alt > 8) TempWindSpeed += (alt-8.0f) * 5.0f*KNOTS_TO_FTPSEC;	
	 if (alt > 12) TempWindSpeed -= (alt-12.0f) * 5.0f*KNOTS_TO_FTPSEC;
	 //second layer increate from 22 to 28 k
	 if (alt > 18) TempWindSpeed -= (alt-18.0f) * (1.0f+WindSpeed/10* FTPSEC_TO_KNOTS)*KNOTS_TO_FTPSEC;	
	 if (alt > 22) TempWindSpeed += (alt-22.0f) * (1.0f+WindSpeed/10* FTPSEC_TO_KNOTS)*KNOTS_TO_FTPSEC;
	 //jet stream layer increate from 35 to 40 k
	 if (alt > 32) TempWindSpeed += (alt-32.0f) * 15.0f*KNOTS_TO_FTPSEC;	
	 if (alt > 36) TempWindSpeed -= (alt-36.0f) * 15.0f*KNOTS_TO_FTPSEC;
	return TempWindSpeed * 0.9113F ;
	}

	else return WindSpeed * 0.9113F;
//#endif
/*	{
		// WindSpeed is in km/hr
		// 1 km/hr == 0.9113 ft/s

		return WindSpeed * 0.9113F;
	}*/
}

float WeatherClass::WindHeadingAt(const Tpoint *pos)
{
	if(g_bEnableWindsAloft)
	{
		 float alt = -pos->z/1000;
		 float TempWindheading;

	 float gndlevel = -OTWDriver.GetGroundLevel(pos->x, pos->y)/1000.0f;

	 // ground effect
	 TempWindheading = WindHeading + max(0.0f,min((alt-gndlevel) * 10.0f* DTR ,30.0f*DTR));
	  if (TempWindheading > PI)  TempWindheading -= 2*PI;

	 //first wind layer
	  if(alt > 4)
	  {
	 TempWindheading -=  max(0.0f,min((alt-4.0f) * 4.0f* DTR ,20.0f*DTR));
	  if (TempWindheading < -PI)  TempWindheading += 2*PI;
	  }
	 //second wind layer
	  if(alt > 8)
	  {
	 TempWindheading +=  max(0.0f,min((alt-8.0f ) * 20.0f* DTR ,WindSpeed*3* FTPSEC_TO_KNOTS *DTR));
	  if (TempWindheading > PI)  TempWindheading -= 2*PI;
	  }
	  	 //third wind layer
	  if(alt > 16)
	  {
	 TempWindheading +=  max(0.0f,min((alt-16.0f ) * 10.0f* DTR ,30.0f*DTR));
	  if (TempWindheading > PI)  TempWindheading -= 2*PI;
	  }
	  //fourth wind layer
	  if(alt > 29)
	  {
	 TempWindheading +=  max(0.0f,min((alt-29.0f ) * 10.0f* DTR ,40.0f*DTR));
	  if (TempWindheading > PI)  TempWindheading -= 2*PI;
	  }
	return TempWindheading ;
	}

	else return WindHeading;
}

float WeatherClass::TemperatureAt(const Tpoint *pos)
{
    float alt = -pos->z;
    alt /= 1000;
    // assuming 3 deg drop per 1000ft 
    // adiabatic lapse rate is actually 1degC per 100m which is ~330ft
    return Temp - 3 * alt; 
}



// =====================================
// Deterministic random function
// =====================================

#define RAND_MAGIC_NUMBER 57

// Returns a deterministic random number between 0 and max
// based off a seed value. Series are generally created
// by passing the return value back as a seed.
int SRand (int *seed, int max)
	{
	long	x;
	int		y;

	x = ((RAND_MAGIC_NUMBER * *seed) + 1) % 256;
	*seed = ((RAND_MAGIC_NUMBER * x) + 1) % 256;
	y = *seed % max;
	return y;
	}
	