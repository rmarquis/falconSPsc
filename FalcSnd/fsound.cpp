#include <windows.h>
#include "fsound.h"
#include <mmreg.h>
#include "sim\include\stdhdr.h"
#include "falclib.h"
#include "dsound.h"
#include "psound.h"
#include "f4thread.h"
#include "soundfx.h"
#include "PlayerOp.h"
#include "sim\include\simdrive.h"
#include "sim\include\simlib.h"
#include "sim\include\otwdrive.h"
#include "voicemapper.h"

//MI for disabling VMS
#include "sim\include\aircrft.h"
extern bool g_bRealisticAvionics;

//#include "codelib\tools\lists\lists.h"
//#include "codelib\sound\sndlib\SndMgr.h"

//#include "comp.h"
#include "conv.h"
#include "VoiceManager.h"
#include "talkio.h"

// ALL RESMGR CODE ADDITIONS START HERE
#define _USE_RES_MGR_ 1

#ifndef _USE_RES_MGR_ // DON'T USE RESMGR

	#define FS_HANDLE FILE *
	#define FS_OPEN   fopen
	#define FS_READ   fread
	#define FS_CLOSE  fclose
	#define FS_SEEK   fseek
	#define FS_TELL   ftell

#else // USE RESMGR

//	#include "cmpclass.h"
	extern "C"
	{
		#include "codelib\resources\reslib\src\resmgr.h"
	}

	#define FS_HANDLE FILE *
	#define FS_OPEN   RES_FOPEN
	#define FS_READ   RES_FREAD
	#define FS_CLOSE  RES_FCLOSE
	#define FS_SEEK   RES_FSEEK
	#define FS_TELL   RES_FTELL

#endif

/*#include "simbase.h"
#include "campbase.h"
#include "awcsmsg.h"*/

// these are only (at least for the moment) for positional sounds
// sound group max volume levels can affect a group of sounds.
// right now the values are always at the max (or hacked in for test)
// presumably, this will hook into the UI's settings screen where there
// are vol sliders for different groups

//	not needed anymore using PlayerOptions class
	/*
float gGroupMaxVols[NUM_SOUND_GROUPS] =
{
	0.0f,
	0.0f,
	0.0f,
	0.0f,
};*/


WAVEFORMATEX	mono_8bit_8k;
WAVEFORMATEX	mono_8bit_22k;
WAVEFORMATEX	mono_16bit_22k;
WAVEFORMATEX	stereo_8bit_22k;
WAVEFORMATEX	stereo_16bit_22k=
{
	WAVE_FORMAT_PCM,
	2,
	22050,
	88200,
	4,
	16,
	0x0000,
};
WAVEFORMATEX	mono_16bit_8k=
{
	WAVE_FORMAT_PCM,
	1,
	8000,
	16000,
	2,
	16,
	0x0000,
};
LIST			*sndHandleList;
VoiceFilter		*voiceFilter = NULL;
//AWACSMessage	*messageCenter;

// stuff for chat buffers
static const WORD	SAMPLE_SIZE	= 1;					// Bytes per sample
static const DWORD	SAMPLE_RATE	= 8000;					// Sample per second
typedef enum State { Receive=0, Transmit, NotReady };
static State	chatMode;
LPDIRECTSOUNDCAPTURE		chatInputDevice;
extern "C" LPDIRECTSOUND		DIRECT_SOUND_OBJECT;
// Chat IO stuff prototypes
BOOL ChatSetup( void );
void ChatCleanup( void );

long gSoundFlags=FSND_SOUND|FSND_REPETE;
static int soundCount = 0;

BOOL gSoundManagerRunning = FALSE;
extern char FalconObjectDataDir[_MAX_PATH];
extern char FalconSoundThrDirectory[_MAX_PATH];

#define MAX_FALCON_SOUNDS  16//8

// positional sound camera location
static Trotation CamRot = IMatrix;
static Tpoint CamPos = {0,0,0};

static void LoadSFX (char *falconDataDir);
static void UnLoadSFX (void);
static BOOL ReadSFXTable(char *sndtable);
static BOOL WriteSFXTable(char *sndtable);
BOOL SaveSFXTable();

WAVEFORMATEX Mono_22K_8Bit=
{
	WAVE_FORMAT_PCM,
	1,
	22050,
	22050,
	1,
	8,
	0,
};

#if 0
WAVE Mono_8K_8Bit=
{
	0,0,0,0,
	0l,
	0,0,0,0,0,0,0,0,
	0l,
	WAVE_FORMAT_PCM,
	1,
	8000,
	8000,
	1,
	8,
	0,0,0,0,
	0,
};

WAVE Mono_22K_16Bit=
{
	0,0,0,0,
	0l,
	0,0,0,0,0,0,0,0,
	0l,
	WAVE_FORMAT_PCM,
	1,
	22050,
	44100,
	2,
	16,
	0,0,0,0,
	0,
};

WAVE Stereo_22K_8Bit=
{
	0,0,0,0,
	0l,
	0,0,0,0,0,0,0,0,
	0l,
	WAVE_FORMAT_PCM,
	2,
	22050,
	44100,
	2,
	8,
	0,0,0,0,
	0,
};

WAVE Stereo_22K_16Bit=
{
	0,0,0,0,
	0l,
	0,0,0,0,0,0,0,0,
	0l,
	WAVE_FORMAT_PCM,
	2,
	22050,
	88200,
	4,
	16,
	0,0,0,0,
	0,
};

