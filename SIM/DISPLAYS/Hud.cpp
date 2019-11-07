#include "stdhdr.h"
#include <float.h>
#include "hud.h"
#include "guns.h"
#include "aircrft.h"
#include "fcc.h"
#include "object.h"
#include "airframe.h"
#include "otwdrive.h"
#include "playerop.h"
#include "Graphics\Include\RenderOW.h"
#include "Graphics\Include\Mono2d.h"
#include "simdrive.h"
#include "atcbrain.h"
#include "campbase.h"
#include "ptdata.h"
#include "simfeat.h"
#include "mfd.h"
#include "camp2sim.h"
#include "fack.h"
#include "cpmanager.h"
#include "objectiv.h"
#include "fsound.h"
#include "soundfx.h"
#include "smsdraw.h"
#include "flightData.h"
#include "fack.h"
#include "lantirn.h"
#include "missile.h"
#include "radardoppler.h"

//MI for ICP stuff
#include "icp.h"
extern bool g_bRealisticAvionics;
extern bool g_bNoRPMOnHud;
extern bool g_bINS;
extern bool g_bNewPitchLadder;
extern float g_fReconCameraHalfFOV;
extern float g_fReconCameraOffset;

// For HUD coloring
static DWORD HUDcolor[] = {
	0xff00ff00,
	0xff0000ff,
	0xffff0000,
	0xffffff00,
	0xffff00ff,
	0xff00ffff,
	0xff7f0000,
	0xff007f00,
	0xff00007f,
	0xff7f7f00,
	0xff007f7f,
	0xff7f007f,
	0xff7f7f7f,
	0xfffffffe,
	0xff000000,
	0xff00bf00,
};

const DWORD*	HudClass::hudColor = HUDcolor;
const int		HudClass::NumHudColors = sizeof(HUDcolor)/sizeof(HUDcolor[0]);
int				HudClass::flash = FALSE;
//MI
int				HudClass::Warnflash = FALSE;

float hudWinX[NUM_WIN] = {
 -0.15F,  -0.70F, -0.7F,  -0.7F,   -0.7F,   // 0..4
  0.55F,  -0.8F,  -0.8F,   0.0F,    0.55F,  // 5..9
 -0.0F,   -0.0F,   0.55F,  0.55F,  -0.9F,   //10..14
  0.0F,    0.0F,   0.0F,   0.65F,  -0.5F,   //15..19
 -0.5F,   -0.5F,  -0.5F,  -0.5F,    0.55F,  //20..24	//MI tweaked
  0.55F,  -0.3F,  -0.20F, -0.175F, -0.65F,  //25..29
  0.0F,    0.52F,  0.35F,  0.55F,  -0.7F,   //30..34
 -0.9F,    0.52F,  0.65F, -0.90F,   0.65F,  //35..39
 -0.325F, -0.325F, 0.56F, -0.3F,   -0.05F   //40..44
};

float hudWinY[NUM_WIN] = {
  0.80F,   0.23F, -0.15F, -0.23F,   0.55F,   // 0..4	//MI value 2 from 0.20 to 0.25 and value 5 from 0.45 to 0.55
  0.23F,  -0.31F, -0.39F,  0.0F,   -0.39F,  // 5..9		//MI value 1 from 0.19 to -0.25
  0.2F,    0.05F, -0.47F, -0.55F,  -0.47F,   //10..14
  0.0F,    0.0F,   0.0F,   0.68F,  -0.45F,   //15..19
 -0.53F,  -0.61F, -0.69F, -0.78F,  -0.31F,  //20..24 DED stuff
 -0.20F,   0.32F, -0.03F, -0.11F,  -0.07F,  //25..29
  0.0F,   -0.06F,  0.21F,  0.51F,  -0.55F,   //30..34
 -0.63F,  -0.13F,  0.60F, -0.1F,   -0.1F,  //35..39
  0.85F,  -0.3F,   0.03F, -0.3F,    0.60F  //40..44
};

float hudWinWidth[NUM_WIN] = {
  0.1F,    0.05F,  0.15F,  0.15F,   0.15F,
  0.05F,   0.25F,  0.25F,  0.0F,    0.25F,
  0.65F,   0.3F,   0.25F,  0.25F,   0.35F,
  0.0F,    0.0F,   0.0F,   0.25F,   1.0F,
  1.0F,    1.0F,   1.0F,   1.0F,    0.3F,
  0.3F,    0.6F,   0.4F,   0.35F,   0.1F,
  0.0F,    0.1F,   0.2F,   0.15F,   0.15F,
  0.35F,   0.1F,   0.25F,  0.15F,   0.15F,
  0.65F,   0.65F,  0.06F,  0.5F,    0.1F
};

float hudWinHeight[NUM_WIN] = {
  0.06F,   0.06F, 0.06F,   0.06F,   0.06F,
  0.06F,   0.06F, 0.06F,   0.0F,    0.06F,
  0.1F,    0.06F, 0.06F,   0.06F,   0.06F,
  0.0F,    0.0F,  0.0F,    0.06F,   0.06F,
  0.06F,   0.06F, 0.06F,   0.06F,   0.06F,
  0.06F,   0.07F, 0.06F,   0.06F,   0.06F,
  0.0F,    0.06F, 0.06F,   0.06F,   0.06F,
  0.06F,   0.06F, 0.06F,   0.55F,   0.55F,
  0.1F,    0.1F,  0.45F,   1.7F,    0.05F
};

char *hudNumbers[101] = {
   "0","1","2","3","4","5","6","7","8","9",
   "10","11","12","13","14","15","16","17","18","19",
   "20","21","22","23","24","25","26","27","28","29",
   "30","31","32","33","34","35","36","37","38","39",
   "40","41","42","43","44","45","46","47","48","49",
   "50","51","52","53","54","55","56","57","58","59",
   "60","61","62","63","64","65","66","67","68","69",
   "70","71","72","73","74","75","76","77","78","79",
   "80","81","82","83","84","85","86","87","88","89",
   "90","91","92","93","94","95","96","97","98","99","100" };

HudClass *TheHud = NULL;

HudClass::HudClass(void) : DrawableClass ()
{
   headingPos      = Low;
   ownship         = NULL;
   HudData.tgtId   = -1;
   lowAltWarning   = 300.0F;
   maxGs           = 0.0F;
   SetHalfAngle((float)atan (0.25F * tan(30.0F * DTR)) * RTD);	//MI halfangle is degrees
   waypointX = 0.0F;
   waypointY = 0.0F;
   waypointZ = 0.0F;
   waypointRange = 0.0F;
   waypointArrival = 0.0F;
   waypointBearing = 0.0F;
   waypointRange = 0.0F;
   waypointAz = 0.0F;
   waypointEl = 0.0F;
   waypointValid = FALSE;
   //curRwy = NULL;
   curRwy = 0;

   	SetScalesSwitch( VAH );
	SetFPMSwitch( ATT_FPM );
	SetVelocitySwitch( CAS );

	dedSwitch = DED_OFF;
	radarSwitch = RADAR_AUTO;
	brightnessSwitch = DAY;
	display = NULL;
	curColorIdx = 0;
   //SYM Wheel
   SymWheelPos = 0.5F;
	SetLightLevel();
	targetPtr = NULL;
	targetData = NULL;
	eegsFrameNum = 0;
	lastEEGSstepTime = 0;
   pixelXCenter = 320.0F;
   pixelYCenter = 240.0F;
// 2000-11-10 ADDED BY S.G. FOR THE Drift C/O switch
   driftCOSwitch = DRIFT_CO_OFF;
// END OF ADDED SECTION
   //MI MSLFloor stuff
   MSLFloor = 10000;
   if(-cockpitFlightData.z >= MSLFloor)
	   WasAboveMSLFloor = TRUE;
   else
	   WasAboveMSLFloor = FALSE;
   TFAdv = 400;
   DefaultTargetSpan = 35.0f;
   WhichMode = 0;
   OA1Az = 0.0F;
   OA1Elev = 0.0F;
   OA1Valid = FALSE;
   OA2Az = 0.0F;
   OA2Elev = 0.0F;
   OA2Valid = FALSE;
   VIPAz = 0.0F;
   VIPElev = 0.0F;
   VIPValid = FALSE;
   VRPAz = 0.0F;
   VRPElev = 0.0F;
   VRPValid = FALSE;
   CalcRoll = CalcBank = TRUE;
   //added check for font so PW's pit looks good too
   if(OTWDriver.pCockpitManager->HudFont() == 1)
   {
	   YRALTText = -0.17F;
	   YALText = -0.24F;
   }
   else
   {
	   YRALTText = -0.16F;
	   YALText = -0.25F;
   }
   //MI check when to hide the funnel
   HideFunnel = FALSE;
   HideFunnelTimer = SimLibElapsedTime;
   ShowFunnelTimer = SimLibElapsedTime;
   SetHideTimer = FALSE;
   SetShowTimer = FALSE;

   SlantRange = 0.0F;
   RET_CENTER = hudWinY[BORESIGHT_CROSS_WINDOW] + hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F;
   ReticlePosition = 0;	//for the wheel
   RetPos = 0;	//controls where to move it

   sprintf(SpeedText, "");
   //MI init
   curHudColor = hudColor[curColorIdx];
}


