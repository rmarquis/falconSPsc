#ifndef _COCKPITMAN_H
#define _COCKPITMAN_H
#include <stdio.h>

#include "stdhdr.h"
#include "otwdrive.h"
#include "Graphics\Include\imagebuf.h"
#include "Graphics\Include\Device.h"

#include "cpsurface.h"
#include "cppanel.h"
#include "cpcursor.h"
#include "cpmisc.h"
#include "dispopts.h"

#ifdef USE_SH_POOLS
extern MEM_POOL gCockMemPool;
#endif

#define _ALWAYS_DIRTY 1

#define _CPMAN_USE_STL_CONTAINERS

#ifdef _CPMAN_USE_STL_CONTAINERS
#include <vector>
#endif

#include "classtbl.h"

//====================================================//
// Default view
//====================================================//

#define COCKPIT_DEFAULT_PANEL 1100
#define PADLOCK_DEFAULT_PANEL 100

//====================================================//
// Special Defines for Special Devices
//====================================================//

#define ICP_INITIAL_SECONDARY_BUTTON	1018
#define ICP_INITIAL_PRIMARY_BUTTON	1013
#define ICP_INITIAL_TERTIARY_BUTTON	1011

//====================================================//
// Mouse and Keyboard Events
//====================================================//

#define CP_MOUSE_MOVE		0
#define CP_MOUSE_DOWN		1
#define CP_MOUSE_UP			2
#define CP_MOUSE_BUTTON0	3
#define CP_MOUSE_BUTTON1	4
#define CP_CHECK_EVENT		-1 // fake event, used to query status.

//====================================================//
// BLIT Properties
//====================================================//

#define CPOPAQUE			0
#define CPTRANSPARENT	1

//====================================================//
// Cycle Control Words
//====================================================//

#define BEGIN_CYCLE	0x0001
#define END_CYCLE		0x8000
#define CYCLE_ALL		0xFFFF

//====================================================//
// Persistance Types
//====================================================//

#define NONPERSISTANT	0
#define PERSISTANT		1

//====================================================//
// Location of Cockpit Data and Images
//====================================================//

#define COCKPIT_FILE_6x4					"6_ckpit.dat"
#define COCKPIT_FILE_8x6					"8_ckpit.dat"
#define COCKPIT_FILE_10x7					"10_ckpit.dat"
// OW new
#define COCKPIT_FILE_12x9					"12_ckpit.dat"
#define COCKPIT_FILE_16x12					"16_ckpit.dat"

#define PADLOCK_FILE_6x4					"6_plock.dat"
#define PADLOCK_FILE_8x6					"8_plock.dat"
#define PADLOCK_FILE_10x7					"10_plock.dat"
// OW new
#define PADLOCK_FILE_12x9					"12_plock.dat"
#define PADLOCK_FILE_16x12					"16_plock.dat"

#define COCKPIT_DIR				"art\\ckptart\\"

//====================================================//
// Miscellaneous Defines
//====================================================//
#define PANELS_INACTIVE -1
#define NO_ADJ_PANEL		-1
#define PADLOCK_PANEL	100

//====================================================//
// Defines for Parsing up Data File
//====================================================//

#define MAX_LINE_BUFFER	512

#define TYPE_MANAGER_STR		"MANAGER"
#define TYPE_SURFACE_STR		"SURFACE"
#define TYPE_PANEL_STR			"PANEL"
#define TYPE_LIGHT_STR			"LIGHT"
#define TYPE_BUTTON_STR			"BUTTON"
#define TYPE_BUTTONVIEW_STR	"BUTTONVIEW"
#define TYPE_MULTI_STR			"MULTI"
#define TYPE_INDICATOR_STR		"INDICATOR"	
#define TYPE_DIAL_STR			"DIAL"
#define TYPE_CURSOR_STR			"CURSOR"
#define TYPE_DED_STR				"DED"
#define TYPE_ADI_STR				"ADI"
#define TYPE_MACHASI_STR		"MACHASI"
#define TYPE_HSI_STR				"HSI"
#define TYPE_DIGITS_STR			"DIGITS"
#define TYPE_BUFFER_STR			"BUFFER"
#define TYPE_SOUND_STR			"SOUND"
#define TYPE_CHEVRON_STR		"CHEVRON"
#define TYPE_LIFTLINE_STR		"LIFTLINE"
#define TYPE_TEXT_STR			"TEXT"
#define TYPE_KNEEVIEW_STR		"KNEEVIEW"