char *ReadFile(char Filename[])
{
	char *Data;
	long size;
	DWORD bytesread;
	HANDLE wf;

	wf=CreateFile(Filename,GENERIC_READ,FILE_SHARE_READ,NULL,
				  OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,
				  NULL);

	if(wf == INVALID_HANDLE_VALUE)
		return(NULL);

	size=GetFileSize(wf,NULL);
//	Data=(char *)malloc(size);
	Data= new char[size];
	if(Data == NULL)
		return(NULL);
	ReadFile(wf,Data,size,&bytesread,NULL);
	CloseHandle(wf);
	return(Data);
}

void InitWaveFormatEXData( void )
	{
	mono_8bit_8k.wFormatTag = 0x0001;
	mono_8bit_8k.nChannels = 0x0001;
	mono_8bit_8k.nSamplesPerSec = 8000;
	mono_8bit_8k.nAvgBytesPerSec = 16000;
	mono_8bit_8k.nBlockAlign = 0x0002;
	mono_8bit_8k.wBitsPerSample = 16;
	mono_8bit_8k.cbSize = 0x0000;

   mono_8bit_22k.wFormatTag = 0x0001;
	mono_8bit_22k.nChannels = 0x0001;
	mono_8bit_22k.nSamplesPerSec = 0x00005622;
	mono_8bit_22k.nAvgBytesPerSec = 0x00005622;
	mono_8bit_22k.nBlockAlign = 0x0001;
	mono_8bit_22k.wBitsPerSample = 0x0008;
	mono_8bit_22k.cbSize = 0x0000;
	
	mono_16bit_22k.wFormatTag = 0x0001;
	mono_16bit_22k.nChannels = 0x0001;
	mono_16bit_22k.nSamplesPerSec = 0x00005622;
	mono_16bit_22k.nAvgBytesPerSec = 0x00005622;
	mono_16bit_22k.nBlockAlign = 0x0001;
	mono_16bit_22k.wBitsPerSample = 0x0010;
	mono_16bit_22k.cbSize = 0x0000;
	
	stereo_8bit_22k.wFormatTag = 0x0001;
	stereo_8bit_22k.nChannels = 0x0002;
	stereo_8bit_22k.nSamplesPerSec = 0x00005622;
	stereo_8bit_22k.nAvgBytesPerSec = 0x00005622;
	stereo_8bit_22k.nBlockAlign = 0x0001;
	stereo_8bit_22k.wBitsPerSample = 0x0008;
	stereo_8bit_22k.cbSize = 0x0000;
	
	stereo_16bit_22k.wFormatTag = 0x0001;
	stereo_16bit_22k.nChannels = 1;
	stereo_16bit_22k.nSamplesPerSec = 22050;
	stereo_16bit_22k.nAvgBytesPerSec = 44100;
	stereo_16bit_22k.nBlockAlign = 0x0002;
	stereo_16bit_22k.wBitsPerSample = 16;
	stereo_16bit_22k.cbSize = 0x0000;
	
/*	mono_16bit_8k.wFormatTag = 0x0001;
	mono_16bit_8k.nChannels = 0x0001;
	mono_16bit_8k.nSamplesPerSec = 0x00001f40;
	mono_16bit_8k.nAvgBytesPerSec = 0x00003e80;
	mono_16bit_8k.nBlockAlign = 0x0001;
	mono_16bit_8k.wBitsPerSample = 0x0010;
	mono_16bit_8k.cbSize = 0x0000;*/
	mono_16bit_8k.wFormatTag = 1;
	mono_16bit_8k.nChannels = 1;
	mono_16bit_8k.nSamplesPerSec = 8000;
	mono_16bit_8k.nAvgBytesPerSec = 16000;
	mono_16bit_8k.nBlockAlign = 2;
	mono_16bit_8k.wBitsPerSample = 16;
	mono_16bit_8k.cbSize = 0;
	}

#endif

/* InitSoundManager:
 * Intergation of SndMgr - 
 * Add SoundBegin()
 * Add ConversationBSegin()
 * Also need to create SoundManagerEnd() function
 * Add SoundEnd() and ConversationEnd()
 */
//int InitSoundManager (HWND hWnd, int mode, char *falconDataDir)
int InitSoundManager (HWND hWnd, int, char *falconDataDir)
{
    ShiAssert(FALSE == IsBadStringPtr(falconDataDir, _MAX_PATH));
    ShiAssert(FALSE != IsWindow(hWnd));
    ShiAssert(FALSE == IsBadStringPtr(FalconObjectDataDir, _MAX_PATH));
	ShiAssert(FALSE == IsBadStringPtr(FalconSoundThrDirectory, _MAX_PATH));

	if(gSoundDriver == NULL)
	{
		gSoundDriver=new CSoundMgr;
		if(!gSoundDriver)
			return FALSE;
		else if(!gSoundDriver->InstallDSound(hWnd,DSSCL_NORMAL,&stereo_16bit_22k))
		{
			delete gSoundDriver;
			gSoundDriver=NULL;
			return(FALSE);
		}
	}

	if( gSoundFlags & FSND_REPETE)
		{
		voiceFilter = new VoiceFilter;
		voiceFilter->SetUpVoiceFilter();
		voiceFilter->StartVoiceManager();
//		messageCenter = new AWACSMessage;
		}
	char sfxtable[_MAX_PATH];
	sprintf (sfxtable, "%s\\%s", FalconObjectDataDir, FALCONSNDTABLE);
	ShiAssert(SFX_DEF == NULL);
	if (ReadSFXTable (sfxtable) == FALSE)
	    return FALSE;
	LoadSFX( falconDataDir );

	g_voicemap.SetVoiceCount(voiceFilter->fragfile.MaxVoices());
	gSoundManagerRunning = TRUE;

	return(TRUE);
}

