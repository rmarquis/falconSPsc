#include "falclib.h"
#include "dispcfg.h"
#include "f4thread.h"

#include "sinput.h"
#include "cpmanager.h"
#include "otwdrive.h"
#include "playerop.h"
#include "simio.h"
#include "dispopts.h"
#include "simdrive.h"
#include "aircrft.h"

// OW FIXME: this had to be added after installing the DX8 Beta 1 SDK
extern "C" HRESULT WINAPI DirectInputCreateEx(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID *ppvOut, LPUNKNOWN punkOuter);

BOOL						gWindowActive;
BOOL						gOccupiedBySim;
LPDIRECTINPUT7				gpDIObject;
HANDLE						gphDeviceEvent[SIM_NUMDEVICES];
LPDIRECTINPUTDEVICE7			gpDIDevice[SIM_NUMDEVICES];
BOOL						gpDeviceAcquired[SIM_NUMDEVICES];
BOOL						gSimInputEnabled = FALSE;
BOOL						gDIEnabled = FALSE;
extern int NoRudder;

int gCurController = SIM_JOYSTICK1;
extern int g_nThrottleID;		// OW

void SetupInputFunctions (void);
void CleanupInputFunctions (void);

// Callback for joystick enumeration
BOOL FAR PASCAL InitJoystick(LPCDIDEVICEINSTANCE pdinst, LPVOID pvRef);
void JoystickStopAllEffects (void);

DWORD nextJoyRead = 0;
DWORD minJoyReadInterval = 84; //12 times pre second

extern int NumHats;


//*******************************************
//	void InputCycle()
// One iteration of the input loop.  This
// may now poke directly into Sim/Graphics
// state, so it _must_ be run on the Sim
// thread.
//*******************************************

void InputCycle(void)
{
	DWORD dw;
	HWND	hWnd;
	
	hWnd = FalconDisplay.appWin;
	
	if(gWindowActive){//check for focus
		// Always be sure we have these devices acquired if we have focus 
		AcquireDeviceInput(SIM_KEYBOARD, TRUE);
		//AcquireDeviceInput(gCurController, TRUE);
		AcquireDeviceInput(SIM_MOUSE, TRUE);
		if(!CheckDeviceAcquisition(SIM_MOUSE)){
			AcquireDeviceInput(SIM_MOUSE, TRUE);
		}
	}
	
	
	// Went to Wait for single to make sure both keyboard and mouse are processed
	// each time, instead of one or the other
	//dw = MsgWaitForMultipleObjects(SIM_NUMDEVICES - 1, gphDeviceEvent, 0, 0, QS_ALLINPUT); //VWF Kludge until DINPUT supports joysticks

	dw = WaitForSingleObject(gphDeviceEvent[SIM_MOUSE],0);
	if(dw == WAIT_OBJECT_0)
		OnSimMouseInput(FalconDisplay.appWin);
	
	dw = WaitForSingleObject(gphDeviceEvent[SIM_KEYBOARD],0);
	if(dw == WAIT_OBJECT_0)
		OnSimKeyboardInput();

	//We need to simply read the joystick every time through
	//using a minimum read interval for faster computers
	//if(GetTickCount() >= nextJoyRead)
	{
		GetJoystickInput();
		ProcessJoyButtonAndPOVHat();
		nextJoyRead = GetTickCount() +  minJoyReadInterval;
	}
}

//*******************************************
//	void NoInputCycle()
// One iteration of the input loop.  This
// may now poke directly into Sim/Graphics
// state, so it _must_ be run on the Sim
// thread. - simulates no inputs
//*******************************************

void NoInputCycle(void)
{
	unsigned int i;

	IO.analog[0].engrValue = 0.0f;
	IO.analog[1].engrValue = 0.0f;
	// ReadThrottle(); - don't read throttle 10000 times a second
	IO.analog[3].engrValue = 0.0f;

	for(i=0;i<SIMLIB_MAX_DIGITAL;i++)
		IO.digital[i] = FALSE;
	
	for(i=0; i<gCurJoyCaps.dwPOVs;i++)
		IO.povHatAngle[i] = (unsigned long)-1;	
}


//***********************************
//	BOOL SetupDIMouseAndKeyboard()
//***********************************

