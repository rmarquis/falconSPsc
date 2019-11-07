//#define _USE_SECRET_CODE_ 1

#pragma warning(disable:4100)
#pragma warning(disable:4663)
#pragma warning(disable:4511)
#pragma warning(disable:4512)
#pragma warning(disable:4018)
#pragma warning(disable:4245)

#include "f4version.h"
#include <windows.h>
#include <time.h>
#include <stdio.h>
#include <direct.h>
#include "Statistics.h"
#include "FalcLib.h"
#include "resource.h"
#include "stdhdr.h"
#include "ClassTbl.h"
#include "Entity.h"
#include "camp2sim.h"
#include "f4find.h"
#include "hud.h"
#include "otwdrive.h"
#include "simobj.h"
#include "simDrive.h"
#include "simLoop.h"
#include "falcmesg.h"
#include "fsound.h"
#include "sms.h"
#include "Graphics\Include\imagebuf.h"
#include "movie\avimovie.h"
#include "f4comms.h"
#include "FalcSnd\psound.h"
#include "FalcSnd\voicemapper.h"
#include "CampStr.h"
#include "find.h"
#include "misseval.h"
#include "cmpclass.h"
#include "dispcfg.h"
#include "falcuser.h"
#include "userids.h"
#include "ui95\chandler.h"
#include "sinput.h"
#include "CmpClass.h"
#include "ThreadMgr.h"
#include "feature.h"
#include "falcmem.h"

#ifdef USE_SH_POOLS
#include "SmartHeap\Include\smrtheap.h"
#endif

#include "Weather.h"
#include "Campaign.h"
#include "playerop.h"
#include "simio.h"
#include "codelib\resources\reslib\src\resmgr.h"
#include "inpFunc.h"
#include "logbook.h"
#include "rules.h"
#include "iaction.h"
#include "CampJoin.h"
#include "TimerThread.h"
#include "ascii.h"
#include "ehandler.h"
#include "DispOpts.h"
#include "rules.h"
#include "openfile.h"
#include "VRInput.h"
#include "Theaterdef.h"

extern "C"
{
	#include "amdlib.h"
}

#include "ddraw.h"
//#include <crtdbg.h>

#pragma warning(default:4100)
#pragma warning(default:4663)
#pragma warning(default:4511)
#pragma warning(default:4512)
#pragma warning(default:4018)
#pragma warning(default:4245)

// OW
bool g_bHas3DNow = false;
extern bool g_bEnumSoftwareDevices;
bool g_bEnableCockpitVerifier = false;
bool g_writeSndTbl = false;
bool g_writeMissionTbl = false;
extern void ReadFalcon4Config();

// Begin - Uplink stuff
#include "include\comsup.h"

#pragma warning(disable:4192)
#import "gnet\bin\core.tlb"
#import "gnet\bin\shared.tlb" named_guids
#pragma warning(default:4192)

#include <atlbase.h>
CComModule _Module;
#include <atlcom.h>

BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()

struct __declspec(uuid("41C27D56-3A03-4E9D-BE01-3423126C3983")) GameSpyUplink;
GNETCORELib::IUplinkPtr m_pUplink;

#include <winsock2.h>
WSADATA wsadata;
extern "C" int InitWS2(WSADATA *wsaData);


extern bool g_bEnableUplink;
extern char g_strMasterServerName[0x40];
extern int g_nMasterServerPort;
extern char g_strServerName[0x40];
extern char g_strServerLocation[0x40];
extern char g_strServerAdmin[0x40];
extern char g_strServerAdminEmail[0x40];
// End - Uplink stuff


void DoRecoShit (void);

extern int HighResolutionHackFlag;		// From Graphics\DDStuff\DevMgr.cpp
extern uchar gCampJoinTries;

// Campaign tool includes
#include "dialog.h"

// UI Includes
#include "uicomms.h"
#include "ui_ia.h"
#undef fopen
#undef fclose

#pragma warning (disable : 4127)	// Conditional Expression is Constant

char top_space[] = "                                                                               ";
char program_name[] = "    ****    Falcon 4.0    ****    ";
char legal_crap[] = "    ****    (c)1998 MicroProse, Inc. All Rights Reserved.    ****    ";
char bottom_space[] = "                                                                               ";

extern int	voice_;
extern int GraphicSettingMult;
extern char	gUI_CampaignFile[];
extern char gUI_AutoSaveName[];
extern int gCampDataVersion,gCurrentDataVersion,gClearPilotInfo,gTacticalFullEdit;	
static int i_am (char *with);

extern "C"
{
	extern int ComIPGetHostIDIndex;
	extern int force_ip_address;
}