HudClass::~HudClass(void)
{
	ClearTarget();
}

void HudClass::SetOwnship (AircraftClass* newOwnship)
{
	// Quit now if this is a redundant call
	if (ownship == newOwnship)
		return;

	// Drop any target we may have had
	ClearTarget();

	// Take the new ownship pointer
	ownship = newOwnship;

	// Initialize the gunsight and FCC pointer
	if (newOwnship) {
		FCC = ownship->FCC;
		ShiAssert( FCC );

		// Reset our history data to something reasonable for this platform.
		// TODO:  Could do a better job here by assuming level flight and calculating a false history...
		int i;
		for (i=0; i<NumEEGSFrames; i++) {
			eegsFrameArray[i].time	= SimLibElapsedTime - (NumEEGSFrames-i);
			eegsFrameArray[i].x		= newOwnship->XPos();
			eegsFrameArray[i].y		= newOwnship->YPos();
			eegsFrameArray[i].z		= newOwnship->ZPos();
			eegsFrameArray[i].vx	= 0.0f;
			eegsFrameArray[i].vy	= 0.0f;
			eegsFrameArray[i].vz	= 0.0f;
		}
		for (i=0; i<NumEEGSSegments; i++) {
			funnel1X[i] = 0.0f;
			funnel1Y[i] = 0.0f;
			funnel2X[i] = 0.0f;
			funnel2Y[i] = 0.0f;
		}
	} else {
		FCC = NULL;
	}

	eegsFrameNum = 0;
	lastEEGSstepTime = 0;
}

void HudClass::SetTarget (SimObjectType* newTarget)
{
	if (newTarget == targetPtr)
		return;
	
	ClearTarget();

	if (newTarget)
	{
		ShiAssert( newTarget->BaseData() != (FalconEntity*)0xDDDDDDDD );
		newTarget->Reference( SIM_OBJ_REF_ARGS );
		targetPtr = newTarget;
		targetData = newTarget->localData;
	}
}

void HudClass::ClearTarget (void)
{
	if (targetPtr) {
		targetPtr->Release( SIM_OBJ_REF_ARGS );
		targetPtr = NULL;
		targetData = NULL;
	}
}

void HudClass::DisplayInit (ImageBuffer* image)
{
   DisplayExit();

   privateDisplay = new Render2D;
   ((Render2D*)privateDisplay)->Setup (image);

   privateDisplay->SetColor (0xff00ff00);
}

void HudClass::Display (VirtualDisplay *newDisplay)
{

char tmpStr[240];
mlTrig rollTrig;

	// We must have one to draw...
	ShiAssert( ownship );

   // Various ways to be broken
   if (ownship->mFaults &&
       (
        (ownship->mFaults->GetFault(FaultClass::flcs_fault) & FaultClass::dmux) ||
         ownship->mFaults->GetFault(FaultClass::dmux_fault) ||
         ownship->mFaults->GetFault(FaultClass::hud_fault)
       )
      )
   {
	   //MI still allow STBY reticle for bombing
	   if(FCC->GetSubMode() == FireControlComputer::MAN && WhichMode == 2)
		  DrawMANReticle();
      return;
   }
   // JPO - check systems have power
   if (!ownship->HasPower(AircraftClass::HUDPower))
       return;
   display = newDisplay;

   // Do we draw flashing things this frame?
   flash = (vuxRealTime & 0x200);
   Warnflash = (vuxRealTime & 0x080);
   mlSinCos (&rollTrig, cockpitFlightData.roll);
   alphaHudUnits = RadToHudUnits(cockpitFlightData.alpha*DTR - cockpitFlightData.windOffset*rollTrig.sin);
// 2000-11-10 MODIFIED BY S.G. TO HANDLE THE 'driftCO' switch
// if (ownship->OnGround()) {
   if (ownship->OnGround() || driftCOSwitch == DRIFT_CO_ON) {
#if 1
	   // With weight on wheels, the jet turns on "drift cutout" to keep the fpm centered on the HUD
	   betaHudUnits = 0.0f;
#else
	   // While I'm looking for the pitch ladder clipping bug...
	   betaHudUnits = RadToHudUnits(cockpitFlightData.beta*DTR + cockpitFlightData.windOffset*rollTrig.cos);
#endif
   } else {
	   betaHudUnits = RadToHudUnits(cockpitFlightData.beta*DTR + cockpitFlightData.windOffset*rollTrig.cos);
   }

   //MI
   if(g_bRealisticAvionics)
   {
	   if(!(WhichMode == 2 && FCC->GetSubMode() == FireControlComputer::MAN))
	   {
		   DrawBoresightCross();
		   DrawAirspeed();
		   DrawAltitude();
		   // Marco Edit
		   if (FCC && FCC->GetMasterMode() !=FireControlComputer::Dogfight)
			   DrawHeading();		// Don't draw heading in Dogfight Mode
	   }
   }
   else
   {
	   DrawBoresightCross();
	   DrawAirspeed();
	   DrawAltitude();
	   // Marco Edit
	   if (FCC && FCC->GetMasterMode() !=FireControlComputer::Dogfight)
		   DrawHeading();		// Don't draw heading in Dogfight Mode
   }
#if 0	// Turned off to evaluate easy mode HUD clutter level...  SCR 9/18/98
#ifdef _DEBUG
{
int i;
int days, hours, minutes;
double cur_time;

	cur_time = SimLibElapsedTime / SEC_TO_MSEC;
	days = FloatToInt32(cur_time / (3600.0F * 24.0F));
	cur_time -= days * 3600.0F * 24.0F;
	hours = FloatToInt32(cur_time / 3600.0F);
	cur_time -= hours * 3600.0F;
	minutes = FloatToInt32(cur_time / 60.0F);
	cur_time -= minutes * 60.0F;
	sprintf (tmpStr, "%3d-%02d:%02d:%05.2f", days, hours, minutes, (float)cur_time);
   display->TextRight (0.95F, 0.9F, tmpStr);
   ShiAssert (strlen(tmpStr) < sizeof(tmpStr));

	sprintf (tmpStr, "T%-4d", HudData.tgtId);
   ShiAssert (strlen(tmpStr) < sizeof(tmpStr));
	display->TextLeft (-0.9F, 0.9F, tmpStr);
   sprintf (tmpStr, "C%3d", ownship->Id().num_);
   ShiAssert (strlen(tmpStr) < sizeof(tmpStr));
	display->TextLeft (-0.9F, 0.8F, tmpStr);

   sprintf (tmpStr, "S%3.0f", ownship->af->dbrake*100.0F);
   ShiAssert (strlen(tmpStr) < sizeof(tmpStr));
   display->TextLeft (-0.9F, 0.6F, tmpStr);

   for (i=0; i<10; i++)
      if (debstr[i][0])
         display->TextLeft (-0.8F, 0.8F - 0.1F * i, debstr[i]);
}
#endif
#endif

	if (fpmSwitch == FPM_AUTO)
	{
		if (fabs(cockpitFlightData.pitch) > 10.0f*DTR) {
			DrawPitchLadder();
		} else {
			DrawHorizonLine();
		}
	}
	else if ((ownship && ownship->af && ownship->af->gearPos > 0.5F) || (fpmSwitch == ATT_FPM))
	{
	if  (FCC && FCC->GetMasterMode() !=FireControlComputer::Dogfight)
	    DrawPitchLadder();//me123 status ok. don't draw ladders in dogfight mode
	}

	if (IsSOI())
	{
	    if (g_bRealisticAvionics)
		display->TextLeft(-0.8F, 0.65F, "*");
	    else
		display->TextLeft(-0.8F, 0.65F, "SOI");
	}

   DrawAlphaNumeric();

   if ((!FCC->postDrop || flash) && 
       fpmSwitch != FPM_OFF && 
       FCC && FCC->GetMasterMode() !=FireControlComputer::Dogfight) // JPO not show in DGFT
      DrawFPM();

   switch (FCC->GetMasterMode())
   {
   case FireControlComputer::Gun:
       if (FCC->GetSubMode() != FireControlComputer::STRAF) 
	   {
		   DrawTDBox();
		   DrawGuns();
       }
       else    
	   DrawAirGroundGravity();
      break;

      case FireControlComputer::Dogfight:
         DrawDogfight();
      break;

      case FireControlComputer::MissileOverride:
         DrawMissileOverride();
      break;

      case FireControlComputer::Missile:
         DrawAirMissile();
      break;

      case FireControlComputer::AirGroundHARM:
         DrawHarm();
      break;

      case FireControlComputer::AirGroundMissile:
		 DrawGroundMissile();
      break;

      case FireControlComputer::ILS:
		  DrawILS();
		  //MI
		  if(OTWDriver.pCockpitManager) 
		  {
			  if(OTWDriver.pCockpitManager->mpIcp->GetCMDSTR())
				  DrawCMDSTRG();
		  }
      break;

      case FireControlComputer::Nav:
         DrawNav();
      break;

      case FireControlComputer::AirGroundBomb:
         DrawAirGroundGravity();
      break;

      case FireControlComputer::AirGroundLaser:
         DrawTargetingPod();
      break;

      case FireControlComputer::AirGroundCamera:
         DrawRPod();
      break;
   }

   // Check ground Collision
   if (flash && ownship && ownship->mFaults->GetFault(alt_low))
   {
      display->AdjustOriginInViewport (0.0F, hudWinY[BORESIGHT_CROSS_WINDOW] +
         hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F);
      display->AdjustOriginInViewport (0.0F, MISSILE_RETICLE_OFFSET);
      display->Line (0.4F,  0.4F, -0.4F, -0.4F);
      display->Line (0.4F, -0.4F, -0.4F,  0.4F);
      display->AdjustOriginInViewport (0.0F, -MISSILE_RETICLE_OFFSET);
      display->AdjustOriginInViewport (0.0F, -(hudWinY[BORESIGHT_CROSS_WINDOW] +
         hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
   }

   //MI removed RPM indication
   //M.N. we need RPM indication for Flightmodel testing
   if (!g_bNoRPMOnHud/* || !g_bRealisticAvionics*/)
   {
	   if (ownship)
	   {
		  sprintf (tmpStr, "RPM %3d", FloatToInt32(ownship->af->rpm*100.0F+0.99F));
		  ShiAssert (strlen(tmpStr) < sizeof(tmpStr));
			  display->TextLeft (-0.9F, -0.75F, tmpStr);
	   }
   }

   display->ZeroRotationAboutOrigin();
   display->CenterOriginInViewport();
}

//-------------------------------
// Scales Switch
//-------------------------------

int HudClass::GetScalesSwitch(void) {

	return scalesSwitch;
}

void HudClass::SetScalesSwitch(ScalesSwitch state) {
	if (PlayerOptions.GetAvionicsType() == ATEasy) {
		scalesSwitch = H;
	} else {
		scalesSwitch = state;
	}
}

void HudClass::CycleScalesSwitch(void) {

	switch(scalesSwitch) {
	  case VV_VAH:
		SetScalesSwitch( VAH );
		break;
	  case VAH: 
		SetScalesSwitch( SS_OFF );
		break;
	  case SS_OFF:
		SetScalesSwitch( VV_VAH );
		break;
     default:
      SetScalesSwitch (VAH);
      break;
	}
}

//-------------------------------
// FPM Switch
//-------------------------------

int HudClass::GetFPMSwitch(void) {

	return fpmSwitch;
}

void HudClass::SetFPMSwitch(FPMSwitch state) {
	if (PlayerOptions.GetAvionicsType() == ATEasy) {
       fpmSwitch = FPM_AUTO;
	} else {
       fpmSwitch = state;
	}
}

void HudClass::CycleFPMSwitch(void) {
	switch(fpmSwitch) {
	  case ATT_FPM:
		SetFPMSwitch( FPM );
		break;
	  case FPM:
		SetFPMSwitch( FPM_OFF );
		break;
	  case FPM_OFF:
		SetFPMSwitch( ATT_FPM );
		break;
     default:
        SetFPMSwitch(ATT_FPM);
      break;
	}
}

// 2000-11-10 FUNCTIONS ADDED BY S.G. FOR THE Drift C/O switch
//-------------------------------
// Drift C/O Switch
//-------------------------------

int HudClass::GetDriftCOSwitch(void) {

	return driftCOSwitch;
}

void HudClass::SetDriftCOSwitch(DriftCOSwitch state) {

	driftCOSwitch = state;
}

void HudClass::CycleDriftCOSwitch(void) {
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) {
		switch(driftCOSwitch) {
		case DRIFT_CO_ON:
			driftCOSwitch = DRIFT_CO_OFF;
		break;
		case DRIFT_CO_OFF:
			driftCOSwitch = DRIFT_CO_ON;
		break;
		}
	}
}

// END OF ADDED SECTION

//-------------------------------
// DED Switch
//-------------------------------

int HudClass::GetDEDSwitch(void) {

	return dedSwitch;
}

void HudClass::SetDEDSwitch(DEDSwitch state) {

	dedSwitch = state;
}

void HudClass::CycleDEDSwitch(void) {
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) 
	{
		//MI
		if(!g_bRealisticAvionics)
		{
			switch(dedSwitch) 
			{
			case DED_DATA:
				dedSwitch = DED_OFF;
				break;
			case DED_OFF:
				dedSwitch = DED_DATA;
				break;
			}
		}
		else
		{
			switch(dedSwitch) 
			{
			case DED_OFF:
				dedSwitch = PFL_DATA;
			break;
			case PFL_DATA:
				dedSwitch = DED_DATA;
			break;
			case DED_DATA:
				dedSwitch = DED_OFF;
			break;
			default:
			break;
			}
		}
	}
}

