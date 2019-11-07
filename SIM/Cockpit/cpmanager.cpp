// This is a test of the emergency broadcasting system.


#include "stdafx.h"
#include <tchar.h>
#include <windows.h>
#include <stdio.h>

#include "stdhdr.h"
#include "simdrive.h"

#include "cpres.h"
#include "cpchev.h"
#include "cpmanager.h"
#include "cpobject.h"
#include "cpsurface.h"
#include "cppanel.h"
#include "cplight.h"
#include "button.h"
#include "cpindicator.h"
#include "cpdial.h"
#include "icp.h"
#include "KneeBoard.h"
#include "cpded.h"
#include "cpadi.h"
#include "cpmachasi.h"
#include "dispcfg.h"
#include "cphsi.h"
#include "sinput.h"
#include "cpdigits.h"
#include "inpfunc.h"
#include "Graphics\Include\filemem.h"
#include "Graphics\Include\image.h"
#include "Graphics\Include\TimeMgr.h"
#include "Graphics\Include\TOD.h"
#include "fsound.h"
#include "soundfx.h"
#include "cplift.h"
#include "cpsound.h"
#include "mfd.h"
#include "f4thread.h"
#include "cptext.h"
#include "dispopts.h"
#include "Graphics\Include\drawbsp.h"
#include "Graphics\Include\renderow.h"
#include "sms.h"
#include "simweapn.h"
#include "cpkneeview.h"
#include "flightData.h"
//#include "fault.h"
#include "datadir.h"
#include "hud.h"
#include "commands.h"
#include "ui\include\logbook.h"
#include "falcsnd\voicemanager.h"
#include "aircrft.h"

#include "FalcLib\include\playerop.h"

#include "Dofsnswitches.h"

extern bool g_bEnableCockpitVerifier;
extern bool g_bRealisticAvinoics;

// This controls how often the 2D cockpit lighting is recomputed.
static const float COCKPIT_LIGHT_CHANGE_TOLERANCE = 0.1f;
static int gDebugLineNum;

ImageBuffer		*gpTemplateSurface	= NULL;
GLubyte			*gpTemplateImage	= NULL;
GLulong			*gpTemplatePalette	= NULL;
FlightData cockpitFlightData;

#if DO_HIRESCOCK_HACK
BOOL gDoCockpitHack;
#endif

#ifdef USE_SH_POOLS
MEM_POOL gCockMemPool;
#endif

#ifdef _DEBUG

// Special assert for Snqqpy
#define CockpitMessage(badString, area, lineNum )																	\
{                                                                 \
char	buffer[80];															  		\
int choice;															  		\
                                                                  \
	sprintf( buffer, "Cockpit Error in %s at line %0d - bad string is %s", area, lineNum, badString);\
	choice = MessageBox(NULL, buffer, "Problem:  ",		 				\
						MB_ICONERROR | MB_ABORTRETRYIGNORE | MB_TASKMODAL);		\
	if (choice == IDABORT) {													\
		exit(-1);																\
	}																			\
	if (choice == IDRETRY) {													\
		__asm int 3																\
	}																			\
}
#else
#define CockpitMessage(A,B,C)
#endif

void ReadImage(char* pfilename, GLubyte** image, GLulong** palette) {

	int					result;
	int					totalWidth;
	CImageFileMemory 	texFile;
		
	// Make sure we recognize this file type
	texFile.imageType = CheckImageType(pfilename );
	ShiAssert( texFile.imageType != IMAGE_TYPE_UNKNOWN );

	// Open the input file
	result = texFile.glOpenFileMem( pfilename );
	ShiAssert( result == 1 );

	// Read the image data (note that ReadTextureImage will close texFile for us)
	texFile.glReadFileMem();
	result = ReadTextureImage( &texFile );
	if (result != GOOD_READ) {
		ShiError( "Failed to read bitmap.  CD Error?" );
	}

	// Store the image size (check it in debug mode)
	totalWidth = texFile.image.width;

	*image = texFile.image.image;

	if(palette) {
		*palette = texFile.image.palette;
	}
	else {
		glReleaseMemory( (char*)texFile.image.palette );
	}
}



// This is a helper function for SetTOD which uses the lit 16 bit
// palette to convert from 8 bit data to 16 bit lit data for a
// single image buffer element of the cockpit.
// SCR 3/10/98

void Translate8to16( WORD *pal, BYTE *src, ImageBuffer *image ) {
	int		row;
	WORD	*tgt;
	WORD	*end;
	void	*imgPtr;

	ShiAssert(FALSE == F4IsBadReadPtr(pal, 256 * sizeof(*pal)));
	ShiAssert( src );
	ShiAssert( image );

// OW FIXME
//	ShiAssert( image->PixelSize() == sizeof(*tgt) );

	imgPtr = image->Lock();

	for (row = 0; row < image->targetYres(); row++) {
		tgt = (WORD*)image->Pixel( imgPtr, row, 0 );
		end = tgt + image->targetXres();
		while (tgt < end) {
			*tgt = pal[*src];
			tgt++;
			src++;
		}
	}

	image->Unlock();
}

void Translate8to32( DWORD *pal, BYTE *src, ImageBuffer *image ) {
	int		row;
	DWORD	*tgt;
	DWORD	*end;
	void	*imgPtr;

	ShiAssert(FALSE == F4IsBadReadPtr(pal, 256 * sizeof(*pal)));
	ShiAssert( src );
	ShiAssert( image );

// OW FIXME
//	ShiAssert( image->PixelSize() == sizeof(*tgt) );

	imgPtr = image->Lock();

	for (row = 0; row < image->targetYres(); row++) {
		tgt = (DWORD*)image->Pixel( imgPtr, row, 0 );
		end = tgt + image->targetXres();
		while (tgt < end) {
			*tgt = pal[*src];
			tgt++;
			src++;
		}
	}

	image->Unlock();
}


//====================================================//
// FindToken()
//====================================================//

char* FindToken(char** string, const char* separators) {
	
	char*		result;
	char*		token;

	// find first occurance of something other than separator
	token = _tcsspnp(*string, separators);

	// if there are none, we are probably at the end of string, nothing left to parse
	if(token == NULL) {
		// return the position of the terminator as the string
		*string = strchr(*string, '\0');
	}
	else { // we still have tokens to parse
	
		// starting with the first character other than a separator...
		// find the first separator.
		result = strpbrk(token, separators);

		if(result == NULL) { // found a string no separators
			// return the position of the terminator as the string
			*string = strchr(*string, '\0');
		}
		else { // still have characters to parse

			*result = NULL;
			*string = result + 1;
		}
	}

	return(token);
}

int FindCockpit(const char *pCPFile, Vis_Types eCPVisType, const TCHAR* eCPName, const TCHAR* eCPNameNCTR, TCHAR *strCPFile)
{
	if(eCPVisType == MapVisId(VIS_F16C))
		sprintf(strCPFile, "%s%s", COCKPIT_DIR, pCPFile);
	else
	{
		sprintf(strCPFile, "%s%d\\%s", COCKPIT_DIR, MapVisId(eCPVisType), pCPFile);
		
		if(!ResExistFile(strCPFile))
		{
			sprintf(strCPFile, "%s%s\\%s", COCKPIT_DIR, eCPName, pCPFile);
			
			if(!ResExistFile(strCPFile))
			{
				sprintf(strCPFile, "%s%s\\%s", COCKPIT_DIR, eCPNameNCTR, pCPFile);
				
				if(!ResExistFile(strCPFile))
				{
					// F16C fallback
					sprintf(strCPFile, "%s%s", COCKPIT_DIR, pCPFile);
					return MapVisId(VIS_F16C);
				}
			}
		}
	}

	return eCPVisType;
}


//====================================================//
// CockpitManager::CockpitManager
//====================================================//
#if DO_HIRESCOCK_HACK
CockpitManager::CockpitManager(ImageBuffer *pOTWImage, char *pCPFile, BOOL mainCockpit, int scale, BOOL doHack, Vis_Types eCPVisType, TCHAR* eCPName, TCHAR* eCPNameNCTR) {
#else
CockpitManager::CockpitManager(ImageBuffer *pOTWImage, char *pCPFile, BOOL mainCockpit, int scale) {
#endif

	CP_HANDLE*			pcockpitDataFile;
	const int			lineLen = MAX_LINE_BUFFER - 1;
	char				plineBuffer[MAX_LINE_BUFFER] = "";
	char*				presult;
	BOOL				quitFlag  = FALSE;
	int					idNum;
	char				ptype[16] = "";
	
#if DO_HIRESCOCK_HACK
	gDoCockpitHack = doHack;
	m_eCPVisType = eCPVisType;
	if (eCPName)
	{
		strcpy(m_eCPName, eCPName);
		
		for (int i = 0; i < strlen(m_eCPName); i++)
		{
			if (m_eCPName[i] == '/' || m_eCPName[i] == '\\')
			{
				if (i < strlen(m_eCPName) - 1)
				{
					strcpy(m_eCPName + i, m_eCPName + i + 1);
					i--;
				}
				else
					m_eCPName[i] = 0;
			}
		}
	}

	if (eCPNameNCTR)
	{
		strcpy(m_eCPNameNCTR, eCPNameNCTR);

		for (int i = 0; i < strlen(m_eCPNameNCTR); i++)
		{
			if (m_eCPNameNCTR[i] == '/' || m_eCPNameNCTR[i] == '\\')
			{
				if (i < strlen(m_eCPNameNCTR) - 1)
				{
					strcpy(m_eCPName + i, m_eCPName + i + 1);
					i--;
				}
				else
					m_eCPNameNCTR[i] = 0;
			}
		}
	}
#endif

	mpOTWImage			= pOTWImage;
	mpOwnship			= NULL;

#ifndef _CPMAN_USE_STL_CONTAINERS
 	mpButtonViews		= NULL;
#endif

	mpSoundList			= NULL;

	ADIGpDevReading	= -HORIZONTAL_SCALE;
	ADIGsDevReading	= -VERTICAL_SCALE;
	mHiddenFlag			= FALSE;

	mScale				= scale;

	if(mainCockpit) {
		mpIcp				= new ICPClass;
		mpHsi				= new CPHsi;
		mpKneeBoard		= new KneeBoard;
	}
	else {
		mpIcp				= NULL;
		mpHsi				= NULL;
		mpKneeBoard		= NULL;
	}

	mSurfaceTally		= 0;
	mPanelTally			= 0;
	mObjectTally		= 0;
	mButtonTally		= 0;
	mCursorTally		= 0;
	mButtonViewTally	= 0;
   mNumSurfaces      = 0;
   mNumPanels        = 0;
   mNumObjects       = 0;
   mNumCursors       = 0;
   mNumButtons       = 0;
   mNumButtonViews   = 0;

	lightLevel			= 1.0F;

	mCycleBit			= BEGIN_CYCLE;
	mIsInitialized		= FALSE;
	mIsNextInitialized	= FALSE;
	mpGeometry			= FALSE;
	mHudFont           = 0;
	mMFDFont           = 0;
	mDEDFont				 = 0;
	mGeneralFont		 = 0;
	mPopUpFont			 = 0;
	mKneeFont			 = 0;
	mSABoxFont			 = 0;
	mLabelFont         = 0;

	char strCPFile[MAX_PATH];

	m_eCPVisType = (Vis_Types)FindCockpit(pCPFile, eCPVisType, eCPName, eCPNameNCTR, strCPFile);

	pCPFile = strCPFile;

	pcockpitDataFile = CP_OPEN(pCPFile, "r");

   if (!pcockpitDataFile)
   {
      if (mainCockpit)
      {
         // Try to use the 8x6 and doHack instead
         pcockpitDataFile = CP_OPEN (COCKPIT_FILE_8x6, "r");
      }
      else
      {
         pcockpitDataFile = CP_OPEN (PADLOCK_FILE_8x6, "r");
      }
      gDoCockpitHack = TRUE;
   }
	F4Assert(pcockpitDataFile);			//Error: Couldn't open file
   gDebugLineNum = 0;

	// Load Buffer creation for DEMO release
	mpLoadBuffer = NULL;

	while(!quitFlag) {
		presult	= fgets(plineBuffer, lineLen, pcockpitDataFile);
      gDebugLineNum ++;
		quitFlag	= (presult == NULL);

		if((*plineBuffer == '#') && (!quitFlag)) {
			sscanf((plineBuffer + 1), "%d %s", &idNum, ptype);

			if(!strcmpi(ptype, TYPE_MANAGER_STR)) {
				ParseManagerInfo(pcockpitDataFile);
			}
			else if(!strcmpi(ptype, TYPE_TEXT_STR)) {
				CreateText(idNum, pcockpitDataFile);
			}
			else if(!strcmpi(ptype, TYPE_CHEVRON_STR)) {
				CreateChevron(idNum, pcockpitDataFile);
			}
			else if(!strcmpi(ptype, TYPE_LIFTLINE_STR)) {
				CreateLiftLine(idNum, pcockpitDataFile);
			}
			else if(!strcmpi(ptype, TYPE_SURFACE_STR)) {
				CreateSurface(idNum, pcockpitDataFile);
			}
			else if(!strcmpi(ptype, TYPE_PANEL_STR)) {
				CreatePanel(idNum, pcockpitDataFile);
			}
			else if(!strcmpi(ptype, TYPE_LIGHT_STR)) {
				CreateLight(idNum, pcockpitDataFile);
			}
			else if(!strcmpi(ptype, TYPE_BUTTON_STR)) {
				CreateButton(idNum, pcockpitDataFile);
			}
			else if(!strcmpi(ptype, TYPE_BUTTONVIEW_STR)) {
				CreateButtonView(idNum, pcockpitDataFile);
			}
			else if(!strcmpi(ptype, TYPE_INDICATOR_STR)) {
				CreateIndicator(idNum, pcockpitDataFile);
			}
			else if(!strcmpi(ptype, TYPE_DIGITS_STR)) {
				CreateDigits(idNum, pcockpitDataFile);
			}
			else if(!strcmpi(ptype, TYPE_ADI_STR)) {
				CreateAdi(idNum, pcockpitDataFile);
			}
			else if(!strcmpi(ptype, TYPE_DIAL_STR)) {
				CreateDial(idNum, pcockpitDataFile);
			}
			else if(!strcmpi(ptype, TYPE_CURSOR_STR)) {
				CreateCursor(idNum, pcockpitDataFile);
			}
			else if(!strcmpi(ptype, TYPE_DED_STR)) {
				CreateDed(idNum, pcockpitDataFile);
			}
			else if(!strcmpi(ptype, TYPE_MACHASI_STR)) {
				CreateMachAsi(idNum, pcockpitDataFile);
			}
			else if(!strcmpi(ptype, TYPE_HSI_STR)) {
				CreateHsiView(idNum, pcockpitDataFile);
			}
			else if(!strcmpi(ptype, TYPE_SOUND_STR)) {
				CreateSound(idNum, pcockpitDataFile);
			}
			else if(!strcmpi(ptype, TYPE_KNEEVIEW_STR)) {
				CreateKneeView(idNum, pcockpitDataFile);
			}
			else if(!strcmpi(ptype, TYPE_BUFFER_STR)) {
				LoadBuffer(pcockpitDataFile);
			}
			else {
	            CockpitMessage(ptype, "Top Level", gDebugLineNum);
			}
		}
	}

	CP_CLOSE(pcockpitDataFile);
	
	glReleaseMemory((char*) mpLoadBuffer);
	mpLoadBuffer = NULL;

	ResolveReferences();

	if(mpSoundList) {
		delete mpSoundList;
	}

	mpActivePanel		= NULL;	// first panel in list
	mpNextActivePanel	= NULL;
	mViewChanging		= TRUE;

	// Initialize the lighting conditions and register for future time of day updates
	TimeUpdateCallback( this );
	TheTimeManager.RegisterTimeUpdateCB( TimeUpdateCallback, this );

	mpCockpitCritSec = F4CreateCriticalSection("mpCockpitCritSec");

#ifndef _CPMAN_USE_STL_CONTAINERS
	// Only the old code has a problem with overflow
   F4Assert(mSurfaceTally == mNumSurfaces);
   F4Assert(mObjectTally == mNumObjects);
   F4Assert(mPanelTally == mNumPanels);
   F4Assert(mButtonTally == mNumButtons);
   F4Assert(mButtonViewTally == mNumButtonViews);
#endif

	if(g_bEnableCockpitVerifier)
	{
		if(mSurfaceTally != mNumSurfaces ||
		   mObjectTally != mNumObjects ||
		   mPanelTally != mNumPanels ||
		   mButtonTally != mNumButtons ||
		   mButtonViewTally != mNumButtonViews)
		{
			char buf[0x400];
			sprintf(buf, "Verify error detected!\n\nNumSurfaces:\t%.3d\t\tSurfaceTally:\t%.3d\nNumObjects:\t%.3d\t\tObjectTally:\t%.3d\nNumPanels:\t%.3d\t\tPanelTally:\t%.3d\nNumButtons:\t%.3d\t\tButtonTally:\t%.3d\nNumButtonViews:\t%.3d\t\tButtonViewTally:\t%.3d\t\n",
				mNumSurfaces, mSurfaceTally, mNumObjects, mObjectTally, mNumPanels, mPanelTally, mNumButtons, mButtonTally, mNumButtonViews, mButtonViewTally);
			::MessageBox(NULL, buf, "Falcon 4.0 Cockpit Verifier", MB_OK | MB_SETFOREGROUND);
		}

		else ::MessageBox(NULL, "No errors.", "Falcon 4.0 Cockpit Verifier", MB_OK | MB_SETFOREGROUND);
	}
}


//====================================================//
// CockpitManager::~CockpitManager
//====================================================//

CockpitManager::~CockpitManager(){
	int i;

	// Stop receiving time updates
	TheTimeManager.ReleaseTimeUpdateCB( TimeUpdateCallback, this );

	if(gpTemplateSurface) {
		gpTemplateSurface->Cleanup();
		delete gpTemplateSurface;	
		gpTemplateSurface = NULL;

		glReleaseMemory((char*) gpTemplateImage);
		gpTemplateImage = NULL;
		glReleaseMemory((char*) gpTemplatePalette);
		gpTemplatePalette = NULL;
	}


#ifndef _CPMAN_USE_STL_CONTAINERS
	for(i = 0; i < mNumSurfaces; i++) {
		delete mpSurfaces[i];
	}

	delete [] mpSurfaces;
#else
	for(i = 0; i < mpSurfaces.size(); i++)
		if(mpSurfaces[i]) delete mpSurfaces[i];

	mpSurfaces.clear();
#endif

#ifndef _CPMAN_USE_STL_CONTAINERS
	for(i = 0; i < mNumPanels; i++) { 
		delete mpPanels[i];
	}

	delete [] mpPanels;
#else
	for(i = 0; i < mpPanels.size(); i++)  
		if(mpPanels[i]) delete mpPanels[i];

	mpPanels.clear();
#endif

#ifndef _CPMAN_USE_STL_CONTAINERS
	for(i = 0; i < mNumObjects; i++) {
		delete mpObjects[i];
	}
	delete [] mpObjects;
#else
	for(i = 0; i < mpObjects.size(); i++)
		if(mpObjects[i]) delete mpObjects[i];

	mpObjects.clear();
#endif

#ifndef _CPMAN_USE_STL_CONTAINERS
	if(mpButtonObjects) {
		for(i = 0; i < mNumButtons; i++) {
			delete mpButtonObjects[i];
		}
		delete [] mpButtonObjects;
	}
#else
	if(mpButtonObjects.size()) {
		for(i = 0; i < mpButtonObjects.size(); i++) {
			if(mpButtonObjects[i]) delete mpButtonObjects[i];
		}
		mpButtonObjects.clear();
	}
#endif

#ifndef _CPMAN_USE_STL_CONTAINERS
	for(i = 0; i < mNumButtonViews; i++) {
		delete mpButtonViews[i];
	}
	delete [] mpButtonViews;
#else
	for(i = 0; i < mpButtonViews.size(); i++)
		if(mpButtonViews[i]) delete mpButtonViews[i];
	
	mpButtonViews.clear();
#endif

#ifndef _CPMAN_USE_STL_CONTAINERS
	if(mpCursors) {
		for(i = 0; i < mNumCursors; i++) {
			delete mpCursors[i];
		}
		delete [] mpCursors;
	}
#else
	if(mpCursors.size()) {
		for(i = 0; i < mpCursors.size(); i++) {
			if(mpCursors[i]) delete mpCursors[i];
		}
		mpCursors.clear();
	}
#endif

	if(mpIcp) {
		delete mpIcp;
		mpIcp = NULL;
	}

	if(mpHsi) {
		delete mpHsi;
		mpHsi = NULL;
	}

	if (mpKneeBoard) {
		delete mpKneeBoard;
		mpKneeBoard = NULL;
	}

	for(i = 0; i < BOUNDS_TOTAL; i++) {
		if(mpViewBounds[i]) {
			delete mpViewBounds[i];
		}
	}
	if(mpGeometry) {
		DrawableBSP::Unlock(mpGeometry->GetID());
		delete mpGeometry;
		mpGeometry = NULL;
	}

	F4DestroyCriticalSection(mpCockpitCritSec);
	mpCockpitCritSec = NULL; // JB 010108

}




//====================================================//
// CockpitManager::SetupControlTemplate
//====================================================//

void CockpitManager::SetupControlTemplate(char* pfileName, int width, int height) {

	char ptemplateFile[MAX_LINE_BUFFER];

	if(!gpTemplateSurface) {

		gpTemplateSurface	= new ImageBuffer;

		FindCockpit(pfileName, m_eCPVisType, m_eCPName, m_eCPNameNCTR, ptemplateFile);

		gpTemplateSurface->Setup(&FalconDisplay.theDisplayDevice,
									width,
									height,
									SystemMem,
									None);
		gpTemplateSurface->SetChromaKey(0xFFFF0000);
		ReadImage(ptemplateFile, &gpTemplateImage, &gpTemplatePalette);
	}
}

//====================================================//
// CockpitManager::ParseManagerInfo
//====================================================//

void CockpitManager::ParseManagerInfo(FILE* pcockpitDataFile) {

	const int	lineLen = MAX_LINE_BUFFER - 1;
	char			plineBuffer[MAX_LINE_BUFFER] = "";
	char*			plinePtr;
	char*			ptoken;
	char			pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
	char			ptemplateFileName[32] = "";
	int			i;
	RECT			viewBounds;
	int			numSounds;
	int		    twodpit[2] = { 1196, 1197 };
	bool		    dogeometry = false;

	for(i = 0; i < BOUNDS_TOTAL; i++) {
		mpViewBounds[i] = NULL;
	}

	fgets(plineBuffer, lineLen, pcockpitDataFile);
   gDebugLineNum ++;
	plinePtr = plineBuffer;
	ptoken = FindToken(&plinePtr, pseparators);

	while(strcmpi(ptoken, END_MARKER)){

		if(!strcmpi(ptoken, PROP_NUMSURFACES_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);
			sscanf(ptoken, "%d", &mNumSurfaces);
#ifndef _CPMAN_USE_STL_CONTAINERS
			mpSurfaces = NULL;
			#ifdef USE_SH_POOLS
			if(mNumSurfaces > 0) {
				mpSurfaces = (CPSurface **)MemAllocPtr(gCockMemPool,sizeof(CPSurface *)*mNumSurfaces,FALSE);
			}
			#else
			mpSurfaces	= new CPSurface*[mNumSurfaces];
			#endif
#endif
		}
		else if(!strcmpi(ptoken, PROP_MFDLEFT_STR)) {
			#ifdef USE_SH_POOLS
			mpViewBounds[BOUNDS_MFDLEFT]	= (ViewportBounds *)MemAllocPtr(gCockMemPool,sizeof(ViewportBounds),FALSE);
			#else
			mpViewBounds[BOUNDS_MFDLEFT]	= new ViewportBounds;
			#endif
		
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &viewBounds.top, 
														&viewBounds.left, 
														&viewBounds.bottom, 
														&viewBounds.right);
			ConvertRecttoVBounds(&viewBounds, mpViewBounds[BOUNDS_MFDLEFT], DisplayOptions.DispWidth, DisplayOptions.DispHeight, mScale);
		}
		else if(!strcmpi(ptoken, PROP_MFDRIGHT_STR)) {
			#ifdef USE_SH_POOLS
			mpViewBounds[BOUNDS_MFDRIGHT]	= (ViewportBounds *)MemAllocPtr(gCockMemPool,sizeof(ViewportBounds),FALSE);
			#else
			mpViewBounds[BOUNDS_MFDRIGHT]	= new ViewportBounds;
			#endif
		
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &viewBounds.top, 
														&viewBounds.left, 
														&viewBounds.bottom, 
														&viewBounds.right);
			ConvertRecttoVBounds(&viewBounds, mpViewBounds[BOUNDS_MFDRIGHT], DisplayOptions.DispWidth, DisplayOptions.DispHeight, mScale);
		}
		else if(!strcmpi(ptoken, PROP_HUD_STR)) {

			#ifdef USE_SH_POOLS
			mpViewBounds[BOUNDS_HUD]	= (ViewportBounds *)MemAllocPtr(gCockMemPool,sizeof(ViewportBounds),FALSE);
			#else
			mpViewBounds[BOUNDS_HUD]	= new ViewportBounds;
			#endif
	
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &viewBounds.top, 
														&viewBounds.left, 
														&viewBounds.bottom, 
														&viewBounds.right);
			ConvertRecttoVBounds(&viewBounds, mpViewBounds[BOUNDS_HUD], DisplayOptions.DispWidth, DisplayOptions.DispHeight, mScale);
		}
		else if(!strcmpi(ptoken, PROP_RWR_STR)) {

			#ifdef USE_SH_POOLS
			mpViewBounds[BOUNDS_RWR]	= (ViewportBounds *)MemAllocPtr(gCockMemPool,sizeof(ViewportBounds),FALSE);
			#else
			mpViewBounds[BOUNDS_RWR]	= new ViewportBounds;
			#endif
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &(viewBounds.top), 
														&(viewBounds.left), 
														&(viewBounds.bottom), 
														&(viewBounds.right));
			ConvertRecttoVBounds(&viewBounds, mpViewBounds[BOUNDS_RWR], DisplayOptions.DispWidth, DisplayOptions.DispHeight, mScale);
		}
		else if(!strcmpi(ptoken, PROP_NUMPANELS_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);
			sscanf(ptoken, "%d", &mNumPanels);

#ifndef _CPMAN_USE_STL_CONTAINERS
			#ifdef USE_SH_POOLS
			if(mNumPanels) {
				mpPanels = (CPPanel **)MemAllocPtr(gCockMemPool,sizeof(CPPanel *)*mNumPanels,FALSE);
			}
			#else
			mpPanels	= new CPPanel*[mNumPanels];
			#endif
#endif
		}
		else if(!strcmpi(ptoken, PROP_NUMOBJECTS_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);
			sscanf(ptoken, "%d", &mNumObjects);

#ifndef _CPMAN_USE_STL_CONTAINERS
			#ifdef USE_SH_POOLS
			if(mNumObjects) {
				mpObjects = (CPObject **)MemAllocPtr(gCockMemPool,sizeof(CPObject *)*mNumObjects,FALSE);
			}
			#else
			mpObjects = new CPObject*[mNumObjects];
			#endif
#endif
		}
		else if(!strcmpi(ptoken, PROP_NUMSOUNDS_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);
			sscanf(ptoken, "%d", &numSounds);
			if(numSounds > 0) {
				mpSoundList = new CPSoundList(numSounds);
			}
			else {
				mpSoundList = NULL;
			}
		}
		else if(!strcmpi(ptoken, PROP_NUMBUTTONS_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);
			sscanf(ptoken, "%d", &mNumButtons);
#ifndef _CPMAN_USE_STL_CONTAINERS
			mpButtonObjects = NULL;
			#ifdef USE_SH_POOLS
			if(mNumButtons) {
				mpButtonObjects = (CPButtonObject **)MemAllocPtr(gCockMemPool,sizeof(CPButtonObject *)*mNumButtons,FALSE);
			}
			#else
			mpButtonObjects = new CPButtonObject*[mNumButtons];
			#endif
#endif
		}
		else if(!strcmpi(ptoken, PROP_NUMBUTTONVIEWS_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);
			sscanf(ptoken, "%d", &mNumButtonViews);
#ifndef _CPMAN_USE_STL_CONTAINERS
			#ifdef USE_SH_POOLS
			if(mNumButtonViews) {
				mpButtonViews = (CPButtonView **)MemAllocPtr(gCockMemPool,sizeof(CPButtonView *)*mNumButtonViews,FALSE);
			}
			#else
			mpButtonViews = new CPButtonView*[mNumButtonViews];
			#endif
#endif
		}
		else if(!strcmpi(ptoken, PROP_NUMCURSORS_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);
			sscanf(ptoken, "%d", &mNumCursors);
#ifndef _CPMAN_USE_STL_CONTAINERS
			mpCursors = NULL;
			#ifdef USE_SH_POOLS
			if(mNumCursors > 0) {
				mpCursors = (CPCursor **)MemAllocPtr(gCockMemPool,sizeof(CPCursor *)*mNumCursors,FALSE);
			}
			#else
			mpCursors = new CPCursor*[mNumCursors];
			#endif
         for (int i=0; i<mNumCursors; i++)
            mpCursors[i] = NULL;
#endif
		}
		else if(!strcmpi(ptoken, PROP_MOUSEBORDER_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);
			sscanf(ptoken, "%d", &mMouseBorder);
		}
		else if(!strcmpi(ptoken, PROP_HUDFONT)) {
			ptoken = FindToken(&plinePtr, pseparators);
			sscanf(ptoken, "%d", &mHudFont);
		}
		else if(!strcmpi(ptoken, PROP_MFDFONT)) {
			ptoken = FindToken(&plinePtr, pseparators);
			sscanf(ptoken, "%d", &mMFDFont);
		}
		else if(!strcmpi(ptoken, PROP_DEDFONT)) {
			ptoken = FindToken(&plinePtr, pseparators);
			sscanf(ptoken, "%d", &mDEDFont);
		}
		else if(!strcmpi(ptoken, PROP_POPFONT)) {
			ptoken = FindToken(&plinePtr, pseparators);
			sscanf(ptoken, "%d", &mPopUpFont);
		}
		else if(!strcmpi(ptoken, PROP_KNEEFONT)) {
			ptoken = FindToken(&plinePtr, pseparators);
			sscanf(ptoken, "%d", &mKneeFont);
		}
		else if(!strcmpi(ptoken, PROP_GENFONT)) {
			ptoken = FindToken(&plinePtr, pseparators);
			sscanf(ptoken, "%d", &mGeneralFont);
		}
		else if(!strcmpi(ptoken, PROP_SAFONT)) {
			ptoken = FindToken(&plinePtr, pseparators);
			sscanf(ptoken, "%d", &mSABoxFont);
		}
		else if(!strcmpi(ptoken, PROP_LABELFONT)) {
			ptoken = FindToken(&plinePtr, pseparators);
			sscanf(ptoken, "%d", &mLabelFont);
		}
		else if(!strcmpi(ptoken, PROP_TEMPLATEFILE_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);
			sscanf(ptoken, "%d", &mTemplateWidth);

			ptoken = FindToken(&plinePtr, pseparators);
			sscanf(ptoken, "%d", &mTemplateHeight);

			ptoken = FindToken(&plinePtr, pseparators);
			sscanf(ptoken, "%s", ptemplateFileName);

			SetupControlTemplate(ptemplateFileName, mTemplateWidth, mTemplateHeight);
		}
		else if(!strcmpi(ptoken, PROP_DOGEOMETRY_STR)) {
		    dogeometry = true;
		}
		else if (!strcmpi(ptoken, PROP_DO2DPIT_STR)) {
		    dogeometry = true;
		    ptoken = FindToken(&plinePtr, "=;\n");	
		    sscanf(ptoken, "%d %d", &twodpit[0], &twodpit[1]);
		}
