#ifndef _BUTTON_H
#define _BUTTON_H

#ifndef _WINDOWS_
#include <windows.h>
#endif
#include "cpcb.h"
#include "cpmanager.h"

#ifdef USE_SH_POOLS
extern MEM_POOL gCockMemPool;
#endif

class CPButtonView;
class ImageBuffer;

#define _CPBUTTON_USE_STL_CONTAINERS

#ifdef _CPBUTTON_USE_STL_CONTAINERS
#include <vector>
#endif


//====================================================//
// Normal Button States
//====================================================//

#define CPBUTTON_OFF			0
#define CPBUTTON_ON			1



//====================================================
// Initialization Struct for:
//							CPButtonObject Class
//							CPButtonView Class
//====================================================

	typedef struct
		{
		int	objectId;
		int	callbackSlot;
		int	totalStates;
		int	normalState;
		int	totalViews;
		int	cursorIndex;
		int	delay;
		int	sound1;
		int	sound2;
		}
	ButtonObjectInitStr;


	typedef struct
		{
		int				objectId;
		int				parentButton;
		int				transparencyType;
		int				states;
		BOOL				persistant;
		RECT				destRect;
		RECT*				pSrcRect;	// List of Rects
		ImageBuffer*	pOTWImage;
		ImageBuffer*	pTemplate;
		int				scale;
		}
	ButtonViewInitStr;



//====================================================
// CPButtonObject Class Definition
//====================================================

class CPButtonObject {

	//----------------------------------------------------
	// Object Identification
	//----------------------------------------------------

	int				mIdNum;

	//----------------------------------------------------
	// Sound Info
	//----------------------------------------------------

	int				mSound1;
	int				mSound2;

	//----------------------------------------------------
	// State Information
	//----------------------------------------------------

	int				mDelay;
	int				mDelayInc;
	int				mTotalStates;
	int				mNormalState;
	int				mCurrentState;

	//----------------------------------------------------
	// Pointers and data for Runtime Callback functions
	//----------------------------------------------------

	int					mCallbackSlot;
	ButtonCallback		mTransAeroToState;
	ButtonCallback		mTransStateToAero;

	//----------------------------------------------------
	// Pointer to this buttons views
	//----------------------------------------------------

#ifdef _CPBUTTON_USE_STL_CONTAINERS
	std::vector<CPButtonView*> mpButtonView;		//List of views
#else
	CPButtonView**	mpButtonView;		//List of views

	int				mTotalViews;
	int				mViewSlot;			// Used only for adding views to button
#endif

	//----------------------------------------------------
	// Indicies for Cursor Information
	//----------------------------------------------------

	int				mCursorIndex;

public:

#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(gCockMemPool,size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif


	//----------------------------------------------------
	// Access Functions
	//----------------------------------------------------

	void				SetCurrentState(int);
	int				GetCurrentState();
	int				GetNormalState();
	int				GetCursorIndex();
	int				GetId();
	int				GetSound(int);
	void				SetSound(int, int);

	//----------------------------------------------------
	// Runtime Functions
	//----------------------------------------------------
	
	void				AddView(CPButtonView*);
	void				NotifyViews(void);
	void				HandleMouseEvent(int);
	void				HandleEvent(int);
	void				DecrementDelay(void);
	BOOL				DoBlit(void);
	void				UpdateStatus(void);

	//----------------------------------------------------
	// Constructors and Destructors
	//----------------------------------------------------

	CPButtonObject(ButtonObjectInitStr*);
	~CPButtonObject();
};



//====================================================
// CPButtonView Class Definition
//====================================================

class CPButtonView {

	//----------------------------------------------------
	// Identification Tag and Callback Ids
	//----------------------------------------------------

	int				mIdNum;
	int				mParentButton;
	int				mTransparencyType;
	RECT				mDestRect;
	BOOL				mDirtyFlag;
	BOOL				mPersistant;
	int				mStates;

	//----------------------------------------------------
	// Pointers to the Outside World
	//----------------------------------------------------

	ImageBuffer		*mpOTWImage;
	ImageBuffer		*mpTemplate;

	//----------------------------------------------------
	// Pointer to the Button Object that owns this view
	//----------------------------------------------------

	CPButtonObject	*mpButtonObject;

	//----------------------------------------------------
	// Source Locations for Template Surface
	//----------------------------------------------------

	RECT				*mpSrcRect;	// List of Rects
	int				mScale;

public:

#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(gCockMemPool,size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif

	//----------------------------------------------------
	// Runtime Member Functions
	//----------------------------------------------------

	void						DisplayBlit(void);
	BOOL						HandleEvent(int*, int, int, int);
	void						SetDirtyFlag() {mDirtyFlag = TRUE;};
	int						GetId();
	int						GetTransparencyType(void);
	int						GetParentButton(void);
	void						SetParentButtonPointer(CPButtonObject*);
	virtual void			CreateLit(void){};
	void						DiscardLit(void){};
	void						Translate(WORD*) {};
	void						Translate(DWORD*) {};
	void						UpdateView();

	//----------------------------------------------------
	// Constructors and Destructors
	//----------------------------------------------------

	CPButtonView(ButtonViewInitStr*);
	~CPButtonView();
};

#endif
