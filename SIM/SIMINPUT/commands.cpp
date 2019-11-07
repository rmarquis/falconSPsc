#include "stdhdr.h"
#include "commands.h"
#include "simdrive.h"
#include "OTWDrive.h"
#include "aircrft.h"
#include "fcc.h"
#include "sms.h"
#include "SmsDraw.h"
#include "airframe.h"
#include "radar.h"
#include "radardoppler.h"
#include "mfd.h"
#include "voicecomunication\voicecom.h"//me123
// OW FIXME: this had to be added after installing the DX8 Beta 1 SDK
#define DIRECTINPUT_VERSION 0x0700
#include "dinput.h"

#include "dispcfg.h"
#include "resource.h"
#include "cpmanager.h"
#include "fsound.h"
#include "falcsnd\psound.h"
#include "soundfx.h"
#include "threadMgr.h"
#include "hud.h"
#include "camp2sim.h"
#include "fack.h"
#include "acmi\src\include\acmirec.h"
#include "cpmisc.h"
#include "navsystem.h"
#include "cphsi.h"
#include "KneeBoard.h"
#include "ui\include\uicomms.h"
#include "missile.h"
#include "mavdisp.h"
#include "laserpod.h"
#include "HarmPod.h"
#include "playerop.h"
#include "alr56.h"
#include "MsgInc\TrackMsg.h"
#include "falcsnd\voicemanager.h" //just for debug :DSP
#include "digi.h"
#include "PilotInputs.h"
#include "lantirn.h"
#include "dofsnswitches.h"

#include "campbase.h"  // Marco for AIM9P

//MI Bullseye
#include "campaign.h"
#include "cmpclass.h"
#include "caution.h"

int ShowFrameRate = 0;
int testFlag = 0;
int narrowFOV = 0;
int keyboardPickleOverride = 0;
int keyboardTriggerOverride = 0;
float throttleOffsetRate = 0.0F;
float throttleOffset = 0.0F;
float rudderOffset = 0.0F;
float rudderOffsetRate = 0.0F;
float pitchStickOffset = 0.0F;
float rollStickOffset = 0.0F;
float pitchStickOffsetRate = 0.0F;
float rollStickOffsetRate = 0.0F;
float pitchRudderTrimRate = 0.0F;
float pitchAileronTrimRate = 0.0f;
float pitchElevatorTrimRate = 0.0f;
float pitchManualTrim = 0.0F;	//MI
float yawManualTrim = 0.0F;		//MI
float rollManualTrim = 0.0F;	//MI
int	UseKeyboardThrottle = FALSE;
extern int ShowVersion;
extern float gSpeedyGonzales;
int gDoOwnshipSmoke = 0;
extern PilotInputs UserStickInputs;

extern CommandsKeyCombo;
extern CommandsKeyComboMod;
extern BOOL WINAPI FistOfGod(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
extern HINSTANCE hInst;
extern void RequestPlayerDivert(void);
extern int radioMenu;
extern SensorClass* FindSensor (SimMoverClass* theObject, int sensorType);
extern SensorClass* FindLaserPod (SimMoverClass* theObject);
static int theFault = 1;
extern int supercruise;

//extern bool g_bHardCoreReal; //me123		MI replaced with g_bRealisticAvionics
//MI for ICP stuff
extern bool g_bRealisticAvionics;
extern bool g_bMLU;
extern bool g_bIFF;
extern bool g_bINS;

extern int g_nMaxSimTimeAcceleration;
extern int g_nShowDebugLabels;
extern int g_nMaxDebugLabel;

#ifdef DEBUG
bool g_bShowTextures = 1;
#endif

// MN for keyboard control stuff
extern float g_fAFRudderRight;
extern float g_fAFRudderLeft;
extern float g_fAFThrottleDown;
extern float g_fAFThrottleUp;
extern float g_fAFAileronLeft;
extern float g_fAFAileronRight;
extern float g_fAFElevatorDown;
extern float g_fAFElevatorUp;
extern float g_frollStickOffset;
extern float g_fpitchStickOffset;
extern float g_frudderOffset;

#ifdef DAVE_DBG
int MoveBoom = FALSE;
#endif

#ifdef _DO_VTUNE_

BOOL VtuneNoop(void) {
	return FALSE;
}

int doVtune = FALSE;
BOOL(*pauseFn)(void) = VtuneNoop;
BOOL(*resumeFn)(void) = VtuneNoop;
HINSTANCE hlib = 0;

int lTestFlag1 = 0;
int lTestFlag2 = 0;

void ToggleVtune(unsigned long, int state, void*)
{
	if(!hlib)
	{
		hlib = LoadLibrary( "vtuneapi.dll" );
		ShiAssert( hlib );

		pauseFn  = (int (__cdecl *)(void))GetProcAddress( hlib, "VtPauseSampling" );
		if (!pauseFn)
			pauseFn = VtuneNoop;
		resumeFn = (int (__cdecl *)(void))GetProcAddress( hlib, "VtResumeSampling" );
		if (!resumeFn)
			resumeFn = VtuneNoop;
	}

	if (state & KEY_DOWN)
	{
		if(doVtune)
		{
			doVtune = FALSE;
			pauseFn();
			MonoPrint( "VTUNE PAUSED\n" );
		}
		else
		{
			MonoPrint( "VTUNE RECORDING\n" );
			doVtune = TRUE;
			resumeFn();
		}
	}
}
#endif	// _DO_VTUNE_


void KneeboardTogglePage(unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.playerEntity)->ejectTriggered)
	{
		KneeBoard *board = OTWDriver.pCockpitManager->mpKneeBoard;
		ShiAssert( board );

		if (board->GetPage() == KneeBoard::MAP) {
			board->SetPage( KneeBoard::BRIEF );
		}
		else if (board->GetPage() == KneeBoard::BRIEF) {
		    board->SetPage(KneeBoard::STEERPOINT);
		} else {
			board->SetPage( KneeBoard::MAP );
		}
	}
}


void ToggleNVGMode(unsigned long, int state, void*)
{
	if (state & KEY_DOWN)
		OTWDriver.NVGToggle();
}


void SimToggleDropPattern(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
	PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.playerEntity, SensorClass::RWR);

		if(theRwr)
      {
			theRwr->ToggleAutoDrop();
		}
	}
}

void ToggleSmoke(unsigned long, int state, void*)
{
   	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   	{
		// toggle our current smoke state
		gDoOwnshipSmoke ^= 1;

		// edg: I'm not sure if there's a better message to handle this,
		// but I'm using track message
		// HACK HACK HACK!  This should be status info on the aircraft concerned...
		FalconTrackMessage* trackMsg = new FalconTrackMessage(1, SimDriver.playerEntity->Id(), FalconLocalGame );
		ShiAssert( trackMsg );
		if ( gDoOwnshipSmoke )
			trackMsg->dataBlock.trackType = Track_SmokeOn;
		else
			trackMsg->dataBlock.trackType = Track_SmokeOff;
		trackMsg->dataBlock.id = SimDriver.playerEntity->Id();
	
		// Send our track list
		FalconSendMessage (trackMsg, TRUE);
	}
}

extern long gRefreshScoresList;

void OTWToggleScoreDisplay(unsigned long, int state, void*) {
	if (state & KEY_DOWN) {
		unsigned int	flag;

		if (SimDriver.RunningDogfight()) {
			flag = SHOW_DOGFIGHT_SCORES;
		} else if (SimDriver.RunningTactical()) {
			flag = SHOW_TE_SCORES;
		} else {
			return;
		}

		gRefreshScoresList = TRUE;

		if (OTWDriver.GetFrontTextFlags() & flag) {
			OTWDriver.SetFrontTextFlags(OTWDriver.GetFrontTextFlags() & ~flag);
		} else {
			OTWDriver.SetFrontTextFlags(OTWDriver.GetFrontTextFlags() | flag);
		}
	}
}

void OTWToggleSidebar(unsigned long, int state, void*)
{	
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
      OTWDriver.ToggleSidebar();
	}
}

void SimRadarAAModeStep(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);

      if (theRadar)
         theRadar->StepAAmode();
   }
}

void SimRadarAGModeStep(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);

      if (theRadar)
         theRadar->StepAGmode();
   }
}

void SimRadarGainUp(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);

      if (theRadar)
      {
         theRadar->StepAGgain( 1 );
      }
   }
}

void SimRadarGainDown(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);

      if (theRadar)
      {
         theRadar->StepAGgain( -1 );
      }
   }
}

void SimRadarStandby(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);

      if (theRadar)
      {
         theRadar->SetEmitting( !theRadar->IsEmitting() );
      }
   }
}

void SimRadarRangeStepUp(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
	   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);
	   if(theRadar)
		   theRadar->RangeStep( 1 );
   } 		   
   lTestFlag1 = 1 - lTestFlag1;
}

void SimRadarRangeStepDown(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
	   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);
	   
	   if(theRadar)
		   theRadar->RangeStep( -1 );
   }
   lTestFlag2 = 1 - lTestFlag2;
}

void SimRadarNextTarget(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
      RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);
      if (theRadar)
         theRadar->NextTarget();

// M.N. added full realism mode
	  if (PlayerOptions.GetAvionicsType() != ATRealistic && PlayerOptions.GetAvionicsType() != ATRealisticAV) {
		  HarmTargetingPod *theHTS = (HarmTargetingPod*)FindSensor(SimDriver.playerEntity, SensorClass::HTS);
		  if (theHTS)
			  theHTS->NextTarget();
	  }
   }
}

void SimRadarPrevTarget(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
      RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);
      if (theRadar)
         theRadar->PrevTarget();

// M.N. added full realism mode
	  if (PlayerOptions.GetAvionicsType() != ATRealistic && PlayerOptions.GetAvionicsType() != ATRealisticAV) {
		  HarmTargetingPod *theHTS = (HarmTargetingPod*)FindSensor(SimDriver.playerEntity, SensorClass::HTS);
		  if (theHTS)
			  theHTS->PrevTarget();
	  }
   }
}

void SimRadarBarScanChange(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);

      if (theRadar)
         theRadar->StepAAscanHeight();
   }
}

void SimRadarFOVStep(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);

      if (theRadar)
         theRadar->StepAGfov();
   }
}

void SimMaverickFOVStep(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN) && SimDriver.playerEntity->Sms)
   {
      if (SimDriver.playerEntity->Sms->curWeaponType == wtAgm65 && SimDriver.playerEntity->Sms->curWeapon)
      {
      MaverickDisplayClass* mavDisplay =
         (MaverickDisplayClass*)((MissileClass*)SimDriver.playerEntity->Sms->curWeapon)->display;

         mavDisplay->ToggleFOV();
      }
      else if (SimDriver.playerEntity->Sms->curWeaponClass == wcGbuWpn)
      {
      LaserPodClass* laserPod = (LaserPodClass* )FindLaserPod (SimDriver.playerEntity);

         if (laserPod)
         {
            laserPod->ToggleFOV();
         }
      }
   }
}

void SimSOIFOVStep(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN) && SimDriver.playerEntity->Sms)
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);

      if (theRadar && theRadar->IsSOI())
         theRadar->StepAGfov();
      else if (SimDriver.playerEntity->Sms->curWeaponType == wtAgm65 && SimDriver.playerEntity->Sms->curWeapon)
      {
	  ShiAssert(SimDriver.playerEntity->Sms->curWeapon->IsMissile());
      MaverickDisplayClass* mavDisplay =
         (MaverickDisplayClass*)((MissileClass*)SimDriver.playerEntity->Sms->curWeapon)->display;

         if (mavDisplay->IsSOI())
            mavDisplay->ToggleFOV();
      }
      else if (SimDriver.playerEntity->Sms->curWeaponClass == wcGbuWpn)
      {
      LaserPodClass* laserPod = (LaserPodClass* )FindLaserPod (SimDriver.playerEntity);

         if (laserPod)
         {
            laserPod->ToggleFOV();
         }
      }
   }
}

void SimRadarFreeze(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);

      if (theRadar)
         theRadar->ToggleAGfreeze();
   }
}

void SimRadarSnowplow(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);

      if (theRadar)
         theRadar->ToggleAGsnowPlow();
   }
}

void SimRadarCursorZero(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);

      if (theRadar)
         theRadar->ToggleAGcursorZero();
   }
}

void SimRadarAzimuthScanChange(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);

      if (theRadar)
         theRadar->StepAAscanWidth();
   }
}

void SimDesignate(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
   {
      if (state & KEY_DOWN)
      {
		  //MI
		  if(g_bRealisticAvionics)
		  {
			  if(SimDriver.playerEntity->FCC)
			  {
				  if(SimDriver.playerEntity->FCC->IsSOI)
					SimDriver.playerEntity->FCC->HSDDesignate = 1;
				  else
					  SimDriver.playerEntity->FCC->designateCmd = TRUE;
			  }
		  }
		  else
			  SimDriver.playerEntity->FCC->designateCmd = TRUE;
      }
      else
      {
		  //MI
		  if(g_bRealisticAvionics)
		  {
			  if(SimDriver.playerEntity->FCC)
			  {
				  SimDriver.playerEntity->FCC->HSDDesignate = 0;
				  SimDriver.playerEntity->FCC->designateCmd = FALSE;
			  }
		  }
		  else
			  SimDriver.playerEntity->FCC->designateCmd = FALSE;
      }
   }
}

void SimDropTrack(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
   {
      if ( state & KEY_DOWN)
	  {
		  //MI
		  if(g_bRealisticAvionics)
		  {
			  if(SimDriver.playerEntity->FCC)
			  {
				  if(SimDriver.playerEntity->FCC->IsSOI)
					SimDriver.playerEntity->FCC->HSDDesignate = -1;
				  else
					  SimDriver.playerEntity->FCC->dropTrackCmd = TRUE;
			  }
		  }
		  else
			  SimDriver.playerEntity->FCC->dropTrackCmd = TRUE;
	  }
      else
	  {
		  //MI
		  if(g_bRealisticAvionics)
		  {
			  if(SimDriver.playerEntity->FCC)
			  {
				  SimDriver.playerEntity->FCC->HSDDesignate = 0;
				  SimDriver.playerEntity->FCC->dropTrackCmd = FALSE;
			  }
		  }
		  else
			  SimDriver.playerEntity->FCC->dropTrackCmd = FALSE;
	  }
   }
}

void SimACMBoresight(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
      RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);
      if (theRadar)
         theRadar->SelectACMBore();

// M.N. added full realism mode
	  if (PlayerOptions.GetAvionicsType() != ATRealistic && PlayerOptions.GetAvionicsType() != ATRealisticAV) {
		  HarmTargetingPod *theHTS = (HarmTargetingPod*)FindSensor(SimDriver.playerEntity, SensorClass::HTS);
		  if (theHTS)
			  theHTS->BoresightTarget();
	  }
   }
}

void SimACMVertical(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);

      if (theRadar && theRadar->IsSOI())
         theRadar->SelectACMVertical();
   }
}

void SimACMSlew(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);

      if (theRadar && theRadar->IsSOI())
         theRadar->SelectACMSlew();
   }
}

void SimACM30x20 (unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);

      if (theRadar && theRadar->IsSOI())
         theRadar->SelectACM30x20();
   }
}

void SimRadarElevationDown(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);
      if (theRadar)
      {
// 2001-02-21 MODIFIED BY S.G. MOVES TOO MUCH
		//theRadar->StepAAelvation( -8 );
		theRadar->StepAAelvation( -4 );
      }
   }
}

void SimRadarElevationUp(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);
      if (theRadar)
      {
// 2001-02-21 MODIFIED BY S.G. MOVES TOO MUCH
//		theRadar->StepAAelvation( 8 );
		theRadar->StepAAelvation( 4 );
      }
   }
}

void SimRadarElevationCenter(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   RadarClass* theRadar = (RadarClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);
      if (theRadar)
      {
		theRadar->StepAAelvation( 0 );
      }
   }
}

// RWR Stuff (ALR56)

void SimRWRSetPriority (unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.playerEntity, SensorClass::RWR);

      if (theRwr)
      {
         theRwr->TogglePriority ();
      }
   }
}

//void SimRWRSetSound (unsigned long, int state, void*)
//{
//   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
//   {
//   PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.playerEntity, SensorClass::RWR);
//
//      if (theRwr)
//      {
//         theRwr->SetSound (1 - theRwr->PlaySound());
//      }
//   }
//}

void SimRWRSetTargetSep (unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.playerEntity, SensorClass::RWR);

      if (theRwr)
      {
         theRwr->ToggleTargetSep ();
      }
   }
}

void SimRWRSetUnknowns (unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.playerEntity, SensorClass::RWR);

      if (theRwr)
      {
         theRwr->ToggleUnknowns ();
      }
   }
}

void SimRWRSetNaval (unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.playerEntity, SensorClass::RWR);

      if (theRwr)
      {
         theRwr->ToggleNaval ();
      }
   }
}

void SimRWRSetGroundPriority (unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.playerEntity, SensorClass::RWR);

      if (theRwr)
      {
         theRwr->ToggleLowAltPriority();
      }
   }
}

void SimRWRSetSearch (unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.playerEntity, SensorClass::RWR);

      if (theRwr)
      {
         theRwr->ToggleSearch();
      }
   }
}

void SimRWRHandoff (unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
   PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.playerEntity, SensorClass::RWR);

      if (theRwr)
      {
         theRwr->SelectNextEmitter();

// JB 010727 RP5 RWR
// 2001-02-15 ADDED BY S.G. SO WE HEAR THE SOUND ***RIGHT AWAY*** I WON'T PASS A targetList (although I could) SO NOT ALL OF THE ROUTINE IS DONE
//            IN 1.08i2, DoAudio PROCESSES THE WHOLE CONTACT LIST BY ITSELF AND NOT JUST THE PASSED CONTACT. SINCE 1.07 DOESN'T I'M STUCK AT DOING THIS :-(
//            THIS WON'T BE FPS INTENSIVE ANYWAY SINCE IT ONLY RUNS WHEN THE HANDOFF BUTTON IS PRESSED
//            LATER ON, I MIGHT MAKE THIS CODE 1.08i2 'COMPATIBLE'
				 theRwr->Exec(NULL);
      }
   }
}

void SimPrevWaypoint(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
      SimDriver.playerEntity->FCC->waypointStepCmd = -1;
   }
}

void SimNextWaypoint(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
      SimDriver.playerEntity->FCC->waypointStepCmd = 1;
   }
}

void SimTogglePaused(unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      SimDriver.TogglePause();
}

void SimSpeedyGonzalesUp(unsigned long, int state, void*)
{
	if ((state & KEY_DOWN) && (!gCommsMgr->Online()))
	{
		gSpeedyGonzales *= 1.25;
	}
	
	if (gSpeedyGonzales >= 32.0)
	{
		gSpeedyGonzales = 32.0;
	}
	
	MonoPrint ("Speedy Up %f\n", gSpeedyGonzales);
}

void SimSpeedyGonzalesDown(unsigned long, int state, void*)
{
	if ((state & KEY_DOWN) && (!gCommsMgr->Online()))
	{
		gSpeedyGonzales /= 1.25;
	}
	
	if (gSpeedyGonzales < 1.0)
	{
		gSpeedyGonzales = 1.0;
	}

	MonoPrint ("Speedy Down %f\n", gSpeedyGonzales);
}

void SimPickle(unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
   {
      keyboardPickleOverride = TRUE;
	  //MI
	  if(g_bRealisticAvionics)
	  {
		  if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && SimDriver.playerEntity->AVTRState(AircraftClass::AVTRStateFlags::AVTR_AUTO)) // 2002-02-15 MODIFIED BY S.G. Check if MOTION_OWNSHIP before going in otherwise it might CTD just after eject
		  { 
			  if(SimDriver.playerEntity->Sms && SimDriver.playerEntity->Sms->MasterArm() ==
				SMSBaseClass::Safe)
				return;

			  if(SimDriver.AVTROn() == FALSE)
			  {
				  SimDriver.SetAVTR(TRUE);
				  SimDriver.playerEntity->AddAVTRSeconds();
				  ACMIToggleRecording(0, state, NULL);
			  }
			  else
				  SimDriver.playerEntity->AddAVTRSeconds();
		  }
		  
		  //Targeting Pod, Fire laser automatically
		  if(SimDriver.playerEntity && SimDriver.playerEntity->FCC && SimDriver.playerEntity->FCC->LaserArm && 
			  SimDriver.playerEntity->FCC->GetMasterMode() == FireControlComputer::AirGroundLaser)
		  {
			  SimDriver.playerEntity->FCC->CheckForLaserFire = TRUE;
			  SimDriver.playerEntity->FCC->LaserWasFired = FALSE;
		  }
	  }
   }
   else
   {
      keyboardPickleOverride = FALSE;
   }
}

void SimTrigger(unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
   {
	   keyboardTriggerOverride = TRUE;
	   if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && SimDriver.playerEntity->AVTRState(AircraftClass::AVTRStateFlags::AVTR_AUTO)) // 2002-02-15 ADDED BY S.G. Check if MOTION_OWNSHIP before going in otherwise it might CTD just after eject
	   { 
		   if(SimDriver.playerEntity->Sms && SimDriver.playerEntity->Sms->MasterArm() ==
			   SMSBaseClass::Safe)
			   return;

		   if(SimDriver.AVTROn() == FALSE)
		   {
			   SimDriver.SetAVTR(TRUE);
			   SimDriver.playerEntity->AddAVTRSeconds();
			   ACMIToggleRecording(0, state, NULL);
		   }
		   else
			   SimDriver.playerEntity->AddAVTRSeconds();
	   }
   } 
   else
   {
      keyboardTriggerOverride = FALSE;
   }
}

void SimMissileStep(unsigned long, int state, void*)//me123 addet nosewheel stearing
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
	 if (!SimDriver.playerEntity->af->IsSet(AirframeClass::InAir) )
	 {
		//if (g_bHardCoreReal)	MI
		 if(g_bRealisticAvionics)
		{
			if (!SimDriver.playerEntity->af->IsSet(AirframeClass::NoseSteerOn))	
				SimDriver.playerEntity->af->SetFlag(AirframeClass::NoseSteerOn);

			else SimDriver.playerEntity->af->ClearFlag(AirframeClass::NoseSteerOn);
		}
	 }

	else
	   SimDriver.playerEntity->FCC->WeaponStep();
   }
}

void SimCursorUp(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
   {
	   RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);
	   FireControlComputer* pFCC = SimDriver.playerEntity->GetFCC();
      if (state & KEY_DOWN)
      {
		  //MI
		  if(g_bRealisticAvionics)
		  {
			  LaserPodClass* laserPod = (LaserPodClass* )FindLaserPod (SimDriver.playerEntity);
			  MaverickDisplayClass* mavDisplay = NULL;
			  HarmTargetingPod *theHTS = (HarmTargetingPod*)FindSensor(SimDriver.playerEntity, SensorClass::HTS);
			  if(SimDriver.playerEntity->Sms->curWeaponType == wtAgm65 && SimDriver.playerEntity->Sms->curWeapon)
			  {
				  ShiAssert(SimDriver.playerEntity->Sms->curWeapon->IsMissile());
				  mavDisplay = (MaverickDisplayClass*)((MissileClass*)SimDriver.playerEntity->Sms->curWeapon)->display;
			  }
			  //ACM Modes get's us directly into ACM Slew
			  if(theRadar && (theRadar->GetRadarMode() == RadarClass::ACM_30x20 ||
				  theRadar->GetRadarMode() == RadarClass::ACM_BORE ||
				  theRadar->GetRadarMode() == RadarClass::ACM_10x60))
			  {
				  //if we have a lock, break it
				  SimDriver.playerEntity->FCC->dropTrackCmd = TRUE;
				  theRadar->SelectACMSlew();
			  }
			  else if((theRadar && theRadar->IsSOI()) || (mavDisplay && mavDisplay->IsSOI()) || 
				  (laserPod && laserPod->IsSOI()))
				  SimDriver.playerEntity->FCC->cursorYCmd = 1;
			  else if(pFCC && pFCC->IsSOI)
				  SimDriver.playerEntity->FCC->HSDCursorYCmd = 1;
			  else if(theHTS && SimDriver.playerEntity->GetSOI() == SimVehicleClass::SOI_WEAPON)
				  SimDriver.playerEntity->FCC->cursorYCmd = 1;
			  else if(TheHud && TheHud->IsSOI())
				  SimDriver.playerEntity->FCC->cursorYCmd = 1;
		  }
		  else
			  SimDriver.playerEntity->FCC->cursorYCmd = 1;
      }
      else
      {
		  if(g_bRealisticAvionics)
			  SimDriver.playerEntity->FCC->dropTrackCmd = FALSE;
         SimDriver.playerEntity->FCC->cursorYCmd = 0;
		 SimDriver.playerEntity->FCC->HSDCursorYCmd = 0;
		 
      }
   }
}

