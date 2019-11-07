#include "F4Thread.h"
#include "sinput.h"
#include "simio.h"
#include "simdrive.h"
#include "OTWDrive.h"
#include "inpFunc.h"
#include "cpmanager.h"
#include "ffeedbk.h"
#include "datadir.h"
#include "flightData.h"
#include "ui\include\logbook.h"
#include "aircrft.h"

DIDEVCAPS gCurJoyCaps;
BOOL CALLBACK JoystickEnumEffectTypeProc(LPCDIEFFECTINFO pei, LPVOID pv);
BOOL JoystickCreateEffect(DWORD dwEffectFlags);

#define BUTTON_PRESSED 0x80
#define KEY_DOWN   0x8
#define REPEAT_DELAY 200

static DWORD LastPressed[SIMLIB_MAX_DIGITAL] = {0};
static DWORD LastPressedPOV[SIMLIB_MAX_POV] = {0};
static int LastDirPOV[SIMLIB_MAX_POV] = {0};
int	center = FALSE;
int	setABdetent = FALSE;
long mHelmetIsUR = FALSE; // hack for UR Helmet detected
long mHelmetID;
float UR_HEAD_VIEW=160.0f;
float UR_PREV_X=0.0f;
float UR_PREV_Y=0.0f;

extern int DisableSmoothing;
extern int NoRudder;
static int JoyOutput[SIMLIB_MAX_ANALOG][2] = {0};
extern int PickleOverride;
extern int TriggerOverride;
extern int	UseKeyboardThrottle;
static int hasForceFeedback = FALSE;

extern int g_nFFEffectAutoCenter; // JB 020306 Don't stop the centering FF effect

extern int g_nThrottleMode;	// OW - Win2k throttle workaround


enum{
	Elevator,
	Aileron,
	Throttle,
	Rudder,
	CursorX,
	CursorY,
	NewInput = 0,
	OldInput = 1,
	MAX_DIFF = 10000,
};

LPDIRECTINPUTEFFECT* gForceFeedbackEffect;
int* gForceEffectIsRepeating = NULL;
int* gForceEffectHasDirection = NULL;
int gNumEffectsLoaded = 0;

int g_nThrottleID = DIJOFS_Z;		// OW

void SetJoystickCenter(void)
{
	center = TRUE;
	//IO.analog[0].center = IO.analog[0].engrValue * -1000;
	//IO.analog[1].center = IO.analog[1].engrValue * -1000;
	//IO.analog[3].center = IO.analog[3].engrValue * -1000;
}

void ProcessJoystickInput(int axis, long *value)
{
	JoyOutput[axis][OldInput] = JoyOutput[axis][NewInput];
	JoyOutput[axis][NewInput] = *value;

	if(!DisableSmoothing)
	{		
		int diff = abs(JoyOutput[axis][NewInput] - JoyOutput[axis][OldInput])*FloatToInt32((SimLibMajorFrameTime/0.1F));
		if(diff > MAX_DIFF)
		{
			*value = JoyOutput[axis][OldInput];
			JoyOutput[axis][NewInput] = JoyOutput[axis][OldInput] + 
											(	JoyOutput[axis][NewInput] - 
												JoyOutput[axis][OldInput]	)/2;
		}
	}
	IO.analog[axis].ioVal = *value;
}

void GetURHelmetInput()
{
	HRESULT hRes;
	DIJOYSTATE joyState;
//	BOOL POVCentered;
	float headx,heady,vx,vy;
	static long PrevButtonStates;
	
	hRes = ((LPDIRECTINPUTDEVICE2)gpDIDevice[SIM_JOYSTICK1 + mHelmetID])->Poll();
		
	hRes = gpDIDevice[SIM_JOYSTICK1 + mHelmetID]->GetDeviceState(sizeof(DIJOYSTATE),&joyState);
		
	switch(hRes)
	{
		case DI_OK:
			headx=(float)(joyState.lX);
			heady=-(float)(joyState.lY);

			headx=headx * UR_HEAD_VIEW;
			heady=heady * UR_HEAD_VIEW;

			headx=headx / 10000.0f;
			heady=heady / 10000.0f;

			vx=(headx * 0.8f + UR_PREV_X * 0.2f);
			vy=(heady * 0.8f + UR_PREV_Y * 0.2f);

			if(vy > 25.0f) vy=25.0f; // peg
			if(vy < -85.0f) vy=-85.0f; // peg

			cockpitFlightData.headYaw   = vx * DTR;
			cockpitFlightData.headPitch = vy * DTR;

			UR_PREV_X=vx;
			UR_PREV_Y=vy;

			break;
							
		default:
			AcquireDeviceInput(SIM_JOYSTICK1 + mHelmetID, TRUE);
			break;
	}
}