//BOOL SetupDIMouseAndKeyboard(HINSTANCE hInst, HWND hWnd){
BOOL SetupDIMouseAndKeyboard(HINSTANCE, HWND hWnd){
	
	BOOL	SetupResult = TRUE;
	BOOL	MouseSetupResult;
	BOOL	KeyboardSetupResult;
	BOOL	CursorSetupResult;
#if 1
#endif
	if(!gDIEnabled)
	{
		ShiAssert(gDIEnabled != FALSE);
		return FALSE;
	}
	
	gOccupiedBySim		= TRUE;
	gyFuzz				= gxFuzz = 0;
	gxPos					= DisplayOptions.DispWidth/2;
	gyPos					= DisplayOptions.DispHeight/2;
	gMouseSensitivity	= NORM_SENSITIVITY;
	gWindowActive		= TRUE;
	
	// Register with DirectInput and get an IDirectInput to play with
	
	DIPROPDWORD dipdw = {{sizeof(DIPROPDWORD), sizeof(DIPROPHEADER), 0, DIPH_DEVICE}, DINPUT_BUFFERSIZE};
	
	MouseSetupResult	= SetupDIDevice(hWnd, TRUE, SIM_MOUSE, GUID_SysMouse, &c_dfDIMouse, &dipdw); // Mouse
	KeyboardSetupResult	= SetupDIDevice(hWnd, FALSE, SIM_KEYBOARD, GUID_SysKeyboard, &c_dfDIKeyboard, &dipdw); // Keyboard
	
#if 1
	
#endif
	
	CursorSetupResult		= CreateSimCursors();
	
	// JB 010618 Disable these messages as they don't appear to do anything useful
	/*
	if(!KeyboardSetupResult){
		DIMessageBox(999, MB_OK, SSI_NO_KEYBOARD_INIT);
		SetupResult = FALSE;
	}
	
	if(SetupResult && !MouseSetupResult){
		SetupResult = DIMessageBox(999, MB_YESNO, SSI_NO_MOUSE_INIT);
	}
	else if (MouseSetupResult)
	{
		//      ShowCursor(FALSE);
	}
*/	
#if 1
#endif	
	
	if(SetupResult && !CursorSetupResult){
		SetupResult = DIMessageBox(999, MB_YESNO, SSI_NO_CURSOR_INIT);
	}
	
	if(SetupResult){
		gSelectedCursor = 0; //VWF Kludge for now
	}
	
	if(SetupResult){
		gSimInputEnabled = TRUE;
	}
	else{
		gSimInputEnabled = FALSE;
	}
	
	SetupInputFunctions ();
	
	gTimeLastMouseMove = vuxRealTime;
	
	return (SetupResult);
}

//***********************************
//	BOOL SetupDIJoystick()
//***********************************

BOOL SetupDIJoystick(HINSTANCE hInst, HWND hWnd)
{
	BOOL	SetupResult = TRUE;
	BOOL	JoystickSetupResult;
	GUID	joystick;
	DIDEVICEINSTANCE devinst;
	DIDEVICEOBJECTINSTANCE devobj;
	HRESULT hres;
		
	devobj.dwSize = sizeof(DIDEVICEOBJECTINSTANCE);
	devinst.dwSize = sizeof(DIDEVICEINSTANCE);

	gCurJoyCaps.dwSize = sizeof(DIDEVCAPS);

	gDIEnabled = VerifyResult(DirectInputCreateEx(hInst, DIRECTINPUT_VERSION, IID_IDirectInput7, (void **) &gpDIObject, NULL));
	if(!gDIEnabled)
		return gDIEnabled;

	gpDIObject->EnumDevices(DIDEVTYPE_JOYSTICK,InitJoystick, &hWnd, DIEDFL_ATTACHEDONLY);
	
	gCurController = SIM_JOYSTICK1;

	if(gTotalJoy)
	{
		joystick = PlayerOptions.GetJoystick();
		for(int i = 0; (i < SIM_NUMDEVICES - SIM_JOYSTICK1) && gpDIDevice[SIM_JOYSTICK1 + i]; i++)
		{
			gpDIDevice[SIM_JOYSTICK1 + i]->GetDeviceInfo(&devinst);
			if(!memcmp(&joystick, &devinst.guidInstance,sizeof(GUID)) )
			{
				gCurController = SIM_JOYSTICK1 + i;
				break;
			}
		}

		IO.SetAnalogIsUsed(0, TRUE);
		IO.SetAnalogIsUsed(1, TRUE);

		hres = gpDIDevice[gCurController]->GetObjectInfo(&devobj, g_nThrottleID, DIPH_BYOFFSET);
		if(SUCCEEDED(hres))
			IO.SetAnalogIsUsed(2, TRUE);
		else
			IO.SetAnalogIsUsed(2, FALSE);

		hres = gpDIDevice[gCurController]->GetObjectInfo(&devobj,DIJOFS_RZ,DIPH_BYOFFSET);
		if(SUCCEEDED(hres) && !NoRudder)
			IO.SetAnalogIsUsed(3, TRUE);
		else
			IO.SetAnalogIsUsed(3, FALSE);

		if(!IO.ReadFile() || memcmp(&joystick, &devinst.guidInstance,sizeof(GUID)))
		{
			IO.analog[0].center = 0;
			IO.analog[1].center = 0;
			IO.analog[2].center = 3000;
			IO.analog[3].center = 0;
		}

		gpDIDevice[gCurController]->GetCapabilities(&gCurJoyCaps);

		if(NumHats != -1)
			gCurJoyCaps.dwPOVs = NumHats;
		else
			gCurJoyCaps.dwPOVs = max(1, gCurJoyCaps.dwPOVs);

		JoystickSetupResult = VerifyResult(gpDIDevice[gCurController]->Acquire());
		gpDeviceAcquired[gCurController] = JoystickSetupResult;

		// JB 011023 Doesn't seem to matter if this fails.
		//if(SetupResult && !JoystickSetupResult){
		//	SetupResult = DIMessageBox(999, MB_YESNO, SSI_NO_JOYSTICK_INIT);
		//}
	}
	else
	{
		MonoPrint("No Joysticks found. It would be useful to have one!! :)\n");
	}

	gDIEnabled = TRUE;

	return gDIEnabled;

}