void SimCursorDown(unsigned long, int state, void*)
{
   //if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) &&
		!F4IsBadReadPtr(SimDriver.playerEntity->FCC, sizeof(FireControlComputer))) // JB 010408 CTD
   {
	   RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);
	   FireControlComputer* pFCC = SimDriver.playerEntity->GetFCC();
	  if (state & KEY_DOWN)
      {
		  //MI
		  if(g_bRealisticAvionics)
		  {
			  LaserPodClass* laserPod = (LaserPodClass* )FindLaserPod (SimDriver.playerEntity);
			  MaverickDisplayClass* mavDisplay = NULL;
			  HarmTargetingPod *theHTS = (HarmTargetingPod*)FindSensor(SimDriver.playerEntity, SensorClass::HTS);
			  if(SimDriver.playerEntity->Sms->curWeaponType == wtAgm65 && SimDriver.playerEntity->Sms->curWeapon)
			  {
				  ShiAssert(SimDriver.playerEntity->Sms->curWeapon->IsMissile());
				  mavDisplay = (MaverickDisplayClass*)((MissileClass*)SimDriver.playerEntity->Sms->curWeapon)->display;
			  }
			  //ACM Modes get's us directly into ACM Slew
			  if(theRadar && (theRadar->GetRadarMode() == RadarClass::ACM_30x20 ||
				  theRadar->GetRadarMode() == RadarClass::ACM_BORE ||
				  theRadar->GetRadarMode() == RadarClass::ACM_10x60))
			  {
				  //if we have a lock, break it
				  SimDriver.playerEntity->FCC->dropTrackCmd = TRUE;
				  theRadar->SelectACMSlew();
			  }
			  else if((theRadar && theRadar->IsSOI()) || (mavDisplay && mavDisplay->IsSOI()) || 
				  (laserPod && laserPod->IsSOI()))
				  SimDriver.playerEntity->FCC->cursorYCmd = -1;
			  else if(pFCC && pFCC->IsSOI)
				  SimDriver.playerEntity->FCC->HSDCursorYCmd = -1;
			  else if(theHTS && SimDriver.playerEntity->GetSOI() == SimVehicleClass::SOI_WEAPON)
				  SimDriver.playerEntity->FCC->cursorYCmd = -1;
			  else if(TheHud && TheHud->IsSOI())
				  SimDriver.playerEntity->FCC->cursorYCmd = -1;
		  }
		  else
			  SimDriver.playerEntity->FCC->cursorYCmd = -1;
      }
      else
      {
		  if(g_bRealisticAvionics)
			  SimDriver.playerEntity->FCC->dropTrackCmd = FALSE;
         SimDriver.playerEntity->FCC->cursorYCmd = 0;
		 SimDriver.playerEntity->FCC->HSDCursorYCmd = 0;
      }
   }
}

void SimCursorLeft(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
   {
	   RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);
	   FireControlComputer* pFCC = SimDriver.playerEntity->GetFCC();
      if (state & KEY_DOWN)
      {
		  //MI
		  if(g_bRealisticAvionics)
		  {
			  LaserPodClass* laserPod = (LaserPodClass* )FindLaserPod (SimDriver.playerEntity);
			  MaverickDisplayClass* mavDisplay = NULL;
			  HarmTargetingPod *theHTS = (HarmTargetingPod*)FindSensor(SimDriver.playerEntity, SensorClass::HTS);
			  if(SimDriver.playerEntity->Sms->curWeaponType == wtAgm65 && SimDriver.playerEntity->Sms->curWeapon)
			  {
				  ShiAssert(SimDriver.playerEntity->Sms->curWeapon->IsMissile());
				  mavDisplay = (MaverickDisplayClass*)((MissileClass*)SimDriver.playerEntity->Sms->curWeapon)->display;
			  }
			  //ACM Modes get's us directly into ACM Slew
			  if(theRadar && (theRadar->GetRadarMode() == RadarClass::ACM_30x20 ||
				  theRadar->GetRadarMode() == RadarClass::ACM_BORE ||
				  theRadar->GetRadarMode() == RadarClass::ACM_10x60))
			  {
				  //if we have a lock, break it
				  SimDriver.playerEntity->FCC->dropTrackCmd = TRUE;
				  theRadar->SelectACMSlew();
			  }
			  else if((theRadar && theRadar->IsSOI()) || (mavDisplay && mavDisplay->IsSOI()) || 
				  (laserPod && laserPod->IsSOI()))
				  SimDriver.playerEntity->FCC->cursorXCmd = -1;
			  else if(pFCC && pFCC->IsSOI)
				  SimDriver.playerEntity->FCC->HSDCursorXCmd = -1;
			  else if(theHTS && SimDriver.playerEntity->GetSOI() == SimVehicleClass::SOI_WEAPON)
				  SimDriver.playerEntity->FCC->cursorXCmd = -1;
			  else if(TheHud && TheHud->IsSOI())
				  SimDriver.playerEntity->FCC->cursorXCmd = -1;
		  }
		  else
			  SimDriver.playerEntity->FCC->cursorXCmd = -1;
      }
      else
      {
		  if(g_bRealisticAvionics)
			  SimDriver.playerEntity->FCC->dropTrackCmd = FALSE;
         SimDriver.playerEntity->FCC->cursorXCmd = 0;
		 SimDriver.playerEntity->FCC->HSDCursorXCmd = 0;
      }
   }
}

void SimCursorRight(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
   {
	   RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);
	   FireControlComputer* pFCC = SimDriver.playerEntity->GetFCC();
      if (state & KEY_DOWN)
      {
		  //MI
		  if(g_bRealisticAvionics)
		  {
			  LaserPodClass* laserPod = (LaserPodClass* )FindLaserPod (SimDriver.playerEntity);
			  MaverickDisplayClass* mavDisplay = NULL;
			  HarmTargetingPod *theHTS = (HarmTargetingPod*)FindSensor(SimDriver.playerEntity, SensorClass::HTS);
			  if(SimDriver.playerEntity->Sms->curWeaponType == wtAgm65 && SimDriver.playerEntity->Sms->curWeapon)
			  {
				  ShiAssert(SimDriver.playerEntity->Sms->curWeapon->IsMissile());
				  mavDisplay = (MaverickDisplayClass*)((MissileClass*)SimDriver.playerEntity->Sms->curWeapon)->display;
			  }
			  //ACM Modes get's us directly into ACM Slew
			  if(theRadar && (theRadar->GetRadarMode() == RadarClass::ACM_30x20 ||
				  theRadar->GetRadarMode() == RadarClass::ACM_BORE ||
				  theRadar->GetRadarMode() == RadarClass::ACM_10x60))
			  {
				  //if we have a lock, break it
				  SimDriver.playerEntity->FCC->dropTrackCmd = TRUE;
				  theRadar->SelectACMSlew();
			  }
			  else if((theRadar && theRadar->IsSOI()) || (mavDisplay && mavDisplay->IsSOI()) || 
				  (laserPod && laserPod->IsSOI()))
				  SimDriver.playerEntity->FCC->cursorXCmd = 1;
			  else if(pFCC && pFCC->IsSOI)
				  SimDriver.playerEntity->FCC->HSDCursorXCmd = 1;
			  else if(theHTS && SimDriver.playerEntity->GetSOI() == SimVehicleClass::SOI_WEAPON)
				  SimDriver.playerEntity->FCC->cursorXCmd = 1;
			  else if(TheHud && TheHud->IsSOI())
				  SimDriver.playerEntity->FCC->cursorXCmd = 1;
		  }
		  else
			  SimDriver.playerEntity->FCC->cursorXCmd = 1;
      }
      else
      {
		  if(g_bRealisticAvionics)
			  SimDriver.playerEntity->FCC->dropTrackCmd = FALSE;
         SimDriver.playerEntity->FCC->cursorXCmd = 0;
		 SimDriver.playerEntity->FCC->HSDCursorXCmd = 0;
      }
   }
}

void SimToggleAutopilot(unsigned long, int state, void*)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
	switch (PlayerOptions.GetAutopilotMode())
	{
	case APIntelligent: // allowed even in realistic.
	    SimDriver.playerEntity->ToggleAutopilot();
	    break;
	    
	case APEnhanced:
	    if (!SimDriver.playerEntity->OnGround() || 
		SimDriver.playerEntity->AutopilotType() == AircraftClass::CombatAP)
		SimDriver.playerEntity->ToggleAutopilot();
	    break;
	case APNormal: // POGO/JPO - if auto pilot normal, && realistic, this isn't used.
		if ((!SimDriver.playerEntity->OnGround() || SimDriver.playerEntity->AutopilotType() == AircraftClass::CombatAP))
		{
			if (!g_bRealisticAvionics)
				SimDriver.playerEntity->ToggleAutopilot();
			else
				SimRightAPSwitch(0, state, NULL);
		}
    break;
	}
    }
    
}

void SimStepSMSLeft(unsigned long, int state, void*)
{
SMSClass* Sms;

   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
      Sms = SimDriver.playerEntity->Sms;

      if (Sms)
      {
         Sms->drawable->StepDisplayMode();
      }
   }
}

void SimStepSMSRight(unsigned long, int, void*)
{
}

void SimSelectSRMOverride (unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
	   if(SimDriver.playerEntity->FCC->GetMasterMode() == FireControlComputer::Dogfight)
		   return;

	   SimDriver.playerEntity->FCC->SetMasterMode(FireControlComputer::Dogfight);
	   MfdDisplay[1]->SetNewMasterMode(SimDriver.playerEntity->FCC->GetMainMasterMode());
	   if(SimDriver.playerEntity->Sms && SimDriver.playerEntity->Sms->drawable)
		   SimDriver.playerEntity->Sms->drawable->SetDisplayMode(SmsDrawable::Wpn);

	   //MI 02/02/02
	   if(SimDriver.playerEntity->Sms && SimDriver.playerEntity->Sms->GetCoolState() == SMSClass::WARM 
		   && SimDriver.playerEntity->Sms->MasterArm() == SMSClass::Arm)
	   {
		   SimDriver.playerEntity->Sms->SetCoolState(SMSClass::COOLING);
	   }
   }
}

void SimSelectMRMOverride(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
	   if(SimDriver.playerEntity->FCC->GetMasterMode() == FireControlComputer::MissileOverride)
		   return;

	   SimDriver.playerEntity->FCC->SetMasterMode(FireControlComputer::MissileOverride);
	   MfdDisplay[1]->SetNewMasterMode(SimDriver.playerEntity->FCC->GetMainMasterMode());
	   if(SimDriver.playerEntity->Sms && SimDriver.playerEntity->Sms->drawable)
		   SimDriver.playerEntity->Sms->drawable->SetDisplayMode(SmsDrawable::Wpn);

	   //MI 02/02/02
	   if(SimDriver.playerEntity->Sms && SimDriver.playerEntity->Sms->GetCoolState() == SMSClass::WARM 
		   && SimDriver.playerEntity->Sms->MasterArm() == SMSClass::Arm)
	   {
		   SimDriver.playerEntity->Sms->SetCoolState(SMSClass::COOLING);
	   }
   }
}

void SimDeselectOverride(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
	   SimDriver.playerEntity->FCC->ClearOverrideMode();
	   MfdDisplay[1]->SetNewMasterMode(SimDriver.playerEntity->FCC->GetMainMasterMode());
   }
}

// Support for AIM9 Uncage/Cage
void SimToggleMissileCage(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
	   //MI check for MAV Displays
	  if (SimDriver.playerEntity->Sms->curWeaponType == wtAgm65 && SimDriver.playerEntity->Sms->curWeapon &&
		  SimDriver.playerEntity->Sms->Powered)
	  { 
		  ShiAssert(SimDriver.playerEntity->Sms->curWeapon->IsMissile());
		  ((MissileClass*)SimDriver.playerEntity->Sms->curWeapon)->Covered = FALSE;
		  return;
	  }
      SimDriver.playerEntity->FCC->missileCageCmd = TRUE;
   }
}

// Marco Edit - Support for AIM9 Spot/Scan mode(s)
void SimToggleMissileSpotScan(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN) && SimDriver.playerEntity->Sms)
   {
      SimWeaponClass* wpn; 
	  wpn = SimDriver.playerEntity->Sms->curWeapon;
	  if (g_bRealisticAvionics && wpn && ((CampBaseClass*)wpn)->GetSPType() == SPTYPE_AIM9P)
	  {
	     SimDriver.playerEntity->FCC->missileSpotScanCmd = FALSE;
	  }
	  else
	  {
         SimDriver.playerEntity->FCC->missileSpotScanCmd = TRUE;
	  }
   }
}

// Marco Edit - Support for Bore/Slave
void SimToggleMissileBoreSlave (unsigned long val, int state, void *)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
      SimDriver.playerEntity->FCC->missileSlaveCmd = TRUE;
   }
}

// Marco Edit - Support for TD/BP
void SimToggleMissileTDBPUncage (unsigned long val, int state, void *)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN) && SimDriver.playerEntity->Sms)
   {
      SimWeaponClass* wpn; 
	  wpn = SimDriver.playerEntity->Sms->curWeapon;
	  if (g_bRealisticAvionics && wpn && ((CampBaseClass*)wpn)->GetSPType() == SPTYPE_AIM9P)
	  {
	     SimDriver.playerEntity->FCC->missileTDBPCmd = FALSE;
	  }
	  else
	  {
         SimDriver.playerEntity->FCC->missileTDBPCmd = TRUE;
	  }
   }
}

void SimDropChaff(unsigned long, int state, void*)
{
	static unsigned int realEWSProgNum = FALSE;
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
	   
	   //MI
	   if(!g_bRealisticAvionics || !SimDriver.playerEntity->af->platform->IsF16())
		   SimDriver.playerEntity->dropChaffCmd = TRUE;
	   else if (g_bMLU)
	   {
		//me123 hack hack  i want the posibility to assign two programs...so now a chaff hit will default to program 1
			
			if (!realEWSProgNum) realEWSProgNum= SimDriver.playerEntity->EWSProgNum;
			SimDriver.playerEntity->EWSProgNum = 0;
		    SimDriver.playerEntity->DropEWS();
		    SimDropProgrammed(0, KEY_DOWN, NULL);

	   }
	   else
	   {
		   SimDropProgrammed(0, KEY_DOWN, NULL);
	   }
   }
   else if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) &&
	   g_bMLU)
   {
	   SimDriver.playerEntity->EWSProgNum = realEWSProgNum ;
	   realEWSProgNum = FALSE;
   }
}

void SimDropFlare(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
	   if(!g_bRealisticAvionics || !SimDriver.playerEntity->af->platform->IsF16())
		   SimDriver.playerEntity->dropFlareCmd = TRUE;
	   else
		   SimDropProgrammed(0, KEY_DOWN, NULL);
   }
}

void SimHSDRangeStepUp (unsigned long, int state, void*)
{
   //if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) // JB 010220 CTD
	 if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && !F4IsBadReadPtr(SimDriver.playerEntity, sizeof(AircraftClass)) && SimDriver.playerEntity->FCC && !F4IsBadWritePtr(SimDriver.playerEntity->FCC, sizeof(FireControlComputer)) && (state & KEY_DOWN)) // JB 010220 CTD
   {
      SimDriver.playerEntity->FCC->HSDRangeStepCmd = 1;
   }
}

void SimHSDRangeStepDown (unsigned long, int state, void*)
{
   //if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) // JB 010220 CTD
	 if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && !F4IsBadReadPtr(SimDriver.playerEntity, sizeof(AircraftClass)) && SimDriver.playerEntity->FCC && !F4IsBadWritePtr(SimDriver.playerEntity->FCC, sizeof(FireControlComputer)) && (state & KEY_DOWN)) // JB 010220 CTD
   {
      SimDriver.playerEntity->FCC->HSDRangeStepCmd = -1;
   }
}

void SimToggleInvincible(unsigned long, int state, void*)
{
	
	if(FalconLocalGame && !FalconLocalGame->rules.InvulnerableOn())
		return;	
	
	if (state & KEY_DOWN)
	{
		if(PlayerOptions.InvulnerableOn())
			{
			PlayerOptions.ClearSimFlag(SIM_INVULNERABLE);
			SimDriver.playerEntity->UnSetFalcFlag(FEC_INVULNERABLE);
			}
		else
			{
			PlayerOptions.SetSimFlag(SIM_INVULNERABLE);
			SimDriver.playerEntity->SetFalcFlag(FEC_INVULNERABLE);
			}
	}
}

void SimFCCSubModeStep (unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
      SimDriver.playerEntity->FCC->NextSubMode();
   }
}

void SimEndFlight(unsigned long, int state, void*)
{
   if ((state & KEY_DOWN))
   {
      OTWDriver.EndFlight();
   }
}

void SimNextAAWeapon(unsigned long val, int state, void* pButton)
{
SMSClass* Sms;

   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
	   if(!g_bRealisticAvionics)
	   {
		   //MI original Code
		   if ( ((CPButtonObject*)pButton)->GetCurrentState() == CPBUTTON_OFF ) {
			   SimICPAA(val, state, pButton);
			   Sms = SimDriver.playerEntity->Sms;
			   if (Sms)
				Sms->GetNextWeapon(wdAir);
		   } else {
			   Sms = SimDriver.playerEntity->Sms;
			   if (Sms)
				Sms->GetNextWeapon(wdAir);
			   OTWDriver.pCockpitManager->mpIcp->HandleInput(AA_BUTTON, (CPButtonObject*)pButton);
			   MfdDisplay[0]->SetNewMode(MFDClass::FCRMode);
			   MfdDisplay[1]->SetNewMode(MFDClass::SMSMode);
		   }
	   }
	   else
	   {
		   //MI modified for ICP
		   if(SimDriver.playerEntity->FCC->GetMasterMode() != (FireControlComputer::Gun) &&
			   SimDriver.playerEntity->FCC->GetMasterMode() != (FireControlComputer::Missile) &&
			   SimDriver.playerEntity->FCC->GetMasterMode() != (FireControlComputer::Dogfight) &&
			   SimDriver.playerEntity->FCC->GetMasterMode() != (FireControlComputer::MissileOverride))
					SimDriver.playerEntity->FCC->SetMasterMode(FireControlComputer::Gun);
		   Sms = SimDriver.playerEntity->Sms;
		   if (Sms) 
		   {
			   Sms->GetNextWeapon(wdAir);
			   Sms->drawable->SetDisplayMode(SmsDrawable::Wpn);
			   // Marco Edit - Dogfight check for AIM120
			   if (SimDriver.playerEntity->FCC->GetMasterMode() == (FireControlComputer::Dogfight) && Sms->curWeaponType == wtAim120)
			   {
					SimDriver.playerEntity->FCC->SetDgftSubMode(FireControlComputer::Aim120);
			   }
		   }

		   // Put the radar in the its default AA mode
		   RadarClass* pradar = (RadarClass*) FindSensor (SimDriver.playerEntity, SensorClass::Radar);
			 if (pradar)
			   pradar->DefaultAAMode();
		   OTWDriver.pCockpitManager->mpIcp->SetICPFlag(ICPClass::MODE_A_A);
		   OTWDriver.pCockpitManager->mpIcp->ClearICPFlag(ICPClass::MODE_A_G);
		   MfdDisplay[0]->SetNewMasterMode(SimDriver.playerEntity->FCC->GetMainMasterMode());
		   MfdDisplay[1]->SetNewMasterMode(SimDriver.playerEntity->FCC->GetMainMasterMode());
	   }
   }
}

void SimStepMasterArm(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN) && SimDriver.playerEntity->Sms)
   {
      SimDriver.playerEntity->Sms->StepMasterArm();
   }
}

void SimArmMasterArm(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN) && SimDriver.playerEntity->Sms)
   {
      SimDriver.playerEntity->Sms->SetMasterArm(SMSBaseClass::Arm);
   }
}

void SimSafeMasterArm(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN) && SimDriver.playerEntity->Sms)
   {
      SimDriver.playerEntity->Sms->SetMasterArm(SMSBaseClass::Safe);
   }
}

void SimSimMasterArm(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN) && SimDriver.playerEntity->Sms)
   {
      SimDriver.playerEntity->Sms->SetMasterArm(SMSBaseClass::Sim);
   }
}

void SimNextAGWeapon (unsigned long val, int state, void* pButton)
{
SMSClass* Sms;

	 if (
	  F4IsBadReadPtr(SimDriver.playerEntity, sizeof(AircraftClass)) || !SimDriver.playerEntity->FCC || // JB 010305 CTD
		F4IsBadReadPtr(SimDriver.playerEntity->FCC, sizeof(FireControlComputer)) || // JB 010305 CTD
    SimDriver.playerEntity->FCC->GetMasterMode() == FireControlComputer::MissileOverride ||
		  SimDriver.playerEntity->FCC->GetMasterMode() == FireControlComputer::Dogfight) return;

	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(!g_bRealisticAvionics)
		{
			//MI original code
			if ( ((CPButtonObject*)pButton)->GetCurrentState() == CPBUTTON_OFF ) {
				SimICPAG(val, state, pButton);
				Sms = SimDriver.playerEntity->Sms;
				if (Sms)
				Sms->GetNextWeapon(wdGround);
			} else {
				Sms = SimDriver.playerEntity->Sms;
				if (Sms)
				Sms->GetNextWeapon(wdGround);
			   OTWDriver.pCockpitManager->mpIcp->HandleInput(AG_BUTTON, (CPButtonObject*)pButton);
			   MfdDisplay[0]->SetNewMode(MFDClass::FCRMode);
			   MfdDisplay[1]->SetNewMode(MFDClass::SMSMode);
			}
		}
		else
		{
			//MI modified for ICP
			Sms = SimDriver.playerEntity->Sms;
			if (Sms) 
			{
			    Sms->GetNextWeapon(wdGround);
			    Sms->drawable->SetDisplayMode(SmsDrawable::Wpn);
			}
			//Put the radar in the its default AG mode
			RadarClass* pradar = (RadarClass*) FindSensor (SimDriver.playerEntity, SensorClass::Radar);
			if (pradar)
				pradar->DefaultAGMode();
			OTWDriver.pCockpitManager->mpIcp->SetICPFlag(ICPClass::MODE_A_G);
			OTWDriver.pCockpitManager->mpIcp->ClearICPFlag(ICPClass::MODE_A_A);
			// Configure the MFDs
			MfdDisplay[0]->SetNewMasterMode(SimDriver.playerEntity->FCC->GetMainMasterMode());
			MfdDisplay[1]->SetNewMasterMode(SimDriver.playerEntity->FCC->GetMainMasterMode());
		}
	}
}

void SimNextNavMode (unsigned long val, int state, void* pButton)
{
	if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.playerEntity)->ejectTriggered)
	{
		if (OTWDriver.pCockpitManager->mpIcp->GetICPPrimaryMode() != NAV_MODE) {
			SimICPNav(val, state, pButton);
		} else {
			SimICPTILS(val, state, OTWDriver.pCockpitManager->GetButtonPointer( ICP_ILS_BUTTON_ID ));
		}
	}
}