//-------------------------------
// Velocity Switch
//-------------------------------

int HudClass::GetVelocitySwitch(void)
{
	return velocitySwitch;
}

void HudClass::SetVelocitySwitch(VelocitySwitch state)
{
	if (PlayerOptions.GetAvionicsType() == ATEasy) {
		velocitySwitch = CAS;
	} else {
		velocitySwitch = state;
	}
}

void HudClass::CycleVelocitySwitch(void) {

	switch(velocitySwitch) {
	  case CAS:
		SetVelocitySwitch( TAS );
		break;
	  case TAS:
		SetVelocitySwitch( GND_SPD );
		break;
	  case GND_SPD:
		SetVelocitySwitch( CAS );
		break;
	}
}

//-------------------------------
// Radar Switch
//-------------------------------

int HudClass::GetRadarSwitch(void) {

	return radarSwitch;
}

void HudClass::SetRadarSwitch(RadarSwitch state) {

	radarSwitch = state;
}

void HudClass::CycleRadarSwitch(void) {

	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) {
		switch(radarSwitch) {
		case ALT_RADAR:
			radarSwitch = BARO;
		break;
		case BARO:
			radarSwitch = RADAR_AUTO;
		break;
		case RADAR_AUTO:
			radarSwitch = ALT_RADAR;
		break;
		}
	}
}

//-------------------------------
// Brightness Switch
//-------------------------------

int HudClass::GetBrightnessSwitch(void) {

	return brightnessSwitch;
}

void HudClass::SetBrightnessSwitch(BrightnessSwitch state) {

	brightnessSwitch = state;
}

void HudClass::CycleBrightnessSwitch(void) {
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) {
		switch(brightnessSwitch) {
		case DAY:
			brightnessSwitch = BRIGHT_AUTO;
		break;
		case BRIGHT_AUTO:
			brightnessSwitch = NIGHT;
		break;
		case NIGHT:
			brightnessSwitch = DAY;
		break;
		default:
			brightnessSwitch = DAY;
		break;
		}

		SetLightLevel();
	}
}
//MI
void HudClass::CycleBrightnessSwitchUp(void) 
{
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) 
	{
		if(!SimDriver.playerEntity->HasPower(AircraftClass::HUDPower))
			SimDriver.playerEntity->PowerOn(AircraftClass::HUDPower);
		switch(brightnessSwitch) 
		{
		case OFF:
			brightnessSwitch = NIGHT;
		break;
		case NIGHT:
			brightnessSwitch = BRIGHT_AUTO;
		break;
		case BRIGHT_AUTO:
			brightnessSwitch = DAY;
		break;
		default:
		break;
		}

		SetLightLevel();
	}
}
void HudClass::CycleBrightnessSwitchDown(void) 
{
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) {
		switch(brightnessSwitch) 
		{
		case DAY:
			brightnessSwitch = BRIGHT_AUTO;
		break;
		case BRIGHT_AUTO:
			brightnessSwitch = NIGHT;
		break;
		case NIGHT:
			brightnessSwitch = OFF;
			SimDriver.playerEntity->PowerOff(AircraftClass::HUDPower);
		break;
		default:
		break;		
		}

		SetLightLevel();
	}
}