void PlayThatFunkyMusicWhiteBoy();
void load_voice_recognition_demo_sound_file (void);
extern void EnableCampaignMenus (void);
extern void DisableCampaignMenus (void);
extern void CampaignPreloadSuccess (int remote);
extern void CampaignJoinSuccess (void);
extern void CampaignJoinFail (void);
extern void DisplayJoinStatusWindow (int);
extern void ServerBrowserExit();
extern BOOL WINAPI BriefDialog(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
BOOL DoSimOptions (HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
extern char	*BSP;
extern char	*BTP;
extern long MovieCount;
extern int MainLastGroup;
extern int flag_keep_smoke_trails;
extern int gTimeModeServer;
extern int gUnlimitedAmmo;
extern float UR_HEAD_VIEW;
int FileVerify (void);
extern void LoadTrails();

LRESULT CALLBACK PlayVoicesProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK SimWndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
extern void UIScramblePlayerFlight(void);
void PlayMovie(char *filename,int left,int top,int w,int h,void *theSurface);
//!void PlayMovie(char *filename,short left,short top,short w,short h,UInt theSurface);
extern void PlayUIMovieQ(); // defined in UI_Main.cpp
extern BOOL ReadyToPlayMovie; // defined in UI_Cmpgn.cpp
#ifdef DEBUG
extern int gCampPlayerInput;
#endif
extern C_SoundBite *gInstantBites,*gDogfightBites,*gCampaignBites;
extern long CampEventSoundID;
//extern void TAC_REF_XrefCB2(long ID);
extern void UpdateMissionWindow(long ID);
extern void update_tactical_flight_information (void);
extern void CopyDFSettingsToWindow (void);
extern void CheckCampaignFlyButton (void);
extern void GameHasStarted(void);

void RebuildCurrentWPList();
void UI_HandleAirbaseDestroyed();
void UI_HandleAirbaseDestroyed();
void UI_HandleFlightCancel();
void UI_HandleAircraftDestroyed();
void UI_UpdateOccupationMap();
void OpenTEGameOverWindow();
void ProcessChatStr(CHATSTR *msg);

void RebuildGameTree();
void UI_UpdateDogfight(long winID,short Setting); // LParam=Window,wParam=Setting
void UI_UpdateGameList();
void EndCommitCB(long ID,short hittype,C_Base *control);
void ReplayUIMovie(long MovieID);
void OpenMainCampaignCB(long ID,short hittype,C_Base *control);
void ViewRemoteLogbook(long playerID);
void RelocateSquadron();
void ShutdownCampaign (void);
int tactical_is_training (void);
class tactical_mission;
void tactical_restart_mission (void);

int noUIcomms = FALSE;
char FalconMovieDirectory[_MAX_PATH];
char FalconMovieMode[_MAX_PATH];
int displayCampaign = FALSE;
int studlyCampaignDude = FALSE;
int RepairObjective = FALSE;
int DestroyObjective = FALSE;
int ClearObjManualFlags = FALSE;
int doUI = FALSE;
int wait_for_loaded = TRUE;
int eyeFlyEnabled = FALSE;
static int lTestVar = TRUE;
int NoRudder = FALSE;
int DisableSmoothing = FALSE;
int NumHats = -1;

static int KeepFocus=0;
char FalconUIArtDirectory[_MAX_PATH];
char FalconUIArtThrDirectory[_MAX_PATH];
char FalconUISoundDirectory[_MAX_PATH];
char FalconSoundThrDirectory[_MAX_PATH];

extern ulong	gCampJoinLastData;		// Last vuxRealtime we received data about this game

extern char FalconPictureDirectory[_MAX_PATH]; // JB 010623
extern void LoadTheaterList(); // JPO

//int tactical_enable_motion = TRUE;

#ifdef NDEBUG
int auto_start = TRUE;
int intro_movie = TRUE;
#else
int auto_start = FALSE;
int intro_movie = FALSE;
#endif

// Debug Assert softswitches
#ifdef DEBUG
int f4AssertsOn = TRUE, f4HardCrashOn = FALSE;
int shiAssertsOn= TRUE, 
	shiWarningsOn = TRUE, // JB 010325
	shiHardCrashOn= FALSE;
#endif

#ifdef DEBUG
extern int gPlayerPilotLock;
#endif

static int numProcessors;
static HACCEL hAccel;
HWND mainMenuWnd;
HWND mainAppWnd;
int doNetwork = FALSE;	// referred in splash.cpp

static int numZips = 0;
static int* resourceHandle;
static void ParseCommandLine (LPSTR cmdLine);
static void SystemLevelInit (void);
static void SystemLevelExit (void);
static void CtrlAltDelMask( int state );
void ConsoleWrite (char *);

HINSTANCE hInst;
extern void CampMain (HINSTANCE hInstance, int nCmdShow);
extern void ReadCampAIInputs (char * name);
extern BOOL CALLBACK SelectMission(HWND, UINT, WPARAM, LPARAM);
extern BOOL WINAPI SelectSquadron(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
extern void CampaignConnectionTimer (void);

extern CampaignTime gConnectionTime;
extern CampaignTime gResendTime;
extern int gCampJoinStatus;

void UIMain (void);
void UI_LoadSkyWeatherData();
int UI_Startup();
void UI_Cleanup();
extern C_Handler *gMainHandler;
extern long gScreenShotEnabled;
void UI_UpdateVU();
void RecieveScenarioInfo();

void UI_CommsErrorMessage(WORD error);
void LeaveDogfight();
BOOL VersionInfo=FALSE;

void STPRender(C_Base *control);
void UpdateRules(void);
BOOL CleanupDIJoystick(void);
BOOL SetupDIJoystick(HINSTANCE hInst, HWND hWnd);
void SetVoiceVolumes(void);
void IncDecTalkerToPlay(int delta);
void IncDecMsgToPlay(int delta);
void IncDecDataToPlay(int delta);

char SecretCode[] = "SecretCodeGoesHere";     //8/3/97
char lTestVarString[] = "JustForGilman1";
#ifdef _USE_SECRET_CODE_
BOOL VersionData=FALSE;
char PetersSecretCode [] = "ereHseoGedoCterceS"; // SecretCodeGoesHere (backwords)
#endif // _USE_SECRET_CODE_

int MajorVersion = F4MajorVersion;
int MinorVersion = F4MinorVersion;
//int BuildNumber  = Charmaker(F4BuildNumber);
int BuildNumber  = F4BuildNumber;

extern BOOL SaveSFXTable();
extern BOOL WriteMissionData();

//used to display version number in game (not part of version system)
int ShowVersion  = 0;

                                                     
#define ZIPFILE_NAME    "ziplist.fil"
#ifdef CAMPTOOL
// Renaming tool stuff
extern VU_ID_NUMBER RenameTable[65536];
extern int gRenameIds;
// Window handles
extern HWND hMainWnd;
extern HWND hToolWnd;
#endif
falcon4LeakCheck flc;
extern HRESULT  StartServer( HWND hDlg );//me123
extern void StopVoice ();
void BuildAscii()
{
	short i,kbd,scan;
	short vkey,shiftstates;

	for(i=31;i<256;i++)
	{
		vkey=VkKeyScan(static_cast<char>(i));
		if(vkey != -1)
		{
			kbd = static_cast<short>(MapVirtualKey(vkey,2) & 0xff);

			if(kbd)
			{
				shiftstates = static_cast<short>((vkey >> 8) & 0x07);
				scan		= static_cast<short>(MapVirtualKey(vkey,0) & 0xff);

				Key_Chart[scan].Ascii[shiftstates]= static_cast<char>(i);
				Key_Chart[scan].Flags[shiftstates] |= _IS_ASCII_;
				if(i >= '0' && i <= '9')
					Key_Chart[scan].Flags[shiftstates] |= _IS_DIGIT_;
				if(isalpha(i))
					Key_Chart[scan].Flags[shiftstates] |= _IS_ALPHA_;
			}
		}
	}
}

static BOOLEAN initApplication( HINSTANCE hInstance, HINSTANCE hPrevInstance, int)
{
   WNDCLASS    wc;
	BOOL rc;
#ifdef _USE_SECRET_CODE_
    struct tm expirationDate = { 0, 0, 0, 16, 7, 97 };
    time_t expirationTime = mktime(&expirationDate);
    time_t curTime = time(NULL);
    if (curTime > expirationTime)
       return FALSE;
#endif _USE_SECRET_CODE_

   if (!hPrevInstance)
   {
      wc.style = CS_HREDRAW | CS_VREDRAW;
      wc.lpfnWndProc = (WNDPROC)SimWndProc; // The client window procedure.
      wc.cbClsExtra = 0;                     // No room reserved for extra data.
      wc.cbWndExtra = sizeof(DWORD);
      wc.hInstance = hInstance;
//      wc.hIcon = LoadIcon (hInstance, "ICON1.ICO");
      wc.hIcon = LoadIcon (hInstance, MAKEINTRESOURCE(IDI_ICON1));	// OW BC
      wc.hCursor = NULL;
      wc.hbrBackground = (HBRUSH)GetStockObject (WHITE_BRUSH);
      wc.lpszMenuName = MAKEINTRESOURCE(F4_DEMO_MENU);
      wc.lpszClassName = "Falcon4Class";
      rc = RegisterClass( &wc );
      if( !rc )
      {
        return FALSE;
      }
   }

   mainMenuWnd= CreateWindow ("Falcon4Class",
                        "F4:OS Debug Window",
                        WS_OVERLAPPEDWINDOW,
                        720,
                        100,
                        240,
                        100,
                        NULL,
                        NULL,
                        hInstance,
                        NULL);

#ifndef NDEBUG
   ShowWindow (mainMenuWnd, SW_SHOW);
	if (auto_start)
	{
// OW this causes some strange behaviour when the escape key is pressed for the first time
//	   ShowWindow (mainMenuWnd, SW_MINIMIZE);
	}
#endif

   hAccel = LoadAccelerators (hInstance, MAKEINTRESOURCE (IDR_FALCON4_ACC1));

/*
	// OW
	if(FAILED(CoInitialize(NULL))
		return FALSE;

	_Module.Init(ObjectMap, hInstance);
	_Module.dwThreadID = GetCurrentThreadId();
*/
  return TRUE;

} /* initApplication */


/*************************************************************************\
*
*  FUNCTION: WinMain(HANDLE, HANDLE, LPSTR, int)
*
*  PURPOSE: calls initialization function, processes message loop
*
*  COMMENTS:
*
\*************************************************************************/
int PASCAL HandleWinMain ( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR  lpCmdLine,
   int nCmdShow)
{
	char tmpPath[_MAX_PATH];
	MSG  msg;
	char buf[60],title[60];
	char fileName[_MAX_PATH];
	FILE *testopen;

	#ifdef _DEBUG
//	_CrtSetBreakAlloc(739752);
	// JPO in Debug - set up to check lots of memory
	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF| // Check for memory leaks
	    _CRTDBG_ALLOC_MEM_DF| // debug heap allocations
	    // _CRTDBG_DELAY_FREE_MEM_DF| // Check for use of free memory - expensive
	    0 // just so it looks nice.
	    );
	#endif

    //_Module.Init(ObjectMap, hInstance, &LIBID_SIMPLECLIENTLib);
    _Module.Init(ObjectMap, hInstance);
    // _Module.dwThreadID = GetCurrentThreadId();

	InitWS2(&wsadata);	// OW init Winsock now, we need it for GNet

	char strVersion[0x20];
	sprintf(strVersion,"%1d.%02d.%1d.%5d",MajorVersion, MinorVersion, gLangIDNum, BuildNumber);

	// OW
	g_bHas3DNow = has3DNow() ? true : false;		// detect 3dnow support

	#ifdef _DEBUG
	if(g_bHas3DNow)
		MonoPrint("3DNow detected\n");
	#endif

	ReadFalcon4Config();	// OW: read config file

	HRESULT hr = CoInitialize(NULL);
	if(FAILED(hr))
		MonoPrint("HandleWinMain: Error 0x%X occured during COM initialization!", hr);

	// Begin - Uplink stuff
	try
	{
		if(g_bEnableUplink)
		{
			// Make sure all objects are registered
			ComSup::RegisterServer("GNGameSpy.dll");
			ComSup::RegisterServer("GNCorePS.dll");
			ComSup::RegisterServer("GNShared.dll");

			// Create Uplink service object
			CheckHR(m_pUplink.CreateInstance(__uuidof(GameSpyUplink)));

			m_pUplink->PutMasterServerName(g_strMasterServerName);
			m_pUplink->PutMasterServerPort(g_nMasterServerPort);
			m_pUplink->PutQueryPort(7778);
			m_pUplink->PutHeartbeatInterval(60000);
			m_pUplink->PutServerVersion(strVersion);
			m_pUplink->PutServerVersionMin(strVersion);
			m_pUplink->PutServerLocation(g_strServerLocation);
			m_pUplink->PutServerName(g_strServerName);
			m_pUplink->PutGameName("Falcon4");
			m_pUplink->PutGameMode("openplaying");
		}
	}

	catch(_com_error e)
	{
		MonoPrint("HandleWinMain: Error 0x%X occured during JetNet initialization!", e.Error());
	}
	// End - Uplink stuff

	/*
	* set up data associated with this window
	*/
	
#ifdef USE_SMART_HEAP
	MemRegisterTask();
#endif

#ifdef _MSC_VER
	// Set the FPU to Truncate
	_controlfp(_RC_CHOP,MCW_RC);
	
	// Set the FPU to 24bit precision
	_controlfp(_PC_24,MCW_PC);
#endif

/* OW	
	// Kus Kludge...
	
	HINSTANCE	gr3dfx, mprp2, mprd3d, gen;
	
	gr3dfx = LoadLibrary( "gr3dfx.mpr" );
	gen = LoadLibrary( "gen.mpr" );
	mprd3d = LoadLibrary( "mprd3d.mpr" );
	mprp2 = LoadLibrary( "mprp2.mpr" );
*/
	
	hInst = hInstance;
//!	int Done=0;
	
	ParseCommandLine (lpCmdLine);
	
#ifdef VOICE_INPUT
	// Read in voice recognition crap
	InitVoiceRecognitionTopicsFiles(FalconDataDirectory);
#endif

	lTestVar = !strncmp (lTestVarString, "JustForGilman", 13);
	// PW Kludge
	if(VersionInfo)
	{
		int hCrt;
		FILE *hf;
		
		// Hack to make printf work
		AllocConsole();
		hCrt = _open_osfhandle(
			(long) GetStdHandle(STD_OUTPUT_HANDLE),
			_O_TEXT
			);
		hf = _fdopen( hCrt, "w" );
		*stdout = *hf;
		setvbuf( stdout, NULL, _IONBF, 0 );
		
		
		sprintf(title,"Falcon 4.0 - Version %1d.%02d.%1d.%5d",MajorVersion,MinorVersion,gLangIDNum,BuildNumber);
		
#ifdef NDEBUG
		strcpy(buf,"Release Mode");
#else
		strcpy(buf,"Debug Mode");
#endif // NDEBUG
		
#ifdef _USE_SECRET_CODE_
		if(VersionData)
		{
			int i;
			for(i=0;i<18;i++)
				sprintf(&buf[i*2],"%02x\n",SecretCode[i]);
		}
#endif // _USE_SECRET_CODE_
		printf ("%s:%s\n", title, buf);
		return(FALSE);
	}
	
	TheWeather = new WeatherClass();
	// This SHOULD NOT BE REQUIRED -- IT IS _VERY_ EASY TO BREAK CODE THAT DEPENDS ON THIS
	// I'd like to make it go away soon....
	SetCurrentDirectory(FalconDataDirectory);

	FileVerify();
	
	sprintf (FalconCampaignSaveDirectory, "%s\\Campaign\\Save", FalconDataDirectory);
	sprintf (FalconCampUserSaveDirectory, "%s\\Campaign\\Save", FalconDataDirectory);
	
	sprintf (FalconPictureDirectory, "%s\\Pictures", FalconDataDirectory); // JB 010623

	// MN Create PictureDir if not present
	_mkdir(FalconPictureDirectory);

	// Test for CD stuff
	{
		char buffer[MAX_PATH];

		EnableOpenTest();
		sprintf(buffer,"%s\\terrain\\theater.map",FalconTerrainDataDir);
		testopen=FILE_Open(buffer,"r");
		if(!testopen)
			exit(-1);
		fclose(testopen);
		sprintf(buffer,"%s\\falcon4.ini",FalconObjectDataDir);
		testopen=FILE_Open(buffer,"r");
		if(!testopen)
			exit(-1);
		fclose(testopen);
		sprintf(buffer,"%s\\smoketrail.gif",FalconMiscTexDataDir);
		testopen=FILE_Open(buffer,"r");
		if(!testopen)
			exit(-1);
		fclose(testopen);
		DisableOpenTest();
	}

   ResInit(NULL);
   ResCreatePath (FalconDataDirectory, FALSE);
   ResAddPath (FalconCampaignSaveDirectory, FALSE);
   sprintf (tmpPath, "%s\\Zips", FalconDataDirectory);
   ResAddPath (tmpPath, FALSE);
   sprintf (tmpPath, "%s\\Config", FalconDataDirectory);
   ResAddPath (tmpPath, FALSE);
   sprintf (tmpPath, "%s\\Art", FalconDataDirectory);	// This one can go if zips are always used
   ResAddPath (tmpPath, TRUE);
   sprintf (tmpPath, "%s", FalconPictureDirectory); // JB 010623
   ResAddPath (tmpPath, TRUE); // JB 010623
   sprintf (tmpPath, "%s\\sim", FalconDataDirectory);	// JPO - so we can find raw sim files
   ResAddPath (tmpPath, TRUE);

	// This SHOULD NOT BE REQUIRED -- IT IS _VERY_ EASY TO BREAK CODE THAT DEPENDS ON THIS
	// I'd like to make it go away soon....
	ResSetDirectory (FalconDataDirectory);

#ifdef __WATCOMC__
	chdir(FalconDataDirectory);
#else
	_chdir(FalconDataDirectory);
#endif
	sprintf(fileName, "%s\\%s.ini", FalconObjectDataDir, "Falcon4");

	gLangIDNum = GetPrivateProfileInt("Lang", "Id", 0, fileName);

	// Init movie player before the windows
// OW now called by Device::Setup
//	movieInit(2, NULL);

	UI_LoadSkyWeatherData();

	FalconDisplay.Setup(gLangIDNum);

	mainAppWnd = FalconDisplay.appWin;

	if (g_writeSndTbl)
	    SaveSFXTable();
	if (g_writeMissionTbl)
	    WriteMissionData();
	if(gSoundFlags & FSND_SOUND) // switch for turning on/off sound stuff
	{
		InitSoundManager(FalconDisplay.appWin, 0, FalconDataDirectory);
	}
	g_voicemap.LoadVoices();

#ifdef VOICE_INPUT
	load_voice_recognition_demo_sound_file ();
#endif

	if (!initApplication(hInstance, hPrevInstance, nCmdShow))
		return FALSE;

	while (GetMessage(&msg, NULL, 0, 0))
	{
		//if (!TranslateAccelerator (msg.hwnd, hAccel, &msg))
		//	TranslateMessage(&msg);
		DispatchMessage (&msg);   // Dispatch message to window.
	}

	SystemLevelExit();
    _Module.Term();


/* OW	
	// Kus Kludge...
	if( mprp2 )
		FreeLibrary( mprp2 );
	if( mprd3d )
		FreeLibrary( mprd3d );
	if( gen )
		FreeLibrary( gen );
	if( gr3dfx )
		FreeLibrary( gr3dfx );
*/

	#ifdef _DEBUG
	//_CrtDumpMemoryLeaks();
	#endif

	if(m_pUplink)
		m_pUplink.Release();

	CoUninitialize();

	ExitProcess (0);

	return (msg.wParam);      // Returns value from PostQuitMessage.
}

// set up structured exception handling here
int PASCAL WinMain ( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR  lpCmdLine,
   int nCmdShow)
{
	int Result = -1;
	__try
	{
#ifdef VOICE_INPUT
		InitVoiceRecognitionEngine();
#endif
		Result = HandleWinMain ( hInstance, hPrevInstance, lpCmdLine, nCmdShow);
	}
	__except(RecordExceptionInfo(GetExceptionInformation(), "WinMain Thread"))
	{
		// Do nothing here - RecordExceptionInfo() has already done
		// everything that is needed. Actually this code won't even
		// get called unless you return EXCEPTION_EXECUTE_HANDLER from
		// the __except clause.
	}
	return Result;
}

void EndUI( void )
{
	// Looking for multiplayer stomp...
	ShiAssert( TeamInfo[1]==NULL || TeamInfo[1]->cteam != 0xFC );
	ShiAssert( TeamInfo[2]==NULL || TeamInfo[2]->cteam != 0xFC );

	doUI=FALSE;
	TheCampaign.Suspend();
	UI_Cleanup();
	TheCampaign.Resume();
	if (auto_start)
	{
		SetFocus(mainMenuWnd);
	}
}

LRESULT CALLBACK SimWndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
LRESULT retval = 0;	

	// Looking for multiplayer stomp...
	ShiAssert( TeamInfo[1]==NULL || TeamInfo[1]->cteam != 0xFC );
	ShiAssert( TeamInfo[2]==NULL || TeamInfo[2]->cteam != 0xFC );


#ifdef _USE_SECRET_CODE_
	int i;

	for(i=0;i<18;i++)
		if(PetersSecretCode[17-i] != SecretCode[i])
			break;

	if(i != 18)
		message=WM_DESTROY;
#endif // _USE_SECRET_CODE_

	switch (message)
	{

	case WM_CREATE :
         SetFocus (hwnd);
         retval = 0;
      break;


#ifdef _DEBUG	// We only care about these in debug mode
      case WM_COMMAND:
         switch (LOWORD(wParam))
         {
            case ID_SHOW_VERSION:
               if (ShowVersion != 2)
                  ShowVersion = 2;
               else
                  ShowVersion = 0;
            break;

            case ID_SHOW_MAJOR_VERSION:
               if (ShowVersion != 1)
                  ShowVersion = 1;
               else
                  ShowVersion = 0;
            break;

            case ID_FILE_EXIT:
				PostQuitMessage (0);
				retval = 0;
            break;

			case ID_CAMPAIGN_NEW:
				TheCampaign.NewCampaign((FalconGameType)lParam, "default");
				EnableCampaignMenus();
				break;

			case ID_CAMPAIGN_LOAD:
				if (!OpenCampFile(hwnd))
					{
					TheCampaign.EndCampaign();
					return 0;
					}
				EnableMenuItem(GetMenu(hwnd), ID_CAMPAIGN_SAVE, MF_ENABLED);
				EnableCampaignMenus();
				break;

			case ID_CAMPAIGN_EXIT:
				DisableCampaignMenus();
				TheCampaign.EndCampaign();
#if CAMPTOOL
				if (hMainWnd)
					PostMessage (hMainWnd, WM_CLOSE, 0, 0);
#endif
				DisableCampaignMenus();
				break;

			case ID_CAMPAIGN_SAVE:
				SaveCampFile(hwnd,LOWORD(wParam));
				break;		

			case ID_CAMPAIGN_SAVEAS:
			case ID_CAMPAIGN_SAVEALLAS:
			case ID_CAMPAIGN_SAVEINSTANTAS:
				SaveAsCampFile(hwnd,LOWORD(wParam));
				break;

			case ID_CAMPAIGN_DISPLAY:
#ifdef CAMPTOOL
				if (!displayCampaign)
					{
					CampMain (hInst, SW_SHOW);
					displayCampaign = TRUE;
					}
				else
					{
					if (hMainWnd)
						PostMessage (hMainWnd, WM_CLOSE, 0, 0);
					displayCampaign = FALSE;
					}
				CheckMenuItem(GetMenu(hwnd), ID_CAMPAIGN_DISPLAY, (displayCampaign ? MF_CHECKED : MF_UNCHECKED));
#endif CAMPTOOL
				break;

            case ID_CAMPAIGN_PAUSED:
				if (gameCompressionRatio)
					SetTimeCompression(0);
				else
					SetTimeCompression(1);
				CheckMenuItem(GetMenu(hwnd), ID_CAMPAIGN_PAUSED, (gameCompressionRatio ? MF_UNCHECKED : MF_CHECKED));
				break;

#ifdef CAMPTOOL
			case ID_CAMPAIGN_SELECTSQUADRON:
				DialogBox(hInst,MAKEINTRESOURCE(IDD_SQUADRONDIALOG),mainMenuWnd,(DLGPROC)SelectSquadron);
				break;

			case ID_CAMPAIGN_FLYMISSION:
				DialogBox(hInst,MAKEINTRESOURCE(IDD_MISSDIALOG),mainMenuWnd,(DLGPROC)SelectMission);
				break;

			case ID_CAMPAIGN_RENAMINGON:
				gRenameIds = 1-gRenameIds;
				CheckMenuItem(GetMenu(hwnd), ID_CAMPAIGN_RENAMINGON, (gRenameIds ? MF_CHECKED : MF_UNCHECKED));
				break;
#endif

            case ID_SOUND_START:
               F4SoundStart();
            break;

            case ID_SOUND_STOP:
               F4SoundStop();
            break;

			case ID_VOICES_TOOL:
				DialogBox(hInst,MAKEINTRESOURCE(IDD_PLAYVOICES),FalconDisplay.appWin,(DLGPROC)PlayVoicesProc);
            	break;

			case ID_UI_AIRBASE:
				PostMessage(FalconDisplay.appWin,FM_START_UI,0,0); // Start UI
#ifdef DEBUG
            ShowCursor(FALSE); // Turn off mouse cursor for until EXIT in UI is selected
#endif
				break;
            default:
               retval = 0;
            break;
         }
      break;
#endif // Debug stuff...


      case WM_DESTROY :                    
         PostMessage (hwnd, WM_COMMAND, ID_FILE_EXIT, 0);
         retval = 0;
      break;

      case WM_ACTIVATE:
         retval = 0;
      break;

      case WM_USER:
         retval = 0;
      break;

		// OW
		case FM_DISP_ENTER_MODE:
		{
			FalconDisplay._EnterMode((FalconDisplayConfiguration::DisplayMode) wParam, LOWORD(lParam), HIWORD(lParam));
			break;
		}

		case FM_DISP_LEAVE_MODE:
		{
			FalconDisplay._LeaveMode();
			break;
		}

		case FM_DISP_TOGGLE_FULLSCREEN:
		{
			FalconDisplay._ToggleFullScreen();
			break;
		}

      default:
         retval = DefWindowProc (hwnd, message, wParam, lParam);
      break;
	}
	return retval;
}

int get_ip (char *str)
{
	char
		*src;

	unsigned int
		addr=0;

	if(!str)
		return 0;

	src = str;

	while (*src)
	{
		if (*src == '.')
		{
			*src = 0;
			addr = addr * 256 + atoi (str);
			*src = '.';
			src ++;
			str = src;
		}
		else
		{
			src ++;
		}
	}

	addr = addr * 256 + atoi (str);

	return addr;
}

void ParseCommandLine (LPSTR cmdLine)
{
char* arg;
LONG retval = ERROR_SUCCESS;
DWORD value;
DWORD type, size;
HKEY theKey;

	if (i_am ("rheydon"))
	{
		InitDebug (DEBUGGER_TEXT_MODE);
//		gSoundFlags = 0;
		FalconDisplay.displayFullScreen = FALSE;
//		wait_for_loaded = FALSE;
		auto_start = TRUE;
		F4SetAsserts(TRUE);
//		F4SetHardCrash(TRUE);
//		ShiSetHardCrash(TRUE);
		ShiSetAsserts(TRUE);
	}
#ifdef DEBUG
	else if (i_am ("mmortime"))
	{
		InitDebug (DEBUGGER_TEXT_MODE);
		FalconDisplay.displayFullScreen = FALSE;
		RepairObjective = 1;
		intro_movie = FALSE;
		eyeFlyEnabled = TRUE;
		ShiSetAsserts(TRUE);
		F4SetAsserts(TRUE);
	}
	else if (i_am ("kklemmic"))
	{
		InitDebug (DEBUGGER_TEXT_MODE);
		auto_start = TRUE;
		wait_for_loaded = FALSE;
		FalconDisplay.displayFullScreen = FALSE;
		F4SetAsserts(TRUE);
		F4SetHardCrash(TRUE);
		ShiSetHardCrash(TRUE);
		ShiSetAsserts(TRUE);
	}
	else if (i_am ("dpower"))
	{
		InitDebug (DEBUGGER_TEXT_MODE);
		//auto_start = TRUE;
		FalconDisplay.displayFullScreen = FALSE;
		RepairObjective = 1;
		intro_movie = FALSE;
		eyeFlyEnabled = TRUE;
		ShiSetAsserts(TRUE);
		F4SetAsserts(TRUE);
		//wait_for_loaded = FALSE;
	}
	else if (i_am ("ericg") || i_am ("chrisw"))
	{
		eyeFlyEnabled = TRUE;
	}
	else if (i_am ("lrosensh"))
	{
		InitDebug (DEBUGGER_TEXT_MODE);
		auto_start = TRUE;
		FalconDisplay.displayFullScreen = FALSE;
      intro_movie = TRUE;
//      gUnlimitedAmmo = 3;
	}
	else if (i_am ("vincentf"))
	{
		InitDebug (DEBUGGER_TEXT_MODE);
		gSoundFlags = 0;
		FalconDisplay.displayFullScreen = FALSE;
		wait_for_loaded = FALSE;
		auto_start = TRUE;
	}

	else
	{
		InitDebug (DEBUGGER_TEXT_MODE);
		auto_start = TRUE;
	}
#endif 

	// HACK HACK HACK - stuff
//	F4SetAsserts(FALSE);
//	ShiSetAsserts(FALSE);

   size = sizeof (FalconDataDirectory);
   retval = RegOpenKeyEx(HKEY_LOCAL_MACHINE, FALCON_REGISTRY_KEY,
      0, KEY_ALL_ACCESS, &theKey);

	size = sizeof (ComIPGetHostIDIndex);
	retval = RegQueryValueEx(theKey, "HostIDX", 0, &type, (LPBYTE)&value, &size);
	if (retval == ERROR_SUCCESS)
	{
		ComIPGetHostIDIndex = value;
	}

   retval = RegCloseKey(theKey);

	// Parse Command Line
	arg = strtok (cmdLine, " ");

   if (arg != NULL)
   {
      do
   	{
         if (!stricmp (arg, "-file"))
            SimDriver.doFile = 1 - SimDriver.doFile;

         if (!stricmp (arg, "-event"))
            SimDriver.doEvent = TRUE;

         if (strnicmp (arg, "-repair", 2) == 0)
            RepairObjective = 1;

		 if (stricmp (arg, "-armageddon") == 0)
            DestroyObjective = 1;

         if (stricmp (arg, "-log") == 0)
            log_frame_rate = TRUE;


         if (strnicmp (arg, "-C", 2) == 0)
            ClearObjManualFlags = 1;


         if (strnicmp (arg, "-UA", 3) == 0)
            gUnlimitedAmmo ++;
		          
         //if (!strncmp (arg, "-g", 2))
         //   FalconDisplay.deviceNumber = 1 - FalconDisplay.deviceNumber;

		 if (!strnicmp (arg, "-g", 2))
		 {
			 ///if(sscanf(arg,"-g%d",GraphicSettingMult) != 1)
			//	GraphicSettingMult = 1;
			 int temp = atoi(&arg[2]);
			 if(temp >= 1)
				GraphicSettingMult = temp;
			 else
				 GraphicSettingMult = 1;
		 }

         if (!stricmp (arg, "-full"))
            FalconDisplay.displayFullScreen = TRUE;
         else if (!stricmp (arg, "-window"))
            FalconDisplay.displayFullScreen = FALSE;

         if (stricmp (arg, "-hires") == 0)
            HighResolutionHackFlag = TRUE;

		 if(!stricmp(arg,"-version"))
			VersionInfo=TRUE;

		 if(!stricmp(arg,"-norudder"))
			NoRudder=TRUE;

		 if(!stricmp(arg,"-nosmoothing"))
			DisableSmoothing=TRUE;		 

		 if (!stricmp (arg, "-numhats"))
        {
            if ((arg = strtok (NULL, " ")) != NULL)
			{
				NumHats = atoi(arg);
			}
        }

		 if(strnicmp(arg,"-nosound", 8) == 0)
			gSoundFlags &= (0xffffffff ^ FSND_SOUND);

		 if(strnicmp(arg,"-nopete", 7) == 0)
			gSoundFlags &=(0xffffffff ^ FSND_REPETE);

#ifdef DEBUG
		 if(strnicmp(arg,"-noassert", 9) == 0)
            {
			F4SetAsserts(FALSE);
			// KCK: If this line is causing your compile to fail, update
			// codelib, don't comment it out.
			ShiSetAsserts(FALSE);
            }

		 // JB 010325
		 if(strnicmp(arg,"-nowarning", 10) == 0)
			ShiSetWarnings(FALSE);

		 if(strnicmp(arg,"-hardcrash", 9) == 0)
		    {
			F4SetAsserts(TRUE);
			F4SetHardCrash(TRUE);
			// KCK: If this line is causing your compile to fail, update
			// codelib, don't comment it out.
			ShiSetHardCrash(TRUE);
			ShiSetAsserts(TRUE);
			}

		 if(stricmp(arg,"-resetpilots") == 0)
			gClearPilotInfo = 1;
#endif

		 if(stricmp(arg,"-tacedit") == 0)
			gTacticalFullEdit = 1;

		 if(stricmp(arg,"-norsc") == 0)
			 _LOAD_ART_RESOURCES_=0;
		 if(stricmp(arg,"-usersc") == 0)
			 _LOAD_ART_RESOURCES_=1;

		if (strnicmp (arg, "-auto", 5) == 0)
		{
			auto_start = TRUE;
		}

		if (strnicmp (arg, "-nomovie", 8) == 0)
		{
			intro_movie = FALSE;
		}

		if (strnicmp (arg, "-noUIcomms", 8) == 0)
		{
			noUIcomms = TRUE;
		}

/* MN 020104 always allow UI screenshots
		if (strnicmp (arg, "-screen", 8) == 0)
		{
			gScreenShotEnabled = 1;
		}*/

		if (strnicmp (arg, "-time", 5) == 0)
		{
			gTimeModeServer = 1;
		}

		if (strnicmp (arg, "-movie", 6) == 0)
		{
			intro_movie = TRUE;
		}

		if (strnicmp (arg, "-noloader", 9) == 0)
		{
			wait_for_loaded = FALSE;
		}

#ifdef DEBUG
		if(strnicmp(arg,"-campinput", 10) == 0)
			gCampPlayerInput = atoi(arg+10);
#endif

        if (!stricmp (arg, "-bandwidth") || !stricmp (arg, "-bandwith") ||!stricmp (arg, "-bw") )
        {
            if ((arg = strtok (NULL, " ")) != NULL)
            {
                F4CommsBandwidth = atoi(arg);
                if (F4CommsBandwidth < -1)
                    F4CommsBandwidth *= -1;
            }
        }
 
        if (!stricmp (arg, "-urview"))
        {
            if ((arg = strtok (NULL, " ")) != NULL)
            {
                UR_HEAD_VIEW = (float)atoi(arg);
				if(UR_HEAD_VIEW < 50) UR_HEAD_VIEW=50;
				if(UR_HEAD_VIEW > 160) UR_HEAD_VIEW=160;
            }
        }
 
        if (!stricmp (arg, "-latency"))
        {
            if ((arg = strtok (NULL, " ")) != NULL)
			{
				F4CommsLatency = atoi(arg);
				if (F4CommsLatency < 0)
					F4CommsLatency *= -1;
			}
        }

        if (!stricmp (arg, "-drop"))
        {
            if ((arg = strtok (NULL, " ")) != NULL)
			{
				F4CommsDropInterval = atoi(arg);
				if (F4CommsDropInterval < 0)
					F4CommsDropInterval*= -1;
			}
        }

		if (!stricmp (arg, "-session"))
		{
            if ((arg = strtok (NULL, " ")) != NULL)
			{
				F4SessionUpdateTime = atoi (arg);
			}
		}

		if ((!stricmp (arg, "-hostidx")) || (!stricmp (arg, "-hostid")))
		{
			if ((arg = strtok (NULL, " ")) != NULL)
			{
				ComIPGetHostIDIndex = atoi (arg);
			}
		}

		if (!stricmp (arg, "-alive"))
		{
            if ((arg = strtok (NULL, " ")) != NULL)
			{
				F4SessionAliveTimeout = atoi (arg);
			}
		}

		if (!stricmp (arg, "-reply"))
		{
            if ((arg = strtok (NULL, " ")) != NULL)
			{
				F4LatencyPercentage = atoi (arg);
			}
		}

		if (!stricmp(arg, "-mono")) // turn on MONOCHROME support
		{
		   InitDebug (DEBUGGER_TEXT_MODE);
		}

		if (!stricmp(arg, "-nomono")) // turn off MONOCHROME support
		{
		   InitDebug (-1);
		}

		if (!stricmp(arg, "-head")) // turn on head tracking support
		{
         OTWDriver.SetHeadTracking(TRUE);
		}

        if (!stricmp (arg, "-swap"))
        {
            if ((arg = strtok (NULL, " ")) != NULL)
			{
				F4CommsLatency = atoi(arg);
				if (F4CommsSwapInterval < 0)
					F4CommsSwapInterval*= -1;
			}
        }

        if (!stricmp (arg, "-pf"))
        {
            if ((arg = strtok (NULL, " ")) != NULL)
			{
				SGpfSwitchVal = atoi(arg);
				if (SGpfSwitchVal < 0)
					SGpfSwitchVal *= -1;
			}
        }

        if (!stricmp (arg, "-ef"))
        {
           eyeFlyEnabled = 1 - eyeFlyEnabled;
        }

		if (!stricmp (arg, "-ip"))
		{
            if ((arg = strtok (NULL, " ")) != NULL)
			{
				force_ip_address = get_ip (arg);
			}

            MonoPrint ("Force IP Address to %08x\n", force_ip_address);
		}

		if (!stricmp (arg, "-smoke"))
		{
			flag_keep_smoke_trails = TRUE;
		}

#ifdef _USE_SECRET_CODE_
		 if(!stricmp(arg,"-versiondata"))
			 VersionData=TRUE;
#endif // _USE_SECRET_CODE_

		// OW
		if (!stricmp (arg, "-enumswdev"))
			g_bEnumSoftwareDevices = true;

		if (!stricmp (arg, "-cockpitverifier"))
			g_bEnableCockpitVerifier = true;
		if (!stricmp (arg, "-writesndtbl"))
		    g_writeSndTbl = true;
		if (!stricmp (arg, "-writemissiontbl"))
		    g_writeMissionTbl = true;

   	}
      while ((arg = strtok (NULL, " ")) != NULL);
   }
   
   size = sizeof (FalconDataDirectory);
   retval = RegOpenKeyEx(HKEY_LOCAL_MACHINE, FALCON_REGISTRY_KEY,
      0, KEY_ALL_ACCESS, &theKey);
   retval = RegQueryValueEx(theKey, "baseDir", 0, &type, (LPBYTE)&FalconDataDirectory, &size);
   if (retval != ERROR_SUCCESS)
   {
		SimLibPrintMessage ("No Registry Variable\n");
		strcpy (FalconDataDirectory, ".\\");
   }
   size = sizeof (FalconTerrainDataDir);
   RegQueryValueEx(theKey, "theaterDir", 0, &type, (LPBYTE)FalconTerrainDataDir, &size);
   size = sizeof (FalconObjectDataDir);
   RegQueryValueEx(theKey, "objectDir", 0, &type, (LPBYTE)FalconObjectDataDir, &size);
   strcpy(Falcon3DDataDir, FalconObjectDataDir);
   size = sizeof (FalconMiscTexDataDir);
   RegQueryValueEx(theKey, "misctexDir", 0, &type, (LPBYTE)FalconMiscTexDataDir, &size);
   size = sizeof (FalconMovieDirectory);
   retval = RegQueryValueEx(theKey, "movieDir", 0, &type, (LPBYTE)FalconMovieDirectory, &size);
   if(retval != ERROR_SUCCESS)
   {
	   strcpy(FalconMovieDirectory,FalconDataDirectory);
   }
   size = sizeof (FalconMovieMode);
   retval = RegQueryValueEx(theKey, "movieMode", 0, &type, (LPBYTE)FalconMovieMode, &size);
   if(retval != ERROR_SUCCESS)
	   strcpy(FalconMovieMode,"Hurry");
   else   if (size <= 1)
	   strcpy(FalconMovieMode,"Hurry");

   size = sizeof (FalconUIArtDirectory);
   retval = RegQueryValueEx(theKey, "uiArtDir", 0, &type, (LPBYTE)FalconUIArtDirectory, &size);
   if(retval != ERROR_SUCCESS)
   {
	   strcpy(FalconUIArtDirectory,FalconDataDirectory);
	   strcpy(FalconUIArtThrDirectory,FalconDataDirectory);
   }
   size = sizeof (FalconUISoundDirectory);
   retval = RegQueryValueEx(theKey, "uiSoundDir", 0, &type, (LPBYTE)FalconUISoundDirectory, &size);
   if(retval != ERROR_SUCCESS)
   {
	   strcpy(FalconUISoundDirectory,FalconDataDirectory);
   }
   strcpy(FalconSoundThrDirectory, FalconDataDirectory);
   strcat(FalconSoundThrDirectory, "\\sounds");
   retval = RegCloseKey(theKey);
}

void SystemLevelInit (void)
{
char tmpPath[_MAX_PATH];
FILE* zipFile;
int i;

#ifndef NDEBUG
   if (FalconDisplay.displayFullScreen)
      CtrlAltDelMask (TRUE);
#endif

	SimDriver.InitializeSimMemoryPools();

   ASD = new AS_DataClass();
   srand ((unsigned int) time (NULL));

   sprintf (tmpPath, "%s\\%s", FalconDataDirectory, ZIPFILE_NAME);
   zipFile = fopen (tmpPath, "r");
   if (!zipFile) {
	   char	string[300];
	   sprintf( string, "Failed to open %s\n", tmpPath );
	   OutputDebugString( string );
	   ShiError(string);
   }
   fscanf (zipFile, "%d", &numZips);
   resourceHandle = new int[numZips];
   for (i=0; i<numZips; i++)
   {
      fscanf (zipFile, "%*c%s", tmpPath);
      resourceHandle[i] = ResAttach (FalconDataDirectory, tmpPath, FALSE);
   }
   //ResAddPath (FalconTerrainDataDir, FALSE);
   fclose (zipFile);


	// This SHOULD NOT BE REQUIRED -- IT IS _VERY_ EASY TO BREAK CODE THAT DEPENDS ON THIS
	// I'd like to make it go away soon....
	ResSetDirectory (FalconDataDirectory);

#ifdef __WATCOMC__
	chdir(FalconDataDirectory);
#else
   _chdir(FalconDataDirectory);
#endif
   LoadTheaterList();
   TheaterDef *td;
   if (td = g_theaters.GetCurrentTheater()) {
       g_theaters.SetNewTheater(td);
	   g_theaters.DoSoundSetup();
	InitVU();
   }
   else {
	ReadCampAIInputs ("Falcon4");
	numProcessors = F4GetNumProcessors();

	if (!LoadClassTable("Falcon4"))
		{
		MessageBox( NULL,"No Entities Loaded.","Error", MB_OK | MB_ICONSTOP | MB_SETFOREGROUND );
		exit(0);
		}
	InitVU();
	if (!LoadTactics("Falcon4"))
		{
		MessageBox( NULL,"No Tactics Loaded.","Error", MB_OK | MB_ICONSTOP | MB_SETFOREGROUND );
		exit(0);
		}
	LoadTrails ();
   }
#ifndef		NO_TIMER_THREAD
   beginTimer();
#endif
   ThreadManager::setup();
	Camp_Init(1);

	BuildAscii();

	gCommsMgr=new UIComms;
	gCommsMgr->Setup(FalconDisplay.appWin);
	
	if(UI_logbk.Load())
		LogBook.LoadData(&UI_logbk.Pilot);

	SetupDIJoystick(hInst, FalconDisplay.appWin);
	
	SimulationLoopControl::StartSim();

	F4SoundStart();

	LoadFunctionTables();

#ifdef NDEBUG
	ShowCursor(FALSE);
#endif
}

void SystemLevelExit (void)
{
int i;

#ifdef NDEBUG
	ShowCursor(TRUE);
#endif
	// OW
	while(ShowCursor(TRUE) < 0);

	ServerBrowserExit();
	 StopVoice ();//me123
	CleanupDIAll();

	gCommsMgr->Cleanup();
	delete gCommsMgr;
	gCommsMgr=NULL;

// OW now called by Device::Cleanup
//   movieUnInit();
   ExitSoundManager();
   FalconDisplay.Cleanup();
   SimulationLoopControl::StopSim();
   Camp_Exit();
   Camp_FreeMemory();
   UserFunctionTable.ClearTable();

   // ExitReal-Time Loop;
   //ThreadManager::cleanup();
#ifndef NO_TIMER_THREAD
   endTimer();
#endif

   ExitVU();
   UnloadClassTable();
   FreeTactics();

   for (i=0; i<numZips; i++)
   {
      ResDetach (resourceHandle[i]);
   }
   delete [] resourceHandle;
   ResExit();

   SimDriver.ReleaseSimMemoryPools();

   // Make sure there is no keytrapping
   CtrlAltDelMask (FALSE);

/*
	// OW
	_Module.RevokeClassObjects();
	_Module.Term();
	CoUninitialize();
*/
}

void CampaignAutoSave(FalconGameType gametype)
{
	if (!tactical_is_training())
	{
		gCommsMgr->SaveStats();
		if(FalconLocalGame->IsLocal())
		{
			TheCampaign.SetCreationIter(TheCampaign.GetCreationIter()+1);
			TheCampaign.SaveCampaign(gametype,gUI_AutoSaveName,0);
			if(gCommsMgr->Online())
			{
				// Send messages to remote players with new Iter Number
				// So they can save their stats & update Iter in their campaign
				gCommsMgr->UpdateGameIter();
			}
		}
	}
}

#ifdef DEBUG
	HANDLE gDispatchThreadID;
#endif

LRESULT CALLBACK FalconMessageHandler (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT retval = 0;
	static InTimer=0;


	// Looking for multiplayer stomp...
	ShiAssert( TeamInfo[1]==NULL || TeamInfo[1]->cteam != 0xFC );
	ShiAssert( TeamInfo[2]==NULL || TeamInfo[2]->cteam != 0xFC );

#ifdef _MSC_VER
	// Set the FPU to Truncate
	_controlfp(_RC_CHOP,MCW_RC);

	// Set the FPU to 24bit precision
	_controlfp(_PC_24,MCW_PC);
#endif

#ifdef DEBUG
	static int in = 0;
	static UINT lastFive[5];
	gDispatchThreadID = (HANDLE)GetCurrentThreadId();
	for (int i=0; i<4; i++)
		lastFive[i] = lastFive[i+1];
	if (campCritical && campCritical->owningThread == (HANDLE)GetCurrentThreadId() && campCritical->count)
		MonoPrint("We got game.\n");
	lastFive[4] = message;
	in ++;
#endif

	switch (message)
	{

#ifdef NDEBUG
     case WM_NCACTIVATE:
         if (FalconDisplay.displayFullScreen)
         {
            if (!wParam)
               return 0L;
         }
        break; 
#endif
		// Scott OR someone competent needs to trap for surface lost
		// until then UI is only thing that can handle surface lost
		case WM_ACTIVATEAPP:
		case WM_ACTIVATE:
			if(doUI && FalconDisplay.displayFullScreen)
			{
				RECT rect;
				// restore surfaces
				if(gMainHandler)
				{
// OW FIXME
// OW Update: Fixed in c_handler. not need to re-enable
#if 0
					gMainHandler->EnterCritical();
					if(FalconDisplay.GetImageBuffer() && FalconDisplay.GetImageBuffer()->frontSurface())
						MPRRestoreSurface(FalconDisplay.GetImageBuffer()->frontSurface());
					gMainHandler->LeaveCritical();
#endif
					GetWindowRect(FalconDisplay.appWin,&rect);
					InvalidateRect(FalconDisplay.appWin,&rect,FALSE);
				}
			}
			break;
		case WM_KILLFOCUS:
			if(KeepFocus && FalconDisplay.displayFullScreen)
				PostMessage(hwnd,FM_GIVE_FOCUS,0,0);
			break;

		case WM_CREATE :
			PostMessage(hwnd,FM_START_GAME,0,0);
			break;

		case FM_STP_START_RENDER:
			//this allows the UI to refresh the controls BEFORE it enters this function
			Sleep(100);
			STPRender((C_Base *)lParam);
			break;

		case FM_UPDATE_RULES:
			//UpdateRules();
			break;

		case FM_START_GAME:
			if (intro_movie)
				SendMessage(hwnd,FM_PLAY_INTRO_MOVIE,0,0); // Play Movie
			SystemLevelInit();

			if (auto_start)
				PostMessage(hwnd,FM_START_UI,0,0); // Start UI
			break;

		case FM_START_UI:
			KeepFocus=0;
			TheCampaign.Suspend();
			if (wParam)
				g_theaters.DoSoundSetup();

			FalconLocalSession->SetFlyState(FLYSTATE_IN_UI);
#ifdef DEBUG
			gPlayerPilotLock = 0;
#endif
			doUI=TRUE;

			UI_Startup();
			TheCampaign.Resume();
			break;

		case FM_END_UI:
			//edg : as far as i can tell this is never called
			EndUI();
			break;

		case FM_UI_UPDATE_GAMELIST: // I Use this to update my trees which display who is playing
			UI_UpdateGameList();
			break;

		case FM_REFRESH_DOGFIGHT:
			CopyDFSettingsToWindow();
			break;

		case FM_REFRESH_TACTICAL:
			if (gMainHandler != NULL)
			{
				UpdateMissionWindow(TAC_AIRCRAFT);
				CheckCampaignFlyButton();
			}
			break;

		case FM_REFRESH_CAMPAIGN:
			if (gMainHandler != NULL)
			{
				UpdateMissionWindow(CB_MISSION_SCREEN);
				CheckCampaignFlyButton();
			}
			break;

		case FM_TIMER_UPDATE:
			if(gMainHandler != NULL)
			{
				if(InTimer)
					break;
				InTimer=1;
				PlayThatFunkyMusicWhiteBoy();
				UI_UpdateVU();
				if(gCommsMgr)
					RebuildGameTree();
				gMainHandler->ProcessUserCallbacks();
				InTimer=0;
			}
			break;

		case FM_BOOT_PLAYER:
			switch(wParam)
			{
				case game_Dogfight:
					LeaveDogfight();
					break;
			}
			break;

		case FM_TACREF_BUTTON_HANDLER:
//			if(gMainHandler != NULL)
//			{
//				TAC_REF_XrefCB2(lParam);
//	  		}
			
			break;
		// =========================================================
		// KCK: These are used for loading/joining/ending a campaign
		// and are called under all four game sections.
		// =========================================================

		case FM_LOAD_CAMPAIGN:
         if (lTestVar)
         {
			   // Load a campaign here (this should allow tactical engagements too, so we
               // So we can eliminate the LOAD_TACTICAL case.
			   if ((FalconGameType)lParam != game_Campaign && (FalconGameType)lParam != game_TacticalEngagement)
				   strcpy(gUI_CampaignFile,"Instant");

			   retval = TheCampaign.LoadCampaign((FalconGameType)lParam, gUI_CampaignFile);

			   // Notify UI of our success
			   if (retval)
				   PostMessage(FalconDisplay.appWin,FM_JOIN_SUCCEEDED,0,0);
			   else
				   PostMessage(FalconDisplay.appWin,FM_JOIN_FAILED,0,0);
         }
			break;

		case FM_REVERT_CAMPAIGN:
		{
			int gametype = FalconLocalGame->GetGameType();

			// Game aborted - reload current campaign
			strcpy(gUI_CampaignFile,TheCampaign.SaveFile);
			SendMessage(hwnd,FM_SHUTDOWN_CAMPAIGN,0,0);

			// KCK: This is well and truely stupid
			if (gametype == game_Campaign)
			{
				StartCampaignGame(1,gametype);
			}
			else if (gametype == game_TacticalEngagement)
			{
				tactical_restart_mission();
			}
			break;
		}

		case FM_AUTOSAVE_CAMPAIGN:
			CampaignAutoSave((FalconGameType)lParam);
			break;

		case FM_JOIN_CAMPAIGN:
         if (lTestVar)
         {
			   // Join a campaign here
			   if (gCommsMgr)
				   {
				   FalconGameEntity	*game = (FalconGameEntity*)gCommsMgr->GetTargetGame();
				   if (!game || (VuGameEntity*)game == vuPlayerPoolGroup)
					   {
					   MonoPrint("Campaign Join Error: Not a valid game.\n");
					   PostMessage(FalconDisplay.appWin,FM_JOIN_FAILED,0,0);
					   return 0;
					   }


				   // wParam determines phase of loading we'd like to perform:
				   switch (wParam)
					   {
					   case JOIN_PRELOAD_ONLY:			// Preload only
						   MonoPrint("Requesting campaign preload.\n");
						   retval = TheCampaign.RequestScenarioStats(game);
						   break;
					   case JOIN_REQUEST_ALL_DATA:		// Request all game data
						   MonoPrint("Requesting all campaign data.\n");
						   retval = TheCampaign.RequestScenarioStats(game);
						   break;
					   case JOIN_CAMP_DATA_ONLY:		// Request only non-preload data (Called by Campaign only)
						   MonoPrint("Requesting campaign data.\n");
						   retval = TheCampaign.JoinCampaign((FalconGameType)lParam, game);
						   break;
					   }
				   }
			   if (!retval)
				   PostMessage(FalconDisplay.appWin,FM_JOIN_FAILED,0,0);
         }
			break;

		case FM_JOIN_SUCCEEDED:
			MonoPrint("Starting %s game.\n",(wParam)? "remote":"local");
			CampaignJoinSuccess();
			if (!gMainHandler)
				SendMessage(hwnd,FM_START_UI,0,0);
			break;

		case FM_JOIN_FAILED:
			// Theoretically, the error code should be in wParam
			// PETER TODO: Pop up a dialog explaining reason for failure
			MonoPrint("Failed to join game.\n");
			CampaignJoinFail();
			break;

		case FM_GAME_FULL:
			MonoPrint ("Game Is Full\n");
			DisplayJoinStatusWindow (CAMP_GAME_FULL);
			CampaignJoinFail ();
			break;

		case FM_MATCH_IN_PROGRESS:
			MonoPrint ("Match Play game in progress\n");
         GameHasStarted();
			CampaignJoinFail ();
			break;

		case FM_ONLINE_STATUS:
			if(!doUI)
				break;
			if(!gMainHandler)
				break;

			UI_CommsErrorMessage(static_cast<WORD>(wParam));
			break;

		case FM_SHUTDOWN_CAMPAIGN:
			// Remove any connection callbacks we might have had running
			ShutdownCampaign();
			break;

		// ==========================================================
		// KCK: These are for sim entry/exit from the varios sections
		// ==========================================================

		case FM_START_INSTANTACTION:
			// Mark us as loading
			FalconLocalSession->SetFlyState(FLYSTATE_LOADING);



			//InstantAction.SetAgBiasFlag(InstantActionSettings.MissionType); // 0=A-A,1=A-G
			//InstantAction.SetSamFlag(InstantActionSettings.SamSites);
			//InstantAction.SetAAAFlag(InstantActionSettings.AAASites);

			instant_action::set_campaign_time ();
			instant_action::move_player_flight ();
			instant_action::create_wave ();

			MonoPrint("Starting.. %d\n",vuxRealTime);

			SimulationLoopControl::StartGraphics();
			EndUI();
			KeepFocus=1;
			break;

		case FM_END_INSTANTACTION:
/*
			SendMessage(hwnd,FM_SHUTDOWN_CAMPAIGN,wParam,lParam);
			PostMessage(hwnd,FM_START_UI,0,0);
*/
			break;

		case FM_START_DOGFIGHT:
         if (lTestVar)
         {
			// Mark us as loading
			FalconLocalSession->SetFlyState(FLYSTATE_LOADING);
			SimulationLoopControl::StartGraphics();
			EndUI();
			KeepFocus=1;
         }
			break;

		case FM_START_CAMPAIGN:
         if (lTestVar)
         {
			// Mark us as loading
			FalconLocalSession->SetFlyState(FLYSTATE_LOADING);
			   SimulationLoopControl::StartGraphics();
			   EndUI();
			KeepFocus=1;
         }
			break;

		case FM_START_TACTICAL:
         if (lTestVar)
         {
			// Mark us as loading
			FalconLocalSession->SetFlyState(FLYSTATE_LOADING);
			   SimulationLoopControl::StartGraphics();
			   EndUI();
			KeepFocus=1;
         }
			break;

		case FM_GOT_CAMPAIGN_DATA:
			switch(wParam)
				{
				// KCK: This is the data we just got - In case Peter wants to check off the
				// data in some sort of "Getting campaign data" dialog or otherwise do something
				// with it.
				case CAMP_NEED_PRELOAD:
					MonoPrint("Got Scenario Stats.\n");
					gCampJoinTries = 0;
					if(FalconLocalGame)
						CampaignPreloadSuccess(!FalconLocalGame->IsLocal());
					if (gMainHandler) // Removed GameType check - RH
						RecieveScenarioInfo();
					SetCursor(gCursors[CRSR_F16]);
					break;
				case CAMP_NEED_ENTITIES:
					if(!FalconLocalGame || VuLocalGame == vuPlayerPoolGroup)
						break;
					gCampJoinLastData=vuxRealTime;
					MonoPrint("Got Entities.\n");
					gCampJoinTries = 0;
					TheCampaign.GotJoinData();
					DisplayJoinStatusWindow (wParam);
					break;
				case CAMP_NEED_WEATHER:
					if( !FalconLocalGame || VuLocalGame == vuPlayerPoolGroup)
						break;
					gCampJoinLastData=vuxRealTime;
					MonoPrint("Got weather.\n");
					gCampJoinTries = 0;
					TheCampaign.GotJoinData();
					DisplayJoinStatusWindow (wParam);
					break;
				case CAMP_NEED_PERSIST:
					if( !FalconLocalGame || VuLocalGame == vuPlayerPoolGroup)
						break;
					gCampJoinLastData=vuxRealTime;
					MonoPrint("Got persistant lists.\n");
					gCampJoinTries = 0;
					TheCampaign.GotJoinData();
					DisplayJoinStatusWindow (wParam);
					break;
				case CAMP_NEED_OBJ_DELTAS:
					if( !FalconLocalGame || VuLocalGame == vuPlayerPoolGroup)
						break;
					gCampJoinLastData=vuxRealTime;
					gCampJoinTries = 0;
					MonoPrint("Got objective data.\n");
					TheCampaign.GotJoinData();
					DisplayJoinStatusWindow (wParam);
					break;
				case CAMP_NEED_TEAM_DATA:
					if( !FalconLocalGame || VuLocalGame == vuPlayerPoolGroup)
						break;
					gCampJoinLastData=vuxRealTime;
					gCampJoinTries = 0;
					MonoPrint("Got team data.\n");
					TheCampaign.GotJoinData();
					DisplayJoinStatusWindow (wParam);
					break;
				case CAMP_NEED_UNIT_DATA:
					if( !FalconLocalGame || VuLocalGame == vuPlayerPoolGroup)
						break;
					gCampJoinLastData=vuxRealTime;
					MonoPrint("Got unit data.\n");
					gCampJoinTries = 0;
					TheCampaign.GotJoinData();
					DisplayJoinStatusWindow (wParam);
					break;
				case CAMP_NEED_VC:
					if( !FalconLocalGame || VuLocalGame == vuPlayerPoolGroup)
						break;
					gCampJoinLastData=vuxRealTime;
					MonoPrint("Got VC data.\n");
					gCampJoinTries = 0;
					TheCampaign.GotJoinData();
					DisplayJoinStatusWindow (wParam);
					break;
				case CAMP_NEED_PRIORITIES:
					if( !FalconLocalGame || VuLocalGame == vuPlayerPoolGroup)
						break;
					gCampJoinLastData=vuxRealTime;
					MonoPrint("Got Priorities data.\n");
					gCampJoinTries = 0;
					TheCampaign.GotJoinData();
					DisplayJoinStatusWindow (wParam);
					break;
				}
			break;

		// =========================================================
		// KCK: Campaign triggered events
		// =========================================================

		case FM_CAMPAIGN_OVER:
			// This is called when the campaign is over (endgame triggered)
			// wParam is the win result (win/lose/draw)
			// lParam is TRUE if this is our first attempt at calling this
			if (lParam)
				LogBook.FinishCampaign(static_cast<short>(wParam));
			// KCK: If UI is running, pause the Campaign
			if (gMainHandler)
				SetTimeCompression(0);
			TheCampaign.EndgameResult = static_cast<uchar>(wParam);
			// KCK: We should make sure the fly button and time compression check 
			// "TheCampaign.EndgameResult". If it's non zero, the player shouldn't be
			// able to perform these actions.
			break;

		case FM_OPEN_GAME_OVER_WIN:
			switch(wParam)
			{
				case game_InstantAction:
					break;
				case game_Dogfight:
					break;
				case game_Campaign:
					break;
				case game_TacticalEngagement:
					OpenTEGameOverWindow();
					break;
			}
			break;

		case FM_CAMPAIGN_EVENT:
			// Currently unused
			break;

		case FM_ATTACK_WARNING:
			// Basically let the UI know to let the player know we're about to be attacked
			// and what interceptors to take (if any)
			UIScramblePlayerFlight();
			break;

		case FM_AIRBASE_ATTACK:
			CampEventSoundID = 500005;
			break;

		case FM_AIRBASE_DISABLED:
			// NOTE: this will be accompanied by a squadron rebase/recall.
			UI_HandleAirbaseDestroyed();
			break;

		case FM_SQUADRON_RECALLED:
			MonoPrint("Player squadron recalled!\n");
			break;

		case FM_SQUADRON_REBASED:
			RelocateSquadron();
			break;

		case FM_REFRESH_CAMPMAP:
			UI_UpdateOccupationMap();
			break;

		case FM_REBUILD_WP_LIST:
			RebuildCurrentWPList();
			break;

		case FM_PLAYER_FLIGHT_CANCELED:
			UI_HandleFlightCancel();
			break;

		case FM_PLAYER_AIRCRAFT_DESTROYED:
			// PETER TODO: Post message saying player aircraft destroyed (while waiting for takeoff) and go back to mission screen
			MonoPrint("Player aircraft destroyed (while waiting for takeoff)\n");
			UI_HandleAircraftDestroyed();
			break;

		case FM_RECEIVE_CHAT:
			ProcessChatStr((CHATSTR*)lParam);
			break;


		case FM_PLAY_UI_MOVIE:
			if(gMainHandler && ReadyToPlayMovie)
				PlayUIMovieQ();
			break;

		case FM_REPLAY_UI_MOVIE:
			if(gMainHandler && ReadyToPlayMovie)
				ReplayUIMovie(lParam);
			break;

		case FM_REMOTE_LOGBOOK:
			if(gMainHandler && gCommsMgr)
				ViewRemoteLogbook(lParam);
			break;

		case FM_PLAY_INTRO_MOVIE:
			FalconDisplay.EnterMode(FalconDisplayConfiguration::Movie);
			SetFocus(hwnd);

			PlayMovie("movies\\intro.avi", -1, -1, 0, 0, FalconDisplay.GetImageBuffer()->frontSurface());

			FalconDisplay.LeaveMode();
			break;

		case FM_EXIT_GAME:
			EndUI();
			if (auto_start)
			{
				PostQuitMessage (0);
				retval = 0;
			}
			break;

		case FM_GIVE_FOCUS:
			SetActiveWindow(FalconDisplay.appWin);
			SetFocus(FalconDisplay.appWin);
			break;

#ifdef VOICE_INPUT
		case FM_CHECK_FOR_VR_INPUT:
			DoVoiceRecognitionInput();
			break;
#endif

#if 0
#ifdef NDEBUG	// This disables Alt-Tab, Windows Key, and other task switching
		case WM_SYSCOMMAND:
			switch (wParam & 0xFFF0) {	// Windows uses low 4 bits, so we have to maskt them off
				case SC_SCREENSAVE:
				case SC_MONITORPOWER:
					// Do not allow screen saver or power down
		            OutputDebugString("Dropped SC_SCREENSAVE or SC_MONITORPOWER\n");
					in --;
					return 0;
				case SC_NEXTWINDOW:
				case SC_PREVWINDOW:
				case SC_TASKLIST:
				case SC_KEYMENU:
					//Prevent ALT-Tab and such
					//someday we'll might shut down the graphics, minimize ourselves, 
					//then restart the graphics when we return.
		            OutputDebugString("Dropped SC_NEXTWINDOW, SC_PREVWINDOW, SC_TASKLIST or SC_KEYMENU\n");
					in --;
					return 0;
				default:
					in --;
					return DefWindowProc (hwnd, message, wParam, lParam);
			}
			break;
#endif
#endif

		case WM_DESTROY :                    
			break;

		case WM_USER:
			break;


		// OW
		case FM_DISP_ENTER_MODE:
		{
			FalconDisplay._EnterMode((FalconDisplayConfiguration::DisplayMode) wParam, LOWORD(lParam), HIWORD(lParam));
			break;
		}

		case FM_DISP_LEAVE_MODE:
		{
			FalconDisplay._LeaveMode();
			break;
		}

		case FM_DISP_TOGGLE_FULLSCREEN:
		{
			FalconDisplay._ToggleFullScreen();
			break;
		}

		default:
			if(gMainHandler != NULL)
			{
				if(gMainHandler->EventHandler(hwnd,message,wParam,lParam))
					retval = DefWindowProc (hwnd, message, wParam, lParam);
				else
					retval=0;
			}
			else
				retval = DefWindowProc (hwnd, message, wParam, lParam);
			break;
	}


	// Looking for multiplayer stomp...
	ShiAssert( TeamInfo[1]==NULL || TeamInfo[1]->cteam != 0xFC );
	ShiAssert( TeamInfo[2]==NULL || TeamInfo[2]->cteam != 0xFC );


#ifndef DAVE_DBG
#ifdef DEBUG
	if (campCritical) 
	{
		if ((campCritical->owningThread == (HANDLE)GetCurrentThreadId()) && (campCritical->count) && (in == 1))
		{
			// OW
			// *((unsigned int *) 0x00) = 0;

			MonoPrint ("FATAL - Campaign Critical Section Still Held - Place a BREAKPOINT here - Msg %08x %d - Robin\n", GetCurrentThreadId (), campCritical->count);
		}
	}
	if (vuCritical) 
	{
		if ((vuCritical->owningThread == (HANDLE)GetCurrentThreadId()) && (vuCritical->count) && (in == 1))
		{
			// OW
			// *((unsigned int *) 0x00) = 0;

			MonoPrint ("FATAL - Vu Critical Section Still Held - Place a BREAKPOINT here - Msg %08x %d - Robin\n", GetCurrentThreadId (), vuCritical->count);
		}
	}

	in --;
#endif
#endif


	// Looking for multiplayer stomp...
	ShiAssert( TeamInfo[1]==NULL || TeamInfo[1]->cteam != 0xFC );
	ShiAssert( TeamInfo[2]==NULL || TeamInfo[2]->cteam != 0xFC );


	return retval;
}

void PlayMovie(char *filename, int left, int top, int w, int h, void *theSurface)
//void PlayMovie(char *filename, short left, short top, short w, short h, UInt theSurface)
{
HWND hwnd;
int hMovie=-1, mode;
char movieFile[_MAX_PATH];
MSG               msg;
int stopMovie = FALSE;
RECT theRect;
POINT pt;

   hwnd = FalconDisplay.appWin;
   sprintf(movieFile, "%s\\%s", FalconMovieDirectory, filename);

	if (left == -1)
	{
		GetClientRect(hwnd, &theRect);
		pt.x = 0;
		pt.y = 0;
		ClientToScreen(hwnd, &pt);
		OffsetRect (&theRect, pt.x, pt.y);
		left = theRect.left;
		top = theRect.top;
		mode = MOVIE_MODE_INTERLACE;
		if (!stricmp(FalconMovieMode,"Hurry"))
		{
			mode |= MOVIE_MODE_HURRY;
		}
	}
	else
	{
		pt.x = left;
		pt.y = top;
		ClientToScreen(hwnd, &pt);
		SetRect(&theRect, left, top, left+w, left+h);
		left = pt.x;
		top = pt.y;
		mode = MOVIE_MODE_NORMAL;
	}

   F4SilenceVoices();

   MonoPrint( "stopMovie = %d.\n",stopMovie);

   ShowCursor(FALSE);
   
   while (TRUE)
   {
      if ( PeekMessage ( &msg, NULL, 0 , 0, PM_NOREMOVE ) )
      {
         if ( !GetMessage ( &msg, NULL, 0, 0 ) )
            break;
		 
         if (msg.message == WM_KEYUP)	// any key press will stop the movie
            stopMovie = TRUE;
         else if (msg.message == WM_LBUTTONUP) // lmouse click stops it,too
            stopMovie = TRUE;
			

         if (msg.message != WM_ACTIVATEAPP)
         {
            TranslateMessage ( &msg );
            DispatchMessage ( &msg );
         }
         else
         {
            Sleep(1);
         }
      }
      Sleep( 1 );

      if ( hMovie == -1 )
      {

		MonoPrint( "theSurface %x\n", theSurface );
		MonoPrint( "movieFile  %x\n", movieFile  );


         hMovie =  movieOpen( movieFile, NULL, ( LPVOID ) theSurface, 0, NULL, 
                     left, top, mode, MOVIE_USE_AUDIO );

		 MonoPrint( "movieOpen() returned %d\n", hMovie );
		 MonoPrint( "stopMovie = %d.\n",stopMovie);

         if ( hMovie < 0 )
			 {
			 MonoPrint ("Move file %s not found.\n", movieFile);
             break;
			 }

         if ( movieStart( hMovie ) != MOVIE_OK )
         {
			MonoPrint( "error with movieStart.\n");
            movieClose( hMovie );
            break;
         }
      }

      if (stopMovie || !movieIsPlaying( hMovie ) )
      {
		MonoPrint( "Premature movie exit.\n");
         movieClose( hMovie );
         InvalidateRect(hwnd, &theRect, FALSE);
         break;
      }
   }		// end while (1)
   ShowCursor(TRUE);  
   F4HearVoices();
}

void ShutdownCampaign (void)
	{
	if (gMainHandler)
		gMainHandler->RemoveUserCallback(CampaignConnectionTimer);

	// Shutdown campaign stuff here
	TheCampaign.EndCampaign();
#if CAMPTOOL
	if (hMainWnd)
		PostMessage (hMainWnd, WM_CLOSE, 0, 0);
#endif
	SetTimeCompression(0);
	DisableCampaignMenus();
	gCampJoinStatus = 0;
	}

void EnableCampaignMenus (void)
	{
	EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_SAVEAS, MF_ENABLED);
	EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_SAVEALLAS, MF_ENABLED);
	EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_SAVEINSTANTAS, MF_ENABLED);
	EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_LOAD, MF_GRAYED);
	EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_NEW, MF_GRAYED);
	EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_JOIN, MF_GRAYED);