void SimEject(unsigned long, int state, void*)
{
	if(SimDriver.playerEntity != NULL && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
	{
		// We only want to eject if the eject key combo is held 
		// for > 1 second.
		if(((AircraftClass *)SimDriver.playerEntity)->ejectTriggered == FALSE)
		{
			if(state & KEY_DOWN)
			{
				//MI
				if(g_bRealisticAvionics)
				{
					if(((AircraftClass*)SimDriver.playerEntity)->SeatArmed)
					{
						// Start the timer
						((AircraftClass *)SimDriver.playerEntity)->ejectCountdown = 1.0;
						((AircraftClass *)SimDriver.playerEntity)->doEjectCountdown = TRUE;
					}
				}
				else
				{
					// Start the timer
					((AircraftClass *)SimDriver.playerEntity)->ejectCountdown = 1.0;
					((AircraftClass *)SimDriver.playerEntity)->doEjectCountdown = TRUE;
				}
			}
			else
			{
				// Cancel the timer
				((AircraftClass *)SimDriver.playerEntity)->doEjectCountdown = FALSE;
			}
		}
	}
}


/// SIM Time Management
void TimeAccelerate (unsigned long, int state, void*)
{
   // edg: it's ok to accel time when ejected....
   // if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
   {
      if (state & KEY_DOWN)
      {
         if (gameCompressionRatio != 2)
            SetTimeCompression (2);
         else
            SetTimeCompression (1);
		 F4HearVoices();
      }	  
   }
}

void TimeAccelerateMaxToggle (unsigned long, int state, void*)
{
   // edg: it's ok to accel time when ejected....
   // if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
   {
      if (state & KEY_DOWN)
      {
         if (gameCompressionRatio != 4)
            SetTimeCompression (4);
         else
            SetTimeCompression (1);
		 F4HearVoices();
      }
   }
}

// JB 010109
void TimeAccelerateInc (unsigned long, int state, void*)
{
	int newcomp;

	if (state & KEY_DOWN)
	{
		newcomp = gameCompressionRatio * 2;
		if (newcomp > g_nMaxSimTimeAcceleration)
			newcomp = g_nMaxSimTimeAcceleration;

		SetTimeCompression (newcomp);
	
		F4HearVoices();
	}	  
}

void TimeAccelerateDec (unsigned long, int state, void*)
{
	int newcomp;

	if (state & KEY_DOWN)
	{
		newcomp = gameCompressionRatio / 2;
		if (newcomp == 0)
			newcomp = 1;

		SetTimeCompression (newcomp);
	
		F4HearVoices();
	}	  
}
// JB 010109

// SMS Control
void BombRippleIncrement (unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
   {
      if (state & KEY_DOWN && SimDriver.playerEntity->Sms)
      {
         SimDriver.playerEntity->Sms->IncrementRippleCount();
      }
   }
}

void BombIntervalIncrement (unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
   {
      if (state & KEY_DOWN && SimDriver.playerEntity->Sms)
      {
         SimDriver.playerEntity->Sms->IncrementRippleInterval();
      }
   }
}

void BombRippleDecrement (unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
   {
      if (state & KEY_DOWN && SimDriver.playerEntity->Sms)
      {
         SimDriver.playerEntity->Sms->DecrementRippleCount();
      }
   }
}

void BombIntervalDecrement (unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
   {
      if (state & KEY_DOWN && SimDriver.playerEntity->Sms)
      {
         SimDriver.playerEntity->Sms->DecrementRippleInterval();
      }
   }
}

void BombBurstIncrement (unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
   {
      if (state & KEY_DOWN && SimDriver.playerEntity->Sms)
      {
         SimDriver.playerEntity->Sms->IncrementBurstHeight();
      }
   }
}

void BombBurstDecrement (unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
   {
      if (state & KEY_DOWN && SimDriver.playerEntity->Sms)
      {
         SimDriver.playerEntity->Sms->DecrementBurstHeight();
      }
   }
}

void BombPairRelease (unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
   {
      if (state & KEY_DOWN && SimDriver.playerEntity->Sms)
      {
         SimDriver.playerEntity->Sms->SetPair(TRUE);
      }
   }
}

void BombSGLRelease (unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
   {
      if (state & KEY_DOWN && SimDriver.playerEntity->Sms)
      {
         SimDriver.playerEntity->Sms->SetPair(FALSE);
      }
   }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void AFBrakesOut (unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
   {
      if (SimDriver.playerEntity->af->HydraulicA() == 0) return;
      if (state & KEY_DOWN)
      {
         SimDriver.playerEntity->af->speedBrake = 1.0F;
      }
      else
      {
         SimDriver.playerEntity->af->speedBrake = 0.0F;
      }
   }
}

void AFBrakesToggle (unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
   {
      if (SimDriver.playerEntity->af->HydraulicA() == 0) return;
      if (state & KEY_DOWN)
      {
		  //MI fix for the new Speedbrake
#if 0
         if (SimDriver.playerEntity->af->speedBrake > 0.0F)
            SimDriver.playerEntity->af->speedBrake = -1.0F;
         else
            SimDriver.playerEntity->af->speedBrake = 1.0F;
#else
		 //Close the brake
		 if(SimDriver.playerEntity->af->dbrake > 0)
			 SimDriver.playerEntity->af->speedBrake = -1.0F;
		 else
			 SimDriver.playerEntity->af->speedBrake = 1.0F;

		 SimDriver.playerEntity->af->BrakesToggle = TRUE;
#endif


		 /*
	   	 F4SoundFXSetPos( SFX_AIRBRAKE, TRUE,
				SimDriver.playerEntity->XPos(),
				SimDriver.playerEntity->YPos(),
				SimDriver.playerEntity->ZPos(), 1.0f );
		  */
      }
   }
}

void AFBrakesIn (unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
   {
      if (SimDriver.playerEntity->af->HydraulicA() == 0) return;
      if (state & KEY_DOWN)
         SimDriver.playerEntity->af->speedBrake = -1.0F;
      else
         SimDriver.playerEntity->af->speedBrake = 0.0F;
   }
}

void AFGearToggle (unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && SimDriver.playerEntity->af && (state & KEY_DOWN)) // 2002-02-15 MODIFIED BY S.G. Uncommented the ...IsSetFlag(MOTION_OWNSHIP) section and moved it before the ...af check since MOTION_OWNSHIP is only cleared when we're ejected (and we have no gears when ejected anyway)
   {
       // check to see if gear is working
	   //MI but we want to be able to move our handle with no hydraulics,
	   //at least when on ground as nothing happens with the gear anyway
	   if(SimDriver.playerEntity->af->IsSet(AirframeClass::InAir))
	   {
		   if ( SimDriver.playerEntity->mFaults->GetFault( FaultClass::gear_fault ) ||
			   SimDriver.playerEntity->af->HydraulicB() == 0 ||
			   SimDriver.playerEntity->af->altGearDeployed)
		   {
			   return;
		   }
	   }
       
         if (SimDriver.playerEntity->af->gearHandle > 0.0F)
		 {
            SimDriver.playerEntity->af->gearHandle = -1.0F;
			/*
	   		F4SoundFXSetPos( SFX_GEARUP, TRUE,
				SimDriver.playerEntity->XPos(),
				SimDriver.playerEntity->YPos(),
				SimDriver.playerEntity->ZPos(), 1.0f );
			 */
		 }
         else
		 {
            SimDriver.playerEntity->af->gearHandle = 1.0F;
			/*
	   		F4SoundFXSetPos( SFX_GEARDN, TRUE,
				SimDriver.playerEntity->XPos(),
				SimDriver.playerEntity->YPos(),
				SimDriver.playerEntity->ZPos(), 1.0f );
			*/
		 }
   }
}

void AFElevatorUp (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
//      pitchStickOffsetRate = 0.5F;
		pitchStickOffsetRate = g_fAFElevatorUp;
   else
   {
      pitchStickOffsetRate = 0.0F;
   }
}

void AFElevatorDown (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
//      pitchStickOffsetRate -= 0.5F;
		pitchStickOffsetRate = -g_fAFElevatorDown;
   else
   {
      pitchStickOffsetRate = 0.0F;
   }
}

void AFAileronRight (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
//      rollStickOffsetRate += 0.8F;
		rollStickOffsetRate = g_fAFAileronRight;
   else
   {
      rollStickOffsetRate = 0.0F;
   }
}

void AFAileronLeft (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
//      rollStickOffsetRate -= 0.8F;
		rollStickOffsetRate = -g_fAFAileronLeft;
   else
   {
      rollStickOffsetRate = 0.0F;
   }
}

void AFThrottleUp (unsigned long, int state, void*)
{
	if (state & KEY_DOWN)
	{
		if(!UseKeyboardThrottle)
		{
			UseKeyboardThrottle = TRUE;
			throttleOffset = UserStickInputs.throttle;
		}			
		//throttleOffsetRate += 0.01F;
		throttleOffsetRate = g_fAFThrottleUp;
	}
	else
		throttleOffsetRate = 0.0F;
}


void AFThrottleDown (unsigned long, int state, void*)
{
	if (state & KEY_DOWN)
	{
		if(!UseKeyboardThrottle)
		{
			UseKeyboardThrottle = TRUE;
			throttleOffset = UserStickInputs.throttle;
		}
//		throttleOffsetRate -= 0.01F;
		throttleOffsetRate = -g_fAFThrottleDown;
	}
	else
		throttleOffsetRate = 0.0F;
}

void AFRudderLeft (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
//      rudderOffsetRate = 0.5F;
		rudderOffsetRate = g_fAFRudderLeft;
   else
   {
      rudderOffsetRate = 0.0F;
   }
}

void AFRudderRight (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
//      rudderOffsetRate = -0.5F;
		rudderOffsetRate = -g_fAFRudderRight;
   else
   {
      rudderOffsetRate = 0.0F;
   }
}

void AFCoarseThrottleUp (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
   {
   int tmpThrottle = FloatToInt32(throttleOffset * 100.0F);

      if (throttleOffset < 1.0F)
      {
         throttleOffset = (tmpThrottle + 25) * 0.01F;
      }
      else
      {
         throttleOffset = (tmpThrottle + 50/30) * 0.01F;
      }
   }
}

void AFCoarseThrottleDown (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
   {
   int tmpThrottle = FloatToInt32(throttleOffset * 100.0F);

      if (throttleOffset < 1.0F)
      {
         throttleOffset = (tmpThrottle - 25) * 0.01F;
      }
      else
      {
         throttleOffset = (tmpThrottle - 50/30) * 0.01F;
      }
   }
}

void AFABFull (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN) && (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
      throttleOffset = max (1.50F, throttleOffset);
}

void AFABOn (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN) && (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
      throttleOffset = max (1.01F, throttleOffset);
}

void AFIdle (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN) && (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
      throttleOffset = 0.0F;
}

// 2000-11-23 S.G. Back to its original state. I've added the 'SimCATSwitch' routine
#if 0
	void OTWTimeOfDayStep (unsigned long, int state, void*)//me123 now cat III
	{
	   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN) && SimDriver.playerEntity->Sms)
		  SimDriver.playerEntity->Sms->StepCatIII();
	}
#else
	void OTWTimeOfDayStep (unsigned long, int state, void*)
	{
	#ifdef _DEBUG
	   if ((state & KEY_DOWN) && (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
		  OTWDriver.todOffset += 1800.0F;
	#endif
	}
#endif

void OTWStepNextAC (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN))
      OTWDriver.ViewStepNext();
}

void OTWStepPrevAC (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN))
      OTWDriver.ViewStepPrev();
}

void OTWStepNextPadlock (unsigned long, int state, void*)
{
	if ((state & KEY_DOWN) && (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))) {
		// 2002-02-08 ADDED BY S.G. If we're not in PadlockF3 or PadlockEFOV mode, we need to switch to it first...
		if (FalconLocalGame->rules.GetPadlockMode() != PDDisabled && OTWDriver.GetOTWDisplayMode() != OTWDriverClass::ModePadlockF3 && OTWDriver.GetOTWDisplayMode() != OTWDriverClass::ModePadlockEFOV) {
			OTWDriver.Padlock_SetPriority(OTWDriverClass::PriorityNone);
			OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModePadlockF3);
		}
		else // END OF ADDED SECTION 2002-02-08
			OTWDriver.TargetStepNext();
	}
}

// 2002-03-12 ADDED BY S.G. So we can priorotize air things
void OTWStepNextPadlockAA (unsigned long, int state, void*)
{
	if ((state & KEY_DOWN) && (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))) {
		// 2002-02-08 ADDED BY S.G. If we're not in PadlockF3 or PadlockEFOV mode, we need to switch to it first...
		if (FalconLocalGame->rules.GetPadlockMode() != PDDisabled && OTWDriver.GetOTWDisplayMode() != OTWDriverClass::ModePadlockF3 && OTWDriver.GetOTWDisplayMode() != OTWDriverClass::ModePadlockEFOV) {
			OTWDriver.Padlock_SetPriority(OTWDriverClass::PriorityAA);
			OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModePadlockF3);
		}
		else // END OF ADDED SECTION 2002-02-08
			OTWDriver.TargetStepNext();
	}
}

// 2002-03-12 ADDED BY S.G. So we can priorotize ground things
void OTWStepNextPadlockAG (unsigned long, int state, void*)
{
	if ((state & KEY_DOWN) && (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))) {
		// 2002-02-08 ADDED BY S.G. If we're not in PadlockF3 or PadlockEFOV mode, we need to switch to it first...
		if (FalconLocalGame->rules.GetPadlockMode() != PDDisabled && OTWDriver.GetOTWDisplayMode() != OTWDriverClass::ModePadlockF3 && OTWDriver.GetOTWDisplayMode() != OTWDriverClass::ModePadlockEFOV) {
			OTWDriver.Padlock_SetPriority(OTWDriverClass::PriorityAG);
			OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModePadlockF3);
		}
		else // END OF ADDED SECTION 2002-02-08
			OTWDriver.TargetStepNext();
	}
}

void OTWStepPrevPadlock (unsigned long, int state, void*)
{
	if ((state & KEY_DOWN) && (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))) {
		// 2002-02-08 ADDED BY S.G. If we're not in PadlockF3 or PadlockEFOV mode, we need to switch to it first...
		if (FalconLocalGame->rules.GetPadlockMode() != PDDisabled && OTWDriver.GetOTWDisplayMode() != OTWDriverClass::ModePadlockF3 && OTWDriver.GetOTWDisplayMode() != OTWDriverClass::ModePadlockEFOV) {
			OTWDriver.Padlock_SetPriority(OTWDriverClass::PriorityNone);
			OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModePadlockF3);
		}
		else // END OF ADDED SECTION 2002-02-08
			OTWDriver.TargetStepPrev();
	}
}

// 2002-03-12 ADDED BY S.G. So we can priorotize air things
void OTWStepPrevPadlockAA (unsigned long, int state, void*)
{
	if ((state & KEY_DOWN) && (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))) {
		// 2002-02-08 ADDED BY S.G. If we're not in PadlockF3 or PadlockEFOV mode, we need to switch to it first...
		if (FalconLocalGame->rules.GetPadlockMode() != PDDisabled && OTWDriver.GetOTWDisplayMode() != OTWDriverClass::ModePadlockF3 && OTWDriver.GetOTWDisplayMode() != OTWDriverClass::ModePadlockEFOV) {
			OTWDriver.Padlock_SetPriority(OTWDriverClass::PriorityAA);
			OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModePadlockF3);
		}
		else // END OF ADDED SECTION 2002-02-08
			OTWDriver.TargetStepPrev();
	}
}

// 2002-03-12 ADDED BY S.G. So we can priorotize ground things
void OTWStepPrevPadlockAG (unsigned long, int state, void*)
{
	if ((state & KEY_DOWN) && (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))) {
		// 2002-02-08 ADDED BY S.G. If we're not in PadlockF3 or PadlockEFOV mode, we need to switch to it first...
		if (FalconLocalGame->rules.GetPadlockMode() != PDDisabled && OTWDriver.GetOTWDisplayMode() != OTWDriverClass::ModePadlockF3 && OTWDriver.GetOTWDisplayMode() != OTWDriverClass::ModePadlockEFOV) {
			OTWDriver.Padlock_SetPriority(OTWDriverClass::PriorityAG);
			OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModePadlockF3);
		}
		else // END OF ADDED SECTION 2002-02-08
			OTWDriver.TargetStepPrev();
	}
}

void OTWToggleNames (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.IDTagToggle();
}

void OTWToggleCampNames (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.CampTagToggle();
}

void OTWToggleActionCamera (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && FalconLocalGame->rules.ExternalViewOn())
      OTWDriver.ToggleActionCamera();
}