void HudClass::DrawAlphaNumeric(void)
{
	//MI not here in BUP reticle mode
	if(g_bRealisticAvionics && WhichMode == 2 && FCC->GetSubMode() == FireControlComputer::MAN)
		return;

char tmpStr[40];

   // Window 3 (Master Arm / ILS)
   if (PlayerOptions.GetAvionicsType() != ATEasy) {
	   if (FCC && FCC->GetMasterMode() == FireControlComputer::ILS)
		  DrawWindowString (3, "ILS");
	   else switch (ownship->Sms->MasterArm())
      {
         case SMSBaseClass::Safe:
			 //MI not here in real
			 if(!g_bRealisticAvionics)
				 DrawWindowString (3, "SAF");
         break;

         case SMSBaseClass::Arm:
            DrawWindowString (3, "ARM");
         break;

         case SMSBaseClass::Sim:
            DrawWindowString (3, "SIM");
         break;
      }
   }

   // Window 4 (Mach Number)
   sprintf (tmpStr, "%.2f", cockpitFlightData.mach);
   ShiAssert (strlen(tmpStr) < 40);
   DrawWindowString (4, tmpStr);

   // Window 5 (Gs)
   //MI (JPO - fixed elsewhere !)
   sprintf (tmpStr, "%.1f", cockpitFlightData.gs);
   ShiAssert (strlen(tmpStr) < 40);
   DrawWindowString (5, tmpStr);

   if (PlayerOptions.GetAvionicsType() != ATEasy) 
   {
	   //MI changed for INS stuff
	   if(g_bINS && g_bRealisticAvionics)
	   {
		   if(!ownship->INSState(AircraftClass::INS_Aligned) &&
			   ownship->INSState(AircraftClass::INS_AlignNorm) && (cockpitFlightData.kias <= 1.0F
			   && !ownship->INS60kts) || ownship->INSState(AircraftClass::INS_AlignFlight))
		   {
			   sprintf(tmpStr,"ALIGN");
		   }
		   else if(ownship->INSState(AircraftClass::INS_Aligned) &&
			   ownship->INSState(AircraftClass::INS_AlignNorm) && cockpitFlightData.kias <= 1.0F
			   && !ownship->INS60kts || ownship->INSState(AircraftClass::INS_AlignFlight))
		   {
			   if(flash)
				   sprintf(tmpStr, "ALIGN");
			   else
			   {
				   if (cockpitFlightData.gs > maxGs)
					   maxGs = cockpitFlightData.gs;
				   sprintf (tmpStr, "%.1f", maxGs);
			   }
		   }
		   else
		   {
			   if (cockpitFlightData.gs > maxGs)
				   maxGs = cockpitFlightData.gs;
			   sprintf (tmpStr, "%.1f", maxGs);
			   
		   }
	   }
	   else
	   {
		   // Window 7 (Max Gs)
		   if (cockpitFlightData.gs > maxGs)
			   maxGs = cockpitFlightData.gs;
		   sprintf (tmpStr, "%.1f", maxGs);
	   }
	   ShiAssert (strlen(tmpStr) < 40);
	   DrawWindowString (7, tmpStr);
   }

   // Window 8 (Master/Sub Mode)
   if (ownship->Sms->drawable && ownship->Sms->drawable->DisplayMode() == SmsDrawable::SelJet)
      DrawWindowString (8, "JETT");
   else
   {
	   //MI
	   if(!g_bRealisticAvionics)
	   {
		   if (ownship->Sms->NumCurrentWpn() >= 0)
			   sprintf (tmpStr, "%d %s", ownship->Sms->NumCurrentWpn(), FCC->subModeString);
		   else
			   sprintf (tmpStr, "%s", FCC->subModeString);
	   }
	   else
	   {
		   if(FCC && FCC->IsAGMasterMode())
		   {
			   if(ownship->Sms && ownship->Sms->curWeaponType == wtAgm65)
			   {
				   if(ownship->Sms->MavSubMode == SMSBaseClass::PRE)
					   sprintf(tmpStr,"PRE");
				   else if(ownship->Sms->MavSubMode == SMSBaseClass::VIS)
					   sprintf(tmpStr, "VIS");
				   else
					   sprintf(tmpStr,"BORE");
			   }
			   else
				   sprintf (tmpStr, "%s", FCC->subModeString);
		   }
		   else
		   {
			   if (ownship->Sms->NumCurrentWpn() >= 0)
				   sprintf (tmpStr, "%d %s", ownship->Sms->NumCurrentWpn(), FCC->subModeString);
			   else
				   sprintf (tmpStr, "%s", FCC->subModeString);
		   }
	   }
	   
	   DrawWindowString (8, tmpStr);
   }

   // Window 10 (Range)
   // Done in missile or gun mode section

   // Window 11 (Master Caution)
   //MI Warn reset is correct
   int ofont = display->CurFont();
   display->SetFont(3);
   if(!g_bRealisticAvionics)
   {
	   if (ownship->mFaults->MasterCaution() && flash)
	   {
		   DrawWindowString (11, "WARN");
	   }
   }
   else
   {
	   if(ownship->mFaults->WarnReset() && Warnflash)
	   {
		   //Fuel doesn't flash warning
		   if(!ownship->mFaults->GetFault(fuel_low_fault) &&
			   !ownship->mFaults->GetFault(fuel_trapped) &&
			   !ownship->mFaults->GetFault(fuel_home))
			   DrawWindowString(11, "WARN");
	   }
   }
   display->SetFont(ofont);

   // Window 12, 15 (Bingo, trapped or home Fuel) JPO additions
   if (ownship->mFaults->GetFault(fuel_low_fault) ||
       ownship->mFaults->GetFault(fuel_trapped) ||
       ownship->mFaults->GetFault(fuel_home))
   {
	   //MI Warn Reset is correct
	   if(!g_bRealisticAvionics)
	   {
		   if (ownship->mFaults->MasterCaution() && flash)
			   DrawWindowString (12, "FUEL");
	   }
	   else
	   {
		   if(ownship->mFaults->WarnReset() && Warnflash)
			   DrawWindowString(12,"FUEL");
	   } 

      // JPO existing BINGO fuel fault
      if (ownship->mFaults->GetFault(fuel_low_fault) &&
	  PlayerOptions.GetAvionicsType() != ATEasy)
      {
	  DrawWindowString (15, "FUEL");
      }
      // JPO additional Trapped fuel
      if (ownship->mFaults->GetFault(fuel_trapped) &&
	  PlayerOptions.GetAvionicsType() != ATEasy)
      {
	  DrawWindowString (15, "TRP FUEL");
      }
      // JPO additional less than 700lbs over home plate.
      if (ownship->mFaults->GetFault(fuel_home) &&
	  PlayerOptions.GetAvionicsType() != ATEasy)
      {
		  //MI
		  char tempstr[10] = "";
		  sprintf(tempstr, "Fuel %03d", ownship->af->HomeFuel/100);
		  DrawWindowString (15, tempstr); //XXX work out real value.
      }

	  //MI warn reset is correct
	  if(!g_bRealisticAvionics)
	  {
      if (ownship->mFaults->GetFault(fuel_low_fault) &&
	  ownship->mFaults->MasterCaution() &&
          F4SoundFXPlaying( ownship->af->GetBingoSnd())) // JB 010425
      {
         F4SoundFXSetDist( ownship->af->GetBingoSnd(), FALSE, 0.0f, 1.0f );
      }
	  }
	  //MI fix for Bingo warning. This get's called in cautions.cpp
#if 0
	  else
	  {
		  if (ownship->mFaults->GetFault(fuel_low_fault) && 
			  ownship->mFaults->WarnReset() &&
			  F4SoundFXPlaying( ownship->af->GetBingoSnd())) // JB 010425
			  {
				F4SoundFXSetDist( ownship->af->GetBingoSnd(), FALSE, 0.0f, 1.0f );
			  }
	  }
#endif
   }

   // Window 13 (Closure)
   // Done in missile or gun section

   // Window 14 (Steerpoint)
   // Done in nav section

   // Window 19 (Variable)
   // Done in missile or gun section

   //MI they wanted DED data, so here it is.....
   //if (dedSwitch == DED_DATA && OTWDriver.GetOTWDisplayMode() == OTWDriverClass::ModeHud)
   if (dedSwitch == DED_DATA || dedSwitch == PFL_DATA)// && OTWDriver.GetOTWDisplayMode() == OTWDriverClass::ModeHud)
   {
		if(OTWDriver.pCockpitManager) {
	
			if(!g_bRealisticAvionics && dedSwitch == DED_DATA)
			{
				char line1[40];
				char line2[40];
				char line3[40];

				OTWDriver.pCockpitManager->mpIcp->Exec();
				OTWDriver.pCockpitManager->mpIcp->GetDEDStrings(line1, line2, line3);
				
				DrawWindowString (21, line1);
				DrawWindowString (22, line2);
				DrawWindowString (23, line3);
			}
			else
			{
				if(!SimDriver.playerEntity->HasPower(AircraftClass::UFCPower) ||
					(FCC && FCC->GetMasterMode() == FireControlComputer::Dogfight))
					return;

				OTWDriver.pCockpitManager->mpIcp->Exec();
				OTWDriver.pCockpitManager->mpIcp->ExecPfl();
			
				char line1[27] = "";
				char line2[27] = "";
				char line3[27] = "";
				char line4[27] = "";
				char line5[27] = "";
				line1[26] = '\0';
				line2[26] = '\0';
				line3[26] = '\0';
				line4[26] = '\0';
				line5[26] = '\0';
				static float xPos = -0.70F;
				float yPos = -0.60F;
				for(int j = 0; j < 5; j++)
				{
					for(int i = 0; i < 26; i++)
					{
						switch(j)
						{
						case 0:
							if(dedSwitch == DED_DATA)
								line1[i] = OTWDriver.pCockpitManager->mpIcp->DEDLines[j][i];
							else if(dedSwitch == PFL_DATA)
								line1[i] = OTWDriver.pCockpitManager->mpIcp->PFLLines[j][i];
							break;
						case 1:
							if(dedSwitch == DED_DATA)
								line2[i] = OTWDriver.pCockpitManager->mpIcp->DEDLines[j][i];
							else if(dedSwitch == PFL_DATA)
								line2[i] = OTWDriver.pCockpitManager->mpIcp->PFLLines[j][i];
							break;
						case 2:
							if(dedSwitch == DED_DATA)
								line3[i] = OTWDriver.pCockpitManager->mpIcp->DEDLines[j][i];
							else if(dedSwitch == PFL_DATA)
								line3[i] = OTWDriver.pCockpitManager->mpIcp->PFLLines[j][i];
							break;
						case 3:
							if(dedSwitch == DED_DATA)
								line4[i] = OTWDriver.pCockpitManager->mpIcp->DEDLines[j][i];
							else if(dedSwitch == PFL_DATA)
								line4[i] = OTWDriver.pCockpitManager->mpIcp->PFLLines[j][i];
							break;
						case 4:
							if(dedSwitch == DED_DATA)
								line5[i] = OTWDriver.pCockpitManager->mpIcp->DEDLines[j][i];
							else if(dedSwitch == PFL_DATA)
								line5[i] = OTWDriver.pCockpitManager->mpIcp->PFLLines[j][i];
							break;				
						}
					}
					switch(j)
					{
					case 0:
						display->TextLeft (xPos, yPos + 0.03F, line1);
						break;
					case 1:
						display->TextLeft (xPos, yPos + 0.03F, line2);
						break;
					case 2:
						display->TextLeft (xPos, yPos + 0.03F, line3);
						break;
					case 3:
						display->TextLeft (xPos, yPos + 0.03F, line4);
						break;
					case 4:
						display->TextLeft (xPos, yPos + 0.03F, line5);
						break;
					default:
						break;
					}
					yPos -= 0.08F;
				}
			}
		}
   }

   //MI TFR info if needed
   if(theLantirn && theLantirn->IsEnabled() && ownship && ownship->mFaults && !ownship->mFaults->WarnReset()
	   && ownship->RFState != 2)
   {
	   char tempstr[20] = "";
	   if (theLantirn->evasize  == 1 && flash)
		   sprintf(tempstr, "FLY UP");
	   else if (theLantirn->evasize  == 2 && flash)
		   sprintf(tempstr, "OBSTACLE");
	   if (theLantirn->SpeedUp && !flash)
		   sprintf(tempstr, "SLOW");
	   
	   display->TextCenter (0, 0.25, tempstr);
   }
}