// Hook to save the sfx tbl
BOOL SaveSFXTable ()
{
    char sfxtable[_MAX_PATH];
    if (FalconObjectDataDir == NULL) return FALSE;
    sprintf (sfxtable, "%s\\%s", FalconObjectDataDir, FALCONSNDTABLE);
    ShiAssert(SFX_DEF == NULL);
    if (WriteSFXTable (sfxtable) == FALSE)
	return FALSE;
    return TRUE;
}

/* Load Sound Loads a wave file and returns a handle to the sound
 * Update this with sound manager calls
 * SND_EXPORT int AudioLoad( char * filename )
 * need to research more about the existing variable Flags
 * and determine what functionality will be needed for it
 */
int F4LoadSound(char filename[],long Flags)
{
	int SoundID=SND_NO_HANDLE;

	if (gSoundDriver)
		SoundID=gSoundDriver->LoadWaveFile(filename,Flags, NULL);

	return(SoundID);
}

int F4LoadFXSound(char filename[],long Flags, SFX_DEF_ENTRY *sfx)
{
	int SoundID=SND_NO_HANDLE;

	if (gSoundDriver)
		SoundID=gSoundDriver->LoadWaveFile(filename,Flags, sfx);

	return(SoundID);
}

/*
 * Same as function above, except for raw sounds
 * These two functions, load the file and fill the buffer
 * Does not play sound immediately. The load raw function will
 * have to be updated to the file. Wavs have the load audio which
 * contains the load wav function. The raw will be based off the same
 * with the exception of a generic load file and have the Wav header
 * slapped on. 8bit-22k
 */
//int F4LoadRawSound (int flags, char *data, int len)
int F4LoadRawSound (int, char *data, int len)
{
	int SoundID=SND_NO_HANDLE;

	if(gSoundDriver)
		SoundID=gSoundDriver->AddRawSample(&Mono_22K_8Bit,data,len,0);

	return(SoundID);
}

/* 
 * Funcionality: free a sound loaded
 */
void F4FreeSound(int* sound)
{
	if (gSoundDriver)
	   gSoundDriver->RemoveSample(*sound);
   *sound=SND_NO_HANDLE;
}

/*
 * Like the function says.
 *  Need to know if these functions are what they are looking for
	or if the functionality should be more a stop - the sound if started
	again would start at the beginning. Pause being, resume from point
	of pause.
    SoundPause // This stop is more of a pause
    SoundResume

SND_EXPORT int SoundStopChannel( AUDIO_CHANNEL * channel )
 */
void F4SoundStop (void)
	{

	if (gSoundDriver)
		{
		gSoundDriver->StopAllSamples();
		}
	}

/* Assume that a sound loaded will call this to execute play
 * assume its more of a resume
 */
void F4SoundStart (void)
{
	if(gSoundDriver)
		F4SoundFXInit();
}

/* Must replace the CreateStream function call with
	AUDIO_ITEM		*tlkItem = NULL;
	int				dummyHandle;

	dummyHandle = AudioCreate( &( waveFormat ) );
	audioChannel = SoundRequestChannel( dummyHandle,
							 &( audioHandle ), DSB_SIZE );

	tlkItem = AudioGetItem( audioHandle );

	if ( audioChannel )
		SoundStreamChannel( audioChannel, fillSoundBuffer );
 *
 */
/*int F4CreateStream(int Flags)
{
	int StreamID=SND_NO_HANDLE;

	if(gSoundDriver)
	{
		switch(Flags)
		{
			case 1: // 22k 8bit mono
				StreamID=gSoundDriver->CreateStream(&Mono_22K_8Bit,0.5f);
				break;
			case 2: // 22k 16bit mono
				StreamID=gSoundDriver->CreateStream(&Mono_22K_16Bit,0.5f);
				break;
			case 3: // 22k 8bit stereo
				StreamID=gSoundDriver->CreateStream(&Stereo_22K_8Bit,0.5f);
				break;
			case 4: // 22k 16bit stereo
				StreamID=gSoundDriver->CreateStream(&Stereo_22K_16Bit,0.5f);
				break;
		}
	}
	return(StreamID);
}
*/

int F4CreateStream(WAVEFORMATEX *fmt,float seconds)
{
	if(gSoundDriver)
		return(gSoundDriver->CreateStream(fmt,seconds));
	return(SND_NO_HANDLE);
}

/*
 * Need to know if this is similar to the SoundReleaseChannel Call
 */
void F4RemoveStream(int StreamID)
{
	if(gSoundDriver && StreamID != SND_NO_HANDLE)
		gSoundDriver->RemoveStream(StreamID);
}

/* Is this merely a play channel call for a sound in a stream?
 * handle1 = AudioStream( "c:\\msdev\\compdata\\stream.wav", -1 );
 */
int F4StartStream(char *filename,long flags)
{
	WAVEFORMATEX Header;
	int StreamID=SND_NO_HANDLE;
	long size,NumSamples;

	if(gSoundDriver)
	{
		// Start KLUDGE
		gSoundDriver->LoadRiffFormat(filename,&Header,&size,&NumSamples);
		if(Header.wFormatTag == WAVE_FORMAT_IMA_ADPCM)
		{
			Header.wFormatTag=WAVE_FORMAT_PCM;
			Header.wBitsPerSample *=4;
			Header.nBlockAlign = (unsigned short)(Header.nChannels * Header.wBitsPerSample / 8);
			Header.nAvgBytesPerSec=Header.nSamplesPerSec * Header.nBlockAlign;
		}
		StreamID=gSoundDriver->CreateStream(&Header,0.5);
		// END KLUDGE
		if(StreamID != SND_NO_HANDLE)
		{
			if(gSoundDriver->StartFileStream(StreamID,filename,flags))
				return(StreamID);
			else
				gSoundDriver->RemoveStream(StreamID);
		}
	}
	return(SND_NO_HANDLE);
}