//**********************************************************
//	BOOL CleanupDIMouseAndKeyboard()
// Closes all input devices and terminates the input thread.
//**********************************************************

BOOL CleanupDIMouseAndKeyboard(){
	BOOL CleanupResult = TRUE;

	AcquireDeviceInput(SIM_MOUSE, FALSE);
	AcquireDeviceInput(SIM_KEYBOARD, FALSE);

	CleanupResult = CleanupResult && CleanupDIDevice(SIM_MOUSE);
	CleanupResult = CleanupResult && CleanupDIDevice(SIM_KEYBOARD);

	CleanupSimCursors();

	gOccupiedBySim = FALSE;
	gSimInputEnabled = FALSE;

	//	ShowCursor(TRUE);

	//CleanupInputFunctions();
	return(CleanupResult);
}

//**********************************************************
//	BOOL CleanupDIJoystick()
// Closes all input devices and terminates the input thread.
//**********************************************************

BOOL CleanupDIJoystick(void)
{
	BOOL CleanupResult = TRUE;
   DIPROPDWORD DIPropAutoCenter;

   JoystickStopAllEffects();

	JoystickReleaseEffects();
	AcquireDeviceInput(SIM_JOYSTICK1, FALSE);
   // Reenable stock auto-center
   DIPropAutoCenter.diph.dwSize = sizeof(DIPropAutoCenter);
   DIPropAutoCenter.diph.dwHeaderSize = sizeof(DIPROPHEADER);
   DIPropAutoCenter.diph.dwObj = 0;
   DIPropAutoCenter.diph.dwHow = DIPH_DEVICE;
   DIPropAutoCenter.dwData = DIPROPAUTOCENTER_ON;

	if(gpDIDevice[SIM_JOYSTICK1 + gTotalJoy - 1])
		gpDIDevice[SIM_JOYSTICK1 + gTotalJoy - 1]->SetProperty(DIPROP_AUTOCENTER, &DIPropAutoCenter.diph);

	while(gTotalJoy > 0)
	{
		CleanupResult = CleanupResult && CleanupDIDevice(SIM_JOYSTICK1 + gTotalJoy - 1);
		delete gDIDevNames[SIM_JOYSTICK1 + gTotalJoy - 1];
		gDIDevNames[SIM_JOYSTICK1 + gTotalJoy - 1] = NULL;
		gTotalJoy--;
	}

	gOccupiedBySim = FALSE;
	gSimInputEnabled = FALSE;

	return(CleanupResult);
}

void CleanupDIAll(void)
{
//    CleanupDIMouseAndKeyboard();
    CleanupDIJoystick();
    if (gpDIObject)
	gpDIObject->Release();
    gpDIObject = NULL;
}