void OTWSelectEFOVPadlockMode (unsigned long, int state, void*)
{
   if ( (FalconLocalGame->rules.GetPadlockMode() != PDDisabled) && (state & KEY_DOWN) && \
	   (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
   {
	  // 20020-03-12 MODIFIED BY S.G. Not working, lets try with three different functions, one that doesn't care, one that checks for AA and one for AG
/*	  if (state & CTRL_KEY)
		 OTWDriver.Padlock_SetPriority(OTWDriverClass::PriorityAG);
	  else
		 OTWDriver.Padlock_SetPriority(OTWDriverClass::PriorityAA);
*/
	  OTWDriver.Padlock_SetPriority(OTWDriverClass::PriorityNone);
	  // END OF MODIFIED SECTION
	  OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModePadlockEFOV);
   }
}

// 2002-03-12 ADDED BY S.G. So we can priorotize air things
void OTWSelectEFOVPadlockModeAA (unsigned long, int state, void*)
{
   if ( (FalconLocalGame->rules.GetPadlockMode() != PDDisabled) && (state & KEY_DOWN) && \
	   (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
   {
	  OTWDriver.Padlock_SetPriority(OTWDriverClass::PriorityAA);
	  OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModePadlockEFOV);
   }
}

// 2002-03-12 ADDED BY S.G. So we can priorotize ground things
void OTWSelectEFOVPadlockModeAG (unsigned long, int state, void*)
{
   if ( (FalconLocalGame->rules.GetPadlockMode() != PDDisabled) && (state & KEY_DOWN) && \
	   (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
   {
	  OTWDriver.Padlock_SetPriority(OTWDriverClass::PriorityAG);
	  OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModePadlockEFOV);
   }
}

void OTWSelectF3PadlockMode (unsigned long, int state, void*)
{
   if ( (FalconLocalGame->rules.GetPadlockMode() != PDDisabled) && (state & KEY_DOWN) && \
	   (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
   {
	  // 20020-03-12 ADDED BY S.G. If we ask for, allow multi state padlocking by choosing what to look for	  
	  // 20020-03-12 MODIFIED BY S.G. Not working, lets try with three different functions, one that doesn't care, one that checks for AA and one for AG
/*	  if (state & CTRL_KEY)
		 OTWDriver.Padlock_SetPriority(OTWDriverClass::PriorityAG);
	  else
		 OTWDriver.Padlock_SetPriority(OTWDriverClass::PriorityAA);
*/
	  OTWDriver.Padlock_SetPriority(OTWDriverClass::PriorityNone);
	  // END OF MODIFIED SECTION
	  OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModePadlockF3);
   }
}

// 2002-03-12 ADDED BY S.G. So we can priorotize air things
void OTWSelectF3PadlockModeAA (unsigned long, int state, void*)
{
   if ( (FalconLocalGame->rules.GetPadlockMode() != PDDisabled) && (state & KEY_DOWN) && \
	   (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
   {
	  OTWDriver.Padlock_SetPriority(OTWDriverClass::PriorityAA);
	  OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModePadlockF3);
   }
}

// 2002-03-12 ADDED BY S.G. So we can priorotize ground things
void OTWSelectF3PadlockModeAG (unsigned long, int state, void*)
{
   if ( (FalconLocalGame->rules.GetPadlockMode() != PDDisabled) && (state & KEY_DOWN) && \
	   (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
   {
	  OTWDriver.Padlock_SetPriority(OTWDriverClass::PriorityAG);
	  OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModePadlockF3);
   }
}

void OTWStepMFD1 (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN) && (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
   {
      MfdDisplay[0]->changeMode = TRUE_NEXT;
	  //MI
	  if(SimDriver.playerEntity->FCC && SimDriver.playerEntity->FCC->IsSOI && g_bRealisticAvionics)
	  {
		  if(MfdDisplay[0]->CurMode() == MFDClass::FCCMode)
			  SimDriver.playerEntity->StepSOI(2);
	  }
   } 
}

void OTWStepMFD2 (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN) && (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
   {
      MfdDisplay[1]->changeMode = TRUE_NEXT;
	  //MI
	  if(SimDriver.playerEntity->FCC && SimDriver.playerEntity->FCC->IsSOI && g_bRealisticAvionics)
	  {
		  if(MfdDisplay[1]->CurMode() == MFDClass::FCCMode)
			  SimDriver.playerEntity->StepSOI(2);
	  }
   } 
}

void OTWStepMFD3 (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN) && (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
   {
      MfdDisplay[2]->changeMode = TRUE_NEXT;
	  //MI
	  if(SimDriver.playerEntity->FCC && SimDriver.playerEntity->FCC->IsSOI && g_bRealisticAvionics)
	  {
		  if(MfdDisplay[2]->CurMode() == MFDClass::FCCMode)
			  SimDriver.playerEntity->StepSOI(2);
	  }
   }
}

void OTWStepMFD4 (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN) && (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
   {
      MfdDisplay[3]->changeMode = TRUE_NEXT;
	  //MI
	  if(SimDriver.playerEntity->FCC && SimDriver.playerEntity->FCC->IsSOI && g_bRealisticAvionics)
	  {
		  if(MfdDisplay[3]->CurMode() == MFDClass::FCCMode)
			  SimDriver.playerEntity->StepSOI(2);
	  }
   }
}

void OTWToggleScales (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN) && (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
   {
		TheHud->CycleScalesSwitch();
   }
}

void OTWTogglePitchLadder (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN) && (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
   {
		TheHud->CycleFPMSwitch();
   }
}

void OTWStepHeadingScale (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN) && (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
      OTWDriver.StepHeadingScale();
}

void OTWSelectHUDMode (unsigned long, int state, void*)
{
	if ((state & KEY_DOWN) && (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
		OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModeHud);
}

void OTWToggleGLOC (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN) && !FalconLocalGame->rules.BlackoutOn() && \
	   (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
      OTWDriver.ToggleGLOC();
}

void OTWSelectChaseMode (unsigned long, int state, void*)
{
   if (FalconLocalGame && FalconLocalGame->rules.ExternalViewOn() && \
		(state & KEY_DOWN) && (SimDriver.playerEntity) && \
		(SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) )
		OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModeChase);
}

void OTWSelectOrbitMode (unsigned long, int state, void*)
{
	if (FalconLocalGame)
	{
		if(FalconLocalGame->rules.ExternalViewOn())
		{
			if(state & KEY_DOWN)
			{
				// if(SimDriver.playerEntity)
				// {
				// if(SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
				OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModeOrbit);
				// }
			}
		}
	}
}

void OTWTrackExternal(unsigned long, int state, void*) 
{
   if (FalconLocalGame && FalconLocalGame->rules.ExternalViewOn() && 
	   (state & KEY_DOWN) && (SimDriver.playerEntity) && 
	   (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) )
      OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModeTargetToSelf);
}

void OTWTrackTargetToWeapon(unsigned long, int state, void*)
{
   if (FalconLocalGame && FalconLocalGame->rules.ExternalViewOn() && \
	   (state & KEY_DOWN) && (SimDriver.playerEntity) && \
	   (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) )
      OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModeTargetToWeapon);
}

void OTWSelectAirFriendlyMode (unsigned long, int state, void*)
{
   if (FalconLocalGame && FalconLocalGame->rules.ExternalViewOn() && \
	   (state & KEY_DOWN) && (SimDriver.playerEntity) && \
	   (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
      OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModeAirFriendly);
}

void OTWSelectIncomingMode (unsigned long, int state, void*)
{
   if (FalconLocalGame && FalconLocalGame->rules.ExternalViewOn() && \
	   (state & KEY_DOWN) && (SimDriver.playerEntity) && \
	   (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
      OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModeIncoming);
}

void OTWSelectGroundFriendlyMode (unsigned long, int state, void*)
{
   if (FalconLocalGame && FalconLocalGame->rules.ExternalViewOn() && \
	   (state & KEY_DOWN) && (SimDriver.playerEntity) && \
	   (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
      OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModeGroundFriendly);
}

void OTWSelectAirEnemyMode (unsigned long, int state, void*)
{
   if (FalconLocalGame && FalconLocalGame->rules.ExternalViewOn() && \
	   (state & KEY_DOWN) && (SimDriver.playerEntity) && \
	   (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
      OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModeAirEnemy);
}

void OTWSelectGroundEnemyMode (unsigned long, int state, void*)
{
   if (FalconLocalGame && FalconLocalGame->rules.ExternalViewOn() && \
	   (state & KEY_DOWN) && (SimDriver.playerEntity) && \
	   (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
      OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModeGroundEnemy);
}

void OTWSelectTargetMode (unsigned long, int state, void*)
{
   if (FalconLocalGame && FalconLocalGame->rules.ExternalViewOn() && \
	   (state & KEY_DOWN) && (SimDriver.playerEntity) && \
	   (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
      OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModeTarget);
}

void OTWSelectWeaponMode (unsigned long, int state, void*)
{
   if (FalconLocalGame && FalconLocalGame->rules.ExternalViewOn() && \
	   (state & KEY_DOWN) && (SimDriver.playerEntity) && \
	   (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
      OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModeWeapon);
}

void OTWSelectSatelliteMode (unsigned long, int state, void*)
{
   if (FalconLocalGame && FalconLocalGame->rules.ExternalViewOn() && \
	   (state & KEY_DOWN) && (SimDriver.playerEntity) && \
	   (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
      OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModeSatellite);
}

void OTWSelectFlybyMode (unsigned long, int state, void*)
{
   if (FalconLocalGame && FalconLocalGame->rules.ExternalViewOn() && \
	   (state & KEY_DOWN) && (SimDriver.playerEntity) && \
	   (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
      OTWDriver.SetOTWDisplayMode(OTWDriverClass::ModeFlyby);
}

void OTWShowTestVersion (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
   {
      if (ShowVersion != 2)
         ShowVersion = 2;
      else
         ShowVersion = 0;
   }
}

void OTWShowVersion (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
   {
      if (ShowVersion != 1)
         ShowVersion = 1;
      else
         ShowVersion = 0;
   }
}

void OTWSelect2DCockpitMode (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN) && (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
	{
		OTWDriver.SetOTWDisplayMode(OTWDriverClass::Mode2DCockpit);
	}
}

void OTWSelect3DCockpitMode (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN) && (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
      OTWDriver.SetOTWDisplayMode(OTWDriverClass::Mode3DCockpit);
}

void OTWToggleBilinearFilter (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.ToggleBilinearFilter();
}

void OTWToggleShading (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.ToggleShading();
}

void OTWToggleHaze (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.ToggleHaze();
}

void OTWToggleLocationDisplay (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.ToggleLocationDisplay();
}

void OTWToggleAeroDisplay (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.ToggleAeroDisplay();
}

void OTWToggleRoof (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.ToggleRoof();
}

void OTWScaleDown (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.ScaleDown();
}

void OTWScaleUp (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.ScaleUp();
}

void OTWSetObjDetail (unsigned long val, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.SetDetail(val - DIK_1);
}

void OTWObjDetailDown (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.DetailDown();
}

void OTWObjDetailUp (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.DetailUp();
}

void OTWTextureIncrease (unsigned long, int state, void*) {

   if (state & KEY_DOWN)
      OTWDriver.TextureUp();
}

void OTWTextureDecrease (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.TextureDown();
}

void OTWToggleClouds (unsigned long, int state, void*)
{
   if (state & KEY_DOWN && FalconLocalGame->rules.WeatherOn())
      OTWDriver.ToggleWeather();
}

void OTWStepHudColor (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN) && (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
      TheHud->HudColorStep();
}

void OTWToggleEyeFly (unsigned long, int state, void*)
{
   if (state & KEY_DOWN && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) // 2002-02-15 MODIFIED BY S.G. ToggleEyeFly brings you to another AC and can crash when you're ejected since your class isn't AircraftClass anymore
      OTWDriver.ToggleEyeFly();
}

void OTWEnterPosition (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.StartLocationEntry();
}

void OTWToggleFrameRate (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      ShowFrameRate = 1 - ShowFrameRate;
}

void OTWToggleAutoScale (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
	   FalconDisplay.ToggleFullScreen();
      //OTWDriver.ToggleAutoScale();
}

void OTWSetScale (unsigned long val, int state, void*)
{
   if (state & KEY_DOWN)
   {
      OTWDriver.SetScale((float)(val - DIK_1 + 1));
      OTWDriver.RescaleAllObjects();
   }
}

void OTWViewUpRight (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
   {
      OTWDriver.ViewTiltUp();
	  OTWDriver.ViewSpinRight();
	  }
   else
      OTWDriver.ViewTiltHold();
}

void OTWViewUpLeft (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
   {
      OTWDriver.ViewTiltUp();
	  OTWDriver.ViewSpinLeft();
	  }
   else
      OTWDriver.ViewTiltHold();
}

void OTWViewDownRight (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
   {
      OTWDriver.ViewTiltDown();
	  OTWDriver.ViewSpinRight();
	  }
   else
      OTWDriver.ViewTiltHold();
}

void OTWViewDownLeft (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
   {
      OTWDriver.ViewTiltDown();
	  OTWDriver.ViewSpinLeft();
	  }
   else
      OTWDriver.ViewTiltHold();
}

void OTWViewUp (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.ViewTiltUp();
   else
      OTWDriver.ViewTiltHold();
}

void OTWViewDown (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.ViewTiltDown();
   else
      OTWDriver.ViewTiltHold();
}

void OTWViewLeft (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.ViewSpinLeft();
   else
      OTWDriver.ViewSpinHold();
}

void OTWViewRight (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.ViewSpinRight();
   else
      OTWDriver.ViewSpinHold();
}

void OTWViewReset (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.ViewReset();
}

void OTWViewZoomIn (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
   {
      OTWDriver.ViewZoomIn();

	  /*
	  ** edg: what is this?!
	  ** leave it our becuase its causing a CRASH when ejected
      theFault = theFault ++;
      if (theFault > 32)
         theFault = 0;
      MonoPrint ("Next fail %s\n", FaultClass::mpFSubSystemNames[theFault]);
	  */
   }
}

void OTWViewZoomOut (unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
   {
      OTWDriver.ViewZoomOut();
	  /*
	  ** edg: what is this?!
	  ** leave it our becuase its causing a CRASH when ejected
      ((AircraftClass*)SimDriver.playerEntity)->AddFault(1, (1 << theFault), 1, 0);
	  */
   }
}

void OTWSwapMFDS (unsigned long, int state, void*)
{
   if ((state & KEY_DOWN) && (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
      MFDSwapDisplays();
}

void OTWGlanceForward (unsigned long, int, void*)
{
	if ((SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
		OTWDriver.GlanceForward();
}

void OTWCheckSix (unsigned long, int, void*)
{
	if ((SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
		OTWDriver.GlanceAft();
}

void OTWStateStep (unsigned long, int state, void*)
{
   if (state & KEY_DOWN && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) // 2002-02-15 MODIFIED BY S.G. ToggleEyeFly brings you to another AC and can crash when you're ejected since your class isn't AircraftClass anymore
      OTWDriver.EyeFlyStateStep();
}


void CommandsSetKeyCombo (unsigned long val, int state, void*)
{
   if (state & KEY_DOWN)
   {
      CommandsKeyCombo = val;
      CommandsKeyComboMod = state & MODS_MASK;
   }
}

void KevinsFistOfGod (unsigned long, int state, void*)
{
	if (state & KEY_DOWN)
		RequestPlayerDivert();
}

void SuperCruise (unsigned long, int state, void*)
{
	if (state & KEY_DOWN)
	{
		supercruise = 1 - supercruise;
	}
}

// 2000-11-10 FUNCTION ADDED BY S.G. TO HANDLE THE 'driftCO' switch
void SimDriftCO (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) // 2002-02-15 MODIFIED BY S.G. Only valid when not ejected
	{
		TheHud->CycleDriftCOSwitch();
	}
}

// END OF ADDED SECTION

// 2000-11-17 FUNCTION ADDED BY S.G. TO HANDLE THE 'Cat I/III' switch
void SimCATSwitch(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN) && SimDriver.playerEntity->Sms)
   {
	// MI Changed to what we have before, so everything works like before
	// SimDriver.playerEntity->Sms->ChooseLimiterMode(127); // 127 means check Cat config
	SimDriver.playerEntity->Sms->StepCatIII();
   }
}

// END OF ADDED SECTION

void OTW1200DView (unsigned long, int state, void*){
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		OTWDriver.pCockpitManager->SetActivePanel(100);	//100 = id for 12:00 Down panel
	}
}

void OTW1200HUDView (unsigned long, int state, void*){
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		OTWDriver.pCockpitManager->SetActivePanel(0);	//0 = id for 12:00 hud panel
	}
}

void OTW1200View (unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) 
   {
	   //MI check for nightlighting
	   if(SimDriver.playerEntity)
	   {
		   if(SimDriver.playerEntity->WideView)
			   OTWDriver.pCockpitManager->SetActivePanel(91100);
		   else
			   OTWDriver.pCockpitManager->SetActivePanel(1100);
	   }
	   else
		   OTWDriver.pCockpitManager->SetActivePanel(1100);	//1100 = id for 12:00 50-50 panel
	}
}

void OTW1200LView (unsigned long, int state, void*){
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		OTWDriver.pCockpitManager->SetActivePanel(600 );	//100 = id for 12:00 panel
	}
}

void OTW1000View(unsigned long, int state, void*){
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		OTWDriver.pCockpitManager->SetActivePanel(500);	//101 = id for 10:00 panel
	}
}

void OTW200View(unsigned long, int state, void*){
	if ((state & KEY_DOWN) && (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
	{
		OTWDriver.pCockpitManager->SetActivePanel(200);	//102 = id for 2:00 panel
	}
}

void OTW300View(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) 
   {
		OTWDriver.pCockpitManager->SetActivePanel(300);	//103 = id for 3:00 panel
   } 
}

void OTW400View(unsigned long, int state, void*){
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		OTWDriver.pCockpitManager->SetActivePanel(400);	//104 = id for 4:00 panel
	}
}

void OTW800View(unsigned long, int state, void*){
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		OTWDriver.pCockpitManager->SetActivePanel(800);	//105 = id for 8:00 panel
	}
}

void OTW900View(unsigned long, int state, void*){
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		OTWDriver.pCockpitManager->SetActivePanel(900);	//106 = id for 9:00 panel
	}
}

void OTW1200RView(unsigned long, int state, void*){
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		OTWDriver.pCockpitManager->SetActivePanel(700);	//106 = id for 9:00 panel
	}
}

void SimToggleChatMode(unsigned long, int, void*)
{
   if (SimDriver.playerEntity)
   {
//	   F4ChatToggleXmitReceive();
   }
}

void SimWheelBrakes(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
   {
      if (SimDriver.playerEntity->af->HydraulicB() == 0) return;
      if (state & KEY_DOWN && !SimDriver.playerEntity->af->IsSet(AirframeClass::GearBroken))
         SimDriver.playerEntity->af->SetFlag(AirframeClass::WheelBrakes);
      else
         SimDriver.playerEntity->af->ClearFlag(AirframeClass::WheelBrakes);
   }
}

void SimMotionFreeze(unsigned long, int state, void*)
{
   if ((state & KEY_DOWN) && (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
   {
      SimDriver.SetMotion (1 - SimDriver.MotionOn());
      SetTimeCompression (1);
	  F4HearVoices();
   }
}

void ScreenShot(unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
   {
      OTWDriver.takeScreenShot = TRUE;
   }
}

void FOVToggle(unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
   {
      if (OTWDriver.GetFOV() > 30.0F * DTR)
      {
         narrowFOV = TRUE;
         OTWDriver.SetFOV( 20.0F * DTR );
      }
      else
      {
         narrowFOV = FALSE;
         OTWDriver.SetFOV( 60.0F * DTR );
      }
   }
}

void OTWToggleAlpha(unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
      OTWDriver.ToggleAlpha ();
}

void ACMIToggleRecording(unsigned long, int state, void*)
{
   if (state & KEY_DOWN)
	  SimDriver.doFile = TRUE;
}


void SimSelectiveJettison(unsigned long, int state, void*)
{
	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
	if (SimDriver.playerEntity->Sms && SimDriver.playerEntity->Sms->drawable)
         SimDriver.playerEntity->Sms->drawable->SetDisplayMode(SmsDrawable::SelJet);
	}
}

void SimEmergencyJettison(unsigned long, int state, void*)
{
 	if(SimDriver.playerEntity != NULL && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
 	{
		if(!SimDriver.playerEntity->Sms || !SimDriver.playerEntity->Sms->drawable)
			return;
		//MI
 		if(g_bRealisticAvionics)
 		{
 			//not if we're on the ground an our switch isn't set
 			if(SimDriver.playerEntity->OnGround() && 
 				SimDriver.playerEntity->Sms && !SimDriver.playerEntity->Sms->GndJett)
 			{
 				return;
 			}
		}
 		//Emergency Jettison is only happening if we hold the button more then 1 sec
 		//if(((AircraftClass *)SimDriver.playerEntity)->EmerJettTriggered == FALSE)
 		{
 			if(state & KEY_DOWN)
 			{
 				//MI
 				if(g_bRealisticAvionics)
 				{
					//Set our display mode
					if(MfdDisplay[1]->GetCurMode() != MFDClass::SMSMode || 
						SimDriver.playerEntity->Sms->drawable->DisplayMode() != SmsDrawable::EmergJet)
					{
						MfdDisplay[0]->EmergStoreMode = MfdDisplay[0]->CurMode();
						MfdDisplay[1]->EmergStoreMode = MfdDisplay[1]->CurMode();
						MfdDisplay[1]->SetNewMode(MFDClass::SMSMode);
						if(SimDriver.playerEntity->Sms->drawable->DisplayMode() !=
							SmsDrawable::EmergJet)
						{
							SimDriver.playerEntity->Sms->drawable->EmergStoreMode = 
								SimDriver.playerEntity->Sms->drawable->DisplayMode();

							SimDriver.playerEntity->Sms->drawable->SetDisplayMode(
								SmsDrawable::EmergJet);
						}
					}
					// Start the timer
					((AircraftClass *)SimDriver.playerEntity)->JettCountown = 1.0;
					((AircraftClass *)SimDriver.playerEntity)->doJettCountdown = TRUE;
				}
 				else
 				{
 					SimDriver.playerEntity->Sms->EmergencyJettison();
 				}
 			}
 			else
 			{
				MfdDisplay[1]->SetNewMode(MfdDisplay[1]->EmergStoreMode);
				SimDriver.playerEntity->Sms->drawable->SetDisplayMode(
					SimDriver.playerEntity->Sms->drawable->EmergStoreMode);
				// Cancel the timer
 				((AircraftClass *)SimDriver.playerEntity)->doJettCountdown = FALSE;
 			}
 		}
 	}
}

void SimECMOn(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
   {
      if (SimDriver.playerEntity->IsSetFlag(ECM_ON))
      {
         // Can't turn off ECM w/ ECM pod broken
         if (!SimDriver.playerEntity->mFaults->GetFault(FaultClass::epod_fault))
		 {
            SimDriver.playerEntity->UnSetFlag(ECM_ON);
			//MI EWS stuff
			if(g_bRealisticAvionics)
				SimDriver.playerEntity->ManualECM = FALSE;
		 }
      }
      else
      {
         // Can't turn on ECM w/ broken blanker
         if (SimDriver.playerEntity->HasSPJamming() && !SimDriver.playerEntity->mFaults->GetFault(FaultClass::blkr_fault))
		 {
			 //MI no Jammer with WOW, unless ground Jett is on
			 if(g_bRealisticAvionics && SimDriver.playerEntity->Sms)
			 {
				 if(SimDriver.playerEntity->OnGround() && !SimDriver.playerEntity->Sms->GndJett)
					 return;
			 }
			 SimDriver.playerEntity->SetFlag(ECM_ON);
			 
			 //MI EWS stuff
			 if(g_bRealisticAvionics)
				 SimDriver.playerEntity->ManualECM = TRUE;
		 }
      }
   }
}

void SoundOff(unsigned long, int state, void*)
{
	if(state & KEY_DOWN)
	{
		if(gSoundDriver->GetMasterVolume() <= -7000)
			gSoundDriver->SetMasterVolume(PlayerOptions.GroupVol[MASTER_SOUND_GROUP]);
		else
			gSoundDriver->SetMasterVolume(-10000);
	}
}

/////////////// Vince's Cockpit Stuff /////////////////
void SimHsiCourseInc (unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected... Vince, you should know better than this ;-)
		OTWDriver.pCockpitManager->mpHsi->IncState(CPHsi::HSI_STA_CRS_STATE);
	}
}

void SimHsiCourseDec(unsigned long, int state, void*)
{
   if (state & KEY_DOWN && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected... Vince, you should know better than this ;-)
		OTWDriver.pCockpitManager->mpHsi->DecState(CPHsi::HSI_STA_CRS_STATE);
	}
}

void SimHsiHeadingInc(unsigned long, int state, void*)
{
   if (state & KEY_DOWN && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected... Vince, you should know better than this ;-)
		OTWDriver.pCockpitManager->mpHsi->IncState(CPHsi::HSI_STA_HDG_STATE);
	}
}

void SimHsiHeadingDec(unsigned long, int state, void*)
{
   if (state & KEY_DOWN && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected... Vince, you should know better than this ;-)
		OTWDriver.pCockpitManager->mpHsi->DecState(CPHsi::HSI_STA_HDG_STATE);
	}
}

void SimAVTRToggle(unsigned long val, int state, void* pButton)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		// edg: folded my command function in here.  Not sure if
		// a simdriver setting needs to be made, but keeping it in
		ACMIToggleRecording ( val, state, pButton);
		if(SimDriver.AVTROn() == TRUE) {
			SimDriver.SetAVTR(FALSE);
		}
		else {
			SimDriver.SetAVTR(TRUE);
		}
	}
}


void SimMPOToggle(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
	   ShiAssert(SimDriver.playerEntity->af);
	   if(SimDriver.playerEntity->af->IsSet(AirframeClass::MPOverride))
	   {
		   SimDriver.playerEntity->af->ClearFlag(AirframeClass::MPOverride);
	   }
	   else
	   {
		   SimDriver.playerEntity->af->SetFlag(AirframeClass::MPOverride);
	   }
#ifdef DAVE_DBG
MoveBoom = 1 - MoveBoom;
#endif
/*

		if(SimDriver.playerEntity->MPOCmd == FALSE) {
			SimDriver.playerEntity->MPOCmd = TRUE;
		}
		else {
			SimDriver.playerEntity->MPOCmd = FALSE;
		}*/
	}
}

void BreakToggle(unsigned long, int, void*)
{
	// You can set a global flag here if you want.
	// This (might) be mapped to "Ctrl-z, t"
}

void SimSilenceHorn(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		ShiAssert(SimDriver.playerEntity->af);
		SimDriver.playerEntity->af->SetFlag(AirframeClass::HornSilenced);
	}
}

void SimStepHSIMode(unsigned long, int state, void*)
{
   if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		gNavigationSys->StepInstrumentMode();
	}
}
//////////////////////////////////////

// =============================================//
// Callback Function CBEOSB_1
// =============================================//
void SimCBEOSB_1L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 1, 1);
		MfdDisplay[0]->ButtonPushed(0,0);
	}
}

void SimCBEOSB_1R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 1, 1);
	   MfdDisplay[1]->ButtonPushed(0,1);
	}
}

// =============================================//
// Callback Function CBEOSB_2
// =============================================//

void SimCBEOSB_2L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 2, 1);
		MfdDisplay[0]->ButtonPushed(1,0);
	}
}

void SimCBEOSB_2R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 2, 1);
		MfdDisplay[1]->ButtonPushed(1,1);
	}
}

// =============================================//
// Callback Function CBEOSB_3
// =============================================//

void SimCBEOSB_3L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 3, 1);
		MfdDisplay[0]->ButtonPushed(2,0);
	}
}

void SimCBEOSB_3R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 3, 1);
		MfdDisplay[1]->ButtonPushed(2,1);
	}
}

// =============================================//
// Callback Function CBEOSB_4
// =============================================//

void SimCBEOSB_4L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 4, 1);
		MfdDisplay[0]->ButtonPushed(3,0);
	}
}

void SimCBEOSB_4R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 4, 1);
		MfdDisplay[1]->ButtonPushed(3,1);
	}
}


// =============================================//
// Callback Function CBEOSB_5
// =============================================//

void SimCBEOSB_5L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 5, 1);
	   MfdDisplay[0]->ButtonPushed(4,0);
	}
}

void SimCBEOSB_5R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 5, 1);
		MfdDisplay[1]->ButtonPushed(4,1);
	}
}

// =============================================//
// Callback Function CBEOSB_6
// =============================================//

void SimCBEOSB_6L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 6, 1);
		MfdDisplay[0]->ButtonPushed(5,0);
	}
}

void SimCBEOSB_6R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 6, 1);
		MfdDisplay[1]->ButtonPushed(5,1);
	}
}

// =============================================//
// Callback Function CBEOSB_7
// =============================================//
void SimCBEOSB_7L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 7, 1);
		MfdDisplay[0]->ButtonPushed(6,0);
	}
}

void SimCBEOSB_7R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 7, 1);
		MfdDisplay[1]->ButtonPushed(6,1);
	}
}

// =============================================//
// Callback Function CBEOSB_8
// =============================================//
void SimCBEOSB_8L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 8, 1);
		MfdDisplay[0]->ButtonPushed(7,0);
	}
}

void SimCBEOSB_8R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 8, 1);
		MfdDisplay[1]->ButtonPushed(7,1);
	}
}

// =============================================//
// Callback Function CBEOSB_9
// =============================================//

void SimCBEOSB_9L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 9, 1);
		MfdDisplay[0]->ButtonPushed(8,0);
	}
}

void SimCBEOSB_9R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 9, 1);
		MfdDisplay[1]->ButtonPushed(8,1);
	}
}

// =============================================//
// Callback Function CBEOSB_10
// =============================================//
void SimCBEOSB_10L(unsigned long, int state, void*) {
	//MI
	if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
	OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 10, 1);
   MfdDisplay[0]->ButtonPushed(9,0);
	}
}

void SimCBEOSB_10R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 10, 1);
		MfdDisplay[1]->ButtonPushed(9,1);
	}
}

// =============================================//
// Callback Function CBEOSB_11
// =============================================//

void SimCBEOSB_11L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 11, 1);
		MfdDisplay[0]->ButtonPushed(10,0);
	}
}

void SimCBEOSB_11R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 11, 1);
		MfdDisplay[1]->ButtonPushed(10,1);
	}
}

// =============================================//
// Callback Function CBEOSB_12
// =============================================//

void SimCBEOSB_12L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 12, 1);
		MfdDisplay[0]->ButtonPushed(11,0);
	}
}

void SimCBEOSB_12R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 12, 1);
		MfdDisplay[1]->ButtonPushed(11,1);
	}
}

// =============================================//
// Callback Function CBEOSB_13
// =============================================//
void SimCBEOSB_13L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 13, 1);
		MfdDisplay[0]->ButtonPushed(12,0);
	}
}

void SimCBEOSB_13R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 13, 1);
		MfdDisplay[1]->ButtonPushed(12,1);
	}
}

// =============================================//
// Callback Function CBEOSB_14
// =============================================//
void SimCBEOSB_14L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 14, 1);
		MfdDisplay[0]->ButtonPushed(13,0);
	}
}

void SimCBEOSB_14R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 14, 1);
		MfdDisplay[1]->ButtonPushed(13,1);
	}
}

// =============================================//
// Callback Function CBEOSB_15
// =============================================//
void SimCBEOSB_15L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 15, 1);
		MfdDisplay[0]->ButtonPushed(14,0);
	}
}

void SimCBEOSB_15R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 15, 1);
		MfdDisplay[1]->ButtonPushed(14,1);
	}
}

// =============================================//
// Callback Function CBEOSB_16
// =============================================//
void SimCBEOSB_16L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 16, 1);
		MfdDisplay[0]->ButtonPushed(15,0);
	}
}

void SimCBEOSB_16R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 16, 1);
		MfdDisplay[1]->ButtonPushed(15,1);
	}
}