/* Is this merely a repeat channel call for a sound in a stream?
*/
BOOL F4LoopStream(int StreamID,char *filename)
{
	if(gSoundDriver && StreamID != SND_NO_HANDLE)
		return(gSoundDriver->StartFileStream(StreamID,filename,SND_STREAM_LOOP));

	return(FALSE);
}

BOOL F4StartRawStream(int StreamID,char *Data,long size)
{
	if(gSoundDriver && StreamID != SND_NO_HANDLE)
		return(gSoundDriver->StartMemoryStream(StreamID,Data,size));

	return(FALSE);
}

BOOL F4StartCallbackStream(int StreamID,void *ptr,DWORD (*cb)(void *,char *,DWORD))
{
	if(gSoundDriver && StreamID != SND_NO_HANDLE)
		return(gSoundDriver->StartCallbackStream(StreamID,ptr,cb));
	return(FALSE);
}

void F4StopStream(int StreamID)
{
	if(gSoundDriver && StreamID != SND_NO_HANDLE)
	{
		gSoundDriver->StopStream(StreamID);
		gSoundDriver->RemoveStream(StreamID);
	}
}

void F4StopAllStreams()
{
	// add here reset for lists
/* This I must kill the voices here in buffer*/
	if ( voiceFilter )
		{
		voiceFilter->ResetVoiceManager();
		}
/*		Also the following just stops and I need to play them here
	*/
	if(gSoundDriver)
		gSoundDriver->StopAllStreams();
}
/*
void F4PlayVoiceStreams()
{
	if(voiceFilter)
		voiceFilter->ResumeVoiceStreams();
}*/

long F4SetStreamVolume(int ID,long vol)
{
	if(gSoundDriver)
		return(gSoundDriver->SetStreamVolume(ID,vol));
	return(-10000);
}

void F4HearVoices()
{
	if(voiceFilter)
		voiceFilter->HearVoices();
}

void F4SilenceVoices()
{
	if(voiceFilter)
		voiceFilter->SilenceVoices();
}

long F4StreamPlayed(int StreamID)
{
	if(gSoundDriver)
		return(gSoundDriver->GetStreamPlayTime(StreamID));
	return(0);
}
// Direction (dBs) =-10000 to 10000 where -=Left,0=Center,+=Right (This is a Percentage)
/* Update this with the audio function 
	SND_EXPORT int AudioSetPan( int handle, int location );
 * There is also a linear pan function AudioSetPanRate
 */
void F4PanSound (int soundIdx, int PanDir)
{
	if(gSoundDriver && soundIdx != SND_NO_HANDLE)
		gSoundDriver->SetSamplePan(soundIdx,PanDir);
}

// Pitch = 0.1 to 10.0 where 1.0 is the original Pitch (ie Frequency)
/* Use audio function
SND_EXPORT int AudioSetPitch( int handle, int pitch )
 *
 */
void F4PitchBend(int soundIdx, float Pitch)
{
	if(gSoundDriver && soundIdx != SND_NO_HANDLE)
		gSoundDriver->SetSamplePitch(soundIdx,Pitch);
}

/* Use audio function
 * AudioSetVolume
 */
// Volume is in dBs (-10000 -> 0)
long F4SetVolume(int soundIdx,int Volume)
{
	if(gSoundDriver && soundIdx != SND_NO_HANDLE)
		return(gSoundDriver->SetSampleVolume(soundIdx,Volume));
	return(-10000);
}

// Volume is in dBs (-10000 -> 0)
void F4SetGroupVolume(int group,int vol)
{
	if ( group < 0 || group >= NUM_SOUND_GROUPS )
		return;
	//gGroupMaxVols[ group ] = vol;
	PlayerOptions.GroupVol[ group ] = vol;
}

void F4SetSoundFlags(int soundIdx,long flags)
{
	SOUNDLIST *snd;

	if(gSoundDriver && soundIdx != SND_NO_HANDLE)
	{
		snd=gSoundDriver->FindSample(soundIdx);
		if(snd != NULL)
			snd->Flags |= flags;
	}
}

//void F4SetStreamFlags(int soundIdx,long flags)
void F4SetStreamFlags(int,long)
{
/*
	if(gSoundDriver && soundIdx != SND_NO_HANDLE)
	{
	}
*/
}

int F4GetVolume(int soundIdx)
{
	if(gSoundDriver && soundIdx != SND_NO_HANDLE)
		return(gSoundDriver->GetSampleVolume(soundIdx));
	return(0);
}
/* Use audio function
 * SND_EXPORT void AudioPlay( int handle )
 * possible use the SoundPlayChannel: need to know the level of interaction they
 * want.
 */      
void F4PlaySound (int soundIdx)
{
	if(gSoundDriver && soundIdx != SND_NO_HANDLE)
		gSoundDriver->PlaySample(soundIdx,0);
}

void F4PlaySound (int soundIdx,int flags)
{
	if(gSoundDriver && soundIdx != SND_NO_HANDLE)
		gSoundDriver->PlaySample(soundIdx,flags);
}

/* AudioCheck if playing
 * SND_EXPORT int AudioIsPlaying( int handle )
 */
int F4IsSoundPlaying(int theSound)
{
	if(gSoundDriver && theSound != SND_NO_HANDLE)
		return(gSoundDriver->IsSamplePlaying(theSound));

	return(0);
}