#define PROP_ORIENTATION_STR	"orientation"
#define PROP_PARENTBUTTON_STR	"parentbutton"
#define PROP_HORIZONTAL_STR	"horizontal"
#define PROP_VERTICAL_STR		"vertical"
#define PROP_NUMPANELS_STR		"numpanels"
#define PROP_MOUSEBORDER_STR	"mouseborder"
#define PROP_NUMOBJECTS_STR	"numobjects"
#define PROP_TEMPLATEFILE_STR	"templatefile"
#define PROP_FILENAME_STR		"filename"
#define PROP_SRCLOC_STR			"srcloc"
#define PROP_DESTLOC_STR		"destloc"
#define PROP_NUMSURFACES_STR	"numsurfaces"
#define PROP_NUMCURSORS_STR	"numcursors"
#define PROP_SURFACES_STR		"surfaces"
#define PROP_NUMOBJECTS_STR	"numobjects"
#define PROP_NUMBUTTONS_STR	"numbuttons"
#define PROP_NUMBUTTONVIEWS_STR	"numbuttonviews"
#define PROP_OBJECTS_STR		"objects"
#define PROP_TRANSPARENT_STR	"transparent"
#define PROP_OPAQUE_STR			"opaque"
#define PROP_STATES_STR			"states"
#define PROP_CALLBACKSLOT_STR	"callbackslot"
#define PROP_CURSORID_STR		"cursorid"
#define PROP_HOTSPOT_STR		"hotspot"
#define PROP_BUTTONS_STR		"buttons"
#define PROP_MINVAL_STR			"minval"
#define PROP_MAXVAL_STR			"maxval"
#define PROP_MINPOS_STR			"minpos"
#define PROP_MAXPOS_STR			"maxpos"
#define PROP_NUMTAPES_STR		"numtapes"
#define PROP_RADIUS0_STR		"radius0"
#define PROP_RADIUS1_STR		"radius1"
#define PROP_RADIUS2_STR		"radius2"
#define PROP_NUMENDPOINTS_STR	"numendpoints"
#define PROP_POINTS_STR			"points"
#define PROP_VALUES_STR			"values"
#define PROP_INITSTATE_STR		"initstate"
#define PROP_TYPE_STR			"type"
#define PROP_MOMENTARY_STR		"momentary"
#define PROP_EXCLUSIVE_STR		"exclusive"
#define PROP_TOGGLE_STR			"toggle"
#define PROP_INNERARC_STR		"innerarc"
#define PROP_OUTERARC_STR		"outerarc"
#define PROP_INNERSRCARC_STR	"innersrcarc"
#define PROP_OFFSET_STR			"offset"
#define PROP_PANTILT_STR		"pantilt"
#define PROP_MASKTOP_STR		"masktop"
#define PROP_MOUSEBOUNDS_STR	"mousebounds"
#define PROP_ADJPANELS_STR		"adjpanels"
#define PROP_CYCLEBITS_STR		"cyclebits"
#define PROP_PERSISTANT_STR	"persistant"
#define PROP_BSURFACE_STR		"bsurface"
#define PROP_BSRCLOC_STR		"bsrcloc"
#define PROP_BDESTLOC_STR		"bdestloc"
#define PROP_COLOR0_STR			"color0"
#define PROP_COLOR1_STR			"color1"
#define PROP_COLOR2_STR			"color2"
#define PROP_COLOR3_STR			"color3"
#define PROP_COLOR4_STR			"color4"
#define PROP_COLOR5_STR			"color5"
#define PROP_COLOR6_STR			"color6"
#define PROP_COLOR7_STR			"color7"
#define PROP_COLOR8_STR			"color8"
#define PROP_COLOR9_STR			"color9"
#define PROP_HUD_STR				"hud"
#define PROP_MFDLEFT_STR		"mfdleft"
#define PROP_MFDRIGHT_STR		"mfdright"
#define PROP_OSBLEFT_STR		"osbleft"
#define PROP_OSBRIGHT_STR		"osbright"
#define PROP_RWR_STR				"rwr"
#define PROP_NUMDIGITS_STR		"numdigits"
#define PROP_BUTTONVIEWS_STR  "buttonviews"
#define PROP_DELAY_STR			"delay"
#define PROP_NUMSOUNDS_STR		"numsounds"
#define PROP_ENTRY_STR			"entry"
#define PROP_SOUND1_STR			"sound1"
#define PROP_SOUND2_STR			"sound2"
#define PROP_CALIBRATIONVAL_STR	"calibrationval"
#define PROP_ILSLIMITS_STR		"ilslimits"
#define PROP_BUFFERSIZE_STR	"buffersize"
#define PROP_DOGEOMETRY_STR	"dogeometry"
#define PROP_NEEDLERADIUS_STR	"needleradius"
#define PROP_STARTANGLE_STR	"startangle"
#define PROP_ARCLENGTH_STR		"arclength"
#define PROP_MAXMACHVALUE_STR	"maxmachvalue"
#define PROP_MINMACHVALUE_STR	"minmachvalue"