// =============================================//
// Callback Function CBEOSB_17
// =============================================//
void SimCBEOSB_17L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 17, 1);
		MfdDisplay[0]->ButtonPushed(16,0);
	}
}

void SimCBEOSB_17R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 17, 1);
		MfdDisplay[1]->ButtonPushed(16,1);
	}
}

// =============================================//
// Callback Function CBEOSB_18
// =============================================//

void SimCBEOSB_18L(unsigned long, int state, void*) {
	if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 18, 1);
		MfdDisplay[0]->ButtonPushed(17,0);
	}
}

void SimCBEOSB_18R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 18, 1);
		MfdDisplay[1]->ButtonPushed(17,1);
	}
}

// =============================================//
// Callback Function CBEOSB_19
// =============================================//
void SimCBEOSB_19L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 19, 1);
		MfdDisplay[0]->ButtonPushed(18,0);
	}
}

void SimCBEOSB_19R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 19, 1);
		MfdDisplay[1]->ButtonPushed(18,1);
	}
}

// =============================================//
// Callback Function CBEOSB_20
// =============================================//
void SimCBEOSB_20L(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(0, 20, 1);
		MfdDisplay[0]->ButtonPushed(19,0);
	}
}

void SimCBEOSB_20R(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		OTWDriver.pCockpitManager->mMiscStates.SetMFDButtonState(1, 20, 1);
		MfdDisplay[1]->ButtonPushed(19,1);
	}
}

void SimCBEOSB_GAINUP_R(unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
	{
		MfdDisplay[1]->IncreaseBrightness();
	}
}

void SimCBEOSB_GAINUP_L(unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
	{
		MfdDisplay[0]->IncreaseBrightness();
	}
}

void SimCBEOSB_GAINDOWN_R(unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
	{
		MfdDisplay[1]->DecreaseBrightness();
	}
}

void SimCBEOSB_GAINDOWN_L(unsigned long, int state, void*)
{
	if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
	{
		MfdDisplay[0]->DecreaseBrightness();
	}
}


// =============================================//
// Callback Function CBEICPTILS
// =============================================//

void SimICPTILS(unsigned long, int state, void* pButton) 
{
	if(!g_bRealisticAvionics)
	{
		//MI Original code
		if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.playerEntity)->ejectTriggered) {
			OTWDriver.pCockpitManager->mpIcp->HandleInput(ILS_BUTTON, (CPButtonObject*)pButton);

			// If we're in NAV mode, update the FCC/HUD mode
			if (OTWDriver.pCockpitManager->mpIcp->GetICPPrimaryMode() == NAV_MODE) {
				if (OTWDriver.pCockpitManager->mpIcp->GetICPSecondaryMode() == ILS_MODE) {
					SimDriver.playerEntity->FCC->SetMasterMode(FireControlComputer::ILS);
				} else {
					SimDriver.playerEntity->FCC->SetMasterMode(FireControlComputer::Nav);
				}
			}
		}
	}
	else
	{
		//MI modified/added for ICP stuff
	    if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.playerEntity)->ejectTriggered) 
		{
	 	   OTWDriver.pCockpitManager->mpIcp->HandleInput(ILS_BUTTON, (CPButtonObject*)pButton);
		}

	    if(!OTWDriver.pCockpitManager->mpIcp->IsICPSet(ICPClass::MODE_CNI))
		    return;
	    else
		{
		    // If we're in NAV mode, update the FCC/HUD mode
		    //if (OTWDriver.pCockpitManager->mpIcp->GetICPPrimaryMode() == NAV_MODE) 
			if(SimDriver.playerEntity && SimDriver.playerEntity->FCC && SimDriver.playerEntity->FCC->IsNavMasterMode())
			{  
			    if (OTWDriver.pCockpitManager->mpIcp->GetICPSecondaryMode() == ILS_MODE &&
					(gNavigationSys->GetInstrumentMode() == NavigationSystem::ILS_NAV ||
					 gNavigationSys->GetInstrumentMode() == NavigationSystem::ILS_TACAN))
				{
				    SimDriver.playerEntity->FCC->SetMasterMode(FireControlComputer::ILS);
				} 
			    else	 
				{
				    SimDriver.playerEntity->FCC->SetMasterMode(FireControlComputer::Nav);
				}
			} 
		} 
	}
}

// =============================================//
// Callback Function CBEICPALOW
// =============================================//

void SimICPALOW(unsigned long, int state, void* pButton) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.playerEntity)->ejectTriggered) {
		OTWDriver.pCockpitManager->mpIcp->HandleInput(ALOW_BUTTON, (CPButtonObject*)pButton);
	}
}

// =============================================//
// Callback Function CBEICPFAck
// =============================================//
void SimICPFAck(unsigned long, int state, void* pButton) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.playerEntity)->ejectTriggered) {
		OTWDriver.pCockpitManager->mpIcp->HandleInput(FACK_BUTTON, (CPButtonObject*)pButton);
		if(g_bRealisticAvionics)
			SimDriver.playerEntity->mFaults->ClearAvioncFault();
	}
}


// =============================================//
// Callback Function CBEICPPrevious
// =============================================//

void SimICPPrevious(unsigned long, int state, void* pButton) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.playerEntity)->ejectTriggered) {
		OTWDriver.pCockpitManager->mpIcp->HandleInput(PREV_BUTTON, (CPButtonObject*)pButton);
	}
}

// =============================================//
// Callback Function CBEICPNext
// =============================================//

void SimICPNext(unsigned long, int state, void* pButton) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.playerEntity)->ejectTriggered) {
		OTWDriver.pCockpitManager->mpIcp->HandleInput(NEXT_BUTTON, (CPButtonObject*)pButton);
	}
}

// =============================================//
// Callback Function CBEICPLink
// =============================================//

void SimICPLink(unsigned long, int state, void* pButton) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.playerEntity)->ejectTriggered) {

// test code
		FalconDLinkMessage *pmsg;
		pmsg = new FalconDLinkMessage (SimDriver.playerEntity->Id(), FalconLocalGame);
		

		pmsg->dataBlock.numPoints	= 4;
		pmsg->dataBlock.targetType	= 50;
		pmsg->dataBlock.threatType	= 90;
   
		
		pmsg->dataBlock.ptype[0]	= FalconDLinkMessage::IP;
		pmsg->dataBlock.px[0]		= 300;
		pmsg->dataBlock.py[0]		= 300;
		pmsg->dataBlock.pz[0]		= 300;
		pmsg->dataBlock.arrivalTime[0]	= SimLibElapsedTime;

		pmsg->dataBlock.ptype[1]	= FalconDLinkMessage::TGT;
		pmsg->dataBlock.px[1]		= 400;
		pmsg->dataBlock.py[1]		= 500;
		pmsg->dataBlock.pz[1]		= 600;
		pmsg->dataBlock.arrivalTime[1]		= SimLibElapsedTime;

		pmsg->dataBlock.ptype[2]	= FalconDLinkMessage::EGR;
		pmsg->dataBlock.px[2]		= 500;
		pmsg->dataBlock.py[2]		= 800;
		pmsg->dataBlock.pz[2]		= 400;
		pmsg->dataBlock.arrivalTime[2]	= SimLibElapsedTime;

		pmsg->dataBlock.ptype[3]	= FalconDLinkMessage::CP;
		pmsg->dataBlock.px[3]		= 600;
		pmsg->dataBlock.py[3]		= 700;
		pmsg->dataBlock.pz[3]		= 300;
		pmsg->dataBlock.arrivalTime[3]	= SimLibElapsedTime;
	
		FalconSendMessage (pmsg, TRUE);
//end test code

		OTWDriver.pCockpitManager->mpIcp->HandleInput(DLINK_BUTTON, (CPButtonObject*)pButton);
	}
}

// =============================================//
// Callback Function CBEICPCrus
// =============================================//

void SimICPCrus(unsigned long, int state, void* pButton) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.playerEntity)->ejectTriggered) {
		OTWDriver.pCockpitManager->mpIcp->HandleInput(CRUS_BUTTON, (CPButtonObject*)pButton);
	}
}

// =============================================//
// Callback Function CBEICPStpt
// =============================================//

void SimICPStpt(unsigned long, int state, void* pButton) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.playerEntity)->ejectTriggered) {
		OTWDriver.pCockpitManager->mpIcp->HandleInput(STPT_BUTTON, (CPButtonObject*)pButton);
	}
}

// =============================================//
// Callback Function CBEICPMark
// =============================================//

void SimICPMark(unsigned long, int state, void* pButton) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.playerEntity)->ejectTriggered) {
		OTWDriver.pCockpitManager->mpIcp->HandleInput(MARK_BUTTON, (CPButtonObject*)pButton);
	}
}

// =============================================//
// Callback Function CBEICPEnter
// =============================================//

void SimICPEnter(unsigned long, int state, void* pButton) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.playerEntity)->ejectTriggered) {
		OTWDriver.pCockpitManager->mpIcp->HandleInput(ENTR_BUTTON, (CPButtonObject*)pButton);
	}
}

// =============================================//
// Callback Function CBEICPCom
// =============================================//

void SimICPCom1(unsigned long, int state, void* pButton) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.playerEntity)->ejectTriggered) {
		OTWDriver.pCockpitManager->mpIcp->HandleInput(COMM1_BUTTON, (CPButtonObject*)pButton);
	}
}


void SimICPCom2(unsigned long, int state, void* pButton) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.playerEntity)->ejectTriggered) {
		OTWDriver.pCockpitManager->mpIcp->HandleInput(COMM2_BUTTON, (CPButtonObject*)pButton);
	}
}

// =============================================//
// Callback Function CBEICPNav
// =============================================//

void SimICPNav(unsigned long, int state, void* pButton) {

   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.playerEntity)->ejectTriggered) {

		OTWDriver.pCockpitManager->mpIcp->HandleInput(NAV_BUTTON, (CPButtonObject*)pButton);
if (
	 F4IsBadReadPtr(SimDriver.playerEntity, sizeof(AircraftClass)) || // JB 010317 CTD
	 !SimDriver.playerEntity->FCC || // JB 010307 CTD
	 F4IsBadReadPtr(SimDriver.playerEntity->FCC, sizeof(FireControlComputer)) || // JB 010307 CTD
	 SimDriver.playerEntity->FCC->GetMasterMode() == FireControlComputer::MissileOverride ||
	 SimDriver.playerEntity->FCC->GetMasterMode() == FireControlComputer::Dogfight) return;

		// Select our FCC/HUD mode based on the NAV sub mode (ILS or not)
		if (OTWDriver.pCockpitManager->mpIcp->GetICPSecondaryMode() == ILS_MODE) {
			SimDriver.playerEntity->FCC->SetMasterMode(FireControlComputer::ILS);
		} else {
			SimDriver.playerEntity->FCC->SetMasterMode(FireControlComputer::Nav);
		}

		// Put the radar in the its default AA mode
		RadarClass* pradar = (RadarClass*) FindSensor (SimDriver.playerEntity, SensorClass::Radar);
		if (pradar && !F4IsBadCodePtr((FARPROC) pradar)) // JB 010220 CTD
			pradar->DefaultAAMode();
		
		if (g_bRealisticAvionics) {
		    MfdDisplay[0]->SetNewMasterMode(SimDriver.playerEntity->FCC->GetMainMasterMode());
		    MfdDisplay[1]->SetNewMasterMode(SimDriver.playerEntity->FCC->GetMainMasterMode());
		} else {
		    // Configure the MFDs
		    MfdDisplay[0]->SetNewMode(MFDClass::FCRMode);
		    MfdDisplay[1]->SetNewMode(MFDClass::FCCMode);
		}
	}
}

//MI Added for backup ICP Mode setting
void SimICPNav1(unsigned long, int state, void* pButton) 
{
	if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.playerEntity)->ejectTriggered) 
	{

		OTWDriver.pCockpitManager->mpIcp->HandleInput(NAV_BUTTON, (CPButtonObject*)pButton);

		// Select our FCC/HUD mode based on the NAV sub mode (ILS or not)
		if (OTWDriver.pCockpitManager->mpIcp->GetICPSecondaryMode() == ILS_MODE) 
		{
			SimDriver.playerEntity->FCC->SetMasterMode(FireControlComputer::ILS);
		} 
		else 
		{
			SimDriver.playerEntity->FCC->SetMasterMode(FireControlComputer::Nav);
		}

		// Put the radar in the its default AA mode
		RadarClass* pradar = (RadarClass*) FindSensor (SimDriver.playerEntity, SensorClass::Radar);
		if (pradar)
			pradar->DefaultAAMode();

		// Configure the MFDs
		MfdDisplay[0]->SetNewMasterMode(SimDriver.playerEntity->FCC->GetMainMasterMode());
		MfdDisplay[1]->SetNewMasterMode(SimDriver.playerEntity->FCC->GetMainMasterMode());
	}
}

// =============================================//
// Callback Function CBEICPAA
// =============================================//

void SimICPAA(unsigned long, int state, void* pButton) 
{
	if (!SimDriver.playerEntity || !SimDriver.playerEntity->IsSetFlag( MOTION_OWNSHIP )) // 2002-02-15 ADDED BY S.G. Do it once here and removed the corresponding line from below
		return;

	if(!g_bRealisticAvionics)
	{
		//MI Original code
		if (state & KEY_DOWN && !((AircraftClass*)SimDriver.playerEntity)->ejectTriggered) {

			OTWDriver.pCockpitManager->mpIcp->HandleInput(AA_BUTTON, (CPButtonObject*)pButton);
			SimDriver.playerEntity->FCC->SetMasterMode(FireControlComputer::Gun);

			// Put the radar in the its default AA mode
			RadarClass* pradar = (RadarClass*) FindSensor (SimDriver.playerEntity, SensorClass::Radar);
			if (pradar && !F4IsBadReadPtr(pradar, sizeof(RadarClass))) // JB 010404 CTD
				pradar->DefaultAAMode();

			// Configure the MFDs
			MfdDisplay[0]->SetNewMode(MFDClass::FCRMode);
			MfdDisplay[1]->SetNewMode(MFDClass::SMSMode);
		}
	}
	else
	{
		//MI 3/1/2002 if we're in DF or MRM, don't do anything
		if(SimDriver.playerEntity->FCC && (SimDriver.playerEntity->FCC->GetMasterMode() 
			== FireControlComputer::Dogfight || SimDriver.playerEntity->FCC->GetMasterMode() == FireControlComputer::MissileOverride))
			return;
		//Set our display mode
		// Marco edit - SimDriver.playerEntity was often 00000000 in dogfight - CTD S.G. Caugth above now
		if (SimDriver.playerEntity->Sms)
		{
			 if (SimDriver.playerEntity->Sms->drawable)//me123 ctd in dogfight games on entry...this was 00000000
			 {
				 SimDriver.playerEntity->Sms->drawable->SetDisplayMode(SmsDrawable::Wpn);
			 }
		}

	   //MI added/modified for ICP Stuff
	   if (state & KEY_DOWN && !((AircraftClass*)SimDriver.playerEntity)->ejectTriggered)    
	   { 
		   //Player hit the button. Is our MODE_A_A Flag set?
		   //Since we start in NAV Mode by default, we want to get into
		   //A_A mode the first time we push the button
		   if(OTWDriver.pCockpitManager->mpIcp->IsICPSet(ICPClass::MODE_A_A))
		   {
			   //Player pushed this button previously, so let's go back
			   //to NAV mode
			   SimDriver.playerEntity->FCC->SetMasterMode(FireControlComputer::Nav);
			   
			   //passes our puttonstate to the ICP
			   OTWDriver.pCockpitManager->mpIcp->HandleInput(NAV_BUTTON, (CPButtonObject*)pButton);
			   
			   // Put the radar in the its default AA mode
			   RadarClass* pradar = (RadarClass*) FindSensor (SimDriver.playerEntity, SensorClass::Radar);
			   if (pradar)
					pradar->DefaultAAMode();			   

			   // Configure the MFDs
			   MfdDisplay[0]->SetNewMasterMode(SimDriver.playerEntity->FCC->GetMainMasterMode());
			   MfdDisplay[1]->SetNewMasterMode(SimDriver.playerEntity->FCC->GetMainMasterMode());

			   //Clear the flag indicating we are in A_A mode
			   //This is so we know that we have to go into A_A mode next time
			   OTWDriver.pCockpitManager->mpIcp->ClearICPFlag(ICPClass::MODE_A_A);

			   //If our A_G mode flag is set, clear it so we get into
			   //A_G mode when we push the A_G button
			   if(OTWDriver.pCockpitManager->mpIcp->IsICPSet(ICPClass::MODE_A_G))
				   OTWDriver.pCockpitManager->mpIcp->ClearICPFlag(ICPClass::MODE_A_G);
		   }
		   
		   //no, we press this button the first time
		   else
		   {
			   SimDriver.playerEntity->FCC->SetMasterMode(FireControlComputer::Gun);

			   //passes our puttonstate to the ICP
			   OTWDriver.pCockpitManager->mpIcp->HandleInput(AA_BUTTON, (CPButtonObject*)pButton);
			   
			   // Put the radar in the its default AA mode
			   RadarClass* pradar = (RadarClass*) FindSensor (SimDriver.playerEntity, SensorClass::Radar);
			   if (pradar)
					pradar->DefaultAAMode();

			   // Configure the MFDs
			   MfdDisplay[0]->SetNewMasterMode(SimDriver.playerEntity->FCC->GetMainMasterMode());
			   MfdDisplay[1]->SetNewMasterMode(SimDriver.playerEntity->FCC->GetMainMasterMode());
			   
			   //Set the flag indicating that we pushed the button
			   //so we go into NAV mode the next time we push it
			   OTWDriver.pCockpitManager->mpIcp->SetICPFlag(ICPClass::MODE_A_A);

			   //If our A_G NAV mode flag is set, clear it so we get into
			   //A_G mode when we push that button
			   if(OTWDriver.pCockpitManager->mpIcp->IsICPSet(ICPClass::MODE_A_G))
				   OTWDriver.pCockpitManager->mpIcp->ClearICPFlag(ICPClass::MODE_A_G);
		   }
	   } 	
	}
}

//MI added for backup ICP Mode setting
void SimICPAA1(unsigned long, int state, void* pButton) 
{
	if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.playerEntity)->ejectTriggered) 
	{
		//MI 3/1/2002 if we're in DF or MRM, don't do anything
		if(SimDriver.playerEntity && SimDriver.playerEntity->FCC && (SimDriver.playerEntity->FCC->GetMasterMode() == 
			FireControlComputer::Dogfight || SimDriver.playerEntity->FCC->GetMasterMode() == FireControlComputer::MissileOverride))
			return;

		OTWDriver.pCockpitManager->mpIcp->HandleInput(AA_BUTTON, (CPButtonObject*)pButton);
		SimDriver.playerEntity->FCC->SetMasterMode(FireControlComputer::Gun);

		// Put the radar in the its default AA mode
		RadarClass* pradar = (RadarClass*) FindSensor (SimDriver.playerEntity, SensorClass::Radar);
		if (pradar)
			pradar->DefaultAAMode();

		// Configure the MFDs
		MfdDisplay[0]->SetNewMasterMode(SimDriver.playerEntity->FCC->GetMainMasterMode());
		MfdDisplay[1]->SetNewMasterMode(SimDriver.playerEntity->FCC->GetMainMasterMode());
	}
}

// =============================================//
// Callback Function CBEICPAG
// =============================================//

void SimICPAG(unsigned long, int state, void* pButton) 
{
	if (!SimDriver.playerEntity || !SimDriver.playerEntity->IsSetFlag( MOTION_OWNSHIP )) // 2002-02-15 ADDED BY S.G. Do it once here and removed the corresponding line from below
		return;

	if(!g_bRealisticAvionics)
	{
		//MI Original Code
	   if (state & KEY_DOWN && !((AircraftClass*)SimDriver.playerEntity)->ejectTriggered) {

			OTWDriver.pCockpitManager->mpIcp->HandleInput(AG_BUTTON, (CPButtonObject*)pButton);
			SimDriver.playerEntity->FCC->SetMasterMode(FireControlComputer::AirGroundBomb);

			// Put the radar in the its default AG mode
			RadarClass* pradar = (RadarClass*) FindSensor (SimDriver.playerEntity, SensorClass::Radar);
			if (pradar)
				pradar->DefaultAGMode();

			// Configure the MFDs
			MfdDisplay[0]->SetNewMode(MFDClass::FCRMode);
			MfdDisplay[1]->SetNewMode(MFDClass::SMSMode);
		}
	}
	else
	{
		//MI 3/1/2002 if we're in DF or MRM, don't do anything
		if(SimDriver.playerEntity->FCC && (SimDriver.playerEntity->FCC->GetMasterMode() == 
			FireControlComputer::Dogfight || SimDriver.playerEntity->FCC->GetMasterMode() == FireControlComputer::MissileOverride))
			return;

		if (SimDriver.playerEntity->Sms)	//JPO CTDfix	//Set our display mode
			SimDriver.playerEntity->Sms->drawable->SetDisplayMode(SmsDrawable::Wpn);
		//MI added/modified for ICP Stuff
		if (state & KEY_DOWN && !((AircraftClass*)SimDriver.playerEntity)->ejectTriggered) 
		{
		   //Have we pushed this button before?
		   if(OTWDriver.pCockpitManager->mpIcp->IsICPSet(ICPClass::MODE_A_G))
		   {
			   //Yes, we pushed it before. Let's go back into NAV mode
			   //Set our FCC into NAV mode
			   SimDriver.playerEntity->FCC->SetMasterMode(FireControlComputer::Nav);

			   //passes our puttonstate to the ICP
			   OTWDriver.pCockpitManager->mpIcp->HandleInput(NAV_BUTTON, (CPButtonObject*)pButton);
			   
			   // Put the radar in the its default AA mode
			   RadarClass* pradar = (RadarClass*) FindSensor (SimDriver.playerEntity, SensorClass::Radar);
			   if (pradar)
					pradar->DefaultAGMode();

			   // Configure the MFDs
			   		// Configure the MFDs
			   MfdDisplay[0]->SetNewMasterMode(SimDriver.playerEntity->FCC->GetMainMasterMode());
			   MfdDisplay[1]->SetNewMasterMode(SimDriver.playerEntity->FCC->GetMainMasterMode());


			   //Clear the flag indicating we are in A_G mode
			   //This is so we know that we have to go into A_G mode next time
			   OTWDriver.pCockpitManager->mpIcp->ClearICPFlag(ICPClass::MODE_A_G);

			   //If our A_A NAV mode flag is set, clear it so we get into
			   //A_A mode when we push that button
			   if(OTWDriver.pCockpitManager->mpIcp->IsICPSet(ICPClass::MODE_A_A))
				   OTWDriver.pCockpitManager->mpIcp->ClearICPFlag(ICPClass::MODE_A_A);

		   }
		   else if (SimDriver.playerEntity->Sms)
		   {
			   //This is the first time we press it, let's go into A_G mode
			   //Set our system into A_G mode
			   SimDriver.playerEntity->FCC->SetMasterMode(FireControlComputer::AirGroundBomb);
			   SimDriver.playerEntity->Sms->GetNextWeapon(wdGround);

			   //passes our puttonstate to the ICP
			   OTWDriver.pCockpitManager->mpIcp->HandleInput(AG_BUTTON, (CPButtonObject*)pButton);

			   // Put the radar in the its default AG mode
			   //Makes us go into NAV mode next time we push it.
			   RadarClass* pradar = (RadarClass*) FindSensor (SimDriver.playerEntity, SensorClass::Radar);
			   if (pradar)
					pradar->DefaultAGMode();

			   // Configure the MFDs
			   MfdDisplay[0]->SetNewMasterMode(SimDriver.playerEntity->FCC->GetMainMasterMode());
			   MfdDisplay[1]->SetNewMasterMode(SimDriver.playerEntity->FCC->GetMainMasterMode());

			   //Set the Flag that we've been here before
			   OTWDriver.pCockpitManager->mpIcp->SetICPFlag(ICPClass::MODE_A_G);

			   //If our A_A mode flag is set, clear it so we get into
			   //A_A mode when we push the A_A button
			   if(OTWDriver.pCockpitManager->mpIcp->IsICPSet(ICPClass::MODE_A_A))
				   OTWDriver.pCockpitManager->mpIcp->ClearICPFlag(ICPClass::MODE_A_A);
		   }
	   }
	}
}