int F4SoundFXPlaying(int sfxId)
{
    ShiAssert (sfxId < NumSFX );
    ShiAssert (sfxId > 0 );
    if (sfxId <= 0 || sfxId >= NumSFX) return 0;

    if( gSoundManagerRunning == FALSE ) return 0;
    return F4IsSoundPlaying(SFX_DEF[sfxId].handle);
}

/* 
 * SND_EXPORT int AudioSetLoop( int handle, int loop_count )
 */
void F4LoopSound (int soundIdx)
{
	if(gSoundDriver && soundIdx != SND_NO_HANDLE)
		gSoundDriver->PlaySample(soundIdx,SND_LOOP_SAMPLE | SND_EXCLUSIVE);
}

/* Need to know the differenced between different stops
 * Soundstop, stopstream, stopsound, Q: will the stop channel 
 * be sufficient for all of this? This stop sound is stop, not pause
 */
void F4StopSound (int soundIdx)
{
	if(gSoundDriver && soundIdx != SND_NO_HANDLE)
		gSoundDriver->StopSample(soundIdx);
}

/* Exit Sound Manager: Add the SoundEnd() function and the 
 * the Conversation function here
 */
void ExitSoundManager (void)
	{
	gSoundManagerRunning = FALSE;
	if(gSoundDriver)
		{
//		delete messageCenter;
		delete voiceFilter;
		voiceFilter=NULL;
		gSoundDriver->RemoveAllSamples();
		gSoundDriver->RemoveAllStreams();
		gSoundDriver->RemoveDSound();
		delete gSoundDriver;
		gSoundDriver=NULL;
		}
	UnLoadSFX();
	if (SFX_DEF != BuiltinSFX)  
		{
	    delete [] SFX_DEF;
		}
	SFX_DEF = NULL;
	}
//void PlayRadioMessage( int talker, int msgid, int *data );
/* Need to find out what the purpose of this callback is.
*/
//void CALLBACK SoundHandler (UINT a, UINT b, DWORD c, DWORD d, DWORD e)
void CALLBACK SoundHandler (UINT, UINT, DWORD, DWORD, DWORD)
{
}

// JPO - read in the sound fx table
BOOL ReadSFXTable(char *sndtable) 
{
    ShiAssert(FALSE == IsBadStringPtr(sndtable, 256));
    FILE *fp = fopen(sndtable, "rb");

    if (fp == NULL) {
	SFX_DEF = BuiltinSFX;
	NumSFX = BuiltinNSFX;
	return TRUE;
    }
    UINT nsfx;
    int vrsn;
    if (fread(&vrsn, sizeof(vrsn), 1, fp) != 1 ||
	fread(&nsfx, sizeof(nsfx), 1, fp) != 1) {
	SFX_DEF = BuiltinSFX;
	NumSFX = BuiltinNSFX;
	fclose (fp);
	return TRUE;
    }
    ShiAssert(nsfx >= SFX_LAST && nsfx < 32767); // arbitrary test
    if (nsfx < SFX_LAST) {
	ShiWarning ("Out of date SFX table");
	SFX_DEF = BuiltinSFX;
	NumSFX = BuiltinNSFX;
	fclose (fp);
	return TRUE;
    }

    if (vrsn != SFX_TABLE_VRSN) {
	ShiWarning("Old Version of Sound Table");
	SFX_DEF = BuiltinSFX;
	NumSFX = BuiltinNSFX;
	fclose (fp);
	return TRUE;
    }
    SFX_DEF = new SFX_DEF_ENTRY[nsfx];
    if (fread(SFX_DEF, sizeof(*SFX_DEF), nsfx, fp) != nsfx) {
	ShiAssert(!"Read error on Sound Table");
	fclose (fp);
	return FALSE;
    }
    NumSFX = nsfx;
    fclose (fp);
    return TRUE;
}

void LoadSFX (char *falconDataDir)
{
    int i;
    char fname[MAX_PATH];
    
    for (i=0; i<NumSFX; i++) // first pass - most important buffers
    {
	if ((SFX_DEF[i].flags & SFX_FLAGS_HIGH) == 0)
	    continue;
	sprintf( fname, "%s\\%s", FalconSoundThrDirectory, SFX_DEF[i].fileName );
	SFX_DEF[i].handle = F4LoadFXSound(fname, SND_EXCLUSIVE, &SFX_DEF[i]);
	ShiAssert (SFX_DEF[i].handle != SND_NO_HANDLE);
    }
    for (i=0; i<NumSFX; i++)
    {
	if (SFX_DEF[i].flags & SFX_FLAGS_HIGH)
	    continue;
	sprintf( fname, "%s\\%s", FalconSoundThrDirectory, SFX_DEF[i].fileName );
	SFX_DEF[i].handle = F4LoadFXSound(fname, SND_EXCLUSIVE, &SFX_DEF[i]);
	ShiAssert (SFX_DEF[i].handle != SND_NO_HANDLE);
    }
}

// JPO - write out the sound fx table
BOOL WriteSFXTable(char *sndtable) 
{
    ShiAssert(FALSE == IsBadStringPtr(sndtable, _MAX_PATH));
    if(BuiltinNSFX <= 0 || BuiltinSFX == NULL) return FALSE;
    ShiAssert(FALSE == F4IsBadReadPtr (BuiltinSFX, sizeof(SFX_DEF) * BuiltinNSFX));
    FILE *fp = fopen(sndtable, "wb");

    if (fp == NULL) {
	return FALSE;
    }
    int vrsn = SFX_TABLE_VRSN;
    if (fwrite(&vrsn, sizeof(vrsn), 1, fp) != 1 ||
	fwrite(&BuiltinNSFX, sizeof(BuiltinNSFX), 1, fp) != 1) {
	ShiAssert(!"Write error on Sound Table");
	fclose (fp);
	return FALSE;
    }
    if (fwrite(BuiltinSFX, sizeof(*BuiltinSFX), BuiltinNSFX, fp) != (UINT)BuiltinNSFX) {
	ShiAssert(!"Write error on Sound Table");
	fclose (fp);
	return FALSE;
    }
    fclose (fp);
    return TRUE;
}

