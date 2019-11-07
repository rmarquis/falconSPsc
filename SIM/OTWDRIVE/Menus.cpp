#include <time.h>
#include "stdhdr.h"
#include "otwdrive.h"
#include "Graphics\Include\renderow.h"
#include "Graphics\Include\drawbsp.h"
#include "simdrive.h"
#include "mesg.h"
#include "MsgInc\AWACSMsg.h"
#include "MsgInc\ATCMsg.h"
#include "MsgInc\FACMsg.h"
#include "MsgInc\TankerMsg.h"
#include "falcmesg.h"
#include "aircrft.h"
#include "falclib\include\f4find.h"

// OW FIXME: this had to be added after installing the DX8 Beta 1 SDK
#define DIRECTINPUT_VERSION 0x0700
#include "dinput.h"

#include "fsound.h"
#include "fakerand.h"
#include "dogfight.h"
#include "falcsess.h"
#include "wingorder.h"
#include "classtbl.h"
#include "TimerThread.h"
#include "F4Version.h"
#include "ui\include\uicomms.h"
#include "entity.h"
#include "airframe.h"

extern int MajorVersion;
extern int MinorVersion;
extern int BuildNumber;
extern int ShowVersion;
extern int ShowFrameRate;
extern int endAbort;	// From OTWdrive.cpp
extern DrawableBSP *endDialogObject;
int endsAvail[3] = {0};
int	start = 0;
extern int CommandsKeyCombo;
extern int CommandsKeyComboMod;
static int exitMenuDesired = 0;

int tactical_is_training (void);

extern char FalconPictureDirectory[_MAX_PATH]; // JB 010623

#define EXITMENU_POPUP_TIME		15000			// Exit menu will pop up 15 seconds after death

void ResetVoices(void);

#include "sim\include\IVibeData.h"
extern IntellivibeData g_intellivibeData;
extern void *gSharedIntellivibe;
extern bool g_bShowFlaps;

void OTWDriverClass::ShowVersionString (void)
{
char verStr[24];

   if (ShowVersion == 1)
      sprintf (verStr, "%d.%02d", MajorVersion, MinorVersion);
   else
      sprintf (verStr, "%d.%02d%d%c", MajorVersion, MinorVersion, gLangIDNum);
   renderer->SetColor (0xff00ff00);
   renderer->TextCenter (-0.9F, 0.9F, verStr);
}

void OTWDriverClass::ShowPosition (void)
{
char posStr[40];

   renderer->SetColor (0xff00ff00);
   sprintf (posStr, "      X = %10.2f", flyingEye->XPos() / FEET_PER_KM);
   renderer->TextRight (0.95F, 0.95F, posStr);
   sprintf (posStr, "      Y = %10.2f", flyingEye->YPos() / FEET_PER_KM);
   renderer->TextRight (0.95F, 0.90F, posStr);
   sprintf (posStr, "      Z = %10.2f", flyingEye->ZPos());
   renderer->TextRight (0.95F, 0.85F, posStr);
   sprintf (posStr, "Heading = %10.2f", flyingEye->Yaw() * RTD);
   renderer->TextRight (0.95F, 0.80F, posStr);
   sprintf (posStr, "  Pitch = %10.2f", flyingEye->Pitch() * RTD);
   renderer->TextRight (0.95F, 0.75F, posStr);
   sprintf (posStr, "   Roll = %10.2f", flyingEye->Roll() * RTD); // 2002-01-31 ADDED BY S.G. Added roll to the readout
   renderer->TextRight (0.95F, 0.70F, posStr);
}


