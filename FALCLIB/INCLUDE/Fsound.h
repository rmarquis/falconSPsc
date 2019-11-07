#ifndef _FSOUND_H
#define _FSOUND_H

#pragma warning (push)
#pragma warning (disable : 4201)
#include <mmsystem.h>
#pragma warning (pop)

#ifdef __cplusplus
extern "C"
{
#endif

struct Tpoint;
struct Trotation;
struct SfxDef;

// KCK: Used to convert flight and wing callnumbers to the correct CALLNUMBER eval value
#define VF_SHORTCALLSIGN_OFFSET		45
#define	VF_FLIGHTNUMBER_OFFSET		36

int  F4LoadFXSound(char filename[], long Flags, struct SfxDef *sfx);
int  F4LoadSound(char filename[], long Flags);
int  F4LoadRawSound (int flags, char *data, int len);
int  F4IsSoundPlaying (int soundIdx);
int  F4GetVolume (int soundIdx);
void F4FreeSound(int* sound);
void F4PlaySound (int soundIdx);
void F4LoopSound (int soundIdx);
void F4StopSound (int soundIdx);
void ExitSoundManager (void);
void F4SoundStop (void);
void F4SoundStart (void);
void F4PitchBend (int soundIdx, float PitchMultiplier); // new frequency = orignal freq * PitchMult
void F4PanSound (int soundIdx, int Direction); // dBs -10000 to 10000 (0=Center)
long F4SetVolume(int soundIdx,int Volume); // dBs -10000 -> 0
//int F4CreateStream(int Flags); // NOT Necessary (OBSOLETE)
int F4CreateStream(WAVEFORMATEX *fmt,float seconds);
long F4StreamPlayed(int StreamID);
void F4RemoveStream(int StreamID);
int F4StartStream(char *filename,long flags=0);
int F4StartRawStream(int StreamID,char *Data,long size);
BOOL F4StartCallbackStream(int StreamID,void *ptr,DWORD (*cb)(void *,char *,DWORD));
void F4StopStream(int StreamID);
long F4SetStreamVolume(int StreamID,long volume);
void F4HearVoices();
void F4SilenceVoices();
void F4StopAllStreams();
void F4ChatToggleXmitReceive( void );

/*
** Sound Groups
*/
/*
#define FX_SOUND_GROUP					0
#define ENGINE_SOUND_GROUP				1
#define SIDEWINDER_SOUND_GROUP			2
#define RWR_SOUND_GROUP					3
#define NUM_SOUND_GROUPS				4
*/
#include "SoundGroups.h"

void F4SetGroupVolume(int group,int vol); // dBs -10000 -> 0

extern long gSoundFlags; // defined in top of fsound

enum
{
	FSND_SOUND=0x00000001,
	FSND_REPETE=0x00000002, // Pete's BRA bearing voice stuff
};


// for positional sound effects stuff
void F4SoundFXSetPos( int sfxId, int override, float x, float y, float z, float pscale, float volume = 0.0F );
void F4SoundFXSetDist( int sfxId, int override, float dist, float pscale );
//void F4SoundFXSetCamPos( float x, float y, float z ); // obsolete JPO
void F4SoundFXSetCamPosAndOrient(Tpoint *campos, Trotation *camrot);
void F4SoundFXPositionDriver( unsigned int begFrame, unsigned int endFrame );
void F4SoundFXInit( void );
void F4SoundFXEnd( void );
BOOL F4SoundFXPlaying(int sfxId);

#ifdef __cplusplus
}
#endif


#ifdef _INC_WINDOWS
int  InitSoundManager (HWND hWnd, int mode, char *falconDataDir);
#endif

#endif