void UnLoadSFX (void)
{
int i;

   for (i=0; i<NumSFX; i++)
   {
      F4FreeSound(&(SFX_DEF[i].handle));
      ShiAssert (SFX_DEF[i].handle == SND_NO_HANDLE );
   }
}

/*
** Name: ChatSetup
** Description:
**		Setup the chat buffer and kick off other chat stuff...
*/
BOOL
ChatSetup( void )
{
	if(!gSoundDriver) return(FALSE);

	#ifdef CHAT_USED
	HRESULT	result;
	WAVEFORMATEX audioFormat;

	// Setup the audio format structure we want
	/*
	audioFormat.wFormatTag		= WAVE_FORMAT_PCM; 
	audioFormat.nChannels		= 1; 
	audioFormat.nSamplesPerSec	= SAMPLE_RATE; 
	audioFormat.nAvgBytesPerSec	= SAMPLE_RATE * SAMPLE_SIZE; 
	audioFormat.nBlockAlign		= SAMPLE_SIZE; 
	audioFormat.wBitsPerSample	= 8 * SAMPLE_SIZE; 
	audioFormat.cbSize			= 0; 
	*/

	audioFormat.wFormatTag		= SND_FORMAT;
	audioFormat.nChannels		= SND_CHANNELS;
	audioFormat.nSamplesPerSec	= SND_SAMPLE_RATE;
	audioFormat.nAvgBytesPerSec	= SND_AVG_RATE;
	audioFormat.nBlockAlign		= SND_BLOCK_ALIGN;
	audioFormat.wBitsPerSample	= SND_BIT_RATE;
	audioFormat.cbSize			= 0; 
	
	
	// Setup input stuff
	result = DirectSoundCaptureCreate( NULL, &chatInputDevice, NULL );
	DSErrorCheck( result );


	// Setup our service modules
	SetupTalkIO( chatInputDevice, DIRECT_SOUND_OBJECT, &audioFormat );

	// Fake that we're transmitting
	chatMode = Transmit;

	// Now toggle into receive mode
	F4ChatToggleXmitReceive();

	#endif

	return TRUE;
}
				 

/*
** Name: ChatCleanup
** Description:
**		Cleanup chat stuff...
*/
void
ChatCleanup( void )
{
	#ifdef CHAT_USED
	HRESULT	result;

	if(!gSoundDriver) return;


	// If we were transmitting, stop
	if (chatMode == Transmit) {
		F4ChatToggleXmitReceive( );
	}

	// Cleanup our service modules
	CleanupTalkIO();

	
	// Clean up input stuff
	result = chatInputDevice->Release();
	DSErrorCheck( result );
	#endif

}


void
F4ChatToggleXmitReceive( void )
{
	if(!gSoundDriver) return;

	// Switch modes
	if (chatMode == Transmit) {
		chatMode = Receive;
//		KCK: Commented out - need to use new VoiceDataMessage
//		EndTransmission();
	} else {
		chatMode = Transmit;
//		KCK: Commented out - need to use new VoiceDataMessage
//		BeginTransmission();
	}
}


/*
** Name: F4SoundSetCamPosAndOrient
** Description:
**		Should be called at the start of the otwframe to set the
**		Camera position and orientation.
*/
extern "C" void
F4SoundFXSetCamPosAndOrient( Tpoint *campos, Trotation *camrot)
{
    CamPos = *campos;
    CamRot = *camrot;
}



/*
** Name: F4SoundSetPos
** Description:
**		Sets a sound's position in the world.  We use the camera location
**		to get distance squared to camera and then set the distance in
**		the effects table.
*/
extern "C" void
F4SoundFXSetPos( int sfxId, int override, float x, float y, float z, float pscale, float volume  )
{
	float dx, dy, dz;
	float dsq;
	SFX_DEF_ENTRY *sfxp;

	// consider 0 to be an invalid id
	if( gSoundManagerRunning == FALSE || sfxId<=0) return;

	// sanity check
    ShiAssert (sfxId < NumSFX );
	ShiAssert (sfxId > 0 );

	// get diffs to camera position
	dx = x - CamPos.x;
	dy = y - CamPos.y;
	dz = z - CamPos.z;

	// get dist squared
	dsq = max(0.0F, dx * dx + dy * dy + dz * dz - volume);

	// get pointer to the sfx entry
	sfxp = &SFX_DEF[ sfxId ];

	ShiAssert(sfxp->flags & SFX_FLAGS_3D); // should be on
	ShiAssert(pscale == 1.0f || (sfxp->flags & SFX_FLAGS_FREQ));

	// if this is an external only sound and we're in cockpit adjust
	if ( (sfxp->flags & SFX_POS_EXTERN) && OTWDriver.DisplayInCockpit() )
	{
		dsq *= 20.5f;
	}

	// if dist is beyond maximum, we do nothing
	if ( dsq >= sfxp->maxDistSq )
		return;

	// check to see what frame the sound was updated on
	if ( sfxp->lastFrameUpdated == SimLibFrameCount )
	{
		// we've updated it already this frame, compare distance...
		if ( dsq < sfxp->distSq )
		{
			sfxp->distSq = dsq;
			sfxp->override = override;
			sfxp->pitchScale = pscale;
			if(gSoundDriver && sfxp->handle != SND_NO_HANDLE)
			    gSoundDriver->SetSamplePosition(sfxp->handle, x, y, z);

		}
	}
	else
	{
		// first time this sound was positioned for the frame
		// set the distance squared, frameupdate and override flag
		sfxp->distSq = dsq;
		sfxp->lastFrameUpdated = SimLibFrameCount;
		sfxp->override = override;
		sfxp->pitchScale = pscale;
		if(gSoundDriver && sfxp->handle != SND_NO_HANDLE)
		    gSoundDriver->SetSamplePosition(sfxp->handle, x, y, z);

	}
}