//MI added for backup ICP Mode setting
void SimICPAG1(unsigned long, int state, void* pButton) 
{
	if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.playerEntity)->ejectTriggered && SimDriver.playerEntity->Sms) 
	{
		//MI 3/1/2002 if we're in DF or MRM, don't do anything
		if(SimDriver.playerEntity && SimDriver.playerEntity->FCC && (SimDriver.playerEntity->FCC->GetMasterMode() == 
			FireControlComputer::Dogfight || SimDriver.playerEntity->FCC->GetMasterMode() == FireControlComputer::MissileOverride))
			return;

		OTWDriver.pCockpitManager->mpIcp->HandleInput(AG_BUTTON, (CPButtonObject*)pButton);
		SimDriver.playerEntity->FCC->SetMasterMode(FireControlComputer::AirGroundBomb);
		SimDriver.playerEntity->Sms->GetNextWeapon(wdGround);

		// Put the radar in the its default AG mode
		RadarClass* pradar = (RadarClass*) FindSensor (SimDriver.playerEntity, SensorClass::Radar);
		if (pradar)
			pradar->DefaultAGMode();

		// Configure the MFDs
		MfdDisplay[0]->SetNewMasterMode(SimDriver.playerEntity->FCC->GetMainMasterMode());
		MfdDisplay[1]->SetNewMasterMode(SimDriver.playerEntity->FCC->GetMainMasterMode());
	}
}
// =============================================//
// Callback Function SimHUDScales
// =============================================//

void SimHUDScales(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		TheHud->CycleScalesSwitch();
	}
}

// =============================================//
// Callback Function SimHUDFPM
// =============================================//

void SimHUDFPM(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		TheHud->CycleFPMSwitch();
	}
}

// =============================================//
// Callback Function SimHUDDED
// =============================================//

void SimHUDDED(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		TheHud->CycleDEDSwitch();
	}
}

// =============================================//
// Callback Function 
// =============================================//

void SimHUDVelocity(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		TheHud->CycleVelocitySwitch();
	}
}

// =============================================//
// Callback Function SimHUDRadar
// =============================================//

void SimHUDRadar(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		TheHud->CycleRadarSwitch();
	}
}

// =============================================//
// Callback Function SimHUDBrightness
// =============================================//

void SimHUDBrightness(unsigned long, int state, void*) {
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		TheHud->CycleBrightnessSwitch();
}

//MI
void SimHUDBrightnessUp(unsigned long, int state, void*) 
{
	if(!g_bRealisticAvionics)
		return;
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		TheHud->CycleBrightnessSwitchUp();
}
void SimHUDBrightnessDown(unsigned long, int state, void*) 
{
	if(!g_bRealisticAvionics)
		return;
   if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) // 2002-02-15 MODIFIED BY S.G. Any cockpit stuff is valid only if the player hasn't ejected...
		TheHud->CycleBrightnessSwitchDown();
}

// =============================================//
// Callback Function ExtinguishMasterCaution
// =============================================//

void ExtinguishMasterCaution(unsigned long, int state, void*) {
	
	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(SimDriver.playerEntity->mFaults->MasterCaution() == TRUE) {
			SimDriver.playerEntity->mFaults->ClearMasterCaution();
		}
		else {
			OTWDriver.pCockpitManager->mMiscStates.SetMasterCautionEvent();
		}
	}

}


void SimCycleLeftAuxComDigit(unsigned long, int state, void*) {
	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		gNavigationSys->StepTacanChannelDigit(NavigationSystem::AUXCOMM, 2);
	}
}

void SimDecLeftAuxComDigit(unsigned long, int state, void*) {
	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		gNavigationSys->StepTacanChannelDigit(NavigationSystem::AUXCOMM, 2, -1);
	}
}


void SimCycleCenterAuxComDigit(unsigned long, int state, void*) {
	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		gNavigationSys->StepTacanChannelDigit(NavigationSystem::AUXCOMM, 1);
	}
}

void SimDecCenterAuxComDigit(unsigned long, int state, void*) {
	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		gNavigationSys->StepTacanChannelDigit(NavigationSystem::AUXCOMM, 1, -1);
	}
}

void SimCycleRightAuxComDigit(unsigned long, int state, void*) {
	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		gNavigationSys->StepTacanChannelDigit(NavigationSystem::AUXCOMM, 0);
	}
}
void SimDecRightAuxComDigit(unsigned long, int state, void*) {
	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		gNavigationSys->StepTacanChannelDigit(NavigationSystem::AUXCOMM, 0, -1);
	}
}
void SimCycleBandAuxComDigit(unsigned long, int state, void*) {
	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		gNavigationSys->StepTacanBand(NavigationSystem::AUXCOMM);
	}
}

void SimToggleAuxComMaster(unsigned long, int state, void*) {
	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		gNavigationSys->ToggleControlSrc();
	}
}

void SimToggleAuxComAATR(unsigned long, int state, void*) {
	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		gNavigationSys->ToggleDomain(NavigationSystem::AUXCOMM);
	}
}

void SimToggleUHFMaster(unsigned long, int state, void*) {
	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		gNavigationSys->ToggleUHFSrc();
	}
}

#include <dvoice.h>
extern IDirectPlayVoiceClient* g_pVoiceClient;
void SimTransmitCom1(unsigned long val, int state, void *)
{
	    HRESULT             hr = S_OK;
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
	Transmit(1);
	}
	else
	{
	Transmit(0);
	}
}
void SimTransmitCom2(unsigned long val, int state, void *)
{
	    HRESULT             hr = S_OK;
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
	Transmit(2);
	}
	else
	{
	Transmit(0);
	}
}



void SimToggleExtLights(unsigned long, int state, void*) {
// JPO changes 
    // only applies if complex aircraft and also sets the extra status flags.
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN) && SimDriver.playerEntity->IsComplex()) 
	{
		AircraftClass *ac = SimDriver.playerEntity;
		if (ac->Status() & STATUS_EXT_LIGHTS) { // lights are on
			ac->SetSwitch(COMP_TAIL_STROBE, FALSE);
			ac->SetSwitch(COMP_NAV_LIGHTS, FALSE);
			ac->ClearStatusBit (STATUS_EXT_LIGHTS|STATUS_EXT_NAVLIGHTS|STATUS_EXT_TAILSTROBE);
		}
		else {
			ac->SetSwitch(COMP_TAIL_STROBE, TRUE);
			ac->SetSwitch(COMP_NAV_LIGHTS, TRUE);
			ac->SetStatusBit (STATUS_EXT_LIGHTS|STATUS_EXT_NAVLIGHTS|STATUS_EXT_TAILSTROBE);
		}
    }
}
// =============================================//

void IncreaseAlow(unsigned long, int state , void*) {
	//MI
	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(TheHud && TheHud->lowAltWarning >= 1000.0F) {
			TheHud->lowAltWarning += 1000.0F;
			TheHud->lowAltWarning = min(max(0.0F, TheHud->lowAltWarning), 99999.0F);
		}
		else if(TheHud){
			TheHud->lowAltWarning += 100.0F;
			TheHud->lowAltWarning = min(max(0.0F, TheHud->lowAltWarning), 99999.0F);
		}
	}
}


void DecreaseAlow(unsigned long, int state, void*) {
	//MI
	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(TheHud && TheHud->lowAltWarning > 1000.0F) {
			TheHud->lowAltWarning -= 1000.0F;
			TheHud->lowAltWarning = min(max(0.0F, TheHud->lowAltWarning), 99999.0F);
		}
		else if(TheHud){
			TheHud->lowAltWarning -= 100.0F;
			TheHud->lowAltWarning = min(max(0.0F, TheHud->lowAltWarning), 99999.0F);
		}
	}
}

void SaveCockpitDefaults(unsigned long, int state, void*)
{
   if (OTWDriver.pCockpitManager && (state & KEY_DOWN))
   {
      OTWDriver.pCockpitManager->SaveCockpitDefaults();
   }
}

void LoadCockpitDefaults(unsigned long, int state, void*)
{
   if (OTWDriver.pCockpitManager && (state & KEY_DOWN))
   {
      OTWDriver.pCockpitManager->LoadCockpitDefaults();
   }
}

// JB 000509
void SimSetBubbleSize (unsigned long val, int state, void*)
{
   if (state & KEY_DOWN && !(gCommsMgr && gCommsMgr->Online()))
      FalconLocalSession->SetBubbleRatio(.5f + float(val - DIK_1) / 2.0f);
}
// JB 000509


// JPO - allow throttle to go past idle detent setting
void SimThrottleIdleDetent(unsigned long, int state, void*)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		// throttle to off?
		if (SimDriver.playerEntity->af->Throtl() < 0.1f) {
			SimDriver.playerEntity->af->SetFlag(AirframeClass::EngineStopped);
		}
		else if (SimDriver.playerEntity->af->rpm >= 0.20f) {
			// engine light 
			SimDriver.playerEntity->af->ClearFlag(AirframeClass::EngineStopped);
			SimDriver.playerEntity->mFaults->ClearFault(FaultClass::eng_fault, FaultClass::fl_out);
		}
    }
}

// set Jfs to position
void SimJfsStart(unsigned long, int state, void*)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		// if jfs allready running, clear it.
		if (SimDriver.playerEntity->af->IsSet(AirframeClass::JfsStart)) {
			SimDriver.playerEntity->af->ClearFlag(AirframeClass::JfsStart);
		}
		// otherwise - if throttle is at idle, and accumulators charged, attempt JFS start
		else if (SimDriver.playerEntity->af->Throtl() < 0.1f) {
			SimDriver.playerEntity->af->JfsEngineStart ();
		}
    }
}

// step the EPU setting
void SimEpuToggle (unsigned long, int state, void*)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		// toggle EPU off/auto/on.
		SimDriver.playerEntity->af->StepEpuSwitch ();
    }
}

// JPO: TRIM command
void AFRudderTrimLeft (unsigned long, int state, void*)
{
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 ADDED BY S.G. Moved outside the two if statement
		//MI additions
		if(g_bRealisticAvionics)
		{
			if(SimDriver.playerEntity->TrimAPDisc)
				return;
		}

		if (state & KEY_DOWN)
			pitchRudderTrimRate = -0.25F;
		else
			pitchRudderTrimRate = 0.0F;
	}
}

void AFRudderTrimRight (unsigned long, int state, void*)
{
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 ADDED BY S.G. Moved outside the two if statement
	//MI additions
		if(g_bRealisticAvionics)
		{
			if(SimDriver.playerEntity->TrimAPDisc)
				return;
		}

		if (state & KEY_DOWN)
			pitchRudderTrimRate = 0.25F;
		else
			pitchRudderTrimRate = 0.0F;
	}
}

void AFAileronTrimLeft (unsigned long, int state, void*)
{
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 ADDED BY S.G. Moved outside the two if statement
	//MI additions
		if(g_bRealisticAvionics)
		{
			if(SimDriver.playerEntity->TrimAPDisc)
				return;
		}

		if (state & KEY_DOWN)
			pitchAileronTrimRate = -0.25F;
		else
			pitchAileronTrimRate = 0.0F;
	}
}

void AFAileronTrimRight (unsigned long, int state, void*)
{
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 ADDED BY S.G. Moved outside the two if statement
		//MI additions
		if(g_bRealisticAvionics)
		{
			if(SimDriver.playerEntity && SimDriver.playerEntity->TrimAPDisc)
				return;
		}

		if (state & KEY_DOWN)
			pitchAileronTrimRate = 0.25F;
		else
			pitchAileronTrimRate = 0.0F;
	}
}

void AFElevatorTrimUp (unsigned long, int state, void*)
{
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 ADDED BY S.G. Moved outside the two if statement
		//MI additions
		if(g_bRealisticAvionics)
		{
			if(SimDriver.playerEntity && SimDriver.playerEntity->TrimAPDisc)
				return;
		}

		if (state & KEY_DOWN)
			pitchElevatorTrimRate = -0.25F;
		else
			pitchElevatorTrimRate = 0.0F;
	}
}

void AFElevatorTrimDown (unsigned long, int state, void*)
{
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)) { // 2002-02-15 ADDED BY S.G. Moved outside the two if statement
	//MI additions
		if(g_bRealisticAvionics)
		{
			if(SimDriver.playerEntity && SimDriver.playerEntity->TrimAPDisc)
				return;
		}

		if (state & KEY_DOWN)
			pitchElevatorTrimRate = 0.25F;
		else
			pitchElevatorTrimRate = 0.0F;
	}
}

void AFAlternateGear (unsigned long, int state, void*)
{
	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
	{
		if (state & KEY_DOWN) {
			SimDriver.playerEntity->af->gearHandle = 1.0F;
			SimDriver.playerEntity->af->altGearDeployed = true;
		}
	}
}

void AFAlternateGearReset (unsigned long, int state, void*)
{
	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
	{
		if (state & KEY_DOWN) {
			SimDriver.playerEntity->af->altGearDeployed = false;
		}
	}
}

void AFResetTrim (unsigned long, int state, void*)
{
	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
	{
		if (state & KEY_DOWN) {
			UserStickInputs.Reset();
		}
	}
}

//MI ICP Stuff
void SimICPIFF(unsigned long, int state, void* pButton)
{
	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(g_bRealisticAvionics)
		{ 
			//Player pushed the button, tell it to our ICP class
			OTWDriver.pCockpitManager->mpIcp->HandleInput(IFF_BUTTON, (CPButtonObject*)pButton);
		}
	}		
}

void SimICPLIST(unsigned long, int state, void* pButton)
{
	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(g_bRealisticAvionics)
		{ 
			//Player pushed the button, tell it to our ICP class
			OTWDriver.pCockpitManager->mpIcp->HandleInput(LIST_BUTTON, (CPButtonObject*)pButton);
		}
	}		
}

void SimICPTHREE(unsigned long, int state, void* pButton)
{
	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(g_bRealisticAvionics)
		{ 
			//Player pushed the button, tell it to our ICP class
			OTWDriver.pCockpitManager->mpIcp->HandleInput(THREE_BUTTON, (CPButtonObject*)pButton);
		}
	}		
}

void SimICPSIX(unsigned long, int state, void* pButton)
{
	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(g_bRealisticAvionics)
		{ 
			//Player pushed the button, tell it to our ICP class
			OTWDriver.pCockpitManager->mpIcp->HandleInput(SIX_BUTTON, (CPButtonObject*)pButton);
		}
		else
		{
			if (state & KEY_DOWN && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag( MOTION_OWNSHIP ) && !((AircraftClass*)SimDriver.playerEntity)->ejectTriggered) 
			{
				OTWDriver.pCockpitManager->mpIcp->HandleInput(DLINK_BUTTON, (CPButtonObject*)pButton);
			}
		}
	}		
}

void SimICPEIGHT(unsigned long, int state, void* pButton)
{
	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(g_bRealisticAvionics)
		{ 
			//Player pushed the button, tell it to our ICP class
			OTWDriver.pCockpitManager->mpIcp->HandleInput(EIGHT_BUTTON, (CPButtonObject*)pButton);
		}
	}		
}

void SimICPNINE(unsigned long, int state, void* pButton)
{
	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(g_bRealisticAvionics)
		{ 
			//Player pushed the button, tell it to our ICP class
			OTWDriver.pCockpitManager->mpIcp->HandleInput(NINE_BUTTON, (CPButtonObject*)pButton);
		}
	}		
}

void SimICPZERO(unsigned long, int state, void* pButton)
{
	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(g_bRealisticAvionics)
		{ 
			//Player pushed the button, tell it to our ICP class
			OTWDriver.pCockpitManager->mpIcp->HandleInput(ZERO_BUTTON, (CPButtonObject*)pButton);
		}
	}		
}

void SimICPResetDED(unsigned long, int state, void* pButton)
{
	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(g_bRealisticAvionics)
		{ 
			//Player pushed the button, tell it to our ICP class
			OTWDriver.pCockpitManager->mpIcp->HandleInput(CNI_MODE, (CPButtonObject*)pButton);
		}
	}		
}

void SimICPDEDUP(unsigned long, int state, void* pButton)
{
	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(g_bRealisticAvionics)
		{ 
			//Player pushed the button, tell it to our ICP class
			OTWDriver.pCockpitManager->mpIcp->HandleInput(UP_BUTTON, (CPButtonObject*)pButton);
		}
	}		
}

void SimICPDEDDOWN(unsigned long, int state, void* pButton)
{
	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(g_bRealisticAvionics)
		{ 
			//Player pushed the button, tell it to our ICP class
			OTWDriver.pCockpitManager->mpIcp->HandleInput(DOWN_BUTTON, (CPButtonObject*)pButton);
		}
	}		
}

void SimICPDEDSEQ(unsigned long, int state, void* pButton)
{
	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(g_bRealisticAvionics)
		{ 
			//Player pushed the button, tell it to our ICP class
			OTWDriver.pCockpitManager->mpIcp->HandleInput(SEQ_BUTTON, (CPButtonObject*)pButton);
		}
	}
}

void SimICPCLEAR(unsigned long, int state, void* pButton)
{
	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(g_bRealisticAvionics)
		{ 
			//Player pushed the button, tell it to our ICP class
			OTWDriver.pCockpitManager->mpIcp->HandleInput(CLEAR_BUTTON, (CPButtonObject*)pButton);
		}
	}		
}

void SimRALTSTDBY(unsigned long, int state, void* pButton)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		SimDriver.playerEntity->af->platform->RaltStdby();
    }
}

void SimRALTON(unsigned long, int state, void* pButton)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		//Set it on here 
		SimDriver.playerEntity->af->platform->RaltOn();
    }
}

void SimRALTOFF(unsigned long, int state, void* pButton)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		//Set it off here
		SimDriver.playerEntity->af->platform->RaltOff();
		//take the juice
		SimDriver.playerEntity->PowerOff(AircraftClass::RaltPower);
    }
}

void SimFLIRToggle(unsigned long, int state, void* pButton)
{
    if (theLantirn && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		theLantirn -> ToggleFLIR();
    }
}

void SimSMSPower(unsigned long val, int state, void *)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		SimDriver.playerEntity->PowerToggle (AircraftClass::SMSPower);

}

void SimFCCPower(unsigned long val, int state, void *)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		SimDriver.playerEntity->PowerToggle (AircraftClass::FCCPower);
}

void SimMFDPower(unsigned long val, int state, void *)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		SimDriver.playerEntity->PowerToggle (AircraftClass::MFDPower);
}

void SimUFCPower(unsigned long val, int state, void *)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		SimDriver.playerEntity->PowerToggle (AircraftClass::UFCPower);
}

void SimGPSPower(unsigned long val, int state, void *)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		SimDriver.playerEntity->PowerToggle (AircraftClass::GPSPower);
}

void SimDLPower(unsigned long val, int state, void *)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		SimDriver.playerEntity->PowerToggle (AircraftClass::DLPower);
}

void SimMAPPower(unsigned long val, int state, void *)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		SimDriver.playerEntity->PowerToggle (AircraftClass::MAPPower);
}

void SimRightHptPower(unsigned long val, int state, void *)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.playerEntity->PowerToggle (AircraftClass::RightHptPower);
		//MI
		if(g_bRealisticAvionics)
		{
			if(SimDriver.playerEntity->FCC)
			{
				if(SimDriver.playerEntity->HasPower(AircraftClass::RightHptPower))
				{
					SimDriver.playerEntity->FCC->LaserFire = FALSE;
					SimDriver.playerEntity->FCC->InhibitFire = FALSE;
				}
				else
				{
					SimDriver.playerEntity->FCC->LaserFire = FALSE;
					SimDriver.playerEntity->FCC->InhibitFire = TRUE;
				}
			}
		}
	}
}

void SimLeftHptPower(unsigned long val, int state, void *)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		SimDriver.playerEntity->PowerToggle (AircraftClass::LeftHptPower);
}

void SimTISLPower(unsigned long val, int state, void *)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		SimDriver.playerEntity->PowerToggle (AircraftClass::TISLPower);
}

void SimFCRPower(unsigned long val, int state, void *)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		SimDriver.playerEntity->PowerToggle (AircraftClass::FCRPower);
}

void SimHUDPower(unsigned long val, int state, void *)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		SimDriver.playerEntity->PowerToggle (AircraftClass::HUDPower);
}

void SimToggleRealisticAvionics(unsigned long val, int state, void *)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) // 2002-02-10 MODIFIED BY S.G. Removed ; at the end of the if statement
		g_bRealisticAvionics = !g_bRealisticAvionics;
}

void SimIncFuelSwitch(unsigned long val, int state, void *)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		SimDriver.playerEntity->af->IncFuelSwitch();
}

void SimDecFuelSwitch(unsigned long val, int state, void *)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		SimDriver.playerEntity->af->DecFuelSwitch();
}

void SimIncFuelPump(unsigned long val, int state, void *)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		SimDriver.playerEntity->af->IncFuelPump();
}

void SimDecFuelPump(unsigned long val, int state, void *)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		SimDriver.playerEntity->af->DecFuelPump();
}

void SimToggleMasterFuel(unsigned long val, int state, void *)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		SimDriver.playerEntity->af->ToggleEngineFlag(AirframeClass::MasterFuelOff);
}

void SimIncAirSource(unsigned long val, int state, void *)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		SimDriver.playerEntity->af->IncAirSource();
}

void SimDecAirSource(unsigned long val, int state, void *)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		SimDriver.playerEntity->af->DecAirSource();
}
void SimLandingLightToggle(unsigned long val, int state, void *)
{
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		SimDriver.playerEntity->af->ToggleLL();
}
void SimParkingBrakeToggle(unsigned long val, int state, void *)
{
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		SimDriver.playerEntity->af->TogglePB();
}
// JB carrier start
void SimHookToggle(unsigned long val, int state, void *)
{
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN) 
		//&& SimDriver.playerEntity->mFaults->GetFault( FaultClass::hook_fault )
		)
	{
		SimDriver.playerEntity->af->ToggleHook();
	}
}
// JB carrier end
void SimLaserArmToggle(unsigned long val, int state, void *)
{
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(SimDriver.playerEntity->FCC)
			SimDriver.playerEntity->FCC->ToggleLaserArm();
	}
}
void SimFuelDoorToggle(unsigned long val, int state, void *)
{
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
		SimDriver.playerEntity->af->ToggleEngineFlag(AirframeClass::FuelDoorOpen);
}
//Autopilot
//Right switch. Three positions. ALT Hold, OFF and ATT Hold
//ALT Hold holds ALT, and left switch controls what we hold.
void SimRightAPSwitch(unsigned long val, int state, void *)
{
	//This is the right switch, in the upper position.
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(g_bRealisticAvionics)
		{
			//We get Altitude hold now.?
			if(SimDriver.playerEntity->IsOn(AircraftClass::AltHold))	//up
			{
				//from Alt hold to Att hold
				SimDriver.playerEntity->ClearAPFlag(AircraftClass::AltHold);
				//can't be both
				SimDriver.playerEntity->SetAPFlag(AircraftClass::AttHold);	//down
				SimDriver.playerEntity->SetNewPitch();
				SimDriver.playerEntity->SetNewRoll();
			}
			else if(SimDriver.playerEntity->IsOn(AircraftClass::AttHold))
			{
				//all off
				SimDriver.playerEntity->ClearAPFlag(AircraftClass::AltHold);
				SimDriver.playerEntity->ClearAPFlag(AircraftClass::AttHold);
				//take the juice
				SimDriver.playerEntity->PowerOff(AircraftClass::APPower);
			}
			else
			{
				//from off to Alt Hold
				SimDriver.playerEntity->SetAPFlag(AircraftClass::AltHold);
				SimDriver.playerEntity->ClearAPFlag(AircraftClass::AttHold);
				//tell us what we should hold
				SimDriver.playerEntity->SetNewAlt();
			}
			//Tell us what we want
			SimDriver.playerEntity->SetAPParameters();
		}
	}
}
//Left AP Switch
void SimLeftAPSwitch(unsigned long val, int state, void *)
{
	//This is the right switch, in the upper position.
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		//Middle position
		if(SimDriver.playerEntity->IsOn(AircraftClass::RollHold))
		{
			SimDriver.playerEntity->ClearAPFlag(AircraftClass::RollHold);
			SimDriver.playerEntity->ClearAPFlag(AircraftClass::HDGSel);
			SimDriver.playerEntity->SetAPFlag(AircraftClass::StrgSel);	//waypoint AP
		}
		//down position
		else if(SimDriver.playerEntity->IsOn(AircraftClass::StrgSel))
		{
			SimDriver.playerEntity->ClearAPFlag(AircraftClass::StrgSel);
			SimDriver.playerEntity->ClearAPFlag(AircraftClass::RollHold);
			SimDriver.playerEntity->SetAPFlag(AircraftClass::HDGSel);
		}
		//up position
		else
		{
			SimDriver.playerEntity->ClearAPFlag(AircraftClass::HDGSel);
			SimDriver.playerEntity->ClearAPFlag(AircraftClass::StrgSel);
			SimDriver.playerEntity->SetAPFlag(AircraftClass::RollHold);
		}
		if(SimDriver.playerEntity->IsOn(AircraftClass::AttHold))
		{
			if(SimDriver.playerEntity->IsOn(AircraftClass::RollHold))
			{
				SimDriver.playerEntity->SetNewRoll();
				SimDriver.playerEntity->SetNewPitch();
			}
		}
	}
}
void SimAPOverride(unsigned long val, int state, void *)
{
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
	{
		if(state & KEY_DOWN)
		{
		    // JPO - save AP type.
			//MI only if it's != APOFF
			if(SimDriver.playerEntity->AutopilotType() != AircraftClass::APOff)
				SimDriver.playerEntity->lastapType = SimDriver.playerEntity->AutopilotType();
			if(SimDriver.playerEntity->autopilotType != AircraftClass::APOff)
				SimDriver.playerEntity->SetAutopilot(AircraftClass::APOff);
		    SimDriver.playerEntity->SetAPFlag(AircraftClass::Override);
		}
		else
		{
			SimDriver.playerEntity->SetAutopilot(SimDriver.playerEntity->lastapType);
		    SimDriver.playerEntity->ClearAPFlag(AircraftClass::Override);
		    if (SimDriver.playerEntity->lastapType != AircraftClass::LantirnAP) { // JPO - lantirn stays
			SimDriver.playerEntity->SetAPParameters();
			if(SimDriver.playerEntity->IsOn(AircraftClass::RollHold))
			    SimDriver.playerEntity->SetNewRoll();
			if(SimDriver.playerEntity->IsOn(AircraftClass::AttHold))
			    SimDriver.playerEntity->SetNewPitch();
			if(SimDriver.playerEntity->IsOn(AircraftClass::AltHold))
			    SimDriver.playerEntity->SetNewAlt();
		    }
		}
	}
}