#ifdef CAMPTOOL
	EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_DISPLAY, MF_ENABLED);
#endif
	EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_PAUSED, MF_ENABLED);
	EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_SELECTSQUADRON, MF_ENABLED);
	EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_FLYMISSION, MF_ENABLED);
	EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_EXIT, MF_ENABLED);
	}

void DisableCampaignMenus (void)
	{
	EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_SAVE, MF_GRAYED);
	EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_SAVEAS, MF_GRAYED);
	EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_SAVEALLAS, MF_GRAYED);
	EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_SAVEINSTANTAS, MF_GRAYED);
	EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_LOAD, MF_ENABLED);
	EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_NEW, MF_ENABLED);
	EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_PAUSED, MF_GRAYED);
	EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_DISPLAY, MF_GRAYED);
	EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_FLYMISSION, MF_GRAYED);
	EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_SELECTSQUADRON, MF_GRAYED);
	if (doNetwork)
		EnableMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_JOIN, MF_ENABLED);
	CheckMenuItem(GetMenu(mainMenuWnd), ID_CAMPAIGN_PAUSED, MF_CHECKED);
	}

void ConsoleWrite (char* str)
{
HANDLE hStdIn;
DWORD num;
LPVOID lpMsgBuf;

  hStdIn = GetStdHandle(STD_OUTPUT_HANDLE);

  if (hStdIn == INVALID_HANDLE_VALUE)
  {
 
   FormatMessage( 
       FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
       NULL,
       GetLastError(),
       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
       (LPTSTR) &lpMsgBuf,
       0,
       NULL 
   );
  }
  WriteConsole ( hStdIn, str, strlen(str), &num, NULL);
}

void CtrlAltDelMask( int state )
{
	int was;

	if( state )
		SystemParametersInfo( SPI_SCREENSAVERRUNNING, TRUE, &was, 0 );
	else
		SystemParametersInfo( SPI_SCREENSAVERRUNNING, FALSE, &was, 0 );
}

int i_am (char *with)
{
	DWORD type, size;
	char name[64];
	HKEY key;
	long retval;

	size = 63;
	retval = RegOpenKeyEx (HKEY_LOCAL_MACHINE, "Network\\Logon", 0, KEY_ALL_ACCESS, &key);

	if (retval == ERROR_SUCCESS)
	{
		RegQueryValueEx (key, "Username", 0, &type, (uchar*)&name, &size);

		if (stricmp (name, with) == 0)
		{
			return TRUE;
		}

		RegCloseKey (key);
	}

	return FALSE;
}