void GetJoystickInput()
{
	
	//IO.ReadAllAnalogs();
	//IO.ReadAllDigitals();
	
	
	HRESULT hRes;
	DIJOYSTATE joyState;
	BOOL POVCentered;
	unsigned int i =0;
	static long PrevButtonStates;
	
	if(gTotalJoy && gCurController >= SIM_JOYSTICK1)
	{
		hRes = ((LPDIRECTINPUTDEVICE2)gpDIDevice[gCurController])->Poll();
		
		hRes = gpDIDevice[gCurController]->GetDeviceState(sizeof(DIJOYSTATE),&joyState);
		
		switch(hRes)
		{
		case DI_OK:

			// OW - Win2k throttle workaround
			if(g_nThrottleMode == 1)
			{
				switch(g_nThrottleID)
				{
					case DIJOFS_SLIDER(0): joyState.lZ = joyState.rglSlider[0]; break;
					case DIJOFS_SLIDER(1): joyState.lZ = joyState.rglSlider[1]; break;
				}
			}

			ProcessJoystickInput(Elevator, &joyState.lX);
			ProcessJoystickInput(Aileron, &joyState.lY); 
			ProcessJoystickInput(Throttle, &joyState.lZ);
			ProcessJoystickInput(Rudder, &joyState.lRz);	

			//IO.analog[0].engrValue = min(max((joyState.lX + IO.analog[0].center)/1000.0f, -1.0F),1.0F);
			IO.analog[Elevator].engrValue = (float)joyState.lX + IO.analog[Elevator].center;
			if(IO.analog[Elevator].engrValue * IO.analog[Elevator].center > 0)
				IO.analog[Elevator].engrValue /= (10000.0F + (float)abs(IO.analog[Elevator].center));
			else
				IO.analog[Elevator].engrValue /= (10000.0F - (float)abs(IO.analog[Elevator].center));

			//IO.analog[1].engrValue = min(max((joyState.lY + IO.analog[1].center)/1000.0f, -1.0F),1.0F);
			IO.analog[Aileron].engrValue = (float)joyState.lY + IO.analog[Aileron].center;
			if(IO.analog[Aileron].engrValue * IO.analog[Aileron].center > 0)
				IO.analog[Aileron].engrValue /= (9400.0F + (float)abs(IO.analog[Aileron].center));
			else
				IO.analog[Aileron].engrValue /= (9400.0F - (float)abs(IO.analog[Aileron].center));

			if (IO.AnalogIsUsed(Throttle))
			{
				if(!UseKeyboardThrottle || abs(JoyOutput[Throttle][OldInput] - joyState.lZ) > 500.0F)
				{
					UseKeyboardThrottle = FALSE;
					if(IO.analog[Throttle].center && joyState.lZ > IO.analog[Throttle].center)
						IO.analog[Throttle].engrValue = (15000.0F - joyState.lZ)/(15000.0F - IO.analog[Throttle].center);
					else if(IO.analog[Throttle].center)
						IO.analog[Throttle].engrValue = 1.0F + (IO.analog[Throttle].center - joyState.lZ)/(IO.analog[Throttle].center*2.0F);
					else
						IO.analog[Throttle].engrValue = (15000.0F - joyState.lZ)/10000.0F;
				}
			}

			if (IO.AnalogIsUsed(Rudder))
			{
				//IO.analog[3].engrValue = min(max((joyState.lRz + IO.analog[3].center)/1000.0f, -1.0F),1.0F);
				IO.analog[Rudder].engrValue = (float)joyState.lRz + IO.analog[Rudder].center;
				if(IO.analog[Rudder].engrValue * IO.analog[Rudder].center > 0)
					IO.analog[Rudder].engrValue /= (10000.0F + (float)abs(IO.analog[Rudder].center));
				else
					IO.analog[Rudder].engrValue /= (10000.0F - (float)abs(IO.analog[Rudder].center));
			}
			
			for(i=0; i<gCurJoyCaps.dwPOVs;i++)
			{
				POVCentered = (LOWORD(joyState.rgdwPOV[i]) == 0xFFFF);
				
				if(POVCentered)
					IO.povHatAngle[i] = (unsigned long)-1;
				else
					IO.povHatAngle[i] = joyState.rgdwPOV[i];
			}

			for(i=0;i<SIMLIB_MAX_DIGITAL;i++)
				IO.digital[i] = (short)(joyState.rgbButtons[i] & BUTTON_PRESSED);

			if(center)
			{
				if(abs(joyState.lX) == 10000)
					IO.analog[Elevator].center = 0;
				else
					IO.analog[Elevator].center = joyState.lX * -1;

				if(abs(joyState.lY) == 10000)
					IO.analog[Aileron].center = 0;
				else
					IO.analog[Aileron].center = joyState.lY * -1;

				if(abs(joyState.lRz) == 10000)
					IO.analog[Rudder].center = 0;
				else
					IO.analog[Rudder].center = joyState.lRz * -1;
				center = FALSE;
				IO.SaveFile();
			}

			if(setABdetent)
			{
				IO.analog[Throttle].center = joyState.lZ;
				setABdetent = FALSE;
				IO.SaveFile();
			}

			break;
							
		default:
			AcquireDeviceInput(gCurController, TRUE);
			IO.analog[Elevator].engrValue = 0.0f;
			IO.analog[Aileron].engrValue = 0.0f;
			IO.analog[Throttle].engrValue = 0.0f;
			IO.analog[Rudder].engrValue = 0.0f;
			
			for(i=0;i<SIMLIB_MAX_DIGITAL;i++)
				IO.digital[i] = FALSE;
			
			IO.povHatAngle[0] = (unsigned long)-1;
			
		}
		if(mHelmetIsUR)
		{
			GetURHelmetInput();
		}
	}
	else
	{
		IO.analog[0].engrValue = 0.0f;
		IO.analog[1].engrValue = 0.0f;
		IO.analog[2].engrValue = 1000.0f;
		IO.analog[3].engrValue = 0.0f;
		
		for(i=0;i<SIMLIB_MAX_DIGITAL;i++)
			IO.digital[i] = FALSE;
		
		IO.povHatAngle[0] = (unsigned long)-1;
		if(mHelmetIsUR)
		{
			GetURHelmetInput();
		}
	}
	
	
	
}