// 2000-11-12 ADDED BY S.G. SO COMMENTED LINE DON'T TRIGGER AN ASSERT
		else if(!strcmpi(ptoken, "//"))
			;
// END OF ADDED SECTION
		else {
// OW <- S.G. NO NEED ANYMORE?
            CockpitMessage(ptoken, "Manager", gDebugLineNum);
		}

		fgets(plineBuffer, lineLen, pcockpitDataFile);
      gDebugLineNum ++;
		plinePtr = plineBuffer;
		ptoken = FindToken(&plinePtr, pseparators);	
	}
	if (dogeometry) {
			CreateCockpitGeometry(&mpGeometry, twodpit[0], twodpit[1]);

	}
}

//====================================================//
// CockpitManager::CreateSound
//====================================================//

void CockpitManager::CreateSound(int idNum, FILE* pcockpitDataFile) {

	int entry=0;
	const int	lineLen = MAX_LINE_BUFFER - 1;
	char*			plinePtr=NULL;
	char*			ptoken=NULL;
	char			plineBuffer[MAX_LINE_BUFFER] = "";
	char			pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};

	fgets(plineBuffer, lineLen, pcockpitDataFile);
   gDebugLineNum ++;
	plinePtr = plineBuffer;
	ptoken = FindToken(&plinePtr, pseparators);	

	while(strcmpi(ptoken, END_MARKER)){

		if(!strcmpi(ptoken, PROP_ENTRY_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d", &entry);
		}
		else {
         CockpitMessage(ptoken, "Sound", gDebugLineNum);
		}

		fgets(plineBuffer, lineLen, pcockpitDataFile);
      gDebugLineNum ++;
		plinePtr = plineBuffer;
		ptoken = FindToken(&plinePtr, pseparators);	
	}

	mpSoundList->AddSound(idNum, entry);
}