void OTWDriverClass::ShowAerodynamics (void)
{
    char posStr[120];
    
    if(otwPlatform && 
	otwPlatform == SimDriver.playerEntity && 
	otwPlatform->IsAirplane()) {
	AirframeClass *af = ((AircraftClass*)otwPlatform)->af;
	renderer->SetColor (0xff00ff00);
	sprintf (posStr, "Cd %8.4f Cl %8.4f Cy %8.4f Mu %8.4f", af->Cd(), af->Cl(), af->Cy(), af->mu);
	renderer->TextRight (0.95F, 0.65F, posStr);
	sprintf (posStr, "XDrag %6.4f Thrust %8.1f Mass %8.1f", af->XSAero(), af->Thrust()*af->Mass(), af->Mass());
	renderer->TextRight (0.95F, 0.60F, posStr);
	sprintf (posStr, "AoABias %5.2f Lift %5.2f Down %6.4f", af->AOABias(), -af->ZSAero(), af->ZSProp());
	renderer->TextRight (0.95F, 0.55F, posStr);
	sprintf (posStr, "AOA %5.2f Tef %10.4f Lef %10.4f", af->alpha, af->tefFactor, af->lefFactor);
	renderer->TextRight (0.95F, 0.50F, posStr);
    }
}

void OTWDriverClass::ShowFlaps(void)
{
    char posStr[120];
    
    if(g_bShowFlaps && otwPlatform && 
	otwPlatform == SimDriver.playerEntity && 
	otwPlatform->IsAirplane()) {
	AirframeClass *af = ((AircraftClass*)otwPlatform)->af;
	if (af->HasManualFlaps()) {
	    renderer->SetColor (0xff00ff00);
	    sprintf (posStr, "AOA %5.2f Flaps %3.0f Lef %3.0f", 
		af->alpha, 
		af->TefDegrees(), 
		af->LefDegrees());
	    renderer->TextLeft (-0.95F, 0.90F, posStr);
	}
    }
}

void OTWDriverClass::TakeScreenShot(void)
{
char fileName[_MAX_PATH];
char tmpStr[_MAX_PATH];
time_t ltime;
struct tm* today;

   time( &ltime );
   takeScreenShot = FALSE;
   today = localtime (&ltime);
   strftime( tmpStr, _MAX_PATH-1,
      "%m_%d_%Y-%H_%M_%S", today );
   //MI put them where they belong
#if 0
   sprintf (fileName, "%s\\%s", FalconDataDirectory, tmpStr);
#else
   sprintf (fileName, "%s\\%s", FalconPictureDirectory, tmpStr);
#endif

   OTWImage->BackBufferToRAW(fileName);
}

// NOTE Exit Menu looks like this
//
//    End     Mission
//
//    Resume  Mission
//
//    Discard Mission
//

void OTWDriverClass::DrawExitMenu (void)
{
Tpoint origin = { 0.0f, 0.0f, 0.0f };
int oldState;

   if(exitMenuOn != exitMenuDesired)
   {
      ChangeExitMenu (exitMenuDesired);
   }

   if (exitMenuOn)
   {
	   ShiAssert( endDialogObject );

      if (SimDriver.RunningDogfight())
      {
         // TODO: Show final score board
         endDialogObject->SetSwitchMask( 0, TRUE);
         endDialogObject->SetSwitchMask( 1, FALSE);
         endDialogObject->SetSwitchMask( 2, FALSE);
         endsAvail[0] = TRUE;
         endsAvail[1] = FALSE;
         endsAvail[2] = FALSE;
      }
      else if (SimDriver.RunningInstantAction())
      {
         endDialogObject->SetSwitchMask( 0, TRUE);
         endDialogObject->SetSwitchMask( 2, FALSE);
         endsAvail[0] = TRUE;
         endsAvail[2] = FALSE;

         if (SimDriver.playerEntity)
         {
            endDialogObject->SetSwitchMask( 1, TRUE);
            endsAvail[1] = TRUE;
         }
         else
         {
            endDialogObject->SetSwitchMask( 1, FALSE);
            endsAvail[1] = FALSE;
         }
      }
      else if (SimDriver.RunningCampaignOrTactical())
      {
         endDialogObject->SetSwitchMask( 0, TRUE);
         endDialogObject->SetSwitchMask( 1, TRUE);
         endsAvail[0] = TRUE;
         endsAvail[1] = TRUE;
		 if (tactical_is_training() || (gCommsMgr && gCommsMgr->Online()))
            {
            endDialogObject->SetSwitchMask( 2, FALSE);
            endsAvail[2] = FALSE;
            }
         else
            {
            endDialogObject->SetSwitchMask( 2, TRUE);
            endsAvail[2] = TRUE;
            }

      }

      oldState = renderer->GetObjectTextureState();
	  renderer->SetObjectTextureState( TRUE );
      renderer->StartFrame();
	  renderer->SetViewport(-1.0F, 1.0F, 1.0F, -1.0F);
	  renderer->SetCamera( &origin, &IMatrix );
	  endDialogObject->Draw( renderer );
      renderer->FinishFrame();
	  renderer->SetObjectTextureState( oldState );
   }
}