void ProcessJoyButtonAndPOVHat(void)
{
unsigned int i;
InputFunctionType theFunc;

	for(i=0;i<gCurJoyCaps.dwButtons;i++)
	{
		if(IO.digital[i])
		{
			DWORD curTicks = GetTickCount();
			if(curTicks > LastPressed[i] + REPEAT_DELAY)
			{
				int ID;
				theFunc = UserFunctionTable.GetButtonFunction(i,&ID);
				if(theFunc)
				{
					if( ID  < 0) {
						theFunc(1, KEY_DOWN, NULL);
					}
					else {
						theFunc(1, KEY_DOWN, OTWDriver.pCockpitManager->GetButtonPointer(ID));
						OTWDriver.pCockpitManager->Dispatch(ID, 0);//the 0 should be mousside but I don't have anywhere
					}												//to store it and all functions currently use 0. ;)
				}
				else if(i == 0)
				{
					TriggerOverride = TRUE;
				}
				else if(i == 1)
				{
					PickleOverride = TRUE;
				}
				LastPressed[i] = curTicks;
			}
		}
		else if (LastPressed[i])
		{
			LastPressed[i] = 0;
			int ID;
			theFunc = UserFunctionTable.GetButtonFunction(i,&ID);
			if(theFunc)
				theFunc(1, 0, NULL);
			else if(i == 0)
			{
				TriggerOverride = FALSE;
			}
			else if(i == 1)
			{
				PickleOverride = FALSE;
			}
		}
		
	}

	for(i=0;i<gCurJoyCaps.dwPOVs;i++)
	{
		if(IO.povHatAngle[i] != -1)
		{
			int ID;
			InputFunctionType theFunc;
			int Direction = 0;

			if((IO.povHatAngle[i] < 2250 || IO.povHatAngle[i] > 33750) && IO.povHatAngle[i] != -1)
				Direction = 0;
			else if(IO.povHatAngle[i] < 6750 )
				Direction = 1;
			else if(IO.povHatAngle[i] < 11250)
				Direction = 2;
			else if(IO.povHatAngle[i] < 15750)
				Direction = 3;
			else if(IO.povHatAngle[i] < 20250)
				Direction = 4;
			else if(IO.povHatAngle[i] < 24750)
				Direction = 5;
			else if(IO.povHatAngle[i] < 29250)
				Direction = 6;
			else if(IO.povHatAngle[i] < 33750)
				Direction = 7;

			if(LastDirPOV[i] != Direction)
			{
				theFunc = UserFunctionTable.GetPOVFunction(i,LastDirPOV[i],&ID);
				if(theFunc)
					theFunc(1, 0, NULL);
				LastDirPOV[i] = Direction;
			}

			DWORD curTicks = GetTickCount();
			if(curTicks > LastPressedPOV[i] + REPEAT_DELAY)
			{
				theFunc = UserFunctionTable.GetPOVFunction(i,Direction,&ID);
				if(theFunc)
				{
					if( ID  < 0) {
						theFunc(1, KEY_DOWN, NULL);
					}
					else {
						theFunc(1, KEY_DOWN, OTWDriver.pCockpitManager->GetButtonPointer(ID));
						OTWDriver.pCockpitManager->Dispatch(ID, 0);//the 0 should be mousside but I don't have anywhere
					}												//to store it and all functions currently use 0. ;)
				}
				else
					SimDriver.POVKludgeFunction(IO.povHatAngle[i]);

				LastPressedPOV[i] = curTicks;
			}
		}
		else if (LastPressedPOV[i])
		{
			LastPressedPOV[i] = 0;
			int ID;
			theFunc = UserFunctionTable.GetPOVFunction(i,LastDirPOV[i],&ID);
			if(theFunc)
				theFunc(1, 0, NULL);
			else
				SimDriver.POVKludgeFunction(IO.povHatAngle[i]);
		}
		
	}
	
	//SimDriver.POVKludgeFunction(IO.povHatAngle[0]);	
	
}

float ReadThrottle(void)
{
	HRESULT hRes;
	DIJOYSTATE joyState;

	if(gTotalJoy && gCurController >= SIM_JOYSTICK1)
	{
		hRes = ((LPDIRECTINPUTDEVICE2)gpDIDevice[gCurController])->Poll();
		
		hRes = gpDIDevice[gCurController]->GetDeviceState(sizeof(DIJOYSTATE),&joyState);

		switch(hRes)
		{
		case DI_OK:
			JoyOutput[Throttle][OldInput] = JoyOutput[Throttle][NewInput];
			JoyOutput[Throttle][NewInput] = joyState.lZ;
			if(IO.analog[Throttle].center && joyState.lZ > IO.analog[Throttle].center)
				IO.analog[Throttle].engrValue = (15000.0F - joyState.lZ)/(15000.0F - IO.analog[Throttle].center);
			else if(IO.analog[Throttle].center)
				IO.analog[Throttle].engrValue = 1.0F + (IO.analog[Throttle].center - joyState.lZ)/(IO.analog[Throttle].center*2.0F);
			else
				IO.analog[Throttle].engrValue = (15000.0F - joyState.lZ)/10000.0F;
			break;
			
		default:
			AcquireDeviceInput(gCurController, TRUE);
			if(SimDriver.playerEntity && SimDriver.playerEntity->OnGround())
				IO.analog[Throttle].engrValue = 0.0f;
			else
				IO.analog[Throttle].engrValue = 1.0f;
		}
	}
	else
	{
		if(SimDriver.playerEntity && SimDriver.playerEntity->OnGround())
			IO.analog[Throttle].engrValue = 0.0f;
		else
			IO.analog[Throttle].engrValue = 1.0f;
	}
	return IO.analog[Throttle].engrValue;
}

