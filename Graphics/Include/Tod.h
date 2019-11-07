#ifndef EJFILE_TIME_OF_DAY_H
#define EJFILE_TIME_OF_DAY_H

#include "grtypes.h"
#include "star.h"

#define	GL_TIME_OF_DAY_USE_SUN			0x001
#define	GL_TIME_OF_DAY_USE_MOON			0x002
#define	GL_TIME_OF_DAY_USE_STAR			0x004

#define	GL_TIME_OF_DAY_HAS_SUNBITMAP	0x100
#define	GL_TIME_OF_DAY_HAS_MOONBITMAP	0x200

#define	GL_STAR_NEW_COLOR				0x2
#define	GL_STAR_END_OF_LIST				0x4

#define NVG_SKY_LEVEL					0.390625f	// 100 256ths
#define NVG_LIGHT_LEVEL					0.703125f	// 180 256ths

#define	MOON_PHASE_SIZE					128			// max is 128
#define	NEW_MOON_PHASE					(MOON_PHASE_SIZE/2)

//#define	USE_TRANSPARENT_MOON

typedef struct TimeOfDayStruct {
	DWORD	Time;
	Tcolor	SkyColor;
	Tcolor	HazeSkyColor;
	Tcolor	GroundColor;
	Tcolor	HazeGroundColor;
	Tcolor	TextureLighting;
	float	Ambient, Diffuse, StarIntensity;
	int		Flag;
	float	SunPitch, MoonPitch;
	// JPO additions
	Tcolor	RainColor;
	Tcolor	SnowColor;
	Tcolor	LightningColor;
	Tcolor VisColor;
	float	MinVis;
} TimeOfDayStruct;


//--------------------------------------------------------------

class CTimeOfDay {
//--------------------------------------------------------------
public:
	CTimeOfDay( void )	{ TimeOfDay = NULL; };
	~CTimeOfDay()		{ if (IsReady())	Cleanup (); };

	void	Setup( char *dataPath );
	void	Cleanup( void );

	BOOL	IsReady( void )		{ return (TimeOfDay != NULL); };

	void	SetNVGmode( BOOL state );
	BOOL	GetNVGmode( void )				{ return NVGmode; };

	void	GetLightDirection( Tpoint *LightDirection );
	void	CalculateSunMoonPos( Tpoint *pos, BOOL ismoon=FALSE );
	void	CalculateSunGroundPos( Tpoint *pos );
	void	SetSunGlareAngle( int angle );	
	float	GetSunGlare( int yaw, int pitch );
	int		GetStarData( Tpoint **vtx, Tcolor *color );
	float	GetStarIntensity( void )	{ return StarIntensity; };
	void	CreateMoonPhaseMask (unsigned char *image, int phase);

public:
	
	void CalculateMoonPhase ();
	void RotateMoonMask (int angle);
	void SetCurrentSkyColor(Tcolor *rgb) { CurrentSkyColor = *rgb; };
	void GetCurrentSkyColor(Tcolor *rgb) { *rgb = CurrentSkyColor; };
	void GetSkyColor(Tcolor *rgb) { *rgb = SkyColor; };
	void GetHazeSkyColor(Tcolor *rgb) { *rgb = HazeSkyColor; };
	void GetGroundColor(Tcolor *rgb) { *rgb = GroundColor; };
	void GetHazeGroundColor(Tcolor *rgb) { *rgb = HazeGroundColor; };
	void GetTextureLightingColor(Tcolor *rgb) { *rgb = TextureLighting; };
	void GetHazeSunriseColor(Tcolor *rgb) { *rgb = HazeSunriseColor; };
	void GetHazeSunsetColor(Tcolor *rgb) { *rgb = HazeSunsetColor; };
	void GetHazeSunHorizonColor(Tcolor *rgb) {
		if (ISunPitch > 4096) *rgb = HazeSunsetColor; 
		else *rgb = HazeSunriseColor; 
	};
	float	GetAmbientValue() { return Ambient; };
	float	GetDiffuseValue() { return Diffuse; };
	float	GetLightLevel() { return Ambient + Diffuse; };
	float	GetMinVisibility() { return MinVis; };
	void GetVisColor (Tcolor *rgb) { *rgb = VisColor; };
	int ThereIsASun() { return (ISunPitch > 0); };
	int ThereIsAMoon() { return (IMoonPitch > 0); };
	int GetSunPitch() { 
		int pitch = ISunPitch;
		if (pitch > 4096) pitch = 8192 - pitch;
		return pitch; 
	};
	int GetSunYaw() { 
		int yaw = ISunYaw;
		if (ISunPitch > 4096) yaw += 8192;
		return yaw & 0x3fff;
	};
	int GetMoonPitch() { 
		int pitch = IMoonPitch;
		if (pitch > 4096) pitch = 8192 - pitch;
		return pitch; 
	};
	int GetMoonYaw() { 
		int yaw = IMoonYaw;
		if (IMoonPitch > 4096) yaw += 8192;
		return yaw & 0x3fff;
	};
	StarData *GetStarData () { return TheStarData; };
	float	CalculateMoonBlend (float glare);
	int	CalculateMoonPercent(void);
	void CreateMoonPhase (unsigned char *src, unsigned char *dest);

	DWORD GetRainColor() { return RainColor; };
	DWORD GetSnowColor() { return SnowColor; };
	Tcolor GetLightningColor() { return LightningColor; };
	int GetTotalTimeOfDay() { return TotalTimeOfDay; };
	int GetTimeOfDay(int i) { return TimeOfDay[i].Time; };
	float GetGroundColoring(int c) { return (TimeOfDay[c].GroundColor.r + TimeOfDay[c].GroundColor.g + TimeOfDay[c].GroundColor.b); };

protected:
	static unsigned char	MoonPhaseMask[8*64];
	static unsigned char	CurrentMoonPhaseMask[8*64];
	Tpoint			SunCoord, MoonCoord;
	int				MoonPhase;
	unsigned		lastMoonTime;
	StarData		*TheStarData;
	TimeOfDayStruct *TimeOfDay;
	int				TotalTimeOfDay;
	Tcolor			CurrentSkyColor;
	Tcolor			SkyColor, HazeSkyColor;
	Tcolor			GroundColor, HazeGroundColor;
	Tcolor			HazeSunriseColor, HazeSunsetColor;
	Tcolor			TextureLighting;
	DWORD			RainColor, SnowColor;
	Tcolor			LightningColor;
	Tcolor			VisColor;
	float			MinVis;
	float			Ambient, Diffuse;
	int				Flag;
	int				ISunPitch, IMoonPitch;
	int				ISunYaw, IMoonYaw;
	int				ISunTilt, IMoonTilt;
	float			SunGlareCosine, SunGlareFactor;
	float			StarIntensity;
	BOOL			NVGmode;

protected:
	static void TimeUpdateCallback( void *self );
	void	UpdateSkyProperties();
	int		ReadTODFile (FILE *in, TimeOfDayStruct *tod, int countflag=0);
	void	SetDefaultColor(Tcolor *col, Tcolor *defcol);
	void	SetVar (TimeOfDayStruct *tod);
	static DWORD	MakeColor(Tcolor *col);
};

extern class CTimeOfDay TheTimeOfDay;

#endif