void OTWDriverClass::Timeout (void)
{
	SetExitMenu( FALSE );

	// See if we were already on the way out...
	if (endFlightTimer)
	{
		// Just ignore this case - we are exiting and that's all we care about
	}
	else
	{
		// end in 5 seconds
		endFlightTimer = vuxRealTime + 5000;
		ResetVoices();
	}
	
	// no hud when ending flight -- may want to make sure other stuff
	// isn't set too....
	SetOTWDisplayMode(ModeNone);
	
	// Get out of eyeFly if we were in it
	if (eyeFly)
	{
#if 1
		ToggleEyeFly();
#else
		otwPlatform = lastotwPlatform;
		lastotwPlatform = NULL;
		eyeFly = FALSE;
#endif
	}

	// if we've got an otwplatform (ie the f16) jump out ahead of it for
	// a ways to get a fly-by effect
	if ( endFlightPointSet == FALSE )
	{
		if ( otwPlatform )
		{
//			MonoPrint ("Panning exit\n");
			SetEndFlightPoint( otwPlatform->XPos() + otwPlatform->dmx[0][0] * 10.0f + otwPlatform->XDelta() * 2.0f,
							   otwPlatform->YPos() + otwPlatform->dmx[0][1] * 10.0f + otwPlatform->YDelta() * 2.0f,
							   otwPlatform->ZPos() + otwPlatform->dmx[0][2] * 10.0f + otwPlatform->ZDelta() * 2.0f - 20.0f );
			SetEndFlightVec(0.0f, 0.0f, 0.0f);
		}
		else
		{
//          MonoPrint ("Fixed exit\n");
			// not otwplatform, use last focus point with some randomness...
			SetEndFlightPoint(	focusPoint.x + 100.0f * PRANDFloat(), 
								focusPoint.y + 100.0f * PRANDFloat(), 
								focusPoint.z - 100.0f );
			SetEndFlightVec(0.0f, 0.0f, 0.0f);
		}
	}
}

void OTWDriverClass::ExitMenu (unsigned long i)
{
   if (i == DIK_ESCAPE)
   {
      SetExitMenu( FALSE );
   }
   else if ((i == DIK_E && endsAvail[0]) || (i == DIK_D && endsAvail[2]))
   {
     g_intellivibeData.IsEndFlight = true;
		 memcpy (gSharedIntellivibe, &g_intellivibeData, sizeof(g_intellivibeData));

     if (i == DIK_D || tactical_is_training())
         endAbort = TRUE;

      SetExitMenu( FALSE );

      // if already set end now
      if ( endFlightTimer || !gameCompressionRatio )
      {
   	   endFlightTimer = vuxRealTime;
      }
      else
      {
         // end in 5 seconds
         endFlightTimer = vuxRealTime + 5000;
		 ResetVoices();
      }

       // no hud when ending flight -- may want to make sure other stuff
      // isn't set too....
      SetOTWDisplayMode(ModeChase);

      // if we've got an otwplatform (ie the f16) jump out ahead of it for
      // a ways to get a fly-by effect
      if ( endFlightPointSet == FALSE )
      {
	      if ( otwPlatform )
	      {
//			   MonoPrint ("Panning exit\n");
			   SetEndFlightPoint( otwPlatform->XPos() + otwPlatform->dmx[0][0] * 10.0f + otwPlatform->XDelta() * 2.0f,
			                      otwPlatform->YPos() + otwPlatform->dmx[0][1] * 10.0f + otwPlatform->YDelta() * 2.0f,
			                      otwPlatform->ZPos() + otwPlatform->dmx[0][2] * 10.0f + otwPlatform->ZDelta() * 2.0f - 20.0f );
			   SetEndFlightVec(0.0f, 0.0f, 0.0f);
	      }
	      else
	      {
//          MonoPrint ("Fixed exit\n");
		       // not otwplatform, use last focus point with some randomness...
               SetEndFlightPoint(focusPoint.x + 100.0f * PRANDFloat(), focusPoint.y + 100.0f * PRANDFloat(), focusPoint.z - 100.0f);
               SetEndFlightVec(0.0f, 0.0f, 0.0f);
	      }
      }

      if (eyeFly)
      {
#if 1
		ToggleEyeFly();
#else
		otwPlatform = lastotwPlatform;
		lastotwPlatform = NULL;
		eyeFly = FALSE;
#endif
      }
   }
   else if (i == DIK_R && endsAvail[1])
   {
	   // Start E3 HACK
	   if (SimDriver.RunningInstantAction () && SimDriver.playerEntity && SimDriver.playerEntity->IsSetFlag (MOTION_OWNSHIP))
	   {
		   SimDriver.playerEntity->ResetFuel ();
	   }
	   // End E3 HACK

	   SetExitMenu( FALSE );
   }
}