BOOL FAR PASCAL InitJoystick(LPCDIDEVICEINSTANCE pdinst, LPVOID pvRef)
{
	BOOL					SetupResult;
	LPDIRECTINPUTDEVICE		pdev;
	DIPROPRANGE				diprg;
	HWND					hWndMain = *((HWND *)pvRef);
	DIDEVCAPS				devcaps;
	DIDEVICEOBJECTINSTANCE	devobj;
	HRESULT					hres;
	
	devobj.dwSize = sizeof(DIDEVICEOBJECTINSTANCE);

	devcaps.dwSize = sizeof(DIDEVCAPS);

	DIPROPDWORD dipdw = {{sizeof(DIPROPDWORD), sizeof(DIPROPHEADER), 
		0, DIPH_DEVICE},DJOYSTICK_BUFFERSIZE};
	
	SetupResult = VerifyResult(gpDIObject->CreateDeviceEx(pdinst->guidInstance, IID_IDirectInputDevice7, (void **) &pdev, NULL) );
	
	if(!SetupResult)
		return DIENUM_CONTINUE;
	
	// set joystick data format
	SetupResult = VerifyResult(pdev->SetDataFormat( &c_dfDIJoystick) );
	
	SetupResult = VerifyResult(pdev->GetCapabilities( &devcaps) );
	
	// set the cooperative level
	if(SetupResult)
	{/*
#ifdef NDEBUG
		SetupResult = VerifyResult(pdev->SetCooperativeLevel( hWndMain,
			DISCL_NONEXCLUSIVE | DISCL_FOREGROUND) );
#else*/
// OW: Force feedback fix (you need the exclusive cooperative level for any force-feedback device)
#if 0
		SetupResult = VerifyResult(pdev->SetCooperativeLevel( hWndMain,
			DISCL_NONEXCLUSIVE | DISCL_BACKGROUND) );
#else
// 2001-10-22 MODIFIED BY S.G. AS WELL AS FOREGROUND.
//		SetupResult = VerifyResult(pdev->SetCooperativeLevel( hWndMain,
//			DISCL_EXCLUSIVE | DISCL_BACKGROUND));
		SetupResult = VerifyResult(pdev->SetCooperativeLevel( hWndMain,
			DISCL_EXCLUSIVE | DISCL_FOREGROUND));
#endif
//#endif
	}
	
	// Setup X Axis
	if(SetupResult)
	{
		// set X-axis range to (-10000 ... +10000)
		
		diprg.diph.dwSize       = sizeof(diprg);
		diprg.diph.dwHeaderSize = sizeof(diprg.diph);
		diprg.diph.dwObj        = DIJOFS_X;
		diprg.diph.dwHow        = DIPH_BYOFFSET;
		diprg.lMin              = -10000;
		diprg.lMax              = +10000;
		
		SetupResult = VerifyResult(pdev->SetProperty(DIPROP_RANGE, &diprg.diph) );
		
	}
	
	if(SetupResult)
	{
		//
		// And again for Y-axis range
		//
		diprg.diph.dwObj        = DIJOFS_Y;
		
		SetupResult = VerifyResult(pdev->SetProperty( DIPROP_RANGE, &diprg.diph) );
		
	}

	
	hres = pdev->GetObjectInfo(&devobj,DIJOFS_RZ,DIPH_BYOFFSET);
	if(SetupResult && SUCCEEDED(hres) && !NoRudder)
	{
		//
		// And again for Z-axis rotation (rudder)
		//
		diprg.diph.dwObj        = DIJOFS_RZ;
		
		SetupResult = VerifyResult(pdev->SetProperty( DIPROP_RANGE, &diprg.diph) );
		if (SetupResult)
			IO.SetAnalogIsUsed(3, TRUE);
		else
			IO.SetAnalogIsUsed(3, FALSE);
	}
	else
		IO.SetAnalogIsUsed(3, FALSE);

	hres = pdev->GetObjectInfo(&devobj, g_nThrottleID, DIPH_BYOFFSET);

	if (FAILED(hres))
	{
		DWORD dwVersion = GetVersion();
 
		// Get the Windows version.

		DWORD dwWindowsMajorVersion =  (DWORD)(LOBYTE(LOWORD(dwVersion)));
		DWORD dwWindowsMinorVersion =  (DWORD)(HIBYTE(LOWORD(dwVersion)));

		// Get the build number for Windows NT/Windows 2000 or Win32s.

		DWORD dwBuild;
		if (dwVersion < 0x80000000)                // Windows NT/2000
			dwBuild = (DWORD)(HIWORD(dwVersion));
		else if (dwWindowsMajorVersion < 4)        // Win32s
			dwBuild = (DWORD)(HIWORD(dwVersion) & ~0x8000);
		else                  // Windows 95/98 -- No build number
			dwBuild =  0;

		// OW - Win2k throttle workaround
		if(g_nThrottleMode || dwWindowsMajorVersion >= 5)
		{
			// retry with slider 0
			g_nThrottleMode = 1;
			g_nThrottleID = DIJOFS_SLIDER(0);
			hres = pdev->GetObjectInfo(&devobj, g_nThrottleID, DIPH_BYOFFSET);
		}
	}

	if(SetupResult && SUCCEEDED(hres) )
	{
		//
		// And again for Z-axis range (throttle)
		//
		diprg.diph.dwObj        = g_nThrottleID;
		diprg.lMin              = 0;
		diprg.lMax              = 15000;
		
		SetupResult = VerifyResult(pdev->SetProperty( DIPROP_RANGE, &diprg.diph) );
		if(SetupResult)
			IO.SetAnalogIsUsed(2, TRUE);
		else
			IO.SetAnalogIsUsed(2, FALSE);
	}
	else
		IO.SetAnalogIsUsed(2, FALSE);
	
	if(SetupResult )
	{
		dipdw.dwData = 100;
		dipdw.diph.dwObj = DIJOFS_X;
		dipdw.diph.dwHow = DIPH_BYOFFSET;
		// set X axis dead zone to 1% 
		pdev->SetProperty( DIPROP_DEADZONE, &dipdw.diph);
	}
	
	if(SetupResult )
	{
		dipdw.diph.dwObj = DIJOFS_Y;
		
		// set Y axis dead zone to 1% 
		SetupResult = VerifyResult(pdev->SetProperty( DIPROP_DEADZONE, &dipdw.diph));
	}
	
	if(IO.AnalogIsUsed(3) && SetupResult)
	{
		dipdw.diph.dwObj = DIJOFS_RZ;
		
		// set RZ axis dead zone to 1% 
		SetupResult = VerifyResult(pdev->SetProperty( DIPROP_DEADZONE, &dipdw.diph));
	}
	
	
	//
	// Add it to our list of devices.  
	//        
	
	if(SetupResult)
	{
	/*
	*  Convert it to a Device2 so we can Poll() it. 
	*  (Will also increment reference count if it succeeds)
	*/
		
		SetupResult = VerifyResult(pdev->QueryInterface(IID_IDirectInputDevice2, 
			(LPVOID *)&gpDIDevice[SIM_JOYSTICK1 + gTotalJoy]) );
	}

   // Check for Force Feedback
   if (SetupResult)
   {
      if(devcaps.dwFlags & DIDC_FORCEFEEDBACK)
      {
         //Got it
         OutputDebugString("ForceFeedback device found.\n");

         // we're supporting ForceFeedback
         if(!JoystickCreateEffect(0xffffffff))
         {
            OutputDebugString("JoystickCreateEffects() failed - ForceFeedback disabled\n");
         }
         else
         {
         DIPROPDWORD DIPropAutoCenter;

            // Disable stock auto-center
            DIPropAutoCenter.diph.dwSize = sizeof(DIPropAutoCenter);
            DIPropAutoCenter.diph.dwHeaderSize = sizeof(DIPROPHEADER);
            DIPropAutoCenter.diph.dwObj = 0;
            DIPropAutoCenter.diph.dwHow = DIPH_DEVICE;
            DIPropAutoCenter.dwData = DIPROPAUTOCENTER_OFF;

            if (!VerifyResult(gpDIDevice[SIM_JOYSTICK1 + gTotalJoy]->SetProperty(DIPROP_AUTOCENTER, &DIPropAutoCenter.diph)))
            {
               OutputDebugString("Failed to turn auto-center off.\n");
            }
            else
            {
               hasForceFeedback = TRUE;
            }
         }
      }
   }
	
	if(SetupResult)
	{
		//use pdinst to get name of joystick for display purposes
		int len = _tcslen(pdinst->tszProductName);
		gDIDevNames[SIM_JOYSTICK1 + gTotalJoy] = new _TCHAR[len + 1];
		
		if(gDIDevNames[SIM_JOYSTICK1 + gTotalJoy])
			_tcscpy(gDIDevNames[SIM_JOYSTICK1 + gTotalJoy],pdinst->tszProductName);
		
		if(!strcmp(pdinst->tszProductName,"Union Reality Gear"))
		{
            OutputDebugString("UR Helmet found\n");
			OTWDriver.SetHeadTracking(TRUE);
			mHelmetIsUR=1;
			mHelmetID=gTotalJoy;
		}

		gTotalJoy++;
		
		//we've used up our whole array, sorry no more!!
		if( (SIM_JOYSTICK1 + gTotalJoy) >= SIM_NUMDEVICES)
			return DIENUM_STOP;
	}
	
	//if the call to QueryInterface succeeded the reference count was incremented to 2
	//so this will drop it to 1 or 0.
	pdev->Release();
	return DIENUM_CONTINUE;
}