/*
** Name: F4SoundSetDist
** Description:
**		Sets a sound's position in the world.  We use the camera location
**		to get distance squared to camera and then set the distance in
**		the effects table.
**		Like the above function, however caller passes in explicit dist
*/
extern "C" void
F4SoundFXSetDist( int sfxId, int override, float dsq, float pscale  )
{

	SFX_DEF_ENTRY *sfxp;

	// consider 0 to be an invalid id
	if( gSoundManagerRunning == FALSE || sfxId<=0) return;

	if (pscale > 1.0F)
	{
//		MonoPrint ("F4SoundFXSetDist pscale = %f\n", pscale);
	}

	// sanity check
    ShiAssert (sfxId < NumSFX );
	ShiAssert (sfxId > 0 );
	
	// get pointer to the sfx entry
	sfxp = &SFX_DEF[ sfxId ];

	ShiAssert(pscale == 1.0f || (sfxp->flags & SFX_FLAGS_FREQ));
	
	if(g_bRealisticAvionics &&
	    (sfxp->flags & SFX_FLAGS_VMS))
	{
	    //MI no VMS when on ground
	    if(SimDriver.playerEntity)
		{
			if(OTWDriver.DisplayInCockpit() && SimDriver.playerEntity->OnGround() ||
			!SimDriver.playerEntity->playBetty || !SimDriver.playerEntity->IsSetFlag(MOTION_OWNSHIP))
			{ 
				return;
			}
		}
	}
	//MI Missile volume
	if(g_bRealisticAvionics && sfxp->soundGroup & SIDEWINDER_SOUND_GROUP && SimDriver.playerEntity)
	{
		// volume only attenuates down with lowest being -10000
		sfxp->minVol = -10000;
		//make sure we don't hear it
		if(SimDriver.playerEntity->MissileVolume == 8)
			sfxp->maxVol = -10000;
		else
			sfxp->maxVol = -(float)SimDriver.playerEntity->MissileVolume * 250;
	}
	//MI Threat volume
	if(g_bRealisticAvionics && sfxp->soundGroup & RWR_SOUND_GROUP && SimDriver.playerEntity)
	{
		// volume only attenuates down with lowest being -10000
		sfxp->minVol = -10000;
		//make sure we don't hear it
		if(SimDriver.playerEntity->ThreatVolume == 8)
			sfxp->maxVol = -10000;
		else
			sfxp->maxVol = -(float)(SimDriver.playerEntity->ThreatVolume * 250);
	}
	// if this is an external only sound and we're in cockpit do nothing
	if ( (sfxp->flags & SFX_POS_EXTERN) && OTWDriver.DisplayInCockpit() )
	{
		dsq *= 20.5f;
	}

	// if dist is beyond maximum, we do nothing
	if ( dsq >= sfxp->maxDistSq )
		return;


	// check to see what frame the sound was updated on
	if ( sfxp->lastFrameUpdated == SimLibFrameCount )
	{
		// we've updated it already this frame, compare distance...
		if ( dsq < sfxp->distSq )
		{
			sfxp->distSq = dsq;
			sfxp->override = override;
			sfxp->pitchScale = pscale;
			if(gSoundDriver && sfxp->handle != SND_NO_HANDLE)
			    gSoundDriver->Disable3dSample(sfxp->handle);
		}
	}
	else
	{
		// first time this sound was positioned for the frame
		// set the distance squared, frameupdate and override flag
		sfxp->distSq = dsq;
		sfxp->lastFrameUpdated = SimLibFrameCount;
		sfxp->override = override;
		sfxp->pitchScale = pscale;
		if(gSoundDriver && sfxp->handle != SND_NO_HANDLE)
		    gSoundDriver->Disable3dSample(sfxp->handle);

	}
}

// this variable and mask is used to stagger positional volume setting
// for looping sounds.  mask = 7 is every 8th frame
// mask = 3 is every 4th frame
static unsigned int sPosLoopStagger = 0;
#define LOOP_STAGGER_MASK		0x00000003U