//====================================================//
// CockpitManager::LoadBuffer
//====================================================//
void CockpitManager::LoadBuffer(FILE* pcockpitDataFile)
{
 char	psurfaceFile[MAX_LINE_BUFFER];
	char	pfileName[20] = "";
	char*	plinePtr;
	char*	ptoken;
	char	plineBuffer[MAX_LINE_BUFFER] = "";
	const int lineLen = MAX_LINE_BUFFER - 1;
	char	pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};

	fgets(plineBuffer, lineLen, pcockpitDataFile);
   gDebugLineNum ++;
	plinePtr = plineBuffer;
	ptoken = FindToken(&plinePtr, pseparators);	

	while(strcmpi(ptoken, END_MARKER)){

		if(!strcmpi(ptoken, PROP_FILENAME_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%s", pfileName);

			if(m_eCPVisType == MapVisId(VIS_F16C))
				sprintf(psurfaceFile, "%s%s", COCKPIT_DIR, pfileName);
			else
			{
				sprintf(psurfaceFile, "%s%d\\%s", COCKPIT_DIR, MapVisId(m_eCPVisType), pfileName);

				if(!ResExistFile(psurfaceFile))
				{
					sprintf(psurfaceFile, "%s%s\\%s", COCKPIT_DIR, m_eCPName, pfileName);
					
					if(!ResExistFile(psurfaceFile))
					{
						sprintf(psurfaceFile, "%s%s\\%s", COCKPIT_DIR, m_eCPNameNCTR, pfileName);
						
						if(!ResExistFile(psurfaceFile))
						{
							// F16C fallback
							sprintf(psurfaceFile, "%s%s", COCKPIT_DIR, pfileName);
						}
					}
				}
			}
		}
		else if(!strcmpi(ptoken, PROP_BUFFERSIZE_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d", &mLoadBufferWidth, &mLoadBufferHeight);
		}
		else {
         CockpitMessage(ptoken, "Template", gDebugLineNum);
		}

		fgets(plineBuffer, lineLen, pcockpitDataFile);
      gDebugLineNum ++;
		plinePtr = plineBuffer;
		ptoken = FindToken(&plinePtr, pseparators);	
	}

	if(mpLoadBuffer) {
		glReleaseMemory((char*) mpLoadBuffer);
	}

	ReadImage(psurfaceFile, &mpLoadBuffer, NULL);
}


//====================================================//
// CockpitManager::CreateText
//====================================================//

void CockpitManager::CreateText(int idNum, FILE* pcockpitDataFile) {
	char				plineBuffer[MAX_LINE_BUFFER] = "";
	char*				plinePtr;
	char				*ptoken;
	char				pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
	const int		lineLen = MAX_LINE_BUFFER - 1;
	ObjectInitStr	objectInitStr;
	int				numStrings = 0;


	fgets(plineBuffer, lineLen, pcockpitDataFile);
   gDebugLineNum ++;
	plinePtr = plineBuffer;
	ptoken = FindToken(&plinePtr, pseparators);	

	while(strcmpi(ptoken, END_MARKER)){

		if(!strcmpi(ptoken, PROP_CYCLEBITS_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%x", &objectInitStr.cycleBits);
		}
		else if(!strcmpi(ptoken, PROP_DESTLOC_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &(objectInitStr.destRect.top), 
														&(objectInitStr.destRect.left), 
														&(objectInitStr.destRect.bottom), 
														&(objectInitStr.destRect.right));
		}
		else if(!strcmpi(ptoken, PROP_CALLBACKSLOT_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &objectInitStr.callbackSlot);
		}
		else if(!strcmpi(ptoken, PROP_NUMSTRINGS_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &numStrings);
		}
		else {
         CockpitMessage(ptoken, "Text", gDebugLineNum);
		}

		fgets(plineBuffer, lineLen, pcockpitDataFile);
      gDebugLineNum ++;
		plinePtr = plineBuffer;
		ptoken = FindToken(&plinePtr, pseparators);	
	}

	objectInitStr.scale			= mScale;
	objectInitStr.idNum			= idNum;
	objectInitStr.pOTWImage		= mpOTWImage;
	objectInitStr.pTemplate		= gpTemplateSurface;
	objectInitStr.pCPManager	= this;

#ifndef _CPMAN_USE_STL_CONTAINERS
	mpObjects[mObjectTally]		= new CPText(&objectInitStr, numStrings);
	mObjectTally++;
#else
	CPText *p = new CPText(&objectInitStr, numStrings);
	ShiAssert(p);
	if(!p) return;

	mpObjects.push_back(p);
	mObjectTally++;
#endif
}




//====================================================//
// CockpitManager::CreateChevron
//====================================================//

void CockpitManager::CreateChevron(int idNum, FILE* pcockpitDataFile) {

	char				plineBuffer[MAX_LINE_BUFFER] = "";
	char*				plinePtr;
	char				*ptoken;
	char				pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
	const int		lineLen = MAX_LINE_BUFFER - 1;
	ObjectInitStr	objectInitStr;
	ChevronInitStr		chevInitStr;

	fgets(plineBuffer, lineLen, pcockpitDataFile);
   gDebugLineNum ++;
	plinePtr = plineBuffer;
	ptoken = FindToken(&plinePtr, pseparators);	

	while(strcmpi(ptoken, END_MARKER)){

		if(!strcmpi(ptoken, PROP_PANTILT_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%f %f", &(chevInitStr.pan), &(chevInitStr.tilt));
		}
		else if(!strcmpi(ptoken, PROP_PANTILTLABEL_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d", &(chevInitStr.panLabel), &(chevInitStr.tiltLabel));
		}
		else {
         CockpitMessage(ptoken, "Chevron", gDebugLineNum);
		}

		fgets(plineBuffer, lineLen, pcockpitDataFile);
      gDebugLineNum ++;
		plinePtr = plineBuffer;
		ptoken = FindToken(&plinePtr, pseparators);	
	}

	objectInitStr.destRect.top = 0;
	objectInitStr.destRect.left = 0;
	objectInitStr.destRect.bottom = 0;
	objectInitStr.destRect.right = 0;

	objectInitStr.callbackSlot	= 8;
	objectInitStr.cycleBits		= 0xFFFF;

	objectInitStr.idNum			= idNum;
	objectInitStr.pOTWImage		= mpOTWImage;
	objectInitStr.pTemplate		= gpTemplateSurface;
	objectInitStr.pCPManager	= this;

#ifndef _CPMAN_USE_STL_CONTAINERS
	mpObjects[mObjectTally]		= new CPChevron(&objectInitStr, &chevInitStr);
	mObjectTally++;
#else
	CPChevron *p = new CPChevron(&objectInitStr, &chevInitStr);
	ShiAssert(p);
	if(!p) return;

	mpObjects.push_back(p);
	mObjectTally++;
#endif
}


//====================================================//
// CockpitManager::CreateLiftLine
//====================================================//

void CockpitManager::CreateLiftLine(int idNum, FILE* pcockpitDataFile) {

	char				plineBuffer[MAX_LINE_BUFFER] = "";
	char*				plinePtr;
	char				*ptoken;
	char				pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
	const int		lineLen = MAX_LINE_BUFFER - 1;
	ObjectInitStr	objectInitStr;
	LiftInitStr		liftInitStr;

	liftInitStr.doLabel = FALSE;

	fgets(plineBuffer, lineLen, pcockpitDataFile);
   gDebugLineNum ++;
	plinePtr = plineBuffer;
	ptoken = FindToken(&plinePtr, pseparators);	

	while(strcmpi(ptoken, END_MARKER)){

		if(!strcmpi(ptoken, PROP_PANTILT_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%f %f", &(liftInitStr.pan), &(liftInitStr.tilt));
		}
		else if(!strcmpi(ptoken, PROP_PANTILTLABEL_STR)) {
			liftInitStr.doLabel = TRUE;

			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d", &(liftInitStr.panLabel), &(liftInitStr.tiltLabel));
		}
		else {
         CockpitMessage(ptoken, "Lift Line", gDebugLineNum);
		}

		fgets(plineBuffer, lineLen, pcockpitDataFile);
      gDebugLineNum ++;
		plinePtr = plineBuffer;
		ptoken = FindToken(&plinePtr, pseparators);	
	}

	objectInitStr.destRect.top = 0;
	objectInitStr.destRect.left = 0;
	objectInitStr.destRect.bottom = 0;
	objectInitStr.destRect.right = 0;

	objectInitStr.callbackSlot	= 8;
	objectInitStr.cycleBits		= 0xFFFF;

	objectInitStr.idNum			= idNum;
	objectInitStr.pOTWImage		= mpOTWImage;
	objectInitStr.pTemplate		= gpTemplateSurface;
	objectInitStr.pCPManager	= this;

#ifndef _CPMAN_USE_STL_CONTAINERS
	mpObjects[mObjectTally]		= new CPLiftLine(&objectInitStr, &liftInitStr);
	mObjectTally++;
#else
	CPLiftLine *p = new CPLiftLine(&objectInitStr, &liftInitStr);
	ShiAssert(p);
	if(!p) return;

	mpObjects.push_back(p);
	mObjectTally++;
#endif
}


//====================================================//
// CockpitManager::CreateDed
//====================================================//

void CockpitManager::CreateDed(int idNum, FILE* pcockpitDataFile) {

	char				plineBuffer[MAX_LINE_BUFFER] = "";
	char*				plinePtr;
	char				*ptoken;
	char				pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
	const int		lineLen = MAX_LINE_BUFFER - 1;
	ObjectInitStr	objectInitStr;
	DedInitStr  dedInitStr;

	//MI fixup
	if(!g_bRealisticAvionics)
		dedInitStr.color0 = 0xff38e0f8; // default JPO
	else
		dedInitStr.color0 = 0xFF00FF9C;
	dedInitStr.dedtype = DEDT_DED;

	fgets(plineBuffer, lineLen, pcockpitDataFile);
   gDebugLineNum ++;
	plinePtr = plineBuffer;
	ptoken = FindToken(&plinePtr, pseparators);	

	while(strcmpi(ptoken, END_MARKER)){

		if(!strcmpi(ptoken, PROP_CYCLEBITS_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%x", &objectInitStr.cycleBits);
		}
		else if(!strcmpi(ptoken, PROP_DESTLOC_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &(objectInitStr.destRect.top), 
														&(objectInitStr.destRect.left), 
														&(objectInitStr.destRect.bottom), 
														&(objectInitStr.destRect.right));
		}
		else if(!strcmpi(ptoken, PROP_CALLBACKSLOT_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &objectInitStr.callbackSlot);
		}
		else if(!strcmpi(ptoken, PROP_COLOR0_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%lx", &dedInitStr.color0);
		}
		else if (!strcmpi(ptoken, PROP_DED_TYPE)) {
			ptoken = FindToken(&plinePtr, pseparators);
			if (!strcmpi(ptoken, PROP_DED_DED)) {
			    dedInitStr.dedtype = DEDT_DED;
			}
			else if (!strcmpi(ptoken, PROP_DED_PFL)) {
			    dedInitStr.dedtype = DEDT_PFL;
			}
			else
			    CockpitMessage(ptoken, "DED", gDebugLineNum);
		}
		else {
         CockpitMessage(ptoken, "DED", gDebugLineNum);
		}

		fgets(plineBuffer, lineLen, pcockpitDataFile);
      gDebugLineNum ++;
		plinePtr = plineBuffer;
		ptoken = FindToken(&plinePtr, pseparators);	
	}

	objectInitStr.scale			= mScale;
	objectInitStr.idNum			= idNum;
	objectInitStr.pOTWImage		= mpOTWImage;
	objectInitStr.pTemplate		= gpTemplateSurface;
	objectInitStr.pCPManager	= this;

#ifndef _CPMAN_USE_STL_CONTAINERS
	mpObjects[mObjectTally]		= new CPDed(&objectInitStr, &dedInitStr);
	mObjectTally++;
#else
	CPDed *p = new CPDed(&objectInitStr, &dedInitStr);
	ShiAssert(p);
	if(!p) return;

	mpObjects.push_back(p);
	mObjectTally++;
#endif
}

//====================================================//
// CockpitManager::CreateCursor
//====================================================//

void CockpitManager::CreateCursor(int idNum, FILE* pcockpitDataFile) {
	char				plineBuffer[MAX_LINE_BUFFER] = "";
	char*				plinePtr;
	char				*ptoken;
	char				pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
	const int		lineLen = MAX_LINE_BUFFER - 1;
	CursorInitStr	cursorInitStruct;

	fgets(plineBuffer, lineLen, pcockpitDataFile);
   gDebugLineNum ++;
	plinePtr = plineBuffer;
	ptoken = FindToken(&plinePtr, pseparators);	

	while(strcmpi(ptoken, END_MARKER)){

		if(!strcmpi(ptoken, PROP_SRCLOC_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &(cursorInitStruct.srcRect.top), 
														&(cursorInitStruct.srcRect.left), 
														&(cursorInitStruct.srcRect.bottom), 
														&(cursorInitStruct.srcRect.right));
		}
		else if(!strcmpi(ptoken, PROP_HOTSPOT_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d", &cursorInitStruct.xhotSpot, 
											&cursorInitStruct.yhotSpot);
		}
		else {
         CockpitMessage(ptoken, "Cursor", gDebugLineNum);
		}

		fgets(plineBuffer, lineLen, pcockpitDataFile);
      gDebugLineNum ++;
		plinePtr = plineBuffer;
		ptoken = FindToken(&plinePtr, pseparators);	
	}

	cursorInitStruct.idNum			= idNum;
	cursorInitStruct.pOtwImage		= mpOTWImage;
	cursorInitStruct.pTemplate		= gpTemplateSurface;

#ifndef _CPMAN_USE_STL_CONTAINERS
	mpCursors[mCursorTally]			= new CPCursor(&cursorInitStruct);
	mCursorTally++;
#else
	CPCursor *p = new CPCursor(&cursorInitStruct);
	ShiAssert(p);
	if(!p) return;

	mpCursors.push_back(p);
	mCursorTally++;
#endif
}

void CockpitManager::CreateDigits(int idNum, FILE* pcockpitDataFile) {

	char					plineBuffer[MAX_LINE_BUFFER] = "";
	char*					plinePtr;
	char					*ptoken;
	char					pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
	const int				lineLen = MAX_LINE_BUFFER - 1;

	DigitsInitStr			digitsInitStr={0};
	ObjectInitStr			objectInitStr;
	int						destIndex = 0;
	RECT					*pdestRects=NULL;
	int						srcIndex =0;
	RECT					*psrcRects;

	#ifdef USE_SH_POOLS
	psrcRects = (RECT *)MemAllocPtr(gCockMemPool,sizeof(RECT)*10,FALSE);
	#else
	psrcRects	= new RECT[10];
	#endif

	objectInitStr.bsrcRect.top		= 0;
	objectInitStr.bsrcRect.left	= 0;
	objectInitStr.bsrcRect.bottom = 0;
	objectInitStr.bsrcRect.right	= 0;
	objectInitStr.bdestRect			= objectInitStr.bsrcRect;
	objectInitStr.bsurface			= -1;
	digitsInitStr.numDestDigits   = 0;

	fgets(plineBuffer, lineLen, pcockpitDataFile);
   gDebugLineNum ++;
	plinePtr = plineBuffer;
	ptoken = FindToken(&plinePtr, pseparators);	

	while(strcmpi(ptoken, END_MARKER)){

		if(!strcmpi(ptoken, PROP_CYCLEBITS_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%x", &objectInitStr.cycleBits);
		}
		else if(!strcmpi(ptoken, PROP_NUMDIGITS_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &digitsInitStr.numDestDigits);

			#ifdef USE_SH_POOLS
			if(digitsInitStr.numDestDigits > 0) {
				pdestRects = (RECT *)MemAllocPtr(gCockMemPool,sizeof(RECT)*digitsInitStr.numDestDigits,FALSE);
			}
			#else
			pdestRects = new RECT[digitsInitStr.numDestDigits];
			#endif

		}
		else if(!strcmpi(ptoken, PROP_SRCLOC_STR)) {
			F4Assert(srcIndex < 10);
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &(psrcRects[srcIndex].top), 
													&(psrcRects[srcIndex].left), 
													&(psrcRects[srcIndex].bottom), 
													&(psrcRects[srcIndex].right));
			srcIndex++;
		}
		else if(!strcmpi(ptoken, PROP_DESTLOC_STR)) {
			F4Assert(destIndex < digitsInitStr.numDestDigits);

			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &(pdestRects[destIndex].top), 
														&(pdestRects[destIndex].left), 
														&(pdestRects[destIndex].bottom), 
														&(pdestRects[destIndex].right));
	
			destIndex++;
		}
		else if(!strcmpi(ptoken, PROP_CALLBACKSLOT_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &objectInitStr.callbackSlot);
		}
		else if(!strcmpi(ptoken, PROP_PERSISTANT_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &objectInitStr.persistant);
		}
		else {
         CockpitMessage(ptoken, "Digit", gDebugLineNum);
		}

		fgets(plineBuffer, lineLen, pcockpitDataFile);
      gDebugLineNum ++;
		plinePtr = plineBuffer;
		ptoken = FindToken(&plinePtr, pseparators);	
	}

	digitsInitStr.psrcRects = psrcRects;
	digitsInitStr.pdestRects = pdestRects;

	objectInitStr.scale			= mScale;

	objectInitStr.idNum			= idNum;
	objectInitStr.pOTWImage		= mpOTWImage;
	objectInitStr.pTemplate		= gpTemplateSurface;
	objectInitStr.pCPManager	= this;
	objectInitStr.destRect		= digitsInitStr.pdestRects[0];
	digitsInitStr.psrcRects		= psrcRects;
	digitsInitStr.pdestRects	= pdestRects;

#ifndef _CPMAN_USE_STL_CONTAINERS
	mpObjects[mObjectTally]		= new CPDigits(&objectInitStr, &digitsInitStr);
	mObjectTally++;
#else
	CPDigits *p = new CPDigits(&objectInitStr, &digitsInitStr);
	ShiAssert(p);
	if(!p) return;

	mpObjects.push_back(p);
	mObjectTally++;
#endif
}

void CockpitManager::CreateIndicator(int idNum, FILE* pcockpitDataFile) {

	char						plineBuffer[MAX_LINE_BUFFER] = "";
	char*						plinePtr;
	char						*ptoken;
	char						porientationStr[15] = "";
	char						pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
	const int				lineLen = MAX_LINE_BUFFER - 1;
	IndicatorInitStr		indicatorInitStr={0};
	ObjectInitStr			objectInitStr;
	int						destIndex = 0;


	objectInitStr.bsrcRect.top			= 0;
	objectInitStr.bsrcRect.left		= 0;
	objectInitStr.bsrcRect.bottom		= 0;
	objectInitStr.bsrcRect.right		= 0;
	objectInitStr.bdestRect				= objectInitStr.bsrcRect;
	objectInitStr.bsurface				= -1;
	indicatorInitStr.calibrationVal	= 0;
	indicatorInitStr.pdestRect       = NULL;
	indicatorInitStr.psrcRect        = NULL;
	indicatorInitStr.minPos          = NULL;
	indicatorInitStr.maxPos          = NULL;
	indicatorInitStr.numTapes        = 0;

	fgets(plineBuffer, lineLen, pcockpitDataFile);
   gDebugLineNum ++;
	plinePtr = plineBuffer;
	ptoken = FindToken(&plinePtr, pseparators);	

	while(strcmpi(ptoken, END_MARKER)){

		if(!strcmpi(ptoken, PROP_MINVAL_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%f", &indicatorInitStr.minVal);
		}
		else if(!strcmpi(ptoken, PROP_CYCLEBITS_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%x", &objectInitStr.cycleBits);
		}
		else if(!strcmpi(ptoken, PROP_MAXVAL_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%f", &indicatorInitStr.maxVal);
		}
		else if(!strcmpi(ptoken, PROP_MINPOS_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &indicatorInitStr.minPos[destIndex]);
		}
		else if(!strcmpi(ptoken, PROP_MAXPOS_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &indicatorInitStr.maxPos[destIndex]);
		}
		else if(!strcmpi(ptoken, PROP_NUMTAPES_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &indicatorInitStr.numTapes);
			#ifdef USE_SH_POOLS
			if(indicatorInitStr.numTapes > 0) {
				indicatorInitStr.pdestRect = (RECT *)MemAllocPtr(gCockMemPool,sizeof(RECT)*indicatorInitStr.numTapes,FALSE);
				indicatorInitStr.psrcRect = (RECT *)MemAllocPtr(gCockMemPool,sizeof(RECT)*indicatorInitStr.numTapes,FALSE);
				indicatorInitStr.minPos = (int *)MemAllocPtr(gCockMemPool,sizeof(int)*indicatorInitStr.numTapes,FALSE);
				indicatorInitStr.maxPos = (int *)MemAllocPtr(gCockMemPool,sizeof(int)*indicatorInitStr.numTapes,FALSE);
			}
			#else
			indicatorInitStr.pdestRect = new RECT[indicatorInitStr.numTapes];
			indicatorInitStr.psrcRect = new RECT[indicatorInitStr.numTapes];
			indicatorInitStr.minPos = new int[indicatorInitStr.numTapes];
			indicatorInitStr.maxPos = new int[indicatorInitStr.numTapes];
			#endif
		}
		else if(!strcmpi(ptoken, PROP_CALIBRATIONVAL_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &indicatorInitStr.calibrationVal);
		}
		else if(!strcmpi(ptoken, PROP_SRCLOC_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &(indicatorInitStr.psrcRect[destIndex].top), 
													&(indicatorInitStr.psrcRect[destIndex].left), 
													&(indicatorInitStr.psrcRect[destIndex].bottom), 
													&(indicatorInitStr.psrcRect[destIndex].right));
		}
		else if(!strcmpi(ptoken, PROP_DESTLOC_STR)) {
			F4Assert(destIndex < indicatorInitStr.numTapes);

			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &(indicatorInitStr.pdestRect[destIndex].top), 
														&(indicatorInitStr.pdestRect[destIndex].left), 
														&(indicatorInitStr.pdestRect[destIndex].bottom), 
														&(indicatorInitStr.pdestRect[destIndex].right));
	
			objectInitStr.transparencyType	= CPOPAQUE;
			if (destIndex == 0)
			{
				for (int tmpVar=1; tmpVar<indicatorInitStr.numTapes; tmpVar++)
				{
					indicatorInitStr.maxPos[tmpVar] = indicatorInitStr.maxPos[0];
					indicatorInitStr.minPos[tmpVar] = indicatorInitStr.minPos[0];
					indicatorInitStr.psrcRect[tmpVar] = indicatorInitStr.psrcRect[0];
				}
			}
			destIndex++;
		}
		else if(!strcmpi(ptoken, PROP_CALLBACKSLOT_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &objectInitStr.callbackSlot);
		}
		else if(!strcmpi(ptoken, PROP_PERSISTANT_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &objectInitStr.persistant);
		}
		else if(!strcmpi(ptoken, PROP_ORIENTATION_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%s", porientationStr);
			if(!strcmpi(porientationStr, PROP_HORIZONTAL_STR)) {
				indicatorInitStr.orientation = IND_HORIZONTAL;
			}
			else if(!strcmpi(porientationStr, PROP_VERTICAL_STR)) {
				indicatorInitStr.orientation = IND_VERTICAL;
			}
			else {
				ShiWarning("Bad Orientation String");
			}
		}
		else {
         CockpitMessage(ptoken, "Indicator", gDebugLineNum);
		}

		fgets(plineBuffer, lineLen, pcockpitDataFile);
      gDebugLineNum ++;
		plinePtr = plineBuffer;
		ptoken = FindToken(&plinePtr, pseparators);	
	}

	objectInitStr.scale			= mScale;
	objectInitStr.idNum			= idNum;
	objectInitStr.pOTWImage		= mpOTWImage;
	objectInitStr.pTemplate		= gpTemplateSurface;
	objectInitStr.pCPManager	= this;
	objectInitStr.destRect		= *indicatorInitStr.pdestRect;

#ifndef _CPMAN_USE_STL_CONTAINERS
	mpObjects[mObjectTally]		= new CPIndicator(&objectInitStr, &indicatorInitStr);
	delete [] indicatorInitStr.pdestRect;
	delete [] indicatorInitStr.psrcRect;
	delete [] indicatorInitStr.minPos;
	delete [] indicatorInitStr.maxPos;
	mObjectTally++;
#else
	CPIndicator *p = new CPIndicator(&objectInitStr, &indicatorInitStr);
	ShiAssert(p);
	if(!p) return;

	delete [] indicatorInitStr.pdestRect;
	delete [] indicatorInitStr.psrcRect;
	delete [] indicatorInitStr.minPos;
	delete [] indicatorInitStr.maxPos;

	mpObjects.push_back(p);
	mObjectTally++;
#endif
}

//====================================================//
// CockpitManager::CreateDial
//====================================================//

void CockpitManager::CreateDial(int idNum, FILE* pcockpitDataFile) {

	char						plineBuffer[MAX_LINE_BUFFER] = "";
	char*						plinePtr;
	char						*ptoken;
	char						pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
	const int				lineLen = MAX_LINE_BUFFER - 1;
	DialInitStr				dialInitStr={0};
	ObjectInitStr			objectInitStr;
	int						valuesIndex = 0;
	int						pointsIndex = 0;

	fgets(plineBuffer, lineLen, pcockpitDataFile);
   gDebugLineNum ++;
	plinePtr = plineBuffer;
	ptoken = FindToken(&plinePtr, pseparators);	
	dialInitStr.ppoints = NULL;
	dialInitStr.pvalues = NULL;

	while(strcmpi(ptoken, END_MARKER)){

		if(!strcmpi(ptoken, PROP_NUMENDPOINTS_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &dialInitStr.endPoints);
			#ifdef USE_SH_POOLS
			if(dialInitStr.endPoints > 0) {
				dialInitStr.ppoints = (float *)MemAllocPtr(gCockMemPool,sizeof(float)*dialInitStr.endPoints,FALSE);
				dialInitStr.pvalues = (float *)MemAllocPtr(gCockMemPool,sizeof(float)*dialInitStr.endPoints,FALSE);
			}
			#else
			dialInitStr.ppoints	= new float[dialInitStr.endPoints];
			dialInitStr.pvalues = new float[dialInitStr.endPoints];
			#endif
		}
		else if(!strcmpi(ptoken, PROP_CYCLEBITS_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%x", &objectInitStr.cycleBits);
		}
		else if(!strcmpi(ptoken, PROP_POINTS_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);
			while(ptoken) {
				sscanf(ptoken, "%f", &dialInitStr.ppoints[pointsIndex]);
				ptoken = FindToken(&plinePtr, pseparators);
				pointsIndex++;
			}
		}
		else if(!strcmpi(ptoken, PROP_VALUES_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);
			while(ptoken) {
				sscanf(ptoken, "%f", &dialInitStr.pvalues[valuesIndex]);
				ptoken = FindToken(&plinePtr, pseparators);
				valuesIndex++;
			}
		}
		else if(!strcmpi(ptoken, PROP_RADIUS0_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &dialInitStr.radius0);
		}
		else if(!strcmpi(ptoken, PROP_RADIUS1_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &dialInitStr.radius1);
		}
		else if(!strcmpi(ptoken, PROP_RADIUS2_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &dialInitStr.radius2);
		}
		else if(!strcmpi(ptoken, PROP_COLOR0_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%lx", &dialInitStr.color0);
		}
		else if(!strcmpi(ptoken, PROP_COLOR1_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%lx", &dialInitStr.color1);
		}
		else if(!strcmpi(ptoken, PROP_COLOR2_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%lx", &dialInitStr.color2);
		}

		else if(!strcmpi(ptoken, PROP_SRCLOC_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &(dialInitStr.srcRect.top), 
													&(dialInitStr.srcRect.left), 
													&(dialInitStr.srcRect.bottom), 
													&(dialInitStr.srcRect.right));
		}
		else if(!strcmpi(ptoken, PROP_DESTLOC_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &(objectInitStr.destRect.top), 
														&(objectInitStr.destRect.left), 
														&(objectInitStr.destRect.bottom), 
														&(objectInitStr.destRect.right));
	
			objectInitStr.transparencyType	= CPOPAQUE;
		}
		else if(!strcmpi(ptoken, PROP_CALLBACKSLOT_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &objectInitStr.callbackSlot);
		}

		else if(!strcmpi(ptoken, PROP_PERSISTANT_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &objectInitStr.persistant);

			if(!objectInitStr.persistant) {
				objectInitStr.bsrcRect.top		= 0;
				objectInitStr.bsrcRect.left	= 0;
				objectInitStr.bsrcRect.bottom = 0;
				objectInitStr.bsrcRect.right	= 0;

				objectInitStr.bdestRect			= objectInitStr.bsrcRect;
				objectInitStr.bsurface			= -1;
			}
		}
		else if(!strcmpi(ptoken, PROP_BSRCLOC_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &(objectInitStr.bsrcRect.top), 
													&(objectInitStr.bsrcRect.left), 
													&(objectInitStr.bsrcRect.bottom), 
													&(objectInitStr.bsrcRect.right));
		}
		else if(!strcmpi(ptoken, PROP_BDESTLOC_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &(objectInitStr.bdestRect.top), 
													&(objectInitStr.bdestRect.left), 
													&(objectInitStr.bdestRect.bottom), 
													&(objectInitStr.bdestRect.right));
		}
		else if(!strcmpi(ptoken, PROP_BSURFACE_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d", &(objectInitStr.bsurface));
		}
		else {
         CockpitMessage(ptoken, "Dial", gDebugLineNum);
		}

		fgets(plineBuffer, lineLen, pcockpitDataFile);
      gDebugLineNum ++;
		plinePtr = plineBuffer;
		ptoken = FindToken(&plinePtr, pseparators);	
	}

	objectInitStr.scale				= mScale;
	objectInitStr.idNum				= idNum;
	objectInitStr.pOTWImage			= mpOTWImage;
	objectInitStr.pTemplate			= gpTemplateSurface;
	objectInitStr.pCPManager		= this;

#ifndef _CPMAN_USE_STL_CONTAINERS
	mpObjects[mObjectTally]		= new CPDial(&objectInitStr, &dialInitStr);
	mObjectTally++;
#else
	CPDial *p = new CPDial(&objectInitStr, &dialInitStr);
	ShiAssert(p);
	if(!p) return;

	mpObjects.push_back(p);
	mObjectTally++;
#endif
}


//====================================================//
// CockpitManager::CreateSurface
//====================================================//

void CockpitManager::CreateSurface(int idNum, FILE* pcockpitDataFile) {
	char				plineBuffer[MAX_LINE_BUFFER] = "";
	char*				plinePtr;
	char				*ptoken;
	char				pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
	char				pfileName[32] = "";
	char				psurfaceFile[MAX_LINE_BUFFER];
	const int		lineLen = MAX_LINE_BUFFER - 1;
	SurfaceInitStr surfaceInitStruct={0};
	RECT				destRect;

	fgets(plineBuffer, lineLen, pcockpitDataFile);
   gDebugLineNum ++;
	plinePtr = plineBuffer;
	ptoken = FindToken(&plinePtr, pseparators);	
	surfaceInitStruct.srcRect.top = 0;
	surfaceInitStruct.srcRect.bottom = 0;
	surfaceInitStruct.srcRect.left = 0;
	surfaceInitStruct.srcRect.right = 0;

	while(strcmpi(ptoken, END_MARKER)){

		if(!strcmpi(ptoken, PROP_FILENAME_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%s", pfileName);

			if(m_eCPVisType == MapVisId(VIS_F16C))
				sprintf(psurfaceFile, "%s%s", COCKPIT_DIR, pfileName);
			else
			{
				sprintf(psurfaceFile, "%s%d\\%s", COCKPIT_DIR, MapVisId(m_eCPVisType), pfileName);

				if(!ResExistFile(psurfaceFile))
				{
					sprintf(psurfaceFile, "%s%s\\%s", COCKPIT_DIR, m_eCPName, pfileName);
					
					if(!ResExistFile(psurfaceFile))
					{
						sprintf(psurfaceFile, "%s%s\\%s", COCKPIT_DIR, m_eCPNameNCTR, pfileName);
						
						if(!ResExistFile(psurfaceFile))
						{
							// F16C fallback
							sprintf(psurfaceFile, "%s%s", COCKPIT_DIR, pfileName);
						}
					}
				}
			}
		}
		else if(!strcmpi(ptoken, PROP_SRCLOC_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &(surfaceInitStruct.srcRect.top), 
														&(surfaceInitStruct.srcRect.left), 
														&(surfaceInitStruct.srcRect.bottom), 
														&(surfaceInitStruct.srcRect.right));
		}
		else {
         CockpitMessage(ptoken, "Surface", gDebugLineNum);
		}

		fgets(plineBuffer, lineLen, pcockpitDataFile);
      gDebugLineNum ++;
		plinePtr = plineBuffer;
		ptoken = FindToken(&plinePtr, pseparators);	
	}

	destRect.top		= 0;
	destRect.left		= 0;
	destRect.bottom	= surfaceInitStruct.srcRect.bottom - surfaceInitStruct.srcRect.top + 1;
	destRect.right		= surfaceInitStruct.srcRect.right - surfaceInitStruct.srcRect.left + 1;

	#ifdef USE_SH_POOLS
	surfaceInitStruct.psrcBuffer = (BYTE *)MemAllocPtr(gCockMemPool,sizeof(BYTE)*destRect.bottom*destRect.right,FALSE);
	#else
	surfaceInitStruct.psrcBuffer = new BYTE[destRect.bottom * destRect.right];
	#endif

	surfaceInitStruct.srcRect.right	+= 1;
	surfaceInitStruct.srcRect.bottom += 1;

	ImageCopy(mpLoadBuffer, surfaceInitStruct.psrcBuffer, mLoadBufferWidth, &surfaceInitStruct.srcRect);

	surfaceInitStruct.idNum			= idNum;

	surfaceInitStruct.pOtwImage	= OTWDriver.OTWImage;

#ifndef _CPMAN_USE_STL_CONTAINERS
	mpSurfaces[mSurfaceTally]		= new CPSurface(&surfaceInitStruct);
	mSurfaceTally++;
#else
	CPSurface *p = new CPSurface(&surfaceInitStruct);
	ShiAssert(p);
	if(!p) return;

	mpSurfaces.push_back(p);
	mSurfaceTally++;
#endif
}

//====================================================//
// CockpitManager::ImageCopy
//====================================================//

void CockpitManager::ImageCopy(GLubyte* ploadBuffer, 
										 GLubyte* psrcBuffer, 
										 int width, 
										 RECT* psrcRect)
{
	int	i, j, n;
	int	rowIndex;

	for(i = psrcRect->top, n = 0; i < psrcRect->bottom; i++) {
		rowIndex = i * width;
		for(j = psrcRect->left; j < psrcRect->right; j++, n++) {
			psrcBuffer[n] = ploadBuffer[rowIndex + j];		
		}
	}
}


//====================================================//
// CockpitManager::CreatePanel
//====================================================//

void CockpitManager::CreatePanel(int idNum, FILE* pcockpitDataFile) {
	char				plineBuffer[MAX_LINE_BUFFER] = "";
	char*				plinePtr;
	char				*ptoken;
	char				pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
	const int		lineLen = MAX_LINE_BUFFER - 1;
	PanelInitStr	*ppanelInitStr;
	int				surfaceIndex = 0;
	int				objectIndex = 0;
	int				buttonViewIndex = 0;
   int            i;
	char				ptransparencyStr[32] = "";

	#ifdef USE_SH_POOLS
	ppanelInitStr = (PanelInitStr *)MemAllocPtr(gCockMemPool,sizeof(PanelInitStr),FALSE);
	#else
	ppanelInitStr = new PanelInitStr;
	#endif

	memset(ppanelInitStr->pviewRects, 0, sizeof(RECT*) * BOUNDS_TOTAL);
	ppanelInitStr->psurfaceData		= NULL;
	ppanelInitStr->pobjectIDs			= NULL;
	ppanelInitStr->pbuttonViewIDs		= NULL;
	ppanelInitStr->numButtonViews		= NULL;
	ppanelInitStr->doGeometry	      = FALSE;
	ppanelInitStr->mfdFont           = mMFDFont;
	ppanelInitStr->hudFont           = mHudFont;
	ppanelInitStr->dedFont           = mDEDFont;
	ppanelInitStr->pan = 0; // JPO more inits
	ppanelInitStr->tilt = 0;
	ppanelInitStr->maskTop = 0;

	fgets(plineBuffer, lineLen, pcockpitDataFile);
   gDebugLineNum ++;
	plinePtr = plineBuffer;
	ptoken = FindToken(&plinePtr, pseparators);	

	while(strcmpi(ptoken, END_MARKER)) {

		if(!strcmpi(ptoken, PROP_NUMSURFACES_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &ppanelInitStr->numSurfaces);
			#ifdef USE_SH_POOLS
			if(ppanelInitStr->numSurfaces > 0) {
				ppanelInitStr->psurfaceData = (PanelSurfaceStr *)MemAllocPtr(gCockMemPool,sizeof(PanelSurfaceStr)*ppanelInitStr->numSurfaces,FALSE);
			}
			#else
			ppanelInitStr->psurfaceData = new PanelSurfaceStr[ppanelInitStr->numSurfaces];
			#endif
		}
		else if(!strcmpi(ptoken, PROP_DOGEOMETRY_STR)) {
			ppanelInitStr->doGeometry	= TRUE;
		}
		else if(!strcmpi(ptoken, PROP_MFDLEFT_STR)) {
		
			#ifdef USE_SH_POOLS
			ppanelInitStr->pviewRects[BOUNDS_MFDLEFT]   = (RECT *)MemAllocPtr(gCockMemPool,sizeof(RECT),FALSE);
			#else
			ppanelInitStr->pviewRects[BOUNDS_MFDLEFT]	= new RECT;
			#endif
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &(ppanelInitStr->pviewRects[BOUNDS_MFDLEFT]->top), 
														&(ppanelInitStr->pviewRects[BOUNDS_MFDLEFT]->left), 
														&(ppanelInitStr->pviewRects[BOUNDS_MFDLEFT]->bottom), 
														&(ppanelInitStr->pviewRects[BOUNDS_MFDLEFT]->right));
		}
		else if(!strcmpi(ptoken, PROP_MFDRIGHT_STR)) {
		
			#ifdef USE_SH_POOLS
			ppanelInitStr->pviewRects[BOUNDS_MFDRIGHT]   = (RECT *)MemAllocPtr(gCockMemPool,sizeof(RECT),FALSE);
			#else
			ppanelInitStr->pviewRects[BOUNDS_MFDRIGHT]	= new RECT;
			#endif
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &(ppanelInitStr->pviewRects[BOUNDS_MFDRIGHT]->top), 
														&(ppanelInitStr->pviewRects[BOUNDS_MFDRIGHT]->left), 
														&(ppanelInitStr->pviewRects[BOUNDS_MFDRIGHT]->bottom), 
														&(ppanelInitStr->pviewRects[BOUNDS_MFDRIGHT]->right));
		}
		else if(!strcmpi(ptoken, PROP_HUD_STR)) {
	
			#ifdef USE_SH_POOLS
			ppanelInitStr->pviewRects[BOUNDS_HUD]   = (RECT *)MemAllocPtr(gCockMemPool,sizeof(RECT),FALSE);
			#else
			ppanelInitStr->pviewRects[BOUNDS_HUD]	= new RECT;
			#endif
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &(ppanelInitStr->pviewRects[BOUNDS_HUD]->top), 
														&(ppanelInitStr->pviewRects[BOUNDS_HUD]->left), 
														&(ppanelInitStr->pviewRects[BOUNDS_HUD]->bottom), 
														&(ppanelInitStr->pviewRects[BOUNDS_HUD]->right));
		}
		else if(!strcmpi(ptoken, PROP_RWR_STR)) {

			#ifdef USE_SH_POOLS
			ppanelInitStr->pviewRects[BOUNDS_RWR]   = (RECT *)MemAllocPtr(gCockMemPool,sizeof(RECT),FALSE);
			#else
			ppanelInitStr->pviewRects[BOUNDS_RWR]	= new RECT;
			#endif
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &(ppanelInitStr->pviewRects[BOUNDS_RWR]->top), 
														&(ppanelInitStr->pviewRects[BOUNDS_RWR]->left), 
														&(ppanelInitStr->pviewRects[BOUNDS_RWR]->bottom), 
														&(ppanelInitStr->pviewRects[BOUNDS_RWR]->right));

		}
		else if(!strcmpi(ptoken, PROP_MOUSEBOUNDS_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &(ppanelInitStr->mouseBounds.top), 
														&(ppanelInitStr->mouseBounds.left), 
														&(ppanelInitStr->mouseBounds.bottom), 
														&(ppanelInitStr->mouseBounds.right));
		}
		else if(!strcmpi(ptoken, PROP_ADJPANELS_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d %d %d %d %d", &(ppanelInitStr->adjacentPanels.N), 
														&(ppanelInitStr->adjacentPanels.NE), 
														&(ppanelInitStr->adjacentPanels.E), 
														&(ppanelInitStr->adjacentPanels.SE),
														&(ppanelInitStr->adjacentPanels.S),
														&(ppanelInitStr->adjacentPanels.SW),
														&(ppanelInitStr->adjacentPanels.W),
														&(ppanelInitStr->adjacentPanels.NW));
		}
		else if(!strcmpi(ptoken, PROP_SURFACES_STR)) {
			ptoken = FindToken(&plinePtr, "\n=;");
			sscanf(ptoken, "%d %d %s %d %d %d %d", &ppanelInitStr->psurfaceData[surfaceIndex].surfaceNum, 
																&ppanelInitStr->psurfaceData[surfaceIndex].persistant,
																ptransparencyStr,
																&ppanelInitStr->psurfaceData[surfaceIndex].destRect.top, 
																&ppanelInitStr->psurfaceData[surfaceIndex].destRect.left, 
																&ppanelInitStr->psurfaceData[surfaceIndex].destRect.bottom, 
																&ppanelInitStr->psurfaceData[surfaceIndex].destRect.right);
	
			ppanelInitStr->psurfaceData[surfaceIndex].psurface = NULL;			
			
			if(!strcmpi(ptransparencyStr, PROP_TRANSPARENT_STR)) {
				ppanelInitStr->psurfaceData[surfaceIndex].transparencyType = CPTRANSPARENT;
			}
			else if(!strcmpi(ptransparencyStr, PROP_OPAQUE_STR)) {
				ppanelInitStr->psurfaceData[surfaceIndex].transparencyType = CPOPAQUE;
			}
			else {
				ShiWarning("Bad Transparency Type"); //couldn't read in transparency type
			}

			surfaceIndex++;
		}
		else if(!strcmpi(ptoken, PROP_NUMOBJECTS_STR)) {
			ptoken = FindToken(&plinePtr, "\n=;");
			sscanf(ptoken, "%d", &ppanelInitStr->numObjects);
			#ifdef USE_SH_POOLS
			if(ppanelInitStr->numObjects > 0) {
				ppanelInitStr->pobjectIDs   = (int *)MemAllocPtr(gCockMemPool,sizeof(int)*ppanelInitStr->numObjects,FALSE);
			}
			#else
			ppanelInitStr->pobjectIDs	= new int[ppanelInitStr->numObjects];
			#endif
		}
		else if(!strcmpi(ptoken, PROP_OBJECTS_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);
			while(ptoken) {
				sscanf(ptoken, "%d", &ppanelInitStr->pobjectIDs[objectIndex]);
				ptoken = FindToken(&plinePtr, pseparators);
				objectIndex++;
			}
		}
		else if(!strcmpi(ptoken, PROP_OFFSET_STR)) {
			ptoken = FindToken(&plinePtr, "\n=;");
			sscanf(ptoken, "%d %d", &ppanelInitStr->xOffset, &ppanelInitStr->yOffset);
		}
		else if(!strcmpi(ptoken, PROP_NUMBUTTONVIEWS_STR)) {
			ptoken = FindToken(&plinePtr, "\n=;");
			sscanf(ptoken, "%d", &ppanelInitStr->numButtonViews);
			if(ppanelInitStr->numButtonViews > 0) {
			#ifdef USE_SH_POOLS
				ppanelInitStr->pbuttonViewIDs   = (int *)MemAllocPtr(gCockMemPool,sizeof(int)*ppanelInitStr->numButtonViews,FALSE);
			#else
			ppanelInitStr->pbuttonViewIDs	= new int[ppanelInitStr->numButtonViews];
			#endif
			}
		}
		else if(!strcmpi(ptoken, PROP_BUTTONVIEWS_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);
			while(ptoken) {
				sscanf(ptoken, "%d", &ppanelInitStr->pbuttonViewIDs[buttonViewIndex]);
				ptoken = FindToken(&plinePtr, pseparators);
				buttonViewIndex++;
			}
		}
		else if(!strcmpi(ptoken, PROP_PANTILT_STR)) {
			ptoken = FindToken(&plinePtr, "\n=;");
			sscanf(ptoken, "%f %f", &ppanelInitStr->pan, &ppanelInitStr->tilt);
		}
		else if(!strcmpi(ptoken, PROP_HUDFONT)) {
			ptoken = FindToken(&plinePtr, "\n=;");
			sscanf(ptoken, "%d", &ppanelInitStr->hudFont);
		}
		else if(!strcmpi(ptoken, PROP_MFDFONT)) {
			ptoken = FindToken(&plinePtr, "\n=;");
			sscanf(ptoken, "%d", &ppanelInitStr->mfdFont);
		}
		else if(!strcmpi(ptoken, PROP_DEDFONT)) {
			ptoken = FindToken(&plinePtr, "\n=;");
			sscanf(ptoken, "%d", &ppanelInitStr->dedFont);
		}
		else if(!strcmpi(ptoken, PROP_MASKTOP_STR)) {
			ptoken = FindToken(&plinePtr, "\n=;");
			sscanf(ptoken, "%d", &ppanelInitStr->maskTop);
		}
		else if(!strcmpi(ptoken, PROP_CURSORID_STR)) {
			ptoken = FindToken(&plinePtr, "\n=;");
			sscanf(ptoken, "%d", &ppanelInitStr->defaultCursor);
		}
		else if(!strcmpi(ptoken, PROP_OSBLEFT_STR)) {
         for (i=0; i<20; i++)
         {
   			ptoken = FindToken(&plinePtr, pseparators);
				sscanf(ptoken, "%f", &ppanelInitStr->osbLocation[0][i][0]);
   			ptoken = FindToken(&plinePtr, pseparators);
				sscanf(ptoken, "%f", &ppanelInitStr->osbLocation[0][i][1]);
			}
		}
		else if(!strcmpi(ptoken, PROP_OSBRIGHT_STR)) {
         for (i=0; i<20; i++)
         {
   			ptoken = FindToken(&plinePtr, pseparators);
				sscanf(ptoken, "%f", &ppanelInitStr->osbLocation[1][i][0]);
   			ptoken = FindToken(&plinePtr, pseparators);
				sscanf(ptoken, "%f", &ppanelInitStr->osbLocation[1][i][1]);
			}
		}
// 2000-11-12 ADDED BY S.G. SO COMMENTED LINE DON'T TRIGGER AN ASSERT
		else if(!strcmpi(ptoken, "//"))
			;
// END OF ADDED SECTION
		else {
         CockpitMessage(ptoken, "Panel", gDebugLineNum);
		}

		fgets(plineBuffer, lineLen, pcockpitDataFile);
      gDebugLineNum ++;
		plinePtr = plineBuffer;
		ptoken	= FindToken(&plinePtr, pseparators);	
	}

	F4Assert(surfaceIndex == ppanelInitStr->numSurfaces);		// should have as many surfaces as specified in file
	F4Assert(objectIndex == ppanelInitStr->numObjects);		// should have as many objects as specified in file

	ppanelInitStr->scale				= mScale;
	ppanelInitStr->cockpitWidth	= DisplayOptions.DispWidth;
	ppanelInitStr->cockpitHeight	= DisplayOptions.DispHeight;
	ppanelInitStr->idNum				= idNum;
	ppanelInitStr->pOtwImage		= OTWDriver.OTWImage;

#ifndef _CPMAN_USE_STL_CONTAINERS
	mpPanels[mPanelTally]			= new CPPanel(ppanelInitStr);
	mPanelTally++;
#else
	CPPanel *p = new CPPanel(ppanelInitStr);
	ShiAssert(p);
	if(!p) return;
	mpPanels.push_back(p);
	mPanelTally++;
#endif
}


//====================================================//
// CockpitManager::ResolveReferences
//====================================================//

void CockpitManager::ResolveReferences(void) {

	int	i, j, k;
	int	panelSurfaces;
	int	panelObjects;
	int	panelButtonViews;
	int	surfaceId;
	int	objectId;
	int	buttonId;
	int	buttonViewId;
	BOOL	found;

	// loop thru all panels
	for(i = 0; i < mPanelTally; i++) {
		panelSurfaces	= mpPanels[i]->mNumSurfaces;

		// loop thru each panel's list of surfaces
		for(j = 0; j < panelSurfaces; j++) {
			surfaceId	= mpPanels[i]->mpSurfaceData[j].surfaceNum;
			found			= FALSE;
			k = 0;
		
			// search cpmanager's list of surface pointers 
			while((!found) && (k < mSurfaceTally)) {
				if(mpSurfaces[k]->mIdNum == surfaceId) {
					mpPanels[i]->mpSurfaceData[j].psurface = mpSurfaces[k];
					found	= TRUE;
				}
				else {
					k++;
				}
			}
			F4Assert(found);    //couldn't find the surface in our list
		}
	}

	// loop thru all panels
	for(i = 0; i < mPanelTally; i++) {
		panelObjects = mpPanels[i]->mNumObjects;

		// loop thru each of the panel's objects
		for(j = 0; j < panelObjects; j++) {
			objectId		= mpPanels[i]->mpObjectIDs[j];
			found			= FALSE;
			k = 0;
			
			// search cpmanager's list of object pointers
			while((!found) && (k < mObjectTally)) {
				if(mpObjects[k]->mIdNum == objectId) {
					mpPanels[i]->mpObjects[j] = mpObjects[k];
					found = TRUE;
				}
				else {
					k++;
				}
			}
			F4Assert(found);		//couldn't find the object in our list
		}
	}

	for(i = 0; i < mButtonViewTally; i++) {
		
		found		= FALSE;
		j			= 0;
		buttonId = mpButtonViews[i]->GetParentButton();

		while(!found && j < mButtonTally) {

			if(mpButtonObjects[j]->GetId() == buttonId) {
				mpButtonObjects[j]->AddView(mpButtonViews[i]);
				found		= TRUE;
			}
			else {
				j++;
			}
		}

		F4Assert(found);
	}


	for(i = 0; i < mButtonTally; i++) {
		if(mpButtonObjects[i]->GetSound(1) >= 0) {
			mpButtonObjects[i]->SetSound(1, mpSoundList->GetSoundIndex(mpButtonObjects[i]->GetSound(1)));
			if(mpButtonObjects[i]->GetSound(2) >= 0) {
				mpButtonObjects[i]->SetSound(2, mpSoundList->GetSoundIndex(mpButtonObjects[i]->GetSound(2)));
			}
		}		
	}


	for(i = 0; i < mPanelTally; i++) {
		panelButtonViews = mpPanels[i]->mNumButtonViews;

		// loop thru each of the panel's objects
		for(j = 0; j < panelButtonViews; j++) {
			buttonViewId	= mpPanels[i]->mpButtonViewIDs[j];
			found				= FALSE;
			k					= 0;
			
			// search cpmanager's list of object pointers
			while((!found) && (k < mButtonViewTally)) {
				if(mpButtonViews[k]->GetId() == buttonViewId) {
					mpPanels[i]->mpButtonViews[j] = mpButtonViews[k];
					found = TRUE;
				}
				else {
					k++;
				}
			}
			F4Assert(found);		//couldn't find the object in our list
		}
	}


	if(mpIcp) {
										// loop thru each of the panel's buttons
		buttonId		= ICP_INITIAL_PRIMARY_BUTTON;
		found			= FALSE;
		i = 0;
			
		// search cpmanager's list of button pointers
		while((!found) && (i < mButtonTally)) {
			if(mpButtonObjects[i]->GetId() == buttonId) {
				mpIcp->InitPrimaryExclusiveButton(mpButtonObjects[i]);
				found = TRUE;
			}
			else {
				i++;
			}
		}

		F4Assert(found);		//couldn't find the button in our list	


		buttonId		= ICP_INITIAL_TERTIARY_BUTTON;
		found			= FALSE;
		i = 0;
			
		// search cpmanager's list of button pointers
		while((!found) && (i < mButtonTally)) {
			if(mpButtonObjects[i]->GetId() == buttonId) {
				mpIcp->InitTertiaryExclusiveButton(mpButtonObjects[i]);
				found = TRUE;
			}
			else {
				i++;
			}
		}

		F4Assert(found);		//couldn't find the button in our list	

	}
}

//====================================================//
// CockpitManager::CreateKneeView
//====================================================//

void CockpitManager::CreateKneeView(int idNum, FILE* pcockpitDataFile)
{
	char						plineBuffer[MAX_LINE_BUFFER] = "";
	char*						plinePtr;
	char						*ptoken;
	char						pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
	const int				lineLen = MAX_LINE_BUFFER - 1;
	ObjectInitStr			objectInitStr;

	objectInitStr.cycleBits			= 0x0000;

	objectInitStr.bsrcRect.top		= 0;
	objectInitStr.bsrcRect.left	= 0;
	objectInitStr.bsrcRect.bottom = 0;
	objectInitStr.bsrcRect.right	= 0;
	objectInitStr.bdestRect			= objectInitStr.bsrcRect;
	objectInitStr.bsurface			= -1;
	objectInitStr.callbackSlot		= -1;
	
	fgets(plineBuffer, lineLen, pcockpitDataFile);
   gDebugLineNum ++;
	plinePtr = plineBuffer;
	ptoken = FindToken(&plinePtr, pseparators);	

	while(strcmpi(ptoken, END_MARKER)){

		if(!strcmpi(ptoken, PROP_CYCLEBITS_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%x", &objectInitStr.cycleBits);
		}
		else if(!strcmpi(ptoken, PROP_DESTLOC_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &(objectInitStr.destRect.top), 
														&(objectInitStr.destRect.left), 
														&(objectInitStr.destRect.bottom), 
														&(objectInitStr.destRect.right));
	
		}
		else {
         CockpitMessage(ptoken, "Kneeboard", gDebugLineNum);
		}

		fgets(plineBuffer, lineLen, pcockpitDataFile);
      gDebugLineNum ++;
		plinePtr = plineBuffer;
		ptoken = FindToken(&plinePtr, pseparators);	
	}

	objectInitStr.scale			= mScale;
	objectInitStr.idNum			= idNum;
	objectInitStr.pOTWImage		= mpOTWImage;
	objectInitStr.pTemplate		= gpTemplateSurface;
	objectInitStr.pCPManager	= this;

#ifndef _CPMAN_USE_STL_CONTAINERS
	mpObjects[mObjectTally]		= new CPKneeView(&objectInitStr, mpKneeBoard);
	mObjectTally++;
#else
	CPKneeView *p = new CPKneeView(&objectInitStr, mpKneeBoard);
	ShiAssert(p);
	if(!p) return;

	mpObjects.push_back(p);
	mObjectTally++;
#endif
}

//====================================================//
// CockpitManager::CreateLight
//====================================================//

void CockpitManager::CreateLight(int idNum, FILE* pcockpitDataFile) {

	char				plineBuffer[MAX_LINE_BUFFER] = "";
	char*				plinePtr;
	char				*ptoken;
	char				pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
	const int			lineLen = MAX_LINE_BUFFER - 1;
	LightButtonInitStr	lightButtonInitStr={0};
	ObjectInitStr		objectInitStr;
	int					state = 0;
	char				ptransparencyStr[32] = "";

	lightButtonInitStr.cursorId	= -1; // just so that we pass something nice to the light class
	objectInitStr.cycleBits			= 0x0000;

	objectInitStr.bsrcRect.top		= 0;
	objectInitStr.bsrcRect.left	= 0;
	objectInitStr.bsrcRect.bottom = 0;
	objectInitStr.bsrcRect.right	= 0;
	objectInitStr.bdestRect			= objectInitStr.bsrcRect;
	objectInitStr.bsurface			= -1;
	lightButtonInitStr.psrcRect   = NULL;

	fgets(plineBuffer, lineLen, pcockpitDataFile);
   gDebugLineNum ++;
	plinePtr = plineBuffer;
	ptoken = FindToken(&plinePtr, pseparators);	
	lightButtonInitStr.states = 0;

	while(strcmpi(ptoken, END_MARKER)){

		if(!strcmpi(ptoken, PROP_STATES_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &lightButtonInitStr.states);
			#ifdef USE_SH_POOLS
			if(lightButtonInitStr.states > 0) {
				lightButtonInitStr.psrcRect = (RECT *)MemAllocPtr(gCockMemPool,sizeof(RECT)*lightButtonInitStr.states,FALSE);
			}
			#else
			lightButtonInitStr.psrcRect = new RECT[lightButtonInitStr.states];
			#endif
		}
		else if(!strcmpi(ptoken, PROP_CYCLEBITS_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%x", &objectInitStr.cycleBits);
		}
		else if(!strcmpi(ptoken, PROP_CURSORID_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &lightButtonInitStr.cursorId);
		}
		else if(!strcmpi(ptoken, PROP_INITSTATE_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &lightButtonInitStr.initialState);
		}
		else if(!strcmpi(ptoken, PROP_SRCLOC_STR)) {
			F4Assert(state < lightButtonInitStr.states);

			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &(lightButtonInitStr.psrcRect[state].top), 
													&(lightButtonInitStr.psrcRect[state].left), 
													&(lightButtonInitStr.psrcRect[state].bottom), 
													&(lightButtonInitStr.psrcRect[state].right));
			lightButtonInitStr.psrcRect[state].bottom++;
			lightButtonInitStr.psrcRect[state].right++;
			state++;
		}
		else if(!strcmpi(ptoken, PROP_DESTLOC_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%s %d %d %d %d", ptransparencyStr,
														&(objectInitStr.destRect.top), 
														&(objectInitStr.destRect.left), 
														&(objectInitStr.destRect.bottom), 
														&(objectInitStr.destRect.right));
	
			if(!strcmpi(ptransparencyStr, PROP_TRANSPARENT_STR)) {
				objectInitStr.transparencyType	= CPTRANSPARENT;
			}
			else if(!strcmpi(ptransparencyStr, PROP_OPAQUE_STR)) {
				objectInitStr.transparencyType	= CPOPAQUE;
			}
			else {
				ShiWarning("Bad Transparency Type"); //couldn't read in transparency type
			}
		}
		else if(!strcmpi(ptoken, PROP_CALLBACKSLOT_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &objectInitStr.callbackSlot);
		}
		else if(!strcmpi(ptoken, PROP_PERSISTANT_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &objectInitStr.persistant);
		}
		else {
         CockpitMessage(ptoken, "Light", gDebugLineNum);
		}

		fgets(plineBuffer, lineLen, pcockpitDataFile);
      gDebugLineNum ++;
		plinePtr = plineBuffer;
		ptoken = FindToken(&plinePtr, pseparators);	
	}


	objectInitStr.scale			= mScale;
	objectInitStr.idNum			= idNum;
	objectInitStr.pOTWImage		= mpOTWImage;
	objectInitStr.pTemplate		= gpTemplateSurface;
	objectInitStr.pCPManager	= this;

#ifndef _CPMAN_USE_STL_CONTAINERS
	mpObjects[mObjectTally]		= new CPLight(&objectInitStr, &lightButtonInitStr);
	mObjectTally++;
#else
	CPLight *p = new CPLight(&objectInitStr, &lightButtonInitStr);
	ShiAssert(p);
	if(!p) return;

	mpObjects.push_back(p);
	mObjectTally++;
#endif
}


//====================================================//
// CockpitManager::CreateButtonView
//====================================================//

void CockpitManager::CreateButtonView(int idNum, FILE* pcockpitDataFile) {

	char					plineBuffer[MAX_LINE_BUFFER] = "";
	char*					plinePtr;
	char					*ptoken;
	char					pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
	const int				lineLen = MAX_LINE_BUFFER - 1;
	char					ptransparencyStr[32] = "";
	ButtonViewInitStr		buttonViewInitStr={0};
	int						state = 0;

	buttonViewInitStr.objectId = idNum;
	buttonViewInitStr.pSrcRect = NULL;
	buttonViewInitStr.states = 0;

	fgets(plineBuffer, lineLen, pcockpitDataFile);
   gDebugLineNum ++;
	plinePtr = plineBuffer;
	ptoken = FindToken(&plinePtr, pseparators);	

	while(strcmpi(ptoken, END_MARKER)){

	
		if(!strcmpi(ptoken, PROP_SRCLOC_STR)) {
			F4Assert(state < buttonViewInitStr.states);

			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &(buttonViewInitStr.pSrcRect[state].top), 
													&(buttonViewInitStr.pSrcRect[state].left), 
													&(buttonViewInitStr.pSrcRect[state].bottom), 
													&(buttonViewInitStr.pSrcRect[state].right));
			buttonViewInitStr.pSrcRect[state].bottom++;
			buttonViewInitStr.pSrcRect[state].right++;
			state++;
		}

		else if(!strcmpi(ptoken, PROP_STATES_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &buttonViewInitStr.states);
			#ifdef USE_SH_POOLS
			if(buttonViewInitStr.states > 0) {
				buttonViewInitStr.pSrcRect = (RECT *)MemAllocPtr(gCockMemPool,sizeof(RECT)*buttonViewInitStr.states,FALSE);
			}
			#else
			buttonViewInitStr.pSrcRect = new RECT[buttonViewInitStr.states];
			#endif
		}
		else if(!strcmpi(ptoken, PROP_PARENTBUTTON_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &buttonViewInitStr.parentButton);
		}
		else if(!strcmpi(ptoken, PROP_DESTLOC_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%s %d %d %d %d", ptransparencyStr,
														&(buttonViewInitStr.destRect.top), 
														&(buttonViewInitStr.destRect.left), 
														&(buttonViewInitStr.destRect.bottom), 
														&(buttonViewInitStr.destRect.right));

			buttonViewInitStr.destRect.bottom++; 
			buttonViewInitStr.destRect.right++;
	
			if(!strcmpi(ptransparencyStr, PROP_TRANSPARENT_STR)) {
				buttonViewInitStr.transparencyType	= CPTRANSPARENT;
			}
			else if(!strcmpi(ptransparencyStr, PROP_OPAQUE_STR)) {
				buttonViewInitStr.transparencyType	= CPOPAQUE;
			}
			else {
				ShiWarning("Bad Transparency Type"); //couldn't read in transparency type
			}
		}
		else if(!strcmpi(ptoken, PROP_PERSISTANT_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &buttonViewInitStr.persistant);
		}
		else {
         CockpitMessage(ptoken, "ButtonView", gDebugLineNum);
		}

		fgets(plineBuffer, lineLen, pcockpitDataFile);
      gDebugLineNum ++;
		plinePtr = plineBuffer;
		ptoken = FindToken(&plinePtr, pseparators);	
	}

	buttonViewInitStr.scale			= mScale;
	buttonViewInitStr.objectId		= idNum;
	buttonViewInitStr.pOTWImage	= mpOTWImage;
	buttonViewInitStr.pTemplate	= gpTemplateSurface;

#ifndef _CPMAN_USE_STL_CONTAINERS
	mpButtonViews[mButtonViewTally]		= new CPButtonView(&buttonViewInitStr);
	mButtonViewTally++;
#else
	CPButtonView *p = new CPButtonView(&buttonViewInitStr);
	ShiAssert(p);
	if(!p) return;

	mpButtonViews.push_back(p);
	mButtonViewTally++;
#endif
}

//====================================================//
// CockpitManager::CreateButton
//====================================================//

void CockpitManager::CreateButton(int idNum, FILE* pcockpitDataFile) {

	char						plineBuffer[MAX_LINE_BUFFER] = "";
	char*						plinePtr;
	char						*ptoken;
	char						pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
	const int				lineLen = MAX_LINE_BUFFER - 1;
	ButtonObjectInitStr	buttonObjectInitStr;


	buttonObjectInitStr.objectId = idNum;

	buttonObjectInitStr.sound1 = -1;
	buttonObjectInitStr.sound2 = -1;

	fgets(plineBuffer, lineLen, pcockpitDataFile);
   gDebugLineNum ++;
	plinePtr = plineBuffer;
	ptoken = FindToken(&plinePtr, pseparators);	

	while(strcmpi(ptoken, END_MARKER)){

		if(!strcmpi(ptoken, PROP_STATES_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &buttonObjectInitStr.totalStates);
		}
		else if(!strcmpi(ptoken, PROP_DELAY_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &buttonObjectInitStr.delay);
		}
		else if(!strcmpi(ptoken, PROP_CURSORID_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &buttonObjectInitStr.cursorIndex);
		}
		else if(!strcmpi(ptoken, PROP_INITSTATE_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &buttonObjectInitStr.normalState);
		}
		else if(!strcmpi(ptoken, PROP_SOUND1_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &buttonObjectInitStr.sound1);
		}
		else if(!strcmpi(ptoken, PROP_SOUND2_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &buttonObjectInitStr.sound2);
		}
		else if(!strcmpi(ptoken, PROP_CALLBACKSLOT_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &buttonObjectInitStr.callbackSlot);
		}
		else if(!strcmpi(ptoken, PROP_NUMBUTTONVIEWS_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &buttonObjectInitStr.totalViews);
		}
		else {
         CockpitMessage(ptoken, "Button", gDebugLineNum);
		}

		fgets(plineBuffer, lineLen, pcockpitDataFile);
      gDebugLineNum ++;
		plinePtr = plineBuffer;
		ptoken = FindToken(&plinePtr, pseparators);	
	}

#ifndef _CPMAN_USE_STL_CONTAINERS
	mpButtonObjects[mButtonTally]		= new CPButtonObject(&buttonObjectInitStr);
	mButtonTally++;
#else
	CPButtonObject *p = new CPButtonObject(&buttonObjectInitStr);
	ShiAssert(p);
	if(!p) return;

	mpButtonObjects.push_back(p);
	mButtonTally++;
#endif
}


//====================================================//
// CockpitManager::CreateAdi
//====================================================//

void CockpitManager::CreateAdi(int idNum, FILE* pcockpitDataFile) {

	char					plineBuffer[MAX_LINE_BUFFER] = "";
	char*					plinePtr;
	char					*ptoken;
	char					pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
	const int				lineLen = MAX_LINE_BUFFER - 1;
	ObjectInitStr			objectInitStr;
	char					ptransparencyStr[32] = "";
	ADIInitStr				adiInitStr={0};

	adiInitStr.doBackRect = FALSE;
	adiInitStr.backSrc.top = 0;
	adiInitStr.backSrc.left = 0;
	adiInitStr.backSrc.bottom = 0;
	adiInitStr.backSrc.right = 0;
	adiInitStr.color0 = 0xFF20A2C8;
	adiInitStr.color1 = 0xff808080;
	adiInitStr.color2 = 0xffffffff;
	adiInitStr.color3 = 0xFF6CF3F3;
	adiInitStr.color4 = 0xFF6CF3F3;

	fgets(plineBuffer, lineLen, pcockpitDataFile);
   gDebugLineNum ++;
	plinePtr = plineBuffer;
	ptoken = FindToken(&plinePtr, pseparators);	

	while(strcmpi(ptoken, END_MARKER)){

		if(!strcmpi(ptoken, PROP_SRCLOC_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &(adiInitStr.srcRect.top), 
													&(adiInitStr.srcRect.left), 
													&(adiInitStr.srcRect.bottom), 
													&(adiInitStr.srcRect.right));
		}
		else if(!strcmpi(ptoken, PROP_ILSLIMITS_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &(adiInitStr.ilsLimits.top), 
													&(adiInitStr.ilsLimits.left), 
													&(adiInitStr.ilsLimits.bottom), 
													&(adiInitStr.ilsLimits.right));

		}
		else if(!strcmpi(ptoken, PROP_CYCLEBITS_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%x", &objectInitStr.cycleBits);
		}
		else if(!strcmpi(ptoken, PROP_DESTLOC_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%s %d %d %d %d", ptransparencyStr,
														&(objectInitStr.destRect.top), 
														&(objectInitStr.destRect.left), 
														&(objectInitStr.destRect.bottom), 
														&(objectInitStr.destRect.right));
	
			if(!strcmpi(ptransparencyStr, PROP_TRANSPARENT_STR)) {
				objectInitStr.transparencyType	= CPTRANSPARENT;
			}
			else if(!strcmpi(ptransparencyStr, PROP_OPAQUE_STR)) {
				objectInitStr.transparencyType	= CPOPAQUE;
			}
			else {
				ShiWarning("Bad Transparency Type"); //couldn't read in transparency type
			}
		}
		else if(!strcmpi(ptoken, PROP_CALLBACKSLOT_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &objectInitStr.callbackSlot);
		}
		else if(!strcmpi(ptoken, PROP_PERSISTANT_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &objectInitStr.persistant);

			if(!objectInitStr.persistant) {
				objectInitStr.bsrcRect.top		= 0;
				objectInitStr.bsrcRect.left	= 0;
				objectInitStr.bsrcRect.bottom = 0;
				objectInitStr.bsrcRect.right	= 0;

				objectInitStr.bdestRect			= objectInitStr.bsrcRect;
				objectInitStr.bsurface			= -1;
			}
		}
		else if(!strcmpi(ptoken, PROP_BSRCLOC_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &(objectInitStr.bsrcRect.top), 
													&(objectInitStr.bsrcRect.left), 
													&(objectInitStr.bsrcRect.bottom), 
													&(objectInitStr.bsrcRect.right));
		}
		else if(!strcmpi(ptoken, PROP_BDESTLOC_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &(objectInitStr.bdestRect.top), 
													&(objectInitStr.bdestRect.left), 
													&(objectInitStr.bdestRect.bottom), 
													&(objectInitStr.bdestRect.right));
		}
		else if(!strcmpi(ptoken, PROP_BSURFACE_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d", &(objectInitStr.bsurface));
		}

		else if(!strcmpi(ptoken, PROP_BLITBACKGROUND_STR)) {
			adiInitStr.doBackRect = TRUE;
			LoadBuffer(pcockpitDataFile);
		}
		else if(!strcmpi(ptoken, PROP_BACKDEST_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
	
			sscanf(ptoken, "%d %d %d %d", &(adiInitStr.backDest.top), 
													&(adiInitStr.backDest.left), 
													&(adiInitStr.backDest.bottom), 
													&(adiInitStr.backDest.right));
		}
		else if(!strcmpi(ptoken, PROP_BACKSRC_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
	
			sscanf(ptoken, "%d %d %d %d", &(adiInitStr.backSrc.top), 
													&(adiInitStr.backSrc.left), 
													&(adiInitStr.backSrc.bottom), 
													&(adiInitStr.backSrc.right));
		}
		else if(!strcmpi(ptoken, PROP_COLOR0_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%lx", &adiInitStr.color0);
		}
		else if(!strcmpi(ptoken, PROP_COLOR1_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%lx", &adiInitStr.color1);
		}
		else if(!strcmpi(ptoken, PROP_COLOR2_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%lx", &adiInitStr.color2);
		}
		else if(!strcmpi(ptoken, PROP_COLOR3_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%lx", &adiInitStr.color3);
		}
		else if(!strcmpi(ptoken, PROP_COLOR4_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%lx", &adiInitStr.color4);
		}
		else {
			CockpitMessage(ptoken, "ADI", gDebugLineNum);
		}

		fgets(plineBuffer, lineLen, pcockpitDataFile);
      gDebugLineNum ++;
		plinePtr = plineBuffer;
		ptoken = FindToken(&plinePtr, pseparators);	
	}


	if(adiInitStr.doBackRect) {
		RECT destRect;
		
		
		destRect.top		= 0;
		destRect.left		= 0;
		destRect.bottom	= adiInitStr.backSrc.bottom - adiInitStr.backSrc.top + 1;
		destRect.right		= adiInitStr.backSrc.right - adiInitStr.backSrc.left + 1;

		#ifdef USE_SH_POOLS
		adiInitStr.pBackground = (BYTE *)MemAllocPtr(gCockMemPool,sizeof(BYTE)*destRect.bottom*destRect.right,FALSE);
		#else
		adiInitStr.pBackground = new BYTE[destRect.bottom * destRect.right];
		#endif

		adiInitStr.backSrc.right	+= 1;
		adiInitStr.backSrc.bottom += 1;

		ImageCopy(mpLoadBuffer, adiInitStr.pBackground, mLoadBufferWidth, &adiInitStr.backSrc);

		adiInitStr.backSrc.top		= destRect.top;
		adiInitStr.backSrc.left		= destRect.left;
		adiInitStr.backSrc.bottom	= destRect.bottom;
		adiInitStr.backSrc.right	= destRect.right;
	}

	objectInitStr.scale			= mScale;
	objectInitStr.idNum			= idNum;
	objectInitStr.pOTWImage		= mpOTWImage;
	objectInitStr.pTemplate		= gpTemplateSurface;
	objectInitStr.pCPManager	= this;

#ifndef _CPMAN_USE_STL_CONTAINERS
	mpObjects[mObjectTally]		= new CPAdi(&objectInitStr, &adiInitStr);
	mObjectTally++;
#else
	CPAdi *p = new CPAdi(&objectInitStr, &adiInitStr);
	ShiAssert(p);
	if(!p) return;

	mpObjects.push_back(p);
	mObjectTally++;
#endif
}

//====================================================//
// CockpitManager::CreateHsiView
//====================================================//

void CockpitManager::CreateHsiView(int idNum, FILE* pcockpitDataFile) {

	char						plineBuffer[MAX_LINE_BUFFER] = "";
	char*						plinePtr;
	char						*ptoken;
	char						pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
	const int				lineLen = MAX_LINE_BUFFER - 1;
	ObjectInitStr			objectInitStr;
	char						ptransparencyStr[32] = "";
	int						destcount = 0;
	int						srccount = 0;
	RECT						destination;
	RECT						source;
	int						transparencyType=0;
	HsiInitStr				hsiInitStr;


	//MI
	if(!g_bRealisticAvionics)
	{
		hsiInitStr.colors[HSI_COLOR_ARROWS] = 0xff1e6cff;
		hsiInitStr.colors[HSI_COLOR_ARROWGHOST] = 0xff1e6cff;
		hsiInitStr.colors[HSI_COLOR_COURSEWARN] = 0xff0000ff;
		hsiInitStr.colors[HSI_COLOR_HEADINGMARK] = 0xff469646;
		hsiInitStr.colors[HSI_COLOR_STATIONBEARING] = 0xff323cff;
		hsiInitStr.colors[HSI_COLOR_COURSE] = 0xff73dcf0;
		hsiInitStr.colors[HSI_COLOR_AIRCRAFT] = 0xffe0e0e0;
		hsiInitStr.colors[HSI_COLOR_CIRCLES] = 0xffe0e0e0;
		hsiInitStr.colors[HSI_COLOR_DEVBAR] = 0xff73dcf0;
		hsiInitStr.colors[HSI_COLOR_ILSDEVWARN] = 0xff0000fd;
	}
	else
	{
		hsiInitStr.colors[HSI_COLOR_ARROWS] = 0xff0000ff;
		hsiInitStr.colors[HSI_COLOR_ARROWGHOST] = 0xff0000ff;
		hsiInitStr.colors[HSI_COLOR_COURSEWARN] = 0xff0000ff;
		hsiInitStr.colors[HSI_COLOR_HEADINGMARK] = 0xffffffff;
		hsiInitStr.colors[HSI_COLOR_STATIONBEARING] = 0xffffffff;
		hsiInitStr.colors[HSI_COLOR_COURSE] = 0xffffffff;
		hsiInitStr.colors[HSI_COLOR_AIRCRAFT] = 0xffffffff;
		hsiInitStr.colors[HSI_COLOR_CIRCLES] = 0xffe0e0e0;
		hsiInitStr.colors[HSI_COLOR_DEVBAR] = 0xffffffff;
		hsiInitStr.colors[HSI_COLOR_ILSDEVWARN] = 0xff0000fd;
	}

	fgets(plineBuffer, lineLen, pcockpitDataFile);
   gDebugLineNum ++;
	plinePtr = plineBuffer;
	ptoken = FindToken(&plinePtr, pseparators);	

	while(strcmpi(ptoken, END_MARKER)){

		if(!strcmpi(ptoken, PROP_SRCLOC_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &(source.top), 
														&(source.left), 
														&(source.bottom), 
														&(source.right));
			if(srccount == 0) {
				hsiInitStr.compassSrc = source;
			}
			else if(srccount == 1) {
				hsiInitStr.devSrc = source; // course deviation ring
			}
			else {
				ShiWarning("Bad HSI Source Count"); //couldn't read in transparency type
			}

			srccount++;
		}
		else if(!strcmpi(ptoken, PROP_CYCLEBITS_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%x", &objectInitStr.cycleBits);
		}
		else if(!strcmpi(ptoken, PROP_DESTLOC_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%s %d %d %d %d", ptransparencyStr,
														&(destination.top), 
														&(destination.left), 
														&(destination.bottom), 
														&(destination.right));
	
			if(!strcmpi(ptransparencyStr, PROP_TRANSPARENT_STR)) {
				transparencyType	= CPTRANSPARENT;
			}
			else if(!strcmpi(ptransparencyStr, PROP_OPAQUE_STR)) {
				objectInitStr.transparencyType	= CPOPAQUE;
			}
			else {
				ShiWarning("Bad Transparency Type"); //couldn't read in transparency type
			}

			if(destcount == 0) {
				objectInitStr.destRect = destination;
				objectInitStr.transparencyType = transparencyType;
			}
			else if(destcount == 1) {
				hsiInitStr.compassDest = destination;;
				hsiInitStr.compassTransparencyType = transparencyType;
			}
			else if(destcount == 2) {
				hsiInitStr.devDest = destination;;
			}
			else {
				ShiWarning("Bad dest count"); 
			}

			destcount++;
		}
		else if(!strcmpi(ptoken, PROP_CALLBACKSLOT_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &objectInitStr.callbackSlot);
		}
		else if(!strcmpi(ptoken, PROP_PERSISTANT_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &objectInitStr.persistant);

			if(!objectInitStr.persistant) {
				objectInitStr.bsrcRect.top		= 0;
				objectInitStr.bsrcRect.left	= 0;
				objectInitStr.bsrcRect.bottom = 0;
				objectInitStr.bsrcRect.right	= 0;

				objectInitStr.bdestRect			= objectInitStr.bsrcRect;
				objectInitStr.bsurface			= -1;
			}
		}
		else if(!strcmpi(ptoken, PROP_WARNFLAG_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &(hsiInitStr.warnFlag.top), 
													&(hsiInitStr.warnFlag.left), 
													&(hsiInitStr.warnFlag.bottom), 
													&(hsiInitStr.warnFlag.right));
		}
		else if(!strcmpi(ptoken, PROP_BSRCLOC_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &(objectInitStr.bsrcRect.top), 
													&(objectInitStr.bsrcRect.left), 
													&(objectInitStr.bsrcRect.bottom), 
													&(objectInitStr.bsrcRect.right));
		}
		else if(!strcmpi(ptoken, PROP_BDESTLOC_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &(objectInitStr.bdestRect.top), 
													&(objectInitStr.bdestRect.left), 
													&(objectInitStr.bdestRect.bottom), 
													&(objectInitStr.bdestRect.right));
		}
		else if(!strcmpi(ptoken, PROP_BSURFACE_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d", &(objectInitStr.bsurface));
		}
		else if(!strcmpi(ptoken, PROP_COLOR0_STR)) {
		    ptoken = FindToken(&plinePtr, pseparators);	
		    sscanf(ptoken, "%lx", &hsiInitStr.colors[0]);
		}
		else if(!strcmpi(ptoken, PROP_COLOR1_STR)) {
		    ptoken = FindToken(&plinePtr, pseparators);	
		    sscanf(ptoken, "%lx", &hsiInitStr.colors[1]);
		}
		else if(!strcmpi(ptoken, PROP_COLOR2_STR)) {
		    ptoken = FindToken(&plinePtr, pseparators);	
		    sscanf(ptoken, "%lx", &hsiInitStr.colors[2]);
		}
		else if(!strcmpi(ptoken, PROP_COLOR3_STR)) {
		    ptoken = FindToken(&plinePtr, pseparators);	
		    sscanf(ptoken, "%lx", &hsiInitStr.colors[3]);
		}
		else if(!strcmpi(ptoken, PROP_COLOR4_STR)) {
		    ptoken = FindToken(&plinePtr, pseparators);	
		    sscanf(ptoken, "%lx", &hsiInitStr.colors[4]);
		}
		else if(!strcmpi(ptoken, PROP_COLOR5_STR)) {
		    ptoken = FindToken(&plinePtr, pseparators);	
		    sscanf(ptoken, "%lx", &hsiInitStr.colors[5]);
		}
		else if(!strcmpi(ptoken, PROP_COLOR6_STR)) {
		    ptoken = FindToken(&plinePtr, pseparators);	
		    sscanf(ptoken, "%lx", &hsiInitStr.colors[6]);
		}
		else if(!strcmpi(ptoken, PROP_COLOR7_STR)) {
		    ptoken = FindToken(&plinePtr, pseparators);	
		    sscanf(ptoken, "%lx", &hsiInitStr.colors[7]);
		}
		else if(!strcmpi(ptoken, PROP_COLOR8_STR)) {
		    ptoken = FindToken(&plinePtr, pseparators);	
		    sscanf(ptoken, "%lx", &hsiInitStr.colors[8]);
		}
		else if(!strcmpi(ptoken, PROP_COLOR9_STR)) {
		    ptoken = FindToken(&plinePtr, pseparators);	
		    sscanf(ptoken, "%lx", &hsiInitStr.colors[9]);
		}
		else {
         CockpitMessage(ptoken, "HSI", gDebugLineNum);
		}

		fgets(plineBuffer, lineLen, pcockpitDataFile);
      gDebugLineNum ++;
		plinePtr = plineBuffer;
		ptoken = FindToken(&plinePtr, pseparators);	
	}

	objectInitStr.scale			= mScale;
	objectInitStr.idNum			= idNum;
	objectInitStr.pOTWImage		= mpOTWImage;
	objectInitStr.pTemplate		= gpTemplateSurface;

	objectInitStr.pCPManager	= this;

	hsiInitStr.pHsi				= mpHsi;

#ifndef _CPMAN_USE_STL_CONTAINERS
	mpObjects[mObjectTally]		= new CPHsiView(&objectInitStr, &hsiInitStr);
	mObjectTally++;
#else
	CPHsiView *p = new CPHsiView(&objectInitStr, &hsiInitStr);
	ShiAssert(p);
	if(!p) return;

	mpObjects.push_back(p);
	mObjectTally++;
#endif
}

//====================================================//
// CockpitManager::CreateMachAsi
//====================================================//

void CockpitManager::CreateMachAsi(int idNum, FILE* pcockpitDataFile) {

	char						plineBuffer[MAX_LINE_BUFFER] = "";
	char*						plinePtr;
	char						*ptoken;
	char						pseparators[] = {0x20, 0x2c, 0x3d, 0x3b, 0x0d, 0x0a, 0x09, 0x00};
	const int				lineLen = MAX_LINE_BUFFER - 1;
	ObjectInitStr			objectInitStr;
	MachAsiInitStr			machAsiInitStr;
	char						ptransparencyStr[32] = "";

	machAsiInitStr.color0 =  0xFF181842;
	machAsiInitStr.color1 =  0xFF0C0C7A;
	machAsiInitStr.end_angle = 1.0F;
	machAsiInitStr.end_radius = 0.21F;

	fgets(plineBuffer, lineLen, pcockpitDataFile);
   gDebugLineNum ++;
	plinePtr = plineBuffer;
	ptoken = FindToken(&plinePtr, pseparators);	

	while(strcmpi(ptoken, END_MARKER)){


		if(!strcmpi(ptoken, PROP_CYCLEBITS_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%x", &objectInitStr.cycleBits);
		}
		else if(!strcmpi(ptoken, PROP_DESTLOC_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%s %d %d %d %d", ptransparencyStr,
														&(objectInitStr.destRect.top), 
														&(objectInitStr.destRect.left), 
														&(objectInitStr.destRect.bottom), 
														&(objectInitStr.destRect.right));
	
			if(!strcmpi(ptransparencyStr, PROP_TRANSPARENT_STR)) {
				objectInitStr.transparencyType	= CPTRANSPARENT;
			}
			else if(!strcmpi(ptransparencyStr, PROP_OPAQUE_STR)) {
				objectInitStr.transparencyType	= CPOPAQUE;
			}
			else {
				ShiWarning("Bad Transparency Type"); //couldn't read in transparency type
			}
		}
		else if(!strcmpi(ptoken, PROP_COLOR0_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%lx", &machAsiInitStr.color0);
		}
		else if(!strcmpi(ptoken, PROP_COLOR1_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%lx", &machAsiInitStr.color1);
		}
		else if(!strcmpi(ptoken, PROP_ENDANGLE)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%f", &machAsiInitStr.end_angle);
		}
		else if(!strcmpi(ptoken, PROP_ENDLENGTH)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%f", &machAsiInitStr.end_radius);
		}
		else if(!strcmpi(ptoken, PROP_CALLBACKSLOT_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &objectInitStr.callbackSlot);
		}
		else if(!strcmpi(ptoken, PROP_MINVAL_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%f", &machAsiInitStr.min_dial_value);
		}
		else if(!strcmpi(ptoken, PROP_MAXVAL_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%f", &machAsiInitStr.max_dial_value);
		}
		else if(!strcmpi(ptoken, PROP_STARTANGLE_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%f", &machAsiInitStr.dial_start_angle);
		}
		else if(!strcmpi(ptoken, PROP_ARCLENGTH_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%f", &machAsiInitStr.dial_arc_length);
		}
		else if(!strcmpi(ptoken, PROP_NEEDLERADIUS_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);
			sscanf(ptoken, "%d", &machAsiInitStr.needle_radius);
		}
		else if(!strcmpi(ptoken, PROP_PERSISTANT_STR)) {
			ptoken = FindToken(&plinePtr, pseparators);	
			sscanf(ptoken, "%d", &objectInitStr.persistant);

			if(!objectInitStr.persistant) {
				objectInitStr.bsrcRect.top		= 0;
				objectInitStr.bsrcRect.left	= 0;
				objectInitStr.bsrcRect.bottom = 0;
				objectInitStr.bsrcRect.right	= 0;

				objectInitStr.bdestRect			= objectInitStr.bsrcRect;
				objectInitStr.bsurface			= -1;
			}
		}
		else if(!strcmpi(ptoken, PROP_BSRCLOC_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &(objectInitStr.bsrcRect.top), 
													&(objectInitStr.bsrcRect.left), 
													&(objectInitStr.bsrcRect.bottom), 
													&(objectInitStr.bsrcRect.right));
		}
		else if(!strcmpi(ptoken, PROP_BDESTLOC_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d %d %d %d", &(objectInitStr.bdestRect.top), 
													&(objectInitStr.bdestRect.left), 
													&(objectInitStr.bdestRect.bottom), 
													&(objectInitStr.bdestRect.right));
		}
		else if(!strcmpi(ptoken, PROP_BSURFACE_STR)) {
			ptoken = FindToken(&plinePtr, "=;\n");	
			sscanf(ptoken, "%d", &(objectInitStr.bsurface));
		}
		else {
         CockpitMessage(ptoken, "MachAsi", gDebugLineNum);
		}

		fgets(plineBuffer, lineLen, pcockpitDataFile);
      gDebugLineNum ++;
		plinePtr = plineBuffer;
		ptoken = FindToken(&plinePtr, pseparators);	
	}

	objectInitStr.scale			= mScale;
	objectInitStr.idNum			= idNum;
	objectInitStr.pOTWImage		= mpOTWImage;
	objectInitStr.pTemplate		= gpTemplateSurface;
	objectInitStr.pCPManager	= this;

#ifndef _CPMAN_USE_STL_CONTAINERS
	mpObjects[mObjectTally]		= new CPMachAsi(&objectInitStr, &machAsiInitStr);
	mObjectTally++;
#else
	CPMachAsi *p = new CPMachAsi(&objectInitStr, &machAsiInitStr);
	ShiAssert(p);
	if(!p) return;

	mpObjects.push_back(p);
	mObjectTally++;
#endif
}

//====================================================//
// CockpitManager::Exec
//====================================================//

void CockpitManager::Exec() {

	mViewChanging = FALSE;

	if(!mpActivePanel) {
		return;
	}

	if (!mpOwnship) {
		return;
	}

	if(mIsInitialized == FALSE) {
		mpActivePanel->Exec(mpOwnship, CYCLE_ALL); //RUN ALL
		mIsInitialized = TRUE;
		mIsNextInitialized = TRUE;
	}
	else {
		
		if(mCycleBit == END_CYCLE) {
			mCycleBit = BEGIN_CYCLE;
		}
		else {
			mCycleBit = mCycleBit << 1;
		}

		mpActivePanel->Exec(mpOwnship, mCycleBit);
	}
}



//====================================================//
// CockpitManager::GeometryDraw
//====================================================//

void CockpitManager::GeometryDraw(void)
{
	static const int dofmap[] = { 	
	    COMP_LT_STAB, 	
		COMP_RT_STAB,
		COMP_LT_FLAP,
		COMP_RT_FLAP,
		COMP_RUDDER,
		COMP_LT_LEF,
		COMP_RT_LEF,
		COMP_LT_AIR_BRAKE_TOP,
		COMP_LT_AIR_BRAKE_BOT,
		COMP_RT_AIR_BRAKE_TOP,
		COMP_RT_AIR_BRAKE_BOT,
		COMP_CANOPY_DOF,
	};
	static const int dofmap_size = sizeof(dofmap) / sizeof(dofmap[0]);

	static const int switchmap[] = {
		COMP_WING_VAPOR,
		COMP_TAIL_STROBE,
		COMP_NAV_LIGHTS,
	};
	static const int switchmap_size = sizeof(switchmap) / sizeof(switchmap[0]);

	Tpoint			tempLight, worldLight;
	BOOL				drawWing = TRUE;
	BOOL				drawReflection = TRUE;
	BOOL				drawOrdinance = TRUE;
	DrawableBSP*	child;
	int				stationNum;


	if(mpActivePanel && mpGeometry) {
		
		if ( !mpActivePanel->DoGeometry() || PlayerOptions.ObjectDetailLevel() < 1.5F) {
			drawOrdinance = FALSE;
		}

		if ( mpActivePanel->DoGeometry() && PlayerOptions.ObjectDetailLevel() >= 1.0F) {
 			mpGeometry->SetSwitchMask( 7, 1);
		}
		else {
  			mpGeometry->SetSwitchMask( 7, 0);
			drawWing = FALSE;
		}

		if ( PlayerOptions.SimVisualCueMode == VCReflection || PlayerOptions.SimVisualCueMode == VCBoth) {
  			mpGeometry->SetSwitchMask( 3, 1);
		}
		else {
  			mpGeometry->SetSwitchMask( 3, 0);
			drawReflection = FALSE;
		}

		if(drawWing || drawReflection) {

			// Force object texture on for this part regardless of player settings
			BOOL oldState = OTWDriver.renderer->GetObjectTextureState();
			OTWDriver.renderer->SetObjectTextureState( TRUE );

			// Setup the local lighting and transform environment (body relative)
			OTWDriver.renderer->GetLightDirection( &worldLight );
			MatrixMultTranspose( &(OTWDriver.ownshipRot), &worldLight, &tempLight ); 
			OTWDriver.renderer->SetLightDirection( &tempLight );
			OTWDriver.renderer->SetCamera( &Origin, &(OTWDriver.headMatrix) );

			SMSClass		*sms = SimDriver.playerEntity->Sms;

			// Attach the appropriate ordinance
			if(sms && drawOrdinance == TRUE) {
				for (stationNum=1; stationNum < sms->NumHardpoints(); stationNum++)
				{
					if (sms->hardPoint[stationNum]->GetRack()) {
						child = sms->hardPoint[stationNum]->GetRack();
						mpGeometry->AttachChild( child, stationNum-1 );
					} 
					else if (sms->hardPoint[stationNum]->weaponPointer && sms->hardPoint[stationNum]->weaponPointer->drawPointer) {
						child = (DrawableBSP*)(sms->hardPoint[stationNum]->weaponPointer->drawPointer);
						mpGeometry->AttachChild( child, stationNum-1 );
					}
				}
			}

			// Draw the cockpit object
			mpGeometry->Draw( OTWDriver.renderer );

			// Put the light back into world space
			OTWDriver.renderer->SetLightDirection( &worldLight );

			// Restore camera
			OTWDriver.renderer->SetCamera( &(OTWDriver.cameraPos), &(OTWDriver.cameraRot) );

			// Restore the texture settings
			OTWDriver.renderer->SetObjectTextureState( oldState );

			// Detach the appropriate ordinance
			if(sms && drawOrdinance == TRUE) {
				for (stationNum=1; stationNum < sms->NumHardpoints(); stationNum++)
				{
					if (sms->hardPoint[stationNum]->GetRack()) {
						child = sms->hardPoint[stationNum]->GetRack();
						mpGeometry->DetachChild( child, stationNum-1 );
					}
					else if (sms->hardPoint[stationNum]->weaponPointer && sms->hardPoint[stationNum]->weaponPointer->drawPointer) {
						child = (DrawableBSP*)(sms->hardPoint[stationNum]->weaponPointer->drawPointer);
						mpGeometry->DetachChild( child, stationNum-1 );
					}
				}
			}
			//MI
			for(int i = 0; i < dofmap_size; i++)
			{
				if(dofmap[i] == COMP_LT_STAB || dofmap[i] == COMP_RT_STAB)
					mpGeometry->SetDOFangle(dofmap[i], -SimDriver.playerEntity->GetDOFValue(dofmap[i]));
				else
					mpGeometry->SetDOFangle(dofmap[i], SimDriver.playerEntity->GetDOFValue(dofmap[i]));
			}

			for(i = 0; i < switchmap_size; i++)
				mpGeometry->SetSwitchMask(switchmap[i], SimDriver.playerEntity->GetSwitch(switchmap[i]));
		}
	}
}


//====================================================//
// CockpitManager::DisplayBlit
//====================================================//

void CockpitManager::DisplayBlit(){
#if DO_HIRESCOCK_HACK
	if(gDoCockpitHack) {
		return;
	}

	if(mIsInitialized && mpActivePanel) {
		mpActivePanel->DisplayBlit();
	}
#else
	if(mIsInitialized && mpActivePanel) {
		mpActivePanel->DisplayBlit();
	}
#endif
}

// OW
void CockpitManager::DisplayBlit3D(){
#if DO_HIRESCOCK_HACK
	if(gDoCockpitHack) {
		return;
	}

	if(mIsInitialized && mpActivePanel) {
		mpActivePanel->DisplayBlit3D();
	}
#else
	if(mIsInitialized && mpActivePanel) {
		mpActivePanel->DisplayBlit3D();
	}
#endif
}

//====================================================//
// CockpitManager::DisplayDraw
//====================================================//

void CockpitManager::DisplayDraw(){

#if DO_HIRESCOCK_HACK

	if(gDoCockpitHack) {
		return;
	}

	if(mIsInitialized && mpActivePanel) {
		OTWDriver.renderer->StartFrame();
		OTWDriver.renderer->SetViewport(-1.0F, 1.0F, 1.0F, -1.0F);
		OTWDriver.renderer->CenterOriginInViewport();
		OTWDriver.renderer->ZeroRotationAboutOrigin();

		mpActivePanel->DisplayDraw();
		OTWDriver.renderer->FinishFrame();
	}
#else
	if(mIsInitialized && mpActivePanel) {
		OTWDriver.renderer->StartFrame();
		OTWDriver.renderer->SetViewport(-1.0F, 1.0F, 1.0F, -1.0F);
		OTWDriver.renderer->CenterOriginInViewport();
		OTWDriver.renderer->ZeroRotationAboutOrigin();

		mpActivePanel->DisplayDraw();
		OTWDriver.renderer->FinishFrame();
	}
#endif
}

CPButtonObject* CockpitManager::GetButtonPointer(int buttonId) {

	BOOL					found			= FALSE;
	int					i				= 0;	
	CPButtonObject*	preturnValue = NULL;

	while(!found && i < mNumButtons) {
		if(mpButtonObjects[i]->GetId() == buttonId) {
			found = TRUE;
			preturnValue = mpButtonObjects[i];
		}
		else {
			i++;
		}
	}

	F4Assert(found);
	return(preturnValue);
}


//====================================================//
// CockpitManager::Dispatch
//====================================================//

void CockpitManager::Dispatch(int buttonId, int mouseSide) {
	
	BOOL	found	= FALSE;
	int	i		= 0;	
	int	event;

	while(!found && i< mButtonTally) {
		if(mpButtonObjects[i]->GetId() == buttonId) {
			found = TRUE;

			if(mouseSide == STARTDAT_MOUSE_LEFT) {
				event = CP_MOUSE_BUTTON0;
			}
			else {
				event = CP_MOUSE_BUTTON1;
			}

			mpButtonObjects[i]->HandleEvent(event);
		}
		else {
			i++;
		}
	}

//	F4Assert(found);
}



int CockpitManager::POVDispatch(int direction, int curXPos, int curYPos)
{
	int	cursorIndex = -1;

	if(SimDriver.InSim()) {
		if(mpActivePanel) {
			if(mpActivePanel->POVDispatch(direction))
			{
				if (mpNextActivePanel)
				{
					mpNextActivePanel->Dispatch(&cursorIndex, CP_MOUSE_MOVE, curXPos, curYPos);
				}
				else
				{
					MonoPrint ("Cannot Change to Next Active Panel\n");
				}
				F4SoundFXSetDist(SFX_CP_CHNGVIEW, TRUE, 0.0f, 1.0f);
			}
		}
		else {
			cursorIndex = -1;
		}
	}
	else {
		cursorIndex = -1;
	}

	return cursorIndex;
}


int CockpitManager::Dispatch(int event, int xpos, int ypos) {

	int		cursorIndex;

	if(SimDriver.InSim()) {
		if(mpActivePanel) {
			if(mpActivePanel->Dispatch(&cursorIndex, event, xpos, ypos))
			{
				if (mpNextActivePanel)
				{
					mpNextActivePanel->Dispatch(&cursorIndex, CP_MOUSE_MOVE, xpos, ypos);
				}
				else
				{
					MonoPrint ("Cannot Change to Next Active Panel\n");
				}
				F4SoundFXSetDist(SFX_CP_CHNGVIEW, TRUE, 0.0f, 1.0f);
			}
		}
		else {
			cursorIndex = -1;
		}
	}
	else {
		cursorIndex = -1;
	}

	return cursorIndex;
}


//====================================================//
// CockpitManager::SetOwnship
//====================================================//

void CockpitManager::SetOwnship(SimBaseClass* paircraftClass) {
	
	mpOwnship				= paircraftClass;

	if(mpIcp) {
		mpIcp->SetOwnship();		// necessary for now, may not need when done
	}
}

//====================================================//
// CockpitManager::GetCockpitMaskTop
//====================================================//

float CockpitManager::GetCockpitMaskTop() {
	
	float returnValue;

#if DO_HIRESCOCK_HACK
	if(mpActivePanel && !gDoCockpitHack){
		returnValue = mpActivePanel->mMaskTop;
	}
	else {
		returnValue = -1.0F;
	}
#else
	if(mpActivePanel){
		returnValue = mpActivePanel->mMaskTop;
	}
	else {
		returnValue = -1.0F;
	}
#endif
	return(returnValue);
}


//====================================================//
// CockpitManager::SetActivePanel
//====================================================//

void CockpitManager::SetActivePanel(int panelId) {

	int i			= 0;
	int found	= FALSE;

	mCycleBit = BEGIN_CYCLE;

	if(panelId == PANELS_INACTIVE) {
		
		F4EnterCriticalSection(mpCockpitCritSec);

		if(mpActivePanel) {
			mpActivePanel->DiscardLitSurfaces();
		}
		mpActivePanel		= NULL;
		mpNextActivePanel	= NULL;
		
		F4LeaveCriticalSection(mpCockpitCritSec);

		mIsNextInitialized = FALSE;
	}
	else if(mpActivePanel == NULL || mpActivePanel->mIdNum != panelId) {

		// loop thru all the panels
		while((!found) && (i < mPanelTally)) {
			// if we find the panel with our id, make it active
			if(mpPanels[i]->mIdNum == panelId) {

				F4EnterCriticalSection(mpCockpitCritSec);
				mpNextActivePanel		= mpPanels[i];
				F4LeaveCriticalSection(mpCockpitCritSec);
				mIsNextInitialized	= FALSE;
				found						= TRUE;
			}
			else {
				i++;
			}
		}
		F4Assert(found);
	}
//	MonoPrint("panelID == %d\n", panelId);
}

//====================================================//
// CockpitManager::ShowRwr
//====================================================//
BOOL CockpitManager::ShowRwr(void) {
	
	if(mpViewBounds[BOUNDS_RWR]) {
		return(TRUE);
	}
	else if(mpActivePanel && mpActivePanel->mpViewBounds[BOUNDS_RWR]) {
		return(TRUE);
	}

	return (FALSE);
}


//====================================================//
// CockpitManager::ShowHud
//====================================================//
BOOL CockpitManager::ShowHud(void) {
	
	if(mpViewBounds[BOUNDS_HUD]) {
		return(TRUE);
	}
	else if(mpActivePanel && mpActivePanel->mpViewBounds[BOUNDS_HUD]) {
		return(TRUE);
	}

	return (FALSE);
}

//====================================================//
// CockpitManager::ShowMfd
//====================================================//
BOOL CockpitManager::ShowMfd(void) {
	
	if(mpViewBounds[BOUNDS_MFDLEFT] && mpViewBounds[BOUNDS_MFDRIGHT]) {
		return(TRUE);
	}
	else if(mpActivePanel && mpActivePanel->mpViewBounds[BOUNDS_MFDLEFT] && mpActivePanel->mpViewBounds[BOUNDS_MFDRIGHT]) {
		return(TRUE);
	}

	return (FALSE);
}

int CockpitManager::HudFont(void)
{
int retval;

	if(mpActivePanel)
	{
		retval = mpActivePanel->HudFont();
	}
	else
	{
		retval = mHudFont;
	}

	return retval;
}

int CockpitManager::MFDFont(void)
{
int retval;

	if(mpActivePanel)
	{
		retval = mpActivePanel->MFDFont();
	}
	else
	{
		retval = mMFDFont;
	}

	return retval;
}

int CockpitManager::DEDFont(void)
{
int retval;

	if(mpActivePanel)
	{
		retval = mpActivePanel->DEDFont();
	}
	else
	{
		retval = mDEDFont;
	}

	return retval;
}
//====================================================//
// CockpitManager::GetViewportBounds
//====================================================//
BOOL CockpitManager::GetViewportBounds(ViewportBounds* bounds, int viewPort) {

	BOOL returnValue = FALSE;
#if DO_HIRESCOCK_HACK
	if(mpActivePanel && !gDoCockpitHack) {
		returnValue		= mpActivePanel->GetViewportBounds(bounds, viewPort);
	}
	else if(mpViewBounds[viewPort]) {
		*bounds			= *mpViewBounds[viewPort];
		returnValue		= TRUE;
	}
#else
	if(mpActivePanel) {
		returnValue		= mpActivePanel->GetViewportBounds(bounds, viewPort);
	}
	else if(mpViewBounds[viewPort]) {
		*bounds			= *mpViewBounds[viewPort];
		returnValue		= TRUE;
	}
#endif
	return(returnValue);
}

void CockpitManager::SetNextView(void)
{
	F4EnterCriticalSection(mpCockpitCritSec);

	if(!mIsNextInitialized && mpNextActivePanel) {
		if(mpNextActivePanel != mpActivePanel) {

			// OW
			// keep them all in video memory, otherwise the texture manger eats up all the video memory and we have to create them in system memory
//			if(!PlayerOptions.bFast2DCockpit)
			{	
				if (mpActivePanel)
					mpActivePanel->DiscardLitSurfaces();
			}

			mpNextActivePanel->CreateLitSurfaces(lightLevel);
			mpActivePanel	= mpNextActivePanel;

			mMouseBounds	= mpActivePanel->mMouseBounds;
			mpActivePanel->SetDirtyFlags();

			mpNextActivePanel = NULL;
			mIsInitialized		= FALSE;
			mViewChanging		= TRUE;
		}
	}

	F4LeaveCriticalSection(mpCockpitCritSec);
}


#if DO_HIRESCOCK_HACK
float	CockpitManager::GetPan(void) {

	if(mpActivePanel && !gDoCockpitHack) {
		return (mpActivePanel->mPan);
	}

	return (0.0F);
}

float	CockpitManager::GetTilt(void) {

	if(mpActivePanel && !gDoCockpitHack) {
		return (mpActivePanel->mTilt);
	}
	
	return (0.0F);
}
#else
float	CockpitManager::GetPan(void) {

	if(mpActivePanel) {
		return (mpActivePanel->mPan);
	}

	return (0.0F);
}

float	CockpitManager::GetTilt(void) {

	if(mpActivePanel) {
		return (mpActivePanel->mTilt);
	}
	
	return (0.0F);
}
#endif

void CockpitManager::SetTOD(float newLightLevel) {
	
	if ((fabs(lightLevel - newLightLevel) <= COCKPIT_LIGHT_CHANGE_TOLERANCE) && 
		(TheTimeOfDay.GetNVGmode() == inNVGmode)) {
		return;
	}

	lightLevel	= newLightLevel;
	inNVGmode	= TheTimeOfDay.GetNVGmode();

	// Apply lighting effects to palette here
	if(mpActivePanel) {
		mpActivePanel->SetTOD(newLightLevel);
	}
}


/***************************************************************************\
    Update the light level on the cockpit.
\***************************************************************************/
void CockpitManager::TimeUpdateCallback( void *self ) {
	((CockpitManager*)self)->SetTOD( TheTimeOfDay.GetLightLevel() );
}


// Save the current cockpit state
void CockpitManager::SaveCockpitDefaults (void)
{
char dataFileName[_MAX_PATH];
char tmpStr[_MAX_PATH];
char tmpStr1[_MAX_PATH];

   sprintf (dataFileName, "%s\\config\\%s.ini", FalconDataDirectory, LogBook.Callsign());

   // Save HUD Data
   sprintf (tmpStr, "%d", TheHud->GetHudColor());
   WritePrivateProfileString("Hud", "Color", tmpStr, dataFileName);

   sprintf (tmpStr, "%d", TheHud->GetScalesSwitch());
   WritePrivateProfileString("Hud", "Scales", tmpStr, dataFileName);

   sprintf (tmpStr, "%d", TheHud->GetBrightnessSwitch());
   WritePrivateProfileString("Hud", "Brightness", tmpStr, dataFileName);

   sprintf (tmpStr, "%d", TheHud->GetFPMSwitch());
   WritePrivateProfileString("Hud", "FPM", tmpStr, dataFileName);

   sprintf (tmpStr, "%d", TheHud->GetDEDSwitch());
   WritePrivateProfileString("Hud", "DED", tmpStr, dataFileName);

   sprintf (tmpStr, "%d", TheHud->GetVelocitySwitch());
   WritePrivateProfileString("Hud", "Velocity", tmpStr, dataFileName);

   sprintf (tmpStr, "%d", TheHud->GetRadarSwitch());
   WritePrivateProfileString("Hud", "Alt", tmpStr, dataFileName);

   //MI
   sprintf(tmpStr, "%d", (int)(TheHud->SymWheelPos * 1000));
   WritePrivateProfileString("Hud", "SymWheelPos", tmpStr, dataFileName);

   // Save the ICP Master Mode
   if(g_bRealisticAvionics)
   {
	   if(mpIcp)
	   {
		   if(mpIcp->IsICPSet(ICPClass::MODE_A_G))
		   {
			   sprintf(tmpStr, "%d", 1);
			   WritePrivateProfileString("ICP", "MasterMode", tmpStr, dataFileName);
		   }
		   else if(mpIcp->IsICPSet(ICPClass::MODE_A_A))
		   {
			   sprintf(tmpStr, "%d", 2);
			   WritePrivateProfileString("ICP", "MasterMode", tmpStr, dataFileName);
		   }
		   else
		   {
			   sprintf(tmpStr, "%d", 0);
			   WritePrivateProfileString("ICP", "MasterMode", tmpStr, dataFileName);
		   }
	   }
   }
   else
   {
	   if (mpIcp->GetPrimaryExclusiveButton())
	   {
		  sprintf (tmpStr, "%d", mpIcp->GetPrimaryExclusiveButton()->GetId());
		  sprintf (tmpStr1, "%d", mpIcp->GetICPPrimaryMode());
	   }
	   else
	   {
		  sprintf (tmpStr, "-1");
		  sprintf (tmpStr1, "-1");
	   }
	   WritePrivateProfileString("ICP", "PrimaryId", tmpStr, dataFileName);
	   WritePrivateProfileString("ICP", "PrimaryMode", tmpStr1, dataFileName);

	   if (mpIcp->GetSecondaryExclusiveButton())
	   {
		  sprintf (tmpStr, "%d", mpIcp->GetSecondaryExclusiveButton()->GetId());
		  sprintf (tmpStr1, "%d", mpIcp->GetICPSecondaryMode());
	   }
	   else
	   {
		  sprintf (tmpStr, "-1");
		  sprintf (tmpStr1, "-1");
	   }
	   WritePrivateProfileString("ICP", "SecondaryId", tmpStr, dataFileName);
	   WritePrivateProfileString("ICP", "SecondaryMode", tmpStr1, dataFileName);

	   if (mpIcp->GetTertiaryExclusiveButton())
	   {
		  sprintf (tmpStr, "%d", mpIcp->GetTertiaryExclusiveButton()->GetId());
		  sprintf (tmpStr1, "%d", mpIcp->GetICPTertiaryMode());
	   }
	   else
	   {
		  sprintf (tmpStr, "-1");
		  sprintf (tmpStr1, "-1");
	   }
	   WritePrivateProfileString("ICP", "TertiaryId", tmpStr, dataFileName);
	   WritePrivateProfileString("ICP", "TertiaryMode", tmpStr1, dataFileName);
   }

   // Save the MFD States
   for (int i=0; i<NUM_MFDS; i++)
   {
      sprintf (tmpStr1, "Display%d", i);
      sprintf (tmpStr, "%d", MfdDisplay[i]->mode);
      WritePrivateProfileString("MFD", tmpStr1, tmpStr, dataFileName);
      // JPO - having written old format, dump the new format
      for (int mm = 0; mm < MFDClass::MAXMM; mm++) {
	  for (int j = 0; j < 3; j++) {
	      sprintf (tmpStr1, "Display%d-%d-%d", i, mm, j);
	      sprintf (tmpStr, "%d", MfdDisplay[i]->primarySecondary[mm][j]);
	      WritePrivateProfileString("MFD", tmpStr1, tmpStr, dataFileName);
	  }
          sprintf(tmpStr1, "Display%d-%d-csel", i, mm);
	  sprintf(tmpStr, "%d", MfdDisplay[i]->cursel[mm]);
	  WritePrivateProfileString("MFD", tmpStr1, tmpStr, dataFileName);
      }

   }
   //MI save EWS stuff
   if(mpIcp)
   {
	   //Chaff and Flare Bingo
	   sprintf(tmpStr1, "Flare Bingo");
	   sprintf(tmpStr,"%d", mpIcp->FlareBingo);
	   WritePrivateProfileString("EWS", tmpStr1, tmpStr, dataFileName);

	   sprintf(tmpStr1, "Chaff Bingo");
	   sprintf(tmpStr,"%d", mpIcp->ChaffBingo);
	   WritePrivateProfileString("EWS", tmpStr1, tmpStr, dataFileName);

	   sprintf(tmpStr1, "Jammer");
	   sprintf(tmpStr,"%d", mpIcp->EWS_JAMMER_ON ? 1 : 0);
	   WritePrivateProfileString("EWS", tmpStr1, tmpStr, dataFileName);

	   sprintf(tmpStr1, "Bingo");
	   sprintf(tmpStr,"%d", mpIcp->EWS_BINGO_ON ? 1 : 0);
	   WritePrivateProfileString("EWS", tmpStr1, tmpStr, dataFileName);

   }
   for(i = 0; i < MAX_PGMS - 1; i++)
   {
	   if(mpIcp)
	   {
		   //Chaff Burst quantity
		   sprintf(tmpStr1,"PGM %d Chaff BQ" , i);
		   sprintf(tmpStr,"%d", mpIcp->iCHAFF_BQ[i]);
		   WritePrivateProfileString("EWS", tmpStr1, tmpStr, dataFileName);

		   //Chaff Burst Interval
		   sprintf(tmpStr1,"PGM %d Chaff BI" , i);
		   sprintf(tmpStr,"%d", (int)(mpIcp->fCHAFF_BI[i] * 1000));
		   WritePrivateProfileString("EWS", tmpStr1, tmpStr, dataFileName);

		   //Chaff Salvo quantity
		   sprintf(tmpStr1, "PGM %d Chaff SQ", i);
		   sprintf(tmpStr, "%d", mpIcp->iCHAFF_SQ[i]);
		   WritePrivateProfileString("EWS", tmpStr1, tmpStr, dataFileName);

		   //Chaff Salvo Interval
		   sprintf(tmpStr1,"PGM %d Chaff SI" , i);
		   sprintf(tmpStr,"%d", (int)(mpIcp->fCHAFF_SI[i] * 1000));
		   WritePrivateProfileString("EWS", tmpStr1, tmpStr, dataFileName);

		   //Flare Burst quantity
		   sprintf(tmpStr1,"PGM %d Flare BQ" , i);
		   sprintf(tmpStr,"%d", mpIcp->iFLARE_BQ[i]);
		   WritePrivateProfileString("EWS", tmpStr1, tmpStr, dataFileName);

		   //Flare Burst Interval
		   sprintf(tmpStr1,"PGM %d Flare BI" , i);
		   sprintf(tmpStr,"%d", (int)(mpIcp->fFLARE_BI[i] * 1000));
		   WritePrivateProfileString("EWS", tmpStr1, tmpStr, dataFileName);

		   //Flare Salvo quantity
		   sprintf(tmpStr1, "PGM %d Flare SQ", i);
		   sprintf(tmpStr, "%d", mpIcp->iFLARE_SQ[i]);
		   WritePrivateProfileString("EWS", tmpStr1, tmpStr, dataFileName);

		   //Flare Salvo Interval
		   sprintf(tmpStr1,"PGM %d Flare SI" , i);
		   sprintf(tmpStr,"%d", (int)(mpIcp->fFLARE_SI[i] * 1000));
		   WritePrivateProfileString("EWS", tmpStr1, tmpStr, dataFileName);
	   }
   }

   //Save the current PGM and number selection
   if(SimDriver.playerEntity)
   {
	   sprintf(tmpStr1, "Mode Selection");
	   sprintf(tmpStr, "%d", SimDriver.playerEntity->EWSPGM());
	   WritePrivateProfileString("EWS", tmpStr1, tmpStr, dataFileName);

	   sprintf(tmpStr1, "Number Selection");
	   sprintf(tmpStr, "%d", SimDriver.playerEntity->EWSProgNum);
	   WritePrivateProfileString("EWS", tmpStr1, tmpStr, dataFileName);
   }
   //MI save Bullseye show option
   if(mpIcp)
   {
	   //Chaff and Flare Bingo
	   sprintf(tmpStr1, "BullseyeInfoOnMFD");
	   sprintf(tmpStr,"%d", mpIcp->ShowBullseyeInfo ? 1 : 0);
	   WritePrivateProfileString("Bullseye", tmpStr1, tmpStr, dataFileName);
   }

   //MI load the laser starting time
   if(mpIcp)
   {
	   sprintf(tmpStr1, "LaserST");
	   sprintf(tmpStr,"%d", mpIcp->LaserTime);
	   WritePrivateProfileString("Laser", tmpStr1, tmpStr, dataFileName);
   }

   //MI save Cockpit selection
   if(SimDriver.playerEntity)
   {
	   sprintf(tmpStr1, "WideView");
	   sprintf(tmpStr,"%d", SimDriver.playerEntity->WideView ? 1 : 0);
	   WritePrivateProfileString("Cockpit View", tmpStr1, tmpStr, dataFileName);
   }


   // Save the current OTW View
   sprintf (tmpStr, "%d", OTWDriver.GetOTWDisplayMode());
   WritePrivateProfileString("OTW", "Mode", tmpStr, dataFileName);

	sprintf(tmpStr, "%d", VM->GetRadioFreq(0));
   WritePrivateProfileString("COMMS", "Comm1", tmpStr, dataFileName);
	sprintf(tmpStr, "%d",  VM->GetRadioFreq(1));
   WritePrivateProfileString("COMMS", "Comm2", tmpStr, dataFileName);

   // Master Arm switch
	 if (SimDriver.playerEntity && SimDriver.playerEntity->Sms) // JB 010628 CTD // JPO CTD fix
	 {
		 sprintf (tmpStr, "%d", SimDriver.playerEntity->Sms->MasterArm());
		 WritePrivateProfileString("Weapons", "MasterArm", tmpStr, dataFileName);
	 }
}


// Save the current cockpit state
void CockpitManager::LoadCockpitDefaults (void)
{
char dataFileName[_MAX_PATH];
char tmpStr[_MAX_PATH];
int pMode, sMode, tMode, i;
int pButton, sButton, tButton, buttonId;
CPButtonObject* theButton;

   sprintf (dataFileName, "%s\\config\\%s.ini", FalconDataDirectory, LogBook.Callsign());

   // Load HUD Data
   TheHud->SetHudColor(GetPrivateProfileInt("Hud", "Color", TheHud->GetHudColor(), dataFileName));
   TheHud->SetScalesSwitch((HudClass::ScalesSwitch)GetPrivateProfileInt("Hud", "Scales", TheHud->GetScalesSwitch(), dataFileName));
   OTWDriver.pCockpitManager->Dispatch(1066, 0);
   TheHud->SetFPMSwitch((HudClass::FPMSwitch)GetPrivateProfileInt("Hud", "FPM", TheHud->GetFPMSwitch(), dataFileName));
   OTWDriver.pCockpitManager->Dispatch(1067, 0);
   TheHud->SetDEDSwitch((HudClass::DEDSwitch)GetPrivateProfileInt("Hud", "DED", TheHud->GetDEDSwitch(), dataFileName));
   OTWDriver.pCockpitManager->Dispatch(1068, 0);
   TheHud->SetVelocitySwitch((HudClass::VelocitySwitch)GetPrivateProfileInt("Hud", "Velocity", TheHud->GetVelocitySwitch(), dataFileName));
   OTWDriver.pCockpitManager->Dispatch(1069, 0);
   TheHud->SetRadarSwitch((HudClass::RadarSwitch)GetPrivateProfileInt("Hud", "Alt", TheHud->GetRadarSwitch(), dataFileName));
   OTWDriver.pCockpitManager->Dispatch(1070, 0);
   TheHud->SetBrightnessSwitch((HudClass::BrightnessSwitch)GetPrivateProfileInt("Hud", "Brightness", TheHud->GetBrightnessSwitch(), dataFileName));
   OTWDriver.pCockpitManager->Dispatch(1071, 0);

   //MI
   TheHud->SymWheelPos = GetPrivateProfileInt("Hud", "SymWheelPos", 1000.0F, dataFileName);
   TheHud->SymWheelPos /= 1000.0F;
   TheHud->SymWheelPos = max(0.5F,min(TheHud->SymWheelPos, 1.0F));

   // Reload the ICP Master Mode
   //MI
   if(g_bRealisticAvionics)
   {
	   int Mode = GetPrivateProfileInt("ICP", "MasterMode", -1, dataFileName);
	   if(Mode >= 0 && Mode < 3)
	   {
		   if(mpIcp)
		   {
			   switch(Mode)
			   {
			   case 0:	//NAV
				   if(mpIcp->IsICPSet(ICPClass::MODE_A_G))
				   {
					   SimICPAG(0, KEY_DOWN, NULL);
				   }
				   else if(mpIcp->IsICPSet(ICPClass::MODE_A_A))
				   {
					   SimICPAA(0, KEY_DOWN, NULL);
				   }
			   break;
			   case 1:	//AG
				   if(!mpIcp->IsICPSet(ICPClass::MODE_A_G))
				   {
					   SimICPAG(0, KEY_DOWN, NULL);
				   }
			   break;
			   case 2:	//AA
				   if(!mpIcp->IsICPSet(ICPClass::MODE_A_A))
				   {
					   SimICPAA(0, KEY_DOWN, NULL);
				   }
			   break;
			   default:
			   break;
			   }
			   mpIcp->ChangeToCNI();
			   mpIcp->LastMode = CNI_MODE;
			   mpIcp->SetICPTertiaryMode(CNI_MODE);
		   }
	   }
   }
   else
   {
	   pButton = GetPrivateProfileInt("ICP", "PrimaryId", -1, dataFileName);
	   sButton = GetPrivateProfileInt("ICP", "SecondaryId", -1, dataFileName);
	   tButton = GetPrivateProfileInt("ICP", "TertiaryId", -1, dataFileName);
	   pMode = GetPrivateProfileInt("ICP", "PrimaryMode", mpIcp->GetICPPrimaryMode(), dataFileName);
	   sMode = GetPrivateProfileInt("ICP", "SecondaryMode", mpIcp->GetICPSecondaryMode(), dataFileName);
	   tMode = GetPrivateProfileInt("ICP", "TertiaryMode", mpIcp->GetICPTertiaryMode(), dataFileName);
 


	   if (VM)
	   {
		   VM->ChangeRadioFreq(GetPrivateProfileInt("COMMS", "Comm1", rcfFlight, dataFileName), 0);
		   VM->ChangeRadioFreq(GetPrivateProfileInt("COMMS", "Comm2", rcfPackage, dataFileName), 1);
	   }

	   // Find the primary mode button
		buttonId		= pButton;
		i = 0;
		theButton = NULL;
			
		// search cpmanager's list of button pointers
		while(i < mButtonTally)
	   {
			if(mpButtonObjects[i]->GetId() == buttonId)
		  {
			 theButton = mpButtonObjects[i];
			 break;
			}
			else
		  {
				i++;
			}
	   }

	   // If found, push it
	   if (theButton)
	   {
		  theButton->HandleMouseEvent(3);  //Button 0 down
		  theButton->HandleEvent(buttonId);
	   }

	   // Find the secondary mode button
	   theButton = NULL;
		buttonId		= sButton;
		i = 0;
			
		// search cpmanager's list of button pointers
		while(i < mButtonTally)
	   {
			if(mpButtonObjects[i]->GetId() == buttonId)
		  {
			 theButton = mpButtonObjects[i];
			 break;
			}
			else
		  {
				i++;
			}
	   }

	   // If found, push it
	   if (theButton)
	   {
		  theButton->HandleMouseEvent(3);  //Button 0 down
		  theButton->HandleEvent(buttonId);
	   }


	   // Find the tertiary mode button
	   theButton = NULL;
		buttonId		= tButton;
		i = 0;
			
		// search cpmanager's list of button pointers
		while(i < mButtonTally)
	   {
			if(mpButtonObjects[i]->GetId() == buttonId)
		  {
			 theButton = mpButtonObjects[i];
			 break;
			}
			else
		  {
				i++;
			}
	   }

	   // If found, push it
	   if (theButton)
	   {
		  theButton->HandleMouseEvent(3);  //Button 0 down
		  theButton->HandleEvent(buttonId);
	   }
   }

   // load the OTW View
   OTWDriver.SetOTWDisplayMode((OTWDriverClass::OTWDisplayMode)GetPrivateProfileInt("OTW", "Mode", OTWDriver.GetOTWDisplayMode(), dataFileName));

   // Load the MFD States
   for (i=0; i<NUM_MFDS; i++)
   {
       // JPO ignore the old format in favour of newer stuff
//       sprintf (tmpStr, "Display%d", i);
//       MfdDisplay[i]->SetNewMode ((MFDClass::MfdMode)GetPrivateProfileInt("MFD", tmpStr, MfdDisplay[i]->mode, dataFileName));
       for (int mm = 0; mm < MFDClass::MAXMM; mm++) {
	   for (int j = 0; j < 3; j++) {
	       sprintf (tmpStr, "Display%d-%d-%d", i, mm, j);
	       int val = GetPrivateProfileInt("MFD", tmpStr, MfdDisplay[i]->primarySecondary[mm][j], dataFileName);
	       MfdDisplay[i]->primarySecondary[mm][j] = (MFDClass::MfdMode)val;
	   }
	   sprintf (tmpStr, "Display%d-%d-curmm", i, mm);
	   MfdDisplay[i]->cursel[mm] = GetPrivateProfileInt("MFD", tmpStr, MfdDisplay[i]->cursel[mm], dataFileName);
       }
   }

   //MI EWS stuff
   if(mpIcp && SimDriver.playerEntity && !F4IsBadReadPtr(SimDriver.playerEntity, sizeof(AircraftClass)))
   {
	   //Chaff and Flare Bingo
	   mpIcp->FlareBingo = GetPrivateProfileInt("EWS", "Flare Bingo", 
		   mpIcp->FlareBingo, dataFileName);

	   mpIcp->ChaffBingo = GetPrivateProfileInt("EWS", "Chaff Bingo", 
		   mpIcp->FlareBingo, dataFileName);

	   //Jammer and Bingo
	   int temp = GetPrivateProfileInt("EWS", "Jammer",
		   SimDriver.playerEntity->EWSProgNum, dataFileName);
	   if(temp == 1)
		   mpIcp->EWS_JAMMER_ON = TRUE;
	   else
		   mpIcp->EWS_JAMMER_ON = FALSE;
	   
	   temp = GetPrivateProfileInt("EWS", "Bingo",
		   SimDriver.playerEntity->EWSProgNum, dataFileName);

	   if(temp == 1)
		   mpIcp->EWS_BINGO_ON = TRUE;
	   else
		   mpIcp->EWS_BINGO_ON = FALSE;
   }
   for(i = 0; i < MAX_PGMS - 1; i++)
   {
	   if(mpIcp)
	   {
		   sprintf(tmpStr, "PGM %d Chaff BQ", i);
		   mpIcp->iCHAFF_BQ[i] = GetPrivateProfileInt("EWS", tmpStr, mpIcp->iCHAFF_BQ[i],
			   dataFileName);

		   sprintf(tmpStr, "PGM %d Chaff BI", i);
		   mpIcp->fCHAFF_BI[i] = GetPrivateProfileInt("EWS", tmpStr, mpIcp->fCHAFF_BI[i] * 1000,
			   dataFileName);
		   mpIcp->fCHAFF_BI[i] /= 1000;

		   sprintf(tmpStr, "PGM %d Chaff SQ", i);
		   mpIcp->iCHAFF_SQ[i] = GetPrivateProfileInt("EWS", tmpStr, mpIcp->iCHAFF_SQ[i],
			   dataFileName);

		   sprintf(tmpStr, "PGM %d Chaff SI", i);
		   mpIcp->fCHAFF_SI[i] = GetPrivateProfileInt("EWS", tmpStr, mpIcp->fCHAFF_SI[i] * 1000,
			   dataFileName);
		   mpIcp->fCHAFF_SI[i] /= 1000;


		   sprintf(tmpStr, "PGM %d Flare BQ", i);
		   mpIcp->iFLARE_BQ[i] = GetPrivateProfileInt("EWS", tmpStr, mpIcp->iFLARE_BQ[i],
			   dataFileName);

		   sprintf(tmpStr, "PGM %d Flare BI", i);
		   mpIcp->fFLARE_BI[i] = GetPrivateProfileInt("EWS", tmpStr, mpIcp->fFLARE_BI[i] * 1000,
			   dataFileName);
		   mpIcp->fFLARE_BI[i] /= 1000;

		   sprintf(tmpStr, "PGM %d Flare SQ", i);
		   mpIcp->iFLARE_SQ[i] = GetPrivateProfileInt("EWS", tmpStr, mpIcp->iFLARE_SQ[i],
			   dataFileName);

		   sprintf(tmpStr, "PGM %d Flare SI", i);
		   mpIcp->fFLARE_SI[i] = GetPrivateProfileInt("EWS", tmpStr, mpIcp->fFLARE_SI[i] * 1000,
			   dataFileName);
		   mpIcp->fFLARE_SI[i] /= 1000;
	   }
   }
   //MI save Bullseye show option
   if(mpIcp)
   {
	   //Chaff and Flare Bingo
	   sprintf(tmpStr, "BullseyeInfoOnMFD");
	   int temp = GetPrivateProfileInt("Bullseye", tmpStr, 0, dataFileName);
	   if(temp <= 0)
		   mpIcp->ShowBullseyeInfo = FALSE;
	   else 
		   mpIcp->ShowBullseyeInfo = TRUE;
   }
   //MI load the laser starting time
   if(mpIcp)
   {
	   sprintf(tmpStr, "LaserST");
	   int temp = GetPrivateProfileInt("Laser", tmpStr, 8, dataFileName);
	   if(temp < 0)
		   mpIcp->LaserTime = 8.0F;	//standard
	   else if(temp > 176)
		   mpIcp->LaserTime = 176.0F;	//maximum
	   else 
		   mpIcp->LaserTime = temp;
   }
   //MI save Cockpit selection
   if(SimDriver.playerEntity && !F4IsBadReadPtr(SimDriver.playerEntity, sizeof(AircraftClass)))
   {
	   sprintf(tmpStr, "WideView");
	   int temp = GetPrivateProfileInt("Cockpit View", tmpStr, 0, dataFileName);
	   if(temp <= 0)
		   SimDriver.playerEntity->WideView = FALSE;
	   else
		   SimDriver.playerEntity->WideView = TRUE;

	   if(OTWDriver.pCockpitManager)
	   {
		   if(SimDriver.playerEntity->WideView)
			   OTWDriver.pCockpitManager->SetActivePanel(91100);
		   else
			   OTWDriver.pCockpitManager->SetActivePanel(1100);
	   }
   }

   // Master Arm
   //if (SimDriver.playerEntity) // JB 010220 CTD
	 if (SimDriver.playerEntity && SimDriver.playerEntity->Sms && !F4IsBadReadPtr(SimDriver.playerEntity, sizeof(AircraftClass)) && !F4IsBadCodePtr((FARPROC) SimDriver.playerEntity->Sms)) // JB 010220 CTD
   {
	   SimDriver.playerEntity->Sms->SetMasterArm((SMSBaseClass::MasterArmState)GetPrivateProfileInt("Weapons", "MasterArm",
		   SimDriver.playerEntity->Sms->MasterArm(), dataFileName));

	   //MI EWS stuff
	   SimDriver.playerEntity->SetPGM((AircraftClass::EWSPGMSwitch)GetPrivateProfileInt("EWS", "Mode Selection",
		   SimDriver.playerEntity->EWSPGM(), dataFileName));

	   SimDriver.playerEntity->EWSProgNum = GetPrivateProfileInt("EWS", "Number Selection",
		   SimDriver.playerEntity->EWSProgNum, dataFileName);
   }
   OTWDriver.pCockpitManager->Dispatch(1097, 0);
}

//====================================================//
// CreateCockpitGeometry
//====================================================//

void CreateCockpitGeometry(DrawableBSP** ppGeometry, int normalType, int dogType)
{
	int model=0;
	int team=0;
	int texture=1;

	BOOL isDogfight;

	isDogfight = FalconLocalGame->GetGameType() == game_Dogfight;

	if(isDogfight) {
		model = dogType;	// change this to proper vis type when it gets into classtable
		team = FalconLocalSession->GetTeam();
 		switch(team) {
		case 1:
			texture = 3;
			break;

		case 2:
			texture = 4;
			break;

		case 3:
			texture = 1;
			break;

		case 4:
		default:
			texture = 2;
			break;
		}
	}
	else {
		model = normalType; // change this to proper vis type when it gets into classtable
	}

	// Wings and reflection
	DrawableBSP::LockAndLoad (model);
   *ppGeometry = new DrawableBSP( model, &Origin, &IMatrix, 1.0f );
	(*ppGeometry)->SetScale( 10.0f );	// Keep the geometry away from the clipping plane


	if(isDogfight) {
		(*ppGeometry)->SetTextureSet(texture);
	}
}


long CalculateNVGColor(long dayColor) {

	long nvgColor = 0;
	long red;
	long green;
	long blue;

	red	= dayColor & 0x000000ff;
	green	= (dayColor & 0x0000ff00) >> 8;
	blue	= (dayColor & 0x00ff0000) >> 16;

	nvgColor = (red + green + blue) / 3;
	nvgColor = nvgColor << 8;
	return nvgColor;
}

// JPO
// used to set all the lights buttons and switches to their corerct states.
void	CockpitManager::InitialiseInstruments(void)
{
    // actually for now - its jsut buttons that need to be up to date.
    for (int i = 0; i< mButtonTally; i++) {
	mpButtonObjects[i]->UpdateStatus();
    }
}