//BOOL JoystickCreateEffect(DWORD dwEffectFlags)
BOOL JoystickCreateEffect(DWORD)
{
FILE* fPtr=NULL;
unsigned int effectType=0;
int numEffects=0;
char effectName[20]={0};
int i=0;
unsigned int j=0, k=0;
DIEFFECT effectHolder;
DIENVELOPE envelopeHolder;
DICUSTOMFORCE customHolder;
DIPERIODIC periodicHolder;
DICONSTANTFORCE constantHolder;
DIRAMPFORCE rampHolder;
DICONDITION* conditionHolder = NULL;
char str1[80]={0}, str2[80]={0};
DWORD* axesArray = NULL;
long* dirArray = NULL;
long* forceData = NULL;
char dataFileName[_MAX_PATH];
GUID guidEffect;
DWORD SetupResult=FALSE;
LPDIRECTINPUTDEVICE2 joystickDevice = (LPDIRECTINPUTDEVICE2)gpDIDevice[gCurController];
int guidType=0;

   sprintf (dataFileName, "%s\\config\\feedback.ini", FalconDataDirectory);

   fPtr = fopen (dataFileName, "r");

   // Find the file
   if (!fPtr)
   {
      OutputDebugString ("Unable to open force feedback data file");
      return FALSE;
   }
   fclose (fPtr);

    JoystickReleaseEffects();

   // Start parsing
   numEffects = GetPrivateProfileInt("EffectData", "numEffects", 0, dataFileName);
   gNumEffectsLoaded = numEffects;
   gForceFeedbackEffect = new LPDIRECTINPUTEFFECT[numEffects];
   gForceEffectIsRepeating = new int[numEffects];
   gForceEffectHasDirection = new int[numEffects];

   // Read each effect
   for (i=0; i<numEffects; i++)
   {
       ZeroMemory(&effectHolder, sizeof effectHolder);
       ZeroMemory(&envelopeHolder, sizeof envelopeHolder);
       effectHolder.dwSize = sizeof (DIEFFECT);
       envelopeHolder.dwSize = sizeof (DIENVELOPE);
      // Set the key
      sprintf (effectName, "Effect%d", i);
      gForceFeedbackEffect[i] = NULL;
      gForceEffectIsRepeating[i] = FALSE;
      gForceEffectHasDirection[i] = FALSE;

      // Read all the common data for this effect
      effectHolder.dwFlags = GetPrivateProfileInt(effectName, "flags", 0, dataFileName);
      effectHolder.dwDuration = GetPrivateProfileInt(effectName, "duration", 0, dataFileName);
      effectHolder.dwSamplePeriod = GetPrivateProfileInt(effectName, "SamplePeriod", 0, dataFileName);
      effectHolder.dwGain = GetPrivateProfileInt(effectName, "gain", 0, dataFileName);
      effectHolder.dwTriggerButton = GetPrivateProfileInt(effectName, "triggerButton", 0, dataFileName);
      effectHolder.dwTriggerRepeatInterval = GetPrivateProfileInt(effectName, "triggerRepeatInterval", 0, dataFileName);
      effectHolder.cAxes = GetPrivateProfileInt(effectName, "numAxes", 0, dataFileName);
      gForceEffectHasDirection[i] = GetPrivateProfileInt(effectName, "hasDirection", 0, dataFileName);

      if (axesArray)
      {
         delete axesArray;
         axesArray = NULL;
      }

      if (dirArray)
      {
         delete dirArray;
         dirArray = NULL;
      }

      if (effectHolder.cAxes)
      {
         axesArray = new DWORD [effectHolder.cAxes];
         effectHolder.rgdwAxes = axesArray;

         dirArray = new long [effectHolder.cAxes];
         effectHolder.rglDirection = dirArray;

         for (j=0; j<effectHolder.cAxes; j++)
         {
            sprintf (str1, "axes%d", j);
            sprintf (str2, "direction%d", j);
            k = GetPrivateProfileInt(effectName, str1, 0, dataFileName);
            switch (k)
            {
               case 0:
                  effectHolder.rgdwAxes[j] = DIJOFS_X;
               break;

               case 1:
                  effectHolder.rgdwAxes[j] = DIJOFS_Y;
               break;

               case 2:
               effectHolder.rgdwAxes[j] = g_nThrottleID;
               break;

               case 4:
                  effectHolder.rgdwAxes[j] = DIJOFS_RX;
               break;

               case 5:
                  effectHolder.rgdwAxes[j] = DIJOFS_RY;
               break;

               case 6:
                  effectHolder.rgdwAxes[j] = DIJOFS_RZ;
               break;
            }

            effectHolder.rglDirection[j] = GetPrivateProfileInt(effectName, str2, 0, dataFileName);
         }
      }
      else
      {
         effectHolder.rgdwAxes = NULL;
         effectHolder.rglDirection = NULL;
      }

      // Check for envelope
      j = GetPrivateProfileInt(effectName, "envelopeAttackLevel", -1, dataFileName);
      if (j != 0xffffffff)
      {
         envelopeHolder.dwSize = sizeof (DIENVELOPE);
         envelopeHolder.dwAttackLevel = j;
         envelopeHolder.dwAttackTime = GetPrivateProfileInt(effectName, "envelopeAttackTime", 0, dataFileName);
         envelopeHolder.dwFadeLevel = GetPrivateProfileInt(effectName, "envelopeFadeLevel", 0, dataFileName);
         envelopeHolder.dwFadeTime = GetPrivateProfileInt(effectName, "envelopeFadeTime", 0, dataFileName);
         effectHolder.lpEnvelope = &envelopeHolder;
      }
      else
      {
         effectHolder.lpEnvelope = NULL;
      }

      effectType = GetPrivateProfileInt(effectName, "effectType", 0, dataFileName);
      switch (effectType)
      {
         case DIEFT_CUSTOMFORCE:
            customHolder.cChannels = GetPrivateProfileInt(effectName, "customForceChannels", 0, dataFileName);
            customHolder.dwSamplePeriod = GetPrivateProfileInt(effectName, "customForceSamplePeriod", 0, dataFileName);
            customHolder.cSamples = GetPrivateProfileInt(effectName, "customForceSamples", 0, dataFileName);

            if (forceData)
            {
               delete forceData;
               forceData = NULL;
            }
            forceData = new long[customHolder.cChannels * customHolder.cSamples];
            customHolder.rglForceData = forceData;

            for (j=0; j<customHolder.cChannels; j++)
            {
               for (k=0; k<customHolder.cSamples; k++)
               {
                  sprintf (str1, "customForceForceChannel%dSample%d", j, k);
                  customHolder.rglForceData[j*k + j] = GetPrivateProfileInt(effectName, str1, 0, dataFileName);
               }
            }

            // Add the sizes
            effectHolder.cbTypeSpecificParams = 3 * sizeof (DWORD) + sizeof (long) * customHolder.cChannels * customHolder.cSamples;
            effectHolder.lpvTypeSpecificParams = &customHolder;

            // enumerate for a custom force effect
            SetupResult = VerifyResult (joystickDevice->EnumEffects((LPDIENUMEFFECTSCALLBACK)JoystickEnumEffectTypeProc,
               &guidEffect, DIEFT_CUSTOMFORCE));

            guidEffect = GUID_Square;
            if (!SetupResult)
            {
               OutputDebugString("EnumEffects(Costum Force) failed\n");
               continue;
            }
         break;

         case DIEFT_PERIODIC:
            periodicHolder.dwMagnitude = GetPrivateProfileInt(effectName, "periodicForceMagnitude", 0, dataFileName);
            periodicHolder.lOffset = GetPrivateProfileInt(effectName, "periodicForceOffset", 0, dataFileName);
            periodicHolder.dwPhase = GetPrivateProfileInt(effectName, "periodicForcePhase", 0, dataFileName);
            periodicHolder.dwPeriod = GetPrivateProfileInt(effectName, "periodicForcePeriod", 0, dataFileName);
            gForceEffectIsRepeating[i] = TRUE;

            // Add the sizes
            effectHolder.cbTypeSpecificParams = 3 * sizeof (DWORD) + sizeof (long);
            effectHolder.lpvTypeSpecificParams = &periodicHolder;

            // enumerate for a periodic force effect

            // enumerate for a periodic force effect
            guidType = GetPrivateProfileInt(effectName, "periodicType", -1, dataFileName);
            switch (guidType)
            {
               case 0:
                  guidEffect = GUID_Sine;
               break;

               case 1:
                  guidEffect = GUID_Square;
               break;

               case 2:
                  guidEffect = GUID_Triangle;
               break;

               case 3:
                  guidEffect = GUID_SawtoothUp;
               break;

               case 4:
                  guidEffect = GUID_SawtoothDown;
               break;

               default:
                  SetupResult = VerifyResult (joystickDevice->EnumEffects((LPDIENUMEFFECTSCALLBACK)JoystickEnumEffectTypeProc,
                     &guidEffect, DIEFT_PERIODIC));
               break;
            }

            if (!SetupResult)
            {
               OutputDebugString("EnumEffects(Periodic Force) failed\n");
               continue;
            }
         break;

         case DIEFT_CONSTANTFORCE:
            constantHolder.lMagnitude = GetPrivateProfileInt(effectName, "constantForceMagnitude", 0, dataFileName);

            // Add the sizes
            effectHolder.cbTypeSpecificParams = sizeof (long);
            effectHolder.lpvTypeSpecificParams = &constantHolder;

            // enumerate for a constant force effect
            SetupResult = VerifyResult (joystickDevice->EnumEffects((LPDIENUMEFFECTSCALLBACK)JoystickEnumEffectTypeProc,
               &guidEffect, DIEFT_CONSTANTFORCE));

            if (!SetupResult)
            {
               OutputDebugString("EnumEffects(Constant Force) failed\n");
               continue;
            }
         break;

         case DIEFT_RAMPFORCE:
            rampHolder.lStart = GetPrivateProfileInt(effectName, "rampForceStart", 0, dataFileName);
            rampHolder.lEnd = GetPrivateProfileInt(effectName, "rampForceEnd", 0, dataFileName);

            // Add the sizes
            effectHolder.cbTypeSpecificParams = 2 * sizeof (long);
            effectHolder.lpvTypeSpecificParams = &rampHolder;

            // enumerate for a ramp force effect
            SetupResult = VerifyResult (joystickDevice->EnumEffects((LPDIENUMEFFECTSCALLBACK)JoystickEnumEffectTypeProc,
               &guidEffect, DIEFT_RAMPFORCE));

            if (!SetupResult)
            {
               OutputDebugString("EnumEffects(Ramp Force) failed\n");
               continue;
            }
         break;

         case DIEFT_CONDITION:
            if (conditionHolder)
            {
               delete conditionHolder;
               conditionHolder = NULL;
            }

            conditionHolder = new DICONDITION[effectHolder.cAxes];
            for (j=0; j<effectHolder.cAxes; j++)
            {
               conditionHolder[j].lOffset = GetPrivateProfileInt(effectName, "conditionOffset", 0, dataFileName);
               conditionHolder[j].lPositiveCoefficient = GetPrivateProfileInt(effectName, "conditionPositiveCoefficient", 0, dataFileName);
               conditionHolder[j].lNegativeCoefficient = GetPrivateProfileInt(effectName, "conditionNegativeCoefficient", 0, dataFileName);
               conditionHolder[j].dwPositiveSaturation = GetPrivateProfileInt(effectName, "conditionPositiveSaturation", 0, dataFileName);
               conditionHolder[j].dwNegativeSaturation = GetPrivateProfileInt(effectName, "conditionNegativeSaturation", 0, dataFileName);
               conditionHolder[j].lDeadBand = GetPrivateProfileInt(effectName, "conditionDeadband", 0, dataFileName);
            }

            // Add the size
            effectHolder.cbTypeSpecificParams = sizeof (DICONDITION) * effectHolder.cAxes;
            effectHolder.lpvTypeSpecificParams = conditionHolder;

            guidEffect = GUID_Spring;
         break;

         default:
            effectHolder.cbTypeSpecificParams = 0;
            effectHolder.lpvTypeSpecificParams = NULL;
         break;
      }

      // call CreateEffect()
      SetupResult = VerifyResult (joystickDevice->CreateEffect(guidEffect, &effectHolder,
         &gForceFeedbackEffect[i], NULL));
   }

   if (axesArray)
   {
      delete axesArray;
      axesArray = NULL;
   }

   if (dirArray)
   {
      delete dirArray;
      dirArray = NULL;
   }

   if (forceData)
   {
      delete forceData;
      forceData = NULL;
   }

   if (conditionHolder)
   {
      delete conditionHolder;
      conditionHolder = NULL;
   }

   return TRUE;
}

