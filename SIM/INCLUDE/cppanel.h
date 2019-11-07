#ifndef _CPPANEL_H
#define _CPPANEL_H

#include "cpvbounds.h"
#include "button.h"
#include "cpsurface.h"
#include "cpobject.h"
#include "icp.h"

#ifdef USE_SH_POOLS
extern MEM_POOL gCockMemPool;
#endif


//====================================================//
// Initialization Structs
//====================================================//

typedef struct {
			int			surfaceNum;	
			int			transparencyType;
			BOOL			persistant;
			CPSurface	*psurface;
			RECT			destRect;
			} PanelSurfaceStr;


typedef struct {
			int			N;
			int			NE;
			int			E;
			int			SE;
			int			S;
			int			SW;
			int			W;
			int			NW;
			} PanelAdjStr;

typedef struct {
			int						idNum;
			float						pan;
			float						tilt;
			int						maskTop;
			int						xOffset;
			int						yOffset;
			int						numSurfaces;
			int						numObjects;
			int						*pobjectIDs;
			int						numButtonViews;
			int						*pbuttonViewIDs;
			RECT						mouseBounds;
			PanelAdjStr				adjacentPanels;
			int						defaultCursor;
			int						cockpitWidth;
			int						cockpitHeight;
			RECT						*pviewRects[BOUNDS_TOTAL];
			ImageBuffer				*pOtwImage;
			PanelSurfaceStr		*psurfaceData;
			BOOL						doGeometry;
         float                osbLocation[2][20][2];
			int						scale;
			int                  hudFont;
			int                  mfdFont;
			int                  dedFont;
			} PanelInitStr;

//====================================================//
// Class Predeclarations
//====================================================//
class CPButtonObject;
class CPObject;
class CPButtonView;


//====================================================//
// CPPanel class definition
//====================================================//
class CPPanel {
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(gCockMemPool,size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
public:

	//====================================================//
	// My ID tag 
	//====================================================//

	int					mIdNum;
	int					mxPanelOffset;
	int					myPanelOffset;
	int               mHudFont;
	int               mMFDFont;
	int               mDEDFont;

	//====================================================//
	// Default Cursor Stuff
	//====================================================//
	RECT					mMouseBounds;
	PanelAdjStr			mAdjacentPanels;
	int					mDefaultCursor;

	//====================================================//
	// Camera Angles and Cockpit Masks
	//====================================================//

	float					mPan;
	float					mTilt;
	float					mMaskTop;
	ViewportBounds		*mpViewBounds[BOUNDS_TOTAL];
	int					mScale;

	//====================================================//
	// Information and Links to Surfaces
	//====================================================//

	int					mNumSurfaces;
	PanelSurfaceStr	*mpSurfaceData;

	//====================================================//
	// Geometry Stuff
	//====================================================//
	
	BOOL					mDoGeometry;

	//====================================================//
	// Information and Links to Objects 
	//====================================================//

	int					mNumObjects;
	int					*mpObjectIDs;
	int					mNumButtonViews;
	int					*mpButtonViewIDs;

	CPObject				**mpObjects;
	CPButtonView		**mpButtonViews;
   float             osbLocation[2][20][2];

	//====================================================//
	// Pointers to the Outside World
	//====================================================//

	ImageBuffer			*mpOTWImage;

	//====================================================//
	// Constructors and Destructors
	//====================================================//

	CPPanel(PanelInitStr*);
	virtual ~CPPanel();

	//====================================================//
	// Public Runtime Functions
	//====================================================//

	void	Exec(SimBaseClass*, int);
	void	CreateLitSurfaces(float);	// Added for TOD Effects 2/9/98
	void	DiscardLitSurfaces(void);	// Added for TOD Effects 2/9/98
	void	SetTOD(float);			// Added for TOD Effects 2/9/98
	void	DisplayBlit();
	void	DisplayDraw();
	BOOL	DoGeometry(void);					// Should we draw the wings and the reflections?
	BOOL	Dispatch(int*, int, int, int);
	BOOL	POVDispatch(int);
	void	SetDirtyFlags(void);
	BOOL	GetViewportBounds(ViewportBounds*, int);
	int   HudFont(void) {return mHudFont;};
	int   MFDFont(void) {return mMFDFont;};
	int   DEDFont(void) {return mDEDFont;};

	// OW
	void	DisplayBlit3D();
};

#endif