void HudClass::DrawWindowString (int window, char *str, int boxed)
{
float x, y, width, height;

   window --;

   x = hudWinX[window];
   y = hudWinY[window];
   width = hudWinWidth[window];
   height = hudWinHeight[window];

   if (x > 0.01F)
      display->TextRight (x + width, y + height * 0.5F, str, boxed);
   else if (x < -0.01F)
      display->TextLeft (x, y + height * 0.5F, str, boxed);
   else
      display->TextCenter (x, y + height * 0.5F, str, boxed);
}
const static float Tadpolesize = 0.029F;
const static float Linelenght = 0.07F;
void HudClass::DrawFPM(void)
{
	//MI not here in BUP reticle mode
	if(g_bRealisticAvionics && WhichMode == 2 && FCC->GetSubMode() == FireControlComputer::MAN)
		return;

float dx, dy;

	ShiAssert(ownship);

   dx = betaHudUnits;
   dy = hudWinY[BORESIGHT_CROSS_WINDOW] +
      hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F -
      alphaHudUnits;

   //MI -- Make the FPM a bit bigger....why did they make it so complicated??
	if(!g_bRealisticAvionics)
	{
   display->Line (-0.0075F + dx, 0.015F + dy, 0.0075F + dx, 0.015F + dy);
   display->Line (0.0075F + dx, 0.015F + dy, 0.015F + dx, 0.0075F + dy);
   display->Line (0.015F + dx, 0.0075F + dy, 0.015F + dx, -0.0075F + dy);
   display->Line (0.015F + dx, -0.0075F + dy, 0.0075F + dx, -0.015F + dy);
   display->Line (0.0075F + dx, -0.015F + dy, -0.0075F + dx, -0.015F + dy);
   display->Line (-0.0075F + dx, -0.015F + dy, -0.015F + dx, -0.0075F + dy);
   display->Line (-0.015F + dx, -0.0075F + dy, -0.015F + dx, 0.0075F + dy);
   display->Line (-0.015F + dx, 0.0075F + dy, -0.0075F + dx, 0.015F + dy);
   display->Line (0.015F + dx, dy, 0.05F + dx, dy);
   display->Line (-0.015F + dx, dy, -0.05F + dx, dy);
   display->Line (dx, 0.015F + dy, dx, 0.05F + dy);
	}
	else
	{
		//MI
		if(g_bINS && g_bRealisticAvionics)
		{
			if(ownship->INSState(AircraftClass::INS_HUD_FPM))
			{
				display->Circle(dx,dy,Tadpolesize);
				display->Line(dx + Tadpolesize, dy, dx + Tadpolesize + Linelenght, dy);
				display->Line(dx - Tadpolesize, dy, dx - Tadpolesize - Linelenght, dy);
				display->Line(dx, dy + Tadpolesize, dx, dy + Tadpolesize + Linelenght - 0.025);
			}
		}
		else
		{
			display->Circle(dx,dy,Tadpolesize);
			display->Line(dx + Tadpolesize, dy, dx + Tadpolesize + Linelenght, dy);
			display->Line(dx - Tadpolesize, dy, dx - Tadpolesize - Linelenght, dy);
			display->Line(dx, dy + Tadpolesize, dx, dy + Tadpolesize + Linelenght - 0.025);
		}
	}


   // AOA Bracket 11 - 15 Degrees
   // Check for gear down
   if (((AircraftClass*)ownship)->af->gearPos > 0.5F)
   {
   float aoaOffset = cockpitFlightData.alpha - 13.0F;

      //aoaOffset = min ( max (aoaOffset, -2.0F), 2.0F) / 2.0F;
	  aoaOffset = aoaOffset / 2.0F;
      display->Line (-0.15F + dx, -0.12F + 0.12F * aoaOffset + dy,
         -0.15F + dx, 0.12F + 0.12F * aoaOffset + dy);
      display->Line (-0.15F + dx, -0.12F + 0.12F * aoaOffset + dy,
         -0.13F + dx,-0.12F + 0.12F * aoaOffset + dy);
      display->Line (-0.15F + dx,  0.12F + 0.12F * aoaOffset + dy,
         -0.13F + dx, 0.12F + 0.12F * aoaOffset + dy);
   }
}