void JoystickReleaseEffects (void)
{
    if (!hasForceFeedback) return;
    // Get rid of any old effects
    ShiAssert(gNumEffectsLoaded > 0 && gNumEffectsLoaded < 20); // arbitrary for now JPO
    if (gNumEffectsLoaded > 20) return; // arbitray sanity

    for (int i=0; i<gNumEffectsLoaded; i++) {
//	gForceFeedbackEffect[i]->Unload();
	ShiAssert(FALSE == F4IsBadReadPtr(gForceFeedbackEffect[i], sizeof(*gForceFeedbackEffect[i])));
	if (F4IsBadReadPtr(gForceFeedbackEffect[i], sizeof(*gForceFeedbackEffect[i])))
	    continue;
	gForceFeedbackEffect[i]->Release();
    }
    delete[] gForceFeedbackEffect;
    gForceFeedbackEffect = NULL;
    delete[] gForceEffectIsRepeating;
    gForceEffectIsRepeating = NULL;
    delete[] gForceEffectHasDirection;
    gForceEffectHasDirection = NULL;
    gForceFeedbackEffect = NULL;
    gNumEffectsLoaded = 0;
}

void JoystickStopAllEffects (void)
{
int i;

   if (!hasForceFeedback)
      return;

   for (i=0; i<gNumEffectsLoaded; i++)
   {
		 if (i != g_nFFEffectAutoCenter)
      JoystickStopEffect (i);
   }
}