/*
** Name: F4SoundPositionDriver
** Description:
**	Goes thru the list of sound effects and determines if they should be
**	started/stopped or volume increased/decreased.
*/
extern "C" void
F4SoundFXPositionDriver( unsigned int begFrame, unsigned int endFrame )
{
	static unsigned long lastPlayTime = 0;
	int i;
	SFX_DEF_ENTRY *sfxp;
	BOOL isPlaying;
	int volLevel;
	float volRange;
	float maxVol;
	int curVolLevel;

	// Maximum of 20 Hz update on the sound
	if (vuxRealTime - lastPlayTime > 50)
	{
		lastPlayTime = vuxRealTime;
		if(gSoundManagerRunning == FALSE || !SimDriver.InSim()) return;

		// set stagger counter
		sPosLoopStagger = (++sPosLoopStagger) & LOOP_STAGGER_MASK;

		// main loop thru sound effects
		for ( i = 0, sfxp = &SFX_DEF[0]; i < NumSFX; i++, sfxp++ )
		{
			// is the sound positional?
			if ( !( sfxp->flags & SFX_POSITIONAL ) )
				continue; // no

			// set maximum volume based on the individual sound or
			// the sound group, whichever is quieter
			maxVol = sfxp->maxVol;
			if ( maxVol > PlayerOptions.GroupVol[ sfxp->soundGroup ] )
				maxVol = (float)PlayerOptions.GroupVol[ sfxp->soundGroup ];

			// get volume range
			volRange = maxVol - sfxp->minVol;

			// is it a looping or 1 shot sound?
			if ( sfxp->flags & SFX_POS_LOOPED )
			{
				isPlaying =	 F4IsSoundPlaying( sfxp->handle );
				// looped sound

				// if no positioning was set this frame and the sound is
				// playing, stop it
				if ( !(sfxp->lastFrameUpdated >= begFrame && sfxp->lastFrameUpdated <= endFrame ) ) 
				{
					sfxp->distSq = sfxp->maxDistSq;
					if ( isPlaying )
						F4StopSound( sfxp->handle );
					continue;
				}

				// too far
				if  (sfxp->distSq >= sfxp->maxDistSq )
				{
					sfxp->distSq = sfxp->maxDistSq;
					if ( isPlaying )
						F4StopSound( sfxp->handle );
					continue;
				}

				// if it's not playing, start it
				if ( !isPlaying )
				{
					F4LoopSound( sfxp->handle );
					// now adjust volume
					// volume only attenuates down with lowest being -10000
					volLevel = FloatToInt32(maxVol - volRange * sfxp->distSq/sfxp->maxDistSq);
					volLevel = max( volLevel, -10000 );
					F4SetVolume(sfxp->handle, volLevel);
				}
				else // sound already playing
				{
					// adjust volume
					volLevel = FloatToInt32(maxVol - volRange * sfxp->distSq/sfxp->maxDistSq);
					volLevel = max( volLevel, -10000 );

					// get current level & diff
					curVolLevel = F4GetVolume( sfxp->handle );
					// curVolLevel = abs( volLevel - curVolLevel );

					// set volume if difference is large enough and either
					// this sound is in the current stagger slot or the
					// override (no stagger) is set
					if ( curVolLevel != volLevel &&
						 ( sPosLoopStagger == ( i & LOOP_STAGGER_MASK ) ||
							sfxp->override ) )
					{
						F4SetVolume(sfxp->handle, volLevel);
					}
				}


				// check pitch freq
				if ( sfxp->curPitchScale != sfxp->pitchScale )
				{
					//float newPitch;
					ShiAssert(sfxp->flags & SFX_FLAGS_FREQ);
   					//newPitch = (int)((float)(AudioGetItem (sfxp->handle))->wave->nSamplesPerSec) * sfxp->pitchScale;
					F4PitchBend(sfxp->handle, sfxp->pitchScale);
					//AudioSetPitch( sfxp->handle, newPitch );
					sfxp->curPitchScale = sfxp->pitchScale;
				}
			}
			else
			{
				// if this sound wasn't positioned this frame continue...
				// one shot...
				if ( !(sfxp->lastFrameUpdated >= begFrame && sfxp->lastFrameUpdated <= endFrame ) ||
					 sfxp->distSq >= sfxp->maxDistSq )
				{
					continue;
				}

				isPlaying =	 F4IsSoundPlaying( sfxp->handle );

				if ( isPlaying )
				{
					// should we overrride when louder sound plays?
					if ( sfxp->override )
					{
						//int curLevel = AudioGetVolume( sfxp->handle );
						int curLevel = F4GetVolume( sfxp->handle );
						// some sounds we always want to hear some of if within
						// hearing dist (ie explosions), don't attenuate these all the
						// way
						volLevel = FloatToInt32(maxVol - volRange * sfxp->distSq/sfxp->maxDistSq);
						volLevel = max( volLevel, -10000 );

						if ( curLevel < volLevel + 200 )
						{
							F4PlaySound( sfxp->handle, SND_OVERRIDE);
							F4SetVolume( sfxp->handle, volLevel );
							//AudioSetVolume( sfxp->handle, volLevel );
						}
					}
					continue;
				}

				// sound isn't being played, start it and set vol
				// F4PlaySound( sfxp->handle );
				volLevel = FloatToInt32(maxVol - volRange * sfxp->distSq/sfxp->maxDistSq);
				volLevel = max( volLevel, -10000 );
				F4SetVolume( sfxp->handle, volLevel );
				F4PlaySound( sfxp->handle );
				//AudioSetVolume( sfxp->handle, volLevel );
				
			}
		}
		if (gSoundDriver)
		    gSoundDriver->SetCameraPostion(&CamPos, &CamRot);
	}
}


/*
** Name: F4SoundFXInit
** Description:
**		Inits soundfx variables
*/
extern "C" void
F4SoundFXInit( void )
{
	int i;
	SFX_DEF_ENTRY *sfxp;

	if(!gSoundDriver) return;

	// main loop thru sound effects
	for ( i = 0, sfxp = &SFX_DEF[0]; i < NumSFX; i++, sfxp++ )
	{
		sfxp->distSq = sfxp->maxDistSq;
	  	sfxp->lastFrameUpdated = 0;
	}
}


/*
** Name: F4SoundFXEnd
** Description:
**		Stops all sounds from playing
*/
extern "C" void
F4SoundFXEnd( void )
{
	int i;
	SFX_DEF_ENTRY *sfxp;

	if(!gSoundDriver) return;

	// main loop thru sound effects
	for ( i = 0, sfxp = &SFX_DEF[0]; i < NumSFX; i++, sfxp++ )
	{
	 	if ( F4IsSoundPlaying( sfxp->handle ) )
			F4StopSound( sfxp->handle );
	}
}