void SimToggleTFR (unsigned long val, int state, void *)
{
    if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		if (SimDriver.playerEntity->AutopilotType() == AircraftClass::APOff) {
			if (theLantirn->GetTFRMode() == LantirnClass::TFR_STBY)
			theLantirn->SetTFRMode(LantirnClass::TFR_NORM);
			SimDriver.playerEntity->SetAutopilot(AircraftClass::LantirnAP);
			//MI turn off any other AP modes
			SimDriver.playerEntity->ClearAPFlag(AircraftClass::AltHold);
			SimDriver.playerEntity->ClearAPFlag(AircraftClass::AttHold);
			//MI update our lastAP state, for operating the RF Switch
			SimDriver.playerEntity->lastapType = AircraftClass::LantirnAP;
		}
		else 
		{
			SimDriver.playerEntity->SetAutopilot(AircraftClass::APOff);
			SimDriver.playerEntity->lastapType = AircraftClass::APOff;
		}
    }
}
void SimWarnReset(unsigned long val, int state, void *)
{
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.playerEntity->mFaults->ClearWarnReset();
		SimDriver.playerEntity->mFaults->SetManWarnReset();
		TheHud->ResetMaxG();
	}
}
void SimReticleSwitch(unsigned long val, int state, void *)
{
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(TheHud->WhichMode == 0)	//off
			TheHud->WhichMode = 1;	//normal
		else if(TheHud->WhichMode == 1)	//normal
			TheHud->WhichMode = 2;	//backup
		else
			TheHud->WhichMode = 0;	//off
	}
}


void SimInteriorLight(unsigned long val, int state, void *)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		switch (SimDriver.playerEntity->GetInteriorLight()) {
		default:
		case AircraftClass::LT_OFF:
			SimDriver.playerEntity->SetInteriorLight(AircraftClass::LT_LOW);
			SimDriver.playerEntity->PowerOn(AircraftClass::InteriorLightPower);
			break;
		case AircraftClass::LT_LOW:
			SimDriver.playerEntity->SetInteriorLight(AircraftClass::LT_NORMAL);
			SimDriver.playerEntity->PowerOn(AircraftClass::InteriorLightPower);
			break;
		case AircraftClass::LT_NORMAL:
			SimDriver.playerEntity->SetInteriorLight(AircraftClass::LT_OFF);
			SimDriver.playerEntity->PowerOff(AircraftClass::InteriorLightPower);
			break;
		}
    }
}

void SimInstrumentLight(unsigned long val, int state, void *)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		switch (SimDriver.playerEntity->GetInstrumentLight()) {
		default:
		case AircraftClass::LT_OFF:
			SimDriver.playerEntity->SetInstrumentLight(AircraftClass::LT_LOW);
			SimDriver.playerEntity->PowerOn(AircraftClass::InstrumentLightPower);
			break;
		case AircraftClass::LT_LOW:
			SimDriver.playerEntity->SetInstrumentLight(AircraftClass::LT_NORMAL);
			SimDriver.playerEntity->PowerOn(AircraftClass::InstrumentLightPower);
			break;
		case AircraftClass::LT_NORMAL:
			SimDriver.playerEntity->SetInstrumentLight(AircraftClass::LT_OFF);
			SimDriver.playerEntity->PowerOff(AircraftClass::InstrumentLightPower);
			break;
		}
    }
}
void SimSpotLight(unsigned long val, int state, void *)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)) {
		switch (SimDriver.playerEntity->GetSpotLight()) {
		default:
		case AircraftClass::LT_OFF:
			SimDriver.playerEntity->SetSpotLight(AircraftClass::LT_LOW);
			SimDriver.playerEntity->PowerOn(AircraftClass::SpotLightPower);
			break;
		case AircraftClass::LT_LOW:
			SimDriver.playerEntity->SetSpotLight(AircraftClass::LT_NORMAL);
			SimDriver.playerEntity->PowerOn(AircraftClass::SpotLightPower);
			break;
		case AircraftClass::LT_NORMAL:
			SimDriver.playerEntity->SetSpotLight(AircraftClass::LT_OFF);
			SimDriver.playerEntity->PowerOff(AircraftClass::SpotLightPower);
			break;
		}
    }
}
//MI TMS switch
void SimTMSUp(unsigned long val, int state, void *)
{
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
	{
		if(state & KEY_DOWN)
		{
			if(g_bRealisticAvionics)
			{
				RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);
				bool HasMavs = FALSE;
				LaserPodClass* laserPod = (LaserPodClass* )FindLaserPod (SimDriver.playerEntity);
				MaverickDisplayClass* mavDisplay = NULL;
				HarmTargetingPod *theHTS = (HarmTargetingPod*)FindSensor(SimDriver.playerEntity, SensorClass::HTS);
				if(SimDriver.playerEntity->Sms->curWeaponType == wtAgm65 && SimDriver.playerEntity->Sms->curWeapon)
				{
					HasMavs = TRUE;
					ShiAssert(SimDriver.playerEntity->Sms->curWeapon->IsMissile());
					mavDisplay = (MaverickDisplayClass*)((MissileClass*)SimDriver.playerEntity->Sms->curWeapon)->display;
				}
				
				if(SimDriver.playerEntity->FCC && SimDriver.playerEntity->FCC->IsSOI)
				{
					SimDriver.playerEntity->FCC->HSDDesignate = 1;
					return;
				}
				else if(theRadar && theRadar->IsSOI())
				{
					//ACM Modes
					if(theRadar->GetRadarMode() == RadarClass::ACM_30x20 ||
						theRadar->GetRadarMode() == RadarClass::ACM_SLEW ||
						theRadar->GetRadarMode() == RadarClass::ACM_BORE ||
						theRadar->GetRadarMode() == RadarClass::ACM_10x60)
					{
						SimDriver.playerEntity->FCC->dropTrackCmd = TRUE;
						theRadar->SelectACMBore();
					}
					else if(theRadar->GetRadarMode() == RadarClass::RWS ||
					        theRadar->GetRadarMode() == RadarClass::LRS ||
							theRadar->GetRadarMode() == RadarClass::VS ||
						theRadar->GetRadarMode() == RadarClass::TWS ||
						theRadar->GetRadarMode() == RadarClass::SAM ||
						theRadar->IsAG())
					{
						SimDriver.playerEntity->FCC->designateCmd = TRUE;
					}
				}
				else if(TheHud && TheHud->IsSOI())
					SimDriver.playerEntity->FCC->designateCmd = TRUE;
				else if(HasMavs && mavDisplay && mavDisplay->IsSOI())
				{
					if(SimDriver.playerEntity->Sms->curWeapon &&
						!((MissileClass*)SimDriver.playerEntity->Sms->curWeapon)->Covered &&
						SimDriver.playerEntity->Sms->MavCoolTimer < 0.0F)
					{
						SimDriver.playerEntity->FCC->designateCmd = TRUE;
					}
				}
				else if(laserPod && laserPod->IsSOI())
				{
					if(SimDriver.playerEntity->FCC->preDesignate)
					{
						SimDriver.playerEntity->FCC->SetLastDesignate();
						SimDriver.playerEntity->FCC->preDesignate = FALSE;
					}
					SimDriver.playerEntity->FCC->designateCmd = TRUE;
				}
				else if(theHTS)
					SimDriver.playerEntity->FCC->designateCmd = TRUE;
			}
		}
		else
		{
			SimDriver.playerEntity->FCC->designateCmd = FALSE;
			SimDriver.playerEntity->FCC->dropTrackCmd = FALSE;
			SimDriver.playerEntity->FCC->HSDDesignate = 0;
		}
	}
} 
void SimTMSLeft(unsigned long val, int state, void *)
{
	bool HasMavs;
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
	{ 
		if(state & KEY_DOWN)
		{
			if(g_bRealisticAvionics && !g_bMLU)
			{
				RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);
				LaserPodClass *laserPod = (LaserPodClass* )FindLaserPod (SimDriver.playerEntity);
				MaverickDisplayClass* mavDisplay = NULL;
				if(SimDriver.playerEntity->Sms->curWeaponType == wtAgm65 && SimDriver.playerEntity->Sms->curWeapon)
				{
					HasMavs = TRUE;
					ShiAssert(SimDriver.playerEntity->Sms->curWeapon->IsMissile());
					mavDisplay = (MaverickDisplayClass*)((MissileClass*)SimDriver.playerEntity->Sms->curWeapon)->display;
				}
				if(theRadar && theRadar->IsSOI())
				{
					if(theRadar->DrawRCR)
						theRadar->DrawRCR = FALSE;
					else
						theRadar->DrawRCR = TRUE;
				}
				//laserpod, toggle BHOT
				if(laserPod && laserPod->IsSOI())
					laserPod->TogglePolarity();
				else if(mavDisplay && mavDisplay->IsSOI())
					((MissileClass*)SimDriver.playerEntity->Sms->curWeapon)->HOC = !((MissileClass*)SimDriver.playerEntity->Sms->curWeapon)->HOC;
			}
			else if(g_bRealisticAvionics && g_bMLU)
				SimIFFIn(0, KEY_DOWN, NULL);
		}
		else if(g_bRealisticAvionics && g_bMLU)
			SimIFFIn(0, 0, NULL);
	}
}
void SimTMSDown(unsigned long val, int state, void *)
{
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
	{
		if(state & KEY_DOWN)
		{
			if(g_bRealisticAvionics)
			{
				RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);
				if(theRadar && theRadar->IsSOI())
				{
					//ACM Modes
					if(theRadar->GetRadarMode() == RadarClass::ACM_30x20 ||
						theRadar->GetRadarMode() == RadarClass::ACM_SLEW ||
						theRadar->GetRadarMode() == RadarClass::ACM_BORE ||
						theRadar->GetRadarMode() == RadarClass::ACM_10x60)
					{
						//First, drop our track
						SimDriver.playerEntity->FCC->dropTrackCmd = TRUE;
						theRadar->SelectACMVertical();
					}
					else if(theRadar->GetRadarMode() == RadarClass::TWS)
					{
						if(theRadar->CurrentTarget())
						{
							//First, drop our track
							SimDriver.playerEntity->FCC->dropTrackCmd = TRUE;
						}
						else if(!theRadar->CurrentTarget())
						{
							theRadar->SelectRWS();
						}
					}
					else if(theRadar->GetRadarMode() == RadarClass::SAM && theRadar->IsSet(RadarDopplerClass::STTingTarget))
						theRadar->SelectSAM();
					else
						SimDriver.playerEntity->FCC->dropTrackCmd = TRUE;
				}
				if(SimDriver.playerEntity->FCC && SimDriver.playerEntity->FCC->IsSOI)
				{
					SimDriver.playerEntity->FCC->HSDDesignate = -1;
					return;
				}
				else
					SimDriver.playerEntity->FCC->dropTrackCmd = TRUE;
			}
		}
		else
		{
			SimDriver.playerEntity->FCC->dropTrackCmd = FALSE;
			SimDriver.playerEntity->FCC->HSDDesignate = 0;
		}
	}
}
void SimTMSRight(unsigned long val, int state, void *)
{
static VU_TIME tmstimer = 0;
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
	{
		if(g_bMLU)
		{
			if(state & KEY_DOWN && (!g_bMLU || !tmstimer || SimLibElapsedTime - tmstimer < 500))
			{
				if (!tmstimer)  tmstimer = SimLibElapsedTime;
			}
			else
			{
				if (SimLibElapsedTime - tmstimer < 500 || !g_bMLU)
				{
					if(g_bRealisticAvionics)
					{
						RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);
						if(theRadar)
						{
							//ACM Modes
							if(theRadar->GetRadarMode() == RadarClass::ACM_30x20 ||
								theRadar->GetRadarMode() == RadarClass::ACM_SLEW ||
								theRadar->GetRadarMode() == RadarClass::ACM_BORE ||
								theRadar->GetRadarMode() == RadarClass::ACM_10x60)
							{
								SimDriver.playerEntity->FCC->dropTrackCmd = TRUE;
								theRadar->SelectACM30x20();
							}
							else if(theRadar->GetRadarMode() == RadarClass::RWS ||
									theRadar->GetRadarMode() == RadarClass::LRS ||
									theRadar->GetRadarMode() == RadarClass::VS ||
									theRadar->GetRadarMode() == RadarClass::SAM)
								theRadar->SelectTWS();
							else if(theRadar->GetRadarMode() == RadarClass::TWS)
								theRadar->NextTarget();
						}
					}
				}
				else 
				{
						RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);
						if(theRadar)
						{
							//ACM Modes
							if(theRadar->GetRadarMode() == RadarClass::ACM_30x20 ||
								theRadar->GetRadarMode() == RadarClass::ACM_SLEW ||
								theRadar->GetRadarMode() == RadarClass::ACM_BORE ||
								theRadar->GetRadarMode() == RadarClass::ACM_10x60)
							{
								SimDriver.playerEntity->FCC->dropTrackCmd = TRUE;
								theRadar->SelectACM30x20();
							}
							else if(theRadar->GetRadarMode() == RadarClass::RWS ||
									theRadar->GetRadarMode() == RadarClass::LRS ||
									theRadar->GetRadarMode() == RadarClass::SAM)
								theRadar->SelectTWS();
							else if(theRadar->GetRadarMode() == RadarClass::TWS)
								theRadar->SelectRWS();
						}
				}
			tmstimer = 0;
			SimDriver.playerEntity->FCC->dropTrackCmd = FALSE;
			}
		}
		else
		{
			if(state & KEY_DOWN)
			{
				if(g_bRealisticAvionics)
				{
					RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);
					if(theRadar)
					{
						//ACM Modes
						if(theRadar->GetRadarMode() == RadarClass::ACM_30x20 ||
							theRadar->GetRadarMode() == RadarClass::ACM_SLEW ||
							theRadar->GetRadarMode() == RadarClass::ACM_BORE ||
							theRadar->GetRadarMode() == RadarClass::ACM_10x60)
						{
							SimDriver.playerEntity->FCC->dropTrackCmd = TRUE;
							theRadar->SelectACM30x20();
						}
						else if(theRadar->GetRadarMode() == RadarClass::RWS ||
								theRadar->GetRadarMode() == RadarClass::LRS ||
								theRadar->GetRadarMode() == RadarClass::VS ||
								theRadar->GetRadarMode() == RadarClass::SAM)
							theRadar->SelectTWS();
						else if(theRadar->GetRadarMode() == RadarClass::TWS)
							theRadar->NextTarget();
					}
				}
			}
			else
			{
				SimDriver.playerEntity->FCC->dropTrackCmd = FALSE;
			}
		}
	}
}
void SimSeatArm(unsigned long val, int state, void *)
{
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{ 
		if(g_bRealisticAvionics)
		{
			SimDriver.playerEntity->StepSeatArm();
		}
	}
}
void SimEWSRWRPower(unsigned long val, int state, void *)
{
	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{ 
		SimDriver.playerEntity->PowerToggle(AircraftClass::EWSRWRPower);
	}
}
void SimEWSJammerPower(unsigned long val, int state, void *)
{
	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{ 
		SimDriver.playerEntity->PowerToggle(AircraftClass::EWSJammerPower);
	}
}
void SimEWSChaffPower(unsigned long val, int state, void *)
{
	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{ 
		SimDriver.playerEntity->PowerToggle(AircraftClass::EWSChaffPower);
	}
}
void SimEWSFlarePower(unsigned long val, int state, void *)
{
	if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{ 
		SimDriver.playerEntity->PowerToggle(AircraftClass::EWSFlarePower);
	}
}
void SimEWSPGMInc(unsigned long val, int state, void *)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    { 
		SimDriver.playerEntity->IncEWSPGM();
    }
}
void SimEWSPGMDec(unsigned long val, int state, void *)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    { 
		SimDriver.playerEntity->DecEWSPGM();
    }
}
void SimEWSProgInc(unsigned long val, int state, void *)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    { 
		SimDriver.playerEntity->IncEWSProg();
    }
}
void SimEWSProgDec(unsigned long val, int state, void *)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    { 
		SimDriver.playerEntity->DecEWSProg();
    }
}

void SimMainPowerInc(unsigned long val, int state, void *)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    { 
	SimDriver.playerEntity->IncMainPower();
    }
}
void SimMainPowerDec(unsigned long val, int state, void *)
{
    if (SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    { 
	SimDriver.playerEntity->DecMainPower();
    }
}
void SimInhibitVMS(unsigned long val, int state, void *)
{
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{ 
		if(g_bRealisticAvionics)
		{
			SimDriver.playerEntity->ToggleBetty();
		}
	}
}
void SimRFSwitch(unsigned long val, int state, void *)
{
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{ 
		if(state & KEY_DOWN)
		{
			if(g_bRealisticAvionics)
			{
				RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);
				if(theRadar)
				{
					if(SimDriver.playerEntity->RFState == 0)
					{
						//QUIET --> no Radar
						SimDriver.playerEntity->RFState = 1;
						theRadar->SetEmitting(FALSE);
					}
					else if(SimDriver.playerEntity->RFState == 1)
					{
						//store our current AP mode
						if(SimDriver.playerEntity->AutopilotType() == AircraftClass::LantirnAP) 
						{
							SimDriver.playerEntity->lastapType = SimDriver.playerEntity->AutopilotType();
							SimDriver.playerEntity->SetAutopilot(AircraftClass::APOff);
						}
						//SILENT --> No CARA, no TFR, no Radar
						SimDriver.playerEntity->RFState = 2;
						theRadar->SetEmitting(FALSE);
					}
					else
					{
						//NORM
						SimDriver.playerEntity->RFState = 0;
						theRadar->SetEmitting(TRUE);
						//restore AP
						SimDriver.playerEntity->SetAutopilot(SimDriver.playerEntity->lastapType);
					}
				}
			}
		}
	}
}

void SimRwrPower(unsigned long val, int state, void *)
{
    if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    { 
	PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.playerEntity, SensorClass::RWR);
	if(theRwr) {
	    theRwr->SetPower(!theRwr->IsOn());
	}
    }
}