void JoystickStopEffect(int effectNum)
{
   if (!hasForceFeedback || effectNum >= gNumEffectsLoaded || !gForceFeedbackEffect || !gForceFeedbackEffect[effectNum])
      return;

   ShiAssert(FALSE == F4IsBadReadPtr(gForceFeedbackEffect[effectNum], sizeof *gForceFeedbackEffect[effectNum]));
   VerifyResult(gForceFeedbackEffect[effectNum]->Stop ());
}

int JoystickPlayEffect(int effectNum, int data)
{
DWORD           SetupResult;
DIEFFECT        diEffect;
LONG            rglDirections[2] = { 0, 0 };

   if (!hasForceFeedback || effectNum >= gNumEffectsLoaded || !gForceFeedbackEffect || !gForceFeedbackEffect[effectNum])
      return FALSE;

   ShiAssert(FALSE == F4IsBadReadPtr(&gForceEffectHasDirection[effectNum], sizeof gForceEffectHasDirection[effectNum]));
   ShiAssert(FALSE == F4IsBadReadPtr(gForceFeedbackEffect[effectNum], sizeof *gForceFeedbackEffect[effectNum]));
   // initialize DIEFFECT structure
   memset(&diEffect, 0, sizeof(DIEFFECT));
   diEffect.dwSize = sizeof(DIEFFECT);
   if (gForceEffectHasDirection[effectNum])
   {
      // set the direction
      // since this is a polar coordinate effect, we will pass the angle
      // in as the direction relative to the x-axis, and will leave 0
      // for the y-axis direction

      // Direction is passed in in degrees, we convert to 100ths
      // of a degree to make it easier for the caller.
      rglDirections[0]        = data * 100;
      diEffect.dwFlags        = DIEFF_OBJECTOFFSETS | DIEFF_POLAR;
      diEffect.cAxes          = 2;
      diEffect.rglDirection   = rglDirections;
      SetupResult = VerifyResult (gForceFeedbackEffect[effectNum]->SetParameters(&diEffect, DIEP_DIRECTION));

      if(!SetupResult)
      {
          OutputDebugString("SetParameters(Bounce effect) failed\n");
          return FALSE;
      }
   }

   if (effectNum == JoyRunwayRumble1 || effectNum == JoyRunwayRumble2)
   {
   DIPERIODIC periodicHolder;

      periodicHolder.dwMagnitude = 2000;
      periodicHolder.lOffset = 0;
      periodicHolder.dwPhase = 0;
      periodicHolder.dwPeriod = data;
      diEffect.cbTypeSpecificParams = sizeof (DIPERIODIC);
      diEffect.lpvTypeSpecificParams = &periodicHolder;
      SetupResult = VerifyResult (gForceFeedbackEffect[effectNum]->SetParameters(&diEffect, DIEP_TYPESPECIFICPARAMS));
      if(!SetupResult)
      {
          OutputDebugString("SetParameters(Runway Rumble) failed\n");
          return FALSE;
      }
   }

   if (effectNum == JoyAutoCenter)
   {
   DICONDITION conditionHolder[2];

      conditionHolder[0].lOffset = 0;
      conditionHolder[0].lPositiveCoefficient = data;
      conditionHolder[0].lNegativeCoefficient = data;
      conditionHolder[0].dwPositiveSaturation = data;
      conditionHolder[0].dwNegativeSaturation = data;
      conditionHolder[0].lDeadBand = 100;
      conditionHolder[1].lOffset = 0;
      conditionHolder[1].lPositiveCoefficient = data;
      conditionHolder[1].lNegativeCoefficient = data;
      conditionHolder[1].dwPositiveSaturation = data;
      conditionHolder[1].dwNegativeSaturation = data;
      conditionHolder[1].lDeadBand = 100;
      diEffect.cbTypeSpecificParams = sizeof (DICONDITION) * 2;
      diEffect.lpvTypeSpecificParams = conditionHolder;
      SetupResult = VerifyResult (gForceFeedbackEffect[effectNum]->SetParameters(&diEffect, DIEP_TYPESPECIFICPARAMS));
      if(!SetupResult)
      {
          OutputDebugString("SetParameters(Auto Center) failed\n");
          return FALSE;
      }
   }

   // play the effect
   SetupResult = VerifyResult (gForceFeedbackEffect[effectNum]->Start (1, 0));
   if(!SetupResult)
   {
       //JoystickCreateEffect(1);
       MonoPrint("Start Effect %d Failed\n", effectNum);
       return FALSE;
   }
   MonoPrint("Started effect %d\n", effectNum);

   return TRUE;
}

BOOL CALLBACK JoystickEnumEffectTypeProc(LPCDIEFFECTINFO pei, LPVOID pv)
{
    GUID *pguidEffect = NULL;

    // validate pv
    // BUGBUG

    // report back the guid of the effect we enumerated
    if(pv)
    {

        pguidEffect = (GUID *)pv;

        *pguidEffect = pei->guid;

    }

    // BUGBUG - look at this some more....
    return DIENUM_STOP;

}