#define PROP_LIFTCENTERS_STR	 "liftcenters"
#define PROP_NUMSTRINGS_STR	 "numstrings"
#define PROP_PANTILTLABEL_STR	 "pantiltlabel"
#define PROP_BLITBACKGROUND_STR "blitbackground"
#define PROP_BACKDEST_STR		"backdest"
#define PROP_BACKSRC_STR		"backsrc"
#define PROP_WARNFLAG_STR		"warnflag"
#define PROP_ENDLENGTH        "endlength"
#define PROP_ENDANGLE        "endangle"
#define PROP_HUDFONT        "hudfont"
#define PROP_MFDFONT        "mfdfont"
#define PROP_DEDFONT        "dedfont"
#define PROP_GENFONT        "generalfont"
#define PROP_POPFONT        "popupfont"
#define PROP_KNEEFONT       "kneefont"
#define PROP_SAFONT			 "saboxfont"
#define PROP_LABELFONT		 "labelfont"
#define END_MARKER				 "#end"
#define PROP_DED_TYPE	    "dedtype"
#define PROP_DED_DED	    "ded"
#define PROP_DED_PFL	    "pfl"
#define PROP_DO2DPIT_STR    "cockpit2d"
#define PROP_LIFT_LINE_COLOR "liftlinecolor"

// special 3d cockpit keys
#define PROP_3D_PADBACKGROUND	"padlockbg"
#define PROP_3D_PADLIFTLINE	"padlockliftline"
#define PROP_3D_PADBOXSIDE	"padlockvpside"
#define PROP_3D_PADBOXTOP	"padlockvptop"
#define PROP_3D_PADTICK		"padlocktick"
#define PROP_3D_NEEDLE0		"needlecolor0"
#define PROP_3D_NEEDLE1 	"needlecolor1"
#define PROP_3D_DED		"dedcolor"
#define PROP_3D_COCKPIT		"cockpitmodel"
#define PROP_3D_COCKPITDF	"cockpitdfmodel"
#define PROP_3D_MAINMODEL	"mainmodel"
#define PROP_3D_DAMAGEDMODEL	"damagedmodel"


extern OTWDriverClass OTWDriver;

//====================================================//
// Miscellaneous Utility Functions
//====================================================//

int	Compare(const void *, const void *);
char*	FindToken(char**lineptr, const char*separators);
void	CreateCockpitGeometry(DrawableBSP**, int, int);
void	ReadImage(char*, GLubyte**, GLulong**);
void	Translate8to16( WORD *, BYTE *, ImageBuffer * );
void	Translate8to32( DWORD *, BYTE *, ImageBuffer * );// OW
long	CalculateNVGColor(long);
int FindCockpit(const char *pCPFile, Vis_Types eCPVisType, const TCHAR* eCPName, const TCHAR* eCPNameNCTR, TCHAR *strCPFile);