void SimDropProgrammed(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

    if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		if(SimDriver.playerEntity->OnGround())
			return;

		//drop our program manually
		if(g_bRealisticAvionics)
		{
			PlayerRwrClass* theRwr = (PlayerRwrClass*)FindSensor(SimDriver.playerEntity, SensorClass::RWR);

			//Check for Power and Failure
			if(!SimDriver.playerEntity->HasPower(AircraftClass::UFCPower) ||
				SimDriver.playerEntity->mFaults->GetFault(FaultClass::ufc_fault) ||
				SimDriver.playerEntity->IsExploding())
				return;

			//Check for our switch
			if(SimDriver.playerEntity->EWSPGM() == AircraftClass::EWSPGMSwitch::Off ||
				SimDriver.playerEntity->EWSPGM() == AircraftClass::EWSPGMSwitch::Stby)
				return;

			if(theRwr)
			{
				SimDriver.playerEntity->DropEWS();
				if(g_bMLU && val)
					theRwr->ReleaseManual = 2;
				else
					theRwr->ReleaseManual = TRUE;
			}
		}
    }
}
//MI
void SimPinkySwitch(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) &&
		SimDriver.playerEntity->Sms)
	{
		if(state & KEY_DOWN)
		{
			if(g_bRealisticAvionics)
			{
				//check what all we got first
				RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);
				bool HasMavs = FALSE;
				LaserPodClass* laserPod = (LaserPodClass* )FindLaserPod (SimDriver.playerEntity);
				MaverickDisplayClass* mavDisplay = NULL;
				if(SimDriver.playerEntity->Sms->curWeaponType == wtAgm65 && SimDriver.playerEntity->Sms->curWeapon)
				{
					HasMavs = TRUE;
					ShiAssert(SimDriver.playerEntity->Sms->curWeapon->IsMissile());
					mavDisplay = (MaverickDisplayClass*)((MissileClass*)SimDriver.playerEntity->Sms->curWeapon)->display;
				}
				//now check what to toggle
				if(theRadar && theRadar->IsSOI())
				{
					//Toggle EXP and NORM
					if((g_bMLU && (theRadar->GetRadarMode() == RadarClass::RWS ||
						theRadar->GetRadarMode() == RadarClass::LRS ||
						theRadar->GetRadarMode() == RadarClass::SAM)) ||
						theRadar->GetRadarMode() == RadarClass::TWS)
					{
						theRadar->ToggleFlag(RadarDopplerClass::EXP);
					}
					else if(theRadar->GetRadarMode() == RadarClass::GM ||
						theRadar->GetRadarMode() == RadarClass::GMT ||
						theRadar->GetRadarMode() == RadarClass::SEA)
					{ 
						theRadar->StepAGfov();
					} 
				}
				else if(SimDriver.playerEntity->FCC && SimDriver.playerEntity->FCC->IsSOI)
					SimDriver.playerEntity->FCC->ToggleHSDZoom();
				else if(HasMavs && mavDisplay && mavDisplay->IsSOI())
					mavDisplay->ToggleFOV();
				else if(laserPod && laserPod->IsSOI())
					laserPod->ToggleFOV();
			}
		}
	}
}
//MI
void SimGndJettEnable(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

    if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		if(SimDriver.playerEntity->Sms)
		{
			if(SimDriver.playerEntity->Sms->GndJett)
			{
				SimDriver.playerEntity->Sms->GndJett = FALSE;
				//No jammer when on ground
				if(SimDriver.playerEntity->OnGround())
					SimDriver.playerEntity->UnSetFlag(ECM_ON);
			}
			else
				SimDriver.playerEntity->Sms->GndJett = TRUE;
		}
	}
}
//MI
void SimExtlPower(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)
		&& SimDriver.playerEntity->af->IsSet(AirframeClass::HasComplexGear))
	{  
		AircraftClass *ac = SimDriver.playerEntity;

		if(ac->ExtlState(AircraftClass::ExtlLightFlags::Extl_Main_Power))
			ac->ExtlOff(AircraftClass::ExtlLightFlags::Extl_Main_Power);
		else
			ac->ExtlOn(AircraftClass::ExtlLightFlags::Extl_Main_Power);	
	}
}
void SimExtlAntiColl(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN) 
		&& SimDriver.playerEntity->af->IsSet(AirframeClass::HasComplexGear))
	{
		AircraftClass *ac = SimDriver.playerEntity;

		if(ac->ExtlState(AircraftClass::ExtlLightFlags::Extl_Anit_Coll))
			ac->ExtlOff(AircraftClass::ExtlLightFlags::Extl_Anit_Coll);
		else
			ac->ExtlOn(AircraftClass::ExtlLightFlags::Extl_Anit_Coll);
	}
}
void SimExtlSteady(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(SimDriver.playerEntity->ExtlState(AircraftClass::ExtlLightFlags::Extl_Steady_Flash))
			SimDriver.playerEntity->ExtlOff(AircraftClass::ExtlLightFlags::Extl_Steady_Flash);
		else
			SimDriver.playerEntity->ExtlOn(AircraftClass::ExtlLightFlags::Extl_Steady_Flash);
	}
}
void SimExtlWing(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN)
		&& SimDriver.playerEntity->af->IsSet(AirframeClass::HasComplexGear))
	{    
		AircraftClass *ac = SimDriver.playerEntity;

		if(ac->ExtlState(AircraftClass::ExtlLightFlags::Extl_Wing_Tail))
		    ac->ExtlOff(AircraftClass::ExtlLightFlags::Extl_Wing_Tail);
		else
			ac->ExtlOn(AircraftClass::ExtlLightFlags::Extl_Wing_Tail);
	}
}
//MI DMS
void SimDMSUp(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	//Up always goes to the HUD (where possible)
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.playerEntity->StepSOI(1);
	}
}
void SimDMSLeft(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	if ((state & KEY_DOWN) && (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
	{
		MfdDisplay[0]->changeMode = TRUE_NEXT;
		if(SimDriver.playerEntity->FCC && SimDriver.playerEntity->FCC->IsSOI && g_bRealisticAvionics)
		{ 
			if(MfdDisplay[0]->CurMode() == MFDClass::FCCMode)
				SimDriver.playerEntity->StepSOI(2);
		}
	}		
}
void SimDMSDown(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	//Down toggles between MFD's
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.playerEntity->StepSOI(2);
	}
}
void SimDMSRight(unsigned long val, int state, void *)
{
	if((state & KEY_DOWN) && (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
	{
		MfdDisplay[1]->changeMode = TRUE_NEXT;
		if(SimDriver.playerEntity->FCC && SimDriver.playerEntity->FCC->IsSOI && g_bRealisticAvionics)
		{ 
		  if(MfdDisplay[1]->CurMode() == MFDClass::FCCMode)
			  SimDriver.playerEntity->StepSOI(2);
		} 
	}
}
void SimAVTRSwitch(unsigned long val, int state, void *pButton)
{
	if(!g_bRealisticAvionics)
		return;

	if((state & KEY_DOWN) && (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
	{
		//off-auto-on
		if(SimDriver.playerEntity->AVTRState(AircraftClass::AVTRStateFlags::AVTR_OFF))
		{
			SimDriver.playerEntity->AVTROn(AircraftClass::AVTRStateFlags::AVTR_AUTO);
			SimDriver.playerEntity->AVTROff(AircraftClass::AVTRStateFlags::AVTR_OFF);
			SimDriver.playerEntity->AVTROff(AircraftClass::AVTRStateFlags::AVTR_ON);	//just in case
		}
		else if(SimDriver.playerEntity->AVTRState(AircraftClass::AVTRStateFlags::AVTR_AUTO))
		{
			SimDriver.playerEntity->AVTROn(AircraftClass::AVTRStateFlags::AVTR_ON);
			SimDriver.playerEntity->AVTROff(AircraftClass::AVTRStateFlags::AVTR_AUTO);
			SimDriver.playerEntity->AVTROff(AircraftClass::AVTRStateFlags::AVTR_OFF);	//just in case
			if(SimDriver.AVTROn() == FALSE)
			{
				SimDriver.SetAVTR(TRUE);
				ACMIToggleRecording ( val, state, pButton);
			}
		}
		else
		{
			SimDriver.playerEntity->AVTROn(AircraftClass::AVTRStateFlags::AVTR_OFF);
			SimDriver.playerEntity->AVTROff(AircraftClass::AVTRStateFlags::AVTR_ON);
			SimDriver.playerEntity->AVTROff(AircraftClass::AVTRStateFlags::AVTR_AUTO);	//just in case
			if(SimDriver.AVTROn() == TRUE) 
			{
				SimDriver.SetAVTR(FALSE);
				ACMIToggleRecording( val, state, pButton);
			}
		}		
	}
}
//MI
void SimAutoAVTR(unsigned long val, int state, void *pButton)
{
	if(!g_bRealisticAvionics)
		return;
	if((state & KEY_DOWN) && (SimDriver.playerEntity) && (SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP)))
	{
		if(SimDriver.playerEntity->Sms && SimDriver.playerEntity->Sms->MasterArm() ==
			SMSBaseClass::Safe)
			return;

		if(!SimDriver.playerEntity->AVTRState(AircraftClass::AVTRStateFlags::AVTR_AUTO))
			return;

		if(SimDriver.AVTROn() == FALSE)
		{ 
			SimDriver.SetAVTR(TRUE);
			SimDriver.playerEntity->AddAVTRSeconds();
			ACMIToggleRecording(0, state, NULL);
		}
		else
			SimDriver.playerEntity->AddAVTRSeconds();
	}
}
//MI
void SimIFFPower(unsigned long val, int state, void *)
{
}
//MI
void SimIFFIn(unsigned long val, int state, void *)
{
}
//MI
void SimINSInc(unsigned long val, int state, void *)
{
	//don't bother
	if(!g_bINS || !g_bRealisticAvionics)
		return;

	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(SimDriver.playerEntity->INSState(AircraftClass::INS_PowerOff))
			SimDriver.playerEntity->SwitchINSToAlign();
		else if(SimDriver.playerEntity->INSState(AircraftClass::INS_AlignNorm))
			SimDriver.playerEntity->SwitchINSToNav();
		else if(SimDriver.playerEntity->INSState(AircraftClass::INS_Nav))
			SimDriver.playerEntity->SwitchINSToInFLT();
	}
}
//MI
void SimINSDec(unsigned long val, int state, void *)
{
	//don't bother
	if(!g_bINS || !g_bRealisticAvionics)
		return;

	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(SimDriver.playerEntity->INSState(AircraftClass::INS_AlignFlight))
			SimDriver.playerEntity->SwitchINSToNav();
		else if(SimDriver.playerEntity->INSState(AircraftClass::INS_Nav))
			SimDriver.playerEntity->SwitchINSToAlign();
		else if(SimDriver.playerEntity->INSState(AircraftClass::INS_AlignNorm))
			SimDriver.playerEntity->SwitchINSToOff();
	}
}
//MI
void SimLEFLockSwitch(unsigned long val, int state, void *)
{
	//don't bother
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(SimDriver.playerEntity->LEFLocked)
		{
			SimDriver.playerEntity->LEFLocked = FALSE;
			if(SimDriver.playerEntity->mFaults && !SimDriver.playerEntity->LEFState(AircraftClass::LEFSASYNCH))
				SimDriver.playerEntity->mFaults->ClearFault(lef_fault);
		}
		else
		{
			SimDriver.playerEntity->LEFLocked = TRUE;			
			if(SimDriver.playerEntity->mFaults)
				SimDriver.playerEntity->mFaults->SetCaution(lef_fault);
		}
	}	
}
void SimDigitalBUP(unsigned long val, int state, void *)
{
}
void SimAltFlaps(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(SimDriver.playerEntity->TEFExtend) 
			SimDriver.playerEntity->TEFExtend = FALSE;
		else
			SimDriver.playerEntity->TEFExtend = TRUE;
	}
}
void SimManualFlyup(unsigned long val, int state, void *)
{
}
void SimFLCSReset(unsigned long val, int state, void *)
{
}
void SimFLTBIT(unsigned long val, int state, void *)
{
}
void SimOBOGSBit(unsigned long val, int state, void *)
{
}
//MI
void SimMalIndLights(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
	{
		if(state & KEY_DOWN)
			SimDriver.playerEntity->TestLights = TRUE;
		else
			SimDriver.playerEntity->TestLights = FALSE;
	}
}
void SimProbeHeat(unsigned long val, int state, void *)
{
}
void SimEPUGEN(unsigned long val, int state, void *)
{
}
void SimTestSwitch(unsigned long val, int state, void *)
{
}
void SimOverHeat(unsigned long val, int state, void *)
{
}
void SimTrimAPDisc(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(SimDriver.playerEntity->TrimAPDisc) 
			SimDriver.playerEntity->TrimAPDisc = FALSE;
		else
			SimDriver.playerEntity->TrimAPDisc = TRUE;
	}
}
void SimMaxPower(unsigned long val, int state, void *)
{
}
void SimABReset(unsigned long val, int state, void *)
{
}
void SimTrimNoseUp(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		pitchManualTrim = 0.50F;
	}
}
void SimTrimNoseDown(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		pitchManualTrim = -0.50F;
	}
}
void SimTrimYawLeft(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		yawManualTrim = -2.0F;
	}
}
void SimTrimYawRight(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		yawManualTrim = 2.0F;
	}
}
void SimTrimRollLeft(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		rollManualTrim = -0.50F;
	}
}
void SimTrimRollRight(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		rollManualTrim = 0.50F;
	}
}
void SimStepMissileVolumeDown(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.playerEntity->MissileVolume++;
		if(SimDriver.playerEntity->MissileVolume > 8)
			SimDriver.playerEntity->MissileVolume = 8;
	}
}
void SimStepMissileVolumeUp(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.playerEntity->MissileVolume--;
		if(SimDriver.playerEntity->MissileVolume < 0)
			SimDriver.playerEntity->MissileVolume = 0;
	}
}
void SimStepThreatVolumeDown(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.playerEntity->ThreatVolume++;
		if(SimDriver.playerEntity->ThreatVolume > 8)
			SimDriver.playerEntity->ThreatVolume = 8;
	}
}
void SimStepThreatVolumeUp(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		SimDriver.playerEntity->ThreatVolume--;
		if(SimDriver.playerEntity->ThreatVolume < 0)
			SimDriver.playerEntity->ThreatVolume = 0;
	}
}
void SimTriggerFirstDetent(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
	{
		LaserPodClass* laserPod = (LaserPodClass* )FindLaserPod (SimDriver.playerEntity);
		if(state & KEY_DOWN)
		{
			//AVTR
			if(SimDriver.playerEntity->AVTRState(AircraftClass::AVTRStateFlags::AVTR_AUTO))
			{
				if(SimDriver.playerEntity->Sms && SimDriver.playerEntity->Sms->MasterArm() ==
					SMSBaseClass::Safe)
					return;

				if(SimDriver.AVTROn() == FALSE)
				{
					SimDriver.SetAVTR(TRUE);
					SimDriver.playerEntity->AddAVTRSeconds();
					ACMIToggleRecording(0, state, NULL);
				}
				else
					SimDriver.playerEntity->AddAVTRSeconds();
			}
			//Targeting Pod, Fire laser
			if(laserPod && SimDriver.playerEntity->FCC->LaserArm && SimDriver.playerEntity->FCC->GetMasterMode()
				== FireControlComputer::AirGroundLaser)
			{
				if(!SimDriver.playerEntity->FCC->InhibitFire)
				{
					if(SimDriver.playerEntity->HasPower(AircraftClass::RightHptPower))
					{
						SimDriver.playerEntity->FCC->ManualFire = TRUE;
						SimDriver.playerEntity->FCC->CheckForLaserFire = FALSE;
					}
				}
			}
		}
		else
		{
			SimDriver.playerEntity->FCC->ManualFire = FALSE;
			SimDriver.playerEntity->FCC->CheckForLaserFire = FALSE;
		}
	}
}
void SimTriggerSecondDetent(unsigned long val, int state, void *)
{
	//if we push this one, we must have pushed the first stage as well
	if(!g_bRealisticAvionics)
		return;
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
	{
		LaserPodClass* laserPod = (LaserPodClass* )FindLaserPod (SimDriver.playerEntity);
		if(state & KEY_DOWN)
		{
			//First, check for AVTR. The 30 seconds start when we release the trigger completely.
			 if(SimDriver.playerEntity->AVTRState(AircraftClass::AVTRStateFlags::AVTR_AUTO))
			 {
				 if(SimDriver.playerEntity->Sms && SimDriver.playerEntity->Sms->MasterArm() ==
					 SMSBaseClass::Safe)
					 return;

				 if(SimDriver.AVTROn() == FALSE)
				 {
					 SimDriver.SetAVTR(TRUE);
					 SimDriver.playerEntity->AddAVTRSeconds();
					 ACMIToggleRecording(0, state, NULL);
				 }
				 else
					 SimDriver.playerEntity->AddAVTRSeconds();
			 }
			//Gun
			if(SimDriver.playerEntity->FCC &&
				(SimDriver.playerEntity->FCC->GetMasterMode() == FireControlComputer::Dogfight ||
				SimDriver.playerEntity->FCC->GetMasterMode() == FireControlComputer::MissileOverride || 
				SimDriver.playerEntity->FCC->GetSubMode() == FireControlComputer::STRAF || 
				SimDriver.playerEntity->Sms->curWeaponType == wtGuns))
			{
				SimDriver.playerEntity->GunFire = TRUE;
			}
			//Targeting Pod, Fire laser
			if(laserPod && SimDriver.playerEntity->FCC->LaserArm && SimDriver.playerEntity->FCC->GetMasterMode()
				== FireControlComputer::AirGroundLaser)
			{
				if(!SimDriver.playerEntity->FCC->InhibitFire)
				{
					if(SimDriver.playerEntity->HasPower(AircraftClass::RightHptPower))
					{
						SimDriver.playerEntity->FCC->ManualFire = TRUE;
						SimDriver.playerEntity->FCC->CheckForLaserFire = FALSE;
					}
				}
			}
		}
		else
		{
			SimDriver.playerEntity->GunFire = FALSE;
			SimDriver.playerEntity->FCC->ManualFire = TRUE;
			SimDriver.playerEntity->FCC->CheckForLaserFire = FALSE;
		}
		
	}
}

void AFFullFlap(unsigned long, int state, void*)
{
    if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		SimDriver.playerEntity->af->TEFMax();
    }
}
void AFNoFlap(unsigned long, int state, void*)
{
    if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		SimDriver.playerEntity->af->TEFClose();
    }
}
void AFIncFlap(unsigned long, int state, void*) 
{
    if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		SimDriver.playerEntity->af->TEFInc();
    }
}
void AFDecFlap(unsigned long, int state, void*)
{
    if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		SimDriver.playerEntity->af->TEFDec();
    }
}

void AFFullLEF(unsigned long, int state, void*)
{
    if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		SimDriver.playerEntity->af->LEFMax();
    }
}

void AFNoLEF(unsigned long, int state, void*)
{
    if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		SimDriver.playerEntity->af->LEFClose();
    }
}

void AFIncLEF(unsigned long, int state, void*)
{
    if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		SimDriver.playerEntity->af->LEFInc();
    }
}

void AFDecLEF(unsigned long, int state, void*)
{
    if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		SimDriver.playerEntity->af->LEFDec();
    }
}

void AFDragChute(unsigned long, int state, void*)
{
    if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		if (SimDriver.playerEntity->af->HasDragChute())
			switch (SimDriver.playerEntity->af->dragChute) {
			case AirframeClass::DRAGC_STOWED:
			SimDriver.playerEntity->af->dragChute = AirframeClass::DRAGC_DEPLOYED;
			break;
			case AirframeClass::DRAGC_DEPLOYED:
			case AirframeClass::DRAGC_TRAILING:
			SimDriver.playerEntity->af->dragChute = AirframeClass::DRAGC_JETTISONNED;
			break;
		}
    }
}

void SimRetUp(unsigned long, int state, void*)
{
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		if(TheHud)
		{
			TheHud->RetPos -= 1;
			TheHud->MoveRetCenter();
			if(TheHud->ReticlePosition > -12)
				TheHud->ReticlePosition--;
		}
    }
}
void SimRetDn(unsigned long, int state, void*)
{
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		if(TheHud)
		{
			TheHud->RetPos +=1;
			TheHud->MoveRetCenter();
			if(TheHud->ReticlePosition < 0)
				TheHud->ReticlePosition++;
		}
    }
}
void SimCursorEnable(unsigned long, int state, void*)
{
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		if(SimDriver.playerEntity->Sms)
		{
			if(SimDriver.playerEntity->Sms->curWeaponType == wtAgm65 && SimDriver.playerEntity->Sms->curWeapon)
				SimDriver.playerEntity->Sms->StepMavSubMode();
			else if(SimDriver.playerEntity->Sms->curWeaponType == wtAim9 || SimDriver.playerEntity->Sms->curWeaponType == wtAim120)
			{
				if(SimDriver.playerEntity->FCC)
					SimDriver.playerEntity->FCC->missileSlaveCmd = TRUE;
			}
		}		
	}
}

void AFCanopyToggle (unsigned long, int state, void*)
{
    if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
	SimDriver.playerEntity->af->CanopyToggle();
    }
}
void SimStepComm1VolumeUp(unsigned long val, int state, void *)
{
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp)
			OTWDriver.pCockpitManager->mpIcp->Comm1Volume--;
		if(OTWDriver.pCockpitManager->mpIcp->Comm1Volume < 0)
			OTWDriver.pCockpitManager->mpIcp->Comm1Volume = 0;
	}
}
void SimStepComm1VolumeDown(unsigned long val, int state, void *)
{
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp)
			OTWDriver.pCockpitManager->mpIcp->Comm1Volume++;
		if(OTWDriver.pCockpitManager->mpIcp->Comm1Volume > 8)
			OTWDriver.pCockpitManager->mpIcp->Comm1Volume = 8;
	}
}
void SimStepComm2VolumeUp(unsigned long val, int state, void *)
{
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp)
			OTWDriver.pCockpitManager->mpIcp->Comm2Volume--;
		if(OTWDriver.pCockpitManager->mpIcp->Comm2Volume < 0)
			OTWDriver.pCockpitManager->mpIcp->Comm2Volume = 0;
	}
}
void SimStepComm2VolumeDown(unsigned long val, int state, void *)
{
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp)
			OTWDriver.pCockpitManager->mpIcp->Comm2Volume++;
		if(OTWDriver.pCockpitManager->mpIcp->Comm2Volume > 8)
			OTWDriver.pCockpitManager->mpIcp->Comm2Volume = 8;
	}
}

#ifdef DEBUG
void SimSwitchTextureOnOff(unsigned long val, int state, void *)
{
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		g_bShowTextures = !g_bShowTextures;
	}
}
#endif
void SimSymWheelUp(unsigned long, int state, void*)
{
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		if(TheHud)
		{
			TheHud->SymWheelPos += 0.1F;
			if(TheHud->SymWheelPos > 1.0F)
				TheHud->SymWheelPos = 1.0F;
			if(TheHud->SymWheelPos < 0.61F)
				SimDriver.playerEntity->PowerOn(AircraftClass::HUDPower);
			TheHud->SetLightLevel();
		}
    }
}
void SimSymWheelDn(unsigned long, int state, void*)
{
	if(!g_bRealisticAvionics)
		return;

	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		if(TheHud)
		{
			TheHud->SymWheelPos -= 0.1F;
			if(TheHud->SymWheelPos < 0.59F)
			{
				TheHud->SymWheelPos = 0.5F;
				SimDriver.playerEntity->PowerOff(AircraftClass::HUDPower);
			}
			TheHud->SetLightLevel();
		}
    }
}
void SimToggleCockpit(unsigned long, int state, void*)
{
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		CPPanel* curPanel = OTWDriver.pCockpitManager->GetActivePanel();
		if(!curPanel)
			return;
		if(curPanel->mIdNum == 5000)
		{
			if(SimDriver.playerEntity->WideView)
			{
				SimDriver.playerEntity->WideView = FALSE;
				OTWDriver.pCockpitManager->SetActivePanel(1100);
			}
			else
			{
				OTWDriver.pCockpitManager->SetActivePanel(91100);
				SimDriver.playerEntity->WideView = TRUE;
			}	
		}
		else if(SimDriver.playerEntity->WideView)
		{
			SimDriver.playerEntity->WideView = FALSE;
			OTWDriver.pCockpitManager->SetActivePanel(curPanel->mIdNum - 90000);
		}
		else
		{
			OTWDriver.pCockpitManager->SetActivePanel(curPanel->mIdNum + 90000);
			SimDriver.playerEntity->WideView = TRUE;
		}			   
    }
}
void SimToggleGhostMFDs(unsigned long, int state, void*)
{
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
    {
		CPPanel* curPanel = OTWDriver.pCockpitManager->GetActivePanel();
		if(!curPanel)
			return;
		if(curPanel->mIdNum == 5000)
		{
			//Switch it off
			if(SimDriver.playerEntity->WideView)
				OTWDriver.pCockpitManager->SetActivePanel(91100);
			else
				OTWDriver.pCockpitManager->SetActivePanel(1100);
		}
		else
		{
			//Switch it on
			OTWDriver.pCockpitManager->SetActivePanel(5000);
		}			   
    }
}
// JB 020313
void SimFuelDump(unsigned long val, int state, void *)
{
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{
		float fueltodump = max((SimDriver.playerEntity->af->Fuel() +	SimDriver.playerEntity->af->ExternalFuel()) / 15.0, 100);
		SimDriver.playerEntity->af->AddFuel(-fueltodump);
	}
}
// JB 020316
void SimCycleDebugLabels(unsigned long val, int state, void *)
{
	if(state & KEY_DOWN)
	{
		if (g_nShowDebugLabels > 0)
		{
			g_nShowDebugLabels *= 2;
			if (g_nShowDebugLabels >= g_nMaxDebugLabel)
				g_nShowDebugLabels = 0;
		}
		else
			g_nShowDebugLabels = 1;
	}
}

// 2002-03-22 ADDED BY S.G.
#include "dogfight.h"
void SimRegen(unsigned long val, int state, void *)
{
	// 2002-04-10 MN CTD Fix SimDriver.playerEntity was NULL
	if(state & KEY_DOWN && SimDriver.playerEntity)
	{
		if (FalconLocalGame && FalconLocalGame->GetGameType() == game_Dogfight) {
			SimDriver.playerEntity->SetFalcFlag(FEC_REGENERATING);
			SimDriver.playerEntity->SetDead(1);
		}
	}
}
//MI
void SimRangeKnobDown(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{ 
		RadarDopplerClass* theRadard = (RadarDopplerClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);
		//MI
		if(theRadard) 
		{			   
			if(theRadard->GetRadarMode() == RadarClass::GM ||
				theRadard->GetRadarMode() == RadarClass::GMT ||
				theRadard->GetRadarMode() == RadarClass::SEA)
			{
				theRadard->StepAGgain(-1);
			}
			else
				theRadard->RangeStep(-1);
		}
	}
} 
void SimRangeKnobUp(unsigned long val, int state, void *)
{
	if(!g_bRealisticAvionics)
		return;
	if(SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP) && (state & KEY_DOWN))
	{ 
		RadarDopplerClass* theRadard = (RadarDopplerClass*)FindSensor(SimDriver.playerEntity, SensorClass::Radar);
		//MI
		if(theRadard) 
		{			   
			if(theRadard->GetRadarMode() == RadarClass::GM ||
				theRadard->GetRadarMode() == RadarClass::GMT ||
				theRadard->GetRadarMode() == RadarClass::SEA)
			{
				theRadard->StepAGgain(1);
			}
			else
				theRadard->RangeStep(1);
		}
	}
}