int OTWDriverClass::HandleMouseClick (long x, long y)
{
int key = 0;
float xRes = (float)renderer->GetXRes();
float yRes = (float)renderer->GetYRes();
float logicalX, logicalY;
int	passThru = TRUE;

	if(InExitMenu()) {

		passThru = FALSE;

		// Correct for screen resolution
		logicalX = (float)x / xRes;
		logicalY = (float)y / yRes;

		if (logicalX >= 230.0F/640.0F && logicalX <= 250.0F/640.0F)
		{
			if (logicalY >= 200.0F/480.0F && logicalY <= 220.0F/480.0F && endsAvail[0])
			{
				key = DIK_E;
			}
			else if (logicalY >= 235.0F/480.0F && logicalY <= 255.0F/480.0F && endsAvail[1])
			{
				key = DIK_R;
			}
			else if (logicalY >= 270.0F/480.0F && logicalY <= 290.0F/480.0F && endsAvail[2])
			{
				key = DIK_D;
			}
		}

		if (key)
			ExitMenu (key);
	}

   return passThru;
}

void OTWDriverClass::SetExitMenu (int newVal)
{
	exitMenuDesired = newVal;
	exitMenuTimer = 0;
}

void OTWDriverClass::ChangeExitMenu (int newVal)
{
int texSet;

	if (newVal == TRUE)
   {
      // if already set end now
      if ( endFlightTimer)
      {
   	   endFlightTimer = vuxRealTime;
         newVal = FALSE;
      }
      else if (!endDialogObject)
      {
			Tpoint	pos = {4.0f, 0.0f, 0.0f };
         pos.x *= (60.0F * DTR) / GetFOV();
			endDialogObject = new DrawableBSP(MapVisId(VIS_END_MISSION), &pos, &IMatrix, 1.0f );

         switch (gLangIDNum)
         {
            case F4LANG_UK:               // UK
            case F4LANG_ENGLISH:          // US
               texSet = 0;
            break;

            case F4LANG_GERMAN:           // DE
               texSet = 1;
            break;

            case F4LANG_FRENCH:           // FR
               texSet = 2;
            break;

		      case F4LANG_SPANISH:
   				texSet = 3;
            break;

		      case F4LANG_ITALIAN:
   				texSet = 4;
            break;

		      case F4LANG_PORTUGESE:
   				texSet = 5;
            break;

            default:
               texSet = 0;
            break;
         }
         endDialogObject->SetTextureSet(texSet);
		}
	}
   else
   {
		delete endDialogObject;
		endDialogObject = NULL;
	}

	exitMenuOn = newVal;
}

void OTWDriverClass::StartExitMenuCountdown(void)
	{
	exitMenuTimer = vuxRealTime + EXITMENU_POPUP_TIME;
	}

void OTWDriverClass::CancelExitMenuCountdown(void)
	{
	exitMenuTimer = 0;
	}