void HudClass::DrawTDBox(void)
{
	if (targetData)
	{
		display->AdjustOriginInViewport (0.0F, (hudWinY[BORESIGHT_CROSS_WINDOW] +
			hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
		
		DrawDesignateMarker( Square, targetData->az, targetData->el, targetData->droll );
		
		display->AdjustOriginInViewport (0.0F, -(hudWinY[BORESIGHT_CROSS_WINDOW] +
			hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
	}
}


void HudClass::DrawDesignateMarker(DesignateShape shape, float az, float el, float dRoll)
{
float xPos, yPos;
char tmpStr[12];
mlTrig trig;
float offset = MRToHudUnits(45.0F);

	xPos = RadToHudUnits(az);
	yPos = RadToHudUnits(el);

   if (fabs (az) < 90.0F * DTR &&
       fabs (el) < 90.0F * DTR &&
       fabs (xPos) < 0.90F && fabs(yPos + hudWinY[BORESIGHT_CROSS_WINDOW] +
         hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F) < 0.90F)
   {
      if (shape == Square)
      {
         display->AdjustOriginInViewport(xPos, yPos);
         display->Line (-0.05F, -0.05F, -0.05F,  0.05F);
         display->Line (-0.05F, -0.05F,  0.05F, -0.05F);
         display->Line ( 0.05F,  0.05F, -0.05F,  0.05F);
         display->Line ( 0.05F,  0.05F,  0.05F, -0.05F);
         display->AdjustOriginInViewport(-xPos, -yPos);
      }
      else if (shape == Circle)
      {
         display->Circle (xPos, yPos, 0.05F);
      }
   }
   else
   {
      mlSinCos (&trig, dRoll);
      xPos = offset * trig.sin;
      yPos = offset * trig.cos;
      display->Line (0.0f, 0.0f, xPos, yPos);
      sprintf (tmpStr, "%.0f", (float)acos (cos (az) * cos (el)) * RTD);
      ShiAssert (strlen(tmpStr) < sizeof(tmpStr));
      display->TextRight(-0.075F, display->TextHeight()*0.5F, tmpStr);
   }
}


void HudClass::DrawBoresightCross(void)
{
float xCenter = hudWinX[BORESIGHT_CROSS_WINDOW] + hudWinWidth[BORESIGHT_CROSS_WINDOW] * 0.5F;
float yCenter = hudWinY[BORESIGHT_CROSS_WINDOW] + hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F;

   display->Line ( xCenter + 0.025F, yCenter, xCenter + 0.05F, yCenter);
   display->Line ( xCenter - 0.025F, yCenter, xCenter - 0.05F, yCenter);
   display->Line ( xCenter, yCenter + 0.025F, xCenter, yCenter + 0.05F);
   display->Line ( xCenter, yCenter - 0.025F, xCenter, yCenter - 0.05F);

   if (HudData.IsSet(HudDataType::RadarNoRad))
   {
	   //MI
	   if(!g_bRealisticAvionics)
		   display->TextCenter (xCenter, yCenter + 0.075F, "NO RAD");
	   else
		   display->TextCenter (xCenter, yCenter + 0.15F, "NO RAD");
   }

   if (HudData.IsSet(HudDataType::RadarBoresight | HudDataType::RadarSlew))
   {
	   if(!g_bRealisticAvionics)
	   {
		   yCenter -= RadToHudUnits(3.0F * DTR);
		   display->Line ( xCenter + 0.05F, yCenter, xCenter + 0.1F, yCenter);
		   display->Line ( xCenter - 0.05F, yCenter, xCenter - 0.1F, yCenter);
		   display->Line ( xCenter, yCenter + 0.05F, xCenter, yCenter + 0.15F);
		   display->Line ( xCenter, yCenter - 0.05F, xCenter, yCenter - 0.15F);
		   display->Point (xCenter, yCenter);
	   }
	   else
	   {
		   //MI draw the cross correct
		   if(HudData.IsSet(HudDataType::RadarSlew))
		   {
			   yCenter -= RadToHudUnits(3.0F * DTR);
			   display->Line ( xCenter + 0.02F, yCenter, xCenter + 0.1F, yCenter);
			   display->Line ( xCenter - 0.02F, yCenter, xCenter - 0.1F, yCenter);
			   display->Line(xCenter, yCenter + 0.02F, xCenter, yCenter + 0.0533F);
			   display->Line(xCenter, yCenter + 0.0733F, xCenter, yCenter + 0.1066F);
			   display->Line(xCenter, yCenter + 0.1266F, xCenter, yCenter + 0.16F);
			   display->Line(xCenter, yCenter - 0.02F, xCenter, yCenter - 0.0533F);
			   display->Line(xCenter, yCenter - 0.0733F, xCenter, yCenter - 0.1066F);
			   display->Line(xCenter, yCenter - 0.1266F, xCenter, yCenter - 0.16F);
			   display->Point (xCenter, yCenter);
		   }
		   else
		   {
			   yCenter -= RadToHudUnits(3.0F * DTR);
			   display->Line ( xCenter + 0.02F, yCenter, xCenter + 0.1F, yCenter);
			   display->Line ( xCenter - 0.02F, yCenter, xCenter - 0.1F, yCenter);
			   display->Line ( xCenter, yCenter + 0.02F, xCenter, yCenter + 0.16F);
			   display->Line ( xCenter, yCenter - 0.02F, xCenter, yCenter - 0.16F);
			   display->Point (xCenter, yCenter);
		   }
	   }
      if (HudData.IsSet(HudDataType::RadarSlew))
      {
         display->Circle (xCenter + HudData.radarAz/(60.0F * DTR) * 0.1F,
            yCenter + HudData.radarEl/(60.0F * DTR) * 0.15F, 0.025F);
      }
   }

   if (HudData.IsSet(HudDataType::RadarVertical))
   {
      display->Line ( xCenter, yCenter - 0.15F, xCenter, yCenter - 2.0F);
   }
}


void HudClass::DrawHorizonLine(void)
{
	float dx, dy;
	float x1, x2, y;
	
	// Pitch ladder is centered about the flight path marker
	dx = betaHudUnits;
	dy = -alphaHudUnits;
	display->AdjustOriginInViewport (dx, hudWinY[BORESIGHT_CROSS_WINDOW] +
		hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F + dy);

	// AdjustRotationAboutOrigin about nose marker
	display->AdjustRotationAboutOrigin(-cockpitFlightData.roll);
	x1	= hudWinWidth[PITCH_LADDER_WINDOW] * 0.25F;
	x2	= hudWinWidth[PITCH_LADDER_WINDOW] * 2.00F;
	y	= -cockpitFlightData.gamma * RTD * degreesForScreen;

	display->Line( x1, y,  x2, y);
	display->Line(-x1, y, -x2, y);

	// Put the display offsets back the way they were
	display->ZeroRotationAboutOrigin();
	display->AdjustOriginInViewport (-dx, -hudWinY[BORESIGHT_CROSS_WINDOW] -
		hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F - dy);
}

const static float PitchLadderDiff = 0.15F;
void HudClass::DrawPitchLadder(void)
{

	//MI INS stuff
	if(g_bRealisticAvionics && g_bINS)
	{
		if(ownship && ownship->INSState(AircraftClass::INS_PowerOff) ||
			!ownship->INSState(AircraftClass::INS_HUD_STUFF))
			return;
	}
	//MI not here in BUP reticle mode
	if(g_bRealisticAvionics && WhichMode == 2 && FCC->GetSubMode() == FireControlComputer::MAN)
		return;

char tmpStr[12];
float vert[18][2];
int i, a;
float dx, dy;

   // Pitch ladder is centered about the boresight cross
   display->AdjustOriginInViewport (0.0F, hudWinY[BORESIGHT_CROSS_WINDOW] +
      hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F);



   dx = betaHudUnits;
   dy = -alphaHudUnits;

   // Well, really it is centered on the flight path marker*/	
	 
   display->AdjustOriginInViewport (dx, dy);

   // Rotate about nose marker
// display->AdjustRotationAboutOrigin(-cockpitFlightData.roll);
   display->AdjustRotationAboutOrigin(-ownship->GetMu());

   /*-------------------------*/
   /* elevation in tenths      */
   /*-------------------------*/
   a = FloatToInt32( cockpitFlightData.gamma * 10.0F * RTD );
   float offset = cockpitFlightData.gamma * 10.0F * RTD - a;
   i = a%50;

   //MI Smaller pitchladder
   if(!g_bRealisticAvionics)
   {
	   vert[0][0] = hudWinWidth[PITCH_LADDER_WINDOW] * 0.25F;
	   vert[1][0] = hudWinWidth[PITCH_LADDER_WINDOW] * 0.35F;
	   vert[2][0] = hudWinWidth[PITCH_LADDER_WINDOW] * 0.45F;
	   vert[3][0] = hudWinWidth[PITCH_LADDER_WINDOW] * 0.55F;
	   vert[4][0] = hudWinWidth[PITCH_LADDER_WINDOW] * 0.65F;
	   vert[5][0] = hudWinWidth[PITCH_LADDER_WINDOW] * 0.75F;
	   vert[6][0] = vert[5][0];
   }
   else
   {
	   vert[0][0] = (hudWinWidth[PITCH_LADDER_WINDOW] - PitchLadderDiff) * 0.25F;
	   vert[1][0] = (hudWinWidth[PITCH_LADDER_WINDOW] - PitchLadderDiff) * 0.35F;
	   vert[2][0] = (hudWinWidth[PITCH_LADDER_WINDOW] - PitchLadderDiff) * 0.45F;
	   vert[3][0] = (hudWinWidth[PITCH_LADDER_WINDOW] - PitchLadderDiff) * 0.55F;
	   vert[4][0] = (hudWinWidth[PITCH_LADDER_WINDOW] - PitchLadderDiff) * 0.65F;
	   vert[5][0] = (hudWinWidth[PITCH_LADDER_WINDOW] - PitchLadderDiff) * 0.75F;
	   vert[6][0] = vert[5][0];
   }

   vert[7][0] = -vert[0][0];
   vert[8][0] = -vert[1][0];
   vert[9][0] = -vert[2][0];
   vert[10][0] = -vert[3][0];
   vert[11][0] = -vert[4][0];
   vert[12][0] = -vert[5][0];
   vert[13][0] = -vert[5][0];

   //MI Smaller pitchladder
   if(!g_bRealisticAvionics)
   {
	   vert[14][0] = hudWinWidth[PITCH_LADDER_WINDOW] * 1.00F;
	   vert[15][0] = -vert[14][0];
	   vert[16][0] = hudWinWidth[PITCH_LADDER_WINDOW] * 2.00F;
	   vert[17][0] = -vert[16][0];
   }
   else
   { 
	   vert[14][0] = (hudWinWidth[PITCH_LADDER_WINDOW] - PitchLadderDiff) * 1.00F;
	   vert[15][0] = -vert[14][0];
	   vert[16][0] = (hudWinWidth[PITCH_LADDER_WINDOW] - PitchLadderDiff) * 2.00F;
	   vert[17][0] = -vert[16][0];
   }

   if(!g_bNewPitchLadder)
	   vert[0][1] = -(0.1F * i + 20.0F) * degreesForScreen;
   else
	   vert[0][1] = -(0.1F * (i + offset) + 20.0F) * degreesForScreen;

   a = (a-i)/10 - 20;         /* starting number   */

   // JPO - draw the 2.5 degree line when gear is down and locked.
   if (((AircraftClass*)ownship)->af->gearPos > 0.5F && 
       a < -2 && a + 50 > -2) { // JPO we surround the -2.5 line
       float delta = (float)a - -2.5f;
       vert[0][1] = vert[0][1] - delta * degreesForScreen; // adjust

       vert[1][1] = vert[0][1];
       vert[2][1] = vert[0][1];
       vert[3][1] = vert[0][1];
       vert[4][1] = vert[0][1];
       vert[5][1] = vert[0][1];
       vert[7][1] = vert[0][1];
       vert[8][1] = vert[0][1];
       vert[9][1] = vert[0][1];
       vert[10][1] = vert[0][1];
       vert[11][1] = vert[0][1];
       vert[12][1] = vert[0][1];
       vert[14][1] = vert[0][1];
       vert[15][1] = vert[0][1];
       
       display->Line(vert[0][0], vert[0][1], vert[1][0], vert[1][1]);
       display->Line(vert[2][0], vert[2][1], vert[3][0], vert[3][1]);
       display->Line(vert[4][0], vert[4][1], vert[5][0], vert[5][1]);
       display->Line(vert[7][0], vert[7][1], vert[8][0], vert[8][1]);
       display->Line(vert[9][0], vert[9][1], vert[10][0], vert[10][1]);
       display->Line(vert[11][0], vert[11][1], vert[12][0], vert[12][1]);

       // reset it
       vert[0][1] = -(0.1F * i + 20.0F) * degreesForScreen;
   }

   //for (i=0; i<10; i++)
   for (i=0; i<30; i++)
   {
      if (a >= -90 && a <= 90)
      {
		  //MI no - is drawn on the real pitchladder
		  if(!g_bRealisticAvionics)
			  sprintf (tmpStr , "%d", a);
		  else
		  {
			  if(a > 0)
				  sprintf (tmpStr , "%d", a);
			  else
				  sprintf(tmpStr, "%d", -a);
		  }
         ShiAssert (strlen(tmpStr) < 12);

		 if ( a > 0 ) {
			 vert[5][1] = vert[0][1];
			 vert[6][1] = vert[0][1] - hudWinWidth[PITCH_LADDER_WINDOW] * 0.1F;
			 vert[7][1] = vert[0][1];
			 vert[12][1] = vert[0][1];
			 vert[13][1] = vert[6][1];
			 vert[14][1] = vert[0][1];
			 vert[15][1] = vert[0][1];
			 
			 display->Line(vert[0][0], vert[0][1], vert[5][0], vert[5][1]);
			 display->Line(vert[5][0], vert[5][1], vert[6][0], vert[6][1]);
			 display->Line(vert[7][0], vert[7][1], vert[12][0], vert[12][1]);
			 display->Line(vert[12][0], vert[12][1], vert[13][0], vert[13][1]);
			 display->TextCenterVertical (vert[14][0], vert[14][1], tmpStr);
			 display->TextCenterVertical (vert[15][0], vert[15][1], tmpStr);
		 } else if ( a < 0 ) {
			 vert[1][1] = vert[0][1];
			 vert[2][1] = vert[0][1];
			 vert[3][1] = vert[0][1];
			 vert[4][1] = vert[0][1];
			 vert[5][1] = vert[0][1];
			 vert[6][1] = vert[0][1] + hudWinWidth[PITCH_LADDER_WINDOW] * 0.1F;
			 vert[7][1] = vert[0][1];
			 vert[8][1] = vert[0][1];
			 vert[9][1] = vert[0][1];
			 vert[10][1] = vert[0][1];
			 vert[11][1] = vert[0][1];
			 vert[12][1] = vert[0][1];
			 vert[13][1] = vert[6][1];
			 vert[14][1] = vert[0][1];
			 vert[15][1] = vert[0][1];
			 
			 display->Line(vert[0][0], vert[0][1], vert[1][0], vert[1][1]);
			 display->Line(vert[2][0], vert[2][1], vert[3][0], vert[3][1]);
			 display->Line(vert[4][0], vert[4][1], vert[5][0], vert[5][1]);
			 display->Line(vert[5][0], vert[5][1], vert[6][0], vert[6][1]);
			 display->Line(vert[7][0], vert[7][1], vert[8][0], vert[8][1]);
			 display->Line(vert[9][0], vert[9][1], vert[10][0], vert[10][1]);
			 display->Line(vert[11][0], vert[11][1], vert[12][0], vert[12][1]);
			 display->Line(vert[12][0], vert[12][1], vert[13][0], vert[13][1]);
			 display->TextCenterVertical (vert[14][0], vert[14][1], tmpStr);
			 display->TextCenterVertical (vert[15][0], vert[15][1], tmpStr);
		 } else {
			 ShiAssert( a == 0 );
			 vert[7][1] = vert[0][1];
			 vert[16][1] = vert[0][1];
			 vert[17][1] = vert[0][1];
			 display->Line(vert[0][0], vert[0][1], vert[16][0], vert[16][1]);
			 display->Line(vert[7][0], vert[7][1], vert[17][0], vert[17][1]);
		 }
      }

      a += 5;

      vert[0][1] +=  5.0F * degreesForScreen;
   }
        
   display->ZeroRotationAboutOrigin();
   display->AdjustOriginInViewport (-dx, -dy);

   display->AdjustOriginInViewport (0.0F, -(hudWinY[BORESIGHT_CROSS_WINDOW] +
      hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
}

float HudClass::MRToHudUnits (float mr)
{
float retval;

   if(mr * 0.001F > 2.0F * halfAngle * DTR)
   {
      retval = 2.0F;
   }
   else
   {
      retval = (float)(tan(mr * 0.001F) / tan (halfAngle * DTR));
   }
   return (retval);
}

float HudClass::RadToHudUnits (float rad)
{
	float retval;

	if(rad > 2.0F * halfAngle * DTR)
	{
		//retval = 2.0F;
		retval = (float)(rad/(halfAngle * DTR));
	}
	else
	{
		retval = (float)(tan(rad) / tan (halfAngle * DTR));
	}
	return retval;
}

float HudClass::HudUnitsToRad (float hudUnits)
{
float retval;

   retval = (float)atan (hudUnits * tan (halfAngle * DTR));
   return (retval);
}

void HudClass::GetBoresightPos (float* xPos, float* yPos)
{
   *xPos = hudWinX[BORESIGHT_CROSS_WINDOW] + hudWinWidth[BORESIGHT_CROSS_WINDOW] * 0.5F;
   *yPos = hudWinY[BORESIGHT_CROSS_WINDOW] + hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F;
}

void HudClass::SetHalfAngle (float newAngle)
{
   halfAngle = newAngle;
   degreesForScreen = 1.0F / halfAngle;
}

void HudClass::PushButton(int whichButton, int whichMFD)
{
   switch (whichButton)
   {
      case 0:
         MfdDisplay[whichMFD]->SetNewMode(MFDClass::MfdMenu);
      break;

      case 14:
         MFDSwapDisplays();
      break;
   }
}

HudDataType::HudDataType (void)
{
	tgtAz = tgtEl = tgtAta = tgtDroll = 0.0F;
	tgtId = -1;
	flags = 0;
	}

DWORD HudClass::GetHudColor(void)
{
	return curHudColor;
}

void HudClass::SetHudColor(DWORD newColor)
{
	curHudColor = newColor;
	//MI fixup. If I don't update the curColorIdx as well, what good does it do????
	curColorIdx = 0;
	while(newColor != hudColor[curColorIdx])
	{
		curColorIdx++;
		if(curColorIdx > NumHudColors)
		{
			curColorIdx = 0;
			break;
		}
	}
}

void HudClass::HudColorStep(void)
{
   curColorIdx ++;
   curColorIdx %= NumHudColors;

	SetLightLevel();
}

void HudClass::CalculateBrightness(float percent, DWORD* color)
{
	DWORD	red;
	DWORD	green;
	DWORD	blue;

	red	= *color & 0xff;
	green = (*color & 0xff00) >> 8;
	blue	= (*color & 0xff0000) >> 16;

	red	= FloatToInt32((float)red * percent);
	green	= FloatToInt32((float)green * percent);
	blue	= FloatToInt32((float)blue * percent);

	green = green << 8;
	blue	= blue << 16;

	*color = 0xff000000;
	*color = *color | red | green | blue;
}


void HudClass::SetLightLevel(void)
{	
	curHudColor = hudColor[curColorIdx];

	if(!g_bRealisticAvionics)
	{
		if(brightnessSwitch == BRIGHT_AUTO) {
			CalculateBrightness(0.8F, &curHudColor);
		} 
		else if(brightnessSwitch == NIGHT) {
			CalculateBrightness(0.6F, &curHudColor);
		}
	}
	else
	{
		if(brightnessSwitch == BRIGHT_AUTO) {
			CalculateBrightness(0.8F * (SymWheelPos), &curHudColor);
		} 
		else if(brightnessSwitch == NIGHT) {
			CalculateBrightness(0.6F * (SymWheelPos), &curHudColor);
		}
		else if(brightnessSwitch == DAY) {
			CalculateBrightness(1.0F * (SymWheelPos), &curHudColor);
		}
	}
}


VuEntity* HudClass::CanSeeTarget (int type, VuEntity* entity, FalconEntity* platform)
{
	//MI camera fix
#if 0
Tpoint point;
ThreeDVertex pixel;
VuEntity* retval = NULL;
WeaponClassDataType *wc;
float top, left, bottom, right;
float dx, dy, dz;
float maxRangeSqrd;
float curRangeSqrd;

   wc = (WeaponClassDataType*)Falcon4ClassTable[type-VU_LAST_ENTITY_TYPE].dataPtr;

   maxRangeSqrd = (wc->Range * KM_TO_FT) * (wc->Range * KM_TO_FT);

   point.x = entity->XPos();
   point.y = entity->YPos();
   point.z = OTWDriver.GetGroundLevel (point.x, point.y);

   dx = point.x - platform->XPos();
   dy = point.y - platform->YPos();
   dz = point.z - platform->ZPos();

   curRangeSqrd = dx*dx + dy*dy + dz*dz;

   if (curRangeSqrd < maxRangeSqrd)
   {
      // Check LOS
      if (OTWDriver.CheckLOS( platform, (FalconEntity*)entity))
      {
         // For ownship, check if in the circle
         if (platform == ownship)
         {
            OTWDriver.renderer->TransformPoint(&point, &pixel );

            if (pixel.clipFlag == ON_SCREEN)
            {
               // It's in front, but is it in the circle?
               bottom = pixelYCenter + 6.0F*sightRadius;
               top    = pixelYCenter - 6.0F*sightRadius;
               left   = pixelXCenter - 6.0F*sightRadius;
               right  = pixelXCenter + 6.0F*sightRadius;

               if (pixel.x > left && pixel.x < right && pixel.y < bottom && pixel.y > top)
               {
                  retval = entity;
               }
            }
         }
         else // In range and good LOS enough for AI
         {
            retval = entity;
         }
      }
   }

   return retval;
#else
   Tpoint point;
Trotation RR, RR2;
float cosP,sinP,cosY,sinY, sinR, cosR = 0.0F;
float el, az = 0.0F;
VuEntity* retval = NULL;
WeaponClassDataType *wc;
float dx, dy, dz;
float maxRangeSqrd;
float curRangeSqrd;
float CameraHalfFOV = 1.4F * DTR;
float offset = -8.0F * DTR;

        wc = 
(WeaponClassDataType*)Falcon4ClassTable[type-VU_LAST_ENTITY_TYPE].dataPtr;

        maxRangeSqrd = (wc->Range * KM_TO_FT) * (wc->Range * KM_TO_FT);

        point.x = entity->XPos();
        point.y = entity->YPos();
        point.z = OTWDriver.GetGroundLevel (point.x, point.y);

        dx = point.x - platform->XPos();
        dy = point.y - platform->YPos();
        dz = point.z - platform->ZPos();
        point.x = dx;
        point.y = dy;
        point.z = dz;
        curRangeSqrd = dx*dx + dy*dy + dz*dz;

        if (curRangeSqrd < maxRangeSqrd)
        {
                // Check LOS
                if (OTWDriver.CheckLOS( platform, (FalconEntity*)entity))
                {
					// For ownship, check if within camera FOV
					if (platform == ownship)
					{
						//dpc - Rotation Matrix should be done once per every camera shot, not for 
						//each entity, ah well...
						//But it doesn't look like it's hogging down FPS so it remains here...
						//I should probably calculate one matrix that does all three - Yaw, Pitch 
						//and Roll but this works
						//and I don't want to mess it up...End effect is the same.

						cosP = (float)cos (- platform->Pitch());
						sinP = (float)sin (- platform->Pitch());
						cosY = (float)cos (- platform->Yaw());
						sinY = (float)sin (- platform->Yaw());
						cosR = (float)cos (- platform->Roll());
						sinR = (float)sin (- platform->Roll());

						RR.M11 = cosY;
						RR.M21 = sinY;
						RR.M31 = 0;

						RR.M12 = -sinY;
						RR.M22 = cosY;
						RR.M32 = 0;

						RR.M13 = 0;
						RR.M23 = 0;
						RR.M33 = 1;
						//First rotation is only doing (-Yaw)
						point.x = dx * RR.M11 + dy * RR.M12 + dz * RR.M13;
						point.y = dx * RR.M21 + dy * RR.M22 + dz * RR.M23;
						point.z = dx * RR.M31 + dy * RR.M32 + dz * RR.M33;
						dx = point.x; dy = point.y; dz = point.z;

						cosY = (float)cos (0);
						sinY = (float)sin (0);
						cosR = (float)cos (0);
						sinR = (float)sin (0);

						RR2.M11 = cosP;
						RR2.M21 = 0;
						RR2.M31 = -sinP;

						RR2.M12 = 0;
						RR2.M22 = 1;
						RR2.M32 = 0;

						RR2.M13 = sinP;
						RR2.M23 = 0;
						RR2.M33 = cosP;
						//Second rotation is doing (-Pitch)
						point.x = dx * RR2.M11 + dy * RR2.M12 + dz * RR2.M13;
						point.y = dx * RR2.M21 + dy * RR2.M22 + dz * RR2.M23;
						point.z = dx * RR2.M31 + dy * RR2.M32 + dz * RR2.M33;
						dx = point.x; dy = point.y; dz = point.z;

						if (point.x > 0.0F)     //check to see if entity is in front of us (maybe not needed but anyway...)
						{
							//One last rotation: (-Roll) this time
							point.y = dy * cosR - dz * sinR;
							point.z = dy * sinR + dz * cosR;

							el = (float)atan2(-point.z, (float)sqrt(point.x*point.x+point.y*point.y));
							az = (float)atan2(point.y,point.x);
							if (fabs(el-g_fReconCameraOffset * DTR) < g_fReconCameraHalfFOV * DTR && fabs(az) < g_fReconCameraHalfFOV * DTR)
							{
								retval = entity;
							}
						}
					}
					else // In range and good LOS enough for AI
					{
						retval = entity;
					}
                }
        }
        return retval;
#endif
}
void HudClass::DrawDTOSSBox(void)
{
	if (targetData)
	{
		display->AdjustOriginInViewport (0.0F, (hudWinY[BORESIGHT_CROSS_WINDOW] +
			hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
		
		DrawTDMarker(targetData->az, targetData->el, targetData->droll, 0.03F );
		
		display->AdjustOriginInViewport (0.0F, -(hudWinY[BORESIGHT_CROSS_WINDOW] +
			hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
	}
}
void HudClass::DrawTDMarker(float az, float el, float dRoll, float size)
{
float xPos, yPos;
char tmpStr[12];
mlTrig trig;
float offset = MRToHudUnits(45.0F);
//MI
RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);


	xPos = RadToHudUnits(az);
	yPos = RadToHudUnits(el);

   if (fabs (az) < 90.0F * DTR && fabs (el) < 90.0F * DTR && fabs (xPos) < 0.90F && 
	   fabs(yPos + hudWinY[BORESIGHT_CROSS_WINDOW] + hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F) < 0.90F)
   {
	   display->AdjustOriginInViewport(xPos, yPos);
	   //MI
	   if(g_bRealisticAvionics)
	   {
		   if(SimDriver.playerEntity && SimDriver.playerEntity->Sms && SimDriver.playerEntity->Sms
			   ->curWeaponType == wtAgm65 && SimDriver.playerEntity->Sms->curWeapon)
		   {
			   if(SimDriver.playerEntity->Sms->MavSubMode == SMSBaseClass::VIS)
			   {
				   if(FCC->preDesignate)
				   {				 
					   display->Line (-size, -size, -size, size);	
					   display->Line (-size, -size, size, -size);
					   display->Line ( size, size, -size, size);
					   display->Line ( size, size, size, -size);
				   }
				   else
					   display->Circle(0.0F, 0.0F, size);
			   }
			   else
			   {
				   display->Line (-size, -size, -size, size);	
				   display->Line (-size, -size, size, -size);
				   display->Line ( size, size, -size, size);
				   display->Line ( size, size, size, -size);
			   }
				   
		   }
		   else
		   {
			   display->Line (-size, -size, -size, size);
			   display->Line (-size, -size, size, -size);
			   display->Line ( size, size, -size, size);
			   display->Line ( size, size, size, -size);
			   //MI add a GO STT readout if we're a SARH and not in STT
			   if(ownship && g_bRealisticAvionics)
			   {
				   if(ownship->Sms && ownship->Sms->curWeapon && ownship->Sms->curWeapon->IsMissile() &&
					   ((MissileClass *)ownship->Sms->curWeapon)->GetSeekerType() == SensorClass::RadarHoming &&
					   theRadar && !theRadar->IsSet(RadarDopplerClass::STTingTarget))
				   {
					   display->TextCenter(0.0F, -0.1F, "GO STT");
				   }
			   }
		   }
	   }
	   else
	   {
		   display->Line (-size, -size, -size, size);
		   display->Line (-size, -size, size, -size);
		   display->Line ( size, size, -size, size);
		   display->Line ( size, size, size, -size);
	   }
	   display->AdjustOriginInViewport(-xPos, -yPos);
   }
   else
   {
      mlSinCos (&trig, dRoll);
      xPos = offset * trig.sin;
      yPos = offset * trig.cos;
      display->Line (0.0f, 0.0f, xPos, yPos);
      sprintf (tmpStr, "%.0f", (float)acos (cos (az) * cos (el)) * RTD);
      ShiAssert (strlen(tmpStr) < sizeof(tmpStr));
      display->TextRight(-0.075F, display->TextHeight()*0.5F, tmpStr);
   }
}
void HudClass::DrawAATDBox(void)
{
		if (targetData)
	{
		display->AdjustOriginInViewport (0.0F, (hudWinY[BORESIGHT_CROSS_WINDOW] +
			hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
		
		DrawTDMarker(targetData->az, targetData->el, targetData->droll, 0.07F );
		
		display->AdjustOriginInViewport (0.0F, -(hudWinY[BORESIGHT_CROSS_WINDOW] +
			hudWinHeight[BORESIGHT_CROSS_WINDOW] * 0.5F));
	}
}