//====================================================//
// Forward Declarations of Cockpit Objects 
// and Outside World
//====================================================//

class OTWDriverClass;
class	AircraftClass;

class CPObject;
class CPLight;
class	CPDial;
class	CPGauge;
class CPButtonObject;
class CPButtonView;
class CPSurface;
class CPPanel;
class CPMulit;
class CPMisc;
class ICPClass;
class KneeBoard;
class CPDed;
class CPHsi;
class CPSoundList;


void Translate8to16(WORD *, BYTE *, ImageBuffer *);

//====================================================//
// CockpitManager Class Definition
//====================================================//
extern BOOL gDoCockpitHack;
#define DO_HIRESCOCK_HACK 1

class CockpitManager {
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(gCockMemPool,size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
private:

	//====================================================//
	// Temporary pointers, used mainly during initialization
	// and cleanup
	//====================================================//

	int				mLoadBufferHeight;
	int				mLoadBufferWidth;
	int				mSurfaceTally;
	int				mPanelTally;
	int				mObjectTally;
	int				mCursorTally;
	int				mButtonTally;
	int				mButtonViewTally;

	int				mNumSurfaces;
	int				mNumPanels;
	int				mNumObjects;
	int				mNumCursors;
	int				mNumButtons;
	int				mNumButtonViews;
	int            mHudFont;
	int            mMFDFont;
	int            mDEDFont;
	int            mGeneralFont;
	int            mPopUpFont;
	int            mKneeFont;
	int            mSABoxFont;
	int            mLabelFont;

// OW 
#ifndef _CPMAN_USE_STL_CONTAINERS
	CPSurface		**mpSurfaces;
	CPPanel			**mpPanels;
	CPObject			**mpObjects;
	CPCursor			**mpCursors;
	CPButtonObject	**mpButtonObjects;
	CPButtonView	**mpButtonViews;
#else
	std::vector<CPSurface*> mpSurfaces;
	std::vector<CPPanel*> mpPanels;
	std::vector<CPObject*> mpObjects;
	std::vector<CPCursor*> mpCursors;
	std::vector<CPButtonObject*> mpButtonObjects;
	std::vector<CPButtonView	*> mpButtonViews;
#endif

	//====================================================//
	// Internal Pointers, Used Mainly for Runtime
	//====================================================//

	CPPanel			*mpActivePanel;
	CPPanel			*mpNextActivePanel;

	//====================================================//
	// Lighting Stuff
	//====================================================//
	
	float				lightLevel;
	BOOL				inNVGmode;
	Vis_Types m_eCPVisType;		// OW
	_TCHAR		m_eCPName[15]; // JB 010711 Name of aircraft for cockpit loading
	_TCHAR		m_eCPNameNCTR[5]; // JB 010808 Alternate name of aircraft for cockpit loading

	//====================================================//
	// Control Variables
	//====================================================//

	BOOL				mIsInitialized;
	BOOL				mIsNextInitialized;
	RECT				mMouseBounds;
	int				mCycleBit;

	//====================================================//
	// Pointer to buffer for Texture Loading
	//====================================================//

	GLubyte			*mpLoadBuffer;

	//====================================================//
	// Initialization Member Functions
	//====================================================//
	
	void CreateText(int, FILE*);
	void CreateChevron(int, FILE*);
	void CreateLiftLine(int, FILE*);
	void CreateSurface(int, FILE*);
	void CreatePanel(int, FILE*);
	void CreateSwitch(int, FILE*);
	void CreateLight(int, FILE*);
	void CreateButton(int, FILE*);
	void CreateButtonView(int, FILE*);
	void CreateIndicator(int, FILE*);
	void CreateDial(int, FILE*);
	void CreateMulti(int, FILE*);
	void CreateCursor(int, FILE*);
	void CreateDed(int, FILE*);
	void CreateAdi(int, FILE*);
	void CreateMachAsi(int, FILE*);
	void CreateHsiView(int, FILE*);
	void CreateDigits(int, FILE*);
	void CreateSound(int, FILE*);
	void CreateKneeView(int, FILE*);
	void LoadBuffer(FILE*);
	void SetupControlTemplate(char*, int, int);
	void ParseManagerInfo(FILE*);
	void ResolveReferences(void);
	static void	TimeUpdateCallback( void *self );

public:
	//====================================================//
	// Dimensions specific to the Cockpit
	//====================================================//

	int				mTemplateWidth;
	int				mTemplateHeight;
	int				mMouseBorder;
	int				mScale; // for stretched cockpits

	//====================================================//
	// Miscellaneous State Information
	//====================================================//
	CPMisc			mMiscStates;
	CPSoundList		*mpSoundList;
	BOOL				mViewChanging;
	F4CSECTIONHANDLE*	mpCockpitCritSec;

	//====================================================//
	// Pointer to Device Viewport Boundaries
	//====================================================//

	ViewportBounds	*mpViewBounds[BOUNDS_TOTAL];


	//====================================================//
	// Pointers to special avionics devices
	//====================================================//

	ICPClass			*mpIcp;
	CPHsi				*mpHsi;
	DrawableBSP		*mpGeometry;		// Pointer to wings and reflections
	KneeBoard		*mpKneeBoard;

	float				ADIGpDevReading;
	float				ADIGsDevReading;
	BOOL				mHiddenFlag;

	//====================================================//
	// Pointers to the Outside World
	//====================================================//

	ImageBuffer		*mpOTWImage;
	SimBaseClass	*mpOwnship;

	//====================================================//
	// Public Constructors and Destructions
	//====================================================//

	~CockpitManager();

#if DO_HIRESCOCK_HACK
	CockpitManager(ImageBuffer*, char*, BOOL, int, BOOL, Vis_Types eCPVisType = VIS_F16C, TCHAR* eCPName = NULL, TCHAR* eCPNameNCTR = NULL);
#else
	CockpitManager(ImageBuffer*, char*, BOOL, int);
#endif

	//====================================================//
	// Public Runtime Functions
	//====================================================//

	void			Exec(void);						// Called by main sim thread
	void			DisplayBlit(void);				// Called by display thread
	void			DisplayDraw(void);				// Called by display thread
	void			GeometryDraw(void);				// Called by display thread for drawing wings and reflections
	int			Dispatch(int, int, int);		// Called by input thread
	void			Dispatch(int, int);
	int			POVDispatch(int, int, int);

	// OW
	void			DisplayBlit3D(void);				// Called by display thread
	void			InitialiseInstruments(void);			// JPO - set switches and lights up correctly

	//====================================================//
	// Public Auxillary Functions
	//====================================================//

	CPButtonObject* GetButtonPointer(int);
	void		BoundMouseCursor(int*, int*);
	void		SetActivePanel(int);
	void		SetDefaultPanel(int defaultPanel) {SetActivePanel(defaultPanel);}
	void		SetOwnship(SimBaseClass*);
	BOOL		ShowHud();
	BOOL		ShowMfd();
	BOOL		ShowRwr();
	CPPanel*	GetActivePanel()	{return mpActivePanel;}
	int		GetCockpitWidth() {return DisplayOptions.DispWidth;};
	int		GetCockpitHeight(){return DisplayOptions.DispHeight;};
	BOOL		GetViewportBounds(ViewportBounds*, int);
	float		GetCockpitMaskTop(void);
	void		SetNextView(void);
	float		GetPan(void);
	float		GetTilt(void);
	void		SetTOD(float);
	void		ImageCopy(GLubyte*, GLubyte*, int, RECT*);
   void     LoadCockpitDefaults (void);
   void     SaveCockpitDefaults (void);
	int      HudFont(void);
	int      MFDFont(void);
	int      DEDFont(void);
	int      GeneralFont(void) { return mGeneralFont;};
	int      PopUpFont(void) { return mPopUpFont;};
	int      KneeFont(void) { return mKneeFont;};
	int      SABoxFont(void) { return mSABoxFont;};
	int      LabelFont(void) { return mLabelFont;};

};

//====================================================//
// Pointers to template and cockpit palette
//====================================================//

extern ImageBuffer	*gpTemplateSurface;
extern GLubyte			*gpTemplateImage;
extern GLulong			*gpTemplatePalette;

#endif

